/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

*/

#include "system.hpp"
#include "classes.hpp"
#pragma hdrstop

#include "Train.h"
#include "MdlMngr.h"
#include "Globals.h"
#include "Timer.h"
#include "Driver.h"
#include "Console.h"
//---------------------------------------------------------------------------

#pragma package(smart_init)

using namespace Timer;

TCab::TCab()
{
    CabPos1.x = -1.0;
    CabPos1.y = 1.0;
    CabPos1.z = 1.0;
    CabPos2.x = 1.0;
    CabPos2.y = 1.0;
    CabPos2.z = -1.0;
    bEnabled = false;
    bOccupied = true;
    dimm_r = dimm_g = dimm_b = 1;
    intlit_r = intlit_g = intlit_b = 0;
    intlitlow_r = intlitlow_g = intlitlow_b = 0;
    iGaugesMax = 100; // 95 - trzeba pobieraæ to z pliku konfiguracyjnego
    ggList = new TGauge[iGaugesMax];
    iGauges = 0; // na razie nie s¹ dodane
    iButtonsMax = 60; // 55 - trzeba pobieraæ to z pliku konfiguracyjnego
    btList = new TButton[iButtonsMax];
    iButtons = 0;
}

void TCab::Init(double Initx1, double Inity1, double Initz1, double Initx2,
                           double Inity2, double Initz2, bool InitEnabled, bool InitOccupied)
{
    CabPos1.x = Initx1;
    CabPos1.y = Inity1;
    CabPos1.z = Initz1;
    CabPos2.x = Initx2;
    CabPos2.y = Inity2;
    CabPos2.z = Initz2;
    bEnabled = InitEnabled;
    bOccupied = InitOccupied;
}

void TCab::Load(TQueryParserComp *Parser)
{
    AnsiString str = Parser->GetNextSymbol().LowerCase();
    if (str == AnsiString("cablight"))
    {
        dimm_r = Parser->GetNextSymbol().ToDouble();
        dimm_g = Parser->GetNextSymbol().ToDouble();
        dimm_b = Parser->GetNextSymbol().ToDouble();
        intlit_r = Parser->GetNextSymbol().ToDouble();
        intlit_g = Parser->GetNextSymbol().ToDouble();
        intlit_b = Parser->GetNextSymbol().ToDouble();
        intlitlow_r = Parser->GetNextSymbol().ToDouble();
        intlitlow_g = Parser->GetNextSymbol().ToDouble();
        intlitlow_b = Parser->GetNextSymbol().ToDouble();
        str = Parser->GetNextSymbol().LowerCase();
    }
    CabPos1.x = str.ToDouble();
    CabPos1.y = Parser->GetNextSymbol().ToDouble();
    CabPos1.z = Parser->GetNextSymbol().ToDouble();
    CabPos2.x = Parser->GetNextSymbol().ToDouble();
    CabPos2.y = Parser->GetNextSymbol().ToDouble();
    CabPos2.z = Parser->GetNextSymbol().ToDouble();

    bEnabled = True;
    bOccupied = True;
}

TCab::~TCab()
{
    delete[] ggList;
    delete[] btList;
};

TGauge *__fastcall TCab::Gauge(int n)
{ // pobranie adresu obiektu aniomowanego ruchem
    if (n < 0)
    { // rezerwacja wolnego
        ggList[iGauges].Clear();
        return ggList + iGauges++;
    }
    else if (n < iGauges)
        return ggList + n;
    return NULL;
};
TButton *__fastcall TCab::Button(int n)
{ // pobranie adresu obiektu animowanego wyborem 1 z 2
    if (n < 0)
    { // rezerwacja wolnego
        return btList + iButtons++;
    }
    else if (n < iButtons)
        return btList + n;
    return NULL;
};

void TCab::Update()
{ // odczyt parametrów i ustawienie animacji submodelom
    int i;
    for (i = 0; i < iGauges; ++i)
    { // animacje izometryczne
        ggList[i].UpdateValue(); // odczyt parametru i przeliczenie na k¹t
        ggList[i].Update(); // ustawienie animacji
    }
    for (i = 0; i < iButtons; ++i)
    { // animacje dwustanowe
        // btList[i].Update(); //odczyt parametru i wybór submodelu
    }
};

TTrain::TTrain()
{
    ActiveUniversal4 = false;
    ShowNextCurrent = false;
    // McZapkie-240302 - przyda sie do tachometru
    fTachoVelocity = 0;
    fTachoCount = 0;
    fPPress = fNPress = 0;

    // asMessage="";
    fMechCroach = 0.25;
    pMechShake = vector3(0, 0, 0);
    vMechMovement = vector3(0, 0, 0);
    pMechOffset = vector3(0, 0, 0);
    fBlinkTimer = 0;
    fHaslerTimer = 0;
    keybrakecount = 0;
    DynamicSet(NULL); // ustawia wszystkie mv*
    iCabLightFlag = 0;
    // hunter-091012
    bCabLight = false;
    bCabLightDim = false;
    //-----
    pMechSittingPosition = vector3(0, 0, 0); // ABu: 180404
    LampkaUniversal3_st = false; // ABu: 030405
    dsbNastawnikJazdy = NULL;
    dsbNastawnikBocz = NULL;
    dsbRelay = NULL;
    dsbPneumaticRelay = NULL;
    dsbSwitch = NULL;
    dsbPneumaticSwitch = NULL;
    dsbReverserKey = NULL; // hunter-121211
    dsbCouplerAttach = NULL;
    dsbCouplerDetach = NULL;
    dsbDieselIgnition = NULL;
    dsbDoorClose = NULL;
    dsbDoorOpen = NULL;
    dsbPantUp = NULL;
    dsbPantDown = NULL;
    dsbWejscie_na_bezoporow = NULL;
    dsbWejscie_na_drugi_uklad = NULL; // hunter-081211: poprawka literowki
    dsbHasler = NULL;
    dsbBuzzer = NULL;
    dsbSlipAlarm = NULL; // Bombardier 011010: alarm przy poslizgu dla 181/182
    dsbCouplerStretch = NULL;
    dsbEN57_CouplerStretch = NULL;
    dsbBufferClamp = NULL;
    iRadioChannel = 0;
    fTachoTimer = 0.0; // w³¹czenie skoków wskazañ prêdkoœciomierza
}

TTrain::~TTrain()
{
    if (DynamicObject)
        if (DynamicObject->Mechanik)
            DynamicObject->Mechanik->TakeControl(
                true); // likwidacja kabiny wymaga przejêcia przez AI
}

bool TTrain::Init(TDynamicObject *NewDynamicObject, bool e3d)
{ // powi¹zanie rêcznego sterowania kabin¹ z pojazdem
    // Global::pUserDynamic=NewDynamicObject; //pojazd renderowany bez trzêsienia
    DynamicSet(NewDynamicObject);
    if (!e3d)
        if (DynamicObject->Mechanik == NULL)
            return false;
    // if (DynamicObject->Mechanik->AIControllFlag==AIdriver)
    // return false;
    DynamicObject->MechInside = true;

    /*    iPozSzereg=28;
        for (int i=1; i<mvControlled->MainCtrlPosNo; i++)
         {
          if (mvControlled->RList[i].Bn>1)
           {
            iPozSzereg=i-1;
            i=mvControlled->MainCtrlPosNo+1;
           }
         }
    */
    MechSpring.Init(0, 500);
    vMechVelocity = vector3(0, 0, 0);
    pMechOffset = vector3(-0.4, 3.3, 5.5);
    fMechCroach = 0.5;
    fMechSpringX = 1;
    fMechSpringY = 0.1;
    fMechSpringZ = 0.1;
    fMechMaxSpring = 0.15;
    fMechRoll = 0.05;
    fMechPitch = 0.1;
    fMainRelayTimer = 0; // Hunter, do k...y nêdzy, ustawiaj wartoœci pocz¹tkowe zmiennych!

    if (!LoadMMediaFile(DynamicObject->asBaseDir + DynamicObject->MoverParameters->TypeName +
                        ".mmd"))
        return false;

    // McZapkie: w razie wykolejenia
    //    dsbDerailment=TSoundsManager::GetFromName("derail.wav");
    // McZapkie: jazda luzem:
    //    dsbRunningNoise=TSoundsManager::GetFromName("runningnoise.wav");

    // McZapkie? - dzwieki slyszalne tylko wewnatrz kabiny - generowane przez obiekt sterowany:

    // McZapkie-080302    sWentylatory.Init("wenton.wav","went.wav","wentoff.wav");
    // McZapkie-010302
    //    sCompressor.Init("compressor-start.wav","compressor.wav","compressor-stop.wav");

    //    sHorn1.Init("horn1.wav",0.3);
    //    sHorn2.Init("horn2.wav",0.3);

    //  sHorn1.Init("horn1-start.wav","horn1.wav","horn1-stop.wav");
    //  sHorn2.Init("horn2-start.wav","horn2.wav","horn2-stop.wav");

    //  sConverter.Init("converter.wav",1.5); //NBMX obsluga przez AdvSound
    iCabn = 0;
    // Ra: taka proteza - przes³anie kierunku do cz³onów connected
    if (mvControlled->ActiveDir > 0)
    { // by³o do przodu
        mvControlled->DirectionBackward();
        mvControlled->DirectionForward();
    }
    else if (mvControlled->ActiveDir < 0)
    {
        mvControlled->DirectionForward();
        mvControlled->DirectionBackward();
    }
    return true;
}

