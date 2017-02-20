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

#include "stdafx.h"

#include "Train.h"

#include "Globals.h"
#include "Logs.h"
#include "MdlMngr.h"
#include "Timer.h"
#include "Driver.h"
#include "Console.h"
#include "McZapkie\hamulce.h"
#include "McZapkie\MOVER.h"
//---------------------------------------------------------------------------

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
    iGaugesMax = 100; // 95 - trzeba pobierać to z pliku konfiguracyjnego
    ggList = new TGauge[iGaugesMax];
    iGauges = 0; // na razie nie są dodane
    iButtonsMax = 60; // 55 - trzeba pobierać to z pliku konfiguracyjnego
    btList = new TButton[iButtonsMax];
    iButtons = 0;
}

void TCab::Init(double Initx1, double Inity1, double Initz1, double Initx2, double Inity2,
                double Initz2, bool InitEnabled, bool InitOccupied)
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

void TCab::Load(cParser &Parser)
{
    std::string token;
    Parser.getTokens();
    Parser >> token;
    if (token == "cablight")
    {
		Parser.getTokens( 9, false );
		Parser
			>> dimm_r
			>> dimm_g
			>> dimm_b
			>> intlit_r
			>> intlit_g
			>> intlit_b
			>> intlitlow_r
			>> intlitlow_g
			>> intlitlow_b;
		Parser.getTokens(); Parser >> token;
    }
	CabPos1.x = std::stod( token );
	Parser.getTokens( 5, false );
	Parser
		>> CabPos1.y
		>> CabPos1.z
		>> CabPos2.x
		>> CabPos2.y
		>> CabPos2.z;

    bEnabled = true;
    bOccupied = true;
}

TCab::~TCab()
{
    delete[] ggList;
    delete[] btList;
};

TGauge *TCab::Gauge(int n)
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
TButton *TCab::Button(int n)
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
        ggList[i].UpdateValue(); // odczyt parametru i przeliczenie na kąt
        ggList[i].Update(); // ustawienie animacji
    }
    for (i = 0; i < iButtons; ++i)
    { // animacje dwustanowe
        btList[i].Update(); // odczyt parametru i wybór submodelu
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
    fTachoTimer = 0.0; // włączenie skoków wskazań prędkościomierza

    //
    for( int i = 0; i < 8; i++ ) {
        bMains[ i ] = false;
        fCntVol[ i ] = 0.0f;
        bPants[ i ][ 0 ] = false;
        bPants[ i ][ 1 ] = false;
        bFuse[ i ] = false;
        bBatt[ i ] = false;
        bConv[ i ] = false;
        bComp[ i ][ 0 ] = false;
        bComp[ i ][ 1 ] = false;
        bHeat[ i ] = false;
    }
    for( int i = 0; i < 9; ++i )
        for( int j = 0; j < 10; ++j )
            fEIMParams[ i ][ j ] = 0.0;

    for( int i = 0; i < 20; ++i )
        for( int j = 0; j < 3; ++j )
            fPress[ i ][ j ] = 0.0;
}

TTrain::~TTrain()
{
    if (DynamicObject)
        if (DynamicObject->Mechanik)
            DynamicObject->Mechanik->TakeControl(
                true); // likwidacja kabiny wymaga przejęcia przez AI
}

