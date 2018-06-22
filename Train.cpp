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
#include "World.h"
#include "Camera.h"
#include "Logs.h"
#include "MdlMngr.h"
#include "Timer.h"
#include "Driver.h"
#include "mtable.h"
#include "Console.h"
#include "sound.h"

void control_mapper::insert(TGauge const& Gauge, std::string const& Label)
{
	auto const submodel = Gauge.SubModel;
	if (submodel != nullptr) {
		m_controlnames.emplace(submodel, Label);
	}
}

std::string control_mapper::find(TSubModel const* Control) const
{
	auto const lookup = m_controlnames.find(Control);
	if (lookup != m_controlnames.end()) {
		return lookup->second;
	} else {
		return "";
	}
}

void TCab::Load(cParser& Parser)
{
	// NOTE: clearing control tables here is bit of a crutch, imposed by current scheme of loading compartments anew on each cab change
	ggList.clear();
	btList.clear();

	std::string token;
	Parser.getTokens();
	Parser >> token;
	if (token == "cablight") {
		Parser.getTokens(9, false);
		Parser >> dimm.r >> dimm.g >> dimm.b >> intlit.r >> intlit.g >> intlit.b >> intlitlow.r >> intlitlow.g >> intlitlow.b;
		Parser.getTokens();
		Parser >> token;
	}
	CabPos1.x = std::stod(token);
	Parser.getTokens(5, false);
	Parser >> CabPos1.y >> CabPos1.z >> CabPos2.x >> CabPos2.y >> CabPos2.z;

	bEnabled = true;
	bOccupied = true;
}

TGauge& TCab::Gauge(int n)
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
	if (n < 0) {
		ggList.emplace_back();
		return ggList.back();
	} else {
		return ggList[n];
	}
};

TButton& TCab::Button(int n)
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
	if (n < 0) {
		btList.emplace_back();
		return btList.back();
	} else {
		return btList[n];
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
	for (auto& gauge : ggList) {
		// animacje izometryczne
		gauge.UpdateValue(); // odczyt parametru i przeliczenie na kąt
		gauge.Update(); // ustawienie animacji
	}
	for (auto& button : btList) {
		// animacje dwustanowe
		button.update(); // odczyt parametru i wybór submodelu
	}
};

// NOTE: we're currently using universal handlers and static handler map but it may be beneficial to have these implemented on individual class instance basis
// TBD, TODO: consider this approach if we ever want to have customized consist behaviour to received commands, based on the consist/vehicle type or whatever
TTrain::commandhandler_map const TTrain::m_commandhandlers = {

	{ user_command::aidriverenable, &TTrain::OnCommand_aidriverenable }, { user_command::aidriverdisable, &TTrain::OnCommand_aidriverdisable },
	{ user_command::mastercontrollerincrease, &TTrain::OnCommand_mastercontrollerincrease }, { user_command::mastercontrollerincreasefast, &TTrain::OnCommand_mastercontrollerincreasefast },
	{ user_command::mastercontrollerdecrease, &TTrain::OnCommand_mastercontrollerdecrease }, { user_command::mastercontrollerdecreasefast, &TTrain::OnCommand_mastercontrollerdecreasefast },
	{ user_command::mastercontrollerset, &TTrain::OnCommand_mastercontrollerset }, { user_command::secondcontrollerincrease, &TTrain::OnCommand_secondcontrollerincrease },
	{ user_command::secondcontrollerincreasefast, &TTrain::OnCommand_secondcontrollerincreasefast }, { user_command::secondcontrollerdecrease, &TTrain::OnCommand_secondcontrollerdecrease },
	{ user_command::secondcontrollerdecreasefast, &TTrain::OnCommand_secondcontrollerdecreasefast }, { user_command::secondcontrollerset, &TTrain::OnCommand_secondcontrollerset },
	{ user_command::notchingrelaytoggle, &TTrain::OnCommand_notchingrelaytoggle }, { user_command::mucurrentindicatorothersourceactivate, &TTrain::OnCommand_mucurrentindicatorothersourceactivate },
	{ user_command::independentbrakeincrease, &TTrain::OnCommand_independentbrakeincrease }, { user_command::independentbrakeincreasefast, &TTrain::OnCommand_independentbrakeincreasefast },
	{ user_command::independentbrakedecrease, &TTrain::OnCommand_independentbrakedecrease }, { user_command::independentbrakedecreasefast, &TTrain::OnCommand_independentbrakedecreasefast },
	{ user_command::independentbrakeset, &TTrain::OnCommand_independentbrakeset }, { user_command::independentbrakebailoff, &TTrain::OnCommand_independentbrakebailoff },
	{ user_command::trainbrakeincrease, &TTrain::OnCommand_trainbrakeincrease }, { user_command::trainbrakedecrease, &TTrain::OnCommand_trainbrakedecrease },
	{ user_command::trainbrakeset, &TTrain::OnCommand_trainbrakeset }, { user_command::trainbrakecharging, &TTrain::OnCommand_trainbrakecharging },
	{ user_command::trainbrakerelease, &TTrain::OnCommand_trainbrakerelease }, { user_command::trainbrakefirstservice, &TTrain::OnCommand_trainbrakefirstservice },
	{ user_command::trainbrakeservice, &TTrain::OnCommand_trainbrakeservice }, { user_command::trainbrakefullservice, &TTrain::OnCommand_trainbrakefullservice },
	{ user_command::trainbrakehandleoff, &TTrain::OnCommand_trainbrakehandleoff }, { user_command::trainbrakeemergency, &TTrain::OnCommand_trainbrakeemergency },
	{ user_command::trainbrakebasepressureincrease, &TTrain::OnCommand_trainbrakebasepressureincrease },
	{ user_command::trainbrakebasepressuredecrease, &TTrain::OnCommand_trainbrakebasepressuredecrease }, { user_command::trainbrakebasepressurereset, &TTrain::OnCommand_trainbrakebasepressurereset },
	{ user_command::trainbrakeoperationtoggle, &TTrain::OnCommand_trainbrakeoperationtoggle }, { user_command::manualbrakeincrease, &TTrain::OnCommand_manualbrakeincrease },
	{ user_command::manualbrakedecrease, &TTrain::OnCommand_manualbrakedecrease }, { user_command::alarmchaintoggle, &TTrain::OnCommand_alarmchaintoggle },
	{ user_command::wheelspinbrakeactivate, &TTrain::OnCommand_wheelspinbrakeactivate }, { user_command::sandboxactivate, &TTrain::OnCommand_sandboxactivate },
	{ user_command::epbrakecontroltoggle, &TTrain::OnCommand_epbrakecontroltoggle }, { user_command::trainbrakeoperationmodeincrease, &TTrain::OnCommand_trainbrakeoperationmodeincrease },
	{ user_command::trainbrakeoperationmodedecrease, &TTrain::OnCommand_trainbrakeoperationmodedecrease }, { user_command::brakeactingspeedincrease, &TTrain::OnCommand_brakeactingspeedincrease },
	{ user_command::brakeactingspeeddecrease, &TTrain::OnCommand_brakeactingspeeddecrease }, { user_command::brakeactingspeedsetcargo, &TTrain::OnCommand_brakeactingspeedsetcargo },
	{ user_command::brakeactingspeedsetpassenger, &TTrain::OnCommand_brakeactingspeedsetpassenger }, { user_command::brakeactingspeedsetrapid, &TTrain::OnCommand_brakeactingspeedsetrapid },
	{ user_command::brakeloadcompensationincrease, &TTrain::OnCommand_brakeloadcompensationincrease },
	{ user_command::brakeloadcompensationdecrease, &TTrain::OnCommand_brakeloadcompensationdecrease }, { user_command::mubrakingindicatortoggle, &TTrain::OnCommand_mubrakingindicatortoggle },
	{ user_command::reverserincrease, &TTrain::OnCommand_reverserincrease }, { user_command::reverserdecrease, &TTrain::OnCommand_reverserdecrease },
	{ user_command::reverserforwardhigh, &TTrain::OnCommand_reverserforwardhigh }, { user_command::reverserforward, &TTrain::OnCommand_reverserforward },
	{ user_command::reverserneutral, &TTrain::OnCommand_reverserneutral }, { user_command::reverserbackward, &TTrain::OnCommand_reverserbackward },
	{ user_command::alerteracknowledge, &TTrain::OnCommand_alerteracknowledge }, { user_command::batterytoggle, &TTrain::OnCommand_batterytoggle },
	{ user_command::batteryenable, &TTrain::OnCommand_batteryenable }, { user_command::batterydisable, &TTrain::OnCommand_batterydisable },
	{ user_command::pantographcompressorvalvetoggle, &TTrain::OnCommand_pantographcompressorvalvetoggle },
	{ user_command::pantographcompressoractivate, &TTrain::OnCommand_pantographcompressoractivate }, { user_command::pantographtogglefront, &TTrain::OnCommand_pantographtogglefront },
	{ user_command::pantographtogglerear, &TTrain::OnCommand_pantographtogglerear }, { user_command::pantographraisefront, &TTrain::OnCommand_pantographraisefront },
	{ user_command::pantographraiserear, &TTrain::OnCommand_pantographraiserear }, { user_command::pantographlowerfront, &TTrain::OnCommand_pantographlowerfront },
	{ user_command::pantographlowerrear, &TTrain::OnCommand_pantographlowerrear }, { user_command::pantographlowerall, &TTrain::OnCommand_pantographlowerall },
	{ user_command::linebreakertoggle, &TTrain::OnCommand_linebreakertoggle }, { user_command::linebreakeropen, &TTrain::OnCommand_linebreakeropen },
	{ user_command::linebreakerclose, &TTrain::OnCommand_linebreakerclose }, { user_command::fuelpumptoggle, &TTrain::OnCommand_fuelpumptoggle },
	{ user_command::fuelpumpenable, &TTrain::OnCommand_fuelpumpenable }, { user_command::fuelpumpdisable, &TTrain::OnCommand_fuelpumpdisable },
	{ user_command::oilpumptoggle, &TTrain::OnCommand_oilpumptoggle }, { user_command::oilpumpenable, &TTrain::OnCommand_oilpumpenable },
	{ user_command::oilpumpdisable, &TTrain::OnCommand_oilpumpdisable }, { user_command::waterheaterbreakertoggle, &TTrain::OnCommand_waterheaterbreakertoggle },
	{ user_command::waterheaterbreakerclose, &TTrain::OnCommand_waterheaterbreakerclose }, { user_command::waterheaterbreakeropen, &TTrain::OnCommand_waterheaterbreakeropen },
	{ user_command::waterheatertoggle, &TTrain::OnCommand_waterheatertoggle }, { user_command::waterheaterenable, &TTrain::OnCommand_waterheaterenable },
	{ user_command::waterheaterdisable, &TTrain::OnCommand_waterheaterdisable }, { user_command::waterpumpbreakertoggle, &TTrain::OnCommand_waterpumpbreakertoggle },
	{ user_command::waterpumpbreakerclose, &TTrain::OnCommand_waterpumpbreakerclose }, { user_command::waterpumpbreakeropen, &TTrain::OnCommand_waterpumpbreakeropen },
	{ user_command::waterpumptoggle, &TTrain::OnCommand_waterpumptoggle }, { user_command::waterpumpenable, &TTrain::OnCommand_waterpumpenable },
	{ user_command::waterpumpdisable, &TTrain::OnCommand_waterpumpdisable }, { user_command::watercircuitslinktoggle, &TTrain::OnCommand_watercircuitslinktoggle },
	{ user_command::watercircuitslinkenable, &TTrain::OnCommand_watercircuitslinkenable }, { user_command::watercircuitslinkdisable, &TTrain::OnCommand_watercircuitslinkdisable },
	{ user_command::convertertoggle, &TTrain::OnCommand_convertertoggle }, { user_command::converterenable, &TTrain::OnCommand_converterenable },
	{ user_command::converterdisable, &TTrain::OnCommand_converterdisable }, { user_command::convertertogglelocal, &TTrain::OnCommand_convertertogglelocal },
	{ user_command::converteroverloadrelayreset, &TTrain::OnCommand_converteroverloadrelayreset }, { user_command::compressortoggle, &TTrain::OnCommand_compressortoggle },
	{ user_command::compressorenable, &TTrain::OnCommand_compressorenable }, { user_command::compressordisable, &TTrain::OnCommand_compressordisable },
	{ user_command::compressortogglelocal, &TTrain::OnCommand_compressortogglelocal }, { user_command::motorconnectorsopen, &TTrain::OnCommand_motorconnectorsopen },
	{ user_command::motorconnectorsclose, &TTrain::OnCommand_motorconnectorsclose }, { user_command::motordisconnect, &TTrain::OnCommand_motordisconnect },
	{ user_command::motoroverloadrelaythresholdtoggle, &TTrain::OnCommand_motoroverloadrelaythresholdtoggle },
	{ user_command::motoroverloadrelaythresholdsetlow, &TTrain::OnCommand_motoroverloadrelaythresholdsetlow },
	{ user_command::motoroverloadrelaythresholdsethigh, &TTrain::OnCommand_motoroverloadrelaythresholdsethigh }, { user_command::motoroverloadrelayreset, &TTrain::OnCommand_motoroverloadrelayreset },
	{ user_command::heatingtoggle, &TTrain::OnCommand_heatingtoggle }, { user_command::heatingenable, &TTrain::OnCommand_heatingenable },
	{ user_command::heatingdisable, &TTrain::OnCommand_heatingdisable }, { user_command::lightspresetactivatenext, &TTrain::OnCommand_lightspresetactivatenext },
	{ user_command::lightspresetactivateprevious, &TTrain::OnCommand_lightspresetactivateprevious }, { user_command::headlighttoggleleft, &TTrain::OnCommand_headlighttoggleleft },
	{ user_command::headlightenableleft, &TTrain::OnCommand_headlightenableleft }, { user_command::headlightdisableleft, &TTrain::OnCommand_headlightdisableleft },
	{ user_command::headlighttoggleright, &TTrain::OnCommand_headlighttoggleright }, { user_command::headlightenableright, &TTrain::OnCommand_headlightenableright },
	{ user_command::headlightdisableright, &TTrain::OnCommand_headlightdisableright }, { user_command::headlighttoggleupper, &TTrain::OnCommand_headlighttoggleupper },
	{ user_command::headlightenableupper, &TTrain::OnCommand_headlightenableupper }, { user_command::headlightdisableupper, &TTrain::OnCommand_headlightdisableupper },
	{ user_command::redmarkertoggleleft, &TTrain::OnCommand_redmarkertoggleleft }, { user_command::redmarkerenableleft, &TTrain::OnCommand_redmarkerenableleft },
	{ user_command::redmarkerdisableleft, &TTrain::OnCommand_redmarkerdisableleft }, { user_command::redmarkertoggleright, &TTrain::OnCommand_redmarkertoggleright },
	{ user_command::redmarkerenableright, &TTrain::OnCommand_redmarkerenableright }, { user_command::redmarkerdisableright, &TTrain::OnCommand_redmarkerdisableright },
	{ user_command::headlighttogglerearleft, &TTrain::OnCommand_headlighttogglerearleft }, { user_command::headlighttogglerearright, &TTrain::OnCommand_headlighttogglerearright },
	{ user_command::headlighttogglerearupper, &TTrain::OnCommand_headlighttogglerearupper }, { user_command::redmarkertogglerearleft, &TTrain::OnCommand_redmarkertogglerearleft },
	{ user_command::redmarkertogglerearright, &TTrain::OnCommand_redmarkertogglerearright }, { user_command::redmarkerstoggle, &TTrain::OnCommand_redmarkerstoggle },
	{ user_command::endsignalstoggle, &TTrain::OnCommand_endsignalstoggle }, { user_command::headlightsdimtoggle, &TTrain::OnCommand_headlightsdimtoggle },
	{ user_command::headlightsdimenable, &TTrain::OnCommand_headlightsdimenable }, { user_command::headlightsdimdisable, &TTrain::OnCommand_headlightsdimdisable },
	{ user_command::interiorlighttoggle, &TTrain::OnCommand_interiorlighttoggle }, { user_command::interiorlightenable, &TTrain::OnCommand_interiorlightenable },
	{ user_command::interiorlightdimdisable, &TTrain::OnCommand_interiorlightdisable }, { user_command::interiorlightdimtoggle, &TTrain::OnCommand_interiorlightdimtoggle },
	{ user_command::interiorlightdimenable, &TTrain::OnCommand_interiorlightdimenable }, { user_command::interiorlightdimdisable, &TTrain::OnCommand_interiorlightdimdisable },
	{ user_command::instrumentlighttoggle, &TTrain::OnCommand_instrumentlighttoggle }, { user_command::instrumentlightenable, &TTrain::OnCommand_instrumentlightenable },
	{ user_command::instrumentlightdisable, &TTrain::OnCommand_instrumentlightdisable }, { user_command::doorlocktoggle, &TTrain::OnCommand_doorlocktoggle },
	{ user_command::doortoggleleft, &TTrain::OnCommand_doortoggleleft }, { user_command::doortoggleright, &TTrain::OnCommand_doortoggleright },
	{ user_command::carcouplingincrease, &TTrain::OnCommand_carcouplingincrease }, { user_command::carcouplingdisconnect, &TTrain::OnCommand_carcouplingdisconnect },
	{ user_command::departureannounce, &TTrain::OnCommand_departureannounce }, { user_command::hornlowactivate, &TTrain::OnCommand_hornlowactivate },
	{ user_command::hornhighactivate, &TTrain::OnCommand_hornhighactivate }, { user_command::whistleactivate, &TTrain::OnCommand_whistleactivate },
	{ user_command::radiotoggle, &TTrain::OnCommand_radiotoggle }, { user_command::radiochannelincrease, &TTrain::OnCommand_radiochannelincrease },
	{ user_command::radiochanneldecrease, &TTrain::OnCommand_radiochanneldecrease }, { user_command::radiostopsend, &TTrain::OnCommand_radiostopsend },
	{ user_command::radiostoptest, &TTrain::OnCommand_radiostoptest }, { user_command::cabchangeforward, &TTrain::OnCommand_cabchangeforward },
	{ user_command::cabchangebackward, &TTrain::OnCommand_cabchangebackward }, { user_command::generictoggle0, &TTrain::OnCommand_generictoggle },
	{ user_command::generictoggle1, &TTrain::OnCommand_generictoggle }, { user_command::generictoggle2, &TTrain::OnCommand_generictoggle },
	{ user_command::generictoggle3, &TTrain::OnCommand_generictoggle }, { user_command::generictoggle4, &TTrain::OnCommand_generictoggle },
	{ user_command::generictoggle5, &TTrain::OnCommand_generictoggle }, { user_command::generictoggle6, &TTrain::OnCommand_generictoggle },
	{ user_command::generictoggle7, &TTrain::OnCommand_generictoggle }, { user_command::generictoggle8, &TTrain::OnCommand_generictoggle },
	{ user_command::generictoggle9, &TTrain::OnCommand_generictoggle }
};

std::vector<std::string> const TTrain::fPress_labels = {

	"ch1:  ", "ch2:  ", "ch3:  ", "ch4:  ", "ch5:  ", "ch6:  ", "ch7:  ", "ch8:  ", "ch9:  ", "ch0:  "
};

TTrain::TTrain()
{
	ShowNextCurrent = false;
	// McZapkie-240302 - przyda sie do tachometru
	fTachoVelocity = 0;
	fTachoCount = 0;
	fPPress = fNPress = 0;

	// asMessage="";
	pMechShake = Math3D::vector3(0, 0, 0);
	vMechMovement = Math3D::vector3(0, 0, 0);
	pMechOffset = Math3D::vector3(0, 0, 0);
	fBlinkTimer = 0;
	fHaslerTimer = 0;
	DynamicSet(NULL); // ustawia wszystkie mv*
	iCabLightFlag = 0;
	// hunter-091012
	bCabLight = false;
	bCabLightDim = false;
	//-----
	pMechSittingPosition = Math3D::vector3(0, 0, 0); // ABu: 180404
	InstrumentLightActive = false; // ABu: 030405
	fTachoTimer = 0.0; // włączenie skoków wskazań prędkościomierza

	//
	for (int i = 0; i < 8; i++) {
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
	for (int i = 0; i < 9; ++i)
		for (int j = 0; j < 10; ++j)
			fEIMParams[i][j] = 0.0;

	for (int i = 0; i < 20; ++i)
		for (int j = 0; j < 3; ++j)
			fPress[i][j] = 0.0;
}

TTrain::~TTrain() {}

bool TTrain::Init(TDynamicObject* NewDynamicObject, bool e3d)
{ // powiązanie ręcznego sterowania kabiną z pojazdem
	if (NewDynamicObject->Mechanik == nullptr) {
		ErrorLog("Bad config: can't take control of inactive vehicle \"" + NewDynamicObject->asName + "\"");
		return false;
	}

	DynamicSet(NewDynamicObject);
	if (!e3d)
		if (DynamicObject->Mechanik == NULL)
			return false;

	DynamicObject->MechInside = true;

	MechSpring.Init(125.0);
	vMechVelocity = Math3D::vector3(0, 0, 0);
	pMechOffset = Math3D::vector3(0, 0, 0);
	fMechSpringX = 0.2;
	fMechSpringY = 0.2;
	fMechSpringZ = 0.1;
	fMechMaxSpring = 0.15;
	fMechRoll = 0.05;
	fMechPitch = 0.1;
	fMainRelayTimer = 0; // Hunter, do k...y nędzy, ustawiaj wartości początkowe zmiennych!

	if (false == LoadMMediaFile(DynamicObject->asBaseDir + DynamicObject->MoverParameters->TypeName + ".mmd")) {
		return false;
	}

	iCabn = 0;
	// Ra: taka proteza - przesłanie kierunku do członów connected
	if (mvControlled->ActiveDir > 0) { // było do przodu
		mvControlled->DirectionBackward();
		mvControlled->DirectionForward();
	} else if (mvControlled->ActiveDir < 0) {
		mvControlled->DirectionForward();
		mvControlled->DirectionBackward();
	}
	return true;
}

PyObject* TTrain::GetTrainState()
{
	auto const* mover = DynamicObject->MoverParameters;
	auto* dict = PyDict_New();
	if ((dict == nullptr) || (mover == nullptr)) {
		return nullptr;
	}

	PyDict_SetItemString(dict, "cab", PyGetInt(mover->ActiveCab));
	// basic systems state data
	PyDict_SetItemString(dict, "battery", PyGetBool(mvControlled->Battery));
	PyDict_SetItemString(dict, "linebreaker", PyGetBool(mvControlled->Mains));
	PyDict_SetItemString(dict, "converter", PyGetBool(mover->ConverterFlag));
	PyDict_SetItemString(dict, "converter_overload", PyGetBool(mover->ConvOvldFlag));
	PyDict_SetItemString(dict, "compress", PyGetBool(mover->CompressorFlag));
	// reverser
	PyDict_SetItemString(dict, "direction", PyGetInt(mover->ActiveDir));
	// throttle
	PyDict_SetItemString(dict, "mainctrl_pos", PyGetInt(mover->MainCtrlPos));
	PyDict_SetItemString(dict, "main_ctrl_actual_pos", PyGetInt(mover->MainCtrlActualPos));
	PyDict_SetItemString(dict, "scndctrl_pos", PyGetInt(mover->ScndCtrlPos));
	PyDict_SetItemString(dict, "scnd_ctrl_actual_pos", PyGetInt(mover->ScndCtrlActualPos));
	// brakes
	PyDict_SetItemString(dict, "manual_brake", PyGetBool(mvOccupied->ManualBrakePos > 0));
	bool const bEP = (mvControlled->LocHandle->GetCP() > 0.2) || (fEIMParams[0][2] > 0.01);
	PyDict_SetItemString(dict, "dir_brake", PyGetBool(bEP));
	bool bPN;
	if ((typeid(*mvControlled->Hamulec) == typeid(TLSt)) || (typeid(*mvControlled->Hamulec) == typeid(TEStED))) {
		TBrake* temp_ham = mvControlled->Hamulec.get();
		bPN = (static_cast<TLSt*>(temp_ham)->GetEDBCP() > 0.2);
	} else
		bPN = false;
	PyDict_SetItemString(dict, "indir_brake", PyGetBool(bPN));
	// other controls
	PyDict_SetItemString(dict, "ca", PyGetBool(TestFlag(mvOccupied->SecuritySystem.Status, s_aware)));
	PyDict_SetItemString(dict, "shp", PyGetBool(TestFlag(mvOccupied->SecuritySystem.Status, s_active)));
	PyDict_SetItemString(dict, "pantpress", PyGetFloat(mvControlled->PantPress));
	PyDict_SetItemString(dict, "universal3", PyGetBool(InstrumentLightActive));
	PyDict_SetItemString(dict, "radio_channel", PyGetInt(iRadioChannel));
	// movement data
	PyDict_SetItemString(dict, "velocity", PyGetFloat(mover->Vel));
	PyDict_SetItemString(dict, "tractionforce", PyGetFloat(mover->Ft));
	PyDict_SetItemString(dict, "slipping_wheels", PyGetBool(mover->SlippingWheels));
	PyDict_SetItemString(dict, "sanding", PyGetBool(mover->SandDose));
	// electric current data
	PyDict_SetItemString(dict, "traction_voltage", PyGetFloat(mover->RunningTraction.TractionVoltage));
	PyDict_SetItemString(dict, "voltage", PyGetFloat(mover->Voltage));
	PyDict_SetItemString(dict, "im", PyGetFloat(mover->Im));
	PyDict_SetItemString(dict, "fuse", PyGetBool(mover->FuseFlag));
	PyDict_SetItemString(dict, "epfuse", PyGetBool(mover->EpFuse));
	// induction motor state data
	char const* TXTT[10] = { "fd", "fdt", "fdb", "pd", "pdt", "pdb", "itothv", "1", "2", "3" };
	char const* TXTC[10] = { "fr", "frt", "frb", "pr", "prt", "prb", "im", "vm", "ihv", "uhv" };
	char const* TXTP[3] = { "bc", "bp", "sp" };
	for (int j = 0; j < 10; ++j)
		PyDict_SetItemString(dict, (std::string("eimp_t_") + std::string(TXTT[j])).c_str(), PyGetFloatS(fEIMParams[0][j]));
	for (int i = 0; i < 8; ++i) {
		for (int j = 0; j < 10; ++j)
			PyDict_SetItemString(dict, ("eimp_c" + std::to_string(i + 1) + "_" + std::string(TXTC[j])).c_str(), PyGetFloatS(fEIMParams[i + 1][j]));

		PyDict_SetItemString(dict, ("eimp_c" + std::to_string(i + 1) + "_ms").c_str(), PyGetBool(bMains[i]));
		PyDict_SetItemString(dict, ("eimp_c" + std::to_string(i + 1) + "_cv").c_str(), PyGetFloatS(fCntVol[i]));
		PyDict_SetItemString(dict, ("eimp_u" + std::to_string(i + 1) + "_pf").c_str(), PyGetBool(bPants[i][0]));
		PyDict_SetItemString(dict, ("eimp_u" + std::to_string(i + 1) + "_pr").c_str(), PyGetBool(bPants[i][1]));
		PyDict_SetItemString(dict, ("eimp_c" + std::to_string(i + 1) + "_fuse").c_str(), PyGetBool(bFuse[i]));
		PyDict_SetItemString(dict, ("eimp_c" + std::to_string(i + 1) + "_batt").c_str(), PyGetBool(bBatt[i]));
		PyDict_SetItemString(dict, ("eimp_c" + std::to_string(i + 1) + "_conv").c_str(), PyGetBool(bConv[i]));
		PyDict_SetItemString(dict, ("eimp_u" + std::to_string(i + 1) + "_comp_a").c_str(), PyGetBool(bComp[i][0]));
		PyDict_SetItemString(dict, ("eimp_u" + std::to_string(i + 1) + "_comp_w").c_str(), PyGetBool(bComp[i][1]));
		PyDict_SetItemString(dict, ("eimp_c" + std::to_string(i + 1) + "_heat").c_str(), PyGetBool(bHeat[i]));
	}
	for (int i = 0; i < 20; ++i) {
		for (int j = 0; j < 3; ++j)
			PyDict_SetItemString(dict, ("eimp_pn" + std::to_string(i + 1) + "_" + TXTP[j]).c_str(), PyGetFloatS(fPress[i][j]));
	}
	// multi-unit state data
	PyDict_SetItemString(dict, "car_no", PyGetInt(iCarNo));
	PyDict_SetItemString(dict, "power_no", PyGetInt(iPowerNo));
	PyDict_SetItemString(dict, "unit_no", PyGetInt(iUnitNo));

	for (int i = 0; i < 20; i++) {
		PyDict_SetItemString(dict, ("doors_" + std::to_string(i + 1)).c_str(), PyGetFloatS(bDoors[i][0]));
		PyDict_SetItemString(dict, ("doors_r_" + std::to_string(i + 1)).c_str(), PyGetFloatS(bDoors[i][1]));
		PyDict_SetItemString(dict, ("doors_l_" + std::to_string(i + 1)).c_str(), PyGetFloatS(bDoors[i][2]));
		PyDict_SetItemString(dict, ("doors_no_" + std::to_string(i + 1)).c_str(), PyGetInt(iDoorNo[i]));
		PyDict_SetItemString(dict, ("code_" + std::to_string(i + 1)).c_str(), PyGetString((std::to_string(iUnits[i]) + cCode[i]).c_str()));
		PyDict_SetItemString(dict, ("car_name" + std::to_string(i + 1)).c_str(), PyGetString(asCarName[i].c_str()));
		PyDict_SetItemString(dict, ("slip_" + std::to_string(i + 1)).c_str(), PyGetBool(bSlip[i]));
	}
	// ai state data
	auto const* driver = DynamicObject->Mechanik;

	PyDict_SetItemString(dict, "velocity_desired", PyGetFloat(driver->VelDesired));
	PyDict_SetItemString(dict, "velroad", PyGetFloat(driver->VelRoad));
	PyDict_SetItemString(dict, "vellimitlast", PyGetFloat(driver->VelLimitLast));
	PyDict_SetItemString(dict, "velsignallast", PyGetFloat(driver->VelSignalLast));
	PyDict_SetItemString(dict, "velsignalnext", PyGetFloat(driver->VelSignalNext));
	PyDict_SetItemString(dict, "velnext", PyGetFloat(driver->VelNext));
	PyDict_SetItemString(dict, "actualproximitydist", PyGetFloat(driver->ActualProximityDist));
	// train data
	PyDict_SetItemString(dict, "trainnumber", PyGetString(driver->TrainName().c_str()));
	PyDict_SetItemString(dict, "train_stationindex", PyGetInt(driver->StationIndex()));
	auto const stationcount{ driver->StationCount() };
	PyDict_SetItemString(dict, "train_stationcount", PyGetInt(stationcount));
	if (stationcount > 0) {
		// timetable stations data, if there's any
		auto const* timetable{ driver->TrainTimetable() };
		for (auto stationidx = 1; stationidx <= stationcount; ++stationidx) {
			auto const stationlabel{ "train_station" + std::to_string(stationidx) + "_" };
			auto const& timetableline{ timetable->TimeTable[stationidx] };
			PyDict_SetItemString(dict, (stationlabel + "name").c_str(), PyGetString(Bezogonkow(timetableline.StationName).c_str()));
			PyDict_SetItemString(dict, (stationlabel + "fclt").c_str(), PyGetString(Bezogonkow(timetableline.StationWare).c_str()));
			PyDict_SetItemString(dict, (stationlabel + "lctn").c_str(), PyGetFloat(timetableline.km));
			PyDict_SetItemString(dict, (stationlabel + "vmax").c_str(), PyGetInt(timetableline.vmax));
			PyDict_SetItemString(dict, (stationlabel + "ah").c_str(), PyGetInt(timetableline.Ah));
			PyDict_SetItemString(dict, (stationlabel + "am").c_str(), PyGetInt(timetableline.Am));
			PyDict_SetItemString(dict, (stationlabel + "dh").c_str(), PyGetInt(timetableline.Dh));
			PyDict_SetItemString(dict, (stationlabel + "dm").c_str(), PyGetInt(timetableline.Dm));
		}
	}
	// world state data
	PyDict_SetItemString(dict, "hours", PyGetInt(simulation::Time.data().wHour));
	PyDict_SetItemString(dict, "minutes", PyGetInt(simulation::Time.data().wMinute));
	PyDict_SetItemString(dict, "seconds", PyGetInt(simulation::Time.second()));
	PyDict_SetItemString(dict, "air_temperature", PyGetInt(Global.AirTemperature));

	return dict;
}

TTrain::state_t TTrain::get_state() const
{
	return { btLampkaSHP.getValue(), btLampkaCzuwaka.getValue(), btLampkaRadioStop.getValue(), btLampkaOpory.getValue(), btLampkaWylSzybki.getValue(), btLampkaNadmSil.getValue(),
		btLampkaStyczn.getValue(), btLampkaPoslizg.getValue(), btLampkaNadmPrzetw.getValue(), btLampkaPrzetwOff.getValue(), btLampkaNadmSpr.getValue(), btLampkaNadmWent.getValue(),
		btLampkaWysRozr.getValue(), btLampkaOgrzewanieSkladu.getValue(), btHaslerBrakes.getValue(), btHaslerCurrent.getValue(),
		(TestFlag(mvOccupied->SecuritySystem.Status, s_CAalarm) || TestFlag(mvOccupied->SecuritySystem.Status, s_SHPalarm)), fTachoVelocity, static_cast<float>(mvOccupied->Compressor),
		static_cast<float>(mvOccupied->PipePress), static_cast<float>(mvOccupied->BrakePress), fHVoltage, { fHCurrent[(mvControlled->TrainType & dt_EZT) ? 0 : 1], fHCurrent[2], fHCurrent[3] } };
}

bool TTrain::is_eztoer() const
{
	return (
	(mvControlled->TrainType == dt_EZT) && (mvOccupied->BrakeSubsystem == ss_ESt) && (mvControlled->Battery == true) && (mvControlled->EpFuse == true) && (mvControlled->ActiveDir != 0)); // od yB
}

// moves train brake lever to specified position, potentially emits switch sound if conditions are met
void TTrain::set_train_brake(double const Position)
{
	auto const originalbrakeposition{ static_cast<int>(100.0 * mvOccupied->fBrakeCtrlPos) };

	mvOccupied->BrakeLevelSet(Position);

	if (static_cast<int>(100.0 * mvOccupied->fBrakeCtrlPos) == originalbrakeposition) {
		return;
	}

	if ((true == is_eztoer()) && (false == (((originalbrakeposition / 100 == 0) || (originalbrakeposition / 100 >= 5)) && ((mvOccupied->BrakeCtrlPos == 0) || (mvOccupied->BrakeCtrlPos >= 5))))) {
		// sound feedback if the lever movement activates one of the switches
		dsbPneumaticSwitch.play();
	}
}

void TTrain::set_train_brake_speed(TDynamicObject* Vehicle, int const Speed)
{
	if (true == Vehicle->MoverParameters->BrakeDelaySwitch(Speed)) {
		// visual feedback
		// TODO: add setting indicator to vehicle class, for external lever/indicator
		if (Vehicle == DynamicObject) {
			if (ggBrakeProfileCtrl.SubModel != nullptr) {
				ggBrakeProfileCtrl.UpdateValue(((mvOccupied->BrakeDelayFlag & bdelay_R) != 0 ? 2.0 : mvOccupied->BrakeDelayFlag - 1), dsbSwitch);
			}
			if (ggBrakeProfileG.SubModel != nullptr) {
				ggBrakeProfileG.UpdateValue((mvOccupied->BrakeDelayFlag == bdelay_G ? 1.0 : 0.0), dsbSwitch);
			}
			if (ggBrakeProfileR.SubModel != nullptr) {
				ggBrakeProfileR.UpdateValue(((mvOccupied->BrakeDelayFlag & bdelay_R) != 0 ? 1.0 : 0.0), dsbSwitch);
			}
		}
	}
}

void TTrain::set_paired_open_motor_connectors_button(bool const State)
{
	if ((mvControlled->TrainType == dt_ET41) || (mvControlled->TrainType == dt_ET42)) {
		// crude implementation of the button affecting entire unit for multi-unit engines
		// TODO: rework it into part of standard command propagation system
		if ((mvControlled->Couplers[side::front].Connected != nullptr) && (true == TestFlag(mvControlled->Couplers[side::front].CouplingFlag, coupling::permanent))) {
			mvControlled->Couplers[side::front].Connected->StLinSwitchOff = State;
		}
		if ((mvControlled->Couplers[side::rear].Connected != nullptr) && (true == TestFlag(mvControlled->Couplers[side::rear].CouplingFlag, coupling::permanent))) {
			mvControlled->Couplers[side::rear].Connected->StLinSwitchOff = State;
		}
	}
}

// locates nearest vehicle belonging to the consist
TDynamicObject* TTrain::find_nearest_consist_vehicle() const
{
	if (false == FreeFlyModeFlag) {
		return DynamicObject;
	}
	auto coupler{ -2 }; // scan for vehicle, not any specific coupler
	auto* vehicle{ DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(), 1, 1500, coupler) };
	if (vehicle == nullptr)
		vehicle = DynamicObject->ABuScanNearestObject(DynamicObject->GetTrack(), -1, 1500, coupler);
	// TBD, TODO: perform owner test for the located vehicle
	return vehicle;
}