void TTrain::OnKeyDown(int cKey)
{ // naciœniêcie klawisza
    bool isEztOer;
    isEztOer = ((mvControlled->TrainType == dt_EZT) && (mvControlled->Battery == true) &&
                (mvControlled->EpFuse == true) && (mvOccupied->BrakeSubsystem == ss_ESt) &&
                (mvControlled->ActiveDir != 0)); // od yB
    // isEztOer=(mvControlled->TrainType==dt_EZT)&&(mvControlled->Mains)&&(mvOccupied->BrakeSubsystem==ss_ESt)&&(mvControlled->ActiveDir!=0);
    // isEztOer=((mvControlled->TrainType==dt_EZT)&&(mvControlled->Battery==true)&&(mvControlled->EpFuse==true)&&(mvOccupied->BrakeSubsystem==Oerlikon)&&(mvControlled->ActiveDir!=0));

    if (GetAsyncKeyState(VK_SHIFT) < 0)
    { // wciœniêty [Shift]
        if (cKey == Global::Keys[k_IncMainCtrlFAST]) // McZapkie-200702: szybkie przelaczanie na
                                                     // poz. bezoporowa
        {
            if (mvControlled->IncMainCtrl(2))
            {
                dsbNastawnikJazdy->SetCurrentPosition(0);
                dsbNastawnikJazdy->Play(0, 0, 0);
            }
        }
        else if (cKey == Global::Keys[k_DirectionBackward])
        {
            if (mvControlled->Radio == false)
                if (GetAsyncKeyState(VK_CONTROL) >= 0)
                {
                    dsbSwitch->SetVolume(DSBVOLUME_MAX);
                    dsbSwitch->Play(0, 0, 0);
                    mvControlled->Radio = true;
                }
        }
        else

            if (cKey == Global::Keys[k_DecMainCtrlFAST])
            if (mvControlled->DecMainCtrl(2))
            {
                dsbNastawnikJazdy->SetCurrentPosition(0);
                dsbNastawnikJazdy->Play(0, 0, 0);
            }
            else
                ;
        else if (cKey == Global::Keys[k_IncScndCtrlFAST])
            if (mvControlled->IncScndCtrl(2))
            {
                if (dsbNastawnikBocz) // hunter-081211
                {
                    dsbNastawnikBocz->SetCurrentPosition(0);
                    dsbNastawnikBocz->Play(0, 0, 0);
                }
                else if (!dsbNastawnikBocz)
                {
                    dsbNastawnikJazdy->SetCurrentPosition(0);
                    dsbNastawnikJazdy->Play(0, 0, 0);
                }
            }
            else
                ;
        else if (cKey == Global::Keys[k_DecScndCtrlFAST])
            if (mvControlled->DecScndCtrl(2))
            {
                if (dsbNastawnikBocz) // hunter-081211
                {
                    dsbNastawnikBocz->SetCurrentPosition(0);
                    dsbNastawnikBocz->Play(0, 0, 0);
                }
                else if (!dsbNastawnikBocz)
                {
                    dsbNastawnikJazdy->SetCurrentPosition(0);
                    dsbNastawnikJazdy->Play(0, 0, 0);
                }
            }
            else
                ;
        else if (cKey == Global::Keys[k_IncLocalBrakeLevelFAST])
            if (mvOccupied->IncLocalBrakeLevel(2))
                ;
            else
                ;
        else if (cKey == Global::Keys[k_DecLocalBrakeLevelFAST])
            if (mvOccupied->DecLocalBrakeLevel(2))
                ;
            else
                ;
        // McZapkie-240302 - wlaczanie glownego obwodu klawiszem M+shift
        //-----------
        // hunter-141211: wyl. szybki zalaczony przeniesiony do TTrain::Update()
        /* if (cKey==Global::Keys[k_Main])
        {
           ggMainOnButton.PutValue(1);
           if (mvControlled->MainSwitch(true))
             {
                if (mvControlled->EngineType==DieselEngine)
                 dsbDieselIgnition->Play(0,0,0);
                else
                 dsbNastawnikJazdy->Play(0,0,0);
             }
        }
        else */
        if (cKey == Global::Keys[k_Battery])
        {
            // if
            // (((mvControlled->TrainType==dt_EZT)||(mvControlled->EngineType==ElectricSeriesMotor)||(mvControlled->EngineType==DieselElectric))&&(!mvControlled->Battery))
            if (!mvControlled->Battery)
            { // wy³¹cznik jest te¿ w SN61, ewentualnie za³¹czaæ pr¹d na sta³e z poziomu FIZ
                if (mvOccupied->BatterySwitch(true)) // bateria potrzebna np. do zapalenia œwiate³
                {
                    dsbSwitch->Play(0, 0, 0);
                    if (TestFlag(mvOccupied->SecuritySystem.SystemType,
                                 2)) // Ra: znowu w kabinie jest coœ, co byæ nie powinno!
                    {
                        SetFlag(mvOccupied->SecuritySystem.Status, s_active);
                        SetFlag(mvOccupied->SecuritySystem.Status, s_SHPalarm);
                    }
                }
            }
        }
        else if (cKey == Global::Keys[k_StLinOff])
        {
            if (mvControlled->TrainType == dt_EZT)
            {
                if ((mvControlled->Signalling == false))
                {
                    dsbSwitch->Play(0, 0, 0);
                    mvControlled->Signalling = true;
                }
            }
        }
        else if (cKey == Global::Keys[k_Sand])
        {
            if (mvControlled->TrainType == dt_EZT)
            {
                if (!mvControlled->DoorSignalling)
                {
                    dsbSwitch->Play(0, 0, 0);
                    mvControlled->DoorSignalling = true;
                }
            }
        }
        if (cKey == Global::Keys[k_Main])
        {
            if (fabs(ggMainOnButton.GetValue()) < 0.001)
                if (dsbSwitch)
                {
                    dsbSwitch->SetVolume(DSBVOLUME_MAX);
                    dsbSwitch->Play(0, 0, 0);
                }
        }
        else
            //-----------

            if (cKey ==
                Global::Keys[k_BrakeProfile]) // McZapkie-240302-B: przelacznik opoznienia hamowania
        { // yB://ABu: male poprawki, zeby bylo mozna ustawic dowolny wagon
            int CouplNr = -2;
            if (!FreeFlyModeFlag)
            {
                if (GetAsyncKeyState(VK_CONTROL) < 0)
                    if (mvOccupied->BrakeDelaySwitch(bdelay_R + bdelay_M))
                    {
                        dsbPneumaticRelay->SetVolume(DSBVOLUME_MAX);
                        dsbPneumaticRelay->Play(0, 0, 0);
                    }
                    else
                        ;
                else if (mvOccupied->BrakeDelaySwitch(bdelay_P))
                {
                    dsbPneumaticRelay->SetVolume(DSBVOLUME_MAX);
                    dsbPneumaticRelay->Play(0, 0, 0);
                }
            }
            else
            {
                TDynamicObject *temp;
                temp = (DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(), -1, 1500,
                                                            CouplNr));
                if (temp == NULL)
                {
                    CouplNr = -2;
                    temp = (DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(), 1, 1500,
                                                                CouplNr));
                }
                if (temp)
                {
                    if (GetAsyncKeyState(VK_CONTROL) < 0)
                        if (temp->MoverParameters->BrakeDelaySwitch(bdelay_R + bdelay_M))
                        {
                            dsbPneumaticRelay->SetVolume(DSBVOLUME_MAX);
                            dsbPneumaticRelay->Play(0, 0, 0);
                        }
                        else
                            ;
                    else if (temp->MoverParameters->BrakeDelaySwitch(bdelay_P))
                    {
                        dsbPneumaticRelay->SetVolume(DSBVOLUME_MAX);
                        dsbPneumaticRelay->Play(0, 0, 0);
                    }
                }
            }
        }
        else
            //-----------
            // hunter-261211: przetwornica i sprzezarka przeniesione do TTrain::Update()
            /* if (cKey==Global::Keys[k_Converter])   //NBMX 14-09-2003: przetwornica wl
            {
              if ((mvControlled->PantFrontVolt) || (mvControlled->PantRearVolt) ||
            (mvControlled->EnginePowerSource.SourceType!=CurrentCollector) ||
            (!Global::bLiveTraction))
                 if (mvControlled->ConverterSwitch(true))
                 {
                     dsbSwitch->SetVolume(DSBVOLUME_MAX);
                     dsbSwitch->Play(0,0,0);
                 }
            }
            else
            if (cKey==Global::Keys[k_Compressor])   //NBMX 14-09-2003: sprezarka wl
            {
              if ((mvControlled->ConverterFlag) || (mvControlled->CompressorPower<2))
                 if (mvControlled->CompressorSwitch(true))
                 {
                     dsbSwitch->SetVolume(DSBVOLUME_MAX);
                     dsbSwitch->Play(0,0,0);
                 }
            }
            else */

            if (cKey == Global::Keys[k_Converter])
        {
            if (ggConverterButton.GetValue() == 0)
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play(0, 0, 0);
            }
        }
        else
            // if
            // ((cKey==Global::Keys[k_Compressor])&&((mvControlled->EngineType==ElectricSeriesMotor)||(mvControlled->TrainType==dt_EZT)))
            // //hunter-110212: poprawka dla EZT
            if ((cKey == Global::Keys[k_Compressor]) &&
                (mvControlled->CompressorPower < 2)) // hunter-091012: tak jest poprawnie
        {
            if (ggCompressorButton.GetValue() == 0)
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play(0, 0, 0);
            }
        }
        else if (cKey == Global::Keys[k_SmallCompressor]) // Winger 160404: mala sprezarka wl
        { // Ra: dŸwiêk, gdy razem z [Shift]
            if ((mvControlled->TrainType & dt_EZT) ? mvControlled == mvOccupied :
                                                     !mvOccupied->ActiveCab) // tylko w maszynowym
                if (Console::Pressed(VK_CONTROL)) // z [Ctrl]
                    mvControlled->bPantKurek3 = true; // zbiornik pantografu po³¹czony jest ze
                                                      // zbiornikiem g³ównym (pompowanie nie ma
                                                      // sensu)
                else if (!mvControlled->PantCompFlag) // jeœli wy³¹czona
                    if (mvControlled->Battery) // jeszcze musi byæ za³¹czona bateria
                        if (mvControlled->PantPress < 4.8) // pisz¹, ¿e to tak nie dzia³a
                        {
                            mvControlled->PantCompFlag = true;
                            dsbSwitch->SetVolume(DSBVOLUME_MAX);
                            dsbSwitch->Play(0, 0, 0); // dŸwiêk tylko po naciœniêciu klawisza
                        }
        }
        else if (cKey == VkKeyScan('q')) // ze Shiftem - w³¹czenie AI
        { // McZapkie-240302 - wlaczanie automatycznego pilota (zadziala tylko w trybie debugmode)
            if (DynamicObject->Mechanik)
            {
                if (DebugModeFlag)
                    if (DynamicObject->Mechanik
                            ->AIControllFlag) //¿eby nie trzeba by³o roz³¹czaæ dla zresetowania
                        DynamicObject->Mechanik->TakeControl(false);
                DynamicObject->Mechanik->TakeControl(true);
            }
        }
        else if (cKey == Global::Keys[k_MaxCurrent]) // McZapkie-160502: F - wysoki rozruch
        {
            if ((mvControlled->EngineType == DieselElectric) && (mvControlled->ShuntModeAllow) &&
                (mvControlled->MainCtrlPos == 0))
            {
                mvControlled->ShuntMode = true;
            }
            if (mvControlled->CurrentSwitch(true))
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play(0, 0, 0);
            }
            /* Ra: przeniesione do Mover.cpp
                   if (mvControlled->TrainType!=dt_EZT) //to powinno byæ w fizyce, a nie w kabinie!
                    if (mvControlled->MinCurrentSwitch(true))
                    {
                     dsbSwitch->SetVolume(DSBVOLUME_MAX);
                     dsbSwitch->Play(0,0,0);
                    }
            */
        }
        else if (cKey == Global::Keys[k_CurrentAutoRelay]) // McZapkie-241002: G - wlaczanie PSR
        {
            if (mvControlled->AutoRelaySwitch(true))
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play(0, 0, 0);
            }
        }
        else if (cKey == Global::Keys[k_FailedEngineCutOff]) // McZapkie-060103: E - wylaczanie
                                                             // sekcji silnikow
        {
            if (mvControlled->CutOffEngine())
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play(0, 0, 0);
            }
        }
        else if (cKey == Global::Keys[k_OpenLeft]) // NBMX 17-09-2003: otwieranie drzwi
        {
            if (mvOccupied->DoorOpenCtrl == 1)
                if (mvOccupied->CabNo < 0 ? mvOccupied->DoorRight(true) :
                                            mvOccupied->DoorLeft(true))
                {
                    dsbSwitch->SetVolume(DSBVOLUME_MAX);
                    dsbSwitch->Play(0, 0, 0);
                    if (dsbDoorOpen)
                    {
                        dsbDoorOpen->SetCurrentPosition(0);
                        dsbDoorOpen->Play(0, 0, 0);
                    }
                }
        }
        else if (cKey == Global::Keys[k_OpenRight]) // NBMX 17-09-2003: otwieranie drzwi
        {
            if (mvOccupied->DoorCloseCtrl == 1)
                if (mvOccupied->CabNo < 0 ? mvOccupied->DoorLeft(true) :
                                            mvOccupied->DoorRight(true))
                {
                    dsbSwitch->SetVolume(DSBVOLUME_MAX);
                    dsbSwitch->Play(0, 0, 0);
                    if (dsbDoorOpen)
                    {
                        dsbDoorOpen->SetCurrentPosition(0);
                        dsbDoorOpen->Play(0, 0, 0);
                    }
                }
        }
        else
            //-----------
            // hunter-131211: dzwiek dla przelacznika universala podniesionego
            // hunter-091012: ubajerowanie swiatla w kabinie (wyrzucenie przyciemnienia pod Univ4)
            if (cKey == Global::Keys[k_Univ3])
        {
            if (Console::Pressed(VK_CONTROL))
            {
                if (bCabLight == false) //(ggCabLightButton.GetValue()==0)
                {
                    dsbSwitch->SetVolume(DSBVOLUME_MAX);
                    dsbSwitch->Play(0, 0, 0);
                }
            }
            else
            {
                if (ggUniversal3Button.GetValue() == 0)
                {
                    dsbSwitch->SetVolume(DSBVOLUME_MAX);
                    dsbSwitch->Play(0, 0, 0);
                }
                /*
                 if (Console::Pressed(VK_CONTROL))
                  {//z [Ctrl] zapalamy albo gasimy œwiate³ko w kabinie
                   if (iCabLightFlag<2) ++iCabLightFlag; //zapalenie
                  }
                */
            }
        }
        else
            //-----------
            // hunter-091012: dzwiek dla przyciemnienia swiatelka w kabinie
            if (cKey == Global::Keys[k_Univ4])
        {
            if (Console::Pressed(VK_CONTROL))
            {
                if (bCabLightDim == false) //(ggCabLightDimButton.GetValue()==0)
                {
                    dsbSwitch->SetVolume(DSBVOLUME_MAX);
                    dsbSwitch->Play(0, 0, 0);
                }
            }
        }
        else
            //-----------
            if (cKey == Global::Keys[k_PantFrontUp])
        { // Winger 160204: podn. przedn. pantografu
            if (mvOccupied->ActiveCab ==
                1) //||((mvOccupied->ActiveCab<1)&&((mvControlled->TrainType&(dt_ET40|dt_ET41|dt_ET42|dt_EZT))==0)))
            { // przedni gdy w kabinie 1 lub (z wyj¹tkiem ET40, ET41, ET42 i EZT) gdy w kabinie -1
                mvControlled->PantFrontSP = false;
                if (mvControlled->PantFront(true))
                    if (mvControlled->PantFrontStart != 1)
                    {
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                    }
            }
            else
            // if
            // ((mvOccupied->ActiveCab<1)&&(mvControlled->TrainType&(dt_ET40|dt_ET41|dt_ET42|dt_EZT)))
            { // w kabinie -1 dla ET40, ET41, ET42 i EZT
                mvControlled->PantRearSP = false;
                if (mvControlled->PantRear(true))
                    if (mvControlled->PantRearStart != 1)
                    {
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                    }
            }
        }
        else if (cKey == Global::Keys[k_PantRearUp])
        { // Winger 160204: podn. tyln. pantografu wzglêdem kierunku jazdy
            if (mvOccupied->ActiveCab ==
                1) //||((mvOccupied->ActiveCab<1)&&((mvControlled->TrainType&(dt_ET40|dt_ET41|dt_ET42|dt_EZT))==0)))
            { // tylny gdy w kabinie 1 lub (z wyj¹tkiem ET40, ET41, ET42 i EZT) gdy w kabinie -1
                mvControlled->PantRearSP = false;
                if (mvControlled->PantRear(true))
                    if (mvControlled->PantRearStart != 1)
                    {
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                    }
            }
            else
            // if
            // ((mvOccupied->ActiveCab<1)&&(mvControlled->TrainType&(dt_ET40|dt_ET41|dt_ET42|dt_EZT)))
            { // przedni w kabinie -1 dla ET40, ET41, ET42 i EZT
                mvControlled->PantFrontSP = false;
                if (mvControlled->PantFront(true))
                    if (mvControlled->PantFrontStart != 1)
                    {
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                    }
            }
        }
        else if (cKey == Global::Keys[k_Active]) // yB 300407: przelacznik rozrzadu
        { // Ra 2014-06: uruchomi³em to, aby aktywowaæ czuwak w zajmowanym cz³onie, a wy³¹czyæ w
          // innych
            // Ra 2014-03: aktywacja czuwaka przepiêta na ustawienie kierunku w mvOccupied
            // if (mvControlled->Battery) //jeœli bateria jest ju¿ za³¹czona
            // mvOccupied->BatterySwitch(true); //to w ten oto durny sposób aktywuje siê CA/SHP
            //        if (mvControlled->CabActivisation())
            //           {
            //            dsbSwitch->SetVolume(DSBVOLUME_MAX);
            //            dsbSwitch->Play(0,0,0);
            //           }
        }
        else if (cKey == Global::Keys[k_Heating]) // Winger 020304: ogrzewanie skladu - wlaczenie
        { // Ra 2014-09: w trybie latania obs³uga jest w World.cpp
            if (!FreeFlyModeFlag)
            {
                if ((mvControlled->Heating == false) &&
                    ((mvControlled->EngineType == ElectricSeriesMotor) &&
                         (mvControlled->Mains == true) ||
                     (mvControlled->ConverterFlag)))
                {
                    mvControlled->Heating = true;
                    dsbSwitch->SetVolume(DSBVOLUME_MAX);
                    dsbSwitch->Play(0, 0, 0);
                }
            }
        }
        else
            // ABu 060205: dzielo Wingera po malutkim liftingu:
            if (cKey == Global::Keys[k_LeftSign]) // lewe swiatlo - w³¹czenie
        {
            if ((GetAsyncKeyState(VK_CONTROL) < 0) &&
                (ggRearLeftLightButton.SubModel)) // hunter-230112 - z controlem zapala z tylu
            {
                //------------------------------
                if (mvOccupied->ActiveCab == 1)
                { // kabina 1
                    if (((DynamicObject->iLights[1]) & 3) == 0)
                    {
                        DynamicObject->iLights[1] |= 1;
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        ggRearLeftLightButton.PutValue(1);
                    }
                    if (((DynamicObject->iLights[1]) & 3) == 2)
                    {
                        DynamicObject->iLights[1] &= (255 - 2);
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        if (ggRearLeftEndLightButton.SubModel)
                        {
                            ggRearLeftEndLightButton.PutValue(0);
                            ggRearLeftLightButton.PutValue(0);
                        }
                        else
                            ggRearLeftLightButton.PutValue(0);
                    }
                }
                else
                { // kabina -1
                    if (((DynamicObject->iLights[0]) & 3) == 0)
                    {
                        DynamicObject->iLights[0] |= 1;
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        ggRearLeftLightButton.PutValue(1);
                    }
                    if (((DynamicObject->iLights[0]) & 3) == 2)
                    {
                        DynamicObject->iLights[0] &= (255 - 2);
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        if (ggRearLeftEndLightButton.SubModel)
                        {
                            ggRearLeftEndLightButton.PutValue(0);
                            ggRearLeftLightButton.PutValue(0);
                        }
                        else
                            ggRearLeftLightButton.PutValue(0);
                    }
                }
                //----------------------
            }
            else
            {
                if (mvOccupied->ActiveCab == 1)
                { // kabina 1
                    if (((DynamicObject->iLights[0]) & 3) == 0)
                    {
                        DynamicObject->iLights[0] |= 1;
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        ggLeftLightButton.PutValue(1);
                    }
                    if (((DynamicObject->iLights[0]) & 3) == 2)
                    {
                        DynamicObject->iLights[0] &= (255 - 2);
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        if (ggLeftEndLightButton.SubModel)
                        {
                            ggLeftEndLightButton.PutValue(0);
                            ggLeftLightButton.PutValue(0);
                        }
                        else
                            ggLeftLightButton.PutValue(0);
                    }
                }
                else
                { // kabina -1
                    if (((DynamicObject->iLights[1]) & 3) == 0)
                    {
                        DynamicObject->iLights[1] |= 1;
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        ggLeftLightButton.PutValue(1);
                    }
                    if (((DynamicObject->iLights[1]) & 3) == 2)
                    {
                        DynamicObject->iLights[1] &= (255 - 2);
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        if (ggLeftEndLightButton.SubModel)
                        {
                            ggLeftEndLightButton.PutValue(0);
                            ggLeftLightButton.PutValue(0);
                        }
                        else
                            ggLeftLightButton.PutValue(0);
                    }
                }
            } //-----------
        }
        else if (cKey == Global::Keys[k_UpperSign]) // ABu 060205: œwiat³o górne - w³¹czenie
        {
            if ((GetAsyncKeyState(VK_CONTROL) < 0) &&
                (ggRearUpperLightButton.SubModel)) // hunter-230112 - z controlem zapala z tylu
            {
                //------------------------------
                if ((mvOccupied->ActiveCab) == 1)
                { // kabina 1
                    if (((DynamicObject->iLights[1]) & 12) == 0)
                    {
                        DynamicObject->iLights[1] |= 4;
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        ggRearUpperLightButton.PutValue(1);
                    }
                }
                else
                { // kabina -1
                    if (((DynamicObject->iLights[0]) & 12) == 0)
                    {
                        DynamicObject->iLights[0] |= 4;
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        ggRearUpperLightButton.PutValue(1);
                    }
                }
            } //------------------------------
            else
            {
                if ((mvOccupied->ActiveCab) == 1)
                { // kabina 1
                    if (((DynamicObject->iLights[0]) & 12) == 0)
                    {
                        DynamicObject->iLights[0] |= 4;
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        ggUpperLightButton.PutValue(1);
                    }
                }
                else
                { // kabina -1
                    if (((DynamicObject->iLights[1]) & 12) == 0)
                    {
                        DynamicObject->iLights[1] |= 4;
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        ggUpperLightButton.PutValue(1);
                    }
                }
            }
        }
        else if (cKey ==
                 Global::Keys[k_RightSign]) // Winger 070304: swiatla tylne (koncowki) - wlaczenie
        {
            if ((GetAsyncKeyState(VK_CONTROL) < 0) &&
                (ggRearRightLightButton.SubModel)) // hunter-230112 - z controlem zapala z tylu
            {
                //------------------------------
                if (mvOccupied->ActiveCab == 1)
                { // kabina 1
                    if (((DynamicObject->iLights[1]) & 48) == 0)
                    {
                        DynamicObject->iLights[1] |= 16;
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        ggRearRightLightButton.PutValue(1);
                    }
                    if (((DynamicObject->iLights[1]) & 48) == 32)
                    {
                        DynamicObject->iLights[1] &= (255 - 32);
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        if (ggRearRightEndLightButton.SubModel)
                        {
                            ggRearRightEndLightButton.PutValue(0);
                            ggRearRightLightButton.PutValue(0);
                        }
                        else
                            ggRearRightLightButton.PutValue(0);
                    }
                }
                else
                { // kabina -1
                    if (((DynamicObject->iLights[0]) & 48) == 0)
                    {
                        DynamicObject->iLights[0] |= 16;
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        ggRearRightLightButton.PutValue(1);
                    }
                    if (((DynamicObject->iLights[0]) & 48) == 32)
                    {
                        DynamicObject->iLights[0] &= (255 - 32);
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        if (ggRearRightEndLightButton.SubModel)
                        {
                            ggRearRightEndLightButton.PutValue(0);
                            ggRearRightLightButton.PutValue(0);
                        }
                        else
                            ggRearRightLightButton.PutValue(0);
                    }
                }
            } //------------------------------
            else
            {
                if (mvOccupied->ActiveCab == 1)
                { // kabina 1
                    if (((DynamicObject->iLights[0]) & 48) == 0)
                    {
                        DynamicObject->iLights[0] |= 16;
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        ggRightLightButton.PutValue(1);
                    }
                    if (((DynamicObject->iLights[0]) & 48) == 32)
                    {
                        DynamicObject->iLights[0] &= (255 - 32);
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        if (ggRightEndLightButton.SubModel)
                        {
                            ggRightEndLightButton.PutValue(0);
                            ggRightLightButton.PutValue(0);
                        }
                        else
                            ggRightLightButton.PutValue(0);
                    }
                }
                else
                { // kabina -1
                    if (((DynamicObject->iLights[1]) & 48) == 0)
                    {
                        DynamicObject->iLights[1] |= 16;
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        ggRightLightButton.PutValue(1);
                    }
                    if (((DynamicObject->iLights[1]) & 48) == 32)
                    {
                        DynamicObject->iLights[1] &= (255 - 32);
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        if (ggRightEndLightButton.SubModel)
                        {
                            ggRightEndLightButton.PutValue(0);
                            ggRightLightButton.PutValue(0);
                        }
                        else
                            ggRightLightButton.PutValue(0);
                    }
                }
            }
        }
    }
    else // McZapkie-240302 - klawisze bez shifta
    {
        if (cKey == Global::Keys[k_IncMainCtrl])
        {
            if (mvControlled->IncMainCtrl(1))
            {
                dsbNastawnikJazdy->SetCurrentPosition(0);
                dsbNastawnikJazdy->Play(0, 0, 0);
            }
        }
        else if (cKey == Global::Keys[k_DecMainCtrl])
            if (mvControlled->DecMainCtrl(1))
            {
                dsbNastawnikJazdy->SetCurrentPosition(0);
                dsbNastawnikJazdy->Play(0, 0, 0);
            }
            else
                ;
        else if (cKey == Global::Keys[k_IncScndCtrl])
            //        if (MoverParameters->ScndCtrlPos<MoverParameters->ScndCtrlPosNo)
            //         if (mvControlled->EnginePowerSource.SourceType==CurrentCollector)
            if (mvControlled->ShuntMode)
            {
                mvControlled->AnPos += (GetDeltaTime() / 0.85f);
                if (mvControlled->AnPos > 1)
                    mvControlled->AnPos = 1;
            }
            else if (mvControlled->IncScndCtrl(1))
            {
                if (dsbNastawnikBocz) // hunter-081211
                {
                    dsbNastawnikBocz->SetCurrentPosition(0);
                    dsbNastawnikBocz->Play(0, 0, 0);
                }
                else if (!dsbNastawnikBocz)
                {
                    dsbNastawnikJazdy->SetCurrentPosition(0);
                    dsbNastawnikJazdy->Play(0, 0, 0);
                }
            }
            else
                ;
        else if (cKey == Global::Keys[k_DecScndCtrl])
            //       if (mvControlled->EnginePowerSource.SourceType==CurrentCollector)
            if (mvControlled->ShuntMode)
            {
                mvControlled->AnPos -= (GetDeltaTime() / 0.55f);
                if (mvControlled->AnPos < 0)
                    mvControlled->AnPos = 0;
            }
            else

                if (mvControlled->DecScndCtrl(1))
            //        if (MoverParameters->ScndCtrlPos>0)
            {
                if (dsbNastawnikBocz) // hunter-081211
                {
                    dsbNastawnikBocz->SetCurrentPosition(0);
                    dsbNastawnikBocz->Play(0, 0, 0);
                }
                else if (!dsbNastawnikBocz)
                {
                    dsbNastawnikJazdy->SetCurrentPosition(0);
                    dsbNastawnikJazdy->Play(0, 0, 0);
                }
            }
            else
                ;
        else if (cKey == Global::Keys[k_IncLocalBrakeLevel])
        { // Ra 2014-09: w trybie latania obs³uga jest w World.cpp
            if (!FreeFlyModeFlag)
            {
                if (GetAsyncKeyState(VK_CONTROL) < 0)
                    if ((mvOccupied->LocalBrake == ManualBrake) || (mvOccupied->MBrake == true))
                    {
                        mvOccupied->IncManualBrakeLevel(1);
                    }
                    else
                        ;
                else if (mvOccupied->LocalBrake != ManualBrake)
                    mvOccupied->IncLocalBrakeLevel(1);
            }
        }
        else if (cKey == Global::Keys[k_DecLocalBrakeLevel])
        { // Ra 2014-06: wersja dla swobodnego latania przeniesiona do World.cpp
            if (!FreeFlyModeFlag)
            {
                if (GetAsyncKeyState(VK_CONTROL) < 0)
                    if ((mvOccupied->LocalBrake == ManualBrake) || (mvOccupied->MBrake == true))
                        mvOccupied->DecManualBrakeLevel(1);
                    else
                        ;
                else // Ra 1014-06: AI potrafi zahamowaæ pomocniczym mimo jego braku - odhamowaæ
                     // jakoœ trzeba
                    if ((mvOccupied->LocalBrake != ManualBrake) || mvOccupied->LocalBrakePos)
                    mvOccupied->DecLocalBrakeLevel(1);
            }
        }
        else if ((cKey == Global::Keys[k_IncBrakeLevel]) && (mvOccupied->BrakeHandle != FV4a))
            // if (mvOccupied->IncBrakeLevel())
            if (mvOccupied->BrakeLevelAdd(
                    Global::fBrakeStep)) // nieodpowiedni warunek; true, jeœli mo¿na dalej krêciæ
            {
                keybrakecount = 0;
                if ((isEztOer) && (mvOccupied->BrakeCtrlPos < 3))
                { // Ra: uzale¿niæ dŸwiêk od zmiany stanu EP, nie od klawisza
                    dsbPneumaticSwitch->SetVolume(-10);
                    dsbPneumaticSwitch->Play(0, 0, 0);
                }
            }
            else
                ;
        else if ((cKey == Global::Keys[k_DecBrakeLevel]) && (mvOccupied->BrakeHandle != FV4a))
        {
            // now¹ wersjê dostarczy³ ZiomalCl ("fixed looped sound in ezt when using NUM_9 key")
            if ((mvOccupied->BrakeCtrlPos > -1) || (keybrakecount > 1))
            {

                if ((isEztOer) && (mvControlled->Mains) && (mvOccupied->BrakeCtrlPos != -1))
                { // Ra: uzale¿niæ dŸwiêk od zmiany stanu EP, nie od klawisza
                    dsbPneumaticSwitch->SetVolume(-10);
                    dsbPneumaticSwitch->Play(0, 0, 0);
                }
                // mvOccupied->DecBrakeLevel();
                mvOccupied->BrakeLevelAdd(-Global::fBrakeStep);
            }
            else
                keybrakecount += 1;
            // koniec wersji dostarczonej przez ZiomalCl
            /* wersja poprzednia - ten pierwszy if ze œrednikiem nie dzia³a³ jak warunek
                     if ((mvOccupied->BrakeCtrlPos>-1)|| (keybrakecount>1))
                      {
                        if (mvOccupied->DecBrakeLevel());
                          {
                          if ((isEztOer) && (mvOccupied->BrakeCtrlPos<2)&&(keybrakecount<=1))
                          {
                           dsbPneumaticSwitch->SetVolume(-10);
                           dsbPneumaticSwitch->Play(0,0,0);
                          }
                         }
                      }
                       else keybrakecount+=1;
            */
        }
        else if (cKey == Global::Keys[k_EmergencyBrake])
        {
            // while (mvOccupied->IncBrakeLevel());
            mvOccupied->BrakeLevelSet(mvOccupied->Handle->GetPos(bh_EB));
            if (mvOccupied->BrakeCtrlPosNo <= 0.1) // hamulec bezpieczeñstwa dla wagonów
                mvOccupied->EmergencyBrakeFlag = true;
        }
        else if (cKey == Global::Keys[k_Brake3])
        {
            if ((isEztOer) && ((mvOccupied->BrakeCtrlPos == 1) || (mvOccupied->BrakeCtrlPos == -1)))
            {
                dsbPneumaticSwitch->SetVolume(-10);
                dsbPneumaticSwitch->Play(0, 0, 0);
            }
            // while (mvOccupied->BrakeCtrlPos>mvOccupied->BrakeCtrlPosNo-1 &&
            // mvOccupied->DecBrakeLevel());
            // while (mvOccupied->BrakeCtrlPos<mvOccupied->BrakeCtrlPosNo-1 &&
            // mvOccupied->IncBrakeLevel());
            mvOccupied->BrakeLevelSet(mvOccupied->BrakeCtrlPosNo - 1);
        }
        else if (cKey == Global::Keys[k_Brake2])
        {
            if ((isEztOer) && ((mvOccupied->BrakeCtrlPos == 1) || (mvOccupied->BrakeCtrlPos == -1)))
            {
                dsbPneumaticSwitch->SetVolume(-10);
                dsbPneumaticSwitch->Play(0, 0, 0);
            }
            // while (mvOccupied->BrakeCtrlPos>mvOccupied->BrakeCtrlPosNo/2 &&
            // mvOccupied->DecBrakeLevel());
            // while (mvOccupied->BrakeCtrlPos<mvOccupied->BrakeCtrlPosNo/2 &&
            // mvOccupied->IncBrakeLevel());
            mvOccupied->BrakeLevelSet(mvOccupied->BrakeCtrlPosNo / 2 +
                                      (mvOccupied->BrakeHandle == FV4a ? 1 : 0));
            if (GetAsyncKeyState(VK_CONTROL) < 0)
                mvOccupied->BrakeLevelSet(mvOccupied->Handle->GetPos(bh_NP)); // yB: czy ten stos
                                                                              // funkcji nie
                                                                              // powinien byæ jako
                                                                              // oddzielna funkcja
                                                                              // movera?
        }
        else if (cKey == Global::Keys[k_Brake1])
        {
            if ((isEztOer) && (mvOccupied->BrakeCtrlPos != 1))
            {
                dsbPneumaticSwitch->SetVolume(-10);
                dsbPneumaticSwitch->Play(0, 0, 0);
            }
            // while (mvOccupied->BrakeCtrlPos>1 && mvOccupied->DecBrakeLevel());
            // while (mvOccupied->BrakeCtrlPos<1 && mvOccupied->IncBrakeLevel());
            mvOccupied->BrakeLevelSet(1);
        }
        else if (cKey == Global::Keys[k_Brake0])
        {
            if (Console::Pressed(VK_CONTROL))
            {
                mvOccupied->BrakeCtrlPos2 = 0; // wyrownaj kapturek
            }
            else
            {
                if ((isEztOer) &&
                    ((mvOccupied->BrakeCtrlPos == 1) || (mvOccupied->BrakeCtrlPos == -1)))
                {
                    dsbPneumaticSwitch->SetVolume(-10);
                    dsbPneumaticSwitch->Play(0, 0, 0);
                }
                // while (mvOccupied->BrakeCtrlPos>0 && mvOccupied->DecBrakeLevel());
                // while (mvOccupied->BrakeCtrlPos<0 && mvOccupied->IncBrakeLevel());
                mvOccupied->BrakeLevelSet(0);
            }
        }
        else if (cKey == Global::Keys[k_WaveBrake]) //[Num.]
        {
            if ((isEztOer) && (mvControlled->Mains) && (mvOccupied->BrakeCtrlPos != -1))
            {
                dsbPneumaticSwitch->SetVolume(-10);
                dsbPneumaticSwitch->Play(0, 0, 0);
            }
            // while (mvOccupied->BrakeCtrlPos>-1 && mvOccupied->DecBrakeLevel());
            // while (mvOccupied->BrakeCtrlPos<-1 && mvOccupied->IncBrakeLevel());
            mvOccupied->BrakeLevelSet(-1);
        }
        else
            //---------------
            // hunter-131211: zbicie czuwaka przeniesione do TTrain::Update()
            if (cKey == Global::Keys[k_Czuwak])
        { // Ra: tu zosta³ tylko dŸwiêk
            // dsbBuzzer->Stop();
            // if (mvOccupied->SecuritySystemReset())
            if (fabs(ggSecurityResetButton.GetValue()) < 0.001)
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play(0, 0, 0);
            }
            // ggSecurityResetButton.PutValue(1);
        }
        else
            //---------------
            // hunter-221211: hamulec przeciwposlizgowy przeniesiony do TTrain::Update()
            if (cKey == Global::Keys[k_AntiSlipping])
        {
            if (mvOccupied->BrakeSystem != ElectroPneumatic)
            {
                // if (mvControlled->AntiSlippingButton())
                if (fabs(ggAntiSlipButton.GetValue()) < 0.001)
                {
                    // Dlaczego bylo '-50'???
                    // dsbSwitch->SetVolume(-50);
                    dsbSwitch->SetVolume(DSBVOLUME_MAX);
                    dsbSwitch->Play(0, 0, 0);
                }
                // ggAntiSlipButton.PutValue(1);
            }
        }
        else
            //---------------

            if (cKey == Global::Keys[k_Fuse])
        {
            if (GetAsyncKeyState(VK_CONTROL) < 0) // z controlem
            {
                ggConverterFuseButton.PutValue(1); // hunter-261211
                if ((mvControlled->Mains == false) && (ggConverterButton.GetValue() == 0))
                    mvControlled->ConvOvldFlag = false;
            }
            else
            {
                ggFuseButton.PutValue(1);
                mvControlled->FuseOn();
            }
        }
        else
            // McZapkie-240302 - zmiana kierunku: 'd' do przodu, 'r' do tylu
            if (cKey == Global::Keys[k_DirectionForward])
        {
            if (mvOccupied->DirectionForward())
            {
                //------------
                // hunter-121211: dzwiek kierunkowego
                if (dsbReverserKey)
                {
                    dsbReverserKey->SetCurrentPosition(0);
                    dsbReverserKey->SetVolume(DSBVOLUME_MAX);
                    dsbReverserKey->Play(0, 0, 0);
                }
                else if (!dsbReverserKey)
                    if (dsbSwitch)
                    {
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                    }
                //------------
                if (mvOccupied->ActiveDir) // jeœli kierunek niezerowy
                    if (DynamicObject->Mechanik) // na wszelki wypadek
                        DynamicObject->Mechanik->CheckVehicles(
                            Change_direction); // aktualizacja skrajnych pojazdów w sk³adzie
            }
        }
        else if (cKey == Global::Keys[k_DirectionBackward]) // r
        {
            if (GetAsyncKeyState(VK_CONTROL) < 0)
            { // wciœniêty [Ctrl]
                if (mvControlled->Radio == true)
                {
                    dsbSwitch->SetVolume(DSBVOLUME_MAX);
                    dsbSwitch->Play(0, 0, 0);
                    mvControlled->Radio = false;
                }
            }
            else if (mvOccupied->DirectionBackward())
            {
                //------------
                // hunter-121211: dzwiek kierunkowego
                if (dsbReverserKey)
                {
                    dsbReverserKey->SetCurrentPosition(0);
                    dsbReverserKey->SetVolume(DSBVOLUME_MAX);
                    dsbReverserKey->Play(0, 0, 0);
                }
                else if (!dsbReverserKey)
                    if (dsbSwitch)
                    {
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                    }
                //------------
                if (mvOccupied->ActiveDir) // jeœli kierunek niezerowy
                    if (DynamicObject->Mechanik) // na wszelki wypadek
                        DynamicObject->Mechanik->CheckVehicles(
                            Change_direction); // aktualizacja skrajnych pojazdów w sk³adzie
            }
        }
        else
            // McZapkie-240302 - wylaczanie glownego obwodu
            //-----------
            // hunter-141211: wyl. szybki wylaczony przeniesiony do TTrain::Update()
            if (cKey == Global::Keys[k_Main])
        {
            if (fabs(ggMainOffButton.GetValue()) < 0.001)
                if (dsbSwitch)
                {
                    dsbSwitch->SetVolume(DSBVOLUME_MAX);
                    dsbSwitch->Play(0, 0, 0);
                }
        }
        else

            if (cKey == Global::Keys[k_Battery])
        {
            // if ((mvControlled->TrainType==dt_EZT) ||
            // (mvControlled->EngineType==ElectricSeriesMotor)||
            // (mvControlled->EngineType==DieselElectric))
            if (mvOccupied->BatterySwitch(false))
            { // ewentualnie zablokowaæ z FIZ, np. w samochodach siê nie od³¹cza akumulatora
                dsbSwitch->Play(0, 0, 0);
                // mvOccupied->SecuritySystem.Status=0;
                mvControlled->PantFront(false);
                mvControlled->PantRear(false);
            }
        }

        //-----------
        //      if (cKey==Global::Keys[k_Active])   //yB 300407: przelacznik rozrzadu
        //      {
        //        if (mvControlled->CabDeactivisation())
        //           {
        //            dsbSwitch->SetVolume(DSBVOLUME_MAX);
        //            dsbSwitch->Play(0,0,0);
        //           }
        //      }
        //      else
        if (cKey == Global::Keys[k_BrakeProfile])
        { // yB://ABu: male poprawki, zeby bylo mozna ustawic dowolny wagon
            int CouplNr = -2;
            if (!FreeFlyModeFlag)
            {
                if (GetAsyncKeyState(VK_CONTROL) < 0)
                    if (mvOccupied->BrakeDelaySwitch(bdelay_R))
                    {
                        dsbPneumaticRelay->SetVolume(DSBVOLUME_MAX);
                        dsbPneumaticRelay->Play(0, 0, 0);
                    }
                    else
                        ;
                else if (mvOccupied->BrakeDelaySwitch(bdelay_G))
                {
                    dsbPneumaticRelay->SetVolume(DSBVOLUME_MAX);
                    dsbPneumaticRelay->Play(0, 0, 0);
                }
            }
            else
            {
                TDynamicObject *temp;
                temp = (DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(), -1, 1500,
                                                            CouplNr));
                if (temp == NULL)
                {
                    CouplNr = -2;
                    temp = (DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(), 1, 1500,
                                                                CouplNr));
                }
                if (temp)
                {
                    if (GetAsyncKeyState(VK_CONTROL) < 0)
                        if (temp->MoverParameters->BrakeDelaySwitch(bdelay_R))
                        {
                            dsbPneumaticRelay->SetVolume(DSBVOLUME_MAX);
                            dsbPneumaticRelay->Play(0, 0, 0);
                        }
                        else
                            ;
                    else if (temp->MoverParameters->BrakeDelaySwitch(bdelay_G))
                    {
                        dsbPneumaticRelay->SetVolume(DSBVOLUME_MAX);
                        dsbPneumaticRelay->Play(0, 0, 0);
                    }
                }
            }
        }
        else
            //-----------
            // hunter-261211: przetwornica i sprzezarka przeniesione do TTrain::Update()
            if (cKey == Global::Keys[k_Converter])
        {
            if (ggConverterButton.GetValue() != 0)
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play(0, 0, 0);
            }
        }
        else
            // if
            // ((cKey==Global::Keys[k_Compressor])&&((mvControlled->EngineType==ElectricSeriesMotor)||(mvControlled->TrainType==dt_EZT)))
            // //hunter-110212: poprawka dla EZT
            if ((cKey == Global::Keys[k_Compressor]) &&
                (mvControlled->CompressorPower < 2)) // hunter-091012: tak jest poprawnie
        {
            if (ggCompressorButton.GetValue() != 0)
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play(0, 0, 0);
            }
        }
        else
            //-----------
            if (cKey == Global::Keys[k_Releaser]) // odluzniacz
        {
            if (!FreeFlyModeFlag)
            {
                if ((mvControlled->EngineType == ElectricSeriesMotor) ||
                    (mvControlled->EngineType == DieselElectric) ||
                    (mvControlled->EngineType == ElectricInductionMotor))
                    if (mvControlled->TrainType != dt_EZT)
                        if (mvOccupied->BrakeCtrlPosNo > 0)
                        {
                            ggReleaserButton.PutValue(1);
                            if (mvOccupied->BrakeReleaser(1))
                            {
                                dsbPneumaticRelay->SetVolume(-80);
                                dsbPneumaticRelay->Play(0, 0, 0);
                            }
                        }
            }
        }
        else if (cKey == Global::Keys[k_SmallCompressor]) // Winger 160404: mala sprezarka wl
        { // Ra: bez [Shift] te¿ daæ dŸwiêk
            if ((mvControlled->TrainType & dt_EZT) ? mvControlled == mvOccupied :
                                                     !mvOccupied->ActiveCab) // tylko w maszynowym
                if (Console::Pressed(VK_CONTROL)) // z [Ctrl]
                    mvControlled->bPantKurek3 = false; // zbiornik pantografu po³¹czony jest z ma³¹
                                                       // sprê¿ark¹ (pompowanie ma sens, ale potem
                                                       // trzeba prze³¹czyæ)
                else if (!mvControlled->PantCompFlag) // jeœli wy³¹czona
                    if (mvControlled->Battery) // jeszcze musi byæ za³¹czona bateria
                        if (mvControlled->PantPress < 4.8) // pisz¹, ¿e to tak nie dzia³a
                        {
                            mvControlled->PantCompFlag = true;
                            dsbSwitch->SetVolume(DSBVOLUME_MAX);
                            dsbSwitch->Play(0, 0, 0); // dŸwiêk tylko po naciœniêciu klawisza
                        }
        }
        // McZapkie-240302 - wylaczanie automatycznego pilota (w trybie ~debugmode mozna tylko raz)
        else if (cKey == VkKeyScan('q')) // bez Shift
        {
            if (DynamicObject->Mechanik)
                DynamicObject->Mechanik->TakeControl(false);
        }
        else if (cKey == Global::Keys[k_MaxCurrent]) // McZapkie-160502: f - niski rozruch
        {
            if ((mvControlled->EngineType == DieselElectric) && (mvControlled->ShuntModeAllow) &&
                (mvControlled->MainCtrlPos == 0))
            {
                mvControlled->ShuntMode = false;
            }
            if (mvControlled->CurrentSwitch(false))
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play(0, 0, 0);
            }
            /* Ra: przeniesione do Mover.cpp
                   if (mvControlled->TrainType!=dt_EZT)
                    if (mvControlled->MinCurrentSwitch(false))
                    {
                     dsbSwitch->SetVolume(DSBVOLUME_MAX);
                     dsbSwitch->Play(0,0,0);
                    }
            */
        }
        else if (cKey == Global::Keys[k_CurrentAutoRelay]) // McZapkie-241002: g - wylaczanie PSR
        {
            if (mvControlled->AutoRelaySwitch(false))
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play(0, 0, 0);
            }
        }
        else
            // hunter-201211: piasecznica poprawiona oraz przeniesiona do TTrain::Update()
            if (cKey == Global::Keys[k_Sand])
        {
            /*
              if (mvControlled->TrainType!=dt_EZT)
              {
                if (mvControlled->SandDoseOn())
                 if (mvControlled->SandDose)
                  {
                    dsbPneumaticRelay->SetVolume(-30);
                    dsbPneumaticRelay->Play(0,0,0);
                  }
              }
            */
            if (mvControlled->TrainType == dt_EZT)
            {
                if (mvControlled->DoorSignalling)
                {
                    dsbSwitch->Play(0, 0, 0);
                    mvControlled->DoorSignalling = false;
                }
            }
        }
        else if (cKey == Global::Keys[k_CabForward])
        {
            if (!CabChange(1))
                if (TestFlag(DynamicObject->MoverParameters->Couplers[0].CouplingFlag,
                             ctrain_passenger))
                { // przejscie do nastepnego pojazdu
                    Global::changeDynObj = DynamicObject->PrevConnected;
                    Global::changeDynObj->MoverParameters->ActiveCab =
                        DynamicObject->PrevConnectedNo ? -1 : 1;
                }
            if (DynamicObject->MoverParameters->ActiveCab)
                mvControlled->PantCompFlag = false; // wyjœcie z maszynowego wy³¹cza sprê¿arkê
        }
        else if (cKey == Global::Keys[k_CabBackward])
        {
            if (!CabChange(-1))
                if (TestFlag(DynamicObject->MoverParameters->Couplers[1].CouplingFlag,
                             ctrain_passenger))
                { // przejscie do poprzedniego
                    Global::changeDynObj = DynamicObject->NextConnected;
                    Global::changeDynObj->MoverParameters->ActiveCab =
                        DynamicObject->NextConnectedNo ? -1 : 1;
                }
            if (DynamicObject->MoverParameters->ActiveCab)
                mvControlled->PantCompFlag =
                    false; // wyjœcie z maszynowego wy³¹cza sprê¿arkê pomocnicz¹
        }
        else if (cKey == Global::Keys[k_Couple])
        { // ABu051104: male zmiany, zeby mozna bylo laczyc odlegle wagony
            // da sie zoptymalizowac, ale nie ma na to czasu :(
            if (iCabn > 0)
            {
                if (!FreeFlyModeFlag) // tryb 'kabinowy'
                { /*
                   if (mvControlled->Couplers[iCabn-1].CouplingFlag==0)
                   {
                     if
                   (mvControlled->Attach(iCabn-1,mvControlled->Couplers[iCabn-1].Connected,ctrain_coupler))
                     {
                       dsbCouplerAttach->SetVolume(DSBVOLUME_MAX);
                       dsbCouplerAttach->Play(0,0,0);
                       //ABu: aha, a guzik, nie dziala i nie bedzie, a przydalo by sie cos takiego:
                       //DynamicObject->NextConnected=mvControlled->Couplers[iCabn-1].Connected;
                       //DynamicObject->PrevConnected=mvControlled->Couplers[iCabn-1].Connected;
                     }
                   }
                   else
                   if (!TestFlag(mvControlled->Couplers[iCabn-1].CouplingFlag,ctrain_pneumatic))
                   {
                     //ABu021104: zeby caly czas bylo widac sprzegi:
                     if
                   (mvControlled->Attach(iCabn-1,mvControlled->Couplers[iCabn-1].Connected,mvControlled->Couplers[iCabn-1].CouplingFlag+ctrain_pneumatic))
                     //if
                   (mvControlled->Attach(iCabn-1,mvControlled->Couplers[iCabn-1].Connected,ctrain_pneumatic))
                     {
                       rsHiss.Play(1,DSBPLAY_LOOPING,true,DynamicObject->GetPosition());
                     }
                   }*/
                }
                else
                { // tryb freefly
                    int CouplNr = -1; // normalnie ¿aden ze sprzêgów
                    TDynamicObject *tmp;
                    tmp = DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(), 1, 1500,
                                                              CouplNr);
                    if (tmp == NULL)
                        tmp = DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(), -1,
                                                                  1500, CouplNr);
                    if (tmp && (CouplNr != -1))
                    {
                        if (tmp->MoverParameters->Couplers[CouplNr].CouplingFlag ==
                            0) // najpierw hak
                        {
                            if ((tmp->MoverParameters->Couplers[CouplNr]
                                     .Connected->Couplers[CouplNr]
                                     .AllowedFlag &
                                 tmp->MoverParameters->Couplers[CouplNr].AllowedFlag &
                                 ctrain_coupler) == ctrain_coupler)
                                if (tmp->MoverParameters->Attach(
                                        CouplNr, 2,
                                        tmp->MoverParameters->Couplers[CouplNr].Connected,
                                        ctrain_coupler))
                                {
                                    // tmp->MoverParameters->Couplers[CouplNr].Render=true;
                                    // //pod³¹czony sprzêg bêdzie widoczny
                                    if (DynamicObject->Mechanik) // na wszelki wypadek
                                        DynamicObject->Mechanik->CheckVehicles(
                                            Connect); // aktualizacja flag kierunku w sk³adzie
                                    dsbCouplerAttach->SetVolume(DSBVOLUME_MAX);
                                    dsbCouplerAttach->Play(0, 0, 0);
                                }
                                else
                                    WriteLog("Mechanical coupling failed.");
                        }
                        else if (!TestFlag(tmp->MoverParameters->Couplers[CouplNr].CouplingFlag,
                                           ctrain_pneumatic)) // pneumatyka
                        {
                            if ((tmp->MoverParameters->Couplers[CouplNr]
                                     .Connected->Couplers[CouplNr]
                                     .AllowedFlag &
                                 tmp->MoverParameters->Couplers[CouplNr].AllowedFlag &
                                 ctrain_pneumatic) == ctrain_pneumatic)
                                if (tmp->MoverParameters->Attach(
                                        CouplNr, 2,
                                        tmp->MoverParameters->Couplers[CouplNr].Connected,
                                        tmp->MoverParameters->Couplers[CouplNr].CouplingFlag +
                                            ctrain_pneumatic))
                                {
                                    rsHiss.Play(1, DSBPLAY_LOOPING, true, tmp->GetPosition());
                                    DynamicObject->SetPneumatic(CouplNr,
                                                                1); // Ra: to mi siê nie podoba !!!!
                                    tmp->SetPneumatic(CouplNr, 1);
                                }
                        }
                        else if (!TestFlag(tmp->MoverParameters->Couplers[CouplNr].CouplingFlag,
                                           ctrain_scndpneumatic)) // zasilajacy
                        {
                            if ((tmp->MoverParameters->Couplers[CouplNr]
                                     .Connected->Couplers[CouplNr]
                                     .AllowedFlag &
                                 tmp->MoverParameters->Couplers[CouplNr].AllowedFlag &
                                 ctrain_scndpneumatic) == ctrain_scndpneumatic)
                                if (tmp->MoverParameters->Attach(
                                        CouplNr, 2,
                                        tmp->MoverParameters->Couplers[CouplNr].Connected,
                                        tmp->MoverParameters->Couplers[CouplNr].CouplingFlag +
                                            ctrain_scndpneumatic))
                                {
                                    //              rsHiss.Play(1,DSBPLAY_LOOPING,true,tmp->GetPosition());
                                    dsbCouplerDetach->SetVolume(DSBVOLUME_MAX);
                                    dsbCouplerDetach->Play(0, 0, 0);
                                    DynamicObject->SetPneumatic(CouplNr,
                                                                0); // Ra: to mi siê nie podoba !!!!
                                    tmp->SetPneumatic(CouplNr, 0);
                                }
                        }
                        else if (!TestFlag(tmp->MoverParameters->Couplers[CouplNr].CouplingFlag,
                                           ctrain_controll)) // ukrotnionko
                        {
                            if ((tmp->MoverParameters->Couplers[CouplNr]
                                     .Connected->Couplers[CouplNr]
                                     .AllowedFlag &
                                 tmp->MoverParameters->Couplers[CouplNr].AllowedFlag &
                                 ctrain_controll) == ctrain_controll)
                                if (tmp->MoverParameters->Attach(
                                        CouplNr, 2,
                                        tmp->MoverParameters->Couplers[CouplNr].Connected,
                                        tmp->MoverParameters->Couplers[CouplNr].CouplingFlag +
                                            ctrain_controll))
                                {
                                    dsbCouplerAttach->SetVolume(DSBVOLUME_MAX);
                                    dsbCouplerAttach->Play(0, 0, 0);
                                }
                        }
                        else if (!TestFlag(tmp->MoverParameters->Couplers[CouplNr].CouplingFlag,
                                           ctrain_passenger)) // mostek
                        {
                            if ((tmp->MoverParameters->Couplers[CouplNr]
                                     .Connected->Couplers[CouplNr]
                                     .AllowedFlag &
                                 tmp->MoverParameters->Couplers[CouplNr].AllowedFlag &
                                 ctrain_passenger) == ctrain_passenger)
                                if (tmp->MoverParameters->Attach(
                                        CouplNr, 2,
                                        tmp->MoverParameters->Couplers[CouplNr].Connected,
                                        tmp->MoverParameters->Couplers[CouplNr].CouplingFlag +
                                            ctrain_passenger))
                                {
                                    //                  rsHiss.Play(1,DSBPLAY_LOOPING,true,tmp->GetPosition());
                                    dsbCouplerDetach->SetVolume(DSBVOLUME_MAX);
                                    dsbCouplerDetach->Play(0, 0, 0);
                                    DynamicObject->SetPneumatic(CouplNr, 0);
                                    tmp->SetPneumatic(CouplNr, 0);
                                }
                        }
                    }
                }
            }
        }
        else if (cKey == Global::Keys[k_DeCouple])
        { // ABu051104: male zmiany, zeby mozna bylo rozlaczac odlegle wagony
            if (iCabn > 0)
            {
                if (!FreeFlyModeFlag) // tryb 'kabinowy' (pozwala równie¿ roz³¹czyæ sprzêgi
                                      // zablokowane)
                {
                    if (DynamicObject->DettachStatus(iCabn - 1) < 0) // jeœli jest co odczepiæ
                        if (DynamicObject->Dettach(iCabn - 1)) // iCab==1:przód,iCab==2:ty³
                        {
                            dsbCouplerDetach->SetVolume(DSBVOLUME_MAX); // w kabinie ten dŸwiêk?
                            dsbCouplerDetach->Play(0, 0, 0);
                        }
                }
                else
                { // tryb freefly
                    int CouplNr = -1;
                    TDynamicObject *tmp;
                    tmp = DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(), 1, 1500,
                                                              CouplNr);
                    if (tmp == NULL)
                        tmp = DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(), -1,
                                                                  1500, CouplNr);
                    if (tmp && (CouplNr != -1))
                    {
                        if ((tmp->MoverParameters->Couplers[CouplNr].CouplingFlag & ctrain_depot) ==
                            0) // je¿eli sprzêg niezablokowany
                            if (tmp->DettachStatus(CouplNr) < 0) // jeœli jest co odczepiæ i siê da
                                if (!tmp->Dettach(CouplNr))
                                { // dŸwiêk odczepiania
                                    dsbCouplerDetach->SetVolume(DSBVOLUME_MAX);
                                    dsbCouplerDetach->Play(0, 0, 0);
                                }
                    }
                }
                if (DynamicObject->Mechanik) // na wszelki wypadek
                    DynamicObject->Mechanik->CheckVehicles(
                        Disconnect); // aktualizacja skrajnych pojazdów w sk³adzie
            }
        }
        else if (cKey == Global::Keys[k_CloseLeft]) // NBMX 17-09-2003: zamykanie drzwi
        {
            if (mvOccupied->CabNo < 0 ? mvOccupied->DoorRight(false) : mvOccupied->DoorLeft(false))
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play(0, 0, 0);
                if (dsbDoorClose)
                {
                    dsbDoorClose->SetCurrentPosition(0);
                    dsbDoorClose->Play(0, 0, 0);
                }
            }
        }
        else if (cKey == Global::Keys[k_CloseRight]) // NBMX 17-09-2003: zamykanie drzwi
        {
            if (mvOccupied->CabNo < 0 ? mvOccupied->DoorLeft(false) : mvOccupied->DoorRight(false))
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play(0, 0, 0);
                if (dsbDoorClose)
                {
                    dsbDoorClose->SetCurrentPosition(0);
                    dsbDoorClose->Play(0, 0, 0);
                }
            }
        }
        else
            //-----------
            // hunter-131211: dzwiek dla przelacznika universala
            // hunter-091012: ubajerowanie swiatla w kabinie (wyrzucenie przyciemnienia pod Univ4)
            if (cKey == Global::Keys[k_Univ3])
        {
            if (Console::Pressed(VK_CONTROL))
            {
                if (bCabLight == true) //(ggCabLightButton.GetValue()!=0)
                {
                    dsbSwitch->SetVolume(DSBVOLUME_MAX);
                    dsbSwitch->Play(0, 0, 0);
                }
            }
            else
            {
                if (ggUniversal3Button.GetValue() != 0)
                {
                    dsbSwitch->SetVolume(DSBVOLUME_MAX);
                    dsbSwitch->Play(0, 0, 0);
                }
                /*
                 if (Console::Pressed(VK_CONTROL))
                  {//z [Ctrl] zapalamy albo gasimy œwiate³ko w kabinie
                   if (iCabLightFlag) --iCabLightFlag; //gaszenie
                  } */
            }
        }
        else
            //-----------
            // hunter-091012: dzwiek dla przyciemnienia swiatelka w kabinie
            if (cKey == Global::Keys[k_Univ4])
        {
            if (Console::Pressed(VK_CONTROL))
            {
                if (bCabLightDim == true) //(ggCabLightDimButton.GetValue()!=0)
                {
                    dsbSwitch->SetVolume(DSBVOLUME_MAX);
                    dsbSwitch->Play(0, 0, 0);
                }
            }
        }
        //-----------
        else if (cKey == Global::Keys[k_PantFrontDown]) // Winger 160204: opuszczanie prz. patyka
        {
            if (mvOccupied->ActiveCab ==
                1) //||((mvOccupied->ActiveCab<1)&&(mvControlled->TrainType!=dt_ET40)&&(mvControlled->TrainType!=dt_ET41)&&(mvControlled->TrainType!=dt_ET42)&&(mvControlled->TrainType!=dt_EZT)))
            {
                // if (!mvControlled->PantFrontUp) //jeœli by³ opuszczony
                // if () //jeœli po³amany
                //  //to powtórzone opuszczanie naprawia
                if (mvControlled->PantFront(false))
                {
                    dsbSwitch->SetVolume(DSBVOLUME_MAX);
                    dsbSwitch->Play(0, 0, 0);
                }
            }
            else
            // if
            // ((mvOccupied->ActiveCab<1)&&((mvControlled->TrainType==dt_ET40)||(mvControlled->TrainType==dt_ET41)||(mvControlled->TrainType==dt_ET42)||(mvControlled->TrainType==dt_EZT)))
            {
                if (mvControlled->PantRear(false))
                {
                    dsbSwitch->SetVolume(DSBVOLUME_MAX);
                    dsbSwitch->Play(0, 0, 0);
                }
            }
        }
        else if (cKey == Global::Keys[k_PantRearDown]) // Winger 160204: opuszczanie tyl. patyka
        {
            if (mvOccupied->ActiveCab ==
                1) //||((mvOccupied->ActiveCab<1)&&(mvControlled->TrainType!=dt_ET40)&&(mvControlled->TrainType!=dt_ET41)&&(mvControlled->TrainType!=dt_ET42)&&(mvControlled->TrainType!=dt_EZT)))
            {
                if (mvControlled->PantSwitchType == "impulse")
                    ggPantFrontButtonOff.PutValue(1);
                if (mvControlled->PantRear(false))
                {
                    dsbSwitch->SetVolume(DSBVOLUME_MAX);
                    dsbSwitch->Play(0, 0, 0);
                }
            }
            else
            // if
            // ((mvOccupied->ActiveCab<1)&&((mvControlled->TrainType==dt_ET40)||(mvControlled->TrainType==dt_ET41)||(mvControlled->TrainType==dt_ET42)||(mvControlled->TrainType==dt_EZT)))
            {
                /* if (mvControlled->PantSwitchType=="impulse")
                ggPantRearButtonOff.PutValue(1);  */
                if (mvControlled->PantFront(false))
                {
                    dsbSwitch->SetVolume(DSBVOLUME_MAX);
                    dsbSwitch->Play(0, 0, 0);
                }
            }
        }
        else if (cKey == Global::Keys[k_Heating]) // Winger 020304: ogrzewanie - wylaczenie
        { // Ra 2014-09: w trybie latania obs³uga jest w World.cpp
            if (!FreeFlyModeFlag)
            {
                if (mvControlled->Heating == true)
                {
                    dsbSwitch->SetVolume(DSBVOLUME_MAX);
                    dsbSwitch->Play(0, 0, 0);
                    mvControlled->Heating = false;
                }
            }
        }
        else if (cKey == Global::Keys[k_LeftSign]) // ABu 060205: lewe swiatlo - wylaczenie
        {
            if ((GetAsyncKeyState(VK_CONTROL) < 0) &&
                (ggRearLeftLightButton.SubModel)) // hunter-230112 - z controlem gasi z tylu
            {
                //------------------------------
                if (mvOccupied->ActiveCab == 1)
                { // kabina 1
                    if (((DynamicObject->iLights[1]) & 3) == 0)
                    {
                        DynamicObject->iLights[1] |= 2;
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        if (ggRearLeftEndLightButton.SubModel)
                        {
                            ggRearLeftEndLightButton.PutValue(1);
                            ggRearLeftLightButton.PutValue(0);
                        }
                        else
                            ggRearLeftLightButton.PutValue(-1);
                    }
                    if (((DynamicObject->iLights[1]) & 3) == 1)
                    {
                        DynamicObject->iLights[1] &= (255 - 1);
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        ggRearLeftLightButton.PutValue(0);
                    }
                }
                else
                { // kabina -1
                    if (((DynamicObject->iLights[0]) & 3) == 0)
                    {
                        DynamicObject->iLights[0] |= 2;
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        if (ggRearLeftEndLightButton.SubModel)
                        {
                            ggRearLeftEndLightButton.PutValue(1);
                            ggRearLeftLightButton.PutValue(0);
                        }
                        else
                            ggRearLeftLightButton.PutValue(-1);
                    }
                    if (((DynamicObject->iLights[1]) & 3) == 1)
                    {
                        DynamicObject->iLights[1] &= (255 - 1);
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        ggLeftLightButton.PutValue(0);
                    }
                }
            } //------------------------------
            else
            {
                if (mvOccupied->ActiveCab == 1)
                { // kabina 1
                    if (((DynamicObject->iLights[0]) & 3) == 0)
                    {
                        DynamicObject->iLights[0] |= 2;
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        if (ggLeftEndLightButton.SubModel)
                        {
                            ggLeftEndLightButton.PutValue(1);
                            ggLeftLightButton.PutValue(0);
                        }
                        else
                            ggLeftLightButton.PutValue(-1);
                    }
                    if (((DynamicObject->iLights[0]) & 3) == 1)
                    {
                        DynamicObject->iLights[0] &= (255 - 1);
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        ggLeftLightButton.PutValue(0);
                    }
                }
                else
                { // kabina -1
                    if (((DynamicObject->iLights[1]) & 3) == 0)
                    {
                        DynamicObject->iLights[1] |= 2;
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        if (ggLeftEndLightButton.SubModel)
                        {
                            ggLeftEndLightButton.PutValue(1);
                            ggLeftLightButton.PutValue(0);
                        }
                        else
                            ggLeftLightButton.PutValue(-1);
                    }
                    if (((DynamicObject->iLights[1]) & 3) == 1)
                    {
                        DynamicObject->iLights[1] &= (255 - 1);
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        ggLeftLightButton.PutValue(0);
                    }
                }
            }
        }
        else if (cKey == Global::Keys[k_UpperSign]) // ABu 060205: œwiat³o górne - wy³¹czenie
        {
            if ((GetAsyncKeyState(VK_CONTROL) < 0) &&
                (ggRearUpperLightButton.SubModel)) // hunter-230112 - z controlem gasi z tylu
            {
                //------------------------------
                if (mvOccupied->ActiveCab == 1)
                { // kabina 1
                    if (((DynamicObject->iLights[1]) & 12) == 4)
                    {
                        DynamicObject->iLights[1] &= (255 - 4);
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        ggRearUpperLightButton.PutValue(0);
                    }
                }
                else
                { // kabina -1
                    if (((DynamicObject->iLights[0]) & 12) == 4)
                    {
                        DynamicObject->iLights[0] &= (255 - 4);
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        ggRearUpperLightButton.PutValue(0);
                    }
                }
            } //------------------------------
            else
            {
                if (mvOccupied->ActiveCab == 1)
                { // kabina 1
                    if (((DynamicObject->iLights[0]) & 12) == 4)
                    {
                        DynamicObject->iLights[0] &= (255 - 4);
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        ggUpperLightButton.PutValue(0);
                    }
                }
                else
                { // kabina -1
                    if (((DynamicObject->iLights[1]) & 12) == 4)
                    {
                        DynamicObject->iLights[1] &= (255 - 4);
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        ggUpperLightButton.PutValue(0);
                    }
                }
            }
        }
        if (cKey == Global::Keys[k_RightSign]) // Winger 070304: swiatla tylne (koncowki) -
                                               // wlaczenie
        {
            if ((GetAsyncKeyState(VK_CONTROL) < 0) &&
                (ggRearRightLightButton.SubModel)) // hunter-230112 - z controlem gasi z tylu
            {
                //------------------------------
                if (mvOccupied->ActiveCab == 1)
                { // kabina 1 (od strony 0)
                    if (((DynamicObject->iLights[1]) & 48) == 0)
                    {
                        DynamicObject->iLights[1] |= 32;
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        if (ggRearRightEndLightButton.SubModel)
                        {
                            ggRearRightEndLightButton.PutValue(1);
                            ggRearRightLightButton.PutValue(0);
                        }
                        else
                            ggRearRightLightButton.PutValue(-1);
                    }
                    if (((DynamicObject->iLights[1]) & 48) == 16)
                    {
                        DynamicObject->iLights[1] &= (255 - 16);
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        ggRearRightLightButton.PutValue(0);
                    }
                }
                else
                { // kabina -1
                    if (((DynamicObject->iLights[0]) & 48) == 0)
                    {
                        DynamicObject->iLights[0] |= 32;
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        if (ggRearRightEndLightButton.SubModel)
                        {
                            ggRearRightEndLightButton.PutValue(1);
                            ggRearRightLightButton.PutValue(0);
                        }
                        else
                            ggRearRightLightButton.PutValue(-1);
                    }
                    if (((DynamicObject->iLights[0]) & 48) == 16)
                    {
                        DynamicObject->iLights[0] &= (255 - 16);
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        ggRearRightLightButton.PutValue(0);
                    }
                }
            } //------------------------------
            else
            {
                if (mvOccupied->ActiveCab == 1)
                { // kabina 0
                    if (((DynamicObject->iLights[0]) & 48) == 0)
                    {
                        DynamicObject->iLights[0] |= 32;
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        if (ggRightEndLightButton.SubModel)
                        {
                            ggRightEndLightButton.PutValue(1);
                            ggRightLightButton.PutValue(0);
                        }
                        else
                            ggRightLightButton.PutValue(-1);
                    }
                    if (((DynamicObject->iLights[0]) & 48) == 16)
                    {
                        DynamicObject->iLights[0] &= (255 - 16);
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        ggRightLightButton.PutValue(0);
                    }
                }
                else
                { // kabina -1
                    if (((DynamicObject->iLights[1]) & 48) == 0)
                    {
                        DynamicObject->iLights[1] |= 32;
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        if (ggRightEndLightButton.SubModel)
                        {
                            ggRightEndLightButton.PutValue(1);
                            ggRightLightButton.PutValue(0);
                        }
                        else
                            ggRightLightButton.PutValue(-1);
                    }
                    if (((DynamicObject->iLights[1]) & 48) == 16)
                    {
                        DynamicObject->iLights[1] &= (255 - 16);
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        ggRightLightButton.PutValue(0);
                    }
                }
            }
        }
        else if (cKey == Global::Keys[k_StLinOff]) // Winger 110904: wylacznik st. liniowych
        {
            if ((mvControlled->TrainType != dt_EZT) && (mvControlled->TrainType != dt_EP05) &&
                (mvControlled->TrainType != dt_ET40))
            {
                ggStLinOffButton.PutValue(1); // Ra: by³o Fuse...
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play(0, 0, 0);
                if (mvControlled->MainCtrlPosNo > 0)
                {
                    mvControlled->StLinFlag =
                        false; // yBARC - zmienione na przeciwne, bo true to zalaczone
                    dsbRelay->SetVolume(DSBVOLUME_MAX);
                    dsbRelay->Play(0, 0, 0);
                }
            }
            if (mvControlled->TrainType == dt_EZT)
            {
                if (mvControlled->Signalling == true)
                {
                    dsbSwitch->Play(0, 0, 0);
                    mvControlled->Signalling = false;
                }
            }
        }
        else
        {
            // McZapkie: poruszanie sie po kabinie, w updatemechpos zawarte sa wiezy

            // double dt=Timer::GetDeltaTime();
            if (mvOccupied->ActiveCab < 0)
                fMechCroach = -0.5;
            else
                fMechCroach = 0.5;
            //        if (!GetAsyncKeyState(VK_SHIFT)<0)         // bez shifta
            if (!Console::Pressed(VK_CONTROL)) // gdy [Ctrl] zwolniony (dodatkowe widoki)
            {
                if (cKey == Global::Keys[k_MechLeft])
                {
                    vMechMovement.x += fMechCroach;
                    if (DynamicObject->Mechanik)
                        if (!FreeFlyModeFlag) //¿eby nie mieszaæ obserwuj¹c z zewn¹trz
                            DynamicObject->Mechanik->RouteSwitch(1); // na skrzy¿owaniu skrêci w
                                                                     // lewo
                }
                else if (cKey == Global::Keys[k_MechRight])
                {
                    vMechMovement.x -= fMechCroach;
                    if (DynamicObject->Mechanik)
                        if (!FreeFlyModeFlag) //¿eby nie mieszaæ obserwuj¹c z zewn¹trz
                            DynamicObject->Mechanik->RouteSwitch(
                                2); // na skrzy¿owaniu skrêci w prawo
                }
                else if (cKey == Global::Keys[k_MechBackward])
                {
                    vMechMovement.z -= fMechCroach;
                    // if (DynamicObject->Mechanik)
                    // if (!FreeFlyModeFlag) //¿eby nie mieszaæ obserwuj¹c z zewn¹trz
                    //  DynamicObject->Mechanik->RouteSwitch(0);  //na skrzy¿owaniu stanie i poczeka
                }
                else if (cKey == Global::Keys[k_MechForward])
                {
                    vMechMovement.z += fMechCroach;
                    if (DynamicObject->Mechanik)
                        if (!FreeFlyModeFlag) //¿eby nie mieszaæ obserwuj¹c z zewn¹trz
                            DynamicObject->Mechanik->RouteSwitch(
                                3); // na skrzy¿owaniu pojedzie prosto
                }
                else if (cKey == Global::Keys[k_MechUp])
                    pMechOffset.y += 0.2; // McZapkie-120302 - wstawanie
                else if (cKey == Global::Keys[k_MechDown])
                    pMechOffset.y -= 0.2; // McZapkie-120302 - siadanie
            }
        }

        //    else
        if (DebugModeFlag)
        { // przesuwanie sk³adu o 100m
            TDynamicObject *d = DynamicObject;
            if (cKey == VkKeyScan('['))
            {
                while (d)
                {
                    d->Move(100.0 * d->DirectionGet());
                    d = d->Next(); // pozosta³e te¿
                }
                d = DynamicObject->Prev();
                while (d)
                {
                    d->Move(100.0 * d->DirectionGet());
                    d = d->Prev(); // w drug¹ stronê te¿
                }
            }
            else if (cKey == VkKeyScan(']'))
            {
                while (d)
                {
                    d->Move(-100.0 * d->DirectionGet());
                    d = d->Next(); // pozosta³e te¿
                }
                d = DynamicObject->Prev();
                while (d)
                {
                    d->Move(-100.0 * d->DirectionGet());
                    d = d->Prev(); // w drug¹ stronê te¿
                }
            }
        }
        if (cKey == VkKeyScan('-'))
        { // zmniejszenie numeru kana³u radiowego
            if (iRadioChannel > 0)
                --iRadioChannel; // 0=wy³¹czony
        }
        else if (cKey == VkKeyScan('='))
        { // zmniejszenie numeru kana³u radiowego
            if (iRadioChannel < 8)
                ++iRadioChannel; // 0=wy³¹czony
        }
    }
}

