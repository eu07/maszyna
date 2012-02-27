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
 pNext=NULL;
 vRotateAngles=vector3(0.0f,0.0f,0.0f); //aktualne k¹ty obrotu
 vDesiredAngles=vector3(0.0f,0.0f,0.0f); //docelowe k¹ty obrotu
 fRotateSpeed=0.0;
 vTranslation=vector3(0.0f,0.0f,0.0f); //aktualne przesuniêcie
 vTranslateTo=vector3(0.0f,0.0f,0.0f); //docelowe przesuniêcie
 fTranslateSpeed=0.0;
 pSubModel=NULL;
 iAnim=0; //po³o¿enie pocz¹tkowe
}

__fastcall TAnimContainer::~TAnimContainer()
{
 SafeDelete(pNext);
}

bool __fastcall TAnimContainer::Init(TSubModel *pNewSubModel)
{
 fRotateSpeed=0.0f;
 pSubModel=pNewSubModel;
 return (pSubModel!=NULL);
}

void __fastcall TAnimContainer::SetRotateAnim(vector3 vNewRotateAngles, double fNewRotateSpeed)
{
 if (!this) return; //wywo³ywane z eventu, gdy brak modelu
 vDesiredAngles=vNewRotateAngles;
 fRotateSpeed=fNewRotateSpeed;
 iAnim|=1;
}

void __fastcall TAnimContainer::SetTranslateAnim(vector3 vNewTranslate, double fNewSpeed)
{
 if (!this) return; //wywo³ywane z eventu, gdy brak modelu
 vTranslateTo=vNewTranslate;
 fTranslateSpeed=fNewSpeed;
 iAnim|=2;
}

void __fastcall TAnimContainer::UpdateModel()
{
 if (pSubModel)
 {
  if (fTranslateSpeed!=0)
  {
   vector3 dif=vTranslateTo-vTranslation;
   vector3 s=SafeNormalize(dif);
   s=fTranslateSpeed*s*Timer::GetDeltaTime();
   if (LengthSquared3(s)>LengthSquared3(dif)) s=dif; //¿eby nie jecha³o na drug¹ stronê
   if (s.x==0?s.y==0?s.z==0:false:false)
   {vTranslation=vTranslateTo; //nie potrzeba przeliczaæ ju¿
    fTranslateSpeed=0.0;
    if (vTranslation.x==0.0)
     if (vTranslation.y==0.0)
      if (vTranslation.z==0.0)
       iAnim&=~2; //przesuniêcie jest zerowe - jest w punkcie pocz¹tkowym
   }
   else
    vTranslation+=s;
  }
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

   bool anim=false;
   vector3 dif=vDesiredAngles-vRotateAngles;
   double s;
   s=fRotateSpeed*sign(dif.x)*Timer::GetDeltaTime();
   if (fabs(s)>=fabs(dif.x))
    vRotateAngles.x=vDesiredAngles.x;
   else
   {vRotateAngles.x+=s; anim=true;}
   s=fRotateSpeed*sign(dif.y)*Timer::GetDeltaTime();
   if (fabs(s)>=fabs(dif.y))
    vRotateAngles.y=vDesiredAngles.y;
   else
   {vRotateAngles.y+=s; anim=true;}
   s=fRotateSpeed*sign(dif.z)*Timer::GetDeltaTime();
   if (fabs(s)>=fabs(dif.z))
    vRotateAngles.z=vDesiredAngles.z;
   else
   {vRotateAngles.z+=s; anim=true;}
   while (vRotateAngles.x>= 360) vRotateAngles.x-=360;
   while (vRotateAngles.x<=-360) vRotateAngles.x+=360;
   while (vRotateAngles.y>= 360) vRotateAngles.y-=360;
   while (vRotateAngles.y<=-360) vRotateAngles.y+=360;
   while (vRotateAngles.z>= 360) vRotateAngles.z-=360;
   while (vRotateAngles.z<=-360) vRotateAngles.z+=360;
   if (vRotateAngles.x==0.0)
    if (vRotateAngles.y==0.0)
     if (vRotateAngles.z==0.0)
      iAnim&=~1; //k¹ty s¹ zerowe
   if (!anim) fRotateSpeed=0.0; //nie potrzeba przeliczaæ ju¿
  }
  if (iAnim&3) //zmieniona pozycja wzglêdem pocz¹tkowej
  {pSubModel->SetTranslate(vTranslation);
   pSubModel->SetRotateXYZ(vRotateAngles); //ustawia typ animacji
  }
 }
}

