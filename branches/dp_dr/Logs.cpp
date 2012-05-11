//---------------------------------------------------------------------------

#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#include "Logs.h"
#include "Globals.h"

#include <stdio.h>
#include <iostream>
#include <fstream>

std::ofstream output; //standardowy "log.txt", mo�na go wy��czy�
std::ofstream errors; //lista b��d�w "errors.txt", zawsze dziala

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
        if (!output.is_open())
            output.open("log.txt", std::ios::trunc);

        output << str << "\n";
        output.flush();

        //hunter-271211: pisanie do konsoli tylko, gdy nie jest ukrywana
        if (Global::bHideConsole==false)
          WriteConsoleOnly(str);

    }
  };
}
void __fastcall ErrorLog(const char* str)
{//Ra: bezwarunkowa rejestracja powa�nych b��d�w
 if (!errors.is_open())
 {errors.open("errors.txt",std::ios::trunc);
  errors << AnsiString("EU07.EXE "+Global::asRelease).c_str() << "\n";
 }
 if (str) errors << str;
 errors << "\n";
 errors.flush();
};

void __fastcall Error(const AnsiString &asMessage,bool box)
{
 if (box)
  MessageBox(NULL,asMessage.c_str(),AnsiString("EU07 "+Global::asRelease).c_str(),MB_OK);
 WriteLog(asMessage.c_str());
}
void __fastcall ErrorLog(const AnsiString &asMessage)
{//zapisywanie b��d�w "errors.txt"
 ErrorLog(asMessage.c_str());
 WriteLog(asMessage.c_str()); //do "log.txt" ewentualnie te�
}

void __fastcall WriteLog(const AnsiString &str)
{//Ra: wersja z AnsiString jest zamienna z Error()
 WriteLog(str.c_str());
};
//---------------------------------------------------------------------------

#pragma package(smart_init)
