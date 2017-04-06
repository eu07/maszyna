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
#include "Usefull.h"
#include "Console.h"
#include "Timer.h"
#include "mover.h"

//---------------------------------------------------------------------------

// TViewPyramid TCamera::OrgViewPyramid;
//={vector3(-1,1,1),vector3(1,1,1),vector3(-1,-1,1),vector3(1,-1,1),vector3(0,0,0)};

void TCamera::Init(vector3 NPos, vector3 NAngle)
{

    vUp = vector3(0, 1, 0);
    //    pOffset= vector3(-0.8,0,0);
    CrossDist = 10;
    Velocity = vector3(0, 0, 0);
    Pitch = NAngle.x;
    Yaw = NAngle.y;
    Roll = NAngle.z;
    Pos = NPos;

    //    Type= tp_Follow;
    Type = (Global::bFreeFly ? tp_Free : tp_Follow);
    //    Type= tp_Free;
};

void TCamera::OnCursorMove(double x, double y)
{
    // McZapkie-170402: zeby mysz dzialala zawsze    if (Type==tp_Follow)
    Pitch += y;
    Yaw += -x;
    if (Yaw > M_PI)
        Yaw -= 2 * M_PI;
    else if (Yaw < -M_PI)
        Yaw += 2 * M_PI;
    if (Type == tp_Follow) // jeżeli jazda z pojazdem
    {
        clamp(Pitch, -M_PI_4, M_PI_4); // ograniczenie kąta spoglądania w dół i w górę
    }
}

