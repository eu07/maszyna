//---------------------------------------------------------------------------

#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#include "Logs.h"
#include "Globals.h"
//#include "loader.h"
//#include "qutils.h"

#include <stdio.h>

bool first= true;
bool qfirstdebug= true;
bool qdpfirst= true;
bool qswlfirst= true;
bool qwarfirst= true;
bool qtexfirst= true;

int cc = 0;
int LASTFID = -1;


char endstring[10]= "\n";

void __fastcall WriteConsoleOnly(char *str, double value)
{
    char buf[255];
    sprintf(buf,"%s %f \n",str,value);

//    stdout=  GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD wr= 0;
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),buf,strlen(buf),&wr,NULL);
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),endstring,strlen(endstring),&wr,NULL);
}

void __fastcall WriteConsoleOnly(char *str)
{
//    printf("%n ffafaf /n",str);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_GREEN);
    DWORD wr= 0;
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),str,strlen(str),&wr,NULL);
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),endstring,strlen(endstring),&wr,NULL);
}

void __fastcall WriteLog(AnsiString str, double value)
{
 if (Global::bWriteLogEnabled)
  {
    if (str != "")
    {
        char buf[255];
        sprintf(buf,"%s %f",str,value);
        WriteLog(buf);
    }
  };
}
void __fastcall WriteLog(AnsiString str)
{

 if (Global::bWriteLogEnabled)
  {
    if (str != "")
    {
        FILE *stream=NULL;
        if (first)
        {
            stream = fopen("log.txt", "w");
            first= false;
            //Global::gtc1 =  GetTickCount();
        }
        else
        stream = fopen("log.txt", "a+");
        fprintf(stream, str.c_str());
        fprintf(stream, "\n");
        fclose(stream);
        WriteConsoleOnly(str.c_str());
        
        //if (Global::ircradio) Global::SKT->socket_send_line(Global::s, str.c_str(), 0);
    }
  };
}

void __fastcall QueuedLog(AnsiString qstr)
{

FILE *stream;

if (Global::bQueuedAdvLog)
{
 if (qfirstdebug)
        {
        stream = fopen("data\\logs\\debug.txt", "w");
        qfirstdebug= false;
        }
        else
   stream = fopen("data\\logs\\debug.txt", "a+");
   fprintf(stream, "%s", qstr);
   fprintf(stream, "\n");
   fclose(stream);
 }
}

void __fastcall QueuedTexturesList(AnsiString qstr)
{

FILE *stream;

//if (Global::bQueuedAdvLog)
//{
 if (qtexfirst)
        {
        stream = fopen("data\\logs\\textures.txt", "w");
        qtexfirst= false;
        }
        else
   stream = fopen("data\\logs\\textures.txt", "a+");
   fprintf(stream, "%s", qstr);
   fprintf(stream, "\n");
   fclose(stream);
// }
}

bool checklogforid(int FID)
{
// if (FID == 101) return Global::DEBUG_101;
// if (FID == 102) return Global::DEBUG_102;
// if (FID == 103) return Global::DEBUG_103;
// if (FID == 104) return Global::DEBUG_104;
// if (FID == 105) return Global::DEBUG_105;
// if (FID == 106) return Global::DEBUG_106;
// if (FID == 107) return Global::DEBUG_107;
// if (FID == 108) return Global::DEBUG_108;
// if (FID == 109) return Global::DEBUG_109;
// if (FID == 110) return Global::DEBUG_110;
// if (FID == 111) return Global::DEBUG_111;
// if (FID == 112) return Global::DEBUG_112;
// if (FID == 113) return Global::DEBUG_113;
// if (FID == 120) return Global::DEBUG_120;
}

__fastcall DEBUG(AnsiString PAR, int FID, int NUM)
{

 AnsiString N = "";
 AnsiString P = PAR;
 bool LOGON;

      //Global::DEBUG_101 = true;
      //Global::DEBUG_103 = true;
      //Global::DEBUG_104 = true;
      //Global::DEBUG_105 = true;
      //Global::DEBUG_112 = true;
      //Global::DEBUG_113 = 0;
      //Global::DEBUG_120 = true;
      LOGON = checklogforid(FID);

 if ( LOGON )
     {
  if (P != "") PAR = " << " + PAR;

      if (FID == 101) N = "TWorld::Update()";
      if (FID == 102) N = "TGround::Update()";
      if (FID == 103) N = "TDynamicObject::FastUpdate()";
      if (FID == 104) N = "TDynamicObject::Update()";
      if (FID == 105) N = "TWorld::Render()";
      if (FID == 106) N = "TGround::Render()";
      if (FID == 107) N = "TGroundNode::Render()";
      if (FID == 108) N = "TGroundNode::RenderAlpha()";
      if (FID == 109) N = "TGround::Render2()";
      if (FID == 110) N = "TGroundNode::Render2()";
      if (FID == 111) N = "TAnimModel::Render2()";
      if (FID == 112) N = "TTrain::Update()";
      if (FID == 113) N = "TTrain::OnKeyPress()";
      if (FID == 120) N = "TGround::CheckQuery()";

      if (NUM == 1) WriteLog(N + " - " + IntToStr(NUM) + PAR );
      if (NUM != 1) WriteLog(N + " - " + IntToStr(NUM));
     }

 }
//---------------------------------------------------------------------------

#pragma package(smart_init)
