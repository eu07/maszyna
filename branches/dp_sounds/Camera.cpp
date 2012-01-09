//---------------------------------------------------------------------------
#include    "system.hpp"
#include    "classes.hpp"

#include "opengl/glew.h"
#include "opengl/glut.h"

#pragma hdrstop

#include "Camera.h"
#include "Usefull.h"
#include "Globals.h"
#include "Timer.h"
#include "mover.hpp"

//TViewPyramid TCamera::OrgViewPyramid;
//={vector3(-1,1,1),vector3(1,1,1),vector3(-1,-1,1),vector3(1,-1,1),vector3(0,0,0)};

const vector3 OrgCrossPos= vector3(0,-10,0);

void __fastcall TCamera::Init(vector3 NPos, vector3 NAngle)
{

    pOffset= vector3(-0.0,0,0);
    vUp= vector3(0,1,0);
//    pOffset= vector3(-0.8,0,0);
    CrossPos= OrgCrossPos;
    CrossDist= 10;
    Velocity= vector3(0,0,0);
    OldVelocity= vector3(0,0,0);
    Pitch= NAngle.x;
    Yaw= NAngle.y;
    Roll= NAngle.z;
    Pos= NPos;

//    Type= tp_Follow;
    Type= (Global::bFreeFly ? tp_Free : tp_Follow);
//    Type= tp_Free;
};

void __fastcall TCamera::OnCursorMove(double x, double y)
{
//McZapkie-170402: zeby mysz dzialala zawsze    if (Type==tp_Follow)
 Pitch+=y;
 Yaw+=-x;
 if (Yaw>M_PI)
  Yaw-=2*M_PI;
 else if (Yaw<-M_PI)
  Yaw+=2*M_PI;
 if (Type==tp_Follow) //je¿eli jazda z pojazdem
 {
  Fix(Pitch,-M_PI_4,M_PI_4); //ograniczenie k¹ta spogl¹dania w dó³ i w górê
  //Fix(Yaw,-M_PI,M_PI);
 }
}

void __fastcall TCamera::Update()
{
    //ABu: zmiana i uniezaleznienie predkosci od FPS
    double a= (Pressed(VK_SHIFT)?5.00:1.00);
    if (Pressed(VK_CONTROL))
     a=a*100;
//    OldVelocity=Velocity;
    if (FreeFlyModeFlag==true)
      Type=tp_Free;
    else
      Type=tp_Follow;
    if (Type==tp_Free)
    {
        if (Pressed(Global::Keys[k_MechUp]))   Velocity.y+= a;
        if (Pressed(Global::Keys[k_MechDown])) Velocity.y-= a;
//McZapkie-170402: zeby nie bylo konfliktow
/*
        if (Pressed(VkKeyScan('d')))
            Velocity.x+= a*Timer::GetDeltaTime();
        if (Pressed(VkKeyScan('a')))
            Velocity.x-= a*Timer::GetDeltaTime();
        if (Pressed(VkKeyScan('w')))
            Velocity.z-= a*Timer::GetDeltaTime();
        if (Pressed(VkKeyScan('s')))
            Velocity.z+= a*Timer::GetDeltaTime();

        if (Pressed(VK_NUMPAD4) || Pressed(VK_NUMPAD7) || Pressed(VK_NUMPAD1))
            Yaw+= +1*M_PI*Timer::GetDeltaTime();

        if (Pressed(VK_NUMPAD6) || Pressed(VK_NUMPAD9) || Pressed(VK_NUMPAD3))
            Yaw+= -1*M_PI*Timer::GetDeltaTime();

        if (Pressed(VK_NUMPAD2) || Pressed(VK_NUMPAD1) || Pressed(VK_NUMPAD3))
            Pitch+= -1*M_PI*Timer::GetDeltaTime();

        if (Pressed(VK_NUMPAD8) || Pressed(VK_NUMPAD7) || Pressed(VK_NUMPAD9))
            Pitch+= +1*M_PI*Timer::GetDeltaTime();
        if (Pressed(VkKeyScan('.')))
            Roll+= -1*M_PI*Timer::GetDeltaTime();
        if (Pressed(VkKeyScan(',')))
            Roll+= +1*M_PI*Timer::GetDeltaTime();

        if (Pressed(VK_NUMPAD5))
            Pitch=Roll= 0.0f;
*/

//McZapkie-170402: poruszanie i rozgladanie we free takie samo jak w follow
        if (Pressed(Global::Keys[k_MechRight]))    Velocity.x+=a;
        if (Pressed(Global::Keys[k_MechLeft]))     Velocity.x-=a;
        if (Pressed(Global::Keys[k_MechForward]))  Velocity.z-=a;
        if (Pressed(Global::Keys[k_MechBackward])) Velocity.z+=a;
//gora-dol
        if (Pressed(VK_NUMPAD9)) Pos.y+=0.1;
        if (Pressed(VK_NUMPAD3)) Pos.y-=0.1;

//McZapkie: zeby nie hustalo przy malym FPS:
//        Velocity= (Velocity+OldVelocity)/2;
//    matrix4x4 mat;
        vector3 Vec=Velocity;
        Vec.RotateY(Yaw);
        Pos=Pos+Vec*Timer::GetDeltaRenderTime(); //czas bez pauzy
        Velocity=Velocity/2; //p³ynne hamowanie ruchu
//    double tmp= 10*DeltaTime;
//        Velocity+= -Velocity*10 * Timer::GetDeltaTime();//( tmp<1 ? tmp : 1 );
//        Type= tp_Free;
    }

}