void
TCamera::OnCommand( command_data const &Command ) {

    double const walkspeed = 1.0;
    double const runspeed = ( DebugModeFlag ? 50.0 : 7.5 );
    double const speedmultiplier = ( DebugModeFlag ? 7.5 : 1.0 );

    switch( Command.command ) {

        case user_command::viewturn: {

            OnCursorMove(
                reinterpret_cast<double const &>( Command.param1 ) *  0.005 * Global::fMouseXScale / Global::ZoomFactor,
                reinterpret_cast<double const &>( Command.param2 ) * -0.01  * Global::fMouseYScale / Global::ZoomFactor );
            break;
        }

        case user_command::movevector: {

            auto const movespeed =
                ( Type == tp_Free ?
                    runspeed * speedmultiplier :
                    walkspeed );

            // left-right
            double const movex = reinterpret_cast<double const &>( Command.param1 );
            if( movex > 0.0 ) {
                m_keys.right = true;
                m_keys.left = false;
            }
            else if( movex < 0.0 ) {
                m_keys.right = false;
                m_keys.left = true;
            }
            else {
                m_keys.right = false;
                m_keys.left = false;
            }
            // 2/3rd of the stick range enables walk speed, past that we lerp between walk and run speed
            m_moverate.x =
                walkspeed
                + ( std::max( 0.0, std::abs( movex ) - 0.65 ) / 0.35 ) * ( movespeed - walkspeed );

            // forward-back
            double const movez = reinterpret_cast<double const &>( Command.param2 );
            if( movez > 0.0 ) {
                m_keys.forward = true;
                m_keys.back = false;
            }
            else if( movez < 0.0 ) {
                m_keys.forward = false;
                m_keys.back = true;
            }
            else {
                m_keys.forward = false;
                m_keys.back = false;
            }
            m_moverate.z =
                walkspeed
                + ( std::max( 0.0, std::abs( movez ) - 0.65 ) / 0.35 ) * ( movespeed - walkspeed );
            break;
        }

        case user_command::moveforward: {

            if( Command.action != GLFW_RELEASE ) {
                m_keys.forward = true;
                m_moverate.z =
                    ( Type == tp_Free ?
                        walkspeed * speedmultiplier :
                        walkspeed );
            }
            else {
                m_keys.forward = false;
            }
            break;
        }

        case user_command::moveback: {

            if( Command.action != GLFW_RELEASE ) {
                m_keys.back = true;
                m_moverate.z =
                    ( Type == tp_Free ?
                        walkspeed * speedmultiplier :
                        walkspeed );
            }
            else {
                m_keys.back = false;
            }
            break;
        }

        case user_command::moveleft: {

            if( Command.action != GLFW_RELEASE ) {
                m_keys.left = true;
                m_moverate.x =
                    ( Type == tp_Free ?
                        walkspeed * speedmultiplier :
                        walkspeed );
            }
            else {
                m_keys.left = false;
            }
            break;
        }

        case user_command::moveright: {

            if( Command.action != GLFW_RELEASE ) {
                m_keys.right = true;
                m_moverate.x =
                    ( Type == tp_Free ?
                        walkspeed * speedmultiplier :
                        walkspeed );
            }
            else {
                m_keys.right = false;
            }
            break;
        }

        case user_command::moveup: {

            if( Command.action != GLFW_RELEASE ) {
                m_keys.up = true;
                m_moverate.y =
                    ( Type == tp_Free ?
                        walkspeed * speedmultiplier :
                        walkspeed );
            }
            else {
                m_keys.up = false;
            }
            break;
        }

        case user_command::movedown: {

            if( Command.action != GLFW_RELEASE ) {
                m_keys.down = true;
                m_moverate.y =
                    ( Type == tp_Free ?
                        walkspeed * speedmultiplier :
                        walkspeed );
            }
            else {
                m_keys.down = false;
            }
            break;
        }

        case user_command::moveforwardfast: {

            if( Command.action != GLFW_RELEASE ) {
                m_keys.forward = true;
                m_moverate.z =
                    ( Type == tp_Free ?
                        runspeed * speedmultiplier :
                        walkspeed );
            }
            else {
                m_keys.forward = false;
            }
            break;
        }

        case user_command::movebackfast: {

            if( Command.action != GLFW_RELEASE ) {
                m_keys.back = true;
                m_moverate.z =
                    ( Type == tp_Free ?
                        runspeed * speedmultiplier :
                        walkspeed );
            }
            else {
                m_keys.back = false;
            }
            break;
        }

        case user_command::moveleftfast: {

            if( Command.action != GLFW_RELEASE ) {
                m_keys.left = true;
                m_moverate.x =
                    ( Type == tp_Free ?
                        runspeed * speedmultiplier :
                        walkspeed );
            }
            else {
                m_keys.left = false;
            }
            break;
        }

        case user_command::moverightfast: {

            if( Command.action != GLFW_RELEASE ) {
                m_keys.right = true;
                m_moverate.x =
                    ( Type == tp_Free ?
                        runspeed * speedmultiplier :
                        walkspeed );
            }
            else {
                m_keys.right = false;
            }
            break;
        }

        case user_command::moveupfast: {

            if( Command.action != GLFW_RELEASE ) {
                m_keys.up = true;
                m_moverate.y =
                    ( Type == tp_Free ?
                        runspeed * speedmultiplier :
                        walkspeed );
            }
            else {
                m_keys.up = false;
            }
            break;
        }

        case user_command::movedownfast: {

            if( Command.action != GLFW_RELEASE ) {
                m_keys.down = true;
                m_moverate.y =
                    ( Type == tp_Free ?
                        runspeed * speedmultiplier :
                        walkspeed );
            }
            else {
                m_keys.down = false;
            }
            break;
        }
/*
        case user_command::moveforwardfastest: {

            if( Command.action != GLFW_RELEASE ) {
                m_keys.forward = true;
                m_moverate.z =
                    ( Type == tp_Free ?
                        8.0 :
                        0.8 );
            }
            else {
                m_keys.forward = false;
            }
            break;
        }

        case user_command::movebackfastest: {

            if( Command.action != GLFW_RELEASE ) {
                m_keys.back = true;
                m_moverate.z =
                    ( Type == tp_Free ?
                        8.0 :
                        0.8 );
            }
            else {
                m_keys.back = false;
            }
            break;
        }

        case user_command::moveleftfastest: {

            if( Command.action != GLFW_RELEASE ) {
                m_keys.left = true;
                m_moverate.x =
                    ( Type == tp_Free ?
                        8.0 :
                        0.8 );
            }
            else {
                m_keys.left = false;
            }
            break;
        }

        case user_command::moverightfastest: {

            if( Command.action != GLFW_RELEASE ) {
                m_keys.right = true;
                m_moverate.x =
                    ( Type == tp_Free ?
                        8.0 :
                        0.8 );
            }
            else {
                m_keys.right = false;
            }
            break;
        }

        case user_command::moveupfastest: {

            if( Command.action != GLFW_RELEASE ) {
                m_keys.up = true;
                m_moverate.y =
                    ( Type == tp_Free ?
                        8.0 :
                        0.8 );
            }
            else {
                m_keys.up = false;
            }
            break;
        }

        case user_command::movedownfastest: {

            if( Command.action != GLFW_RELEASE ) {
                m_keys.down = true;
                m_moverate.y =
                    ( Type == tp_Free ?
                        8.0 :
                        0.8 );
            }
            else {
                m_keys.down = false;
            }
            break;
        }
*/
    }
}

