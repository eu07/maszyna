//---------------------------------------------------------------------------

/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak and others

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

#include "Timer.h"
#include "Texture.h"
#include "Ground.h"
#include "Globals.h"
#include "Event.h"
#include "EvLaunch.h"
#include "TractionPower.h"
#include "Traction.h"
#include "Track.h"
#include "RealSound.h"
#include "AnimModel.h"
#include "MemCell.h"
#include "mtable.hpp"
#include "DynObj.h"
#include "Data.h"
#include "parser.h" //Tolaris-010603
#include "Driver.h"
#include "Console.h"
#include "Names.h"


#define _PROBLEND 1
//---------------------------------------------------------------------------
#pragma package(smart_init)


bool bCondition; //McZapkie: do testowania warunku na event multiple
AnsiString LogComment;

// tablica pozycji sektorów do renderowania progresywnego (zale¿nie od FPS)
//const int AreaFast[]={10,10,10,10,10,10,9,8,7,6,5}; //pe³ny zakres widocznoœci 2km
//const int AreaSlow[]={ 7, 7, 7, 7, 7, 6,5,4,0,0,0}; //ograniczenie widocznoœci 1.5km
//const int AreaMini[]={ 5, 5, 5, 5, 5, 4,0,0,0,0,0}; //ograniczenie widocznoœci 1km
//---------------------------------------------------------------------------
// Obiekt renderuj¹cy siatkê jest sztucznie tworzonym obiektem pomocniczym,
// grupuj¹cym siatki obiektów dla danej tekstury. Obiektami sk³adowymi mog¹
// byc trójk¹ty terenu, szyny, podsypki, a tak¿e proste modele np. s³upy.
// Obiekty sk³adowe dodane s¹ do listy TSubRect::nMeshed z list¹ zrobion¹ na
// TGroundNode::nNext3, gdzie s¹ posortowane wg tekstury. Obiekty renderuj¹ce
// s¹ wpisane na listê TSubRect::nRootMesh (TGroundNode::nNext2) oraz na
// odpowiednie listy renderowania, gdzie zastêpuj¹ obiekty sk³adowe (nNext3).
// Problematyczne s¹ tory/drogi/rzeki, gdzie u¿ywane sa 2 tekstury. Dlatego
// tory s¹ zdublowane jako TP_TRACK oraz TP_DUMMYTRACK. Jeœli tekstura jest
// tylko jedna (np. zwrotnice), nie jest u¿ywany TP_DUMMYTRACK.
//---------------------------------------------------------------------------
__fastcall TGroundNode::TGroundNode()
{//nowy obiekt terenu - pusty
 iType=GL_POINTS;
 Vertices=NULL;
 Next=nNext2=NULL;
 pCenter=vector3(0,0,0);
 iCount=0; //wierzcho³ków w trójk¹cie
 //iNumPts=0; //punktów w linii
 TextureID=0;
 iFlags=0; //tryb przezroczystoœci nie zbadany
 DisplayListID=0;
 Pointer=NULL; //zerowanie wskaŸnika kontekstowego
 bVisible=false; //czy widoczny
 fSquareRadius=10000*10000;
 fSquareMinRadius=0;
 asName="";
 //Color= TMaterialColor(1);
 //fAngle=0; //obrót dla modelu
 //fLineThickness=1.0; //mm dla linii
 for (int i=0;i<3;i++)
 {
  Ambient[i]=Global::whiteLight[i]*255;
  Diffuse[i]=Global::whiteLight[i]*255;
  Specular[i]=Global::noLight[i]*255;
 }
 nNext3=NULL; //nie wyœwietla innych
 iVboPtr=-1; //indeks w VBO sektora (-1: nie u¿ywa VBO)
 iVersion=0; //wersja siatki
}

__fastcall TGroundNode::~TGroundNode()
{
 //if (iFlags&0x200) //czy obiekt zosta³ utworzony?
  switch (iType)
  {case TP_MEMCELL:    SafeDelete(MemCell); break;
   case TP_EVLAUNCH:   SafeDelete(EvLaunch); break;
   case TP_TRACTION:   SafeDelete(Traction); break;
   case TP_TRACTIONPOWERSOURCE:
                       SafeDelete(TractionPowerSource); break;
   case TP_TRACK:      SafeDelete(pTrack); break;
   case TP_DYNAMIC:    SafeDelete(DynamicObject); break;
   case TP_MODEL:
    if (iFlags&0x200) //czy model zosta³ utworzony?
     delete Model;
    Model=NULL;
    break;
   case TP_TERRAIN:
   {//pierwsze nNode zawiera model E3D, reszta to trójk¹ty
    for (int i=1;i<iCount;++i)
     nNode->Vertices=NULL; //zerowanie wskaŸników w kolejnych elementach, bo nie s¹ do usuwania
    delete[] nNode; //usuniêcie tablicy i pierwszego elementu
   }
   case TP_SUBMODEL: //dla formalnoœci, nie wymaga usuwania
    break;
   case GL_LINES:
   case GL_LINE_STRIP:
   case GL_LINE_LOOP:
    SafeDeleteArray(Points);
    break;
   case GL_TRIANGLE_STRIP:
   case GL_TRIANGLE_FAN:
   case GL_TRIANGLES:
    SafeDeleteArray(Vertices);
    break;
  }
}

void __fastcall TGroundNode::Init(int n)
{//utworzenie tablicy wierzcho³ków
 bVisible=false;
 iNumVerts=n;
 Vertices=new TGroundVertex[iNumVerts];
}

__fastcall TGroundNode::TGroundNode(TGroundNodeType t,int n)
{//utworzenie obiektu
 TGroundNode(); //domyœlne ustawienia
 iNumVerts=n;
 if (iNumVerts) Vertices=new TGroundVertex[iNumVerts];
 iType=t;
 switch (iType)
 {//zale¿nie od typu
  case TP_TRACK:
   pTrack=new TTrack(this);
  break;
 }
}

void __fastcall TGroundNode::InitCenter()
{//obliczenie œrodka ciê¿koœci obiektu
 for (int i=0;i<iNumVerts;i++)
  pCenter+=Vertices[i].Point;
 pCenter/=iNumVerts;
}

void __fastcall TGroundNode::InitNormals()
{//obliczenie wektorów normalnych
    vector3 v1,v2,v3,v4,v5,n1,n2,n3,n4;
    int i;
    float tu,tv;
    switch (iType)
    {
        case GL_TRIANGLE_STRIP :
            v1= Vertices[0].Point-Vertices[1].Point;
            v2= Vertices[1].Point-Vertices[2].Point;
            n1= SafeNormalize(CrossProduct(v1,v2));
            if (Vertices[0].Normal==vector3(0,0,0))
                Vertices[0].Normal= n1;
            v3= Vertices[2].Point-Vertices[3].Point;
            n2= SafeNormalize(CrossProduct(v3,v2));
            if (Vertices[1].Normal==vector3(0,0,0))
                Vertices[1].Normal= (n1+n2)*0.5;

            for (i=2; i<iNumVerts-2; i+=2)
            {
                v4= Vertices[i-1].Point-Vertices[i].Point;
                v5= Vertices[i].Point-Vertices[i+1].Point;
                n3= SafeNormalize(CrossProduct(v3,v4));
                n4= SafeNormalize(CrossProduct(v5,v4));
                if (Vertices[i].Normal==vector3(0,0,0))
                    Vertices[i].Normal= (n1+n2+n3)/3;
                if (Vertices[i+1].Normal==vector3(0,0,0))
                    Vertices[i+1].Normal= (n2+n3+n4)/3;
                n1= n3;
                n2= n4;
                v3= v5;
            }
            if (Vertices[i].Normal==vector3(0,0,0))
                Vertices[i].Normal= (n1+n2)/2;
            if (Vertices[i+1].Normal==vector3(0,0,0))
                Vertices[i+1].Normal= n2;
        break;
        case GL_TRIANGLE_FAN :

        break;
        case GL_TRIANGLES :
            for (i=0; i<iNumVerts; i+=3)
            {
                v1= Vertices[i+0].Point-Vertices[i+1].Point;
                v2= Vertices[i+1].Point-Vertices[i+2].Point;
                n1= SafeNormalize(CrossProduct(v1,v2));
                if (Vertices[i+0].Normal==vector3(0,0,0))
                    Vertices[i+0].Normal= (n1);
                if (Vertices[i+1].Normal==vector3(0,0,0))
                    Vertices[i+1].Normal= (n1);
                if (Vertices[i+2].Normal==vector3(0,0,0))
                    Vertices[i+2].Normal= (n1);
                tu= floor(Vertices[i+0].tu);
                tv= floor(Vertices[i+0].tv);
                Vertices[i+1].tv-= tv;
                Vertices[i+2].tv-= tv;
                Vertices[i+0].tv-= tv;
                Vertices[i+1].tu-= tu;
                Vertices[i+2].tu-= tu;
                Vertices[i+0].tu-= tu;
            }
        break;
    }
}

void __fastcall TGroundNode::MoveMe(vector3 pPosition)
{//przesuwanie obiektów scenerii o wektor w celu redukcji trzêsienia
 pCenter+=pPosition;
 switch (iType)
 {
  case TP_TRACTION:
   Traction->pPoint1+=pPosition;
   Traction->pPoint2+=pPosition;
   Traction->pPoint3+=pPosition;
   Traction->pPoint4+=pPosition;
   Traction->Optimize();
   break;
  case TP_MODEL:
  case TP_DYNAMIC:
  case TP_MEMCELL:
  case TP_EVLAUNCH:
   break;
  case TP_TRACK:
   pTrack->MoveMe(pPosition);
   break;
  case TP_SOUND: //McZapkie - dzwiek zapetlony w zaleznosci od odleglosci
   pStaticSound->vSoundPosition+=pPosition;
   break;
  case GL_LINES:
  case GL_LINE_STRIP:
  case GL_LINE_LOOP:
   for (int i=0; i<iNumPts;i++)
    Points[i]+=pPosition;
   ResourceManager::Unregister(this);
   break;
  default:
   for (int i=0; i<iNumVerts; i++)
    Vertices[i].Point+=pPosition;
   ResourceManager::Unregister(this);
 }
}

void __fastcall TGroundNode::RaRenderVBO()
{//renderowanie z domyslnego bufora VBO
 glColor3ub(Diffuse[0],Diffuse[1],Diffuse[2]);
 if (TextureID)
  glBindTexture(GL_TEXTURE_2D,TextureID); // Ustaw aktywn¹ teksturê
 glDrawArrays(iType,iVboPtr,iNumVerts);   // Narysuj naraz wszystkie trójk¹ty
}

void __fastcall TGroundNode::RenderVBO()
{//renderowanie obiektu z VBO - faza nieprzezroczystych
 double mgn=SquareMagnitude(pCenter-Global::pCameraPosition);
 if ((mgn>fSquareRadius || (mgn<fSquareMinRadius)) && (iType!=TP_EVLAUNCH)) //McZapkie-070602: nie rysuj odleglych obiektow ale sprawdzaj wyzwalacz zdarzen
     return;
 int i,a;
 switch (iType)
 {
  case TP_TRACTION: return;
  case TP_TRACK: if (iNumVerts) pTrack->RaRenderVBO(iVboPtr); return;
  case TP_MODEL: Model->RenderVBO(&pCenter); return;
  case TP_SOUND: //McZapkie - dzwiek zapetlony w zaleznosci od odleglosci
   if ((pStaticSound->GetStatus()&DSBSTATUS_PLAYING)==DSBPLAY_LOOPING)
   {
    pStaticSound->Play(1,DSBPLAY_LOOPING,true,pStaticSound->vSoundPosition);
    pStaticSound->AdjFreq(1.0,Timer::GetDeltaTime());
   }
   return;
  case TP_MEMCELL: return;
  case TP_EVLAUNCH:
   if (EvLaunch->Render())
    if ((EvLaunch->dRadius<0)||(mgn<EvLaunch->dRadius))
    {
     if (Console::Pressed(VK_SHIFT) && EvLaunch->Event2!=NULL)
      Global::pGround->AddToQuery(EvLaunch->Event2,NULL);
     else
      if (EvLaunch->Event1!=NULL)
       Global::pGround->AddToQuery(EvLaunch->Event1,NULL);
    }
   return;
  case GL_LINES:
  case GL_LINE_STRIP:
  case GL_LINE_LOOP:
   if (iNumPts)
   {float linealpha=255000*fLineThickness/(mgn+1.0);
    if (linealpha>255) linealpha=255;
    float r,g,b;
    r=floor(Diffuse[0]*Global::ambientDayLight[0]);  //w zaleznosci od koloru swiatla
    g=floor(Diffuse[1]*Global::ambientDayLight[1]);
    b=floor(Diffuse[2]*Global::ambientDayLight[2]);
    glColor4ub(r,g,b,linealpha); //przezroczystosc dalekiej linii
    //glDisable(GL_LIGHTING); //nie powinny œwieciæ
    glDrawArrays(iType,iVboPtr,iNumPts); //rysowanie linii
    //glEnable(GL_LIGHTING);
   }
   return;
  default:
   if (iVboPtr>=0)
    RaRenderVBO();
 };
 return;
};

void __fastcall TGroundNode::RenderAlphaVBO()
{//renderowanie obiektu z VBO - faza przezroczystych
 double mgn=SquareMagnitude(pCenter-Global::pCameraPosition);
 float r,g,b;
 if (mgn<fSquareMinRadius) return;
 if (mgn>fSquareRadius) return;
 int i,a;
#ifdef _PROBLEND
 if ((PROBLEND)) // sprawdza, czy w nazwie nie ma @    //Q: 13122011 - Szociu: 27012012
 {
 glDisable(GL_BLEND);
 glAlphaFunc(GL_GREATER,0.45);     // im mniejsza wartoœæ, tym wiêksza ramka, domyœlnie 0.1f
 };
#endif
 switch (iType)
 {
  case TP_TRACTION:
   if (bVisible)
   {
#ifdef _PROBLEND
    glEnable(GL_BLEND);
    glAlphaFunc(GL_GREATER,0.04);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
    Traction->RaRenderVBO(mgn,iVboPtr);
   }
   return;
  case TP_MODEL:
#ifdef _PROBLEND
    glEnable(GL_BLEND);
    glAlphaFunc(GL_GREATER,0.04);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
   Model->RenderAlphaVBO(&pCenter);
   return;
  case GL_LINES:
  case GL_LINE_STRIP:
  case GL_LINE_LOOP:
   if (iNumPts)
   {float linealpha=255000*fLineThickness/(mgn+1.0);
    if (linealpha>255) linealpha=255;
    r=Diffuse[0]*Global::ambientDayLight[0];  //w zaleznosci od koloru swiatla
    g=Diffuse[1]*Global::ambientDayLight[1];
    b=Diffuse[2]*Global::ambientDayLight[2];
    glColor4ub(r,g,b,linealpha); //przezroczystosc dalekiej linii
    //glDisable(GL_LIGHTING); //nie powinny œwieciæ
    glDrawArrays(iType,iVboPtr,iNumPts); //rysowanie linii
    //glEnable(GL_LIGHTING);
#ifdef _PROBLEND
    glEnable(GL_BLEND);
    glAlphaFunc(GL_GREATER,0.04);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
   }
#ifdef _PROBLEND
    glEnable(GL_BLEND);
    glAlphaFunc(GL_GREATER,0.04);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
   return;
  default:
   if (iVboPtr>=0)
   {RaRenderVBO();
#ifdef _PROBLEND
    glEnable(GL_BLEND);
    glAlphaFunc(GL_GREATER,0.04);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
    return;
   }
 }
#ifdef _PROBLEND
    glEnable(GL_BLEND);
    glAlphaFunc(GL_GREATER,0.04);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
 return;
}

void __fastcall TGroundNode::Compile(bool many)
{//tworzenie skompilowanej listy w wyœwietlaniu DL
 if (!many)
 {//obs³uga pojedynczej listy
  if (DisplayListID) Release();
  if (Global::bManageNodes)
  {
   DisplayListID=glGenLists(1);
   glNewList(DisplayListID,GL_COMPILE);
   iVersion=Global::iReCompile; //aktualna wersja siatek (do WireFrame)
  }
 }
 if ((iType==GL_LINES)||(iType==GL_LINE_STRIP)||(iType==GL_LINE_LOOP))
 {
#ifdef USE_VERTEX_ARRAYS
  glVertexPointer(3,GL_DOUBLE,sizeof(vector3),&Points[0].x);
#endif
  glBindTexture(GL_TEXTURE_2D,0);
#ifdef USE_VERTEX_ARRAYS
  glDrawArrays(iType,0,iNumPts);
#else
  glBegin(iType);
  for (int i=0;i<iNumPts;i++)
   glVertex3dv(&Points[i].x);
  glEnd();
#endif
 }
 else if (iType==GL_TRIANGLE_STRIP || iType==GL_TRIANGLE_FAN || iType==GL_TRIANGLES )
 {//jak nie linie, to trójk¹ty
  TGroundNode *tri=this;
  do
  {//pêtla po obiektach w grupie w celu po³¹czenia siatek
#ifdef USE_VERTEX_ARRAYS
   glVertexPointer(3, GL_DOUBLE, sizeof(TGroundVertex), &tri->Vertices[0].Point.x);
   glNormalPointer(GL_DOUBLE, sizeof(TGroundVertex), &tri->Vertices[0].Normal.x);
   glTexCoordPointer(2, GL_FLOAT, sizeof(TGroundVertex), &tri->Vertices[0].tu);
#endif
   glColor3ub(tri->Diffuse[0],tri->Diffuse[1],tri->Diffuse[2]);
   glBindTexture(GL_TEXTURE_2D,Global::bWireFrame?0:tri->TextureID);
#ifdef USE_VERTEX_ARRAYS
   glDrawArrays(Global::bWireFrame?GL_LINE_LOOP:tri->iType,0,tri->iNumVerts);
#else
   glBegin(Global::bWireFrame?GL_LINE_LOOP:tri->iType);
   for (int i=0;i<tri->iNumVerts;i++)
   {
    glNormal3d(tri->Vertices[i].Normal.x,tri->Vertices[i].Normal.y,tri->Vertices[i].Normal.z);
    glTexCoord2f(tri->Vertices[i].tu,tri->Vertices[i].tv);
    glVertex3dv(&tri->Vertices[i].Point.x);
   };
   glEnd();
#endif
/*
   if (tri->pTriGroup) //jeœli z grupy
   {tri=tri->pNext2; //nastêpny w sektorze
    while (tri?!tri->pTriGroup:false) tri=tri->pNext2; //szukamy kolejnego nale¿¹cego do grupy
   }
   else
*/
    tri=NULL; //a jak nie, to koniec
  } while (tri);
 }
 else if (iType==TP_MESH)
 {//grupa ze wspóln¹ tekstur¹ - wrzucanie do wspólnego Display List
  if (TextureID)
   glBindTexture(GL_TEXTURE_2D,TextureID); // Ustaw aktywn¹ teksturê
  TGroundNode *n=nNode;
  while (n?n->TextureID==TextureID:false)
  {//wszystkie obiekty o tej samej testurze
   switch (n->iType)
   {//poszczególne typy ró¿nie siê tworzy
    case TP_TRACK:
    case TP_DUMMYTRACK:
     n->pTrack->Compile(TextureID); //dodanie trójk¹tów dla podanej tekstury
     break;
   }
   n=n->nNext3; //nastêpny z listy
  }
 }
 if (!many)
  if (Global::bManageNodes)
   glEndList();
};

void TGroundNode::Release()
{
 if (DisplayListID)
  glDeleteLists(DisplayListID,1);
 DisplayListID=0;
};

void __fastcall TGroundNode::RenderHidden()
{//renderowanie obiektów niewidocznych
 double mgn=SquareMagnitude(pCenter-Global::pCameraPosition);
 switch (iType)
 {
  case TP_SOUND: //McZapkie - dzwiek zapetlony w zaleznosci od odleglosci
   if ((pStaticSound->GetStatus()&DSBSTATUS_PLAYING)==DSBPLAY_LOOPING)
   {
    pStaticSound->Play(1,DSBPLAY_LOOPING,true,pStaticSound->vSoundPosition);
    pStaticSound->AdjFreq(1.0,Timer::GetDeltaTime());
   }
   return;
  case TP_EVLAUNCH:
   if (EvLaunch->Render())
    if ((EvLaunch->dRadius<0)||(mgn<EvLaunch->dRadius))
    {
     WriteLog("Eventlauncher "+asName);
     if (Console::Pressed(VK_SHIFT)&&(EvLaunch->Event2))
      Global::pGround->AddToQuery(EvLaunch->Event2,NULL);
     else
      if (EvLaunch->Event1)
       Global::pGround->AddToQuery(EvLaunch->Event1,NULL);
    }
   return;
 }
};

void __fastcall TGroundNode::RenderDL()
{//wyœwietlanie obiektu przez Display List
 switch (iType)
 {//obiekty renderowane niezale¿nie od odleg³oœci
  case TP_SUBMODEL:
   TSubModel::fSquareDist=0;
   return smTerrain->RenderDL();
 }
 //if (pTriGroup) if (pTriGroup!=this) return; //wyœwietla go inny obiekt
 double mgn=SquareMagnitude(pCenter-Global::pCameraPosition);
 if ((mgn>fSquareRadius)||(mgn<fSquareMinRadius)) //McZapkie-070602: nie rysuj odleglych obiektow ale sprawdzaj wyzwalacz zdarzen
  return;
 int i,a;
 switch (iType)
 {
  case TP_TRACK:
   return pTrack->Render();
  case TP_MODEL:
   return Model->RenderDL(&pCenter);
 }
  // TODO: sprawdzic czy jest potrzebny warunek fLineThickness < 0
  //if ((iNumVerts&&(iFlags&0x10))||(iNumPts&&(fLineThickness<0)))
  if ((iFlags&0x10)||(fLineThickness<0))
  {
   if (!DisplayListID||(iVersion!=Global::iReCompile)) //Ra: wymuszenie rekompilacji
   {
    Compile();
    if (Global::bManageNodes)
     ResourceManager::Register(this);
   };

   if ((iType==GL_LINES)||(iType==GL_LINE_STRIP)||(iType==GL_LINE_LOOP))
   //if (iNumPts)
   {//wszelkie linie s¹ rysowane na samym koñcu
    float r,g,b;
    r=Diffuse[0]*Global::ambientDayLight[0];  //w zaleznosci od koloru swiatla
    g=Diffuse[1]*Global::ambientDayLight[1];
    b=Diffuse[2]*Global::ambientDayLight[2];
    glColor4ub(r,g,b,1.0);
    glCallList(DisplayListID);
    //glColor4fv(Diffuse); //przywrócenie koloru
    //glColor3ub(Diffuse[0],Diffuse[1],Diffuse[2]);
   }
   // GL_TRIANGLE etc
   else
    glCallList(DisplayListID);
   SetLastUsage(Timer::GetSimulationTime());
  };
};

