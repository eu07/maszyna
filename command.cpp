/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "command.h"

#include "Globals.h"
#include "Logs.h"
#include "Timer.h"
#include "utilities.h"

namespace simulation {

command_queue Commands;
commanddescription_sequence Commands_descriptions = {

    { "aidriverenable", command_target::vehicle },
    { "aidriverdisable", command_target::vehicle },
    { "mastercontrollerincrease", command_target::vehicle },
    { "mastercontrollerincreasefast", command_target::vehicle },
    { "mastercontrollerdecrease", command_target::vehicle },
    { "mastercontrollerdecreasefast", command_target::vehicle },
    { "mastercontrollerset", command_target::vehicle },
    { "secondcontrollerincrease", command_target::vehicle },
    { "secondcontrollerincreasefast", command_target::vehicle },
    { "secondcontrollerdecrease", command_target::vehicle },
    { "secondcontrollerdecreasefast", command_target::vehicle },
    { "secondcontrollerset", command_target::vehicle },
    { "mucurrentindicatorothersourceactivate", command_target::vehicle },
    { "independentbrakeincrease", command_target::vehicle },
    { "independentbrakeincreasefast", command_target::vehicle },
    { "independentbrakedecrease", command_target::vehicle },
    { "independentbrakedecreasefast", command_target::vehicle },
    { "independentbrakeset", command_target::vehicle },
    { "independentbrakebailoff", command_target::vehicle },
    { "trainbrakeincrease", command_target::vehicle },
    { "trainbrakedecrease", command_target::vehicle },
    { "trainbrakeset", command_target::vehicle },
    { "trainbrakecharging", command_target::vehicle },
    { "trainbrakerelease", command_target::vehicle },
    { "trainbrakefirstservice", command_target::vehicle },
    { "trainbrakeservice", command_target::vehicle },
    { "trainbrakefullservice", command_target::vehicle },
    { "trainbrakehandleoff", command_target::vehicle },
    { "trainbrakeemergency", command_target::vehicle },
    { "trainbrakebasepressureincrease", command_target::vehicle },
    { "trainbrakebasepressuredecrease", command_target::vehicle },
    { "trainbrakebasepressurereset", command_target::vehicle },
    { "trainbrakeoperationtoggle", command_target::vehicle },
    { "manualbrakeincrease", command_target::vehicle },
    { "manualbrakedecrease", command_target::vehicle },
    { "alarmchaintoggle", command_target::vehicle },
    { "wheelspinbrakeactivate", command_target::vehicle },
    { "sandboxactivate", command_target::vehicle },
    { "reverserincrease", command_target::vehicle },
    { "reverserdecrease", command_target::vehicle },
    { "reverserforwardhigh", command_target::vehicle },
    { "reverserforward", command_target::vehicle },
    { "reverserneutral", command_target::vehicle },
    { "reverserbackward", command_target::vehicle },
    { "waterpumpbreakertoggle", command_target::vehicle },
    { "waterpumpbreakerclose", command_target::vehicle },
    { "waterpumpbreakeropen", command_target::vehicle },
    { "waterpumptoggle", command_target::vehicle },
    { "waterpumpenable", command_target::vehicle },
    { "waterpumpdisable", command_target::vehicle },
    { "waterheaterbreakertoggle", command_target::vehicle },
    { "waterheaterbreakerclose", command_target::vehicle },
    { "waterheaterbreakeropen", command_target::vehicle },
    { "waterheatertoggle", command_target::vehicle },
    { "waterheaterenable", command_target::vehicle },
    { "waterheaterdisable", command_target::vehicle },
    { "watercircuitslinktoggle", command_target::vehicle },
    { "watercircuitslinkenable", command_target::vehicle },
    { "watercircuitslinkdisable", command_target::vehicle },
    { "fuelpumptoggle", command_target::vehicle },
    { "fuelpumpenable", command_target::vehicle },
    { "fuelpumpdisable", command_target::vehicle },
    { "oilpumptoggle", command_target::vehicle },
    { "oilpumpenable", command_target::vehicle },
    { "oilpumpdisable", command_target::vehicle },
    { "linebreakertoggle", command_target::vehicle },
    { "linebreakeropen", command_target::vehicle },
    { "linebreakerclose", command_target::vehicle },
    { "convertertoggle", command_target::vehicle },
    { "converterenable", command_target::vehicle },
    { "converterdisable", command_target::vehicle },
    { "convertertogglelocal", command_target::vehicle },
    { "converteroverloadrelayreset", command_target::vehicle },
    { "compressortoggle", command_target::vehicle },
    { "compressorenable", command_target::vehicle },
    { "compressordisable", command_target::vehicle },
    { "compressortogglelocal", command_target::vehicle },
    { "motoroverloadrelaythresholdtoggle", command_target::vehicle },
    { "motoroverloadrelaythresholdsetlow", command_target::vehicle },
    { "motoroverloadrelaythresholdsethigh", command_target::vehicle },
    { "motoroverloadrelayreset", command_target::vehicle },
    { "notchingrelaytoggle", command_target::vehicle },
    { "epbrakecontroltoggle", command_target::vehicle },
	{ "trainbrakeoperationmodeincrease", command_target::vehicle },
	{ "trainbrakeoperationmodedecrease", command_target::vehicle },
    { "brakeactingspeedincrease", command_target::vehicle },
    { "brakeactingspeeddecrease", command_target::vehicle },
    { "brakeactingspeedsetcargo", command_target::vehicle },
    { "brakeactingspeedsetpassenger", command_target::vehicle },
    { "brakeactingspeedsetrapid", command_target::vehicle },
    { "brakeloadcompensationincrease", command_target::vehicle },
    { "brakeloadcompensationdecrease", command_target::vehicle },
    { "mubrakingindicatortoggle", command_target::vehicle },
    { "alerteracknowledge", command_target::vehicle },
    { "hornlowactivate", command_target::vehicle },
    { "hornhighactivate", command_target::vehicle },
    { "whistleactivate", command_target::vehicle },
    { "radiotoggle", command_target::vehicle },
    { "radiochannelincrease", command_target::vehicle },
    { "radiochanneldecrease", command_target::vehicle },
    { "radiostopsend", command_target::vehicle },
    { "radiostoptest", command_target::vehicle },
    // TBD, TODO: make cab change controls entity-centric
    { "cabchangeforward", command_target::vehicle },
    { "cabchangebackward", command_target::vehicle },

    { "viewturn", command_target::entity },
    { "movehorizontal", command_target::entity },
    { "movehorizontalfast", command_target::entity },
    { "movevertical", command_target::entity },
    { "moveverticalfast", command_target::entity },
    { "moveleft", command_target::entity },
    { "moveright", command_target::entity },
    { "moveforward", command_target::entity },
    { "moveback", command_target::entity },
    { "moveup", command_target::entity },
    { "movedown", command_target::entity },
    // TBD, TODO: make coupling controls entity-centric
    { "carcouplingincrease", command_target::vehicle },
    { "carcouplingdisconnect", command_target::vehicle },
    { "doortoggleleft", command_target::vehicle },
    { "doortoggleright", command_target::vehicle },
    { "doorpermitleft", command_target::vehicle },
    { "doorpermitright", command_target::vehicle },
    { "doorpermitpresetactivatenext", command_target::vehicle },
    { "doorpermitpresetactivateprevious", command_target::vehicle },
    { "dooropenleft", command_target::vehicle },
    { "dooropenright", command_target::vehicle },
    { "dooropenall", command_target::vehicle },
    { "doorcloseleft", command_target::vehicle },
    { "doorcloseright", command_target::vehicle },
    { "doorcloseall", command_target::vehicle },
    { "departureannounce", command_target::vehicle },
    { "doorlocktoggle", command_target::vehicle },
    { "pantographcompressorvalvetoggle", command_target::vehicle },
    { "pantographcompressoractivate", command_target::vehicle },
    { "pantographtogglefront", command_target::vehicle },
    { "pantographtogglerear", command_target::vehicle },
    { "pantographraisefront", command_target::vehicle },
    { "pantographraiserear", command_target::vehicle },
    { "pantographlowerfront", command_target::vehicle },
    { "pantographlowerrear", command_target::vehicle },
    { "pantographlowerall", command_target::vehicle },
    { "heatingtoggle", command_target::vehicle },
    { "heatingenable", command_target::vehicle },
    { "heatingdisable", command_target::vehicle },
    { "lightspresetactivatenext", command_target::vehicle },
    { "lightspresetactivateprevious", command_target::vehicle },
    { "headlighttoggleleft", command_target::vehicle },
    { "headlightenableleft", command_target::vehicle },
    { "headlightdisableleft", command_target::vehicle },
    { "headlighttoggleright", command_target::vehicle },
    { "headlightenableright", command_target::vehicle },
    { "headlightdisableright", command_target::vehicle },
    { "headlighttoggleupper", command_target::vehicle },
    { "headlightenableupper", command_target::vehicle },
    { "headlightdisableupper", command_target::vehicle },
    { "redmarkertoggleleft", command_target::vehicle },
    { "redmarkerenableleft", command_target::vehicle },
    { "redmarkerdisableleft", command_target::vehicle },
    { "redmarkertoggleright", command_target::vehicle },
    { "redmarkerenableright", command_target::vehicle },
    { "redmarkerdisableright", command_target::vehicle },
    { "headlighttogglerearleft", command_target::vehicle },
    { "headlighttogglerearright", command_target::vehicle },
    { "headlighttogglerearupper", command_target::vehicle },
    { "redmarkertogglerearleft", command_target::vehicle },
    { "redmarkertogglerearright", command_target::vehicle },
    { "redmarkerstoggle", command_target::vehicle },
    { "endsignalstoggle", command_target::vehicle },
    { "headlightsdimtoggle", command_target::vehicle },
    { "headlightsdimenable", command_target::vehicle },
    { "headlightsdimdisable", command_target::vehicle },
    { "motorconnectorsopen", command_target::vehicle },
    { "motorconnectorsclose", command_target::vehicle },
    { "motordisconnect", command_target::vehicle },
    { "interiorlighttoggle", command_target::vehicle },
    { "interiorlightenable", command_target::vehicle },
    { "interiorlightdisable", command_target::vehicle },
    { "interiorlightdimtoggle", command_target::vehicle },
    { "interiorlightdimenable", command_target::vehicle },
    { "interiorlightdimdisable", command_target::vehicle },
    { "instrumentlighttoggle", command_target::vehicle },
    { "instrumentlightenable", command_target::vehicle },
    { "instrumentlightdisable", command_target::vehicle },
    { "dashboardlighttoggle", command_target::vehicle },
    { "timetablelighttoggle", command_target::vehicle },
    { "generictoggle0", command_target::vehicle },
    { "generictoggle1", command_target::vehicle },
    { "generictoggle2", command_target::vehicle },
    { "generictoggle3", command_target::vehicle },
    { "generictoggle4", command_target::vehicle },
    { "generictoggle5", command_target::vehicle },
    { "generictoggle6", command_target::vehicle },
    { "generictoggle7", command_target::vehicle },
    { "generictoggle8", command_target::vehicle },
    { "generictoggle9", command_target::vehicle },
    { "batterytoggle", command_target::vehicle },
    { "batteryenable", command_target::vehicle },
    { "batterydisable", command_target::vehicle },
    { "motorblowerstogglefront", command_target::vehicle },
    { "motorblowerstogglerear", command_target::vehicle },
    { "motorblowersdisableall", command_target::vehicle }

};

} // simulation

