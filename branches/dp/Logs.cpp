//---------------------------------------------------------------------------

#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#include "Logs.h"
#include "Globals.h"

#include <stdio.h>

bool first= true;
char endstring[10]= "\n";

void __fastcall WriteConsoleOnly(char *str, double value)
{
    char buf[255];
    sprintf(buf,"%s %f \n",str,value);

//    stdout=  GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD wr= 0;
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),buf,strlen(buf),&wr,NULL);
    //WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),endstring,strlen(endstring),&wr,NULL);
}

void __fastcall WriteConsoleOnly(char *str)
{
//    printf("%n ffafaf /n",str);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_GREEN);
    DWORD wr= 0;
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),str,strlen(str),&wr,NULL);
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),endstring,strlen(endstring),&wr,NULL);
}

void __fastcall WriteLog(char *str, double value)
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
void __fastcall WriteLog(char *str)
{
 if (Global::bWriteLogEnabled)
  {
    if (str)
    {
        FILE *stream=NULL;
        if (first)
        {
            stream = fopen("log.txt", "w");
            first= false;
        }
        else
            stream = fopen("log.txt", "a+");
        fprintf(stream, str);
        fprintf(stream, "\n");
        fclose(stream);
        WriteConsoleOnly(str);
    }
  };
}

//---------------------------------------------------------------------------

#pragma package(smart_init)