// command handlers
void TTrain::OnCommand_aidriverenable(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// on press
		if (Train->DynamicObject->Mechanik == nullptr) {
			return;
		}

		if (true == Train->DynamicObject->Mechanik->AIControllFlag) {
			//żeby nie trzeba było rozłączać dla zresetowania
			Train->DynamicObject->Mechanik->TakeControl(false);
		}
		Train->DynamicObject->Mechanik->TakeControl(true);
	}
}

void TTrain::OnCommand_aidriverdisable(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// on press
		if (Train->DynamicObject->Mechanik)
			Train->DynamicObject->Mechanik->TakeControl(false);
	}
}

void TTrain::OnCommand_mastercontrollerincrease(TTrain* Train, command_data const& Command)
{
	if (Command.action != GLFW_RELEASE) {
		// on press or hold
		Train->mvControlled->IncMainCtrl(1);
	}
}

void TTrain::OnCommand_mastercontrollerincreasefast(TTrain* Train, command_data const& Command)
{
	if (Command.action != GLFW_RELEASE) {
		// on press or hold
		Train->mvControlled->IncMainCtrl(2);
	}
}

void TTrain::OnCommand_mastercontrollerdecrease(TTrain* Train, command_data const& Command)
{
	if (Command.action != GLFW_RELEASE) {
		// on press or hold
		Train->mvControlled->DecMainCtrl(1);
	}
}

void TTrain::OnCommand_mastercontrollerdecreasefast(TTrain* Train, command_data const& Command)
{
	if (Command.action != GLFW_RELEASE) {
		// on press or hold
		Train->mvControlled->DecMainCtrl(2);
	}
}

void TTrain::OnCommand_mastercontrollerset(TTrain* Train, command_data const& Command)
{
	auto positionchange{ std::min<int>(
		                 Command.param1, (Train->mvControlled->CoupledCtrl ? Train->mvControlled->MainCtrlPosNo + Train->mvControlled->ScndCtrlPosNo : Train->mvControlled->MainCtrlPosNo)) -
		                 (Train->mvControlled->CoupledCtrl ? Train->mvControlled->MainCtrlPos + Train->mvControlled->ScndCtrlPos : Train->mvControlled->MainCtrlPos) };
	while ((positionchange < 0) && (true == Train->mvControlled->DecMainCtrl(1))) {
		++positionchange;
	}
	while ((positionchange > 0) && (true == Train->mvControlled->IncMainCtrl(1))) {
		--positionchange;
	}
}

void TTrain::OnCommand_secondcontrollerincrease(TTrain* Train, command_data const& Command)
{
	if (Command.action != GLFW_RELEASE) {
		// on press or hold
		if ((Train->mvControlled->EngineType == DieselElectric) && (true == Train->mvControlled->ShuntMode)) {
			Train->mvControlled->AnPos = clamp(Train->mvControlled->AnPos + 0.025, 0.0, 1.0);
		} else {
			Train->mvControlled->IncScndCtrl(1);
		}
	}
}

void TTrain::OnCommand_secondcontrollerincreasefast(TTrain* Train, command_data const& Command)
{
	if (Command.action != GLFW_RELEASE) {
		// on press or hold
		if ((Train->mvControlled->EngineType == DieselElectric) && (true == Train->mvControlled->ShuntMode)) {
			Train->mvControlled->AnPos = 1.0;
		} else {
			Train->mvControlled->IncScndCtrl(2);
		}
	}
}

void TTrain::OnCommand_notchingrelaytoggle(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (false == Train->mvOccupied->AutoRelayFlag) {
			// turn on
			Train->mvOccupied->AutoRelaySwitch(true);
		} else {
			//turn off
			Train->mvOccupied->AutoRelaySwitch(false);
		}
	}
}

void TTrain::OnCommand_mucurrentindicatorothersourceactivate(TTrain* Train, command_data const& Command)
{
	if (Train->ggNextCurrentButton.SubModel == nullptr) {
		if (Command.action == GLFW_PRESS) {
			WriteLog("Current Indicator Source switch is missing, or wasn't defined");
		}
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// turn on
		Train->ShowNextCurrent = true;
		// visual feedback
		Train->ggNextCurrentButton.UpdateValue(1.0, Train->dsbSwitch);
	} else if (Command.action == GLFW_RELEASE) {
		//turn off
		Train->ShowNextCurrent = false;
		// visual feedback
		Train->ggNextCurrentButton.UpdateValue(0.0, Train->dsbSwitch);
	}
}

void TTrain::OnCommand_secondcontrollerdecrease(TTrain* Train, command_data const& Command)
{
	if (Command.action != GLFW_RELEASE) {
		// on press or hold
		if ((Train->mvControlled->EngineType == DieselElectric) && (true == Train->mvControlled->ShuntMode)) {
			Train->mvControlled->AnPos = clamp(Train->mvControlled->AnPos - 0.025, 0.0, 1.0);
		} else {
			Train->mvControlled->DecScndCtrl(1);
		}
	}
}

void TTrain::OnCommand_secondcontrollerdecreasefast(TTrain* Train, command_data const& Command)
{
	if (Command.action != GLFW_RELEASE) {
		// on press or hold
		if ((Train->mvControlled->EngineType == DieselElectric) && (true == Train->mvControlled->ShuntMode)) {
			Train->mvControlled->AnPos = 0.0;
		} else {
			Train->mvControlled->DecScndCtrl(2);
		}
	}
}

void TTrain::OnCommand_secondcontrollerset(TTrain* Train, command_data const& Command)
{
	auto const targetposition{ std::min<int>(Command.param1, Train->mvControlled->ScndCtrlPosNo) };
	while ((targetposition < Train->mvControlled->ScndCtrlPos) && (true == Train->mvControlled->DecScndCtrl(1))) {
		// all work is done in the header
		;
	}
	while ((targetposition > Train->mvControlled->ScndCtrlPos) && (true == Train->mvControlled->IncScndCtrl(1))) {
		// all work is done in the header
		;
	}
}

void TTrain::OnCommand_independentbrakeincrease(TTrain* Train, command_data const& Command)
{
	if (Command.action != GLFW_RELEASE) {
		if (Train->mvOccupied->LocalBrake != ManualBrake) {
			Train->mvOccupied->IncLocalBrakeLevel(1);
		}
	}
}

void TTrain::OnCommand_independentbrakeincreasefast(TTrain* Train, command_data const& Command)
{
	if (Command.action != GLFW_RELEASE) {
		if (Train->mvOccupied->LocalBrake != ManualBrake) {
			Train->mvOccupied->IncLocalBrakeLevel(2);
		}
	}
}

void TTrain::OnCommand_independentbrakedecrease(TTrain* Train, command_data const& Command)
{
	if (Command.action != GLFW_RELEASE) {
		if ((Train->mvOccupied->LocalBrake != ManualBrake)
		    // Ra 1014-06: AI potrafi zahamować pomocniczym mimo jego braku - odhamować jakoś trzeba
		    // TODO: sort AI out so it doesn't do things it doesn't have equipment for
		    || (Train->mvOccupied->LocalBrakePos != 0)) {
			Train->mvOccupied->DecLocalBrakeLevel(1);
		}
	}
}

void TTrain::OnCommand_independentbrakedecreasefast(TTrain* Train, command_data const& Command)
{
	if (Command.action != GLFW_RELEASE) {
		if ((Train->mvOccupied->LocalBrake != ManualBrake)
		    // Ra 1014-06: AI potrafi zahamować pomocniczym mimo jego braku - odhamować jakoś trzeba
		    // TODO: sort AI out so it doesn't do things it doesn't have equipment for
		    || (Train->mvOccupied->LocalBrakePos != 0)) {
			Train->mvOccupied->DecLocalBrakeLevel(2);
		}
	}
}

void TTrain::OnCommand_independentbrakeset(TTrain* Train, command_data const& Command)
{
	Train->mvControlled->LocalBrakePos = (std::round(interpolate<double>(0.0, LocalBrakePosNo, clamp(reinterpret_cast<double const&>(Command.param1), 0.0, 1.0))));
}

void TTrain::OnCommand_independentbrakebailoff(TTrain* Train, command_data const& Command)
{
	if (false == FreeFlyModeFlag) {
		// TODO: check if this set of conditions can be simplified.
		// it'd be more flexible to have an attribute indicating whether bail off position is supported
		if ((Train->mvControlled->TrainType != dt_EZT) &&
		    ((Train->mvControlled->EngineType == ElectricSeriesMotor) || (Train->mvControlled->EngineType == DieselElectric) || (Train->mvControlled->EngineType == ElectricInductionMotor)) &&
		    (Train->mvOccupied->BrakeCtrlPosNo > 0)) {
			if (Command.action == GLFW_PRESS) {
				// press or hold
				// visual feedback
				Train->ggReleaserButton.UpdateValue(1.0, Train->dsbSwitch);

				Train->mvOccupied->BrakeReleaser(1);
			} else if (Command.action == GLFW_RELEASE) {
				// release
				// visual feedback
				Train->ggReleaserButton.UpdateValue(0.0, Train->dsbSwitch);

				Train->mvOccupied->BrakeReleaser(0);
			}
		}
	} else {
		// car brake handling, while in walk mode
		auto* vehicle{ Train->find_nearest_consist_vehicle() };
		if (vehicle != nullptr) {
			if (Command.action == GLFW_PRESS) {
				// press or hold
				vehicle->MoverParameters->BrakeReleaser(1);
			} else if (Command.action == GLFW_RELEASE) {
				// release
				vehicle->MoverParameters->BrakeReleaser(0);
			}
		}
	}
}

void TTrain::OnCommand_trainbrakeincrease(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_REPEAT && Train->mvOccupied->BrakeHandle == FV4a)
		Train->mvOccupied->BrakeLevelAdd(Global.brake_speed * Command.time_delta);
	else if (Command.action == GLFW_PRESS && Train->mvOccupied->BrakeHandle != FV4a)
		Train->set_train_brake(Train->mvOccupied->fBrakeCtrlPos + Global.fBrakeStep);
}

void TTrain::OnCommand_trainbrakedecrease(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_REPEAT && Train->mvOccupied->BrakeHandle == FV4a)
		Train->mvOccupied->BrakeLevelAdd(-Global.brake_speed * Command.time_delta);
	else if (Command.action == GLFW_PRESS && Train->mvOccupied->BrakeHandle != FV4a)
		Train->set_train_brake(Train->mvOccupied->fBrakeCtrlPos - Global.fBrakeStep);

	if (Command.action == GLFW_RELEASE) {
		// release
		if ((Train->mvOccupied->BrakeCtrlPos == -1) && (Train->mvOccupied->BrakeHandle == FVel6) && (Train->DynamicObject->Controller != AIdriver) && (Global.iFeedbackMode < 3)) {
			// Odskakiwanie hamulce EP
			Train->set_train_brake(0);
		}
	}
}

void TTrain::OnCommand_trainbrakeset(TTrain* Train, command_data const& Command)
{
	Train->mvControlled->BrakeLevelSet(
	interpolate(Train->mvControlled->Handle->GetPos(bh_MIN), Train->mvControlled->Handle->GetPos(bh_MAX), clamp(reinterpret_cast<double const&>(Command.param1), 0.0, 1.0)));
}

void TTrain::OnCommand_trainbrakecharging(TTrain* Train, command_data const& Command)
{
	if (Command.action != GLFW_RELEASE) {
		// press or hold
		Train->set_train_brake(-1);
	} else {
		// release
		if ((Train->mvOccupied->BrakeCtrlPos == -1) && (Train->mvOccupied->BrakeHandle == FVel6) && (Train->DynamicObject->Controller != AIdriver) && (Global.iFeedbackMode < 3)) {
			// Odskakiwanie hamulce EP
			Train->set_train_brake(0);
		}
	}
}

void TTrain::OnCommand_trainbrakerelease(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		Train->set_train_brake(0);
	}
}

void TTrain::OnCommand_trainbrakefirstservice(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		Train->set_train_brake(1);
	}
}

void TTrain::OnCommand_trainbrakeservice(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		Train->set_train_brake((Train->mvOccupied->BrakeCtrlPosNo / 2 + (Train->mvOccupied->BrakeHandle == FV4a ? 1 : 0)));
	}
}

void TTrain::OnCommand_trainbrakefullservice(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		Train->set_train_brake(Train->mvOccupied->BrakeCtrlPosNo - 1);
	}
}

void TTrain::OnCommand_trainbrakehandleoff(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		Train->set_train_brake(Train->mvOccupied->Handle->GetPos(bh_NP));
	}
}

void TTrain::OnCommand_trainbrakeemergency(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		Train->set_train_brake(Train->mvOccupied->Handle->GetPos(bh_EB));
		/*
        if( Train->mvOccupied->BrakeCtrlPosNo <= 0.1 ) {
            // hamulec bezpieczeństwa dla wagonów
            Train->mvOccupied->RadioStopFlag = true;
        }
*/
	}
}

void TTrain::OnCommand_trainbrakebasepressureincrease(TTrain* Train, command_data const& Command)
{
	if (Command.action != GLFW_RELEASE) {
		switch (Train->mvOccupied->BrakeHandle) {
			case FV4a: {
				Train->mvOccupied->BrakeCtrlPos2 = clamp(Train->mvOccupied->BrakeCtrlPos2 - 0.01, -1.5, 2.0);
				break;
			}
			default: {
				Train->mvOccupied->BrakeLevelAdd(0.01);
				break;
			}
		}
	}
}

void TTrain::OnCommand_trainbrakebasepressuredecrease(TTrain* Train, command_data const& Command)
{
	if (Command.action != GLFW_RELEASE) {
		switch (Train->mvOccupied->BrakeHandle) {
			case FV4a: {
				Train->mvOccupied->BrakeCtrlPos2 = clamp(Train->mvOccupied->BrakeCtrlPos2 + 0.01, -1.5, 2.0);
				break;
			}
			default: {
				Train->mvOccupied->BrakeLevelAdd(-0.01);
				break;
			}
		}
	}
}

void TTrain::OnCommand_trainbrakebasepressurereset(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		Train->mvOccupied->BrakeCtrlPos2 = 0;
	}
}

void TTrain::OnCommand_trainbrakeoperationtoggle(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		auto* vehicle{ Train->find_nearest_consist_vehicle() };
		if (vehicle == nullptr) {
			return;
		}

		vehicle->MoverParameters->Hamulec->SetBrakeStatus(vehicle->MoverParameters->Hamulec->GetBrakeStatus() ^ b_dmg);
	}
}

void TTrain::OnCommand_manualbrakeincrease(TTrain* Train, command_data const& Command)
{
	if (Command.action != GLFW_RELEASE) {
		auto* vehicle{ Train->find_nearest_consist_vehicle() };
		if (vehicle == nullptr) {
			return;
		}

		if ((vehicle->MoverParameters->LocalBrake == ManualBrake) || (vehicle->MoverParameters->MBrake == true)) {
			vehicle->MoverParameters->IncManualBrakeLevel(1);
		}
	}
}

void TTrain::OnCommand_manualbrakedecrease(TTrain* Train, command_data const& Command)
{
	if (Command.action != GLFW_RELEASE) {
		auto* vehicle{ Train->find_nearest_consist_vehicle() };
		if (vehicle == nullptr) {
			return;
		}

		if ((vehicle->MoverParameters->LocalBrake == ManualBrake) || (vehicle->MoverParameters->MBrake == true)) {
			vehicle->MoverParameters->DecManualBrakeLevel(1);
		}
	}
}

void TTrain::OnCommand_alarmchaintoggle(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		if (false == Train->mvOccupied->AlarmChainFlag) {
			// pull
			Train->mvOccupied->AlarmChainSwitch(true);
			// visual feedback
			Train->ggAlarmChain.UpdateValue(1.0);
		} else {
			// release
			Train->mvOccupied->AlarmChainSwitch(false);
			// visual feedback
			Train->ggAlarmChain.UpdateValue(0.0);
		}
	}
}

void TTrain::OnCommand_wheelspinbrakeactivate(TTrain* Train, command_data const& Command)
{
	// TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
	if (Train->ggAntiSlipButton.SubModel == nullptr) {
		if (Command.action == GLFW_PRESS) {
			WriteLog("Wheelspin Brake button is missing, or wasn't defined");
		}
		return;
	}

	if (Train->mvOccupied->BrakeSystem != ElectroPneumatic) {
		// standard behaviour
		if (Command.action == GLFW_PRESS) {
			// visual feedback
			Train->ggAntiSlipButton.UpdateValue(1.0, Train->dsbSwitch);

			// NOTE: system activation is (repeatedly) done in the train update routine
		} else if (Command.action == GLFW_RELEASE) {
			// visual feedback
			Train->ggAntiSlipButton.UpdateValue(0.0);
		}
	} else {
		// electro-pneumatic, custom case
		if (Command.action == GLFW_PRESS) {
			// visual feedback
			Train->ggAntiSlipButton.UpdateValue(1.0, Train->dsbPneumaticSwitch);

			if ((Train->mvOccupied->BrakeHandle == St113) && (Train->mvControlled->EpFuse == true)) {
				Train->mvOccupied->SwitchEPBrake(1);
			}
		} else if (Command.action == GLFW_RELEASE) {
			// visual feedback
			Train->ggAntiSlipButton.UpdateValue(0.0);

			Train->mvOccupied->SwitchEPBrake(0);
		}
	}
}

void TTrain::OnCommand_sandboxactivate(TTrain* Train, command_data const& Command)
{
	// TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
	if (Train->ggSandButton.SubModel == nullptr) {
		if (Command.action == GLFW_PRESS) {
			WriteLog("Sandbox activation button is missing, or wasn't defined");
		}
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// visual feedback
		Train->ggSandButton.UpdateValue(1.0, Train->dsbSwitch);

		Train->mvControlled->Sandbox(true);
	} else if (Command.action == GLFW_RELEASE) {
		// visual feedback
		Train->ggSandButton.UpdateValue(0.0);

		Train->mvControlled->Sandbox(false);
	}
}

void TTrain::OnCommand_epbrakecontroltoggle(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (false == Train->mvOccupied->EpFuse) {
			// turn on
			if (Train->mvOccupied->EpFuseSwitch(true)) {
				// audio feedback
				Train->dsbPneumaticSwitch.play();
				// visual feedback
				// NOTE: there's no button for ep brake control switch
				// TBD, TODO: add ep brake control switch?
			}
		} else {
			//turn off
			if (Train->mvOccupied->EpFuseSwitch(false)) {
				// audio feedback
				Train->dsbPneumaticSwitch.play();
				// visual feedback
				// NOTE: there's no button for ep brake control switch
				// TBD, TODO: add ep brake control switch?
			}
		}
	}
}

void TTrain::OnCommand_trainbrakeoperationmodeincrease(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (((Train->mvOccupied->BrakeOpModeFlag << 1) & Train->mvOccupied->BrakeOpModes) != 0) {
			// next mode
			Train->mvOccupied->BrakeOpModeFlag <<= 1;
			// audio feedback
			Train->dsbPneumaticSwitch.play();
			// visual feedback
			// NOTE: there's no button for brake operation mode switch
			// TBD, TODO: add brake operation mode switch?
		}
	}
}

void TTrain::OnCommand_trainbrakeoperationmodedecrease(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (((Train->mvOccupied->BrakeOpModeFlag >> 1) & Train->mvOccupied->BrakeOpModes) != 0) {
			// previous mode
			Train->mvOccupied->BrakeOpModeFlag >>= 1;
			// audio feedback
			Train->dsbPneumaticSwitch.play();
			// visual feedback
			// NOTE: there's no button for brake operation mode switch
			// TBD, TODO: add brake operation mode switch?
		}
	}
}

void TTrain::OnCommand_brakeactingspeedincrease(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		auto* vehicle{ Train->find_nearest_consist_vehicle() };
		if (vehicle == nullptr) {
			return;
		}

		if ((vehicle->MoverParameters->BrakeDelayFlag & bdelay_M) != 0) {
			// can't speed it up any more than this
			return;
		}
		auto const fasterbrakesetting = (vehicle->MoverParameters->BrakeDelayFlag < bdelay_R ? vehicle->MoverParameters->BrakeDelayFlag << 1 : vehicle->MoverParameters->BrakeDelayFlag | bdelay_M);

		Train->set_train_brake_speed(vehicle, fasterbrakesetting);
	}
}

void TTrain::OnCommand_brakeactingspeeddecrease(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		auto* vehicle{ Train->find_nearest_consist_vehicle() };
		if (vehicle == nullptr) {
			return;
		}

		if (vehicle->MoverParameters->BrakeDelayFlag == bdelay_G) {
			// can't slow it down any more than this
			return;
		}
		auto const slowerbrakesetting = (vehicle->MoverParameters->BrakeDelayFlag < bdelay_M ? vehicle->MoverParameters->BrakeDelayFlag >> 1 : vehicle->MoverParameters->BrakeDelayFlag ^ bdelay_M);

		Train->set_train_brake_speed(vehicle, slowerbrakesetting);
	}
}

void TTrain::OnCommand_brakeactingspeedsetcargo(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		auto* vehicle{ Train->find_nearest_consist_vehicle() };
		if (vehicle == nullptr) {
			return;
		}

		Train->set_train_brake_speed(vehicle, bdelay_G);
	}
}

void TTrain::OnCommand_brakeactingspeedsetpassenger(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		auto* vehicle{ Train->find_nearest_consist_vehicle() };
		if (vehicle == nullptr) {
			return;
		}

		Train->set_train_brake_speed(vehicle, bdelay_P);
	}
}

void TTrain::OnCommand_brakeactingspeedsetrapid(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		auto* vehicle{ Train->find_nearest_consist_vehicle() };
		if (vehicle == nullptr) {
			return;
		}

		Train->set_train_brake_speed(vehicle, bdelay_R);
	}
}

void TTrain::OnCommand_brakeloadcompensationincrease(TTrain* Train, command_data const& Command)
{
	if ((true == FreeFlyModeFlag) && (Command.action == GLFW_PRESS)) {
		auto* vehicle{ Train->find_nearest_consist_vehicle() };
		if (vehicle != nullptr) {
			vehicle->MoverParameters->IncBrakeMult();
		}
	}
}

void TTrain::OnCommand_brakeloadcompensationdecrease(TTrain* Train, command_data const& Command)
{
	if ((true == FreeFlyModeFlag) && (Command.action == GLFW_PRESS)) {
		auto* vehicle{ Train->find_nearest_consist_vehicle() };
		if (vehicle != nullptr) {
			vehicle->MoverParameters->DecBrakeMult();
		}
	}
}

void TTrain::OnCommand_mubrakingindicatortoggle(TTrain* Train, command_data const& Command)
{
	if (Train->ggSignallingButton.SubModel == nullptr) {
		if (Command.action == GLFW_PRESS) {
			WriteLog("Braking Indicator switch is missing, or wasn't defined");
		}
		return;
	}
	if (Train->mvControlled->TrainType != dt_EZT) {
		//
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (false == Train->mvControlled->Signalling) {
			// turn on
			Train->mvControlled->Signalling = true;
			// visual feedback
			Train->ggSignallingButton.UpdateValue(1.0, Train->dsbSwitch);
		} else {
			//turn off
			Train->mvControlled->Signalling = false;
			// visual feedback
			Train->ggSignallingButton.UpdateValue(0.0, Train->dsbSwitch);
		}
	}
}

void TTrain::OnCommand_reverserincrease(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		if (Train->mvOccupied->DirectionForward()) {
			// aktualizacja skrajnych pojazdów w składzie
			if ((Train->mvOccupied->ActiveDir) && (Train->DynamicObject->Mechanik)) {
				Train->DynamicObject->Mechanik->CheckVehicles(Change_direction);
			}
		}
	}
}

void TTrain::OnCommand_reverserdecrease(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		if (Train->mvOccupied->DirectionBackward()) {
			// aktualizacja skrajnych pojazdów w składzie
			if ((Train->mvOccupied->ActiveDir) && (Train->DynamicObject->Mechanik)) {
				Train->DynamicObject->Mechanik->CheckVehicles(Change_direction);
			}
		}
	}
}

void TTrain::OnCommand_reverserforwardhigh(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		OnCommand_reverserforward(Train, Command);
		OnCommand_reverserincrease(Train, Command);
	}
}

void TTrain::OnCommand_reverserforward(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// HACK: try to move the reverser one position back, in case it's set to "high forward"
		OnCommand_reverserdecrease(Train, Command);

		if (Train->mvOccupied->ActiveDir < 1) {
			while ((Train->mvOccupied->ActiveDir < 1) && (true == Train->mvOccupied->DirectionForward())) {
				// all work is done in the header
			}
			// aktualizacja skrajnych pojazdów w składzie
			if ((Train->mvOccupied->ActiveDir == 1) && (Train->DynamicObject->Mechanik)) {
				Train->DynamicObject->Mechanik->CheckVehicles(Change_direction);
			}
		}
	}
}

void TTrain::OnCommand_reverserneutral(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		while ((Train->mvOccupied->ActiveDir < 0) && (true == Train->mvOccupied->DirectionForward())) {
			// all work is done in the header
		}
		while ((Train->mvOccupied->ActiveDir > 0) && (true == Train->mvOccupied->DirectionBackward())) {
			// all work is done in the header
		}
	}
}

void TTrain::OnCommand_reverserbackward(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		if (Train->mvOccupied->ActiveDir > -1) {
			while ((Train->mvOccupied->ActiveDir > -1) && (true == Train->mvOccupied->DirectionBackward())) {
				// all work is done in the header
			}
			// aktualizacja skrajnych pojazdów w składzie
			if ((Train->mvOccupied->ActiveDir == -1) && (Train->DynamicObject->Mechanik)) {
				Train->DynamicObject->Mechanik->CheckVehicles(Change_direction);
			}
		}
	}
}

void TTrain::OnCommand_alerteracknowledge(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// visual feedback
		Train->ggSecurityResetButton.UpdateValue(1.0, Train->dsbSwitch);
		if (Train->CAflag == false) {
			Train->CAflag = true;
			Train->mvOccupied->SecuritySystemReset();
		}
	} else if (Command.action == GLFW_RELEASE) {
		// visual feedback
		Train->ggSecurityResetButton.UpdateValue(0.0);

		Train->fCzuwakTestTimer = 0.0f;
		if (TestFlag(Train->mvOccupied->SecuritySystem.Status, s_CAtest)) {
			SetFlag(Train->mvOccupied->SecuritySystem.Status, -s_CAtest);
			Train->mvOccupied->s_CAtestebrake = false;
			Train->mvOccupied->SecuritySystem.SystemBrakeCATestTimer = 0.0;
		}
		Train->CAflag = false;
	}
}

