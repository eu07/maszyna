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

//101206 Ra: trapezoidalne drogi

__fastcall TSwitchExtension::TSwitchExtension()
{//na pocz¹tku wszystko puste
    CurrentIndex=0;
    pNexts[0]= NULL;
    pNexts[1]= NULL;
    pPrevs[0]= NULL;
    pPrevs[1]= NULL;
    fOffset1=fOffset2=fDesiredOffset1=fDesiredOffset2=0;
}
__fastcall TSwitchExtension::~TSwitchExtension()
{
//    SafeDelete(pNexts[0]);
//    SafeDelete(pNexts[1]);
//    SafeDelete(pPrevs[0]);
//    SafeDelete(pPrevs[1]);
}

__fastcall TTrack::TTrack()
{//tworzenie nowego odcinka ruchu
    pNext=pPrev= NULL;
    Segment= NULL;
    SwitchExtension= NULL;
    TextureID1= 0;
    fTexLength= 4.0;
    TextureID2= 0;
    fTexHeight= 0.2;
    fTexWidth= 0.9;
    fTexSlope= 1.1;
    HelperPts= NULL;
    iCategoryFlag= 1;
    fTrackWidth= 1.435;
    fFriction= 0.15;
    fSoundDistance= -1;
    iQualityFlag= 20;
    iDamageFlag= 0;
    eEnvironment= e_flat;
    bVisible= true;
    Event0= NULL;
    Event1= NULL;
    Event2= NULL;
    Eventall0= NULL;
    Eventall1= NULL;
    Eventall2= NULL;
    fVelocity= -1;
    fTrackLength= 100.0;
    fRadius= 0;
    fRadiusTable[0]= 0; //dwa promienie nawet dla prostego
    fRadiusTable[1]= 0;
    iNumDynamics= 0;
    ScannedFlag=false;
    DisplayListID = 0;
    iTrapezoid=0; //parametry kszta³tu: 0-standard, 1-przechy³ka, 2-trapez, 3-oba
}

__fastcall TTrack::~TTrack()
{
    SafeDeleteArray(HelperPts);
    switch (eType)
    {
        case tt_Switch:
            SafeDelete(SwitchExtension);
        break;
        case tt_Normal:
            SafeDelete(Segment);
        break;
    }
}

void __fastcall TTrack::Init()
{
    switch (eType)
    {
        case tt_Switch:
            SwitchExtension= new TSwitchExtension();
        break;
        case tt_Normal:
            Segment= new TSegment();
        break;
    }
}

void __fastcall TTrack::ConnectPrevPrev(TTrack *pTrack)
{
    if (pTrack)
    {
        pPrev= pTrack;
        bPrevSwitchDirection= true;
        pTrack->pPrev= this;
        pTrack->bPrevSwitchDirection= true;
    }
}

