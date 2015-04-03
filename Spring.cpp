//---------------------------------------------------------------------------

#include "system.hpp"
#include "classes.hpp"
#include "opengl/glew.h"
#include "opengl/glut.h"
#pragma hdrstop

#include "Spring.h"
#include "Usefull.h"

__fastcall TSpring::TSpring()
{
    vForce1 = vForce2 = vector3(0, 0, 0);
    Ks = 0;
    Kd = 0;
    restLen = 0;
}

__fastcall TSpring::~TSpring() {}

void TSpring::Init(double nrestLen, double nKs, double nKd)
{
    Ks = nKs;
    Kd = nKd;
    restLen = nrestLen;
}

bool TSpring::ComputateForces(vector3 pPosition1, vector3 pPosition2)
{

    double dist, Hterm, Dterm;
    vector3 springForce, deltaV, deltaP;
    //		p1 = &system[spring->p1];
    //		p2 = &system[spring->p2];
    //		VectorDifference(&p1->pos,&p2->pos,&deltaP);	// Vector distance
    deltaP = pPosition1 - pPosition2;
    //		dist = VectorLength(&deltaP);					// Magnitude of
    //deltaP
    dist = deltaP.Length();
    if (dist == 0)
    {
        vForce1 = vForce2 = vector3(0, 0, 0);
        return false;
    }

    //		Hterm = (dist - spring->restLen) * spring->Ks;	// Ks * (dist - rest)
    Hterm = (dist - restLen) * Ks; // Ks * (dist - rest)

    //		VectorDifference(&p1->v,&p2->v,&deltaV);		// Delta Velocity Vector
    deltaV = pPosition1 - pPosition2;

    //		Dterm = (DotProduct(&deltaV,&deltaP) * spring->Kd) / dist; // Damping Term
    // Dterm = (DotProduct(deltaV,deltaP) * Kd) / dist;
    Dterm = 0;

    //		ScaleVector(&deltaP,1.0f / dist, &springForce);	// Normalize Distance Vector
    //		ScaleVector(&springForce,-(Hterm + Dterm),&springForce);	// Calc Force
    springForce = deltaP / dist * (-(Hterm + Dterm));
    //		VectorSum(&p1->f,&springForce,&p1->f);			// Apply to Particle 1
    //		VectorDifference(&p2->f,&springForce,&p2->f);	// - Force on Particle 2
    vForce1 = springForce;
    vForce2 = springForce;

    return true;
}

void TSpring::Render() {}

//---------------------------------------------------------------------------

#pragma package(smart_init)
