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
#pragma hdrstop

#include "AnimModel.h"
#include "usefull.h"
#include "Timer.h"
#include "MdlMngr.h"
//McZapkie:
#include "Texture.h"
#include "Globals.h"


const double fOnTime= 0.66;
const double fOffTime= fOnTime+0.66;

__fastcall TAnimContainer::TAnimContainer()
{
    pNext= NULL;
//    fAngle= 0.0f;
//    vRotateAxis= vector3(0.0f,0.0f,0.0f);
    vRotateAngles= vector3(0.0f,0.0f,0.0f);
//    fDesiredAngle= 0.0f;
    vDesiredAngles= vector3(0.0f,0.0f,0.0f);
    fRotateSpeed= 0.0f;
}

__fastcall TAnimContainer::~TAnimContainer()
{
    SafeDelete(pNext);
}

bool __fastcall TAnimContainer::Init(TSubModel *pNewSubModel)
{
//    fAngle= 0.0f;
    fRotateSpeed= 0.0f;
    pSubModel= pNewSubModel;
    return (pSubModel!=NULL);
}
/*
void __fastcall TAnimContainer::SetRotateAnim(vector3 vNewRotateAxis, double fNewDesiredAngle, double fNewRotateSpeed, bool bResetAngle)
{
    fRotateSpeed= fNewRotateSpeed;
    vRotateAxis= Normalize(vNewRotateAxis);
    fDesiredAngle= fNewDesiredAngle;
    if (bResetAngle)
        fAngle= 0;
} */

void __fastcall TAnimContainer::SetRotateAnim(vector3 vNewRotateAngles, double fNewRotateSpeed, bool bResetAngle)
{
    vDesiredAngles= vNewRotateAngles;
    fRotateSpeed= fNewRotateSpeed;
}


bool __fastcall TAnimContainer::UpdateModel()
{
    if (pSubModel)
    {
        if (fRotateSpeed!=0)
        {
/*
            double dif= fDesiredAngle-fAngle;
            double s= fRotateSpeed*sign(dif)*Timer::GetDeltaTime();
            if ((abs(s)-abs(dif))>0)
                fAngle= fDesiredAngle;
            else
                fAngle+= s;

            while (fAngle>360) fAngle-= 360;
            while (fAngle<-360) fAngle+= 360;
            pSubModel->SetRotate(vRotateAxis,fAngle);*/

            vector3 dif= vDesiredAngles-vRotateAngles;
            double s;
            s= fRotateSpeed*sign(dif.x)*Timer::GetDeltaTime();
            if ((abs(s)-abs(dif.x))>-0.1)
                vRotateAngles.x= vDesiredAngles.x;
            else
                vRotateAngles.x+= s;

            s= fRotateSpeed*sign(dif.y)*Timer::GetDeltaTime();
            if ((abs(s)-abs(dif.y))>-0.1)
                vRotateAngles.y= vDesiredAngles.y;
            else
                vRotateAngles.y+= s;

            s= fRotateSpeed*sign(dif.z)*Timer::GetDeltaTime();
            if ((abs(s)-abs(dif.z))>-0.1)
                vRotateAngles.z= vDesiredAngles.z;
            else
                vRotateAngles.z+= s;

            while (vRotateAngles.x>360) vRotateAngles.x-= 360;
            while (vRotateAngles.x<-360) vRotateAngles.x+= 360;
            while (vRotateAngles.y>360) vRotateAngles.y-= 360;
            while (vRotateAngles.y<-360) vRotateAngles.y+= 360;
            while (vRotateAngles.z>360) vRotateAngles.z-= 360;
            while (vRotateAngles.z<-360) vRotateAngles.z+= 360;

            pSubModel->SetRotateXYZ(vRotateAngles);
        }

    }
//    if (pNext)
  //      pNext->UpdateModel();
}

//---------------------------------------------------------------------------

