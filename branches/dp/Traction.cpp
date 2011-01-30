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

#include    "system.hpp"
#include    "classes.hpp"
#pragma hdrstop

#include "Traction.h"
#include "mctools.hpp"
#include "Globals.h"
#include "Usefull.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

TTraction::TTraction()//:TNode()
{
    pPoint1=pPoint2=pPoint3=pPoint4= vector3(0,0,0);
    vFront=vector3(0,0,1);
    vUp=vector3(0,1,0);
    vLeft=vector3(1,0,0);
    fHeightDifference=0;
    iNumSections=0;
    iLines=0;
//    dwFlags= 0;
    Wires=2;
//    fU=fR= 0;
    uiDisplayList= glGenLists(1);
    glNewList(uiDisplayList,GL_COMPILE);
    asPowerSupplyName="";
//    mdPole= NULL;
//    ReplacableSkinID= 0;
}

TTraction::~TTraction()
{
    glDeleteLists(uiDisplayList,1);
}

void __fastcall TTraction::Optimize()
{
    glNewList(uiDisplayList,GL_COMPILE);

    glBindTexture(GL_TEXTURE_2D, 0);
//    glColor3ub(0,0,0); McZapkie: to do render

//    glPushMatrix();
//    glTranslatef(pPosition.x,pPosition.y,pPosition.z);

  if (Wires!=0)
  {
      //Dlugosc odcinka trakcji 'Winger
      double ddp=hypot(pPoint2.x-pPoint1.x,pPoint2.z-pPoint1.z);

      if (Wires==2) WireOffset=0;
      //Przewoz jezdny 1 'Marcin
      glBegin(GL_LINE_STRIP);
          glVertex3f(pPoint1.x-(pPoint2.z/ddp-pPoint1.z/ddp)*WireOffset,pPoint1.y,pPoint1.z-(-pPoint2.x/ddp+pPoint1.x/ddp)*WireOffset);
          glVertex3f(pPoint2.x-(pPoint2.z/ddp-pPoint1.z/ddp)*WireOffset,pPoint2.y,pPoint2.z-(-pPoint2.x/ddp+pPoint1.x/ddp)*WireOffset);
      glEnd();
      //Nie wiem co 'Marcin
      vector3 pt1,pt2,pt3,pt4,v1,v2;
      v1= pPoint4-pPoint3;
      v2= pPoint2-pPoint1;
      float step= 0;
      if (iNumSections>0)
        step= 1.0f/(float)iNumSections;
      float f= step;
      float mid= 0.5;
      float t;

      //Przewod nosny 'Marcin
      if (Wires != 1)
      {
       glBegin(GL_LINE_STRIP);
           glVertex3f(pPoint3.x,pPoint3.y,pPoint3.z);
           for (int i=0; i<iNumSections-1; i++)
           {
               pt3= pPoint3+v1*f;
               t= (1-fabs(f-mid)*2);
               glVertex3f(pt3.x,pt3.y-sqrt(t)*fHeightDifference,pt3.z);
               f+= step;
           }
           glVertex3f(pPoint4.x,pPoint4.y,pPoint4.z);
       glEnd();
       }

      //Drugi przewod jezdny 'Winger
      if (Wires == 3)
      {
      glBegin(GL_LINE_STRIP);
          glVertex3f(pPoint1.x+(pPoint2.z/ddp-pPoint1.z/ddp)*WireOffset,pPoint1.y,pPoint1.z+(-pPoint2.x/ddp+pPoint1.x/ddp)*WireOffset);
          glVertex3f(pPoint2.x+(pPoint2.z/ddp-pPoint1.z/ddp)*WireOffset,pPoint2.y,pPoint2.z+(-pPoint2.x/ddp+pPoint1.x/ddp)*WireOffset);
      glEnd();
      }

      f= step;

      //Przewody pionowe (wieszaki) 'Marcin, poprawki na 2 przewody jezdne 'Winger
      if (Wires != 1)
      {
       glBegin(GL_LINES);
           for (int i=0; i<iNumSections-1; i++)
           {
               pt3= pPoint3+v1*f;
               pt4= pPoint1+v2*f;
               t= (1-fabs(f-mid)*2);
               if ((i%2) == 0)
               {
               glVertex3f(pt3.x,pt3.y-sqrt(t)*fHeightDifference,pt3.z);
               glVertex3f(pt4.x-(pPoint2.z/ddp-pPoint1.z/ddp)*WireOffset,pt4.y,pt4.z-(-pPoint2.x/ddp+pPoint1.x/ddp)*WireOffset);
               }
               else
               {
               glVertex3f(pt3.x,pt3.y-sqrt(t)*fHeightDifference,pt3.z);
               glVertex3f(pt4.x+(pPoint2.z/ddp-pPoint1.z/ddp)*WireOffset,pt4.y,pt4.z+(-pPoint2.x/ddp+pPoint1.x/ddp)*WireOffset);
               }
               //endif;
               f+= step;

           }
       glEnd();
      }
      glEndList();
  }
}
/*
void __fastcall TTraction::InitCenter(vector3 Angles, vector3 pOrigin)
{
    pPosition= (pPoint2+pPoint1)*0.5f;
    fSquaredRadius= SquareMagnitude((pPoint2-pPoint1)*0.5f);
} */