void TTrain::OnKeyUp(int cKey)
{ // zwolnienie klawisza
    if (GetAsyncKeyState(VK_SHIFT) < 0)
    { // wciœniêty [Shift]
    }
    else
    {
        if (cKey == Global::Keys[k_StLinOff]) // Winger 110904: wylacznik st. liniowych
        { // zwolnienie klawisza daje powrót przycisku do zwyk³ego stanu
            if ((mvControlled->TrainType != dt_EZT) && (mvControlled->TrainType != dt_EP05) &&
                (mvControlled->TrainType != dt_ET40))
                ggStLinOffButton.PutValue(0);
        }
    }
};

void TTrain::UpdateMechPosition(double dt)
{ // Ra: mechanik powinien byæ telepany niezale¿nie od pozycji pojazdu
    // Ra: trzeba zrobiæ model bujania g³ow¹ i wczepiæ go do pojazdu

    // DynamicObject->vFront=DynamicObject->GetDirection(); //to jest ju¿ policzone

    // Ra: tu by siê przyda³o uwzglêdniæ rozk³ad si³:
    // - na postoju horyzont prosto, kabina skosem
    // - przy szybkiej jeŸdzie kabina prosto, horyzont pochylony

    vector3 pNewMechPosition;
    // McZapkie: najpierw policzê pozycjê w/m kabiny

    // ABu: rzucamy kabina tylko przy duzym FPS!
    // Mala histereza, zeby bez przerwy nie przelaczalo przy FPS~17
    // Granice mozna ustalic doswiadczalnie. Ja proponuje 14:20
    double r1, r2, r3;
    int iVel = DynamicObject->GetVelocity();
    if (iVel > 150)
        iVel = 150;
    if (!Global::iSlowMotion // musi byæ pe³na prêdkoœæ
        && (pMechOffset.y < 4.0)) // Ra 15-01: przy ogl¹daniu pantografu bujanie przeszkadza
    {
        if (!(random((GetFPS() + 1) / 15) > 0))
        {
            if ((iVel > 0) && (random(155 - iVel) < 16))
            {
                r1 = (double(random(iVel * 2) - iVel) / ((iVel * 2) * 4)) * fMechSpringX;
                r2 = (double(random(iVel * 2) - iVel) / ((iVel * 2) * 4)) * fMechSpringY;
                r3 = (double(random(iVel * 2) - iVel) / ((iVel * 2) * 4)) * fMechSpringZ;
                MechSpring.ComputateForces(vector3(r1, r2, r3), pMechShake);
                //      MechSpring.ComputateForces(vector3(double(random(200)-100)/200,double(random(200)-100)/200,double(random(200)-100)/500),pMechShake);
            }
            else
                MechSpring.ComputateForces(vector3(-mvControlled->AccN * dt,
                                                   mvControlled->AccV * dt * 10,
                                                   -mvControlled->AccS * dt),
                                           pMechShake);
        }
        vMechVelocity -= (MechSpring.vForce2 + vMechVelocity * 100) *
                         (fMechSpringX + fMechSpringY + fMechSpringZ) / (200);

        // McZapkie:
        pMechShake += vMechVelocity * dt;
        // Ra 2015-01: dotychczasowe rzucanie
        pMechOffset += vMechMovement * dt;
        if ((pMechShake.y > fMechMaxSpring) || (pMechShake.y < -fMechMaxSpring))
            vMechVelocity.y = -vMechVelocity.y;
        // ABu011104: 5*pMechShake.y, zeby ladnie pudlem rzucalo :)
        pNewMechPosition = pMechOffset + vector3(pMechShake.x, 5 * pMechShake.y, pMechShake.z);
        vMechMovement = 0.5 * vMechMovement;
    }
    else
    { // hamowanie rzucania przy spadku FPS
        pMechShake -=
            pMechShake *
            Min0R(dt, 1); // po tym chyba potrafi¹ zostaæ jakieœ u³amki, które powoduj¹ zjazd
        pMechOffset += vMechMovement * dt;
        vMechVelocity.y = 0.5 * vMechVelocity.y;
        pNewMechPosition = pMechOffset + vector3(pMechShake.x, 5 * pMechShake.y, pMechShake.z);
        vMechMovement = 0.5 * vMechMovement;
    }
    // numer kabiny (-1: kabina B)
    if (DynamicObject->Mechanik) // mo¿e nie byæ?
        if (DynamicObject->Mechanik->AIControllFlag) // jeœli prowadzi AI
        { // Ra: przesiadka, jeœli AI zmieni³o kabinê (a cz³on?)...
            if (iCabn != (DynamicObject->MoverParameters->ActiveCab == -1 ?
                              2 :
                              DynamicObject->MoverParameters->ActiveCab))
                InitializeCab(DynamicObject->MoverParameters->ActiveCab,
                              DynamicObject->asBaseDir + DynamicObject->MoverParameters->TypeName +
                                  ".mmd");
        }
    iCabn = (DynamicObject->MoverParameters->ActiveCab == -1 ?
                 2 :
                 DynamicObject->MoverParameters->ActiveCab);
    if (!DebugModeFlag)
    { // sprawdzaj wiêzy //Ra: nie tu!
        if (pNewMechPosition.x < Cabine[iCabn].CabPos1.x)
            pNewMechPosition.x = Cabine[iCabn].CabPos1.x;
        if (pNewMechPosition.x > Cabine[iCabn].CabPos2.x)
            pNewMechPosition.x = Cabine[iCabn].CabPos2.x;
        if (pNewMechPosition.z < Cabine[iCabn].CabPos1.z)
            pNewMechPosition.z = Cabine[iCabn].CabPos1.z;
        if (pNewMechPosition.z > Cabine[iCabn].CabPos2.z)
            pNewMechPosition.z = Cabine[iCabn].CabPos2.z;
        if (pNewMechPosition.y > Cabine[iCabn].CabPos1.y + 1.8)
            pNewMechPosition.y = Cabine[iCabn].CabPos1.y + 1.8;
        if (pNewMechPosition.y < Cabine[iCabn].CabPos1.y + 0.5)
            pNewMechPosition.y = Cabine[iCabn].CabPos2.y + 0.5;

        if (pMechOffset.x < Cabine[iCabn].CabPos1.x)
            pMechOffset.x = Cabine[iCabn].CabPos1.x;
        if (pMechOffset.x > Cabine[iCabn].CabPos2.x)
            pMechOffset.x = Cabine[iCabn].CabPos2.x;
        if (pMechOffset.z < Cabine[iCabn].CabPos1.z)
            pMechOffset.z = Cabine[iCabn].CabPos1.z;
        if (pMechOffset.z > Cabine[iCabn].CabPos2.z)
            pMechOffset.z = Cabine[iCabn].CabPos2.z;
        if (pMechOffset.y > Cabine[iCabn].CabPos1.y + 1.8)
            pMechOffset.y = Cabine[iCabn].CabPos1.y + 1.8;
        if (pMechOffset.y < Cabine[iCabn].CabPos1.y + 0.5)
            pMechOffset.y = Cabine[iCabn].CabPos2.y + 0.5;
    }
    pMechPosition = DynamicObject->mMatrix *
                    pNewMechPosition; // po³o¿enie wzglêdem œrodka pojazdu w uk³adzie scenerii
    pMechPosition += DynamicObject->GetPosition();
};

