//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Console.h"
#include "Globals.h"
#include "Logs.h"
#include "PoKeys55.h"
#include "LPT.h"

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

/* //kod do przetrawienia:
//aby siê nie w³¹czacz wygaszacz ekranu, co jakiœ czas naciska siê wirtualnie ScrollLock

[DllImport("user32.dll")]
static extern void keybd_event(byte bVk, byte bScan, uint dwFlags, UIntPtr dwExtraInfo);

private static void PressScrollLock()
{//przyciska i zwalnia ScrollLock
 const byte vkScroll = 0x91;
 const byte keyeventfKeyup = 0x2;
 keybd_event(vkScroll, 0x45, 0, (UIntPtr)0);
 keybd_event(vkScroll, 0x45, keyeventfKeyup, (UIntPtr)0);
};

[DllImport("user32.dll")]
private static extern bool SystemParametersInfo(int uAction,int uParam,int &lpvParam,int flags);

public static Int32 GetScreenSaverTimeout()
{
 Int32 value=0;
 SystemParametersInfo(14,0,&value,0);
 return value;
};
*/

//Ra: do poprawienia
void SetLedState(char Code,bool bOn)
{//Ra: bajer do migania LED-ami w klawiaturze
 if (Win32Platform==VER_PLATFORM_WIN32_NT)
 {
  //WriteLog(AnsiString(int(GetAsyncKeyState(Code))));
  if (bool(GetAsyncKeyState(Code))!=bOn)
  {
   keybd_event(Code,MapVirtualKey(Code,0),KEYEVENTF_EXTENDEDKEY,0);
   keybd_event(Code,MapVirtualKey(Code,0),KEYEVENTF_EXTENDEDKEY|KEYEVENTF_KEYUP,0);
  }
 }
 else
 {
  TKeyboardState KBState;
  GetKeyboardState(KBState);
  KBState[Code]=bOn?1:0;
  SetKeyboardState(KBState);
 };
};

//---------------------------------------------------------------------------

int Console::iBits=0; //zmienna statyczna - obiekt Console jest jednen wspólny
int Console::iMode=0;
int Console::iConfig=0;
TPoKeys55 *Console::PoKeys55=NULL;
TLPT *Console::LPT=NULL;

__fastcall Console::Console()
{
 PoKeys55=NULL;
};

__fastcall Console::~Console()
{
 delete PoKeys55;
};

void __fastcall Console::ModeSet(int m,int h)
{//ustawienie trybu pracy
 iMode=m;
 iConfig=h;
};

int __fastcall Console::On()
{//za³¹czenie konsoli (np. nawi¹zanie komunikacji)
 switch (iMode)
 {case 1: //kontrolki klawiatury
  case 2: //kontrolki klawiatury
   iConfig=0; //licznik u¿ycia Scroll Lock
  break;
  case 3: //LPT
   LPT=new TLPT(); //otwarcie inpout32.dll
   if (LPT?LPT->Connect(iConfig):false)
   {//wys³aæ 0?
    BitsUpdate(-1); //aktualizacjia stanów, bo przy wczytywaniu mog³o byæ nieaktywne
    WriteLog("InpOut32.dll OK");
   }
   else
   {//po³¹czenie nie wysz³o, ma byæ NULL
    delete LPT;
    LPT=NULL;
   }
  break;
  case 4: //PoKeys
   PoKeys55=new TPoKeys55();
   if (PoKeys55?PoKeys55->Connect():false)
   {WriteLog("Found "+PoKeys55->Version());
    BitsUpdate(-1); //aktualizacjia stanów, bo przy wczytywaniu mog³o byæ nieaktywne
   }
   else
   {//po³¹czenie nie wysz³o, ma byæ NULL
    delete PoKeys55;
    PoKeys55=NULL;
   }
  break;
 }
 return 0;
};

