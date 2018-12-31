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

gamepad_input::gamepad_input() {

    m_modecommands = {

        { user_command::mastercontrollerincrease, user_command::mastercontrollerdecrease },
        { user_command::trainbrakedecrease, user_command::trainbrakeincrease },
        { user_command::secondcontrollerincrease, user_command::secondcontrollerdecrease },
        { user_command::independentbrakedecrease, user_command::independentbrakeincrease }
    };
}

bool
gamepad_input::init() {

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
        m_axes.clear();
        m_buttons.clear();
        return false;
    }

    int count;

    glfwGetJoystickAxes( m_deviceid, &count );
    m_axes.resize( count );

    glfwGetJoystickButtons( m_deviceid, &count );
    m_buttons.resize( count );

    return true;
}

// checks state of the controls and sends issued commands
void
gamepad_input::poll() {

    if( m_deviceid == -1 ) {
        // if there's no gamepad we can skip the rest
        return;
    }

    int count; std::size_t idx = 0;
    // poll button state
    auto const buttons = glfwGetJoystickButtons( m_deviceid, &count );
    if( count ) { // safety check in case joystick gets pulled out
        for( auto &button : m_buttons ) {

            if( button != buttons[ idx ] ) {
                // button pressed or released, both are important
                on_button(
                    static_cast<gamepad_button>( idx ),
                    ( buttons[ idx ] == 1 ?
                        GLFW_PRESS :
                        GLFW_RELEASE ) );
            }
            else {
                // otherwise we only pass info about button being held down
                if( button == 1 ) {

                    on_button(
                        static_cast<gamepad_button>( idx ),
                        GLFW_REPEAT );
                }
            }
            button = buttons[ idx ];
            ++idx;
        }
    }

    // poll axes state
    idx = 0;
    glm::vec2 leftstick, rightstick, triggers;
    auto const axes = glfwGetJoystickAxes( m_deviceid, &count );
    if( count ) {
        // safety check in case joystick gets pulled out
        if( count >= 2 ) {
            leftstick = glm::vec2(
                ( std::abs( axes[ gamepad_axes::leftstick_x ] ) > m_deadzone ?
                    axes[ gamepad_axes::leftstick_x ] :
                    0.0f ),
                ( std::abs( axes[ gamepad_axes::leftstick_y ] ) > m_deadzone ?
                    axes[ gamepad_axes::leftstick_y ] :
                    0.0f ) );
        }
        if( count >= 4 ) {
            rightstick = glm::vec2(
                ( std::abs( axes[ gamepad_axes::rightstick_x ] ) > m_deadzone ?
                    axes[ gamepad_axes::rightstick_x ] :
                    0.0f ),
                ( std::abs( axes[ gamepad_axes::rightstick_y ] ) > m_deadzone ?
                    axes[ gamepad_axes::rightstick_y ] :
                    0.0f ) );
        }
        if( count >= 6 ) {
            triggers = glm::vec2(
                ( axes[ gamepad_axes::lefttrigger ] > m_deadzone ?
                    axes[ gamepad_axes::lefttrigger ] :
                    0.0f ),
                ( axes[ gamepad_axes::righttrigger ] > m_deadzone ?
                    axes[ gamepad_axes::righttrigger ] :
                    0.0f ) );
        }
    }
    process_axes( leftstick, rightstick, triggers );
}

void
gamepad_input::on_button( gamepad_button const Button, int const Action ) {

    switch( Button ) {
        // NOTE: this is rigid coupling, down the road we should support more flexible binding of functions with buttons
        case gamepad_button::a:
        case gamepad_button::b:
        case gamepad_button::x:
        case gamepad_button::y: {

            if( Action == GLFW_RELEASE ) {
                // TODO: send GLFW_RELEASE for whatever command could be issued by the mode active until now
                // if the button was released the stick switches to control the movement
                m_mode = control_mode::entity;
                // zero the stick and the accumulator so the input won't bleed between modes
                m_leftstick = glm::vec2();
                m_modeaccumulator = 0.0f;
            }
            else {
                // otherwise set control mode to match pressed button
                m_mode = static_cast<control_mode>( Button );
            }
            break;
        }
        default: {
            break;
        }
    }
}

