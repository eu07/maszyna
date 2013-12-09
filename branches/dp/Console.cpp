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
//Ra: klasa statyczna gromadz�ca sygna�y steruj�ce oraz informacje zwrotne
//Ra: stan wej�cia zmieniany klawiatur� albo dedykowanym urz�dzeniem
//Ra: stan wyj�cia zmieniany przez symulacj� (mierniki, kontrolki)

/*******************************
Do klawisza klawiatury przypisana jest maska bitowa oraz numer wej�cia.
Naci�ni�cie klawisza powoduje wywo�anie procedury ustawienia bitu o podanej
masce na podanym wej�ciu. Zwonienie klawisza analogicznie wywo�uje zerowanie
bitu wg maski. Zasadniczo w masce ustawiony jest jeden bit, ale w razie
potrzeby mo�e by� ich wi�cej.

Oddzielne wej�cia s� wprowadzone po to, by mo�na by�o u�ywa� wi�cej ni� 32
bity do sterowania. Podzia� na wej�cia jest r�wnie� ze wzgl�d�w organizacyjnych,
np. sterowanie �wiat�ami mo�e mie� oddzielny numer wej�cia ni� prze��czanie
radia, poniewa� nie ma potrzeby ich uzale�nia� (tzn. bada� wsp�ln� mask� bitow�).

Do ka�dego wej�cia podpi�ty jest skrypt binarny, charakterystyczny dla danej
konstrukcji pojazdu. Sprawdza on zale�no�ci (w tym uszkodzenia) za pomoc�
operacji logicznych na maskach bitowych. Do ka�dego wej�cia jest przypisana
jedna, oddzielna maska 32 bit, ale w razie potrzeby istnieje te� mo�liwo��
korzystania z masek innych wej��. Skrypt mo�e te� wysy�a� maski na inne wej�cia,
ale nale�y unika� rekurencji.

Definiowanie wej�� oraz przeznaczenia ich masek jest w gestii konstruktora
skryptu. Ka�dy pojazd mo�e mie� inny schemat wej�� i masek, ale w miar� mo�liwo�ci
nale�y d��y� do unifikacji. Skrypty mog� r�wnie� u�ywa� dodatkowych masek bitowych.
Maski bitowe odpowiadaj� stanom prze��cznik�w, czujnik�w, stycznik�w itd.

Dzia�anie jest nast�puj�ce:
- na klawiaturze konsoli naciskany jest przycisk
- naci�ni�cie przycisku zamieniane jest na mask� bitow� oraz numer wej�cia
- wywo�ywany jest skrypt danego wej�cia z ow� mask�
- skrypt sprawdza zale�no�ci i np. modyfikuje w�asno�ci fizyki albo inne maski
- ewentualnie do wyzwalacza czasowego dodana jest maska i numer wej�cia

/*******************************/

