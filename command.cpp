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

#include "globals.h"
#include "logs.h"
#include "timer.h"

namespace simulation {

command_queue Commands;

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

command_relay::command_relay() {

    m_targets = commandtarget_map{

        { user_command::mastercontrollerincrease, command_target::vehicle },
        { user_command::mastercontrollerincreasefast, command_target::vehicle },
        { user_command::mastercontrollerdecrease, command_target::vehicle },
        { user_command::mastercontrollerdecreasefast, command_target::vehicle },
        { user_command::secondcontrollerincrease, command_target::vehicle },
        { user_command::secondcontrollerincreasefast, command_target::vehicle },
        { user_command::secondcontrollerdecrease, command_target::vehicle },
        { user_command::secondcontrollerdecreasefast, command_target::vehicle },
        { user_command::independentbrakeincrease, command_target::vehicle },
        { user_command::independentbrakeincreasefast, command_target::vehicle },
        { user_command::independentbrakedecrease, command_target::vehicle },
        { user_command::independentbrakedecreasefast, command_target::vehicle },
        { user_command::independentbrakebailoff, command_target::vehicle },
        { user_command::trainbrakeincrease, command_target::vehicle },
        { user_command::trainbrakedecrease, command_target::vehicle },
        { user_command::trainbrakecharging, command_target::vehicle },
        { user_command::trainbrakerelease, command_target::vehicle },
        { user_command::trainbrakefirstservice, command_target::vehicle },
        { user_command::trainbrakeservice, command_target::vehicle },
        { user_command::trainbrakefullservice, command_target::vehicle },
        { user_command::trainbrakeemergency, command_target::vehicle },
/*
const int k_AntiSlipping = 21;
const int k_Sand = 22;
*/
        { user_command::reverserincrease, command_target::vehicle },
        { user_command::reverserdecrease, command_target::vehicle },
        { user_command::linebreakertoggle, command_target::vehicle },
/*
const int k_Fuse = 26;
*/
        { user_command::convertertoggle, command_target::vehicle },
        { user_command::compressortoggle, command_target::vehicle },
        { user_command::motoroverloadrelaythresholdtoggle, command_target::vehicle },
/*
const int k_CurrentAutoRelay = 30;
const int k_BrakeProfile = 31;
*/
        { user_command::alerteracknowledge, command_target::vehicle },
/*
const int k_Czuwak = 32;
const int k_Horn = 33;
const int k_Horn2 = 34;
const int k_FailedEngineCutOff = 35;
*/
        { user_command::viewturn, command_target::entity },
        { user_command::movevector, command_target::entity },
        { user_command::moveleft, command_target::entity },
        { user_command::moveright, command_target::entity },
        { user_command::moveforward, command_target::entity },
        { user_command::moveback, command_target::entity },
        { user_command::moveup, command_target::entity },
        { user_command::movedown, command_target::entity },
        { user_command::moveleftfast, command_target::entity },
        { user_command::moverightfast, command_target::entity },
        { user_command::moveforwardfast, command_target::entity },
        { user_command::movebackfast, command_target::entity },
        { user_command::moveupfast, command_target::entity },
        { user_command::movedownfast, command_target::entity },
/*
const int k_CabForward = 42;
const int k_CabBackward = 43;
const int k_Couple = 44;
const int k_DeCouple = 45;
const int k_ProgramQuit = 46;
// const int k_ProgramPause= 47;
const int k_ProgramHelp = 48;
*/
        { user_command::doortoggleleft, command_target::vehicle },
        { user_command::doortoggleright, command_target::vehicle },
/*
const int k_DepartureSignal = 53;
*/
        { user_command::pantographtogglefront, command_target::vehicle },
        { user_command::pantographtogglerear, command_target::vehicle },
/*
const int k_Heating = 58;
// const int k_FreeFlyMode= 59;
*/
        { user_command::headlighttoggleleft, command_target::vehicle },
        { user_command::headlighttoggleright, command_target::vehicle },
        { user_command::headlighttoggleupper, command_target::vehicle },
        { user_command::redmarkertoggleleft, command_target::vehicle },
        { user_command::redmarkertoggleright, command_target::vehicle },
        { user_command::headlighttogglerearleft, command_target::vehicle },
        { user_command::headlighttogglerearright, command_target::vehicle },
        { user_command::headlighttogglerearupper, command_target::vehicle },
        { user_command::redmarkertogglerearleft, command_target::vehicle },
        { user_command::redmarkertogglerearright, command_target::vehicle },
/*
const int k_SmallCompressor = 63;
const int k_StLinOff = 64;
const int k_CurrentNext = 65;
const int k_Univ1 = 66;
const int k_Univ2 = 67;
const int k_Univ3 = 68;
const int k_Univ4 = 69;
const int k_EndSign = 70;
const int k_Active = 71;
*/
        { user_command::batterytoggle, command_target::vehicle }
/*
const int k_WalkMode = 73;
int const k_DimHeadlights = 74;
*/
    };

#ifdef _DEBUG
    m_commandnames = {

        "mastercontrollerincrease",
        "mastercontrollerincreasefast",
        "mastercontrollerdecrease",
        "mastercontrollerdecreasefast",
        "secondcontrollerincrease",
        "secondcontrollerincreasefast",
        "secondcontrollerdecrease",
        "secondcontrollerdecreasefast",
        "independentbrakeincrease",
        "independentbrakeincreasefast",
        "independentbrakedecrease",
        "independentbrakedecreasefast",
        "independentbrakebailoff",
        "trainbrakeincrease",
        "trainbrakedecrease",
        "trainbrakecharging",
        "trainbrakerelease",
        "trainbrakefirstservice",
        "trainbrakeservice",
        "trainbrakefullservice",
        "trainbrakeemergency",
/*
const int k_AntiSlipping = 21;
const int k_Sand = 22;
*/
        "reverserincrease",
        "reverserdecrease",
        "linebreakertoggle",
/*
const int k_Fuse = 26;
*/
        "convertertoggle",
        "compressortoggle",
        "motoroverloadrelaythresholdtoggle",
/*
const int k_MaxCurrent = 29;
const int k_CurrentAutoRelay = 30;
const int k_BrakeProfile = 31;
*/
        "alerteracknowledge",
/*
const int k_Czuwak = 32;
const int k_Horn = 33;
const int k_Horn2 = 34;
const int k_FailedEngineCutOff = 35;
*/
        "", //"viewturn",
        "", //"movevector",
        "", //"moveleft",
        "", //"moveright",
        "", //"moveforward",
        "", //"moveback",
        "", //"moveup",
        "", //"movedown",
        "", //"moveleftfast",
        "", //"moverightfast",
        "", //"moveforwardfast",
        "", //"movebackfast",
        "", //"moveupfast",
        "", //"movedownfast"
/*
const int k_CabForward = 42;
const int k_CabBackward = 43;
const int k_Couple = 44;
const int k_DeCouple = 45;
const int k_ProgramQuit = 46;
// const int k_ProgramPause= 47;
const int k_ProgramHelp = 48;
*/
        "doortoggleleft",
        "doortoggleright",
/*
const int k_DepartureSignal = 53;
*/
        "pantographtogglefront",
        "pantographtogglerear",
/*
const int k_Heating = 58;
// const int k_FreeFlyMode= 59;
*/
        "headlighttoggleleft",
        "headlighttoggleright",
        "headlighttoggleupper",
        "redmarkertoggleleft",
        "redmarkertoggleright",
        "headlighttogglerearleft",
        "headlighttogglerearright",
        "headlighttogglerearupper",
        "redmarkertogglerearleft",
        "redmarkertogglerearright",
/*
const int k_SmallCompressor = 63;
const int k_StLinOff = 64;
const int k_CurrentNext = 65;
const int k_Univ1 = 66;
const int k_Univ2 = 67;
const int k_Univ3 = 68;
const int k_Univ4 = 69;
const int k_EndSign = 70;
const int k_Active = 71;
*/
        "batterytoggle"
/*
const int k_WalkMode = 73;
int const k_DimHeadlights = 74;
*/
    };
#endif
}

void
command_relay::post( user_command const Command, std::uint64_t const Param1, std::uint64_t const Param2, int const Action, std::uint16_t const Recipient ) const {

    auto const &lookup = m_targets.find( Command );
    if( lookup == m_targets.end() ) {
        // shouldn't be really needed but, eh
        return;
    }
    if( ( lookup->second == command_target::vehicle )
     && ( true == FreeFlyModeFlag )
     && ( false == DebugModeFlag ) ) {
        // don't pass vehicle commands if the user isn't in one, unless we're in debug mode
        return;
    }

    simulation::Commands.push(
        command_data{
            Command,
            Action,
            Param1,
            Param2,
            Timer::GetDeltaTime() },
        static_cast<std::size_t>( lookup->second ) | Recipient );

#ifdef _DEBUG
    if( Action != GLFW_RELEASE ) {
    // key was pressed or is still held
        auto const &commandname = m_commandnames.at( static_cast<std::size_t>( Command ) );
        if( false == commandname.empty() ) {
            WriteLog( "Command issued: " + commandname );
        }
    }
/*
    else {
    // key was released (but we don't log this)
        WriteLog( "Key released: " + m_commandnames.at( static_cast<std::size_t>( Command ) ) );
    }
*/
#endif
}

//---------------------------------------------------------------------------
