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

#include    "system.hpp"
#include    "classes.hpp"

#include "opengl/glew.h"
#include "opengl/glut.h"

#pragma hdrstop

#include "Timer.h"
#include "Texture.h"
#include "ground.h"
#include "Globals.h"
#include "EvLaunch.h"
#include "TractionPower.h"

#include "parser.h" //Tolaris-010603
//---------------------------------------------------------------------------

#pragma package(smart_init)


bool bCondition; //McZapkie: do testowania warunku na event multiple
AnsiString LogComment;

// tablica pozycji sektorów do renderowania progresywnego (zale¿nie od FPS)
const int AreaFast[]={10,10,10,10,10,10,9,8,7,6,5}; //pe³ny zakres widocznoœci 2km
const int AreaSlow[]={ 7, 7, 7, 7, 7, 6,5,4,0,0,0}; //ograniczenie widocznoœci 1.5km
//---------------------------------------------------------------------------

__fastcall TGroundNode::TGroundNode()
{//nowy obiekt terenu - pusty
 Vertices=NULL;
 Next=pNext2=NULL;
 pCenter=vector3(0,0,0);
 iNumVerts=0; //wierzcho³ków w trójk¹cie
 //iNumPts=0; //punktów w linii
 TextureID=0;
 iFlags=0; //tryb przezroczystoœci nie zbadany
 DisplayListID = 0;
 //TexAlpha=false;
 Pointer=NULL; //zerowanie wskaŸnika kontekstowego
 iType=GL_POINTS;
 bVisible=false; //czy widoczny
 bStatic=true;
 fSquareRadius=2000*2000;
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
 bAllocated=true; //zawsze true
 pNext3=NULL; //nie wyœwietla innych
 iVboPtr=-1; //indeks w VBO sektora (-1: nie u¿ywa VBO)
}

__fastcall TGroundNode::~TGroundNode()
{
    if (bAllocated)
    {
        if (iType==TP_MEMCELL)
            SafeDelete(MemCell);
        if (iType==TP_EVLAUNCH)
            SafeDelete(EvLaunch);
        if (iType==TP_TRACTION)
            SafeDelete(Traction);
        if (iType==TP_TRACTIONPOWERSOURCE)
            SafeDelete(TractionPowerSource);
        if (iType==TP_TRACK)
            SafeDelete(pTrack);
        if (iType==TP_DYNAMIC)
            SafeDelete(DynamicObject);
        if (iType==TP_MODEL)
            SafeDelete(Model);
        if (iType==GL_LINES || iType==GL_LINE_STRIP || iType==GL_LINE_LOOP )
            SafeDeleteArray(Points);
        if (iType==GL_TRIANGLE_STRIP || iType==GL_TRIANGLE_FAN || iType==GL_TRIANGLES )
            SafeDeleteArray(Vertices);
    }
}

void __fastcall TGroundNode::Init(int n)
{//utworzenie tablicy wierzcho³ków
 bVisible=false;
 iNumVerts=n;
 Vertices=new TGroundVertex[iNumVerts];
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
   for (int i=0; i<iNumPts; i++)
    Points[i]+=pPosition;
   ResourceManager::Unregister(this);
   break;
  default:
   for (int i=0; i<iNumVerts; i++)
    Vertices[i].Point+=pPosition;
   ResourceManager::Unregister(this);
 }
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
int TGroundRect::iFrameNumber=0; //licznik wyœwietlanych klatek

void __fastcall TGroundRect::Render()
{//renderowanie kwadratu kilometrowego (DL), jeœli jeszcze nie zrobione
 if (iLastDisplay!=iFrameNumber)
 {
  //for (TGroundNode* node=pRender;node;node=node->pNext3)
  // node->Render(); //nieprzezroczyste trójk¹ty kwadratu kilometrowego
  if (pRender)
  {//³¹czenie trójk¹tów w jedn¹ listê - trochê wioska
   if (!pRender->DisplayListID)
   {//je¿eli nie skompilowany, kompilujemy wszystkie trójk¹ty w jeden
    pRender->fSquareRadius=5000.0*5000.0; //aby agregat nigdy nie znika³
    pRender->DisplayListID=glGenLists(1);
    glNewList(pRender->DisplayListID,GL_COMPILE);
    for (TGroundNode* node=pRender;node;node=node->pNext3)
     node->Compile(true);
    glEndList();
   }
   pRender->Render(); //nieprzezroczyste trójk¹ty kwadratu kilometrowego
  } 
  iLastDisplay=iFrameNumber;
 }
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

void __fastcall TGround::MoveGroundNode(vector3 pPosition)
{
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
    :(Current->iFlags&4)?true //~czy teksturê ma nieprzezroczyst¹?
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
}

void __fastcall TGroundNode::RaRenderVBO()
{//renderowanie z bufora VBO (Vertex Array gdy brak VBO)
 glColor3ub(Diffuse[0],Diffuse[1],Diffuse[2]);
 if (TextureID)
  glBindTexture(GL_TEXTURE_2D,TextureID); // Ustaw aktywn¹ teksturê
 glDrawArrays(iType,iVboPtr,iNumVerts);   // Narysuj naraz wszystkie trójk¹ty
}

void __fastcall TGroundNode::RaRender()
{
 double mgn=SquareMagnitude(pCenter-Global::pCameraPosition);
 if ((mgn>fSquareRadius || (mgn<fSquareMinRadius)) && (iType!=TP_EVLAUNCH)) //McZapkie-070602: nie rysuj odleglych obiektow ale sprawdzaj wyzwalacz zdarzen
     return;
 int i,a;
 switch (iType)
 {
  case TP_TRACTION: return;
  case TP_TRACK: if (iNumVerts) pTrack->RaRenderVBO(iVboPtr); return;
  case TP_MODEL: Model->RaRender(&pCenter); return;
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
    if (EvLaunch->dRadius<0 || mgn<EvLaunch->dRadius)
    {
     if (Pressed(VK_SHIFT) && EvLaunch->Event2!=NULL)
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
    glDisable(GL_LIGHTING); //nie powinny œwieciæ
    glDrawArrays(iType,iVboPtr,iNumPts); //rysowanie linii
    glEnable(GL_LIGHTING);
   }
   return;
  default:
   if (iVboPtr>=0)
    RaRenderVBO();
 };
/* Ra: trójk¹ty i linie renderuj¹ siê z VBO sektora
    // TODO: sprawdzic czy jest potrzebny warunek fLineThickness < 0
    if(
        (iNumVerts && (!Global::bRenderAlpha || !TexAlpha)) ||
        (iNumPts && (!Global::bRenderAlpha || fLineThickness < 0)))
    {

        if ( !DisplayListID || Global::bReCompile) //Ra: wymuszenie rekompilacji
        {
         Compile();
         if (Global::bManageNodes)
          ResourceManager::Register(this);
        };

        // GL_LINE, GL_LINE_STRIP, GL_LINE_LOOP
        if (iNumPts)
        {
            r=Diffuse[0]*Global::ambientDayLight[0];  //w zaleznosci od koloru swiatla
            g=Diffuse[1]*Global::ambientDayLight[1];
            b=Diffuse[2]*Global::ambientDayLight[2];
            glColor4ub(r,g,b,1.0);
            glCallList(DisplayListID);
        }
        // GL_TRIANGLE etc
        else
        {
            glCallList(DisplayListID);
        };

        SetLastUsage(Timer::GetSimulationTime());

    };
*/
 return;
};

void __fastcall TGroundNode::RaRenderAlpha()
{
 double mgn=SquareMagnitude(pCenter-Global::pCameraPosition);
 float r,g,b;
 if (mgn<fSquareMinRadius) return;
 if (mgn>fSquareRadius) return;
 int i,a;
 switch (iType)
 {
  case TP_TRACTION:
   if (bVisible)
    Traction->RaRenderVBO(mgn,iVboPtr);
   return;
  case TP_MODEL:
   //Model->RaRenderAlpha(pCenter,fAngle); return;
   Model->RaRenderAlpha(&pCenter); return;
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
    glDisable(GL_LIGHTING); //nie powinny œwieciæ
    glDrawArrays(iType,iVboPtr,iNumPts); //rysowanie linii
    glEnable(GL_LIGHTING);
   }
   return;
  default:
   if (iVboPtr>=0)
   {RaRenderVBO();
    return;
   }
 };
 return;
}

