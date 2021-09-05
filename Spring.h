/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#ifndef ParticlesH
#define ParticlesH

#include "dumb3d.h"
/*
#define STATIC_THRESHOLD 0.17f
const double m_Kd = 0.02f; // DAMPING FACTOR
const double m_Kr = 0.8f; // 1.0 = SUPERBALL BOUNCE 0.0 = DEAD WEIGHT
const double m_Ksh = 5.0f; // HOOK'S SPRING CONSTANT
const double m_Ksd = 0.1f; // SPRING DAMPING CONSTANT

const double m_Csf = 0.9f; // Default Static Friction
const double m_Ckf = 0.7f; // Default Kinetic Friction
*/
class TSpring {

public:
    TSpring() = default;
    //    void Init(TParticnp1, TParticle *np2, double nKs= 0.5f, double nKd= 0.002f,
    //    double nrestLen= -1.0f);
    void Init(double nKs = 0.5f, double nKd = 0.002f);
    Math3D::vector3 ComputateForces( Math3D::vector3 const &pPosition1, Math3D::vector3 const &pPosition2);
//private:
// members
    double restLen { 0.01 }; // LENGTH OF SPRING AT REST
    double Ks { 0.0 }; // SPRING CONSTANT
    double Kd { 0.0 }; // SPRING DAMPING
    float ks{ 0.f };
    float kd{ 0.f };
};

//---------------------------------------------------------------------------
#endif
