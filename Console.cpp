/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "Console.h"
#include "Globals.h"
#include "LPT.h"
#include "Logs.h"
#include "MWD.h" // maciek001: obsluga portu COM
#include "PoKeys55.h"
#include "mczapkie/mctools.h"

//---------------------------------------------------------------------------
// Ra: klasa statyczna gromadząca sygnały sterujące oraz informacje zwrotne
// Ra: stan wejścia zmieniany klawiaturą albo dedykowanym urządzeniem
// Ra: stan wyjścia zmieniany przez symulację (mierniki, kontrolki)

/*******************************
Do klawisza klawiatury przypisana jest maska bitowa oraz numer wejścia.
Naciśnięcie klawisza powoduje wywołanie procedury ustawienia bitu o podanej
masce na podanym wejściu. Zwonienie klawisza analogicznie wywołuje zerowanie
bitu wg maski. Zasadniczo w masce ustawiony jest jeden bit, ale w razie
potrzeby może być ich więcej.

Oddzielne wejścia są wprowadzone po to, by można było używać więcej niż 32
bity do sterowania. Podział na wejścia jest również ze względów organizacyjnych,
np. sterowanie światłami może mieć oddzielny numer wejścia niż przełączanie
radia, ponieważ nie ma potrzeby ich uzależniać (tzn. badać wspólną maskę bitową).

Do każdego wejścia podpięty jest skrypt binarny, charakterystyczny dla danej
konstrukcji pojazdu. Sprawdza on zależności (w tym uszkodzenia) za pomocą
operacji logicznych na maskach bitowych. Do każdego wejścia jest przypisana
jedna, oddzielna maska 32 bit, ale w razie potrzeby istnieje też możliwość
korzystania z masek innych wejść. Skrypt może też wysyłać maski na inne wejścia,
ale należy unikać rekurencji.

Definiowanie wejść oraz przeznaczenia ich masek jest w gestii konstruktora
skryptu. Każdy pojazd może mieć inny schemat wejść i masek, ale w miarę możliwości
należy dążyć do unifikacji. Skrypty mogą również używać dodatkowych masek bitowych.
Maski bitowe odpowiadają stanom przełączników, czujników, styczników itd.

Działanie jest następujące:
- na klawiaturze konsoli naciskany jest przycisk
- naciśnięcie przycisku zamieniane jest na maskę bitową oraz numer wejścia
- wywoływany jest skrypt danego wejścia z ową maską
- skrypt sprawdza zależności i np. modyfikuje własności fizyki albo inne maski
- ewentualnie do wyzwalacza czasowego dodana jest maska i numer wejścia

/*******************************/

/* //kod do przetrawienia:
//aby się nie włączacz wygaszacz ekranu, co jakiś czas naciska się wirtualnie ScrollLock

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

// static class member storage allocation
TKeyTrans Console::ktTable[4 * 256];

// Ra: do poprawienia
void SetLedState(char Code, bool bOn){
    // Ra: bajer do migania LED-ami w klawiaturze
    // NOTE: disabled for the time being
    // TODO: find non Borland specific equivalent, or get rid of it
    /*   if (Win32Platform == VER_PLATFORM_WIN32_NT)
        {
            // WriteLog(AnsiString(int(GetAsyncKeyState(Code))));
            if (bool(GetAsyncKeyState(Code)) != bOn)
            {
                keybd_event(Code, MapVirtualKey(Code, 0), KEYEVENTF_EXTENDEDKEY, 0);
                keybd_event(Code, MapVirtualKey(Code, 0), KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,
       0);
            }
        }
        else
        {
            TKeyboardState KBState;
            GetKeyboardState(KBState);
            KBState[Code] = bOn ? 1 : 0;
            SetKeyboardState(KBState);
        };
    */
};

//---------------------------------------------------------------------------

int Console::iBits = 0; // zmienna statyczna - obiekt Console jest jednen wspólny
int Console::iMode = 0;
int Console::iConfig = 0;
TPoKeys55 *Console::PoKeys55[2] = {NULL, NULL};
TLPT *Console::LPT = NULL;
MWDComm *Console::MWD = NULL; // maciek001: obiekt dla MWD
int Console::iSwitch[8]; // bistabilne w kabinie, załączane z [Shift], wyłączane bez
int Console::iButton[8]; // monostabilne w kabinie, załączane podczas trzymania klawisza

