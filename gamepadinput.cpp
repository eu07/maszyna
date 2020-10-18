/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "gamepadinput.h"
#include "Logs.h"
#include "Timer.h"
#include "utilities.h"
#include "parser.h"

glm::vec2 circle_to_square( glm::vec2 const &Point, int const Roundness = 0 ) {

    // Determine the theta angle
    auto angle = std::atan2( Point.x, Point.y ) + M_PI;

    glm::vec2 squared;
    // Scale according to which wall we're clamping to
    // X+ wall
    if( angle <= M_PI_4 || angle > 7 * M_PI_4 )
        squared = Point * (float)( 1.0 / std::cos( angle ) );
    // Y+ wall
    else if( angle > M_PI_4 && angle <= 3 * M_PI_4 )
        squared = Point * (float)( 1.0 / std::sin( angle ) );
    // X- wall
    else if( angle > 3 * M_PI_4 && angle <= 5 * M_PI_4 )
        squared = Point * (float)( -1.0 / std::cos( angle ) );
    // Y- wall
    else if( angle > 5 * M_PI_4 && angle <= 7 * M_PI_4 )
        squared = Point * (float)( -1.0 / std::sin( angle ) );

    // Early-out for a perfect square output
    if( Roundness == 0 )
        return squared;

    // Find the inner-roundness scaling factor and LERP
    auto const length = glm::length( Point );
    auto const factor = std::pow( length, Roundness );
    return interpolate( Point, squared, (float)factor );
}

bool
gamepad_input::init() {

    m_inputaxes.clear();
    m_inputbuttons.clear();
    // NOTE: we're only checking for joystick_1 and rely for it to stay connected throughout.
    // not exactly flexible, but for quick hack it'll do
    auto const name = glfwGetJoystickName( GLFW_JOYSTICK_1 );
    if( name != nullptr ) {
        WriteLog( "Connected gamepad: " + std::string( name ) );
        m_deviceid = GLFW_JOYSTICK_1;
    }
    else {
        // no joystick, 
        WriteLog( "No gamepad detected" );
        return false;
    }

    int count;

    glfwGetJoystickAxes( m_deviceid, &count );
    m_inputaxes.assign( count, { 0.0f, 0.0f, {} } );

    glfwGetJoystickButtons( m_deviceid, &count );
    m_inputbuttons.assign( count, { GLFW_RELEASE, -1, user_command::none } );

    recall_bindings();

    return true;
}

// checks state of the controls and sends issued commands
void
gamepad_input::poll() {

    if( m_deviceid == -1 ) {
        // if there's no gamepad we can skip the rest
        return;
    }

    int count;
    std::size_t idx = 0;
    // poll button state
    auto const buttons = glfwGetJoystickButtons( m_deviceid, &count );
    if( count ) { // safety check in case joystick gets pulled out
        for( auto &button : m_inputbuttons ) {

            if( button.state != buttons[ idx ] ) {
                // button pressed or released, both are important
                on_button(
                    idx,
                    ( buttons[ idx ] == 1 ?
                        GLFW_PRESS :
                        GLFW_RELEASE ) );
            }
            else {
                // otherwise we only pass info about button being held down
                if( button.state == GLFW_PRESS ) {

                    on_button(
                        idx,
                        GLFW_REPEAT );
                }
            }
            button.state = buttons[ idx ];
            ++idx;
        }
    }

    // poll axes state
    idx = 0;
    auto const axes = glfwGetJoystickAxes( m_deviceid, &count );
    if( count ) {
        // safety check in case joystick gets pulled out
        for( auto &axis : m_inputaxes ) {
            axis.state = axes[ idx ];
            ++idx;
        }
    }
    process_axes();
}

