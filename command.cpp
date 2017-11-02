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

namespace simulation {

command_queue Commands;
commanddescription_sequence Commands_descriptions = {

    { "mastercontrollerincrease", command_target::vehicle },
    { "mastercontrollerincreasefast", command_target::vehicle },
    { "mastercontrollerdecrease", command_target::vehicle },
    { "mastercontrollerdecreasefast", command_target::vehicle },
    { "secondcontrollerincrease", command_target::vehicle },
    { "secondcontrollerincreasefast", command_target::vehicle },
    { "secondcontrollerdecrease", command_target::vehicle },
    { "secondcontrollerdecreasefast", command_target::vehicle },
    { "mucurrentindicatorothersourceactivate", command_target::vehicle },
    { "independentbrakeincrease", command_target::vehicle },
    { "independentbrakeincreasefast", command_target::vehicle },
    { "independentbrakedecrease", command_target::vehicle },
    { "independentbrakedecreasefast", command_target::vehicle },
    { "independentbrakebailoff", command_target::vehicle },
    { "trainbrakeincrease", command_target::vehicle },
    { "trainbrakedecrease", command_target::vehicle },
    { "trainbrakecharging", command_target::vehicle },
    { "trainbrakerelease", command_target::vehicle },
    { "trainbrakefirstservice", command_target::vehicle },
    { "trainbrakeservice", command_target::vehicle },
    { "trainbrakefullservice", command_target::vehicle },
    { "trainbrakeemergency", command_target::vehicle },
    { "manualbrakeincrease", command_target::vehicle },
    { "manualbrakedecrease", command_target::vehicle },
    { "alarmchaintoggle", command_target::vehicle },
    { "wheelspinbrakeactivate", command_target::vehicle },
    { "sandboxactivate", command_target::vehicle },
    { "reverserincrease", command_target::vehicle },
    { "reverserdecrease", command_target::vehicle },
    { "linebreakertoggle", command_target::vehicle },
    { "convertertoggle", command_target::vehicle },
    { "convertertogglelocal", command_target::vehicle },
    { "converteroverloadrelayreset", command_target::vehicle },
    { "compressortoggle", command_target::vehicle },
    { "compressortogglelocal", command_target::vehicle },
    { "motoroverloadrelaythresholdtoggle", command_target::vehicle },
    { "motoroverloadrelayreset", command_target::vehicle },
    { "notchingrelaytoggle", command_target::vehicle },
    { "epbrakecontroltoggle", command_target::vehicle },
    { "brakeactingspeedincrease", command_target::vehicle },
    { "brakeactingspeeddecrease", command_target::vehicle },
    { "mubrakingindicatortoggle", command_target::vehicle },
    { "alerteracknowledge", command_target::vehicle },
    { "hornlowactivate", command_target::vehicle },
    { "hornhighactivate", command_target::vehicle },
    { "radiotoggle", command_target::vehicle },
    { "radiostoptest", command_target::vehicle },
/*
const int k_FailedEngineCutOff = 35;
*/
    { "viewturn", command_target::entity },
    { "movevector", command_target::entity },
    { "moveleft", command_target::entity },
    { "moveright", command_target::entity },
    { "moveforward", command_target::entity },
    { "moveback", command_target::entity },
    { "moveup", command_target::entity },
    { "movedown", command_target::entity },
    { "moveleftfast", command_target::entity },
    { "moverightfast", command_target::entity },
    { "moveforwardfast", command_target::entity },
    { "movebackfast", command_target::entity },
    { "moveupfast", command_target::entity },
    { "movedownfast", command_target::entity },
/*
const int k_CabForward = 42;
const int k_CabBackward = 43;
const int k_Couple = 44;
const int k_DeCouple = 45;
const int k_ProgramQuit = 46;
// const int k_ProgramPause= 47;
const int k_ProgramHelp = 48;
*/
    { "doortoggleleft", command_target::vehicle },
    { "doortoggleright", command_target::vehicle },
    { "departureannounce", command_target::vehicle },
    { "doorlocktoggle", command_target::vehicle },
    { "pantographcompressorvalvetoggle", command_target::vehicle },
    { "pantographcompressoractivate", command_target::vehicle },
    { "pantographtogglefront", command_target::vehicle },
    { "pantographtogglerear", command_target::vehicle },
    { "pantographlowerall", command_target::vehicle },
    { "heatingtoggle", command_target::vehicle },
/*
// const int k_FreeFlyMode= 59;
*/
    { "headlighttoggleleft", command_target::vehicle },
    { "headlighttoggleright", command_target::vehicle },
    { "headlighttoggleupper", command_target::vehicle },
    { "redmarkertoggleleft", command_target::vehicle },
    { "redmarkertoggleright", command_target::vehicle },
    { "headlighttogglerearleft", command_target::vehicle },
    { "headlighttogglerearright", command_target::vehicle },
    { "headlighttogglerearupper", command_target::vehicle },
    { "redmarkertogglerearleft", command_target::vehicle },
    { "redmarkertogglerearright", command_target::vehicle },
    { "headlightsdimtoggle", command_target::vehicle },
    { "motorconnectorsopen", command_target::vehicle },
    { "motordisconnect", command_target::vehicle },
    { "interiorlighttoggle", command_target::vehicle },
    { "interiorlightdimtoggle", command_target::vehicle },
    { "instrumentlighttoggle", command_target::vehicle },
/*
const int k_EndSign = 70;
const int k_Active = 71;
*/
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
    { "batterytoggle", command_target::vehicle }
/*
const int k_WalkMode = 73;
*/
};

}

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
command_relay::post( user_command const Command, std::uint64_t const Param1, std::uint64_t const Param2,
                     int const Action, std::uint16_t const Recipient,
                     command_data::desired_state_t state) const {

    auto const &command = simulation::Commands_descriptions[ static_cast<std::size_t>( Command ) ];
    if( ( command.target == command_target::vehicle )
     && ( true == FreeFlyModeFlag )
     && ( ( false == DebugModeFlag )
       && ( true == Global::RealisticControlMode ) ) ) {
        // in realistic control mode don't pass vehicle commands if the user isn't in one, unless we're in debug mode
        return;
    }

    simulation::Commands.push(
        command_data{
            Command,
            Action,
            Param1,
            Param2,
            state, Timer::GetDeltaTime() },
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
