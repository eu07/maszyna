//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software                                     
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "system.hpp"
#include "classes.hpp"
#include "opengl/glew.h"
#include "opengl/glut.h"

#pragma hdrstop


#include "World.h"
#include "logs.h"
#include "Globals.h"
#include "lights.h"


  GLfloat  LU_pos[] = {  10.0f, 0.0, 0.0f, 1.0f };
  GLfloat  LL_pos[] = {  0.0f, 0.0f, 0.0f, 1.0f };
  GLfloat  LR_pos[] = { -10.0f, 0.0f, 0.0f, 1.0f };

  GLfloat  LR_rot[] = { 0.0f, 0.0f, 0.0f, 1.0f };
  GLfloat  LL_rot[] = { 0.0f, 0.0f, 0.0f, 1.0f };
  GLfloat  LU_rot[] = { 0.0f, 0.0f, 0.0f, 1.0f };

  GLfloat  LU_dir[] = {  0.0f, -1.0f, 30.0f };
  GLfloat  LL_dir[] = { -2.0f, -1.0f, 10.0f };
  GLfloat  LR_dir[] = {  2.0f, -1.0f, 10.0f };


double TLights::ltxp = 0.0;
double TLights::ltyp = 0.0;
double TLights::ltzp = 0.0;

double TLights::sltxp = 0.0;
double TLights::sltyp = 0.0;
double TLights::sltzp = 0.0;

double TLights::ltsdxp = 0.0;
double TLights::ltsdyp = 0.0;
double TLights::ltsdzp = 0.0;

double TLights::sltsdxp = 0.0;
double TLights::sltsdyp = 0.0;
double TLights::sltsdzp = 0.0;

bool TLights::bcablight1 = false;
bool TLights::bcablight2 = false;
bool TLights::breflightL = false;
bool TLights::breflightR = false;
bool TLights::breflightU = false;

float TLights::CAB1Ar = 1.0;
float TLights::CAB1Ag = 0.3;
float TLights::CAB1Ab = 0.0;
float TLights::CAB1Dr = 0.8;
float TLights::CAB1Dg = 0.0;
float TLights::CAB1Db = 0.0;
float TLights::CAB1Sr = 0.4;
float TLights::CAB1Sg = 0.0;
float TLights::CAB1Sb = 0.0;

int TLights::useSAME_AMB_SPEC = false;
int TLights::currentcab = 1;



__fastcall TLights::TLights()
{

}

__fastcall TLights::~TLights()
{

}

__fastcall TLights::cablight_0()
{
    int lt = GL_LIGHT0 + 2;
    Light *light = &spots[2];
    light->amb[0] = CAB1Ar*Global::cablightint;
    light->amb[1] = CAB1Ag*Global::cablightint;
    light->amb[2] = CAB1Ab*Global::cablightint;
    light->diff[0] = CAB1Dr*Global::cablightint;
    light->diff[1] = CAB1Dg*Global::cablightint;
    light->diff[2] = CAB1Db*Global::cablightint;
    light->spec[0] = CAB1Sr;
    light->spec[1] = CAB1Sg;
    light->spec[2] = CAB1Sb;

    glLightfv(lt, GL_AMBIENT, light->amb);
    glLightfv(lt, GL_DIFFUSE, light->diff);
    glLightfv(lt, GL_SPECULAR, light->spec);
}