/* //kod do przetrawienia:
//aby si� nie w��czacz wygaszacz ekranu, co jaki� czas naciska si� wirtualnie ScrollLock

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

int Console::iBits=0; //zmienna statyczna - obiekt Console jest jednen wsp�lny
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
{//za��czenie konsoli (np. nawi�zanie komunikacji)
 switch (iMode)
 {case 1: //kontrolki klawiatury
  case 2: //kontrolki klawiatury
   iConfig=0; //licznik u�ycia Scroll Lock
  break;
  case 3: //LPT
   LPT=new TLPT(); //otwarcie inpout32.dll
   if (LPT?LPT->Connect(iConfig):false)
   {//wys�a� 0?
    BitsUpdate(-1); //aktualizacjia stan�w, bo przy wczytywaniu mog�o by� nieaktywne
    WriteLog("InpOut32.dll OK");
   }
   else
   {//po��czenie nie wysz�o, ma by� NULL
    delete LPT;
    LPT=NULL;
   }
  break;
  case 4: //PoKeys
   PoKeys55=new TPoKeys55();
   if (PoKeys55?PoKeys55->Connect():false)
   {WriteLog("Found "+PoKeys55->Version());
    BitsUpdate(-1); //aktualizacjia stan�w, bo przy wczytywaniu mog�o by� nieaktywne
   }
   else
   {//po��czenie nie wysz�o, ma by� NULL
    delete PoKeys55;
    PoKeys55=NULL;
   }
  break;
 }
 return 0;
};

void __fastcall Console::Off()
{//wy��czenie informacji zwrotnych (reset pulpitu)
 BitsClear(-1);
 if ((iMode==1)||(iMode==2))
  if (iConfig&1) //licznik u�ycia Scroll Lock
  {//bez sensu to jest, ale mi si� samo w��cza
   SetLedState(VK_SCROLL,true); //przyci�ni�ty
   SetLedState(VK_SCROLL,false); //zwolniony
  }
 delete PoKeys55; PoKeys55=NULL;
 delete LPT; LPT=NULL;
};

void __fastcall Console::BitsSet(int mask,int entry)
{//ustawienie bit�w o podanej masce (mask) na wej�ciu (entry)
 if ((iBits&mask)!=mask) //je�eli zmiana
 {iBits|=mask;
  BitsUpdate(mask);
 }
};

void __fastcall Console::BitsClear(int mask,int entry)
{//zerowanie bit�w o podanej masce (mask) na wej�ciu (entry)
 if (iBits&mask) //je�eli zmiana
 {iBits&=~mask;
  BitsUpdate(mask);
 }
};

void __fastcall Console::BitsUpdate(int mask)
{//aktualizacja stanu interfejsu informacji zwrotnej; (mask) - zakres zmienianych bit�w
 switch (iMode)
 {case 1: //sterowanie �wiate�kami klawiatury: CA/SHP+opory
   if (mask&3) //gdy SHP albo CA
    SetLedState(VK_CAPITAL,iBits&3);
   if (mask&4) //gdy jazda na oporach
   {//Scroll Lock ma jako� dziwnie... zmiana stanu na przeciwny
    SetLedState(VK_SCROLL,true); //przyci�ni�ty
    SetLedState(VK_SCROLL,false); //zwolniony
    ++iConfig; //licznik u�ycia Scroll Lock
   }
   break;
  case 2: //sterowanie �wiate�kami klawiatury: CA+SHP
   if (mask&2) //gdy CA
    SetLedState(VK_CAPITAL,iBits&2);
   if (mask&1) //gdy SHP
   {//Scroll Lock ma jako� dziwnie... zmiana stanu na przeciwny
    SetLedState(VK_SCROLL,true); //przyci�ni�ty
    SetLedState(VK_SCROLL,false); //zwolniony
    ++iConfig; //licznik u�ycia Scroll Lock
   }
   break;
  case 3: //LPT Marcela z modyfikacj� (jazda na oporach zamiast brz�czyka)
   if (LPT)
    LPT->Out(iBits);
   break;
  case 4: //PoKeys55 wg Marcela - wersja druga z ko�ca 2012
   if (PoKeys55)
   {//pewnie trzeba b�dzie to dodatkowo buforowa� i oczekiwa� na potwierdzenie
    if (mask&0x0001) //b0 gdy SHP
     PoKeys55->Write(0x40,23-1,iBits&0x0001?1:0);
    if (mask&0x0002) //b1 gdy zmieniony CA
     PoKeys55->Write(0x40,24-1,iBits&0x0002?1:0);
    if (mask&0x0004) //b2 gdy jazda na oporach
     PoKeys55->Write(0x40,32-1,iBits&0x0004?1:0);
    if (mask&0x0008) //b3 Lampka WS (wy��cznika szybkiego)
     PoKeys55->Write(0x40,25-1,iBits&0x0008?1:0);
    if (mask&0x0010) //b4 Lampka przeka�nika nadmiarowego silnik�w trakcyjnych
     PoKeys55->Write(0x40,27-1,iBits&0x0010?1:0);
    if (mask&0x0020) //b5 Lampka stycznik�w liniowych
     PoKeys55->Write(0x40,29-1,iBits&0x0020?1:0);
    if (mask&0x0040) //b6 Lampka po�lizgu
     PoKeys55->Write(0x40,30-1,iBits&0x0040?1:0);
    if (mask&0x0080) //b7 Lampka "przetwornicy"
     PoKeys55->Write(0x40,28-1,iBits&0x0080?1:0);
    if (mask&0x0100) //b8 Kontrolka przeka�nika nadmiarowego spr�arki
     PoKeys55->Write(0x40,33-1,iBits&0x0100?1:0);
    if (mask&0x0200) //b9 Kontrolka sygnalizacji wentylator�w i opor�w
     PoKeys55->Write(0x40,26-1,iBits&0x0200?1:0);
    if (mask&0x0400) //b10 Kontrolka wysokiego rozruchu
     PoKeys55->Write(0x40,31-1,iBits&0x0400?1:0);
    if (mask&0x0800) //b11 Kontrolka ogrzewania poci�gu
     PoKeys55->Write(0x40,34-1,iBits&0x0800?1:0);
    if (mask&0x1000) //b12 Ci�nienie w cylindrach do odbijania w haslerze
     PoKeys55->Write(0x40,52-1,iBits&0x1000?1:0);
    if (mask&0x2000) //b13 Pr�d na silnikach do odbijania w haslerze
     PoKeys55->Write(0x40,53-1,iBits&0x2000?1:0);
   }
   break;
 }
};

bool __fastcall Console::Pressed(int x)
{//na razie tak - czyta si� tylko klawiatura
 return Global::bActive&&(GetKeyState(x)<0);
};

void __fastcall Console::ValueSet(int x,double y)
{//ustawienie warto�ci (y) na kanale analogowym (x)
 if (iMode==4)
  if (PoKeys55)
  {
   PoKeys55->PWM(x,(((Global::fCalibrateOut[x][3]*y)+Global::fCalibrateOut[x][2])*y+Global::fCalibrateOut[x][1])*y+Global::fCalibrateOut[x][0]); //zakres <0;1>
  }
};

void __fastcall Console::Update()
{//funkcja powinna by� wywo�ywana regularnie, np. raz w ka�dej ramce ekranowej
 if (iMode==4)
  if (PoKeys55)
   if (PoKeys55->Update())
   {//wykrycie przestawionych prze��cznik�w

   }
};

float __fastcall Console::AnalogGet(int x)
{//pobranie warto�ci analogowej
 if (iMode==4)
  if (PoKeys55)
   return PoKeys55->fAnalog[x];
 return -1.0;
};

unsigned char __fastcall Console::DigitalGet(int x)
{//pobranie warto�ci cyfrowej
 if (iMode==4)
  if (PoKeys55)
   return PoKeys55->iInputs[x];
 return 0;
};