__fastcall TAnimModel::TAnimModel()
{
    pRoot= NULL;
    pModel= NULL;
    iNumLights= 0;
    fBlinkTimer= 0;
    ReplacableSkinId= 0;
    for (int i=0; i<iMaxNumLights; i++)
    {
        LightsOn[i]=LightsOff[i]= NULL;
        lsLights[i]= ls_Off;
    }
}

__fastcall TAnimModel::~TAnimModel()
{
    SafeDelete(pRoot);
}

bool __fastcall TAnimModel::Init(TModel3d *pNewModel)
{
    fBlinkTimer= double(random(1000*fOffTime))/(1000*fOffTime);;
    pModel= pNewModel;
    return (pModel!=NULL);
}

bool __fastcall TAnimModel::Init(AnsiString asName, AnsiString asReplacableTexture)
{
//    asName="models//"+asName;
    if (asReplacableTexture!=AnsiString("none"))
      ReplacableSkinId= TTexturesManager::GetTextureID(asReplacableTexture.c_str());
    return (Init(TModelsManager::GetModel(asName.c_str())));
}

bool __fastcall TAnimModel::Load(cParser *parser)
{
    AnsiString str;
    std::string token;
    parser->getTokens();
    *parser >> token;
    str= AnsiString(token.c_str());

    parser->getTokens();
    *parser >> token;
    if (!Init(str,AnsiString(token.c_str())))
    if (str!="notload")
    {
        Error(AnsiString("Model: "+str+" Does not exist"));
        return false;
    }

    LightsOn[0]= pModel->GetFromName("Light_On00");
    LightsOn[1]= pModel->GetFromName("Light_On01");
    LightsOn[2]= pModel->GetFromName("Light_On02");
    LightsOn[3]= pModel->GetFromName("Light_On03");
    LightsOn[4]= pModel->GetFromName("Light_On04");
    LightsOn[5]= pModel->GetFromName("Light_On05");
    LightsOn[6]= pModel->GetFromName("Light_On06");
    LightsOn[7]= pModel->GetFromName("Light_On07");

    LightsOff[0]= pModel->GetFromName("Light_Off00");
    LightsOff[1]= pModel->GetFromName("Light_Off01");
    LightsOff[2]= pModel->GetFromName("Light_Off02");
    LightsOff[3]= pModel->GetFromName("Light_Off03");
    LightsOff[4]= pModel->GetFromName("Light_Off04");
    LightsOff[5]= pModel->GetFromName("Light_Off05");
    LightsOff[6]= pModel->GetFromName("Light_Off06");
    LightsOff[7]= pModel->GetFromName("Light_Off07");

    for (int i=0; i<iMaxNumLights; i++)
    {
        if (LightsOn[i] && LightsOff[i]);
//            lsSwiatla[i]= ls_Off;
        else
        {
            iNumLights= i;
            break;
        }
    }

    int i=0;
    int ti;

    parser->getTokens();
    *parser >> token;

    if (token.compare( "lights" ) == 0)
     {
      parser->getTokens();
      *parser >> token;
      str= AnsiString(token.c_str());
      do
      {
        ti= str.ToInt();
        if (i<iMaxNumLights)
            lsLights[i]= ti;
        i++;
 //        if (Parser->EndOfFile)
 //            break;
        parser->getTokens();
        *parser >> token;
        str= AnsiString(token.c_str());
      } while (str!="endmodel");
     }
}

TAnimContainer* __fastcall TAnimModel::AddContainer(char *pName)
{
    TSubModel *tsb= pModel->GetFromName(pName);
    if (tsb)
    {
        TAnimContainer *tmp= new TAnimContainer();
        tmp->Init(tsb);
        tmp->pNext= pRoot;
        pRoot= tmp;
        return tmp;
    }
    return NULL;
}

TAnimContainer* __fastcall TAnimModel::GetContainer(char *pName)
{
    TAnimContainer *pCurrent;
    for (pCurrent= pRoot; pCurrent!=NULL; pCurrent=pCurrent->pNext)
        if (pCurrent->GetName() == pName)
            return pCurrent;
    return AddContainer(pName);
}