__fastcall TLights::loadlights()
{
 AnsiString LINE, TEST, PAR, asModel;
 TStringList *SKYCFG;
 SKYCFG = new TStringList;
 SKYCFG->LoadFromFile(Global::g___APPDIR + "data\\CAB.txt");

 WriteLog("LOADING CAB CONFIG...");

 CAB1Ar = 1.0;
 CAB1Ag = 0.3;
 CAB1Ab = 0.0;
 CAB1Dr = 0.8;
 CAB1Dg = 0.0;
 CAB1Db = 0.0;
 CAB1Sr = 0.4;
 CAB1Sg = 0.0;
 CAB1Sb = 0.0;

 for (int l= 0; l < SKYCFG->Count; l++)
      {
       LINE = SKYCFG->Strings[l];
       TEST = LINE.SubString(1, LINE.Pos(":"));
       PAR =  LINE.SubString(LINE.Pos(":")+2, 255);
       WriteLog(TEST + ", [" + PAR + "]");

       if (TEST == "CAB1AR:") CAB1Ar = StrToFloat(PAR);
       if (TEST == "CAB1AG:") CAB1Ag = StrToFloat(PAR);
       if (TEST == "CAB1AB:") CAB1Ab = StrToFloat(PAR);
       if (TEST == "CAB1DR:") CAB1Dr = StrToFloat(PAR);
       if (TEST == "CAB1DG:") CAB1Dg = StrToFloat(PAR);
       if (TEST == "CAB1DB:") CAB1Db = StrToFloat(PAR);
       if (TEST == "CAB1SR:") CAB1Sr = StrToFloat(PAR);
       if (TEST == "CAB1SG:") CAB1Sg = StrToFloat(PAR);
       if (TEST == "CAB1SB:") CAB1Sb = StrToFloat(PAR);
      }

    int lt = GL_LIGHT0 + 2;
    Light *light = &spots[2];
    light->amb[0] = CAB1Ar;
    light->amb[1] = CAB1Ag;
    light->amb[2] = CAB1Ab;
    light->diff[0] = CAB1Dr;
    light->diff[1] = CAB1Dg;
    light->diff[2] = CAB1Db;
    light->spec[0] = CAB1Sr;
    light->spec[1] = CAB1Sg;
    light->spec[2] = CAB1Sb;

    glLightfv(lt, GL_AMBIENT, light->amb);
    glLightfv(lt, GL_DIFFUSE, light->diff);
    glLightfv(lt, GL_SPECULAR, light->spec);

    setcablights("201e-w", 1);
    delete SKYCFG;
}

__fastcall TLights::InitLights()
{

  for (int k = 1; k < NUM_LIGHTS; ++k) {
    int lt = GL_LIGHT0 + k;
    Light *light = &spots[k];

    glEnable(lt);
    glLightfv(lt, GL_AMBIENT, light->amb);
    glLightfv(lt, GL_DIFFUSE, light->diff);

    if (useSAME_AMB_SPEC) glLightfv(lt, GL_SPECULAR, light->amb);
    else glLightfv(lt, GL_SPECULAR, light->spec);

    glLightf(lt, GL_SPOT_EXPONENT, light->spotExp);
    glLightf(lt, GL_SPOT_CUTOFF, light->spotCutoff);
    glLightf(lt, GL_CONSTANT_ATTENUATION, light->atten[0]);
    glLightf(lt, GL_LINEAR_ATTENUATION, light->atten[1]);
    glLightf(lt, GL_QUADRATIC_ATTENUATION, light->atten[2]);


  }
     glDisable(GL_LIGHT1);
     glDisable(GL_LIGHT2);
     glDisable(GL_LIGHT3);
     glDisable(GL_LIGHT4);
     glDisable(GL_LIGHT5);
     glDisable(GL_LIGHT6);
};


__fastcall TLights::AimLights()
{
  int k;

  for (k = 1; k < NUM_LIGHTS; ++k)
  {
    Light *light = &spots[k];

    light->rot[0] = light->swing[0] * sin(light->arc[0]);
    light->arc[0] += light->arcIncr[0];
    if (light->arc[0] > TWO_PI)
      light->arc[0] -= TWO_PI;

    light->rot[1] = light->swing[1] * sin(light->arc[1]);
    light->arc[1] += light->arcIncr[1];
    if (light->arc[1] > TWO_PI)
      light->arc[1] -= TWO_PI;

    light->rot[2] = light->swing[2] * sin(light->arc[2]);
    light->arc[2] += light->arcIncr[2];
    if (light->arc[2] > TWO_PI)
      light->arc[2] -= TWO_PI;

  }
}

