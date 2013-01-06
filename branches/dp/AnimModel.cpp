//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak and others

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
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------


const double fOnTime= 0.66;
const double fOffTime= fOnTime+0.66;

__fastcall TAnimAdvanced::TAnimAdvanced()
{
};
__fastcall TAnimAdvanced::~TAnimAdvanced()
{
 //delete[] pVocaloidMotionData; //plik zosta� zmodyfikowany
};

int __fastcall TAnimAdvanced::SortByBone()
{//sortowanie pliku animacji w celu optymalniejszego wykonania
 //rekordy zostaj� u�o�one wg kolejnych ramek dla ka�dej ko�ci
 //u�o�enie ko�ci alfabetycznie nie jest niezb�dne, ale upraszcza sortowanie b�belkowe
 TAnimVocaloidFrame buf; //bufor roboczy (przyda�o by si� pascalowe Swap()
 int i,j,k,swaps=0,last=iMovements-1,e;
 for (i=0;i<iMovements;++i)
  for (j=0;j<15;++j)
   if (pMovementData[i].cBone[j]=='\0') //je�li znacznik ko�ca
    for (++j;j<15;++j)
     pMovementData[i].cBone[j]='\0'; //zerowanie bajt�w za znacznikiem ko�ca
 for (i=0;i<last;++i) //do przedostatniego
 {//dop�ki nie posortowane
  j=i; //z kt�rym por�wnywa�
  k=i; //z kt�rym zamieni� (nie trzeba zamienia�, je�li ==i)
  while (++j<iMovements)
  {e=strcmp(pMovementData[k].cBone,pMovementData[j].cBone); //numery trzeba por�wnywa� inaczej
   if (e>0)
    k=j; //trzeba zamieni� - ten pod j jest mniejszy
   else if (!e)
    if (pMovementData[k].iFrame>pMovementData[j].iFrame)
     k=j; //numer klatki pod j jest mniejszy
  }
  if (k>i)
  {//je�li trzeba przestawi�
   //buf=pMovementData[i];
   //pMovementData[i]=pMovementData[k];
   //pMovementData[k]=buf;
   memcpy(&buf,pMovementData+i,sizeof(TAnimVocaloidFrame));
   memcpy(pMovementData+i,pMovementData+k,sizeof(TAnimVocaloidFrame));
   memcpy(pMovementData+k,&buf,sizeof(TAnimVocaloidFrame));
   ++swaps;
  }
 }
 return swaps;
};

__fastcall TAnimContainer::TAnimContainer()
{
 pNext=NULL;
 vRotateAngles=vector3(0.0f,0.0f,0.0f); //aktualne k�ty obrotu
 vDesiredAngles=vector3(0.0f,0.0f,0.0f); //docelowe k�ty obrotu
 fRotateSpeed=0.0;
 vTranslation=vector3(0.0f,0.0f,0.0f); //aktualne przesuni�cie
 vTranslateTo=vector3(0.0f,0.0f,0.0f); //docelowe przesuni�cie
 fTranslateSpeed=0.0;
 fAngleSpeed=0.0;
 pSubModel=NULL;
 iAnim=0; //po�o�enie pocz�tkowe
 pMovementData=NULL; //nie ma zaawansowanej animacji
 mAnim=NULL; //nie ma macierzy obrotu dla submodelu
}

__fastcall TAnimContainer::~TAnimContainer()
{
 SafeDelete(pNext);
 delete mAnim; //AnimContainer jest w�a�cicielem takich macierzy
}

bool __fastcall TAnimContainer::Init(TSubModel *pNewSubModel)
{
 fRotateSpeed=0.0f;
 pSubModel=pNewSubModel;
 return (pSubModel!=NULL);
}

void __fastcall TAnimContainer::SetRotateAnim(vector3 vNewRotateAngles, double fNewRotateSpeed)
{
 if (!this) return; //wywo�ywane z eventu, gdy brak modelu
 vDesiredAngles=vNewRotateAngles;
 fRotateSpeed=fNewRotateSpeed;
 iAnim|=1;
}

void __fastcall TAnimContainer::SetTranslateAnim(vector3 vNewTranslate, double fNewSpeed)
{
 if (!this) return; //wywo�ywane z eventu, gdy brak modelu
 vTranslateTo=vNewTranslate;
 fTranslateSpeed=fNewSpeed;
 iAnim|=2;
}