Console::Console()
{
    PoKeys55[0] = PoKeys55[1] = NULL;
    for (int i = 0; i < 8; ++i)
    { // zerowanie przełączników
        iSwitch[i] = 0; // bity 0..127 - bez [Ctrl], 128..255 - z [Ctrl]
        iButton[i] = 0; // bity 0..127 - bez [Shift], 128..255 - z [Shift]
    }
};

Console::~Console()
{
    delete PoKeys55[0];
    delete PoKeys55[1];
    delete MWD;
};

void Console::ModeSet(int m, int h)
{ // ustawienie trybu pracy
    iMode = m;
    iConfig = h;
};

int Console::On()
{ // załączenie konsoli (np. nawiązanie komunikacji)
    iSwitch[0] = iSwitch[1] = iSwitch[2] = iSwitch[3] = 0; // bity 0..127 - bez [Ctrl]
    iSwitch[4] = iSwitch[5] = iSwitch[6] = iSwitch[7] = 0; // bity 128..255 - z [Ctrl]
    switch (iMode)
    {
    case 1: // kontrolki klawiatury
    case 2: // kontrolki klawiatury
        iConfig = 0; // licznik użycia Scroll Lock
        break;
    case 3: // LPT
        LPT = new TLPT(); // otwarcie inpout32.dll
        if (LPT ? LPT->Connect(iConfig) : false)
        { // wysłać 0?
            BitsUpdate(-1); // aktualizacjia stanów, bo przy wczytywaniu mogło być nieaktywne
            WriteLog("Feedback Mode 3: InpOut32.dll OK");
        }
        else
        { // połączenie nie wyszło, ma być NULL
            delete LPT;
            LPT = NULL;
        }
        break;
    case 4: // PoKeys
        PoKeys55[0] = new TPoKeys55();
        if (PoKeys55[0] ? PoKeys55[0]->Connect() : false)
        {
            WriteLog("Found " + PoKeys55[0]->Version());
            BitsUpdate(-1); // aktualizacjia stanów, bo przy wczytywaniu mogło być nieaktywne
        }
        else
        { // połączenie nie wyszło, ma być NULL
            delete PoKeys55[0];
            PoKeys55[0] = NULL;
        }
        break;
    case 5: // maciek001: MWD OK
        MWD = new MWDComm();
        if (!(MWD->Open()))
        {
            delete MWD;
            iMode = 0;
            Global::iFeedbackMode = 0;
            WriteLog("COM Port not open!");
        }
        break;
    }
    return 0;
};

void Console::Off()
{ // wyłączenie informacji zwrotnych (reset pulpitu)
    BitsClear(-1);
    if ((iMode == 1) || (iMode == 2))
        if (iConfig & 1) // licznik użycia Scroll Lock
        { // bez sensu to jest, ale mi się samo włącza
            SetLedState(VK_SCROLL, true); // przyciśnięty
            SetLedState(VK_SCROLL, false); // zwolniony
        }
    delete PoKeys55[0];
    PoKeys55[0] = NULL;
    delete PoKeys55[1];
    PoKeys55[1] = NULL;
    delete LPT;
    LPT = NULL;
    delete MWD; // maciek001: zamykanie portu COM i takie tam inne ;)
    MWD = NULL;
};

void Console::BitsSet(int mask, int entry)
{ // ustawienie bitów o podanej masce (mask) na wejściu (entry)
    if ((iBits & mask) != mask) // jeżeli zmiana
    {
        int old = iBits; // poprzednie stany
        iBits |= mask;
        BitsUpdate(old ^ iBits); // 1 dla bitów zmienionych
        if (iMode == 4)
            WriteLog("PoKeys::BitsSet: mask: " + std::to_string(mask) +
                     " iBits: " + std::to_string(iBits));
    }
};

