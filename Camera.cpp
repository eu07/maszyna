/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "Camera.h"

#include "Globals.h"
#include "utilities.h"
#include "Console.h"
#include "Timer.h"
#include "DynObj.h"
#include "MOVER.h"

//---------------------------------------------------------------------------

void TCamera::Init( Math3D::vector3 const &NPos, Math3D::vector3 const &NAngle/*, TCameraType const NType*/, TDynamicObject *Owner ) {

    vUp = { 0, 1, 0 };
    Velocity = { 0, 0, 0 };
    Angle = NAngle;
    Pos = NPos;

    m_owner = Owner;
};

void TCamera::Reset() {

    Angle = {};
    m_rotationoffsets = {};
};


void TCamera::OnCursorMove(double x, double y) {

    m_rotationoffsets += glm::dvec3 { y, x, 0.0 };
}

bool
TCamera::OnCommand( command_data const &Command ) {

    auto const walkspeed { 1.0 };
    auto const runspeed { 10.0 };

	// threshold position on stick between walk lerp and walk/run lerp
	auto const stickthreshold = 2.0 / 3.0;

    bool iscameracommand { true };
    switch( Command.command ) {

        case user_command::viewturn: {

            OnCursorMove(
                Command.param1 *  0.005 * Global.fMouseXScale / Global.ZoomFactor,
                Command.param2 *  0.01  * Global.fMouseYScale / Global.ZoomFactor );
            break;
        }

        case user_command::movehorizontal:
        case user_command::movehorizontalfast: {

            auto const movespeed = (
                m_owner == nullptr ?       runspeed : // free roam
                false == FreeFlyModeFlag ? walkspeed : // vehicle cab
                0.0 ); // vehicle external

//            if( movespeed == 0.0 ) { break; } // enable to fix external cameras in place

            auto const speedmultiplier = (
                ( ( m_owner == nullptr ) && ( Command.command == user_command::movehorizontalfast ) ) ?
                    30.0 :
                    1.0 );

            // left-right
            auto const movexparam { Command.param1 };
            // 2/3rd of the stick range lerps walk speed, past that we lerp between max walk and run speed
            auto const movex { walkspeed * std::min(std::abs(movexparam) * (1.0 / stickthreshold), 1.0)
                + ( std::max( 0.0, std::abs( movexparam ) - stickthreshold ) / (1.0 - stickthreshold) ) * std::max( 0.0, movespeed - walkspeed ) };

            m_moverate.x = (
                movexparam > 0.0 ?  movex * speedmultiplier :
                movexparam < 0.0 ? -movex * speedmultiplier :
                0.0 );

            // forward-back
            double const movezparam { Command.param2 };
            auto const movez { walkspeed * std::min(std::abs(movezparam) * (1.0 / stickthreshold), 1.0)
                + ( std::max( 0.0, std::abs( movezparam ) - stickthreshold ) / (1.0 - stickthreshold) ) * std::max( 0.0, movespeed - walkspeed ) };

            // NOTE: z-axis is flipped given world coordinate system
            m_moverate.z = (
                movezparam > 0.0 ? -movez * speedmultiplier :
                movezparam < 0.0 ?  movez * speedmultiplier :
                0.0 );

            break;
        }

        case user_command::movevertical:
        case user_command::moveverticalfast: {

            auto const movespeed = (
                m_owner == nullptr ?       runspeed * 0.5 : // free roam
                false == FreeFlyModeFlag ? walkspeed : // vehicle cab
                0.0 ); // vehicle external

//            if( movespeed == 0.0 ) { break; } // enable to fix external cameras in place

            auto const speedmultiplier = (
                ( ( m_owner == nullptr ) && ( Command.command == user_command::moveverticalfast ) ) ?
                    10.0 :
                    1.0 );
            // up-down
            auto const moveyparam { Command.param1 };
            // 2/3rd of the stick range lerps walk speed, past that we lerp between max walk and run speed
            auto const movey { walkspeed * std::min(std::abs(moveyparam) * (1.0 / stickthreshold), 1.0)
                + ( std::max( 0.0, std::abs( moveyparam ) - stickthreshold ) / (1.0 - stickthreshold) ) * std::max( 0.0, movespeed - walkspeed ) };

            m_moverate.y = (
                moveyparam > 0.0 ?  movey * speedmultiplier :
                moveyparam < 0.0 ? -movey * speedmultiplier :
                0.0 );

            break;
        }

        default: {

            iscameracommand = false;
            break;
        }
    } // switch

    return iscameracommand;
}