void __fastcall TTraction::Render(float mgn)   //McZapkie: mgn to odleglosc od obserwatora
{
  //McZapkie: ustalanie przezroczystosci i koloru linii:
    if (Wires!=0 && !TestFlag(DamageFlag,128))  //rysuj jesli sa druty i nie zerwana
    {
      glColor4f(0,0,0,1);  //jak nieznany kolor to czarne nieprzezroczyste
      float linealpha=1000*WireThickness*WireThickness/(mgn+1.0);

      if(linealpha > 1.5)
          linealpha = 1.5;

      glEnable(GL_LINE_SMOOTH);
      glLineWidth(linealpha);

      if(linealpha > 1.0)
          linealpha = 1.0;

      //McZapkie-261102: kolor zalezy od materialu i zasniedzenia
      float r,g,b;
      switch (Material)
      {
          case 1:
              if (TestFlag(DamageFlag,1))
               {
                 r=0.2;
                 g=0.6;
                 b=0.3;  //zielona miedz
               }
              else
               {
                 r=0.6;
                 g=0.2;
                 b=0.1;  //czerwona miedz
               }
          break;
          case 2:
              if (TestFlag(DamageFlag,1))
               {
                 r=0.2;
                 g=0.2;
                 b=0.2;  //czarne Al
               }
              else
               {
                 r=0.5;
                 g=0.5;
                 b=0.5;  //srebrne Al
               }
          break;
      }
      r=r*Global::ambientDayLight[0];  //w zaleznosci od koloru swiatla
      g=g*Global::ambientDayLight[1];
      b=b*Global::ambientDayLight[2];
      glColor3f(r,g,b); // linealpha);
      glCallList(uiDisplayList);
      glLineWidth(1.0);
    }
}

int __fastcall TTraction::RaArrayPrepare()
{//przygotowanie tablic do skopiowania do VBO (zliczanie wierzcho³ków)
 //if (bVisible) //o ile w ogóle widaæ
  switch (Wires)
  {
   case 1: iLines=2; break;
   case 2: iLines=iNumSections?4*(iNumSections)-2+2:4; break;
   case 3: iLines=iNumSections?4*(iNumSections)-2+4:6; break;
   case 4: iLines=iNumSections?4*(iNumSections)-2+6:8; break;
   default: iLines=0;
  }
 //else iLines=0;
 return iLines;
};

