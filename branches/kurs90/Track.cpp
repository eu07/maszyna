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

#include "Texture.h"
#include "Timer.h"
#include "usefull.h"
#include "Globals.h"
#include "Track.h"
#include "Ground.h"

#pragma package(smart_init)

//101206 Ra: trapezoidalne drogi i tory

__fastcall TSwitchExtension::TSwitchExtension()
{//na pocz�tku wszystko puste
 CurrentIndex=0;
 pNexts[0]=NULL; //wska�niki do kolejnych odcink�w ruchu
 pNexts[1]=NULL;
 pPrevs[0]=NULL;
 pPrevs[1]=NULL;
 fOffset1=fOffset2=fDesiredOffset1=fDesiredOffset2=0.0; //po�o�enie zasadnicze
 pOwner=NULL;
 pNextAnim=NULL;
 bMovement=false; //nie potrzeba przelicza� fOffset1
}
__fastcall TSwitchExtension::~TSwitchExtension()
{//nie ma nic do usuwania
}

__fastcall TTrack::TTrack()
{//tworzenie nowego odcinka ruchu
 pNext=pPrev=NULL; //s�siednie
 Segment=NULL; //dane odcinka
 SwitchExtension=NULL; //dodatkowe parametry zwrotnicy i obrotnicy
 TextureID1=0; //tekstura szyny
 fTexLength=4.0; //powtarzanie tekstury
 TextureID2=0; //tekstura podsypki albo drugiego toru zwrotnicy
 fTexHeight=0.6; //nowy profil podsypki ;)
 fTexWidth=0.9;
 fTexSlope=0.9;
 iCategoryFlag=1; //1-tor, 2-droga, 4-rzeka, 8-samolot?
 fTrackWidth=1.435; //rozstaw toru, szeroko�� nawierzchni
 fFriction=0.15; //wsp�czynnik tarcia
 fSoundDistance=-1;
 iQualityFlag=20;
 iDamageFlag=0;
 eEnvironment=e_flat;
 bVisible=true;
 Event0=NULL;
 Event1=NULL;
 Event2=NULL;
 Eventall0=NULL;
 Eventall1=NULL;
 Eventall2=NULL;
 fVelocity=-1; //ograniczenie pr�dko�ci
 fTrackLength=100.0;
 fRadius=0; //promie� wybranego toru zwrotnicy
 fRadiusTable[0]=0; //dwa promienie nawet dla prostego
 fRadiusTable[1]=0;
 iNumDynamics=0;
 ScannedFlag=false;
 DisplayListID=0;
 iTrapezoid=0; //parametry kszta�tu: 0-standard, 1-przechy�ka, 2-trapez, 3-oba
 pTraction=NULL; //drut zasilaj�cy najbli�szy punktu 1 toru
 fTexRatio=1.0; //proporcja bok�w nawierzchni (�eby zaoszcz�dzi� na rozmiarach tekstur...)
}

__fastcall TTrack::~TTrack()
{//likwidacja odcinka
 if (eType==tt_Normal)
  delete Segment; //dla zwrotnic nie usuwa� tego (kopiowany)
 else
  SafeDelete(SwitchExtension);
}

void __fastcall TTrack::Init()
{//tworzenie pomocniczych danych
 switch (eType)
 {
  case tt_Switch:
   SwitchExtension=new TSwitchExtension();
  break;
  case tt_Normal:
   Segment=new TSegment();
  break;
  case tt_Turn: //oba potrzebne
   SwitchExtension=new TSwitchExtension();
   Segment=new TSegment();
  break;
 }
}

void __fastcall TTrack::ConnectPrevPrev(TTrack *pTrack)
{//�aczenie tor�w
 if (pTrack)
 {
  pPrev=pTrack;
  bPrevSwitchDirection=true;
  pTrack->pPrev=this;
  pTrack->bPrevSwitchDirection=true;
 }
}
void __fastcall TTrack::ConnectPrevNext(TTrack *pTrack)
{
 if (pTrack)
 {
  pPrev=pTrack;
  bPrevSwitchDirection=false;
  pTrack->pNext=this;
  pTrack->bNextSwitchDirection=false;
  if (bVisible)
   if (pTrack->bVisible)
    if (eType==tt_Normal) //je�li ��czone s� dwa normalne
     if (pTrack->eType==tt_Normal)
      if ((fTrackWidth!=pTrack->fTrackWidth) //Ra: je�li kolejny ma inne wymiary
       || (fTexHeight!=pTrack->fTexWidth)
       || (fTexWidth!=pTrack->fTexWidth)
       || (fTexSlope!=pTrack->fTexSlope))
       pTrack->iTrapezoid|=2; //to rysujemy potworka
 }
}
void __fastcall TTrack::ConnectNextPrev(TTrack *pTrack)
{
 if (pTrack)
 {
  pNext=pTrack;
  bNextSwitchDirection=false;
  pTrack->pPrev=this;
  pTrack->bPrevSwitchDirection=false;
  if (bVisible)
   if (pTrack->bVisible)
    if (eType==tt_Normal) //je�li ��czone s� dwa normalne
     if (pTrack->eType==tt_Normal)
      if ((fTrackWidth!=pTrack->fTrackWidth) //Ra: je�li kolejny ma inne wymiary
       || (fTexHeight!=pTrack->fTexWidth)
       || (fTexWidth!=pTrack->fTexWidth)
       || (fTexSlope!=pTrack->fTexSlope))
       iTrapezoid|=2; //to rysujemy potworka
 }
}
void __fastcall TTrack::ConnectNextNext(TTrack *pTrack)
{
 if (pTrack)
 {
  pNext=pTrack;
  bNextSwitchDirection=true;
  pTrack->pNext=this;
  pTrack->bNextSwitchDirection=true;
 }
}

vector3 __fastcall MakeCPoint(vector3 p,double d,double a1,double a2)
{
 vector3 cp=vector3(0,0,1);
 cp.RotateX(DegToRad(a2));
 cp.RotateY(DegToRad(a1));
 cp=cp*d+p;
 return cp;
}

vector3 __fastcall LoadPoint(cParser *parser)
{//pobranie wsp�rz�dnych punktu
 vector3 p;
 std::string token;
 parser->getTokens(3);
 *parser >> p.x >> p.y >> p.z;
 return p;
}