bool TTrain::Update()
{
    DWORD stat;
    double dt = Timer::GetDeltaTime();
    if (DynamicObject->mdKabina)
    { // Ra: TODO: odczyty klawiatury/pulpitu nie powinny byæ uzale¿nione od istnienia modelu kabiny
        tor = DynamicObject->GetTrack(); // McZapkie-180203
        // McZapkie: predkosc wyswietlana na tachometrze brana jest z obrotow kol
        float maxtacho = 3;
        fTachoVelocity = Min0R(fabs(11.31 * mvControlled->WheelDiameter * mvControlled->nrot),
                               mvControlled->Vmax * 1.05);
        { // skacze osobna zmienna
            float ff = floor(
                GlobalTime->mr); // skacze co sekunde - pol sekundy pomiar, pol sekundy ustawienie
            if (ff != fTachoTimer) // jesli w tej sekundzie nie zmienial
            {
                if (fTachoVelocity > 1) // jedzie
                    fTachoVelocityJump = fTachoVelocity + (2 - random(3) + random(3)) * 0.5;
                else
                    fTachoVelocityJump = 0; // stoi
                fTachoTimer = ff; // juz zmienil
            }
        }
        if (fTachoVelocity > 1) // McZapkie-270503: podkrecanie tachometru
        {
            if (fTachoCount < maxtacho)
                fTachoCount += dt * 3; // szybciej zacznij stukac
        }
        else if (fTachoCount > 0)
            fTachoCount -=
                dt * 0.66; // schodz powoli - niektore haslery to ze 4 sekundy potrafia stukac

        /* Ra: to by trzeba by³o przemyœleæ, zmienione na szybko problemy robi
          //McZapkie: predkosc wyswietlana na tachometrze brana jest z obrotow kol
          double vel=fabs(11.31*mvControlled->WheelDiameter*mvControlled->nrot);
          if (iSekunda!=floor(GlobalTime->mr)||(vel<1.0))
          {fTachoVelocity=vel;
           if (fTachoVelocity>1.0) //McZapkie-270503: podkrecanie tachometru
           {
            if (fTachoCount<maxtacho)
             fTachoCount+=dt;
           }
           else
            if (fTachoCount>0)
             fTachoCount-=dt;
           if (mvControlled->TrainType==dt_EZT)
            //dla EZT wskazówka porusza siê niestabilnie
            if (fTachoVelocity>7.0)
            {fTachoVelocity=floor(0.5+fTachoVelocity+random(5)-random(5));
          //*floor(0.2*fTachoVelocity);
             if (fTachoVelocity<0.0) fTachoVelocity=0.0;
            }
           iSekunda=floor(GlobalTime->mr);
          }
        */
        // Ra 2014-09: napiêcia i pr¹dy musz¹ byæ ustalone najpierw, bo wysy³ane s¹ ewentualnie na
        // PoKeys
        if (mvControlled->EngineType != DieselElectric) // Ra 2014-09: czy taki rozdzia³ ma sens?
            fHVoltage = mvControlled->RunningTraction
                            .TractionVoltage; // Winger czy to nie jest zle? *mvControlled->Mains);
        else
            fHVoltage = mvControlled->Voltage;
        if (ShowNextCurrent)
        { // jeœli pokazywaæ drugi cz³on
            if (mvSecond)
            { // o ile jest ten drugi
                fHCurrent[0] = mvSecond->ShowCurrent(0) * 1.05;
                fHCurrent[1] = mvSecond->ShowCurrent(1) * 1.05;
                fHCurrent[2] = mvSecond->ShowCurrent(2) * 1.05;
                fHCurrent[3] = mvSecond->ShowCurrent(3) * 1.05;
            }
            else
                fHCurrent[0] = fHCurrent[1] = fHCurrent[2] = fHCurrent[3] = 0.0; // gdy nie ma
                                                                                 // cz³ona
        }
        else
        { // normalne pokazywanie
            fHCurrent[0] = mvControlled->ShowCurrent(0);
            fHCurrent[1] = mvControlled->ShowCurrent(1);
            fHCurrent[2] = mvControlled->ShowCurrent(2);
            fHCurrent[3] = mvControlled->ShowCurrent(3);
        }
        if (Global::iFeedbackMode == 4)
        { // wykonywaæ tylko gdy wyprowadzone na pulpit
            Console::ValueSet(0,
                              mvOccupied->Compressor); // Ra: sterowanie miernikiem: zbiornik g³ówny
            Console::ValueSet(1, mvOccupied->PipePress); // Ra: sterowanie miernikiem: przewód
                                                         // g³ówny
            Console::ValueSet(
                2, mvOccupied->BrakePress); // Ra: sterowanie miernikiem: cylinder hamulcowy
            Console::ValueSet(3, fHVoltage); // woltomierz wysokiego napiêcia
            Console::ValueSet(4, fHCurrent[2]); // Ra: sterowanie miernikiem: drugi amperomierz
            Console::ValueSet(
                5, fHCurrent[(mvControlled->TrainType & dt_EZT) ? 0 : 1]); // pierwszy amperomierz;
                                                                           // dla EZT pr¹d ca³kowity
            Console::ValueSet(6, fTachoVelocity); ////Ra: prêdkoœæ na pin 43 - wyjœcie analogowe (to
                                                  ///nie jest PWM); skakanie zapewnia mechanika
                                                  ///napêdu
        }

        // hunter-080812: wyrzucanie szybkiego na elektrykach gdy nie ma napiecia przy dowolnym
        // ustawieniu kierunkowego
        // Ra: to ju¿ jest w T_MoverParameters::TractionForce(), ale zale¿y od kierunku
        if (mvControlled->EngineType == ElectricSeriesMotor)
            if (fabs(mvControlled->RunningTraction.TractionVoltage) <
                0.5 *
                    mvControlled->EnginePowerSource
                        .MaxVoltage) // minimalne napiêcie pobieraæ z FIZ?
                mvControlled->MainSwitch(false);

        // hunter-091012: swiatlo
        if (bCabLight == true)
        {
            if (bCabLightDim == true)
                iCabLightFlag = 1;
            else
                iCabLightFlag = 2;
        }
        else
            iCabLightFlag = 0;

        //------------------
        // hunter-261211: nadmiarowy przetwornicy i ogrzewania
        // Ra 15-01: to musi st¹d wylecieæ - zale¿noœci nie mog¹ byæ w kabinie
        if (mvControlled->ConverterFlag == true)
        {
            fConverterTimer += dt;
            if ((mvControlled->CompressorFlag == true) && (mvControlled->CompressorPower == 1) &&
                ((mvControlled->EngineType == ElectricSeriesMotor) ||
                 (mvControlled->TrainType == dt_EZT)) &&
                (DynamicObject->Controller == Humandriver)) // hunter-110212: poprawka dla EZT
            { // hunter-091012: poprawka (zmiana warunku z CompressorPower /rozne od 0/ na /rowne
              // 1/)
                if (fConverterTimer < fConverterPrzekaznik)
                {
                    mvControlled->ConvOvldFlag = true;
                    mvControlled->MainSwitch(false);
                }
                else if (fConverterTimer >= fConverterPrzekaznik)
                    mvControlled->CompressorSwitch(true);
            }
        }
        else
            fConverterTimer = 0;
        //------------------

        double vol = 0;
        //    int freq=1;
        double dfreq;

        // McZapkie-280302 - syczenie
        if ((mvOccupied->BrakeHandle == FV4a) || (mvOccupied->BrakeHandle == FVel6))
        {
            if (rsHiss.AM != 0) // upuszczanie z PG
            {
                fPPress = (1 * fPPress + mvOccupied->Handle->GetSound(s_fv4a_b)) / (2);
                if (fPPress > 0)
                {
                    vol = 2 * rsHiss.AM * fPPress;
                }
                if (vol > 0.001)
                {
                    rsHiss.Play(vol, DSBPLAY_LOOPING, true, DynamicObject->GetPosition());
                }
                else
                {
                    rsHiss.Stop();
                }
            }
            if (rsHissU.AM != 0) // upuszczanie z PG
            {
                fNPress = (1 * fNPress + mvOccupied->Handle->GetSound(s_fv4a_u)) / (2);
                if (fNPress > 0)
                {
                    vol = rsHissU.AM * fNPress;
                }
                if (vol > 0.001)
                {
                    rsHissU.Play(vol, DSBPLAY_LOOPING, true, DynamicObject->GetPosition());
                }
                else
                {
                    rsHissU.Stop();
                }
            }
            if (rsHissE.AM != 0) // upuszczanie przy naglym
            {
                vol = mvOccupied->Handle->GetSound(s_fv4a_e) * rsHissE.AM;
                if (vol > 0.001)
                {
                    rsHissE.Play(vol, DSBPLAY_LOOPING, true, DynamicObject->GetPosition());
                }
                else
                {
                    rsHissE.Stop();
                }
            }
            if (rsHissX.AM != 0) // upuszczanie sterujacego fala
            {
                vol = mvOccupied->Handle->GetSound(s_fv4a_x) * rsHissX.AM;
                if (vol > 0.001)
                {
                    rsHissX.Play(vol, DSBPLAY_LOOPING, true, DynamicObject->GetPosition());
                }
                else
                {
                    rsHissX.Stop();
                }
            }
            if (rsHissT.AM != 0) // upuszczanie z czasowego
            {
                vol = mvOccupied->Handle->GetSound(s_fv4a_t) * rsHissT.AM;
                if (vol > 0.001)
                {
                    rsHissT.Play(vol, DSBPLAY_LOOPING, true, DynamicObject->GetPosition());
                }
                else
                {
                    rsHissT.Stop();
                }
            }

        } // koniec FV4a
        else // jesli nie FV4a
        {
            if (rsHiss.AM != 0) // upuszczanie z PG
            {
                fPPress = (4 * fPPress + Max0R(mvOccupied->dpLocalValve, mvOccupied->dpMainValve)) /
                          (4 + 1);
                if (fPPress > 0)
                {
                    vol = 2 * rsHiss.AM * fPPress * 0.01;
                }
                if (vol > 0.01)
                {
                    rsHiss.Play(vol, DSBPLAY_LOOPING, true, DynamicObject->GetPosition());
                }
                else
                {
                    rsHiss.Stop();
                }
            }
            if (rsHissU.AM != 0) // napelnianie PG
            {
                fNPress = (4 * fNPress + Min0R(mvOccupied->dpLocalValve, mvOccupied->dpMainValve)) /
                          (4 + 1);
                if (fNPress < 0)
                {
                    vol = -2 * rsHissU.AM * fNPress * 0.004;
                }
                if (vol > 0.01)
                {
                    rsHissU.Play(vol, DSBPLAY_LOOPING, true, DynamicObject->GetPosition());
                }
                else
                {
                    rsHissU.Stop();
                }
            }
        } // koniec nie FV4a

        // Winger-160404 - syczenie pomocniczego (luzowanie)
        /*      if (rsSBHiss.AM!=0)
               {
                  fSPPress=(mvOccupied->LocalBrakeRatio())-(mvOccupied->LocalBrakePos);
                  if (fSPPress>0)
                   {
                    vol=2*rsSBHiss.AM*fSPPress;
                   }
                  if (vol>0.1)
                   {
                    rsSBHiss.Play(vol,DSBPLAY_LOOPING,true,DynamicObject->GetPosition());
                   }
                  else
                   {
                    rsSBHiss.Stop();
                   }
               }
        */
        // szum w czasie jazdy
        vol = 0.0;
        dfreq = 1.0;
        if (rsRunningNoise.AM != 0)
        {
            if (DynamicObject->GetVelocity() != 0)
            {
                if (!TestFlag(mvOccupied->DamageFlag,
                              dtrain_wheelwear)) // McZpakie-221103: halas zalezny od kola
                {
                    dfreq = rsRunningNoise.FM * mvOccupied->Vel + rsRunningNoise.FA;
                    vol = rsRunningNoise.AM * mvOccupied->Vel + rsRunningNoise.AA;
                    switch (tor->eEnvironment)
                    {
                    case e_tunnel:
                    {
                        vol *= 3;
                        dfreq *= 0.95;
                    }
                    break;
                    case e_canyon:
                    {
                        vol *= 1.1;
                    }
                    break;
                    case e_bridge:
                    {
                        vol *= 2;
                        dfreq *= 0.98;
                    }
                    break;
                    }
                }
                else // uszkodzone kolo (podkucie)
                    if (fabs(mvOccupied->nrot) > 0.01)
                {
                    dfreq = rsRunningNoise.FM * mvOccupied->Vel + rsRunningNoise.FA;
                    vol = rsRunningNoise.AM * mvOccupied->Vel + rsRunningNoise.AA;
                    switch (tor->eEnvironment)
                    {
                    case e_tunnel:
                    {
                        vol *= 2;
                    }
                    break;
                    case e_canyon:
                    {
                        vol *= 1.1;
                    }
                    break;
                    case e_bridge:
                    {
                        vol *= 1.5;
                    }
                    break;
                    }
                }
                if (fabs(mvOccupied->nrot) > 0.01)
                    vol *= 1 +
                           mvOccupied->UnitBrakeForce /
                               (1 + mvOccupied->MaxBrakeForce); // hamulce wzmagaja halas
                vol = vol * (20.0 + tor->iDamageFlag) / 21;
                rsRunningNoise.AdjFreq(dfreq, 0);
                rsRunningNoise.Play(vol, DSBPLAY_LOOPING, true, DynamicObject->GetPosition());
            }
            else
                rsRunningNoise.Stop();
        }

        if (rsBrake.AM != 0)
        {
            if ((!mvOccupied->SlippingWheels) && (mvOccupied->UnitBrakeForce > 10.0) &&
                (DynamicObject->GetVelocity() > 0.01))
            {
                //        vol=rsBrake.AA+rsBrake.AM*(DynamicObject->GetVelocity()*100+mvOccupied->UnitBrakeForce);
                vol =
                    rsBrake.AM * sqrt((DynamicObject->GetVelocity() * mvOccupied->UnitBrakeForce));
                dfreq = rsBrake.FA + rsBrake.FM * DynamicObject->GetVelocity();
                rsBrake.AdjFreq(dfreq, 0);
                rsBrake.Play(vol, DSBPLAY_LOOPING, true, DynamicObject->GetPosition());
            }
            else
            {
                rsBrake.Stop();
            }
        }

        if (rsEngageSlippery.AM != 0)
        {
            if /*((fabs(mvControlled->dizel_engagedeltaomega)>0.2) && */ (
                mvControlled->dizel_engage > 0.1)
            {
                if (fabs(mvControlled->dizel_engagedeltaomega) > 0.2)
                {
                    dfreq = rsEngageSlippery.FA +
                            rsEngageSlippery.FM * fabs(mvControlled->dizel_engagedeltaomega);
                    vol = rsEngageSlippery.AA + rsEngageSlippery.AM * (mvControlled->dizel_engage);
                }
                else
                {
                    dfreq =
                        1; // rsEngageSlippery.FA+0.7*rsEngageSlippery.FM*(fabs(mvControlled->enrot)+mvControlled->nmax);
                    if (mvControlled->dizel_engage > 0.2)
                        vol =
                            rsEngageSlippery.AA +
                            0.2 * rsEngageSlippery.AM * (mvControlled->enrot / mvControlled->nmax);
                    else
                        vol = 0;
                }
                rsEngageSlippery.AdjFreq(dfreq, 0);
                rsEngageSlippery.Play(vol, DSBPLAY_LOOPING, true, DynamicObject->GetPosition());
            }
            else
            {
                rsEngageSlippery.Stop();
            }
        }

        if (FreeFlyModeFlag)
            rsFadeSound.Stop(); // wy³¹cz to cholerne cykanie!
        else
            rsFadeSound.Play(1, DSBPLAY_LOOPING, true, DynamicObject->GetPosition());

        // McZapkie! - to wazne - SoundFlag wystawiane jest przez moje moduly
        // gdy zachodza pewne wydarzenia komentowane dzwiekiem.
        // Mysle ze wystarczy sprawdzac a potem zerowac SoundFlag tutaj
        // a nie w DynObject - gdyby cos poszlo zle to po co szarpac dzwiekiem co 10ms.

        if (TestFlag(mvOccupied->SoundFlag,
                     sound_relay)) // przekaznik - gdy bezpiecznik, automatyczny rozruch itp
        {
            if (mvOccupied->EventFlag || TestFlag(mvOccupied->SoundFlag, sound_loud))
            {
                mvOccupied->EventFlag = false; // Ra: w kabinie?
                dsbRelay->SetVolume(DSBVOLUME_MAX);
            }
            else
            {
                dsbRelay->SetVolume(-40);
            }
            if (!TestFlag(mvOccupied->SoundFlag, sound_manyrelay))
                dsbRelay->Play(0, 0, 0);
            else
            {
                if (TestFlag(mvOccupied->SoundFlag, sound_loud))
                    dsbWejscie_na_bezoporow->Play(0, 0, 0);
                else
                    dsbWejscie_na_drugi_uklad->Play(0, 0, 0);
            }
        }
        // potem dorobic bufory, sprzegi jako RealSound.
        if (TestFlag(mvOccupied->SoundFlag, sound_bufferclamp)) // zderzaki uderzaja o siebie
        {
            if (TestFlag(mvOccupied->SoundFlag, sound_loud))
                dsbBufferClamp->SetVolume(DSBVOLUME_MAX);
            else
                dsbBufferClamp->SetVolume(-20);
            dsbBufferClamp->Play(0, 0, 0);
        }
        if (dsbCouplerStretch)
            if (TestFlag(mvOccupied->SoundFlag, sound_couplerstretch)) // sprzegi sie rozciagaja
            {
                if (TestFlag(mvOccupied->SoundFlag, sound_loud))
                    dsbCouplerStretch->SetVolume(DSBVOLUME_MAX);
                else
                    dsbCouplerStretch->SetVolume(-20);
                dsbCouplerStretch->Play(0, 0, 0);
            }

        if (mvOccupied->SoundFlag == 0)
            if (mvOccupied->EventFlag)
                if (TestFlag(mvOccupied->DamageFlag, dtrain_wheelwear))
                { // Ra: przenieœæ do DynObj!
                    if (rsRunningNoise.AM != 0)
                    {
                        rsRunningNoise.Stop();
                        // float aa=rsRunningNoise.AA;
                        float am = rsRunningNoise.AM;
                        float fa = rsRunningNoise.FA;
                        float fm = rsRunningNoise.FM;
                        rsRunningNoise.Init("lomotpodkucia.wav", -1, 0, 0, 0,
                                            true); // MC: zmiana szumu na lomot
                        if (rsRunningNoise.AM == 1)
                            rsRunningNoise.AM = am;
                        rsRunningNoise.AA = 0.7;
                        rsRunningNoise.FA = fa;
                        rsRunningNoise.FM - fm;
                    }
                    mvOccupied->EventFlag = false;
                }

        mvOccupied->SoundFlag = 0;
        /*
            for (int b=0; b<2; b++) //MC: aby zerowac stukanie przekaznikow w czlonie silnikowym
             if (TestFlag(mvControlled->Couplers[b].CouplingFlag,ctrain_controll))
              if (mvControlled->Couplers[b].Connected.Power>0.01)
               mvControlled->Couplers[b]->Connected->SoundFlag=0;
        */

        // McZapkie! - koniec obslugi dzwiekow z mover.pas

        // youBy - prad w drugim czlonie: galaz lub calosc
        {
            TDynamicObject *tmp;
            tmp = NULL;
            if (DynamicObject->NextConnected)
                if ((TestFlag(mvControlled->Couplers[1].CouplingFlag, ctrain_controll)) &&
                    (mvOccupied->ActiveCab == 1))
                    tmp = DynamicObject->NextConnected;
            if (DynamicObject->PrevConnected)
                if ((TestFlag(mvControlled->Couplers[0].CouplingFlag, ctrain_controll)) &&
                    (mvOccupied->ActiveCab == -1))
                    tmp = DynamicObject->PrevConnected;
            if (tmp)
                if (tmp->MoverParameters->Power > 0)
                {
                    if (ggI1B.SubModel)
                    {
                        ggI1B.UpdateValue(tmp->MoverParameters->ShowCurrent(1));
                        ggI1B.Update();
                    }
                    if (ggI2B.SubModel)
                    {
                        ggI2B.UpdateValue(tmp->MoverParameters->ShowCurrent(2));
                        ggI2B.Update();
                    }
                    if (ggI3B.SubModel)
                    {
                        ggI3B.UpdateValue(tmp->MoverParameters->ShowCurrent(3));
                        ggI3B.Update();
                    }
                    if (ggItotalB.SubModel)
                    {
                        ggItotalB.UpdateValue(tmp->MoverParameters->ShowCurrent(0));
                        ggItotalB.Update();
                    }
                }
        }
        /*
        //McZapkie-240302    ggVelocity.UpdateValue(DynamicObject->GetVelocity());
            //fHaslerTimer+=dt;
            //if (fHaslerTimer>fHaslerTime)
            {//Ra: ryzykowne jest to, gdy¿ mo¿e siê nie uaktualniaæ prêdkoœæ
             //Ra: prêdkoœæ siê powinna zaokr¹glaæ tam gdzie siê liczy fTachoVelocity
             if (ggVelocity.SubModel)
             {//ZiomalCl: wskazanie Haslera w kabinie A ze zwloka czasowa oraz odpowiednia
        tolerancja
              //Nalezy sie zastanowic na przyszlosc nad rozroznieniem predkosciomierzy (dokladnosc
        wskazan, zwloka czasowa wskazania, inne funkcje)
              //ZiomalCl: W ezt typu stare EN57 wskazania haslera sa mniej dokladne (linka)
              //ggVelocity.UpdateValue(fTachoVelocity>2?fTachoVelocity+0.5-random(mvControlled->TrainType==dt_EZT?5:2)/2:0);
              ggVelocity.UpdateValue(Min0R(fTachoVelocity,mvControlled->Vmax*1.05)); //ograniczenie
        maksymalnego wskazania na analogowym
              ggVelocity.Update();
             }
             if (ggVelocityDgt.SubModel)
             {//Ra 2014-07: prêdkoœciomierz cyfrowy
              ggVelocityDgt.UpdateValue(fTachoVelocity);
              ggVelocityDgt.Update();
             }
             if (ggVelocity_B.SubModel)
             {//ZiomalCl: wskazanie Haslera w kabinie B ze zwloka czasowa oraz odpowiednia
        tolerancja
              //Nalezy sie zastanowic na przyszlosc nad rozroznieniem predkosciomierzy (dokladnosc
        wskazan, zwloka czasowa wskazania, inne funkcje)
              //Velocity_B.UpdateValue(fTachoVelocity>2?fTachoVelocity+0.5-random(mvControlled->TrainType==dt_EZT?5:2)/2:0);
              ggVelocity_B.UpdateValue(fTachoVelocity);
              ggVelocity_B.Update();
             }
             //fHaslerTimer-=fHaslerTime; //1.2s (???)
            }
        */
        // McZapkie-300302: zegarek
        if (ggClockMInd.SubModel)
        {
            ggClockSInd.UpdateValue(int(GlobalTime->mr));
            ggClockSInd.Update();
            ggClockMInd.UpdateValue(GlobalTime->mm);
            ggClockMInd.Update();
            ggClockHInd.UpdateValue(GlobalTime->hh + GlobalTime->mm / 60.0);
            ggClockHInd.Update();
        }

        Cabine[iCabn].Update(); // nowy sposób ustawienia animacji
        if (ggZbS.SubModel)
        {
            ggZbS.UpdateValue(mvOccupied->Handle->GetCP());
            ggZbS.Update();
        }

        // youBy - napiecie na silnikach
        if (ggEngineVoltage.SubModel)
        {
            if (mvControlled->DynamicBrakeFlag)
            {
                ggEngineVoltage.UpdateValue(abs(mvControlled->Im * 5));
            }
            else
            {
                int x;
                if ((mvControlled->TrainType == dt_ET42) &&
                    (mvControlled->Imax == mvControlled->ImaxHi))
                    x = 1;
                else
                    x = 2;
                if ((mvControlled->RList[mvControlled->MainCtrlActualPos].Mn > 0) &&
                    (abs(mvControlled->Im) > 0))
                {
                    ggEngineVoltage.UpdateValue(
                        (x * (mvControlled->RunningTraction.TractionVoltage -
                              mvControlled->RList[mvControlled->MainCtrlActualPos].R *
                                  abs(mvControlled->Im)) /
                         mvControlled->RList[mvControlled->MainCtrlActualPos].Mn));
                }
                else
                {
                    ggEngineVoltage.UpdateValue(0);
                }
            }
            ggEngineVoltage.Update();
        }

        // Winger 140404 - woltomierz NN
        if (ggLVoltage.SubModel)
        {
            if (mvControlled->Battery == true)
                ggLVoltage.UpdateValue(mvControlled->BatteryVoltage);
            else
                ggLVoltage.UpdateValue(0);
            ggLVoltage.Update();
        }

        if (mvControlled->EngineType == DieselElectric)
        { // ustawienie zmiennych dla silnika spalinowego
            fEngine[1] = mvControlled->ShowEngineRotation(1);
            fEngine[2] = mvControlled->ShowEngineRotation(2);
            // if (ggEnrot1m.SubModel)
            //{
            // ggEnrot1m.UpdateValue(mvControlled->ShowEngineRotation(1));
            // ggEnrot1m.Update();
            //}
            // if (ggEnrot2m.SubModel)
            //{
            // ggEnrot2m.UpdateValue(mvControlled->ShowEngineRotation(2));
            // ggEnrot2m.Update();
            //}
        }

        else if (mvControlled->EngineType == DieselEngine)
        { // albo dla innego spalinowego
            fEngine[1] = mvControlled->ShowEngineRotation(1);
            fEngine[2] = mvControlled->ShowEngineRotation(2);
            fEngine[3] = mvControlled->ShowEngineRotation(3);
            // if (ggEnrot1m.SubModel)
            //{
            // ggEnrot1m.UpdateValue(mvControlled->ShowEngineRotation(1));
            // ggEnrot1m.Update();
            //}
            // if (ggEnrot2m.SubModel)
            //{
            // ggEnrot2m.UpdateValue(mvControlled->ShowEngineRotation(2));
            // ggEnrot2m.Update();
            //}
            // if (ggEnrot3m.SubModel)
            // if (mvControlled->Couplers[1].Connected)
            // {
            //  ggEnrot3m.UpdateValue(mvControlled->ShowEngineRotation(3));
            //  ggEnrot3m.Update();
            // }
            // if (ggEngageRatio.SubModel)
            //{
            // ggEngageRatio.UpdateValue(mvControlled->dizel_engage);
            // ggEngageRatio.Update();
            //}
            if (ggMainGearStatus.SubModel)
            {
                if (mvControlled->Mains)
                    ggMainGearStatus.UpdateValue(1.1 -
                                                 fabs(mvControlled->dizel_automaticgearstatus));
                else
                    ggMainGearStatus.UpdateValue(0);
                ggMainGearStatus.Update();
            }
            if (ggIgnitionKey.SubModel)
            {
                ggIgnitionKey.UpdateValue(mvControlled->dizel_enginestart);
                ggIgnitionKey.Update();
            }
        }

        if (mvControlled->SlippingWheels)
        { // Ra 2014-12: lokomotywy 181/182 dostaj¹ SlippingWheels po zahamowaniu powy¿ej 2.85 bara
          // i bucza³y
            double veldiff = (DynamicObject->GetVelocity() - fTachoVelocity) / mvControlled->Vmax;
            if (veldiff <
                -0.01) // 1% Vmax rezerwy, ¿eby 181/182 nie bucza³y po zahamowaniu, ale to proteza
            {
                if (fabs(mvControlled->Im) > 10.0)
                    btLampkaPoslizg.TurnOn();
                rsSlippery.Play(-rsSlippery.AM * veldiff + rsSlippery.AA, DSBPLAY_LOOPING, true,
                                DynamicObject->GetPosition());
                if (mvControlled->TrainType ==
                    dt_181) // alarm przy poslizgu dla 181/182 - BOMBARDIER
                    if (dsbSlipAlarm)
                        dsbSlipAlarm->Play(0, 0, DSBPLAY_LOOPING);
            }
            else
            {
                if ((mvOccupied->UnitBrakeForce > 100.0) && (DynamicObject->GetVelocity() > 1.0))
                {
                    rsSlippery.Play(rsSlippery.AM * veldiff + rsSlippery.AA, DSBPLAY_LOOPING, true,
                                    DynamicObject->GetPosition());
                    if (mvControlled->TrainType == dt_181)
                        if (dsbSlipAlarm)
                            dsbSlipAlarm->Stop();
                }
            }
        }
        else
        {
            btLampkaPoslizg.TurnOff();
            rsSlippery.Stop();
            if (mvControlled->TrainType == dt_181)
                if (dsbSlipAlarm)
                    dsbSlipAlarm->Stop();
        }

        if (mvControlled->Mains)
        {
            btLampkaWylSzybki.TurnOn();
            btLampkaOpory.Turn(mvControlled->StLinFlag ? mvControlled->ResistorsFlagCheck() :
                                                         false);
            btLampkaBezoporowa.Turn(mvControlled->ResistorsFlagCheck() ||
                                    (mvControlled->MainCtrlActualPos == 0)); // do EU04
            if ((mvControlled->Itot != 0) || (mvOccupied->BrakePress > 2) ||
                (mvOccupied->PipePress < 3.6))
                btLampkaStyczn.TurnOff(); // Ra: czy to jest udawanie dzia³ania styczników
                                          // liniowych?
            else if (mvOccupied->BrakePress < 1)
                btLampkaStyczn.TurnOn(); // mozna prowadzic rozruch
            if (((TestFlag(mvControlled->Couplers[1].CouplingFlag, ctrain_controll)) &&
                 (mvControlled->CabNo == 1)) ||
                ((TestFlag(mvControlled->Couplers[0].CouplingFlag, ctrain_controll)) &&
                 (mvControlled->CabNo == -1)))
                btLampkaUkrotnienie.TurnOn();
            else
                btLampkaUkrotnienie.TurnOff();

            //         if ((TestFlag(mvControlled->BrakeStatus,+b_Rused+b_Ractive)))//Lampka
            //         drugiego stopnia hamowania
            btLampkaHamPosp.Turn((TestFlag(mvOccupied->BrakeStatus, 1))); // lampka drugiego stopnia
                                                                          // hamowania  //TODO:
                                                                          // youBy wyci¹gn¹æ flagê
                                                                          // wysokiego stopnia

            // hunter-111211: wylacznik cisnieniowy - Ra: tutaj? w kabinie? //yBARC -
            // omujborzegrzesiuzniszczylesmicalydzien
            //        if (mvControlled->TrainType!=dt_EZT)
            //         if (((mvOccupied->BrakePress > 2) || ( mvOccupied->PipePress < 3.6 )) && (
            //         mvControlled->MainCtrlPos != 0 ))
            //          mvControlled->StLinFlag=true;
            //-------

            // hunter-121211: lampka zanikowo-pradowego wentylatorow:
            btLampkaNadmWent.Turn((mvControlled->RventRot < 5.0) &&
                                  mvControlled->ResistorsFlagCheck());
            //-------

            btLampkaNadmSil.Turn(mvControlled->FuseFlagCheck());
            btLampkaWysRozr.Turn(mvControlled->Imax == mvControlled->ImaxHi);
            if (((mvControlled->ScndCtrlActualPos > 0) ||
                 ((mvControlled->RList[mvControlled->MainCtrlActualPos].ScndAct != 0) &&
                  (mvControlled->RList[mvControlled->MainCtrlActualPos].ScndAct != 255))) &&
                (!mvControlled->DelayCtrlFlag))
                btLampkaBoczniki.TurnOn();
            else
                btLampkaBoczniki.TurnOff();

            btLampkaNapNastHam.Turn(mvControlled->ActiveDir !=
                                    0); // napiecie na nastawniku hamulcowym
            btLampkaSprezarka.Turn(mvControlled->CompressorFlag); // mutopsitka dziala
            // boczniki
            unsigned char scp; // Ra: dopisa³em "unsigned"
            // Ra: w SU45 boczniki wchodz¹ na MainCtrlPos, a nie na MainCtrlActualPos - pokiæka³
            // ktoœ?
            scp = mvControlled->RList[mvControlled->MainCtrlPos].ScndAct;
            scp = (scp == 255 ? 0 : scp); // Ra: whatta hella is this?
            if ((mvControlled->ScndCtrlPos > 0) || (mvControlled->ScndInMain) && (scp > 0))
            { // boczniki pojedynczo
                btLampkaBocznik1.TurnOn();
                btLampkaBocznik2.Turn(mvControlled->ScndCtrlPos > 1);
                btLampkaBocznik3.Turn(mvControlled->ScndCtrlPos > 2);
                btLampkaBocznik4.Turn(mvControlled->ScndCtrlPos > 3);
            }
            else
            { // wy³¹czone wszystkie cztery
                btLampkaBocznik1.TurnOff();
                btLampkaBocznik2.TurnOff();
                btLampkaBocznik3.TurnOff();
                btLampkaBocznik4.TurnOff();
            }
            /*
                    { //sprezarka w drugim wozie
                    bool comptemp=false;
                    if (DynamicObject->NextConnected)
                       if (TestFlag(mvControlled->Couplers[1].CouplingFlag,ctrain_controll))
                          comptemp=DynamicObject->NextConnected->MoverParameters->CompressorFlag;
                    if ((DynamicObject->PrevConnected) && (!comptemp))
                       if (TestFlag(mvControlled->Couplers[0].CouplingFlag,ctrain_controll))
                          comptemp=DynamicObject->PrevConnected->MoverParameters->CompressorFlag;
                    btLampkaSprezarkaB.Turn(comptemp);
            */
        }
        else // wylaczone
        {
            btLampkaWylSzybki.TurnOff();
            btLampkaOpory.TurnOff();
            btLampkaStyczn.TurnOff();
            btLampkaNadmSil.TurnOff();
            btLampkaUkrotnienie.TurnOff();
            btLampkaHamPosp.TurnOff();
            btLampkaBoczniki.TurnOff();
            btLampkaNapNastHam.TurnOff();
            btLampkaSprezarka.TurnOff();
            btLampkaBezoporowa.TurnOff();
        }
        if (mvControlled->Signalling == true)
        {

            if ((mvOccupied->BrakePress >= 0.145f) && (mvControlled->Battery == true) &&
                (mvControlled->Signalling == true))
            {
                btLampkaHamowanie1zes.TurnOn();
            }
            if (mvControlled->BrakePress < 0.075f)
            {
                btLampkaHamowanie1zes.TurnOff();
            }
        }
        else
        {
            btLampkaHamowanie1zes.TurnOff();
        }
        btLampkaBlokadaDrzwi.Turn(mvControlled->DoorSignalling ?
                                      mvOccupied->DoorBlockedFlag() && mvControlled->Battery :
                                      false);

        { // yB - wskazniki drugiego czlonu
            TDynamicObject *
                tmp; //=mvControlled->mvSecond; //Ra 2014-07: trzeba to jeszcze wyj¹æ z kabiny...
                     // Ra 2014-07: no nie ma potrzeby szukaæ tego w ka¿dej klatce
            tmp = NULL;
            if ((TestFlag(mvControlled->Couplers[1].CouplingFlag, ctrain_controll)) &&
                (mvOccupied->ActiveCab > 0))
                tmp = DynamicObject->NextConnected;
            if ((TestFlag(mvControlled->Couplers[0].CouplingFlag, ctrain_controll)) &&
                (mvOccupied->ActiveCab < 0))
                tmp = DynamicObject->PrevConnected;

            if (tmp)
                if (tmp->MoverParameters->Mains)
                {
                    btLampkaWylSzybkiB.TurnOn();
                    btLampkaOporyB.Turn(tmp->MoverParameters->ResistorsFlagCheck());
                    btLampkaBezoporowaB.Turn(
                        tmp->MoverParameters->ResistorsFlagCheck() ||
                        (tmp->MoverParameters->MainCtrlActualPos == 0)); // do EU04
                    if ((tmp->MoverParameters->Itot != 0) ||
                        (tmp->MoverParameters->BrakePress > 0.2) ||
                        (tmp->MoverParameters->PipePress < 0.36))
                        btLampkaStycznB.TurnOff(); //
                    else if (tmp->MoverParameters->BrakePress < 0.1)
                        btLampkaStycznB.TurnOn(); // mozna prowadzic rozruch

                    //-----------------
                    //        //hunter-271211: brak jazdy w drugim czlonie, gdy w pierwszym tez nie
                    //        ma (i odwrotnie) - Ra: tutaj? w kabinie?
                    //        if (tmp->MoverParameters->TrainType!=dt_EZT)
                    //         if (((tmp->MoverParameters->BrakePress > 2) || (
                    //         tmp->MoverParameters->PipePress < 3.6 )) && (
                    //         tmp->MoverParameters->MainCtrlPos != 0 ))
                    //          {
                    //           tmp->MoverParameters->MainCtrlActualPos=0; //inaczej StLinFlag nie
                    //           zmienia sie na false w drugim pojezdzie
                    //           //tmp->MoverParameters->StLinFlag=true;
                    //           mvControlled->StLinFlag=true;
                    //          }
                    //        if (mvControlled->StLinFlag==true)
                    //         tmp->MoverParameters->MainCtrlActualPos=0;
                    //         //tmp->MoverParameters->StLinFlag=true;

                    //-----------------
                    // hunter-271211: sygnalizacja poslizgu w pierwszym pojezdzie, gdy wystapi w
                    // drugim
                    btLampkaPoslizg.Turn(tmp->MoverParameters->SlippingWheels);
                    //-----------------

                    btLampkaSprezarkaB.Turn(
                        tmp->MoverParameters->CompressorFlag); // mutopsitka dziala
                    if ((tmp->MoverParameters->BrakePress >= 0.145f * 10) &&
                        (mvControlled->Battery == true) && (mvControlled->Signalling == true))
                    {
                        btLampkaHamowanie2zes.TurnOn();
                    }
                    if ((tmp->MoverParameters->BrakePress < 0.075f * 10) ||
                        (mvControlled->Battery == false) || (mvControlled->Signalling == false))
                    {
                        btLampkaHamowanie2zes.TurnOff();
                    }
                    btLampkaNadmPrzetwB.Turn(
                        tmp->MoverParameters->ConvOvldFlag); // nadmiarowy przetwornicy?
                    btLampkaPrzetwB.Turn(
                        !tmp->MoverParameters->ConverterFlag); // zalaczenie przetwornicy
                }
                else // wylaczone
                {
                    btLampkaWylSzybkiB.TurnOff();
                    btLampkaOporyB.TurnOff();
                    btLampkaStycznB.TurnOff();
                    btLampkaSprezarkaB.TurnOff();
                    btLampkaBezoporowaB.TurnOff();
                    btLampkaHamowanie2zes.TurnOff();
                    btLampkaNadmPrzetwB.TurnOn();
                    btLampkaPrzetwB.TurnOff();
                }

            // hunter-261211: jakis stary kod (i niezgodny z prawda), zahaszowalem
            // if (tmp)
            // if (tmp->MoverParameters->ConverterFlag==true)
            //     btLampkaNadmPrzetwB.TurnOff();
            // else
            //     btLampkaNadmPrzetwB.TurnOn();

        } //**************************************************** */
        if (mvControlled->Battery)
        {
            switch (mvControlled->TrainType)
            { // zale¿nie od typu lokomotywy
            case dt_EZT:
                btLampkaHamienie.Turn((mvControlled->BrakePress >= 0.2) &&
                                      mvControlled->Signalling);
                break;
            case dt_ET41: // odhamowanie drugiego cz³onu
                if (mvSecond) // bo mo¿e komuœ przyjœæ do g³owy je¿d¿enie jednym cz³onem
                    btLampkaHamienie.Turn(mvSecond->BrakePress < 0.4);
                break;
            default:
                btLampkaHamienie.Turn((mvOccupied->BrakePress >= 0.1) ||
                                      mvControlled->DynamicBrakeFlag);
            }
            // KURS90
            btLampkaMaxSila.Turn(abs(mvControlled->Im) >= 350);
            btLampkaPrzekrMaxSila.Turn(abs(mvControlled->Im) >= 450);
            btLampkaRadio.Turn(mvControlled->Radio);
            btLampkaHamulecReczny.Turn(mvOccupied->ManualBrakePos > 0);
            // NBMX wrzesien 2003 - drzwi oraz sygna³ odjazdu
            btLampkaDoorLeft.Turn(mvOccupied->DoorLeftOpened);
            btLampkaDoorRight.Turn(mvOccupied->DoorRightOpened);
            btLampkaNapNastHam.Turn((mvControlled->ActiveDir != 0) &&
                                    (mvOccupied->EpFuse)); // napiecie na nastawniku hamulcowym
            btLampkaForward.Turn(mvControlled->ActiveDir > 0); // jazda do przodu
            btLampkaBackward.Turn(mvControlled->ActiveDir < 0); // jazda do ty³u
        }
        else
        { // gdy bateria wy³¹czona
            btLampkaHamienie.TurnOff();
            btLampkaMaxSila.TurnOff();
            btLampkaPrzekrMaxSila.TurnOff();
            btLampkaRadio.TurnOff();
            btLampkaHamulecReczny.TurnOff();
            btLampkaDoorLeft.TurnOff();
            btLampkaDoorRight.TurnOff();
            btLampkaNapNastHam.TurnOff();
            btLampkaForward.TurnOff();
            btLampkaBackward.TurnOff();
        }
        // McZapkie-080602: obroty (albo translacje) regulatorow
        if (ggMainCtrl.SubModel)
        {
            if (mvControlled->CoupledCtrl)
                ggMainCtrl.UpdateValue(
                    double(mvControlled->MainCtrlPos + mvControlled->ScndCtrlPos));
            else
                ggMainCtrl.UpdateValue(double(mvControlled->MainCtrlPos));
            ggMainCtrl.Update();
        }
        if (ggMainCtrlAct.SubModel)
        {
            if (mvControlled->CoupledCtrl)
                ggMainCtrlAct.UpdateValue(
                    double(mvControlled->MainCtrlActualPos + mvControlled->ScndCtrlActualPos));
            else
                ggMainCtrlAct.UpdateValue(double(mvControlled->MainCtrlActualPos));
            ggMainCtrlAct.Update();
        }
        if (ggScndCtrl.SubModel)
        { // Ra: od byte odejmowane boolean i konwertowane potem na double?
            ggScndCtrl.UpdateValue(
                double(mvControlled->ScndCtrlPos -
                       ((mvControlled->TrainType == dt_ET42) && mvControlled->DynamicBrakeFlag)));
            ggScndCtrl.Update();
        }
        if (ggDirKey.SubModel)
        {
            if (mvControlled->TrainType != dt_EZT)
                ggDirKey.UpdateValue(double(mvControlled->ActiveDir));
            else
                ggDirKey.UpdateValue(double(mvControlled->ActiveDir) +
                                     double(mvControlled->Imin == mvControlled->IminHi));
            ggDirKey.Update();
        }
        if (ggBrakeCtrl.SubModel)
        {
            if (DynamicObject->Mechanik ?
                    (DynamicObject->Mechanik->AIControllFlag ? false : Global::iFeedbackMode == 4) :
                    false) // nie blokujemy AI
            { // Ra: nie najlepsze miejsce, ale na pocz¹tek gdzieœ to daæ trzeba
                double b = Console::AnalogGet(0); // odczyt z pulpitu i modyfikacja pozycji kranu
                if ((b >= 0.0) && ((mvOccupied->BrakeHandle == FV4a) ||
                                   (mvOccupied->BrakeHandle ==
                                    FVel6))) // mo¿e mo¿na usun¹æ ograniczenie do FV4a i FVel6?
                {
                    b = (((Global::fCalibrateIn[0][3] * b) + Global::fCalibrateIn[0][2]) * b +
                         Global::fCalibrateIn[0][1]) *
                            b +
                        Global::fCalibrateIn[0][0];
                    if (b < -2.0)
                        b = -2.0;
                    else if (b > mvOccupied->BrakeCtrlPosNo)
                        b = mvOccupied->BrakeCtrlPosNo;
                    ggBrakeCtrl.UpdateValue(b); // przesów bez zaokr¹glenia
                    mvOccupied->BrakeLevelSet(b);
                }
                // else //standardowa prodedura z kranem powi¹zanym z klawiatur¹
                // ggBrakeCtrl.UpdateValue(double(mvOccupied->BrakeCtrlPos));
            }
            // else //standardowa prodedura z kranem powi¹zanym z klawiatur¹
            // ggBrakeCtrl.UpdateValue(double(mvOccupied->BrakeCtrlPos));
            ggBrakeCtrl.UpdateValue(mvOccupied->fBrakeCtrlPos);
            ggBrakeCtrl.Update();
        }
        if (ggLocalBrake.SubModel)
        {
            if (DynamicObject->Mechanik ?
                    (DynamicObject->Mechanik->AIControllFlag ? false : Global::iFeedbackMode == 4) :
                    false) // nie blokujemy AI
            { // Ra: nie najlepsze miejsce, ale na pocz¹tek gdzieœ to daæ trzeba
                double b = Console::AnalogGet(1); // odczyt z pulpitu i modyfikacja pozycji kranu
                if ((b >= 0.0) && (mvOccupied->BrakeLocHandle == FD1))
                {
                    b = (((Global::fCalibrateIn[1][3] * b) + Global::fCalibrateIn[1][2]) * b +
                         Global::fCalibrateIn[1][1]) *
                            b +
                        Global::fCalibrateIn[1][0];
                    if (b < 0.0)
                        b = 0.0;
                    else if (b > Hamulce::LocalBrakePosNo)
                        b = Hamulce::LocalBrakePosNo;
                    ggLocalBrake.UpdateValue(b); // przesów bez zaokr¹glenia
                    mvOccupied->LocalBrakePos =
                        int(1.09 * b); // sposób zaokr¹glania jest do ustalenia
                }
                else // standardowa prodedura z kranem powi¹zanym z klawiatur¹
                    ggLocalBrake.UpdateValue(double(mvOccupied->LocalBrakePos));
            }
            else // standardowa prodedura z kranem powi¹zanym z klawiatur¹
                ggLocalBrake.UpdateValue(double(mvOccupied->LocalBrakePos));
            ggLocalBrake.Update();
        }
        if (ggManualBrake.SubModel != NULL)
        {
            ggManualBrake.UpdateValue(double(mvOccupied->ManualBrakePos));
            ggManualBrake.Update();
        }
        if (ggBrakeProfileCtrl.SubModel)
        {
            ggBrakeProfileCtrl.UpdateValue(
                double(mvOccupied->BrakeDelayFlag == 4 ? 2 : mvOccupied->BrakeDelayFlag - 1));
            ggBrakeProfileCtrl.Update();
        }
        if (ggBrakeProfileG.SubModel)
        {
            ggBrakeProfileG.UpdateValue(double(mvOccupied->BrakeDelayFlag == bdelay_G ? 1 : 0));
            ggBrakeProfileG.Update();
        }
        if (ggBrakeProfileR.SubModel)
        {
            ggBrakeProfileR.UpdateValue(double(mvOccupied->BrakeDelayFlag == bdelay_R ? 1 : 0));
            ggBrakeProfileR.Update();
        }

        if (ggMaxCurrentCtrl.SubModel)
        {
            ggMaxCurrentCtrl.UpdateValue(double(mvControlled->Imax == mvControlled->ImaxHi));
            ggMaxCurrentCtrl.Update();
        }

        // NBMX wrzesien 2003 - drzwi
        if (ggDoorLeftButton.SubModel)
        {
            ggDoorLeftButton.PutValue(mvOccupied->DoorLeftOpened ? 1 : 0);
            ggDoorLeftButton.Update();
        }
        if (ggDoorRightButton.SubModel)
        {
            ggDoorRightButton.PutValue(mvOccupied->DoorRightOpened ? 1 : 0);
            ggDoorRightButton.Update();
        }
        if (ggDepartureSignalButton.SubModel)
        {
            //      ggDepartureSignalButton.UpdateValue(double());
            ggDepartureSignalButton.Update();
        }

        // NBMX dzwignia sprezarki
        if (ggCompressorButton.SubModel) // hunter-261211: poprawka
            ggCompressorButton.Update();
        if (ggMainButton.SubModel)
            ggMainButton.Update();
        if (ggRadioButton.SubModel)
        {
            ggRadioButton.PutValue(mvControlled->Radio ? 1 : 0);
            ggRadioButton.Update();
        }
        if (ggConverterButton.SubModel)
            ggConverterButton.Update();
        if (ggConverterOffButton.SubModel)
            ggConverterOffButton.Update();

        if (((DynamicObject->iLights[0]) == 0) && ((DynamicObject->iLights[1]) == 0))
        {
            ggRightLightButton.PutValue(0);
            ggLeftLightButton.PutValue(0);
            ggUpperLightButton.PutValue(0);
            ggRightEndLightButton.PutValue(0);
            ggLeftEndLightButton.PutValue(0);
        }

        //---------
        // hunter-101211: poprawka na zle obracajace sie przelaczniki
        /*
        if (((DynamicObject->iLights[0]&1)==1)
         ||((DynamicObject->iLights[1]&1)==1))
           ggLeftLightButton.PutValue(1);
        if (((DynamicObject->iLights[0]&16)==16)
         ||((DynamicObject->iLights[1]&16)==16))
           ggRightLightButton.PutValue(1);
        if (((DynamicObject->iLights[0]&4)==4)
         ||((DynamicObject->iLights[1]&4)==4))
           ggUpperLightButton.PutValue(1);

        if (((DynamicObject->iLights[0]&2)==2)
         ||((DynamicObject->iLights[1]&2)==2))
           if (ggLeftEndLightButton.SubModel)
           {
              ggLeftEndLightButton.PutValue(1);
              ggLeftLightButton.PutValue(0);
           }
           else
              ggLeftLightButton.PutValue(-1);

        if (((DynamicObject->iLights[0]&32)==32)
         ||((DynamicObject->iLights[1]&32)==32))
           if (ggRightEndLightButton.SubModel)
           {
              ggRightEndLightButton.PutValue(1);
              ggRightLightButton.PutValue(0);
           }
           else
              ggRightLightButton.PutValue(-1);
         */

        //--------------
        // hunter-230112

        // REFLEKTOR LEWY
        // glowne oswietlenie
        if ((DynamicObject->iLights[0] & 1) == 1)
            if ((mvOccupied->ActiveCab) == 1)
                ggLeftLightButton.PutValue(1);
            else if ((mvOccupied->ActiveCab) == -1)
                ggRearLeftLightButton.PutValue(1);

        if ((DynamicObject->iLights[1] & 1) == 1)
            if ((mvOccupied->ActiveCab) == -1)
                ggLeftLightButton.PutValue(1);
            else if ((mvOccupied->ActiveCab) == 1)
                ggRearLeftLightButton.PutValue(1);

        // koñcówki
        if ((DynamicObject->iLights[0] & 2) == 2)
            if ((mvOccupied->ActiveCab) == 1)
            {
                if (ggLeftEndLightButton.SubModel)
                {
                    ggLeftEndLightButton.PutValue(1);
                    ggLeftLightButton.PutValue(0);
                }
                else
                    ggLeftLightButton.PutValue(-1);
            }
            else if ((mvOccupied->ActiveCab) == -1)
            {
                if (ggRearLeftEndLightButton.SubModel)
                {
                    ggRearLeftEndLightButton.PutValue(1);
                    ggRearLeftLightButton.PutValue(0);
                }
                else
                    ggRearLeftLightButton.PutValue(-1);
            }

        if ((DynamicObject->iLights[1] & 2) == 2)
            if ((mvOccupied->ActiveCab) == -1)
            {
                if (ggLeftEndLightButton.SubModel)
                {
                    ggLeftEndLightButton.PutValue(1);
                    ggLeftLightButton.PutValue(0);
                }
                else
                    ggLeftLightButton.PutValue(-1);
            }
            else if ((mvOccupied->ActiveCab) == 1)
            {
                if (ggRearLeftEndLightButton.SubModel)
                {
                    ggRearLeftEndLightButton.PutValue(1);
                    ggRearLeftLightButton.PutValue(0);
                }
                else
                    ggRearLeftLightButton.PutValue(-1);
            }
        //--------------
        // REFLEKTOR GORNY
        if ((DynamicObject->iLights[0] & 4) == 4)
            if ((mvOccupied->ActiveCab) == 1)
                ggUpperLightButton.PutValue(1);
            else if ((mvOccupied->ActiveCab) == -1)
                ggRearUpperLightButton.PutValue(1);

        if ((DynamicObject->iLights[1] & 4) == 4)
            if ((mvOccupied->ActiveCab) == -1)
                ggUpperLightButton.PutValue(1);
            else if ((mvOccupied->ActiveCab) == 1)
                ggRearUpperLightButton.PutValue(1);
        //--------------
        // REFLEKTOR PRAWY
        // g³ówne oœwietlenie
        if ((DynamicObject->iLights[0] & 16) == 16)
            if ((mvOccupied->ActiveCab) == 1)
                ggRightLightButton.PutValue(1);
            else if ((mvOccupied->ActiveCab) == -1)
                ggRearRightLightButton.PutValue(1);

        if ((DynamicObject->iLights[1] & 16) == 16)
            if ((mvOccupied->ActiveCab) == -1)
                ggRightLightButton.PutValue(1);
            else if ((mvOccupied->ActiveCab) == 1)
                ggRearRightLightButton.PutValue(1);

        // koñcówki
        if ((DynamicObject->iLights[0] & 32) == 32)
            if ((mvOccupied->ActiveCab) == 1)
            {
                if (ggRightEndLightButton.SubModel)
                {
                    ggRightEndLightButton.PutValue(1);
                    ggRightLightButton.PutValue(0);
                }
                else
                    ggRightLightButton.PutValue(-1);
            }
            else if ((mvOccupied->ActiveCab) == -1)
            {
                if (ggRearRightEndLightButton.SubModel)
                {
                    ggRearRightEndLightButton.PutValue(1);
                    ggRearRightLightButton.PutValue(0);
                }
                else
                    ggRearRightLightButton.PutValue(-1);
            }

        if ((DynamicObject->iLights[1] & 32) == 32)
            if ((mvOccupied->ActiveCab) == -1)
            {
                if (ggRightEndLightButton.SubModel)
                {
                    ggRightEndLightButton.PutValue(1);
                    ggRightLightButton.PutValue(0);
                }
                else
                    ggRightLightButton.PutValue(-1);
            }
            else if ((mvOccupied->ActiveCab) == 1)
            {
                if (ggRearRightEndLightButton.SubModel)
                {
                    ggRearRightEndLightButton.PutValue(1);
                    ggRearRightLightButton.PutValue(0);
                }
                else
                    ggRearRightLightButton.PutValue(-1);
            }

        //---------
        // Winger 010304 - pantografy
        if (ggPantFrontButton.SubModel)
        {
            if (mvControlled->PantFrontUp)
                ggPantFrontButton.PutValue(1);
            else
                ggPantFrontButton.PutValue(0);
            ggPantFrontButton.Update();
        }
        if (ggPantRearButton.SubModel)
        {
            ggPantRearButton.PutValue(mvControlled->PantRearUp ? 1 : 0);
            ggPantRearButton.Update();
        }
        if (ggPantFrontButtonOff.SubModel)
        {
            ggPantFrontButtonOff.Update();
        }
        // Winger 020304 - ogrzewanie
        //----------
        // hunter-080812: poprawka na ogrzewanie w elektrykach - usuniete uzaleznienie od
        // przetwornicy
        if (ggTrainHeatingButton.SubModel)
        {
            if (mvControlled->Heating)
            {
                ggTrainHeatingButton.PutValue(1);
                // if (mvControlled->ConverterFlag==true)
                // btLampkaOgrzewanieSkladu.TurnOn();
            }
            else
            {
                ggTrainHeatingButton.PutValue(0);
                // btLampkaOgrzewanieSkladu.TurnOff();
            }
            ggTrainHeatingButton.Update();
        }
        if (ggSignallingButton.SubModel != NULL)
        {
            ggSignallingButton.PutValue(mvControlled->Signalling ? 1 : 0);
            ggSignallingButton.Update();
        }
        if (ggDoorSignallingButton.SubModel != NULL)
        {
            ggDoorSignallingButton.PutValue(mvControlled->DoorSignalling ? 1 : 0);
            ggDoorSignallingButton.Update();
        }
        // if (ggDistCounter.SubModel)
        //{//Ra 2014-07: licznik kilometrów
        // ggDistCounter.PutValue(mvControlled->DistCounter);
        // ggDistCounter.Update();
        //}
        if ((((mvControlled->EngineType == ElectricSeriesMotor) && (mvControlled->Mains == true) &&
              (mvControlled->ConvOvldFlag == false)) ||
             (mvControlled->ConverterFlag)) &&
            (mvControlled->Heating == true))
            btLampkaOgrzewanieSkladu.TurnOn();
        else
            btLampkaOgrzewanieSkladu.TurnOff();

        //----------
        // hunter-261211: jakis stary kod (i niezgodny z prawda),
        // zahaszowalem i poprawilem
        // youBy-220913: ale przyda sie do lampki samej przetwornicy
        btLampkaPrzetw.Turn(!mvControlled->ConverterFlag);
        btLampkaNadmPrzetw.Turn(mvControlled->ConvOvldFlag);
        //----------

        // McZapkie-141102: SHP i czuwak, TODO: sygnalizacja kabinowa
        if (mvOccupied->SecuritySystem.Status > 0)
        {
            if (fBlinkTimer > fCzuwakBlink)
                fBlinkTimer = -fCzuwakBlink;
            else
                fBlinkTimer += dt;

            // hunter-091012: dodanie testu czuwaka
            if ((TestFlag(mvOccupied->SecuritySystem.Status, s_aware)) ||
                (TestFlag(mvOccupied->SecuritySystem.Status, s_CAtest)))
            {
                btLampkaCzuwaka.Turn(fBlinkTimer > 0);
            }
            else
                btLampkaCzuwaka.TurnOff();
            btLampkaSHP.Turn(TestFlag(mvOccupied->SecuritySystem.Status, s_active));

            // hunter-091012: rozdzielenie alarmow
            // if (TestFlag(mvOccupied->SecuritySystem.Status,s_alarm))
            if (TestFlag(mvOccupied->SecuritySystem.Status, s_CAalarm) ||
                TestFlag(mvOccupied->SecuritySystem.Status, s_SHPalarm))
            {
                dsbBuzzer->GetStatus(&stat);
                if (!(stat & DSBSTATUS_PLAYING))
                    dsbBuzzer->Play(0, 0, DSBPLAY_LOOPING);
            }
            else
            {
                dsbBuzzer->GetStatus(&stat);
                if (stat & DSBSTATUS_PLAYING)
                    dsbBuzzer->Stop();
            }
        }
        else // wylaczone
        {
            btLampkaCzuwaka.TurnOff();
            btLampkaSHP.TurnOff();
            dsbBuzzer->GetStatus(&stat);
            if (stat & DSBSTATUS_PLAYING)
                dsbBuzzer->Stop();
        }

        //******************************************
        // przelaczniki

        if (Console::Pressed(Global::Keys[k_Horn]))
        {
            if (Console::Pressed(VK_SHIFT))
            {
                SetFlag(mvOccupied->WarningSignal, 2);
                mvOccupied->WarningSignal &= (255 - 1);
                if (ggHornButton.SubModel)
                    ggHornButton.UpdateValue(1);
            }
            else
            {
                SetFlag(mvOccupied->WarningSignal, 1);
                mvOccupied->WarningSignal &= (255 - 2);
                if (ggHornButton.SubModel)
                    ggHornButton.UpdateValue(-1);
            }
        }
        else
        {
            mvOccupied->WarningSignal = 0;
            if (ggHornButton.SubModel)
                ggHornButton.UpdateValue(0);
        }

        if (Console::Pressed(Global::Keys[k_Horn2]))
            if (Global::Keys[k_Horn2] != Global::Keys[k_Horn])
            {
                SetFlag(mvOccupied->WarningSignal, 2);
            }

        //----------------
        // hunter-141211: wyl. szybki zalaczony i wylaczony przeniesiony z OnKeyPress()
        if (Console::Pressed(VK_SHIFT) && Console::Pressed(Global::Keys[k_Main]))
        {
            fMainRelayTimer += dt;
            ggMainOnButton.PutValue(1);
            if (mvControlled->Mains != true) // hunter-080812: poprawka
                mvControlled->ConverterSwitch(false);
            if (fMainRelayTimer > mvControlled->InitialCtrlDelay) // wlaczanie WSa z opoznieniem
                if (mvControlled->MainSwitch(true))
                {
                    //        if (mvControlled->MainCtrlPos!=0) //zabezpieczenie, by po wrzuceniu
                    //        pozycji przed wlaczonym
                    //         mvControlled->StLinFlag=true; //WSem nie wrzucilo na ta pozycje po
                    //         jego zalaczeniu //yBARC - co to tutaj robi?!
                    if (mvControlled->EngineType == DieselEngine)
                        dsbDieselIgnition->Play(0, 0, 0);
                }
        }
        else
        {
            if (ggConverterButton.GetValue() !=
                0) // po puszczeniu przycisku od WSa odpalanie potwora
                mvControlled->ConverterSwitch(true);
            // hunter-091012: przeniesione z mover.pas, zeby dzwiek sie nie zapetlal, drugi warunek
            // zeby nie odtwarzalo w nieskonczonosc i przeniesienie zerowania timera
            if ((mvControlled->Mains != true) && (fMainRelayTimer > 0))
            {
                dsbRelay->Play(0, 0, 0);
                fMainRelayTimer = 0;
            }
            ggMainOnButton.UpdateValue(0);
        }
        //---

        if (!Console::Pressed(VK_SHIFT) && Console::Pressed(Global::Keys[k_Main]))
        {
            ggMainOffButton.PutValue(1);
            if (mvControlled->MainSwitch(false))
                dsbRelay->Play(0, 0, 0);
        }
        else
            ggMainOffButton.UpdateValue(0);

        /* if (cKey==Global::Keys[k_Main])     //z shiftem
         {
            ggMainOnButton.PutValue(1);
            if (mvControlled->MainSwitch(true))
              {
                 if (mvControlled->MainCtrlPos!=0) //hunter-131211: takie zabezpieczenie
                  mvControlled->StLinFlag=true;

                 if (mvControlled->EngineType==DieselEngine)
                  dsbDieselIgnition->Play(0,0,0);
                 else
                  dsbNastawnikJazdy->Play(0,0,0);
              }
         }
         else */

        /* if (cKey==Global::Keys[k_Main])    //bez shifta
        {
          ggMainOffButton.PutValue(1);
          if (mvControlled->MainSwitch(false))
             {
                dsbNastawnikJazdy->Play(0,0,0);
             }
        }
        else */

        //----------------
        // hunter-131211: czuwak przeniesiony z OnKeyPress
        // hunter-091012: zrobiony test czuwaka
        if (Console::Pressed(Global::Keys[k_Czuwak]))
        { // czuwak testuje kierunek, ale podobno w EZT nie, wiêc mo¿e byæ w rozrz¹dczym
            fCzuwakTestTimer += dt;
            ggSecurityResetButton.PutValue(1);
            if (CAflag == false)
            {
                CAflag = true;
                mvOccupied->SecuritySystemReset();
            }
            else if (fCzuwakTestTimer > 1.0)
                SetFlag(mvOccupied->SecuritySystem.Status, s_CAtest);
        }
        else
        {
            fCzuwakTestTimer = 0;
            ggSecurityResetButton.UpdateValue(0);
            if (TestFlag(mvOccupied->SecuritySystem.Status,
                         s_CAtest)) //&&(!TestFlag(mvControlled->SecuritySystem.Status,s_CAebrake)))
            {
                SetFlag(mvOccupied->SecuritySystem.Status, -s_CAtest);
                mvOccupied->s_CAtestebrake = false;
                mvOccupied->SecuritySystem.SystemBrakeCATestTimer = 0;
                //        if ((!TestFlag(mvOccupied->SecuritySystem.Status,s_SHPebrake))
                //         ||(!TestFlag(mvOccupied->SecuritySystem.Status,s_CAebrake)))
                //        mvControlled->EmergencyBrakeFlag=false; //YB-HN
            }
            CAflag = false;
        }
        /*
        if ( Console::Pressed(Global::Keys[k_Czuwak]) )
        {
         ggSecurityResetButton.PutValue(1);
         if ((mvOccupied->SecuritySystem.Status&s_aware)&&
             (mvOccupied->SecuritySystem.Status&s_active))
          {
           mvOccupied->SecuritySystem.SystemTimer=0;
           mvOccupied->SecuritySystem.Status-=s_aware;
           mvOccupied->SecuritySystem.VelocityAllowed=-1;
           CAflag=1;
          }
         else if (CAflag!=1)
           mvOccupied->SecuritySystemReset();
        }
        else
        {
         ggSecurityResetButton.UpdateValue(0);
         CAflag=0;
        }
        */

        //-----------------
        // hunter-201211: piasecznica przeniesiona z OnKeyPress, wlacza sie tylko,
        // gdy trzymamy przycisk, a nie tak jak wczesniej (raz nacisnelo sie 's'
        // i sypala caly czas)
        /*
        if (cKey==Global::Keys[k_Sand])
        {
            if (mvControlled->SandDoseOn())
             if (mvControlled->SandDose)
              {
                dsbPneumaticRelay->SetVolume(-30);
                dsbPneumaticRelay->Play(0,0,0);
              }
        }
       */

        if (Console::Pressed(Global::Keys[k_Sand]))
        {
            mvControlled->SandDose = true;
            // mvControlled->SandDoseOn(true);
        }
        else
        {
            mvControlled->SandDose = false;
            // mvControlled->SandDoseOn(false);
            // dsbPneumaticRelay->SetVolume(-30);
            // dsbPneumaticRelay->Play(0,0,0);
        }

        //-----------------
        // hunter-221211: hamowanie przy poslizgu
        if (Console::Pressed(Global::Keys[k_AntiSlipping]))
        {
            if (mvControlled->BrakeSystem != ElectroPneumatic)
            {
                ggAntiSlipButton.PutValue(1);
                mvControlled->AntiSlippingBrake();
            }
        }
        else
            ggAntiSlipButton.UpdateValue(0);
        //-----------------
        // hunter-261211: przetwornica i sprezarka
        if (Console::Pressed(VK_SHIFT) &&
            Console::Pressed(Global::Keys[k_Converter])) // NBMX 14-09-2003: przetwornica wl
        { //(mvControlled->CompressorPower<2)
            ggConverterButton.PutValue(1);
            if ((mvControlled->PantFrontVolt != 0.0) || (mvControlled->PantRearVolt != 0.0) ||
                (mvControlled->EnginePowerSource.SourceType !=
                 CurrentCollector) /*||(!Global::bLiveTraction)*/)
                mvControlled->ConverterSwitch(true);
            // if
            // ((mvControlled->EngineType!=ElectricSeriesMotor)&&(mvControlled->TrainType!=dt_EZT))
            // //hunter-110212: poprawka dla EZT
            if (mvControlled->CompressorPower == 2) // hunter-091012: tak jest poprawnie
                mvControlled->CompressorSwitch(true);
        }
        else
        {
            if (mvControlled->ConvSwitchType == "impulse")
            {
                ggConverterButton.PutValue(0);
                ggConverterOffButton.PutValue(0);
            }
        }

        //     if (
        //     Console::Pressed(VK_SHIFT)&&Console::Pressed(Global::Keys[k_Compressor])&&((mvControlled->EngineType==ElectricSeriesMotor)||(mvControlled->TrainType==dt_EZT))
        //     )   //NBMX 14-09-2003: sprezarka wl
        if (Console::Pressed(VK_SHIFT) && Console::Pressed(Global::Keys[k_Compressor]) &&
            (mvControlled->CompressorPower < 2)) // hunter-091012: tak jest poprawnie
        { // hunter-110212: poprawka dla EZT
            ggCompressorButton.PutValue(1);
            mvControlled->CompressorSwitch(true);
        }

        if (!Console::Pressed(VK_SHIFT) &&
            Console::Pressed(Global::Keys[k_Converter])) // NBMX 14-09-2003: przetwornica wl
        {
            ggConverterButton.PutValue(0);
            ggConverterOffButton.PutValue(1);
            mvControlled->ConverterSwitch(false);
        }

        //     if (
        //     !Console::Pressed(VK_SHIFT)&&Console::Pressed(Global::Keys[k_Compressor])&&((mvControlled->EngineType==ElectricSeriesMotor)||(mvControlled->TrainType==dt_EZT))
        //     )   //NBMX 14-09-2003: sprezarka wl
        if (!Console::Pressed(VK_SHIFT) && Console::Pressed(Global::Keys[k_Compressor]) &&
            (mvControlled->CompressorPower < 2)) // hunter-091012: tak jest poprawnie
        { // hunter-110212: poprawka dla EZT
            ggCompressorButton.PutValue(0);
            mvControlled->CompressorSwitch(false);
        }

        /*
        bez szifta
        if (cKey==Global::Keys[k_Converter])       //NBMX wyl przetwornicy
        {
             if (mvControlled->ConverterSwitch(false))
             {
              dsbSwitch->SetVolume(DSBVOLUME_MAX);
              dsbSwitch->Play(0,0,0);;
             }
        }
        else
        if (cKey==Global::Keys[k_Compressor])       //NBMX wyl sprezarka
        {
             if (mvControlled->CompressorSwitch(false))
             {
              dsbSwitch->SetVolume(DSBVOLUME_MAX);
              dsbSwitch->Play(0,0,0);;
             }
        }
        else
        */
        //-----------------

        if (!FreeFlyModeFlag)
        {
            if (Console::Pressed(Global::Keys[k_Releaser])) // yB: odluzniacz caly czas trzymany,
                                                            // warunki powinny byc takie same, jak
                                                            // przy naciskaniu. Wlasciwie stamtad
                                                            // mozna wyrzucic sprawdzanie
                                                            // nacisniecia.
            {
                if ((mvControlled->EngineType == ElectricSeriesMotor) ||
                    (mvControlled->EngineType == DieselElectric))
                    if (mvControlled->TrainType != dt_EZT)
                        if ((mvOccupied->BrakeCtrlPosNo > 0) && (mvControlled->ActiveDir != 0))
                        {
                            ggReleaserButton.PutValue(1);
                            mvOccupied->BrakeReleaser(1);
                        }
            } // releaser
            else
                mvOccupied->BrakeReleaser(0);
        } // FFMF

        if (Console::Pressed(Global::Keys[k_Univ1]))
        {
            if (!DebugModeFlag)
            {
                if (ggUniversal1Button.SubModel)
                    if (Console::Pressed(VK_SHIFT))
                        ggUniversal1Button.IncValue(dt / 2);
                    else
                        ggUniversal1Button.DecValue(dt / 2);
            }
        }
        if (!Console::Pressed(Global::Keys[k_SmallCompressor]))
            // Ra: przecieœæ to na zwolnienie klawisza
            if (DynamicObject->Mechanik ? !DynamicObject->Mechanik->AIControllFlag :
                                          false) // nie wy³¹czaæ, gdy AI
                mvControlled->PantCompFlag = false; // wy³¹czona, gdy nie trzymamy klawisza
        if (Console::Pressed(Global::Keys[k_Univ2]))
        {
            if (!DebugModeFlag)
            {
                if (ggUniversal2Button.SubModel)
                    if (Console::Pressed(VK_SHIFT))
                        ggUniversal2Button.IncValue(dt / 2);
                    else
                        ggUniversal2Button.DecValue(dt / 2);
            }
        }

        // hunter-091012: zrobione z uwzglednieniem przelacznika swiatla
        if (Console::Pressed(Global::Keys[k_Univ3]))
        {
            if (Console::Pressed(VK_SHIFT))
            {
                if (Console::Pressed(VK_CONTROL))
                {
                    bCabLight = true;
                    if (ggCabLightButton.SubModel)
                    {
                        ggCabLightButton.PutValue(1);
                        btCabLight.TurnOn();
                    }
                }
                else
                {
                    if (ggUniversal3Button.SubModel)
                    {
                        ggUniversal3Button.PutValue(1); // hunter-131211: z UpdateValue na PutValue
                                                        // - by zachowywal sie jak pozostale
                                                        // przelaczniki
                        if (btLampkaUniversal3.Active())
                            LampkaUniversal3_st = true;
                    }
                }
            }
            else
            {
                if (Console::Pressed(VK_CONTROL))
                {
                    bCabLight = false;
                    if (ggCabLightButton.SubModel)
                    {
                        ggCabLightButton.PutValue(0);
                        btCabLight.TurnOff();
                    }
                }
                else
                {
                    if (ggUniversal3Button.SubModel)
                    {
                        ggUniversal3Button.PutValue(0); // hunter-131211: z UpdateValue na PutValue
                                                        // - by zachowywal sie jak pozostale
                                                        // przelaczniki
                        if (btLampkaUniversal3.Active())
                            LampkaUniversal3_st = false;
                    }
                }
            }
        }

        // hunter-091012: to w ogole jest bez sensu i tak namodzone ze nie wiadomo o co chodzi -
        // zakomentowalem i zrobilem po swojemu
        /*
        if ( Console::Pressed(Global::Keys[k_Univ3]) )
        {
          if (ggUniversal3Button.SubModel)


           if (Console::Pressed(VK_CONTROL))
           {//z [Ctrl] zapalamy albo gasimy œwiate³ko w kabinie
     //tutaj jest bez sensu, trzeba reagowaæ na wciskanie klawisza!
            if (Console::Pressed(VK_SHIFT))
            {//zapalenie
             if (iCabLightFlag<2) ++iCabLightFlag;
            }
            else
            {//gaszenie
             if (iCabLightFlag) --iCabLightFlag;
            }

           }
           else
           {//bez [Ctrl] prze³¹czamy coœtem
            if (Console::Pressed(VK_SHIFT))
            {
             ggUniversal3Button.PutValue(1);  //hunter-131211: z UpdateValue na PutValue - by
     zachowywal sie jak pozostale przelaczniki
             if (btLampkaUniversal3.Active())
              LampkaUniversal3_st=true;
            }
            else
            {
             ggUniversal3Button.PutValue(0);  //hunter-131211: z UpdateValue na PutValue - by
     zachowywal sie jak pozostale przelaczniki
             if (btLampkaUniversal3.Active())
              LampkaUniversal3_st=false;
            }
           }
        } */

        // ABu030405 obsluga lampki uniwersalnej:
        if (btLampkaUniversal3.Active()) // w ogóle jest
            if (LampkaUniversal3_st) // za³¹czona
                switch (LampkaUniversal3_typ)
                {
                case 0:
                    btLampkaUniversal3.Turn(mvControlled->Battery);
                    break;
                case 1:
                    btLampkaUniversal3.Turn(mvControlled->Mains);
                    break;
                case 2:
                    btLampkaUniversal3.Turn(mvControlled->ConverterFlag);
                    break;
                default:
                    btLampkaUniversal3.TurnOff();
                }
            else
                btLampkaUniversal3.TurnOff();

        /*
        if (Console::Pressed(Global::Keys[k_Univ4]))
        {
           if (ggUniversal4Button.SubModel)
           if (Console::Pressed(VK_SHIFT))
           {
              ActiveUniversal4=true;
              //ggUniversal4Button.UpdateValue(1);
           }
           else
           {
              ActiveUniversal4=false;
              //ggUniversal4Button.UpdateValue(0);
           }
        }
        */

        // hunter-091012: przepisanie univ4 i zrobione z uwzglednieniem przelacznika swiatla
        if (Console::Pressed(Global::Keys[k_Univ4]))
        {
            if (Console::Pressed(VK_SHIFT))
            {
                if (Console::Pressed(VK_CONTROL))
                {
                    bCabLightDim = true;
                    if (ggCabLightDimButton.SubModel)
                    {
                        ggCabLightDimButton.PutValue(1);
                    }
                }
                else
                {
                    ActiveUniversal4 = true;
                    // ggUniversal4Button.UpdateValue(1);
                }
            }
            else
            {
                if (Console::Pressed(VK_CONTROL))
                {
                    bCabLightDim = false;
                    if (ggCabLightDimButton.SubModel)
                    {
                        ggCabLightDimButton.PutValue(0); // hunter-131211: z UpdateValue na PutValue
                                                         // - by zachowywal sie jak pozostale
                                                         // przelaczniki
                    }
                }
                else
                {
                    ActiveUniversal4 = false;
                    // ggUniversal4Button.UpdateValue(0);
                }
            }
        }
        // Odskakiwanie hamulce EP
        if ((!Console::Pressed(Global::Keys[k_DecBrakeLevel])) &&
            (!Console::Pressed(Global::Keys[k_WaveBrake])) && (mvOccupied->BrakeCtrlPos == -1) &&
            (mvOccupied->BrakeHandle == FVel6) && (DynamicObject->Controller != AIdriver) &&
            (Global::iFeedbackMode != 4))
        {
            // mvOccupied->BrakeCtrlPos=(mvOccupied->BrakeCtrlPos)+1;
            // mvOccupied->IncBrakeLevel();
            mvOccupied->BrakeLevelSet(mvOccupied->BrakeCtrlPos + 1);
            keybrakecount = 0;
            if ((mvOccupied->TrainType == dt_EZT) && (mvControlled->Mains) &&
                (mvControlled->ActiveDir != 0))
            {
                dsbPneumaticSwitch->SetVolume(-10);
                dsbPneumaticSwitch->Play(0, 0, 0);
            }
        }

        // Ra: przeklejka z SPKS - p³ynne poruszanie hamulcem
        // if ((mvOccupied->BrakeHandle==FV4a)&&(Console::Pressed(Global::Keys[k_IncBrakeLevel])))
        if ((Console::Pressed(Global::Keys[k_IncBrakeLevel])))
        {
            if (Console::Pressed(VK_CONTROL))
            {
                // mvOccupied->BrakeCtrlPos2-=dt/20.0;
                // if (mvOccupied->BrakeCtrlPos2<-1.5) mvOccupied->BrakeCtrlPos2=-1.5;
            }
            else
            {
                // mvOccupied->BrakeCtrlPosR+=(mvOccupied->BrakeCtrlPosR>mvOccupied->BrakeCtrlPosNo?0:dt*2);
                // mvOccupied->BrakeCtrlPos= floor(mvOccupied->BrakeCtrlPosR+0.499);
            }
        }
        // if ((mvOccupied->BrakeHandle==FV4a)&&(Console::Pressed(Global::Keys[k_DecBrakeLevel])))
        if ((Console::Pressed(Global::Keys[k_DecBrakeLevel])))
        {
            if (Console::Pressed(VK_CONTROL))
            {
                // mvOccupied->BrakeCtrlPos2+=(mvOccupied->BrakeCtrlPos2>2?0:dt/20.0);
                // if (mvOccupied->BrakeCtrlPos2<-3) mvOccupied->BrakeCtrlPos2=-3;
                // mvOccupied->BrakeLevelAdd(mvOccupied->fBrakeCtrlPos<-1?0:dt*2);
            }
            else
            {
                // mvOccupied->BrakeCtrlPosR-=(mvOccupied->BrakeCtrlPosR<-1?0:dt*2);
                // mvOccupied->BrakeCtrlPos= floor(mvOccupied->BrakeCtrlPosR+0.499);
                // mvOccupied->BrakeLevelAdd(mvOccupied->fBrakeCtrlPos<-1?0:-dt*2);
            }
        }

        if ((mvOccupied->BrakeHandle == FV4a) && (Console::Pressed(Global::Keys[k_IncBrakeLevel])))
        {
            if (Console::Pressed(VK_CONTROL))
            {
                mvOccupied->BrakeCtrlPos2 -= dt / 20.0;
                if (mvOccupied->BrakeCtrlPos2 < -1.5)
                    mvOccupied->BrakeCtrlPos2 = -1.5;
            }
            else
            {
                //        mvOccupied->BrakeCtrlPosR+=(mvOccupied->BrakeCtrlPosR>mvOccupied->BrakeCtrlPosNo?0:dt*2);
                mvOccupied->BrakeLevelAdd(dt * 2);
                //        mvOccupied->BrakeCtrlPos= floor(mvOccupied->BrakeCtrlPosR+0.499);
            }
        }

        if ((mvOccupied->BrakeHandle == FV4a) && (Console::Pressed(Global::Keys[k_DecBrakeLevel])))
        {
            if (Console::Pressed(VK_CONTROL))
            {
                mvOccupied->BrakeCtrlPos2 += (mvOccupied->BrakeCtrlPos2 > 2 ? 0 : dt / 20.0);
                if (mvOccupied->BrakeCtrlPos2 < -3)
                    mvOccupied->BrakeCtrlPos2 = -3;
            }
            else
            {
                //        mvOccupied->BrakeCtrlPosR-=(mvOccupied->BrakeCtrlPosR<-1?0:dt*2);
                //        mvOccupied->BrakeCtrlPos= floor(mvOccupied->BrakeCtrlPosR+0.499);
                mvOccupied->BrakeLevelAdd(-dt * 2);
            }
        }

        //    bool kEP;
        //    kEP=(mvOccupied->BrakeSubsystem==Knorr)||(mvOccupied->BrakeSubsystem==Hik)||(mvOccupied->BrakeSubsystem==Kk);
        if ((mvOccupied->BrakeSystem == ElectroPneumatic) && ((mvOccupied->BrakeHandle == St113)) &&
            (mvControlled->EpFuse == true))
            if (Console::Pressed(Global::Keys[k_AntiSlipping])) // kEP
            {
                ggAntiSlipButton.UpdateValue(1);
                if (mvOccupied->SwitchEPBrake(1))
                {
                    dsbPneumaticSwitch->SetVolume(-10);
                    dsbPneumaticSwitch->Play(0, 0, 0);
                }
            }
            else
            {
                if (mvOccupied->SwitchEPBrake(0))
                {
                    dsbPneumaticSwitch->SetVolume(-10);
                    dsbPneumaticSwitch->Play(0, 0, 0);
                }
            }

        if (Console::Pressed(Global::Keys[k_DepartureSignal]))
        {
            ggDepartureSignalButton.PutValue(1);
            btLampkaDepartureSignal.TurnOn();
            mvControlled->DepartureSignal = true;
        }
        else
        {
            btLampkaDepartureSignal.TurnOff();
            if (DynamicObject->Mechanik) // mo¿e nie byæ?
                if (!DynamicObject->Mechanik->AIControllFlag) // tylko jeœli nie prowadzi AI
                    mvControlled->DepartureSignal = false;
        }

        if (Console::Pressed(Global::Keys[k_Main])) //[]
        {
            if (Console::Pressed(VK_SHIFT))
                ggMainButton.PutValue(1);
            else
                ggMainButton.PutValue(0);
        }
        else
            ggMainButton.PutValue(0);

        if (Console::Pressed(Global::Keys[k_CurrentNext]))
        {
            if (mvControlled->TrainType != dt_EZT)
            {
                if (ShowNextCurrent == false)
                {
                    if (ggNextCurrentButton.SubModel)
                    {
                        ggNextCurrentButton.UpdateValue(1);
                        dsbSwitch->SetVolume(DSBVOLUME_MAX);
                        dsbSwitch->Play(0, 0, 0);
                        ShowNextCurrent = true;
                    }
                }
            }
            else
            {
                if (Console::Pressed(VK_SHIFT))
                {
                    //     if (Console::Pressed(k_CurrentNext))
                    { // Ra: by³o pod VK_F3
                        if ((mvOccupied->EpFuseSwitch(true)))
                        {
                            dsbPneumaticSwitch->SetVolume(-10);
                            dsbPneumaticSwitch->Play(0, 0, 0);
                        }
                    }
                }
                else
                {
                    //     if (Console::Pressed(k_CurrentNext))
                    { // Ra: by³o pod VK_F3
                        if (Console::Pressed(VK_CONTROL))
                        {
                            if ((mvOccupied->EpFuseSwitch(false)))
                            {
                                dsbPneumaticSwitch->SetVolume(-10);
                                dsbPneumaticSwitch->Play(0, 0, 0);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            if (ShowNextCurrent == true)
            {
                if (ggNextCurrentButton.SubModel)
                {
                    ggNextCurrentButton.UpdateValue(0);
                    dsbSwitch->SetVolume(DSBVOLUME_MAX);
                    dsbSwitch->Play(0, 0, 0);
                    ShowNextCurrent = false;
                }
            }
        }

        // Winger 010304  PantAllDownButton
        if (Console::Pressed(Global::Keys[k_PantFrontUp]))
        {
            if (Console::Pressed(VK_SHIFT))
                ggPantFrontButton.PutValue(1);
            else
                ggPantAllDownButton.PutValue(1);
        }
        else if (mvControlled->PantSwitchType == "impulse")
        {
            ggPantFrontButton.PutValue(0);
            ggPantAllDownButton.PutValue(0);
        }

        if (Console::Pressed(Global::Keys[k_PantRearUp]))
        {
            if (Console::Pressed(VK_SHIFT))
                ggPantRearButton.PutValue(1);
            else
                ggPantFrontButtonOff.PutValue(1);
        }
        else if (mvControlled->PantSwitchType == "impulse")
        {
            ggPantRearButton.PutValue(0);
            ggPantFrontButtonOff.PutValue(0);
        }

        /*  if ((mvControlled->Mains) && (mvControlled->EngineType==ElectricSeriesMotor))
            {
        //tu dac w przyszlosci zaleznosc od wlaczenia przetwornicy
            if (mvControlled->ConverterFlag)          //NBMX -obsluga przetwornicy
            {
            //glosnosc zalezna od nap. sieci
            //-2000 do 0
             long tmpVol;
             int trackVol;
             trackVol=3550-2000;
             if (mvControlled->RunningTraction.TractionVoltage<2000)
              {
               tmpVol=0;
              }
             else
              {
               tmpVol=mvControlled->RunningTraction.TractionVoltage-2000;
              }
             sConverter.Volume(-2000*(trackVol-tmpVol)/trackVol);

               if (!sConverter.Playing())
                 sConverter.TurnOn();
            }
            else                        //wyl przetwornicy
            sConverter.TurnOff();
           }
           else
           {
              if (sConverter.Playing())
               sConverter.TurnOff();
           }
           sConverter.Update();
        */

        //  if (fabs(DynamicObject->GetVelocity())>0.5)
        if (FreeFlyModeFlag ? false : fTachoCount > maxtacho)
        {
            dsbHasler->GetStatus(&stat);
            if (!(stat & DSBSTATUS_PLAYING))
                dsbHasler->Play(0, 0, DSBPLAY_LOOPING);
        }
        else
        {
            if (FreeFlyModeFlag ? true : fTachoCount < 1)
            {
                dsbHasler->GetStatus(&stat);
                if (stat & DSBSTATUS_PLAYING)
                    dsbHasler->Stop();
            }
        }

        // koniec mieszania z dzwiekami

        // guziki:
        ggMainOffButton.Update();
        ggMainOnButton.Update();
        ggMainButton.Update();
        ggSecurityResetButton.Update();
        ggReleaserButton.Update();
        ggAntiSlipButton.Update();
        ggFuseButton.Update();
        ggConverterFuseButton.Update();
        ggStLinOffButton.Update();
        ggRadioButton.Update();
        ggDepartureSignalButton.Update();
        ggPantFrontButton.Update();
        ggPantRearButton.Update();
        ggPantFrontButtonOff.Update();
        ggUpperLightButton.Update();
        ggLeftLightButton.Update();
        ggRightLightButton.Update();
        ggLeftEndLightButton.Update();
        ggRightEndLightButton.Update();
        // hunter-230112
        ggRearUpperLightButton.Update();
        ggRearLeftLightButton.Update();
        ggRearRightLightButton.Update();
        ggRearLeftEndLightButton.Update();
        ggRearRightEndLightButton.Update();
        //------------
        ggPantAllDownButton.Update();
        ggConverterButton.Update();
        ggConverterOffButton.Update();
        ggTrainHeatingButton.Update();
        ggSignallingButton.Update();
        ggNextCurrentButton.Update();
        ggHornButton.Update();
        ggUniversal1Button.Update();
        ggUniversal2Button.Update();
        ggUniversal3Button.Update();
        // hunter-091012
        ggCabLightButton.Update();
        ggCabLightDimButton.Update();
        //------
        if (ActiveUniversal4)
            ggUniversal4Button.PermIncValue(dt);

        ggUniversal4Button.Update();
        ggMainOffButton.UpdateValue(0);
        ggMainOnButton.UpdateValue(0);
        ggSecurityResetButton.UpdateValue(0);
        ggReleaserButton.UpdateValue(0);
        ggAntiSlipButton.UpdateValue(0);
        ggDepartureSignalButton.UpdateValue(0);
        ggFuseButton.UpdateValue(0);
        ggConverterFuseButton.UpdateValue(0);
    }
    // wyprowadzenie sygna³ów dla haslera na PoKeys (zaznaczanie na taœmie)
    btHaslerBrakes.Turn(DynamicObject->MoverParameters->BrakePress > 0.4); // ciœnienie w cylindrach
    btHaslerCurrent.Turn(DynamicObject->MoverParameters->Im != 0.0); // pr¹d na silnikach
    return true; //(DynamicObject->Update(dt));
} // koniec update

bool TTrain::CabChange(int iDirection)
{ // McZapkie-090902: zmiana kabiny 1->0->2 i z powrotem
    if (DynamicObject->Mechanik ? DynamicObject->Mechanik->AIControllFlag :
                                  true) // jeœli prowadzi AI albo jest w innym cz³onie
    { // jak AI prowadzi, to nie mo¿na mu mieszaæ
        if (abs(DynamicObject->MoverParameters->ActiveCab + iDirection) > 1)
            return false; // ewentualna zmiana pojazdu
        DynamicObject->MoverParameters->ActiveCab =
            DynamicObject->MoverParameters->ActiveCab + iDirection;
    }
    else
    { // jeœli pojazd prowadzony rêcznie albo wcale (wagon)
        DynamicObject->MoverParameters->CabDeactivisation();
        if (DynamicObject->MoverParameters->ChangeCab(iDirection))
            if (InitializeCab(DynamicObject->MoverParameters->ActiveCab,
                              DynamicObject->asBaseDir + DynamicObject->MoverParameters->TypeName +
                                  ".mmd"))
            { // zmiana kabiny w ramach tego samego pojazdu
                DynamicObject->MoverParameters
                    ->CabActivisation(); // za³¹czenie rozrz¹du (wirtualne kabiny)
                return true; // uda³o siê zmieniæ kabinê
            }
        DynamicObject->MoverParameters->CabActivisation(); // aktywizacja poprzedniej, bo jeszcze
                                                           // nie wiadomo, czy jakiœ pojazd jest
    }
    return false; // ewentualna zmiana pojazdu
}

// McZapkie-310302
// wczytywanie pliku z danymi multimedialnymi (dzwieki, kontrolki, kabiny)
bool TTrain::LoadMMediaFile(AnsiString asFileName)
{
    double dSDist;
    TFileStream *fs;
    fs = new TFileStream(asFileName, fmOpenRead | fmShareCompat);
    AnsiString str = "";
    //    DecimalSeparator='.';
    int size = fs->Size;
    str.SetLength(size);
    fs->Read(str.c_str(), size);
    str += "";
    delete fs;
    TQueryParserComp *Parser;
    Parser = new TQueryParserComp(NULL);
    Parser->TextToParse = str;
    //    Parser->LoadStringToParse(asFile);
    Parser->First();
    str = "";
    dsbPneumaticSwitch = TSoundsManager::GetFromName("silence1.wav", true);
    while ((!Parser->EndOfFile) && (str != AnsiString("internaldata:")))
    {
        str = Parser->GetNextSymbol().LowerCase();
    }
    if (str == AnsiString("internaldata:"))
    {
        while (!Parser->EndOfFile)
        {
            str = Parser->GetNextSymbol().LowerCase();
            // SEKCJA DZWIEKOW
            if (str == AnsiString("ctrl:")) // nastawnik:
            {
                str = Parser->GetNextSymbol().LowerCase();
                dsbNastawnikJazdy = TSoundsManager::GetFromName(str.c_str(), true);
            }
            else
                //---------------
                // hunter-081211: nastawnik bocznikowania
                if (str == AnsiString("ctrlscnd:")) // nastawnik bocznikowania:
            {
                str = Parser->GetNextSymbol().LowerCase();
                dsbNastawnikBocz = TSoundsManager::GetFromName(str.c_str(), true);
            }
            else
                //---------------
                // hunter-131211: dzwiek kierunkowego
                if (str == AnsiString("reverserkey:")) // nastawnik kierunkowy
            {
                str = Parser->GetNextSymbol().LowerCase();
                dsbReverserKey = TSoundsManager::GetFromName(str.c_str(), true);
            }
            else
                //---------------
                if (str == AnsiString("buzzer:")) // bzyczek shp:
            {
                str = Parser->GetNextSymbol().LowerCase();
                dsbBuzzer = TSoundsManager::GetFromName(str.c_str(), true);
            }
            else if (str == AnsiString("slipalarm:")) // Bombardier 011010: alarm przy poslizgu:
            {
                str = Parser->GetNextSymbol().LowerCase();
                dsbSlipAlarm = TSoundsManager::GetFromName(str.c_str(), true);
            }
            else if (str == AnsiString("tachoclock:")) // cykanie rejestratora:
            {
                str = Parser->GetNextSymbol().LowerCase();
                dsbHasler = TSoundsManager::GetFromName(str.c_str(), true);
            }
            else if (str == AnsiString("switch:")) // przelaczniki:
            {
                str = Parser->GetNextSymbol().LowerCase();
                dsbSwitch = TSoundsManager::GetFromName(str.c_str(), true);
            }
            else if (str == AnsiString("pneumaticswitch:")) // stycznik EP:
            {
                str = Parser->GetNextSymbol().LowerCase();
                dsbPneumaticSwitch = TSoundsManager::GetFromName(str.c_str(), true);
            }
            else
                //---------------
                // hunter-111211: wydzielenie wejscia na bezoporowa i na drugi uklad do pliku
                if (str == AnsiString("wejscie_na_bezoporow:"))
            {
                str = Parser->GetNextSymbol().LowerCase();
                dsbWejscie_na_bezoporow = TSoundsManager::GetFromName(str.c_str(), true);
            }
            else if (str == AnsiString("wejscie_na_drugi_uklad:"))
            {
                str = Parser->GetNextSymbol().LowerCase();
                dsbWejscie_na_drugi_uklad = TSoundsManager::GetFromName(str.c_str(), true);
            }
            else
                //---------------
                if (str == AnsiString("relay:")) // styczniki itp:
            {
                str = Parser->GetNextSymbol().LowerCase();
                dsbRelay = TSoundsManager::GetFromName(str.c_str(), true);
                if (!dsbWejscie_na_bezoporow) // hunter-111211: domyslne, gdy brak
                    dsbWejscie_na_bezoporow =
                        TSoundsManager::GetFromName("wejscie_na_bezoporow.wav", true);
                if (!dsbWejscie_na_drugi_uklad)
                    dsbWejscie_na_drugi_uklad =
                        TSoundsManager::GetFromName("wescie_na_drugi_uklad.wav", true);
            }
            else if (str == AnsiString("pneumaticrelay:")) // wylaczniki pneumatyczne:
            {
                str = Parser->GetNextSymbol().LowerCase();
                dsbPneumaticRelay = TSoundsManager::GetFromName(str.c_str(), true);
            }
            else if (str == AnsiString("couplerattach:")) // laczenie:
            {
                str = Parser->GetNextSymbol().LowerCase();
                dsbCouplerAttach = TSoundsManager::GetFromName(str.c_str(), true);
                dsbCouplerStretch = TSoundsManager::GetFromName(
                    "en57_couplerstretch.wav", true); // McZapkie-090503: PROWIZORKA!!!
            }
            else if (str == AnsiString("couplerdetach:")) // rozlaczanie:
            {
                str = Parser->GetNextSymbol().LowerCase();
                dsbCouplerDetach = TSoundsManager::GetFromName(str.c_str(), true);
                dsbBufferClamp = TSoundsManager::GetFromName(
                    "en57_bufferclamp.wav", true); // McZapkie-090503: PROWIZORKA!!!
            }
            else if (str == AnsiString("ignition:"))
            { // odpalanie silnika
                str = Parser->GetNextSymbol().LowerCase();
                dsbDieselIgnition = TSoundsManager::GetFromName(str.c_str(), true);
            }
            else if (str == AnsiString("brakesound:")) // hamowanie zwykle:
            {
                str = Parser->GetNextSymbol();
                rsBrake.Init(str.c_str(), -1, 0, 0, 0, true, true);
                rsBrake.AM =
                    Parser->GetNextSymbol().ToDouble() / (1 + mvOccupied->MaxBrakeForce * 1000);
                rsBrake.AA = Parser->GetNextSymbol().ToDouble();
                rsBrake.FM = Parser->GetNextSymbol().ToDouble() / (1 + mvOccupied->Vmax);
                rsBrake.FA = Parser->GetNextSymbol().ToDouble();
            }
            else if (str == AnsiString("slipperysound:")) // sanie:
            {
                str = Parser->GetNextSymbol();
                rsSlippery.Init(str.c_str(), -1, 0, 0, 0, true);
                rsSlippery.AM = Parser->GetNextSymbol().ToDouble() / (1 + mvOccupied->Vmax);
                rsSlippery.AA = Parser->GetNextSymbol().ToDouble();
                rsSlippery.FM = 0.0;
                rsSlippery.FA = 1.0;
            }
            else if (str == AnsiString("airsound:")) // syk:
            {
                str = Parser->GetNextSymbol();
                rsHiss.Init(str.c_str(), -1, 0, 0, 0, true);
                rsHiss.AM = Parser->GetNextSymbol().ToDouble();
                rsHiss.AA = Parser->GetNextSymbol().ToDouble();
                rsHiss.FM = 0.0;
                rsHiss.FA = 1.0;
            }
            else if (str == AnsiString("airsound2:")) // syk:
            {
                str = Parser->GetNextSymbol();
                rsHissU.Init(str.c_str(), -1, 0, 0, 0, true);
                rsHissU.AM = Parser->GetNextSymbol().ToDouble();
                rsHissU.AA = Parser->GetNextSymbol().ToDouble();
                rsHissU.FM = 0.0;
                rsHissU.FA = 1.0;
            }
            else if (str == AnsiString("airsound3:")) // syk:
            {
                str = Parser->GetNextSymbol();
                rsHissE.Init(str.c_str(), -1, 0, 0, 0, true);
                rsHissE.AM = Parser->GetNextSymbol().ToDouble();
                rsHissE.AA = Parser->GetNextSymbol().ToDouble();
                rsHissE.FM = 0.0;
                rsHissE.FA = 1.0;
            }
            else if (str == AnsiString("airsound4:")) // syk:
            {
                str = Parser->GetNextSymbol();
                rsHissX.Init(str.c_str(), -1, 0, 0, 0, true);
                rsHissX.AM = Parser->GetNextSymbol().ToDouble();
                rsHissX.AA = Parser->GetNextSymbol().ToDouble();
                rsHissX.FM = 0.0;
                rsHissX.FA = 1.0;
            }
            else if (str == AnsiString("airsound5:")) // syk:
            {
                str = Parser->GetNextSymbol();
                rsHissT.Init(str.c_str(), -1, 0, 0, 0, true);
                rsHissT.AM = Parser->GetNextSymbol().ToDouble();
                rsHissT.AA = Parser->GetNextSymbol().ToDouble();
                rsHissT.FM = 0.0;
                rsHissT.FA = 1.0;
            }
            else if (str == AnsiString("fadesound:")) // syk:
            {
                str = Parser->GetNextSymbol();
                rsFadeSound.Init(str.c_str(), -1, 0, 0, 0, true);
                rsFadeSound.AM = 1.0;
                rsFadeSound.AA = 1.0;
                rsFadeSound.FM = 1.0;
                rsFadeSound.FA = 1.0;
            }
            else if (str == AnsiString("localbrakesound:")) // syk:
            {
                str = Parser->GetNextSymbol();
                rsSBHiss.Init(str.c_str(), -1, 0, 0, 0, true);
                rsSBHiss.AM = Parser->GetNextSymbol().ToDouble();
                rsSBHiss.AA = Parser->GetNextSymbol().ToDouble();
                rsSBHiss.FM = 0.0;
                rsSBHiss.FA = 1.0;
            }
            else if (str == AnsiString("runningnoise:")) // szum podczas jazdy:
            {
                str = Parser->GetNextSymbol();
                rsRunningNoise.Init(str.c_str(), -1, 0, 0, 0, true, true);
                rsRunningNoise.AM = Parser->GetNextSymbol().ToDouble() / (1 + mvOccupied->Vmax);
                rsRunningNoise.AA = Parser->GetNextSymbol().ToDouble();
                rsRunningNoise.FM = Parser->GetNextSymbol().ToDouble() / (1 + mvOccupied->Vmax);
                rsRunningNoise.FA = Parser->GetNextSymbol().ToDouble();
            }
            else if (str == AnsiString("engageslippery:")) // tarcie tarcz sprzegla:
            {
                str = Parser->GetNextSymbol();
                rsEngageSlippery.Init(str.c_str(), -1, 0, 0, 0, true, true);
                rsEngageSlippery.AM = Parser->GetNextSymbol().ToDouble();
                rsEngageSlippery.AA = Parser->GetNextSymbol().ToDouble();
                rsEngageSlippery.FM = Parser->GetNextSymbol().ToDouble() / (1 + mvOccupied->nmax);
                rsEngageSlippery.FA = Parser->GetNextSymbol().ToDouble();
            }
            else if (str == AnsiString("mechspring:")) // parametry bujania kamery:
            {
                str = Parser->GetNextSymbol();
                MechSpring.Init(0, str.ToDouble(), Parser->GetNextSymbol().ToDouble());
                fMechSpringX = Parser->GetNextSymbol().ToDouble();
                fMechSpringY = Parser->GetNextSymbol().ToDouble();
                fMechSpringZ = Parser->GetNextSymbol().ToDouble();
                fMechMaxSpring = Parser->GetNextSymbol().ToDouble();
                fMechRoll = Parser->GetNextSymbol().ToDouble();
                fMechPitch = Parser->GetNextSymbol().ToDouble();
            }
            else if (str == AnsiString("pantographup:")) // podniesienie patyka:
            {
                str = Parser->GetNextSymbol().LowerCase();
                dsbPantUp = TSoundsManager::GetFromName(str.c_str(), true);
            }
            else if (str == AnsiString("pantographdown:")) // podniesienie patyka:
            {
                str = Parser->GetNextSymbol().LowerCase();
                dsbPantDown = TSoundsManager::GetFromName(str.c_str(), true);
            }
            else if (str == AnsiString("doorclose:")) // zamkniecie drzwi:
            {
                str = Parser->GetNextSymbol().LowerCase();
                dsbDoorClose = TSoundsManager::GetFromName(str.c_str(), true);
            }
            else if (str == AnsiString("dooropen:")) // wotwarcie drzwi:
            {
                str = Parser->GetNextSymbol().LowerCase();
                dsbDoorOpen = TSoundsManager::GetFromName(str.c_str(), true);
            }
        }
    }
    else
        return false; // nie znalazl sekcji internal
    delete Parser; // Ra: jak siê coœ newuje, to trzeba zdeletowaæ, inaczej syf siê robi
    if (!InitializeCab(mvOccupied->ActiveCab, asFileName)) // zle zainicjowana kabina
        return false;
    else
    {
        if (DynamicObject->Controller == Humandriver)
            DynamicObject->bDisplayCab = true; // McZapkie-030303: mozliwosc wyswietlania kabiny, w
                                               // przyszlosci dac opcje w mmd
        return true;
    }
}

bool TTrain::InitializeCab(int NewCabNo, AnsiString asFileName)
{
    bool parse = false;
    double dSDist;
    TFileStream *fs;
    fs = new TFileStream(asFileName, fmOpenRead | fmShareCompat);
    AnsiString str = "";
    // DecimalSeparator='.';
    int size = fs->Size;
    str.SetLength(size);
    fs->Read(str.c_str(), size);
    str += "";
    delete fs;
    TQueryParserComp *Parser;
    Parser = new TQueryParserComp(NULL);
    Parser->TextToParse = str;
    // Parser->LoadStringToParse(asFile);
    Parser->First();
    str = "";
    int cabindex = 0;
    TGauge *gg; // roboczy wsaŸnik na obiekt animuj¹cy ga³kê
    DynamicObject->mdKabina = NULL; // likwidacja wskaŸnika na dotychczasow¹ kabinê
    switch (NewCabNo)
    { // ustalenie numeru kabiny do wczytania
    case -1:
        cabindex = 2;
        break;
    case 1:
        cabindex = 1;
        break;
    case 0:
        cabindex = 0;
    }
    AnsiString cabstr = AnsiString("cab") + cabindex + AnsiString("definition:");
    while ((!Parser->EndOfFile) && (AnsiCompareStr(str, cabstr) != 0))
        str = Parser->GetNextSymbol().LowerCase(); // szukanie kabiny
    if (cabindex != 1)
        if (AnsiCompareStr(str, cabstr) != 0) // jeœli nie znaleziony wpis kabiny
        { // próba szukania kabiny 1
            cabstr = AnsiString("cab1definition:");
            Parser->First();
            while ((!Parser->EndOfFile) && (AnsiCompareStr(str, cabstr) != 0))
                str = Parser->GetNextSymbol().LowerCase(); // szukanie kabiny
        }
    if (AnsiCompareStr(str, cabstr) == 0) // jeœli znaleziony wpis kabiny
    {
        Cabine[cabindex].Load(Parser);
        str = Parser->GetNextSymbol().LowerCase();
        if (AnsiCompareStr(AnsiString("driver") + cabindex + AnsiString("pos:"), str) ==
            0) // pozycja poczatkowa maszynisty
        {
            pMechOffset.x = Parser->GetNextSymbol().ToDouble();
            pMechOffset.y = Parser->GetNextSymbol().ToDouble();
            pMechOffset.z = Parser->GetNextSymbol().ToDouble();
            pMechSittingPosition.x = pMechOffset.x;
            pMechSittingPosition.y = pMechOffset.y;
            pMechSittingPosition.z = pMechOffset.z;
        }
        // ABu: pozycja siedzaca mechanika
        str = Parser->GetNextSymbol().LowerCase();
        if (AnsiCompareStr(AnsiString("driver") + cabindex + AnsiString("sitpos:"), str) ==
            0) // ABu 180404 pozycja siedzaca maszynisty
        {
            pMechSittingPosition.x = Parser->GetNextSymbol().ToDouble();
            pMechSittingPosition.y = Parser->GetNextSymbol().ToDouble();
            pMechSittingPosition.z = Parser->GetNextSymbol().ToDouble();
            parse = true;
        }
        // else parse=false;
        while (!Parser->EndOfFile)
        { // ABu: wstawione warunki, wczesniej tylko to:
            //   str=Parser->GetNextSymbol().LowerCase();
            if (parse)
                str = Parser->GetNextSymbol().LowerCase();
            else
                parse = true;
            // inicjacja kabiny
            // Ra 2014-08: zmieniamy zasady - zamiast przypisywaæ submodel do istniej¹cych obiektów
            // animuj¹cych
            // bêdziemy teraz uaktywniaæ obiekty animuj¹ce z tablicy i podawaæ im submodel oraz
            // wskaŸnik na parametr
            if (AnsiCompareStr(AnsiString("cab") + cabindex + AnsiString("model:"), str) ==
                0) // model kabiny
            {
                str = Parser->GetNextSymbol().LowerCase();
                if (str != AnsiString("none"))
                {
                    str = DynamicObject->asBaseDir + str;
                    Global::asCurrentTexturePath =
                        DynamicObject->asBaseDir; // bie¿¹ca sciezka do tekstur to dynamic/...
                    TModel3d *k = TModelsManager::GetModel(
                        str.c_str(), true); // szukaj kabinê jako oddzielny model
                    Global::asCurrentTexturePath =
                        AnsiString(szTexturePath); // z powrotem defaultowa sciezka do tekstur
                    // if (DynamicObject->mdKabina!=k)
                    if (k)
                        DynamicObject->mdKabina = k; // nowa kabina
                    //(mdKabina) mo¿e zostaæ to samo po przejœciu do innego cz³onu bez zmiany
                    //kabiny, przy powrocie musi byæ wi¹zanie ponowne
                    // else
                    // break; //wyjœcie z pêtli, bo model zostaje bez zmian
                }
                else if (cabindex == 1) // model tylko, gdy nie ma kabiny 1
                    DynamicObject->mdKabina =
                        DynamicObject
                            ->mdModel; // McZapkie-170103: szukaj elementy kabiny w glownym modelu
                ActiveUniversal4 = false;
                ggMainCtrl.Clear();
                ggMainCtrlAct.Clear();
                ggScndCtrl.Clear();
                ggDirKey.Clear();
                ggBrakeCtrl.Clear();
                ggLocalBrake.Clear();
                ggManualBrake.Clear();
                ggBrakeProfileCtrl.Clear();
                ggBrakeProfileG.Clear();
                ggBrakeProfileR.Clear();
                ggMaxCurrentCtrl.Clear();
                ggMainOffButton.Clear();
                ggMainOnButton.Clear();
                ggSecurityResetButton.Clear();
                ggReleaserButton.Clear();
                ggAntiSlipButton.Clear();
                ggHornButton.Clear();
                ggNextCurrentButton.Clear();
                ggUniversal1Button.Clear();
                ggUniversal2Button.Clear();
                ggUniversal3Button.Clear();
                // hunter-091012
                ggCabLightButton.Clear();
                ggCabLightDimButton.Clear();
                //-------
                ggUniversal4Button.Clear();
                ggFuseButton.Clear();
                ggConverterFuseButton.Clear();
                ggStLinOffButton.Clear();
                ggDoorLeftButton.Clear();
                ggDoorRightButton.Clear();
                ggDepartureSignalButton.Clear();
                ggCompressorButton.Clear();
                ggConverterButton.Clear();
                ggPantFrontButton.Clear();
                ggPantRearButton.Clear();
                ggPantFrontButtonOff.Clear();
                ggPantAllDownButton.Clear();
                ggZbS.Clear();
                ggI1B.Clear();
                ggI2B.Clear();
                ggI3B.Clear();
                ggItotalB.Clear();

                ggClockSInd.Clear();
                ggClockMInd.Clear();
                ggClockHInd.Clear();
                ggEngineVoltage.Clear();
                ggLVoltage.Clear();
                // ggLVoltage.Output(0); //Ra: sterowanie miernikiem: niskie napiêcie
                // ggEnrot1m.Clear();
                // ggEnrot2m.Clear();
                // ggEnrot3m.Clear();
                // ggEngageRatio.Clear();
                ggMainGearStatus.Clear();
                ggIgnitionKey.Clear();
                btLampkaPoslizg.Clear(6);
                btLampkaStyczn.Clear(5);
                btLampkaNadmPrzetw.Clear(
                    (mvControlled->TrainType & (dt_EZT)) ? -1 : 7); // EN57 nie ma tej lampki
                btLampkaPrzetw.Clear((mvControlled->TrainType & (dt_EZT)) ? 7 : -1); // za to ma tê
                btLampkaPrzekRozn.Clear();
                btLampkaPrzekRoznPom.Clear();
                btLampkaNadmSil.Clear(4);
                btLampkaUkrotnienie.Clear();
                btLampkaHamPosp.Clear();
                btLampkaWylSzybki.Clear(3);
                btLampkaNadmWent.Clear(9);
                btLampkaNadmSpr.Clear(8);
                btLampkaOpory.Clear(2);
                btLampkaWysRozr.Clear(
                    (mvControlled->TrainType & (dt_ET22)) ? -1 : 10); // ET22 nie ma tej lampki
                btLampkaBezoporowa.Clear();
                btLampkaBezoporowaB.Clear();
                btLampkaMaxSila.Clear();
                btLampkaPrzekrMaxSila.Clear();
                btLampkaRadio.Clear();
                btLampkaHamulecReczny.Clear();
                btLampkaBlokadaDrzwi.Clear();
                btLampkaUniversal3.Clear();
                btLampkaWentZaluzje.Clear();
                btLampkaOgrzewanieSkladu.Clear(11);
                btLampkaSHP.Clear(0);
                btLampkaCzuwaka.Clear(1);
                btLampkaDoorLeft.Clear();
                btLampkaDoorRight.Clear();
                btLampkaDepartureSignal.Clear();
                btLampkaRezerwa.Clear();
                btLampkaBoczniki.Clear();
                btLampkaBocznik1.Clear();
                btLampkaBocznik2.Clear();
                btLampkaBocznik3.Clear();
                btLampkaBocznik4.Clear();
                btLampkaRadiotelefon.Clear();
                btLampkaHamienie.Clear();
                btLampkaSprezarka.Clear();
                btLampkaSprezarkaB.Clear();
                btLampkaNapNastHam.Clear();
                btLampkaJazda.Clear();
                btLampkaStycznB.Clear();
                btLampkaHamowanie1zes.Clear();
                btLampkaHamowanie2zes.Clear();
                btLampkaNadmPrzetwB.Clear();
                btLampkaPrzetwB.Clear();
                btLampkaWylSzybkiB.Clear();
                btLampkaForward.Clear();
                btLampkaBackward.Clear();
                btCabLight.Clear(); // hunter-171012
                ggLeftLightButton.Clear();
                ggRightLightButton.Clear();
                ggUpperLightButton.Clear();
                ggLeftEndLightButton.Clear();
                ggRightEndLightButton.Clear();
                // hunter-230112
                ggRearLeftLightButton.Clear();
                ggRearRightLightButton.Clear();
                ggRearUpperLightButton.Clear();
                ggRearLeftEndLightButton.Clear();
                ggRearRightEndLightButton.Clear();
                btHaslerBrakes.Clear(12); // ciœnienie w cylindrach do odbijania na haslerze
                btHaslerCurrent.Clear(13); // pr¹d na silnikach do odbijania na haslerze
            }
            // SEKCJA REGULATOROW
            else if (str == AnsiString("mainctrl:")) // nastawnik
            {
                if (!DynamicObject->mdKabina)
                {
                    delete Parser;
                    WriteLog("Cab not initialised!");
                    return false;
                }
                else
                    ggMainCtrl.Load(Parser, DynamicObject->mdKabina);
            }
            else if (str == AnsiString("mainctrlact:")) // zabek pozycji aktualnej
                ggMainCtrlAct.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("scndctrl:")) // bocznik
                ggScndCtrl.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("dirkey:")) // klucz kierunku
                ggDirKey.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("brakectrl:")) // hamulec zasadniczy
                ggBrakeCtrl.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("localbrake:")) // hamulec pomocniczy
                ggLocalBrake.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("manualbrake:")) // hamulec reczny
                ggManualBrake.Load(Parser, DynamicObject->mdKabina);
            // sekcja przelacznikow obrotowych
            else if (str == AnsiString("brakeprofile_sw:")) // przelacznik tow/osob/posp
                ggBrakeProfileCtrl.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("brakeprofileg_sw:")) // przelacznik tow/osob
                ggBrakeProfileG.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("brakeprofiler_sw:")) // przelacznik osob/posp
                ggBrakeProfileR.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("maxcurrent_sw:")) // przelacznik rozruchu
                ggMaxCurrentCtrl.Load(Parser, DynamicObject->mdKabina);
            // SEKCJA przyciskow sprezynujacych
            else if (str ==
                     AnsiString(
                         "main_off_bt:")) // przycisk wylaczajacy (w EU07 wyl szybki czerwony)
                ggMainOffButton.Load(Parser, DynamicObject->mdKabina);
            else if (str ==
                     AnsiString("main_on_bt:")) // przycisk wlaczajacy (w EU07 wyl szybki zielony)
                ggMainOnButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("security_reset_bt:")) // przycisk zbijajacy SHP/czuwak
                ggSecurityResetButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("releaser_bt:")) // przycisk odluzniacza
                ggReleaserButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("releaser_bt:")) // przycisk odluzniacza
                ggReleaserButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("antislip_bt:")) // przycisk antyposlizgowy
                ggAntiSlipButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("horn_bt:")) // dzwignia syreny
                ggHornButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("fuse_bt:")) // bezp. nadmiarowy
                ggFuseButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("converterfuse_bt:")) // hunter-261211: odblokowanie
                                                             // przekaznika nadm. przetw. i ogrz.
                ggConverterFuseButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("stlinoff_bt:")) // st. liniowe
                ggStLinOffButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("door_left_sw:")) // drzwi lewe
                ggDoorLeftButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("door_right_sw:")) // drzwi prawe
                ggDoorRightButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("departure_signal_bt:")) // sygnal odjazdu
                ggDepartureSignalButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("upperlight_sw:")) // swiatlo
                ggUpperLightButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("leftlight_sw:")) // swiatlo
                ggLeftLightButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("rightlight_sw:")) // swiatlo
                ggRightLightButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("leftend_sw:")) // swiatlo
                ggLeftEndLightButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("rightend_sw:")) // swiatlo
                ggRightEndLightButton.Load(Parser, DynamicObject->mdKabina);
            //---------------------
            // hunter-230112: przelaczniki swiatel tylnich
            else if (str == AnsiString("rearupperlight_sw:")) // swiatlo
                ggRearUpperLightButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("rearleftlight_sw:")) // swiatlo
                ggRearLeftLightButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("rearrightlight_sw:")) // swiatlo
                ggRearRightLightButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("rearleftend_sw:")) // swiatlo
                ggRearLeftEndLightButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("rearrightend_sw:")) // swiatlo
                ggRearRightEndLightButton.Load(Parser, DynamicObject->mdKabina);
            //------------------
            else if (str == AnsiString("compressor_sw:")) // sprezarka
                ggCompressorButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("converter_sw:")) // przetwornica
                ggConverterButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("converteroff_sw:")) // przetwornica wyl
                ggConverterOffButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("main_sw:")) // wyl szybki (ezt)
                ggMainButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("radio_sw:")) // radio
                ggRadioButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("pantfront_sw:")) // patyk przedni
                ggPantFrontButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("pantrear_sw:")) // patyk tylny
                ggPantRearButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("pantfrontoff_sw:")) // patyk przedni w dol
                ggPantFrontButtonOff.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("pantalloff_sw:")) // patyk przedni w dol
                ggPantAllDownButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("trainheating_sw:")) // grzanie skladu
                ggTrainHeatingButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("signalling_sw:")) // Sygnalizacja hamowania
                ggSignallingButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("door_signalling_sw:")) // Sygnalizacja blokady drzwi
                ggDoorSignallingButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("nextcurrent_sw:")) // grzanie skladu
                ggNextCurrentButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("cablight_sw:")) // hunter-091012: swiatlo w kabinie
                ggCabLightButton.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("cablightdim_sw:"))
                ggCabLightDimButton.Load(
                    Parser,
                    DynamicObject->mdKabina); // hunter-091012: przyciemnienie swiatla w kabinie
            // ABu 090305: uniwersalne przyciski lub inne rzeczy
            else if (str == AnsiString("universal1:"))
                ggUniversal1Button.Load(Parser, DynamicObject->mdKabina, DynamicObject->mdModel);
            else if (str == AnsiString("universal2:"))
                ggUniversal2Button.Load(Parser, DynamicObject->mdKabina, DynamicObject->mdModel);
            else if (str == AnsiString("universal3:"))
                ggUniversal3Button.Load(Parser, DynamicObject->mdKabina, DynamicObject->mdModel);
            else if (str == AnsiString("universal4:"))
                ggUniversal4Button.Load(Parser, DynamicObject->mdKabina, DynamicObject->mdModel);
            // SEKCJA WSKAZNIKOW
            else if ((str == AnsiString("tachometer:")) || (str == AnsiString("tachometerb:")))
            { // predkosciomierz wskazówkowy z szarpaniem
                gg = Cabine[cabindex].Gauge(-1); // pierwsza wolna ga³ka
                gg->Load(Parser, DynamicObject->mdKabina);
                gg->AssignFloat(&fTachoVelocityJump);
            }
            else if (str == AnsiString("tachometern:"))
            { // predkosciomierz wskazówkowy bez szarpania
                gg = Cabine[cabindex].Gauge(-1); // pierwsza wolna ga³ka
                gg->Load(Parser, DynamicObject->mdKabina);
                gg->AssignFloat(&fTachoVelocity);
            }
            else if (str == AnsiString("tachometerd:"))
            { // predkosciomierz cyfrowy
                gg = Cabine[cabindex].Gauge(-1); // pierwsza wolna ga³ka
                gg->Load(Parser, DynamicObject->mdKabina);
                gg->AssignFloat(&fTachoVelocity);
            }
            else if ((str == AnsiString("hvcurrent1:")) || (str == AnsiString("hvcurrent1b:")))
            { // 1szy amperomierz
                gg = Cabine[cabindex].Gauge(-1); // pierwsza wolna ga³ka
                gg->Load(Parser, DynamicObject->mdKabina);
                gg->AssignFloat(fHCurrent + 1);
            }
            else if ((str == AnsiString("hvcurrent2:")) || (str == AnsiString("hvcurrent2b:")))
            { // 2gi amperomierz
                gg = Cabine[cabindex].Gauge(-1); // pierwsza wolna ga³ka
                gg->Load(Parser, DynamicObject->mdKabina);
                gg->AssignFloat(fHCurrent + 2);
            }
            else if ((str == AnsiString("hvcurrent3:")) || (str == AnsiString("hvcurrent3b:")))
            { // 3ci amperomierz
                gg = Cabine[cabindex].Gauge(-1); // pierwsza wolna ga³ka
                gg->Load(Parser, DynamicObject->mdKabina);
                gg->AssignFloat(fHCurrent + 3);
            }
            else if ((str == AnsiString("hvcurrent:")) || (str == AnsiString("hvcurrentb:")))
            { // amperomierz calkowitego pradu
                gg = Cabine[cabindex].Gauge(-1); // pierwsza wolna ga³ka
                gg->Load(Parser, DynamicObject->mdKabina);
                gg->AssignFloat(fHCurrent);
            }
            else if ((str == AnsiString("brakepress:")) || (str == AnsiString("brakepressb:")))
            { // manometr cylindrow hamulcowych //Ra 2014-08: przeniesione do TCab
                gg = Cabine[cabindex].Gauge(-1); // pierwsza wolna ga³ka
                gg->Load(Parser, DynamicObject->mdKabina, NULL, 0.1);
                gg->AssignDouble(&mvOccupied->BrakePress);
            }
            else if ((str == AnsiString("pipepress:")) || (str == AnsiString("pipepressb:")))
            { // manometr przewodu hamulcowego
                TGauge *gg = Cabine[cabindex].Gauge(-1); // pierwsza wolna ga³ka
                gg->Load(Parser, DynamicObject->mdKabina, NULL, 0.1);
                gg->AssignDouble(&mvOccupied->PipePress);
            }
            else if (str ==
                     AnsiString(
                         "limpipepress:")) // manometr zbiornika sterujacego zaworu maszynisty
                ggZbS.Load(Parser, DynamicObject->mdKabina, NULL, 0.1);
            else if (str == AnsiString("cntrlpress:"))
            { // manometr zbiornika kontrolnego/rorz¹du
                gg = Cabine[cabindex].Gauge(-1); // pierwsza wolna ga³ka
                gg->Load(Parser, DynamicObject->mdKabina, NULL, 0.1);
                gg->AssignDouble(&mvControlled->PantPress);
            }
            else if ((str == AnsiString("compressor:")) || (str == AnsiString("compressorb:")))
            { // manometr sprezarki/zbiornika glownego
                gg = Cabine[cabindex].Gauge(-1); // pierwsza wolna ga³ka
                gg->Load(Parser, DynamicObject->mdKabina, NULL, 0.1);
                gg->AssignDouble(&mvOccupied->Compressor);
            }
            // yB - dla drugiej sekcji
            else if (str == AnsiString("hvbcurrent1:")) // 1szy amperomierz
                ggI1B.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("hvbcurrent2:")) // 2gi amperomierz
                ggI2B.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("hvbcurrent3:")) // 3ci amperomierz
                ggI3B.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("hvbcurrent:")) // amperomierz calkowitego pradu
                ggItotalB.Load(Parser, DynamicObject->mdKabina);
            //*************************************************************
            else if (str == AnsiString("clock:")) // zegar analogowy
            {
                if (Parser->GetNextSymbol() == AnsiString("analog"))
                {
                    // McZapkie-300302: zegarek
                    ggClockSInd.Init(DynamicObject->mdKabina->GetFromName("ClockShand"), gt_Rotate,
                                     0.016666667, 0, 0);
                    ggClockMInd.Init(DynamicObject->mdKabina->GetFromName("ClockMhand"), gt_Rotate,
                                     0.016666667, 0, 0);
                    ggClockHInd.Init(DynamicObject->mdKabina->GetFromName("ClockHhand"), gt_Rotate,
                                     0.083333333, 0, 0);
                }
            }
            else if (str == AnsiString("evoltage:")) // woltomierz napiecia silnikow
                ggEngineVoltage.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("hvoltage:"))
            { // woltomierz wysokiego napiecia
                gg = Cabine[cabindex].Gauge(-1); // pierwsza wolna ga³ka
                gg->Load(Parser, DynamicObject->mdKabina);
                gg->AssignFloat(&fHVoltage);
            }
            else if (str == AnsiString("lvoltage:")) // woltomierz niskiego napiecia
                ggLVoltage.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("enrot1m:"))
            { // obrotomierz
                gg = Cabine[cabindex].Gauge(-1); // pierwsza wolna ga³ka
                gg->Load(Parser, DynamicObject->mdKabina);
                gg->AssignFloat(fEngine + 1);
            } // ggEnrot1m.Load(Parser,DynamicObject->mdKabina);
            else if (str == AnsiString("enrot2m:"))
            { // obrotomierz
                gg = Cabine[cabindex].Gauge(-1); // pierwsza wolna ga³ka
                gg->Load(Parser, DynamicObject->mdKabina);
                gg->AssignFloat(fEngine + 2);
            } // ggEnrot2m.Load(Parser,DynamicObject->mdKabina);
            else if (str == AnsiString("enrot3m:"))
            { // obrotomierz
                gg = Cabine[cabindex].Gauge(-1); // pierwsza wolna ga³ka
                gg->Load(Parser, DynamicObject->mdKabina);
                gg->AssignFloat(fEngine + 3);
            } // ggEnrot3m.Load(Parser,DynamicObject->mdKabina);
            else if (str == AnsiString("engageratio:"))
            { // np. ciœnienie sterownika sprzêg³a
                gg = Cabine[cabindex].Gauge(-1); // pierwsza wolna ga³ka
                gg->Load(Parser, DynamicObject->mdKabina);
                gg->AssignDouble(&mvControlled->dizel_engage);
            } // ggEngageRatio.Load(Parser,DynamicObject->mdKabina);
            else if (str == AnsiString("maingearstatus:")) // np. ciœnienie sterownika skrzyni
                                                           // biegów
                ggMainGearStatus.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("ignitionkey:")) //
                ggIgnitionKey.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("distcounter:"))
            { // Ra 2014-07: licznik kilometrów
                gg = Cabine[cabindex].Gauge(-1); // pierwsza wolna ga³ka
                gg->Load(Parser, DynamicObject->mdKabina);
                gg->AssignDouble(&mvControlled->DistCounter);
            }
            // SEKCJA LAMPEK
            else if (str == AnsiString("i-maxft:"))
                btLampkaMaxSila.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-maxftt:"))
                btLampkaPrzekrMaxSila.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-radio:"))
                btLampkaRadio.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-manual_brake:"))
                btLampkaHamulecReczny.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-door_blocked:"))
                btLampkaBlokadaDrzwi.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-slippery:"))
                btLampkaPoslizg.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-contactors:"))
                btLampkaStyczn.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-conv_ovld:"))
                btLampkaNadmPrzetw.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-converter:"))
                btLampkaPrzetw.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-diff_relay:"))
                btLampkaPrzekRozn.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-diff_relay2:"))
                btLampkaPrzekRoznPom.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-motor_ovld:"))
                btLampkaNadmSil.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-train_controll:"))
                btLampkaUkrotnienie.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-brake_delay_r:"))
                btLampkaHamPosp.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-mainbreaker:"))
                btLampkaWylSzybki.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-vent_ovld:"))
                btLampkaNadmWent.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-comp_ovld:"))
                btLampkaNadmSpr.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-resistors:"))
                btLampkaOpory.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-no_resistors:"))
                btLampkaBezoporowa.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-no_resistors_b:"))
                btLampkaBezoporowaB.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-highcurrent:"))
                btLampkaWysRozr.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-universal3:"))
            {
                btLampkaUniversal3.Load(Parser, DynamicObject->mdKabina, DynamicObject->mdModel);
                LampkaUniversal3_typ = 0;
            }
            else if (str == AnsiString("i-universal3_M:"))
            {
                btLampkaUniversal3.Load(Parser, DynamicObject->mdKabina, DynamicObject->mdModel);
                LampkaUniversal3_typ = 1;
            }
            else if (str == AnsiString("i-universal3_C:"))
            {
                btLampkaUniversal3.Load(Parser, DynamicObject->mdKabina, DynamicObject->mdModel);
                LampkaUniversal3_typ = 2;
            }
            else if (str == AnsiString("i-vent_trim:"))
                btLampkaWentZaluzje.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-trainheating:"))
                btLampkaOgrzewanieSkladu.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-security_aware:"))
                btLampkaCzuwaka.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-security_cabsignal:"))
                btLampkaSHP.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-door_left:"))
                btLampkaDoorLeft.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-door_right:"))
                btLampkaDoorRight.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-departure_signal:"))
                btLampkaDepartureSignal.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-reserve:"))
                btLampkaRezerwa.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-scnd:"))
                btLampkaBoczniki.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-scnd1:"))
                btLampkaBocznik1.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-scnd2:"))
                btLampkaBocznik2.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-scnd3:"))
                btLampkaBocznik3.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-scnd4:"))
                btLampkaBocznik4.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-braking:"))
                btLampkaHamienie.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-braking-ezt:"))
                btLampkaHamowanie1zes.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-braking-ezt2:"))
                btLampkaHamowanie2zes.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-compressor:"))
                btLampkaSprezarka.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-compressorb:"))
                btLampkaSprezarkaB.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-voltbrake:"))
                btLampkaNapNastHam.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-mainbreakerb:"))
                btLampkaWylSzybkiB.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-resistorsb:"))
                btLampkaOporyB.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-contactorsb:"))
                btLampkaStycznB.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-conv_ovldb:"))
                btLampkaNadmPrzetwB.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-converterb:"))
                btLampkaPrzetwB.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-forward:"))
                btLampkaForward.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-backward:"))
                btLampkaBackward.Load(Parser, DynamicObject->mdKabina);
            else if (str == AnsiString("i-cablight:")) // hunter-171012
                btCabLight.Load(Parser, DynamicObject->mdKabina);
            // btLampkaUnknown.Init("unknown",mdKabina,false);
        }
    }
    else
    {
        delete Parser;
        return false;
    }
    // ABu 050205: tego wczesniej nie bylo:
    delete Parser;
    if (DynamicObject->mdKabina)
    {
        DynamicObject->mdKabina
            ->Init(); // obrócenie modelu oraz optymalizacja, równie¿ zapisanie binarnego
        return true;
    }
    return AnsiCompareStr(str, AnsiString("none"));
}

