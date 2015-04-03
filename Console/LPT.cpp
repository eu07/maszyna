//---------------------------------------------------------------------------

#include "LPT.h"
#include <windows.h>

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
    // ladowanie dll-ki
    hDLL = LoadLibrary("inpout32.dll");
    if (hDLL)
    {
        InPort = (InPortType)GetProcAddress(hDLL, "Inp32");
        OutPort = (OutPortType)GetProcAddress(hDLL, "Out32");
    }
    else
        return false; // MessageBox(NULL,"ERROR","B³¹d przy ³adowaniu pliku",MB_OK);
    address =
        port; //&0xFFFFFC; //ostatnie 2 bity maj¹ byæ zerowe -> a niech sobie OUT-uj¹, gdzie chc¹
    switch (address) // nie dotyczy 0x3BC
    {
    case 0x0378:
    case 0x0278:
        OutPort(address + 0x402, 0); // SPP, czyli jednokierunkowe wyjœcie
        break;
    case 0xBC00:
    case 0xBD00:
        OutPort(address + 0x006, 0); // 0xBC06? czysta improwizacja
    }
    return bool(OutPort);
};

void TLPT::Out(int x) { // wys³anie bajtu do portu OutPort(address, x); };
