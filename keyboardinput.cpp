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
#include "globals.h"
#include "logs.h"
#include "parser.h"
#include "world.h"

extern TWorld World;

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
                    else if( bindingkeyname == "none" )  { binding = -1; }
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

    // store key state
    if( Key != -1 ) {
        m_keys[ Key ] = Action;
    }

    if( true == is_movement_key( Key ) ) {
        // if the received key was one of movement keys, it's been handled and we don't need to bother further
        return true;
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
    m_command = (
        Action == GLFW_RELEASE ?
            user_command::none :
            lookup->second );

    return true;
}

void
keyboard_input::default_bindings() {

    m_commands = {
        // aidriverenable
        { GLFW_KEY_Q | keymodifier::shift },
        // aidriverdisable
        { GLFW_KEY_Q },
        // mastercontrollerincrease
        { GLFW_KEY_KP_ADD },
        // mastercontrollerincreasefast
        { GLFW_KEY_KP_ADD | keymodifier::shift },
        // mastercontrollerdecrease
        { GLFW_KEY_KP_SUBTRACT },
        // mastercontrollerdecreasefast
        { GLFW_KEY_KP_SUBTRACT | keymodifier::shift },
        // mastercontrollerset
        { -1 },
        // secondcontrollerincrease
        { GLFW_KEY_KP_DIVIDE },
        // secondcontrollerincreasefast
        { GLFW_KEY_KP_DIVIDE | keymodifier::shift },
        // secondcontrollerdecrease
        { GLFW_KEY_KP_MULTIPLY },
        // secondcontrollerdecreasefast
        { GLFW_KEY_KP_MULTIPLY | keymodifier::shift },
        // secondcontrollerset
        { -1 },
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
        // independentbrakeset
        { -1 },
        // independentbrakebailoff
        { GLFW_KEY_KP_4 },
        // trainbrakeincrease
        { GLFW_KEY_KP_3 },
        // trainbrakedecrease
        { GLFW_KEY_KP_9 },
        // trainbrakeset
        { -1 },
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
        // trainbrakehandleoff
        { GLFW_KEY_KP_5 | keymodifier::control },
        // trainbrakeemergency
        { GLFW_KEY_KP_0 },
        // trainbrakebasepressureincrease
        { GLFW_KEY_KP_3 | keymodifier::control },
        // trainbrakebasepressuredecrease
        { GLFW_KEY_KP_9 | keymodifier::control },
        // trainbrakebasepressurereset
        { GLFW_KEY_KP_6 | keymodifier::control },
        // trainbrakeoperationtoggle
        { GLFW_KEY_KP_4 | keymodifier::control },
        // manualbrakeincrease
        { GLFW_KEY_KP_1 | keymodifier::control },
        // manualbrakedecrease
        { GLFW_KEY_KP_7 | keymodifier::control },
        // alarm chain toggle
        { GLFW_KEY_B | keymodifier::shift | keymodifier::control },
        // wheelspinbrakeactivate
        { GLFW_KEY_KP_ENTER },
        // sandboxactivate
        { GLFW_KEY_S },
        // reverserincrease
        { GLFW_KEY_D },
        // reverserdecrease
        { GLFW_KEY_R },
        // reverserforwardhigh
        { -1 },
        // reverserforward
        { -1 },
        // reverserneutral
        { -1 },
        // reverserbackward
        { -1 },
        // waterpumpbreakertoggle
        { GLFW_KEY_W | keymodifier::control },
        // waterpumpbreakerclose
        { -1 },
        // waterpumpbreakeropen
        { -1 },
        // waterpumptoggle
        { GLFW_KEY_W },
        // waterpumpenable
        { -1 },
        // waterpumpdisable
        { -1 },
        // waterheaterbreakertoggle
        { GLFW_KEY_W | keymodifier::control | keymodifier::shift },
        // waterheaterbreakerclose
        { -1 },
        // waterheaterbreakeropen
        { -1 },
        // waterheatertoggle
        { GLFW_KEY_W | keymodifier::shift },
        // waterheaterenable
        { -1 },
        // waterheaterdisable
        { -1 },
        // watercircuitslinktoggle
        { GLFW_KEY_H | keymodifier::shift },
        // watercircuitslinkenable
        { -1 },
        // watercircuitslinkdisable
        { -1 },
        // fuelpumptoggle
        { GLFW_KEY_F },
        // fuelpumpenable,
        { -1 },
        // fuelpumpdisable,
        { -1 },
        // oilpumptoggle
        { GLFW_KEY_F | keymodifier::shift },
        // oilpumpenable,
        { -1 },
        // oilpumpdisable,
        { -1 },
        // linebreakertoggle
        { GLFW_KEY_M },
        // linebreakeropen
        { -1 },
        // linebreakerclose
        { -1 },
        // convertertoggle
        { GLFW_KEY_X },
        // converterenable,
        { -1 },
        // converterdisable,
        { -1 },
        // convertertogglelocal
        { GLFW_KEY_X | keymodifier::shift },
        // converteroverloadrelayreset
        { GLFW_KEY_N | keymodifier::control },
        // compressortoggle
        { GLFW_KEY_C },
        // compressorenable
        { -1 },
        // compressordisable
        { -1 },
        // compressortoggleloal
        { GLFW_KEY_C | keymodifier::shift },
        // motoroverloadrelaythresholdtoggle
        { GLFW_KEY_F },
        // motoroverloadrelaythresholdsetlow
        { -1 },
        // motoroverloadrelaythresholdsethigh
        { -1 },
        // motoroverloadrelayreset
        { GLFW_KEY_N },
        // notchingrelaytoggle
        { GLFW_KEY_G },
        // epbrakecontroltoggle
        { GLFW_KEY_Z | keymodifier::control },
		// trainbrakeoperationmodeincrease
        { GLFW_KEY_KP_2 | keymodifier::control },
		// trainbrakeoperationmodedecrease
        { GLFW_KEY_KP_8 | keymodifier::control },
        // brakeactingspeedincrease
        { GLFW_KEY_B | keymodifier::shift },
        // brakeactingspeeddecrease
        { GLFW_KEY_B },
        // brakeactingspeedsetcargo
        { -1 },
        // brakeactingspeedsetpassenger
        { -1 },
        // brakeactingspeedsetrapid
        { -1 },
        // brakeloadcompensationincrease
        { GLFW_KEY_H | keymodifier::shift | keymodifier::control },
        // brakeloadcompensationdecrease
        { GLFW_KEY_H | keymodifier::control },
        // mubrakingindicatortoggle
        { GLFW_KEY_L | keymodifier::shift },
        // alerteracknowledge
        { GLFW_KEY_SPACE },
        // hornlowactivate
        { GLFW_KEY_A },
        // hornhighactivate
        { GLFW_KEY_S },
        // whistleactivate
        { GLFW_KEY_Z },
        // radiotoggle
        { GLFW_KEY_R | keymodifier::control },
        // radiochannelincrease
        { GLFW_KEY_R | keymodifier::shift },
        // radiochanneldecrease
        { GLFW_KEY_R },
        // radiostopsend
        { GLFW_KEY_PAUSE | keymodifier::shift | keymodifier::control },
        // radiostoptest
        { GLFW_KEY_R | keymodifier::shift | keymodifier::control },
        // cabchangeforward
        { GLFW_KEY_HOME },
        // cabchangebackward
        { GLFW_KEY_END },
        // viewturn
        { -1 },
        // movehorizontal
        { -1 },
        // movehorizontalfast
        { -1 },
        // movevertical
        { -1 },
        // moveverticalfast
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
        // carcouplingincrease
        { GLFW_KEY_INSERT },
        // carcouplingdisconnect
        { GLFW_KEY_DELETE },
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
        // pantographraisefront
        { -1 },
        // pantographraiserear
        { -1 },
        // pantographlowerfront
        { -1 },
        // pantographlowerrear
        { -1 },
        // pantographlowerall
        { GLFW_KEY_P | keymodifier::control },
        // heatingtoggle
        { GLFW_KEY_H },
        // heatingenable
        { -1 },
        // heatingdisable
        { -1 },
        // lightspresetactivatenext
        { GLFW_KEY_T | keymodifier::shift },
        // lightspresetactivateprevious
        { GLFW_KEY_T },
        // headlighttoggleleft
        { GLFW_KEY_Y },
        // headlightenableleft
        { -1 },
        // headlightdisableleft
        { -1 },
        // headlighttoggleright
        { GLFW_KEY_I },
        // headlightenableright
        { -1 },
        // headlightdisableright
        { -1 },
        // headlighttoggleupper
        { GLFW_KEY_U },
        // headlightenableupper
        { -1 },
        // headlightdisableupper
        { -1 },
        // redmarkertoggleleft
        { GLFW_KEY_Y | keymodifier::shift },
        // redmarkerenableleft
        { -1 },
        // redmarkerdisableleft
        { -1 },
        // redmarkertoggleright
        { GLFW_KEY_I | keymodifier::shift },
        // redmarkerenableright
        { -1 },
        // redmarkerdisableright
        { -1 },
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
        // redmarkerstoggle
        { GLFW_KEY_E | keymodifier::shift },
        // endsignalstoggle
        { GLFW_KEY_E },
        // headlightsdimtoggle
        { GLFW_KEY_L | keymodifier::control },
        // headlightsdimenable
        { -1 },
        // headlightsdimdisable
        { -1 },
        // motorconnectorsopen
        { GLFW_KEY_L },
        // motorconnectorsclose
        { -1 },
        // motordisconnect
        { GLFW_KEY_E | keymodifier::control },
        // interiorlighttoggle
        { GLFW_KEY_APOSTROPHE },
        // interiorlightenable
        { -1 },
        // interiorlightdisable
        { -1 },
        // interiorlightdimtoggle
        { GLFW_KEY_APOSTROPHE | keymodifier::control },
        // interiorlightdimenable
        { -1 },
        // interiorlightdimdisable
        { -1 },
        // instrumentlighttoggle
        { GLFW_KEY_SEMICOLON },
        // instrumentlightenable
        { -1 },
        // instrumentlightdisable,
        { -1 },
        // "generictoggle0"
        { GLFW_KEY_0 },
        // "generictoggle1"
        { GLFW_KEY_1 },
        // "generictoggle2"
        { GLFW_KEY_2 },
        // "generictoggle3"
        { GLFW_KEY_3 },
        // "generictoggle4"
        { GLFW_KEY_4 },
        // "generictoggle5"
        { GLFW_KEY_5 },
        // "generictoggle6"
        { GLFW_KEY_6 },
        // "generictoggle7"
        { GLFW_KEY_7 },
        // "generictoggle8"
        { GLFW_KEY_8 },
        // "generictoggle9"
        { GLFW_KEY_9 },
        // "batterytoggle"
        { GLFW_KEY_J },
        // batteryenable
        { -1 },
        // batterydisable
        { -1 },
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
    // cache movement key bindings
    m_bindingscache.forward = m_commands[ static_cast<std::size_t>( user_command::moveforward ) ].binding;
    m_bindingscache.back = m_commands[ static_cast<std::size_t>( user_command::moveback ) ].binding;
    m_bindingscache.left = m_commands[ static_cast<std::size_t>( user_command::moveleft ) ].binding;
    m_bindingscache.right = m_commands[ static_cast<std::size_t>( user_command::moveright ) ].binding;
    m_bindingscache.up = m_commands[ static_cast<std::size_t>( user_command::moveup ) ].binding;
    m_bindingscache.down = m_commands[ static_cast<std::size_t>( user_command::movedown ) ].binding;
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
        ( Global.shiftState ? 1.f : 0.5f ) *
        ( m_keys[ m_bindingscache.left ] != GLFW_RELEASE ? -1.f :
          m_keys[ m_bindingscache.right ] != GLFW_RELEASE ? 1.f :
          0.f ),
        // z-axis
        ( Global.shiftState ? 1.f : 0.5f ) *
        ( m_keys[ m_bindingscache.forward ] != GLFW_RELEASE ? 1.f :
          m_keys[ m_bindingscache.back ] != GLFW_RELEASE ?   -1.f :
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
        ( Global.shiftState ? 1.f : 0.5f ) *
        ( m_keys[ m_bindingscache.up ] != GLFW_RELEASE ?    1.f :
          m_keys[ m_bindingscache.down ] != GLFW_RELEASE ? -1.f :
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