void TCamera::Update()
{
    if( FreeFlyModeFlag == true ) { Type = tp_Free; }
    else                          { Type = tp_Follow; }

    // check for sent user commands
    // NOTE: this is a temporary arrangement, for the transition period from old command setup to the new one
    // ultimately we'll need to track position of camera/driver for all human entities present in the scenario
    command_data command;
    // NOTE: currently we're only storing commands for local entity and there's no id system in place,
    // so we're supplying 'default' entity id of 0
    while( simulation::Commands.pop( command, static_cast<std::size_t>( command_target::entity ) | 0 ) ) {

        OnCommand( command );
    }

    auto const deltatime = Timer::GetDeltaRenderTime(); // czas bez pauzy

#ifdef EU07_USE_OLD_COMMAND_SYSTEM
    double a = 0.8; // default (walk) movement speed
    if( Type == tp_Free ) {
        // when not in the cab the speed modifiers are active
        if( Global::shiftState ) { a = 2.5; }
        if( Global::ctrlState ) { a *= 10.0; }
    }

    if( ( Type == tp_Free )
     || ( false == Global::ctrlState ) ) {
        // ctrl is used for mirror view, so we ignore the controls when in vehicle if ctrl is pressed
        if( Console::Pressed( Global::Keys[ k_MechUp ] ) )
            Velocity.y = clamp( Velocity.y + a * 10.0 * deltatime, -a, a );
        if( Console::Pressed( Global::Keys[ k_MechDown ] ) )
            Velocity.y = clamp( Velocity.y - a * 10.0 * deltatime, -a, a );

        // McZapkie-170402: poruszanie i rozgladanie we free takie samo jak w follow
        if( Console::Pressed( Global::Keys[ k_MechRight ] ) )
            Velocity.x = clamp( Velocity.x + a * 10.0 * deltatime, -a, a );
        if( Console::Pressed( Global::Keys[ k_MechLeft ] ) )
            Velocity.x = clamp( Velocity.x - a * 10.0 * deltatime, -a, a );
        if( Console::Pressed( Global::Keys[ k_MechForward ] ) )
            Velocity.z = clamp( Velocity.z - a * 10.0 * deltatime, -a, a );
        if( Console::Pressed( Global::Keys[ k_MechBackward ] ) )
            Velocity.z = clamp( Velocity.z + a * 10.0 * deltatime, -a, a );
    }
#else
/*
    m_moverate = 0.8; // default (walk) movement speed
    if( Type == tp_Free ) {
        // when not in the cab the speed modifiers are active
        if( Global::shiftState ) { m_moverate = 2.5; }
        if( Global::ctrlState ) { m_moverate *= 10.0; }
    }
*/
    if( ( Type == tp_Free )
     || ( false == Global::ctrlState ) ) {
        // ctrl is used for mirror view, so we ignore the controls when in vehicle if ctrl is pressed
        if( m_keys.up )
            Velocity.y = clamp( Velocity.y + m_moverate.y * 10.0 * deltatime, -m_moverate.y, m_moverate.y );
        if( m_keys.down )
            Velocity.y = clamp( Velocity.y - m_moverate.y * 10.0 * deltatime, -m_moverate.y, m_moverate.y );

        // McZapkie-170402: poruszanie i rozgladanie we free takie samo jak w follow
        if( m_keys.right )
            Velocity.x = clamp( Velocity.x + m_moverate.x * 10.0 * deltatime, -m_moverate.x, m_moverate.x );
        if( m_keys.left )
            Velocity.x = clamp( Velocity.x - m_moverate.x * 10.0 * deltatime, -m_moverate.x, m_moverate.x );
        if( m_keys.forward )
            Velocity.z = clamp( Velocity.z - m_moverate.z * 10.0 * deltatime, -m_moverate.z, m_moverate.z );
        if( m_keys.back )
            Velocity.z = clamp( Velocity.z + m_moverate.z * 10.0 * deltatime, -m_moverate.z, m_moverate.z );
    }
#endif

    if( Type == tp_Free ) {
        vector3 Vec = Velocity;
        Vec.RotateY( Yaw );
        Pos += Vec * 5.0 * deltatime;
    }
    else {

    }
/*
    if( deltatime < 1.0 / 20.0 ) {
        // płynne hamowanie ruchu
        Velocity -= Velocity * 20.0 * deltatime;
    }
    else {
        // instant stop
        Velocity.Zero();
    }
    if( std::abs( Velocity.x ) < 0.01 ) { Velocity.x = 0.0; }
    if( std::abs( Velocity.y ) < 0.01 ) { Velocity.y = 0.0; }
    if( std::abs( Velocity.z ) < 0.01 ) { Velocity.z = 0.0; }
*/
/*
    Velocity *= 0.5;
    if( std::abs( Velocity.x ) < 0.01 ) { Velocity.x = 0.0; }
    if( std::abs( Velocity.y ) < 0.01 ) { Velocity.y = 0.0; }
    if( std::abs( Velocity.z ) < 0.01 ) { Velocity.z = 0.0; }
*/
}