//------------------------------------------------------------------------------
__fastcall TSubRect::TSubRect()
{
 pRootNode=NULL; //lista wszystkich obiektów jest pusta
 pRenderHidden=pRenderRect=pRenderRectAlpha=pRender=pRenderMixed=pRenderAlpha=pRenderWires=NULL;
 pTrackAnim=NULL; //nic nie animujemy
 pTriGroup=NULL;
 iNodeCount=0; //licznik obiektów
}
__fastcall TSubRect::~TSubRect()
{
 if (Global::bManageNodes) //Ra: tu siê coœ sypie
  ResourceManager::Unregister(this); //wyrejestrowanie ze sprz¹tacza
}
void __fastcall TSubRect::AddNode(TGroundNode *Node)
{//przyczepienie obiektu do sektora, kwalifikacja trójk¹tów do ³¹czenia
 //Ra: trzeba zrobiæ sortowanie obiektów na grupy:
 //pRenderHidden    - lista obiektów niewidocznych, "renderowanych" równie¿ z ty³u
 //pRenderRect      - lista grup renderowanych z sektora
 //pRenderRectAlpha - lista grup renderowanych z sektora z przezroczystoœci¹
 //pRender          - lista grup renderowanych z w³asnych VBO albo DL
 //pRenderAlpha     - lista grup renderowanych z w³asnych VBO z przezroczystoœci¹ albo DL
 //pRenderWires     - lista grup renderowanych z w³asnych VBO - druty
 switch (Node->iType)
 {case TP_SOUND: //te obiekty s¹ sprawdzanie niezale¿nie od kierunku patrzenia
  case TP_EVLAUNCH:
   Node->pNext3=pRenderHidden; pRenderHidden=Node; //do listy koniecznych
   break;
  case TP_TRACK:
   Node->pTrack->RaOwnerSet(this); //gdzie ma zg³aszaæ animacjê
   if (Global::bUseVBO)
   {Node->pNext3=pRenderRect; pRenderRect=Node;} //VBO: do nieprzezroczystych z sektora
   else
   {Node->pNext3=pRender; pRender=Node;} //DL: do nieprzezroczystych
   break;
  case GL_TRIANGLE_STRIP:
  case GL_TRIANGLE_FAN:
  case GL_TRIANGLES:
   if (Node->iFlags&4) //czy jest przezroczyste?
   {Node->pNext3=pRenderRectAlpha; pRenderRectAlpha=Node;} //DL: do przezroczystych z sektora
   else
    if (Global::bUseVBO)
    {Node->pNext3=pRenderRect; pRenderRect=Node;} //VBO: do nieprzezroczystych z sektora
    else
    {Node->pNext3=pRender; pRender=Node;} //DL: do nieprzezroczystych wszelakich
/*
   //Ra: na razie wy³¹czone do testów VBO
   //if ((Node->iType==GL_TRIANGLE_STRIP)||(Node->iType==GL_TRIANGLE_FAN)||(Node->iType==GL_TRIANGLES))
    if (Node->fSquareMinRadius==0.0) //znikaj¹ce z bliska nie mog¹ byæ optymalizowane
     if (Node->fSquareRadius>=160000.0) //tak od 400m to ju¿ normalne trójk¹ty musz¹ byæ
     //if (Node->iFlags&2) //i nieprzezroczysty
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
  case GL_LINE_LOOP: //te na koñcu, ¿eby nie ³apa³y koloru nieba
   //if (Global::bUseVBO)
   Node->pNext3=pRenderWires; pRenderWires=Node; //lista drutów
   break;
  case TP_MODEL: //modle zawsze wyœwietlane z w³asnego VBO
   if ((Node->iFlags&0x04040004)==0) //czy brak przezroczystoœci?
   {Node->pNext3=pRender; pRender=Node;} //do nieprzezroczystych
   else if ((Node->iFlags&0x02020002)==0) //czy brak nieprzezroczystoœci?
   {Node->pNext3=pRenderAlpha; pRenderAlpha=Node;} //do przezroczystych
   else //jak i take i takie, to bêdzie dwa razy renderowane...
   {Node->pNext3=pRenderMixed; pRenderMixed=Node;} //do mieszanych
   break;
  case TP_MEMCELL:
  case TP_TRACTIONPOWERSOURCE: //a te w ogóle pomijamy
  case TP_ISOLATED: //lista torów w obwodzie izolowanym - na razie ignorowana
   break;
  case TP_DYNAMIC:
   return;
 }
 Node->pNext2=pRootNode; //dopisanie do ogólnej listy
 pRootNode=Node;
 ++iNodeCount; //licznik obiektów
};

bool __fastcall TSubRect::RaTrackAnimAdd(TTrack *t)
{//aktywacja animacji torów w VBO (zwrotnica, obrotnica)
 if (m_nVertexCount<0) return true; //nie ma animacji, gdy nie widaæ
 if (pTrackAnim)
  pTrackAnim->RaAnimListAdd(t);
 else
  pTrackAnim=t;
 return false; //bêdzie animowane...
}

void __fastcall TSubRect::RaAnimate()
{
 if (!pTrackAnim) return; //nie ma nic do animowania
 if (Global::bUseVBO)
 {//odœwie¿enie VBO sektora
  if (Global::bOpenGL_1_5) //modyfikacje VBO s¹ dostêpne od OpenGL 1.5
   glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_nVBOVertices);
  else //dla OpenGL 1.4 z GL_ARB_vertex_buffer_object odœwie¿enie ca³ego sektora
   Release(); //opró¿nienie VBO sektora, aby siê odœwie¿y³ z nowymi ustawieniami
 }
 pTrackAnim=pTrackAnim->RaAnimate(); //przeliczenie animacji kolejnego
};

void __fastcall TSubRect::LoadNodes()
{//utworzenie siatek VBO dla wszystkich node w sektorze
 if (m_nVertexCount>=0) return; //obiekty by³y ju¿ sprawdzone
 m_nVertexCount=0; //-1 oznacza, ¿e nie sprawdzono listy obiektów
 if (!pRootNode) return;
 TGroundNode *n=pRootNode;
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
  n=n->pNext2;
 }
 if (!m_nVertexCount) return; //jeœli nie ma obiektów do wyœwietlenia z VBO, to koniec
 if (Global::bUseVBO)
 {//tylko liczenie wierzcho³ów, gdy nie ma VBO
  MakeArray(m_nVertexCount);
  n=pRootNode;
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
       //miejsce w tablicach normalnych i teksturowania siê zmarnuje...
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
   n=n->pNext2;
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

void __fastcall TGroundNode::Compile(bool many)
{//tworzenie skompilowanej listy w wyœwietlaniu DL
 if (!many)
 {if (DisplayListID) Release();
  if (Global::bManageNodes)
  {
   DisplayListID=glGenLists(1);
   glNewList(DisplayListID,GL_COMPILE);
  }
 }
 if (iType == GL_LINES || iType == GL_LINE_STRIP || iType == GL_LINE_LOOP)
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
   glDrawArrays(Global::bWireFrame?GL_LINE_STRIP:tri->iType,0,tri->iNumVerts);
#else
   glBegin(Global::bWireFrame?GL_LINE_STRIP:tri->iType);
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
 };
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
 double mgn= SquareMagnitude(pCenter-Global::pCameraPosition);
 switch (iType)
 {
  case TP_SOUND: //McZapkie - dzwiek zapetlony w zaleznosci od odleglosci
   if ((pStaticSound->GetStatus()&DSBSTATUS_PLAYING)==DSBPLAY_LOOPING)
   {
    pStaticSound->Play(1,DSBPLAY_LOOPING,true,pStaticSound->vSoundPosition);
    pStaticSound->AdjFreq(1.0, Timer::GetDeltaTime());
   }
   return;
  case TP_EVLAUNCH:
   if (EvLaunch->Render())
    if (EvLaunch->dRadius<0 || mgn<EvLaunch->dRadius)
    {
     if (Pressed(VK_SHIFT) && EvLaunch->Event2!=NULL)
      Global::pGround->AddToQuery(EvLaunch->Event2,NULL);
     else
      if (EvLaunch->Event1!=NULL)
       Global::pGround->AddToQuery(EvLaunch->Event1,NULL);
    }
   return;
 }
};

void __fastcall TGroundNode::Render()
{//wyœwietlanie obiektów przez Display List
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
   //return Model->Render(pCenter,fAngle);
   return Model->Render(&pCenter);
 }
    // TODO: sprawdzic czy jest potrzebny warunek fLineThickness < 0
  if(
   (iNumVerts && (!Global::bRenderAlpha || (iFlags&2))) ||
   (iNumPts && (!Global::bRenderAlpha || fLineThickness < 0)))
  {
   if (!DisplayListID||Global::bReCompile) //Ra: wymuszenie rekompilacji
   {
    Compile();
    if (Global::bManageNodes)
     ResourceManager::Register(this);
   };

   if (iType == GL_LINES || iType == GL_LINE_STRIP || iType == GL_LINE_LOOP)
   //if (iNumPts)
   {
    float r,g,b;
    r=Diffuse[0]*Global::ambientDayLight[0];  //w zaleznosci od koloru swiatla
    g=Diffuse[1]*Global::ambientDayLight[1];
    b=Diffuse[2]*Global::ambientDayLight[2];
    glColor4ub(r,g,b,1.0);
    glCallList(DisplayListID);
   }
   // GL_TRIANGLE etc
   else
    glCallList(DisplayListID);
   SetLastUsage(Timer::GetSimulationTime());
  };
};

void __fastcall TGroundNode::RenderAlpha()
{
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
   if (Global::bRenderAlpha && bVisible)
    Traction->Render(mgn);
   return;
  case TP_MODEL:
   //Model->RenderAlpha(pCenter,fAngle);
   Model->RenderAlpha(&pCenter);
   return;
  case TP_TRACK:
   pTrack->RenderAlpha();
   return;
 };

 // TODO: sprawdzic czy jest potrzebny warunek fLineThickness < 0
 if (
     (iNumVerts && (iFlags&4)) ||
     (iNumPts && (Global::bRenderAlpha || fLineThickness > 0)))
 {

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
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

__fastcall TGround::TGround()
{
    RootNode=NULL;
    RootDynamic=NULL;
    QueryRootEvent=NULL;
    tmpEvent=NULL;
    tmp2Event=NULL;
    OldQRE=NULL;
    RootEvent=NULL;
    iNumNodes=0;
    pTrain=NULL;
    Global::pGround=this;
    bInitDone=false; //Ra: ¿eby nie robi³o dwa razy
}

__fastcall TGround::~TGround()
{
    Free();
}

void __fastcall TGround::Free()
{
    TEvent *tmp;
    for (TEvent *Current=RootEvent; Current!=NULL; )
    {
        tmp= Current;
        Current= Current->Next2;
        delete tmp;
    }
    TGroundNode *tmpn;
    for (TGroundNode *Current=RootNode; Current!=NULL; )
    {
        tmpn= Current;
        Current= Current->Next;
        delete tmpn;
    }
    for (TGroundNode *Current=RootDynamic; Current!=NULL; )
    {
        tmpn= Current;
        Current= Current->Next;
        delete tmpn;
    }
    iNumNodes=0;
    RootNode=NULL;
    RootDynamic=NULL;
}


double fTrainSetVel=0;
double fTrainSetDir=0;
double fTrainSetDist=0;
AnsiString asTrainSetTrack= "";
int iTrainSetConnection= 0;
bool bTrainSet= false;
AnsiString asTrainName= "";
int iTrainSetWehicleNumber= 0;
TGroundNode *TrainSetNode= NULL;

TGroundVertex TempVerts[100];
Byte TempConnectionType[200]; //Ra: ujemne, gdy odwrotnie

void __fastcall TGround::RaTriangleDivider(TGroundNode* node)
{//tworzy dodatkowe trójk¹ty i zmiejsza podany
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
 //no to tworzymy trzy dodatkowe trójk¹ty
 TGroundNode* tri[4]; //zmiena robocza - trzy wskaŸniki
 tri[3]=node; //do kompletu
 int i,j;
 for (i=0;i<3;++i)
 {//skopiowanie parametrów do nowych trzech trójk¹tów
  tri[i]=new TGroundNode();
  tri[i]->iType=GL_TRIANGLES;
  tri[i]->Init(3);
  tri[i]->TextureID=node->TextureID;
  tri[i]->iFlags=node->iFlags;
  for (j=0;j<4;++j)
  {tri[i]->Ambient[j]=node->Ambient[j];
   tri[i]->Diffuse[j]=node->Diffuse[j];
   tri[i]->Specular[j]=node->Specular[j];
  }
  tri[i]->asName=node->asName;
  tri[i]->fSquareRadius=node->fSquareRadius;
  tri[i]->fSquareMinRadius=node->fSquareMinRadius;
  tri[i]->bVisible=node->bVisible;
  tri[i]->bStatic=node->bStatic;
  tri[i]->Next=RootNode;
  RootNode=tri[i]; //dopisanie z przodu do listy
  iNumNodes++;
 }
 tri[0]->Vertices[0]=node->Vertices[0]; //przepisanie wspó³rzêdnych
 tri[1]->Vertices[1]=node->Vertices[1];
 tri[2]->Vertices[2]=node->Vertices[2];
 tri[3]->Vertices[0].HalfSet(tri[1]->Vertices[1],tri[2]->Vertices[2]);
 tri[3]->Vertices[1].HalfSet(tri[0]->Vertices[0],tri[2]->Vertices[2]);
 tri[3]->Vertices[2].HalfSet(tri[1]->Vertices[1],tri[0]->Vertices[0]);
 tri[0]->Vertices[1]=tri[3]->Vertices[2];
 tri[0]->Vertices[2]=tri[3]->Vertices[1];
 tri[1]->Vertices[0]=tri[3]->Vertices[2];
 tri[1]->Vertices[2]=tri[3]->Vertices[0];
 tri[2]->Vertices[0]=tri[3]->Vertices[1];
 tri[2]->Vertices[1]=tri[3]->Vertices[0];
 for (i=0;i<4;++i)
 {//przeliczenie œrodka ciê¿koœci wszystkich czterech
  tri[i]->pCenter=(tri[i]->Vertices[0].Point+tri[i]->Vertices[1].Point+tri[i]->Vertices[2].Point)/3;
 }
 for (i=0;i<3;++i)
  RaTriangleDivider(tri[i]); //rekurencja, bo nawet na TD raz nie wystarczy
}

TGroundNode* __fastcall TGround::AddGroundNode(cParser* parser)
{//wczytanie wpisu typu "node"
 //parser->LoadTraction=Global::bLoadTraction; //Ra: tu nie potrzeba powtarzaæ
 AnsiString str,str1,str2,str3,Skin,DriverType,asNodeName;
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
 else if (str=="semaphore")           tmp->iType=TP_SEMAPHORE;
 else if (str=="dynamic")             tmp->iType=TP_DYNAMIC;
 else if (str=="sound")               tmp->iType=TP_SOUND;
 else if (str=="track")               tmp->iType=TP_TRACK;
 else if (str=="memcell")             tmp->iType=TP_MEMCELL;
 else if (str=="eventlauncher")       tmp->iType=TP_EVLAUNCH;
 else if (str=="traction")            tmp->iType=TP_TRACTION;
 else if (str=="tractionpowersource") tmp->iType=TP_TRACTIONPOWERSOURCE;
 else if (str=="isolated")            tmp->iType=TP_ISOLATED;
 else bError=true;
 //WriteLog("-> node "+str+" "+tmp->asName);
 if (bError)
 {
  MessageBox(0,AnsiString("Scene parse error near "+str).c_str(),"Error",MB_OK);
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
   tmp->MemCell=new TMemCell();
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
   tmp->pTrack=new TTrack();
   if ((DebugModeFlag) && (tmp->asName!=AnsiString("")))
     WriteLog(tmp->asName.c_str());
   tmp->pTrack->Load(parser,pOrigin);

//            str=Parser->GetNextSymbol().LowerCase();
  //          str=Parser->GetNextSymbol().LowerCase();
    //        str=Parser->GetNextSymbol().LowerCase();
   tmp->pCenter=(tmp->pTrack->CurrentSegment()->FastGetPoint_0()+
                   tmp->pTrack->CurrentSegment()->FastGetPoint(0.5)+
                   tmp->pTrack->CurrentSegment()->FastGetPoint_1() ) * 0.33333f;
   break;
  case TP_SOUND :
   tmp->pStaticSound=new TRealSound;
   parser->getTokens(3);
   *parser >> tmp->pCenter.x >> tmp->pCenter.y >> tmp->pCenter.z;
   tmp->pCenter+=pOrigin;
   parser->getTokens();
   *parser >> token;
   str=AnsiString(token.c_str());
   tmp->pStaticSound->Init(str.c_str(),sqrt(tmp->fSquareRadius),tmp->pCenter.x,tmp->pCenter.y,tmp->pCenter.z);

//            tmp->pDirectSoundBuffer=TSoundsManager::GetFromName(str.c_str());
//            tmp->iState=(Parser->GetNextSymbol().LowerCase()=="loop"?DSBPLAY_LOOPING:0);
   parser->getTokens(); *parser >> token;
   break;
  case TP_DYNAMIC :
   tmp->DynamicObject=new TDynamicObject();
//   tmp->DynamicObject->Load(Parser);
   parser->getTokens();
   *parser >> token;
   str1=AnsiString(token.c_str());
// str2=Parser->GetNextSymbol().LowerCase(); McZapkie-131102: model w .mmd
//McZapkie: doszedl parametr ze zmienialna skora
   parser->getTokens();
   *parser >> token;
   Skin=AnsiString(token.c_str());
   parser->getTokens();
   *parser >> token;
   str3=AnsiString(token.c_str());
   if (bTrainSet)
   {
    str=asTrainSetTrack;
    parser->getTokens();
    *parser >> tf1;
//    int1=Parser->GetNextSymbol().ToInt();                 //Cab
    parser->getTokens();
    *parser >> token;
    DriverType=AnsiString(token.c_str());            //McZapkie:010303 - w przyszlosci rozne konfiguracje mechanik/pomocnik itp
    tf3=fTrainSetVel;
    parser->getTokens();
    *parser >> int1;
    //TempConnectionType[iTrainSetWehicleNumber]=toupper(TempConnectionType[iTrainSetWehicleNumber]);
    TempConnectionType[iTrainSetWehicleNumber]=int1;
    iTrainSetWehicleNumber++;
   }
   else
   {
    fTrainSetDist=0; //zerowanie dodatkowego przesuniêcia
    asTrainName="none";
    parser->getTokens();
    *parser >> token;
    str=AnsiString(token.c_str());           //track
    parser->getTokens();
    *parser >> tf1;                           //Dist
//  tf2=Parser->GetNextSymbol().ToDouble();
//  int1=Parser->GetNextSymbol().ToInt();    //Cab
    parser->getTokens();
    *parser >> token;
    DriverType=AnsiString(token.c_str());  //McZapkie:010303
    parser->getTokens();
    *parser >> tf3;                           //Vel
   }
   parser->getTokens();
   *parser >> int2;                               //Load
   if (int2>0)
   {
    parser->getTokens();
    *parser >> token;
    str2=AnsiString(token.c_str());  //LoadType
    if (str2==AnsiString("enddynamic"))           //idiotoodpornosc - ³adunek bez podanego typu
    {
     str2=""; int2=0;
    }
   }
   else
    str2="";  //brak ladunku

   tmp1=FindGroundNode(str,TP_TRACK);
   if (tmp1 && tmp1->pTrack)
   {
    Track=tmp1->pTrack;
//       if (bTrainSet)
  //                      tmp->DynamicObject->Init(Track,2,"",fTrainSetVel);
    //                else
					
					//ZiomalCl: poprawka na zmiane polozenia pociagu wzgledem toru podanego we wpisie 
					//- szukamy nazwy toru polozonego n metrow (n - odleglosc we wpisie trainset) od naszego toru
					//gdy znajdziemy, to do tego wlasnie skladu przypisujemy pociag
					if(Track->Length()<tf1)
                    {
                      double l1=tf1;
                      int fDirection1=1;
                      for(int i=0; i<20;i++)
                      {
                      if(Track->Length()<l1)
                        {
                        l1=l1-Track->Length();
                        if (fDirection1>0)
                        {
                          if (Track->bNextSwitchDirection)
                          {
                          Track= Track->CurrentNext();
                          fDirection1= -fDirection1;
                          }
                         else
                          {
                          Track= Track->CurrentNext();
                          }
                        }
                        else
                        if (fDirection1<0)
                        {
                          if (Track->bPrevSwitchDirection)
                          {
                          Track= Track->CurrentPrev();
                          fDirection1= -fDirection1;
                          }
                          else
                          {
                          Track= Track->CurrentPrev();
                          }
                        }

                        }

                      }
                      tf1=l1;
                    }
                    else if(tf1<0)
                    {
                      double l1=tf1;
                      int fDirection1=-1;

                      if (fDirection1>0)
                        {
                          if (Track->bNextSwitchDirection)
                          {
                          Track= Track->CurrentNext();
                          fDirection1= -fDirection1;
                          }
                         else
                          {
                          Track= Track->CurrentNext();
                          }
                        }
                        else
                        if (fDirection1<0)
                        {
                          if (Track->bPrevSwitchDirection)
                          {
                          Track= Track->CurrentPrev();
                          fDirection1= -fDirection1;
                          }
                          else
                          {
                          Track= Track->CurrentPrev();
                          }
                        }

                      for(int i=0; i<20;i++)
                      {

                        if(-Track->Length()>l1)
                        {
                        l1=Track->Length()+l1;
                        if (fDirection1>0)
                        {
                          if (Track->bNextSwitchDirection)
                          {
                          Track= Track->CurrentNext();
                          fDirection1= -fDirection1;
                          }
                         else
                          {
                          Track= Track->CurrentNext();
                          }
                        }
                        else
                        if (fDirection1<0)
                        {
                          if (Track->bPrevSwitchDirection)
                          {
                          Track= Track->CurrentPrev();
                          fDirection1= -fDirection1;
                          }
                          else
                          {
                          Track= Track->CurrentPrev();
                          }
                        }


                        }
                      }
                      tf1=Track->Length()+l1;

                    }             
					
    tmp->DynamicObject->Init(asNodeName,str1,Skin,str3,Track,(tf1==-1.0?fTrainSetDist:tf1+fTrainSetDist),DriverType,tf3,asTrainName,int2,str2,(tf1==-1.0));
    tmp->pCenter=tmp->DynamicObject->GetPosition();
//McZapkie-030203: sygnaly czola pociagu, ale tylko dla pociagow jadacych
    if (tf3>0) //predkosc poczatkowa, jak ja lubie takie nazwy zmiennych
    {
     if (bTrainSet)
     {
      if (asTrainName!=AnsiString("none"))
      {
       if (iTrainSetWehicleNumber==1)
       {
        tmp->DynamicObject->MoverParameters->EndSignalsFlag=1+4+16; //trojkat dla rozkladowych
        if ((tmp->DynamicObject->EndSignalsLight1Active())
           ||(tmp->DynamicObject->EndSignalsLight1oldActive()))
         tmp->DynamicObject->MoverParameters->HeadSignalsFlag=2+32;
        else
         tmp->DynamicObject->MoverParameters->HeadSignalsFlag=64;
       }
       if (iTrainSetWehicleNumber==2)
        LastDyn->MoverParameters->HeadSignalsFlag=0; //zgaszone swiatla od strony wagonow
      }
      else
      {
       if (iTrainSetWehicleNumber==1)
       {
        tmp->DynamicObject->MoverParameters->EndSignalsFlag=16;    //manewry
        tmp->DynamicObject->MoverParameters->HeadSignalsFlag=1;
       }
       if (iTrainSetWehicleNumber==2)
         LastDyn->MoverParameters->HeadSignalsFlag=0; //zgaszone swiatla od strony wagonow
      }
     }
     else
     {
      tmp->DynamicObject->MoverParameters->EndSignalsFlag=16;         //manewry pojed. pojazdu
      tmp->DynamicObject->MoverParameters->HeadSignalsFlag=1;
     }
     //Track->AddDynamicObject(Current->DynamicObject);
     LastDyn=tmp->DynamicObject;
    }
   }
   else
   {
    Error("Track does not exist \""+tmp->DynamicObject->asTrack+"\"");
    delete tmp;
    return NULL;
   }
   parser->getTokens();
   *parser >> token;
   if (token.compare("enddynamic")!=0)
    Error("enddynamic statement missing");
   tmp->bStatic=false;
   break;
  case TP_MODEL :
   parser->getTokens(3);
   *parser >> tmp->pCenter.x >> tmp->pCenter.y >> tmp->pCenter.z;
   parser->getTokens();
   *parser >> tf1;//tmp->fAngle;
   //OlO_EU&KAKISH-030103: obracanie punktow zaczepien w modelu
   tmp->pCenter.RotateY(aRotate.y/180*M_PI);
   //McZapkie-260402: model tez ma wspolrzedne wzgledne
   tmp->pCenter+=pOrigin;
   //tmp->fAngle+=aRotate.y; // /180*M_PI
   tmp->Model=new TAnimModel();
   tmp->Model->RaAnglesSet(aRotate.x,tf1+aRotate.y,aRotate.z); //dostosowanie do pochylania linii
   //tmp->Model->RaAnglesSet(0,tf1+aRotate.y,0);
//   str=Parser->GetNextSymbol().LowerCase();
   if (!tmp->Model->Load(parser))
    return NULL;
   tmp->iFlags=tmp->Model->Flags(); //ustalenie, czy przezroczysty
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
   tmp->TextureID=TTexturesManager::GetTextureID(str.c_str());
   tmp->iFlags=TTexturesManager::GetAlpha(tmp->TextureID)?4:2;
   i=0;
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
   //Ra: dzielenie trójk¹tów siê nie sprawdza - spadek FPS i znacznie d³u¿sze wczytywanie
   RaTriangleDivider(tmp); //no ale trzeba... teraz inaczej, mo¿e nie bêdzie tak Ÿle
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
  case TP_ISOLATED: //lista torów w obwodzie izolowanym - na razie ignorowana
   do
   {//po¿arcie dodatkowych parametrów
    parser->getTokens();
    *parser >> token;
   } while (token.compare("endisolated")!=0);
   break;
 }
 if (tmp->bStatic)
 {//jeœli nie jest pojazdem
  if (Global::bLoadTraction?true:(tmp->iType!=TP_TRACTION))
  {
   tmp->Next=RootNode;
   RootNode=tmp; //dopisanie z przodu do listy
   iNumNodes++;
  }
 }
 else
 {//jeœli jest pojazdem
  tmp->Next=RootDynamic;
  RootDynamic=tmp; //dopisanie z przodu do listy
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

bool __fastcall TGround::Init(AnsiString asFile)
{
    Global::pGround=this;
    pTrain=NULL;

    pOrigin=aRotate=vector3(0,0,0); //zerowanie przesuniêcia i obrotu

    AnsiString str="";
  //  TFileStream *fs;
//    int size;


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
    const int OriginStackMaxDepth=1000; //rozmiar stosu dla zagnie¿d¿enia origin
    int OriginStackTop=0;
    vector3 OriginStack[OriginStackMaxDepth]; //stos zagnie¿d¿enia origin

    double tf;
    int ParamCount,ParamPos;

    int hh,mm,srh,srm,ssh,ssm; //ustawienia czasu
    //ABu: Jezeli nie ma definicji w scenerii to ustawiane ponizsze wartosci:
    hh=10;  //godzina startu
    mm=30;  //minuty startu
    srh=6;  //godzina wschodu slonca
    srm=0;  //minuty wschodu slonca
    ssh=20; //godzina zachodu slonca
    ssm=0;  //minuty zachodu slonca
    TGroundNode *LastNode=NULL; //do trainset
    iNumNodes=0;
    token="";
    parser.getTokens();
    parser >> token;

    while (token!="") //(!Parser->EndOfFile)
    {
        str=AnsiString(token.c_str());
        if (str==AnsiString("node"))
        {
         LastNode=AddGroundNode(&parser); //rozpoznanie wêz³a
         if (LastNode)
         {
          if (Global::bLoadTraction?false:LastNode->iType==TP_TRACTION)
           SafeDelete(LastNode); //usuwamy druty, jeœli wy³¹czone
         }
         else
         {
          Error("Scene parse error near "+AnsiString(token.c_str()));
          break;
         }
        }
        else
        if (str==AnsiString("trainset"))
        {
         iTrainSetWehicleNumber=0;
         TrainSetNode=NULL;
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
         if (asTrainName!=AnsiString("none"))
          if (TrainSetNode) //trainset bez dynamic siê sypa³
          {//gdy podana nazwa, w³¹czenie jazdy poci¹gowej
           if((TrainSetNode->DynamicObject->EndSignalsLight1Active())
            ||(TrainSetNode->DynamicObject->EndSignalsLight1oldActive()))
            TrainSetNode->DynamicObject->MoverParameters->HeadSignalsFlag=2+32;
           else
            TrainSetNode->DynamicObject->MoverParameters->EndSignalsFlag=64;
          }
         bTrainSet=false;
         fTrainSetVel=0;
         //iTrainSetConnection=0;
         TrainSetNode=NULL;
         iTrainSetWehicleNumber=0;
        }
        else
        if (str==AnsiString("event"))
        {
            TEvent *tmp;
            tmp=RootEvent;
            RootEvent=new TEvent();
            RootEvent->Load(&parser);
            if (RootEvent->Type==tp_Unknown)
            {delete RootEvent;
             RootEvent=tmp; //przywrócenie z pominiêciem
            }
            else
             RootEvent->Next2=tmp;
        }
//        else
//        if (str==AnsiString("include"))  //Tolaris to zrobil wewnatrz parsera
//        {
//            Include(Parser);
//        }
        else
        if (str==AnsiString("rotate"))
        {
         //parser.getTokens(3);
         //parser >> aRotate.x >> aRotate.y >> aRotate.z; //Ra: to potrafi dawaæ b³êdne rezultaty
         parser.getTokens(); parser >> aRotate.x;
         parser.getTokens(); parser >> aRotate.y;
         parser.getTokens(); parser >> aRotate.z;
         //WriteLog("*** rotate "+AnsiString(aRotate.x)+" "+AnsiString(aRotate.y)+" "+AnsiString(aRotate.z));
        }
        else
        if (str==AnsiString("origin"))
        {
//            str=Parser->GetNextSymbol().LowerCase();
//            if (str=="begin")
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
        else
        if (str==AnsiString("endorigin"))
        {
//            else
  //          if (str=="end")
            {
                if (OriginStackTop<=0)
                {
                    MessageBox(0,AnsiString("Origin stack underflow ").c_str(),"Error",MB_OK);
                    break;
                }

                OriginStackTop--; //zmniejszenie wskaŸnika stosu
                pOrigin-=OriginStack[OriginStackTop];
            }
//            else
            {
  //              MessageBox(0,AnsiString("Scene parse error near "+str).c_str(),"Error",MB_OK);
    //            break;
            }

        }
        else
        if (str==AnsiString("atmo"))   //TODO: uporzadkowac gdzie maja byc parametry mgly!
        {
            WriteLog("Scenery atmo definition");
            parser.getTokens(3);
            parser >> Global::AtmoColor[0] >> Global::AtmoColor[1] >> Global::AtmoColor[2];
            glClearColor (Global::AtmoColor[0], Global::AtmoColor[1], Global::AtmoColor[2], 0.0);                  // Background Color
            parser.getTokens(2);
            parser >> Global::fFogStart >> Global::fFogEnd;

            if (Global::fFogStart==36)
            Global::bTimeChange=true;

            if (Global::fFogEnd>0)
            {
              parser.getTokens(3);
              parser >> Global::FogColor[0] >> Global::FogColor[1] >> Global::FogColor[2];
              glFogi(GL_FOG_MODE, GL_LINEAR);
              glFogfv(GL_FOG_COLOR, Global::FogColor);				// Set Fog Color
              glFogf(GL_FOG_START, Global::fFogStart);	        		// Fog Start Depth
              glFogf(GL_FOG_END, Global::fFogEnd);         			// Fog End Depth
              glEnable(GL_FOG);
            }
            else
                glDisable(GL_FOG);
            parser.getTokens();
            parser >> token;
            while (token.compare("endatmo")!=0)
             {
              parser.getTokens();
              parser >> token;
             }
        }
        else
        if (str==AnsiString("time"))
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
        else
        if (str==AnsiString("light"))
        {
         WriteLog("Scenery light definition");
         glDisable(GL_LIGHTING);
         vector3 lp;
         parser.getTokens(); parser >> lp.x;
         parser.getTokens(); parser >> lp.y;
         parser.getTokens(); parser >> lp.z;
         lp=Normalize(lp);
         Global::lightPos[0]=lp.x;
         Global::lightPos[1]=lp.y;
         Global::lightPos[2]=lp.z;
         glLightfv(GL_LIGHT0,GL_POSITION,Global::lightPos);        //daylight position
         parser.getTokens(); parser >> Global::ambientDayLight[0];
         parser.getTokens(); parser >> Global::ambientDayLight[1];
         parser.getTokens(); parser >> Global::ambientDayLight[2];
         glLightfv(GL_LIGHT0,GL_AMBIENT,Global::ambientDayLight);  //kolor wszechobceny

         parser.getTokens(); parser >> Global::diffuseDayLight[0];
         parser.getTokens(); parser >> Global::diffuseDayLight[1];
         parser.getTokens(); parser >> Global::diffuseDayLight[2];
	 glLightfv(GL_LIGHT0,GL_DIFFUSE,Global::diffuseDayLight);  //kolor padaj¹cy

         parser.getTokens(); parser >> Global::specularDayLight[0];
         parser.getTokens(); parser >> Global::specularDayLight[1];
         parser.getTokens(); parser >> Global::specularDayLight[2];
  	 glLightfv(GL_LIGHT0,GL_SPECULAR,Global::specularDayLight); //kolor odbity

         glEnable(GL_LIGHTING);
         do
          {  parser.getTokens(); parser >> token;
          } while (token.compare("endlight")!=0);

        }
        else
        if (str==AnsiString("camera"))
        {
         vector3 xyz,abc;
         WriteLog("Scenery camera definition");
         parser.getTokens(3);
         parser >> xyz.x >> xyz.y >> xyz.z;
         parser.getTokens(3);
         parser >> abc.x >> abc.y >> abc.z;
         int into=-1; //do której definicji kamery wstawiæ
         do
         {//opcjonalna siódma liczba okreœla numer kamery
          parser.getTokens(); parser >> token;
          if (into<0) into=atoi(token.c_str()); //takie sobie, bo mo¿na wpisaæ -1
         } while (token.compare("endcamera")!=0);
         if (into<0) into=++Global::iCameraLast;
         if ((into>=0)&&(into<10))
         {//przepisanie do odpowiedniego miejsca w tabelce
          Global::pFreeCameraInit[into]=xyz;
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
        {
         if (!bInitDone) //Ra: ¿eby nie robi³o dwa razy
         {bInitDone=true;
          WriteLog("InitNormals");
          for (TGroundNode* Current=RootNode;Current;Current=Current->Next)
          {
           Current->InitNormals();
           if (Current->iType!=TP_DYNAMIC)
           {//pojazdów to w ogóle nie dotyczy
            if ((Current->iType!=GL_TRIANGLES)&&(Current->iType!=GL_TRIANGLE_STRIP)?true //~czy trójk¹t?
             :(Current->iFlags&4)?true //~czy teksturê ma nieprzezroczyst¹?
              //:(Current->iNumVerts!=3)?true //~czy tylko jeden trójk¹t?
               :(Current->fSquareMinRadius!=0.0)?true //~czy widoczny z bliska?
                :(Current->fSquareRadius<=90000.0)) //~czy widoczny z daleka?
             GetSubRect(Current->pCenter.x,Current->pCenter.z)->AddNode(Current);
            else //dodajemy do kwadratu kilometrowego
             GetRect(Current->pCenter.x,Current->pCenter.z)->AddNode(Current);
           }
           //if (Current->iType!=TP_DYNAMIC)
           // GetSubRect(Current->pCenter.x,Current->pCenter.z)->AddNode(Current);
          }
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
         }
        }
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
        else if (str!=AnsiString(""))
        {
            Error(AnsiString("Unrecognized command: "+str));
//            WriteLog(token.c_str());
            break;
        }
        else
        if (str==AnsiString(""))
            break;

        if (bTrainSet && (LastNode?(LastNode->iType==TP_DYNAMIC):false))
        {
         if (TrainSetNode) //je¿eli jest przedostatni dynamic
          TrainSetNode->DynamicObject->AttachPrev(LastNode->DynamicObject,TempConnectionType[iTrainSetWehicleNumber-2]);
         TrainSetNode=LastNode; //ostatnio wczytany
//         fTrainSetVel=0; a po co to???
        }

        LastNode=NULL;

        token="";
        parser.getTokens();
	parser >> token;

    }
//    while(token!="");
//    DecimalSeparator=',';

    delete parser;


//------------------------------------Init dynamic---------------------------------
  /*
    for (Current=RootDynamic; Current!=NULL; Current=Current->Next)
    {
        if (Current->iType==TP_DYNAMIC)
        {
            tmp= FindGroundNode(Current->DynamicObject->asTrack,TP_TRACK);
            if (tmp && tmp->pTrack)
            {
                Track= tmp->pTrack;
                Current->DynamicObject->Init(Track,2);
                Current->pCenter= Current->DynamicObject->GetPosition();
//                Track->AddDynamicObject(Current->DynamicObject);

//                GetSubRect(Current->pCenter.x,Current->pCenter.z)->AddNode(Current);
            }
            else
            {
                Error("Track does not exist \""+Current->DynamicObject->asTrack+"\"");
                return false;
            }
        }
        else
            Error("Node is not dynamic \""+Current->asName+"\"");

    }
    return true;
    */
    return true;
}

const double SqrError= 0.00012;
bool __fastcall Equal(vector3 v1, vector3 v2)
{
    return (SquareMagnitude(v1-v2)<SqrError);
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
                      tmp= FindGroundNode(tmp->MemCell->asTrackName,TP_TRACK);
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
                    Error("Event \""+Current->asName+"\" cannot find node \""+
                                     Current->asNodeName+"\"");
            break;
            case tp_GetValues:
            case tp_WhoIs:
            case tp_LogValues: //skojarzenie z memcell
             tmp= FindGroundNode(Current->asNodeName,TP_MEMCELL);
             if (tmp)
             {
              Current->Params[8].asGroundNode=tmp;
              Current->Params[9].asMemCell=tmp->MemCell;
             }
             else
              Error("Event \""+Current->asName+"\" cannot find memcell \""+Current->asNodeName+"\"");
            break;
            case tp_Animation :
             tmp=FindGroundNode(Current->asNodeName,TP_MODEL); //egzemplarza modelu do animowania
             if (tmp)
             {
              strcpy(buff,Current->Params[9].asText); //skopiowanie nazwy submodelu do bufora roboczego
              delete Current->Params[9].asText; //usuniêcie nazwy submodelu
              Current->Params[9].asAnimContainer=tmp->Model->GetContainer(buff); //submodel
             }
             else
              Error("Event \""+Current->asName+"\" cannot find model \""+Current->asNodeName+"\"");
             Current->asNodeName="";
            break;
            case tp_Lights :
                tmp= FindGroundNode(Current->asNodeName,TP_MODEL);
                if (tmp)
                    Current->Params[9].asModel= tmp->Model;
                else
                    Error("Event \""+Current->asName+"\" cannot find model \""+
                                     Current->asNodeName+"\"");
                Current->asNodeName="";
            break;
            case tp_Switch :
                tmp= FindGroundNode(Current->asNodeName,TP_TRACK);
                if (tmp)
                    Current->Params[9].asTrack= tmp->pTrack;
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
                        Error("Event \""+Current->asName+"\" cannot find node \""+
                                         Current->asNodeName+"\"");
                }
                Current->asNodeName= "";
            break;
            case tp_Multiple :
                if (Current->Params[9].asText!=NULL)
                {
                    strcpy(buff,Current->Params[9].asText);
                    delete Current->Params[9].asText;
                    if (Current->Params[8].asInt<0) //ujemne znaczy sie chodzi o zajetosc toru
                     {
                      tmp= FindGroundNode(buff,TP_TRACK);
                      if (!tmp)
                        Error(AnsiString("Track \"")+AnsiString(buff)+AnsiString("\" does not exist"));
                      else
                       Current->Params[9].asTrack= tmp->pTrack;
                     }
                    if (Current->Params[8].asInt>0) //dodatnie znaczy sie chodzi o komorke pamieciowa
                     {
                      tmp= FindGroundNode(buff,TP_MEMCELL);
                      if (tmp==NULL)
                       {
                        Error(AnsiString("MemCell \"")+AnsiString(buff)+AnsiString("\" does not exist"));
                       }
                      else
                       {
                        Current->Params[9].asMemCell= tmp->MemCell;
                        if (!Current->Params[9].asMemCell)
                          Error(AnsiString("MemCell \"")+AnsiString(buff)+AnsiString("\" does not exist"));
                       }
                     }
                 }
                for (i=0; i<8; i++)
                {
                     if (Current->Params[i].asText!=NULL)
                     {
                        strcpy(buff,Current->Params[i].asText);
                        delete Current->Params[i].asText;
                        Current->Params[i].asEvent= FindEvent(buff);
                        if (!Current->Params[i].asEvent) //Ra: tylko w logu informacja o braku
                         WriteLog(AnsiString("Event \"")+AnsiString(buff)+AnsiString("\" does not exist"));
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
{
    TGroundNode *Current,*tmp;
    TTrack *Track;
    int iConnection,state;
    for (Current= RootNode; Current!=NULL; Current= Current->Next)
    {
        if (Current->iType==TP_TRACK)
        {
            Track= Current->pTrack;
            Track->AssignEvents(
                ( (Track->asEvent0Name!=AnsiString("")) ? FindEvent(Track->asEvent0Name) : NULL ),
                ( (Track->asEvent1Name!=AnsiString("")) ? FindEvent(Track->asEvent1Name) : NULL ),
                ( (Track->asEvent2Name!=AnsiString("")) ? FindEvent(Track->asEvent2Name) : NULL ) );
            Track->AssignallEvents(
                ( (Track->asEventall0Name!=AnsiString("")) ? FindEvent(Track->asEventall0Name) : NULL ),
                ( (Track->asEventall1Name!=AnsiString("")) ? FindEvent(Track->asEventall1Name) : NULL ),
                ( (Track->asEventall2Name!=AnsiString("")) ? FindEvent(Track->asEventall2Name) : NULL ) ); //MC-280503
            switch (Track->eType)
            {
                case tt_Turn: //obrotnicê te¿ ³¹czymy na starcie z innymi torami
                 tmp=FindGroundNode(Current->asName,TP_MODEL); //szukamy modelu o tej samej nazwie
                 if (tmp) //mamy model, trzeba zapamiêtaæ wskaŸnik do jego animacji
                 {//jak coœ pójdzie Ÿle, to robimy z tego normalny tor
                  //Track->ModelAssign(tmp->Model->GetContainer(NULL)); //wi¹zanie toru z modelem obrotnicy
                  Track->RaAssign(Current,tmp->Model); //wi¹zanie toru z modelem obrotnicy
                  //break; //jednak po³¹czê z s¹siednim, jak ma siê wysypywaæ null track
                 }
                case tt_Normal :
                    if (Track->CurrentPrev()==NULL)
                    {
                        tmp= FindTrack(Track->CurrentSegment()->FastGetPoint_0(),iConnection,Current);
                        switch (iConnection)
                        {
                            case -1: break;
                            case 0:
                                Track->ConnectPrevPrev(tmp->pTrack);
                            break;
                            case 1:
                                Track->ConnectPrevNext(tmp->pTrack);
                            break;
                            case 2: //Ra:zwrotnice nie maj¹ stanu pocz¹tkowego we wpisie
                                Track->ConnectPrevPrev(tmp->pTrack);
                                tmp->pTrack->SetConnections(0);
                            break;
                            case 3:
                                Track->ConnectPrevNext(tmp->pTrack);
                                tmp->pTrack->SetConnections(0);
                            break;
                            case 4:
                                tmp->pTrack->Switch(1);
                                Track->ConnectPrevPrev(tmp->pTrack);
                                tmp->pTrack->SetConnections(1);
                                tmp->pTrack->Switch(0);
                            break;
                            case 5:
                                tmp->pTrack->Switch(1);
                                Track->ConnectPrevNext(tmp->pTrack);
                                tmp->pTrack->SetConnections(1);
                                tmp->pTrack->Switch(0);
                            break;
                        }
                    }
                    if (Track->CurrentNext()==NULL)
                    {
                        tmp= FindTrack(Track->CurrentSegment()->FastGetPoint_1(),iConnection,Current);
                        switch (iConnection)
                        {
                            case -1: break;
                            case 0:
                                Track->ConnectNextPrev(tmp->pTrack);
                            break;
                            case 1:
                                Track->ConnectNextNext(tmp->pTrack);
                            break;
                            case 2:
                                //state= tmp->pTrack->GetSwitchState();
                                //tmp->pTrack->Switch(0);
                                Track->ConnectNextPrev(tmp->pTrack);
                                tmp->pTrack->SetConnections(0);
                                //tmp->pTrack->Switch(state);
                            break;
                            case 3:
                                //state= tmp->pTrack->GetSwitchState();
                                //tmp->pTrack->Switch(0);
                                Track->ConnectNextNext(tmp->pTrack);
                                tmp->pTrack->SetConnections(0);
                                //tmp->pTrack->Switch(state);
                            break;
                            case 4:
                                //state= tmp->pTrack->GetSwitchState();
                                tmp->pTrack->Switch(1);
                                Track->ConnectNextPrev(tmp->pTrack);
                                tmp->pTrack->SetConnections(1);
                                //tmp->pTrack->Switch(state);
                                tmp->pTrack->Switch(0);
                            break;
                            case 5:
                                //state= tmp->pTrack->GetSwitchState();
                                tmp->pTrack->Switch(1);
                                Track->ConnectNextNext(tmp->pTrack);
                                tmp->pTrack->SetConnections(1);
                                //tmp->pTrack->Switch(state);
                                tmp->pTrack->Switch(0);
                            break;
                        }
                    }
                break;
            }
        }
    }
    return true;
}

void __fastcall TGround::TrackJoin(TGroundNode *Current)
{//wyszukiwanie s¹siednich torów do pod³¹czenia (wydzielone dla obrotnicy)
 TTrack *Track=Current->pTrack;
 TGroundNode *tmp;
 int iConnection;
 if (!Track->CurrentPrev())
 {tmp=FindTrack(Track->CurrentSegment()->FastGetPoint_0(),iConnection,Current); //Current do pominiêcia
  switch (iConnection)
  {
   case 0: Track->ConnectPrevPrev(tmp->pTrack); break;
   case 1: Track->ConnectPrevNext(tmp->pTrack); break;
  }
 }
 if (!Track->CurrentNext())
 {
  tmp= FindTrack(Track->CurrentSegment()->FastGetPoint_1(),iConnection,Current);
  switch (iConnection)
  {
   case 0: Track->ConnectNextPrev(tmp->pTrack); break;
   case 1: Track->ConnectNextNext(tmp->pTrack); break;
  }
 }
}

//McZapkie-070602: wyzwalacze zdarzen
bool __fastcall TGround::InitLaunchers()
{
    TGroundNode *Current,*tmp;
    TEventLauncher *EventLauncher;
    int i;
    for (Current= RootNode; Current!=NULL; Current= Current->Next)
    {
       if (Current->iType==TP_EVLAUNCH)
         {
           EventLauncher= Current->EvLaunch;
           if (EventLauncher->iCheckMask!=0)
            if (EventLauncher->asMemCellName!=AnsiString("none"))
              {
                tmp= FindGroundNode(EventLauncher->asMemCellName,TP_MEMCELL);
                if (tmp)
                 {
                    EventLauncher->MemCell= tmp->MemCell;
                 }
                else
                  MessageBox(0,"Cannot find Memory Cell for Event Launcher","Error",MB_OK);
              }
             else
              EventLauncher->MemCell= NULL;
           EventLauncher->Event1= (EventLauncher->asEvent1Name!=AnsiString("none")) ? FindEvent(EventLauncher->asEvent1Name) : NULL;
           EventLauncher->Event2= (EventLauncher->asEvent2Name!=AnsiString("none")) ? FindEvent(EventLauncher->asEvent2Name) : NULL;
         }
    }
 return true;
}

TGroundNode* __fastcall TGround::FindTrack(vector3 Point, int &iConnection, TGroundNode *Exclude= NULL)
{
    int state;
    TTrack *Track;
    TGroundNode *Current,*tmp;
    iConnection= -1;
    for (Current= RootNode; Current!=NULL; Current= Current->Next)
    {
        if ((Current->iType==TP_TRACK) && (Current!=Exclude))
        {
            Track= Current->pTrack;
            switch (Track->eType)
            {
                case tt_Normal :
                    if (Track->CurrentPrev()==NULL)
                        if (Equal(Track->CurrentSegment()->FastGetPoint_0(),Point))
                        {

                            iConnection= 0;
                            return Current;
                        }
                    if (Track->CurrentNext()==NULL)
                        if (Equal(Track->CurrentSegment()->FastGetPoint_1(),Point))
                        {
                            iConnection= 1;
                            return Current;
                        }
                break;

                case tt_Switch :
                    state= Track->GetSwitchState();
                    Track->Switch(0);

                    if (Track->CurrentPrev()==NULL)
                        if (Equal(Track->CurrentSegment()->FastGetPoint_0(),Point))
                        {
                            iConnection= 2;
                            Track->Switch(state);
                            return Current;
                        }
                    if (Track->CurrentNext()==NULL)
                        if (Equal(Track->CurrentSegment()->FastGetPoint_1(),Point))
                        {
                            iConnection= 3;
                            Track->Switch(state);
                            return Current;
                        }
                    Track->Switch(1);
                    if (Track->CurrentPrev()==NULL)
                        if (Equal(Track->CurrentSegment()->FastGetPoint_0(),Point))
                        {
                            iConnection= 4;
                            Track->Switch(state);
                            return Current;
                        }
                    if (Track->CurrentNext()==NULL)
                        if (Equal(Track->CurrentSegment()->FastGetPoint_1(),Point))
                        {
                            iConnection= 5;
                            Track->Switch(state);
                            return Current;
                        }
                    Track->Switch(state);
                break;
            }
        }
    }
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
  WriteLog("EVENT ADDED TO QUEUE:");
  WriteLog(Event->asName.c_str());
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
        WriteLog("EVENT LAUNCHED:");
        WriteLog(QueryRootEvent->asName.c_str());
        switch (QueryRootEvent->Type)
        {
            case tp_AddValues: //ró¿ni siê jedn¹ flag¹ od UpdateValues
            case tp_UpdateValues :
                QueryRootEvent->Params[9].asMemCell->UpdateValues(QueryRootEvent->Params[0].asText,
                                                                  QueryRootEvent->Params[1].asdouble,
                                                                  QueryRootEvent->Params[2].asdouble,
                                                                  QueryRootEvent->Params[12].asInt);
//McZapkie-100302 - updatevalues oprocz zmiany wartosci robi putcommand dla wszystkich 'dynamic' na danym torze
                if (QueryRootEvent->Params[10].asTrack)
                 {
                  loc.X= -QueryRootEvent->Params[8].asGroundNode->pCenter.x;
                  loc.Y=  QueryRootEvent->Params[8].asGroundNode->pCenter.z;
                  loc.Z=  QueryRootEvent->Params[8].asGroundNode->pCenter.y;
                  for (int i=0; i<QueryRootEvent->Params[10].asTrack->iNumDynamics; i++)
                   {
                    QueryRootEvent->Params[9].asMemCell->PutCommand(QueryRootEvent->Params[10].asTrack->Dynamics[i]->MoverParameters,loc);
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
              loc.X= -QueryRootEvent->Params[8].asGroundNode->pCenter.x;
              loc.Y=  QueryRootEvent->Params[8].asGroundNode->pCenter.z;
              loc.Z=  QueryRootEvent->Params[8].asGroundNode->pCenter.y;
              QueryRootEvent->Params[9].asMemCell->PutCommand(QueryRootEvent->Activator->MoverParameters,loc);
              if (Global::bMultiplayer) //potwierdzenie wykonania dla serwera - najczêœciej odczyt semafora
               WyslijEvent(QueryRootEvent->asName,QueryRootEvent->Activator->GetasName());
             }
             WriteLog("Type: GetValues");
            break;
            case tp_PutValues :
                if (QueryRootEvent->Activator)
                {
                    loc.X= -QueryRootEvent->Params[3].asdouble;
                    loc.Y=  QueryRootEvent->Params[5].asdouble;
                    loc.Z=  QueryRootEvent->Params[4].asdouble;
                    QueryRootEvent->Activator->MoverParameters->PutCommand(QueryRootEvent->Params[0].asText,
                                                                           QueryRootEvent->Params[1].asdouble,
                                                                           QueryRootEvent->Params[2].asdouble,loc);
                }
                WriteLog("Type: PutValues");
            break;
            case tp_Lights :
                if (QueryRootEvent->Params[9].asModel)
                    for (i=0; i<iMaxNumLights; i++)
                        if (QueryRootEvent->Params[i].asInt>=0)
                            QueryRootEvent->Params[9].asModel->lsLights[i]=(TLightState)QueryRootEvent->Params[i].asInt;
            break;
            case tp_Velocity :
                Error("Not implemented yet :(");
            break;
            case tp_Exit :
                MessageBox(0,QueryRootEvent->asNodeName.c_str()," THE END ",MB_OK);
                return false;
            case tp_Sound :
              { if (QueryRootEvent->Params[0].asInt==0)
                  QueryRootEvent->Params[9].asRealSound->Stop();
                if (QueryRootEvent->Params[0].asInt==1)
                  QueryRootEvent->Params[9].asRealSound->Play(1,0,true,QueryRootEvent->Params[9].asRealSound->vSoundPosition);
                if (QueryRootEvent->Params[0].asInt==-1)
                  QueryRootEvent->Params[9].asRealSound->Play(1,DSBPLAY_LOOPING,true,QueryRootEvent->Params[9].asRealSound->vSoundPosition);
               }
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
            break;
            case tp_TrackVel :
                if (QueryRootEvent->Params[9].asTrack)
                {
                    WriteLog("type: TrackVel");
//                    WriteLog("Vel: ",QueryRootEvent->Params[0].asdouble);
                    QueryRootEvent->Params[9].asTrack->fVelocity= QueryRootEvent->Params[0].asdouble;
                    if (DebugModeFlag)
                     WriteLog("vel: ",QueryRootEvent->Params[9].asTrack->fVelocity);
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
                  else
                  if (QueryRootEvent->Params[8].asInt==conditional_propability)
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
                   if (!bCondition && Global::bWriteLogEnabled && DebugModeFlag) //nie zgadza sie wiec sprawdzmy co
                     {
                       LogComment="";
                       if (TestFlag(QueryRootEvent->Params[8].asInt,conditional_memstring))
                        LogComment=AnsiString(QueryRootEvent->Params[10].asText)+"="+QueryRootEvent->Params[9].asMemCell->szText;
                       if (TestFlag(QueryRootEvent->Params[8].asInt,conditional_memval1))
                        LogComment+=" v1:"+FloatToStrF(QueryRootEvent->Params[11].asdouble,ffFixed,8,2)+"="+FloatToStrF(QueryRootEvent->Params[9].asMemCell->fValue1,ffFixed,8,2);
                       if (TestFlag(QueryRootEvent->Params[8].asInt,conditional_memval2))
                        LogComment+=" v2:"+FloatToStrF(QueryRootEvent->Params[12].asdouble,ffFixed,8,2)+"="+FloatToStrF(QueryRootEvent->Params[9].asMemCell->fValue2,ffFixed,8,2);
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
                 if (Global::bMultiplayer) //dajemy znaæ do serwera o wykonaniu
                 {
                  if (QueryRootEvent->Activator)
                   WyslijEvent(QueryRootEvent->asName,QueryRootEvent->Activator->GetasName());
                  else
                   WyslijEvent(QueryRootEvent->asName,NULL);
                 }
                }
               }
            break;
            case tp_WhoIs: //pobranie nazwy poci¹gu do komórki pamiêci
             QueryRootEvent->Params[9].asMemCell->UpdateValues(
              QueryRootEvent->Activator->TrainParams->TrainName.c_str(),
              QueryRootEvent->Activator->TrainParams->StationCount,
              QueryRootEvent->Activator->TrainParams->StationIndex,
              conditional_memstring|conditional_memval1|conditional_memval2);
             WriteLog("Train detected: "+QueryRootEvent->Activator->TrainParams->TrainName);
            break;
            case tp_LogValues: //zapisanie zawartoœci komórki pamiêci do logu
             WriteLog("Memcell \""+QueryRootEvent->asNodeName+"\": "+
              QueryRootEvent->Params[9].asMemCell->szText+", "+
              QueryRootEvent->Params[9].asMemCell->fValue1+", "+
              QueryRootEvent->Params[9].asMemCell->fValue2);
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
        SwapBuffers(hDC);					// Swap Buffers (Double Buffering)
}

bool __fastcall TGround::Update(double dt, int iter)
//blablabla
{
   if(iter>1) //ABu: ponizsze wykonujemy tylko jesli wiecej niz jedna iteracja
   {
      //Pierwsza iteracja i wyznaczenie stalych:
      for (TGroundNode *Current= RootDynamic; Current!=NULL; Current= Current->Next)
      {
         Current->DynamicObject->MoverParameters->ComputeConstans();
         Current->DynamicObject->MoverParameters->SetCoupleDist();
         Current->DynamicObject->UpdateForce(dt, dt, false);
      }
      for (TGroundNode *Current= RootDynamic; Current!=NULL; Current= Current->Next)
      {
         Current->DynamicObject->FastUpdate(dt);
      }
      //pozostale iteracje
      for(int i=1 ; i<(iter-1); i++)
      {
         for (TGroundNode *Current= RootDynamic; Current!=NULL; Current= Current->Next)
         {
            Current->DynamicObject->UpdateForce(dt, dt, false);
         }
         for (TGroundNode *Current= RootDynamic; Current!=NULL; Current= Current->Next)
         {
            Current->DynamicObject->FastUpdate(dt);
         }
      }
      //ABu 200205: a to robimy tylko raz, bo nie potrzeba wiecej
      //Winger 180204 - pantografy
      double dt1=dt*iter;
      if (Global::bEnableTraction)
      {
         for (TGroundNode *Current= RootDynamic; Current!=NULL; Current= Current->Next)
         {
            if ((Current->DynamicObject->MoverParameters->EnginePowerSource.SourceType==CurrentCollector)
            //ABu: usunalem, bo sie krzaczylo: && (Current->DynamicObject->MoverParameters->PantFrontUp || Current->DynamicObject->MoverParameters->PantRearUp))
            //     a za to dodalem to:
               &&(Current->DynamicObject->MoverParameters->CabNo!=0))
               GetTraction(Current->DynamicObject->GetPosition(), Current->DynamicObject);
            Current->DynamicObject->UpdateForce(dt, dt1, true);//,true);
         }
         for (TGroundNode *Current= RootDynamic; Current!=NULL; Current= Current->Next)
         {
            Current->DynamicObject->Update(dt,dt1);
         }
      }
      else
      {
         for (TGroundNode *Current= RootDynamic; Current!=NULL; Current= Current->Next)
         {
            Current->DynamicObject->UpdateForce(dt, dt1, true);//,true);
         }
         for (TGroundNode *Current= RootDynamic; Current!=NULL; Current= Current->Next)
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
         for (TGroundNode *Current= RootDynamic; Current!=NULL; Current= Current->Next)
         {
            if ((Current->DynamicObject->MoverParameters->EnginePowerSource.SourceType==CurrentCollector)
               &&(Current->DynamicObject->MoverParameters->CabNo!=0))
               GetTraction(Current->DynamicObject->GetPosition(), Current->DynamicObject);
            Current->DynamicObject->MoverParameters->ComputeConstans();
            Current->DynamicObject->MoverParameters->SetCoupleDist();
            Current->DynamicObject->UpdateForce(dt, dt, true);//,true);
         }
         for (TGroundNode *Current= RootDynamic; Current!=NULL; Current= Current->Next)
         {
            Current->DynamicObject->Update(dt,dt);
         }
      }
      else
      {
         for (TGroundNode *Current= RootDynamic; Current!=NULL; Current= Current->Next)
         {
            Current->DynamicObject->MoverParameters->ComputeConstans();
            Current->DynamicObject->MoverParameters->SetCoupleDist();
            Current->DynamicObject->UpdateForce(dt, dt, true);//,true);
         }
         for (TGroundNode *Current= RootDynamic; Current!=NULL; Current= Current->Next)
         {
            Current->DynamicObject->Update(dt,dt);
         }
      }
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
    dirz= model->vFront;
    diry= model->vUp;
    dirx= model->vLeft;
    dwys= model->GetPosition();
    dynwys= dwys.y;
    np1wy=1000;
    np2wy=1000;
    //p1wy=0;   
    //p2wy=0;
    int n= 2; //iloœæ kwadratów hektometrowych mapy do przeszukania
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
    for (int j= r-n; j<r+n; j++)
        for (int i= c-n; i<c+n; i++)
        {
            tmp= FastGetSubRect(i,j);
            if (tmp)
            {
                for (node= tmp->pRootNode; node!=NULL; node=node->pNext2)
                {
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


bool __fastcall TGround::RaRender(vector3 pPosition)
{
 ++TGroundRect::iFrameNumber; //zwiêszenie licznika ramek
 CameraDirection.x=sin(Global::pCameraRotation); //wektor kierunkowy
 CameraDirection.z=cos(Global::pCameraRotation);
 int tr,tc;
 TGroundNode *node;
 glColor3f(1.0f,1.0f,1.0f);
 int n=2*iNumSubRects; //iloœæ sektorów mapy do wyœwietlenia
 int c=GetColFromX(pPosition.x);
 int r=GetRowFromZ(pPosition.z);
 TSubRect *tmp;
 int i,j,k;
 //renderowanie czo³gowe dla obiektów aktywnych a niewidocznych
 for (j=r-n;j<r+n;j++)
  for (i=c-n;i<c+n;i++)
  {
   if ((tmp=FastGetSubRect(i,j))!=NULL)
    for (node=tmp->pRenderHidden;node!=NULL;node=node->pNext3)
     node->RenderHidden();
  }
 //renderowanie progresywne - zale¿ne od FPS oraz kierunku patrzenia
 iRendered=0; //iloœæ renderowanych sektorów
 vector3 direction;
 iRange=Global::slowmotion?AreaSlow:AreaFast;
 n=(iRange[0]*n)/10; //tak dla zasady - 10 albo 7
 for (j=-n;j<=n;j++)
 {k=iRange[j<0?-j:j]; //zasiêg na danym poziomie
  for (i=-k;i<=k;i++)
  {
   direction=vector3(i,0,j);
   if (LengthSquared3(direction)>5)
   {direction=SafeNormalize(direction);
    if (CameraDirection.x*direction.x+CameraDirection.z*direction.z<0.55)
     continue; //pomijanie zbêdnych sektorów
   }
   Rects[(i+c)/iNumSubRects][(j+r)/iNumSubRects].RaRender(); //kwadrat kilometrowy nie zawsze, bo szkoda FPS
   if ((tmp=FastGetSubRect(i+c,j+r))!=NULL)
    if (tmp->iNodeCount) //je¿eli s¹ jakieœ obiekty, bo po co puste sektory przelatywaæ
     pRendered[iRendered++]=tmp; //tworzenie listy sektorów do renderowania
  }
 }
 for (i=0;i<iRendered;i++)
 {//renderowanie nieprzezroczystych
  tmp=pRendered[i];
  tmp->RaAnimate(); //przeliczenia animacji w sektorze przed zapiêciem VBO
   tmp->LoadNodes(); //ewentualne tworzenie siatek
   if (tmp->StartVBO())
   {for (node=tmp->pRenderRect;node!=NULL;node=node->pNext3)
     if (node->iVboPtr>=0)
      node->RaRender(); //nieprzezroczyste obiekty terenu
    tmp->EndVBO();
   }
   for (node=tmp->pRender;node;node=node->pNext3)
    node->RaRender(); //nieprzezroczyste modele
   for (node=tmp->pRenderMixed;node;node=node->pNext3)
    node->RaRender(); //nieprzezroczyste z mieszanych modeli
 }
 return true;
}

bool __fastcall TGround::RaRenderAlpha(vector3 pPosition)
{
 TGroundNode *node;
 glColor4f(1.0f,1.0f,1.0f,1.0f);
 TSubRect *tmp;
 int i;
 for (i=0;i<iRendered;i++)
 {//renderowanie przezroczystych trójk¹tów sektora
  tmp=pRendered[i];
  tmp->RaAnimate(); //przeliczenia animacji w sektorze przed zapiêciem VBO
   tmp->LoadNodes(); //ewentualne tworzenie siatek
   if (tmp->StartVBO())
   {for (node=tmp->pRenderRectAlpha;node!=NULL;node=node->pNext3)
     if (node->iVboPtr>=0)
      node->RaRenderAlpha(); //nieprzezroczyste obiekty terenu
    tmp->EndVBO();
   }
 }  
 for (i=0;i<iRendered;i++)
 {//renderowanie przezroczystych modeli oraz pojazdów
  tmp=pRendered[i];
  for (node=tmp->pRenderMixed;node;node=node->pNext3)
   node->RaRenderAlpha(); //przezroczyste z mieszanych modeli
  for (node=tmp->pRenderAlpha;node;node=node->pNext3)
   node->RaRenderAlpha(); //przezroczyste modele
  for (node=tmp->pRenderRect;node;node=node->pNext3)
   if (node->iType==TP_TRACK)
    node->pTrack->RaRenderDynamic(); //przezroczyste fragmenty pojazdów na torach
 }
 for (i=0;i<iRendered;i++)
 {//druty na koñcu, ¿eby siê nie robi³y bia³e plamy na tle lasu
  tmp=pRendered[i];
  if (tmp->StartVBO())
  {for (node=tmp->pRenderWires;node;node=node->pNext3)
    node->RaRenderAlpha(); //przezroczyste modele
   tmp->EndVBO();
  }
 }
 return true;
};

bool __fastcall TGround::Render(vector3 pPosition)
{//renderowanie scenerii z Display List - faza nieprzezroczystych
 ++TGroundRect::iFrameNumber; //zwiêszenie licznika ramek
 CameraDirection.x=sin(Global::pCameraRotation); //wektor kierunkowy
 CameraDirection.z=cos(Global::pCameraRotation);
 int tr,tc;
 TGroundNode *node;
 glColor3f(1.0f,1.0f,1.0f);
 int n=2*iNumSubRects; //iloœæ sektorów mapy do wyœwietlenia
 int c=GetColFromX(pPosition.x);
 int r=GetRowFromZ(pPosition.z);
 TSubRect *tmp;
 int i,j,k;
 //renderowanie czo³gowe dla obiektów aktywnych a niewidocznych
 for (j=r-n;j<r+n;j++)
  for (i=c-n;i<c+n;i++)
   if ((tmp=FastGetSubRect(i,j))!=NULL)
   {tmp->LoadNodes(); //oznaczanie aktywnych sektorów
    for (node=tmp->pRenderHidden;node!=NULL;node=node->pNext3)
     node->RenderHidden();
   }
 //renderowanie progresywne - zale¿ne od FPS oraz kierunku patrzenia
 iRendered=0; //iloœæ renderowanych sektorów
 vector3 direction;
 iRange=Global::slowmotion?AreaSlow:AreaFast;
 n=(iRange[0]*n)/10; //tak dla zasady - 10 albo 7
 for (j=-n;j<=n;j++)
 {k=iRange[j<0?-j:j]; //zasiêg na danym poziomie
  for (i=-k;i<=k;i++)
  {
   direction=vector3(i,0,j);
   if (LengthSquared3(direction)>5)
   {direction=SafeNormalize(direction);
    if (CameraDirection.x*direction.x+CameraDirection.z*direction.z<0.55)
     continue; //pomijanie zbêdnych sektorów
   }
   Rects[(i+c)/iNumSubRects][(j+r)/iNumSubRects].Render(); //kwadrat kilometrowy nie zawsze, bo szkoda FPS
   if ((tmp=FastGetSubRect(i+c,j+r))!=NULL)
    if (tmp->iNodeCount) //je¿eli s¹ jakieœ obiekty, bo po co puste sektory przelatywaæ
     pRendered[iRendered++]=tmp; //tworzenie listy sektorów do renderowania
  }
 }
 for (i=0;i<iRendered;i++)
 {//renderowanie nieprzezroczystych
  tmp=pRendered[i];
  tmp->RaAnimate(); //przeliczenia animacji torów w sektorze
  for (node=tmp->pRender;node;node=node->pNext3)
   node->Render(); //nieprzezroczyste obiekty (pojazdy z torów)
  for (node=tmp->pRenderMixed;node;node=node->pNext3)
   node->Render(); //nieprzezroczyste z mieszanych modeli
 }
 return true;
}

bool __fastcall TGround::RenderAlpha(vector3 pPosition)
{//renderowanie scenerii z Display List - faza przezroczystych
 TGroundNode *node;
 glColor4f(1.0f,1.0f,1.0f,1.0f);
 TSubRect *tmp;
 //Ra: renderowanie progresywne - zale¿ne od FPS oraz kierunku patrzenia
 int i;
 for (i=0;i<iRendered;i++)
 {//przezroczyste trójk¹ty w oddzielnym cyklu przed modelami
  tmp=pRendered[i];
  for (node=tmp->pRenderRectAlpha;node;node=node->pNext3)
   node->RenderAlpha(); //przezroczyste modele
 }
 for (i=0;i<iRendered;i++)
 {//renderowanie przezroczystych modeli oraz pojazdów
  tmp=pRendered[i];
  for (node=tmp->pRenderMixed;node;node=node->pNext3)
   node->RenderAlpha(); //przezroczyste z mieszanych modeli
  for (node=tmp->pRenderAlpha;node;node=node->pNext3)
   node->RenderAlpha(); //przezroczyste modele
  for (node=tmp->pRender;node;node=node->pNext3)
   if (node->iType==TP_TRACK)
    node->pTrack->RenderAlpha(); //przezroczyste fragmenty pojazdów na torach
 }
 for (i=0;i<iRendered;i++)
 {//druty na koñcu, ¿eby siê nie robi³y bia³e plamy na tle lasu
  tmp=pRendered[i];
  for (node=tmp->pRenderWires;node;node=node->pNext3)
   node->RenderAlpha(); //przezroczyste modele
 }
 return true;
}

//---------------------------------------------------------------------------
void __fastcall TGround::Navigate(String ClassName,UINT Msg,WPARAM wParam,LPARAM lParam)
{//wys³anie komunikatu do steruj¹cego
 HWND h=FindWindow(ClassName.c_str(),0); //mo¿na by to zapamiêtaæ
 WriteLog(AnsiString(SendMessage(h,Msg,wParam,lParam)));
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
{//Ra: jeszcze do wyczyszczenia
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
//---------------------------------------------------------------------------


