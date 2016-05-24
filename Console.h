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
class TConsoleDevice; // urz¹dzenie pod³¹czalne za pomoc¹ DLL
class TPoKeys55;
class TLPT;

// klasy konwersji znaków wprowadzanych z klawiatury
class TKeyTrans
{ // przekodowanie kodu naciœniêcia i zwolnienia klawisza
  public:
    short int iDown, iUp;
};

class Console
{ // Ra: klasa statyczna gromadz¹ca sygna³y steruj¹ce oraz informacje zwrotne
    // Ra: stan wejœcia zmieniany klawiatur¹ albo dedykowanym urz¹dzeniem
    // Ra: stan wyjœcia zmieniany przez symulacjê (mierniki, kontrolki)
  private:
    static int iMode; // tryb pracy
    static int iConfig; // dodatkowa informacja o sprzêcie (np. numer LPT)
    static int iBits; // podstawowy zestaw lampek
    static TPoKeys55 *PoKeys55[2]; // mo¿e ich byæ kilka
    static TLPT *LPT;
    static void BitsUpdate(int mask);
    // zmienne dla trybu "jednokabinowego", potrzebne do wspó³pracy z pulpitem (PoKeys)
    // u¿ywaj¹c klawiatury, ka¿dy pojazd powinien mieæ w³asny stan prze³¹czników
    // bazowym sterowaniem jest wirtualny strumieñ klawiatury
    // przy zmianie kabiny z PoKeys, do kabiny s¹ wysy³ane stany tych przycisków
    static int iSwitch[8]; // bistabilne w kabinie, za³¹czane z [Shift], wy³¹czane bez
    static int iButton[8]; // monostabilne w kabinie, za³¹czane podczas trzymania klawisza
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