void Console::BitsClear(int mask, int entry)
{ // zerowanie bitów o podanej masce (mask) na wejściu (entry)
    if (iBits & mask) // jeżeli zmiana
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
    case 1: // sterowanie światełkami klawiatury: CA/SHP+opory
        if (mask & 3) // gdy SHP albo CA
            SetLedState(VK_CAPITAL, iBits & 3);
        if (mask & 4) // gdy jazda na oporach
        { // Scroll Lock ma jakoś dziwnie... zmiana stanu na przeciwny
            SetLedState(VK_SCROLL, true); // przyciśnięty
            SetLedState(VK_SCROLL, false); // zwolniony
            ++iConfig; // licznik użycia Scroll Lock
        }
        break;
    case 2: // sterowanie światełkami klawiatury: CA+SHP
        if (mask & 2) // gdy CA
            SetLedState(VK_CAPITAL, iBits & 2);
        if (mask & 1) // gdy SHP
        { // Scroll Lock ma jakoś dziwnie... zmiana stanu na przeciwny
            SetLedState(VK_CAPITAL, true); // przyciśnięty
            SetLedState(VK_CAPITAL, false); // zwolniony
            ++iConfig; // licznik użycia Scroll Lock
        }
        break;
    case 3: // LPT Marcela z modyfikacją (jazda na oporach zamiast brzęczyka)
        if (LPT)
            LPT->Out(iBits);
        break;
    case 4: // PoKeys55 wg Marcela - wersja druga z końca 2012
        if (PoKeys55[0])
        { // pewnie trzeba będzie to dodatkowo buforować i oczekiwać na potwierdzenie
            if (mask & 0x0001) // b0 gdy SHP
                PoKeys55[0]->Write(0x40, 23 - 1, (iBits & 0x0001) ? 1 : 0);
            if (mask & 0x0002) // b1 gdy zmieniony CA
                PoKeys55[0]->Write(0x40, 24 - 1, (iBits & 0x0002) ? 1 : 0);
            if (mask & 0x0004) // b2 gdy jazda na oporach
                PoKeys55[0]->Write(0x40, 32 - 1, (iBits & 0x0004) ? 1 : 0);
            if (mask & 0x0008) // b3 Lampka WS (wyłącznika szybkiego)
                PoKeys55[0]->Write(0x40, 25 - 1, (iBits & 0x0008) ? 1 : 0);
            if (mask & 0x0010) // b4 Lampka przekaźnika nadmiarowego silników trakcyjnych
                PoKeys55[0]->Write(0x40, 27 - 1, (iBits & 0x0010) ? 1 : 0);
            if (mask & 0x0020) // b5 Lampka styczników liniowych
                PoKeys55[0]->Write(0x40, 29 - 1, (iBits & 0x0020) ? 1 : 0);
            if (mask & 0x0040) // b6 Lampka poślizgu
                PoKeys55[0]->Write(0x40, 30 - 1, (iBits & 0x0040) ? 1 : 0);
            if (mask & 0x0080) // b7 Lampka "przetwornicy"
                PoKeys55[0]->Write(0x40, 28 - 1, (iBits & 0x0080) ? 1 : 0);
            if (mask & 0x0100) // b8 Kontrolka przekaźnika nadmiarowego sprężarki
                PoKeys55[0]->Write(0x40, 33 - 1, (iBits & 0x0100) ? 1 : 0);
            if (mask & 0x0200) // b9 Kontrolka sygnalizacji wentylatorów i oporów
                PoKeys55[0]->Write(0x40, 26 - 1, (iBits & 0x0200) ? 1 : 0);
            if (mask & 0x0400) // b10 Kontrolka wysokiego rozruchu
                PoKeys55[0]->Write(0x40, 31 - 1, (iBits & 0x0400) ? 1 : 0);
            if (mask & 0x0800) // b11 Kontrolka ogrzewania pociągu
                PoKeys55[0]->Write(0x40, 34 - 1, (iBits & 0x0800) ? 1 : 0);
            if (mask & 0x1000) // b12 Ciśnienie w cylindrach do odbijania w haslerze
                PoKeys55[0]->Write(0x40, 52 - 1, (iBits & 0x1000) ? 1 : 0);
            if (mask & 0x2000) // b13 Prąd na silnikach do odbijania w haslerze
                PoKeys55[0]->Write(0x40, 53 - 1, (iBits & 0x2000) ? 1 : 0);
            if (mask & 0x4000) // b14 Brzęczyk SHP lub CA
                PoKeys55[0]->Write(0x40, 16 - 1, (iBits & 0x4000) ? 1 : 0);
        }
        break;
    case 5: // maciek001: MWD lampki i kontrolki
        /* out3: ogrzewanie sk?adu, opory rozruchowe, poslizg, zaluzjewent, -, -, czuwak, shp
           out4: stycz.liniowe, pezekaznikróżnicobwpomoc, nadmiarprzetw, roznicowy obw. gł,
           nadmiarsilniki, wylszybki, zanikprąduprzyjeździenaoporach, nadmiarsprezarki
           out5: HASLER */
        if (mask & 0x0001)
            if (iBits & 1)
            {
                MWD->WriteDataBuff[4] |= 1 << 7; // SHP	HASLER też
                if (!MWD->bSHPstate)
                {
                    MWD->bSHPstate = true;
                    MWD->bPrzejazdSHP = true;
                }
                else
                    MWD->bPrzejazdSHP = false;
            }
            else
            {
                MWD->WriteDataBuff[4] &= ~(1 << 7);
                MWD->bPrzejazdSHP = false;
                MWD->bSHPstate = false;
            }
        if (mask & 0x0002)
            if (iBits & 2)
                MWD->WriteDataBuff[4] |= 1 << 6; // CA
            else
                MWD->WriteDataBuff[4] &= ~(1 << 6);
        if (mask & 0x0004)
            if (iBits & 4)
                MWD->WriteDataBuff[4] |= 1 << 1; // jazda na oporach rozruchowych
            else
                MWD->WriteDataBuff[4] &= ~(1 << 1);
        if (mask & 0x0008)
            if (iBits & 8)
                MWD->WriteDataBuff[5] |= 1 << 5; // wyłącznik szybki
            else
                MWD->WriteDataBuff[5] &= ~(1 << 5);
        if (mask & 0x0010)
            if (iBits & 0x10)
                MWD->WriteDataBuff[5] |= 1 << 4; // nadmiarowy silników trakcyjnych
            else
                MWD->WriteDataBuff[5] &= ~(1 << 4);
        if (mask & 0x0020)
            if (iBits & 0x20)
                MWD->WriteDataBuff[4] |= 1 << 0; // styczniki liniowe
            else
                MWD->WriteDataBuff[5] &= ~(1 << 0);
        if (mask & 0x0040)
            if (iBits & 0x40)
                MWD->WriteDataBuff[4] |= 1 << 2; // poślizg
            else
                MWD->WriteDataBuff[4] &= ~(1 << 2);
        if (mask & 0x0080)
            if (iBits & 0x80)
                MWD->WriteDataBuff[5] |= 1 << 2; // (nadmiarowy) przetwornicy? ++
            else
                MWD->WriteDataBuff[5] &= ~(1 << 2);
        if (mask & 0x0100)
            if (iBits & 0x100)
                MWD->WriteDataBuff[5] |= 1 << 7; // nadmiarowy sprężarki
            else
                MWD->WriteDataBuff[5] &= ~(1 << 7);
        if (mask & 0x0200)
            if (iBits & 0x200)
                MWD->WriteDataBuff[2] |= 1 << 1; // wentylatory i opory
            else
                MWD->WriteDataBuff[2] &= ~(1 << 1);
        if (mask & 0x0400)
            if (iBits & 0x400)
                MWD->WriteDataBuff[2] |= 1 << 2; // wysoki rozruch
            else
                MWD->WriteDataBuff[2] &= ~(1 << 2);
        if (mask & 0x0800)
            if (iBits & 0x800)
                MWD->WriteDataBuff[4] |= 1 << 0; // ogrzewanie pociągu
            else
                MWD->WriteDataBuff[4] &= ~(1 << 0);
        if (mask & 0x1000)
            if (iBits & 0x1000)
                MWD->bHamowanie = true; // hasler: ciśnienie w hamulcach 	HASLER rysik 2
            else
                MWD->bHamowanie = false;
        if (mask & 0x2000)
            if (iBits & 0x2000)
                MWD->WriteDataBuff[6] |=
                    1 << 4; // hasler: prąd "na" silnikach 		HASLER rysik 3
            else
                MWD->WriteDataBuff[6] &= ~(1 << 4);
        if (mask & 0x4000)
            if (iBits & 0x4000)
                MWD->WriteDataBuff[6] |= 1 << 7; // brzęczyk SHP/CA
            else
                MWD->WriteDataBuff[6] &= ~(1 << 7);
        // if(mask & 0x8000) if(iBits & 0x8000) MWD->WriteDataBuff[1] |= 1<<7; (puste)
        // else MWD->WriteDataBuff[0] &= ~(1<<7);
        break;
    }
};

