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
 if ( token.compare( "recuperation" ) == 0 )
  Recuperation= true;
 parser->getTokens();
 *parser >> token;
 if ( token.compare( "end" ) != 0 )
  Error("tractionpowersource end statement missing");
 InternalRes*=10; //coœ ma³a ta rezystancja by³a...  
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


//---------------------------------------------------------------------------

#pragma package(smart_init)
