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

// NOTE: we're currently using universal handlers and static handler map but it may be beneficial to have these implemented on individual class instance basis
// TBD, TODO: consider this approach if we ever want to have customized consist behaviour to received commands, based on the consist/vehicle type or whatever
TTrain::commandhandler_map const TTrain::m_commandhandlers = {

    { user_command::mastercontrollerincrease, &TTrain::OnCommand_mastercontrollerincrease },
    { user_command::mastercontrollerincreasefast, &TTrain::OnCommand_mastercontrollerincreasefast },
    { user_command::mastercontrollerdecrease, &TTrain::OnCommand_mastercontrollerdecrease },
    { user_command::mastercontrollerdecreasefast, &TTrain::OnCommand_mastercontrollerdecreasefast },
    { user_command::secondcontrollerincrease, &TTrain::OnCommand_secondcontrollerincrease },
    { user_command::secondcontrollerincreasefast, &TTrain::OnCommand_secondcontrollerincreasefast },
    { user_command::secondcontrollerdecrease, &TTrain::OnCommand_secondcontrollerdecrease },
    { user_command::secondcontrollerdecreasefast, &TTrain::OnCommand_secondcontrollerdecreasefast },
    { user_command::notchingrelaytoggle, &TTrain::OnCommand_notchingrelaytoggle },
    { user_command::mucurrentindicatorothersourceactivate, &TTrain::OnCommand_mucurrentindicatorothersourceactivate },
    { user_command::independentbrakeincrease, &TTrain::OnCommand_independentbrakeincrease },
    { user_command::independentbrakeincreasefast, &TTrain::OnCommand_independentbrakeincreasefast },
    { user_command::independentbrakedecrease, &TTrain::OnCommand_independentbrakedecrease },
    { user_command::independentbrakedecreasefast, &TTrain::OnCommand_independentbrakedecreasefast },
    { user_command::independentbrakebailoff, &TTrain::OnCommand_independentbrakebailoff },
    { user_command::trainbrakeincrease, &TTrain::OnCommand_trainbrakeincrease },
    { user_command::trainbrakedecrease, &TTrain::OnCommand_trainbrakedecrease },
    { user_command::trainbrakecharging, &TTrain::OnCommand_trainbrakecharging },
    { user_command::trainbrakerelease, &TTrain::OnCommand_trainbrakerelease },
    { user_command::trainbrakefirstservice, &TTrain::OnCommand_trainbrakefirstservice },
    { user_command::trainbrakeservice, &TTrain::OnCommand_trainbrakeservice },
    { user_command::trainbrakefullservice, &TTrain::OnCommand_trainbrakefullservice },
    { user_command::trainbrakeemergency, &TTrain::OnCommand_trainbrakeemergency },
    { user_command::wheelspinbrakeactivate, &TTrain::OnCommand_wheelspinbrakeactivate },
    { user_command::sandboxactivate, &TTrain::OnCommand_sandboxactivate },
    { user_command::epbrakecontroltoggle, &TTrain::OnCommand_epbrakecontroltoggle },
    { user_command::brakeactingspeedincrease, &TTrain::OnCommand_brakeactingspeedincrease },
    { user_command::brakeactingspeeddecrease, &TTrain::OnCommand_brakeactingspeeddecrease },
    { user_command::mubrakingindicatortoggle, &TTrain::OnCommand_mubrakingindicatortoggle },
    { user_command::reverserincrease, &TTrain::OnCommand_reverserincrease },
    { user_command::reverserdecrease, &TTrain::OnCommand_reverserdecrease },
    { user_command::alerteracknowledge, &TTrain::OnCommand_alerteracknowledge },
    { user_command::batterytoggle, &TTrain::OnCommand_batterytoggle },
    { user_command::pantographcompressorvalvetoggle, &TTrain::OnCommand_pantographcompressorvalvetoggle },
    { user_command::pantographcompressoractivate, &TTrain::OnCommand_pantographcompressoractivate },
    { user_command::pantographtogglefront, &TTrain::OnCommand_pantographtogglefront },
    { user_command::pantographtogglerear, &TTrain::OnCommand_pantographtogglerear },
    { user_command::pantographlowerall, &TTrain::OnCommand_pantographlowerall },
    { user_command::linebreakertoggle, &TTrain::OnCommand_linebreakertoggle },
    { user_command::convertertoggle, &TTrain::OnCommand_convertertoggle },
    { user_command::converteroverloadrelayreset, &TTrain::OnCommand_converteroverloadrelayreset },
    { user_command::compressortoggle, &TTrain::OnCommand_compressortoggle },
    { user_command::motorconnectorsopen, &TTrain::OnCommand_motorconnectorsopen },
    { user_command::motordisconnect, &TTrain::OnCommand_motordisconnect },
    { user_command::motoroverloadrelaythresholdtoggle, &TTrain::OnCommand_motoroverloadrelaythresholdtoggle },
    { user_command::motoroverloadrelayreset, &TTrain::OnCommand_motoroverloadrelayreset },
    { user_command::heatingtoggle, &TTrain::OnCommand_heatingtoggle },
    { user_command::headlighttoggleleft, &TTrain::OnCommand_headlighttoggleleft },
    { user_command::headlighttoggleright, &TTrain::OnCommand_headlighttoggleright },
    { user_command::headlighttoggleupper, &TTrain::OnCommand_headlighttoggleupper },
    { user_command::redmarkertoggleleft, &TTrain::OnCommand_redmarkertoggleleft },
    { user_command::redmarkertoggleright, &TTrain::OnCommand_redmarkertoggleright },
    { user_command::headlighttogglerearleft, &TTrain::OnCommand_headlighttogglerearleft },
    { user_command::headlighttogglerearright, &TTrain::OnCommand_headlighttogglerearright },
    { user_command::headlighttogglerearupper, &TTrain::OnCommand_headlighttogglerearupper },
    { user_command::redmarkertogglerearleft, &TTrain::OnCommand_redmarkertogglerearleft },
    { user_command::redmarkertogglerearright, &TTrain::OnCommand_redmarkertogglerearright },
    { user_command::headlightsdimtoggle, &TTrain::OnCommand_headlightsdimtoggle },
    { user_command::interiorlighttoggle, &TTrain::OnCommand_interiorlighttoggle },
    { user_command::interiorlightdimtoggle, &TTrain::OnCommand_interiorlightdimtoggle },
    { user_command::instrumentlighttoggle, &TTrain::OnCommand_instrumentlighttoggle },
    { user_command::doorlocktoggle, &TTrain::OnCommand_doorlocktoggle },
    { user_command::doortoggleleft, &TTrain::OnCommand_doortoggleleft },
    { user_command::doortoggleright, &TTrain::OnCommand_doortoggleright },
    { user_command::departureannounce, &TTrain::OnCommand_departureannounce },
    { user_command::hornlowactivate, &TTrain::OnCommand_hornlowactivate },
    { user_command::hornhighactivate, &TTrain::OnCommand_hornhighactivate },
    { user_command::radiotoggle, &TTrain::OnCommand_radiotoggle }
};

std::vector<std::string> const TTrain::fPress_labels = {

    "ch1:  ", "ch2:  ", "ch3:  ", "ch4:  ", "ch5:  ", "ch6:  ", "ch7:  ", "ch8:  ", "ch9:  ", "ch0:  "
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

    auto const &mover = DynamicObject->MoverParameters;

    PyDict_SetItemString( dict, "cab", PyGetInt( mover->ActiveCab ) );
    // basic systems state data
    PyDict_SetItemString( dict, "battery", PyGetBool( mvControlled->Battery ) );
    PyDict_SetItemString( dict, "linebreaker", PyGetBool( mvControlled->Mains ) );
    PyDict_SetItemString( dict, "converter", PyGetBool( mover->ConverterFlag ) );
    PyDict_SetItemString( dict, "converter_overload", PyGetBool( mover->ConvOvldFlag ) );
    PyDict_SetItemString( dict, "compress", PyGetBool( mover->CompressorFlag ) );
    // reverser
    PyDict_SetItemString( dict, "direction", PyGetInt( mover->ActiveDir ) );
    // throttle
    PyDict_SetItemString( dict, "mainctrl_pos", PyGetInt( mover->MainCtrlPos ) );
    PyDict_SetItemString( dict, "main_ctrl_actual_pos", PyGetInt( mover->MainCtrlActualPos ) );
    PyDict_SetItemString( dict, "scndctrl_pos", PyGetInt( mover->ScndCtrlPos ) );
    PyDict_SetItemString( dict, "scnd_ctrl_actual_pos", PyGetInt( mover->ScndCtrlActualPos ) );
    // brakes
    PyDict_SetItemString( dict, "manual_brake", PyGetBool( mvOccupied->ManualBrakePos > 0 ) );
    bool const bEP = ( mvControlled->LocHandle->GetCP()>0.2 ) || ( fEIMParams[ 0 ][ 2 ]>0.01 );
    PyDict_SetItemString( dict, "dir_brake", PyGetBool( bEP ) );
    bool bPN;
    if( ( typeid( *mvControlled->Hamulec ) == typeid( TLSt ) )
     || ( typeid( *mvControlled->Hamulec ) == typeid( TEStED ) ) ) {

        TBrake* temp_ham = mvControlled->Hamulec.get();
        bPN = ( static_cast<TLSt*>( temp_ham )->GetEDBCP()>0.2 );
    }
    else
        bPN = false;
    PyDict_SetItemString( dict, "indir_brake", PyGetBool( bPN ) );
    // other controls
    PyDict_SetItemString( dict, "ca", PyGetBool( TestFlag( mvOccupied->SecuritySystem.Status, s_aware ) ) );
    PyDict_SetItemString( dict, "shp", PyGetBool( TestFlag( mvOccupied->SecuritySystem.Status, s_active ) ) );
    PyDict_SetItemString( dict, "pantpress", PyGetFloat( mvControlled->PantPress ) );
    PyDict_SetItemString( dict, "universal3", PyGetBool( LampkaUniversal3_st ) );
    // movement data
    PyDict_SetItemString( dict, "velocity", PyGetFloat( mover->Vel ) );
    PyDict_SetItemString( dict, "tractionforce", PyGetFloat( mover->Ft ) );
    PyDict_SetItemString( dict, "slipping_wheels", PyGetBool( mover->SlippingWheels ) );
    // electric current data
    PyDict_SetItemString( dict, "traction_voltage", PyGetFloat( mover->RunningTraction.TractionVoltage ) );
    PyDict_SetItemString( dict, "voltage", PyGetFloat( mover->Voltage ) );
    PyDict_SetItemString( dict, "im", PyGetFloat( mover->Im ) );
    PyDict_SetItemString( dict, "fuse", PyGetBool( mover->FuseFlag ) );
    // induction motor state data
    char* TXTT[ 10 ] = { "fd", "fdt", "fdb", "pd", "pdt", "pdb", "itothv", "1", "2", "3" };
    char* TXTC[ 10 ] = { "fr", "frt", "frb", "pr", "prt", "prb", "im", "vm", "ihv", "uhv" };
    char* TXTP[ 3 ] = { "bc", "bp", "sp" };
    for( int j = 0; j < 10; ++j )
        PyDict_SetItemString( dict, ( std::string( "eimp_t_" ) + std::string( TXTT[ j ] ) ).c_str(), PyGetFloatS( fEIMParams[ 0 ][ j ] ) );
    for( int i = 0; i < 8; ++i ) {
        for( int j = 0; j < 10; ++j )
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
    for( int i = 0; i < 20; ++i ) {
        for( int j = 0; j < 3; ++j )
            PyDict_SetItemString( dict, ( std::string( "eimp_pn" ) + std::to_string( i + 1 ) + std::string( "_" ) + std::string( TXTP[ j ] ) ).c_str(),
            PyGetFloatS( fPress[ i ][ j ] ) );
    }
    // multi-unit state data
    PyDict_SetItemString( dict, "car_no", PyGetInt( iCarNo ) );
    PyDict_SetItemString( dict, "power_no", PyGetInt( iPowerNo ) );
    PyDict_SetItemString( dict, "unit_no", PyGetInt( iUnitNo ) );

    for( int i = 0; i < 20; i++ ) {
        PyDict_SetItemString( dict, ( std::string( "doors_" ) + std::to_string( i + 1 ) ).c_str(), PyGetFloatS( bDoors[ i ][ 0 ] ) );
        PyDict_SetItemString( dict, ( std::string( "doors_r_" ) + std::to_string( i + 1 ) ).c_str(), PyGetFloatS( bDoors[ i ][ 1 ] ) );
        PyDict_SetItemString( dict, ( std::string( "doors_l_" ) + std::to_string( i + 1 ) ).c_str(), PyGetFloatS( bDoors[ i ][ 2 ] ) );
        PyDict_SetItemString( dict, ( std::string( "doors_no_" ) + std::to_string( i + 1 ) ).c_str(), PyGetInt( iDoorNo[ i ] ) );
        PyDict_SetItemString( dict, ( std::string( "code_" ) + std::to_string( i + 1 ) ).c_str(), PyGetString( std::string( std::to_string( iUnits[ i ] ) +
            cCode[ i ] ).c_str() ) );
        PyDict_SetItemString( dict, ( std::string( "car_name" ) + std::to_string( i + 1 ) ).c_str(), PyGetString( asCarName[ i ].c_str() ) );
    }
    // ai state data
    auto const &driver = DynamicObject->Mechanik;

    PyDict_SetItemString( dict, "velocity_desired", PyGetFloat( driver->VelDesired ) );
    PyDict_SetItemString( dict, "velroad", PyGetFloat( driver->VelRoad ) );
    PyDict_SetItemString( dict, "vellimitlast", PyGetFloat( driver->VelLimitLast ) );
    PyDict_SetItemString( dict, "velsignallast", PyGetFloat( driver->VelSignalLast ) );
    PyDict_SetItemString( dict, "velsignalnext", PyGetFloat( driver->VelSignalNext ) );
    PyDict_SetItemString( dict, "velnext", PyGetFloat( driver->VelNext ) );
    PyDict_SetItemString( dict, "actualproximitydist", PyGetFloat( driver->ActualProximityDist ) );
    PyDict_SetItemString( dict, "trainnumber", PyGetString( driver->TrainName().c_str() ) );
    // world state data
    PyDict_SetItemString( dict, "hours", PyGetInt( simulation::Time.data().wHour ) );
    PyDict_SetItemString( dict, "minutes", PyGetInt( simulation::Time.data().wMinute ) );
    PyDict_SetItemString( dict, "seconds", PyGetInt( simulation::Time.second() ) );

    return dict;
}

bool TTrain::is_eztoer() const {

    return
        ( ( mvControlled->TrainType == dt_EZT )
       && ( mvOccupied->BrakeSubsystem == ss_ESt )
       && ( mvControlled->Battery == true )
       && ( mvControlled->EpFuse == true )
       && ( mvControlled->ActiveDir != 0 ) ); // od yB
}

// command handlers
void TTrain::OnCommand_mastercontrollerincrease( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // on press or hold
        if( Train->mvControlled->IncMainCtrl( 1 ) ) {
            // sound feedback
            Train->play_sound( Train->dsbNastawnikJazdy );
        }
    }
}

void TTrain::OnCommand_mastercontrollerincreasefast( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // on press or hold
        if( Train->mvControlled->IncMainCtrl( 2 ) ) {
            // sound feedback
            Train->play_sound( Train->dsbNastawnikJazdy );
        }
    }
}

void TTrain::OnCommand_mastercontrollerdecrease( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // on press or hold
        if( Train->mvControlled->DecMainCtrl( 1 ) ) {
            // sound feedback
            Train->play_sound( Train->dsbNastawnikJazdy );
        }
    }
}

void TTrain::OnCommand_mastercontrollerdecreasefast( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // on press or hold
        if( Train->mvControlled->DecMainCtrl( 2 ) ) {
            // sound feedback
            Train->play_sound( Train->dsbNastawnikJazdy );
        }
    }
}

void TTrain::OnCommand_secondcontrollerincrease( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // on press or hold
        if( Train->mvControlled->ShuntMode ) {
            Train->mvControlled->AnPos += ( Command.time_delta * 0.75f );
            if( Train->mvControlled->AnPos > 1 )
                Train->mvControlled->AnPos = 1;
        }
        else if( Train->mvControlled->IncScndCtrl( 1 ) ) {
            // sound feedback
            Train->play_sound( Train->dsbNastawnikBocz, Train->dsbNastawnikJazdy, DSBVOLUME_MAX, 0 );
        }
    }
}

void TTrain::OnCommand_secondcontrollerincreasefast( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // on press or hold
        if( Train->mvControlled->IncScndCtrl( 2 ) ) {
            // sound feedback
            Train->play_sound( Train->dsbNastawnikBocz, Train->dsbNastawnikJazdy, DSBVOLUME_MAX, 0 );
        }
    }
}

void TTrain::OnCommand_notchingrelaytoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->mvOccupied->AutoRelayFlag ) {
            // turn on
            if( Train->mvOccupied->AutoRelaySwitch( true ) ) {
                // audio feedback
                Train->play_sound( Train->dsbSwitch );
                // visual feedback
                // NOTE: there's no button for notching relay control
                // TBD, TODO: add notching relay control button?
            }
        }
        else {
            //turn off
            if( Train->mvOccupied->AutoRelaySwitch( false ) ) {
                // audio feedback
                Train->play_sound( Train->dsbSwitch );
                // visual feedback
                // NOTE: there's no button for notching relay control
                // TBD, TODO: add notching relay control button?
            }
        }
    }
}

