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
#include "simulation.h"
#include "Logs.h"
#include "MdlMngr.h"
#include "Timer.h"
#include "Driver.h"
#include "Console.h"

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

    { user_command::aidriverenable, &TTrain::OnCommand_aidriverenable },
    { user_command::aidriverdisable, &TTrain::OnCommand_aidriverdisable },
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
    { user_command::trainbrakehandleoff, &TTrain::OnCommand_trainbrakehandleoff },
    { user_command::trainbrakeemergency, &TTrain::OnCommand_trainbrakeemergency },
    { user_command::trainbrakebasepressureincrease, &TTrain::OnCommand_trainbrakebasepressureincrease },
    { user_command::trainbrakebasepressuredecrease, &TTrain::OnCommand_trainbrakebasepressuredecrease },
    { user_command::trainbrakebasepressurereset, &TTrain::OnCommand_trainbrakebasepressurereset },
    { user_command::trainbrakeoperationtoggle, &TTrain::OnCommand_trainbrakeoperationtoggle },
    { user_command::manualbrakeincrease, &TTrain::OnCommand_manualbrakeincrease },
    { user_command::manualbrakedecrease, &TTrain::OnCommand_manualbrakedecrease },
    { user_command::alarmchaintoggle, &TTrain::OnCommand_alarmchaintoggle },
    { user_command::wheelspinbrakeactivate, &TTrain::OnCommand_wheelspinbrakeactivate },
    { user_command::sandboxactivate, &TTrain::OnCommand_sandboxactivate },
    { user_command::epbrakecontroltoggle, &TTrain::OnCommand_epbrakecontroltoggle },
    { user_command::brakeactingspeedincrease, &TTrain::OnCommand_brakeactingspeedincrease },
    { user_command::brakeactingspeeddecrease, &TTrain::OnCommand_brakeactingspeeddecrease },
    { user_command::brakeloadcompensationincrease, &TTrain::OnCommand_brakeloadcompensationincrease },
    { user_command::brakeloadcompensationdecrease, &TTrain::OnCommand_brakeloadcompensationdecrease },
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
    { user_command::lightspresetactivatenext, &TTrain::OnCommand_lightspresetactivatenext },
    { user_command::lightspresetactivateprevious, &TTrain::OnCommand_lightspresetactivateprevious },
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
    { user_command::redmarkerstoggle, &TTrain::OnCommand_redmarkerstoggle },
    { user_command::endsignalstoggle, &TTrain::OnCommand_endsignalstoggle },
    { user_command::headlightsdimtoggle, &TTrain::OnCommand_headlightsdimtoggle },
    { user_command::interiorlighttoggle, &TTrain::OnCommand_interiorlighttoggle },
    { user_command::interiorlightdimtoggle, &TTrain::OnCommand_interiorlightdimtoggle },
    { user_command::instrumentlighttoggle, &TTrain::OnCommand_instrumentlighttoggle },
    { user_command::doorlocktoggle, &TTrain::OnCommand_doorlocktoggle },
    { user_command::doortoggleleft, &TTrain::OnCommand_doortoggleleft },
    { user_command::doortoggleright, &TTrain::OnCommand_doortoggleright },
    { user_command::carcouplingincrease, &TTrain::OnCommand_carcouplingincrease },
    { user_command::carcouplingdisconnect, &TTrain::OnCommand_carcouplingdisconnect },
    { user_command::departureannounce, &TTrain::OnCommand_departureannounce },
    { user_command::hornlowactivate, &TTrain::OnCommand_hornlowactivate },
    { user_command::hornhighactivate, &TTrain::OnCommand_hornhighactivate },
    { user_command::radiotoggle, &TTrain::OnCommand_radiotoggle },
    { user_command::radiochannelincrease, &TTrain::OnCommand_radiochannelincrease },
    { user_command::radiochanneldecrease, &TTrain::OnCommand_radiochanneldecrease },
    { user_command::radiostoptest, &TTrain::OnCommand_radiostoptest },
    { user_command::cabchangeforward, &TTrain::OnCommand_cabchangeforward },
    { user_command::cabchangebackward, &TTrain::OnCommand_cabchangebackward },
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
	PyDict_SetItemString( dict, "sanding", PyGetBool( mover->SandDose ));
    // electric current data
    PyDict_SetItemString( dict, "traction_voltage", PyGetFloat( mover->RunningTraction.TractionVoltage ) );
    PyDict_SetItemString( dict, "voltage", PyGetFloat( mover->Voltage ) );
    PyDict_SetItemString( dict, "im", PyGetFloat( mover->Im ) );
    PyDict_SetItemString( dict, "fuse", PyGetBool( mover->FuseFlag ) );
	PyDict_SetItemString( dict, "epfuse", PyGetBool( mover->EpFuse ));
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
		PyDict_SetItemString( dict, ( std::string( "slip_" ) + std::to_string( i + 1 )).c_str(), PyGetBool( bSlip[i]) );
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

// locates nearest vehicle belonging to the consist
TDynamicObject *
TTrain::find_nearest_consist_vehicle() const {

    if( false == FreeFlyModeFlag ) {
        return DynamicObject;
    }
    auto coupler { -2 }; // scan for vehicle, not any specific coupler
    auto *vehicle{ DynamicObject->ABuScanNearestObject( DynamicObject->GetTrack(), 1, 1500, coupler ) };
    if( vehicle == nullptr )
        vehicle = DynamicObject->ABuScanNearestObject( DynamicObject->GetTrack(), -1, 1500, coupler );
    // TBD, TODO: perform owner test for the located vehicle
    return vehicle;
}


// command handlers
void TTrain::OnCommand_aidriverenable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // on press
        if( Train->DynamicObject->Mechanik == nullptr ) { return; }

        if( true == Train->DynamicObject->Mechanik->AIControllFlag ) {
            //żeby nie trzeba było rozłączać dla zresetowania
            Train->DynamicObject->Mechanik->TakeControl( false );
        }
        Train->DynamicObject->Mechanik->TakeControl( true );
    }
}

void TTrain::OnCommand_aidriverdisable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // on press
        if( Train->DynamicObject->Mechanik )
            Train->DynamicObject->Mechanik->TakeControl( false );
    }
}

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
            Train->mvControlled->AnPos = clamp(
                Train->mvControlled->AnPos + ( Command.time_delta * 1.0f ),
                0.0, 1.0 );
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
            Train->mvControlled->AnPos = clamp(
                Train->mvControlled->AnPos - ( Command.time_delta * 1.0f ),
                0.0, 1.0 );
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

    if( false == FreeFlyModeFlag ) {
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
    else {
        // car brake handling, while in walk mode
        auto *vehicle { Train->find_nearest_consist_vehicle() };
        if( vehicle != nullptr ) {
            if( Command.action != GLFW_RELEASE ) {
                // press or hold
                vehicle->MoverParameters->BrakeReleaser( 1 );
            }
            else {
                // release
                vehicle->MoverParameters->BrakeReleaser( 0 );
            }
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
                    Train->dsbPneumaticSwitch.play();
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
                    Train->dsbPneumaticSwitch.play();
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
                Train->dsbPneumaticSwitch.play();
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
            Train->dsbPneumaticSwitch.play();
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
                Train->dsbPneumaticSwitch.play();
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
            Train->dsbPneumaticSwitch.play();
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
            Train->dsbPneumaticSwitch.play();
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
            Train->dsbPneumaticSwitch.play();
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
            Train->dsbPneumaticSwitch.play();
        }
        Train->mvOccupied->BrakeLevelSet( Train->mvOccupied->BrakeCtrlPosNo - 1 );
    }
}

void TTrain::OnCommand_trainbrakehandleoff( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

        Train->mvOccupied->BrakeLevelSet( Train->mvOccupied->Handle->GetPos( bh_NP ) );
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

void TTrain::OnCommand_trainbrakebasepressureincrease( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {

        switch( Train->mvOccupied->BrakeHandle ) {
            case FV4a: {
                Train->mvOccupied->BrakeCtrlPos2 = clamp( Train->mvOccupied->BrakeCtrlPos2 - 0.01, -1.5, 2.0 );
                break;
            }
            default: {
                Train->mvOccupied->BrakeLevelAdd( 0.01 );
                break;
            }
        }
    }
}

void TTrain::OnCommand_trainbrakebasepressuredecrease( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {

        switch( Train->mvOccupied->BrakeHandle ) {
            case FV4a: {
                Train->mvOccupied->BrakeCtrlPos2 = clamp( Train->mvOccupied->BrakeCtrlPos2 + 0.01, -1.5, 2.0 );
                break;
            }
            default: {
                Train->mvOccupied->BrakeLevelAdd( -0.01 );
                break;
            }
        }
    }
}

void TTrain::OnCommand_trainbrakebasepressurereset( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

        Train->mvOccupied->BrakeCtrlPos2 = 0;
    }
}

void TTrain::OnCommand_trainbrakeoperationtoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

        auto *vehicle { Train->find_nearest_consist_vehicle() };
        if( vehicle == nullptr ) { return; }

        vehicle->MoverParameters->Hamulec->SetBrakeStatus( vehicle->MoverParameters->Hamulec->GetBrakeStatus() ^ b_dmg );
    }
}

void TTrain::OnCommand_manualbrakeincrease( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {

        auto *vehicle { Train->find_nearest_consist_vehicle() };
        if( vehicle == nullptr ) { return; }

        if( ( vehicle->MoverParameters->LocalBrake == ManualBrake )
         || ( vehicle->MoverParameters->MBrake == true ) ) {

            vehicle->MoverParameters->IncManualBrakeLevel( 1 );
        }
    }
}

void TTrain::OnCommand_manualbrakedecrease( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {

        auto *vehicle { Train->find_nearest_consist_vehicle() };
        if( vehicle == nullptr ) { return; }

        if( ( vehicle->MoverParameters->LocalBrake == ManualBrake )
         || ( vehicle->MoverParameters->MBrake == true ) ) {

            vehicle->MoverParameters->DecManualBrakeLevel( 1 );
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

    if( Train->mvOccupied->BrakeSystem != ElectroPneumatic ) {
        // standard behaviour
        if( Command.action != GLFW_RELEASE ) {
            // press or hold
            Train->mvControlled->AntiSlippingBrake();
            // visual feedback
            Train->ggAntiSlipButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            // release
            // visual feedback
            Train->ggAntiSlipButton.UpdateValue( 0.0 );
        }
    }
    else {
        // electro-pneumatic, custom case
        if( Command.action != GLFW_RELEASE ) {
            // press or hold
            if( ( Train->mvOccupied->BrakeHandle == St113 )
             && ( Train->mvControlled->EpFuse == true ) ) {
                Train->mvOccupied->SwitchEPBrake( 1 );
            }
            // visual feedback
            Train->ggAntiSlipButton.UpdateValue( 1.0, Train->dsbPneumaticSwitch );
        }
        else {
            // release
            Train->mvOccupied->SwitchEPBrake( 0 );
            // visual feedback
            Train->ggAntiSlipButton.UpdateValue( 0.0 );
        }
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
                Train->dsbPneumaticSwitch.play();
                // visual feedback
                // NOTE: there's no button for ep brake control switch
                // TBD, TODO: add ep brake control switch?
            }
        }
        else {
            //turn off
            if( Train->mvOccupied->EpFuseSwitch( false ) ) {
                // audio feedback
                Train->dsbPneumaticSwitch.play();
                // visual feedback
                // NOTE: there's no button for ep brake control switch
                // TBD, TODO: add ep brake control switch?
            }
        }
    }
}

