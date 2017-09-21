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
#include "sound.h"

void
control_mapper::insert( TGauge const &Gauge, std::string const &Label ) {

    auto const submodel = Gauge.SubModel;
    if( submodel != nullptr ) {
        m_controlnames.emplace( submodel, Label );
    }
}

std::string
control_mapper::find( TSubModel const *Control ) const {

    auto const lookup = m_controlnames.find( Control );
    if( lookup != m_controlnames.end() ) {
        return lookup->second;
    }
    else {
        return "";
    }
}

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
/*
    iGaugesMax = 100; // 95 - trzeba pobierać to z pliku konfiguracyjnego
    ggList = new TGauge[iGaugesMax];
    iGauges = 0; // na razie nie są dodane
    iButtonsMax = 60; // 55 - trzeba pobierać to z pliku konfiguracyjnego
    btList = new TButton[iButtonsMax];
    iButtons = 0;
*/
}
/*
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
*/
void TCab::Load(cParser &Parser)
{
    // NOTE: clearing control tables here is bit of a crutch, imposed by current scheme of loading compartments anew on each cab change
    ggList.clear();
    btList.clear();

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
/*
    delete[] ggList;
    delete[] btList;
*/
};

TGauge &TCab::Gauge(int n)
{ // pobranie adresu obiektu aniomowanego ruchem
/*
    if (n < 0)
    { // rezerwacja wolnego
        ggList[iGauges].Clear();
        return ggList + iGauges++;
    }
    else if (n < iGauges)
        return ggList + n;
    return NULL;
*/
    if( n < 0 ) {
        ggList.emplace_back();
        return ggList.back();
    }
    else {
        return ggList[ n ];
    }
};

TButton &TCab::Button(int n)
{ // pobranie adresu obiektu animowanego wyborem 1 z 2
/*
    if (n < 0)
    { // rezerwacja wolnego
        return btList + iButtons++;
    }
    else if (n < iButtons)
        return btList + n;
    return NULL;
*/
    if( n < 0 ) {
        btList.emplace_back();
        return btList.back();
    }
    else {
        return btList[ n ];
    }
};

void TCab::Update()
{ // odczyt parametrów i ustawienie animacji submodelom
/*
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
*/
    for( auto &gauge : ggList ) {
        // animacje izometryczne
        gauge.UpdateValue(); // odczyt parametru i przeliczenie na kąt
        gauge.Update(); // ustawienie animacji
    }
    for( auto &button : btList ) {
        // animacje dwustanowe
        button.Update(); // odczyt parametru i wybór submodelu
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
    { user_command::manualbrakeincrease, &TTrain::OnCommand_manualbrakeincrease },
    { user_command::manualbrakedecrease, &TTrain::OnCommand_manualbrakedecrease },
    { user_command::alarmchaintoggle, &TTrain::OnCommand_alarmchaintoggle },
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
    { user_command::convertertogglelocal, &TTrain::OnCommand_convertertogglelocal },
    { user_command::converteroverloadrelayreset, &TTrain::OnCommand_converteroverloadrelayreset },
    { user_command::compressortoggle, &TTrain::OnCommand_compressortoggle },
    { user_command::compressortogglelocal, &TTrain::OnCommand_compressortogglelocal },
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
    { user_command::radiotoggle, &TTrain::OnCommand_radiotoggle },
    { user_command::radiostoptest, &TTrain::OnCommand_radiostoptest },
    { user_command::generictoggle0, &TTrain::OnCommand_generictoggle },
    { user_command::generictoggle1, &TTrain::OnCommand_generictoggle },
    { user_command::generictoggle2, &TTrain::OnCommand_generictoggle },
    { user_command::generictoggle3, &TTrain::OnCommand_generictoggle },
    { user_command::generictoggle4, &TTrain::OnCommand_generictoggle },
    { user_command::generictoggle5, &TTrain::OnCommand_generictoggle },
    { user_command::generictoggle6, &TTrain::OnCommand_generictoggle },
    { user_command::generictoggle7, &TTrain::OnCommand_generictoggle },
    { user_command::generictoggle8, &TTrain::OnCommand_generictoggle },
    { user_command::generictoggle9, &TTrain::OnCommand_generictoggle }
};

std::vector<std::string> const TTrain::fPress_labels = {

    "ch1:  ", "ch2:  ", "ch3:  ", "ch4:  ", "ch5:  ", "ch6:  ", "ch7:  ", "ch8:  ", "ch9:  ", "ch0:  "
};

TTrain::TTrain() {
/*
    Universal4Active = false;
*/
    ShowNextCurrent = false;
    // McZapkie-240302 - przyda sie do tachometru
    fTachoVelocity = 0;
    fTachoCount = 0;
    fPPress = fNPress = 0;

    // asMessage="";
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
    InstrumentLightActive = false; // ABu: 030405
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

	sound_man->destroy_sound(&dsbNastawnikJazdy);
	sound_man->destroy_sound(&dsbNastawnikBocz);
	sound_man->destroy_sound(&dsbRelay);
	sound_man->destroy_sound(&dsbPneumaticRelay);
	sound_man->destroy_sound(&dsbSwitch);
	sound_man->destroy_sound(&dsbPneumaticSwitch);
	sound_man->destroy_sound(&dsbReverserKey);
	sound_man->destroy_sound(&dsbCouplerAttach);
	sound_man->destroy_sound(&dsbCouplerDetach);
	sound_man->destroy_sound(&dsbDieselIgnition);
	sound_man->destroy_sound(&dsbDoorClose);
	sound_man->destroy_sound(&dsbDoorOpen);
	sound_man->destroy_sound(&dsbPantUp);
	sound_man->destroy_sound(&dsbPantDown);
	sound_man->destroy_sound(&dsbWejscie_na_bezoporow);
	sound_man->destroy_sound(&dsbWejscie_na_drugi_uklad);
	sound_man->destroy_sound(&rsBrake);
	sound_man->destroy_sound(&rsSlippery);
	sound_man->destroy_sound(&rsHiss);
	sound_man->destroy_sound(&rsHissU);
	sound_man->destroy_sound(&rsHissE);
	sound_man->destroy_sound(&rsHissX);
	sound_man->destroy_sound(&rsHissT);
	sound_man->destroy_sound(&rsSBHiss);
	sound_man->destroy_sound(&rsRunningNoise);
	sound_man->destroy_sound(&rsEngageSlippery);
	sound_man->destroy_sound(&rsFadeSound);
	sound_man->destroy_sound(&dsbHasler);
	sound_man->destroy_sound(&dsbBuzzer);
	sound_man->destroy_sound(&dsbSlipAlarm);
	sound_man->destroy_sound(&dsbCouplerStretch);
	sound_man->destroy_sound(&dsbEN57_CouplerStretch);
	sound_man->destroy_sound(&dsbBufferClamp);
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
    pMechOffset = vector3( 0, 0, 0 );
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
    PyDict_SetItemString( dict, "universal3", PyGetBool( InstrumentLightActive ) );
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
    const char* TXTT[ 10 ] = { "fd", "fdt", "fdb", "pd", "pdt", "pdb", "itothv", "1", "2", "3" };
    const char* TXTC[ 10 ] = { "fr", "frt", "frb", "pr", "prt", "prb", "im", "vm", "ihv", "uhv" };
    const char* TXTP[ 3 ] = { "bc", "bp", "sp" };
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
        Train->mvControlled->IncMainCtrl( 1 );
    }
}

void TTrain::OnCommand_mastercontrollerincreasefast( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // on press or hold
        Train->mvControlled->IncMainCtrl( 2 );
    }
}

void TTrain::OnCommand_mastercontrollerdecrease( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // on press or hold
        Train->mvControlled->DecMainCtrl( 1 );
    }
}

void TTrain::OnCommand_mastercontrollerdecreasefast( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // on press or hold
        Train->mvControlled->DecMainCtrl( 2 );
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
        else {
            Train->mvControlled->IncScndCtrl( 1 );
        }
    }
}

void TTrain::OnCommand_secondcontrollerincreasefast( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // on press or hold
        Train->mvControlled->IncScndCtrl( 2 );
    }
}

void TTrain::OnCommand_notchingrelaytoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->mvOccupied->AutoRelayFlag ) {
            // turn on
            Train->mvOccupied->AutoRelaySwitch( true );
        }
        else {
            //turn off
            Train->mvOccupied->AutoRelaySwitch( false );
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
        // visual feedback
        Train->ggNextCurrentButton.UpdateValue( 1.0, Train->dsbSwitch );
    }
    else if( Command.action == GLFW_RELEASE ) {
        //turn off
        Train->ShowNextCurrent = false;
        // visual feedback
        Train->ggNextCurrentButton.UpdateValue( 0.0, Train->dsbSwitch );
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
        Train->mvControlled->DecScndCtrl( 1 );
    }
}