bool TTrain::Init(TDynamicObject *NewDynamicObject, bool e3d)
{ // powiązanie ręcznego sterowania kabiną z pojazdem
    // Global::pUserDynamic=NewDynamicObject; //pojazd renderowany bez trzęsienia
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
    MechSpring.Init(0.015, 250);
    vMechVelocity = vector3(0, 0, 0);
    pMechOffset = vector3(-0.4, 3.3, 5.5);
    fMechCroach = 0.5;
    fMechSpringX = 1;
    fMechSpringY = 0.5;
    fMechSpringZ = 0.5;
    fMechMaxSpring = 0.15;
    fMechRoll = 0.05;
    fMechPitch = 0.1;
    fMainRelayTimer = 0; // Hunter, do k...y nędzy, ustawiaj wartości początkowe zmiennych!

	if( false == LoadMMediaFile( DynamicObject->asBaseDir + DynamicObject->MoverParameters->TypeName + ".mmd" ) ) {
        return false;
	}

    // McZapkie: w razie wykolejenia
    //    dsbDerailment=TSoundsManager::GetFromName("derail.wav");
    // McZapkie: jazda luzem:
    //    dsbRunningNoise=TSoundsManager::GetFromName("runningnoise.wav");

    // McZapkie? - dzwieki slyszalne tylko wewnatrz kabiny - generowane przez
    // obiekt sterowany:

    // McZapkie-080302 sWentylatory.Init("wenton.wav","went.wav","wentoff.wav");
    // McZapkie-010302
    //    sCompressor.Init("compressor-start.wav","compressor.wav","compressor-stop.wav");

    //    sHorn1.Init("horn1.wav",0.3);
    //    sHorn2.Init("horn2.wav",0.3);

    //  sHorn1.Init("horn1-start.wav","horn1.wav","horn1-stop.wav");
    //  sHorn2.Init("horn2-start.wav","horn2.wav","horn2-stop.wav");

    //  sConverter.Init("converter.wav",1.5); //NBMX obsluga przez AdvSound
    iCabn = 0;
    // Ra: taka proteza - przesłanie kierunku do członów connected
    if (mvControlled->ActiveDir > 0)
    { // było do przodu
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

PyObject *TTrain::GetTrainState() {
    PyObject *dict = PyDict_New();
    if( dict == NULL ) {
        return NULL;
    }

    PyDict_SetItemString( dict, "direction", PyGetInt( DynamicObject->MoverParameters->ActiveDir ) );
    PyDict_SetItemString( dict, "cab", PyGetInt( DynamicObject->MoverParameters->ActiveCab ) );
    PyDict_SetItemString( dict, "slipping_wheels",
        PyGetBool( DynamicObject->MoverParameters->SlippingWheels ) );
    PyDict_SetItemString( dict, "converter",
        PyGetBool( DynamicObject->MoverParameters->ConverterFlag ) );
    PyDict_SetItemString( dict, "main_ctrl_actual_pos",
        PyGetInt( DynamicObject->MoverParameters->MainCtrlActualPos ) );
    PyDict_SetItemString( dict, "scnd_ctrl_actual_pos",
        PyGetInt( DynamicObject->MoverParameters->ScndCtrlActualPos ) );
    PyDict_SetItemString( dict, "fuse", PyGetBool( DynamicObject->MoverParameters->FuseFlag ) );
    PyDict_SetItemString( dict, "converter_overload",
        PyGetBool( DynamicObject->MoverParameters->ConvOvldFlag ) );
    PyDict_SetItemString( dict, "voltage", PyGetFloat( DynamicObject->MoverParameters->Voltage ) );
    PyDict_SetItemString( dict, "velocity", PyGetFloat( DynamicObject->MoverParameters->Vel ) );
    PyDict_SetItemString( dict, "im", PyGetFloat( DynamicObject->MoverParameters->Im ) );
    PyDict_SetItemString( dict, "compress",
        PyGetBool( DynamicObject->MoverParameters->CompressorFlag ) );
    PyDict_SetItemString( dict, "hours", PyGetInt( GlobalTime->hh ) );
    PyDict_SetItemString( dict, "minutes", PyGetInt( GlobalTime->mm ) );
    PyDict_SetItemString( dict, "seconds", PyGetInt( GlobalTime->mr ) );
    PyDict_SetItemString( dict, "velocity_desired", PyGetFloat( DynamicObject->Mechanik->VelDesired ) );
    char* TXTT[ 10 ] = { "fd", "fdt", "fdb", "pd", "pdt", "pdb", "itothv", "1", "2", "3" };
    char* TXTC[ 10 ] = { "fr", "frt", "frb", "pr", "prt", "prb", "im", "vm", "ihv", "uhv" };
    char* TXTP[ 3 ] = { "bc", "bp", "sp" };
    for( int j = 0; j<10; j++ )
        PyDict_SetItemString( dict, ( std::string( "eimp_t_" ) + std::string( TXTT[ j ] ) ).c_str(), PyGetFloatS( fEIMParams[ 0 ][ j ] ) );
    for( int i = 0; i<8; i++ ) {
        for( int j = 0; j<10; j++ )
        PyDict_SetItemString( dict, ( std::string( "eimp_c" ) + std::to_string( i + 1 ) + std::string( "_" ) + std::string( TXTC[ j ] ) ).c_str(), PyGetFloatS( fEIMParams[ i + 1 ][ j ] ) );
        PyDict_SetItemString( dict, ( std::string( "eimp_c" ) + std::to_string( i + 1 ) + std::string( "_ms" ) ).c_str(), PyGetBool( bMains[ i ] ) );
        PyDict_SetItemString( dict, ( std::string( "eimp_c" ) + std::to_string( i + 1 ) + std::string( "_cv" ) ).c_str(), PyGetFloatS( fCntVol[ i ] ) );
        PyDict_SetItemString( dict, ( std::string( "eimp_u" ) + std::to_string( i + 1 ) + std::string( "_pf" ) ).c_str(), PyGetBool( bPants[ i ][ 0 ] ) );
        PyDict_SetItemString( dict, ( std::string( "eimp_u" ) + std::to_string( i + 1 ) + std::string( "_pr" ) ).c_str(), PyGetBool( bPants[ i ][ 1 ] ) );
        PyDict_SetItemString( dict, ( std::string( "eimp_c" ) + std::to_string( i + 1 ) + std::string( "_fuse" ) ).c_str(), PyGetBool( bFuse[ i ] ) );
        PyDict_SetItemString( dict, ( std::string( "eimp_c" ) + std::to_string( i + 1 ) + std::string( "_batt" ) ).c_str(), PyGetBool( bBatt[ i ] ) );
        PyDict_SetItemString( dict, ( std::string( "eimp_c" ) + std::to_string( i + 1 ) + std::string( "_conv" ) ).c_str(), PyGetBool( bConv[ i ] ) );
        PyDict_SetItemString( dict, ( std::string( "eimp_u" ) + std::to_string( i + 1 ) + std::string( "_comp_a" ) ).c_str(), PyGetBool( bComp[ i ][ 0 ] ) );
        PyDict_SetItemString( dict, ( std::string( "eimp_u" ) + std::to_string( i + 1 ) + std::string( "_comp_w" ) ).c_str(), PyGetBool( bComp[ i ][ 1 ] ) );
        PyDict_SetItemString( dict, ( std::string( "eimp_c" ) + std::to_string( i + 1 ) + std::string( "_heat" ) ).c_str(), PyGetBool( bHeat[ i ] ) );

    }
    for( int i = 0; i<20; i++ ) {
        for( int j = 0; j<3; j++ )
            PyDict_SetItemString( dict, ( std::string( "eimp_pn" ) + std::to_string( i + 1 ) + std::string( "_" ) + std::string( TXTP[ j ] ) ).c_str(),
            PyGetFloatS( fPress[ i ][ j ] ) );
    }
    bool bEP, bPN;
    bEP = ( mvControlled->LocHandle->GetCP()>0.2 ) || ( fEIMParams[ 0 ][ 2 ]>0.01 );
    PyDict_SetItemString( dict, "dir_brake", PyGetBool( bEP ) );
    if( ( typeid( *mvControlled->Hamulec ) == typeid( TLSt ) )
     || ( typeid( *mvControlled->Hamulec ) == typeid( TEStED ) ) ) {

        TBrake* temp_ham = mvControlled->Hamulec.get();
        //        TLSt* temp_ham2 = temp_ham;
        bPN = ( static_cast<TLSt*>( temp_ham )->GetEDBCP()>0.2 );
    }
    else
        bPN = false;
    PyDict_SetItemString( dict, "indir_brake", PyGetBool( bPN ) );
    for( int i = 0; i<20; i++ ) {
        PyDict_SetItemString( dict, ( std::string( "doors_" ) + std::to_string( i + 1 ) ).c_str(), PyGetFloatS( bDoors[ i ][ 0 ] ) );
        PyDict_SetItemString( dict, ( std::string( "doors_r_" ) + std::to_string( i + 1 ) ).c_str(), PyGetFloatS( bDoors[ i ][ 1 ] ) );
        PyDict_SetItemString( dict, ( std::string( "doors_l_" ) + std::to_string( i + 1 ) ).c_str(), PyGetFloatS( bDoors[ i ][ 2 ] ) );
        PyDict_SetItemString( dict, ( std::string( "doors_no_" ) + std::to_string( i + 1 ) ).c_str(), PyGetInt( iDoorNo[ i ] ) );
        PyDict_SetItemString( dict, ( std::string( "code_" ) + std::to_string( i + 1 ) ).c_str(), PyGetString( std::string( std::to_string( iUnits[ i ] ) +
            cCode[ i ] ).c_str() ) );
        PyDict_SetItemString( dict, ( std::string( "car_name" ) + std::to_string( i + 1 ) ).c_str(), PyGetString( asCarName[ i ].c_str() ) );
    }
    PyDict_SetItemString( dict, "car_no", PyGetInt( iCarNo ) );
    PyDict_SetItemString( dict, "power_no", PyGetInt( iPowerNo ) );
    PyDict_SetItemString( dict, "unit_no", PyGetInt( iUnitNo ) );
    PyDict_SetItemString( dict, "universal3", PyGetBool( LampkaUniversal3_st ) );
    PyDict_SetItemString( dict, "ca", PyGetBool( TestFlag( mvOccupied->SecuritySystem.Status, s_aware ) ) );
    PyDict_SetItemString( dict, "shp", PyGetBool( TestFlag( mvOccupied->SecuritySystem.Status, s_active ) ) );
    PyDict_SetItemString( dict, "manual_brake", PyGetBool( mvOccupied->ManualBrakePos > 0 ) );
    PyDict_SetItemString( dict, "pantpress", PyGetFloat( mvControlled->PantPress ) );
    PyDict_SetItemString( dict, "trainnumber", PyGetString( DynamicObject->Mechanik->TrainName().c_str() ) );
    PyDict_SetItemString( dict, "velnext", PyGetFloat( DynamicObject->Mechanik->VelNext ) );
    PyDict_SetItemString( dict, "actualproximitydist", PyGetFloat( DynamicObject->Mechanik->ActualProximityDist ) );
    PyDict_SetItemString( dict, "velsignallast", PyGetFloat( DynamicObject->Mechanik->VelSignalLast ) );
    PyDict_SetItemString( dict, "vellimitlast", PyGetFloat( DynamicObject->Mechanik->VelLimitLast ) );
    PyDict_SetItemString( dict, "velroad", PyGetFloat( DynamicObject->Mechanik->VelRoad ) );
    PyDict_SetItemString( dict, "velsignalnext", PyGetFloat( DynamicObject->Mechanik->VelSignalNext ) );
    PyDict_SetItemString( dict, "battery", PyGetBool( mvControlled->Battery ) );
    PyDict_SetItemString( dict, "tractionforce", PyGetFloat( DynamicObject->MoverParameters->Ft ) );

    return dict;
}

void TTrain::OnKeyDown(int cKey)
{ // naciśnięcie klawisza
    bool isEztOer;
    isEztOer = ((mvControlled->TrainType == dt_EZT) && (mvControlled->Battery == true) &&
                (mvControlled->EpFuse == true) && (mvOccupied->BrakeSubsystem == ss_ESt) &&
                (mvControlled->ActiveDir != 0)); // od yB
    // isEztOer=(mvControlled->TrainType==dt_EZT)&&(mvControlled->Mains)&&(mvOccupied->BrakeSubsystem==ss_ESt)&&(mvControlled->ActiveDir!=0);
    // isEztOer=((mvControlled->TrainType==dt_EZT)&&(mvControlled->Battery==true)&&(mvControlled->EpFuse==true)&&(mvOccupied->BrakeSubsystem==Oerlikon)&&(mvControlled->ActiveDir!=0));

    if (GetAsyncKeyState(VK_SHIFT) < 0)
	{ // wciśnięty [Shift]
        if (cKey == Global::Keys[k_IncMainCtrlFAST]) // McZapkie-200702: szybkie
        // przelaczanie na poz.
        // bezoporowa
        {
            if (mvControlled->IncMainCtrl(2))
            {
                dsbNastawnikJazdy->SetCurrentPosition(0);
                dsbNastawnikJazdy->Play(0, 0, 0);
            }
        }
        else if (cKey == Global::Keys[k_DirectionBackward])
        {
            if (mvOccupied->Radio == false)
                if (GetAsyncKeyState(VK_CONTROL) >= 0)
                {
                    dsbSwitch->SetVolume(DSBVOLUME_MAX);
                    dsbSwitch->Play(0, 0, 0);
                    mvOccupied->Radio = true;
                }
        }
        else if (cKey == Global::Keys[k_DecMainCtrlFAST])
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
			{ // wyłącznik jest też w SN61, ewentualnie
				// załączać prąd na stałe z poziomu FIZ
				if (mvOccupied->BatterySwitch(true)) // bateria potrzebna np. do zapalenia świateł
                {
                    dsbSwitch->Play(0, 0, 0);
                    if (ggBatteryButton.SubModel)
                    {
                        ggBatteryButton.PutValue(1);
                    }
                    if (mvOccupied->LightsPosNo > 0)
                    {
                        SetLights();
                    }
                    if (TestFlag(mvOccupied->SecuritySystem.SystemType,
						2)) // Ra: znowu w kabinie jest coś, co być nie powinno!
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
                if (ggDoorSignallingButton.SubModel != NULL)
                {
                    if (!mvControlled->DoorSignalling)
                    {
                        mvOccupied->DoorBlocked = true;
                        dsbSwitch->Play(0, 0, 0);
                        mvControlled->DoorSignalling = true;
                    }
                }
            }
            else if (mvControlled->TrainType != dt_EZT)
            {
                if (ggSandButton.SubModel != NULL)
                {
                    ggSandButton.PutValue(1);
                    // dsbPneumaticRelay->SetVolume(-80);
                    // dsbPneumaticRelay->Play(0, 0, 0);
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
        else if (cKey == Global::Keys[k_BrakeProfile]) // McZapkie-240302-B:
        //-----------

        // przelacznik opoznienia
        // hamowania
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
        //-----------
        // hunter-261211: przetwornica i sprzezarka przeniesione do
        // TTrain::Update()
        /* if (cKey==Global::Keys[k_Converter])   //NBMX 14-09-2003:
przetwornica wl
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
          if ((mvControlled->ConverterFlag) ||
(mvControlled->CompressorPower<2))
                 if (mvControlled->CompressorSwitch(true))
                 {
                         dsbSwitch->SetVolume(DSBVOLUME_MAX);
                         dsbSwitch->Play(0,0,0);
                 }
        }
        else */

        else if (cKey == Global::Keys[k_Converter])
        {
            if (ggConverterButton.GetValue() == 0)
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play(0, 0, 0);
            }
        }
        else if ((cKey == Global::Keys[k_Compressor]) &&
                 (mvControlled->CompressorPower < 2)) // hunter-091012: tak jest poprawnie
        // if
        // ((cKey==Global::Keys[k_Compressor])&&((mvControlled->EngineType==ElectricSeriesMotor)||(mvControlled->TrainType==dt_EZT)))
        // //hunter-110212: poprawka dla EZT

        {
            if (ggCompressorButton.GetValue() == 0)
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play(0, 0, 0);
            }
        }
        else if (cKey == Global::Keys[k_SmallCompressor]) // Winger 160404: mala
        // sprezarka wl
		{ // Ra: dźwięk, gdy razem z [Shift]
            if ((mvControlled->TrainType & dt_EZT) ? mvControlled == mvOccupied :
                                                     !mvOccupied->ActiveCab) // tylko w maszynowym
                if (Console::Pressed(VK_CONTROL)) // z [Ctrl]
					mvControlled->bPantKurek3 = true; // zbiornik pantografu połączony
				// jest ze zbiornikiem głównym
                // (pompowanie nie ma sensu)
				else if (!mvControlled->PantCompFlag) // jeśli wyłączona
					if (mvControlled->Battery) // jeszcze musi być załączona bateria
						if (mvControlled->PantPress < 4.8) // piszą, że to tak nie działa
                        {
                            mvControlled->PantCompFlag = true;
                            dsbSwitch->SetVolume(DSBVOLUME_MAX);
							dsbSwitch->Play(0, 0, 0); // dźwięk tylko po naciśnięciu klawisza
                        }
        }
		else if (cKey == VkKeyScan('q')) // ze Shiftem - włączenie AI
        { // McZapkie-240302 - wlaczanie automatycznego pilota (zadziala tylko w
            // trybie debugmode)
            if (DynamicObject->Mechanik)
            {
                if (DebugModeFlag)
					if (DynamicObject->Mechanik->AIControllFlag) //żeby nie trzeba było
						// rozłączać dla
                        // zresetowania
                        DynamicObject->Mechanik->TakeControl(false);
                DynamicObject->Mechanik->TakeControl(true);
            }
        }
        else if (cKey == Global::Keys[k_MaxCurrent]) // McZapkie-160502: F -
        // wysoki rozruch
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
				   if (mvControlled->TrainType!=dt_EZT) //to powinno być w fizyce, a
               nie w kabinie!
                            if (mvControlled->MinCurrentSwitch(true))
                            {
                             dsbSwitch->SetVolume(DSBVOLUME_MAX);
                             dsbSwitch->Play(0,0,0);
                            }
            */
        }
        else if (cKey == Global::Keys[k_CurrentAutoRelay]) // McZapkie-241002: G -
        // wlaczanie PSR
        {
            if (mvControlled->AutoRelaySwitch(true))
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play(0, 0, 0);
            }
        }
        else if (cKey == Global::Keys[k_FailedEngineCutOff]) // McZapkie-060103: E
        // - wylaczanie
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
				if (mvOccupied->CabNo < 0 ?	mvOccupied->DoorRight(true) : mvOccupied->DoorLeft(true))
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
			if (mvOccupied->DoorOpenCtrl == 1)
				if (mvOccupied->CabNo < 0 ? mvOccupied->DoorLeft(true) : mvOccupied->DoorRight(true))
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
        //-----------
        // hunter-131211: dzwiek dla przelacznika universala podniesionego
        // hunter-091012: ubajerowanie swiatla w kabinie (wyrzucenie
        // przyciemnienia pod Univ4)
        else if (cKey == Global::Keys[k_Univ3])
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
				  {//z [Ctrl] zapalamy albo gasimy światełko w kabinie
                   if (iCabLightFlag<2) ++iCabLightFlag; //zapalenie
                  }
                */
            }
        }

        //-----------
        // hunter-091012: dzwiek dla przyciemnienia swiatelka w kabinie
        else if (cKey == Global::Keys[k_Univ4])
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

        //-----------
        else if (cKey == Global::Keys[k_PantFrontUp])
        { // Winger 160204: podn.
            // przedn. pantografu
            if (mvOccupied->ActiveCab ==
                1) //||((mvOccupied->ActiveCab<1)&&((mvControlled->TrainType&(dt_ET40|dt_ET41|dt_ET42|dt_EZT))==0)))
			{ // przedni gdy w kabinie 1 lub (z wyjątkiem ET40, ET41, ET42 i EZT) gdy
                // w kabinie -1
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
        { // Winger 160204: podn.
            // tyln. pantografu
			// względem kierunku jazdy
            if (mvOccupied->ActiveCab ==
                1) //||((mvOccupied->ActiveCab<1)&&((mvControlled->TrainType&(dt_ET40|dt_ET41|dt_ET42|dt_EZT))==0)))
			{ // tylny gdy w kabinie 1 lub (z wyjątkiem ET40, ET41, ET42 i EZT) gdy w
                // kabinie -1
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
		{ // Ra 2014-06: uruchomiłem to, aby aktywować czuwak w zajmowanym członie,
			// a wyłączyć w innych
			// Ra 2014-03: aktywacja czuwaka przepięta na ustawienie kierunku w
            // mvOccupied
			// if (mvControlled->Battery) //jeśli bateria jest już załączona
			// mvOccupied->BatterySwitch(true); //to w ten oto durny sposób aktywuje
			// się CA/SHP
            //        if (mvControlled->CabActivisation())
            //           {
            //            dsbSwitch->SetVolume(DSBVOLUME_MAX);
            //            dsbSwitch->Play(0,0,0);
            //           }
        }
        else if (cKey == Global::Keys[k_Heating]) // Winger 020304: ogrzewanie
        // skladu - wlaczenie
		{ // Ra 2014-09: w trybie latania obsługa jest w World.cpp
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
		else if (cKey == Global::Keys[k_LeftSign]) // lewe swiatlo - włączenie
        // ABu 060205: dzielo Wingera po malutkim liftingu:
        {
			if (false == (mvOccupied->LightsPosNo > 0))
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
        }
		else if (cKey == Global::Keys[k_UpperSign]) // ABu 060205: światło górne -
		// włączenie
        {
			if (mvOccupied->LightsPosNo > 0) //kręciolek od swiatel
            {
                if ((mvOccupied->LightsPos < mvOccupied->LightsPosNo) || (mvOccupied->LightsWrap))
                {
                    mvOccupied->LightsPos++;
                    if (mvOccupied->LightsPos > mvOccupied->LightsPosNo)
                    {
                        mvOccupied->LightsPos = 1;
                    }
                    dsbSwitch->SetVolume(DSBVOLUME_MAX);
                    dsbSwitch->Play(0, 0, 0);
                    SetLights();
                }
            }
            else if ((GetAsyncKeyState(VK_CONTROL) < 0) &&
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
        else if (cKey == Global::Keys[k_RightSign]) // Winger 070304: swiatla
        // tylne (koncowki) -
        // wlaczenie
        {
			if (false == (mvOccupied->LightsPosNo > 0))
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
            //         if
            //         (mvControlled->EnginePowerSource.SourceType==CurrentCollector)
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
        { // Ra 2014-09: w
            // trybie latania
            // obsługa jest w
            // World.cpp
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
        { // Ra 2014-06: wersja dla
            // swobodnego latania
            // przeniesiona do
            // World.cpp
            if (!FreeFlyModeFlag)
            {
                if (GetAsyncKeyState(VK_CONTROL) < 0)
                    if ((mvOccupied->LocalBrake == ManualBrake) || (mvOccupied->MBrake == true))
                        mvOccupied->DecManualBrakeLevel(1);
                    else
                        ;
                else // Ra 1014-06: AI potrafi zahamować pomocniczym mimo jego braku -
                    // odhamować jakoś trzeba
                    if ((mvOccupied->LocalBrake != ManualBrake) || mvOccupied->LocalBrakePos)
                    mvOccupied->DecLocalBrakeLevel(1);
            }
        }
        else if ((cKey == Global::Keys[k_IncBrakeLevel]) && (mvOccupied->BrakeHandle != FV4a))
            // if (mvOccupied->IncBrakeLevel())
            if (mvOccupied->BrakeLevelAdd(Global::fBrakeStep)) // nieodpowiedni
            // warunek; true, jeśli
            // można dalej kręcić
            {
                keybrakecount = 0;
                if ((isEztOer) && (mvOccupied->BrakeCtrlPos < 3))
                { // Ra: uzależnić dźwięk od zmiany stanu EP, nie od klawisza
                    dsbPneumaticSwitch->SetVolume(-10);
                    dsbPneumaticSwitch->Play(0, 0, 0);
                }
            }
            else
                ;
        else if ((cKey == Global::Keys[k_DecBrakeLevel]) && (mvOccupied->BrakeHandle != FV4a))
        {
            // nową wersję dostarczył ZiomalCl ("fixed looped sound in ezt when using
            // NUM_9 key")
            if ((mvOccupied->BrakeCtrlPos > -1) || (keybrakecount > 1))
            {

                if ((isEztOer) && (mvControlled->Mains) && (mvOccupied->BrakeCtrlPos != -1))
                { // Ra: uzależnić dźwięk od zmiany stanu EP, nie od klawisza
                    dsbPneumaticSwitch->SetVolume(-10);
                    dsbPneumaticSwitch->Play(0, 0, 0);
                }
                // mvOccupied->DecBrakeLevel();
                mvOccupied->BrakeLevelAdd(-Global::fBrakeStep);
            }
            else
                keybrakecount += 1;
            // koniec wersji dostarczonej przez ZiomalCl
            /* wersja poprzednia - ten pierwszy if ze średnikiem nie działał jak
               warunek
                     if ((mvOccupied->BrakeCtrlPos>-1)|| (keybrakecount>1))
                      {
                        if (mvOccupied->DecBrakeLevel());
                          {
                          if ((isEztOer) &&
               (mvOccupied->BrakeCtrlPos<2)&&(keybrakecount<=1))
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
            if (mvOccupied->BrakeCtrlPosNo <= 0.1) // hamulec bezpieczeństwa dla wagonów
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
                mvOccupied->BrakeLevelSet(
                    mvOccupied->Handle->GetPos(bh_NP)); // yB: czy ten stos funkcji nie
            // powinien być jako oddzielna
            // funkcja movera?
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
        else if (cKey == Global::Keys[k_Czuwak])
        //---------------
        // hunter-131211: zbicie czuwaka przeniesione do TTrain::Update()
        { // Ra: tu został tylko dźwięk
            // dsbBuzzer->Stop();
            // if (mvOccupied->SecuritySystemReset())
            if (fabs(ggSecurityResetButton.GetValue()) < 0.001)
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play(0, 0, 0);
            }
            // ggSecurityResetButton.PutValue(1);
        }
        else if (cKey == Global::Keys[k_AntiSlipping])
        //---------------
        // hunter-221211: hamulec przeciwposlizgowy przeniesiony do
        // TTrain::Update()
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
        else if (cKey == Global::Keys[k_Fuse])
        //---------------
        {
            if (GetAsyncKeyState(VK_CONTROL) < 0) // z controlem
            {
                ggConverterFuseButton.PutValue(1); // hunter-261211
                if ((mvControlled->Mains == false) && (ggConverterButton.GetValue() == 0) &&
                    (mvControlled->TrainType != dt_EZT))
                    mvControlled->ConvOvldFlag = false;
            }
            else
            {
                ggFuseButton.PutValue(1);
                mvControlled->FuseOn();
            }
        }
        else if (cKey == Global::Keys[k_DirectionForward])
        // McZapkie-240302 - zmiana kierunku: 'd' do przodu, 'r' do tylu
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
                if (mvOccupied->ActiveDir) // jeśli kierunek niezerowy
                    if (DynamicObject->Mechanik) // na wszelki wypadek
                        DynamicObject->Mechanik->CheckVehicles(
                            Change_direction); // aktualizacja skrajnych pojazdów w składzie
            }
        }
        else if (cKey == Global::Keys[k_DirectionBackward]) // r
        {
            if (GetAsyncKeyState(VK_CONTROL) < 0)
            { // wciśnięty [Ctrl]
                if (mvOccupied->Radio == true)
                {
                    dsbSwitch->SetVolume(DSBVOLUME_MAX);
                    dsbSwitch->Play(0, 0, 0);
                    mvOccupied->Radio = false;
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
                if (mvOccupied->ActiveDir) // jeśli kierunek niezerowy
                    if (DynamicObject->Mechanik) // na wszelki wypadek
                        DynamicObject->Mechanik->CheckVehicles(
                            Change_direction); // aktualizacja skrajnych pojazdów w składzie
            }
        }
        else if (cKey == Global::Keys[k_Main])
        // McZapkie-240302 - wylaczanie glownego obwodu
        //-----------
        // hunter-141211: wyl. szybki wylaczony przeniesiony do TTrain::Update()
        {
            if (fabs(ggMainOffButton.GetValue()) < 0.001)
                if (dsbSwitch)
                {
                    dsbSwitch->SetVolume(DSBVOLUME_MAX);
                    dsbSwitch->Play(0, 0, 0);
                }
        }
        else if (cKey == Global::Keys[k_Battery])
        {
            // if ((mvControlled->TrainType==dt_EZT) ||
            // (mvControlled->EngineType==ElectricSeriesMotor)||
            // (mvControlled->EngineType==DieselElectric))
            if (mvOccupied->BatterySwitch(false))
            { // ewentualnie zablokować z FIZ,
                // np. w samochodach się nie
                // odłącza akumulatora
                if (ggBatteryButton.SubModel)
                {
                    ggBatteryButton.PutValue(0);
                }
                dsbSwitch->Play(0, 0, 0);
                // mvOccupied->SecuritySystem.Status=0;
                mvControlled->PantFront(false);
                mvControlled->PantRear(false);
            }
        }

        //-----------
        //      if (cKey==Global::Keys[k_Active])   //yB 300407: przelacznik
        //      rozrzadu
        //      {
        //        if (mvControlled->CabDeactivisation())
        //           {
        //            dsbSwitch->SetVolume(DSBVOLUME_MAX);
        //            dsbSwitch->Play(0,0,0);
        //           }
        //      }
        //      else
        if (cKey == Global::Keys[k_BrakeProfile])
        { // yB://ABu: male poprawki, zeby
            // bylo mozna ustawic dowolny
            // wagon
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
        else if (cKey == Global::Keys[k_Converter])
        //-----------
        // hunter-261211: przetwornica i sprzezarka przeniesione do
        // TTrain::Update()
        {
            if (ggConverterButton.GetValue() != 0)
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play(0, 0, 0);
            }
        }
        // if
        // ((cKey==Global::Keys[k_Compressor])&&((mvControlled->EngineType==ElectricSeriesMotor)||(mvControlled->TrainType==dt_EZT)))
        // //hunter-110212: poprawka dla EZT
        else if ((cKey == Global::Keys[k_Compressor]) &&
                 (mvControlled->CompressorPower < 2)) // hunter-091012: tak jest poprawnie
        {
            if (ggCompressorButton.GetValue() != 0)
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play(0, 0, 0);
            }
        }
        //-----------
        else if (cKey == Global::Keys[k_Releaser]) // odluzniacz
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
                            mvOccupied->BrakeReleaser(1);
                            // if (mvOccupied->BrakeReleaser(1))
                            // {
                            // dsbPneumaticRelay->SetVolume(-80);
                            // dsbPneumaticRelay->Play(0, 0, 0);
                            // }
                        }
            }
        }
        else if (cKey == Global::Keys[k_SmallCompressor]) // Winger 160404: mala
        // sprezarka wl
        { // Ra: bez [Shift] też dać dźwięk
            if ((mvControlled->TrainType & dt_EZT) ? mvControlled == mvOccupied :
                                                     !mvOccupied->ActiveCab) // tylko w maszynowym
                if (Console::Pressed(VK_CONTROL)) // z [Ctrl]
                    mvControlled->bPantKurek3 =
                        false; // zbiornik pantografu połączony jest z małą sprężarką
                // (pompowanie ma sens, ale potem trzeba przełączyć)
                else if (!mvControlled->PantCompFlag) // jeśli wyłączona
                    if (mvControlled->Battery) // jeszcze musi być załączona bateria
                        if (mvControlled->PantPress < 4.8) // piszą, że to tak nie działa
                        {
                            mvControlled->PantCompFlag = true;
                            dsbSwitch->SetVolume(DSBVOLUME_MAX);
                            dsbSwitch->Play(0, 0, 0); // dźwięk tylko po naciśnięciu klawisza
                        }
        }
        // McZapkie-240302 - wylaczanie automatycznego pilota (w trybie ~debugmode
        // mozna tylko raz)
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
        else if (cKey == Global::Keys[k_CurrentAutoRelay]) // McZapkie-241002: g -
        // wylaczanie PSR
        {
            if (mvControlled->AutoRelaySwitch(false))
            {
                dsbSwitch->SetVolume(DSBVOLUME_MAX);
                dsbSwitch->Play(0, 0, 0);
            }
        }

        // hunter-201211: piasecznica poprawiona oraz przeniesiona do
        // TTrain::Update()
        else if (cKey == Global::Keys[k_Sand])
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
                if (ggDoorSignallingButton.SubModel != NULL)
                {
                    if (mvControlled->DoorSignalling)
                    {
                        mvOccupied->DoorBlocked = false;
                        dsbSwitch->Play(0, 0, 0);
                        mvControlled->DoorSignalling = false;
                    }
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
                mvControlled->PantCompFlag = false; // wyjście z maszynowego wyłącza sprężarkę
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
                    false; // wyjście z maszynowego wyłącza sprężarkę pomocniczą
        }
        else if (cKey == Global::Keys[k_Couple])
        { // ABu051104: male zmiany, zeby
            // mozna bylo laczyc odlegle
            // wagony
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
                       //ABu: aha, a guzik, nie dziala i nie bedzie, a przydalo by sie
        cos takiego:
                       //DynamicObject->NextConnected=mvControlled->Couplers[iCabn-1].Connected;
                       //DynamicObject->PrevConnected=mvControlled->Couplers[iCabn-1].Connected;
                     }
                   }
                   else
                   if
        (!TestFlag(mvControlled->Couplers[iCabn-1].CouplingFlag,ctrain_pneumatic))
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
                    int CouplNr = -1; // normalnie żaden ze sprzęgów
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
                                    // //podłączony sprzęg będzie widoczny
                                    if (DynamicObject->Mechanik) // na wszelki wypadek
                                        DynamicObject->Mechanik->CheckVehicles(
                                            Connect); // aktualizacja flag kierunku w składzie
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
                                                                1); // Ra: to mi się nie podoba !!!!
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
                                                                0); // Ra: to mi się nie podoba !!!!
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
        { // ABu051104: male zmiany,
            // zeby mozna bylo rozlaczac
            // odlegle wagony
            if (iCabn > 0)
            {
                if (!FreeFlyModeFlag) // tryb 'kabinowy' (pozwala również rozłączyć
                // sprzęgi zablokowane)
                {
                    if (DynamicObject->DettachStatus(iCabn - 1) < 0) // jeśli jest co odczepić
                        if (DynamicObject->Dettach(iCabn - 1)) // iCab==1:przód,iCab==2:tył
                        {
                            dsbCouplerDetach->SetVolume(DSBVOLUME_MAX); // w kabinie ten dźwięk?
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
                            0) // jeżeli sprzęg niezablokowany
                            if (tmp->DettachStatus(CouplNr) < 0) // jeśli jest co odczepić i się da
                                if (!tmp->Dettach(CouplNr))
                                { // dźwięk odczepiania
                                    dsbCouplerDetach->SetVolume(DSBVOLUME_MAX);
                                    dsbCouplerDetach->Play(0, 0, 0);
                                }
                    }
                }
                if (DynamicObject->Mechanik) // na wszelki wypadek
                    DynamicObject->Mechanik->CheckVehicles(
                        Disconnect); // aktualizacja skrajnych pojazdów w składzie
            }
        }
        else if (cKey == Global::Keys[k_CloseLeft]) // NBMX 17-09-2003: zamykanie drzwi
        {
			if( mvOccupied->DoorCloseCtrl == 1 ) {
				if( mvOccupied->CabNo < 0 ? mvOccupied->DoorRight( false ) : mvOccupied->DoorLeft( false ) ) {
					dsbSwitch->SetVolume( DSBVOLUME_MAX );
					dsbSwitch->Play( 0, 0, 0 );
					if( dsbDoorClose ) {
						dsbDoorClose->SetCurrentPosition( 0 );
						dsbDoorClose->Play( 0, 0, 0 );
					}
                }
            }
        }
        else if (cKey == Global::Keys[k_CloseRight]) // NBMX 17-09-2003: zamykanie drzwi
        {
			if( mvOccupied->DoorCloseCtrl == 1 ) {
				if( mvOccupied->CabNo < 0 ? mvOccupied->DoorLeft( false ) : mvOccupied->DoorRight( false ) ) {
					dsbSwitch->SetVolume( DSBVOLUME_MAX );
					dsbSwitch->Play( 0, 0, 0 );
					if( dsbDoorClose ) {
						dsbDoorClose->SetCurrentPosition( 0 );
						dsbDoorClose->Play( 0, 0, 0 );
					}
                }
            }
        }

        //-----------
        // hunter-131211: dzwiek dla przelacznika universala
        // hunter-091012: ubajerowanie swiatla w kabinie (wyrzucenie
        // przyciemnienia pod Univ4)
        else if (cKey == Global::Keys[k_Univ3])
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
                  {//z [Ctrl] zapalamy albo gasimy światełko w kabinie
                   if (iCabLightFlag) --iCabLightFlag; //gaszenie
                  } */
            }
        }

        //-----------
        // hunter-091012: dzwiek dla przyciemnienia swiatelka w kabinie
        else if (cKey == Global::Keys[k_Univ4])
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
        else if (cKey == Global::Keys[k_PantFrontDown]) // Winger 160204:
        // opuszczanie prz. patyka
        {
            if (mvOccupied->ActiveCab ==
                1) //||((mvOccupied->ActiveCab<1)&&(mvControlled->TrainType!=dt_ET40)&&(mvControlled->TrainType!=dt_ET41)&&(mvControlled->TrainType!=dt_ET42)&&(mvControlled->TrainType!=dt_EZT)))
            {
                // if (!mvControlled->PantFrontUp) //jeśli był opuszczony
                // if () //jeśli połamany
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
        else if (cKey == Global::Keys[k_PantRearDown]) // Winger 160204:
        // opuszczanie tyl. patyka
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
        else if (cKey == Global::Keys[k_Heating]) // Winger 020304: ogrzewanie -
        // wylaczenie
        { // Ra 2014-09: w trybie latania obsługa jest w World.cpp
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
        else if (cKey == Global::Keys[k_LeftSign]) // ABu 060205: lewe swiatlo -
        // wylaczenie
        {
			if (false == (mvOccupied->LightsPosNo > 0))
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
        }
        else if (cKey == Global::Keys[k_UpperSign]) // ABu 060205: światło górne -
        // wyłączenie
        {
			if (mvOccupied->LightsPosNo > 0) //kręciolek od swiatel
            {
                if ((mvOccupied->LightsPos > 1) || (mvOccupied->LightsWrap))
                {
                    mvOccupied->LightsPos--;
                    if (mvOccupied->LightsPos < 1)
                    {
                        mvOccupied->LightsPos = mvOccupied->LightsPosNo;
                    }
                    dsbSwitch->SetVolume(DSBVOLUME_MAX);
                    dsbSwitch->Play(0, 0, 0);
                    SetLights();
                }
            }
            else if ((GetAsyncKeyState(VK_CONTROL) < 0) &&
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
        if (cKey == Global::Keys[k_RightSign]) // Winger 070304: swiatla tylne
        // (koncowki) - wlaczenie
        {
			if (false == (mvOccupied->LightsPosNo > 0))
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
        }
        else if (cKey == Global::Keys[k_StLinOff]) // Winger 110904: wylacznik st.
        // liniowych
        {
            if ((mvControlled->TrainType != dt_EZT) && (mvControlled->TrainType != dt_EP05) &&
                (mvControlled->TrainType != dt_ET40))
            {
                ggStLinOffButton.PutValue(1); // Ra: było Fuse...
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
                        if (!FreeFlyModeFlag) //żeby nie mieszać obserwując z zewnątrz
                            DynamicObject->Mechanik->RouteSwitch(
                                1); // na skrzyżowaniu skręci w lewo
                }
                else if (cKey == Global::Keys[k_MechRight])
                {
                    vMechMovement.x -= fMechCroach;
                    if (DynamicObject->Mechanik)
                        if (!FreeFlyModeFlag) //żeby nie mieszać obserwując z zewnątrz
                            DynamicObject->Mechanik->RouteSwitch(
                                2); // na skrzyżowaniu skręci w prawo
                }
                else if (cKey == Global::Keys[k_MechBackward])
                {
                    vMechMovement.z -= fMechCroach;
                    // if (DynamicObject->Mechanik)
                    // if (!FreeFlyModeFlag) //żeby nie mieszać obserwując z zewnątrz
                    //  DynamicObject->Mechanik->RouteSwitch(0);  //na skrzyżowaniu stanie
                    //  i poczeka
                }
                else if (cKey == Global::Keys[k_MechForward])
                {
                    vMechMovement.z += fMechCroach;
                    if (DynamicObject->Mechanik)
                        if (!FreeFlyModeFlag) //żeby nie mieszać obserwując z zewnątrz
                            DynamicObject->Mechanik->RouteSwitch(
                                3); // na skrzyżowaniu pojedzie prosto
                }
                else if (cKey == Global::Keys[k_MechUp])
                    pMechOffset.y += 0.2; // McZapkie-120302 - wstawanie
                else if (cKey == Global::Keys[k_MechDown])
                    pMechOffset.y -= 0.2; // McZapkie-120302 - siadanie
            }
        }

        //    else
        if (DebugModeFlag)
        { // przesuwanie składu o 100m
            TDynamicObject *d = DynamicObject;
            if (cKey == VkKeyScan('['))
            {
                while (d)
                {
                    d->Move(100.0 * d->DirectionGet());
                    d = d->Next(); // pozostałe też
                }
                d = DynamicObject->Prev();
                while (d)
                {
                    d->Move(100.0 * d->DirectionGet());
                    d = d->Prev(); // w drugą stronę też
                }
            }
            else if (cKey == VkKeyScan(']'))
            {
                while (d)
                {
                    d->Move(-100.0 * d->DirectionGet());
                    d = d->Next(); // pozostałe też
                }
                d = DynamicObject->Prev();
                while (d)
                {
                    d->Move(-100.0 * d->DirectionGet());
                    d = d->Prev(); // w drugą stronę też
                }
            }
        }
        if (cKey == VkKeyScan('-'))
        { // zmniejszenie numeru kanału radiowego
            if (iRadioChannel > 0)
                --iRadioChannel; // 0=wyłączony
        }
        else if (cKey == VkKeyScan('='))
        { // zmniejszenie numeru kanału radiowego
            if (iRadioChannel < 8)
                ++iRadioChannel; // 0=wyłączony
        }
    }
}

void TTrain::OnKeyUp(int cKey)
{ // zwolnienie klawisza
    if (GetAsyncKeyState(VK_SHIFT) < 0)
    { // wciśnięty [Shift]
    }
    else
    {
        if (cKey == Global::Keys[k_StLinOff]) // Winger 110904: wylacznik st. liniowych
        { // zwolnienie klawisza daje powrót przycisku do zwykłego stanu
            if ((mvControlled->TrainType != dt_EZT) && (mvControlled->TrainType != dt_EP05) &&
                (mvControlled->TrainType != dt_ET40))
                ggStLinOffButton.PutValue(0);
        }
    }
};

void TTrain::UpdateMechPosition(double dt)
{ // Ra: mechanik powinien być
    // telepany niezależnie od pozycji
    // pojazdu
    // Ra: trzeba zrobić model bujania głową i wczepić go do pojazdu

    // DynamicObject->vFront=DynamicObject->GetDirection(); //to jest już
    // policzone

    // Ra: tu by się przydało uwzględnić rozkład sił:
    // - na postoju horyzont prosto, kabina skosem
    // - przy szybkiej jeździe kabina prosto, horyzont pochylony

    vector3 pNewMechPosition;
    Math3D::vector3 shake;
    // McZapkie: najpierw policzę pozycję w/m kabiny

    // ABu: rzucamy kabina tylko przy duzym FPS!
    // Mala histereza, zeby bez przerwy nie przelaczalo przy FPS~17
    // Granice mozna ustalic doswiadczalnie. Ja proponuje 14:20
    double const iVel = std::min(DynamicObject->GetVelocity(), 150.0);

    if (!Global::iSlowMotion // musi być pełna prędkość
        && (pMechOffset.y < 4.0)) // Ra 15-01: przy oglądaniu pantografu bujanie przeszkadza
    {
        if( iVel > 0.0 ) {
            // acceleration-driven base shake
            shake += 1.25 * MechSpring.ComputateForces(
                vector3(
                     -mvControlled->AccN * dt * 5.0, // highlight side sway
                      mvControlled->AccV * dt,
                     -mvControlled->AccS * dt * 1.25 ), // accent acceleration/deceleration
                pMechShake );

            if( Random( iVel ) > 25.0 ) {
                // extra shake at increased velocity
                shake += MechSpring.ComputateForces(
                    vector3(
                        ( Random( iVel * 2 ) - iVel ) / ( ( iVel * 2 ) * 4 ) * fMechSpringX,
                        ( Random( iVel * 2 ) - iVel ) / ( ( iVel * 2 ) * 4 ) * fMechSpringY,
                        ( Random( iVel * 2 ) - iVel ) / ( ( iVel * 2 ) * 4 ) * fMechSpringZ ),
                    pMechShake );
//                    * (( 200 - DynamicObject->MyTrack->iQualityFlag ) * 0.0075 ); // scale to 75-150% based on track quality
            }
//            shake *= 1.25;
        }
        vMechVelocity -= (shake + vMechVelocity * 100) * (fMechSpringX + fMechSpringY + fMechSpringZ) / (200);
//        shake *= 0.95 * dt; // shake damping

        // McZapkie:
        pMechShake += vMechVelocity * dt;
        // Ra 2015-01: dotychczasowe rzucanie
        pMechOffset += vMechMovement * dt;
        if ((pMechShake.y > fMechMaxSpring) || (pMechShake.y < -fMechMaxSpring))
            vMechVelocity.y = -vMechVelocity.y;
        // ABu011104: 5*pMechShake.y, zeby ladnie pudlem rzucalo :)
        pNewMechPosition = pMechOffset + vector3(1.5 * pMechShake.x, 2.0 * pMechShake.y, 1.5 * pMechShake.z);
        vMechMovement = 0.5 * vMechMovement;
    }
    else
    { // hamowanie rzucania przy spadku FPS
        pMechShake -= pMechShake * std::min(dt, 1.0); // po tym chyba potrafią zostać jakieś ułamki, które powodują zjazd
        pMechOffset += vMechMovement * dt;
        vMechVelocity.y = 0.5 * vMechVelocity.y;
        pNewMechPosition = pMechOffset + vector3(pMechShake.x, 5 * pMechShake.y, pMechShake.z);
        vMechMovement = 0.5 * vMechMovement;
    }
    // numer kabiny (-1: kabina B)
    if (DynamicObject->Mechanik) // może nie być?
        if (DynamicObject->Mechanik->AIControllFlag) // jeśli prowadzi AI
        { // Ra: przesiadka, jeśli AI zmieniło kabinę (a człon?)...
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
    { // sprawdzaj więzy //Ra: nie tu!
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
                    pNewMechPosition; // położenie względem środka pojazdu w układzie scenerii
    pMechPosition += DynamicObject->GetPosition();
};

bool TTrain::Update( double const Deltatime )
{
    DWORD stat;
    double dt = Deltatime; // Timer::GetDeltaTime();
    if (DynamicObject->mdKabina)
    { // Ra: TODO: odczyty klawiatury/pulpitu nie
        // powinny być uzależnione od istnienia modelu
        // kabiny
        tor = DynamicObject->GetTrack(); // McZapkie-180203
        // McZapkie: predkosc wyswietlana na tachometrze brana jest z obrotow kol
        float maxtacho = 3;
        fTachoVelocity = Min0R(fabs(11.31 * mvControlled->WheelDiameter * mvControlled->nrot),
                               mvControlled->Vmax * 1.05);
        { // skacze osobna zmienna
            float ff = floor(GlobalTime->mr); // skacze co sekunde - pol sekundy
            // pomiar, pol sekundy ustawienie
            if (ff != fTachoTimer) // jesli w tej sekundzie nie zmienial
            {
                if (fTachoVelocity > 1) // jedzie
                    fTachoVelocityJump = fTachoVelocity + (2 - Random(3) + Random(3)) * 0.5;
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
            fTachoCount -= dt * 0.66; // schodz powoli - niektore haslery to ze 4
        // sekundy potrafia stukac

        /* Ra: to by trzeba było przemyśleć, zmienione na szybko problemy robi
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
            //dla EZT wskazówka porusza się niestabilnie
            if (fTachoVelocity>7.0)
    {fTachoVelocity=floor(0.5+fTachoVelocity+random(5)-random(5));
    //*floor(0.2*fTachoVelocity);
             if (fTachoVelocity<0.0) fTachoVelocity=0.0;
            }
           iSekunda=floor(GlobalTime->mr);
          }
        */
        // Ra 2014-09: napięcia i prądy muszą być ustalone najpierw, bo wysyłane są
        // ewentualnie na
        // PoKeys
		if ((mvControlled->EngineType != DieselElectric) && (mvControlled->EngineType != ElectricInductionMotor)) // Ra 2014-09: czy taki rozdzia? ma sens?
			fHVoltage = mvControlled->RunningTraction.TractionVoltage; // Winger czy to nie jest zle?
        // *mvControlled->Mains);
        else
            fHVoltage = mvControlled->Voltage;
        if (ShowNextCurrent)
        { // jeśli pokazywać drugi człon
            if (mvSecond)
            { // o ile jest ten drugi
                fHCurrent[0] = mvSecond->ShowCurrent(0) * 1.05;
                fHCurrent[1] = mvSecond->ShowCurrent(1) * 1.05;
                fHCurrent[2] = mvSecond->ShowCurrent(2) * 1.05;
                fHCurrent[3] = mvSecond->ShowCurrent(3) * 1.05;
            }
            else
                fHCurrent[0] = fHCurrent[1] = fHCurrent[2] = fHCurrent[3] =
                    0.0; // gdy nie ma człona
        }
        else
        { // normalne pokazywanie
            fHCurrent[0] = mvControlled->ShowCurrent(0);
            fHCurrent[1] = mvControlled->ShowCurrent(1);
            fHCurrent[2] = mvControlled->ShowCurrent(2);
            fHCurrent[3] = mvControlled->ShowCurrent(3);
        }

        bool kier = (DynamicObject->DirectionGet() * mvOccupied->ActiveCab > 0);
        TDynamicObject *p = DynamicObject->GetFirstDynamic(mvOccupied->ActiveCab < 0 ? 1 : 0, 4);
        int in = 0;
        fEIMParams[0][6] = 0;
        iCarNo = 0;
        iPowerNo = 0;
        iUnitNo = 1;

        for (int i = 0; i < 8; i++)
        {
            bMains[i] = false;
            fCntVol[i] = 0.0f;
            bPants[i][0] = false;
            bPants[i][1] = false;
            bFuse[i] = false;
            bBatt[i] = false;
            bConv[i] = false;
            bComp[i][0] = false;
            bComp[i][1] = false;
            bHeat[i] = false;
        }
        for (int i = 0; i < 20; i++)
        {
            if (p)
            {
                fPress[i][0] = p->MoverParameters->BrakePress;
                fPress[i][1] = p->MoverParameters->PipePress;
                fPress[i][2] = p->MoverParameters->ScndPipePress;
                bDoors[i][0] = (p->dDoorMoveL > 0.001) || (p->dDoorMoveR > 0.001);
                bDoors[i][1] = (p->dDoorMoveR > 0.001);
                bDoors[i][2] = (p->dDoorMoveL > 0.001);
                iDoorNo[i] = p->iAnimType[ANIM_DOORS];
                iUnits[i] = iUnitNo;
                cCode[i] = p->MoverParameters->TypeName[p->MoverParameters->TypeName.length()];
                asCarName[i] = p->GetName();
				bPants[iUnitNo - 1][0] = (bPants[iUnitNo - 1][0] || p->MoverParameters->PantFrontUp);
                bPants[iUnitNo - 1][1] = (bPants[iUnitNo - 1][1] || p->MoverParameters->PantRearUp);
				bComp[iUnitNo - 1][0] = (bComp[iUnitNo - 1][0] || p->MoverParameters->CompressorAllow);
                if (p->MoverParameters->CompressorSpeed > 0.00001)
                {
					bComp[iUnitNo - 1][1] = (bComp[iUnitNo - 1][1] || p->MoverParameters->CompressorFlag);
                }
                if ((in < 8) && (p->MoverParameters->eimc[eimc_p_Pmax] > 1))
                {
                    fEIMParams[1 + in][0] = p->MoverParameters->eimv[eimv_Fr];
                    fEIMParams[1 + in][1] = Max0R(fEIMParams[1 + in][0], 0);
                    fEIMParams[1 + in][2] = -Min0R(fEIMParams[1 + in][0], 0);
                    fEIMParams[1 + in][3] = p->MoverParameters->eimv[eimv_Fr] /
                                            Max0R(p->MoverParameters->eimv[eimv_Fful], 1);
                    fEIMParams[1 + in][4] = Max0R(fEIMParams[1 + in][3], 0);
                    fEIMParams[1 + in][5] = -Min0R(fEIMParams[1 + in][3], 0);
                    fEIMParams[1 + in][6] = p->MoverParameters->eimv[eimv_If];
                    fEIMParams[1 + in][7] = p->MoverParameters->eimv[eimv_U];
					fEIMParams[1 + in][8] = p->MoverParameters->Itot;//p->MoverParameters->eimv[eimv_Ipoj];
                    fEIMParams[1 + in][9] = p->MoverParameters->Voltage;
                    fEIMParams[0][6] += fEIMParams[1 + in][8];
                    bMains[in] = p->MoverParameters->Mains;
                    fCntVol[in] = p->MoverParameters->BatteryVoltage;
                    bFuse[in] = p->MoverParameters->FuseFlag;
                    bBatt[in] = p->MoverParameters->Battery;
                    bConv[in] = p->MoverParameters->ConverterFlag;
                    bHeat[in] = p->MoverParameters->Heating;
                    in++;
                    iPowerNo = in;
                }
                //                p = p->NextC(4);                                       //prev
                if ((kier ? p->NextC(128) : p->PrevC(128)) != (kier ? p->NextC(4) : p->PrevC(4)))
                    iUnitNo++;
                p = (kier ? p->NextC(4) : p->PrevC(4));
                iCarNo = i + 1;
            }
            else
            {
                fPress[i][0] = 0;
                fPress[i][1] = 0;
                fPress[i][2] = 0;
                bDoors[i][0] = false;
                bDoors[i][1] = false;
                bDoors[i][2] = false;
                iUnits[i] = 0;
                cCode[i] = 0; //'0';
                asCarName[i] = "";
            }
        }

        if (mvControlled == mvOccupied)
            fEIMParams[0][3] = mvControlled->eimv[eimv_Fzad]; // procent zadany
        else
            fEIMParams[0][3] =
                mvControlled->eimv[eimv_Fzad] - mvOccupied->LocalBrakeRatio(); // procent zadany
        fEIMParams[0][4] = Max0R(fEIMParams[0][3], 0);
        fEIMParams[0][5] = -Min0R(fEIMParams[0][3], 0);
        fEIMParams[0][1] = fEIMParams[0][4] * mvControlled->eimv[eimv_Fful];
        fEIMParams[0][2] = fEIMParams[0][5] * mvControlled->eimv[eimv_Fful];
        fEIMParams[0][0] = fEIMParams[0][1] - fEIMParams[0][2];
        fEIMParams[0][7] = 0;
        fEIMParams[0][8] = 0;
        fEIMParams[0][9] = 0;

        for (int i = in; i < 8; i++)
        {
            fEIMParams[1 + i][0] = 0;
            fEIMParams[1 + i][1] = 0;
            fEIMParams[1 + i][2] = 0;
            fEIMParams[1 + i][3] = 0;
            fEIMParams[1 + i][4] = 0;
            fEIMParams[1 + i][5] = 0;
            fEIMParams[1 + i][6] = 0;
            fEIMParams[1 + i][7] = 0;
            fEIMParams[1 + i][8] = 0;
            fEIMParams[1 + i][9] = 0;
        }

        if (Global::iFeedbackMode == 4)
        { // wykonywać tylko gdy wyprowadzone na pulpit
            Console::ValueSet(0,
                              mvOccupied->Compressor); // Ra: sterowanie miernikiem: zbiornik główny
            Console::ValueSet(1,
                              mvOccupied->PipePress); // Ra: sterowanie miernikiem: przewód główny
            Console::ValueSet(2, mvOccupied->BrakePress); // Ra: sterowanie miernikiem: cylinder hamulcowy
            Console::ValueSet(3, fHVoltage); // woltomierz wysokiego napięcia
            Console::ValueSet(4, fHCurrent[2]); // Ra: sterowanie miernikiem: drugi amperomierz
            Console::ValueSet(5,
                fHCurrent[(mvControlled->TrainType & dt_EZT) ? 0 : 1]); // pierwszy amperomierz; dla
            // EZT prąd całkowity
            Console::ValueSet(6, fTachoVelocity); ////Ra: prędkość na pin 43 - wyjście
            /// analogowe (to nie jest PWM);
            /// skakanie zapewnia mechanika
            /// napędu
        }

        // hunter-080812: wyrzucanie szybkiego na elektrykach gdy nie ma napiecia
        // przy dowolnym ustawieniu kierunkowego
        // Ra: to już jest w T_MoverParameters::TractionForce(), ale zależy od
        // kierunku
        if (mvControlled->EngineType == ElectricSeriesMotor)
            if (fabs(mvControlled->RunningTraction.TractionVoltage) <
                0.5 *
                    mvControlled->EnginePowerSource
                        .MaxVoltage) // minimalne napięcie pobierać z FIZ?
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
        // Ra 15-01: to musi stąd wylecieć - zależności nie mogą być w kabinie
        if (mvControlled->ConverterFlag == true)
        {
            fConverterTimer += dt;
            if ((mvControlled->CompressorFlag == true) && (mvControlled->CompressorPower == 1) &&
                ((mvControlled->EngineType == ElectricSeriesMotor) ||
                 (mvControlled->TrainType == dt_EZT)) &&
                (DynamicObject->Controller == Humandriver)) // hunter-110212: poprawka dla EZT
            { // hunter-091012: poprawka (zmiana warunku z CompressorPower /rozne od
                // 0/ na /rowne 1/)
                if (fConverterTimer < fConverterPrzekaznik)
                {
                    mvControlled->ConvOvldFlag = true;
                    if (mvControlled->TrainType != dt_EZT)
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
            rsFadeSound.Stop(); // wyłącz to cholerne cykanie!
        else
            rsFadeSound.Play(1, DSBPLAY_LOOPING, true, DynamicObject->GetPosition());

        // McZapkie! - to wazne - SoundFlag wystawiane jest przez moje moduly
        // gdy zachodza pewne wydarzenia komentowane dzwiekiem.
        // Mysle ze wystarczy sprawdzac a potem zerowac SoundFlag tutaj
        // a nie w DynObject - gdyby cos poszlo zle to po co szarpac dzwiekiem co
        // 10ms.

        if (TestFlag(mvOccupied->SoundFlag, sound_relay)) // przekaznik - gdy
        // bezpiecznik,
        // automatyczny rozruch
        // itp
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
                { // Ra: przenieść do DynObj!
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
                        rsRunningNoise.FM = fm;
                    }
                    mvOccupied->EventFlag = false;
                }

        mvOccupied->SoundFlag = 0;
        /*
            for (int b=0; b<2; b++) //MC: aby zerowac stukanie przekaznikow w
           czlonie silnikowym
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
            {//Ra: ryzykowne jest to, gdyż może się nie uaktualniać prędkość
             //Ra: prędkość się powinna zaokrąglać tam gdzie się liczy
     fTachoVelocity
             if (ggVelocity.SubModel)
     {//ZiomalCl: wskazanie Haslera w kabinie A ze zwloka czasowa oraz odpowiednia
     tolerancja
      //Nalezy sie zastanowic na przyszlosc nad rozroznieniem predkosciomierzy
     (dokladnosc wskazan, zwloka czasowa wskazania, inne funkcje)
              //ZiomalCl: W ezt typu stare EN57 wskazania haslera sa mniej dokladne
     (linka)
              //ggVelocity.UpdateValue(fTachoVelocity>2?fTachoVelocity+0.5-random(mvControlled->TrainType==dt_EZT?5:2)/2:0);
      ggVelocity.UpdateValue(Min0R(fTachoVelocity,mvControlled->Vmax*1.05));
     //ograniczenie maksymalnego wskazania na analogowym
              ggVelocity.Update();
             }
             if (ggVelocityDgt.SubModel)
             {//Ra 2014-07: prędkościomierz cyfrowy
              ggVelocityDgt.UpdateValue(fTachoVelocity);
              ggVelocityDgt.Update();
             }
             if (ggVelocity_B.SubModel)
     {//ZiomalCl: wskazanie Haslera w kabinie B ze zwloka czasowa oraz odpowiednia
     tolerancja
      //Nalezy sie zastanowic na przyszlosc nad rozroznieniem predkosciomierzy
     (dokladnosc wskazan, zwloka czasowa wskazania, inne funkcje)
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
        { // Ra 2014-12: lokomotywy 181/182
            // dostają SlippingWheels po zahamowaniu
            // powyżej 2.85 bara i buczały
            double veldiff = (DynamicObject->GetVelocity() - fTachoVelocity) / mvControlled->Vmax;
            if (veldiff < -0.01) // 1% Vmax rezerwy, żeby 181/182 nie buczały po
            // zahamowaniu, ale to proteza
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

        if ((mvControlled->Mains) || (mvControlled->TrainType == dt_EZT))
        {
            btLampkaNadmSil.Turn(mvControlled->FuseFlagCheck());
        }
        else
        {
            btLampkaNadmSil.TurnOff();
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
                btLampkaStyczn.TurnOff(); // Ra: czy to jest udawanie działania
            // styczników liniowych?
            else if (mvOccupied->BrakePress < 1)
                btLampkaStyczn.TurnOn(); // mozna prowadzic rozruch
            if (((TestFlag(mvControlled->Couplers[1].CouplingFlag, ctrain_controll)) &&
                 (mvControlled->CabNo == 1)) ||
                ((TestFlag(mvControlled->Couplers[0].CouplingFlag, ctrain_controll)) &&
                 (mvControlled->CabNo == -1)))
                btLampkaUkrotnienie.TurnOn();
            else
                btLampkaUkrotnienie.TurnOff();

            //         if
            //         ((TestFlag(mvControlled->BrakeStatus,+b_Rused+b_Ractive)))//Lampka
            //         drugiego stopnia hamowania
            btLampkaHamPosp.Turn((TestFlag(mvOccupied->BrakeStatus, 1))); // lampka drugiego stopnia
            // hamowania  //TODO: youBy
            // wyciągnąć flagę wysokiego
            // stopnia

            // hunter-111211: wylacznik cisnieniowy - Ra: tutaj? w kabinie? //yBARC -
            // omujborzegrzesiuzniszczylesmicalydzien
            //        if (mvControlled->TrainType!=dt_EZT)
            //         if (((mvOccupied->BrakePress > 2) || ( mvOccupied->PipePress <
            //         3.6 )) && ( mvControlled->MainCtrlPos != 0 ))
            //          mvControlled->StLinFlag=true;
            //-------

            // hunter-121211: lampka zanikowo-pradowego wentylatorow:
            btLampkaNadmWent.Turn((mvControlled->RventRot < 5.0) &&
                                  mvControlled->ResistorsFlagCheck());
            //-------

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
            unsigned char scp; // Ra: dopisałem "unsigned"
            // Ra: w SU45 boczniki wchodzą na MainCtrlPos, a nie na MainCtrlActualPos
            // - pokićkał ktoś?
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
            { // wyłączone wszystkie cztery
                btLampkaBocznik1.TurnOff();
                btLampkaBocznik2.TurnOff();
                btLampkaBocznik3.TurnOff();
                btLampkaBocznik4.TurnOff();
            }
            /*
                    { //sprezarka w drugim wozie
                    bool comptemp=false;
                    if (DynamicObject->NextConnected)
                       if
               (TestFlag(mvControlled->Couplers[1].CouplingFlag,ctrain_controll))
                          comptemp=DynamicObject->NextConnected->MoverParameters->CompressorFlag;
                    if ((DynamicObject->PrevConnected) && (!comptemp))
                       if
               (TestFlag(mvControlled->Couplers[0].CouplingFlag,ctrain_controll))
                          comptemp=DynamicObject->PrevConnected->MoverParameters->CompressorFlag;
                    btLampkaSprezarkaB.Turn(comptemp);
            */
        }
        else // wylaczone
        {
            btLampkaWylSzybki.TurnOff();
            btLampkaOpory.TurnOff();
            btLampkaStyczn.TurnOff();
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
            TDynamicObject *tmp; //=mvControlled->mvSecond; //Ra 2014-07: trzeba to
            // jeszcze wyjąć z kabiny...
            // Ra 2014-07: no nie ma potrzeby szukać tego w każdej klatce
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
                    //        //hunter-271211: brak jazdy w drugim czlonie, gdy w
                    //        pierwszym tez nie ma (i odwrotnie) - Ra: tutaj? w kabinie?
                    //        if (tmp->MoverParameters->TrainType!=dt_EZT)
                    //         if (((tmp->MoverParameters->BrakePress > 2) || (
                    //         tmp->MoverParameters->PipePress < 3.6 )) && (
                    //         tmp->MoverParameters->MainCtrlPos != 0 ))
                    //          {
                    //           tmp->MoverParameters->MainCtrlActualPos=0; //inaczej
                    //           StLinFlag nie zmienia sie na false w drugim pojezdzie
                    //           //tmp->MoverParameters->StLinFlag=true;
                    //           mvControlled->StLinFlag=true;
                    //          }
                    //        if (mvControlled->StLinFlag==true)
                    //         tmp->MoverParameters->MainCtrlActualPos=0;
                    //         //tmp->MoverParameters->StLinFlag=true;

                    //-----------------
                    // hunter-271211: sygnalizacja poslizgu w pierwszym pojezdzie, gdy
                    // wystapi w drugim
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
            { // zależnie od typu lokomotywy
            case dt_EZT:
                btLampkaHamienie.Turn((mvControlled->BrakePress >= 0.2) &&
                                      mvControlled->Signalling);
                break;
            case dt_ET41: // odhamowanie drugiego członu
                if (mvSecond) // bo może komuś przyjść do głowy jeżdżenie jednym członem
                    btLampkaHamienie.Turn(mvSecond->BrakePress < 0.4);
                break;
            default:
                btLampkaHamienie.Turn((mvOccupied->BrakePress >= 0.1) ||
                                      mvControlled->DynamicBrakeFlag);
            }
            // KURS90
            btLampkaMaxSila.Turn(abs(mvControlled->Im) >= 350);
            btLampkaPrzekrMaxSila.Turn(abs(mvControlled->Im) >= 450);
            btLampkaRadio.Turn(mvOccupied->Radio);
            btLampkaHamulecReczny.Turn(mvOccupied->ManualBrakePos > 0);
            // NBMX wrzesien 2003 - drzwi oraz sygnał odjazdu
            btLampkaDoorLeft.Turn(mvOccupied->DoorLeftOpened);
            btLampkaDoorRight.Turn(mvOccupied->DoorRightOpened);
            btLampkaNapNastHam.Turn((mvControlled->ActiveDir != 0) &&
                                    (mvOccupied->EpFuse)); // napiecie na nastawniku hamulcowym
            btLampkaForward.Turn(mvControlled->ActiveDir > 0); // jazda do przodu
            btLampkaBackward.Turn(mvControlled->ActiveDir < 0); // jazda do tyłu
            btLampkaED.Turn(mvControlled->DynamicBrakeFlag); // hamulec ED
        }
        else
        { // gdy bateria wyłączona
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
            btLampkaED.TurnOff();
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
        { // Ra: od byte odejmowane boolean i konwertowane
            // potem na double?
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
                    (DynamicObject->Mechanik->AIControllFlag ? false : 
						(Global::iFeedbackMode == 4 || (Global::bMWDmasterEnable && Global::bMWDBreakEnable))) :
                    false) // nie blokujemy AI
            { // Ra: nie najlepsze miejsce, ale na początek gdzieś to dać trzeba
				// Firleju: dlatego kasujemy i zastepujemy funkcją w Console
                if (((mvOccupied->BrakeHandle == FV4a) ||
                                   (mvOccupied->BrakeHandle == FVel6))) // może można usunąć ograniczenie do FV4a i FVel6?
                {
                    double b = Console::AnalogCalibrateGet(0);
					b = Global::CutValueToRange(-2.0, b, mvOccupied->BrakeCtrlPosNo); // przycięcie zmiennej do granic

                    ggBrakeCtrl.UpdateValue(b); // przesów bez zaokrąglenia
                    mvOccupied->BrakeLevelSet(b);
                }
                // else //standardowa prodedura z kranem powiązanym z klawiaturą
                // ggBrakeCtrl.UpdateValue(double(mvOccupied->BrakeCtrlPos));
            }
            // else //standardowa prodedura z kranem powiązanym z klawiaturą
            // ggBrakeCtrl.UpdateValue(double(mvOccupied->BrakeCtrlPos));
            ggBrakeCtrl.UpdateValue(mvOccupied->fBrakeCtrlPos);
            ggBrakeCtrl.Update();
        }
        if (ggLocalBrake.SubModel)
        {
            if (DynamicObject->Mechanik ?
                    (DynamicObject->Mechanik->AIControllFlag ? false : (Global::iFeedbackMode == 4 || Global::bMWDmasterEnable)) :
                    false) // nie blokujemy AI
            { // Ra: nie najlepsze miejsce, ale na początek gdzieś to dać trzeba
			  // Firleju: dlatego kasujemy i zastepujemy funkcją w Console
                if ((mvOccupied->BrakeLocHandle == FD1))
                {
                    double b = Console::AnalogCalibrateGet(1);
					b = Global::CutValueToRange(0.0, b, LocalBrakePosNo); // przycięcie zmiennej do granic
                    ggLocalBrake.UpdateValue(b); // przesów bez zaokrąglenia
                    mvOccupied->LocalBrakePos =
                        int(1.09 * b); // sposób zaokrąglania jest do ustalenia
                }
                else // standardowa prodedura z kranem powiązanym z klawiaturą
                    ggLocalBrake.UpdateValue(double(mvOccupied->LocalBrakePos));
            }
            else // standardowa prodedura z kranem powiązanym z klawiaturą
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
            ggRadioButton.PutValue(mvOccupied->Radio ? 1 : 0);
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

        // końcówki
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
        // główne oświetlenie
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

        // końcówki
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
        if (ggLightsButton.SubModel)
        {
            ggLightsButton.PutValue(mvOccupied->LightsPos - 1);
            ggLightsButton.Update();
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
        // hunter-080812: poprawka na ogrzewanie w elektrykach - usuniete
        // uzaleznienie od przetwornicy
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
                if (dsbBuzzer)
                {
                    dsbBuzzer->GetStatus(&stat);
                    if (!(stat & DSBSTATUS_PLAYING))
                    {
                        dsbBuzzer->Play(0, 0, DSBPLAY_LOOPING);
                        Console::BitsSet(1 << 14); // ustawienie bitu 16 na PoKeys
                    }
                }
            }
            else
            {
                if (dsbBuzzer)
                {
                    dsbBuzzer->GetStatus(&stat);
                    if (stat & DSBSTATUS_PLAYING)
                    {
                        dsbBuzzer->Stop();
                        Console::BitsClear(1 << 14); // ustawienie bitu 16 na PoKeys
                    }
                }
            }
        }
        else // wylaczone
        {
            btLampkaCzuwaka.TurnOff();
            btLampkaSHP.TurnOff();
            if (dsbBuzzer)
            {
                dsbBuzzer->GetStatus(&stat);
                if (stat & DSBSTATUS_PLAYING)
                {
                    dsbBuzzer->Stop();
                    Console::BitsClear(1 << 14); // ustawienie bitu 16 na PoKeys
                }
            }
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
        // hunter-141211: wyl. szybki zalaczony i wylaczony przeniesiony z
        // OnKeyPress()
        if (Console::Pressed(VK_SHIFT) && Console::Pressed(Global::Keys[k_Main]))
        {
            fMainRelayTimer += dt;
            ggMainOnButton.PutValue(1);
            if (mvControlled->Mains != true) // hunter-080812: poprawka
                mvControlled->ConverterSwitch(false);
            if (fMainRelayTimer > mvControlled->InitialCtrlDelay) // wlaczanie WSa z opoznieniem
                if (mvControlled->MainSwitch(true))
                {
                    //        if (mvControlled->MainCtrlPos!=0) //zabezpieczenie, by po
                    //        wrzuceniu pozycji przed wlaczonym
                    //         mvControlled->StLinFlag=true; //WSem nie wrzucilo na ta
                    //         pozycje po jego zalaczeniu //yBARC - co to tutaj robi?!
                    if (mvControlled->EngineType == DieselEngine)
                        dsbDieselIgnition->Play(0, 0, 0);
                }
        }
        else
        {
            if (ggConverterButton.GetValue() !=
                0) // po puszczeniu przycisku od WSa odpalanie potwora
                mvControlled->ConverterSwitch(true);
            // hunter-091012: przeniesione z mover.pas, zeby dzwiek sie nie zapetlal,
            // drugi warunek zeby nie odtwarzalo w nieskonczonosc i przeniesienie
            // zerowania timera
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
                 if (mvControlled->MainCtrlPos!=0) //hunter-131211: takie
         zabezpieczenie
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
        { // czuwak testuje kierunek, ale podobno w
            // EZT nie, więc może być w rozrządczym
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
            if (mvControlled->TrainType != dt_EZT && ggSandButton.SubModel != NULL)
            {
                // dsbPneumaticRelay->SetVolume(-30);
                // dsbPneumaticRelay->Play(0,0,0);
                ggSandButton.PutValue(1);
                mvControlled->SandDose = true;
                // mvControlled->SandDoseOn(true);
            }
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
            if ((mvControlled->TrainType == dt_EZT) && (!TestFlag(mvControlled->EngDmgFlag, 4)))
                mvControlled->ConvOvldFlag = false;
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

        if ((!FreeFlyModeFlag) &&
            (!(DynamicObject->Mechanik ? DynamicObject->Mechanik->AIControllFlag : false)))
        {
            if (Console::Pressed(Global::Keys[k_Releaser])) // yB: odluzniacz caly
            // czas trzymany, warunki
            // powinny byc takie same,
            // jak przy naciskaniu.
            // Wlasciwie stamtad mozna
            // wyrzucic sprawdzanie
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
            // Ra: przecieść to na zwolnienie klawisza
            if (DynamicObject->Mechanik ? !DynamicObject->Mechanik->AIControllFlag :
                                          false) // nie wyłączać, gdy AI
                mvControlled->PantCompFlag = false; // wyłączona, gdy nie trzymamy klawisza
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
                        ggUniversal3Button.PutValue(1); // hunter-131211: z UpdateValue na
                        // PutValue - by zachowywal sie jak
                        // pozostale przelaczniki
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
                        ggUniversal3Button.PutValue(0); // hunter-131211: z UpdateValue na
                        // PutValue - by zachowywal sie jak
                        // pozostale przelaczniki
                        if (btLampkaUniversal3.Active())
                            LampkaUniversal3_st = false;
                    }
                }
            }
        }

        // hunter-091012: to w ogole jest bez sensu i tak namodzone ze nie wiadomo o
        // co chodzi - zakomentowalem i zrobilem po swojemu
        /*
        if ( Console::Pressed(Global::Keys[k_Univ3]) )
        {
          if (ggUniversal3Button.SubModel)


           if (Console::Pressed(VK_CONTROL))
           {//z [Ctrl] zapalamy albo gasimy światełko w kabinie
     //tutaj jest bez sensu, trzeba reagować na wciskanie klawisza!
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
           {//bez [Ctrl] przełączamy cośtem
            if (Console::Pressed(VK_SHIFT))
            {
          ggUniversal3Button.PutValue(1);  //hunter-131211: z UpdateValue na
     PutValue - by zachowywal sie jak pozostale przelaczniki
             if (btLampkaUniversal3.Active())
              LampkaUniversal3_st=true;
            }
            else
            {
          ggUniversal3Button.PutValue(0);  //hunter-131211: z UpdateValue na
     PutValue - by zachowywal sie jak pozostale przelaczniki
             if (btLampkaUniversal3.Active())
              LampkaUniversal3_st=false;
            }
           }
        } */

        // ABu030405 obsluga lampki uniwersalnej:
        if (btLampkaUniversal3.Active()) // w ogóle jest
            if (LampkaUniversal3_st) // załączona
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

        // hunter-091012: przepisanie univ4 i zrobione z uwzglednieniem przelacznika
        // swiatla
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
                        ggCabLightDimButton.PutValue(0); // hunter-131211: z UpdateValue na
                        // PutValue - by zachowywal sie jak
                        // pozostale przelaczniki
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
			(Global::iFeedbackMode != 4) && (!(Global::bMWDmasterEnable && Global::bMWDBreakEnable)))
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

        // Ra: przeklejka z SPKS - płynne poruszanie hamulcem
        // if
        // ((mvOccupied->BrakeHandle==FV4a)&&(Console::Pressed(Global::Keys[k_IncBrakeLevel])))
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
        // if
        // ((mvOccupied->BrakeHandle==FV4a)&&(Console::Pressed(Global::Keys[k_DecBrakeLevel])))
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
                //        mvOccupied->BrakeCtrlPos=
                //        floor(mvOccupied->BrakeCtrlPosR+0.499);
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
                //        mvOccupied->BrakeCtrlPos=
                //        floor(mvOccupied->BrakeCtrlPosR+0.499);
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
            if (DynamicObject->Mechanik) // może nie być?
                if (!DynamicObject->Mechanik->AIControllFlag) // tylko jeśli nie prowadzi AI
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
                    { // Ra: było pod VK_F3
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
                    { // Ra: było pod VK_F3
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

        /*  if ((mvControlled->Mains) &&
        (mvControlled->EngineType==ElectricSeriesMotor))
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
        ggSandButton.Update();
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
        ggBatteryButton.Update();
        //------
        if (ActiveUniversal4)
            ggUniversal4Button.PermIncValue(dt);

        ggUniversal4Button.Update();
        ggMainOffButton.UpdateValue(0);
        ggMainOnButton.UpdateValue(0);
        ggSecurityResetButton.UpdateValue(0);
        ggReleaserButton.UpdateValue(0);
        ggSandButton.UpdateValue(0);
        ggAntiSlipButton.UpdateValue(0);
        ggDepartureSignalButton.UpdateValue(0);
        ggFuseButton.UpdateValue(0);
        ggConverterFuseButton.UpdateValue(0);

        pyScreens.update();
    }
    // wyprowadzenie sygnałów dla haslera na PoKeys (zaznaczanie na taśmie)
    btHaslerBrakes.Turn(DynamicObject->MoverParameters->BrakePress > 0.4); // ciśnienie w cylindrach
    btHaslerCurrent.Turn(DynamicObject->MoverParameters->Im != 0.0); // prąd na silnikach

    m_updated = true;
    return true; //(DynamicObject->Update(dt));
} // koniec update

bool TTrain::CabChange(int iDirection)
{ // McZapkie-090902: zmiana kabiny 1->0->2 i z powrotem
    if (DynamicObject->Mechanik ? DynamicObject->Mechanik->AIControllFlag :
                                  true) // jeśli prowadzi AI albo jest w innym członie
    { // jak AI prowadzi, to nie można mu mieszać
        if (abs(DynamicObject->MoverParameters->ActiveCab + iDirection) > 1)
            return false; // ewentualna zmiana pojazdu
        DynamicObject->MoverParameters->ActiveCab =
            DynamicObject->MoverParameters->ActiveCab + iDirection;
    }
    else
    { // jeśli pojazd prowadzony ręcznie albo wcale (wagon)
        DynamicObject->MoverParameters->CabDeactivisation();
        if (DynamicObject->MoverParameters->ChangeCab(iDirection))
            if (InitializeCab(DynamicObject->MoverParameters->ActiveCab,
                              DynamicObject->asBaseDir + DynamicObject->MoverParameters->TypeName +
                                  ".mmd"))
            { // zmiana kabiny w ramach tego samego pojazdu
                DynamicObject->MoverParameters
                    ->CabActivisation(); // załączenie rozrządu (wirtualne kabiny)
                return true; // udało się zmienić kabinę
            }
        DynamicObject->MoverParameters->CabActivisation(); // aktywizacja
        // poprzedniej, bo
        // jeszcze nie wiadomo,
        // czy jakiś pojazd jest
    }
    return false; // ewentualna zmiana pojazdu
}

// McZapkie-310302
// wczytywanie pliku z danymi multimedialnymi (dzwieki, kontrolki, kabiny)
bool TTrain::LoadMMediaFile(std::string const &asFileName)
{
    double dSDist;
    cParser parser(asFileName, cParser::buffer_FILE);
    // NOTE: yaml-style comments are disabled until conflict in use of # is resolved
    // parser.addCommentStyle( "#", "\n" );
    //Wartości domyślne by nie wysypywało przy wybrakowanych mmd @240816 Stele
    dsbPneumaticSwitch = TSoundsManager::GetFromName("silence1.wav", true);
    dsbBufferClamp = TSoundsManager::GetFromName("en57_bufferclamp.wav", true);
    dsbCouplerDetach = TSoundsManager::GetFromName("couplerdetach.wav", true);
    dsbCouplerStretch = TSoundsManager::GetFromName("en57_couplerstretch.wav", true);
    dsbCouplerAttach = TSoundsManager::GetFromName("couplerattach.wav", true);

    std::string token;
    do
    {
        token = "";
        parser.getTokens();
        parser >> token;
    } while ((token != "") && (token != "internaldata:"));

    if (token == "internaldata:")
    {

        do
        {
            token = "";
            parser.getTokens();
            parser >> token;
            // SEKCJA DZWIEKOW
            if (token == "ctrl:")
            {
                // nastawnik:
                dsbNastawnikJazdy =
                    TSoundsManager::GetFromName(parser.getToken<std::string>().c_str(), true);
            }
            else if (token == "ctrlscnd:")
            {
                // hunter-081211: nastawnik bocznikowania
                dsbNastawnikBocz =
                    TSoundsManager::GetFromName(parser.getToken<std::string>().c_str(), true);
            }
            else if (token == "reverserkey:")
            {
                // hunter-131211: dzwiek kierunkowego
                dsbReverserKey =
                    TSoundsManager::GetFromName(parser.getToken<std::string>().c_str(), true);
            }
            else if (token == "buzzer:")
            {
                // bzyczek shp:
                dsbBuzzer =
                    TSoundsManager::GetFromName(parser.getToken<std::string>().c_str(), true);
            }
            else if (token == "slipalarm:")
            {
                // Bombardier 011010: alarm przy poslizgu:
                dsbSlipAlarm =
                    TSoundsManager::GetFromName(parser.getToken<std::string>().c_str(), true);
            }
            else if (token == "tachoclock:")
            {
                // cykanie rejestratora:
                dsbHasler =
                    TSoundsManager::GetFromName(parser.getToken<std::string>().c_str(), true);
            }
            else if (token == "switch:")
            {
                // przelaczniki:
                dsbSwitch =
                    TSoundsManager::GetFromName(parser.getToken<std::string>().c_str(), true);
            }
            else if (token == "pneumaticswitch:")
            {
                // stycznik EP:
                dsbPneumaticSwitch =
                    TSoundsManager::GetFromName(parser.getToken<std::string>().c_str(), true);
            }
            else if (token == "wejscie_na_bezoporow:")
            {
                // hunter-111211: wydzielenie wejscia na bezoporowa i na drugi uklad do pliku
                dsbWejscie_na_bezoporow =
                    TSoundsManager::GetFromName(parser.getToken<std::string>().c_str(), true);
            }
            else if (token == "wejscie_na_drugi_uklad:")
            {

                dsbWejscie_na_drugi_uklad =
                    TSoundsManager::GetFromName(parser.getToken<std::string>().c_str(), true);
            }
            else if (token == "relay:")
            {
                // styczniki itp:
                dsbRelay =
                    TSoundsManager::GetFromName(parser.getToken<std::string>().c_str(), true);
                if (!dsbWejscie_na_bezoporow)
                { // hunter-111211: domyslne, gdy brak
                    dsbWejscie_na_bezoporow =
                        TSoundsManager::GetFromName("wejscie_na_bezoporow.wav", true);
                }
                if (!dsbWejscie_na_drugi_uklad)
                {
                    dsbWejscie_na_drugi_uklad =
                        TSoundsManager::GetFromName("wescie_na_drugi_uklad.wav", true);
                }
            }
            else if (token == "pneumaticrelay:")
            {
                // wylaczniki pneumatyczne:
                dsbPneumaticRelay =
                    TSoundsManager::GetFromName(parser.getToken<std::string>().c_str(), true);
            }
            else if (token == "couplerattach:")
            {
                // laczenie:
                dsbCouplerAttach =
                    TSoundsManager::GetFromName(parser.getToken<std::string>().c_str(), true);
            }
            else if (token == "couplerstretch:")
            {
                // laczenie:
                dsbCouplerStretch = TSoundsManager::GetFromName(
                    parser.getToken<std::string>().c_str(),
                    true); // McZapkie-090503: PROWIZORKA!!! "en57_couplerstretch.wav"
            }
            else if (token == "couplerdetach:")
            {
                // rozlaczanie:
                dsbCouplerDetach =
                    TSoundsManager::GetFromName(parser.getToken<std::string>().c_str(), true);
            }
            else if (token == "bufferclamp:")
            {
                // laczenie:
                dsbBufferClamp = TSoundsManager::GetFromName(
                    parser.getToken<std::string>().c_str(),
                    true); // McZapkie-090503: PROWIZORKA!!! "en57_bufferclamp.wav"
            }
            else if (token == "ignition:")
            {
                // odpalanie silnika
                dsbDieselIgnition =
                    TSoundsManager::GetFromName(parser.getToken<std::string>().c_str(), true);
            }
            else if (token == "brakesound:")
            {
                // hamowanie zwykle:
                rsBrake.Init(parser.getToken<std::string>(), -1, 0, 0, 0, true, true);
                parser.getTokens(4, false);
                parser >> rsBrake.AM >> rsBrake.AA >> rsBrake.FM >> rsBrake.FA;
                rsBrake.AM /= (1 + mvOccupied->MaxBrakeForce * 1000);
                rsBrake.FM /= (1 + mvOccupied->Vmax);
            }
            else if (token == "slipperysound:")
            {
                // sanie:
                rsSlippery.Init(parser.getToken<std::string>(), -1, 0, 0, 0, true);
                parser.getTokens(2, false);
                parser >> rsSlippery.AM >> rsSlippery.AA;
                rsSlippery.FM = 0.0;
                rsSlippery.FA = 1.0;
                rsSlippery.AM /= (1 + mvOccupied->Vmax);
            }
            else if (token == "airsound:")
            {
                // syk:
                rsHiss.Init(parser.getToken<std::string>(), -1, 0, 0, 0, true);
                parser.getTokens(2, false);
                parser >> rsHiss.AM >> rsHiss.AA;
                rsHiss.FM = 0.0;
                rsHiss.FA = 1.0;
            }
            else if (token == "airsound2:")
            {
                // syk:
                rsHissU.Init(parser.getToken<std::string>(), -1, 0, 0, 0, true);
                parser.getTokens(2, false);
                parser >> rsHissU.AM >> rsHissU.AA;
                rsHissU.FM = 0.0;
                rsHissU.FA = 1.0;
            }
            else if (token == "airsound3:")
            {
                // syk:
                rsHissE.Init(parser.getToken<std::string>(), -1, 0, 0, 0, true);
                parser.getTokens(2, false);
                parser >> rsHissE.AM >> rsHissE.AA;
                rsHissE.FM = 0.0;
                rsHissE.FA = 1.0;
            }
            else if (token == "airsound4:")
            {
                // syk:
                rsHissX.Init(parser.getToken<std::string>(), -1, 0, 0, 0, true);
                parser.getTokens(2, false);
                parser >> rsHissX.AM >> rsHissX.AA;
                rsHissX.FM = 0.0;
                rsHissX.FA = 1.0;
            }
            else if (token == "airsound5:")
            {
                // syk:
                rsHissT.Init(parser.getToken<std::string>(), -1, 0, 0, 0, true);
                parser.getTokens(2, false);
                parser >> rsHissT.AM >> rsHissT.AA;
                rsHissT.FM = 0.0;
                rsHissT.FA = 1.0;
            }
            else if (token == "fadesound:")
            {
                // syk:
                rsFadeSound.Init(parser.getToken<std::string>(), -1, 0, 0, 0, true);
                rsFadeSound.AM = 1.0;
                rsFadeSound.AA = 1.0;
                rsFadeSound.FM = 1.0;
                rsFadeSound.FA = 1.0;
            }
            else if (token == "localbrakesound:")
            {
                // syk:
                rsSBHiss.Init(parser.getToken<std::string>(), -1, 0, 0, 0, true);
                parser.getTokens(2, false);
                parser >> rsSBHiss.AM >> rsSBHiss.AA;
                rsSBHiss.FM = 0.0;
                rsSBHiss.FA = 1.0;
            }
            else if (token == "runningnoise:")
            {
                // szum podczas jazdy:
                rsRunningNoise.Init(parser.getToken<std::string>(), -1, 0, 0, 0, true, true);
                parser.getTokens(4, false);
                parser >> rsRunningNoise.AM >> rsRunningNoise.AA >> rsRunningNoise.FM >>
                    rsRunningNoise.FA;
                rsRunningNoise.AM /= (1 + mvOccupied->Vmax);
                rsRunningNoise.FM /= (1 + mvOccupied->Vmax);
            }
            else if (token == "engageslippery:")
            {
                // tarcie tarcz sprzegla:
                rsEngageSlippery.Init(parser.getToken<std::string>(), -1, 0, 0, 0, true, true);
                parser.getTokens(4, false);
                parser >> rsEngageSlippery.AM >> rsEngageSlippery.AA >> rsEngageSlippery.FM >>
                    rsEngageSlippery.FA;
                rsEngageSlippery.FM /= (1 + mvOccupied->nmax);
            }
            else if (token == "mechspring:")
            {
                // parametry bujania kamery:
                double ks, kd;
                parser.getTokens(2, false);
                parser >> ks >> kd;
                MechSpring.Init(MechSpring.restLen, ks, kd);
                parser.getTokens(6, false);
                parser >> fMechSpringX >> fMechSpringY >> fMechSpringZ >> fMechMaxSpring >>
                    fMechRoll >> fMechPitch;
            }
            else if (token == "pantographup:")
            {
                // podniesienie patyka:
                dsbPantUp =
                    TSoundsManager::GetFromName(parser.getToken<std::string>().c_str(), true);
            }
            else if (token == "pantographdown:")
            {
                // podniesienie patyka:
                dsbPantDown =
                    TSoundsManager::GetFromName(parser.getToken<std::string>().c_str(), true);
            }
            else if (token == "doorclose:")
            {
                // zamkniecie drzwi:
                dsbDoorClose =
                    TSoundsManager::GetFromName(parser.getToken<std::string>().c_str(), true);
            }
            else if (token == "dooropen:")
            {
                // otwarcie drzwi:
                dsbDoorOpen =
                    TSoundsManager::GetFromName(parser.getToken<std::string>().c_str(), true);
            }

        } while (token != "");
    }
    else
    {
        return false;
    } // nie znalazl sekcji internal

    if (!InitializeCab(mvOccupied->ActiveCab, asFileName))
    {
        // zle zainicjowana kabina
        return false;
    }
    else
    {
        if (DynamicObject->Controller == Humandriver)
        {
            // McZapkie-030303: mozliwosc wyswietlania kabiny, w przyszlosci dac opcje w mmd
            DynamicObject->bDisplayCab = true;
        }
        return true;
    }
}

bool TTrain::InitializeCab(int NewCabNo, std::string const &asFileName)
{
    pyScreens.reset(this);
    pyScreens.setLookupPath(DynamicObject->asBaseDir);
    bool parse = false;
    double dSDist;
    int cabindex = 0;
    DynamicObject->mdKabina = NULL; // likwidacja wskaźnika na dotychczasową kabinę
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
        break;
    }
    std::string cabstr("cab" + std::to_string(cabindex) + "definition:");

    std::shared_ptr<cParser> parser = std::make_shared<cParser>(asFileName, cParser::buffer_FILE);
    // NOTE: yaml-style comments are disabled until conflict in use of # is resolved
    // parser.addCommentStyle( "#", "\n" );
    std::string token;
    do
    {
        // szukanie kabiny
        token = "";
        parser->getTokens();
        *parser >> token;

    } while ((token != "") && (token != cabstr));

    if ((cabindex != 1) && (token != cabstr))
    {
        // jeśli nie znaleziony wpis kabiny, próba szukania kabiny 1
        cabstr = "cab1definition:";
        // crude way to start parsing from beginning
        parser = std::make_shared<cParser>(asFileName, cParser::buffer_FILE);
        // NOTE: yaml-style comments are disabled until conflict in use of # is resolved
        // parser.addCommentStyle( "#", "\n" );
        do
        {
            token = "";
            parser->getTokens();
            *parser >> token;
        } while ((token != "") && (token != cabstr));
    }
    if (token == cabstr)
    {
        // jeśli znaleziony wpis kabiny
        Cabine[cabindex].Load(*parser);
        // NOTE: the next part is likely to break if sitpos doesn't follow pos
        parser->getTokens();
        *parser >> token;
        if (token == std::string("driver" + std::to_string(cabindex) + "pos:"))
        {
            // pozycja poczatkowa maszynisty
            parser->getTokens(3, false);
            *parser >> pMechOffset.x >> pMechOffset.y >> pMechOffset.z;
            pMechSittingPosition.x = pMechOffset.x;
            pMechSittingPosition.y = pMechOffset.y;
            pMechSittingPosition.z = pMechOffset.z;
        }
        // ABu: pozycja siedzaca mechanika
        parser->getTokens();
        *parser >> token;
        if (token == std::string("driver" + std::to_string(cabindex) + "sitpos:"))
        {
            // ABu 180404 pozycja siedzaca maszynisty
            parser->getTokens(3, false);
            *parser >> pMechSittingPosition.x >> pMechSittingPosition.y >> pMechSittingPosition.z;
            parse = true;
        }
        // else parse=false;
        do
        {
            // ABu: wstawione warunki, wczesniej tylko to:
            //   str=Parser->GetNextSymbol().LowerCase();
            if (parse == true)
            {

                token = "";
                parser->getTokens();
                *parser >> token;
            }
            else
            {
                parse = true;
            }
            // inicjacja kabiny
            // Ra 2014-08: zmieniamy zasady - zamiast przypisywać submodel do
            // istniejących obiektów animujących
            // będziemy teraz uaktywniać obiekty animujące z tablicy i podawać im
            // submodel oraz wskaźnik na parametr
            if (token == std::string("cab" + std::to_string(cabindex) + "model:"))
            {
                // model kabiny
                parser->getTokens();
                *parser >> token;
                if (token != "none")
                {

                    Global::asCurrentTexturePath =
                        DynamicObject->asBaseDir; // bieżąca sciezka do tekstur to dynamic/...
                    TModel3d *kabina =
                        TModelsManager::GetModel(DynamicObject->asBaseDir + token,
                                                 true); // szukaj kabinę jako oddzielny model
                    Global::asCurrentTexturePath =
                        szTexturePath; // z powrotem defaultowa sciezka do tekstur
                    // if (DynamicObject->mdKabina!=k)
                    if (kabina != nullptr)
                    {
                        DynamicObject->mdKabina = kabina; // nowa kabina
                    }
                    //(mdKabina) może zostać to samo po przejściu do innego członu bez
                    // zmiany kabiny, przy powrocie musi być wiązanie ponowne
                    // else
                    // break; //wyjście z pętli, bo model zostaje bez zmian
                }
                else if (cabindex == 1)
                {
                    // model tylko, gdy nie ma kabiny 1
                    DynamicObject->mdKabina =
                        DynamicObject
                            ->mdModel; // McZapkie-170103: szukaj elementy kabiny w glownym modelu
                }
                ActiveUniversal4 = false;
                clear_cab_controls();
            }
            if (nullptr == DynamicObject->mdKabina)
            {
                // don't bother with other parts until the cab is initialised
                continue;
            }
            else if (true == initialize_gauge(*parser, token, cabindex))
            {
                // matched the token, grab the next one
                continue;
            }
            else if (true == initialize_button(*parser, token, cabindex))
            {
                // matched the token, grab the next one
                continue;
            }
            else if (token == "pyscreen:")
            {
                pyScreens.init(*parser, DynamicObject->mdKabina, DynamicObject->GetName(),
                               NewCabNo);
            }
            // btLampkaUnknown.Init("unknown",mdKabina,false);
        } while (token != "");
    }
    else
    {
        return false;
    }
    pyScreens.start();
    if (DynamicObject->mdKabina)
    {
        DynamicObject->mdKabina->Init(); // obrócenie modelu oraz optymalizacja,
        // również zapisanie binarnego
        return true;
    }
    return (token == "none");
}

void TTrain::MechStop()
{ // likwidacja ruchu kamery w kabinie (po powrocie
    // przez [F4])
    pMechPosition = vector3(0, 0, 0);
    pMechShake = vector3(0, 0, 0);
    vMechMovement = vector3(0, 0, 0);
    vMechVelocity = vector3(0, 0, 0); // tu zostawały jakieś ułamki, powodujące uciekanie kamery
};

vector3 TTrain::MirrorPosition(bool lewe)
{ // zwraca współrzędne widoku kamery z lusterka
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
    return DynamicObject->GetPosition(); // współrzędne środka pojazdu
};

void TTrain::DynamicSet(TDynamicObject *d)
{ // taka proteza: chcę podłączyć
    // kabinę EN57 bezpośrednio z
    // silnikowym, aby nie robić tego
    // przez ukrotnienie
    // drugi silnikowy i tak musi być ukrotniony, podobnie jak kolejna jednostka
    // problem się robi ze światłami, które będą zapalane w silnikowym, ale muszą
    // świecić się w rozrządczych
    // dla EZT światła czołowe będą "zapalane w silnikowym", ale widziane z
    // rozrządczych
    // również wczytywanie MMD powinno dotyczyć aktualnego członu
    // problematyczna może być kwestia wybranej kabiny (w silnikowym...)
    // jeśli silnikowy będzie zapięty odwrotnie (tzn. -1), to i tak powinno
    // jeździć dobrze
    // również hamowanie wykonuje się zaworem w członie, a nie w silnikowym...
    DynamicObject = d; // jedyne miejsce zmiany
    mvOccupied = mvControlled = d ? DynamicObject->MoverParameters : NULL; // albo silnikowy w EZT
    if (!DynamicObject)
        return;
    if (mvControlled->TrainType & dt_EZT) // na razie dotyczy to EZT
        if (DynamicObject->NextConnected ? mvControlled->Couplers[1].AllowedFlag & ctrain_depot :
                                           false)
        { // gdy jest człon od sprzęgu 1, a sprzęg łączony
            // warsztatowo (powiedzmy)
            if ((mvControlled->Power < 1.0) && (mvControlled->Couplers[1].Connected->Power >
                                                1.0)) // my nie mamy mocy, ale ten drugi ma
                mvControlled =
                    DynamicObject->NextConnected->MoverParameters; // będziemy sterować tym z mocą
        }
        else if (DynamicObject->PrevConnected ?
                     mvControlled->Couplers[0].AllowedFlag & ctrain_depot :
                     false)
        { // gdy jest człon od sprzęgu 0, a sprzęg łączony
            // warsztatowo (powiedzmy)
            if ((mvControlled->Power < 1.0) && (mvControlled->Couplers[0].Connected->Power >
                                                1.0)) // my nie mamy mocy, ale ten drugi ma
                mvControlled =
                    DynamicObject->PrevConnected->MoverParameters; // będziemy sterować tym z mocą
        }
    mvSecond = NULL; // gdyby się nic nie znalazło
    if (mvOccupied->Power > 1.0) // dwuczłonowe lub ukrotnienia, żeby nie szukać każdorazowo
        if (mvOccupied->Couplers[1].Connected ?
                mvOccupied->Couplers[1].AllowedFlag & ctrain_controll :
                false)
        { // gdy jest człon od sprzęgu 1, a sprzęg łączony
            // warsztatowo (powiedzmy)
            if (mvOccupied->Couplers[1].Connected->Power > 1.0) // ten drugi ma moc
                mvSecond =
                    (TMoverParameters *)mvOccupied->Couplers[1].Connected; // wskaźnik na drugiego
        }
        else if (mvOccupied->Couplers[0].Connected ?
                     mvOccupied->Couplers[0].AllowedFlag & ctrain_controll :
                     false)
        { // gdy jest człon od sprzęgu 0, a sprzęg łączony
            // warsztatowo (powiedzmy)
            if (mvOccupied->Couplers[0].Connected->Power > 1.0) // ale ten drugi ma moc
                mvSecond =
                    (TMoverParameters *)mvOccupied->Couplers[0].Connected; // wskaźnik na drugiego
        }
};

void TTrain::Silence()
{ // wyciszenie dźwięków przy wychodzeniu
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
        dsbHasler->Stop(); // wyłączenie dźwięków opuszczanej kabiny
    if (dsbBuzzer)
        dsbBuzzer->Stop();
    if (dsbSlipAlarm)
        dsbSlipAlarm->Stop(); // dźwięk alarmu przy poślizgu
    // sConverter.Stop();
    // sSmallCompressor->Stop();
    if (dsbCouplerStretch)
        dsbCouplerStretch->Stop();
    if (dsbEN57_CouplerStretch)
        dsbEN57_CouplerStretch->Stop();
    if (dsbBufferClamp)
        dsbBufferClamp->Stop();
};

void TTrain::SetLights()
{
    TDynamicObject *p = DynamicObject->GetFirstDynamic(mvOccupied->ActiveCab < 0 ? 1 : 0, 4);
    bool kier = (DynamicObject->DirectionGet() * mvOccupied->ActiveCab > 0);
    int xs = (kier ? 0 : 1);
    if (kier ? p->NextC(1) : p->PrevC(1)) // jesli jest nastepny, to tylko przod
    {
        p->RaLightsSet(mvOccupied->Lights[xs][mvOccupied->LightsPos - 1] * (1 - xs),
                       mvOccupied->Lights[1 - xs][mvOccupied->LightsPos - 1] * xs);
        p = (kier ? p->NextC(4) : p->PrevC(4));
        while (p)
        {
            if (kier ? p->NextC(1) : p->PrevC(1))
            {
                p->RaLightsSet(0, 0);
            }
            else
            {
                p->RaLightsSet(mvOccupied->Lights[xs][mvOccupied->LightsPos - 1] * xs,
                               mvOccupied->Lights[1 - xs][mvOccupied->LightsPos - 1] * (1 - xs));
            }
            p = (kier ? p->NextC(4) : p->PrevC(4));
        }
    }
    else // calosc
    {
        p->RaLightsSet(mvOccupied->Lights[xs][mvOccupied->LightsPos - 1],
                       mvOccupied->Lights[1 - xs][mvOccupied->LightsPos - 1]);
    }
};

// clears state of all cabin controls
void TTrain::clear_cab_controls()
{

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
    ggSandButton.Clear();
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
                // ggLVoltage.Output(0); //Ra: sterowanie miernikiem: niskie napięcie
    // ggEnrot1m.Clear();
    // ggEnrot2m.Clear();
    // ggEnrot3m.Clear();
    // ggEngageRatio.Clear();
    ggMainGearStatus.Clear();
    ggIgnitionKey.Clear();
				// Jeśli ustawiamy nową wartość dla PoKeys wolna jest 15
				// Numer 14 jest używany dla buczka SHP w innym miejscu
    btLampkaPoslizg.Clear(6);
    btLampkaStyczn.Clear(5);
    btLampkaNadmPrzetw.Clear((mvControlled->TrainType & (dt_EZT)) ? -1 :
                                                                    7); // EN57 nie ma tej lampki
    btLampkaPrzetw.Clear((mvControlled->TrainType & (dt_EZT)) ? 7 : -1); // za to ma tę
    btLampkaPrzekRozn.Clear();
    btLampkaPrzekRoznPom.Clear();
    btLampkaNadmSil.Clear(4);
    btLampkaUkrotnienie.Clear();
    btLampkaHamPosp.Clear();
    btLampkaWylSzybki.Clear(3);
    btLampkaNadmWent.Clear(9);
    btLampkaNadmSpr.Clear(8);
    btLampkaOpory.Clear(2);
    btLampkaWysRozr.Clear((mvControlled->TrainType & (dt_ET22)) ? -1 :
                                                                  10); // ET22 nie ma tej lampki
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
    ggLightsButton.Clear();
    // hunter-230112
    ggRearLeftLightButton.Clear();
    ggRearRightLightButton.Clear();
    ggRearUpperLightButton.Clear();
    ggRearLeftEndLightButton.Clear();
    ggRearRightEndLightButton.Clear();
                btHaslerBrakes.Clear(12); // ciśnienie w cylindrach do odbijania na haslerze
                btHaslerCurrent.Clear(13); // prąd na silnikach do odbijania na haslerze
}

// initializes a button matching provided label. returns: true if the label was found, false
// otherwise
// NOTE: this is temporary work-around for compiler else-if limit
// TODO: refactor the cabin controls into some sensible structure
bool TTrain::initialize_button(cParser &Parser, std::string const &Label, int const Cabindex)
{

    TButton *bt; // roboczy wskaźnik na obiekt animujący lampkę

    // SEKCJA LAMPEK
    if (Label == "i-maxft:")
    {
        btLampkaMaxSila.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-maxftt:")
    {
        btLampkaPrzekrMaxSila.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-radio:")
    {
        btLampkaRadio.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-manual_brake:")
    {
        btLampkaHamulecReczny.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-door_blocked:")
    {
        btLampkaBlokadaDrzwi.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-slippery:")
    {
        btLampkaPoslizg.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-contactors:")
    {
        btLampkaStyczn.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-conv_ovld:")
    {
        btLampkaNadmPrzetw.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-converter:")
    {
        btLampkaPrzetw.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-diff_relay:")
    {
        btLampkaPrzekRozn.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-diff_relay2:")
    {
        btLampkaPrzekRoznPom.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-motor_ovld:")
    {
        btLampkaNadmSil.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-train_controll:")
    {
        btLampkaUkrotnienie.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-brake_delay_r:")
    {
        btLampkaHamPosp.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-mainbreaker:")
    {
        btLampkaWylSzybki.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-vent_ovld:")
    {
        btLampkaNadmWent.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-comp_ovld:")
    {
        btLampkaNadmSpr.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-resistors:")
    {
        btLampkaOpory.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-no_resistors:")
    {
        btLampkaBezoporowa.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-no_resistors_b:")
    {
        btLampkaBezoporowaB.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-highcurrent:")
    {
        btLampkaWysRozr.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-universal3:")
    {
        btLampkaUniversal3.Load(Parser, DynamicObject->mdKabina, DynamicObject->mdModel);
        LampkaUniversal3_typ = 0;
    }
    else if (Label == "i-universal3_M:")
    {
        btLampkaUniversal3.Load(Parser, DynamicObject->mdKabina, DynamicObject->mdModel);
        LampkaUniversal3_typ = 1;
    }
    else if (Label == "i-universal3_C:")
    {
        btLampkaUniversal3.Load(Parser, DynamicObject->mdKabina, DynamicObject->mdModel);
        LampkaUniversal3_typ = 2;
    }
    else if (Label == "i-vent_trim:")
    {
        btLampkaWentZaluzje.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-trainheating:")
    {
        btLampkaOgrzewanieSkladu.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-security_aware:")
    {
        btLampkaCzuwaka.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-security_cabsignal:")
    {
        btLampkaSHP.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-door_left:")
    {
        btLampkaDoorLeft.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-door_right:")
    {
        btLampkaDoorRight.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-departure_signal:")
    {
        btLampkaDepartureSignal.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-reserve:")
    {
        btLampkaRezerwa.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-scnd:")
    {
        btLampkaBoczniki.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-scnd1:")
    {
        btLampkaBocznik1.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-scnd2:")
    {
        btLampkaBocznik2.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-scnd3:")
    {
        btLampkaBocznik3.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-scnd4:")
    {
        btLampkaBocznik4.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-braking:")
    {
        btLampkaHamienie.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-braking-ezt:")
    {
        btLampkaHamowanie1zes.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-braking-ezt2:")
    {
        btLampkaHamowanie2zes.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-compressor:")
    {
        btLampkaSprezarka.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-compressorb:")
    {
        btLampkaSprezarkaB.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-voltbrake:")
    {
        btLampkaNapNastHam.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-mainbreakerb:")
    {
        btLampkaWylSzybkiB.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-resistorsb:")
    {
        btLampkaOporyB.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-contactorsb:")
    {
        btLampkaStycznB.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-conv_ovldb:")
    {
        btLampkaNadmPrzetwB.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-converterb:")
    {
        btLampkaPrzetwB.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-forward:")
    {
        btLampkaForward.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-backward:")
    {
        btLampkaBackward.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-cablight:")
    { // hunter-171012
        btCabLight.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "i-doors:")
    {
        int i = Parser.getToken<int>() - 1;
        bt = Cabine[Cabindex].Button(-1); // pierwsza wolna lampka
        bt->Load(Parser, DynamicObject->mdKabina);
        bt->AssignBool(bDoors[0] + 3 * i);
    }
    else
    {
        // failed to match the label
        return false;
    }

    return true;
}

// initializes a gauge matching provided label. returns: true if the label was found, false
// otherwise
// NOTE: this is temporary work-around for compiler else-if limit
// TODO: refactor the cabin controls into some sensible structure
bool TTrain::initialize_gauge(cParser &Parser, std::string const &Label, int const Cabindex)
{

    TGauge *gg; // roboczy wskaźnik na obiekt animujący gałkę

    /*	sanity check
            if( !DynamicObject->mdKabina ) {
                    WriteLog( "Cab not initialised!" );
                    return false;
            }
    */
    // SEKCJA REGULATOROW
    if (Label == "mainctrl:")
    {
        // nastawnik
        ggMainCtrl.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "mainctrlact:")
    {
        // zabek pozycji aktualnej
        ggMainCtrlAct.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "scndctrl:")
    {
        // bocznik
        ggScndCtrl.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "dirkey:")
    {
        // klucz kierunku
        ggDirKey.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "brakectrl:")
    {
        // hamulec zasadniczy
        ggBrakeCtrl.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "localbrake:")
    {
        // hamulec pomocniczy
        ggLocalBrake.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "manualbrake:")
    {
        // hamulec reczny
        ggManualBrake.Load(Parser, DynamicObject->mdKabina);
    }
    // sekcja przelacznikow obrotowych
    else if (Label == "brakeprofile_sw:")
    {
        // przelacznik tow/osob/posp
        ggBrakeProfileCtrl.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "brakeprofileg_sw:")
    {
        // przelacznik tow/osob
        ggBrakeProfileG.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "brakeprofiler_sw:")
    {
        // przelacznik osob/posp
        ggBrakeProfileR.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "maxcurrent_sw:")
    {
        // przelacznik rozruchu
        ggMaxCurrentCtrl.Load(Parser, DynamicObject->mdKabina);
    }
    // SEKCJA przyciskow sprezynujacych
    else if (Label == "main_off_bt:")
    {
        // przycisk wylaczajacy (w EU07 wyl szybki czerwony)
        ggMainOffButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "main_on_bt:")
    {
        // przycisk wlaczajacy (w EU07 wyl szybki zielony)
        ggMainOnButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "security_reset_bt:")
    {
        // przycisk zbijajacy SHP/czuwak
        ggSecurityResetButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "releaser_bt:")
    {
        // przycisk odluzniacza
        ggReleaserButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "sand_bt:")
    {
        // przycisk piasecznicy
        ggSandButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "antislip_bt:")
    {
        // przycisk antyposlizgowy
        ggAntiSlipButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "horn_bt:")
    {
        // dzwignia syreny
        ggHornButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "fuse_bt:")
    {
        // bezp. nadmiarowy
        ggFuseButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "converterfuse_bt:")
    {
        // hunter-261211:
        // odblokowanie przekaznika nadm. przetw. i ogrz.
        ggConverterFuseButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "stlinoff_bt:")
    {
        // st. liniowe
        ggStLinOffButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "door_left_sw:")
    {
        // drzwi lewe
        ggDoorLeftButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "door_right_sw:")
    {
        // drzwi prawe
        ggDoorRightButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "departure_signal_bt:")
    {
        // sygnal odjazdu
        ggDepartureSignalButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "upperlight_sw:")
    {
        // swiatlo
        ggUpperLightButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "leftlight_sw:")
    {
        // swiatlo
        ggLeftLightButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "rightlight_sw:")
    {
        // swiatlo
        ggRightLightButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "leftend_sw:")
    {
        // swiatlo
        ggLeftEndLightButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "rightend_sw:")
    {
        // swiatlo
        ggRightEndLightButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "lights_sw:")
    {
        // swiatla wszystkie
        ggLightsButton.Load(Parser, DynamicObject->mdKabina);
    }
    //---------------------
    // hunter-230112: przelaczniki swiatel tylnich
    else if (Label == "rearupperlight_sw:")
    {
        // swiatlo
        ggRearUpperLightButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "rearleftlight_sw:")
    {
        // swiatlo
        ggRearLeftLightButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "rearrightlight_sw:")
    {
        // swiatlo
        ggRearRightLightButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "rearleftend_sw:")
    {
        // swiatlo
        ggRearLeftEndLightButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "rearrightend_sw:")
    {
        // swiatlo
        ggRearRightEndLightButton.Load(Parser, DynamicObject->mdKabina);
    }
    //------------------
    else if (Label == "compressor_sw:")
    {
        // sprezarka
        ggCompressorButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "converter_sw:")
    {
        // przetwornica
        ggConverterButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "converteroff_sw:")
    {
        // przetwornica wyl
        ggConverterOffButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "main_sw:")
    {
        // wyl szybki (ezt)
        ggMainButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "radio_sw:")
    {
        // radio
        ggRadioButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "pantfront_sw:")
    {
        // patyk przedni
        ggPantFrontButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "pantrear_sw:")
    {
        // patyk tylny
        ggPantRearButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "pantfrontoff_sw:")
    {
        // patyk przedni w dol
        ggPantFrontButtonOff.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "pantalloff_sw:")
    {
        // patyk przedni w dol
        ggPantAllDownButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "trainheating_sw:")
    {
        // grzanie skladu
        ggTrainHeatingButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "signalling_sw:")
    {
        // Sygnalizacja hamowania
        ggSignallingButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "door_signalling_sw:")
    {
        // Sygnalizacja blokady drzwi
        ggDoorSignallingButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "nextcurrent_sw:")
    {
        //  prąd drugiego członu
        ggNextCurrentButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "cablight_sw:")
    {
        // hunter-091012: swiatlo w kabinie
        ggCabLightButton.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "cablightdim_sw:")
    {
        // hunter-091012: przyciemnienie swiatla w kabinie
        ggCabLightDimButton.Load(Parser, DynamicObject->mdKabina);
    }
    // ABu 090305: uniwersalne przyciski lub inne rzeczy
    else if (Label == "universal1:")
    {
        ggUniversal1Button.Load(Parser, DynamicObject->mdKabina, DynamicObject->mdModel);
    }
    else if (Label == "universal2:")
    {
        ggUniversal2Button.Load(Parser, DynamicObject->mdKabina, DynamicObject->mdModel);
    }
    else if (Label == "universal3:")
    {
        ggUniversal3Button.Load(Parser, DynamicObject->mdKabina, DynamicObject->mdModel);
    }
    else if (Label == "universal4:")
    {
        ggUniversal4Button.Load(Parser, DynamicObject->mdKabina, DynamicObject->mdModel);
    }
    // SEKCJA WSKAZNIKOW
    else if ((Label == "tachometer:") || (Label == "tachometerb:"))
    {
        // predkosciomierz wskaźnikowy z szarpaniem
        gg = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gg->Load(Parser, DynamicObject->mdKabina);
        gg->AssignFloat(&fTachoVelocityJump);
    }
    else if (Label == "tachometern:")
    {
        // predkosciomierz wskaźnikowy bez szarpania
        gg = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gg->Load(Parser, DynamicObject->mdKabina);
        gg->AssignFloat(&fTachoVelocity);
    }
    else if (Label == "tachometerd:")
    {
        // predkosciomierz cyfrowy
        gg = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gg->Load(Parser, DynamicObject->mdKabina);
        gg->AssignFloat(&fTachoVelocity);
    }
    else if ((Label == "hvcurrent1:") || (Label == "hvcurrent1b:"))
    {
        // 1szy amperomierz
        gg = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gg->Load(Parser, DynamicObject->mdKabina);
        gg->AssignFloat(fHCurrent + 1);
    }
    else if ((Label == "hvcurrent2:") || (Label == "hvcurrent2b:"))
    {
        // 2gi amperomierz
        gg = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gg->Load(Parser, DynamicObject->mdKabina);
        gg->AssignFloat(fHCurrent + 2);
    }
    else if ((Label == "hvcurrent3:") || (Label == "hvcurrent3b:"))
    {
        // 3ci amperomierz
        gg = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałska
        gg->Load(Parser, DynamicObject->mdKabina);
        gg->AssignFloat(fHCurrent + 3);
    }
    else if ((Label == "hvcurrent:") || (Label == "hvcurrentb:"))
    {
        // amperomierz calkowitego pradu
        gg = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gg->Load(Parser, DynamicObject->mdKabina);
        gg->AssignFloat(fHCurrent);
    }
    else if (Label == "eimscreen:")
    {
        // amperomierz calkowitego pradu
        int i, j;
        Parser.getTokens(2, false);
        Parser >> i >> j;
        gg = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gg->Load(Parser, DynamicObject->mdKabina);
        gg->AssignFloat(&fEIMParams[i][j]);
    }
    else if (Label == "brakes:")
    {
        // amperomierz calkowitego pradu
        int i, j;
        Parser.getTokens(2, false);
        Parser >> i >> j;
        gg = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gg->Load(Parser, DynamicObject->mdKabina);
        gg->AssignFloat(&fPress[i - 1][j]);
    }
    else if ((Label == "brakepress:") || (Label == "brakepressb:"))
    {
        // manometr cylindrow hamulcowych
        // Ra 2014-08: przeniesione do TCab
        gg = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gg->Load(Parser, DynamicObject->mdKabina, NULL, 0.1);
        gg->AssignDouble(&mvOccupied->BrakePress);
    }
    else if ((Label == "pipepress:") || (Label == "pipepressb:"))
    {
        // manometr przewodu hamulcowego
        TGauge *gg = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gg->Load(Parser, DynamicObject->mdKabina, NULL, 0.1);
        gg->AssignDouble(&mvOccupied->PipePress);
    }
    else if (Label == "limpipepress:")
    {
        // manometr zbiornika sterujacego zaworu maszynisty
        ggZbS.Load(Parser, DynamicObject->mdKabina, NULL, 0.1);
    }
    else if (Label == "cntrlpress:")
    {
        // manometr zbiornika kontrolnego/rorzďż˝du
        gg = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gg->Load(Parser, DynamicObject->mdKabina, NULL, 0.1);
        gg->AssignDouble(&mvControlled->PantPress);
    }
    else if ((Label == "compressor:") || (Label == "compressorb:"))
    {
        // manometr sprezarki/zbiornika glownego
        gg = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gg->Load(Parser, DynamicObject->mdKabina, NULL, 0.1);
        gg->AssignDouble(&mvOccupied->Compressor);
    }
    // yB - dla drugiej sekcji
    else if (Label == "hvbcurrent1:")
    {
        // 1szy amperomierz
        ggI1B.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "hvbcurrent2:")
    {
        // 2gi amperomierz
        ggI2B.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "hvbcurrent3:")
    {
        // 3ci amperomierz
        ggI3B.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "hvbcurrent:")
    {
        // amperomierz calkowitego pradu
        ggItotalB.Load(Parser, DynamicObject->mdKabina);
    }
    //*************************************************************
    else if (Label == "clock:")
    {
        // zegar analogowy
        if (Parser.getToken<std::string>() == "analog")
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
    else if (Label == "evoltage:")
    {
        // woltomierz napiecia silnikow
        ggEngineVoltage.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "hvoltage:")
    {
        // woltomierz wysokiego napiecia
        gg = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gg->Load(Parser, DynamicObject->mdKabina);
        gg->AssignFloat(&fHVoltage);
    }
    else if (Label == "lvoltage:")
    {
        // woltomierz niskiego napiecia
        ggLVoltage.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "enrot1m:")
    {
        // obrotomierz
        gg = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gg->Load(Parser, DynamicObject->mdKabina);
        gg->AssignFloat(fEngine + 1);
    } // ggEnrot1m.Load(Parser,DynamicObject->mdKabina);
    else if (Label == "enrot2m:")
    {
        // obrotomierz
        gg = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gg->Load(Parser, DynamicObject->mdKabina);
        gg->AssignFloat(fEngine + 2);
    } // ggEnrot2m.Load(Parser,DynamicObject->mdKabina);
    else if (Label == "enrot3m:")
    { // obrotomierz
        gg = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gg->Load(Parser, DynamicObject->mdKabina);
        gg->AssignFloat(fEngine + 3);
    } // ggEnrot3m.Load(Parser,DynamicObject->mdKabina);
    else if (Label == "engageratio:")
    {
        // np. ciśnienie sterownika sprzęgła
        gg = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gg->Load(Parser, DynamicObject->mdKabina);
        gg->AssignDouble(&mvControlled->dizel_engage);
    } // ggEngageRatio.Load(Parser,DynamicObject->mdKabina);
    else if (Label == "maingearstatus:")
    {
        // np. ciśnienie sterownika skrzyni biegów
        ggMainGearStatus.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "ignitionkey:")
    {
        ggIgnitionKey.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "distcounter:")
    {
        // Ra 2014-07: licznik kilometrów
        gg = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gg->Load(Parser, DynamicObject->mdKabina);
        gg->AssignDouble(&mvControlled->DistCounter);
    }
    else
    {
        // failed to match the label
        return false;
    }

    return true;
}