void __fastcall TGroundNode::RenderAlphaDL()
{
// SPOSOB NA POZBYCIE SIE RAMKI DOOKOLA TEXTURY ALPHA DLA OBIEKTOW ZAGNIEZDZONYCH W SCN JAKO NODE

//W GROUND.H dajemy do klasy TGroundNode zmienna bool PROBLEND to samo robimy w klasie TGround
//nastepnie podczas wczytywania textury dla TRIANGLES w TGround::AddGroundNode
//sprawdzamy czy w nazwie jest @ i wg tego
//ustawiamy PROBLEND na true dla wlasnie wczytywanego trojkata (kazdy trojkat jest osobnym nodem)
//nastepnie podczas renderowania w bool __fastcall TGroundNode::RenderAlpha()
//na poczatku ustawiamy standardowe GL_GREATER = 0.04
//pozniej sprawdzamy czy jest wlaczony PROBLEND dla aktualnie renderowanego noda TRIANGLE, wlasciwie dla kazdego node'a
//i jezeli tak to odpowiedni GL_GREATER w przeciwnym wypadku standardowy 0.04


 //if (pTriGroup) if (pTriGroup!=this) return; //wyœwietla go inny obiekt
 double mgn=SquareMagnitude(pCenter-Global::pCameraPosition);
 float r,g,b;
 if (mgn<fSquareMinRadius)
     return;
 if (mgn>fSquareRadius)
     return;
 int i,a;
 switch (iType)
 {
  case TP_TRACTION:
   if (bVisible)
    Traction->Render(mgn);
   return;
  case TP_MODEL:
   Model->RenderAlphaDL(&pCenter);
   return;
  case TP_TRACK:
   //pTrack->RenderAlpha();
   return;
 };

 // TODO: sprawdzic czy jest potrzebny warunek fLineThickness < 0
 if (
     (iNumVerts && (iFlags&0x20)) ||
     (iNumPts && (fLineThickness > 0)))
 {
#ifdef _PROBLEND
     if ((PROBLEND) ) // sprawdza, czy w nazwie nie ma @    //Q: 13122011 - Szociu: 27012012
          {
               glDisable(GL_BLEND);
               glAlphaFunc(GL_GREATER,0.45);     // im mniejsza wartoœæ, tym wiêksza ramka, domyœlnie 0.1f
          };
#endif
     if (!DisplayListID) //||Global::bReCompile) //Ra: wymuszenie rekompilacji
     {
         Compile();
         if (Global::bManageNodes)
             ResourceManager::Register(this);
     };

     // GL_LINE, GL_LINE_STRIP, GL_LINE_LOOP
     if (iNumPts)
     {
         float linealpha=255000*fLineThickness/(mgn+1.0);
         if (linealpha>255)
           linealpha= 255;
         r=Diffuse[0]*Global::ambientDayLight[0];  //w zaleznosci od koloru swiatla
         g=Diffuse[1]*Global::ambientDayLight[1];
         b=Diffuse[2]*Global::ambientDayLight[2];
         glColor4ub(r,g,b,linealpha); //przezroczystosc dalekiej linii
         glCallList(DisplayListID);
     }
     // GL_TRIANGLE etc
     else
      glCallList(DisplayListID);
     SetLastUsage(Timer::GetSimulationTime());
 };
#ifdef _PROBLEND
 if ((PROBLEND)) // sprawdza, czy w nazwie nie ma @    //Q: 13122011 - Szociu: 27012012
 {glEnable(GL_BLEND);
  glAlphaFunc(GL_GREATER,0.04);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
 };
#endif
}

//------------------------------------------------------------------------------
//------------------ Podstawowy pojemnik terenu - sektor -----------------------
//------------------------------------------------------------------------------
__fastcall TSubRect::TSubRect()
{
 nRootNode=NULL; //lista wszystkich obiektów jest pusta
 nRenderHidden=nRenderRect=nRenderRectAlpha=nRender=nRenderMixed=nRenderAlpha=nRenderWires=NULL;
 tTrackAnim=NULL; //nic nie animujemy
 tTracks=NULL; //nie ma jeszcze torów
 nRootMesh=nMeshed=NULL; //te listy te¿ s¹ puste
 iNodeCount=0; //licznik obiektów
 iTracks=0; //licznik torów
}
__fastcall TSubRect::~TSubRect()
{
 if (Global::bManageNodes) //Ra: tu siê coœ sypie
  ResourceManager::Unregister(this); //wyrejestrowanie ze sprz¹tacza
 //TODO: usun¹æ obiekty z listy (nRootMesh), bo s¹ one tworzone dla sektora
}

void __fastcall TSubRect::NodeAdd(TGroundNode *Node)
{//przyczepienie obiektu do sektora, wstêpna kwalifikacja na listy renderowania
 if (!this) return; //zabezpiecznie przed obiektami przekraczaj¹cymi obszar roboczy
 //Ra: sortowanie obiektów na listy renderowania:
 //nRenderHidden    - lista obiektów niewidocznych, "renderowanych" równie¿ z ty³u
 //nRenderRect      - lista grup renderowanych z sektora
 //nRenderRectAlpha - lista grup renderowanych z sektora z przezroczystoœci¹
 //nRender          - lista grup renderowanych z w³asnych VBO albo DL
 //nRenderAlpha     - lista grup renderowanych z w³asnych VBO albo DL z przezroczystoœci¹
 //nRenderWires     - lista grup renderowanych z w³asnych VBO albo DL - druty i linie
 //nMeshed          - obiekty do pogrupowania wg tekstur
 GLuint t; //pomocniczy kod tekstury
 switch (Node->iType)
 {case TP_SOUND: //te obiekty s¹ sprawdzanie niezale¿nie od kierunku patrzenia
  case TP_EVLAUNCH:
   Node->nNext3=nRenderHidden; nRenderHidden=Node; //do listy koniecznych
   break;
  case TP_TRACK: //TODO: tory z cieniem (tunel, canyon) te¿ daæ bez ³¹czenia?
   ++iTracks; //jeden tor wiêcej
   Node->pTrack->RaOwnerSet(this); //do którego sektora ma zg³aszaæ animacjê
   //if (Global::bUseVBO?false:!Node->pTrack->IsGroupable())
   if (Global::bUseVBO?true:!Node->pTrack->IsGroupable()) //TODO: tymczasowo dla VBO wy³¹czone
    RaNodeAdd(Node); //tory ruchome nie s¹ grupowane przy Display Lists (wymagaj¹ odœwie¿ania DL)
   else
   {//tory nieruchome mog¹ byæ pogrupowane wg tekstury, przy VBO wszystkie
    Node->TextureID=Node->pTrack->TextureGet(0); //pobranie tekstury do sortowania
    t=Node->pTrack->TextureGet(1);
    if (Node->TextureID) //je¿eli jest pierwsza
    {if (t&&(Node->TextureID!=t))
     {//jeœli s¹ dwie ró¿ne tekstury, dodajemy drugi obiekt dla danego toru
      TGroundNode *n=new TGroundNode();
      n->iType=TP_DUMMYTRACK; //obiekt renderuj¹cy siatki dla tekstury
      n->TextureID=t;
      n->pTrack=Node->pTrack; //wskazuje na ten sam tor
      n->pCenter=Node->pCenter;
      n->fSquareRadius=Node->fSquareRadius;
      n->fSquareMinRadius=Node->fSquareMinRadius;
      n->iFlags=Node->iFlags;
      n->nNext2=nRootMesh; nRootMesh=n; //podczepienie do listy, ¿eby usun¹æ na koñcu
      n->nNext3=nMeshed; nMeshed=n;
     }
    }
    else
     Node->TextureID=t; //jest tylko druga tekstura
    if (Node->TextureID)
    {Node->nNext3=nMeshed; nMeshed=Node;} //do podzielenia potem
   }
   break;
  case GL_TRIANGLE_STRIP:
  case GL_TRIANGLE_FAN:
  case GL_TRIANGLES:
   //Node->nNext3=nMeshed; nMeshed=Node; //do podzielenia potem
   if (Node->iFlags&0x20) //czy jest przezroczyste?
   {Node->nNext3=nRenderRectAlpha; nRenderRectAlpha=Node;} //DL: do przezroczystych z sektora
   else
    if (Global::bUseVBO)
    {Node->nNext3=nRenderRect; nRenderRect=Node;} //VBO: do nieprzezroczystych z sektora
    else
    {Node->nNext3=nRender; nRender=Node;} //DL: do nieprzezroczystych wszelakich
/*
   //Ra: na razie wy³¹czone do testów VBO
   //if ((Node->iType==GL_TRIANGLE_STRIP)||(Node->iType==GL_TRIANGLE_FAN)||(Node->iType==GL_TRIANGLES))
    if (Node->fSquareMinRadius==0.0) //znikaj¹ce z bliska nie mog¹ byæ optymalizowane
     if (Node->fSquareRadius>=160000.0) //tak od 400m to ju¿ normalne trójk¹ty musz¹ byæ
     //if (Node->iFlags&0x10) //i nieprzezroczysty
     {if (pTriGroup) //je¿eli by³ ju¿ jakiœ grupuj¹cy
      {if (pTriGroup->fSquareRadius>Node->fSquareRadius) //i mia³ wiêkszy zasiêg
        Node->fSquareRadius=pTriGroup->fSquareRadius; //zwiêkszenie zakresu widocznoœci grupuj¹cego
       pTriGroup->pTriGroup=Node; //poprzedniemu doczepiamy nowy
      }
      Node->pTriGroup=Node; //nowy lider ma siê sam wyœwietlaæ - wskaŸnik na siebie
      pTriGroup=Node; //zapamiêtanie lidera
     }
*/
   break;
  case TP_TRACTION:
  case GL_LINES:
  case GL_LINE_STRIP:
  case GL_LINE_LOOP: //te renderowane na koñcu, ¿eby nie ³apa³y koloru nieba
   Node->nNext3=nRenderWires; nRenderWires=Node; //lista drutów
   break;
  case TP_MODEL: //modele zawsze wyœwietlane z w³asnego VBO
   //jeœli model jest prosty, mo¿na próbowaæ zrobiæ wspóln¹ siatkê (s³upy)
   if ((Node->iFlags&0x20200020)==0) //czy brak przezroczystoœci?
   {Node->nNext3=nRender; nRender=Node;} //do nieprzezroczystych
   else if ((Node->iFlags&0x10100010)==0) //czy brak nieprzezroczystoœci?
   {Node->nNext3=nRenderAlpha; nRenderAlpha=Node;} //do przezroczystych
   else //jak i take i takie, to bêdzie dwa razy renderowane...
   {Node->nNext3=nRenderMixed; nRenderMixed=Node;} //do mieszanych
   //Node->nNext3=nMeshed; //dopisanie do listy sortowania
   //nMeshed=Node;
   break;
  case TP_MEMCELL:
  case TP_TRACTIONPOWERSOURCE: //a te w ogóle pomijamy
//  case TP_ISOLATED: //lista torów w obwodzie izolowanym - na razie ignorowana
   break;
  case TP_DYNAMIC:
   return; //tych nie dopisujemy wcale
 }
 Node->nNext2=nRootNode; //dopisanie do ogólnej listy
 nRootNode=Node;
 ++iNodeCount; //licznik obiektów
}

void __fastcall TSubRect::RaNodeAdd(TGroundNode *Node)
{//finalna kwalifikacja na listy renderowania, jeœli nie obs³ugiwane grupowo
 switch (Node->iType)
 {case TP_TRACK:
   if (Global::bUseVBO)
   {Node->nNext3=nRenderRect; nRenderRect=Node;} //VBO: do nieprzezroczystych z sektora
   else
   {Node->nNext3=nRender; nRender=Node;} //DL: do nieprzezroczystych
   break;
  case GL_TRIANGLE_STRIP:
  case GL_TRIANGLE_FAN:
  case GL_TRIANGLES:
   if (Node->iFlags&0x20) //czy jest przezroczyste?
   {Node->nNext3=nRenderRectAlpha; nRenderRectAlpha=Node;} //DL: do przezroczystych z sektora
   else
    if (Global::bUseVBO)
    {Node->nNext3=nRenderRect; nRenderRect=Node;} //VBO: do nieprzezroczystych z sektora
    else
    {Node->nNext3=nRender; nRender=Node;} //DL: do nieprzezroczystych wszelakich
   break;
  case TP_MODEL: //modele zawsze wyœwietlane z w³asnego VBO
   if ((Node->iFlags&0x20200020)==0) //czy brak przezroczystoœci?
   {Node->nNext3=nRender; nRender=Node;} //do nieprzezroczystych
   else if ((Node->iFlags&0x10100010)==0) //czy brak nieprzezroczystoœci?
   {Node->nNext3=nRenderAlpha; nRenderAlpha=Node;} //do przezroczystych
   else //jak i take i takie, to bêdzie dwa razy renderowane...
   {Node->nNext3=nRenderMixed; nRenderMixed=Node;} //do mieszanych
   break;
  case TP_MESH: //grupa ze wspóln¹ tekstur¹
   //{Node->nNext3=nRenderRect; nRenderRect=Node;} //do nieprzezroczystych z sektora
   {Node->nNext3=nRender; nRender=Node;} //do nieprzezroczystych
   break;
  case TP_SUBMODEL: //submodele terenu w kwadracie kilometrowym id¹ do nRootMesh
   //WriteLog("nRootMesh was "+AnsiString(nRootMesh?"not null ":"null ")+IntToHex(int(this),8));
   Node->nNext3=nRootMesh; //przy VBO musi byæ inaczej
   nRootMesh=Node;
   break;
 }
}

void __fastcall TSubRect::Sort()
{//przygotowanie sektora do renderowania
 TGroundNode **n0,*n1,*n2; //wskaŸniki robocze
 delete[] tTracks; //usuniêcie listy
 tTracks=iTracks?new TTrack*[iTracks]:NULL; //tworzenie tabeli torów do renderowania pojazdów
 if (tTracks)
 {//wype³nianie tabeli torów
  int i=0;
  for (n1=nRootNode;n1;n1=n1->nNext2) //kolejne obiekty z sektora
   if (n1->iType==TP_TRACK)
    tTracks[i++]=n1->pTrack;
 }
 //sortowanie obiektów w sektorze na listy renderowania
 if (!nMeshed) return; //nie ma nic do sortowania
 bool sorted=false;
 while (!sorted)
 {//sortowanie b¹belkowe obiektów wg tekstury
  sorted=true; //zak³adamy posortowanie
  n0=&nMeshed; //wskaŸnik niezbêdny do zamieniania obiektów
  n1=nMeshed; //lista obiektów przetwarzanych na statyczne siatki
  while (n1)
  {//sprawdzanie stanu posortowania obiektów i ewentualne zamiany
   n2=n1->nNext3; //kolejny z tej listy
   if (n2) //jeœli istnieje
    if (n1->TextureID>n2->TextureID)
    {//zamiana elementów miejscami
     *n0=n2; //drugi bêdzie na pocz¹tku
     n1->nNext3=n2->nNext3; //ten zza drugiego bêdzie za pierwszym
     n2->nNext3=n1; //a za drugim bêdzie pierwszy
     sorted=false; //potrzebny kolejny przebieg
    }
   n0=&(n1->nNext3);
   n1=n2;
  };
 }
 //wyrzucenie z listy obiektów pojedynczych (nie ma z czym ich grupowaæ)
 //nawet jak s¹ pojedyncze, to i tak lepiej, aby by³y w jednym Display List
/*
    else
    {//dodanie do zwyk³ej listy renderowania i usuniêcie z grupowego
     *n0=n2; //drugi bêdzie na pocz¹tku
     RaNodeAdd(n1); //nie ma go z czym zgrupowaæ; (n1->nNext3) zostanie nadpisane
     n1=n2; //potrzebne do ustawienia (n0)
    }
*/
 //...
 //przegl¹danie listy i tworzenie obiektów renderuj¹cych dla danej tekstury
 GLuint t=0; //pomocniczy kod tekstury
 n1=nMeshed; //lista obiektów przetwarzanych na statyczne siatki
 while (n1)
 {//dla ka¿dej tekstury powinny istnieæ co najmniej dwa obiekty, ale dla DL nie ma to znaczenia
  if (t<n1->TextureID) //jeœli (n1) ma inn¹ teksturê ni¿ poprzednie
  {//mo¿na zrobiæ obiekt renderuj¹cy
   t=n1->TextureID;
   n2=new TGroundNode();
   n2->nNext2=nRootMesh; nRootMesh=n2; //podczepienie na pocz¹tku listy
   nRootMesh->iType=TP_MESH; //obiekt renderuj¹cy siatki dla tekstury
   nRootMesh->TextureID=t;
   nRootMesh->nNode=n1; //pierwszy element z listy
   nRootMesh->pCenter=n1->pCenter;
   nRootMesh->fSquareRadius=1e8; //widaæ bez ograniczeñ
   nRootMesh->fSquareMinRadius=0.0;
   nRootMesh->iFlags=0x10;
   RaNodeAdd(nRootMesh); //dodanie do odpowiedniej listy renderowania
  }
  n1=n1->nNext3; //kolejny z tej listy
 };
}

TTrack* __fastcall TSubRect::FindTrack(vector3 *Point,int &iConnection,TTrack *Exclude)
{//szukanie toru, którego koniec jest najbli¿szy (*Point)
 TTrack *Track;
 for (int i=0;i<iTracks;++i)
  if (tTracks[i]!=Exclude) //mo¿na u¿yæ tabelê torów, bo jest mniejsza
  {
   iConnection=tTracks[i]->TestPoint(Point);
   if (iConnection>=0) return tTracks[i]; //szukanie TGroundNode nie jest potrzebne
  }
/*
 TGroundNode *Current;
 for (Current=nRootNode;Current;Current=Current->Next)
  if ((Current->iType==TP_TRACK)&&(Current->pTrack!=Exclude)) //mo¿na u¿yæ tabelê torów
   {
    iConnection=Current->pTrack->TestPoint(Point);
    if (iConnection>=0) return Current;
   }
*/
 return NULL;
};

bool __fastcall TSubRect::RaTrackAnimAdd(TTrack *t)
{//aktywacja animacji torów w VBO (zwrotnica, obrotnica)
 if (m_nVertexCount<0) return true; //nie ma animacji, gdy nie widaæ
 if (tTrackAnim)
  tTrackAnim->RaAnimListAdd(t);
 else
  tTrackAnim=t;
 return false; //bêdzie animowane...
}

void __fastcall TSubRect::RaAnimate()
{//wykonanie animacji
 if (!tTrackAnim) return; //nie ma nic do animowania
 if (Global::bUseVBO)
 {//odœwie¿enie VBO sektora
  if (Global::bOpenGL_1_5) //modyfikacje VBO s¹ dostêpne od OpenGL 1.5
   glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_nVBOVertices);
  else //dla OpenGL 1.4 z GL_ARB_vertex_buffer_object odœwie¿enie ca³ego sektora
   Release(); //opró¿nienie VBO sektora, aby siê odœwie¿y³ z nowymi ustawieniami
 }
 tTrackAnim=tTrackAnim->RaAnimate(); //przeliczenie animacji kolejnego
};

void __fastcall TSubRect::LoadNodes()
{//utworzenie siatek VBO dla wszystkich node w sektorze
 if (m_nVertexCount>=0) return; //obiekty by³y ju¿ sprawdzone
 m_nVertexCount=0; //-1 oznacza, ¿e nie sprawdzono listy obiektów
 if (!nRootNode) return;
 TGroundNode *n=nRootNode;
 while (n)
 {switch (n->iType)
  {case GL_TRIANGLE_STRIP:
   case GL_TRIANGLE_FAN:
   case GL_TRIANGLES:
    n->iVboPtr=m_nVertexCount; //nowy pocz¹tek
    m_nVertexCount+=n->iNumVerts;
    break;
   case GL_LINES:
   case GL_LINE_STRIP:
   case GL_LINE_LOOP:
    n->iVboPtr=m_nVertexCount; //nowy pocz¹tek
    m_nVertexCount+=n->iNumPts; //miejsce w tablicach normalnych i teksturowania siê zmarnuje...
    break;
   case TP_TRACK:
    n->iVboPtr=m_nVertexCount; //nowy pocz¹tek
    n->iNumVerts=n->pTrack->RaArrayPrepare(); //zliczenie wierzcho³ków
    m_nVertexCount+=n->iNumVerts;
    break;
   case TP_TRACTION:
    n->iVboPtr=m_nVertexCount; //nowy pocz¹tek
    n->iNumVerts=n->Traction->RaArrayPrepare(); //zliczenie wierzcho³ków
    m_nVertexCount+=n->iNumVerts;
    break;
  }
  n=n->nNext2; //nastêpny z sektora
 }
 if (!m_nVertexCount) return; //jeœli nie ma obiektów do wyœwietlenia z VBO, to koniec
 if (Global::bUseVBO)
 {//tylko liczenie wierzcho³ów, gdy nie ma VBO
  MakeArray(m_nVertexCount);
  n=nRootNode;
  int i;
  while (n)
  {if (n->iVboPtr>=0)
    switch (n->iType)
    {case GL_TRIANGLE_STRIP:
     case GL_TRIANGLE_FAN:
     case GL_TRIANGLES:
      for (i=0;i<n->iNumVerts;++i)
      {//Ra: trójk¹ty mo¿na od razu wczytywaæ do takich tablic... to mo¿e poczekaæ
       m_pVNT[n->iVboPtr+i].x=n->Vertices[i].Point.x;
       m_pVNT[n->iVboPtr+i].y=n->Vertices[i].Point.y;
       m_pVNT[n->iVboPtr+i].z=n->Vertices[i].Point.z;
       m_pVNT[n->iVboPtr+i].nx=n->Vertices[i].Normal.x;
       m_pVNT[n->iVboPtr+i].ny=n->Vertices[i].Normal.y;
       m_pVNT[n->iVboPtr+i].nz=n->Vertices[i].Normal.z;
       m_pVNT[n->iVboPtr+i].u=n->Vertices[i].tu;
       m_pVNT[n->iVboPtr+i].v=n->Vertices[i].tv;
      }
      break;
     case GL_LINES:
     case GL_LINE_STRIP:
     case GL_LINE_LOOP:
      for (i=0;i<n->iNumPts;++i)
      {
       m_pVNT[n->iVboPtr+i].x=n->Points[i].x;
       m_pVNT[n->iVboPtr+i].y=n->Points[i].y;
       m_pVNT[n->iVboPtr+i].z=n->Points[i].z;
       //miejsce w tablicach normalnych i teksturowania siê marnuje...
      }
      break;
     case TP_TRACK:
      if (n->iNumVerts) //bo tory zabezpieczaj¹ce s¹ niewidoczne
       n->pTrack->RaArrayFill(m_pVNT+n->iVboPtr,m_pVNT);
      break;
     case TP_TRACTION:
      if (n->iNumVerts) //druty mog¹ byæ niewidoczne...?
       n->Traction->RaArrayFill(m_pVNT+n->iVboPtr);
      break;
    }
   n=n->nNext2; //nastêpny z sektora
  }
  BuildVBOs();
 }
 if (Global::bManageNodes)
  ResourceManager::Register(this); //dodanie do automatu zwalniaj¹cego pamiêæ
}