bool Console::Pressed(int x)
{ // na razie tak - czyta się tylko klawiatura
	if (!Global::bActive)
		return false;

	if (glfwGetKey(Global::window, x) == GLFW_TRUE)
		return true;
	else
		return false;
};

void Console::ValueSet(int x, double y)
{ // ustawienie wartości (y) na kanale analogowym (x)
    if (iMode == 4)
        if (PoKeys55[0])
        {
            x = Global::iPoKeysPWM[x];
            if (Global::iCalibrateOutDebugInfo == x)
                WriteLog("CalibrateOutDebugInfo: oryginal=" + std::to_string(y), false);
            if (Global::fCalibrateOutMax[x] > 0)
            {
                y = Global::CutValueToRange(0, y, Global::fCalibrateOutMax[x]);
                if (Global::iCalibrateOutDebugInfo == x)
                    WriteLog(" cutted=" + std::to_string(y), false);
                y = y / Global::fCalibrateOutMax[x]; // sprowadzenie do <0,1> jeśli podana
                                                     // maksymalna wartość
                if (Global::iCalibrateOutDebugInfo == x)
                    WriteLog(" fraction=" + std::to_string(y), false);
            }
            double temp = (((((Global::fCalibrateOut[x][5] * y) + Global::fCalibrateOut[x][4]) * y +
                             Global::fCalibrateOut[x][3]) *
                                y +
                            Global::fCalibrateOut[x][2]) *
                               y +
                           Global::fCalibrateOut[x][1]) *
                              y +
                          Global::fCalibrateOut[x][0]; // zakres <0;1>
            if (Global::iCalibrateOutDebugInfo == x)
                WriteLog(" calibrated=" + std::to_string(temp));
            PoKeys55[0]->PWM(x, temp);
        }
    if (iMode == 5 && MWD) // pwm-y i prędkość
    {
        unsigned int iliczba;
        if (x == 0)
        {
            iliczba = (unsigned int)floor((y / (Global::fMWDzg[0] * 10) * Global::fMWDzg[1]) +
                                          0.5); // zbiornik główny
            MWD->WriteDataBuff[12] = (unsigned char)(iliczba >> 8);
            MWD->WriteDataBuff[11] = (unsigned char)iliczba;
        }
        else if (x == 1)
        {
            iliczba = (unsigned int)floor((y / (Global::fMWDpg[0] * 10) * Global::fMWDpg[1]) +
                                          0.5); // przewód główny
            MWD->WriteDataBuff[10] = (unsigned char)(iliczba >> 8);
            MWD->WriteDataBuff[9] = (unsigned char)iliczba;
        }
        else if (x == 2)
        {
            iliczba = (unsigned int)floor((y / (Global::fMWDph[0] * 10) * Global::fMWDph[1]) +
                                          0.5); // cylinder hamulcowy
            MWD->WriteDataBuff[8] = (unsigned char)(iliczba >> 8);
            MWD->WriteDataBuff[7] = (unsigned char)iliczba;
        }
        else if (x == 3)
        {
            iliczba = (unsigned int)floor((y / Global::fMWDvolt[0] * Global::fMWDvolt[1]) +
                                          0.5); // woltomierz WN
            MWD->WriteDataBuff[14] = (unsigned char)(iliczba >> 8);
            MWD->WriteDataBuff[13] = (unsigned char)iliczba;
        }
        else if (x == 4)
        {
            iliczba = (unsigned int)floor((y / Global::fMWDamp[0] * Global::fMWDamp[1]) +
                                          0.5); // amp WN 1
            MWD->WriteDataBuff[16] = (unsigned char)(iliczba >> 8);
            MWD->WriteDataBuff[15] = (unsigned char)iliczba;
        }
        else if (x == 5)
        {
            iliczba = (unsigned int)floor((y / Global::fMWDamp[0] * Global::fMWDamp[1]) +
                                          0.5); // amp WN 2
            MWD->WriteDataBuff[18] = (unsigned char)(iliczba >> 8);
            MWD->WriteDataBuff[17] = (unsigned char)iliczba;
        }
        else if (x == 6)
        {
            iliczba = (unsigned int)floor((y / Global::fMWDamp[0] * Global::fMWDamp[1]) +
                                          0.5); // amp WN 3
            MWD->WriteDataBuff[20] = (unsigned int)(iliczba >> 8);
            MWD->WriteDataBuff[19] = (unsigned char)iliczba;
        }
        else if (x == 7)
            MWD->WriteDataBuff[0] = (unsigned char)floor(y); // prędkość
    }
};

