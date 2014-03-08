//---------------------------------------------------------------------------

/*
    MaSzyna EU07 locomotive simulator component
    Copyright (C) 2004  Maciej Czapkiewicz and others

*/


#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#include "Mover.h"
#include "mctools.hpp"
#include "Timer.h"
#include "Globals.h"
#include "TractionPower.h"

#include "Usefull.h"

//---------------------------------------------------------------------------

__fastcall TTractionPowerSource::TTractionPowerSource()
{
 NominalVoltage=0;
 VoltageFrequency=0;
 InternalRes=1;
 MaxOutputCurrent=0;
 FastFuseTimeOut=1;
 FastFuseRepetition=3;
 SlowFuseTimeOut=60;
 Recuperation=false;

 TotalCurrent=0;
 TotalPreviousCurrent=0;
 OutputVoltage=0;
 FastFuse=false;
 SlowFuse=false;
 FuseTimer=0;
 FuseCounter=0;
 //asName="";
 psNode[0]=NULL; //sekcje zostan¹ pod³¹czone do zasilaczy
 psNode[1]=NULL;
 bSection=false; //sekcja nie jest Ÿród³em zasilania, tylko grupuje przês³a
};

__fastcall TTractionPowerSource::~TTractionPowerSource()
{
};

void __fastcall TTractionPowerSource::Init()
{
};

bool __fastcall TTractionPowerSource::Load(cParser *parser)
{
 std::string token;
 //AnsiString str;
 //str= Parser->GetNextSymbol()LowerCase();
 //asName= str;
 parser->getTokens(5);
 *parser >> NominalVoltage >> VoltageFrequency >> InternalRes >> MaxOutputCurrent >> FastFuseTimeOut;
 parser->getTokens();
 *parser >> FastFuseRepetition;
 parser->getTokens();
 *parser >> SlowFuseTimeOut;
 parser->getTokens();
 *parser >> token;
 if (token.compare("recuperation")==0)
  Recuperation=true;
 else if (token.compare("section")==0) //od³¹cznik sekcyjny
  bSection=true; //nie jest Ÿród³em zasilania, a jedynie informuje o pr¹dzie od³¹czenia sekcji z obwodu
 parser->getTokens();
 *parser >> token;
 if (token.compare("end")!=0)
  Error("tractionpowersource end statement missing");
 //if (!bSection) //od³¹cznik sekcji zasadniczo nie ma impedancji (0.01 jest OK)
  if (InternalRes<0.1) //coœ ma³a ta rezystancja by³a...
   InternalRes=0.2; //tak oko³o 0.2, wg http://www.ikolej.pl/fileadmin/user_upload/Seminaria_IK/13_05_07_Prezentacja_Kruczek.pdf
 return true;
};


bool __fastcall TTractionPowerSource::Render()
{
 return true;
};

bool __fastcall TTractionPowerSource::Update(double dt)
{//powinno byæ wykonane raz na krok fizyki
 if (TotalPreviousCurrent>MaxOutputCurrent)
 {
  FastFuse=true;
  FuseCounter+=1;
  if (FuseCounter>FastFuseRepetition)
   SlowFuse=true;
  FuseTimer=0;
 }
 if (FastFuse||SlowFuse)
 {//jeœli któryœ z bezpieczników zadzia³a³
  TotalCurrent=0;
  FuseTimer+=dt;
  if (!SlowFuse)
  {//gdy szybki, odczekaæ krótko i za³¹czyæ
   if (FuseTimer>FastFuseTimeOut)
    FastFuse=false;
  }
  else
   if (FuseTimer>SlowFuseTimeOut)
   {SlowFuse=false;
    FuseCounter=0; //dajemy znów szanse
   }
 }
 TotalPreviousCurrent=TotalCurrent;
 TotalCurrent=0;
 return true;
};

double __fastcall TTractionPowerSource::GetVoltage(double Current)
{
 if (SlowFuse || FastFuse)
 {
  if (Current>0.1)
   FuseTimer=0;
  return 0;
 }
 if ((Current>0) || ((Current<0) && (Recuperation)))
   TotalCurrent+=Current;
 OutputVoltage=NominalVoltage-InternalRes*TotalPreviousCurrent;
 return OutputVoltage;
};

void __fastcall TTractionPowerSource::PowerSet(TTractionPowerSource *ps)
{//wskazanie zasilacza w obiekcie sekcji
 if (!psNode[0])
  psNode[0]=ps;
 else if (!psNode[1])
  psNode[1]=ps;
 //else ErrorLog("nie mo¿e byæ wiêcej punktów zasilania ni¿ dwa"); 
};

//---------------------------------------------------------------------------

#pragma package(smart_init)