vector3 TCamera::GetDirection()
{
    matrix4x4 mat;
    vector3 Vec;
    Vec = vector3(0, 0, 1);
    Vec.RotateY(Yaw);

    return (Normalize(Vec));
}

bool TCamera::SetMatrix()
{
    glRotated( -Roll * 180.0 / M_PI, 0.0, 0.0, 1.0 ); // po wyłączeniu tego kręci się pojazd, a sceneria nie
    glRotated( -Pitch * 180.0 / M_PI, 1.0, 0.0, 0.0 );
    glRotated( -Yaw * 180.0 / M_PI, 0.0, 1.0, 0.0 ); // w zewnętrznym widoku: kierunek patrzenia

    if( Type == tp_Follow )
    {
        gluLookAt(
            Pos.x, Pos.y, Pos.z,
            LookAt.x, LookAt.y, LookAt.z,
            vUp.x, vUp.y, vUp.z); // Ra: pOffset is zero
    }
    else {
        glTranslated( -Pos.x, -Pos.y, -Pos.z ); // nie zmienia kierunku patrzenia
    }

    Global::SetCameraPosition(Pos); // było +pOffset
    return true;
}

bool TCamera::SetMatrix( glm::mat4 &Matrix ) {

    Matrix = glm::rotate( Matrix, (float)-Roll, glm::vec3( 0.0f, 0.0f, 1.0f ) ); // po wyłączeniu tego kręci się pojazd, a sceneria nie
    Matrix = glm::rotate( Matrix, (float)-Pitch, glm::vec3( 1.0f, 0.0f, 0.0f ) );
    Matrix = glm::rotate( Matrix, (float)-Yaw, glm::vec3( 0.0f, 1.0f, 0.0f ) ); // w zewnętrznym widoku: kierunek patrzenia

    if( Type == tp_Follow ) {

        Matrix *= glm::lookAt(
            glm::vec3( Pos.x, Pos.y, Pos.z ),
            glm::vec3( LookAt.x, LookAt.y, LookAt.z ),
            glm::vec3( vUp.x, vUp.y, vUp.z ) );
    }
    else {
        Matrix = glm::translate( Matrix, glm::vec3( -Pos.x, -Pos.y, -Pos.z ) ); // nie zmienia kierunku patrzenia
    }

    Global::SetCameraPosition( Pos ); // było +pOffset
    return true;
}

void TCamera::SetCabMatrix(vector3 &p)
{ // ustawienie widoku z kamery bez przesunięcia robionego przez OpenGL - nie powinno tak trząść

    glRotated(-Roll * 180.0 / M_PI, 0.0, 0.0, 1.0);
    glRotated(-Pitch * 180.0 / M_PI, 1.0, 0.0, 0.0);
    glRotated(-Yaw * 180.0 / M_PI, 0.0, 1.0, 0.0); // w zewnętrznym widoku: kierunek patrzenia
    if (Type == tp_Follow)
        gluLookAt(Pos.x - p.x, Pos.y - p.y, Pos.z - p.z, LookAt.x - p.x, LookAt.y - p.y,
                  LookAt.z - p.z, vUp.x, vUp.y, vUp.z); // Ra: pOffset is zero
}

void TCamera::RaLook()
{ // zmiana kierunku patrzenia - przelicza Yaw
    vector3 where = LookAt - Pos + vector3(0, 3, 0); // trochę w górę od szyn
    if ((where.x != 0.0) || (where.z != 0.0))
        Yaw = atan2(-where.x, -where.z); // kąt horyzontalny
    double l = Length3(where);
    if (l > 0.0)
        Pitch = asin(where.y / l); // kąt w pionie
};

void TCamera::Stop()
{ // wyłącznie bezwładnego ruchu po powrocie do kabiny
    Type = tp_Follow;
    Velocity = vector3(0, 0, 0);
};