void Console::Update()
{ // funkcja powinna być wywoływana regularnie, np. raz w każdej ramce ekranowej
    if (iMode == 4)
        if (PoKeys55[0])
            if (PoKeys55[0]->Update((Global::iPause & 8) > 0))
            { // wykrycie przestawionych przełączników?
                Global::iPause &= ~8;
            }
            else
            { // błąd komunikacji - zapauzować symulację?
                if (!(Global::iPause & 8)) // jeśli jeszcze nie oflagowana
                    Global::iTextMode = GLFW_KEY_F1; // pokazanie czasu/pauzy
                Global::iPause |= 8; // tak???
                PoKeys55[0]->Connect(); // próba ponownego podłączenia
            }
    if (iMode == 5) // Obs?uga MWD OK
    {
        MWD->Run();
    }
};

float Console::AnalogGet(int x)
{ // pobranie wartości analogowej
    if (iMode == 4)
        if (PoKeys55[0])
            return PoKeys55[0]->fAnalog[x];
    return -1.0;
};

float Console::AnalogCalibrateGet(int x)
{ // pobranie i kalibracja wartości analogowej, jeśli nie ma PoKeys zwraca NULL
    if (iMode == 4 && PoKeys55[0])
    {
        float b = PoKeys55[0]->fAnalog[x];
        return (((((Global::fCalibrateIn[x][5] * b) + Global::fCalibrateIn[x][4]) * b +
                  Global::fCalibrateIn[x][3]) *
                     b +
                 Global::fCalibrateIn[x][2]) *
                    b +
                Global::fCalibrateIn[x][1]) *
                   b +
               Global::fCalibrateIn[x][0];
    }
    if (iMode == 5 && MWD) // maciek001: obs?uga hamulc?w (wej?? analogowych) OK
    {
        float b = MWD->fAnalog[x];
        // b =
        // b*(Global::fMWDAnalogCalib[x][0]-Global::fMWDAnalogCalib[x][1])/Global::fMWDAnalogCalib[x][3]+Global::fMWDAnalogCalib[x][1]/Global::fMWDAnalogCalib[x][3];
        b = (b - Global::fMWDAnalogCalib[x][1]) /
            (Global::fMWDAnalogCalib[x][1] - Global::fMWDAnalogCalib[x][0]);
        switch (x)
        {
        case 0:
            return (b * 8 - 2);
            break;
        case 1:
            return (b * 10);
            break;
        default:
            return 0;
        }
    }
    return -1.0; // odcięcie
};

