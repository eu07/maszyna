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

#define BYTETOWRITE 31  		// ilość bajtów przesyłanych z MaSzyny
#define BYTETOREAD  16  		// ilość bajtów przesyłanych do MaSzyny

typedef unsigned char BYTE;
typedef unsigned long DWORD;

class TMWDComm
{
private:
	int MWDTime;	//
	char lastStateData[6], maskData[6], maskSwitch[6], bitSwitch[6];
	int bocznik, nastawnik, kierunek;
	char bnkMask;

	bool ReadData();	//BYTE *pReadDataBuff);
	bool SendData();	//BYTE *pWriteDataBuff);
	void CheckData();	//sprawdzanie zmian wejść i kontrola mazaków HASLERA
	void KeyBoard(int key, bool s);

	//void CheckData2();

	bool bRysik1H;
	bool bRysik1L;
	bool bRysik2H;
	bool bRysik2L;

public:
	bool Open();		// Otwarcie portu
	bool Close();		// Zamknięcie portu
	bool Run();			// Obsługa portu
	bool GetMWDState(); // sprawdź czy port jest otwarty, 0 zamknięty, 1 otwarty

	// zmienne do rysików HASLERA
	bool bSHPstate;
	bool bPrzejazdSHP;
	bool bKabina1;
	bool bKabina2;
	bool bHamowanie;
	bool bCzuwak;
	
	unsigned int uiAnalog[4]; 		// trzymanie danych z wejść analogowych

	BYTE ReadDataBuff[BYTETOREAD+2]; //17]; // bufory danych
	BYTE WriteDataBuff[BYTETOWRITE+2]; //31];

    TMWDComm(); // konstruktor
    ~TMWDComm(); // destruktor
};
#endif

/*
INFO - wpisy do eu07.ini:

mwdmasterenable yes 		// włącz MWD (master MWD Enable)
mwddebugenable yes 		// włącz logowanie 
mwddebugmode 4 			// tryb debugowania (które logi)

mwdcomportname COM3 		// nazwa portu
mwdbaudrate 500000 		// prędkość transmisji

mwdinputenable yes 		// włącz wejścia (przyciski, przełączniki)
mwdbreakenable yes 		// włącz hamulce (wejścia analogowe)

mwdmainbreakconfig 0 1023 	// konfiguracja kranu zespolonego -> min, max (położenie kranu - odczyt z ADC)
mwdlocbreakconfig 0 1023 	// konfiguracja kranu maszynisty  -> min, max (położenie kranu - odczyt z ADC)
mwdanalogin2config 0 1023
mwdanalogin2config 0 1023

mwdmaintankpress 0.9 1023 	// max ciśnienie w zbiorniku głownym i rozdzielczość
mwdmainpipepress 0.7 1023 	// max ciśnienie w przewodzie głównym i rozdzielczość
mwdbreakpress 0.5 1023  	// max ciśnienie w cylindrach hamulcowych i rozdzielczość

mwdhivoltmeter 4000 1023 	// max napięcie na woltomierzu WN
mwdhiampmeter 800 1023  	// max prąd amperomierza WN

mwddivider 5 			// dzielnik - czym większy tym rzadziej czyta diwajs
*/