void __fastcall TTrack::Load(cParser *parser,vector3 pOrigin)
{//pobranie obiektu trajektorii ruchu
 vector3 pt,vec,p1,p2,cp1,cp2;
 double a1,a2,r1,r2,d1,d2,a;
 AnsiString str;
 bool bCurve;
 int i;//,state; //Ra: teraz ju� nie ma pocz�tkowego stanu zwrotnicy we wpisie
 std::string token;

 parser->getTokens();
 *parser >> token;
 str=AnsiString(token.c_str());

 if (str=="normal")
 {
  eType=tt_Normal;
  iCategoryFlag=1;
 }
 else if (str=="switch")
 {
  eType=tt_Switch;
  iCategoryFlag=1;
 }
 else if (str=="turn")
 {//Ra: to jest obrotnica
  eType=tt_Turn;
  iCategoryFlag=1;
 }
 else if (str=="road")
 {
  eType=tt_Normal;
  iCategoryFlag=2;
 }
 else if (str=="cross")
 {//Ra: to b�dzie skrzy�owanie dr�g
  eType=tt_Cross;
  iCategoryFlag=2;
 }
 else if (str=="river")
 {eType=tt_Normal;
  iCategoryFlag=4;
 }
 else
  eType=tt_Unknown;
 if (DebugModeFlag)
  WriteLog(str.c_str());
 parser->getTokens(4);
 *parser >> fTrackLength >> fTrackWidth >> fFriction >> fSoundDistance;
//    fTrackLength=Parser->GetNextSymbol().ToDouble();                       //track length 100502
//    fTrackWidth=Parser->GetNextSymbol().ToDouble();                        //track width
//    fFriction=Parser->GetNextSymbol().ToDouble();                          //friction coeff.
//    fSoundDistance=Parser->GetNextSymbol().ToDouble();   //snd
 parser->getTokens(2);
 *parser >> iQualityFlag >> iDamageFlag;
//    iQualityFlag=Parser->GetNextSymbol().ToInt();   //McZapkie: qualityflag
//    iDamageFlag=Parser->GetNextSymbol().ToInt();   //damage
 parser->getTokens();
 *parser >> token;
 str=AnsiString(token.c_str());  //environment
 if (str=="flat")
  eEnvironment=e_flat;
 else if (str=="mountains" || str=="mountain")
  eEnvironment=e_mountains;
 else if (str=="canyon")
  eEnvironment=e_canyon;
 else if (str=="tunnel")
  eEnvironment=e_tunnel;
 else if (str=="bridge")
  eEnvironment=e_bridge;
 else if (str=="bank")
  eEnvironment=e_bank;
 else
 {
  eEnvironment=e_unknown;
  Error("Unknown track environment: \""+str+"\"");
 }
 parser->getTokens();
 *parser >> token;
 bVisible=(token.compare( "vis" )==0);   //visible
 if (bVisible)
  {
   parser->getTokens();
   *parser >> token;
   str=AnsiString(token.c_str());   //railtex
   TextureID1=(str=="none"?0:TTexturesManager::GetTextureID(str.c_str(),(iCategoryFlag==1)?Global::iRailProFiltering:Global::iBallastFiltering));
   parser->getTokens();
   *parser >> fTexLength; //tex tile length
   parser->getTokens();
   *parser >> token;
   str=AnsiString(token.c_str());   //sub || railtex
   TextureID2=(str=="none"?0:TTexturesManager::GetTextureID(str.c_str(),(eType==tt_Normal)?Global::iBallastFiltering:Global::iRailProFiltering));
   parser->getTokens(3);
   *parser >> fTexHeight >> fTexWidth >> fTexSlope;
//      fTexHeight=Parser->GetNextSymbol().ToDouble(); //tex sub height
//      fTexWidth=Parser->GetNextSymbol().ToDouble(); //tex sub width
//      fTexSlope=Parser->GetNextSymbol().ToDouble(); //tex sub slope width
   if (iCategoryFlag==4)
    fTexHeight=-fTexHeight; //rzeki maj� wysoko�� odwrotnie ni� drogi
  }
 else
  if (DebugModeFlag) WriteLog("unvis");
 Init();
 double segsize=5.0f; //d�ugo�� odcinka segmentowania
 switch (eType)
 {//Ra: �uki segmentowane co 5m albo 314-k�tem foremnym
  case tt_Turn: //obrotnica jest prawie jak zwyk�y tor
  case tt_Normal:
   p1=LoadPoint(parser)+pOrigin; //pobranie wsp�rz�dnych P1
   parser->getTokens();
   *parser >> r1; //pobranie przechy�ki w P1
   cp1=LoadPoint(parser); //pobranie wsp�rz�dnych punkt�w kontrolnych
   cp2=LoadPoint(parser);
   p2=LoadPoint(parser)+pOrigin; //pobranie wsp�rz�dnych P2
   parser->getTokens(2);
   *parser >> r2 >> fRadius; //pobranie przechy�ki w P1 i promienia

   if (fRadius!=0) //gdy podany promie�
      segsize=Min0R(5.0,0.3+fabs(fRadius)*0.03); //do 250m - 5, potem 1 co 50m

   if ((((p1+p1+p2)/3.0-p1-cp1).Length()<0.02)||(((p1+p2+p2)/3.0-p2+cp1).Length()<0.02))
    cp1=cp2=vector3(0,0,0); //"prostowanie" prostych z kontrolnymi, dok�adno�� 2cm

   if ((cp1==vector3(0,0,0)) && (cp2==vector3(0,0,0))) //Ra: hm, czasem dla prostego s� podane...
    Segment->Init(p1,p2,segsize,r1,r2); //gdy prosty, kontrolne wyliczane przy zmiennej przechy�ce
   else
    Segment->Init(p1,cp1+p1,cp2+p2,p2,segsize,r1,r2); //gdy �uk (ustawia bCurve=true)
   if ((r1!=0)||(r2!=0)) iTrapezoid=1; //s� przechy�ki do uwzgl�dniania w rysowaniu
   if (eType==tt_Turn) //obrotnica ma doklejk�
   {SwitchExtension=new TSwitchExtension(); //zwrotnica ma doklejk�
    SwitchExtension->Segments->Init(p1,p2,segsize); //kopia oryginalnego toru
   }
   else if (iCategoryFlag==2)
    if (TextureID1&&fTexLength)
    {//dla drogi trzeba ustali� proporcje bok�w nawierzchni
     float w,h;
     glBindTexture(GL_TEXTURE_2D,TextureID1);
     glGetTexLevelParameterfv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&w);
     glGetTexLevelParameterfv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&h);
     if (h!=0.0) fTexRatio=w/h; //proporcja bok�w
    }
  break;

  case tt_Cross: //skrzy�owanie dr�g - 4 punkty z wektorami kontrolnymi
  case tt_Switch: //zwrotnica
   //problemy z animacj� iglic powstaje, gdzy odcinek prosty ma zmienn� przechy�k�
   //wtedy dzieli si� na dodatkowe odcinki (po 0.2m, bo R=0) i animacj� diabli bior�
   //Ra: na razie nie podejmuj� si� przerabiania iglic

   SwitchExtension=new TSwitchExtension(); //zwrotnica ma doklejk�

   p1=LoadPoint(parser)+pOrigin; //pobranie wsp�rz�dnych P1
   parser->getTokens();
   *parser >> r1;
   cp1=LoadPoint(parser);
   cp2=LoadPoint(parser);
   p2=LoadPoint(parser)+pOrigin; //pobranie wsp�rz�dnych P2
   parser->getTokens(2);
   *parser >> r2 >> fRadiusTable[0];

   if (fRadiusTable[0]>0)
      segsize=Min0R(5.0,0.2+fRadiusTable[0]*0.02);
   else
   {cp1=(p1+p1+p2)/3.0-p1; cp2=(p1+p2+p2)/3.0-p2; segsize=5.0;} //u�omny prosty

   if (!(cp1==vector3(0,0,0)) && !(cp2==vector3(0,0,0)))
       SwitchExtension->Segments[0].Init(p1,cp1+p1,cp2+p2,p2,segsize,r1,r2);
   else
       SwitchExtension->Segments[0].Init(p1,p2,segsize,r1,r2);

   p1=LoadPoint(parser)+pOrigin; //pobranie wsp�rz�dnych P3
   parser->getTokens();
   *parser >> r1;
   cp1=LoadPoint(parser);
   cp2=LoadPoint(parser);
   p2=LoadPoint(parser)+pOrigin; //pobranie wsp�rz�dnych P4
   parser->getTokens(2);
   *parser >> r2 >> fRadiusTable[1];

   if (fRadiusTable[1]>0)
      segsize=Min0R(5.0,0.2+fRadiusTable[1]*0.02);
   else
      segsize=5.0;

   if (!(cp1==vector3(0,0,0)) && !(cp2==vector3(0,0,0)))
       SwitchExtension->Segments[1].Init(p1,cp1+p1,cp2+p2,p2,segsize,r1,r2);
   else
       SwitchExtension->Segments[1].Init(p1,p2,segsize,r1,r2);

   Switch(0); //na sta�e w po�o�eniu 0 - nie ma pocz�tkowego stanu zwrotnicy we wpisie

   //Ra: zamieni� p�niej na iloczyn wektorowy
   {vector3 v1,v2;
    double a1,a2;
    v1=SwitchExtension->Segments[0].FastGetPoint_1()-SwitchExtension->Segments[0].FastGetPoint_0();
    v2=SwitchExtension->Segments[1].FastGetPoint_1()-SwitchExtension->Segments[1].FastGetPoint_0();
    a1=atan2(v1.x,v1.z);
    a2=atan2(v2.x,v2.z);
    a2=a2-a1;
    while (a2>M_PI) a2=a2-2*M_PI;
    while (a2<-M_PI) a2=a2+2*M_PI;
    SwitchExtension->RightSwitch=a2<0; //lustrzany uk�ad OXY...
   }
  break;
 }
 parser->getTokens();
 *parser >> token;
 str=AnsiString(token.c_str());
 while (str!="endtrack")
 {
  if (str=="event0")
   {
      parser->getTokens();
      *parser >> token;
      asEvent0Name=AnsiString(token.c_str());
   }
  else
  if (str=="event1")
   {
      parser->getTokens();
      *parser >> token;
      asEvent1Name=AnsiString(token.c_str());
   }
  else
  if (str=="event2")
   {
      parser->getTokens();
      *parser >> token;
      asEvent2Name=AnsiString(token.c_str());
   }
  else
  if (str=="eventall0")
   {
      parser->getTokens();
      *parser >> token;
      asEventall0Name=AnsiString(token.c_str());
   }
  else
  if (str=="eventall1")
   {
      parser->getTokens();
      *parser >> token;
      asEventall1Name=AnsiString(token.c_str());
   }
  else
  if (str=="eventall2")
   {
      parser->getTokens();
      *parser >> token;
      asEventall2Name=AnsiString(token.c_str());
   }
  else
  if (str=="velocity")
   {
     parser->getTokens();
     *parser >> fVelocity;
//         fVelocity=Parser->GetNextSymbol().ToDouble(); //*0.28; McZapkie-010602
   }
  else
   Error("Unknown track property: \""+str+"\"");
  parser->getTokens(); *parser >> token;
  str=AnsiString(token.c_str());
 }
}

bool __fastcall TTrack::AssignEvents(TEvent *NewEvent0,TEvent *NewEvent1,TEvent *NewEvent2)
{
    bool bError=false;
    if (!Event0)
    {
        if (NewEvent0)
        {
            Event0=NewEvent0;
            asEvent0Name="";
        }
        else
        {
            if (!asEvent0Name.IsEmpty())
            {
                Error(AnsiString("Event0 \"")+asEvent0Name+AnsiString("\" does not exist"));
                bError=true;
            }
        }
    }
    else
    {
        Error(AnsiString("Event 0 cannot be assigned to track, track already has one"));
        bError=true;
    }

    if (!Event1)
    {
        if (NewEvent1)
        {
            Event1=NewEvent1;
            asEvent1Name="";
        }
        else
        {
            if (!asEvent0Name.IsEmpty())
            {//Ra: tylko w logu informacja
                WriteLog(AnsiString("Event1 \"")+asEvent1Name+AnsiString("\" does not exist").c_str());
                bError=true;
            }
        }
    }
    else
    {
        Error(AnsiString("Event 1 cannot be assigned to track, track already has one"));
        bError=true;
    }

    if (!Event2)
    {
        if (NewEvent2)
        {
            Event2=NewEvent2;
            asEvent2Name="";
        }
        else
        {
            if (!asEvent0Name.IsEmpty())
            {//Ra: tylko w logu informacja
                WriteLog(AnsiString("Event2 \"")+asEvent2Name+AnsiString("\" does not exist"));
                bError=true;
            }
        }
    }
    else
    {
        Error(AnsiString("Event 2 cannot be assigned to track, track already has one"));
        bError=true;
    }
    return !bError;
}

bool __fastcall TTrack::AssignallEvents(TEvent *NewEvent0,TEvent *NewEvent1,TEvent *NewEvent2)
{
    bool bError=false;
    if (!Eventall0)
    {
        if (NewEvent0)
        {
            Eventall0=NewEvent0;
            asEventall0Name="";
        }
        else
        {
            if (!asEvent0Name.IsEmpty())
            {
                Error(AnsiString("Eventall 0 \"")+asEventall0Name+AnsiString("\" does not exist"));
                bError=true;
            }
        }
    }
    else
    {
        Error(AnsiString("Eventall 0 cannot be assigned to track, track already has one"));
        bError=true;
    }

    if (!Eventall1)
    {
        if (NewEvent1)
        {
            Eventall1=NewEvent1;
            asEventall1Name="";
        }
        else
        {
            if (!asEvent0Name.IsEmpty())
            {//Ra: tylko w logu informacja
                WriteLog(AnsiString("Eventall 1 \"")+asEventall1Name+AnsiString("\" does not exist"));
                bError=true;
            }
        }
    }
    else
    {
        Error(AnsiString("Eventall 1 cannot be assigned to track, track already has one"));
        bError=true;
    }

    if (!Eventall2)
    {
        if (NewEvent2)
        {
            Eventall2=NewEvent2;
            asEventall2Name="";
        }
        else
        {
            if (!asEvent0Name.IsEmpty())
            {//Ra: tylko w logu informacja
                WriteLog(AnsiString("Eventall 2 \"")+asEventall2Name+AnsiString("\" does not exist"));
                bError=true;
            }
        }
    }
    else
    {
        Error(AnsiString("Eventall 2 cannot be assigned to track, track already has one"));
        bError=true;
    }
    return !bError;
}


//ABu: przeniesione z Track.h i poprawione!!!
bool __fastcall TTrack::AddDynamicObject(TDynamicObject *Dynamic)
{
    if (iNumDynamics<iMaxNumDynamics)
    {
        Dynamics[iNumDynamics]=Dynamic;
        iNumDynamics++;
        Dynamic->MyTrack=this; //ABu: Na ktorym torze jestesmy.
        return true;
    }
    else
    {
        Error("Cannot add dynamic to track");
        return false;
    }
};

