//---------------------------------------------------------------------------

#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#include "Button.h"
#include "Timer.h"
#include <winuser.h>

//---------------------------------------------------------------------------

#pragma package(smart_init)

//Ra: do poprawienia
typedef enum {ktCapsLock,ktNumLock,ktScrollLock} TKeyType;

//Ra: tymczasowo tu bajer do migania LED-ami w klawiaturze
void SetLedState(TKeyType KeyCode,bool bOn)
{TKeyboardState KBState;
 char Code;
 switch (KeyCode)
 {case ktScrollLock: Code=VK_SCROLL; break;
  case ktCapsLock: Code=VK_CAPITAL; break;
  case ktNumLock: Code=VK_NUMLOCK; break;
 }
 GetKeyboardState(KBState);
 if (Win32Platform==VER_PLATFORM_WIN32_NT)
 {
  if (bool(KBState[Code])!=bOn)
  {
   keybd_event(Code,MapVirtualKey(Code,0),KEYEVENTF_EXTENDEDKEY,0);
   keybd_event(Code,MapVirtualKey(Code,0),KEYEVENTF_EXTENDEDKEY|KEYEVENTF_KEYUP,0);
  }
 }
 else
 {
  KBState[Code]=bOn?1:0;
  SetKeyboardState(KBState);
 };
};


__fastcall TButton::TButton()
{
 pModelOn=NULL;
 pModelOff=NULL;
 bOn=false;
 iFeedbackBit=0; //bez zwrotnego
}

__fastcall TButton::~TButton()
{
}

void __fastcall TButton::Clear()
{
 pModelOn=NULL;
 pModelOff=NULL;
 bOn=false;
 iFeedback&=~iFeedbackBit; //gaszenie
}


void __fastcall TButton::Init(AnsiString asName, TModel3d *pModel, bool bNewOn)
{
 pModelOn=pModel->GetFromName(AnsiString(asName+"_on").c_str());
 pModelOff=pModel->GetFromName(AnsiString(asName+"_off").c_str());
 bOn=bNewOn;
 Update();
}

void __fastcall TButton::Load(TQueryParserComp *Parser, TModel3d *pModel)
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

void __fastcall TButton::Update()
{
//  if ((pModelOn!=NULL) && (pModelOn!=NULL))
 {
  if (pModelOn) pModelOn->Visible=bOn;
  if (pModelOff) pModelOff->Visible=!bOn;
 }
 if (iFeedbackBit)
 {if (bOn) //zapalenie
  {if ((iFeedback&iFeedbackBit)==0)
   {iFeedback|=iFeedbackBit; //zapalanie
    if (iFeedbackBit&3) //gdy SHP albo CA
     SetLedState(ktCapsLock,iFeedback&3);
   }
  }
  else //zgaszenie
   if ((iFeedback&iFeedbackBit)!=0)
   {iFeedback&=~iFeedbackBit; //gaszenie
    if (iFeedbackBit&3) //gdy SHP albo CA
     SetLedState(ktCapsLock,iFeedback&3);
   }
 }
}