// posts specified command for specified recipient
void
command_queue::push( command_data const &Command, std::size_t const Recipient ) {

    auto lookup = m_commands.emplace( Recipient, commanddata_sequence() );
    // recipient stack was either located or created, so we can add to it quite safely
    lookup.first->second.emplace( Command );
}

// retrieves oldest posted command for specified recipient, if any. returns: true on retrieval, false if there's nothing to retrieve
bool
command_queue::pop( command_data &Command, std::size_t const Recipient ) {

    auto lookup = m_commands.find( Recipient );
    if( lookup == m_commands.end() ) {
        // no command stack for this recipient, so no commands
        return false;
    }
    auto &commands = lookup->second;
    if( true == commands.empty() ) {

        return false;
    }
    // we have command stack with command(s) on it, retrieve and pop the first one
    Command = commands.front();
    commands.pop();

    return true;
}

void
command_relay::post( user_command const Command, double const Param1, double const Param2, int const Action, std::uint16_t const Recipient ) const {

    auto const &command = simulation::Commands_descriptions[ static_cast<std::size_t>( Command ) ];
    if( ( command.target == command_target::vehicle )
     && ( true == FreeFlyModeFlag )
     && ( ( false == DebugModeFlag )
       && ( true == Global.RealisticControlMode ) ) ) {
        // in realistic control mode don't pass vehicle commands if the user isn't in one, unless we're in debug mode
        return;
    }

    simulation::Commands.push(
        command_data{
            Command,
            Action,
            Param1,
            Param2,
            Timer::GetDeltaTime() },
        static_cast<std::size_t>( command.target ) | Recipient );
/*
#ifdef _DEBUG
    if( Action != GLFW_RELEASE ) {
    // key was pressed or is still held
        if( false == command.name.empty() ) {
            if( false == (
                ( Command == user_command::moveleft )
             || ( Command == user_command::moveleftfast )
             || ( Command == user_command::moveright )
             || ( Command == user_command::moverightfast )
             || ( Command == user_command::moveforward )
             || ( Command == user_command::moveforwardfast )
             || ( Command == user_command::moveback )
             || ( Command == user_command::movebackfast )
             || ( Command == user_command::moveup )
             || ( Command == user_command::moveupfast )
             || ( Command == user_command::movedown )
             || ( Command == user_command::movedownfast )
             || ( Command == user_command::movevector )
             || ( Command == user_command::viewturn ) ) ) {
                WriteLog( "Command issued: " + command.name );
            }
        }
    }
    else {
    // key was released (but we don't log this)
        WriteLog( "Key released: " + command.name );
    }
#endif
*/
}

//---------------------------------------------------------------------------
