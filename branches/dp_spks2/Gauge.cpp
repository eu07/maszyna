//---------------------------------------------------------------------------

/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak
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

#include "Timer.h"
#include "QueryParserComp.hpp"
#include "Model3d.h"
#include "Gauge.h"
#include "Console.h"

__fastcall TGauge::TGauge()
{
 fFriction=0;
 SubModel=NULL;
 eType=gt_Unknown;
 fStepSize=5;
 iChannel=-1; //kana³ analogowej komunikacji zwrotnej
}

__fastcall TGauge::~TGauge()
{
}

void __fastcall TGauge::Clear()
{
  SubModel= NULL;
  eType= gt_Unknown;
  fValue= 0;
  fDesiredValue= 0;
}


void __fastcall TGauge::Init(TSubModel *NewSubModel,TGaugeType eNewType,double fNewScale,double fNewOffset,double fNewFriction,double fNewValue)
{//ustawienie parametrów animacji submodelu
 if (NewSubModel)
 {
  fFriction=fNewFriction;
  fValue=fNewValue;
  fOffset=fNewOffset;
  fScale=fNewScale;
  SubModel=NewSubModel;
  eType=eNewType;
  NewSubModel->WillBeAnimated(); //wy³¹czenie ignowania jedynkowego transformu 
 }
}

void __fastcall TGauge::Load(TQueryParserComp *Parser,TModel3d *md1,TModel3d *md2)
{
 AnsiString str1=Parser->GetNextSymbol();
 AnsiString str2=Parser->GetNextSymbol().LowerCase();
 double val3=Parser->GetNextSymbol().ToDouble();
 double val4=Parser->GetNextSymbol().ToDouble();
 double val5=Parser->GetNextSymbol().ToDouble();
 TSubModel *sm=md1->GetFromName(str1.c_str());
 if (!sm) //jeœli nie znaleziony
  if (md2) //a jest podany drugi model (np. zewnêtrzny)
   md2->GetFromName(str1.c_str()); //to mo¿e tam bêdzie, co za ró¿nica gdzie
 if (str2=="mov")
  Init(sm,gt_Move,val3,val4,val5);
 else if (str2=="wip")
  Init(sm,gt_Wiper,val3,val4,val5);
 else
  Init(sm,gt_Rotate,val3,val4,val5);
}


void __fastcall TGauge::PermIncValue(double fNewDesired)
{
//    double p;
//    p= fNewDesired;
    fDesiredValue= fDesiredValue+fNewDesired*fScale+fOffset;
    if (fDesiredValue-fOffset>360/fScale)
    {
       fDesiredValue=fDesiredValue-(360/fScale);
       fValue=fValue-(360/fScale);
    }
}

void __fastcall TGauge::IncValue(double fNewDesired)
{
//    double p;
//    p= fNewDesired;
    fDesiredValue= fDesiredValue+fNewDesired*fScale+fOffset;
    if (fDesiredValue>fScale+fOffset)
       fDesiredValue=fScale+fOffset;
}

void __fastcall TGauge::DecValue(double fNewDesired)
{
//    double p;
//    p= fNewDesired;
    fDesiredValue= fDesiredValue-fNewDesired*fScale+fOffset;
    if(fDesiredValue<0) fDesiredValue=0;
}

void __fastcall TGauge::UpdateValue(double fNewDesired)
{
//    double p;
//    p= fNewDesired;
 fDesiredValue=fNewDesired*fScale+fOffset;
 if (iChannel>=0)
  Console::ValueSet(iChannel,fNewDesired);
}

void __fastcall TGauge::PutValue(double fNewDesired) //McZapkie-281102: natychmiastowe wpisanie wartosci
{
    fDesiredValue= fNewDesired*fScale+fOffset;
    fValue=fDesiredValue;
}


void __fastcall TGauge::Update()
{
    float dt=Timer::GetDeltaTime();
    if (fFriction>0 && dt<fFriction/2)     //McZapkie-281102: zabezpieczenie przed oscylacjami dla dlugich czasow
        fValue+= dt*(fDesiredValue-fValue)/fFriction;
    else
        fValue= fDesiredValue;
    if (SubModel)
    {
     switch (eType)
     {
      case gt_Rotate:
       SubModel->SetRotate(float3(0,1,0),fValue*360.0);
      break;
      case gt_Move:
       SubModel->SetTranslate(float3(0,0,fValue));
      break;
      case gt_Wiper:
       SubModel->SetTranslate(float3(0,0,fValue));
       SubModel=SubModel->ChildGet();
       if (SubModel)
       {SubModel->SetTranslate(float3(0,0,fValue));
        SubModel=SubModel->ChildGet();
        if (SubModel)
         SubModel->SetTranslate(float3(0,0,fValue));
       }
      break;
     }
    }

}

void __fastcall TGauge::Render()
{
}

//---------------------------------------------------------------------------

#pragma package(smart_init)
