//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Console.h"
#include "Globals.h"
#include "Logs.h"
//#include "RaPoKeys.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
//Ra: klasa statyczna gromadz¹ca sygna³y steruj¹ce oraz informacje zwrotne
//Ra: stan wejœcia zmieniany klawiatur¹ albo dedykowanym urz¹dzeniem
//Ra: stan wyjœcia zmieniany przez symulacjê (mierniki, kontrolki)

/*******************************
Do klawisza klawiatury przypisana jest maska bitowa oraz numer wejœcia.
Naciœniêcie klawisza powoduje wywo³anie procedury ustawienia bitu o podanej
masce na podanym wejœciu. Zwonienie klawisza analogicznie wywo³uje zerowanie
bitu wg maski. Zasadniczo w masce ustawiony jest jeden bit, ale w razie
potrzeby mo¿e byæ ich wiêcej.

Oddzielne wejœcia s¹ wprowadzone po to, by mo¿na by³o u¿ywaæ wiêcej ni¿ 32
bity do sterowania. Podzia³ na wejœcia jest równie¿ ze wzglêdów organizacyjnych,
np. sterowanie œwiat³ami mo¿e mieæ oddzielny numer wejœcia ni¿ prze³¹czanie
radia, poniewa¿ nie ma potrzeby ich uzale¿niaæ (tzn. badaæ wspóln¹ maskê bitow¹).

Do ka¿dego wejœcia podpiêty jest skrypt binarny, charakterystyczny dla danej
konstrukcji pojazdu. Sprawdza on zale¿noœci (w tym uszkodzenia) za pomoc¹
operacji logicznych na maskach bitowych. Do ka¿dego wejœcia jest przypisana
jedna, oddzielna maska 32 bit, ale w razie potrzeby istnieje te¿ mo¿liwoœæ
korzystania z masek innych wejœæ. Skrypt mo¿e te¿ wysy³aæ maski na inne wejœcia,
ale nale¿y unikaæ rekurencji.

Definiowanie wejœæ oraz przeznaczenia ich masek jest w gestii konstruktora
skryptu. Ka¿dy pojazd mo¿e mieæ inny schemat wejœæ i masek, ale w miarê mo¿liwoœci
nale¿y d¹¿yæ do unifikacji. Skrypty mog¹ równie¿ u¿ywaæ dodatkowych masek bitowych.
Maski bitowe odpowiadaj¹ stanom prze³¹czników, czujników, styczników itd.

Dzia³anie jest nastêpuj¹ce:
- na klawiaturze konsoli naciskany jest przycisk
- naciœniêcie przycisku zamieniane jest na maskê bitow¹ oraz numer wejœcia
- wywo³ywany jest skrypt danego wejœcia z ow¹ mask¹
- skrypt sprawdza zale¿noœci i np. modyfikuje w³asnoœci fizyki albo inne maski
- ewentualnie do wyzwalacza czasowego dodana jest maska i numer wejœcia

/*******************************/

//int pomain();

//Ra: do poprawienia
typedef enum {ktCapsLock,ktNumLock,ktScrollLock} TKeyType;

void SetLedState(TKeyType KeyCode,bool bOn)
{//Ra: bajer do migania LED-ami w klawiaturze
 TKeyboardState KBState;
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

int Console::iBits=0; //zmienna statyczna - obiekt Console jest jednen wspólny
int Console::iMode=0;
int Console::iConfig=0;

__fastcall Console::Console()
{
};

__fastcall Console::~Console()
{
};

void __fastcall Console::ModeSet(int m,int h)
{//ustawienie trybu pracy
 iMode=m;
 iConfig=h;
};

int __fastcall Console::On()
{//za³¹czenie konsoli (np. nawi¹zanie komunikacji)
 //WriteLog(TPoKeys::Count());
 //pomain();
};

void __fastcall Console::Off()
{//wy³¹czenie informacji zwrotnych (reset pulpitu)
 BitsClear(-1);
};

void __fastcall Console::BitsSet(int mask,int entry)
{//ustawienie bitów o podanej masce (mask) na wejœciu (entry)
 if ((iBits&mask)!=mask) //je¿eli zmiana
 {iBits|=mask;
  BitsUpdate(mask);
 }
};

void __fastcall Console::BitsClear(int mask,int entry)
{//zerowanie bitów o podanej masce (mask) na wejœciu (entry)
 if (iBits&mask) //je¿eli zmiana
 {iBits&=~mask;
  BitsUpdate(mask);
 }
};

void __fastcall Console::BitsUpdate(int mask)
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

bool __fastcall Console::Pressed(int x)
{//na razie tak
 return Global::bActive&&(GetKeyState(x)<0);
};
