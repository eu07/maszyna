//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Feedback.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)
//Ra: do poprawienia
typedef enum {ktCapsLock,ktNumLock,ktScrollLock} TKeyType;

//Ra: bajer do migania LED-ami w klawiaturze
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

//---------------------------------------------------------------------------

int Feedback::iBits=0; //zmienna statyczna - Feedback jest jednen wspólny
int Feedback::iMode=0;
int Feedback::iConfig=0;

__fastcall Feedback::Feedback()
{
};

__fastcall Feedback::~Feedback()
{
};

void __fastcall Feedback::ModeSet(int m,int h)
{//zmiana trybu pracy
 iMode=m;
 iConfig=h;
};

void __fastcall Feedback::BitsSet(int mask)
{//ustawienie bitów o podanej masce
 if ((iBits&mask)!=mask) //je¿eli zmiana
 {iBits|=mask;
  BitsUpdate(mask);
 }
};

void __fastcall Feedback::BitsClear(int mask)
{//zerowanie bitów o podanej masce
 if (iBits&mask) //je¿eli zmiana
 {iBits&=~mask;
  BitsUpdate(mask);
 }
};

void __fastcall Feedback::BitsUpdate(int mask)
{//aktualizacja stanu interfejsu informacji zwrotnej
 switch (iMode)
 {case 1: //sterowanie œwiate³kami klawiatury: CA/SHP+opory
   if (mask&3) //gdy SHP albo CA
    SetLedState(ktCapsLock,iBits&3);
   if (mask==256) //gdy jazda na oporach
   {//Scroll Lock ma jakoœ dziwnie... zmiana stanu na przeciwny
    SetLedState(ktScrollLock,true); //przyciœniêty
    SetLedState(ktScrollLock,false); //zwolniony
   }
   break;
  case 2: //sterowanie œwiate³kami klawiatury: CA+SHP
   if (mask&2) //gdy CA
    SetLedState(ktCapsLock,iBits&2);
   if (mask&1) //gdy SHP
   {//Scroll Lock ma jakoœ dziwnie... zmiana stanu na przeciwny
    SetLedState(ktScrollLock,true); //przyciœniêty
    SetLedState(ktScrollLock,false); //zwolniony
   }
   break;
  case 3: //LPT Marcela
   // (do zrobienia)
   break;
 }
};

