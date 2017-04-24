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

std::ofstream output; // standardowy "log.txt", można go wyłączyć
std::ofstream errors; // lista błędów "errors.txt", zawsze działa
std::ofstream comms; // lista komunikatow "comms.txt", można go wyłączyć

char endstring[10] = "\n";

std::string filename_date() {

    ::SYSTEMTIME st;
    ::GetLocalTime( &st );
    char buffer[ 256 ];
    sprintf( buffer,
        "%d%02d%02d_%02d%02d",
        st.wYear,
        st.wMonth,
        st.wDay,
        st.wHour,
        st.wMinute );

    return std::string( buffer );
}

std::string filename_scenery() {

    auto extension = Global::SceneryFile.rfind( '.' );
    if( extension != std::string::npos ) {
        return Global::SceneryFile.substr( 0, extension );
    }
    else {
        return Global::SceneryFile;
    }
}

void WriteConsoleOnly(const char *str, double value)
{
    char buf[255];
    sprintf(buf, "%s %f \n", str, value);
    // stdout=  GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD wr = 0;
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), buf, (DWORD)strlen(buf), &wr, NULL);
    // WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),endstring,strlen(endstring),&wr,NULL);
}

void WriteConsoleOnly(const char *str, bool newline)
{
    // printf("%n ffafaf /n",str);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    DWORD wr = 0;
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), str, (DWORD)strlen(str), &wr, NULL);
    if (newline)
        WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), endstring, (DWORD)strlen(endstring), &wr, NULL);
}

void WriteLog(const char *str, double value)
{
    if (Global::iWriteLogEnabled)
    {
        if (str)
        {
            char buf[255];
            sprintf(buf, "%s %f", str, value);
            WriteLog(buf);
        }
    }
};
void WriteLog(const char *str, bool newline)
{
    if (str)
    {
        if (Global::iWriteLogEnabled & 1)
        {
            if( !output.is_open() ) {
                
                std::string const filename =
                    ( Global::MultipleLogs ?
                    "logs/log (" + filename_scenery() + ") " + filename_date() + ".txt" :
                    "log.txt" );
                output.open( filename, std::ios::trunc );
            }
            output << str;
            if (newline)
                output << "\n";
            output.flush();
        }
        // hunter-271211: pisanie do konsoli tylko, gdy nie jest ukrywana
        if (Global::iWriteLogEnabled & 2)
            WriteConsoleOnly(str, newline);
    }
};

void ErrorLog(const char *str)
{ // Ra: bezwarunkowa rejestracja poważnych błędów
    if (!errors.is_open()) {

        std::string const filename =
            ( Global::MultipleLogs ?
            "logs/errors (" + filename_scenery() + ") " + filename_date() + ".txt" :
            "errors.txt" );
        errors.open( filename, std::ios::trunc );
        errors << "EU07.EXE " + Global::asRelease << "\n";
    }
    if (str)
        errors << str;
    errors << "\n";
    errors.flush();
};

void Error(const std::string &asMessage, bool box)
{
    // if (box)
    //	MessageBox(NULL, asMessage.c_str(), string("EU07 " + Global::asRelease).c_str(), MB_OK);
    ErrorLog(asMessage.c_str());
}

void Error(const char *&asMessage, bool box)
{
    // if (box)
    //	MessageBox(NULL, asMessage, string("EU07 " + Global::asRelease).c_str(), MB_OK);
    ErrorLog(asMessage);
    WriteLog(asMessage);
}

void ErrorLog(const std::string &str, bool newline)
{
    ErrorLog(str.c_str());
    WriteLog(str.c_str(), newline);
}

void WriteLog(const std::string &str, bool newline)
{ // Ra: wersja z AnsiString jest zamienna z Error()
    WriteLog(str.c_str(), newline);
};

void CommLog(const char *str)
{ // Ra: warunkowa rejestracja komunikatów
    WriteLog(str);
    /*    if (Global::iWriteLogEnabled & 4)
    {
    if (!comms.is_open())
    {
    comms.open("comms.txt", std::ios::trunc);
    comms << AnsiString("EU07.EXE " + Global::asRelease).c_str() << "\n";
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
