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

        WriteConsoleOnly(str);
    }
  };
}

//---------------------------------------------------------------------------

#pragma package(smart_init)