void __fastcall TAnimContainer::AnimSetVMD(double fNewSpeed)
{
 if (!this) return; //wywo�ywane z eventu, gdy brak modelu
 //skala do ustalenia, "cal" japo�ski (sun) to nieco ponad 3cm
 //X-w lewo, Y-w g�r�, Z-do ty�u
 //minimalna wysoko�� to -7.66, a nadal musi by� ponad pod�og�
 //if (pMovementData->iFrame>0) return; //tylko pierwsza ramka
 vTranslateTo=vector3(0.1*pMovementData->f3Vector.x,0.1*pMovementData->f3Vector.z,0.1*pMovementData->f3Vector.y);
 if (LengthSquared3(vTranslateTo)>0.0?true:LengthSquared3(vTranslation)>0.0)
  {//je�li ma by� przesuni�te albo jest przesuni�cie
   iAnim|=2; //wy��czy si� samo
   if (fNewSpeed>0.0)
    fTranslateSpeed=fNewSpeed; //pr�dko�� jest mno�nikiem, nie podlega skalowaniu
   else //za p�no na animacje, trzeba przestawi�
    vTranslation=vTranslateTo;
  }
 if ((qCurrent.w<1.0)||(pMovementData->qAngle.w<1.0))
 {//je�li jest jaki� obr�t
  if (!mAnim)
  {mAnim=new float4x4(); //b�dzie potrzebna macierz animacji
   mAnim->Identity(); //jedynkowanie na pocz�tek
  }
  iAnim|=4; //animacja kwaternionowa
  qStart=qCurrent; //potrzebna pocz�tkowa do interpolacji
  //---+ - te� niby dobrze, ale nie tak tr�ca w�osy na pocz�tku (macha w d�)
  //-+-+ - d�o� ma w g�rze zamiast na pasie w pozycji pocz�tkowej
  //+--+ - g�owa do ty�u (broda w g�r�) w pozycji pocz�tkowej
  //--++ - pozycja pocz�tkowa dobra, tr�ca u g�ry, ale z r�kami jako� nie tak
  //++++ - k�adzie si� brzuchem do g�ry
  //-+++ - r�ce w g�rze na pocz�tku, zamiast w d�, �okie� jakby w przeciwn� stron�
  //+-++ - nie podnosi r�ko do g�owy
  //++-+ - d�o� ma w g�rze zamiast na pasie
  qDesired=float4(-pMovementData->qAngle.x,-pMovementData->qAngle.z,+pMovementData->qAngle.y,pMovementData->qAngle.w); //tu trzeba b�dzie osie zamieni�
  if (fNewSpeed>0.0)
  {fAngleSpeed=fNewSpeed; //wtedy animowa� za pomoc� interpolacji
   fAngleCurrent=0.0; //pocz�tek interpolacji
  }
  else
  {//za p�no na animacj�, mo�na tylko przestawi� w docelowe miejsce
   fAngleSpeed=0.0;
   fAngleCurrent=1.0; //interpolacja zako�czona
   qCurrent=qDesired;
  }
 }
 //if (!strcmp(pSubModel->pName,"?Z?�?^?[")) //jak g��wna ko��
 // WriteLog(AnsiString(pMovementData->iFrame)+": "+AnsiString(pMovementData->f3Vector.x)+" "+AnsiString(pMovementData->f3Vector.y)+" "+AnsiString(pMovementData->f3Vector.z));
}

