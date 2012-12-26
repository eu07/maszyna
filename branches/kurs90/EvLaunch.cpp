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
#include "parser.h"



//---------------------------------------------------------------------------

__fastcall TEventLauncher::TEventLauncher()
{
 DeltaTime=-1;
 UpdatedTime=0;
 fVal1=fVal2=0;
 szText=NULL;
 iCheckMask=0;
 MemCell=NULL;
 iHour=iMinute=-1; //takiego czasu nigdy nie bêdzie
}

__fastcall TEventLauncher::~TEventLauncher()
{
    SafeDeleteArray(szText);
}

void __fastcall TEventLauncher::Init()
{
}


bool __fastcall TEventLauncher::Load(cParser *parser)
{//wczytanie wyzwalacza zdarzeñ
 AnsiString str;
 std::string token;
 char *szKey;
 parser->getTokens();
 *parser >> dRadius; //promieñ dzia³ania
 if (dRadius>0.0)
  dRadius*=dRadius; //do kwadratu, pod warunkiem, ¿e nie jest ujemne
 parser->getTokens(); //klawisz steruj¹cy
 *parser >> token;
 str=AnsiString(token.c_str());
 if (str!="none")
 {
  szKey=new char[1];
  strcpy(szKey,str.c_str()); //Ra: to jest nieco niezwyk³e
  iKey=VkKeyScan(szKey[0]);
 }
 else
  iKey=0;
 parser->getTokens();
 *parser >> DeltaTime;
 if (DeltaTime<0)
  DeltaTime=-DeltaTime; //dla ujemnego zmieniamy na dodatni
 else if (DeltaTime>0)
 {//wartoœæ dodatnia oznacza wyzwalanie o okreœlonej godzinie
  iMinute=int(DeltaTime)%100; //minuty s¹ najm³odszymi cyframi dziesietnymi
  iHour=int(DeltaTime-iMinute)/100; //godzina to setki
  DeltaTime=0; //bez powtórzeñ
  WriteLog("EventLauncher at "+IntToStr(iHour)+":"+IntToStr(iMinute)); //wyœwietlenie czasu
 }
 parser->getTokens();
 *parser >> token;
 asEvent1Name=AnsiString(token.c_str()); //pierwszy event
 parser->getTokens();
 *parser >> token;
 asEvent2Name=AnsiString(token.c_str()); //drugi event

 parser->getTokens();
 *parser >> token;
 str=AnsiString(token.c_str());
 if (str==AnsiString("condition"))
 {//obs³uga wyzwalania warunkowego
  parser->getTokens();
  *parser >> token;
  asMemCellName=AnsiString(token.c_str());
  parser->getTokens();
  *parser >> token;
  SafeDeleteArray(szText);
  szText=new char[256];
  strcpy(szText,token.c_str());
  if (token.compare("*")!=0)       //*=nie brac command pod uwage
    iCheckMask|=conditional_memstring;
  parser->getTokens();
  *parser >> token;
  if (token.compare("*")!=0)       //*=nie brac command pod uwage
  {
   iCheckMask|=conditional_memval1;
   str= AnsiString(token.c_str());
   fVal1= str.ToDouble();
  }
  else fVal1= 0;
  parser->getTokens();
  *parser >> token;
  if (token.compare("*")!=0)       //*=nie brac command pod uwage
  {
   iCheckMask|=conditional_memval2;
   str=AnsiString(token.c_str());
   fVal2=str.ToDouble();
  }
  else fVal2=0;
  parser->getTokens(); //s³owo zamykaj¹ce
  *parser >> token;
 }
 return true;
};

bool __fastcall TEventLauncher::Render()
{//"renderowanie" wyzwalacza
 bool bCond=false;
 if (iKey!=0)
 {
  bCond=(Pressed(iKey)); //czy klawisz wciœniêty
 }
 if (DeltaTime>0)
 {
  if (UpdatedTime>DeltaTime)
  {
   UpdatedTime=0; //naliczanie od nowa
   bCond=true;
  }
  else
   UpdatedTime+=Timer::GetDeltaTime(); //aktualizacja naliczania czasu
 }
 if (GlobalTime->hh==iHour)
 {if (GlobalTime->mm==iMinute)
  {//zgodnoœæ czasu uruchomienia
   if (UpdatedTime<10)
   {
    UpdatedTime=20; //czas do kolejnego wyzwolenia?
    bCond=true;
   }
  }
 }
 else
  UpdatedTime=1;
 if (bCond) //jeœli spe³niony zosta³ warunek
 {
  if ((iCheckMask!=0)&&MemCell) //sprawdzanie warunku na komórce pamiêci
   bCond=MemCell->Compare(szText,fVal1,fVal2,iCheckMask);
 }
 return bCond;  //sprawdzanie dRadius w Ground.cpp
}

bool __fastcall TEventLauncher::IsGlobal()
{//sprawdzenie, czy jest globalnym wyzwalaczem czasu
 if (DeltaTime==0)
  if (iHour>=0)
   if (iMinute>=0)
    if (dRadius<0.0) //bez ograniczenia zasiêgu
     return true;
 return false;
};
//---------------------------------------------------------------------------

#pragma package(smart_init)