bool __fastcall TSubRect::StartVBO()
{//pocz¹tek rysowania elementów z VBO w sektorze
 SetLastUsage(Timer::GetSimulationTime()); //te z ty³u bêd¹ niepotrzebnie zwalniane
 return CMesh::StartVBO();
};

void TSubRect::Release()
{//wirtualne zwolnienie zasobów przez sprz¹tacz albo destruktor
 if (Global::bUseVBO)
  CMesh::Clear(); //usuwanie buforów
};

void __fastcall TSubRect::RenderDL()
{//renderowanie nieprzezroczystych (DL)
 TGroundNode *node;
 RaAnimate(); //przeliczenia animacji torów w sektorze
 for (node=nRender;node;node=node->nNext3)
  node->RenderDL(); //nieprzezroczyste obiekty (oprócz pojazdów)
 for (node=nRenderMixed;node;node=node->nNext3)
  node->RenderDL(); //nieprzezroczyste z mieszanych modeli
 for (int j=0;j<iTracks;++j)
  tTracks[j]->RenderDyn(); //nieprzezroczyste fragmenty pojazdów na torach
};

void __fastcall TSubRect::RenderAlphaDL()
{//renderowanie przezroczystych modeli oraz pojazdów (DL)
 TGroundNode *node;
 for (node=nRenderMixed;node;node=node->nNext3)
  node->RenderAlphaDL(); //przezroczyste z mieszanych modeli
 for (node=nRenderAlpha;node;node=node->nNext3)
  node->RenderAlphaDL(); //przezroczyste modele
 //for (node=tmp->nRender;node;node=node->nNext3)
 // if (node->iType==TP_TRACK)
 //  node->pTrack->RenderAlpha(); //przezroczyste fragmenty pojazdów na torach
 for (int j=0;j<iTracks;++j)
  tTracks[j]->RenderDynAlpha(); //przezroczyste fragmenty pojazdów na torach
};

void __fastcall TSubRect::RenderVBO()
{//renderowanie nieprzezroczystych (VBO)
 TGroundNode *node;
 RaAnimate(); //przeliczenia animacji torów w sektorze
 LoadNodes(); //czemu tutaj?
 if (StartVBO())
 {for (node=nRenderRect;node;node=node->nNext3)
   if (node->iVboPtr>=0)
    node->RenderVBO(); //nieprzezroczyste obiekty terenu
  EndVBO();
 }
 for (node=nRender;node;node=node->nNext3)
  node->RenderVBO(); //nieprzezroczyste obiekty (oprócz pojazdów)
 for (node=nRenderMixed;node;node=node->nNext3)
  node->RenderVBO(); //nieprzezroczyste z mieszanych modeli
 for (int j=0;j<iTracks;++j)
  tTracks[j]->RenderDyn(); //nieprzezroczyste fragmenty pojazdów na torach
};

void __fastcall TSubRect::RenderAlphaVBO()
{//renderowanie przezroczystych modeli oraz pojazdów (VBO)
 TGroundNode *node;
 for (node=nRenderMixed;node;node=node->nNext3)
  node->RenderAlphaVBO(); //przezroczyste z mieszanych modeli
 for (node=nRenderAlpha;node;node=node->nNext3)
  node->RenderAlphaVBO(); //przezroczyste modele
 //for (node=tmp->nRender;node;node=node->nNext3)
 // if (node->iType==TP_TRACK)
 //  node->pTrack->RenderAlpha(); //przezroczyste fragmenty pojazdów na torach
 for (int j=0;j<iTracks;++j)
  tTracks[j]->RenderDynAlpha(); //przezroczyste fragmenty pojazdów na torach
};

//---------------------------------------------------------------------------
//------------------ Kwadrat kilometrowy ------------------------------------
//---------------------------------------------------------------------------
int TGroundRect::iFrameNumber=0; //licznik wyœwietlanych klatek

__fastcall TGroundRect::TGroundRect()
{
 pSubRects=NULL;
 nTerrain=NULL;
};

__fastcall TGroundRect::~TGroundRect()
{
 SafeDeleteArray(pSubRects);
};

void __fastcall TGroundRect::RenderDL()
{//renderowanie kwadratu kilometrowego (DL), jeœli jeszcze nie zrobione
 if (iLastDisplay!=iFrameNumber)
 {//tylko jezeli dany kwadrat nie by³ jeszcze renderowany
  //for (TGroundNode* node=pRender;node;node=node->pNext3)
  // node->Render(); //nieprzezroczyste trójk¹ty kwadratu kilometrowego
  if (nRender)
  {//³¹czenie trójk¹tów w jedn¹ listê - trochê wioska
   if (!nRender->DisplayListID||(nRender->iVersion!=Global::iReCompile))
   {//je¿eli nie skompilowany, kompilujemy wszystkie trójk¹ty w jeden
    nRender->fSquareRadius=5000.0*5000.0; //aby agregat nigdy nie znika³
    nRender->DisplayListID=glGenLists(1);
    glNewList(nRender->DisplayListID,GL_COMPILE);
    nRender->iVersion=Global::iReCompile; //aktualna wersja siatek
    for (TGroundNode* node=nRender;node;node=node->nNext3) //nastêpny tej grupy
     node->Compile(true);
    glEndList();
   }
   nRender->RenderDL(); //nieprzezroczyste trójk¹ty kwadratu kilometrowego
  }
  if (nRootMesh)
   nRootMesh->RenderDL();
  iLastDisplay=iFrameNumber; //drugi raz nie potrzeba
 }
};