void __fastcall TAnimContainer::UpdateModel()
{
 if (pSubModel)
 {
  if (fTranslateSpeed!=0.0)
  {
   vector3 dif=vTranslateTo-vTranslation; //wektor w kierunku docelowym
   double l=LengthSquared3(dif); //d�ugo�� wektora potrzebnego przemieszczenia
   if (l>=0.0001)
   {//je�li do przemieszczenia jest ponad 1cm
    vector3 s=SafeNormalize(dif); //jednostkowy wektor kierunku
    s=s*(fTranslateSpeed*Timer::GetDeltaTime()); //przemieszczenie w podanym czasie z dan� pr�dko�ci�
    if (LengthSquared3(s)<l) //�eby nie jecha�o na drug� stron�
     vTranslation+=s;
    else
     vTranslation=vTranslateTo; //koniec animacji
   }
   else
   {//koniec animowania
    vTranslation=vTranslateTo;
    fTranslateSpeed=0.0; //wy��czenie przeliczania wektora
    if (LengthSquared3(vTranslation)<=0.0001) //je�li jest w punkcie pocz�tkowym
     iAnim&=~2; //wy��czy� zmian� pozycji submodelu
   }
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
      iAnim&=~1; //k�ty s� zerowe
   if (!anim) fRotateSpeed=0.0; //nie potrzeba przelicza� ju�
  }
  if (fAngleSpeed!=0.0)
  {//obr�t kwaternionu (interpolacja)
  }
  if (iAnim&1) //zmieniona pozycja wzgl�dem pocz�tkowej
   pSubModel->SetRotateXYZ(vRotateAngles); //ustawia typ animacji
  if (iAnim&2) //zmieniona pozycja wzgl�dem pocz�tkowej
   pSubModel->SetTranslate(vTranslation);
  if (iAnim&4) //zmieniona pozycja wzgl�dem pocz�tkowej
  {
   if (fAngleSpeed>0.0f)
   {fAngleCurrent+=fAngleSpeed*Timer::GetDeltaTime(); //aktualny parametr interpolacji
    if (fAngleCurrent>=1.0f)
    {//interpolacja zako�czona, ustawienie na pozycj� ko�cow�
     qCurrent=qDesired;
     fAngleSpeed=0.0; //wy��czenie przeliczania wektora
    }
    else
    {//obliczanie pozycji po�redniej
     //qCurrent=Normalize(Slerp(qStart,qDesired,fAngleCurrent)); //interpolacja sferyczna k�ta
     qCurrent=Slerp(qStart,qDesired,fAngleCurrent); //interpolacja sferyczna k�ta
    }
   }
   mAnim->Quaternion(&qCurrent); //wype�nienie macierzy (wymaga normalizacji?)
  }
  pSubModel->mAnimMatrix=mAnim; //u�yczenie do submodelu (na czas renderowania!)
 }
 //if (!strcmp(pSubModel->pName,"?Z?�?^?[")) //jak g��wna ko��
 // WriteLog(AnsiString(pMovementData->iFrame)+": "+AnsiString(iAnim)+" "+AnsiString(vTranslation.x)+" "+AnsiString(vTranslation.y)+" "+AnsiString(vTranslation.z));
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
  lsLights[i]=ls_Off; //a je�li s�, to wy��czone
 }
 vAngle.x=vAngle.y=vAngle.z=0.0; //zerowanie obrot�w egzemplarza
 pAdvanced=NULL; //nie ma zaawansowanej animacji
}

__fastcall TAnimModel::~TAnimModel()
{
 delete pAdvanced; //nie ma zaawansowanej animacji
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
 if (asReplacableTexture.SubString(1,1)=="*") //od gwiazdki zaczynaj� si� teksty na wy�wietlaczach
  asText=asReplacableTexture.SubString(2,asReplacableTexture.Length()-1); //zapami�tanie tekstu
 else if (asReplacableTexture!="none")
  ReplacableSkinId[1]=TTexturesManager::GetTextureID(asReplacableTexture.c_str());
 if (TTexturesManager::GetAlpha(ReplacableSkinId[1]))
  iTexAlpha=0x31310031; //tekstura z kana�em alfa - nie renderowa� w cyklu nieprzezroczystych
 else
  iTexAlpha=0x30300030; //tekstura nieprzezroczysta - nie renderowa� w cyklu przezroczystych
 return (Init(TModelsManager::GetModel(asName.c_str())));
}

bool __fastcall TAnimModel::Load(cParser *parser, bool ter)
{//rozpoznanie wpisu modelu i ustawienie �wiate�
 AnsiString str;
 std::string token;
 parser->getTokens(); //nazwa modelu
 *parser >> token;
 str=AnsiString(token.c_str());
 parser->getTokens(1,false); //tekstura (zmienia na ma�e)
 *parser >> token;
 if (!Init(str,AnsiString(token.c_str())))
 {if (str!="notload")
  {//gdy brak modelu
   if (ter) //je�li teren
   {if (str.SubString(str.Length()-3,4)==".t3d")
     str[str.Length()-2]='e';
    Global::asTerrainModel=str;
    WriteLog(AnsiString("Terrain model \""+str+"\" will be created."));
   }
   else
    ErrorLog(AnsiString("Missed file: "+str));
  }
 }
 else
 {//wi�zanie �wiate�, o ile model wczytany
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
  if (LightsOn[i]||LightsOff[i]) //Ra: zlikwidowa�em wym�g istnienia obu
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
   ti=str.ToInt(); //stan �wiat�a jest liczb�
   if (i<iMaxNumLights)
    lsLights[i]=(TLightState)ti;
   i++;
   parser->getTokens();
   *parser >> token;
   str=AnsiString(token.c_str());
  } while ((str!="endmodel")&&(str!="endterrain")); //tymczasowo oba do odwo�ania
 }
 return true;
}