void TTrain::MechStop()
{ // likwidacja ruchu kamery w kabinie (po powrocie przez [F4])
    pMechPosition = vector3(0, 0, 0);
    pMechShake = vector3(0, 0, 0);
    vMechMovement = vector3(0, 0, 0);
    vMechVelocity = vector3(0, 0, 0); // tu zostawa³y jakieœ u³amki, powoduj¹ce uciekanie kamery
};

vector3 TTrain::MirrorPosition(bool lewe)
{ // zwraca wspó³rzêdne widoku kamery z lusterka
    switch (iCabn)
    {
    case 1: // przednia (1)
        return DynamicObject->mMatrix *
               vector3(lewe ? Cabine[iCabn].CabPos2.x : Cabine[iCabn].CabPos1.x,
                       1.5 + Cabine[iCabn].CabPos1.y, Cabine[iCabn].CabPos2.z);
    case 2: // tylna (-1)
        return DynamicObject->mMatrix *
               vector3(lewe ? Cabine[iCabn].CabPos1.x : Cabine[iCabn].CabPos2.x,
                       1.5 + Cabine[iCabn].CabPos1.y, Cabine[iCabn].CabPos1.z);
    }
    return DynamicObject->GetPosition(); // wspó³rzêdne œrodka pojazdu
};