void
gamepad_input::process_axes( glm::vec2 Leftstick, glm::vec2 const &Rightstick, glm::vec2 const &Triggers ) {

    // right stick, look around
    if( ( Rightstick.x != 0.0f ) || ( Rightstick.y != 0.0f ) ) {
        // TODO: make toggles for the axis flip
        auto const deltatime = Timer::GetDeltaRenderTime() * 60.0;
        double const turnx =  Rightstick.x * 10.0 * deltatime;
        double const turny = -Rightstick.y * 10.0 * deltatime;
        m_relay.post(
            user_command::viewturn,
            turnx,
            turny,
            GLFW_PRESS,
		    0 );
    }

    // left stick, either movement or controls, depending on currently active mode
    if( m_mode == control_mode::entity ) {

        if( (   Leftstick.x != 0.0 ||   Leftstick.y != 0.0 )
         || ( m_leftstick.x != 0.0 || m_leftstick.y != 0.0 ) ) {
            m_relay.post(
                user_command::movehorizontal,
                Leftstick.x,
                Leftstick.y,
                GLFW_PRESS,
			    0 );
        }
    }
    else {
        // vehicle control modes
        process_mode( Leftstick.y, 0 );
    }

    m_rightstick = Rightstick;
    m_leftstick = Leftstick;
    m_triggers = Triggers;
}

void
gamepad_input::process_mode( float const Value, std::uint16_t const Recipient ) {

    // TODO: separate multiplier for each mode, to allow different, customizable sensitivity for each control
    auto const deltatime = Timer::GetDeltaTime() * 15.0;
    auto const &lookup = m_modecommands.at( static_cast<size_t>( m_mode ) );

    if( Value >= 0.0f ) {
        if( m_modeaccumulator < 0.0f ) {
            // reset accumulator if we're going in the other direction i.e. issuing opposite control
            // this also means we should indicate the previous command no longer applies
            // (normally it's handled when the stick enters dead zone, but it's possible there's no actual dead zone)
            m_relay.post(
                lookup.second,
                0, 0,
                GLFW_RELEASE,
                Recipient );
            m_modeaccumulator = 0.0f;
        }
        if( Value > m_deadzone ) {
            m_modeaccumulator += ( Value - m_deadzone ) / ( 1.0 - m_deadzone ) * deltatime;
            // we're making sure there's always a positive charge left in the accumulator,
            // to more reliably decect when the stick goes from active to dead zone, below
            while( m_modeaccumulator > 1.0f ) {
                // send commands if the accumulator(s) was filled
                m_relay.post(
                    lookup.first,
                    0, 0,
                    GLFW_PRESS,
                    Recipient );
                m_modeaccumulator -= 1.0f;
            }
        }
        else {
            // if the accumulator isn't empty it's an indicator the stick moved from active to neutral zone
            // indicate it with proper RELEASE command
            m_relay.post(
                lookup.first,
                0, 0,
                GLFW_RELEASE,
                Recipient );
            m_modeaccumulator = 0.0f;
        }
    }
    else {
        if( m_modeaccumulator > 0.0f ) {
            // reset accumulator if we're going in the other direction i.e. issuing opposite control
            // this also means we should indicate the previous command no longer applies
            // (normally it's handled when the stick enters dead zone, but it's possible there's no actual dead zone)
            m_relay.post(
                lookup.first,
                0, 0,
                GLFW_RELEASE,
                Recipient );
            m_modeaccumulator = 0.0f;
        }
        if( Value < m_deadzone ) {
            m_modeaccumulator += ( Value + m_deadzone ) / ( 1.0 - m_deadzone ) * deltatime;
            // we're making sure there's always a negative charge left in the accumulator,
            // to more reliably decect when the stick goes from active to dead zone, below
            while( m_modeaccumulator < -1.0f ) {
                // send commands if the accumulator(s) was filled
                m_relay.post(
                    lookup.second,
                    0, 0,
                    GLFW_PRESS,
                    Recipient );
                m_modeaccumulator += 1.0f;
            }
        }
        else {
            // if the accumulator isn't empty it's an indicator the stick moved from active to neutral zone
            // indicate it with proper RELEASE command
            m_relay.post(
                lookup.second,
                0, 0,
                GLFW_RELEASE,
                Recipient );
            m_modeaccumulator = 0.0f;
        }
    }
}

//---------------------------------------------------------------------------
