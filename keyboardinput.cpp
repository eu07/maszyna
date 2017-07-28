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
#include "Logs.h"
#include "parser.h"

bool
keyboard_input::recall_bindings() {

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
        // numpad block
        { "num_/", GLFW_KEY_KP_DIVIDE }, { "num_*", GLFW_KEY_KP_MULTIPLY }, { "num_-", GLFW_KEY_KP_SUBTRACT },
        { "num_7", GLFW_KEY_KP_7 }, { "num_8", GLFW_KEY_KP_8 }, { "num_9", GLFW_KEY_KP_9 }, { "num_+", GLFW_KEY_KP_ADD },
        { "num_4", GLFW_KEY_KP_4 }, { "num_5", GLFW_KEY_KP_5 }, { "num_6", GLFW_KEY_KP_6 },
        { "num_1", GLFW_KEY_KP_1 }, { "num_2", GLFW_KEY_KP_2 }, { "num_3", GLFW_KEY_KP_3 }, { "num_enter", GLFW_KEY_KP_ENTER },
        { "num_0", GLFW_KEY_KP_0 }, { "num_.", GLFW_KEY_KP_DECIMAL }
    };

    cParser bindingparser( "eu07_input-keyboard.ini", cParser::buffer_FILE );
    if( false == bindingparser.ok() ) {
        return false;
    }

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
                        m_commands.at( static_cast<std::size_t>( lookup->second ) ).binding = binding;
                    }
                }
            }
        }
    }

    bind();

    return true;
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
        // mastercontrollerincrease
        { GLFW_KEY_KP_ADD },
        // mastercontrollerincreasefast
        { GLFW_KEY_KP_ADD | keymodifier::shift },
        // mastercontrollerdecrease
        { GLFW_KEY_KP_SUBTRACT },
        // mastercontrollerdecreasefast
        { GLFW_KEY_KP_SUBTRACT | keymodifier::shift },
        // secondcontrollerincrease
        { GLFW_KEY_KP_DIVIDE },
        // secondcontrollerincreasefast
        { GLFW_KEY_KP_DIVIDE | keymodifier::shift },
        // secondcontrollerdecrease
        { GLFW_KEY_KP_MULTIPLY },
        // secondcontrollerdecreasefast
        { GLFW_KEY_KP_MULTIPLY | keymodifier::shift },
        // mucurrentindicatorothersourceactivate
        { GLFW_KEY_Z | keymodifier::shift },
        // independentbrakeincrease
        { GLFW_KEY_KP_1 },
        // independentbrakeincreasefast
        { GLFW_KEY_KP_1 | keymodifier::shift },
        // independentbrakedecrease
        { GLFW_KEY_KP_7 },
        // independentbrakedecreasefast
        { GLFW_KEY_KP_7 | keymodifier::shift },
        // independentbrakebailoff
        { GLFW_KEY_KP_4 },
        // trainbrakeincrease
        { GLFW_KEY_KP_3 },
        // trainbrakedecrease
        { GLFW_KEY_KP_9 },
        // trainbrakecharging
        { GLFW_KEY_KP_DECIMAL },
        // trainbrakerelease
        { GLFW_KEY_KP_6 },
        // trainbrakefirstservice
        { GLFW_KEY_KP_8 },
        // trainbrakeservice
        { GLFW_KEY_KP_5 },
        // trainbrakefullservice
        { GLFW_KEY_KP_2 },
        // trainbrakeemergency
        { GLFW_KEY_KP_0 },
        // wheelspinbrakeactivate,
        { GLFW_KEY_KP_ENTER },
        // sandboxactivate,
        { GLFW_KEY_S },
        // reverserincrease
        { GLFW_KEY_D },
        // reverserdecrease
        { GLFW_KEY_R },
        // linebreakertoggle
        { GLFW_KEY_M },
        // convertertoggle
        { GLFW_KEY_X },
        // convertertogglelocal
        { GLFW_KEY_X | keymodifier::shift },
        // converteroverloadrelayreset
        { GLFW_KEY_N | keymodifier::control },
        // compressortoggle
        { GLFW_KEY_C },
        // compressortoggleloal
        { GLFW_KEY_C | keymodifier::shift },
        // motoroverloadrelaythresholdtoggle
        { GLFW_KEY_F },
        // motoroverloadrelayreset
        { GLFW_KEY_N },
        // notchingrelaytoggle
        { GLFW_KEY_G },
        // epbrakecontroltoggle
        { GLFW_KEY_Z | keymodifier::control },
        // brakeactingspeedincrease
        { GLFW_KEY_B | keymodifier::shift },
        // brakeactingspeeddecrease
        { GLFW_KEY_B },
        // mubrakingindicatortoggle
        { GLFW_KEY_L | keymodifier::shift },
        // alerteracknowledge
        { GLFW_KEY_SPACE },
        // hornlowactivate
        { GLFW_KEY_A },
        // hornhighactivate
        { GLFW_KEY_A | keymodifier::shift },
        // radiotoggle
        { GLFW_KEY_R | keymodifier::control },
        // viewturn
        { -1 },
        // movevector
        { -1 },
        // moveleft
        { GLFW_KEY_LEFT },
        // moveright
        { GLFW_KEY_RIGHT },
        // moveforward
        { GLFW_KEY_UP },
        // moveback
        { GLFW_KEY_DOWN },
        // moveup
        { GLFW_KEY_PAGE_UP },
        // movedown
        { GLFW_KEY_PAGE_DOWN },
        // moveleftfast
        { -1 },
        // moverightfast
        { -1 },
        // moveforwardfast
        { -1 },
        // movebackfast
        { -1 },
        // moveupfast
        { -1 },
        // movedownfast
        { -1 },