void TTrain::DynamicSet(TDynamicObject *d)
{ // taka proteza: chcê pod³¹czyæ kabinê EN57 bezpoœrednio z silnikowym, aby nie robiæ tego przez
  // ukrotnienie
    // drugi silnikowy i tak musi byæ ukrotniony, podobnie jak kolejna jednostka
    // problem siê robi ze œwiat³ami, które bêd¹ zapalane w silnikowym, ale musz¹ œwieciæ siê w
    // rozrz¹dczych
    // dla EZT œwiat³a czo³owe bêd¹ "zapalane w silnikowym", ale widziane z rozrz¹dczych
    // równie¿ wczytywanie MMD powinno dotyczyæ aktualnego cz³onu
    // problematyczna mo¿e byæ kwestia wybranej kabiny (w silnikowym...)
    // jeœli silnikowy bêdzie zapiêty odwrotnie (tzn. -1), to i tak powinno jeŸdziæ dobrze
    // równie¿ hamowanie wykonuje siê zaworem w cz³onie, a nie w silnikowym...
    DynamicObject = d; // jedyne miejsce zmiany
    mvOccupied = mvControlled = d ? DynamicObject->MoverParameters : NULL; // albo silnikowy w EZT
    if (!DynamicObject)
        return;
    if (mvControlled->TrainType & dt_EZT) // na razie dotyczy to EZT
        if (DynamicObject->NextConnected ? mvControlled->Couplers[1].AllowedFlag & ctrain_depot :
                                           false)
        { // gdy jest cz³on od sprzêgu 1, a sprzêg ³¹czony warsztatowo (powiedzmy)
            if ((mvControlled->Power < 1.0) && (mvControlled->Couplers[1].Connected->Power >
                                                1.0)) // my nie mamy mocy, ale ten drugi ma
                mvControlled =
                    DynamicObject->NextConnected->MoverParameters; // bêdziemy sterowaæ tym z moc¹
        }
        else if (DynamicObject->PrevConnected ?
                     mvControlled->Couplers[0].AllowedFlag & ctrain_depot :
                     false)
        { // gdy jest cz³on od sprzêgu 0, a sprzêg ³¹czony warsztatowo (powiedzmy)
            if ((mvControlled->Power < 1.0) && (mvControlled->Couplers[0].Connected->Power >
                                                1.0)) // my nie mamy mocy, ale ten drugi ma
                mvControlled =
                    DynamicObject->PrevConnected->MoverParameters; // bêdziemy sterowaæ tym z moc¹
        }
    mvSecond = NULL; // gdyby siê nic nie znalaz³o
    if (mvOccupied->Power > 1.0) // dwucz³onowe lub ukrotnienia, ¿eby nie szukaæ ka¿dorazowo
        if (mvOccupied->Couplers[1].Connected ?
                mvOccupied->Couplers[1].AllowedFlag & ctrain_controll :
                false)
        { // gdy jest cz³on od sprzêgu 1, a sprzêg ³¹czony warsztatowo (powiedzmy)
            if (mvOccupied->Couplers[1].Connected->Power > 1.0) // ten drugi ma moc
                mvSecond =
                    (TMoverParameters *)mvOccupied->Couplers[1].Connected; // wskaŸnik na drugiego
        }
        else if (mvOccupied->Couplers[0].Connected ?
                     mvOccupied->Couplers[0].AllowedFlag & ctrain_controll :
                     false)
        { // gdy jest cz³on od sprzêgu 0, a sprzêg ³¹czony warsztatowo (powiedzmy)
            if (mvOccupied->Couplers[0].Connected->Power > 1.0) // ale ten drugi ma moc
                mvSecond =
                    (TMoverParameters *)mvOccupied->Couplers[0].Connected; // wskaŸnik na drugiego
        }
};

