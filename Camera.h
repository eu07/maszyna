/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "dumb3d.h"
#include "dynobj.h"
#include "command.h"

using namespace Math3D;

//---------------------------------------------------------------------------
enum TCameraType
{ // tryby pracy kamery
    tp_Follow, // jazda z pojazdem
    tp_Free, // stoi na scenerii
    tp_Satelite // widok z góry (nie używany)
};

class TCamera
{
  private:
    struct keys {
        bool forward{ false };
        bool back{ false };
        bool left{ false };
        bool right{ false };
        bool up{ false };
        bool down{ false };
        bool run{ false };
    } m_keys;
    glm::dvec3 m_moverate;

  public: // McZapkie: potrzebuje do kiwania na boki
    double Pitch;
    double Yaw; // w środku: 0=do przodu; na zewnątrz: 0=na południe
    double Roll;
    TCameraType Type;
    vector3 Pos; // współrzędne obserwatora
    vector3 LookAt; // współrzędne punktu, na który ma patrzeć
    vector3 vUp;
    vector3 Velocity;
    vector3 CrossPos;
    double CrossDist;
    void Init(vector3 NPos, vector3 NAngle);
    void Reset()
    {
        Pitch = Yaw = Roll = 0;
    };
    void OnCursorMove(double const x, double const y);
    void OnCommand( command_data const &Command );
    void Update();
    vector3 GetDirection();
    bool SetMatrix();
    bool SetMatrix(glm::mat4 &Matrix);
    void SetCabMatrix( vector3 &p );
    void RaLook();
    void Stop();
    // bool GetMatrix(matrix4x4 &Matrix);
    vector3 PtNext, PtPrev;
};