void TTrain::OnCommand_batterytoggle(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (false == Train->mvOccupied->Battery) {
			// turn on
			OnCommand_batteryenable(Train, Command);
		} else {
			//turn off
			OnCommand_batterydisable(Train, Command);
		}
	}
}

void TTrain::OnCommand_batteryenable(TTrain* Train, command_data const& Command)
{
	if (true == Train->mvOccupied->Battery) {
		return;
	} // already on

	if (Command.action == GLFW_PRESS) {
		// ignore repeats
		// wyłącznik jest też w SN61, ewentualnie załączać prąd na stałe z poziomu FIZ
		if (Train->mvOccupied->BatterySwitch(true)) {
			// bateria potrzebna np. do zapalenia świateł
			if (Train->ggBatteryButton.SubModel) {
				Train->ggBatteryButton.UpdateValue(1.0, Train->dsbSwitch);
			}
			// side-effects
			if (Train->mvOccupied->LightsPosNo > 0) {
				Train->SetLights();
			}
			if (TestFlag(Train->mvOccupied->SecuritySystem.SystemType, 2)) {
				// Ra: znowu w kabinie jest coś, co być nie powinno!
				SetFlag(Train->mvOccupied->SecuritySystem.Status, s_active);
				SetFlag(Train->mvOccupied->SecuritySystem.Status, s_SHPalarm);
			}
		}
	}
}

void TTrain::OnCommand_batterydisable(TTrain* Train, command_data const& Command)
{
	if (false == Train->mvOccupied->Battery) {
		return;
	} // already off

	if (Command.action == GLFW_PRESS) {
		// ignore repeats
		if (Train->mvOccupied->BatterySwitch(false)) {
			// ewentualnie zablokować z FIZ, np. w samochodach się nie odłącza akumulatora
			if (Train->ggBatteryButton.SubModel) {
				Train->ggBatteryButton.UpdateValue(0.0, Train->dsbSwitch);
			}
			// side-effects
			if (false == Train->mvControlled->ConverterFlag) {
				// if there's no (low voltage) power source left, drop pantographs
				Train->mvControlled->PantFront(false);
				Train->mvControlled->PantRear(false);
			}
		}
	}
}

void TTrain::OnCommand_pantographtogglefront(TTrain* Train, command_data const& cmd)
{
	command_data Command = cmd;

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (false == Train->mvControlled->PantFrontUp) {
			// turn on...
			OnCommand_pantographraisefront(Train, Command);
		} else {
			// ...or turn off
			OnCommand_pantographlowerfront(Train, Command);
		}
	} else if (Command.action == GLFW_RELEASE) {
		// impulse switches return automatically to neutral position
		// NOTE: this routine is used also by dedicated raise and lower commands
		if (Train->mvOccupied->PantSwitchType == "impulse") {
			if (Train->ggPantFrontButton.SubModel)
				Train->ggPantFrontButton.UpdateValue(0.0, Train->dsbSwitch);
			// NOTE: currently we animate the selectable pantograph control based on standard key presses
			// TODO: implement actual selection control, and refactor handling this control setup in a separate method
			if (Train->ggPantSelectedButton.SubModel)
				Train->ggPantSelectedButton.UpdateValue(0.0, Train->dsbSwitch);
			// also the switch off button, in cabs which have it
			if (Train->ggPantFrontButtonOff.SubModel)
				Train->ggPantFrontButtonOff.UpdateValue(0.0, Train->dsbSwitch);
			if (Train->ggPantSelectedDownButton.SubModel) {
				// NOTE: currently we animate the selectable pantograph control based on standard key presses
				// TODO: implement actual selection control, and refactor handling this control setup in a separate method
				Train->ggPantSelectedDownButton.UpdateValue(0.0, Train->dsbSwitch);
			}
		}
	}
}

void TTrain::OnCommand_pantographtogglerear(TTrain* Train, command_data const& cmd)
{
	command_data Command = cmd;

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (false == Train->mvControlled->PantRearUp) {
			// turn on...
			OnCommand_pantographraiserear(Train, Command);
		} else {
			// ...or turn off
			OnCommand_pantographlowerrear(Train, Command);
		}
	} else if (Command.action == GLFW_RELEASE) {
		// impulse switches return automatically to neutral position
		// NOTE: this routine is used also by dedicated raise and lower commands
		if (Train->mvOccupied->PantSwitchType == "impulse") {
			if (Train->ggPantRearButton.SubModel)
				Train->ggPantRearButton.UpdateValue(0.0, Train->dsbSwitch);
			// NOTE: currently we animate the selectable pantograph control based on standard key presses
			// TODO: implement actual selection control, and refactor handling this control setup in a separate method
			if (Train->ggPantSelectedButton.SubModel)
				Train->ggPantSelectedButton.UpdateValue(0.0, Train->dsbSwitch);
			// also the switch off button, in cabs which have it
			if (Train->ggPantRearButtonOff.SubModel)
				Train->ggPantRearButtonOff.UpdateValue(0.0, Train->dsbSwitch);
			if (Train->ggPantSelectedDownButton.SubModel) {
				// NOTE: currently we animate the selectable pantograph control based on standard key presses
				// TODO: implement actual selection control, and refactor handling this control setup in a separate method
				Train->ggPantSelectedDownButton.UpdateValue(0.0, Train->dsbSwitch);
			}
		}
	}
}

void TTrain::OnCommand_pantographraisefront(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		// visual feedback
		if (Train->ggPantFrontButton.SubModel)
			Train->ggPantFrontButton.UpdateValue(1.0, Train->dsbSwitch);
		// NOTE: currently we animate the selectable pantograph control based on standard key presses
		// TODO: implement actual selection control, and refactor handling this control setup in a separate method
		if (Train->ggPantSelectedButton.SubModel)
			Train->ggPantSelectedButton.UpdateValue(1.0, Train->dsbSwitch);
		// pantograph control can have two-button setup
		if (Train->ggPantFrontButtonOff.SubModel)
			Train->ggPantFrontButtonOff.UpdateValue(0.0, Train->dsbSwitch);
		// NOTE: currently we animate the selectable pantograph control based on standard key presses
		// TODO: implement actual selection control, and refactor handling this control setup in a separate method
		if (Train->ggPantSelectedDownButton.SubModel)
			Train->ggPantSelectedDownButton.UpdateValue(0.0, Train->dsbSwitch);

		if (true == Train->mvControlled->PantFrontUp) {
			return;
		} // already up

		// TBD, TODO: impulse switch should only work when the power is on?
		if (Train->mvControlled->PantFront(true)) {
			Train->mvControlled->PantFrontSP = false;
		}
	} else if (Command.action == GLFW_RELEASE) {
		// visual feedback
		// NOTE: bit of a hax here, we're reusing button reset routine so we don't need a copy in every branch
		OnCommand_pantographtogglefront(Train, Command);
	}
}

void TTrain::OnCommand_pantographraiserear(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		// visual feedback
		if (Train->ggPantRearButton.SubModel)
			Train->ggPantRearButton.UpdateValue(1.0, Train->dsbSwitch);
		// NOTE: currently we animate the selectable pantograph control based on standard key presses
		// TODO: implement actual selection control, and refactor handling this control setup in a separate method
		if (Train->ggPantSelectedButton.SubModel)
			Train->ggPantSelectedButton.UpdateValue(1.0, Train->dsbSwitch);
		// pantograph control can have two-button setup
		if (Train->ggPantRearButtonOff.SubModel)
			Train->ggPantRearButtonOff.UpdateValue(0.0, Train->dsbSwitch);
		// NOTE: currently we animate the selectable pantograph control based on standard key presses
		// TODO: implement actual selection control, and refactor handling this control setup in a separate method
		if (Train->ggPantSelectedDownButton.SubModel)
			Train->ggPantSelectedDownButton.UpdateValue(0.0, Train->dsbSwitch);

		if (true == Train->mvControlled->PantRearUp) {
			return;
		} // already up

		// TBD, TODO: impulse switch should only work when the power is on?
		if (Train->mvControlled->PantRear(true)) {
			Train->mvControlled->PantRearSP = false;
		}
	} else if (Command.action == GLFW_RELEASE) {
		// visual feedback
		// NOTE: bit of a hax here, we're reusing button reset routine so we don't need a copy in every branch
		OnCommand_pantographtogglerear(Train, Command);
	}
}

void TTrain::OnCommand_pantographlowerfront(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		if (Train->mvOccupied->PantSwitchType == "impulse") {
			if ((Train->ggPantFrontButtonOff.SubModel == nullptr) && (Train->ggPantSelectedDownButton.SubModel == nullptr)) {
				// with impulse buttons we expect a dedicated switch to lower the pantograph, and if the cabin lacks it
				// then another control has to be used (like pantographlowerall)
				// TODO: we should have a way to define presence of cab controls without having to bind these to 3d submodels
				return;
			}
		}

		// visual feedback
		if (Train->ggPantFrontButton.SubModel)
			Train->ggPantFrontButton.UpdateValue(0.0, Train->dsbSwitch);
		// NOTE: currently we animate the selectable pantograph control based on standard key presses
		// TODO: implement actual selection control, and refactor handling this control setup in a separate method
		if (Train->ggPantSelectedButton.SubModel)
			Train->ggPantSelectedButton.UpdateValue(0.0, Train->dsbSwitch);
		// pantograph control can have two-button setup
		if (Train->ggPantFrontButtonOff.SubModel)
			Train->ggPantFrontButtonOff.UpdateValue(1.0, Train->dsbSwitch);
		// NOTE: currently we animate the selectable pantograph control based on standard key presses
		// TODO: implement actual selection control, and refactor handling this control setup in a separate method
		if (Train->ggPantSelectedDownButton.SubModel)
			Train->ggPantSelectedDownButton.UpdateValue(1.0, Train->dsbSwitch);

		if (false == Train->mvControlled->PantFrontUp) {
			return;
		} // already down

		// TBD, TODO: impulse switch should only work when the power is on?
		if (Train->mvControlled->PantFront(false)) {
			Train->mvControlled->PantFrontSP = false;
		}
	} else if (Command.action == GLFW_RELEASE) {
		// visual feedback
		// NOTE: bit of a hax here, we're reusing button reset routine so we don't need a copy in every branch
		OnCommand_pantographtogglefront(Train, Command);
	}
}

void TTrain::OnCommand_pantographlowerrear(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		if (Train->mvOccupied->PantSwitchType == "impulse") {
			if ((Train->ggPantRearButtonOff.SubModel == nullptr) && (Train->ggPantSelectedDownButton.SubModel == nullptr)) {
				// with impulse buttons we expect a dedicated switch to lower the pantograph, and if the cabin lacks it
				// then another control has to be used (like pantographlowerall)
				// TODO: we should have a way to define presence of cab controls without having to bind these to 3d submodels
				return;
			}
		}

		// visual feedback
		if (Train->ggPantRearButton.SubModel)
			Train->ggPantRearButton.UpdateValue(0.0, Train->dsbSwitch);
		// NOTE: currently we animate the selectable pantograph control based on standard key presses
		// TODO: implement actual selection control, and refactor handling this control setup in a separate method
		if (Train->ggPantSelectedButton.SubModel)
			Train->ggPantSelectedButton.UpdateValue(0.0, Train->dsbSwitch);
		// pantograph control can have two-button setup
		if (Train->ggPantRearButtonOff.SubModel)
			Train->ggPantRearButtonOff.UpdateValue(1.0, Train->dsbSwitch);
		// NOTE: currently we animate the selectable pantograph control based on standard key presses
		// TODO: implement actual selection control, and refactor handling this control setup in a separate method
		if (Train->ggPantSelectedDownButton.SubModel)
			Train->ggPantSelectedDownButton.UpdateValue(1.0, Train->dsbSwitch);

		if (false == Train->mvControlled->PantRearUp) {
			return;
		} // already down

		// TBD, TODO: impulse switch should only work when the power is on?
		if (Train->mvControlled->PantRear(false)) {
			Train->mvControlled->PantRearSP = false;
		}
	} else if (Command.action == GLFW_RELEASE) {
		// visual feedback
		// NOTE: bit of a hax here, we're reusing button reset routine so we don't need a copy in every branch
		OnCommand_pantographtogglerear(Train, Command);
	}
}

void TTrain::OnCommand_pantographlowerall(TTrain* Train, command_data const& Command)
{
	if ((Train->ggPantAllDownButton.SubModel == nullptr) && (Train->ggPantSelectedDownButton.SubModel == nullptr)) {
		// TODO: expand definition of cab controls so we can know if the control is present without testing for presence of 3d switch
		if (Command.action == GLFW_PRESS) {
			WriteLog("Lower All Pantographs switch is missing, or wasn't defined");
		}
		return;
	}
	if (Command.action == GLFW_PRESS) {
		// press the button
		// since we're just lowering all potential pantographs we don't need to test for state and effect
		// front...
		Train->mvControlled->PantFrontSP = false;
		Train->mvControlled->PantFront(false);
		// ...and rear
		Train->mvControlled->PantRearSP = false;
		Train->mvControlled->PantRear(false);
		// visual feedback
		if (Train->ggPantAllDownButton.SubModel)
			Train->ggPantAllDownButton.UpdateValue(1.0, Train->dsbSwitch);
		if (Train->ggPantSelectedDownButton.SubModel) {
			Train->ggPantSelectedDownButton.UpdateValue(1.0, Train->dsbSwitch);
		}
	} else if (Command.action == GLFW_RELEASE) {
		// release the button
		// visual feedback
		if (Train->ggPantAllDownButton.SubModel)
			Train->ggPantAllDownButton.UpdateValue(0.0);
		if (Train->ggPantSelectedDownButton.SubModel) {
			Train->ggPantSelectedDownButton.UpdateValue(0.0);
		}
	}
}

void TTrain::OnCommand_pantographcompressorvalvetoggle(TTrain* Train, command_data const& Command)
{
	if ((Train->mvControlled->TrainType == dt_EZT ? (Train->mvControlled != Train->mvOccupied) : (Train->mvOccupied->ActiveCab != 0))) {
		// tylko w maszynowym
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// only react to press
		if (Train->mvControlled->bPantKurek3 == false) {
			// connect pantographs with primary tank
			Train->mvControlled->bPantKurek3 = true;
			// visual feedback:
			Train->ggPantCompressorValve.UpdateValue(0.0);
		} else {
			// connect pantograps with pantograph compressor
			Train->mvControlled->bPantKurek3 = false;
			// visual feedback:
			Train->ggPantCompressorValve.UpdateValue(1.0);
		}
	}
}

void TTrain::OnCommand_pantographcompressoractivate(TTrain* Train, command_data const& Command)
{
	if ((Train->mvControlled->TrainType == dt_EZT ? (Train->mvControlled != Train->mvOccupied) : (Train->mvOccupied->ActiveCab != 0))) {
		// tylko w maszynowym
		return;
	}

	if ((Train->mvControlled->PantPress > 4.8) || (false == Train->mvControlled->Battery)) {
		// needs live power source and low enough pressure to work
		return;
	}

	if (Command.action != GLFW_RELEASE) {
		// press or hold to activate
		Train->mvControlled->PantCompFlag = true;
		// visual feedback:
		Train->ggPantCompressorButton.UpdateValue(1.0);
	} else {
		// release to disable
		Train->mvControlled->PantCompFlag = false;
		// visual feedback:
		Train->ggPantCompressorButton.UpdateValue(0.0);
	}
}

void TTrain::OnCommand_linebreakertoggle(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// press or hold...
		if (Train->m_linebreakerstate == 0) {
			// ...to close the circuit
			// NOTE: bit of a dirty shortcut here
			OnCommand_linebreakerclose(Train, Command);
		} else if (Train->m_linebreakerstate == 1) {
			// ...to open the circuit
			OnCommand_linebreakeropen(Train, Command);
		}
	} else if (Command.action == GLFW_RELEASE) {
		// release...
		if ((Train->ggMainOnButton.SubModel != nullptr) || (Train->mvControlled->TrainType == dt_EZT)) {
			// only impulse switches react to release events; since we don't have switch type definition for the line breaker,
			// we detect it from presence of relevant button, or presume such switch arrangement for EMUs
			if (Train->m_linebreakerstate == 0) {
				// ...after opening circuit, or holding for too short time to close it
				OnCommand_linebreakeropen(Train, Command);
			} else {
				// ...after closing the circuit
				// NOTE: bit of a dirty shortcut here
				OnCommand_linebreakerclose(Train, Command);
			}
		}
	}
}

void TTrain::OnCommand_linebreakeropen(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// visual feedback
		if (Train->ggMainOffButton.SubModel != nullptr) {
			// two separate switches to close and break the circuit
			Train->ggMainOffButton.UpdateValue(1.0, Train->dsbSwitch);
		} else if (Train->ggMainButton.SubModel != nullptr) {
			// single two-state switch
			// NOTE: we don't have switch type definition for the line breaker switch
			// so for the time being we have hard coded "impulse" switches for all EMUs
			// TODO: have proper switch type config for all switches, and put it in the cab switch descriptions, not in the .fiz
			if (Train->mvControlled->TrainType == dt_EZT) {
				Train->ggMainButton.UpdateValue(1.0, Train->dsbSwitch);
			} else {
				Train->ggMainButton.UpdateValue(0.0, Train->dsbSwitch);
			}
		} else {
			// fallback for cabs with no submodel
			Train->ggMainButton.UpdateValue(0.0, Train->dsbSwitch);
		}
		// play sound immediately when the switch is hit, not after release
		Train->fMainRelayTimer = 0.0f;

		if (Train->m_linebreakerstate == 0) {
			return;
		} // already in the desired state
		// NOTE: we don't have switch type definition for the line breaker switch
		// so for the time being we have hard coded "impulse" switches for all EMUs
		// TODO: have proper switch type config for all switches, and put it in the cab switch descriptions, not in the .fiz
		if (Train->mvControlled->TrainType == dt_EZT) {
			// a single impulse switch can't open the circuit, only close it
			return;
		}

		if (true == Train->mvControlled->MainSwitch(false)) {
			Train->m_linebreakerstate = 0;
		}
	} else if (Command.action == GLFW_RELEASE) {
		// visual feedback
		// we don't exactly know which of the two buttons was used, so reset both
		// for setup with two separate swiches
		if (Train->ggMainOnButton.SubModel != nullptr) {
			Train->ggMainOnButton.UpdateValue(0.0, Train->dsbSwitch);
		}
		if (Train->ggMainOffButton.SubModel != nullptr) {
			Train->ggMainOffButton.UpdateValue(0.0, Train->dsbSwitch);
		}
		// and the two-state switch too, for good measure
		if (Train->ggMainButton.SubModel != nullptr) {
			Train->ggMainButton.UpdateValue(0.0, Train->dsbSwitch);
		}
	}
}

void TTrain::OnCommand_linebreakerclose(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// visual feedback
		if (Train->ggMainOnButton.SubModel != nullptr) {
			// two separate switches to close and break the circuit
			Train->ggMainOnButton.UpdateValue(1.0, Train->dsbSwitch);
		} else if (Train->ggMainButton.SubModel != nullptr) {
			// single two-state switch
			Train->ggMainButton.UpdateValue(1.0, Train->dsbSwitch);
		} else {
			// fallback for cabs with no submodel
			Train->ggMainButton.UpdateValue(1.0, Train->dsbSwitch);
		}

		// the actual closing of the line breaker is handled in the train update routine
	} else if (Command.action == GLFW_RELEASE) {
		// visual feedback
		if (Train->ggMainOnButton.SubModel != nullptr) {
			// setup with two separate switches
			Train->ggMainOnButton.UpdateValue(0.0, Train->dsbSwitch);
		}
		// NOTE: we don't have switch type definition for the line breaker switch
		// so for the time being we have hard coded "impulse" switches for all EMUs
		// TODO: have proper switch type config for all switches, and put it in the cab switch descriptions, not in the .fiz
		if (Train->mvControlled->TrainType == dt_EZT) {
			if (Train->ggMainButton.SubModel != nullptr) {
				Train->ggMainButton.UpdateValue(0.0, Train->dsbSwitch);
			}
		}

		if (Train->m_linebreakerstate == 1) {
			return;
		} // already in the desired state

		if (Train->m_linebreakerstate == 2) {
			// we don't need to start the diesel twice, but the other types (with impulse switch setup) still need to be launched
			if ((Train->mvControlled->EngineType != DieselEngine) && (Train->mvControlled->EngineType != DieselElectric)) {
				// try to finalize state change of the line breaker, set the state based on the outcome
				Train->m_linebreakerstate = (Train->mvControlled->MainSwitch(true) ? 1 : 0);
			}
		}
		// on button release reset the closing timer
		Train->fMainRelayTimer = 0.0f;
	}
}

void TTrain::OnCommand_fuelpumptoggle(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (false == Train->mvControlled->FuelPump.is_enabled) {
			// turn on
			OnCommand_fuelpumpenable(Train, Command);
		} else {
			//turn off
			OnCommand_fuelpumpdisable(Train, Command);
		}
	}
}

void TTrain::OnCommand_fuelpumpenable(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// visual feedback
		Train->ggFuelPumpButton.UpdateValue(1.0, Train->dsbSwitch);

		if (true == Train->mvControlled->FuelPump.is_enabled) {
			return;
		} // already enabled

		Train->mvControlled->FuelPumpSwitch(true);
	}
}

void TTrain::OnCommand_fuelpumpdisable(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// visual feedback
		Train->ggFuelPumpButton.UpdateValue(0.0, Train->dsbSwitch);

		if (false == Train->mvControlled->FuelPump.is_enabled) {
			return;
		} // already disabled

		Train->mvControlled->FuelPumpSwitch(false);
	}
}

void TTrain::OnCommand_oilpumptoggle(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (false == Train->mvControlled->OilPump.is_enabled) {
			// turn on
			OnCommand_oilpumpenable(Train, Command);
		} else {
			//turn off
			OnCommand_oilpumpdisable(Train, Command);
		}
	}
}

void TTrain::OnCommand_oilpumpenable(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// visual feedback
		Train->ggOilPumpButton.UpdateValue(1.0, Train->dsbSwitch);

		if (true == Train->mvControlled->OilPump.is_enabled) {
			return;
		} // already enabled

		Train->mvControlled->OilPumpSwitch(true);
	}
}

void TTrain::OnCommand_oilpumpdisable(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// visual feedback
		Train->ggOilPumpButton.UpdateValue(0.0, Train->dsbSwitch);

		if (false == Train->mvControlled->OilPump.is_enabled) {
			return;
		} // already disabled

		Train->mvControlled->OilPumpSwitch(false);
	}
}

void TTrain::OnCommand_waterheaterbreakertoggle(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (false == Train->mvControlled->WaterHeater.breaker) {
			// turn on
			OnCommand_waterheaterbreakerclose(Train, Command);
		} else {
			//turn off
			OnCommand_waterheaterbreakeropen(Train, Command);
		}
	}
}

void TTrain::OnCommand_waterheaterbreakerclose(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// visual feedback
		Train->ggWaterHeaterBreakerButton.UpdateValue(1.0, Train->dsbSwitch);

		if (true == Train->mvControlled->WaterHeater.breaker) {
			return;
		} // already enabled

		Train->mvControlled->WaterHeaterBreakerSwitch(true);
	}
}

void TTrain::OnCommand_waterheaterbreakeropen(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// visual feedback
		Train->ggWaterHeaterBreakerButton.UpdateValue(0.0, Train->dsbSwitch);

		if (false == Train->mvControlled->WaterHeater.breaker) {
			return;
		} // already enabled

		Train->mvControlled->WaterHeaterBreakerSwitch(false);
	}
}

void TTrain::OnCommand_waterheatertoggle(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (false == Train->mvControlled->WaterHeater.is_enabled) {
			// turn on
			OnCommand_waterheaterenable(Train, Command);
		} else {
			//turn off
			OnCommand_waterheaterdisable(Train, Command);
		}
	}
}

void TTrain::OnCommand_waterheaterenable(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// visual feedback
		Train->ggWaterHeaterButton.UpdateValue(1.0, Train->dsbSwitch);

		if (true == Train->mvControlled->WaterHeater.is_enabled) {
			return;
		} // already enabled

		Train->mvControlled->WaterHeaterSwitch(true);
	}
}

void TTrain::OnCommand_waterheaterdisable(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// visual feedback
		Train->ggWaterHeaterButton.UpdateValue(0.0, Train->dsbSwitch);

		if (false == Train->mvControlled->WaterHeater.is_enabled) {
			return;
		} // already disabled

		Train->mvControlled->WaterHeaterSwitch(false);
	}
}

void TTrain::OnCommand_waterpumpbreakertoggle(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (false == Train->mvControlled->WaterPump.breaker) {
			// turn on
			OnCommand_waterpumpbreakerclose(Train, Command);
		} else {
			//turn off
			OnCommand_waterpumpbreakeropen(Train, Command);
		}
	}
}

void TTrain::OnCommand_waterpumpbreakerclose(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// visual feedback
		Train->ggWaterPumpBreakerButton.UpdateValue(1.0, Train->dsbSwitch);

		if (true == Train->mvControlled->WaterPump.breaker) {
			return;
		} // already enabled

		Train->mvControlled->WaterPumpBreakerSwitch(true);
	}
}

void TTrain::OnCommand_waterpumpbreakeropen(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// visual feedback
		Train->ggWaterPumpBreakerButton.UpdateValue(0.0, Train->dsbSwitch);

		if (false == Train->mvControlled->WaterPump.breaker) {
			return;
		} // already enabled

		Train->mvControlled->WaterPumpBreakerSwitch(false);
	}
}

void TTrain::OnCommand_waterpumptoggle(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (false == Train->mvControlled->WaterPump.is_enabled) {
			// turn on
			OnCommand_waterpumpenable(Train, Command);
		} else {
			//turn off
			OnCommand_waterpumpdisable(Train, Command);
		}
	}
}

void TTrain::OnCommand_waterpumpenable(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// visual feedback
		Train->ggWaterPumpButton.UpdateValue(1.0, Train->dsbSwitch);

		if (true == Train->mvControlled->WaterPump.is_enabled) {
			return;
		} // already enabled

		Train->mvControlled->WaterPumpSwitch(true);
	}
}

void TTrain::OnCommand_waterpumpdisable(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// visual feedback
		Train->ggWaterPumpButton.UpdateValue(0.0, Train->dsbSwitch);

		if (false == Train->mvControlled->WaterPump.is_enabled) {
			return;
		} // already disabled

		Train->mvControlled->WaterPumpSwitch(false);
	}
}

void TTrain::OnCommand_watercircuitslinktoggle(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (false == Train->mvControlled->WaterCircuitsLink) {
			// turn on
			OnCommand_watercircuitslinkenable(Train, Command);
		} else {
			//turn off
			OnCommand_watercircuitslinkdisable(Train, Command);
		}
	}
}

void TTrain::OnCommand_watercircuitslinkenable(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// visual feedback
		Train->ggWaterCircuitsLinkButton.UpdateValue(1.0, Train->dsbSwitch);

		if (true == Train->mvControlled->WaterCircuitsLink) {
			return;
		} // already enabled

		Train->mvControlled->WaterCircuitsLinkSwitch(true);
	}
}

void TTrain::OnCommand_watercircuitslinkdisable(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// visual feedback
		Train->ggWaterCircuitsLinkButton.UpdateValue(0.0, Train->dsbSwitch);

		if (false == Train->mvControlled->WaterCircuitsLink) {
			return;
		} // already disabled

		Train->mvControlled->WaterCircuitsLinkSwitch(false);
	}
}

void TTrain::OnCommand_convertertoggle(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if ((false == Train->mvControlled->ConverterAllow) && (Train->ggConverterButton.GetValue() < 0.5)) {
			// turn on
			OnCommand_converterenable(Train, Command);
		} else {
			//turn off
			OnCommand_converterdisable(Train, Command);
		}
	} else if (Command.action == GLFW_RELEASE) {
		// on button release...
		if (Train->mvOccupied->ConvSwitchType == "impulse") {
			// ...return switches to start position if applicable
			Train->ggConverterButton.UpdateValue(0.0, Train->dsbSwitch);
			Train->ggConverterOffButton.UpdateValue(0.0, Train->dsbSwitch);
		}
	}
}

void TTrain::OnCommand_converterenable(TTrain* Train, command_data const& Command)
{
	if (Train->mvControlled->ConverterStart == start::automatic) {
		// let the automatic thing do its automatic thing...
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// visual feedback
		Train->ggConverterButton.UpdateValue(1.0, Train->dsbSwitch);

		if (true == Train->mvControlled->ConverterAllow) {
			return;
		} // already enabled

		// impulse type switch has no effect if there's no power
		// NOTE: this is most likely setup wrong, but the whole thing is smoke and mirrors anyway
		if ((Train->mvOccupied->ConvSwitchType != "impulse") || (Train->mvControlled->Mains)) {
			// won't start if the line breaker button is still held
			if (true == Train->mvControlled->ConverterSwitch(true)) {
				// side effects
				// control the compressor, if it's paired with the converter
				if (Train->mvControlled->CompressorPower == 2) {
					// hunter-091012: tak jest poprawnie
					Train->mvControlled->CompressorSwitch(true);
				}
			}
		}
	} else if (Command.action == GLFW_RELEASE) {
		// potentially reset impulse switch position, using shared code branch
		OnCommand_convertertoggle(Train, Command);
	}
}

void TTrain::OnCommand_converterdisable(TTrain* Train, command_data const& Command)
{
	if (Train->mvControlled->ConverterStart == start::automatic) {
		// let the automatic thing do its automatic thing...
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// visual feedback
		Train->ggConverterButton.UpdateValue(0.0, Train->dsbSwitch);
		if (Train->ggConverterOffButton.SubModel != nullptr) {
			Train->ggConverterOffButton.UpdateValue(1.0, Train->dsbSwitch);
		}

		if (false == Train->mvControlled->ConverterAllow) {
			return;
		} // already disabled

		if (true == Train->mvControlled->ConverterSwitch(false)) {
			// side effects
			// control the compressor, if it's paired with the converter
			if (Train->mvControlled->CompressorPower == 2) {
				// hunter-091012: tak jest poprawnie
				Train->mvControlled->CompressorSwitch(false);
			}
			if ((Train->mvControlled->TrainType == dt_EZT) && (false == TestFlag(Train->mvControlled->EngDmgFlag, 4))) {
				Train->mvControlled->ConvOvldFlag = false;
			}
			// if there's no (low voltage) power source left, drop pantographs
			if (false == Train->mvControlled->Battery) {
				Train->mvControlled->PantFront(false);
				Train->mvControlled->PantRear(false);
			}
		}
	} else if (Command.action == GLFW_RELEASE) {
		// potentially reset impulse switch position, using shared code branch
		OnCommand_convertertoggle(Train, Command);
	}
}

