//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Console.h"
#include "Globals.h"
#include "Logs.h"
#include "PoKeys55.h"

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

int Console::iBits=0; //zmienna statyczna - obiekt Console jest jednen wsp�lny
int Console::iMode=0;
int Console::iConfig=0;
TPoKeys55 *Console::PoKeys55=NULL;

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
 return 0;
};

void __fastcall Console::Off()
{//wy��czenie informacji zwrotnych (reset pulpitu)
 BitsClear(-1);
 delete PoKeys55;
 PoKeys55=NULL;
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
    SetLedState(ktCapsLock,iBits&3);
   if (mask&4) //gdy jazda na oporach
   {//Scroll Lock ma jako� dziwnie... zmiana stanu na przeciwny
    SetLedState(ktScrollLock,true); //przyci�ni�ty
    SetLedState(ktScrollLock,false); //zwolniony
   }
   break;
  case 2: //sterowanie �wiate�kami klawiatury: CA+SHP
   if (mask&2) //gdy CA
    SetLedState(ktCapsLock,iBits&2);
   if (mask&1) //gdy SHP
   {//Scroll Lock ma jako� dziwnie... zmiana stanu na przeciwny
    SetLedState(ktScrollLock,true); //przyci�ni�ty
    SetLedState(ktScrollLock,false); //zwolniony
   }
   break;
  case 3: //LPT Marcela z modyfikacj� (jazda na oporach zamiast brz�czyka)
   // (do zrobienia)
   break;
  case 4: //PoKeys55 wg Marcela
   if (PoKeys55)
   {//pewnie trzeba b�dzie to dodatkowo buforowa� i oczekiwa� na potwierdzenie
    if (mask&0x001) //gdy SHP
     PoKeys55->Write(0x40,41-1,iBits&0x001?1:0);
    if (mask&0x002) //gdy zmieniony CA
     PoKeys55->Write(0x40,40-1,iBits&0x002?1:0);
    if (mask&0x004) //gdy jazda na oporach
     PoKeys55->Write(0x40,50-1,iBits&0x004?1:0);
    if (mask&0x008) //b3 Lampka WS (wy��cznika szybkiego)
     PoKeys55->Write(0x40,51-1,iBits&0x008?1:0);
    if (mask&0x010) //b4 Lampka przeka�nika nadmiarowego silnik�w trakcyjnych
     PoKeys55->Write(0x40,52-1,iBits&0x010?1:0);
    if (mask&0x020) //b5 Lampka stycznik�w liniowych
     PoKeys55->Write(0x40,54-1,iBits&0x020?1:0);
    if (mask&0x040) //b6 Lampka po�lizgu
     PoKeys55->Write(0x40,55-1,iBits&0x040?1:0);
    if (mask&0x080) //b7 Lampka "przetwornicy"
     PoKeys55->Write(0x40,53-1,iBits&0x080?1:0);
    if (mask&0x100) //b8 Kontrolka przeka�nika nadmiarowego spr�arki
     PoKeys55->Write(0x40,42-1,iBits&0x100?1:0);
    if (mask&0x200) //b9 Kontrolka sygnalizacji wentylator�w i opor�w
     PoKeys55->Write(0x40,48-1,iBits&0x200?1:0);
    if (mask&0x400) //b10 Kontrolka wysokiego rozruchu
     PoKeys55->Write(0x40,49-1,iBits&0x400?1:0);
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