void TTrain::OnCommand_brakeactingspeedincrease( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

        auto *vehicle { Train->find_nearest_consist_vehicle() };
        if( vehicle == nullptr ) { return; }

        if( ( vehicle->MoverParameters->BrakeDelayFlag & bdelay_M ) != 0 ) {
            // can't speed it up any more than this
            return;
        }
        auto const fasterbrakesetting = (
            vehicle->MoverParameters->BrakeDelayFlag < bdelay_R ?
                vehicle->MoverParameters->BrakeDelayFlag << 1 :
                vehicle->MoverParameters->BrakeDelayFlag | bdelay_M );

        if( true == vehicle->MoverParameters->BrakeDelaySwitch( fasterbrakesetting ) ) {

            if( vehicle == Train->DynamicObject ) {
                // visual feedback
                // TODO: add setting indicator to vehicle
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
}

void TTrain::OnCommand_brakeactingspeeddecrease( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

        auto *vehicle { Train->find_nearest_consist_vehicle() };
        if( vehicle == nullptr ) { return; }

        if( vehicle->MoverParameters->BrakeDelayFlag == bdelay_G ) {
            // can't slow it down any more than this
            return;
        }
        auto const slowerbrakesetting = (
            vehicle->MoverParameters->BrakeDelayFlag < bdelay_M ?
                vehicle->MoverParameters->BrakeDelayFlag >> 1 :
                vehicle->MoverParameters->BrakeDelayFlag ^ bdelay_M );

        if( true == vehicle->MoverParameters->BrakeDelaySwitch( slowerbrakesetting ) ) {

            if( vehicle == Train->DynamicObject ) {
                // visual feedback
                // TODO: add setting indicator to vehicle
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
}

void TTrain::OnCommand_brakeloadcompensationincrease( TTrain *Train, command_data const &Command ) {

    if( ( true == FreeFlyModeFlag )
     && ( Command.action == GLFW_PRESS ) ) {
        auto *vehicle { Train->find_nearest_consist_vehicle() };
        if( vehicle != nullptr ) {
            vehicle->MoverParameters->IncBrakeMult();
        }
    }
}

void TTrain::OnCommand_brakeloadcompensationdecrease( TTrain *Train, command_data const &Command ) {

    if( ( true == FreeFlyModeFlag )
     && ( Command.action == GLFW_PRESS ) ) {
        auto *vehicle { Train->find_nearest_consist_vehicle() };
        if( vehicle != nullptr ) {
            vehicle->MoverParameters->DecBrakeMult();
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

void TTrain::OnCommand_batterytoggle( TTrain *Train, command_data const &Command ) {

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

void TTrain::OnCommand_pantographtogglefront( TTrain *Train, command_data const &Command ) {

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

void TTrain::OnCommand_pantographtogglerear( TTrain *Train, command_data const &Command ) {

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
        if( Train->m_linebreakerstate == 0 ) {
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
                if( ( Train->mvControlled->EngineType == DieselEngine )
                 || ( Train->mvControlled->EngineType == DieselElectric ) ) {
                    if( Train->mvControlled->MainSwitch( true ) ) {
                        // side-effects
                        Train->mvControlled->ConverterSwitch( ( Train->ggConverterButton.GetValue() > 0.5 ) || ( Train->mvControlled->ConverterStart == start::automatic ) );
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
                    // visual feedback
                    Train->ggMainOffButton.UpdateValue( 1.0, Train->dsbSwitch );
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
                        // visual feedback
                        Train->ggMainButton.UpdateValue( 0.0, Train->dsbSwitch );
                    }
                }
            }
            // play sound immediately when the switch is hit, not after release
            if( Train->fMainRelayTimer > 0.0f ) {
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
            if( ( Train->mvControlled->EngineType != DieselEngine )
             && ( Train->mvControlled->EngineType != DieselElectric ) ) {
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

void TTrain::OnCommand_motorconnectorsopen( TTrain *Train, command_data const &Command ) {

    // TODO: don't rely on presense of 3d model to determine presence of the switch
    if( Train->ggStLinOffButton.SubModel == nullptr ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Open Motor Power Connectors button is missing, or wasn't defined" );
        }
        return;
    }
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

void TTrain::OnCommand_motoroverloadrelaythresholdtoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( true == Train->mvControlled->ShuntModeAllow ?
                ( false == Train->mvControlled->ShuntMode ) :
                ( Train->mvControlled->Imax < Train->mvControlled->ImaxHi ) ) ) {
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

void TTrain::OnCommand_lightspresetactivatenext( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo == 0 ) {
        // lights are controlled by preset selector
        return;
    }
    if( Command.action != GLFW_PRESS ) {
        // one change per key press
        return;
    }

    if( ( Train->mvOccupied->LightsPos < Train->mvOccupied->LightsPosNo )
     || ( true == Train->mvOccupied->LightsWrap ) ) {
        // active light preset is stored as value in range 1-LigthPosNo
        Train->mvOccupied->LightsPos = (
            Train->mvOccupied->LightsPos < Train->mvOccupied->LightsPosNo ?
                Train->mvOccupied->LightsPos + 1 :
                1 ); // wrap mode

        Train->SetLights();
        // visual feedback
        if( Train->ggLightsButton.SubModel != nullptr ) {
            Train->ggLightsButton.UpdateValue( Train->mvOccupied->LightsPos - 1, Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_lightspresetactivateprevious( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo == 0 ) {
        // lights are controlled by preset selector
        return;
    }
    if( Command.action != GLFW_PRESS ) {
        // one change per key press
        return;
    }

    if( ( Train->mvOccupied->LightsPos > 1 )
     || ( true == Train->mvOccupied->LightsWrap ) ) {
        // active light preset is stored as value in range 1-LigthPosNo
        Train->mvOccupied->LightsPos = (
            Train->mvOccupied->LightsPos > 1 ?
                Train->mvOccupied->LightsPos - 1 :
                Train->mvOccupied->LightsPosNo ); // wrap mode

        Train->SetLights();
        // visual feedback
        if( Train->ggLightsButton.SubModel != nullptr ) {
            Train->ggLightsButton.UpdateValue( Train->mvOccupied->LightsPos - 1, Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_headlighttoggleleft( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    int const vehicleside =
        ( Train->mvOccupied->ActiveCab == 1 ?
            side::front :
            side::rear );

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->DynamicObject->iLights[ vehicleside ] & light::headlight_left ) == 0 ) {
            // turn on
            Train->DynamicObject->iLights[ vehicleside ] ^= light::headlight_left;
            // visual feedback
            Train->ggLeftLightButton.UpdateValue( 1.0, Train->dsbSwitch );
            // if the light is controlled by 3-way switch, disable marker light
            if( Train->ggLeftEndLightButton.SubModel == nullptr ) {
                Train->DynamicObject->iLights[ vehicleside ] &= ~light::redmarker_left;
            }
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ vehicleside ] ^= light::headlight_left;
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

    int const vehicleside =
        ( Train->mvOccupied->ActiveCab == 1 ?
            side::front :
            side::rear );

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->DynamicObject->iLights[ vehicleside ] & light::headlight_right ) == 0 ) {
            // turn on
            Train->DynamicObject->iLights[ vehicleside ] ^= light::headlight_right;
            // visual feedback
            Train->ggRightLightButton.UpdateValue( 1.0, Train->dsbSwitch );
            // if the light is controlled by 3-way switch, disable marker light
            if( Train->ggRightEndLightButton.SubModel == nullptr ) {
                Train->DynamicObject->iLights[ vehicleside ] &= ~light::redmarker_right;
            }
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ vehicleside ] ^= light::headlight_right;
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

    int const vehicleside =
        ( Train->mvOccupied->ActiveCab == 1 ?
            side::front :
            side::rear );

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->DynamicObject->iLights[ vehicleside ] & light::headlight_upper ) == 0 ) {
            // turn on
            Train->DynamicObject->iLights[ vehicleside ] ^= light::headlight_upper;
            // visual feedback
            Train->ggUpperLightButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ vehicleside ] ^= light::headlight_upper;
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

    int const vehicleside =
        ( Train->mvOccupied->ActiveCab == 1 ?
            side::front :
            side::rear );

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->DynamicObject->iLights[ vehicleside ] & light::redmarker_left ) == 0 ) {
            // turn on
            Train->DynamicObject->iLights[ vehicleside ] ^= light::redmarker_left;
            // visual feedback
            if( Train->ggLeftEndLightButton.SubModel != nullptr ) {
                Train->ggLeftEndLightButton.UpdateValue( 1.0, Train->dsbSwitch );
            }
            else {
                // we interpret lack of dedicated switch as a sign the light is controlled with 3-way switch
                // this is crude, but for now will do
                Train->ggLeftLightButton.UpdateValue( -1.0, Train->dsbSwitch );
                // if the light is controlled by 3-way switch, disable the headlight
                Train->DynamicObject->iLights[ vehicleside ] &= ~light::headlight_left;
            }
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ vehicleside ] ^= light::redmarker_left;
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

    int const vehicleside =
        ( Train->mvOccupied->ActiveCab == 1 ?
            side::front :
            side::rear );

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->DynamicObject->iLights[ vehicleside ] & light::redmarker_right ) == 0 ) {
            // turn on
            Train->DynamicObject->iLights[ vehicleside ] ^= light::redmarker_right;
            // visual feedback
            if( Train->ggRightEndLightButton.SubModel != nullptr ) {
                Train->ggRightEndLightButton.UpdateValue( 1.0, Train->dsbSwitch );
            }
            else {
                // we interpret lack of dedicated switch as a sign the light is controlled with 3-way switch
                // this is crude, but for now will do
                Train->ggRightLightButton.UpdateValue( -1.0, Train->dsbSwitch );
                // if the light is controlled by 3-way switch, disable the headlight
                Train->DynamicObject->iLights[ vehicleside ] &= ~light::headlight_right;
            }
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ vehicleside ] ^= light::redmarker_right;
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

    int const vehicleside =
        ( Train->mvOccupied->ActiveCab == 1 ?
            side::rear :
            side::front );

    if( Command.action == GLFW_PRESS ) {
        // NOTE: we toggle the light on opposite side, as 'rear right' is 'front left' on the rear end etc
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->DynamicObject->iLights[ vehicleside ] & light::headlight_right ) == 0 ) {
            // turn on
            Train->DynamicObject->iLights[ vehicleside ] ^= light::headlight_right;
            // visual feedback
            Train->ggRearLeftLightButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ vehicleside ] ^= light::headlight_right;
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

    int const vehicleside =
        ( Train->mvOccupied->ActiveCab == 1 ?
            side::rear :
            side::front );

    if( Command.action == GLFW_PRESS ) {
        // NOTE: we toggle the light on opposite side, as 'rear right' is 'front left' on the rear end etc
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->DynamicObject->iLights[ vehicleside ] & light::headlight_left ) == 0 ) {
            // turn on
            Train->DynamicObject->iLights[ vehicleside ] ^= light::headlight_left;
            // visual feedback
            Train->ggRearRightLightButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ vehicleside ] ^= light::headlight_left;
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

    int const vehicleside =
        ( Train->mvOccupied->ActiveCab == 1 ?
            side::rear :
            side::front );

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->DynamicObject->iLights[ vehicleside ] & light::headlight_upper ) == 0 ) {
            // turn on
            Train->DynamicObject->iLights[ vehicleside ] ^= light::headlight_upper;
            // visual feedback
            Train->ggRearUpperLightButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ vehicleside ] ^= light::headlight_upper;
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

    int const vehicleside =
        ( Train->mvOccupied->ActiveCab == 1 ?
            side::rear :
            side::front );

    if( Command.action == GLFW_PRESS ) {
        // NOTE: we toggle the light on opposite side, as 'rear right' is 'front left' on the rear end etc
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->DynamicObject->iLights[ vehicleside ] & light::redmarker_right ) == 0 ) {
            // turn on
            Train->DynamicObject->iLights[ vehicleside ] ^= light::redmarker_right;
            // visual feedback
            Train->ggRearLeftEndLightButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ vehicleside ] ^= light::redmarker_right;
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

    int const vehicleside =
        ( Train->mvOccupied->ActiveCab == 1 ?
            side::rear :
            side::front );

    if( Command.action == GLFW_PRESS ) {
        // NOTE: we toggle the light on opposite side, as 'rear right' is 'front left' on the rear end etc
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->DynamicObject->iLights[ vehicleside ] & light::redmarker_left ) == 0 ) {
            // turn on
            Train->DynamicObject->iLights[ vehicleside ] ^= light::redmarker_left;
            // visual feedback
            Train->ggRearRightEndLightButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            //turn off
            Train->DynamicObject->iLights[ vehicleside ] ^= light::redmarker_left;
            // visual feedback
            Train->ggRearRightEndLightButton.UpdateValue( 0.0, Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_redmarkerstoggle( TTrain *Train, command_data const &Command ) {

    if( ( true == FreeFlyModeFlag )
     && ( Command.action == GLFW_PRESS ) ) {

        auto *vehicle { std::get<TDynamicObject *>( simulation::Region->find_vehicle( Global::pCameraPosition, 10, false, true ) ) };

        if( vehicle == nullptr ) { return; }

        int const CouplNr {
            clamp(
                vehicle->DirectionGet()
                * ( LengthSquared3( vehicle->HeadPosition() - Global::pCameraPosition ) > LengthSquared3( vehicle->RearPosition() - Global::pCameraPosition ) ?
                     1 :
                    -1 ),
                0, 1 ) }; // z [-1,1] zrobić [0,1]

        auto const lightset { light::redmarker_left | light::redmarker_right };

        vehicle->iLights[ CouplNr ] = (
            false == TestFlag( vehicle->iLights[ CouplNr ], lightset ) ?
                vehicle->iLights[ CouplNr ] |= lightset : // turn signals on
                vehicle->iLights[ CouplNr ] ^= lightset ); // turn signals off
    }
}

void TTrain::OnCommand_endsignalstoggle( TTrain *Train, command_data const &Command ) {

    if( ( true == FreeFlyModeFlag )
     && ( Command.action == GLFW_PRESS ) ) {

        auto *vehicle { std::get<TDynamicObject *>( simulation::Region->find_vehicle( Global::pCameraPosition, 10, false, true ) ) };

        if( vehicle == nullptr ) { return; }

        int const CouplNr {
            clamp(
                vehicle->DirectionGet()
                * ( LengthSquared3( vehicle->HeadPosition() - Global::pCameraPosition ) > LengthSquared3( vehicle->RearPosition() - Global::pCameraPosition ) ?
                     1 :
                    -1 ),
                0, 1 ) }; // z [-1,1] zrobić [0,1]

        auto const lightset { light::rearendsignals };

        vehicle->iLights[ CouplNr ] = (
            false == TestFlag( vehicle->iLights[ CouplNr ], lightset ) ?
                vehicle->iLights[ CouplNr ] |= lightset : // turn signals on
                vehicle->iLights[ CouplNr ] ^= lightset ); // turn signals off
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
            Train->btCabLight.Turn( true );
            // visual feedback
            Train->ggCabLightButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            //turn off
            Train->bCabLight = false;
            Train->btCabLight.Turn( false );
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
                }
            }
            else {
                // in the rear cab sides are reversed
                if( Train->mvOccupied->DoorRight( true ) ) {
                    Train->ggDoorRightButton.UpdateValue( 1.0, Train->dsbSwitch );
                }
            }
        }
        else {
            // close
            if( Train->mvOccupied->ActiveCab == 1 ) {
                if( Train->mvOccupied->DoorLeft( false ) ) {
                    Train->ggDoorLeftButton.UpdateValue( 0.0, Train->dsbSwitch );
                }
            }
            else {
                // in the rear cab sides are reversed
                if( Train->mvOccupied->DoorRight( false ) ) {
                    Train->ggDoorRightButton.UpdateValue( 0.0, Train->dsbSwitch );
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
                }
            }
            else {
                // in the rear cab sides are reversed
                if( Train->mvOccupied->DoorLeft( true ) ) {
                    Train->ggDoorLeftButton.UpdateValue( 1.0, Train->dsbSwitch );
                }
            }
        }
        else {
            // close
            if( Train->mvOccupied->ActiveCab == 1 ) {
                if( Train->mvOccupied->DoorRight( false ) ) {
                    Train->ggDoorRightButton.UpdateValue( 0.0, Train->dsbSwitch );
                }
            }
            else {
                // in the rear cab sides are reversed
                if( Train->mvOccupied->DoorLeft( false ) ) {
                    Train->ggDoorLeftButton.UpdateValue( 0.0, Train->dsbSwitch );
                }
            }
        }
    }
}

void TTrain::OnCommand_carcouplingincrease( TTrain *Train, command_data const &Command ) {

    if( true == FreeFlyModeFlag ) {
        // tryb freefly
        auto coupler { -1 };
        auto *vehicle { Train->DynamicObject->ABuScanNearestObject( Train->DynamicObject->GetTrack(), 1, 1500, coupler ) };
        if( vehicle == nullptr )
            vehicle = Train->DynamicObject->ABuScanNearestObject( Train->DynamicObject->GetTrack(), -1, 1500, coupler );

        if( ( coupler != -1 )
         && ( vehicle != nullptr ) ) {

            vehicle->couple( coupler );
        }
        if( Train->DynamicObject->Mechanik ) {
            // aktualizacja flag kierunku w składzie
            Train->DynamicObject->Mechanik->CheckVehicles( Connect );
        }
    }
}

void TTrain::OnCommand_carcouplingdisconnect( TTrain *Train, command_data const &Command ) {

    if( true == FreeFlyModeFlag ) {
        // tryb freefly
        auto coupler { -1 };
        auto *vehicle { Train->DynamicObject->ABuScanNearestObject( Train->DynamicObject->GetTrack(), 1, 1500, coupler ) };
        if( vehicle == nullptr )
            vehicle = Train->DynamicObject->ABuScanNearestObject( Train->DynamicObject->GetTrack(), -1, 1500, coupler );

        if( ( coupler != -1 )
         && ( vehicle != nullptr ) ) {

            vehicle->uncouple( coupler );
        }
        if( Train->DynamicObject->Mechanik ) {
            // aktualizacja flag kierunku w składzie
            Train->DynamicObject->Mechanik->CheckVehicles( Disconnect );
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
        if( false == Train->mvOccupied->DepartureSignal ) {
            // turn on
            Train->mvOccupied->signal_departure( true );
            // visual feedback
            Train->ggDepartureSignalButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // turn off
        Train->mvOccupied->signal_departure( false );
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

void TTrain::OnCommand_radiochannelincrease( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        Train->iRadioChannel = clamp( Train->iRadioChannel + 1, 1, 10 );
    }
}

void TTrain::OnCommand_radiochanneldecrease( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        Train->iRadioChannel = clamp( Train->iRadioChannel - 1, 1, 10 );
    }
}


void TTrain::OnCommand_radiostoptest( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        Train->Dynamic()->RadioStop();
    }
}

void TTrain::OnCommand_cabchangeforward( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        if( false == Train->CabChange( 1 ) ) {
            if( TestFlag( Train->DynamicObject->MoverParameters->Couplers[ side::front ].CouplingFlag, coupling::gangway ) ) {
                // przejscie do nastepnego pojazdu
                Global::changeDynObj = Train->DynamicObject->PrevConnected;
                Global::changeDynObj->MoverParameters->ActiveCab = (
                    Train->DynamicObject->PrevConnectedNo ?
                    -1 :
                     1 );
            }
        }
    }
}

void TTrain::OnCommand_cabchangebackward( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        if( false == Train->CabChange( -1 ) ) {
            if( TestFlag( Train->DynamicObject->MoverParameters->Couplers[ side::rear ].CouplingFlag, coupling::gangway ) ) {
                // przejscie do nastepnego pojazdu
                Global::changeDynObj = Train->DynamicObject->NextConnected;
                Global::changeDynObj->MoverParameters->ActiveCab = (
                    Train->DynamicObject->NextConnectedNo ?
                    -1 :
                     1 );
            }
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

    vector3 position = DynamicObject->mMatrix * pMechPosition; // położenie względem środka pojazdu w układzie scenerii
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
                fTachoCount += Deltatime * 3; // szybciej zacznij stukac
        }
        else if (fTachoCount > 0)
            fTachoCount -= Deltatime * 0.66; // schodz powoli - niektore haslery to ze 4
        // sekundy potrafia stukac

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
                cCode[i] = p->MoverParameters->TypeName[p->MoverParameters->TypeName.length() - 1];
                asCarName[i] = p->name();
				bPants[iUnitNo - 1][0] = (bPants[iUnitNo - 1][0] || p->MoverParameters->PantFrontUp);
                bPants[iUnitNo - 1][1] = (bPants[iUnitNo - 1][1] || p->MoverParameters->PantRearUp);
				bComp[iUnitNo - 1][0] = (bComp[iUnitNo - 1][0] || p->MoverParameters->CompressorAllow);
				bSlip[i] = p->MoverParameters->SlippingWheels;
                if (p->MoverParameters->CompressorSpeed > 0.00001)
                {
					bComp[iUnitNo - 1][1] = (bComp[iUnitNo - 1][1] || p->MoverParameters->CompressorFlag);
                }
                if ((in < 8) && (p->MoverParameters->eimc[eimc_p_Pmax] > 1))
                {
                    fEIMParams[1 + in][0] = p->MoverParameters->eimv[eimv_Fmax];
                    fEIMParams[1 + in][1] = Max0R(fEIMParams[1 + in][0], 0);
                    fEIMParams[1 + in][2] = -Min0R(fEIMParams[1 + in][0], 0);
                    fEIMParams[1 + in][3] = p->MoverParameters->eimv[eimv_Fmax] /
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
				bSlip[i] = false;
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
            fConverterTimer += Deltatime;
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

        if (mvControlled->SlippingWheels) {
            // Ra 2014-12: lokomotywy 181/182 dostają SlippingWheels po zahamowaniu powyżej 2.85 bara i buczały
            double veldiff = (DynamicObject->GetVelocity() - fTachoVelocity) / mvControlled->Vmax;
            if( veldiff < -0.01 ) {
                // 1% Vmax rezerwy, żeby 181/182 nie buczały po zahamowaniu, ale to proteza
                if( std::abs( mvControlled->Im ) > 10.0 ) {
                    btLampkaPoslizg.Turn( true );
                }
            }
        }
        else {
            btLampkaPoslizg.Turn( false );
        }

        if ((mvControlled->Mains) || (mvControlled->TrainType == dt_EZT))
        {
            btLampkaNadmSil.Turn(mvControlled->FuseFlagCheck());
        }
        else
        {
            btLampkaNadmSil.Turn( false );
        }

        if (mvControlled->Battery || mvControlled->ConverterFlag)
        {
            btLampkaWylSzybki.Turn(
                ( ( (m_linebreakerstate > 0)
                 || (true == mvControlled->Mains) ) ?
                    true :
                    false ) );
            // NOTE: 'off' variant uses the same test, but opposite resulting states
            btLampkaWylSzybkiOff.Turn(
                ( ( ( m_linebreakerstate > 0 )
                 || ( true == mvControlled->Mains ) ) ?
                    false :
                    true ) );

            btLampkaPrzetw.Turn( mvControlled->ConverterFlag );
            btLampkaPrzetwOff.Turn( false == mvControlled->ConverterFlag );
            btLampkaNadmPrzetw.Turn( mvControlled->ConvOvldFlag );

            btLampkaOpory.Turn(
                mvControlled->StLinFlag ?
                    mvControlled->ResistorsFlagCheck() :
                    false );

            btLampkaBezoporowa.Turn( mvControlled->ResistorsFlagCheck() || ( mvControlled->MainCtrlActualPos == 0 ) ); // do EU04
            if( ( mvControlled->Itot != 0 )
             || ( mvOccupied->BrakePress > 2 )
             || ( mvOccupied->PipePress < 3.6 ) ) {
                // Ra: czy to jest udawanie działania styczników liniowych?
                btLampkaStyczn.Turn( false );
            }
            else if( mvOccupied->BrakePress < 1 )
                btLampkaStyczn.Turn( true ); // mozna prowadzic rozruch
            if( ( ( TestFlag( mvControlled->Couplers[ 1 ].CouplingFlag, ctrain_controll ) ) && ( mvControlled->CabNo == 1 ) ) ||
                ( ( TestFlag( mvControlled->Couplers[ 0 ].CouplingFlag, ctrain_controll ) ) && ( mvControlled->CabNo == -1 ) ) )
                btLampkaUkrotnienie.Turn( true );
            else
                btLampkaUkrotnienie.Turn( false );

            //         if
            //         ((TestFlag(mvControlled->BrakeStatus,+b_Rused+b_Ractive)))//Lampka drugiego stopnia hamowania
            btLampkaHamPosp.Turn((TestFlag(mvOccupied->Hamulec->GetBrakeStatus(), 1))); // lampka drugiego stopnia hamowania
            //TODO: youBy wyciągnąć flagę wysokiego stopnia

            // hunter-121211: lampka zanikowo-pradowego wentylatorow:
            btLampkaNadmWent.Turn( ( mvControlled->RventRot < 5.0 ) && ( mvControlled->ResistorsFlagCheck() ) );
            //-------

            btLampkaWysRozr.Turn(!(mvControlled->Imax < mvControlled->ImaxHi));

            if( ( false == mvControlled->DelayCtrlFlag )
             && ( ( mvControlled->ScndCtrlActualPos > 0 )
               || ( ( mvControlled->RList[ mvControlled->MainCtrlActualPos ].ScndAct != 0 )
                 && ( mvControlled->RList[ mvControlled->MainCtrlActualPos ].ScndAct != 255 ) ) ) ) {
                btLampkaBoczniki.Turn( true );
            }
            else {
                btLampkaBoczniki.Turn( false );
            }

            btLampkaNapNastHam.Turn(mvControlled->ActiveDir != 0); // napiecie na nastawniku hamulcowym
            btLampkaSprezarka.Turn(mvControlled->CompressorFlag); // mutopsitka dziala
            btLampkaSprezarkaOff.Turn( false == mvControlled->CompressorFlag );
            // boczniki
            unsigned char scp; // Ra: dopisałem "unsigned"
            // Ra: w SU45 boczniki wchodzą na MainCtrlPos, a nie na MainCtrlActualPos
            // - pokićkał ktoś?
            scp = mvControlled->RList[mvControlled->MainCtrlPos].ScndAct;
            scp = (scp == 255 ? 0 : scp); // Ra: whatta hella is this?
            if ((mvControlled->ScndCtrlPos > 0) || (mvControlled->ScndInMain != 0) && (scp > 0))
            { // boczniki pojedynczo
                btLampkaBocznik1.Turn( true );
                btLampkaBocznik2.Turn(mvControlled->ScndCtrlPos > 1);
                btLampkaBocznik3.Turn(mvControlled->ScndCtrlPos > 2);
                btLampkaBocznik4.Turn(mvControlled->ScndCtrlPos > 3);
            }
            else
            { // wyłączone wszystkie cztery
                btLampkaBocznik1.Turn( false );
                btLampkaBocznik2.Turn( false );
                btLampkaBocznik3.Turn( false );
                btLampkaBocznik4.Turn( false );
            }
        }
        else {
            // wylaczone
            btLampkaWylSzybki.Turn( false );
            btLampkaWylSzybkiOff.Turn( false );
            btLampkaWysRozr.Turn( false );
            btLampkaOpory.Turn( false );
            btLampkaStyczn.Turn( false );
            btLampkaUkrotnienie.Turn( false );
            btLampkaHamPosp.Turn( false );
            btLampkaBoczniki.Turn( false );
            btLampkaNapNastHam.Turn( false );
            btLampkaPrzetw.Turn( false );
            btLampkaPrzetwOff.Turn( false );
            btLampkaNadmPrzetw.Turn( false );
            btLampkaSprezarka.Turn( false );
            btLampkaSprezarkaOff.Turn( false );
            btLampkaBezoporowa.Turn( false );
        }
        if (mvControlled->Signalling == true) {

            if( ( mvOccupied->BrakePress >= 0.145f )
             && ( mvControlled->Battery == true )
             && ( mvControlled->Signalling == true ) ) {

                btLampkaHamowanie1zes.Turn( true );
            }
            if (mvControlled->BrakePress < 0.075f) {

                btLampkaHamowanie1zes.Turn( false );
            }
        }
        else {

            btLampkaHamowanie1zes.Turn( false );
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
                if ( mvControlled->Battery || mvControlled->ConverterFlag ) {

                    btLampkaWylSzybkiB.Turn( tmp->MoverParameters->Mains );
                    btLampkaWylSzybkiBOff.Turn( false == tmp->MoverParameters->Mains );

                    btLampkaOporyB.Turn(tmp->MoverParameters->ResistorsFlagCheck());
                    btLampkaBezoporowaB.Turn(
                        ( true == tmp->MoverParameters->ResistorsFlagCheck() )
                     || ( tmp->MoverParameters->MainCtrlActualPos == 0 ) ); // do EU04

                    if( ( tmp->MoverParameters->Itot != 0 )
                     || ( tmp->MoverParameters->BrakePress > 0.2 )
                     || ( tmp->MoverParameters->PipePress < 0.36 ) ) {
                        btLampkaStycznB.Turn( false );
                    }
                    else if( tmp->MoverParameters->BrakePress < 0.1 ) {
                        btLampkaStycznB.Turn( true ); // mozna prowadzic rozruch
                    }

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

                    btLampkaSprezarkaB.Turn( tmp->MoverParameters->CompressorFlag ); // mutopsitka dziala
                    btLampkaSprezarkaBOff.Turn( false == tmp->MoverParameters->CompressorFlag );
                    if ((tmp->MoverParameters->BrakePress >= 0.145f * 10) &&
                        (mvControlled->Battery == true) && (mvControlled->Signalling == true))
                    {
                        btLampkaHamowanie2zes.Turn( true );
                    }
                    if ((tmp->MoverParameters->BrakePress < 0.075f * 10) ||
                        (mvControlled->Battery == false) || (mvControlled->Signalling == false))
                    {
                        btLampkaHamowanie2zes.Turn( false );
                    }
                    btLampkaNadmPrzetwB.Turn( tmp->MoverParameters->ConvOvldFlag ); // nadmiarowy przetwornicy?
                    btLampkaPrzetwB.Turn( tmp->MoverParameters->ConverterFlag ); // zalaczenie przetwornicy
                    btLampkaPrzetwBOff.Turn( false == tmp->MoverParameters->ConverterFlag );
                }
                else // wylaczone
                {
                    btLampkaWylSzybkiB.Turn( false );
                    btLampkaWylSzybkiBOff.Turn( false );
                    btLampkaOporyB.Turn( false );
                    btLampkaStycznB.Turn( false );
                    btLampkaSprezarkaB.Turn( false );
                    btLampkaSprezarkaBOff.Turn( false );
                    btLampkaBezoporowaB.Turn( false );
                    btLampkaHamowanie2zes.Turn( false );
                    btLampkaNadmPrzetwB.Turn( false );
                    btLampkaPrzetwB.Turn( false );
                    btLampkaPrzetwBOff.Turn( false );
                }
        }

        if( mvControlled->Battery || mvControlled->ConverterFlag ) {
            switch (mvControlled->TrainType) {
                // zależnie od typu lokomotywy
                case dt_EZT: {
                    btLampkaHamienie.Turn( ( mvControlled->BrakePress >= 0.2 ) && mvControlled->Signalling );
                    break;
                }
                case dt_ET41: {
                    // odhamowanie drugiego członu
                    if( mvSecond ) {
                        // bo może komuś przyjść do głowy jeżdżenie jednym członem
                        btLampkaHamienie.Turn( mvSecond->BrakePress < 0.4 );
                    }
                    break;
                }
                default: {
                    btLampkaHamienie.Turn( ( mvOccupied->BrakePress >= 0.1 ) || mvControlled->DynamicBrakeFlag );
                    btLampkaBrakingOff.Turn( ( mvOccupied->BrakePress < 0.1 ) && ( false == mvControlled->DynamicBrakeFlag ) );
                    break;
                }
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
            // light indicators
            // NOTE: sides are hardcoded to deal with setups where single cab is equipped with all indicators
            btLampkaUpperLight.Turn( ( mvOccupied->iLights[ side::front ] & light::headlight_upper ) != 0 );
            btLampkaLeftLight.Turn( ( mvOccupied->iLights[ side::front ] & light::headlight_left ) != 0 );
            btLampkaRightLight.Turn( ( mvOccupied->iLights[ side::front ] & light::headlight_right ) != 0 );
            btLampkaLeftEndLight.Turn( ( mvOccupied->iLights[ side::front ] & light::redmarker_left ) != 0 );
            btLampkaRightEndLight.Turn( ( mvOccupied->iLights[ side::front ] & light::redmarker_right ) != 0 );
            btLampkaRearUpperLight.Turn( ( mvOccupied->iLights[ side::rear ] & light::headlight_upper ) != 0 );
            btLampkaRearLeftLight.Turn( ( mvOccupied->iLights[ side::rear ] & light::headlight_left ) != 0 );
            btLampkaRearRightLight.Turn( ( mvOccupied->iLights[ side::rear ] & light::headlight_right ) != 0 );
            btLampkaRearLeftEndLight.Turn( ( mvOccupied->iLights[ side::rear ] & light::redmarker_left ) != 0 );
            btLampkaRearRightEndLight.Turn( ( mvOccupied->iLights[ side::rear ] & light::redmarker_right ) != 0 );
        }
        else
        { // gdy bateria wyłączona
            btLampkaHamienie.Turn( false );
            btLampkaBrakingOff.Turn( false );
            btLampkaMaxSila.Turn( false );
            btLampkaPrzekrMaxSila.Turn( false );
            btLampkaRadio.Turn( false );
            btLampkaHamulecReczny.Turn( false );
            btLampkaDoorLeft.Turn( false );
            btLampkaDoorRight.Turn( false );
            btLampkaDepartureSignal.Turn( false );
            btLampkaNapNastHam.Turn( false );
            btLampkaForward.Turn( false );
            btLampkaBackward.Turn( false );
            btLampkaED.Turn( false );
            // light indicators
            btLampkaUpperLight.Turn( false );
            btLampkaLeftLight.Turn( false );
            btLampkaRightLight.Turn( false );
            btLampkaLeftEndLight.Turn( false );
            btLampkaRightEndLight.Turn( false );
            btLampkaRearUpperLight.Turn( false );
            btLampkaRearLeftLight.Turn( false );
            btLampkaRearRightLight.Turn( false );
            btLampkaRearLeftEndLight.Turn( false );
            btLampkaRearRightEndLight.Turn( false );
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
            if (DynamicObject->Mechanik ?
                    (DynamicObject->Mechanik->AIControllFlag ? false : 
						(Global::iFeedbackMode == 4 || (Global::bMWDmasterEnable && Global::bMWDBreakEnable))) :
                    false) // nie blokujemy AI
            { // Ra: nie najlepsze miejsce, ale na początek gdzieś to dać trzeba
				// Firleju: dlatego kasujemy i zastepujemy funkcją w Console
				if (mvOccupied->BrakeHandle == FV4a)
                {
                    double b = Console::AnalogCalibrateGet(0);
					b = b * 8.0 - 2.0;
                    b = clamp<double>( b, -2.0, mvOccupied->BrakeCtrlPosNo ); // przycięcie zmiennej do granic
					if (Global::bMWDdebugEnable && Global::iMWDDebugMode & 4) WriteLog("FV4a break position = " + to_string(b));
					ggBrakeCtrl.UpdateValue(b); // przesów bez zaokrąglenia
					mvOccupied->BrakeLevelSet(b);
				}
                if (mvOccupied->BrakeHandle == FVel6) // może można usunąć ograniczenie do FV4a i FVel6?
                {
                    double b = Console::AnalogCalibrateGet(0);
					b = b * 7.0 - 1.0;
                    b = clamp<double>( b, -1.0, mvOccupied->BrakeCtrlPosNo ); // przycięcie zmiennej do granic
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

        if( ggLocalBrake.SubModel ) {

            if( ( DynamicObject->Mechanik != nullptr )
             && ( false == DynamicObject->Mechanik->AIControllFlag ) // nie blokujemy AI
             && ( mvOccupied->BrakeLocHandle == FD1 )
             && ( ( Global::iFeedbackMode == 4 )
               || ( Global::bMWDmasterEnable && Global::bMWDBreakEnable ) ) ) {
                // Ra: nie najlepsze miejsce, ale na początek gdzieś to dać trzeba
                // Firleju: dlatego kasujemy i zastepujemy funkcją w Console
                auto const b = clamp<double>(
                    Console::AnalogCalibrateGet( 1 ) * 10.0,
                    0.0,
                    ManualBrakePosNo );
                ggLocalBrake.UpdateValue( b ); // przesów bez zaokrąglenia
                mvOccupied->LocalBrakePos = int( 1.09 * b ); // sposób zaokrąglania jest do ustalenia
                if( ( true == Global::bMWDdebugEnable )
                 && ( ( Global::iMWDDebugMode & 4 ) != 0 ) ) {
                    WriteLog( "FD1 break position = " + to_string( b ) );
                }
            }
            else {
                // standardowa prodedura z kranem powiązanym z klawiaturą
                ggLocalBrake.UpdateValue( double( mvOccupied->LocalBrakePos ) );
            }
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

        //---------
        // hunter-080812: poprawka na ogrzewanie w elektrykach - usuniete uzaleznienie od przetwornicy
        if( ( mvControlled->Heating == true )
         && ( ( mvControlled->ConverterFlag )
           || ( ( mvControlled->EngineType == ElectricSeriesMotor )
             && ( mvControlled->Mains == true )
             && ( mvControlled->ConvOvldFlag == false ) ) ) )
            btLampkaOgrzewanieSkladu.Turn( true );
        else
            btLampkaOgrzewanieSkladu.Turn( false );

        //----------

        // McZapkie-141102: SHP i czuwak, TODO: sygnalizacja kabinowa
        if (mvOccupied->SecuritySystem.Status > 0)
        {
            if (fBlinkTimer >  fCzuwakBlink)
                fBlinkTimer = -fCzuwakBlink;
            else
                fBlinkTimer += Deltatime;

            // hunter-091012: dodanie testu czuwaka
            if( ( TestFlag( mvOccupied->SecuritySystem.Status, s_aware ) )
             || ( TestFlag( mvOccupied->SecuritySystem.Status, s_CAtest ) ) ) {
                btLampkaCzuwaka.Turn( fBlinkTimer > 0 );
            }
            else
                btLampkaCzuwaka.Turn( false );

            btLampkaSHP.Turn(TestFlag(mvOccupied->SecuritySystem.Status, s_active));
        }
        else // wylaczone
        {
            btLampkaCzuwaka.Turn( false );
            btLampkaSHP.Turn( true );
        }
        // przelaczniki
        // ABu030405 obsluga lampki uniwersalnej:
        if( btInstrumentLight.Active() ) // w ogóle jest
            if( InstrumentLightActive ) // załączona
                switch( InstrumentLightType ) {
                    case 0:
                        btInstrumentLight.Turn( mvControlled->Battery || mvControlled->ConverterFlag );
                        break;
                    case 1:
                        btInstrumentLight.Turn( mvControlled->Mains );
                        break;
                    case 2:
                        btInstrumentLight.Turn( mvControlled->ConverterFlag );
                        break;
                    default:
                        btInstrumentLight.Turn( false );
                }
            else
                btInstrumentLight.Turn( false );

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

        ggLightsButton.Update();
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
        ggDimHeadlightsButton.Update();
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
        for( auto &universal : ggUniversals ) {
            universal.Update();
        }
        // hunter-091012
        ggInstrumentLightButton.Update();
        ggCabLightButton.Update();
        ggCabLightDimButton.Update();
        ggBatteryButton.Update();
        //------
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

    // sounds
    update_sounds( Deltatime );

    if( false == m_radiomessages.empty() ) {
        // erase completed radio messages from the list
        m_radiomessages.erase(
            std::remove_if(
                std::begin( m_radiomessages ), std::end( m_radiomessages ),
                []( sound_source &source ) {
                    return ( false == source.is_playing() ); } ),
            std::end( m_radiomessages ) );
    }

    return true; //(DynamicObject->Update(dt));
} // koniec update

void
TTrain::update_sounds( double const Deltatime ) {

    if( Deltatime == 0.0 ) { return; }

    double volume { 0.0 };
    double const brakevolumescale { 0.5 };

    // Winger-160404 - syczenie pomocniczego (luzowanie)
    if( m_lastlocalbrakepressure != -1.f ) {
        // calculate rate of pressure drop in local brake cylinder, once it's been initialized
        auto const brakepressuredifference { mvOccupied->LocBrakePress - m_lastlocalbrakepressure };
        m_localbrakepressurechange = interpolate<float>( m_localbrakepressurechange, 10 * ( brakepressuredifference / Deltatime ), 0.1f );
    }
    m_lastlocalbrakepressure = mvOccupied->LocBrakePress;
    // local brake, release
    if( ( m_localbrakepressurechange < -0.05f )
     && ( mvOccupied->LocBrakePress > mvOccupied->BrakePress - 0.05 ) ) {
        rsSBHiss
            .gain( clamp( rsSBHiss.m_amplitudeoffset + rsSBHiss.m_amplitudefactor * -m_localbrakepressurechange * 0.05, 0.0, 1.5 ) )
            .play( sound_flags::exclusive | sound_flags::looping );
    }
    else {
        // don't stop the sound too abruptly
        volume = std::max( 0.0, rsSBHiss.gain() - 0.1 * Deltatime );
        rsSBHiss.gain( volume );
        if( volume < 0.05 ) {
            rsSBHiss.stop();
        }
    }
    // local brake, engage
    if( m_localbrakepressurechange > 0.05f ) {
        rsSBHissU
            .gain( clamp( rsSBHissU.m_amplitudeoffset + rsSBHissU.m_amplitudefactor * m_localbrakepressurechange * 0.05, 0.0, 1.5 ) )
            .play( sound_flags::exclusive | sound_flags::looping );
    }
    else {
        // don't stop the sound too abruptly
        volume = std::max( 0.0, rsSBHissU.gain() - 0.1 * Deltatime );
        rsSBHissU.gain( volume );
        if( volume < 0.05 ) {
            rsSBHissU.stop();
        }
    }

    // McZapkie-280302 - syczenie
    // TODO: softer volume reduction than plain abrupt stop, perhaps as reusable wrapper?
    if( ( mvOccupied->BrakeHandle == FV4a )
     || ( mvOccupied->BrakeHandle == FVel6 ) ) {
        // upuszczanie z PG
        fPPress = interpolate( fPPress, static_cast<float>( mvOccupied->Handle->GetSound( s_fv4a_b ) ), 0.05f );
        volume = (
            fPPress > 0 ?
                rsHiss.m_amplitudefactor * fPPress * 0.25 :
                0 );
        if( volume * brakevolumescale > 0.05 ) {
            rsHiss
                .gain( volume * brakevolumescale )
                .play( sound_flags::exclusive | sound_flags::looping );
        }
        else {
            rsHiss.stop();
        }
        // napelnianie PG
        fNPress = interpolate( fNPress, static_cast<float>( mvOccupied->Handle->GetSound( s_fv4a_u ) ), 0.25f );
        volume = (
            fNPress > 0 ?
                rsHissU.m_amplitudefactor * fNPress :
                0 );
        if( volume * brakevolumescale > 0.05 ) {
            rsHissU
                .gain( volume * brakevolumescale )
                .play( sound_flags::exclusive | sound_flags::looping );
        }
        else {
            rsHissU.stop();
        }
        // upuszczanie przy naglym
        volume = mvOccupied->Handle->GetSound( s_fv4a_e ) * rsHissE.m_amplitudefactor;
        if( volume * brakevolumescale > 0.05 ) {
            rsHissE
                .gain( volume * brakevolumescale )
                .play( sound_flags::exclusive | sound_flags::looping );
        }
        else {
            rsHissE.stop();
        }
        // upuszczanie sterujacego fala
        volume = mvOccupied->Handle->GetSound( s_fv4a_x ) * rsHissX.m_amplitudefactor;
        if( volume * brakevolumescale > 0.05 ) {
            rsHissX
                .gain( volume * brakevolumescale )
                .play( sound_flags::exclusive | sound_flags::looping );
        }
        else {
            rsHissX.stop();
        }
        // upuszczanie z czasowego 
        volume = mvOccupied->Handle->GetSound( s_fv4a_t ) * rsHissT.m_amplitudefactor;
        if( volume * brakevolumescale > 0.05 ) {
            rsHissT
                .gain( volume * brakevolumescale )
                .play( sound_flags::exclusive | sound_flags::looping );
        }
        else {
            rsHissT.stop();
        }

    } else {
        // jesli nie FV4a
        // upuszczanie z PG
        fPPress = ( 4.0f * fPPress + std::max( mvOccupied->dpLocalValve, mvOccupied->dpMainValve ) ) / ( 4.0f + 1.0f );
        volume = (
            fPPress > 0.0f ?
                2.0 * rsHiss.m_amplitudefactor * fPPress :
                0.0 );
        if( volume > 0.05 ) {
            rsHiss
                .gain( volume )
                .play( sound_flags::exclusive | sound_flags::looping );
        }
        else {
            rsHiss.stop();
        }
        // napelnianie PG
        fNPress = ( 4.0f * fNPress + Min0R( mvOccupied->dpLocalValve, mvOccupied->dpMainValve ) ) / ( 4.0f + 1.0f );
        volume = (
            fNPress < 0.0f ?
                -1.0 * rsHissU.m_amplitudefactor * fNPress :
                 0.0 );
        if( volume > 0.01 ) {
            rsHissU
                .gain( volume )
                .play( sound_flags::exclusive | sound_flags::looping );
        }
        else {
            rsHissU.stop();
        }
    } // koniec nie FV4a

    // ambient sound
    // since it's typically ticking of the clock we can center it on tachometer or on middle of compartment bounding area
    rsFadeSound.play( sound_flags::exclusive | sound_flags::looping );

    if( mvControlled->TrainType == dt_181 ) {
        // alarm przy poslizgu dla 181/182 - BOMBARDIER
        if( ( mvControlled->SlippingWheels )
         && ( DynamicObject->GetVelocity() > 1.0 ) ) {
            dsbSlipAlarm.play( sound_flags::exclusive | sound_flags::looping );
        }
        else {
            dsbSlipAlarm.stop();
        }
    }

    // szum w czasie jazdy
    if( ( false == FreeFlyModeFlag )
     && ( false == Global::CabWindowOpen )
     && ( DynamicObject->GetVelocity() > 0.5 ) ) {

        volume = rsRunningNoise.m_amplitudeoffset + rsRunningNoise.m_amplitudefactor * mvOccupied->Vel;
        auto frequency { rsRunningNoise.m_frequencyoffset + rsRunningNoise.m_frequencyfactor * mvOccupied->Vel };

        if( std::abs( mvOccupied->nrot ) > 0.01 ) {
            // hamulce wzmagaja halas
            volume *= 1 + 0.25 * ( mvOccupied->UnitBrakeForce / ( 1 + mvOccupied->MaxBrakeForce ) );
        }
        // scale volume by track quality
        volume *= ( 20.0 + DynamicObject->MyTrack->iDamageFlag ) / 21;
        // scale volume with vehicle speed
        // TBD, TODO: disable the scaling for sounds combined from speed-based samples?
        volume *=
            interpolate(
                0.0, 1.0,
                clamp(
                    mvOccupied->Vel / 40.0,
                    0.0, 1.0 ) );
        rsRunningNoise
            .pitch( frequency )
            .gain( volume )
            .play( sound_flags::exclusive | sound_flags::looping );
    }
    else {
        // don't play the optional ending sound if the listener switches views
        rsRunningNoise.stop( true == FreeFlyModeFlag );
    }

    // McZapkie-141102: SHP i czuwak, TODO: sygnalizacja kabinowa
    if (mvOccupied->SecuritySystem.Status > 0) {
        // hunter-091012: rozdzielenie alarmow
        if( TestFlag( mvOccupied->SecuritySystem.Status, s_CAalarm )
         || TestFlag( mvOccupied->SecuritySystem.Status, s_SHPalarm ) ) {

            if( false == dsbBuzzer.is_playing() ) {
                dsbBuzzer.play( sound_flags::looping );
                Console::BitsSet( 1 << 14 ); // ustawienie bitu 16 na PoKeys
            }
        }
        else {
            if( true == dsbBuzzer.is_playing() ) {
                dsbBuzzer.stop();
                Console::BitsClear( 1 << 14 ); // ustawienie bitu 16 na PoKeys
            }
        }
    }
    else {
        // wylaczone
        if( true == dsbBuzzer.is_playing() ) {
            dsbBuzzer.stop();
            Console::BitsClear( 1 << 14 ); // ustawienie bitu 16 na PoKeys
        }
    }

    if( fTachoCount > 3.f ) {
        dsbHasler.play( sound_flags::exclusive | sound_flags::looping );
    }
    else if( fTachoCount < 1.f ) {
        dsbHasler.stop();
    }
}

bool TTrain::CabChange(int iDirection)
{ // McZapkie-090902: zmiana kabiny 1->0->2 i z powrotem
    if( ( DynamicObject->Mechanik == nullptr )
     || ( true == DynamicObject->Mechanik->AIControllFlag ) ) {
        // jeśli prowadzi AI albo jest w innym członie
        // jak AI prowadzi, to nie można mu mieszać
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
                dsbNastawnikJazdy.deserialize( parser, sound_type::single );
                dsbNastawnikJazdy.owner( DynamicObject );
            }
            else if (token == "ctrlscnd:")
            {
                // hunter-081211: nastawnik bocznikowania
                dsbNastawnikBocz.deserialize( parser, sound_type::single );
                dsbNastawnikBocz.owner( DynamicObject );
            }
            else if (token == "reverserkey:")
            {
                // hunter-131211: dzwiek kierunkowego
                dsbReverserKey.deserialize( parser, sound_type::single );
                dsbReverserKey.owner( DynamicObject );
            }
            else if (token == "buzzer:")
            {
                // bzyczek shp:
                dsbBuzzer.deserialize( parser, sound_type::single );
                dsbBuzzer.owner( DynamicObject );
            }
            else if (token == "slipalarm:")
            {
                // Bombardier 011010: alarm przy poslizgu:
                dsbSlipAlarm.deserialize( parser, sound_type::single );
                dsbSlipAlarm.owner( DynamicObject );
            }
            else if (token == "tachoclock:")
            {
                // cykanie rejestratora:
                dsbHasler.deserialize( parser, sound_type::single );
                dsbHasler.owner( DynamicObject );
            }
            else if (token == "switch:")
            {
                // przelaczniki:
                dsbSwitch.deserialize( parser, sound_type::single );
                dsbSwitch.owner( DynamicObject );
            }
            else if (token == "pneumaticswitch:")
            {
                // stycznik EP:
                dsbPneumaticSwitch.deserialize( parser, sound_type::single );
                dsbPneumaticSwitch.owner( DynamicObject );
            }
            else if (token == "airsound:")
            {
                // syk:
                rsHiss.deserialize( parser, sound_type::single, sound_parameters::amplitude );
                rsHiss.owner( DynamicObject );
                if( true == rsSBHiss.empty() ) {
                    // fallback for vehicles without defined local brake hiss sound
                    rsSBHiss = rsHiss;
                }
            }
            else if (token == "airsound2:")
            {
                // syk:
                rsHissU.deserialize( parser, sound_type::single, sound_parameters::amplitude );
                rsHissU.owner( DynamicObject );
                if( true == rsSBHissU.empty() ) {
                    // fallback for vehicles without defined local brake hiss sound
                    rsSBHissU = rsHissU;
                }
            }
            else if (token == "airsound3:")
            {
                // syk:
                rsHissE.deserialize( parser, sound_type::single, sound_parameters::amplitude );
                rsHissE.owner( DynamicObject );
            }
            else if (token == "airsound4:")
            {
                // syk:
                rsHissX.deserialize( parser, sound_type::single, sound_parameters::amplitude );
                rsHissX.owner( DynamicObject );
            }
            else if (token == "airsound5:")
            {
                // syk:
                rsHissT.deserialize( parser, sound_type::single, sound_parameters::amplitude );
                rsHissT.owner( DynamicObject );
            }
            else if (token == "localbrakesound:")
            {
                // syk:
                rsSBHiss.deserialize( parser, sound_type::single, sound_parameters::amplitude );
                rsSBHiss.owner( DynamicObject );
            }
            else if( token == "localbrakesound2:" ) {
                // syk:
                rsSBHissU.deserialize( parser, sound_type::single, sound_parameters::amplitude );
                rsSBHissU.owner( DynamicObject );
            }
            else if (token == "fadesound:")
            {
                // ambient sound:
                rsFadeSound.deserialize( parser, sound_type::single );
                rsFadeSound.owner( DynamicObject );
            }
            else if( token == "runningnoise:" ) {
                // szum podczas jazdy:
                rsRunningNoise.deserialize( parser, sound_type::single, sound_parameters::amplitude | sound_parameters::frequency );
                rsRunningNoise.owner( DynamicObject );

                rsRunningNoise.m_amplitudefactor /= ( 1 + mvOccupied->Vmax );
                rsRunningNoise.m_frequencyfactor /= ( 1 + mvOccupied->Vmax );
            }
            else if (token == "mechspring:")
            {
                // parametry bujania kamery:
                double ks, kd;
                parser.getTokens(2, false);
                parser
                    >> ks
                    >> kd;
                MechSpring.Init(MechSpring.restLen, ks, kd);
                parser.getTokens(6, false);
                parser
                    >> fMechSpringX
                    >> fMechSpringY
                    >> fMechSpringZ
                    >> fMechMaxSpring
                    >> fMechRoll
                    >> fMechPitch;
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
    // reset sound positions and owner
    auto const nullvector { glm::vec3() };
    std::vector<sound_source *> sounds = {
        &dsbReverserKey, &dsbNastawnikJazdy, &dsbNastawnikBocz,
        &dsbSwitch, &dsbPneumaticSwitch,
        &rsHiss, &rsHissU, &rsHissE, &rsHissX, &rsHissT, &rsSBHiss, &rsSBHissU,
        &rsFadeSound, &rsRunningNoise,
        &dsbHasler, &dsbBuzzer, &dsbSlipAlarm, &m_radiosound
    };
    for( auto sound : sounds ) {
        sound->offset( nullvector );
        sound->owner( DynamicObject );
    }

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
                    // bieżąca sciezka do tekstur to dynamic/...
                    Global::asCurrentTexturePath = DynamicObject->asBaseDir;
                    // szukaj kabinę jako oddzielny model
                    TModel3d *kabina = TModelsManager::GetModel(DynamicObject->asBaseDir + token, true);
                    // z powrotem defaultowa sciezka do tekstur
                    Global::asCurrentTexturePath = szTexturePath;
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
                else if (cabindex == 1) {
                    // model tylko, gdy nie ma kabiny 1
                    // McZapkie-170103: szukaj elementy kabiny w glownym modelu
                    DynamicObject->mdKabina = DynamicObject->mdModel;
                }
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
                pyScreens.init(*parser, DynamicObject->mdKabina, DynamicObject->name(), NewCabNo);
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
        // configure placement of sound emitters which aren't bound with any device model, and weren't placed manually
        // try first to bind sounds to location of possible devices
        if( dsbReverserKey.offset() == nullvector ) {
            dsbReverserKey.offset( ggDirKey.model_offset() );
        }
        if( dsbNastawnikJazdy.offset() == nullvector ) {
            dsbNastawnikJazdy.offset( ggMainCtrl.model_offset() );
        }
        if( dsbNastawnikBocz.offset() == nullvector ) {
            dsbNastawnikBocz.offset( ggScndCtrl.model_offset() );
        }
        if( dsbBuzzer.offset() == nullvector ) {
            dsbBuzzer.offset( btLampkaCzuwaka.model_offset() );
        }
        // radio has two potential items which can provide the position
        if( m_radiosound.offset() == nullvector ) {
            m_radiosound.offset( ggRadioButton.model_offset() );
        }
        if( m_radiosound.offset() == nullvector ) {
            m_radiosound.offset( btLampkaRadio.model_offset() );
        }
        auto const localbrakeoffset { ggLocalBrake.model_offset() };
        std::vector<sound_source *> localbrakesounds = {
            &rsSBHiss, &rsSBHissU
        };
        for( auto sound : localbrakesounds ) {
            if( sound->offset() == nullvector ) {
                sound->offset( localbrakeoffset );
            }
        }
        // NOTE: if the local brake model can't be located the emitter will also be assigned location of main brake
        auto const brakeoffset { ggBrakeCtrl.model_offset() };
        std::vector<sound_source *> brakesounds = {
            &rsHiss, &rsHissU, &rsHissE, &rsHissX, &rsHissT, &rsSBHiss, &rsSBHissU,
        };
        for( auto sound : brakesounds ) {
            if( sound->offset() == nullvector ) {
                sound->offset( brakeoffset );
            }
        }
        // for whatever is left fallback on generic location, centre of the cab 
        auto const caboffset { glm::dvec3 { ( Cabine[ cabindex ].CabPos1 + Cabine[ cabindex ].CabPos2 ) * 0.5 } + glm::dvec3 { 0, 1, 0 } };
        for( auto sound : sounds ) {
            if( sound->offset() == nullvector ) {
                sound->offset( caboffset );
            }
        }

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

void
TTrain::radio_message( sound_source *Message, int const Channel ) {

    if( ( false == mvOccupied->Radio )
     || ( iRadioChannel != Channel ) ) {
        // skip message playback if the radio isn't able to receive it
        return;
    }
    auto const radiorange { 7500 * 7500 };
    if( glm::length2( Message->location() - glm::dvec3 { DynamicObject->GetPosition() } ) > radiorange ) {
        return;
    }

    m_radiomessages.emplace_back( m_radiosound );
    // assign sound to the template and play it
    m_radiomessages.back()
        .copy_sounds( *Message )
        .play( sound_flags::exclusive );
}

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
    btLampkaNadmPrzetw.Clear(((mvControlled->TrainType & dt_EZT) != 0) ? -1 : 7); // EN57 nie ma tej lampki
    btLampkaPrzetw.Clear();
    btLampkaPrzetwOff.Clear(((mvControlled->TrainType & dt_EZT) != 0) ? 7 : -1 ); // za to ma tę
    btLampkaPrzetwB.Clear();
    btLampkaPrzetwBOff.Clear();
    btLampkaPrzekRozn.Clear();
    btLampkaPrzekRoznPom.Clear();
    btLampkaNadmSil.Clear(4);
    btLampkaUkrotnienie.Clear();
    btLampkaHamPosp.Clear();
    btLampkaWylSzybki.Clear(3);
    btLampkaWylSzybkiOff.Clear();
    btLampkaWylSzybkiB.Clear();
    btLampkaWylSzybkiBOff.Clear();
    btLampkaNadmWent.Clear(9);
    btLampkaNadmSpr.Clear(8);
    btLampkaOpory.Clear(2);
    btLampkaWysRozr.Clear(((mvControlled->TrainType & dt_ET22) != 0) ? -1 : 10); // ET22 nie ma tej lampki
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
    btLampkaBrakingOff.Clear();
    btLampkaSprezarka.Clear();
    btLampkaSprezarkaB.Clear();
    btLampkaSprezarkaOff.Clear();
    btLampkaSprezarkaBOff.Clear();
    btLampkaNapNastHam.Clear();
    btLampkaStycznB.Clear();
    btLampkaHamowanie1zes.Clear();
    btLampkaHamowanie2zes.Clear();
    btLampkaNadmPrzetwB.Clear();
    btLampkaForward.Clear();
    btLampkaBackward.Clear();
    // light indicators
    btLampkaUpperLight.Clear();
    btLampkaLeftLight.Clear();
    btLampkaRightLight.Clear();
    btLampkaLeftEndLight.Clear();
    btLampkaRightEndLight.Clear();
    btLampkaRearUpperLight.Clear();
    btLampkaRearLeftLight.Clear();
    btLampkaRearRightLight.Clear();
    btLampkaRearLeftEndLight.Clear();
    btLampkaRearRightEndLight.Clear();
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
    ggLightsButton.PutValue( mvOccupied->LightsPos - 1 );

    int const vehicleside =
        ( mvOccupied->ActiveCab == 1 ?
            side::front :
            side::rear );

    if( ( DynamicObject->iLights[ vehicleside ] & light::headlight_left ) != 0 ) {
        ggLeftLightButton.PutValue( 1.0 );
    }
    if( ( DynamicObject->iLights[ vehicleside ] & light::headlight_right ) != 0 ) {
        ggRightLightButton.PutValue( 1.0 );
    }
    if( ( DynamicObject->iLights[ vehicleside ] & light::headlight_upper ) != 0 ) {
        ggUpperLightButton.PutValue( 1.0 );
    }
    if( ( DynamicObject->iLights[ vehicleside ] & light::redmarker_left ) != 0 ) {
        if( ggLeftEndLightButton.SubModel != nullptr ) {
            ggLeftEndLightButton.PutValue( 1.0 );
        }
        else {
            ggLeftLightButton.PutValue( -1.0 );
        }
    }
    if( ( DynamicObject->iLights[ vehicleside ] & light::redmarker_right ) != 0 ) {
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

    ggInstrumentLightButton.PutValue( (
        InstrumentLightActive ?
            1.0 :
            0.0 ) );
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

// initializes a button matching provided label. returns: true if the label was found, false otherwise
// TODO: refactor the cabin controls into some sensible structure
bool TTrain::initialize_button(cParser &Parser, std::string const &Label, int const Cabindex) {

    std::unordered_map<std::string, TButton &> const lights = {
        { "i-maxft:", btLampkaMaxSila },
        { "i-maxftt:", btLampkaPrzekrMaxSila },
        { "i-radio:", btLampkaRadio },
        { "i-manual_brake:", btLampkaHamulecReczny },
        { "i-door_blocked:", btLampkaBlokadaDrzwi },
        { "i-slippery:", btLampkaPoslizg },
        { "i-contactors:", btLampkaStyczn },
        { "i-conv_ovld:", btLampkaNadmPrzetw },
        { "i-converter:", btLampkaPrzetw },
        { "i-converteroff:", btLampkaPrzetwOff },
        { "i-converterb:", btLampkaPrzetwB },
        { "i-converterboff:", btLampkaPrzetwBOff },
        { "i-diff_relay:", btLampkaPrzekRozn },
        { "i-diff_relay2:", btLampkaPrzekRoznPom },
        { "i-motor_ovld:", btLampkaNadmSil },
        { "i-train_controll:", btLampkaUkrotnienie },
        { "i-brake_delay_r:", btLampkaHamPosp },
        { "i-mainbreaker:", btLampkaWylSzybki },
        { "i-mainbreakerb:", btLampkaWylSzybkiB },
        { "i-mainbreakeroff:", btLampkaWylSzybkiOff },
        { "i-mainbreakerboff:", btLampkaWylSzybkiBOff },
        { "i-vent_ovld:", btLampkaNadmWent },
        { "i-comp_ovld:", btLampkaNadmSpr },
        { "i-resistors:", btLampkaOpory },
        { "i-no_resistors:", btLampkaBezoporowa },
        { "i-no_resistors_b:", btLampkaBezoporowaB },
        { "i-highcurrent:", btLampkaWysRozr },
        { "i-vent_trim:", btLampkaWentZaluzje },
        { "i-trainheating:", btLampkaOgrzewanieSkladu },
        { "i-security_aware:", btLampkaCzuwaka },
        { "i-security_cabsignal:", btLampkaSHP },
        { "i-door_left:", btLampkaDoorLeft },
        { "i-door_right:", btLampkaDoorRight },
        { "i-departure_signal:", btLampkaDepartureSignal },
        { "i-reserve:", btLampkaRezerwa },
        { "i-scnd:", btLampkaBoczniki },
        { "i-scnd1:", btLampkaBocznik1 },
        { "i-scnd2:", btLampkaBocznik2 },
        { "i-scnd3:", btLampkaBocznik3 },
        { "i-scnd4:", btLampkaBocznik4 },
        { "i-braking:", btLampkaHamienie },
        { "i-brakingoff:", btLampkaBrakingOff },
        { "i-dynamicbrake:", btLampkaED },
        { "i-braking-ezt:", btLampkaHamowanie1zes },
        { "i-braking-ezt2:", btLampkaHamowanie2zes },
        { "i-compressor:", btLampkaSprezarka },
        { "i-compressorb:", btLampkaSprezarkaB },
        { "i-compressoroff:", btLampkaSprezarkaOff },
        { "i-compressorboff:", btLampkaSprezarkaBOff },
        { "i-voltbrake:", btLampkaNapNastHam },
        { "i-resistorsb:", btLampkaOporyB },
        { "i-contactorsb:", btLampkaStycznB },
        { "i-conv_ovldb:", btLampkaNadmPrzetwB },
        { "i-forward:", btLampkaForward },
        { "i-backward:", btLampkaBackward },
        { "i-upperlight:", btLampkaUpperLight },
        { "i-leftlight:", btLampkaLeftLight },
        { "i-rightlight:", btLampkaRightLight },
        { "i-leftend:", btLampkaLeftEndLight },
        { "i-rightend:", btLampkaRightEndLight },
        { "i-rearupperlight:", btLampkaRearUpperLight },
        { "i-rearleftlight:", btLampkaRearLeftLight },
        { "i-rearrightlight:", btLampkaRearRightLight },
        { "i-rearleftend:", btLampkaRearLeftEndLight },
        { "i-rearrightend:",  btLampkaRearRightEndLight },
        { "i-cablight:", btCabLight }
    };
    auto lookup = lights.find( Label );
    if( lookup != lights.end() ) {
        lookup->second.Load( Parser, DynamicObject, DynamicObject->mdKabina );
    }

    else if( Label == "i-instrumentlight:" ) {
        btInstrumentLight.Load( Parser, DynamicObject, DynamicObject->mdKabina, DynamicObject->mdModel );
        InstrumentLightType = 0;
    }
    else if( Label == "i-instrumentlight_M:" ) {
        btInstrumentLight.Load( Parser, DynamicObject, DynamicObject->mdKabina, DynamicObject->mdModel );
        InstrumentLightType = 1;
    }
    else if( Label == "i-instrumentlight_C:" ) {
        btInstrumentLight.Load( Parser, DynamicObject, DynamicObject->mdKabina, DynamicObject->mdModel );
        InstrumentLightType = 2;
    }
    else if (Label == "i-doors:")
    {
        int i = Parser.getToken<int>() - 1;
        auto &button = Cabine[Cabindex].Button(-1); // pierwsza wolna lampka
        button.Load(Parser, DynamicObject, DynamicObject->mdKabina);
        button.AssignBool(bDoors[0] + 3 * i);
    }
    else
    {
        // failed to match the label
        return false;
    }

    return true;
}

// initializes a gauge matching provided label. returns: true if the label was found, false otherwise
// TODO: refactor the cabin controls into some sensible structure
bool TTrain::initialize_gauge(cParser &Parser, std::string const &Label, int const Cabindex) {

    std::unordered_map<std::string, TGauge &> const gauges = {
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
        lookup->second.Load( Parser, DynamicObject, DynamicObject->mdKabina );
        m_controlmapper.insert( lookup->second, lookup->first );
    }
    // ABu 090305: uniwersalne przyciski lub inne rzeczy
    else if( Label == "mainctrlact:" ) {
        ggMainCtrlAct.Load( Parser, DynamicObject, DynamicObject->mdKabina, DynamicObject->mdModel );
    }
    // SEKCJA WSKAZNIKOW
    else if ((Label == "tachometer:") || (Label == "tachometerb:"))
    {
        // predkosciomierz wskaźnikowy z szarpaniem
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
        gauge.AssignFloat(&fTachoVelocityJump);
        // bind tachometer sound location to the meter
        if( dsbHasler.offset() == glm::vec3() ) {
            dsbHasler.offset( gauge.model_offset() );
        }
    }
    else if (Label == "tachometern:")
    {
        // predkosciomierz wskaźnikowy bez szarpania
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
        gauge.AssignFloat(&fTachoVelocity);
        // bind tachometer sound location to the meter
        if( dsbHasler.offset() == glm::vec3() ) {
            dsbHasler.offset( gauge.model_offset() );
        }
    }
    else if (Label == "tachometerd:")
    {
        // predkosciomierz cyfrowy
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
        gauge.AssignFloat(&fTachoVelocity);
        // bind tachometer sound location to the meter
        if( dsbHasler.offset() == glm::vec3() ) {
            dsbHasler.offset( gauge.model_offset() );
        }
    }
    else if ((Label == "hvcurrent1:") || (Label == "hvcurrent1b:"))
    {
        // 1szy amperomierz
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
        gauge.AssignFloat(fHCurrent + 1);
    }
    else if ((Label == "hvcurrent2:") || (Label == "hvcurrent2b:"))
    {
        // 2gi amperomierz
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
        gauge.AssignFloat(fHCurrent + 2);
    }
    else if ((Label == "hvcurrent3:") || (Label == "hvcurrent3b:"))
    {
        // 3ci amperomierz
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałska
        gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
        gauge.AssignFloat(fHCurrent + 3);
    }
    else if ((Label == "hvcurrent:") || (Label == "hvcurrentb:"))
    {
        // amperomierz calkowitego pradu
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
        gauge.AssignFloat(fHCurrent);
    }
    else if (Label == "eimscreen:")
    {
        // amperomierz calkowitego pradu
        int i, j;
        Parser.getTokens(2, false);
        Parser >> i >> j;
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
        gauge.AssignFloat(&fEIMParams[i][j]);
    }
    else if (Label == "brakes:")
    {
        // amperomierz calkowitego pradu
        int i, j;
        Parser.getTokens(2, false);
        Parser >> i >> j;
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
        gauge.AssignFloat(&fPress[i - 1][j]);
    }
    else if ((Label == "brakepress:") || (Label == "brakepressb:"))
    {
        // manometr cylindrow hamulcowych
        // Ra 2014-08: przeniesione do TCab
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina, nullptr, 0.1);
        gauge.AssignDouble(&mvOccupied->BrakePress);
    }
    else if ((Label == "pipepress:") || (Label == "pipepressb:"))
    {
        // manometr przewodu hamulcowego
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina, nullptr, 0.1);
        gauge.AssignDouble(&mvOccupied->PipePress);
    }
    else if (Label == "limpipepress:")
    {
        // manometr zbiornika sterujacego zaworu maszynisty
        ggZbS.Load(Parser, DynamicObject, DynamicObject->mdKabina, nullptr, 0.1);
    }
    else if (Label == "cntrlpress:")
    {
        // manometr zbiornika kontrolnego/rorzďż˝du
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina, nullptr, 0.1);
        gauge.AssignDouble(&mvControlled->PantPress);
    }
    else if ((Label == "compressor:") || (Label == "compressorb:"))
    {
        // manometr sprezarki/zbiornika glownego
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina, nullptr, 0.1);
        gauge.AssignDouble(&mvOccupied->Compressor);
    }
    // yB - dla drugiej sekcji
    else if (Label == "hvbcurrent1:")
    {
        // 1szy amperomierz
        ggI1B.Load(Parser, DynamicObject, DynamicObject->mdKabina);
    }
    else if (Label == "hvbcurrent2:")
    {
        // 2gi amperomierz
        ggI2B.Load(Parser, DynamicObject, DynamicObject->mdKabina);
    }
    else if (Label == "hvbcurrent3:")
    {
        // 3ci amperomierz
        ggI3B.Load(Parser, DynamicObject, DynamicObject->mdKabina);
    }
    else if (Label == "hvbcurrent:")
    {
        // amperomierz calkowitego pradu
        ggItotalB.Load(Parser, DynamicObject, DynamicObject->mdKabina);
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
        ggEngineVoltage.Load(Parser, DynamicObject, DynamicObject->mdKabina);
    }
    else if (Label == "hvoltage:")
    {
        // woltomierz wysokiego napiecia
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
        gauge.AssignFloat(&fHVoltage);
    }
    else if (Label == "lvoltage:")
    {
        // woltomierz niskiego napiecia
        ggLVoltage.Load(Parser, DynamicObject, DynamicObject->mdKabina);
    }
    else if (Label == "enrot1m:")
    {
        // obrotomierz
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
        gauge.AssignFloat(fEngine + 1);
    } // ggEnrot1m.Load(Parser,DynamicObject->mdKabina);
    else if (Label == "enrot2m:")
    {
        // obrotomierz
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
        gauge.AssignFloat(fEngine + 2);
    } // ggEnrot2m.Load(Parser,DynamicObject->mdKabina);
    else if (Label == "enrot3m:")
    { // obrotomierz
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
        gauge.AssignFloat(fEngine + 3);
    } // ggEnrot3m.Load(Parser,DynamicObject->mdKabina);
    else if (Label == "engageratio:")
    {
        // np. ciśnienie sterownika sprzęgła
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
        gauge.AssignDouble(&mvControlled->dizel_engage);
    } // ggEngageRatio.Load(Parser,DynamicObject->mdKabina);
    else if (Label == "maingearstatus:")
    {
        // np. ciśnienie sterownika skrzyni biegów
        ggMainGearStatus.Load(Parser, DynamicObject, DynamicObject->mdKabina);
    }
    else if (Label == "ignitionkey:")
    {
        ggIgnitionKey.Load(Parser, DynamicObject, DynamicObject->mdKabina);
    }
    else if (Label == "distcounter:")
    {
        // Ra 2014-07: licznik kilometrów
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
        gauge.AssignDouble(&mvControlled->DistCounter);
    }
    else
    {
        // failed to match the label
        return false;
    }

    return true;
}