void TTrain::OnCommand_convertertogglelocal(TTrain* Train, command_data const& Command)
{
	if (Train->mvOccupied->ConverterStart == start::automatic) {
		// let the automatic thing do its automatic thing...
		return;
	}
	if (Train->ggConverterLocalButton.SubModel == nullptr) {
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if ((false == Train->mvOccupied->ConverterAllowLocal) && (Train->ggConverterLocalButton.GetValue() < 0.5)) {
			// turn on
			// visual feedback
			Train->ggConverterLocalButton.UpdateValue(1.0, Train->dsbSwitch);
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
		} else {
			//turn off
			// visual feedback
			Train->ggConverterLocalButton.UpdateValue(0.0, Train->dsbSwitch);
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

void TTrain::OnCommand_converteroverloadrelayreset(TTrain* Train, command_data const& Command)
{
	if (Train->ggConverterFuseButton.SubModel == nullptr) {
		if (Command.action == GLFW_PRESS) {
			WriteLog("Converter Overload Relay Reset button is missing, or wasn't defined");
		}
		//        return;
	}

	if (Command.action == GLFW_PRESS) {
		// visual feedback
		Train->ggConverterFuseButton.UpdateValue(1.0, Train->dsbSwitch);

		if ((Train->mvControlled->Mains == false) && (Train->ggConverterButton.GetValue() < 0.05) && (Train->mvControlled->TrainType != dt_EZT)) {
			Train->mvControlled->ConvOvldFlag = false;
		}
	} else if (Command.action == GLFW_RELEASE) {
		// visual feedback
		Train->ggConverterFuseButton.UpdateValue(0.0, Train->dsbSwitch);
	}
}

void TTrain::OnCommand_compressortoggle(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (false == Train->mvControlled->CompressorAllow) {
			// turn on
			OnCommand_compressorenable(Train, Command);
		} else {
			//turn off
			OnCommand_compressordisable(Train, Command);
		}
	}
	/*
    // disabled because we don't have yet support for compressor switch type definition
    else if( Command.action == GLFW_RELEASE ) {
        // on button release...
        if( Train->mvOccupied->CompSwitchType == "impulse" ) {
            // ...return switches to start position if applicable
            Train->ggCompressorButton.UpdateValue( 0.0, Train->dsbSwitch );
            Train->ggCompressorOffButton.UpdateValue( 0.0, Train->dsbSwitch );
        }
    }
*/
}

void TTrain::OnCommand_compressorenable(TTrain* Train, command_data const& Command)
{
	if (Train->mvControlled->CompressorPower >= 2) {
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// visual feedback
		Train->ggCompressorButton.UpdateValue(1.0, Train->dsbSwitch);

		if (true == Train->mvControlled->CompressorAllow) {
			return;
		} // already enabled

		// impulse type switch has no effect if there's no power
		// NOTE: this is most likely setup wrong, but the whole thing is smoke and mirrors anyway
		//        if( ( Train->mvOccupied->CompSwitchType != "impulse" )
		//         || ( Train->mvControlled->Mains ) ) {

		Train->mvControlled->CompressorSwitch(true);
		//        }
	} else if (Command.action == GLFW_RELEASE) {
		// potentially reset impulse switch position, using shared code branch
		OnCommand_compressortoggle(Train, Command);
	}
}

void TTrain::OnCommand_compressordisable(TTrain* Train, command_data const& Command)
{
	if (Train->mvControlled->CompressorPower >= 2) {
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// visual feedback
		Train->ggCompressorButton.UpdateValue(0.0, Train->dsbSwitch);
		/*
        if( Train->ggCompressorOffButton.SubModel != nullptr ) {
            Train->ggCompressorOffButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
*/
		if (false == Train->mvControlled->CompressorAllow) {
			return;
		} // already disabled

		Train->mvControlled->CompressorSwitch(false);
	} else if (Command.action == GLFW_RELEASE) {
		// potentially reset impulse switch position, using shared code branch
		OnCommand_compressortoggle(Train, Command);
	}
}

void TTrain::OnCommand_compressortogglelocal(TTrain* Train, command_data const& Command)
{
	if (Train->mvOccupied->CompressorPower >= 2) {
		return;
	}
	if (Train->ggCompressorLocalButton.SubModel == nullptr) {
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (false == Train->mvOccupied->CompressorAllowLocal) {
			// turn on
			// visual feedback
			Train->ggCompressorLocalButton.UpdateValue(1.0, Train->dsbSwitch);
			// effect
			Train->mvOccupied->CompressorAllowLocal = true;
		} else {
			// turn off
			// visual feedback
			Train->ggCompressorLocalButton.UpdateValue(0.0, Train->dsbSwitch);
			// effect
			Train->mvOccupied->CompressorAllowLocal = false;
		}
	}
}

void TTrain::OnCommand_motorconnectorsopen(TTrain* Train, command_data const& cmd)
{
	command_data Command = cmd;
	// TODO: don't rely on presense of 3d model to determine presence of the switch
	if (Train->ggStLinOffButton.SubModel == nullptr) {
		if (Command.action == GLFW_PRESS) {
			WriteLog("Open Motor Power Connectors button is missing, or wasn't defined");
		}
		return;
	}
	// HACK: because we don't have modeled actual circuits this is a simplification of the real mechanics
	// namely, pressing the button will flip it in the entire unit, which isn't exactly physically possible
	if (Command.action == GLFW_PRESS) {
		// button works while it's held down but we can ignore repeats
		if (false == Train->mvControlled->StLinSwitchOff) {
			// open the connectors
			// visual feedback
			Train->ggStLinOffButton.UpdateValue(1.0, Train->dsbSwitch);

			Train->mvControlled->StLinSwitchOff = true;
			Train->set_paired_open_motor_connectors_button(true);
		} else {
			// potentially close the connectors
			OnCommand_motorconnectorsclose(Train, Command);
		}
	} else if (Command.action == GLFW_RELEASE) {
		// button released
		if (Train->mvControlled->StLinSwitchType != "toggle") {
			// default button type (impulse) ceases its work on button release
			// visual feedback
			Train->ggStLinOffButton.UpdateValue(0.0, Train->dsbSwitch);

			Train->mvControlled->StLinSwitchOff = false;
			Train->set_paired_open_motor_connectors_button(false);
		}
	}
}

void TTrain::OnCommand_motorconnectorsclose(TTrain* Train, command_data const& Command)
{
	// TODO: don't rely on presense of 3d model to determine presence of the switch
	if (Train->ggStLinOffButton.SubModel == nullptr) {
		if (Command.action == GLFW_PRESS) {
			WriteLog("Open Motor Power Connectors button is missing, or wasn't defined");
		}
		return;
	}

	if (Command.action == GLFW_PRESS) {
		if (Train->mvControlled->StLinSwitchType == "toggle") {
			// default type of button (impulse) has only one effect on press, but the toggle type can toggle the state
			// visual feedback
			Train->ggStLinOffButton.UpdateValue(0.0, Train->dsbSwitch);
		}

		if (false == Train->mvControlled->StLinSwitchOff) {
			return;
		} // already closed

		Train->mvControlled->StLinSwitchOff = false;
		Train->set_paired_open_motor_connectors_button(false);
	}
}

void TTrain::OnCommand_motordisconnect(TTrain* Train, command_data const& Command)
{
	if ((Train->mvControlled->TrainType == dt_EZT ? (Train->mvControlled != Train->mvOccupied) : (Train->mvOccupied->ActiveCab != 0))) {
		// tylko w maszynowym
		return;
	}

	if (Command.action == GLFW_PRESS) {
		Train->mvControlled->CutOffEngine();
	}
}

void TTrain::OnCommand_motoroverloadrelaythresholdtoggle(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if ((true == Train->mvControlled->ShuntModeAllow ? (false == Train->mvControlled->ShuntMode) : (Train->mvControlled->Imax < Train->mvControlled->ImaxHi))) {
			// turn on
			OnCommand_motoroverloadrelaythresholdsethigh(Train, Command);
		} else {
			//turn off
			OnCommand_motoroverloadrelaythresholdsetlow(Train, Command);
		}
	}
}

void TTrain::OnCommand_motoroverloadrelaythresholdsetlow(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (true == Train->mvControlled->CurrentSwitch(false)) {
			// visual feedback
			Train->ggMaxCurrentCtrl.UpdateValue(0.0, Train->dsbSwitch);
		}
	}
}

void TTrain::OnCommand_motoroverloadrelaythresholdsethigh(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (true == Train->mvControlled->CurrentSwitch(true)) {
			// visual feedback
			Train->ggMaxCurrentCtrl.UpdateValue(1.0, Train->dsbSwitch);
		}
	}
}

void TTrain::OnCommand_motoroverloadrelayreset(TTrain* Train, command_data const& Command)
{
	if (Train->ggFuseButton.SubModel == nullptr) {
		if (Command.action == GLFW_PRESS) {
			WriteLog("Motor Overload Relay Reset button is missing, or wasn't defined");
		}
		//        return;
	}

	if (Command.action == GLFW_PRESS) {
		// visual feedback
		Train->ggFuseButton.UpdateValue(1.0, Train->dsbSwitch);

		Train->mvControlled->FuseOn();
	} else if (Command.action == GLFW_RELEASE) {
		// visual feedback
		Train->ggFuseButton.UpdateValue(0.0, Train->dsbSwitch);
	}
}

void TTrain::OnCommand_lightspresetactivatenext(TTrain* Train, command_data const& Command)
{
	if (Train->mvOccupied->LightsPosNo == 0) {
		// lights are controlled by preset selector
		return;
	}
	if (Command.action != GLFW_PRESS) {
		// one change per key press
		return;
	}

	if ((Train->mvOccupied->LightsPos < Train->mvOccupied->LightsPosNo) || (true == Train->mvOccupied->LightsWrap)) {
		// active light preset is stored as value in range 1-LigthPosNo
		Train->mvOccupied->LightsPos = (Train->mvOccupied->LightsPos < Train->mvOccupied->LightsPosNo ? Train->mvOccupied->LightsPos + 1 : 1); // wrap mode

		Train->SetLights();
		// visual feedback
		if (Train->ggLightsButton.SubModel != nullptr) {
			Train->ggLightsButton.UpdateValue(Train->mvOccupied->LightsPos - 1, Train->dsbSwitch);
		}
	}
}

void TTrain::OnCommand_lightspresetactivateprevious(TTrain* Train, command_data const& Command)
{
	if (Train->mvOccupied->LightsPosNo == 0) {
		// lights are controlled by preset selector
		return;
	}
	if (Command.action != GLFW_PRESS) {
		// one change per key press
		return;
	}

	if ((Train->mvOccupied->LightsPos > 1) || (true == Train->mvOccupied->LightsWrap)) {
		// active light preset is stored as value in range 1-LigthPosNo
		Train->mvOccupied->LightsPos = (Train->mvOccupied->LightsPos > 1 ? Train->mvOccupied->LightsPos - 1 : Train->mvOccupied->LightsPosNo); // wrap mode

		Train->SetLights();
		// visual feedback
		if (Train->ggLightsButton.SubModel != nullptr) {
			Train->ggLightsButton.UpdateValue(Train->mvOccupied->LightsPos - 1, Train->dsbSwitch);
		}
	}
}

void TTrain::OnCommand_headlighttoggleleft(TTrain* Train, command_data const& Command)
{
	int const vehicleside = (Train->mvOccupied->ActiveCab == 1 ? side::front : side::rear);

	bool current_state = (bool)(Train->DynamicObject->iLights[vehicleside] & light::headlight_left);

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (!current_state) {
			// turn on
			OnCommand_headlightenableleft(Train, Command);
		} else {
			//turn off
			OnCommand_headlightdisableleft(Train, Command);
		}
	}
}

void TTrain::OnCommand_headlightenableleft(TTrain* Train, command_data const& Command)
{
	if (Train->mvOccupied->LightsPosNo > 0) {
		// lights are controlled by preset selector
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		int const vehicleside = (Train->mvOccupied->ActiveCab == 1 ? side::front : side::rear);

		if ((Train->DynamicObject->iLights[vehicleside] & light::headlight_left) != 0) {
			return;
		} // already enabled

		Train->DynamicObject->iLights[vehicleside] ^= light::headlight_left;
		// visual feedback
		Train->ggLeftLightButton.UpdateValue(1.0, Train->dsbSwitch);
		// if the light is controlled by 3-way switch, disable marker light
		if (Train->ggLeftEndLightButton.SubModel == nullptr) {
			Train->DynamicObject->iLights[vehicleside] &= ~light::redmarker_left;
		}
	}
}

void TTrain::OnCommand_headlightdisableleft(TTrain* Train, command_data const& Command)
{
	if (Train->mvOccupied->LightsPosNo > 0) {
		// lights are controlled by preset selector
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		int const vehicleside = (Train->mvOccupied->ActiveCab == 1 ? side::front : side::rear);

		if ((Train->DynamicObject->iLights[vehicleside] & light::headlight_left) == 0) {
			return;
		} // already disabled

		Train->DynamicObject->iLights[vehicleside] ^= light::headlight_left;
		// visual feedback
		Train->ggLeftLightButton.UpdateValue(0.0, Train->dsbSwitch);
	}
}

void TTrain::OnCommand_headlighttoggleright(TTrain* Train, command_data const& Command)
{
	int const vehicleside = (Train->mvOccupied->ActiveCab == 1 ? side::front : side::rear);

	bool current_state = (bool)(Train->DynamicObject->iLights[vehicleside] & light::headlight_right);

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (!current_state) {
			// turn on
			OnCommand_headlightenableright(Train, Command);
		} else {
			//turn off
			OnCommand_headlightdisableright(Train, Command);
		}
	}
}

void TTrain::OnCommand_headlightenableright(TTrain* Train, command_data const& Command)
{
	if (Train->mvOccupied->LightsPosNo > 0) {
		// lights are controlled by preset selector
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		int const vehicleside = (Train->mvOccupied->ActiveCab == 1 ? side::front : side::rear);

		if ((Train->DynamicObject->iLights[vehicleside] & light::headlight_right) != 0) {
			return;
		} // already enabled

		Train->DynamicObject->iLights[vehicleside] ^= light::headlight_right;
		// visual feedback
		Train->ggRightLightButton.UpdateValue(1.0, Train->dsbSwitch);
		// if the light is controlled by 3-way switch, disable marker light
		if (Train->ggRightEndLightButton.SubModel == nullptr) {
			Train->DynamicObject->iLights[vehicleside] &= ~light::redmarker_right;
		}
	}
}

void TTrain::OnCommand_headlightdisableright(TTrain* Train, command_data const& Command)
{
	if (Train->mvOccupied->LightsPosNo > 0) {
		// lights are controlled by preset selector
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		int const vehicleside = (Train->mvOccupied->ActiveCab == 1 ? side::front : side::rear);

		if ((Train->DynamicObject->iLights[vehicleside] & light::headlight_right) == 0) {
			return;
		} // already disabled

		Train->DynamicObject->iLights[vehicleside] ^= light::headlight_right;
		// visual feedback
		Train->ggRightLightButton.UpdateValue(0.0, Train->dsbSwitch);
	}
}

void TTrain::OnCommand_headlighttoggleupper(TTrain* Train, command_data const& Command)
{
	int const vehicleside = (Train->mvOccupied->ActiveCab == 1 ? side::front : side::rear);

	bool current_state = (bool)(Train->DynamicObject->iLights[vehicleside] & light::headlight_upper);

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (!current_state) {
			// turn on
			OnCommand_headlightenableupper(Train, Command);
		} else {
			//turn off
			OnCommand_headlightdisableupper(Train, Command);
		}
	}
}

void TTrain::OnCommand_headlightenableupper(TTrain* Train, command_data const& Command)
{
	if (Train->mvOccupied->LightsPosNo > 0) {
		// lights are controlled by preset selector
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		int const vehicleside = (Train->mvOccupied->ActiveCab == 1 ? side::front : side::rear);

		if ((Train->DynamicObject->iLights[vehicleside] & light::headlight_upper) != 0) {
			return;
		} // already enabled

		Train->DynamicObject->iLights[vehicleside] ^= light::headlight_upper;
		// visual feedback
		Train->ggUpperLightButton.UpdateValue(1.0, Train->dsbSwitch);
	}
}

void TTrain::OnCommand_headlightdisableupper(TTrain* Train, command_data const& Command)
{
	if (Train->mvOccupied->LightsPosNo > 0) {
		// lights are controlled by preset selector
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		int const vehicleside = (Train->mvOccupied->ActiveCab == 1 ? side::front : side::rear);

		if ((Train->DynamicObject->iLights[vehicleside] & light::headlight_upper) == 0) {
			return;
		} // already disabled

		Train->DynamicObject->iLights[vehicleside] ^= light::headlight_upper;
		// visual feedback
		Train->ggUpperLightButton.UpdateValue(0.0, Train->dsbSwitch);
	}
}

void TTrain::OnCommand_redmarkertoggleleft(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		int const vehicleside = (Train->mvOccupied->ActiveCab == 1 ? side::front : side::rear);

		if ((Train->DynamicObject->iLights[vehicleside] & light::redmarker_left) == 0) {
			// turn on
			OnCommand_redmarkerenableleft(Train, Command);
		} else {
			//turn off
			OnCommand_redmarkerdisableleft(Train, Command);
		}
	}
}

void TTrain::OnCommand_redmarkerenableleft(TTrain* Train, command_data const& Command)
{
	if (Train->mvOccupied->LightsPosNo > 0) {
		// lights are controlled by preset selector
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		int const vehicleside = (Train->mvOccupied->ActiveCab == 1 ? side::front : side::rear);

		if ((Train->DynamicObject->iLights[vehicleside] & light::redmarker_left) != 0) {
			return;
		} // already enabled

		Train->DynamicObject->iLights[vehicleside] ^= light::redmarker_left;
		// visual feedback
		if (Train->ggLeftEndLightButton.SubModel != nullptr) {
			Train->ggLeftEndLightButton.UpdateValue(1.0, Train->dsbSwitch);
		} else {
			// we interpret lack of dedicated switch as a sign the light is controlled with 3-way switch
			// this is crude, but for now will do
			Train->ggLeftLightButton.UpdateValue(-1.0, Train->dsbSwitch);
			// if the light is controlled by 3-way switch, disable the headlight
			Train->DynamicObject->iLights[vehicleside] &= ~light::headlight_left;
		}
	}
}

void TTrain::OnCommand_redmarkerdisableleft(TTrain* Train, command_data const& Command)
{
	if (Train->mvOccupied->LightsPosNo > 0) {
		// lights are controlled by preset selector
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		int const vehicleside = (Train->mvOccupied->ActiveCab == 1 ? side::front : side::rear);

		if ((Train->DynamicObject->iLights[vehicleside] & light::redmarker_left) == 0) {
			return;
		} // already disabled

		Train->DynamicObject->iLights[vehicleside] ^= light::redmarker_left;
		// visual feedback
		if (Train->ggLeftEndLightButton.SubModel != nullptr) {
			Train->ggLeftEndLightButton.UpdateValue(0.0, Train->dsbSwitch);
		} else {
			// we interpret lack of dedicated switch as a sign the light is controlled with 3-way switch
			// this is crude, but for now will do
			Train->ggLeftLightButton.UpdateValue(0.0, Train->dsbSwitch);
		}
	}
}

void TTrain::OnCommand_redmarkertoggleright(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		int const vehicleside = (Train->mvOccupied->ActiveCab == 1 ? side::front : side::rear);

		if ((Train->DynamicObject->iLights[vehicleside] & light::redmarker_right) == 0) {
			// turn on
			OnCommand_redmarkerenableright(Train, Command);
		} else {
			//turn off
			OnCommand_redmarkerdisableright(Train, Command);
		}
	}
}

void TTrain::OnCommand_redmarkerenableright(TTrain* Train, command_data const& Command)
{
	if (Train->mvOccupied->LightsPosNo > 0) {
		// lights are controlled by preset selector
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		int const vehicleside = (Train->mvOccupied->ActiveCab == 1 ? side::front : side::rear);

		if ((Train->DynamicObject->iLights[vehicleside] & light::redmarker_right) != 0) {
			return;
		} // already enabled

		Train->DynamicObject->iLights[vehicleside] ^= light::redmarker_right;
		// visual feedback
		if (Train->ggRightEndLightButton.SubModel != nullptr) {
			Train->ggRightEndLightButton.UpdateValue(1.0, Train->dsbSwitch);
		} else {
			// we interpret lack of dedicated switch as a sign the light is controlled with 3-way switch
			// this is crude, but for now will do
			Train->ggRightLightButton.UpdateValue(-1.0, Train->dsbSwitch);
			// if the light is controlled by 3-way switch, disable the headlight
			Train->DynamicObject->iLights[vehicleside] &= ~light::headlight_right;
		}
	}
}

void TTrain::OnCommand_redmarkerdisableright(TTrain* Train, command_data const& Command)
{
	if (Train->mvOccupied->LightsPosNo > 0) {
		// lights are controlled by preset selector
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		int const vehicleside = (Train->mvOccupied->ActiveCab == 1 ? side::front : side::rear);

		if ((Train->DynamicObject->iLights[vehicleside] & light::redmarker_right) == 0) {
			return;
		} // already disabled

		Train->DynamicObject->iLights[vehicleside] ^= light::redmarker_right;
		// visual feedback
		if (Train->ggRightEndLightButton.SubModel != nullptr) {
			Train->ggRightEndLightButton.UpdateValue(0.0, Train->dsbSwitch);
		} else {
			// we interpret lack of dedicated switch as a sign the light is controlled with 3-way switch
			// this is crude, but for now will do
			Train->ggRightLightButton.UpdateValue(0.0, Train->dsbSwitch);
		}
	}
}

void TTrain::OnCommand_headlighttogglerearleft(TTrain* Train, command_data const& Command)
{
	if (Train->mvOccupied->LightsPosNo > 0) {
		// lights are controlled by preset selector
		return;
	}

	int const vehicleside = (Train->mvOccupied->ActiveCab == 1 ? side::rear : side::front);

	if (Command.action == GLFW_PRESS) {
		// NOTE: we toggle the light on opposite side, as 'rear right' is 'front left' on the rear end etc
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if ((Train->DynamicObject->iLights[vehicleside] & light::headlight_right) == 0) {
			// turn on
			Train->DynamicObject->iLights[vehicleside] ^= light::headlight_right;
			// visual feedback
			Train->ggRearLeftLightButton.UpdateValue(1.0, Train->dsbSwitch);
		} else {
			//turn off
			Train->DynamicObject->iLights[vehicleside] ^= light::headlight_right;
			// visual feedback
			Train->ggRearLeftLightButton.UpdateValue(0.0, Train->dsbSwitch);
		}
	}
}

void TTrain::OnCommand_headlighttogglerearright(TTrain* Train, command_data const& Command)
{
	if (Train->mvOccupied->LightsPosNo > 0) {
		// lights are controlled by preset selector
		return;
	}

	int const vehicleside = (Train->mvOccupied->ActiveCab == 1 ? side::rear : side::front);

	if (Command.action == GLFW_PRESS) {
		// NOTE: we toggle the light on opposite side, as 'rear right' is 'front left' on the rear end etc
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if ((Train->DynamicObject->iLights[vehicleside] & light::headlight_left) == 0) {
			// turn on
			Train->DynamicObject->iLights[vehicleside] ^= light::headlight_left;
			// visual feedback
			Train->ggRearRightLightButton.UpdateValue(1.0, Train->dsbSwitch);
		} else {
			//turn off
			Train->DynamicObject->iLights[vehicleside] ^= light::headlight_left;
			// visual feedback
			Train->ggRearRightLightButton.UpdateValue(0.0, Train->dsbSwitch);
		}
	}
}

void TTrain::OnCommand_headlighttogglerearupper(TTrain* Train, command_data const& Command)
{
	if (Train->mvOccupied->LightsPosNo > 0) {
		// lights are controlled by preset selector
		return;
	}

	int const vehicleside = (Train->mvOccupied->ActiveCab == 1 ? side::rear : side::front);

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if ((Train->DynamicObject->iLights[vehicleside] & light::headlight_upper) == 0) {
			// turn on
			Train->DynamicObject->iLights[vehicleside] ^= light::headlight_upper;
			// visual feedback
			Train->ggRearUpperLightButton.UpdateValue(1.0, Train->dsbSwitch);
		} else {
			//turn off
			Train->DynamicObject->iLights[vehicleside] ^= light::headlight_upper;
			// visual feedback
			Train->ggRearUpperLightButton.UpdateValue(0.0, Train->dsbSwitch);
		}
	}
}

void TTrain::OnCommand_redmarkertogglerearleft(TTrain* Train, command_data const& Command)
{
	if (Train->mvOccupied->LightsPosNo > 0) {
		// lights are controlled by preset selector
		return;
	}

	int const vehicleside = (Train->mvOccupied->ActiveCab == 1 ? side::rear : side::front);

	if (Command.action == GLFW_PRESS) {
		// NOTE: we toggle the light on opposite side, as 'rear right' is 'front left' on the rear end etc
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if ((Train->DynamicObject->iLights[vehicleside] & light::redmarker_right) == 0) {
			// turn on
			Train->DynamicObject->iLights[vehicleside] ^= light::redmarker_right;
			// visual feedback
			Train->ggRearLeftEndLightButton.UpdateValue(1.0, Train->dsbSwitch);
		} else {
			//turn off
			Train->DynamicObject->iLights[vehicleside] ^= light::redmarker_right;
			// visual feedback
			Train->ggRearLeftEndLightButton.UpdateValue(0.0, Train->dsbSwitch);
		}
	}
}

void TTrain::OnCommand_redmarkertogglerearright(TTrain* Train, command_data const& Command)
{
	if (Train->mvOccupied->LightsPosNo > 0) {
		// lights are controlled by preset selector
		return;
	}

	int const vehicleside = (Train->mvOccupied->ActiveCab == 1 ? side::rear : side::front);

	if (Command.action == GLFW_PRESS) {
		// NOTE: we toggle the light on opposite side, as 'rear right' is 'front left' on the rear end etc
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if ((Train->DynamicObject->iLights[vehicleside] & light::redmarker_left) == 0) {
			// turn on
			Train->DynamicObject->iLights[vehicleside] ^= light::redmarker_left;
			// visual feedback
			Train->ggRearRightEndLightButton.UpdateValue(1.0, Train->dsbSwitch);
		} else {
			//turn off
			Train->DynamicObject->iLights[vehicleside] ^= light::redmarker_left;
			// visual feedback
			Train->ggRearRightEndLightButton.UpdateValue(0.0, Train->dsbSwitch);
		}
	}
}

void TTrain::OnCommand_redmarkerstoggle(TTrain* Train, command_data const& Command)
{
	if ((true == FreeFlyModeFlag) && (Command.action == GLFW_PRESS)) {
		auto* vehicle{ std::get<TDynamicObject*>(simulation::Region->find_vehicle(Global.pCameraPosition, 10, false, true)) };

		if (vehicle == nullptr) {
			return;
		}

		int const CouplNr{
			clamp(vehicle->DirectionGet() * (LengthSquared3(vehicle->HeadPosition() - Global.pCameraPosition) > LengthSquared3(vehicle->RearPosition() - Global.pCameraPosition) ? 1 : -1), 0, 1)
		}; // z [-1,1] zrobić [0,1]

		auto const lightset{ light::redmarker_left | light::redmarker_right };

		vehicle->iLights[CouplNr] = (false == TestFlag(vehicle->iLights[CouplNr], lightset) ? vehicle->iLights[CouplNr] |= lightset : // turn signals on
		                             vehicle->iLights[CouplNr] ^= lightset); // turn signals off
	}
}

void TTrain::OnCommand_endsignalstoggle(TTrain* Train, command_data const& Command)
{
	if ((true == FreeFlyModeFlag) && (Command.action == GLFW_PRESS)) {
		auto* vehicle{ std::get<TDynamicObject*>(simulation::Region->find_vehicle(Global.pCameraPosition, 10, false, true)) };

		if (vehicle == nullptr) {
			return;
		}

		int const CouplNr{
			clamp(vehicle->DirectionGet() * (LengthSquared3(vehicle->HeadPosition() - Global.pCameraPosition) > LengthSquared3(vehicle->RearPosition() - Global.pCameraPosition) ? 1 : -1), 0, 1)
		}; // z [-1,1] zrobić [0,1]

		auto const lightset{ light::rearendsignals };

		vehicle->iLights[CouplNr] = (false == TestFlag(vehicle->iLights[CouplNr], lightset) ? vehicle->iLights[CouplNr] |= lightset : // turn signals on
		                             vehicle->iLights[CouplNr] ^= lightset); // turn signals off
	}
}

void TTrain::OnCommand_headlightsdimtoggle(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (false == Train->DynamicObject->DimHeadlights) {
			// turn on
			OnCommand_headlightsdimenable(Train, Command);
		} else {
			//turn off
			OnCommand_headlightsdimdisable(Train, Command);
		}
	}
}

void TTrain::OnCommand_headlightsdimenable(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (Train->ggDimHeadlightsButton.SubModel == nullptr) {
			// TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
			WriteLog("Dim Headlights switch is missing, or wasn't defined");
			return;
		}
		// visual feedback
		Train->ggDimHeadlightsButton.UpdateValue(1.0, Train->dsbSwitch);

		if (true == Train->DynamicObject->DimHeadlights) {
			return;
		} // already enabled

		Train->DynamicObject->DimHeadlights = true;
	}
}

void TTrain::OnCommand_headlightsdimdisable(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (Train->ggDimHeadlightsButton.SubModel == nullptr) {
			// TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
			WriteLog("Dim Headlights switch is missing, or wasn't defined");
			return;
		}
		// visual feedback
		Train->ggDimHeadlightsButton.UpdateValue(0.0, Train->dsbSwitch);

		if (false == Train->DynamicObject->DimHeadlights) {
			return;
		} // already enabled

		Train->DynamicObject->DimHeadlights = false;
	}
}

void TTrain::OnCommand_interiorlighttoggle(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (false == Train->bCabLight) {
			// turn on
			OnCommand_interiorlightenable(Train, Command);
		} else {
			//turn off
			OnCommand_interiorlightdisable(Train, Command);
		}
	}
}

void TTrain::OnCommand_interiorlightenable(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (Train->ggCabLightButton.SubModel == nullptr) {
			// TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
			WriteLog("Interior Light switch is missing, or wasn't defined");
			return;
		}
		// visual feedback
		Train->ggCabLightButton.UpdateValue(1.0, Train->dsbSwitch);

		if (true == Train->bCabLight) {
			return;
		} // already enabled

		Train->bCabLight = true;
		Train->btCabLight.turn(true);
	}
}

void TTrain::OnCommand_interiorlightdisable(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (Train->ggCabLightButton.SubModel == nullptr) {
			// TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
			WriteLog("Interior Light switch is missing, or wasn't defined");
			return;
		}
		// visual feedback
		Train->ggCabLightButton.UpdateValue(0.0, Train->dsbSwitch);

		if (false == Train->bCabLight) {
			return;
		} // already disabled

		Train->bCabLight = false;
		Train->btCabLight.turn(false);
	}
}

void TTrain::OnCommand_interiorlightdimtoggle(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (false == Train->bCabLightDim) {
			// turn on
			OnCommand_interiorlightdimenable(Train, Command);
		} else {
			//turn off
			OnCommand_interiorlightdimdisable(Train, Command);
		}
	}
}

void TTrain::OnCommand_interiorlightdimenable(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (Train->ggCabLightDimButton.SubModel == nullptr) {
			// TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
			WriteLog("Dim Interior Light switch is missing, or wasn't defined");
			return;
		}
		// visual feedback
		Train->ggCabLightDimButton.UpdateValue(1.0, Train->dsbSwitch);

		if (true == Train->bCabLightDim) {
			return;
		} // already enabled

		Train->bCabLightDim = true;
	}
}

void TTrain::OnCommand_interiorlightdimdisable(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (Train->ggCabLightDimButton.SubModel == nullptr) {
			// TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
			WriteLog("Dim Interior Light switch is missing, or wasn't defined");
			return;
		}
		// visual feedback
		Train->ggCabLightDimButton.UpdateValue(0.0, Train->dsbSwitch);

		if (false == Train->bCabLightDim) {
			return;
		} // already disabled

		Train->bCabLightDim = false;
	}
}

void TTrain::OnCommand_instrumentlighttoggle(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (false == Train->InstrumentLightActive) {
			// turn on
			OnCommand_instrumentlightenable(Train, Command);
		} else {
			//turn off
			OnCommand_instrumentlightdisable(Train, Command);
		}
	}
}

void TTrain::OnCommand_instrumentlightenable(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (Train->ggInstrumentLightButton.SubModel == nullptr) {
			// TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
			WriteLog("Instrument Light switch is missing, or wasn't defined");
			return;
		}
		// visual feedback
		Train->ggInstrumentLightButton.UpdateValue(1.0, Train->dsbSwitch);

		if (true == Train->InstrumentLightActive) {
			return;
		} // already enabled

		Train->InstrumentLightActive = true;
	}
}

void TTrain::OnCommand_instrumentlightdisable(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (Train->ggInstrumentLightButton.SubModel == nullptr) {
			// TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
			WriteLog("Instrument Light switch is missing, or wasn't defined");
			return;
		}
		// visual feedback
		Train->ggInstrumentLightButton.UpdateValue(0.0, Train->dsbSwitch);

		if (false == Train->InstrumentLightActive) {
			return;
		} // already disabled

		Train->InstrumentLightActive = false;
	}
}