void  __fastcall TTraction::RaArrayFill(CVertNormTex *Vert)
{//wype³nianie tablic VBO
 CVertNormTex *old=Vert;
 double ddp=hypot(pPoint2.x-pPoint1.x,pPoint2.z-pPoint1.z);
 if (Wires==2) WireOffset=0;
 //jezdny
 Vert->x=pPoint1.x-(pPoint2.z/ddp-pPoint1.z/ddp)*WireOffset;
 Vert->y=pPoint1.y;
 Vert->z=pPoint1.z-(-pPoint2.x/ddp+pPoint1.x/ddp)*WireOffset;
 ++Vert;
 Vert->x=pPoint2.x-(pPoint2.z/ddp-pPoint1.z/ddp)*WireOffset;
 Vert->y=pPoint2.y;
 Vert->z=pPoint2.z-(-pPoint2.x/ddp+pPoint1.x/ddp)*WireOffset;
 ++Vert;
 //Nie wiem co 'Marcin
 vector3 pt1,pt2,pt3,pt4,v1,v2;
 v1=pPoint4-pPoint3;
 v2=pPoint2-pPoint1;
 float step=0;
 if (iNumSections>0)
  step=1.0f/(float)iNumSections;
 float f=step;
 float mid=0.5;
 float t;
 //Przewod nosny 'Marcin
 if (Wires>1)
 {//lina noœna w kawa³kach
  Vert->x=pPoint3.x;
  Vert->y=pPoint3.y;
  Vert->z=pPoint3.z;
  ++Vert;
  for (int i=0;i<iNumSections-1;i++)
  {
   pt3=pPoint3+v1*f;
   t=(1-fabs(f-mid)*2);
   Vert->x=pt3.x;
   Vert->y=pt3.y-sqrt(t)*fHeightDifference;
   Vert->z=pt3.z;
   ++Vert;
   Vert->x=pt3.x; //drugi raz, bo nie jest line_strip
   Vert->y=pt3.y-sqrt(t)*fHeightDifference;
   Vert->z=pt3.z;
   ++Vert;
   f+=step;
  }
  Vert->x=pPoint4.x;
  Vert->y=pPoint4.y;
  Vert->z=pPoint4.z;
  ++Vert;
 }
 //Drugi przewod jezdny 'Winger
 if (Wires==3)
 {
  Vert->x=pPoint1.x+(pPoint2.z/ddp-pPoint1.z/ddp)*WireOffset;
  Vert->y=pPoint1.y;
  Vert->z=pPoint1.z+(-pPoint2.x/ddp+pPoint1.x/ddp)*WireOffset;
  ++Vert;
  Vert->x=pPoint2.x+(pPoint2.z/ddp-pPoint1.z/ddp)*WireOffset;
  Vert->y=pPoint2.y;
  Vert->z=pPoint2.z+(-pPoint2.x/ddp+pPoint1.x/ddp)*WireOffset;
  ++Vert;
 }
 f=step;
 //Przewody pionowe (wieszaki) 'Marcin, poprawki na 2 przewody jezdne 'Winger
 if (Wires>1)
 {
  for (int i=0;i<iNumSections-1;i++)
  {
   pt3=pPoint3+v1*f;
   pt4=pPoint1+v2*f;
   t=(1-fabs(f-mid)*2);
   Vert->x=pt3.x;
   Vert->y=pt3.y-sqrt(t)*fHeightDifference;
   Vert->z=pt3.z;
   ++Vert;
   if ((i%2)==0)
   {
    Vert->x=pt4.x-(pPoint2.z/ddp-pPoint1.z/ddp)*WireOffset;
    Vert->y=pt4.y;
    Vert->z=pt4.z-(-pPoint2.x/ddp+pPoint1.x/ddp)*WireOffset;
   }
   else
   {
    Vert->x=pt4.x+(pPoint2.z/ddp-pPoint1.z/ddp)*WireOffset;
    Vert->y=pt4.y;
    Vert->z=pt4.z+(-pPoint2.x/ddp+pPoint1.x/ddp)*WireOffset;
   }
   ++Vert;
   f+=step;
  }
 }
 if ((Vert-old)!=iLines)
  WriteLog("!!! Wygenerowano punktów "+AnsiString(Vert-old)+", powinno byæ "+AnsiString(iLines));
};

void  __fastcall TTraction::RaRenderVBO(float mgn,int iPtr)
{//renderowanie z u¿yciem VBO
 if (Wires!=0 && !TestFlag(DamageFlag,128))  //rysuj jesli sa druty i nie zerwana
 {
  glBindTexture(GL_TEXTURE_2D,0);
  glDisable(GL_LIGHTING); //aby nie u¿ywa³o wektorów normalnych do kolorowania
  glColor4f(0,0,0,1);  //jak nieznany kolor to czarne nieprzezroczyste
  //Ra: glEnable(GL_LINE_SMOOTH) kiepsko wygl¹da - robi gradient
  float linealpha=5000*WireThickness/(mgn+1.0); //*WireThickness
  if (linealpha>1.2) linealpha=1.2; //zbyt grube nie s¹ dobre
  glLineWidth(linealpha);
  //McZapkie-261102: kolor zalezy od materialu i zasniedzenia
  float r,g,b;
  switch (Material)
  {
   case 1:
    if (TestFlag(DamageFlag,1))
    {
     r=0.2; g=0.6; b=0.3;  //zielona miedz
    }
    else
    {
     r=0.6; g=0.2; b=0.1;  //czerwona miedz
    }
   break;
   case 2:
    if (TestFlag(DamageFlag,1))
    {
     r=0.2; g=0.2; b=0.2;  //czarne Al
    }
    else
    {
     r=0.5; g=0.5; b=0.5;  //srebrne Al
    }
   break;
  }
  r=r*Global::ambientDayLight[0];  //w zaleznosci od koloru swiatla
  g=g*Global::ambientDayLight[1];
  b=b*Global::ambientDayLight[2];
  if (linealpha>1.0) linealpha=1.0; //trzeba ograniczyæ do <=1
  glColor4f(r,g,b,linealpha);
  glDrawArrays(GL_LINES,iPtr,iLines);
  glLineWidth(1.0);
  glEnable(GL_LIGHTING);
 }
};

