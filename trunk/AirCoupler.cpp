//---------------------------------------------------------------------------

#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#include "AirCoupler.h"
#include "Timer.h"

__fastcall TAirCoupler::TAirCoupler()
{
 Clear();
}

__fastcall TAirCoupler::~TAirCoupler()
{
}

int __fastcall TAirCoupler::GetStatus()
{//zwraca 1, jeœli istnieje model prosty, 2 gdy skoœny
 int x=0;
 if (pModelOn) x=1;
 if (pModelxOn) x=2;
 return x;
}

void __fastcall TAirCoupler::Clear()
{//zerowanie wskaŸników
 pModelOn=NULL;
 pModelOff=NULL;
 pModelxOn=NULL;
 bOn=false;
 bxOn=false;
}


void __fastcall TAirCoupler::Init(AnsiString asName, TModel3d *pModel)
{//wyszukanie submodeli
 if (!pModel) return; //nie ma w czym szukaæ
 pModelOn=pModel->GetFromName(AnsiString(asName+"_on").c_str()); //po³¹czony na wprost
 pModelOff=pModel->GetFromName(AnsiString(asName+"_off").c_str()); //odwieszony
 pModelxOn=pModel->GetFromName(AnsiString(asName+"_xon").c_str()); //po³¹czony na skos
}

void __fastcall TAirCoupler::Load(TQueryParserComp *Parser, TModel3d *pModel)
{
 AnsiString str=Parser->GetNextSymbol().LowerCase();
 if (pModel)
  Init(str,pModel);
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
        pModelOn->iVisible=bOn;
    if (pModelOff)
        pModelOff->iVisible=!(bOn||bxOn);
    if (pModelxOn)
        pModelxOn->iVisible=bxOn;
   }
}

//---------------------------------------------------------------------------

#pragma package(smart_init)
