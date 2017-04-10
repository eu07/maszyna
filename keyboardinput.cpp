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
        // whenever shift key is used it may affect currently pressed movement keys, so check and update these
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

    if( true == update_movement( Key, Action ) ) {
        // if the received key was one of movement keys, it's been handled and we don't need to bother further
        return true;
    }

    // store key state
    if( Key != -1 ) {
        m_keys[ Key ] = Action;
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
*/
        { "linebreakertoggle", command_target::vehicle, GLFW_KEY_M },
        { "reverserincrease", command_target::vehicle, GLFW_KEY_D },
        { "reverserdecrease", command_target::vehicle, GLFW_KEY_R },
/*
const int k_Fuse = 26;
*/
        { "compressortoggle", command_target::vehicle, GLFW_KEY_C },
        { "convertertoggle", command_target::vehicle, GLFW_KEY_X },
/*
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
        { "moveleftfast", command_target::entity, -1 },
        { "moverightfast", command_target::entity, -1 },
        { "moveforwardfast", command_target::entity, -1 },
        { "movebackfast", command_target::entity, -1 },
        { "moveupfast", command_target::entity, -1 },
        { "movedownfast", command_target::entity, -1 },
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
*/
        { "pantographtogglefront", command_target::vehicle, GLFW_KEY_P },
        { "pantographtogglerear", command_target::vehicle, GLFW_KEY_O },
/*
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
*/
        { "batterytoggle", command_target::vehicle, GLFW_KEY_J }
/*
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
    // cache movement key bindings, so we can test them faster in the input loop
    m_bindingscache.forward = m_commands[ static_cast<std::size_t>( user_command::moveforward ) ].binding;
    m_bindingscache.back = m_commands[ static_cast<std::size_t>( user_command::moveback ) ].binding;
    m_bindingscache.left = m_commands[ static_cast<std::size_t>( user_command::moveleft ) ].binding;
    m_bindingscache.right = m_commands[ static_cast<std::size_t>( user_command::moveright ) ].binding;
    m_bindingscache.up = m_commands[ static_cast<std::size_t>( user_command::moveup ) ].binding;
    m_bindingscache.down = m_commands[ static_cast<std::size_t>( user_command::movedown ) ].binding;
}

// NOTE: ugliest code ever, gg
bool
keyboard_input::update_movement( int const Key, int const Action ) {

    bool shift =
        ( ( Key == GLFW_KEY_LEFT_SHIFT )
       || ( Key == GLFW_KEY_RIGHT_SHIFT ) );
    bool movementkey =
        ( ( Key == m_bindingscache.forward )
       || ( Key == m_bindingscache.back )
       || ( Key == m_bindingscache.left )
       || ( Key == m_bindingscache.right )
       || ( Key == m_bindingscache.up )
       || ( Key == m_bindingscache.down ) );

    if( false == ( shift || movementkey ) ) { return false; }

    if( false == shift ) {
        // TODO: pass correct entity id once the missing systems are in place
        if( Key == m_bindingscache.forward ) {
            m_keys[ Key ] = Action;
            m_relay.post(
                ( m_shift ?
                    user_command::moveforwardfast :
                    user_command::moveforward ),
                0, 0,
                m_keys[ m_bindingscache.forward ],
                0 );
            return true;
        }
        else if( Key == m_bindingscache.back ) {
            m_keys[ Key ] = Action;
            m_relay.post(
                ( m_shift ?
                    user_command::movebackfast :
                    user_command::moveback ),
                0, 0,
                m_keys[ m_bindingscache.back ],
                0 );
            return true;
        }
        else if( Key == m_bindingscache.left ) {
            m_keys[ Key ] = Action;
            m_relay.post(
                ( m_shift ?
                    user_command::moveleftfast :
                    user_command::moveleft ),
                0, 0,
                m_keys[ m_bindingscache.left ],
                0 );
            return true;
        }
        else if( Key == m_bindingscache.right ) {
            m_keys[ Key ] = Action;
            m_relay.post(
                ( m_shift ?
                    user_command::moverightfast :
                    user_command::moveright ),
                0, 0,
                m_keys[ m_bindingscache.right ],
                0 );
            return true;
        }
        else if( Key == m_bindingscache.up ) {
            m_keys[ Key ] = Action;
            m_relay.post(
                ( m_shift ?
                    user_command::moveupfast :
                    user_command::moveup ),
                0, 0,
                m_keys[ m_bindingscache.up ],
                0 );
            return true;
        }
        else if( Key == m_bindingscache.down ) {
            m_keys[ Key ] = Action;
            m_relay.post(
                ( m_shift ?
                    user_command::movedownfast :
                    user_command::movedown ),
                0, 0,
                m_keys[ m_bindingscache.down ],
                0 );
            return true;
        }
    }
    else {
        // if it's not the movement keys but one of shift keys, we might potentially need to update movement state
        if( m_keys[ Key ] == Action ) {
            // but not if it's just repeat
            return false;
        }
        // bit of recursion voodoo here, we fake relevant key presses so we don't have to duplicate the code from above
        if( m_keys[ m_bindingscache.forward ] != GLFW_RELEASE ) { update_movement( m_bindingscache.forward, m_keys[ m_bindingscache.forward ] ); }
        if( m_keys[ m_bindingscache.back ] != GLFW_RELEASE ) { update_movement( m_bindingscache.back, m_keys[ m_bindingscache.back ] ); }
        if( m_keys[ m_bindingscache.left ] != GLFW_RELEASE ) { update_movement( m_bindingscache.left, m_keys[ m_bindingscache.left ] ); }
        if( m_keys[ m_bindingscache.right ] != GLFW_RELEASE ) { update_movement( m_bindingscache.right, m_keys[ m_bindingscache.right ] ); }
        if( m_keys[ m_bindingscache.up ] != GLFW_RELEASE ) { update_movement( m_bindingscache.up, m_keys[ m_bindingscache.up ] ); }
        if( m_keys[ m_bindingscache.down ] != GLFW_RELEASE ) { update_movement( m_bindingscache.down, m_keys[ m_bindingscache.down ] ); }
    }

    return false;
}

//---------------------------------------------------------------------------
