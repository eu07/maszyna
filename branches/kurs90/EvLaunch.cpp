//---------------------------------------------------------------------------

/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak and others

*/

#include    "system.hpp"
#pragma hdrstop

#include "mtable.hpp"
#include "Timer.h"
#include "Globals.h"
#include "EvLaunch.h"
#include "Event.h"

#include "Usefull.h"
#include "MemCell.h"
#include "parser.h"
#include "Console.h"



//---------------------------------------------------------------------------

__fastcall TEventLauncher::TEventLauncher()
{//ustawienie pocz�tkowych warto�ci dla wszystkich zmiennych 
 iKey=0;
 DeltaTime=-1;
 UpdatedTime=0;
 fVal1=fVal2=0;
 szText=NULL;
 iHour=iMinute=-1; //takiego czasu nigdy nie b�dzie
 dRadius=0;
 Event1=Event2=NULL;
 MemCell=NULL;
 iCheckMask=0;
}

__fastcall TEventLauncher::~TEventLauncher()
{
 SafeDeleteArray(szText);
}

void __fastcall TEventLauncher::Init()
{
}

bool __fastcall TEventLauncher::Load(cParser *parser)
{//wczytanie wyzwalacza zdarze�
 AnsiString str;
 std::string token;
 parser->getTokens();
 *parser >> dRadius; //promie� dzia�ania
 if (dRadius>0.0)
  dRadius*=dRadius; //do kwadratu, pod warunkiem, �e nie jest ujemne
 parser->getTokens(); //klawisz steruj�cy
 *parser >> token;
 str=AnsiString(token.c_str());
 if (str!="none")
 {
  if (str.Length()==1)
   iKey=VkKeyScan(str[1]); //jeden znak jest konwertowany na kod klawisza
  else
   iKey=str.ToIntDef(0); //a jak wi�cej, to jakby numer klawisza jest
 }
 parser->getTokens();
 *parser >> DeltaTime;
 if (DeltaTime<0)
  DeltaTime=-DeltaTime; //dla ujemnego zmieniamy na dodatni
 else if (DeltaTime>0)
 {//warto�� dodatnia oznacza wyzwalanie o okre�lonej godzinie
  iMinute=int(DeltaTime)%100; //minuty s� najm�odszymi cyframi dziesietnymi
  iHour=int(DeltaTime-iMinute)/100; //godzina to setki
  DeltaTime=0; //bez powt�rze�
  WriteLog("EventLauncher at "+IntToStr(iHour)+":"+IntToStr(iMinute)); //wy�wietlenie czasu
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
 {//obs�uga wyzwalania warunkowego
  parser->getTokens();
  *parser >> token;
  asMemCellName=AnsiString(token.c_str());
  parser->getTokens();
  *parser >> token;
  SafeDeleteArray(szText);
  szText=new char[256];
  strcpy(szText,token.c_str());
  if (token.compare("*")!=0) //*=nie bra� command pod uwag�
   iCheckMask|=conditional_memstring;
  parser->getTokens();
  *parser >> token;
  if (token.compare("*")!=0) //*=nie bra� warto�ci 1. pod uwag�
  {
   iCheckMask|=conditional_memval1;
   str=AnsiString(token.c_str());
   fVal1=str.ToDouble();
  }
  else fVal1=0;
  parser->getTokens();
  *parser >> token;
  if (token.compare("*")!=0) //*=nie bra� warto�ci 2. pod uwag�
  {
   iCheckMask|=conditional_memval2;
   str=AnsiString(token.c_str());
   fVal2=str.ToDouble();
  }
  else fVal2=0;
  parser->getTokens(); //s�owo zamykaj�ce
  *parser >> token;
 }
 return true;
};

bool __fastcall TEventLauncher::Render()
{//"renderowanie" wyzwalacza
 bool bCond=false;
 if (iKey!=0)
 {
  if (Global::bActive) //tylko je�li okno jest aktywne
   bCond=(Console::Pressed(iKey)); //czy klawisz wci�ni�ty
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
  {//zgodno�� czasu uruchomienia
   if (UpdatedTime<10)
   {
    UpdatedTime=20; //czas do kolejnego wyzwolenia?
    bCond=true;
   }
  }
 }
 else
  UpdatedTime=1;
 if (bCond) //je�li spe�niony zosta� warunek
 {
  if ((iCheckMask!=0)&&MemCell) //sprawdzanie warunku na kom�rce pami�ci
   bCond=MemCell->Compare(szText,fVal1,fVal2,iCheckMask);
 }
 return bCond;  //sprawdzanie dRadius w Ground.cpp
}

bool __fastcall TEventLauncher::IsGlobal()
{//sprawdzenie, czy jest globalnym wyzwalaczem czasu
 if (DeltaTime==0)
  if (iHour>=0)
   if (iMinute>=0)
    if (dRadius<0.0) //bez ograniczenia zasi�gu
     return true;
 return false;
};
//---------------------------------------------------------------------------

#pragma package(smart_init)
