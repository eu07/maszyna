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
#include "Globals.h"
#include "Logs.h"
#include "parser.h"

namespace input {

std::array<char, GLFW_KEY_LAST + 1> keys { GLFW_RELEASE };
bool key_alt;
bool key_ctrl;
bool key_shift;

}

bool
keyboard_input::recall_bindings() {

    cParser bindingparser( "eu07_input-keyboard.ini", cParser::buffer_FILE );
    if( false == bindingparser.ok() ) {
        return false;
    }

    // build helper translation tables
    std::unordered_map<std::string, user_command> nametocommandmap;
    std::size_t commandid = 0;
    for( auto const &description : simulation::Commands_descriptions ) {
        nametocommandmap.emplace(
            description.name,
            static_cast<user_command>( commandid ) );
        ++commandid;
    }
    std::unordered_map<std::string, int> nametokeymap = {
        { "0", GLFW_KEY_0 }, { "1", GLFW_KEY_1 }, { "2", GLFW_KEY_2 }, { "3", GLFW_KEY_3 }, { "4", GLFW_KEY_4 },
        { "5", GLFW_KEY_5 }, { "6", GLFW_KEY_6 }, { "7", GLFW_KEY_7 }, { "8", GLFW_KEY_8 }, { "9", GLFW_KEY_9 },
        { "-", GLFW_KEY_MINUS }, { "=", GLFW_KEY_EQUAL },
        { "a", GLFW_KEY_A }, { "b", GLFW_KEY_B }, { "c", GLFW_KEY_C }, { "d", GLFW_KEY_D }, { "e", GLFW_KEY_E },
        { "f", GLFW_KEY_F }, { "g", GLFW_KEY_G }, { "h", GLFW_KEY_H }, { "i", GLFW_KEY_I }, { "j", GLFW_KEY_J },
        { "k", GLFW_KEY_K }, { "l", GLFW_KEY_L }, { "m", GLFW_KEY_M }, { "n", GLFW_KEY_N }, { "o", GLFW_KEY_O },
        { "p", GLFW_KEY_P }, { "q", GLFW_KEY_Q }, { "r", GLFW_KEY_R }, { "s", GLFW_KEY_S }, { "t", GLFW_KEY_T },
        { "u", GLFW_KEY_U }, { "v", GLFW_KEY_V }, { "w", GLFW_KEY_W }, { "x", GLFW_KEY_X }, { "y", GLFW_KEY_Y }, { "z", GLFW_KEY_Z },
        { "-", GLFW_KEY_MINUS }, { "=", GLFW_KEY_EQUAL }, { "backspace", GLFW_KEY_BACKSPACE },
        { "[", GLFW_KEY_LEFT_BRACKET }, { "]", GLFW_KEY_RIGHT_BRACKET }, { "\\", GLFW_KEY_BACKSLASH },
        { ";", GLFW_KEY_SEMICOLON }, { "'", GLFW_KEY_APOSTROPHE }, { "enter", GLFW_KEY_ENTER },
        { ",", GLFW_KEY_COMMA }, { ".", GLFW_KEY_PERIOD }, { "/", GLFW_KEY_SLASH },
        { "space", GLFW_KEY_SPACE },
        { "pause", GLFW_KEY_PAUSE }, { "insert", GLFW_KEY_INSERT }, { "delete", GLFW_KEY_DELETE }, { "home", GLFW_KEY_HOME }, { "end", GLFW_KEY_END },
        // numpad block
        { "num_/", GLFW_KEY_KP_DIVIDE }, { "num_*", GLFW_KEY_KP_MULTIPLY }, { "num_-", GLFW_KEY_KP_SUBTRACT },
        { "num_7", GLFW_KEY_KP_7 }, { "num_8", GLFW_KEY_KP_8 }, { "num_9", GLFW_KEY_KP_9 }, { "num_+", GLFW_KEY_KP_ADD },
        { "num_4", GLFW_KEY_KP_4 }, { "num_5", GLFW_KEY_KP_5 }, { "num_6", GLFW_KEY_KP_6 },
        { "num_1", GLFW_KEY_KP_1 }, { "num_2", GLFW_KEY_KP_2 }, { "num_3", GLFW_KEY_KP_3 }, { "num_enter", GLFW_KEY_KP_ENTER },
        { "num_0", GLFW_KEY_KP_0 }, { "num_.", GLFW_KEY_KP_DECIMAL }
    };

    // NOTE: to simplify things we expect one entry per line, and whole entry in one line
    while( true == bindingparser.getTokens( 1, true, "\n" ) ) {

        std::string bindingentry;
        bindingparser >> bindingentry;
        cParser entryparser( bindingentry );
        if( true == entryparser.getTokens( 1, true, "\n\r\t " ) ) {

            std::string commandname;
            entryparser >> commandname;

            auto const lookup = nametocommandmap.find( commandname );
            if( lookup == nametocommandmap.end() ) {

                WriteLog( "Keyboard binding defined for unknown command, \"" + commandname + "\"" );
            }
            else {
                int binding{ 0 };
                while( entryparser.getTokens( 1, true, "\n\r\t " ) ) {

                    std::string bindingkeyname;
                    entryparser >> bindingkeyname;

                         if( bindingkeyname == "shift" ) { binding |= keymodifier::shift; }
                    else if( bindingkeyname == "ctrl" )  { binding |= keymodifier::control; }
                    else if( bindingkeyname == "none" )  { binding = 0; }
                    else {
                        // regular key, convert it to glfw key code
                        auto const keylookup = nametokeymap.find( bindingkeyname );
                        if( keylookup == nametokeymap.end() ) {

                            WriteLog( "Keyboard binding included unrecognized key, \"" + bindingkeyname + "\"" );
                        }
                        else {
                            // replace any existing binding, preserve modifiers
                            // (protection from cases where there's more than one key listed in the entry)
                            binding = keylookup->second | ( binding & 0xffff0000 );
                        }
                    }

                    if( ( binding & 0xffff ) != 0 ) {
                        m_bindingsetups.emplace_back( binding_setup{ lookup->second, binding } );
                    }
                }
            }
        }
    }

    return true;
}

