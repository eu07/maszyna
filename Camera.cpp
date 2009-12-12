//---------------------------------------------------------------------------
#include    "system.hpp"
#include    "classes.hpp"

#pragma hdrstop

#include "Camera.h"
#include "Usefull.h"
#include "Globals.h"
#include "Timer.h"

//TViewPyramid TCamera::OrgViewPyramid;
//={vector3(-1,1,1),vector3(1,1,1),vector3(-1,-1,1),vector3(1,-1,1),vector3(0,0,0)};
#include <gl/gl.h>
#include <gl/glu.h>
#include "opengl/glut.h"

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

bool __fastcall TCamera::OnCursorMove(double x, double y)
{
//McZapkie-170402: zeby mysz dzialala zawsze    if (Type==tp_Follow)
    {
        Pitch+= y;
        Yaw+= -x;
            if (Yaw>M_PI)
               Yaw-=M_PI+M_PI;
            else
               if (Yaw<-M_PI)
                  Yaw+=M_PI+M_PI;
        if (Type==tp_Follow)
        {
            Fix(Pitch,-M_PI_4,M_PI_4);
            //Fix(Yaw,-M_PI,M_PI);
        }

    }

}

bool __fastcall TCamera::Update()
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
        if (Pressed(Global::Keys[k_MechUp]))
            Velocity.y+= a;//*Timer::GetDeltaTime();
        if (Pressed(Global::Keys[k_MechDown]))
            Velocity.y-= a;//*Timer::GetDeltaTime();
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
        if (Pressed(Global::Keys[k_MechRight]))
            Velocity.x+= a;//*Timer::GetDeltaTime();
        if (Pressed(Global::Keys[k_MechLeft]))
            Velocity.x-= a;//*Timer::GetDeltaTime();
        if (Pressed(Global::Keys[k_MechForward]))
            Velocity.z-= a;//*Timer::GetDeltaTime();
        if (Pressed(Global::Keys[k_MechBackward]))
            Velocity.z+= a;//*Timer::GetDeltaTime();
//gora-dol
        if (Pressed(VK_NUMPAD9))
            Pos.y+=0.1;
        if (Pressed(VK_NUMPAD3))
            Pos.y-=0.1;

//McZapkie: zeby nie hustalo przy malym FPS:
//        Velocity= (Velocity+OldVelocity)/2;
//    matrix4x4 mat;
        vector3 Vec;
        Vec= Velocity;
        Vec.RotateY(Yaw);
        Pos= Pos+Vec*Timer::GetDeltaTime();
        Velocity= Velocity/2;
//    double tmp= 10*DeltaTime;
//        Velocity+= -Velocity*10 * Timer::GetDeltaTime();//( tmp<1 ? tmp : 1 );
//        Type= tp_Free;
    }

}

bool __fastcall TCamera::Render()
{


}


vector3 __fastcall TCamera::GetDirection()
{
    matrix4x4 mat;
    vector3 Vec;
    Vec= vector3(0,0,1);
    Vec.RotateY(Yaw);

    return(Normalize(Vec));
}

//bool __fastcall TCamera::GetMatrix(matrix4x4 &Matrix)
bool __fastcall TCamera::SetMatrix()
{
/*
    if (Pitch>M_PI*0.25)
        Pitch= M_PI*0.25;

    if (Pitch<-M_PI*0.05)
        Pitch= -M_PI*0.05;

    if (Yaw>M_PI*0.33)
        Yaw= M_PI*0.33;

    if (Yaw<-M_PI*0.33)
        Yaw= -M_PI*0.33;

  */
    glRotatef(-Roll*180.0f/M_PI,0,0,1);
    glRotatef(-Pitch*180.0f/M_PI,1,0,0);
    glRotatef(-Yaw*180.0f/M_PI,0,1,0);

    if (Type==tp_Follow)
    {
        gluLookAt(Pos.x+pOffset.x,Pos.y+pOffset.y,Pos.z+pOffset.z,
                LookAt.x+pOffset.x,LookAt.y+pOffset.y,LookAt.z+pOffset.z,vUp.x,vUp.y,vUp.z);
//        gluLookAt(Pos.x,Pos.y,Pos.z,Pos.x+Velocity.x,Pos.y+Velocity.y,Pos.z+Velocity.z,0,1,0);
//        return true;
    }

    if (Type==tp_Satelite)
        Pitch= M_PI*0.5;

    if (Type!=tp_Follow)
    {
        glTranslatef(-Pos.x,-Pos.y,-Pos.z);
    }

    Global::SetCameraPosition(Pos+pOffset);
    Global::SetCameraRotation(Yaw);    
    return true;
}
//---------------------------------------------------------------------------
#pragma package(smart_init)





