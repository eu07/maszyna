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
#include "stdafx.h"
#include "MWD.h"
#include "Globals.h"
#include "Logs.h"
#include "World.h"

#include <windows.h>

HANDLE hComm;

TMWDComm::TMWDComm() // konstruktor
{
	MWDTime = 0;
	bSHPstate = false;
	bPrzejazdSHP = false;
	bKabina1 = true;	// pasuje wyciągnąć dane na temat kabiny 
	bKabina2 = false;	// i ustawiać te dwa parametry!
	bHamowanie = false;
	bCzuwak = false;

	bRysik1H = false;
	bRysik1L = false;
	bRysik2H = false;
	bRysik2L = false;

	bocznik = 0;
	nastawnik = 0;
	kierunek = 0;
	bnkMask = 0;

	int i = 6;

	while (i)
	{
		i--;
		lastStateData[i] = 0;
		maskData[i] = 0;
		maskSwitch[i] = 0;
		bitSwitch[i] = 0;
	}

	i = 0;
	while (i<BYTETOWRITE)
	{
		if (i<BYTETOREAD)ReadDataBuff[i] = 0;
		WriteDataBuff[i] = 0;
		i++;
	}
	i = 0;
	while (i<6)
	{
		lastStateData[i] = 0;
		maskSwitch[i] = 0;
		bitSwitch[i] = 0;
		i++;
	}
}

TMWDComm::~TMWDComm() // destruktor
{
    Close();
}

bool TMWDComm::Open() // otwieranie portu COM
{
    LPCSTR portId = Global::sMWDPortId.c_str();
    hComm = CreateFile(portId, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL, NULL);

    if (hComm == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            WriteLog("PortCOM ERROR: serial port does not exist"); // serial port does not exist.
                                                                   // Inform user.
        }
        WriteLog("PortCOM ERROR! not open!");
        // some other error occurred. Inform user.
        return FALSE;
    }

    DCB CommDCB;
    CommDCB.DCBlength = sizeof(DCB);
    GetCommState(hComm, &CommDCB);

    CommDCB.BaudRate = Global::iMWDBaudrate;
    CommDCB.fBinary = TRUE;
    CommDCB.fParity = FALSE;
    CommDCB.fOutxCtsFlow = FALSE; // No CTS output flow control
    CommDCB.fOutxDsrFlow = FALSE; // No DSR output flow control
    CommDCB.fDtrControl = FALSE; // DTR flow control type

    CommDCB.fDsrSensitivity = FALSE; // DSR sensitivity
    CommDCB.fTXContinueOnXoff = FALSE; // XOFF continues Tx
    CommDCB.fOutX = FALSE; // No XON/XOFF out flow control
    CommDCB.fInX = FALSE; // No XON/XOFF in flow control
    CommDCB.fErrorChar = FALSE; // Disable error replacement
    CommDCB.fNull = FALSE; // Disable null stripping
    CommDCB.fRtsControl = RTS_CONTROL_DISABLE;

    CommDCB.fAbortOnError = FALSE;

    CommDCB.ByteSize = 8;
    CommDCB.Parity = NOPARITY;
    CommDCB.StopBits = ONESTOPBIT;

    // konfiguracja portu
    if (!SetCommState(hComm, &CommDCB))
    {
        // dwError = GetLastError ();
        WriteLog("Unable to configure the serial port!");
        return FALSE;
    }
    WriteLog("PortCOM OPEN and CONFIG!");
    return TRUE;
}

bool TMWDComm::Close() // zamykanie portu COM
{
	Global::bMWDmasterEnable = false;              // główne włączenie portu!
	Global::bMWDInputEnable = false;               // włącz wejścia
	Global::bMWDBreakEnable = false;               // włącz wejścia analogowe

    WriteLog("COM Port is closing...");
    int i = 0;
    while (i < BYTETOWRITE) // zerowanie danych...
    {
        WriteDataBuff[i] = 0;
        i++;
    }
    Sleep(100);
    SendData(); // wysyłanie do pulpitu: zatrzymanie haslera i zgaszenie lampek
    Sleep(700);
    CloseHandle(hComm);
    WriteLog("COM is close!");
    return TRUE;
}