void TTrain::Silence()
{ // wyciszenie dŸwiêków przy wychodzeniu
    if (dsbNastawnikJazdy)
        dsbNastawnikJazdy->Stop();
    if (dsbNastawnikBocz)
        dsbNastawnikBocz->Stop();
    if (dsbRelay)
        dsbRelay->Stop();
    if (dsbPneumaticRelay)
        dsbPneumaticRelay->Stop();
    if (dsbSwitch)
        dsbSwitch->Stop();
    if (dsbPneumaticSwitch)
        dsbPneumaticSwitch->Stop();
    if (dsbReverserKey)
        dsbReverserKey->Stop();
    if (dsbCouplerAttach)
        dsbCouplerAttach->Stop();
    if (dsbCouplerDetach)
        dsbCouplerDetach->Stop();
    if (dsbDieselIgnition)
        dsbDieselIgnition->Stop();
    if (dsbDoorClose)
        dsbDoorClose->Stop();
    if (dsbDoorOpen)
        dsbDoorOpen->Stop();
    if (dsbPantUp)
        dsbPantUp->Stop();
    if (dsbPantDown)
        dsbPantDown->Stop();
    if (dsbWejscie_na_bezoporow)
        dsbWejscie_na_bezoporow->Stop();
    if (dsbWejscie_na_drugi_uklad)
        dsbWejscie_na_drugi_uklad->Stop();
    rsBrake.Stop();
    rsSlippery.Stop();
    rsHiss.Stop();
    rsHissU.Stop();
    rsHissE.Stop();
    rsHissX.Stop();
    rsHissT.Stop();
    rsSBHiss.Stop();
    rsRunningNoise.Stop();
    rsEngageSlippery.Stop();
    rsFadeSound.Stop();
    if (dsbHasler)
        dsbHasler->Stop(); // wy³¹czenie dŸwiêków opuszczanej kabiny
    if (dsbBuzzer)
        dsbBuzzer->Stop();
    if (dsbSlipAlarm)
        dsbSlipAlarm->Stop(); // dŸwiêk alarmu przy poœlizgu
    // sConverter.Stop();
    // sSmallCompressor->Stop();
    if (dsbCouplerStretch)
        dsbCouplerStretch->Stop();
    if (dsbEN57_CouplerStretch)
        dsbEN57_CouplerStretch->Stop();
    if (dsbBufferClamp)
        dsbBufferClamp->Stop();
};