void __fastcall Console::Off()
{//wy³¹czenie informacji zwrotnych (reset pulpitu)
 BitsClear(-1);
 if ((iMode==1)||(iMode==2))
  if (iConfig&1) //licznik u¿ycia Scroll Lock
  {//bez sensu to jest, ale mi siê samo w³¹cza
   SetLedState(VK_SCROLL,true); //przyciœniêty
   SetLedState(VK_SCROLL,false); //zwolniony
  }
 delete PoKeys55; PoKeys55=NULL;
 delete LPT; LPT=NULL;
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
{//aktualizacja stanu interfejsu informacji zwrotnej; (mask) - zakres zmienianych bitów
 switch (iMode)
 {case 1: //sterowanie œwiate³kami klawiatury: CA/SHP+opory
   if (mask&3) //gdy SHP albo CA
    SetLedState(VK_CAPITAL,iBits&3);
   if (mask&4) //gdy jazda na oporach
   {//Scroll Lock ma jakoœ dziwnie... zmiana stanu na przeciwny
    SetLedState(VK_SCROLL,true); //przyciœniêty
    SetLedState(VK_SCROLL,false); //zwolniony
    ++iConfig; //licznik u¿ycia Scroll Lock
   }
   break;
  case 2: //sterowanie œwiate³kami klawiatury: CA+SHP
   if (mask&2) //gdy CA
    SetLedState(VK_CAPITAL,iBits&2);
   if (mask&1) //gdy SHP
   {//Scroll Lock ma jakoœ dziwnie... zmiana stanu na przeciwny
    SetLedState(VK_SCROLL,true); //przyciœniêty
    SetLedState(VK_SCROLL,false); //zwolniony
    ++iConfig; //licznik u¿ycia Scroll Lock
   }
   break;
  case 3: //LPT Marcela z modyfikacj¹ (jazda na oporach zamiast brzêczyka)
   if (LPT)
    LPT->Out(iBits);
   break;
  case 4: //PoKeys55 wg Marcela - wersja druga z koñca 2012
   if (PoKeys55)
   {//pewnie trzeba bêdzie to dodatkowo buforowaæ i oczekiwaæ na potwierdzenie
    if (mask&0x0001) //b0 gdy SHP
     PoKeys55->Write(0x40,23-1,iBits&0x0001?1:0);
    if (mask&0x0002) //b1 gdy zmieniony CA
     PoKeys55->Write(0x40,24-1,iBits&0x0002?1:0);
    if (mask&0x0004) //b2 gdy jazda na oporach
     PoKeys55->Write(0x40,32-1,iBits&0x0004?1:0);
    if (mask&0x0008) //b3 Lampka WS (wy³¹cznika szybkiego)
     PoKeys55->Write(0x40,25-1,iBits&0x0008?1:0);
    if (mask&0x0010) //b4 Lampka przekaŸnika nadmiarowego silników trakcyjnych
     PoKeys55->Write(0x40,27-1,iBits&0x0010?1:0);
    if (mask&0x0020) //b5 Lampka styczników liniowych
     PoKeys55->Write(0x40,29-1,iBits&0x0020?1:0);
    if (mask&0x0040) //b6 Lampka poœlizgu
     PoKeys55->Write(0x40,30-1,iBits&0x0040?1:0);
    if (mask&0x0080) //b7 Lampka "przetwornicy"
     PoKeys55->Write(0x40,28-1,iBits&0x0080?1:0);
    if (mask&0x0100) //b8 Kontrolka przekaŸnika nadmiarowego sprê¿arki
     PoKeys55->Write(0x40,33-1,iBits&0x0100?1:0);
    if (mask&0x0200) //b9 Kontrolka sygnalizacji wentylatorów i oporów
     PoKeys55->Write(0x40,26-1,iBits&0x0200?1:0);
    if (mask&0x0400) //b10 Kontrolka wysokiego rozruchu
     PoKeys55->Write(0x40,31-1,iBits&0x0400?1:0);
    if (mask&0x0800) //b11 Kontrolka ogrzewania poci¹gu
     PoKeys55->Write(0x40,34-1,iBits&0x0800?1:0);
    if (mask&0x1000) //b12 Ciœnienie w cylindrach do odbijania w haslerze
     PoKeys55->Write(0x40,52-1,iBits&0x1000?1:0);
    if (mask&0x2000) //b13 Pr¹d na silnikach do odbijania w haslerze
     PoKeys55->Write(0x40,53-1,iBits&0x2000?1:0);
   }
   break;
 }
};

bool __fastcall Console::Pressed(int x)
{//na razie tak - czyta siê tylko klawiatura
 return Global::bActive&&(GetKeyState(x)<0);
};

void __fastcall Console::ValueSet(int x,double y)
{//ustawienie wartoœci (y) na kanale analogowym (x)
 if (iMode==4)
  if (PoKeys55)
  {
   PoKeys55->PWM(x,(((Global::fCalibrateOut[x][3]*y)+Global::fCalibrateOut[x][2])*y+Global::fCalibrateOut[x][1])*y+Global::fCalibrateOut[x][0]); //zakres <0;1>
  }
};

void __fastcall Console::Update()
{//funkcja powinna byæ wywo³ywana regularnie, np. raz w ka¿dej ramce ekranowej
 if (iMode==4)
  if (PoKeys55)
   if (PoKeys55->Update())
   {//wykrycie przestawionych prze³¹czników

   }
};

float __fastcall Console::AnalogGet(int x)
{//pobranie wartoœci analogowej
 if (iMode==4)
  if (PoKeys55)
   return PoKeys55->fAnalog[x];
 return -1.0;
};

unsigned char __fastcall Console::DigitalGet(int x)
{//pobranie wartoœci cyfrowej
 if (iMode==4)
  if (PoKeys55)
   return PoKeys55->iInputs[x];
 return 0;
};