bool __fastcall TAnimModel::Render(vector3 pPosition, double fAngle)
{
    fBlinkTimer-= Timer::GetDeltaTime();
    if (fBlinkTimer<=0)
        fBlinkTimer= fOffTime;

    for (int i=0; i<iNumLights; i++)
//        if (LightsOn[i])
            if (lsLights[i]==ls_Blink)
            {
                LightsOn[i]->Visible= (fBlinkTimer<fOnTime);
                LightsOff[i]->Visible= !(fBlinkTimer<fOnTime);
            }
            else
            {
                LightsOn[i]->Visible= (lsLights[i]==ls_On);
                LightsOff[i]->Visible= (lsLights[i]==ls_Off);
            }

    TAnimContainer *pCurrent;
    for (pCurrent= pRoot; pCurrent!=NULL; pCurrent=pCurrent->pNext)
        pCurrent->UpdateModel();
    if (pModel)
        pModel->Render(pPosition, fAngle, ReplacableSkinId);
}

bool __fastcall TAnimModel::Render(double fSquareDistance)
{
    fBlinkTimer-= Timer::GetDeltaTime();
    if (fBlinkTimer<=0)
        fBlinkTimer= fOffTime;

    for (int i=0; i<iNumLights; i++)
//        if (LightsOn[i])
            if (lsLights[i]==ls_Blink)
            {
                LightsOn[i]->Visible= (fBlinkTimer<fOnTime);
                LightsOff[i]->Visible= !(fBlinkTimer<fOnTime);
            }
            else
            {
                LightsOn[i]->Visible= (lsLights[i]==ls_On);
                LightsOff[i]->Visible= (lsLights[i]==ls_Off);
            }

    TAnimContainer *pCurrent;
    for (pCurrent= pRoot; pCurrent!=NULL; pCurrent=pCurrent->pNext)
        pCurrent->UpdateModel();
    if (pModel)
        pModel->Render(fSquareDistance, ReplacableSkinId);
}

bool __fastcall TAnimModel::RenderAlpha(double fSquareDistance)
{
    fBlinkTimer-= Timer::GetDeltaTime();
    if (fBlinkTimer<=0)
        fBlinkTimer= fOffTime;

    for (int i=0; i<iNumLights; i++)
//        if (LightsOn[i])
            if (lsLights[i]==ls_Blink)
            {
                LightsOn[i]->Visible= (fBlinkTimer<fOnTime);
                LightsOff[i]->Visible= !(fBlinkTimer<fOnTime);
            }
            else
            {
                LightsOn[i]->Visible= (lsLights[i]==ls_On);
                LightsOff[i]->Visible= (lsLights[i]==ls_Off);
            }

    TAnimContainer *pCurrent;
    for (pCurrent= pRoot; pCurrent!=NULL; pCurrent=pCurrent->pNext)
        pCurrent->UpdateModel();
    if (pModel)
        pModel->RenderAlpha(fSquareDistance, ReplacableSkinId);
}

bool __fastcall TAnimModel::RenderAlpha(vector3 pPosition, double fAngle)
{
    fBlinkTimer-= Timer::GetDeltaTime();
    if (fBlinkTimer<=0)
        fBlinkTimer= fOffTime;

    for (int i=0; i<iNumLights; i++)
//        if (LightsOn[i])
            if (lsLights[i]==ls_Blink)
            {
                LightsOn[i]->Visible= (fBlinkTimer<fOnTime);
                LightsOff[i]->Visible= !(fBlinkTimer<fOnTime);
            }
            else
            {
                LightsOn[i]->Visible= (lsLights[i]==ls_On);
                LightsOff[i]->Visible= (lsLights[i]==ls_Off);
            }

    TAnimContainer *pCurrent;
    for (pCurrent= pRoot; pCurrent!=NULL; pCurrent=pCurrent->pNext)
        pCurrent->UpdateModel();
    if (pModel)
        pModel->RenderAlpha(pPosition, fAngle, ReplacableSkinId);
}


//---------------------------------------------------------------------------

#pragma package(smart_init)

