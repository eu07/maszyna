/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "Spring.h"

void TSpring::Init(double nKs, double nKd) {
    Ks = nKs;
    Kd = nKd;
    ks = Ks;
    kd = Kd;
}

Math3D::vector3 TSpring::ComputateForces( Math3D::vector3 const &pPosition1, Math3D::vector3 const &pPosition2) {

    Math3D::vector3 springForce;
    //		p1 = &system[spring->p1];
    //		p2 = &system[spring->p2];
    //		VectorDifference(&p1->pos,&p2->pos,&deltaP);	// Vector distance
    auto deltaP = pPosition1 - pPosition2;
    //		dist = VectorLength(&deltaP);					// Magnitude of deltaP
    auto dist = deltaP.Length();
    if( dist > restLen ) {

        //		Hterm = (dist - spring->restLen) * spring->Ks;	// Ks * (dist - rest)
        auto Hterm = ( dist - restLen ) * Ks; // Ks * (dist - rest)

        //		VectorDifference(&p1->v,&p2->v,&deltaV);		// Delta Velocity Vector
        auto deltaV = pPosition1 - pPosition2;

        //		Dterm = (DotProduct(&deltaV,&deltaP) * spring->Kd) / dist; // Damping Term
        auto Dterm = (DotProduct(deltaV,deltaP) * Kd) / dist;
        //Dterm = 0;

        //		ScaleVector(&deltaP,1.0f / dist, &springForce);	// Normalize Distance Vector
        //		ScaleVector(&springForce,-(Hterm + Dterm),&springForce);	// Calc Force
        springForce = deltaP / dist * ( -( Hterm + Dterm ) );
        //		VectorSum(&p1->f,&springForce,&p1->f);			// Apply to Particle 1
        //		VectorDifference(&p2->f,&springForce,&p2->f);	// - Force on Particle 2
    }

    return springForce;
}

//---------------------------------------------------------------------------
