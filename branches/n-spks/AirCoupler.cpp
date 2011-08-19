//---------------------------------------------------------------------------

#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#include "AirCoupler.h"
#include "Timer.h"

__fastcall TAirCoupler::TAirCoupler()
{
    pModelOn= NULL;
    pModelOff= NULL;
    pModelxOn= NULL;      
    bOn= false;
    bxOn= false;    
}

__fastcall TAirCoupler::~TAirCoupler()
{
}

int __fastcall TAirCoupler::GetStatus()
{
int x=0;
if(pModelOn) x=1;
if(pModelxOn) x=2;
return x;
}

void __fastcall TAirCoupler::Clear()
{
    pModelOn= NULL;
    pModelOff= NULL;
    pModelxOn= NULL;    
    bOn= false;
    bxOn= false;    
}


void __fastcall TAirCoupler::Init(AnsiString asName, TModel3d *pModel)
{
    pModelOn= pModel->GetFromName(AnsiString(asName+"_on").c_str());
    pModelOff= pModel->GetFromName(AnsiString(asName+"_off").c_str());
    pModelxOn= pModel->GetFromName(AnsiString(asName+"_xon").c_str());
}

void __fastcall TAirCoupler::Load(TQueryParserComp *Parser, TModel3d *pModel)
{
    AnsiString str=Parser->GetNextSymbol().LowerCase();
    if (pModel)
     {
       Init(str,pModel);
     }
    else
     {
       pModelOn=NULL;
       pModelxOn=NULL;       
       pModelOff=NULL;
     }
}

void __fastcall TAirCoupler::Update()
{
//  if ((pModelOn!=NULL) && (pModelOn!=NULL))
   {
    if (pModelOn)
        pModelOn->Visible= bOn;
    if (pModelOff)
        pModelOff->Visible= !(bOn||bxOn);
    if (pModelxOn)
        pModelxOn->Visible= bxOn;
   }
}

//---------------------------------------------------------------------------

#pragma package(smart_init)
