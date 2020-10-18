/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "LPT.h"
#include "Logs.h"

// LPT na USB:
// USB\VID_067B&PID_2305&REV_0200
//{9d7debbc-c85d-11d1-9eb4-006008c3a19a}
// USB\VID_067B&PID_2305\5&1E41AFF0&0&2
// IEEE-1284 Controller

HINSTANCE hDLL;
typedef USHORT(__stdcall *InPortType)(USHORT BasePort);
typedef void(__stdcall *OutPortType)(USHORT BasePort, USHORT value);
InPortType InPort;
OutPortType OutPort;

bool TLPT::Connect(int port)
{
	WriteLog("lpt: trying to connect lpt, port " + std::to_string(port));

    // ladowanie dll-ki
    hDLL = LoadLibrary("inpout32.dll");
    if (hDLL)
    {
		WriteLog("lpt: inpout32 loaded");
        InPort = (InPortType)GetProcAddress(hDLL, "Inp32");
		OutPort = (OutPortType)GetProcAddress(hDLL, "Out32");
    }
	else
	{
		WriteLog("lpt: failed to load inpout32");
		return false;
	}
    address = port;
    switch (address) // nie dotyczy 0x3BC
    {
    case 0x0378:
    case 0x0278:
        OutPort(address + 0x402, 0); // SPP, czyli jednokierunkowe wyjście
        break;
    case 0xBC00:
    case 0xBD00:
        OutPort(address + 0x006, 0); // 0xBC06? czysta improwizacja
    }

	WriteLog("lpt: port opened successfully");

    return OutPort != 0;
};

void TLPT::Out(int x)
{ // wysłanie bajtu do portu
    OutPort(address, x);
};