void __fastcall TGroundRect::RenderVBO()
{//renderowanie kwadratu kilometrowego (VBO), jeœli jeszcze nie zrobione
 if (iLastDisplay!=iFrameNumber)
 {//tylko jezeli dany kwadrat nie by³ jeszcze renderowany
  LoadNodes(); //ewentualne tworzenie siatek
  if (StartVBO())
  {for (TGroundNode* node=nRenderRect;node;node=node->nNext3) //nastêpny tej grupy
    node->RaRenderVBO(); //nieprzezroczyste trójk¹ty kwadratu kilometrowego
   EndVBO();
   iLastDisplay=iFrameNumber;
  }                          
  if (nTerrain)
   nTerrain->smTerrain->iVisible=iFrameNumber; //ma siê wyœwietliæ w tej ramce
 }
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

void __fastcall TGround::MoveGroundNode(vector3 pPosition)
{//Ra: to wymaga gruntownej reformy
/*
 TGroundNode *Current;
 for (Current=RootNode;Current!=NULL;Current=Current->Next)
  Current->MoveMe(pPosition);

 TGroundRect *Rectx=new TGroundRect; //kwadrat kilometrowy
 for(int i=0;i<iNumRects;i++)
  for(int j=0;j<iNumRects;j++)
   Rects[i][j]=*Rectx; //kopiowanie zawartoœci do ka¿dego kwadratu
 delete Rectx;
 for (Current=RootNode;Current!=NULL;Current=Current->Next)
 {//roz³o¿enie obiektów na mapie
  if (Current->iType!=TP_DYNAMIC)
  {//pojazdów to w ogóle nie dotyczy
   if ((Current->iType!=GL_TRIANGLES)&&(Current->iType!=GL_TRIANGLE_STRIP)?true //~czy trójk¹t?
    :(Current->iFlags&0x20)?true //~czy teksturê ma nieprzezroczyst¹?
     //:(Current->iNumVerts!=3)?true //~czy tylko jeden trójk¹t?
     :(Current->fSquareMinRadius!=0.0)?true //~czy widoczny z bliska?
      :(Current->fSquareRadius<=90000.0)) //~czy widoczny z daleka?
    GetSubRect(Current->pCenter.x,Current->pCenter.z)->AddNode(Current);
   else //dodajemy do kwadratu kilometrowego
    GetRect(Current->pCenter.x,Current->pCenter.z)->AddNode(Current);
  }
 }
 for (Current=RootDynamic;Current!=NULL;Current=Current->Next)
 {
  Current->pCenter+=pPosition;
  Current->DynamicObject->UpdatePos();
 }
 for (Current=RootDynamic;Current!=NULL;Current=Current->Next)
  Current->DynamicObject->MoverParameters->Physic_ReActivation();
*/
}

__fastcall TGround::TGround()
{
 //RootNode=NULL;
 nRootDynamic=NULL;
 QueryRootEvent=NULL;
 tmpEvent=NULL;
 tmp2Event=NULL;
 OldQRE=NULL;
 RootEvent=NULL;
 iNumNodes=0;
 //pTrain=NULL;
 Global::pGround=this;
 bInitDone=false; //Ra: ¿eby nie robi³o dwa razy FirstInit
 for (int i=0;i<TP_LAST;i++)
  nRootOfType[i]=NULL; //zerowanie tablic wyszukiwania
 bDynamicRemove=false; //na razie nic do usuniêcia
 sTracks=new TNames(); //nazwy torów - na razie tak
}

__fastcall TGround::~TGround()
{
 Free();
}

void __fastcall TGround::Free()
{
 TEvent *tmp;
 for (TEvent *Current=RootEvent;Current;)
 {
  tmp=Current;
  Current=Current->Next2;
  delete tmp;
 }
 TGroundNode *tmpn;
 for (int i=0;i<TP_LAST;++i)
 {for (TGroundNode *Current=nRootOfType[i];Current;)
  {
   tmpn=Current;
   Current=Current->Next;
   delete tmpn;
  }
  nRootOfType[i]=NULL;
 }
 for (TGroundNode *Current=nRootDynamic;Current;)
 {
  tmpn=Current;
  Current=Current->Next;
  delete tmpn;
 }
 iNumNodes=0;
 //RootNode=NULL;
 nRootDynamic=NULL;
 delete sTracks;
}

TGroundNode* __fastcall TGround::FindGroundNode(AnsiString asNameToFind,TGroundNodeType iNodeType)
{//wyszukiwanie obiektu o podanej nazwie i konkretnym typie
 if (iNodeType==TP_TRACK)
 {//wyszukiwanie w drzewie binarnym
  return (TGroundNode*)sTracks->Find(TP_TRACK,asNameToFind.c_str());
 }
 //standardowe wyszukiwanie liniowe
 TGroundNode *Current;
 for (Current=nRootOfType[iNodeType];Current;Current=Current->Next)
  if (Current->asName==asNameToFind)
   return Current;
 return NULL;
}


double fTrainSetVel=0;
double fTrainSetDir=0;
double fTrainSetDist=0; //odleg³oœæ sk³adu od punktu 1 w stronê punktu 2
AnsiString asTrainSetTrack= "";
int iTrainSetConnection= 0;
bool bTrainSet= false;
AnsiString asTrainName= "";
int iTrainSetWehicleNumber= 0;
TGroundNode *TrainSetNode=NULL; //poprzedni pojazd do ³¹czenia
TGroundNode *TrainSetDriver=NULL; //pojazd, któremu zostanie wys³any rozk³ad

TGroundVertex TempVerts[10000]; //tu wczytywane s¹ trójk¹ty
Byte TempConnectionType[200]; //Ra: sprzêgi w sk³adzie; ujemne, gdy odwrotnie

void __fastcall TGround::RaTriangleDivider(TGroundNode* node)
{//tworzy dodatkowe trójk¹ty i zmiejsza podany
 //to jest wywo³ywane przy wczytywaniu trójk¹tów
 //dodatkowe trójk¹ty s¹ dodawane do g³ównej listy trójk¹tów
 //podzia³ trójk¹tów na sektory i kwadraty jest dokonywany póŸniej w FirstInit
 if (node->iType!=GL_TRIANGLES) return; //tylko pojedyncze trójk¹ty
 if (node->iNumVerts!=3) return; //tylko gdy jeden trójk¹t
 double x0=1000.0*floor(0.001*node->pCenter.x)-200.0; double x1=x0+1400.0;
 double z0=1000.0*floor(0.001*node->pCenter.z)-200.0; double z1=z0+1400.0;
 if (
  (node->Vertices[0].Point.x>=x0) && (node->Vertices[0].Point.x<=x1) &&
  (node->Vertices[0].Point.z>=z0) && (node->Vertices[0].Point.z<=z1) &&
  (node->Vertices[1].Point.x>=x0) && (node->Vertices[1].Point.x<=x1) &&
  (node->Vertices[1].Point.z>=z0) && (node->Vertices[1].Point.z<=z1) &&
  (node->Vertices[2].Point.x>=x0) && (node->Vertices[2].Point.x<=x1) &&
  (node->Vertices[2].Point.z>=z0) && (node->Vertices[2].Point.z<=z1))
  return; //trójk¹t wystaj¹cy mniej ni¿ 200m z kw. kilometrowego jest do przyjêcia
 //Ra: przerobiæ na dzielenie na 2 trójk¹ty, podzia³ w przeciêciu z siatk¹ kilometrow¹
 //Ra: i z rekurencj¹ bêdzie dzieliæ trzy trójk¹ty, jeœli bêdzie taka potrzeba
 int divide=-1; //bok do podzielenia: 0=AB, 1=BC, 2=CA; +4=podzia³ po OZ; +8 na x1/z1
 double min=0,mul; //jeœli przechodzi przez oœ, iloczyn bêdzie ujemny
 x0+=200.0; x1-=200.0; //przestawienie na siatkê
 z0+=200.0; z1-=200.0;
 mul=(node->Vertices[0].Point.x-x0)*(node->Vertices[1].Point.x-x0); //AB na wschodzie
 if (mul<min) min=mul,divide=0;
 mul=(node->Vertices[1].Point.x-x0)*(node->Vertices[2].Point.x-x0); //BC na wschodzie
 if (mul<min) min=mul,divide=1;
 mul=(node->Vertices[2].Point.x-x0)*(node->Vertices[0].Point.x-x0); //CA na wschodzie
 if (mul<min) min=mul,divide=2;
 mul=(node->Vertices[0].Point.x-x1)*(node->Vertices[1].Point.x-x1); //AB na zachodzie
 if (mul<min) min=mul,divide=8;
 mul=(node->Vertices[1].Point.x-x1)*(node->Vertices[2].Point.x-x1); //BC na zachodzie
 if (mul<min) min=mul,divide=9;
 mul=(node->Vertices[2].Point.x-x1)*(node->Vertices[0].Point.x-x1); //CA na zachodzie
 if (mul<min) min=mul,divide=10;
 mul=(node->Vertices[0].Point.z-z0)*(node->Vertices[1].Point.z-z0); //AB na po³udniu
 if (mul<min) min=mul,divide=4;
 mul=(node->Vertices[1].Point.z-z0)*(node->Vertices[2].Point.z-z0); //BC na po³udniu
 if (mul<min) min=mul,divide=5;
 mul=(node->Vertices[2].Point.z-z0)*(node->Vertices[0].Point.z-z0); //CA na po³udniu
 if (mul<min) min=mul,divide=6;
 mul=(node->Vertices[0].Point.z-z1)*(node->Vertices[1].Point.z-z1); //AB na pó³nocy
 if (mul<min) min=mul,divide=12;
 mul=(node->Vertices[1].Point.z-z1)*(node->Vertices[2].Point.z-z1); //BC na pó³nocy
 if (mul<min) min=mul,divide=13;
 mul=(node->Vertices[2].Point.z-z1)*(node->Vertices[0].Point.z-z1); //CA na pó³nocy
 if (mul<min) divide=14;
 //tworzymy jeden dodatkowy trójk¹t, dziel¹c jeden bok na przeciêciu siatki kilometrowej
 TGroundNode* ntri; //wskaŸnik na nowy trójk¹t
 ntri=new TGroundNode(); //a ten jest nowy
 ntri->iType=GL_TRIANGLES; //kopiowanie parametrów, przyda³by siê konstruktor kopiuj¹cy
 ntri->Init(3);
 ntri->TextureID=node->TextureID;
 ntri->iFlags=node->iFlags;
 for (int j=0;j<4;++j)
 {ntri->Ambient[j]=node->Ambient[j];
  ntri->Diffuse[j]=node->Diffuse[j];
  ntri->Specular[j]=node->Specular[j];
 }
 ntri->asName=node->asName;
 ntri->fSquareRadius=node->fSquareRadius;
 ntri->fSquareMinRadius=node->fSquareMinRadius;
 ntri->bVisible=node->bVisible; //a s¹ jakieœ niewidoczne?
 ntri->Next=nRootOfType[GL_TRIANGLES];
 nRootOfType[GL_TRIANGLES]=ntri; //dopisanie z przodu do listy
 iNumNodes++;
 switch (divide&3)
 {//podzielenie jednego z boków, powstaje wierzcho³ek D
  case 0: //podzia³ AB (0-1) -> ADC i DBC
   ntri->Vertices[2]=node->Vertices[2]; //wierzcho³ek C jest wspólny
   ntri->Vertices[1]=node->Vertices[1]; //wierzcho³ek B przechodzi do nowego
   //node->Vertices[1].HalfSet(node->Vertices[0],node->Vertices[1]); //na razie D tak
   if (divide&4)
    node->Vertices[1].SetByZ(node->Vertices[0],node->Vertices[1],divide&8?z1:z0);
   else
    node->Vertices[1].SetByX(node->Vertices[0],node->Vertices[1],divide&8?x1:x0);
   ntri->Vertices[0]=node->Vertices[1]; //wierzcho³ek D jest wspólny
   break;
  case 1: //podzia³ BC (1-2) -> ABD i ADC
   ntri->Vertices[0]=node->Vertices[0]; //wierzcho³ek A jest wspólny
   ntri->Vertices[2]=node->Vertices[2]; //wierzcho³ek C przechodzi do nowego
   //node->Vertices[2].HalfSet(node->Vertices[1],node->Vertices[2]); //na razie D tak
   if (divide&4)
    node->Vertices[2].SetByZ(node->Vertices[1],node->Vertices[2],divide&8?z1:z0);
   else
    node->Vertices[2].SetByX(node->Vertices[1],node->Vertices[2],divide&8?x1:x0);
   ntri->Vertices[1]=node->Vertices[2]; //wierzcho³ek D jest wspólny
   break;
  case 2: //podzia³ CA (2-0) -> ABD i DBC
   ntri->Vertices[1]=node->Vertices[1]; //wierzcho³ek B jest wspólny
   ntri->Vertices[2]=node->Vertices[2]; //wierzcho³ek C przechodzi do nowego
   //node->Vertices[2].HalfSet(node->Vertices[2],node->Vertices[0]); //na razie D tak
   if (divide&4)
    node->Vertices[2].SetByZ(node->Vertices[2],node->Vertices[0],divide&8?z1:z0);
   else
    node->Vertices[2].SetByX(node->Vertices[2],node->Vertices[0],divide&8?x1:x0);
   ntri->Vertices[0]=node->Vertices[2]; //wierzcho³ek D jest wspólny
   break;
 }
 //przeliczenie œrodków ciê¿koœci obu
 node->pCenter=(node->Vertices[0].Point+node->Vertices[1].Point+node->Vertices[2].Point)/3.0;
 ntri->pCenter=(ntri->Vertices[0].Point+ntri->Vertices[1].Point+ntri->Vertices[2].Point)/3.0;
 RaTriangleDivider(node); //rekurencja, bo nawet na TD raz nie wystarczy
 RaTriangleDivider(ntri);
};

TGroundNode* __fastcall TGround::AddGroundNode(cParser* parser)
{//wczytanie wpisu typu "node"
 //parser->LoadTraction=Global::bLoadTraction; //Ra: tu nie potrzeba powtarzaæ
 AnsiString str,str1,str2,str3,str4,Skin,DriverType,asNodeName;
 int nv,ti,i,n;
 double tf,r,rmin,tf1,tf2,tf3,tf4,l,dist,mgn;
 int int1,int2;
 bool bError=false,curve;
 vector3 pt,front,up,left,pos,tv;
 matrix4x4 mat2,mat1,mat;
 GLuint TexID;
 TGroundNode *tmp1;
 TTrack *Track;
 TRealSound *tmpsound;
 std::string token;
 parser->getTokens(2);
 *parser >> r >> rmin;
 parser->getTokens();
 *parser >> token;
 asNodeName=AnsiString(token.c_str());
 parser->getTokens();
 *parser >> token;
 str=AnsiString(token.c_str());
 TGroundNode *tmp,*tmp2;
 tmp=new TGroundNode();
 tmp->asName=(asNodeName==AnsiString("none")?AnsiString(""):asNodeName);
 if (r>=0) tmp->fSquareRadius=r*r;
 tmp->fSquareMinRadius=rmin*rmin;
 if      (str=="triangles")           tmp->iType=GL_TRIANGLES;
 else if (str=="triangle_strip")      tmp->iType=GL_TRIANGLE_STRIP;
 else if (str=="triangle_fan")        tmp->iType=GL_TRIANGLE_FAN;
 else if (str=="lines")               tmp->iType=GL_LINES;
 else if (str=="line_strip")          tmp->iType=GL_LINE_STRIP;
 else if (str=="line_loop")           tmp->iType=GL_LINE_LOOP;
 else if (str=="model")               tmp->iType=TP_MODEL;
 else if (str=="terrain")             tmp->iType=TP_TERRAIN;
 else if (str=="dynamic")             tmp->iType=TP_DYNAMIC;
 else if (str=="sound")               tmp->iType=TP_SOUND;
 else if (str=="track")               tmp->iType=TP_TRACK;
 else if (str=="memcell")             tmp->iType=TP_MEMCELL;
 else if (str=="eventlauncher")       tmp->iType=TP_EVLAUNCH;
 else if (str=="traction")            tmp->iType=TP_TRACTION;
 else if (str=="tractionpowersource") tmp->iType=TP_TRACTIONPOWERSOURCE;
// else if (str=="isolated")            tmp->iType=TP_ISOLATED;
 else bError=true;
 //WriteLog("-> node "+str+" "+tmp->asName);
 if (bError)
 {
  Error(AnsiString("Scene parse error near "+str).c_str());
  for (int i=0;i<60;++i)
  {//Ra: skopiowanie dalszej czêœci do logu - taka prowizorka, lepsza ni¿ nic
   parser->getTokens(); //pobranie linijki tekstu nie dzia³a
   *parser >> token;
   WriteLog(token.c_str());
  }
  //if (tmp==RootNode) RootNode=NULL;
  delete tmp;
  return NULL;
 }
 switch (tmp->iType)
 {
  case TP_TRACTION :
   tmp->Traction=new TTraction();
   parser->getTokens();
   *parser >> token;
   tmp->Traction->asPowerSupplyName=AnsiString(token.c_str());
   parser->getTokens(3);
   *parser >> tmp->Traction->NominalVoltage >> tmp->Traction->MaxCurrent >> tmp->Traction->Resistivity;
   parser->getTokens();
   *parser >> token;
   if (token.compare("cu")==0)
    tmp->Traction->Material=1;
   else if (token.compare("al")==0)
    tmp->Traction->Material=2;
   else
    tmp->Traction->Material=0;
   parser->getTokens();
   *parser >> tmp->Traction->WireThickness;
   parser->getTokens();
   *parser >> tmp->Traction->DamageFlag;
   parser->getTokens(3);
   *parser >> tmp->Traction->pPoint1.x >> tmp->Traction->pPoint1.y >> tmp->Traction->pPoint1.z;
   tmp->Traction->pPoint1+=pOrigin;
   parser->getTokens(3);
   *parser >> tmp->Traction->pPoint2.x >> tmp->Traction->pPoint2.y >> tmp->Traction->pPoint2.z;
   tmp->Traction->pPoint2+=pOrigin;
   parser->getTokens(3);
   *parser >> tmp->Traction->pPoint3.x >> tmp->Traction->pPoint3.y >> tmp->Traction->pPoint3.z;
   tmp->Traction->pPoint3+=pOrigin;
   parser->getTokens(3);
   *parser >> tmp->Traction->pPoint4.x >> tmp->Traction->pPoint4.y >> tmp->Traction->pPoint4.z;
   tmp->Traction->pPoint4+=pOrigin;
   parser->getTokens();
   *parser >> tf1;
   tmp->Traction->fHeightDifference=
    (tmp->Traction->pPoint3.y-tmp->Traction->pPoint1.y+
     tmp->Traction->pPoint4.y-tmp->Traction->pPoint2.y)*0.5f-tf1;
   parser->getTokens();
   *parser >> tf1;
   if (tf1>0)
    tmp->Traction->iNumSections=(tmp->Traction->pPoint1-tmp->Traction->pPoint2).Length()/tf1;
   else tmp->Traction->iNumSections=0;
   parser->getTokens();
   *parser >> tmp->Traction->Wires;
   parser->getTokens();
   *parser >> tmp->Traction->WireOffset;
   parser->getTokens();
   *parser >> token;
   tmp->bVisible=(token.compare("vis")==0);
   parser->getTokens();
   *parser >> token;
   if ( token.compare( "endtraction" )!=0)
     Error("ENDTRACTION delimiter missing! "+str2+" found instead.");
   if (Global::bLoadTraction)
    tmp->Traction->Optimize();
   tmp->pCenter=(tmp->Traction->pPoint2+tmp->Traction->pPoint1)*0.5f;
   //if (!Global::bLoadTraction) SafeDelete(tmp); //Ra: tak byæ nie mo¿e, bo NULL to b³¹d
   break;
  case TP_TRACTIONPOWERSOURCE :
   parser->getTokens(3);
   *parser >> tmp->pCenter.x >> tmp->pCenter.y >> tmp->pCenter.z;
   tmp->pCenter+=pOrigin;
   tmp->TractionPowerSource=new TTractionPowerSource();
   tmp->TractionPowerSource->Load(parser);
   break;
  case TP_MEMCELL :
   parser->getTokens(3);
   *parser >> tmp->pCenter.x >> tmp->pCenter.y >> tmp->pCenter.z;
   tmp->pCenter+=pOrigin;
   tmp->MemCell=new TMemCell(&tmp->pCenter);
   tmp->MemCell->Load(parser);
   break;
  case TP_EVLAUNCH :
   parser->getTokens(3);
   *parser >> tmp->pCenter.x >> tmp->pCenter.y >> tmp->pCenter.z;
   tmp->pCenter+=pOrigin;
   tmp->EvLaunch=new TEventLauncher();
   tmp->EvLaunch->Load(parser);
   break;
  case TP_TRACK :
   tmp->pTrack=new TTrack(tmp);
   if (DebugModeFlag)
    if (!tmp->asName.IsEmpty())
     WriteLog(tmp->asName.c_str());
   if (!tmp->asName.IsEmpty()) //jest pusta gdy "none"
    sTracks->Add(TP_TRACK,tmp->asName.c_str(),tmp); //dodanie do wyszukiwarki
   tmp->pTrack->Load(parser,pOrigin,tmp->asName); //w nazwie mo¿e byæ nazwa odcinka izolowanego
   tmp->pCenter=(tmp->pTrack->CurrentSegment()->FastGetPoint_0()+
                 tmp->pTrack->CurrentSegment()->FastGetPoint(0.5)+
                 tmp->pTrack->CurrentSegment()->FastGetPoint_1() )/3.0;
   break;
  case TP_SOUND :
   tmp->pStaticSound=new TRealSound;
   parser->getTokens(3);
   *parser >> tmp->pCenter.x >> tmp->pCenter.y >> tmp->pCenter.z;
   tmp->pCenter+=pOrigin;
   parser->getTokens();
   *parser >> token;
   str=AnsiString(token.c_str());
   tmp->pStaticSound->Init(str.c_str(),sqrt(tmp->fSquareRadius),tmp->pCenter.x,tmp->pCenter.y,tmp->pCenter.z,false);

//            tmp->pDirectSoundBuffer=TSoundsManager::GetFromName(str.c_str());
//            tmp->iState=(Parser->GetNextSymbol().LowerCase()=="loop"?DSBPLAY_LOOPING:0);
   parser->getTokens(); *parser >> token;
   break;
  case TP_DYNAMIC :
   tmp->DynamicObject=new TDynamicObject();
   //tmp->DynamicObject->Load(Parser);
   parser->getTokens();
   *parser >> token;
   str1=AnsiString(token.c_str()); //katalog
   //McZapkie: doszedl parametr ze zmienialna skora
   parser->getTokens();
   *parser >> token;
   Skin=AnsiString(token.c_str()); //tekstura wymienna
   parser->getTokens();
   *parser >> token;
   str3=AnsiString(token.c_str()); //McZapkie-131102: model w MMD
   if (bTrainSet)
   {//jeœli pojazd jest umieszczony w sk³adzie
    str=asTrainSetTrack;
    parser->getTokens();
    *parser >> tf1; //Ra: -1 oznacza odwrotne wstawienie, normalnie w sk³adzie 0
    parser->getTokens();
    *parser >> token;
    DriverType=AnsiString(token.c_str()); //McZapkie:010303 - w przyszlosci rozne konfiguracje mechanik/pomocnik itp
    tf3=fTrainSetVel; //prêdkoœæ
    parser->getTokens();
    *parser >> token;
    str4=AnsiString(token.c_str());
    int2=str4.Pos("."); //yB: wykorzystuje tutaj zmienna, ktora potem bedzie ladunkiem
    if (int2>0) //yB: jesli znalazl kropke, to ja przetwarza jako parametry
    {
      int1=str4.SubString(1, int2-1).ToInt(); //niech sprzegiem bedzie do kropki cos
      str4=str4.SubString(int2, str4.Length()+5);
    }
    else
    {
     int1=str4.ToInt();
     str4="";
    }
    int2=0; //zeruje po wykorzystaniu
//    *parser >> int1; //yB: nastawy i takie tam TUTAJ!!!!!
    TempConnectionType[iTrainSetWehicleNumber]=int1;
    iTrainSetWehicleNumber++;
   }
   else
   {//pojazd wstawiony luzem
    fTrainSetDist=0; //zerowanie dodatkowego przesuniêcia
    asTrainName=""; //puste oznacza jazdê pojedynczego bez rozk³adu, "none" jest dla sk³adu (trainset)
    parser->getTokens();
    *parser >> token;
    str=AnsiString(token.c_str()); //track
    parser->getTokens();
    *parser >> tf1; //Ra: -1 oznacza odwrotne wstawienie
    parser->getTokens();
    *parser >> token;
    DriverType=AnsiString(token.c_str()); //McZapkie:010303: obsada
    parser->getTokens();
    *parser >> tf3; //prêdkoœæ
   }
   parser->getTokens();
   *parser >> int2; //iloœæ ³adunku
   if (int2>0)
   {//je¿eli ³adunku jest wiêcej ni¿ 0, to rozpoznajemy jego typ
    parser->getTokens();
    *parser >> token;
    str2=AnsiString(token.c_str());  //LoadType
    if (str2==AnsiString("enddynamic")) //idiotoodpornoœæ: ³adunek bez podanego typu
    {
     str2=""; int2=0; //iloœæ bez typu siê nie liczy jako ³adunek
    }
   }
   else
    str2="";  //brak ladunku

   tmp1=FindGroundNode(str,TP_TRACK); //poszukiwanie toru
   if (tmp1 && tmp1->pTrack)
   {//jeœli tor znaleziony
    Track=tmp1->pTrack;
    //WriteLog("Dynamic shift: "+AnsiString(fTrainSetDist));
    tf1=tmp->DynamicObject->Init(asNodeName,str1,Skin,str3,Track,(tf1==-1.0?fTrainSetDist:tf1+fTrainSetDist),DriverType,tf3,asTrainName,int2,str2,(tf1==-1.0),str4);
    if (tf1>0.0) //zero oznacza b³¹d
    {fTrainSetDist-=tf1; //przesuniêcie dla kolejnego, minus bo idziemy w stronê punktu 1
     tmp->pCenter=tmp->DynamicObject->GetPosition();
/*
     //McZapkie-030203: sygnaly czola pociagu, ale tylko dla pociagow jadacych
     if (tf3>0.0) //predkosc poczatkowa, jak ja lubie takie nazwy zmiennych
     {//œwiat³a w sk³adzie ruchomym - AI i tak zapali je sobie po swojemu
      if (bTrainSet)
      {//czo³o jest zawsze od pierwszego pojazdu w trainset
       if (asTrainName!="none")
       {//jadacy sk³ad z rozk³adem jest oœwietlony odpowiednio
        if (iTrainSetWehicleNumber==1) //jeœli ma numer 0 (czyli aktualny jest 1)
        {
         tmp->DynamicObject->RaLightsSet(1+4+16,-1); //trojk¹t dla rozk³adowych
         if ((tmp->DynamicObject->EndSignalsLight1Active())
            ||(tmp->DynamicObject->EndSignalsLight1oldActive()))
          tmp->DynamicObject->RaLightsSet(-1,2+32); //z ty³u
         else
          tmp->DynamicObject->RaLightsSet(-1,64);
        }
        if (iTrainSetWehicleNumber==2) //jeœli jest drugi pojazd, to gasi w pierwszym
         LastDyn->RaLightsSet(-1,0); //zgaszone swiatla od strony wagonow
       }
       else
       {//jak nie ma rozk³adu, to manewruje
        if (iTrainSetWehicleNumber==1) //jeœli pierwszy w sk³adzie
         tmp->DynamicObject->RaLightsSet(16,1); //manewry
        if (iTrainSetWehicleNumber==2)
         LastDyn->RaLightsSet(-1,0); //zgaszone swiatla od strony wagonow
       }
      }
      else
       tmp->DynamicObject->RaLightsSet(16,1); //manewry pojedynczego pojazdu
      LastDyn=tmp->DynamicObject;
     }
*/
    }
    else
    {//LastNode=NULL;
     delete tmp;
     tmp=NULL; //nie mo¿e byæ tu return, bo trzeba pomin¹æ jeszcze enddynamic
    }
   }
   else
   {//gdy tor nie znaleziony
    Error("Track does not exist \""+tmp->DynamicObject->asTrack+"\"");
    delete tmp;
    tmp=NULL; //nie mo¿e byæ tu return, bo trzeba pomin¹æ jeszcze enddynamic
   }
   parser->getTokens();
   *parser >> token;
   if (token.compare("destination")==0)
   {//dok¹d wagon ma jechaæ, uwzglêdniane przy manewrach
    parser->getTokens();
    *parser >> token;
    if (tmp)
     tmp->DynamicObject->asDestination=AnsiString(token.c_str());
    *parser >> token;
   }
   if (token.compare("enddynamic")!=0)
    Error("enddynamic statement missing");
   break;
  case TP_TERRAIN:
  case TP_MODEL:
   parser->getTokens(3);
   *parser >> tmp->pCenter.x >> tmp->pCenter.y >> tmp->pCenter.z;
   parser->getTokens();
   *parser >> tf1;
   //OlO_EU&KAKISH-030103: obracanie punktow zaczepien w modelu
   tmp->pCenter.RotateY(aRotate.y/180.0*M_PI);
   //McZapkie-260402: model tez ma wspolrzedne wzgledne
   tmp->pCenter+=pOrigin;
   //tmp->fAngle+=aRotate.y; // /180*M_PI
/*
   if (tmp->iType==TP_MODEL)
   {//jeœli standardowy model
*/
    tmp->Model=new TAnimModel();
    tmp->Model->RaAnglesSet(aRotate.x,tf1+aRotate.y,aRotate.z); //dostosowanie do pochylania linii
    if (tmp->Model->Load(parser,tmp->iType==TP_TERRAIN)) //wczytanie modelu, tekstury i stanu œwiate³...
     tmp->iFlags=tmp->Model->Flags()|0x200; //ustalenie, czy przezroczysty; flaga usuwania
    else
     if (tmp->iType!=TP_TERRAIN)
     {//model nie wczyta³ siê - ignorowanie node
      delete tmp;
      tmp=NULL; //nie mo¿e byæ tu return
      break; //nie mo¿e byæ tu return?
     }
/*
   }
   else if (tmp->iType==TP_TERRAIN)
   {//nie potrzeba nak³adki animuj¹cej submodele
    *parser >> token;
    tmp->pModel3D=TModelsManager::GetModel(token.c_str(),false);
    do //Ra: z tym to trochê bez sensu jest
    {parser->getTokens();
     *parser >> token;
     str=AnsiString(token.c_str());
    } while (str!="endterrains");
   }
*/
   if (tmp->iType==TP_TERRAIN)
   {//jeœli model jest terenem, trzeba utworzyæ dodatkowe obiekty
    //po wczytaniu model ma ju¿ utworzone DL albo VBO
    Global::pTerrainCompact=tmp->Model; //istnieje co najmniej jeden obiekt terenu
    tmp->iCount=Global::pTerrainCompact->TerrainCount()+1; //zliczenie submodeli
    tmp->nNode=new TGroundNode[tmp->iCount]; //sztuczne node dla kwadratów
    tmp->nNode[0].iType=TP_MODEL; //pierwszy zawiera model (dla delete)
    tmp->nNode[0].Model=Global::pTerrainCompact;
    tmp->nNode[0].iFlags=0x200; //nie wyœwietlany, ale usuwany
    for (i=1;i<tmp->iCount;++i)
    {//a reszta to submodele
     tmp->nNode[i].iType=TP_SUBMODEL; //
     tmp->nNode[i].smTerrain=Global::pTerrainCompact->TerrainSquare(i-1);
     tmp->nNode[i].iFlags=0x10; //nieprzezroczyste; nie usuwany
     tmp->nNode[i].bVisible=true;
     tmp->nNode[i].pCenter=tmp->pCenter; //nie przesuwamy w inne miejsce
     //tmp->nNode[i].asName=
    }
   }
   //str=Parser->GetNextSymbol().LowerCase();
   break;
  case TP_GEOMETRY :
  case GL_TRIANGLES :
  case GL_TRIANGLE_STRIP :
  case GL_TRIANGLE_FAN :
   parser->getTokens();
   *parser >> token;
   //McZapkie-050702: opcjonalne wczytywanie parametrow materialu (ambient,diffuse,specular)
   if (token.compare("material")==0)
   {
    parser->getTokens();
    *parser >> token;
    while (token.compare("endmaterial")!=0)
    {
     if (token.compare("ambient:")==0)
     {
      parser->getTokens(); *parser >> tmp->Ambient[0];
      parser->getTokens(); *parser >> tmp->Ambient[1];
      parser->getTokens(); *parser >> tmp->Ambient[2];
     }
     else if (token.compare("diffuse:")==0)
     {//Ra: coœ jest nie tak, bo w jednej linijce nie dzia³a
      parser->getTokens(); *parser >> tmp->Diffuse[0];
      parser->getTokens(); *parser >> tmp->Diffuse[1];
      parser->getTokens(); *parser >> tmp->Diffuse[2];
     }
     else if (token.compare("specular:")==0)
     {
      parser->getTokens(); *parser >> tmp->Specular[0];
      parser->getTokens(); *parser >> tmp->Specular[1];
      parser->getTokens(); *parser >> tmp->Specular[2];
     }
     else Error("Scene material failure!");
     parser->getTokens();
     *parser >> token;
    }
   }
   if (token.compare("endmaterial")==0)
   {
    parser->getTokens();
    *parser >> token;
   }
   str=AnsiString(token.c_str());
#ifdef _PROBLEND
   // PROBLEND Q: 13122011 - Szociu: 27012012
   PROBLEND = true;     // domyslnie uruchomione nowe wyœwietlanie
   tmp->PROBLEND = true;  // odwolanie do tgroundnode, bo rendering jest w tej klasie
   if (str.Pos("@") > 0)     // sprawdza, czy w nazwie tekstury jest znak "@"
       {
       PROBLEND = false;     // jeœli jest, wyswietla po staremu
       tmp->PROBLEND = false;
       }
#endif
   tmp->TextureID=TTexturesManager::GetTextureID(str.c_str());
   tmp->iFlags=TTexturesManager::GetAlpha(tmp->TextureID)?0x220:0x210; //z usuwaniem
   if (((tmp->iType==GL_TRIANGLES)&&(tmp->iFlags&0x10))?Global::pTerrainCompact->TerrainLoaded():false)
   {//jeœli jest tekstura nieprzezroczysta, a teren za³adowany, to pomijamy trójk¹ty
    do
    {//pomijanie wtrójk¹tów
     parser->getTokens();
     *parser >> token;
    } while (token.compare("endtri")!=0);
    //delete tmp; //nie ma co tego trzymaæ
    //tmp=NULL; //to jest b³¹d
   }
   else
   {i=0;
    do
    {
     parser->getTokens(3);
     *parser >> TempVerts[i].Point.x >> TempVerts[i].Point.y >> TempVerts[i].Point.z;
     parser->getTokens(3);
     *parser >> TempVerts[i].Normal.x >> TempVerts[i].Normal.y >> TempVerts[i].Normal.z;
/*
     str=Parser->GetNextSymbol().LowerCase();
     if (str==AnsiString("x"))
         TempVerts[i].tu=(TempVerts[i].Point.x+Parser->GetNextSymbol().ToDouble())/Parser->GetNextSymbol().ToDouble();
     else
     if (str==AnsiString("y"))
         TempVerts[i].tu=(TempVerts[i].Point.y+Parser->GetNextSymbol().ToDouble())/Parser->GetNextSymbol().ToDouble();
     else
     if (str==AnsiString("z"))
         TempVerts[i].tu=(TempVerts[i].Point.z+Parser->GetNextSymbol().ToDouble())/Parser->GetNextSymbol().ToDouble();
     else
         TempVerts[i].tu=str.ToDouble();;

     str=Parser->GetNextSymbol().LowerCase();
     if (str==AnsiString("x"))
         TempVerts[i].tv=(TempVerts[i].Point.x+Parser->GetNextSymbol().ToDouble())/Parser->GetNextSymbol().ToDouble();
     else
     if (str==AnsiString("y"))
         TempVerts[i].tv=(TempVerts[i].Point.y+Parser->GetNextSymbol().ToDouble())/Parser->GetNextSymbol().ToDouble();
     else
     if (str==AnsiString("z"))
         TempVerts[i].tv=(TempVerts[i].Point.z+Parser->GetNextSymbol().ToDouble())/Parser->GetNextSymbol().ToDouble();
     else
         TempVerts[i].tv=str.ToDouble();;
*/
     parser->getTokens(2);
     *parser >> TempVerts[i].tu >> TempVerts[i].tv;

//    tf=Parser->GetNextSymbol().ToDouble();
   //          TempVerts[i].tu=tf;
     //        tf=Parser->GetNextSymbol().ToDouble();
       //      TempVerts[i].tv=tf;

     TempVerts[i].Point.RotateZ(aRotate.z/180*M_PI);
     TempVerts[i].Point.RotateX(aRotate.x/180*M_PI);
     TempVerts[i].Point.RotateY(aRotate.y/180*M_PI);
     TempVerts[i].Normal.RotateZ(aRotate.z/180*M_PI);
     TempVerts[i].Normal.RotateX(aRotate.x/180*M_PI);
     TempVerts[i].Normal.RotateY(aRotate.y/180*M_PI);
     TempVerts[i].Point+=pOrigin;
     tmp->pCenter+=TempVerts[i].Point;
     i++;
     parser->getTokens();
     *parser >> token;

//   }

    } while (token.compare("endtri")!=0);
    nv=i;
    tmp->Init(nv); //utworzenie tablicy wierzcho³ków
    tmp->pCenter/=(nv>0?nv:1);

//   memcpy(tmp->Vertices,TempVerts,nv*sizeof(TGroundVertex));

    r=0;
    for (int i=0;i<nv;i++)
    {
     tmp->Vertices[i]=TempVerts[i];
     tf=SquareMagnitude(tmp->Vertices[i].Point-tmp->pCenter);
     if (tf>r) r=tf;
    }

//   tmp->fSquareRadius=2000*2000+r;
    tmp->fSquareRadius+=r;
    RaTriangleDivider(tmp); //Ra: dzielenie trójk¹tów jest teraz ca³kiem wydajne
   } //koniec wczytywania trójk¹tów
   break;
  case GL_LINES :
  case GL_LINE_STRIP :
  case GL_LINE_LOOP :
   parser->getTokens(3);
   *parser >> tmp->Diffuse[0] >> tmp->Diffuse[1] >> tmp->Diffuse[2];
//   tmp->Diffuse[0]=Parser->GetNextSymbol().ToDouble()/255;
//   tmp->Diffuse[1]=Parser->GetNextSymbol().ToDouble()/255;
//   tmp->Diffuse[2]=Parser->GetNextSymbol().ToDouble()/255;
   parser->getTokens();
   *parser >> tmp->fLineThickness;
   i=0;
   parser->getTokens();
   *parser >> token;
   do
   {
    str=AnsiString(token.c_str());
    TempVerts[i].Point.x=str.ToDouble();
    parser->getTokens(2);
    *parser >> TempVerts[i].Point.y >> TempVerts[i].Point.z;
    TempVerts[i].Point.RotateZ(aRotate.z/180*M_PI);
    TempVerts[i].Point.RotateX(aRotate.x/180*M_PI);
    TempVerts[i].Point.RotateY(aRotate.y/180*M_PI);
    TempVerts[i].Point+=pOrigin;
    tmp->pCenter+=TempVerts[i].Point;
    i++;
    parser->getTokens();
    *parser >> token;
   } while (token.compare("endline")!=0);
   nv=i;
//   tmp->Init(nv);
   tmp->Points=new vector3[nv];
   tmp->iNumPts=nv;
   tmp->pCenter/=(nv>0?nv:1);
   for (int i=0;i<nv;i++)
    tmp->Points[i]=TempVerts[i].Point;
   break;
/*
  case TP_ISOLATED: //lista torów w obwodzie izolowanym - na razie ignorowana
   do
   {//po¿arcie dodatkowych parametrów
    parser->getTokens();
    *parser >> token;
   } while (token.compare("endisolated")!=0);
   break;
*/
 }
 return tmp;
}

TSubRect* __fastcall TGround::FastGetSubRect(int iCol, int iRow)
{
 int br,bc,sr,sc;
 br=iRow/iNumSubRects;
 bc=iCol/iNumSubRects;
 sr=iRow-br*iNumSubRects;
 sc=iCol-bc*iNumSubRects;
 if ( (br<0) || (bc<0) || (br>=iNumRects) || (bc>=iNumRects) ) return NULL;
 return (Rects[br][bc].FastGetRect(sc,sr));
}

TSubRect* __fastcall TGround::GetSubRect(int iCol,int iRow)
{//znalezienie ma³ego kwadratu mapy
 int br,bc,sr,sc;
 br=iRow/iNumSubRects; //wspó³rzêdne kwadratu kilometrowego
 bc=iCol/iNumSubRects;
 sr=iRow-br*iNumSubRects; //wspó³rzêdne wzglêne ma³ego kwadratu
 sc=iCol-bc*iNumSubRects;
 if ( (br<0) || (bc<0) || (br>=iNumRects) || (bc>=iNumRects) )
  return NULL; //jeœli poza map¹
 return (Rects[br][bc].SafeGetRect(sc,sr)); //pobranie ma³ego kwadratu
}

TEvent* __fastcall TGround::FindEvent(const AnsiString &asEventName)
{
 for (TEvent *Current=RootEvent;Current;Current=Current->Next2)
 {
  if (Current->asName==asEventName)
   return Current;
 }
 return NULL;
}

void __fastcall TGround::FirstInit()
{//ustalanie zale¿noœci na scenerii przed wczytaniem pojazdów
 if (bInitDone) return; //Ra: ¿eby nie robi³o siê dwa razy
 bInitDone=true;
 WriteLog("InitNormals");
 int i,j;
 for (i=0;i<TP_LAST;++i)
 {for (TGroundNode *Current=nRootOfType[i];Current;Current=Current->Next)
  {
   Current->InitNormals();
   if (Current->iType!=TP_DYNAMIC)
   {//pojazdów w ogóle nie dotyczy dodawanie do mapy
    if (i==TP_EVLAUNCH?Current->EvLaunch->IsGlobal():false)
     srGlobal.NodeAdd(Current); //dodanie do globalnego obiektu
    else if (i==TP_TERRAIN)
    {//specjalne przetwarzanie terenu wczytanego z pliku E3D
     AnsiString xxxzzz; //nazwa kwadratu
     TGroundRect *gr;
     for (j=1;j<Current->iCount;++j)
     {//od 1 do koñca s¹ zestawy trójk¹tów
      xxxzzz=AnsiString(Current->nNode[j].smTerrain->pName); //pobranie nazwy
      gr=GetRect(1000*(xxxzzz.SubString(1,3).ToIntDef(0)-500),1000*(xxxzzz.SubString(4,3).ToIntDef(0)-500));
      if (Global::bUseVBO)
       gr->nTerrain=Current->nNode+j; //zapamiêtanie
      else
       gr->RaNodeAdd(&Current->nNode[j]);
     }
    }
//    else if ((Current->iType!=GL_TRIANGLES)&&(Current->iType!=GL_TRIANGLE_STRIP)?true //~czy trójk¹t?
    else if ((Current->iType!=GL_TRIANGLES)?true //~czy trójk¹t?
     :(Current->iFlags&0x20)?true //~czy teksturê ma nieprzezroczyst¹?
      :(Current->fSquareMinRadius!=0.0)?true //~czy widoczny z bliska?
       :(Current->fSquareRadius<=90000.0)) //~czy widoczny z daleka?
     GetSubRect(Current->pCenter.x,Current->pCenter.z)->NodeAdd(Current);
    else //dodajemy do kwadratu kilometrowego
     GetRect(Current->pCenter.x,Current->pCenter.z)->NodeAdd(Current);
   }
   //if (Current->iType!=TP_DYNAMIC)
   // GetSubRect(Current->pCenter.x,Current->pCenter.z)->AddNode(Current);
  }
 }
 for (i=0;i<iNumRects;++i)
  for (j=0;j<iNumRects;++j)
   Rects[i][j].Optimize(); //optymalizacja obiektów w sektorach
 WriteLog("InitNormals OK");
 WriteLog("InitTracks");
 InitTracks(); //³¹czenie odcinków ze sob¹ i przyklejanie eventów
 WriteLog("InitTracks OK");
 WriteLog("InitEvents");
 InitEvents();
 WriteLog("InitEvents OK");
 WriteLog("InitLaunchers");
 InitLaunchers();
 WriteLog("InitLaunchers OK");
 WriteLog("InitGlobalTime");
 //ABu 160205: juz nie TODO :)
 GlobalTime=new TMTableTime(hh,mm,srh,srm,ssh,ssm); //McZapkie-300302: inicjacja czasu rozkladowego - TODO: czytac z trasy!
 WriteLog("InitGlobalTime OK");
 //jeszcze ustawienie pogody, gdyby nie by³o w scenerii wpisów
 glClearColor(Global::AtmoColor[0], Global::AtmoColor[1], Global::AtmoColor[2], 0.0);                  // Background Color
 if (Global::fFogEnd>0)
 {
  glFogi(GL_FOG_MODE, GL_LINEAR);
  glFogfv(GL_FOG_COLOR, Global::FogColor); // set fog color
  glFogf(GL_FOG_START, Global::fFogStart); // fog start depth
  glFogf(GL_FOG_END, Global::fFogEnd); // fog end depth
  glEnable(GL_FOG);
 }
 else
  glDisable(GL_FOG);
 glDisable(GL_LIGHTING);
 glLightfv(GL_LIGHT0,GL_POSITION,Global::lightPos);        //daylight position
 glLightfv(GL_LIGHT0,GL_AMBIENT,Global::ambientDayLight);  //kolor wszechobceny
 glLightfv(GL_LIGHT0,GL_DIFFUSE,Global::diffuseDayLight);  //kolor padaj¹cy
 glLightfv(GL_LIGHT0,GL_SPECULAR,Global::specularDayLight); //kolor odbity
 //musi byæ tutaj, bo wczeœniej nie mieliœmy wartoœci œwiat³a
 if (Global::bDoubleAmbient) //Ra: wczeœniej by³o ambient dawane na obydwa œwiat³a
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT,Global::ambientDayLight);
 glEnable(GL_LIGHTING);
 WriteLog("FirstInit is done");
};

