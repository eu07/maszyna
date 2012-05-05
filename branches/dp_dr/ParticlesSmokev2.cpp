//---------------------------------------------------------------------------

#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#include "ParticlesSmokev2.h"
#include "Globals.h"
#include "opengl/glew.h"

__fastcall TParticle::TParticle()
{
}

__fastcall TParticle::~TParticle()
{
}

__fastcall TParticle::Render(TModel3d *mdmod)
{
if (ObjSqrDist<490000)
   {
    glPushMatrix();
      glTranslatef(pos.x,pos.y,pos.z);
//    glPushMatrix();
//      glScalef(2,2,2);
      float alfa=(Age>2?0.5f:Age*0.25f);
      GLfloat ambientLight[] = {0.0, 0.0, 0.0, alfa};
      glEnable(GL_LIGHT3);
      GLfloat position[] = { 0.0f, 0.0f, 0.0f, 0.0f };

// Assign created components to GL_LIGHT0
glLightfv(GL_LIGHT3, GL_AMBIENT, ambientLight);
glLightfv(GL_LIGHT3, GL_POSITION, position);


      mdmod->RenderAlpha(ObjSqrDist,0);
//      glColor4f(1.0, 1.0, 1.0, 1.0);
//    glPopMatrix();
      glDisable(GL_LIGHT3);
    glPopMatrix();
   }
}

//------------------------------

__fastcall TSmoke::TSmoke()
{
}

__fastcall TSmoke::~TSmoke()
{
}

__fastcall TSmoke::Init(float radius)
{
 ERadius=radius;

 AnsiString asModel;
 asModel="particle.t3d";
 mdParticle=TModelsManager::GetModel(asModel.c_str());

 int i;
 for (i=0;i<MaxParticles;i++)
   Particles[i].exist=false;
}

__fastcall TSmoke::CreateParticle(int number)
{
// randomize();

 Particles[number].exist=true;
 Particles[number].Age=(random(int(nAge))+nAge);
 if (Particles[number].Age>MaxAge)
   Particles[number].Age=MaxAge;
 Particles[number].V=(EV);
                               //rnd3: 0 1 2
 Particles[number].V.x=((random(31)-15)*0.07f*0.3f);
 Particles[number].V.z=((random(31)-15)*0.07f*0.3f);
 Particles[number].V.y+=((random(31)-15)*0.07f*0.3f);

 vector3 offset;

// randomize();

 offset=vector3((random(31)-15),(random(2)),(random(31)-15));

// randomize();

 offset*=(ERadius*random(31)*0.07f*0.07f);
 Particles[number].pos=(EPos+offset);
}

__fastcall TSmoke::Update(float dt, vector3 nPos, float nV, float newAge, int Part2Make)
{
 nAge=newAge;
 EPos=nPos;
 EV=vector3(0,nV,0);

 int i;
 for(i=0;i<MaxParticles;i++)
  {

//   randomize();

   if (Particles[i].exist)
    {
     Particles[i].Age-=dt;
     if (Particles[i].Age<0)
       Particles[i].exist=false;
     else
      {
       Particles[i].pos+=(Particles[i].V*dt);
       if (Particles[i].V.y>1)
         Particles[i].V.y*=(1-dt*0.67f);
      }
    }
   else
   if (Part2Make>0)
    {
     CreateParticle(i);
     --Part2Make;
    }
  }

 if (Part2Make>0)
   for (i=0;i<Part2Make;i++)
     CreateParticle(random(MaxParticles));

}

__fastcall TSmoke::Render()
{
 int i;
  TParticle tmp;
  bool change;

  for (i=0;i<MaxParticles;i++)
   {
    Particles[i].ObjSqrDist= SquareMagnitude(Global::pCameraPosition-Particles[i].pos);
   }
  for (i=0; i<MaxParticles; i++)
  {
       change=false;
       for (int j=0; j<MaxParticles-i; j++)
          if (Particles[j+1].ObjSqrDist > Particles[j].ObjSqrDist)   //porownanie sasiadow
          {
              tmp = Particles[j];
              Particles[j] = Particles[j+1];
              Particles[j+1] = tmp;    //wypchanie babelka
              change=true;
          }
       if(!change) break;      // nie dokonano zmian - koniec!
  }
 for (i=0;i<MaxParticles;i++)
   if (Particles[i].exist)
    {
     Particles[i].Render(mdParticle);
    }
}

//---------------------------------------------------------------------------

#pragma package(smart_init)








