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
#include <deque>

std::ofstream output; // standardowy "log.txt", można go wyłączyć
std::ofstream errors; // lista błędów "errors.txt", zawsze działa
std::ofstream comms; // lista komunikatow "comms.txt", można go wyłączyć
char logbuffer[ 256 ];

char endstring[10] = "\n";

std::deque<std::string> log_scrollback;

std::string filename_date() {
    ::SYSTEMTIME st;

#ifdef __unix__
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
	    "%d%02d%02d_%02d%02d%03d",
        st.wYear,
        st.wMonth,
        st.wDay,
        st.wHour,
	    st.wMinute,
	    st.wMilliseconds);

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

// log service stacks
std::deque < std::pair<std::string, bool>> InfoStack;
std::deque<std::string> ErrorStack;

// lock for log stacks
std::mutex logMutex;


void LogService()
{
	while (!Global.applicationQuitOrder)
	{
		{
			// --- Obsługa InfoStack ---
			while (!InfoStack.empty())
			{
				logMutex.lock();
				std::string msg = InfoStack.front().first;
				bool isError = InfoStack.front().second;
				InfoStack.pop_front();
				logMutex.unlock();

				// log to file
				if (Global.iWriteLogEnabled & 1)
				{
					if (!output.is_open())
					{
						std::string filename = (Global.MultipleLogs ? "logs/log (" + filename_scenery() + ") " + filename_date() + ".txt" : "log.txt");
						output.open(filename, std::ios::trunc);
					}
					output << msg << "\n";
					output.flush();
				}

				// log to scrollback imgui
				log_scrollback.emplace_back(msg);
				if (log_scrollback.size() > 200)
					log_scrollback.pop_front();

				// log to console
				if (Global.iWriteLogEnabled & 2)
				{
					if (isError)
						printf("\033[1;37;41m%s\033[0m\n", msg.c_str());
					else
						printf("\033[32m%s\033[0m\n", msg.c_str());
				}
			}

			// --- Obsługa ErrorStack ---
			while (!ErrorStack.empty())
			{
				logMutex.lock();
				std::string msg = ErrorStack.front();
				ErrorStack.pop_front();
				logMutex.unlock();

				if (!(Global.iWriteLogEnabled & 1))
					continue;

				if (!errors.is_open())
				{
					std::string filename = (Global.MultipleLogs ? "logs/errors (" + filename_scenery() + ") " + filename_date() + ".txt" : "errors.txt");
					errors.open(filename, std::ios::trunc);
					errors << "EU07.EXE " + Global.asVersion << "\n";
				}

				errors << msg << "\n";
				errors.flush();
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
}


void WriteLog(const char *str, logtype const Type, bool isError)
{
	if (!str || *str == '\0')
		return;
	if (TestFlag(Global.DisabledLogTypes, static_cast<unsigned int>(Type)))
		return;
	logMutex.lock();
	InfoStack.emplace_back(str, isError);
	logMutex.unlock();
}

void ErrorLog(const char *str, logtype const Type)
{
	if (!str || *str == '\0')
		return;
	if (TestFlag(Global.DisabledLogTypes, static_cast<unsigned int>(Type)))
		return;
	logMutex.lock();
	ErrorStack.emplace_back(str);
	logMutex.unlock();
}


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
    WriteLog( str.c_str(), Type, true );
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