void __fastcall TCamera::Render()
{


}


vector3 __fastcall TCamera::GetDirection()
{
    matrix4x4 mat;
    vector3 Vec;
    Vec= vector3(0,0,1);
    Vec.RotateY(Yaw);

    return (Normalize(Vec));
}

//bool __fastcall TCamera::GetMatrix(matrix4x4 &Matrix)
bool __fastcall TCamera::SetMatrix()
{
    glRotated(-Roll*180.0f/M_PI,0,0,1);
    glRotated(-Pitch*180.0f/M_PI,1,0,0);
    glRotated(-Yaw*180.0f/M_PI,0,1,0); //w zewnêtrznym widoku: kierunek patrzenia

    if (Type==tp_Follow)
    {
//        gluLookAt(Pos.x+pOffset.x,Pos.y+pOffset.y,Pos.z+pOffset.z,
//                LookAt.x+pOffset.x,LookAt.y+pOffset.y,LookAt.z+pOffset.z,vUp.x,vUp.y,vUp.z);
//        gluLookAt(Pos.x+pOffset.x,Pos.y+pOffset.y,Pos.z+pOffset.z,
//                LookAt.x+pOffset.x,LookAt.y+pOffset.y,LookAt.z+pOffset.z,vUp.x,vUp.y,vUp.z);
     gluLookAt(Pos.x,Pos.y,Pos.z,LookAt.x,LookAt.y,LookAt.z,vUp.x,vUp.y,vUp.z); //Ra: pOffset is zero
//        gluLookAt(Pos.x,Pos.y,Pos.z,Pos.x+Velocity.x,Pos.y+Velocity.y,Pos.z+Velocity.z,0,1,0);
//        return true;
    }

    if (Type==tp_Satelite)
        Pitch= M_PI*0.5;

    if (Type!=tp_Follow)
    {
     glTranslated(-Pos.x,-Pos.y,-Pos.z); //nie zmienia kierunku patrzenia
    }

    Global::SetCameraPosition(Pos); //by³o +pOffset
    return true;
}

void __fastcall TCamera::RaLook()
{//zmiana kierunku patrzenia - przelicza Yaw
 vector3 where=LookAt-Pos+vector3(0,3,0); //trochê w górê od szyn
 if ((where.x!=0.0)||(where.z!=0.0))
  Yaw=atan2(-where.x,-where.z); //k¹t horyzontalny
 double l=Length3(where);
 if (l>0.0)
  Pitch=asin(where.y/l); //k¹t w pionie
};
//---------------------------------------------------------------------------
#pragma package(smart_init)