void __fastcall TTrack::MoveMe(vector3 pPosition)
{
    if(SwitchExtension)
    {
        SwitchExtension->Segments[0].MoveMe(1*pPosition);
        SwitchExtension->Segments[1].MoveMe(1*pPosition);
        SwitchExtension->Segments[2].MoveMe(3*pPosition); //Ra: 3 razy?
        SwitchExtension->Segments[3].MoveMe(4*pPosition);
    }
    else
    {
       Segment->MoveMe(pPosition);
    };
    ResourceManager::Unregister(this);
};


const int numPts=4;
const int nnumPts=12;
const vector6 szyna[nnumPts]= //szyna - vextor3(x,y,mapowanie tekstury)
{vector6( 0.111,0.000,0.00, 1.000, 0.000,0.000),
 vector6( 0.045,0.025,0.15, 0.707, 0.707,0.000),
 vector6( 0.045,0.110,0.25, 0.707,-0.707,0.000),
 vector6( 0.071,0.140,0.35, 0.707,-0.707,0.000), //albo tu 0.073
 vector6( 0.072,0.170,0.40, 0.707, 0.707,0.000),
 vector6( 0.052,0.180,0.45, 0.000, 1.000,0.000),
 vector6( 0.020,0.180,0.55, 0.000, 1.000,0.000),
 vector6( 0.000,0.170,0.60,-0.707, 0.707,0.000),
 vector6( 0.001,0.140,0.65,-0.707,-0.707,0.000), //albo tu -0.001
 vector6( 0.027,0.110,0.75,-0.707,-0.707,0.000), //albo zostanie asymetryczna
 vector6( 0.027,0.025,0.85,-0.707, 0.707,0.000),
 vector6(-0.039,0.000,1.00,-1.000, 0.000,0.000)
};
const vector6 iglica[nnumPts]= //iglica - vextor3(x,y,mapowanie tekstury)
{vector6( 0.010,0.000,0.00, 1.000, 0.000,0.000),
 vector6( 0.010,0.025,0.15, 1.000, 0.000,0.000),
 vector6( 0.010,0.110,0.25, 1.000, 0.000,0.000),
 vector6( 0.010,0.140,0.35, 1.000, 0.000,0.000),
 vector6( 0.010,0.170,0.40, 1.000, 0.000,0.000),
 vector6( 0.010,0.180,0.45, 0.707, 0.707,0.000),
 vector6( 0.000,0.180,0.55, 0.707, 0.707,0.000),
 vector6( 0.000,0.170,0.60,-1.000, 0.000,0.000),
 vector6( 0.000,0.140,0.65,-1.000, 0.000,0.000),
 vector6( 0.000,0.110,0.75,-1.000, 0.000,0.000),
 vector6( 0.000,0.025,0.85,-0.707, 0.707,0.000),
 vector6(-0.040,0.000,1.00,-1.000, 0.000,0.000) //1mm wi�cej, �eby nie nachodzi�y tekstury?
};