__fastcall TLights::DrawLights()
{
  int k;

  for (k = 2; k < NUM_LIGHTS; ++k)              // k = 2
  {
    Light *light = &spots[k];
  }
}



__fastcall TLights::SetLights(float lx, float ly, float lz)
{

  int k;

  for (k = 1; k < NUM_LIGHTS; ++k) {
    int lt = GL_LIGHT0 + k;
    Light *light = &spots[k];

    glPushMatrix();

    if (k == 1)    glTranslatef(lx+0, ly+0, lz+ -17);  // L
    if (k == 2)    glTranslatef(lx+0, ly+0, lz+ -17);  // C
    if (k == 3)    glTranslatef(lx+0, ly+0, lz+ -17);  // R

//  glRotatef(19.0, 1, 0, 0);
//  glRotatef(light->rot[1], 0, 1, 0);
//  glRotatef(39.0, 0, 0, 1);


//    if ((ax == true) && (ab == false))
//    {
//    ab = true;
//    }

    glLightf(lt, GL_SPOT_EXPONENT, light->spotExp);
    glLightf(lt, GL_SPOT_CUTOFF, light->spotCutoff);
    glLightf(lt, GL_CONSTANT_ATTENUATION, light->atten[0]);
    glLightf(lt, GL_LINEAR_ATTENUATION, light->atten[1]);
    glLightf(lt, GL_QUADRATIC_ATTENUATION, light->atten[2]);
    glLightfv(lt, GL_POSITION, light->pos);
    glLightfv(lt, GL_SPOT_DIRECTION, light->spotDir);
    glPopMatrix();
  }

}

__fastcall TLights::setheadlightL(int cabnum)
{
    int k=3;                       // 3
    int lt = GL_LIGHT0 + k;
    Light *light = &spots[k];

    sltxp= -1.2;
    sltyp= 1.5;
    sltzp= 6.0;
    sltsdxp= -1.5;
    sltsdyp= 1.1;
    sltsdzp= 43.6;
           
    // SPOT POSITION
    light->pos[0] = -1; //sltxp;
    light->pos[1] = 17; //sltyp;
    light->pos[2] =0; //sltzp;

    // SPOT DIRECTION
    light->spotDir[0] = -1; //sltsdxp;
    light->spotDir[1] = -4; //sltsdyp;
    light->spotDir[2] = 0; //sltsdzp;
    glEnable(lt);
//    glPushMatrix();

//    if (k == 1)    glTranslatef(lx+0, ly+0, lz+ -17);  // L
//    if (k == 2)    glTranslatef(lx+0, ly+0, lz+ -17);  // C
//    if (k == 2)    glTranslatef(pos.x, pos.y, pos.z);  // R

//  glRotatef(19.0, 1, 0, 0);
//  glRotatef(light->rot[1], 0, 1, 0);
//  glRotatef(39.0, 0, 0, 1);



    glLightf(lt, GL_SPOT_EXPONENT, light->spotExp);
    glLightf(lt, GL_SPOT_CUTOFF, light->spotCutoff);
    glLightf(lt, GL_CONSTANT_ATTENUATION, light->atten[0]);
    glLightf(lt, GL_LINEAR_ATTENUATION, light->atten[1]);
    glLightf(lt, GL_QUADRATIC_ATTENUATION, light->atten[2]);
    glLightfv(lt, GL_POSITION, light->pos);
    glLightfv(lt, GL_SPOT_DIRECTION, light->spotDir);
//    glPopMatrix();

     glBegin(GL_LINES);
     glVertex3f(light->pos[0], light->pos[1], light->pos[2]);
     glVertex3f(light->spotDir[0], light->spotDir[1], light->spotDir[2]);
     glEnd();
}