bool __fastcall TGround::Init(AnsiString asFile,HDC hDC)
{//g³ówne wczytywanie scenerii
 if (asFile.LowerCase().SubString(1,7)=="scenery")
  asFile.Delete(1,8); //Ra: usuniêcie niepotrzebnych znaków - zgodnoœæ wstecz z 2003
 WriteLog("Loading scenery from "+asFile);
 Global::pGround=this;
 //pTrain=NULL;
 pOrigin=aRotate=vector3(0,0,0); //zerowanie przesuniêcia i obrotu
 AnsiString str="";
 //TFileStream *fs;
 //int size;
 std::string subpath=Global::asCurrentSceneryPath.c_str(); //   "scenery/";
 cParser parser(asFile.c_str(),cParser::buffer_FILE,subpath,Global::bLoadTraction);
 std::string token;

/*
    TFileStream *fs;
    fs=new TFileStream(asFile , fmOpenRead	| fmShareCompat	);
    AnsiString str="";
    int size=fs->Size;
    str.SetLength(size);
    fs->Read(str.c_str(),size);
    str+="";
    delete fs;
    TQueryParserComp *Parser;
    Parser=new TQueryParserComp(NULL);
    Parser->TextToParse=str;
//    Parser->LoadStringToParse(asFile);
    Parser->First();
    AnsiString Token,asFileName;
*/
    const int OriginStackMaxDepth=100; //rozmiar stosu dla zagnie¿d¿enia origin
    int OriginStackTop=0;
    vector3 OriginStack[OriginStackMaxDepth]; //stos zagnie¿d¿enia origin

    double tf;
    int ParamCount,ParamPos;

    //ABu: Jezeli nie ma definicji w scenerii to ustawiane ponizsze wartosci:
    hh=10;  //godzina startu
    mm=30;  //minuty startu
    srh=6;  //godzina wschodu slonca
    srm=0;  //minuty wschodu slonca
    ssh=20; //godzina zachodu slonca
    ssm=0;  //minuty zachodu slonca
    TGroundNode *LastNode=NULL; //do u¿ycia w trainset
    iNumNodes=0;
    token="";
    parser.getTokens();
    parser >> token;
    int refresh=0;

    while (token!="") //(!Parser->EndOfFile)
    {
     if (refresh==50)
     {//SwapBuffers(hDC); //Ra: bez ogranicznika za bardzo spowalnia :( a u niektórych miga
      refresh=0;
     }
     else ++refresh;
     str=AnsiString(token.c_str());
     if (str==AnsiString("node"))
     {
      LastNode=AddGroundNode(&parser); //rozpoznanie wêz³a
      if (LastNode)
      {//je¿eli przetworzony poprawnie
       if (LastNode->iType==GL_TRIANGLES)
       {if (!LastNode->Vertices)
         SafeDelete(LastNode); //usuwamy nieprzezroczyste trójk¹ty terenu
       }
       else if (Global::bLoadTraction?false:LastNode->iType==TP_TRACTION)
        SafeDelete(LastNode); //usuwamy druty, jeœli wy³¹czone
       if (LastNode) //dopiero na koniec dopisujemy do tablic
        if (LastNode->iType!=TP_DYNAMIC)
        {//jeœli nie jest pojazdem
         LastNode->Next=nRootOfType[LastNode->iType]; //ostatni dodany do³¹czamy na koñcu nowego
         nRootOfType[LastNode->iType]=LastNode; //ustawienie nowego na pocz¹tku listy
         iNumNodes++;
        }
        else
        {//jeœli jest pojazdem
         if (LastNode->DynamicObject->Mechanik)
          TrainSetDriver=LastNode; //pojazd, któremu zostanie wys³any rozk³ad
         LastNode->Next=nRootDynamic;
         nRootDynamic=LastNode; //dopisanie z przodu do listy
         //if (bTrainSet && (LastNode?(LastNode->iType==TP_DYNAMIC):false))
         if (TrainSetNode) //je¿eli istnieje wczeœniejszy TP_DYNAMIC
          TrainSetNode->DynamicObject->AttachPrev(LastNode->DynamicObject,TempConnectionType[iTrainSetWehicleNumber-2]);
         TrainSetNode=LastNode; //ostatnio wczytany
        }
      }
      else
      {
       Error("Scene parse error near "+AnsiString(token.c_str()));
       //break;
      }
     }
     else
     if (str==AnsiString("trainset"))
     {
      iTrainSetWehicleNumber=0;
      TrainSetNode=NULL;
      TrainSetDriver=NULL; //pojazd, któremu zostanie wys³any rozk³ad
      bTrainSet=true;
      parser.getTokens();
      parser >> token;
      asTrainName=AnsiString(token.c_str());  //McZapkie: rodzaj+nazwa pociagu w SRJP
      parser.getTokens();
      parser >> token;
      asTrainSetTrack=AnsiString(token.c_str()); //œcie¿ka startowa
      parser.getTokens(2);
      parser >> fTrainSetDist >> fTrainSetVel; //przesuniêcie i prêdkoœæ
     }
     else
     if (str==AnsiString("endtrainset"))
     {//McZapkie-110103: sygnaly konca pociagu ale tylko dla pociagow rozkladowych
      if (TrainSetNode) //trainset bez dynamic siê sypa³
      {
       if (TrainSetDriver) //pojazd, któremu zostanie wys³any rozk³ad
        TrainSetDriver->DynamicObject->Mechanik->PutCommand("Timetable:"+asTrainName,fTrainSetVel,0,NULL);
       if (asTrainName!="none")
       {//gdy podana nazwa, w³¹czenie jazdy poci¹gowej
/*
        if((TrainSetNode->DynamicObject->EndSignalsLight1Active())
         ||(TrainSetNode->DynamicObject->EndSignalsLight1oldActive()))
         TrainSetNode->DynamicObject->MoverParameters->HeadSignal=2+32;
        else
         TrainSetNode->DynamicObject->MoverParameters->EndSignalsFlag=64;
*/
       }
      }
      if (LastNode) //ostatni wczytany obiekt
       if (LastNode->iType==TP_DYNAMIC) //o ile jest pojazdem (na ogó³ jest, ale kto wie...)
        if (!TempConnectionType[iTrainSetWehicleNumber-1]) //jeœli ostatni pojazd ma sprzêg 0
         LastNode->DynamicObject->RaLightsSet(-1,2+32+64); //to za³o¿ymy mu koñcówki blaszane (jak AI siê odpali, to sobie poprawi)
      bTrainSet=false;
      fTrainSetVel=0;
      //iTrainSetConnection=0;
      TrainSetNode=NULL;
      iTrainSetWehicleNumber=0;
     }
     else if (str==AnsiString("event"))
     {
      TEvent *tmp;
      tmp=RootEvent;
      RootEvent=new TEvent();
      RootEvent->Load(&parser,&pOrigin);
      if (RootEvent->Type==tp_Unknown)
      {delete RootEvent;
       RootEvent=tmp; //przywrócenie z pominiêciem
      }
      else
      {RootEvent->Next2=tmp;
       if (RootEvent->Type!=tp_Ignored)
        if (RootEvent->asName.Pos("onstart")) //event uruchamiany automatycznie po starcie
         AddToQuery(RootEvent,NULL); //dodanie do kolejki
      }
     }
//     else
//     if (str==AnsiString("include"))  //Tolaris to zrobil wewnatrz parsera
//     {
//         Include(Parser);
//     }
     else if (str==AnsiString("rotate"))
     {
      //parser.getTokens(3);
      //parser >> aRotate.x >> aRotate.y >> aRotate.z; //Ra: to potrafi dawaæ b³êdne rezultaty
      parser.getTokens(); parser >> aRotate.x;
      parser.getTokens(); parser >> aRotate.y;
      parser.getTokens(); parser >> aRotate.z;
      //WriteLog("*** rotate "+AnsiString(aRotate.x)+" "+AnsiString(aRotate.y)+" "+AnsiString(aRotate.z));
     }
     else if (str==AnsiString("origin"))
     {
//      str=Parser->GetNextSymbol().LowerCase();
//      if (str=="begin")
      {
       if (OriginStackTop>=OriginStackMaxDepth-1)
       {
        MessageBox(0,AnsiString("Origin stack overflow ").c_str(),"Error",MB_OK);
        break;
       }
       parser.getTokens(3);
       parser >> OriginStack[OriginStackTop].x >> OriginStack[OriginStackTop].y >> OriginStack[OriginStackTop].z;
       pOrigin+=OriginStack[OriginStackTop]; //sumowanie ca³kowitego przesuniêcia
       OriginStackTop++; //zwiêkszenie wskaŸnika stosu
      }
     }
     else if (str==AnsiString("endorigin"))
     {
//      else
  //    if (str=="end")
      {
       if (OriginStackTop<=0)
       {
        MessageBox(0,AnsiString("Origin stack underflow ").c_str(),"Error",MB_OK);
        break;
       }

       OriginStackTop--; //zmniejszenie wskaŸnika stosu
       pOrigin-=OriginStack[OriginStackTop];
      }
     }
     else if (str==AnsiString("atmo"))   //TODO: uporzadkowac gdzie maja byc parametry mgly!
     {//Ra: ustawienie parametrów OpenGL przeniesione do FirstInit
      WriteLog("Scenery atmo definition");
      parser.getTokens(3);
      parser >> Global::AtmoColor[0] >> Global::AtmoColor[1] >> Global::AtmoColor[2];
      parser.getTokens(2);
      parser >> Global::fFogStart >> Global::fFogEnd;
      if (Global::fFogEnd>0.0)
      {//ostatnie 3 parametry s¹ opcjonalne
        parser.getTokens(3);
        parser >> Global::FogColor[0] >> Global::FogColor[1] >> Global::FogColor[2];
      }
      parser.getTokens();
      parser >> token;
      while (token.compare("endatmo")!=0)
      {//a kolejne parametry s¹ pomijane
       parser.getTokens();
       parser >> token;
      }
     }
     else if (str==AnsiString("time"))
     {
      WriteLog("Scenery time definition");
      char temp_in[9];
      char temp_out[9];
      int i, j;
      parser.getTokens();
      parser >> temp_in;
      for(j=0;j<=8;j++) temp_out[j]=' ';
      for (i=0; temp_in[i]!=':'; i++)
         temp_out[i]=temp_in[i];
      hh=atoi(temp_out);
      for(j=0;j<=8;j++) temp_out[j]=' ';
      for (j=i+1; j<=8; j++)
         temp_out[j-(i+1)]=temp_in[j];
      mm=atoi(temp_out);


      parser.getTokens();
      parser >> temp_in;
      for(j=0;j<=8;j++) temp_out[j]=' ';
      for (i=0; temp_in[i]!=':'; i++)
         temp_out[i]=temp_in[i];
      srh=atoi(temp_out);
      for(j=0;j<=8;j++) temp_out[j]=' ';
      for (j=i+1; j<=8; j++)
         temp_out[j-(i+1)]=temp_in[j];
      srm=atoi(temp_out);

      parser.getTokens();
      parser >> temp_in;
      for(j=0;j<=8;j++) temp_out[j]=' ';
      for (i=0; temp_in[i]!=':'; i++)
         temp_out[i]=temp_in[i];
      ssh=atoi(temp_out);
      for(j=0;j<=8;j++) temp_out[j]=' ';
      for (j=i+1; j<=8; j++)
         temp_out[j-(i+1)]=temp_in[j];
      ssm=atoi(temp_out);
      while (token.compare("endtime")!=0)
      {
       parser.getTokens();
       parser >> token;
      }
     }
     else if (str==AnsiString("light"))
     {//Ra: ustawianie œwiat³a przeniesione do FirstInit
      WriteLog("Scenery light definition");
      vector3 lp;
      parser.getTokens(); parser >> lp.x;
      parser.getTokens(); parser >> lp.y;
      parser.getTokens(); parser >> lp.z;
      lp=Normalize(lp);
      Global::lightPos[0]=lp.x; //daylight position
      Global::lightPos[1]=lp.y;
      Global::lightPos[2]=lp.z;
      parser.getTokens(); parser >> Global::ambientDayLight[0]; //kolor wszechobceny
      parser.getTokens(); parser >> Global::ambientDayLight[1];
      parser.getTokens(); parser >> Global::ambientDayLight[2];

      parser.getTokens(); parser >> Global::diffuseDayLight[0]; //kolor padaj¹cy
      parser.getTokens(); parser >> Global::diffuseDayLight[1];
      parser.getTokens(); parser >> Global::diffuseDayLight[2];

      parser.getTokens(); parser >> Global::specularDayLight[0]; //kolor odbity
      parser.getTokens(); parser >> Global::specularDayLight[1];
      parser.getTokens(); parser >> Global::specularDayLight[2];

      do
       {  parser.getTokens(); parser >> token;
       } while (token.compare("endlight")!=0);

     }
     else if (str==AnsiString("camera"))
     {
      vector3 xyz,abc;
      xyz=abc=vector3(0,0,0); //wartoœci domyœlne, bo nie wszystie musz¹ byæ
      int i=-1,into=-1; //do której definicji kamery wstawiæ
      WriteLog("Scenery camera definition");
      do
      {//opcjonalna siódma liczba okreœla numer kamery, a kiedyœ by³y tylko 3
       parser.getTokens(); parser >> token;
       switch (++i)
       {//kiedyœ camera mia³o tylko 3 wspó³rzêdne
        case 0: xyz.x=atof(token.c_str()); break;
        case 1: xyz.y=atof(token.c_str()); break;
        case 2: xyz.z=atof(token.c_str()); break;
        case 3: abc.x=atof(token.c_str()); break;
        case 4: abc.y=atof(token.c_str()); break;
        case 5: abc.z=atof(token.c_str()); break;
        case 6: into=atoi(token.c_str()); //takie sobie, bo mo¿na wpisaæ -1
       }
      } while (token.compare("endcamera")!=0);
      if (into<0) into=++Global::iCameraLast;
      if ((into>=0)&&(into<10))
      {//przepisanie do odpowiedniego miejsca w tabelce
       Global::pFreeCameraInit[into]=xyz;
       abc.x=DegToRad(abc.x);
       abc.y=DegToRad(abc.y);
       abc.z=DegToRad(abc.z);
       Global::pFreeCameraInitAngle[into]=abc;
       Global::iCameraLast=into; //numer ostatniej
      }
     }
     else if (str==AnsiString("sky"))
     {//youBy - niebo z pliku
      WriteLog("Scenery sky definition");
      parser.getTokens();
      parser >> token;
      AnsiString SkyTemp;
      SkyTemp=AnsiString(token.c_str());
      if (Global::asSky=="1") Global::asSky=SkyTemp;
      do
      {//po¿arcie dodatkowych parametrów
       parser.getTokens(); parser >> token;
      } while (token.compare("endsky")!=0);
      WriteLog(Global::asSky.c_str());
     }
     else if (str==AnsiString("firstinit"))
      FirstInit();
     else if (str==AnsiString("description"))
     {
      do
      {
       parser.getTokens();
       parser >> token;
      } while (token.compare("enddescription")!=0);
     }
     else if (str==AnsiString("test"))
     {//wypisywanie treœci po przetworzeniu
      WriteLog("---> Parser test:");
      do
      {
       parser.getTokens();
       parser >> token;
       WriteLog(token.c_str());
      } while (token.compare("endtest")!=0);
      WriteLog("---> End of parser test.");
     }
     else if (str==AnsiString("config"))
     {//mo¿liwoœæ przedefiniowania parametrów w scenerii
      Global::ConfigParse(NULL,&parser); //parsowanie dodatkowych ustawieñ
     }
     else if (str!=AnsiString(""))
     {//pomijanie od nierozpoznanej komendy do jej zakoñczenia
      if ((token.length()>2)&&(atof(token.c_str())==0.0))
      {//jeœli nie liczba, to spróbowaæ pomin¹æ komendê
       WriteLog(AnsiString("Unrecognized command: "+str));
       str="end"+str;
       do
       {
        parser.getTokens();
        token="";
        parser >> token;
       } while ((token!="")&&(token.compare(str.c_str())!=0));
      }
      else //jak liczba to na pewno b³¹d
       Error(AnsiString("Unrecognized command: "+str));
     }
     else
     if (str==AnsiString(""))
         break;

     //LastNode=NULL;

     token="";
     parser.getTokens();
     parser >> token;
    }

 delete parser;
/* //Ra: testy sortownika nazw
 //TFileStream *fs=new TFileStream("names.txt",fmCreate);
 //fs->Write(sTracks->cBuffer,4*65536);
 //delete fs;
 int *r=new int[5000]; //robocza tablica indeksów - numery posortowanych rekordów
 int *q=r; //wskaŸnik roboczy, przekazywany przez referencjê
 sTracks->rTypes[TP_TRACK]->ListGet(sTracks->rRecords,q); //drzewo jest ju¿ posortowane - zamieniæ je na listê
 for (;r<q;++r) //wyœwietlenie listy nazw
  WriteLog(sTracks->rRecords[*r].cName);
 delete[] r;
*/
 sTracks->Sort(TP_TRACK); //finalne sortowanie drzewa
 if (!bInitDone) FirstInit(); //jeœli nie by³o w scenerii
 if (Global::pTerrainCompact)
  TerrainWrite(); //Ra: teraz mo¿na zapisaæ teren w jednym pliku
 return true;
}

