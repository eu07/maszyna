/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

/*
    Program obsługi portu COM i innych na potrzeby sterownika MWDevice
        (oraz innych wykorzystujących komunikację przez port COM)
    dla Symulatora Pojazdów Szynowych MaSzyna
    author: Maciej Witek 2016
        Autor nie ponosi odpowiedzialności za niewłaciwe używanie lub działanie programu!
*/

#ifndef MWDH
#define MWDH
//---------------------------------------------------------------------------

typedef unsigned char BYTE;
typedef unsigned long DWORD;

class MWDComm
{
  private:
    int MWDTime; //
    char lastStateData[6], maskData[6], maskSwitch[6], bitSwitch[6];
    int bocznik, nastawnik, kierunek;
    char bnkMask;

    bool ReadData(); // BYTE *pReadDataBuff);
    bool SendData(); // BYTE *pWriteDataBuff);
    void CheckData(); // sprawdzanie zmian wejść i kontrola mazaków HASLERA
    void KeyBoard(int key, bool s);

    bool bRysik1H;
    bool bRysik1L;
    bool bRysik2H;
    bool bRysik2L;

  public:
    bool Open(); // Otwarcie portu
    bool Close(); // Zamknięcie portu
    bool Run(); // Obsługa portu
    bool GetMWDState(); // sprawdź czy port jest otwarty, 0 zamknięty, 1 otwarty

    // zmienne do rysików HASLERA
    bool bSHPstate;
    bool bPrzejazdSHP;
    bool bKabina1;
    bool bKabina2;
    bool bHamowanie;
    bool bCzuwak;

    float fAnalog[4]; // trzymanie danych z wejść analogowych

    BYTE ReadDataBuff[17]; // bufory danych
    BYTE WriteDataBuff[31];

    MWDComm(); // konstruktor
    ~MWDComm(); // destruktor
};
#endif

/*
        INFO - zmiany dokonane w innych plikach niezbędne do prawidłowego działania:

        Console.cpp:
         Console::AnalogCalibrateGet	- obsługa kranów hamulców
         Console::Update		- wywoływanie obsługi MWD
         Console::ValueSet		- obsługa manometrów, mierników WN (PWM-y)
         Console::BitsUpdate		- ustawiania lampek
         Console::Off			- zamykanie portu COM
         Console::On			- otwieranie portu COM
         Console::~Console		- usuwanie MWD (jest też w Console OFF)

         MWDComm * Console::MWD = NULL; - luzem, obiekt i wskaźnik(?)
         dodatkowo zmieniłem int na long int dla BitSet i BitClear oraz iBits

        Train.cpp:
         if (Global::iFeedbackMode == 5) - pobieranie prędkości, manometrów i mierników WN
         if (ggBrakeCtrl.SubModel)       - możliwość sterowania hamulcem zespolonym
         if (ggLocalBrake.SubModel) 	 - możliwość sterowania hamulcem lokomotywy

        Globals.h:
         dodano zmienne dla MWD
        Globals.cpp:
         dodano inicjalizaję zmiennych i odczyt z ini ustawień

        Wpisy do pliku eu07.ini

                //maciek001 MWD
                comportname COM3	// wybór portu COM
                mwdbaudrate 500000

                mwdbreakenable yes	// czy załączyć sterowanie hamulcami? blokuje klawiature
                mwdbreak 1 255 0 255	// hamulec zespolony
                mwdbreak 2 255 0 255	// hamulec lokomotywy

                mwdzbiornikglowny 	0.82 255
                mwdprzewodglowny 	0.7 255
                mwdcylinderhamulcowy 	0.43 255
                mwdwoltomierzwn 	4000 255
                mwdamperomierzwn 	800 255
*/
