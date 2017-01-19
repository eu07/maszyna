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
// Ra: klasa statyczna gromadz�ca sygna�y steruj�ce oraz informacje zwrotne
// Ra: stan wej�cia zmieniany klawiatur� albo dedykowanym urz�dzeniem
// Ra: stan wyj�cia zmieniany przez symulacj� (mierniki, kontrolki)

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

int Console::iBits = 0; // zmienna statyczna - obiekt Console jest jednen wsp�lny
int Console::iMode = 0;
int Console::iConfig = 0;
TPoKeys55 *Console::PoKeys55[2] = {NULL, NULL};
TLPT *Console::LPT = NULL;
MWDComm *Console::MWD = NULL; // maciek001: obiekt dla MWD
int Console::iSwitch[8]; // bistabilne w kabinie, za��czane z [Shift], wy��czane bez
int Console::iButton[8]; // monostabilne w kabinie, za��czane podczas trzymania klawisza

Console::Console()
{
    PoKeys55[0] = PoKeys55[1] = NULL;
    for (int i = 0; i < 8; ++i)
    { // zerowanie prze��cznik�w
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
{ // za��czenie konsoli (np. nawi�zanie komunikacji)
    iSwitch[0] = iSwitch[1] = iSwitch[2] = iSwitch[3] = 0; // bity 0..127 - bez [Ctrl]
    iSwitch[4] = iSwitch[5] = iSwitch[6] = iSwitch[7] = 0; // bity 128..255 - z [Ctrl]
    switch (iMode)
    {
    case 1: // kontrolki klawiatury
    case 2: // kontrolki klawiatury
        iConfig = 0; // licznik u�ycia Scroll Lock
        break;
    case 3: // LPT
        LPT = new TLPT(); // otwarcie inpout32.dll
        if (LPT ? LPT->Connect(iConfig) : false)
        { // wys�a� 0?
            BitsUpdate(-1); // aktualizacjia stan�w, bo przy wczytywaniu mog�o by� nieaktywne
            WriteLog("Feedback Mode 3: InpOut32.dll OK");
        }
        else
        { // po��czenie nie wysz�o, ma by� NULL
            delete LPT;
            LPT = NULL;
        }
        break;
    case 4: // PoKeys
        PoKeys55[0] = new TPoKeys55();
        if (PoKeys55[0] ? PoKeys55[0]->Connect() : false)
        {
            WriteLog("Found " + PoKeys55[0]->Version());
            BitsUpdate(-1); // aktualizacjia stan�w, bo przy wczytywaniu mog�o by� nieaktywne
        }
        else
        { // po��czenie nie wysz�o, ma by� NULL
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
{ // wy��czenie informacji zwrotnych (reset pulpitu)
    BitsClear(-1);
    if ((iMode == 1) || (iMode == 2))
        if (iConfig & 1) // licznik u�ycia Scroll Lock
        { // bez sensu to jest, ale mi si� samo w��cza
            SetLedState(VK_SCROLL, true); // przyci�ni�ty
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
{ // ustawienie bit�w o podanej masce (mask) na wej�ciu (entry)
    if ((iBits & mask) != mask) // je�eli zmiana
    {
        int old = iBits; // poprzednie stany
        iBits |= mask;
        BitsUpdate(old ^ iBits); // 1 dla bit�w zmienionych
        if (iMode == 4)
            WriteLog("PoKeys::BitsSet: mask: " + std::to_string(mask) +
                     " iBits: " + std::to_string(iBits));
    }
};

void Console::BitsClear(int mask, int entry)
{ // zerowanie bit�w o podanej masce (mask) na wej�ciu (entry)
    if (iBits & mask) // je�eli zmiana
    {
        int old = iBits; // poprzednie stany
        iBits &= ~mask;
        BitsUpdate(old ^ iBits); // 1 dla bit�w zmienionych
    }
};

void Console::BitsUpdate(int mask)
{ // aktualizacja stanu interfejsu informacji zwrotnej; (mask) - zakres zmienianych bit�w
    switch (iMode)
    {
    case 1: // sterowanie �wiate�kami klawiatury: CA/SHP+opory
        if (mask & 3) // gdy SHP albo CA
            SetLedState(VK_CAPITAL, iBits & 3);
        if (mask & 4) // gdy jazda na oporach
        { // Scroll Lock ma jako� dziwnie... zmiana stanu na przeciwny
            SetLedState(VK_SCROLL, true); // przyci�ni�ty
            SetLedState(VK_SCROLL, false); // zwolniony
            ++iConfig; // licznik u�ycia Scroll Lock
        }
        break;
    case 2: // sterowanie �wiate�kami klawiatury: CA+SHP
        if (mask & 2) // gdy CA
            SetLedState(VK_CAPITAL, iBits & 2);
        if (mask & 1) // gdy SHP
        { // Scroll Lock ma jako� dziwnie... zmiana stanu na przeciwny
            SetLedState(VK_SCROLL, true); // przyci�ni�ty
            SetLedState(VK_SCROLL, false); // zwolniony
            ++iConfig; // licznik u�ycia Scroll Lock
        }
        break;
    case 3: // LPT Marcela z modyfikacj� (jazda na oporach zamiast brz�czyka)
        if (LPT)
            LPT->Out(iBits);
        break;
    case 4: // PoKeys55 wg Marcela - wersja druga z ko�ca 2012
        if (PoKeys55[0])
        { // pewnie trzeba b�dzie to dodatkowo buforowa� i oczekiwa� na potwierdzenie
            if (mask & 0x0001) // b0 gdy SHP
                PoKeys55[0]->Write(0x40, 23 - 1, (iBits & 0x0001) ? 1 : 0);
            if (mask & 0x0002) // b1 gdy zmieniony CA
                PoKeys55[0]->Write(0x40, 24 - 1, (iBits & 0x0002) ? 1 : 0);
            if (mask & 0x0004) // b2 gdy jazda na oporach
                PoKeys55[0]->Write(0x40, 32 - 1, (iBits & 0x0004) ? 1 : 0);
            if (mask & 0x0008) // b3 Lampka WS (wy��cznika szybkiego)
                PoKeys55[0]->Write(0x40, 25 - 1, (iBits & 0x0008) ? 1 : 0);
            if (mask & 0x0010) // b4 Lampka przeka�nika nadmiarowego silnik�w trakcyjnych
                PoKeys55[0]->Write(0x40, 27 - 1, (iBits & 0x0010) ? 1 : 0);
            if (mask & 0x0020) // b5 Lampka stycznik�w liniowych
                PoKeys55[0]->Write(0x40, 29 - 1, (iBits & 0x0020) ? 1 : 0);
            if (mask & 0x0040) // b6 Lampka po�lizgu
                PoKeys55[0]->Write(0x40, 30 - 1, (iBits & 0x0040) ? 1 : 0);
            if (mask & 0x0080) // b7 Lampka "przetwornicy"
                PoKeys55[0]->Write(0x40, 28 - 1, (iBits & 0x0080) ? 1 : 0);
            if (mask & 0x0100) // b8 Kontrolka przeka�nika nadmiarowego spr��arki
                PoKeys55[0]->Write(0x40, 33 - 1, (iBits & 0x0100) ? 1 : 0);
            if (mask & 0x0200) // b9 Kontrolka sygnalizacji wentylator�w i opor�w
                PoKeys55[0]->Write(0x40, 26 - 1, (iBits & 0x0200) ? 1 : 0);
            if (mask & 0x0400) // b10 Kontrolka wysokiego rozruchu
                PoKeys55[0]->Write(0x40, 31 - 1, (iBits & 0x0400) ? 1 : 0);
            if (mask & 0x0800) // b11 Kontrolka ogrzewania poci�gu
                PoKeys55[0]->Write(0x40, 34 - 1, (iBits & 0x0800) ? 1 : 0);
            if (mask & 0x1000) // b12 Ci�nienie w cylindrach do odbijania w haslerze
                PoKeys55[0]->Write(0x40, 52 - 1, (iBits & 0x1000) ? 1 : 0);
            if (mask & 0x2000) // b13 Pr�d na silnikach do odbijania w haslerze
                PoKeys55[0]->Write(0x40, 53 - 1, (iBits & 0x2000) ? 1 : 0);
            if (mask & 0x4000) // b14 Brz�czyk SHP lub CA
                PoKeys55[0]->Write(0x40, 16 - 1, (iBits & 0x4000) ? 1 : 0);
        }
        break;
    case 5: // maciek001: MWD lampki i kontrolki
        /* out3: ogrzewanie sk?adu, opory rozruchowe, poslizg, zaluzjewent, -, -, czuwak, shp
           out4: stycz.liniowe, pezekaznikr??nicobwpomoc, nadmiarprzetw, roznicowy obw. g?,
           nadmiarsilniki, wylszybki, zanikpr?duprzyje?dzienaoporach, nadmiarsprezarki
           out5: HASLER */
        if (mask & 0x0001)
            if (iBits & 1)
            {
                MWD->WriteDataBuff[4] |= 1 << 7; // SHP	HASLER te�
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
                MWD->WriteDataBuff[5] |= 1 << 5; // wy??cznik szybki
            else
                MWD->WriteDataBuff[5] &= ~(1 << 5);
        if (mask & 0x0010)
            if (iBits & 0x10)
                MWD->WriteDataBuff[5] |= 1 << 4; // nadmiarowy silnik?w trakcyjnych
            else
                MWD->WriteDataBuff[5] &= ~(1 << 4);
        if (mask & 0x0020)
            if (iBits & 0x20)
                MWD->WriteDataBuff[4] |= 1 << 0; // styczniki liniowe
            else
                MWD->WriteDataBuff[5] &= ~(1 << 0);
        if (mask & 0x0040)
            if (iBits & 0x40)
                MWD->WriteDataBuff[4] |= 1 << 2; // po?lizg
            else
                MWD->WriteDataBuff[4] &= ~(1 << 2);
        if (mask & 0x0080)
            if (iBits & 0x80)
                MWD->WriteDataBuff[5] |= 1 << 2; // (nadmiarowy) przetwornicy? ++
            else
                MWD->WriteDataBuff[5] &= ~(1 << 2);
        if (mask & 0x0100)
            if (iBits & 0x100)
                MWD->WriteDataBuff[5] |= 1 << 7; // nadmiarowy spr??arki
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
                MWD->WriteDataBuff[4] |= 1 << 0; // ogrzewanie poci?gu
            else
                MWD->WriteDataBuff[4] &= ~(1 << 0);
        if (mask & 0x1000)
            if (iBits & 0x1000)
                MWD->bHamowanie = true; // hasler: ci?nienie w hamulcach 	HASLER rysik 2
            else
                MWD->bHamowanie = false;
        if (mask & 0x2000)
            if (iBits & 0x2000)
                MWD->WriteDataBuff[6] |=
                    1 << 4; // hasler: pr?d "na" silnikach 		HASLER rysik 3
            else
                MWD->WriteDataBuff[6] &= ~(1 << 4);
        if (mask & 0x4000)
            if (iBits & 0x4000)
                MWD->WriteDataBuff[6] |= 1 << 7; // brz?czyk SHP/CA
            else
                MWD->WriteDataBuff[6] &= ~(1 << 7);
        // if(mask & 0x8000) if(iBits & 0x8000) MWD->WriteDataBuff[1] |= 1<<7; (puste)
        // else MWD->WriteDataBuff[0] &= ~(1<<7);
        break;
    }
};

bool Console::Pressed(int x)
{ // na razie tak - czyta si� tylko klawiatura
    return Global::bActive && (GetKeyState(x) < 0);
};

void Console::ValueSet(int x, double y)
{ // ustawienie warto�ci (y) na kanale analogowym (x)
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
                y = y / Global::fCalibrateOutMax[x]; // sprowadzenie do <0,1> je�li podana
                // maksymalna warto��
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
    if (iMode == 5 && MWD) // pwm-y i pr�dko��
    {
        unsigned int iliczba;
        if (x == 0)
        {
            iliczba = (unsigned int)floor((y / (Global::fMWDzg[0] * 10) * Global::fMWDzg[1]) +
                                          0.5); // zbiornik g??wny
            MWD->WriteDataBuff[12] = (unsigned char)(iliczba >> 8);
            MWD->WriteDataBuff[11] = (unsigned char)iliczba;
        }
        else if (x == 1)
        {
            iliczba = (unsigned int)floor((y / (Global::fMWDpg[0] * 10) * Global::fMWDpg[1]) +
                                          0.5); // przew?d g??wny
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
            MWD->WriteDataBuff[0] = (unsigned char)floor(y); // pr�dko��
    }
};

void Console::Update()
{ // funkcja powinna by� wywo�ywana regularnie, np. raz w ka�dej ramce ekranowej
    if (iMode == 4)
        if (PoKeys55[0])
            if (PoKeys55[0]->Update((Global::iPause & 8) > 0))
            { // wykrycie przestawionych prze��cznik�w?
                Global::iPause &= ~8;
            }
            else
            { // b��d komunikacji - zapauzowa� symulacj�?
                if (!(Global::iPause & 8)) // je�li jeszcze nie oflagowana
                    Global::iTextMode = VK_F1; // pokazanie czasu/pauzy
                Global::iPause |= 8; // tak???
                PoKeys55[0]->Connect(); // pr�ba ponownego pod��czenia
            }
    if (iMode == 5) // Obs?uga MWD OK
    {
        MWD->Run();
    }
};

float Console::AnalogGet(int x)
{ // pobranie warto�ci analogowej
    if (iMode == 4)
        if (PoKeys55[0])
            return PoKeys55[0]->fAnalog[x];
    return -1.0;
};

float Console::AnalogCalibrateGet(int x)
{ // pobranie i kalibracja warto�ci analogowej, je�li nie ma PoKeys zwraca NULL
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
    return -1.0; // odci�cie
};

unsigned char Console::DigitalGet(int x)
{ // pobranie warto�ci cyfrowej
    if (iMode == 4)
        if (PoKeys55[0])
            return PoKeys55[0]->iInputs[x];
    return 0;
};

void Console::OnKeyDown(int k)
{ // naci�ni�cie klawisza z powoduje wy��czenie, a
    if (k & 0x10000) // je�li [Shift]
    { // ustawienie bitu w tabeli prze��cznik�w bistabilnych
        if (k & 0x20000) // je�li [Ctrl], to zestaw dodatkowy
            iSwitch[4 + (char(k) >> 5)] |= 1 << (k & 31); // za��cz bistabliny dodatkowy
        else
        { // z [Shift] w��czenie bitu bistabilnego i dodatkowego monostabilnego
            iSwitch[char(k) >> 5] |= 1 << (k & 31); // za��cz bistabliny podstawowy
            iButton[4 + (char(k) >> 5)] |= (1 << (k & 31)); // za��cz monostabilny dodatkowy
        }
    }
    else
    { // zerowanie bitu w tabeli prze��cznik�w bistabilnych
        if (k & 0x20000) // je�li [Ctrl], to zestaw dodatkowy
            iSwitch[4 + (char(k) >> 5)] &= ~(1 << (k & 31)); // wy��cz bistabilny dodatkowy
        else
        {
            iSwitch[char(k) >> 5] &= ~(1 << (k & 31)); // wy��cz bistabilny podstawowy
            iButton[char(k) >> 5] |= 1 << (k & 31); // za��cz monostabilny podstawowy
        }
    }
};
void Console::OnKeyUp(int k)
{ // puszczenie klawisza w zasadzie nie ma znaczenia dla iSwitch, ale zeruje iButton
    if ((k & 0x20000) == 0) // monostabilne tylko bez [Ctrl]
        if (k & 0x10000) // je�li [Shift]
            iButton[4 + (char(k) >> 5)] &= ~(1 << (k & 31)); // wy��cz monostabilny dodatkowy
        else
            iButton[char(k) >> 5] &= ~(1 << (k & 31)); // wy��cz monostabilny podstawowy
};
int Console::KeyDownConvert(int k)
{
    return int(ktTable[k & 0x3FF].iDown);
};
int Console::KeyUpConvert(int k)
{
    return int(ktTable[k & 0x3FF].iUp);
};
