/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
/*
MaSzyna EU07 locomotive simulator
Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others
*/
/*
Authors:
MarcinW, McZapkie, Shaxbee, ABu, nbmx, youBy, Ra, winger, mamut, Q424,
Stele, firleju, szociu, hunter, ZiomalCl, OLI_EU and others
*/

#include "stdafx.h"

#include "application.h"
#include "Logs.h"
#include <cstdlib>
#ifdef WITHDUMPGEN
#ifdef _WIN32
#include <Windows.h>
#include <DbgHelp.h>
#endif
#endif

#ifdef _MSC_VER
#pragma comment(linker, "/subsystem:console /ENTRY:mainCRTStartup")
#endif

void export_e3d_standalone(std::string in, std::string out, int flags, bool dynamic);

#ifdef WITHDUMPGEN
#include <windows.h>
#include <dbghelp.h>
#include <ctime>
#include <string>
#include <sstream>
#include <iomanip>
#include <Globals.h>

#ifdef _WIN32
#pragma comment(lib, "Dbghelp.lib")

LONG WINAPI CrashHandler(EXCEPTION_POINTERS *ExceptionInfo)
{
	// Get current local time
	SYSTEMTIME st;
	GetLocalTime(&st);

	// Format: crash_YYYY-MM-DD_HH-MM-SS.dmp
	std::ostringstream oss;
	oss << "crash_" << std::setw(4) << std::setfill('0') << st.wYear << "-" << std::setw(2) << std::setfill('0') << st.wMonth << "-" << std::setw(2) << std::setfill('0') << st.wDay << "_"
	    << std::setw(2) << std::setfill('0') << st.wHour << "-" << std::setw(2) << std::setfill('0') << st.wMinute << "-" << std::setw(2) << std::setfill('0') << st.wSecond << ".dmp";

	std::string filename = oss.str();

	HANDLE hFile = CreateFileA(filename.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
		dumpInfo.ThreadId = GetCurrentThreadId();
		dumpInfo.ExceptionPointers = ExceptionInfo;
		dumpInfo.ClientPointers = FALSE;

		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpWithDataSegs, &dumpInfo, nullptr, nullptr);
		CloseHandle(hFile);
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

#endif


int main(int argc, char *argv[])
{
#ifdef WITHDUMPGEN
#ifdef _WIN32
	SetUnhandledExceptionFilter(CrashHandler);
	#endif
#endif
	// init start timestamp
	Global.startTimestamp = std::chrono::steady_clock::now();

    // quick short-circuit for standalone e3d export
    if (argc == 6 && std::string(argv[1]) == "-e3d") {
        std::string in(argv[2]);
        std::string out(argv[3]);
        int flags = std::stoi(std::string(argv[4]));
        int dynamic = std::stoi(std::string(argv[5]));
        export_e3d_standalone(in, out, flags, dynamic);
    } else {
        try {
            auto result { Application.init( argc, argv ) };
            if( result == 0 ) {
                result = Application.run();
                Application.exit();
            }
        } catch( std::bad_alloc const &Error ) {
            ErrorLog( "Critical error, memory allocation failure: " + std::string( Error.what() ) );
        }
    }
#ifndef _WIN32
    fflush(stdout);
    fflush(stderr);
#endif
	std::_Exit(0); // skip destructors, there are ordering errors which causes segfaults
}