void __fastcall TTrack::ConnectPrevNext(TTrack *pTrack)
{
    if (pTrack)
    {
        pPrev= pTrack;
        bPrevSwitchDirection= false;
        pTrack->pNext= this;
        pTrack->bNextSwitchDirection= false;
        if ((fTrackWidth!=pTrack->fTrackWidth) //Ra: jeœli kolejny ma inne wymiary
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
        pNext= pTrack;
        bNextSwitchDirection= false;
        pTrack->pPrev= this;
        pTrack->bPrevSwitchDirection= false;
        if ((fTrackWidth!=pTrack->fTrackWidth) //Ra: jeœli kolejny ma inne wymiary
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
        pNext= pTrack;
        bNextSwitchDirection= true;
        pTrack->pNext= this;
        pTrack->bNextSwitchDirection= true;
    }
}

/*
bool __fastcall TTrack::ConnectNext1(TTrack *pNewNext)
{
    if (pNewNext)
    {
        if (pNext1)
            pNext1->pPrev= NULL;
        pNext1= pNewNext;
        pNext1->pPrev= this;
        return true;
    }
    return false;
}

bool __fastcall TTrack::ConnectNext2(TTrack *pNewNext)
{
    if (pNewNext)
    {
        if (pNext2)
            pNext2->pPrev= NULL;
        pNext2= pNewNext;
        pNext2->pPrev= this;
        return true;
    }
    return false;
}

bool __fastcall TTrack::ConnectPrev1(TTrack *pNewPrev)
{
    if (pNewPrev)
    {
        if (pPrev)
        {
            if (pPrev->pNext1==this)
                pPrev->pNext1= NULL;
            if (pPrev->pNext2==this)
                pPrev->pNext2= NULL;
        }
        pPrev= pNewPrev;
        pPrev->pNext1= this;
        return true;
    }
    return false;
}

bool __fastcall TTrack::ConnectPrev2(TTrack *pNewPrev)
{
    if (pNewPrev)
    {
        if (pPrev)
        {
            if (pPrev->pNext1==this)
                pPrev->pNext1= NULL;
            if (pPrev->pNext2==this)
                pPrev->pNext2= NULL;
        }
        pPrev= pNewPrev;
        pPrev->pNext2= this;
        return true;
    }
    return false;
}
*/
vector3 __fastcall MakeCPoint(vector3 p, double d, double a1, double a2)
{
    vector3 cp= vector3(0,0,1);
    cp.RotateX(DegToRad(a2));
    cp.RotateY(DegToRad(a1));
    cp= cp*d+p;

    return cp;

}

vector3 __fastcall LoadPoint(cParser *parser)
{//pobranie wspó³rzêdnych punktu
    vector3 p;
    std::string token;
    parser->getTokens(3);
    *parser >> p.x >> p.y >> p.z;
    return p;
}

/* Ra: to siê niczym nie ró¿ni od powy¿szego
vector3 __fastcall LoadCPoint(cParser *parser)
{//pobranie wspó³rzêdnych wektora kontrolnego
    vector3 cp;
    std::string token;
    parser->getTokens(3);
    *parser >> cp.x >> cp.y >> cp.z;
    return cp;
}
*/

/* Ra: to chyba kiedyœ by³a zwrotnica...
const vector3 sw1pts[]= { vector3(1.378,0.0,25.926),vector3(0.378,0.0,16.926),
                          vector3(0.0,0.0,26.0), vector3(0.0,0.0,26.0),
                          vector3(0.0,0.0,6.0) };

const vector3 sw1pt1= vector3(0.0,0.0,26.0);
const vector3 sw1cpt1= vector3(0.0,0.0,26.0);
const vector3 sw1pt2= vector3(1.378,0.0,25.926);
const vector3 sw1cpt2= vector3(0.378,0.0,16.926);
const vector3 sw1pt3= vector3(0.0,0.0,0.0);
const vector3 sw1cpt3= vector3(0.0,0.0,6.0);
*/

void __fastcall TTrack::Load(cParser *parser, vector3 pOrigin)
{//pobranie obiektu trajektorii ruchu
    vector3 pt,vec,p1,p2,cp1,cp2,p3,cp3,p4,cp4,p5,cp5,swpt[3],swcp[3],dir;
    double a1,a2,r1,r2,d1,d2,a;
    AnsiString str;
    bool bCurve;
    int i;//,state; //Ra: teraz ju¿ nie ma pocz¹tkowego stanu zwrotnicy we wpisie
    std::string token;

    parser->getTokens();
    *parser >> token;
    str= AnsiString(token.c_str());

    if (str=="normal")
     {
        eType= tt_Normal;
        iCategoryFlag= 1;
     }
    else
    if (str=="switch")
     {
        eType= tt_Switch;
        iCategoryFlag= 1;
     }
    else
    if (str=="turn")
     {//Ra: to bêdzie obrotnica
        eType= tt_Turn;
        iCategoryFlag= 1;
     }
    else
    if (str=="road")
     {
        eType= tt_Normal;
        iCategoryFlag= 2;
     }
    else
    if (str=="cross")
     {//Ra: to bêdzie skrzy¿owanie dróg
        eType= tt_Cross;
        iCategoryFlag= 2;
     }
    else
    if (str=="river")
     {
        eType= tt_Normal;
        iCategoryFlag= 4;
     }
    else
       eType= tt_Unknown;
    if (DebugModeFlag)
     WriteLog(str.c_str());
    parser->getTokens(4);
    *parser >> fTrackLength >> fTrackWidth >> fFriction >> fSoundDistance;
//    fTrackLength= Parser->GetNextSymbol().ToDouble();                       //track length 100502
//    fTrackWidth= Parser->GetNextSymbol().ToDouble();                        //track width
//    fFriction= Parser->GetNextSymbol().ToDouble();                          //friction coeff.
//    fSoundDistance= Parser->GetNextSymbol().ToDouble();   //snd
    parser->getTokens(2);
    *parser >> iQualityFlag >> iDamageFlag;
//    iQualityFlag= Parser->GetNextSymbol().ToInt();   //McZapkie: qualityflag
//    iDamageFlag= Parser->GetNextSymbol().ToInt();   //damage
    parser->getTokens();
    *parser >> token;
    str= AnsiString(token.c_str());  //environment
    if (str=="flat")
     {
       eEnvironment= e_flat;
     }
    else
    if (str=="mountains" || str=="mountain")
     {
       eEnvironment= e_mountains;
     }
    else
    if (str=="canyon")
     {
       eEnvironment= e_canyon;
     }
    else
    if (str=="tunnel")
     {
       eEnvironment= e_tunnel;
     }
    else
    if (str=="bridge")
     {
       eEnvironment= e_bridge;
     }
    else
    if (str=="bank")
     {
       eEnvironment= e_bank;
     }
    else
       {
        eEnvironment= e_unknown;
        Error("Unknown track environment: \""+str+"\"");
       }
    parser->getTokens();
    *parser >> token;
    bVisible= (token.compare( "vis" ) == 0 );   //visible
    if (bVisible)
     {
      parser->getTokens();
      *parser >> token;
      str= AnsiString(token.c_str());   //railtex
      TextureID1= (str=="none"?0:TTexturesManager::GetTextureID(str.c_str()));
      parser->getTokens();
      *parser >> fTexLength; //tex tile length
      parser->getTokens();
      *parser >> token;
      str= AnsiString(token.c_str());   //sub || railtex
      TextureID2= (str=="none"?0:TTexturesManager::GetTextureID(str.c_str()));
      parser->getTokens(3);
      *parser >> fTexHeight >> fTexWidth >> fTexSlope;
//      fTexHeight= Parser->GetNextSymbol().ToDouble(); //tex sub height
//      fTexWidth= Parser->GetNextSymbol().ToDouble(); //tex sub width
//      fTexSlope= Parser->GetNextSymbol().ToDouble(); //tex sub slope width
     }
    else
     {
     if (DebugModeFlag)
      WriteLog("unvis");
     }
    Init();
    double segsize=5.0f;
    switch (eType)
    {
        case tt_Turn: //obrotnica jest prawie jak zwyk³y tor
        case tt_Normal:
            p1= LoadPoint(parser)+pOrigin; //pobranie wspó³rzêdnych P1
            parser->getTokens();
            *parser >> r1; //pobranie przechy³ki w P1

            cp1= LoadPoint(parser); //pobranie wspó³rzêdnych punktów kontrolnych
            cp2= LoadPoint(parser);

            p2= LoadPoint(parser)+pOrigin; //pobranie wspó³rzêdnych P2
            parser->getTokens(2);
            *parser >> r2 >> fRadius; //pobranie przechy³ki w P1 i promienia

            if (fRadius!=0) //gdy podany promieñ
               segsize=Min0R(5.0,0.2+fabs(fRadius)*0.02); //do 250m - 5, potem 1 co 50m

            if ((cp1==vector3(0,0,0)) && (cp2==vector3(0,0,0))) //Ra: hm, czasem dla prostego s¹ podane...
             Segment->Init(p1,p2,segsize,r1,r2); //gdy prosty, kontrolne wylicza
            else
             Segment->Init(p1,cp1+p1,cp2+p2,p2,segsize,r1,r2); //gdy ³uk (ustawia bCurve=true)
            if ((r1!=0)||(r2!=0)) iTrapezoid=1; //s¹ przechy³ki do uwzglêdniania w rysowaniu
        break;

        case tt_Switch:
            //state=0; // Parser->GetNextSymbol().ToInt();

            SwitchExtension= new TSwitchExtension(); //zwrotnica ma doklejkê

            p1= LoadPoint(parser)+pOrigin; //pobranie wspó³rzêdnych P1
            parser->getTokens();
            *parser >> r1;
            cp1= LoadPoint(parser);
            cp2= LoadPoint(parser);
            p2= LoadPoint(parser)+pOrigin; //pobranie wspó³rzêdnych P2
            parser->getTokens(2);
            *parser >> r2 >> fRadiusTable[0];

            if (fRadiusTable[0]!=0)
               segsize=Min0R(5.0,0.2+fabs(fRadiusTable[0])*0.02);

            if (!(cp1==vector3(0,0,0)) && !(cp2==vector3(0,0,0)))
                SwitchExtension->Segments[0].Init(p1,cp1+p1,cp2+p2,p2,segsize,r1,r2);
            else
                SwitchExtension->Segments[0].Init(p1,p2,segsize,r1,r2);

            p1= LoadPoint(parser)+pOrigin; //pobranie wspó³rzêdnych P3
            parser->getTokens();
            *parser >> r1;
            cp1= LoadPoint(parser);
            cp2= LoadPoint(parser);
            p2= LoadPoint(parser)+pOrigin; //pobranie wspó³rzêdnych P4
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

            Switch(0); //na sta³e w po³o¿eniu 0 - nie ma pocz¹tkowego stanu zwrotnicy we wpisie

        break;
        case tt_Cross: //skrzy¿owanie dróg
        //Ra: do przemyœlenia i zrobienia
        break;
    }
    parser->getTokens();
    *parser >> token;
    str= AnsiString(token.c_str());
    while (str!="endtrack")
    {
        if (str=="event0")
         {
            parser->getTokens();
            *parser >> token;
            asEvent0Name= AnsiString(token.c_str());
         }
        else
        if (str=="event1")
         {
            parser->getTokens();
            *parser >> token;
            asEvent1Name= AnsiString(token.c_str());
         }
        else
        if (str=="event2")
         {
            parser->getTokens();
            *parser >> token;
            asEvent2Name= AnsiString(token.c_str());
         }
        else
        if (str=="eventall0")
         {
            parser->getTokens();
            *parser >> token;
            asEventall0Name= AnsiString(token.c_str());
         }
        else
        if (str=="eventall1")
         {
            parser->getTokens();
            *parser >> token;
            asEventall1Name= AnsiString(token.c_str());
         }
        else
        if (str=="eventall2")
         {
            parser->getTokens();
            *parser >> token;
            asEventall2Name= AnsiString(token.c_str());
         }
        else
        if (str=="velocity")
         {
           parser->getTokens();
           *parser >> fVelocity;
//            fVelocity= Parser->GetNextSymbol().ToDouble(); //*0.28; McZapkie-010602
         }
        else
            Error("Unknown track property: \""+str+"\"");
       parser->getTokens(); *parser >> token;
       str= AnsiString(token.c_str());
    }
}

bool __fastcall TTrack::AssignEvents(TEvent *NewEvent0, TEvent *NewEvent1, TEvent *NewEvent2)
{
    bool bError= false;
    if (!Event0)
    {
        if (NewEvent0)
        {
            Event0= NewEvent0;
            asEvent0Name= "";
        }
        else
        {
            if (asEvent0Name!=AnsiString(""))
            {
                Error(AnsiString("Event0 \"")+asEvent0Name+AnsiString("\" does not exist"));
                bError= true;
            }
        }
    }
    else
    {
        Error(AnsiString("Event 0 cannot be assigned to track, track already has one"));
        bError= true;
    }

    if (!Event1)
    {
        if (NewEvent1)
        {
            Event1= NewEvent1;
            asEvent1Name= "";
        }
        else
        {
            if (asEvent1Name!=AnsiString(""))
            {
                Error(AnsiString("Event1 \"")+asEvent1Name+AnsiString("\" does not exist"));
                bError= true;
            }
        }
    }
    else
    {
        Error(AnsiString("Event 1 cannot be assigned to track, track already has one"));
        bError= true;
    }

    if (!Event2)
    {
        if (NewEvent2)
        {
            Event2= NewEvent2;
            asEvent2Name= "";
        }
        else
        {
            if (asEvent2Name!=AnsiString(""))
            {
                Error(AnsiString("Event2 \"")+asEvent2Name+AnsiString("\" does not exist"));
                bError= true;
            }
        }
    }
    else
    {
        Error(AnsiString("Event 2 cannot be assigned to track, track already has one"));
        bError= true;
    }
    return !bError;
}

bool __fastcall TTrack::AssignallEvents(TEvent *NewEvent0, TEvent *NewEvent1, TEvent *NewEvent2)
{
    bool bError= false;
    if (!Eventall0)
    {
        if (NewEvent0)
        {
            Eventall0= NewEvent0;
            asEventall0Name= "";
        }
        else
        {
            if (asEventall0Name!=AnsiString(""))
            {
                Error(AnsiString("Eventall 0 \"")+asEventall0Name+AnsiString("\" does not exist"));
                bError= true;
            }
        }
    }
    else
    {
        Error(AnsiString("Eventall 0 cannot be assigned to track, track already has one"));
        bError= true;
    }

    if (!Eventall1)
    {
        if (NewEvent1)
        {
            Eventall1= NewEvent1;
            asEventall1Name= "";
        }
        else
        {
            if (asEventall1Name!=AnsiString(""))
            {
                Error(AnsiString("Eventall 1 \"")+asEventall1Name+AnsiString("\" does not exist"));
                bError= true;
            }
        }
    }
    else
    {
        Error(AnsiString("Eventall 1 cannot be assigned to track, track already has one"));
        bError= true;
    }

    if (!Eventall2)
    {
        if (NewEvent2)
        {
            Eventall2= NewEvent2;
            asEventall2Name= "";
        }
        else
        {
            if (asEventall2Name!=AnsiString(""))
            {
                Error(AnsiString("Eventall 2 \"")+asEventall2Name+AnsiString("\" does not exist"));
                bError= true;
            }
        }
    }
    else
    {
        Error(AnsiString("Eventall 2 cannot be assigned to track, track already has one"));
        bError= true;
    }
    return !bError;
}


//ABu: przeniesione z Track.h i poprawione!!!
bool __fastcall TTrack::AddDynamicObject(TDynamicObject *Dynamic)
    {
        if (iNumDynamics<iMaxNumDynamics)
        {
            Dynamics[iNumDynamics]= Dynamic;
            iNumDynamics++;
            Dynamic->MyTrack = this; //ABu: Na ktorym torze jestesmy.
            return true;
        }
        else
        {
            Error("Cannot add dynamic to track");
            return false;
        }
    };

void TTrack::MoveMe(vector3 pPosition)
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

const int numPts= 4;
const int nnumPts= 12;

void TTrack::Compile()
{//przygotowanie trójk¹tów do wyœwielenia - model proceduralny
    if(DisplayListID)
        Release(); //zwolnienie zasobów

    if(Global::bManageNodes)
    {
        DisplayListID = glGenLists(1); //otwarcie nowej listy
        glNewList(DisplayListID, GL_COMPILE);
    };

    glColor3f(1.0f,1.0f,1.0f);

    //McZapkie-310702: zmiana oswietlenia w tunelu, wykopie
    GLfloat  ambientLight[4]  = {0.5f, 0.5f, 0.5f, 1.0f};
    GLfloat  diffuseLight[4]  = {0.5f, 0.5f, 0.5f, 1.0f};
    GLfloat  specularLight[4] = {0.5f, 0.5f, 0.5f, 1.0f};

    switch (eEnvironment)
    {//modyfikacje oœwietlenia zale¿nie od œrodowiska
        case e_canyon: //wykop
            for(int li=0; li<3; li++)
            {
                ambientLight[li]= Global::ambientDayLight[li]*0.7;
                diffuseLight[li]= Global::diffuseDayLight[li]*0.3;
                specularLight[li]= Global::specularDayLight[li]*0.4;
            }
            glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);
	        glLightfv(GL_LIGHT0,GL_SPECULAR,specularLight);
        break;
        case e_tunnel: //tunel
            for(int li=0; li<3; li++)
            {
                ambientLight[li]= Global::ambientDayLight[li]*0.2;
                diffuseLight[li]= Global::diffuseDayLight[li]*0.1;
                specularLight[li]= Global::specularDayLight[li]*0.2;
            }
            glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);
            glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);
            glLightfv(GL_LIGHT0,GL_SPECULAR,specularLight);
        break;
    }

    double fHTW=fTrackWidth/2;
    double rozp=fabs(fTexWidth)+fabs(fTexSlope); //podsypka na zewn¹trz szyny albo pobocze
    double fHTW2=(iTrapezoid&2)?pNext->fTrackWidth/2:fHTW; //po³owa rozstawu/nawierzchni
    double rozp2=(iTrapezoid&2)?fabs(pNext->fTexWidth)+fabs(pNext->fTexSlope):fabs(fTexWidth)+fabs(fTexSlope);
    double roll1,roll2;
    switch (iCategoryFlag)
    {
     case 1:   //tor
      {
      // zwykla szyna: //Ra: czemu g³ówki s¹ asymetryczne na wysokoœci 0.140?
        vector3 rpts1[nnumPts]= { vector3( fHTW+0.111,0.000,0.00),vector3( fHTW+0.045,0.025,0.15), //lewa
                                  vector3( fHTW+0.045,0.110,0.25),vector3( fHTW+0.073,0.140,0.35),
                                  vector3( fHTW+0.072,0.170,0.40),vector3( fHTW+0.052,0.180,0.45),
                                  vector3( fHTW+0.020,0.180,0.55),vector3( fHTW      ,0.170,0.60),
                                  vector3( fHTW+0.001,0.140,0.65),vector3( fHTW+0.027,0.110,0.75),
                                  vector3( fHTW+0.027,0.025,0.85),vector3( fHTW-0.039,0.000,1.00) };

        vector3 rpts2[nnumPts]= { vector3(-fHTW+0.039,0.000,1.00),vector3(-fHTW-0.027,0.025,0.85), //prawa
                                  vector3(-fHTW-0.027,0.110,0.75),vector3(-fHTW-0.001,0.140,0.65),
                                  vector3(-fHTW      ,0.170,0.60),vector3(-fHTW-0.020,0.180,0.55),
                                  vector3(-fHTW-0.052,0.180,0.45),vector3(-fHTW-0.072,0.170,0.40),
                                  vector3(-fHTW-0.073,0.140,0.35),vector3(-fHTW-0.045,0.110,0.25),
                                  vector3(-fHTW-0.045,0.025,0.15),vector3(-fHTW-0.111,0.000,0.00) };

        vector3 rpts3[nnumPts]= { vector3( fHTW+0.010,0.000,0.00),vector3( fHTW+0.010,0.025,0.15), //pocz¹tek iglicy lewej
                                  vector3( fHTW+0.010,0.110,0.25),vector3( fHTW+0.010,0.140,0.35),
                                  vector3( fHTW+0.010,0.170,0.40),vector3( fHTW+0.010,0.180,0.45),
                                  vector3( fHTW      ,0.180,0.55),vector3( fHTW      ,0.170,0.60),
                                  vector3( fHTW      ,0.140,0.65),vector3( fHTW      ,0.110,0.75),
                                  vector3( fHTW      ,0.025,0.85),vector3( fHTW-0.040,0.000,1.00) };

        vector3 rpts4[nnumPts]= { vector3(-fHTW+0.040,0.000,1.00),vector3(-fHTW      ,0.025,0.85), //pocz¹tek iglicy prawej
                                  vector3(-fHTW      ,0.110,0.75),vector3(-fHTW      ,0.140,0.65),
                                  vector3(-fHTW      ,0.170,0.60),vector3(-fHTW      ,0.180,0.55),
                                  vector3(-fHTW-0.010,0.180,0.45),vector3(-fHTW-0.010,0.170,0.40),
                                  vector3(-fHTW-0.010,0.140,0.35),vector3(-fHTW-0.010,0.110,0.25),
                                  vector3(-fHTW-0.010,0.025,0.15),vector3(-fHTW-0.010,0.000,0.00) };

// podsypka z podkladami:
        vector3 bpts1[numPts]= { vector3(fHTW+rozp,-fTexHeight,0.0), //lewy brzeg
                                 vector3(fHTW+fTexWidth,0.0,0.33), //krawêdŸ za³amania
                                 vector3(-fHTW-fTexWidth,0.0,0.67), //druga
                                 vector3(-fHTW-rozp,-fTexHeight,1.0) }; //prawy skos

        switch (eType)
        {
            case tt_Normal:
// podsypka z podkladami:
                if (TextureID2)
                {
                    glBindTexture(GL_TEXTURE_2D, TextureID2);
                    Segment->RenderLoft(bpts1,numPts,fTexLength);
                }
// szyny
                if (TextureID1)
                {
                    glBindTexture(GL_TEXTURE_2D, TextureID1);
                    Segment->RenderLoft(rpts1,nnumPts,fTexLength);
                    Segment->RenderLoft(rpts2,nnumPts,fTexLength);
                }
            break;
            case tt_Switch:

                if (TextureID1)
                {
                    SwitchExtension->fDesiredOffset1;
                    double v = (SwitchExtension->fDesiredOffset1-SwitchExtension->fOffset1);
                    SwitchExtension->fOffset1+= sign(v)*Timer::GetDeltaTime()*0.1;
                    if (SwitchExtension->fOffset1<=0.01)
                        SwitchExtension->fOffset1= 0.01;
                    if (SwitchExtension->fOffset1>=fMaxOffset-0.01)
                        SwitchExtension->fOffset1= fMaxOffset-0.01;
//McZapkie-130302 - poprawione rysowanie szyn
                    glBindTexture(GL_TEXTURE_2D, TextureID1);
                    SwitchExtension->Segments[0].RenderLoft(rpts1,nnumPts,fTexLength); //lewa szyna normalna ca³a
                    SwitchExtension->Segments[0].RenderLoft(rpts2,nnumPts,fTexLength,2); //prawa szyna za iglic¹
                    SwitchExtension->Segments[0].RenderSwitchRail(rpts2,rpts4,nnumPts,fTexLength,2,-SwitchExtension->fOffset1); //prawa iglica
                    glBindTexture(GL_TEXTURE_2D, TextureID2);
                    SwitchExtension->Segments[1].RenderLoft(rpts1,nnumPts,fTexLength,2); //lewa szyna za iglic¹
                    SwitchExtension->Segments[1].RenderSwitchRail(rpts1,rpts3,nnumPts,fTexLength,2,fMaxOffset-SwitchExtension->fOffset1); //lewa iglica
                    SwitchExtension->Segments[1].RenderLoft(rpts2,nnumPts,fTexLength); //prawa szyna normalnie ca³a
                }
            break;
        }

      }
     break;
     case 2:   //McZapkie-260302 - droga - rendering
//McZapkie:240702-zmieniony zakres widzialnosci
     {vector3 bpts1[4]; //wyliczone punkty przydadz¹ siê dalej
      if (TextureID1||TextureID2) //punkty siê przydadz¹, nawet jeœli nawierzchni nie ma
       if (iTrapezoid) //trapez albo przechy³ki
       {//nawierzchnia trapezowata - zmienic ewentualnie mapowanie tekstury z wê¿szej strony
        Segment->GetRolls(roll1,roll2);
        //double sin1=sin(roll1),cos1=cos(roll1),sin2=sin(roll2),cos2=cos(roll2);
        bpts1[0]=vector3(fHTW*cos(roll1),-fHTW*sin(roll1),0.0); //lewy brzeg pocz¹tku
        bpts1[1]=vector3(-bpts1[0].x,-bpts1[0].y,1.0); //prawy brzeg pocz¹tku symetrycznie
        bpts1[2]=vector3(fHTW2*cos(roll2),-fHTW2*sin(roll2),0.0); //lewy brzeg koñca
        bpts1[3]=vector3(-bpts1[2].x,-bpts1[2].y,1.0); //prawy brzeg pocz¹tku symetrycznie
       }
       else
       {bpts1[0]=vector3(fHTW,0.0,0.0); //zawsze standardowe mapowanie
        bpts1[1]=vector3(-fHTW,0.0,1.0);
       }
      if (TextureID1) //jeœli podana by³a tekstura, generujemy trójk¹ty
      {//tworzenie trójk¹tów nawierzchnii szosy
       glBindTexture(GL_TEXTURE_2D, TextureID1);
       Segment->RenderLoft(bpts1,iTrapezoid?-2:2,fTexLength);
      }
      if (TextureID2)
      {//pobocze drogi - poziome przy przechy³ce (a mo¿e krawê¿nik i chodnik zrobiæ jak w Midtown Madness 2?)
       glBindTexture(GL_TEXTURE_2D, TextureID2);
       vector3 rpts1[6],rpts2[6]; //wspó³rzêdne przekroju i mapowania dla prawej i lewej strony
       rpts1[0]=vector3(fHTW+rozp,-fTexHeight,0.0); //lewy brzeg podstawy
       rpts1[1]=vector3(bpts1[0].x+fTexWidth,bpts1[0].y,0.5), //lewa krawêdŸ za³amania
       rpts1[2]=vector3(bpts1[0].x,bpts1[0].y,1.0); //lewy brzeg pobocza (mapowanie mo¿e byæ inne
       rpts2[0]=vector3(bpts1[1].x,bpts1[1].y,1.0); //prawy brzeg pobocza
       rpts2[1]=vector3(bpts1[1].x-fTexWidth,bpts1[1].y,0.5); //prawa krawêdŸ za³amania
       rpts2[2]=vector3(-fHTW-rozp,-fTexHeight,0.0); //prawy brzeg podstawy
       if (iTrapezoid) //trapez albo przechy³ki
       {//pobocza do trapezowatej nawierzchni - dodatkowe punkty z drugiej strony odcinka
        rpts1[3]=vector3(fHTW2+rozp2,-pNext->fTexHeight,0.0); //lewy brzeg lewego pobocza
        rpts1[4]=vector3(bpts1[2].x+pNext->fTexWidth,bpts1[2].y,0.5); //krawêdŸ za³amania
        rpts1[5]=vector3(bpts1[2].x,bpts1[2].y,1.0); //brzeg pobocza
        rpts2[3]=vector3(bpts1[3].x,bpts1[3].y,1.0);
        rpts2[4]=vector3(bpts1[3].x-pNext->fTexWidth,bpts1[3].y,0.5);
        rpts2[5]=vector3(-fHTW2-rozp2,-pNext->fTexHeight,0.0); //prawy brzeg prawego pobocza
        Segment->RenderLoft(rpts1,-3,fTexLength);
        Segment->RenderLoft(rpts2,-3,fTexLength);
       }
       else
       {//pobocza zwyk³e, brak przechy³ki
        Segment->RenderLoft(rpts1,3,fTexLength);
        Segment->RenderLoft(rpts2,3,fTexLength);
       }
      }
     }
     break;
     case 4:   //McZapkie-260302 - rzeka- rendering
         vector3 bpts1[numPts]= { vector3(fHTW,0.0,0.0),vector3(fHTW,0.2,0.33),
                                vector3(-fHTW,0.0,0.67),vector3(-fHTW,0.0,1.0) };
         //Ra: dziwnie ten kszta³t wygl¹da
         if(TextureID1)
         {
             glBindTexture(GL_TEXTURE_2D, TextureID1);
             Segment->RenderLoft(bpts1,numPts,fTexLength);
             if(TextureID2)
             {//brzegi rzeki prawie jak pobocze derogi, tylko inny znak ma wysokoœæ
                 vector3 rpts1[3]= { vector3(fHTW+rozp,fTexHeight,0.0),
                                     vector3(fHTW+fTexWidth,0.0,0.5),
                                     vector3(fHTW,0.0,1.0) };
                 vector3 rpts2[3]= { vector3(-fHTW,0.0,1.0),
                                     vector3(-fHTW-fTexWidth,0.0,0.5),
                                     vector3(-fHTW-rozp,fTexHeight,0.1) }; //Ra: po kiego 0.1?
                 glBindTexture(GL_TEXTURE_2D, TextureID2);      //brzeg rzeki
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

    glDeleteLists(DisplayListID, 1);
    DisplayListID = 0;

};

bool __fastcall TTrack::Render()
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
              glBindTexture(GL_TEXTURE_2D, 0);
              glBegin(GL_LINE_STRIP);
                  pos1= Segment->FastGetPoint(0);
                  pos2= Segment->FastGetPoint(0.5);
                  pos3= Segment->FastGetPoint(1);
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
   return true;

}

bool __fastcall TTrack::RenderAlpha()
{
    glColor3f(1.0f,1.0f,1.0f);
//McZapkie-310702: zmiana oswietlenia w tunelu, wykopie
    GLfloat  ambientLight[4]= { 0.5f,  0.5f, 0.5f, 1.0f };
    GLfloat  diffuseLight[4]= { 0.5f,  0.5f, 0.5f, 1.0f };
    GLfloat  specularLight[4]= { 0.5f,  0.5f, 0.5f, 1.0f };
    switch (eEnvironment)
    {
     case e_canyon:
      {
        for (int li=0; li<3; li++)
         {
           ambientLight[li]= Global::ambientDayLight[li]*0.8;
           diffuseLight[li]= Global::diffuseDayLight[li]*0.4;
           specularLight[li]= Global::specularDayLight[li]*0.5;
         }
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
           specularLight[li]= Global::specularDayLight[li]*0.2;
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
   glLightModelfv(GL_LIGHT_MODEL_AMBIENT,Global::ambientDayLight);
   glLightfv(GL_LIGHT0,GL_DIFFUSE,Global::diffuseDayLight);
   glLightfv(GL_LIGHT0,GL_SPECULAR,Global::specularDayLight);
   return true;

}


//---------------------------------------------------------------------------

#pragma package(smart_init)
