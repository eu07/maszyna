//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Console.h"
#include "Globals.h"
#include "Logs.h"
//#include "RaPoKeys.h"

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

int Console::iBits=0; //zmienna statyczna - obiekt Console jest jednen wsp�lny
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
{//za��czenie konsoli (np. nawi�zanie komunikacji)
 //WriteLog(TPoKeys::Count());
 //pomain();
};

void __fastcall Console::Off()
{//wy��czenie informacji zwrotnych (reset pulpitu)
 BitsClear(-1);
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
{//aktualizacja stanu interfejsu informacji zwrotnej
 switch (iMode)
 {case 1: //sterowanie �wiate�kami klawiatury: CA/SHP+opory
   if (mask&3) //gdy SHP albo CA
    SetLedState(ktCapsLock,iBits&3);
   if (mask==256) //gdy jazda na oporach
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
  case 3: //LPT Marcela
   // (do zrobienia)
   break;
 }
};

bool __fastcall Console::Pressed(int x)
{//na razie tak
 return Global::bActive&&(GetKeyState(x)<0);
};