unsigned char Console::DigitalGet(int x)
{ // pobranie wartości cyfrowej
    if (iMode == 4)
        if (PoKeys55[0])
            return PoKeys55[0]->iInputs[x];
    return 0;
};

void Console::OnKeyDown(int k)
{ // naciśnięcie klawisza z powoduje wyłączenie, a
    if (k & 0x10000) // jeśli [Shift]
    { // ustawienie bitu w tabeli przełączników bistabilnych
        if (k & 0x20000) // jeśli [Ctrl], to zestaw dodatkowy
            iSwitch[4 + (char(k) >> 5)] |= 1 << (k & 31); // załącz bistabliny dodatkowy
        else
        { // z [Shift] włączenie bitu bistabilnego i dodatkowego monostabilnego
            iSwitch[char(k) >> 5] |= 1 << (k & 31); // załącz bistabliny podstawowy
            iButton[4 + (char(k) >> 5)] |= (1 << (k & 31)); // załącz monostabilny dodatkowy
        }
    }
    else
    { // zerowanie bitu w tabeli przełączników bistabilnych
        if (k & 0x20000) // jeśli [Ctrl], to zestaw dodatkowy
            iSwitch[4 + (char(k) >> 5)] &= ~(1 << (k & 31)); // wyłącz bistabilny dodatkowy
        else
        {
            iSwitch[char(k) >> 5] &= ~(1 << (k & 31)); // wyłącz bistabilny podstawowy
            iButton[char(k) >> 5] |= 1 << (k & 31); // załącz monostabilny podstawowy
        }
    }
};
void Console::OnKeyUp(int k)
{ // puszczenie klawisza w zasadzie nie ma znaczenia dla iSwitch, ale zeruje iButton
    if ((k & 0x20000) == 0) // monostabilne tylko bez [Ctrl]
        if (k & 0x10000) // jeśli [Shift]
            iButton[4 + (char(k) >> 5)] &= ~(1 << (k & 31)); // wyłącz monostabilny dodatkowy
        else
            iButton[char(k) >> 5] &= ~(1 << (k & 31)); // wyłącz monostabilny podstawowy
};
int Console::KeyDownConvert(int k)
{
    return int(ktTable[k & 0x3FF].iDown);
};
int Console::KeyUpConvert(int k)
{
    return int(ktTable[k & 0x3FF].iUp);
};
