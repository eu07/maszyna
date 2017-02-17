/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#ifndef ConsoleH
#define ConsoleH
//---------------------------------------------------------------------------
class TConsoleDevice; // urządzenie podłączalne za pomocą DLL
class TPoKeys55;
class TLPT;
class TMWDComm;        // maciek001: dodana obsluga portu COM

// klasy konwersji znaków wprowadzanych z klawiatury
class TKeyTrans
{ // przekodowanie kodu naciśnięcia i zwolnienia klawisza
  public:
    short int iDown, iUp;
};

class Console
{ // Ra: klasa statyczna gromadząca sygnały sterujące oraz informacje zwrotne
    // Ra: stan wejścia zmieniany klawiaturą albo dedykowanym urządzeniem
    // Ra: stan wyjścia zmieniany przez symulację (mierniki, kontrolki)
  private:
    static int iMode; // tryb pracy
    static int iConfig; // dodatkowa informacja o sprzęcie (np. numer LPT)
    static int iBits; // podstawowy zestaw lampek
    static TPoKeys55 *PoKeys55[2]; // może ich być kilka
    static TLPT *LPT;
    static TMWDComm *MWDComm; // maciek001: na potrzeby MWD
    static void BitsUpdate(int mask);
    // zmienne dla trybu "jednokabinowego", potrzebne do współpracy z pulpitem (PoKeys)
    // używając klawiatury, każdy pojazd powinien mieć własny stan przełączników
    // bazowym sterowaniem jest wirtualny strumień klawiatury
    // przy zmianie kabiny z PoKeys, do kabiny są wysyłane stany tych przycisków
    static int iSwitch[8]; // bistabilne w kabinie, załączane z [Shift], wyłączane bez
    static int iButton[8]; // monostabilne w kabinie, załączane podczas trzymania klawisza
    static TKeyTrans ktTable[4 * 256]; // tabela wczesnej konwersji klawiatury
  public:
    Console();
    ~Console();
    static void ModeSet(int m, int h = 0);
    static void BitsSet(int mask, int entry = 0);
    static void BitsClear(int mask, int entry = 0);
    static int On();
    static void Off();
    static bool Pressed(int x);
    static void ValueSet(int x, double y);
    static void Update();
    static float AnalogGet(int x);
	static float AnalogCalibrateGet(int x);
    static unsigned char DigitalGet(int x);
    static void OnKeyDown(int k);
    static void OnKeyUp(int k);
    static int KeyDownConvert(int k);
    static int KeyUpConvert(int k);
};

#endif