bool
keyboard_input::key( int const Key, int const Action ) {

    bool modifier( false );

    if( ( Key == GLFW_KEY_LEFT_SHIFT ) || ( Key == GLFW_KEY_RIGHT_SHIFT ) ) {
        // update internal state, but don't bother passing these
        input::key_shift =
            ( Action == GLFW_RELEASE ?
                false :
                true );
        modifier = true;
        // whenever shift key is used it may affect currently pressed movement keys, so check and update these
    }
    if( ( Key == GLFW_KEY_LEFT_CONTROL ) || ( Key == GLFW_KEY_RIGHT_CONTROL ) ) {
        // update internal state, but don't bother passing these
        input::key_ctrl =
            ( Action == GLFW_RELEASE ?
                false :
                true );
        modifier = true;
    }
    if( ( Key == GLFW_KEY_LEFT_ALT ) || ( Key == GLFW_KEY_RIGHT_ALT ) ) {
        // update internal state, but don't bother passing these
        input::key_alt =
            ( Action == GLFW_RELEASE ?
                false :
                true );
    }

    if( Key == -1 ) { return false; }

    // store key state
    input::keys[ Key ] = Action;

    if( true == is_movement_key( Key ) ) {
        // if the received key was one of movement keys, it's been handled and we don't need to bother further
        return true;
    }

    // include active modifiers for currently pressed key, except if the key is a modifier itself
    auto const key =
        Key
        | ( modifier ? 0 : ( input::key_shift ? keymodifier::shift   : 0 ) )
        | ( modifier ? 0 : ( input::key_ctrl  ? keymodifier::control : 0 ) );

    auto const lookup = m_bindings.find( key );
    if( lookup == m_bindings.end() ) {
        // no binding for this key
        return false;
    }
    // NOTE: basic keyboard controls don't have any parameters
    // as we haven't yet implemented either item id system or multiplayer, the 'local' controlled vehicle and entity have temporary ids of 0
    // TODO: pass correct entity id once the missing systems are in place
    m_relay.post( lookup->second, 0, 0, Action, 0 );
    m_command = (
        Action == GLFW_RELEASE ?
            user_command::none :
            lookup->second );

    return true;
}

int
keyboard_input::key( int const Key ) const {

    return input::keys[ Key ];
}

void
keyboard_input::bind() {

    for( auto const &bindingsetup : m_bindingsetups ) {

        m_bindings[ bindingsetup.binding ] = bindingsetup.command;
    }
    // cache movement key bindings
    m_bindingscache.forward = binding( user_command::moveforward );
    m_bindingscache.back = binding( user_command::moveback );
    m_bindingscache.left = binding( user_command::moveleft );
    m_bindingscache.right = binding( user_command::moveright );
    m_bindingscache.up = binding( user_command::moveup );
    m_bindingscache.down = binding( user_command::movedown );
}

int
keyboard_input::binding( user_command const Command ) const {

    for( auto const &binding : m_bindings ) {
        if( binding.second == Command ) {
            return binding.first;
        }
    }
    return -1;
}

bool
keyboard_input::is_movement_key( int const Key ) const {

    bool const ismovementkey =
        ( ( Key == m_bindingscache.forward )
       || ( Key == m_bindingscache.back )
       || ( Key == m_bindingscache.left )
       || ( Key == m_bindingscache.right )
       || ( Key == m_bindingscache.up )
       || ( Key == m_bindingscache.down ) );

    return ismovementkey;
}

void
keyboard_input::poll() {

    glm::vec2 const movementhorizontal {
        // x-axis
        ( Global.shiftState ? 1.f : (2.0f / 3.0f) ) *
        ( input::keys[ m_bindingscache.left ] != GLFW_RELEASE ? -1.f :
          input::keys[ m_bindingscache.right ] != GLFW_RELEASE ? 1.f :
          0.f ),
        // z-axis
        ( Global.shiftState ? 1.f : (2.0f / 3.0f) ) *
        ( input::keys[ m_bindingscache.forward ] != GLFW_RELEASE ? 1.f :
          input::keys[ m_bindingscache.back ] != GLFW_RELEASE ?   -1.f :
          0.f ) };

    if( (   movementhorizontal.x != 0.f ||   movementhorizontal.y != 0.f )
     || ( m_movementhorizontal.x != 0.f || m_movementhorizontal.y != 0.f ) ) {
        m_relay.post(
            ( true == Global.ctrlState ?
                user_command::movehorizontalfast :
                user_command::movehorizontal ),
            movementhorizontal.x,
            movementhorizontal.y,
            GLFW_PRESS,
            0 );
    }

    m_movementhorizontal = movementhorizontal;

    float const movementvertical {
        // y-axis
        ( Global.shiftState ? 1.f : (2.0f / 3.0f) ) *
        ( input::keys[ m_bindingscache.up ] != GLFW_RELEASE ?    1.f :
          input::keys[ m_bindingscache.down ] != GLFW_RELEASE ? -1.f :
          0.f ) };

    if( (   movementvertical != 0.f )
     || ( m_movementvertical != 0.f ) ) {
        m_relay.post(
            ( true == Global.ctrlState ?
                user_command::moveverticalfast :
                user_command::movevertical ),
            movementvertical,
            0,
            GLFW_PRESS,
            0 );
    }

    m_movementvertical = movementvertical;
}

//---------------------------------------------------------------------------