void TTrain::OnCommand_heatingtoggle(TTrain* Train, command_data const& Command)
{
	if (Train->ggTrainHeatingButton.SubModel == nullptr) {
		if (Command.action == GLFW_PRESS) {
			WriteLog("Train Heating switch is missing, or wasn't defined");
		}
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (false == Train->mvControlled->Heating) {
			// turn on
			OnCommand_heatingenable(Train, Command);
		} else {
			//turn off
			OnCommand_heatingdisable(Train, Command);
		}
	}
}

void TTrain::OnCommand_heatingenable(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// visual feedback
		Train->ggTrainHeatingButton.UpdateValue(1.0, Train->dsbSwitch);

		if (true == Train->mvControlled->Heating) {
			return;
		} // already enabled

		Train->mvControlled->Heating = true;
	}
}

void TTrain::OnCommand_heatingdisable(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// visual feedback
		Train->ggTrainHeatingButton.UpdateValue(0.0, Train->dsbSwitch);

		if (false == Train->mvControlled->Heating) {
			return;
		} // already disabled

		Train->mvControlled->Heating = false;
	}
}

void TTrain::OnCommand_generictoggle(TTrain* Train, command_data const& Command)
{
	auto const itemindex = static_cast<int>(Command.command) - static_cast<int>(user_command::generictoggle0);
	auto& item = Train->ggUniversals[itemindex];

	if (item.SubModel == nullptr) {
		if (Command.action == GLFW_PRESS) {
			WriteLog("Train generic item " + std::to_string(itemindex) + " is missing, or wasn't defined");
		}
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (item.GetValue() < 0.25) {
			// turn on
			// visual feedback
			item.UpdateValue(1.0, Train->dsbSwitch);
		} else {
			// turn off
			// visual feedback
			item.UpdateValue(0.0, Train->dsbSwitch);
		}
	}
}

void TTrain::OnCommand_doorlocktoggle(TTrain* Train, command_data const& Command)
{
	if (Train->ggDoorSignallingButton.SubModel == nullptr) {
		if (Command.action == GLFW_PRESS) {
			WriteLog("Door Lock switch is missing, or wasn't defined");
		}
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the sound can loop uninterrupted
		if (false == Train->mvControlled->DoorSignalling) {
			// turn on
			// TODO: check wheter we really need separate flags for this
			Train->mvControlled->DoorSignalling = true;
			Train->mvOccupied->DoorBlocked = true;
			// visual feedback
			Train->ggDoorSignallingButton.UpdateValue(1.0, Train->dsbSwitch);
		} else {
			// turn off
			// TODO: check wheter we really need separate flags for this
			Train->mvControlled->DoorSignalling = false;
			Train->mvOccupied->DoorBlocked = false;
			// visual feedback
			Train->ggDoorSignallingButton.UpdateValue(0.0, Train->dsbSwitch);
		}
	}
}

void TTrain::OnCommand_doortoggleleft(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// NOTE: test how the door state check works with consists where the occupied vehicle doesn't have opening doors
		if (false == (Train->mvOccupied->ActiveCab == 1 ? Train->mvOccupied->DoorLeftOpened : Train->mvOccupied->DoorRightOpened)) {
			// open
			if (Train->mvOccupied->DoorOpenCtrl != control::driver) {
				return;
			}
			if (Train->mvOccupied->ActiveCab == 1) {
				Train->mvOccupied->DoorLeft(true);
			} else {
				// in the rear cab sides are reversed...
				Train->mvOccupied->DoorRight(true);
			}
			// visual feedback
			Train->ggDoorLeftButton.UpdateValue(1.0, Train->dsbSwitch);
		} else {
			// close
			if (Train->mvOccupied->DoorCloseCtrl != control::driver) {
				return;
			}
			// TODO: move door opening/closing to the update, so the switch animation doesn't hinge on door working
			if (Train->mvOccupied->ActiveCab == 1) {
				Train->mvOccupied->DoorLeft(false);
			} else {
				// in the rear cab sides are reversed...
				Train->mvOccupied->DoorRight(false);
			}
			// visual feedback
			Train->ggDoorLeftButton.UpdateValue(0.0, Train->dsbSwitch);
		}
	}
}

void TTrain::OnCommand_doortoggleright(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		// NOTE: test how the door state check works with consists where the occupied vehicle doesn't have opening doors
		if (false == (Train->mvOccupied->ActiveCab == 1 ? Train->mvOccupied->DoorRightOpened : Train->mvOccupied->DoorLeftOpened)) {
			// open
			if (Train->mvOccupied->DoorOpenCtrl != control::driver) {
				return;
			}
			if (Train->mvOccupied->ActiveCab == 1) {
				Train->mvOccupied->DoorRight(true);
			} else {
				// in the rear cab sides are reversed...
				Train->mvOccupied->DoorLeft(true);
			}
			// visual feedback
			Train->ggDoorRightButton.UpdateValue(1.0, Train->dsbSwitch);
		} else {
			// close
			if (Train->mvOccupied->DoorCloseCtrl != control::driver) {
				return;
			}
			if (Train->mvOccupied->ActiveCab == 1) {
				Train->mvOccupied->DoorRight(false);
			} else {
				// in the rear cab sides are reversed...
				Train->mvOccupied->DoorLeft(false);
			}
			// visual feedback
			Train->ggDoorRightButton.UpdateValue(0.0, Train->dsbSwitch);
		}
	}
}

void TTrain::OnCommand_carcouplingincrease(TTrain* Train, command_data const& Command)
{
	if ((true == FreeFlyModeFlag) && (Command.action == GLFW_PRESS)) {
		// tryb freefly, press only
		auto coupler{ -1 };
		auto* vehicle{ Train->DynamicObject->ABuScanNearestObject(Train->DynamicObject->GetTrack(), 1, 1500, coupler) };
		if (vehicle == nullptr)
			vehicle = Train->DynamicObject->ABuScanNearestObject(Train->DynamicObject->GetTrack(), -1, 1500, coupler);

		if ((coupler != -1) && (vehicle != nullptr)) {
			vehicle->couple(coupler);
		}
		if (Train->DynamicObject->Mechanik) {
			// aktualizacja flag kierunku w składzie
			Train->DynamicObject->Mechanik->CheckVehicles(Connect);
		}
	}
}

void TTrain::OnCommand_carcouplingdisconnect(TTrain* Train, command_data const& Command)
{
	if ((true == FreeFlyModeFlag) && (Command.action == GLFW_PRESS)) {
		// tryb freefly, press only
		auto coupler{ -1 };
		auto* vehicle{ Train->DynamicObject->ABuScanNearestObject(Train->DynamicObject->GetTrack(), 1, 1500, coupler) };
		if (vehicle == nullptr)
			vehicle = Train->DynamicObject->ABuScanNearestObject(Train->DynamicObject->GetTrack(), -1, 1500, coupler);

		if ((coupler != -1) && (vehicle != nullptr)) {
			vehicle->uncouple(coupler);
		}
		if (Train->DynamicObject->Mechanik) {
			// aktualizacja flag kierunku w składzie
			Train->DynamicObject->Mechanik->CheckVehicles(Disconnect);
		}
	}
}

void TTrain::OnCommand_departureannounce(TTrain* Train, command_data const& Command)
{
	if (Train->ggDepartureSignalButton.SubModel == nullptr) {
		if (Command.action == GLFW_PRESS) {
			WriteLog("Departure Signal button is missing, or wasn't defined");
		}
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the sound can loop uninterrupted
		if (false == Train->mvOccupied->DepartureSignal) {
			// turn on
			Train->mvOccupied->signal_departure(true);
			// visual feedback
			Train->ggDepartureSignalButton.UpdateValue(1.0, Train->dsbSwitch);
		}
	} else if (Command.action == GLFW_RELEASE) {
		// turn off
		Train->mvOccupied->signal_departure(false);
		// visual feedback
		Train->ggDepartureSignalButton.UpdateValue(0.0, Train->dsbSwitch);
	}
}

void TTrain::OnCommand_hornlowactivate(TTrain* Train, command_data const& Command)
{
	if ((Train->ggHornButton.SubModel == nullptr) && (Train->ggHornLowButton.SubModel == nullptr)) {
		if (Command.action == GLFW_PRESS) {
			WriteLog("Horn button is missing, or wasn't defined");
		}
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// only need to react to press, sound will continue until stopped
		if (false == TestFlag(Train->mvOccupied->WarningSignal, 1)) {
			// turn on
			Train->mvOccupied->WarningSignal |= 1;
			/*
            if( true == TestFlag( Train->mvOccupied->WarningSignal, 2 ) ) {
                // low and high horn are treated as mutually exclusive
                Train->mvControlled->WarningSignal &= ~2;
            }
*/
			// visual feedback
			Train->ggHornButton.UpdateValue(-1.0);
			Train->ggHornLowButton.UpdateValue(1.0);
		}
	} else if (Command.action == GLFW_RELEASE) {
		// turn off
		/*
        // NOTE: we turn off both low and high horn, due to unreliability of release event when shift key is involved
        Train->mvOccupied->WarningSignal &= ~( 1 | 2 );
*/
		Train->mvOccupied->WarningSignal &= ~1;
		// visual feedback
		Train->ggHornButton.UpdateValue(0.0);
		Train->ggHornLowButton.UpdateValue(0.0);
	}
}

void TTrain::OnCommand_hornhighactivate(TTrain* Train, command_data const& Command)
{
	if ((Train->ggHornButton.SubModel == nullptr) && (Train->ggHornHighButton.SubModel == nullptr)) {
		if (Command.action == GLFW_PRESS) {
			WriteLog("Horn button is missing, or wasn't defined");
		}
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// only need to react to press, sound will continue until stopped
		if (false == TestFlag(Train->mvOccupied->WarningSignal, 2)) {
			// turn on
			Train->mvOccupied->WarningSignal |= 2;
			/*
            if( true == TestFlag( Train->mvOccupied->WarningSignal, 1 ) ) {
                // low and high horn are treated as mutually exclusive
                Train->mvControlled->WarningSignal &= ~1;
            }
*/
			// visual feedback
			Train->ggHornButton.UpdateValue(1.0);
			Train->ggHornHighButton.UpdateValue(1.0);
		}
	} else if (Command.action == GLFW_RELEASE) {
		// turn off
		/*
        // NOTE: we turn off both low and high horn, due to unreliability of release event when shift key is involved
        Train->mvOccupied->WarningSignal &= ~( 1 | 2 );
*/
		Train->mvOccupied->WarningSignal &= ~2;
		// visual feedback
		Train->ggHornButton.UpdateValue(0.0);
		Train->ggHornHighButton.UpdateValue(0.0);
	}
}

void TTrain::OnCommand_whistleactivate(TTrain* Train, command_data const& Command)
{
	if (Train->ggWhistleButton.SubModel == nullptr) {
		if (Command.action == GLFW_PRESS) {
			WriteLog("Whistle button is missing, or wasn't defined");
		}
		return;
	}

	if (Command.action == GLFW_PRESS) {
		// only need to react to press, sound will continue until stopped
		if (false == TestFlag(Train->mvOccupied->WarningSignal, 4)) {
			// turn on
			Train->mvOccupied->WarningSignal |= 4;
			// visual feedback
			Train->ggWhistleButton.UpdateValue(1.0);
		}
	} else if (Command.action == GLFW_RELEASE) {
		// turn off
		Train->mvOccupied->WarningSignal &= ~4;
		// visual feedback
		Train->ggWhistleButton.UpdateValue(0.0);
	}
}

void TTrain::OnCommand_radiotoggle(TTrain* Train, command_data const& Command)
{
	if (Train->ggRadioButton.SubModel == nullptr) {
		if (Command.action == GLFW_PRESS) {
			WriteLog("Radio switch is missing, or wasn't defined");
		}
		/*
        // NOTE: we ignore the lack of 3d model to allow system reset after receiving radio-stop signal
        return;
*/
	}

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the sound can loop uninterrupted
		if (false == Train->mvOccupied->Radio) {
			// turn on
			Train->mvOccupied->Radio = true;
			// visual feedback
			Train->ggRadioButton.UpdateValue(1.0, Train->dsbSwitch);
		} else {
			// turn off
			Train->mvOccupied->Radio = false;
			// visual feedback
			Train->ggRadioButton.UpdateValue(0.0, Train->dsbSwitch);
		}
	}
}

void TTrain::OnCommand_radiochannelincrease(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		Train->iRadioChannel = clamp(Train->iRadioChannel + 1, 1, 10);
		// visual feedback
		Train->ggRadioChannelSelector.UpdateValue(Train->iRadioChannel - 1);
		Train->ggRadioChannelNext.UpdateValue(1.0);
	} else if (Command.action == GLFW_RELEASE) {
		// visual feedback
		Train->ggRadioChannelNext.UpdateValue(0.0);
	}
}

void TTrain::OnCommand_radiochanneldecrease(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		Train->iRadioChannel = clamp(Train->iRadioChannel - 1, 1, 10);
		// visual feedback
		Train->ggRadioChannelSelector.UpdateValue(Train->iRadioChannel - 1);
		Train->ggRadioChannelPrevious.UpdateValue(1.0);
	} else if (Command.action == GLFW_RELEASE) {
		// visual feedback
		Train->ggRadioChannelPrevious.UpdateValue(0.0);
	}
}

void TTrain::OnCommand_radiostopsend(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		if ((true == Train->mvOccupied->Radio) && (Train->mvControlled->Battery || Train->mvControlled->ConverterFlag)) {
			simulation::Region->RadioStop(Train->Dynamic()->GetPosition());
		}
		// visual feedback
		Train->ggRadioStop.UpdateValue(1.0);
	} else if (Command.action == GLFW_RELEASE) {
		// visual feedback
		Train->ggRadioStop.UpdateValue(0.0);
	}
}

void TTrain::OnCommand_radiostoptest(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		if ((Train->RadioChannel() == 10) && (Train->mvControlled->Battery || Train->mvControlled->ConverterFlag)) {
			Train->Dynamic()->RadioStop();
		}
		// visual feedback
		Train->ggRadioTest.UpdateValue(1.0);
	} else if (Command.action == GLFW_RELEASE) {
		// visual feedback
		Train->ggRadioTest.UpdateValue(0.0);
	}
}

void TTrain::OnCommand_cabchangeforward(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		if (false == Train->CabChange(1)) {
			if (TestFlag(Train->DynamicObject->MoverParameters->Couplers[side::front].CouplingFlag, coupling::gangway)) {
				// przejscie do nastepnego pojazdu
				Global.changeDynObj = Train->DynamicObject->PrevConnected;
				Global.changeDynObj->MoverParameters->ActiveCab = (Train->DynamicObject->PrevConnectedNo ? -1 : 1);
			}
		}
	}
}