void
gamepad_input::bind( std::vector< std::reference_wrapper<user_command> > &Targets, cParser &Input, std::unordered_map<std::string, user_command> const &Translator, std::string const Point ) {

    for( auto &bindingtarget : Targets ) {
        // grab command(s) associated with the input pin
        auto const bindingcommandname{ Input.getToken<std::string>() };
        if( true == bindingcommandname.empty() ) {
            // no tokens left, may as well complain...
            WriteLog( "Gamepad binding for " + Point + " didn't specify associated command(s)" );
            // ...can't quit outright though, as provided references are likely to be unitialized
            bindingtarget.get() = user_command::none;
            continue;
        }
        auto const commandlookup = Translator.find( bindingcommandname );
        if( commandlookup == Translator.end() ) {
            WriteLog( "Gamepad binding for " + Point + " specified unknown command, \"" + bindingcommandname + "\"" );
            bindingtarget.get() = user_command::none;
        }
        else {
            bindingtarget.get() = commandlookup->second;
        }
    }
}

bool
gamepad_input::recall_bindings() {

    cParser bindingparser( "eu07_input-gamepad.ini", cParser::buffer_FILE );
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

    std::unordered_map<std::string, input_type> nametotypemap {
        { "3state", input_type::threestate },
        { "value", input_type::value },
        { "value_invert", input_type::value_invert } };

    // NOTE: to simplify things we expect one entry per line, and whole entry in one line
    while( true == bindingparser.getTokens( 1, true, "\n\r" ) ) {

        std::string bindingentry;
        bindingparser >> bindingentry;
        cParser entryparser( bindingentry );

        if( false == entryparser.getTokens( 1, true, "\n\r\t " ) ) { continue; }

        std::string bindingpoint {};
        entryparser >> bindingpoint;
        auto const splitbindingpoint { split_string_and_number( bindingpoint ) };

        if( splitbindingpoint.first == "axis" ) {
            // one or more sets of: [modeIDX] input type, parameters
            // [optional] modeIDX associates the set with control mode IDX
            // input types:
            // -- range commandname IDX; axis value is passed as paramIDX of commandname
            // -- 3state commandname commandname; positive axis value issues first commandname, negative value issues second commandname

            auto const axisindex { splitbindingpoint.second };
            // sanity check, connected gamepad isn't guaranteed to have that many axes
            if( axisindex >= m_inputaxes.size() ) { continue; }

            int controlmode { -1 }; // unless stated otherwise the set will be associated with the default control mode

            while( true == entryparser.getTokens( 1, true, "\n\r\t " ) ) {

                std::string key {};
                entryparser >> key;
                // check for potential mode indicator
                auto const splitkey { split_string_and_number( key ) };
                if( splitkey.first == "mode" ) {
                    // indicate we'll be processing specified mode
                    controlmode = splitkey.second;
                    continue;
                }
                // if we're still here handle the original key as binding type
                std::string const bindingtypename { key };
                auto const typelookup = nametotypemap.find( bindingtypename );
                if( typelookup == nametotypemap.end() ) {

                    WriteLog( "Gamepad binding for " + bindingpoint + " specified unknown control type, \"" + bindingtypename + "\"" );
                }
                else {

                    std::vector< std::reference_wrapper<user_command> > bindingtargets;
                    auto const bindingtype { typelookup->second };
                    std::get<0>( m_inputaxes[ axisindex ].bindings[ controlmode ] ) = bindingtype;
                    // retrieve regular commands associated with the axis and mode
                    switch( bindingtype ) {
                        case input_type::value:
                        case input_type::value_invert: {
                            bindingtargets.emplace_back( std::ref( std::get<1>( m_inputaxes[ axisindex ].bindings[ controlmode ] ) ) );
                            break;
                        }
                        case input_type::threestate: {
                            bindingtargets.emplace_back( std::ref( std::get<1>( m_inputaxes[ axisindex ].bindings[ controlmode ] ) ) );
                            bindingtargets.emplace_back( std::ref( std::get<2>( m_inputaxes[ axisindex ].bindings[ controlmode ] ) ) );
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                    bind( bindingtargets, entryparser, nametocommandmap, bindingpoint );
                    // handle potential remaining input type-specific parameters
                    switch( bindingtype ) {
                        case input_type::value:
                        case input_type::value_invert: {
                            auto const paramidxname { entryparser.getToken<std::string>() };
                            auto const paramidx { (
                                // exceptions
                                paramidxname == "y" ? 1 :
                                paramidxname == "2" ? 1 : // human-readable param index starts with 1
                                // default
                                0 ) };
                            std::get<2>( m_inputaxes[ axisindex ].bindings[ controlmode ] ) = static_cast<user_command>( paramidx );
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                }
            }
        }
        else if( splitbindingpoint.first == "button" ) {

            auto const buttonindex { splitbindingpoint.second };
            // sanity check, connected gamepad isn't guaranteed to have that many buttons
            if( buttonindex >= m_inputbuttons.size() ) { continue; }

            auto const bindingtype { entryparser.getToken<std::string>() };
            if( bindingtype == "mode" ) {
                // special case, mode selector
                if( true == entryparser.getTokens( 1, true, "\n\r\t " ) ) {
                    entryparser >> m_inputbuttons[ buttonindex ].mode;
                }
                else {
                    WriteLog( "Gamepad binding for " + bindingpoint + " didn't specify mode index" );
                }
            }
            else {
                // regular button, single bound command
                std::vector< std::reference_wrapper<user_command> > bindingtargets;
                bindingtargets.emplace_back( std::ref( m_inputbuttons[ buttonindex ].binding ) );
                bind( bindingtargets, entryparser, nametocommandmap, bindingpoint );
            }
        }
    }

    return true;
}

void
gamepad_input::on_button( int const Button, int const Action ) {

    auto const &button { m_inputbuttons[ Button ] };

    if( button.mode >= 0 ) {

        switch( Action ) {
            case GLFW_PRESS: {
                for( auto &axis : m_inputaxes ) {
                    axis.accumulator = 0.0f;
                }
                [[fallthrough]];
            }
            case GLFW_REPEAT: {
                m_mode = Button;
                break;
            }
            case GLFW_RELEASE: {
                if( m_mode == Button ) {
                    m_mode = -1;
                    for( auto &axis : m_inputaxes ) {
                        axis.accumulator = 0.0f;
                    }
                }
                break;
            }
            default: {
                break;
            }
        }
    }

    if( button.binding != user_command::none ) {

        m_relay.post(
            button.binding,
            0,
            0,
            Action,
            // as we haven't yet implemented either item id system or multiplayer, the 'local' controlled vehicle and entity have temporary ids of 0
            // TODO: pass correct entity id once the missing systems are in place
            0 );
    }
}

void
gamepad_input::process_axes() {

    input_type inputtype;
    user_command boundcommand1, boundcommand2;
    auto binding { std::tie( inputtype, boundcommand1, boundcommand2 ) };

    // since some commands can potentially collect values from two different axes we can't post them directly
    // instead, we use a small scratchpad to first put these commands together, then post them in their completed state
    std::unordered_map<user_command, std::tuple< double, double, int > > commands;
    // HACK: generate movement reset, it'll be eiter overriden by actual movement command, or issued if another control mode was activated
    commands[ user_command::movehorizontal ] = { 0.0, 0.0, GLFW_PRESS };

    for( auto &axis : m_inputaxes ) {

        if( axis.bindings.empty() ) { continue; }

        auto const lookup { axis.bindings.find( m_mode ) };
        if( lookup == axis.bindings.end() ) { continue; }

        binding = lookup->second;

        switch( inputtype ) {
            case input_type::threestate: {
                // TODO: separate multiplier for each mode, to allow different, customizable sensitivity for each control
                auto const deltatime { Timer::GetDeltaTime() * 15.0 };
                if( axis.state >= 0.0f ) {
                    // first bound command selected
                    if( axis.accumulator < 0.0f ) {
                        // we were issuing the other command, post notification that's no longer the case
                        if( boundcommand2 != user_command::none ) {
                            m_relay.post(
                                boundcommand2,
                                0, 0,
                                GLFW_RELEASE,
                                0 );
                            axis.accumulator = 0.0f;
                        }
                    }
                    if( boundcommand1 != user_command::none ) {
                        if( axis.state > m_deadzone ) {
                            axis.accumulator += ( axis.state - m_deadzone ) / ( 1.0 - m_deadzone ) * deltatime;
                            // we're making sure there's always a positive charge left in the accumulator,
                            // to more reliably decect when the stick goes from active to dead zone, below
                            while( axis.accumulator > 1.0f ) {
                                // send commands if the accumulator(s) was filled
                                m_relay.post(
                                    boundcommand1,
                                    0, 0,
                                    GLFW_PRESS,
                                    0 );
                                axis.accumulator -= 1.0f;
                            }
                        }
                        else {
                            // if the accumulator isn't empty it's an indicator the stick moved from active to neutral zone
                            // indicate it with proper RELEASE command
                            m_relay.post(
                                boundcommand1,
                                0, 0,
                                GLFW_RELEASE,
                                0 );
                            axis.accumulator = 0.0f;
                        }
                    }
                }
                else {
                    // second bound command selected
                    if( axis.accumulator > 0.0f ) {
                        // we were issuing the other command, post notification that's no longer the case
                        if( boundcommand1 != user_command::none ) {
                            m_relay.post(
                                boundcommand1,
                                0, 0,
                                GLFW_RELEASE,
                                0 );
                            axis.accumulator = 0.0f;
                        }
                    }
                    if( boundcommand1 != user_command::none ) {
                        if( axis.state < -m_deadzone ) {
                            axis.accumulator += ( axis.state + m_deadzone ) / ( 1.0 - m_deadzone ) * deltatime;
                            // we're making sure there's always a positive charge left in the accumulator,
                            // to more reliably decect when the stick goes from active to dead zone, below
                            while( axis.accumulator < -1.0f ) {
                                // send commands if the accumulator(s) was filled
                                m_relay.post(
                                    boundcommand2,
                                    0, 0,
                                    GLFW_PRESS,
                                    0 );
                                axis.accumulator += 1.0f;
                            }
                        }
                        else {
                            // if the accumulator isn't empty it's an indicator the stick moved from active to neutral zone
                            // indicate it with proper RELEASE command
                            m_relay.post(
                                boundcommand2,
                                0, 0,
                                GLFW_RELEASE,
                                0 );
                            axis.accumulator = 0.0f;
                        }
                    }
                }
                break;
            }
            case input_type::value:
            case input_type::value_invert: {
                auto &command { commands[ boundcommand1 ] };
                std::get<int>( command ) = GLFW_PRESS;
                auto &param { (
                    static_cast<int>( boundcommand2 ) == 0 ? // for this type boundcommand2 stores param index
                        std::get<0>( command ) :
                        std::get<1>( command ) ) };
                param = axis.state;
                if( std::abs( param ) < m_deadzone ) {
                    param = 0.0;
                }
                else {
                    param = (
                        param > 0.0 ?
                            ( param - m_deadzone ) / ( 1.0 - m_deadzone ) :
                            ( param + m_deadzone ) / ( 1.0 - m_deadzone ) );
                }
                if( param != 0.0 ) {
                    if( inputtype == input_type::value_invert ) {
                        param *= -1.0;
                    }
                }
                // scale passed value according to command type
                switch( boundcommand1 ) {
                    case user_command::viewturn: {
                        param *= 10.0 * ( Timer::GetDeltaRenderTime() * 60.0 );
                        break;
                    }
                    case user_command::movehorizontal:
                    case user_command::movehorizontalfast: {
                        // these expect value in -1:1 range
                        break;
                    }
                    default: {
                        // commands generally expect their parameter to be in 0:1 range
                        param = param * 0.5 + 0.5;
                        break;
                    }
                }
                break;
            }
            default: {
                break;
            }
        }
    }
    // issue remaining, assembled commands
    for( auto const &command : commands ) {
        auto const param1 { std::get<0>( command.second ) };
        auto const param2 { std::get<1>( command.second ) };
        auto &lastparams { m_lastcommandparams[ command.first ] };
        if( ( param1 != 0.0 ) || ( std::get<0>( lastparams ) != 0.0 )
         || ( param2 != 0.0 ) || ( std::get<1>( lastparams ) != 0.0 ) ) {
            m_relay.post(
                command.first,
                param1,
                param2,
                std::get<2>( command.second ),
                // as we haven't yet implemented either item id system or multiplayer, the 'local' controlled vehicle and entity have temporary ids of 0
                // TODO: pass correct entity id once the missing systems are in place
                0 );
            lastparams = std::tie( param1, param2 );
        }
    }
}

//---------------------------------------------------------------------------
