/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "keyboardinput.h"
#include "logs.h"

void
keyboard_input::recall_bindings() {

    // TODO: implement

    bind();
}

bool
keyboard_input::key( int const Key, int const Action ) {

    bool modifier( false );

    if( ( Key == GLFW_KEY_LEFT_SHIFT ) || ( Key == GLFW_KEY_RIGHT_SHIFT ) ) {
        // update internal state, but don't bother passing these
        m_shift =
            ( Action == GLFW_RELEASE ?
                false :
                true );
        modifier = true;
    }
    if( ( Key == GLFW_KEY_LEFT_CONTROL ) || ( Key == GLFW_KEY_RIGHT_CONTROL ) ) {
        // update internal state, but don't bother passing these
        m_ctrl =
            ( Action == GLFW_RELEASE ?
                false :
                true );
        modifier = true;
    }
    if( ( Key == GLFW_KEY_LEFT_ALT ) || ( Key == GLFW_KEY_RIGHT_ALT ) ) {
        // currently we have no interest in these whatsoever
        return false;
    }

    // include active modifiers for currently pressed key, except if the key is a modifier itself
    auto const key =
        Key
        | ( modifier ? 0 : ( m_shift ? keymodifier::shift   : 0 ) )
        | ( modifier ? 0 : ( m_ctrl  ? keymodifier::control : 0 ) );

    auto const lookup = m_bindings.find( key );
    if( lookup == m_bindings.end() ) {
        // no binding for this key
        return false;
    }
    // NOTE: basic keyboard controls don't have any parameters
    // as we haven't yet implemented either item id system or multiplayer, the 'local' controlled vehicle and entity have temporary ids of 0
    // TODO: pass correct entity id once the missing systems are in place
    m_relay.post( lookup->second, 0, 0, Action, 0 );

    return true;
}

void
keyboard_input::mouse( double Mousex, double Mousey ) {

    m_relay.post(
        user_command::viewturn,
        reinterpret_cast<std::uint64_t const &>( Mousex ),
        reinterpret_cast<std::uint64_t const &>( Mousey ),
        GLFW_PRESS,
        // as we haven't yet implemented either item id system or multiplayer, the 'local' controlled vehicle and entity have temporary ids of 0
        // TODO: pass correct entity id once the missing systems are in place
        0 );
}

