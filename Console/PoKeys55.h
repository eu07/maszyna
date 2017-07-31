/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
#pragma once

#include <string>
//---------------------------------------------------------------------------
class TPoKeys55
{ // komunikacja z PoKeys bez określania przeznaczenia pinów
    unsigned char cRequest; // numer żądania do sprawdzania odpowiedzi
    unsigned char OutputBuffer[65]; // Allocate a memory buffer equal to our endpoint size + 1
    unsigned char InputBuffer[65]; // Allocate a memory buffer equal to our endpoint size + 1
    int iPWM[8]; // 0-5:wyjścia PWM,6:analogowe,7:częstotliwośc PWM
	int iPWMbits;
    int iLastCommand;
    int iFaza;
    int iRepeats; // liczba powtórzeń
    bool bNoError; // zerowany po przepełnieniu licznika powtórzeń, ustawiany po udanej operacji
  public:
    float fAnalog[7]; // wejścia analogowe, stan <0.0,1.0>
    int iInputs[8];
    TPoKeys55();
    ~TPoKeys55();
    bool Connect();
    void Close();
    bool Write(unsigned char c, unsigned char b3, unsigned char b4 = 0, unsigned char b5 = 0);
	bool Read();
    bool ReadLoop(int i);
    std::string Version();
    bool PWM(int x, float y);
	bool Update(bool pause);
};
//---------------------------------------------------------------------------