bool __fastcall TGround::InitEvents()
{
    TGroundNode* tmp;
    char buff[255];
    int i;
    for (TEvent *Current=RootEvent; Current!=NULL; Current= Current->Next2)
    {
        switch (Current->Type)
        {
            case tp_AddValues:
            case tp_UpdateValues:
                tmp= FindGroundNode(Current->asNodeName,TP_MEMCELL);

                if (tmp)
                 {
//McZapkie-100302
                    Current->Params[8].asGroundNode= tmp;
                    Current->Params[9].asMemCell= tmp->MemCell;
                    if (tmp->MemCell->asTrackName!=AnsiString("none"))
                     {
                      tmp=FindGroundNode(tmp->MemCell->asTrackName,TP_TRACK);
                      if (tmp!=NULL)
                       {
                        Current->Params[10].asTrack= tmp->pTrack;
                       }
                      else
                       Error("MemCell track not exist!");
                     }
                    else
                     Current->Params[10].asTrack=NULL;
                 }
                else
                    Error("Event \""+Current->asName+"\" cannot find memcell \""+
                                     Current->asNodeName+"\"");
            break;
            case tp_GetValues:
            case tp_WhoIs:
            case tp_LogValues: //skojarzenie z memcell
             tmp=FindGroundNode(Current->asNodeName,TP_MEMCELL);
             if (tmp)
             {
              Current->Params[8].asGroundNode=tmp;
              Current->Params[9].asMemCell=tmp->MemCell;
             }
             else
              Error("Event \""+Current->asName+"\" cannot find memcell \""+Current->asNodeName+"\"");
            break;
            case tp_CopyValues :
             tmp=FindGroundNode(Current->asNodeName,TP_MEMCELL); //komórka docelowa
             if (tmp)
             {
              Current->Params[8].asGroundNode=tmp;
              Current->Params[9].asMemCell=tmp->MemCell;
             }
             else
              Error("Event \""+Current->asName+"\" cannot find memcell \""+Current->asNodeName+"\"");
            break;
            case tp_Animation:
             tmp=FindGroundNode(Current->asNodeName,TP_MODEL); //egzemplarza modelu do animowania
             if (tmp)
             {
              strcpy(buff,Current->Params[9].asText); //skopiowanie nazwy submodelu do bufora roboczego
              SafeDeleteArray(Current->Params[9].asText); //usuniêcie nazwy submodelu
              Current->Params[9].asAnimContainer=tmp->Model->GetContainer(buff); //submodel
              if (Current->Params[9].asAnimContainer)
               Current->Params[9].asAnimContainer->WillBeAnimated(); //oflagowanie animacji
             }
             else
              Error("Event \""+Current->asName+"\" cannot find model \""+Current->asNodeName+"\"");
             Current->asNodeName="";
            break;
            case tp_Lights:
             tmp=FindGroundNode(Current->asNodeName,TP_MODEL);
             if (tmp)
              Current->Params[9].asModel=tmp->Model;
             else
              Error("Event \""+Current->asName+"\" cannot find model \""+
                               Current->asNodeName+"\"");
             Current->asNodeName="";
            break;
            case tp_Visible:
             tmp=FindGroundNode(Current->asNodeName,TP_MODEL); //najpierw model
             if (!tmp) tmp=FindGroundNode(Current->asNodeName,TP_TRACTION); //mo¿e druty?
             if (!tmp) tmp=FindGroundNode(Current->asNodeName,TP_TRACK); //albo tory?
             if (tmp)
              Current->Params[9].asGroundNode=tmp;
             else
              Error("Event \""+Current->asName+"\" cannot find model \""+
                               Current->asNodeName+"\"");
             Current->asNodeName="";
            break;
            case tp_Switch:
                tmp= FindGroundNode(Current->asNodeName,TP_TRACK);
                if (tmp)
                    Current->Params[9].asTrack=tmp->pTrack;
                else
                    Error("Event \""+Current->asName+"\" cannot find track \""+
                                     Current->asNodeName+"\"");
                Current->asNodeName="";
            break;
            case tp_Sound :
                tmp= FindGroundNode(Current->asNodeName,TP_SOUND);
                if (tmp)
                    Current->Params[9].asRealSound= tmp->pStaticSound;
                else
                    Error("Event \""+Current->asName+"\" cannot find static sound \""+
                                     Current->asNodeName+"\"");
                Current->asNodeName="";
            break;
            case tp_TrackVel :
                if (Current->asNodeName!=AnsiString(""))
                {
                    tmp= FindGroundNode(Current->asNodeName,TP_TRACK);
                    if (tmp)
                        Current->Params[9].asTrack= tmp->pTrack;
                    else
                        Error("Event \""+Current->asName+"\" cannot find track \""+
                                         Current->asNodeName+"\"");
                }
                Current->asNodeName= "";
            break;
            case tp_DynVel :
                if (Current->asNodeName==AnsiString("activator"))
                    Current->Params[9].asDynamic= NULL;
                else
                {
                    tmp= FindGroundNode(Current->asNodeName,TP_DYNAMIC);
                    if (tmp)
                        Current->Params[9].asDynamic= tmp->DynamicObject;
                    else
                        Error("Event \""+Current->asName+"\" cannot find dynamic \""+
                                         Current->asNodeName+"\"");
                }
                Current->asNodeName= "";
            break;
            case tp_Multiple:
             if (Current->Params[9].asText!=NULL)
             {
              strcpy(buff,Current->Params[9].asText);
              SafeDeleteArray(Current->Params[9].asText);
              if (Current->Params[8].asInt&(conditional_trackoccupied|conditional_trackfree))
              {//jeœli chodzi o zajetosc toru
               tmp=FindGroundNode(buff,TP_TRACK);
               if (tmp) Current->Params[9].asTrack=tmp->pTrack;
               if (!Current->Params[9].asTrack)
                Error(AnsiString("Track \"")+AnsiString(buff)+"\" does not exist in \""+Current->asName+"\"");
              }
              else if (Current->Params[8].asInt&(conditional_memstring|conditional_memval1|conditional_memval2))
              {//jeœli chodzi o komorke pamieciow¹
               tmp=FindGroundNode(buff,TP_MEMCELL);
               if (tmp) Current->Params[9].asMemCell=tmp->MemCell;
               if (!Current->Params[9].asMemCell)
                Error(AnsiString("MemCell \"")+AnsiString(buff)+AnsiString("\" does not exist"));
              }
             }
             for (i=0;i<8;i++)
             {
              if (Current->Params[i].asText!=NULL)
              {
               strcpy(buff,Current->Params[i].asText);
               SafeDeleteArray(Current->Params[i].asText);
               Current->Params[i].asEvent=FindEvent(buff);
               if (!Current->Params[i].asEvent) //Ra: tylko w logu informacja o braku
                if (AnsiString(Current->Params[i].asText).SubString(1,5)!="none_")
                {WriteLog(AnsiString("Event \"")+AnsiString(buff)+AnsiString("\" does not exist"));
                 ErrorLog("Missed event: "+AnsiString(buff)+" in multiple "+Current->asName);
                }
              }
             }
            break;
        }
        if (Current->fDelay<0)
            AddToQuery(Current,NULL);
    }
 return true;
}

bool __fastcall TGround::InitTracks()
{//³¹czenie torów ze sob¹ i z eventami
 TGroundNode *Current,*Model;
 TTrack *tmp; //znaleziony tor
 TTrack *Track;
 int iConnection,state;
 AnsiString name;
 //tracks=tracksfar=0;
 for (Current=nRootOfType[TP_TRACK];Current;Current=Current->Next)
 {
  Track=Current->pTrack;
  Track->AssignEvents(
   Track->asEvent0Name.IsEmpty()?NULL:FindEvent(Track->asEvent0Name),
   Track->asEvent1Name.IsEmpty()?NULL:FindEvent(Track->asEvent1Name),
   Track->asEvent2Name.IsEmpty()?NULL:FindEvent(Track->asEvent2Name));
  Track->AssignallEvents(
   Track->asEventall0Name.IsEmpty()?NULL:FindEvent(Track->asEventall0Name),
   Track->asEventall1Name.IsEmpty()?NULL:FindEvent(Track->asEventall1Name),
   Track->asEventall2Name.IsEmpty()?NULL:FindEvent(Track->asEventall2Name)); //MC-280503
  switch (Track->eType)
  {
   case tt_Turn: //obrotnicê te¿ ³¹czymy na starcie z innymi torami
    Model=FindGroundNode(Current->asName,TP_MODEL); //szukamy modelu o tej samej nazwie
    if (tmp) //mamy model, trzeba zapamiêtaæ wskaŸnik do jego animacji
    {//jak coœ pójdzie Ÿle, to robimy z tego normalny tor
     //Track->ModelAssign(tmp->Model->GetContainer(NULL)); //wi¹zanie toru z modelem obrotnicy
     Track->RaAssign(Current,Model->Model); //wi¹zanie toru z modelem obrotnicy
     //break; //jednak po³¹czê z s¹siednim, jak ma siê wysypywaæ null track
    }
   case tt_Normal: //tylko proste s¹ pod³¹czane do rozjazdów, st¹d dwa rozjazdy siê nie po³¹cz¹ ze sob¹
    if (Track->CurrentPrev()==NULL) //tylko jeœli jeszcze nie pod³¹czony
    {
     tmp=FindTrack(Track->CurrentSegment()->FastGetPoint_0(),iConnection,Current);
     switch (iConnection)
     {
      case -1: //Ra: pierwsza koncepcja zawijania samochodów i statków
       //if ((Track->iCategoryFlag&1)==0) //jeœli nie jest torem szynowym
       // Track->ConnectPrevPrev(Track,0); //³¹czenie koñca odcinka do samego siebie
       break;
      case 0:
       Track->ConnectPrevPrev(tmp,0);
      break;
      case 1:
       Track->ConnectPrevNext(tmp,1);
      break;
      case 2:
       Track->ConnectPrevPrev(tmp,0); //do Point1 pierwszego
       tmp->SetConnections(0); //zapamiêtanie ustawieñ w Segmencie
      break;
      case 3:
       Track->ConnectPrevNext(tmp,1); //do Point2 pierwszego
       tmp->SetConnections(0); //zapamiêtanie ustawieñ w Segmencie
      break;
      case 4:
       tmp->Switch(1);
       Track->ConnectPrevPrev(tmp,0); //do Point1 drugiego
       tmp->SetConnections(1); //robi te¿ Switch(0)
       tmp->Switch(0);
      break;
      case 5:
       tmp->Switch(1);
       Track->ConnectPrevNext(tmp,3); //do Point2 drugiego
       tmp->SetConnections(1); //robi te¿ Switch(0)
       tmp->Switch(0);
      break;
     }
    }
    if (Track->CurrentNext()==NULL) //tylko jeœli jeszcze nie pod³¹czony
    {
     tmp=FindTrack(Track->CurrentSegment()->FastGetPoint_1(),iConnection,Current);
     switch (iConnection)
     {
      case -1: //Ra: pierwsza koncepcja zawijania samochodów i statków
       //if ((Track->iCategoryFlag&1)==0) //jeœli nie jest torem szynowym
       // Track->ConnectNextNext(Track,1); //³¹czenie koñca odcinka do samego siebie
      break;
      case 0:
       Track->ConnectNextPrev(tmp,0);
      break;
      case 1:
       Track->ConnectNextNext(tmp,1);
      break;
      case 2:
       Track->ConnectNextPrev(tmp,0);
       tmp->SetConnections(0); //zapamiêtanie ustawieñ w Segmencie
      break;
      case 3:
       Track->ConnectNextNext(tmp,1);
       tmp->SetConnections(0); //zapamiêtanie ustawieñ w Segmencie
      break;
      case 4:
       tmp->Switch(1);
       Track->ConnectNextPrev(tmp,0);
       tmp->SetConnections(1); //robi te¿ Switch(0)
       //tmp->Switch(0);
      break;
      case 5:
       tmp->Switch(1);
       Track->ConnectNextNext(tmp,3);
       tmp->SetConnections(1); //robi te¿ Switch(0)
       //tmp->Switch(0);
      break;
     }
    }
    break;
   case tt_Switch: //dla rozjazdów szukamy eventów sygnalizacji rozprucia
    Track->AssignForcedEvents(
     FindEvent(Current->asName+":forced+"),
     FindEvent(Current->asName+":forced-"));
    break;
  }
  name=Track->IsolatedName(); //pobranie nazwy odcinka izolowanego
  if (!name.IsEmpty()) //jeœli zosta³a zwrócona nazwa
   Track->IsolatedEventsAssign(FindEvent(name+":busy"),FindEvent(name+":free"));
  if (Current->asName.SubString(1,1)=="*") //mo¿liwy portal, jeœli nie pod³¹czony od striny 1
   if (!Track->CurrentPrev()&&Track->CurrentNext())
    Track->iCategoryFlag|=0x100; //ustawienie flagi portalu
 }
 //WriteLog("Total "+AnsiString(tracks)+", far "+AnsiString(tracksfar));
 return true;
}

void __fastcall TGround::TrackJoin(TGroundNode *Current)
{//wyszukiwanie s¹siednich torów do pod³¹czenia (wydzielone na u¿ytek obrotnicy)
 TTrack *Track=Current->pTrack;
 TTrack *tmp;
 int iConnection;
 if (!Track->CurrentPrev())
 {tmp=FindTrack(Track->CurrentSegment()->FastGetPoint_0(),iConnection,Current); //Current do pominiêcia
  switch (iConnection)
  {
   case 0: Track->ConnectPrevPrev(tmp,0); break;
   case 1: Track->ConnectPrevNext(tmp,1); break;
  }
 }
 if (!Track->CurrentNext())
 {
  tmp=FindTrack(Track->CurrentSegment()->FastGetPoint_1(),iConnection,Current);
  switch (iConnection)
  {
   case 0: Track->ConnectNextPrev(tmp,0); break;
   case 1: Track->ConnectNextNext(tmp,1); break;
  }
 }
}

//McZapkie-070602: wyzwalacze zdarzen
bool __fastcall TGround::InitLaunchers()
{
 TGroundNode *Current,*tmp;
 TEventLauncher *EventLauncher;
 int i;
 for (Current=nRootOfType[TP_EVLAUNCH];Current;Current=Current->Next)
 {
  EventLauncher=Current->EvLaunch;
  if (EventLauncher->iCheckMask!=0)
   if (EventLauncher->asMemCellName!=AnsiString("none"))
   {//jeœli jest powi¹zana komórka pamiêci
    tmp=FindGroundNode(EventLauncher->asMemCellName,TP_MEMCELL);
    if (tmp)
     EventLauncher->MemCell=tmp->MemCell; //jeœli znaleziona, dopisaæ
    else
     MessageBox(0,"Cannot find Memory Cell for Event Launcher","Error",MB_OK);
   }
   else
    EventLauncher->MemCell=NULL;
  EventLauncher->Event1=(EventLauncher->asEvent1Name!=AnsiString("none"))?FindEvent(EventLauncher->asEvent1Name):NULL;
  EventLauncher->Event2=(EventLauncher->asEvent2Name!=AnsiString("none"))?FindEvent(EventLauncher->asEvent2Name):NULL;
 }
 return true;
}

TTrack* __fastcall TGround::FindTrack(vector3 Point,int &iConnection,TGroundNode *Exclude)
{//wyszukiwanie innego toru koñcz¹cego siê w (Point)
 TTrack *Track;
 TGroundNode *Current;
 TTrack *tmp;
 iConnection=-1;
 TSubRect *sr;
 //najpierw szukamy w okolicznych segmentach
 int c=GetColFromX(Point.x);
 int r=GetRowFromZ(Point.z);
 if ((sr=FastGetSubRect(c,r))!=NULL) //75% torów jest w tym samym sektorze
  if ((tmp=sr->FindTrack(&Point,iConnection,Exclude->pTrack))!=NULL)
   return tmp;
 int i,x,y;
 for (i=1;i<9;++i) //sektory w kolejnoœci odleg³oœci, 4 jest tu wystarczaj¹ce, 9 na wszelki wypadek
 {//niemal wszystkie pod³¹czone tory znajduj¹ siê w s¹siednich 8 sektorach
  x=SectorOrder[i].x;
  y=SectorOrder[i].y;
  if ((sr=FastGetSubRect(c+y,r+x))!=NULL)
   if ((tmp=sr->FindTrack(&Point,iConnection,Exclude->pTrack))!=NULL)
    return tmp;
  if ((sr=FastGetSubRect(c+y,r-x))!=NULL)
   if ((tmp=sr->FindTrack(&Point,iConnection,Exclude->pTrack))!=NULL)
    return tmp;
  if ((sr=FastGetSubRect(c-y,r+x))!=NULL)
   if ((tmp=sr->FindTrack(&Point,iConnection,Exclude->pTrack))!=NULL)
    return tmp;
  if ((sr=FastGetSubRect(c-y,r-x))!=NULL)
   if ((tmp=sr->FindTrack(&Point,iConnection,Exclude->pTrack))!=NULL)
    return tmp;
 }
#if 0
 //wyszukiwanie czo³gowe (po wszystkich jak leci) - nie ma chyba sensu
 for (Current=nRootOfType[TP_TRACK];Current;Current=Current->Next)
 {
  if ((Current->iType==TP_TRACK) && (Current!=Exclude))
  {
   iConnection=Current->pTrack->TestPoint(&Point);
   if (iConnection>=0) return Current->pTrack;
  }
 }
#endif
 return NULL;
}


/*
TGroundNode* __fastcall TGround::CreateGroundNode()
{
    TGroundNode *tmp= new TGroundNode();
//    RootNode->Prev= tmp;
    tmp->Next= RootNode;
    RootNode= tmp;
    return tmp;
}

TGroundNode* __fastcall TGround::GetVisible( AnsiString asName )
{
    MessageBox(NULL,"Error","TGround::GetVisible( AnsiString asName ) is obsolete",MB_OK);
    return RootNode->Find(asName);
//    return FirstVisible->FindVisible(asName);
}

TGroundNode* __fastcall TGround::GetNode( AnsiString asName )
{
    return RootNode->Find(asName);
}
*/
bool __fastcall TGround::AddToQuery(TEvent *Event, TDynamicObject *Node)
{
 if (!Event->bLaunched)
 {
  WriteLog("EVENT ADDED TO QUEUE: "+Event->asName);
  Event->Activator=Node;
  Event->fStartTime=fabs(Event->fDelay)+Timer::GetTime();
  Event->bLaunched=true;
  if (QueryRootEvent)
   QueryRootEvent->AddToQuery(Event);
  else
  {
   Event->Next=QueryRootEvent;
   QueryRootEvent=Event;
  }
 }
 return true;
}

