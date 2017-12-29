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

    { "mastercontrollerincrease", command_target::vehicle, command_mode::oneoff },
    { "mastercontrollerincreasefast", command_target::vehicle, command_mode::oneoff },
    { "mastercontrollerdecrease", command_target::vehicle, command_mode::oneoff },
    { "mastercontrollerdecreasefast", command_target::vehicle, command_mode::oneoff },
    { "secondcontrollerincrease", command_target::vehicle, command_mode::oneoff },
    { "secondcontrollerincreasefast", command_target::vehicle, command_mode::oneoff },
    { "secondcontrollerdecrease", command_target::vehicle, command_mode::oneoff },
    { "secondcontrollerdecreasefast", command_target::vehicle, command_mode::oneoff },
    { "mucurrentindicatorothersourceactivate", command_target::vehicle, command_mode::oneoff },
    { "independentbrakeincrease", command_target::vehicle, command_mode::oneoff },
    { "independentbrakeincreasefast", command_target::vehicle, command_mode::oneoff },
    { "independentbrakedecrease", command_target::vehicle, command_mode::oneoff },
    { "independentbrakedecreasefast", command_target::vehicle, command_mode::oneoff },
    { "independentbrakebailoff", command_target::vehicle, command_mode::oneoff },
    { "trainbrakeincrease", command_target::vehicle, command_mode::continuous },
    { "trainbrakedecrease", command_target::vehicle, command_mode::continuous },
    { "trainbrakecharging", command_target::vehicle, command_mode::oneoff },
    { "trainbrakerelease", command_target::vehicle, command_mode::oneoff },
    { "trainbrakefirstservice", command_target::vehicle, command_mode::oneoff },
    { "trainbrakeservice", command_target::vehicle, command_mode::oneoff },
    { "trainbrakefullservice", command_target::vehicle, command_mode::oneoff },
    { "trainbrakeemergency", command_target::vehicle, command_mode::oneoff },
    { "manualbrakeincrease", command_target::vehicle, command_mode::oneoff },
    { "manualbrakedecrease", command_target::vehicle, command_mode::oneoff },
    { "alarmchaintoggle", command_target::vehicle, command_mode::oneoff },
    { "wheelspinbrakeactivate", command_target::vehicle, command_mode::oneoff },
    { "sandboxactivate", command_target::vehicle, command_mode::oneoff },
    { "reverserincrease", command_target::vehicle, command_mode::oneoff },
    { "reverserdecrease", command_target::vehicle, command_mode::oneoff },
    { "linebreakertoggle", command_target::vehicle, command_mode::oneoff },
    { "convertertoggle", command_target::vehicle, command_mode::oneoff },
    { "convertertogglelocal", command_target::vehicle, command_mode::oneoff },
    { "converteroverloadrelayreset", command_target::vehicle, command_mode::oneoff },
    { "compressortoggle", command_target::vehicle, command_mode::oneoff },
    { "compressortogglelocal", command_target::vehicle, command_mode::oneoff },
    { "motoroverloadrelaythresholdtoggle", command_target::vehicle, command_mode::oneoff },
    { "motoroverloadrelayreset", command_target::vehicle, command_mode::oneoff },
    { "notchingrelaytoggle", command_target::vehicle, command_mode::oneoff },
    { "epbrakecontroltoggle", command_target::vehicle, command_mode::oneoff },
    { "brakeactingspeedincrease", command_target::vehicle, command_mode::oneoff },
    { "brakeactingspeeddecrease", command_target::vehicle, command_mode::oneoff },
    { "mubrakingindicatortoggle", command_target::vehicle, command_mode::oneoff },
    { "alerteracknowledge", command_target::vehicle, command_mode::oneoff },
    { "hornlowactivate", command_target::vehicle, command_mode::oneoff },
    { "hornhighactivate", command_target::vehicle, command_mode::oneoff },
    { "radiotoggle", command_target::vehicle, command_mode::oneoff },
    { "radiostoptest", command_target::vehicle, command_mode::oneoff },
/*
const int k_FailedEngineCutOff = 35;
*/
    { "viewturn", command_target::entity, command_mode::oneoff },
    { "movevector", command_target::entity, command_mode::oneoff },
    { "moveleft", command_target::entity, command_mode::oneoff },
    { "moveright", command_target::entity, command_mode::oneoff },
    { "moveforward", command_target::entity, command_mode::oneoff },
    { "moveback", command_target::entity, command_mode::oneoff },
    { "moveup", command_target::entity, command_mode::oneoff },
    { "movedown", command_target::entity, command_mode::oneoff },
    { "moveleftfast", command_target::entity, command_mode::oneoff },
    { "moverightfast", command_target::entity, command_mode::oneoff },
    { "moveforwardfast", command_target::entity, command_mode::oneoff },
    { "movebackfast", command_target::entity, command_mode::oneoff },
    { "moveupfast", command_target::entity, command_mode::oneoff },
    { "movedownfast", command_target::entity, command_mode::oneoff },