__fastcall TLights::setheadlightR(int cabnum)
{
    int k=4;                       // 2
    int lt = GL_LIGHT0 + k;
    Light *light = &spots[k];

    sltxp= 1.2;
    sltyp= 1.5;
    sltzp= 6.0;
    sltsdxp= 1.5;
    sltsdyp= 1.1;
    sltsdzp= 43.6;

    // SPOT POSITION
    light->pos[0] = sltxp;
    light->pos[1] = sltyp;
    light->pos[2] = sltzp;

    // SPOT DIRECTION
    light->spotDir[0] = sltsdxp;
    light->spotDir[1] = sltsdyp;
    light->spotDir[2] = sltsdzp;

     glBegin(GL_LINES);
     glVertex3f(light->pos[0], light->pos[1], light->pos[2]);
     glVertex3f(light->spotDir[0], light->spotDir[1], light->spotDir[2]);
     glEnd();

     glPushMatrix();
     glLightf(lt, GL_SPOT_EXPONENT, light->spotExp);
     glLightf(lt, GL_SPOT_CUTOFF, light->spotCutoff);
     glLightf(lt, GL_CONSTANT_ATTENUATION, light->atten[0]);
     glLightf(lt, GL_LINEAR_ATTENUATION, light->atten[1]);
     glLightf(lt, GL_QUADRATIC_ATTENUATION, light->atten[2]);
     glLightfv(lt, GL_POSITION, light->pos);
     glLightfv(lt, GL_SPOT_DIRECTION, light->spotDir);
     glPopMatrix();
}

__fastcall TLights::setheadlightU(int cabnum)
{
    int k=2;                       // 2
    int lt = GL_LIGHT0 + k;
    Light *light = &spots[k];

    sltxp= 0.0;
    sltyp= 3.7;
    sltzp= 6.0;
    sltsdxp= 0.0;
    sltsdyp= 1.1;
    sltsdzp= 53.6;

    // SPOT POSITION
    light->pos[0] = sltxp;
    light->pos[1] = sltyp;
    light->pos[2] = sltzp;

    // SPOT DIRECTION
    light->spotDir[0] = sltsdxp;
    light->spotDir[1] = sltsdyp;
    light->spotDir[2] = sltsdzp;

     glBegin(GL_LINES);
     glVertex3f(light->pos[0], light->pos[1], light->pos[2]);
     glVertex3f(light->spotDir[0], light->spotDir[1], light->spotDir[2]);
     glEnd();

     glPushMatrix();
     glLightf(lt, GL_SPOT_EXPONENT, light->spotExp);
     glLightf(lt, GL_SPOT_CUTOFF, light->spotCutoff);
     glLightf(lt, GL_CONSTANT_ATTENUATION, light->atten[0]);
     glLightf(lt, GL_LINEAR_ATTENUATION, light->atten[1]);
     glLightf(lt, GL_QUADRATIC_ATTENUATION, light->atten[2]);
     glLightfv(lt, GL_POSITION, light->pos);
     glLightfv(lt, GL_SPOT_DIRECTION, light->spotDir);
     glPopMatrix();
}

__fastcall TLights::setcablightA(int cabnum, vector3 pos, vector3 dir)
{
    int k=2;                       // 2
    int lt = GL_LIGHT0 + k;
    Light *light = &spots[k];

    // SPOT POSITION
    light->pos[0] = pos.x; //sltxp;
    light->pos[1] = pos.y; //sltyp;
    light->pos[2] = pos.z; //sltzp;

    // SPOT DIRECTION
    light->spotDir[0] = dir.x; //sltsdxp;
    light->spotDir[1] = dir.y; //sltsdyp;
    light->spotDir[2] = dir.z; //sltsdzp;

//     glBegin(GL_LINES);
//     glVertex3f(light->pos[0], light->pos[1], light->pos[2]);
//     glVertex3f(light->spotDir[0], light->spotDir[1], light->spotDir[2]);
//     glEnd();


//     glPushMatrix();
     glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, light->spotExp);
     glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, light->spotCutoff);
     glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, light->atten[0]);
     glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, light->atten[1]);
     glLightf(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, light->atten[2]);
     glLightfv(GL_LIGHT2, GL_POSITION, light->pos);
     glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, light->spotDir);
