/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include <vcl.h>
#pragma hdrstop

#include "Console.h"
#include "Globals.h"
#include "Logs.h"
#include "PoKeys55.h"
#include "LPT.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
// Ra: klasa statyczna gromadz¹ca sygna³y steruj¹ce oraz informacje zwrotne
// Ra: stan wejœcia zmieniany klawiatur¹ albo dedykowanym urz¹dzeniem
// Ra: stan wyjœcia zmieniany przez symulacjê (mierniki, kontrolki)

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

// Ra: do poprawienia
void SetLedState(char Code, bool bOn)
{ // Ra: bajer do migania LED-ami w klawiaturze
    if (Win32Platform == VER_PLATFORM_WIN32_NT)
    {
        // WriteLog(AnsiString(int(GetAsyncKeyState(Code))));
        if (bool(GetAsyncKeyState(Code)) != bOn)
        {
            keybd_event(Code, MapVirtualKey(Code, 0), KEYEVENTF_EXTENDEDKEY, 0);
            keybd_event(Code, MapVirtualKey(Code, 0), KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
        }
    }
    else
    {
        TKeyboardState KBState;
        GetKeyboardState(KBState);
        KBState[Code] = bOn ? 1 : 0;
        SetKeyboardState(KBState);
    };
};

//---------------------------------------------------------------------------

int Console::iBits = 0; // zmienna statyczna - obiekt Console jest jednen wspólny
int Console::iMode = 0;
int Console::iConfig = 0;
TPoKeys55 *Console::PoKeys55[2] = {NULL, NULL};
TLPT *Console::LPT = NULL;
int Console::iSwitch[8]; // bistabilne w kabinie, za³¹czane z [Shift], wy³¹czane bez
int Console::iButton[8]; // monostabilne w kabinie, za³¹czane podczas trzymania klawisza

Console::Console()
{
    PoKeys55[0] = PoKeys55[1] = NULL;
    for (int i = 0; i < 8; ++i)
    { // zerowanie prze³¹czników
        iSwitch[i] = 0; // bity 0..127 - bez [Ctrl], 128..255 - z [Ctrl]
        iButton[i] = 0; // bity 0..127 - bez [Shift], 128..255 - z [Shift]
    }
};

Console::~Console()
{
    delete PoKeys55[0];
    delete PoKeys55[1];
};

void Console::ModeSet(int m, int h)
{ // ustawienie trybu pracy
    iMode = m;
    iConfig = h;
};

int Console::On()
{ // za³¹czenie konsoli (np. nawi¹zanie komunikacji)
    iSwitch[0] = iSwitch[1] = iSwitch[2] = iSwitch[3] = 0; // bity 0..127 - bez [Ctrl]
    iSwitch[4] = iSwitch[5] = iSwitch[6] = iSwitch[7] = 0; // bity 128..255 - z [Ctrl]
    switch (iMode)
    {
    case 1: // kontrolki klawiatury
    case 2: // kontrolki klawiatury
        iConfig = 0; // licznik u¿ycia Scroll Lock
        break;
    case 3: // LPT
        LPT = new TLPT(); // otwarcie inpout32.dll
        if (LPT ? LPT->Connect(iConfig) : false)
        { // wys³aæ 0?
            BitsUpdate(-1); // aktualizacjia stanów, bo przy wczytywaniu mog³o byæ nieaktywne
            WriteLog("Feedback Mode 3: InpOut32.dll OK");
        }
        else
        { // po³¹czenie nie wysz³o, ma byæ NULL
            delete LPT;
            LPT = NULL;
        }
        break;
    case 4: // PoKeys
        PoKeys55[0] = new TPoKeys55();
        if (PoKeys55[0] ? PoKeys55[0]->Connect() : false)
        {
            WriteLog("Found " + PoKeys55[0]->Version());
            BitsUpdate(-1); // aktualizacjia stanów, bo przy wczytywaniu mog³o byæ nieaktywne
        }
        else
        { // po³¹czenie nie wysz³o, ma byæ NULL
            delete PoKeys55[0];
            PoKeys55[0] = NULL;
        }
        break;
    }
    return 0;
};

void Console::Off()
{ // wy³¹czenie informacji zwrotnych (reset pulpitu)
    BitsClear(-1);
    if ((iMode == 1) || (iMode == 2))
        if (iConfig & 1) // licznik u¿ycia Scroll Lock
        { // bez sensu to jest, ale mi siê samo w³¹cza
            SetLedState(VK_SCROLL, true); // przyciœniêty
            SetLedState(VK_SCROLL, false); // zwolniony
        }
    delete PoKeys55[0];
    PoKeys55[0] = NULL;
    delete PoKeys55[1];
    PoKeys55[1] = NULL;
    delete LPT;
    LPT = NULL;
};

void Console::BitsSet(int mask, int entry)
{ // ustawienie bitów o podanej masce (mask) na wejœciu (entry)
    if ((iBits & mask) != mask) // je¿eli zmiana
    {
        int old = iBits; // poprzednie stany
        iBits |= mask;
        BitsUpdate(old ^ iBits); // 1 dla bitów zmienionych
		if (iMode == 4)
			WriteLog("PoKeys::BitsSet: mask: " + AnsiString(mask) + " iBits: " + AnsiString(iBits));
    }
};

void Console::BitsClear(int mask, int entry)
{ // zerowanie bitów o podanej masce (mask) na wejœciu (entry)
    if (iBits & mask) // je¿eli zmiana
    {
        int old = iBits; // poprzednie stany
        iBits &= ~mask;
        BitsUpdate(old ^ iBits); // 1 dla bitów zmienionych
    }
};

void Console::BitsUpdate(int mask)
{ // aktualizacja stanu interfejsu informacji zwrotnej; (mask) - zakres zmienianych bitów
    switch (iMode)
    {
    case 1: // sterowanie œwiate³kami klawiatury: CA/SHP+opory
        if (mask & 3) // gdy SHP albo CA
            SetLedState(VK_CAPITAL, iBits & 3);
        if (mask & 4) // gdy jazda na oporach
        { // Scroll Lock ma jakoœ dziwnie... zmiana stanu na przeciwny
            SetLedState(VK_SCROLL, true); // przyciœniêty
            SetLedState(VK_SCROLL, false); // zwolniony
            ++iConfig; // licznik u¿ycia Scroll Lock
        }
        break;
    case 2: // sterowanie œwiate³kami klawiatury: CA+SHP
        if (mask & 2) // gdy CA
            SetLedState(VK_CAPITAL, iBits & 2);
        if (mask & 1) // gdy SHP
        { // Scroll Lock ma jakoœ dziwnie... zmiana stanu na przeciwny
            SetLedState(VK_SCROLL, true); // przyciœniêty
            SetLedState(VK_SCROLL, false); // zwolniony
            ++iConfig; // licznik u¿ycia Scroll Lock
        }
        break;
    case 3: // LPT Marcela z modyfikacj¹ (jazda na oporach zamiast brzêczyka)
        if (LPT)
            LPT->Out(iBits);
        break;
    case 4: // PoKeys55 wg Marcela - wersja druga z koñca 2012
        if (PoKeys55[0])
        { // pewnie trzeba bêdzie to dodatkowo buforowaæ i oczekiwaæ na potwierdzenie
            if (mask & 0x0001) // b0 gdy SHP
                PoKeys55[0]->Write(0x40, 23 - 1, iBits & 0x0001 ? 1 : 0);
            if (mask & 0x0002) // b1 gdy zmieniony CA
                PoKeys55[0]->Write(0x40, 24 - 1, iBits & 0x0002 ? 1 : 0);
            if (mask & 0x0004) // b2 gdy jazda na oporach
                PoKeys55[0]->Write(0x40, 32 - 1, iBits & 0x0004 ? 1 : 0);
            if (mask & 0x0008) // b3 Lampka WS (wy³¹cznika szybkiego)
                PoKeys55[0]->Write(0x40, 25 - 1, iBits & 0x0008 ? 1 : 0);
            if (mask & 0x0010) // b4 Lampka przekaŸnika nadmiarowego silników trakcyjnych
                PoKeys55[0]->Write(0x40, 27 - 1, iBits & 0x0010 ? 1 : 0);
            if (mask & 0x0020) // b5 Lampka styczników liniowych
                PoKeys55[0]->Write(0x40, 29 - 1, iBits & 0x0020 ? 1 : 0);
            if (mask & 0x0040) // b6 Lampka poœlizgu
                PoKeys55[0]->Write(0x40, 30 - 1, iBits & 0x0040 ? 1 : 0);
            if (mask & 0x0080) // b7 Lampka "przetwornicy"
                PoKeys55[0]->Write(0x40, 28 - 1, iBits & 0x0080 ? 1 : 0);
            if (mask & 0x0100) // b8 Kontrolka przekaŸnika nadmiarowego sprê¿arki
                PoKeys55[0]->Write(0x40, 33 - 1, iBits & 0x0100 ? 1 : 0);
            if (mask & 0x0200) // b9 Kontrolka sygnalizacji wentylatorów i oporów
                PoKeys55[0]->Write(0x40, 26 - 1, iBits & 0x0200 ? 1 : 0);
            if (mask & 0x0400) // b10 Kontrolka wysokiego rozruchu
                PoKeys55[0]->Write(0x40, 31 - 1, iBits & 0x0400 ? 1 : 0);
            if (mask & 0x0800) // b11 Kontrolka ogrzewania poci¹gu
                PoKeys55[0]->Write(0x40, 34 - 1, iBits & 0x0800 ? 1 : 0);
            if (mask & 0x1000) // b12 Ciœnienie w cylindrach do odbijania w haslerze
                PoKeys55[0]->Write(0x40, 52 - 1, iBits & 0x1000 ? 1 : 0);
            if (mask & 0x2000) // b13 Pr¹d na silnikach do odbijania w haslerze
                PoKeys55[0]->Write(0x40, 53 - 1, iBits & 0x2000 ? 1 : 0);
			if (mask & 0x4000) // b14 Brzêczyk SHP lub CA
				PoKeys55[0]->Write(0x40, 16 - 1, iBits & 0x4000 ? 1 : 0);
		}
        break;
    }
};

bool Console::Pressed(int x)
{ // na razie tak - czyta siê tylko klawiatura
    return Global::bActive && (GetKeyState(x) < 0);
};

void Console::ValueSet(int x, double y)
{ // ustawienie wartoœci (y) na kanale analogowym (x)
    if (iMode == 4)
        if (PoKeys55[0])
        {
   			x = Global::iPoKeysPWM[x];
			if (Global::iCalibrateOutDebugInfo == x)
				WriteLog("CalibrateOutDebugInfo: oryginal=" + AnsiString(y), false);
			if (Global::fCalibrateOutMax[x] > 0)
			{
				y = Global::CutValueToRange(0, y, Global::fCalibrateOutMax[x]);
				if (Global::iCalibrateOutDebugInfo == x)
					WriteLog(" cutted=" + AnsiString(y),false);
				y = y / Global::fCalibrateOutMax[x]; // sprowadzenie do <0,1> jeœli podana maksymalna wartoœæ
				if (Global::iCalibrateOutDebugInfo == x)
					WriteLog(" fraction=" + AnsiString(y),false);
			}
			double temp = (((((Global::fCalibrateOut[x][5] * y) + Global::fCalibrateOut[x][4]) * y +
				Global::fCalibrateOut[x][3]) * y + Global::fCalibrateOut[x][2]) * y +
				Global::fCalibrateOut[x][1]) * y +
				Global::fCalibrateOut[x][0]; // zakres <0;1>
			if (Global::iCalibrateOutDebugInfo == x)
				WriteLog(" calibrated=" + AnsiString(temp));
			PoKeys55[0]->PWM(x, temp); 
        }
};

void Console::Update()
{ // funkcja powinna byæ wywo³ywana regularnie, np. raz w ka¿dej ramce ekranowej
    if (iMode == 4)
        if (PoKeys55[0])
            if (PoKeys55[0]->Update((Global::iPause & 8) > 0))
            { // wykrycie przestawionych prze³¹czników?
                Global::iPause &= ~8;
            }
            else
            { // b³¹d komunikacji - zapauzowaæ symulacjê?
                if (!(Global::iPause & 8)) // jeœli jeszcze nie oflagowana
                    Global::iTextMode = VK_F1; // pokazanie czasu/pauzy
                Global::iPause |= 8; // tak???
                PoKeys55[0]->Connect(); // próba ponownego pod³¹czenia
            }
};

float Console::AnalogGet(int x)
{ // pobranie wartoœci analogowej
    if (iMode == 4)
        if (PoKeys55[0])
            return PoKeys55[0]->fAnalog[x];
    return -1.0;
};

float Console::AnalogCalibrateGet(int x)
{ // pobranie i kalibracja wartoœci analogowej, jeœli nie ma PoKeys zwraca NULL
	if (iMode == 4 && PoKeys55[0])
	{
		float b = PoKeys55[0]->fAnalog[x];
		return (((((Global::fCalibrateIn[x][5] * b) + Global::fCalibrateIn[x][4]) * b +
			Global::fCalibrateIn[x][3]) * b + Global::fCalibrateIn[x][2]) * b +
			Global::fCalibrateIn[x][1]) *b + Global::fCalibrateIn[x][0];
	}
    return -1.0; //odciêcie
};

unsigned char Console::DigitalGet(int x)
{ // pobranie wartoœci cyfrowej
    if (iMode == 4)
        if (PoKeys55[0])
            return PoKeys55[0]->iInputs[x];
    return 0;
};

void Console::OnKeyDown(int k)
{ // naciœniêcie klawisza z powoduje wy³¹czenie, a
    if (k & 0x10000) // jeœli [Shift]
    { // ustawienie bitu w tabeli prze³¹czników bistabilnych
        if (k & 0x20000) // jeœli [Ctrl], to zestaw dodatkowy
            iSwitch[4 + (char(k) >> 5)] |= 1 << (k & 31); // za³¹cz bistabliny dodatkowy
        else
        { // z [Shift] w³¹czenie bitu bistabilnego i dodatkowego monostabilnego
            iSwitch[char(k) >> 5] |= 1 << (k & 31); // za³¹cz bistabliny podstawowy
            iButton[4 + (char(k) >> 5)] |= (1 << (k & 31)); // za³¹cz monostabilny dodatkowy
        }
    }
    else
    { // zerowanie bitu w tabeli prze³¹czników bistabilnych
        if (k & 0x20000) // jeœli [Ctrl], to zestaw dodatkowy
            iSwitch[4 + (char(k) >> 5)] &= ~(1 << (k & 31)); // wy³¹cz bistabilny dodatkowy
        else
        {
            iSwitch[char(k) >> 5] &= ~(1 << (k & 31)); // wy³¹cz bistabilny podstawowy
            iButton[char(k) >> 5] |= 1 << (k & 31); // za³¹cz monostabilny podstawowy
        }
    }
};
void Console::OnKeyUp(int k)
{ // puszczenie klawisza w zasadzie nie ma znaczenia dla iSwitch, ale zeruje iButton
    if ((k & 0x20000) == 0) // monostabilne tylko bez [Ctrl]
        if (k & 0x10000) // jeœli [Shift]
            iButton[4 + (char(k) >> 5)] &= ~(1 << (k & 31)); // wy³¹cz monostabilny dodatkowy
        else
            iButton[char(k) >> 5] &= ~(1 << (k & 31)); // wy³¹cz monostabilny podstawowy
};
int Console::KeyDownConvert(int k)
{
    return int(ktTable[k & 0x3FF].iDown);
};
int Console::KeyUpConvert(int k)
{
    return int(ktTable[k & 0x3FF].iUp);
};