void __fastcall TTrack::Compile()
{//przygotowanie tr�jk�t�w do wy�wielenia - model proceduralny
 if (DisplayListID)
     Release(); //zwolnienie zasob�w

 if (Global::bManageNodes)
 {
     DisplayListID=glGenLists(1); //otwarcie nowej listy
     glNewList(DisplayListID,GL_COMPILE);
 };

 glColor3f(1.0f,1.0f,1.0f);

 //McZapkie-310702: zmiana oswietlenia w tunelu, wykopie
 GLfloat  ambientLight[4] ={0.5f,0.5f,0.5f,1.0f};
 GLfloat  diffuseLight[4] ={0.5f,0.5f,0.5f,1.0f};
 GLfloat  specularLight[4]={0.5f,0.5f,0.5f,1.0f};

 switch (eEnvironment)
 {//modyfikacje o�wietlenia zale�nie od �rodowiska
     case e_canyon: //wykop
         for(int li=0; li<3; li++)
         {
             ambientLight[li]=Global::ambientDayLight[li]*0.7;
             diffuseLight[li]=Global::diffuseDayLight[li]*0.3;
             specularLight[li]=Global::specularDayLight[li]*0.4;
         }
         glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);
         glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);
         glLightfv(GL_LIGHT0,GL_SPECULAR,specularLight);
     break;
     case e_tunnel: //tunel
         for(int li=0; li<3; li++)
         {
             ambientLight[li]=Global::ambientDayLight[li]*0.2;
             diffuseLight[li]=Global::diffuseDayLight[li]*0.1;
             specularLight[li]=Global::specularDayLight[li]*0.2;
         }
         glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);
         glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);
         glLightfv(GL_LIGHT0,GL_SPECULAR,specularLight);
     break;
 }

 double fHTW=0.5*fabs(fTrackWidth); //po�owa szeroko�ci
 double side=fabs(fTexWidth); //szerok�� podsypki na zewn�trz szyny albo pobocza
 double rozp=fHTW+side+fabs(fTexSlope); //brzeg zewn�trzny
 double hypot1=hypot(fTexSlope,fTexHeight); //rozmiar pochylenia do liczenia normalnych
 if (hypot1==0.0) hypot1=1.0;
 vector3 normal1=vector3(fTexSlope/hypot1,fTexHeight/hypot1,0.0); //wektor normalny
 double fHTW2,side2,rozp2,fTexHeight2,hypot2;
 vector3 normal2;
 if (iTrapezoid&2) //ten bit oznacza, �e istnieje odpowiednie pNext
 {//Ra: na tym si� lubi wiesza�, ciekawe co za �mieci si� pod��czaj�...
  fHTW2=0.5*fabs(pNext->fTrackWidth); //po�owa rozstawu/nawierzchni
  side2=fabs(pNext->fTexWidth);
  rozp2=fHTW2+side2+fabs(pNext->fTexSlope);
  fTexHeight2=pNext->fTexHeight;
  hypot2=hypot(pNext->fTexSlope,pNext->fTexHeight);
  if (hypot2==0.0) hypot2=1.0;
  normal2=vector3(pNext->fTexSlope/hypot2,fTexHeight2/hypot2,0.0);
  //zabezpieczenia przed zawieszeniem - logowa� to?
  if (fHTW2>5.0*fHTW) {fHTW2=fHTW; WriteLog("niedopasowanie 1");};
  if (side2>5.0*side) {side2=side; WriteLog("niedopasowanie 2");};
  if (rozp2>5.0*rozp) {rozp2=rozp; WriteLog("niedopasowanie 3");};
  //if (fabs(fTexHeight2)>5.0*fabs(fTexHeight)) {fTexHeight2=fTexHeight; WriteLog("niedopasowanie 4");};
 }
 else //gdy nie ma nast�pnego albo jest nieodpowiednim ko�cem podpi�ty
 {fHTW2=fHTW; side2=side; rozp2=rozp; fTexHeight2=fTexHeight; normal2=normal1;}
 double roll1,roll2;
 switch (iCategoryFlag)
 {
  case 1:   //tor
  {
   Segment->GetRolls(roll1,roll2);
   double sin1=sin(roll1),cos1=cos(roll1),sin2=sin(roll2),cos2=cos(roll2);
   // zwykla szyna: //Ra: czemu g��wki s� asymetryczne na wysoko�ci 0.140?
   vector6 rpts1[24],rpts2[24],rpts3[24],rpts4[24];
   int i;
   for (i=0;i<12;++i)
   {rpts1[i]   =vector6(( fHTW+szyna[i].x)*cos1+szyna[i].y*sin1,-( fHTW+szyna[i].x)*sin1+szyna[i].y*cos1,szyna[i].z ,+szyna[i].n.x*cos1+szyna[i].n.y*sin1,-szyna[i].n.x*sin1+szyna[i].n.y*cos1,0.0);
    rpts2[11-i]=vector6((-fHTW-szyna[i].x)*cos1+szyna[i].y*sin1,-(-fHTW-szyna[i].x)*sin1+szyna[i].y*cos1,szyna[i].z ,-szyna[i].n.x*cos1+szyna[i].n.y*sin1,+szyna[i].n.x*sin1+szyna[i].n.y*cos1,0.0);
   }
   if (iTrapezoid) //trapez albo przechy�ki, to oddzielne punkty na ko�cu
    for (i=0;i<12;++i)
    {rpts1[12+i]=vector6(( fHTW2+szyna[i].x)*cos2+szyna[i].y*sin2,-( fHTW2+szyna[i].x)*sin2+szyna[i].y*cos2,szyna[i].z ,+szyna[i].n.x*cos2+szyna[i].n.y*sin2,-szyna[i].n.x*sin2+szyna[i].n.y*cos2,0.0);
     rpts2[23-i]=vector6((-fHTW2-szyna[i].x)*cos2+szyna[i].y*sin2,-(-fHTW2-szyna[i].x)*sin2+szyna[i].y*cos2,szyna[i].z ,-szyna[i].n.x*cos2+szyna[i].n.y*sin2,+szyna[i].n.x*sin2+szyna[i].n.y*cos2,0.0);
    }
   switch (eType) //dalej zale�nie od typu
   {
    case tt_Turn: //obrotnica jak zwyk�y tor, tylko animacja dochodzi
     if (InMovement()) //je�li si� kr�ci
     {//wyznaczamy wsp�rz�dne ko�c�w, przy za�o�eniu sta�ego �r�dka i d�ugo�ci
      double hlen=0.5*SwitchExtension->Segments->GetLength(); //po�owa d�ugo�ci
      //SwitchExtension->fOffset1=SwitchExtension->pAnim?SwitchExtension->pAnim->AngleGet():0.0; //pobranie k�ta z modelu
      TAnimContainer *ac=SwitchExtension->pModel?SwitchExtension->pModel->GetContainer(NULL):NULL;
      if (ac)
       SwitchExtension->fOffset1=ac?180+ac->AngleGet():0.0; //pobranie k�ta z modelu
      double sina=hlen*sin(DegToRad(SwitchExtension->fOffset1)),cosa=hlen*cos(DegToRad(SwitchExtension->fOffset1));
      vector3 middle=SwitchExtension->Segments->FastGetPoint(0.5);
      Segment->Init(middle+vector3(sina,0.0,cosa),middle-vector3(sina,0.0,cosa),5.0);
     }
    case tt_Normal:
     if (TextureID2)
     {//podsypka z podk�adami jest tylko dla zwyk�ego toru
      vector6 bpts1[8]; //punkty g��wnej p�aszczyzny nie przydaj� si� do robienia bok�w
      if (iTrapezoid) //trapez albo przechy�ki
      {//podsypka z podkladami trapezowata
       //ewentualnie poprawi� mapowanie, �eby �rodek mapowa� si� na 1.435/4.671 ((0.3464,0.6536)
       //bo si� tekstury podsypki rozje�d�aj� po zmianie proporcji profilu
       bpts1[0]=vector6(rozp,-fTexHeight,0.0,normal1.x,-normal1.y,0.0); //lewy brzeg
       bpts1[1]=vector6((fHTW+side)*cos1,-(fHTW+side)*sin1,0.33,0.0,1.0,0.0); //kraw�d� za�amania
       bpts1[2]=vector6(-bpts1[1].x,-bpts1[1].y,0.67,-normal1.x,-normal1.y,0.0); //prawy brzeg pocz�tku symetrycznie
       bpts1[3]=vector6(-rozp,-fTexHeight,1.0,-normal1.x,-normal1.y,0.0); //prawy skos
       bpts1[4]=vector6(rozp2,-fTexHeight2,0.0,normal2.x,-normal2.y,0.0); //lewy brzeg
       bpts1[5]=vector6((fHTW2+side2)*cos2,-(fHTW2+side2)*sin2,0.33,0.0,1.0,0.0); //kraw�d� za�amania
       bpts1[6]=vector6(-bpts1[5].x,-bpts1[5].y,0.67,0.0,1.0,0.0); //prawy brzeg pocz�tku symetrycznie
       bpts1[7]=vector6(-rozp2,-fTexHeight2,1.0,-normal2.x,-normal2.y,0.0); //prawy skos
      }
      else
      {bpts1[0]=vector6(rozp,-fTexHeight,0.0,normal1.x,-normal1.y,0.0); //lewy brzeg
       bpts1[1]=vector6(fHTW+side,0.0,0.33,normal1.x,-normal1.y,0.0); //kraw�d� za�amania
       bpts1[2]=vector6(-fHTW-side,0.0,0.67,-normal1.x,-normal1.y,0.0); //druga
       bpts1[3]=vector6(-rozp,-fTexHeight,1.0,-normal1.x,-normal1.y,0.0); //prawy skos
      }
      glBindTexture(GL_TEXTURE_2D,TextureID2);
      Segment->RenderLoft(bpts1,iTrapezoid?-4:4,fTexLength);
     }
     if (TextureID1)
     {// szyny
      glBindTexture(GL_TEXTURE_2D,TextureID1);
      Segment->RenderLoft(rpts1,iTrapezoid?-nnumPts:nnumPts,fTexLength);
      Segment->RenderLoft(rpts2,iTrapezoid?-nnumPts:nnumPts,fTexLength);
     }
     break;
    case tt_Switch: //dla zwrotnicy dwa razy szyny
     if (TextureID1)
     {//iglice liczone tylko dla zwrotnic
      vector6 rpts3[24],rpts4[24];
      for (i=0;i<12;++i)
      {rpts3[i]   =vector6(( fHTW+iglica[i].x)*cos1+iglica[i].y*sin1,-( fHTW+iglica[i].x)*sin1+iglica[i].y*cos1,iglica[i].z,+iglica[i].n.x*cos1+iglica[i].n.y*sin1,-iglica[i].n.x*sin1+iglica[i].n.y*cos1,0.0);
       rpts4[11-i]=vector6((-fHTW-iglica[i].x)*cos1+iglica[i].y*sin1,-(-fHTW-iglica[i].x)*sin1+iglica[i].y*cos1,iglica[i].z,-iglica[i].n.x*cos1+iglica[i].n.y*sin1,+iglica[i].n.x*sin1+iglica[i].n.y*cos1,0.0);
      }
      if (iTrapezoid) //trapez albo przechy�ki, to oddzielne punkty na ko�cu
       for (i=0;i<12;++i)
       {rpts3[12+i]=vector6(( fHTW2+iglica[i].x)*cos2+iglica[i].y*sin2,-( fHTW2+iglica[i].x)*sin2+iglica[i].y*cos2,iglica[i].z,+iglica[i].n.x*cos2+iglica[i].n.y*sin2,-iglica[i].n.x*sin2+iglica[i].n.y*cos2,0.0);
        rpts4[23-i]=vector6((-fHTW2-iglica[i].x)*cos2+iglica[i].y*sin2,-(-fHTW2-iglica[i].x)*sin2+iglica[i].y*cos2,iglica[i].z,-iglica[i].n.x*cos2+iglica[i].n.y*sin2,+iglica[i].n.x*sin2+iglica[i].n.y*cos2,0.0);
       }
      if (InMovement())
      {//Ra: troch� bez sensu, �e tu jest animacja
       double v=SwitchExtension->fDesiredOffset1-SwitchExtension->fOffset1;
       SwitchExtension->fOffset1+=sign(v)*Timer::GetDeltaTime()*0.1;
       //Ra: trzeba da� to do klasy...
       if (SwitchExtension->fOffset1<=0.00)
       {SwitchExtension->fOffset1; //1cm?
        SwitchExtension->bMovement=false; //koniec animacji
       }
       if (SwitchExtension->fOffset1>=fMaxOffset)
       {SwitchExtension->fOffset1=fMaxOffset; //maksimum-1cm?
        SwitchExtension->bMovement=false; //koniec animacji
       }
      }
      //McZapkie-130302 - poprawione rysowanie szyn
      if (SwitchExtension->RightSwitch)
      {//nowa wersja z SPKS, ale odwrotnie lewa/prawa
       glBindTexture(GL_TEXTURE_2D,TextureID1);
       SwitchExtension->Segments[0].RenderLoft(rpts1,nnumPts,fTexLength,2);
       SwitchExtension->Segments[0].RenderSwitchRail(rpts1,rpts3,nnumPts,fTexLength,2,SwitchExtension->fOffset1);
       SwitchExtension->Segments[0].RenderLoft(rpts2,nnumPts,fTexLength);
       glBindTexture(GL_TEXTURE_2D,TextureID2);
       SwitchExtension->Segments[1].RenderLoft(rpts1,nnumPts,fTexLength);
       SwitchExtension->Segments[1].RenderLoft(rpts2,nnumPts,fTexLength,2);
       SwitchExtension->Segments[1].RenderSwitchRail(rpts2,rpts4,nnumPts,fTexLength,2,-fMaxOffset+SwitchExtension->fOffset1);
       //WriteLog("Kompilacja prawej"); WriteLog(AnsiString(SwitchExtension->fOffset1).c_str());
      }
      else
      {//lewa dzia�a lepiej ni� prawa
       glBindTexture(GL_TEXTURE_2D,TextureID1);
       SwitchExtension->Segments[0].RenderLoft(rpts1,nnumPts,fTexLength); //lewa szyna normalna ca�a
       SwitchExtension->Segments[0].RenderLoft(rpts2,nnumPts,fTexLength,2); //prawa szyna za iglic�
       SwitchExtension->Segments[0].RenderSwitchRail(rpts2,rpts4,nnumPts,fTexLength,2,-SwitchExtension->fOffset1); //prawa iglica
       glBindTexture(GL_TEXTURE_2D,TextureID2);
       SwitchExtension->Segments[1].RenderLoft(rpts1,nnumPts,fTexLength,2); //lewa szyna za iglic�
       SwitchExtension->Segments[1].RenderSwitchRail(rpts1,rpts3,nnumPts,fTexLength,2,fMaxOffset-SwitchExtension->fOffset1); //lewa iglica
       SwitchExtension->Segments[1].RenderLoft(rpts2,nnumPts,fTexLength); //prawa szyna normalnie ca�a
       //WriteLog("Kompilacja lewej"); WriteLog(AnsiString(SwitchExtension->fOffset1).c_str());
      }
     }
     break;
   }
  } //koniec obs�ugi tor�w
  break;
  case 2:   //McZapkie-260302 - droga - rendering
  //McZapkie:240702-zmieniony zakres widzialnosci
  {vector6 bpts1[4]; //punkty g��wnej p�aszczyzny przydaj� si� do robienia bok�w
   if (TextureID1||TextureID2) //punkty si� przydadz�, nawet je�li nawierzchni nie ma
   {//double max=2.0*(fHTW>fHTW2?fHTW:fHTW2); //z szerszej strony jest 100%
    double max=fTexRatio*fTexLength; //test: szeroko�� proporcjonalna do d�ugo�ci
    double map1=max>0.0?fHTW/max:0.5; //obci�cie tekstury od strony 1
    double map2=max>0.0?fHTW2/max:0.5; //obci�cie tekstury od strony 2
    if (iTrapezoid) //trapez albo przechy�ki
    {//nawierzchnia trapezowata
     Segment->GetRolls(roll1,roll2);
     bpts1[0]=vector6(fHTW*cos(roll1),-fHTW*sin(roll1),0.5-map1,sin(roll1),cos(roll1),0.0); //lewy brzeg pocz�tku
     bpts1[1]=vector6(-bpts1[0].x,-bpts1[0].y,0.5+map1,-sin(roll1),cos(roll1),0.0); //prawy brzeg pocz�tku symetrycznie
     bpts1[2]=vector6(fHTW2*cos(roll2),-fHTW2*sin(roll2),0.5-map2,sin(roll2),cos(roll2),0.0); //lewy brzeg ko�ca
     bpts1[3]=vector6(-bpts1[2].x,-bpts1[2].y,0.5+map2,-sin(roll2),cos(roll2),0.0); //prawy brzeg pocz�tku symetrycznie
    }
    else
    {bpts1[0]=vector6( fHTW,0.0,0.5-map1,0.0,1.0,0.0);
     bpts1[1]=vector6(-fHTW,0.0,0.5+map1,0.0,1.0,0.0);
    }
   }
   if (TextureID1) //je�li podana by�a tekstura, generujemy tr�jk�ty
   {//tworzenie tr�jk�t�w nawierzchni szosy
    glBindTexture(GL_TEXTURE_2D,TextureID1);
    Segment->RenderLoft(bpts1,iTrapezoid?-2:2,fTexLength);
   }
   if (TextureID2)
   {//pobocze drogi - poziome przy przechy�ce (a mo�e kraw�nik i chodnik zrobi� jak w Midtown Madness 2?)
    glBindTexture(GL_TEXTURE_2D,TextureID2);
    vector6 rpts1[6],rpts2[6]; //wsp�rz�dne przekroju i mapowania dla prawej i lewej strony
    rpts1[0]=vector6(rozp,-fTexHeight,0.0); //lewy brzeg podstawy
    rpts1[1]=vector6(bpts1[0].x+side,bpts1[0].y,0.5), //lewa kraw�d� za�amania
    rpts1[2]=vector6(bpts1[0].x,bpts1[0].y,1.0); //lewy brzeg pobocza (mapowanie mo�e by� inne
    rpts2[0]=vector6(bpts1[1].x,bpts1[1].y,1.0); //prawy brzeg pobocza
    rpts2[1]=vector6(bpts1[1].x-side,bpts1[1].y,0.5); //prawa kraw�d� za�amania
    rpts2[2]=vector6(-rozp,-fTexHeight,0.0); //prawy brzeg podstawy
    if (iTrapezoid) //trapez albo przechy�ki
    {//pobocza do trapezowatej nawierzchni - dodatkowe punkty z drugiej strony odcinka
     rpts1[3]=vector6(rozp2,-fTexHeight2,0.0); //lewy brzeg lewego pobocza
     rpts1[4]=vector6(bpts1[2].x+side2,bpts1[2].y,0.5); //kraw�d� za�amania
     rpts1[5]=vector6(bpts1[2].x,bpts1[2].y,1.0); //brzeg pobocza
     rpts2[3]=vector6(bpts1[3].x,bpts1[3].y,1.0);
     rpts2[4]=vector6(bpts1[3].x-side2,bpts1[3].y,0.5);
     rpts2[5]=vector6(-rozp2,-fTexHeight2,0.0); //prawy brzeg prawego pobocza
     Segment->RenderLoft(rpts1,-3,fTexLength);
     Segment->RenderLoft(rpts2,-3,fTexLength);
    }
    else
    {//pobocza zwyk�e, brak przechy�ki
     Segment->RenderLoft(rpts1,3,fTexLength);
     Segment->RenderLoft(rpts2,3,fTexLength);
    }
   }
  }
  break;
  case 4:   //McZapkie-260302 - rzeka- rendering
   //Ra: rzeki na razie bez zmian, przechy�ki na pewno nie maj�
   vector6 bpts1[numPts]={vector6( fHTW,0.0,0.0), vector6( fHTW,0.2,0.33),
                          vector6(-fHTW,0.0,0.67),vector6(-fHTW,0.0,1.0 ) };
   //Ra: dziwnie ten kszta�t wygl�da
   if(TextureID1)
   {
    glBindTexture(GL_TEXTURE_2D,TextureID1);
    Segment->RenderLoft(bpts1,numPts,fTexLength);
    if(TextureID2)
    {//brzegi rzeki prawie jak pobocze derogi, tylko inny znak ma wysoko��
     //znak jest zmieniany przy wczytywaniu, wi�c tu musi byc minus fTexHeight
     vector6 rpts1[3]={ vector6(rozp,-fTexHeight,0.0),
                        vector6(fHTW+side,0.0,0.5),
                        vector6(fHTW,0.0,1.0) };
     vector6 rpts2[3]={ vector6(-fHTW,0.0,1.0),
                        vector6(-fHTW-side,0.0,0.5),
                        vector6(-rozp,-fTexHeight,0.1) }; //Ra: po kiego 0.1?
     glBindTexture(GL_TEXTURE_2D,TextureID2);      //brzeg rzeki
     Segment->RenderLoft(rpts1,3,fTexLength);
     Segment->RenderLoft(rpts2,3,fTexLength);
    }
   }
   break;
 }
 if(Global::bManageNodes)
  glEndList();
};

