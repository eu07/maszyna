//---------------------------------------------------------------------------

#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#include "Logs.h"
#include "Globals.h"

#include <stdio.h>
#include <iostream>
#include <fstream>

std::ofstream output;

bool first= true;
char endstring[10]= "\n";

void __fastcall WriteConsoleOnly(const char* str, double value)
{
    char buf[255];
    sprintf(buf,"%s %f \n",str,value);

//    stdout=  GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD wr= 0;
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),buf,strlen(buf),&wr,NULL);
    //WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),endstring,strlen(endstring),&wr,NULL);
}

void __fastcall WriteConsoleOnly(const char* str)
{
//    printf("%n ffafaf /n",str);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_GREEN);
    DWORD wr= 0;
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),str,strlen(str),&wr,NULL);
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),endstring,strlen(endstring),&wr,NULL);
}

void __fastcall WriteLog(const char* str, double value)
{
 if (Global::bWriteLogEnabled)
  {
    if (str)
    {
        char buf[255];
        sprintf(buf,"%s %f",str,value);
        WriteLog(buf);
    }
  };
}
void __fastcall WriteLog(const char* str)
{
 if (Global::bWriteLogEnabled)
  {
    if (str)
    {

        if(!output)
            output.open("log.txt", std::ios::trunc);

        output << str << "\n";
        output.flush();

        //hunter-271211: pisanie do konsoli tylko, gdy nie jest ukrywana
        if (Global::bHideConsole==false)
          WriteConsoleOnly(str);

    }
  };
}

void __fastcall Error(const AnsiString &asMessage,bool box)
{
 if (box)
  MessageBox(NULL,asMessage.c_str(),"EU07-424",MB_OK);
 WriteLog(asMessage.c_str());
}
void __fastcall WriteLog(const AnsiString &str)
{//Ra: wersja z AnsiString jest zamienna z Error()
 WriteLog(str.c_str());
};
//---------------------------------------------------------------------------

#pragma package(smart_init)