bool __fastcall TGround::CheckQuery()
{
    TLocation loc;
    int i;
	double rprobability;
    Double evtime, evlowesttime;
    evlowesttime= 1000000;
if (QueryRootEvent)
   {
   OldQRE= QueryRootEvent;
   tmpEvent= QueryRootEvent;
   }
if (QueryRootEvent)
{
 for (i=0; i<90; i++)
 {
  evtime = ((tmpEvent->fStartTime)-(Timer::GetTime()));
  if (evtime<evlowesttime)
   {
   evlowesttime= evtime;
   tmp2Event= tmpEvent;
   }
  if (tmpEvent->Next)
   tmpEvent = tmpEvent->Next;
  else
   i=100;
 }

 if (OldQRE!=tmp2Event)
 {
 QueryRootEvent->AddToQuery(QueryRootEvent);
 QueryRootEvent= tmp2Event;
 }
}
    while (QueryRootEvent && QueryRootEvent->fStartTime<Timer::GetTime())
    {

        if (QueryRootEvent->bEnabled)
        {
        WriteLog("EVENT LAUNCHED: "+QueryRootEvent->asName);
        switch (QueryRootEvent->Type)
        {
         case tp_CopyValues: //pobranie wartoœci z innej komórki
/*
          QueryRootEvent->Params[0].asText=QueryRootEvent->Params[9].asMemCell->U
          QueryRootEvent->Params[1].asdouble=
          QueryRootEvent->Params[2].asdouble=
          QueryRootEvent->Params[12].asInt=conditional_memstring|conditional_memval1|conditional_memval2; //wszystko
*/
         case tp_AddValues: //ró¿ni siê jedn¹ flag¹ od UpdateValues
         case tp_UpdateValues :
          QueryRootEvent->Params[9].asMemCell->UpdateValues(QueryRootEvent->Params[0].asText,
                                                            QueryRootEvent->Params[1].asdouble,
                                                            QueryRootEvent->Params[2].asdouble,
                                                            QueryRootEvent->Params[12].asInt);
//McZapkie-100302 - updatevalues oprocz zmiany wartosci robi putcommand dla wszystkich 'dynamic' na danym torze
                if (QueryRootEvent->Params[10].asTrack)
                 {
                  //loc.X= -QueryRootEvent->Params[8].asGroundNode->pCenter.x;
                  //loc.Y=  QueryRootEvent->Params[8].asGroundNode->pCenter.z;
                  //loc.Z=  QueryRootEvent->Params[8].asGroundNode->pCenter.y;
                  for (int i=0; i<QueryRootEvent->Params[10].asTrack->iNumDynamics; i++)
                   {
                    //QueryRootEvent->Params[9].asMemCell->PutCommand(QueryRootEvent->Params[10].asTrack->Dynamics[i]->Mechanik,loc);
                    QueryRootEvent->Params[9].asMemCell->PutCommand(QueryRootEvent->Params[10].asTrack->Dynamics[i]->Mechanik,&QueryRootEvent->Params[8].asGroundNode->pCenter);
                   }
                  LogComment="Type: UpdateValues & Track command - ";
                  LogComment+=AnsiString(QueryRootEvent->Params[0].asText);
                 }
                else
                  {
                   LogComment="Type: UpdateValues - ";
                   LogComment+=AnsiString(QueryRootEvent->Params[0].asText);
                  }
                if (DebugModeFlag)
                 WriteLog(LogComment.c_str());
            break;
            case tp_GetValues :
             if (QueryRootEvent->Activator)
             {
              //loc.X= -QueryRootEvent->Params[8].asGroundNode->pCenter.x;
              //loc.Y=  QueryRootEvent->Params[8].asGroundNode->pCenter.z;
              //loc.Z=  QueryRootEvent->Params[8].asGroundNode->pCenter.y;
              if (Global::iMultiplayer) //potwierdzenie wykonania dla serwera - najczêœciej odczyt semafora
               WyslijEvent(QueryRootEvent->asName,QueryRootEvent->Activator->GetName());
               //QueryRootEvent->Params[9].asMemCell->PutCommand(QueryRootEvent->Activator->Mechanik,loc);
              QueryRootEvent->Params[9].asMemCell->PutCommand(QueryRootEvent->Activator->Mechanik,&QueryRootEvent->Params[8].asGroundNode->pCenter);
             }
             WriteLog("Type: GetValues");
            break;
            case tp_PutValues :
             if (QueryRootEvent->Activator)
             {
              loc.X=-QueryRootEvent->Params[3].asdouble; //zamiana, bo fizyka ma inaczej ni¿ sceneria
              loc.Y= QueryRootEvent->Params[5].asdouble;
              loc.Z= QueryRootEvent->Params[4].asdouble;
              if (QueryRootEvent->Activator->Mechanik) //przekazanie rozkazu do AI
               QueryRootEvent->Activator->Mechanik->PutCommand(QueryRootEvent->Params[0].asText,
                                                               QueryRootEvent->Params[1].asdouble,
                                                               QueryRootEvent->Params[2].asdouble,loc);
              else
              {//przekazanie do pojazdu
               QueryRootEvent->Activator->MoverParameters->PutCommand(QueryRootEvent->Params[0].asText,
                                                                      QueryRootEvent->Params[1].asdouble,
                                                                      QueryRootEvent->Params[2].asdouble,loc);
              }
             }
             WriteLog("Type: PutValues");
            break;
            case tp_Lights :
             if (QueryRootEvent->Params[9].asModel)
              for (i=0; i<iMaxNumLights; i++)
               if (QueryRootEvent->Params[i].asInt>=0) //-1 zostawia bez zmiany
                QueryRootEvent->Params[9].asModel->lsLights[i]=(TLightState)QueryRootEvent->Params[i].asInt;
            break;
            case tp_Visible:
             if (QueryRootEvent->Params[9].asGroundNode)
              QueryRootEvent->Params[9].asGroundNode->bVisible=(QueryRootEvent->Params[i].asInt>0);
            break;
            case tp_Velocity :
                Error("Not implemented yet :(");
            break;
            case tp_Exit :
                MessageBox(0,QueryRootEvent->asNodeName.c_str()," THE END ",MB_OK);
                return false;
            case tp_Sound :
             if (QueryRootEvent->Params[0].asInt==0)
               QueryRootEvent->Params[9].asRealSound->Stop();
             if (QueryRootEvent->Params[0].asInt==1)
               QueryRootEvent->Params[9].asRealSound->Play(1,0,true,QueryRootEvent->Params[9].asRealSound->vSoundPosition);
             if (QueryRootEvent->Params[0].asInt==-1)
               QueryRootEvent->Params[9].asRealSound->Play(1,DSBPLAY_LOOPING,true,QueryRootEvent->Params[9].asRealSound->vSoundPosition);
            break;
            case tp_Disable :
                Error("Not implemented yet :(");
            break;
            case tp_Animation : //Marcin: dorobic translacje - Ra: dorobi³em ;-)
             if (QueryRootEvent->Params[0].asInt==1)
               QueryRootEvent->Params[9].asAnimContainer->SetRotateAnim(
                    vector3(QueryRootEvent->Params[1].asdouble,
                            QueryRootEvent->Params[2].asdouble,
                            QueryRootEvent->Params[3].asdouble),
                            QueryRootEvent->Params[4].asdouble);
             else if (QueryRootEvent->Params[0].asInt==2)
               QueryRootEvent->Params[9].asAnimContainer->SetTranslateAnim(
                    vector3(QueryRootEvent->Params[1].asdouble,
                            QueryRootEvent->Params[2].asdouble,
                            QueryRootEvent->Params[3].asdouble),
                            QueryRootEvent->Params[4].asdouble);
            break;
            case tp_Switch :
             if (QueryRootEvent->Params[9].asTrack)
              QueryRootEvent->Params[9].asTrack->Switch(QueryRootEvent->Params[0].asInt);
             if (Global::iMultiplayer) //dajemy znaæ do serwera o prze³o¿eniu
              WyslijEvent(QueryRootEvent->asName,""); //wys³anie nazwy eventu prze³¹czajacego
             //Ra: bardziej by siê przyda³a nazwa toru, ale nie ma do niej st¹d dostêpu
            break;
            case tp_TrackVel :
             if (QueryRootEvent->Params[9].asTrack)
             {
              WriteLog("type: TrackVel");
//            WriteLog("Vel: ",QueryRootEvent->Params[0].asdouble);
              QueryRootEvent->Params[9].asTrack->VelocitySet(QueryRootEvent->Params[0].asdouble);
              if (DebugModeFlag)
               WriteLog("vel: ",QueryRootEvent->Params[9].asTrack->VelocityGet());
             }
            break;
            case tp_DynVel :
             Error("Event \"DynVel\" is obsolete");
            break;
            case tp_Multiple :
               {
                bCondition=(QueryRootEvent->Params[8].asInt==0);  //bezwarunkowo
                if (!bCondition)                                  //warunkowo
                 {
                  if (QueryRootEvent->Params[8].asInt==conditional_trackoccupied)
                   bCondition=(!QueryRootEvent->Params[9].asTrack->IsEmpty());
                  else
                  if (QueryRootEvent->Params[8].asInt==conditional_trackfree)
                   bCondition=(QueryRootEvent->Params[9].asTrack->IsEmpty());
                  else if (QueryRootEvent->Params[8].asInt==conditional_propability)
                  {
                   rprobability=1.0*rand()/RAND_MAX;
                   bCondition=(QueryRootEvent->Params[10].asdouble>rprobability);
                   WriteLog("Random integer: "+CurrToStr(rprobability)+"/"+CurrToStr(QueryRootEvent->Params[10].asdouble));
                  }
                  else
                  {
                   bCondition=
                   QueryRootEvent->Params[9].asMemCell->Compare(QueryRootEvent->Params[10].asText,
                                                                QueryRootEvent->Params[11].asdouble,
                                                                QueryRootEvent->Params[12].asdouble,
                                                                QueryRootEvent->Params[8].asInt);
                   if (!bCondition && Global::iWriteLogEnabled && DebugModeFlag) //nie zgadza sie wiec sprawdzmy co
                     {
                       LogComment="";
                       if (TestFlag(QueryRootEvent->Params[8].asInt,conditional_memstring))
                        LogComment=AnsiString(QueryRootEvent->Params[10].asText)+"="+QueryRootEvent->Params[9].asMemCell->Text();
                       if (TestFlag(QueryRootEvent->Params[8].asInt,conditional_memval1))
                        LogComment+=" v1:"+FloatToStrF(QueryRootEvent->Params[11].asdouble,ffFixed,8,2)+"="+FloatToStrF(QueryRootEvent->Params[9].asMemCell->Value1(),ffFixed,8,2);
                       if (TestFlag(QueryRootEvent->Params[8].asInt,conditional_memval2))
                        LogComment+=" v2:"+FloatToStrF(QueryRootEvent->Params[12].asdouble,ffFixed,8,2)+"="+FloatToStrF(QueryRootEvent->Params[9].asMemCell->Value2(),ffFixed,8,2);
                       WriteLog(LogComment.c_str());
                     }
                   }
                 }
                if (bCondition)                                  //warunek spelniony
                {
                 WriteLog("Multiple passed");
                 for (i=0; i<8; i++)
                 {
                  if (QueryRootEvent->Params[i].asEvent)
                   AddToQuery(QueryRootEvent->Params[i].asEvent,QueryRootEvent->Activator);
                 }
                 if (Global::iMultiplayer) //dajemy znaæ do serwera o wykonaniu
                 {
                  if (QueryRootEvent->Activator)
                   WyslijEvent(QueryRootEvent->asName,QueryRootEvent->Activator->GetName());
                  else
                   WyslijEvent(QueryRootEvent->asName,"");
                 }
                }
               }
            break;
            case tp_WhoIs: //pobranie nazwy poci¹gu do komórki pamiêci
             if (QueryRootEvent->Activator->Mechanik)
             {//tylko jeœli ktoœ tam siedzi - nie powinno dotyczyæ pasa¿era!
              QueryRootEvent->Params[9].asMemCell->UpdateValues(
               QueryRootEvent->Activator->Mechanik->TrainName().c_str(),
               QueryRootEvent->Activator->Mechanik->StationCount(),
               QueryRootEvent->Activator->Mechanik->StationIndex(),
               conditional_memstring|conditional_memval1|conditional_memval2);
              WriteLog("Train detected: "+QueryRootEvent->Activator->Mechanik->TrainName());
             }
            break;
            case tp_LogValues: //zapisanie zawartoœci komórki pamiêci do logu
             WriteLog("Memcell \""+QueryRootEvent->asNodeName+"\": "+
              QueryRootEvent->Params[9].asMemCell->Text()+", "+
              QueryRootEvent->Params[9].asMemCell->Value1()+", "+
              QueryRootEvent->Params[9].asMemCell->Value2());
            break;
        }
        };
        QueryRootEvent->bLaunched= false;
        QueryRootEvent->fStartTime= 0;
        QueryRootEvent= QueryRootEvent->Next;
    }
    return true;

}

void __fastcall TGround::OpenGLUpdate(HDC hDC)
{
 SwapBuffers(hDC); //swap buffers (double buffering)
}

bool __fastcall TGround::Update(double dt, int iter)
//blablabla
{
   if(iter>1) //ABu: ponizsze wykonujemy tylko jesli wiecej niz jedna iteracja
   {
      //Pierwsza iteracja i wyznaczenie stalych:
      for (TGroundNode *Current=nRootDynamic;Current;Current=Current->Next)
      {
         Current->DynamicObject->MoverParameters->ComputeConstans();
         Current->DynamicObject->CoupleDist();
         Current->DynamicObject->UpdateForce(dt,dt,false);
      }
      for (TGroundNode *Current=nRootDynamic;Current;Current=Current->Next)
      {
         Current->DynamicObject->FastUpdate(dt);
      }
      //pozostale iteracje
      for(int i=1 ; i<(iter-1); i++)
      {
         for (TGroundNode *Current=nRootDynamic;Current;Current=Current->Next)
         {
            Current->DynamicObject->UpdateForce(dt,dt,false);
         }
         for (TGroundNode *Current=nRootDynamic;Current;Current=Current->Next)
         {
            Current->DynamicObject->FastUpdate(dt);
         }
      }
      //ABu 200205: a to robimy tylko raz, bo nie potrzeba wiecej
      //Winger 180204 - pantografy
      double dt1=dt*iter;
      if (Global::bEnableTraction)
      {
         for (TGroundNode *Current=nRootDynamic;Current;Current=Current->Next)
         {
            if ((Current->DynamicObject->MoverParameters->EnginePowerSource.SourceType==CurrentCollector)
            //ABu: usunalem, bo sie krzaczylo: && (Current->DynamicObject->MoverParameters->PantFrontUp || Current->DynamicObject->MoverParameters->PantRearUp))
            //     a za to dodalem to:
               &&(Current->DynamicObject->MoverParameters->CabNo!=0))
               GetTraction(Current->DynamicObject->GetPosition(), Current->DynamicObject);
            Current->DynamicObject->UpdateForce(dt, dt1, true);//,true);
         }
         for (TGroundNode *Current=nRootDynamic;Current;Current=Current->Next)
         {
            Current->DynamicObject->Update(dt,dt1);
         }
      }
      else
      {
         for (TGroundNode *Current=nRootDynamic;Current;Current=Current->Next)
         {
            Current->DynamicObject->UpdateForce(dt, dt1, true);//,true);
         }
         for (TGroundNode *Current=nRootDynamic;Current;Current=Current->Next)
         {
            Current->DynamicObject->Update(dt,dt1);
         }
      }
   }
   else
   //jezeli jest tylko jedna iteracja
   {
      if (Global::bEnableTraction)
      {
         for (TGroundNode *Current=nRootDynamic;Current;Current=Current->Next)
         {
            if ((Current->DynamicObject->MoverParameters->EnginePowerSource.SourceType==CurrentCollector)
               &&(Current->DynamicObject->MoverParameters->CabNo!=0))
               GetTraction(Current->DynamicObject->GetPosition(), Current->DynamicObject);
            Current->DynamicObject->MoverParameters->ComputeConstans();
            Current->DynamicObject->CoupleDist();
            Current->DynamicObject->UpdateForce(dt,dt,true);//,true);
         }
         for (TGroundNode *Current=nRootDynamic;Current;Current=Current->Next)
         {
            Current->DynamicObject->Update(dt,dt);
         }
      }
      else
      {
         for (TGroundNode *Current=nRootDynamic;Current;Current=Current->Next)
         {
            Current->DynamicObject->MoverParameters->ComputeConstans();
            Current->DynamicObject->CoupleDist();
            Current->DynamicObject->UpdateForce(dt,dt,true);//,true);
         }
         for (TGroundNode *Current=nRootDynamic;Current;Current=Current->Next)
         {
            Current->DynamicObject->Update(dt,dt);
         }
      }
   }
 if (bDynamicRemove)
 {//jeœli jest coœ do usuniêcia z listy, to trzeba na koñcu
  for (TGroundNode *Current=nRootDynamic;Current;Current=Current->Next)
   if (!Current->DynamicObject->bEnabled)
   {
    DynamicRemove(Current->DynamicObject); //usuniêcie tego i pod³¹czonych
    Current=nRootDynamic; //sprawdzanie listy od pocz¹tku
   }
  bDynamicRemove=false; //na razie koniec
 }
 return true;
}

//Winger 170204 - szukanie trakcji nad pantografami
bool __fastcall TGround::GetTraction(vector3 pPosition, TDynamicObject *model)
{//Ra: to siê powinno daæ uproœciæ
    double t1x,t1y,t1z,t2x,t2y,t2z,dx,dy,dz,p1rx,p1rz,p2rx,p2rz,odl1,odl2,ntx1,ntx2,nty1,nty2,ntz1,ntz2;
    double p1x,p1z,p2x,p2z,py;
    double bp1xl,bp1y,bp1zl,bp2xl,bp2y,bp2zl,bp1xp,bp1zp,bp2xp,bp2zp;
    double t1p1x,t1p1y,t1p1z,t2p1x,t2p1y,t2p1z,wsp1,wsp2;
    double odlt1t2, odlt1p1, rwt1t2, odlt2p1;
    double wsp1wb,np1wx,np1wy,np1wz,np2wx,np2wy,np2wz,zt1,zt2,zp1w,zp2w;
    double t1p2x,t1p2y,t1p2z,t2p2x,t2p2y,t2p2z;
    double odlt1p2,odlt2p2;
    double wsp2wb,dynwys,nty1pk,nty2pk;
    double zt1x,zt2x,p1wy,p2wy,wspwp;
    double maxwyspant,minwyspant,szerpant;
    double p1wx,p1wz,p2wx,p2wz,liczwsp1,liczwsp2,p1a1,p1a2,p1b1,p1b2,p2a1,p2a2,p2b1,p2b2;
    vector3 dirx, diry, dirz, dwys;
    vector3 pant1,pant2, pant1l, pant1p, pant2l, pant2p;
    vector3 tr1p1, tr1p2, tr2p1, tr2p2;
    TGroundNode *node,*oldnode;
    p1z= model->pant1x;
    py= model->panty;
    p2z= model->pant2x;
//    model->mdModel->GetFromName
    wspwp= model->panth;
    dirz= model->VectorFront();
    diry= model->VectorUp();
    dirx= model->VectorLeft();
    dwys= model->GetPosition();
    dynwys= dwys.y;
    np1wy=1000;
    np2wy=1000;
    //p1wy=0;
    //p2wy=0;
    int n= 1; //iloœæ sektorów mapy do przeszukania
    int c= GetColFromX(pPosition.x);
    int r= GetRowFromZ(pPosition.z);
    TSubRect *tmp,*tmp2;
    pant1= pPosition+(dirx*0)+(diry*py)+(dirz*p1z);
    pant1l= pant1+(dirx*-1)+(diry*0)+(dirz*0);
    pant1p= pant1+(dirx*1)+(diry*0)+(dirz*0);
    pant2= pPosition+(dirx*0)+(diry*py)+(dirz*p2z);
    pant2l= pant2+(dirx*-1)+(diry*0)+(dirz*0);
    pant2p= pant2+(dirx*1)+(diry*0)+(dirz*0);
    bp1xl= pant1l.x;
    bp1zl= pant1l.z;
    bp1xp= pant1p.x;
    bp1zp= pant1p.z;
    bp2xl= pant2l.x;
    bp2zl= pant2l.z;
    bp2xp= pant2p.x;
    bp2zp= pant2p.z;
    for (int j= r-n; j<=r+n; j++)
        for (int i= c-n; i<=c+n; i++)
        {
            tmp= FastGetSubRect(i,j);
            if (tmp)
            {
                for (node=tmp->nRenderWires;node;node=node->nNext3)  //nastêpny z grupy
                {//w grupie tej s¹ druty oraz inne linie
                    if (node->iType==TP_TRACTION)
                    {
                        t1x= node->Traction->pPoint1.x;
                        t1z= node->Traction->pPoint1.z;
                        t1y= node->Traction->pPoint1.y;
                        t2x= node->Traction->pPoint2.x;
                        t2z= node->Traction->pPoint2.z;
                        t2y= node->Traction->pPoint2.y;
                        //Patyk 1
                         p1wx=277;
                         p1wz=277;
                         //zt1x=t1x;
                         //zt2x=t2x;
                         liczwsp1=(t2z-t1z)*(bp1xp-bp1xl)-(bp1zp-bp1zl)*(t2x-t1x);
                         if (liczwsp1!=0)
                          {
                          p1wx=((bp1xp-bp1xl)*(t2x-t1x)*(bp1zl-t1z)-(bp1zp-bp1zl)*(t2x-t1x)*bp1xl+(t2z-t1z)*(bp1xp-bp1xl)*t1x)/liczwsp1;
                          liczwsp2= t2x-t1x;
                          if (liczwsp2!=0)
                           p1wz=(t2z-t1z)*(p1wx-t1x)/liczwsp2+t1z;
                          else
                           {
                           liczwsp2=bp1xp-bp1xl;
                           if (liczwsp2!=0)
                            p1wz=(bp1zp-bp1zl)*(p1wx-bp1xl)/liczwsp2+bp1zl;
                           }
                          p1a1=hypot(p1wx-t1x,p1wz-t1z);
                          p1a2=hypot(p1wx-t2x,p1wz-t2z);
                          p1b1=hypot(p1wx-bp1xl,p1wz-bp1zl);
                          p1b2=hypot(p1wx-bp1xp,p1wz-bp1zp);
                          if ((p1a1+p1a2-0.1>hypot(t2x-t1x,t2z-t1z)) || (p1b1+p1b2-1>hypot(bp1xp-bp1xl,bp1zp-bp1zl)))
                           {
                           p1wx=277;
                           p1wz=277;
                           }
                          }
                          if ((p1wx!=277) && (p1wz!=277))
                           {
                                zt1=t1x-t2x;
                                if (zt1<0)
                                 zt1=-zt1;
                                zt2=t1z-t2z;
                                if (zt2<0)
                                 zt2=-zt2;
                                if (zt1<=zt2)
                                 {
                                 zt1x=t1z;
                                 zt2x=t2z;
                                 zp1w=p1wz;
                                 }
                                else
                                 {
                                 zt1x=t1x;
                                 zt2x=t2x;
                                 zp1w=p1wx;
                                 }
                                p1wy=(t2y-t1y)*(zp1w-zt1x)/(zt2x-zt1x)+t1y;
                                if (p1wy<np1wy)
                                 np1wy=p1wy;
                          }

                        //Patyk 2
                         p2wx=277;
                         p2wz=277;
                         //zt1x=t1x;
                         //zt2x=t2x;
                         liczwsp1=(t2z-t1z)*(bp2xp-bp2xl)-(bp2zp-bp2zl)*(t2x-t1x);
                         if (liczwsp1!=0)
                          {
                          p2wx=((bp2xp-bp2xl)*(t2x-t1x)*(bp2zl-t1z)-(bp2zp-bp2zl)*(t2x-t1x)*bp2xl+(t2z-t1z)*(bp2xp-bp2xl)*t1x)/liczwsp1;
                          liczwsp2= t2x-t1x;
                          if (liczwsp2!=0)
                           p2wz=(t2z-t1z)*(p2wx-t1x)/liczwsp2+t1z;
                          else
                           {
                           liczwsp2=bp2xp-bp2xl;
                           if (liczwsp2!=0)
                            p2wz=(bp2zp-bp2zl)*(p2wx-bp2xl)/liczwsp2+bp2zl;
                           }
                          p2a1=hypot(p2wx-t1x,p2wz-t1z);
                          p2a2=hypot(p2wx-t2x,p2wz-t2z);
                          p2b1=hypot(p2wx-bp2xl,p2wz-bp2zl);
                          p2b2=hypot(p2wx-bp2xp,p2wz-bp2zp);
                          if ((p2a1+p2a2-0.1>hypot(t2x-t1x,t2z-t1z)) || (p2b1+p2b2-1>hypot(bp2xp-bp2xl,bp2zp-bp2zl)))
                           {
                           p2wx=277;
                           p2wz=277;
                           }
                          }
                          if ((p2wx!=277) && (p2wz!=277))
                           {
                                zt1=t1x-t2x;
                                if (zt1<0)
                                 zt1=-zt1;
                                zt2=t1z-t2z;
                                if (zt2<0)
                                 zt2=-zt2;
                                if (zt1<=zt2)
                                 {
                                 zt1x=t1z;
                                 zt2x=t2z;
                                 zp2w=p2wz;
                                 }
                                else
                                 {
                                 zt1x=t1x;
                                 zt2x=t2x;
                                 zp2w=p2wx;
                                 }
                                p2wy=(t2y-t1y)*(zp2w-zt1x)/(zt2x-zt1x)+t1y;
                                if (p2wy<np2wy)
                                 np2wy=p2wy;
                          }
                    }
                }
            }
        }

    nty1=np1wy-dynwys+0.4-wspwp;
    nty2=np2wy-dynwys+0.4-wspwp;
    nty1pk=nty1;
    nty2pk=nty2;
    wsp1wb=nty1-6.039;
    if (wsp1wb<0)
    wsp1wb=-wsp1wb;
    wsp1= 1-wsp1wb;
    nty1= nty1-(wsp1*0.17);
    if (nty1pk>6.039)
    {
        wsp1wb=nty1pk-6.544;
        if (wsp1wb<0)
        wsp1wb=-wsp1wb;
        wsp1= 0.5-wsp1wb;
        nty1= nty1-(wsp1*0.19);
    }
    if (nty1<0)
    nty1=-nty1;
    if ((nty1<10)&&(nty1>0))
     model->PantTraction1= nty1;
    wsp1wb=nty2-6.039;
    if (wsp1wb<0)
    wsp1wb=-wsp1wb;
    wsp1= 1-wsp1wb;
    nty2= nty2-(wsp1*0.17);
    if (nty2pk>6.039)
    {
        wsp1wb=nty2pk-6.544;
        if (wsp1wb<0)
        wsp1wb=-wsp1wb;
        wsp1= 0.5-wsp1wb;
        nty2= nty2-(wsp1*0.19);
    }
    if (nty2<0)
    nty2=-nty2;
    if ((nty2<10)&&(nty2>0))
     model->PantTraction2= nty2;
    if ((np1wy==1000) && (np2wy==1000))
     {
     model->PantTraction1= 5.8171;
     model->PantTraction2= 5.8171;
     }
    return true;
}