void TTrack::Release()
{
 if (DisplayListID)
  glDeleteLists(DisplayListID,1);
 DisplayListID=0;
};

void __fastcall TTrack::Render()
{

    if(bVisible && SquareMagnitude(Global::pCameraPosition-Segment->FastGetPoint(0.5)) < 810000)
    {
        if(!DisplayListID)
        {
            Compile();
            if(Global::bManageNodes)
                ResourceManager::Register(this);
        };
        SetLastUsage(Timer::GetSimulationTime());
        glCallList(DisplayListID);
        if (InMovement()) Release(); //zwrotnica w trakcie animacji do odrysowania
    };

    for (int i=0; i<iNumDynamics; i++)
    {
        Dynamics[i]->Render();
    }
#ifdef _DEBUG
            if (DebugModeFlag && ScannedFlag) //McZapkie-230702
            {
              vector3 pos1,pos2,pos3;
              glDisable(GL_DEPTH_TEST);
                  glDisable(GL_LIGHTING);
              glColor3ub(255,0,0);
              glBindTexture(GL_TEXTURE_2D,0);
              glBegin(GL_LINE_STRIP);
                  pos1=Segment->FastGetPoint_0();
                  pos2=Segment->FastGetPoint(0.5);
                  pos3=Segment->FastGetPoint_1();
                  glVertex3f(pos1.x,pos1.y,pos1.z);
                  glVertex3f(pos2.x,pos2.y+10,pos2.z);
                  glVertex3f(pos3.x,pos3.y,pos3.z);
              glEnd();
              glEnable(GL_LIGHTING);
              glEnable(GL_DEPTH_TEST);
            }
#endif
   ScannedFlag=false;
   glLightfv(GL_LIGHT0,GL_AMBIENT,Global::ambientDayLight);
   glLightfv(GL_LIGHT0,GL_DIFFUSE,Global::diffuseDayLight);
   glLightfv(GL_LIGHT0,GL_SPECULAR,Global::specularDayLight);
}

void __fastcall TTrack::RenderAlpha()
{
    glColor3f(1.0f,1.0f,1.0f);
//McZapkie-310702: zmiana oswietlenia w tunelu, wykopie
    GLfloat ambientLight[4]= {0.5f,0.5f,0.5f,1.0f};
    GLfloat diffuseLight[4]= {0.5f,0.5f,0.5f,1.0f};
    GLfloat specularLight[4]={0.5f,0.5f,0.5f,1.0f};
    switch (eEnvironment)
    {
     case e_canyon:
      {
        for (int li=0; li<3; li++)
         {
           ambientLight[li]= Global::ambientDayLight[li]*0.8;
           diffuseLight[li]= Global::diffuseDayLight[li]*0.4;
           specularLight[li]=Global::specularDayLight[li]*0.5;
         }
	    glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);
        glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);
	    glLightfv(GL_LIGHT0,GL_SPECULAR,specularLight);
      }
     break;
     case e_tunnel:
      {
        for (int li=0; li<3; li++)
         {
           ambientLight[li]= Global::ambientDayLight[li]*0.2;
           diffuseLight[li]= Global::diffuseDayLight[li]*0.1;
           specularLight[li]=Global::specularDayLight[li]*0.2;
         }
	glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);
	glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);
	glLightfv(GL_LIGHT0,GL_SPECULAR,specularLight);
      }
     break;
    }

    for (int i=0; i<iNumDynamics; i++)
    {
        //if(SquareMagnitude(Global::pCameraPosition-Dynamics[i]->GetPosition())<20000)
        Dynamics[i]->RenderAlpha();
    }
   glLightfv(GL_LIGHT0,GL_AMBIENT,Global::ambientDayLight);
   glLightfv(GL_LIGHT0,GL_DIFFUSE,Global::diffuseDayLight);
   glLightfv(GL_LIGHT0,GL_SPECULAR,Global::specularDayLight);
}

bool __fastcall TTrack::CheckDynamicObject(TDynamicObject *Dynamic)
{
    for (int i=0; i<iNumDynamics; i++)
        if (Dynamic==Dynamics[i])
            return true;
    return false;
};

bool __fastcall TTrack::RemoveDynamicObject(TDynamicObject *Dynamic)
{
    for (int i=0; i<iNumDynamics; i++)
    {
        if (Dynamic==Dynamics[i])
        {
            iNumDynamics--;
            for (i; i<iNumDynamics; i++)
                Dynamics[i]=Dynamics[i+1];
            return true;

        }
    }
    Error("Cannot remove dynamic from track");
    return false;
}

bool __fastcall TTrack::InMovement()
{//tory animowane (zwrotnica, obrotnica) maj� SwitchExtension
 if (SwitchExtension)
 {if (eType==tt_Switch)
   return SwitchExtension->bMovement; //ze zwrotnic� �atwiej
  if (eType==tt_Turn)
   if (SwitchExtension->pModel)
   {if (!SwitchExtension->CurrentIndex) return false; //0=zablokowana si� nie animuje
    //trzeba ka�dorazowo por�wnywa� z k�tem modelu
    TAnimContainer *ac=SwitchExtension->pModel?SwitchExtension->pModel->GetContainer(NULL):NULL;
    return ac?(ac->AngleGet()!=SwitchExtension->fOffset1):false;
    //return true; //je�li jest taki obiekt
   }
 }
 return false;
};
void __fastcall TTrack::RaAssign(TGroundNode *gn,TAnimContainer *ac)
{//Ra: wi�zanie toru z modelem obrotnicy
 //if (eType==tt_Turn) SwitchExtension->pAnim=p;
};
void __fastcall TTrack::RaAssign(TGroundNode *gn,TAnimModel *am)
{//Ra: wi�zanie toru z modelem obrotnicy
 if (eType==tt_Turn)
 {SwitchExtension->pModel=am;
  SwitchExtension->pMyNode=gn;
 }
};

int __fastcall TTrack::RaArrayPrepare()
{//przygotowanie tablic do skopiowania do VBO (zliczanie wierzcho�k�w)
 if (bVisible) //o ile w og�le wida�
  switch (iCategoryFlag)
  {
   case 1: //tor
    if (eType==tt_Switch) //dla zwrotnicy tylko szyny
     return 48*((TextureID1?SwitchExtension->Segments[0].RaSegCount():0)+(TextureID2?SwitchExtension->Segments[1].RaSegCount():0));
    else //dla toru podsypka plus szyny
     return (Segment->RaSegCount())*((TextureID1?48:0)+(TextureID2?8:0));
   case 2: //droga
    return (Segment->RaSegCount())*((TextureID1?4:0)+(TextureID2?12:0));
   case 4: //rzeki do przemy�lenia
    return (Segment->RaSegCount())*((TextureID1?4:0)+(TextureID2?12:0));
  }
 return 0;
};