bool TMWDComm::GetMWDState() // sprawdzanie otwarcia portu COM
{
    if (hComm > 0)
        return 1;
    else
        return 0;
}

bool TMWDComm::ReadData() // odbieranie danych + odczyta danych analogowych i zapis do zmiennych
{
	DWORD bytes_read;
	ReadFile(hComm, &ReadDataBuff[0], BYTETOREAD, &bytes_read, NULL);
	if (Global::bMWDdebugEnable && Global::iMWDDebugMode == 128) WriteLog("Data receive. Checking data...");
	if (Global::bMWDBreakEnable)
	{
		uiAnalog[0] = (ReadDataBuff[9] << 8) + ReadDataBuff[8];
		uiAnalog[1] = (ReadDataBuff[11] << 8) + ReadDataBuff[10];
		uiAnalog[2] = (ReadDataBuff[13] << 8) + ReadDataBuff[12];
		uiAnalog[3] = (ReadDataBuff[15] << 8) + ReadDataBuff[14];
		if (Global::bMWDdebugEnable && (Global::iMWDDebugMode & 1))
		{
			WriteLog("Main Break = " + to_string(uiAnalog[0]));
			WriteLog("Locomotiv Break = " + to_string(uiAnalog[1]));
		}
		if (Global::bMWDdebugEnable && (Global::iMWDDebugMode & 2))
		{
			WriteLog("Analog input 1 = " + to_string(uiAnalog[2]));
			WriteLog("Analog imput 2 = " + to_string(uiAnalog[3]));
		}
	}
	if (Global::bMWDInputEnable) CheckData();
	return TRUE;
}

bool TMWDComm::SendData() // wysyłanie danych
{
    DWORD bytes_write;
    DWORD fdwEvtMask;

    WriteFile(hComm, &WriteDataBuff[0], BYTETOWRITE, &bytes_write, NULL);

    return TRUE;
}

bool TMWDComm::Run() // wywoływanie obsługi MWD + generacja większego opóźnienia
{
	if (GetMWDState())
	{
		MWDTime++;
		if (!(MWDTime % Global::iMWDdivider))
		{
			MWDTime = 0;
			if (Global::bMWDdebugEnable && Global::iMWDDebugMode == 128) WriteLog("Sending data...");
			SendData();
			if (Global::bMWDdebugEnable && Global::iMWDDebugMode == 128) WriteLog(" complet!\nReceiving data...");
			ReadData();
			if (Global::bMWDdebugEnable && Global::iMWDDebugMode == 128) WriteLog(" complet!");
			return 1;
		}
	}
	else
	{
		WriteLog("Port COM: connection ERROR!");
		Close();
		// może spróbować się połączyć znowu?
		return 0;
	}
	return 1;
}

