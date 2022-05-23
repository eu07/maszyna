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
#include "Event.h"
#include "simulationtime.h"
#include "Camera.h"
#include "Logs.h"
#include "MdlMngr.h"
#include "Model3d.h"
#include "dumb3d.h"
#include "Timer.h"
#include "Driver.h"
#include "DynObj.h"
#include "mtable.h"
#include "Console.h"
#include "application.h"
#include "renderer.h"
/*
namespace input {

extern user_command command;

}
*/
void
control_mapper::clear() {

    *this = control_mapper();
}

void
control_mapper::insert( TGauge const &Gauge, std::string const &Label ) {

    if( Gauge.SubModel   != nullptr ) { m_controlnames.emplace( Gauge.SubModel, Label ); }
    if( Gauge.SubModelOn != nullptr ) { m_controlnames.emplace( Gauge.SubModelOn, Label ); }

    m_names.emplace( Label );
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

bool
control_mapper::contains( std::string const Control ) const {

    return ( m_names.find( Control ) != m_names.end() );
}

void TTrain::screen_entry::deserialize( cParser &Input ) {

    while( true == deserialize_mapping( Input ) ) {
        ; // all work done by while()
    }
}

bool TTrain::screen_entry::deserialize_mapping( cParser &Input ) {
    // token can be a key or block end
    auto const key { Input.getToken<std::string>( true, "\n\r\t  ,;[]" ) };

    if( ( true == key.empty() ) || ( key == "}" ) ) { return false; }

    if( key == "{" ) {
        script = Input.getToken<std::string>();
    }
    else if( key == "target:" ) {
        target = Input.getToken<std::string>();
    }
	else if (key == "updatetime:") {
		updatetime = Input.getToken<int>();
	}
    else if( key == "parameters:" ) {
        parameters = dictionary_source( Input.getToken<std::string>() );
    }
    else {
        // HACK: we expect this to be true only if the screen entry doesn't start with a { which means legacy configuration format
        target = key;
        script = Input.getToken<std::string>();
        return false;
    }

    return true;
}

void TCab::Load(cParser &Parser)
{
    // NOTE: clearing control tables here is bit of a crutch, imposed by current scheme of loading compartments anew on each cab change
    ggList.clear();
    btList.clear();

    std::string token;
    Parser.getTokens();
    Parser >> token;
    if (token == "cablight:")
    {
		Parser.getTokens( 9, false );
/*
		Parser
			>> dimm.r
			>> dimm.g
			>> dimm.b
			>> intlit.r
			>> intlit.g
			>> intlit.b
			>> intlitlow.r
			>> intlitlow.g
			>> intlitlow.b;
*/
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

void TCab::Update( bool const Power )
{ // odczyt parametrów i ustawienie animacji submodelom
    for( auto &gauge : ggList ) {
        // animacje izometryczne
        gauge.UpdateValue(); // odczyt parametru i przeliczenie na kąt
        gauge.Update(); // ustawienie animacji
    }

    for( auto &button : btList ) {
        // animacje dwustanowe
        button.Update( Power ); // odczyt parametru i wybór submodelu
    }
};

// NOTE: we're currently using universal handlers and static handler map but it may be beneficial to have these implemented on individual class instance basis
// TBD, TODO: consider this approach if we ever want to have customized consist behaviour to received commands, based on the consist/vehicle type or whatever
TTrain::commandhandler_map const TTrain::m_commandhandlers = {

    { user_command::aidriverenable, &TTrain::OnCommand_aidriverenable },
    { user_command::aidriverdisable, &TTrain::OnCommand_aidriverdisable },
    { user_command::jointcontrollerset, &TTrain::OnCommand_jointcontrollerset },
    { user_command::mastercontrollerincrease, &TTrain::OnCommand_mastercontrollerincrease },
    { user_command::mastercontrollerincreasefast, &TTrain::OnCommand_mastercontrollerincreasefast },
    { user_command::mastercontrollerdecrease, &TTrain::OnCommand_mastercontrollerdecrease },
    { user_command::mastercontrollerdecreasefast, &TTrain::OnCommand_mastercontrollerdecreasefast },
    { user_command::mastercontrollerset, &TTrain::OnCommand_mastercontrollerset },
    { user_command::secondcontrollerincrease, &TTrain::OnCommand_secondcontrollerincrease },
    { user_command::secondcontrollerincreasefast, &TTrain::OnCommand_secondcontrollerincreasefast },
    { user_command::secondcontrollerdecrease, &TTrain::OnCommand_secondcontrollerdecrease },
    { user_command::secondcontrollerdecreasefast, &TTrain::OnCommand_secondcontrollerdecreasefast },
    { user_command::secondcontrollerset, &TTrain::OnCommand_secondcontrollerset },
    { user_command::notchingrelaytoggle, &TTrain::OnCommand_notchingrelaytoggle },
    { user_command::tempomattoggle, &TTrain::OnCommand_tempomattoggle },
    { user_command::mucurrentindicatorothersourceactivate, &TTrain::OnCommand_mucurrentindicatorothersourceactivate },
    { user_command::independentbrakeincrease, &TTrain::OnCommand_independentbrakeincrease },
    { user_command::independentbrakeincreasefast, &TTrain::OnCommand_independentbrakeincreasefast },
    { user_command::independentbrakedecrease, &TTrain::OnCommand_independentbrakedecrease },
    { user_command::independentbrakedecreasefast, &TTrain::OnCommand_independentbrakedecreasefast },
    { user_command::independentbrakeset, &TTrain::OnCommand_independentbrakeset },
    { user_command::independentbrakebailoff, &TTrain::OnCommand_independentbrakebailoff },
	{ user_command::universalbrakebutton1, &TTrain::OnCommand_universalbrakebutton1 },
	{ user_command::universalbrakebutton2, &TTrain::OnCommand_universalbrakebutton2 },
	{ user_command::universalbrakebutton3, &TTrain::OnCommand_universalbrakebutton3 },
    { user_command::trainbrakeincrease, &TTrain::OnCommand_trainbrakeincrease },
    { user_command::trainbrakedecrease, &TTrain::OnCommand_trainbrakedecrease },
    { user_command::trainbrakeset, &TTrain::OnCommand_trainbrakeset },
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
    { user_command::alarmchainenable, &TTrain::OnCommand_alarmchainenable},
    { user_command::alarmchaindisable, &TTrain::OnCommand_alarmchaindisable},
    { user_command::wheelspinbrakeactivate, &TTrain::OnCommand_wheelspinbrakeactivate },
    { user_command::sandboxactivate, &TTrain::OnCommand_sandboxactivate },
    { user_command::autosandboxtoggle, &TTrain::OnCommand_autosandboxtoggle },
    { user_command::autosandboxactivate, &TTrain::OnCommand_autosandboxactivate },
    { user_command::autosandboxdeactivate, &TTrain::OnCommand_autosandboxdeactivate },
    { user_command::epbrakecontroltoggle, &TTrain::OnCommand_epbrakecontroltoggle },
	{ user_command::trainbrakeoperationmodeincrease, &TTrain::OnCommand_trainbrakeoperationmodeincrease },
	{ user_command::trainbrakeoperationmodedecrease, &TTrain::OnCommand_trainbrakeoperationmodedecrease },
    { user_command::brakeactingspeedincrease, &TTrain::OnCommand_brakeactingspeedincrease },
    { user_command::brakeactingspeeddecrease, &TTrain::OnCommand_brakeactingspeeddecrease },
    { user_command::brakeactingspeedsetcargo, &TTrain::OnCommand_brakeactingspeedsetcargo },
    { user_command::brakeactingspeedsetpassenger, &TTrain::OnCommand_brakeactingspeedsetpassenger },
    { user_command::brakeactingspeedsetrapid, &TTrain::OnCommand_brakeactingspeedsetrapid },
    { user_command::brakeloadcompensationincrease, &TTrain::OnCommand_brakeloadcompensationincrease },
    { user_command::brakeloadcompensationdecrease, &TTrain::OnCommand_brakeloadcompensationdecrease },
    { user_command::mubrakingindicatortoggle, &TTrain::OnCommand_mubrakingindicatortoggle },
    { user_command::reverserincrease, &TTrain::OnCommand_reverserincrease },
    { user_command::reverserdecrease, &TTrain::OnCommand_reverserdecrease },
    { user_command::reverserforwardhigh, &TTrain::OnCommand_reverserforwardhigh },
    { user_command::reverserforward, &TTrain::OnCommand_reverserforward },
    { user_command::reverserneutral, &TTrain::OnCommand_reverserneutral },
    { user_command::reverserbackward, &TTrain::OnCommand_reverserbackward },
    { user_command::alerteracknowledge, &TTrain::OnCommand_alerteracknowledge },
    { user_command::cabsignalacknowledge, &TTrain::OnCommand_cabsignalacknowledge },
    { user_command::batterytoggle, &TTrain::OnCommand_batterytoggle },
    { user_command::batteryenable, &TTrain::OnCommand_batteryenable },
    { user_command::batterydisable, &TTrain::OnCommand_batterydisable },
	{ user_command::cabactivationtoggle, &TTrain::OnCommand_cabactivationtoggle },
	{ user_command::cabactivationenable, &TTrain::OnCommand_cabactivationenable },
	{ user_command::cabactivationdisable, &TTrain::OnCommand_cabactivationdisable },
    { user_command::pantographcompressorvalvetoggle, &TTrain::OnCommand_pantographcompressorvalvetoggle },
    { user_command::pantographcompressorvalveenable, &TTrain::OnCommand_pantographcompressorvalveenable },
    { user_command::pantographcompressorvalvedisable, &TTrain::OnCommand_pantographcompressorvalvedisable },
    { user_command::pantographcompressoractivate, &TTrain::OnCommand_pantographcompressoractivate },
    { user_command::pantographtogglefront, &TTrain::OnCommand_pantographtogglefront },
    { user_command::pantographtogglerear, &TTrain::OnCommand_pantographtogglerear },
    { user_command::pantographraisefront, &TTrain::OnCommand_pantographraisefront },
    { user_command::pantographraiserear, &TTrain::OnCommand_pantographraiserear },
    { user_command::pantographlowerfront, &TTrain::OnCommand_pantographlowerfront },
    { user_command::pantographlowerrear, &TTrain::OnCommand_pantographlowerrear },
    { user_command::pantographlowerall, &TTrain::OnCommand_pantographlowerall },
    { user_command::pantographselectnext, &TTrain::OnCommand_pantographselectnext },
    { user_command::pantographselectprevious, &TTrain::OnCommand_pantographselectprevious },
    { user_command::pantographtoggleselected, &TTrain::OnCommand_pantographtoggleselected },
    { user_command::pantographraiseselected, &TTrain::OnCommand_pantographraiseselected },
    { user_command::pantographlowerselected, &TTrain::OnCommand_pantographlowerselected },
    { user_command::pantographvalvesupdate, &TTrain::OnCommand_pantographvalvesupdate },
    { user_command::pantographvalvesoff, &TTrain::OnCommand_pantographvalvesoff },
    { user_command::linebreakertoggle, &TTrain::OnCommand_linebreakertoggle },
    { user_command::linebreakeropen, &TTrain::OnCommand_linebreakeropen },
    { user_command::linebreakerclose, &TTrain::OnCommand_linebreakerclose },
    { user_command::fuelpumptoggle, &TTrain::OnCommand_fuelpumptoggle },
    { user_command::fuelpumpenable, &TTrain::OnCommand_fuelpumpenable },
    { user_command::fuelpumpdisable, &TTrain::OnCommand_fuelpumpdisable },
    { user_command::oilpumptoggle, &TTrain::OnCommand_oilpumptoggle },
    { user_command::oilpumpenable, &TTrain::OnCommand_oilpumpenable },
    { user_command::oilpumpdisable, &TTrain::OnCommand_oilpumpdisable },
    { user_command::waterheaterbreakertoggle, &TTrain::OnCommand_waterheaterbreakertoggle },
    { user_command::waterheaterbreakerclose, &TTrain::OnCommand_waterheaterbreakerclose },
    { user_command::waterheaterbreakeropen, &TTrain::OnCommand_waterheaterbreakeropen },
    { user_command::waterheatertoggle, &TTrain::OnCommand_waterheatertoggle },
    { user_command::waterheaterenable, &TTrain::OnCommand_waterheaterenable },
    { user_command::waterheaterdisable, &TTrain::OnCommand_waterheaterdisable },
    { user_command::waterpumpbreakertoggle, &TTrain::OnCommand_waterpumpbreakertoggle },
    { user_command::waterpumpbreakerclose, &TTrain::OnCommand_waterpumpbreakerclose },
    { user_command::waterpumpbreakeropen, &TTrain::OnCommand_waterpumpbreakeropen },
    { user_command::waterpumptoggle, &TTrain::OnCommand_waterpumptoggle },
    { user_command::waterpumpenable, &TTrain::OnCommand_waterpumpenable },
    { user_command::waterpumpdisable, &TTrain::OnCommand_waterpumpdisable },
    { user_command::watercircuitslinktoggle, &TTrain::OnCommand_watercircuitslinktoggle },
    { user_command::watercircuitslinkenable, &TTrain::OnCommand_watercircuitslinkenable },
    { user_command::watercircuitslinkdisable, &TTrain::OnCommand_watercircuitslinkdisable },
    { user_command::convertertoggle, &TTrain::OnCommand_convertertoggle },
    { user_command::converterenable, &TTrain::OnCommand_converterenable },
    { user_command::converterdisable, &TTrain::OnCommand_converterdisable },
    { user_command::convertertogglelocal, &TTrain::OnCommand_convertertogglelocal },
    { user_command::converteroverloadrelayreset, &TTrain::OnCommand_converteroverloadrelayreset },
    { user_command::compressortoggle, &TTrain::OnCommand_compressortoggle },
    { user_command::compressorenable, &TTrain::OnCommand_compressorenable },
    { user_command::compressordisable, &TTrain::OnCommand_compressordisable },
    { user_command::compressortogglelocal, &TTrain::OnCommand_compressortogglelocal },
	{ user_command::compressorpresetactivatenext, &TTrain::OnCommand_compressorpresetactivatenext },
	{ user_command::compressorpresetactivateprevious, &TTrain::OnCommand_compressorpresetactivateprevious },
	{ user_command::compressorpresetactivatedefault, &TTrain::OnCommand_compressorpresetactivatedefault },
    { user_command::motorblowerstogglefront, &TTrain::OnCommand_motorblowerstogglefront },
    { user_command::motorblowerstogglerear, &TTrain::OnCommand_motorblowerstogglerear },
    { user_command::motorblowersdisableall, &TTrain::OnCommand_motorblowersdisableall },
    { user_command::coolingfanstoggle, &TTrain::OnCommand_coolingfanstoggle },
    { user_command::motorconnectorsopen, &TTrain::OnCommand_motorconnectorsopen },
    { user_command::motorconnectorsclose, &TTrain::OnCommand_motorconnectorsclose },
    { user_command::motordisconnect, &TTrain::OnCommand_motordisconnect },
    { user_command::motoroverloadrelaythresholdtoggle, &TTrain::OnCommand_motoroverloadrelaythresholdtoggle },
    { user_command::motoroverloadrelaythresholdsetlow, &TTrain::OnCommand_motoroverloadrelaythresholdsetlow },
    { user_command::motoroverloadrelaythresholdsethigh, &TTrain::OnCommand_motoroverloadrelaythresholdsethigh },
    { user_command::motoroverloadrelayreset, &TTrain::OnCommand_motoroverloadrelayreset },
    { user_command::universalrelayreset1, &TTrain::OnCommand_universalrelayreset },
    { user_command::universalrelayreset2, &TTrain::OnCommand_universalrelayreset },
    { user_command::universalrelayreset3, &TTrain::OnCommand_universalrelayreset },
    { user_command::heatingtoggle, &TTrain::OnCommand_heatingtoggle },
    { user_command::heatingenable, &TTrain::OnCommand_heatingenable },
    { user_command::heatingdisable, &TTrain::OnCommand_heatingdisable },
    { user_command::lightspresetactivatenext, &TTrain::OnCommand_lightspresetactivatenext },
    { user_command::lightspresetactivateprevious, &TTrain::OnCommand_lightspresetactivateprevious },
    { user_command::headlighttoggleleft, &TTrain::OnCommand_headlighttoggleleft },
    { user_command::headlightenableleft, &TTrain::OnCommand_headlightenableleft },
    { user_command::headlightdisableleft, &TTrain::OnCommand_headlightdisableleft },
    { user_command::headlighttoggleright, &TTrain::OnCommand_headlighttoggleright },
    { user_command::headlightenableright, &TTrain::OnCommand_headlightenableright },
    { user_command::headlightdisableright, &TTrain::OnCommand_headlightdisableright },
    { user_command::headlighttoggleupper, &TTrain::OnCommand_headlighttoggleupper },
    { user_command::headlightenableupper, &TTrain::OnCommand_headlightenableupper },
    { user_command::headlightdisableupper, &TTrain::OnCommand_headlightdisableupper },
    { user_command::redmarkertoggleleft, &TTrain::OnCommand_redmarkertoggleleft },
    { user_command::redmarkerenableleft, &TTrain::OnCommand_redmarkerenableleft },
    { user_command::redmarkerdisableleft, &TTrain::OnCommand_redmarkerdisableleft },
    { user_command::redmarkertoggleright, &TTrain::OnCommand_redmarkertoggleright },
    { user_command::redmarkerenableright, &TTrain::OnCommand_redmarkerenableright },
    { user_command::redmarkerdisableright, &TTrain::OnCommand_redmarkerdisableright },
    { user_command::headlighttogglerearleft, &TTrain::OnCommand_headlighttogglerearleft },
    { user_command::headlightenablerearleft, &TTrain::OnCommand_headlightenablerearleft },
    { user_command::headlightdisablerearleft, &TTrain::OnCommand_headlightdisablerearleft },
    { user_command::headlighttogglerearright, &TTrain::OnCommand_headlighttogglerearright },
    { user_command::headlightenablerearright, &TTrain::OnCommand_headlightenablerearright },
    { user_command::headlightdisablerearright, &TTrain::OnCommand_headlightdisablerearright },
    { user_command::headlighttogglerearupper, &TTrain::OnCommand_headlighttogglerearupper },
    { user_command::headlightenablerearupper, &TTrain::OnCommand_headlightenablerearupper },
    { user_command::headlightdisablerearupper, &TTrain::OnCommand_headlightdisablerearupper },
    { user_command::redmarkertogglerearleft, &TTrain::OnCommand_redmarkertogglerearleft },
    { user_command::redmarkerenablerearleft, &TTrain::OnCommand_redmarkerenablerearleft },
    { user_command::redmarkerdisablerearleft, &TTrain::OnCommand_redmarkerdisablerearleft },
    { user_command::redmarkertogglerearright, &TTrain::OnCommand_redmarkertogglerearright },
    { user_command::redmarkerenablerearright, &TTrain::OnCommand_redmarkerenablerearright },
    { user_command::redmarkerdisablerearright, &TTrain::OnCommand_redmarkerdisablerearright },
    { user_command::redmarkerstoggle, &TTrain::OnCommand_redmarkerstoggle },
    { user_command::endsignalstoggle, &TTrain::OnCommand_endsignalstoggle },
    { user_command::headlightsdimtoggle, &TTrain::OnCommand_headlightsdimtoggle },
    { user_command::headlightsdimenable, &TTrain::OnCommand_headlightsdimenable },
    { user_command::headlightsdimdisable, &TTrain::OnCommand_headlightsdimdisable },
    { user_command::interiorlighttoggle, &TTrain::OnCommand_interiorlighttoggle },
    { user_command::interiorlightenable, &TTrain::OnCommand_interiorlightenable },
    { user_command::interiorlightdisable, &TTrain::OnCommand_interiorlightdisable },
    { user_command::interiorlightdimtoggle, &TTrain::OnCommand_interiorlightdimtoggle },
    { user_command::interiorlightdimenable, &TTrain::OnCommand_interiorlightdimenable },
    { user_command::interiorlightdimdisable, &TTrain::OnCommand_interiorlightdimdisable },
    { user_command::compartmentlightstoggle, &TTrain::OnCommand_compartmentlightstoggle },
    { user_command::compartmentlightsenable, &TTrain::OnCommand_compartmentlightsenable },
    { user_command::compartmentlightsdisable, &TTrain::OnCommand_compartmentlightsdisable },
    { user_command::instrumentlighttoggle, &TTrain::OnCommand_instrumentlighttoggle },
    { user_command::instrumentlightenable, &TTrain::OnCommand_instrumentlightenable },
    { user_command::instrumentlightdisable, &TTrain::OnCommand_instrumentlightdisable },
    { user_command::dashboardlighttoggle, &TTrain::OnCommand_dashboardlighttoggle },
    { user_command::dashboardlightenable, &TTrain::OnCommand_dashboardlightenable },
    { user_command::dashboardlightdisable, &TTrain::OnCommand_dashboardlightdisable },
    { user_command::timetablelighttoggle, &TTrain::OnCommand_timetablelighttoggle },
    { user_command::timetablelightenable, &TTrain::OnCommand_timetablelightenable },
    { user_command::timetablelightdisable, &TTrain::OnCommand_timetablelightdisable },
    { user_command::doorlocktoggle, &TTrain::OnCommand_doorlocktoggle },
    { user_command::doortoggleleft, &TTrain::OnCommand_doortoggleleft },
    { user_command::doortoggleright, &TTrain::OnCommand_doortoggleright },
    { user_command::doorpermitleft, &TTrain::OnCommand_doorpermitleft },
    { user_command::doorpermitright, &TTrain::OnCommand_doorpermitright },
    { user_command::doorpermitpresetactivatenext, &TTrain::OnCommand_doorpermitpresetactivatenext },
    { user_command::doorpermitpresetactivateprevious, &TTrain::OnCommand_doorpermitpresetactivateprevious },
    { user_command::dooropenleft, &TTrain::OnCommand_dooropenleft },
    { user_command::dooropenright, &TTrain::OnCommand_dooropenright },
    { user_command::doorcloseleft, &TTrain::OnCommand_doorcloseleft },
    { user_command::doorcloseright, &TTrain::OnCommand_doorcloseright },
    { user_command::dooropenall, &TTrain::OnCommand_dooropenall },
    { user_command::doorcloseall, &TTrain::OnCommand_doorcloseall },
    { user_command::doorsteptoggle, &TTrain::OnCommand_doorsteptoggle },
    { user_command::doormodetoggle, &TTrain::OnCommand_doormodetoggle },
	{ user_command::mirrorstoggle, &TTrain::OnCommand_mirrorstoggle },
    { user_command::nearestcarcouplingincrease, &TTrain::OnCommand_nearestcarcouplingincrease },
    { user_command::nearestcarcouplingdisconnect, &TTrain::OnCommand_nearestcarcouplingdisconnect },
    { user_command::nearestcarcoupleradapterattach, &TTrain::OnCommand_nearestcarcoupleradapterattach },
    { user_command::nearestcarcoupleradapterremove, &TTrain::OnCommand_nearestcarcoupleradapterremove },
    { user_command::occupiedcarcouplingdisconnect, &TTrain::OnCommand_occupiedcarcouplingdisconnect },
    { user_command::departureannounce, &TTrain::OnCommand_departureannounce },
    { user_command::hornlowactivate, &TTrain::OnCommand_hornlowactivate },
    { user_command::hornhighactivate, &TTrain::OnCommand_hornhighactivate },
    { user_command::whistleactivate, &TTrain::OnCommand_whistleactivate },
    { user_command::radiotoggle, &TTrain::OnCommand_radiotoggle },
    { user_command::radioenable, &TTrain::OnCommand_radioenable },
    { user_command::radiodisable, &TTrain::OnCommand_radiodisable },
    { user_command::radiochannelincrease, &TTrain::OnCommand_radiochannelincrease },
    { user_command::radiochanneldecrease, &TTrain::OnCommand_radiochanneldecrease },
    { user_command::radiochannelset, &TTrain::OnCommand_radiochannelset },
    { user_command::radiostopsend, &TTrain::OnCommand_radiostopsend },
    { user_command::radiostopenable, &TTrain::OnCommand_radiostopenable },
    { user_command::radiostopdisable, &TTrain::OnCommand_radiostopdisable },
    { user_command::radiostoptest, &TTrain::OnCommand_radiostoptest },
    { user_command::radiocall3send, &TTrain::OnCommand_radiocall3send },
	{ user_command::radiovolumeincrease, &TTrain::OnCommand_radiovolumeincrease },
	{ user_command::radiovolumedecrease, &TTrain::OnCommand_radiovolumedecrease },
	{ user_command::radiovolumeset, &TTrain::OnCommand_radiovolumeset },
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
    { user_command::generictoggle9, &TTrain::OnCommand_generictoggle },
    { user_command::vehiclemoveforwards, &TTrain::OnCommand_vehiclemoveforwards },
    { user_command::vehiclemovebackwards, &TTrain::OnCommand_vehiclemovebackwards },
    { user_command::vehicleboost, &TTrain::OnCommand_vehicleboost },
	{ user_command::springbraketoggle, &TTrain::OnCommand_springbraketoggle },
	{ user_command::springbrakeenable, &TTrain::OnCommand_springbrakeenable },
	{ user_command::springbrakedisable, &TTrain::OnCommand_springbrakedisable },
	{ user_command::springbrakeshutofftoggle, &TTrain::OnCommand_springbrakeshutofftoggle },
	{ user_command::springbrakeshutoffenable, &TTrain::OnCommand_springbrakeshutoffenable },
	{ user_command::springbrakeshutoffdisable, &TTrain::OnCommand_springbrakeshutoffdisable },
	{ user_command::springbrakerelease, &TTrain::OnCommand_springbrakerelease },
    { user_command::distancecounteractivate, &TTrain::OnCommand_distancecounteractivate },
	{ user_command::speedcontrolincrease, &TTrain::OnCommand_speedcontrolincrease },
	{ user_command::speedcontroldecrease, &TTrain::OnCommand_speedcontroldecrease },
	{ user_command::speedcontrolpowerincrease, &TTrain::OnCommand_speedcontrolpowerincrease },
	{ user_command::speedcontrolpowerdecrease, &TTrain::OnCommand_speedcontrolpowerdecrease },
	{ user_command::speedcontrolbutton0, &TTrain::OnCommand_speedcontrolbutton },
	{ user_command::speedcontrolbutton1, &TTrain::OnCommand_speedcontrolbutton },
	{ user_command::speedcontrolbutton2, &TTrain::OnCommand_speedcontrolbutton },
	{ user_command::speedcontrolbutton3, &TTrain::OnCommand_speedcontrolbutton },
	{ user_command::speedcontrolbutton4, &TTrain::OnCommand_speedcontrolbutton },
	{ user_command::speedcontrolbutton5, &TTrain::OnCommand_speedcontrolbutton },
	{ user_command::speedcontrolbutton6, &TTrain::OnCommand_speedcontrolbutton },
	{ user_command::speedcontrolbutton7, &TTrain::OnCommand_speedcontrolbutton },
	{ user_command::speedcontrolbutton8, &TTrain::OnCommand_speedcontrolbutton },
	{ user_command::speedcontrolbutton9, &TTrain::OnCommand_speedcontrolbutton },
	{ user_command::inverterenable1, &TTrain::OnCommand_inverterenable },
	{ user_command::inverterenable2, &TTrain::OnCommand_inverterenable },
	{ user_command::inverterenable3, &TTrain::OnCommand_inverterenable },
	{ user_command::inverterenable4, &TTrain::OnCommand_inverterenable },
	{ user_command::inverterenable5, &TTrain::OnCommand_inverterenable },
	{ user_command::inverterenable6, &TTrain::OnCommand_inverterenable },
	{ user_command::inverterenable7, &TTrain::OnCommand_inverterenable },
	{ user_command::inverterenable8, &TTrain::OnCommand_inverterenable },
	{ user_command::inverterenable9, &TTrain::OnCommand_inverterenable },
	{ user_command::inverterenable10, &TTrain::OnCommand_inverterenable },
	{ user_command::inverterenable11, &TTrain::OnCommand_inverterenable },
	{ user_command::inverterenable12, &TTrain::OnCommand_inverterenable },
	{ user_command::inverterdisable1, &TTrain::OnCommand_inverterdisable },
	{ user_command::inverterdisable2, &TTrain::OnCommand_inverterdisable },
	{ user_command::inverterdisable3, &TTrain::OnCommand_inverterdisable },
	{ user_command::inverterdisable4, &TTrain::OnCommand_inverterdisable },
	{ user_command::inverterdisable5, &TTrain::OnCommand_inverterdisable },
	{ user_command::inverterdisable6, &TTrain::OnCommand_inverterdisable },
	{ user_command::inverterdisable7, &TTrain::OnCommand_inverterdisable },
	{ user_command::inverterdisable8, &TTrain::OnCommand_inverterdisable },
	{ user_command::inverterdisable9, &TTrain::OnCommand_inverterdisable },
	{ user_command::inverterdisable10, &TTrain::OnCommand_inverterdisable },
	{ user_command::inverterdisable11, &TTrain::OnCommand_inverterdisable },
	{ user_command::inverterdisable12, &TTrain::OnCommand_inverterdisable },
	{ user_command::invertertoggle1, &TTrain::OnCommand_invertertoggle },
	{ user_command::invertertoggle2, &TTrain::OnCommand_invertertoggle },
	{ user_command::invertertoggle3, &TTrain::OnCommand_invertertoggle },
	{ user_command::invertertoggle4, &TTrain::OnCommand_invertertoggle },
	{ user_command::invertertoggle5, &TTrain::OnCommand_invertertoggle },
	{ user_command::invertertoggle6, &TTrain::OnCommand_invertertoggle },
	{ user_command::invertertoggle7, &TTrain::OnCommand_invertertoggle },
	{ user_command::invertertoggle8, &TTrain::OnCommand_invertertoggle },
	{ user_command::invertertoggle9, &TTrain::OnCommand_invertertoggle },
	{ user_command::invertertoggle10, &TTrain::OnCommand_invertertoggle },
	{ user_command::invertertoggle11, &TTrain::OnCommand_invertertoggle },
	{ user_command::invertertoggle12, &TTrain::OnCommand_invertertoggle },
};

std::vector<std::string> const TTrain::fPress_labels = {

    "ch1:  ", "ch2:  ", "ch3:  ", "ch4:  ", "ch5:  ", "ch6:  ", "ch7:  ", "ch8:  ", "ch9:  ", "ch0:  "
};

TTrain::TTrain() {

    ShowNextCurrent = false;
    // McZapkie-240302 - przyda sie do tachometru
    fTachoVelocity = 0;
    fTachoCount = 0;
    fPPress = fNPress = 0;

    // asMessage="";
    pMechOffset = Math3D::vector3(0, 0, 0);
    fBlinkTimer = 0;
    fHaslerTimer = 0;
    DynamicSet(NULL); // ustawia wszystkie mv*
    //-----
    pMechSittingPosition = Math3D::vector3(0, 0, 0); // ABu: 180404
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
		//bComp[ i ][ 2 ] = false;
		//bComp[ i ][ 3 ] = false;
        bHeat[ i ] = false;
    }
	bCompressors.clear();
    for( int i = 0; i < 9; ++i )
		for (int j = 0; j < 10; ++j)
		{
			fEIMParams[i][j] = 0.0;
			fDieselParams[i][j] = 0.0;
		}

	for ( int i = 0; i < 20; ++i )
	{
		for ( int j = 0; j < 7; ++j )
			fPress[i][j] = 0.0;
		bBrakes[i][0] = bBrakes[i][1] = false;
	}
}

TTrain::~TTrain()
{
}

bool TTrain::Init(TDynamicObject *NewDynamicObject, bool e3d)
{ // powiązanie ręcznego sterowania kabiną z pojazdem
    if( NewDynamicObject->Mechanik == nullptr ) {
        /*
        ErrorLog( "Bad config: can't take control of inactive vehicle \"" + NewDynamicObject->asName + "\"" );
        return false;
        */
        auto const activecab { (
            NewDynamicObject->MoverParameters->CabOccupied > 0 ? "1" :
            NewDynamicObject->MoverParameters->CabOccupied < 0 ? "2" :
            "p" ) };
        NewDynamicObject->create_controller( activecab, NewDynamicObject->ctOwner != nullptr );
    }

    DynamicSet(NewDynamicObject);
    if (!e3d)
        if (DynamicObject->Mechanik == NULL)
            return false;

    DynamicObject->MechInside = true;

    fMainRelayTimer = 0; // Hunter, do k...y nędzy, ustawiaj wartości początkowe zmiennych!

    iCabn = (
        mvOccupied->CabOccupied > 0 ? 1 :
        mvOccupied->CabOccupied < 0 ? 2 :
        0 );

    {
        Global.CurrentMaxTextureSize = Global.iMaxCabTextureSize;

        auto const filename { mvOccupied->TypeName + ".mmd" };
        LoadMMediaFile( filename );
        InitializeCab(
            mvOccupied->CabOccupied,
            filename );

        Global.CurrentMaxTextureSize = Global.iMaxTextureSize;

        if( DynamicObject->Controller == Humandriver ) {
            // McZapkie-030303: mozliwosc wyswietlania kabiny, w przyszlosci dac opcje w mmd
            DynamicObject->bDisplayCab = true;
        }
    }

    // Ra: taka proteza - przesłanie kierunku do członów connected
    /*
    if (mvControlled->DirActive > 0)
    { // było do przodu
        mvControlled->DirectionBackward();
        mvControlled->DirectionForward();
    }
    else if (mvControlled->DirActive < 0)
    {
        mvControlled->DirectionForward();
        mvControlled->DirectionBackward();
    }
    */
    if( false == DynamicObject->Mechanik->AIControllFlag ) {
        DynamicObject->Mechanik->sync_consist_reversers();
    }

    return true;
}

dictionary_source *TTrain::GetTrainState( dictionary_source const &Extraparameters ) {

    if( ( mvOccupied   == nullptr )
     || ( mvControlled == nullptr ) ) { return nullptr; }

    auto *dict { new dictionary_source( Extraparameters ) };
    if( dict == nullptr ) { return nullptr; }

    dict->insert( "name", DynamicObject->asName );
    dict->insert( "cab", mvOccupied->CabOccupied );
	dict->insert( "cabactive", mvOccupied->CabActive );
	dict->insert( "master", mvOccupied->CabMaster );
    // basic systems state data
    dict->insert( "battery", mvOccupied->Power24vIsAvailable );
    dict->insert( "linebreaker", mvControlled->Mains );
    dict->insert( "main_init", ( mvControlled->MainsInitTimeCountdown < mvControlled->MainsInitTime ) && ( mvControlled->MainsInitTimeCountdown > 0.0 ) );
    dict->insert( "main_ready", ( false == mvControlled->Mains ) && ( fHVoltage > 0.0 ) && ( mvControlled->MainsInitTimeCountdown <= 0.0 ) );
    dict->insert( "converter", mvOccupied->Power110vIsAvailable );
    dict->insert( "converter_overload", mvControlled->ConvOvldFlag );
    dict->insert( "compress", mvControlled->CompressorFlag );
    dict->insert( "pant_compressor", mvPantographUnit->PantCompFlag );
    dict->insert( "lights_front", mvOccupied->iLights[ end::front ] );
    dict->insert( "lights_rear", mvOccupied->iLights[ end::rear ] );
    dict->insert( "lights_compartments", mvOccupied->CompartmentLights.is_active || mvOccupied->CompartmentLights.is_disabled );
    if( Dynamic()->Mechanik ) {
        auto const *controller { Dynamic()->Mechanik };
        auto const cabmodifier { cab_to_end() == end::front ? 1 : -1 };
        auto const traindirection { controller->Direction() * cabmodifier };
        auto const *frontvehicle { controller->Vehicle( traindirection >= 0 ? end::front : end::rear ) };
        auto const *rearvehicle { controller->Vehicle( traindirection >= 0 ? end::rear : end::front ) };
        auto const frontvehicledirection { ( frontvehicle->DirectionGet() == controller->Vehicle()->DirectionGet() ? 1 : -1 ) };
        auto const rearvehicledirection { ( rearvehicle->DirectionGet() == controller->Vehicle()->DirectionGet() ? 1 : -1 ) };
        auto const fronttrainlights { frontvehicle->MoverParameters->iLights[ frontvehicledirection * cabmodifier >= 0 ? end::front : end::rear ] };
        auto const reartrainlights{ rearvehicle->MoverParameters->iLights[ rearvehicledirection * cabmodifier >= 0 ? end::rear : end::front ] };
        dict->insert( "lights_train_front", fronttrainlights );
        dict->insert( "lights_train_rear", reartrainlights );
    }
    else {
        // fallback, in the unlikely case we lose the controller
        dict->insert( "lights_train_front", mvOccupied->iLights[ end::front ] );
        dict->insert( "lights_train_rear", mvOccupied->iLights[ end::rear ] );
    }
    // reverser
    dict->insert( "direction", mvOccupied->DirActive );
    // throttle
    dict->insert( "mainctrl_pos", mvControlled->MainCtrlPos );
    dict->insert( "main_ctrl_actual_pos", mvControlled->MainCtrlActualPos );
    dict->insert( "scndctrl_pos", mvControlled->ScndCtrlPos );
    dict->insert( "scnd_ctrl_actual_pos", mvControlled->ScndCtrlActualPos );
	dict->insert( "brakectrl_pos", mvControlled->fBrakeCtrlPos );
	dict->insert( "localbrake_pos", mvControlled->LocalBrakePosA );
	dict->insert( "new_speed", mvOccupied->NewSpeed);
 	dict->insert( "speedctrl", mvOccupied->SpeedCtrlValue);
	dict->insert( "speedctrlpower", mvOccupied->SpeedCtrlUnit.DesiredPower );
	dict->insert( "speedctrlactive", mvOccupied->SpeedCtrlUnit.IsActive );
	dict->insert( "speedctrlstandby", mvOccupied->SpeedCtrlUnit.Standby );
   // brakes
    dict->insert( "manual_brake", ( mvOccupied->ManualBrakePos > 0 ) );
    bool const bEP = ( mvControlled->LocHandle->GetCP() > 0.2 ) || ( fEIMParams[ 0 ][ 5 ] > 0.01 );
    dict->insert( "dir_brake", bEP );
    bool bPN { false };
    if( ( typeid( *mvOccupied->Hamulec ) == typeid( TLSt ) )
     || ( typeid( *mvOccupied->Hamulec ) == typeid( TEStED ) ) ) {

        TBrake* temp_ham = mvOccupied->Hamulec.get();
        bPN = ( static_cast<TLSt*>( temp_ham )->GetEDBCP() > 0.2 );
    }
    dict->insert( "indir_brake", bPN );
	dict->insert( "emergency_brake", mvOccupied->AlarmChainFlag );
	dict->insert( "brake_delay_flag", mvOccupied->BrakeDelayFlag );
	dict->insert( "brake_op_mode_flag", mvOccupied->BrakeOpModeFlag );
    // other controls
    dict->insert( "ca", mvOccupied->SecuritySystem.is_vigilance_blinking());
    dict->insert( "shp", mvOccupied->SecuritySystem.is_cabsignal_blinking());
    dict->insert( "distance_counter", m_distancecounter );
    dict->insert( "pantpress", std::abs( mvPantographUnit->PantPress ) );
    dict->insert( "universal3", InstrumentLightActive );
	for (auto idx = 0; idx < ggUniversals.size(); idx++) {
		if (idx != 3) {
			dict->insert("universal" + std::to_string(idx), (ggUniversals[idx].GetValue() > 0.5));
		}
	}
    dict->insert( "radio", mvOccupied->Radio );
    dict->insert( "radio_channel", RadioChannel() );
	dict->insert( "radio_volume", Global.RadioVolume );
    dict->insert( "door_lock", mvOccupied->Doors.lock_enabled );
	dict->insert( "door_step", mvOccupied->Doors.step_enabled );
	dict->insert( "door_permit_left", mvOccupied->Doors.instances[side::left].open_permit );
	dict->insert( "door_permit_right", mvOccupied->Doors.instances[side::right].open_permit );
    // movement data
    dict->insert( "velocity", std::abs( mvOccupied->Vel ) );
    dict->insert( "tractionforce", std::abs( mvOccupied->Ft ) );
    dict->insert( "slipping_wheels", mvOccupied->SlippingWheels );
    dict->insert( "sanding", mvOccupied->SandDose );
    dict->insert( "odometer", mvOccupied->DistCounter );
    // electric current data
    dict->insert( "traction_voltage", std::abs( mvPantographUnit->PantographVoltage ) );
    dict->insert( "voltage", std::abs( mvControlled->EngineVoltage ) );
    dict->insert( "im", std::abs(  mvControlled->Im ) );
    dict->insert( "fuse", mvControlled->FuseFlag );
    dict->insert( "epfuse", mvOccupied->EpFuse );
    // induction motor state data
    char const *TXTT[ 10 ] = { "fd", "fdt", "fdb", "pd", "pdt", "pdb", "itothv", "1", "2", "3" };
    char const *TXTC[ 10 ] = { "fr", "frt", "frb", "pr", "prt", "prb", "im", "vm", "ihv", "uhv" };
	char const *TXTD[ 10 ] = { "enrot", "nrot", "fill_des", "fill_real", "clutch_des", "clutch_real", "water_temp", "oil_press", "engine_temp", "retarder_fill" };
    char const *TXTP[ 7 ] = { "bc", "bp", "sp", "cp", "rp", "mass", "spring" };
	char const *TXTB[ 2 ] = { "spring_active", "spring_shutoff" };
    for( int j = 0; j < 10; ++j )
        dict->insert( ( "eimp_t_" + std::string( TXTT[ j ] ) ), fEIMParams[ 0 ][ j ] );
    for( int i = 0; i < 8; ++i ) {
        auto const idx { std::to_string( i + 1 ) };
        for( int j = 0; j < 10; ++j )
            dict->insert( ( "eimp_c" + idx + "_" + std::string( TXTC[ j ] ) ), fEIMParams[ i + 1 ][ j ] );

		for (int j = 0; j < 10; ++j)
			dict->insert(("diesel_param_" + idx + "_" + std::string(TXTD[j])), fDieselParams[i + 1][j]);

        dict->insert( ( "eimp_c" + idx + "_ms" ), bMains[ i ] );
        dict->insert( ( "eimp_c" + idx + "_cv" ), fCntVol[ i ] );
        dict->insert( ( "eimp_c" + idx + "_fuse" ), bFuse[ i ] );
        dict->insert( ( "eimp_c" + idx + "_batt" ), bBatt[ i ] );
        dict->insert( ( "eimp_c" + idx + "_conv" ), bConv[ i ] );
        dict->insert( ( "eimp_c" + idx + "_heat" ), bHeat[ i ] );

        dict->insert( ( "eimp_u" + idx + "_pf" ), bPants[ i ][ 0 ] );
        dict->insert( ( "eimp_u" + idx + "_pr" ), bPants[ i ][ 1 ] );
        dict->insert( ( "eimp_u" + idx + "_comp_a" ), bComp[ i ][ 0 ] );
        dict->insert( ( "eimp_u" + idx + "_comp_w" ), bComp[ i ][ 1 ] );
    }

	dict->insert( "compressors_no", (int)bCompressors.size() );
	for (int i = 0; i < bCompressors.size(); i++)
	{
        auto const idx { std::to_string( i + 1 ) };
        dict->insert("compressors_" + idx + "_allow", std::get<0>(bCompressors[i]));
		dict->insert("compressors_" + idx + "_work", std::get<1>(bCompressors[i]));
		dict->insert("compressors_" + idx + "_car_no", std::get<2>(bCompressors[i]));
	}


	bool kier = (DynamicObject->DirectionGet() * mvOccupied->CabOccupied > 0);
	TDynamicObject *p = DynamicObject->GetFirstDynamic(mvOccupied->CabOccupied < 0 ? end::rear : end::front, 4);
	int in = 0;
	while (p && in < 8)
	{
		if (p->MoverParameters->eimc[eimc_p_Pmax] > 1)
		{
			in++;
			dict->insert(("eimp_c" + std::to_string(in) + "_invno"), p->MoverParameters->InvertersNo);
			for (int j = 0; j < p->MoverParameters->InvertersNo; j++) {
				dict->insert(("eimp_c" + std::to_string(in) + "_inv" + std::to_string(j + 1) + "_act"), p->MoverParameters->Inverters[j].IsActive);
				dict->insert(("eimp_c" + std::to_string(in) + "_inv" + std::to_string(j + 1) + "_error"), p->MoverParameters->Inverters[j].Error);
				dict->insert(("eimp_c" + std::to_string(in) + "_inv" + std::to_string(j + 1) + "_allow"), p->MoverParameters->Inverters[j].Activate);
			}
		}
		p = (kier ? p->Next(4) : p->Prev(4));
	}
    for( int i = 0; i < 20; ++i ) {
        for( int j = 0; j < 7; ++j ) {
            dict->insert( ( "eimp_pn" + std::to_string( i + 1 ) + "_" + TXTP[ j ] ), fPress[ i ][ j ] );
        }
		for ( int j = 0; j < 2; ++j)  {
			dict->insert( ( "brakes_" + std::to_string( i + 1 ) + "_" + TXTB[ j ] ), bBrakes[ i ][ j ] );
		}
    }
    // multi-unit state data
    dict->insert( "car_no", iCarNo );
    dict->insert( "power_no", iPowerNo );
    dict->insert( "unit_no", iUnitNo );

    for( int i = 0; i < 20; i++ ) {
        auto const caridx { std::to_string( i + 1 ) };
        dict->insert( ( "doors_" + caridx ), bDoors[ i ][ 0 ] );
        dict->insert( ( "doors_l_" + caridx ), bDoors[ i ][ 1 ] );
        dict->insert( ( "doors_r_" + caridx ), bDoors[ i ][ 2 ] );
        dict->insert( ( "doorstep_l_" + caridx ), bDoors[ i ][ 3 ] );
        dict->insert( ( "doorstep_r_" + caridx ), bDoors[ i ][ 4 ] );
        dict->insert( ( "doors_no_" + caridx ), iDoorNo[ i ] );
        dict->insert( ( "code_" + caridx ), ( std::to_string( iUnits[ i ] ) + cCode[ i ] ) );
        dict->insert( ( "car_name" + caridx ), asCarName[ i ] );
        dict->insert( ( "slip_" + caridx ), bSlip[ i ] );
    }
    // ai state data
    auto const *driver { (
        DynamicObject->ctOwner != nullptr ?
            DynamicObject->ctOwner :
            DynamicObject->Mechanik ) };

    dict->insert( "velocity_desired", driver->VelDesired );
    dict->insert( "velroad", driver->VelRoad );
    dict->insert( "vellimitlast", driver->VelLimitLast );
    dict->insert( "velsignallast", driver->VelSignalLast );
    dict->insert( "velsignalnext", driver->VelSignalNext );
    dict->insert( "velnext", driver->VelNext );
    dict->insert( "actualproximitydist", driver->ActualProximityDist );
    // train data
    driver->TrainTimetable().serialize( dict );
    dict->insert( "train_atpassengerstop", driver->IsAtPassengerStop );
    dict->insert( "train_length", driver->fLength );
    // world state data
    dict->insert( "scenario", Global.SceneryFile );
    dict->insert( "hours", static_cast<int>( simulation::Time.data().wHour ) );
    dict->insert( "minutes", static_cast<int>( simulation::Time.data().wMinute ) );
    dict->insert( "seconds", static_cast<int>( simulation::Time.second() ) );
    dict->insert( "air_temperature", Global.AirTemperature );
    dict->insert( "light_level", Global.fLuminance - std::max( 0.f, Global.Overcast - 1.f ) );

    return dict;
}

TTrain::state_t
TTrain::get_state() const {

	return {
		btLampkaSHP.GetValue(),
		btLampkaCzuwaka.GetValue(),
		btLampkaRadioStop.GetValue(),
		btLampkaOpory.GetValue(),
		btLampkaWylSzybki.GetValue(),
		btLampkaPrzekRozn.GetValue(),
		btLampkaNadmSil.GetValue(),
		btLampkaStyczn.GetValue(),
		btLampkaPoslizg.GetValue(),
		btLampkaNadmPrzetw.GetValue(),
		btLampkaPrzetwOff.GetValue(),
		btLampkaNadmSpr.GetValue(),
		btLampkaNadmWent.GetValue(),
		btLampkaWysRozr.GetValue(),
		btLampkaOgrzewanieSkladu.GetValue(),
		static_cast<std::uint8_t>(iCabn),
		btHaslerBrakes.GetValue(),
		btHaslerCurrent.GetValue(),
		mvOccupied->SecuritySystem.is_beeping(),
		btLampkaHVoltageB.GetValue(),
		fTachoVelocity,
		static_cast<float>(mvOccupied->Compressor),
		static_cast<float>(mvOccupied->PipePress),
		static_cast<float>(mvOccupied->BrakePress),
		static_cast<float>(mvPantographUnit->PantPress),
		fHVoltage,
		{ fHCurrent[(mvControlled->TrainType & dt_EZT) ? 0 : 1], fHCurrent[2], fHCurrent[3] },
		ggLVoltage.GetValue(),
		mvOccupied->DistCounter,
		static_cast<std::uint8_t>(RadioChannel()),
		btLampkaSpringBrakeActive.GetValue(),
		btLampkaNapNastHam.GetValue(),
		mvOccupied->DirActive > 0,
		mvOccupied->DirActive < 0,
		mvOccupied->Doors.instances[mvOccupied->CabOccupied < 0 ? side::right : side::left].open_permit,
		mvOccupied->Doors.instances[mvOccupied->CabOccupied < 0 ? side::right : side::left].is_open,
		mvOccupied->Doors.instances[mvOccupied->CabOccupied < 0 ? side::left : side::right].open_permit,
		mvOccupied->Doors.instances[mvOccupied->CabOccupied < 0 ? side::left : side::right].is_open,
		mvOccupied->Doors.step_enabled,
		mvOccupied->Power24vIsAvailable,
		0,
		mvOccupied->LockPipe,
		btLampkaRadioMessage.GetValue(),
    };
}

bool TTrain::is_eztoer() const {

    return
        ( ( mvControlled->TrainType == dt_EZT )
       && ( mvOccupied->BrakeSubsystem == TBrakeSubSystem::ss_ESt )
       && ( mvControlled->Power24vIsAvailable == true )
       && ( mvControlled->EpFuse == true )
       && ( mvControlled->DirActive != 0 ) ); // od yB
}

// mover master controller to specified position
void TTrain::set_master_controller( double const Position ) {

    auto positionchange {
        std::min<int>(
            Position,
            ( mvControlled->CoupledCtrl ?
                mvControlled->MainCtrlPosNo + mvControlled->ScndCtrlPosNo :
                mvControlled->MainCtrlPosNo ) )
        - ( mvControlled->CoupledCtrl ?
                mvControlled->MainCtrlPos + mvControlled->ScndCtrlPos :
                mvControlled->MainCtrlPos ) };
    while( ( positionchange < 0 )
        && ( true == mvControlled->DecMainCtrl( 1 ) ) ) {
        ++positionchange;
    }
    while( ( positionchange > 0 )
        && ( true == mvControlled->IncMainCtrl( 1 ) ) ) {
        --positionchange;
    }
}

// moves train brake lever to specified position, potentially emits switch sound if conditions are met
void TTrain::set_train_brake( double const Position ) {

    auto const originalbrakeposition { static_cast<int>( 100.0 * mvOccupied->fBrakeCtrlPos ) };
    mvOccupied->BrakeLevelSet( Position );

    if( static_cast<int>( 100.0 * mvOccupied->fBrakeCtrlPos ) == originalbrakeposition ) { return; }

    if( ( true == is_eztoer() )
     && ( false == (
            ( ( originalbrakeposition / 100 == 0 ) || ( originalbrakeposition / 100 >= 5 ) )
         && ( ( mvOccupied->BrakeCtrlPos == 0 ) || ( mvOccupied->BrakeCtrlPos >= 5 ) ) ) ) ) {
        // sound feedback if the lever movement activates one of the switches
        if( dsbPneumaticSwitch ) {
            dsbPneumaticSwitch->play();
        }
    }
}

void TTrain::zero_charging_train_brake() {

    if( ( mvOccupied->BrakeCtrlPos == -1 )
     && ( DynamicObject->Controller != AIdriver )
     && ( Global.iFeedbackMode < 3 )
     && ( ( mvOccupied->BrakeHandle == TBrakeHandle::FVel6 )
       || ( mvOccupied->BrakeHandle == TBrakeHandle::MHZ_EN57 )
       || ( mvOccupied->BrakeHandle == TBrakeHandle::MHZ_K8P ) ) ) {
        // Odskakiwanie hamulce EP
        set_train_brake( 0 );
    }
}

void TTrain::set_train_brake_speed( TDynamicObject *Vehicle, int const Speed ) {

    if( true == Vehicle->MoverParameters->BrakeDelaySwitch( Speed ) ) {
        // visual feedback
        // TODO: add setting indicator to vehicle class, for external lever/indicator
        if( Vehicle == DynamicObject ) {
            if( ggBrakeProfileCtrl.SubModel != nullptr ) {
                ggBrakeProfileCtrl.UpdateValue(
                    ( ( mvOccupied->BrakeDelayFlag & bdelay_R ) != 0 ?
                        2.0 :
                        mvOccupied->BrakeDelayFlag - 1 ),
                    dsbSwitch );
            }
            if( ggBrakeProfileG.SubModel != nullptr ) {
                ggBrakeProfileG.UpdateValue(
                    ( mvOccupied->BrakeDelayFlag == bdelay_G ?
                        1.0 :
                        0.0 ),
                    dsbSwitch );
            }
            if( ggBrakeProfileR.SubModel != nullptr ) {
                ggBrakeProfileR.UpdateValue(
                    ( ( mvOccupied->BrakeDelayFlag & bdelay_R ) != 0 ?
                        1.0 :
                        0.0 ),
                    dsbSwitch );
            }
        }
    }
}

void TTrain::set_paired_open_motor_connectors_button( bool const State ) {

    if( ( mvControlled->TrainType == dt_ET41 )
     || ( mvControlled->TrainType == dt_ET42 ) ) {
        // crude implementation of the button affecting entire unit for multi-unit engines
        // TODO: rework it into part of standard command propagation system
        if( ( mvControlled->Couplers[ end::front ].Connected != nullptr )
         && ( true == TestFlag( mvControlled->Couplers[ end::front ].CouplingFlag, coupling::permanent ) ) ) {
            mvControlled->Couplers[ end::front ].Connected->StLinSwitchOff = State;
        }
        if( ( mvControlled->Couplers[ end::rear ].Connected != nullptr )
         && ( true == TestFlag( mvControlled->Couplers[ end::rear ].CouplingFlag, coupling::permanent ) ) ) {
            mvControlled->Couplers[ end::rear ].Connected->StLinSwitchOff = State;
        }
    }
}

// locates nearest vehicle belonging to the consist
TDynamicObject *
TTrain::find_nearest_consist_vehicle(bool freefly, glm::vec3 pos) const {
	if (!freefly)
		return DynamicObject;

    auto coupler { -2 }; // scan for vehicle, not any specific coupler
	auto *vehicle{ DynamicObject->ABuScanNearestObject( pos, DynamicObject->GetTrack(), 1, 1500, coupler ) };
    if( vehicle == nullptr )
		vehicle = DynamicObject->ABuScanNearestObject( pos, DynamicObject->GetTrack(), -1, 1500, coupler );
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

auto const EU07_CONTROLLER_BASERETURNDELAY { 0.5f };
auto const EU07_CONTROLLER_KEYBOARDETURNDELAY { 1.5f };

void TTrain::OnCommand_jointcontrollerset( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // on press or hold
        // value controls brake in range 0-0.5, master controller in range 0.5-1.0
        if( Command.param1 >= 0.5 ) {
            Train->set_master_controller(
                ( Command.param1 * 2 - 1 )
                * ( Train->mvControlled->CoupledCtrl ?
                        Train->mvControlled->MainCtrlPosNo + Train->mvControlled->ScndCtrlPosNo :
                        Train->mvControlled->MainCtrlPosNo ) );
            Train->m_mastercontrollerinuse = true;
            Train->mvOccupied->LocalBrakePosA = 0;
        }
        else {
            Train->mvOccupied->LocalBrakePosA = (
                clamp(
                    1.0 - ( Command.param1 * 2 ),
                    0.0, 1.0 ) );
            if( Train->mvControlled->MainCtrlPowerPos() > 0 ) {
                Train->set_master_controller( Train->mvControlled->MainCtrlNoPowerPos() );
            }
        }
    }
    else {
        // release
        Train->m_mastercontrollerinuse = false;
        Train->m_mastercontrollerreturndelay = EU07_CONTROLLER_BASERETURNDELAY; // NOTE: keyboard return delay is omitted for other input sources
    }
}

void TTrain::OnCommand_mastercontrollerincrease( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // on press or hold
        if( ( Train->ggJointCtrl.SubModel != nullptr )
         && ( Train->mvOccupied->LocalBrakePosA > 0.0 ) ) {
            OnCommand_independentbrakedecrease( Train, Command );
        }
        else {
            Train->mvControlled->IncMainCtrl( 1 );
            Train->m_mastercontrollerinuse = true;
        }
    }
    else if (Command.action == GLFW_RELEASE) {
        // release
        Train->m_mastercontrollerinuse = false;
        Train->m_mastercontrollerreturndelay = EU07_CONTROLLER_KEYBOARDETURNDELAY + EU07_CONTROLLER_BASERETURNDELAY;
    }
}

void TTrain::OnCommand_mastercontrollerincreasefast( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // on press or hold
        if( ( Train->ggJointCtrl.SubModel != nullptr )
         && ( Train->mvOccupied->LocalBrakePosA > 0.0 ) ) {
            OnCommand_independentbrakedecreasefast( Train, Command );
        }
        else {
            Train->mvControlled->IncMainCtrl( Train->mvControlled->MainCtrlPosNo );
            Train->m_mastercontrollerinuse = true;
        }
    }
    else {
        // release
        Train->m_mastercontrollerinuse = false;
        Train->m_mastercontrollerreturndelay = EU07_CONTROLLER_KEYBOARDETURNDELAY + EU07_CONTROLLER_BASERETURNDELAY;
    }
}

void TTrain::OnCommand_mastercontrollerdecrease( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // on press or hold
        if( ( Train->ggJointCtrl.SubModel != nullptr )
         && ( Train->mvControlled->IsMainCtrlNoPowerPos() ) ) {
            OnCommand_independentbrakeincrease( Train, Command );
        }
        else {
            Train->mvControlled->DecMainCtrl( 1 );
            Train->m_mastercontrollerinuse = true;
        }
    }
    else if (Command.action == GLFW_RELEASE) {
        // release
        Train->m_mastercontrollerinuse = false;
        Train->m_mastercontrollerreturndelay = EU07_CONTROLLER_KEYBOARDETURNDELAY + EU07_CONTROLLER_BASERETURNDELAY;
    }
}

void TTrain::OnCommand_mastercontrollerdecreasefast( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // on press or hold
        if( ( Train->ggJointCtrl.SubModel != nullptr )
         && ( Train->mvControlled->IsMainCtrlNoPowerPos() ) ) {
            OnCommand_independentbrakeincreasefast( Train, Command );
        }
        else {
            Train->mvControlled->DecMainCtrl( Train->mvControlled->MainCtrlPowerPos() );
            Train->m_mastercontrollerinuse = true;
        }
    }
    else {
        // release
        Train->m_mastercontrollerinuse = false;
        Train->m_mastercontrollerreturndelay = EU07_CONTROLLER_KEYBOARDETURNDELAY + EU07_CONTROLLER_BASERETURNDELAY;
    }
}

void TTrain::OnCommand_mastercontrollerset( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // on press or hold
        Train->set_master_controller( Command.param1 );
        Train->m_mastercontrollerinuse = true;
    }
    else {
        // release
        Train->m_mastercontrollerinuse = false;
        Train->m_mastercontrollerreturndelay = EU07_CONTROLLER_BASERETURNDELAY; // NOTE: keyboard return delay is omitted for other input sources
    }
}

void TTrain::OnCommand_secondcontrollerincrease( TTrain *Train, command_data const &Command ) {

    if( ( Train->mvControlled->EngineType == TEngineType::DieselElectric )
     && ( true == Train->mvControlled->ShuntModeAllow )
     && ( true == Train->mvControlled->ShuntMode ) ) {
        if( Command.action != GLFW_RELEASE ) {
            Train->mvControlled->AnPos = clamp(
                Train->mvControlled->AnPos + 0.025,
                0.0, 1.0 );
        }
    }
    else {
        // regular mode
        // push or pushtoggle control type
        if( Train->ggScndCtrl.is_push() ) {
            if( Command.action == GLFW_PRESS ) {
                // activate on press
                Train->mvControlled->IncScndCtrl( 1 );
            }
        }
        // toggle control type
        else {
            if( Command.action != GLFW_RELEASE ) {
                Train->mvControlled->IncScndCtrl( 1 );
            }
        }
        // HACK: potentially animate push or pushtoggle control
        if( Train->ggScndCtrl.is_push() ) {
            auto const activeposition { Train->ggScndCtrl.is_toggle() ? 1.f : 1.f };
            auto const neutralposition { Train->ggScndCtrl.is_toggle() ? 0.5f : 0.f };
            Train->ggScndCtrl.UpdateValue(
                ( ( Command.action == GLFW_RELEASE ) ? neutralposition : activeposition ),
                Train->dsbSwitch );
        }
        // potentially animate tempomat button
        if( ( Train->ggScndCtrlButton.is_push() )
         && ( Train->mvControlled->ScndCtrlPos <= 1 ) ) {
            Train->ggScndCtrlButton.UpdateValue(
                ( ( Command.action == GLFW_RELEASE ) ? 0.f : 1.f ),
                Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_secondcontrollerincreasefast( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // on press or hold
        if( ( Train->mvControlled->EngineType == TEngineType::DieselElectric )
         && ( true == Train->mvControlled->ShuntMode ) ) {
            Train->mvControlled->AnPos = 1.0;
        }
        else {
            Train->mvControlled->IncScndCtrl( 2 );
        }
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

void TTrain::OnCommand_tempomattoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }

    if( Train->ggScndCtrlButton.is_push() ) {
        // impulse switch
        if( Command.action == GLFW_RELEASE ) {
            // just move the button(s) back to default position
            // visual feedback
            Train->ggScndCtrlButton.UpdateValue( 0.0, Train->dsbSwitch );
            Train->ggScndCtrlOffButton.UpdateValue( 0.0, Train->dsbSwitch );
            return;
        }
        // glfw_press
        if( Train->mvControlled->ScndCtrlPos == 0 ) {
            // turn on if it's not active
            Train->mvControlled->IncScndCtrl( 1 );
            // visual feedback
            Train->ggScndCtrlButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            // otherwise turn off
            Train->mvControlled->DecScndCtrl( 2 );
            // visual feedback
            if( Train->m_controlmapper.contains( "tempomatoff_sw:" ) ) {
                Train->ggScndCtrlOffButton.UpdateValue( 1.0, Train->dsbSwitch );
            }
            else {
                Train->ggScndCtrlButton.UpdateValue( 1.0, Train->dsbSwitch );
            }
        }
    }
    else {
        // two-state switch
        if( Command.action == GLFW_RELEASE ) { return; }

        if( Train->mvControlled->ScndCtrlPos == 0 ) {
            // turn on
            Train->mvControlled->IncScndCtrl( 1 );
            // visual feedback
            Train->ggScndCtrlButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            //turn off
            Train->mvControlled->DecScndCtrl( 2 );
            // visual feedback
            Train->ggScndCtrlButton.UpdateValue( 0.0, Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_distancecounteractivate( TTrain *Train, command_data const &Command ) {
    // NOTE: distance meter activation button is presumed to be of impulse type
    if( Command.action == GLFW_PRESS ) {
        // visual feedback
        Train->ggDistanceCounterButton.UpdateValue( 1.0, Train->dsbSwitch );
        // activate or start anew
        Train->m_distancecounter = 0.f;
    }
    else if( Command.action == GLFW_RELEASE ) {
        // visual feedback
        Train->ggDistanceCounterButton.UpdateValue( 0.0, Train->dsbSwitch );
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

    if( ( Train->mvControlled->EngineType == TEngineType::DieselElectric )
     && ( true == Train->mvControlled->ShuntMode ) ) {
        if( Command.action != GLFW_RELEASE ) {
            Train->mvControlled->AnPos = clamp(
                Train->mvControlled->AnPos - 0.025,
                0.0, 1.0 );
        }
    }
    else {
        // regular mode
        // push or pushtoggle control type
        if( Train->ggScndCtrl.is_push() ) {
            // basic push control can't decrease state, but pushtoggle can
            if( true == Train->ggScndCtrl.is_toggle() ) {
                if( Command.action == GLFW_PRESS ) {
                    // activate on press
                    Train->mvControlled->DecScndCtrl( 1 );
                }
            }
        }
        // toggle control type
        else {
            if( Command.action != GLFW_RELEASE ) {
                Train->mvControlled->DecScndCtrl( 1 );
            }
        }
        // HACK: potentially animate push or pushtoggle control
        if( Train->ggScndCtrl.is_push() ) {
            auto const activeposition { Train->ggScndCtrl.is_toggle() ? 0.f : 1.f };
            auto const neutralposition { Train->ggScndCtrl.is_toggle() ? 0.5f : 0.f };
            Train->ggScndCtrl.UpdateValue(
                ( ( Command.action == GLFW_RELEASE ) ? neutralposition : activeposition ),
                Train->dsbSwitch );
        }
        // potentially animate tempomat button
        if( ( Train->ggScndCtrlButton.is_push() )
         && ( Train->mvControlled->ScndCtrlPos <= 1 ) ) {
            if( Train->m_controlmapper.contains( "tempomatoff_sw:" ) ) {
                Train->ggScndCtrlOffButton.UpdateValue(
                    ( ( Command.action == GLFW_RELEASE ) ? 0.f : 1.f ),
                    Train->dsbSwitch );
            }
            else {
                Train->ggScndCtrlButton.UpdateValue(
                    ( ( Command.action == GLFW_RELEASE ) ? 0.f : 1.f ),
                    Train->dsbSwitch );
            }
        }
    }
}

void TTrain::OnCommand_secondcontrollerdecreasefast( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // on press or hold
        if( ( Train->mvControlled->EngineType == TEngineType::DieselElectric )
         && ( true == Train->mvControlled->ShuntMode ) ) {
            Train->mvControlled->AnPos = 0.0;
        }
        else {
            Train->mvControlled->DecScndCtrl( 2 );
        }
    }
}

void TTrain::OnCommand_secondcontrollerset( TTrain *Train, command_data const &Command ) {

    auto const targetposition{ std::min<int>( Command.param1, Train->mvControlled->ScndCtrlPosNo ) };
    // HACK: potentially animate push or pushtoggle control
    if( Train->ggScndCtrl.is_push() ) {
        auto const activeposition {
            Train->ggScndCtrl.is_toggle() ?
                ( targetposition < Train->mvControlled->ScndCtrlPos ? 0.f :
                  targetposition > Train->mvControlled->ScndCtrlPos ? 1.f :
                  Train->ggScndCtrl.GetDesiredValue() ) : // leave the control in its current position if it hits the limit
                ( targetposition == 0 ? 0.f : 1.f ) };
        auto const neutralposition { Train->ggScndCtrl.is_toggle() ? 0.5f : 0.f };
        Train->ggScndCtrl.UpdateValue(
            ( ( Command.action == GLFW_RELEASE ) ? neutralposition : activeposition ),
            Train->dsbSwitch );
    }
    // update control value
    if( Command.action != GLFW_RELEASE ) {
        // on press or hold
        while( ( targetposition < Train->mvControlled->GetVirtualScndPos() )
            && ( true == Train->mvControlled->DecScndCtrl( 1 ) ) ) {
            // all work is done in the header
            ;
        }
        while( ( targetposition > Train->mvControlled->GetVirtualScndPos() )
            && ( true == Train->mvControlled->IncScndCtrl( 1 ) ) ) {
            // all work is done in the header
            ;
        }
    }
}

void TTrain::OnCommand_independentbrakeincrease( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {

        if( Train->mvOccupied->LocalBrake != TLocalBrake::ManualBrake ) {
            if( ( Train->ggJointCtrl.SubModel != nullptr )
             && ( Train->mvOccupied->MainCtrlPos > 0 ) ) {
                OnCommand_mastercontrollerdecrease( Train, Command );
            }
            else {
                Train->mvOccupied->IncLocalBrakeLevel( (Train->ggJointCtrl.SubModel == nullptr) ?
                    (Global.brake_speed * Command.time_delta * LocalBrakePosNo) : 1 );
                if( Train->ggJointCtrl.SubModel != nullptr ) {
                    Train->m_mastercontrollerinuse = true;
                }
            }
        }
    }
}

void TTrain::OnCommand_independentbrakeincreasefast( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {

        if( Train->mvOccupied->LocalBrake != TLocalBrake::ManualBrake ) {
            if( ( Train->ggJointCtrl.SubModel != nullptr )
                && ( Train->mvOccupied->MainCtrlPos > 0 ) ) {
                OnCommand_mastercontrollerdecreasefast( Train, Command );
            }
            else {
                Train->mvOccupied->IncLocalBrakeLevel( LocalBrakePosNo );
                if( Train->ggJointCtrl.SubModel != nullptr ) {
                    Train->m_mastercontrollerinuse = true;
                }
            }
        }
    }
}

void TTrain::OnCommand_independentbrakedecrease( TTrain *Train, command_data const &Command ) {

	if( Command.action != GLFW_RELEASE ) {

        if( ( Train->mvOccupied->LocalBrake != TLocalBrake::ManualBrake )
            // Ra 1014-06: AI potrafi zahamować pomocniczym mimo jego braku - odhamować jakoś trzeba
            // TODO: sort AI out so it doesn't do things it doesn't have equipment for
         || ( Train->mvOccupied->LocalBrakePosA > 0 ) ) {
            if( ( Train->ggJointCtrl.SubModel != nullptr )
             && ( Train->mvOccupied->LocalBrakePosA == 0.0 ) ) {
                OnCommand_mastercontrollerincrease( Train, Command );
            }
            else {
                Train->mvOccupied->DecLocalBrakeLevel(
                            (Train->ggJointCtrl.SubModel == nullptr) ?
                            (Global.brake_speed * Command.time_delta * LocalBrakePosNo) : 1);
                if( Train->ggJointCtrl.SubModel != nullptr ) {
                    Train->m_mastercontrollerinuse = true;
                }
            }
        }
    }
}

void TTrain::OnCommand_independentbrakedecreasefast( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {

        if( ( Train->mvOccupied->LocalBrake != TLocalBrake::ManualBrake )
            // Ra 1014-06: AI potrafi zahamować pomocniczym mimo jego braku - odhamować jakoś trzeba
            // TODO: sort AI out so it doesn't do things it doesn't have equipment for
         || ( Train->mvOccupied->LocalBrakePosA > 0 ) ) {
            if( ( Train->ggJointCtrl.SubModel != nullptr )
             && ( Train->mvOccupied->LocalBrakePosA == 0.0 ) ) {
                OnCommand_mastercontrollerincreasefast( Train, Command );
            }
            else {
                Train->mvOccupied->DecLocalBrakeLevel( LocalBrakePosNo );
                if( Train->ggJointCtrl.SubModel != nullptr ) {
                    Train->m_mastercontrollerinuse = true;
                }
            }
        }
    }
}

void TTrain::OnCommand_independentbrakeset( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {

        Train->mvOccupied->LocalBrakePosA = (
            clamp(
                Command.param1,
                0.0, 1.0 ) );
    }
/*
    Train->mvControlled->LocalBrakePos = (
        std::round(
            interpolate<double>(
                0.0,
                LocalBrakePosNo,
                clamp(
                    Command.param1,
                    0.0, 1.0 ) ) ) );
*/
}

void TTrain::OnCommand_independentbrakebailoff( TTrain *Train, command_data const &Command ) {

    if( false == Command.freefly ) {
        // TODO: check if this set of conditions can be simplified.
        // it'd be more flexible to have an attribute indicating whether bail off position is supported
        if( ( Train->mvControlled->TrainType != dt_EZT )
         && ( ( Train->mvControlled->EngineType == TEngineType::ElectricSeriesMotor )
           || ( Train->mvControlled->EngineType == TEngineType::DieselElectric )
           || ( Train->mvControlled->EngineType == TEngineType::ElectricInductionMotor ) )
         && ( Train->mvOccupied->BrakeCtrlPosNo > 0 ) ) {

            if( Command.action == GLFW_PRESS ) {
                // press or hold
                // visual feedback
                Train->ggReleaserButton.UpdateValue( 1.0, Train->dsbSwitch );

                Train->mvOccupied->BrakeReleaser( 1 );
            }
            else if( Command.action == GLFW_RELEASE ) {
                // release
                // visual feedback
                Train->ggReleaserButton.UpdateValue( 0.0, Train->dsbSwitch );

                Train->mvOccupied->BrakeReleaser( 0 );
            }
        }
    }
    else {
        // car brake handling, while in walk mode
		auto *vehicle { Train->find_nearest_consist_vehicle(Command.freefly, Command.location) };
        if( vehicle != nullptr ) {
            if( Command.action == GLFW_PRESS ) {
                // press or hold
                vehicle->MoverParameters->BrakeReleaser( 1 );
            }
            else if( Command.action == GLFW_RELEASE ) {
                // release
                vehicle->MoverParameters->BrakeReleaser( 0 );
            }
        }
    }
}

void TTrain::OnCommand_universalbrakebutton1(TTrain *Train, command_data const &Command) {

			if (Command.action == GLFW_PRESS) {
				// press or hold
				// visual feedback
				Train->ggUniveralBrakeButton1.UpdateValue(1.0, Train->dsbSwitch);

				Train->mvOccupied->UniversalBrakeButton(0,1);
			}
			else if (Command.action == GLFW_RELEASE) {
				// release
				// visual feedback
				Train->ggUniveralBrakeButton1.UpdateValue(0.0, Train->dsbSwitch);

				Train->mvOccupied->UniversalBrakeButton(0,0);
			}
}

void TTrain::OnCommand_universalbrakebutton2(TTrain *Train, command_data const &Command) {

	if (Command.action == GLFW_PRESS) {
		// press or hold
		// visual feedback
		Train->ggUniveralBrakeButton2.UpdateValue(1.0, Train->dsbSwitch);

		Train->mvOccupied->UniversalBrakeButton(1, 1);
	}
	else if (Command.action == GLFW_RELEASE) {
		// release
		// visual feedback
		Train->ggUniveralBrakeButton2.UpdateValue(0.0, Train->dsbSwitch);

		Train->mvOccupied->UniversalBrakeButton(1, 0);
	}
}

void TTrain::OnCommand_universalbrakebutton3(TTrain *Train, command_data const &Command) {

	if (Command.action == GLFW_PRESS) {
		// press or hold
		// visual feedback
		Train->ggUniveralBrakeButton3.UpdateValue(1.0, Train->dsbSwitch);

		Train->mvOccupied->UniversalBrakeButton(2, 1);
	}
	else if (Command.action == GLFW_RELEASE) {
		// release
		// visual feedback
		Train->ggUniveralBrakeButton3.UpdateValue(0.0, Train->dsbSwitch);

		Train->mvOccupied->UniversalBrakeButton(2, 0);
	}
}

void TTrain::OnCommand_trainbrakeincrease( TTrain *Train, command_data const &Command ) {
	if (Command.action == GLFW_REPEAT && Train->mvOccupied->BrakeHandle == TBrakeHandle::FV4a)
		Train->mvOccupied->BrakeLevelAdd( Global.brake_speed * Command.time_delta * Train->mvOccupied->BrakeCtrlPosNo );
	else if (Command.action == GLFW_PRESS && Train->mvOccupied->BrakeHandle != TBrakeHandle::FV4a)
		Train->set_train_brake( Train->mvOccupied->fBrakeCtrlPos + Global.fBrakeStep );
}

void TTrain::OnCommand_trainbrakedecrease( TTrain *Train, command_data const &Command ) {
	if (Command.action == GLFW_REPEAT && Train->mvOccupied->BrakeHandle == TBrakeHandle::FV4a)
		Train->mvOccupied->BrakeLevelAdd( -Global.brake_speed * Command.time_delta * Train->mvOccupied->BrakeCtrlPosNo );
	else if (Command.action == GLFW_PRESS && Train->mvOccupied->BrakeHandle != TBrakeHandle::FV4a)
		Train->set_train_brake( Train->mvOccupied->fBrakeCtrlPos - Global.fBrakeStep );
    else if (Command.action == GLFW_RELEASE) {
        // release
        Train->zero_charging_train_brake();
    }
}

void TTrain::OnCommand_trainbrakeset( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // press or hold
        Train->mvOccupied->BrakeLevelSet(
            interpolate(
                Train->mvOccupied->Handle->GetPos( bh_MIN ),
                Train->mvOccupied->Handle->GetPos( bh_MAX ),
                clamp(
                    Command.param1,
                    0.0, 1.0 ) ) );
    } else {
        // release
        Train->zero_charging_train_brake();
    }
}

void TTrain::OnCommand_trainbrakecharging( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {
        // press or hold
        Train->set_train_brake( -1 );
    }
    else {
        // release
        Train->zero_charging_train_brake();
    }
}

void TTrain::OnCommand_trainbrakerelease( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

        Train->set_train_brake( 0 );
    }
}

void TTrain::OnCommand_trainbrakefirstservice( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

        Train->set_train_brake( 1 );
    }
}

void TTrain::OnCommand_trainbrakeservice( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

        Train->set_train_brake( (
            Train->mvOccupied->BrakeCtrlPosNo / 2
            + ( Train->mvOccupied->BrakeHandle == TBrakeHandle::FV4a ?
                1 :
                0 ) ) );
    }
}

void TTrain::OnCommand_trainbrakefullservice( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

        Train->set_train_brake( Train->mvOccupied->BrakeCtrlPosNo - 1 );
    }
}

void TTrain::OnCommand_trainbrakehandleoff( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

        Train->set_train_brake( Train->mvOccupied->Handle->GetPos( bh_NP ) );
    }
}

void TTrain::OnCommand_trainbrakeemergency( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

        Train->set_train_brake( Train->mvOccupied->Handle->GetPos( bh_EB ) );
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
            case TBrakeHandle::FV4a: {
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
            case TBrakeHandle::FV4a: {
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

		auto *vehicle { Train->find_nearest_consist_vehicle(Command.freefly, Command.location) };
        if( vehicle == nullptr ) { return; }

        vehicle->MoverParameters->Hamulec->SetBrakeStatus( vehicle->MoverParameters->Hamulec->GetBrakeStatus() ^ b_dmg );
    }
}

void TTrain::OnCommand_manualbrakeincrease( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {

		auto *vehicle { Train->find_nearest_consist_vehicle(Command.freefly, Command.location) };
        if( vehicle == nullptr ) { return; }

        if( ( vehicle->MoverParameters->LocalBrake == TLocalBrake::ManualBrake )
         || ( vehicle->MoverParameters->MBrake == true ) ) {

            vehicle->MoverParameters->IncManualBrakeLevel( 1 );
        }
    }
}

void TTrain::OnCommand_manualbrakedecrease( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_RELEASE ) {

		auto *vehicle { Train->find_nearest_consist_vehicle(Command.freefly, Command.location) };
        if( vehicle == nullptr ) { return; }

        if( ( vehicle->MoverParameters->LocalBrake == TLocalBrake::ManualBrake )
         || ( vehicle->MoverParameters->MBrake == true ) ) {

            vehicle->MoverParameters->DecManualBrakeLevel( 1 );
        }
    }
}

void TTrain::OnCommand_alarmchaintoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

        if( false == Train->mvOccupied->AlarmChainFlag ) {
            OnCommand_alarmchainenable(Train, Command);
        }
        else {
            OnCommand_alarmchaindisable(Train, Command);
        }
    }
}

void TTrain::OnCommand_alarmchainenable(TTrain *Train, command_data const &Command)
{
	if (Command.action == GLFW_PRESS) {

        // pull
        Train->mvOccupied->AlarmChainSwitch(true);
        // visual feedback
        Train->ggAlarmChain.UpdateValue(1.0);
    }
}


void TTrain::OnCommand_alarmchaindisable(TTrain *Train, command_data const &Command) {
	if (Command.action == GLFW_PRESS) {

        // release
        Train->mvOccupied->AlarmChainSwitch(false);
        // visual feedback
        Train->ggAlarmChain.UpdateValue(0.0);
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

    if( Train->mvOccupied->BrakeSystem != TBrakeSystem::ElectroPneumatic ) {
        // standard behaviour
        if( Command.action == GLFW_PRESS ) {
            // visual feedback
            Train->ggAntiSlipButton.UpdateValue( 1.0, Train->dsbSwitch );

            // NOTE: system activation is (repeatedly) done in the train update routine
        }
        else if( Command.action == GLFW_RELEASE ) {
            // visual feedback
            Train->ggAntiSlipButton.UpdateValue( 0.0 );
        }
    }
    else {
        // electro-pneumatic, custom case
        if( Command.action == GLFW_PRESS ) {
            // visual feedback
            Train->ggAntiSlipButton.UpdateValue( 1.0, Train->dsbPneumaticSwitch );

            if( ( Train->mvOccupied->BrakeHandle == TBrakeHandle::St113 )
             && ( Train->mvControlled->EpFuse == true ) ) {
                Train->mvOccupied->SwitchEPBrake( 1 );
            }
        }
        else if( Command.action == GLFW_RELEASE ) {
            // visual feedback
            Train->ggAntiSlipButton.UpdateValue( 0.0 );

            Train->mvOccupied->SwitchEPBrake( 0 );
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
        // visual feedback
        Train->ggSandButton.UpdateValue( 1.0, Train->dsbSwitch );

        Train->mvControlled->SandboxManual( true );
    }
    else if( Command.action == GLFW_RELEASE) {
        // visual feedback
        Train->ggSandButton.UpdateValue( 0.0 );

        Train->mvControlled->SandboxManual( false );
    }
}

void TTrain::OnCommand_autosandboxtoggle(TTrain *Train, command_data const &Command) {

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (false == Train->mvOccupied->SandDoseAutoAllow) {
			// turn on
			OnCommand_autosandboxactivate(Train, Command);
		}
		else {
			//turn off
			OnCommand_autosandboxdeactivate(Train, Command);
		}
	}
};

void TTrain::OnCommand_autosandboxactivate(TTrain *Train, command_data const &Command) {
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		Train->mvOccupied->SandboxAutoAllow(true);
		Train->ggAutoSandButton.UpdateValue(1.0, Train->dsbSwitch);
	}
};

void TTrain::OnCommand_autosandboxdeactivate(TTrain *Train, command_data const &Command) {
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		Train->mvOccupied->SandboxAutoAllow(false);
		Train->ggAutoSandButton.UpdateValue(0.0, Train->dsbSwitch);
	}
};

void TTrain::OnCommand_epbrakecontroltoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }

    auto const ispush{ ( static_cast<int>( Train->ggEPFuseButton.type() ) & static_cast<int>( TGaugeType::push ) ) != 0 };
    auto const istoggle{ ( static_cast<int>( Train->ggEPFuseButton.type() ) & static_cast<int>( TGaugeType::toggle ) ) != 0 };

    if( Command.action == GLFW_PRESS ) {
        if( istoggle ) {
            // switch state
            if( false == Train->mvOccupied->EpFuse ) {
                // turn on
                if( Train->mvOccupied->EpFuseSwitch( true ) ) {
                    // audio feedback
                    if( Train->dsbPneumaticSwitch ) {
                        Train->dsbPneumaticSwitch->play();
                    }
                };
            }
            else {
                //turn off
                Train->mvOccupied->EpFuseSwitch( false );
            }
        }
        else if( ispush ) {
            // potentially turn on
            if( Train->mvOccupied->EpFuseSwitch( true ) ) {
                // audio feedback
                if( Train->dsbPneumaticSwitch ) {
                    Train->dsbPneumaticSwitch->play();
                }
            };
        }
        // visual feedback
        Train->ggEPFuseButton.UpdateValue( (
            ispush ? 1.0f : // push or pushtoggle
            Train->mvOccupied->EpFuse ? 1.0f : 0.0f ), // toggle
            Train->dsbSwitch );
    }
    else if( Command.action == GLFW_RELEASE ) {
        if( ispush ) {
            // return the switch to neutral position
            Train->ggEPFuseButton.UpdateValue( 0.0f, Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_trainbrakeoperationmodeincrease(TTrain *Train, command_data const &Command) {

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( ( Train->mvOccupied->BrakeOpModeFlag << 1 ) & Train->mvOccupied->BrakeOpModes ) != 0 ) {
            // next mode
            Train->mvOccupied->BrakeOpModeFlag <<= 1;
			// visual feedback
            Train->ggBrakeOperationModeCtrl.UpdateValue(
                Train->mvOccupied->BrakeOpModeFlag > 0 ?
                    std::log2( Train->mvOccupied->BrakeOpModeFlag ) :
                    0 ); // audio fallback
        }
	}
}

void TTrain::OnCommand_trainbrakeoperationmodedecrease(TTrain *Train, command_data const &Command) {

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( ( Train->mvOccupied->BrakeOpModeFlag >> 1 ) & Train->mvOccupied->BrakeOpModes ) != 0 ) {
			// previous mode
			Train->mvOccupied->BrakeOpModeFlag >>= 1;
			// visual feedback
            Train->ggBrakeOperationModeCtrl.UpdateValue(
                Train->mvOccupied->BrakeOpModeFlag > 0 ?
                    std::log2( Train->mvOccupied->BrakeOpModeFlag ) :
                    0 );
		}
	}
}

void TTrain::OnCommand_brakeactingspeedincrease( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

		auto *vehicle { Train->find_nearest_consist_vehicle(Command.freefly, Command.location) };
        if( vehicle == nullptr ) { return; }

        if( ( vehicle->MoverParameters->BrakeDelayFlag & bdelay_M ) != 0 ) {
            // can't speed it up any more than this
            return;
        }
        auto const fasterbrakesetting = (
            vehicle->MoverParameters->BrakeDelayFlag < bdelay_R ?
                vehicle->MoverParameters->BrakeDelayFlag << 1 :
                vehicle->MoverParameters->BrakeDelayFlag | bdelay_M );

        Train->set_train_brake_speed( vehicle, fasterbrakesetting );
    }
}

void TTrain::OnCommand_brakeactingspeeddecrease( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

		auto *vehicle { Train->find_nearest_consist_vehicle(Command.freefly, Command.location) };
        if( vehicle == nullptr ) { return; }

        if( vehicle->MoverParameters->BrakeDelayFlag == bdelay_G ) {
            // can't slow it down any more than this
            return;
        }
        auto const slowerbrakesetting = (
            vehicle->MoverParameters->BrakeDelayFlag < bdelay_M ?
                vehicle->MoverParameters->BrakeDelayFlag >> 1 :
                vehicle->MoverParameters->BrakeDelayFlag ^ bdelay_M );

        Train->set_train_brake_speed( vehicle, slowerbrakesetting );
    }
}

void TTrain::OnCommand_brakeactingspeedsetcargo( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

		auto *vehicle { Train->find_nearest_consist_vehicle(Command.freefly, Command.location) };
        if( vehicle == nullptr ) { return; }

        Train->set_train_brake_speed( vehicle, bdelay_G );
    }
}

void TTrain::OnCommand_brakeactingspeedsetpassenger( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

		auto *vehicle { Train->find_nearest_consist_vehicle(Command.freefly, Command.location) };
        if( vehicle == nullptr ) { return; }

        Train->set_train_brake_speed( vehicle, bdelay_P );
    }
}

void TTrain::OnCommand_brakeactingspeedsetrapid( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

		auto *vehicle{ Train->find_nearest_consist_vehicle(Command.freefly, Command.location) };
        if( vehicle == nullptr ) { return; }

        Train->set_train_brake_speed( vehicle, bdelay_R );
    }
}

void TTrain::OnCommand_brakeloadcompensationincrease( TTrain *Train, command_data const &Command ) {

    if( ( true == Command.freefly )
     && ( Command.action == GLFW_PRESS ) ) {
		auto *vehicle { Train->find_nearest_consist_vehicle(Command.freefly, Command.location) };
        if( vehicle != nullptr ) {
            vehicle->MoverParameters->IncBrakeMult();
        }
    }
}

void TTrain::OnCommand_brakeloadcompensationdecrease( TTrain *Train, command_data const &Command ) {

    if( ( true == Command.freefly )
     && ( Command.action == GLFW_PRESS ) ) {
		auto *vehicle { Train->find_nearest_consist_vehicle(Command.freefly, Command.location) };
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

        // HACK: master controller position isn't set in occupied vehicle in E(D)MUs
        // so we do a manual check in relevant vehicle here
        if( false == Train->mvControlled->EIMDirectionChangeAllow() ) { return; }

        if( Train->mvOccupied->DirectionForward() ) {
            // aktualizacja skrajnych pojazdów w składzie
            if( ( Train->mvOccupied->DirActive )
             && ( Train->DynamicObject->Mechanik ) ) {

                Train->DynamicObject->Mechanik->DirectionChange();
            }
        }
    }
}

void TTrain::OnCommand_reverserdecrease( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

        // HACK: master controller position isn't set in occupied vehicle in E(D)MUs
        // so we do a manual check in relevant vehicle here
        if( false == Train->mvControlled->EIMDirectionChangeAllow() ) { return; }

        if( Train->mvOccupied->DirectionBackward() ) {
            // aktualizacja skrajnych pojazdów w składzie
            if( ( Train->mvOccupied->DirActive )
             && ( Train->DynamicObject->Mechanik ) ) {

                Train->DynamicObject->Mechanik->DirectionChange();;
            }
        }
    }
}

void TTrain::OnCommand_reverserforwardhigh( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

        // HACK: master controller position isn't set in occupied vehicle in E(D)MUs
        // so we do a manual check in relevant vehicle here
        if( false == Train->mvControlled->EIMDirectionChangeAllow() ) { return; }

        // HACK: try to move the reverser one position back, in case it's set to "high forward"
        OnCommand_reverserdecrease( Train, Command );

        if( Train->mvOccupied->DirActive < 1 ) {

            while( ( Train->mvOccupied->DirActive < 1 )
                && ( true == Train->mvOccupied->DirectionForward() ) ) {
                // all work is done in the header
            }
            // aktualizacja skrajnych pojazdów w składzie
            if( ( Train->mvOccupied->DirActive == 1 )
             && ( Train->DynamicObject->Mechanik ) ) {

                Train->DynamicObject->Mechanik->DirectionChange();
            }
        }
    OnCommand_reverserincrease( Train, Command );
    }
}

void TTrain::OnCommand_reverserforward( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

        // HACK: master controller position isn't set in occupied vehicle in E(D)MUs
        // so we do a manual check in relevant vehicle here
        if( false == Train->mvControlled->EIMDirectionChangeAllow() ) { return; }

        // HACK: try to move the reverser one position back, in case it's set to "high forward"
        //OnCommand_reverserdecrease( Train, Command );
		// visual feedback
		Train->ggDirForwardButton.UpdateValue(1.0, Train->dsbSwitch);

        if( Train->mvOccupied->DirActive == 0 ) {

            while( ( Train->mvOccupied->DirActive < 1 )
                && ( true == Train->mvOccupied->DirectionForward() ) ) {
                // all work is done in the header
            }
            // aktualizacja skrajnych pojazdów w składzie
            if( ( Train->mvOccupied->DirActive == 1 )
             && ( Train->DynamicObject->Mechanik ) ) {

                Train->DynamicObject->Mechanik->DirectionChange();
            }
        }
    }
	else if (Command.action == GLFW_RELEASE) {
		// release
		// visual feedback
		Train->ggDirForwardButton.UpdateValue(0.0, Train->dsbSwitch);
	}
}

void TTrain::OnCommand_reverserneutral( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

        // HACK: master controller position isn't set in occupied vehicle in E(D)MUs
        // so we do a manual check in relevant vehicle here
        if( false == Train->mvControlled->EIMDirectionChangeAllow() ) { return; }
		// visual feedback
		Train->ggDirNeutralButton.UpdateValue(1.0, Train->dsbSwitch);
        while( ( Train->mvOccupied->DirActive < 0 )
            && ( true == Train->mvOccupied->DirectionForward() ) ) {
            // all work is done in the header
        }
        while( ( Train->mvOccupied->DirActive > 0 )
            && ( true == Train->mvOccupied->DirectionBackward() ) ) {
            // all work is done in the header
        }
    }
	else if (Command.action == GLFW_RELEASE) {
		// release
		// visual feedback
		Train->ggDirNeutralButton.UpdateValue(0.0, Train->dsbSwitch);
	}
}

void TTrain::OnCommand_reverserbackward( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

        // HACK: master controller position isn't set in occupied vehicle in E(D)MUs
        // so we do a manual check in relevant vehicle here
        if( false == Train->mvControlled->EIMDirectionChangeAllow() ) { return; }
        Train->ggDirBackwardButton.UpdateValue(1.0, Train->dsbSwitch);
        if( Train->mvOccupied->DirActive == 0 ) {

            while( ( Train->mvOccupied->DirActive > -1 )
                && ( true == Train->mvOccupied->DirectionBackward() ) ) {
                // all work is done in the header
            }
            // aktualizacja skrajnych pojazdów w składzie
            if( ( Train->mvOccupied->DirActive == -1 )
             && ( Train->DynamicObject->Mechanik ) ) {

                Train->DynamicObject->Mechanik->DirectionChange();
            }
        }
    }
	else if (Command.action == GLFW_RELEASE) {
		// release
		// visual feedback
		Train->ggDirBackwardButton.UpdateValue(0.0, Train->dsbSwitch);
	}
}

void TTrain::OnCommand_alerteracknowledge( TTrain *Train, command_data const &Command ) {
    if( Command.action == GLFW_PRESS ) {
        // visual feedback
        Train->ggSecurityResetButton.UpdateValue( 1.0, Train->dsbSwitch );

        if (Train->mvOccupied->TrainType == dt_EZT || Train->mvOccupied->DirActive != 0)
			Train->mvOccupied->SecuritySystem.acknowledge_press();
    }
    else if( Command.action == GLFW_RELEASE ) {
        // visual feedback
        Train->ggSecurityResetButton.UpdateValue( 0.0 );

        if (Train->mvOccupied->TrainType == dt_EZT || Train->mvOccupied->DirActive != 0)
			Train->mvOccupied->SecuritySystem.acknowledge_release();
    }
}

void TTrain::OnCommand_cabsignalacknowledge( TTrain *Train, command_data const &Command ) {
	// TODO: visual feedback
	if( Command.action == GLFW_PRESS ) {
        if(Train->mvOccupied->SecuritySystem.has_separate_acknowledge()) {
            Train->mvOccupied->SecuritySystem.cabsignal_reset();
            Train->ggSHPResetButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
	} else if( Command.action == GLFW_RELEASE ) {
        Train->ggSHPResetButton.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_batterytoggle( TTrain *Train, command_data const &Command )
{
    if( Command.action != GLFW_REPEAT ) {
        // keep the switch from flipping back and forth if key is held down
        if( false == Train->mvOccupied->Power24vIsAvailable ) {
            // turn on
            OnCommand_batteryenable( Train, Command );
        }
        else {
            //turn off
            OnCommand_batterydisable( Train, Command );
        }
    }
}

void TTrain::OnCommand_batteryenable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // visual feedback
        Train->ggBatteryButton.UpdateValue( 1.0f, Train->dsbSwitch );
        Train->ggBatteryOnButton.UpdateValue( 1.0f, Train->dsbSwitch );

        Train->mvOccupied->BatterySwitch( true );

        // side-effects
        if( Train->mvOccupied->LightsPosNo > 0 ) {
            Train->Dynamic()->SetLights();
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        if( Train->ggBatteryButton.type() == TGaugeType::push ) {
            // return the switch to neutral position
            Train->ggBatteryButton.UpdateValue( 0.5f );
        }
        Train->ggBatteryOnButton.UpdateValue( 0.0f, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_batterydisable( TTrain *Train, command_data const &Command ) {
    // TBD, TODO: ewentualnie zablokować z FIZ, np. w samochodach się nie odłącza akumulatora
    if( Command.action == GLFW_PRESS ) {
        // visual feedback
        Train->ggBatteryButton.UpdateValue( 0.0f, Train->dsbSwitch );
        Train->ggBatteryOffButton.UpdateValue( 1.0f, Train->dsbSwitch );

        Train->mvOccupied->BatterySwitch( false );
    }
    else if( Command.action == GLFW_RELEASE ) {
        if( Train->ggBatteryButton.type() == TGaugeType::push ) {
            // return the switch to neutral position
            Train->ggBatteryButton.UpdateValue( 0.5f );
        }
        Train->ggBatteryOffButton.UpdateValue( 0.0f, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_cabactivationtoggle(TTrain *Train, command_data const &Command) {

	if (Command.action != GLFW_REPEAT) {
		// keep the switch from flipping back and forth if key is held down
		if (0 == Train->mvOccupied->CabActive) {
			// turn on
			OnCommand_cabactivationenable(Train, Command);
		}
		else {
			//turn off
			OnCommand_cabactivationdisable(Train, Command);
		}
	}
}

void TTrain::OnCommand_cabactivationenable(TTrain *Train, command_data const &Command) {

	if (Command.action == GLFW_PRESS) {
		// visual feedback
		if (Train->ggCabActivationButton.type() == TGaugeType::push) {
			Train->ggCabActivationButton.UpdateValue(1.0f, Train->dsbSwitch);
		}

		Train->mvOccupied->CabActivisation();

		// side-effects
		if (Train->mvOccupied->LightsPosNo > 0) {
			Train->Dynamic()->SetLights();
		}
	}
	else if (Command.action == GLFW_RELEASE) {
		if (Train->ggCabActivationButton.type() == TGaugeType::push) {
			// return the switch to neutral position
			Train->ggCabActivationButton.UpdateValue(0.5f);
		}
	}
}

void TTrain::OnCommand_cabactivationdisable(TTrain *Train, command_data const &Command) {
	// TBD, TODO: ewentualnie zablokować z FIZ, np. w samochodach się nie odłącza akumulatora
	if (Command.action == GLFW_PRESS) {
		// visual feedback
		if (Train->ggCabActivationButton.type() == TGaugeType::push) {
			Train->ggCabActivationButton.UpdateValue(0.0f, Train->dsbSwitch);
		}

		Train->mvOccupied->CabDeactivisation();
		if ((Train->mvOccupied->LightsPosNo > 0) && (Train->mvOccupied->InactiveCabFlag & activation::redmarkers)) {
			Train->Dynamic()->SetLights();
		}
	}
	else if (Command.action == GLFW_RELEASE) {
		if (Train->ggCabActivationButton.type() == TGaugeType::push) {
			// return the switch to neutral position
			Train->ggCabActivationButton.UpdateValue(0.5f);
		}
	}
}

void TTrain::OnCommand_pantographtogglefront( TTrain *Train, command_data const &Command ) {

    // HACK: presence of pantograph selector prevents manual operation of the individual valves
    if( Train->m_controlmapper.contains( "pantselect_sw:" ) ) { return; }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        auto const &pantograph { Train->mvPantographUnit->Pantographs[ end::front ] };
        auto const state {
            pantograph.valve.is_enabled
         || pantograph.is_active }; // fallback for impulse switches
        if( state ) {
            OnCommand_pantographlowerfront( Train, Command );
        }
        else {
            OnCommand_pantographraisefront( Train, Command );
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // impulse switches return automatically to neutral position
        if( Train->mvOccupied->PantSwitchType == "impulse" ) {
            auto const ismanual { Train->iCabn == 0 };
            Train->mvOccupied->OperatePantographValve(
                end::front,
                operation_t::none,
                ( ismanual ?
                    range_t::local :
                    range_t::consist ) );
        }
    }
}

void TTrain::OnCommand_pantographtogglerear( TTrain *Train, command_data const &Command ) {

    // HACK: presence of pantograph selector prevents manual operation of the individual valves
    if( Train->m_controlmapper.contains( "pantselect_sw:" ) ) { return; }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        auto const &pantograph { Train->mvPantographUnit->Pantographs[ end::rear ] };
        auto const state {
            pantograph.valve.is_enabled
         || pantograph.is_active }; // fallback for impulse switches
        if( state ) {
            OnCommand_pantographlowerrear( Train, Command );
        }
        else {
            OnCommand_pantographraiserear( Train, Command );
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // impulse switches return automatically to neutral position
        if( Train->mvOccupied->PantSwitchType == "impulse" ) {
            auto const ismanual { Train->iCabn == 0 };
            Train->mvOccupied->OperatePantographValve(
                end::rear,
                operation_t::none,
                ( ismanual ?
                    range_t::local :
                    range_t::consist ) );
        }
    }
}

void TTrain::OnCommand_pantographraisefront( TTrain *Train, command_data const &Command ) {

    // HACK: presence of pantograph selector prevents manual operation of the individual valves
    if( Train->m_controlmapper.contains( "pantselect_sw:" ) ) { return; }
    // prevent operation without submodel outside of engine compartment
    if( ( Train->iCabn != 0 )
     && ( false == Train->m_controlmapper.contains( "pantfront_sw:" ) ) ) { return; }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        // HACK: don't propagate pantograph commands issued from engine compartment, these are presumed to be manually moved levers
        auto const ismanual { Train->iCabn == 0 };
        Train->mvOccupied->OperatePantographValve(
            end::front,
            ( Train->mvOccupied->PantSwitchType == "impulse" ?
                operation_t::enable_on :
                operation_t::enable ),
            ( ismanual ?
                range_t::local :
                range_t::consist ) );
    }
    else if( Command.action == GLFW_RELEASE ) {
        // NOTE: bit of a hax here, we're reusing button reset routine so we don't need a copy in every branch
        OnCommand_pantographtogglefront( Train, Command );
    }
}

void TTrain::OnCommand_pantographraiserear( TTrain *Train, command_data const &Command ) {

    // HACK: presence of pantograph selector prevents manual operation of the individual valves
    if( Train->m_controlmapper.contains( "pantselect_sw:" ) ) { return; }
    // prevent operation without submodel outside of engine compartment
    if( ( Train->iCabn != 0 )
     && ( false == Train->m_controlmapper.contains( "pantrear_sw:" ) ) ) { return; }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        // HACK: don't propagate pantograph commands issued from engine compartment, these are presumed to be manually moved levers
        auto const ismanual { Train->iCabn == 0 };
        Train->mvOccupied->OperatePantographValve(
            end::rear,
            ( Train->mvOccupied->PantSwitchType == "impulse" ?
                operation_t::enable_on :
                operation_t::enable ),
            ( ismanual ?
                range_t::local :
                range_t::consist ) );
   }
    else if( Command.action == GLFW_RELEASE ) {
        // NOTE: bit of a hax here, we're reusing button reset routine so we don't need a copy in every branch
        OnCommand_pantographtogglerear( Train, Command );
    }
}

void TTrain::OnCommand_pantographlowerfront( TTrain *Train, command_data const &Command ) {

    // HACK: presence of pantograph selector prevents manual operation of the individual valves
    if( Train->m_controlmapper.contains( "pantselect_sw:" ) ) { return; }
    // prevent operation without submodel outside of engine compartment
    if( ( Train->iCabn != 0 )
     && ( false == Train->m_controlmapper.contains(
         Train->mvOccupied->PantSwitchType == "impulse" ?
            "pantfrontoff_sw:" :
            "pantfront_sw:" ) ) ) {
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        // HACK: don't propagate pantograph commands issued from engine compartment, these are presumed to be manually moved levers
        auto const ismanual { Train->iCabn == 0 };
        Train->mvOccupied->OperatePantographValve(
            end::front,
            ( Train->mvOccupied->PantSwitchType == "impulse" ?
                operation_t::disable_on :
                operation_t::disable ),
            ( ismanual ?
                range_t::local :
                range_t::consist ) );
    }
    else if( Command.action == GLFW_RELEASE ) {
        // NOTE: bit of a hax here, we're reusing button reset routine so we don't need a copy in every branch
        OnCommand_pantographtogglefront( Train, Command );
    }
}

void TTrain::OnCommand_pantographlowerrear( TTrain *Train, command_data const &Command ) {

    // HACK: presence of pantograph selector prevents manual operation of the individual valves
    if( Train->m_controlmapper.contains( "pantselect_sw:" ) ) { return; }

    if( ( Train->iCabn != 0 )
     && ( false == Train->m_controlmapper.contains(
         Train->mvOccupied->PantSwitchType == "impulse" ?
            "pantrearoff_sw:" :
            "pantrear_sw:" ) ) ) {
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        // HACK: don't propagate pantograph commands issued from engine compartment, these are presumed to be manually moved levers
        auto const ismanual { Train->iCabn == 0 };
        Train->mvOccupied->OperatePantographValve(
            end::rear,
            ( Train->mvOccupied->PantSwitchType == "impulse" ?
                operation_t::disable_on :
                operation_t::disable ),
            ( ismanual ?
                range_t::local :
                range_t::consist ) );
    }
    else if( Command.action == GLFW_RELEASE ) {
        // NOTE: bit of a hax here, we're reusing button reset routine so we don't need a copy in every branch
        OnCommand_pantographtogglerear( Train, Command );
    }
}

void TTrain::OnCommand_pantographlowerall( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }

    if( Train->ggPantAllDownButton.SubModel == nullptr ) {
        // TODO: expand definition of cab controls so we can know if the control is present without testing for presence of 3d switch
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Lower All Pantographs switch is missing, or wasn't defined" );
        }
        return;
    }

    if( Train->ggPantAllDownButton.type() == TGaugeType::toggle ) {
        // two-state switch, only cares about press events
        if( Command.action == GLFW_PRESS ) {
            Train->mvPantographUnit->DropAllPantographs( false == Train->mvPantographUnit->PantAllDown );
            // visual feedback
            Train->ggPantAllDownButton.UpdateValue( ( Train->mvPantographUnit->PantAllDown ? 1.0 : 0.0 ), Train->dsbSwitch );
        }
    }
    else {
        // impulse switch
        Train->mvControlled->DropAllPantographs( Command.action == GLFW_PRESS );
        // visual feedback
        Train->ggPantAllDownButton.UpdateValue( ( Command.action == GLFW_PRESS ? 1.0 : 0.0 ), Train->dsbSwitch );
    }
}

void TTrain::OnCommand_pantographselectnext( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_PRESS ) { return; }

    if( false == Train->m_controlmapper.contains( "pantselect_sw:" ) ) { return; }

    Train->change_pantograph_selection( 1 );
}

void TTrain::OnCommand_pantographselectprevious( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_PRESS )  { return; }

    if( false == Train->m_controlmapper.contains( "pantselect_sw:" ) ) { return; }

    Train->change_pantograph_selection( -1 );
}

void TTrain::OnCommand_pantographtoggleselected( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        auto const state {
            Train->mvPantographUnit->PantsValve.is_enabled
         || Train->mvPantographUnit->PantsValve.is_active }; // fallback for impulse switches
        if( state ) {
            OnCommand_pantographlowerselected( Train, Command );
        }
        else {
            OnCommand_pantographraiseselected( Train, Command );
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // impulse switches return automatically to neutral position
        if( Train->m_controlmapper.contains( "pantselectedoff_sw:" ) ) {
            // two buttons setup
            if( Train->ggPantSelectedButton.type() != TGaugeType::toggle ) {
                Train->mvOccupied->OperatePantographsValve( operation_t::enable_off );
                // visual feedback
                Train->ggPantSelectedButton.UpdateValue( 0.0, Train->dsbSwitch );
            }
            if( Train->ggPantSelectedDownButton.type() != TGaugeType::toggle ) {
                Train->mvOccupied->OperatePantographsValve( operation_t::disable_off );
                // visual feedback
                Train->ggPantSelectedDownButton.UpdateValue( 0.0, Train->dsbSwitch );
            }
        }
        else {
            if( Train->ggPantSelectedButton.type() != TGaugeType::toggle ) {
                // special case, just one impulse switch controlling both states
                // with neutral position mid-way
                Train->mvOccupied->OperatePantographsValve( operation_t::none );
                // visual feedback
                Train->ggPantSelectedButton.UpdateValue( 0.5, Train->dsbSwitch );
            }
        }
    }
}

void TTrain::OnCommand_pantographraiseselected( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }

    if( Command.action == GLFW_PRESS ) {
        // raise selected
        Train->mvOccupied->OperatePantographsValve(
            Train->ggPantSelectedButton.type() != TGaugeType::toggle ?
                operation_t::enable_on :
                operation_t::enable );
        // visual feedback
        Train->ggPantSelectedButton.UpdateValue( 1.0, Train->dsbSwitch );
    }
    else if( Command.action == GLFW_RELEASE ) {
        // NOTE: bit of a hax here, we're reusing button reset routine so we don't need a copy in every branch
        OnCommand_pantographtoggleselected( Train, Command );
    }
}

void TTrain::OnCommand_pantographlowerselected( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }

    if( Command.action == GLFW_PRESS ) {
        // lower selected
        Train->mvOccupied->OperatePantographsValve(
            Train->ggPantSelectedDownButton.type() != TGaugeType::toggle ?
                operation_t::disable_on :
                operation_t::disable );
        // visual feedback
        if( Train->m_controlmapper.contains( "pantselectedoff_sw:" ) ) {
            // two button setup
            Train->ggPantSelectedDownButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            // single button
            Train->ggPantSelectedButton.UpdateValue( 0.0, Train->dsbSwitch );
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // NOTE: bit of a hax here, we're reusing button reset routine so we don't need a copy in every branch
        OnCommand_pantographtoggleselected( Train, Command );
    }
}

void TTrain::update_pantograph_valves() {

    auto const &presets { mvOccupied->PantsPreset.first };
    auto &selection { mvOccupied->PantsPreset.second[ cab_to_end() ] };

    auto const preset { presets[ selection ] - '0' };
    auto const swapends { cab_to_end() != end::front };
    // check desired states for both pantographs; value: whether the pantograph should be raised
    auto const frontstate { preset & ( swapends ? 2 : 1 ) };
    auto const rearstate { preset & ( swapends ? 1 : 2 ) };
    mvOccupied->OperatePantographValve( end::front, ( frontstate ? operation_t::enable : operation_t::disable ) );
    mvOccupied->OperatePantographValve( end::rear, ( rearstate ? operation_t::enable : operation_t::disable ) );
}

void TTrain::change_pantograph_selection( int const Change ) {

    auto const &presets { mvOccupied->PantsPreset.first };
    auto &selection { mvOccupied->PantsPreset.second[ cab_to_end() ] };
    auto const initialstate { selection };
    selection = clamp<int>( selection + Change, 0, std::max<int>( presets.size() - 1, 0 ) );

    if( selection == initialstate ) { return; } // no change, nothing to do

    // potentially adjust pantograph valves to match the new state
    if( false == m_controlmapper.contains( "pantvalves_sw:" ) ) {
        update_pantograph_valves();
    }
}

void TTrain::OnCommand_pantographvalvesupdate( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }

    if( Command.action == GLFW_PRESS ) {
        // implement action
        Train->update_pantograph_valves();
        // visual feedback
        Train->ggPantValvesButton.UpdateValue( 1.0, Train->dsbSwitch );
    }
    else if( Command.action == GLFW_RELEASE ) {
        // visual feedback
        // NOTE: pantvalves_sw: is a specialized button, with no toggle behavior support
        Train->ggPantValvesButton.UpdateValue( 0.5, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_pantographvalvesoff( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }

    if( Command.action == GLFW_PRESS ) {
        // implement action
        Train->mvOccupied->OperatePantographValve( end::front, operation_t::disable );
        Train->mvOccupied->OperatePantographValve( end::rear, operation_t::disable );
        // visual feedback
        Train->ggPantValvesButton.UpdateValue( 0.0, Train->dsbSwitch );
    }
    else if( Command.action == GLFW_RELEASE ) {
        // visual feedback
        // NOTE: pantvalves_sw: is a specialized button, with no toggle behavior support
        Train->ggPantValvesButton.UpdateValue( 0.5, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_pantographcompressorvalvetoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only react to press
        if( Train->mvControlled->bPantKurek3 == false ) {
            // connect pantographs with primary tank
            OnCommand_pantographcompressorvalveenable( Train, Command );
        }
        else {
            // connect pantograps with pantograph compressor
            OnCommand_pantographcompressorvalvedisable( Train, Command );
        }
    }
}

void TTrain::OnCommand_pantographcompressorvalveenable( TTrain *Train, command_data const &Command ) {

    auto const valveispresent {
        ( Train->ggPantCompressorValve.SubModel != nullptr )
     || ( ( Train->mvOccupied == Train->mvPantographUnit )
       && ( Train->iCabn == 0 ) ) };

    if( false == valveispresent ) {
        // tylko w maszynowym, unless actual device is present
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only react to press
        // connect pantographs with primary tank
        Train->mvControlled->bPantKurek3 = true;
        // visual feedback:
        Train->ggPantCompressorValve.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_pantographcompressorvalvedisable( TTrain *Train, command_data const &Command ) {

    auto const valveispresent {
        ( Train->ggPantCompressorValve.SubModel != nullptr )
     || ( ( Train->mvOccupied == Train->mvPantographUnit )
       && ( Train->iCabn == 0 ) ) };

    if( false == valveispresent ) {
        // tylko w maszynowym, unless actual device is present
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only react to press
        // connect pantograps with pantograph compressor
        Train->mvControlled->bPantKurek3 = false;
        // visual feedback:
        Train->ggPantCompressorValve.UpdateValue( 1.0 );
    }
}

void TTrain::OnCommand_pantographcompressoractivate( TTrain *Train, command_data const &Command ) {

    // tylko w maszynowym, unless actual device is present
    auto const switchispresent {
        ( Train->m_controlmapper.contains( "pantcompressor_sw:" ) )
     || ( ( Train->mvOccupied == Train->mvPantographUnit )
       && ( Train->iCabn == 0 ) ) };
    if( false == switchispresent ) {
        return;
    }

    if( Command.action != GLFW_RELEASE ) {
        // press or hold to activate
        if( ( Train->mvPantographUnit->PantPress < 4.8 )
         && ( true == Train->mvPantographUnit->Power24vIsAvailable ) ) {
            // needs live power source and low enough pressure to work
            Train->mvPantographUnit->PantCompFlag = true;
        }
        // visual feedback
        Train->ggPantCompressorButton.UpdateValue( 1.0 );
    }
    else {
        // release to disable
        Train->mvPantographUnit->PantCompFlag = false;
        // visual feedback
        Train->ggPantCompressorButton.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_linebreakertoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // press or hold...
        if( Train->m_linebreakerstate == 0 ) {
            // ...to close the circuit
            // NOTE: bit of a dirty shortcut here
            OnCommand_linebreakerclose( Train, Command );
        }
        else if( Train->m_linebreakerstate == 1 ) {
            // ...to open the circuit
            OnCommand_linebreakeropen( Train, Command );
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // release...
        if( ( Train->ggMainOnButton.SubModel != nullptr )
         || ( Train->ggMainButton.type() != TGaugeType::toggle ) ) {
            // only impulse switches react to release events
            // NOTE: we presume dedicated state switch is of impulse type
            if( Train->m_linebreakerstate == 0 ) {
                // ...after opening circuit, or holding for too short time to close it
                OnCommand_linebreakeropen( Train, Command );
            }
            else {
                // ...after closing the circuit
                // NOTE: bit of a dirty shortcut here
                OnCommand_linebreakerclose( Train, Command );
            }
        }
        // HACK: ignition key ignores lack of submodel, so we can start vehicles without any modeled controls
        Train->ggIgnitionKey.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_linebreakeropen( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // visual feedback
        if( Train->ggMainOffButton.SubModel != nullptr ) {
            Train->ggMainOffButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else if( Train->ggMainButton.SubModel != nullptr ) {
            Train->ggMainButton.UpdateValue( 0.0, Train->dsbSwitch );
        }
        else if( Train->ggMainOnButton.SubModel != nullptr ) {
            // NOTE: legacy behaviour, for vehicles equipped only with impulse close switch
            // it doesn't make any real sense to animate this one, but some people can't get over how there's no visual reaction to their keypress
            Train->ggMainOnButton.UpdateValue( 1.0, Train->dsbSwitch );
            return;
        }
        // play sound immediately when the switch is hit, not after release
        Train->fMainRelayTimer = 0.0f;

        if( Train->m_linebreakerstate == 0 ) { return; } // already in the desired state

        if( true == Train->mvControlled->MainSwitch( false ) ) {
            Train->m_linebreakerstate = 0;
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // visual feedback
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
            Train->ggMainButton.UpdateValue( (
                Train->ggMainButton.type() != TGaugeType::toggle ?
                    0.5 :
                    0.0 ),
                Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_linebreakerclose( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // visual feedback
        if( Train->ggMainOnButton.SubModel != nullptr ) {
            // two separate switches to close and break the circuit
            Train->ggMainOnButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else if( Train->ggMainButton.SubModel != nullptr ) {
            // single two-state switch
            Train->ggMainButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            // no switch capable of doing the job
            // HACK: ignition key ignores lack of submodel, so we can start vehicles without any modeled controls
            Train->ggIgnitionKey.UpdateValue( 1.0 );
            return;
        }
        // the actual closing of the line breaker is handled in the train update routine
    }
    else if( Command.action == GLFW_RELEASE ) {
        // visual feedback
        if( Train->ggMainOnButton.SubModel != nullptr ) {
            // setup with two separate switches
            Train->ggMainOnButton.UpdateValue( 0.0, Train->dsbSwitch );
        }
        else if( Train->ggMainButton.SubModel != nullptr ) {
            if( Train->ggMainButton.type() != TGaugeType::toggle ) {
                Train->ggMainButton.UpdateValue( 0.5, Train->dsbSwitch );
            }
        }

        if( Train->m_linebreakerstate == 1 ) { return; } // already in the desired state

        if( Train->m_linebreakerstate == 2 ) {
            // we don't need to start the diesel twice, but the other types (with impulse switch setup) still need to be launched
            // NOTE: this behaviour should depend on MainOnButton presence and type_delayed
            // TODO: change it when/if vehicle definition files get their proper switch types
            if( Train->mvControlled->EngineType == TEngineType::ElectricSeriesMotor ) {
                // try to finalize state change of the line breaker, set the state based on the outcome
                Train->m_linebreakerstate = (
                    Train->mvControlled->MainSwitch( true ) ?
                        1 :
                        0 );
            }
        }
        // on button release reset the closing timer
        Train->fMainRelayTimer = 0.0f;
    }
}

void TTrain::OnCommand_fuelpumptoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }

    if( Train->ggFuelPumpButton.type() == TGaugeType::push ) {
        // impulse switch
        // currently there's no off button so we always try to turn it on
        OnCommand_fuelpumpenable( Train, Command );
    }
    else {
        // two-state switch
        if( Command.action == GLFW_RELEASE ) { return; }

        if( false == Train->mvControlled->FuelPump.is_enabled ) {
            // turn on
            OnCommand_fuelpumpenable( Train, Command );
        }
        else {
            //turn off
            OnCommand_fuelpumpdisable( Train, Command );
        }
    }
}

void TTrain::OnCommand_fuelpumpenable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }

    if( Train->ggFuelPumpButton.type() == TGaugeType::push ) {
        // impulse switch
        if( Command.action == GLFW_PRESS ) {
            // visual feedback
            Train->ggFuelPumpButton.UpdateValue( 1.0, Train->dsbSwitch );
            Train->mvControlled->FuelPumpSwitch( true );
        }
        else if( Command.action == GLFW_RELEASE ) {
            // visual feedback
            Train->ggFuelPumpButton.UpdateValue( 0.0, Train->dsbSwitch );
            Train->mvControlled->FuelPumpSwitch( false );
        }
    }
    else {
        // two-state switch, only cares about press events
        if( Command.action == GLFW_PRESS ) {
            // visual feedback
            Train->ggFuelPumpButton.UpdateValue( 1.0, Train->dsbSwitch );
            Train->mvControlled->FuelPumpSwitch( true );
            Train->mvControlled->FuelPumpSwitchOff( false );
        }
    }
}

void TTrain::OnCommand_fuelpumpdisable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }

    if( Train->ggFuelPumpButton.type() == TGaugeType::push ) {
        // impulse switch
        // currently there's no disable return type switch
        return;
    }
    else {
        // two-state switch, only cares about press events
        if( Command.action == GLFW_PRESS ) {
            // visual feedback
            Train->ggFuelPumpButton.UpdateValue( 0.0, Train->dsbSwitch );
            Train->mvControlled->FuelPumpSwitch( false );
            Train->mvControlled->FuelPumpSwitchOff( true );
        }
    }
}

void TTrain::OnCommand_oilpumptoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }

    if( Train->ggOilPumpButton.type() == TGaugeType::push ) {
        // impulse switch
        // currently there's no off button so we always try to turn it on
        OnCommand_oilpumpenable( Train, Command );
    }
    else {
        // two-state switch
        if( Command.action == GLFW_RELEASE ) { return; }

        if( false == Train->mvControlled->OilPump.is_enabled ) {
            // turn on
            OnCommand_oilpumpenable( Train, Command );
        }
        else {
            //turn off
            OnCommand_oilpumpdisable( Train, Command );
        }
    }
}

void TTrain::OnCommand_oilpumpenable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }

    if( Train->ggOilPumpButton.type() == TGaugeType::push ) {
        // impulse switch
        if( Command.action == GLFW_PRESS ) {
            // visual feedback
            Train->ggOilPumpButton.UpdateValue( 1.0, Train->dsbSwitch );
            Train->mvControlled->OilPumpSwitch( true );
        }
        else if( Command.action == GLFW_RELEASE ) {
            // visual feedback
            Train->ggOilPumpButton.UpdateValue( 0.0, Train->dsbSwitch );
            Train->mvControlled->OilPumpSwitch( false );
        }
    }
    else {
        // two-state switch, only cares about press events
        if( Command.action == GLFW_PRESS ) {
            // visual feedback
            Train->ggOilPumpButton.UpdateValue( 1.0, Train->dsbSwitch );
            Train->mvControlled->OilPumpSwitch( true );
            Train->mvControlled->OilPumpSwitchOff( false );
        }
    }
}

void TTrain::OnCommand_oilpumpdisable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }

    if( Train->ggOilPumpButton.type() == TGaugeType::push ) {
        // impulse switch
        // currently there's no disable return type switch
        return;
    }
    else {
        // two-state switch, only cares about press events
        if( Command.action == GLFW_PRESS ) {
            // visual feedback
            Train->ggOilPumpButton.UpdateValue( 0.0, Train->dsbSwitch );
            Train->mvControlled->OilPumpSwitch( false );
            Train->mvControlled->OilPumpSwitchOff( true );
        }
    }
}

void TTrain::OnCommand_waterheaterbreakertoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->mvControlled->WaterHeater.breaker ) {
            // turn on
            OnCommand_waterheaterbreakerclose( Train, Command );
        }
        else {
            //turn off
            OnCommand_waterheaterbreakeropen( Train, Command );
        }
    }
}

void TTrain::OnCommand_waterheaterbreakerclose( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // visual feedback
        Train->ggWaterHeaterBreakerButton.UpdateValue( 1.0, Train->dsbSwitch );

        if( true == Train->mvControlled->WaterHeater.breaker ) { return; } // already enabled

        Train->mvControlled->WaterHeaterBreakerSwitch( true );
    }
}

void TTrain::OnCommand_waterheaterbreakeropen( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // visual feedback
        Train->ggWaterHeaterBreakerButton.UpdateValue( 0.0, Train->dsbSwitch );

        if( false == Train->mvControlled->WaterHeater.breaker ) { return; } // already enabled

        Train->mvControlled->WaterHeaterBreakerSwitch( false );
    }
}

void TTrain::OnCommand_waterheatertoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->mvControlled->WaterHeater.is_enabled ) {
            // turn on
            OnCommand_waterheaterenable( Train, Command );
        }
        else {
            //turn off
            OnCommand_waterheaterdisable( Train, Command );
        }
    }
}

void TTrain::OnCommand_waterheaterenable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // visual feedback
        Train->ggWaterHeaterButton.UpdateValue( 1.0, Train->dsbSwitch );

        if( true == Train->mvControlled->WaterHeater.is_enabled ) { return; } // already enabled

        Train->mvControlled->WaterHeaterSwitch( true );
    }
}

void TTrain::OnCommand_waterheaterdisable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // visual feedback
        Train->ggWaterHeaterButton.UpdateValue( 0.0, Train->dsbSwitch );

        if( false == Train->mvControlled->WaterHeater.is_enabled ) { return; } // already disabled

        Train->mvControlled->WaterHeaterSwitch( false );
    }
}

void TTrain::OnCommand_waterpumpbreakertoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->mvControlled->WaterPump.breaker ) {
            // turn on
            OnCommand_waterpumpbreakerclose( Train, Command );
        }
        else {
            //turn off
            OnCommand_waterpumpbreakeropen( Train, Command );
        }
    }
}

void TTrain::OnCommand_waterpumpbreakerclose( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // visual feedback
        Train->ggWaterPumpBreakerButton.UpdateValue( 1.0, Train->dsbSwitch );

        if( true == Train->mvControlled->WaterPump.breaker ) { return; } // already enabled

        Train->mvControlled->WaterPumpBreakerSwitch( true );
    }
}

void TTrain::OnCommand_waterpumpbreakeropen( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // visual feedback
        Train->ggWaterPumpBreakerButton.UpdateValue( 0.0, Train->dsbSwitch );

        if( false == Train->mvControlled->WaterPump.breaker ) { return; } // already enabled

        Train->mvControlled->WaterPumpBreakerSwitch( false );
    }
}

void TTrain::OnCommand_waterpumptoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }

    if( Train->ggWaterPumpButton.type() == TGaugeType::push ) {
        // impulse switch
        // currently there's no off button so we always try to turn it on
        OnCommand_waterpumpenable( Train, Command );
    }
    else {
        // two-state switch
        if( Command.action == GLFW_RELEASE ) { return; }

        if( false == Train->mvControlled->WaterPump.is_enabled ) {
            // turn on
            OnCommand_waterpumpenable( Train, Command );
        }
        else {
            //turn off
            OnCommand_waterpumpdisable( Train, Command );
        }
    }
}

void TTrain::OnCommand_waterpumpenable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }

    if( Train->ggWaterPumpButton.type() == TGaugeType::push ) {
        // impulse switch
        if( Command.action == GLFW_PRESS ) {
            // visual feedback
            Train->ggWaterPumpButton.UpdateValue( 1.0, Train->dsbSwitch );
            Train->mvControlled->WaterPumpSwitch( true );
        }
        else if( Command.action == GLFW_RELEASE ) {
            // visual feedback
            Train->ggWaterPumpButton.UpdateValue( 0.0, Train->dsbSwitch );
            Train->mvControlled->WaterPumpSwitch( false );
        }
    }
    else {
        // two-state switch, only cares about press events
        if( Command.action == GLFW_PRESS ) {
            // visual feedback
            Train->ggWaterPumpButton.UpdateValue( 1.0, Train->dsbSwitch );
            Train->mvControlled->WaterPumpSwitch( true );
            Train->mvControlled->WaterPumpSwitchOff( false );
        }
    }
}

void TTrain::OnCommand_waterpumpdisable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }

    if( Train->ggWaterPumpButton.type() == TGaugeType::push ) {
        // impulse switch
        // currently there's no disable return type switch
        return;
    }
    else {
        // two-state switch, only cares about press events
        if( Command.action == GLFW_PRESS ) {
            // visual feedback
            Train->ggWaterPumpButton.UpdateValue( 0.0, Train->dsbSwitch );
            Train->mvControlled->WaterPumpSwitch( false );
            Train->mvControlled->WaterPumpSwitchOff( true );
        }
    }
}

void TTrain::OnCommand_watercircuitslinktoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->mvControlled->WaterCircuitsLink ) {
            // turn on
            OnCommand_watercircuitslinkenable( Train, Command );
        }
        else {
            //turn off
            OnCommand_watercircuitslinkdisable( Train, Command );
        }
    }
}

void TTrain::OnCommand_watercircuitslinkenable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // visual feedback
        Train->ggWaterCircuitsLinkButton.UpdateValue( 1.0, Train->dsbSwitch );

        if( true == Train->mvControlled->WaterCircuitsLink ) { return; } // already enabled

        Train->mvControlled->WaterCircuitsLinkSwitch( true );
    }
}

void TTrain::OnCommand_watercircuitslinkdisable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // visual feedback
        Train->ggWaterCircuitsLinkButton.UpdateValue( 0.0, Train->dsbSwitch );

        if( false == Train->mvControlled->WaterCircuitsLink ) { return; } // already disabled

        Train->mvControlled->WaterCircuitsLinkSwitch( false );
    }
}

void TTrain::OnCommand_convertertoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        auto const overloadrelayisopen { (
            Train->Dynamic()->Mechanik != nullptr ?
                Train->Dynamic()->Mechanik->IsAnyConverterOverloadRelayOpen :
                Train->mvOccupied->ConvOvldFlag ) };

        if( Train->mvOccupied->ConvSwitchType != "impulse" ?
                Train->ggConverterButton.GetValue() < 0.5 :
                ( ( false == Train->mvOccupied->Power110vIsAvailable )
               && ( false == overloadrelayisopen ) ) ) {
            // turn on
            OnCommand_converterenable( Train, Command );
        }
        else {
            //turn off
            OnCommand_converterdisable( Train, Command );
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

void TTrain::OnCommand_converterenable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // visual feedback
        Train->ggConverterButton.UpdateValue( 1.0, Train->dsbSwitch );

        // impulse type switch has no effect if there's no power
        // NOTE: this is most likely setup wrong, but the whole thing is smoke and mirrors anyway
        if( ( Train->mvOccupied->ConvSwitchType != "impulse" )
         || ( Train->mvControlled->Mains ) ) {
            // won't start if the line breaker button is still held
            Train->mvOccupied->ConverterSwitch( true );
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // potentially reset impulse switch position, using shared code branch
        OnCommand_convertertoggle( Train, Command );
    }
}

void TTrain::OnCommand_converterdisable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // visual feedback
        Train->ggConverterButton.UpdateValue( 0.0, Train->dsbSwitch );
        if( Train->ggConverterOffButton.SubModel != nullptr ) {
            Train->ggConverterOffButton.UpdateValue( 1.0, Train->dsbSwitch );
        }

        Train->mvOccupied->ConverterSwitch( false );
    }
    else if( Command.action == GLFW_RELEASE ) {
        // potentially reset impulse switch position, using shared code branch
        OnCommand_convertertoggle( Train, Command );
    }
}

void TTrain::OnCommand_convertertogglelocal( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->ConverterStart == start_t::automatic ) {
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
        // visual feedback
        Train->ggConverterFuseButton.UpdateValue( 1.0, Train->dsbSwitch );

        Train->mvControlled->RelayReset( relay_t::primaryconverteroverload );
    }
    else if( Command.action == GLFW_RELEASE ) {
        // visual feedback
        Train->ggConverterFuseButton.UpdateValue( 0.0, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_compressortoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        auto const compressorisenabled { (
            Train->Dynamic()->Mechanik ?
                Train->Dynamic()->Mechanik->IsAnyCompressorEnabled :
                Train->mvOccupied->CompressorAllow ) };

        if( false == compressorisenabled ) {
            // turn on
            OnCommand_compressorenable( Train, Command );
        }
        else {
            //turn off
            OnCommand_compressordisable( Train, Command );
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

void TTrain::OnCommand_compressorenable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // visual feedback
        Train->ggCompressorButton.UpdateValue( 1.0, Train->dsbSwitch );

        // impulse type switch has no effect if there's no power
        // NOTE: this is most likely setup wrong, but the whole thing is smoke and mirrors anyway
//        if( ( Train->mvOccupied->CompSwitchType != "impulse" )
//         || ( Train->mvControlled->Mains ) ) {

            Train->mvOccupied->CompressorSwitch( true );
//        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // potentially reset impulse switch position, using shared code branch
        OnCommand_compressortoggle( Train, Command );
    }

}

void TTrain::OnCommand_compressordisable( TTrain *Train, command_data const &Command ) {

    if( Train->mvControlled->CompressorPower >= 2 ) { return; }

    if( Command.action == GLFW_PRESS ) {
        // visual feedback
        Train->ggCompressorButton.UpdateValue( 0.0, Train->dsbSwitch );
/*
        if( Train->ggCompressorOffButton.SubModel != nullptr ) {
            Train->ggCompressorOffButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
*/
        Train->mvOccupied->CompressorSwitch( false );
    }
    else if( Command.action == GLFW_RELEASE ) {
        // potentially reset impulse switch position, using shared code branch
        OnCommand_compressortoggle( Train, Command );
    }
}

void TTrain::OnCommand_compressortogglelocal( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->CompressorPower >= 2 ) { return; }
    if( Train->ggCompressorLocalButton.SubModel == nullptr ) { return; }

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

void TTrain::OnCommand_compressorpresetactivatenext(TTrain *Train, command_data const &Command) {

	if (Train->mvOccupied->CompressorListPosNo == 0) { return; }
    if( Command.action == GLFW_REPEAT ) { return; }

    if( Train->ggCompressorListButton.type() == TGaugeType::push ) {
        // impulse switch
        if( Train->mvOccupied->CompressorListPosNo < Train->mvOccupied->CompressorListDefPos + 1 ) { return; }

        Train->mvOccupied->ChangeCompressorPreset( (
            Command.action == GLFW_PRESS ?
                Train->mvOccupied->CompressorListDefPos + 1 :
                Train->mvOccupied->CompressorListDefPos ) );
        // visual feedback
        Train->ggCompressorListButton.UpdateValue( Train->mvOccupied->CompressorListPos - 1, Train->dsbSwitch );
    }
    else {
        // multi-state switch
        if( Command.action == GLFW_RELEASE ) { return; }

        if( ( Train->mvOccupied->CompressorListPos < Train->mvOccupied->CompressorListPosNo )
         || ( true == Train->mvOccupied->CompressorListWrap ) ) {
            // active light preset is stored as value in range 1-LigthPosNo
            Train->mvOccupied->ChangeCompressorPreset( (
                Train->mvOccupied->CompressorListPos < Train->mvOccupied->CompressorListPosNo ?
                    Train->mvOccupied->CompressorListPos + 1 :
                    1 ) ); // wrap mode
            // visual feedback
            Train->ggCompressorListButton.UpdateValue( Train->mvOccupied->CompressorListPos - 1, Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_compressorpresetactivateprevious(TTrain *Train, command_data const &Command) {

	if (Train->mvOccupied->CompressorListPosNo == 0) { return; }
	if (Command.action != GLFW_PRESS) { return; } // one change per key press

    if( Train->ggCompressorListButton.type() == TGaugeType::push ) {
        // impulse switch toggles only between positions 'default' and 'default+1'
        return;
    }

	if ((Train->mvOccupied->CompressorListPos > 1)
		|| (true == Train->mvOccupied->CompressorListWrap)) {
		// active light preset is stored as value in range 1-LigthPosNo
		Train->mvOccupied->ChangeCompressorPreset( (
			Train->mvOccupied->CompressorListPos > 1 ?
			Train->mvOccupied->CompressorListPos - 1 :
			Train->mvOccupied->CompressorListPosNo) ); // wrap mode

		// visual feedback
		if (Train->ggCompressorListButton.SubModel != nullptr) {
			Train->ggCompressorListButton.UpdateValue(Train->mvOccupied->CompressorListPos - 1, Train->dsbSwitch);
		}
	}
}

void TTrain::OnCommand_compressorpresetactivatedefault(TTrain *Train, command_data const &Command) {

	if (Train->mvOccupied->CompressorListPosNo == 0) { return; }
	if (Command.action != GLFW_PRESS) { return; } // one change per key press

	Train->mvOccupied->ChangeCompressorPreset( Train->mvOccupied->CompressorListDefPos );

    // visual feedback
	if (Train->ggCompressorListButton.SubModel != nullptr) {
			Train->ggCompressorListButton.UpdateValue(Train->mvOccupied->CompressorListPos - 1, Train->dsbSwitch);
	}
}

void TTrain::OnCommand_motorblowerstogglefront( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }

    if( Train->ggMotorBlowersFrontButton.type() == TGaugeType::push ) {
        // impulse switch
        // currently there's no off button so we always try to turn it on
        OnCommand_motorblowersenablefront( Train, Command );
    }
    else {
        // two-state switch
        if( Command.action == GLFW_RELEASE ) { return; }

        if( false == Train->mvControlled->MotorBlowers[end::front].is_enabled ) {
            // turn on
            OnCommand_motorblowersenablefront( Train, Command );
        }
        else {
            //turn off
            OnCommand_motorblowersdisablefront( Train, Command );
        }
    }
}

void TTrain::OnCommand_motorblowersenablefront( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }

    if( Train->ggMotorBlowersFrontButton.type() == TGaugeType::push ) {
        // impulse switch
        if( Command.action == GLFW_PRESS ) {
            // visual feedback
            Train->ggMotorBlowersFrontButton.UpdateValue( 1.f, Train->dsbSwitch );
            Train->mvControlled->MotorBlowersSwitch( true, end::front );
        }
        else if( Command.action == GLFW_RELEASE ) {
            // visual feedback
            Train->ggMotorBlowersFrontButton.UpdateValue( 0.f, Train->dsbSwitch );
            Train->mvControlled->MotorBlowersSwitch( false, end::front );
        }
    }
    else {
        // two-state switch, only cares about press events
        if( Command.action == GLFW_PRESS ) {
            // visual feedback
            Train->ggMotorBlowersFrontButton.UpdateValue( 1.f, Train->dsbSwitch );
            Train->mvControlled->MotorBlowersSwitch( true, end::front );
            Train->mvControlled->MotorBlowersSwitchOff( false, end::front );
        }
    }
}

void TTrain::OnCommand_motorblowersdisablefront( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }

    if( Train->ggMotorBlowersFrontButton.type() == TGaugeType::push ) {
        // impulse switch
        // currently there's no disable return type switch
        return;
    }
    else {
        // two-state switch, only cares about press events
        if( Command.action == GLFW_PRESS ) {
            // visual feedback
            Train->ggMotorBlowersFrontButton.UpdateValue( 0.f, Train->dsbSwitch );
            Train->mvControlled->MotorBlowersSwitch( false, end::front );
            Train->mvControlled->MotorBlowersSwitchOff( true, end::front );
        }
    }
}

void TTrain::OnCommand_motorblowerstogglerear( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }

    if( Train->ggMotorBlowersRearButton.type() == TGaugeType::push ) {
        // impulse switch
        // currently there's no off button so we always try to turn it on
        OnCommand_motorblowersenablerear( Train, Command );
    }
    else {
        // two-state switch
        if( Command.action == GLFW_RELEASE ) { return; }

        if( false == Train->mvControlled->MotorBlowers[ end::rear ].is_enabled ) {
            // turn on
            OnCommand_motorblowersenablerear( Train, Command );
        }
        else {
            //turn off
            OnCommand_motorblowersdisablerear( Train, Command );
        }
    }
}

void TTrain::OnCommand_motorblowersenablerear( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }

    if( Train->ggMotorBlowersRearButton.type() == TGaugeType::push ) {
        // impulse switch
        if( Command.action == GLFW_PRESS ) {
            // visual feedback
            Train->ggMotorBlowersRearButton.UpdateValue( 1.f, Train->dsbSwitch );
            Train->mvControlled->MotorBlowersSwitch( true, end::rear );
        }
        else if( Command.action == GLFW_RELEASE ) {
            // visual feedback
            Train->ggMotorBlowersRearButton.UpdateValue( 0.f, Train->dsbSwitch );
            Train->mvControlled->MotorBlowersSwitch( false, end::rear );
        }
    }
    else {
        // two-state switch, only cares about press events
        if( Command.action == GLFW_PRESS ) {
            // visual feedback
            Train->ggMotorBlowersRearButton.UpdateValue( 1.f, Train->dsbSwitch );
            Train->mvControlled->MotorBlowersSwitch( true, end::rear );
            Train->mvControlled->MotorBlowersSwitchOff( false, end::rear );
        }
    }
}

void TTrain::OnCommand_motorblowersdisablerear( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }

    if( Train->ggMotorBlowersRearButton.type() == TGaugeType::push ) {
        // impulse switch
        // currently there's no disable return type switch
        return;
    }
    else {
        // two-state switch, only cares about press events
        if( Command.action == GLFW_PRESS ) {
            // visual feedback
            Train->ggMotorBlowersRearButton.UpdateValue( 0.f, Train->dsbSwitch );
            Train->mvControlled->MotorBlowersSwitch( false, end::rear );
            Train->mvControlled->MotorBlowersSwitchOff( true, end::rear );
        }
    }
}

void TTrain::OnCommand_motorblowersdisableall( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }

    if( Train->ggMotorBlowersAllOffButton.type() == TGaugeType::push ) {
        // impulse switch
        if( Command.action == GLFW_PRESS ) {
            // visual feedback
            Train->ggMotorBlowersAllOffButton.UpdateValue( 1.f, Train->dsbSwitch );
            Train->mvControlled->MotorBlowersSwitchOff( true, end::front );
            Train->mvControlled->MotorBlowersSwitchOff( true, end::rear );
        }
        else if( Command.action == GLFW_RELEASE ) {
            // visual feedback
            Train->ggMotorBlowersAllOffButton.UpdateValue( 0.f, Train->dsbSwitch );
            Train->mvControlled->MotorBlowersSwitchOff( false, end::front );
            Train->mvControlled->MotorBlowersSwitchOff( false, end::rear );
        }
    }
    else {
        // two-state switch, only cares about press events
        // NOTE: generally this switch doesn't come in two-state form
        if( Command.action == GLFW_PRESS ) {
            if( Train->ggMotorBlowersAllOffButton.GetDesiredValue() < 0.5f ) {
                // switch is off, activate
                Train->mvControlled->MotorBlowersSwitchOff( true, end::front );
                Train->mvControlled->MotorBlowersSwitchOff( true, end::rear );
                // visual feedback
                Train->ggMotorBlowersRearButton.UpdateValue( 1.f, Train->dsbSwitch );
            }
            else {
                // deactivate
                Train->mvControlled->MotorBlowersSwitchOff( false, end::front );
                Train->mvControlled->MotorBlowersSwitchOff( false, end::rear );
                // visual feedback
                Train->ggMotorBlowersRearButton.UpdateValue( 0.f, Train->dsbSwitch );
            }
        }
    }
}

void TTrain::OnCommand_coolingfanstoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action != GLFW_PRESS ) { return; }

    Train->mvControlled->RVentForceOn = ( !Train->mvControlled->RVentForceOn );
}

void TTrain::OnCommand_motorconnectorsopen( TTrain *Train, command_data const &Command ) {

    // TODO: don't rely on presense of 3d model to determine presence of the switch
    if( Train->ggStLinOffButton.SubModel == nullptr ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Open Motor Power Connectors button is missing, or wasn't defined" );
        }
        return;
    }
    // HACK: because we don't have modeled actual circuits this is a simplification of the real mechanics
    // namely, pressing the button will flip it in the entire unit, which isn't exactly physically possible
    if( Command.action == GLFW_PRESS ) {
        // button works while it's held down but we can ignore repeats
        if( false == Train->mvControlled->StLinSwitchOff ) {
            // open the connectors
            // visual feedback
            Train->ggStLinOffButton.UpdateValue( 1.0, Train->dsbSwitch );

            Train->mvControlled->StLinSwitchOff = true;
            Train->set_paired_open_motor_connectors_button( true );
        }
        else {
            // potentially close the connectors
            OnCommand_motorconnectorsclose( Train, Command );
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // button released
        if( Train->mvControlled->StLinSwitchType != "toggle" ) {
            // default button type (impulse) ceases its work on button release
            // visual feedback
            Train->ggStLinOffButton.UpdateValue( 0.0, Train->dsbSwitch );

            Train->mvControlled->StLinSwitchOff = false;
            Train->set_paired_open_motor_connectors_button( false );
        }
    }
}

void TTrain::OnCommand_motorconnectorsclose( TTrain *Train, command_data const &Command ) {

    // TODO: don't rely on presense of 3d model to determine presence of the switch
    if( Train->ggStLinOffButton.SubModel == nullptr ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Open Motor Power Connectors button is missing, or wasn't defined" );
        }
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        if( Train->mvControlled->StLinSwitchType == "toggle" ) {
            // default type of button (impulse) has only one effect on press, but the toggle type can toggle the state
            // visual feedback
            Train->ggStLinOffButton.UpdateValue( 0.0, Train->dsbSwitch );
        }

        if( false == Train->mvControlled->StLinSwitchOff ) { return; } // already closed

        Train->mvControlled->StLinSwitchOff = false;
        Train->set_paired_open_motor_connectors_button( false );
    }
}

void TTrain::OnCommand_motordisconnect( TTrain *Train, command_data const &Command ) {

    if( ( Train->mvControlled->TrainType == dt_EZT ?
            ( Train->mvControlled != Train->mvOccupied ) :
            ( Train->iCabn != 0 ) ) ) {
        // tylko w maszynowym
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        Train->mvControlled->CutOffEngine();
    }
}

void TTrain::OnCommand_motoroverloadrelaythresholdtoggle( TTrain *Train, command_data const &Command )
{
    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( true == Train->mvControlled->ShuntModeAllow ?
                ( false == Train->mvControlled->ShuntMode ) :
                ( false == Train->mvControlled->MotorOverloadRelayHighThreshold ) ) ) {
            // turn on
            OnCommand_motoroverloadrelaythresholdsethigh( Train, Command );
        }
        else {
            //turn off
            OnCommand_motoroverloadrelaythresholdsetlow( Train, Command );
        }
    }
}

void TTrain::OnCommand_motoroverloadrelaythresholdsetlow( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        Train->mvControlled->CurrentSwitch( false );
        // visual feedback
        Train->ggMaxCurrentCtrl.UpdateValue( Train->mvControlled->MotorOverloadRelayHighThreshold ? 1 : 0, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_motoroverloadrelaythresholdsethigh( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        Train->mvControlled->CurrentSwitch( true );
        // visual feedback
        Train->ggMaxCurrentCtrl.UpdateValue( Train->mvControlled->MotorOverloadRelayHighThreshold ? 1 : 0, Train->dsbSwitch );
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
        // visual feedback
        Train->ggFuseButton.UpdateValue( 1.0, Train->dsbSwitch );

        Train->mvControlled->FuseOn();
    }
    else if( Command.action == GLFW_RELEASE ) {
        // visual feedback
        Train->ggFuseButton.UpdateValue( 0.0, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_universalrelayreset( TTrain *Train, command_data const &Command ) {

    auto const itemindex = static_cast<int>( Command.command ) - static_cast<int>( user_command::universalrelayreset1 );
    auto &item = Train->ggRelayResetButtons[ itemindex ];

    // NOTE: relay reset switches are impulse-only
    if( Command.action == GLFW_PRESS ) {
        Train->mvOccupied->UniversalResetButton( itemindex );
        // visual feedback
        item.UpdateValue( 1.0 );
    }
    else if( Command.action == GLFW_RELEASE ) {
        // visual feedback
        item.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_lightspresetactivatenext( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo == 0 ) {
        // no preset selector
        return;
    }
    if( Command.action != GLFW_PRESS ) {
        // one change per key press
        return;
    }

    if( ( Train->mvOccupied->LightsPos < Train->mvOccupied->LightsPosNo )
     || ( true == Train->mvOccupied->LightsWrap ) ) {
        // active light preset is stored as value in range 1-LigthPosNo
        auto const restartcycle { Train->mvOccupied->LightsPos == Train->mvOccupied->LightsPosNo };
        Train->mvOccupied->LightsPos = (
            false == restartcycle ?
                Train->mvOccupied->LightsPos + 1 :
                1 ); // wrap mode

        Train->Dynamic()->SetLights();
        // visual feedback
        if( Train->ggLightsButton.SubModel != nullptr ) {
            // HACK: skip submodel animation when restarting cycle, since it plays in the 'wrong' direction
            if( false == restartcycle ) {
                Train->ggLightsButton.UpdateValue( Train->mvOccupied->LightsPos - 1, Train->dsbSwitch );
            }
            else {
                Train->ggLightsButton.PutValue( Train->mvOccupied->LightsPos - 1 );
            }
        }
    }
}

void TTrain::OnCommand_lightspresetactivateprevious( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo == 0 ) {
        // no preset selector
        return;
    }
    if( Command.action != GLFW_PRESS ) {
        // one change per key press
        return;
    }

    if( ( Train->mvOccupied->LightsPos > 1 )
     || ( true == Train->mvOccupied->LightsWrap ) ) {
        // active light preset is stored as value in range 1-LigthPosNo
        auto const restartcycle { Train->mvOccupied->LightsPos == 1 };
        Train->mvOccupied->LightsPos = (
            false == restartcycle ?
                Train->mvOccupied->LightsPos - 1 :
                Train->mvOccupied->LightsPosNo ); // wrap mode

        Train->Dynamic()->SetLights();
        // visual feedback
        if( Train->ggLightsButton.SubModel != nullptr ) {
            // HACK: skip submodel animation when restarting cycle, since it plays in the 'wrong' direction
            if( false == restartcycle ) {
                Train->ggLightsButton.UpdateValue( Train->mvOccupied->LightsPos - 1, Train->dsbSwitch );
            }
            else {
                Train->ggLightsButton.PutValue( Train->mvOccupied->LightsPos - 1 );
            }
        }
    }
}

void TTrain::OnCommand_headlighttoggleleft( TTrain *Train, command_data const &Command ) {

    auto const vehicleend { Train->cab_to_end() };

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->mvOccupied->iLights[ vehicleend ] & light::headlight_left ) == 0 ) {
            // turn on
            OnCommand_headlightenableleft( Train, Command );
        }
        else {
            //turn off
            OnCommand_headlightdisableleft( Train, Command );
        }
    }
}

void TTrain::OnCommand_headlightenableleft( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // visual feedback
        Train->ggLeftLightButton.UpdateValue( 1.0, Train->dsbSwitch );
        // implementation
        auto const vehicleend { Train->cab_to_end() };

        if( ( Train->mvOccupied->iLights[ vehicleend ] & light::headlight_left ) == 0 ) {
            Train->mvOccupied->iLights[ vehicleend ] ^= light::headlight_left;
        }
        // if the light is controlled by 3-way switch, disable marker light
        if( Train->ggLeftEndLightButton.SubModel == nullptr ) {
            Train->mvOccupied->iLights[ vehicleend ] &= ~light::redmarker_left;
        }
    }
}

void TTrain::OnCommand_headlightdisableleft( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        int const vehicleend { Train->cab_to_end() };

        if( ( Train->mvOccupied->iLights[ vehicleend ] & light::headlight_left ) == 0 ) { return; } // already disabled

        Train->mvOccupied->iLights[ vehicleend ] ^= light::headlight_left;
        // visual feedback
        Train->ggLeftLightButton.UpdateValue( 0.0, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_headlighttoggleright( TTrain *Train, command_data const &Command ) {

    auto const vehicleend { Train->cab_to_end() };

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->mvOccupied->iLights[ vehicleend ] & light::headlight_right ) == 0 ) {
            // turn on
            OnCommand_headlightenableright( Train, Command );
        }
        else {
            //turn off
            OnCommand_headlightdisableright( Train, Command );
        }
    }
}

void TTrain::OnCommand_headlightenableright( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // visual feedback
        Train->ggRightLightButton.UpdateValue( 1.0, Train->dsbSwitch );
        // implementation
        auto const vehicleend { Train->cab_to_end() };

        if( ( Train->mvOccupied->iLights[ vehicleend ] & light::headlight_right ) == 0 ) {
            Train->mvOccupied->iLights[ vehicleend ] ^= light::headlight_right;
        }
        // if the light is controlled by 3-way switch, disable marker light
        if( Train->ggRightEndLightButton.SubModel == nullptr ) {
            Train->mvOccupied->iLights[ vehicleend ] &= ~light::redmarker_right;
        }
    }
}

void TTrain::OnCommand_headlightdisableright( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        auto const vehicleend { Train->cab_to_end() };

        if( ( Train->mvOccupied->iLights[ vehicleend ] & light::headlight_right ) == 0 ) { return; } // already disabled

        Train->mvOccupied->iLights[ vehicleend ] ^= light::headlight_right;
        // visual feedback
        Train->ggRightLightButton.UpdateValue( 0.0, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_headlighttoggleupper( TTrain *Train, command_data const &Command ) {

    auto const vehicleend { Train->cab_to_end() };

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->mvOccupied->iLights[ vehicleend ] & light::headlight_upper ) == 0 ) {
            // turn on
            OnCommand_headlightenableupper( Train, Command );
        }
        else {
            //turn off
            OnCommand_headlightdisableupper( Train, Command );
        }
    }
}

void TTrain::OnCommand_headlightenableupper( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        auto const vehicleend { Train->cab_to_end() };

        if( ( Train->mvOccupied->iLights[ vehicleend ] & light::headlight_upper ) != 0 ) { return; } // already enabled

        Train->mvOccupied->iLights[ vehicleend ] ^= light::headlight_upper;
        // visual feedback
        Train->ggUpperLightButton.UpdateValue( 1.0, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_headlightdisableupper( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        auto const vehicleend { Train->cab_to_end() };

        if( ( Train->mvOccupied->iLights[ vehicleend ] & light::headlight_upper ) == 0 ) { return; } // already disabled

        Train->mvOccupied->iLights[ vehicleend ] ^= light::headlight_upper;
        // visual feedback
        Train->ggUpperLightButton.UpdateValue( 0.0, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_redmarkertoggleleft( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        auto const vehicleend { Train->cab_to_end() };

        if( ( Train->mvOccupied->iLights[ vehicleend ] & light::redmarker_left ) == 0 ) {
            // turn on
            OnCommand_redmarkerenableleft( Train, Command );
        }
        else {
            //turn off
            OnCommand_redmarkerdisableleft( Train, Command );
        }
    }
}

void TTrain::OnCommand_redmarkerenableleft( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        auto const vehicleend { Train->cab_to_end() };

        if( ( Train->mvOccupied->iLights[ vehicleend ] & light::redmarker_left ) != 0 ) { return; } // already enabled

        Train->mvOccupied->iLights[ vehicleend ] ^= light::redmarker_left;
        // visual feedback
        if( Train->ggLeftEndLightButton.SubModel != nullptr ) {
            Train->ggLeftEndLightButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            // we interpret lack of dedicated switch as a sign the light is controlled with 3-way switch
            // this is crude, but for now will do
            Train->ggLeftLightButton.UpdateValue( -1.0, Train->dsbSwitch );
            // if the light is controlled by 3-way switch, disable the headlight
            Train->mvOccupied->iLights[ vehicleend ] &= ~light::headlight_left;
        }
    }
}

void TTrain::OnCommand_redmarkerdisableleft( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        auto const vehicleend { Train->cab_to_end() };

        if( ( Train->mvOccupied->iLights[ vehicleend ] & light::redmarker_left ) == 0 ) { return; } // already disabled

        Train->mvOccupied->iLights[ vehicleend ] ^= light::redmarker_left;
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

void TTrain::OnCommand_redmarkertoggleright( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        auto const vehicleend { Train->cab_to_end() };

        if( ( Train->mvOccupied->iLights[ vehicleend ] & light::redmarker_right ) == 0 ) {
            // turn on
            OnCommand_redmarkerenableright( Train, Command );
        }
        else {
            //turn off
            OnCommand_redmarkerdisableright( Train, Command );
        }
    }
}

void TTrain::OnCommand_redmarkerenableright( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        auto const vehicleend { Train->cab_to_end() };

        if( ( Train->mvOccupied->iLights[ vehicleend ] & light::redmarker_right ) != 0 ) { return; } // already enabled

        Train->mvOccupied->iLights[ vehicleend ] ^= light::redmarker_right;
        // visual feedback
        if( Train->ggRightEndLightButton.SubModel != nullptr ) {
            Train->ggRightEndLightButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            // we interpret lack of dedicated switch as a sign the light is controlled with 3-way switch
            // this is crude, but for now will do
            Train->ggRightLightButton.UpdateValue( -1.0, Train->dsbSwitch );
            // if the light is controlled by 3-way switch, disable the headlight
            Train->mvOccupied->iLights[ vehicleend ] &= ~light::headlight_right;
        }
    }
}

void TTrain::OnCommand_redmarkerdisableright( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        auto const vehicleend { Train->cab_to_end() };

        if( ( Train->mvOccupied->iLights[ vehicleend ] & light::redmarker_right ) == 0 ) { return; } // already disabled

        Train->mvOccupied->iLights[ vehicleend ] ^= light::redmarker_right;
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

void TTrain::OnCommand_headlighttogglerearleft( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // NOTE: we toggle the light on opposite side, as 'rear right' is 'front left' on the rear end etc
        auto const vehicleotherend { (
            Train->cab_to_end() == end::front ?
                end::rear :
                end::front ) };
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->mvOccupied->iLights[ vehicleotherend ] & light::headlight_right ) == 0 ) {
            OnCommand_headlightenablerearleft(Train, Command);
        }
        else {
            OnCommand_headlightdisablerearleft(Train, Command);
        }
    }
}

void TTrain::OnCommand_headlightenablerearleft( TTrain *Train, command_data const &Command ) {
    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        auto const vehicleotherend { (
            Train->cab_to_end() == end::front ?
                end::rear :
                end::front ) };
        // already enabled
        if( ( Train->mvOccupied->iLights[ vehicleotherend ] & light::headlight_right ) == 0 ) {
            // turn on
            Train->mvOccupied->iLights[ vehicleotherend ] ^= light::headlight_right;
            // visual feedback
            Train->ggRearLeftLightButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_headlightdisablerearleft( TTrain *Train, command_data const &Command ) {
    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }
    if( Command.action == GLFW_PRESS ) {
        auto const vehicleotherend { (
            Train->cab_to_end() == end::front ?
                end::rear :
                end::front ) };
        // already disabled
        if( ( Train->mvOccupied->iLights[ vehicleotherend ] & light::headlight_right ) == 0 ) {return;}

        //turn off
        Train->mvOccupied->iLights[ vehicleotherend ] ^= light::headlight_right;
        // visual feedback
        Train->ggRearLeftLightButton.UpdateValue( 0.0, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_headlighttogglerearright( TTrain *Train, command_data const &Command ) {
    if( Command.action == GLFW_PRESS ) {
        // NOTE: we toggle the light on opposite side, as 'rear right' is 'front left' on the rear end etc
        auto const vehicleotherend { (
            Train->cab_to_end() == end::front ?
                end::rear :
                end::front ) };
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->mvOccupied->iLights[ vehicleotherend ] & light::headlight_left ) == 0 ) {
            OnCommand_headlightenablerearright(Train, Command);
        }
        else {
            OnCommand_headlightdisablerearright(Train, Command);
        }
    }
}

void TTrain::OnCommand_headlightenablerearright( TTrain *Train, command_data const &Command ) {
    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // NOTE: we toggle the light on opposite side, as 'rear right' is 'front left' on the rear end etc
        auto const vehicleotherend { (
            Train->cab_to_end() == end::front ?
                end::rear :
                end::front ) };

        if( ( Train->mvOccupied->iLights[ vehicleotherend ] & light::headlight_left ) == 0 ) {
            // turn on
            Train->mvOccupied->iLights[ vehicleotherend ] ^= light::headlight_left;
            // visual feedback
            Train->ggRearRightLightButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_headlightdisablerearright( TTrain *Train, command_data const &Command ) {
    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // NOTE: we toggle the light on opposite side, as 'rear right' is 'front left' on the rear end etc
        auto const vehicleotherend { (
            Train->cab_to_end() == end::front ?
                end::rear :
                end::front ) };
        // already disabled
        if( ( Train->mvOccupied->iLights[ vehicleotherend ] & light::headlight_left ) == 0 ) { return; }

        //turn off
        Train->mvOccupied->iLights[ vehicleotherend ] ^= light::headlight_left;
        // visual feedback
        Train->ggRearRightLightButton.UpdateValue( 0.0, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_headlighttogglerearupper( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        auto const vehicleotherend { (
            Train->cab_to_end() == end::front ?
                end::rear :
                end::front ) };
        if( ( Train->mvOccupied->iLights[ vehicleotherend ] & light::headlight_upper ) == 0 ) {
            OnCommand_headlightenablerearupper(Train, Command);
        }
        else {
            OnCommand_headlightdisablerearupper(Train, Command);
        }
    }
}

void TTrain::OnCommand_headlightenablerearupper( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        auto const vehicleotherend { (
            Train->cab_to_end() == end::front ?
                end::rear :
                end::front ) };
        if( ( Train->mvOccupied->iLights[ vehicleotherend ] & light::headlight_upper ) == 0 ) {
            // turn on
            Train->mvOccupied->iLights[ vehicleotherend ] ^= light::headlight_upper;
            // visual feedback
            Train->ggRearUpperLightButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_headlightdisablerearupper( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        auto const vehicleotherend { (
            Train->cab_to_end() == end::front ?
                end::rear :
                end::front ) };
        // already disabled?
        if( ( Train->mvOccupied->iLights[ vehicleotherend ] & light::headlight_upper ) == 0 ) { return ; }

        //turn off
        Train->mvOccupied->iLights[ vehicleotherend ] ^= light::headlight_upper;
        // visual feedback
        Train->ggRearUpperLightButton.UpdateValue( 0.0, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_redmarkertogglerearleft( TTrain *Train, command_data const &Command ) {
    if( Command.action == GLFW_PRESS ) {
        // NOTE: we toggle the light on opposite side, as 'rear right' is 'front left' on the rear end etc
        auto const vehicleotherend { (
            Train->cab_to_end() == end::front ?
                end::rear :
                end::front ) };
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->mvOccupied->iLights[ vehicleotherend ] & light::redmarker_right ) == 0 ) {
            OnCommand_redmarkerenablerearleft(Train, Command);
        }
        else {
            OnCommand_redmarkerdisablerearleft(Train, Command);
        }
    }
}

void TTrain::OnCommand_redmarkerenablerearleft( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // NOTE: we toggle the light on opposite side, as 'rear right' is 'front left' on the rear end etc
        auto const vehicleotherend { (
            Train->cab_to_end() == end::front ?
                end::rear :
                end::front ) };
        if( ( Train->mvOccupied->iLights[ vehicleotherend ] & light::redmarker_right ) == 0 ) {
            // turn on
            Train->mvOccupied->iLights[ vehicleotherend ] ^= light::redmarker_right;
            // visual feedback
            Train->ggRearLeftEndLightButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_redmarkerdisablerearleft( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // NOTE: we toggle the light on opposite side, as 'rear right' is 'front left' on the rear end etc
        auto const vehicleotherend { (
            Train->cab_to_end() == end::front ?
                end::rear :
                end::front ) };
        if( ( Train->mvOccupied->iLights[ vehicleotherend ] & light::redmarker_right ) == 0 ) { return; }
        //turn off
        Train->mvOccupied->iLights[ vehicleotherend ] ^= light::redmarker_right;
        // visual feedback
        Train->ggRearLeftEndLightButton.UpdateValue( 0.0, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_redmarkertogglerearright( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // NOTE: we toggle the light on opposite side, as 'rear right' is 'front left' on the rear end etc
        auto const vehicleotherend { (
            Train->cab_to_end() == end::front ?
                end::rear :
                end::front ) };
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( ( Train->mvOccupied->iLights[ vehicleotherend ] & light::redmarker_left ) == 0 ) {
            OnCommand_redmarkerenablerearright(Train, Command);
        }
        else {
            OnCommand_redmarkerdisablerearright(Train, Command);
        }
    }
}

void TTrain::OnCommand_redmarkerenablerearright( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // NOTE: we toggle the light on opposite side, as 'rear right' is 'front left' on the rear end etc
        auto const vehicleotherend { (
            Train->cab_to_end() == end::front ?
                end::rear :
                end::front ) };
        if( ( Train->mvOccupied->iLights[ vehicleotherend ] & light::redmarker_left ) == 0 ) {
            // turn on
            Train->mvOccupied->iLights[ vehicleotherend ] ^= light::redmarker_left;
            // visual feedback
            Train->ggRearRightEndLightButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_redmarkerdisablerearright( TTrain *Train, command_data const &Command ) {

    if( Train->mvOccupied->LightsPosNo > 0 ) {
        // lights are controlled by preset selector
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // NOTE: we toggle the light on opposite side, as 'rear right' is 'front left' on the rear end etc
        auto const vehicleotherend { (
            Train->cab_to_end() == end::front ?
                end::rear :
                end::front ) };
        if( ( Train->mvOccupied->iLights[ vehicleotherend ] & light::redmarker_left ) == 0 ) { return; }
        //turn off
        Train->mvOccupied->iLights[ vehicleotherend ] ^= light::redmarker_left;
        // visual feedback
        Train->ggRearRightEndLightButton.UpdateValue( 0.0, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_redmarkerstoggle( TTrain *Train, command_data const &Command ) {

    if( ( true == Command.freefly )
     && ( Command.action == GLFW_PRESS ) ) {

        auto *vehicle { std::get<TDynamicObject *>( simulation::Region->find_vehicle( Command.location, 10, false, true ) ) };

        if( vehicle == nullptr ) { return; }

        int const CouplNr {
            clamp(
                vehicle->DirectionGet()
                * ( Math3D::LengthSquared3( vehicle->HeadPosition() - Command.location ) > Math3D::LengthSquared3( vehicle->RearPosition() - Command.location ) ?
                     1 :
                    -1 ),
                0, 1 ) }; // z [-1,1] zrobić [0,1]

        auto const lightset { light::redmarker_left | light::redmarker_right };

        vehicle->MoverParameters->iLights[ CouplNr ] = (
            false == TestFlag( vehicle->MoverParameters->iLights[ CouplNr ], lightset ) ?
                vehicle->MoverParameters->iLights[ CouplNr ] |= lightset : // turn signals on
                vehicle->MoverParameters->iLights[ CouplNr ] ^= lightset ); // turn signals off
    }
}

void TTrain::OnCommand_endsignalstoggle( TTrain *Train, command_data const &Command ) {

    if( ( true == Command.freefly )
     && ( Command.action == GLFW_PRESS ) ) {

        auto *vehicle { std::get<TDynamicObject *>( simulation::Region->find_vehicle( Command.location, 10, false, true ) ) };

        if( vehicle == nullptr ) { return; }

        int const CouplNr {
            clamp(
                vehicle->DirectionGet()
                * ( Math3D::LengthSquared3( vehicle->HeadPosition() - Command.location ) > Math3D::LengthSquared3( vehicle->RearPosition() - Command.location ) ?
                     1 :
                    -1 ),
                0, 1 ) }; // z [-1,1] zrobić [0,1]

        auto const lightset { light::rearendsignals };

        vehicle->MoverParameters->iLights[ CouplNr ] = (
            false == TestFlag( vehicle->MoverParameters->iLights[ CouplNr ], lightset ) ?
                vehicle->MoverParameters->iLights[ CouplNr ] |= lightset : // turn signals on
                vehicle->MoverParameters->iLights[ CouplNr ] ^= lightset ); // turn signals off
    }
}

void TTrain::OnCommand_headlightsdimtoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->DynamicObject->DimHeadlights ) {
            // turn on
            OnCommand_headlightsdimenable( Train, Command );
        }
        else {
            //turn off
            OnCommand_headlightsdimdisable( Train, Command );
        }
    }
}

void TTrain::OnCommand_headlightsdimenable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( Train->ggDimHeadlightsButton.SubModel == nullptr ) {
            // TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
            WriteLog( "Dim Headlights switch is missing, or wasn't defined" );
            return;
        }
        // visual feedback
        Train->ggDimHeadlightsButton.UpdateValue( 1.0, Train->dsbSwitch );

        if( true == Train->DynamicObject->DimHeadlights ) { return; } // already enabled

        Train->DynamicObject->DimHeadlights = true;
    }
}

void TTrain::OnCommand_headlightsdimdisable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( Train->ggDimHeadlightsButton.SubModel == nullptr ) {
            // TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
            WriteLog( "Dim Headlights switch is missing, or wasn't defined" );
            return;
        }
        // visual feedback
        Train->ggDimHeadlightsButton.UpdateValue( 0.0, Train->dsbSwitch );

        if( false == Train->DynamicObject->DimHeadlights ) { return; } // already enabled

        Train->DynamicObject->DimHeadlights = false;
    }
}

void TTrain::OnCommand_interiorlighttoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->Cabine[Train->iCabn].bLight ) {
            // turn on
            OnCommand_interiorlightenable( Train, Command );
        }
        else {
            //turn off
            OnCommand_interiorlightdisable( Train, Command );
        }
    }
}

void TTrain::OnCommand_interiorlightenable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->m_controlmapper.contains( "cablight_sw:" ) ) {
            // TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
            WriteLog( "Interior Light switch is missing, or wasn't defined" );
            return;
        }
        // store lighting switch states
        if( false == Train->DynamicObject->JointCabs ) {
            // vehicles with separate cabs get separate lighting switch states
            Train->Cabine[ Train->iCabn ].bLight = true;
        }
        else {
            // joint virtual cabs share lighting switch states
            for( auto &cab : Train->Cabine ) {
                cab.bLight = true;
            }
        }
    }
}

void TTrain::OnCommand_interiorlightdisable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->m_controlmapper.contains( "cablight_sw:" ) ) {
            // TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
            WriteLog( "Interior Light switch is missing, or wasn't defined" );
            return;
        }
        // store lighting switch states
        if( false == Train->DynamicObject->JointCabs ) {
            // vehicles with separate cabs get separate lighting switch states
            Train->Cabine[ Train->iCabn ].bLight = false;
        }
        else {
            // joint virtual cabs share lighting switch states
            for( auto &cab : Train->Cabine ) {
                cab.bLight = false;
            }
        }
    }
}

void TTrain::OnCommand_interiorlightdimtoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->Cabine[ Train->iCabn ].bLightDim ) {
            // turn on
            OnCommand_interiorlightdimenable( Train, Command );
        }
        else {
            //turn off
            OnCommand_interiorlightdimdisable( Train, Command );
        }
    }
}

void TTrain::OnCommand_interiorlightdimenable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( Train->ggCabLightDimButton.SubModel == nullptr ) {
            // TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
            WriteLog( "Dim Interior Light switch is missing, or wasn't defined" );
            return;
        }
        // visual feedback
        Train->ggCabLightDimButton.UpdateValue( 1.0, Train->dsbSwitch );
        // store lighting switch states
        if( false == Train->DynamicObject->JointCabs ) {
            // vehicles with separate cabs get separate lighting switch states
            Train->Cabine[ Train->iCabn ].bLightDim = true;
        }
        else {
            // joint virtual cabs share lighting switch states
            for( auto &cab : Train->Cabine ) {
                cab.bLightDim = true;
            }
        }
    }
}

void TTrain::OnCommand_interiorlightdimdisable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( Train->ggCabLightDimButton.SubModel == nullptr ) {
            // TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
            WriteLog( "Dim Interior Light switch is missing, or wasn't defined" );
            return;
        }
        // visual feedback
        Train->ggCabLightDimButton.UpdateValue( 0.0, Train->dsbSwitch );
        // store lighting switch states
        if( false == Train->DynamicObject->JointCabs ) {
            // vehicles with separate cabs get separate lighting switch states
            Train->Cabine[ Train->iCabn ].bLightDim = false;
        }
        else {
            // joint virtual cabs share lighting switch states
            for( auto &cab : Train->Cabine ) {
                cab.bLightDim = false;
            }
        }
    }
}

void TTrain::OnCommand_compartmentlightstoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }

    // keep the switch from flipping back and forth if key is held down
    if( ( false == Train->mvOccupied->CompartmentLights.is_active )
     && ( false == Train->mvOccupied->CompartmentLights.is_enabled ) ) {
        // turn on
        OnCommand_compartmentlightsenable( Train, Command );
    }
    else {
        //turn off
        OnCommand_compartmentlightsdisable( Train, Command );
    }
}

void TTrain::OnCommand_compartmentlightsenable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        Train->mvOccupied->CompartmentLightsSwitch( true );
        if( Train->m_controlmapper.contains( "compartmentlights_sw:" ) ) {
            auto const istoggle{ ( static_cast<int>( Train->ggCompartmentLightsButton.type() ) & static_cast<int>( TGaugeType::toggle ) ) != 0 };
            if( istoggle ) {
                Train->mvOccupied->CompartmentLightsSwitchOff( false );
            }
        }
        // visual feedback
        if( Train->m_controlmapper.contains( "compartmentlights_sw:" ) ) {
            Train->ggCompartmentLightsButton.UpdateValue( 1.0f, Train->dsbSwitch );
        }
        if( Train->m_controlmapper.contains( "compartmentlightson_sw:" ) ) {
            Train->ggCompartmentLightsOnButton.UpdateValue( 1.0f, Train->dsbSwitch );
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        if( Train->m_controlmapper.contains( "compartmentlights_sw:" ) ) {
            if( Train->ggCompartmentLightsButton.type() == TGaugeType::push ) {
                // return the switch to neutral position
                Train->mvOccupied->CompartmentLightsSwitch( false );
                Train->mvOccupied->CompartmentLightsSwitchOff( false );
                Train->ggCompartmentLightsButton.UpdateValue( 0.5f );
            }
        }
        if( Train->m_controlmapper.contains( "compartmentlightson_sw:" ) ) {
            Train->mvOccupied->CompartmentLightsSwitch( false );
            Train->ggCompartmentLightsOnButton.UpdateValue( 0.0f, Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_compartmentlightsdisable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        Train->mvOccupied->CompartmentLightsSwitchOff( true );
        if( Train->m_controlmapper.contains( "compartmentlights_sw:" ) ) {
            auto const istoggle{ ( static_cast<int>( Train->ggCompartmentLightsButton.type() ) & static_cast<int>( TGaugeType::toggle ) ) != 0 };
            if( istoggle ) {
                Train->mvOccupied->CompartmentLightsSwitch( false );
            }
        }
        // visual feedback
        if( Train->m_controlmapper.contains( "compartmentlights_sw:" ) ) {
            Train->ggCompartmentLightsButton.UpdateValue( 0.0f, Train->dsbSwitch );
        }
        if( Train->m_controlmapper.contains( "compartmentlightsoff_sw:" ) ) {
            Train->ggCompartmentLightsOffButton.UpdateValue( 1.0f, Train->dsbSwitch );
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        if( Train->m_controlmapper.contains( "compartmentlights_sw:" ) ) {
            if( Train->ggCompartmentLightsButton.type() == TGaugeType::push ) {
                // return the switch to neutral position
                Train->mvOccupied->CompartmentLightsSwitch( false );
                Train->mvOccupied->CompartmentLightsSwitchOff( false );
                Train->ggCompartmentLightsButton.UpdateValue( 0.5f );
            }
        }
        if( Train->m_controlmapper.contains( "compartmentlightsoff_sw:" ) ) {
            Train->mvOccupied->CompartmentLightsSwitchOff( false );
            Train->ggCompartmentLightsOffButton.UpdateValue( 0.0f, Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_instrumentlighttoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( false == Train->InstrumentLightActive ) {
            // turn on
            OnCommand_instrumentlightenable( Train, Command );
        }
        else {
            //turn off
            OnCommand_instrumentlightdisable( Train, Command );
        }
    }
}

void TTrain::OnCommand_instrumentlightenable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( Train->ggInstrumentLightButton.SubModel == nullptr ) {
            // TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
            WriteLog( "Instrument Light switch is missing, or wasn't defined" );
            return;
        }
        // visual feedback
        Train->ggInstrumentLightButton.UpdateValue( 1.0, Train->dsbSwitch );

        if( true == Train->InstrumentLightActive ) { return; } // already enabled

        Train->InstrumentLightActive = true;
    }
}

void TTrain::OnCommand_instrumentlightdisable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the switch doesn't flip back and forth if key is held down
        if( Train->ggInstrumentLightButton.SubModel == nullptr ) {
            // TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
            WriteLog( "Instrument Light switch is missing, or wasn't defined" );
            return;
        }
        // visual feedback
        Train->ggInstrumentLightButton.UpdateValue( 0.0, Train->dsbSwitch );

        if( false == Train->InstrumentLightActive ) { return; } // already disabled

        Train->InstrumentLightActive = false;
    }
}

void TTrain::OnCommand_dashboardlighttoggle( TTrain *Train, command_data const &Command ) {
    if( false == Train->DashboardLightActive ) {
        OnCommand_dashboardlightenable(Train, Command);
    }
    else {
        OnCommand_dashboardlightdisable(Train, Command);
    }
}

void TTrain::OnCommand_dashboardlightenable( TTrain *Train, command_data const &Command ) {
    // only reacting to press, so the switch doesn't flip back and forth if key is held down
    if( Command.action != GLFW_PRESS ) { return; }

    if( Train->ggDashboardLightButton.SubModel == nullptr ) {
        // TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
        WriteLog( "Dashboard Light switch is missing, or wasn't defined" );
        return;
    }

    if( false == Train->DashboardLightActive ) {
        // turn on
        Train->DashboardLightActive = true;
        // visual feedback
        Train->ggDashboardLightButton.UpdateValue( 1.0, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_dashboardlightdisable( TTrain *Train, command_data const &Command ) {
    // only reacting to press, so the switch doesn't flip back and forth if key is held down
    if( Command.action != GLFW_PRESS ) { return; }

    if( Train->ggDashboardLightButton.SubModel == nullptr ) {
        // TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
        WriteLog( "Dashboard Light switch is missing, or wasn't defined" );
        return;
    }

    if( Train->DashboardLightActive ) {
        //turn off
        Train->DashboardLightActive = false;
        // visual feedback
        Train->ggDashboardLightButton.UpdateValue( 0.0, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_timetablelighttoggle( TTrain *Train, command_data const &Command ) {
    // only reacting to press, so the switch doesn't flip back and forth if key is held down
    if( Command.action != GLFW_PRESS ) { return; }

    if( false == Train->TimetableLightActive ) {
        OnCommand_timetablelightenable(Train, Command);
    }
    else {
        OnCommand_timetablelightdisable(Train, Command);
    }
}

void TTrain::OnCommand_timetablelightenable( TTrain *Train, command_data const &Command ) {
    // only reacting to press, so the switch doesn't flip back and forth if key is held down
    if( Command.action != GLFW_PRESS ) { return; }

    if( Train->ggTimetableLightButton.SubModel == nullptr ) {
        // TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
        WriteLog( "Timetable Light switch is missing, or wasn't defined" );
        return;
    }

    if( false == Train->TimetableLightActive ) {
        // turn on
        Train->TimetableLightActive = true;
        // visual feedback
        Train->ggTimetableLightButton.UpdateValue( 1.0, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_timetablelightdisable( TTrain *Train, command_data const &Command ) {
    // only reacting to press, so the switch doesn't flip back and forth if key is held down
    if( Command.action != GLFW_PRESS ) { return; }

    if( Train->ggTimetableLightButton.SubModel == nullptr ) {
        // TODO: proper control deviced definition for the interiors, that doesn't hinge of presence of 3d submodels
        WriteLog( "Timetable Light switch is missing, or wasn't defined" );
        return;
    }
    if( Train->TimetableLightActive ) {
        //turn off
        Train->TimetableLightActive = false;
        // visual feedback
        Train->ggTimetableLightButton.UpdateValue( 0.0, Train->dsbSwitch );
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
        // ignore repeats so the switch doesn't flip back and forth if key is held down
        if( false == Train->mvControlled->HeatingAllow ) {
            // turn on
            OnCommand_heatingenable( Train, Command );
        }
        else {
            //turn off
            OnCommand_heatingdisable( Train, Command );
        }
    }
    else if( Command.action == GLFW_RELEASE ) {

        if( Train->ggTrainHeatingButton.type() == TGaugeType::push ) {
            // impulse switch
            // visual feedback
            Train->ggTrainHeatingButton.UpdateValue( 0.0, Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_heatingenable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

        Train->mvOccupied->HeatingSwitch( true );
        // visual feedback
        Train->ggTrainHeatingButton.UpdateValue( 1.0, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_heatingdisable( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

        Train->mvOccupied->HeatingSwitch( false );
        // visual feedback
        Train->ggTrainHeatingButton.UpdateValue(
            ( Train->ggTrainHeatingButton.type() == TGaugeType::push ?
                1.0 :
                0.0 ),
            Train->dsbSwitch );
    }
}

void TTrain::OnCommand_generictoggle( TTrain *Train, command_data const &Command ) {

    auto const itemindex = static_cast<int>( Command.command ) - static_cast<int>( user_command::generictoggle0 );
    auto &item = Train->ggUniversals[ itemindex ];
/*
    if( item.SubModel == nullptr ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Train generic item " + std::to_string( itemindex ) + " is missing, or wasn't defined" );
        }
        return;
    }
*/

    if( Command.action == GLFW_PRESS ) {

        if( item.type() == TGaugeType::push ) {
            // impulse switch
            // turn on
            // visual feedback
            item.UpdateValue( 1.0 );
        }
        else {
            // two-state switch
            if( item.GetDesiredValue() < 0.5 ) {
                // turn on
                // visual feedback
                item.UpdateValue( 1.0 );
            }
            else {
                // turn off
                // visual feedback
                item.UpdateValue( 0.0 );
            }
        }
    }
    else if( Command.action == GLFW_RELEASE ) {

        if( item.type() == TGaugeType::push ) {
            // impulse switch
            // turn off
            // visual feedback
            item.UpdateValue( 0.0 );
        }
    }
}

void TTrain::OnCommand_springbraketoggle(TTrain *Train, command_data const &Command) {

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (false == Train->mvOccupied->SpringBrake.Activate) {
			// turn on
			OnCommand_springbrakeenable(Train, Command);
		}
		else {
			//turn off
			OnCommand_springbrakedisable(Train, Command);
		}
	}
	else if (Command.action == GLFW_RELEASE) {
		// release
		// visual feedback
		Train->ggSpringBrakeOffButton.UpdateValue(0.0, Train->dsbSwitch);
		Train->ggSpringBrakeOnButton.UpdateValue(0.0, Train->dsbSwitch);
	}
};

void TTrain::OnCommand_springbrakeenable(TTrain *Train, command_data const &Command) {
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		Train->mvOccupied->SpringBrakeActivate(true);
		// visual feedback
		Train->ggSpringBrakeOnButton.UpdateValue(1.0, Train->dsbSwitch);
		Train->ggSpringBrakeOffButton.UpdateValue(0.0, Train->dsbSwitch);
	}
	else if (Command.action == GLFW_RELEASE) {
		// release
		// visual feedback
		Train->ggSpringBrakeOnButton.UpdateValue(0.0, Train->dsbSwitch);
	}
};

void TTrain::OnCommand_springbrakedisable(TTrain *Train, command_data const &Command) {
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		Train->mvOccupied->SpringBrakeActivate(false);
		// visual feedback
		Train->ggSpringBrakeOffButton.UpdateValue(1.0, Train->dsbSwitch);
		Train->ggSpringBrakeOnButton.UpdateValue(0.0, Train->dsbSwitch);
	}
	else if (Command.action == GLFW_RELEASE) {
		// release
		// visual feedback
		Train->ggSpringBrakeOffButton.UpdateValue(0.0, Train->dsbSwitch);
	}
};
void TTrain::OnCommand_springbrakeshutofftoggle(TTrain *Train, command_data const &Command) {

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		if (false == Train->mvOccupied->SpringBrake.ShuttOff) {
			// turn on
			OnCommand_springbrakeshutoffenable(Train, Command);
		}
		else {
			//turn off
			OnCommand_springbrakeshutoffdisable(Train, Command);
		}
	}
};

void TTrain::OnCommand_springbrakeshutoffenable(TTrain *Train, command_data const &Command) {
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		Train->mvOccupied->SpringBrakeShutOff(true);
	}
};

void TTrain::OnCommand_springbrakeshutoffdisable(TTrain *Train, command_data const &Command) {
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		Train->mvOccupied->SpringBrakeShutOff(false);
	}
};

void TTrain::OnCommand_springbrakerelease(TTrain *Train, command_data const &Command) {
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down

		auto *vehicle{ Train->find_nearest_consist_vehicle(Command.freefly, Command.location) };
		if (vehicle == nullptr) { return; }
		Train->mvOccupied->SpringBrakeRelease();
	}
};

void TTrain::OnCommand_speedcontrolincrease(TTrain *Train, command_data const &Command) {
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		Train->mvOccupied->SpeedCtrlInc();
		// visual feedback
		Train->ggSpeedControlIncreaseButton.UpdateValue(1.0, Train->dsbSwitch);
	}
	else if (Command.action == GLFW_RELEASE) {
		// release
		// visual feedback
		Train->ggSpeedControlIncreaseButton.UpdateValue(0.0, Train->dsbSwitch);
	}
};

void TTrain::OnCommand_speedcontroldecrease(TTrain *Train, command_data const &Command) {
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		Train->mvOccupied->SpeedCtrlDec();
		// visual feedback
		Train->ggSpeedControlDecreaseButton.UpdateValue(1.0, Train->dsbSwitch);
	}
	else if (Command.action == GLFW_RELEASE) {
		// release
		// visual feedback
		Train->ggSpeedControlDecreaseButton.UpdateValue(0.0, Train->dsbSwitch);
	}
};

void TTrain::OnCommand_speedcontrolpowerincrease(TTrain *Train, command_data const &Command) {
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		Train->mvOccupied->SpeedCtrlPowerInc();
		// visual feedback
		Train->ggSpeedControlPowerIncreaseButton.UpdateValue(1.0, Train->dsbSwitch);
	}
	else if (Command.action == GLFW_RELEASE) {
		// release
		// visual feedback
		Train->ggSpeedControlPowerIncreaseButton.UpdateValue(0.0, Train->dsbSwitch);
	}
};

void TTrain::OnCommand_speedcontrolpowerdecrease(TTrain *Train, command_data const &Command) {
	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		Train->mvOccupied->SpeedCtrlPowerDec();
		// visual feedback
		Train->ggSpeedControlPowerDecreaseButton.UpdateValue(1.0, Train->dsbSwitch);
	}
	else if (Command.action == GLFW_RELEASE) {
		// release
		// visual feedback
		Train->ggSpeedControlPowerDecreaseButton.UpdateValue(0.0, Train->dsbSwitch);
	}
};

void TTrain::OnCommand_speedcontrolbutton(TTrain *Train, command_data const &Command) {

	auto const itemindex = static_cast<int>(Command.command) - static_cast<int>(user_command::speedcontrolbutton0);
	auto &item = Train->ggSpeedCtrlButtons[itemindex];

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		Train->mvOccupied->SpeedCtrlButton(itemindex);
		// visual feedback
		Train->ggSpeedCtrlButtons[itemindex].UpdateValue(1.0, Train->dsbSwitch);
	}
	else if (Command.action == GLFW_RELEASE) {
		// release
		// visual feedback
		Train->ggSpeedCtrlButtons[itemindex].UpdateValue(0.0, Train->dsbSwitch);
	}
};

void TTrain::OnCommand_inverterenable(TTrain *Train, command_data const &Command) {

	auto itemindex = static_cast<int>(Command.command) - static_cast<int>(user_command::inverterenable1);
	auto &item = Train->ggInverterEnableButtons[itemindex];

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		bool kier = (Train->DynamicObject->DirectionGet() * Train->mvOccupied->CabOccupied > 0);
		int flag = Train->DynamicObject->MoverParameters->InverterControlCouplerFlag;
		TDynamicObject *p = Train->DynamicObject->GetFirstDynamic(Train->mvOccupied->CabOccupied < 0 ? end::rear : end::front, flag);
		while (p)
		{
			if (p->MoverParameters->eimc[eimc_p_Pmax] > 1)
			{
				if (itemindex < p->MoverParameters->InvertersNo)
				{
					p->MoverParameters->Inverters[itemindex].Activate = true;
					break;
				}
				else
				{
					itemindex -= p->MoverParameters->InvertersNo;
				}

			}
			p = (kier ? p->Next(flag) : p->Prev(flag));
		}
		// visual feedback
		item.UpdateValue(1.0, Train->dsbSwitch);
	}
	else if (Command.action == GLFW_RELEASE) {
		// release
		// visual feedback
		item.UpdateValue(0.0, Train->dsbSwitch);
	}
};

void TTrain::OnCommand_inverterdisable(TTrain *Train, command_data const &Command) {

	auto itemindex = static_cast<int>(Command.command) - static_cast<int>(user_command::inverterdisable1);
	auto &item = Train->ggInverterDisableButtons[itemindex];

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		bool kier = (Train->DynamicObject->DirectionGet() * Train->mvOccupied->CabOccupied > 0);
		int flag = Train->DynamicObject->MoverParameters->InverterControlCouplerFlag;
		TDynamicObject *p = Train->DynamicObject->GetFirstDynamic(Train->mvOccupied->CabOccupied < 0 ? end::rear : end::front, flag);
		while (p)
		{
			if (p->MoverParameters->eimc[eimc_p_Pmax] > 1)
			{
				if (itemindex < p->MoverParameters->InvertersNo)
				{
					p->MoverParameters->Inverters[itemindex].Activate = false;
					break;
				}
				else
				{
					itemindex -= p->MoverParameters->InvertersNo;
				}

			}
			p = (kier ? p->Next(flag) : p->Prev(flag));
		}
		// visual feedback
		item.UpdateValue(1.0, Train->dsbSwitch);
	}
	else if (Command.action == GLFW_RELEASE) {
		// release
		// visual feedback
		item.UpdateValue(0.0, Train->dsbSwitch);
	}
};

void TTrain::OnCommand_invertertoggle(TTrain *Train, command_data const &Command) {

	auto itemindex = static_cast<int>(Command.command) - static_cast<int>(user_command::invertertoggle1);
	auto &item = Train->ggInverterToggleButtons[itemindex];

	if (Command.action == GLFW_PRESS) {
		// only reacting to press, so the switch doesn't flip back and forth if key is held down
		bool kier = (Train->DynamicObject->DirectionGet() * Train->mvOccupied->CabOccupied > 0);
		int flag = Train->DynamicObject->MoverParameters->InverterControlCouplerFlag;
		TDynamicObject *p = Train->DynamicObject->GetFirstDynamic(Train->mvOccupied->CabOccupied < 0 ? end::rear : end::front, flag);
		while (p)
		{
			if (p->MoverParameters->eimc[eimc_p_Pmax] > 1)
			{
				if (itemindex < p->MoverParameters->InvertersNo)
				{
					p->MoverParameters->Inverters[itemindex].Activate = !p->MoverParameters->Inverters[itemindex].Activate;
					// visual feedback
					item.UpdateValue(p->MoverParameters->Inverters[itemindex].Activate ? 1.0 : 0.0, Train->dsbSwitch);
					break;
				}
				else
				{
					itemindex -= p->MoverParameters->InvertersNo;
				}

			}
			p = (kier ? p->Next(flag) : p->Prev(flag));
		}
	}
};

void TTrain::OnCommand_doorlocktoggle(TTrain *Train, command_data const &Command) {

    if( Train->ggDoorSignallingButton.SubModel == nullptr ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Door Lock switch is missing, or wasn't defined" );
        }
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only reacting to press, so the sound can loop uninterrupted
        if( false == Train->mvOccupied->Doors.lock_enabled ) {
            // turn on
            // TODO: door lock command to send through consist
            Train->mvOccupied->LockDoors( true );
            // visual feedback
            Train->ggDoorSignallingButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            // turn off
            // TODO: door lock command to send through consist
            Train->mvOccupied->LockDoors( false );
            // visual feedback
            Train->ggDoorSignallingButton.UpdateValue( 0.0, Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_doortoggleleft( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // NOTE: test how the door state check works with consists where the occupied vehicle doesn't have opening doors
        if( false == (
                ( Train->ggDoorLeftButton.GetDesiredValue() > 0.5 )
             || ( Train->ggDoorLeftOnButton.GetDesiredValue() > 0.5 ) ) ) {
            // open
            OnCommand_dooropenleft( Train, Command );
        }
        else {
            // close
            if( ( Train->ggDoorAllOffButton.SubModel != nullptr )
             && ( Train->ggDoorLeftOffButton.SubModel == nullptr ) ) {
                // OnCommand_doorcloseall( Train, Command );
                // if two-button setup lacks dedicated closing button require the user to press appropriate button manually
                return;
            }
            else {
                OnCommand_doorcloseleft( Train, Command );
            }
        }
    }
    else if( Command.action == GLFW_RELEASE ) {

        if( true == (
              ( Train->ggDoorLeftButton.GetDesiredValue() > 0.5 )
           || ( Train->ggDoorLeftOnButton.GetDesiredValue() > 0.5 ) ) ) {
            // open
            if( ( Train->mvOccupied->Doors.has_autowarning )
             && ( Train->mvOccupied->DepartureSignal ) ) {
                // complete closing the doors
                if( ( Train->ggDoorAllOffButton.SubModel != nullptr )
                 && ( Train->ggDoorLeftOffButton.SubModel == nullptr ) ) {
                    // OnCommand_doorcloseall( Train, Command );
                    // if two-button setup lacks dedicated closing button require the user to press appropriate button manually
                    return;
                }
                else {
                    OnCommand_doorcloseleft( Train, Command );
                }
            }
            else {
                OnCommand_dooropenleft( Train, Command );
            }
        }
        else {
            // close
            if( ( Train->ggDoorAllOffButton.SubModel != nullptr )
             && ( Train->ggDoorLeftOffButton.SubModel == nullptr ) ) {
                // OnCommand_doorcloseall( Train, Command );
                // if two-button setup lacks dedicated closing button require the user to press appropriate button manually
                return;
            }
            else {
                OnCommand_doorcloseleft( Train, Command );
            }
        }
        // visual feedback
        // dedicated closing buttons are presumed to be impulse switches and return automatically to neutral position
        // NOTE: temporary arrangement, can be removed when LD system is in place
        if( Train->ggDoorLeftOffButton.SubModel )
            Train->ggDoorLeftOffButton.UpdateValue( 0.0, Train->dsbSwitch );
        if( Train->ggDoorLeftOnButton.SubModel )
            Train->ggDoorLeftOnButton.UpdateValue( 0.0, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_doorpermitleft( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }
    if( false == Train->mvOccupied->Doors.permit_presets.empty() ) { return; }

    auto const side { (
    Train->cab_to_end() == end::front ?
        side::left :
        side::right ) };

    if( Command.action == GLFW_PRESS ) {

        if( Train->ggDoorLeftPermitButton.is_push() ) {
            // impulse switch
            Train->mvOccupied->PermitDoors( side );
            // visual feedback
            Train->ggDoorLeftPermitButton.UpdateValue( 1.0, Train->dsbSwitch );
            // start potential timer for remote door control
            Train->m_doorpermittimers[ side ] = Train->mvOccupied->DoorsOpenWithPermitAfter;
        }
        else {
            // two-state switch
            auto const newstate { !( Train->ggDoorLeftPermitButton.GetDesiredValue() > 0.5 ) };

            Train->mvOccupied->PermitDoors( side, newstate );
            // visual feedback
            Train->ggDoorLeftPermitButton.UpdateValue( ( newstate ? 1.0 : 0.0 ), Train->dsbSwitch );
        }
    }
    else if( Command.action == GLFW_RELEASE ) {

        if( Train->ggDoorLeftPermitButton.is_push() ) {
            // impulse switch
            // visual feedback
            Train->ggDoorLeftPermitButton.UpdateValue( 0.0, Train->dsbSwitch );
            // reset potential remote door control timer
            Train->m_doorpermittimers[ side ] = -1.f;
        }
    }
}

void TTrain::OnCommand_doorpermitright( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_REPEAT ) { return; }
    if( false == Train->mvOccupied->Doors.permit_presets.empty() ) { return; }

    auto const side { (
        Train->cab_to_end() == end::front ?
            side::right :
            side::left ) };

    if( Command.action == GLFW_PRESS ) {

        if( Train->ggDoorRightPermitButton.type() == TGaugeType::push ) {
            // impulse switch
            Train->mvOccupied->PermitDoors( side );
            // visual feedback
            Train->ggDoorRightPermitButton.UpdateValue( 1.0, Train->dsbSwitch );
            // start potential timer for remote door control
            Train->m_doorpermittimers[ side ] = Train->mvOccupied->DoorsOpenWithPermitAfter;
        }
        else {
            // two-state switch
            auto const newstate { !( Train->ggDoorRightPermitButton.GetDesiredValue() > 0.5 ) };

            Train->mvOccupied->PermitDoors( side, newstate );
            // visual feedback
            Train->ggDoorRightPermitButton.UpdateValue( ( newstate ? 1.0 : 0.0 ), Train->dsbSwitch );
        }
    }
    else if( Command.action == GLFW_RELEASE ) {

        if( Train->ggDoorRightPermitButton.type() == TGaugeType::push ) {
            // impulse switch
            // visual feedback
            Train->ggDoorRightPermitButton.UpdateValue( 0.0, Train->dsbSwitch );
            // reset potential remote door control timer
            Train->m_doorpermittimers[ side ] = -1.f;
        }
    }
}

void TTrain::OnCommand_doorpermitpresetactivatenext( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

        Train->mvOccupied->ChangeDoorPermitPreset( 1 );
        // visual feedback
        Train->ggDoorPermitPresetButton.UpdateValue( Train->mvOccupied->Doors.permit_preset, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_doorpermitpresetactivateprevious( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {

        Train->mvOccupied->ChangeDoorPermitPreset( -1 );
        // visual feedback
        Train->ggDoorPermitPresetButton.UpdateValue( Train->mvOccupied->Doors.permit_preset, Train->dsbSwitch );
    }
}


void TTrain::OnCommand_dooropenleft( TTrain *Train, command_data const &Command ) {

    auto const remoteopencontrol {
        ( Train->mvOccupied->Doors.open_control == control_t::driver )
     || ( Train->mvOccupied->Doors.open_control == control_t::mixed ) };

    if( false == remoteopencontrol ) { return; }

    if( ( Train->ggDoorLeftOnButton.SubModel == nullptr )
     && ( Train->ggDoorLeftButton.SubModel == nullptr ) ) {

        return;
    }

    if( Command.action == GLFW_PRESS ) {
        Train->mvOccupied->OperateDoors(
            ( Train->cab_to_end() == end::front ?
                side::left :
                side::right ),
            true );
        // visual feedback
        if( Train->ggDoorLeftOnButton.SubModel != nullptr ) {
            // two separate impulse switches
            Train->ggDoorLeftOnButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            // single two-state switch
            Train->ggDoorLeftButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // visual feedback
        if( Train->ggDoorLeftOnButton.SubModel != nullptr ) {
            // two separate impulse switches
            Train->ggDoorLeftOnButton.UpdateValue( 0.0, Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_doorcloseleft( TTrain *Train, command_data const &Command ) {

    auto const remoteclosecontrol {
        ( Train->mvOccupied->Doors.close_control == control_t::driver )
     || ( Train->mvOccupied->Doors.close_control == control_t::mixed ) };

    if( false == remoteclosecontrol ) { return; }

    if( ( Train->ggDoorLeftOffButton.SubModel == nullptr )
     && ( Train->ggDoorLeftButton.SubModel == nullptr ) ) {

        return;
    }

    if( Command.action == GLFW_PRESS ) {

        if( Train->mvOccupied->Doors.has_autowarning ) {
            // automatic departure signal delays actual door closing until the button is released
            Train->mvOccupied->signal_departure( true );
        }
        else {
            // TODO: move door opening/closing to the update, so the switch animation doesn't hinge on door working
            Train->mvOccupied->OperateDoors(
                ( Train->cab_to_end() == end::front ?
                    side::left :
                    side::right ),
                false );
        }
        // visual feedback
        if( Train->ggDoorLeftOffButton.SubModel != nullptr ) {
            // two separate switches to open and close the door
            Train->ggDoorLeftOffButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            // single two-state switch
            Train->ggDoorLeftButton.UpdateValue( 0.0, Train->dsbSwitch );
        }
    }
    else if( Command.action == GLFW_RELEASE ) {

        if( Train->mvOccupied->Doors.has_autowarning ) {
            // automatic departure signal delays actual door closing until the button is released
            Train->mvOccupied->signal_departure( false );
            // now we can actually close the door
            Train->mvOccupied->OperateDoors(
                ( Train->cab_to_end() == end::front ?
                    side::left :
                    side::right ),
                false );
        }
        // visual feedback
        // dedicated closing buttons are presumed to be impulse switches and return automatically to neutral position
        if( Train->ggDoorLeftOffButton.SubModel )
            Train->ggDoorLeftOffButton.UpdateValue( 0.0, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_doortoggleright( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        // NOTE: test how the door state check works with consists where the occupied vehicle doesn't have opening doors
        if( false == (
                ( Train->ggDoorRightButton.GetDesiredValue() > 0.5 )
             || ( Train->ggDoorRightOnButton.GetDesiredValue() > 0.5 ) ) ) {
            // open
            OnCommand_dooropenright( Train, Command );
        }
        else {
            // close
            if( ( Train->ggDoorAllOffButton.SubModel != nullptr )
             && ( Train->ggDoorRightOffButton.SubModel == nullptr ) ) {
                // OnCommand_doorcloseall( Train, Command );
                // if two-button setup lacks dedicated closing button require the user to press appropriate button manually
                return;
            }
            else {
                OnCommand_doorcloseright( Train, Command );
            }
        }
    }
    else if( Command.action == GLFW_RELEASE ) {

        if( true == (
                ( Train->ggDoorRightButton.GetDesiredValue() > 0.5 )
             || ( Train->ggDoorRightOnButton.GetDesiredValue() > 0.5 ) ) ) {
            // open
            if( ( Train->mvOccupied->Doors.has_autowarning )
             && ( Train->mvOccupied->DepartureSignal ) ) {
                // complete closing the doors
                if( ( Train->ggDoorAllOffButton.SubModel != nullptr )
                 && ( Train->ggDoorRightOffButton.SubModel == nullptr ) ) {
                    // OnCommand_doorcloseall( Train, Command );
                    // if two-button setup lacks dedicated closing button require the user to press appropriate button manually
                    return;
                }
                else {
                    OnCommand_doorcloseright( Train, Command );
                }
            }
            else {
                OnCommand_dooropenright( Train, Command );
            }
        }
        else {
            // close
            if( ( Train->ggDoorAllOffButton.SubModel != nullptr )
             && ( Train->ggDoorRightOffButton.SubModel == nullptr ) ) {
                // OnCommand_doorcloseall( Train, Command );
                // if two-button setup lacks dedicated closing button require the user to press appropriate button manually
                return;
            }
            else {
                OnCommand_doorcloseright( Train, Command );
            }
        }
        // visual feedback
        // dedicated closing buttons are presumed to be impulse switches and return automatically to neutral position
        // NOTE: temporary arrangement, can be removed when LD system is in place
        if( Train->ggDoorRightOffButton.SubModel )
            Train->ggDoorRightOffButton.UpdateValue( 0.0, Train->dsbSwitch );
        if( Train->ggDoorRightOnButton.SubModel )
            Train->ggDoorRightOnButton.UpdateValue( 0.0, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_dooropenright( TTrain *Train, command_data const &Command ) {

    auto const remoteopencontrol {
        ( Train->mvOccupied->Doors.open_control == control_t::driver )
     || ( Train->mvOccupied->Doors.open_control == control_t::mixed ) };

    if( false == remoteopencontrol ) { return; }

    if( ( Train->ggDoorRightOnButton.SubModel == nullptr )
     && ( Train->ggDoorRightButton.SubModel == nullptr ) ) {

        return;
    }

    if( Command.action == GLFW_PRESS ) {

        Train->mvOccupied->OperateDoors(
            ( Train->cab_to_end() == end::front ?
                side::right :
                side::left ),
            true );
        // visual feedback
        if( Train->ggDoorRightOnButton.SubModel != nullptr ) {
            // two separate impulse switches
            Train->ggDoorRightOnButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            // single two-state switch
            Train->ggDoorRightButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // visual feedback
        if( Train->ggDoorRightOnButton.SubModel != nullptr ) {
            // two separate impulse switches
            Train->ggDoorRightOnButton.UpdateValue( 0.0, Train->dsbSwitch );
        }
    }
}

void TTrain::OnCommand_doorcloseright( TTrain *Train, command_data const &Command ) {

    auto const remoteclosecontrol {
        ( Train->mvOccupied->Doors.close_control == control_t::driver )
     || ( Train->mvOccupied->Doors.close_control == control_t::mixed ) };

    if( false == remoteclosecontrol ) { return; }

    if( ( Train->ggDoorRightOffButton.SubModel == nullptr )
     && ( Train->ggDoorRightButton.SubModel == nullptr ) ) {

        return;
    }

    if( Command.action == GLFW_PRESS ) {

        if( Train->mvOccupied->Doors.has_autowarning ) {
            // automatic departure signal delays actual door closing until the button is released
            Train->mvOccupied->signal_departure( true );
        }
        else {
            Train->mvOccupied->OperateDoors(
                ( Train->cab_to_end() == end::front ?
                    side::right :
                    side::left ),
                false );
        }
        // visual feedback
        if( Train->ggDoorRightOffButton.SubModel != nullptr ) {
            // two separate switches to open and close the door
            Train->ggDoorRightOffButton.UpdateValue( 1.0, Train->dsbSwitch );
        }
        else {
            // single two-state switch
            Train->ggDoorRightButton.UpdateValue( 0.0, Train->dsbSwitch );
        }
    }
    else if( Command.action == GLFW_RELEASE ) {

        if( Train->mvOccupied->Doors.has_autowarning ) {
            // automatic departure signal delays actual door closing until the button is released
            Train->mvOccupied->signal_departure( false );
            // now we can actually close the door
            Train->mvOccupied->OperateDoors(
                ( Train->cab_to_end() == end::front ?
                    side::right :
                    side::left ),
                false );
        }
        // visual feedback
        // dedicated closing buttons are presumed to be impulse switches and return automatically to neutral position
        if( Train->ggDoorRightOffButton.SubModel )
            Train->ggDoorRightOffButton.UpdateValue( 0.0, Train->dsbSwitch );
    }
}

void TTrain::OnCommand_dooropenall( TTrain *Train, command_data const &Command ) {

    auto const remoteopencontrol {
        ( Train->mvOccupied->Doors.open_control == control_t::driver )
     || ( Train->mvOccupied->Doors.open_control == control_t::mixed ) };

    if( false == remoteopencontrol ) { return; }

    if( Train->ggDoorAllOnButton.SubModel == nullptr ) {
        // TODO: expand definition of cab controls so we can know if the control is present without testing for presence of 3d switch
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Open All Doors switch is missing, or wasn't defined" );
        }
        return;
    }

    if( Command.action == GLFW_PRESS ) {

        Train->mvOccupied->OperateDoors( side::right, true );
        Train->mvOccupied->OperateDoors( side::left, true );
        // visual feedback
        Train->ggDoorAllOnButton.UpdateValue( 1.0, Train->dsbSwitch );
    }
    else if( Command.action == GLFW_RELEASE ) {
        // visual feedback
        Train->ggDoorAllOnButton.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_doorcloseall( TTrain *Train, command_data const &Command ) {

    auto const remoteclosecontrol {
        ( Train->mvOccupied->Doors.close_control == control_t::driver )
     || ( Train->mvOccupied->Doors.close_control == control_t::mixed ) };

    if( false == remoteclosecontrol ) { return; }

    if( Train->ggDoorAllOffButton.SubModel == nullptr ) {
        // TODO: expand definition of cab controls so we can know if the control is present without testing for presence of 3d switch
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Close All Doors switch is missing, or wasn't defined" );
        }
        return;
    }

    if( Command.action == GLFW_PRESS ) {

        if( Train->mvOccupied->Doors.has_autowarning ) {
            Train->mvOccupied->signal_departure( true );
        }
        if( Train->ggDoorAllOffButton.type() != TGaugeType::push_delayed ) {
            // delays the action until the button is released
            Train->mvOccupied->OperateDoors( side::right, false );
            Train->mvOccupied->OperateDoors( side::left, false );
        }
        // visual feedback
        Train->ggDoorLeftButton.UpdateValue( 0.0, Train->dsbSwitch );
        Train->ggDoorRightButton.UpdateValue( 0.0, Train->dsbSwitch );
        if( Train->ggDoorAllOffButton.SubModel )
            Train->ggDoorAllOffButton.UpdateValue( 1.0, Train->dsbSwitch );
    }
    else if( Command.action == GLFW_RELEASE ) {
        if( Train->mvOccupied->Doors.has_autowarning ) {
            Train->mvOccupied->signal_departure( false );
        }
        if( Train->ggDoorAllOffButton.type() == TGaugeType::push_delayed ) {
            // now we can actually close the door
            Train->mvOccupied->OperateDoors( side::right, false );
            Train->mvOccupied->OperateDoors( side::left, false );
        }
        // visual feedback
        if( Train->ggDoorAllOffButton.SubModel )
            Train->ggDoorAllOffButton.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_doorsteptoggle( TTrain *Train, command_data const &Command ) {
    // TODO: move logic/visualization code to the gauge, on_command() should return hint whether it should invoke a reaction
    if( Command.action == GLFW_PRESS ) {
        // effect
        if( false == Train->ggDoorStepButton.is_delayed() ) {
            Train->mvOccupied->PermitDoorStep( false == Train->mvOccupied->Doors.step_enabled );
        }
        // visual feedback
        auto const isactive { (
            Train->ggDoorStepButton.is_push() // always press push button
         || Train->mvOccupied->Doors.step_enabled ) }; // for toggle buttons indicate item state
        Train->ggDoorStepButton.UpdateValue( isactive ? 1 : 0 );
    }
    else if( Command.action == GLFW_RELEASE ) {
        // effect
        if( Train->ggDoorStepButton.is_delayed() ) {
            Train->mvOccupied->PermitDoorStep( false == Train->mvOccupied->Doors.step_enabled );
        }
        // visual feedback
        if( Train->ggDoorStepButton.is_push() ) {
            Train->ggDoorStepButton.UpdateValue( 0 );
        }
    }
}

void TTrain::OnCommand_doormodetoggle( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        Train->mvOccupied->ChangeDoorControlMode( false == Train->mvOccupied->Doors.remote_only );
    }
}

void TTrain::OnCommand_mirrorstoggle(TTrain *Train, command_data const &Command) {

	if (Command.action != GLFW_PRESS) { return; }

// only reacting to press, so the sound can loop uninterrupted
	if (false == Train->mvOccupied->MirrorForbidden) {
		// turn on
		Train->mvOccupied->MirrorForbidden = true;
	}
	else {
		// turn off
		Train->mvOccupied->MirrorForbidden = false;
	}
}

void TTrain::OnCommand_nearestcarcouplingincrease( TTrain *Train, command_data const &Command ) {

	if( ( true == Command.freefly )
     && ( Command.action == GLFW_PRESS ) ) {
        // tryb freefly, press only
        auto coupler { -1 };
		auto *vehicle { Train->DynamicObject->ABuScanNearestObject( Command.location, Train->DynamicObject->GetTrack(), 1, 1500, coupler ) };
        if( vehicle == nullptr )
			vehicle = Train->DynamicObject->ABuScanNearestObject( Command.location, Train->DynamicObject->GetTrack(), -1, 1500, coupler );

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

void TTrain::OnCommand_nearestcarcouplingdisconnect( TTrain *Train, command_data const &Command ) {

	if( ( true == Command.freefly )
     && ( Command.action == GLFW_PRESS ) ) {
        // tryb freefly, press only
        auto coupler { -1 };
		auto *vehicle { Train->DynamicObject->ABuScanNearestObject( Command.location, Train->DynamicObject->GetTrack(), 1, 1500, coupler ) };
        if( vehicle == nullptr )
			vehicle = Train->DynamicObject->ABuScanNearestObject( Command.location, Train->DynamicObject->GetTrack(), -1, 1500, coupler );

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

void TTrain::OnCommand_nearestcarcoupleradapterattach( TTrain *Train, command_data const &Command ) {

    if( ( true == Command.freefly )
     && ( Command.action == GLFW_PRESS ) ) {
        // tryb freefly, press only
        auto *vehicle { std::get<TDynamicObject *>( simulation::Region->find_vehicle( Command.location, 50, false, true ) ) };
        if( vehicle == nullptr ) { return; }

        auto const coupler = (
            glm::length2( glm::vec3 { vehicle->CouplerPosition( end::front ) } - Command.location ) < glm::length2( glm::vec3 { vehicle->CouplerPosition( end::rear ) } - Command.location ) ?
                end::front :
                end::rear );

        vehicle->attach_coupler_adapter( coupler );
    }
}

void TTrain::OnCommand_nearestcarcoupleradapterremove( TTrain *Train, command_data const &Command ) {

    if( ( true == Command.freefly )
     && ( Command.action == GLFW_PRESS ) ) {
        // tryb freefly, press only
        auto *vehicle { std::get<TDynamicObject *>( simulation::Region->find_vehicle( Command.location, 50, false, true ) ) };
        if( vehicle == nullptr ) { return; }

        auto const coupler = (
            glm::length2( glm::vec3 { vehicle->CouplerPosition( end::front ) } - Command.location ) < glm::length2( glm::vec3 { vehicle->CouplerPosition( end::rear ) } - Command.location ) ?
                end::front :
                end::rear );

        vehicle->remove_coupler_adapter( coupler );
    }
}

void TTrain::OnCommand_occupiedcarcouplingdisconnect( TTrain *Train, command_data const &Command ) {

//    if( false == Train->m_controlmapper.contains( "couplingdisconnect_sw:" ) ) { return; }

    if( Command.action == GLFW_PRESS ) {
        // visual feedback
        Train->m_couplingdisconnect = true;

        if( Train->iCabn == 0 ) { return; }

        if( Train->DynamicObject ) {
            Train->DynamicObject->uncouple( Train->cab_to_end() );
            if( Train->DynamicObject->Mechanik ) {
                // aktualizacja flag kierunku w składzie
                Train->DynamicObject->Mechanik->CheckVehicles( Disconnect );
            }
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // visual feedback
        Train->m_couplingdisconnect = false;
    }
}

void TTrain::OnCommand_occupiedcarcouplingdisconnectback(TTrain *Train, command_data const &Command) {

	//    if( false == Train->m_controlmapper.contains( "couplingdisconnect_sw:" ) ) { return; }

	if (Command.action == GLFW_PRESS) {
		// visual feedback
		Train->m_couplingdisconnectback = true;

		if (Train->iCabn == 0) { return; }

		if (Train->DynamicObject) {
			Train->DynamicObject->uncouple( 1 - Train->cab_to_end() );
			if (Train->DynamicObject->Mechanik) {
				// aktualizacja flag kierunku w składzie
				Train->DynamicObject->Mechanik->CheckVehicles(Disconnect);
			}
		}
	}
	else if (Command.action == GLFW_RELEASE) {
		// visual feedback
		Train->m_couplingdisconnectback = false;
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

void TTrain::OnCommand_whistleactivate( TTrain *Train, command_data const &Command ) {

    if( Train->ggWhistleButton.SubModel == nullptr ) {
        if( Command.action == GLFW_PRESS ) {
            WriteLog( "Whistle button is missing, or wasn't defined" );
        }
        return;
    }

    if( Command.action == GLFW_PRESS ) {
        // only need to react to press, sound will continue until stopped
        if( false == TestFlag( Train->mvOccupied->WarningSignal, 4 ) ) {
            // turn on
            Train->mvOccupied->WarningSignal |= 4;
            // visual feedback
            Train->ggWhistleButton.UpdateValue( 1.0 );
        }
    }
    else if( Command.action == GLFW_RELEASE ) {
        // turn off
        Train->mvOccupied->WarningSignal &= ~4;
        // visual feedback
        Train->ggWhistleButton.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_radiotoggle( TTrain *Train, command_data const &Command ) {
    if( Command.action != GLFW_PRESS ) { return; }

    // NOTE: we ignore the lack of 3d model to allow system reset after receiving radio-stop signal
/*
    if( false == Train->m_controlmapper.contains( "radio_sw:" ) ) {
        return;
    }
*/
    // only reacting to press, so the sound can loop uninterrupted
    if( false == Train->mvOccupied->Radio ) {
        // turn on
        OnCommand_radioenable(Train, Command);
    }
    else {
        // turn off
        OnCommand_radiodisable(Train, Command);
    }
}

void TTrain::OnCommand_radioenable( TTrain *Train, command_data const &Command ) {
    if( Command.action != GLFW_PRESS ) { return; }

    if( false == Train->mvOccupied->Radio ) {
        Train->mvOccupied->Radio = true;
    }
}

void TTrain::OnCommand_radiodisable( TTrain *Train, command_data const &Command ) {
    if( Command.action != GLFW_PRESS ) { return; }

    if(Train->mvOccupied->Radio ) {
        Train->mvOccupied->Radio = false;
    }
}

void TTrain::OnCommand_radiochannelincrease( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        command_data newCommand = Command;
        newCommand.param1 = Train->RadioChannel() + 1;
        OnCommand_radiochannelset(Train, newCommand);

        Train->ggRadioChannelNext.UpdateValue( 1.0 );
    }
    else if( Command.action == GLFW_RELEASE ) {
        // visual feedback
        Train->ggRadioChannelNext.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_radiochanneldecrease( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        command_data newCommand = Command;
        newCommand.param1 = Train->RadioChannel() - 1;
        OnCommand_radiochannelset(Train, newCommand);

        Train->ggRadioChannelPrevious.UpdateValue( 1.0 );
    }
    else if( Command.action == GLFW_RELEASE ) {
        // visual feedback
        Train->ggRadioChannelPrevious.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_radiochannelset(TTrain *Train, command_data const &Command) {
    if( Command.action != GLFW_RELEASE ) {
        // on press or hold
        Train->RadioChannel() = clamp((int) Command.param1, 1, 10);
        Train->ggRadioChannelSelector.UpdateValue( Train->RadioChannel() - 1 );
    }
}


void TTrain::OnCommand_radiostopsend( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        if( ( true == Train->mvOccupied->Radio )
         && ( Train->mvOccupied->Power24vIsAvailable || Train->mvOccupied->Power110vIsAvailable ) ) {
            simulation::Region->RadioStop( Train->Dynamic()->GetPosition() );
        }
        // visual feedback
        Train->ggRadioStop.UpdateValue( 1.0 );
    }
    else if( Command.action == GLFW_RELEASE ) {
        // visual feedback
        Train->ggRadioStop.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_radiostopenable( TTrain *Train, command_data const &Command ) {
    if( Command.action == GLFW_PRESS && Train->ggRadioStop.GetValue() == 0 ) {
        if( ( true == Train->mvOccupied->Radio )
         && ( Train->mvOccupied->Power24vIsAvailable || Train->mvOccupied->Power110vIsAvailable ) ) {
            simulation::Region->RadioStop( Train->Dynamic()->GetPosition() );
        }
        // visual feedback
        Train->ggRadioStop.UpdateValue( 1.0 );
    }
}

void TTrain::OnCommand_radiostopdisable( TTrain *Train, command_data const &Command ) {
    if( Command.action == GLFW_PRESS && Train->ggRadioStop.GetValue() > 0 ) {
        // visual feedback
        Train->ggRadioStop.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_radiostoptest( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        if( ( Train->RadioChannel() == 10 )
         && ( true == Train->mvOccupied->Radio )
         && ( Train->mvOccupied->Power24vIsAvailable || Train->mvOccupied->Power110vIsAvailable ) ) {
            Train->Dynamic()->RadioStop();
        }
        // visual feedback
        Train->ggRadioTest.UpdateValue( 1.0 );
    }
    else if( Command.action == GLFW_RELEASE ) {
        // visual feedback
        Train->ggRadioTest.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_radiocall3send( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        if( ( Train->RadioChannel() != 10 )
         && ( true == Train->mvOccupied->Radio )
         && ( Train->mvOccupied->Power24vIsAvailable || Train->mvOccupied->Power110vIsAvailable) ) {
            simulation::Events.queue_receivers( radio_message::call3, Train->Dynamic()->GetPosition() );
        }
        // visual feedback
        Train->ggRadioCall3.UpdateValue( 1.0 );
    }
    else if( Command.action == GLFW_RELEASE ) {
        // visual feedback
        Train->ggRadioCall3.UpdateValue( 0.0 );
    }
}

void TTrain::OnCommand_radiovolumeincrease(TTrain *Train, command_data const &Command) {

	if (Command.action == GLFW_PRESS) {
        command_data newCommand = Command;
        newCommand.param1 = Global.RadioVolume + 0.125;
        OnCommand_radiovolumeset(Train, newCommand);
		Train->ggRadioVolumeNext.UpdateValue(1.0);
	}
	else if (Command.action == GLFW_RELEASE) {
		// visual feedback
		Train->ggRadioVolumeNext.UpdateValue(0.0);
	}
}

void TTrain::OnCommand_radiovolumedecrease(TTrain *Train, command_data const &Command) {

	if (Command.action == GLFW_PRESS) {
        command_data newCommand = Command;
        newCommand.param1 = Global.RadioVolume - 0.125;
        OnCommand_radiovolumeset(Train, newCommand);
		Train->ggRadioVolumePrevious.UpdateValue(1.0);
	}
	else if (Command.action == GLFW_RELEASE) {
		// visual feedback
		Train->ggRadioVolumePrevious.UpdateValue(0.0);
	}
}

void TTrain::OnCommand_radiovolumeset(TTrain *Train, command_data const &Command) {
    if( Command.action != GLFW_RELEASE ) {
        // on press or hold
        Global.RadioVolume = clamp(Command.param1, 0.0, 1.0);
        Train->ggRadioVolumeSelector.UpdateValue(Global.RadioVolume);
    }
}


void TTrain::OnCommand_cabchangeforward( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        auto const *owner { (
            Train->DynamicObject->ctOwner != nullptr ?
                Train->DynamicObject->ctOwner :
                Train->DynamicObject->Mechanik ) };
        auto const movedirection { 1 };
        if( false == Train->CabChange( movedirection ) ) {
            auto const exitdirection { (
                movedirection > 0 ?
                    end::front :
                    end::rear ) };
            if( TestFlag( Train->mvOccupied->Couplers[ exitdirection ].CouplingFlag, coupling::gangway ) ) {
                // przejscie do nastepnego pojazdu
                auto *targetvehicle = (
                    exitdirection == end::front ?
                        Train->DynamicObject->PrevConnected() :
                        Train->DynamicObject->NextConnected() );
                targetvehicle->MoverParameters->CabOccupied = (
                    Train->mvOccupied->Neighbours[ exitdirection ].vehicle_end ?
                        -1 :
                         1 );
                Train->MoveToVehicle( targetvehicle );
            }
        }
        // HACK: match consist door permit state with the preset in the active cab
        if( Train->ggDoorPermitPresetButton.SubModel != nullptr ) {
            Train->mvOccupied->ChangeDoorPermitPreset( 0 );
        }
        // HACK: update lights state
        if( Train->mvOccupied->LightsPosNo > 0 ) {
            Train->DynamicObject->SetLights();
        }
    }
}

void TTrain::OnCommand_cabchangebackward( TTrain *Train, command_data const &Command ) {

    if( Command.action == GLFW_PRESS ) {
        auto const *owner { (
            Train->DynamicObject->ctOwner != nullptr ?
                Train->DynamicObject->ctOwner :
                Train->DynamicObject->Mechanik ) };
        auto const movedirection { -1 };
        if( false == Train->CabChange( movedirection ) ) {
            // current vehicle doesn't extend any farther in this direction, check if we there's one connected we can move to
            auto const exitdirection { (
                movedirection > 0 ?
                    end::front :
                    end::rear ) };
            if( TestFlag( Train->mvOccupied->Couplers[ exitdirection ].CouplingFlag, coupling::gangway ) ) {
                // przejscie do nastepnego pojazdu
                auto *targetvehicle = (
                    exitdirection == end::front ?
                        Train->DynamicObject->PrevConnected() :
                        Train->DynamicObject->NextConnected() );
                targetvehicle->MoverParameters->CabOccupied = (
                    Train->mvOccupied->Neighbours[ exitdirection ].vehicle_end ?
                        -1 :
                         1 );
                Train->MoveToVehicle( targetvehicle );
            }
        }
        // HACK: match consist door permit state with the preset in the active cab
        if( Train->ggDoorPermitPresetButton.SubModel != nullptr ) {
            Train->mvOccupied->ChangeDoorPermitPreset( 0 );
        }
        // HACK: update lights state
        if( Train->mvOccupied->LightsPosNo > 0 ) {
            Train->DynamicObject->SetLights();
        }
    }
}

void TTrain::OnCommand_vehiclemoveforwards(TTrain *Train, const command_data &Command) {
	if (Command.action == GLFW_RELEASE || !DebugModeFlag)
		return;

	Train->DynamicObject->move_set(100.0);
}

void TTrain::OnCommand_vehiclemovebackwards(TTrain *Train, const command_data &Command) {
	if (Command.action == GLFW_RELEASE || !DebugModeFlag)
		return;

	Train->DynamicObject->move_set(-100.0);
}

void TTrain::OnCommand_vehicleboost(TTrain *Train, const command_data &Command) {
	if (Command.action == GLFW_RELEASE || !DebugModeFlag)
		return;

	double boost = Command.param1 != 0.0 ? Command.param1 : 2.78;

    if( Train->DynamicObject == nullptr ) { return; }

    auto *vehicle { Train->DynamicObject };
	while( vehicle ) {
		vehicle->MoverParameters->V += vehicle->DirectionGet() * boost;
		vehicle = vehicle->Next(); // pozostałe też
	}
	vehicle = Train->DynamicObject->Prev();
	while( vehicle ) {
		vehicle->MoverParameters->V += vehicle->DirectionGet() * boost;
		vehicle = vehicle->Prev(); // w drugą stronę też
	}
}

// cab movement update, fixed step part
void TTrain::UpdateCab() {

    // Ra: przesiadka, jeśli AI zmieniło kabinę (a człon?)...
    if( ( DynamicObject->Mechanik ) // może nie być?
     && ( DynamicObject->Mechanik->AIControllFlag ) ) {

        if( iCabn != ( // numer kabiny (-1: kabina B)
                mvOccupied->CabOccupied == -1 ?
                    2 :
                    mvOccupied->CabOccupied ) ) {

            InitializeCab(
                mvOccupied->CabOccupied,
                mvOccupied->TypeName + ".mmd" );
        }
    }
    iCabn = ( mvOccupied->CabOccupied == -1 ?
        2 :
        mvOccupied->CabOccupied );
}

bool TTrain::Update( double const Deltatime )
{
    // train state update
    // line breaker:
    if( m_linebreakerstate == 0 ) {
        if( true == mvControlled->Mains ) {
            // crude way to sync state of the linebreaker with ai-issued commands
            m_linebreakerstate = 1;
        }
    }
    if( m_linebreakerstate == 1 ) {
        if( false == ( mvControlled->Mains || mvControlled->dizel_startup ) ) {
            // crude way to catch cases where the main was knocked out
            // because the state of the line breaker isn't changed to match, we need to do it here manually
            m_linebreakerstate = 0;
        }
    }

    if( ( ( ggMainButton.SubModel != nullptr ) && ( ggMainButton.GetDesiredValue() > 0.95 ) )
     || ( ( ggMainOnButton.SubModel != nullptr ) && ( ggMainOnButton.GetDesiredValue() > 0.95 )
     || ( ggIgnitionKey.GetDesiredValue() > 0.95 ) ) ) { // HACK: fallback
        // keep track of period the line breaker button is held down, to determine when/if circuit closes
        if( mvControlled->MainSwitchCheck() ) {
            fMainRelayTimer += Deltatime;
        }
    }
    else {
        // button isn't down, reset the timer
        fMainRelayTimer = 0.0f;
    }
    if( ggMainOffButton.GetDesiredValue() > 0.95 ) {
        // if the button disconnecting the line breaker is down prevent the timer from accumulating
        fMainRelayTimer = 0.0f;
    }
    if( m_linebreakerstate == 0 ) {
        if( fMainRelayTimer > mvControlled->InitialCtrlDelay ) {
            // wlaczanie WSa z opoznieniem
            // mark the line breaker as ready to close; for electric series vehicles with impulse switch the setup is completed on button release
            m_linebreakerstate = 2;
        }
    }
    if( m_linebreakerstate == 2 ) {
        // for diesels and/or vehicles with toggle switch setup we complete the engine start here
        // TODO: make it a test for main_on_bt of type push_delayed instead
        if( ( ggMainOnButton.SubModel == nullptr )
         || ( mvControlled->EngineType != TEngineType::ElectricSeriesMotor ) ) {
            // try to finalize state change of the line breaker, set the state based on the outcome
            m_linebreakerstate = (
                mvControlled->MainSwitch( true ) ?
                    1 :
                    0 );
        }
    }
    // door permits
    for( auto idx = 0; idx < 2; ++idx ) {
        auto &doorpermittimer { m_doorpermittimers[ idx ] };
        if( doorpermittimer < 0.f ) {
            continue;
        }
        doorpermittimer -= Deltatime;
        if( doorpermittimer < 0.f ) {
            mvOccupied->OperateDoors( static_cast<side>( idx ), true );
        }
    }
    // helper variables
    if( DynamicObject->Mechanik != nullptr ) {
        m_doors = (
            DynamicObject->Mechanik->IsAnyDoorOpen[ side::right ]
         || DynamicObject->Mechanik->IsAnyDoorOpen[ side::left ] );
        m_doorpermits = (
            DynamicObject->Mechanik->IsAnyDoorPermitActive[ side::right ]
         || DynamicObject->Mechanik->IsAnyDoorPermitActive[ side::left ] );
		m_doorspermitleft = mvOccupied->Doors.instances[(cab_to_end() == end::front ? side::left : side::right)].open_permit
							&& ((simulation::Time.data().wSecond % 2 < 1)
								|| (mvOccupied->DoorsPermitLightBlinking < 1)
								|| ((mvOccupied->DoorsPermitLightBlinking < 2)
									&& (DynamicObject->Mechanik->IsAnyDoorOpen[(cab_to_end() == end::front ? side::left : side::right)])
								|| ((mvOccupied->DoorsPermitLightBlinking < 3)
									&& DynamicObject->Mechanik->IsAnyDoorOnlyOpen[(cab_to_end() == end::front ? side::left : side::right)])));
		m_doorspermitright = mvOccupied->Doors.instances[(cab_to_end() == end::front ? side::right : side::left)].open_permit
							&& ((simulation::Time.data().wSecond % 2 < 1)
								|| (mvOccupied->DoorsPermitLightBlinking < 1)
								|| ((mvOccupied->DoorsPermitLightBlinking < 2)
									&& (DynamicObject->Mechanik->IsAnyDoorOpen[(cab_to_end() == end::front ? side::right : side::left)])
								|| ((mvOccupied->DoorsPermitLightBlinking < 3)
									&& DynamicObject->Mechanik->IsAnyDoorOnlyOpen[(cab_to_end() == end::front ? side::right : side::left)])));
    }
	m_dirforward = ( mvControlled->DirActive > 0 );
	m_dirneutral = ( mvControlled->DirActive == 0 );
	m_dirbackward = ( mvControlled->DirActive < 0 );

    // check for received user commands
    // NOTE: this is a temporary arrangement, for the transition period from old command setup to the new one
    // eventually commands are going to be retrieved directly by the vehicle, filtered through active control stand
    // and ultimately executed, provided the stand allows it.
    command_data commanddata;
    while( simulation::Commands.pop( commanddata, static_cast<std::size_t>( command_target::vehicle ) | id() ) ) {

        auto lookup = m_commandhandlers.find( commanddata.command );
        if( lookup != m_commandhandlers.end() ) {
            // debug data
            if( commanddata.action == GLFW_PRESS ) {
                WriteLog( mvOccupied->Name + " received command: [" + simulation::Commands_descriptions[ static_cast<std::size_t>( commanddata.command ) ].name + "]" );
            }
            // pass the command to the assigned handler
            lookup->second( this, commanddata );
        }
    }

    UpdateCab();

    if( ( DynamicObject->Mechanik != nullptr )
     && ( false == DynamicObject->Mechanik->AIControllFlag ) ) {
        // nie blokujemy AI
        if( ( mvOccupied->TrainType == dt_ET40 )
         || ( mvOccupied->TrainType == dt_EP05 )
         || ( mvOccupied->HasCamshaft ) ) {
            // dla ET40 i EU05 automatyczne cofanie nastawnika - i tak nie będzie to działać dobrze...
            // TODO: use deltatime to stabilize speed
/*
            if( false == (
                ( input::command == user_command::mastercontrollerset )
                || ( input::command == user_command::mastercontrollerincrease )
                || ( input::command == user_command::mastercontrollerdecrease ) ) ) {
*/
            if( false == ( m_mastercontrollerinuse || Global.ctrlState ) ) {
                m_mastercontrollerreturndelay -= Deltatime;
                if( m_mastercontrollerreturndelay < 0.f ) {
                    m_mastercontrollerreturndelay = EU07_CONTROLLER_BASERETURNDELAY;
                    if( mvOccupied->MainCtrlPos > mvOccupied->MainCtrlActualPos ) {
                        mvOccupied->DecMainCtrl( 1 );
                    }
                    else if( mvOccupied->MainCtrlPos < mvOccupied->MainCtrlActualPos ) {
                        // Ra 15-01: a to nie miało być tylko cofanie?
                        mvOccupied->IncMainCtrl( 1 );
                    }
                }
            }
        }
    }

    // McZapkie: predkosc wyswietlana na tachometrze brana jest z obrotow kol
    auto const maxtacho { 3.0 };
    fTachoVelocity = static_cast<float>( std::min( std::abs(11.31 * mvControlled->WheelDiameter * mvControlled->nrot), mvControlled->Vmax * 1.05) );
    { // skacze osobna zmienna
        float ff = simulation::Time.data().wSecond; // skacze co sekunde - pol sekundy
        // pomiar, pol sekundy ustawienie
        if (ff != fTachoTimer) // jesli w tej sekundzie nie zmienial
        {
            if (fTachoVelocity > 1) // jedzie
                fTachoVelocityJump = fTachoVelocity + (2.0 - LocalRandom(3) + LocalRandom(3)) * 0.5;
            else
                fTachoVelocityJump = 0; // stoi
            fTachoTimer = ff; // juz zmienil
        }
    }
    if (fTachoVelocity > 1) // McZapkie-270503: podkrecanie tachometru
    {
        // szybciej zacznij stukac
        fTachoCount = std::min( maxtacho, fTachoCount + Deltatime * 3 );
    }
    else if( fTachoCount > 0 ) {
        // schodz powoli - niektore haslery to ze 4 sekundy potrafia stukac
        fTachoCount = std::max( 0.0, fTachoCount - Deltatime * 0.66 );
    }

    // Ra 2014-09: napięcia i prądy muszą być ustalone najpierw, bo wysyłane są ewentualnie na PoKeys
    if( ( mvControlled->EngineType != TEngineType::DieselElectric )
     && ( mvControlled->EngineType != TEngineType::ElectricInductionMotor ) ) { // Ra 2014-09: czy taki rozdzia? ma sens?
        fHVoltage = std::max(
            mvControlled->PantographVoltage,
            mvControlled->GetTrainsetHighVoltage() ); // Winger czy to nie jest zle?
    }
    // *mvControlled->Mains);
    else {
        fHVoltage = mvControlled->EngineVoltage;
    }
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

    bool kier = (DynamicObject->DirectionGet() * mvOccupied->CabOccupied > 0);
    TDynamicObject *p = DynamicObject->GetFirstDynamic(mvOccupied->CabOccupied < 0 ? end::rear : end::front, 4);
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
		//bComp[i][2] = false;
		//bComp[i][3] = false;
        bHeat[i] = false;
    }
	bCompressors.clear();
    for (int i = 0; i < 20; i++)
    {
        if (p)
        {
            fPress[i][0] = p->MoverParameters->BrakePress;
            fPress[i][1] = p->MoverParameters->PipePress;
            fPress[i][2] = p->MoverParameters->ScndPipePress;
			fPress[i][3] = p->MoverParameters->CntrlPipePress;
			fPress[i][4] = p->MoverParameters->Hamulec->GetBRP();
			fPress[i][5] = (p->MoverParameters->TotalMass - p->MoverParameters->Mred) * 0.001;
			fPress[i][6] = p->MoverParameters->SpringBrake.SBP;
			bBrakes[i][0] = p->MoverParameters->SpringBrake.IsActive;
			bBrakes[i][1] = p->MoverParameters->SpringBrake.ShuttOff;
            bDoors[i][1] = ( p->MoverParameters->Doors.instances[ side::left ].position > 0.f );
            bDoors[i][2] = ( p->MoverParameters->Doors.instances[ side::right ].position > 0.f );
            bDoors[i][3] = ( p->MoverParameters->Doors.instances[ side::left ].step_position > 0.f );
            bDoors[i][4] = ( p->MoverParameters->Doors.instances[ side::right ].step_position > 0.f );
            bDoors[i][0] = ( bDoors[i][1] || bDoors[i][2] );
            iDoorNo[i] = p->iAnimType[ANIM_DOORS];
            iUnits[i] = iUnitNo;
            cCode[i] = p->MoverParameters->TypeName[p->MoverParameters->TypeName.length() - 1];
            asCarName[i] = p->name();
            if( p->MoverParameters->EnginePowerSource.SourceType == TPowerSource::CurrentCollector ) {
				bPants[iUnitNo - 1][end::front] = ( bPants[iUnitNo - 1][end::front] || p->MoverParameters->Pantographs[end::front].is_active );
                bPants[iUnitNo - 1][end::rear]  = ( bPants[iUnitNo - 1][end::rear]  || p->MoverParameters->Pantographs[end::rear].is_active );
            }
            // TBD, TODO: clean up compressor data arrangement?
            if( iUnitNo <= 8 ) {
			    bComp[iUnitNo - 1][0] = (bComp[iUnitNo - 1][0] || p->MoverParameters->CompressorAllow || (p->MoverParameters->CompressorStart == start_t::automatic));
            }
            if (p->MoverParameters->CompressorSpeed > 0.00001)
            {
                if( iUnitNo <= 8 ) {
				    bComp[iUnitNo - 1][1] = (bComp[iUnitNo - 1][1] || p->MoverParameters->CompressorFlag);
                }
				bCompressors.emplace_back(
					p->MoverParameters->CompressorAllow || (p->MoverParameters->CompressorStart == start_t::automatic),
					p->MoverParameters->CompressorFlag,
					i);
            }
			bSlip[i] = p->MoverParameters->SlippingWheels;
            if ((in < 8) && (p->MoverParameters->eimc[eimc_p_Pmax] > 1))
            {
                fEIMParams[1 + in][0] = p->MoverParameters->eimv[eimv_Fmax];
                fEIMParams[1 + in][1] = Max0R(fEIMParams[1 + in][0], 0);
                fEIMParams[1 + in][2] = -Min0R(fEIMParams[1 + in][0], 0);
                fEIMParams[1 + in][3] = p->MoverParameters->eimv[eimv_Fmax] / Max0R(p->MoverParameters->eimv[eimv_Fful], 1);
                fEIMParams[1 + in][4] = Max0R(fEIMParams[1 + in][3], 0);
                fEIMParams[1 + in][5] = -Min0R(fEIMParams[1 + in][3], 0);
                fEIMParams[1 + in][6] = p->MoverParameters->eimv[eimv_If];
                fEIMParams[1 + in][7] = p->MoverParameters->eimv[eimv_U];
				fEIMParams[1 + in][8] = p->MoverParameters->Itot;//p->MoverParameters->eimv[eimv_Ipoj];
                fEIMParams[1 + in][9] = p->MoverParameters->EngineVoltage;
                fEIMParams[0][6] += fEIMParams[1 + in][8];
                bMains[in] = p->MoverParameters->Mains;
                fCntVol[in] = p->MoverParameters->BatteryVoltage;
                bFuse[in] = p->MoverParameters->FuseFlag;
                bBatt[in] = p->MoverParameters->Battery;
                bConv[in] = p->MoverParameters->ConverterFlag;
                bHeat[in] = p->MoverParameters->Heating;
				//bComp[in][2] = (p->MoverParameters->CompressorAllow || (p->MoverParameters->CompressorStart == start_t::automatic));
				//bComp[in][3] = (p->MoverParameters->CompressorFlag);
                in++;
                iPowerNo = in;
            }
			if ((in < 8)
                && ((p->MoverParameters->EngineType==TEngineType::DieselEngine)
                ||(p->MoverParameters->EngineType==TEngineType::DieselElectric)))
			{
				fDieselParams[1 + in][0] = p->MoverParameters->enrot*60;
				fDieselParams[1 + in][1] = p->MoverParameters->nrot;
				fDieselParams[1 + in][2] = p->MoverParameters->RList[p->MoverParameters->MainCtrlPos].R;
				fDieselParams[1 + in][3] = p->MoverParameters->dizel_fill;
				fDieselParams[1 + in][4] = p->MoverParameters->RList[p->MoverParameters->MainCtrlPos].Mn;
				fDieselParams[1 + in][5] = p->MoverParameters->dizel_engage;
				fDieselParams[1 + in][6] = p->MoverParameters->dizel_heat.Twy;
				fDieselParams[1 + in][7] = p->MoverParameters->OilPump.pressure;
				fDieselParams[1 + in][8] = p->MoverParameters->dizel_heat.Ts;
				fDieselParams[1 + in][9] = p->MoverParameters->hydro_R_Fill;
				bMains[in] = p->MoverParameters->Mains;
				fCntVol[in] = p->MoverParameters->BatteryVoltage;
				bFuse[in] = p->MoverParameters->FuseFlag;
				bBatt[in] = p->MoverParameters->Battery;
				bConv[in] = p->MoverParameters->ConverterFlag;
				bHeat[in] = p->MoverParameters->Heating;
				in++;
				iPowerNo = in;
			}
            if ((kier ? p->Next(coupling::permanent) : p->Prev(coupling::permanent)) != (kier ? p->Next(coupling::control) : p->Prev(coupling::control)))
                iUnitNo++;
            p = (kier ? p->Next(coupling::control) : p->Prev(coupling::control));
            iCarNo = i + 1;
        }
        else
        {
            fPress[i][0]
            = fPress[i][1]
            = fPress[i][2]
			= fPress[i][3]
			= fPress[i][4]
			= fPress[i][5]
            = 0;
            bDoors[i][0]
            = bDoors[i][1]
            = bDoors[i][2]
            = bDoors[i][3]
            = bDoors[i][4]
            = false;
			bBrakes[i][0]
			= bBrakes[i][1]
			= false;
			bSlip[i] = false;
            iUnits[i] = 0;
            cCode[i] = 0; //'0';
            asCarName[i] = "";
        }
    }

//        if (mvControlled == mvOccupied)
//            fEIMParams[0][3] = mvControlled->eimv[eimv_Fzad]; // procent zadany
//        else
//            fEIMParams[0][3] =
//                mvControlled->eimv[eimv_Fzad] - mvOccupied->LocalBrakeRatio(); // procent zadany
	fEIMParams[0][3] = mvOccupied->eimic_real;
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
		for (int j = 0; j <= 9; j++)
		{
			fEIMParams[1 + i][j] = 0;
			fDieselParams[1 + i][j] = 0;
		}
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
    //------------------
    // hunter-261211: nadmiarowy przetwornicy i ogrzewania
    // Ra 15-01: to musi stąd wylecieć - zależności nie mogą być w kabinie
    if (mvControlled->ConverterFlag == true)
    {
        fConverterTimer += Deltatime;
        if ((mvControlled->CompressorFlag == true) && (mvControlled->CompressorPower == 1) &&
            ((mvControlled->EngineType == TEngineType::ElectricSeriesMotor) ||
                (mvControlled->TrainType == dt_EZT)) &&
            (DynamicObject->Controller == Humandriver) // hunter-110212: poprawka dla EZT
            && ( false == DynamicObject->Mechanik->AIControllFlag ) )
        { // hunter-091012: poprawka (zmiana warunku z CompressorPower /rozne od 0/ na /rowne 1/)
            if (fConverterTimer < fConverterPrzekaznik)
            {
                mvControlled->ConvOvldFlag = true;
                if (mvControlled->TrainType != dt_EZT)
                    mvControlled->MainSwitch( false, ( mvControlled->TrainType == dt_EZT ? range_t::unit : range_t::local ) );
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
    auto const lowvoltagepower { mvOccupied->Power24vIsAvailable || mvOccupied->Power110vIsAvailable };

    // youBy - prad w drugim czlonie: galaz lub calosc
    {
        TDynamicObject *tmp { nullptr };
        if (DynamicObject->NextConnected())
            if ((TestFlag(mvControlled->Couplers[end::rear].CouplingFlag, coupling::control)) &&
                (mvOccupied->CabOccupied == 1))
                tmp = DynamicObject->NextConnected();
        if (DynamicObject->PrevConnected())
            if ((TestFlag(mvControlled->Couplers[end::front].CouplingFlag, coupling::control)) &&
                (mvOccupied->CabOccupied == -1))
                tmp = DynamicObject->PrevConnected();
        if( tmp ) {
            if( tmp->MoverParameters->Power > 0 ) {
                if( ggI1B.SubModel ) {
                    ggI1B.UpdateValue( tmp->MoverParameters->ShowCurrent( 1 ) );
                    ggI1B.Update();
                }
                if( ggI2B.SubModel ) {
                    ggI2B.UpdateValue( tmp->MoverParameters->ShowCurrent( 2 ) );
                    ggI2B.Update();
                }
                if( ggI3B.SubModel ) {
                    ggI3B.UpdateValue( tmp->MoverParameters->ShowCurrent( 3 ) );
                    ggI3B.Update();
                }
                if( ggItotalB.SubModel ) {
                    ggItotalB.UpdateValue( tmp->MoverParameters->ShowCurrent( 0 ) );
                    ggItotalB.Update();
                }
                if( ggWater1TempB.SubModel ) {
                    ggWater1TempB.UpdateValue( tmp->MoverParameters->dizel_heat.temperatura1 );
                    ggWater1TempB.Update();
                }
                if( ggOilPressB.SubModel ) {
                    ggOilPressB.UpdateValue( tmp->MoverParameters->OilPump.pressure );
                    ggOilPressB.Update();
                }
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

    Cabine[iCabn].Update( lowvoltagepower ); // nowy sposób ustawienia animacji
/*
    if (ggZbS.SubModel)
    {
        ggZbS.UpdateValue(mvOccupied->Handle->GetCP());
        ggZbS.Update();
    }
*/
    // replacement for the above. TODO: move it to a more suitable place
    m_brakehandlecp = mvOccupied->Handle->GetCP();

    // youBy - napiecie na silnikach
    if (ggEngineVoltage.SubModel)
    {
        if (mvControlled->DynamicBrakeFlag)
        {
            ggEngineVoltage.UpdateValue(std::abs(mvControlled->Im * 5));
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
                (std::abs(mvControlled->Im) > 0))
            {
                ggEngineVoltage.UpdateValue(
                    (x * (std::abs(mvControlled->EngineVoltage) -
                            mvControlled->RList[mvControlled->MainCtrlActualPos].R *
                                std::abs(mvControlled->Im)) /
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
                ( mvOccupied->Power110vIsAvailable ?
                    mvOccupied->NominalBatteryVoltage :
                    0.0 ),
                ( mvOccupied->Power24vIsAvailable ?
                    mvOccupied->BatteryVoltage :
                    0.0 ) ) );
        ggLVoltage.Update();
    }

    if (mvControlled->EngineType == TEngineType::DieselElectric)
    { // ustawienie zmiennych dla silnika spalinowego
        fEngine[1] = mvControlled->ShowEngineRotation(1);
        fEngine[2] = mvControlled->ShowEngineRotation(2);
    }

    else if (mvControlled->EngineType == TEngineType::DieselEngine)
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
        if( ( ggIgnitionKey.SubModel)
         && ( ggIgnitionKey.GetDesiredValue() == 0.0 ) )
        {
            ggIgnitionKey.UpdateValue(
                ( mvControlled->Mains )
                || ( mvControlled->dizel_startup )
                || ( fMainRelayTimer > 0.f )
                || ( ( ggMainButton.SubModel != nullptr ) && ( ggMainButton.GetDesiredValue() > 0.95 ) )
                || ( ( ggMainOnButton.SubModel != nullptr ) && ( ggMainOnButton.GetDesiredValue() > 0.95 ) ) );
        }
        ggIgnitionKey.Update();
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

    if( true == lowvoltagepower ) {
        // McZapkie-141102: SHP i czuwak, TODO: sygnalizacja kabinowa
        if( mvOccupied->SecuritySystem.is_vigilance_blinking() ) {
            if( fBlinkTimer >  fCzuwakBlink )
                fBlinkTimer = -fCzuwakBlink;
            else
                fBlinkTimer += Deltatime;

            btLampkaCzuwaka.Turn( fBlinkTimer > 0 );
        }
        else {
            fBlinkTimer = 0.0;
            btLampkaCzuwaka.Turn( false );
        }

        btLampkaSHP.Turn(mvOccupied->SecuritySystem.is_cabsignal_blinking());
        btLampkaCzuwakaSHP.Turn( btLampkaSHP.GetValue() || btLampkaCzuwaka.GetValue() );

        btLampkaWylSzybki.Turn(
            ( ( (m_linebreakerstate == 2)
             || (true == mvControlled->Mains) ) ?
                true :
                false ) );
        btLampkaWylSzybkiOff.Turn(
            ( ( ( m_linebreakerstate == 2 )
             || ( true == mvControlled->Mains ) ) ?
                false :
                true ) );
        btLampkaMainBreakerReady.Turn(
            ( ( ( mvControlled->MainsInitTimeCountdown > 0.0 )
             || ( m_linebreakerstate == 2 )
             || ( true == mvControlled->Mains ) ) ?
                false :
                true ) );
        btLampkaMainBreakerBlinkingIfReady.Turn(
            ( ( (m_linebreakerstate == 2)
             || (true == mvControlled->Mains)
             || ( ( mvControlled->MainsInitTimeCountdown < 0.0 ) && ( simulation::Time.data().wMilliseconds > 500 ) ) ) ?
                true :
                false ) );

        btLampkaPrzetw.Turn( mvOccupied->Power110vIsAvailable );
        btLampkaPrzetwOff.Turn( false == mvOccupied->Power110vIsAvailable );
        btLampkaNadmPrzetw.Turn( Dynamic()->Mechanik ? Dynamic()->Mechanik->IsAnyConverterOverloadRelayOpen : mvControlled->ConvOvldFlag );

        btLampkaOpory.Turn(
            mvControlled->StLinFlag ?
                mvControlled->ResistorsFlagCheck() :
                false );

        btLampkaBezoporowa.Turn(
            ( true == mvControlled->ResistorsFlagCheck() )
         || ( mvControlled->MainCtrlActualPos == 0 ) ); // do EU04

        btLampkaStyczn.Turn(
            ( ( mvControlled->StLinFlag ) || ( mvControlled->ControlPressureSwitch ) ) ?
                false :
                ( mvControlled->BrakePress < 1.0 ) ); // mozna prowadzic rozruch

        btLampkaPrzekRozn.Turn(
            ( ( mvControlled->GroundRelay ) || ( mvControlled->ControlPressureSwitch ) ) ?
                false :
                ( mvControlled->BrakePress < 1.0 ) ); // relay is off and needs a reset

        btLampkaNadmSil.Turn(
            ( ( false == mvControlled->FuseFlagCheck() ) || ( mvControlled->ControlPressureSwitch ) ) ?
                false :
                ( mvControlled->BrakePress < 1.0 ) ); // relay is off and needs a reset

        if( ( ( mvControlled->CabOccupied ==  1 ) && ( TestFlag( mvControlled->Couplers[ end::rear  ].CouplingFlag, coupling::control ) ) )
         || ( ( mvControlled->CabOccupied == -1 ) && ( TestFlag( mvControlled->Couplers[ end::front ].CouplingFlag, coupling::control ) ) ) ) {
            btLampkaUkrotnienie.Turn( true );
        }
        else {
            btLampkaUkrotnienie.Turn( false );
        }

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

        btLampkaNapNastHam.Turn(mvControlled->DirActive != 0); // napiecie na nastawniku hamulcowym
        btLampkaSprezarka.Turn(mvControlled->CompressorFlag); // mutopsitka dziala
        btLampkaSprezarkaOff.Turn( false == mvControlled->CompressorFlag );
        btLampkaFuelPumpOff.Turn( false == mvControlled->FuelPump.is_active );
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

        if( mvControlled->Signalling == true ) {
            if( mvOccupied->BrakePress >= 1.45f ) {
                btLampkaHamowanie1zes.Turn( true );
            }
            if( mvControlled->BrakePress < 0.75f ) {
                btLampkaHamowanie1zes.Turn( false );
            }
        }
        else {
            btLampkaHamowanie1zes.Turn( false );
        }

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
		btLampkaRadioMessage.Turn(radio_message_played);
        btLampkaRadioStop.Turn( mvOccupied->Radio && mvOccupied->RadioStopFlag );
        btLampkaHamulecReczny.Turn(mvOccupied->ManualBrakePos > 0);
        // NBMX wrzesien 2003 - drzwi oraz sygnał odjazdu
        if( DynamicObject->Mechanik != nullptr ) {
            btLampkaDoorLeft.Turn( DynamicObject->Mechanik->IsAnyDoorOpen[ ( cab_to_end() == end::front ? side::left : side::right ) ] );
            btLampkaDoorRight.Turn( DynamicObject->Mechanik->IsAnyDoorOpen[ ( cab_to_end() == end::front ? side::right : side::left ) ] );
        }
        btLampkaBlokadaDrzwi.Turn( mvOccupied->Doors.is_locked );
        btLampkaDoorLockOff.Turn( false == mvOccupied->Doors.lock_enabled );
        btLampkaDepartureSignal.Turn( mvControlled->DepartureSignal );
        btLampkaNapNastHam.Turn((mvControlled->DirActive != 0) && (mvOccupied->EpFuse)); // napiecie na nastawniku hamulcowym
        btLampkaForward.Turn(mvControlled->DirActive > 0); // jazda do przodu
        btLampkaBackward.Turn(mvControlled->DirActive < 0); // jazda do tyłu
        btLampkaED.Turn(mvControlled->DynamicBrakeFlag); // hamulec ED
        btLampkaBrakeProfileG.Turn( TestFlag( mvOccupied->BrakeDelayFlag, bdelay_G ) );
        btLampkaBrakeProfileP.Turn( TestFlag( mvOccupied->BrakeDelayFlag, bdelay_P ) );
        btLampkaBrakeProfileR.Turn( TestFlag( mvOccupied->BrakeDelayFlag, bdelay_R ) );
		btLampkaSpringBrakeActive.Turn( mvOccupied->SpringBrake.IsActive );
		btLampkaSpringBrakeInactive.Turn( !mvOccupied->SpringBrake.IsActive );
        // light indicators
        // NOTE: sides are hardcoded to deal with setups where single cab is equipped with all indicators
        btLampkaUpperLight.Turn( ( mvOccupied->iLights[ end::front ] & light::headlight_upper ) != 0 );
        btLampkaLeftLight.Turn( ( mvOccupied->iLights[ end::front ] & light::headlight_left ) != 0 );
        btLampkaRightLight.Turn( ( mvOccupied->iLights[ end::front ] & light::headlight_right ) != 0 );
        btLampkaLeftEndLight.Turn( ( mvOccupied->iLights[ end::front ] & light::redmarker_left ) != 0 );
        btLampkaRightEndLight.Turn( ( mvOccupied->iLights[ end::front ] & light::redmarker_right ) != 0 );
        btLampkaRearUpperLight.Turn( ( mvOccupied->iLights[ end::rear ] & light::headlight_upper ) != 0 );
        btLampkaRearLeftLight.Turn( ( mvOccupied->iLights[ end::rear ] & light::headlight_left ) != 0 );
        btLampkaRearRightLight.Turn( ( mvOccupied->iLights[ end::rear ] & light::headlight_right ) != 0 );
        btLampkaRearLeftEndLight.Turn( ( mvOccupied->iLights[ end::rear ] & light::redmarker_left ) != 0 );
        btLampkaRearRightEndLight.Turn( ( mvOccupied->iLights[ end::rear ] & light::redmarker_right ) != 0 );
        // others
        btLampkaMalfunction.Turn( mvControlled->dizel_heat.PA );
        btLampkaMotorBlowers.Turn( ( mvControlled->MotorBlowers[ end::front ].is_active ) && ( mvControlled->MotorBlowers[ end::rear ].is_active ) );
        btLampkaCoolingFans.Turn( mvControlled->RventRot > 1.0 );
        btLampkaTempomat.Turn( mvOccupied->SpeedCtrlUnit.IsActive );
        btLampkaDistanceCounter.Turn( m_distancecounter >= 0.f );
        // universal devices state indicators
        for( auto idx = 0; idx < btUniversals.size(); ++idx ) {
            btUniversals[ idx ].Turn( ggUniversals[ idx ].GetValue() > 0.5 );
        }
    }
    else {
        // wylaczone
        btLampkaCzuwaka.Turn( false );
        btLampkaSHP.Turn( false );
		btLampkaCzuwakaSHP.Turn( false );
        btLampkaWylSzybki.Turn( false );
        btLampkaWylSzybkiOff.Turn( false );
        btLampkaMainBreakerReady.Turn( false );
        btLampkaMainBreakerBlinkingIfReady.Turn( false );
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
        btLampkaFuelPumpOff.Turn( false );
        btLampkaBezoporowa.Turn( false );
        btLampkaHamowanie1zes.Turn( false );
        btLampkaHamienie.Turn( false );
        btLampkaBrakingOff.Turn( false );
        btLampkaBrakeProfileG.Turn( false );
        btLampkaBrakeProfileP.Turn( false );
        btLampkaBrakeProfileR.Turn( false );
		btLampkaSpringBrakeActive.Turn( false );
		btLampkaSpringBrakeInactive.Turn( false );
        btLampkaMaxSila.Turn( false );
        btLampkaPrzekrMaxSila.Turn( false );
        btLampkaRadio.Turn( false );
		btLampkaRadioMessage.Turn( false );
        btLampkaRadioStop.Turn( false );
        btLampkaHamulecReczny.Turn( false );
        btLampkaDoorLeft.Turn( false );
        btLampkaDoorRight.Turn( false );
        btLampkaBlokadaDrzwi.Turn( false );
        btLampkaDoorLockOff.Turn( false );
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
        // others
        btLampkaMalfunction.Turn( false );
        btLampkaMotorBlowers.Turn( false );
        btLampkaCoolingFans.Turn( false );
        btLampkaTempomat.Turn( false );
        btLampkaDistanceCounter.Turn( false );
        // universal devices state indicators
        for( auto &universal : btUniversals ) {
            universal.Turn( false );
        }
    }

    { // yB - wskazniki drugiego czlonu
        TDynamicObject *tmp { nullptr }; //=mvControlled->mvSecond; //Ra 2014-07: trzeba to jeszcze wyjąć z kabiny...
        // Ra 2014-07: no nie ma potrzeby szukać tego w każdej klatce
        if ((TestFlag(mvControlled->Couplers[1].CouplingFlag, coupling::control)) &&
            (mvOccupied->CabOccupied > 0))
            tmp = DynamicObject->NextConnected();
        if ((TestFlag(mvControlled->Couplers[0].CouplingFlag, coupling::control)) &&
            (mvOccupied->CabOccupied < 0))
            tmp = DynamicObject->PrevConnected();

        if( tmp ) {
            if( lowvoltagepower ) {

                auto const *mover{ tmp->MoverParameters };

                btLampkaWylSzybkiB.Turn( mover->Mains );
                btLampkaWylSzybkiBOff.Turn(
                    ( false == mover->Mains )
                /*&& ( mover->MainsInitTimeCountdown <= 0.0 )*/
                /*&& ( fHVoltage != 0.0 )*/ );

                btLampkaOporyB.Turn( mover->ResistorsFlagCheck() );
                btLampkaBezoporowaB.Turn(
                    ( true == mover->ResistorsFlagCheck() )
                 || ( mover->MainCtrlActualPos == 0 ) ); // do EU04

                if( ( mover->StLinFlag )
                 || ( mover->ControlPressureSwitch ) ) {
                    btLampkaStycznB.Turn( false );
                }
                else if( mover->BrakePress < 1.0 ) {
                    btLampkaStycznB.Turn( true ); // mozna prowadzic rozruch
                }
                // hunter-271211: sygnalizacja poslizgu w pierwszym pojezdzie, gdy wystapi w drugim
                if( mover->SlippingWheels ) {
                    // Ra 2014-12: lokomotywy 181/182 dostają SlippingWheels po zahamowaniu powyżej 2.85 bara i buczały
                    auto const veldiff { ( DynamicObject->GetVelocity() - fTachoVelocity ) / mvControlled->Vmax };
                    if( veldiff < -0.01 ) {
                        // 1% Vmax rezerwy, żeby 181/182 nie buczały po zahamowaniu, ale to proteza
                        auto const lightstate { std::abs( mover->Im ) > 10.0 };
                        btLampkaPoslizg.Turn( btLampkaPoslizg.GetValue() || lightstate );
                    }
                }

                btLampkaSprezarkaB.Turn( mover->CompressorFlag ); // mutopsitka dziala
                btLampkaSprezarkaBOff.Turn( false == mover->CompressorFlag );
                if( mvControlled->Signalling == true ) {
                    if( mover->BrakePress >= 1.45f ) {
                        btLampkaHamowanie2zes.Turn( true );
                    }
                    if( mover->BrakePress < 0.75f ) {
                        btLampkaHamowanie2zes.Turn( false );
                    }
                }
                else {
                    btLampkaHamowanie2zes.Turn( false );
                }
                btLampkaNadmPrzetwB.Turn( mover->ConvOvldFlag ); // nadmiarowy przetwornicy?
                btLampkaPrzetwB.Turn( mover->ConverterFlag ); // zalaczenie przetwornicy
                btLampkaPrzetwBOff.Turn( false == mover->ConverterFlag );
                btLampkaHVoltageB.Turn( mover->NoVoltRelay && mover->OvervoltageRelay );
                btLampkaMalfunctionB.Turn( mover->dizel_heat.PA );
                // motor fuse indicator turns on if the fuse was blown in any unit under control
                if( mover->Mains ) {
                    btLampkaNadmSil.Turn( btLampkaNadmSil.GetValue() || mover->FuseFlagCheck() );
                }
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
                btLampkaHVoltageB.Turn( false );
                btLampkaMalfunctionB.Turn( false );
            }
        }
    }
    // McZapkie-080602: obroty (albo translacje) regulatorow
    if( ggJointCtrl.SubModel != nullptr ) {
        // joint master controller moves forward to adjust power and backward to adjust brakes
        auto const brakerangemultiplier {
            /* NOTE: scaling disabled as it was conflicting with associating sounds with control positions
            ( mvControlled->CoupledCtrl ?
                mvControlled->MainCtrlPosNo + mvControlled->ScndCtrlPosNo :
                mvControlled->MainCtrlPosNo )
            / static_cast<double>(LocalBrakePosNo)
            */
            1 };
        ggJointCtrl.UpdateValue(
            ( mvOccupied->LocalBrakePosA > 0.0 ? mvOccupied->LocalBrakePosA * LocalBrakePosNo * -1 * brakerangemultiplier :
                mvControlled->CoupledCtrl ? double( mvControlled->MainCtrlPos + mvControlled->ScndCtrlPos ) :
                double( mvControlled->MainCtrlPos ) ),
            dsbNastawnikJazdy );
        ggJointCtrl.Update();
    }
    if ( ggMainCtrl.SubModel != nullptr ) {

#ifdef _WIN32
        if( ( DynamicObject->Mechanik != nullptr )
         && ( false == DynamicObject->Mechanik->AIControllFlag ) // nie blokujemy AI
         && ( Global.iFeedbackMode == 4 )
         && ( Global.fCalibrateIn[ 2 ][ 1 ] != 0.0 ) ) {

            set_master_controller( Console::AnalogCalibrateGet( 2 ) * mvOccupied->MainCtrlPosNo );
			mvOccupied->eimic_analog = Console::AnalogCalibrateGet(2);
		}
#endif

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
    if (ggMainCtrlAct.SubModel != nullptr )
    {
        if (mvControlled->CoupledCtrl)
            ggMainCtrlAct.UpdateValue(
                double(mvControlled->MainCtrlActualPos + mvControlled->ScndCtrlActualPos));
        else
            ggMainCtrlAct.UpdateValue(double(mvControlled->MainCtrlActualPos));
        ggMainCtrlAct.Update();
    }
    if (ggScndCtrl.SubModel != nullptr ) {
        // Ra: od byte odejmowane boolean i konwertowane potem na double?
        if( false == ggScndCtrl.is_push() ) {
            ggScndCtrl.UpdateValue(
                double( mvControlled->ScndCtrlPos
                    - ( ( mvControlled->TrainType == dt_ET42 ) && mvControlled->DynamicBrakeFlag ) ),
                dsbNastawnikBocz );
        }
        ggScndCtrl.Update();
    }
    if( ggScndCtrlButton.SubModel != nullptr ) {
        if( ggScndCtrlButton.is_toggle() ) {
            ggScndCtrlButton.UpdateValue(
                ( ( mvControlled->ScndCtrlPos > 0 ) ? 1.f : 0.f ),
                dsbSwitch );
        }
        ggScndCtrlButton.Update( lowvoltagepower );
    }
    if( ggScndCtrlOffButton.SubModel != nullptr ) {
        ggScndCtrlOffButton.Update( lowvoltagepower );
    }
    if( ggDistanceCounterButton.SubModel != nullptr ) {
        ggDistanceCounterButton.Update();
    }
    if (ggDirKey.SubModel != nullptr ) {
        if( mvControlled->TrainType != dt_EZT ) {
            ggDirKey.UpdateValue(
                double( mvControlled->DirActive ),
                dsbReverserKey );
        }
        else {
            ggDirKey.UpdateValue(
                double( mvControlled->DirActive ) + double( mvControlled->Imin == mvControlled->IminHi ),
                dsbReverserKey );
        }
        ggDirKey.Update();
    }
    if (ggBrakeCtrl.SubModel != nullptr )
    {
#ifdef _WIN32
        if (DynamicObject->Mechanik ?
                (DynamicObject->Mechanik->AIControllFlag ? false :
					(Global.iFeedbackMode == 4 /*|| (Global.bMWDmasterEnable && Global.bMWDBreakEnable)*/)) :
                false && Global.fCalibrateIn[ 0 ][ 1 ] != 0.0) // nie blokujemy AI
        { // Ra: nie najlepsze miejsce, ale na początek gdzieś to dać trzeba
			// Firleju: dlatego kasujemy i zastepujemy funkcją w Console
			if (mvOccupied->BrakeHandle == TBrakeHandle::FV4a)
            {
                double b = Console::AnalogCalibrateGet(0);
				b = b * 8.0 - 2.0;
                b = clamp<double>( b, -2.0, mvOccupied->BrakeCtrlPosNo ); // przycięcie zmiennej do granic
				ggBrakeCtrl.UpdateValue(b); // przesów bez zaokrąglenia
				mvOccupied->BrakeLevelSet(b);
			}
            else if (mvOccupied->BrakeHandle == TBrakeHandle::FVel6) // może można usunąć ograniczenie do FV4a i FVel6?
            {
                double b = Console::AnalogCalibrateGet(0);
				b = b * 7.0 - 1.0;
                b = clamp<double>( b, -1.0, mvOccupied->BrakeCtrlPosNo ); // przycięcie zmiennej do granic
                ggBrakeCtrl.UpdateValue(b); // przesów bez zaokrąglenia
                mvOccupied->BrakeLevelSet(b);
            }
            else {
                double b = Console::AnalogCalibrateGet( 0 );
                b = b * ( mvOccupied->Handle->GetPos( bh_MAX ) - mvOccupied->Handle->GetPos( bh_MIN ) ) + mvOccupied->Handle->GetPos( bh_MIN );
                b = clamp<double>( b, mvOccupied->Handle->GetPos( bh_MIN ), mvOccupied->Handle->GetPos( bh_MAX ) ); // przycięcie zmiennej do granic
                ggBrakeCtrl.UpdateValue( b ); // przesów bez zaokrąglenia
                mvOccupied->BrakeLevelSet( b );
            }
        }
        else
#endif
        {
            // else //standardowa prodedura z kranem powiązanym z klawiaturą
            // ggBrakeCtrl.UpdateValue(double(mvOccupied->BrakeCtrlPos));
            ggBrakeCtrl.UpdateValue( mvOccupied->fBrakeCtrlPos );
            ggBrakeCtrl.Update();
        }
    }

    if( ggLocalBrake.SubModel != nullptr ) {
#ifdef _WIN32
        if( ( DynamicObject->Mechanik != nullptr )
         && ( false == DynamicObject->Mechanik->AIControllFlag ) // nie blokujemy AI
         && ( mvOccupied->BrakeLocHandle == TBrakeHandle::FD1 )
         && ( ( Global.iFeedbackMode == 4 )  && Global.fCalibrateIn[ 0 ][ 1 ] != 0.0
/*         || ( Global.bMWDmasterEnable && Global.bMWDBreakEnable )*/ ) ) {
            // Ra: nie najlepsze miejsce, ale na początek gdzieś to dać trzeba
            // Firleju: dlatego kasujemy i zastepujemy funkcją w Console
            auto const b = clamp<double>(
                Console::AnalogCalibrateGet( 1 ),
                0.0,
                1.0 );
            mvOccupied->LocalBrakePosA = b;
            ggLocalBrake.UpdateValue( b * LocalBrakePosNo );
        }
        else
#endif
        {
            // standardowa prodedura z kranem powiązanym z klawiaturą
            ggLocalBrake.UpdateValue( mvOccupied->LocalBrakePosA * LocalBrakePosNo );
        }
        ggLocalBrake.Update();
    }
    ggDirForwardButton.Update( lowvoltagepower );
    ggDirNeutralButton.Update( lowvoltagepower );
    ggDirBackwardButton.Update( lowvoltagepower );
    ggAlarmChain.Update();
    ggBrakeProfileCtrl.Update();
    ggBrakeProfileG.Update();
    ggBrakeProfileR.Update();
	ggBrakeOperationModeCtrl.Update();
    ggMaxCurrentCtrl.UpdateValue(
        ( true == mvControlled->ShuntModeAllow ?
            ( true == mvControlled->ShuntMode ?
                1.f :
                0.f ) :
            ( mvControlled->MotorOverloadRelayHighThreshold ?
                1.f :
                0.f ) ) );
    ggMaxCurrentCtrl.Update();
    // NBMX wrzesien 2003 - drzwi
    ggDoorLeftPermitButton.Update( lowvoltagepower );
    ggDoorRightPermitButton.Update( lowvoltagepower );
    ggDoorPermitPresetButton.Update( lowvoltagepower );
    ggDoorLeftButton.Update( lowvoltagepower );
    ggDoorRightButton.Update( lowvoltagepower );
    ggDoorLeftOnButton.Update( lowvoltagepower );
    ggDoorRightOnButton.Update( lowvoltagepower );
    ggDoorLeftOffButton.Update( lowvoltagepower );
    ggDoorRightOffButton.Update( lowvoltagepower );
    ggDoorAllOnButton.Update( lowvoltagepower );
    ggDoorAllOffButton.Update( lowvoltagepower );
    ggDoorSignallingButton.Update( lowvoltagepower );
    ggDoorStepButton.Update( lowvoltagepower );
    // NBMX dzwignia sprezarki
    ggCompressorButton.Update();
    ggCompressorLocalButton.Update();
	ggCompressorListButton.Update();

    //---------
    // hunter-080812: poprawka na ogrzewanie w elektrykach - usuniete uzaleznienie od przetwornicy
    if( ( mvControlled->Heating == true )
     && ( mvControlled->ConvOvldFlag == false ) )
        btLampkaOgrzewanieSkladu.Turn( true );
    else
        btLampkaOgrzewanieSkladu.Turn( false );

    //----------

    // lights
    auto const lightpower { (
        InstrumentLightType == 0 ? mvOccupied->Power24vIsAvailable || mvOccupied->Power110vIsAvailable :
        InstrumentLightType == 1 ? mvControlled->Mains :
        InstrumentLightType == 2 ? mvOccupied->Power110vIsAvailable :
        InstrumentLightType == 3 ? mvOccupied->Power24vIsAvailable || mvOccupied->Power110vIsAvailable :
        InstrumentLightType == 4 ? mvOccupied->Power24vIsAvailable || mvOccupied->Power110vIsAvailable :
        false ) };
    InstrumentLightActive = (
        InstrumentLightType == 3 ? true : // TODO: link the light state with the state of the master key
        InstrumentLightType == 4 ? ( mvOccupied->iLights[end::front] != 0 ) || ( mvOccupied->iLights[end::rear] != 0 ) :
        InstrumentLightActive );
    btInstrumentLight.Turn( InstrumentLightActive && lightpower );
    btDashboardLight.Turn( DashboardLightActive && lightpower );
    btTimetableLight.Turn( TimetableLightActive && lightpower );

    // guziki:
    ggMainOffButton.Update();
    ggMainOnButton.Update();
    ggMainButton.Update();
    ggSecurityResetButton.Update();
    ggSHPResetButton.Update();
    ggReleaserButton.Update();
	ggSpringBrakeOnButton.Update();
	ggSpringBrakeOffButton.Update();
	ggUniveralBrakeButton1.Update();
	ggUniveralBrakeButton2.Update();
	ggUniveralBrakeButton3.Update();
    ggEPFuseButton.Update();
    ggAntiSlipButton.Update();
    ggSandButton.Update();
    ggAutoSandButton.Update();
    ggFuseButton.Update();
    ggConverterFuseButton.Update();
    ggStLinOffButton.Update();
    ggRadioChannelSelector.Update();
    ggRadioChannelPrevious.Update();
    ggRadioChannelNext.Update();
    ggRadioStop.Update();
    ggRadioTest.Update();
    ggRadioCall3.Update();
	ggRadioVolumeSelector.Update();
	ggRadioVolumePrevious.Update();
	ggRadioVolumeNext.Update();
    ggDepartureSignalButton.Update();
/*
    ggPantFrontButton.Update();
    ggPantRearButton.Update();
    ggPantFrontButtonOff.Update();
    ggPantRearButtonOff.Update();
*/
    ggPantAllDownButton.Update();
    ggPantSelectedDownButton.Update();
    ggPantSelectedButton.Update();
    ggPantValvesButton.Update();
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
    if( DynamicObject->Mechanik != nullptr ) {
        ggHelperButton.UpdateValue( DynamicObject->Mechanik->HelperState );
    }
	ggHelperButton.Update();

    ggSpeedControlIncreaseButton.Update( lowvoltagepower );
	ggSpeedControlDecreaseButton.Update( lowvoltagepower );
	ggSpeedControlPowerIncreaseButton.Update( lowvoltagepower );
	ggSpeedControlPowerDecreaseButton.Update( lowvoltagepower );
	for (auto &speedctrlbutton : ggSpeedCtrlButtons) {
		speedctrlbutton.Update( lowvoltagepower );
	}
    for( auto &universal : ggUniversals ) {
        universal.Update();
    }
	for (auto &item : ggInverterEnableButtons) {
		item.Update();
	}
	for (auto &item : ggInverterDisableButtons) {
		item.Update();
	}
	for (auto &item : ggInverterToggleButtons) {
		item.Update();
	}
    for( auto &relayresetbutton : ggRelayResetButtons ) {
        relayresetbutton.Update();
    }
    // hunter-091012
    ggInstrumentLightButton.Update();
    ggDashboardLightButton.Update();
    ggTimetableLightButton.Update();
    ggCabLightDimButton.Update();
    ggCompartmentLightsButton.Update();
    ggCompartmentLightsOnButton.Update();
    ggCompartmentLightsOffButton.Update();
    ggBatteryButton.Update();
    ggBatteryOnButton.Update();
    ggBatteryOffButton.Update();
	if ((ggCabActivationButton.SubModel != nullptr) && (ggCabActivationButton.type() != TGaugeType::push))
	{
		ggCabActivationButton.UpdateValue(mvOccupied->IsCabMaster() ? 1.0 : 0.0);
	}
	ggCabActivationButton.Update();

    ggWaterPumpBreakerButton.Update();
    ggWaterPumpButton.Update();
    ggWaterHeaterBreakerButton.Update();
    ggWaterHeaterButton.Update();
    ggWaterCircuitsLinkButton.Update();
    ggFuelPumpButton.Update();
    ggOilPumpButton.Update();
    ggMotorBlowersFrontButton.Update();
    ggMotorBlowersRearButton.Update();
    ggMotorBlowersAllOffButton.Update();

    // wyprowadzenie sygnałów dla haslera na PoKeys (zaznaczanie na taśmie)
    btHaslerBrakes.Turn(mvOccupied->BrakePress > 0.4); // ciśnienie w cylindrach
    btHaslerCurrent.Turn(mvOccupied->Im != 0.0); // prąd na silnikach

    // calculate current level of interior illumination
    {
        // TODO: organize it along with rest of train update in a more sensible arrangement
        // Ra: uzeleżnic od napięcia w obwodzie sterowania
        // hunter-091012: uzaleznienie jasnosci od przetwornicy
        int cabidx { 0 };
        for( auto &cab : Cabine ) {

            auto const cablightlevel =
                ( ( cab.bLight == false ) ? 0.f :
                  ( cab.bLightDim == true ) ? 0.4f :
                  1.f )
                * ( mvOccupied->Power110vIsAvailable ? 1.f : 0.5f );

            if( cab.LightLevel != cablightlevel ) {
                cab.LightLevel = cablightlevel;
                DynamicObject->set_cab_lights( cabidx, cab.LightLevel );
            }
            if( cabidx == iCabn ) {
                DynamicObject->InteriorLightLevel = cablightlevel;
            }

            ++cabidx;
        }
    }

    // anti slip system activation, maintained while the control button is down
    if( mvOccupied->BrakeSystem != TBrakeSystem::ElectroPneumatic ) {
        if( ggAntiSlipButton.GetDesiredValue() > 0.95 ) {
            mvControlled->AntiSlippingBrake();
        }
    }
    // screens

    if (!FreeFlyModeFlag && simulation::Train == this) // don't bother if we're outside
        update_screens(Deltatime);

    // sounds
    update_sounds( Deltatime );

    return true; //(DynamicObject->Update(dt));
} // koniec update

void
TTrain::update_sounds( double const Deltatime ) {

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
    if( rsSBHiss ) {
        if( ( m_localbrakepressurechange < -0.05f )
         && ( mvOccupied->LocBrakePress > mvOccupied->BrakePress - 0.05 ) ) {
            rsSBHiss->gain( clamp( rsSBHiss->m_amplitudeoffset + rsSBHiss->m_amplitudefactor * -m_localbrakepressurechange * 0.05, 0.0, 1.5 ) );
            rsSBHiss->play( sound_flags::exclusive | sound_flags::looping );
        }
        else {
            // don't stop the sound too abruptly
            volume = std::max( 0.0, rsSBHiss->gain() - 0.1 * Deltatime );
            rsSBHiss->gain( volume );
            if( volume < 0.05 ) {
                rsSBHiss->stop();
            }
        }
    }
    // local brake, engage
    if( rsSBHissU ) {
        if( m_localbrakepressurechange > 0.05f ) {
            rsSBHissU->gain( clamp( rsSBHissU->m_amplitudeoffset + rsSBHissU->m_amplitudefactor * m_localbrakepressurechange * 0.05, 0.0, 1.5 ) );
            rsSBHissU->play( sound_flags::exclusive | sound_flags::looping );
        }
        else {
            // don't stop the sound too abruptly
            volume = std::max( 0.0, rsSBHissU->gain() - 0.1 * Deltatime );
            rsSBHissU->gain( volume );
            if( volume < 0.05 ) {
                rsSBHissU->stop();
            }
        }
    }

    // McZapkie-280302 - syczenie
    // TODO: softer volume reduction than plain abrupt stop, perhaps as reusable wrapper?
    if( ( mvOccupied->BrakeHandle == TBrakeHandle::FV4a )
     || ( mvOccupied->BrakeHandle == TBrakeHandle::FVel6 ) ) {
        // upuszczanie z PG
        if( rsHiss ) {
            fPPress = interpolate( fPPress, static_cast<float>( mvOccupied->Handle->GetSound( s_fv4a_b ) ), 0.05f );
            volume = (
                fPPress > 0 ?
                    rsHiss->m_amplitudefactor * fPPress * 0.25 + rsHiss->m_amplitudeoffset :
                    0 );
            if( volume * brakevolumescale > 0.05 ) {
                rsHiss->gain( volume * brakevolumescale );
                rsHiss->play( sound_flags::exclusive | sound_flags::looping );
            }
            else {
                rsHiss->stop();
            }
        }
        // napelnianie PG
        if( rsHissU ) {
            fNPress = interpolate( fNPress, static_cast<float>( mvOccupied->Handle->GetSound( s_fv4a_u ) ), 0.25f );
            volume = (
                fNPress > 0 ?
                    rsHissU->m_amplitudefactor * fNPress + rsHissU->m_amplitudeoffset :
                    0 );
            if( volume * brakevolumescale > 0.05 ) {
                rsHissU->gain( volume * brakevolumescale );
                rsHissU->play( sound_flags::exclusive | sound_flags::looping );
            }
            else {
                rsHissU->stop();
            }
        }
        // upuszczanie przy naglym
        if( rsHissE ) {
            volume = mvOccupied->Handle->GetSound( s_fv4a_e ) * rsHissE->m_amplitudefactor + rsHissE->m_amplitudeoffset;
            if( volume * brakevolumescale > 0.05 ) {
                rsHissE->gain( volume * brakevolumescale );
                rsHissE->play( sound_flags::exclusive | sound_flags::looping );
            }
            else {
                rsHissE->stop();
            }
        }
        // upuszczanie sterujacego fala
        if( rsHissX ) {
            volume = mvOccupied->Handle->GetSound( s_fv4a_x ) * rsHissX->m_amplitudefactor + rsHissX->m_amplitudeoffset;
            if( volume * brakevolumescale > 0.05 ) {
                rsHissX->gain( volume * brakevolumescale );
                rsHissX->play( sound_flags::exclusive | sound_flags::looping );
            }
            else {
                rsHissX->stop();
            }
        }
        // upuszczanie z czasowego
        if( rsHissT ) {
            volume = mvOccupied->Handle->GetSound( s_fv4a_t ) * rsHissT->m_amplitudefactor + +rsHissT->m_amplitudeoffset;
            if( volume * brakevolumescale > 0.05 ) {
                rsHissT->gain( volume * brakevolumescale );
                rsHissT->play( sound_flags::exclusive | sound_flags::looping );
            }
            else {
                rsHissT->stop();
            }
        }

    } else {
        // jesli nie FV4a
        // upuszczanie z PG
        if( rsHiss ) {
            fPPress = ( 4.0f * fPPress + std::max( 0.0, mvOccupied->dpMainValve ) ) / ( 4.0f + 1.0f );
            volume = (
                fPPress > 0.0f ?
                    2.0 * rsHiss->m_amplitudefactor * fPPress + rsHiss->m_amplitudeoffset :
                    0.0 );
            if( volume > 0.05 ) {
                rsHiss->gain( volume );
                rsHiss->play( sound_flags::exclusive | sound_flags::looping );
            }
            else {
                rsHiss->stop();
            }
        }
        // napelnianie PG
        if( rsHissU ) {
            fNPress = ( 4.0f * fNPress + Min0R( 0.0, mvOccupied->dpMainValve ) ) / ( 4.0f + 1.0f );
            volume = (
                fNPress < 0.0f ?
                    -1.0 * rsHissU->m_amplitudefactor * fNPress + rsHissU->m_amplitudeoffset :
                     0.0 );
            if( volume > 0.01 ) {
                rsHissU->gain( volume );
                rsHissU->play( sound_flags::exclusive | sound_flags::looping );
            }
            else {
                rsHissU->stop();
            }
        }
    } // koniec nie FV4a

    // brakes
    if( rsBrake ) {
        if( ( mvOccupied->UnitBrakeForce > 10.0 )
         && ( mvOccupied->Vel > 0.05 ) ) {

            auto const brakeforceratio{
                clamp(
                    mvOccupied->UnitBrakeForce / std::max( 1.0, mvOccupied->BrakeForceR( 1.0, mvOccupied->Vel ) / ( mvOccupied->NAxles * std::max( 1, mvOccupied->NBpA ) ) ),
                    0.0, 1.0 ) };
            // HACK: in external view mute the sound rather than stop it, in case there's an opening bookend it'd (re)play on sound restart after returning inside
            volume = (
                FreeFlyModeFlag ?
                    0.0 :
                    rsBrake->m_amplitudeoffset
                    + std::sqrt( brakeforceratio * interpolate( 0.4, 1.0, ( mvOccupied->Vel / ( 1 + mvOccupied->Vmax ) ) ) ) * rsBrake->m_amplitudefactor );
            rsBrake->pitch( rsBrake->m_frequencyoffset + mvOccupied->Vel * rsBrake->m_frequencyfactor );
            rsBrake->gain( volume );
            rsBrake->play( sound_flags::exclusive | sound_flags::looping );
        }
        else {
            rsBrake->stop();
        }
    }

    // ambient sound
    // since it's typically ticking of the clock we can center it on tachometer or on middle of compartment bounding area
    if( rsFadeSound ) {
        rsFadeSound->play( sound_flags::exclusive | sound_flags::looping );
    }

    if( dsbSlipAlarm ) {
        // alarm przy poslizgu dla 181/182 - BOMBARDIER
        if( ( mvControlled->SlippingWheels )
         && ( DynamicObject->GetVelocity() > 1.0 ) ) {
            dsbSlipAlarm->play( sound_flags::exclusive | sound_flags::looping );
        }
        else {
            dsbSlipAlarm->stop();
        }
    }
    // szum w czasie jazdy
    if( rsRunningNoise ) {
        if( ( false == FreeFlyModeFlag )
         && ( false == Global.CabWindowOpen )
         && ( DynamicObject->GetVelocity() > 0.5 ) ) {

            update_sounds_runningnoise( *rsRunningNoise );
        }
        else {
            // don't play the optional ending sound if the listener switches views
            rsRunningNoise->stop( true == FreeFlyModeFlag );
        }
    }
    // hunting oscillation noise
    if( rsHuntingNoise ) {
        if( ( false == FreeFlyModeFlag )
         && ( false == Global.CabWindowOpen )
         && ( DynamicObject->GetVelocity() > 0.5 )
         && ( DynamicObject->IsHunting ) ) {

            update_sounds_runningnoise( *rsHuntingNoise );
            // modify calculated sound volume by hunting amount
            auto const huntingamount =
                interpolate(
                    0.0, 1.0,
                    clamp(
                    ( mvOccupied->Vel - DynamicObject->HuntingShake.fadein_begin ) / ( DynamicObject->HuntingShake.fadein_end - DynamicObject->HuntingShake.fadein_begin ),
                        0.0, 1.0 ) );

            rsHuntingNoise->gain( rsHuntingNoise->gain() * huntingamount );
        }
        else {
            // don't play the optional ending sound if the listener switches views
            rsHuntingNoise->stop( true == FreeFlyModeFlag );
        }
    }
    // rain sound
    if( m_rainsound ) {
        if( ( false == FreeFlyModeFlag )
         && ( false == Global.CabWindowOpen )
         && ( Global.Weather == "rain:" ) ) {
            if( m_rainsound->is_combined() ) {
                m_rainsound->pitch( Global.Overcast - 1.0 );
            }
            m_rainsound->gain( m_rainsound->m_amplitudeoffset + m_rainsound->m_amplitudefactor * 1.f );
            m_rainsound->play( sound_flags::exclusive | sound_flags::looping );
        }
        else {
            m_rainsound->stop();
        }
    }

    if( dsbHasler ) {
        if( fTachoCount >= 3.f ) {
            auto const frequency{ (
                true == dsbHasler->is_combined() ?
                    fTachoVelocity * 0.01 :
                    dsbHasler->m_frequencyoffset + dsbHasler->m_frequencyfactor ) };
            dsbHasler->pitch( frequency );
            dsbHasler->gain( dsbHasler->m_amplitudeoffset + dsbHasler->m_amplitudefactor );
            dsbHasler->play( sound_flags::exclusive | sound_flags::looping );
        }
        else if( fTachoCount < 1.f ) {
            dsbHasler->stop();
        }
    }

    // power-reliant sounds
    if( mvOccupied->Power24vIsAvailable || mvOccupied->Power110vIsAvailable ) {
        // McZapkie-141102: SHP i czuwak, TODO: sygnalizacja kabinowa
            // hunter-091012: rozdzielenie alarmow
        if( mvOccupied->SecuritySystem.is_beeping() ) {
            if( dsbBuzzer && false == dsbBuzzer->is_playing() ) {
                dsbBuzzer->pitch( dsbBuzzer->m_frequencyoffset + dsbBuzzer->m_frequencyfactor );
                dsbBuzzer->gain( dsbBuzzer->m_amplitudeoffset + dsbBuzzer->m_amplitudefactor );
                dsbBuzzer->play( sound_flags::looping );
#ifdef _WIN32
                Console::BitsSet( 1 << 14 ); // ustawienie bitu 16 na PoKeys
#endif
            }
	    }
        else {
            if( dsbBuzzer && true == dsbBuzzer->is_playing() ) {
                dsbBuzzer->stop();
#ifdef _WIN32
                Console::BitsClear( 1 << 14 ); // ustawienie bitu 16 na PoKeys
#endif
            }
        }
        // distance meter alert
        if( m_distancecounterclear ) {
            auto const *owner{ (
                DynamicObject->ctOwner != nullptr ?
                    DynamicObject->ctOwner :
                    DynamicObject->Mechanik ) };
            if( m_distancecounter > owner->fLength ) {
                // play assigned sound if the train travelled its full length since meter activation
                // TBD: check all combinations of directions and active cab
                m_distancecounter = -1.f; // turn off the meter after its task is done
                m_distancecounterclear->pitch( m_distancecounterclear->m_frequencyoffset + m_distancecounterclear->m_frequencyfactor );
                m_distancecounterclear->gain( m_distancecounterclear->m_amplitudeoffset + m_distancecounterclear->m_amplitudefactor );
                m_distancecounterclear->play( sound_flags::exclusive );
            }
        }
    }
    else {
        // stop power-reliant sounds if power is cut
        if( dsbBuzzer ) {
            if( true == dsbBuzzer->is_playing() ) {
                dsbBuzzer->stop();
#ifdef _WIN32
                Console::BitsClear( 1 << 14 ); // ustawienie bitu 16 na PoKeys
#endif
            }
        }
        if( m_distancecounterclear ) {
            m_distancecounterclear->stop();
        }
    }

    update_sounds_radio();
}

void TTrain::update_sounds_runningnoise( sound_source &Sound ) {
    // frequency calculation
    auto const normalizer { (
        true == Sound.is_combined() ?
            mvOccupied->Vmax * 0.01f :
            1.f ) };
    auto const frequency {
        Sound.m_frequencyoffset
        + Sound.m_frequencyfactor * mvOccupied->Vel * normalizer };

    // volume calculation
    auto volume =
        Sound.m_amplitudeoffset
        + Sound.m_amplitudefactor * interpolate(
            mvOccupied->Vel / ( 1 + mvOccupied->Vmax ), 1.0,
            0.5 ); // scale base volume between 0.5-1.0
    if( std::abs( mvOccupied->nrot ) > 0.01 ) {
        // hamulce wzmagaja halas
        auto const brakeforceratio { (
        clamp(
            mvOccupied->UnitBrakeForce / std::max( 1.0, mvOccupied->BrakeForceR( 1.0, mvOccupied->Vel ) / ( mvOccupied->NAxles * std::max( 1, mvOccupied->NBpA ) ) ),
            0.0, 1.0 ) ) };

        volume *= 1 + 0.125 * brakeforceratio;
    }
    // scale volume by track quality
    // TODO: track quality and/or environment factors as separate subroutine
    volume *=
        interpolate(
            0.8, 1.2,
            clamp(
                DynamicObject->MyTrack->iQualityFlag / 20.0,
                0.0, 1.0 ) );
    // for single sample sounds muffle the playback at low speeds
    if( false == Sound.is_combined() ) {
        volume *=
            interpolate(
                0.0, 1.0,
                clamp(
                    mvOccupied->Vel / 25.0,
                    0.0, 1.0 ) );
    }

    if( volume > 0.05 ) {
        Sound
            .pitch( frequency )
            .gain( volume )
            .play( sound_flags::exclusive | sound_flags::looping );
    }
    else {
        Sound.stop();
    }
}

void TTrain::update_sounds_radio() {

	radio_message_played = false;
    if( false == m_radiomessages.empty() ) {
        // erase completed radio messages from the list
        m_radiomessages.erase(
            std::remove_if(
                std::begin( m_radiomessages ), std::end( m_radiomessages ),
                []( auto const &source ) {
                    return ( false == source.second->is_playing() ); } ),
            std::end( m_radiomessages ) );
    }
    // adjust audibility of remaining messages based on current radio conditions
    auto const radioenabled { ( true == mvOccupied->Radio ) && ( mvOccupied->Power24vIsAvailable || mvOccupied->Power110vIsAvailable ) };
    for( auto &message : m_radiomessages ) {
        auto const volume {
            ( true == radioenabled )
         && ( Dynamic()->Mechanik != nullptr )
         && ( message.first == RadioChannel() ) ?
                Global.RadioVolume :
                0.0 };
        message.second->gain( volume );
		radio_message_played |= (true == radioenabled) && (Dynamic()->Mechanik != nullptr) && (message.first == RadioChannel());
    }
    // radiostop
    if( m_radiostop ) {
        if( ( true == radioenabled )
         && ( true == mvOccupied->RadioStopFlag ) ) {
            m_radiostop->play( sound_flags::exclusive | sound_flags::looping );
			radio_message_played |= true;
        }
        else {
            m_radiostop->stop();
        }
    }
	if (radio_message_played)
	{
		btLampkaRadioMessage.gain(Global.RadioVolume);
	}
}

void TTrain::update_screens(double dt) {
    for (auto &screen : m_screens) {
        if (screen.updatetimecounter >= 0)
            screen.updatetimecounter += dt;

        if (screen.updatetimecounter <= screen.updatetime)
            continue;

        screen.updatetimecounter = screen.updatetime > 0 ? 0 : -1;

        auto state_dict = GetTrainState(screen.parameters);

        state_dict->insert("touches", *screen.touch_list);
        screen.touch_list->clear();

        Application.request({ screen.script, state_dict, screen.rt } );
    }
}

void TTrain::add_distance( double const Distance ) {

    auto const meterenabled { ( m_distancecounter >= 0 ) && ( mvOccupied->Power24vIsAvailable || mvOccupied->Power110vIsAvailable) };

    if( true == meterenabled ) { m_distancecounter += Distance * Occupied()->CabOccupied; }
    else                       { m_distancecounter  = -1.f; }
}

bool TTrain::CabChange(int iDirection)
{ // McZapkie-090902: zmiana kabiny 1->0->2 i z powrotem
    if( ( DynamicObject->Mechanik == nullptr )
     || ( true == DynamicObject->Mechanik->AIControllFlag ) ) {
        // jeśli prowadzi AI albo jest w innym członie
        // jak AI prowadzi, to nie można mu mieszać
        if (std::abs(mvOccupied->CabOccupied + iDirection) > 1)
            return false; // ewentualna zmiana pojazdu
        mvOccupied->CabOccupied += iDirection;
    }
    else
    { // jeśli pojazd prowadzony ręcznie albo wcale (wagon)
        mvOccupied->CabDeactivisationAuto();
        if( mvOccupied->ChangeCab( iDirection ) ) {
            if( InitializeCab( mvOccupied->CabOccupied, mvOccupied->TypeName + ".mmd" ) ) {
                // zmiana kabiny w ramach tego samego pojazdu
                mvOccupied->CabActivisationAuto(); // załączenie rozrządu (wirtualne kabiny)
                DynamicObject->Mechanik->DirectionChange();
                return true; // udało się zmienić kabinę
            }
        }
        // aktywizacja poprzedniej, bo jeszcze nie wiadomo, czy jakiś pojazd jest
        mvOccupied->CabActivisationAuto();
    }
    return false; // ewentualna zmiana pojazdu
}

// McZapkie-310302
// wczytywanie pliku z danymi multimedialnymi (dzwieki, kontrolki, kabiny)
bool TTrain::LoadMMediaFile(std::string const &asFileName)
{
    // initialize sounds so potential entries from previous vehicle don't stick around
    std::unordered_map<
        std::string,
        std::tuple<std::optional<sound_source> &, sound_placement, float, sound_type, int, double>>
        internalsounds = {
            {"ctrl:", {dsbNastawnikJazdy, sound_placement::internal, EU07_SOUND_CABCONTROLSCUTOFFRANGE, sound_type::single, 0, 100.0}},
            {"ctrlscnd:", {dsbNastawnikBocz, sound_placement::internal, EU07_SOUND_CABCONTROLSCUTOFFRANGE, sound_type::single, 0, 100.0}},
            {"reverserkey:", {dsbReverserKey, sound_placement::internal, EU07_SOUND_CABCONTROLSCUTOFFRANGE, sound_type::single, 0, 100.0}},
            {"buzzer:", {dsbBuzzer, sound_placement::internal, EU07_SOUND_CABCONTROLSCUTOFFRANGE, sound_type::single, 0, 100.0}},
            {"radiostop:", {m_radiostop, sound_placement::internal, EU07_SOUND_CABCONTROLSCUTOFFRANGE, sound_type::single, 0, 100.0}},
            {"slipalarm:", {dsbSlipAlarm, sound_placement::internal, EU07_SOUND_CABCONTROLSCUTOFFRANGE, sound_type::single, 0, 100.0}},
            {"distancecounter:", {m_distancecounterclear, sound_placement::internal, EU07_SOUND_CABCONTROLSCUTOFFRANGE, sound_type::single, 0, 100.0}},
            {"tachoclock:", {dsbHasler, sound_placement::internal, EU07_SOUND_CABCONTROLSCUTOFFRANGE, sound_type::single, 0, 100.0}},
            {"switch:", {dsbSwitch, sound_placement::internal, EU07_SOUND_CABCONTROLSCUTOFFRANGE, sound_type::single, 0, 100.0}},
            {"pneumaticswitch:", {dsbPneumaticSwitch, sound_placement::internal, EU07_SOUND_CABCONTROLSCUTOFFRANGE, sound_type::single, 0, 100.0}},
            {"airsound:", {rsHiss, sound_placement::internal, EU07_SOUND_CABCONTROLSCUTOFFRANGE, sound_type::single, sound_parameters::amplitude, 100.0}},
            {"airsound2:", {rsHissU, sound_placement::internal, EU07_SOUND_CABCONTROLSCUTOFFRANGE, sound_type::single, sound_parameters::amplitude, 100.0}},
            {"airsound3:", {rsHissE, sound_placement::internal, EU07_SOUND_CABCONTROLSCUTOFFRANGE, sound_type::single, sound_parameters::amplitude, 100.0}},
            {"airsound4:", {rsHissX, sound_placement::internal, EU07_SOUND_CABCONTROLSCUTOFFRANGE, sound_type::single, sound_parameters::amplitude, 100.0}},
            {"airsound5:", {rsHissT, sound_placement::internal, EU07_SOUND_CABCONTROLSCUTOFFRANGE, sound_type::single, sound_parameters::amplitude, 100.0}},
            {"localbrakesound:", {rsSBHiss, sound_placement::internal, EU07_SOUND_CABCONTROLSCUTOFFRANGE, sound_type::single, sound_parameters::amplitude, 100.0}},
            {"localbrakesound2:", {rsSBHissU, sound_placement::internal, EU07_SOUND_CABCONTROLSCUTOFFRANGE, sound_type::single, sound_parameters::amplitude, 100.0}},
            {"brakesound:", {rsBrake, sound_placement::internal, -1, sound_type::single, sound_parameters::amplitude | sound_parameters::frequency, 100.0}},
            {"fadesound:", {rsFadeSound, sound_placement::internal, EU07_SOUND_CABCONTROLSCUTOFFRANGE, sound_type::single, 0, 100.0}},
            {"runningnoise:", {rsRunningNoise, sound_placement::internal, EU07_SOUND_GLOBALRANGE, sound_type::single, sound_parameters::amplitude | sound_parameters::frequency, mvOccupied->Vmax }},
            {"huntingnoise:", {rsHuntingNoise, sound_placement::internal, EU07_SOUND_GLOBALRANGE, sound_type::single, sound_parameters::amplitude | sound_parameters::frequency, mvOccupied->Vmax }},
            {"rainsound:", {m_rainsound, sound_placement::internal, -1, sound_type::single, 0, 100.0}},
        };
    for( auto &soundconfig : internalsounds ) {
        std::get<std::optional<sound_source> &>( soundconfig.second ).reset();
    }
    // NOTE: since radiosound is an incomplete template not using std::optional it gets a special treatment
    m_radiosound.owner( DynamicObject );
	CabSoundLocations.clear();

    cParser parser( asFileName, cParser::buffer_FILE, DynamicObject->asBaseDir );
    // NOTE: yaml-style comments are disabled until conflict in use of # is resolved
    // parser.addCommentStyle( "#", "\n" );
    std::string token;
    do
    {
        token = "";
        parser.getTokens();
        parser >> token;
    } while ((token != "") && (token != "internaldata:"));

    if (token == "internaldata:") {

        do {
            token = "";
            parser.getTokens();
            parser >> token;

            auto lookup { internalsounds.find( token ) };
            if( lookup == internalsounds.end() ) { continue; }

            auto const soundconfig { lookup->second };
            sound_source sound {
                std::get<sound_placement>( soundconfig ),
                std::get<float>( soundconfig ) };
            sound.deserialize(
                parser,
                std::get<sound_type>( soundconfig ),
                std::get<int>( soundconfig ),
                std::get<double>( soundconfig ) );
            sound.owner( DynamicObject );
            std::get<std::optional<sound_source> &>( soundconfig ) = sound;
        } while( token != "" );


        // assign default samples to sound emitters which weren't included in the config file
        if( !m_rainsound ) {
            sound_source rainsound;
            rainsound.deserialize( "rainsound_default", sound_type::single );
            rainsound.owner( DynamicObject );
            m_rainsound = rainsound;
        }
        if (!rsSBHiss) {
            // fallback for vehicles without defined local brake hiss sound
            rsSBHiss = rsHiss;
        }
        if (!rsSBHissU) {
            // fallback for vehicles without defined local brake hiss sound
            rsSBHissU = rsHissU;
        }
        if (rsBrake) {
            rsBrake->m_frequencyfactor /= (1 + mvOccupied->Vmax);
        }
        if (rsRunningNoise) {
            rsRunningNoise->m_frequencyfactor /= (1 + mvOccupied->Vmax);
        }
        if (rsHuntingNoise) {
            rsHuntingNoise->m_frequencyfactor /= (1 + mvOccupied->Vmax);
        }
    }
	auto const nullvector{ glm::vec3() };
	std::vector<std::reference_wrapper<std::optional<sound_source>>> sounds = {
		dsbReverserKey, dsbNastawnikJazdy, dsbNastawnikBocz,
		dsbSwitch, dsbPneumaticSwitch,
		rsHiss, rsHissU, rsHissE, rsHissX, rsHissT, rsSBHiss, rsSBHissU,
		rsFadeSound, rsRunningNoise, rsHuntingNoise,
		dsbHasler, dsbBuzzer, dsbSlipAlarm, m_distancecounterclear, m_rainsound, m_radiostop
	};
	for (auto &sound : sounds) {
		if (sound.get()) {
			CabSoundLocations.emplace_back(sound, sound.get()->offset());
		}
	}

    return true;
}

bool TTrain::InitializeCab(int NewCabNo, std::string const &asFileName)
{
    m_controlmapper.clear();
    // clear python screens
    m_screens.clear();
    // reset sound positions
    auto const nullvector { glm::vec3() };
    std::vector<std::reference_wrapper<std::optional<sound_source>>> sounds = {
        dsbReverserKey, dsbNastawnikJazdy, dsbNastawnikBocz,
        dsbSwitch, dsbPneumaticSwitch,
        rsHiss, rsHissU, rsHissE, rsHissX, rsHissT, rsSBHiss, rsSBHissU,
        rsFadeSound, rsRunningNoise, rsHuntingNoise,
        dsbHasler, dsbBuzzer, dsbSlipAlarm, m_distancecounterclear, m_rainsound, m_radiostop
    };
    for( auto &sound : sounds ) {
        if( sound.get() ) {
            sound.get()->offset( nullvector );
        }
    }
    m_radiosound.offset( nullvector );
	for (auto &sound : CabSoundLocations) {
		if ((sound.first.get())
			&& (sound.first.get()->offset() == nullvector)) {
			sound.first.get()->offset(sound.second);
		}
	}
    // reset view angles
    pMechViewAngle = { 0.0, 0.0 };

    is_cab_initialized = true; // the attempt may fail, but it's the attempt that counts

    bool parse = false;
    int cabindex = 0;
    DynamicObject->mdKabina = nullptr; // likwidacja wskaźnika na dotychczasową kabinę
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
    iCabn = cabindex;

    std::string cabstr("cab" + std::to_string(cabindex) + "definition:");

    cParser parser( asFileName, cParser::buffer_FILE, DynamicObject->asBaseDir );
	parser.allowRandomIncludes = true;
    // NOTE: yaml-style comments are disabled until conflict in use of # is resolved
    // parser.addCommentStyle( "#", "\n" );
    std::string token;
    do
    {
        // szukanie kabiny
        token = "";
        parser.getTokens();
        parser >> token;
    } while ((token != "") && (token != cabstr));

    if (token == cabstr)
    {
        // jeśli znaleziony wpis kabiny
        Cabine[cabindex].Load(parser);
        // NOTE: the position and angle definitions depend on strict entry order
        // TODO: refactor into more flexible arrangement
        parser.getTokens();
        parser >> token;
        if( token == std::string( "driver" + std::to_string( cabindex ) + "angle:" ) ) {
            // camera view angle
            parser.getTokens( 2, false );
            // angle is specified in degrees but internally stored in radians
            glm::vec2 viewangle;
            parser
                >> viewangle.y // yaw first, then pitch
                >> viewangle.x;
            pMechViewAngle = glm::radians( viewangle );

            Global.pCamera.Angle.x = pMechViewAngle.x;
            Global.pCamera.Angle.y = pMechViewAngle.y;

            parser.getTokens();
            parser >> token;
        }
        if (token == std::string("driver" + std::to_string(cabindex) + "pos:"))
        {
            // pozycja poczatkowa maszynisty
            parser.getTokens(3, false);
            parser
                >> pMechOffset.x
                >> pMechOffset.y
                >> pMechOffset.z;
            pMechSittingPosition = pMechOffset;

            parser.getTokens();
            parser >> token;
        }
        // ABu: pozycja siedzaca mechanika
        if (token == std::string("driver" + std::to_string(cabindex) + "sitpos:"))
        {
            // ABu 180404 pozycja siedzaca maszynisty
            parser.getTokens(3, false);
            parser
                >> pMechSittingPosition.x
                >> pMechSittingPosition.y
                >> pMechSittingPosition.z;

            parser.getTokens();
            parser >> token;
        }
        // else parse=false;
        do {
            if( parse == true ) {
                token = "";
                parser.getTokens();
                parser >> token;
            }
            else {
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
                parser.getTokens();
                parser >> token;
				std::replace(token.begin(), token.end(), '\\', '/');
                if (token != "none")
                {
                    // bieżąca sciezka do tekstur to dynamic/...
                    Global.asCurrentTexturePath = DynamicObject->asBaseDir;
                    // szukaj kabinę jako oddzielny model
                    // name can contain leading slash, erase it to avoid creation of double slashes when the name is combined with current directory
                    replace_slashes( token );
                    erase_leading_slashes( token );
                    if( token[ 0 ] == '/' ) {
                        token.erase( 0, 1 );
                    }
					TModel3d *kabina = TModelsManager::GetModel(DynamicObject->asBaseDir + token, true, true,
					    (Global.network_servers.empty() && !Global.network_client) ? 0 : id());
                    // z powrotem defaultowa sciezka do tekstur
                    Global.asCurrentTexturePath = szTexturePath;
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
/*
            if (nullptr == DynamicObject->mdKabina)
            {
                // don't bother with other parts until the cab is initialised
                continue;
            }
*/
            else if (true == initialize_gauge(parser, token, cabindex))
            {
                // matched the token, grab the next one
                continue;
            }
            else if (true == initialize_button(parser, token, cabindex))
            {
                // matched the token, grab the next one
                continue;
            }
            // TODO: add "pydestination:"
            else if (token == "pyscreen:")
            {
                screen_entry screen;
                screen.deserialize(parser);
                if ((false == screen.script.empty()) && (substr_path(screen.script).empty()))
                {
                    screen.script = DynamicObject->asBaseDir + screen.script;
                }

                opengl_texture *tex = nullptr;
                TSubModel *submodel = nullptr;
                if (screen.target != "none")
                {
                    submodel = (DynamicObject->mdKabina ?
                                    DynamicObject->mdKabina->GetFromName(screen.target) :
                                    DynamicObject->mdLowPolyInt ?
                                    DynamicObject->mdLowPolyInt->GetFromName(screen.target) :
                                    nullptr);
                    if (submodel == nullptr)
                    {
                        WriteLog("Python Screen: submodel " + screen.target +
                                 " not found - Ignoring screen");
                        continue;
                    }
                    auto const material{submodel->GetMaterial()};
                    if (material <= 0)
                    {
                        // sub model nie posiada tekstury lub tekstura wymienna - nie obslugiwana
                        WriteLog("Python Screen: invalid texture id " + std::to_string(material) +
                                 " - Ignoring screen");
                        continue;
                    }

                    tex = &GfxRenderer->Texture(GfxRenderer->Material(material).textures[0]);
                }
                else
                {
                    // TODO: fix leak
                    tex = new opengl_texture();
                    tex->make_stub();
                }

                tex->create(true); // make the surface static so it doesn't get destroyed by garbage
                                   // collector if the user spends long time outside cab
                // TBD, TODO: keep texture handles around, so we can undo the static switch when the
                // user changes cabs?
                auto rt = std::make_shared<python_rt>();
                rt->shared_tex = tex->id;

                // record renderer and material binding for future update requests
                m_screens.emplace_back(screen);
                m_screens.back().rt = rt;

                m_screens.back().touch_list = std::make_shared<std::vector<glm::vec2>>();
                if (submodel)
                    submodel->screen_touch_list = m_screens.back().touch_list;

                if (Global.python_displaywindows)
                    m_screens.back().viewer = std::make_unique<python_screen_viewer>(rt, m_screens.back().touch_list, m_screens.back().script);
            }
            else if (token == "pyscreenupdatetime:") {
                parser.getTokens();
                parser >> ScreenUpdateRate;
            }
            // btLampkaUnknown.Init("unknown",mdKabina,false);
        } while ( ( token != "" )
// TODO: enable full per-cab deserialization when/if .mmd files get proper per-cab switch configuration
//               && ( token != "cab1definition:" )
//               && ( token != "cab2definition:" )
               && ( token != "cab0definition:" ) );
		for (auto &screen : m_screens)
		{
			if (screen.updatetime > 0) {
				screen.updatetime = std::max((int)screen.updatetime, Global.PythonScreenUpdateRate) * 0.001;
			}
			if (screen.updatetime == 0) {
				screen.updatetime = std::max(Global.PythonScreenUpdateRate, ScreenUpdateRate) * 0.001;
			}
			if (screen.updatetime < -1) {
				screen.updatetime = -screen.updatetime * 0.001;
			}
		}
    }
    else
    {
        return false;
    }
/*
    if (DynamicObject->mdKabina)
    {
*/
        // configure placement of sound emitters which aren't bound with any device model, and weren't placed manually
        auto const caboffset { glm::dvec3 { ( Cabine[ cabindex ].CabPos1 + Cabine[ cabindex ].CabPos2 ) * 0.5 } +glm::dvec3 { 0, 1, 0 } };
        // NOTE: since radiosound is an incomplete template not using std::optional it gets a special treatment
        if( m_radiosound.offset() == nullvector ) {
            m_radiosound.offset( btLampkaRadio.model_offset() );
        }
        if( m_radiosound.offset() == nullvector ) {
            m_radiosound.offset( caboffset );
        }
        std::vector<std::pair<std::reference_wrapper<std::optional<sound_source>>, glm::vec3>>
            soundlocations = {
                {dsbReverserKey, ggDirKey.model_offset()},
                {dsbNastawnikJazdy, ggJointCtrl.model_offset()},
                {dsbNastawnikJazdy, ggMainCtrl.model_offset()}, // NOTE: fallback for vehicles without universal controller
                {dsbNastawnikBocz, ggScndCtrl.model_offset()},
                {dsbSwitch, caboffset},
                {dsbPneumaticSwitch, caboffset},
                {rsHiss, ggBrakeCtrl.model_offset()},
                {rsHissU, ggBrakeCtrl.model_offset()},
                {rsHissE, ggBrakeCtrl.model_offset()},
                {rsHissX, ggBrakeCtrl.model_offset()},
                {rsHissT, ggBrakeCtrl.model_offset()},
                {rsSBHiss, ggLocalBrake.model_offset()},
                {rsSBHiss, ggBrakeCtrl.model_offset()}, // NOTE: fallback if the local brake model can't be located
                {rsSBHissU, ggLocalBrake.model_offset()},
                {rsSBHissU, ggBrakeCtrl.model_offset()}, // NOTE: fallback if the local brake model can't be located
                {rsFadeSound, caboffset},
                {rsRunningNoise, caboffset},
                {rsHuntingNoise, caboffset},
                {dsbHasler, caboffset},
                {dsbBuzzer, btLampkaCzuwaka.model_offset()},
                {dsbSlipAlarm, caboffset},
                {m_distancecounterclear, btLampkaCzuwaka.model_offset()},
                {m_rainsound, caboffset},
                {m_radiostop, m_radiosound.offset()},
            };
        for( auto & sound : soundlocations ) {
            if( ( sound.first.get() )
             && ( sound.first.get()->offset() == nullvector ) ) {
                sound.first.get()->offset( sound.second );
            }
        }
        // second pass, in case some items received no positioning due to missing submodels etc
        for( auto & sound : soundlocations ) {
            if( ( sound.first.get() )
             && ( sound.first.get()->offset() == nullvector ) ) {
                sound.first.get()->offset( caboffset );
            }
        }

        if( DynamicObject->mdKabina )
            DynamicObject->mdKabina->Init(); // obrócenie modelu oraz optymalizacja, również zapisanie binarnego

        set_cab_controls( NewCabNo < 0 ? 2 : NewCabNo );
/*
        return true;
    }
    return (token == "none");
*/
    return true;
}

Math3D::vector3 TTrain::MirrorPosition(bool lewe)
{ // zwraca współrzędne widoku kamery z lusterka
    auto const shiftdirection { ( lewe ? -1 : 1 ) * ( iCabn == 2 ? 1 : -1 ) };

    return DynamicObject->mMatrix
        * Math3D::vector3(
            mvOccupied->Dim.W * ( 0.5 * shiftdirection ) + ( 0.2 * shiftdirection ),
            1.5 + Cabine[iCabn].CabPos1.y,
            interpolate( Cabine[ iCabn ].CabPos1.z , Cabine[ iCabn ].CabPos2.z, 0.5 ) );
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
    mvOccupied = mvControlled = ( d ? DynamicObject->MoverParameters : nullptr ); // albo silnikowy w EZT

    if( DynamicObject == nullptr ) { return; }

    mvControlled = DynamicObject->FindPowered()->MoverParameters;
    mvSecond = NULL; // gdyby się nic nie znalazło
    if (mvOccupied->Power > 1.0) // dwuczłonowe lub ukrotnienia, żeby nie szukać każdorazowo
        if (mvOccupied->Couplers[1].Connected ?
                mvOccupied->Couplers[1].AllowedFlag & coupling::control :
                false)
        { // gdy jest człon od sprzęgu 1, a sprzęg łączony
            // warsztatowo (powiedzmy)
            if (mvOccupied->Couplers[1].Connected->Power > 1.0) // ten drugi ma moc
                mvSecond =
                    (TMoverParameters *)mvOccupied->Couplers[1].Connected; // wskaźnik na drugiego
        }
        else if (mvOccupied->Couplers[0].Connected ?
                     mvOccupied->Couplers[0].AllowedFlag & coupling::control :
                     false)
        { // gdy jest człon od sprzęgu 0, a sprzęg łączony
            // warsztatowo (powiedzmy)
            if (mvOccupied->Couplers[0].Connected->Power > 1.0) // ale ten drugi ma moc
                mvSecond =
                    (TMoverParameters *)mvOccupied->Couplers[0].Connected; // wskaźnik na drugiego
        }
    // cache nearest unit equipped with pantographs
    {
        auto *lookup { DynamicObject->FindPantographCarrier() };
        // HACK: set pointer to existing vehicle to avoid error checking all over the place
        mvPantographUnit = (
            lookup != nullptr ?
                lookup->MoverParameters :
                mvControlled );
    }
};

void
TTrain::MoveToVehicle(TDynamicObject *target) {
	// > Ra: to nie może być tak robione, to zbytnia proteza jest
	// indeed, too much hacks...
	// TODO: cleanup
	TTrain *target_train = simulation::Trains.find(target->name());
	if (target_train) {
		// let's try to destroy this TTrain and move to already existing one

		if (!Dynamic()->Mechanik || !Dynamic()->Mechanik->AIControllFlag) {
			// tylko jeśli ręcznie prowadzony
			// jeśli prowadzi AI, to mu nie robimy dywersji!
			Occupied()->CabDeactivisation();
			Occupied()->CabOccupied = 0;
			Occupied()->BrakeLevelSet(Occupied()->Handle->GetPos(bh_NP)); //rozwala sterowanie hamulcem GF 04-2016
            Occupied()->MainCtrlPos = Occupied()->MainCtrlNoPowerPos();
            Occupied()->ScndCtrlPos = 0;
            Dynamic()->MechInside = false;
			Dynamic()->Controller = AIdriver;

			Dynamic()->bDisplayCab = false;
			Dynamic()->ABuSetModelShake( {} );

            if( Dynamic()->Mechanik )
                Dynamic()->Mechanik->MoveTo( target );

			target_train->Occupied()->LimPipePress = target_train->Occupied()->PipePress;
			target_train->Occupied()->CabActivisationAuto( true ); // załączenie rozrządu (wirtualne kabiny)
			target_train->Dynamic()->MechInside = true;
            if( target_train->Dynamic()->Mechanik ) {
                target_train->Dynamic()->Controller = target_train->Dynamic()->Mechanik->AIControllFlag;
                target_train->Dynamic()->Mechanik->DirectionChange();
            }
            else {
                target_train->Dynamic()->Controller = Humandriver;
            }
        } else {
			target_train->Dynamic()->bDisplayCab = false;
			target_train->Dynamic()->ABuSetModelShake( {} );
		}

		target_train->Dynamic()->ABuSetModelShake( {} ); // zerowanie przesunięcia przed powrotem?

		// potentially move player
		if (simulation::Train == this) {
			simulation::Train = target_train;
            // our local driver may potentially be in external view mode, in which case we shouldn't activate cab visualization
            target_train->Dynamic()->bDisplayCab |= !FreeFlyModeFlag;
        }

		// delete this TTrain
		pending_delete = true;
	} else {
		// move this TTrain to other dynamic

		// remove TTrain from global list, we're going to change dynamic anyway
		simulation::Trains.detach(Dynamic()->name());

		if (!Dynamic()->Mechanik || !Dynamic()->Mechanik->AIControllFlag) {
			// tylko jeśli ręcznie prowadzony
			// jeśli prowadzi AI, to mu nie robimy dywersji!

			Occupied()->CabDeactivisation();
			Occupied()->CabOccupied = 0;
			Occupied()->BrakeLevelSet(Occupied()->Handle->GetPos(bh_NP)); //rozwala sterowanie hamulcem GF 04-2016
            Occupied()->MainCtrlPos = Occupied()->MainCtrlNoPowerPos();
            Occupied()->ScndCtrlPos = 0;
            Dynamic()->MechInside = false;
			Dynamic()->Controller = AIdriver;

			Dynamic()->bDisplayCab = false;
			Dynamic()->ABuSetModelShake( {} );

            if( Dynamic()->Mechanik )
                Dynamic()->Mechanik->MoveTo( target );

            DynamicSet( target );

            Dynamic()->MechInside = true;
            if( Dynamic()->Mechanik ) {
                Dynamic()->Controller = Dynamic()->Mechanik->AIControllFlag;
                Dynamic()->Mechanik->DirectionChange();
            }
            else {
                Dynamic()->Controller = Humandriver;
            }

			Occupied()->LimPipePress = Occupied()->PipePress;
			Occupied()->CabActivisationAuto( true ); // załączenie rozrządu (wirtualne kabiny)
		} else {
			Dynamic()->bDisplayCab = false;
			Dynamic()->ABuSetModelShake( {} );

			DynamicSet(target);
		}

        {
            auto const filename { Occupied()->TypeName + ".mmd" };
            LoadMMediaFile( filename );
            InitializeCab(
                Occupied()->CabActive,
                filename );
        }

		Dynamic()->ABuSetModelShake( {} ); // zerowanie przesunięcia przed powrotem?

        if( simulation::Train == this ) {
            // our local driver may potentially be in external view mode, in which case we shouldn't activate cab visualization
            Dynamic()->bDisplayCab |= !FreeFlyModeFlag;
        }

		// add it back with updated dynamic name
		simulation::Trains.insert(this);
	}
}

// checks whether specified point is within boundaries of the active cab
bool
TTrain::point_inside( Math3D::vector3 const Point ) const {

    return ( Point.x >= Cabine[ iCabn ].CabPos1.x )       && ( Point.x <= Cabine[ iCabn ].CabPos2.x )
        && ( Point.y >= Cabine[ iCabn ].CabPos1.y + 0.5 ) && ( Point.y <= Cabine[ iCabn ].CabPos2.y + 1.8 )
        && ( Point.z >= Cabine[ iCabn ].CabPos1.z )       && ( Point.z <= Cabine[ iCabn ].CabPos2.z );
}

Math3D::vector3
TTrain::clamp_inside( Math3D::vector3 const &Point ) const {

    if( DebugModeFlag ) { return Point; }

    return {
        clamp( Point.x, Cabine[ iCabn ].CabPos1.x, Cabine[ iCabn ].CabPos2.x ),
        clamp( Point.y, Cabine[ iCabn ].CabPos1.y + 0.5, Cabine[ iCabn ].CabPos2.y + 1.8 ),
        clamp( Point.z, Cabine[ iCabn ].CabPos1.z, Cabine[ iCabn ].CabPos2.z ) };
}

const TTrain::screenentry_sequence& TTrain::get_screens() {
	update_screens(Timer::GetDeltaTime());
	return m_screens;
}

void
TTrain::radio_message( sound_source *Message, int const Channel ) {

    auto const soundrange { Message->range() };
    if( ( soundrange > 0 )
     && ( glm::length2( Message->location() - glm::dvec3 { DynamicObject->GetPosition() } ) > ( soundrange * soundrange ) ) ) {
        // skip message playback if the receiver is outside of the emitter's range
        return;
    }
    // NOTE: we initiate playback of all sounds in range, in case the user switches on the radio or tunes to the right channel mid-play
    m_radiomessages.emplace_back(
        Channel,
        std::make_shared<sound_source>( m_radiosound ) );
    // assign sound to the template and play it
    auto &message = *( m_radiomessages.back().second.get() );
    auto const radioenabled { ( true == mvOccupied->Radio ) && ( mvOccupied->Power24vIsAvailable || mvOccupied->Power110vIsAvailable ) };
    auto const volume {
        ( true == radioenabled )
     && ( Dynamic()->Mechanik != nullptr )
     && ( Channel == RadioChannel() ) ?
            1.0 :
            0.0 };
    message
        .copy_sounds( *Message )
        .gain( volume )
        .play();
}

// clears state of all cabin controls
void TTrain::clear_cab_controls()
{
    // indicators exposed to custom control devices
    btLampkaSHP.Clear(0);
    btLampkaCzuwaka.Clear(1);
    btLampkaOpory.Clear(2);
    btLampkaWylSzybki.Clear(3);
    btLampkaNadmSil.Clear(4);
    btLampkaStyczn.Clear(5);
    btLampkaPoslizg.Clear(6);
    btLampkaNadmPrzetw.Clear(((mvControlled->TrainType & dt_EZT) != 0) ? -1 : 7); // EN57 nie ma tej lampki
    btLampkaPrzetwOff.Clear(((mvControlled->TrainType & dt_EZT) != 0) ? 7 : -1 ); // za to ma tę
    btLampkaNadmSpr.Clear(8);
    btLampkaNadmWent.Clear(9);
    btLampkaWysRozr.Clear(((mvControlled->TrainType & dt_ET22) != 0) ? -1 : 10); // ET22 nie ma tej lampki
    btLampkaOgrzewanieSkladu.Clear(11);
    btHaslerBrakes.Clear(12); // ciśnienie w cylindrach do odbijania na haslerze
    btHaslerCurrent.Clear(13); // prąd na silnikach do odbijania na haslerze
    // Numer 14 jest używany dla buczka SHP w update_sounds()
    // Jeśli ustawiamy nową wartość dla PoKeys wolna jest 15

    // other cab controls
    // TODO: arrange in more readable manner, and eventually refactor
    ggJointCtrl.Clear();
    ggMainCtrl.Clear();
    ggMainCtrlAct.Clear();
    ggScndCtrl.Clear();
    ggScndCtrlButton.Clear();
    ggScndCtrlOffButton.Clear();
    ggDistanceCounterButton.Clear();
    ggDirKey.Clear();
    ggDirForwardButton.Clear();
    ggDirNeutralButton.Clear();
    ggDirBackwardButton.Clear();
    ggBrakeCtrl.Clear();
    ggLocalBrake.Clear();
    ggAlarmChain.Clear();
    ggBrakeProfileCtrl.Clear();
    ggBrakeProfileG.Clear();
    ggBrakeProfileR.Clear();
	ggBrakeOperationModeCtrl.Clear();
    ggMaxCurrentCtrl.Clear();
    ggMainOffButton.Clear();
    ggMainOnButton.Clear();
    ggSecurityResetButton.Clear();
    ggSHPResetButton.Clear();
    ggReleaserButton.Clear();
	ggSpringBrakeOnButton.Clear();
	ggSpringBrakeOffButton.Clear();
	ggUniveralBrakeButton1.Clear();
	ggUniveralBrakeButton2.Clear();
	ggUniveralBrakeButton3.Clear();
    ggEPFuseButton.Clear();
    ggSandButton.Clear();
    ggAutoSandButton.Clear();
    ggAntiSlipButton.Clear();
    ggHornButton.Clear();
    ggHornLowButton.Clear();
    ggHornHighButton.Clear();
    ggWhistleButton.Clear();
	ggHelperButton.Clear();
    ggNextCurrentButton.Clear();
	ggSpeedControlIncreaseButton.Clear();
	ggSpeedControlDecreaseButton.Clear();
	ggSpeedControlPowerIncreaseButton.Clear();
	ggSpeedControlPowerDecreaseButton.Clear();
	for (auto &speedctrlbutton : ggSpeedCtrlButtons) {
		speedctrlbutton.Clear();
	}
    for( auto &universal : ggUniversals ) {
        universal.Clear();
    }
	for (auto &item : ggInverterEnableButtons) {
		item.Clear();
	}
	for (auto &item : ggInverterDisableButtons) {
		item.Clear();
	}
	for (auto &item : ggInverterToggleButtons) {
		item.Clear();
	}
    for( auto &relayresetbutton : ggRelayResetButtons ) {
        relayresetbutton.Clear();
    }
    ggInstrumentLightButton.Clear();
    ggDashboardLightButton.Clear();
    ggTimetableLightButton.Clear();
    // hunter-091012
    ggCabLightDimButton.Clear();
    ggCompartmentLightsButton.Clear();
    ggCompartmentLightsOnButton.Clear();
    ggCompartmentLightsOffButton.Clear();
    ggBatteryButton.Clear();
    ggBatteryOnButton.Clear();
    ggBatteryOffButton.Clear();
	ggCabActivationButton.Clear();
    //-------
    ggFuseButton.Clear();
    ggConverterFuseButton.Clear();
    ggStLinOffButton.Clear();
    ggRadioChannelSelector.Clear();
    ggRadioChannelPrevious.Clear();
    ggRadioChannelNext.Clear();
    ggRadioStop.Clear();
    ggRadioTest.Clear();
    ggRadioCall3.Clear();
	ggRadioVolumeSelector.Clear();
	ggRadioVolumePrevious.Clear();
	ggRadioVolumeNext.Clear();
    ggDoorLeftPermitButton.Clear();
    ggDoorRightPermitButton.Clear();
    ggDoorPermitPresetButton.Clear();
    ggDoorLeftButton.Clear();
    ggDoorRightButton.Clear();
    ggDoorLeftOnButton.Clear();
    ggDoorRightOnButton.Clear();
    ggDoorLeftOffButton.Clear();
    ggDoorRightOffButton.Clear();
    ggDoorAllOnButton.Clear();
    ggDoorAllOffButton.Clear();
    ggTrainHeatingButton.Clear();
    ggSignallingButton.Clear();
    ggDoorSignallingButton.Clear();
    ggDoorStepButton.Clear();
    ggDepartureSignalButton.Clear();
    ggCompressorButton.Clear();
    ggCompressorLocalButton.Clear();
    ggConverterButton.Clear();
    ggConverterOffButton.Clear();
    ggConverterLocalButton.Clear();
    ggMainButton.Clear();
/*
    ggPantFrontButton.Clear();
    ggPantRearButton.Clear();
    ggPantFrontButtonOff.Clear();
    ggPantRearButtonOff.Clear();
*/
    ggPantAllDownButton.Clear();
    ggPantSelectedButton.Clear();
    ggPantSelectedDownButton.Clear();
    ggPantValvesButton.Clear();
    ggPantCompressorButton.Clear();
    ggPantCompressorValve.Clear();
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
    ggMotorBlowersFrontButton.Clear();
    ggMotorBlowersRearButton.Clear();
    ggMotorBlowersAllOffButton.Clear();

    btLampkaPrzetw.Clear();
    btLampkaPrzetwB.Clear();
    btLampkaPrzetwBOff.Clear();
    btLampkaPrzekRozn.Clear();
    btLampkaPrzekRoznPom.Clear();
    btLampkaUkrotnienie.Clear();
    btLampkaHamPosp.Clear();
    btLampkaWylSzybkiOff.Clear();
    btLampkaWylSzybkiB.Clear();
    btLampkaWylSzybkiBOff.Clear();
    btLampkaMainBreakerReady.Clear();
    btLampkaMainBreakerBlinkingIfReady.Clear();
    btLampkaBezoporowa.Clear();
    btLampkaBezoporowaB.Clear();
    btLampkaMaxSila.Clear();
    btLampkaPrzekrMaxSila.Clear();
    btLampkaRadio.Clear();
	btLampkaRadioMessage.Clear();
    btLampkaRadioStop.Clear();
    btLampkaHamulecReczny.Clear();
    btLampkaBlokadaDrzwi.Clear();
    btLampkaDoorLockOff.Clear();
    for( auto &universal : btUniversals ) {
        universal.Clear();
    }
    btInstrumentLight.Clear();
    btDashboardLight.Clear();
    btTimetableLight.Clear();
    btLampkaWentZaluzje.Clear();
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
    btLampkaED.Clear();
    btLampkaBrakeProfileG.Clear();
    btLampkaBrakeProfileP.Clear();
    btLampkaBrakeProfileR.Clear();
	btLampkaSpringBrakeActive.Clear();
	btLampkaSpringBrakeInactive.Clear();
    btLampkaSprezarka.Clear();
    btLampkaSprezarkaB.Clear();
    btLampkaSprezarkaOff.Clear();
    btLampkaSprezarkaBOff.Clear();
    btLampkaFuelPumpOff.Clear();
    btLampkaNapNastHam.Clear();
    btLampkaOporyB.Clear();
    btLampkaStycznB.Clear();
    btLampkaHamowanie1zes.Clear();
    btLampkaHamowanie2zes.Clear();
    btLampkaNadmPrzetwB.Clear();
    btLampkaHVoltageB.Clear();
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
    // others
    btLampkaMalfunction.Clear();
    btLampkaMalfunctionB.Clear();
    btLampkaMotorBlowers.Clear();
    btLampkaCoolingFans.Clear();
    btLampkaTempomat.Clear();
    btLampkaDistanceCounter.Clear();

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
void TTrain::set_cab_controls( int const Cab ) {
    // switches
    // battery
    ggBatteryButton.PutValue(
        ( ggBatteryButton.type() == TGaugeType::push ? 0.5f :
          mvOccupied->Power24vIsAvailable ? 1.f :
          0.f ) );
	// activation
	ggCabActivationButton.PutValue(
		(ggCabActivationButton.type() == TGaugeType::push ? 0.5f :
			mvOccupied->IsCabMaster() ? 1.f :
			0.f));
    // line breaker
    if( ggMainButton.SubModel != nullptr ) { // instead of single main button there can be on/off pair
        ggMainButton.PutValue(
            ( ggMainButton.type() == TGaugeType::push ? 0.5f :
                m_linebreakerstate > 0 ? 1.f :
                0.f ) );
    }
    // motor connectors
    ggStLinOffButton.PutValue(
        ( mvControlled->StLinSwitchOff ?
            1.f :
            0.f ) );
    // radio
    ggRadioChannelSelector.PutValue( ( Dynamic()->Mechanik ? Dynamic()->Mechanik->iRadioChannel : 1 ) - 1 );
    // pantographs
/*
    if( mvOccupied->PantSwitchType != "impulse" ) {
        if( ggPantFrontButton.SubModel ) {
            ggPantFrontButton.PutValue(
                ( mvControlled->Pantographs[end::front].valve.is_enabled ?
                    1.f :
                    0.f ) );
        }
        if( ggPantFrontButtonOff.SubModel ) {
            ggPantFrontButtonOff.PutValue(
                ( mvControlled->Pantographs[end::front].valve.is_disabled ?
                    1.f :
                    0.f ) );
        }
    }
    if( mvOccupied->PantSwitchType != "impulse" ) {
        if( ggPantRearButton.SubModel ) {
            ggPantRearButton.PutValue(
                ( mvControlled->Pantographs[end::rear].valve.is_enabled ?
                    1.f :
                    0.f ) );
        }
        if( ggPantRearButtonOff.SubModel ) {
            ggPantRearButtonOff.PutValue(
                ( mvControlled->Pantographs[end::rear].valve.is_disabled ?
                    1.f :
                    0.f ) );
        }
    }
*/
    // front/end pantograph selection is relative to occupied cab
    if( ggPantSelectedButton.type() == TGaugeType::toggle ) {
        ggPantSelectedButton.PutValue(
            ( mvPantographUnit->PantsValve.is_enabled ?
                1.f :
                0.f ) );
    }
    else {
        if( false == m_controlmapper.contains( "pantselectedoff_sw:" ) ) {
            // single impulse switch arrangement, with neutral position mid-way
            ggPantSelectedButton.PutValue( 0.5f );
        }
    }
    if( ggPantSelectedDownButton.type() == TGaugeType::toggle ) {
        ggPantSelectedDownButton.PutValue(
            ( mvPantographUnit->PantsValve.is_disabled ?
                1.f :
                0.f ) );
    }
    ggPantValvesButton.PutValue( 0.5f );
    // auxiliary compressor
    ggPantCompressorValve.PutValue(
        mvControlled->bPantKurek3 ?
            0.f : // default setting is pantographs connected with primary tank
            1.f );
    ggPantCompressorButton.PutValue(
        mvPantographUnit->PantCompFlag ?
            1.f :
            0.f );
    // converter
    if( mvOccupied->ConvSwitchType != "impulse" ) {
        ggConverterButton.PutValue(
            mvControlled->ConverterAllow ?
                1.f :
                0.f );
    }
    ggConverterLocalButton.PutValue(
        mvControlled->ConverterAllowLocal ?
            1.f :
            0.f );
    // compressor
    ggCompressorButton.PutValue(
        mvControlled->CompressorAllow ?
            1.f :
            0.f );
    ggCompressorLocalButton.PutValue(
        mvControlled->CompressorAllowLocal ?
            1.f :
            0.f );
	ggCompressorListButton.PutValue(mvOccupied->CompressorListPos - 1);
    // motor overload relay threshold / shunt mode
    ggMaxCurrentCtrl.PutValue(
        ( true == mvControlled->ShuntModeAllow ?
            ( true == mvControlled->ShuntMode ?
                1.f :
                0.f ) :
            ( mvControlled->MotorOverloadRelayHighThreshold ?
                1.f :
                0.f ) ) );
    // lights
    ggLightsButton.PutValue( mvOccupied->LightsPos - 1 );

    auto const vehicleend { cab_to_end( Cab ) };

    if( ( mvOccupied->iLights[ vehicleend ] & light::headlight_left ) != 0 ) {
        ggLeftLightButton.PutValue( 1.f );
    }
    if( ( mvOccupied->iLights[ vehicleend ] & light::headlight_right ) != 0 ) {
        ggRightLightButton.PutValue( 1.f );
    }
    if( ( mvOccupied->iLights[ vehicleend ] & light::headlight_upper ) != 0 ) {
        ggUpperLightButton.PutValue( 1.f );
    }
    if( ( mvOccupied->iLights[ vehicleend ] & light::redmarker_left ) != 0 ) {
        if( ggLeftEndLightButton.SubModel != nullptr ) {
            ggLeftEndLightButton.PutValue( 1.f );
        }
        else {
            ggLeftLightButton.PutValue( -1.f );
        }
    }
    if( ( mvOccupied->iLights[ vehicleend ] & light::redmarker_right ) != 0 ) {
        if( ggRightEndLightButton.SubModel != nullptr ) {
            ggRightEndLightButton.PutValue( 1.f );
        }
        else {
            ggRightLightButton.PutValue( -1.f );
        }
    }
    if( true == DynamicObject->DimHeadlights ) {
        ggDimHeadlightsButton.PutValue( 1.f );
    }
    // cab lights
    if( true == Cabine[Cab].bLightDim ) {
        ggCabLightDimButton.PutValue( 1.f );
    }
    // compartment lights
    ggCompartmentLightsButton.PutValue(
        ( ggCompartmentLightsButton.type() == TGaugeType::push ? 0.5f :
          mvOccupied->CompartmentLights.is_enabled ? 1.f :
          0.f ) );
    // instrument lights
    ggInstrumentLightButton.PutValue( (
        InstrumentLightActive ?
            1.f :
            0.f ) );
    ggDashboardLightButton.PutValue( (
        DashboardLightActive ?
            1.f :
            0.f ) );
    ggTimetableLightButton.PutValue( (
        TimetableLightActive ?
            1.f :
            0.f ) );
    // doors permits
    if( false == ggDoorLeftPermitButton.is_push() ) {
        ggDoorLeftPermitButton.PutValue( mvOccupied->Doors.instances[ ( cab_to_end() == end::front ? side::left : side::right ) ].open_permit ? 1.f : 0.f );
    }
    if( false == ggDoorRightPermitButton.is_push() ) {
        ggDoorRightPermitButton.PutValue( mvOccupied->Doors.instances[ ( cab_to_end() == end::front ? side::right : side::left ) ].open_permit ? 1.f : 0.f );
    }
    ggDoorPermitPresetButton.PutValue( mvOccupied->Doors.permit_preset );
    // door controls
    ggDoorLeftButton.PutValue( mvOccupied->Doors.instances[ ( cab_to_end() == end::front ? side::left : side::right ) ].is_closed ? 0.f : 1.f );
    ggDoorRightButton.PutValue( mvOccupied->Doors.instances[ ( cab_to_end() == end::front ? side::right : side::left ) ].is_closed ? 0.f : 1.f );
    // door lock
    ggDoorSignallingButton.PutValue(
        mvOccupied->Doors.lock_enabled ?
            1.f :
            0.f );
    // door step
    if( false == ggDoorStepButton.is_push() ) {
        ggDoorStepButton.PutValue(
            mvOccupied->Doors.step_enabled ?
                1.f :
                0.f );
    }
    // heating
    if( false == ggTrainHeatingButton.is_push() ) {
        ggTrainHeatingButton.PutValue(
            mvControlled->Heating ?
                1.f :
                0.f );
    }
    // brake acting time
    if( ggBrakeProfileCtrl.SubModel != nullptr ) {
        ggBrakeProfileCtrl.PutValue(
            ( ( mvOccupied->BrakeDelayFlag & bdelay_R ) != 0 ?
                2.f :
                mvOccupied->BrakeDelayFlag - 1 ) );
    }
    if( ggBrakeProfileG.SubModel != nullptr ) {
        ggBrakeProfileG.PutValue(
            mvOccupied->BrakeDelayFlag == bdelay_G ?
                1.f :
                0.f );
    }
    if( ggBrakeProfileR.SubModel != nullptr ) {
        ggBrakeProfileR.PutValue(
            ( mvOccupied->BrakeDelayFlag & bdelay_R ) != 0 ?
                1.f :
                0.f );
    }
	if (ggBrakeOperationModeCtrl.SubModel != nullptr) {
		ggBrakeOperationModeCtrl.PutValue(
			(mvOccupied->BrakeOpModeFlag > 0 ?
				std::log2(mvOccupied->BrakeOpModeFlag) :
				0));
	}
    // alarm chain
    ggAlarmChain.PutValue(
        mvControlled->AlarmChainFlag ?
            1.f :
            0.f );
    // brake signalling
    ggSignallingButton.PutValue(
        mvControlled->Signalling ?
            1.f :
            0.f );
    // multiple-unit current indicator source
    ggNextCurrentButton.PutValue(
        ShowNextCurrent ?
            1.f :
            0.f );
    // water pump
    ggWaterPumpBreakerButton.PutValue(
        mvControlled->WaterPump.breaker ?
            1.f :
            0.f );
    if( ggWaterPumpButton.type() != TGaugeType::push ) {
        ggWaterPumpButton.PutValue(
            mvControlled->WaterPump.is_enabled ?
                1.f :
                0.f );
    }
    // water heater
    ggWaterHeaterBreakerButton.PutValue(
        mvControlled->WaterHeater.breaker ?
            1.f :
            0.f );
    ggWaterHeaterButton.PutValue(
        mvControlled->WaterHeater.is_enabled ?
            1.f :
            0.f );
    ggWaterCircuitsLinkButton.PutValue(
        mvControlled->WaterCircuitsLink ?
            1.f :
            0.f );
    // fuel pump
    if( ggFuelPumpButton.type() != TGaugeType::push ) {
        ggFuelPumpButton.PutValue(
            mvControlled->FuelPump.is_enabled ?
                1.f :
                0.f );
    }
    // oil pump
    if( ggOilPumpButton.type() != TGaugeType::push ) {
        ggOilPumpButton.PutValue(
            mvControlled->OilPump.is_enabled ?
            1.f :
            0.f );
    }
    // traction motor fans
    if( ggMotorBlowersFrontButton.type() != TGaugeType::push ) {
        ggMotorBlowersFrontButton.PutValue(
            mvControlled->MotorBlowers[end::front].is_enabled ?
            1.f :
            0.f );
    }
    if( ggMotorBlowersRearButton.type() != TGaugeType::push ) {
        ggMotorBlowersRearButton.PutValue(
            mvControlled->MotorBlowers[end::rear].is_enabled ?
            1.f :
            0.f );
    }
    if( ggMotorBlowersAllOffButton.type() != TGaugeType::push ) {
        ggMotorBlowersAllOffButton.PutValue(
            ( mvControlled->MotorBlowers[end::front].is_disabled
		   || mvControlled->MotorBlowers[end::rear].is_disabled ) ?
                1.f :
                0.f );
    }
    // second controller
    if( ggScndCtrl.is_push() ) {
        ggScndCtrl.PutValue(
            ggScndCtrl.is_toggle() ?
                0.5f : // pushtoggle is two-way control with neutral position in the middle
                0.f ); // push is on/off control, active while held down, due to legacy use
    }
    // tempomat
    if( false == ggScndCtrlButton.is_push() ) {
        ggScndCtrlButton.PutValue(
            ( mvControlled->ScndCtrlPos > 0 ) ?
                1.f :
                0.f );
    }
    // sandbox
    if( ggAutoSandButton.type() != TGaugeType::push ) {
        ggAutoSandButton.PutValue(
            mvControlled->SandDoseAutoAllow ?
                1.f :
                0.f );
     }
    // radio
    ggRadioVolumeSelector.PutValue( Global.RadioVolume );

	//finding each inverter - not so optimal, but action ins performed only during changing cabin
	bool kier = (DynamicObject->DirectionGet() * mvOccupied->CabOccupied > 0);
	int flag = DynamicObject->MoverParameters->InverterControlCouplerFlag;
	int itemstart = 0;
	for (auto &item : ggInverterToggleButtons) //for each button
	{
		int itemindex = itemstart;
		itemstart++;
		TDynamicObject *p = DynamicObject->GetFirstDynamic(mvOccupied->CabOccupied < 0 ? end::rear : end::front, flag);
		while (p)
		{
			if (p->MoverParameters->eimc[eimc_p_Pmax] > 1)
			{
				if (itemindex < p->MoverParameters->InvertersNo)
				{
					// visual feedback
					ggInverterToggleButtons[itemstart-1].PutValue(p->MoverParameters->Inverters[itemindex].Activate ? 1.0 : 0.0);
					break;
				}
				else
				{
					itemindex -= p->MoverParameters->InvertersNo;
				}

			}
			p = (kier ? p->Next(flag) : p->Prev(flag));
		}
	}

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
		{ "i-radiomessage:", btLampkaRadioMessage },
        { "i-radiostop:", btLampkaRadioStop },
        { "i-manual_brake:", btLampkaHamulecReczny },
        { "i-door_blocked:", btLampkaBlokadaDrzwi },
        { "i-door_blockedoff:", btLampkaDoorLockOff },
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
        { "i-mainbreakerready:", btLampkaMainBreakerReady },
        { "i-mainbreakerblinking:", btLampkaMainBreakerBlinkingIfReady },
        { "i-vent_ovld:", btLampkaNadmWent },
        { "i-comp_ovld:", btLampkaNadmSpr },
        { "i-resistors:", btLampkaOpory },
        { "i-no_resistors:", btLampkaBezoporowa },
        { "i-no_resistors_b:", btLampkaBezoporowaB },
        { "i-highcurrent:", btLampkaWysRozr },
        { "i-vent_trim:", btLampkaWentZaluzje },
        { "i-motorblowers:", btLampkaMotorBlowers },
        { "i-coolingfans:", btLampkaCoolingFans },
        { "i-tempomat:", btLampkaTempomat },
        { "i-distancecounter:", btLampkaDistanceCounter },
        { "i-trainheating:", btLampkaOgrzewanieSkladu },
        { "i-security_aware:", btLampkaCzuwaka },
        { "i-security_cabsignal:", btLampkaSHP },
		{ "i-security_aware_cabsignal:", btLampkaCzuwakaSHP },
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
        { "i-brakeprofileg:", btLampkaBrakeProfileG },
        { "i-brakeprofilep:", btLampkaBrakeProfileP },
        { "i-brakeprofiler:", btLampkaBrakeProfileR },
		{ "i-springbrakeactive:", btLampkaSpringBrakeActive },
		{ "i-springbrakeinactive:", btLampkaSpringBrakeInactive },
        { "i-braking-ezt:", btLampkaHamowanie1zes },
        { "i-braking-ezt2:", btLampkaHamowanie2zes },
        { "i-compressor:", btLampkaSprezarka },
        { "i-compressorb:", btLampkaSprezarkaB },
        { "i-compressoroff:", btLampkaSprezarkaOff },
        { "i-compressorboff:", btLampkaSprezarkaBOff },
        { "i-fuelpumpoff:", btLampkaFuelPumpOff },
        { "i-voltbrake:", btLampkaNapNastHam },
        { "i-resistorsb:", btLampkaOporyB },
        { "i-contactorsb:", btLampkaStycznB },
        { "i-conv_ovldb:", btLampkaNadmPrzetwB },
        { "i-hvoltageb:", btLampkaHVoltageB },
        { "i-malfunction:", btLampkaMalfunction },
        { "i-malfunctionb:", btLampkaMalfunctionB },
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
        { "i-dashboardlight:",  btDashboardLight },
        { "i-timetablelight:",  btTimetableLight },
        { "i-universal0:", btUniversals[ 0 ] },
        { "i-universal1:", btUniversals[ 1 ] },
        { "i-universal2:", btUniversals[ 2 ] },
        { "i-universal3:", btUniversals[ 3 ] },
        { "i-universal4:", btUniversals[ 4 ] },
        { "i-universal5:", btUniversals[ 5 ] },
        { "i-universal6:", btUniversals[ 6 ] },
        { "i-universal7:", btUniversals[ 7 ] },
        { "i-universal8:", btUniversals[ 8 ] },
        { "i-universal9:", btUniversals[ 9 ] }
    };
    {
        auto lookup = lights.find( Label );
        if( lookup != lights.end() ) {
            lookup->second.Load( Parser, DynamicObject );
            return true;
        }
    }
    // TODO: move viable dedicated lights to the automatic light array
    std::unordered_map<std::string, bool const *> const autolights = {
        { "i-doors:", &m_doors },
        { "i-doorpermit_left:",  &m_doorspermitleft },
        { "i-doorpermit_right:", &m_doorspermitright },
        { "i-doorpermit_any:", &m_doorpermits },
        { "i-doorstep:", &mvOccupied->Doors.step_enabled },
        { "i-mainpipelock:", &mvOccupied->LockPipe },
        { "i-battery:", &mvOccupied->Power24vIsAvailable },
        { "i-cablight:", &Cabine[ iCabn ].bLight },
    };
    {
        auto lookup = autolights.find( Label );
        if( lookup != autolights.end() ) {
            auto &button = Cabine[ Cabindex ].Button( -1 ); // pierwsza wolna lampka
            button.Load( Parser, DynamicObject );
            button.AssignBool( lookup->second );
            return true;
        }
    }
    // custom lights
    if( Label == "i-instrumentlight:" ) {
        btInstrumentLight.Load( Parser, DynamicObject );
        InstrumentLightType = 0;
    }
    else if( Label == "i-instrumentlight_m:" ) {
        btInstrumentLight.Load( Parser, DynamicObject );
        InstrumentLightType = 1;
    }
    else if( Label == "i-instrumentlight_c:" ) {
        btInstrumentLight.Load( Parser, DynamicObject );
        InstrumentLightType = 2;
    }
    else if( Label == "i-instrumentlight_a:" ) {
        btInstrumentLight.Load( Parser, DynamicObject );
        InstrumentLightType = 3;
    }
    else if( Label == "i-instrumentlight_l:" ) {
        btInstrumentLight.Load( Parser, DynamicObject );
        InstrumentLightType = 4;
    }
    else if (Label == "i-doors:")
    {
        int i = Parser.getToken<int>() - 1;
        auto &button = Cabine[Cabindex].Button(-1); // pierwsza wolna lampka
        button.Load(Parser, DynamicObject);
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
        { "jointctrl:", ggJointCtrl },
        { "mainctrl:", ggMainCtrl },
        { "scndctrl:", ggScndCtrl },
        { "dirkey:" , ggDirKey },
        { "brakectrl:", ggBrakeCtrl },
        { "localbrake:", ggLocalBrake },
        { "alarmchain:", ggAlarmChain },
        { "brakeprofile_sw:", ggBrakeProfileCtrl },
        { "brakeprofileg_sw:", ggBrakeProfileG },
        { "brakeprofiler_sw:", ggBrakeProfileR },
		{ "brakeopmode_sw:", ggBrakeOperationModeCtrl },
        { "maxcurrent_sw:", ggMaxCurrentCtrl },
        { "main_off_bt:", ggMainOffButton },
        { "main_on_bt:", ggMainOnButton },
        { "security_reset_bt:", ggSecurityResetButton },
        { "shp_reset_bt:", ggSHPResetButton },
        { "releaser_bt:", ggReleaserButton },
		{ "springbrakeon_bt:", ggSpringBrakeOnButton },
		{ "springbrakeoff_bt:", ggSpringBrakeOffButton },
		{ "universalbrake1_bt:", ggUniveralBrakeButton1 },
		{ "universalbrake2_bt:", ggUniveralBrakeButton2 },
		{ "universalbrake3_bt:", ggUniveralBrakeButton3 },
		{ "epbrake_bt:", ggEPFuseButton },
        { "sand_bt:", ggSandButton },
		{ "autosandallow_sw:", ggAutoSandButton },
        { "antislip_bt:", ggAntiSlipButton },
        { "horn_bt:", ggHornButton },
        { "hornlow_bt:", ggHornLowButton },
        { "hornhigh_bt:", ggHornHighButton },
        { "whistle_bt:", ggWhistleButton },
		{ "helper_bt:", ggHelperButton },
        { "fuse_bt:", ggFuseButton },
        { "converterfuse_bt:", ggConverterFuseButton },
        { "stlinoff_bt:", ggStLinOffButton },
        { "doorpermitpreset_sw:", ggDoorPermitPresetButton },
        { "door_left_sw:", ggDoorLeftButton },
        { "door_right_sw:", ggDoorRightButton },
        { "doorlefton_sw:", ggDoorLeftOnButton },
        { "doorrighton_sw:", ggDoorRightOnButton },
        { "doorleftoff_sw:", ggDoorLeftOffButton },
        { "doorrightoff_sw:", ggDoorRightOffButton },
        { "doorallon_sw:", ggDoorAllOnButton },
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
		{ "compressorlist_sw:", ggCompressorListButton },
        { "converter_sw:", ggConverterButton },
        { "converterlocal_sw:", ggConverterLocalButton },
        { "converteroff_sw:", ggConverterOffButton },
        { "main_sw:", ggMainButton },
        { "waterpumpbreaker_sw:", ggWaterPumpBreakerButton },
        { "waterpump_sw:", ggWaterPumpButton },
        { "waterheaterbreaker_sw:", ggWaterHeaterBreakerButton },
        { "waterheater_sw:", ggWaterHeaterButton },
        { "water1tempb:", ggWater1TempB },
        { "watercircuitslink_sw:", ggWaterCircuitsLinkButton },
        { "fuelpump_sw:", ggFuelPumpButton },
        { "oilpump_sw:", ggOilPumpButton },
        { "oilpressb:", ggOilPressB },
        { "motorblowersfront_sw:", ggMotorBlowersFrontButton },
        { "motorblowersrear_sw:", ggMotorBlowersRearButton },
        { "motorblowersalloff_sw:", ggMotorBlowersAllOffButton },
        { "radiochannel_sw:", ggRadioChannelSelector },
        { "radiochannelprev_sw:", ggRadioChannelPrevious },
        { "radiochannelnext_sw:", ggRadioChannelNext },
        { "radiostop_sw:", ggRadioStop },
        { "radiotest_sw:", ggRadioTest },
        { "radiocall3_sw:", ggRadioCall3 },
		{ "radiovolume_sw:", ggRadioVolumeSelector },
		{ "radiovolumeprev_sw:", ggRadioVolumePrevious },
		{ "radiovolumenext_sw:", ggRadioVolumeNext },
/*
        { "pantfront_sw:", ggPantFrontButton },
        { "pantrear_sw:", ggPantRearButton },
        { "pantfrontoff_sw:", ggPantFrontButtonOff },
        { "pantrearoff_sw:", ggPantRearButtonOff },
*/
        { "pantalloff_sw:", ggPantAllDownButton },
        { "pantselected_sw:", ggPantSelectedButton },
        { "pantselectedoff_sw:", ggPantSelectedDownButton },
        { "pantvalves_sw:", ggPantValvesButton },
        { "pantcompressor_sw:", ggPantCompressorButton },
        { "pantcompressorvalve_sw:", ggPantCompressorValve },
        { "trainheating_sw:", ggTrainHeatingButton },
        { "signalling_sw:", ggSignallingButton },
        { "door_signalling_sw:", ggDoorSignallingButton },
        { "nextcurrent_sw:", ggNextCurrentButton },
        { "instrumentlight_sw:", ggInstrumentLightButton },
        { "dashboardlight_sw:", ggDashboardLightButton },
        { "timetablelight_sw:", ggTimetableLightButton },
        { "cablightdim_sw:", ggCabLightDimButton },
        { "compartmentlights_sw:", ggCompartmentLightsButton },
        { "compartmentlightson_sw:", ggCompartmentLightsOnButton },
        { "compartmentlightsoff_sw:", ggCompartmentLightsOffButton },
        { "battery_sw:", ggBatteryButton },
        { "batteryon_sw:", ggBatteryOnButton },
        { "batteryoff_sw:", ggBatteryOffButton },
		{ "cabactivation_sw:", ggCabActivationButton },
        { "distancecounter_sw:", ggDistanceCounterButton },
        { "relayreset1_bt:", ggRelayResetButtons[ 0 ] },
        { "relayreset2_bt:", ggRelayResetButtons[ 1 ] },
        { "relayreset3_bt:", ggRelayResetButtons[ 2 ] },
        { "universal0:", ggUniversals[ 0 ] },
        { "universal1:", ggUniversals[ 1 ] },
        { "universal2:", ggUniversals[ 2 ] },
        { "universal3:", ggUniversals[ 3 ] },
        { "universal4:", ggUniversals[ 4 ] },
        { "universal5:", ggUniversals[ 5 ] },
        { "universal6:", ggUniversals[ 6 ] },
        { "universal7:", ggUniversals[ 7 ] },
        { "universal8:", ggUniversals[ 8 ] },
        { "universal9:", ggUniversals[ 9 ] },
		{ "inverterenable1_bt:", ggInverterEnableButtons[0] },
		{ "inverterenable2_bt:", ggInverterEnableButtons[1] },
		{ "inverterenable3_bt:", ggInverterEnableButtons[2] },
		{ "inverterenable4_bt:", ggInverterEnableButtons[3] },
		{ "inverterenable5_bt:", ggInverterEnableButtons[4] },
		{ "inverterenable6_bt:", ggInverterEnableButtons[5] },
		{ "inverterenable7_bt:", ggInverterEnableButtons[6] },
		{ "inverterenable8_bt:", ggInverterEnableButtons[7] },
		{ "inverterenable9_bt:", ggInverterEnableButtons[8] },
		{ "inverterenable10_bt:", ggInverterEnableButtons[9] },
		{ "inverterenable11_bt:", ggInverterEnableButtons[10] },
		{ "inverterenable12_bt:", ggInverterEnableButtons[11] },
		{ "inverterdisable1_bt:", ggInverterDisableButtons[0] },
		{ "inverterdisable2_bt:", ggInverterDisableButtons[1] },
		{ "inverterdisable3_bt:", ggInverterDisableButtons[2] },
		{ "inverterdisable4_bt:", ggInverterDisableButtons[3] },
		{ "inverterdisable5_bt:", ggInverterDisableButtons[4] },
		{ "inverterdisable6_bt:", ggInverterDisableButtons[5] },
		{ "inverterdisable7_bt:", ggInverterDisableButtons[6] },
		{ "inverterdisable8_bt:", ggInverterDisableButtons[7] },
		{ "inverterdisable9_bt:", ggInverterDisableButtons[8] },
		{ "inverterdisable10_bt:", ggInverterDisableButtons[9] },
		{ "inverterdisable11_bt:", ggInverterDisableButtons[10] },
		{ "inverterdisable12_bt:", ggInverterDisableButtons[11] },
		{ "invertertoggle1_bt:", ggInverterToggleButtons[0] },
		{ "invertertoggle2_bt:", ggInverterToggleButtons[1] },
		{ "invertertoggle3_bt:", ggInverterToggleButtons[2] },
		{ "invertertoggle4_bt:", ggInverterToggleButtons[3] },
		{ "invertertoggle5_bt:", ggInverterToggleButtons[4] },
		{ "invertertoggle6_bt:", ggInverterToggleButtons[5] },
		{ "invertertoggle7_bt:", ggInverterToggleButtons[6] },
		{ "invertertoggle8_bt:", ggInverterToggleButtons[7] },
		{ "invertertoggle9_bt:", ggInverterToggleButtons[8] },
		{ "invertertoggle10_bt:", ggInverterToggleButtons[9] },
		{ "invertertoggle11_bt:", ggInverterToggleButtons[10] },
		{ "invertertoggle12_bt:", ggInverterToggleButtons[11] },
    };
    {
        auto const lookup { gauges.find( Label ) };
        if( lookup != gauges.end() ) {
            lookup->second.Load( Parser, DynamicObject );
            m_controlmapper.insert( lookup->second, lookup->first );
            return true;
        }
    }
    // dedicated gauges with state-driven optional submodel
    // TODO: move viable gauges here
    // TODO: convert dedicated gauges to auto-allocated ones, replace dedicated references in command handlers to mapper lookups
    std::unordered_map<std::string, std::tuple<TGauge &, bool const *> > const stategauges = {
        { "tempomat_sw:", { ggScndCtrlButton, &mvOccupied->SpeedCtrlUnit.IsActive } },
        { "tempomatoff_sw:", { ggScndCtrlOffButton, &mvOccupied->SpeedCtrlUnit.IsActive } },
        { "speedinc_bt:", { ggSpeedControlIncreaseButton, &mvOccupied->SpeedCtrlUnit.IsActive } },
		{ "speeddec_bt:", { ggSpeedControlDecreaseButton, &mvOccupied->SpeedCtrlUnit.IsActive } },
		{ "speedctrlpowerinc_bt:", { ggSpeedControlPowerIncreaseButton, &mvOccupied->SpeedCtrlUnit.IsActive } },
		{ "speedctrlpowerdec_bt:", { ggSpeedControlPowerDecreaseButton, &mvOccupied->SpeedCtrlUnit.IsActive } },
		{ "speedbutton0:", { ggSpeedCtrlButtons[ 0 ], &mvOccupied->SpeedCtrlUnit.IsActive } },
		{ "speedbutton1:", { ggSpeedCtrlButtons[ 1 ], &mvOccupied->SpeedCtrlUnit.IsActive } },
		{ "speedbutton2:", { ggSpeedCtrlButtons[ 2 ], &mvOccupied->SpeedCtrlUnit.IsActive } },
		{ "speedbutton3:", { ggSpeedCtrlButtons[ 3 ], &mvOccupied->SpeedCtrlUnit.IsActive } },
		{ "speedbutton4:", { ggSpeedCtrlButtons[ 4 ], &mvOccupied->SpeedCtrlUnit.IsActive } },
		{ "speedbutton5:", { ggSpeedCtrlButtons[ 5 ], &mvOccupied->SpeedCtrlUnit.IsActive } },
		{ "speedbutton6:", { ggSpeedCtrlButtons[ 6 ], &mvOccupied->SpeedCtrlUnit.IsActive } },
		{ "speedbutton7:", { ggSpeedCtrlButtons[ 7 ], &mvOccupied->SpeedCtrlUnit.IsActive } },
		{ "speedbutton8:", { ggSpeedCtrlButtons[ 8 ], &mvOccupied->SpeedCtrlUnit.IsActive } },
		{ "speedbutton9:", { ggSpeedCtrlButtons[ 9 ], &mvOccupied->SpeedCtrlUnit.IsActive } },
        { "doorleftpermit_sw:", { ggDoorLeftPermitButton, &m_doorspermitleft } },
        { "doorrightpermit_sw:", { ggDoorRightPermitButton, &m_doorspermitright } },
        { "dooralloff_sw:", { ggDoorAllOffButton, &m_doors } },
        { "doorstep_sw:", { ggDoorStepButton, &mvOccupied->Doors.step_enabled } },
        { "dirforward_bt:", { ggDirForwardButton, &m_dirforward } },
		{ "dirneutral_bt:", { ggDirNeutralButton, &m_dirneutral } },
		{ "dirbackward_bt:", { ggDirBackwardButton, &m_dirbackward } },
    };
    {
        auto const lookup { stategauges.find( Label ) };
        if( lookup != stategauges.end() ) {
            auto &gauge { std::get<TGauge &>( lookup->second ) };
            gauge.Load( Parser, DynamicObject );
            gauge.AssignState( std::get<bool const *>( lookup->second ) );
            m_controlmapper.insert( gauge, lookup->first );
            return true;
        }
    }
    // TODO: move viable dedicated gauges to the automatic array
    std::unordered_map<std::string, bool *> const autoboolgauges = {
        { "doormode_sw:", &mvOccupied->Doors.remote_only },
        { "coolingfans_sw:", &mvControlled->RVentForceOn },
        { "pantfront_sw:", &mvPantographUnit->Pantographs[end::front].valve.is_enabled },
        { "pantrear_sw:", &mvPantographUnit->Pantographs[end::rear].valve.is_enabled },
        { "pantfrontoff_sw:", &mvPantographUnit->Pantographs[end::front].valve.is_disabled },
        { "pantrearoff_sw:", &mvPantographUnit->Pantographs[end::rear].valve.is_disabled },
        { "radio_sw:", &mvOccupied->Radio },
        { "cablight_sw:", &Cabine[ iCabn ].bLight },
        { "springbraketoggle_bt:", &mvOccupied->SpringBrake.Activate },
        { "couplingdisconnect_sw:", &m_couplingdisconnect },
		{ "couplingdisconnectback_sw:", &m_couplingdisconnectback },
		{ "mirrors_sw:", &mvOccupied->MirrorForbidden },
    };
    {
        auto lookup = autoboolgauges.find( Label );
        if( lookup != autoboolgauges.end() ) {
            auto &gauge = Cabine[ Cabindex ].Gauge( -1 ); // pierwsza wolna lampka
            gauge.Load( Parser, DynamicObject );
            gauge.AssignBool( lookup->second );
            m_controlmapper.insert( gauge, lookup->first );
            return true;
        }
    }
    // TODO: move viable dedicated gauges to the automatic array
    std::unordered_map<std::string, int *> const autointgauges = {
        { "manualbrake:", &mvOccupied->ManualBrakePos },
        { "pantselect_sw:", &mvOccupied->PantsPreset.second[cab_to_end()] },
    };
    {
        auto lookup = autointgauges.find( Label );
        if( lookup != autointgauges.end() ) {
            auto &gauge = Cabine[ Cabindex ].Gauge( -1 ); // pierwsza wolna lampka
            gauge.Load( Parser, DynamicObject );
            gauge.AssignInt( lookup->second );
            m_controlmapper.insert( gauge, lookup->first );
            return true;
        }
    }

    // ABu 090305: uniwersalne przyciski lub inne rzeczy
    if( Label == "mainctrlact:" ) {
        ggMainCtrlAct.Load( Parser, DynamicObject);
    }
    // SEKCJA WSKAZNIKOW
    else if ((Label == "tachometer:") || (Label == "tachometerb:"))
    {
        // predkosciomierz wskaźnikowy z szarpaniem
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject);
        gauge.AssignFloat(&fTachoVelocityJump);
        // bind tachometer sound location to the meter
        if( dsbHasler
         && dsbHasler->offset() == glm::vec3() ) {
            dsbHasler->offset( gauge.model_offset() );
        }
    }
    else if (Label == "tachometern:")
    {
        // predkosciomierz wskaźnikowy bez szarpania
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject);
        gauge.AssignFloat(&fTachoVelocity);
        // bind tachometer sound location to the meter
        if( dsbHasler
         && dsbHasler->offset() == glm::vec3() ) {
            dsbHasler->offset( gauge.model_offset() );
        }
    }
    else if (Label == "tachometerd:")
    {
        // predkosciomierz cyfrowy
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject);
        gauge.AssignFloat(&fTachoVelocity);
        // bind tachometer sound location to the meter
        if( dsbHasler
         && dsbHasler->offset() == glm::vec3() ) {
            dsbHasler->offset( gauge.model_offset() );
        }
    }
    else if ((Label == "hvcurrent1:") || (Label == "hvcurrent1b:"))
    {
        // 1szy amperomierz
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject);
        gauge.AssignFloat(fHCurrent + 1);
    }
    else if ((Label == "hvcurrent2:") || (Label == "hvcurrent2b:"))
    {
        // 2gi amperomierz
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject);
        gauge.AssignFloat(fHCurrent + 2);
    }
    else if ((Label == "hvcurrent3:") || (Label == "hvcurrent3b:"))
    {
        // 3ci amperomierz
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałska
        gauge.Load(Parser, DynamicObject);
        gauge.AssignFloat(fHCurrent + 3);
    }
    else if ((Label == "hvcurrent:") || (Label == "hvcurrentb:"))
    {
        // amperomierz calkowitego pradu
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject);
        gauge.AssignFloat(fHCurrent);
    }
    else if (Label == "eimscreen:")
    {
        // amperomierz calkowitego pradu
        int i, j;
        Parser.getTokens(2, false);
        Parser >> i >> j;
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject);
        gauge.AssignFloat(&fEIMParams[i][j]);
    }
    else if (Label == "brakes:")
    {
        // specified pipe pressure of specified consist vehicle
        int i, j;
        Parser.getTokens(2, false);
        Parser >> i >> j;
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject, 0.1);
        gauge.AssignFloat(&fPress[clamp(i, 1, 20) - 1][clamp(j, 0, 3)]);
    }
    else if ((Label == "brakepress:") || (Label == "brakepressb:"))
    {
        // manometr cylindrow hamulcowych
        // Ra 2014-08: przeniesione do TCab
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject, 0.1);
        gauge.AssignDouble(&mvOccupied->BrakePress);
    }
    else if ((Label == "pipepress:") || (Label == "pipepressb:"))
    {
        // manometr przewodu hamulcowego
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject, 0.1);
        gauge.AssignDouble(&mvOccupied->PipePress);
    }
    else if( Label == "scndpress:" ) {
        // manometr przewodu hamulcowego
        auto &gauge = Cabine[ Cabindex ].Gauge( -1 ); // pierwsza wolna gałka
        gauge.Load( Parser, DynamicObject, 0.1 );
        gauge.AssignDouble( &mvOccupied->ScndPipePress );
    }
    else if (Label == "limpipepress:")
    {
        // manometr zbiornika sterujacego zaworu maszynisty
        auto &gauge = Cabine[ Cabindex ].Gauge( -1 ); // pierwsza wolna gałka
        gauge.Load( Parser, DynamicObject, 0.1 );
        gauge.AssignDouble( &m_brakehandlecp );
    }
    else if (Label == "cntrlpress:")
    {
        // manometr zbiornika kontrolnego/rorzďż˝du
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject, 0.1);
        gauge.AssignDouble(&mvPantographUnit->PantPress);
    }
	else if (Label == "springbrakepress:")
	{
		// manometr cylindra hamulca sprężynowego
		auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
		gauge.Load(Parser, DynamicObject, 0.1);
		gauge.AssignDouble(&mvOccupied->SpringBrake.SBP);
	}
	else if (Label == "epctrlvalue:")
	{
		// wskazowka sterowania sila hamulca ep
		auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
		gauge.Load(Parser, DynamicObject, 0.1);
		gauge.AssignDouble(&mvOccupied->EpForce);
	}
    else if ((Label == "compressor:") || (Label == "compressorb:"))
    {
        // manometr sprezarki/zbiornika glownego
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject, 0.1);
        gauge.AssignDouble(&mvOccupied->Compressor);
    }
    else if( Label == "oilpress:" ) {
        // oil pressure
        auto &gauge = Cabine[ Cabindex ].Gauge( -1 ); // pierwsza wolna gałka
        gauge.Load( Parser, DynamicObject );
        gauge.AssignFloat( &mvControlled->OilPump.pressure );
    }
    else if( Label == "oiltemp:" ) {
        // oil temperature
        auto &gauge = Cabine[ Cabindex ].Gauge( -1 ); // pierwsza wolna gałka
        gauge.Load( Parser, DynamicObject );
        gauge.AssignFloat( &mvControlled->dizel_heat.To );
    }
    else if( Label == "water1temp:" ) {
        // main circuit water temperature
        auto &gauge = Cabine[ Cabindex ].Gauge( -1 ); // pierwsza wolna gałka
        gauge.Load( Parser, DynamicObject );
        gauge.AssignFloat( &mvControlled->dizel_heat.temperatura1 );
    }
    else if( Label == "water2temp:" ) {
        // auxiliary circuit water temperature
        auto &gauge = Cabine[ Cabindex ].Gauge( -1 ); // pierwsza wolna gałka
        gauge.Load( Parser, DynamicObject );
        gauge.AssignFloat( &mvControlled->dizel_heat.temperatura2 );
    }
    else if( Label == "pantpress:" ) {
        // pantograph tank pressure
        auto &gauge = Cabine[ Cabindex ].Gauge( -1 ); // pierwsza wolna gałka
        gauge.Load( Parser, DynamicObject, 0.1 );
        gauge.AssignDouble( &mvPantographUnit->PantPress );
    }
    // yB - dla drugiej sekcji
    else if (Label == "hvbcurrent1:")
    {
        // 1szy amperomierz
        ggI1B.Load(Parser, DynamicObject);
    }
    else if (Label == "hvbcurrent2:")
    {
        // 2gi amperomierz
        ggI2B.Load(Parser, DynamicObject);
    }
    else if (Label == "hvbcurrent3:")
    {
        // 3ci amperomierz
        ggI3B.Load(Parser, DynamicObject);
    }
    else if (Label == "hvbcurrent:")
    {
        // amperomierz calkowitego pradu
        ggItotalB.Load(Parser, DynamicObject);
    }
    //*************************************************************
    else if (Label == "clock:")
    {
        // zegar analogowy
        if (Parser.getToken<std::string>() == "analog")
        {
            if( DynamicObject->mdKabina ) {
                // McZapkie-300302: zegarek
                ggClockSInd.Init( DynamicObject->mdKabina->GetFromName( "ClockShand" ), nullptr, TGaugeAnimation::gt_Rotate, 1.0 / 60.0 );
                ggClockMInd.Init( DynamicObject->mdKabina->GetFromName( "ClockMhand" ), nullptr, TGaugeAnimation::gt_Rotate, 1.0 / 60.0 );
                ggClockHInd.Init( DynamicObject->mdKabina->GetFromName( "ClockHhand" ), nullptr, TGaugeAnimation::gt_Rotate, 1.0 / 12.0 );
            }
        }
    }
    else if( Label == "clock_seconds:" ) {
        ggClockSInd.Load( Parser, DynamicObject );
    }
    else if (Label == "evoltage:")
    {
        // woltomierz napiecia silnikow
        ggEngineVoltage.Load(Parser, DynamicObject);
    }
    else if (Label == "hvoltage:")
    {
        // woltomierz wysokiego napiecia
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject);
        gauge.AssignFloat(&fHVoltage);
    }
    else if (Label == "lvoltage:")
    {
        // woltomierz niskiego napiecia
        ggLVoltage.Load(Parser, DynamicObject);
    }
    else if (Label == "enrot1m:")
    {
        // obrotomierz
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject);
        gauge.AssignFloat(fEngine + 1);
    } // ggEnrot1m.Load(Parser,DynamicObject->mdKabina);
    else if (Label == "enrot2m:")
    {
        // obrotomierz
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject);
        gauge.AssignFloat(fEngine + 2);
    } // ggEnrot2m.Load(Parser,DynamicObject->mdKabina);
    else if (Label == "enrot3m:")
    { // obrotomierz
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject);
        gauge.AssignFloat(fEngine + 3);
    } // ggEnrot3m.Load(Parser,DynamicObject->mdKabina);
    else if (Label == "engageratio:")
    {
        // np. ciśnienie sterownika sprzęgła
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject);
        gauge.AssignDouble(&mvControlled->dizel_engage);
    } // ggEngageRatio.Load(Parser,DynamicObject->mdKabina);
    else if (Label == "maingearstatus:")
    {
        // np. ciśnienie sterownika skrzyni biegów
        ggMainGearStatus.Load(Parser, DynamicObject);
    }
    else if (Label == "ignitionkey:")
    {
        ggIgnitionKey.Load(Parser, DynamicObject);
    }
    else if (Label == "distcounter:")
    {
        // Ra 2014-07: licznik kilometrów
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject);
        gauge.AssignDouble(&mvControlled->DistCounter);
    }
    else if( Label == "shuntmodepower:" ) {
        // shunt mode power slider
        auto &gauge = Cabine[Cabindex].Gauge(-1); // pierwsza wolna gałka
        gauge.Load(Parser, DynamicObject);
        gauge.AssignDouble(&mvControlled->AnPos);
        m_controlmapper.insert( gauge, "shuntmodepower:" );
    }
    else if( Label == "heatingvoltage:" ) {
        if( mvControlled->HeatingPowerSource.SourceType == TPowerSource::Generator ) {
            auto &gauge = Cabine[ Cabindex ].Gauge( -1 ); // pierwsza wolna gałka
            gauge.Load( Parser, DynamicObject );
            gauge.AssignDouble( &(mvControlled->HeatingPowerSource.EngineGenerator.voltage) );
        }
    }
    else if( Label == "heatingcurrent:" ) {
        auto &gauge = Cabine[ Cabindex ].Gauge( -1 ); // pierwsza wolna gałka
        gauge.Load( Parser, DynamicObject );
        gauge.AssignDouble( &( mvControlled->TotalCurrent ) );
    }
    else
    {
        // failed to match the label
        return false;
    }

    return true;
}

uint16_t TTrain::id() {
	if (vid == 0) {
		vid = ++simulation::prev_train_id;
		WriteLog("net: assigning id " + std::to_string(vid) + " to vehicle " + Dynamic()->name(), logtype::net);
	}
	return vid;
}

void train_table::update(double dt)
{
	for (TTrain *train : m_items) {
		if (!train)
			continue;

		train->Update(dt);

		if (train->pending_delete) {
			purge(train->Dynamic()->name());
			if (simulation::Train == train)
				simulation::Train = nullptr;
		}

		// for single-player destroy non-player trains
		if (simulation::Train != train
		        && Global.network_servers.empty() && !Global.network_client) {
			purge(train->Dynamic()->name());
		}
	}
}

TTrain *
train_table::find_id( std::uint16_t const Id ) const {

    if( Id == 0 ) { return nullptr; }

    for( TTrain *train : m_items ) {
        if( !train ) {
            continue;
        }
        if( train->id() == Id ) {
            return train;
        }
    }
    return nullptr;
}