//     glPopMatrix();

}



__fastcall TLights::setcablightB(int cabnum, vector3 pos, vector3 dir)
{


}


// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// SWIATLA LAMP ULICZNYCH ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
__fastcall TLights::setlightposA()
{
           sltxp= 0.0;
           sltyp= 3.5;
           sltzp= 4.5;
           sltsdxp= 0.0;
           sltsdyp= 3.2;
           sltsdzp= 5.6;

    int k=2;
    int lt = GL_LIGHT0 + k;
    Light *light = &spots[k];

    // SPOT POSITION
    light->pos[0] = sltxp;
    light->pos[1] = sltyp;
    light->pos[2] = sltzp;

    // SPOT DIRECTION
    light->spotDir[0] = sltsdxp;
    light->spotDir[1] = sltsdyp;
    light->spotDir[2] = sltsdzp;

     glBegin(GL_LINES);
     glVertex3f(light->pos[0], light->pos[1], light->pos[2]);
     glVertex3f(light->spotDir[0], light->spotDir[1], light->spotDir[2]);
     glEnd();

     glEnable(GL_LIGHTING);
     glEnable(GL_LIGHT1);
     glPushMatrix();
     glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, light->spotExp);
     glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, light->spotCutoff);
     glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, light->atten[0]);
     glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, light->atten[1]);
     glLightf(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, light->atten[2]);
     glLightfv(GL_LIGHT2, GL_POSITION, light->pos);
     glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, light->spotDir);
     glPopMatrix();

}


// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// USTAWIENIE POZYCJI SWIATLA I KIERUNKOW PADANIA ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

__fastcall TLights::setcablights(AnsiString ltype, int cabnum)
{
 currentcab = cabnum;

 if (cabnum == 1)
     {
      if (ltype == "201e")
          {
           sltxp= 0.0;
           sltyp= 2.5;
           sltzp= 5.2;
           sltsdxp= 0.0;
           sltsdyp= 0.2;
           sltsdzp= 5.6;
          }

      if (ltype == "201e-w")
          {
           sltxp= 0.0;
           sltyp= 2.5;
           sltzp= 5.2;
           sltsdxp= 0.0;
           sltsdyp= 0.2;
           sltsdzp= 5.6;
          }

      if (ltype == "303e")
          {
           sltxp= -0.8 + ltxp;
           sltyp= 5.4 + ltyp;         // 4.7
           sltzp= 3.0 + ltzp;         // 5.5
           sltsdxp= -0.1 + ltsdxp;
           sltsdyp= -3.4 + ltsdyp;    // -39
           sltsdzp= 5.4 + ltsdzp;      // 10.8
          }

      if (ltype == "060da")
          {
           sltxp= 0.1;
           sltyp= 9.7;
           sltzp= 5.5;
           sltsdxp= 0.0;
           sltsdyp= -39.1;
           sltsdzp= 10.8;
          }
     }

 if (cabnum == -1)
     {
      if (ltype == "201e")
          {
           sltxp= 0.0;
           sltyp= -2.5;
           sltzp= -5.2;
           sltsdxp= 0.0;
           sltsdyp= -0.2;
           sltsdzp= -5.6;
          }

      if (ltype == "201e-w")
          {
           sltxp= 0.0;
           sltyp= -2.5;
           sltzp= -5.2;
           sltsdxp= 0.0;
           sltsdyp= -0.2;
           sltsdzp= -5.6;
          }

      if (ltype == "303e")
          {
           sltxp= 0.8;
           sltyp=  5.4;
           sltzp= -3.1;
           sltsdxp= 0.1;
           sltsdyp= -3.4;
           sltsdzp= -5.4;
          }
     }
}



//---------------------------------------------------------------------------
#pragma package(smart_init)