void TMWDComm::CheckData() // sprawdzanie wejść cyfrowych i odpowiednie sterowanie maszyną
{
	int i = 0;
	while (i < 6)
	{
		maskData[i] = ReadDataBuff[i] ^ lastStateData[i];
		lastStateData[i] = ReadDataBuff[i];
		i++;
	}
	/*
				Rozpiska portów!
				Port0: 	0 	NC			odblok. przek. sprężarki i wentyl. oporów
						1 	M			wyłącznik wył. szybkiego
						2 	Shift+M		impuls załączający wył. szybki
						3 	N			odblok. przekaźników nadmiarowych
	   i różnicowego obwodu głównegoMMm
						4 	NC			rezerwa
						5 	Ctrl+N		odblok. przek. nadmiarowych
	   przetwornicy, ogrzewania pociągu i różnicowych obw. pomocniczych
						6 	L			wył. styczników liniowych
						7 	SPACE		kasowanie czuwaka

				Port1: 	0 	NC
						1 	(Shift)	X	przetwornica
						2 	(Shift)	C	sprężarka
						3 	S			piasecznice
						4 	(Shift)	H	ogrzewanie składu
						5 				przel. hamowania	Shift+B
	   pspbpwy Ctrl+B pospieszny B towarowy
						6 				przel. hamowania
						7 	(Shift)	F	rozruch w/n

				Port2: 	0 	(Shift) P	pantograf przedni
						1 	(Shift)	O	pantograf tylni
						2 	ENTER		przyhamowanie przy poślizgu
						3 	()		przyciemnienie świateł
						4 	()		przyciemnienie świateł
						5 	NUM6		odluźniacz
						6 	a			syrena lok W
						7	A			syrena lok N

				Port3: 	0 	Shift+J		bateria
						1
						2
						3
						4
						5
						6
						7
		*/

		/*	po przełączeniu bistabilnego najpierw wciskamy klawisz i przy następnym
			wejściu w pętlę MWD puszczamy bo inaczej nie działa
		*/

		// wciskanie przycisków klawiatury
		/*PORT0*/
	if (maskData[0] & 0x02) if (lastStateData[0] & 0x02)
		KeyBoard('M', 1);	// wyłączenie wyłącznika szybkiego
	else KeyBoard('M', 0);	// monostabilny
	if (maskData[0] & 0x04) if (lastStateData[0] & 0x04) // impuls załączający wyłącznik szybki
	{
		KeyBoard(0x10, 1);	// monostabilny
		KeyBoard('M', 1);
	}
	else
	{
		KeyBoard('M', 0);
		KeyBoard(0x10, 0);
	}
	if (maskData[0] & 0x08) if (lastStateData[0] & 0x08)
		KeyBoard('N', 1);	// odblok nadmiarowego silników trakcyjnych
	else KeyBoard('N', 0);	// monostabilny
	if (maskData[0] & 0x20) if (lastStateData[0] & 0x20)
	{							// odblok nadmiarowego przetwornicy, ogrzewania poc.
		KeyBoard(0x11, 1);      // różnicowego obwodów pomocniczych
		KeyBoard('N', 1);         // monostabilny
	}
	else
	{
		KeyBoard('N', 0);
		KeyBoard(0x11, 0);
	}
	if (maskData[0] & 0x40) if (lastStateData[0] & 0x40) KeyBoard('L', 1);	// wył. styczników liniowych
	else						KeyBoard('L', 0);	// monostabilny
	if (maskData[0] & 0x80) if (lastStateData[0] & 0x80) KeyBoard(0x20, 1);	// kasowanie czuwaka/SHP
	else 						KeyBoard(0x20, 0);	// kasowanie czuwaka/SHP

	/*PORT1*/

	// puszczanie przycisku bistabilnego klawiatury
	if (maskSwitch[1] & 0x02) 
	{
		if (bitSwitch[1] & 0x02) 
		{
			KeyBoard('X', 0);
			KeyBoard(0x10, 0);
		}
		else KeyBoard('X', 0);
			maskSwitch[1] &= ~0x02;
	}
	if (maskSwitch[1] & 0x04) {
		if (bitSwitch[1] & 0x04) {
			KeyBoard('C', 0);
			KeyBoard(0x10, 0);
		}
		else KeyBoard('C', 0);
		maskSwitch[1] &= ~0x04;
	}
	if (maskSwitch[1] & 0x10) {
		if (bitSwitch[1] & 0x10) {
			KeyBoard('H', 0);
			KeyBoard(0x10, 0);
		}
		else KeyBoard('H', 0);
		maskSwitch[1] &= ~0x10;
	}
	if (maskSwitch[1] & 0x20 || maskSwitch[1] & 0x40) {
		if (maskSwitch[1] & 0x20) KeyBoard(0x10, 0);
		if (maskSwitch[1] & 0x40) KeyBoard(0x11, 0);
		KeyBoard('B', 0);
		maskSwitch[1] &= ~0x60;
	}
	if (maskSwitch[1] & 0x80) {
		if (bitSwitch[1] & 0x80) {
			KeyBoard('F', 0);
			KeyBoard(0x10, 0);
		}
		else KeyBoard('F', 0);
		maskSwitch[1] &= ~0x80;
	}

	// przetwornica
	if (maskData[1] & 0x02) if (lastStateData[1] & 0x02)
	{
		KeyBoard(0x10, 1);	// bistabilny
		KeyBoard('X', 1);
		maskSwitch[1] |= 0x02;
		bitSwitch[1] |= 0x02;
	}
	else
	{
		maskSwitch[1] |= 0x02;
		bitSwitch[1] &= ~0x02;
		KeyBoard('X', 1);
	}
	// sprężarka
	if (maskData[1] & 0x04) if (lastStateData[1] & 0x04)
	{
		KeyBoard(0x10, 1);	// bistabilny
		KeyBoard('C', 1);
		maskSwitch[1] |= 0x04;
		bitSwitch[1] |= 0x04;
	}
	else
	{
		maskSwitch[1] |= 0x04;
		bitSwitch[1] &= ~0x04;
		KeyBoard('C', 1);
	}
	// piasecznica
	if (maskData[1] & 0x08) if (lastStateData[1] & 0x08)
		KeyBoard('S', 1);
	else
		KeyBoard('S', 0);	// monostabilny
	// ogrzewanie składu
	if (maskData[1] & 0x10) if (lastStateData[1] & 0x10)
	{
		KeyBoard(0x11, 1);	// bistabilny
		KeyBoard('H', 1);
		maskSwitch[1] |= 0x10;
		bitSwitch[1] |= 0x10;
	}
	else
	{
		maskSwitch[1] |= 0x10;
		bitSwitch[1] &= ~0x10;
		KeyBoard('H', 1);
	}
	// przełącznik hamowania
	if (maskData[1] & 0x20 || maskData[1] & 0x40)
	{
		if (lastStateData[1] & 0x20)
		{	// Shift+B
			KeyBoard(0x10, 1);
			maskSwitch[1] |= 0x20;
		}
		else if (lastStateData[1] & 0x40)
		{	// Ctrl+B
			KeyBoard(0x11, 1);
			maskSwitch[1] |= 0x40;
		}
		KeyBoard('B', 1);
	}
	// rozruch wysoki/niski
	if (maskData[1] & 0x80) if (lastStateData[1] & 0x80)
	{
		KeyBoard(0x10, 1);	// bistabilny
		KeyBoard('F', 1);
		maskSwitch[1] |= 0x80;
		bitSwitch[1] |= 0x80;
	}
	else
	{
		maskSwitch[1] |= 0x80;
		bitSwitch[1] &= ~0x80;
		KeyBoard('F', 1);
	}


	//PORT2
	if (maskSwitch[2] & 0x01)
	{
		if (bitSwitch[2] & 0x01)
		{
			KeyBoard('P', 0);
			KeyBoard(0x10, 0);
		}
		else KeyBoard('P', 0);
		maskSwitch[2] &= ~0x01;
	}
	if (maskSwitch[2] & 0x02)
	{
		if (bitSwitch[2] & 0x02)
		{
			KeyBoard('O', 0);
			KeyBoard(0x10, 0);
		}
		else KeyBoard('O', 0);
		maskSwitch[2] &= ~0x02;
	}

	// pantograf przedni
	if (maskData[2] & 0x01) if (lastStateData[2] & 0x01)
	{
		KeyBoard(0x10, 1);	// bistabilny
		KeyBoard('P', 1);
		maskSwitch[2] |= 0x01;
		bitSwitch[2] |= 0x01;
	}
	else
	{
		maskSwitch[2] |= 0x01;
		bitSwitch[2] &= ~0x01;
		KeyBoard('P', 1);
	}
	// pantograf tylni
	if (maskData[2] & 0x02) if (lastStateData[2] & 0x02)
	{
		KeyBoard(0x10, 1);	// bistabilny
		KeyBoard('O', 1);
		maskSwitch[2] |= 0x02;
		bitSwitch[2] |= 0x02;
	}
	else
	{
		maskSwitch[2] |= 0x02;
		bitSwitch[2] &= ~0x02;
		KeyBoard('O', 1);
	}
	// przyhamowanie przy poślizgu
	if (maskData[2] & 0x04) if (lastStateData[2] & 0x04) {
		KeyBoard(0x10, 1);	// monostabilny
		KeyBoard(0x0D, 1);
	}
	else
	{
		KeyBoard(0x0D, 0);
		KeyBoard(0x10, 0);
	}
	/*if(maskData[2] & 0x08) if (lastStateData[2] & 0x08){           // przyciemnienie świateł
	KeyBoard(' ',0);	// bistabilny
	KeyBoard(0x10,1);
	KeyBoard(' ',1);
	}else{
	KeyBoard(' ',0);
	KeyBoard(0x10,0);
	KeyBoard(' ',1);
	}
	if(maskData[2] & 0x10) if (lastStateData[2] & 0x10)  {       // przyciemnienie świateł
	KeyBoard(' ',0);	// bistabilny
	KeyBoard(0x11,1);
	KeyBoard(' ',1);
	}else{
	KeyBoard(' ',0);
	KeyBoard(0x11,0);
	KeyBoard(' ',1);
	}*/
	// odluźniacz
	if (maskData[2] & 0x20) if (lastStateData[2] & 0x20)
		KeyBoard(0x66, 1);
	else
		KeyBoard(0x66, 0);	// monostabilny
	// syrena wysoka
	if (maskData[2] & 0x40) if (lastStateData[2] & 0x40)
	{
		KeyBoard(0x10, 1);	// monostabilny
		KeyBoard('A', 1);
	}
	else
	{
		KeyBoard('A', 0);
		KeyBoard(0x10, 0);
	}
	if (maskData[2] & 0x80) if (lastStateData[2] & 0x80)
		KeyBoard('A', 1);	// syrena niska
	else
		KeyBoard('A', 0);	// monostabilny


	//PORT3

	if (maskSwitch[3] & 0x01)
	{
		if (bitSwitch[3] & 0x01)
		{
			KeyBoard('J', 0);
			KeyBoard(0x10, 0);
		}
		else KeyBoard('J', 0);
		maskSwitch[3] &= ~0x01;
	}
	if (maskSwitch[3] & 0x02)
	{
		if (bitSwitch[3] & 0x02)
		{
			KeyBoard('Y', 0);
			KeyBoard(0x10, 0);
		}
		else KeyBoard('Y', 0);
		maskSwitch[3] &= ~0x02;
	}
	if (maskSwitch[3] & 0x04)
	{
		if (bitSwitch[3] & 0x04)
		{
			KeyBoard('U', 0);
			KeyBoard(0x10, 0);
		}
		else KeyBoard('U', 0);
		maskSwitch[3] &= ~0x04;
	}
	if (maskSwitch[3] & 0x08)
	{
		if (bitSwitch[3] & 0x08)
		{
			KeyBoard('I', 0);
			KeyBoard(0x10, 0);
		}
		else KeyBoard('I', 0);
		maskSwitch[3] &= ~0x08;
	}


	// bateria
	if (maskData[3] & 0x01) if (lastStateData[3] & 0x01)
	{
		KeyBoard(0x10, 1);	// bistabilny
		KeyBoard('J', 1);
		maskSwitch[3] |= 0x01;
		bitSwitch[3] |= 0x01;
	}
	else
	{
		maskSwitch[3] |= 0x01;
		bitSwitch[3] &= ~0x01;
		KeyBoard('J', 1);
	}
	//Światło lewe
	if (maskData[3] & 0x02) if (lastStateData[3] & 0x02)
	{
		KeyBoard(0x10, 1);	// bistabilny
		KeyBoard('Y', 1);
		maskSwitch[3] |= 0x02;
		bitSwitch[3] |= 0x02;
    }else
	{
		maskSwitch[3] |= 0x02;
		bitSwitch[3] &= ~0x02;
		KeyBoard('Y', 1);
    }
    //światło górne
	if (maskData[3] & 0x04) if (lastStateData[3] & 0x04)
	{
		KeyBoard(0x10, 1);	// bistabilny
		KeyBoard('U', 1);
		maskSwitch[3] |= 0x04;
		bitSwitch[3] |= 0x04;
	}
	else
	{
		maskSwitch[3] |= 0x04;
		bitSwitch[3] &= ~0x04;
		KeyBoard('U', 1);
	}
	//światło prawe
	if (maskData[3] & 0x08) if (lastStateData[3] & 0x08)
	{
		KeyBoard(0x10, 1);	// bistabilny
		KeyBoard('I', 1);
		maskSwitch[3] |= 0x08;
		bitSwitch[3] |= 0x08;
	}
	else
	{
		maskSwitch[3] |= 0x08;
		bitSwitch[3] &= ~0x08;
		KeyBoard('I', 1);
	}



    /* NASTAWNIK, BOCZNIK i KIERUNEK */

    if (bnkMask & 1)
    { // puszczanie klawiszy
        KeyBoard(0x6B, 0);
        bnkMask &= ~1;
    }
    if (bnkMask & 2)
    {
        KeyBoard(0x6D, 0);
        bnkMask &= ~2;
    }
    if (bnkMask & 4)
    {
        KeyBoard(0x6F, 0);
        bnkMask &= ~4;
    }
    if (bnkMask & 8)
    {
        KeyBoard(0x6A, 0);
        bnkMask &= ~8;
    }

    if (nastawnik < ReadDataBuff[6])
    {
        bnkMask |= 1;
        nastawnik++;
        KeyBoard(0x6B, 1); // wciśnij + i dodaj 1 do nastawnika
    }
    if (nastawnik > ReadDataBuff[6])
    {
        bnkMask |= 2;
        nastawnik--;
        KeyBoard(0x6D, 1); // wciśnij - i odejmij 1 do nastawnika
    }
    if (bocznik < ReadDataBuff[7])
    {
        bnkMask |= 4;
        bocznik++;
        KeyBoard(0x6F, 1); // wciśnij / i dodaj 1 do bocznika
    }
    if (bocznik > ReadDataBuff[7])
    {
        bnkMask |= 8;
        bocznik--;
        KeyBoard(0x6A, 1); // wciśnij * i odejmij 1 do bocznika
    }

    /* Obsługa HASLERA */
    if (ReadDataBuff[0] & 0x80)
        bCzuwak = true;

    if (bKabina1)
    { // logika rysika 1
        bRysik1H = true;
        bRysik1L = false;
        if (bPrzejazdSHP)
            bRysik1H = false;
    }
    else if (bKabina2)
    {
        bRysik1L = true;
        bRysik1H = false;
        if (bPrzejazdSHP)
            bRysik1L = false;
    }
    else
    {
        bRysik1H = false;
        bRysik1L = false;
    }

    if (bHamowanie)
    { // logika rysika 2
        bRysik2H = false;
        bRysik2L = true;
    }
    else
    {
        if (bCzuwak)
            bRysik2H = true;
        else
            bRysik2H = false;
        bRysik2L = false;
    }
    bCzuwak = false;
    if (bRysik1H)
        WriteDataBuff[6] |= 1 << 0;
    else
        WriteDataBuff[6] &= ~(1 << 0);
    if (bRysik1L)
        WriteDataBuff[6] |= 1 << 1;
    else
        WriteDataBuff[6] &= ~(1 << 1);
    if (bRysik2H)
        WriteDataBuff[6] |= 1 << 2;
    else
        WriteDataBuff[6] &= ~(1 << 2);
    if (bRysik2L)
        WriteDataBuff[6] |= 1 << 3;
    else
        WriteDataBuff[6] &= ~(1 << 3);
}

void TMWDComm::KeyBoard(int key, bool s) // emulacja klawiatury
{
    INPUT ip;
    // Set up a generic keyboard event.
    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0; // hardware scan code for key
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;

    ip.ki.wVk = key; // virtual-key code for the "a" key

    if (s)
    { // Press the "A" key
        ip.ki.dwFlags = 0; // 0 for key press
        SendInput(1, &ip, sizeof(INPUT));
    }
    else
    {
        ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
        SendInput(1, &ip, sizeof(INPUT));
    }

    // return 1;
}