TAnimContainer* __fastcall TAnimModel::AddContainer(char *pName)
{//dodanie sterowania submodelem dla egzemplarza
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
{//szukanie/dodanie sterowania submodelem dla egzemplarza
 if (!pName) return pRoot; //pobranie pierwszego (dla obrotnicy)
 TAnimContainer *pCurrent;
 for (pCurrent=pRoot;pCurrent!=NULL;pCurrent=pCurrent->pNext)
  if (pCurrent->GetName()==pName)
   return pCurrent;
 return AddContainer(pName);
}

void __fastcall TAnimModel::RaPrepare()
{//ustawia �wiat�a i animacje w modelu przed renderowaniem
 fBlinkTimer-=Timer::GetDeltaTime();
 if (fBlinkTimer<=0) fBlinkTimer=fOffTime;
 bool state; //stan �wiat�a
 for (int i=0;i<iNumLights;i++)
 {switch (lsLights[i])
  {case ls_Blink: //migotanie
    state=fBlinkTimer<fOnTime; break;
   case ls_Dark: //zapalone, gdy ciemno
    state=Global::fLuminance<=0.25; break;
   default: //zapalony albo zgaszony
    state=(lsLights[i]==ls_On);
  }
  if (LightsOn[i])  LightsOn[i]->iVisible=state;
  if (LightsOff[i]) LightsOff[i]->iVisible=!state;
 }
 TSubModel::iInstance=(int)this; //�eby nie robi� cudzych animacji
 TSubModel::pasText=&asText; //przekazanie tekstu do wy�wietlacza (!!!! do przemy�lenia)
 if (pAdvanced) //je�li jest zaawansowana animacja
  Advanced(); //wykona� co tam trzeba
 TAnimContainer *pCurrent;
 for (pCurrent=pRoot;pCurrent!=NULL;pCurrent=pCurrent->pNext)
  pCurrent->UpdateModel(); //przeliczenie animacji ka�dego submodelu
}

void __fastcall TAnimModel::RenderVBO(vector3 pPosition,double fAngle)
{//sprawdza �wiat�a i rekurencyjnie renderuje TModel3d
 RaPrepare();
 if (pModel) //renderowanie rekurencyjne submodeli
  pModel->RaRender(pPosition,fAngle,ReplacableSkinId,iTexAlpha);
}

void __fastcall TAnimModel::RenderAlphaVBO(vector3 pPosition,double fAngle)
{
 RaPrepare();
 if (pModel) //renderowanie rekurencyjne submodeli
  pModel->RaRenderAlpha(pPosition,fAngle,ReplacableSkinId,iTexAlpha);
};

void __fastcall TAnimModel::RenderDL(vector3 pPosition,double fAngle)
{
 RaPrepare();
 if (pModel) //renderowanie rekurencyjne submodeli
  pModel->Render(pPosition,fAngle,ReplacableSkinId,iTexAlpha);
}

void __fastcall TAnimModel::RenderAlphaDL(vector3 pPosition,double fAngle)
{
 RaPrepare();
 if (pModel)
  pModel->RenderAlpha(pPosition,fAngle,ReplacableSkinId,iTexAlpha);
};

int __fastcall TAnimModel::Flags()
{//informacja dla TGround, czy ma by� w Render, RenderAlpha, czy RenderMixed
 int i=pModel?pModel->Flags():0; //pobranie flag ca�ego modelu
 if (ReplacableSkinId[1]>0) //je�li ma wymienn� tekstur� 0
  i|=(i&0x01010001)*((iTexAlpha&1)?0x20:0x10);
 //if (ReplacableSkinId[2]>0) //je�li ma wymienn� tekstur� 1
 // i|=(i&0x02020002)*((iTexAlpha&1)?0x10:0x08);
 //if (ReplacableSkinId[3]>0) //je�li ma wymienn� tekstur� 2
 // i|=(i&0x04040004)*((iTexAlpha&1)?0x08:0x04);
 //if (ReplacableSkinId[4]>0) //je�li ma wymienn� tekstur� 3
 // i|=(i&0x08080008)*((iTexAlpha&1)?0x04:0x02);
 return i;
};

//-----------------------------------------------------------------------------
//2011-03-16 cztery nowe funkcje renderowania z mo�liwo�ci� pochylania obiekt�w
//-----------------------------------------------------------------------------

void __fastcall TAnimModel::RenderDL(vector3* vPosition)
{
 RaPrepare();
 if (pModel) //renderowanie rekurencyjne submodeli
  pModel->Render(vPosition,&vAngle,ReplacableSkinId,iTexAlpha);
};
void __fastcall TAnimModel::RenderAlphaDL(vector3* vPosition)
{
 RaPrepare();
 if (pModel) //renderowanie rekurencyjne submodeli
  pModel->RenderAlpha(vPosition,&vAngle,ReplacableSkinId,iTexAlpha);
};
void __fastcall TAnimModel::RenderVBO(vector3* vPosition)
{
 RaPrepare();
 if (pModel) //renderowanie rekurencyjne submodeli
  pModel->RaRender(vPosition,&vAngle,ReplacableSkinId,iTexAlpha);
};
void __fastcall TAnimModel::RenderAlphaVBO(vector3* vPosition)
{
 RaPrepare();
 if (pModel) //renderowanie rekurencyjne submodeli
  pModel->RaRenderAlpha(vPosition,&vAngle,ReplacableSkinId,iTexAlpha);
};

//---------------------------------------------------------------------------
bool __fastcall TAnimModel::TerrainLoaded()
{//zliczanie kwadrat�w kilometrowych (g��wna linia po Next) do tworznia tablicy
 return (this?pModel!=NULL:false);
};
int __fastcall TAnimModel::TerrainCount()
{//zliczanie kwadrat�w kilometrowych (g��wna linia po Next) do tworznia tablicy
 return pModel?pModel->TerrainCount():0;
};
TSubModel* __fastcall TAnimModel::TerrainSquare(int n)
{//pobieranie wska�nik�w do pierwszego submodelu
 return pModel?pModel->TerrainSquare(n):0;
};
void __fastcall TAnimModel::TerrainRenderVBO(int n)
{//renderowanie terenu z VBO
 if (pModel) pModel->TerrainRenderVBO(n);
};
//---------------------------------------------------------------------------

void __fastcall TAnimModel::Advanced()
{//wykonanie zaawansowanych animacji na submodelach
 pAdvanced->fCurrent+=pAdvanced->fFrequency*Timer::GetDeltaTime(); //aktualna ramka zmiennoprzecinkowo
 int frame=floor(pAdvanced->fCurrent); //numer klatki jako int
 TAnimContainer *pCurrent;
 if (pAdvanced->fCurrent>=pAdvanced->fLast)
 {//animacja zosta�a zako�czona
  delete pAdvanced;
  pAdvanced=NULL; //dalej ju� nic
  for (pCurrent=pRoot;pCurrent!=NULL;pCurrent=pCurrent->pNext)
   if (pCurrent->pMovementData) //je�li obs�ugiwany tabelk� animacji
    pCurrent->pMovementData=NULL; //usuwanie wska�nik�w
 }
 else
 {//co� trzeba poanimowa� - wszystkie animowane submodele s� w tym �a�cuchu
  for (pCurrent=pRoot;pCurrent!=NULL;pCurrent=pCurrent->pNext)
   if (pCurrent->pMovementData) //je�li obs�ugiwany tabelk� animacji
    if (frame>=pCurrent->pMovementData->iFrame) //koniec czekania
     if (!strcmp(pCurrent->pMovementData->cBone,(pCurrent->pMovementData+1)->cBone))
     {//jak kolejna ramka dotyczy tego samego submodelu, ustawi� animacj� do kolejnej ramki
      ++pCurrent->pMovementData; //kolejna klatka
      pCurrent->AnimSetVMD(pAdvanced->fFrequency/(double(pCurrent->pMovementData->iFrame)-pAdvanced->fCurrent));
     }
     else
      pCurrent->pMovementData=NULL; //inna nazwa, animowanie zako�czone w aktualnym po�o�eniu
 }
};

void __fastcall TAnimModel::AnimationVND(void* pData, double a, double b, double c, double d)
{//rozpocz�cie wykonywania animacji z podanego pliku
 //tabela w pliku musi by� posortowana wg klatek dla kolejnych ko�ci!
 //skr�cone nagranie ma 3:42 = 222 sekundy, animacja ko�czy si� na klatce 6518
 //daje to 29.36 (~=30) klatek na sekund�
 //w opisach jest podawane 24 albo 36 jako standard - powiedzmy, parametr (d) to FPS animacji
 delete pAdvanced; //usuni�cie ewentualnego poprzedniego
 pAdvanced=NULL; //gdyby si� nie uda�o rozpozna� pliku
 if (AnsiString((char*)pData)=="Vocaloid Motion Data 0002")
 {
  pAdvanced=new TAnimAdvanced();
  pAdvanced->pVocaloidMotionData=(char*)pData; //podczepienie pliku danych
  pAdvanced->iMovements=*((int*)(((char*)pData)+50)); //numer ostatniej klatki
  pAdvanced->pMovementData=(TAnimVocaloidFrame*)(((char*)pData)+54); //rekordy animacji
  //WriteLog(sizeof(TAnimVocaloidFrame));
  pAdvanced->fFrequency=d;
  pAdvanced->fCurrent=0.0; //aktualna ramka
  pAdvanced->fLast=0.0; //ostatnia ramka
/*
  pAdvanced->SortByBone();
    TFileStream *fs=new TFileStream("models\\1.vmd",fmCreate);
    fs->Write(pData,2198342); //2948728);
    delete fs;
*/

  int i,j,k=0,idx=0;
  AnsiString name;
  TAnimContainer *pSub;
  for (i=0;i<pAdvanced->iMovements;++i)
  {if (strcmp(pAdvanced->pMovementData[i].cBone,name.c_str()))
   {//je�li pozycja w tabelce nie by�a wyszukiwana w submodelach
    pSub=GetContainer(pAdvanced->pMovementData[i].cBone); //szukanie
    if (pSub) //znaleziony
    {pSub->pMovementData=pAdvanced->pMovementData+i; //got�w do animowania
     pSub->AnimSetVMD(0.0); //usuawienie pozycji pocz�tkowej (powinna by� zerowa, inaczej b�dzie skok)
    }
    name=AnsiString(pAdvanced->pMovementData[i].cBone); //nowa nazwa do pomijania
   }
   if (pAdvanced->fLast<pAdvanced->pMovementData[i].iFrame)
    pAdvanced->fLast=pAdvanced->pMovementData[i].iFrame;
  }
/*
  for (i=0;i<pAdvanced->iMovements;++i)
  if (AnsiString(pAdvanced->pMovementData[i+1].cBone)!=AnsiString(pAdvanced->pMovementData[i].cBone))
  {//generowane dla ostatniej klatki danej ko�ci
   name="";
   for (j=0;j<15;j++)
    name+=IntToHex((unsigned char)pAdvanced->pMovementData[i].cBone[j],2);
   WriteLog(name+","
    +AnsiString(pAdvanced->pMovementData[i].cBone)+","
    +AnsiString(idx)+"," //indeks
    +AnsiString(i+1-idx)+"," //ile pozycji animacji
    +AnsiString(k)+"," //pierwsza klatka
    +AnsiString(pAdvanced->pMovementData[i].iFrame)+"," //ostatnia klatka
    +AnsiString(pAdvanced->pMovementData[i].f3Vector.x)+","
    +AnsiString(pAdvanced->pMovementData[i].f3Vector.y)+","
    +AnsiString(pAdvanced->pMovementData[i].f3Vector.z)+","
    +AnsiString(pAdvanced->pMovementData[i].fAngle[0])+","
    +AnsiString(pAdvanced->pMovementData[i].fAngle[1])+","
    +AnsiString(pAdvanced->pMovementData[i].fAngle[2])+","
    +AnsiString(pAdvanced->pMovementData[i].fAngle[3])

    );
   idx=i+1;
   k=pAdvanced->pMovementData[i+1].iFrame; //pierwsza klatka nast�pnego
  }
  else
   if (pAdvanced->pMovementData[i].iFrame>0)
    if ((k>pAdvanced->pMovementData[i].iFrame)||(k==0))
     k=pAdvanced->pMovementData[i].iFrame; //pierwsza niezerowa ramka
*/
/*
  for (i=0;i<pAdvanced->iMovements;++i)
   if (AnsiString(pAdvanced->pMovementData[i].cBone)=="\x89\x45\x90\x65\x8E\x77\x82\x4F")
   {name="";
    for (j=0;j<15;j++)
     name+=IntToHex((unsigned char)pAdvanced->pMovementData[i].cBone[j],2);
    WriteLog(name+","
     +AnsiString(i)+"," //pozycja w tabeli
     +AnsiString(pAdvanced->pMovementData[i].iFrame)+"," //pierwsza klatka
    );
   }
*/
 }
};

//---------------------------------------------------------------------------

