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

const vector3 OrgCrossPos = vector3(0, -10, 0);

void TCamera::Init(vector3 NPos, vector3 NAngle)
{

    pOffset = vector3(-0.0, 0, 0);
    vUp = vector3(0, 1, 0);
    //    pOffset= vector3(-0.8,0,0);
    CrossPos = OrgCrossPos;
    CrossDist = 10;
    Velocity = vector3(0, 0, 0);
    OldVelocity = vector3(0, 0, 0);
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
        // Fix(Yaw,-M_PI,M_PI);
    }
}

void TCamera::Update()
{
    // ABu: zmiana i uniezaleznienie predkosci od FPS
    double a = ( Global::shiftState ? 5.00 : 1.00);
    if (Global::ctrlState)
        a = a * 100;
    //    OldVelocity=Velocity;
    if (FreeFlyModeFlag == true)
        Type = tp_Free;
    else
        Type = tp_Follow;
    if (Type == tp_Free)
    {
        if (Console::Pressed(Global::Keys[k_MechUp]))
            Velocity.y += a;
        if (Console::Pressed(Global::Keys[k_MechDown]))
            Velocity.y -= a;

        // McZapkie-170402: poruszanie i rozgladanie we free takie samo jak w follow
        if (Console::Pressed(Global::Keys[k_MechRight]))
            Velocity.x += a;
        if (Console::Pressed(Global::Keys[k_MechLeft]))
            Velocity.x -= a;
        if (Console::Pressed(Global::Keys[k_MechForward]))
            Velocity.z -= a;
        if (Console::Pressed(Global::Keys[k_MechBackward]))
            Velocity.z += a;
        // gora-dol
        // if (Console::Pressed(GLFW_KEY_KP_9)) Pos.y+=0.1;
        // if (Console::Pressed(GLFW_KEY_KP_3)) Pos.y-=0.1;

        // McZapkie: zeby nie hustalo przy malym FPS:
        //        Velocity= (Velocity+OldVelocity)/2;
        //    matrix4x4 mat;
        vector3 Vec = Velocity;
        Vec.RotateY(Yaw);
        Pos = Pos + Vec * Timer::GetDeltaRenderTime(); // czas bez pauzy
        Velocity = Velocity / 2; // płynne hamowanie ruchu
        //    double tmp= 10*DeltaTime;
        //        Velocity+= -Velocity*10 * Timer::GetDeltaTime();//( tmp<1 ? tmp : 1 );
        //        Type= tp_Free;
    }
}

vector3 TCamera::GetDirection()
{
    matrix4x4 mat;
    vector3 Vec;
    Vec = vector3(0, 0, 1);
    Vec.RotateY(Yaw);

    return (Normalize(Vec));
}
/*
bool TCamera::SetMatrix()
{
    glRotated( -Roll * 180.0f / M_PI, 0, 0, 1 ); // po wyłączeniu tego kręci się pojazd, a sceneria nie
    glRotated( -Pitch * 180.0f / M_PI, 1, 0, 0 );
    glRotated( -Yaw * 180.0f / M_PI, 0, 1, 0 ); // w zewnętrznym widoku: kierunek patrzenia

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
*/
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

    glRotated(-Roll * 180.0f / M_PI, 0, 0, 1);
    glRotated(-Pitch * 180.0f / M_PI, 1, 0, 0);
    glRotated(-Yaw * 180.0f / M_PI, 0, 1, 0); // w zewnętrznym widoku: kierunek patrzenia
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

