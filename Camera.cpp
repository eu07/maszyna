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
#include "MOVER.h"

//---------------------------------------------------------------------------

void TCamera::Init( Math3D::vector3 const &NPos, Math3D::vector3 const &NAngle, TCameraType const NType ) {

    vUp = { 0, 1, 0 };
    Velocity = { 0, 0, 0 };
    Pitch = NAngle.x;
    Yaw = NAngle.y;
    Roll = NAngle.z;
    Pos = NPos;

    Type = NType;
};

void TCamera::Reset() {

    Pitch = Yaw = Roll = 0;
    m_rotationoffsets = {};
};


void TCamera::OnCursorMove(double x, double y) {
/*
    Yaw -= x;
    while( Yaw > M_PI ) {
        Yaw -= 2 * M_PI;
    }
    while( Yaw < -M_PI ) {
        Yaw += 2 * M_PI;
    }
    Pitch -= y;
    if (Type == tp_Follow) {
        // jeżeli jazda z pojazdem ograniczenie kąta spoglądania w dół i w górę
        Pitch = clamp( Pitch, -M_PI_4, M_PI_4 );
    }
*/
    m_rotationoffsets += glm::dvec3 { y, x, 0.0 };
}

bool
TCamera::OnCommand( command_data const &Command ) {

    auto const walkspeed { 1.0 };
    auto const runspeed { 10.0 };

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
                Type == TCameraType::tp_Free ?   runspeed :
                Type == TCameraType::tp_Follow ? walkspeed :
                0.0 );

            auto const speedmultiplier = (
                ( ( Type == TCameraType::tp_Free ) && ( Command.command == user_command::movehorizontalfast ) ) ?
                    30.0 :
                    1.0 );

            // left-right
            auto const movexparam { Command.param1 };
            // 2/3rd of the stick range enables walk speed, past that we lerp between walk and run speed
            auto const movex { walkspeed + ( std::max( 0.0, std::abs( movexparam ) - 0.65 ) / 0.35 ) * ( movespeed - walkspeed ) };

            m_moverate.x = (
                movexparam > 0.0 ?  movex * speedmultiplier :
                movexparam < 0.0 ? -movex * speedmultiplier :
                0.0 );

            // forward-back
            double const movezparam { Command.param2 };
            auto const movez { walkspeed + ( std::max( 0.0, std::abs( movezparam ) - 0.65 ) / 0.35 ) * ( movespeed - walkspeed ) };
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
                Type == TCameraType::tp_Free ?   runspeed * 0.5 :
                Type == TCameraType::tp_Follow ? walkspeed :
                0.0 );

            auto const speedmultiplier = (
                ( ( Type == TCameraType::tp_Free ) && ( Command.command == user_command::moveverticalfast ) ) ?
                    10.0 :
                    1.0 );

            // up-down
            auto const moveyparam { Command.param1 };
            // 2/3rd of the stick range enables walk speed, past that we lerp between walk and run speed
            auto const movey { walkspeed + ( std::max( 0.0, std::abs( moveyparam ) - 0.65 ) / 0.35 ) * ( movespeed - walkspeed ) };

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
    if( FreeFlyModeFlag == true ) { Type = TCameraType::tp_Free; }
    else                          { Type = TCameraType::tp_Follow; }

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

    // update position
    if( ( Type == TCameraType::tp_Free )
     || ( false == Global.ctrlState )
     || ( true == DebugCameraFlag ) ) {
        // ctrl is used for mirror view, so we ignore the controls when in vehicle if ctrl is pressed
        // McZapkie-170402: poruszanie i rozgladanie we free takie samo jak w follow
        Velocity.x = clamp( Velocity.x + m_moverate.x * 10.0 * deltatime, -std::abs( m_moverate.x ), std::abs( m_moverate.x ) );
        Velocity.z = clamp( Velocity.z + m_moverate.z * 10.0 * deltatime, -std::abs( m_moverate.z ), std::abs( m_moverate.z ) );
        Velocity.y = clamp( Velocity.y + m_moverate.y * 10.0 * deltatime, -std::abs( m_moverate.y ), std::abs( m_moverate.y ) );
    }

    if( ( Type == TCameraType::tp_Free )
     || ( true == DebugCameraFlag ) ) {
        // free movement position update is handled here, movement while in vehicle is handled by train update
        Math3D::vector3 Vec = Velocity;
        Vec.RotateY( Yaw );
        Pos += Vec * 5.0 * deltatime;
    }
    // update rotation
    auto const rotationfactor { std::min( 1.0, 20 * deltatime ) };

    Yaw -= rotationfactor * m_rotationoffsets.y;
    m_rotationoffsets.y *= ( 1.0 - rotationfactor );
    while( Yaw > M_PI ) {
        Yaw -= 2 * M_PI;
    }
    while( Yaw < -M_PI ) {
        Yaw += 2 * M_PI;
    }

    Pitch -= rotationfactor * m_rotationoffsets.x;
    m_rotationoffsets.x *= ( 1.0 - rotationfactor );
    if( Type == TCameraType::tp_Follow ) {
        // jeżeli jazda z pojazdem ograniczenie kąta spoglądania w dół i w górę
        Pitch = clamp( Pitch, -M_PI_4, M_PI_4 );
    }
}

Math3D::vector3 TCamera::GetDirection() {

    return glm::normalize( glm::rotateY<float>( glm::vec3{ 0.f, 0.f, 1.f }, Yaw ) );
}

bool TCamera::SetMatrix( glm::dmat4 &Matrix ) {

    Matrix = glm::rotate( Matrix, -Roll, glm::dvec3( 0.0, 0.0, 1.0 ) ); // po wyłączeniu tego kręci się pojazd, a sceneria nie
    Matrix = glm::rotate( Matrix, -Pitch, glm::dvec3( 1.0, 0.0, 0.0 ) );
    Matrix = glm::rotate( Matrix, -Yaw, glm::dvec3( 0.0, 1.0, 0.0 ) ); // w zewnętrznym widoku: kierunek patrzenia

    if( ( Type == TCameraType::tp_Follow ) && ( false == DebugCameraFlag ) ) {

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
    Math3D::vector3 where = LookAt - Pos + Math3D::vector3(0, 3, 0); // trochę w górę od szyn
    if( ( where.x != 0.0 ) || ( where.z != 0.0 ) ) {
        Yaw = atan2( -where.x, -where.z ); // kąt horyzontalny
        m_rotationoffsets.y = 0.0;
    }
    double l = Math3D::Length3(where);
    if( l > 0.0 ) {
        Pitch = asin( where.y / l ); // kąt w pionie
        m_rotationoffsets.x = 0.0;
    }
};