void  __fastcall TTrack::RaArrayFill(CVertNormTex *Vert,const CVertNormTex *Start)
{//wype�nianie tablic VBO
 //Ra: trzeba rozdzieli� szyny od podsypki, aby m�c grupowa� wg tekstur
 double fHTW=0.5*fabs(fTrackWidth);
 double side=fabs(fTexWidth); //szerok�� podsypki na zewn�trz szyny albo pobocza
 double rozp=fHTW+side+fabs(fTexSlope); //brzeg zewn�trzny
 double fHTW2,side2,rozp2,fTexHeight2;
 if (iTrapezoid&2) //ten bit oznacza, �e istnieje odpowiednie pNext
 {//Ra: na tym si� lubi wiesza�, ciekawe co za �mieci si� pod��czaj�...
  fHTW2=0.5*fabs(pNext->fTrackWidth); //po�owa rozstawu/nawierzchni
  side2=fabs(pNext->fTexWidth);
  rozp2=fHTW2+side2+fabs(pNext->fTexSlope);
  fTexHeight2=pNext->fTexHeight;
  //zabezpieczenia przed zawieszeniem - logowa� to?
  if (fHTW2>5.0*fHTW) {fHTW2=fHTW; WriteLog("!!!! niedopasowanie 1");};
  if (side2>5.0*side) {side2=side; WriteLog("!!!! niedopasowanie 2");};
  if (rozp2>5.0*rozp) {rozp2=rozp; WriteLog("!!!! niedopasowanie 3");};
  //if (fabs(fTexHeight2)>5.0*fabs(fTexHeight)) {fTexHeight2=fTexHeight; WriteLog("!!!! niedopasowanie 4");};
 }
 else //gdy nie ma nast�pnego albo jest nieodpowiednim ko�cem podpi�ty
 {fHTW2=fHTW; side2=side; rozp2=rozp; fTexHeight2=fTexHeight;}
 double roll1,roll2;
 switch (iCategoryFlag)
 {
  case 1:   //tor
  {
   if (Segment)
    Segment->GetRolls(roll1,roll2);
   else
    roll1=roll2=0.0; //dla zwrotnic
   double sin1=sin(roll1),cos1=cos(roll1),sin2=sin(roll2),cos2=cos(roll2);
   // zwykla szyna: //Ra: czemu g��wki s� asymetryczne na wysoko�ci 0.140?
   vector6 rpts1[24],rpts2[24],rpts3[24],rpts4[24];
   int i;
   for (i=0;i<12;++i)
   {rpts1[i]   =vector6(( fHTW+szyna[i].x)*cos1+szyna[i].y*sin1,-( fHTW+szyna[i].x)*sin1+szyna[i].y*cos1,szyna[i].z ,+szyna[i].n.x*cos1+szyna[i].n.y*sin1,-szyna[i].n.x*sin1+szyna[i].n.y*cos1,0.0);
    rpts2[11-i]=vector6((-fHTW-szyna[i].x)*cos1+szyna[i].y*sin1,-(-fHTW-szyna[i].x)*sin1+szyna[i].y*cos1,szyna[i].z ,-szyna[i].n.x*cos1+szyna[i].n.y*sin1,+szyna[i].n.x*sin1+szyna[i].n.y*cos1,0.0);
   }
   if (iTrapezoid) //trapez albo przechy�ki, to oddzielne punkty na ko�cu
    for (i=0;i<12;++i)
    {rpts1[12+i]=vector6(( fHTW2+szyna[i].x)*cos2+szyna[i].y*sin2,-( fHTW2+szyna[i].x)*sin2+szyna[i].y*cos2,szyna[i].z ,+szyna[i].n.x*cos2+szyna[i].n.y*sin2,-szyna[i].n.x*sin2+szyna[i].n.y*cos2,0.0);
     rpts2[23-i]=vector6((-fHTW2-szyna[i].x)*cos2+szyna[i].y*sin2,-(-fHTW2-szyna[i].x)*sin2+szyna[i].y*cos2,szyna[i].z ,-szyna[i].n.x*cos2+szyna[i].n.y*sin2,+szyna[i].n.x*sin2+szyna[i].n.y*cos2,0.0);
    }
   switch (eType) //dalej zale�nie od typu
   {
    case tt_Turn: //obrotnica jak zwyk�y tor, tylko animacja dochodzi
     SwitchExtension->iLeftVBO=Vert-Start; //indeks toru obrotnicy
    case tt_Normal:
     if (TextureID2)
     {//podsypka z podk�adami jest tylko dla zwyk�ego toru
      vector6 bpts1[8]; //punkty g��wnej p�aszczyzny nie przydaj� si� do robienia bok�w
      if (iTrapezoid) //trapez albo przechy�ki
      {//podsypka z podkladami trapezowata
       //ewentualnie poprawi� mapowanie, �eby �rodek mapowa� si� na 1.435/4.671 ((0.3464,0.6536)
       //bo si� tekstury podsypki rozje�d�aj� po zmianie proporcji profilu
       bpts1[0]=vector6(rozp,-fTexHeight,0.0,-0.707,0.707,0.0); //lewy brzeg
       bpts1[1]=vector6((fHTW+side)*cos1,-(fHTW+side)*sin1,0.33,-0.707,0.707,0.0); //kraw�d� za�amania
       bpts1[2]=vector6(-bpts1[1].x,-bpts1[1].y,0.67,0.707,0.707,0.0); //prawy brzeg pocz�tku symetrycznie
       bpts1[3]=vector6(-rozp,-fTexHeight,1.0,0.707,0.707,0.0); //prawy skos
       bpts1[4]=vector6(rozp2,-fTexHeight2,0.0,-0.707,0.707,0.0); //lewy brzeg
       bpts1[5]=vector6((fHTW2+side2)*cos2,-(fHTW2+side2)*sin2,0.33,-0.707,0.707,0.0); //kraw�d� za�amania
       bpts1[6]=vector6(-bpts1[5].x,-bpts1[5].y,0.67,0.707,0.707,0.0); //prawy brzeg pocz�tku symetrycznie
       bpts1[7]=vector6(-rozp2,-fTexHeight2,1.0,0.707,0.707,0.0); //prawy skos
      }
      else
      {bpts1[0]=vector6(rozp,-fTexHeight,0.0,-0.707,0.707,0.0); //lewy brzeg
       bpts1[1]=vector6(fHTW+side,0.0,0.33,-0.707,0.707,0.0); //kraw�d� za�amania
       bpts1[2]=vector6(-fHTW-side,0.0,0.67,0.707,0.707,0.0); //druga
       bpts1[3]=vector6(-rozp,-fTexHeight,1.0,0.707,0.707,0.0); //prawy skos
      }
      Segment->RaRenderLoft(Vert,bpts1,iTrapezoid?-4:4,fTexLength);
     }
     if (TextureID1)
     {// szyny - generujemy dwie, najwy�ej rysowa� si� b�dzie jedn�
      Segment->RaRenderLoft(Vert,rpts1,iTrapezoid?-nnumPts:nnumPts,fTexLength);
      Segment->RaRenderLoft(Vert,rpts2,iTrapezoid?-nnumPts:nnumPts,fTexLength);
     }
     break;
    case tt_Switch: //dla zwrotnicy dwa razy szyny
     if (TextureID1) //Ra: !!!! tu jest do poprawienia
     {//iglice liczone tylko dla zwrotnic
      vector6 rpts3[24],rpts4[24];
      for (i=0;i<12;++i)
      {rpts3[i]   =vector6((fHTW+iglica[i].x)*cos1+iglica[i].y*sin1,-(fHTW+iglica[i].x)*sin1+iglica[i].y*cos1,iglica[i].z);
       rpts3[i+12]=vector6((fHTW2+szyna[i].x)*cos2+szyna[i].y*sin2,-(fHTW2+szyna[i].x)*sin2+iglica[i].y*cos2,szyna[i].z);
       rpts4[11-i]=vector6((-fHTW-iglica[i].x)*cos1+iglica[i].y*sin1,-(-fHTW-iglica[i].x)*sin1+iglica[i].y*cos1,iglica[i].z);
       rpts4[23-i]=vector6((-fHTW2-szyna[i].x)*cos2+szyna[i].y*sin2,-(-fHTW2-szyna[i].x)*sin2+iglica[i].y*cos2,szyna[i].z);
      }
      if (SwitchExtension->RightSwitch)
      {//nowa wersja z SPKS, ale odwrotnie lewa/prawa
       SwitchExtension->iLeftVBO=Vert-Start; //indeks lewej iglicy
       SwitchExtension->Segments[0].RaRenderLoft(Vert,rpts3,-nnumPts,fTexLength,0,2,SwitchExtension->fOffset1);
       SwitchExtension->Segments[0].RaRenderLoft(Vert,rpts1,nnumPts,fTexLength,2);
       SwitchExtension->Segments[0].RaRenderLoft(Vert,rpts2,nnumPts,fTexLength);
       SwitchExtension->Segments[1].RaRenderLoft(Vert,rpts1,nnumPts,fTexLength);
       SwitchExtension->iRightVBO=Vert-Start; //indeks prawej iglicy
       SwitchExtension->Segments[1].RaRenderLoft(Vert,rpts4,-nnumPts,fTexLength,0,2,-fMaxOffset+SwitchExtension->fOffset1);
       SwitchExtension->Segments[1].RaRenderLoft(Vert,rpts2,nnumPts,fTexLength,2);
      }
      else
      {//lewa dzia�a lepiej ni� prawa
       SwitchExtension->Segments[0].RaRenderLoft(Vert,rpts1,nnumPts,fTexLength); //lewa szyna normalna ca�a
       SwitchExtension->iLeftVBO=Vert-Start; //indeks lewej iglicy
       SwitchExtension->Segments[0].RaRenderLoft(Vert,rpts4,-nnumPts,fTexLength,0,2,-SwitchExtension->fOffset1); //prawa iglica
       SwitchExtension->Segments[0].RaRenderLoft(Vert,rpts2,nnumPts,fTexLength,2); //prawa szyna za iglic�
       SwitchExtension->iRightVBO=Vert-Start; //indeks prawej iglicy
       SwitchExtension->Segments[1].RaRenderLoft(Vert,rpts3,-nnumPts,fTexLength,0,2,fMaxOffset-SwitchExtension->fOffset1); //lewa iglica
       SwitchExtension->Segments[1].RaRenderLoft(Vert,rpts1,nnumPts,fTexLength,2); //lewa szyna za iglic�
       SwitchExtension->Segments[1].RaRenderLoft(Vert,rpts2,nnumPts,fTexLength); //prawa szyna normalnie ca�a
      }
     }
     break;
   }
  } //koniec obs�ugi tor�w
  break;
  case 2: //McZapkie-260302 - droga - rendering
  case 4: //Ra: rzeki na razie jak drogi, przechy�ki na pewno nie maj�
  {vector6 bpts1[4]; //punkty g��wnej p�aszczyzny przydaj� si� do robienia bok�w
   if (TextureID1||TextureID2) //punkty si� przydadz�, nawet je�li nawierzchni nie ma
   {//double max=2.0*(fHTW>fHTW2?fHTW:fHTW2); //z szerszej strony jest 100%
    double max=(iCategoryFlag==4)?0.0:fTexLength; //test: szeroko�� dr�g proporcjonalna do d�ugo�ci
    double map1=max>0.0?fHTW/max:0.5; //obci�cie tekstury od strony 1
    double map2=max>0.0?fHTW2/max:0.5; //obci�cie tekstury od strony 2
    if (iTrapezoid) //trapez albo przechy�ki
    {//nawierzchnia trapezowata
     Segment->GetRolls(roll1,roll2);
     bpts1[0]=vector6(fHTW*cos(roll1),-fHTW*sin(roll1),0.5-map1); //lewy brzeg pocz�tku
     bpts1[1]=vector6(-bpts1[0].x,-bpts1[0].y,0.5+map1); //prawy brzeg pocz�tku symetrycznie
     bpts1[2]=vector6(fHTW2*cos(roll2),-fHTW2*sin(roll2),0.5-map2); //lewy brzeg ko�ca
     bpts1[3]=vector6(-bpts1[2].x,-bpts1[2].y,0.5+map2); //prawy brzeg pocz�tku symetrycznie
    }
    else
    {bpts1[0]=vector6( fHTW,0.0,0.5-map1); //zawsze standardowe mapowanie
     bpts1[1]=vector6(-fHTW,0.0,0.5+map1);
    }
   }
   if (TextureID1) //je�li podana by�a tekstura, generujemy tr�jk�ty
   {//tworzenie tr�jk�t�w nawierzchni szosy
    Segment->RaRenderLoft(Vert,bpts1,iTrapezoid?-2:2,fTexLength);
   }
   if (TextureID2)
   {//pobocze drogi - poziome przy przechy�ce (a mo�e kraw�nik i chodnik zrobi� jak w Midtown Madness 2?)
    vector6 rpts1[6],rpts2[6]; //wsp�rz�dne przekroju i mapowania dla prawej i lewej strony
    rpts1[0]=vector6(rozp,-fTexHeight,0.0); //lewy brzeg podstawy
    rpts1[1]=vector6(bpts1[0].x+side,bpts1[0].y,0.5), //lewa kraw�d� za�amania
    rpts1[2]=vector6(bpts1[0].x,bpts1[0].y,1.0); //lewy brzeg pobocza (mapowanie mo�e by� inne
    rpts2[0]=vector6(bpts1[1].x,bpts1[1].y,1.0); //prawy brzeg pobocza
    rpts2[1]=vector6(bpts1[1].x-side,bpts1[1].y,0.5); //prawa kraw�d� za�amania
    rpts2[2]=vector6(-rozp,-fTexHeight,0.0); //prawy brzeg podstawy
    if (iTrapezoid) //trapez albo przechy�ki
    {//pobocza do trapezowatej nawierzchni - dodatkowe punkty z drugiej strony odcinka
     rpts1[3]=vector6(rozp2,-fTexHeight2,0.0); //lewy brzeg lewego pobocza
     rpts1[4]=vector6(bpts1[2].x+side2,bpts1[2].y,0.5); //kraw�d� za�amania
     rpts1[5]=vector6(bpts1[2].x,bpts1[2].y,1.0); //brzeg pobocza
     rpts2[3]=vector6(bpts1[3].x,bpts1[3].y,1.0);
     rpts2[4]=vector6(bpts1[3].x-side2,bpts1[3].y,0.5);
     rpts2[5]=vector6(-rozp2,-fTexHeight2,0.0); //prawy brzeg prawego pobocza
     Segment->RaRenderLoft(Vert,rpts1,-3,fTexLength);
     Segment->RaRenderLoft(Vert,rpts2,-3,fTexLength);
    }
    else
    {//pobocza zwyk�e, brak przechy�ki
     Segment->RaRenderLoft(Vert,rpts1,3,fTexLength);
     Segment->RaRenderLoft(Vert,rpts2,3,fTexLength);
    }
   }
  }
  break;
 }
};

