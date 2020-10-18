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

std::unordered_map<int, std::string> keyboard_input::keytonamemap =
{
    { GLFW_KEY_0, "0" },
    { GLFW_KEY_1, "1" },
    { GLFW_KEY_2, "2" },
    { GLFW_KEY_3, "3" },
    { GLFW_KEY_4, "4" },
    { GLFW_KEY_5, "5" },
    { GLFW_KEY_6, "6" },
    { GLFW_KEY_7, "7" },
    { GLFW_KEY_8, "8" },
    { GLFW_KEY_9, "9" },
    { GLFW_KEY_MINUS, "-" },
    { GLFW_KEY_EQUAL, "=" },
    { GLFW_KEY_A, "a" },
    { GLFW_KEY_B, "b" },
    { GLFW_KEY_C, "c" },
    { GLFW_KEY_D, "d" },
    { GLFW_KEY_E, "e" },
    { GLFW_KEY_F, "f" },
    { GLFW_KEY_G, "g" },
    { GLFW_KEY_H, "h" },
    { GLFW_KEY_I, "i" },
    { GLFW_KEY_J, "j" },
    { GLFW_KEY_K, "k" },
    { GLFW_KEY_L, "l" },
    { GLFW_KEY_M, "m" },
    { GLFW_KEY_N, "n" },
    { GLFW_KEY_O, "o" },
    { GLFW_KEY_P, "p" },
    { GLFW_KEY_Q, "q" },
    { GLFW_KEY_R, "r" },
    { GLFW_KEY_S, "s" },
    { GLFW_KEY_T, "t" },
    { GLFW_KEY_U, "u" },
    { GLFW_KEY_V, "v" },
    { GLFW_KEY_W, "w" },
    { GLFW_KEY_X, "x" },
    { GLFW_KEY_Y, "y" },
    { GLFW_KEY_Z, "z" },
    { GLFW_KEY_MINUS, "-" },
    { GLFW_KEY_EQUAL, "=" },
    { GLFW_KEY_BACKSPACE, "backspace" },
    { GLFW_KEY_LEFT_BRACKET, "[" },
    { GLFW_KEY_RIGHT_BRACKET, "]" },
    { GLFW_KEY_BACKSLASH, "\\" },
    { GLFW_KEY_SEMICOLON, ";" },
    { GLFW_KEY_APOSTROPHE, "'" },
    { GLFW_KEY_ENTER, "enter" },
    { GLFW_KEY_COMMA, "," },
    { GLFW_KEY_PERIOD, "." },
    { GLFW_KEY_SLASH, "/" },
    { GLFW_KEY_SPACE, "space" },
    { GLFW_KEY_PAUSE, "pause" },
    { GLFW_KEY_INSERT, "insert" },
    { GLFW_KEY_DELETE, "delete" },
    { GLFW_KEY_HOME, "home" },
    { GLFW_KEY_END, "end" },
    { GLFW_KEY_KP_DIVIDE, "num_/" },
    { GLFW_KEY_KP_MULTIPLY, "num_*" },
    { GLFW_KEY_KP_SUBTRACT, "num_-" },
    { GLFW_KEY_KP_7, "num_7" },
    { GLFW_KEY_KP_8, "num_8" },
    { GLFW_KEY_KP_9, "num_9" },
    { GLFW_KEY_KP_ADD, "num_+" },
    { GLFW_KEY_KP_4, "num_4" },
    { GLFW_KEY_KP_5, "num_5" },
    { GLFW_KEY_KP_6, "num_6" },
    { GLFW_KEY_KP_1, "num_1" },
    { GLFW_KEY_KP_2, "num_2" },
    { GLFW_KEY_KP_3, "num_3" },
    { GLFW_KEY_KP_ENTER, "num_enter" },
    { GLFW_KEY_KP_0, "num_0" },
    { GLFW_KEY_KP_DECIMAL, "num_." },
    { GLFW_KEY_F1, "f1" },
    { GLFW_KEY_F2, "f2" },
    { GLFW_KEY_F3, "f3" },
    { GLFW_KEY_F4, "f4" },
    { GLFW_KEY_F5, "f5" },
    { GLFW_KEY_F6, "f6" },
    { GLFW_KEY_F7, "f7" },
    { GLFW_KEY_F8, "f8" },
    { GLFW_KEY_F9, "f9" },
    { GLFW_KEY_F10, "f10" },
    { GLFW_KEY_F11, "f11" },
    { GLFW_KEY_F12, "f12" },
    { GLFW_KEY_TAB, "tab" },
    { GLFW_KEY_ESCAPE, "esc" },
    { GLFW_KEY_LEFT, "left" },
    { GLFW_KEY_RIGHT, "right" },
    { GLFW_KEY_UP, "up" },
    { GLFW_KEY_DOWN, "down" },
    { GLFW_KEY_PAGE_UP, "page_up" },
    { GLFW_KEY_PAGE_DOWN, "page_down" },
};

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
	std::unordered_map<std::string, int> nametokeymap;

	for (const std::pair<int, std::string> &key : keytonamemap) {
		nametokeymap.emplace(key.second, key.first);
	}

    // NOTE: to simplify things we expect one entry per line, and whole entry in one line
    while( true == bindingparser.getTokens( 1, true, "\n\r" ) ) {

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
						m_bindingsetups.insert_or_assign(lookup->second, binding);
                    }
                }
            }
        }
    }

    return true;
}

