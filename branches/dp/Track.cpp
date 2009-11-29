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
#include <gl/gl.h>
#include <gl/glu.h>
#include "opengl/glut.h"
#pragma hdrstop

#include "Texture.h"
#include "Timer.h"
#include "usefull.h"
#include "Globals.h"
#include "Track.h"


__fastcall TSwitchExtension::TSwitchExtension()
{
    CurrentIndex= 0;
    pNexts[0]= NULL;
    pNexts[1]= NULL;
    pPrevs[0]= NULL;
    pPrevs[1]= NULL;
    fOffset1=fOffset2=fDesiredOffset1=fDesiredOffset2= 0;
}
__fastcall TSwitchExtension::~TSwitchExtension()
{
//    SafeDelete(pNexts[0]);
//    SafeDelete(pNexts[1]);
//    SafeDelete(pPrevs[0]);
//    SafeDelete(pPrevs[1]);
}

__fastcall TTrack::TTrack()
{
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
    fVelocity=-1;   //-1;
    fTrackLength= 100.0;
    fRadius= 0;
    fRadiusTable[0]= 0;
    fRadiusTable[1]= 0;
//    Flags.Reset();
    iNumDynamics= 0;
    ScannedFlag=false;
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

bool __fastcall TTrack::Init()
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

bool __fastcall TTrack::ConnectPrevPrev(TTrack *pTrack)
{
    if (pTrack)
    {
        pPrev= pTrack;
        bPrevSwitchDirection= true;
        pTrack->pPrev= this;
        pTrack->bPrevSwitchDirection= true;
    }
}

bool __fastcall TTrack::ConnectPrevNext(TTrack *pTrack)
{
    if (pTrack)
    {
        pPrev= pTrack;
        bPrevSwitchDirection= false;
        pTrack->pNext= this;
        pTrack->bNextSwitchDirection= false;
    }
}

bool __fastcall TTrack::ConnectNextPrev(TTrack *pTrack)
{
    if (pTrack)
    {
        pNext= pTrack;
        bNextSwitchDirection= false;
        pTrack->pPrev= this;
        pTrack->bPrevSwitchDirection= false;
    }
}

bool __fastcall TTrack::ConnectNextNext(TTrack *pTrack)
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
{
    vector3 p;
    std::string token;
    parser->getTokens(3);
    *parser >> p.x >> p.y >> p.z;
    return p;
}

vector3 __fastcall LoadCPoint(cParser *parser)
{
    vector3 cp;
    std::string token;
    parser->getTokens(3);
    *parser >> cp.x >> cp.y >> cp.z;
//    if (cp.x!=0.0f)
   // {
   //     cp.y= Parser->GetNextSymbol().ToDouble();
   //     cp.z= Parser->GetNextSymbol().ToDouble();
   // }
//    else
  //      cp.y=cp.z= 0.0f;
    return cp;
}

const vector3 sw1pts[]= { vector3(1.378,0.0,25.926),vector3(0.378,0.0,16.926),
                          vector3(0.0,0.0,26.0), vector3(0.0,0.0,26.0),
                          vector3(0.0,0.0,6.0) };

const vector3 sw1pt1= vector3(0.0,0.0,26.0);
const vector3 sw1cpt1= vector3(0.0,0.0,26.0);
const vector3 sw1pt2= vector3(1.378,0.0,25.926);
const vector3 sw1cpt2= vector3(0.378,0.0,16.926);
const vector3 sw1pt3= vector3(0.0,0.0,0.0);
const vector3 sw1cpt3= vector3(0.0,0.0,6.0);

bool __fastcall TTrack::Load(cParser *parser, vector3 pOrigin)
{
    vector3 pt,vec,p1,p2,cp1,cp2,p3,cp3,p4,cp4,p5,cp5,swpt[3],swcp[3],dir;
    double a1,a2,r1,r2,d1,d2,a;
    AnsiString str;
    bool bCurve;
    int i,state;
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
    if (str=="road")
     {
        eType= tt_Normal;
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
//    iCategoryFlag=Parser->GetNextSymbol().ToInt();                        //1: rail, 2:road
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
        case tt_Normal:
            p1= LoadPoint(parser)+pOrigin;
            parser->getTokens();
            *parser >> r1;

            cp1= LoadCPoint(parser);
            cp2= LoadCPoint(parser);

            p2= LoadPoint(parser)+pOrigin;
            parser->getTokens(2);
            *parser >> r2 >> fRadius;

            if (fRadius!=0)
               segsize=Min0R(5.0,0.2+fabs(fRadius)*0.02);

            if ((cp1==vector3(0,0,0)) && (cp2==vector3(0,0,0)))
             Segment->Init(p1,p2,segsize,r1,r2);
            else
                Segment->Init(p1,cp1+p1,cp2+p2,p2,segsize,r1,r2);
        break;

        case tt_Switch:
            state=0; // Parser->GetNextSymbol().ToInt();

            SwitchExtension= new TSwitchExtension();

            p1= LoadPoint(parser)+pOrigin;
            parser->getTokens();
            *parser >> r1;
            cp1= LoadCPoint(parser);
            cp2= LoadCPoint(parser);
            p2= LoadPoint(parser)+pOrigin;
            parser->getTokens(2);
            *parser >> r2 >> fRadiusTable[0];

            if (fRadiusTable[0]!=0)
               segsize=Min0R(5.0,0.2+fabs(fRadiusTable[0])*0.02);

            if (!(cp1==vector3(0,0,0)) && !(cp2==vector3(0,0,0)))
                SwitchExtension->Segments[0].Init(p1,cp1+p1,cp2+p2,p2,segsize,r1,r2);
            else
                SwitchExtension->Segments[0].Init(p1,p2,segsize,r1,r2);

            p1= LoadPoint(parser)+pOrigin;
            parser->getTokens();
            *parser >> r1;
            cp1= LoadCPoint(parser);
            cp2= LoadCPoint(parser);
            p2= LoadPoint(parser)+pOrigin;
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

            Switch(state);

                    vector3 v1,v2;
                    double a1,a2;
                    v1=SwitchExtension->Segments[0].FastGetPoint(1)-SwitchExtension->Segments[0].FastGetPoint(0);
                    v2=SwitchExtension->Segments[1].FastGetPoint(1)-SwitchExtension->Segments[1].FastGetPoint(0);
                    a1=atan2(v1.x,v1.z);
                    a2=atan2(v2.x,v2.z);
                    a2=a2-a1;
                    while (a2>M_PI) a2=a2-2*M_PI;
                    while (a2<-M_PI) a2=a2+2*M_PI;
                    SwitchExtension->RightSwitch =a2>0;

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


const int numPts= 4;

bool __fastcall TTrack::Render()
{
//    int i;
  //  double s,step,invLength,fOffset;
    //vector3 pos1,pos2,dir,parallel1,parallel2,pt;
    double v,dist;
//    Segment->Render();
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
           ambientLight[li]= Global::ambientDayLight[li]*0.7;
           diffuseLight[li]= Global::diffuseDayLight[li]*0.3;
           specularLight[li]= Global::specularDayLight[li]*0.4;
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
    double fHTW=fTrackWidth/2;
    float rozp=fabs(fTexWidth)+fabs(fTexSlope);
    switch (iCategoryFlag)
    {
     case 1:   //tor
      {
// zwykla szyna:
        vector3 rpts1[numPts]= { vector3(fHTW+0.07,0.0,0.0),vector3(fHTW+0.07,0.18,0.33),
                                 vector3(fHTW,0.18,0.67),vector3(fHTW,0.0,1.0) };
        vector3 rpts2[numPts]= { vector3(-fHTW,0.0,1.0),vector3(-fHTW,0.18,0.67),
                                 vector3(-fHTW-0.07,0.18,0.33),vector3(-fHTW-0.07,0.0,0.0) };
// szyna dla zwrotnicy:
        vector3 rpts3[numPts]= { vector3(fHTW+0.05,0.0,0.0),vector3(fHTW+0.05,0.18,0.33),
                                 vector3(fHTW+0.05,0.0,0.67),vector3(fHTW-0.05,0.0,1.0) };
        vector3 rpts4[numPts]= { vector3(-fHTW+0.05,0.0,1.0),vector3(-fHTW-0.05,0.0,0.67),
                                 vector3(-fHTW-0.05,0.18,0.33),vector3(-fHTW-0.05,0.0,0.0) };
// podsypka z podkladami:

          vector3 bpts1[numPts]= { vector3(fHTW+rozp,-fTexHeight,0.0),
                                   vector3(fHTW+fTexWidth,0.0,0.33),
                                   vector3(-fHTW-fTexWidth,0.0,0.67),
                                   vector3(-fHTW-rozp,-fTexHeight,1.0) };

        dist= SquareMagnitude(Global::pCameraPosition-Segment->FastGetPoint(0.5));
        switch (eType)
        {
            case tt_Normal:
                if (bVisible && (dist<810000))
                {
// podsypka z podkladami:
                  if (TextureID2!=0)
                 {
                   glBindTexture(GL_TEXTURE_2D, TextureID2);
                   Segment->RenderLoft(bpts1,numPts,fTexLength,0,Min0R(1,dist/10000));
//                   Segment->RenderLoft(bpts1,numPts,fTexLength,0,dist/10000);
                          //                                 ^^^^^^^^^^
                  }
// szyny
                  if ((dist<90000) && TextureID1!=0)
                   {
                    glBindTexture(GL_TEXTURE_2D, TextureID1);
                    Segment->RenderLoft(rpts1,numPts,fTexLength);
                    Segment->RenderLoft(rpts2,numPts,fTexLength);
                   }
                }
            break;
            case tt_Switch:

                if (bVisible && (dist<90000) && TextureID1!=0)
                {
                    SwitchExtension->fDesiredOffset1;
                    v= (SwitchExtension->fDesiredOffset1-SwitchExtension->fOffset1);
                    SwitchExtension->fOffset1+= sign(v)*Timer::GetDeltaTime()*0.1;
                    if (SwitchExtension->fOffset1<=0.01)
                        SwitchExtension->fOffset1= 0.01;
                    if (SwitchExtension->fOffset1>=fMaxOffset-0.01)
                        SwitchExtension->fOffset1= fMaxOffset-0.01;

//McZapkie-130302 - poprawione rysowanie szyn

                    if(SwitchExtension->RightSwitch)
                    {
                    glBindTexture(GL_TEXTURE_2D, TextureID1);
                    SwitchExtension->Segments[0].RenderLoft(rpts1,numPts,fTexLength);
                    SwitchExtension->Segments[0].RenderLoft(rpts2,numPts,fTexLength,2);
                    SwitchExtension->Segments[0].RenderSwitchRail(rpts2,rpts4,numPts,fTexLength,2,-SwitchExtension->fOffset1);
                    glBindTexture(GL_TEXTURE_2D, TextureID2);
                    SwitchExtension->Segments[1].RenderLoft(rpts1,numPts,fTexLength,2);
                    SwitchExtension->Segments[1].RenderSwitchRail(rpts1,rpts3,numPts,fTexLength,2,fMaxOffset-SwitchExtension->fOffset1);
                    SwitchExtension->Segments[1].RenderLoft(rpts2,numPts,fTexLength);
                    }
                    else
                    {
                    glBindTexture(GL_TEXTURE_2D, TextureID2);
                    SwitchExtension->Segments[1].RenderLoft(rpts1,numPts,fTexLength);
                    SwitchExtension->Segments[1].RenderLoft(rpts2,numPts,fTexLength,2);
                    SwitchExtension->Segments[1].RenderSwitchRail(rpts2,rpts4,numPts,fTexLength,2,-fMaxOffset+SwitchExtension->fOffset1);
                    glBindTexture(GL_TEXTURE_2D, TextureID1);
                    SwitchExtension->Segments[0].RenderLoft(rpts1,numPts,fTexLength,2);
                    SwitchExtension->Segments[0].RenderSwitchRail(rpts1,rpts3,numPts,fTexLength,2,SwitchExtension->fOffset1);
                    SwitchExtension->Segments[0].RenderLoft(rpts2,numPts,fTexLength);
                    }
                }
            break;
        }

      }
     break;
     case 2:   //McZapkie-260302 - droga - rendering
      {
//McZapkie:240702-zmieniony zakres widzialnosci
       if (bVisible && (SquareMagnitude(Global::pCameraPosition-Segment->FastGetPoint(0.5)))<90000*(fHTW+fabs(fTexWidth+fTexSlope)+1))
       {
         if (TextureID1!=0)
          {
/*
            vector3 bpts1[numPts]= { vector3(fHTW+rozp,-fTexHeight,0.0),
                                     vector3(fHTW+fTexWidth,0.0,0.33),
                                     vector3(-fHTW-fTexWidth,0.0,0.67),
                                     vector3(-fHTW-rozp,-fTexHeight,1.0) };
            glBindTexture(GL_TEXTURE_2D, TextureID1);
            Segment->RenderLoft(bpts1,numPts,fTexLength);
*/
            vector3 bpts1[2]= { vector3(fHTW,0.0,0.0),
                                vector3(-fHTW,0.0,1.0) };
            glBindTexture(GL_TEXTURE_2D, TextureID1);      //nawierzchnia szosy
            Segment->RenderLoft(bpts1,2,fTexLength);
          }
         if (TextureID2!=0)
          {
/*
            vector3 bpts1[numPts]= { vector3(fHTW+rozp,-fTexHeight,0.0),
                                     vector3(fHTW+fTexWidth,0.0,0.33),
                                     vector3(-fHTW-fTexWidth,0.0,0.67),
                                     vector3(-fHTW-rozp,-fTexHeight,1.0) };
            glBindTexture(GL_TEXTURE_2D, TextureID1);
            Segment->RenderLoft(bpts1,numPts,fTexLength);
*/
            vector3 rpts1[3]= { vector3(fHTW+rozp,-fTexHeight,0.0),
                                vector3(fHTW+fTexWidth,0.0,0.5),
                                vector3(fHTW,0.0,1.0) };
            vector3 rpts2[3]= { vector3(-fHTW,0.0,1.0),
                                vector3(-fHTW-fTexWidth,0.0,0.5),
                                vector3(-fHTW-rozp,-fTexHeight,0.1) };
            glBindTexture(GL_TEXTURE_2D, TextureID2);      //pobocze drogi
            Segment->RenderLoft(rpts1,3,fTexLength);
            Segment->RenderLoft(rpts2,3,fTexLength);
          }
       }
      }
     break;
     case 4:   //McZapkie-260302 - rzeka- rendering
      {
       vector3 bpts1[numPts]= { vector3(fHTW,0.0,0.0),vector3(fHTW,0.2,0.33),
                                vector3(-fHTW,0.0,0.67),vector3(-fHTW,0.0,1.0) };
//McZapkie:240702-zmieniony zakres widzialnosci
       if (bVisible && (SquareMagnitude(Global::pCameraPosition-Segment->FastGetPoint(0.5)))<90000*(fHTW+fabs(fTexWidth+fTexSlope)+1))
       {
// powierzchnia wody:
         if (TextureID1!=0)
          {
            glBindTexture(GL_TEXTURE_2D, TextureID1);
            Segment->RenderLoft(bpts1,numPts,fTexLength);
             if (TextureID2!=0)
              {
                vector3 rpts1[3]= { vector3(fHTW+rozp,fTexHeight,0.0),
                                    vector3(fHTW+fTexWidth,0.0,0.5),
                                    vector3(fHTW,0.0,1.0) };
                vector3 rpts2[3]= { vector3(-fHTW,0.0,1.0),
                                    vector3(-fHTW-fTexWidth,0.0,0.5),
                                    vector3(-fHTW-rozp,fTexHeight,0.1) };
                glBindTexture(GL_TEXTURE_2D, TextureID2);      //brzeg rzeki
                Segment->RenderLoft(rpts1,3,fTexLength);
                Segment->RenderLoft(rpts2,3,fTexLength);
              }
          }
       }
      }
     break;
    }


    for (int i=0; i<iNumDynamics; i++)
    {
        //if(SquareMagnitude(Global::pCameraPosition-Dynamics[i]->GetPosition())<20000)
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
