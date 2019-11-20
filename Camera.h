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
#include "DynObj.h"

//---------------------------------------------------------------------------

class TCamera {

  public: // McZapkie: potrzebuje do kiwania na boki
    void Init( Math3D::vector3 const &Location, Math3D::vector3 const &Angle, TDynamicObject *Owner );
    void Reset();
    void OnCursorMove(double const x, double const y);
    bool OnCommand( command_data const &Command );
    void Update();
    bool SetMatrix(glm::dmat4 &Matrix);
    void RaLook();

    Math3D::vector3 Angle; // pitch, yaw, roll
    Math3D::vector3 Pos; // współrzędne obserwatora
    Math3D::vector3 LookAt; // współrzędne punktu, na który ma patrzeć
    Math3D::vector3 vUp;
    Math3D::vector3 Velocity;

    TDynamicObject *m_owner { nullptr }; // TODO: change to const when shake calculations are part of vehicles update
    Math3D::vector3 m_owneroffset {};

private:
    glm::dvec3 m_moverate;
    glm::dvec3 m_rotationoffsets; // requested changes to pitch, yaw and roll

};

struct viewport_proj_config {
    glm::vec3 pa; // screen lower left corner
    glm::vec3 pb; // screen lower right corner
    glm::vec3 pc; // screen upper left corner
    glm::vec3 pe; // eye position
};