/*
const int k_CabForward = 42;
const int k_CabBackward = 43;
const int k_Couple = 44;
const int k_DeCouple = 45;
const int k_ProgramQuit = 46;
// const int k_ProgramPause= 47;
const int k_ProgramHelp = 48;
*/
        // doortoggleleft
        { GLFW_KEY_COMMA },
        // doortoggleright
        { GLFW_KEY_PERIOD },
        // departureannounce
        { GLFW_KEY_SLASH },
        // doorlocktoggle
        { GLFW_KEY_S | keymodifier::shift },
        // pantographcompressorvalvetoggle
        { GLFW_KEY_V | keymodifier::control },
        // pantographcompressoractivate
        { GLFW_KEY_V | keymodifier::shift },
        // pantographtogglefront
        { GLFW_KEY_P },
        // pantographtogglerear
        { GLFW_KEY_O },
        // pantographlowerall
        { GLFW_KEY_P | keymodifier::control },
        // heatingtoggle
        { GLFW_KEY_H },
/*
// const int k_FreeFlyMode= 59;
*/
        // headlighttoggleleft
        { GLFW_KEY_Y },
        // headlighttoggleright
        { GLFW_KEY_I },
        // headlighttoggleupper
        { GLFW_KEY_U },
        // redmarkertoggleleft
        { GLFW_KEY_Y | keymodifier::shift },
        // redmarkertoggleright
        { GLFW_KEY_I | keymodifier::shift },
        // headlighttogglerearleft
        { GLFW_KEY_Y | keymodifier::control },
        // headlighttogglerearright
        { GLFW_KEY_I | keymodifier::control },
        // headlighttogglerearupper
        { GLFW_KEY_U | keymodifier::control },
        // redmarkertogglerearleft
        { GLFW_KEY_Y | keymodifier::control | keymodifier::shift },
        // redmarkertogglerearright
        { GLFW_KEY_I | keymodifier::control | keymodifier::shift },
        // headlightsdimtoggle
        { GLFW_KEY_L | keymodifier::control },
        // motorconnectorsopen
        { GLFW_KEY_L },
        // motordisconnect
        { GLFW_KEY_E | keymodifier::shift },
        // interiorlighttoggle
        { GLFW_KEY_APOSTROPHE },
        // interiorlightdimtoggle
        { GLFW_KEY_APOSTROPHE | keymodifier::control },
        // instrumentlighttoggle
        { GLFW_KEY_SEMICOLON },
/*
const int k_Univ1 = 66;
const int k_Univ2 = 67;
const int k_Univ3 = 68;
const int k_Univ4 = 69;
const int k_EndSign = 70;
const int k_Active = 71;
*/
        // "batterytoggle"
        { GLFW_KEY_J }
/*
const int k_WalkMode = 73;
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
