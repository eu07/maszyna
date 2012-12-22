//---------------------------------------------------------------------------

/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak and others

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#include "Mover.hpp"
#include "mctools.hpp"
#include "mtable.hpp"
#include "Timer.h"
#include "Globals.h"
#include "EvLaunch.h"
#include "Event.h"

#include "Usefull.h"
#include "MemCell.h"



//---------------------------------------------------------------------------

__fastcall TEventLauncher::TEventLauncher()
{
    DeltaTime= -1;
    UpdatedTime= 0;
    fVal1=fVal2= 0;
    szText= NULL;
    iCheckMask= 0;
    MemCell= NULL;    
}

__fastcall TEventLauncher::~TEventLauncher()
{
    SafeDeleteArray(szText);
}

void __fastcall TEventLauncher::Init()
{
}


bool __fastcall TEventLauncher::Load(cParser *parser)
{
    AnsiString str;
    std::string token;
    char *szKey;
    parser->getTokens();
    *parser >> dRadius;
    dRadius*= dRadius;
    parser->getTokens();
    *parser >> token;
    str= AnsiString(token.c_str());
    if (str!=AnsiString("none"))
     {
//       SafeDeleteArray(szKey);
       szKey= new char[1];
       strcpy(szKey,str.c_str());
       iKey=VkKeyScan(szKey[0]);
     }
    else
     iKey= 0;

    parser->getTokens();
    *parser >> DeltaTime;
    if (DeltaTime<0)
      DeltaTime= -DeltaTime;
    else if (DeltaTime>0)
      {
       WriteLog(IntToStr(int(DeltaTime)).c_str());
       mm=int(DeltaTime)%100;
       WriteLog(IntToStr(mm).c_str());
       hh=int(DeltaTime-mm)/100;
       WriteLog(IntToStr(hh).c_str());
       DeltaTime=0;

      }

    parser->getTokens();
    *parser >> token;
    asEvent1Name= AnsiString(token.c_str());
    parser->getTokens();
    *parser >> token;
    asEvent2Name= AnsiString(token.c_str());

    parser->getTokens();
    *parser >> token;
    str= AnsiString(token.c_str());
    if (str==AnsiString("condition"))
     {
      parser->getTokens();
      *parser >> token;
      asMemCellName= AnsiString(token.c_str());
      parser->getTokens();
      *parser >> token;
      SafeDeleteArray(szText);
      szText= new char[256];
      strcpy(szText,token.c_str());
      if (token.compare( "*" ) != 0)       //*=nie brac command pod uwage
        iCheckMask|=conditional_memstring;
      parser->getTokens();
      *parser >> token;
      if (token.compare( "*" ) != 0)       //*=nie brac command pod uwage
       {
         iCheckMask|=conditional_memval1;
         str= AnsiString(token.c_str());
         fVal1= str.ToDouble();
       }
      else fVal1= 0;
      parser->getTokens();
      *parser >> token;
      if (token.compare( "*" ) != 0)       //*=nie brac command pod uwage
       {
         iCheckMask|=conditional_memval2;
         str= AnsiString(token.c_str());
         fVal2= str.ToDouble();
       }
      else fVal2= 0;
      parser->getTokens(); *parser >> token;
     }
    return true;
}


bool __fastcall TEventLauncher::Render()
{
    bool bCond=false;
    if (iKey!=0)
     {
       bCond= (Pressed(iKey));
     }
    if (DeltaTime>0)
     {
       if (UpdatedTime>DeltaTime)
        {
         UpdatedTime=0;
         bCond=true;
        }
       else
        UpdatedTime+= Timer::GetDeltaTime();
     }
    if ((GlobalTime->hh == hh) && (GlobalTime->mm == mm))
     {
     if (UpdatedTime<10)
        {
         UpdatedTime=20;
         bCond=true;
        }
     }
    else
     UpdatedTime=1;
     
    if (bCond)
      {
        if (iCheckMask!=0 && MemCell!=NULL)
          bCond= MemCell->Compare(szText,fVal1,fVal2,iCheckMask);
      }

    return bCond;  //sprawdzanie dRadius w Ground.cpp
}

//---------------------------------------------------------------------------

#pragma package(smart_init)