void TTrain::OnCommand_mucurrentindicatorothersourceactivate( TTrain *Train, command_data const &Command ) {

    if( Train->ggNextCurrentButton.SubModel == nullptr ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Current Indicator Source switch is missing, or wasn't defined" );
        }
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // turn on
        Train->ShowNextCurrent = true;
        // audio feedback
        if( Train->ggNextCurrentButton.GetValue() < 0.5 ) {
            Train->play_sound( Train->dsbSwitch );
        }
        // visual feedback
        Train->ggNextCurrentButton.UpdateValue( 1.0 );
    }
    else if( Command.action == GLFW_RELEASE ) {
        //turn off
        Train->ShowNextCurrent = false;
        // audio feedback
        if( Train->ggNextCurrentButton.GetValue() > 0.5 ) {
            Train->play_sound( Train->dsbSwitch );
        }
        // visual feedback
        Train->ggNextCurrentButton.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_secondcontrollerdecrease( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // on press or hold
        if( Train->mvControlled->ShuntMode ) {
            Train->mvControlled->AnPos -= ( Command.time_delta * 0.75f );
            if( Train->mvControlled->AnPos > 1 )
                Train->mvControlled->AnPos = 1;
        }
        else if( Train->mvControlled->DecScndCtrl( 1 ) ) {
            // sound feedback
            Train->play_sound( Train->dsbNastawnikBocz, Train->dsbNastawnikJazdy, DSBVOLUME_MAX, 0 );
        }
    }
}

void TTrain::OnCommand_secondcontrollerdecreasefast( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // on press or hold
        if( Train->mvControlled->DecScndCtrl( 2 ) ) {
            // sound feedback
            Train->play_sound( Train->dsbNastawnikBocz, Train->dsbNastawnikJazdy, DSBVOLUME_MAX, 0 );
        }
    }
}

void TTrain::OnCommand_independentbrakeincrease( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {

        if( Train->mvOccupied->LocalBrake != ManualBrake ) {
            Train->mvOccupied->IncLocalBrakeLevel( 1 );
        }
    }
}

void TTrain::OnCommand_independentbrakeincreasefast( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {

        if( Train->mvOccupied->LocalBrake != ManualBrake ) {
            Train->mvOccupied->IncLocalBrakeLevel( 2 );
        }
    }
}

void TTrain::OnCommand_independentbrakedecrease( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {

        if( ( Train->mvOccupied->LocalBrake != ManualBrake )
            // Ra 1014-06: AI potrafi zahamować pomocniczym mimo jego braku - odhamować jakoś trzeba
            // TODO: sort AI out so it doesn't do things it doesn't have equipment for
         || ( Train->mvOccupied->LocalBrakePos != 0 ) ) {
            Train->mvOccupied->DecLocalBrakeLevel( 1 );
        }
    }
}

void TTrain::OnCommand_independentbrakedecreasefast( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {

        if( ( Train->mvOccupied->LocalBrake != ManualBrake )
            // Ra 1014-06: AI potrafi zahamować pomocniczym mimo jego braku - odhamować jakoś trzeba
            // TODO: sort AI out so it doesn't do things it doesn't have equipment for
         || ( Train->mvOccupied->LocalBrakePos != 0 ) ) {
            Train->mvOccupied->DecLocalBrakeLevel( 2 );
        }
    }
}

void TTrain::OnCommand_independentbrakebailoff( TTrain *Train, command_data const &Command ) {
    // TODO: check if this set of conditions can be simplified.
    // it'd be more flexible to have an attribute indicating whether bail off position is supported
    if( ( Train->mvControlled->TrainType != dt_EZT )
     && ( ( Train->mvControlled->EngineType == ElectricSeriesMotor )
       || ( Train->mvControlled->EngineType == DieselElectric )
       || ( Train->mvControlled->EngineType == ElectricInductionMotor ) )
     && ( Train->mvOccupied->BrakeCtrlPosNo > 0 ) ) {

        if( Command.action != GLFW_RELEASE ) {
            // press or hold
            Train->mvOccupied->BrakeReleaser( 1 );
            // audio feedback
            if( Train->ggReleaserButton.GetValue() < 0.05 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            // visual feedback
            Train->ggReleaserButton.UpdateValue( 1.0 );
        }
        else {
            // release
            Train->mvOccupied->BrakeReleaser( 0 );
            // visual feedback
            Train->ggReleaserButton.UpdateValue( 0.0 );
        }
    }
}

void TTrain::OnCommand_trainbrakeincrease( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {

        if( Train->mvOccupied->BrakeHandle == FV4a ) {
            Train->mvOccupied->BrakeLevelAdd( 0.1 /*15.0 * Command.time_delta*/ );
        }
        else {
            if( Train->mvOccupied->BrakeLevelAdd( Global::fBrakeStep ) ) {
                // nieodpowiedni warunek; true, jeśli można dalej kręcić
                Train->keybrakecount = 0;
                if( ( Train->is_eztoer() ) && ( Train->mvOccupied->BrakeCtrlPos < 3 ) ) {
                    // Ra: uzależnić dźwięk od zmiany stanu EP, nie od klawisza
                    Train->play_sound( Train->dsbPneumaticSwitch );
                }
            }
        }
    }
}

void TTrain::OnCommand_trainbrakedecrease( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // press or hold
        if( Train->mvOccupied->BrakeHandle == FV4a ) {
            Train->mvOccupied->BrakeLevelAdd( -0.1 /*-15.0 * Command.time_delta*/ );
        }
        else {
            // nową wersję dostarczył ZiomalCl ("fixed looped sound in ezt when using NUM_9 key")
            if( ( Train->mvOccupied->BrakeCtrlPos > -1 )
             || ( Train->keybrakecount > 1 ) ) {

                if( ( Train->is_eztoer() )
                 && ( Train->mvControlled->Mains )
                 && ( Train->mvOccupied->BrakeCtrlPos != -1 ) ) {
                    // Ra: uzależnić dźwięk od zmiany stanu EP, nie od klawisza
                    Train->play_sound( Train->dsbPneumaticSwitch );
                }
                Train->mvOccupied->BrakeLevelAdd( -Global::fBrakeStep );
            }
            else
                Train->keybrakecount += 1;
            // koniec wersji dostarczonej przez ZiomalCl
        }
    }
    else {
        // release
        if( ( Train->mvOccupied->BrakeCtrlPos == -1 )
         && ( Train->mvOccupied->BrakeHandle == FVel6 )
         && ( Train->DynamicObject->Controller != AIdriver )
         && ( Global::iFeedbackMode != 4 )
         && ( !( Global::bMWDmasterEnable && Global::bMWDBreakEnable ) ) ) {
            // Odskakiwanie hamulce EP
            Train->mvOccupied->BrakeLevelSet( Train->mvOccupied->BrakeCtrlPos + 1 );
            Train->keybrakecount = 0;
            if( ( Train->mvOccupied->TrainType == dt_EZT )
             && ( Train->mvControlled->Mains )
             && ( Train->mvControlled->ActiveDir != 0 ) ) {
                Train->play_sound( Train->dsbPneumaticSwitch );
            }
        }
    }
}

void TTrain::OnCommand_trainbrakecharging( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // press or hold
        // sound feedback
        if( ( Train->is_eztoer() )
         && ( Train->mvControlled->Mains )
         && ( Train->mvOccupied->BrakeCtrlPos != -1 ) ) {
            Train->play_sound( Train->dsbPneumaticSwitch );
        }

        Train->mvOccupied->BrakeLevelSet( -1 );
    }
    else {
        // release
        if( ( Train->mvOccupied->BrakeCtrlPos == -1 )
         && ( Train->mvOccupied->BrakeHandle == FVel6 )
         && ( Train->DynamicObject->Controller != AIdriver )
         && ( Global::iFeedbackMode != 4 )
         && ( !( Global::bMWDmasterEnable && Global::bMWDBreakEnable ) ) ) {
            // Odskakiwanie hamulce EP
            Train->mvOccupied->BrakeLevelSet( Train->mvOccupied->BrakeCtrlPos + 1 );
            Train->keybrakecount = 0;
            if( ( Train->mvOccupied->TrainType == dt_EZT )
             && ( Train->mvControlled->Mains )
             && ( Train->mvControlled->ActiveDir != 0 ) ) {
                Train->play_sound( Train->dsbPneumaticSwitch );
            }
        }
    }
}

void TTrain::OnCommand_trainbrakerelease( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {

        // sound feedback
        if( ( Train->is_eztoer() )
         && ( ( Train->mvOccupied->BrakeCtrlPos == 1 )
           || ( Train->mvOccupied->BrakeCtrlPos == -1 ) ) ) {
            Train->play_sound( Train->dsbPneumaticSwitch );
        }

        Train->mvOccupied->BrakeLevelSet( 0 );
    }
}

void TTrain::OnCommand_trainbrakefirstservice( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {

        // sound feedback
        if( ( Train->is_eztoer() )
            && ( Train->mvControlled->Mains )
            && ( Train->mvOccupied->BrakeCtrlPos != 1 ) ) {
            Train->play_sound( Train->dsbPneumaticSwitch );
        }

        Train->mvOccupied->BrakeLevelSet( 1 );
    }
}

void TTrain::OnCommand_trainbrakeservice( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {

        // sound feedback
        if( ( Train->is_eztoer() )
         && ( Train->mvControlled->Mains )
         && ( ( Train->mvOccupied->BrakeCtrlPos == 1 )
           || ( Train->mvOccupied->BrakeCtrlPos == -1 ) ) ) {
            Train->play_sound( Train->dsbPneumaticSwitch );
        }

        Train->mvOccupied->BrakeLevelSet(
            Train->mvOccupied->BrakeCtrlPosNo / 2
            + ( Train->mvOccupied->BrakeHandle == FV4a ?
               1 :
               0 ) );
    }
}

void TTrain::OnCommand_trainbrakefullservice( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {

        // sound feedback
        if( ( Train->is_eztoer() )
         && ( Train->mvControlled->Mains )
         && ( ( Train->mvOccupied->BrakeCtrlPos == 1 )
           || ( Train->mvOccupied->BrakeCtrlPos == -1 ) ) ) {
            Train->play_sound( Train->dsbPneumaticSwitch );
        }
        Train->mvOccupied->BrakeLevelSet( Train->mvOccupied->BrakeCtrlPosNo - 1 );
    }
}

void TTrain::OnCommand_trainbrakeemergency( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {

        Train->mvOccupied->BrakeLevelSet( Train->mvOccupied->Handle->GetPos( bh_EB ) );
        if( Train->mvOccupied->BrakeCtrlPosNo <= 0.1 ) {
            // hamulec bezpieczeństwa dla wagonów
            Train->mvOccupied->EmergencyBrakeFlag = true;
        }
    }
}

void TTrain::OnCommand_wheelspinbrakeactivate( TTrain *Train, command_data const &Command ) {

    // TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
    if( Train->ggAntiSlipButton.SubModel == nullptr ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Wheelspin Brake button is missing, or wasn't defined" );
        }
        return;
    }
    if( Train->mvOccupied->BrakeSystem == ElectroPneumatic ) {
        return;
    }

    if( Command.action != GLFW_RELEASE ) {
        // press or hold
        Train->mvControlled->AntiSlippingBrake();
        // audio feedback
        if( Train->ggAntiSlipButton.GetValue() < 0.05 ) {
            Train->play_sound( Train->dsbSwitch );
        }
        // visual feedback
        Train->ggAntiSlipButton.UpdateValue( 1.0 );
    }
    else {
        // release
/*
        // audio feedback
        if( Train->ggAntiSlipButton.GetValue() > 0.5 ) {
            Train->play_sound( Train->dsbSwitch );
        }
*/
        // visual feedback
        Train->ggAntiSlipButton.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_sandboxactivate( TTrain *Train, command_data const &Command ) {

    // TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
    if( Train->ggSandButton.SubModel == nullptr ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Sandbox activation button is missing, or wasn't defined" );
        }
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // press
        Train->mvControlled->Sandbox( true );
        // audio feedback
        Train->play_sound( Train->dsbSwitch );
        // visual feedback
        Train->ggSandButton.UpdateValue( 1.0 );
    }
    else if( Command.action == GLFW_RELEASE) {
        // release
        Train->mvControlled->Sandbox( false );
/*
        // audio feedback
        if( Train->ggAntiSlipButton.GetValue() > 0.5 ) {
        Train->play_sound( Train->dsbSwitch );
        }
*/
        // visual feedback
        Train->ggSandButton.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_epbrakecontroltoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->mvOccupied->EpFuse ) {
            // turn on
            if( Train->mvOccupied->EpFuseSwitch( true ) ) {
                // audio feedback
                Train->play_sound( Train->dsbPneumaticSwitch );
                // visual feedback
                // NOTE: there's no button for ep brake control switch
                // TBD, TODO: add ep brake control switch?
            }
        }
        else {
            //turn off
            if( Train->mvOccupied->EpFuseSwitch( false ) ) {
                // audio feedback
                Train->play_sound( Train->dsbPneumaticSwitch );
                // visual feedback
                // NOTE: there's no button for ep brake control switch
                // TBD, TODO: add ep brake control switch?
            }
        }
    }
}

void TTrain::OnCommand_brakeactingspeedincrease( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        if( ( Train->mvOccupied->BrakeDelayFlag & bdelay_M ) != 0 ) {
            // can't speed it up any more than this
            return;
        }
        auto const fasterbrakesetting = (
            Train->mvOccupied->BrakeDelayFlag < bdelay_R ?
                Train->mvOccupied->BrakeDelayFlag << 1 :
                Train->mvOccupied->BrakeDelayFlag | bdelay_M );
        if( true == Train->mvOccupied->BrakeDelaySwitch( fasterbrakesetting ) ) {
            // audio feedback
            Train->play_sound( Train->dsbSwitch );
/*
            Train->play_sound( Train->dsbPneumaticRelay );
*/
            // visual feedback
            if( Train->ggBrakeProfileCtrl.SubModel != nullptr ) {
                Train->ggBrakeProfileCtrl.UpdateValue(
                    ( ( Train->mvOccupied->BrakeDelayFlag & bdelay_R ) != 0 ?
                        2.0 :
                        Train->mvOccupied->BrakeDelayFlag - 1 ) );
            }
            if( Train->ggBrakeProfileG.SubModel != nullptr ) {
                Train->ggBrakeProfileG.UpdateValue(
                    Train->mvOccupied->BrakeDelayFlag == bdelay_G ?
                        1.0 :
                        0.0 );
            }
            if( Train->ggBrakeProfileR.SubModel != nullptr ) {
                Train->ggBrakeProfileR.UpdateValue(
                    ( Train->mvOccupied->BrakeDelayFlag & bdelay_R ) != 0 ?
                        1.0 :
                        0.0 );
            }
        }
    }
}

void TTrain::OnCommand_brakeactingspeeddecrease( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        if( Train->mvOccupied->BrakeDelayFlag == bdelay_G ) {
            // can't slow it down any more than this
            return;
        }
        auto const slowerbrakesetting = (
            Train->mvOccupied->BrakeDelayFlag < bdelay_M ?
                Train->mvOccupied->BrakeDelayFlag >> 1 :
                Train->mvOccupied->BrakeDelayFlag ^ bdelay_M );
        if( true == Train->mvOccupied->BrakeDelaySwitch( slowerbrakesetting ) ) {
            // audio feedback
            Train->play_sound( Train->dsbSwitch );
/*
            Train->play_sound( Train->dsbPneumaticRelay );
*/
            // visual feedback
            if( Train->ggBrakeProfileCtrl.SubModel != nullptr ) {
                Train->ggBrakeProfileCtrl.UpdateValue(
                    ( ( Train->mvOccupied->BrakeDelayFlag & bdelay_R ) != 0 ?
                        2.0 :
                        Train->mvOccupied->BrakeDelayFlag - 1 ) );
            }
            if( Train->ggBrakeProfileG.SubModel != nullptr ) {
                Train->ggBrakeProfileG.UpdateValue(
                    Train->mvOccupied->BrakeDelayFlag == bdelay_G ?
                        1.0 :
                        0.0 );
            }
            if( Train->ggBrakeProfileR.SubModel != nullptr ) {
                Train->ggBrakeProfileR.UpdateValue(
                    ( Train->mvOccupied->BrakeDelayFlag & bdelay_R ) != 0 ?
                        1.0 :
                        0.0 );
            }
        }
    }
}

void TTrain::OnCommand_mubrakingindicatortoggle( TTrain *Train, command_data const &Command ) {

    if( Train->ggSignallingButton.SubModel == nullptr ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Braking Indicator switch is missing, or wasn't defined" );
        }
        return;
    }
    if( Train->mvControlled->TrainType != dt_EZT ) {
        //
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->mvControlled->Signalling) {
            // turn on
            Train->mvControlled->Signalling = true;
            // audio feedback
            if( Train->ggSignallingButton.GetValue() < 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            // visual feedback
            Train->ggSignallingButton.UpdateValue( 1.0 );
        }
        else {
            //turn off
            Train->mvControlled->Signalling = false;
            // audio feedback
            if( Train->ggSignallingButton.GetValue() > 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            // visual feedback
            Train->ggSignallingButton.UpdateValue( 0.0 );
        }
    }
}

void TTrain::OnCommand_reverserincrease( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

        if( Train->mvOccupied->DirectionForward() ) {
            // sound feedback
            Train->play_sound( Train->dsbReverserKey, Train->dsbSwitch, DSBVOLUME_MAX, 0 );
            // aktualizacja skrajnych pojazdów w składzie
            if( ( Train->mvOccupied->ActiveDir )
             && ( Train->DynamicObject->Mechanik ) ) {

                Train->DynamicObject->Mechanik->CheckVehicles( Change_direction );
            }
        }
    }
}

void TTrain::OnCommand_reverserdecrease( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

        if( Train->mvOccupied->DirectionBackward() ) {
            // sound feedback
            Train->play_sound( Train->dsbReverserKey, Train->dsbSwitch, DSBVOLUME_MAX, 0 );
            // aktualizacja skrajnych pojazdów w składzie
            if( ( Train->mvOccupied->ActiveDir )
             && ( Train->DynamicObject->Mechanik ) ) {

                Train->DynamicObject->Mechanik->CheckVehicles( Change_direction );
            }
        }
    }
}

