//---------------------------------------------------------------------------

/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak

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
 eType=gt_Unknown;
 fFriction=0.0;
 fDesiredValue=0.0;
 fValue=0.0;
 fOffset=0.0;
 fScale=1.0;
 fStepSize=5;
 iChannel=-1; //kana³ analogowej komunikacji zwrotnej
 SubModel=NULL;
};

__fastcall TGauge::~TGauge()
{
};

void __fastcall TGauge::Clear()
{
 SubModel=NULL;
 eType=gt_Unknown;
 fValue=0;
 fDesiredValue=0;
};

void __fastcall TGauge::Init(TSubModel *NewSubModel,TGaugeType eNewType,double fNewScale,double fNewOffset,double fNewFriction,double fNewValue)
{//ustawienie parametrów animacji submodelu
 if (NewSubModel)
 {//warunek na wszelki wypadek, gdyby siê submodel nie pod³¹czy³
  fFriction=fNewFriction;
  fValue=fNewValue;
  fOffset=fNewOffset;
  fScale=fNewScale;
  SubModel=NewSubModel;
  eType=eNewType;
  if (eType==gt_Digital)
  {
   TSubModel *sm=SubModel->ChildGet();
   do
   {//pêtla po submodelach potomnych i obracanie ich o k¹t zale¿y od cyfry w (fValue)
    if (sm->pName)
    {//musi mieæ niepust¹ nazwê
     if ((*sm->pName)>='0')
      if ((*sm->pName)<='9')
       sm->WillBeAnimated(); //wy³¹czenie optymalizacji
    }
    sm=sm->NextGet();
   } while (sm);
  }
  else //a banan mo¿e byæ z optymalizacj¹?
   NewSubModel->WillBeAnimated(); //wy³¹czenie ignowania jedynkowego transformu
 }
};

void __fastcall TGauge::Load(TQueryParserComp *Parser,TModel3d *md1,TModel3d *md2,double mul)
{
 AnsiString str1=Parser->GetNextSymbol();
 AnsiString str2=Parser->GetNextSymbol().LowerCase();
 double val3=Parser->GetNextSymbol().ToDouble()*mul;
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
 else if (str2=="dgt")
  Init(sm,gt_Digital,val3,val4,val5);
 else
  Init(sm,gt_Rotate,val3,val4,val5);
};


void __fastcall TGauge::PermIncValue(double fNewDesired)
{
 fDesiredValue=fDesiredValue+fNewDesired*fScale+fOffset;
 if (fDesiredValue-fOffset>360/fScale)
 {
  fDesiredValue=fDesiredValue-(360/fScale);
  fValue=fValue-(360/fScale);
 }
};

void __fastcall TGauge::IncValue(double fNewDesired)
{//u¿ywane tylko dla uniwersali
 fDesiredValue=fDesiredValue+fNewDesired*fScale+fOffset;
 if (fDesiredValue>fScale+fOffset)
  fDesiredValue=fScale+fOffset;
};

void __fastcall TGauge::DecValue(double fNewDesired)
{//u¿ywane tylko dla uniwersali
 fDesiredValue=fDesiredValue-fNewDesired*fScale+fOffset;
 if (fDesiredValue<0) fDesiredValue=0;
};

void __fastcall TGauge::UpdateValue(double fNewDesired)
{//ustawienie wartoœci docelowej
 fDesiredValue=fNewDesired*fScale+fOffset;
 if (iChannel>=0)
  Console::ValueSet(iChannel,fNewDesired);
};

void __fastcall TGauge::PutValue(double fNewDesired)
{//McZapkie-281102: natychmiastowe wpisanie wartosci
 fDesiredValue=fNewDesired*fScale+fOffset;
 fValue=fDesiredValue;
 //if (iChannel>=0) //chyba nie u¿ywane dla mierników
 // Console::ValueSet(iChannel,fNewDesired);
};

void __fastcall TGauge::Update()
{
 float dt=Timer::GetDeltaTime();
 if ((fFriction>0)&&(dt<0.5*fFriction)) //McZapkie-281102: zabezpieczenie przed oscylacjami dla dlugich czasow
  fValue+=dt*(fDesiredValue-fValue)/fFriction;
 else
  fValue=fDesiredValue;
 if (SubModel)
 {//warunek na wszelki wypadek, gdyby siê submodel nie pod³¹czy³
  TSubModel *sm;
  switch (eType)
  {
   case gt_Rotate:
    SubModel->SetRotate(float3(0,1,0),fValue*360.0);
   break;
   case gt_Move:
    SubModel->SetTranslate(float3(0,0,fValue));
   break;
   case gt_Wiper:
    SubModel->SetRotate(float3(0,1,0),fValue*360.0);
    sm=SubModel->ChildGet();
    if (sm)
    {sm->SetRotate(float3(0,1,0),fValue*360.0);
     sm=sm->ChildGet();
     if (sm)
      sm->SetRotate(float3(0,1,0),fValue*360.0);
    }
   break;
   case gt_Digital: //Ra 2014-07: licznik cyfrowy
    sm=SubModel->ChildGet();
    AnsiString n=FormatFloat("0000000000",floor(fValue)); //na razie tak trochê bez sensu
    do
    {//pêtla po submodelach potomnych i obracanie ich o k¹t zale¿y od cyfry w (fValue)
     if (sm->pName)
     {//musi mieæ niepust¹ nazwê
      if ((*sm->pName)>='0')
       if ((*sm->pName)<='9')
        sm->SetRotate(float3(0,1,0),-36.0*(n['0'+10-(*sm->pName)]-'0'));
     }
     sm=sm->NextGet();
    } while (sm);
   break;
  }
 }
};

void __fastcall TGauge::Render()
{
};

void __fastcall TGauge::AssignFloat(float* fValue)
{
 cDataType='f';
 fData=fValue;
};
void __fastcall TGauge::AssignDouble(double* dValue)
{
 cDataType='d';
 dData=dValue;
};
void __fastcall TGauge::AssignInt(int* iValue)
{
 cDataType='i';
 iData=iValue;
};
void __fastcall TGauge::UpdateValue()
{//ustawienie wartoœci docelowej z parametru
 switch (cDataType)
 {//to nie jest zbyt optymalne, mo¿na by zrobiæ osobne funkcje
  case 'f':
   fDesiredValue=(*fData)*fScale+fOffset;
   if (iChannel>=0)
    Console::ValueSet(iChannel,(*fData));
  break;
  case 'd':
   fDesiredValue=(*dData)*fScale+fOffset;
   if (iChannel>=0)
    Console::ValueSet(iChannel,(*dData));
  break;
  case 'i':
   fDesiredValue=(*iData)*fScale+fOffset;
   if (iChannel>=0)
    Console::ValueSet(iChannel,(*iData));
  break;
 }
};



//---------------------------------------------------------------------------

#pragma package(smart_init)
