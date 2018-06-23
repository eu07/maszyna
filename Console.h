/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
#pragma once

#include "Globals.h"

class TConsoleDevice; // urządzenie podłączalne za pomocą DLL
class TPoKeys55;
class TLPT;

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
public:
	Console();
	~Console();
	static void modeSet(int m, int h = 0);
	static void bitsSet(int mask, int entry = 0);
	static void bitsClear(int mask, int entry = 0);
	static int on();
	static void off();

	inline static bool pressed(int x)
	{ // na razie tak - czyta się tylko klawiatura
		if (glfwGetKey(Global.window, x) == GLFW_TRUE) {
			return true;
		} else {
			return false;
		}
	};

	static void valueSet(int x, double y);
	static void update();
	static float analogGet(int x);
	static float analogCalibrateGet(int x);
	static unsigned char digitalGet(int x);
	static void onKeyDown(int k);
	static void onKeyUp(int k);
	static int keyDownConvert(int k);
	static int keyUpConvert(int k);

private:
    static void bitsUpdate(int mask);

	static int iMode; // tryb pracy
	static int iConfig; // dodatkowa informacja o sprzęcie (np. numer LPT)
	static int iBits; // podstawowy zestaw lampek
	static TPoKeys55* poKeys55[2]; // może ich być kilka
	static TLPT* lpt;
	// zmienne dla trybu "jednokabinowego", potrzebne do współpracy z pulpitem (PoKeys)
	// używając klawiatury, każdy pojazd powinien mieć własny stan przełączników
	// bazowym sterowaniem jest wirtualny strumień klawiatury
	// przy zmianie kabiny z PoKeys, do kabiny są wysyłane stany tych przycisków
	static int iSwitch[8]; // bistabilne w kabinie, załączane z [Shift], wyłączane bez
	static int iButton[8]; // monostabilne w kabinie, załączane podczas trzymania klawisza
	static TKeyTrans ktTable[4 * 256]; // tabela wczesnej konwersji klawiatury
};
