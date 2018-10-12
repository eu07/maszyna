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
#include "command.h"

//---------------------------------------------------------------------------
enum class TCameraType
{ // tryby pracy kamery
    tp_Follow, // jazda z pojazdem
    tp_Free, // stoi na scenerii
    tp_Satelite // widok z góry (nie używany)
};

class TCamera {

  public: // McZapkie: potrzebuje do kiwania na boki
    void Init( Math3D::vector3 const &Location, Math3D::vector3 const &Angle, TCameraType const Type );
    void Reset();
    void OnCursorMove(double const x, double const y);
    bool OnCommand( command_data const &Command );
    void Update();
    Math3D::vector3 GetDirection();
    bool SetMatrix(glm::dmat4 &Matrix);
    void RaLook();

    TCameraType Type;
    double Pitch;
    double Yaw; // w środku: 0=do przodu; na zewnątrz: 0=na południe
    double Roll;
    Math3D::vector3 Pos; // współrzędne obserwatora
    Math3D::vector3 LookAt; // współrzędne punktu, na który ma patrzeć
    Math3D::vector3 vUp;
    Math3D::vector3 Velocity;

private:
    glm::dvec3 m_moverate;
    glm::dvec3 m_rotationoffsets; // requested changes to pitch, yaw and roll

};