bool __fastcall TAnimContainer::InMovement()
{//czy trwa animacja - informacja dla obrotnicy
 return fRotateSpeed!=0.0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

__fastcall TAnimModel::TAnimModel()
{
 pRoot=NULL;
 pModel=NULL;
 iNumLights=0;
 fBlinkTimer=0;
 ReplacableSkinId[0]=0;
 ReplacableSkinId[1]=0;
 ReplacableSkinId[2]=0;
 ReplacableSkinId[3]=0;
 ReplacableSkinId[4]=0;
 for (int i=0;i<iMaxNumLights;i++)
 {
  LightsOn[i]=LightsOff[i]=NULL; //normalnie nie ma
  lsLights[i]=ls_Off; //a jeœli s¹, to wy³¹czone
 }
 vAngle.x=vAngle.y=vAngle.z=0.0; //zerowanie obrotów egzemplarza
}

__fastcall TAnimModel::~TAnimModel()
{
 SafeDelete(pRoot);
}

bool __fastcall TAnimModel::Init(TModel3d *pNewModel)
{
 fBlinkTimer=double(random(1000*fOffTime))/(1000*fOffTime);;
 pModel=pNewModel;
 return (pModel!=NULL);
}

bool __fastcall TAnimModel::Init(AnsiString asName, AnsiString asReplacableTexture)
{
 //asName="models//"+asName;
 if (asReplacableTexture!=AnsiString("none"))
  ReplacableSkinId[1]=TTexturesManager::GetTextureID(asReplacableTexture.c_str());
 if (TTexturesManager::GetAlpha(ReplacableSkinId[1]))
  iTexAlpha=0x31310031; //tekstura z kana³em alfa - nie renderowaæ w cyklu nieprzezroczystych
 else
  iTexAlpha=0x30300030; //tekstura nieprzezroczysta - nie renderowaæ w cyklu przezroczystych
 return (Init(TModelsManager::GetModel(asName.c_str())));
}

bool __fastcall TAnimModel::Load(cParser *parser)
{//rozpoznanie wpisu modelu i ustawienie œwiate³
 AnsiString str;
 std::string token;
 parser->getTokens();
 *parser >> token;
 str=AnsiString(token.c_str());
 parser->getTokens();
 *parser >> token;
 if (!Init(str,AnsiString(token.c_str())))
 {if (str!="notload")
  {//gdy brak modelu
   Error(AnsiString("Model: "+str+" does not exist"));
  }
 }
 else
 {//wi¹zanie œwiate³, o ile model wczytany
  LightsOn[0]=pModel->GetFromName("Light_On00");
  LightsOn[1]=pModel->GetFromName("Light_On01");
  LightsOn[2]=pModel->GetFromName("Light_On02");
  LightsOn[3]=pModel->GetFromName("Light_On03");
  LightsOn[4]=pModel->GetFromName("Light_On04");
  LightsOn[5]=pModel->GetFromName("Light_On05");
  LightsOn[6]=pModel->GetFromName("Light_On06");
  LightsOn[7]=pModel->GetFromName("Light_On07");
  LightsOff[0]=pModel->GetFromName("Light_Off00");
  LightsOff[1]=pModel->GetFromName("Light_Off01");
  LightsOff[2]=pModel->GetFromName("Light_Off02");
  LightsOff[3]=pModel->GetFromName("Light_Off03");
  LightsOff[4]=pModel->GetFromName("Light_Off04");
  LightsOff[5]=pModel->GetFromName("Light_Off05");
  LightsOff[6]=pModel->GetFromName("Light_Off06");
  LightsOff[7]=pModel->GetFromName("Light_Off07");
 }
 for (int i=0;i<iMaxNumLights;++i)
  if (LightsOn[i]||LightsOff[i]) //Ra: zlikwidowa³em wymóg istnienia obu
   iNumLights=i+1;

 int i=0;
 int ti;

 parser->getTokens();
 *parser >> token;

 if (token.compare( "lights" ) == 0)
 {
  parser->getTokens();
  *parser >> token;
  str=AnsiString(token.c_str());
  do
  {
   ti=str.ToInt(); //stan œwiat³a jest liczb¹
   if (i<iMaxNumLights)
    lsLights[i]=(TLightState)ti;
   i++;
   parser->getTokens();
   *parser >> token;
   str=AnsiString(token.c_str());
  } while ((str!="endmodel")&&(str!="endterrain"));
 }
 return true;
}

TAnimContainer* __fastcall TAnimModel::AddContainer(char *pName)
{//dodanie submodelu do egzemplarza
 if (!pModel) return NULL;
 TSubModel *tsb=pModel->GetFromName(pName);
 if (tsb)
 {
  TAnimContainer *tmp=new TAnimContainer();
  tmp->Init(tsb);
  tmp->pNext=pRoot;
  pRoot=tmp;
  return tmp;
 }
 return NULL;
}

TAnimContainer* __fastcall TAnimModel::GetContainer(char *pName)
{
 if (!pName) return pRoot; //pobranie pierwszego (dla obrotnicy)
 TAnimContainer *pCurrent;
 for (pCurrent=pRoot;pCurrent!=NULL;pCurrent=pCurrent->pNext)
  if (pCurrent->GetName()==pName)
   return pCurrent;
 return AddContainer(pName);
}

void __fastcall TAnimModel::RaPrepare()
{//ustawia œwiat³a i animacje w modelu przed renderowaniem
 fBlinkTimer-=Timer::GetDeltaTime();
 if (fBlinkTimer<=0) fBlinkTimer=fOffTime;
 bool state; //stan œwiat³a
 for (int i=0;i<iNumLights;i++)
 {switch (lsLights[i])
  {case ls_Blink: //migotanie
    state=fBlinkTimer<fOnTime; break;
   case ls_Dark: //zapalone, gdy ciemno
    state=Global::fLuminance<=0.25; break;
   default: //zapalony albo zgaszony
    state=(lsLights[i]==ls_On);
  }
  if (LightsOn[i])  LightsOn[i]->Visible=state;
  if (LightsOff[i]) LightsOff[i]->Visible=!state;
 }
 TSubModel::iInstance=(int)this; //¿eby nie robiæ cudzych animacji
 TAnimContainer *pCurrent;
 for (pCurrent=pRoot;pCurrent!=NULL;pCurrent=pCurrent->pNext)
  pCurrent->UpdateModel(); //przeliczenie animacji ka¿dego submodelu
}

void __fastcall TAnimModel::RaRender(vector3 pPosition,double fAngle)
{//sprawdza œwiat³a i rekurencyjnie renderuje TModel3d
 RaPrepare();
 if (pModel) //renderowanie rekurencyjne submodeli
  pModel->RaRender(pPosition,fAngle,ReplacableSkinId,iTexAlpha);
}

void __fastcall TAnimModel::RaRenderAlpha(vector3 pPosition,double fAngle)
{
 RaPrepare();
 if (pModel) //renderowanie rekurencyjne submodeli
  pModel->RaRenderAlpha(pPosition,fAngle,ReplacableSkinId,iTexAlpha);
};

void __fastcall TAnimModel::Render(vector3 pPosition,double fAngle)
{
 RaPrepare();
 if (pModel) //renderowanie rekurencyjne submodeli
  pModel->Render(pPosition,fAngle,ReplacableSkinId,iTexAlpha);
}

void __fastcall TAnimModel::RenderAlpha(vector3 pPosition,double fAngle)
{
 RaPrepare();
 if (pModel)
  pModel->RenderAlpha(pPosition,fAngle,ReplacableSkinId,iTexAlpha);
};

int __fastcall TAnimModel::Flags()
{//informacja dla TGround, czy ma byæ w Render, RenderAlpha, czy RenderMixed
 int i=pModel?pModel->Flags():0; //pobranie flag ca³ego modelu
 if (ReplacableSkinId[1]>0) //jeœli ma wymienn¹ teksturê 0
  i|=(i&0x01010001)*((iTexAlpha&1)?0x20:0x10);
 //if (ReplacableSkinId[2]>0) //jeœli ma wymienn¹ teksturê 1
 // i|=(i&0x02020002)*((iTexAlpha&1)?0x10:0x08);
 //if (ReplacableSkinId[3]>0) //jeœli ma wymienn¹ teksturê 2
 // i|=(i&0x04040004)*((iTexAlpha&1)?0x08:0x04);
 //if (ReplacableSkinId[4]>0) //jeœli ma wymienn¹ teksturê 3
 // i|=(i&0x08080008)*((iTexAlpha&1)?0x04:0x02);
 return i;
};

//-----------------------------------------------------------------------------
//2011-03-16 cztery nowe funkcje renderowania z mo¿liwoœci¹ pochylania obiektów
//-----------------------------------------------------------------------------

void __fastcall TAnimModel::Render(vector3* vPosition)
{
 RaPrepare();
 if (pModel) //renderowanie rekurencyjne submodeli
  pModel->Render(vPosition,&vAngle,ReplacableSkinId,iTexAlpha);
};
void __fastcall TAnimModel::RenderAlpha(vector3* vPosition)
{
 RaPrepare();
 if (pModel) //renderowanie rekurencyjne submodeli
  pModel->RenderAlpha(vPosition,&vAngle,ReplacableSkinId,iTexAlpha);
};
void __fastcall TAnimModel::RaRender(vector3* vPosition)
{
 RaPrepare();
 if (pModel) //renderowanie rekurencyjne submodeli
  pModel->RaRender(vPosition,&vAngle,ReplacableSkinId,iTexAlpha);
};
void __fastcall TAnimModel::RaRenderAlpha(vector3* vPosition)
{
 RaPrepare();
 if (pModel) //renderowanie rekurencyjne submodeli
  pModel->RaRenderAlpha(vPosition,&vAngle,ReplacableSkinId,iTexAlpha);
};

//---------------------------------------------------------------------------
bool __fastcall TAnimModel::TerrainLoded()
{//zliczanie kwadratów kilometrowych (g³ówna linia po Next) do tworznia tablicy
 return pModel;
};
int __fastcall TAnimModel::TerrainCount()
{//zliczanie kwadratów kilometrowych (g³ówna linia po Next) do tworznia tablicy
 return pModel?pModel->TerrainCount():0;
};
TSubModel* __fastcall TAnimModel::TerrainSquare(int n)
{//pobieranie wskaŸników do pierwszego submodelu
 return pModel?pModel->TerrainSquare(n):0;
};
//---------------------------------------------------------------------------
#pragma package(smart_init)

