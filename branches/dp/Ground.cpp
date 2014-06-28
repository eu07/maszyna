//---------------------------------------------------------------------------

/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak and others

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
 nNext=nNext2=NULL;
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
   case TP_TRACTION:   SafeDelete(hvTraction); break;
   case TP_TRACTIONPOWERSOURCE:
                       SafeDelete(psTractionPowerSource); break;
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
   hvTraction->pPoint1+=pPosition;
   hvTraction->pPoint2+=pPosition;
   hvTraction->pPoint3+=pPosition;
   hvTraction->pPoint4+=pPosition;
   hvTraction->Optimize();
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
   tsStaticSound->vSoundPosition+=pPosition;
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
  //case TP_SOUND: //McZapkie - dzwiek zapetlony w zaleznosci od odleglosci
  // if ((pStaticSound->GetStatus()&DSBSTATUS_PLAYING)==DSBPLAY_LOOPING)
  // {
  //  pStaticSound->Play(1,DSBPLAY_LOOPING,true,pStaticSound->vSoundPosition);
  //  pStaticSound->AdjFreq(1.0,Timer::GetDeltaTime());
  // }
  // return; //Ra: TODO sprawdziæ, czy dŸwiêki nie s¹ tylko w RenderHidden
  case TP_MEMCELL: return;
  case TP_EVLAUNCH:
   if (EvLaunch->Render())
    if ((EvLaunch->dRadius<0)||(mgn<EvLaunch->dRadius))
    {
     if (Console::Pressed(VK_SHIFT) && EvLaunch->Event2!=NULL)
      Global::AddToQuery(EvLaunch->Event2,NULL);
     else
      if (EvLaunch->Event1!=NULL)
       Global::AddToQuery(EvLaunch->Event1,NULL);
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
    hvTraction->RenderVBO(mgn,iVboPtr);
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
   if ((tsStaticSound->GetStatus()&DSBSTATUS_PLAYING)==DSBPLAY_LOOPING)
   {
    tsStaticSound->Play(1,DSBPLAY_LOOPING,true,tsStaticSound->vSoundPosition);
    tsStaticSound->AdjFreq(1.0,Timer::GetDeltaTime());
   }
   return;
  case TP_EVLAUNCH:
   if (EvLaunch->Render())
    if ((EvLaunch->dRadius<0)||(mgn<EvLaunch->dRadius))
    {
     WriteLog("Eventlauncher "+asName);
     if (Console::Pressed(VK_SHIFT)&&(EvLaunch->Event2))
      Global::AddToQuery(EvLaunch->Event2,NULL);
     else
      if (EvLaunch->Event1)
       Global::AddToQuery(EvLaunch->Event1,NULL);
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
    hvTraction->RenderDL(mgn);
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

TTraction* __fastcall TSubRect::FindTraction(vector3 *Point,int &iConnection,TTraction *Exclude)
{//szukanie przês³a w sektorze, którego koniec jest najbli¿szy (*Point)
 TGroundNode *Current;
 for (Current=nRenderWires;Current;Current=Current->nNext3)
  if ((Current->iType==TP_TRACTION)&&(Current->hvTraction!=Exclude))
   {
    iConnection=Current->hvTraction->TestPoint(Point);
    if (iConnection>=0) return Current->hvTraction;
   }
 return NULL;
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
    n->iNumVerts=n->hvTraction->RaArrayPrepare(); //zliczenie wierzcho³ków
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
       n->hvTraction->RaArrayFill(m_pVNT+n->iVboPtr);
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

void __fastcall TSubRect::RenderSounds()
{//aktualizacja dŸwiêków w pojazdach sektora (sektor mo¿e nie byæ wyœwietlany)
 for (int j=0;j<iTracks;++j)
  tTracks[j]->RenderDynSounds(); //dŸwiêki pojazdów id¹ niezale¿nie od wyœwietlania
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
  Current=Current->evNext2;
  delete tmp;
 }
 TGroundNode *tmpn;
 for (int i=0;i<TP_LAST;++i)
 {for (TGroundNode *Current=nRootOfType[i];Current;)
  {
   tmpn=Current;
   Current=Current->nNext;
   delete tmpn;
  }
  nRootOfType[i]=NULL;
 }
 for (TGroundNode *Current=nRootDynamic;Current;)
 {
  tmpn=Current;
  Current=Current->nNext;
  delete tmpn;
 }
 iNumNodes=0;
 //RootNode=NULL;
 nRootDynamic=NULL;
 delete sTracks;
}

TGroundNode* __fastcall TGround::FindDynamic(AnsiString asNameToFind)
{//wyszukanie pojazdu o podanej nazwie, na razie tylko pojazdy z obsad¹ s¹ interesuj¹ce
 for (TGroundNode *Current=nRootDynamic;Current;Current=Current->nNext)
  if (Current->DynamicObject->Mechanik) //dostêp do pojazdów bez obsady nie jest na razie potrzebny
   if ((Current->asName==asNameToFind))
    return Current;
 return NULL;
};

TGroundNode* __fastcall TGround::FindGroundNode(AnsiString asNameToFind,TGroundNodeType iNodeType)
{//wyszukiwanie obiektu o podanej nazwie i konkretnym typie
 if ((iNodeType==TP_TRACK)||(iNodeType==TP_MEMCELL)||(iNodeType==TP_MODEL))
 {//wyszukiwanie w drzewie binarnym
  return (TGroundNode*)sTracks->Find(iNodeType,asNameToFind.c_str());
 }
 //standardowe wyszukiwanie liniowe
 TGroundNode *Current;
 for (Current=nRootOfType[iNodeType];Current;Current=Current->nNext)
  if (Current->asName==asNameToFind)
   return Current;
 return NULL;
}


double fTrainSetVel=0;
double fTrainSetDir=0;
double fTrainSetDist=0; //odleg³oœæ sk³adu od punktu 1 w stronê punktu 2
AnsiString asTrainSetTrack="";
int iTrainSetConnection=0;
bool bTrainSet=false;
AnsiString asTrainName="";
int iTrainSetWehicleNumber=0;
TGroundNode *nTrainSetNode=NULL; //poprzedni pojazd do ³¹czenia
TGroundNode *nTrainSetDriver=NULL; //pojazd, któremu zostanie wys³any rozk³ad

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
 ntri->nNext=nRootOfType[GL_TRIANGLES];
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
 TTextSound *tmpsound;
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
 //else if (str=="terrain")             tmp->iType=TP_TERRAIN; //tymczasowo do odwo³ania
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
   tmp->hvTraction=new TTraction();
   parser->getTokens();
   *parser >> token;
   tmp->hvTraction->asPowerSupplyName=AnsiString(token.c_str()); //nazwa zasilacza
   parser->getTokens(3);
   *parser >> tmp->hvTraction->NominalVoltage >> tmp->hvTraction->MaxCurrent >> tmp->hvTraction->fResistivity;
   if (tmp->hvTraction->fResistivity==0.01) //tyle jest w sceneriach [om/km]
    tmp->hvTraction->fResistivity=0.075; //taka sensowniejsza wartoœæ za http://www.ikolej.pl/fileadmin/user_upload/Seminaria_IK/13_05_07_Prezentacja_Kruczek.pdf
   tmp->hvTraction->fResistivity*=0.001; //teraz [om/m]
   parser->getTokens();
   *parser >> token;
   //Ra 2014-02: a tutaj damy symbol sieci i jej budowê, np.:
   // SKB70-C, CuCd70-2C, KB95-2C, C95-C, C95-2C, YC95-2C, YpC95-2C, YC120-2C
   // YpC120-2C, YzC120-2C, YwsC120-2C, YC150-C150, YC150-2C150, C150-C150
   // C120-2C, 2C120-2C, 2C120-2C-1, 2C120-2C-2, 2C120-2C-3, 2C120-2C-4
   if (token.compare("none")==0)
    tmp->hvTraction->Material=0;
   else if (token.compare("al")==0)
    tmp->hvTraction->Material=2; //1=aluminiowa, rysuje siê na czarno
   else
    tmp->hvTraction->Material=1; //1=miedziana, rysuje siê na zielono albo czerwono
   parser->getTokens();
   *parser >> tmp->hvTraction->WireThickness;
   parser->getTokens();
   *parser >> tmp->hvTraction->DamageFlag;
   parser->getTokens(3);
   *parser >> tmp->hvTraction->pPoint1.x >> tmp->hvTraction->pPoint1.y >> tmp->hvTraction->pPoint1.z;
   tmp->hvTraction->pPoint1+=pOrigin;
   parser->getTokens(3);
   *parser >> tmp->hvTraction->pPoint2.x >> tmp->hvTraction->pPoint2.y >> tmp->hvTraction->pPoint2.z;
   tmp->hvTraction->pPoint2+=pOrigin;
   parser->getTokens(3);
   *parser >> tmp->hvTraction->pPoint3.x >> tmp->hvTraction->pPoint3.y >> tmp->hvTraction->pPoint3.z;
   tmp->hvTraction->pPoint3+=pOrigin;
   parser->getTokens(3);
   *parser >> tmp->hvTraction->pPoint4.x >> tmp->hvTraction->pPoint4.y >> tmp->hvTraction->pPoint4.z;
   tmp->hvTraction->pPoint4+=pOrigin;
   parser->getTokens();
   *parser >> tf1;
   tmp->hvTraction->fHeightDifference=
    (tmp->hvTraction->pPoint3.y-tmp->hvTraction->pPoint1.y+
     tmp->hvTraction->pPoint4.y-tmp->hvTraction->pPoint2.y)*0.5f-tf1;
   parser->getTokens();
   *parser >> tf1;
   if (tf1>0)
    tmp->hvTraction->iNumSections=(tmp->hvTraction->pPoint1-tmp->hvTraction->pPoint2).Length()/tf1;
   else tmp->hvTraction->iNumSections=0;
   parser->getTokens();
   *parser >> tmp->hvTraction->Wires;
   parser->getTokens();
   *parser >> tmp->hvTraction->WireOffset;
   parser->getTokens();
   *parser >> token;
   tmp->bVisible=(token.compare("vis")==0);
   parser->getTokens();
   *parser >> token;
   if (token.compare("parallel")==0)
   {//jawne wskazanie innego przês³a, na które mo¿e przestawiæ siê pantograf
    parser->getTokens();
    *parser >> token; //wypada³o by to zapamiêtaæ...
    tmp->hvTraction->asParallel=AnsiString(token.c_str());
    parser->getTokens();
    *parser >> token; //a tu ju¿ powinien byæ koniec
   }
   if (token.compare("endtraction")!=0)
    Error("ENDTRACTION delimiter missing! "+str2+" found instead.");
   tmp->hvTraction->Init(); //przeliczenie parametrów
   //if (Global::bLoadTraction)
   // tmp->hvTraction->Optimize(); //generowanie DL dla wszystkiego przy wczytywaniu?
   tmp->pCenter=(tmp->hvTraction->pPoint2+tmp->hvTraction->pPoint1)*0.5f;
   //if (!Global::bLoadTraction) SafeDelete(tmp); //Ra: tak byæ nie mo¿e, bo NULL to b³¹d
   break;
  case TP_TRACTIONPOWERSOURCE:
   parser->getTokens(3);
   *parser >> tmp->pCenter.x >> tmp->pCenter.y >> tmp->pCenter.z;
   tmp->pCenter+=pOrigin;
   tmp->psTractionPowerSource=new TTractionPowerSource();
   tmp->psTractionPowerSource->Load(parser);
   break;
  case TP_MEMCELL:
   parser->getTokens(3);
   *parser >> tmp->pCenter.x >> tmp->pCenter.y >> tmp->pCenter.z;
   tmp->pCenter+=pOrigin;
   tmp->MemCell=new TMemCell(&tmp->pCenter);
   tmp->MemCell->Load(parser);
   if (!tmp->asName.IsEmpty()) //jest pusta gdy "none"
   {//dodanie do wyszukiwarki
    if (sTracks->Update(TP_MEMCELL,tmp->asName.c_str(),tmp)) //najpierw sprawdziæ, czy ju¿ jest
    {//przy zdublowaniu wskaŸnik zostanie podmieniony w drzewku na póŸniejszy (zgodnoœæ wsteczna)
     ErrorLog("Duplicated memcell: "+tmp->asName); //to zg³aszaæ duplikat
    }
    else
     sTracks->Add(TP_MEMCELL,tmp->asName.c_str(),tmp); //nazwa jest unikalna
   }
   break;
  case TP_EVLAUNCH:
   parser->getTokens(3);
   *parser >> tmp->pCenter.x >> tmp->pCenter.y >> tmp->pCenter.z;
   tmp->pCenter+=pOrigin;
   tmp->EvLaunch=new TEventLauncher();
   tmp->EvLaunch->Load(parser);
   break;
  case TP_TRACK :
   tmp->pTrack=new TTrack(tmp);
   if (Global::iWriteLogEnabled&4)
    if (!tmp->asName.IsEmpty())
     WriteLog(tmp->asName.c_str());
   tmp->pTrack->Load(parser,pOrigin,tmp->asName); //w nazwie mo¿e byæ nazwa odcinka izolowanego
   if (!tmp->asName.IsEmpty()) //jest pusta gdy "none"
   {//dodanie do wyszukiwarki
    if (sTracks->Update(TP_TRACK,tmp->asName.c_str(),tmp)) //najpierw sprawdziæ, czy ju¿ jest
    {//przy zdublowaniu wskaŸnik zostanie podmieniony w drzewku na póŸniejszy (zgodnoœæ wsteczna)
     if (tmp->pTrack->iCategoryFlag&1) //jeœli jest zdublowany tor kolejowy
      ErrorLog("Duplicated track: "+tmp->asName); //to zg³aszaæ duplikat
    }
    else
     sTracks->Add(TP_TRACK,tmp->asName.c_str(),tmp); //nazwa jest unikalna
   }
   tmp->pCenter=(tmp->pTrack->CurrentSegment()->FastGetPoint_0()+
                 tmp->pTrack->CurrentSegment()->FastGetPoint(0.5)+
                 tmp->pTrack->CurrentSegment()->FastGetPoint_1() )/3.0;
   break;
  case TP_SOUND :
   tmp->tsStaticSound=new TTextSound;
   parser->getTokens(3);
   *parser >> tmp->pCenter.x >> tmp->pCenter.y >> tmp->pCenter.z;
   tmp->pCenter+=pOrigin;
   parser->getTokens();
   *parser >> token;
   str=AnsiString(token.c_str());
   tmp->tsStaticSound->Init(str.c_str(),sqrt(tmp->fSquareRadius),tmp->pCenter.x,tmp->pCenter.y,tmp->pCenter.z,false);

//            tmp->pDirectSoundBuffer=TSoundsManager::GetFromName(str.c_str());
//            tmp->iState=(Parser->GetNextSymbol().LowerCase()=="loop"?DSBPLAY_LOOPING:0);
   parser->getTokens(); *parser >> token;
   break;
  case TP_DYNAMIC:
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
      int dlugosc= str4.Length();
      int1=str4.SubString(1, int2-1).ToInt(); //niech sprzegiem bedzie do kropki cos
      str4=str4.SubString(int2+1, dlugosc-int2);
    }
    else
    {
     int1=str4.ToInt();
     str4="";
    }
    int2=0; //zeruje po wykorzystaniu
//    *parser >> int1; //yB: nastawy i takie tam TUTAJ!!!!!
    if (int1<0) int1=(-int1)|ctrain_depot; //sprzêg zablokowany (pojazdy nieroz³¹czalne przy manewrach)
    if (tf1!=-1.0)
     if (fabs(tf1)>0.5) //maksymalna odleg³oœæ miêdzy sprzêgami - do przemyœlenia
      int1=0; //likwidacja sprzêgu, jeœli odleg³oœæ zbyt du¿a - to powinno byæ uwzglêdniane w fizyce sprzêgów...
    TempConnectionType[iTrainSetWehicleNumber]=int1; //wartoœæ dodatnia
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
    *parser >> tf3; //prêdkoœæ, niektórzy wpisuj¹ tu "3" jako sprzêg, ¿eby nie by³o tabliczki
    iTrainSetWehicleNumber=0;
    TempConnectionType[iTrainSetWehicleNumber]=3; //likwidacja tabliczki na koñcu?
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
   if (tmp1?tmp1->pTrack!=NULL:false)
   {//jeœli tor znaleziony
    Track=tmp1->pTrack;
    if (!iTrainSetWehicleNumber) //jeœli pierwszy pojazd
     if (Track->evEvent0) //jeœli tor ma Event0
      if (fabs(fTrainSetVel)<=1.0) //a sk³ad stoi
       if (fTrainSetDist>=0.0) //ale mo¿e nie siêgaæ na owy tor
        if (fTrainSetDist<8.0) //i raczej nie siêga
         fTrainSetDist=8.0; //przesuwamy oko³o pó³ EU07 dla wstecznej zgodnoœci
    //WriteLog("Dynamic shift: "+AnsiString(fTrainSetDist));
/* //Ra: to jednak robi du¿e problemy - przesuniêcie w dynamic jest przesuniêciem do ty³u, odwrotnie ni¿ w trainset
    if (!iTrainSetWehicleNumber) //dla pierwszego jest to przesuniêcie (ujemne = do ty³u)
     if (tf1!=-1.0) //-1 wyj¹tkowo oznacza odwrócenie
      tf1=-tf1; //a dla kolejnych odleg³oœæ miêdzy sprzêgami (ujemne = wbite)
*/
    tf3=tmp->DynamicObject->Init(asNodeName,str1,Skin,str3,Track,(tf1==-1.0?fTrainSetDist:fTrainSetDist-tf1),DriverType,tf3,asTrainName,int2,str2,(tf1==-1.0),str4);
    if (tf3!=0.0) //zero oznacza b³¹d
    {fTrainSetDist-=tf3; //przesuniêcie dla kolejnego, minus bo idziemy w stronê punktu 1
     tmp->pCenter=tmp->DynamicObject->GetPosition();
     if (TempConnectionType[iTrainSetWehicleNumber]) //jeœli jest sprzêg
      if (tmp->DynamicObject->MoverParameters->Couplers[tf1==-1.0?0:1].AllowedFlag&ctrain_depot) //jesli zablokowany
       TempConnectionType[iTrainSetWehicleNumber]|=ctrain_depot; //bêdzie blokada
     iTrainSetWehicleNumber++;
    }
    else
    {//LastNode=NULL;
     delete tmp;
     tmp=NULL; //nie mo¿e byæ tu return, bo trzeba pomin¹æ jeszcze enddynamic
    }
   }
   else
   {//gdy tor nie znaleziony
    ErrorLog("Missed track: dynamic placed on \""+tmp->DynamicObject->asTrack+"\"");
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
  //case TP_TERRAIN: //TODO: zrobiæ jak zwyk³y, rozró¿nienie po nazwie albo czymœ innym
  case TP_MODEL:
   if (rmin<0)
   {tmp->iType=TP_TERRAIN;
    tmp->fSquareMinRadius=0; //to w ogóle potrzebne?
   }
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
   else if (!tmp->asName.IsEmpty()) //jest pusta gdy "none"
   {//dodanie do wyszukiwarki
    if (sTracks->Update(TP_MODEL,tmp->asName.c_str(),tmp)) //najpierw sprawdziæ, czy ju¿ jest
    {//przy zdublowaniu wskaŸnik zostanie podmieniony w drzewku na póŸniejszy (zgodnoœæ wsteczna)
     ErrorLog("Duplicated model: "+tmp->asName); //to zg³aszaæ duplikat
    }
    else
     sTracks->Add(TP_MODEL,tmp->asName.c_str(),tmp); //nazwa jest unikalna
   }
   //str=Parser->GetNextSymbol().LowerCase();
   break;
  //case TP_GEOMETRY :
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
   tmp->TextureID=TTexturesManager::GetTextureID(szTexturePath,szSceneryPath,str.c_str());
   tmp->iFlags=TTexturesManager::GetAlpha(tmp->TextureID)?0x220:0x210; //z usuwaniem
   if (((tmp->iType==GL_TRIANGLES)&&(tmp->iFlags&0x10))?Global::pTerrainCompact->TerrainLoaded():false)
   {//jeœli jest tekstura nieprzezroczysta, a teren za³adowany, to pomijamy trójk¹ty
    do
    {//pomijanie trójk¹tów
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
 return (TEvent*)sTracks->Find(0,asEventName.c_str()); //wyszukiwanie w drzewie
/* //powolna wyszukiwarka
 for (TEvent *Current=RootEvent;Current;Current=Current->Next2)
 {
  if (Current->asName==asEventName)
   return Current;
 }
 return NULL;
*/
}

TEvent* __fastcall TGround::FindEventScan(const AnsiString &asEventName)
{//wyszukanie eventu z opcj¹ utworzenia niejawnego dla komórek skanowanych
 TEvent *e=(TEvent*)sTracks->Find(0,asEventName.c_str()); //wyszukiwanie w drzewie eventów
 if (e) return e; //jak istnieje, to w porz¹dku
 if (asEventName.SubString(asEventName.Length()-4,5)==":scan") //jeszcze mo¿e byæ event niejawny
 {//no to szukamy komórki pamiêci o nazwie zawartej w evencie
  AnsiString n=asEventName.SubString(1,asEventName.Length()-5); //do dwukropka
  if (sTracks->Find(TP_MEMCELL,n.c_str())) //jeœli jest takowa komórka pamiêci
   e=new TEvent(n); //utworzenie niejawnego eventu jej odczytu
 }
 return e; //utworzony albo siê nie uda³o  
}

void __fastcall TGround::FirstInit()
{//ustalanie zale¿noœci na scenerii przed wczytaniem pojazdów
 if (bInitDone) return; //Ra: ¿eby nie robi³o siê dwa razy
 bInitDone=true;
 WriteLog("InitNormals");
 int i,j;
 for (i=0;i<TP_LAST;++i)
 {for (TGroundNode *Current=nRootOfType[i];Current;Current=Current->nNext)
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
 WriteLog("InitTraction");
 InitTraction(); //³¹czenie drutów ze sob¹
 WriteLog("InitTraction OK");
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
 if (Global::fMoveLight>=0.0) //albo tak, albo niech ustala minimum ciemnoœci w nocy
 {
  Global::fLuminance= //obliczenie luminacji "œwiat³a w ciemnoœci"
   +0.150*Global::ambientDayLight[0]  //R
   +0.295*Global::ambientDayLight[1]  //G
   +0.055*Global::ambientDayLight[2]; //B
  if (Global::fLuminance>0.1) //jeœli mia³o by byæ za jasno
   for (int i=0;i<3;i++)
    Global::ambientDayLight[i]*=0.1/Global::fLuminance; //ograniczenie jasnoœci w nocy
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT,Global::ambientDayLight);
 }
 else if (Global::bDoubleAmbient) //Ra: wczeœniej by³o ambient dawane na obydwa œwiat³a
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
         LastNode->nNext=nRootOfType[LastNode->iType]; //ostatni dodany do³¹czamy na koñcu nowego
         nRootOfType[LastNode->iType]=LastNode; //ustawienie nowego na pocz¹tku listy
         iNumNodes++;
        }
        else
        {//jeœli jest pojazdem
         //if (!bInitDone) FirstInit(); //jeœli nie by³o w scenerii
         if (LastNode->DynamicObject->Mechanik) //ale mo¿e byæ pasa¿er
          if (LastNode->DynamicObject->Mechanik->Primary()) //jeœli jest g³ównym (pasa¿er nie jest)
           nTrainSetDriver=LastNode; //pojazd, któremu zostanie wys³any rozk³ad
         LastNode->nNext=nRootDynamic;
         nRootDynamic=LastNode; //dopisanie z przodu do listy
         //if (bTrainSet && (LastNode?(LastNode->iType==TP_DYNAMIC):false))
         if (nTrainSetNode) //je¿eli istnieje wczeœniejszy TP_DYNAMIC
          nTrainSetNode->DynamicObject->AttachPrev(LastNode->DynamicObject,TempConnectionType[iTrainSetWehicleNumber-2]);
         nTrainSetNode=LastNode; //ostatnio wczytany
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
      nTrainSetNode=NULL;
      nTrainSetDriver=NULL; //pojazd, któremu zostanie wys³any rozk³ad
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
      if (nTrainSetNode) //trainset bez dynamic siê sypa³
      {//powinien te¿ tu wchodziæ, gdy pojazd bez trainset
       if (nTrainSetDriver) //pojazd, któremu zostanie wys³any rozk³ad
       {
        nTrainSetDriver->DynamicObject->Mechanik->DirectionInitial();
        nTrainSetDriver->DynamicObject->Mechanik->PutCommand("Timetable:"+asTrainName,fTrainSetVel,0,NULL);
       }
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
        if (iTrainSetWehicleNumber?!TempConnectionType[iTrainSetWehicleNumber-1]:false) //jeœli ostatni pojazd ma sprzêg 0
         LastNode->DynamicObject->RaLightsSet(-1,2+32+64); //to za³o¿ymy mu koñcówki blaszane (jak AI siê odpali, to sobie poprawi)
      bTrainSet=false;
      fTrainSetVel=0;
      //iTrainSetConnection=0;
      nTrainSetNode=NULL;
      iTrainSetWehicleNumber=0;
     }
     else if (str==AnsiString("event"))
     {
      TEvent *tmp=new TEvent();
      tmp->Load(&parser,&pOrigin);
      if (tmp->Type==tp_Unknown)
       delete tmp;
      else
      {//najpierw sprawdzamy, czy nie ma, a potem dopisujemy
       TEvent *found=FindEvent(tmp->asName);
       if (found)
       {//jeœli znaleziony duplikat
        int i=tmp->asName.Length();
        if (tmp->asName[1]=='#') //zawsze jeden znak co najmniej jest
        {delete tmp; tmp=NULL;} //utylizacja duplikatu z krzy¿ykiem
        else if (i>8?tmp->asName.SubString(1,9)=="lineinfo:":false) //tymczasowo wyj¹tki
        {delete tmp; tmp=NULL;} //tymczasowa utylizacja duplikatów W5
        else if (i>8?tmp->asName.SubString(i-7,8)=="_warning":false) //tymczasowo wyj¹tki
        {delete tmp; tmp=NULL;} //tymczasowa utylizacja duplikatu z tr¹bieniem
        else if (i>4?tmp->asName.SubString(i-3,4)=="_shp":false) //nie podlegaj¹ logowaniu
        {delete tmp; tmp=NULL;} //tymczasowa utylizacja duplikatu SHP
        if (tmp) //jeœli nie zosta³ zutylizowany
         if (Global::bJoinEvents)
          found->Append(tmp); //doczepka (taki wirtualny multiple bez warunków)
         else
         {ErrorLog("Duplicated event: "+tmp->asName);
          found->Append(tmp); //doczepka (taki wirtualny multiple bez warunków)
          found->Type=tp_Ignored; //dezaktywacja pierwotnego - taka proteza na wsteczn¹ zgodnoœæ
          //SafeDelete(tmp); //bezlitoœnie usuwamy wszelkie duplikaty, ¿eby nie zaœmiecaæ drzewka
         }
       }
       if (tmp)
       {//jeœli nie duplikat
        tmp->evNext2=RootEvent; //lista wszystkich eventów (m.in. do InitEvents)
        RootEvent=tmp;
        if (!found)
        {//jeœli nazwa wyst¹pi³a, to do kolejki i wyszukiwarki dodawany jest tylko pierwszy
         if (RootEvent->Type!=tp_Ignored)
          if (RootEvent->asName.Pos("onstart")) //event uruchamiany automatycznie po starcie
           AddToQuery(RootEvent,NULL); //dodanie do kolejki
         sTracks->Add(0,tmp->asName.c_str(),tmp); //dodanie do wyszukiwarki
        }
       }
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
      lp=Normalize(lp); //kierunek padania
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
 sTracks->Sort(TP_TRACK); //finalne sortowanie drzewa torów
 sTracks->Sort(TP_MEMCELL); //finalne sortowanie drzewa komórek pamiêci
 sTracks->Sort(TP_MODEL); //finalne sortowanie drzewa modeli
 sTracks->Sort(0); //finalne sortowanie drzewa eventów
 if (!bInitDone) FirstInit(); //jeœli nie by³o w scenerii
 if (Global::pTerrainCompact)
  TerrainWrite(); //Ra: teraz mo¿na zapisaæ teren w jednym pliku
 return true;
}

bool __fastcall TGround::InitEvents()
{//³¹czenie eventów z pozosta³ymi obiektami
 TGroundNode *tmp,*trk;
 char buff[255];
 int i;
 for (TEvent *Current=RootEvent;Current;Current=Current->evNext2)
 {
  switch (Current->Type)
  {
   case tp_AddValues: //sumowanie wartoœci
   case tp_UpdateValues: //zmiana wartoœci
    tmp=FindGroundNode(Current->asNodeName,TP_MEMCELL); //nazwa komórki powi¹zanej z eventem
    if (tmp)
    {//McZapkie-100302
     if (Current->iFlags&(conditional_trackoccupied|conditional_trackfree))
     {//jeœli chodzi o zajetosc toru (tor mo¿e byæ inny, ni¿ wpisany w komórce)
      trk=FindGroundNode(Current->asNodeName,TP_TRACK); //nazwa toru ta sama, co nazwa komórki
      if (trk) Current->Params[9].asTrack=trk->pTrack;
      if (!Current->Params[9].asTrack)
       ErrorLog("Bad event: track \""+AnsiString(Current->asNodeName)+"\" does not exists in \""+Current->asName+"\"");
     }
     Current->Params[4].nGroundNode=tmp;
     Current->Params[5].asMemCell=tmp->MemCell; //komórka do aktualizacji
     if (Current->iFlags&(conditional_memcompare))
      Current->Params[9].asMemCell=tmp->MemCell; //komórka do badania warunku
     if (tmp->MemCell->asTrackName!="none") //tor powi¹zany z komórk¹ powi¹zan¹ z eventem
     {//tu potrzebujemy wskaŸnik do komórki w (tmp)
      trk=FindGroundNode(tmp->MemCell->asTrackName,TP_TRACK);
      if (trk)
       Current->Params[6].asTrack=trk->pTrack;
      else
       ErrorLog("Bad memcell: track \""+tmp->MemCell->asTrackName+"\" not exists in memcell \""+tmp->asName+"\"");
     }
     else
      Current->Params[6].asTrack=NULL;
    }
    else
     ErrorLog("Bad event: \""+Current->asName+"\" cannot find memcell \""+Current->asNodeName+"\"");
   break;
   case tp_LogValues: //skojarzenie z memcell
    if (Current->asNodeName.IsEmpty())
    {//brak skojarzenia daje logowanie wszystkich
     Current->Params[9].asMemCell=NULL;
     break;
    }
   case tp_GetValues:
   case tp_WhoIs:
    tmp=FindGroundNode(Current->asNodeName,TP_MEMCELL);
    if (tmp)
    {
     Current->Params[8].nGroundNode=tmp;
     Current->Params[9].asMemCell=tmp->MemCell;
     if (Current->Type==tp_GetValues) //jeœli odczyt komórki
      if (tmp->MemCell->IsVelocity()) //a komórka zawiera komendê SetVelocity albo ShuntVelocity
       Current->bEnabled=false; //to event nie bêdzie dodawany do kolejki
    }
    else
     ErrorLog("Bad event: \""+Current->asName+"\" cannot find memcell \""+Current->asNodeName+"\"");
   break;
   case tp_CopyValues: //skopiowanie komórki do innej
    tmp=FindGroundNode(Current->asNodeName,TP_MEMCELL); //komórka docelowa
    if (tmp)
    {
     Current->Params[4].nGroundNode=tmp;
     Current->Params[5].asMemCell=tmp->MemCell; //komórka docelowa
    }
    else
     ErrorLog("Bad copyvalues: event \""+Current->asName+"\" cannot find memcell \""+Current->asNodeName+"\"");
    strcpy(buff,Current->Params[9].asText); //skopiowanie nazwy drugiej komórki do bufora roboczego
    SafeDeleteArray(Current->Params[9].asText); //usuniêcie nazwy komórki
    tmp=FindGroundNode(buff,TP_MEMCELL); //komórka Ÿód³owa
    if (tmp)
    {
     Current->Params[8].nGroundNode=tmp;
     Current->Params[9].asMemCell=tmp->MemCell; //komórka Ÿród³owa
    }
    else
     ErrorLog("Bad copyvalues: event \""+Current->asName+"\" cannot find memcell \""+AnsiString(buff)+"\"");
   break;
   case tp_Animation: //animacja modelu
    tmp=FindGroundNode(Current->asNodeName,TP_MODEL); //egzemplarza modelu do animowania
    if (tmp)
    {
     strcpy(buff,Current->Params[9].asText); //skopiowanie nazwy submodelu do bufora roboczego
     SafeDeleteArray(Current->Params[9].asText); //usuniêcie nazwy submodelu
     if (Current->Params[0].asInt==4)
      Current->Params[9].asModel=tmp->Model; //model dla ca³omodelowych animacji
     else
     {//standardowo przypisanie submodelu
      Current->Params[9].asAnimContainer=tmp->Model->GetContainer(buff); //submodel
      if (Current->Params[9].asAnimContainer)
      {Current->Params[9].asAnimContainer->WillBeAnimated(); //oflagowanie animacji
       if (!Current->Params[9].asAnimContainer->Event()) //nie szukaæ, gdy znaleziony
        Current->Params[9].asAnimContainer->EventAssign(FindEvent(Current->asNodeName+"."+AnsiString(buff)+":done"));
      }
     }
    }
    else
     ErrorLog("Bad animation: event \""+Current->asName+"\" cannot find model \""+Current->asNodeName+"\"");
    Current->asNodeName="";
   break;
   case tp_Lights: //zmiana œwiete³ modelu
    tmp=FindGroundNode(Current->asNodeName,TP_MODEL);
    if (tmp)
     Current->Params[9].asModel=tmp->Model;
    else
     ErrorLog("Bad lights: event \""+Current->asName+"\" cannot find model \""+Current->asNodeName+"\"");
    Current->asNodeName="";
   break;
   case tp_Visible: //ukrycie albo przywrócenie obiektu
    tmp=FindGroundNode(Current->asNodeName,TP_MODEL); //najpierw model
    if (!tmp) tmp=FindGroundNode(Current->asNodeName,TP_TRACTION); //mo¿e druty?
    if (!tmp) tmp=FindGroundNode(Current->asNodeName,TP_TRACK); //albo tory?
    if (tmp)
     Current->Params[9].nGroundNode=tmp;
    else
     ErrorLog("Bad visibility: event \""+Current->asName+"\" cannot find model \""+Current->asNodeName+"\"");
    Current->asNodeName="";
   break;
   case tp_Switch: //prze³o¿enie zwrotnicy albo zmiana stanu obrotnicy
    tmp=FindGroundNode(Current->asNodeName,TP_TRACK);
    if (tmp)
    {//dowi¹zanie toru
     if (!tmp->pTrack->iAction) //jeœli nie jest zwrotnic¹ ani obrotnic¹
      tmp->pTrack->iAction|=0x100; //to bêdzie siê zmienia³ stan uszkodzenia
     Current->Params[9].asTrack=tmp->pTrack;
     if (!Current->Params[0].asInt) //jeœli prze³¹cza do stanu 0
      if (Current->Params[2].asdouble>=0.0) //jeœli jest zdefiniowany dodatkowy ruch iglic
       Current->Params[9].asTrack->Switch(0,Current->Params[1].asdouble,Current->Params[2].asdouble); //przes³anie parametrów
    }
    else
     ErrorLog("Bad switch: event \""+Current->asName+"\" cannot find track \""+Current->asNodeName+"\"");
    Current->asNodeName="";
   break;
   case tp_Sound: //odtworzenie dŸwiêku
    tmp=FindGroundNode(Current->asNodeName,TP_SOUND);
    if (tmp)
     Current->Params[9].tsTextSound=tmp->tsStaticSound;
    else
     ErrorLog("Bad sound: event \""+Current->asName+"\" cannot find static sound \""+Current->asNodeName+"\"");
    Current->asNodeName="";
   break;
   case tp_TrackVel: //ustawienie prêdkoœci na torze
    if (!Current->asNodeName.IsEmpty())
    {
     tmp=FindGroundNode(Current->asNodeName,TP_TRACK);
     if (tmp)
     {tmp->pTrack->iAction|=0x200; //flaga zmiany prêdkoœci toru jest istotna dla skanowania
      Current->Params[9].asTrack=tmp->pTrack;
     }
     else
      ErrorLog("Bad velocity: event \""+Current->asName+"\" cannot find track \""+Current->asNodeName+"\"");
    }
    Current->asNodeName= "";
   break;
   case tp_DynVel: //komunikacja z pojazdem o konkretnej nazwie
    if (Current->asNodeName=="activator")
     Current->Params[9].asDynamic=NULL;
    else
    {
     tmp=FindGroundNode(Current->asNodeName,TP_DYNAMIC);
     if (tmp)
      Current->Params[9].asDynamic= tmp->DynamicObject;
     else
      Error("Event \""+Current->asName+"\" cannot find dynamic \""+Current->asNodeName+"\"");
    }
    Current->asNodeName= "";
   break;
   case tp_Multiple:
    if (Current->Params[9].asText!=NULL)
    {//przepisanie nazwy do bufora
     strcpy(buff,Current->Params[9].asText);
     SafeDeleteArray(Current->Params[9].asText);
    }
    else buff[0]='\0';
    if (Current->iFlags&(conditional_trackoccupied|conditional_trackfree))
    {//jeœli chodzi o zajetosc toru
     tmp=FindGroundNode(buff,TP_TRACK);
     if (tmp) Current->Params[9].asTrack=tmp->pTrack;
     if (!Current->Params[9].asTrack)
     {ErrorLog(AnsiString("Bad event: Track \"")+AnsiString(buff)+"\" does not exist in \""+Current->asName+"\"");
      Current->iFlags&=~(conditional_trackoccupied|conditional_trackfree); //zerowanie flag
     }
    }
    else if (Current->iFlags&(conditional_memstring|conditional_memval1|conditional_memval2))
    {//jeœli chodzi o komorke pamieciow¹
     tmp=FindGroundNode(buff,TP_MEMCELL);
     if (tmp) Current->Params[9].asMemCell=tmp->MemCell;
     if (!Current->Params[9].asMemCell)
     {ErrorLog(AnsiString("Bad event: MemCell \"")+AnsiString(buff)+AnsiString("\" does not exist in \""+Current->asName+"\""));
      Current->iFlags&=~(conditional_memstring|conditional_memval1|conditional_memval2);
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
   case tp_Voltage: //zmiana napiêcia w zasilaczu (TractionPowerSource)
    if (!Current->asNodeName.IsEmpty())
    {
     tmp=FindGroundNode(Current->asNodeName,TP_TRACTIONPOWERSOURCE); //pod³¹czenie zasilacza
     if (tmp)
      Current->Params[9].psPower=tmp->psTractionPowerSource;
     else
      ErrorLog("Bad voltage: event \""+Current->asName+"\" cannot find power source \""+Current->asNodeName+"\"");
    }
    Current->asNodeName= "";
   break;
  }
  if (Current->fDelay<0)
      AddToQuery(Current,NULL);
 }
 for (TGroundNode *Current=nRootOfType[TP_MEMCELL];Current;Current=Current->nNext)
 {//Ra: eventy komórek pamiêci, wykonywane po wys³aniu komendy do zatrzymanego pojazdu
  Current->MemCell->AssignEvents(FindEvent(Current->asName+":sent"));
 }
 return true;
}

void __fastcall TGround::InitTracks()
{//³¹czenie torów ze sob¹ i z eventami
 TGroundNode *Current,*Model;
 TTrack *tmp; //znaleziony tor
 TTrack *Track;
 int iConnection,state;
 AnsiString name;
 //tracks=tracksfar=0;
 for (Current=nRootOfType[TP_TRACK];Current;Current=Current->nNext)
 {
  Track=Current->pTrack;
  Track->AssignEvents(
   Track->asEvent0Name.IsEmpty()?NULL:FindEvent(Track->asEvent0Name),
   Track->asEvent1Name.IsEmpty()?NULL:FindEventScan(Track->asEvent1Name),
   Track->asEvent2Name.IsEmpty()?NULL:FindEventScan(Track->asEvent2Name));
  Track->AssignallEvents(
   Track->asEventall0Name.IsEmpty()?NULL:FindEvent(Track->asEventall0Name),
   Track->asEventall1Name.IsEmpty()?NULL:FindEvent(Track->asEventall1Name),
   Track->asEventall2Name.IsEmpty()?NULL:FindEvent(Track->asEventall2Name)); //MC-280503
  switch (Track->eType)
  {
   case tt_Table: //obrotnicê te¿ ³¹czymy na starcie z innymi torami
    Model=FindGroundNode(Current->asName,TP_MODEL); //szukamy modelu o tej samej nazwie
    //if (tmp) //mamy model, trzeba zapamiêtaæ wskaŸnik do jego animacji
    {//jak coœ pójdzie Ÿle, to robimy z tego normalny tor
     //Track->ModelAssign(tmp->Model->GetContainer(NULL)); //wi¹zanie toru z modelem obrotnicy
     Track->RaAssign(Current,Model?Model->Model:NULL,FindEvent(Current->asName+":done"),FindEvent(Current->asName+":joined")); //wi¹zanie toru z modelem obrotnicy
     //break; //jednak po³¹czê z s¹siednim, jak ma siê wysypywaæ null track
    }
    if (!Model) //jak nie ma modelu
     break; //to pewnie jest wykolejnica, a ta jest domyœlnie zamkniêta i wykoleja
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
 TIsolated *p=TIsolated::Root();
 while (p)
 {//jeœli siê znajdzie, to podaæ wskaŸnik
  Current=FindGroundNode(p->asName,TP_MEMCELL); //czy jest komóka o odpowiedniej nazwie
  if (Current)
   p->pMemCell=Current->MemCell; //przypisanie powi¹zanej komórki
  else
  {//utworzenie automatycznej komórki
   Current=new TGroundNode(); //to nie musi mieæ nazwy, nazwa w wyszukiwarce wystarczy
   //Current->asName=p->asName; //mazwa identyczna, jak nazwa odcinka izolowanego
   Current->MemCell=new TMemCell(NULL); //nowa komórka
   sTracks->Add(TP_MEMCELL,p->asName.c_str(),Current); //dodanie do wyszukiwarki
   Current->nNext=nRootOfType[TP_MEMCELL]; //to nie powinno tutaj byæ, bo robi siê œmietnik
   nRootOfType[TP_MEMCELL]=Current;
   iNumNodes++;
   p->pMemCell=Current->MemCell; //wskaŸnik komóki przekazany do odcinka izolowanego
  }
  p=p->Next();
 }
}

void __fastcall TGround::InitTraction()
{//³¹czenie drutów ze sob¹ oraz z torami i eventami
 TGroundNode *nCurrent,*nTemp;
 TTraction *tmp; //znalezione przês³o
 TTraction *Traction;
 int iConnection;
 AnsiString name;
 for (nCurrent=nRootOfType[TP_TRACTION];nCurrent;nCurrent=nCurrent->nNext)
 {//pod³¹czenie do zasilacza, ¿eby mo¿na by³o sumowaæ pr¹d kilku pojazdów
  //a jednoczeœnie z jednego miejsca zmieniaæ napiêcie eventem
  //wykonywane najpierw, ¿eby mo¿na by³o logowaæ pod³¹czenie 2 zasilaczy do jednego drutu
  //izolator zawieszony na przêœle jest ma byæ osobnym odcinkiem drutu o d³ugoœci ok. 1m,
  //pod³¹czonym do zasilacza o nazwie "*" (gwiazka); "none" nie bêdzie odpowiednie
  Traction=nCurrent->hvTraction;
  nTemp=FindGroundNode(Traction->asPowerSupplyName,TP_TRACTIONPOWERSOURCE);
  if (nTemp) //jak zasilacz znaleziony
   Traction->PowerSet(nTemp->psTractionPowerSource); //to pod³¹czyæ do przês³a
  else
   if (Traction->asPowerSupplyName!="*") //gwiazdka dla przês³a z izolatorem
    if (Traction->asPowerSupplyName!="none") //dopuszczamy na razie brak pod³¹czenia?
     ErrorLog("Missed TractionPowerSource: "+Traction->asPowerSupplyName);
 }
 for (nCurrent=nRootOfType[TP_TRACTION];nCurrent;nCurrent=nCurrent->nNext)
 {
  Traction=nCurrent->hvTraction;
  if (!Traction->hvNext[0]) //tylko jeœli jeszcze nie pod³¹czony
  {
   tmp=FindTraction(&Traction->pPoint1,iConnection,nCurrent);
   switch (iConnection)
   {
    case 0:
     Traction->Connect(0,tmp,0);
    break;
    case 1:
     Traction->Connect(0,tmp,1);
    break;
   }
   if (Traction->hvNext[0]) //jeœli zosta³ pod³¹czony
    if (Traction->psSection&&tmp->psSection) //tylko przês³o z izolatorem mo¿e nie mieæ zasilania, bo ma 2, trzeba sprawdzaæ s¹siednie
     if (Traction->psSection!=tmp->psSection) //po³¹czone odcinki maj¹ ró¿ne zasilacze
     {//to mo¿e byæ albo pod³¹czenie podstacji lub kabiny sekcyjnej do sekcji, albo b³¹d
      if (Traction->psSection->bSection&&!tmp->psSection->bSection)
      {//(tmp->psSection) jest podstacj¹, a (Traction->psSection) nazw¹ sekcji
       tmp->PowerSet(Traction->psSection); //zast¹pienie wskazaniem sekcji
      }
      else if (!Traction->psSection->bSection&&tmp->psSection->bSection)
      {//(Traction->psSection) jest podstacj¹, a (tmp->psSection) nazw¹ sekcji
       Traction->PowerSet(tmp->psSection); //zast¹pienie wskazaniem sekcji
      }
      else //jeœli obie to sekcje albo obie podstacje, to bêdzie b³¹d
       ErrorLog("Bad power: at "+FloatToStrF(Traction->pPoint1.x,ffFixed,6,2)+" "+FloatToStrF(Traction->pPoint1.y,ffFixed,6,2)+" "+FloatToStrF(Traction->pPoint1.z,ffFixed,6,2));
     }
  }
  if (!Traction->hvNext[1]) //tylko jeœli jeszcze nie pod³¹czony
  {
   tmp=FindTraction(&Traction->pPoint2,iConnection,nCurrent);
   switch (iConnection)
   {
    case 0:
     Traction->Connect(1,tmp,0);
    break;
    case 1:
     Traction->Connect(1,tmp,1);
    break;
   }
   if (Traction->hvNext[1]) //jeœli zosta³ pod³¹czony
    if (Traction->psSection&&tmp->psSection) //tylko przês³o z izolatorem mo¿e nie mieæ zasilania, bo ma 2, trzeba sprawdzaæ s¹siednie
     if (Traction->psSection!=tmp->psSection)
     {//to mo¿e byæ albo pod³¹czenie podstacji lub kabiny sekcyjnej do sekcji, albo b³¹d
      if (Traction->psSection->bSection&&!tmp->psSection->bSection)
      {//(tmp->psSection) jest podstacj¹, a (Traction->psSection) nazw¹ sekcji
       tmp->PowerSet(Traction->psSection); //zast¹pienie wskazaniem sekcji
      }
      else if (!Traction->psSection->bSection&&tmp->psSection->bSection)
      {//(Traction->psSection) jest podstacj¹, a (tmp->psSection) nazw¹ sekcji
       Traction->PowerSet(tmp->psSection); //zast¹pienie wskazaniem sekcji
      }
      else //jeœli obie to sekcje albo obie podstacje, to bêdzie b³¹d
       ErrorLog("Bad power: at "+FloatToStrF(Traction->pPoint2.x,ffFixed,6,2)+" "+FloatToStrF(Traction->pPoint2.y,ffFixed,6,2)+" "+FloatToStrF(Traction->pPoint2.z,ffFixed,6,2));
     }
  }
 }
 iConnection=0; //teraz bêdzie licznikiem koñców
 for (nCurrent=nRootOfType[TP_TRACTION];nCurrent;nCurrent=nCurrent->nNext)
 {//operacje maj¹ce na celu wykrywanie bie¿ni wspólnych i ³¹czenie przêse³ napr¹¿ania
  if (nCurrent->hvTraction->WhereIs()) //oznakowanie przedostatnich przêse³
  {//poszukiwanie bie¿ni wspólnej dla przedostatnich przêse³, równie¿ w celu po³¹czenia zasilania
   //to siê nie sprawdza, bo po³¹czyæ siê mog¹ dwa niezasilane odcinki jako najbli¿sze sobie
   //nCurrent->hvTraction->hvParallel=TractionNearestFind(nCurrent->pCenter,0,nCurrent); //szukanie najbli¿szego przês³a
   //trzeba by zliczaæ koñce, a potem wpisaæ je do tablicy, aby sukcesywnie pod³¹czaæ do zasilaczy
   nCurrent->hvTraction->iTries=5; //oznaczanie koñcowych
   ++iConnection;
  }
  if (nCurrent->hvTraction->fResistance[0]==0.0)
  {nCurrent->hvTraction->ResistanceCalc(); //obliczanie przêse³ w segmencie z bezpoœrednim zasilaniem
   //ErrorLog("Section "+nCurrent->hvTraction->asPowerSupplyName+" connected"); //jako niby b³¹d bêdzie bardziej widoczne
   nCurrent->hvTraction->iTries=0; //nie potrzeba mu szukaæ zasilania
  }
  //if (!Traction->hvParallel) //jeszcze utworzyæ pêtle z bie¿ni wspólnych
 }
 int zg=0; //zgodnoœæ kierunku przêse³, tymczasowo iterator do tabeli koñców
 TGroundNode **nEnds=new TGroundNode*[iConnection]; //koñców jest ok. 10 razy mniej ni¿ wszystkich przêse³ (Quark: 216)
 for (nCurrent=nRootOfType[TP_TRACTION];nCurrent;nCurrent=nCurrent->nNext)
 {//³¹czenie bie¿ni wspólnych, w tym oznaczanie niepodanych jawnie
  if (!Traction->asParallel.IsEmpty()) //bêdzie wskaŸnik na inne przês³o
   if ((Traction->asParallel=="none")||(Traction->asParallel=="*")) //jeœli nieokreœlone
    Traction->iLast=2; //jakby przedostatni - niech po prostu szuka (iLast ju¿ przeliczone)
   else
   {nTemp=FindGroundNode(Traction->asParallel,TP_TRACTION);
    if (nTemp) Traction->hvParallel=nTemp->hvTraction; //o ile znalezione
    if (!Traction->hvParallel)
     ErrorLog("Missed overhead: "+Traction->asParallel); //logowanie braku
   }
  if (nCurrent->hvTraction->iTries>0) //jeœli zaznaczony do pod³¹czenia
   //if (!nCurrent->hvTraction->psPower[0]||!nCurrent->hvTraction->psPower[1])
    if (zg<iConnection) //zabezpieczenie
     nEnds[zg++]=nCurrent; //wype³nianie tabeli koñców w celu szukania im po³¹czeñ
 }
 while (zg<iConnection)
  nEnds[zg++]=NULL; //zape³nienie do koñca tablicy, jeœli by jakieœ koñce wypad³y
 zg=1; //nieefektywny przebieg koñczy ³¹czenie
 while (zg)
 {//ustalenie zastêpczej rezystancji dla ka¿dego przês³a
  zg=0; //flaga pod³¹czonych przêse³ koñcowych: -1=puste wskaŸniki, 0=coœ zosta³o, 1=wykonano ³¹czenie
  for (int i=0;i<iConnection;++i)
   if (nEnds[i]) //za³atwione bêdziemy zerowaæ
   {//ka¿dy przebieg to próba pod³¹czenia koñca segmentu naprê¿ania do innego zasilanego przês³a
    if (nEnds[i]->hvTraction->hvNext[0])
    {//jeœli koñcowy ma ci¹g dalszy od strony 0 (Point1), szukamy odcinka najbli¿szego do Point2
     if (TractionNearestFind(nEnds[i]->hvTraction->pPoint2,0,nEnds[i])) //poszukiwanie przês³a
     {nEnds[i]=NULL;
      zg=1; //jak coœ zosta³o pod³¹czone, to mo¿e zasilanie gdzieœ dodatkowo dotrze
     }
    }
    else if (nEnds[i]->hvTraction->hvNext[1])
    {//jeœli koñcowy ma ci¹g dalszy od strony 1 (Point2), szukamy odcinka najbli¿szego do Point1
     if (TractionNearestFind(nEnds[i]->hvTraction->pPoint1,1,nEnds[i])) //poszukiwanie przês³a
     {nEnds[i]=NULL;
      zg=1; //jak coœ zosta³o pod³¹czone, to mo¿e zasilanie gdzieœ dodatkowo dotrze
     }
    }
    else
    {//gdy koniec jest samotny, to na razie nie zostanie pod³¹czony (nie powinno takich byæ)
     nEnds[i]=NULL;
    }
   }
 }
 delete[] nEnds; //nie potrzebne ju¿
};

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
 for (Current=nRootOfType[TP_EVLAUNCH];Current;Current=Current->nNext)
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
  if (x)
   if ((sr=FastGetSubRect(c+y,r-x))!=NULL)
    if ((tmp=sr->FindTrack(&Point,iConnection,Exclude->pTrack))!=NULL)
     return tmp;
  if (y)
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

TTraction* __fastcall TGround::FindTraction(vector3 *Point,int &iConnection,TGroundNode *Exclude)
{//wyszukiwanie innego przês³a koñcz¹cego siê w (Point)
 TTraction *Traction;
 TGroundNode *Current;
 TTraction *tmp;
 iConnection=-1;
 TSubRect *sr;
 //najpierw szukamy w okolicznych segmentach
 int c=GetColFromX(Point->x);
 int r=GetRowFromZ(Point->z);
 if ((sr=FastGetSubRect(c,r))!=NULL) //wiêkszoœæ bêdzie w tym samym sektorze
  if ((tmp=sr->FindTraction(Point,iConnection,Exclude->hvTraction))!=NULL)
   return tmp;
 int i,x,y;
 for (i=1;i<9;++i) //sektory w kolejnoœci odleg³oœci, 4 jest tu wystarczaj¹ce, 9 na wszelki wypadek
 {//wszystkie przês³a powinny zostaæ znajdowaæ siê w s¹siednich 8 sektorach
  x=SectorOrder[i].x;
  y=SectorOrder[i].y;
  if ((sr=FastGetSubRect(c+y,r+x))!=NULL)
   if ((tmp=sr->FindTraction(Point,iConnection,Exclude->hvTraction))!=NULL)
    return tmp;
  if (x&y)
  {if ((sr=FastGetSubRect(c+y,r-x))!=NULL)
    if ((tmp=sr->FindTraction(Point,iConnection,Exclude->hvTraction))!=NULL)
     return tmp;
   if ((sr=FastGetSubRect(c-y,r+x))!=NULL)
    if ((tmp=sr->FindTraction(Point,iConnection,Exclude->hvTraction))!=NULL)
     return tmp;
  }
  if ((sr=FastGetSubRect(c-y,r-x))!=NULL)
   if ((tmp=sr->FindTraction(Point,iConnection,Exclude->hvTraction))!=NULL)
    return tmp;
 }
 return NULL;
};

TTraction* __fastcall TGround::TractionNearestFind(vector3 &p,int dir,TGroundNode *n)
{//wyszukanie najbli¿szego do (p) przês³a o tej samej nazwie sekcji (ale innego ni¿ pod³¹czone) oraz zasilanego z kierunku (dir)
 TGroundNode *nCurrent,*nBest=NULL;
 int i,j,k,zg;
 double d,dist=200.0*200.0; //[m] odleg³oœæ graniczna
 //najpierw szukamy w okolicznych segmentach
 int c=GetColFromX(n->pCenter.x);
 int r=GetRowFromZ(n->pCenter.z);
 TSubRect *sr;
 for (i=-1;i<=1;++i) //przegl¹damy 9 najbli¿szych sektorów
  for (j=-1;j<=1;++j) //
   if ((sr=FastGetSubRect(c+i,r+j))!=NULL) //o ile w ogóle sektor jest
    for (nCurrent=sr->nRenderWires;nCurrent;nCurrent=nCurrent->nNext3)
     if (nCurrent->iType==TP_TRACTION)
      if (nCurrent->hvTraction->psSection==n->hvTraction->psSection) //jeœli ta sama sekcja
       if (nCurrent!=n) //ale nie jest tym samym
        if (nCurrent->hvTraction!=n->hvTraction->hvNext[0]) //ale nie jest bezpoœrednio pod³¹czonym
         if (nCurrent->hvTraction!=n->hvTraction->hvNext[1])
          if (nCurrent->hvTraction->psPower[k=(DotProduct(n->hvTraction->vParametric,nCurrent->hvTraction->vParametric)>=0?dir^1:dir)]) //ma zasilanie z odpowiedniej strony
           if (nCurrent->hvTraction->fResistance[k]>=0.0) //¿eby siê nie propagowa³y jakieœ ujemne
           {//znaleziony kandydat do po³¹czenia
            d=SquareMagnitude(p-nCurrent->pCenter); //kwadrat odleg³oœci œrodków
            if (dist>d)
            {//zapamiêtanie nowego najbli¿szego
             dist=d; //nowy rekord odleg³oœci
             nBest=nCurrent;
             zg=k; //z którego koñca braæ wskaŸnik zasilacza
            }
           }
 if (nBest) //jak znalezione przês³o z zasilaniem, to pod³¹czenie "równoleg³e"
 {n->hvTraction->ResistanceCalc(dir,nBest->hvTraction->fResistance[zg],nBest->hvTraction->psPower[zg]);
  //testowo skrzywienie przês³a tak, aby pokazaæ sk¹d ma zasilanie
  //if (dir) //1 gdy ci¹g dalszy jest od strony Point2
  // n->hvTraction->pPoint3=0.25*(nBest->pCenter+3*(zg?nBest->hvTraction->pPoint4:nBest->hvTraction->pPoint3));
  //else
  // n->hvTraction->pPoint4=0.25*(nBest->pCenter+3*(zg?nBest->hvTraction->pPoint4:nBest->hvTraction->pPoint3));
 }
 return (nBest?nBest->hvTraction:NULL);
};

bool __fastcall TGround::AddToQuery(TEvent *Event, TDynamicObject *Node)
{
 if (Event->bEnabled) //jeœli mo¿e byæ dodany do kolejki (nie u¿ywany w skanowaniu)
  if (!Event->iQueued) //jeœli nie dodany jeszcze do kolejki
  {//kolejka eventów jest posortowana wzglêdem (fStartTime)
   Event->Activator=Node;
   if (Event->Type==tp_AddValues?(Event->fDelay==0.0):false)
   {//eventy AddValues trzeba wykonywaæ natychmiastowo, inaczej kolejka mo¿e zgubiæ jakieœ dodawanie
    //Ra: kopiowanie wykonania tu jest bez sensu, lepiej by by³o wydzieliæ funkcjê wykonuj¹c¹ eventy i j¹ wywo³aæ
    if (EventConditon(Event))
    {//teraz mog¹ byæ warunki do tych eventów
     Event->Params[5].asMemCell->UpdateValues(Event->Params[0].asText,Event->Params[1].asdouble,Event->Params[2].asdouble,Event->iFlags);
     if (Event->Params[6].asTrack)
     {//McZapkie-100302 - updatevalues oprocz zmiany wartosci robi putcommand dla wszystkich 'dynamic' na danym torze
      for (int i=0;i<Event->Params[6].asTrack->iNumDynamics;++i)
       Event->Params[5].asMemCell->PutCommand(Event->Params[6].asTrack->Dynamics[i]->Mechanik,&Event->Params[4].nGroundNode->pCenter);
      if (DebugModeFlag)
       WriteLog("EVENT EXECUTED: AddValues & Track command - "+AnsiString(Event->Params[0].asText)+" "+AnsiString(Event->Params[1].asdouble)+" "+AnsiString(Event->Params[2].asdouble));
     }
     else
      if (DebugModeFlag)
       WriteLog("EVENT EXECUTED: AddValues - "+AnsiString(Event->Params[0].asText)+" "+AnsiString(Event->Params[1].asdouble)+" "+AnsiString(Event->Params[2].asdouble));
    }
    Event=Event->evJoined; //jeœli jest kolejny o takiej samej nazwie, to idzie do kolejki
   }
   if (Event)
   {//standardowe dodanie do kolejki
    WriteLog("EVENT ADDED TO QUEUE: "+Event->asName+(Node?AnsiString(" by "+Node->asName):AnsiString("")));
    Event->fStartTime=fabs(Event->fDelay)+Timer::GetTime(); //czas od uruchomienia scenerii
    if (Event->fRandomDelay>0.0)
     Event->fStartTime+=Event->fRandomDelay*random(10000)*0.0001; //doliczenie losowego czasu opóŸnienia
    ++Event->iQueued; //zabezpieczenie przed podwójnym dodaniem do kolejki
    if (QueryRootEvent?Event->fStartTime>=QueryRootEvent->fStartTime:false)
     QueryRootEvent->AddToQuery(Event); //dodanie gdzieœ w œrodku
    else
    {//dodanie z przodu: albo nic nie ma, albo ma byæ wykonany szybciej ni¿ pierwszy
     Event->evNext=QueryRootEvent;
     QueryRootEvent=Event;
    }
   }
  }
 return true;
}

bool __fastcall TGround::EventConditon(TEvent *e)
{//sprawdzenie spelnienia warunków dla eventu
 if (e->iFlags<=update_only) return true; //bezwarunkowo
 if (e->iFlags&conditional_trackoccupied)
  return (!e->Params[9].asTrack->IsEmpty());
 else if (e->iFlags&conditional_trackfree)
  return (e->Params[9].asTrack->IsEmpty());
 else if (e->iFlags&conditional_propability)
 {
  double rprobability=1.0*rand()/RAND_MAX;
  WriteLog("Random integer: "+CurrToStr(rprobability)+"/"+CurrToStr(e->Params[10].asdouble));
  return (e->Params[10].asdouble>rprobability);
 }
 else if (e->iFlags&conditional_memcompare)
 {//porównanie wartoœci
  if (tmpEvent->Params[9].asMemCell->Compare(e->Params[10].asText,e->Params[11].asdouble,e->Params[12].asdouble,e->iFlags))
   return true;
  else if (Global::iWriteLogEnabled && DebugModeFlag)
  {//nie zgadza siê wiêc sprawdzmy, co
   LogComment=e->Params[9].asMemCell->Text()+AnsiString(" ")+FloatToStrF(e->Params[9].asMemCell->Value1(),ffFixed,8,2)+" "+FloatToStrF(tmpEvent->Params[9].asMemCell->Value2(),ffFixed,8,2)+" != ";
   if (TestFlag(e->iFlags,conditional_memstring))
    LogComment+=AnsiString(tmpEvent->Params[10].asText);
   else
    LogComment+="*";
   if (TestFlag(tmpEvent->iFlags,conditional_memval1))
    LogComment+=" "+FloatToStrF(tmpEvent->Params[11].asdouble,ffFixed,8,2);
   else
    LogComment+=" *";
   if (TestFlag(tmpEvent->iFlags,conditional_memval2))
    LogComment+=" "+FloatToStrF(tmpEvent->Params[12].asdouble,ffFixed,8,2);
   else
    LogComment+=" *";
   WriteLog(LogComment.c_str());
  }
 }
 return false;
};

bool __fastcall TGround::CheckQuery()
{//sprawdzenie kolejki eventów oraz wykonanie tych, którym czas min¹³
 TLocation loc;
 int i;
/* //Ra: to w ogóle jakiœ chory kod jest; wygl¹da jak wyszukanie eventu z najlepszym czasem
 Double evtime,evlowesttime; //Ra: co to za typ?
 //evlowesttime=1000000;
 if (QueryRootEvent)
 {
  OldQRE=QueryRootEvent;
  tmpEvent=QueryRootEvent;
 }
 if (QueryRootEvent)
 {
  for (i=0;i<90;++i)
  {
   evtime=((tmpEvent->fStartTime)-(Timer::GetTime())); //pobranie wartoœci zmiennej
   if (evtime<evlowesttime)
   {
    evlowesttime=evtime;
    tmp2Event=tmpEvent;
   }
   if (tmpEvent->Next)
    tmpEvent=tmpEvent->Next;
   else
    i=100;
  }
  if (OldQRE!=tmp2Event)
  {
   QueryRootEvent->AddToQuery(QueryRootEvent);
   QueryRootEvent=tmp2Event;
  }
 }
*/
/*
 if (QueryRootEvent)
 {//wypisanie kolejki
  tmpEvent=QueryRootEvent;
  WriteLog("--> Event queue:");
  while (tmpEvent)
  {
   WriteLog(tmpEvent->asName+" "+AnsiString(tmpEvent->fStartTime));
   tmpEvent=tmpEvent->Next;
  }
 }
*/
 while (QueryRootEvent?QueryRootEvent->fStartTime<Timer::GetTime():false)
 {//eventy s¹ posortowana wg czasu wykonania
  tmpEvent=QueryRootEvent; //wyjêcie eventu z kolejki
  if (QueryRootEvent->evJoined) //jeœli jest kolejny o takiej samej nazwie
  {//to teraz on bêdzie nastêpny do wykonania
   QueryRootEvent=QueryRootEvent->evJoined; //nastêpny bêdzie ten doczepiony
   QueryRootEvent->evNext=tmpEvent->evNext; //pamiêtaj¹c o nastêpnym z kolejki
   QueryRootEvent->fStartTime=tmpEvent->fStartTime; //czas musi byæ ten sam, bo nie jest aktualizowany
   QueryRootEvent->Activator=tmpEvent->Activator; //pojazd aktywuj¹cy
   //w sumie mo¿na by go dodaæ normalnie do kolejki, ale trzeba te po³¹czone posortowaæ wg czasu wykonania
  }
  else //a jak nazwa jest unikalna, to kolejka idzie dalej
   QueryRootEvent=QueryRootEvent->evNext; //NULL w skrajnym przypadku
  if (tmpEvent->bEnabled)
  {//w zasadzie te wy³¹czone s¹ skanowane i nie powinny siê nigdy w kolejce znaleŸæ
   WriteLog("EVENT LAUNCHED: "+tmpEvent->asName+(tmpEvent->Activator?AnsiString(" by "+tmpEvent->Activator->asName):AnsiString("")));
   switch (tmpEvent->Type)
   {
    case tp_CopyValues: //skopiowanie wartoœci z innej komórki
     tmpEvent->Params[5].asMemCell->UpdateValues
     (tmpEvent->Params[9].asMemCell->Text(),
      tmpEvent->Params[9].asMemCell->Value1(),
      tmpEvent->Params[9].asMemCell->Value2(),
      tmpEvent->iFlags //flagi okreœlaj¹, co ma byæ skopiowane
     );
    break;
    case tp_AddValues: //ró¿ni siê jedn¹ flag¹ od UpdateValues
    case tp_UpdateValues:
     if (EventConditon(tmpEvent))
     {//teraz mog¹ byæ warunki do tych eventów
      tmpEvent->Params[5].asMemCell->UpdateValues(tmpEvent->Params[0].asText,tmpEvent->Params[1].asdouble,tmpEvent->Params[2].asdouble,tmpEvent->iFlags);
      if (tmpEvent->Params[6].asTrack)
      {//McZapkie-100302 - updatevalues oprocz zmiany wartosci robi putcommand dla wszystkich 'dynamic' na danym torze
       for (int i=0;i<tmpEvent->Params[6].asTrack->iNumDynamics;++i)
        tmpEvent->Params[5].asMemCell->PutCommand(tmpEvent->Params[6].asTrack->Dynamics[i]->Mechanik,&tmpEvent->Params[4].nGroundNode->pCenter);
       if (DebugModeFlag)
        WriteLog("Type: UpdateValues & Track command - "+AnsiString(tmpEvent->Params[0].asText)+" "+AnsiString(tmpEvent->Params[1].asdouble)+" "+AnsiString(tmpEvent->Params[2].asdouble));
      }
      else
       if (DebugModeFlag)
        WriteLog("Type: UpdateValues - "+AnsiString(tmpEvent->Params[0].asText)+" "+AnsiString(tmpEvent->Params[1].asdouble)+" "+AnsiString(tmpEvent->Params[2].asdouble));
     }
    break;
    case tp_GetValues:
     if (tmpEvent->Activator)
     {
      //loc.X= -tmpEvent->Params[8].nGroundNode->pCenter.x;
      //loc.Y=  tmpEvent->Params[8].nGroundNode->pCenter.z;
      //loc.Z=  tmpEvent->Params[8].nGroundNode->pCenter.y;
      if (Global::iMultiplayer) //potwierdzenie wykonania dla serwera (odczyt semafora ju¿ tak nie dzia³a)
       WyslijEvent(tmpEvent->asName,tmpEvent->Activator->GetName());
      //tmpEvent->Params[9].asMemCell->PutCommand(tmpEvent->Activator->Mechanik,loc);
      tmpEvent->Params[9].asMemCell->PutCommand(tmpEvent->Activator->Mechanik,&tmpEvent->Params[8].nGroundNode->pCenter);
     }
     WriteLog("Type: GetValues");
    break;
    case tp_PutValues:
     if (tmpEvent->Activator)
     {
      loc.X=-tmpEvent->Params[3].asdouble; //zamiana, bo fizyka ma inaczej ni¿ sceneria
      loc.Y= tmpEvent->Params[5].asdouble;
      loc.Z= tmpEvent->Params[4].asdouble;
      if (tmpEvent->Activator->Mechanik) //przekazanie rozkazu do AI
       tmpEvent->Activator->Mechanik->PutCommand(tmpEvent->Params[0].asText,tmpEvent->Params[1].asdouble,tmpEvent->Params[2].asdouble,loc);
      else
      {//przekazanie do pojazdu
       tmpEvent->Activator->MoverParameters->PutCommand(tmpEvent->Params[0].asText,tmpEvent->Params[1].asdouble,tmpEvent->Params[2].asdouble,loc);
      }
     }
     WriteLog("Type: PutValues");
    break;
    case tp_Lights:
     if (tmpEvent->Params[9].asModel)
      for (i=0;i<iMaxNumLights;i++)
       if (tmpEvent->Params[i].asdouble>=0) //-1 zostawia bez zmiany
        tmpEvent->Params[9].asModel->LightSet(i,tmpEvent->Params[i].asdouble); //teraz te¿ u³amek
    break;
    case tp_Visible:
     if (tmpEvent->Params[9].nGroundNode)
      tmpEvent->Params[9].nGroundNode->bVisible=(tmpEvent->Params[i].asInt>0);
    break;
    case tp_Velocity :
     Error("Not implemented yet :(");
    break;
    case tp_Exit :
     MessageBox(0,tmpEvent->asNodeName.c_str()," THE END ",MB_OK);
     Global::iTextMode=-1; //wy³¹czenie takie samo jak sekwencja F10 -> Y
     return false;
    case tp_Sound :
     switch (tmpEvent->Params[0].asInt)
     {//trzy mo¿liwe przypadki:
      case 0: tmpEvent->Params[9].tsTextSound->Stop(); break;
      case 1: tmpEvent->Params[9].tsTextSound->Play(1,0,true,tmpEvent->Params[9].tsTextSound->vSoundPosition); break;
      case -1: tmpEvent->Params[9].tsTextSound->Play(1,DSBPLAY_LOOPING,true,tmpEvent->Params[9].tsTextSound->vSoundPosition); break;
     }
    break;
    case tp_Disable :
     Error("Not implemented yet :(");
    break;
    case tp_Animation : //Marcin: dorobic translacje - Ra: dorobi³em ;-)
     if (tmpEvent->Params[0].asInt==1)
      tmpEvent->Params[9].asAnimContainer->SetRotateAnim(
       vector3(tmpEvent->Params[1].asdouble,
               tmpEvent->Params[2].asdouble,
               tmpEvent->Params[3].asdouble),
       tmpEvent->Params[4].asdouble);
     else if (tmpEvent->Params[0].asInt==2)
      tmpEvent->Params[9].asAnimContainer->SetTranslateAnim(
       vector3(tmpEvent->Params[1].asdouble,
               tmpEvent->Params[2].asdouble,
               tmpEvent->Params[3].asdouble),
       tmpEvent->Params[4].asdouble);
     else if (tmpEvent->Params[0].asInt==4)
      tmpEvent->Params[9].asModel->AnimationVND(
       tmpEvent->Params[8].asPointer,
       tmpEvent->Params[1].asdouble, //tu mog¹ byæ dodatkowe parametry, np. od-do
       tmpEvent->Params[2].asdouble,
       tmpEvent->Params[3].asdouble,
       tmpEvent->Params[4].asdouble);
    break;
    case tp_Switch:
     if (tmpEvent->Params[9].asTrack)
      tmpEvent->Params[9].asTrack->Switch(tmpEvent->Params[0].asInt,tmpEvent->Params[1].asdouble,tmpEvent->Params[2].asdouble);
     if (Global::iMultiplayer) //dajemy znaæ do serwera o prze³o¿eniu
      WyslijEvent(tmpEvent->asName,""); //wys³anie nazwy eventu prze³¹czajacego
     //Ra: bardziej by siê przyda³a nazwa toru, ale nie ma do niej st¹d dostêpu
    break;
    case tp_TrackVel:
     if (tmpEvent->Params[9].asTrack)
     {//prêdkoœæ na zwrotnicy mo¿e byæ ograniczona z góry we wpisie, wiêkszej siê ustawi eventem
      WriteLog("type: TrackVel");
      //WriteLog("Vel: ",tmpEvent->Params[0].asdouble);
      tmpEvent->Params[9].asTrack->VelocitySet(tmpEvent->Params[0].asdouble);
      if (DebugModeFlag) //wyœwietlana jest ta faktycznie ustawiona
       WriteLog("vel: ",tmpEvent->Params[9].asTrack->VelocityGet());
     }
    break;
    case tp_DynVel:
     Error("Event \"DynVel\" is obsolete");
    break;
    case tp_Multiple:
    {
     bCondition=EventConditon(tmpEvent);
     if (bCondition||(tmpEvent->iFlags&conditional_anyelse)) //warunek spelniony albo by³o u¿yte else
     {
      WriteLog("Multiple passed");
      for (i=0;i<8;++i)
      {//dodawane do kolejki w kolejnoœci zapisania
       if (tmpEvent->Params[i].asEvent)
        if (bCondition!=bool(tmpEvent->iFlags&(conditional_else<<i)))
        {
         if (tmpEvent->Params[i].asEvent!=tmpEvent)
          AddToQuery(tmpEvent->Params[i].asEvent,tmpEvent->Activator); //normalnie dodaæ
         else //jeœli ma byæ rekurencja
          if (tmpEvent->fDelay>=5.0) //to musi mieæ sensowny okres powtarzania
           if (tmpEvent->iQueued<2)
           {//trzeba zrobiæ wyj¹tek, aby event móg³ siê sam dodaæ do kolejki, raz ju¿ jest, ale bêdzie usuniêty
            //pêtla eventowa mo¿e byæ uruchomiona wiele razy, ale tylko pierwsze uruchomienie zadzia³a
            tmpEvent->iQueued=0; //tymczasowo, aby by³ ponownie dodany do kolejki
            AddToQuery(tmpEvent,tmpEvent->Activator);
            tmpEvent->iQueued=2; //kolejny raz ju¿ absolutnie nie dodawaæ
           }
       }
      }
      if (Global::iMultiplayer) //dajemy znaæ do serwera o wykonaniu
       if ((tmpEvent->iFlags&conditional_anyelse)==0) //jednoznaczne tylko, gdy nie by³o else
       {
        if (tmpEvent->Activator)
         WyslijEvent(tmpEvent->asName,tmpEvent->Activator->GetName());
        else
         WyslijEvent(tmpEvent->asName,"");
       }
     }
    }
    break;
    case tp_WhoIs: //pobranie nazwy poci¹gu do komórki pamiêci
     if (tmpEvent->iFlags&update_load)
     {//jeœli pytanie o ³adunek
      if (tmpEvent->iFlags&update_memadd) //jeœli typ pojazdu
       tmpEvent->Params[9].asMemCell->UpdateValues(
        tmpEvent->Activator->MoverParameters->TypeName.c_str(), //typ pojazdu
        0, //na razie nic
        0, //na razie nic
        tmpEvent->iFlags&(update_memstring|update_memval1|update_memval2));
      else //jeœli parametry ³adunku
       tmpEvent->Params[9].asMemCell->UpdateValues(
        tmpEvent->Activator->MoverParameters->LoadType!=""?tmpEvent->Activator->MoverParameters->LoadType.c_str():"none", //nazwa ³adunku
        tmpEvent->Activator->MoverParameters->Load, //aktualna iloœæ
        tmpEvent->Activator->MoverParameters->MaxLoad, //maksymalna iloœæ
        tmpEvent->iFlags&(update_memstring|update_memval1|update_memval2));
     }
     else if (tmpEvent->iFlags&update_memadd)
     {//jeœli miejsce docelowe pojazdu
      tmpEvent->Params[9].asMemCell->UpdateValues(
       tmpEvent->Activator->asDestination.c_str(), //adres docelowy
       tmpEvent->Activator->DirectionGet(), //kierunek pojazdu wzglêdem czo³a sk³adu (1=zgodny,-1=przeciwny)
       tmpEvent->Activator->MoverParameters->Power, //moc pojazdu silnikowego: 0 dla wagonu
       tmpEvent->iFlags&(update_memstring|update_memval1|update_memval2));
     }
     else if (tmpEvent->Activator->Mechanik)
      if (tmpEvent->Activator->Mechanik->Primary())
      {//tylko jeœli ktoœ tam siedzi - nie powinno dotyczyæ pasa¿era!
       tmpEvent->Params[9].asMemCell->UpdateValues(
        tmpEvent->Activator->Mechanik->TrainName().c_str(),
        tmpEvent->Activator->Mechanik->StationCount()-tmpEvent->Activator->Mechanik->StationIndex(), //ile przystanków do koñca
        tmpEvent->Activator->Mechanik->IsStop()?1:0, //1, gdy ma tu zatrzymanie
        tmpEvent->iFlags);
       WriteLog("Train detected: "+tmpEvent->Activator->Mechanik->TrainName());
      }
    break;
    case tp_LogValues: //zapisanie zawartoœci komórki pamiêci do logu
     if (tmpEvent->Params[9].asMemCell) //jeœli by³a podana nazwa komórki
      WriteLog("Memcell \""+tmpEvent->asNodeName+"\": "+
       tmpEvent->Params[9].asMemCell->Text()+" "+
       tmpEvent->Params[9].asMemCell->Value1()+" "+
       tmpEvent->Params[9].asMemCell->Value2());
     else //lista wszystkich
      for (TGroundNode *Current=nRootOfType[TP_MEMCELL];Current;Current=Current->nNext)
       WriteLog("Memcell \""+Current->asName+"\": "+
        Current->MemCell->Text()+" "+
        Current->MemCell->Value1()+" "+
        Current->MemCell->Value2());
    break;
    case tp_Voltage: //zmiana napiêcia w zasilaczu (TractionPowerSource)
     if (tmpEvent->Params[9].psPower)
     {//na razie takie chamskie ustawienie napiêcia zasilania
      WriteLog("type: Voltage");
      tmpEvent->Params[9].psPower->VoltageSet(tmpEvent->Params[0].asdouble);
     }
    break;
   } //switch (tmpEvent->Type)
  } //if (tmpEvent->bEnabled)
  --tmpEvent->iQueued; //teraz moze byæ ponownie dodany do kolejki
/*
  if (QueryRootEvent->eJoined) //jeœli jest kolejny o takiej samej nazwie
  {//to teraz jego dajemy do wykonania
   QueryRootEvent->eJoined->Next=QueryRootEvent->Next; //pamiêtaj¹c o nastêpnym z kolejki
   QueryRootEvent->eJoined->fStartTime=QueryRootEvent->fStartTime; //czas musi byæ ten sam, bo nie jest aktualizowany
   //QueryRootEvent->fStartTime=0;
   QueryRootEvent=QueryRootEvent->eJoined; //a wykonaæ ten doczepiony
  }
  else
  {//a jak nazwa jest unikalna, to kolejka idzie dalej
   //QueryRootEvent->fStartTime=0;
   QueryRootEvent=QueryRootEvent->Next; //NULL w skrajnym przypadku
  }
*/
 } //while
 return true;
}

void __fastcall TGround::OpenGLUpdate(HDC hDC)
{
 SwapBuffers(hDC); //swap buffers (double buffering)
};

void __fastcall TGround::UpdatePhys(double dt,int iter)
{//aktualizacja fizyki sta³ym krokiem: dt=krok czasu [s], dt*iter=czas od ostatnich przeliczeñ
 for (TGroundNode *Current=nRootOfType[TP_TRACTIONPOWERSOURCE];Current;Current=Current->nNext)
  Current->psTractionPowerSource->Update(dt*iter); //zerowanie sumy pr¹dów
};

bool __fastcall TGround::Update(double dt,int iter)
{//aktualizacja animacji krokiem FPS: dt=krok czasu [s], dt*iter=czas od ostatnich przeliczeñ
 //Ra: w zasadzie to trzeba by utworzyæ oddzieln¹ listê taboru do liczenia fizyki
 //    na któr¹ by siê zapisywa³y wszystkie pojazdy bêd¹ce w ruchu
 //    pojazdy stoj¹ce nie potrzebuj¹ aktualizacji, chyba ¿e np. ktoœ im zmieni nastawê hamulca
 //    oddzieln¹ listê mo¿na by zrobiæ na pojazdy z napêdem, najlepiej posortowan¹ wg typu napêdu
 if (iter>1) //ABu: ponizsze wykonujemy tylko jesli wiecej niz jedna iteracja
 {//pierwsza iteracja i wyznaczenie stalych:
  for (TGroundNode *Current=nRootDynamic;Current;Current=Current->nNext)
  {//
   Current->DynamicObject->MoverParameters->ComputeConstans();
   Current->DynamicObject->CoupleDist();
   Current->DynamicObject->UpdateForce(dt,dt,false);
  }
  for (TGroundNode *Current=nRootDynamic;Current;Current=Current->nNext)
   Current->DynamicObject->FastUpdate(dt);
  //pozostale iteracje
  for (int i=1;i<(iter-1);++i) //jeœli iter==5, to wykona siê 3 razy
  {
   for (TGroundNode *Current=nRootDynamic;Current;Current=Current->nNext)
    Current->DynamicObject->UpdateForce(dt,dt,false);
   for (TGroundNode *Current=nRootDynamic;Current;Current=Current->nNext)
    Current->DynamicObject->FastUpdate(dt);
  }
  //ABu 200205: a to robimy tylko raz, bo nie potrzeba wiêcej
  //Winger 180204 - pantografy
  double dt1=dt*iter; //ca³kowity czas
  for (TGroundNode *Current=nRootDynamic;Current;Current=Current->nNext)
  {//Ra: zmieniæ warunek na sprawdzanie pantografów w jednej zmiennej: czy pantografy i czy podniesione
   if (Current->DynamicObject->MoverParameters->EnginePowerSource.SourceType==CurrentCollector)
   //ABu: usunalem, bo sie krzaczylo: && (Current->DynamicObject->MoverParameters->PantFrontUp || Current->DynamicObject->MoverParameters->PantRearUp))
   //     a za to dodalem to:
   //   &&(Current->DynamicObject->MoverParameters->CabNo!=0)) //Ra: ten warunek jest bez sensu odk¹d trzeba pompowaæ z maszynowego
      GetTraction(Current->DynamicObject);
   Current->DynamicObject->UpdateForce(dt,dt1,true);//,true);
  }
  for (TGroundNode *Current=nRootDynamic;Current;Current=Current->nNext)
   Current->DynamicObject->Update(dt,dt1);
 }
 else
 {//jezeli jest tylko jedna iteracja
  for (TGroundNode *Current=nRootDynamic;Current;Current=Current->nNext)
  {
   if (Current->DynamicObject->MoverParameters->EnginePowerSource.SourceType==CurrentCollector)
      //&&(Current->DynamicObject->MoverParameters->CabNo!=0))
      GetTraction(Current->DynamicObject);
   Current->DynamicObject->MoverParameters->ComputeConstans();
   Current->DynamicObject->CoupleDist();
   Current->DynamicObject->UpdateForce(dt,dt,true);//,true);
  }
  for (TGroundNode *Current=nRootDynamic;Current;Current=Current->nNext)
   Current->DynamicObject->Update(dt,dt);
 }
 if (bDynamicRemove)
 {//jeœli jest coœ do usuniêcia z listy, to trzeba na koñcu
  for (TGroundNode *Current=nRootDynamic;Current;Current=Current->nNext)
   if (!Current->DynamicObject->bEnabled)
   {
    DynamicRemove(Current->DynamicObject); //usuniêcie tego i pod³¹czonych
    Current=nRootDynamic; //sprawdzanie listy od pocz¹tku
   }
  bDynamicRemove=false; //na razie koniec
 }
 return true;
};

//Winger 170204 - szukanie trakcji nad pantografami
bool __fastcall TGround::GetTraction(TDynamicObject *model)
{//aktualizacja drutu zasilaj¹cego dla ka¿dego pantografu, ¿eby odczytaæ napiêcie
 //jeœli pojazd siê nie porusza, to nie ma sensu przeliczaæ tego wiêcej ni¿ raz
 double fRaParam; //parametr równania parametrycznego odcinka drutu
 double fVertical; //odleg³oœæ w pionie; musi byæ w zasiêgu ruchu "pionowego" pantografu
 double fHorizontal; //odleg³oœæ w bok; powinna byæ mniejsza ni¿ pó³ szerokoœci pantografu
 vector3 vLeft,vUp,vFront,dwys;
 vector3 pant0;
 vector3 vParam; //wspó³czynniki równania parametrycznego drutu
 vector3 vStyk; //punkt przebicia drutu przez p³aszczyznê ruchu pantografu
 vector3 vGdzie; //wektor po³o¿enia drutu wzglêdem pojazdu
 vFront=model->VectorFront(); //wektor normalny dla p³aszczyzny ruchu pantografu
 vUp=model->VectorUp(); //wektor pionu pud³a (pochylony od pionu na przechy³ce)
 vLeft=model->VectorLeft(); //wektor odleg³oœci w bok (odchylony od poziomu na przechy³ce)
 dwys=model->GetPosition(); //wspó³rzêdne œrodka pojazdu
 TAnimPant *p; //wskaŸnik do obiektu danych pantografu
 for (int k=0;k<model->iAnimType[ANIM_PANTS];++k)
 {//pêtla po pantografach
  p=model->pants[k].fParamPants;
  if (k?model->MoverParameters->PantRearUp:model->MoverParameters->PantFrontUp)
  {//jeœli pantograf podniesiony
   pant0=dwys+(vLeft*p->vPos.z)+(vUp*p->vPos.y)+(vFront*p->vPos.x);
   if (p->hvPowerWire)
   {//mamy drut z poprzedniego przebiegu
    int n=30; //¿eby siê nie zapêtli³
    while (p->hvPowerWire)
    {//powtarzane a¿ do znalezienia odpowiedniego odcinka na liœcie dwukierunkowej
     //obliczamy wyraz wolny równania p³aszczyzny (to miejsce nie jest odpowienie)
     vParam=p->hvPowerWire->vParametric; //wspó³czynniki równania parametrycznego
     fRaParam=-DotProduct(pant0,vFront);
     //podstawiamy równanie parametryczne drutu do równania p³aszczyzny pantografu
     //vFront.x*(t1x+t*vParam.x)+vFront.y*(t1y+t*vParam.y)+vFront.z*(t1z+t*vParam.z)+fRaDist=0;
     fRaParam=-(DotProduct(p->hvPowerWire->pPoint1,vFront)+fRaParam)/DotProduct(vParam,vFront);
     if (fRaParam<-0.001) //histereza rzêdu 7cm na 70m typowego przês³a daje 1 promil
      p->hvPowerWire=p->hvPowerWire->hvNext[0];
     else if (fRaParam>1.001)
      p->hvPowerWire=p->hvPowerWire->hvNext[1];
     else if (p->hvPowerWire->iLast&3)
     {//dla ostatniego i przedostatniego przês³a wymuszamy szukanie innego
      p->hvPowerWire=NULL; //nie to, ¿e nie ma, ale trzeba sprawdziæ inne
      break;
     }
     else
     {//jeœli t jest w przedziale, wyznaczyæ odleg³oœæ wzd³u¿ wektorów vUp i vLeft
      vStyk=p->hvPowerWire->pPoint1+fRaParam*vParam; //punkt styku p³aszczyzny z drutem (dla generatora ³uku el.)
      vGdzie=vStyk-pant0; //wektor
      //odleg³oœæ w pionie musi byæ w zasiêgu ruchu "pionowego" pantografu
      fVertical=DotProduct(vGdzie,vUp); //musi siê mieœciæ w przedziale ruchu pantografu
      //odleg³oœæ w bok powinna byæ mniejsza ni¿ pó³ szerokoœci pantografu
      fHorizontal=DotProduct(vGdzie,vLeft); //to siê musi mieœciæ w przedziale zaleznym od szerokoœci pantografu
      //jeœli w pionie albo w bok jest za daleko, to dany drut jest nieu¿yteczny
      if (fabs(fHorizontal)>p->fWidth) //0.635 dla AKP-1 AKP-4E
       p->hvPowerWire=NULL; //nie liczy siê
      else
      {//po wyselekcjonowaniu drutu, przypisaæ go do toru, ¿eby nie trzeba by³o szukaæ
       //dla 3 koñcowych przêse³ sprawdziæ wszystkie dostêpne przês³a
       //bo mog¹ byæ umieszczone równolegle nad torem - po³¹czyæ w pierœcieñ
       //najlepiej, jakby odcinki równoleg³e by³y oznaczone we wpisach
       //WriteLog("Drut: "+AnsiString(fHorizontal)+" "+AnsiString(fVertical));
       p->PantTraction=fVertical;
       break; //koniec pêtli, aktualny drut pasuje
      }
     }
     if (--n<=0) //coœ za d³ugo to szukanie trwa
      p->hvPowerWire=NULL;
    }
   }
   if (!p->hvPowerWire) //else nie, bo móg³ zostaæ wyrzucony
   {//poszukiwanie po okolicznych sektorach
    int c=GetColFromX(dwys.x)+1;
    int r=GetRowFromZ(dwys.z)+1;
    TSubRect *tmp;
    TGroundNode *node;
    p->PantTraction=5.0; //taka za du¿a wartoœæ
    for (int j=r-2;j<=r;j++)
     for (int i=c-2;i<=c;i++)
     {//poszukiwanie po najbli¿szych sektorach niewiele da przy wiêkszym zagêszczeniu
      tmp=FastGetSubRect(i,j);
      if (tmp)
      {//dany sektor mo¿e nie mieæ nic w œrodku
       for (node=tmp->nRenderWires;node;node=node->nNext3)  //nastêpny z grupy
        if (node->iType==TP_TRACTION) //w grupie tej s¹ druty oraz inne linie
        {
         vParam=node->hvTraction->vParametric; //wspó³czynniki równania parametrycznego
         fRaParam=-DotProduct(pant0,vFront);
         fRaParam=-(DotProduct(node->hvTraction->pPoint1,vFront)+fRaParam)/DotProduct(vParam,vFront);
         if ((fRaParam>=-0.001)?(fRaParam<=1.001):false)
         {//jeœli tylko jest w przedziale, wyznaczyæ odleg³oœæ wzd³u¿ wektorów vUp i vLeft
          vStyk=node->hvTraction->pPoint1+fRaParam*vParam; //punkt styku p³aszczyzny z drutem (dla generatora ³uku el.)
          vGdzie=vStyk-pant0; //wektor
          fVertical=DotProduct(vGdzie,vUp); //musi siê mieœciæ w przedziale ruchu pantografu
          if (fVertical>=0.0) //jeœli ponad pantografem (bo mo¿e ³apaæ druty spod wiaduktu)
           if (Global::bEnableTraction?fVertical<p->PantWys-0.15:false) //jeœli drut jest ni¿ej ni¿ 15cm pod œlizgiem
           {//prze³¹czamy w tryb po³amania, o ile jedzie; (bEnableTraction) aby da³o siê jeŸdziæ na koœlawych sceneriach
            fHorizontal=DotProduct(vGdzie,vLeft); //i do tego jeszcze wejdzie pod œlizg
            if (fabs(fHorizontal)<=p->fWidth) //0.635 dla AKP-1 AKP-4E
            {p->PantWys=-1.0; //ujemna liczba oznacza po³amanie
             p->hvPowerWire=NULL; //bo inaczej siê zasila w nieskoñczonoœæ z po³amanego
             if (model->MoverParameters->EnginePowerSource.CollectorParameters.CollectorsNo>0) //liczba pantografów
              --model->MoverParameters->EnginePowerSource.CollectorParameters.CollectorsNo; //teraz bêdzie mniejsza
             if (DebugModeFlag)
              ErrorLog("Pant. break: at "+FloatToStrF(pant0.x,ffFixed,7,2)+" "+FloatToStrF(pant0.y,ffFixed,7,2)+" "+FloatToStrF(pant0.z,ffFixed,7,2));
            }
           }
           else if (fVertical<p->PantTraction) //ale ni¿ej, ni¿ poprzednio znaleziony
           {
            fHorizontal=DotProduct(vGdzie,vLeft); //to siê musi mieœciæ w przedziale zaleznym od szerokoœci pantografu
            if (fabs(fHorizontal)<=p->fWidth) //0.635 dla AKP-1 AKP-4E
            {p->hvPowerWire=node->hvTraction; //jakiœ znaleziony
             p->PantTraction=fVertical; //zapamiêtanie nowej wysokoœci
            }
           }
         } //warunek na parametr drutu <0;1>
        } //pêtla po drutach
      } //sektor istnieje
     } //pêtla po sektorach
   } //koniec poszukiwania w sektorach
   if (!p->hvPowerWire) //jeœli drut nie znaleziony
    if (!Global::bLiveTraction) //ale mo¿na oszukiwaæ
     model->pants[k].fParamPants->PantTraction=1.4; //to dajemy coœ tam dla picu
  } //koniec obs³ugi podniesionego
  else
   p->hvPowerWire=NULL; //pantograf opuszczony
 }
 return true;
};

bool __fastcall TGround::RenderDL(vector3 pPosition)
{//renderowanie scenerii z Display List - faza nieprzezroczystych
 glDisable(GL_BLEND);
 glAlphaFunc(GL_GREATER,0.45);     // im mniejsza wartoœæ, tym wiêksza ramka, domyœlnie 0.1f
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
    tmp->RenderSounds(); //jeszcze dŸwiêki pojazdów by siê przyda³y, równie¿ niewidocznych
   }
 //renderowanie progresywne - zale¿ne od FPS oraz kierunku patrzenia
 iRendered=0; //iloœæ renderowanych sektorów
 vector3 direction;
 for (k=0;k<Global::iSegmentsRendered;++k) //sektory w kolejnoœci odleg³oœci
 {//przerobione na u¿ycie SectorOrder
  i=SectorOrder[k].x; //na starcie oba >=0
  j=SectorOrder[k].y;
  do
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
 glEnable(GL_BLEND);
 glAlphaFunc(GL_GREATER,0.04);     // im mniejsza wartoœæ, tym wiêksza ramka, domyœlnie 0.1f
 TGroundNode *node;
 glColor4f(1.0f,1.0f,1.0f,1.0f);
 TSubRect *tmp;
 //Ra: renderowanie progresywne - zale¿ne od FPS oraz kierunku patrzenia
 int i;
 for (i=iRendered-1;i>=0;--i) //od najdalszych
 {//przezroczyste trójk¹ty w oddzielnym cyklu przed modelami
  tmp=pRendered[i];
  for (node=tmp->nRenderRectAlpha;node;node=node->nNext3)
   node->RenderAlphaDL(); //przezroczyste modele
 }
 for (i=iRendered-1;i>=0;--i) //od najdalszych
 {//renderowanie przezroczystych modeli oraz pojazdów
  pRendered[i]->RenderAlphaDL();
 }
 glDisable(GL_LIGHTING); //linie nie powinny œwieciæ
 for (i=iRendered-1;i>=0;--i) //od najdalszych
 {//druty na koñcu, ¿eby siê nie robi³y bia³e plamy na tle lasu
  tmp=pRendered[i];
  for (node=tmp->nRenderWires;node;node=node->nNext3)
   node->RenderAlphaDL(); //druty
 }
 return true;
}

bool __fastcall TGround::RenderVBO(vector3 pPosition)
{//renderowanie scenerii z VBO - faza nieprzezroczystych
 glDisable(GL_BLEND);
 glAlphaFunc(GL_GREATER,0.45);     // im mniejsza wartoœæ, tym wiêksza ramka, domyœlnie 0.1f 
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
   {for (node=tmp->nRenderHidden;node;node=node->nNext3)
     node->RenderHidden();
    tmp->RenderSounds(); //jeszcze dŸwiêki pojazdów by siê przyda³y, równie¿ niewidocznych
   }
  }
 //renderowanie progresywne - zale¿ne od FPS oraz kierunku patrzenia
 iRendered=0; //iloœæ renderowanych sektorów
 vector3 direction;
 for (k=0;k<Global::iSegmentsRendered;++k) //sektory w kolejnoœci odleg³oœci
 {//przerobione na u¿ycie SectorOrder
  i=SectorOrder[k].x; //na starcie oba >=0
  j=SectorOrder[k].y;
  do
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
 glEnable(GL_BLEND);
 glAlphaFunc(GL_GREATER,0.04);     // im mniejsza wartoœæ, tym wiêksza ramka, domyœlnie 0.1f 
 TGroundNode *node;
 glColor4f(1.0f,1.0f,1.0f,1.0f);
 TSubRect *tmp;
 int i;
 for (i=iRendered-1;i>=0;--i) //od najdalszych
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
 for (i=iRendered-1;i>=0;--i) //od najdalszych
  pRendered[i]->RenderAlphaVBO(); //przezroczyste modeli oraz pojazdy
 glDisable(GL_LIGHTING); //linie nie powinny œwieciæ
 for (i=iRendered-1;i>=0;--i) //od najdalszych
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
//--------------------------------
void __fastcall TGround::WyslijParam(int nr,int fl)
{//wys³anie parametrów symulacji w ramce (nr) z flagami (fl)
 DaneRozkaz r;
 r.iSygn='EU07';
 r.iComm=nr; //zwykle 5
 r.iPar[0]=fl; //flagi istotnoœci kolejnych parametrów
 int i=0; //domyœlnie brak danych
 switch (nr)
 {//mo¿na tym przesy³aæ ró¿ne zestawy parametrów
  case 5: //czas i pauza
   r.fPar[1]=Global::fTimeAngleDeg/360.0; //aktualny czas (1.0=doba)
   r.iPar[2]=Global::iPause; //stan zapauzowania
   i=8; //dwa parametry po 4 bajty ka¿dy
   break;
 }
 COPYDATASTRUCT cData;
 cData.dwData='EU07'; //sygnatura
 cData.cbData=12+i; //12+rozmiar danych
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
     if (node->iType==TP_TRACK) //Ra: przebudowaæ na u¿ycie tabeli torów?
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
    n=&((*n)->nNext); //sprawdzenie kolejnego pojazdu na liœcie
   }
   if ((*n)->DynamicObject==d)
   {//jeœli znaleziony
    node=(*n); //zapamiêtanie wêz³a, aby go usun¹æ
    (*n)=node->nNext; //pominiêcie na liœcie
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

void __fastcall TGround::TrackBusyList()
{//wys³anie informacji o wszystkich zajêtych odcinkach
 TGroundNode *Current;
 TTrack *Track;
 AnsiString name;
 for (Current=nRootOfType[TP_TRACK];Current;Current=Current->nNext)
  if (!Current->asName.IsEmpty()) //musi byæ nazwa
   if (Current->pTrack->iNumDynamics) //osi to chyba nie ma jak policzyæ
    WyslijString(Current->asName,8); //zajêty
};
//---------------------------------------------------------------------------

void __fastcall TGround::IsolatedBusyList()
{//wys³anie informacji o wszystkich odcinkach izolowanych
 TIsolated *Current;
 for (Current=TIsolated::Root();Current;Current=Current->Next())
  if (Current->Busy()) //sprawdŸ zajêtoœæ
   WyslijString(Current->asName,11); //zajêty
  else
   WyslijString(Current->asName,12); //wolny
};
//---------------------------------------------------------------------------

void __fastcall TGround::IsolatedBusy(const AnsiString t)
{//wys³anie informacji o wszystkich zajêtych odcinkach izolowanych
 TIsolated *Current;
 for (Current=TIsolated::Root();Current;Current=Current->Next())
  if (Current->asName==t)
   if (Current->Busy()) //sprawdŸ zajetoœæ
    WyslijString(Current->asName,11); //zajêty
 WyslijString("none",12); //wolny none - koniec przesy³ania
};
//---------------------------------------------------------------------------

void __fastcall TGround::Silence(vector3 gdzie)
{//wyciszenie wszystkiego w sektorach przed przeniesieniem kamery z (gdzie)
 int tr,tc;
 TGroundNode *node;
 int n=2*iNumSubRects; //(2*==2km) promieñ wyœwietlanej mapy w sektorach
 int c=GetColFromX(gdzie.x); //sektory wg dotychczasowej pozycji kamery
 int r=GetRowFromZ(gdzie.z);
 TSubRect *tmp;
 int i,j,k;
 //renderowanie czo³gowe dla obiektów aktywnych a niewidocznych
 for (j=r-n;j<=r+n;j++)
  for (i=c-n;i<=c+n;i++)
   if ((tmp=FastGetSubRect(i,j))!=NULL)
   {//tylko dŸwiêki interesuj¹
    for (node=tmp->nRenderHidden;node;node=node->nNext3)
     node->RenderHidden();
    tmp->RenderSounds(); //dŸwiêki pojazdów by siê przyda³o wy³¹czyæ
   }
};
//---------------------------------------------------------------------------