/*
const int k_CabForward = 42;
const int k_CabBackward = 43;
const int k_Couple = 44;
const int k_DeCouple = 45;
const int k_ProgramQuit = 46;
// const int k_ProgramPause= 47;
const int k_ProgramHelp = 48;
*/
    { "doortoggleleft", command_target::vehicle, command_mode::oneoff },
    { "doortoggleright", command_target::vehicle, command_mode::oneoff },
    { "departureannounce", command_target::vehicle, command_mode::oneoff },
    { "doorlocktoggle", command_target::vehicle, command_mode::oneoff },
    { "pantographcompressorvalvetoggle", command_target::vehicle, command_mode::oneoff },
    { "pantographcompressoractivate", command_target::vehicle, command_mode::oneoff },
    { "pantographtogglefront", command_target::vehicle, command_mode::oneoff },
    { "pantographtogglerear", command_target::vehicle, command_mode::oneoff },
    { "pantographlowerall", command_target::vehicle, command_mode::oneoff },
    { "heatingtoggle", command_target::vehicle, command_mode::oneoff },
/*
// const int k_FreeFlyMode= 59;
*/
    { "lightspresetactivatenext", command_target::vehicle, command_mode::oneoff },
    { "lightspresetactivateprevious", command_target::vehicle, command_mode::oneoff },
    { "headlighttoggleleft", command_target::vehicle, command_mode::oneoff },
    { "headlighttoggleright", command_target::vehicle, command_mode::oneoff },
    { "headlighttoggleupper", command_target::vehicle, command_mode::oneoff },
    { "redmarkertoggleleft", command_target::vehicle, command_mode::oneoff },
    { "redmarkertoggleright", command_target::vehicle, command_mode::oneoff },
    { "headlighttogglerearleft", command_target::vehicle, command_mode::oneoff },
    { "headlighttogglerearright", command_target::vehicle, command_mode::oneoff },
    { "headlighttogglerearupper", command_target::vehicle, command_mode::oneoff },
    { "redmarkertogglerearleft", command_target::vehicle, command_mode::oneoff },
    { "redmarkertogglerearright", command_target::vehicle, command_mode::oneoff },
    { "headlightsdimtoggle", command_target::vehicle, command_mode::oneoff },
    { "motorconnectorsopen", command_target::vehicle, command_mode::oneoff },
    { "motordisconnect", command_target::vehicle, command_mode::oneoff },
    { "interiorlighttoggle", command_target::vehicle, command_mode::oneoff },
    { "interiorlightdimtoggle", command_target::vehicle, command_mode::oneoff },
    { "instrumentlighttoggle", command_target::vehicle, command_mode::oneoff },
/*
const int k_EndSign = 70;
const int k_Active = 71;
*/
    { "generictoggle0", command_target::vehicle, command_mode::oneoff },
    { "generictoggle1", command_target::vehicle, command_mode::oneoff },
    { "generictoggle2", command_target::vehicle, command_mode::oneoff },
    { "generictoggle3", command_target::vehicle, command_mode::oneoff },
    { "generictoggle4", command_target::vehicle, command_mode::oneoff },
    { "generictoggle5", command_target::vehicle, command_mode::oneoff },
    { "generictoggle6", command_target::vehicle, command_mode::oneoff },
    { "generictoggle7", command_target::vehicle, command_mode::oneoff },
    { "generictoggle8", command_target::vehicle, command_mode::oneoff },
    { "generictoggle9", command_target::vehicle, command_mode::oneoff },
    { "batterytoggle", command_target::vehicle }
/*
const int k_WalkMode = 73;
*/
};

}

void command_queue::update()
{
	double delta = Timer::GetDeltaTime();
	for (user_command c : m_active_continuous)
	{
	    auto const &desc = simulation::Commands_descriptions[ static_cast<std::size_t>( c ) ];
		command_data data { c, GLFW_REPEAT, 0, 0, command_data::desired_state_t::TOGGLE, delta };
	    auto lookup = m_commands.emplace((size_t)desc.target, commanddata_sequence() );
	    lookup.first->second.emplace( data );
	}
}

// posts specified command for specified recipient
void
command_queue::push( command_data const &Command, std::size_t const Recipient ) {
    auto const &desc = simulation::Commands_descriptions[ static_cast<std::size_t>( Command.command ) ];
	if (desc.mode == command_mode::continuous)
	{
		if (Command.action == GLFW_PRESS)
			m_active_continuous.emplace(Command.command);
		else if (Command.action == GLFW_RELEASE)
			m_active_continuous.erase(Command.command);
	}
	else
	{
	    auto lookup = m_commands.emplace( Recipient, commanddata_sequence() );
	    // recipient stack was either located or created, so we can add to it quite safely
	    lookup.first->second.emplace( Command );
	}
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
            state, Timer::GetDeltaTime()},
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