void TCamera::Update()
{
    // check for sent user commands
    // NOTE: this is a temporary arrangement, for the transition period from old command setup to the new one
    // ultimately we'll need to track position of camera/driver for all human entities present in the scenario
    command_data command;
    // NOTE: currently we're only storing commands for local entity and there's no id system in place,
    // so we're supplying 'default' entity id of 0
    while( simulation::Commands.pop( command, static_cast<std::size_t>( command_target::entity ) | 0 ) ) {

        OnCommand( command );
    }

    auto const deltatime { Timer::GetDeltaRenderTime() }; // czas bez pauzy

    // update rotation
    auto const rotationfactor { std::min( 1.0, 20 * deltatime ) };

    Angle.y -= m_rotationoffsets.y * rotationfactor;
    m_rotationoffsets.y *= ( 1.0 - rotationfactor );
    while( Angle.y > M_PI ) {
        Angle.y -= 2 * M_PI;
    }
    while( Angle.y < -M_PI ) {
        Angle.y += 2 * M_PI;
    }

    Angle.x -= m_rotationoffsets.x * rotationfactor;
    m_rotationoffsets.x *= ( 1.0 - rotationfactor );

    if( m_owner != nullptr ) {
        // jeżeli jazda z pojazdem ograniczenie kąta spoglądania w dół i w górę
        Angle.x = clamp( Angle.x, -M_PI_4, M_PI_4 );
    }

    // update position
    if( ( m_owner == nullptr )
     || ( false == Global.ctrlState )
     || ( true == DebugCameraFlag ) ) {
        // ctrl is used for mirror view, so we ignore the controls when in vehicle if ctrl is pressed
        // McZapkie-170402: poruszanie i rozgladanie we free takie samo jak w follow
        Velocity.x = clamp( Velocity.x + m_moverate.x * 10.0 * deltatime, -std::abs( m_moverate.x ), std::abs( m_moverate.x ) );
        Velocity.z = clamp( Velocity.z + m_moverate.z * 10.0 * deltatime, -std::abs( m_moverate.z ), std::abs( m_moverate.z ) );
        Velocity.y = clamp( Velocity.y + m_moverate.y * 10.0 * deltatime, -std::abs( m_moverate.y ), std::abs( m_moverate.y ) );
    }
    if( ( m_owner == nullptr )
     || ( true == DebugCameraFlag ) ) {
        // free movement position update
        auto movement { Velocity };
        movement.RotateY( Angle.y );
        Pos += movement * 5.0 * deltatime;
    }
    else {
        // attached movement position update
        auto movement { Velocity * -2.0 };
        movement.y = -movement.y;
        if( m_owner->MoverParameters->ActiveCab < 0 ) {
            movement *= -1.f;
            movement.y = -movement.y;
        }
        movement.RotateY( Angle.y );

        m_owneroffset += movement * deltatime;
    }
}

bool TCamera::SetMatrix( glm::dmat4 &Matrix ) {

    Matrix = glm::rotate( Matrix, -Angle.z, glm::dvec3( 0.0, 0.0, 1.0 ) ); // po wyłączeniu tego kręci się pojazd, a sceneria nie
    Matrix = glm::rotate( Matrix, -Angle.x, glm::dvec3( 1.0, 0.0, 0.0 ) );
    Matrix = glm::rotate( Matrix, -Angle.y, glm::dvec3( 0.0, 1.0, 0.0 ) ); // w zewnętrznym widoku: kierunek patrzenia

    if( ( m_owner != nullptr ) && ( false == DebugCameraFlag ) ) {

        Matrix *= glm::lookAt(
            glm::dvec3{ Pos },
            glm::dvec3{ LookAt },
            glm::dvec3{ vUp } );
    }
    else {
        Matrix = glm::translate( Matrix, glm::dvec3{ -Pos } ); // nie zmienia kierunku patrzenia
    }

    return true;
}

void TCamera::RaLook()
{ // zmiana kierunku patrzenia - przelicza Yaw
    Math3D::vector3 where = LookAt - Pos /*+ Math3D::vector3(0, 3, 0)*/; // trochę w górę od szyn
    if( ( where.x != 0.0 ) || ( where.z != 0.0 ) ) {
        Angle.y = atan2( -where.x, -where.z ); // kąt horyzontalny
        m_rotationoffsets.y = 0.0;
    }
    double l = Math3D::Length3(where);
    if( l > 0.0 ) {
        Angle.x = asin( where.y / l ); // kąt w pionie
        m_rotationoffsets.x = 0.0;
    }
};
