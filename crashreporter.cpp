#include <client/crash_report_database.h>
#include <client/settings.h>
#include <client/crashpad_client.h>
#include <client/annotation.h>
#include <fstream>
#include <string>
#include <filesystem>
#include <iostream>
#include "version_info.h"

#if defined __has_attribute
#  if __has_attribute (init_priority)
#    define INITPRIO_CLASS __attribute__ ((init_priority (5000)))
#  endif
#endif

#ifndef INITPRIO_CLASS
#   ifdef _MSC_VER
#pragma init_seg(lib)
#   endif
#endif

#ifndef INITPRIO_CLASS
#   define INITPRIO_CLASS
#endif

class crash_reporter
{
	crashpad::CrashpadClient client;
    std::unique_ptr<crashpad::CrashReportDatabase> database;

public:
    std::string provider = "";
    bool autoupload = false;

	crash_reporter();
    void set_autoupload();
    void upload_reject();
    void upload_accept();
    bool upload_pending();
};

crash_reporter::crash_reporter()
{
#ifdef _WIN32
    if (!std::filesystem::exists("crashdumps/crashpad_handler.exe"))
        return;
#else
    if (!std::filesystem::exists("crashdumps/crashpad_handler"))
        return;
#endif

    autoupload = std::filesystem::exists("crashdumps/autoupload_enabled.conf");

    std::string url, token, prov;

    std::ifstream conf("crashdumps/crashpad.conf");

    std::string line, param, value;
    while (std::getline(conf, line)) {
        std::istringstream linestream(line);

        if (!std::getline(linestream, param, '='))
            continue;
        if (!std::getline(linestream, value, '='))
            continue;

        if (param == "URL")
            url = value;
        else if (param == "TOKEN")
            token = value;
        else if (param == "PROVIDER")
            prov = value;
    }

    if (token.empty())
        return;
    if (url.empty())
        return;
    if (prov.empty())
        return;

#ifdef _WIN32
    base::FilePath db(L"crashdumps");
    base::FilePath handler(L"crashdumps/crashpad_handler.exe");
#else
    base::FilePath db("crashdumps");
    base::FilePath handler("crashdumps/crashpad_handler");
#endif

	std::map<std::string, std::string> annotations;
	annotations["git_hash"] = GIT_HASH;
	annotations["src_date"] = SRC_DATE;
    annotations["format"] = "minidump";
    annotations["token"] = token;

	std::vector<std::string> arguments;
	arguments.push_back("--no-rate-limit");

    database = crashpad::CrashReportDatabase::Initialize(db);

	if (database == nullptr || database->GetSettings() == NULL)
        return;

    std::vector<crashpad::CrashReportDatabase::Report> reports;
    database->GetCompletedReports(&reports);
    for (auto const &report : reports)
        if (report.uploaded)
            database->DeleteReport(report.uuid);

    database->GetSettings()->SetUploadsEnabled(autoupload);

    if (!client.StartHandler(handler, db, db, url, annotations, arguments, true, true))
        return;

    provider = prov;
}

void crash_reporter::set_autoupload()
{
    autoupload = true;
    database->GetSettings()->SetUploadsEnabled(true);
    std::ofstream flag("crashdumps/autoupload_enabled.conf");
    flag << "y" << std::endl;
    flag.close();
}

void crash_reporter::upload_accept()
{
    std::vector<crashpad::CrashReportDatabase::Report> reports;
    database->GetCompletedReports(&reports);
    for (auto const &report : reports)
        if (!report.uploaded)
            database->RequestUpload(report.uuid);
}

void crash_reporter::upload_reject()
{
    std::vector<crashpad::CrashReportDatabase::Report> reports;
    database->GetCompletedReports(&reports);
    for (auto const &report : reports)
        if (!report.uploaded)
            database->DeleteReport(report.uuid);
}

bool crash_reporter::upload_pending()
{
    if (autoupload)
        return false;

    std::vector<crashpad::CrashReportDatabase::Report> reports;
    database->GetCompletedReports(&reports);
    for (auto const &report : reports)
        if (!report.uploaded && !report.upload_explicitly_requested)
            return true;

    return false;
}

crash_reporter crash_reporter_inst INITPRIO_CLASS;

const std::string& crashreport_get_provider()
{
    return crash_reporter_inst.provider;
}

void crashreport_add_info(const char *name, const std::string &value)
{
	char *copy = new char[value.size() + 1];
	strcpy(copy, value.c_str());

	crashpad::Annotation *annotation = new crashpad::Annotation(crashpad::Annotation::Type::kString, name, copy);
	annotation->SetSize(value.size() + 1);
}

void crashreport_set_autoupload()
{
    crash_reporter_inst.set_autoupload();
}

bool crashreport_is_pending()
{
    if (crash_reporter_inst.provider.empty())
        return false;
    return crash_reporter_inst.upload_pending();
}

void crashreport_upload_reject()
{
    crash_reporter_inst.upload_reject();
}

void crashreport_upload_accept()
{
    crash_reporter_inst.upload_accept();
}