void  __fastcall TTrack::RaRenderVBO(int iPtr)
{//renderowanie z u�yciem VBO
 glColor3f(1.0f,1.0f,1.0f);
 //McZapkie-310702: zmiana oswietlenia w tunelu, wykopie
 GLfloat ambientLight[4] ={0.5f,0.5f,0.5f,1.0f};
 GLfloat diffuseLight[4] ={0.5f,0.5f,0.5f,1.0f};
 GLfloat specularLight[4]={0.5f,0.5f,0.5f,1.0f};
 switch (eEnvironment)
 {//modyfikacje o�wietlenia zale�nie od �rodowiska
  case e_canyon: //wykop
   for (int li=0;li<3;li++)
   {
    ambientLight[li]= Global::ambientDayLight[li]*0.7;
    diffuseLight[li]= Global::diffuseDayLight[li]*0.3;
    specularLight[li]=Global::specularDayLight[li]*0.4;
   }
   glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);
   glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);
   glLightfv(GL_LIGHT0,GL_SPECULAR,specularLight);
  break;
  case e_tunnel: //tunel
   for (int li=0;li<3;li++)
   {
    ambientLight[li]= Global::ambientDayLight[li]*0.2;
    diffuseLight[li]= Global::diffuseDayLight[li]*0.1;
    specularLight[li]=Global::specularDayLight[li]*0.2;
   }
   glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);
   glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);
   glLightfv(GL_LIGHT0,GL_SPECULAR,specularLight);
  break;
 }
 int seg;
 int i;
 switch (iCategoryFlag)
 {
  case 1: //tor
   if (eType==tt_Switch) //dla zwrotnicy tylko szyny
   {if (TextureID1)
     if ((seg=SwitchExtension->Segments[0].RaSegCount())>0)
     {glBindTexture(GL_TEXTURE_2D,TextureID1); //szyny +
      for (i=0;i<seg;++i)
       glDrawArrays(GL_TRIANGLE_STRIP,iPtr+24*i,24);
      iPtr+=24*seg; //pomini�cie lewej szyny
      for (i=0;i<seg;++i)
       glDrawArrays(GL_TRIANGLE_STRIP,iPtr+24*i,24);
      iPtr+=24*seg; //pomini�cie prawej szyny
     }
    if (TextureID2)
     if ((seg=SwitchExtension->Segments[1].RaSegCount())>0)
     {glBindTexture(GL_TEXTURE_2D,TextureID2); //szyny -
      for (i=0;i<seg;++i)
       glDrawArrays(GL_TRIANGLE_STRIP,iPtr+24*i,24);
      iPtr+=24*seg; //pomini�cie lewej szyny
      for (i=0;i<seg;++i)
       glDrawArrays(GL_TRIANGLE_STRIP,iPtr+24*i,24);
     }
   }
   else //dla toru podsypka plus szyny
   {if ((seg=Segment->RaSegCount())>0)
    {if (TextureID2)
     {glBindTexture(GL_TEXTURE_2D,TextureID2); //podsypka
      for (i=0;i<seg;++i)
       glDrawArrays(GL_TRIANGLE_STRIP,iPtr+8*i,8);
      iPtr+=8*seg; //pomini�cie podsypki
     }
     if (TextureID1)
     {glBindTexture(GL_TEXTURE_2D,TextureID1); //szyny
      for (i=0;i<seg;++i)
       glDrawArrays(GL_TRIANGLE_STRIP,iPtr+24*i,24);
      iPtr+=24*seg; //pomini�cie lewej szyny
      for (i=0;i<seg;++i)
       glDrawArrays(GL_TRIANGLE_STRIP,iPtr+24*i,24);
     }
    }
   }
   break;
  case 2: //droga
  case 4: //rzeki - jeszcze do przemy�lenia
   if ((seg=Segment->RaSegCount())>0)
   {if (TextureID1)
    {glBindTexture(GL_TEXTURE_2D,TextureID1); //nawierzchnia
     for (i=0;i<seg;++i)
     {glDrawArrays(GL_TRIANGLE_STRIP,iPtr,4); iPtr+=4;}
    }
    if (TextureID2)
    {glBindTexture(GL_TEXTURE_2D,TextureID2); //pobocze
     for (i=0;i<seg;++i)
      glDrawArrays(GL_TRIANGLE_STRIP,iPtr+6*i,6);
     iPtr+=6*seg; //pomini�cie lewego pobocza
     for (i=0;i<seg;++i)
      glDrawArrays(GL_TRIANGLE_STRIP,iPtr+6*i,6);
    }
   }
   break;
 }
 switch (eEnvironment)
 {//przywr�cenie globalnych ustawie� �wiat�a
  case e_canyon: //wykop
  case e_tunnel: //tunel
   glLightfv(GL_LIGHT0,GL_AMBIENT,Global::ambientDayLight);
   glLightfv(GL_LIGHT0,GL_DIFFUSE,Global::diffuseDayLight);
   glLightfv(GL_LIGHT0,GL_SPECULAR,Global::specularDayLight);
 }
};

void  __fastcall TTrack::RaRenderDynamic()
{//renderowanie pojazd�w
 if (!iNumDynamics) return; //nie ma pojazd�w
 glColor3f(1.0f,1.0f,1.0f);
 //McZapkie-310702: zmiana oswietlenia w tunelu, wykopie
 GLfloat ambientLight[4] ={0.5f,0.5f,0.5f,1.0f};
 GLfloat diffuseLight[4] ={0.5f,0.5f,0.5f,1.0f};
 GLfloat specularLight[4]={0.5f,0.5f,0.5f,1.0f};
 switch (eEnvironment)
 {//modyfikacje o�wietlenia zale�nie od �rodowiska
  case e_canyon: //wykop
   for (int li=0;li<3;li++)
   {
    ambientLight[li]= Global::ambientDayLight[li]*0.7;
    diffuseLight[li]= Global::diffuseDayLight[li]*0.3;
    specularLight[li]=Global::specularDayLight[li]*0.4;
   }
   glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);
   glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);
   glLightfv(GL_LIGHT0,GL_SPECULAR,specularLight);
  break;
  case e_tunnel: //tunel
   for (int li=0;li<3;li++)
   {
    ambientLight[li]= Global::ambientDayLight[li]*0.2;
    diffuseLight[li]= Global::diffuseDayLight[li]*0.1;
    specularLight[li]=Global::specularDayLight[li]*0.2;
   }
   glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);
   glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);
   glLightfv(GL_LIGHT0,GL_SPECULAR,specularLight);
  break;
 }
 for (int i=0;i<iNumDynamics;i++)
 {Dynamics[i]->Render(); //zmieni kontekst VBO!
  Dynamics[i]->RenderAlpha();
 }
 switch (eEnvironment)
 {//przywr�cenie globalnych ustawie� �wiat�a
  case e_canyon: //wykop
  case e_tunnel: //tunel
   glLightfv(GL_LIGHT0,GL_AMBIENT,Global::ambientDayLight);
   glLightfv(GL_LIGHT0,GL_DIFFUSE,Global::diffuseDayLight);
   glLightfv(GL_LIGHT0,GL_SPECULAR,Global::specularDayLight);
 }
};

//---------------------------------------------------------------------------