void TTrain::OnCommand_secondcontrollerdecreasefast( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // on press or hold
        Train->mvControlled->DecScndCtrl( 2 );
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
            // visual feedback
            Train->ggReleaserButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            // release
            Train->mvOccupied->BrakeReleaser( 0 );
            // visual feedback
            Train->ggReleaserButton.UpdateValue( 0.0, Train->dsbSwitch );
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
         && ( Global::iFeedbackMode != 4 ) ) {
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
         && ( Global::iFeedbackMode != 4 ) ) {
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

    if( Command.action == GLFW_PRESS ) {

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

    if( Command.action == GLFW_PRESS ) {

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

    if( Command.action == GLFW_PRESS ) {

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

    if( Command.action == GLFW_PRESS ) {

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

    if( Command.action == GLFW_PRESS ) {

        Train->mvOccupied->BrakeLevelSet( Train->mvOccupied->Handle->GetPos( bh_EB ) );
/*
        if( Train->mvOccupied->BrakeCtrlPosNo <= 0.1 ) {
            // hamulec bezpieczeństwa dla wagonów
            Train->mvOccupied->RadioStopFlag = true;
        }
*/
    }
}

void TTrain::OnCommand_manualbrakeincrease( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {

        if( ( Train->mvOccupied->LocalBrake == ManualBrake )
         || ( Train->mvOccupied->MBrake == true ) ) {

            Train->mvOccupied->IncManualBrakeLevel( 1 );
        }
    }
}

void TTrain::OnCommand_manualbrakedecrease( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {

        if( ( Train->mvOccupied->LocalBrake == ManualBrake )
         || ( Train->mvOccupied->MBrake == true ) ) {

            Train->mvOccupied->DecManualBrakeLevel( 1 );
        }
    }
}

void TTrain::OnCommand_alarmchaintoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

        if( false == Train->mvOccupied->AlarmChainFlag ) {
            // pull
            Train->mvOccupied->AlarmChainSwitch( true );
            // visual feedback
            Train->ggAlarmChain.UpdateValue( 1.0 );
        }
        else {
            // release
            Train->mvOccupied->AlarmChainSwitch( false );
            // visual feedback
            Train->ggAlarmChain.UpdateValue( 0.0 );
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
        // visual feedback
        Train->ggAntiSlipButton.UpdateValue( 1.0, Train->dsbSwitch );
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
        // visual feedback
        Train->ggSandButton.UpdateValue( 1.0, Train->dsbSwitch );
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
            // visual feedback
            if( Train->ggBrakeProfileCtrl.SubModel != nullptr ) {
                Train->ggBrakeProfileCtrl.UpdateValue(
                    ( ( Train->mvOccupied->BrakeDelayFlag & bdelay_R ) != 0 ?
                        2.0 :
                        Train->mvOccupied->BrakeDelayFlag - 1 ),
                    Train->dsbSwitch );
            }
            if( Train->ggBrakeProfileG.SubModel != nullptr ) {
                Train->ggBrakeProfileG.UpdateValue(
                    ( Train->mvOccupied->BrakeDelayFlag == bdelay_G ?
                        1.0 :
                        0.0 ),
                    Train->dsbSwitch );
            }
            if( Train->ggBrakeProfileR.SubModel != nullptr ) {
                Train->ggBrakeProfileR.UpdateValue(
                    ( ( Train->mvOccupied->BrakeDelayFlag & bdelay_R ) != 0 ?
                        1.0 :
                        0.0 ),
                    Train->dsbSwitch );
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
            // visual feedback
            if( Train->ggBrakeProfileCtrl.SubModel != nullptr ) {
                Train->ggBrakeProfileCtrl.UpdateValue(
                    ( ( Train->mvOccupied->BrakeDelayFlag & bdelay_R ) != 0 ?
                        2.0 :
                        Train->mvOccupied->BrakeDelayFlag - 1 ),
                    Train->dsbSwitch );
            }
            if( Train->ggBrakeProfileG.SubModel != nullptr ) {
                Train->ggBrakeProfileG.UpdateValue(
                    ( Train->mvOccupied->BrakeDelayFlag == bdelay_G ?
                        1.0 :
                        0.0 ),
                    Train->dsbSwitch );
            }
            if( Train->ggBrakeProfileR.SubModel != nullptr ) {
                Train->ggBrakeProfileR.UpdateValue(
                    ( ( Train->mvOccupied->BrakeDelayFlag & bdelay_R ) != 0 ?
                        1.0 :
                        0.0 ),
                    Train->dsbSwitch );
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
            // visual feedback
            Train->ggSignallingButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            //turn off
            Train->mvControlled->Signalling = false;
            // visual feedback
            Train->ggSignallingButton.UpdateValue( 0.0, Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_reverserincrease( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

        if( Train->mvOccupied->DirectionForward() ) {
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
        Train->ggSecurityResetButton.UpdateValue( 1.0, Train->dsbSwitch );
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

void TTrain::OnCommand_batterytoggle( TTrain *Train, command_data const &Command )
{
    if (Command.desired_state == command_data::ON && Train->mvOccupied->Battery)
        return;
    if (Command.desired_state == command_data::OFF && !Train->mvOccupied->Battery)
        return;

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->mvOccupied->Battery ) {
            // turn on
            // wyłącznik jest też w SN61, ewentualnie załączać prąd na stałe z poziomu FIZ
            if( Train->mvOccupied->BatterySwitch( true ) ) {
                // bateria potrzebna np. do zapalenia świateł
                if( Train->ggBatteryButton.SubModel ) {
                    Train->ggBatteryButton.UpdateValue( 1.0, Train->dsbSwitch );
                }
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
                    Train->ggBatteryButton.UpdateValue( 0.0, Train->dsbSwitch );
                }
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

void TTrain::OnCommand_pantographtogglefront( TTrain *Train, command_data const &cmd )
{
    command_data Command = cmd;

    if (Train->mvOccupied->PantSwitchType != "impulse")
    {
        if (Command.desired_state == Command.ON && Train->mvControlled->PantFrontUp)
            return;
        if (Command.desired_state == Command.OFF && !Train->mvControlled->PantFrontUp)
            return;
    }
    else if (Command.desired_state == Command.OFF)
        Command.action = GLFW_RELEASE;

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->mvControlled->PantFrontUp ) {
            // turn on...
            Train->mvControlled->PantFrontSP = false;
            if( Train->mvControlled->PantFront( true ) ) {
                if( Train->mvControlled->PantFrontStart != 1 ) {
                    // visual feedback
                    if( Train->ggPantFrontButton.SubModel )
                        Train->ggPantFrontButton.UpdateValue( 1.0, Train->dsbSwitch );
                    // NOTE: currently we animate the selectable pantograph control based on standard key presses
                    // TODO: implement actual selection control, and refactor handling this control setup in a separate method
                    if( Train->ggPantSelectedButton.SubModel )
                        Train->ggPantSelectedButton.UpdateValue( 1.0, Train->dsbSwitch );
                    // pantograph control can have two-button setup
                    if( Train->ggPantFrontButtonOff.SubModel )
                        Train->ggPantFrontButtonOff.UpdateValue( 0.0, Train->dsbSwitch );
                    // NOTE: currently we animate the selectable pantograph control based on standard key presses
                    // TODO: implement actual selection control, and refactor handling this control setup in a separate method
                    if( Train->ggPantSelectedDownButton.SubModel )
                        Train->ggPantSelectedDownButton.UpdateValue( 0.0, Train->dsbSwitch );
                }
            }
        }
        else {
            // ...or turn off
            if( Train->mvOccupied->PantSwitchType == "impulse" ) {
                if( ( Train->ggPantFrontButtonOff.SubModel == nullptr )
                 && ( Train->ggPantSelectedDownButton.SubModel == nullptr ) ) {
                   // with impulse buttons we expect a dedicated switch to lower the pantograph, and if the cabin lacks it
                   // then another control has to be used (like pantographlowerall)
                   // TODO: we should have a way to define presense of cab controls without having to bind these to 3d submodels
                    return;
                }
            }

            Train->mvControlled->PantFrontSP = false;
            if( false == Train->mvControlled->PantFront( false ) ) {
                if( Train->mvControlled->PantFrontStart != 0 ) {
                    // visual feedback
                    if( Train->ggPantFrontButton.SubModel )
                        Train->ggPantFrontButton.UpdateValue( 0.0, Train->dsbSwitch );
                    // NOTE: currently we animate the selectable pantograph control based on standard key presses
                    // TODO: implement actual selection control, and refactor handling this control setup in a separate method
                    if( Train->ggPantSelectedButton.SubModel )
                        Train->ggPantSelectedButton.UpdateValue( 0.0, Train->dsbSwitch );
                    // pantograph control can have two-button setup
                    if( Train->ggPantFrontButtonOff.SubModel )
                        Train->ggPantFrontButtonOff.UpdateValue( 1.0, Train->dsbSwitch );
                    // NOTE: currently we animate the selectable pantograph control based on standard key presses
                    // TODO: implement actual selection control, and refactor handling this control setup in a separate method
                    if( Train->ggPantSelectedDownButton.SubModel )
                        Train->ggPantSelectedDownButton.UpdateValue( 1.0, Train->dsbSwitch );
                }
            }
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // impulse switches return automatically to neutral position
        if( Train->mvOccupied->PantSwitchType == "impulse" ) {
            if( Train->ggPantFrontButton.SubModel )
                Train->ggPantFrontButton.UpdateValue( 0.0, Train->dsbSwitch );
            // NOTE: currently we animate the selectable pantograph control based on standard key presses
            // TODO: implement actual selection control, and refactor handling this control setup in a separate method
            if( Train->ggPantSelectedButton.SubModel )
                Train->ggPantSelectedButton.UpdateValue( 0.0, Train->dsbSwitch );
            // also the switch off button, in cabs which have it
            if( Train->ggPantFrontButtonOff.SubModel )
                Train->ggPantFrontButtonOff.UpdateValue( 0.0, Train->dsbSwitch );
            if( Train->ggPantSelectedDownButton.SubModel ) {
                // NOTE: currently we animate the selectable pantograph control based on standard key presses
                // TODO: implement actual selection control, and refactor handling this control setup in a separate method
                Train->ggPantSelectedDownButton.UpdateValue( 0.0, Train->dsbSwitch );
            }
        }
    }
}

void TTrain::OnCommand_pantographtogglerear( TTrain *Train, command_data const &cmd ) {
    command_data Command = cmd;

    if (Train->mvOccupied->PantSwitchType != "impulse")
    {
        if (Command.desired_state == Command.ON && Train->mvControlled->PantRearUp)
            return;
        if (Command.desired_state == Command.OFF && !Train->mvControlled->PantRearUp)
            return;
    }
    else if (Command.desired_state == Command.OFF)
        Command.action = GLFW_RELEASE;

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->mvControlled->PantRearUp ) {
            // turn on...
            Train->mvControlled->PantRearSP = false;
            if( Train->mvControlled->PantRear( true ) ) {
                if( Train->mvControlled->PantRearStart != 1 ) {
                    // visual feedback
                    if( Train->ggPantRearButton.SubModel )
                        Train->ggPantRearButton.UpdateValue( 1.0, Train->dsbSwitch );
                    // NOTE: currently we animate the selectable pantograph control based on standard key presses
                    // TODO: implement actual selection control, and refactor handling this control setup in a separate method
                    if( Train->ggPantSelectedButton.SubModel )
                        Train->ggPantSelectedButton.UpdateValue( 1.0, Train->dsbSwitch );
                    // pantograph control can have two-button setup
                    if( Train->ggPantRearButtonOff.SubModel )
                        Train->ggPantRearButtonOff.UpdateValue( 0.0, Train->dsbSwitch );
                    // NOTE: currently we animate the selectable pantograph control based on standard key presses
                    // TODO: implement actual selection control, and refactor handling this control setup in a separate method
                    if( Train->ggPantSelectedDownButton.SubModel )
                        Train->ggPantSelectedDownButton.UpdateValue( 0.0, Train->dsbSwitch );
                }
            }
        }
        else {
            // ...or turn off
            if( Train->mvOccupied->PantSwitchType == "impulse" ) {
                if( ( Train->ggPantRearButtonOff.SubModel == nullptr )
                 && ( Train->ggPantSelectedDownButton.SubModel == nullptr ) ) {
                    // with impulse buttons we expect a dedicated switch to lower the pantograph, and if the cabin lacks it
                    // then another control has to be used (like pantographlowerall)
                    // TODO: we should have a way to define presense of cab controls without having to bind these to 3d submodels
                    return;
                }
            }

            Train->mvControlled->PantRearSP = false;
            if( false == Train->mvControlled->PantRear( false ) ) {
                if( Train->mvControlled->PantRearStart != 0 ) {
                    // visual feedback
                    if( Train->ggPantRearButton.SubModel )
                        Train->ggPantRearButton.UpdateValue( 0.0, Train->dsbSwitch );
                    // NOTE: currently we animate the selectable pantograph control based on standard key presses
                    // TODO: implement actual selection control, and refactor handling this control setup in a separate method
                    if( Train->ggPantSelectedButton.SubModel )
                        Train->ggPantSelectedButton.UpdateValue( 0.0, Train->dsbSwitch );
                    // pantograph control can have two-button setup
                    if( Train->ggPantRearButtonOff.SubModel )
                        Train->ggPantRearButtonOff.UpdateValue( 1.0, Train->dsbSwitch );
                    // NOTE: currently we animate the selectable pantograph control based on standard key presses
                    // TODO: implement actual selection control, and refactor handling this control setup in a separate method
                    if( Train->ggPantSelectedDownButton.SubModel )
                        Train->ggPantSelectedDownButton.UpdateValue( 1.0, Train->dsbSwitch );
                }
            }
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // impulse switches return automatically to neutral position
        if( Train->mvOccupied->PantSwitchType == "impulse" ) {
            if( Train->ggPantRearButton.SubModel )
                Train->ggPantRearButton.UpdateValue( 0.0, Train->dsbSwitch );
            // NOTE: currently we animate the selectable pantograph control based on standard key presses
            // TODO: implement actual selection control, and refactor handling this control setup in a separate method
            if( Train->ggPantSelectedButton.SubModel )
                Train->ggPantSelectedButton.UpdateValue( 0.0, Train->dsbSwitch );
            // also the switch off button, in cabs which have it
            if( Train->ggPantRearButtonOff.SubModel )
                Train->ggPantRearButtonOff.UpdateValue( 0.0, Train->dsbSwitch );
            if( Train->ggPantSelectedDownButton.SubModel ) {
                // NOTE: currently we animate the selectable pantograph control based on standard key presses
                // TODO: implement actual selection control, and refactor handling this control setup in a separate method
                Train->ggPantSelectedDownButton.UpdateValue( 0.0, Train->dsbSwitch );
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
            // visual feedback:
            Train->ggPantCompressorValve.UpdateValue( 0.0 );
        }
        else {
            // connect pantograps with pantograph compressor
            Train->mvControlled->bPantKurek3 = false;
            // visual feedback:
            Train->ggPantCompressorValve.UpdateValue( 1.0 );
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
        // visual feedback:
        Train->ggPantCompressorButton.UpdateValue( 1.0 );
    }
    else {
        // release to disable
        Train->mvControlled->PantCompFlag = false;
        // visual feedback:
        Train->ggPantCompressorButton.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_pantographlowerall( TTrain *Train, command_data const &Command ) {

    if( ( Train->ggPantAllDownButton.SubModel == nullptr )
     && ( Train->ggPantSelectedDownButton.SubModel == nullptr ) ) {
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
        // visual feedback
        if( Train->ggPantAllDownButton.SubModel )
            Train->ggPantAllDownButton.UpdateValue( 1.0, Train->dsbSwitch );
        if( Train->ggPantSelectedDownButton.SubModel ) {
            Train->ggPantSelectedDownButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // release the button
        // visual feedback
        if( Train->ggPantAllDownButton.SubModel )
            Train->ggPantAllDownButton.UpdateValue( 0.0 );
        if( Train->ggPantSelectedDownButton.SubModel ) {
            Train->ggPantSelectedDownButton.UpdateValue( 0.0 );
        }
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
        if ((Command.desired_state == command_data::TOGGLE && Train->m_linebreakerstate == 0)
                || Command.desired_state == command_data::ON) {
            // ...to close the circuit
            if( Command.action == GLFW_PRESS ) {
                // fresh press, start fresh closing delay calculation
                Train->fMainRelayTimer = 0.0f;
            }
            if( Train->ggMainOnButton.SubModel != nullptr ) {
                // two separate switches to close and break the circuit
                // visual feedback
                Train->ggMainOnButton.UpdateValue( 1.0, Train->dsbSwitch );
            }
            else if( Train->ggMainButton.SubModel != nullptr ) {
                // single two-state switch
                // visual feedback
                Train->ggMainButton.UpdateValue( 1.0, Train->dsbSwitch );
            }
            // keep track of period the button is held down, to determine when/if circuit closes
            if( ( ( ( Train->mvControlled->EngineType != ElectricSeriesMotor )
                 && ( Train->mvControlled->EngineType != ElectricInductionMotor ) ) )
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
        else if ((Command.desired_state == command_data::TOGGLE && Train->m_linebreakerstate == 1)
                || Command.desired_state == command_data::OFF) {
            // ...to open the circuit
            Train->mvControlled->MainSwitch( false );

            Train->m_linebreakerstate = -1;

            if( Train->ggMainOffButton.SubModel != nullptr ) {
                // two separate switches to close and break the circuit
                // visual feedback
                Train->ggMainOffButton.UpdateValue( 1.0, Train->dsbSwitch );
            }
            else if( Train->ggMainButton.SubModel != nullptr ) {
                // single two-state switch
                {
                    // visual feedback
                    Train->ggMainButton.UpdateValue( 0.0, Train->dsbSwitch );
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
                Train->ggMainOnButton.UpdateValue( 0.0, Train->dsbSwitch );
            }
            if( Train->ggMainOffButton.SubModel != nullptr ) {
                Train->ggMainOffButton.UpdateValue( 0.0, Train->dsbSwitch );
            }
            // and the two-state switch too, for good measure
            if( Train->ggMainButton.SubModel != nullptr ) {
                // visual feedback
                Train->ggMainButton.UpdateValue( 0.0, Train->dsbSwitch );
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
                    Train->mvControlled->ConverterSwitch( ( Train->ggConverterButton.GetValue() > 0.5 ) || ( Train->mvControlled->ConverterStart == start::automatic ) );
                    Train->mvControlled->CompressorSwitch( Train->ggCompressorButton.GetValue() > 0.5 );
                }
            }
            // visual feedback
            if( Train->ggMainOnButton.SubModel != nullptr ) {
                // setup with two separate switches
                Train->ggMainOnButton.UpdateValue( 0.0, Train->dsbSwitch );
            }
            // NOTE: we don't have switch type definition for the line breaker switch
            // so for the time being we have hard coded "impulse" switches for all EMUs
            // TODO: have proper switch type config for all switches, and put it in the cab switch descriptions, not in the .fiz
            if( Train->mvControlled->TrainType == dt_EZT ) {
                if( Train->ggMainButton.SubModel != nullptr ) {
                    // visual feedback
                    Train->ggMainButton.UpdateValue( 0.0, Train->dsbSwitch );
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

    if( Train->mvControlled->ConverterStart == start::automatic ) {
        // let the automatic thing do its automatic thing...
        return;
    }

    if (Train->mvControlled->ConverterAllow && Command.desired_state == command_data::ON)
        return;
    if (!Train->mvControlled->ConverterAllow && Command.desired_state == command_data::OFF)
        return;

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( false == Train->mvControlled->ConverterAllow )
         && ( Train->ggConverterButton.GetValue() < 0.5 ) ) {
            // turn on
            // visual feedback
            Train->ggConverterButton.UpdateValue( 1.0, Train->dsbSwitch );
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
            // visual feedback
            Train->ggConverterButton.UpdateValue( 0.0, Train->dsbSwitch );
            if( Train->ggConverterOffButton.SubModel != nullptr ) {
                Train->ggConverterOffButton.UpdateValue( 1.0, Train->dsbSwitch );
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
            Train->ggConverterButton.UpdateValue( 0.0, Train->dsbSwitch );
            Train->ggConverterOffButton.UpdateValue( 0.0, Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_convertertogglelocal( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->ConverterStart == start::automatic ) {
        // let the automatic thing do its automatic thing...
        return;
    }
    if( Train->ggConverterLocalButton.SubModel == nullptr ) {
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( false == Train->mvOccupied->ConverterAllowLocal )
         && ( Train->ggConverterLocalButton.GetValue() < 0.5 ) ) {
            // turn on
            // visual feedback
            Train->ggConverterLocalButton.UpdateValue( 1.0, Train->dsbSwitch );
            // effect
            Train->mvOccupied->ConverterAllowLocal = true;
/*
            if( true == Train->mvControlled->ConverterSwitch( true, range::local ) ) {
                // side effects
                // control the compressor, if it's paired with the converter
                if( Train->mvControlled->CompressorPower == 2 ) {
                    // hunter-091012: tak jest poprawnie
                    Train->mvControlled->CompressorSwitch( true, range::local );
                }
            }
*/
        }
        else {
            //turn off
            // visual feedback
            Train->ggConverterLocalButton.UpdateValue( 0.0, Train->dsbSwitch );
            // effect
            Train->mvOccupied->ConverterAllowLocal = false;
/*
            if( true == Train->mvControlled->ConverterSwitch( false, range::local ) ) {
                // side effects
                // control the compressor, if it's paired with the converter
                if( Train->mvControlled->CompressorPower == 2 ) {
                    // hunter-091012: tak jest poprawnie
                    Train->mvControlled->CompressorSwitch( false, range::local );
                }
                // if there's no (low voltage) power source left, drop pantographs
                if( false == Train->mvControlled->Battery ) {
                    Train->mvControlled->PantFront( false, range::local );
                    Train->mvControlled->PantRear( false, range::local );
                }
            }
*/
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
        // visual feedback
        Train->ggConverterFuseButton.UpdateValue( 1.0, Train->dsbSwitch );
    }
    else if( Command.action == GLFW_RELEASE ) {
        // release
        // visual feedback
        Train->ggConverterFuseButton.UpdateValue( 0.0, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_compressortoggle( TTrain *Train, command_data const &Command ) {

    if( Train->mvControlled->CompressorPower >= 2 ) {
        return;
    }

    if (Train->mvControlled->CompressorAllow && Command.desired_state == command_data::ON)
        return;
    if (!Train->mvControlled->CompressorAllow && Command.desired_state == command_data::OFF)
        return;

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->mvControlled->CompressorAllow ) {
            // turn on
            // visual feedback
            Train->ggCompressorButton.UpdateValue( 1.0, Train->dsbSwitch );
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
                // NOTE: we don't have switch type definition for the compresor switch
                // so for the time being we have hard coded "impulse" switches for all EMUs
                // TODO: have proper switch type config for all switches, and put it in the cab switch descriptions, not in the .fiz
//                if( mvControlled->TrainType == dt_EZT ) {
//                    // visual feedback
//                    ggCompressorButton.UpdateValue( 1.0 );
//                }
//                else {
                    // visual feedback
                    Train->ggCompressorButton.UpdateValue( 0.0, Train->dsbSwitch );
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

void TTrain::OnCommand_compressortogglelocal( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->CompressorPower >= 2 ) {
        return;
    }
    if( Train->ggCompressorLocalButton.SubModel == nullptr ) {
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->mvOccupied->CompressorAllowLocal ) {
            // turn on
            // visual feedback
            Train->ggCompressorLocalButton.UpdateValue( 1.0, Train->dsbSwitch );
            // effect
            Train->mvOccupied->CompressorAllowLocal = true;
        }
        else {
            // turn off
            // visual feedback
            Train->ggCompressorLocalButton.UpdateValue( 0.0, Train->dsbSwitch );
            // effect
            Train->mvOccupied->CompressorAllowLocal = false;
        }
    }
}

void TTrain::OnCommand_motorconnectorsopen( TTrain *Train, command_data const &cmd ) {
    command_data Command = cmd;
    // TODO: don't rely on presense of 3d model to determine presence of the switch
    if( Train->ggStLinOffButton.SubModel == nullptr ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Open Motor Power Connectors button is missing, or wasn't defined" );
        }
        return;
    }

    if (Train->mvControlled->StLinSwitchType == "toggle")
    {
        if (Command.desired_state == Command.ON && Train->mvControlled->StLinSwitchOff)
            return;
        if (Command.desired_state == Command.OFF && !Train->mvControlled->StLinSwitchOff)
            return;
    }
    else if (Command.desired_state == Command.OFF)
        Command.action = GLFW_RELEASE;

    // NOTE: because we don't have modeled actual circuits this is a simplification of the real mechanics
    // namely, pressing the button will flip it in the entire unit, which isn't exactly physically possible
    if( Command.action == GLFW_PRESS ) {
        // button works while it's held down but we can only pay attention to initial press
        if( false == Train->mvControlled->StLinSwitchOff ) {
            // open the connectors
            Train->mvControlled->StLinSwitchOff = true;
            if( ( Train->mvControlled->TrainType == dt_ET41 )
             || ( Train->mvControlled->TrainType == dt_ET42 ) ) {
                // crude implementation of the butto affecting entire unit for multi-unit engines
                // TODO: rework it into part of standard command propagation system
                if( ( Train->mvControlled->Couplers[ 0 ].Connected != nullptr )
                 && ( true == TestFlag( Train->mvControlled->Couplers[ 0 ].CouplingFlag, coupling::permanent ) ) ) {
                    // the first unit isn't allowed to start its compressor until second unit can start its own as well
                    Train->mvControlled->Couplers[ 0 ].Connected->StLinSwitchOff = true;
                }
                if( ( Train->mvControlled->Couplers[ 1 ].Connected != nullptr )
                 && ( true == TestFlag( Train->mvControlled->Couplers[ 1 ].CouplingFlag, coupling::permanent ) ) ) {
                    // the first unit isn't allowed to start its compressor until second unit can start its own as well
                    Train->mvControlled->Couplers[ 1 ].Connected->StLinSwitchOff = true;
                }
            }
            // visual feedback
            Train->ggStLinOffButton.UpdateValue( 1.0, Train->dsbSwitch );
            // effect
            if( true == Train->mvControlled->StLinFlag ) {
                Train->play_sound( Train->dsbRelay );
            }
            // yBARC - zmienione na przeciwne, bo true to zalaczone
            Train->mvControlled->StLinFlag = false;
            if( ( Train->mvControlled->TrainType == dt_ET41 )
             || ( Train->mvControlled->TrainType == dt_ET42 ) ) {
                // crude implementation of the butto affecting entire unit for multi-unit engines
                // TODO: rework it into part of standard command propagation system
                if( ( Train->mvControlled->Couplers[ 0 ].Connected != nullptr )
                 && ( true == TestFlag( Train->mvControlled->Couplers[ 0 ].CouplingFlag, coupling::permanent ) ) ) {
                    // the first unit isn't allowed to start its compressor until second unit can start its own as well
                    Train->mvControlled->Couplers[ 0 ].Connected->StLinFlag = false;
                }
                if( ( Train->mvControlled->Couplers[ 1 ].Connected != nullptr )
                 && ( true == TestFlag( Train->mvControlled->Couplers[ 1 ].CouplingFlag, coupling::permanent ) ) ) {
                    // the first unit isn't allowed to start its compressor until second unit can start its own as well
                    Train->mvControlled->Couplers[ 1 ].Connected->StLinFlag = false;
                }
            }
        }
        else {
            if( Train->mvControlled->StLinSwitchType == "toggle" ) {
                // default type of button (impulse) has only one effect on press, but the toggle type can toggle the state
                Train->mvControlled->StLinSwitchOff = false;
                if( ( Train->mvControlled->TrainType == dt_ET41 )
                 || ( Train->mvControlled->TrainType == dt_ET42 ) ) {
                    // crude implementation of the butto affecting entire unit for multi-unit engines
                    // TODO: rework it into part of standard command propagation system
                    if( ( Train->mvControlled->Couplers[ 0 ].Connected != nullptr )
                     && ( true == TestFlag( Train->mvControlled->Couplers[ 0 ].CouplingFlag, coupling::permanent ) ) ) {
                        // the first unit isn't allowed to start its compressor until second unit can start its own as well
                        Train->mvControlled->Couplers[ 0 ].Connected->StLinSwitchOff = false;
                    }
                    if( ( Train->mvControlled->Couplers[ 1 ].Connected != nullptr )
                     && ( true == TestFlag( Train->mvControlled->Couplers[ 1 ].CouplingFlag, coupling::permanent ) ) ) {
                        // the first unit isn't allowed to start its compressor until second unit can start its own as well
                        Train->mvControlled->Couplers[ 1 ].Connected->StLinSwitchOff = false;
                    }
                }
                // visual feedback
                Train->ggStLinOffButton.UpdateValue( 0.0, Train->dsbSwitch );
            }
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // button released
        if( Train->mvControlled->StLinSwitchType != "toggle" ) {
            // default button type (impulse) works on button release
            Train->mvControlled->StLinSwitchOff = false;
            if( ( Train->mvControlled->TrainType == dt_ET41 )
             || ( Train->mvControlled->TrainType == dt_ET42 ) ) {
                // crude implementation of the butto affecting entire unit for multi-unit engines
                // TODO: rework it into part of standard command propagation system
                if( ( Train->mvControlled->Couplers[ 0 ].Connected != nullptr )
                 && ( true == TestFlag( Train->mvControlled->Couplers[ 0 ].CouplingFlag, coupling::permanent ) ) ) {
                    // the first unit isn't allowed to start its compressor until second unit can start its own as well
                    Train->mvControlled->Couplers[ 0 ].Connected->StLinSwitchOff = false;
                }
                if( ( Train->mvControlled->Couplers[ 1 ].Connected != nullptr )
                 && ( true == TestFlag( Train->mvControlled->Couplers[ 1 ].CouplingFlag, coupling::permanent ) ) ) {
                    // the first unit isn't allowed to start its compressor until second unit can start its own as well
                    Train->mvControlled->Couplers[ 1 ].Connected->StLinSwitchOff = false;
                }
            }
            // visual feedback
            Train->ggStLinOffButton.UpdateValue( 0.0, Train->dsbSwitch );
        }
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
        Train->mvControlled->CutOffEngine();
    }
}

void TTrain::OnCommand_motoroverloadrelaythresholdtoggle( TTrain *Train, command_data const &Command )
{
    if (Train->mvControlled->Imax < Train->mvControlled->ImaxHi && Command.desired_state == command_data::OFF)
        return;
    if (!(Train->mvControlled->Imax < Train->mvControlled->ImaxHi) && Command.desired_state == command_data::ON)
        return;

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( Train->mvControlled->Imax < Train->mvControlled->ImaxHi ) {
            // turn on
            if( true == Train->mvControlled->CurrentSwitch( true ) ) {
                // visual feedback
                Train->ggMaxCurrentCtrl.UpdateValue( 1.0, Train->dsbSwitch );
            }
        }
        else {
            //turn off
            if( true == Train->mvControlled->CurrentSwitch( false ) ) {
                // visual feedback
                Train->ggMaxCurrentCtrl.UpdateValue( 0.0, Train->dsbSwitch );
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
        // visual feedback
        Train->ggFuseButton.UpdateValue( 1.0, Train->dsbSwitch );
    }
    else if( Command.action == GLFW_RELEASE ) {
        // release
        // visual feedback
        Train->ggFuseButton.UpdateValue( 0.0, Train->dsbSwitch );
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

    bool current_state = (bool)(Train->DynamicObject->iLights[ lightsindex ] & TMoverParameters::light::headlight_left);

    if (Command.desired_state == command_data::ON && current_state)
        return;
    if (Command.desired_state == command_data::OFF && !current_state)
        return;

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( !current_state ) {
            // turn on
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::headlight_left;
            // visual feedback
            Train->ggLeftLightButton.UpdateValue( 1.0, Train->dsbSwitch );
            // if the light is controlled by 3-way switch, disable marker light
            if( Train->ggLeftEndLightButton.SubModel == nullptr ) {
                Train->DynamicObject->iLights[ lightsindex ] &= ~TMoverParameters::light::redmarker_left;
            }
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::headlight_left;
            // visual feedback
            Train->ggLeftLightButton.UpdateValue( 0.0, Train->dsbSwitch );
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

    bool current_state = (bool)(Train->DynamicObject->iLights[ lightsindex ] & TMoverParameters::light::headlight_right);

    if (Command.desired_state == command_data::ON && current_state)
        return;
    if (Command.desired_state == command_data::OFF && !current_state)
        return;

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( !current_state ) {
            // turn on
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::headlight_right;
            // visual feedback
            Train->ggRightLightButton.UpdateValue( 1.0, Train->dsbSwitch );
            // if the light is controlled by 3-way switch, disable marker light
            if( Train->ggRightEndLightButton.SubModel == nullptr ) {
                Train->DynamicObject->iLights[ lightsindex ] &= ~TMoverParameters::light::redmarker_right;
            }
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::headlight_right;
            // visual feedback
            Train->ggRightLightButton.UpdateValue( 0.0, Train->dsbSwitch );
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

    bool current_state = (bool)(Train->DynamicObject->iLights[ lightsindex ] & TMoverParameters::light::headlight_upper);

    if (Command.desired_state == command_data::ON && current_state)
        return;
    if (Command.desired_state == command_data::OFF && !current_state)
        return;

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( !current_state ) {
            // turn on
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::headlight_upper;
            // visual feedback
            Train->ggUpperLightButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::headlight_upper;
            // visual feedback
            Train->ggUpperLightButton.UpdateValue( 0.0, Train->dsbSwitch );
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
            if( Train->ggLeftEndLightButton.SubModel != nullptr ) {
                Train->ggLeftEndLightButton.UpdateValue( 1.0, Train->dsbSwitch );
            }
            else {
                // we interpret lack of dedicated switch as a sign the light is controlled with 3-way switch
                // this is crude, but for now will do
                Train->ggLeftLightButton.UpdateValue( -1.0, Train->dsbSwitch );
                // if the light is controlled by 3-way switch, disable the headlight
                Train->DynamicObject->iLights[ lightsindex ] &= ~TMoverParameters::light::headlight_left;
            }
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::redmarker_left;
            // visual feedback
            if( Train->ggLeftEndLightButton.SubModel != nullptr ) {
                Train->ggLeftEndLightButton.UpdateValue( 0.0, Train->dsbSwitch );
            }
            else {
                // we interpret lack of dedicated switch as a sign the light is controlled with 3-way switch
                // this is crude, but for now will do
                Train->ggLeftLightButton.UpdateValue( 0.0, Train->dsbSwitch );
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
            if( Train->ggRightEndLightButton.SubModel != nullptr ) {
                Train->ggRightEndLightButton.UpdateValue( 1.0, Train->dsbSwitch );
            }
            else {
                // we interpret lack of dedicated switch as a sign the light is controlled with 3-way switch
                // this is crude, but for now will do
                Train->ggRightLightButton.UpdateValue( -1.0, Train->dsbSwitch );
                // if the light is controlled by 3-way switch, disable the headlight
                Train->DynamicObject->iLights[ lightsindex ] &= ~TMoverParameters::light::headlight_right;
            }
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::redmarker_right;
            // visual feedback
            if( Train->ggRightEndLightButton.SubModel != nullptr ) {
                Train->ggRightEndLightButton.UpdateValue( 0.0, Train->dsbSwitch );
            }
            else {
                // we interpret lack of dedicated switch as a sign the light is controlled with 3-way switch
                // this is crude, but for now will do
                Train->ggRightLightButton.UpdateValue( 0.0, Train->dsbSwitch );
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
            Train->ggRearLeftLightButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::headlight_right;
            // visual feedback
            Train->ggRearLeftLightButton.UpdateValue( 0.0, Train->dsbSwitch );
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
            Train->ggRearRightLightButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::headlight_left;
            // visual feedback
            Train->ggRearRightLightButton.UpdateValue( 0.0, Train->dsbSwitch );
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
            Train->ggRearUpperLightButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::headlight_upper;
            // visual feedback
            Train->ggRearUpperLightButton.UpdateValue( 0.0, Train->dsbSwitch );
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
            Train->ggRearLeftEndLightButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::redmarker_right;
            // visual feedback
            Train->ggRearLeftEndLightButton.UpdateValue( 0.0, Train->dsbSwitch );
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
            Train->ggRearRightEndLightButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ lightsindex ] ^= TMoverParameters::light::redmarker_left;
            // visual feedback
            Train->ggRearRightEndLightButton.UpdateValue( 0.0, Train->dsbSwitch );
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

    if (Train->DynamicObject->DimHeadlights && Command.desired_state == command_data::ON)
        return;
    if (!Train->DynamicObject->DimHeadlights && Command.desired_state == command_data::OFF)
        return;

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->DynamicObject->DimHeadlights ) {
            // turn on
            Train->DynamicObject->DimHeadlights = true;
            // visual feedback
            Train->ggDimHeadlightsButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            //turn off
            Train->DynamicObject->DimHeadlights = false;
            // visual feedback
            Train->ggDimHeadlightsButton.UpdateValue( 0.0, Train->dsbSwitch );
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
            // visual feedback
            Train->ggCabLightButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            //turn off
            Train->bCabLight = false;
            Train->btCabLight.TurnOff();
            // visual feedback
            Train->ggCabLightButton.UpdateValue( 0.0, Train->dsbSwitch );
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

    if (Train->bCabLightDim && Command.desired_state == command_data::ON)
        return;
    if (!Train->bCabLightDim && Command.desired_state == command_data::OFF)
        return;

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->bCabLightDim ) {
            // turn on
            Train->bCabLightDim = true;
            // visual feedback
            Train->ggCabLightDimButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            //turn off
            Train->bCabLightDim = false;
            // visual feedback
            Train->ggCabLightDimButton.UpdateValue( 0.0, Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_instrumentlighttoggle( TTrain *Train, command_data const &Command ) {

    // NOTE: the check is disabled, as we're presuming light control is present in every vehicle
    // TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
    if( Train->ggInstrumentLightButton.SubModel == nullptr ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Instrument light switch is missing, or wasn't defined" );
        }
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        // NOTE: instrument lighting isn't fully implemented, so we have to rely on the state of the 'button' i.e. light itself
        if( false == Train->InstrumentLightActive ) {
            // turn on
            Train->InstrumentLightActive = true;
            // visual feedback
            Train->ggInstrumentLightButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            //turn off
            Train->InstrumentLightActive = false;
            // visual feedback
            Train->ggInstrumentLightButton.UpdateValue( 0.0, Train->dsbSwitch );
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

    if (Train->mvControlled->Heating && Command.desired_state == command_data::ON)
        return;
    if (!Train->mvControlled->Heating && Command.desired_state == command_data::OFF)
        return;

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->mvControlled->Heating ) {
            // turn on
            Train->mvControlled->Heating = true;
            // visual feedback
            Train->ggTrainHeatingButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            //turn off
            Train->mvControlled->Heating = false;
            // visual feedback
            Train->ggTrainHeatingButton.UpdateValue( 0.0, Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_generictoggle( TTrain *Train, command_data const &Command ) {

    auto const itemindex = static_cast<int>( Command.command ) - static_cast<int>( user_command::generictoggle0 );
    auto &item = Train->ggUniversals[ itemindex ];

    if( item.SubModel == nullptr ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Train generic item " + std::to_string( itemindex ) + " is missing, or wasn't defined" );
        }
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( item.GetValue() < 0.25 ) {
            // turn on
            // visual feedback
            item.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            // turn off
            // visual feedback
            item.UpdateValue( 0.0, Train->dsbSwitch );
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
            // visual feedback
            Train->ggDoorSignallingButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            // turn off
            // TODO: check wheter we really need separate flags for this
            Train->mvControlled->DoorSignalling = false;
            Train->mvOccupied->DoorBlocked = false;
            // visual feedback
            Train->ggDoorSignallingButton.UpdateValue( 0.0, Train->dsbSwitch );
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
                    Train->ggDoorLeftButton.UpdateValue( 1.0, Train->dsbSwitch );
                    Train->play_sound( Train->dsbDoorOpen );
                }
            }
            else {
                // in the rear cab sides are reversed
                if( Train->mvOccupied->DoorRight( true ) ) {
                    Train->ggDoorRightButton.UpdateValue( 1.0, Train->dsbSwitch );
                    Train->play_sound( Train->dsbDoorOpen );
                }
            }
        }
        else {
            // close
            if( Train->mvOccupied->ActiveCab == 1 ) {
                if( Train->mvOccupied->DoorLeft( false ) ) {
                    Train->ggDoorLeftButton.UpdateValue( 0.0, Train->dsbSwitch );
                    Train->play_sound( Train->dsbDoorClose );
                }
            }
            else {
                // in the rear cab sides are reversed
                if( Train->mvOccupied->DoorRight( false ) ) {
                    Train->ggDoorRightButton.UpdateValue( 0.0, Train->dsbSwitch );
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
                    Train->ggDoorRightButton.UpdateValue( 1.0, Train->dsbSwitch );
                    Train->play_sound( Train->dsbDoorOpen );
                }
            }
            else {
                // in the rear cab sides are reversed
                if( Train->mvOccupied->DoorLeft( true ) ) {
                    Train->ggDoorLeftButton.UpdateValue( 1.0, Train->dsbSwitch );
                    Train->play_sound( Train->dsbDoorOpen );
                }
            }
        }
        else {
            // close
            if( Train->mvOccupied->ActiveCab == 1 ) {
                if( Train->mvOccupied->DoorRight( false ) ) {
                    Train->ggDoorRightButton.UpdateValue( 0.0, Train->dsbSwitch );
                    Train->play_sound( Train->dsbDoorClose );
                }
            }
            else {
                // in the rear cab sides are reversed
                if( Train->mvOccupied->DoorLeft( false ) ) {
                    Train->ggDoorLeftButton.UpdateValue( 0.0, Train->dsbSwitch );
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
            // visual feedback
            Train->ggDepartureSignalButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // turn off
        Train->mvControlled->DepartureSignal = false;
        // visual feedback
        Train->ggDepartureSignalButton.UpdateValue( 0.0, Train->dsbSwitch );
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
/*
            if( true == TestFlag( Train->mvOccupied->WarningSignal, 2 ) ) {
                // low and high horn are treated as mutually exclusive
                Train->mvControlled->WarningSignal &= ~2;
            }
*/
            // visual feedback
            Train->ggHornButton.UpdateValue( -1.0 );
            Train->ggHornLowButton.UpdateValue( 1.0 );
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // turn off
/*
        // NOTE: we turn off both low and high horn, due to unreliability of release event when shift key is involved
        Train->mvOccupied->WarningSignal &= ~( 1 | 2 );
*/
        Train->mvOccupied->WarningSignal &= ~1;
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
/*
            if( true == TestFlag( Train->mvOccupied->WarningSignal, 1 ) ) {
                // low and high horn are treated as mutually exclusive
                Train->mvControlled->WarningSignal &= ~1;
            }
*/
            // visual feedback
            Train->ggHornButton.UpdateValue( 1.0 );
            Train->ggHornHighButton.UpdateValue( 1.0 );
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // turn off
/*
        // NOTE: we turn off both low and high horn, due to unreliability of release event when shift key is involved
        Train->mvOccupied->WarningSignal &= ~( 1 | 2 );
*/
        Train->mvOccupied->WarningSignal &= ~2;
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
            // visual feedback
            Train->ggRadioButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            // turn off
            Train->mvOccupied->Radio = false;
            // visual feedback
            Train->ggRadioButton.UpdateValue( 0.0, Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_radiostoptest( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        Train->Dynamic()->RadioStop();
    }
}

void TTrain::OnKeyDown(int cKey)
{ // naciśnięcie klawisza
    bool const isEztOer =
        ( ( mvControlled->TrainType == dt_EZT )
       && ( mvControlled->Battery == true )
       && ( mvControlled->EpFuse == true )
       && ( mvOccupied->BrakeSubsystem == ss_ESt )
       && ( mvControlled->ActiveDir != 0 ) ); // od yB

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
#ifdef EU07_USE_OLD_COMMAND_SYSTEM
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
                if (Global::ctrlState)
                    if ((mvOccupied->LocalBrake == ManualBrake) || (mvOccupied->MBrake == true))
                        mvOccupied->DecManualBrakeLevel(1);
                else // Ra 1014-06: AI potrafi zahamować pomocniczym mimo jego braku -
                    // odhamować jakoś trzeba
                    if ((mvOccupied->LocalBrake != ManualBrake) || mvOccupied->LocalBrakePos)
                    mvOccupied->DecLocalBrakeLevel(1);
            }
        }
        else
#endif
            if (cKey == Global::Keys[k_Brake2]) {
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

#ifdef _WIN32
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
#endif

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
                mvControlled->MainSwitch( false, ( mvControlled->TrainType == dt_EZT ? range::unit : range::local ) );
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
                        mvControlled->MainSwitch( false, ( mvControlled->TrainType == dt_EZT ? range::unit : range::local ) );
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
        double dfreq;
        glm::vec3 dynpos;
        {
        	vector3 v = DynamicObject->GetPosition();
        	dynpos = glm::make_vec3(&v.x);
        }

        // McZapkie-280302 - syczenie
        if ((mvOccupied->BrakeHandle == FV4a) || (mvOccupied->BrakeHandle == FVel6))
        {
            if (rsHiss) // upuszczanie z PG
            {
                fPPress = (1 * fPPress + mvOccupied->Handle->GetSound(s_fv4a_b)) / (2);
                if (fPPress > 0)
                {
                    vol = 2.0 * fPPress;
                }
                if (vol * rsHiss->gain_mul > 0.001)
                {
                	rsHiss->loop().gain(vol).position(dynpos).play();
                }
                else
                {
                    rsHiss->stop();
                }
            }
            if (rsHissU) // upuszczanie z PG
            {
                fNPress = (1 * fNPress + mvOccupied->Handle->GetSound(s_fv4a_u)) / (2);
                vol = 0.0;
                if (fNPress > 0)
                {
                    vol = fNPress;
                }
                if (vol * rsHissU->gain_mul > 0.001)
                {
                    rsHissU->loop().gain(vol).position(dynpos).play();
                }
                else
                {
                    rsHissU->stop();
                }
            }
            if (rsHissE) // upuszczanie przy naglym
            {
                vol = mvOccupied->Handle->GetSound(s_fv4a_e);
                if (vol * rsHissE->gain_mul > 0.001)
                {
                    rsHissE->loop().gain(vol).position(dynpos).play();
                }
                else
                {
                    rsHissE->stop();
                }
            }
            if (rsHissX) // upuszczanie sterujacego fala
            {
                vol = mvOccupied->Handle->GetSound(s_fv4a_x);
                if (vol * rsHissX->gain_mul > 0.001)
                {
                    rsHissX->loop().gain(vol).position(dynpos).play();
                }
                else
                {
                    rsHissX->stop();
                }
            }
            if (rsHissT) // upuszczanie z czasowego 
            {
                vol = mvOccupied->Handle->GetSound(s_fv4a_t);
                if (vol * rsHissT->gain_mul > 0.001)
                {
                    rsHissT->loop().gain(vol).position(dynpos).play();
                }
                else
                {
                    rsHissT->stop();
                }
            }

        } // koniec FV4a
        else // jesli nie FV4a
        {
            if (rsHiss) // upuszczanie z PG
            {
                fPPress = (4.0f * fPPress + std::max(mvOccupied->dpLocalValve, mvOccupied->dpMainValve)) / (4.0f + 1.0f);
                if (fPPress > 0.0f)
                {
                    vol = 2.0 * fPPress;
                }
                if (vol * rsHiss->gain_mul > 0.01)
                {
                    rsHiss->loop().gain(vol).position(dynpos).play();
                }
                else
                {
                    rsHiss->stop();
                }
            }
            if (rsHissU) // napelnianie PG
            {
                fNPress = (4.0f * fNPress + Min0R(mvOccupied->dpLocalValve, mvOccupied->dpMainValve)) / (4.0f + 1.0f);
                if (fNPress < 0.0f)
                {
                    vol = -1.0 * fNPress;
                }
                if (vol * rsHissU->gain_mul > 0.01)
                {
                    rsHissU->loop().gain(vol).position(dynpos).play();
                }
                else
                {
                    rsHissU->stop();
                }
            }
        } // koniec nie FV4a

        // Winger-160404 - syczenie pomocniczego (luzowanie)
        /*      if (rsSBHiss->gain_mul!=0)
               {
                  fSPPress=(mvOccupied->LocalBrakeRatio())-(mvOccupied->LocalBrakePos);
                  if (fSPPress>0)
                   {
                    vol=2*rsSBHiss->gain_mul*fSPPress;
                   }
                  if (vol>0.1)
                   {
                    rsSBHiss.Play(vol,DSBPLAY_LOOPING,true,DynamicObject->GetPosition());
                   }
                  else
                   {
                    rsSBHiss->stop();
                   }
               }
        */
        // szum w czasie jazdy
        vol = 0.0;
        dfreq = 1.0;
        if (rsRunningNoise)
        {
            if (DynamicObject->GetVelocity() != 0)
            {
                if (!TestFlag(mvOccupied->DamageFlag,
                              dtrain_wheelwear)) // McZpakie-221103: halas zalezny od kola
                {
                    dfreq = mvOccupied->Vel;
                    vol = mvOccupied->Vel;
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
                    dfreq = mvOccupied->Vel;
                    vol = mvOccupied->Vel;
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
                rsRunningNoise->pitch(dfreq).gain(vol).position(dynpos).loop().play();
            }
            else
                rsRunningNoise->stop();
        }

        if (rsBrake)
        {
            if ((!mvOccupied->SlippingWheels) && (mvOccupied->UnitBrakeForce > 10.0) &&
                (DynamicObject->GetVelocity() > 0.01))
            {
                //        vol=rsBrake->gain_off+rsBrake->gain_mul*(DynamicObject->GetVelocity()*100+mvOccupied->UnitBrakeForce);
                vol = sqrt((DynamicObject->GetVelocity() * mvOccupied->UnitBrakeForce));
                dfreq = DynamicObject->GetVelocity();
                rsBrake->pitch(dfreq).gain(vol).position(dynpos).loop().play();
            }
            else
            {
                rsBrake->stop();
            }
        }

        if (rsEngageSlippery)
        {
            if /*((fabs(mvControlled->dizel_engagedeltaomega)>0.2) && */ (
                mvControlled->dizel_engage > 0.1)
            {
                if (fabs(mvControlled->dizel_engagedeltaomega) > 0.2)
                {
                    dfreq = fabs(mvControlled->dizel_engagedeltaomega);
                    vol = mvControlled->dizel_engage;
                }
                else
                {
                    dfreq =
                        1; // rsEngageSlippery->pitch_off+0.7*rsEngageSlippery->pitch_mul*(fabs(mvControlled->enrot)+mvControlled->nmax);
                    if (mvControlled->dizel_engage > 0.2)
                        vol =
                            0.2 * (mvControlled->enrot / mvControlled->nmax);
                    else
                        vol = 0;
                }
                rsEngageSlippery->pitch(dfreq).gain(vol).position(dynpos).loop().play();
            }
            else
            {
                rsEngageSlippery->stop();
            }
        }

        if (rsFadeSound)
        {
	        if (FreeFlyModeFlag)
	            rsFadeSound->stop(); // wyłącz to cholerne cykanie!
	        else
	        	rsFadeSound->loop().position(dynpos).play();
    	}

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
                if( dsbRelay != nullptr ) { dsbRelay->gain(1.0f); }
            }
            else
            {
                if( dsbRelay != nullptr ) { dsbRelay->gain(Global::soundgainmode == Global::compat ? 0.9f : 0.5f); }
            }
            if (!TestFlag(mvOccupied->SoundFlag, sound_manyrelay))
			{
                if (dsbRelay) dsbRelay->play();
			}
            else
            {
                if (TestFlag(mvOccupied->SoundFlag, sound_loud))
				{
                    if (dsbWejscie_na_bezoporow)
						dsbWejscie_na_bezoporow->play();
				}
                else
				{
                    if (dsbWejscie_na_drugi_uklad)
						dsbWejscie_na_drugi_uklad->play();
				}
            }
        }

        if( dsbBufferClamp != nullptr ) {
            if( TestFlag( mvOccupied->SoundFlag, sound_bufferclamp ) ) // zderzaki uderzaja o siebie
            {
                if( TestFlag( mvOccupied->SoundFlag, sound_loud ) )
                    dsbBufferClamp->gain(1.0f);
                else
                    dsbBufferClamp->gain(Global::soundgainmode == Global::compat ? 0.9f : 0.5f);
                dsbBufferClamp->play();
            }
        }
        if (dsbCouplerStretch)
            if (TestFlag(mvOccupied->SoundFlag, sound_couplerstretch)) // sprzegi sie rozciagaja
            {
                if (TestFlag(mvOccupied->SoundFlag, sound_loud))
                    dsbCouplerStretch->gain(1.0f);
                else
                    dsbCouplerStretch->gain(Global::soundgainmode == Global::compat ? 0.9f : 0.5f);
                dsbCouplerStretch->play();
            }

        if (mvOccupied->SoundFlag == 0)
            if (mvOccupied->EventFlag)
                if (TestFlag(mvOccupied->DamageFlag, dtrain_wheelwear))
                { // Ra: przenieść do DynObj!
                    if (rsRunningNoise)
                    {
                        rsRunningNoise->stop();
                        // float aa=rsRunningNoise->gain_off;
                        float am = rsRunningNoise->gain_mul;
                        float fa = rsRunningNoise->pitch_off;
                        float fm = rsRunningNoise->pitch_mul;

                       	// MC: zmiana szumu na lomot
                        sound_man->destroy_sound(&rsRunningNoise);
                        rsRunningNoise = sound_man->create_sound("lomotpodkucia.wav");
                        
                        if (rsRunningNoise->gain_mul == 1)
                            rsRunningNoise->gain_mul = am;
                        rsRunningNoise->gain_off = 0.7;
                        rsRunningNoise->pitch_off = fa;
                        rsRunningNoise->pitch_mul = fm;
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
        }

        else if (mvControlled->EngineType == DieselEngine)
        { // albo dla innego spalinowego
            fEngine[1] = mvControlled->ShowEngineRotation(1);
            fEngine[2] = mvControlled->ShowEngineRotation(2);
            fEngine[3] = mvControlled->ShowEngineRotation(3);
            if (ggMainGearStatus.SubModel)
            {
                if (mvControlled->Mains)
                    ggMainGearStatus.UpdateValue(1.1 - std::abs(mvControlled->dizel_automaticgearstatus));
                else
                    ggMainGearStatus.UpdateValue(0.0);
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
                rsSlippery->gain(-veldiff).loop().position(dynpos).play();
                if (mvControlled->TrainType == dt_181) // alarm przy poslizgu dla 181/182 - BOMBARDIER
                    if (dsbSlipAlarm)
                    	dsbSlipAlarm->loop().play();
            }
            else
            {
                if ((mvOccupied->UnitBrakeForce > 100.0) && (DynamicObject->GetVelocity() > 1.0))
                {
                	rsSlippery->gain(veldiff).loop().position(dynpos).play();
                    if (mvControlled->TrainType == dt_181)
                        if (dsbSlipAlarm)
                            dsbSlipAlarm->stop();
                }
            }
        }
        else
        {
            btLampkaPoslizg.TurnOff();
            rsSlippery->stop();
            if (mvControlled->TrainType == dt_181)
                if (dsbSlipAlarm)
                    dsbSlipAlarm->stop();
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
                if ( mvControlled->Battery || mvControlled->ConverterFlag )
                {
                    btLampkaWylSzybkiB.Turn( tmp->MoverParameters->Mains );

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
        if (ggMainCtrl.SubModel) {

            if( mvControlled->CoupledCtrl ) {
                ggMainCtrl.UpdateValue(
                    double( mvControlled->MainCtrlPos + mvControlled->ScndCtrlPos ),
                    dsbNastawnikJazdy );
            }
            else {
                ggMainCtrl.UpdateValue(
                    double( mvControlled->MainCtrlPos ),
                    dsbNastawnikJazdy );
            }
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
        if (ggScndCtrl.SubModel) {
            // Ra: od byte odejmowane boolean i konwertowane potem na double?
            ggScndCtrl.UpdateValue(
                double( mvControlled->ScndCtrlPos
                    - ( ( mvControlled->TrainType == dt_ET42 ) && mvControlled->DynamicBrakeFlag ) ),
                dsbNastawnikBocz );
            ggScndCtrl.Update();
        }
        if (ggDirKey.SubModel) {
            if (mvControlled->TrainType != dt_EZT)
                ggDirKey.UpdateValue(
                    double(mvControlled->ActiveDir),
                    dsbReverserKey);
            else
                ggDirKey.UpdateValue(
                    double(mvControlled->ActiveDir) + double(mvControlled->Imin == mvControlled->IminHi),
                    dsbReverserKey);
            ggDirKey.Update();
        }
        if (ggBrakeCtrl.SubModel)
        {
#ifdef _WIN32
            if (DynamicObject->Mechanik ?
                    (DynamicObject->Mechanik->AIControllFlag ? false : 
						(Global::iFeedbackMode == 4)) :
                    false) // nie blokujemy AI
            { // Ra: nie najlepsze miejsce, ale na początek gdzieś to dać trzeba
				// Firleju: dlatego kasujemy i zastepujemy funkcją w Console
				if (mvOccupied->BrakeHandle == FV4a)
                {
                    double b = Console::AnalogCalibrateGet(0);
					b = b * 8.0 - 2.0;
                    b = clamp<double>( b, -2.0, mvOccupied->BrakeCtrlPosNo ); // przycięcie zmiennej do granic
					ggBrakeCtrl.UpdateValue(b); // przesów bez zaokrąglenia
					mvOccupied->BrakeLevelSet(b);
				}
                if (mvOccupied->BrakeHandle == FVel6) // może można usunąć ograniczenie do FV4a i FVel6?
                {
                    double b = Console::AnalogCalibrateGet(0);
					b = b * 7.0 - 1.0;
                    b = clamp<double>( b, -1.0, mvOccupied->BrakeCtrlPosNo ); // przycięcie zmiennej do granic
                    ggBrakeCtrl.UpdateValue(b); // przesów bez zaokrąglenia
                    mvOccupied->BrakeLevelSet(b);
                }
                // else //standardowa prodedura z kranem powiązanym z klawiaturą
                // ggBrakeCtrl.UpdateValue(double(mvOccupied->BrakeCtrlPos));
            }
#endif
            // else //standardowa prodedura z kranem powiązanym z klawiaturą
            // ggBrakeCtrl.UpdateValue(double(mvOccupied->BrakeCtrlPos));
            ggBrakeCtrl.UpdateValue(mvOccupied->fBrakeCtrlPos);
            ggBrakeCtrl.Update();
        }
        if( ggLocalBrake.SubModel ) {
#ifdef _WIN32
            if( ( DynamicObject->Mechanik != nullptr )
             && ( false == DynamicObject->Mechanik->AIControllFlag ) // nie blokujemy AI
             && ( mvOccupied->BrakeLocHandle == FD1 )
             && ( ( Global::iFeedbackMode == 4 ) ) ) {
                // Ra: nie najlepsze miejsce, ale na początek gdzieś to dać trzeba
                // Firleju: dlatego kasujemy i zastepujemy funkcją w Console
                auto const b = clamp<double>(
                    Console::AnalogCalibrateGet( 1 ) * 10.0,
                    0.0,
                    ManualBrakePosNo );
                ggLocalBrake.UpdateValue( b ); // przesów bez zaokrąglenia
                mvOccupied->LocalBrakePos = int( 1.09 * b ); // sposób zaokrąglania jest do ustalenia
            }
            else // standardowa prodedura z kranem powiązanym z klawiaturą
#endif
                ggLocalBrake.UpdateValue(double(mvOccupied->LocalBrakePos));

            ggLocalBrake.Update();
        }
        if (ggManualBrake.SubModel != nullptr) {
            ggManualBrake.UpdateValue(double(mvOccupied->ManualBrakePos));
            ggManualBrake.Update();
        }
        ggAlarmChain.Update();
        ggBrakeProfileCtrl.Update();
        ggBrakeProfileG.Update();
        ggBrakeProfileR.Update();
        ggMaxCurrentCtrl.Update();
        // NBMX wrzesien 2003 - drzwi
        ggDoorLeftButton.Update();
        ggDoorRightButton.Update();
        ggDoorSignallingButton.Update();
        // NBMX dzwignia sprezarki
        ggCompressorButton.Update();
        ggCompressorLocalButton.Update();

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
                	dsbBuzzer->loop().play();
#ifdef _WIN32
                    Console::BitsSet(1 << 14); // ustawienie bitu 16 na PoKeys
#endif
                }
            }
            else
            {
                if (dsbBuzzer)
                {
                	dsbBuzzer->stop();
#ifdef _WIN32
                    Console::BitsClear(1 << 14); // ustawienie bitu 16 na PoKeys
#endif
                }
            }
        }
        else // wylaczone
        {
            btLampkaCzuwaka.TurnOff();
            btLampkaSHP.TurnOff();
            if (dsbBuzzer)
            {
            	dsbBuzzer->stop();
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
                        dsbDieselIgnition->play();
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
                dsbRelay->play();
                fMainRelayTimer = 0;
            }
            ggMainOnButton.UpdateValue(0);
        }
        //---
        if (!Global::shiftState && Console::Pressed(Global::Keys[k_Main]))
        {
            ggMainOffButton.PutValue(1);
            if (mvControlled->MainSwitch(false))
                dsbRelay->play();
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
                    if (Global::shiftState)
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
                    if (ggInstrumentLightButton.SubModel)
                    {
                        ggInstrumentLightButton.PutValue(1); // hunter-131211: z UpdateValue na
                        // PutValue - by zachowywal sie jak
                        // pozostale przelaczniki
                        if (btInstrumentLight.Active())
                            InstrumentLightActive = true;
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
                    if (ggInstrumentLightButton.SubModel)
                    {
                        ggInstrumentLightButton.PutValue(0); // hunter-131211: z UpdateValue na
                        // PutValue - by zachowywal sie jak
                        // pozostale przelaczniki
                        if (btInstrumentLight.Active())
                            InstrumentLightActive = false;
                    }
                }
            }
        }
#endif
        // ABu030405 obsluga lampki uniwersalnej:
        if (btInstrumentLight.Active()) // w ogóle jest
            if (InstrumentLightActive) // załączona
                switch (InstrumentLightType)
                {
                case 0:
                    btInstrumentLight.Turn( mvControlled->Battery || mvControlled->ConverterFlag );
                    break;
                case 1:
                    btInstrumentLight.Turn(mvControlled->Mains);
                    break;
                case 2:
                    btInstrumentLight.Turn(mvControlled->ConverterFlag);
                    break;
                default:
                    btInstrumentLight.TurnOff();
                }
            else
                btInstrumentLight.TurnOff();

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
                    Universal4Active = true;
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
                    Universal4Active = false;
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
                dsbPneumaticSwitch->play();
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
                    dsbPneumaticSwitch->play();
                }
            }
            else
            {
                if (mvOccupied->SwitchEPBrake(0))
                {
                    dsbPneumaticSwitch->play();
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
                        dsbSwitch->play();
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
                            dsbPneumaticSwitch->play();
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
                                dsbPneumaticSwitch->play();
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
                    dsbSwitch->play();
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
            	dsbHasler->loop().play();
            }
            else {
                if( ( true == FreeFlyModeFlag ) || ( fTachoCount < 1 ) ) {
                    dsbHasler->stop();
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
        ggPantSelectedButton.Update();
        ggPantFrontButtonOff.Update();
        ggPantRearButtonOff.Update();
        ggPantSelectedDownButton.Update();
        ggPantAllDownButton.Update();
        ggPantCompressorButton.Update();
        ggPantCompressorValve.Update();

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
        ggConverterButton.Update();
        ggConverterLocalButton.Update();
        ggConverterOffButton.Update();
        ggTrainHeatingButton.Update();
        ggSignallingButton.Update();
        ggNextCurrentButton.Update();
        ggHornButton.Update();
        ggHornLowButton.Update();
        ggHornHighButton.Update();
/*
        ggUniversal1Button.Update();
        ggUniversal2Button.Update();
        if( Universal4Active ) {
            ggUniversal4Button.PermIncValue( dt );
        }
        ggUniversal4Button.Update();
*/
        for( auto &universal : ggUniversals ) {
            universal.Update();
        }
        // hunter-091012
        ggInstrumentLightButton.Update();
        ggCabLightButton.Update();
        ggCabLightDimButton.Update();
        ggBatteryButton.Update();
        //------
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
            if( mvOccupied->ConverterFlag == true ) {
                // jasnosc dla zalaczonej przetwornicy
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
        if( ggPantAllDownButton.GetValue() == 0.0 ) {
            // the 'lower all' button overrides state of switches, while active itself
            if( ( false == mvControlled->PantFrontUp )
             && ( ggPantFrontButton.GetValue() >= 1.0 ) ) {
                mvControlled->PantFront( true );
            }
            if( ( false == mvControlled->PantRearUp )
             && ( ggPantRearButton.GetValue() >= 1.0 ) ) {
                mvControlled->PantRear( true );
            }
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
                DynamicObject->MoverParameters->CabActivisation(); // załączenie rozrządu (wirtualne kabiny)
                DynamicObject->Mechanik->CheckVehicles( Change_direction );
                return true; // udało się zmienić kabinę
            }
        // aktywizacja poprzedniej, bo jeszcze nie wiadomo, czy jakiś pojazd jest
        DynamicObject->MoverParameters->CabActivisation();
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
    // NOTE: should be no longer needed as safety checks were added,
    // but leaving the defaults for the sake of incomplete mmd files
    dsbPneumaticSwitch = sound_man->create_sound("silence1.wav");
    dsbBufferClamp = sound_man->create_sound("en57_bufferclamp.wav");
    dsbCouplerDetach = sound_man->create_sound("couplerdetach.wav");
    dsbCouplerStretch = sound_man->create_sound("en57_couplerstretch.wav");
    dsbCouplerAttach = sound_man->create_sound("couplerattach.wav");

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
                    sound_man->create_sound(parser.getToken<std::string>());
            }
            else if (token == "ctrlscnd:")
            {
                // hunter-081211: nastawnik bocznikowania
                dsbNastawnikBocz =
                    sound_man->create_sound(parser.getToken<std::string>());
            }
            else if (token == "reverserkey:")
            {
                // hunter-131211: dzwiek kierunkowego
                dsbReverserKey =
                    sound_man->create_sound(parser.getToken<std::string>());
            }
            else if (token == "buzzer:")
            {
                // bzyczek shp:
                dsbBuzzer =
                    sound_man->create_sound(parser.getToken<std::string>());
            }
            else if (token == "slipalarm:")
            {
                // Bombardier 011010: alarm przy poslizgu:
                dsbSlipAlarm =
                    sound_man->create_sound(parser.getToken<std::string>());
            }
            else if (token == "tachoclock:")
            {
                // cykanie rejestratora:
                dsbHasler =
                    sound_man->create_sound(parser.getToken<std::string>());
            }
            else if (token == "switch:")
            {
                // przelaczniki:
                dsbSwitch =
                    sound_man->create_sound(parser.getToken<std::string>());
            }
            else if (token == "pneumaticswitch:")
            {
                // stycznik EP:
                dsbPneumaticSwitch =
                    sound_man->create_sound(parser.getToken<std::string>());
            }
            else if (token == "wejscie_na_bezoporow:")
            {
                // hunter-111211: wydzielenie wejscia na bezoporowa i na drugi uklad do pliku
                dsbWejscie_na_bezoporow =
                    sound_man->create_sound(parser.getToken<std::string>());
            }
            else if (token == "wejscie_na_drugi_uklad:")
            {

                dsbWejscie_na_drugi_uklad =
                    sound_man->create_sound(parser.getToken<std::string>());
            }
            else if (token == "relay:")
            {
                // styczniki itp:
                dsbRelay =
                    sound_man->create_sound(parser.getToken<std::string>());
                if (!dsbWejscie_na_bezoporow)
                { // hunter-111211: domyslne, gdy brak
                    dsbWejscie_na_bezoporow =
                        sound_man->create_sound("wejscie_na_bezoporow.wav");
                }
                if (!dsbWejscie_na_drugi_uklad)
                {
                    dsbWejscie_na_drugi_uklad =
                        sound_man->create_sound("wescie_na_drugi_uklad.wav");
                }
            }
            else if (token == "pneumaticrelay:")
            {
                // wylaczniki pneumatyczne:
                dsbPneumaticRelay =
                    sound_man->create_sound(parser.getToken<std::string>());
            }
            else if (token == "couplerattach:")
            {
                // laczenie:
                dsbCouplerAttach =
                    sound_man->create_sound(parser.getToken<std::string>());
            }
            else if (token == "couplerstretch:")
            {
                // laczenie:
                dsbCouplerStretch =
                    sound_man->create_sound(parser.getToken<std::string>());
            }
            else if (token == "couplerdetach:")
            {
                // rozlaczanie:
                dsbCouplerDetach =
                    sound_man->create_sound(parser.getToken<std::string>());
            }
            else if (token == "bufferclamp:")
            {
                // laczenie:
                dsbBufferClamp =
                    sound_man->create_sound(parser.getToken<std::string>());
            }
            else if (token == "ignition:")
            {
                // odpalanie silnika
                dsbDieselIgnition =
                    sound_man->create_sound(parser.getToken<std::string>());
            }
            else if (token == "brakesound:")
            {
                // hamowanie zwykle:
                rsBrake = sound_man->create_sound(parser.getToken<std::string>());
                parser.getTokens(4, false);
				if (rsBrake)
				{
                	parser >> rsBrake->gain_mul >> rsBrake->gain_off >> rsBrake->pitch_mul >> rsBrake->pitch_off;
	                rsBrake->gain_mul /= (1 + mvOccupied->MaxBrakeForce * 1000);
	                rsBrake->pitch_mul /= (1 + mvOccupied->Vmax);
				}
            }
            else if (token == "slipperysound:")
            {
                // sanie:
                rsSlippery = sound_man->create_sound(parser.getToken<std::string>());
                parser.getTokens(2, false);
				if (rsSlippery)
				{
	                parser >> rsSlippery->gain_mul >> rsSlippery->gain_off;
	                rsSlippery->pitch_mul = 0.0;
	                rsSlippery->pitch_off = 1.0;
	                rsSlippery->gain_mul /= (1 + mvOccupied->Vmax);
				}
            }
            else if (token == "airsound:")
            {
                // syk:
                rsHiss = sound_man->create_sound(parser.getToken<std::string>());
                parser.getTokens(2, false);
				if (rsHiss)
				{
	                parser >> rsHiss->gain_mul >> rsHiss->gain_off;
	                rsHiss->pitch_mul = 0.0;
	                rsHiss->pitch_off = 1.0;
				}
            }
            else if (token == "airsound2:")
            {
                // syk:
                rsHissU = sound_man->create_sound(parser.getToken<std::string>());
                parser.getTokens(2, false);
				if (rsHissU)
				{
	                parser >> rsHissU->gain_mul >> rsHissU->gain_off;
	                rsHissU->pitch_mul = 0.0;
	                rsHissU->pitch_off = 1.0;
				}
            }
            else if (token == "airsound3:")
            {
                // syk:
                rsHissE = sound_man->create_sound(parser.getToken<std::string>());
                parser.getTokens(2, false);
				if (rsHissE)
				{
	                parser >> rsHissE->gain_mul >> rsHissE->gain_off;
	                rsHissE->pitch_mul = 0.0;
	                rsHissE->pitch_off = 1.0;
				}
            }
            else if (token == "airsound4:")
            {
                // syk:
                rsHissX = sound_man->create_sound(parser.getToken<std::string>());
                parser.getTokens(2, false);
				if (rsHissX)
				{
        	        parser >> rsHissX->gain_mul >> rsHissX->gain_off;
    	            rsHissX->pitch_mul = 0.0;
	                rsHissX->pitch_off = 1.0;
				}
            }
            else if (token == "airsound5:")
            {
                // syk:
                rsHissT = sound_man->create_sound(parser.getToken<std::string>());
                parser.getTokens(2, false);
				if (rsHissT)
				{
	                parser >> rsHissT->gain_mul >> rsHissT->gain_off;
	                rsHissT->pitch_mul = 0.0;
	                rsHissT->pitch_off = 1.0;
				}
            }
            else if (token == "fadesound:")
            {
                // syk:
                rsFadeSound = sound_man->create_sound(parser.getToken<std::string>());
				if (rsFadeSound)
				{
	                rsFadeSound->gain_mul = 1.0;
	                rsFadeSound->gain_off = 1.0;
	                rsFadeSound->pitch_mul = 1.0;
	                rsFadeSound->pitch_off = 1.0;
				}
            }
            else if (token == "localbrakesound:")
            {
                // syk:
                rsSBHiss = sound_man->create_sound(parser.getToken<std::string>());
                parser.getTokens(2, false);
				if (rsSBHiss)
				{
	                parser >> rsSBHiss->gain_mul >> rsSBHiss->gain_off;
	                rsSBHiss->pitch_mul = 0.0;
	                rsSBHiss->pitch_off = 1.0;
				}
            }
            else if (token == "runningnoise:")
            {
                // szum podczas jazdy:
                rsRunningNoise = sound_man->create_sound(parser.getToken<std::string>());
                parser.getTokens(4, false);
				if (rsRunningNoise)
				{
	                parser >> rsRunningNoise->gain_mul >> rsRunningNoise->gain_off >> rsRunningNoise->pitch_mul >>
	                    rsRunningNoise->pitch_off;
	                rsRunningNoise->gain_mul /= (1 + mvOccupied->Vmax);
	                rsRunningNoise->pitch_mul /= (1 + mvOccupied->Vmax);
				}
            }
            else if (token == "engageslippery:")
            {
                // tarcie tarcz sprzegla:
                rsEngageSlippery = sound_man->create_sound(parser.getToken<std::string>());
                parser.getTokens(4, false);
				if (rsEngageSlippery)
				{
	                parser >> rsEngageSlippery->gain_mul >> rsEngageSlippery->gain_off >> rsEngageSlippery->pitch_mul >>
	                    rsEngageSlippery->pitch_off;
	                rsEngageSlippery->pitch_mul /= (1 + mvOccupied->nmax);
				}
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
                    sound_man->create_sound(parser.getToken<std::string>());
            }
            else if (token == "pantographdown:")
            {
                // podniesienie patyka:
                dsbPantDown =
                    sound_man->create_sound(parser.getToken<std::string>());
            }
            else if (token == "doorclose:")
            {
                // zamkniecie drzwi:
                dsbDoorClose =
                    sound_man->create_sound(parser.getToken<std::string>());
            }
            else if (token == "dooropen:")
            {
                // otwarcie drzwi:
                dsbDoorOpen =
                    sound_man->create_sound(parser.getToken<std::string>());
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
    m_controlmapper.clear();
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
            if (parse == true) {

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
/*
                Universal4Active = false;
*/
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
{ // likwidacja ruchu kamery w kabinie (po powrocie przez [F4])
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
        dsbNastawnikJazdy->stop();
    if (dsbNastawnikBocz)
        dsbNastawnikBocz->stop();
    if (dsbRelay)
        dsbRelay->stop();
    if (dsbPneumaticRelay)
        dsbPneumaticRelay->stop();
    if (dsbSwitch)
        dsbSwitch->stop();
    if (dsbPneumaticSwitch)
        dsbPneumaticSwitch->stop();
    if (dsbReverserKey)
        dsbReverserKey->stop();
    if (dsbCouplerAttach)
        dsbCouplerAttach->stop();
    if (dsbCouplerDetach)
        dsbCouplerDetach->stop();
    if (dsbDieselIgnition)
        dsbDieselIgnition->stop();
    if (dsbDoorClose)
        dsbDoorClose->stop();
    if (dsbDoorOpen)
        dsbDoorOpen->stop();
    if (dsbPantUp)
        dsbPantUp->stop();
    if (dsbPantDown)
        dsbPantDown->stop();
    if (dsbWejscie_na_bezoporow)
        dsbWejscie_na_bezoporow->stop();
    if (dsbWejscie_na_drugi_uklad)
        dsbWejscie_na_drugi_uklad->stop();
    if (rsBrake)
    	rsBrake->stop();
    if (rsSlippery)
    	rsSlippery->stop();
    if (rsHiss)
    	rsHiss->stop();
    if (rsHissU)
    	rsHissU->stop();
    if (rsHissE)
    	rsHissE->stop();
    if (rsHissX)
    	rsHissX->stop();
    if (rsHissT)
    	rsHissT->stop();
    if (rsSBHiss)
	    rsSBHiss->stop();
	if (rsRunningNoise)
    	rsRunningNoise->stop();
    if (rsEngageSlippery)
    	rsEngageSlippery->stop();
    if (rsFadeSound)
    	rsFadeSound->stop();
    if (dsbHasler)
        dsbHasler->stop(); // wyłączenie dźwięków opuszczanej kabiny
    if (dsbBuzzer)
        dsbBuzzer->stop();
    if (dsbSlipAlarm)
        dsbSlipAlarm->stop(); // dźwięk alarmu przy poślizgu
    // sConverter->stop();
    // sSmallCompressor->stop();
    if (dsbCouplerStretch)
        dsbCouplerStretch->stop();
    if (dsbEN57_CouplerStretch)
        dsbEN57_CouplerStretch->stop();
    if (dsbBufferClamp)
        dsbBufferClamp->stop();
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
    ggAlarmChain.Clear();
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
/*
    ggUniversal1Button.Clear();
    ggUniversal2Button.Clear();
    ggUniversal4Button.Clear();
*/
    for( auto &universal : ggUniversals ) {
        universal.Clear();
    }
    ggInstrumentLightButton.Clear();
    // hunter-091012
    ggCabLightButton.Clear();
    ggCabLightDimButton.Clear();
    //-------
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
    ggPantSelectedButton.Clear();
    ggPantFrontButtonOff.Clear();
    ggPantRearButtonOff.Clear();
    ggPantSelectedDownButton.Clear();
    ggPantAllDownButton.Clear();
    ggPantCompressorButton.Clear();
    ggPantCompressorValve.Clear();
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
    btInstrumentLight.Clear();
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
    // motor connectors
    ggStLinOffButton.PutValue(
        ( mvControlled->StLinSwitchOff ?
            1.0 :
            0.0 ) );
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
        // NOTE: currently we animate the selectable pantograph control for both pantographs
        // TODO: implement actual selection control, and refactor handling this control setup in a separate method
        ggPantSelectedButton.PutValue(
            ( mvControlled->PantFrontUp ?
                1.0 :
                0.0 ) );
        ggPantSelectedDownButton.PutValue(
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
        // NOTE: currently we animate the selectable pantograph control for both pantographs
        // TODO: implement actual selection control, and refactor handling this control setup in a separate method
        ggPantSelectedButton.PutValue(
            ( mvControlled->PantRearUp ?
                1.0 :
                0.0 ) );
        ggPantSelectedDownButton.PutValue(
            ( mvControlled->PantRearUp ?
                0.0 :
                1.0 ) );
    }
    // auxiliary compressor
    ggPantCompressorValve.PutValue(
        mvControlled->bPantKurek3 ?
            0.0 : // default setting is pantographs connected with primary tank
            1.0 );
    ggPantCompressorButton.PutValue(
        mvControlled->PantCompFlag ?
            1.0 :
            0.0 );
    // converter
    if( mvOccupied->ConvSwitchType != "impulse" ) {
        ggConverterButton.PutValue(
            mvControlled->ConverterAllow ?
                1.0 :
                0.0 );
    }
    ggConverterLocalButton.PutValue(
        mvControlled->ConverterAllowLocal ?
            1.0 :
            0.0 );
    // compressor
    ggCompressorButton.PutValue(
        mvControlled->CompressorAllow ?
            1.0 :
            0.0 );
    ggCompressorLocalButton.PutValue(
        mvControlled->CompressorAllowLocal ?
            1.0 :
            0.0 );
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
        if( ggLeftEndLightButton.SubModel != nullptr ) {
            ggLeftEndLightButton.PutValue( 1.0 );
        }
        else {
            ggLeftLightButton.PutValue( -1.0 );
        }
    }
    if( ( DynamicObject->iLights[ lightsindex ] & TMoverParameters::light::redmarker_right ) != 0 ) {
        if( ggRightEndLightButton.SubModel != nullptr ) {
            ggRightEndLightButton.PutValue( 1.0 );
        }
        else {
            ggRightLightButton.PutValue( -1.0 );
        }
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
    if( true == InstrumentLightActive ) {
        ggInstrumentLightButton.PutValue( 1.0 );
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
    // alarm chain
    ggAlarmChain.PutValue(
        mvControlled->AlarmChainFlag ?
            1.0 :
            0.0 );
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
/*
    TButton *bt; // roboczy wskaźnik na obiekt animujący lampkę
*/
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
    else if (Label == "i-instrumentlight:")
    {
        btInstrumentLight.Load(Parser, DynamicObject->mdKabina, DynamicObject->mdModel);
        InstrumentLightType = 0;
    }
    else if (Label == "i-instrumentlight_M:")
    {
        btInstrumentLight.Load(Parser, DynamicObject->mdKabina, DynamicObject->mdModel);
        InstrumentLightType = 1;
    }
    else if (Label == "i-instrumentlight_C:")
    {
        btInstrumentLight.Load(Parser, DynamicObject->mdKabina, DynamicObject->mdModel);
        InstrumentLightType = 2;
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
        auto &button = Cabine[Cabindex].Button(-1); // pierwsza wolna lampka
        button.Load(Parser, DynamicObject->mdKabina);
        button.AssignBool(bDoors[0] + 3 * i);
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
bool TTrain::initialize_gauge(cParser &Parser, std::string const &Label, int const Cabindex) {

/*
    TGauge *gg { nullptr }; // roboczy wskaźnik na obiekt animujący gałkę
*/
    std::unordered_map<std::string, TGauge &> gauges = {
        { "mainctrl:", ggMainCtrl },
        { "scndctrl:", ggScndCtrl },
        { "dirkey:" , ggDirKey },
        { "brakectrl:", ggBrakeCtrl },
        { "localbrake:", ggLocalBrake },
        { "manualbrake:", ggManualBrake },
        { "alarmchain:", ggAlarmChain },
        { "brakeprofile_sw:", ggBrakeProfileCtrl },
        { "brakeprofileg_sw:", ggBrakeProfileG },
        { "brakeprofiler_sw:", ggBrakeProfileR },
        { "maxcurrent_sw:", ggMaxCurrentCtrl },
        { "main_off_bt:", ggMainOffButton },
        { "main_on_bt:", ggMainOnButton },
        { "security_reset_bt:", ggSecurityResetButton },
        { "releaser_bt:", ggReleaserButton },
        { "sand_bt:", ggSandButton },
        { "antislip_bt:", ggAntiSlipButton },
        { "horn_bt:", ggHornButton },
        { "hornlow_bt:", ggHornLowButton },
        { "hornhigh_bt:", ggHornHighButton },
        { "fuse_bt:", ggFuseButton },
        { "converterfuse_bt:", ggConverterFuseButton },
        { "stlinoff_bt:", ggStLinOffButton },
        { "door_left_sw:", ggDoorLeftButton },
        { "door_right_sw:", ggDoorRightButton },
        { "departure_signal_bt:", ggDepartureSignalButton },
        { "upperlight_sw:", ggUpperLightButton },
        { "leftlight_sw:", ggLeftLightButton },
        { "rightlight_sw:", ggRightLightButton },
        { "dimheadlights_sw:", ggDimHeadlightsButton },
        { "leftend_sw:", ggLeftEndLightButton },
        { "rightend_sw:", ggRightEndLightButton },
        { "lights_sw:", ggLightsButton },
        { "rearupperlight_sw:", ggRearUpperLightButton },
        { "rearleftlight_sw:", ggRearLeftLightButton },
        { "rearrightlight_sw:", ggRearRightLightButton },
        { "rearleftend_sw:", ggRearLeftEndLightButton },
        { "rearrightend_sw:",  ggRearRightEndLightButton },
        { "compressor_sw:", ggCompressorButton },
        { "compressorlocal_sw:", ggCompressorLocalButton },
        { "converter_sw:", ggConverterButton },
        { "converterlocal_sw:", ggConverterLocalButton },
        { "converteroff_sw:", ggConverterOffButton },
        { "main_sw:", ggMainButton },
        { "radio_sw:", ggRadioButton },
        { "pantfront_sw:", ggPantFrontButton },
        { "pantrear_sw:", ggPantRearButton },
        { "pantfrontoff_sw:", ggPantFrontButtonOff },
        { "pantrearoff_sw:", ggPantRearButtonOff },
        { "pantalloff_sw:", ggPantAllDownButton },
        { "pantselected_sw:", ggPantSelectedButton },
        { "pantselectedoff_sw:", ggPantSelectedDownButton },
        { "pantcompressor_sw:", ggPantCompressorButton },
        { "pantcompressorvalve_sw:", ggPantCompressorValve },
        { "trainheating_sw:", ggTrainHeatingButton },
        { "signalling_sw:", ggSignallingButton },
        { "door_signalling_sw:", ggDoorSignallingButton },
        { "nextcurrent_sw:", ggNextCurrentButton },
        { "instrumentlight_sw:", ggInstrumentLightButton },
        { "cablight_sw:", ggCabLightButton },
        { "cablightdim_sw:", ggCabLightDimButton },
        { "battery_sw:", ggBatteryButton },
        { "universal0:", ggUniversals[ 0 ] },
        { "universal1:", ggUniversals[ 1 ] },
        { "universal2:", ggUniversals[ 2 ] },
        { "universal3:", ggUniversals[ 3 ] },
        { "universal4:", ggUniversals[ 4 ] },
        { "universal5:", ggUniversals[ 5 ] },
        { "universal6:", ggUniversals[ 6 ] },
        { "universal7:", ggUniversals[ 7 ] },
        { "universal8:", ggUniversals[ 8 ] },
        { "universal9:", ggUniversals[ 9 ] }
    };
    auto lookup = gauges.find( Label );
    if( lookup != gauges.end() ) {
        lookup->second.Load( Parser, DynamicObject->mdKabina );
        m_controlmapper.insert( lookup->second, lookup->first );
    }
    // ABu 090305: uniwersalne przyciski lub inne rzeczy
    else if( Label == "mainctrlact:" ) {
        ggMainCtrlAct.Load( Parser, DynamicObject->mdKabina, DynamicObject->mdModel );
    }
    // SEKCJA WSKAZNIKOW
    else if ((Label == "tachometer:") || (Label == "tachometerb:"))
    {
        // predkosciomierz wskaźnikowy z szarpaniem
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject->mdKabina);
        gauge.AssignFloat(&fTachoVelocityJump);
    }
    else if (Label == "tachometern:")
    {
        // predkosciomierz wskaźnikowy bez szarpania
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject->mdKabina);
        gauge.AssignFloat(&fTachoVelocity);
    }
    else if (Label == "tachometerd:")
    {
        // predkosciomierz cyfrowy
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject->mdKabina);
        gauge.AssignFloat(&fTachoVelocity);
    }
    else if ((Label == "hvcurrent1:") || (Label == "hvcurrent1b:"))
    {
        // 1szy amperomierz
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject->mdKabina);
        gauge.AssignFloat(fHCurrent + 1);
    }
    else if ((Label == "hvcurrent2:") || (Label == "hvcurrent2b:"))
    {
        // 2gi amperomierz
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject->mdKabina);
        gauge.AssignFloat(fHCurrent + 2);
    }
    else if ((Label == "hvcurrent3:") || (Label == "hvcurrent3b:"))
    {
        // 3ci amperomierz
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałska
        gauge.Load(Parser, DynamicObject->mdKabina);
        gauge.AssignFloat(fHCurrent + 3);
    }
    else if ((Label == "hvcurrent:") || (Label == "hvcurrentb:"))
    {
        // amperomierz calkowitego pradu
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject->mdKabina);
        gauge.AssignFloat(fHCurrent);
    }
    else if (Label == "eimscreen:")
    {
        // amperomierz calkowitego pradu
        int i, j;
        Parser.getTokens(2, false);
        Parser >> i >> j;
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject->mdKabina);
        gauge.AssignFloat(&fEIMParams[i][j]);
    }
    else if (Label == "brakes:")
    {
        // amperomierz calkowitego pradu
        int i, j;
        Parser.getTokens(2, false);
        Parser >> i >> j;
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject->mdKabina);
        gauge.AssignFloat(&fPress[i - 1][j]);
    }
    else if ((Label == "brakepress:") || (Label == "brakepressb:"))
    {
        // manometr cylindrow hamulcowych
        // Ra 2014-08: przeniesione do TCab
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject->mdKabina, nullptr, 0.1);
        gauge.AssignDouble(&mvOccupied->BrakePress);
    }
    else if ((Label == "pipepress:") || (Label == "pipepressb:"))
    {
        // manometr przewodu hamulcowego
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject->mdKabina, nullptr, 0.1);
        gauge.AssignDouble(&mvOccupied->PipePress);
    }
    else if (Label == "limpipepress:")
    {
        // manometr zbiornika sterujacego zaworu maszynisty
        ggZbS.Load(Parser, DynamicObject->mdKabina, nullptr, 0.1);
    }
    else if (Label == "cntrlpress:")
    {
        // manometr zbiornika kontrolnego/rorzďż˝du
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject->mdKabina, nullptr, 0.1);
        gauge.AssignDouble(&mvControlled->PantPress);
    }
    else if ((Label == "compressor:") || (Label == "compressorb:"))
    {
        // manometr sprezarki/zbiornika glownego
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject->mdKabina, nullptr, 0.1);
        gauge.AssignDouble(&mvOccupied->Compressor);
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
            ggClockSInd.Init(DynamicObject->mdKabina->GetFromName("ClockShand"), gt_Rotate, 1.0/60.0, 0, 0);
            ggClockMInd.Init(DynamicObject->mdKabina->GetFromName("ClockMhand"), gt_Rotate, 1.0/60.0, 0, 0);
            ggClockHInd.Init(DynamicObject->mdKabina->GetFromName("ClockHhand"), gt_Rotate, 1.0/12.0, 0, 0);
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
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject->mdKabina);
        gauge.AssignFloat(&fHVoltage);
    }
    else if (Label == "lvoltage:")
    {
        // woltomierz niskiego napiecia
        ggLVoltage.Load(Parser, DynamicObject->mdKabina);
    }
    else if (Label == "enrot1m:")
    {
        // obrotomierz
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject->mdKabina);
        gauge.AssignFloat(fEngine + 1);
    } // ggEnrot1m.Load(Parser,DynamicObject->mdKabina);
    else if (Label == "enrot2m:")
    {
        // obrotomierz
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject->mdKabina);
        gauge.AssignFloat(fEngine + 2);
    } // ggEnrot2m.Load(Parser,DynamicObject->mdKabina);
    else if (Label == "enrot3m:")
    { // obrotomierz
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject->mdKabina);
        gauge.AssignFloat(fEngine + 3);
    } // ggEnrot3m.Load(Parser,DynamicObject->mdKabina);
    else if (Label == "engageratio:")
    {
        // np. ciśnienie sterownika sprzęgła
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject->mdKabina);
        gauge.AssignDouble(&mvControlled->dizel_engage);
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
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject->mdKabina);
        gauge.AssignDouble(&mvControlled->DistCounter);
    }
    else
    {
        // failed to match the label
        return false;
    }

    return true;
}

void
TTrain::play_sound( sound* Sound, float gain ) {

    if( Sound ) {
    	Sound->gain(gain).play();
    }
}

void
TTrain::play_sound( sound* Sound, sound* Fallbacksound, float gain ) {

    if( Sound )
        play_sound( Sound, gain);
    else if( Fallbacksound )
        play_sound( Fallbacksound, gain );
}

float TTrain::get_tacho()
{
    return fTachoVelocity;
}

float TTrain::get_tank_pressure()
{
    return mvOccupied->Compressor;
}

float TTrain::get_pipe_pressure()
{
    return mvOccupied->PipePress;
}

float TTrain::get_brake_pressure()
{
    return mvOccupied->BrakePress;
}

float TTrain::get_hv_voltage()
{
    return fHVoltage;
}

std::array<float, 3> TTrain::get_current()
{
    return { fHCurrent[(mvControlled->TrainType & dt_EZT) ? 0 : 1], fHCurrent[2], fHCurrent[3] };
}

bool TTrain::get_alarm()
{
	return (TestFlag(mvOccupied->SecuritySystem.Status, s_CAalarm) ||
        TestFlag(mvOccupied->SecuritySystem.Status, s_SHPalarm));
}

void TTrain::set_mainctrl(int pos)
{
    if (pos < mvControlled->MainCtrlPos)
        mvControlled->DecMainCtrl(1);
    else if (pos > mvControlled->MainCtrlPos)
        mvControlled->IncMainCtrl(1);
}

void TTrain::set_scndctrl(int pos)
{
    if (pos < mvControlled->ScndCtrlPos)
        mvControlled->DecScndCtrl(1);
    else if (pos > mvControlled->ScndCtrlPos)
        mvControlled->IncScndCtrl(1);
}

void TTrain::set_trainbrake(float val)
{
	val = std::min(1.0f, std::max(0.0f, val));
    float min = mvControlled->Handle->GetPos(bh_MIN);
    float max = mvControlled->Handle->GetPos(bh_MAX);
    mvControlled->BrakeLevelSet(min + val * (max - min));
}

void TTrain::set_localbrake(float val)
{
	val = std::min(1.0f, std::max(0.0f, val));
    float min = 0.0f;
    float max = (float)LocalBrakePosNo;
    mvControlled->LocalBrakePos = std::round(min + val * (max - min));
}

int TTrain::get_drive_direction()
{
	return mvOccupied->ActiveDir * mvOccupied->CabNo;
}