void keyboard_input::dump_bindings()
{
	std::fstream stream("eu07_input-keyboard.ini",
	             std::ios_base::binary | std::ios_base::trunc | std::ios_base::out);

	if (!stream.is_open()) {
		ErrorLog("failed to save keyboard config");
		return;
	}

	for (const std::pair<user_command, int> &binding : m_bindingsetups) {
		stream << simulation::Commands_descriptions[static_cast<std::size_t>(binding.first)].name << ' ';

		auto it = keytonamemap.find(binding.second & 0xFFFF);
		if (it != keytonamemap.end()) {
			if (binding.second & keymodifier::control)
				stream << "ctrl ";
			if (binding.second & keymodifier::shift)
				stream << "shift ";

			stream << it->second << "\r\n";
		} else {
			stream << "none\r\n";
		}
	}
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
	m_bindings.clear();

    for( auto const &bindingsetup : m_bindingsetups ) {

        m_bindings[ bindingsetup.second ] = bindingsetup.first;
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

std::unordered_map<int, std::string> keytonamemap = {
    { GLFW_KEY_0, "0" }, { GLFW_KEY_1, "1" }, { GLFW_KEY_2, "2" }, { GLFW_KEY_3, "3" }, { GLFW_KEY_4, "4" },
    { GLFW_KEY_5, "5" }, { GLFW_KEY_6, "6" }, { GLFW_KEY_7, "7" }, { GLFW_KEY_8, "8" }, { GLFW_KEY_9, "9" },
    { GLFW_KEY_MINUS, "-" }, { GLFW_KEY_EQUAL, "=" },
    { GLFW_KEY_A, "A" }, { GLFW_KEY_B, "B" }, { GLFW_KEY_C, "C" }, { GLFW_KEY_D, "D" }, { GLFW_KEY_E, "E" },
    { GLFW_KEY_F, "F" }, { GLFW_KEY_G, "G" }, { GLFW_KEY_H, "H" }, { GLFW_KEY_I, "I" }, { GLFW_KEY_J, "J" },
    { GLFW_KEY_K, "K" }, { GLFW_KEY_L, "L" }, { GLFW_KEY_M, "M" }, { GLFW_KEY_N, "N" }, { GLFW_KEY_O, "O" },
    { GLFW_KEY_P, "P" }, { GLFW_KEY_Q, "Q" }, { GLFW_KEY_R, "R" }, { GLFW_KEY_S, "S" }, { GLFW_KEY_T, "T" },
    { GLFW_KEY_U, "U" }, { GLFW_KEY_V, "V" }, { GLFW_KEY_W, "W" }, { GLFW_KEY_X, "X" }, { GLFW_KEY_Y, "Y" }, { GLFW_KEY_Z, "Z" },
    { GLFW_KEY_BACKSPACE, "BACKSPACE" },
    { GLFW_KEY_LEFT_BRACKET, "[" }, { GLFW_KEY_RIGHT_BRACKET, "]" }, { GLFW_KEY_BACKSLASH, "\\" },
    { GLFW_KEY_SEMICOLON, ";" }, { GLFW_KEY_APOSTROPHE, "'" }, { GLFW_KEY_ENTER, "ENTER" },
    { GLFW_KEY_COMMA, "<" }, { GLFW_KEY_PERIOD, ">" }, { GLFW_KEY_SLASH, "/" },
    { GLFW_KEY_SPACE, "SPACE" },
    { GLFW_KEY_PAUSE, "PAUSE" }, { GLFW_KEY_INSERT, "INSERT" }, { GLFW_KEY_DELETE, "DELETE" }, { GLFW_KEY_HOME, "HOME" }, { GLFW_KEY_END, "END" },
    // numpad block
    { GLFW_KEY_KP_DIVIDE, "NUM /" }, { GLFW_KEY_KP_MULTIPLY, "NUM *" }, { GLFW_KEY_KP_SUBTRACT, "NUM -" },
    { GLFW_KEY_KP_7, "NUM 7" }, { GLFW_KEY_KP_8, "NUM 8" }, { GLFW_KEY_KP_9, "NUM 9" }, { GLFW_KEY_KP_ADD, "NUM +" },
    { GLFW_KEY_KP_4, "NUM 4" }, { GLFW_KEY_KP_5, "NUM 5" }, { GLFW_KEY_KP_6, "NUM 6" },
    { GLFW_KEY_KP_1, "NUM 1" }, { GLFW_KEY_KP_2, "NUM 2" }, { GLFW_KEY_KP_3, "NUM 3" }, { GLFW_KEY_KP_ENTER, "NUM ENTER" },
    { GLFW_KEY_KP_0, "NUM 0" }, { GLFW_KEY_KP_DECIMAL, "NUM ." }
};

std::string
keyboard_input::binding_hint( user_command const Command ) const {

    if( Command == user_command::none ) { return ""; }

    auto const binding { this->binding( Command ) };
    if( binding == -1 ) { return ""; }

    auto const lookup { keytonamemap.find( binding & 0xffff ) };
    if( lookup == keytonamemap.end() ) { return ""; }

    std::string hint;
    if( ( binding & keymodifier::shift ) != 0 ) {
        hint += "SHIFT ";
    }
    if( ( binding & keymodifier::control ) != 0 ) {
        hint += "CTRL ";
    }
    hint += lookup->second;

    return hint;
}

//---------------------------------------------------------------------------
