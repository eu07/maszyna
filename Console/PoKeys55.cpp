/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "PoKeys55.h"
#include <setupapi.h>
#include "utilities.h"

//---------------------------------------------------------------------------
// HIDscaner: http://forum.simflight.com/topic/68257-latest-lua-package-for-fsuipc-and-wideclient/
//#define MY_DEVICE_ID  "Vid_04d8&Pid_003F"
//#define MY_DEVICE_ID  "Vid_1dc3&Pid_1001&Rev_1000&MI_01"
// HID\Vid_1dc3&Pid_1001&Rev_1000&MI_01 - MI_01 to jest interfejs komunikacyjny (00-joystick,
// 02-klawiatura)

HANDLE WriteHandle = INVALID_HANDLE_VALUE;
HANDLE ReadHandle = INVALID_HANDLE_VALUE;
// GUID InterfaceClassGuid={0x4d1e55b2,0xf16f,0x11cf,0x88,0xcb,0x00,0x11,0x11,0x00,0x00,0x30};
//{4d1e55b2-f16f-11cf-88cb-001111000030}

TPoKeys55::TPoKeys55()
{
    cRequest = 0;
    iPWMbits = 1;
    iFaza = 0;
    iLastCommand = 0;
    fAnalog[0] = fAnalog[1] = fAnalog[2] = fAnalog[3] = fAnalog[4] = fAnalog[5] = fAnalog[6] = -1.0;
    iPWM[0] = iPWM[1] = iPWM[2] = iPWM[3] = iPWM[4] = iPWM[5] = iPWM[6] = 0;
    iPWM[7] = 4096;
	iInputs[0] = 0; // czy normalnie są w stanie wysokim?
    iRepeats = 0;
    bNoError = true;
};
//---------------------------------------------------------------------------
TPoKeys55::~TPoKeys55()
{
    Close();
};
//---------------------------------------------------------------------------
void TPoKeys55::Close()
{ // rozłączenie komunikacji
    if (WriteHandle != INVALID_HANDLE_VALUE)
        CloseHandle(WriteHandle);
    WriteHandle = INVALID_HANDLE_VALUE;
    if (ReadHandle != INVALID_HANDLE_VALUE)
        CloseHandle(ReadHandle);
    ReadHandle = INVALID_HANDLE_VALUE;
};
//---------------------------------------------------------------------------
bool TPoKeys55::Connect()
{ // Ra: to jest do wyczyszcznia z niepotrzebnych zmiennych i komunikatów
    Close();
    GUID InterfaceClassGuid = {0x4d1e55b2, 0xf16f, 0x11cf, 0x88, 0xcb, 0x00,
                               0x11,       0x11,   0x00,   0x00, 0x30}; // wszystkie HID tak mają
    HDEVINFO DeviceInfoTable;
    PSP_DEVICE_INTERFACE_DATA InterfaceDataStructure = new SP_DEVICE_INTERFACE_DATA;
    PSP_DEVICE_INTERFACE_DETAIL_DATA DetailedInterfaceDataStructure =
        new SP_DEVICE_INTERFACE_DETAIL_DATA;
    SP_DEVINFO_DATA DevInfoData;
    DWORD InterfaceIndex = 0;
    // DWORD StatusLastError=0;
    DWORD dwRegType;
    DWORD dwRegSize;
    DWORD StructureSize = 0;
    PBYTE PropertyValueBuffer;
    bool MatchFound;
    DWORD ErrorStatus;
    std::string DeviceIDFromRegistry;
    std::string DeviceIDToFind = "Vid_1dc3&Pid_1001&Rev_1000&MI_01";
    // First populate a list of plugged in devices (by specifying "DIGCF_PRESENT"), which are of the
    // specified class GUID.
    DeviceInfoTable =
        SetupDiGetClassDevs(&InterfaceClassGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    // Now look through the list we just populated.  We are trying to see if any of them match our
    // device.
    while (true)
    {
        InterfaceDataStructure->cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        if (SetupDiEnumDeviceInterfaces(DeviceInfoTable, NULL, &InterfaceClassGuid, InterfaceIndex,
                                        InterfaceDataStructure))
        {
            ErrorStatus = GetLastError();
            if (ERROR_NO_MORE_ITEMS == ErrorStatus) // Did we reach the end of the list of matching
            // devices in the DeviceInfoTable?
            { // Cound not find the device. Must not have been attached.
                SetupDiDestroyDeviceInfoList(
                    DeviceInfoTable); // Clean up the old structure we no longer need.
                // ShowMessage("Erreur: Sortie1");
                return false;
            }
        }
        else // Else some other kind of unknown error ocurred...
        {
            ErrorStatus = GetLastError();
            SetupDiDestroyDeviceInfoList(
                DeviceInfoTable); // Clean up the old structure we no longer need.
            // ShowMessage("Erreur: Sortie2");
            return false;
        }
        // Now retrieve the hardware ID from the registry. The hardware ID contains the VID and PID,
        // which we will then
        // check to see if it is the correct device or not.
        // Initialize an appropriate SP_DEVINFO_DATA structure. We need this structure for
        // SetupDiGetDeviceRegistryProperty().
        DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        SetupDiEnumDeviceInfo(DeviceInfoTable, InterfaceIndex, &DevInfoData);
        // First query for the size of the hardware ID, so we can know how big a buffer to allocate
        // for the data.
        SetupDiGetDeviceRegistryProperty(DeviceInfoTable, &DevInfoData, SPDRP_HARDWAREID,
                                         &dwRegType, NULL, 0, &dwRegSize);
        // Allocate a buffer for the hardware ID.
        // PropertyValueBuffer=(BYTE*)malloc(dwRegSize);
        PropertyValueBuffer = new BYTE[dwRegSize];
        if (PropertyValueBuffer == NULL) // if null,error,couldn't allocate enough memory
        { // Can't really recover from this situation,just exit instead.
            // ShowMessage("Allocation PropertyValueBuffer impossible");
            SetupDiDestroyDeviceInfoList(
                DeviceInfoTable); // Clean up the old structure we no longer need.
            return false;
        }
        // Retrieve the hardware IDs for the current device we are looking at. PropertyValueBuffer
        // gets filled with a
        // REG_MULTI_SZ (array of null terminated strings). To find a device,we only care about the
        // very first string in the
        // buffer,which will be the "device ID". The device ID is a string which contains the VID
        // and PID,in the example
        // format "Vid_04d8&Pid_003f".
        SetupDiGetDeviceRegistryProperty(DeviceInfoTable, &DevInfoData, SPDRP_HARDWAREID,
                                         &dwRegType, PropertyValueBuffer, dwRegSize, NULL);
        // Now check if the first string in the hardware ID matches the device ID of my USB device.
        // ListBox1->Items->Add((char*)PropertyValueBuffer);
        DeviceIDFromRegistry = reinterpret_cast<char*>(PropertyValueBuffer);
        // free(PropertyValueBuffer); //No longer need the PropertyValueBuffer,free the memory to
        // prevent potential memory leaks
        delete[] PropertyValueBuffer; // No longer need the PropertyValueBuffer,free the memory to
        // prevent potential memory leaks
        // Convert both strings to lower case.  This makes the code more robust/portable accross OS
        // Versions
        DeviceIDFromRegistry = ToLower( DeviceIDFromRegistry );
        DeviceIDToFind = ToLower( DeviceIDToFind );
        // Now check if the hardware ID we are looking at contains the correct VID/PID
        MatchFound = ( contains( DeviceIDFromRegistry, DeviceIDToFind ) );
        if (MatchFound == true)
        {
            // Device must have been found.  Open read and write handles.  In order to do this,we
            // will need the actual device path first.
            // We can get the path by calling SetupDiGetDeviceInterfaceDetail(),however,we have to
            // call this function twice:  The first
            // time to get the size of the required structure/buffer to hold the detailed interface
            // data,then a second time to actually
            // get the structure (after we have allocated enough memory for the structure.)
            DetailedInterfaceDataStructure->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            // First call populates "StructureSize" with the correct value
            SetupDiGetDeviceInterfaceDetail(DeviceInfoTable, InterfaceDataStructure, NULL, 0,
                                            &StructureSize, NULL);
            DetailedInterfaceDataStructure =
                (PSP_DEVICE_INTERFACE_DETAIL_DATA)(malloc(StructureSize)); // Allocate enough memory
            if (DetailedInterfaceDataStructure ==
                NULL) // if null,error,couldn't allocate enough memory
            { // Can't really recover from this situation,just exit instead.
                SetupDiDestroyDeviceInfoList(
                    DeviceInfoTable); // Clean up the old structure we no longer need.
                return false;
            }
            DetailedInterfaceDataStructure->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            // Now call SetupDiGetDeviceInterfaceDetail() a second time to receive the goods.
            SetupDiGetDeviceInterfaceDetail(DeviceInfoTable, InterfaceDataStructure,
                                            DetailedInterfaceDataStructure, StructureSize, NULL,
                                            NULL);
            // We now have the proper device path,and we can finally open read and write handles to
            // the device.
            // We store the handles in the global variables "WriteHandle" and "ReadHandle",which we
            // will use later to actually communicate.
            WriteHandle = CreateFile((DetailedInterfaceDataStructure->DevicePath), GENERIC_WRITE,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
            ErrorStatus = GetLastError();
            // if (ErrorStatus==ERROR_SUCCESS)
            // ToggleLedBtn->Enabled=true;//Make button no longer greyed out
            ReadHandle = CreateFile((DetailedInterfaceDataStructure->DevicePath), GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
            ErrorStatus = GetLastError();
            if (ErrorStatus == ERROR_SUCCESS)
            {
                // GetPushbuttonState->Enabled=true;//Make button no longer greyed out
                // StateLabel->Enabled=true;//Make label no longer greyed out
            }
            SetupDiDestroyDeviceInfoList(
                DeviceInfoTable); // Clean up the old structure we no longer need.
            iRepeats = 0; // nowe szanse na podłączenie
            return true;
        }
        InterfaceIndex++;
        // Keep looping until we either find a device with matching VID and PID,or until we run out
        // of items.
    } // end of while(true)
    // ShowMessage("Sortie");
    return false;
}
//---------------------------------------------------------------------------
bool TPoKeys55::Write(unsigned char c, unsigned char b3, unsigned char b4, unsigned char b5)
{
    DWORD BytesWritten = 0;
    OutputBuffer[0] = 0; // The first byte is the "Report ID" and does not get transmitted over the
    // USB bus. Always set=0.
    OutputBuffer[1] = 0xBB; // 0xBB - bajt rozpoznawczy dla PoKeys55
    OutputBuffer[2] = iLastCommand = c; // operacja: 0x31: blokowy odczyt wejść
    OutputBuffer[3] = b3; // np. numer pinu (o 1 mniej niż numer na płytce)
    OutputBuffer[4] = b4;
    OutputBuffer[5] = b5;
    OutputBuffer[6] = 0;
    OutputBuffer[7] = ++cRequest; // numer żądania
    OutputBuffer[8] = 0;
    for (int i = 0; i < 8; ++i)
        OutputBuffer[8] += OutputBuffer[i]; // czy sumować też od 9 do 64?
    // The basic Windows I/O functions WriteFile() and ReadFile() can be used to read and write to
    // HID class USB devices
    //(once we have the read and write handles to the device, which are obtained with CreateFile()).
    // The following call to WriteFile() sends 64 bytes of data to the USB device.
    WriteFile(WriteHandle, &OutputBuffer, 65, &BytesWritten,
              0); // Blocking function, unless an "overlapped" structure is used
    return (BytesWritten == 65);
    // Read(); //odczyt trzeba zrobić inaczej - w tym miejscu będzie za szybko i nic się nie odczyta
}

//---------------------------------------------------------------------------

bool TPoKeys55::Read()
{
    DWORD BytesRead = 0;
    InputBuffer[0] = 0; // The first byte is the "Report ID" and does not get transmitted over the
    // USB bus.  Always set=0.
    // Now get the response packet from the firmware.
    // The following call to ReadFIle() retrieves 64 bytes of data from the USB device.
    ReadFile(ReadHandle, &InputBuffer, 65, &BytesRead,
             0); // Blocking function,unless an "overlapped" structure is used
    // InputPacketBuffer[0] is the report ID, which we don't care about.
    // InputPacketBuffer[1] is an echo back of the command.
    // InputPacketBuffer[2] contains the I/O port pin value for the pushbutton.
    return (BytesRead == 65) ? InputBuffer[7] == cRequest : false;
}
//---------------------------------------------------------------------------
bool TPoKeys55::ReadLoop(int i)
{ // próbuje odczytać (i) razy
    do
    {
        if (Read())
            return true;
        Sleep(1); // trochę poczekać, aż odpowie
    } while (--i);
    return false;
}
//---------------------------------------------------------------------------
std::string TPoKeys55::Version()
{ // zwraca numer wersji, funkcja nieoptymalna czasowo (czeka na odpowiedź)
    if (!WriteHandle)
        return "";
    Write(0x00, 0); // 0x00 - Read serial number, version
    if (ReadLoop(10))
    { // 3: serial MSB; 4: serial LSB; 5: software version (v(1+[4-7]).([0-3])); 6: revision number
        std::string s = "PoKeys55 #" + std::to_string((InputBuffer[3] << 8) + InputBuffer[4]);
        s += " v" + std::to_string(1 + (InputBuffer[5] >> 4)) + "." + std::to_string(InputBuffer[5] & 15) +
             "." + std::to_string(InputBuffer[6]);
        /* //Ra: pozyskiwanie daty można sobie darować, jest poniekąd bez sensu
          Write(0x04,0); //0x04 - Read build date: drugi argument zmieniać od 0 do 2, uzyskując
          kolejno po 4 znaki
          if (ReadLoop(5))
          {//2: 0x04; 3-6: char 1-4, 5-8, 9-11; (83-65-112-32-32-49-32-50-48-49-49-0=="Sep  1 2011")
           s+=" ("+AnsiString((char*)InputBuffer+3,4);
           Write(0x04,1); //0x04 - Read build date: drugi argument zmieniać od 0 do 2, uzyskując
          kolejno po 4 znaki
           if (ReadLoop(5))
           {s+=AnsiString((char*)InputBuffer+3,4);
            Write(0x04,2); //0x04 - Read build date: drugi argument zmieniać od 0 do 2, uzyskując
          kolejno po 4 znaki
            if (ReadLoop(5))
             s+=AnsiString((char*)InputBuffer+3,3);
           }
           s+=")";
          }
        */
        return s;
    }
    return "";
};

bool TPoKeys55::PWM(int x, float y)
{ // ustawienie wskazanego PWM (@12Mhz: 12000=1ms=1000Hz)
    // iPWM[7]=1024; //1024==85333.3333333333ns=11718.75Hz
    iPWM[x] = int(0.5f + 0x0FFF * y) & 0x0FFF; // 0x0FFF=4095
    return true;
}

bool TPoKeys55::Update(bool pause)
{ // funkcja powinna być wywoływana regularnie, np. raz w każdej ramce ekranowej
    if (pause)
    { // specjalna procedura, jeśli utracone połączenie spowodowało pauzę
        iLastCommand = 0; // połączenie zostało na nowo otwarte
        // iFaza=0; //jeden błąd i podtrzymanie pauzy jest kontynuowane
    }
    switch (iFaza)
    {
    case 0: // uaktualnienie PWM raz na jakiś czas
        OutputBuffer[9] = 0x3F; // maska użytych PWM
        *((int *)(OutputBuffer + 10)) = iPWM[0]; // PWM1 (pin 22)
        *((int *)(OutputBuffer + 14)) = iPWM[1]; // PWM2 (pin 21)
        *((int *)(OutputBuffer + 18)) = iPWM[2]; // PWM3 (pin 20)
        *((int *)(OutputBuffer + 22)) = iPWM[3]; // PWM4 (pin 19)
        *((int *)(OutputBuffer + 26)) = iPWM[4]; // PWM5 (pin 18)
        *((int *)(OutputBuffer + 30)) = iPWM[5]; // PWM6 (pin 17)
        *((int *)(OutputBuffer + 34)) = iPWM[7]; // PWM period
        if (Write(0xCB, 1)) // wysłanie ustawień (1-ustaw, 0-odczyt)
            iRepeats = 0; // informacja, że poszło dobrze
        ++iFaza; // ta faza została zakończona
        // iRepeats=0;
        break;
    case 1: // odczyt wejść analogowych - komenda i przetwarzanie
        if (iLastCommand != 0x3A) // asynchroniczne ustawienie kontrolki może namieszać
            Write(0x3A, 0); // 0x3A - Analog inputs reading – all analog inputs in one command
        else if (Read())
        { // jest odebrana ramka i zgodność numeru żądania
            fAnalog[0] = ((InputBuffer[21] << 8) + InputBuffer[22]) / 4095.0f; // pin 47
            fAnalog[1] = ((InputBuffer[19] << 8) + InputBuffer[20]) / 4095.0f; // pin 46
            fAnalog[2] = ((InputBuffer[17] << 8) + InputBuffer[18]) / 4095.0f; // pin 45
            fAnalog[3] = ((InputBuffer[15] << 8) + InputBuffer[16]) / 4095.0f; // pin 44
            fAnalog[4] = ((InputBuffer[13] << 8) + InputBuffer[14]) / 4095.0f; // pin 43
            fAnalog[5] = ((InputBuffer[11] << 8) + InputBuffer[12]) / 4095.0f; // pin 42
            fAnalog[6] = ((InputBuffer[9] << 8) + InputBuffer[10]) / 4095.0f; // pin 41
            ++iFaza; // skoro odczytano, można przejść do kolejnej fazy
            iRepeats = 0; // zerowanie licznika prób
        }
        else
            ++iRepeats; // licznik nieudanych prób
        break;
    case 2: // odczyt wejść cyfrowych - komenda i przetwarzanie
        if (iLastCommand != 0x31) // asynchroniczne ustawienie kontrolki może namieszać
            Write(0x31, 0); // 0x31: blokowy odczyt wejść
        else if (Read())
        { // jest odebrana ramka i zgodność numeru żądania
            iInputs[0] = *((int *)(InputBuffer + 3)); // odczyt 32 bitów
            iFaza = 3; // skoro odczytano, można kolejny cykl
            iRepeats = 0; // zerowanie licznika prób
        }
        else
            ++iRepeats; // licznik nieudanych prób
        break;
    case 3: // ustawienie wyjść analogowych, 0..4095 mapować na 0..65520 (<<4)
        if (Write(0x41, 43 - 1, (iPWM[6] >> 4), (iPWM[6] << 4))) // wysłanie ustawień
            iRepeats = 0; // informacja, że poszło dobrze
        iFaza = 0; //++iFaza; //ta faza została zakończona
        // powinno jeszcze przyjść potwierdzenie o kodzie 0x41
        break;
	default:
        iFaza = 0; // na wypadek, gdyby zbłądziło po jakichś zmianach w kodzie
        // iRepeats=0;
    }
    if (!iRepeats)
        bNoError = true; // jest OK
    else if (iRepeats >= 10) // youBy 2014-07: przy 5 powtórzeniach sieje mi pauzą po 2 razy na
    // sekundę, a przy 10 jest ok
    { // przekroczenie liczby prób wymusza kolejną fazę
        ++iFaza;
        iRepeats = 1; // w nowej fazie nowe szanse, ale nie od 0!
        bNoError = false; // zgłosić błąd
    }
    return (bNoError); // true oznacza prawidłowe działanie
    // czy w przypadku błędu komunikacji z PoKeys włączać pauzę?
    // dopiero poprawne podłączenie zeruje licznik prób
};