void TTrain::OnCommand_alerteracknowledge( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // press or hold
        Train->fCzuwakTestTimer += 0.035f;
        if( Train->CAflag == false ) {
            Train->CAflag = true;
            Train->mvOccupied->SecuritySystemReset();
        }
        else {
            if( Train->fCzuwakTestTimer > 1.0 ) {
                SetFlag( Train->mvOccupied->SecuritySystem.Status, s_CAtest );
            }
        }
        // visual feedback
        Train->ggSecurityResetButton.UpdateValue( 1.0 );
        // sound feedback
        if( Train->ggSecurityResetButton.GetValue() < 0.05 ) {
            Train->play_sound( Train->dsbSwitch );
        }
    }
    else {
        // release
        Train->fCzuwakTestTimer = 0.0f;
        if( TestFlag( Train->mvOccupied->SecuritySystem.Status, s_CAtest ) ) {
            SetFlag( Train->mvOccupied->SecuritySystem.Status, -s_CAtest );
            Train->mvOccupied->s_CAtestebrake = false;
            Train->mvOccupied->SecuritySystem.SystemBrakeCATestTimer = 0.0;
        }
        Train->CAflag = false;
        // visual feedback
        Train->ggSecurityResetButton.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_batterytoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->mvOccupied->Battery ) {
            // turn on
            // wyłącznik jest też w SN61, ewentualnie załączać prąd na stałe z poziomu FIZ
            if( Train->mvOccupied->BatterySwitch( true ) ) {
                // bateria potrzebna np. do zapalenia świateł
                if( Train->ggBatteryButton.SubModel ) {
                    Train->ggBatteryButton.UpdateValue( 1 );
                }
                // audio feedback
                Train->play_sound( Train->dsbSwitch );
                // side-effects
                if( Train->mvOccupied->LightsPosNo > 0 ) {
                    Train->SetLights();
                }
                if( TestFlag( Train->mvOccupied->SecuritySystem.SystemType, 2 ) ) {
                    // Ra: znowu w kabinie jest coś, co być nie powinno!
                    SetFlag( Train->mvOccupied->SecuritySystem.Status, s_active );
                    SetFlag( Train->mvOccupied->SecuritySystem.Status, s_SHPalarm );
                }
            }
        }
        else {
            //turn off
            if( Train->mvOccupied->BatterySwitch( false ) ) {
                // ewentualnie zablokować z FIZ, np. w samochodach się nie odłącza akumulatora
                if( Train->ggBatteryButton.SubModel ) {
                    Train->ggBatteryButton.UpdateValue( 0 );
                }
                // audio feedback
                Train->play_sound( Train->dsbSwitch );
                // side-effects
                if( false == Train->mvControlled->ConverterFlag ) {
                    // if there's no (low voltage) power source left, drop pantographs
                    Train->mvControlled->PantFront( false );
                    Train->mvControlled->PantRear( false );
                }
            }
        }
    }
}

