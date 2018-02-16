/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "Logs.h"

#include "Globals.h"
#include "winheaders.h"
#include "utilities.h"
#include "uilayer.h"

std::shared_ptr<ui_panel> ui_log = std::make_shared<ui_panel>( 20, 140 );

std::ofstream output; // standardowy "log.txt", można go wyłączyć
std::ofstream errors; // lista błędów "errors.txt", zawsze działa
std::ofstream comms; // lista komunikatow "comms.txt", można go wyłączyć
char logbuffer[ 256 ];

char endstring[10] = "\n";

std::string filename_date() {
    ::SYSTEMTIME st;

#ifdef __linux__
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    tm *tms = localtime(&ts.tv_sec);
    st.wYear = tms->tm_year;
    st.wMonth = tms->tm_mon;
    st.wDayOfWeek = tms->tm_wday;
    st.wDay = tms->tm_mday;
    st.wHour = tms->tm_hour;
    st.wMinute = tms->tm_min;
    st.wSecond = tms->tm_sec;
    st.wMilliseconds = ts.tv_nsec / 1000000;
#elif _WIN32
    ::GetLocalTime( &st );
#endif

    std::snprintf(
        logbuffer,
        sizeof(logbuffer),
        "%d%02d%02d_%02d%02d",
        st.wYear,
        st.wMonth,
        st.wDay,
        st.wHour,
        st.wMinute );

    return std::string( logbuffer );
}

std::string filename_scenery() {

    auto extension = Global.SceneryFile.rfind( '.' );
    if( extension != std::string::npos ) {
        return Global.SceneryFile.substr( 0, extension );
    }
    else {
        return Global.SceneryFile;
    }
}

void WriteLog( const char *str, logtype const Type ) {

    if( str == nullptr ) { return; }
    if( true == TestFlag( Global.DisabledLogTypes, Type ) ) { return; }

    if (Global.iWriteLogEnabled & 1) {
        if( !output.is_open() ) {

            std::string const filename =
                ( Global.MultipleLogs ?
                    "logs/log (" + filename_scenery() + ") " + filename_date() + ".txt" :
                    "log.txt" );
            output.open( filename, std::ios::trunc );
        }
        output << str << "\n";
        output.flush();
    }

	ui_log->text_lines.emplace_back(std::string(str), Global.UITextColor);
	if (ui_log->text_lines.size() > 20)
		ui_log->text_lines.pop_front();

#ifdef _WIN32
    if( Global.iWriteLogEnabled & 2 ) {
        // hunter-271211: pisanie do konsoli tylko, gdy nie jest ukrywana
        SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), FOREGROUND_GREEN | FOREGROUND_INTENSITY );
        DWORD wr = 0;
        WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), str, (DWORD)strlen( str ), &wr, NULL );
        WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), endstring, (DWORD)strlen( endstring ), &wr, NULL );
    }
#else
	printf("%s\n", str);
#endif
}

// Ra: bezwarunkowa rejestracja poważnych błędów
void ErrorLog( const char *str, logtype const Type ) {

    if( str == nullptr ) { return; }
    if( true == TestFlag( Global.DisabledLogTypes, Type ) ) { return; }

    if (!errors.is_open()) {

        std::string const filename =
            ( Global.MultipleLogs ?
                "logs/errors (" + filename_scenery() + ") " + filename_date() + ".txt" :
                "errors.txt" );
        errors.open( filename, std::ios::trunc );
        errors << "EU07.EXE " + Global.asVersion << "\n";
    }

    errors << str << "\n";
    errors.flush();
};

void Error(const std::string &asMessage, bool box)
{
    // if (box)
    //	MessageBox(NULL, asMessage.c_str(), string("EU07 " + Global.asRelease).c_str(), MB_OK);
    ErrorLog(asMessage.c_str());
}

void Error(const char *&asMessage, bool box)
{
    // if (box)
    //	MessageBox(NULL, asMessage, string("EU07 " + Global.asRelease).c_str(), MB_OK);
    ErrorLog(asMessage);
    WriteLog(asMessage);
}

void ErrorLog(const std::string &str, logtype const Type )
{
    ErrorLog( str.c_str(), Type );
    WriteLog( str.c_str(), Type );
}

void WriteLog(const std::string &str, logtype const Type )
{ // Ra: wersja z AnsiString jest zamienna z Error()
    WriteLog( str.c_str(), Type );
};

void CommLog(const char *str)
{ // Ra: warunkowa rejestracja komunikatów
    WriteLog(str);
    /*    if (Global.iWriteLogEnabled & 4)
    {
    if (!comms.is_open())
    {
    comms.open("comms.txt", std::ios::trunc);
    comms << AnsiString("EU07.EXE " + Global.asRelease).c_str() << "\n";
    }
    if (str)
    comms << str;
    comms << "\n";
    comms.flush();
    }*/
};

void CommLog(const std::string &str)
{ // Ra: wersja z AnsiString jest zamienna z Error()
    WriteLog(str);
};

//---------------------------------------------------------------------------