void TTrain::OnCommand_cabchangebackward(TTrain* Train, command_data const& Command)
{
	if (Command.action == GLFW_PRESS) {
		if (false == Train->CabChange(-1)) {
			if (TestFlag(Train->DynamicObject->MoverParameters->Couplers[side::rear].CouplingFlag, coupling::gangway)) {
				// przejscie do nastepnego pojazdu
				Global.changeDynObj = Train->DynamicObject->NextConnected;
				Global.changeDynObj->MoverParameters->ActiveCab = (Train->DynamicObject->NextConnectedNo ? -1 : 1);
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
	double const iVel = std::min(DynamicObject->GetVelocity(), 150.0);

	if ((false == Global.iSlowMotion) // musi być pełna prędkość
	    && (pMechOffset.y < 4.0)) // Ra 15-01: przy oglądaniu pantografu bujanie przeszkadza
	{
		Math3D::vector3 shakevector;
		if ((mvOccupied->EngineType == DieselElectric) || (mvOccupied->EngineType == DieselEngine)) {
			if (std::abs(mvOccupied->enrot) > 0.0) {
				// engine vibration
				shakevector.x += (std::sin(mvOccupied->eAngle * 4.0) * dt * EngineShake.scale)
				                 // fade in with rpm above threshold
				                 * clamp((mvOccupied->enrot - EngineShake.fadein_offset) * EngineShake.fadein_factor, 0.0, 1.0)
				                 // fade out with rpm above threshold
				                 * interpolate(1.0, 0.0, clamp((mvOccupied->enrot - EngineShake.fadeout_offset) * EngineShake.fadeout_factor, 0.0, 1.0));
			}
		}

		if ((HuntingShake.fadein_begin > 0.f) && (true == mvOccupied->TruckHunting)) {
			// hunting oscillation
			HuntingAngle = clamp_circular(HuntingAngle + 4.0 * HuntingShake.frequency * dt * mvOccupied->Vel, 360.0);
			shakevector.x += (std::sin(glm::radians(HuntingAngle)) * dt * HuntingShake.scale) *
			                 interpolate(0.0, 1.0, clamp((mvOccupied->Vel - HuntingShake.fadein_begin) / (HuntingShake.fadein_end - HuntingShake.fadein_begin), 0.0, 1.0));
		}

		if (iVel > 0.5) {
			// acceleration-driven base shake
			shakevector += Math3D::vector3(-mvOccupied->AccN * dt * 5.0, // highlight side sway
			-mvOccupied->AccVert * dt,
			-mvOccupied->AccSVBased * dt * 1.25); // accent acceleration/deceleration
		}

		shake += 1.25 * MechSpring.ComputateForces(shakevector, pMechShake);

		if (Random(iVel) > 25.0) {
			// extra shake at increased velocity
			shake += MechSpring.ComputateForces(Math3D::vector3((Random(iVel * 2) - iVel) / ((iVel * 2) * 4) * fMechSpringX, (Random(iVel * 2) - iVel) / ((iVel * 2) * 4) * fMechSpringY,
			                                    (Random(iVel * 2) - iVel) / ((iVel * 2) * 4) * fMechSpringZ) *
			                                    1.25,
			pMechShake);
			//                    * (( 200 - DynamicObject->MyTrack->iQualityFlag ) * 0.0075 ); // scale to 75-150% based on track quality
		}
		shake *= 0.85;

		vMechVelocity -= (shake + vMechVelocity * 100) * (fMechSpringX + fMechSpringY + fMechSpringZ) / (200);
		//        shake *= 0.95 * dt; // shake damping

		// McZapkie:
		pMechShake += vMechVelocity * dt;
		if ((pMechShake.y > fMechMaxSpring) || (pMechShake.y < -fMechMaxSpring)) {
			vMechVelocity.y = -vMechVelocity.y;
		}
		// Ra 2015-01: dotychczasowe rzucanie
		pMechOffset += vMechMovement * dt;
		// ABu011104: 5*pMechShake.y, zeby ladnie pudlem rzucalo :)
		pMechPosition = pMechOffset + Math3D::vector3(1.5 * pMechShake.x, 2.0 * pMechShake.y, 1.5 * pMechShake.z);

		//        pMechShake = interpolate( pMechShake, Math3D::vector3(), clamp( dt, 0.0, 1.0 ) );
	} else { // hamowanie rzucania przy spadku FPS
		pMechShake -= pMechShake * std::min(dt, 1.0); // po tym chyba potrafią zostać jakieś ułamki, które powodują zjazd
		pMechOffset += vMechMovement * dt;
		vMechVelocity.y = 0.5 * vMechVelocity.y;
		pMechPosition = pMechOffset + Math3D::vector3(pMechShake.x, 5 * pMechShake.y, pMechShake.z);
	}
	// numer kabiny (-1: kabina B)
	if (DynamicObject->Mechanik) // może nie być?
		if (DynamicObject->Mechanik->AIControllFlag) // jeśli prowadzi AI
		{ // Ra: przesiadka, jeśli AI zmieniło kabinę (a człon?)...
			if (iCabn != (DynamicObject->MoverParameters->ActiveCab == -1 ? 2 : DynamicObject->MoverParameters->ActiveCab))
				InitializeCab(DynamicObject->MoverParameters->ActiveCab, DynamicObject->asBaseDir + DynamicObject->MoverParameters->TypeName + ".mmd");
		}
	iCabn = (DynamicObject->MoverParameters->ActiveCab == -1 ? 2 : DynamicObject->MoverParameters->ActiveCab);
	if (!DebugModeFlag) { // sprawdzaj więzy //Ra: nie tu!
		if (pMechPosition.x < Cabine[iCabn].CabPos1.x)
			pMechPosition.x = Cabine[iCabn].CabPos1.x;
		if (pMechPosition.x > Cabine[iCabn].CabPos2.x)
			pMechPosition.x = Cabine[iCabn].CabPos2.x;
		if (pMechPosition.z < Cabine[iCabn].CabPos1.z)
			pMechPosition.z = Cabine[iCabn].CabPos1.z;
		if (pMechPosition.z > Cabine[iCabn].CabPos2.z)
			pMechPosition.z = Cabine[iCabn].CabPos2.z;
		if (pMechPosition.y > Cabine[iCabn].CabPos1.y + 1.8)
			pMechPosition.y = Cabine[iCabn].CabPos1.y + 1.8;
		if (pMechPosition.y < Cabine[iCabn].CabPos1.y + 0.5)
			pMechPosition.y = Cabine[iCabn].CabPos2.y + 0.5;

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
};

// returns position of the mechanic in the scene coordinates
Math3D::vector3 TTrain::GetWorldMechPosition()
{
	auto position = DynamicObject->mMatrix * pMechPosition; // położenie względem środka pojazdu w układzie scenerii
	position += DynamicObject->GetPosition();
	return position;
}

bool TTrain::Update(double const Deltatime)
{
	// train state verification
	// line breaker:
	if (m_linebreakerstate == 0) {
		if (true == mvControlled->Mains) {
			// crude way to sync state of the linebreaker with ai-issued commands
			m_linebreakerstate = 1;
		}
	}
	if (m_linebreakerstate == 1) {
		if (false == (mvControlled->Mains || mvControlled->dizel_startup)) {
			// crude way to catch cases where the main was knocked out
			// because the state of the line breaker isn't changed to match, we need to do it here manually
			m_linebreakerstate = 0;
		}
	}

	if ((ggMainButton.GetDesiredValue() > 0.95) || (ggMainOnButton.GetDesiredValue() > 0.95)) {
		// keep track of period the line breaker button is held down, to determine when/if circuit closes
		if ((fHVoltage > 0.5 * mvControlled->EnginePowerSource.MaxVoltage) ||
		    ((mvControlled->EngineType != ElectricSeriesMotor) && (mvControlled->EngineType != ElectricInductionMotor) && (true == mvControlled->Battery))) {
			// prevent the switch from working if there's no power
			// TODO: consider whether it makes sense for diesel engines and such
			fMainRelayTimer += Deltatime;
		}
	} else {
		// button isn't down, reset the timer
		fMainRelayTimer = 0.0f;
	}
	if (ggMainOffButton.GetDesiredValue() > 0.95) {
		// if the button disconnecting the line breaker is down prevent the timer from accumulating
		fMainRelayTimer = 0.0f;
	}
	if (m_linebreakerstate == 0) {
		if (fMainRelayTimer > mvControlled->InitialCtrlDelay) {
			// wlaczanie WSa z opoznieniem
			// mark the line breaker as ready to close; for electric vehicles with impulse switch the setup is completed on button release
			m_linebreakerstate = 2;
		}
	}
	if (m_linebreakerstate == 2) {
		// for diesels and/or vehicles with toggle switch setup we complete the engine start here
		if ((ggMainOnButton.SubModel == nullptr) || ((mvControlled->EngineType == DieselEngine) || (mvControlled->EngineType == DieselElectric))) {
			// try to finalize state change of the line breaker, set the state based on the outcome
			m_linebreakerstate = (mvControlled->MainSwitch(true) ? 1 : 0);
		}
	}

	// check for received user commands
	// NOTE: this is a temporary arrangement, for the transition period from old command setup to the new one
	// eventually commands are going to be retrieved directly by the vehicle, filtered through active control stand
	// and ultimately executed, provided the stand allows it.
	command_data commanddata;
	// NOTE: currently we're only storing commands for local vehicle and there's no id system in place,
	// so we're supplying 'default' vehicle id of 0
	while (simulation::Commands.pop(commanddata, static_cast<std::size_t>(command_target::vehicle) | 0)) {
		auto lookup = m_commandhandlers.find(commanddata.command);
		if (lookup != m_commandhandlers.end()) {
			// debug data
			if (commanddata.action == GLFW_PRESS) {
				WriteLog(mvOccupied->Name + " received command: [" + simulation::Commands_descriptions[static_cast<std::size_t>(commanddata.command)].name + "]");
			}
			// pass the command to the assigned handler
			lookup->second(this, commanddata);
		}
	}

	// update driver's position
	{
		auto Vec = Global.pCamera->Velocity * -2.0; // -7.5 * Timer::GetDeltaRenderTime();
		Vec.y = -Vec.y;
		if (mvOccupied->ActiveCab < 0) {
			Vec *= -1.0f;
			Vec.y = -Vec.y;
		}
		Vec.RotateY(Global.pCamera->Yaw);
		vMechMovement = Vec;
	}

	if (DynamicObject->mdKabina) { // Ra: TODO: odczyty klawiatury/pulpitu nie powinny być uzależnione od istnienia modelu kabiny

		if ((DynamicObject->Mechanik != nullptr) && (false == DynamicObject->Mechanik->AIControllFlag) // nie blokujemy AI
		    && ((mvOccupied->TrainType == dt_ET40) || (mvOccupied->TrainType == dt_EP05))) {
			// dla ET40 i EU05 automatyczne cofanie nastawnika - i tak nie będzie to działać dobrze...
			// TODO: remove direct keyboard check, use deltatime to stabilize speed
			if ((glfwGetKey(Global.window, GLFW_KEY_KP_ADD) != GLFW_TRUE) && (mvOccupied->MainCtrlPos > mvOccupied->MainCtrlActualPos)) {
				mvOccupied->DecMainCtrl(1);
			}
			if ((glfwGetKey(Global.window, GLFW_KEY_KP_SUBTRACT) != GLFW_TRUE) && (mvOccupied->MainCtrlPos < mvOccupied->MainCtrlActualPos)) {
				mvOccupied->IncMainCtrl(1); // Ra 15-01: a to nie miało być tylko cofanie?
			}
		}

		tor = DynamicObject->GetTrack(); // McZapkie-180203
		// McZapkie: predkosc wyswietlana na tachometrze brana jest z obrotow kol
		auto const maxtacho{ 3.0 };
		fTachoVelocity = static_cast<float>(std::min(std::abs(11.31 * mvControlled->WheelDiameter * mvControlled->nrot), mvControlled->Vmax * 1.05));
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
			// szybciej zacznij stukac
			fTachoCount = std::min(maxtacho, fTachoCount + Deltatime * 3);
		} else if (fTachoCount > 0) {
			// schodz powoli - niektore haslery to ze 4 sekundy potrafia stukac
			fTachoCount = std::max(0.0, fTachoCount - Deltatime * 0.66);
		}

		// Ra 2014-09: napięcia i prądy muszą być ustalone najpierw, bo wysyłane są ewentualnie na PoKeys
		if ((mvControlled->EngineType != DieselElectric) && (mvControlled->EngineType != ElectricInductionMotor)) // Ra 2014-09: czy taki rozdzia? ma sens?
			fHVoltage = mvControlled->RunningTraction.TractionVoltage; // Winger czy to nie jest zle?
		// *mvControlled->Mains);
		else
			fHVoltage = mvControlled->Voltage;
		if (ShowNextCurrent) { // jeśli pokazywać drugi człon
			if (mvSecond) { // o ile jest ten drugi
				fHCurrent[0] = mvSecond->ShowCurrent(0) * 1.05;
				fHCurrent[1] = mvSecond->ShowCurrent(1) * 1.05;
				fHCurrent[2] = mvSecond->ShowCurrent(2) * 1.05;
				fHCurrent[3] = mvSecond->ShowCurrent(3) * 1.05;
			} else
				fHCurrent[0] = fHCurrent[1] = fHCurrent[2] = fHCurrent[3] = 0.0; // gdy nie ma człona
		} else { // normalne pokazywanie
			fHCurrent[0] = mvControlled->ShowCurrent(0);
			fHCurrent[1] = mvControlled->ShowCurrent(1);
			fHCurrent[2] = mvControlled->ShowCurrent(2);
			fHCurrent[3] = mvControlled->ShowCurrent(3);
		}

		bool kier = (DynamicObject->DirectionGet() * mvOccupied->ActiveCab > 0);
		TDynamicObject* p = DynamicObject->GetFirstDynamic(mvOccupied->ActiveCab < 0 ? 1 : 0, 4);
		int in = 0;
		fEIMParams[0][6] = 0;
		iCarNo = 0;
		iPowerNo = 0;
		iUnitNo = 1;

		for (int i = 0; i < 8; i++) {
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
		for (int i = 0; i < 20; i++) {
			if (p) {
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
				bComp[iUnitNo - 1][0] = (bComp[iUnitNo - 1][0] || p->MoverParameters->CompressorAllow || (p->MoverParameters->CompressorStart == start::automatic));
				bSlip[i] = p->MoverParameters->SlippingWheels;
				if (p->MoverParameters->CompressorSpeed > 0.00001) {
					bComp[iUnitNo - 1][1] = (bComp[iUnitNo - 1][1] || p->MoverParameters->CompressorFlag);
				}
				if ((in < 8) && (p->MoverParameters->eimc[eimc_p_Pmax] > 1)) {
					fEIMParams[1 + in][0] = p->MoverParameters->eimv[eimv_Fmax];
					fEIMParams[1 + in][1] = Max0R(fEIMParams[1 + in][0], 0);
					fEIMParams[1 + in][2] = -Min0R(fEIMParams[1 + in][0], 0);
					fEIMParams[1 + in][3] = p->MoverParameters->eimv[eimv_Fmax] / Max0R(p->MoverParameters->eimv[eimv_Fful], 1);
					fEIMParams[1 + in][4] = Max0R(fEIMParams[1 + in][3], 0);
					fEIMParams[1 + in][5] = -Min0R(fEIMParams[1 + in][3], 0);
					fEIMParams[1 + in][6] = p->MoverParameters->eimv[eimv_If];
					fEIMParams[1 + in][7] = p->MoverParameters->eimv[eimv_U];
					fEIMParams[1 + in][8] = p->MoverParameters->Itot; //p->MoverParameters->eimv[eimv_Ipoj];
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
			} else {
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
			fEIMParams[0][3] = mvControlled->eimv[eimv_Fzad] - mvOccupied->LocalBrakeRatio(); // procent zadany
		fEIMParams[0][4] = Max0R(fEIMParams[0][3], 0);
		fEIMParams[0][5] = -Min0R(fEIMParams[0][3], 0);
		fEIMParams[0][1] = fEIMParams[0][4] * mvControlled->eimv[eimv_Fful];
		fEIMParams[0][2] = fEIMParams[0][5] * mvControlled->eimv[eimv_Fful];
		fEIMParams[0][0] = fEIMParams[0][1] - fEIMParams[0][2];
		fEIMParams[0][7] = 0;
		fEIMParams[0][8] = 0;
		fEIMParams[0][9] = 0;

		for (int i = in; i < 8; i++) {
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
		if (Global.iFeedbackMode == 4) {
			// wykonywać tylko gdy wyprowadzone na pulpit
			// Ra: sterowanie miernikiem: zbiornik główny
			Console::ValueSet(0, mvOccupied->Compressor);
			// Ra: sterowanie miernikiem: przewód główny
			Console::ValueSet(1, mvOccupied->PipePress);
			// Ra: sterowanie miernikiem: cylinder hamulcowy
			Console::ValueSet(2, mvOccupied->BrakePress);
			// woltomierz wysokiego napięcia
			Console::ValueSet(3, fHVoltage);
			// Ra: sterowanie miernikiem: drugi amperomierz
			Console::ValueSet(4, fHCurrent[2]);
			// pierwszy amperomierz; dla EZT prąd całkowity
			Console::ValueSet(5, fHCurrent[(mvControlled->TrainType & dt_EZT) ? 0 : 1]);
			// Ra: prędkość na pin 43 - wyjście analogowe (to nie jest PWM); skakanie zapewnia mechanika napędu
			Console::ValueSet(6, fTachoVelocity);
		}
#endif
		// hunter-080812: wyrzucanie szybkiego na elektrykach gdy nie ma napiecia przy dowolnym ustawieniu kierunkowego
		// Ra: to już jest w T_MoverParameters::TractionForce(), ale zależy od kierunku
		if ((mvControlled->Mains) && (mvControlled->EngineType == ElectricSeriesMotor)) {
			if (std::max(mvControlled->GetTrainsetVoltage(), std::fabs(mvControlled->RunningTraction.TractionVoltage)) < 0.5 * mvControlled->EnginePowerSource.MaxVoltage) {
				// TODO: check whether it should affect entire consist for EMU
				// TODO: check whether it should happen if there's power supplied alternatively through hvcouplers
				// TODO: potentially move this to the mover module, as there isn't much reason to have this dependent on the operator presence
				mvControlled->MainSwitch(false, (mvControlled->TrainType == dt_EZT ? range::unit : range::local));
			}
		}

		// hunter-091012: swiatlo
		if (bCabLight == true) {
			if (bCabLightDim == true)
				iCabLightFlag = 1;
			else
				iCabLightFlag = 2;
		} else
			iCabLightFlag = 0;

		//------------------
		// hunter-261211: nadmiarowy przetwornicy i ogrzewania
		// Ra 15-01: to musi stąd wylecieć - zależności nie mogą być w kabinie
		if (mvControlled->ConverterFlag == true) {
			fConverterTimer += Deltatime;
			if ((mvControlled->CompressorFlag == true) && (mvControlled->CompressorPower == 1) && ((mvControlled->EngineType == ElectricSeriesMotor) || (mvControlled->TrainType == dt_EZT)) &&
			    (DynamicObject->Controller == Humandriver)) // hunter-110212: poprawka dla EZT
			{ // hunter-091012: poprawka (zmiana warunku z CompressorPower /rozne od
				// 0/ na /rowne 1/)
				if (fConverterTimer < fConverterPrzekaznik) {
					mvControlled->ConvOvldFlag = true;
					if (mvControlled->TrainType != dt_EZT)
						mvControlled->MainSwitch(false, (mvControlled->TrainType == dt_EZT ? range::unit : range::local));
				} else if (fConverterTimer >= fConverterPrzekaznik) {
					// changed switch from always true to take into account state of the compressor switch
					mvControlled->CompressorSwitch(mvControlled->CompressorAllow);
				}
			}
		} else
			fConverterTimer = 0;
		//------------------

		// youBy - prad w drugim czlonie: galaz lub calosc
		{
			TDynamicObject* tmp;
			tmp = NULL;
			if (DynamicObject->NextConnected)
				if ((TestFlag(mvControlled->Couplers[1].CouplingFlag, ctrain_controll)) && (mvOccupied->ActiveCab == 1))
					tmp = DynamicObject->NextConnected;
			if (DynamicObject->PrevConnected)
				if ((TestFlag(mvControlled->Couplers[0].CouplingFlag, ctrain_controll)) && (mvOccupied->ActiveCab == -1))
					tmp = DynamicObject->PrevConnected;
			if (tmp) {
				if (tmp->MoverParameters->Power > 0) {
					if (ggI1B.SubModel) {
						ggI1B.UpdateValue(tmp->MoverParameters->ShowCurrent(1));
						ggI1B.Update();
					}
					if (ggI2B.SubModel) {
						ggI2B.UpdateValue(tmp->MoverParameters->ShowCurrent(2));
						ggI2B.Update();
					}
					if (ggI3B.SubModel) {
						ggI3B.UpdateValue(tmp->MoverParameters->ShowCurrent(3));
						ggI3B.Update();
					}
					if (ggItotalB.SubModel) {
						ggItotalB.UpdateValue(tmp->MoverParameters->ShowCurrent(0));
						ggItotalB.Update();
					}
					if (ggWater1TempB.SubModel) {
						ggWater1TempB.UpdateValue(tmp->MoverParameters->dizel_heat.temperatura1);
						ggWater1TempB.Update();
					}
					if (ggOilPressB.SubModel) {
						ggOilPressB.UpdateValue(tmp->MoverParameters->OilPump.pressure_present);
						ggOilPressB.Update();
					}
				}
			}
		}
		// McZapkie-300302: zegarek
		if (ggClockMInd.SubModel) {
			ggClockSInd.UpdateValue(simulation::Time.data().wSecond);
			ggClockSInd.Update();
			ggClockMInd.UpdateValue(simulation::Time.data().wMinute);
			ggClockMInd.Update();
			ggClockHInd.UpdateValue(simulation::Time.data().wHour + simulation::Time.data().wMinute / 60.0);
			ggClockHInd.Update();
		}

		Cabine[iCabn].Update(); // nowy sposób ustawienia animacji
		if (ggZbS.SubModel) {
			ggZbS.UpdateValue(mvOccupied->Handle->GetCP());
			ggZbS.Update();
		}

		// youBy - napiecie na silnikach
		if (ggEngineVoltage.SubModel) {
			if (mvControlled->DynamicBrakeFlag) {
				ggEngineVoltage.UpdateValue(abs(mvControlled->Im * 5));
			} else {
				int x;
				if ((mvControlled->TrainType == dt_ET42) && (mvControlled->Imax == mvControlled->ImaxHi))
					x = 1;
				else
					x = 2;
				if ((mvControlled->RList[mvControlled->MainCtrlActualPos].Mn > 0) && (abs(mvControlled->Im) > 0)) {
					ggEngineVoltage.UpdateValue((x * (mvControlled->RunningTraction.TractionVoltage - mvControlled->RList[mvControlled->MainCtrlActualPos].R * abs(mvControlled->Im)) /
					                             mvControlled->RList[mvControlled->MainCtrlActualPos].Mn));
				} else {
					ggEngineVoltage.UpdateValue(0);
				}
			}
			ggEngineVoltage.Update();
		}

		// Winger 140404 - woltomierz NN
		if (ggLVoltage.SubModel) {
			// NOTE: since we don't have functional converter object, we're faking it here by simple check whether converter is on
			// TODO: implement object-based circuits and power systems model so we can have this working more properly
			ggLVoltage.UpdateValue(std::max((mvControlled->ConverterFlag ? mvControlled->NominalBatteryVoltage : 0.0), (mvControlled->Battery ? mvControlled->BatteryVoltage : 0.0)));
			ggLVoltage.Update();
		}

		if (mvControlled->EngineType == DieselElectric) { // ustawienie zmiennych dla silnika spalinowego
			fEngine[1] = mvControlled->ShowEngineRotation(1);
			fEngine[2] = mvControlled->ShowEngineRotation(2);
		}

		else if (mvControlled->EngineType == DieselEngine) { // albo dla innego spalinowego
			fEngine[1] = mvControlled->ShowEngineRotation(1);
			fEngine[2] = mvControlled->ShowEngineRotation(2);
			fEngine[3] = mvControlled->ShowEngineRotation(3);
			if (ggMainGearStatus.SubModel) {
				if (mvControlled->Mains)
					ggMainGearStatus.UpdateValue(1.1 - std::abs(mvControlled->dizel_automaticgearstatus));
				else
					ggMainGearStatus.UpdateValue(0.0);
				ggMainGearStatus.Update();
			}
			if (ggIgnitionKey.SubModel) {
				ggIgnitionKey.UpdateValue(mvControlled->dizel_startup);
				ggIgnitionKey.Update();
			}
		}

		if (mvControlled->SlippingWheels) {
			// Ra 2014-12: lokomotywy 181/182 dostają SlippingWheels po zahamowaniu powyżej 2.85 bara i buczały
			double veldiff = (DynamicObject->GetVelocity() - fTachoVelocity) / mvControlled->Vmax;
			if (veldiff < -0.01) {
				// 1% Vmax rezerwy, żeby 181/182 nie buczały po zahamowaniu, ale to proteza
				if (std::abs(mvControlled->Im) > 10.0) {
					btLampkaPoslizg.turn(true);
				}
			}
		} else {
			btLampkaPoslizg.turn(false);
		}

		if ((mvControlled->Mains) || (mvControlled->TrainType == dt_EZT)) {
			btLampkaNadmSil.turn(mvControlled->FuseFlagCheck());
		} else {
			btLampkaNadmSil.turn(false);
		}

		if (mvControlled->Battery || mvControlled->ConverterFlag) {
			// alerter test
			if (true == CAflag) {
				if (ggSecurityResetButton.GetDesiredValue() > 0.95) {
					fCzuwakTestTimer += Deltatime;
				}
				if (fCzuwakTestTimer > 1.0) {
					SetFlag(mvOccupied->SecuritySystem.Status, s_CAtest);
				}
			}
			// McZapkie-141102: SHP i czuwak, TODO: sygnalizacja kabinowa
			if (mvOccupied->SecuritySystem.Status > 0) {
				if (fBlinkTimer > fCzuwakBlink)
					fBlinkTimer = -fCzuwakBlink;
				else
					fBlinkTimer += Deltatime;

				// hunter-091012: dodanie testu czuwaka
				if ((TestFlag(mvOccupied->SecuritySystem.Status, s_aware)) || (TestFlag(mvOccupied->SecuritySystem.Status, s_CAtest))) {
					btLampkaCzuwaka.turn(fBlinkTimer > 0);
				} else
					btLampkaCzuwaka.turn(false);

				btLampkaSHP.turn(TestFlag(mvOccupied->SecuritySystem.Status, s_active));
			} else // wylaczone
			{
				btLampkaCzuwaka.turn(false);
				btLampkaSHP.turn(false);
			}

			btLampkaWylSzybki.turn((((m_linebreakerstate == 2) || (true == mvControlled->Mains)) ? true : false));
			// NOTE: 'off' variant uses the same test, but opposite resulting states
			btLampkaWylSzybkiOff.turn((((m_linebreakerstate == 2) || (true == mvControlled->Mains)) ? false : true));

			btLampkaPrzetw.turn(mvControlled->ConverterFlag);
			btLampkaPrzetwOff.turn(false == mvControlled->ConverterFlag);
			btLampkaNadmPrzetw.turn(mvControlled->ConvOvldFlag);

			btLampkaOpory.turn(mvControlled->StLinFlag ? mvControlled->ResistorsFlagCheck() : false);

			btLampkaBezoporowa.turn((true == mvControlled->ResistorsFlagCheck()) || (mvControlled->MainCtrlActualPos == 0)); // do EU04

			if ((mvControlled->Im != 0) || (mvOccupied->BrakePress > 2) || (mvOccupied->PipePress < 3.6)) {
				// Ra: czy to jest udawanie działania styczników liniowych?
				btLampkaStyczn.turn(false);
			} else if (mvOccupied->BrakePress < 1)
				btLampkaStyczn.turn(true); // mozna prowadzic rozruch
			if (((TestFlag(mvControlled->Couplers[1].CouplingFlag, ctrain_controll)) && (mvControlled->CabNo == 1)) ||
			    ((TestFlag(mvControlled->Couplers[0].CouplingFlag, ctrain_controll)) && (mvControlled->CabNo == -1)))
				btLampkaUkrotnienie.turn(true);
			else
				btLampkaUkrotnienie.turn(false);

			//         if
			//         ((TestFlag(mvControlled->BrakeStatus,+b_Rused+b_Ractive)))//Lampka drugiego stopnia hamowania
			btLampkaHamPosp.turn((TestFlag(mvOccupied->Hamulec->GetBrakeStatus(), 1))); // lampka drugiego stopnia hamowania
			//TODO: youBy wyciągnąć flagę wysokiego stopnia

			// hunter-121211: lampka zanikowo-pradowego wentylatorow:
			btLampkaNadmWent.turn((mvControlled->RventRot < 5.0) && (mvControlled->ResistorsFlagCheck()));
			//-------

			btLampkaWysRozr.turn(!(mvControlled->Imax < mvControlled->ImaxHi));

			if ((false == mvControlled->DelayCtrlFlag) && ((mvControlled->ScndCtrlActualPos > 0) || ((mvControlled->RList[mvControlled->MainCtrlActualPos].ScndAct != 0) &&
			                                                                                         (mvControlled->RList[mvControlled->MainCtrlActualPos].ScndAct != 255)))) {
				btLampkaBoczniki.turn(true);
			} else {
				btLampkaBoczniki.turn(false);
			}

			btLampkaNapNastHam.turn(mvControlled->ActiveDir != 0); // napiecie na nastawniku hamulcowym
			btLampkaSprezarka.turn(mvControlled->CompressorFlag); // mutopsitka dziala
			btLampkaSprezarkaOff.turn(false == mvControlled->CompressorFlag);
			btLampkaFuelPumpOff.turn(false == mvControlled->FuelPump.is_active);
			// boczniki
			unsigned char scp; // Ra: dopisałem "unsigned"
			// Ra: w SU45 boczniki wchodzą na MainCtrlPos, a nie na MainCtrlActualPos
			// - pokićkał ktoś?
			scp = mvControlled->RList[mvControlled->MainCtrlPos].ScndAct;
			scp = (scp == 255 ? 0 : scp); // Ra: whatta hella is this?
			if ((mvControlled->ScndCtrlPos > 0) || (mvControlled->ScndInMain != 0) && (scp > 0)) { // boczniki pojedynczo
				btLampkaBocznik1.turn(true);
				btLampkaBocznik2.turn(mvControlled->ScndCtrlPos > 1);
				btLampkaBocznik3.turn(mvControlled->ScndCtrlPos > 2);
				btLampkaBocznik4.turn(mvControlled->ScndCtrlPos > 3);
			} else { // wyłączone wszystkie cztery
				btLampkaBocznik1.turn(false);
				btLampkaBocznik2.turn(false);
				btLampkaBocznik3.turn(false);
				btLampkaBocznik4.turn(false);
			}
		} else {
			// wylaczone
			btLampkaCzuwaka.turn(false);
			btLampkaSHP.turn(false);
			btLampkaWylSzybki.turn(false);
			btLampkaWylSzybkiOff.turn(false);
			btLampkaWysRozr.turn(false);
			btLampkaOpory.turn(false);
			btLampkaStyczn.turn(false);
			btLampkaUkrotnienie.turn(false);
			btLampkaHamPosp.turn(false);
			btLampkaBoczniki.turn(false);
			btLampkaNapNastHam.turn(false);
			btLampkaPrzetw.turn(false);
			btLampkaPrzetwOff.turn(false);
			btLampkaNadmPrzetw.turn(false);
			btLampkaSprezarka.turn(false);
			btLampkaSprezarkaOff.turn(false);
			btLampkaFuelPumpOff.turn(false);
			btLampkaBezoporowa.turn(false);
		}
		if (mvControlled->Signalling == true) {
			if ((mvOccupied->BrakePress >= 0.145f) && (mvControlled->Battery == true) && (mvControlled->Signalling == true)) {
				btLampkaHamowanie1zes.turn(true);
			}
			if (mvControlled->BrakePress < 0.075f) {
				btLampkaHamowanie1zes.turn(false);
			}
		} else {
			btLampkaHamowanie1zes.turn(false);
		}
		btLampkaBlokadaDrzwi.turn(mvControlled->DoorSignalling ? mvOccupied->DoorBlockedFlag() && mvControlled->Battery : false);

		{ // yB - wskazniki drugiego czlonu
			TDynamicObject* tmp; //=mvControlled->mvSecond; //Ra 2014-07: trzeba to
			// jeszcze wyjąć z kabiny...
			// Ra 2014-07: no nie ma potrzeby szukać tego w każdej klatce
			tmp = NULL;
			if ((TestFlag(mvControlled->Couplers[1].CouplingFlag, ctrain_controll)) && (mvOccupied->ActiveCab > 0))
				tmp = DynamicObject->NextConnected;
			if ((TestFlag(mvControlled->Couplers[0].CouplingFlag, ctrain_controll)) && (mvOccupied->ActiveCab < 0))
				tmp = DynamicObject->PrevConnected;

			if (tmp)
				if (mvControlled->Battery || mvControlled->ConverterFlag) {
					btLampkaWylSzybkiB.turn(tmp->MoverParameters->Mains);
					btLampkaWylSzybkiBOff.turn(false == tmp->MoverParameters->Mains);

					btLampkaOporyB.turn(tmp->MoverParameters->ResistorsFlagCheck());
					btLampkaBezoporowaB.turn((true == tmp->MoverParameters->ResistorsFlagCheck()) || (tmp->MoverParameters->MainCtrlActualPos == 0)); // do EU04

					if ((tmp->MoverParameters->Itot != 0) || (tmp->MoverParameters->BrakePress > 0.2) || (tmp->MoverParameters->PipePress < 0.36)) {
						btLampkaStycznB.turn(false);
					} else if (tmp->MoverParameters->BrakePress < 0.1) {
						btLampkaStycznB.turn(true); // mozna prowadzic rozruch
					}

					//-----------------
					// hunter-271211: sygnalizacja poslizgu w pierwszym pojezdzie, gdy
					// wystapi w drugim
					btLampkaPoslizg.turn(tmp->MoverParameters->SlippingWheels);
					//-----------------

					btLampkaSprezarkaB.turn(tmp->MoverParameters->CompressorFlag); // mutopsitka dziala
					btLampkaSprezarkaBOff.turn(false == tmp->MoverParameters->CompressorFlag);
					if ((tmp->MoverParameters->BrakePress >= 0.145f * 10) && (mvControlled->Battery == true) && (mvControlled->Signalling == true)) {
						btLampkaHamowanie2zes.turn(true);
					}
					if ((tmp->MoverParameters->BrakePress < 0.075f * 10) || (mvControlled->Battery == false) || (mvControlled->Signalling == false)) {
						btLampkaHamowanie2zes.turn(false);
					}
					btLampkaNadmPrzetwB.turn(tmp->MoverParameters->ConvOvldFlag); // nadmiarowy przetwornicy?
					btLampkaPrzetwB.turn(tmp->MoverParameters->ConverterFlag); // zalaczenie przetwornicy
					btLampkaPrzetwBOff.turn(false == tmp->MoverParameters->ConverterFlag);
					btLampkaMalfunctionB.turn(tmp->MoverParameters->dizel_heat.PA);
				} else // wylaczone
				{
					btLampkaWylSzybkiB.turn(false);
					btLampkaWylSzybkiBOff.turn(false);
					btLampkaOporyB.turn(false);
					btLampkaStycznB.turn(false);
					btLampkaSprezarkaB.turn(false);
					btLampkaSprezarkaBOff.turn(false);
					btLampkaBezoporowaB.turn(false);
					btLampkaHamowanie2zes.turn(false);
					btLampkaNadmPrzetwB.turn(false);
					btLampkaPrzetwB.turn(false);
					btLampkaPrzetwBOff.turn(false);
					btLampkaMalfunctionB.turn(false);
				}
		}

		if (mvControlled->Battery || mvControlled->ConverterFlag) {
			switch (mvControlled->TrainType) {
				// zależnie od typu lokomotywy
				case dt_EZT: {
					btLampkaHamienie.turn((mvControlled->BrakePress >= 0.2) && mvControlled->Signalling);
					break;
				}
				case dt_ET41: {
					// odhamowanie drugiego członu
					if (mvSecond) {
						// bo może komuś przyjść do głowy jeżdżenie jednym członem
						btLampkaHamienie.turn(mvSecond->BrakePress < 0.4);
					}
					break;
				}
				default: {
					btLampkaHamienie.turn((mvOccupied->BrakePress >= 0.1) || mvControlled->DynamicBrakeFlag);
					btLampkaBrakingOff.turn((mvOccupied->BrakePress < 0.1) && (false == mvControlled->DynamicBrakeFlag));
					break;
				}
			}
			// KURS90
			btLampkaMaxSila.turn(abs(mvControlled->Im) >= 350);
			btLampkaPrzekrMaxSila.turn(abs(mvControlled->Im) >= 450);
			btLampkaRadio.turn(mvOccupied->Radio);
			btLampkaRadioStop.turn(mvOccupied->Radio && mvOccupied->RadioStopFlag);
			btLampkaHamulecReczny.turn(mvOccupied->ManualBrakePos > 0);
			// NBMX wrzesien 2003 - drzwi oraz sygnał odjazdu
			btLampkaDoorLeft.turn(mvOccupied->DoorLeftOpened);
			btLampkaDoorRight.turn(mvOccupied->DoorRightOpened);
			btLampkaDepartureSignal.turn(mvControlled->DepartureSignal);
			btLampkaNapNastHam.turn((mvControlled->ActiveDir != 0) && (mvOccupied->EpFuse)); // napiecie na nastawniku hamulcowym
			btLampkaForward.turn(mvControlled->ActiveDir > 0); // jazda do przodu
			btLampkaBackward.turn(mvControlled->ActiveDir < 0); // jazda do tyłu
			btLampkaED.turn(mvControlled->DynamicBrakeFlag); // hamulec ED
			btLampkaBrakeProfileG.turn(TestFlag(mvOccupied->BrakeDelayFlag, bdelay_G));
			btLampkaBrakeProfileP.turn(TestFlag(mvOccupied->BrakeDelayFlag, bdelay_P));
			btLampkaBrakeProfileR.turn(TestFlag(mvOccupied->BrakeDelayFlag, bdelay_R));
			// light indicators
			// NOTE: sides are hardcoded to deal with setups where single cab is equipped with all indicators
			btLampkaUpperLight.turn((mvOccupied->iLights[side::front] & light::headlight_upper) != 0);
			btLampkaLeftLight.turn((mvOccupied->iLights[side::front] & light::headlight_left) != 0);
			btLampkaRightLight.turn((mvOccupied->iLights[side::front] & light::headlight_right) != 0);
			btLampkaLeftEndLight.turn((mvOccupied->iLights[side::front] & light::redmarker_left) != 0);
			btLampkaRightEndLight.turn((mvOccupied->iLights[side::front] & light::redmarker_right) != 0);
			btLampkaRearUpperLight.turn((mvOccupied->iLights[side::rear] & light::headlight_upper) != 0);
			btLampkaRearLeftLight.turn((mvOccupied->iLights[side::rear] & light::headlight_left) != 0);
			btLampkaRearRightLight.turn((mvOccupied->iLights[side::rear] & light::headlight_right) != 0);
			btLampkaRearLeftEndLight.turn((mvOccupied->iLights[side::rear] & light::redmarker_left) != 0);
			btLampkaRearRightEndLight.turn((mvOccupied->iLights[side::rear] & light::redmarker_right) != 0);
			// others
			btLampkaMalfunction.turn(mvControlled->dizel_heat.PA);
			btLampkaMotorBlowers.turn(mvControlled->RventRot > 0.1);
		} else { // gdy bateria wyłączona
			btLampkaHamienie.turn(false);
			btLampkaBrakingOff.turn(false);
			btLampkaBrakeProfileG.turn(false);
			btLampkaBrakeProfileP.turn(false);
			btLampkaBrakeProfileR.turn(false);
			btLampkaMaxSila.turn(false);
			btLampkaPrzekrMaxSila.turn(false);
			btLampkaRadio.turn(false);
			btLampkaRadioStop.turn(false);
			btLampkaHamulecReczny.turn(false);
			btLampkaDoorLeft.turn(false);
			btLampkaDoorRight.turn(false);
			btLampkaDepartureSignal.turn(false);
			btLampkaNapNastHam.turn(false);
			btLampkaForward.turn(false);
			btLampkaBackward.turn(false);
			btLampkaED.turn(false);
			// light indicators
			btLampkaUpperLight.turn(false);
			btLampkaLeftLight.turn(false);
			btLampkaRightLight.turn(false);
			btLampkaLeftEndLight.turn(false);
			btLampkaRightEndLight.turn(false);
			btLampkaRearUpperLight.turn(false);
			btLampkaRearLeftLight.turn(false);
			btLampkaRearRightLight.turn(false);
			btLampkaRearLeftEndLight.turn(false);
			btLampkaRearRightEndLight.turn(false);
			// others
			btLampkaMalfunction.turn(false);
			btLampkaMotorBlowers.turn(false);
		}

		// McZapkie-080602: obroty (albo translacje) regulatorow
		if (ggMainCtrl.SubModel) {
			if (mvControlled->CoupledCtrl) {
				ggMainCtrl.UpdateValue(double(mvControlled->MainCtrlPos + mvControlled->ScndCtrlPos), dsbNastawnikJazdy);
			} else {
				ggMainCtrl.UpdateValue(double(mvControlled->MainCtrlPos), dsbNastawnikJazdy);
			}
			ggMainCtrl.Update();
		}
		if (ggMainCtrlAct.SubModel) {
			if (mvControlled->CoupledCtrl)
				ggMainCtrlAct.UpdateValue(double(mvControlled->MainCtrlActualPos + mvControlled->ScndCtrlActualPos));
			else
				ggMainCtrlAct.UpdateValue(double(mvControlled->MainCtrlActualPos));
			ggMainCtrlAct.Update();
		}
		if (ggScndCtrl.SubModel) {
			// Ra: od byte odejmowane boolean i konwertowane potem na double?
			ggScndCtrl.UpdateValue(double(mvControlled->ScndCtrlPos - ((mvControlled->TrainType == dt_ET42) && mvControlled->DynamicBrakeFlag)), dsbNastawnikBocz);
			ggScndCtrl.Update();
		}
		if (ggDirKey.SubModel) {
			if (mvControlled->TrainType != dt_EZT)
				ggDirKey.UpdateValue(double(mvControlled->ActiveDir), dsbReverserKey);
			else
				ggDirKey.UpdateValue(double(mvControlled->ActiveDir) + double(mvControlled->Imin == mvControlled->IminHi), dsbReverserKey);
			ggDirKey.Update();
		}
		if (ggBrakeCtrl.SubModel) {
#ifdef _WIN32
			if (DynamicObject->Mechanik ? (DynamicObject->Mechanik->AIControllFlag ? false : (Global.iFeedbackMode == 4 /*|| (Global.bMWDmasterEnable && Global.bMWDBreakEnable)*/)) :
			                              false) // nie blokujemy AI
			{ // Ra: nie najlepsze miejsce, ale na początek gdzieś to dać trzeba
				// Firleju: dlatego kasujemy i zastepujemy funkcją w Console
				if (mvOccupied->BrakeHandle == FV4a) {
					double b = Console::AnalogCalibrateGet(0);
					b = b * 8.0 - 2.0;
					b = clamp<double>(b, -2.0, mvOccupied->BrakeCtrlPosNo); // przycięcie zmiennej do granic
					ggBrakeCtrl.UpdateValue(b); // przesów bez zaokrąglenia
					mvOccupied->BrakeLevelSet(b);
				}
				if (mvOccupied->BrakeHandle == FVel6) // może można usunąć ograniczenie do FV4a i FVel6?
				{
					double b = Console::AnalogCalibrateGet(0);
					b = b * 7.0 - 1.0;
					b = clamp<double>(b, -1.0, mvOccupied->BrakeCtrlPosNo); // przycięcie zmiennej do granic
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
		if (ggLocalBrake.SubModel) {
#ifdef _WIN32
			if ((DynamicObject->Mechanik != nullptr) && (false == DynamicObject->Mechanik->AIControllFlag) // nie blokujemy AI
			    && (mvOccupied->BrakeLocHandle == FD1) &&
			    ((Global.iFeedbackMode == 4)
			    /*|| ( Global.bMWDmasterEnable && Global.bMWDBreakEnable )*/)) {
				// Ra: nie najlepsze miejsce, ale na początek gdzieś to dać trzeba
				// Firleju: dlatego kasujemy i zastepujemy funkcją w Console
				auto const b = clamp<double>(Console::AnalogCalibrateGet(1) * 10.0, 0.0, ManualBrakePosNo);
				ggLocalBrake.UpdateValue(b); // przesów bez zaokrąglenia
				mvOccupied->LocalBrakePos = int(1.09 * b); // sposób zaokrąglania jest do ustalenia
			} else
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

		//---------
		// hunter-080812: poprawka na ogrzewanie w elektrykach - usuniete uzaleznienie od przetwornicy
		if ((mvControlled->Heating == true) && (mvControlled->Mains == true) && (mvControlled->ConvOvldFlag == false))
			btLampkaOgrzewanieSkladu.turn(true);
		else
			btLampkaOgrzewanieSkladu.turn(false);

		//----------

		// przelaczniki
		// ABu030405 obsluga lampki uniwersalnej:
		if (btInstrumentLight.active()) // w ogóle jest
			if (InstrumentLightActive) // załączona
				switch (InstrumentLightType) {
					case 0:
						btInstrumentLight.turn(mvControlled->Battery || mvControlled->ConverterFlag);
						break;
					case 1:
						btInstrumentLight.turn(mvControlled->Mains);
						break;
					case 2:
						btInstrumentLight.turn(mvControlled->ConverterFlag);
						break;
					default:
						btInstrumentLight.turn(false);
				}
			else
				btInstrumentLight.turn(false);

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
		ggRadioChannelSelector.Update();
		ggRadioChannelPrevious.Update();
		ggRadioChannelNext.Update();
		ggRadioStop.Update();
		ggRadioTest.Update();
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
		ggWhistleButton.Update();
		for (auto& universal : ggUniversals) {
			universal.Update();
		}
		// hunter-091012
		ggInstrumentLightButton.Update();
		ggCabLightButton.Update();
		ggCabLightDimButton.Update();
		ggBatteryButton.Update();

		ggWaterPumpBreakerButton.Update();
		ggWaterPumpButton.Update();
		ggWaterHeaterBreakerButton.Update();
		ggWaterHeaterButton.Update();
		ggWaterCircuitsLinkButton.Update();
		ggFuelPumpButton.Update();
		ggOilPumpButton.Update();
		//------
		pyScreens.update();
	}
	// wyprowadzenie sygnałów dla haslera na PoKeys (zaznaczanie na taśmie)
	btHaslerBrakes.turn(DynamicObject->MoverParameters->BrakePress > 0.4); // ciśnienie w cylindrach
	btHaslerCurrent.turn(DynamicObject->MoverParameters->Im != 0.0); // prąd na silnikach

	// calculate current level of interior illumination
	// TODO: organize it along with rest of train update in a more sensible arrangement
	switch (iCabLightFlag) // Ra: uzeleżnic od napięcia w obwodzie sterowania
	{ // hunter-091012: uzaleznienie jasnosci od przetwornicy
		case 0: {
			//światło wewnętrzne zgaszone
			DynamicObject->InteriorLightLevel = 0.0f;
			break;
		}
		case 1: {
			//światło wewnętrzne przygaszone (255 216 176)
			if (mvOccupied->ConverterFlag == true) {
				// jasnosc dla zalaczonej przetwornicy
				DynamicObject->InteriorLightLevel = 0.4f;
			} else {
				DynamicObject->InteriorLightLevel = 0.2f;
			}
			break;
		}
		case 2: {
			//światło wewnętrzne zapalone (255 216 176)
			if (mvOccupied->ConverterFlag == true) {
				// jasnosc dla zalaczonej przetwornicy
				DynamicObject->InteriorLightLevel = 1.0f;
			} else {
				DynamicObject->InteriorLightLevel = 0.5f;
			}
			break;
		}
	}

	// anti slip system activation, maintained while the control button is down
	if (mvOccupied->BrakeSystem != ElectroPneumatic) {
		if (ggAntiSlipButton.GetDesiredValue() > 0.95) {
			mvControlled->AntiSlippingBrake();
		}
	}

	// NOTE: crude way to have the pantographs go back up if they're dropped due to insufficient pressure etc
	// TODO: rework it into something more elegant, when redoing the whole consist/unit/cab etc arrangement
	if (DynamicObject->Mechanik != nullptr && !DynamicObject->Mechanik->AIControllFlag) {
		// don't mess with the ai driving, at least not while switches don't follow ai-set vehicle state
		if ((mvControlled->Battery) || (mvControlled->ConverterFlag)) {
			if (ggPantAllDownButton.GetDesiredValue() < 0.05) {
				// the 'lower all' button overrides state of switches, while active itself
				if ((false == mvControlled->PantFrontUp) && (ggPantFrontButton.GetDesiredValue() >= 0.95)) {
					mvControlled->PantFront(true);
				}
				if ((false == mvControlled->PantRearUp) && (ggPantRearButton.GetDesiredValue() >= 0.95)) {
					mvControlled->PantRear(true);
				}
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
	update_sounds(Deltatime);

	if (false == m_radiomessages.empty()) {
		// erase completed radio messages from the list
		m_radiomessages.erase(
		std::remove_if(std::begin(m_radiomessages), std::end(m_radiomessages), [](sound_source const& source) { return (false == source.is_playing()); }), std::end(m_radiomessages));
	}

	return true; //(DynamicObject->Update(dt));
} // koniec update

void TTrain::update_sounds(double const Deltatime)
{
	if (Deltatime == 0.0) {
		return;
	}

	double volume{ 0.0 };
	double const brakevolumescale{ 0.5 };

	// Winger-160404 - syczenie pomocniczego (luzowanie)
	if (m_lastlocalbrakepressure != -1.f) {
		// calculate rate of pressure drop in local brake cylinder, once it's been initialized
		auto const brakepressuredifference{ mvOccupied->LocBrakePress - m_lastlocalbrakepressure };
		m_localbrakepressurechange = interpolate<float>(m_localbrakepressurechange, 10 * (brakepressuredifference / Deltatime), 0.1f);
	}
	m_lastlocalbrakepressure = mvOccupied->LocBrakePress;
	// local brake, release
	if ((m_localbrakepressurechange < -0.05f) && (mvOccupied->LocBrakePress > mvOccupied->BrakePress - 0.05)) {
		rsSBHiss.gain(clamp(rsSBHiss.m_amplitudeoffset + rsSBHiss.m_amplitudefactor * -m_localbrakepressurechange * 0.05, 0.0, 1.5)).play(sound_flags::exclusive | sound_flags::looping);
	} else {
		// don't stop the sound too abruptly
		volume = std::max(0.0, rsSBHiss.gain() - 0.1 * Deltatime);
		rsSBHiss.gain(volume);
		if (volume < 0.05) {
			rsSBHiss.stop();
		}
	}
	// local brake, engage
	if (m_localbrakepressurechange > 0.05f) {
		rsSBHissU.gain(clamp(rsSBHissU.m_amplitudeoffset + rsSBHissU.m_amplitudefactor * m_localbrakepressurechange * 0.05, 0.0, 1.5)).play(sound_flags::exclusive | sound_flags::looping);
	} else {
		// don't stop the sound too abruptly
		volume = std::max(0.0, rsSBHissU.gain() - 0.1 * Deltatime);
		rsSBHissU.gain(volume);
		if (volume < 0.05) {
			rsSBHissU.stop();
		}
	}

	// McZapkie-280302 - syczenie
	// TODO: softer volume reduction than plain abrupt stop, perhaps as reusable wrapper?
	if ((mvOccupied->BrakeHandle == FV4a) || (mvOccupied->BrakeHandle == FVel6)) {
		// upuszczanie z PG
		fPPress = interpolate(fPPress, static_cast<float>(mvOccupied->Handle->GetSound(s_fv4a_b)), 0.05f);
		volume = (fPPress > 0 ? rsHiss.m_amplitudefactor * fPPress * 0.25 : 0);
		if (volume * brakevolumescale > 0.05) {
			rsHiss.gain(volume * brakevolumescale).play(sound_flags::exclusive | sound_flags::looping);
		} else {
			rsHiss.stop();
		}
		// napelnianie PG
		fNPress = interpolate(fNPress, static_cast<float>(mvOccupied->Handle->GetSound(s_fv4a_u)), 0.25f);
		volume = (fNPress > 0 ? rsHissU.m_amplitudefactor * fNPress : 0);
		if (volume * brakevolumescale > 0.05) {
			rsHissU.gain(volume * brakevolumescale).play(sound_flags::exclusive | sound_flags::looping);
		} else {
			rsHissU.stop();
		}
		// upuszczanie przy naglym
		volume = mvOccupied->Handle->GetSound(s_fv4a_e) * rsHissE.m_amplitudefactor;
		if (volume * brakevolumescale > 0.05) {
			rsHissE.gain(volume * brakevolumescale).play(sound_flags::exclusive | sound_flags::looping);
		} else {
			rsHissE.stop();
		}
		// upuszczanie sterujacego fala
		volume = mvOccupied->Handle->GetSound(s_fv4a_x) * rsHissX.m_amplitudefactor;
		if (volume * brakevolumescale > 0.05) {
			rsHissX.gain(volume * brakevolumescale).play(sound_flags::exclusive | sound_flags::looping);
		} else {
			rsHissX.stop();
		}
		// upuszczanie z czasowego
		volume = mvOccupied->Handle->GetSound(s_fv4a_t) * rsHissT.m_amplitudefactor;
		if (volume * brakevolumescale > 0.05) {
			rsHissT.gain(volume * brakevolumescale).play(sound_flags::exclusive | sound_flags::looping);
		} else {
			rsHissT.stop();
		}

	} else {
		// jesli nie FV4a
		// upuszczanie z PG
		fPPress = (4.0f * fPPress + std::max(mvOccupied->dpLocalValve, mvOccupied->dpMainValve)) / (4.0f + 1.0f);
		volume = (fPPress > 0.0f ? 2.0 * rsHiss.m_amplitudefactor * fPPress : 0.0);
		if (volume > 0.05) {
			rsHiss.gain(volume).play(sound_flags::exclusive | sound_flags::looping);
		} else {
			rsHiss.stop();
		}
		// napelnianie PG
		fNPress = (4.0f * fNPress + Min0R(mvOccupied->dpLocalValve, mvOccupied->dpMainValve)) / (4.0f + 1.0f);
		volume = (fNPress < 0.0f ? -1.0 * rsHissU.m_amplitudefactor * fNPress : 0.0);
		if (volume > 0.01) {
			rsHissU.gain(volume).play(sound_flags::exclusive | sound_flags::looping);
		} else {
			rsHissU.stop();
		}
	} // koniec nie FV4a

	// ambient sound
	// since it's typically ticking of the clock we can center it on tachometer or on middle of compartment bounding area
	rsFadeSound.play(sound_flags::exclusive | sound_flags::looping);

	if (mvControlled->TrainType == dt_181) {
		// alarm przy poslizgu dla 181/182 - BOMBARDIER
		if ((mvControlled->SlippingWheels) && (DynamicObject->GetVelocity() > 1.0)) {
			dsbSlipAlarm.play(sound_flags::exclusive | sound_flags::looping);
		} else {
			dsbSlipAlarm.stop();
		}
	}

	// szum w czasie jazdy
	if ((false == FreeFlyModeFlag) && (false == Global.CabWindowOpen) && (DynamicObject->GetVelocity() > 0.5)) {
		// frequency calculation
		auto const normalizer{ (true == rsRunningNoise.is_combined() ? mvOccupied->Vmax * 0.01f : 1.f) };
		auto const frequency{ rsRunningNoise.m_frequencyoffset + rsRunningNoise.m_frequencyfactor * mvOccupied->Vel * normalizer };

		// volume calculation
		volume = rsRunningNoise.m_amplitudeoffset + rsRunningNoise.m_amplitudefactor * mvOccupied->Vel;
		if (std::abs(mvOccupied->nrot) > 0.01) {
			// hamulce wzmagaja halas
			auto const brakeforceratio{ (
			clamp(mvOccupied->UnitBrakeForce / std::max(1.0, mvOccupied->BrakeForceR(1.0, mvOccupied->Vel) / (mvOccupied->NAxles * std::max(1, mvOccupied->NBpA))), 0.0, 1.0)) };

			volume *= 1 + 0.125 * brakeforceratio;
		}
		// scale volume by track quality
		// TODO: track quality and/or environment factors as separate subroutine
		volume *= interpolate(0.8, 1.2, clamp(DynamicObject->MyTrack->iQualityFlag / 20.0, 0.0, 1.0));
		// for single sample sounds muffle the playback at low speeds
		if (false == rsRunningNoise.is_combined()) {
			volume *= interpolate(0.0, 1.0, clamp(mvOccupied->Vel / 40.0, 0.0, 1.0));
		}

		if (volume > 0.05) {
			rsRunningNoise.pitch(frequency).gain(volume).play(sound_flags::exclusive | sound_flags::looping);
		} else {
			rsRunningNoise.stop();
		}
	} else {
		// don't play the optional ending sound if the listener switches views
		rsRunningNoise.stop(true == FreeFlyModeFlag);
	}

	// McZapkie-141102: SHP i czuwak, TODO: sygnalizacja kabinowa
	if (mvOccupied->SecuritySystem.Status > 0) {
		// hunter-091012: rozdzielenie alarmow
		if (TestFlag(mvOccupied->SecuritySystem.Status, s_CAalarm) || TestFlag(mvOccupied->SecuritySystem.Status, s_SHPalarm)) {
			if (false == dsbBuzzer.is_playing()) {
				dsbBuzzer.play(sound_flags::looping);
#ifdef _WIN32
				Console::BitsSet(1 << 14); // ustawienie bitu 16 na PoKeys
#endif
			}
		} else {
			if (true == dsbBuzzer.is_playing()) {
				dsbBuzzer.stop();
#ifdef _WIN32
				Console::BitsClear(1 << 14); // ustawienie bitu 16 na PoKeys
#endif
			}
		}
	} else {
		// wylaczone
		if (true == dsbBuzzer.is_playing()) {
			dsbBuzzer.stop();
#ifdef _WIN32
			Console::BitsClear(1 << 14); // ustawienie bitu 16 na PoKeys
#endif
		}
	}

	// radiostop
	if ((true == mvOccupied->Radio) && (true == mvOccupied->RadioStopFlag)) {
		m_radiostop.play(sound_flags::exclusive | sound_flags::looping);
	} else {
		m_radiostop.stop();
	}

	if (fTachoCount > 3.f) {
		auto const frequency{ (true == dsbHasler.is_combined() ? fTachoVelocity * 0.01 : 1.0) };
		dsbHasler.pitch(frequency).play(sound_flags::exclusive | sound_flags::looping);
	} else if (fTachoCount < 1.f) {
		dsbHasler.stop();
	}
}

bool TTrain::CabChange(int iDirection)
{ // McZapkie-090902: zmiana kabiny 1->0->2 i z powrotem
	if ((DynamicObject->Mechanik == nullptr) || (true == DynamicObject->Mechanik->AIControllFlag)) {
		// jeśli prowadzi AI albo jest w innym członie
		// jak AI prowadzi, to nie można mu mieszać
		if (abs(DynamicObject->MoverParameters->ActiveCab + iDirection) > 1)
			return false; // ewentualna zmiana pojazdu
		DynamicObject->MoverParameters->ActiveCab = DynamicObject->MoverParameters->ActiveCab + iDirection;
	} else { // jeśli pojazd prowadzony ręcznie albo wcale (wagon)
		DynamicObject->MoverParameters->CabDeactivisation();
		if (DynamicObject->MoverParameters->ChangeCab(iDirection))
			if (InitializeCab(DynamicObject->MoverParameters->ActiveCab,
			    DynamicObject->asBaseDir + DynamicObject->MoverParameters->TypeName + ".mmd")) { // zmiana kabiny w ramach tego samego pojazdu
				DynamicObject->MoverParameters->CabActivisation(); // załączenie rozrządu (wirtualne kabiny)
				DynamicObject->Mechanik->CheckVehicles(Change_direction);
				return true; // udało się zmienić kabinę
			}
		// aktywizacja poprzedniej, bo jeszcze nie wiadomo, czy jakiś pojazd jest
		DynamicObject->MoverParameters->CabActivisation();
	}
	return false; // ewentualna zmiana pojazdu
}

// McZapkie-310302
// wczytywanie pliku z danymi multimedialnymi (dzwieki, kontrolki, kabiny)
bool TTrain::LoadMMediaFile(std::string const& asFileName)
{
	cParser parser(asFileName, cParser::buffer_FILE);
	// NOTE: yaml-style comments are disabled until conflict in use of # is resolved
	// parser.addCommentStyle( "#", "\n" );

	std::string token;
	do {
		token = "";
		parser.getTokens();
		parser >> token;
	} while ((token != "") && (token != "internaldata:"));

	if (token == "internaldata:") {
		do {
			token = "";
			parser.getTokens();
			parser >> token;
			// SEKCJA DZWIEKOW
			if (token == "ctrl:") {
				// nastawnik:
				dsbNastawnikJazdy.deserialize(parser, sound_type::single);
				dsbNastawnikJazdy.owner(DynamicObject);
			} else if (token == "ctrlscnd:") {
				// hunter-081211: nastawnik bocznikowania
				dsbNastawnikBocz.deserialize(parser, sound_type::single);
				dsbNastawnikBocz.owner(DynamicObject);
			} else if (token == "reverserkey:") {
				// hunter-131211: dzwiek kierunkowego
				dsbReverserKey.deserialize(parser, sound_type::single);
				dsbReverserKey.owner(DynamicObject);
			} else if (token == "buzzer:") {
				// bzyczek shp:
				dsbBuzzer.deserialize(parser, sound_type::single);
				dsbBuzzer.owner(DynamicObject);
			} else if (token == "radiostop:") {
				// radiostop
				m_radiostop.deserialize(parser, sound_type::single);
				m_radiostop.owner(DynamicObject);
			} else if (token == "slipalarm:") {
				// Bombardier 011010: alarm przy poslizgu:
				dsbSlipAlarm.deserialize(parser, sound_type::single);
				dsbSlipAlarm.owner(DynamicObject);
			} else if (token == "tachoclock:") {
				// cykanie rejestratora:
				dsbHasler.deserialize(parser, sound_type::single);
				dsbHasler.owner(DynamicObject);
			} else if (token == "switch:") {
				// przelaczniki:
				dsbSwitch.deserialize(parser, sound_type::single);
				dsbSwitch.owner(DynamicObject);
			} else if (token == "pneumaticswitch:") {
				// stycznik EP:
				dsbPneumaticSwitch.deserialize(parser, sound_type::single);
				dsbPneumaticSwitch.owner(DynamicObject);
			} else if (token == "airsound:") {
				// syk:
				rsHiss.deserialize(parser, sound_type::single, sound_parameters::amplitude);
				rsHiss.owner(DynamicObject);
				if (true == rsSBHiss.empty()) {
					// fallback for vehicles without defined local brake hiss sound
					rsSBHiss = rsHiss;
				}
			} else if (token == "airsound2:") {
				// syk:
				rsHissU.deserialize(parser, sound_type::single, sound_parameters::amplitude);
				rsHissU.owner(DynamicObject);
				if (true == rsSBHissU.empty()) {
					// fallback for vehicles without defined local brake hiss sound
					rsSBHissU = rsHissU;
				}
			} else if (token == "airsound3:") {
				// syk:
				rsHissE.deserialize(parser, sound_type::single, sound_parameters::amplitude);
				rsHissE.owner(DynamicObject);
			} else if (token == "airsound4:") {
				// syk:
				rsHissX.deserialize(parser, sound_type::single, sound_parameters::amplitude);
				rsHissX.owner(DynamicObject);
			} else if (token == "airsound5:") {
				// syk:
				rsHissT.deserialize(parser, sound_type::single, sound_parameters::amplitude);
				rsHissT.owner(DynamicObject);
			} else if (token == "localbrakesound:") {
				// syk:
				rsSBHiss.deserialize(parser, sound_type::single, sound_parameters::amplitude);
				rsSBHiss.owner(DynamicObject);
			} else if (token == "localbrakesound2:") {
				// syk:
				rsSBHissU.deserialize(parser, sound_type::single, sound_parameters::amplitude);
				rsSBHissU.owner(DynamicObject);
			} else if (token == "fadesound:") {
				// ambient sound:
				rsFadeSound.deserialize(parser, sound_type::single);
				rsFadeSound.owner(DynamicObject);
			} else if (token == "runningnoise:") {
				// szum podczas jazdy:
				rsRunningNoise.deserialize(parser, sound_type::single, sound_parameters::amplitude | sound_parameters::frequency, mvOccupied->Vmax);
				rsRunningNoise.owner(DynamicObject);

				rsRunningNoise.m_amplitudefactor /= (1 + mvOccupied->Vmax);
				rsRunningNoise.m_frequencyfactor /= (1 + mvOccupied->Vmax);
			} else if (token == "mechspring:") {
				// parametry bujania kamery:
				double ks, kd;
				parser.getTokens(2, false);
				parser >> ks >> kd;
				MechSpring.Init(ks, kd);
				parser.getTokens(6, false);
				parser >> fMechSpringX >> fMechSpringY >> fMechSpringZ >> fMechMaxSpring >> fMechRoll >> fMechPitch;
			} else if (token == "enginespring:") {
				parser.getTokens(5, false);
				parser >> EngineShake.scale >> EngineShake.fadein_offset >> EngineShake.fadein_factor >> EngineShake.fadeout_offset >> EngineShake.fadeout_factor;
				// offsets values are provided as rpm for convenience
				EngineShake.fadein_offset /= 60.f;
				EngineShake.fadeout_offset /= 60.f;
			} else if (token == "huntingspring:") {
				parser.getTokens(4, false);
				parser >> HuntingShake.scale >> HuntingShake.frequency >> HuntingShake.fadein_begin >> HuntingShake.fadein_end;
			}

		} while (token != "");
	} else {
		return false;
	} // nie znalazl sekcji internal

	if (!InitializeCab(mvOccupied->ActiveCab, asFileName)) {
		// zle zainicjowana kabina
		return false;
	} else {
		if (DynamicObject->Controller == Humandriver) {
			// McZapkie-030303: mozliwosc wyswietlania kabiny, w przyszlosci dac opcje w mmd
			DynamicObject->bDisplayCab = true;
		}
		return true;
	}
}

bool TTrain::InitializeCab(int NewCabNo, std::string const& asFileName)
{
	m_controlmapper.clear();
	// reset sound positions and owner
	auto const nullvector{ glm::vec3() };
	std::vector<sound_source*> sounds = { &dsbReverserKey, &dsbNastawnikJazdy, &dsbNastawnikBocz, &dsbSwitch, &dsbPneumaticSwitch, &rsHiss, &rsHissU, &rsHissE, &rsHissX, &rsHissT, &rsSBHiss,
		&rsSBHissU, &rsFadeSound, &rsRunningNoise, &dsbHasler, &dsbBuzzer, &dsbSlipAlarm, &m_radiosound, &m_radiostop };
	for (auto sound : sounds) {
		sound->offset(nullvector);
		sound->owner(DynamicObject);
	}
	// reset view angles
	pMechViewAngle = { 0.0, 0.0 };
	Global.pCamera->Pitch = pMechViewAngle.x;
	Global.pCamera->Yaw = pMechViewAngle.y;

	pyScreens.reset(this);
	pyScreens.setLookupPath(DynamicObject->asBaseDir);
	bool parse = false;
	int cabindex = 0;
	DynamicObject->mdKabina = NULL; // likwidacja wskaźnika na dotychczasową kabinę
	switch (NewCabNo) { // ustalenie numeru kabiny do wczytania
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
	do {
		// szukanie kabiny
		token = "";
		parser->getTokens();
		*parser >> token;

	} while ((token != "") && (token != cabstr));

	if ((cabindex != 1) && (token != cabstr)) {
		// jeśli nie znaleziony wpis kabiny, próba szukania kabiny 1
		cabstr = "cab1definition:";
		// crude way to start parsing from beginning
		parser = std::make_shared<cParser>(asFileName, cParser::buffer_FILE);
		// NOTE: yaml-style comments are disabled until conflict in use of # is resolved
		// parser.addCommentStyle( "#", "\n" );
		do {
			token = "";
			parser->getTokens();
			*parser >> token;
		} while ((token != "") && (token != cabstr));
	}
	if (token == cabstr) {
		// jeśli znaleziony wpis kabiny
		Cabine[cabindex].Load(*parser);
		// NOTE: the position and angle definitions depend on strict entry order
		// TODO: refactor into more flexible arrangement
		parser->getTokens();
		*parser >> token;
		if (token == std::string("driver" + std::to_string(cabindex) + "angle:")) {
			// camera view angle
			parser->getTokens(2, false);
			// angle is specified in degrees but internally stored in radians
			glm::vec2 viewangle;
			*parser >> viewangle.y // yaw first, then pitch
			>> viewangle.x;
			pMechViewAngle = glm::radians(viewangle);
			Global.pCamera->Pitch = pMechViewAngle.x;
			Global.pCamera->Yaw = pMechViewAngle.y;

			parser->getTokens();
			*parser >> token;
		}
		if (token == std::string("driver" + std::to_string(cabindex) + "pos:")) {
			// pozycja poczatkowa maszynisty
			parser->getTokens(3, false);
			*parser >> pMechOffset.x >> pMechOffset.y >> pMechOffset.z;
			pMechSittingPosition = pMechOffset;

			parser->getTokens();
			*parser >> token;
		}
		// ABu: pozycja siedzaca mechanika
		if (token == std::string("driver" + std::to_string(cabindex) + "sitpos:")) {
			// ABu 180404 pozycja siedzaca maszynisty
			parser->getTokens(3, false);
			*parser >> pMechSittingPosition.x >> pMechSittingPosition.y >> pMechSittingPosition.z;

			parser->getTokens();
			*parser >> token;
		}
		// else parse=false;
		do {
			if (parse == true) {
				token = "";
				parser->getTokens();
				*parser >> token;
			} else {
				parse = true;
			}
			// inicjacja kabiny
			// Ra 2014-08: zmieniamy zasady - zamiast przypisywać submodel do
			// istniejących obiektów animujących
			// będziemy teraz uaktywniać obiekty animujące z tablicy i podawać im
			// submodel oraz wskaźnik na parametr
			if (token == std::string("cab" + std::to_string(cabindex) + "model:")) {
				// model kabiny
				parser->getTokens();
				*parser >> token;
				std::replace(token.begin(), token.end(), '\\', '/');
				if (token != "none") {
					// bieżąca sciezka do tekstur to dynamic/...
					Global.asCurrentTexturePath = DynamicObject->asBaseDir;
					// szukaj kabinę jako oddzielny model
					// name can contain leading slash, erase it to avoid creation of double slashes when the name is combined with current directory
					replace_slashes(token);
					if (token[0] == '/') {
						token.erase(0, 1);
					}
					TModel3d* kabina = TModelsManager::GetModel(DynamicObject->asBaseDir + token, true);
					// z powrotem defaultowa sciezka do tekstur
					Global.asCurrentTexturePath = szTexturePath;
					// if (DynamicObject->mdKabina!=k)
					if (kabina != nullptr) {
						DynamicObject->mdKabina = kabina; // nowa kabina
					}
					//(mdKabina) może zostać to samo po przejściu do innego członu bez
					// zmiany kabiny, przy powrocie musi być wiązanie ponowne
					// else
					// break; //wyjście z pętli, bo model zostaje bez zmian
				} else if (cabindex == 1) {
					// model tylko, gdy nie ma kabiny 1
					// McZapkie-170103: szukaj elementy kabiny w glownym modelu
					DynamicObject->mdKabina = DynamicObject->mdModel;
				}
				clear_cab_controls();
			}
			if (nullptr == DynamicObject->mdKabina) {
				// don't bother with other parts until the cab is initialised
				continue;
			} else if (true == initialize_gauge(*parser, token, cabindex)) {
				// matched the token, grab the next one
				continue;
			} else if (true == initialize_button(*parser, token, cabindex)) {
				// matched the token, grab the next one
				continue;
			} else if (token == "pyscreen:") {
				pyScreens.init(*parser, DynamicObject->mdKabina, DynamicObject->name(), NewCabNo);
			}
			// btLampkaUnknown.Init("unknown",mdKabina,false);
		} while (token != "");
	} else {
		return false;
	}
	pyScreens.start();
	if (DynamicObject->mdKabina) {
		// configure placement of sound emitters which aren't bound with any device model, and weren't placed manually
		// try first to bind sounds to location of possible devices
		if (dsbReverserKey.offset() == nullvector) {
			dsbReverserKey.offset(ggDirKey.model_offset());
		}
		if (dsbNastawnikJazdy.offset() == nullvector) {
			dsbNastawnikJazdy.offset(ggMainCtrl.model_offset());
		}
		if (dsbNastawnikBocz.offset() == nullvector) {
			dsbNastawnikBocz.offset(ggScndCtrl.model_offset());
		}
		if (dsbBuzzer.offset() == nullvector) {
			dsbBuzzer.offset(btLampkaCzuwaka.modelOffset());
		}
		// radio has two potential items which can provide the position
		if (m_radiosound.offset() == nullvector) {
			m_radiosound.offset(ggRadioButton.model_offset());
		}
		if (m_radiosound.offset() == nullvector) {
			m_radiosound.offset(btLampkaRadio.modelOffset());
		}
		if (m_radiostop.offset() == nullvector) {
			m_radiostop.offset(m_radiosound.offset());
		}
		auto const localbrakeoffset{ ggLocalBrake.model_offset() };
		std::vector<sound_source*> localbrakesounds = { &rsSBHiss, &rsSBHissU };
		for (auto sound : localbrakesounds) {
			if (sound->offset() == nullvector) {
				sound->offset(localbrakeoffset);
			}
		}
		// NOTE: if the local brake model can't be located the emitter will also be assigned location of main brake
		auto const brakeoffset{ ggBrakeCtrl.model_offset() };
		std::vector<sound_source*> brakesounds = {
			&rsHiss,
			&rsHissU,
			&rsHissE,
			&rsHissX,
			&rsHissT,
			&rsSBHiss,
			&rsSBHissU,
		};
		for (auto sound : brakesounds) {
			if (sound->offset() == nullvector) {
				sound->offset(brakeoffset);
			}
		}
		// for whatever is left fallback on generic location, centre of the cab
		auto const caboffset{ glm::dvec3{ (Cabine[cabindex].CabPos1 + Cabine[cabindex].CabPos2) * 0.5 } + glm::dvec3{ 0, 1, 0 } };
		for (auto sound : sounds) {
			if (sound->offset() == nullvector) {
				sound->offset(caboffset);
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
	pMechPosition = Math3D::vector3(0, 0, 0);
	pMechShake = Math3D::vector3(0, 0, 0);
	vMechMovement = Math3D::vector3(0, 0, 0);
	vMechVelocity = Math3D::vector3(0, 0, 0); // tu zostawały jakieś ułamki, powodujące uciekanie kamery
};

Math3D::vector3 TTrain::MirrorPosition(bool lewe)
{ // zwraca współrzędne widoku kamery z lusterka
	switch (iCabn) {
		case 1: // przednia (1)
			return DynamicObject->mMatrix * Math3D::vector3(lewe ? Cabine[iCabn].CabPos2.x : Cabine[iCabn].CabPos1.x, 1.5 + Cabine[iCabn].CabPos1.y, Cabine[iCabn].CabPos2.z);
		case 2: // tylna (-1)
			return DynamicObject->mMatrix * Math3D::vector3(lewe ? Cabine[iCabn].CabPos1.x : Cabine[iCabn].CabPos2.x, 1.5 + Cabine[iCabn].CabPos1.y, Cabine[iCabn].CabPos1.z);
	}
	return DynamicObject->GetPosition(); // współrzędne środka pojazdu
};

void TTrain::DynamicSet(TDynamicObject* d)
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
		if (DynamicObject->NextConnected ? mvControlled->Couplers[1].AllowedFlag & ctrain_depot : false) { // gdy jest człon od sprzęgu 1, a sprzęg łączony
			// warsztatowo (powiedzmy)
			if ((mvControlled->Power < 1.0) && (mvControlled->Couplers[1].Connected->Power > 1.0)) // my nie mamy mocy, ale ten drugi ma
				mvControlled = DynamicObject->NextConnected->MoverParameters; // będziemy sterować tym z mocą
		} else if (DynamicObject->PrevConnected ? mvControlled->Couplers[0].AllowedFlag & ctrain_depot : false) { // gdy jest człon od sprzęgu 0, a sprzęg łączony
			// warsztatowo (powiedzmy)
			if ((mvControlled->Power < 1.0) && (mvControlled->Couplers[0].Connected->Power > 1.0)) // my nie mamy mocy, ale ten drugi ma
				mvControlled = DynamicObject->PrevConnected->MoverParameters; // będziemy sterować tym z mocą
		}
	mvSecond = NULL; // gdyby się nic nie znalazło
	if (mvOccupied->Power > 1.0) // dwuczłonowe lub ukrotnienia, żeby nie szukać każdorazowo
		if (mvOccupied->Couplers[1].Connected ? mvOccupied->Couplers[1].AllowedFlag & ctrain_controll : false) { // gdy jest człon od sprzęgu 1, a sprzęg łączony
			// warsztatowo (powiedzmy)
			if (mvOccupied->Couplers[1].Connected->Power > 1.0) // ten drugi ma moc
				mvSecond = (TMoverParameters*)mvOccupied->Couplers[1].Connected; // wskaźnik na drugiego
		} else if (mvOccupied->Couplers[0].Connected ? mvOccupied->Couplers[0].AllowedFlag & ctrain_controll : false) { // gdy jest człon od sprzęgu 0, a sprzęg łączony
			// warsztatowo (powiedzmy)
			if (mvOccupied->Couplers[0].Connected->Power > 1.0) // ale ten drugi ma moc
				mvSecond = (TMoverParameters*)mvOccupied->Couplers[0].Connected; // wskaźnik na drugiego
		}
};

void TTrain::radio_message(sound_source* Message, int const Channel)
{
	if ((false == mvOccupied->Radio) || (iRadioChannel != Channel)) {
		// skip message playback if the radio isn't able to receive it
		return;
	}
	auto const soundrange{ Message->range() };
	if ((soundrange > 0) && (glm::length2(Message->location() - glm::dvec3{ DynamicObject->GetPosition() }) > (soundrange * soundrange))) {
		// skip message playback if the receiver is outside of the emitter's range
		return;
	}

	m_radiomessages.emplace_back(m_radiosound);
	// assign sound to the template and play it
	m_radiomessages.back().copy_sounds(*Message).play(sound_flags::exclusive);
}

void TTrain::SetLights()
{
	TDynamicObject* p = DynamicObject->GetFirstDynamic(mvOccupied->ActiveCab < 0 ? 1 : 0, 4);
	bool kier = (DynamicObject->DirectionGet() * mvOccupied->ActiveCab > 0);
	int xs = (kier ? 0 : 1);
	if (kier ? p->NextC(1) : p->PrevC(1)) // jesli jest nastepny, to tylko przod
	{
		p->RaLightsSet(mvOccupied->Lights[xs][mvOccupied->LightsPos - 1] * (1 - xs), mvOccupied->Lights[1 - xs][mvOccupied->LightsPos - 1] * xs);
		p = (kier ? p->NextC(4) : p->PrevC(4));
		while (p) {
			if (kier ? p->NextC(1) : p->PrevC(1)) {
				p->RaLightsSet(0, 0);
			} else {
				p->RaLightsSet(mvOccupied->Lights[xs][mvOccupied->LightsPos - 1] * xs, mvOccupied->Lights[1 - xs][mvOccupied->LightsPos - 1] * (1 - xs));
			}
			p = (kier ? p->NextC(4) : p->PrevC(4));
		}
	} else // calosc
	{
		p->RaLightsSet(mvOccupied->Lights[xs][mvOccupied->LightsPos - 1], mvOccupied->Lights[1 - xs][mvOccupied->LightsPos - 1]);
	}
};

// clears state of all cabin controls
void TTrain::clear_cab_controls()
{
	// indicators exposed to custom control devices
	btLampkaSHP.clear(0);
	btLampkaCzuwaka.clear(1);
	btLampkaOpory.clear(2);
	btLampkaWylSzybki.clear(3);
	btLampkaNadmSil.clear(4);
	btLampkaStyczn.clear(5);
	btLampkaPoslizg.clear(6);
	btLampkaNadmPrzetw.clear(((mvControlled->TrainType & dt_EZT) != 0) ? -1 : 7); // EN57 nie ma tej lampki
	btLampkaPrzetwOff.clear(((mvControlled->TrainType & dt_EZT) != 0) ? 7 : -1); // za to ma tę
	btLampkaNadmSpr.clear(8);
	btLampkaNadmWent.clear(9);
	btLampkaWysRozr.clear(((mvControlled->TrainType & dt_ET22) != 0) ? -1 : 10); // ET22 nie ma tej lampki
	btLampkaOgrzewanieSkladu.clear(11);
	btHaslerBrakes.clear(12); // ciśnienie w cylindrach do odbijania na haslerze
	btHaslerCurrent.clear(13); // prąd na silnikach do odbijania na haslerze
	// Numer 14 jest używany dla buczka SHP w update_sounds()
	// Jeśli ustawiamy nową wartość dla PoKeys wolna jest 15

	// other cab controls
	// TODO: arrange in more readable manner, and eventually refactor
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
	ggWhistleButton.Clear();
	ggNextCurrentButton.Clear();
	for (auto& universal : ggUniversals) {
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
	ggRadioButton.Clear();
	ggRadioChannelSelector.Clear();
	ggRadioChannelPrevious.Clear();
	ggRadioChannelNext.Clear();
	ggRadioStop.Clear();
	ggRadioTest.Clear();
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
	ggOilPressB.Clear();
	ggWater1TempB.Clear();

	ggClockSInd.Clear();
	ggClockMInd.Clear();
	ggClockHInd.Clear();
	ggEngineVoltage.Clear();
	ggLVoltage.Clear();
	ggMainGearStatus.Clear();
	ggIgnitionKey.Clear();

	ggWaterPumpBreakerButton.Clear();
	ggWaterPumpButton.Clear();
	ggWaterHeaterBreakerButton.Clear();
	ggWaterHeaterButton.Clear();
	ggWaterCircuitsLinkButton.Clear();
	ggFuelPumpButton.Clear();
	ggOilPumpButton.Clear();

	btLampkaPrzetw.clear();
	btLampkaPrzetwB.clear();
	btLampkaPrzetwBOff.clear();
	btLampkaPrzekRozn.clear();
	btLampkaPrzekRoznPom.clear();
	btLampkaUkrotnienie.clear();
	btLampkaHamPosp.clear();
	btLampkaWylSzybkiOff.clear();
	btLampkaWylSzybkiB.clear();
	btLampkaWylSzybkiBOff.clear();
	btLampkaBezoporowa.clear();
	btLampkaBezoporowaB.clear();
	btLampkaMaxSila.clear();
	btLampkaPrzekrMaxSila.clear();
	btLampkaRadio.clear();
	btLampkaRadioStop.clear();
	btLampkaHamulecReczny.clear();
	btLampkaBlokadaDrzwi.clear();
	btInstrumentLight.clear();
	btLampkaWentZaluzje.clear();
	btLampkaDoorLeft.clear();
	btLampkaDoorRight.clear();
	btLampkaDepartureSignal.clear();
	btLampkaRezerwa.clear();
	btLampkaBoczniki.clear();
	btLampkaBocznik1.clear();
	btLampkaBocznik2.clear();
	btLampkaBocznik3.clear();
	btLampkaBocznik4.clear();
	btLampkaRadiotelefon.clear();
	btLampkaHamienie.clear();
	btLampkaBrakingOff.clear();
	btLampkaBrakeProfileG.clear();
	btLampkaBrakeProfileP.clear();
	btLampkaBrakeProfileR.clear();
	btLampkaSprezarka.clear();
	btLampkaSprezarkaB.clear();
	btLampkaSprezarkaOff.clear();
	btLampkaSprezarkaBOff.clear();
	btLampkaFuelPumpOff.clear();
	btLampkaNapNastHam.clear();
	btLampkaStycznB.clear();
	btLampkaHamowanie1zes.clear();
	btLampkaHamowanie2zes.clear();
	btLampkaNadmPrzetwB.clear();
	btLampkaForward.clear();
	btLampkaBackward.clear();
	// light indicators
	btLampkaUpperLight.clear();
	btLampkaLeftLight.clear();
	btLampkaRightLight.clear();
	btLampkaLeftEndLight.clear();
	btLampkaRightEndLight.clear();
	btLampkaRearUpperLight.clear();
	btLampkaRearLeftLight.clear();
	btLampkaRearRightLight.clear();
	btLampkaRearLeftEndLight.clear();
	btLampkaRearRightEndLight.clear();
	btCabLight.clear(); // hunter-171012
	// others
	btLampkaMalfunction.clear();
	btLampkaMalfunctionB.clear();
	btLampkaMotorBlowers.clear();

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
}

// NOTE: we can get rid of this function once we have per-cab persistent state
void TTrain::set_cab_controls()
{
	// switches
	// battery
	if (true == mvOccupied->Battery) {
		ggBatteryButton.PutValue(1.0);
	}
	// motor connectors
	ggStLinOffButton.PutValue((mvControlled->StLinSwitchOff ? 1.0 : 0.0));
	// radio
	if (true == mvOccupied->Radio) {
		ggRadioButton.PutValue(1.0);
	}
	ggRadioChannelSelector.PutValue(iRadioChannel - 1);
	// pantographs
	if (mvOccupied->PantSwitchType != "impulse") {
		if (ggPantFrontButton.SubModel) {
			ggPantFrontButton.PutValue((mvControlled->PantFrontUp ? 1.0 : 0.0));
		}
		if (ggPantFrontButtonOff.SubModel) {
			ggPantFrontButtonOff.PutValue((mvControlled->PantFrontUp ? 0.0 : 1.0));
		}
		// NOTE: currently we animate the selectable pantograph control for both pantographs
		// TODO: implement actual selection control, and refactor handling this control setup in a separate method
		if (ggPantSelectedButton.SubModel) {
			ggPantSelectedButton.PutValue((mvControlled->PantFrontUp ? 1.0 : 0.0));
		}
		if (ggPantSelectedDownButton.SubModel) {
			ggPantSelectedDownButton.PutValue((mvControlled->PantFrontUp ? 0.0 : 1.0));
		}
	}
	if (mvOccupied->PantSwitchType != "impulse") {
		if (ggPantRearButton.SubModel) {
			ggPantRearButton.PutValue((mvControlled->PantRearUp ? 1.0 : 0.0));
		}
		if (ggPantRearButtonOff.SubModel) {
			ggPantRearButtonOff.PutValue((mvControlled->PantRearUp ? 0.0 : 1.0));
		}
		// NOTE: currently we animate the selectable pantograph control for both pantographs
		// TODO: implement actual selection control, and refactor handling this control setup in a separate method
		if (ggPantSelectedButton.SubModel) {
			ggPantSelectedButton.PutValue((mvControlled->PantRearUp ? 1.0 : 0.0));
		}
		if (ggPantSelectedDownButton.SubModel) {
			ggPantSelectedDownButton.PutValue((mvControlled->PantRearUp ? 0.0 : 1.0));
		}
	}
	// auxiliary compressor
	ggPantCompressorValve.PutValue(mvControlled->bPantKurek3 ? 0.0 : // default setting is pantographs connected with primary tank
	                               1.0);
	ggPantCompressorButton.PutValue(mvControlled->PantCompFlag ? 1.0 : 0.0);
	// converter
	if (mvOccupied->ConvSwitchType != "impulse") {
		ggConverterButton.PutValue(mvControlled->ConverterAllow ? 1.0 : 0.0);
	}
	ggConverterLocalButton.PutValue(mvControlled->ConverterAllowLocal ? 1.0 : 0.0);
	// compressor
	ggCompressorButton.PutValue(mvControlled->CompressorAllow ? 1.0 : 0.0);
	ggCompressorLocalButton.PutValue(mvControlled->CompressorAllowLocal ? 1.0 : 0.0);
	// motor overload relay threshold / shunt mode
	ggMaxCurrentCtrl.PutValue((true == mvControlled->ShuntModeAllow ? (true == mvControlled->ShuntMode ? 1.0 : 0.0) : (mvControlled->Imax == mvControlled->ImaxHi ? 1.0 : 0.0)));
	// lights
	ggLightsButton.PutValue(mvOccupied->LightsPos - 1);

	int const vehicleside = (mvOccupied->ActiveCab == 1 ? side::front : side::rear);

	if ((DynamicObject->iLights[vehicleside] & light::headlight_left) != 0) {
		ggLeftLightButton.PutValue(1.0);
	}
	if ((DynamicObject->iLights[vehicleside] & light::headlight_right) != 0) {
		ggRightLightButton.PutValue(1.0);
	}
	if ((DynamicObject->iLights[vehicleside] & light::headlight_upper) != 0) {
		ggUpperLightButton.PutValue(1.0);
	}
	if ((DynamicObject->iLights[vehicleside] & light::redmarker_left) != 0) {
		if (ggLeftEndLightButton.SubModel != nullptr) {
			ggLeftEndLightButton.PutValue(1.0);
		} else {
			ggLeftLightButton.PutValue(-1.0);
		}
	}
	if ((DynamicObject->iLights[vehicleside] & light::redmarker_right) != 0) {
		if (ggRightEndLightButton.SubModel != nullptr) {
			ggRightEndLightButton.PutValue(1.0);
		} else {
			ggRightLightButton.PutValue(-1.0);
		}
	}
	if (true == DynamicObject->DimHeadlights) {
		ggDimHeadlightsButton.PutValue(1.0);
	}
	// cab lights
	if (true == bCabLight) {
		ggCabLightButton.PutValue(1.0);
	}
	if (true == bCabLightDim) {
		ggCabLightDimButton.PutValue(1.0);
	}

	ggInstrumentLightButton.PutValue((InstrumentLightActive ? 1.0 : 0.0));
	// doors
	// NOTE: we're relying on the cab models to have switches reversed for the rear cab(?)
	ggDoorLeftButton.PutValue(mvOccupied->DoorLeftOpened ? 1.0 : 0.0);
	ggDoorRightButton.PutValue(mvOccupied->DoorRightOpened ? 1.0 : 0.0);
	// door lock
	if (true == mvControlled->DoorSignalling) {
		ggDoorSignallingButton.PutValue(1.0);
	}
	// heating
	if (true == mvControlled->Heating) {
		ggTrainHeatingButton.PutValue(1.0);
	}
	// brake acting time
	if (ggBrakeProfileCtrl.SubModel != nullptr) {
		ggBrakeProfileCtrl.PutValue(((mvOccupied->BrakeDelayFlag & bdelay_R) != 0 ? 2.0 : mvOccupied->BrakeDelayFlag - 1));
	}
	if (ggBrakeProfileG.SubModel != nullptr) {
		ggBrakeProfileG.PutValue(mvOccupied->BrakeDelayFlag == bdelay_G ? 1.0 : 0.0);
	}
	if (ggBrakeProfileR.SubModel != nullptr) {
		ggBrakeProfileR.PutValue((mvOccupied->BrakeDelayFlag & bdelay_R) != 0 ? 1.0 : 0.0);
	}
	// alarm chain
	ggAlarmChain.PutValue(mvControlled->AlarmChainFlag ? 1.0 : 0.0);
	// brake signalling
	ggSignallingButton.PutValue(mvControlled->Signalling ? 1.0 : 0.0);
	// multiple-unit current indicator source
	ggNextCurrentButton.PutValue(ShowNextCurrent ? 1.0 : 0.0);
	// water pump
	ggWaterPumpBreakerButton.PutValue(mvControlled->WaterPump.breaker ? 1.0 : 0.0);
	ggWaterPumpButton.PutValue(mvControlled->WaterPump.is_enabled ? 1.0 : 0.0);
	// water heater
	ggWaterHeaterBreakerButton.PutValue(mvControlled->WaterHeater.breaker ? 1.0 : 0.0);
	ggWaterHeaterButton.PutValue(mvControlled->WaterHeater.is_enabled ? 1.0 : 0.0);
	ggWaterCircuitsLinkButton.PutValue(mvControlled->WaterCircuitsLink ? 1.0 : 0.0);
	// fuel pump
	ggFuelPumpButton.PutValue(mvControlled->FuelPump.is_enabled ? 1.0 : 0.0);
	// oil pump
	ggOilPumpButton.PutValue(mvControlled->OilPump.is_enabled ? 1.0 : 0.0);

	// we reset all indicators, as they're set during the update pass
	// TODO: when cleaning up break setting indicator state into a separate function, so we can reuse it
}

// initializes a button matching provided label. returns: true if the label was found, false otherwise
// TODO: refactor the cabin controls into some sensible structure
bool TTrain::initialize_button(cParser& Parser, std::string const& Label, int const Cabindex)
{
	std::unordered_map<std::string, TButton&> const lights = { { "i-maxft:", btLampkaMaxSila }, { "i-maxftt:", btLampkaPrzekrMaxSila }, { "i-radio:", btLampkaRadio },
		{ "i-radiostop:", btLampkaRadioStop }, { "i-manual_brake:", btLampkaHamulecReczny }, { "i-door_blocked:", btLampkaBlokadaDrzwi }, { "i-slippery:", btLampkaPoslizg },
		{ "i-contactors:", btLampkaStyczn }, { "i-conv_ovld:", btLampkaNadmPrzetw }, { "i-converter:", btLampkaPrzetw }, { "i-converteroff:", btLampkaPrzetwOff }, { "i-converterb:", btLampkaPrzetwB },
		{ "i-converterboff:", btLampkaPrzetwBOff }, { "i-diff_relay:", btLampkaPrzekRozn }, { "i-diff_relay2:", btLampkaPrzekRoznPom }, { "i-motor_ovld:", btLampkaNadmSil },
		{ "i-train_controll:", btLampkaUkrotnienie }, { "i-brake_delay_r:", btLampkaHamPosp }, { "i-mainbreaker:", btLampkaWylSzybki }, { "i-mainbreakerb:", btLampkaWylSzybkiB },
		{ "i-mainbreakeroff:", btLampkaWylSzybkiOff }, { "i-mainbreakerboff:", btLampkaWylSzybkiBOff }, { "i-vent_ovld:", btLampkaNadmWent }, { "i-comp_ovld:", btLampkaNadmSpr },
		{ "i-resistors:", btLampkaOpory }, { "i-no_resistors:", btLampkaBezoporowa }, { "i-no_resistors_b:", btLampkaBezoporowaB }, { "i-highcurrent:", btLampkaWysRozr },
		{ "i-vent_trim:", btLampkaWentZaluzje }, { "i-motorblowers:", btLampkaMotorBlowers }, { "i-trainheating:", btLampkaOgrzewanieSkladu }, { "i-security_aware:", btLampkaCzuwaka },
		{ "i-security_cabsignal:", btLampkaSHP }, { "i-door_left:", btLampkaDoorLeft }, { "i-door_right:", btLampkaDoorRight }, { "i-departure_signal:", btLampkaDepartureSignal },
		{ "i-reserve:", btLampkaRezerwa }, { "i-scnd:", btLampkaBoczniki }, { "i-scnd1:", btLampkaBocznik1 }, { "i-scnd2:", btLampkaBocznik2 }, { "i-scnd3:", btLampkaBocznik3 },
		{ "i-scnd4:", btLampkaBocznik4 }, { "i-braking:", btLampkaHamienie }, { "i-brakingoff:", btLampkaBrakingOff }, { "i-dynamicbrake:", btLampkaED }, { "i-brakeprofileg:", btLampkaBrakeProfileG },
		{ "i-brakeprofilep:", btLampkaBrakeProfileP }, { "i-brakeprofiler:", btLampkaBrakeProfileR }, { "i-braking-ezt:", btLampkaHamowanie1zes }, { "i-braking-ezt2:", btLampkaHamowanie2zes },
		{ "i-compressor:", btLampkaSprezarka }, { "i-compressorb:", btLampkaSprezarkaB }, { "i-compressoroff:", btLampkaSprezarkaOff }, { "i-compressorboff:", btLampkaSprezarkaBOff },
		{ "i-fuelpumpoff:", btLampkaFuelPumpOff }, { "i-voltbrake:", btLampkaNapNastHam }, { "i-resistorsb:", btLampkaOporyB }, { "i-contactorsb:", btLampkaStycznB },
		{ "i-conv_ovldb:", btLampkaNadmPrzetwB }, { "i-malfunction:", btLampkaMalfunction }, { "i-malfunctionb:", btLampkaMalfunctionB }, { "i-forward:", btLampkaForward },
		{ "i-backward:", btLampkaBackward }, { "i-upperlight:", btLampkaUpperLight }, { "i-leftlight:", btLampkaLeftLight }, { "i-rightlight:", btLampkaRightLight },
		{ "i-leftend:", btLampkaLeftEndLight }, { "i-rightend:", btLampkaRightEndLight }, { "i-rearupperlight:", btLampkaRearUpperLight }, { "i-rearleftlight:", btLampkaRearLeftLight },
		{ "i-rearrightlight:", btLampkaRearRightLight }, { "i-rearleftend:", btLampkaRearLeftEndLight }, { "i-rearrightend:", btLampkaRearRightEndLight }, { "i-cablight:", btCabLight } };
	auto lookup = lights.find(Label);
	if (lookup != lights.end()) {
		lookup->second.load(Parser, DynamicObject, DynamicObject->mdKabina);
	}

	else if (Label == "i-instrumentlight:") {
		btInstrumentLight.load(Parser, DynamicObject, DynamicObject->mdKabina, DynamicObject->mdModel);
		InstrumentLightType = 0;
	} else if (Label == "i-instrumentlight_M:") {
		btInstrumentLight.load(Parser, DynamicObject, DynamicObject->mdKabina, DynamicObject->mdModel);
		InstrumentLightType = 1;
	} else if (Label == "i-instrumentlight_C:") {
		btInstrumentLight.load(Parser, DynamicObject, DynamicObject->mdKabina, DynamicObject->mdModel);
		InstrumentLightType = 2;
	} else if (Label == "i-doors:") {
		int i = Parser.getToken<int>() - 1;
		auto& button = Cabine[Cabindex].Button(-1); // pierwsza wolna lampka
		button.load(Parser, DynamicObject, DynamicObject->mdKabina);
		button.assignBool(bDoors[0] + 3 * i);
	} else {
		// failed to match the label
		return false;
	}

	return true;
}

// initializes a gauge matching provided label. returns: true if the label was found, false otherwise
// TODO: refactor the cabin controls into some sensible structure
bool TTrain::initialize_gauge(cParser& Parser, std::string const& Label, int const Cabindex)
{
	std::unordered_map<std::string, TGauge&> const gauges = { { "mainctrl:", ggMainCtrl }, { "scndctrl:", ggScndCtrl }, { "dirkey:", ggDirKey }, { "brakectrl:", ggBrakeCtrl },
		{ "localbrake:", ggLocalBrake }, { "manualbrake:", ggManualBrake }, { "alarmchain:", ggAlarmChain }, { "brakeprofile_sw:", ggBrakeProfileCtrl }, { "brakeprofileg_sw:", ggBrakeProfileG },
		{ "brakeprofiler_sw:", ggBrakeProfileR }, { "maxcurrent_sw:", ggMaxCurrentCtrl }, { "main_off_bt:", ggMainOffButton }, { "main_on_bt:", ggMainOnButton },
		{ "security_reset_bt:", ggSecurityResetButton }, { "releaser_bt:", ggReleaserButton }, { "sand_bt:", ggSandButton }, { "antislip_bt:", ggAntiSlipButton }, { "horn_bt:", ggHornButton },
		{ "hornlow_bt:", ggHornLowButton }, { "hornhigh_bt:", ggHornHighButton }, { "whistle_bt:", ggWhistleButton }, { "fuse_bt:", ggFuseButton }, { "converterfuse_bt:", ggConverterFuseButton },
		{ "stlinoff_bt:", ggStLinOffButton }, { "door_left_sw:", ggDoorLeftButton }, { "door_right_sw:", ggDoorRightButton }, { "departure_signal_bt:", ggDepartureSignalButton },
		{ "upperlight_sw:", ggUpperLightButton }, { "leftlight_sw:", ggLeftLightButton }, { "rightlight_sw:", ggRightLightButton }, { "dimheadlights_sw:", ggDimHeadlightsButton },
		{ "leftend_sw:", ggLeftEndLightButton }, { "rightend_sw:", ggRightEndLightButton }, { "lights_sw:", ggLightsButton }, { "rearupperlight_sw:", ggRearUpperLightButton },
		{ "rearleftlight_sw:", ggRearLeftLightButton }, { "rearrightlight_sw:", ggRearRightLightButton }, { "rearleftend_sw:", ggRearLeftEndLightButton },
		{ "rearrightend_sw:", ggRearRightEndLightButton }, { "compressor_sw:", ggCompressorButton }, { "compressorlocal_sw:", ggCompressorLocalButton }, { "converter_sw:", ggConverterButton },
		{ "converterlocal_sw:", ggConverterLocalButton }, { "converteroff_sw:", ggConverterOffButton }, { "main_sw:", ggMainButton }, { "waterpumpbreaker_sw:", ggWaterPumpBreakerButton },
		{ "waterpump_sw:", ggWaterPumpButton }, { "waterheaterbreaker_sw:", ggWaterHeaterBreakerButton }, { "waterheater_sw:", ggWaterHeaterButton }, { "water1tempb:", ggWater1TempB },
		{ "watercircuitslink_sw:", ggWaterCircuitsLinkButton }, { "fuelpump_sw:", ggFuelPumpButton }, { "oilpump_sw:", ggOilPumpButton }, { "oilpressb:", ggOilPressB }, { "radio_sw:", ggRadioButton },
		{ "radiochannel_sw:", ggRadioChannelSelector }, { "radiochannelprev_sw:", ggRadioChannelPrevious }, { "radiochannelnext_sw:", ggRadioChannelNext }, { "radiostop_sw:", ggRadioStop },
		{ "radiotest_sw:", ggRadioTest }, { "pantfront_sw:", ggPantFrontButton }, { "pantrear_sw:", ggPantRearButton }, { "pantfrontoff_sw:", ggPantFrontButtonOff },
		{ "pantrearoff_sw:", ggPantRearButtonOff }, { "pantalloff_sw:", ggPantAllDownButton }, { "pantselected_sw:", ggPantSelectedButton }, { "pantselectedoff_sw:", ggPantSelectedDownButton },
		{ "pantcompressor_sw:", ggPantCompressorButton }, { "pantcompressorvalve_sw:", ggPantCompressorValve }, { "trainheating_sw:", ggTrainHeatingButton }, { "signalling_sw:", ggSignallingButton },
		{ "door_signalling_sw:", ggDoorSignallingButton }, { "nextcurrent_sw:", ggNextCurrentButton }, { "instrumentlight_sw:", ggInstrumentLightButton }, { "cablight_sw:", ggCabLightButton },
		{ "cablightdim_sw:", ggCabLightDimButton }, { "battery_sw:", ggBatteryButton }, { "universal0:", ggUniversals[0] }, { "universal1:", ggUniversals[1] }, { "universal2:", ggUniversals[2] },
		{ "universal3:", ggUniversals[3] }, { "universal4:", ggUniversals[4] }, { "universal5:", ggUniversals[5] }, { "universal6:", ggUniversals[6] }, { "universal7:", ggUniversals[7] },
		{ "universal8:", ggUniversals[8] }, { "universal9:", ggUniversals[9] } };
	auto lookup = gauges.find(Label);
	if (lookup != gauges.end()) {
		lookup->second.Load(Parser, DynamicObject, DynamicObject->mdKabina);
		m_controlmapper.insert(lookup->second, lookup->first);
	}
	// ABu 090305: uniwersalne przyciski lub inne rzeczy
	else if (Label == "mainctrlact:") {
		ggMainCtrlAct.Load(Parser, DynamicObject, DynamicObject->mdKabina, DynamicObject->mdModel);
	}
	// SEKCJA WSKAZNIKOW
	else if ((Label == "tachometer:") || (Label == "tachometerb:")) {
		// predkosciomierz wskaźnikowy z szarpaniem
		auto& gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
		gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
		gauge.AssignFloat(&fTachoVelocityJump);
		// bind tachometer sound location to the meter
		if (dsbHasler.offset() == glm::vec3()) {
			dsbHasler.offset(gauge.model_offset());
		}
	} else if (Label == "tachometern:") {
		// predkosciomierz wskaźnikowy bez szarpania
		auto& gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
		gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
		gauge.AssignFloat(&fTachoVelocity);
		// bind tachometer sound location to the meter
		if (dsbHasler.offset() == glm::vec3()) {
			dsbHasler.offset(gauge.model_offset());
		}
	} else if (Label == "tachometerd:") {
		// predkosciomierz cyfrowy
		auto& gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
		gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
		gauge.AssignFloat(&fTachoVelocity);
		// bind tachometer sound location to the meter
		if (dsbHasler.offset() == glm::vec3()) {
			dsbHasler.offset(gauge.model_offset());
		}
	} else if ((Label == "hvcurrent1:") || (Label == "hvcurrent1b:")) {
		// 1szy amperomierz
		auto& gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
		gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
		gauge.AssignFloat(fHCurrent + 1);
	} else if ((Label == "hvcurrent2:") || (Label == "hvcurrent2b:")) {
		// 2gi amperomierz
		auto& gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
		gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
		gauge.AssignFloat(fHCurrent + 2);
	} else if ((Label == "hvcurrent3:") || (Label == "hvcurrent3b:")) {
		// 3ci amperomierz
		auto& gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałska
		gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
		gauge.AssignFloat(fHCurrent + 3);
	} else if ((Label == "hvcurrent:") || (Label == "hvcurrentb:")) {
		// amperomierz calkowitego pradu
		auto& gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
		gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
		gauge.AssignFloat(fHCurrent);
	} else if (Label == "eimscreen:") {
		// amperomierz calkowitego pradu
		int i, j;
		Parser.getTokens(2, false);
		Parser >> i >> j;
		auto& gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
		gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
		gauge.AssignFloat(&fEIMParams[i][j]);
	} else if (Label == "brakes:") {
		// amperomierz calkowitego pradu
		int i, j;
		Parser.getTokens(2, false);
		Parser >> i >> j;
		auto& gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
		gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
		gauge.AssignFloat(&fPress[i - 1][j]);
	} else if ((Label == "brakepress:") || (Label == "brakepressb:")) {
		// manometr cylindrow hamulcowych
		// Ra 2014-08: przeniesione do TCab
		auto& gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
		gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina, nullptr, 0.1);
		gauge.AssignDouble(&mvOccupied->BrakePress);
	} else if ((Label == "pipepress:") || (Label == "pipepressb:")) {
		// manometr przewodu hamulcowego
		auto& gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
		gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina, nullptr, 0.1);
		gauge.AssignDouble(&mvOccupied->PipePress);
	} else if (Label == "limpipepress:") {
		// manometr zbiornika sterujacego zaworu maszynisty
		ggZbS.Load(Parser, DynamicObject, DynamicObject->mdKabina, nullptr, 0.1);
	} else if (Label == "cntrlpress:") {
		// manometr zbiornika kontrolnego/rorzďż˝du
		auto& gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
		gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina, nullptr, 0.1);
		gauge.AssignDouble(&mvControlled->PantPress);
	} else if ((Label == "compressor:") || (Label == "compressorb:")) {
		// manometr sprezarki/zbiornika glownego
		auto& gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
		gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina, nullptr, 0.1);
		gauge.AssignDouble(&mvOccupied->Compressor);
	} else if (Label == "oilpress:") {
		// oil pressure
		auto& gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
		gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina, nullptr);
		gauge.AssignFloat(&mvControlled->OilPump.pressure_present);
	} else if (Label == "oiltemp:") {
		// oil temperature
		auto& gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
		gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina, nullptr);
		gauge.AssignFloat(&mvControlled->dizel_heat.To);
	} else if (Label == "water1temp:") {
		// main circuit water temperature
		auto& gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
		gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina, nullptr);
		gauge.AssignFloat(&mvControlled->dizel_heat.temperatura1);
	} else if (Label == "water2temp:") {
		// auxiliary circuit water temperature
		auto& gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
		gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina, nullptr);
		gauge.AssignFloat(&mvControlled->dizel_heat.temperatura2);
	}
	// yB - dla drugiej sekcji
	else if (Label == "hvbcurrent1:") {
		// 1szy amperomierz
		ggI1B.Load(Parser, DynamicObject, DynamicObject->mdKabina);
	} else if (Label == "hvbcurrent2:") {
		// 2gi amperomierz
		ggI2B.Load(Parser, DynamicObject, DynamicObject->mdKabina);
	} else if (Label == "hvbcurrent3:") {
		// 3ci amperomierz
		ggI3B.Load(Parser, DynamicObject, DynamicObject->mdKabina);
	} else if (Label == "hvbcurrent:") {
		// amperomierz calkowitego pradu
		ggItotalB.Load(Parser, DynamicObject, DynamicObject->mdKabina);
	}
	//*************************************************************
	else if (Label == "clock:") {
		// zegar analogowy
		if (Parser.getToken<std::string>() == "analog") {
			// McZapkie-300302: zegarek
			ggClockSInd.Init(DynamicObject->mdKabina->GetFromName("ClockShand"), gt_Rotate, 1.0 / 60.0);
			ggClockMInd.Init(DynamicObject->mdKabina->GetFromName("ClockMhand"), gt_Rotate, 1.0 / 60.0);
			ggClockHInd.Init(DynamicObject->mdKabina->GetFromName("ClockHhand"), gt_Rotate, 1.0 / 12.0);
		}
	} else if (Label == "evoltage:") {
		// woltomierz napiecia silnikow
		ggEngineVoltage.Load(Parser, DynamicObject, DynamicObject->mdKabina);
	} else if (Label == "hvoltage:") {
		// woltomierz wysokiego napiecia
		auto& gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
		gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
		gauge.AssignFloat(&fHVoltage);
	} else if (Label == "lvoltage:") {
		// woltomierz niskiego napiecia
		ggLVoltage.Load(Parser, DynamicObject, DynamicObject->mdKabina);
	} else if (Label == "enrot1m:") {
		// obrotomierz
		auto& gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
		gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
		gauge.AssignFloat(fEngine + 1);
	} // ggEnrot1m.Load(Parser,DynamicObject->mdKabina);
	else if (Label == "enrot2m:") {
		// obrotomierz
		auto& gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
		gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
		gauge.AssignFloat(fEngine + 2);
	} // ggEnrot2m.Load(Parser,DynamicObject->mdKabina);
	else if (Label == "enrot3m:") { // obrotomierz
		auto& gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
		gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
		gauge.AssignFloat(fEngine + 3);
	} // ggEnrot3m.Load(Parser,DynamicObject->mdKabina);
	else if (Label == "engageratio:") {
		// np. ciśnienie sterownika sprzęgła
		auto& gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
		gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
		gauge.AssignDouble(&mvControlled->dizel_engage);
	} // ggEngageRatio.Load(Parser,DynamicObject->mdKabina);
	else if (Label == "maingearstatus:") {
		// np. ciśnienie sterownika skrzyni biegów
		ggMainGearStatus.Load(Parser, DynamicObject, DynamicObject->mdKabina);
	} else if (Label == "ignitionkey:") {
		ggIgnitionKey.Load(Parser, DynamicObject, DynamicObject->mdKabina);
	} else if (Label == "distcounter:") {
		// Ra 2014-07: licznik kilometrów
		auto& gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
		gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
		gauge.AssignDouble(&mvControlled->DistCounter);
	} else if (Label == "shuntmodepower:") {
		// shunt mode power slider
		auto& gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
		gauge.Load(Parser, DynamicObject, DynamicObject->mdKabina);
		gauge.AssignDouble(&mvControlled->AnPos);
		m_controlmapper.insert(gauge, "shuntmodepower:");
	} else {
		// failed to match the label
		return false;
	}

	return true;
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
	return (TestFlag(mvOccupied->SecuritySystem.Status, s_CAalarm) || TestFlag(mvOccupied->SecuritySystem.Status, s_SHPalarm));
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