bool __fastcall TGround::RenderDL(vector3 pPosition)
{//renderowanie scenerii z Display List - faza nieprzezroczystych
 ++TGroundRect::iFrameNumber; //zwiêszenie licznika ramek (do usuwniania nadanimacji)
 CameraDirection.x=sin(Global::pCameraRotation); //wektor kierunkowy
 CameraDirection.z=cos(Global::pCameraRotation);
 int tr,tc;
 TGroundNode *node;
 glColor3f(1.0f,1.0f,1.0f);
 glEnable(GL_LIGHTING);
 int n=2*iNumSubRects; //(2*==2km) promieñ wyœwietlanej mapy w sektorach
 int c=GetColFromX(pPosition.x);
 int r=GetRowFromZ(pPosition.z);
 TSubRect *tmp;
 for (node=srGlobal.nRenderHidden;node;node=node->nNext3)
  node->RenderHidden(); //rednerowanie globalnych (nie za czêsto?)
 int i,j,k;
 //renderowanie czo³gowe dla obiektów aktywnych a niewidocznych
 for (j=r-n;j<=r+n;j++)
  for (i=c-n;i<=c+n;i++)
   if ((tmp=FastGetSubRect(i,j))!=NULL)
   {tmp->LoadNodes(); //oznaczanie aktywnych sektorów
    for (node=tmp->nRenderHidden;node;node=node->nNext3)
     node->RenderHidden();
   }
 //renderowanie progresywne - zale¿ne od FPS oraz kierunku patrzenia
 iRendered=0; //iloœæ renderowanych sektorów
 vector3 direction;
 //iRange=(Global::iSlowMotion&6)?((Global::iSlowMotion&4)?AreaMini:AreaSlow):AreaFast;
 //n=(iRange[0]*n)/10; //przeliczenie (n) do aktualnego promienia rednerowania
 //n=(Global::iSlowMotion&6)?((Global::iSlowMotion&4)?25:52):90; //iloœæ sektorów w æwiartce (max 400)
 for (k=0;k<Global::iSegmentsRendered;++k) //sektory w kolejnoœci odleg³oœci
 {//przerobione na u¿ycie SectorOrder
  i=SectorOrder[k].x; //na starcie oba >=0
  j=SectorOrder[k].y;
  do
 //for (j=-n;j<=n;j++)
 //{k=iRange[j<0?-j:j]; //zasiêg na danym poziomie
 // for (i=-k;i<=k;i++)
  {
   if (j<=0) i=-i; //pierwszy przebieg: j<=0, i>=0; drugi: j>=0, i<=0; trzeci: j<=0, i<=0 czwarty: j>=0, i>=0;
   j=-j; //i oraz j musi byæ zmienione wczeœniej, ¿eby continue dzia³a³o
   direction=vector3(i,0,j); //wektor od kamery do danego sektora
   if (LengthSquared3(direction)>5) //te blisko s¹ zawsze wyœwietlane
   {direction=SafeNormalize(direction); //normalizacja
    if (CameraDirection.x*direction.x+CameraDirection.z*direction.z<0.55)
     continue; //pomijanie sektorów poza k¹tem patrzenia
   }
   Rects[(i+c)/iNumSubRects][(j+r)/iNumSubRects].RenderDL(); //kwadrat kilometrowy nie zawsze, bo szkoda FPS
   if ((tmp=FastGetSubRect(i+c,j+r))!=NULL)
    if (tmp->iNodeCount) //o ile s¹ jakieœ obiekty, bo po co puste sektory przelatywaæ
     pRendered[iRendered++]=tmp; //tworzenie listy sektorów do renderowania
  }
  while ((i<0)||(j<0)); //s¹ 4 przypadki, oprócz i=j=0
 }
 for (i=0;i<iRendered;i++)
  pRendered[i]->RenderDL(); //renderowanie nieprzezroczystych
 return true;
}

bool __fastcall TGround::RenderAlphaDL(vector3 pPosition)
{//renderowanie scenerii z Display List - faza przezroczystych
 TGroundNode *node;
 glColor4f(1.0f,1.0f,1.0f,1.0f);
 TSubRect *tmp;
 //Ra: renderowanie progresywne - zale¿ne od FPS oraz kierunku patrzenia
 int i;
 for (i=0;i<iRendered;i++)
 {//przezroczyste trójk¹ty w oddzielnym cyklu przed modelami
  tmp=pRendered[i];
  for (node=tmp->nRenderRectAlpha;node;node=node->nNext3)
   node->RenderAlphaDL(); //przezroczyste modele
 }
 for (i=0;i<iRendered;i++)
 {//renderowanie przezroczystych modeli oraz pojazdów
  pRendered[i]->RenderAlphaDL();
 }
 glDisable(GL_LIGHTING); //linie nie powinny œwieciæ
 for (i=0;i<iRendered;i++)
 {//druty na koñcu, ¿eby siê nie robi³y bia³e plamy na tle lasu
  tmp=pRendered[i];
  for (node=tmp->nRenderWires;node;node=node->nNext3)
   node->RenderAlphaDL(); //druty
 }
 return true;
}

bool __fastcall TGround::RenderVBO(vector3 pPosition)
{//renderowanie scenerii z VBO - faza nieprzezroczystych
 ++TGroundRect::iFrameNumber; //zwiêszenie licznika ramek
 CameraDirection.x=sin(Global::pCameraRotation); //wektor kierunkowy
 CameraDirection.z=cos(Global::pCameraRotation);
 int tr,tc;
 TGroundNode *node;
 glColor3f(1.0f,1.0f,1.0f);
 glEnable(GL_LIGHTING);
 int n=2*iNumSubRects; //(2*==2km) promieñ wyœwietlanej mapy w sektorach
 int c=GetColFromX(pPosition.x);
 int r=GetRowFromZ(pPosition.z);
 TSubRect *tmp;
 for (node=srGlobal.nRenderHidden;node;node=node->nNext3)
  node->RenderHidden(); //rednerowanie globalnych (nie za czêsto?)
 int i,j,k;
 //renderowanie czo³gowe dla obiektów aktywnych a niewidocznych
 for (j=r-n;j<=r+n;j++)
  for (i=c-n;i<=c+n;i++)
  {
   if ((tmp=FastGetSubRect(i,j))!=NULL)
    for (node=tmp->nRenderHidden;node;node=node->nNext3)
     node->RenderHidden();
  }
 //renderowanie progresywne - zale¿ne od FPS oraz kierunku patrzenia
 iRendered=0; //iloœæ renderowanych sektorów
 vector3 direction;
 //iRange=(Global::iSlowMotion&6)?((Global::iSlowMotion&4)?AreaMini:AreaSlow):AreaFast;
 //n=(iRange[0]*n)/10; //przeliczenie (n) do aktualnego promienia rednerowania
 //n=(Global::iSlowMotion&6)?((Global::iSlowMotion&4)?25:52):90; //iloœæ sektorów w æwiartce
 for (k=0;k<Global::iSegmentsRendered;++k) //sektory w kolejnoœci odleg³oœci
 {//przerobione na u¿ycie SectorOrder
  i=SectorOrder[k].x; //na starcie oba >=0
  j=SectorOrder[k].y;
  do
 //for (j=-n;j<=n;j++)
 //{k=iRange[j<0?-j:j]; //zasiêg na danym poziomie
 // for (i=-k;i<=k;i++)
  {
   if (j<=0) i=-i; //pierwszy przebieg: j<=0, i>=0; drugi: j>=0, i<=0; trzeci: j<=0, i<=0 czwarty: j>=0, i>=0;
   j=-j; //i oraz j musi byæ zmienione wczeœniej, ¿eby continue dzia³a³o
   direction=vector3(i,0,j); //wektor od kamery do danego sektora
   if (LengthSquared3(direction)>5) //te blisko s¹ zawsze wyœwietlane
   {direction=SafeNormalize(direction); //normalizacja
    if (CameraDirection.x*direction.x+CameraDirection.z*direction.z<0.55)
     continue; //pomijanie sektorów poza k¹tem patrzenia
   }
   Rects[(i+c)/iNumSubRects][(j+r)/iNumSubRects].RenderVBO(); //kwadrat kilometrowy nie zawsze, bo szkoda FPS
   if ((tmp=FastGetSubRect(i+c,j+r))!=NULL)
    if (tmp->iNodeCount) //je¿eli s¹ jakieœ obiekty, bo po co puste sektory przelatywaæ
     pRendered[iRendered++]=tmp; //tworzenie listy sektorów do renderowania
  }
  while ((i<0)||(j<0)); //s¹ 4 przypadki, oprócz i=j=0
 }
 //dodaæ rednerowanie terenu z E3D - jedno VBO jest u¿ywane dla ca³ego modelu, chyba ¿e jest ich wiêcej
 if (Global::pTerrainCompact)
  Global::pTerrainCompact->TerrainRenderVBO(TGroundRect::iFrameNumber);
 for (i=0;i<iRendered;i++)
 {//renderowanie nieprzezroczystych
  pRendered[i]->RenderVBO();
 }
 return true;
}

bool __fastcall TGround::RenderAlphaVBO(vector3 pPosition)
{//renderowanie scenerii z VBO - faza przezroczystych
 TGroundNode *node;
 glColor4f(1.0f,1.0f,1.0f,1.0f);
 TSubRect *tmp;
 int i;
 for (i=0;i<iRendered;i++)
 {//renderowanie przezroczystych trójk¹tów sektora
  tmp=pRendered[i];
   tmp->LoadNodes(); //ewentualne tworzenie siatek
   if (tmp->StartVBO())
   {for (node=tmp->nRenderRectAlpha;node;node=node->nNext3)
     if (node->iVboPtr>=0)
      node->RenderAlphaVBO(); //nieprzezroczyste obiekty terenu
    tmp->EndVBO();
   }
 }
 for (i=0;i<iRendered;i++)
  pRendered[i]->RenderAlphaVBO(); //przezroczyste modeli oraz pojazdy
 glDisable(GL_LIGHTING); //linie nie powinny œwieciæ
 for (i=0;i<iRendered;i++)
 {//druty na koñcu, ¿eby siê nie robi³y bia³e plamy na tle lasu
  tmp=pRendered[i];
  if (tmp->StartVBO())
  {for (node=tmp->nRenderWires;node;node=node->nNext3)
    node->RenderAlphaVBO(); //przezroczyste modele
   tmp->EndVBO();
  }
 }
 return true;
};

//---------------------------------------------------------------------------
void __fastcall TGround::Navigate(String ClassName,UINT Msg,WPARAM wParam,LPARAM lParam)
{//wys³anie komunikatu do steruj¹cego
 HWND h=FindWindow(ClassName.c_str(),0); //mo¿na by to zapamiêtaæ
 SendMessage(h,Msg,wParam,lParam);
};
//--------------------------------
void __fastcall TGround::WyslijEvent(const AnsiString &e,const AnsiString &d)
{//Ra: jeszcze do wyczyszczenia
 DaneRozkaz r;
 r.iSygn='EU07';
 r.iComm=2; //2 - event
 int i=e.Length(),j=d.Length();
 r.cString[0]=char(i);
 strcpy(r.cString+1,e.c_str()); //zakoñczony zerem
 r.cString[i+2]=char(j); //licznik po zerze koñcz¹cym
 strcpy(r.cString+3+i,d.c_str()); //zakoñczony zerem
 COPYDATASTRUCT cData;
 cData.dwData='EU07'; //sygnatura
 cData.cbData=12+i+j; //8+dwa liczniki i dwa zera koñcz¹ce
 cData.lpData=&r;
 Navigate("TEU07SRK",WM_COPYDATA,(WPARAM)Global::hWnd,(LPARAM)&cData);
};
//---------------------------------------------------------------------------
void __fastcall TGround::WyslijString(const AnsiString &t,int n)
{//wys³anie informacji w postaci pojedynczego tekstu
 DaneRozkaz r;
 r.iSygn='EU07';
 r.iComm=n; //numer komunikatu
 int i=t.Length();
 r.cString[0]=char(i);
 strcpy(r.cString+1,t.c_str()); //z zerem koñcz¹cym
 COPYDATASTRUCT cData;
 cData.dwData='EU07'; //sygnatura
 cData.cbData=10+i; //8+licznik i zero koñcz¹ce
 cData.lpData=&r;
 Navigate("TEU07SRK",WM_COPYDATA,(WPARAM)Global::hWnd,(LPARAM)&cData);
};
//---------------------------------------------------------------------------
void __fastcall TGround::WyslijWolny(const AnsiString &t)
{//Ra: jeszcze do wyczyszczenia
 WyslijString(t,4); //tor wolny
};
//--------------------------------
void __fastcall TGround::WyslijNamiary(TGroundNode* t)
{//wys³anie informacji o pojeŸdzie - (float), d³ugoœæ ramki bêdzie zwiêkszana w miarê potrzeby
 DaneRozkaz r;
 r.iSygn='EU07';
 r.iComm=7; //7 - dane pojazdu
 int i=9,j=t->asName.Length();
 r.iPar[ 0]=i; //iloœæ danych liczbowych
 r.fPar[ 1]=Global::fTimeAngleDeg/360.0; //aktualny czas (1.0=doba)
 r.fPar[ 2]=t->DynamicObject->MoverParameters->Loc.X; //pozycja X
 r.fPar[ 3]=t->DynamicObject->MoverParameters->Loc.Y; //pozycja Y
 r.fPar[ 4]=t->DynamicObject->MoverParameters->Loc.Z; //pozycja Z
 r.fPar[ 5]=t->DynamicObject->MoverParameters->V; //prêdkoœæ ruchu X
 r.fPar[ 6]=0; //prêdkoœæ ruchu Y
 r.fPar[ 7]=0; //prêdkoœæ ruchu Z
 r.fPar[ 8]=t->DynamicObject->MoverParameters->AccV; //przyspieszenie X
 //r.fPar[ 9]=0; //przyspieszenie Y //na razie nie
 //r.fPar[10]=0; //przyspieszenie Z
 i<<=2; //iloœæ bajtów
 r.cString[i]=char(j); //na koñcu nazwa, ¿eby jakoœ zidentyfikowaæ
 strcpy(r.cString+i+1,t->asName.c_str()); //zakoñczony zerem
 COPYDATASTRUCT cData;
 cData.dwData='EU07'; //sygnatura
 cData.cbData=10+i+j; //8+licznik i zero koñcz¹ce
 cData.lpData=&r;
 Navigate("TEU07SRK",WM_COPYDATA,(WPARAM)Global::hWnd,(LPARAM)&cData);
};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void __fastcall TGround::RadioStop(vector3 pPosition)
{//zatrzymanie poci¹gów w okolicy
 TGroundNode *node;
 TSubRect *tmp;
 int c=GetColFromX(pPosition.x);
 int r=GetRowFromZ(pPosition.z);
 int i,j;
 int n=2*iNumSubRects; //przegl¹danie czo³gowe okolicznych torów w kwadracie 4km×4km
 for (j=r-n;j<=r+n;j++)
  for (i=c-n;i<=c+n;i++)
   if ((tmp=FastGetSubRect(i,j))!=NULL)
    for (node=tmp->nRootNode;node!=NULL;node=node->nNext2)
     if (node->iType==TP_TRACK)
      node->pTrack->RadioStop(); //przekazanie do ka¿dego toru w ka¿dym segmencie
};

TDynamicObject* __fastcall TGround::DynamicNearest(vector3 pPosition,double distance,bool mech)
{//wyszukanie pojazdu najbli¿szego wzglêdem (pPosition)
 TGroundNode *node;
 TSubRect *tmp;
 TDynamicObject *dyn=NULL;
 int c=GetColFromX(pPosition.x);
 int r=GetRowFromZ(pPosition.z);
 int i,j,k;
 double sqm=distance*distance,sqd; //maksymalny promien poszukiwañ do kwadratu
 for (j=r-1;j<=r+1;j++) //plus dwa zewnêtrzne sektory, ³¹cznie 9
  for (i=c-1;i<=c+1;i++)
   if ((tmp=FastGetSubRect(i,j))!=NULL)
    for (node=tmp->nRootNode;node;node=node->nNext2) //nastêpny z sektora
     if (node->iType==TP_TRACK)
      for (k=0;k<node->pTrack->iNumDynamics;k++)
       if ((sqd=SquareMagnitude(node->pTrack->Dynamics[k]->GetPosition()-pPosition))<sqm)
        if (mech?(node->pTrack->Dynamics[k]->Mechanik!=NULL):true) //czy ma mieæ obsadê
        {
         sqm=sqd; //nowa odleg³oœæ
         dyn=node->pTrack->Dynamics[k]; //nowy lider
        }
 return dyn;
};
//---------------------------------------------------------------------------
void __fastcall TGround::DynamicRemove(TDynamicObject* dyn)
{//Ra: usuniêcie pojazdów ze scenerii (gdy dojad¹ na koniec i nie sa potrzebne)
 TDynamicObject* d=dyn->Prev();
 if (d) //jeœli coœ jest z przodu
  DynamicRemove(d); //zaczynamy od tego z przodu
 else
 {//jeœli mamy ju¿ tego na pocz¹tku
  TGroundNode **n,*node;
  d=dyn; //od pierwszego
  while (d)
  {if (d->MyTrack) d->MyTrack->RemoveDynamicObject(d); //usuniêcie z toru o ile nie usuniêty
   n=&nRootDynamic; //lista pojazdów od pocz¹tku
   //node=NULL; //nie znalezione
   while (*n?(*n)->DynamicObject!=d:false)
   {//usuwanie z listy pojazdów
    n=&((*n)->Next); //sprawdzenie kolejnego pojazdu na liœcie
   }
   if ((*n)->DynamicObject==d)
   {//jeœli znaleziony
    node=(*n); //zapamiêtanie wêz³a, aby go usun¹æ
    (*n)=node->Next; //pominiêcie na liœcie
    Global::TrainDelete(d);
    d=d->Next(); //przejœcie do kolejnego pojazdu, póki jeszcze jest
    delete node; //usuwanie fizyczne z pamiêci
   }
   else
    d=NULL; //coœ nie tak!
  }
 }
};

//---------------------------------------------------------------------------
void __fastcall TGround::TerrainRead(const AnsiString &f)
{//Ra: wczytanie trójk¹tów terenu z pliku E3D
};

//---------------------------------------------------------------------------
void __fastcall TGround::TerrainWrite()
{//Ra: zapisywanie trójk¹tów terenu do pliku E3D
 if (Global::pTerrainCompact->TerrainLoaded())
  return; //jeœli zosta³o wczytane, to nie ma co dalej robiæ
 if (Global::asTerrainModel.IsEmpty()) return;
 //Trójk¹ty s¹ zapisywane kwadratami kilometrowymi.
 //Kwadrat 500500 jest na œrodku (od 0.0 do 1000.0 na OX oraz OZ).
 //Ewentualnie w numerowaniu kwadratów uwzglêdnic wpis //$g.
 //Trójk¹ty s¹ grupowane w submodele wg tekstury.
 //Triangle_strip oraz triangle_fan s¹ rozpisywane na pojedyncze trójk¹ty,
 // chyba ¿e dla danej tekstury wychodzi tylko jeden submodel.
 TModel3d *m=new TModel3d(); //wirtualny model roboczy z oddzielnymi submodelami
 TSubModel *sk; //wskaŸnik roboczy na submodel kwadratu
 TSubModel *st; //wskaŸnik roboczy na submodel tekstury
 //Zliczamy kwadraty z trójk¹tami, iloœæ tekstur oraz wierzcho³ków.
 //Iloœæ kwadratów i iloœæ tekstur okreœli iloœæ submodeli.
 //int sub=0; //ca³kowita iloœæ submodeli
 //int ver=0; //ca³kowita iloœæ wierzcho³ków
 int i,j,k; //indeksy w pêtli
 TGroundNode *Current;
 float8 *ver; //trójk¹ty
 TSubModel::iInstance=0; //pozycja w tabeli wierzcho³ków liczona narastaj¹co
 for (i=0;i<iNumRects;++i) //pêtla po wszystkich kwadratach kilometrowych
  for (j=0;j<iNumRects;++j)
   if (Rects[i][j].iNodeCount)
   {//o ile s¹ jakieœ trójk¹ty w œrodku
    sk=new TSubModel(); //nowy submodel dla kawadratu
    //numer kwadratu XXXZZZ, przy czym X jest ujemne - XXX roœnie na wschód, ZZZ roœnie na pó³noc
    sk->NameSet(AnsiString(1000*(500+i-iNumRects/2)+(500+j-iNumRects/2)).c_str()); //nazwa=numer kwadratu
    m->AddTo(NULL,sk); //dodanie submodelu dla kwadratu
    for (Current=Rects[i][j].nRootNode;Current;Current=Current->nNext2)
     switch (Current->iType)
     {//pêtla po trójk¹tach - zliczanie wierzcho³ków, dodaje submodel dla ka¿dej tekstury
      case GL_TRIANGLES:
       Current->iVboPtr=sk->TriangleAdd(m,Current->TextureID,Current->iNumVerts); //zwiêkszenie iloœci trójk¹tów w submodelu
       m->iNumVerts+=Current->iNumVerts; //zwiêkszenie ca³kowitej iloœci wierzcho³ków
       break;
      case GL_TRIANGLE_STRIP: //na razie nie, bo trzeba przerabiaæ na pojedyncze trójk¹ty
       break;
      case GL_TRIANGLE_FAN: //na razie nie, bo trzeba przerabiaæ na pojedyncze trójk¹ty
       break;
     }
    for (Current=Rects[i][j].nRootNode;Current;Current=Current->nNext2)
     switch (Current->iType)
     {//pêtla po trójk¹tach - dopisywanie wierzcho³ków
      case GL_TRIANGLES:
       //ver=sk->TrianglePtr(TTexturesManager::GetName(Current->TextureID).c_str(),Current->iNumVerts); //wskaŸnik na pocz¹tek
       ver=sk->TrianglePtr(Current->TextureID,Current->iVboPtr,Current->Ambient,Current->Diffuse,Current->Specular); //wskaŸnik na pocz¹tek
       Current->iVboPtr=-1; //bo to by³o tymczasowo u¿ywane
       for (k=0;k<Current->iNumVerts;++k)
       {//przepisanie wspó³rzêdnych
        ver[k].Point.x=Current->Vertices[k].Point.x;
        ver[k].Point.y=Current->Vertices[k].Point.y;
        ver[k].Point.z=Current->Vertices[k].Point.z;
        ver[k].Normal.x=Current->Vertices[k].Normal.x;
        ver[k].Normal.y=Current->Vertices[k].Normal.y;
        ver[k].Normal.z=Current->Vertices[k].Normal.z;
        ver[k].tu=Current->Vertices[k].tu;
        ver[k].tv=Current->Vertices[k].tv;
       }
       break;
      case GL_TRIANGLE_STRIP: //na razie nie, bo trzeba przerabiaæ na pojedyncze trójk¹ty
       break;
      case GL_TRIANGLE_FAN: //na razie nie, bo trzeba przerabiaæ na pojedyncze trójk¹ty
       break;
     }
   }
 m->SaveToBinFile(AnsiString("models\\"+Global::asTerrainModel).c_str());
};
//---------------------------------------------------------------------------