bool __fastcall TTrack::Switch(int i)
{//prze��czenie tor�w z uruchomieniem animacji
 if (SwitchExtension) //tory prze��czalne maj� doklejk�
  if (eType==tt_Switch)
  {//przek�adanie zwrotnicy jak zwykle
   i&=1; //ograniczenie b��d�w !!!!
   SwitchExtension->fDesiredOffset1=fMaxOffset*double(NextMask[i]); //od punktu 1
   //SwitchExtension->fDesiredOffset2=fMaxOffset*double(PrevMask[i]); //od punktu 2
   SwitchExtension->CurrentIndex=i;
   Segment=SwitchExtension->Segments+i; //wybranie aktywnej drogi - potrzebne to?
   pNext=SwitchExtension->pNexts[NextMask[i]]; //prze��czenie ko�c�w
   pPrev=SwitchExtension->pPrevs[PrevMask[i]];
   bNextSwitchDirection=SwitchExtension->bNextSwitchDirection[NextMask[i]];
   bPrevSwitchDirection=SwitchExtension->bPrevSwitchDirection[PrevMask[i]];
   fRadius=fRadiusTable[i]; //McZapkie: wybor promienia toru
   if (SwitchExtension->pOwner?SwitchExtension->pOwner->RaTrackAnimAdd(this):true) //je�li nie dodane do animacji
    SwitchExtension->fOffset1=SwitchExtension->fDesiredOffset1; //nie ma si� co bawi�
   return true;
  }
  else
  {//blokowanie (0, szukanie tor�w) lub odblokowanie (1, roz��czenie) obrotnicy
   if (i)
   {//0: roz��czenie obrotnicy od s�siednich tor�w
    if (pPrev)
     if (bPrevSwitchDirection)
      pPrev->pPrev=NULL;
     else
      pPrev->pNext=NULL;
    if (pNext)
     if (bPrevSwitchDirection)
      pNext->pNext=NULL;
     else
      pNext->pPrev=NULL;
    pNext=pPrev=NULL;
    fVelocity=0.0; //AI, nie ruszaj si�!
    if (SwitchExtension->pOwner)
     SwitchExtension->pOwner->RaTrackAnimAdd(this); //dodanie do listy animacyjnej
   }
   else
   {//1: ustalenie finalnego po�o�enia (gdy nie by�o animacji)
    RaAnimate(); //ostatni etap animowania
    //zablokowanie pozycji i po��czenie do s�siednich tor�w
    Global::pGround->TrackJoin(SwitchExtension->pMyNode);
    if (pNext||pPrev)
     fVelocity=6.0; //jazda dozwolona
   }
   SwitchExtension->CurrentIndex=i; //zapami�tanie stanu zablokowania
   return true;
  }
 Error("Cannot switch normal track");
 return false;
};

void __fastcall TTrack::RaAnimListAdd(TTrack *t)
{//dodanie toru do listy animacyjnej
 if (SwitchExtension)
 {if (t==this) return; //siebie nie dodajemy drugi raz do listy
  if (!t->SwitchExtension) return; //nie podlega animacji
  if (SwitchExtension->pNextAnim)
  {if (SwitchExtension->pNextAnim==t)
    return; //gdy ju� taki jest
   else
    SwitchExtension->pNextAnim->RaAnimListAdd(t);
  }
  else
  {SwitchExtension->pNextAnim=t;
   t->SwitchExtension->pNextAnim=NULL; //nowo dodawany nie mo�e mie� ogona
  }
 }
};

TTrack* __fastcall TTrack::RaAnimate()
{//wykonanie rekurencyjne animacji, wywo�ywane przed wy�wietleniem sektora z VBO
 //zwraca wska�nik toru wymagaj�cego dalszej animacji
 if (SwitchExtension->pNextAnim)
  SwitchExtension->pNextAnim=SwitchExtension->pNextAnim->RaAnimate();
 bool m=true; //animacja trwa
 if (eType==tt_Switch) //dla zwrotnicy tylko szyny
 {double v=SwitchExtension->fDesiredOffset1-SwitchExtension->fOffset1; //pr�dko��
  SwitchExtension->fOffset1+=sign(v)*Timer::GetDeltaTime()*0.1;
  //Ra: trzeba da� to do klasy...
  if (SwitchExtension->fOffset1<=0.00)
  {SwitchExtension->fOffset1; //1cm?
   m=false; //koniec animacji
  }
  if (SwitchExtension->fOffset1>=fMaxOffset)
  {SwitchExtension->fOffset1=fMaxOffset; //maksimum-1cm?
   m=false; //koniec animacji
  }
  if (Global::bUseVBO)
  {//dla OpenGL 1.4 od�wie�y si� ca�y sektor, w p�niejszych poprawiamy fragment
   if (Global::bOpenGL_1_5) //dla OpenGL 1.4 to si� nie wykona poprawnie
    if (TextureID1) //Ra: !!!! tu jest do poprawienia
    {//iglice liczone tylko dla zwrotnic
     vector6 rpts3[24],rpts4[24];
     double fHTW=0.5*fabs(fTrackWidth);
     double fHTW2=fHTW; //Ra: na razie niech tak b�dzie
     double cos1=1.0,sin1=0.0,cos2=1.0,sin2=0.0; //Ra: ...
     for (int i=0;i<12;++i)
     {rpts3[i]   =vector6((fHTW+iglica[i].x)*cos1+iglica[i].y*sin1,-(fHTW+iglica[i].x)*sin1+iglica[i].y*cos1,iglica[i].z);
      rpts3[i+12]=vector6((fHTW2+szyna[i].x)*cos2+szyna[i].y*sin2,-(fHTW2+szyna[i].x)*sin2+iglica[i].y*cos2,szyna[i].z);
      rpts4[11-i]=vector6((-fHTW-iglica[i].x)*cos1+iglica[i].y*sin1,-(-fHTW-iglica[i].x)*sin1+iglica[i].y*cos1,iglica[i].z);
      rpts4[23-i]=vector6((-fHTW2-szyna[i].x)*cos2+szyna[i].y*sin2,-(-fHTW2-szyna[i].x)*sin2+iglica[i].y*cos2,szyna[i].z);
     }
     CVertNormTex Vert[2*2*12]; //na razie 2 segmenty
     CVertNormTex *v=Vert; //bo RaAnimate() modyfikuje wska�nik
     glGetBufferSubData(GL_ARRAY_BUFFER,SwitchExtension->iLeftVBO*sizeof(CVertNormTex),2*2*12*sizeof(CVertNormTex),&Vert);//pobranie fragmentu bufora VBO
     if (SwitchExtension->RightSwitch)
     {//nowa wersja z SPKS, ale odwrotnie lewa/prawa
      SwitchExtension->Segments[0].RaAnimate(v,rpts3,-nnumPts,fTexLength,0,2,SwitchExtension->fOffset1);
      glBufferSubData(GL_ARRAY_BUFFER,SwitchExtension->iLeftVBO*sizeof(CVertNormTex),2*2*12*sizeof(CVertNormTex),&Vert); //wys�anie fragmentu bufora VBO
      v=Vert;
      glGetBufferSubData(GL_ARRAY_BUFFER,SwitchExtension->iRightVBO*sizeof(CVertNormTex),2*2*12*sizeof(CVertNormTex),&Vert);//pobranie fragmentu bufora VBO
      SwitchExtension->Segments[1].RaAnimate(v,rpts4,-nnumPts,fTexLength,0,2,-fMaxOffset+SwitchExtension->fOffset1);
     }
     else
     {//oryginalnie lewa dzia�a�a lepiej ni� prawa
      SwitchExtension->Segments[0].RaAnimate(v,rpts4,-nnumPts,fTexLength,0,2,-SwitchExtension->fOffset1); //prawa iglica
      glBufferSubData(GL_ARRAY_BUFFER,SwitchExtension->iLeftVBO*sizeof(CVertNormTex),2*2*12*sizeof(CVertNormTex),&Vert);//wys�anie fragmentu bufora VBO
      v=Vert;
      glGetBufferSubData(GL_ARRAY_BUFFER,SwitchExtension->iRightVBO*sizeof(CVertNormTex),2*2*12*sizeof(CVertNormTex),&Vert); //pobranie fragmentu bufora VBO
      SwitchExtension->Segments[1].RaAnimate(v,rpts3,-nnumPts,fTexLength,0,2,fMaxOffset-SwitchExtension->fOffset1); //lewa iglica
     }
     glBufferSubData(GL_ARRAY_BUFFER,SwitchExtension->iRightVBO*sizeof(CVertNormTex),2*2*12*sizeof(CVertNormTex),&Vert); //wys�anie fragmentu bufora VBO
    }
  }
  else //gdy Display List
   Release(); //niszczenie skompilowanej listy
 }
 else if (eType==tt_Turn) //dla obrotnicy - szyny i podsypka
 {
  if (SwitchExtension->pModel&&SwitchExtension->CurrentIndex) //0=zablokowana si� nie animuje
  {//trzeba ka�dorazowo por�wnywa� z k�tem modelu
   //SwitchExtension->fOffset1=SwitchExtension->pAnim?SwitchExtension->pAnim->AngleGet():0.0; //pobranie k�ta z modelu
   TAnimContainer *ac=SwitchExtension->pModel?SwitchExtension->pModel->GetContainer(NULL):NULL; //pobranie g��wnego submodelu
   if (ac?(ac->AngleGet()!=SwitchExtension->fOffset1):false) //czy przemie�ci�o si� od ostatniego sprawdzania
   {double hlen=0.5*SwitchExtension->Segments->GetLength(); //po�owa d�ugo�ci
    SwitchExtension->fOffset1=180+ac->AngleGet(); //pobranie k�ta z submodelu
    double sina=hlen*sin(DegToRad(SwitchExtension->fOffset1)),cosa=hlen*cos(DegToRad(SwitchExtension->fOffset1));
    vector3 middle=SwitchExtension->Segments->FastGetPoint(0.5);
    Segment->Init(middle+vector3(sina,0.0,cosa),middle-vector3(sina,0.0,cosa),5.0); //nowy odcinek
    if (Global::bUseVBO)
    {//dla OpenGL 1.4 od�wie�y si� ca�y sektor, w p�niejszych poprawiamy fragment
     if (Global::bOpenGL_1_5) //dla OpenGL 1.4 to si� nie wykona poprawnie
     {int size=RaArrayPrepare();
      CVertNormTex *Vert=new CVertNormTex[size]; //bufor roboczy
      //CVertNormTex *v=Vert; //zmieniane przez
      RaArrayFill(Vert,Vert-SwitchExtension->iLeftVBO); //iLeftVBO powinno zosta� niezmienione
      glBufferSubData(GL_ARRAY_BUFFER,SwitchExtension->iLeftVBO*sizeof(CVertNormTex),size*sizeof(CVertNormTex),Vert); //wys�anie fragmentu bufora VBO
     }
    }
    else //gdy Display List
     Release(); //niszczenie skompilowanej listy, aby si� wygenerowa�a nowa
   } //animacja trwa nadal
  } else m=false; //koniec animacji albo w og�le nie po��czone z modelem
 }
 return m?this:SwitchExtension->pNextAnim; //zwraca obiekt do dalszej animacji
};
//---------------------------------------------------------------------------

