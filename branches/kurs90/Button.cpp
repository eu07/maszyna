//---------------------------------------------------------------------------

#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#include "Button.h"
#include "Timer.h"

__fastcall TButton::TButton()
{
    pModelOn= NULL;
    pModelOff= NULL;
    bOn= false;
}

__fastcall TButton::~TButton()
{
}

__fastcall TButton::Clear()
{
    pModelOn= NULL;
    pModelOff= NULL;
    bOn= false;
}


bool __fastcall TButton::Init(AnsiString asName, TModel3d *pModel, bool bNewOn)
{
    pModelOn= pModel->GetFromName(AnsiString(asName+"_on").c_str());
    pModelOff= pModel->GetFromName(AnsiString(asName+"_off").c_str());
    bOn= bNewOn;
    Update();
}

bool __fastcall TButton::Load(TQueryParserComp *Parser, TModel3d *pModel)
{
    AnsiString str=Parser->GetNextSymbol().LowerCase();
    if (pModel)
     {
       Init(str,pModel,false);
     }
    else
     {
       pModelOn=NULL;
       pModelOff=NULL;
     }
}

bool __fastcall TButton::Update()
{
//  if ((pModelOn!=NULL) && (pModelOn!=NULL))
   {
    if (pModelOn)
        pModelOn->Visible= bOn;
    if (pModelOff)
        pModelOff->Visible= !bOn;
   }
}

//---------------------------------------------------------------------------

#pragma package(smart_init)
