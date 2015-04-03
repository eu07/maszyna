//---------------------------------------------------------------------------

#ifndef ParticlesH
#define ParticlesH

#define STATIC_THRESHOLD 0.17f
// efine STATIC_THRESHOLD	0.03f
const double m_Kd = 0.02f; // DAMPING FACTOR
const double m_Kr = 0.8f;  // 1.0 = SUPERBALL BOUNCE 0.0 = DEAD WEIGHT
const double m_Ksh = 5.0f; // HOOK'S SPRING CONSTANT
const double m_Ksd = 0.1f; // SPRING DAMPING CONSTANT

const double m_Csf = 0.9f; // Default Static Friction
const double m_Ckf = 0.7f; // Default Kinetic Friction

#include "dumb3d.h"
using namespace Math3D;

class TSpring
{
  public:
    TSpring();
    ~TSpring();
    //    void Init(TParticnp1, TParticle *np2, double nKs= 0.5f, double nKd= 0.002f,
    //    double nrestLen= -1.0f);
    void Init(double nrestLen, double nKs = 0.5f, double nKd = 0.002f);
    bool ComputateForces(vector3 pPosition1, vector3 pPosition2);
    void Render();
    vector3 vForce1, vForce2;
    double restLen; // LENGTH OF SPRING AT REST
    double Ks;      // SPRING CONSTANT
    double Kd;      // SPRING DAMPING
  private:
};

//---------------------------------------------------------------------------
#endif
