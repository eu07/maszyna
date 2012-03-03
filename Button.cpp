//---------------------------------------------------------------------------

#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#include "Button.h"
#include "Console.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

__fastcall TButton::TButton()
{
 iFeedbackBit=0;
 Clear();
};

__fastcall TButton::~TButton()
{
};

void __fastcall TButton::Clear()
{
 pModelOn=NULL;
 pModelOff=NULL;
 bOn=false;
 Update(); //kasowanie bitu Feedback, o ile jakiœ ustawiony
};

void __fastcall TButton::Init(AnsiString asName, TModel3d *pModel, bool bNewOn)
{
 pModelOn=pModel->GetFromName(AnsiString(asName+"_on").c_str());
 pModelOff=pModel->GetFromName(AnsiString(asName+"_off").c_str());
 bOn=bNewOn;
 Update();
};

void __fastcall TButton::Load(TQueryParserComp *Parser, TModel3d *pModel)
{
 AnsiString str=Parser->GetNextSymbol().LowerCase();
 if (pModel)
  Init(str,pModel,false);
 else
 {
  pModelOn=NULL;
  pModelOff=NULL;
 }
};

void __fastcall TButton::Update()
{
 if (pModelOn) pModelOn->Visible=bOn;
 if (pModelOff) pModelOff->Visible=!bOn;
 if (iFeedbackBit) //je¿eli generuje informacjê zwrotn¹
 {if (bOn) //zapalenie
   Console::BitsSet(iFeedbackBit);
  else
   Console::BitsClear(iFeedbackBit);
 }
};

