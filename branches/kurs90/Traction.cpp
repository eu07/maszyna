//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

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

TTraction::TTraction()
{
    pPoint1=pPoint2=pPoint3=pPoint4=vector3(0,0,0);
    vFront=vector3(0,0,1);
    vUp=vector3(0,1,0);
    vLeft=vector3(1,0,0);
    fHeightDifference=0;
    iNumSections=0;
    iLines=0;
//    dwFlags= 0;
    Wires=2;
//    fU=fR= 0;
    uiDisplayList=0;
    asPowerSupplyName="";
//    mdPole= NULL;
//    ReplacableSkinID= 0;
 pPrev=pNext=NULL;
 iLast=1; //�e niby ostatni drut
}

TTraction::~TTraction()
{
 if (!Global::bUseVBO)
  glDeleteLists(uiDisplayList,1);
}

void __fastcall TTraction::Optimize()
{
 if (Global::bUseVBO) return;
 uiDisplayList=glGenLists(1);
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
              if ((Wires<4)||((i!=0)&&(i!=iNumSections-2))) 
               glVertex3f(pt3.x,pt3.y-sqrt(t)*fHeightDifference,pt3.z);
               f+= step;
           }
           glVertex3f(pPoint4.x,pPoint4.y,pPoint4.z);
       glEnd();
       }

      //Drugi przewod jezdny 'Winger
      if (Wires > 2)
      {
      glBegin(GL_LINE_STRIP);
          glVertex3f(pPoint1.x+(pPoint2.z/ddp-pPoint1.z/ddp)*WireOffset,pPoint1.y,pPoint1.z+(-pPoint2.x/ddp+pPoint1.x/ddp)*WireOffset);
          glVertex3f(pPoint2.x+(pPoint2.z/ddp-pPoint1.z/ddp)*WireOffset,pPoint2.y,pPoint2.z+(-pPoint2.x/ddp+pPoint1.x/ddp)*WireOffset);
      glEnd();
      }

      f= step; 
  
      if (Wires == 4) 
      { 
      glBegin(GL_LINE_STRIP); 
          glVertex3f(pPoint3.x,pPoint3.y-0.65f*fHeightDifference,pPoint3.z); 
          for (int i=0; i<iNumSections-1; i++) 
          { 
              pt3= pPoint3+v1*f; 
              t= (1-fabs(f-mid)*2); 
              glVertex3f(pt3.x,pt3.y-sqrt(t)*fHeightDifference-((i==0)||(i==iNumSections-2)?0.25f*fHeightDifference:+0.05),pt3.z); 
              f+= step; 
          } 
          glVertex3f(pPoint4.x,pPoint4.y-0.65f*fHeightDifference,pPoint4.z); 
      glEnd(); 
      } 
  

      f= step;

      //Przewody pionowe (wieszaki) 'Marcin, poprawki na 2 przewody jezdne 'Winger
      if (Wires != 1)
      {
       glBegin(GL_LINES);
           for (int i=0; i<iNumSections-1; i++)
           {
              float flo,flo1; 
              flo=(Wires==4?0.25f*fHeightDifference:0); 
              flo1=(Wires==4?+0.05:0); 
               pt3= pPoint3+v1*f;
               pt4= pPoint1+v2*f;
               t= (1-fabs(f-mid)*2);
               if ((i%2) == 0)
               {
               glVertex3f(pt3.x,pt3.y-sqrt(t)*fHeightDifference-((i==0)||(i==iNumSections-2)?flo:flo1),pt3.z);
               glVertex3f(pt4.x-(pPoint2.z/ddp-pPoint1.z/ddp)*WireOffset,pt4.y,pt4.z-(-pPoint2.x/ddp+pPoint1.x/ddp)*WireOffset);
               }
               else
               {
               glVertex3f(pt3.x,pt3.y-sqrt(t)*fHeightDifference-((i==0)||(i==iNumSections-2)?flo:flo1),pt3.z);
               glVertex3f(pt4.x+(pPoint2.z/ddp-pPoint1.z/ddp)*WireOffset,pt4.y,pt4.z+(-pPoint2.x/ddp+pPoint1.x/ddp)*WireOffset);
               } 
               if((Wires==4)&&((i==1)||(i==iNumSections-3))) 
               { 
               glVertex3f(pt3.x,pt3.y-sqrt(t)*fHeightDifference-0.05,pt3.z); 
               glVertex3f(pt3.x,pt3.y-sqrt(t)*fHeightDifference,pt3.z); 
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

void __fastcall TTraction::RenderDL(float mgn)   //McZapkie: mgn to odleglosc od obserwatora
{
  //McZapkie: ustalanie przezroczystosci i koloru linii:
 if (Wires!=0 && !TestFlag(DamageFlag,128))  //rysuj jesli sa druty i nie zerwana
 {
  //glDisable(GL_LIGHTING); //aby nie u�ywa�o wektor�w normalnych do kolorowania
  glColor4f(0,0,0,1);  //jak nieznany kolor to czarne nieprzezroczyste
  if (!Global::bSmoothTraction)
   glDisable(GL_LINE_SMOOTH); //na liniach kiepsko wygl�da - robi gradient
  float linealpha=5000*WireThickness/(mgn+1.0); //*WireThickness
  if (linealpha>1.2) linealpha=1.2; //zbyt grube nie s� dobre
  glLineWidth(linealpha);
  if (linealpha>1.0) linealpha = 1.0;
  //McZapkie-261102: kolor zalezy od materialu i zasniedzenia
  float r,g,b;
  switch (Material)
  {//Ra: kolory podzieli�em przez 2, bo po zmianie ambient za jasne by�y
   //trzeba uwzgl�dni� kierunek �wiecenia S�o�ca - tylko ze S�o�cem wida� kolor
   case 1:
    if (TestFlag(DamageFlag,1))
    {
     r=0.00000; g=0.32549; b=0.2882353;  //zielona miedz
    }
    else
    {
     r=0.35098; g=0.22549; b=0.1;  //czerwona miedz
    }
   break;
   case 2:
    if (TestFlag(DamageFlag,1))
    {
     r=0.10; g=0.10; b=0.10;  //czarne Al
    }
    else
    {
     r=0.25; g=0.25; b=0.25;  //srebrne Al
    }
   break;
  }
  r=r*Global::ambientDayLight[0];  //w zaleznosci od koloru swiatla
  g=g*Global::ambientDayLight[1];
  b=b*Global::ambientDayLight[2];
  if (linealpha>1.0) linealpha=1.0; //trzeba ograniczy� do <=1
  glColor4f(r,g,b,linealpha);
  glCallList(uiDisplayList);
  glLineWidth(1.0);
  glEnable(GL_LINE_SMOOTH);
  //glEnable(GL_LIGHTING); //bez tego si� modele nie o�wietlaj�
 }
}

int __fastcall TTraction::RaArrayPrepare()
{//przygotowanie tablic do skopiowania do VBO (zliczanie wierzcho�k�w)
 //if (bVisible) //o ile w og�le wida�
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
{//wype�nianie tablic VBO
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
 {//lina no�na w kawa�kach
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
  WriteLog("!!! Wygenerowano punkt�w "+AnsiString(Vert-old)+", powinno by� "+AnsiString(iLines));
};

void  __fastcall TTraction::RenderVBO(float mgn,int iPtr)
{//renderowanie z u�yciem VBO
 if (Wires!=0 && !TestFlag(DamageFlag,128))  //rysuj jesli sa druty i nie zerwana
 {
  glBindTexture(GL_TEXTURE_2D,0);
  glDisable(GL_LIGHTING); //aby nie u�ywa�o wektor�w normalnych do kolorowania
  glColor4f(0,0,0,1);  //jak nieznany kolor to czarne nieprzezroczyste
  if (!Global::bSmoothTraction)
   glDisable(GL_LINE_SMOOTH); //na liniach kiepsko wygl�da - robi gradient
  float linealpha=5000*WireThickness/(mgn+1.0); //*WireThickness
  if (linealpha>1.2) linealpha=1.2; //zbyt grube nie s� dobre
  glLineWidth(linealpha);
  //McZapkie-261102: kolor zalezy od materialu i zasniedzenia
  float r,g,b;
  switch (Material)
  {//Ra: kolory podzieli�em przez 2, bo po zmianie ambient za jasne by�y
   //trzeba uwzgl�dni� kierunek �wiecenia S�o�ca - tylko ze S�o�cem wida� kolor
   case 1:
    if (TestFlag(DamageFlag,1))
    {
     r=0.00000; g=0.32549; b=0.2882353;  //zielona miedz
    }
    else
    {
     r=0.35098; g=0.22549; b=0.1;  //czerwona miedz
    }
   break;
   case 2:
    if (TestFlag(DamageFlag,1))
    {
     r=0.10; g=0.10; b=0.10;  //czarne Al
    }
    else
    {
     r=0.25; g=0.25; b=0.25;  //srebrne Al
    }
   break;
  }
  r=r*Global::ambientDayLight[0];  //w zaleznosci od koloru swiatla
  g=g*Global::ambientDayLight[1];
  b=b*Global::ambientDayLight[2];
  if (linealpha>1.0) linealpha=1.0; //trzeba ograniczy� do <=1
  glColor4f(r,g,b,linealpha);
  glDrawArrays(GL_LINES,iPtr,iLines);
  glLineWidth(1.0);
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_LIGHTING); //bez tego si� modele nie o�wietlaj�
 }
};

int __fastcall TTraction::TestPoint(vector3 *Point)
{//sprawdzanie, czy prz�s�a mo�na po��czy�
 if (pPrev==NULL)
  if (pPoint1.Equal(Point))
   return 0;
 if (pNext==NULL)
  if (pPoint2.Equal(Point))
   return 1;
 return -1;
};

void __fastcall TTraction::Connect(int my,TTraction *with,int to)
{//��czenie segmentu (with) od strony (my) do jego (to)
 if (my)
 {//do mojego Point2
  pNext=with;
  iNext=to;
 }
 else
 {//do mojego Point1
  pPrev=with;
  iPrev=to;
 }
 if (to)
 {//do jego Point2
  with->pNext=this;
  with->iNext=my;
 }
 else
 {//do jego Point1
  with->pPrev=this;
  with->iPrev=my;
 }
 if (pPrev) //je�li z obu stron pod��czony
  if (pNext)
   iLast=0; //to nie jest ostatnim
 if (with->pPrev) //temu te�, bo drugi raz ��czenie si� nie nie wykona
  if (with->pNext)
   with->iLast=0; //to nie jest ostatnim
};

void __fastcall TTraction::WhereIs()
{//ustalenie przedostatnich prz�se�
 if (iLast) return; //ma ju� ustalon� informacj� o po�o�eniu
 if (pPrev?pPrev->iLast==1:false) //je�li poprzedni jest ostatnim
  iLast=2; //jest przedostatnim
 else
  if (pNext?pNext->iLast==1:false) //je�li nast�pny jest ostatnim
   iLast=2; //jest przedostatnim
};

void __fastcall TTraction::Init()
{//przeliczenie parametr�w
 vParametric=pPoint2-pPoint1; //wektor mno�nik�w parametru dla r�wnania parametrycznego
};