void TTrain::OnCommand_pantographtogglefront( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->mvControlled->PantFrontUp ) {
            // turn on...
            Train->mvControlled->PantFrontSP = false;
            if( Train->mvControlled->PantFront( true ) ) {
                if( Train->mvControlled->PantFrontStart != 1 ) {
                    // sound feedback
                    Train->play_sound( Train->dsbSwitch );
                    // visual feedback
                    if( Train->ggPantFrontButton.SubModel ) {
                        Train->ggPantFrontButton.UpdateValue( 1.0 );
                    }
                    if( Train->ggPantFrontButtonOff.SubModel != nullptr ) {
                        // pantograph control can have two-button setup
                        Train->ggPantFrontButtonOff.UpdateValue( 0.0 );
                    }
                }
            }
        }
        else {
            // ...or turn off
            if( ( Train->mvOccupied->PantSwitchType == "impulse" )
             && ( Train->ggPantFrontButtonOff.SubModel == nullptr ) ) {
                // with impulse buttons we expect a dedicated switch to lower the pantograph, and if the cabin lacks it
                // then another control has to be used (like pantographlowerall)
                // TODO: we should have a way to define presense of cab controls without having to bind these to 3d submodels
                return;
            }

            Train->mvControlled->PantFrontSP = false;
            if( false == Train->mvControlled->PantFront( false ) ) {
                if( Train->mvControlled->PantFrontStart != 0 ) {
                    // sound feedback
                    Train->play_sound( Train->dsbSwitch );
                    // visual feedback
                    Train->ggPantFrontButton.UpdateValue( 0.0 );
                    // pantograph control can have two-button setup
                    Train->ggPantFrontButtonOff.UpdateValue( 1.0 );
                }
            }
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // impulse switches return automatically to neutral position
        if( Train->mvOccupied->PantSwitchType == "impulse" ) {
            if( Train->ggPantFrontButton.GetValue() > 0.35 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            if( Train->ggPantFrontButton.SubModel ) {
                Train->ggPantFrontButton.UpdateValue( 0.0 );
            }
            // also the switch off button, in cabs which have it
            if( Train->ggPantFrontButtonOff.GetValue() > 0.35 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            if( Train->ggPantFrontButtonOff.SubModel ) {
                Train->ggPantFrontButtonOff.UpdateValue( 0.0 );
            }
        }
    }
}

void TTrain::OnCommand_pantographtogglerear( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->mvControlled->PantRearUp ) {
            // turn on...
            Train->mvControlled->PantRearSP = false;
            if( Train->mvControlled->PantRear( true ) ) {
                if( Train->mvControlled->PantRearStart != 1 ) {
                    // sound feedback
                    Train->play_sound( Train->dsbSwitch );
                    // visual feedback
                    if( Train->ggPantRearButton.SubModel ) {
                        Train->ggPantRearButton.UpdateValue( 1.0 );
                    }
                    if( Train->ggPantRearButtonOff.SubModel != nullptr ) {
                        // pantograph control can have two-button setup
                        Train->ggPantRearButtonOff.UpdateValue( 0.0 );
                    }
                }
            }
        }
        else {
            // ...or turn off
            if( ( Train->mvOccupied->PantSwitchType == "impulse" )
             && ( Train->ggPantRearButtonOff.SubModel == nullptr ) ) {
                // with impulse buttons we expect a dedicated switch to lower the pantograph, and if the cabin lacks it
                // then another control has to be used (like pantographlowerall)
                // TODO: we should have a way to define presense of cab controls without having to bind these to 3d submodels
                return;
            }

            Train->mvControlled->PantRearSP = false;
            if( false == Train->mvControlled->PantRear( false ) ) {
                if( Train->mvControlled->PantRearStart != 0 ) {
                    // sound feedback
                    Train->play_sound( Train->dsbSwitch );
                    // visual feedback
                    Train->ggPantRearButton.UpdateValue( 0.0 );
                    // pantograph control can have two-button setup
                    Train->ggPantRearButtonOff.UpdateValue( 1.0 );
                }
            }
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // impulse switches return automatically to neutral position
        if( Train->mvOccupied->PantSwitchType == "impulse" ) {
            if( Train->ggPantRearButton.GetValue() > 0.35 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            if( Train->ggPantRearButton.SubModel ) {
                Train->ggPantRearButton.UpdateValue( 0.0 );
            }
            // also the switch off button, in cabs which have it
            if( Train->ggPantRearButtonOff.GetValue() > 0.35 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            if( Train->ggPantRearButtonOff.SubModel ) {
                Train->ggPantRearButtonOff.UpdateValue( 0.0 );
            }
        }
    }
}

void TTrain::OnCommand_pantographcompressorvalvetoggle( TTrain *Train, command_data const &Command ) {

    if( ( Train->mvControlled->TrainType == dt_EZT ?
        ( Train->mvControlled != Train->mvOccupied ) :
        ( Train->mvOccupied->ActiveCab != 0 ) ) ) {
        // tylko w maszynowym
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only react to press
        if( Train->mvControlled->bPantKurek3 == false ) {
            // connect pantographs with primary tank
            Train->mvControlled->bPantKurek3 = true;
            // audio feedback
            Train->play_sound( Train->dsbSwitch );
        }
        else {
            // connect pantograps with pantograph compressor
            Train->mvControlled->bPantKurek3 = false;
            // audio feedback
            Train->play_sound( Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_pantographcompressoractivate( TTrain *Train, command_data const &Command ) {

    if( ( Train->mvControlled->TrainType == dt_EZT ?
            ( Train->mvControlled != Train->mvOccupied ) :
            ( Train->mvOccupied->ActiveCab != 0 ) ) ) {
        // tylko w maszynowym
        return;
    }

    if( ( Train->mvControlled->PantPress > 4.8 )
     || ( false == Train->mvControlled->Battery ) ) {
        // needs live power source and low enough pressure to work
        return;
    }

    if( Command.action != GLFW_RELEASE ) {
        // press or hold to activate
        Train->mvControlled->PantCompFlag = true;
        // audio feedback
        if( Command.action == GLFW_PRESS ) {
            Train->play_sound( Train->dsbSwitch );
        }
    }
    else {
        // release to disable
        Train->mvControlled->PantCompFlag = false;
    }
}

void TTrain::OnCommand_pantographlowerall( TTrain *Train, command_data const &Command ) {

    if( Train->ggPantAllDownButton.SubModel == nullptr ) {
        // TODO: expand definition of cab controls so we can know if the control is present without testing for presence of 3d switch
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Lower All Pantographs switch is missing, or wasn't defined" );
        }
        return;
    }
    if( Command.action == GLFW_PRESS ) {
        // press the button
        // since we're just lowering all potential pantographs we don't need to test for state and effect
        // front...
        Train->mvControlled->PantFrontSP = false;
        Train->mvControlled->PantFront( false );
        // ...and rear
        Train->mvControlled->PantRearSP = false;
        Train->mvControlled->PantRear( false );
        // sound feedback
        // TODO: separate sound effect for pneumatic buttons
        if( Train->ggPantAllDownButton.GetValue() < 0.35 ) {
            Train->play_sound( Train->dsbSwitch );
        }
        // visual feedback
        Train->ggPantAllDownButton.UpdateValue( 1.0 );
    }
    else if( Command.action == GLFW_RELEASE ) {
        // release the button
        // sound feedback
/*
        // NOTE: release sound disabled as this is typically pneumatic button
        // TODO: separate sound effect for pneumatic buttons
        if( Train->ggPantAllDownButton.GetValue() > 0.65 ) {
            Train->play_sound( Train->dsbSwitch );
        }
*/
        // visual feedback
        Train->ggPantAllDownButton.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_linebreakertoggle( TTrain *Train, command_data const &Command ) {

    if( ( Command.action == GLFW_PRESS )
     && ( Train->m_linebreakerstate == 1 )
     && ( false == Train->mvControlled->Mains )
     && ( Train->ggMainButton.GetValue() < 0.05 ) ) {
        // crude way to catch cases where the main was knocked out and the user is trying to restart it
        // because the state of the line breaker isn't changed to match, we need to do it here manually
        Train->m_linebreakerstate = 0;
    }
    // NOTE: we don't have switch type definition for the line breaker switch
    // so for the time being we have hard coded "impulse" switches for all EMUs
    // TODO: have proper switch type config for all switches, and put it in the cab switch descriptions, not in the .fiz
    if( ( Train->m_linebreakerstate == 1 )
     && ( Train->mvControlled->TrainType == dt_EZT ) ) {
        // a single impulse switch can't open the circuit, only close it
        return;
    }

    if( Command.action != GLFW_RELEASE ) {
        // press or hold...
        if( Train->m_linebreakerstate == 0 ) {
            // ...to close the circuit
            if( Command.action == GLFW_PRESS ) {
                // fresh press, start fresh closing delay calculation
                Train->fMainRelayTimer = 0.0f;
            }
            if( Train->ggMainOnButton.SubModel != nullptr ) {
                // two separate switches to close and break the circuit
                // audio feedback
                if( Command.action == GLFW_PRESS ) {
                    Train->play_sound( Train->dsbSwitch );
                }
                // visual feedback
                Train->ggMainOnButton.UpdateValue( 1.0 );
            }
            else if( Train->ggMainButton.SubModel != nullptr ) {
                // single two-state switch
                // audio feedback
                if( Command.action == GLFW_PRESS ) {
                    Train->play_sound( Train->dsbSwitch );
                }
                // visual feedback
                Train->ggMainButton.UpdateValue( 1.0 );
            }
            // keep track of period the button is held down, to determine when/if circuit closes
            if( ( false == ( ( Train->mvControlled->EngineType == ElectricSeriesMotor )
                          || ( Train->mvControlled->EngineType == ElectricInductionMotor ) ) )
             || ( Train->fHVoltage > 0.5 * Train->mvControlled->EnginePowerSource.MaxVoltage ) ) {
                // prevent the switch from working if there's no power
                // TODO: consider whether it makes sense for diesel engines and such
                Train->fMainRelayTimer += 0.33f; // Command.time_delta * 5.0;
            }
/*
            if( Train->mvControlled->Mains != true ) {
                // hunter-080812: poprawka
                Train->mvControlled->ConverterSwitch( false );
                Train->mvControlled->CompressorSwitch( false );
            }
*/
            if( Train->fMainRelayTimer > Train->mvControlled->InitialCtrlDelay ) {
                // wlaczanie WSa z opoznieniem
                Train->m_linebreakerstate = 2;
                // for diesels, we complete the engine start here
                // TODO: consider arranging a better way to start the diesel engines
                if( Train->mvControlled->EngineType == DieselEngine ) {
                    if( Train->mvControlled->MainSwitch( true ) ) {
                        // sound feedback, engine start for diesel vehicle
                        Train->play_sound( Train->dsbDieselIgnition );
                        // side-effects
                        Train->mvControlled->ConverterSwitch( Train->ggConverterButton.GetValue() > 0.5 );
                        Train->mvControlled->CompressorSwitch( Train->ggCompressorButton.GetValue() > 0.5 );
                    }
                }
            }
        }
        else if( Train->m_linebreakerstate == 1 ) {
            // ...to open the circuit
            if( true == Train->mvControlled->MainSwitch( false ) ) {

                Train->m_linebreakerstate = -1;

                if( Train->ggMainOffButton.SubModel != nullptr ) {
                    // two separate switches to close and break the circuit
                    // audio feedback
                    if( Command.action == GLFW_PRESS ) {
                        Train->play_sound( Train->dsbSwitch );
                    }
                    // visual feedback
                    Train->ggMainOffButton.UpdateValue( 1.0 );
                }
                else if( Train->ggMainButton.SubModel != nullptr ) {
                    // single two-state switch
/*
                    // NOTE: we don't have switch type definition for the line breaker switch
                    // so for the time being we have hard coded "impulse" switches for all EMUs
                    // TODO: have proper switch type config for all switches, and put it in the cab switch descriptions, not in the .fiz
                    if( Train->mvControlled->TrainType == dt_EZT ) {
                        // audio feedback
                        if( Command.action == GLFW_PRESS ) {
                            Train->play_sound( Train->dsbSwitch );
                        }
                        // visual feedback
                        Train->ggMainButton.UpdateValue( 1.0 );
                    }
                    else
*/
                    {
                        // audio feedback
                        if( Command.action == GLFW_PRESS ) {
                            Train->play_sound( Train->dsbSwitch );
                        }
                        // visual feedback
                        Train->ggMainButton.UpdateValue( 0.0 );
                    }
                }
            }
            // play sound immediately when the switch is hit, not after release
            if( Train->fMainRelayTimer > 0.0f ) {
                Train->play_sound( Train->dsbRelay );
                Train->fMainRelayTimer = 0.0f;
            }
        }
    }
    else {
        // release...
        if( Train->m_linebreakerstate <= 0 ) {
            // ...after opening circuit, or holding for too short time to close it
            // hunter-091012: przeniesione z mover.pas, zeby dzwiek sie nie zapetlal,
            if( Train->fMainRelayTimer > 0.0f ) {
                Train->play_sound( Train->dsbRelay );
                Train->fMainRelayTimer = 0.0f;
            }
            // we don't exactly know which of the two buttons was used, so reset both
            // for setup with two separate swiches
            if( Train->ggMainOnButton.SubModel != nullptr ) {
                Train->ggMainOnButton.UpdateValue( 0.0 );
                // audio feedback
                if( Train->ggMainOnButton.GetValue() > 0.5 ) {
                    Train->play_sound( Train->dsbSwitch );
                }
            }
            if( Train->ggMainOffButton.SubModel != nullptr ) {
                Train->ggMainOffButton.UpdateValue( 0.0 );
            }
            // and the two-state switch too, for good measure
            if( Train->ggMainButton.SubModel != nullptr ) {
                // audio feedback
                if( Train->ggMainButton.GetValue() > 0.5 ) {
                    Train->play_sound( Train->dsbSwitch );
                }
                // visual feedback
                Train->ggMainButton.UpdateValue( 0.0 );
            }
            // finalize the state of the line breaker
            Train->m_linebreakerstate = 0;
        }
        else {
            // ...after closing the circuit
            // we don't need to start the diesel twice, but the other types still need to be launched
            if( Train->mvControlled->EngineType != DieselEngine ) {
                if( Train->mvControlled->MainSwitch( true ) ) {
                    // side-effects
                    Train->mvControlled->ConverterSwitch( Train->ggConverterButton.GetValue() > 0.5 );
                    Train->mvControlled->CompressorSwitch( Train->ggCompressorButton.GetValue() > 0.5 );
                }
            }
            // audio feedback
            if( Train->ggMainOnButton.GetValue() > 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            // visual feedback
            if( Train->ggMainOnButton.SubModel != nullptr ) {
                // setup with two separate switches
                Train->ggMainOnButton.UpdateValue( 0.0 );
            }
            // NOTE: we don't have switch type definition for the line breaker switch
            // so for the time being we have hard coded "impulse" switches for all EMUs
            // TODO: have proper switch type config for all switches, and put it in the cab switch descriptions, not in the .fiz
            if( Train->mvControlled->TrainType == dt_EZT ) {
                if( Train->ggMainButton.SubModel != nullptr ) {
                    // audio feedback
                    if( Train->ggMainButton.GetValue() > 0.5 ) {
                        Train->play_sound( Train->dsbSwitch );
                    }
                    // visual feedback
                    Train->ggMainButton.UpdateValue( 0.0 );
                }
            }
            // finalize the state of the line breaker
            Train->m_linebreakerstate = 1;
        }
        // on button release reset the closing timer, just in case something elsewhere tries to read it
        Train->fMainRelayTimer = 0.0f;
    }
}

void TTrain::OnCommand_convertertoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( false == Train->mvControlled->ConverterAllow )
         && ( Train->ggConverterButton.GetValue() < 0.5 ) ) {
            // turn on
            // sound feedback
            if( Train->ggConverterButton.GetValue() < 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            // visual feedback
            Train->ggConverterButton.UpdateValue( 1.0 );
/*
            if( ( Train->mvControlled->EnginePowerSource.SourceType != CurrentCollector )
             || ( Train->mvControlled->PantRearVolt != 0.0 )
             || ( Train->mvControlled->PantFrontVolt != 0.0 ) ) {
*/
                // impulse type switch has no effect if there's no power
                // NOTE: this is most likely setup wrong, but the whole thing is smoke and mirrors anyway
                if( ( Train->mvOccupied->ConvSwitchType != "impulse" )
                 || ( Train->mvControlled->Mains ) ) {
                    // won't start if the line breaker button is still held
                    if( true == Train->mvControlled->ConverterSwitch( true ) ) {
                        // side effects
                        // control the compressor, if it's paired with the converter
                        if( Train->mvControlled->CompressorPower == 2 ) {
                            // hunter-091012: tak jest poprawnie
                            Train->mvControlled->CompressorSwitch( true );
                        }
                    }
                }
/*
            }
*/
        }
        else {
            //turn off
            // sound feedback
            if( Train->mvOccupied->ConvSwitchType == "impulse" ) {
                if( Train->ggConverterOffButton.GetValue() < 0.5 ) {
                    Train->play_sound( Train->dsbSwitch );
                }
            }
            else {
                if( Train->ggConverterButton.GetValue() > 0.5 ) {
                    Train->play_sound( Train->dsbSwitch );
                }
            }
            // visual feedback
            Train->ggConverterButton.UpdateValue( 0.0 );
            if( Train->ggConverterOffButton.SubModel != nullptr ) {
                Train->ggConverterOffButton.UpdateValue( 1.0 );
            }
            if( true == Train->mvControlled->ConverterSwitch( false ) ) {
                // side effects
                // control the compressor, if it's paired with the converter
                if( Train->mvControlled->CompressorPower == 2 ) {
                    // hunter-091012: tak jest poprawnie
                    Train->mvControlled->CompressorSwitch( false );
                }
                if( ( Train->mvControlled->TrainType == dt_EZT )
                 && ( !TestFlag( Train->mvControlled->EngDmgFlag, 4 ) ) ) {
                    Train->mvControlled->ConvOvldFlag = false;
                }
                // if there's no (low voltage) power source left, drop pantographs
                if( false == Train->mvControlled->Battery ) {
                    Train->mvControlled->PantFront( false );
                    Train->mvControlled->PantRear( false );
                }
            }
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // on button release...
        if( Train->mvOccupied->ConvSwitchType == "impulse" ) {
            // ...return switches to start position if applicable
            if( ( Train->ggConverterButton.GetValue() > 0.0 )
             || ( Train->ggConverterOffButton.GetValue() > 0.0 ) ) {
                // sound feedback
                Train->play_sound( Train->dsbSwitch );
            }
            Train->ggConverterButton.UpdateValue( 0.0 );
            Train->ggConverterOffButton.UpdateValue( 0.0 );
        }
    }
}

void TTrain::OnCommand_converteroverloadrelayreset( TTrain *Train, command_data const &Command ) {

    if( Train->ggConverterFuseButton.SubModel == nullptr ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Converter Overload Relay Reset button is missing, or wasn't defined" );
        }
//        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // press
        if( ( Train->mvControlled->Mains == false )
         && ( Train->ggConverterButton.GetValue() < 0.05 )
         && ( Train->mvControlled->TrainType != dt_EZT ) ) {
            Train->mvControlled->ConvOvldFlag = false;
        }
        // sound feedback
        if( Train->ggConverterFuseButton.GetValue() < 0.5 ) {
            Train->play_sound( Train->dsbSwitch );
        }
        // visual feedback
        Train->ggConverterFuseButton.UpdateValue( 1.0 );
    }
    else if( Command.action == GLFW_RELEASE ) {
        // release
        // sound feedback
        if( Train->ggConverterFuseButton.GetValue() > 0.5 ) {
            Train->play_sound( Train->dsbSwitch );
        }
        // visual feedback
        Train->ggConverterFuseButton.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_compressortoggle( TTrain *Train, command_data const &Command ) {

    if( Train->mvControlled->CompressorPower >= 2 ) {
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->mvControlled->CompressorAllow ) {
            // turn on
            // visual feedback
            Train->ggCompressorButton.UpdateValue( 1.0 );
            // sound feedback
            if( Train->ggCompressorButton.GetValue() < 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            // impulse type switch has no effect if there's no power
            // NOTE: this is most likely setup wrong, but the whole thing is smoke and mirrors anyway
            // (we're presuming impulse type switch for all EMUs for the time being)
//            if( ( mvControlled->TrainType != dt_EZT )
//             || ( mvControlled->Mains ) ) {

                Train->mvControlled->CompressorSwitch( true );
//          }
        }
        else {
            //turn off
            if( true == Train->mvControlled->CompressorSwitch( false ) ) {
                // sound feedback
                Train->play_sound( Train->dsbSwitch );
                // NOTE: we don't have switch type definition for the compresor switch
                // so for the time being we have hard coded "impulse" switches for all EMUs
                // TODO: have proper switch type config for all switches, and put it in the cab switch descriptions, not in the .fiz
//                if( mvControlled->TrainType == dt_EZT ) {
//                    // visual feedback
//                    ggCompressorButton.UpdateValue( 1.0 );
//                }
//                else {
                    // visual feedback
                    Train->ggCompressorButton.UpdateValue( 0.0 );
//                }
            }
        }
    }
/*
    // disabled because EMUs have basic switches. left in case impulse switch code is needed, so we don't have to reinvent the wheel
    else if( Command.action == GLFW_RELEASE ) {
        // on button release...
        // TODO: check if compressor switch is two-state or impulse type
        // NOTE: we don't have switch type definition for the compresor switch
        // so for the time being we have hard coded "impulse" switches for all EMUs
        // TODO: have proper switch type config for all switches, and put it in the cab switch descriptions, not in the .fiz
        if( mvControlled->TrainType == dt_EZT ) {
            if( ggCompressorButton.SubModel != nullptr ) {
                ggCompressorButton.UpdateValue( 0.0 );
                // audio feedback
                play_sound( dsbSwitch );
            }
        }
    }
*/
}

void TTrain::OnCommand_motorconnectorsopen( TTrain *Train, command_data const &Command ) {

    // TODO: don't rely on presense of 3d model to determine presence of the switch
    if( Train->ggStLinOffButton.SubModel == nullptr ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Open Motor Power Connectors button is missing, or wasn't defined" );
        }
        return;
    }

    if( Command.action != GLFW_RELEASE ) {
        // button works while it's held down
        if( true == Train->mvControlled->StLinFlag ) {
            // yBARC - zmienione na przeciwne, bo true to zalaczone
            Train->mvControlled->StLinFlag = false;
            Train->play_sound( Train->dsbRelay );
        }
        Train->mvControlled->StLinSwitchOff = true;
        // sound feedback
        if( Train->ggStLinOffButton.GetValue() < 0.05 ) {
            Train->play_sound( Train->dsbSwitch );
        }
        // visual feedback
        Train->ggStLinOffButton.UpdateValue( 1.0 );
    }
    else {
        // button released
        Train->mvControlled->StLinSwitchOff = false;
        // sound feedback
        if( Train->ggStLinOffButton.GetValue() > 0.5 ) {
            Train->play_sound( Train->dsbSwitch );
        }
        // visual feedback
        Train->ggStLinOffButton.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_motordisconnect( TTrain *Train, command_data const &Command ) {

    if( ( Train->mvControlled->TrainType == dt_EZT ?
            ( Train->mvControlled != Train->mvOccupied ) :
            ( Train->mvOccupied->ActiveCab != 0 ) ) ) {
        // tylko w maszynowym
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        if( true == Train->mvControlled->CutOffEngine() ) {
            // sound feedback
            Train->play_sound( Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_motoroverloadrelaythresholdtoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( Train->mvControlled->Imax < Train->mvControlled->ImaxHi ) {
            // turn on
            if( true == Train->mvControlled->CurrentSwitch( true ) ) {
                // visual feedback
                Train->ggMaxCurrentCtrl.UpdateValue( 1.0 );
                // sound feedback
                if( Train->ggMaxCurrentCtrl.GetValue() < 0.5 ) {
                    Train->play_sound( Train->dsbSwitch );
                }
            }
        }
        else {
            //turn off
            if( true == Train->mvControlled->CurrentSwitch( false ) ) {
                // visual feedback
                Train->ggMaxCurrentCtrl.UpdateValue( 0.0 );
                // sound feedback
                if( Train->ggMaxCurrentCtrl.GetValue() > 0.5 ) {
                    Train->play_sound( Train->dsbSwitch );
                }
            }
        }
    }
}

void TTrain::OnCommand_motoroverloadrelayreset( TTrain *Train, command_data const &Command ) {

    if( Train->ggFuseButton.SubModel == nullptr ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Motor Overload Relay Reset button is missing, or wasn't defined" );
        }
//        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // press
        Train->mvControlled->FuseOn();
        // sound feedback
        if( Train->ggFuseButton.GetValue() < 0.05 ) {
            Train->play_sound( Train->dsbSwitch );
        }
        // visual feedback
        Train->ggFuseButton.UpdateValue( 1.0 );
    }
    else if( Command.action == GLFW_RELEASE ) {
        // release
        // sound feedback
        if( Train->ggFuseButton.GetValue() > 0.5 ) {
            Train->play_sound( Train->dsbSwitch );
        }
        // visual feedback
        Train->ggFuseButton.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_headlighttoggleleft( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    int const lightsindex =
        ( Train->mvOccupied->ActiveCab == 1 ?
            0 :
            1 );

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->DynamicObject->iLights[ lightsindex ] & TMoverParameters::light::headlight_left ) == 0 ) {
            // turn on
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::headlight_left;
            // visual feedback
            Train->ggLeftLightButton.UpdateValue( 1.0 );
            // sound feedback
            if( Train->ggLeftLightButton.GetValue() < 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::headlight_left;
            // visual feedback
            Train->ggLeftLightButton.UpdateValue( 0.0 );
            // sound feedback
            if( Train->ggLeftLightButton.GetValue() > 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
        }
    }
}

void TTrain::OnCommand_headlighttoggleright( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    int const lightsindex =
        ( Train->mvOccupied->ActiveCab == 1 ?
            0 :
            1 );

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->DynamicObject->iLights[ lightsindex ] & TMoverParameters::light::headlight_right ) == 0 ) {
            // turn on
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::headlight_right;
            // visual feedback
            Train->ggRightLightButton.UpdateValue( 1.0 );
            // sound feedback
            if( Train->ggRightLightButton.GetValue() < 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::headlight_right;
            // visual feedback
            Train->ggRightLightButton.UpdateValue( 0.0 );
            // sound feedback
            if( Train->ggRightLightButton.GetValue() > 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
        }
    }
}

void TTrain::OnCommand_headlighttoggleupper( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    int const lightsindex =
        ( Train->mvOccupied->ActiveCab == 1 ?
            0 :
            1 );

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->DynamicObject->iLights[ lightsindex ] & TMoverParameters::light::headlight_upper ) == 0 ) {
            // turn on
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::headlight_upper;
            // visual feedback
            Train->ggUpperLightButton.UpdateValue( 1.0 );
            // sound feedback
            if( Train->ggUpperLightButton.GetValue() < 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::headlight_upper;
            // visual feedback
            Train->ggUpperLightButton.UpdateValue( 0.0 );
            // sound feedback
            if( Train->ggUpperLightButton.GetValue() > 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
        }
    }
}

void TTrain::OnCommand_redmarkertoggleleft( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    int const lightsindex =
        ( Train->mvOccupied->ActiveCab == 1 ?
            0 :
            1 );

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->DynamicObject->iLights[ lightsindex ] & TMoverParameters::light::redmarker_left ) == 0 ) {
            // turn on
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::redmarker_left;
            // visual feedback
            Train->ggLeftEndLightButton.UpdateValue( 1.0 );
            // sound feedback
            if( Train->ggLeftEndLightButton.GetValue() < 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::redmarker_left;
            // visual feedback
            Train->ggLeftEndLightButton.UpdateValue( 0.0 );
            // sound feedback
            if( Train->ggLeftEndLightButton.GetValue() > 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
        }
    }
}

void TTrain::OnCommand_redmarkertoggleright( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    int const lightsindex =
        ( Train->mvOccupied->ActiveCab == 1 ?
            0 :
            1 );

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->DynamicObject->iLights[ lightsindex ] & TMoverParameters::light::redmarker_right ) == 0 ) {
            // turn on
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::redmarker_right;
            // visual feedback
            Train->ggRightEndLightButton.UpdateValue( 1.0 );
            // sound feedback
            if( Train->ggRightEndLightButton.GetValue() < 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::redmarker_right;
            // visual feedback
            Train->ggRightEndLightButton.UpdateValue( 0.0 );
            // sound feedback
            if( Train->ggRightEndLightButton.GetValue() > 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
        }
    }
}

void TTrain::OnCommand_headlighttogglerearleft( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    int const lightsindex =
        ( Train->mvOccupied->ActiveCab == 1 ?
            1 :
            0 );

    if( Command.action == GLFW_PRESS ) {
        // NOTE: we toggle the light on opposite side, as 'rear right' is 'front left' on the rear end etc
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->DynamicObject->iLights[ lightsindex ] & TMoverParameters::light::headlight_right ) == 0 ) {
            // turn on
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::headlight_right;
            // visual feedback
            Train->ggRearLeftLightButton.UpdateValue( 1.0 );
            // sound feedback
            if( Train->ggRearLeftLightButton.GetValue() < 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::headlight_right;
            // visual feedback
            Train->ggRearLeftLightButton.UpdateValue( 0.0 );
            // sound feedback
            if( Train->ggRearLeftLightButton.GetValue() > 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
        }
    }
}

void TTrain::OnCommand_headlighttogglerearright( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    int const lightsindex =
        ( Train->mvOccupied->ActiveCab == 1 ?
            1 :
            0 );

    if( Command.action == GLFW_PRESS ) {
        // NOTE: we toggle the light on opposite side, as 'rear right' is 'front left' on the rear end etc
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->DynamicObject->iLights[ lightsindex ] & TMoverParameters::light::headlight_left ) == 0 ) {
            // turn on
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::headlight_left;
            // visual feedback
            Train->ggRearRightLightButton.UpdateValue( 1.0 );
            // sound feedback
            if( Train->ggRearRightLightButton.GetValue() < 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::headlight_left;
            // visual feedback
            Train->ggRearRightLightButton.UpdateValue( 0.0 );
            // sound feedback
            if( Train->ggRearRightLightButton.GetValue() > 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
        }
    }
}

void TTrain::OnCommand_headlighttogglerearupper( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    int const lightsindex =
        ( Train->mvOccupied->ActiveCab == 1 ?
            1 :
            0 );

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->DynamicObject->iLights[ lightsindex ] & TMoverParameters::light::headlight_upper ) == 0 ) {
            // turn on
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::headlight_upper;
            // visual feedback
            Train->ggRearUpperLightButton.UpdateValue( 1.0 );
            // sound feedback
            if( Train->ggRearUpperLightButton.GetValue() < 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::headlight_upper;
            // visual feedback
            Train->ggRearUpperLightButton.UpdateValue( 0.0 );
            // sound feedback
            if( Train->ggRearUpperLightButton.GetValue() > 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
        }
    }
}

void TTrain::OnCommand_redmarkertogglerearleft( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    int const lightsindex =
        ( Train->mvOccupied->ActiveCab == 1 ?
            1 :
            0 );

    if( Command.action == GLFW_PRESS ) {
        // NOTE: we toggle the light on opposite side, as 'rear right' is 'front left' on the rear end etc
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->DynamicObject->iLights[ lightsindex ] & TMoverParameters::light::redmarker_right ) == 0 ) {
            // turn on
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::redmarker_right;
            // visual feedback
            Train->ggRearLeftEndLightButton.UpdateValue( 1.0 );
            // sound feedback
            if( Train->ggRearLeftEndLightButton.GetValue() < 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::redmarker_right;
            // visual feedback
            Train->ggRearLeftEndLightButton.UpdateValue( 0.0 );
            // sound feedback
            if( Train->ggRearLeftEndLightButton.GetValue() > 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
        }
    }
}

void TTrain::OnCommand_redmarkertogglerearright( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    int const lightsindex =
        ( Train->mvOccupied->ActiveCab == 1 ?
            1 :
            0 );

    if( Command.action == GLFW_PRESS ) {
        // NOTE: we toggle the light on opposite side, as 'rear right' is 'front left' on the rear end etc
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->DynamicObject->iLights[ lightsindex ] & TMoverParameters::light::redmarker_left ) == 0 ) {
            // turn on
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::redmarker_left;
            // visual feedback
            Train->ggRearRightEndLightButton.UpdateValue( 1.0 );
            // sound feedback
            if( Train->ggRearRightEndLightButton.GetValue() < 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::redmarker_left;
            // visual feedback
            Train->ggRearRightEndLightButton.UpdateValue( 0.0 );
            // sound feedback
            if( Train->ggRearRightEndLightButton.GetValue() > 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
        }
    }
}

void TTrain::OnCommand_headlightsdimtoggle( TTrain *Train, command_data const &Command ) {

    // NOTE: the check is disabled, as we're presuming light control is present in every vehicle
    // TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
    if( Train->ggDimHeadlightsButton.SubModel == nullptr ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Dim Headlights switch is missing, or wasn't defined" );
        }
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->DynamicObject->DimHeadlights ) {
            // turn on
            Train->DynamicObject->DimHeadlights = true;
            // audio feedback
            if( Train->ggDimHeadlightsButton.GetValue() < 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            // visual feedback
            Train->ggDimHeadlightsButton.UpdateValue( 1.0 );
        }
        else {
            //turn off
            Train->DynamicObject->DimHeadlights = false;
            // audio feedback
            if( Train->ggDimHeadlightsButton.GetValue() > 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            // visual feedback
            Train->ggDimHeadlightsButton.UpdateValue( 0.0 );
        }
    }
}

void TTrain::OnCommand_interiorlighttoggle( TTrain *Train, command_data const &Command ) {

    // NOTE: the check is disabled, as we're presuming light control is present in every vehicle
    // TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
    if( Train->ggCabLightButton.SubModel == nullptr ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Interior Light switch is missing, or wasn't defined" );
        }
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->bCabLight ) {
            // turn on
            Train->bCabLight = true;
            Train->btCabLight.TurnOn();
            // audio feedback
            if( Train->ggCabLightButton.GetValue() < 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            // visual feedback
            Train->ggCabLightButton.UpdateValue( 1.0 );
        }
        else {
            //turn off
            Train->bCabLight = false;
            Train->btCabLight.TurnOff();
            // audio feedback
            if( Train->ggCabLightButton.GetValue() > 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            // visual feedback
            Train->ggCabLightButton.UpdateValue( 0.0 );
        }
    }
}

void TTrain::OnCommand_interiorlightdimtoggle( TTrain *Train, command_data const &Command ) {

    // TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
    if( Train->ggCabLightDimButton.SubModel == nullptr ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Dim Interior Light switch is missing, or wasn't defined" );
        }
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->bCabLightDim ) {
            // turn on
            Train->bCabLightDim = true;
            // audio feedback
            if( Train->ggCabLightDimButton.GetValue() < 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            // visual feedback
            Train->ggCabLightDimButton.UpdateValue( 1.0 );
        }
        else {
            //turn off
            Train->bCabLightDim = false;
            // audio feedback
            if( Train->ggCabLightDimButton.GetValue() > 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            // visual feedback
            Train->ggCabLightDimButton.UpdateValue( 0.0 );
        }
    }
}

void TTrain::OnCommand_instrumentlighttoggle( TTrain *Train, command_data const &Command ) {

    // NOTE: the check is disabled, as we're presuming light control is present in every vehicle
    // TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
    if( Train->ggUniversal3Button.SubModel == nullptr ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Universal3 switch is missing, or wasn't defined" );
        }
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        // NOTE: instrument lighting isn't fully implemented, so we have to rely on the state of the 'button' i.e. light itself
        if( false == Train->LampkaUniversal3_st ) {
            // turn on
            Train->LampkaUniversal3_st = true;
            // audio feedback
            if( Train->ggUniversal3Button.GetValue() < 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            // visual feedback
            Train->ggUniversal3Button.UpdateValue( 1.0 );
        }
        else {
            //turn off
            Train->LampkaUniversal3_st = false;
            // audio feedback
            if( Train->ggUniversal3Button.GetValue() > 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            // visual feedback
            Train->ggUniversal3Button.UpdateValue( 0.0 );
        }
    }
}

void TTrain::OnCommand_heatingtoggle( TTrain *Train, command_data const &Command ) {

    if( Train->ggTrainHeatingButton.SubModel == nullptr ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Train Heating switch is missing, or wasn't defined" );
        }
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->mvControlled->Heating ) {
            // turn on
            Train->mvControlled->Heating = true;
            // audio feedback
            if( Train->ggTrainHeatingButton.GetValue() < 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            // visual feedback
            Train->ggTrainHeatingButton.UpdateValue( 1.0 );
        }
        else {
            //turn off
            Train->mvControlled->Heating = false;
            // audio feedback
            if( Train->ggTrainHeatingButton.GetValue() > 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            // visual feedback
            Train->ggTrainHeatingButton.UpdateValue( 0.0 );
        }
    }
}

void TTrain::OnCommand_doorlocktoggle( TTrain *Train, command_data const &Command ) {

    if( Train->ggDoorSignallingButton.SubModel == nullptr ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Door Lock switch is missing, or wasn't defined" );
        }
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the sound can loop uninterrupted
        if( false == Train->mvControlled->DoorSignalling ) {
            // turn on
            // TODO: check wheter we really need separate flags for this
            Train->mvControlled->DoorSignalling = true;
            Train->mvOccupied->DoorBlocked = true;
            // audio feedback
            if( Train->ggDoorSignallingButton.GetValue() < 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            // visual feedback
            Train->ggDoorSignallingButton.UpdateValue( 1.0 );
        }
        else {
            // turn off
            // TODO: check wheter we really need separate flags for this
            Train->mvControlled->DoorSignalling = false;
            Train->mvOccupied->DoorBlocked = false;
            // audio feedback
            if( Train->ggDoorSignallingButton.GetValue() > 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            // visual feedback
            Train->ggDoorSignallingButton.UpdateValue( 0.0 );
        }
    }
}

void TTrain::OnCommand_doortoggleleft( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->DoorOpenCtrl != 1 ) {
        return;
    }
    if( Command.action == GLFW_PRESS ) {
        // NOTE: test how the door state check works with consists where the occupied vehicle doesn't have opening doors
        if( false == (
            Train->mvOccupied->ActiveCab == 1 ?
                Train->mvOccupied->DoorLeftOpened :
                Train->mvOccupied->DoorRightOpened ) ) {
            // open
            if( Train->mvOccupied->ActiveCab == 1 ) {
                if( Train->mvOccupied->DoorLeft( true ) ) {
                    Train->ggDoorLeftButton.UpdateValue( 1.0 );
                    // sound feedback
                    if( Train->ggDoorLeftButton.GetValue() < 0.5 ) {
                        Train->play_sound( Train->dsbSwitch );
                    }
                    Train->play_sound( Train->dsbDoorOpen );
                }
            }
            else {
                // in the rear cab sides are reversed
                if( Train->mvOccupied->DoorRight( true ) ) {
                    Train->ggDoorRightButton.UpdateValue( 1.0 );
                    // sound feedback
                    if( Train->ggDoorRightButton.GetValue() < 0.5 ) {
                        Train->play_sound( Train->dsbSwitch );
                    }
                    Train->play_sound( Train->dsbDoorOpen );
                }
            }
        }
        else {
            // close
            if( Train->mvOccupied->ActiveCab == 1 ) {
                if( Train->mvOccupied->DoorLeft( false ) ) {
                    Train->ggDoorLeftButton.UpdateValue( 0.0 );
                    // sound feedback
                    if( Train->ggDoorLeftButton.GetValue() > 0.5 ) {
                        Train->play_sound( Train->dsbSwitch );
                    }
                    Train->play_sound( Train->dsbDoorClose );
                }
            }
            else {
                // in the rear cab sides are reversed
                if( Train->mvOccupied->DoorRight( false ) ) {
                    Train->ggDoorRightButton.UpdateValue( 0.0 );
                    // sound feedback
                    if( Train->ggDoorRightButton.GetValue() > 0.5 ) {
                        Train->play_sound( Train->dsbSwitch );
                    }
                    Train->play_sound( Train->dsbDoorClose );
                }
            }
        }
    }
}

void TTrain::OnCommand_doortoggleright( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->DoorOpenCtrl != 1 ) {
        return;
    }
    if( Command.action == GLFW_PRESS ) {
        // NOTE: test how the door state check works with consists where the occupied vehicle doesn't have opening doors
        if( false == (
            Train->mvOccupied->ActiveCab == 1 ?
            Train->mvOccupied->DoorRightOpened :
            Train->mvOccupied->DoorLeftOpened ) ) {
            // open
            if( Train->mvOccupied->ActiveCab == 1 ) {
                if( Train->mvOccupied->DoorRight( true ) ) {
                    Train->ggDoorRightButton.UpdateValue( 1.0 );
                    // sound feedback
                    if( Train->ggDoorRightButton.GetValue() < 0.5 ) {
                        Train->play_sound( Train->dsbSwitch );
                    }
                    Train->play_sound( Train->dsbDoorOpen );
                }
            }
            else {
                // in the rear cab sides are reversed
                if( Train->mvOccupied->DoorLeft( true ) ) {
                    Train->ggDoorLeftButton.UpdateValue( 1.0 );
                    // sound feedback
                    if( Train->ggDoorLeftButton.GetValue() < 0.5 ) {
                        Train->play_sound( Train->dsbSwitch );
                    }
                    Train->play_sound( Train->dsbDoorOpen );
                }
            }
        }
        else {
            // close
            if( Train->mvOccupied->ActiveCab == 1 ) {
                if( Train->mvOccupied->DoorRight( false ) ) {
                    Train->ggDoorRightButton.UpdateValue( 0.0 );
                    // sound feedback
                    if( Train->ggDoorRightButton.GetValue() > 0.5 ) {
                        Train->play_sound( Train->dsbSwitch );
                    }
                    Train->play_sound( Train->dsbDoorClose );
                }
            }
            else {
                // in the rear cab sides are reversed
                if( Train->mvOccupied->DoorLeft( false ) ) {
                    Train->ggDoorLeftButton.UpdateValue( 0.0 );
                    // sound feedback
                    if( Train->ggDoorLeftButton.GetValue() > 0.5 ) {
                        Train->play_sound( Train->dsbSwitch );
                    }
                    Train->play_sound( Train->dsbDoorClose );
                }
            }
        }
    }
}

void TTrain::OnCommand_departureannounce( TTrain *Train, command_data const &Command ) {

    if( Train->ggDepartureSignalButton.SubModel == nullptr ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Departure Signal button is missing, or wasn't defined" );
        }
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the sound can loop uninterrupted
        if( false == Train->mvControlled->DepartureSignal ) {
            // turn on
            Train->mvControlled->DepartureSignal = true;
            // audio feedback
            if( Train->ggDepartureSignalButton.GetValue() < 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            // visual feedback
            Train->ggDepartureSignalButton.UpdateValue( 1.0 );
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // turn off
        Train->mvControlled->DepartureSignal = false;
        // audio feedback
        if( Train->ggDepartureSignalButton.GetValue() > 0.5 ) {
            Train->play_sound( Train->dsbSwitch );
        }
        // visual feedback
        Train->ggDepartureSignalButton.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_hornlowactivate( TTrain *Train, command_data const &Command ) {

    if( ( Train->ggHornButton.SubModel == nullptr )
     && ( Train->ggHornLowButton.SubModel == nullptr ) ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Horn button is missing, or wasn't defined" );
        }
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only need to react to press, sound will continue until stopped
        if( false == TestFlag( Train->mvOccupied->WarningSignal, 1 ) ) {
            // turn on
            Train->mvOccupied->WarningSignal |= 1;
            if( true == TestFlag( Train->mvOccupied->WarningSignal, 2 ) ) {
                // low and high horn are treated as mutually exclusive
                Train->mvControlled->WarningSignal &= ~2;
            }
            // audio feedback
            if( ( Train->ggHornButton.GetValue() > -0.5 )
             || ( Train->ggHornLowButton.GetValue() < 0.5 ) ) {
                Train->play_sound( Train->dsbSwitch );
            }
            // visual feedback
            Train->ggHornButton.UpdateValue( -1.0 );
            Train->ggHornLowButton.UpdateValue( 1.0 );
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // turn off
        // NOTE: we turn off both low and high horn, due to unreliability of release event when shift key is involved
        Train->mvOccupied->WarningSignal &= ~( 1 | 2 );
        // audio feedback
        if( ( Train->ggHornButton.GetValue() < -0.5 )
         || ( Train->ggHornLowButton.GetValue() > 0.5 ) ) {
            Train->play_sound( Train->dsbSwitch );
        }
        // visual feedback
        Train->ggHornButton.UpdateValue( 0.0 );
        Train->ggHornLowButton.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_hornhighactivate( TTrain *Train, command_data const &Command ) {

    if( ( Train->ggHornButton.SubModel == nullptr )
     && ( Train->ggHornHighButton.SubModel == nullptr ) ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Horn button is missing, or wasn't defined" );
        }
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only need to react to press, sound will continue until stopped
        if( false == TestFlag( Train->mvOccupied->WarningSignal, 2 ) ) {
            // turn on
            Train->mvOccupied->WarningSignal |= 2;
            if( true == TestFlag( Train->mvOccupied->WarningSignal, 1 ) ) {
                // low and high horn are treated as mutually exclusive
                Train->mvControlled->WarningSignal &= ~1;
            }
            // audio feedback
            if( Train->ggHornButton.GetValue() < 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            // visual feedback
            Train->ggHornButton.UpdateValue( 1.0 );
            Train->ggHornHighButton.UpdateValue( 1.0 );
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // turn off
        // NOTE: we turn off both low and high horn, due to unreliability of release event when shift key is involved
        Train->mvOccupied->WarningSignal &= ~( 1 | 2 );
        // audio feedback
        if( Train->ggHornButton.GetValue() > 0.5 ) {
            Train->play_sound( Train->dsbSwitch );
        }
        // visual feedback
        Train->ggHornButton.UpdateValue( 0.0 );
        Train->ggHornHighButton.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_radiotoggle( TTrain *Train, command_data const &Command ) {

    if( Train->ggRadioButton.SubModel == nullptr ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Radio switch is missing, or wasn't defined" );
        }
/*
        // NOTE: we ignore the lack of 3d model to allow system reset after receiving radio-stop signal
        return;
*/
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the sound can loop uninterrupted
        if( false == Train->mvOccupied->Radio ) {
            // turn on
            Train->mvOccupied->Radio = true;
            // audio feedback
            if( Train->ggRadioButton.GetValue() < 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            // visual feedback
            Train->ggRadioButton.UpdateValue( 1.0 );
        }
        else {
            // turn off
            Train->mvOccupied->Radio = false;
            // audio feedback
            if( Train->ggRadioButton.GetValue() > 0.5 ) {
                Train->play_sound( Train->dsbSwitch );
            }
            // visual feedback
            Train->ggRadioButton.UpdateValue( 0.0 );
        }
    }
}

void TTrain::OnKeyDown(int cKey)
{ // naciśnięcie klawisza
    bool isEztOer;
    isEztOer = ((mvControlled->TrainType == dt_EZT) && (mvControlled->Battery == true) &&
                (mvControlled->EpFuse == true) && (mvOccupied->BrakeSubsystem == ss_ESt) &&
                (mvControlled->ActiveDir != 0)); // od yB
    // isEztOer=(mvControlled->TrainType==dt_EZT)&&(mvControlled->Mains)&&(mvOccupied->BrakeSubsystem==ss_ESt)&&(mvControlled->ActiveDir!=0);
    // isEztOer=((mvControlled->TrainType==dt_EZT)&&(mvControlled->Battery==true)&&(mvControlled->EpFuse==true)&&(mvOccupied->BrakeSubsystem==Oerlikon)&&(mvControlled->ActiveDir!=0));

    if (Global::shiftState)
	{ // wciśnięty [Shift]
            if( cKey == Global::Keys[ k_BrakeProfile ] ) // McZapkie-240302-B:
        //-----------

        // przelacznik opoznienia
        // hamowania
        { // yB://ABu: male poprawki, zeby bylo mozna ustawic dowolny wagon
            int CouplNr = -2;
            if( !FreeFlyModeFlag )
            {
#ifdef EU07_USE_OLD_COMMAND_SYSTEM
                if( Global::ctrlState )
                    if (mvOccupied->BrakeDelaySwitch(bdelay_R + bdelay_M))
                    {
                        play_sound( dsbPneumaticRelay );
                    }
                    else
                        ;
                else if (mvOccupied->BrakeDelaySwitch(bdelay_P))
                {
                    play_sound( dsbPneumaticRelay );
                }
#endif
            }
            else
            {
                TDynamicObject *temp;
                temp = (DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(), -1, 1500, CouplNr));
                if (temp == NULL)
                {
                    CouplNr = -2;
                    temp = (DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(), 1, 1500, CouplNr));
                }
                if (temp)
                {
                    if (Global::ctrlState)
                        if (temp->MoverParameters->BrakeDelaySwitch(bdelay_R + bdelay_M))
                        {
                            play_sound( dsbPneumaticRelay );
                        }
                        else
                            ;
                    else if (temp->MoverParameters->BrakeDelaySwitch(bdelay_P))
                    {
                        play_sound( dsbPneumaticRelay );
                    }
                }
            }
        }
        else if( cKey == GLFW_KEY_Q ) // ze Shiftem - włączenie AI
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
                    play_sound( dsbSwitch );
                    SetLights();
                }
            }
        }
    }
    else // McZapkie-240302 - klawisze bez shifta
    {
        if( cKey == Global::Keys[ k_IncLocalBrakeLevel ] )
        { // Ra 2014-09: w
            // trybie latania
            // obsługa jest w
            // World.cpp
            if (!FreeFlyModeFlag)
            {
                if (Global::ctrlState)
                    if ((mvOccupied->LocalBrake == ManualBrake) || (mvOccupied->MBrake == true))
                    {
                        mvOccupied->IncManualBrakeLevel(1);
                    }
#ifdef EU07_USE_OLD_COMMAND_SYSTEM
                else if (mvOccupied->LocalBrake != ManualBrake)
                    mvOccupied->IncLocalBrakeLevel(1);
#endif
            }
        }
        else if (cKey == Global::Keys[k_DecLocalBrakeLevel])
        { // Ra 2014-06: wersja dla
            // swobodnego latania
            // przeniesiona do
            // World.cpp
            if (!FreeFlyModeFlag)
            {
                if (Global::ctrlState)
                    if ((mvOccupied->LocalBrake == ManualBrake) || (mvOccupied->MBrake == true))
                        mvOccupied->DecManualBrakeLevel(1);
#ifdef EU07_USE_OLD_COMMAND_SYSTEM
                else // Ra 1014-06: AI potrafi zahamować pomocniczym mimo jego braku -
                    // odhamować jakoś trzeba
                    if ((mvOccupied->LocalBrake != ManualBrake) || mvOccupied->LocalBrakePos)
                    mvOccupied->DecLocalBrakeLevel(1);
#endif
            }
        }
        else if (cKey == Global::Keys[k_Brake2])
        {
            if (Global::ctrlState)
                mvOccupied->BrakeLevelSet(
                    mvOccupied->Handle->GetPos(bh_NP)); // yB: czy ten stos funkcji nie powinien być jako oddzielna funkcja movera?
        }
        else if (cKey == Global::Keys[k_Brake0])
        {
            if (Global::ctrlState)
            {
                mvOccupied->BrakeCtrlPos2 = 0; // wyrownaj kapturek
            }
        }
        if (cKey == Global::Keys[k_BrakeProfile])
        { // yB://ABu: male poprawki, zeby
            // bylo mozna ustawic dowolny
            // wagon
            int CouplNr = -2;
            if( !FreeFlyModeFlag )
            {
#ifdef EU07_USE_OLD_COMMAND_SYSTEM
                if( Global::ctrlState )
                    if (mvOccupied->BrakeDelaySwitch(bdelay_R))
                    {
                        play_sound( dsbPneumaticRelay );
                    }
                    else
                        ;
                else if (mvOccupied->BrakeDelaySwitch(bdelay_G))
                {
                    play_sound( dsbPneumaticRelay );
                }
#endif
            }
            else
            {
                TDynamicObject *temp;
                temp = (DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(), -1, 1500, CouplNr));
                if (temp == NULL)
                {
                    CouplNr = -2;
                    temp = (DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(), 1, 1500, CouplNr));
                }
                if (temp)
                {
                    if (Global::ctrlState)
                        if (temp->MoverParameters->BrakeDelaySwitch(bdelay_R))
                        {
                            play_sound( dsbPneumaticRelay );
                        }
                        else
                            ;
                    else if (temp->MoverParameters->BrakeDelaySwitch(bdelay_G))
                    {
                        play_sound( dsbPneumaticRelay );
                    }
                }
            }
        }
        // McZapkie-240302 - wylaczanie automatycznego pilota (w trybie ~debugmode
        // mozna tylko raz)
        else if (cKey == GLFW_KEY_Q) // bez Shift
        {
            if (DynamicObject->Mechanik)
                DynamicObject->Mechanik->TakeControl(false);
        }
        else if (cKey == Global::Keys[k_CabForward])
        {
            if( !CabChange( 1 ) ) {
                if( TestFlag( DynamicObject->MoverParameters->Couplers[ 0 ].CouplingFlag,
                    ctrain_passenger ) ) { // przejscie do nastepnego pojazdu
                    Global::changeDynObj = DynamicObject->PrevConnected;
                    Global::changeDynObj->MoverParameters->ActiveCab =
                        DynamicObject->PrevConnectedNo ? -1 : 1;
                }
            }
/*
            NOTE: disabled to allow 'prototypical' 'tricking' pantograph compressor to run unattended
            if (DynamicObject->MoverParameters->ActiveCab)
                mvControlled->PantCompFlag = false; // wyjście z maszynowego wyłącza sprężarkę
*/
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
/*
            NOTE: disabled to allow 'prototypical' 'tricking' pantograph compressor to run unattended
            if (DynamicObject->MoverParameters->ActiveCab)
                mvControlled->PantCompFlag =
                    false; // wyjście z maszynowego wyłącza sprężarkę pomocniczą
*/
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
                    tmp = DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(), 1, 1500, CouplNr);
                    if (tmp == nullptr)
                        tmp = DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(), -1, 1500, CouplNr);
                    if( ( CouplNr != -1 )
                     && ( tmp != nullptr )
                     && ( tmp->MoverParameters->Couplers[ CouplNr ].Connected != nullptr ) )
                    {
                        if (tmp->MoverParameters->Couplers[CouplNr].CouplingFlag == 0) // najpierw hak
                        {
                            if ((tmp->MoverParameters->Couplers[CouplNr].Connected->Couplers[CouplNr].AllowedFlag
                                & tmp->MoverParameters->Couplers[CouplNr].AllowedFlag
                                & ctrain_coupler) == ctrain_coupler)
                                if (tmp->MoverParameters->Attach(
                                        CouplNr, 2,
                                        tmp->MoverParameters->Couplers[CouplNr].Connected,
                                        ctrain_coupler))
                                {
                                    // tmp->MoverParameters->Couplers[CouplNr].Render=true;
                                    // //podłączony sprzęg będzie widoczny
                                    if (DynamicObject->Mechanik) // na wszelki wypadek
                                        DynamicObject->Mechanik->CheckVehicles(Connect); // aktualizacja flag kierunku w składzie
                                    play_sound( dsbCouplerAttach );
                                    // one coupling type per key press
                                    return;
                                }
                                else
                                    WriteLog("Mechanical coupling failed.");
                        }
                        if (!TestFlag(tmp->MoverParameters->Couplers[CouplNr].CouplingFlag, ctrain_pneumatic)) // pneumatyka
                        {
                            if ((tmp->MoverParameters->Couplers[CouplNr].Connected->Couplers[CouplNr].AllowedFlag
                                & tmp->MoverParameters->Couplers[CouplNr].AllowedFlag
                                & ctrain_pneumatic) == ctrain_pneumatic)
                                if (tmp->MoverParameters->Attach(
                                        CouplNr, 2,
                                        tmp->MoverParameters->Couplers[CouplNr].Connected,
                                        (tmp->MoverParameters->Couplers[CouplNr].CouplingFlag | ctrain_pneumatic)))
                                {
                                    // TODO: dedicated 'click' sound for connecting cable-type connections
                                    play_sound( dsbCouplerDetach );
//                                    rsHiss.Play( 1, DSBPLAY_LOOPING, true, tmp->GetPosition() );
                                    DynamicObject->SetPneumatic(CouplNr != 0, true); // Ra: to mi się nie podoba !!!!
                                    tmp->SetPneumatic(CouplNr != 0, true);
                                    // one coupling type per key press
                                    return;
                                }
                        }
                        if (!TestFlag(tmp->MoverParameters->Couplers[CouplNr].CouplingFlag, ctrain_scndpneumatic)) // zasilajacy
                        {
                            if ((tmp->MoverParameters->Couplers[CouplNr].Connected->Couplers[CouplNr].AllowedFlag
                                & tmp->MoverParameters->Couplers[CouplNr].AllowedFlag
                                & ctrain_scndpneumatic) == ctrain_scndpneumatic)
                                if (tmp->MoverParameters->Attach(
                                        CouplNr, 2,
                                        tmp->MoverParameters->Couplers[CouplNr].Connected,
                                        (tmp->MoverParameters->Couplers[CouplNr].CouplingFlag | ctrain_scndpneumatic)))
                                {
                                    // TODO: dedicated 'click' sound for connecting cable-type connections
                                    play_sound( dsbCouplerDetach );
//                                    rsHiss.Play( 1, DSBPLAY_LOOPING, true, tmp->GetPosition() );
                                    DynamicObject->SetPneumatic( CouplNr != 0, false ); // Ra: to mi się nie podoba !!!!
                                    tmp->SetPneumatic(CouplNr != 0, false);
                                    // one coupling type per key press
                                    return;
                                }
                        }
                        if (!TestFlag(tmp->MoverParameters->Couplers[CouplNr].CouplingFlag, ctrain_controll)) // ukrotnionko
                        {
                            if ((tmp->MoverParameters->Couplers[CouplNr].Connected->Couplers[CouplNr].AllowedFlag &
                                 tmp->MoverParameters->Couplers[CouplNr].AllowedFlag &
                                 ctrain_controll) == ctrain_controll)
                                if (tmp->MoverParameters->Attach(
                                        CouplNr, 2,
                                        tmp->MoverParameters->Couplers[CouplNr].Connected,
                                        (tmp->MoverParameters->Couplers[CouplNr].CouplingFlag | ctrain_controll)))
                                {
                                    // TODO: dedicated 'click' sound for connecting cable-type connections
                                    play_sound( dsbCouplerAttach );
                                    // one coupling type per key press
                                    return;
                                }
                        }
                        if (!TestFlag(tmp->MoverParameters->Couplers[CouplNr].CouplingFlag, ctrain_passenger)) // mostek
                        {
                            if ((tmp->MoverParameters->Couplers[CouplNr].Connected->Couplers[CouplNr].AllowedFlag &
                                 tmp->MoverParameters->Couplers[CouplNr].AllowedFlag &
                                 ctrain_passenger) == ctrain_passenger)
                                if (tmp->MoverParameters->Attach(
                                        CouplNr, 2,
                                        tmp->MoverParameters->Couplers[CouplNr].Connected,
                                        (tmp->MoverParameters->Couplers[CouplNr].CouplingFlag | ctrain_passenger)))
                                {
                                    play_sound( dsbCouplerAttach );
/*
                                    DynamicObject->SetPneumatic(CouplNr != 0, false);
                                    tmp->SetPneumatic(CouplNr != 0, false);
*/
                                    // one coupling type per key press
                                    return;
                                }
                        }
                        if( false == TestFlag( tmp->MoverParameters->Couplers[ CouplNr ].CouplingFlag, ctrain_heating ) ) {
                            // heating
                            if( ( tmp->MoverParameters->Couplers[ CouplNr ].Connected->Couplers[ CouplNr ].AllowedFlag
                                & tmp->MoverParameters->Couplers[ CouplNr ].AllowedFlag
                                & ctrain_heating ) == ctrain_heating )
                                if( tmp->MoverParameters->Attach(
                                    CouplNr, 2,
                                    tmp->MoverParameters->Couplers[ CouplNr ].Connected,
                                    ( tmp->MoverParameters->Couplers[ CouplNr ].CouplingFlag | ctrain_heating ) ) ) {

                                    // TODO: dedicated 'click' sound for connecting cable-type connections
                                    play_sound( dsbCouplerAttach );
                                    // one coupling type per key press
                                    return;
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
                if (!FreeFlyModeFlag) // tryb 'kabinowy' (pozwala również rozłączyć sprzęgi zablokowane)
                {
                    if (DynamicObject->DettachStatus(iCabn - 1) < 0) // jeśli jest co odczepić
                        if (DynamicObject->Dettach(iCabn - 1)) // iCab==1:przód,iCab==2:tył
                        {
                            play_sound( dsbCouplerDetach ); // w kabinie ten dźwięk?
                        }
                }
                else
                { // tryb freefly
                    int CouplNr = -1;
                    TDynamicObject *tmp;
                    tmp = DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(), 1, 1500, CouplNr);
                    if (tmp == NULL)
                        tmp = DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(), -1, 1500, CouplNr);
                    if (tmp && (CouplNr != -1))
                    {
                        if ((tmp->MoverParameters->Couplers[CouplNr].CouplingFlag & ctrain_depot) == 0) // jeżeli sprzęg niezablokowany
                            if (tmp->DettachStatus(CouplNr) < 0) // jeśli jest co odczepić i się da
                                if (!tmp->Dettach(CouplNr))
                                { // dźwięk odczepiania
                                    play_sound( dsbCouplerDetach );
                                }
                    }
                }
                if( DynamicObject->Mechanik ) {
                    // aktualizacja skrajnych pojazdów w składzie
                    DynamicObject->Mechanik->CheckVehicles( Disconnect );
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
                    play_sound( dsbSwitch );
                    SetLights();
                }
            }
        }
        //    else
        if (DebugModeFlag)
        { // przesuwanie składu o 100m
            TDynamicObject *d = DynamicObject;
            if (cKey == GLFW_KEY_LEFT_BRACKET)
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
            else if (cKey == GLFW_KEY_RIGHT_BRACKET)
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
        if (cKey == GLFW_KEY_MINUS)
        { // zmniejszenie numeru kanału radiowego
            if (iRadioChannel > 0)
                --iRadioChannel; // 0=wyłączony
        }
        else if (cKey == GLFW_KEY_EQUAL)
        { // zmniejszenie numeru kanału radiowego
            if (iRadioChannel < 8)
                ++iRadioChannel; // 0=wyłączony
        }
    }
}

// cab movement update, fixed step part
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

    Math3D::vector3 shake;
    // McZapkie: najpierw policzę pozycję w/m kabiny

    // ABu: rzucamy kabina tylko przy duzym FPS!
    // Mala histereza, zeby bez przerwy nie przelaczalo przy FPS~17
    // Granice mozna ustalic doswiadczalnie. Ja proponuje 14:20
    double const iVel = std::min( DynamicObject->GetVelocity(), 150.0 );

    if( !Global::iSlowMotion // musi być pełna prędkość
        && ( pMechOffset.y < 4.0 ) ) // Ra 15-01: przy oglądaniu pantografu bujanie przeszkadza
    {
        if( iVel > 0.5 ) {
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
                    ( Random( iVel * 2 ) - iVel ) / ( ( iVel * 2 ) * 4 ) * fMechSpringZ )
                    * 1.25,
                    pMechShake );
                //                    * (( 200 - DynamicObject->MyTrack->iQualityFlag ) * 0.0075 ); // scale to 75-150% based on track quality
            }
//            shake *= 0.85;
        }
        vMechVelocity -= ( shake + vMechVelocity * 100 ) * ( fMechSpringX + fMechSpringY + fMechSpringZ ) / ( 200 );
        //        shake *= 0.95 * dt; // shake damping

        // McZapkie:
        pMechShake += vMechVelocity * dt;
        // Ra 2015-01: dotychczasowe rzucanie
        pMechOffset += vMechMovement * dt;
        if( ( pMechShake.y > fMechMaxSpring ) || ( pMechShake.y < -fMechMaxSpring ) )
            vMechVelocity.y = -vMechVelocity.y;
        // ABu011104: 5*pMechShake.y, zeby ladnie pudlem rzucalo :)
        pMechPosition = pMechOffset + vector3( 1.5 * pMechShake.x, 2.0 * pMechShake.y, 1.5 * pMechShake.z );
//        vMechMovement = 0.5 * vMechMovement;
    }
    else { // hamowanie rzucania przy spadku FPS
        pMechShake -= pMechShake * std::min( dt, 1.0 ); // po tym chyba potrafią zostać jakieś ułamki, które powodują zjazd
        pMechOffset += vMechMovement * dt;
        vMechVelocity.y = 0.5 * vMechVelocity.y;
        pMechPosition = pMechOffset + vector3( pMechShake.x, 5 * pMechShake.y, pMechShake.z );
//        vMechMovement = 0.5 * vMechMovement;
    }
    // numer kabiny (-1: kabina B)
    if( DynamicObject->Mechanik ) // może nie być?
        if( DynamicObject->Mechanik->AIControllFlag ) // jeśli prowadzi AI
        { // Ra: przesiadka, jeśli AI zmieniło kabinę (a człon?)...
            if( iCabn != ( DynamicObject->MoverParameters->ActiveCab == -1 ?
                2 :
                DynamicObject->MoverParameters->ActiveCab ) )
                InitializeCab( DynamicObject->MoverParameters->ActiveCab,
                DynamicObject->asBaseDir + DynamicObject->MoverParameters->TypeName +
                ".mmd" );
        }
    iCabn = ( DynamicObject->MoverParameters->ActiveCab == -1 ?
        2 :
        DynamicObject->MoverParameters->ActiveCab );
    if( !DebugModeFlag ) { // sprawdzaj więzy //Ra: nie tu!
        if( pMechPosition.x < Cabine[ iCabn ].CabPos1.x )
            pMechPosition.x = Cabine[ iCabn ].CabPos1.x;
        if( pMechPosition.x > Cabine[ iCabn ].CabPos2.x )
            pMechPosition.x = Cabine[ iCabn ].CabPos2.x;
        if( pMechPosition.z < Cabine[ iCabn ].CabPos1.z )
            pMechPosition.z = Cabine[ iCabn ].CabPos1.z;
        if( pMechPosition.z > Cabine[ iCabn ].CabPos2.z )
            pMechPosition.z = Cabine[ iCabn ].CabPos2.z;
        if( pMechPosition.y > Cabine[ iCabn ].CabPos1.y + 1.8 )
            pMechPosition.y = Cabine[ iCabn ].CabPos1.y + 1.8;
        if( pMechPosition.y < Cabine[ iCabn ].CabPos1.y + 0.5 )
            pMechPosition.y = Cabine[ iCabn ].CabPos2.y + 0.5;

        if( pMechOffset.x < Cabine[ iCabn ].CabPos1.x )
            pMechOffset.x = Cabine[ iCabn ].CabPos1.x;
        if( pMechOffset.x > Cabine[ iCabn ].CabPos2.x )
            pMechOffset.x = Cabine[ iCabn ].CabPos2.x;
        if( pMechOffset.z < Cabine[ iCabn ].CabPos1.z )
            pMechOffset.z = Cabine[ iCabn ].CabPos1.z;
        if( pMechOffset.z > Cabine[ iCabn ].CabPos2.z )
            pMechOffset.z = Cabine[ iCabn ].CabPos2.z;
        if( pMechOffset.y > Cabine[ iCabn ].CabPos1.y + 1.8 )
            pMechOffset.y = Cabine[ iCabn ].CabPos1.y + 1.8;
        if( pMechOffset.y < Cabine[ iCabn ].CabPos1.y + 0.5 )
            pMechOffset.y = Cabine[ iCabn ].CabPos2.y + 0.5;
    }
};

// returns position of the mechanic in the scene coordinates
vector3
TTrain::GetWorldMechPosition() {

    vector3 position = DynamicObject->mMatrix *pMechPosition; // położenie względem środka pojazdu w układzie scenerii
    position += DynamicObject->GetPosition();
    return position;
}

bool TTrain::Update( double const Deltatime )
{
    // check for sent user commands
    // NOTE: this is a temporary arrangement, for the transition period from old command setup to the new one
    // eventually commands are going to be retrieved directly by the vehicle, filtered through active control stand
    // and ultimately executed, provided the stand allows it.
    command_data commanddata;
    // NOTE: currently we're only storing commands for local vehicle and there's no id system in place,
    // so we're supplying 'default' vehicle id of 0
    while( simulation::Commands.pop( commanddata, static_cast<std::size_t>( command_target::vehicle ) | 0 ) ) {

        auto lookup = m_commandhandlers.find( commanddata.command );
        if( lookup != m_commandhandlers.end() ) {
            // debug data
            if( commanddata.action != GLFW_RELEASE ) {
                WriteLog( mvOccupied->Name + " received command: [" + simulation::Commands_descriptions[ static_cast<std::size_t>( commanddata.command ) ].name + "]" );
            }
            // pass the command to the assigned handler
            lookup->second( this, commanddata );
        }
    }

    // update driver's position
    {
        vector3 Vec = Global::pCamera->Velocity * -2.0;// -7.5 * Timer::GetDeltaRenderTime();
        Vec.y = -Vec.y;
        if( mvOccupied->ActiveCab < 0 ) {
            Vec *= -1.0f;
            Vec.y = -Vec.y;
        }
        Vec.RotateY( Global::pCamera->Yaw );
        vMechMovement = Vec;
    }

    DWORD stat;
    double dt = Deltatime; // Timer::GetDeltaTime();

    if (DynamicObject->mdKabina)
    { // Ra: TODO: odczyty klawiatury/pulpitu nie
        // powinny być uzależnione od istnienia modelu
        // kabiny
        tor = DynamicObject->GetTrack(); // McZapkie-180203
        // McZapkie: predkosc wyswietlana na tachometrze brana jest z obrotow kol
        float maxtacho = 3;
        fTachoVelocity = static_cast<float>( std::min( std::abs(11.31 * mvControlled->WheelDiameter * mvControlled->nrot), mvControlled->Vmax * 1.05) );
        { // skacze osobna zmienna
            float ff = simulation::Time.data().wSecond; // skacze co sekunde - pol sekundy
            // pomiar, pol sekundy ustawienie
            if (ff != fTachoTimer) // jesli w tej sekundzie nie zmienial
            {
                if (fTachoVelocity > 1) // jedzie
                    fTachoVelocityJump = fTachoVelocity + (2.0 - Random(3) + Random(3)) * 0.5;
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
        // Ra 2014-09: napięcia i prądy muszą być ustalone najpierw, bo wysyłane są ewentualnie na PoKeys
		if ((mvControlled->EngineType != DieselElectric)
         && (mvControlled->EngineType != ElectricInductionMotor)) // Ra 2014-09: czy taki rozdzia? ma sens?
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

		if (Global::bMWDmasterEnable) // pobieranie danych dla pulpitu port (COM)
		{ 
			Console::ValueSet(0, mvOccupied->Compressor); // zbiornik główny
			Console::ValueSet(1, mvOccupied->PipePress); // przewód główny
			Console::ValueSet(2, mvOccupied->BrakePress); // cylinder hamulcowy
			Console::ValueSet(3, fHVoltage); // woltomierz wysokiego napięcia
			Console::ValueSet(4, fHCurrent[(mvControlled->TrainType & dt_EZT) ? 0 : 1]); 
			// pierwszy amperomierz; dla EZT prąd całkowity
			Console::ValueSet(5, fHCurrent[2]); // drugi amperomierz 2
			Console::ValueSet(6, fHCurrent[3]); // drugi amperomierz 3
			Console::ValueSet(7, fTachoVelocity);
			//Console::ValueSet(8, mvControlled->BatteryVoltage); // jeszcze nie pora ;)
		}

        // hunter-080812: wyrzucanie szybkiego na elektrykach gdy nie ma napiecia
        // przy dowolnym ustawieniu kierunkowego
        // Ra: to już jest w T_MoverParameters::TractionForce(), ale zależy od
        // kierunku
        if( ( mvControlled->Mains )
         && ( mvControlled->EngineType == ElectricSeriesMotor ) ) {
            if( std::max( mvControlled->GetTrainsetVoltage(), std::fabs( mvControlled->RunningTraction.TractionVoltage ) ) < 0.5 * mvControlled->EnginePowerSource.MaxVoltage ) {
                // TODO: check whether it should affect entire consist for EMU
                // TODO: check whether it should happen if there's power supplied alternatively through hvcouplers
                // TODO: potentially move this to the mover module, as there isn't much reason to have this dependent on the operator presence
                mvControlled->MainSwitch( false, ( mvControlled->TrainType == dt_EZT ? command_range::unit : command_range::local ) );
            }
        }

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
                        mvControlled->MainSwitch( false, ( mvControlled->TrainType == dt_EZT ? command_range::unit : command_range::local ) );
                }
                else if( fConverterTimer >= fConverterPrzekaznik ) {
                    // changed switch from always true to take into account state of the compressor switch
                    mvControlled->CompressorSwitch( mvControlled->CompressorAllow );
                }
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
                    vol = 2.0 * rsHiss.AM * fPPress;
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
            if (rsHiss.AM != 0.0) // upuszczanie z PG
            {
                fPPress = (4.0f * fPPress + std::max(mvOccupied->dpLocalValve, mvOccupied->dpMainValve)) / (4.0f + 1.0f);
                if (fPPress > 0.0f)
                {
                    vol = 2.0 * rsHiss.AM * fPPress;
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
            if (rsHissU.AM != 0.0) // napelnianie PG
            {
                fNPress = (4.0f * fNPress + Min0R(mvOccupied->dpLocalValve, mvOccupied->dpMainValve)) / (4.0f + 1.0f);
                if (fNPress < 0.0f)
                {
                    vol = -1.0 * rsHissU.AM * fNPress;
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
                if( dsbRelay != nullptr ) { dsbRelay->SetVolume( DSBVOLUME_MAX ); }
            }
            else
            {
                if( dsbRelay != nullptr ) { dsbRelay->SetVolume( -40 ); }
            }
            if (!TestFlag(mvOccupied->SoundFlag, sound_manyrelay))
                play_sound( dsbRelay );
            else
            {
                if (TestFlag(mvOccupied->SoundFlag, sound_loud))
                    play_sound( dsbWejscie_na_bezoporow );
                else
                    play_sound( dsbWejscie_na_drugi_uklad );
            }
        }

        if( dsbBufferClamp != nullptr ) {
            if( TestFlag( mvOccupied->SoundFlag, sound_bufferclamp ) ) // zderzaki uderzaja o siebie
            {
                if( TestFlag( mvOccupied->SoundFlag, sound_loud ) )
                    dsbBufferClamp->SetVolume( DSBVOLUME_MAX );
                else
                    dsbBufferClamp->SetVolume( -20 );
                play_sound( dsbBufferClamp );
            }
        }
        if (dsbCouplerStretch)
            if (TestFlag(mvOccupied->SoundFlag, sound_couplerstretch)) // sprzegi sie rozciagaja
            {
                if (TestFlag(mvOccupied->SoundFlag, sound_loud))
                    dsbCouplerStretch->SetVolume(DSBVOLUME_MAX);
                else
                    dsbCouplerStretch->SetVolume(-20);
                play_sound( dsbCouplerStretch );
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
            ggClockSInd.UpdateValue(simulation::Time.data().wSecond);
            ggClockSInd.Update();
            ggClockMInd.UpdateValue(simulation::Time.data().wMinute);
            ggClockMInd.Update();
            ggClockHInd.UpdateValue(simulation::Time.data().wHour + simulation::Time.data().wMinute / 60.0);
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
            // NOTE: since we don't have functional converter object, we're faking it here by simple check whether converter is on
            // TODO: implement object-based circuits and power systems model so we can have this working more properly
            ggLVoltage.UpdateValue(
                std::max(
                    ( mvControlled->ConverterFlag ?
                        mvControlled->NominalBatteryVoltage :
                        0.0 ),
                    ( mvControlled->Battery ?
                        mvControlled->BatteryVoltage :
                        0.0 ) ) );
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
                rsSlippery.Play(-rsSlippery.AM * veldiff + rsSlippery.AA, DSBPLAY_LOOPING, true, DynamicObject->GetPosition());
                if (mvControlled->TrainType == dt_181) // alarm przy poslizgu dla 181/182 - BOMBARDIER
                    if (dsbSlipAlarm)
                        play_sound( dsbSlipAlarm, DSBPLAY_LOOPING );
            }
            else
            {
                if ((mvOccupied->UnitBrakeForce > 100.0) && (DynamicObject->GetVelocity() > 1.0))
                {
                    rsSlippery.Play(rsSlippery.AM * veldiff + rsSlippery.AA, DSBPLAY_LOOPING, true, DynamicObject->GetPosition());
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

        if (mvControlled->Battery || mvControlled->ConverterFlag)
        {
            btLampkaWylSzybki.Turn(
                ( ( (m_linebreakerstate > 0)
                 || (true == mvControlled->Mains) ) ?
                    true :
                    false ) );

            // hunter-261211: jakis stary kod (i niezgodny z prawda),
            // zahaszowalem i poprawilem
            // youBy-220913: ale przyda sie do lampki samej przetwornicy
            btLampkaPrzetw.Turn( !mvControlled->ConverterFlag );
            btLampkaNadmPrzetw.Turn( mvControlled->ConvOvldFlag );

            btLampkaOpory.Turn(
                mvControlled->StLinFlag ?
                    mvControlled->ResistorsFlagCheck() :
                    false );

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
            //         ((TestFlag(mvControlled->BrakeStatus,+b_Rused+b_Ractive)))//Lampka drugiego stopnia hamowania
            btLampkaHamPosp.Turn((TestFlag(mvOccupied->Hamulec->GetBrakeStatus(), 1))); // lampka drugiego stopnia hamowania
            //TODO: youBy wyciągnąć flagę wysokiego stopnia

            // hunter-121211: lampka zanikowo-pradowego wentylatorow:
            btLampkaNadmWent.Turn( ( mvControlled->RventRot < 5.0 ) && ( mvControlled->ResistorsFlagCheck() ) );
            //-------

            btLampkaWysRozr.Turn(!(mvControlled->Imax < mvControlled->ImaxHi));

            if (((mvControlled->ScndCtrlActualPos > 0) ||
                 ((mvControlled->RList[mvControlled->MainCtrlActualPos].ScndAct != 0) &&
                  (mvControlled->RList[mvControlled->MainCtrlActualPos].ScndAct != 255))) &&
                (!mvControlled->DelayCtrlFlag))
                btLampkaBoczniki.TurnOn();
            else
                btLampkaBoczniki.TurnOff();

            btLampkaNapNastHam.Turn(mvControlled->ActiveDir != 0); // napiecie na nastawniku hamulcowym
            btLampkaSprezarka.Turn(mvControlled->CompressorFlag); // mutopsitka dziala
            // boczniki
            unsigned char scp; // Ra: dopisałem "unsigned"
            // Ra: w SU45 boczniki wchodzą na MainCtrlPos, a nie na MainCtrlActualPos
            // - pokićkał ktoś?
            scp = mvControlled->RList[mvControlled->MainCtrlPos].ScndAct;
            scp = (scp == 255 ? 0 : scp); // Ra: whatta hella is this?
            if ((mvControlled->ScndCtrlPos > 0) || (mvControlled->ScndInMain != 0) && (scp > 0))
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
            btLampkaWysRozr.TurnOff();
            btLampkaOpory.TurnOff();
            btLampkaStyczn.TurnOff();
            btLampkaUkrotnienie.TurnOff();
            btLampkaHamPosp.TurnOff();
            btLampkaBoczniki.TurnOff();
            btLampkaNapNastHam.TurnOff();
            btLampkaPrzetw.TurnOff();
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
        if( mvControlled->Battery || mvControlled->ConverterFlag )
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
            btLampkaDepartureSignal.Turn( mvControlled->DepartureSignal );
            btLampkaNapNastHam.Turn((mvControlled->ActiveDir != 0) && (mvOccupied->EpFuse)); // napiecie na nastawniku hamulcowym
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
            btLampkaDepartureSignal.TurnOff();
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
				if (mvOccupied->BrakeHandle == FV4a)
                {
                    double b = Console::AnalogCalibrateGet(0);
					b = b * 8 - 2;
					b = Global::CutValueToRange(-2.0, b, mvOccupied->BrakeCtrlPosNo); // przycięcie zmiennej do granic
					if (Global::bMWDdebugEnable && Global::iMWDDebugMode & 4) WriteLog("FV4a break position = " + to_string(b));
					ggBrakeCtrl.UpdateValue(b); // przesów bez zaokrąglenia
					mvOccupied->BrakeLevelSet(b);
				}
                if (mvOccupied->BrakeHandle == FVel6) // może można usunąć ograniczenie do FV4a i FVel6?
                {
                    double b = Console::AnalogCalibrateGet(0);
					b = b * 7 - 1;
					b = Global::CutValueToRange(-1.0, b, mvOccupied->BrakeCtrlPosNo); // przycięcie zmiennej do granic
					if (Global::bMWDdebugEnable && Global::iMWDDebugMode & 4) WriteLog("FVel6 break position = " + to_string(b));
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
                    (DynamicObject->Mechanik->AIControllFlag ? false : (Global::iFeedbackMode == 4 || (Global::bMWDmasterEnable && Global::bMWDBreakEnable))) :
                    false) // nie blokujemy AI
            { // Ra: nie najlepsze miejsce, ale na początek gdzieś to dać trzeba
			  // Firleju: dlatego kasujemy i zastepujemy funkcją w Console
                if ((mvOccupied->BrakeLocHandle == FD1))
                {
                    double b = Console::AnalogCalibrateGet(1);
					b *= 10;
					b = Global::CutValueToRange(0.0, b, LocalBrakePosNo); // przycięcie zmiennej do granic
                    ggLocalBrake.UpdateValue(b); // przesów bez zaokrąglenia
					if (Global::bMWDdebugEnable && Global::iMWDDebugMode & 4) WriteLog("FD1 break position = " + to_string(b));
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
        ggBrakeProfileCtrl.Update();
        ggBrakeProfileG.Update();
        ggBrakeProfileR.Update();
        ggMaxCurrentCtrl.Update();
        // NBMX wrzesien 2003 - drzwi
        ggDoorLeftButton.Update();
        ggDoorRightButton.Update();
        ggDepartureSignalButton.Update();
        // NBMX dzwignia sprezarki
        ggCompressorButton.Update();
        ggMainButton.Update();
        ggRadioButton.Update();
        ggConverterButton.Update();
        ggConverterOffButton.Update();

#ifdef EU07_USE_OLD_COMMAND_SYSTEM
        if( ( ( DynamicObject->iLights[ 0 ] ) == 0 ) && ( ( DynamicObject->iLights[ 1 ] ) == 0 ) )
        {
            ggRightLightButton.PutValue(0);
            ggLeftLightButton.PutValue(0);
            ggUpperLightButton.PutValue(0);
            ggRightEndLightButton.PutValue(0);
            ggLeftEndLightButton.PutValue(0);
        }

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
#endif
        if (ggLightsButton.SubModel)
        {
            ggLightsButton.PutValue(mvOccupied->LightsPos - 1);
            ggLightsButton.Update();
        }
        ggDimHeadlightsButton.Update();
        //---------
        // Winger 010304 - pantografy
        // NOTE: shouldn't the pantograph updates check whether it's front or rear cabin?
        ggPantFrontButton.Update();
        ggPantRearButton.Update();
        ggPantFrontButtonOff.Update();
        ggTrainHeatingButton.Update();
        ggSignallingButton.Update();
        ggDoorSignallingButton.Update();
        // Winger 020304 - ogrzewanie
        // hunter-080812: poprawka na ogrzewanie w elektrykach - usuniete uzaleznienie od przetwornicy
        if ((((mvControlled->EngineType == ElectricSeriesMotor) && (mvControlled->Mains == true) &&
              (mvControlled->ConvOvldFlag == false)) ||
             (mvControlled->ConverterFlag)) &&
            (mvControlled->Heating == true))
            btLampkaOgrzewanieSkladu.TurnOn();
        else
            btLampkaOgrzewanieSkladu.TurnOff();

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
                        play_sound( dsbBuzzer, DSBVOLUME_MAX, DSBPLAY_LOOPING );
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

#ifdef EU07_USE_OLD_COMMAND_SYSTEM
        if( Console::Pressed( Global::Keys[ k_Horn ] ) )
        {
            if (Global::shiftState)
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
        if (Global::shiftState && Console::Pressed(Global::Keys[k_Main]))
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
        if (!Global::shiftState && Console::Pressed(Global::Keys[k_Main]))
        {
            ggMainOffButton.PutValue(1);
            if (mvControlled->MainSwitch(false))
                dsbRelay->Play(0, 0, 0);
        }
        else
            ggMainOffButton.UpdateValue(0);
#endif
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
#ifdef EU07_USE_OLD_COMMAND_SYSTEM
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
#endif
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

#ifdef EU07_USE_OLD_COMMAND_SYSTEM
        if( Console::Pressed( Global::Keys[ k_Sand ] ) )
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
        if (Global::shiftState &&
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
        //     Global::shiftState&&Console::Pressed(Global::Keys[k_Compressor])&&((mvControlled->EngineType==ElectricSeriesMotor)||(mvControlled->TrainType==dt_EZT))
        //     )   //NBMX 14-09-2003: sprezarka wl
        if (Global::shiftState && Console::Pressed(Global::Keys[k_Compressor]) &&
            (mvControlled->CompressorPower < 2)) // hunter-091012: tak jest poprawnie
        { // hunter-110212: poprawka dla EZT
            ggCompressorButton.PutValue(1);
            mvControlled->CompressorSwitch(true);
        }
        if (!Global::shiftState &&
            Console::Pressed(Global::Keys[k_Converter])) // NBMX 14-09-2003: przetwornica wl
        {
            ggConverterButton.PutValue(0);
            ggConverterOffButton.PutValue(1);
            mvControlled->ConverterSwitch(false);
            if ((mvControlled->TrainType == dt_EZT) && (!TestFlag(mvControlled->EngDmgFlag, 4)))
                mvControlled->ConvOvldFlag = false;
        }
        //     if (
        //     !Global::shiftState&&Console::Pressed(Global::Keys[k_Compressor])&&((mvControlled->EngineType==ElectricSeriesMotor)||(mvControlled->TrainType==dt_EZT))
        //     )   //NBMX 14-09-2003: sprezarka wl
        if (!Global::shiftState && Console::Pressed(Global::Keys[k_Compressor]) &&
            (mvControlled->CompressorPower < 2)) // hunter-091012: tak jest poprawnie
        { // hunter-110212: poprawka dla EZT
            ggCompressorButton.PutValue(0);
            mvControlled->CompressorSwitch(false);
        }
#endif
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
#ifdef EU07_USE_OLD_COMMAND_SYSTEM
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
#endif

        if (Console::Pressed(Global::Keys[k_Univ1]))
        {
            if (!DebugModeFlag)
            {
                if (ggUniversal1Button.SubModel)
                    if (Global::shiftState)
                        ggUniversal1Button.IncValue(dt / 2);
                    else
                        ggUniversal1Button.DecValue(dt / 2);
            }
        }
#ifdef EU07_USE_OLD_COMMAND_SYSTEM
        if (!Console::Pressed(Global::Keys[k_SmallCompressor]))
            // Ra: przecieść to na zwolnienie klawisza
            if (DynamicObject->Mechanik ? !DynamicObject->Mechanik->AIControllFlag :
                                          false) // nie wyłączać, gdy AI
                mvControlled->PantCompFlag = false; // wyłączona, gdy nie trzymamy klawisza
#else
/*
        // NOTE: disabled to allow 'prototypical' 'tricking' pantograph compressor into running unattended
        if( ( ( DynamicObject->Mechanik != nullptr )
           && ( false == DynamicObject->Mechanik->AIControllFlag ) )
         && ( mvControlled->TrainType == dt_EZT ?
                ( mvControlled != mvOccupied ) :
                ( mvOccupied->ActiveCab != 0 ) ) ) {
            // HACK: if we're in one of the cabs we can't be activating pantograph compressor
            // NOTE: this will break in multiplayer setups, do a proper tracking of pantograph user then
            mvControlled->PantCompFlag = false;
        }
*/
#endif
        if (Console::Pressed(Global::Keys[k_Univ2]))
        {
            if (!DebugModeFlag)
            {
                if (ggUniversal2Button.SubModel)
                    if (Global::shiftState)
                        ggUniversal2Button.IncValue(dt / 2);
                    else
                        ggUniversal2Button.DecValue(dt / 2);
            }
        }

#ifdef EU07_USE_OLD_COMMAND_SYSTEM
        // hunter-091012: zrobione z uwzglednieniem przelacznika swiatla
        if (Console::Pressed(Global::Keys[k_Univ3]))
        {
            if (Global::shiftState)
            {
                if (Global::ctrlState)
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
                if (Global::ctrlState)
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
#endif
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

#ifdef EU07_USE_OLD_COMMAND_SYSTEM
        // hunter-091012: przepisanie univ4 i zrobione z uwzglednieniem przelacznika
        // swiatla
        if (Console::Pressed(Global::Keys[k_Univ4]))
        {
            if( Global::shiftState )
            {
                if (Global::ctrlState)
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
                if (Global::ctrlState)
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
                play_sound( dsbPneumaticSwitch );
            }
        }
#endif
/*
        // NOTE: disabled, as it doesn't seem to be used.
        // TODO: get rid of it altogether when we're cleaninng
        // Ra: przeklejka z SPKS - płynne poruszanie hamulcem
        // if
        // ((mvOccupied->BrakeHandle==FV4a)&&(Console::Pressed(Global::Keys[k_IncBrakeLevel])))
        if ((Console::Pressed(Global::Keys[k_IncBrakeLevel])))
        {
            if (Global::ctrlState)
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
            if (Global::ctrlState)
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
*/
        if ((mvOccupied->BrakeHandle == FV4a) && (Console::Pressed(Global::Keys[k_IncBrakeLevel])))
        {
            if (Global::ctrlState)
            {
                mvOccupied->BrakeCtrlPos2 -= dt / 20.0;
                if (mvOccupied->BrakeCtrlPos2 < -1.5)
                    mvOccupied->BrakeCtrlPos2 = -1.5;
            }
#ifdef EU07_USE_OLD_COMMAND_SYSTEM
            else
            {
                //        mvOccupied->BrakeCtrlPosR+=(mvOccupied->BrakeCtrlPosR>mvOccupied->BrakeCtrlPosNo?0:dt*2);
                mvOccupied->BrakeLevelAdd(dt * 2);
                //        mvOccupied->BrakeCtrlPos=
                //        floor(mvOccupied->BrakeCtrlPosR+0.499);
            }
#endif
        }

        if ((mvOccupied->BrakeHandle == FV4a) && (Console::Pressed(Global::Keys[k_DecBrakeLevel])))
        {
            if (Global::ctrlState)
            {
                mvOccupied->BrakeCtrlPos2 += (mvOccupied->BrakeCtrlPos2 > 2 ? 0 : dt / 20.0);
                if (mvOccupied->BrakeCtrlPos2 < -3)
                    mvOccupied->BrakeCtrlPos2 = -3;
            }
#ifdef EU07_USE_OLD_COMMAND_SYSTEM
            else
            {
                //        mvOccupied->BrakeCtrlPosR-=(mvOccupied->BrakeCtrlPosR<-1?0:dt*2);
                //        mvOccupied->BrakeCtrlPos=
                //        floor(mvOccupied->BrakeCtrlPosR+0.499);
                mvOccupied->BrakeLevelAdd(-dt * 2);
            }
#endif
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
                    play_sound( dsbPneumaticSwitch );
                }
            }
            else
            {
                if (mvOccupied->SwitchEPBrake(0))
                {
                    play_sound( dsbPneumaticSwitch );
                }
            }

#ifdef EU07_USE_OLD_COMMAND_SYSTEM
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
            if (Global::shiftState)
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
                        play_sound( dsbSwitch );
                        ShowNextCurrent = true;
                    }
                }
            }
            else
            {
                if (Global::shiftState)
                {
                    //     if (Console::Pressed(k_CurrentNext))
                    { // Ra: było pod GLFW_KEY_F3
                        if ((mvOccupied->EpFuseSwitch(true)))
                        {
                            play_sound( dsbPneumaticSwitch );
                        }
                    }
                }
                else
                {
                    //     if (Console::Pressed(k_CurrentNext))
                    { // Ra: było pod GLFW_KEY_F3
                        if (Global::ctrlState)
                        {
                            if ((mvOccupied->EpFuseSwitch(false)))
                            {
                                play_sound( dsbPneumaticSwitch );
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
                    play_sound( dsbSwitch );
                    ShowNextCurrent = false;
                }
            }
        }
        // Winger 010304  PantAllDownButton
        if (Console::Pressed(Global::Keys[k_PantFrontUp]))
        {
            if (Global::shiftState)
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
            if (Global::shiftState)
                ggPantRearButton.PutValue(1);
            else
                ggPantFrontButtonOff.PutValue(1);
        }
        else if (mvControlled->PantSwitchType == "impulse")
        {
            ggPantRearButton.PutValue(0);
            ggPantFrontButtonOff.PutValue(0);
        }
#endif
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
        if( dsbHasler != nullptr ) {
            if( ( false == FreeFlyModeFlag ) && ( fTachoCount > maxtacho ) ) {
                dsbHasler->GetStatus( &stat );
                if( !( stat & DSBSTATUS_PLAYING ) )
                    play_sound( dsbHasler, DSBVOLUME_MAX, DSBPLAY_LOOPING );
            }
            else {
                if( ( true == FreeFlyModeFlag ) || ( fTachoCount < 1 ) ) {
                    dsbHasler->GetStatus( &stat );
                    if( stat & DSBSTATUS_PLAYING )
                        dsbHasler->Stop();
                }
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
        ggPantRearButtonOff.Update();
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
        ggHornLowButton.Update();
        ggHornHighButton.Update();
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
#ifdef EU07_USE_OLD_COMMAND_SYSTEM
        ggMainOffButton.UpdateValue(0);
        ggMainOnButton.UpdateValue(0);
        ggSecurityResetButton.UpdateValue(0);
        ggReleaserButton.UpdateValue(0);
        ggSandButton.UpdateValue(0);
        ggAntiSlipButton.UpdateValue(0);
        ggDepartureSignalButton.UpdateValue( 0 );
        ggFuseButton.UpdateValue( 0 );
        ggConverterFuseButton.UpdateValue(0);
#endif
        pyScreens.update();
    }
    // wyprowadzenie sygnałów dla haslera na PoKeys (zaznaczanie na taśmie)
    btHaslerBrakes.Turn(DynamicObject->MoverParameters->BrakePress > 0.4); // ciśnienie w cylindrach
    btHaslerCurrent.Turn(DynamicObject->MoverParameters->Im != 0.0); // prąd na silnikach

    // calculate current level of interior illumination
    // TODO: organize it along with rest of train update in a more sensible arrangement
    switch( iCabLightFlag ) // Ra: uzeleżnic od napięcia w obwodzie sterowania
    { // hunter-091012: uzaleznienie jasnosci od przetwornicy
        case 0: {
            //światło wewnętrzne zgaszone
            DynamicObject->InteriorLightLevel = 0.0f;
            break;
        }
        case 1: {
            //światło wewnętrzne przygaszone (255 216 176)
            if( mvOccupied->ConverterFlag == true ) {
                // jasnosc dla zalaczonej przetwornicy
                DynamicObject->InteriorLightLevel = 0.4f;
            }
            else {
                DynamicObject->InteriorLightLevel = 0.2f;
            }
            break;
        }
        case 2: {
            //światło wewnętrzne zapalone (255 216 176)
            if( mvOccupied->ConverterFlag ==
                true ) // jasnosc dla zalaczonej przetwornicy
            {
                DynamicObject->InteriorLightLevel = 1.0f;
            }
            else {
                DynamicObject->InteriorLightLevel = 0.5f;
            }
            break;
        }
    }

    // catch cases where the power goes out, and the linebreaker state is left as closed
    if( ( m_linebreakerstate == 1 )
     && ( false == mvControlled->Mains )
     && ( ggMainButton.GetValue() < 0.05 ) ) {
        // crude way to catch cases where the main was knocked out and the user is trying to restart it
        // because the state of the line breaker isn't changed to match, we need to do it here manually
        m_linebreakerstate = 0;
    }

    // NOTE: crude way to have the pantographs go back up if they're dropped due to insufficient pressure etc
    // TODO: rework it into something more elegant, when redoing the whole consist/unit/cab etc arrangement
    if( ( mvControlled->Battery )
     || ( mvControlled->ConverterFlag ) ) {
        if( ( false == mvControlled->PantFrontUp )
         && ( ggPantFrontButton.GetValue() >= 1.0 ) ) {
            mvControlled->PantFront( true );
        }
        if( ( false == mvControlled->PantRearUp )
         && ( ggPantRearButton.GetValue() >= 1.0 ) ) {
            mvControlled->PantRear( true );
        }
    }
/*
    // NOTE: disabled while switch state isn't preserved while moving between compartments
    // check whether we should raise the pantographs, based on volume in pantograph tank
    if( mvControlled->PantPress > (
            mvControlled->TrainType == dt_EZT ?
                2.4 :
                3.5 ) ) {
        if( ( false == mvControlled->PantFrontUp )
         && ( ggPantFrontButton.GetValue() > 0.95 ) ) {
            mvControlled->PantFront( true );
        }
        if( ( false == mvControlled->PantRearUp )
         && ( ggPantRearButton.GetValue() > 0.95 ) ) {
            mvControlled->PantRear( true );
        }
    }
*/

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
        DynamicObject->mdKabina->Init(); // obrócenie modelu oraz optymalizacja, również zapisanie binarnego
        set_cab_controls();
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
    ggHornLowButton.Clear();
    ggHornHighButton.Clear();
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
    ggDimHeadlightsButton.Clear();
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

// NOTE: we can get rid of this function once we have per-cab persistent state
void TTrain::set_cab_controls() {
    // switches
    // battery
    if( true == mvOccupied->Battery ) {
        ggBatteryButton.PutValue( 1.0 );
    }
    // radio
    if( true == mvOccupied->Radio ) {
        ggRadioButton.PutValue( 1.0 );
    }
    // pantographs
    if( mvOccupied->PantSwitchType != "impulse" ) {
        ggPantFrontButton.PutValue(
            ( mvControlled->PantFrontUp ?
                1.0 :
                0.0 ) );
        ggPantFrontButtonOff.PutValue(
            ( mvControlled->PantFrontUp ?
                0.0 :
                1.0 ) );
    }
    if( mvOccupied->PantSwitchType != "impulse" ) {
        ggPantRearButton.PutValue(
            ( mvControlled->PantRearUp ?
                1.0 :
                0.0 ) );
        ggPantRearButtonOff.PutValue(
            ( mvControlled->PantRearUp ?
                0.0 :
                1.0 ) );
    }
    // converter
    if( mvOccupied->ConvSwitchType != "impulse" ) {
        ggConverterButton.PutValue(
            mvControlled->ConverterAllow ?
                1.0 :
                0.0 );
    }
    // compressor
    if( true == mvControlled->CompressorAllow ) {
        ggCompressorButton.PutValue( 1.0 );
    }
    // motor overload relay threshold / shunt mode
    if( mvControlled->Imax == mvControlled->ImaxHi ) {
        ggMaxCurrentCtrl.PutValue( 1.0 );
    }
    // lights
    int const lightsindex =
        ( mvOccupied->ActiveCab == 1 ?
        0 :
        1 );

    if( ( DynamicObject->iLights[ lightsindex ] & TMoverParameters::light::headlight_left ) != 0 ) {
        ggLeftLightButton.PutValue( 1.0 );
    }
    if( ( DynamicObject->iLights[ lightsindex ] & TMoverParameters::light::headlight_right ) != 0 ) {
        ggRightLightButton.PutValue( 1.0 );
    }
    if( ( DynamicObject->iLights[ lightsindex ] & TMoverParameters::light::headlight_upper ) != 0 ) {
        ggUpperLightButton.PutValue( 1.0 );
    }
    if( ( DynamicObject->iLights[ lightsindex ] & TMoverParameters::light::redmarker_left ) != 0 ) {
        ggLeftEndLightButton.PutValue( 1.0 );
    }
    if( ( DynamicObject->iLights[ lightsindex ] & TMoverParameters::light::redmarker_right ) != 0 ) {
        ggRightEndLightButton.PutValue( 1.0 );
    }
    if( true == DynamicObject->DimHeadlights ) {
        ggDimHeadlightsButton.PutValue( 1.0 );
    }
    // cab lights
    if( true == bCabLight ) {
        ggCabLightButton.PutValue( 1.0 );
    }
    if( true == bCabLightDim ) {
        ggCabLightDimButton.PutValue( 1.0 );
    }
    if( true == LampkaUniversal3_st ) {
        ggUniversal3Button.PutValue( 1.0 );
    }
    // doors
    // NOTE: we're relying on the cab models to have switches reversed for the rear cab(?)
    ggDoorLeftButton.PutValue( mvOccupied->DoorLeftOpened ? 1.0 : 0.0 );
    ggDoorRightButton.PutValue( mvOccupied->DoorRightOpened ? 1.0 : 0.0 );
    // door lock
    if( true == mvControlled->DoorSignalling ) {
        ggDoorSignallingButton.PutValue( 1.0 );
    }
    // heating
    if( true == mvControlled->Heating ) {
        ggTrainHeatingButton.PutValue( 1.0 );
    }
    // brake acting time
    if( ggBrakeProfileCtrl.SubModel != nullptr ) {
        ggBrakeProfileCtrl.PutValue(
            ( ( mvOccupied->BrakeDelayFlag & bdelay_R ) != 0 ?
                2.0 :
                mvOccupied->BrakeDelayFlag - 1 ) );
    }
    if( ggBrakeProfileG.SubModel != nullptr ) {
        ggBrakeProfileG.PutValue(
            mvOccupied->BrakeDelayFlag == bdelay_G ?
                1.0 :
                0.0 );
    }
    if( ggBrakeProfileR.SubModel != nullptr ) {
        ggBrakeProfileR.PutValue(
            ( mvOccupied->BrakeDelayFlag & bdelay_R ) != 0 ?
                1.0 :
                0.0 );
    }
    // brake signalling
    ggSignallingButton.PutValue(
        mvControlled->Signalling ?
            1.0 :
            0.0 );
    // multiple-unit current indicator source
    ggNextCurrentButton.PutValue(
        ShowNextCurrent ?
            1.0 :
            0.0 );
    
    // we reset all indicators, as they're set during the update pass
    // TODO: when cleaning up break setting indicator state into a separate function, so we can reuse it
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
    else if( Label == "i-dynamicbrake:" ) {

        btLampkaED.Load( Parser, DynamicObject->mdKabina );
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
    else if( Label == "hornlow_bt:" ) {
        // dzwignia syreny
        ggHornLowButton.Load( Parser, DynamicObject->mdKabina );
    }
    else if( Label == "hornhigh_bt:" ) {
        // dzwignia syreny
        ggHornHighButton.Load( Parser, DynamicObject->mdKabina );
    }
    else if( Label == "fuse_bt:" )
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
    else if( Label == "dimheadlights_sw:" ) {
        // swiatlo
        ggDimHeadlightsButton.Load( Parser, DynamicObject->mdKabina );
    }
    else if( Label == "leftend_sw:" )
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
    else if( Label == "pantfrontoff_sw:" )
    {
        // patyk przedni w dol
        ggPantFrontButtonOff.Load(Parser, DynamicObject->mdKabina);
    }
    else if( Label == "pantrearoff_sw:" ) {
        // patyk przedni w dol
        ggPantRearButtonOff.Load( Parser, DynamicObject->mdKabina );
    }
    else if( Label == "pantalloff_sw:" )
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
    else if( Label == "battery_sw:" ) {

        ggBatteryButton.Load( Parser, DynamicObject->mdKabina );
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

void
TTrain::play_sound( PSound Sound, int const Volume, DWORD const Flags ) {

    if( Sound ) {

        Sound->SetCurrentPosition( 0 );
        Sound->SetVolume( Volume );
        Sound->Play( 0, 0, Flags );
        return;
    }
}

void
TTrain::play_sound( PSound Sound, PSound Fallbacksound, int const Volume, DWORD const Flags ) {

    if( Sound ) {

        play_sound( Sound, Volume, Flags );
        return;
    }
    if( Fallbacksound ) {

        play_sound( Fallbacksound, Volume, Flags );
        return;
    }
}