void
keyboard_input::default_bindings() {

    m_commands = {

        { "mastercontrollerincrease", command_target::vehicle, GLFW_KEY_KP_ADD },
        { "mastercontrollerincreasefast", command_target::vehicle, GLFW_KEY_KP_ADD | keymodifier::shift },
        { "mastercontrollerdecrease", command_target::vehicle, GLFW_KEY_KP_SUBTRACT },
        { "mastercontrollerdecreasefast", command_target::vehicle, GLFW_KEY_KP_SUBTRACT | keymodifier::shift },
        { "secondcontrollerincrease", command_target::vehicle, GLFW_KEY_KP_DIVIDE },
        { "secondcontrollerincreasefast", command_target::vehicle, GLFW_KEY_KP_DIVIDE | keymodifier::shift },
        { "secondcontrollerdecrease", command_target::vehicle, GLFW_KEY_KP_MULTIPLY },
        { "secondcontrollerdecreasefast", command_target::vehicle, GLFW_KEY_KP_MULTIPLY | keymodifier::shift },
        { "independentbrakeincrease", command_target::vehicle, GLFW_KEY_KP_1 },
        { "independentbrakeincreasefast", command_target::vehicle, GLFW_KEY_KP_1 | keymodifier::shift },
        { "independentbrakedecrease", command_target::vehicle, GLFW_KEY_KP_7 },
        { "independentbrakedecreasefast", command_target::vehicle, GLFW_KEY_KP_7 | keymodifier::shift },
        { "independentbrakebailoff", command_target::vehicle, GLFW_KEY_KP_4 },
        { "trainbrakeincrease", command_target::vehicle, GLFW_KEY_KP_3 },
        { "trainbrakedecrease", command_target::vehicle, GLFW_KEY_KP_9 },
        { "trainbrakecharging", command_target::vehicle, GLFW_KEY_KP_DECIMAL },
        { "trainbrakerelease", command_target::vehicle, GLFW_KEY_KP_6 },
        { "trainbrakefirstservice", command_target::vehicle, GLFW_KEY_KP_8 },
        { "trainbrakeservice", command_target::vehicle, GLFW_KEY_KP_5 },
        { "trainbrakefullservice", command_target::vehicle, GLFW_KEY_KP_2 },
        { "trainbrakeemergency", command_target::vehicle, GLFW_KEY_KP_0 },
/*
const int k_AntiSlipping = 21;
const int k_Sand = 22;
const int k_Main = 23;
*/
        { "reverserincrease", command_target::vehicle, GLFW_KEY_D },
        { "reverserdecrease", command_target::vehicle, GLFW_KEY_R },
/*
const int k_Fuse = 26;
const int k_Compressor = 27;
const int k_Converter = 28;
const int k_MaxCurrent = 29;
const int k_CurrentAutoRelay = 30;
const int k_BrakeProfile = 31;
const int k_Czuwak = 32;
const int k_Horn = 33;
const int k_Horn2 = 34;
const int k_FailedEngineCutOff = 35;
*/
        { "viewturn", command_target::entity, -1 },
        { "movevector", command_target::entity, -1 },
        { "moveleft", command_target::entity, GLFW_KEY_LEFT },
        { "moveright", command_target::entity, GLFW_KEY_RIGHT },
        { "moveforward", command_target::entity, GLFW_KEY_UP },
        { "moveback", command_target::entity, GLFW_KEY_DOWN },
        { "moveup", command_target::entity, GLFW_KEY_PAGE_UP },
        { "movedown", command_target::entity, GLFW_KEY_PAGE_DOWN },
        { "moveleftfast", command_target::entity, GLFW_KEY_LEFT | keymodifier::shift },
        { "moverightfast", command_target::entity, GLFW_KEY_RIGHT | keymodifier::shift },
        { "moveforwardfast", command_target::entity, GLFW_KEY_UP | keymodifier::shift },
        { "movebackfast", command_target::entity, GLFW_KEY_DOWN | keymodifier::shift },
        { "moveupfast", command_target::entity, GLFW_KEY_PAGE_UP | keymodifier::shift },
        { "movedownfast", command_target::entity, GLFW_KEY_PAGE_DOWN | keymodifier::shift }
/*
        { "moveleftfastest", command_target::entity, GLFW_KEY_LEFT | keymodifier::control },
        { "moverightfastest", command_target::entity, GLFW_KEY_RIGHT | keymodifier::control },
        { "moveforwardfastest", command_target::entity, GLFW_KEY_UP | keymodifier::control },
        { "movebackfastest", command_target::entity, GLFW_KEY_DOWN | keymodifier::control },
        { "moveupfastest", command_target::entity, GLFW_KEY_PAGE_UP | keymodifier::control },
        { "movedownfastest", command_target::entity, GLFW_KEY_PAGE_DOWN | keymodifier::control }
*/
/*
const int k_CabForward = 42;
const int k_CabBackward = 43;
const int k_Couple = 44;
const int k_DeCouple = 45;
const int k_ProgramQuit = 46;
// const int k_ProgramPause= 47;
const int k_ProgramHelp = 48;
const int k_OpenLeft = 49;
const int k_OpenRight = 50;
const int k_CloseLeft = 51;
const int k_CloseRight = 52;
const int k_DepartureSignal = 53;
const int k_PantFrontUp = 54;
const int k_PantRearUp = 55;
const int k_PantFrontDown = 56;
const int k_PantRearDown = 57;
const int k_Heating = 58;
// const int k_FreeFlyMode= 59;
const int k_LeftSign = 60;
const int k_UpperSign = 61;
const int k_RightSign = 62;
const int k_SmallCompressor = 63;
const int k_StLinOff = 64;
const int k_CurrentNext = 65;
const int k_Univ1 = 66;
const int k_Univ2 = 67;
const int k_Univ3 = 68;
const int k_Univ4 = 69;
const int k_EndSign = 70;
const int k_Active = 71;
const int k_Battery = 72;
const int k_WalkMode = 73;
int const k_DimHeadlights = 74;
*/
    };

    bind();
}

void
keyboard_input::bind() {

    m_bindings.clear();

    int commandcode{ 0 };
    for( auto const &command : m_commands ) {

        if( command.binding != -1 ) {
            m_bindings.emplace(
                command.binding,
                static_cast<user_command>( commandcode ) );
        }
        ++commandcode;
    }
}

//---------------------------------------------------------------------------
