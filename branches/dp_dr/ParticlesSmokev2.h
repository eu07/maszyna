//---------------------------------------------------------------------------

#ifndef ParticlesSmokev2H
#define ParticlesSmokev2H

#include "MdlMngr.h"

const float MaxAge=30.0f;
const int MaxParticles=1500;

class TParticle
{
public:
  __fastcall TParticle();
  __fastcall ~TParticle();
  __fastcall Render(TModel3d *mdmod);
  vector3 pos;
  vector3 V;
//  float Rotation;
  float Age;
  float ObjSqrDist;
  bool exist;
  bool firstC;
};


class TSmoke
{
private:
  TModel3d *mdParticle;
  TParticle Particles[MaxParticles+1];
  vector3 EPos;
  vector3 EV;
  float ERadius;
  float nAge;
  __fastcall CreateParticle(int number);
public:
  __fastcall Update(float dt, vector3 nPos, float nV, float newAge, int Part2Make);
  __fastcall Render();
  __fastcall Init(float radius);
  __fastcall TSmoke();
  __fastcall ~TSmoke();
};

//---------------------------------------------------------------------------
#endif
