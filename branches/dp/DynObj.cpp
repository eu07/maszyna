//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

*/


#include "system.hpp"
#include "classes.hpp"
#pragma hdrstop

#include "DynObj.h"

#include "Timer.h"
#include "Usefull.h"
//McZapkie-260202
#include "Globals.h"
#include "Texture.h"
#include "AirCoupler.h"

#include "TractionPower.h"
#include "Ground.h" //bo AddToQuery jest
#include "Event.h"
#include "Driver.h"
#include "Camera.h" //bo likwidujemy trz�sienie
#include "Console.h"
#include "Traction.h"
#pragma package(smart_init)

//Ra: taki zapis funkcjonuje lepiej, ale mo�e nie jest optymalny
#define vWorldFront vector3(0,0,1)
#define vWorldUp vector3(0,1,0)
#define vWorldLeft CrossProduct(vWorldUp,vWorldFront)

//Ra: bo te poni�ej to si� powiela�y w ka�dym module odobno
//vector3 vWorldFront=vector3(0,0,1);
//vector3 vWorldUp=vector3(0,1,0);
//vector3 vWorldLeft=CrossProduct(vWorldUp,vWorldFront);

const float maxrot=(M_PI/3.0); //60�

//---------------------------------------------------------------------------
int __fastcall TAnim::TypeSet(int i)
{//ustawienie typu animacji i zale�nej od niego ilo�ci animowanych submodeli
 fMaxDist=-1.0; //normalnie nie pokazywa�
 switch (i)
 {//maska 0x00F: ile u�ywa wska�nik�w na submodele (0 gdy jeden, wtedy bez tablicy)
  //maska 0x0F0: 0-osie,1-drzwi,2-obracane,3-zderzaki,4-w�zki,5-pantografy,6-t�oki
  //maska 0xF00: ile u�ywa liczb float dla wsp�czynnik�w i stanu
  case 0: iFlags=0x000; break; //0-o�
  case 1: iFlags=0x010; break; //1-drzwi
  case 2: iFlags=0x020; break; //2-wahacz, d�wignia itp.
  case 3: iFlags=0x030; break; //3-zderzak
  case 4: iFlags=0x040; break; //4-w�zek
  case 5: //5-pantograf - 5 submodeli
   iFlags=0x055;
   fParamPants=new TAnimPant();
   fParamPants->vPos=vector3(0,0,0); //przypisanie domy�nych wsp�czynnik�w do pantograf�w
   fParamPants->fLenL1=1.176289; //1.22;
   fParamPants->fLenU1=1.724482197; //1.755;
   fParamPants->fHoriz=0.54555075; //przesuni�cie �lizgu w d�ugo�ci pojazdu wzgl�dem osi obrotu dolnego ramienia
   fParamPants->fHeight=0.07; //wysoko�� �lizgu ponad o� obrotu
   fParamPants->fWidth=0.635; //po�owa szeroko�ci �lizgu, 0.635 dla AKP-1 i AKP-4E
   fParamPants->fAngleL0=DegToRad(2.8547285515689267247882521833308);
   fParamPants->fAngleL=fParamPants->fAngleL0; //pocz�tkowy k�t dolnego ramienia
   //fParamPants->pantu=acos((1.22*cos(fParamPants->fAngleL)+0.535)/1.755); //g�rne rami�
   fParamPants->fAngleU0=acos((fParamPants->fLenL1*cos(fParamPants->fAngleL)+fParamPants->fHoriz)/fParamPants->fLenU1); //g�rne rami�
   fParamPants->fAngleU=fParamPants->fAngleU0; //pocz�tkowy k�t
   //fParamPants->PantWys=1.22*sin(fParamPants->fAngleL)+1.755*sin(fParamPants->fAngleU); //wysoko�� pocz�tkowa
   //fParamPants->PantWys=1.176289*sin(fParamPants->fAngleL)+1.724482197*sin(fParamPants->fAngleU); //wysoko�� pocz�tkowa
   fParamPants->PantWys=fParamPants->fLenL1*sin(fParamPants->fAngleL)+fParamPants->fLenU1*sin(fParamPants->fAngleU)+fParamPants->fHeight; //wysoko�� pocz�tkowa
   fParamPants->PantTraction=fParamPants->PantWys;
   fParamPants->hvPowerWire=NULL;
  break;
  case 6: iFlags=0x068; break; //6-t�ok i rozrz�d - 8 submodeli
  default: iFlags=0;
 }
 yUpdate=NULL;
 return iFlags&15; //ile wska�nik�w rezerwowa� dla danego typu animacji
};
__fastcall TAnim::TAnim()
{//potrzebne to w og�le?
};
__fastcall TAnim::~TAnim()
{//usuwanie animacji
 switch (iFlags&0xF0)
 {//usuwanie struktur, zale�nie ile zosta�o stworzonych
  case 0x50: //5-pantograf
   delete fParamPants;
  break;
  case 0x60: //6-t�ok i rozrz�d
  break;
 }
};
void __fastcall TAnim::Parovoz()
{//animowanie t�oka i rozrz�du parowozu
};
//---------------------------------------------------------------------------
TDynamicObject* __fastcall TDynamicObject::FirstFind(int &coupler_nr)
{//szukanie skrajnego po��czonego pojazdu w pociagu
 //od strony sprzegu (coupler_nr) obiektu (start)
 TDynamicObject* temp=this;
 for (int i=0;i<300;i++) //ograniczenie do 300 na wypadek zap�tlenia sk�adu
 {
  if (!temp)
   return NULL; //Ra: zabezpieczenie przed ewentaulnymi b��dami sprz�g�w
  if (temp->MoverParameters->Couplers[coupler_nr].CouplingFlag==0)
   return temp; //nic nie ma ju� dalej pod��czone
  if (coupler_nr==0)
  {//je�eli szukamy od sprz�gu 0
   if (temp->PrevConnected) //je�li mamy co� z przodu
   {
    if (temp->PrevConnectedNo==0) //je�li pojazd od strony sprz�gu 0 jest odwr�cony
     coupler_nr=1-coupler_nr; //to zmieniamy kierunek sprz�gu
    temp=temp->PrevConnected; //ten jest od strony 0
   }
   else
    return temp; //je�li jednak z przodu nic nie ma
  }
  else
  {
   if (temp->NextConnected)
   {if (temp->NextConnectedNo==1) //je�li pojazd od strony sprz�gu 1 jest odwr�cony
     coupler_nr=1-coupler_nr; //to zmieniamy kierunek sprz�gu
    temp=temp->NextConnected; //ten pojazd jest od strony 1
   }
   else
    return temp; //je�li jednak z ty�u nic nie ma
  }
 }
 return NULL; //to tylko po wyczerpaniu p�tli
};

//---------------------------------------------------------------------------
float __fastcall TDynamicObject::GetEPP()
{//szukanie skrajnego po��czonego pojazdu w pociagu
 //od strony sprzegu (coupler_nr) obiektu (start)
 TDynamicObject* temp=this;
 int coupler_nr=0;
 float eq=0,am=0;

 for (int i=0;i<300;i++) //ograniczenie do 300 na wypadek zap�tlenia sk�adu
 {
  if (!temp)
   break; //Ra: zabezpieczenie przed ewentaulnymi b��dami sprz�g�w
  eq+=temp->MoverParameters->PipePress*temp->MoverParameters->Dim.L;
  am+=temp->MoverParameters->Dim.L;
  if ((temp->MoverParameters->Couplers[coupler_nr].CouplingFlag&2)!=2)
   break; //nic nie ma ju� dalej pod��czone
  if (coupler_nr==0)
  {//je�eli szukamy od sprz�gu 0
   if (temp->PrevConnected) //je�li mamy co� z przodu
   {
    if (temp->PrevConnectedNo==0) //je�li pojazd od strony sprz�gu 0 jest odwr�cony
     coupler_nr=1-coupler_nr; //to zmieniamy kierunek sprz�gu
    temp=temp->PrevConnected; //ten jest od strony 0
   }
   else
    break; //je�li jednak z przodu nic nie ma
  }
  else
  {
   if (temp->NextConnected)
   {if (temp->NextConnectedNo==1) //je�li pojazd od strony sprz�gu 1 jest odwr�cony
     coupler_nr=1-coupler_nr; //to zmieniamy kierunek sprz�gu
    temp=temp->NextConnected; //ten pojazd jest od strony 1
   }
   else
    break; //je�li jednak z ty�u nic nie ma
  }
 }

 temp=this;
 coupler_nr=1;
 for (int i=0;i<300;i++) //ograniczenie do 300 na wypadek zap�tlenia sk�adu
 {
  if (!temp)
   break; //Ra: zabezpieczenie przed ewentaulnymi b��dami sprz�g�w
  eq+=temp->MoverParameters->PipePress*temp->MoverParameters->Dim.L;
  am+=temp->MoverParameters->Dim.L;
  if ((temp->MoverParameters->Couplers[coupler_nr].CouplingFlag&2)!=2)
   break; //nic nie ma ju� dalej pod��czone
  if (coupler_nr==0)
  {//je�eli szukamy od sprz�gu 0
   if (temp->PrevConnected) //je�li mamy co� z przodu
   {
    if (temp->PrevConnectedNo==0) //je�li pojazd od strony sprz�gu 0 jest odwr�cony
     coupler_nr=1-coupler_nr; //to zmieniamy kierunek sprz�gu
    temp=temp->PrevConnected; //ten jest od strony 0
   }
   else
    break; //je�li jednak z przodu nic nie ma
  }
  else
  {
   if (temp->NextConnected)
   {if (temp->NextConnectedNo==1) //je�li pojazd od strony sprz�gu 1 jest odwr�cony
     coupler_nr=1-coupler_nr; //to zmieniamy kierunek sprz�gu
    temp=temp->NextConnected; //ten pojazd jest od strony 1
   }
   else
    break; //je�li jednak z ty�u nic nie ma
  }
 }
 eq-=MoverParameters->PipePress*MoverParameters->Dim.L;
 am-=MoverParameters->Dim.L;
 return eq/am;
};


//---------------------------------------------------------------------------
TDynamicObject* __fastcall TDynamicObject::GetFirstDynamic(int cpl_type)
{//Szukanie skrajnego po��czonego pojazdu w pociagu
 //od strony sprzegu (cpl_type) obiektu szukajacego
 //Ra: wystarczy jedna funkcja do szukania w obu kierunkach
 return FirstFind(cpl_type); //u�ywa referencji
};

/*
TDynamicObject* __fastcall TDynamicObject::GetFirstCabDynamic(int cpl_type)
{//ZiomalCl: szukanie skrajnego obiektu z kabin�
 TDynamicObject* temp=this;
 int coupler_nr=cpl_type;
 for (int i=0;i<300;i++) //ograniczenie do 300 na wypadek zap�tlenia sk�adu
 {
  if (!temp)
   return NULL; //Ra: zabezpieczenie przed ewentaulnymi b��dami sprz�g�w
  if (temp->MoverParameters->CabNo!=0&&temp->MoverParameters->SandCapacity!=0)
    return temp; //nic nie ma ju� dalej pod��czone
  if (temp->MoverParameters->Couplers[coupler_nr].CouplingFlag==0)
   return NULL;
  if (coupler_nr==0)
  {//je�eli szukamy od sprz�gu 0
   if (temp->PrevConnectedNo==0) //je�li pojazd od strony sprz�gu 0 jest odwr�cony
    coupler_nr=1-coupler_nr; //to zmieniamy kierunek sprz�gu
   if (temp->PrevConnected)
    temp=temp->PrevConnected; //ten jest od strony 0
  }
  else
  {
   if (temp->NextConnectedNo==1) //je�li pojazd od strony sprz�gu 1 jest odwr�cony
    coupler_nr=1-coupler_nr; //to zmieniamy kierunek sprz�gu
   if (temp->NextConnected)
    temp=temp->NextConnected; //ten pojazd jest od strony 1
  }
 }
 return NULL; //to tylko po wyczerpaniu p�tli
};
*/

void TDynamicObject::ABuSetModelShake(vector3 mShake)
{
 modelShake=mShake;
};

int __fastcall TDynamicObject::GetPneumatic(bool front, bool red)
{
 int x,y,z; //1=prosty, 2=sko�ny
 if (red)
 {
  if (front)
  {
   x=btCPneumatic1.GetStatus();
   y=btCPneumatic1r.GetStatus();
  }
  else
  {
   x=btCPneumatic2.GetStatus();
   y=btCPneumatic2r.GetStatus();
  }
 }
 else
  if (front)
  {
   x=btPneumatic1.GetStatus();
   y=btPneumatic1r.GetStatus();
  }
  else
  {
   x=btPneumatic2.GetStatus();
   y=btPneumatic2r.GetStatus();
  }
 z=0; //brak w�y?
 if ((x==1)&&(y==1)) z=3; //dwa proste
 if ((x==2)&&(y==0)) z=1; //lewy sko�ny, brak prawego
 if ((x==0)&&(y==2)) z=2; //brak lewego, prawy sko�ny

 return z;
}

void __fastcall TDynamicObject::SetPneumatic(bool front,bool red)
{
 int x=0,ten,tamten;
 ten=GetPneumatic(front,red); //1=lewy skos,2=prawy skos,3=dwa proste
 if (front)
  if (PrevConnected) //pojazd od strony sprz�gu 0
   tamten=PrevConnected->GetPneumatic((PrevConnectedNo==0?true:false),red);
 if (!front)
  if (NextConnected) //pojazd od strony sprz�gu 1
   tamten=NextConnected->GetPneumatic((NextConnectedNo==0?true:false),red);
 if (ten==tamten) //je�li uk�ad jest symetryczny
  switch (ten)
   {
    case 1: x=2; break; //mamy lewy skos, da� lewe skosy
    case 2: x=3; break; //mamy prawy skos, da� prawe skosy
    case 3: //wszystkie cztery na prosto
     if (MoverParameters->Couplers[front?0:1].Render) x=1; else x=4;
     break;
   }
 else
 {
  if (ten==2) x=4;
  if (ten==1) x=1;
  if (ten==3) if (tamten==1) x=4; else x=1;
 }
 if (front)
 {if (red) cp1=x; else sp1=x;} //kt�ry pokazywa� z przodu
 else
 {if (red) cp2=x; else sp2=x;} //kt�ry pokazywa� z ty�u
}

void TDynamicObject::UpdateAxle(TAnim *pAnim)
{//animacja osi
 pAnim->smAnimated->SetRotate(float3(1,0,0),*pAnim->dWheelAngle);
};

void TDynamicObject::UpdateBoogie(TAnim *pAnim)
{//animacja w�zka
 pAnim->smAnimated->SetRotate(float3(1,0,0),*pAnim->dWheelAngle);
};

void TDynamicObject::UpdateDoorTranslate(TAnim *pAnim)
{//animacja drzwi - przesuw
 //WriteLog("Dla drzwi nr:", i);
 //WriteLog("Wspolczynnik", DoorSpeedFactor[i]);
 //Ra: te wsp�czynniki s� bez sensu, bo modyfikuj� wektor przesuni�cia
 //w efekcie drzwi otwierane na zewn�trz b�d� odlatywac dowolnie daleko :)
 //ograniczy�em zakres ruchu funkcj� max
 if (pAnim->smAnimated)
 {
  if (pAnim->iNumber&1)
   pAnim->smAnimated->SetTranslate(vector3(0,0,Max0R(dDoorMoveR*pAnim->fSpeed,dDoorMoveR)));
  else
   pAnim->smAnimated->SetTranslate(vector3(0,0,Max0R(dDoorMoveL*pAnim->fSpeed,dDoorMoveL)));
 }
};

void TDynamicObject::UpdateDoorRotate(TAnim *pAnim)
{//animacja drzwi - obr�t
 if (pAnim->smAnimated)
 {//if (MoverParameters->DoorOpenMethod==2) //obrotowe albo dw�j�omne (trzeba kombinowac submodelami i ShiftL=90,R=180)
  if (pAnim->iNumber&1)
   pAnim->smAnimated->SetRotate(float3(1,0,0),dDoorMoveR);
  else
   pAnim->smAnimated->SetRotate(float3(1,0,0),dDoorMoveL);
 }
};

void TDynamicObject::UpdatePant(TAnim *pAnim)
{//animacja pantografu - 4 obracane ramiona, �lizg pi�ty
 float a,b,c;
 a=RadToDeg(pAnim->fParamPants->fAngleL-pAnim->fParamPants->fAngleL0);
 b=RadToDeg(pAnim->fParamPants->fAngleU-pAnim->fParamPants->fAngleU0);
 c=a+b;
 if (pAnim->smElement[0]) pAnim->smElement[0]->SetRotate(float3(-1,0,0),a); //dolne rami�
 if (pAnim->smElement[1]) pAnim->smElement[1]->SetRotate(float3(1,0,0),a);
 if (pAnim->smElement[2]) pAnim->smElement[2]->SetRotate(float3(1,0,0),c); //g�rne rami�
 if (pAnim->smElement[3]) pAnim->smElement[3]->SetRotate(float3(-1,0,0),c);
 if (pAnim->smElement[4]) pAnim->smElement[4]->SetRotate(float3(-1,0,0),b); //�lizg
};

//ABu 29.01.05 przeklejone z render i renderalpha: *********************
void __inline TDynamicObject::ABuLittleUpdate(double ObjSqrDist)
{//ABu290105: pozbierane i uporzadkowane powtarzajace sie rzeczy z Render i RenderAlpha
 //dodatkowy warunek, if (ObjSqrDist<...) zeby niepotrzebnie nie zmianiec w obiektach,
 //ktorych i tak nie widac
 //NBMX wrzesien, MC listopad: zuniwersalnione
 btnOn=false; //czy przywr�ci� stan domy�lny po renderowaniu

 if (ObjSqrDist<160000) //gdy bli�ej ni� 400m
 {
  for (int i=0;i<iAnimations;++i) //wykonanie kolejnych animacji
   if (ObjSqrDist<pAnimations[i].fMaxDist)
    if (pAnimations[i].yUpdate) //je�li zdefiniowana funkcja
     pAnimations[i].yUpdate(pAnimations+i); //aktualizacja animacji (po�o�enia submodeli
  if (ObjSqrDist<2500) //gdy bli�ej ni� 50m
  {
   //ABu290105: rzucanie pudlem
   //te animacje wymagaj� banan�w w modelach!
   mdModel->GetSMRoot()->SetTranslate(modelShake);
   if (mdKabina)
    mdKabina->GetSMRoot()->SetTranslate(modelShake);
   if (mdLoad)
    mdLoad->GetSMRoot()->SetTranslate(modelShake);
   if (mdLowPolyInt)
    mdLowPolyInt->GetSMRoot()->SetTranslate(modelShake);
   if (mdPrzedsionek)
    mdPrzedsionek->GetSMRoot()->SetTranslate(modelShake);
   //ABu: koniec rzucania
   //ABu011104: liczenie obrotow wozkow
   ABuBogies();
   //McZapkie-050402: obracanie kolami
/*
   for (int i=0; i<iAnimatedAxles; i++)
    if (smAnimatedWheel[i])
     smAnimatedWheel[i]->SetRotate(float3(1,0,0),*pWheelAngle[i]);
     //smAnimatedWheel[i]->SetRotate(float3(1,0,0),dWheelAngle[1]);
*/
   //Mczapkie-100402: rysowanie lub nie - sprzegow
   //ABu-240105: Dodatkowy warunek: if (...).Render, zeby rysowal tylko jeden
   //z polaczonych sprzegow
   if ((TestFlag(MoverParameters->Couplers[0].CouplingFlag,ctrain_coupler))
      &&(MoverParameters->Couplers[0].Render))
    {btCoupler1.TurnOn(); btnOn=true;}
   //else btCoupler1.TurnOff();
   if ((TestFlag(MoverParameters->Couplers[1].CouplingFlag,ctrain_coupler))
      &&(MoverParameters->Couplers[1].Render))
    {btCoupler2.TurnOn(); btnOn=true;}
   //else btCoupler2.TurnOff();
  //********************************************************************************
  //przewody powietrzne j.w., ABu: decyzja czy rysowac tylko na podstawie 'render' - juz nie
  //przewody powietrzne, yB: decyzja na podstawie polaczen w t3d
  if (Global::bnewAirCouplers)
  {
   SetPneumatic(false,false); //wczytywanie z t3d ulozenia wezykow
   SetPneumatic(true,false);  //i zapisywanie do zmiennej
   SetPneumatic(true,true);   //ktore z nich nalezy
   SetPneumatic(false,true);  //wyswietlic w tej klatce

   if (TestFlag(MoverParameters->Couplers[0].CouplingFlag,ctrain_pneumatic))
   {
    switch (cp1)
    {
     case 1: btCPneumatic1.TurnOn(); break;
     case 2: btCPneumatic1.TurnxOn(); break;
     case 3: btCPneumatic1r.TurnxOn(); break;
     case 4: btCPneumatic1r.TurnOn(); break;
    }
    btnOn=true;
   }
   //else
   //{
   // btCPneumatic1.TurnOff();
   // btCPneumatic1r.TurnOff();
   //}

   if (TestFlag(MoverParameters->Couplers[1].CouplingFlag,ctrain_pneumatic))
   {
    switch (cp2)
    {
     case 1: btCPneumatic2.TurnOn(); break;
     case 2: btCPneumatic2.TurnxOn(); break;
     case 3: btCPneumatic2r.TurnxOn(); break;
     case 4: btCPneumatic2r.TurnOn(); break;
    }
    btnOn=true;
   }
   //else
   //{
   // btCPneumatic2.TurnOff();
   // btCPneumatic2r.TurnOff();
   //}

   //przewody zasilajace, j.w. (yB)
   if (TestFlag(MoverParameters->Couplers[0].CouplingFlag,ctrain_scndpneumatic))
   {
    switch (sp1)
    {
     case 1: btPneumatic1.TurnOn(); break;
     case 2: btPneumatic1.TurnxOn(); break;
     case 3: btPneumatic1r.TurnxOn(); break;
     case 4: btPneumatic1r.TurnOn(); break;
    }
    btnOn=true;
   }
   //else
   //{
   // btPneumatic1.TurnOff();
   // btPneumatic1r.TurnOff();
   //}

   if (TestFlag(MoverParameters->Couplers[1].CouplingFlag,ctrain_scndpneumatic))
   {
    switch (sp2)
    {
     case 1: btPneumatic2.TurnOn(); break;
     case 2: btPneumatic2.TurnxOn(); break;
     case 3: btPneumatic2r.TurnxOn(); break;
     case 4: btPneumatic2r.TurnOn(); break;
    }
    btnOn=true;
   }
   //else
   //{
   // btPneumatic2.TurnOff();
   // btPneumatic2r.TurnOff();
   //}
  }
//*********************************************************************************/
  else //po staremu ABu'oewmu
  {
   //przewody powietrzne j.w., ABu: decyzja czy rysowac tylko na podstawie 'render'
   if (TestFlag(MoverParameters->Couplers[0].CouplingFlag,ctrain_pneumatic))
   {
      if (MoverParameters->Couplers[0].Render)
        btCPneumatic1.TurnOn();
      else
        btCPneumatic1r.TurnOn();
      btnOn=true;
   }
   //else
   //{
   // btCPneumatic1.TurnOff();
   // btCPneumatic1r.TurnOff();
   //}

   if (TestFlag(MoverParameters->Couplers[1].CouplingFlag,ctrain_pneumatic))
   {
      if (MoverParameters->Couplers[1].Render)
        btCPneumatic2.TurnOn();
      else
        btCPneumatic2r.TurnOn();
      btnOn=true;
   }
   //else
   //{
   // btCPneumatic2.TurnOff();
   // btCPneumatic2r.TurnOff();
   //}

   //przewody powietrzne j.w., ABu: decyzja czy rysowac tylko na podstawie 'render' //yB - zasilajace
   if (TestFlag(MoverParameters->Couplers[0].CouplingFlag,ctrain_scndpneumatic))
   {
      if (MoverParameters->Couplers[0].Render)
        btPneumatic1.TurnOn();
      else
        btPneumatic1r.TurnOn();
      btnOn=true;
   }
   //else
   //{
   // btPneumatic1.TurnOff();
   // btPneumatic1r.TurnOff();
   //}

   if (TestFlag(MoverParameters->Couplers[1].CouplingFlag,ctrain_scndpneumatic))
   {
      if (MoverParameters->Couplers[1].Render)
        btPneumatic2.TurnOn();
      else
        btPneumatic2r.TurnOn();
      btnOn=true;
   }
   //else
   //{
   // btPneumatic2.TurnOff();
   // btPneumatic2r.TurnOff();
   //}
  }
  //*************************************************************/// koniec wezykow
  // uginanie zderzakow
  for (int i=0; i<2; i++)
  {
   double dist=MoverParameters->Couplers[i].Dist/2.0;
   if (smBuforLewy[i])
    if (dist<0)
     smBuforLewy[i]->SetTranslate(vector3(dist,0,0));
   if (smBuforPrawy[i])
    if (dist<0)
     smBuforPrawy[i]->SetTranslate(vector3(dist,0,0));
  }
 }

   //Winger 160204 - podnoszenie pantografow

  //przewody sterowania ukrotnionego
  if (TestFlag(MoverParameters->Couplers[0].CouplingFlag,ctrain_controll))
   {btCCtrl1.TurnOn(); btnOn=true;}
  //else btCCtrl1.TurnOff();
  if (TestFlag(MoverParameters->Couplers[1].CouplingFlag,ctrain_controll))
   {btCCtrl2.TurnOn(); btnOn=true;}
  //else btCCtrl2.TurnOff();
  //McZapkie-181103: mostki przejsciowe
  if (TestFlag(MoverParameters->Couplers[0].CouplingFlag,ctrain_passenger))
   {btCPass1.TurnOn(); btnOn=true;}
  //else btCPass1.TurnOff();
  if (TestFlag(MoverParameters->Couplers[1].CouplingFlag,ctrain_passenger))
   {btCPass2.TurnOn(); btnOn=true;}
  //else btCPass2.TurnOff();
  if (MoverParameters->Battery)
  {//sygnaly konca pociagu
   if (btEndSignals1.Active())
   {
    if (TestFlag(iLights[0],2)
      ||TestFlag(iLights[0],32))
     {btEndSignals1.TurnOn(); btnOn=true;}
    //else btEndSignals1.TurnOff();
   }
   else
   {
    if (TestFlag(iLights[0],2))
     {btEndSignals11.TurnOn(); btnOn=true;}
    //else btEndSignals11.TurnOff();
    if (TestFlag(iLights[0],32))
     {btEndSignals13.TurnOn(); btnOn=true;}
    //else btEndSignals13.TurnOff();
   }
   if (btEndSignals2.Active())
   {
    if (TestFlag(iLights[1],2)
      ||TestFlag(iLights[1],32))
     {btEndSignals2.TurnOn(); btnOn=true;}
    //else btEndSignals2.TurnOff();
   }
   else
   {
    if (TestFlag(iLights[1],2))
     {btEndSignals21.TurnOn(); btnOn=true;}
    //else btEndSignals21.TurnOff();
    if (TestFlag(iLights[1],32))
     {btEndSignals23.TurnOn(); btnOn=true;}
    //else btEndSignals23.TurnOff();
   }
  }
  //tablice blaszane:
  if (TestFlag(iLights[0],64))
   {btEndSignalsTab1.TurnOn(); btnOn=true;}
  //else btEndSignalsTab1.TurnOff();
  if (TestFlag(iLights[1],64))
   {btEndSignalsTab2.TurnOn(); btnOn=true;}
  //else btEndSignalsTab2.TurnOff();
  //McZapkie-181002: krecenie wahaczem (korzysta z kata obrotu silnika)
  if (iAnimType[ANIM_LEVERS])
   for (int i=0;i<4;++i)
    if (smWahacze[i])
     smWahacze[i]->SetRotate(float3(1,0,0),fWahaczeAmp*cos(MoverParameters->eAngle));
  if (Mechanik&&(Controller!=Humandriver))
  {//rysowanie figurki mechanika
   if (smMechanik0) //mechanik od strony sprz�gu 0
    if (smMechanik1) //jak jest drugi, to tego tylko pokazujemy
     smMechanik0->iVisible=MoverParameters->ActiveCab>0;
    else
    {//jak jest tylko jeden, to do drugiej kabiny go obracamy
     smMechanik0->iVisible=(MoverParameters->ActiveCab!=0);
     smMechanik0->SetRotate(float3(0,0,1),MoverParameters->ActiveCab>=0?0:180); //obr�t wzgl�dem osi Z
    }
   if (smMechanik1) //mechanik od strony sprz�gu 1
    smMechanik1->iVisible=MoverParameters->ActiveCab<0;
  }
  //ABu: Przechyly na zakretach
  //Ra: przechy�k� za�atwiamy na etapie przesuwania modelu
  //if (ObjSqrDist<80000) ABuModelRoll(); //przechy�ki od 400m
 }
 if (MoverParameters->Battery)
 {//sygna�y czo�a pociagu //Ra: wy�wietlamy bez ogranicze� odleg�o�ci, by by�y widoczne z daleka
  if (TestFlag(iLights[0],1))
   {btHeadSignals11.TurnOn(); btnOn=true;}
  //else btHeadSignals11.TurnOff();
  if (TestFlag(iLights[0],4))
   {btHeadSignals12.TurnOn(); btnOn=true;}
  //else btHeadSignals12.TurnOff();
  if (TestFlag(iLights[0],16))
   {btHeadSignals13.TurnOn(); btnOn=true;}
  //else btHeadSignals13.TurnOff();
  if (TestFlag(iLights[1],1))
   {btHeadSignals21.TurnOn(); btnOn=true;}
  //else btHeadSignals21.TurnOff();
  if (TestFlag(iLights[1],4))
   {btHeadSignals22.TurnOn(); btnOn=true;}
  //else btHeadSignals22.TurnOff();
  if (TestFlag(iLights[1],16))
   {btHeadSignals23.TurnOn(); btnOn=true;}
  //else btHeadSignals23.TurnOff();
 }
}
//ABu 29.01.05 koniec przeklejenia *************************************

double __fastcall ABuAcos(const vector3 &calc_temp)
{  //Odpowiednik funkcji Arccos, bo cos mi tam nie dzialalo.
 return atan2(-calc_temp.x,calc_temp.z); //Ra: tak pro�ciej
}

TDynamicObject* __fastcall TDynamicObject::ABuFindNearestObject(TTrack *Track,TDynamicObject *MyPointer,int &CouplNr)
{//zwraca wskaznik do obiektu znajdujacego sie na torze (Track), kt�rego sprz�g jest najblizszy kamerze
 //s�u�y np. do ��czenia i rozpinania sprz�g�w
 //WE: Track      - tor, na ktorym odbywa sie poszukiwanie
 //    MyPointer  - wskaznik do obiektu szukajacego
 //WY: CouplNr    - kt�ry sprz�g znalezionego obiektu jest bli�szy kamerze

 //Uwaga! Jesli CouplNr==-2 to szukamy njblizszego obiektu, a nie sprzegu!!!

 if ((Track->iNumDynamics)>0)
 {//o ile w og�le jest co przegl�da� na tym torze
  vector3 poz; //pozycja pojazdu XYZ w scenerii
  vector3 kon; //wektor czo�a wzgl�dem �rodka pojazdu wzgl�em pocz�tku toru
  vector3 tmp; //wektor pomi�dzy kamer� i sprz�giem
  double dist; //odleg�o��
  for (int i=0;i<Track->iNumDynamics;i++)
  {
   if (CouplNr==-2)
   {//wektor [kamera-obiekt] - poszukiwanie obiektu
    tmp=Global::GetCameraPosition()-Track->Dynamics[i]->vPosition;
    dist=tmp.x*tmp.x+tmp.y*tmp.y+tmp.z*tmp.z; //odleg�o�� do kwadratu
    if (dist<100.0) //10 metr�w
     return Track->Dynamics[i];
   }
   else //je�li (CouplNr) inne niz -2, szukamy sprz�gu
   {//wektor [kamera-sprzeg0], potem [kamera-sprzeg1]
    //Powinno byc wyliczone, ale nie zaszkodzi drugi raz:
    //(bo co, jesli nie wykonuje sie obrotow wozkow?)
    Track->Dynamics[i]->modelRot.z=ABuAcos(Track->Dynamics[i]->Axle0.pPosition-Track->Dynamics[i]->Axle1.pPosition);
    poz=Track->Dynamics[i]->vPosition; //pozycja �rodka pojazdu
    kon=vector3( //po�o�enie przodu wzgl�dem �rodka
     -((0.5*Track->Dynamics[i]->MoverParameters->Dim.L)*sin(Track->Dynamics[i]->modelRot.z)),
     0, //yyy... je�li du�e pochylenie i d�ugi pojazd, to mo�e by� problem
     +((0.5*Track->Dynamics[i]->MoverParameters->Dim.L)*cos(Track->Dynamics[i]->modelRot.z))
    );
    tmp=Global::GetCameraPosition()-poz-kon;
    dist=tmp.x*tmp.x+tmp.y*tmp.y+tmp.z*tmp.z; //odleg�o�� do kwadratu
    if (dist<25.0) //5 metr�w
    {
     CouplNr=0;
     return Track->Dynamics[i];
    }
    tmp=Global::GetCameraPosition()-poz+kon;
    dist=tmp.x*tmp.x+tmp.y*tmp.y+tmp.z*tmp.z; //odleg�o�� do kwadratu
    if (dist<25.0) //5 metr�w
    {
     CouplNr=1;
     return Track->Dynamics[i];
    }
   }
  }
  return NULL;
 }
 return NULL;
}



TDynamicObject* __fastcall TDynamicObject::ABuScanNearestObject(TTrack *Track,double ScanDir,double ScanDist,int &CouplNr)
{//skanowanie toru w poszukiwaniu obiektu najblizszego kamerze
 //double MyScanDir=ScanDir;  //Moja orientacja na torze.  //Ra: nie u�ywane
 if (ABuGetDirection()<0) ScanDir=-ScanDir;
 TDynamicObject* FoundedObj;
 FoundedObj=ABuFindNearestObject(Track,this,CouplNr); //zwraca numer sprz�gu znalezionego pojazdu
 if (FoundedObj==NULL)
 {
  double ActDist;    //Przeskanowana odleglosc.
  double CurrDist=0; //Aktualna dlugosc toru.
  if (ScanDir>=0) ActDist=Track->Length()-RaTranslationGet(); //???-przesuni�cie w�zka wzgl�dem Point1 toru
             else ActDist=RaTranslationGet(); //przesuni�cie w�zka wzgl�dem Point1 toru
  while (ActDist<ScanDist)
  {
   ActDist+=CurrDist;
   if (ScanDir>0) //do przodu
   {
    if (Track->iNextDirection)
    {
     Track=Track->CurrentNext();
     ScanDir=-ScanDir;
    }
    else
     Track=Track->CurrentNext();
   }
   else //do ty�u
   {
    if (Track->iPrevDirection)
     Track=Track->CurrentPrev();
    else
    {
     Track=Track->CurrentPrev();
     ScanDir=-ScanDir;
    }
   }
   if (Track!=NULL)
   { //jesli jest kolejny odcinek toru
    CurrDist=Track->Length();
    FoundedObj=ABuFindNearestObject(Track, this, CouplNr);
    if (FoundedObj!=NULL)
     ActDist=ScanDist;
   }
   else //Jesli nie ma, to wychodzimy.
    ActDist=ScanDist;
  }
 } //Koniec szukania najblizszego toru z jakims obiektem.
 return FoundedObj;
}

//ABu 01.11.04 poczatek wyliczania przechylow pudla **********************
void __fastcall TDynamicObject::ABuModelRoll()
{//ustawienie przechy�ki pojazdu i jego zawarto�ci
// Ra: przechy�k� za�atwiamy na etapie przesuwania modelu
}

//ABu 06.05.04 poczatek wyliczania obrotow wozkow **********************

void __fastcall TDynamicObject::ABuBogies()
{//Obracanie wozkow na zakretach. Na razie uwzgl�dnia tylko zakr�ty,
 //bez zadnych gorek i innych przeszkod.
 if ((smBogie[0]!=NULL)&&(smBogie[1]!=NULL))
 {
  modelRot.z=ABuAcos(Axle0.pPosition-Axle1.pPosition); //k�t obrotu pojazdu
  //bogieRot[0].z=ABuAcos(Axle0.pPosition-Axle3.pPosition);
  bogieRot[0].z=Axle0.vAngles.z;
  bogieRot[0]=RadToDeg(modelRot-bogieRot[0]); //mno�enie wektora przez sta��
  smBogie[0]->SetRotateXYZ(bogieRot[0]);
  //bogieRot[1].z=ABuAcos(Axle2.pPosition-Axle1.pPosition);
  bogieRot[1].z=Axle1.vAngles.z;
  bogieRot[1]=RadToDeg(modelRot-bogieRot[1]);
  smBogie[1]->SetRotateXYZ(bogieRot[1]);
 }
};
//ABu 06.05.04 koniec wyliczania obrotow wozkow ************************

//ABu 16.03.03 sledzenie toru przed obiektem: **************************
void __fastcall TDynamicObject::ABuCheckMyTrack()
{//Funkcja przypisujaca obiekt prawidlowej tablicy Dynamics,
 //bo gdzies jest jakis blad i wszystkie obiekty z danego
 //pociagu na poczatku stawiane sa na jednym torze i wpisywane
 //do jednej tablicy. Wykonuje sie tylko raz - po to 'ABuChecked'
 TTrack* OldTrack=MyTrack;
 TTrack* NewTrack=Axle0.GetTrack();
 if ((NewTrack!=OldTrack)&&OldTrack)
 {
  OldTrack->RemoveDynamicObject(this);
  NewTrack->AddDynamicObject(this);
 }
 iAxleFirst=0; //pojazd powi�zany z przedni� osi� - Axle0
}

//Ra: w poni�szej funkcji jest problem ze sprz�gami
TDynamicObject* __fastcall TDynamicObject::ABuFindObject(TTrack *Track,int ScanDir,Byte &CouplFound,double &dist)
{//Zwraca wska�nik najbli�szego obiektu znajduj�cego si�
 //na torze w okre�lonym kierunku, ale tylko wtedy, kiedy
 //obiekty mog� si� zderzy�, tzn. nie mijaj� si�.

 //WE: Track      - tor, na ktorym odbywa sie poszukiwanie,
 //    MyPointer  - wskaznik do obiektu szukajacego. //Ra: zamieni�em na "this"
 //    ScanDir    - kierunek szukania na torze (+1:w stron� Point2, -1:w stron� Point1)
 //    MyScanDir  - kierunek szukania obiektu szukajacego (na jego torze); Ra: nie potrzebne
 //    MyCouplFound - nr sprzegu obiektu szukajacego; Ra: nie potrzebne

 //WY: wskaznik do znalezionego obiektu.
 //    CouplFound - nr sprzegu znalezionego obiektu
 if (Track->iNumDynamics>0)
 {//sens szukania na tym torze jest tylko, gdy s� na nim pojazdy
  double ObjTranslation; //pozycja najblizszego obiektu na torze
  double MyTranslation; //pozycja szukaj�cego na torze
  double MinDist=Track->Length(); //najmniejsza znaleziona odleglo�� (zaczynamy od d�ugo�ci toru)
  double TestDist; //robocza odleg�o�� od kolejnych pojazd�w na danym odcinku
  int iMinDist=-1;  //indeks wykrytego obiektu
  //if (Track->iNumDynamics>1)
  // iMinDist+=0; //tymczasowo pu�apka
  if (MyTrack==Track) //gdy szukanie na tym samym torze
   MyTranslation=RaTranslationGet(); //po�o�enie w�zka wzgl�dem Point1 toru
  else //gdy szukanie na innym torze
   if (ScanDir>0)
    MyTranslation=0; //szukanie w kierunku Point2 (od zera) - jeste�my w Point1
   else
    MyTranslation=MinDist; //szukanie w kierunku Point1 (do zera) - jeste�my w Point2
  if (ScanDir>=0)
  {//je�li szukanie w kierunku Point2
   for (int i=0;i<Track->iNumDynamics;i++)
   {//p�tla po pojazdach
    if (Track->Dynamics[i]!=this) //szukaj�cy si� nie liczy
    {
     TestDist=(Track->Dynamics[i]->RaTranslationGet())-MyTranslation; //odleg�og�o�� tamtego od szukaj�cego
     if ((TestDist>0)&&(TestDist<=MinDist))
     {//gdy jest po w�a�ciwej stronie i bli�ej ni� jaki� wcze�niejszy
      CouplFound=(Track->Dynamics[i]->RaDirectionGet()>0)?1:0; //to, bo (ScanDir>=0)
      if (Track->iCategoryFlag&254) //trajektoria innego typu ni� tor kolejowy
      {//dla tor�w nie ma sensu tego sprawdza�, rzadko co jedzie po jednej szynie i si� mija
       //Ra: mijanie samochod�w wcale nie jest proste
       // Przesuniecie wzgledne pojazdow. Wyznaczane, zeby sprawdzic,
       // czy pojazdy faktycznie sie zderzaja (moga byc przesuniete
       // w/m siebie tak, ze nie zachodza na siebie i wtedy sie mijaja).
       double RelOffsetH; //wzajemna odleg�o�� poprzeczna
       if (CouplFound) //my na tym torze by�my byli w kierunku Point2
        //dla CouplFound=1 s� zwroty zgodne - istotna r�nica przesuni��
        RelOffsetH=(MoverParameters->OffsetTrackH-Track->Dynamics[i]->MoverParameters->OffsetTrackH);
       else
        //dla CouplFound=0 s� zwroty przeciwne - przesuni�cia sumuj� si�
        RelOffsetH=(MoverParameters->OffsetTrackH+Track->Dynamics[i]->MoverParameters->OffsetTrackH);
       if (RelOffsetH<0) RelOffsetH=-RelOffsetH;
       if (RelOffsetH+RelOffsetH>MoverParameters->Dim.W+Track->Dynamics[i]->MoverParameters->Dim.W)
        continue; //odleg�o�� wi�ksza od po�owy sumy szeroko�ci - kolizji nie b�dzie
       //je�li zahaczenie jest niewielkie, a jest miejsce na poboczu, to zjecha� na pobocze
      }
      iMinDist=i; //potencjalna kolizja
      MinDist=TestDist; //odleglo�� pomi�dzy aktywnymi osiami pojazd�w
     }
    }
   }
  }
  else //(ScanDir<0)
  {
   for (int i=0;i<Track->iNumDynamics;i++)
   {
    if (Track->Dynamics[i]!=this)
    {
     TestDist=MyTranslation-(Track->Dynamics[i]->RaTranslationGet()); //???-przesuni�cie w�zka wzgl�dem Point1 toru
     if ((TestDist>0)&&(TestDist<MinDist))
     {
      CouplFound=(Track->Dynamics[i]->RaDirectionGet()>0)?0:1; //odwrotnie, bo (ScanDir<0)
      if (Track->iCategoryFlag&254) //trajektoria innego typu ni� tor kolejowy
      {//dla tor�w nie ma sensu tego sprawdza�, rzadko co jedzie po jednej szynie i si� mija
       //Ra: mijanie samochod�w wcale nie jest proste
       // Przesuni�cie wzgl�dne pojazd�w. Wyznaczane, �eby sprawdzi�,
       // czy pojazdy faktycznie si� zderzaj� (mog� by� przesuni�te
       // w/m siebie tak, �e nie zachodz� na siebie i wtedy sie mijaj�).
       double RelOffsetH; //wzajemna odleg�o�� poprzeczna
       if (CouplFound) //my na tym torze by�my byli w kierunku Point1
        //dla CouplFound=1 s� zwroty zgodne - istotna r�nica przesuni��
        RelOffsetH=(MoverParameters->OffsetTrackH-Track->Dynamics[i]->MoverParameters->OffsetTrackH);
       else
        //dla CouplFound=0 s� zwroty przeciwne - przesuni�cia sumuj� si�
        RelOffsetH=(MoverParameters->OffsetTrackH+Track->Dynamics[i]->MoverParameters->OffsetTrackH);
       if (RelOffsetH<0) RelOffsetH=-RelOffsetH;
       if (RelOffsetH+RelOffsetH>MoverParameters->Dim.W+Track->Dynamics[i]->MoverParameters->Dim.W)
        continue; //odleg�o�� wi�ksza od po�owy sumy szeroko�ci - kolizji nie b�dzie
      }
      iMinDist=i; //potencjalna kolizja
      MinDist=TestDist; //odleglo�� pomi�dzy aktywnymi osiami pojazd�w
     }
    }
   }
  }
  dist+=MinDist; //doliczenie odleg�o�ci przeszkody albo d�ugo�ci odcinka do przeskanowanej odleg�o�ci
  return (iMinDist>=0)?Track->Dynamics[iMinDist]:NULL;
 }
 dist+=Track->Length(); //doliczenie d�ugo�ci odcinka do przeskanowanej odleg�o�ci
 return NULL; //nie ma pojazd�w na torze, to jest NULL
}

int TDynamicObject::DettachStatus(int dir)
{//sprawdzenie odleg�o�ci sprz�g�w rzeczywistych od strony (dir): 0=prz�d,1=ty�
 //Ra: dziwne, �e ta funkcja nie jest u�ywana
 if (!MoverParameters->Couplers[dir].CouplingFlag)
  return 0; //je�li nic nie pod��czone, to jest OK
 return (MoverParameters->DettachStatus(dir)); //czy jest w odpowiedniej odleg�o�ci?
}

int TDynamicObject::Dettach(int dir)
{//roz��czenie sprz�g�w rzeczywistych od strony (dir): 0=prz�d,1=ty�
 //zwraca mask� bitow� aktualnych sprzeg�w (0 je�li roz��czony)
 if (MoverParameters->Couplers[dir].CouplingFlag) //odczepianie, o ile co� pod��czone
  MoverParameters->Dettach(dir);
 return MoverParameters->Couplers[dir].CouplingFlag; //sprz�g po roz��czaniu (czego si� nie da odpi��
}

void TDynamicObject::CouplersDettach(double MinDist,int MyScanDir)
{//funkcja roz��czajaca pod��czone sprz�gi, je�li odleg�o�� przekracza (MinDist)
 //MinDist - dystans minimalny, dla ktorego mozna roz��cza�
 if (MyScanDir>0)
 {
  if (PrevConnected) //pojazd od strony sprz�gu 0
  {
   if (MoverParameters->Couplers[0].CoupleDist>MinDist) //sprz�gi wirtualne zawsze przekraczaj�
   {
    if ((PrevConnectedNo?PrevConnected->NextConnected:PrevConnected->PrevConnected)==this)
    {//Ra: nie roz��czamy znalezionego, je�eli nie do nas pod��czony (mo�e jecha� w innym kierunku)
     PrevConnected->MoverParameters->Couplers[PrevConnectedNo].Connected=NULL;
     if (PrevConnectedNo==0)
     {
      PrevConnected->PrevConnectedNo=2; //sprz�g 0 nie pod��czony
      PrevConnected->PrevConnected=NULL;
     }
     else if (PrevConnectedNo==1)
     {
      PrevConnected->NextConnectedNo=2; //sprz�g 1 nie pod��czony
      PrevConnected->NextConnected=NULL;
     }
    }
    //za to zawsze od��czamy siebie
    PrevConnected=NULL;
    PrevConnectedNo=2; //sprz�g 0 nie pod��czony
    MoverParameters->Couplers[0].Connected=NULL;
   }
  }
 }
 else
 {
  if (NextConnected) //pojazd od strony sprz�gu 1
  {
   if (MoverParameters->Couplers[1].CoupleDist>MinDist) //sprz�gi wirtualne zawsze przekraczaj�
   {
    if ((NextConnectedNo?NextConnected->NextConnected:NextConnected->PrevConnected)==this)
    {//Ra: nie roz��czamy znalezionego, je�eli nie do nas pod��czony (mo�e jecha� w innym kierunku)
     NextConnected->MoverParameters->Couplers[NextConnectedNo].Connected=NULL;
     if (NextConnectedNo==0)
     {
      NextConnected->PrevConnectedNo=2; //sprz�g 0 nie pod��czony
      NextConnected->PrevConnected=NULL;
     }
     else if (NextConnectedNo==1)
     {
      NextConnected->NextConnectedNo=2; //sprz�g 1 nie pod��czony
      NextConnected->NextConnected=NULL;
     }
    }
    NextConnected=NULL;
    NextConnectedNo=2; //sprz�g 1 nie pod��czony
    MoverParameters->Couplers[1].Connected=NULL;
   }
  }
 }
}

void TDynamicObject::ABuScanObjects(int ScanDir,double ScanDist)
{//skanowanie toru w poszukiwaniu koliduj�cych pojazd�w
 //ScanDir - okre�la kierunek poszukiwania zale�nie od zwrotu pr�dko�ci pojazdu
 // ScanDir=1 - od strony Coupler0, ScanDir=-1 - od strony Coupler1
 int MyScanDir=ScanDir;  //zapami�tanie kierunku poszukiwa� na torze pocz�tkowym, wzgl�dem sprz�g�w
 TTrackFollower *FirstAxle=(MyScanDir>0?&Axle0:&Axle1); //mo�na by to trzyma� w trainset
 TTrack *Track=FirstAxle->GetTrack(); //tor na kt�rym "stoi" skrajny w�zek (mo�e by� inny ni� tor pojazdu)
 if (FirstAxle->GetDirection()<0) //czy o� jest ustawiona w stron� Point1?
  ScanDir=-ScanDir; //je�li tak, to kierunek szukania b�dzie przeciwny (teraz wzgl�dem toru)
 Byte MyCouplFound; //numer sprz�gu do pod��czenia w obiekcie szukajacym
 MyCouplFound=(MyScanDir<0)?1:0;
 Byte CouplFound; //numer sprz�gu w znalezionym obiekcie (znaleziony wype�ni)
 TDynamicObject *FoundedObj; //znaleziony obiekt
 double ActDist=0; //przeskanowana odleglo��; odleg�o�� do zawalidrogi
 FoundedObj=ABuFindObject(Track,ScanDir,CouplFound,ActDist); //zaczynamy szuka� na tym samym torze

/*
 if (FoundedObj) //jak co� znajdzie, to �ledzimy
 {//powt�rzenie wyszukiwania tylko do zastawiania pu�epek podczas test�w
  if (ABuGetDirection()<0) ScanDir=ScanDir; //ustalenie kierunku wzgl�dem toru
  FoundedObj=ABuFindObject(Track,this,ScanDir,CouplFound);
 }
*/

 if (DebugModeFlag)
 if (FoundedObj) //kod s�u��cy do logowania b��d�w
  if (CouplFound==0)
  {
   if (FoundedObj->PrevConnected)
    if (FoundedObj->PrevConnected!=this) //od�wie�enie tego samego si� nie liczy
     WriteLog("0! Coupler warning on "+asName+":"+AnsiString(MyCouplFound)+" - "+FoundedObj->asName+":0 connected to "+FoundedObj->PrevConnected->asName+":"+AnsiString(FoundedObj->PrevConnectedNo));
  }
  else
  {
   if (FoundedObj->NextConnected)
    if (FoundedObj->NextConnected!=this) //od�wie�enie tego samego si� nie liczy
     WriteLog("0! Coupler warning on "+asName+":"+AnsiString(MyCouplFound)+" - "+FoundedObj->asName+":1 connected to "+FoundedObj->NextConnected->asName+":"+AnsiString(FoundedObj->NextConnectedNo));
  }


 if (FoundedObj==NULL) //je�li nie ma na tym samym, szukamy po okolicy
 {//szukanie najblizszego toru z jakims obiektem
  //praktycznie przeklejone z TraceRoute()...
  //double CurrDist=0; //aktualna dlugosc toru
  if (ScanDir>=0) //uwzgl�dniamy kawalek przeanalizowanego wcze�niej toru
   ActDist=Track->Length()-FirstAxle->GetTranslation(); //odleg�o�� osi od Point2 toru
  else
   ActDist=FirstAxle->GetTranslation(); //odleg�o�� osi od Point1 toru
  while (ActDist<ScanDist)
  {
   //ActDist+=CurrDist; //odleg�o�� ju� przeanalizowana
   if (ScanDir>0) //w kierunku Point2 toru
   {
    if (Track?Track->iNextDirection:false) //je�li nast�pny tor jest podpi�ty od Point2
     ScanDir=-ScanDir; //to zmieniamy kierunek szukania na tym torze
    Track=Track->CurrentNext(); //potem dopiero zmieniamy wska�nik
   }
   else //w kierunku Point1
   {
    if (Track?!Track->iPrevDirection:true) //je�li poprzedni tor nie jest podpi�ty od Point2
     ScanDir=-ScanDir; //to zmieniamy kierunek szukania na tym torze
    Track=Track->CurrentPrev(); //potem dopiero zmieniamy wska�nik
   }
   if (Track)
   {//jesli jest kolejny odcinek toru
    //CurrDist=Track->Length(); //doliczenie tego toru do przejrzanego dystandu
    FoundedObj=ABuFindObject(Track,ScanDir,CouplFound,ActDist); //przejrzenie pojazd�w tego toru
    if (FoundedObj)
    {
     //ActDist=ScanDist; //wyj�cie z p�tli poszukiwania
     break;
    }
   }
   else //je�li toru nie ma, to wychodzimy
   {
    ActDist=ScanDist+1.0; //koniec przegl�dania tor�w
    break;
   }
  }
 } // Koniec szukania najbli�szego toru z jakim� obiektem.
 //teraz odczepianie i je�li co� si� znalaz�o, doczepianie.
 if (MyScanDir>0?PrevConnected:NextConnected)
  if ((MyScanDir>0?PrevConnected:NextConnected)!=FoundedObj)
   CouplersDettach(1.0,MyScanDir); //od��czamy, je�li dalej ni� metr
 // i ��czenie sprz�giem wirtualnym
 if (FoundedObj)
 {//siebie mo�na bezpiecznie pod��czy� jednostronnie do znalezionego
  MoverParameters->Attach(MyCouplFound,CouplFound,FoundedObj->MoverParameters,ctrain_virtual);
  //MoverParameters->Couplers[MyCouplFound].Render=false; //wirtualnego nie renderujemy
  if (MyCouplFound==0)
  {
   PrevConnected=FoundedObj; //pojazd od strony sprz�gu 0
   PrevConnectedNo=CouplFound;
  }
  else
  {
   NextConnected=FoundedObj; //pojazd od strony sprz�gu 1
   NextConnectedNo=CouplFound;
  }
  if (FoundedObj->MoverParameters->Couplers[CouplFound].CouplingFlag==ctrain_virtual)
  {//Ra: wpinamy si� wirtualnym tylko je�li znaleziony ma wirtualny sprz�g
   FoundedObj->MoverParameters->Attach(CouplFound,MyCouplFound,this->MoverParameters,ctrain_virtual);
   if (CouplFound==0) //je�li widoczny sprz�g 0 znalezionego
   {
    if (DebugModeFlag)
    if (FoundedObj->PrevConnected)
     if (FoundedObj->PrevConnected!=this)
      WriteLog("1! Coupler warning on "+asName+":"+AnsiString(MyCouplFound)+" - "+FoundedObj->asName+":0 connected to "+FoundedObj->PrevConnected->asName+":"+AnsiString(FoundedObj->PrevConnectedNo));
    FoundedObj->PrevConnected=this;
    FoundedObj->PrevConnectedNo=MyCouplFound;
   }
   else //je�li widoczny sprz�g 1 znalezionego
   {
    if (DebugModeFlag)
    if (FoundedObj->NextConnected)
     if (FoundedObj->NextConnected!=this)
      WriteLog("1! Coupler warning on "+asName+":"+AnsiString(MyCouplFound)+" - "+FoundedObj->asName+":1 connected to "+FoundedObj->NextConnected->asName+":"+AnsiString(FoundedObj->NextConnectedNo));
    FoundedObj->NextConnected=this;
    FoundedObj->NextConnectedNo=MyCouplFound;
   }
  }
  //Ra: je�li dwa samochody si� mijaj� na odcinku przed zawrotk�, to odleg�o�� mi�dzy nimi nie mo�e by� liczona w linii prostej!
  fTrackBlock=MoverParameters->Couplers[MyCouplFound].CoupleDist; //odleg�o�� do najbli�szego pojazdu w linii prostej
  if (Track->iCategoryFlag>1) //je�li samoch�d
   if (ActDist>MoverParameters->Dim.L+FoundedObj->MoverParameters->Dim.L) //przeskanowana odleg�o�� wi�ksza od d�ugo�ci pojazd�w
  //else if (ActDist<ScanDist) //dla samochod�w musi by� uwzgl�dniona droga do zawr�cenia
    fTrackBlock=ActDist; //ta odleg�o�� jest wiecej warta
  //if (fTrackBlock<500.0)
  // WriteLog("Collision of "+AnsiString(fTrackBlock)+"m detected by "+asName+":"+AnsiString(MyCouplFound)+" with "+FoundedObj->asName);
 }
 else //nic nie znalezione, to nie ma przeszk�d
  fTrackBlock=10000.0;
}
//----------ABu: koniec skanowania pojazdow

__fastcall TDynamicObject::TDynamicObject()
{
 modelShake=vector3(0,0,0);
 fTrackBlock=10000.0; //brak przeszkody na drodze
 btnOn=false;
 vUp=vWorldUp;
 vFront=vWorldFront;
 vLeft=vWorldLeft;
 iNumAxles=0;
 MoverParameters=NULL;
 Mechanik=NULL;
 MechInside=false;
 //McZapkie-270202
 Controller=AIdriver;
 bDisplayCab=false; //030303
 bBrakeAcc=false;
 NextConnected=PrevConnected=NULL;
 NextConnectedNo=PrevConnectedNo=2; //ABu: Numery sprzegow. 2=nie pod��czony
 CouplCounter=50; //b�dzie sprawdza� na pocz�tku
 asName="";
 bEnabled=true;
 MyTrack=NULL;
 //McZapkie-260202
 dRailLength=25.0;
 for (int i=0;i<MaxAxles;i++)
  dRailPosition[i]=0.0;
 for (int i=0;i<MaxAxles;i++)
  dWheelsPosition[i]=0.0; //b�dzie wczytane z MMD
 iAxles=0;
 dWheelAngle[0]=0.0;
 dWheelAngle[1]=0.0;
 dWheelAngle[2]=0.0;
 //Winger 160204 - pantografy
 //PantVolume = 3.5;
 NoVoltTime=0;
 //smPatykird1[0]=smPatykird1[1]=NULL;
 //smPatykird2[0]=smPatykird2[1]=NULL;
 //smPatykirg1[0]=smPatykirg1[1]=NULL;
 //smPatykirg2[0]=smPatykirg2[1]=NULL;
 //smPatykisl[0]=smPatykisl[1]=NULL;
 dDoorMoveL=0.0;
 dDoorMoveR=0.0;
 //for (int i=0;i<8;i++)
 //{
 // DoorSpeedFactor[i]=random(150);
 // DoorSpeedFactor[i]=(DoorSpeedFactor[i]+100)/100;
 //}
 //iAnimatedAxles=0; //ilo�� obracanych osi
 //iAnimatedDoors=0;
/* stare
 for (int i=0;i<MaxAnimatedAxles;i++)
  smAnimatedWheel[i]=NULL;
*/
 //for (int i=0;i<MaxAnimatedDoors;i++)
 // smAnimatedDoor[i]=NULL;
 mdModel=NULL;
 mdKabina=NULL;
 ReplacableSkinID[0]=0;
 ReplacableSkinID[1]=0;
 ReplacableSkinID[2]=0;
 ReplacableSkinID[3]=0;
 ReplacableSkinID[4]=0;
 iAlpha=0x30300030; //tak gdy tekstury wymienne nie maj� przezroczysto�ci
 //smWiazary[0]=smWiazary[1]=NULL;
 smWahacze[0]=smWahacze[1]=smWahacze[2]=smWahacze[3]=NULL;
 fWahaczeAmp=0;
 mdLoad=NULL;
 mdLowPolyInt=NULL;
 mdPrzedsionek=NULL;
 smMechanik0=smMechanik1=NULL;
 smBuforLewy[0]=smBuforLewy[1]=NULL;
 smBuforPrawy[0]=smBuforPrawy[1]=NULL;
 enginevolume=0;
 smBogie[0]=smBogie[1]=NULL;
 bogieRot[0]=bogieRot[1]=vector3(0,0,0);
 modelRot=vector3(0,0,0);
 eng_vol_act=0.8;
 eng_dfrq=0;
 eng_frq_act=1;
 eng_turbo=0;
 cp1=cp2=sp1=sp2=0;
 iDirection=1; //stoi w kierunku tradycyjnym (0, gdy jest odwr�cony)
 iAxleFirst=0; //numer pierwszej osi w kierunku ruchu (prze��czenie nast�puje, gdy osie sa na tym samym torze)
 iInventory=0; //flagi bitowe posiadanych submodeli (zaktualizuje si� po wczytaniu MMD)
 RaLightsSet(0,0); //pocz�tkowe zerowanie stanu �wiate�
 //Ra: domy�lne ilo�ci animacji dla zgodno�ci wstecz (gdy brak ilo�ci podanych w MMD)
 iAnimType[ANIM_WHEELS ]=8; //0-osie (8)
 iAnimType[ANIM_DOORS  ]=8; //1-drzwi (8)
 iAnimType[ANIM_LEVERS ]=4; //2-wahacze (4) - np. nogi konia
 iAnimType[ANIM_BUFFERS]=4; //3-zderzaki (4)
 iAnimType[ANIM_BOOGIES]=2; //4-w�zki (2)
 iAnimType[ANIM_PANTS  ]=2; //5-pantografy (2)
 iAnimType[ANIM_STEAMS ]=0; //6-t�oki (nap�d parowozu)
 iAnimations=0; //na razie nie ma �adnego
 pAnimations=NULL;
 pAnimated=NULL;
 fShade=0.0; //standardowe o�wietlenie na starcie
 iHornWarning=1; //numer syreny do u�ycia po otrzymaniu sygna�u do jazdy
 asDestination="none"; //stoj�cy nigdzie nie jedzie
 pValveGear=NULL; //Ra: tymczasowo
 iCabs=0; //maski bitowe modeli kabin
 smBrakeSet=NULL; //nastawa hamulca (wajcha)
 smLoadSet=NULL; //nastawa �adunku (wajcha)
 smWiper=NULL; //wycieraczka (poniek�d te� wajcha)
}

__fastcall TDynamicObject::~TDynamicObject()
{//McZapkie-250302 - zamykanie logowania parametrow fizycznych
 SafeDelete(Mechanik);
 SafeDelete(MoverParameters);
 //Ra: wy��czanie d�wi�k�w powinno by� dodane w ich destruktorach, ale si� sypie
/* to te� si� sypie
 for (int i=0;i<MaxAxles;++i)
  rsStukot[i].Stop();   //dzwieki poszczegolnych osi
 rsSilnik.Stop();
 rsWentylator.Stop();
 rsPisk.Stop();
 rsDerailment.Stop();
 sPantUp.Stop();
 sPantDown.Stop();
 sBrakeAcc.Stop(); //dzwiek przyspieszacza
 rsDiesielInc.Stop();
 rscurve.Stop();
*/
 delete[] pAnimations; //obiekty obs�uguj�ce animacj�
 delete[] pAnimated; //lista animowanych submodeli
}

double __fastcall TDynamicObject::Init(
 AnsiString Name, //nazwa pojazdu, np. "EU07-424"
 AnsiString BaseDir, //z kt�rego katalogu wczytany, np. "PKP/EU07"
 AnsiString asReplacableSkin, //nazwa wymiennej tekstury
 AnsiString Type_Name, //nazwa CHK/MMD, np. "303E"
 TTrack *Track, //tor pocz�tkowy wstwawienia (pocz�tek sk�adu)
 double fDist, //dystans wzgl�dem punktu 1
 AnsiString DriverType, //typ obsady
 double fVel, //pr�dko�� pocz�tkowa
 AnsiString TrainName, //nazwa sk�adu, np. "PE2307"
 float Load, //ilo�� �adunku
 AnsiString LoadType, //nazwa �adunku
 bool Reversed, //true, je�li ma sta� odwrotnie w sk�adzie
 AnsiString MoreParams //dodatkowe parametry wczytywane w postaci tekstowej
)
{//Ustawienie pocz�tkowe pojazdu
 iDirection=(Reversed?0:1); //Ra: 0, je�li ma by� wstawiony jako obr�cony ty�em
 asBaseDir="dynamic\\"+BaseDir+"\\"; //McZapkie-310302
 asName=Name;
 AnsiString asAnimName=""; //zmienna robocza do wyszukiwania osi i w�zk�w
 //Ra: zmieniamy znaczenie obsady na jednoliterowe, �eby dosadzi� kierownika
 if (DriverType=="headdriver") DriverType="h"; //steruj�cy kabin� +1
 else if (DriverType=="reardriver") DriverType="r"; //steruj�cy kabin� -1
 //else if (DriverType=="connected") DriverType="c"; //tego trzeba si� pozby� na rzecz ukrotnienia
 else if (DriverType=="passenger") DriverType="p"; //to do przemy�lenia
 else if (DriverType=="nobody") DriverType=""; //nikt nie siedzi
 int Cab; //numer kabiny z obsad� (nie mo�na zaj�� obu)
 if (DriverType.Pos("h")||DriverType.Pos("1")) //od przodu sk�adu
  Cab=1;//iDirection?1:-1; //iDirection=1 gdy normalnie, =0 odwrotnie
 else if (DriverType.Pos("r")||DriverType.Pos("2")) //od ty�u sk�adu
  Cab=-1;//iDirection?-1:1;
 //else if (DriverType=="c") //uaktywnianie wirtualnej kabiny
 // Cab=0; //iDirection?1:-1; //to przestawi steruj�cy
 else if (DriverType=="p")
 {
  if (random(6)<3) Cab=1; else Cab=-1; //losowy przydzia� kabiny
 }
 else if (DriverType=="")
  Cab=0;
/* to nie ma uzasadnienia
 else
 {//obsada nie rozpoznana
  Cab=0;  //McZapkie-010303: w przyszlosci dac tez pomocnika, palacza, konduktora itp.
  Error("Unknown DriverType description: "+DriverType);
  DriverType="nobody";
 }
*/
 //utworzenie parametr�w fizyki
 MoverParameters=new TMoverParameters(iDirection?fVel:-fVel,Type_Name,asName,Load,LoadType,Cab);
 iLights=MoverParameters->iLights; //wska�nik na stan w�asnych �wiate� (zmienimy dla rozrz�dczych EZT)
 //McZapkie: TypeName musi byc nazw� CHK/MMD pojazdu
 if (!MoverParameters->LoadChkFile(asBaseDir))
 {//jak wczytanie CHK si� nie uda, to b��d
  if (ConversionError==-8)
   ErrorLog("Missed file: "+BaseDir+"\\"+Type_Name);
  Error("Cannot load dynamic object "+asName+" from:\r\n"+BaseDir+"\\"+Type_Name+"\r\nError "+ConversionError+" in line "+LineCount);
  return 0.0; //zerowa d�ugo�� to brak pojazdu
 }
 bool driveractive=(fVel!=0.0); //je�li pr�dko�� niezerowa, to aktywujemy ruch
 if (!MoverParameters->CheckLocomotiveParameters(driveractive,(fVel>0?1:-1)*Cab*(iDirection?1:-1))) //jak jedzie lub obsadzony to gotowy do drogi
 {
  Error("Parameters mismatch: dynamic object "+asName+" from\n"+BaseDir+"\\"+Type_Name);
  return 0.0; //zerowa d�ugo�� to brak pojazdu
 }
 MoverParameters->BrakeLevelSet(MoverParameters->BrakeCtrlPos); //poprawienie hamulca po ewentualnym przestawieniu przez Pascal

//dodatkowe parametry yB
 MoreParams+="."; //wykonuje o jedn� iteracj� za ma�o, wi�c trzeba mu doda� kropk� na koniec
 int kropka=MoreParams.Pos("."); //znajd� kropke
 AnsiString ActPar; //na parametry
 while (kropka>0) //jesli sa kropki jeszcze
 {
  int dlugosc=MoreParams.Length();
  ActPar=MoreParams.SubString(1,kropka-1).UpperCase();     //pierwszy parametr;
  MoreParams=MoreParams.SubString(kropka+1,dlugosc-kropka);  //reszta do dalszej obrobki
  kropka=MoreParams.Pos(".");

  if(ActPar.SubString(1,1)=="B") //jesli hamulce
  {  //sprawdzanie kolejno nastaw
   WriteLog("Wpis hamulca: " + ActPar);
   if (ActPar.Pos("G")>0) {MoverParameters->BrakeDelaySwitch(bdelay_G);}
   if (ActPar.Pos("P")>0) {MoverParameters->BrakeDelaySwitch(bdelay_P);}
   if (ActPar.Pos("R")>0) {MoverParameters->BrakeDelaySwitch(bdelay_R);}
   if (ActPar.Pos("M")>0) {MoverParameters->BrakeDelaySwitch(bdelay_R); MoverParameters->BrakeDelaySwitch(bdelay_R+bdelay_M);}
   //wylaczanie hamulca
   if (ActPar.Pos("<>")>0) //wylaczanie na probe hamowania naglego
   {
    MoverParameters->BrakeStatus|=128; //wylacz
   }
   if (ActPar.Pos("0")>0) //wylaczanie na sztywno
   {
    MoverParameters->BrakeStatus|=128; //wylacz
    MoverParameters->Hamulec->ForceEmptiness();
    MoverParameters->BrakeReleaser(1);  //odluznij automatycznie
   }
   if (ActPar.Pos("E")>0) //oprozniony
   {
    MoverParameters->Hamulec->ForceEmptiness();
    MoverParameters->BrakeReleaser(1);  //odluznij automatycznie
    MoverParameters->Pipe->CreatePress(0);
    MoverParameters->Pipe2->CreatePress(0);
   }
   if (ActPar.Pos("Q")>0) //oprozniony
   {
//    MoverParameters->Hamulec->ForceEmptiness(); //TODO: sprawdzic, dlaczego pojawia sie blad przy uzyciu tej linijki w lokomotywie
    MoverParameters->BrakeReleaser(1);  //odluznij automatycznie
    MoverParameters->Pipe->CreatePress(0.0);
    MoverParameters->PipePress=0.0;
    MoverParameters->Pipe2->CreatePress(0.0);
    MoverParameters->ScndPipePress=0.0;
    MoverParameters->PantVolume=1;
    MoverParameters->PantPress=0;
    MoverParameters->CompressedVolume=0;
   }

   if (ActPar.Pos("1")>0) //wylaczanie 10%
   {
    if (random(10)<1) //losowanie 1/10
    {
     MoverParameters->BrakeStatus|=128; //wylacz
     MoverParameters->Hamulec->ForceEmptiness();
     MoverParameters->BrakeReleaser(1);  //odluznij automatycznie
    }
   }
   if (ActPar.Pos("X")>0) //agonalny wylaczanie 20%, usrednienie przekladni
   {
    if (random(100)<20) //losowanie 20/100
    {
     MoverParameters->BrakeStatus|=128; //wylacz
     MoverParameters->Hamulec->ForceEmptiness();
     MoverParameters->BrakeReleaser(1);  //odluznij automatycznie
    }
    if (MoverParameters->BrakeCylMult[2]*MoverParameters->BrakeCylMult[1]>0.01) //jesli jest nastawiacz mechaniczny PL
    {
     float rnd=random(100);
     if (rnd<20) //losowanie 20/100         usrednienie
     {
      MoverParameters->BrakeCylMult[2]=MoverParameters->BrakeCylMult[1]=(MoverParameters->BrakeCylMult[2]+MoverParameters->BrakeCylMult[1])/2;
     }
     else
     if (rnd<70) //losowanie 70/100-20/100    oslabienie
     {
      MoverParameters->BrakeCylMult[1]=MoverParameters->BrakeCylMult[1]*0.50;
      MoverParameters->BrakeCylMult[2]=MoverParameters->BrakeCylMult[2]*0.75;
     }
     else
     if (rnd<80) //losowanie 80/100-70/100    tylko prozny
     {
      MoverParameters->BrakeCylMult[2]=MoverParameters->BrakeCylMult[1];
     }
     else      //tylko ladowny
     {
      MoverParameters->BrakeCylMult[1]=MoverParameters->BrakeCylMult[2];
     }
    }
   }
   //nastawianie ladunku
   if (ActPar.Pos("T")>0) //prozny
   { MoverParameters->DecBrakeMult(); MoverParameters->DecBrakeMult(); } //dwa razy w dol
   if (ActPar.Pos("H")>0) //ladowny I (dla P-� dalej prozny)
   { MoverParameters->IncBrakeMult(); MoverParameters->IncBrakeMult(); MoverParameters->DecBrakeMult(); } //dwa razy w gore i obniz
   if (ActPar.Pos("F")>0) //ladowny II
   { MoverParameters->IncBrakeMult(); MoverParameters->IncBrakeMult(); } //dwa razy w gore
   if (ActPar.Pos("N")>0) //parametr neutralny
   { }
  } //koniec hamulce
  else if(ActPar.SubString(1,1)=="") //tu mozna wpisac inny prefiks i inne rzeczy
  {
    //jakies inne prefiksy
  }

 } //koniec while kropka

 if (MoverParameters->CategoryFlag&2) //je�li samoch�d
 {//ustawianie samochodow na poboczu albo na �rodku drogi
  if (Track->fTrackWidth<3.5) //je�li droga w�ska
   MoverParameters->OffsetTrackH=0.0; //to stawiamy na �rodku, niezale�nie od stanu ruchu
  else
  if (driveractive) //od 3.5m do 8.0m jedzie po �rodku pasa, dla szerszych w odleg�o�ci 1.5m
   MoverParameters->OffsetTrackH=Track->fTrackWidth<=8.0?-Track->fTrackWidth*0.25:-1.5;
  else //jak stoi, to ko�em na poboczu i pobieramy szeroko�� razem z poboczem, ale nie z chodnikiem
   MoverParameters->OffsetTrackH=-0.5*(Track->WidthTotal()-MoverParameters->Dim.W)+0.05;
  iHornWarning=0; //nie b�dzie tr�bienia po podaniu zezwolenia na jazd�
 }
 //w wagonie tez niech jedzie
 //if (MoverParameters->MainCtrlPosNo>0 &&
 // if (MoverParameters->CabNo!=0)
 if (DriverType!="")
 {//McZapkie-040602: je�li co� siedzi w poje�dzie
  if (Name==AnsiString(Global::asHumanCtrlVehicle)) //je�li pojazd wybrany do prowadzenia
  {
   if (DebugModeFlag?false:MoverParameters->EngineType!=Dumb) //jak nie Debugmode i nie jest dumbem
    Controller=Humandriver; //wsadzamy tam steruj�cego
   else //w przeciwnym razie trzeba w��czy� pokazywanie kabiny
    bDisplayCab=true;
  }
  //McZapkie-151102: rozk�ad jazdy czytany z pliku *.txt z katalogu w kt�rym jest sceneria
  if (DriverType.Pos("h")||DriverType.Pos("r"))
  {//McZapkie-110303: mechanik i rozklad tylko gdy jest obsada
   //MoverParameters->ActiveCab=MoverParameters->CabNo; //ustalenie aktywnej kabiny (rozrz�d)
   Mechanik=new TController(Controller,this,Aggressive);
   if (TrainName.IsEmpty()) //je�li nie w sk�adzie
   {
    Mechanik->DirectionInitial();  //za��czenie rozrz�du (wirtualne kabiny) itd.
    Mechanik->PutCommand("Timetable:",iDirection?-fVel:fVel,0,NULL); //tryb poci�gowy z ustalon� pr�dko�ci� (wzgl�dem sprz�g�w)
   }
   //if (TrainName!="none")
   // Mechanik->PutCommand("Timetable:"+TrainName,fVel,0,NULL);
  }
  else
   if (DriverType=="p")
   {//obserwator w charakterze pasa�era
    //Ra: to jest niebezpieczne, bo w razie co b�dzie pomaga� hamulcem bezpiecze�stwa
    Mechanik=new TController(Controller,this,Easyman,false);
   }
 }
 // McZapkie-250202
 iAxles=(MaxAxles<MoverParameters->NAxles)?MaxAxles:MoverParameters->NAxles; //ilo�� osi
 //wczytywanie z pliku nazwatypu.mmd, w tym model
 LoadMMediaFile(asBaseDir,Type_Name,asReplacableSkin);
 //McZapkie-100402: wyszukiwanie submodeli sprzeg�w
 btCoupler1.Init("coupler1",mdModel,false); //false - ma by� wy��czony
 btCoupler2.Init("coupler2",mdModel,false);
 btCPneumatic1.Init("cpneumatic1",mdModel);
 btCPneumatic2.Init("cpneumatic2",mdModel);
 btCPneumatic1r.Init("cpneumatic1r",mdModel);
 btCPneumatic2r.Init("cpneumatic2r",mdModel);
 btPneumatic1.Init("pneumatic1",mdModel);
 btPneumatic2.Init("pneumatic2",mdModel);
 btPneumatic1r.Init("pneumatic1r",mdModel);
 btPneumatic2r.Init("pneumatic2r",mdModel);
 btCCtrl1.Init("cctrl1",mdModel,false);
 btCCtrl2.Init("cctrl2",mdModel,false);
 btCPass1.Init("cpass1",mdModel,false);
 btCPass2.Init("cpass2",mdModel,false);
 //sygnaly
 //ABu 060205: Zmiany dla koncowek swiecacych:
 btEndSignals11.Init("endsignal13",mdModel,false);
 btEndSignals21.Init("endsignal23",mdModel,false);
 btEndSignals13.Init("endsignal12",mdModel,false);
 btEndSignals23.Init("endsignal22",mdModel,false);
 iInventory|=btEndSignals11.Active()  ?0x01:0; //informacja, czy ma poszczeg�lne �wiat�a
 iInventory|=btEndSignals21.Active()  ?0x02:0;
 iInventory|=btEndSignals13.Active()  ?0x04:0;
 iInventory|=btEndSignals23.Active()  ?0x08:0;
 //ABu: to niestety zostawione dla kompatybilnosci modeli:
 btEndSignals1.Init("endsignals1",mdModel,false);
 btEndSignals2.Init("endsignals2",mdModel,false);
 btEndSignalsTab1.Init("endtab1",mdModel,false);
 btEndSignalsTab2.Init("endtab2",mdModel,false);
 iInventory|=btEndSignals1.Active()   ?0x10:0;
 iInventory|=btEndSignals2.Active()   ?0x20:0;
 iInventory|=btEndSignalsTab1.Active()?0x40:0; //tabliczki blaszane
 iInventory|=btEndSignalsTab2.Active()?0x80:0;
 //ABu Uwaga! tu zmienic w modelu!
 btHeadSignals11.Init("headlamp13",mdModel,false); //lewe
 btHeadSignals12.Init("headlamp11",mdModel,false); //g�rne
 btHeadSignals13.Init("headlamp12",mdModel,false); //prawe
 btHeadSignals21.Init("headlamp23",mdModel,false);
 btHeadSignals22.Init("headlamp21",mdModel,false);
 btHeadSignals23.Init("headlamp22",mdModel,false);
 TurnOff(); //resetowanie zmiennych submodeli
 //wyszukiwanie zderzakow
 if (mdModel) //je�li ma w czym szuka�
  for (int i=0;i<2;i++)
  {
   asAnimName=AnsiString("buffer_left0")+(i+1);
   smBuforLewy[i]=mdModel->GetFromName(asAnimName.c_str());
   if (smBuforLewy[i])
    smBuforLewy[i]->WillBeAnimated(); //ustawienie flagi animacji
   asAnimName=AnsiString("buffer_right0")+(i+1);
   smBuforPrawy[i]=mdModel->GetFromName(asAnimName.c_str());
   if (smBuforPrawy[i])
    smBuforPrawy[i]->WillBeAnimated();
  }
 for (int i=0;i<iAxles;i++) //wyszukiwanie osi (0 jest na ko�cu, dlatego dodajemy d�ugo��?)
  dRailPosition[i]=(Reversed?-dWheelsPosition[i]:(dWheelsPosition[i]+MoverParameters->Dim.L))+fDist;
 //McZapkie-250202 end.
 Track->AddDynamicObject(this); //wstawiamy do toru na pozycj� 0, a potem przesuniemy
 //McZapkie: zmieniono na ilosc osi brane z chk
 //iNumAxles=(MoverParameters->NAxles>3 ? 4 : 2 );
 iNumAxles=2;
 //McZapkie-090402: odleglosc miedzy czopami skretu lub osiami
 fAxleDist=Max0R(MoverParameters->BDist,MoverParameters->ADist);
 if (fAxleDist>MoverParameters->Dim.L-0.2) //nie mog� by� za daleko
  fAxleDist=MoverParameters->Dim.L-0.2; //bo b�dzie "walenie w mur"
 float fAxleDistHalf=fAxleDist*0.5;
 //WriteLog("Dynamic "+Type_Name+" of length "+MoverParameters->Dim.L+" at "+AnsiString(fDist));
 //if (Cab) //je�li ma obsad� - zgodno�� wstecz, je�li tor startowy ma Event0
 // if (Track->Event0) //je�li tor ma Event0
 //  if (fDist>=0.0) //je�li je�li w starych sceneriach pocz�tek sk�adu by�by wysuni�ty na ten tor
 //   if (fDist<=0.5*MoverParameters->Dim.L+0.2) //ale nie jest wysuni�ty
 //    fDist+=0.5*MoverParameters->Dim.L+0.2; //wysun�� go na ten tor
 //przesuwanie pojazdu tak, aby jego pocz�tek by� we wskazanym miejcu
 fDist-=0.5*MoverParameters->Dim.L; //dodajemy p� d�ugo�ci pojazdu, bo ustawiamy jego �rodek (zliczanie na minus)
 switch (iNumAxles)
 {//Ra: pojazdy wstawiane s� na tor pocz�tkowy, a potem przesuwane
  case 2: //ustawianie osi na torze
   Axle0.Init(Track,this,iDirection?1:-1);
   Axle0.Move((iDirection?fDist:-fDist)+fAxleDistHalf+0.01,false);
   Axle1.Init(Track,this,iDirection?1:-1);
   Axle1.Move((iDirection?fDist:-fDist)-fAxleDistHalf-0.01,false); //false, �eby nie generowa� event�w
   //Axle2.Init(Track,this,iDirection?1:-1);
   //Axle2.Move((iDirection?fDist:-fDist)-fAxleDistHalft+0.01),false);
   //Axle3.Init(Track,this,iDirection?1:-1);
   //Axle3.Move((iDirection?fDist:-fDist)+fAxleDistHalf-0.01),false);
  break;
  case 4:
   Axle0.Init(Track,this,iDirection?1:-1);
   Axle0.Move((iDirection?fDist:-fDist)+(fAxleDistHalf+MoverParameters->ADist*0.5),false);
   Axle1.Init(Track,this,iDirection?1:-1);
   Axle1.Move((iDirection?fDist:-fDist)-(fAxleDistHalf+MoverParameters->ADist*0.5),false);
   //Axle2.Init(Track,this,iDirection?1:-1);
   //Axle2.Move((iDirection?fDist:-fDist)-(fAxleDistHalf-MoverParameters->ADist*0.5),false);
   //Axle3.Init(Track,this,iDirection?1:-1);
   //Axle3.Move((iDirection?fDist:-fDist)+(fAxleDistHalf-MoverParameters->ADist*0.5),false);
  break;
 }
 Move(0.0001); //potrzebne do wyliczenia aktualnej pozycji; nie mo�e by� zero, bo nie przeliczy pozycji
 //teraz jeszcze trzeba przypisa� pojazdy do nowego toru, bo przesuwanie pocz�tkowe osi nie zrobi�o tego
 ABuCheckMyTrack(); //zmiana toru na ten, co o� Axle0 (o� z przodu)
 TLocation loc; //Ra: ustawienie pozycji do obliczania sprz�g�w
 loc.X=-vPosition.x;
 loc.Y=vPosition.z;
 loc.Z=vPosition.y;
 MoverParameters->Loc=loc; //normalnie przesuwa ComputeMovement() w Update()
 //pOldPos4=Axle1.pPosition; //Ra: nie u�ywane
 //pOldPos1=Axle0.pPosition;
 //ActualTrack= GetTrack(); //McZapkie-030303
 //ABuWozki 060504
 if (mdModel) //je�li ma w czym szuka�
 {
  smBogie[0]=mdModel->GetFromName("bogie1"); //Ra: bo nazwy s� ma�ymi
  smBogie[1]=mdModel->GetFromName("bogie2");
  if (!smBogie[0])
   smBogie[0]=mdModel->GetFromName("boogie01"); //Ra: alternatywna nazwa
  if (!smBogie[1])
   smBogie[1]=mdModel->GetFromName("boogie02"); //Ra: alternatywna nazwa
  if (smBogie[0])
   smBogie[0]->WillBeAnimated();
  if (smBogie[1])
   smBogie[1]->WillBeAnimated();
 }
 //ABu: zainicjowanie zmiennej, zeby nic sie nie ruszylo
 //w pierwszej klatce, potem juz liczona prawidlowa wartosc masy
 MoverParameters->ComputeConstans();
 /*Ra: to nie dzia�a - Event0 musi by� wykonywany ci�gle
 if (fVel==0.0) //je�li stoi
  if (MoverParameters->CabNo!=0) //i ma kogo� w kabinie
   if (Track->Event0) //a jest w tym torze event od stania
    RaAxleEvent(Track->Event0); //dodanie eventu stania do kolejki
 */
 return MoverParameters->Dim.L; //d�ugo�� wi�ksza od zera oznacza OK; 2mm docisku?
}

void __fastcall TDynamicObject::FastMove(double fDistance)
{
 MoverParameters->dMoveLen=MoverParameters->dMoveLen+fDistance;
}

void __fastcall TDynamicObject::Move(double fDistance)
{//przesuwanie pojazdu po trajektorii polega na przesuwaniu poszczeg�lnych osi
 //Ra: warto�� pr�dko�ci 2km/h ma ograniczy� aktywacj� event�w w przypadku drga�
 if (Axle0.GetTrack()==Axle1.GetTrack()) //przed przesuni�ciem
 {//powi�zanie pojazdu z osi� mo�na zmieni� tylko wtedy, gdy skrajne osie s� na tym samym torze
  if (MoverParameters->Vel>2) //|[km/h]| nie ma sensu zmiana osi, jesli pojazd drga na postoju
   iAxleFirst=(MoverParameters->V>=0.0)?1:0; //[m/s] ?1:0 - aktywna druga o� w kierunku jazdy
  //aktualnie eventy aktywuje druga o�, �eby AI nie wy��cza�o sobie semafora za szybko
 }
 bEnabled&=Axle0.Move(fDistance,!iAxleFirst); //o� z przodu pojazdu
 bEnabled&=Axle1.Move(fDistance,iAxleFirst); //o� z ty�u pojazdu
 //Axle2.Move(fDistance,false); //te nigdy pierwsze nie s�
 //Axle3.Move(fDistance,false);
 if (fDistance!=0.0) //nie liczy� ponownie, je�li stoi
 {//liczenie pozycji pojazdu tutaj, bo jest u�ywane w wielu miejscach
  vPosition=0.5*(Axle1.pPosition+Axle0.pPosition); //�rodek mi�dzy skrajnymi osiami
  vFront=Normalize(Axle0.pPosition-Axle1.pPosition); //kierunek ustawienia pojazdu (wektor jednostkowy)
  vLeft=Normalize(CrossProduct(vWorldUp,vFront)); //wektor poziomy w lewo, normalizacja potrzebna z powodu pochylenia (vFront)
  vUp=CrossProduct(vFront,vLeft); //wektor w g�r�, b�dzie jednostkowy
  double a=((Axle1.GetRoll()+Axle0.GetRoll())); //suma przechy�ek
  if (a!=0.0)
  {//wyznaczanie przechylenia tylko je�li jest przechy�ka
   //mo�na by pobra� wektory normalne z toru...
   mMatrix.Identity(); //ta macierz jest potrzebna g��wnie do wy�wietlania
   mMatrix.Rotation(a*0.5,vFront); //obr�t wzd�u� osi o przechy�k�
   vUp=mMatrix*vUp; //wektor w g�r� pojazdu (przekr�cenie na przechy�ce)
   //vLeft=mMatrix*DynamicObject->vLeft;
   //vUp=CrossProduct(vFront,vLeft); //wektor w g�r�
   //vLeft=Normalize(CrossProduct(vWorldUp,vFront)); //wektor w lewo
   vLeft=Normalize(CrossProduct(vUp,vFront)); //wektor w lewo
   //vUp=CrossProduct(vFront,vLeft); //wektor w g�r�
  }
  mMatrix.Identity(); //to te� mo�na by od razu policzy�, ale potrzebne jest do wy�wietlania
  mMatrix.BasisChange(vLeft,vUp,vFront); //przesuwanie jest jednak rzadziej ni� renderowanie
  mMatrix=Inverse(mMatrix); //wyliczenie macierzy dla pojazdu (potrzebna tylko do wy�wietlania?)
  //if (MoverParameters->CategoryFlag&2)
  {//przesuni�cia s� u�ywane po wyrzuceniu poci�gu z toru
   vPosition.x+=MoverParameters->OffsetTrackH*vLeft.x; //dodanie przesuni�cia w bok
   vPosition.z+=MoverParameters->OffsetTrackH*vLeft.z; //vLeft jest wektorem poprzecznym
   //if () na przechy�ce b�dzie dodatkowo zmiana wysoko�ci samochodu
   vPosition.y+=MoverParameters->OffsetTrackV;   //te offsety s� liczone przez moverparam
  }
  //Ra: skopiowanie pozycji do fizyki, tam potrzebna do zrywania sprz�g�w
  //MoverParameters->Loc.X=-vPosition.x; //robi to {Fast}ComputeMovement()
  //MoverParameters->Loc.Y= vPosition.z;
  //MoverParameters->Loc.Z= vPosition.y;
  //obliczanie pozycji sprz�g�w do liczenia zderze�
  vector3 dir=(0.5*MoverParameters->Dim.L)*vFront; //wektor sprz�gu
  vCoulpler[0]=vPosition+dir; //wsp�rz�dne sprz�gu na pocz�tku
  vCoulpler[1]=vPosition-dir; //wsp�rz�dne sprz�gu na ko�cu
  MoverParameters->vCoulpler[0]=vCoulpler[0]; //tymczasowo kopiowane na inny poziom
  MoverParameters->vCoulpler[1]=vCoulpler[1];
  //bCameraNear=
  //if (bCameraNear) //je�li istotne s� szczeg�y (blisko kamery)
  {//przeliczenie cienia
   TTrack *t0=Axle0.GetTrack(); //ju� po przesuni�ciu
   TTrack *t1=Axle1.GetTrack();
   if ((t0->eEnvironment==e_flat)&&(t1->eEnvironment==e_flat)) //mo�e by� e_bridge...
    fShade=0.0; //standardowe o�wietlenie
   else
   {//je�eli te tory maj� niestandardowy stopie� zacienienia (e_canyon, e_tunnel)
    if (t0->eEnvironment==t1->eEnvironment)
    {switch (t0->eEnvironment)
     {//typ zmiany o�wietlenia
      case e_canyon: fShade=0.65; break; //zacienienie w kanionie
      case e_tunnel: fShade=0.20; break; //zacienienie w tunelu
     }
    }
    else //dwa r�ne
    {//liczymy proporcj�
     double d=Axle0.GetTranslation(); //aktualne po�o�enie na torze
     if (Axle0.GetDirection()<0)
      d=t0->fTrackLength-d; //od drugiej strony liczona d�ugo��
     d/=fAxleDist; //rozsataw osi procentowe znajdowanie si� na torze
     switch (t0->eEnvironment)
     {//typ zmiany o�wietlenia - zak�adam, �e drugi tor ma e_flat
      case e_canyon: fShade=(d*0.65)+(1.0-d); break; //zacienienie w kanionie
      case e_tunnel: fShade=(d*0.20)+(1.0-d); break; //zacienienie w tunelu
     }
     switch (t1->eEnvironment)
     {//typ zmiany o�wietlenia - zak�adam, �e pierwszy tor ma e_flat
      case e_canyon: fShade=d+(1.0-d)*0.65; break; //zacienienie w kanionie
      case e_tunnel: fShade=d+(1.0-d)*0.20; break; //zacienienie w tunelu
     }
    }
   }
  }
 }
};

void __fastcall TDynamicObject::AttachPrev(TDynamicObject *Object, int iType)
{//Ra: doczepia Object na ko�cu sk�adu (nazwa funkcji mo�e by� myl�ca)
 //Ra: u�ywane tylko przy wczytywaniu scenerii
 /*
 //Ra: po wstawieniu pojazdu do scenerii nie mia� on ustawionej pozycji, teraz ju� ma
 TLocation loc;
 loc.X=-vPosition.x;
 loc.Y=vPosition.z;
 loc.Z=vPosition.y;
 MoverParameters->Loc=loc; //Ra: do obliczania sprz�g�w, na starcie nie s� przesuni�te
 loc.X=-Object->vPosition.x;
 loc.Y=Object->vPosition.z;
 loc.Z=Object->vPosition.y;
 Object->MoverParameters->Loc=loc; //ustawienie dodawanego pojazdu
 */
 MoverParameters->Attach(iDirection,Object->iDirection^1,Object->MoverParameters,iType,true);
 MoverParameters->Couplers[iDirection].Render=false;
 Object->MoverParameters->Attach(Object->iDirection^1,iDirection,MoverParameters,iType,true);
 Object->MoverParameters->Couplers[Object->iDirection^1].Render=true; //rysowanie sprz�gu w do��czanym
 if (iDirection)
 {//��czenie standardowe
  NextConnected=Object; //normalnie doczepiamy go sobie do sprz�gu 1
  NextConnectedNo=Object->iDirection^1;
 }
 else
 {//��czenie odwrotne
  PrevConnected=Object; //doczepiamy go sobie do sprz�gu 0, gdy stoimy odwrotnie
  PrevConnectedNo=Object->iDirection^1;
 }
 if (Object->iDirection)
 {//do��czany jest normalnie ustawiany
  Object->PrevConnected=this; //on ma nas z przodu
  Object->PrevConnectedNo=iDirection;
 }
 else
 {//do��czany jest odwrotnie ustawiany
  Object->NextConnected=this; //on ma nas z ty�u
  Object->NextConnectedNo=iDirection;
 }
 if (MoverParameters->TrainType&dt_EZT) //w przypadku ��czenia cz�on�w, �wiat�a w rozrz�dczym zale�� od stanu w silnikowym
  if (MoverParameters->Couplers[iDirection].AllowedFlag&ctrain_depot) //gdy sprz�gi ��czone warsztatowo (powiedzmy)
   if ((MoverParameters->Power<1.0)&&(Object->MoverParameters->Power>1.0)) //my nie mamy mocy, ale ten drugi ma
    iLights=Object->MoverParameters->iLights; //to w tym z moc� b�d� �wiat�a za��czane, a w tym bez tylko widoczne
   else if ((MoverParameters->Power>1.0)&&(Object->MoverParameters->Power<1.0)) //my mamy moc, ale ten drugi nie ma
    Object->iLights=MoverParameters->iLights; //to w tym z moc� b�d� �wiat�a za��czane, a w tym bez tylko widoczne
 return;
 //SetPneumatic(1,1); //Ra: to i tak si� nie wykonywa�o po return
 //SetPneumatic(1,0);
 //SetPneumatic(0,1);
 //SetPneumatic(0,0);
}

bool __fastcall TDynamicObject::UpdateForce(double dt, double dt1, bool FullVer)
{
 if (!bEnabled)
  return false;
 if (dt>0)
  MoverParameters->ComputeTotalForce(dt,dt1,FullVer); //wywalenie WS zale�y od ustawienia kierunku
 return true;
}

void __fastcall TDynamicObject::LoadUpdate()
{//prze�adowanie modelu �adunku
 // Ra: nie pr�bujemy wczytywa� modeli miliony razy podczas renderowania!!!
 if ((mdLoad==NULL)&&(MoverParameters->Load>0))
 {
  AnsiString asLoadName=asBaseDir+MoverParameters->LoadType+".t3d"; //zapami�tany katalog pojazdu
  //asLoadName=MoverParameters->LoadType;
  //if (MoverParameters->LoadType!=AnsiString("passengers"))
  Global::asCurrentTexturePath=asBaseDir; //bie��ca �cie�ka do tekstur to dynamic/...
  mdLoad=TModelsManager::GetModel(asLoadName.c_str()); //nowy �adunek
  Global::asCurrentTexturePath=AnsiString(szTexturePath); //z powrotem defaultowa sciezka do tekstur
  //Ra: w MMD mo�na by zapisa� po�o�enie modelu �adunku (np. w�giel) w zale�no�ci od za�adowania
 }
 else if (MoverParameters->Load==0)
  mdLoad=NULL; //nie ma �adunku
 //if ((mdLoad==NULL)&&(MoverParameters->Load>0))
 // {
 //  mdLoad=NULL; //Ra: to jest tu bez sensu - co autor mia� na my�li?
 // }
 MoverParameters->LoadStatus&=3; //po zako�czeniu b�dzie r�wne zero
};

/*
double __fastcall ComputeRadius(double p1x, double p1z, double p2x, double p2z,
                                double p3x, double p3z, double p4x, double p4z)
{

    double v1z= p1x-p2x;
    double v1x= p1z-p2z;
    double v4z= p3x-p4x;
    double v4x= p3z-p4z;
    double A1= p2z-p1z;
    double B1= p1x-p2x;
    double C1= -p1z*B1-p1x*A1;
    double A2= p4z-p3z;
    double B2= p3x-p4x;
    double C2= -p3z*B1-p3x*A1;
    double y= (A1*C2/A2-C1)/(B1-A1*B2/A2);
    double x= (-B2*y-C2)/A2;
}
*/
double __fastcall TDynamicObject::ComputeRadius(vector3 p1, vector3 p2, vector3 p3, vector3 p4)
{
//    vector3 v1

//    TLine l1= TLine(p1,p1-p2);
//    TLine l4= TLine(p4,p4-p3);
//    TPlane p1= l1.GetPlane();
//    vector3 pt;
//    CrossPoint(pt,l4,p1);
      double R= 0.0;
      vector3 p12= p1-p2;
      vector3 p34= p3-p4;
      p12= CrossProduct(p12,vector3(0.0,0.1,0.0));
      p12=Normalize(p12);
      p34= CrossProduct(p34,vector3(0.0,0.1,0.0));
      p34=Normalize(p34);
      if (fabs(p1.x-p2.x)>0.01)
       {
         if (fabs(p12.x-p34.x)>0.001)
          R=(p1.x-p4.x)/(p34.x-p12.x);
       }
      else
       {
         if (fabs(p12.z-p34.z)>0.001)
          R=(p1.z-p4.z)/(p34.z-p12.z);
       }
      return(R);
}

/*
double __fastcall TDynamicObject::ComputeRadius()
{
  double L=0;
  double d=0;
  d=sqrt(SquareMagnitude(Axle0.pPosition-Axle1.pPosition));
  L=Axle1.GetLength(Axle1.pPosition,Axle1.pPosition-Axle2.pPosition,Axle0.pPosition-Axle3.pPosition,Axle0.pPosition);

  double eps=0.01;
  double R= 0;
  double L_d;
  if ((L>0) || (d>0))
   {
     L_d= L-d;
     if (L_d>eps)
      {
        R=L*sqrt(L/(24*(L_d)));
      }
   }
  return R;
}
*/

/* Ra: na razie nie potrzebne
void __fastcall TDynamicObject::UpdatePos()
{
  MoverParameters->Loc.X= -vPosition.x;
  MoverParameters->Loc.Y=  vPosition.z;
  MoverParameters->Loc.Z=  vPosition.y;
}
*/

/*
Ra:
  Powinny by� dwie funkcje wykonuj�ce aktualizacj� fizyki. Jedna wykonuj�ca
krok oblicze�, powtarzana odpowiedni� liczb� razy, a druga wykonuj�ca zbiorcz�
aktualzacj� mniej istotnych element�w.
  Ponadto nale�a�o by ustali� odleg�o�� sk�ad�w od kamery i je�li przekracza
ona np. 10km, to traktowa� sk�ady jako uproszczone, np. bez wnikania w si�y
na sprz�gach, op�nienie dzia�ania hamulca itp. Oczywi�cie musi mie� to pewn�
histerez� czasow�, aby te tryby pracy nie prze��cza�y si� zbyt szybko.
*/


bool __fastcall TDynamicObject::Update(double dt, double dt1)
{
 if (dt==0) return true; //Ra: pauza
 if (!MoverParameters->PhysicActivation&&!MechInside) //to drugie, bo b�d�c w maszynowym blokuje si� fizyka
  return true;   //McZapkie: wylaczanie fizyki gdy nie potrzeba
 if (!MyTrack)
  return false; //pojazdy postawione na torach portalowych maj� MyTrack==NULL
 if (!bEnabled)
  return false; //a normalnie powinny mie� bEnabled==false

//Ra: przenios�em - no ju� lepiej tu, ni� w wy�wietlaniu!
//if ((MoverParameters->ConverterFlag==false) && (MoverParameters->TrainType!=dt_ET22))
//Ra: to nie mo�e tu by�, bo wy��cza spr�ark� w rozrz�dczym EZT!
//if ((MoverParameters->ConverterFlag==false)&&(MoverParameters->CompressorPower!=0))
// MoverParameters->CompressorFlag=false;
//if (MoverParameters->CompressorPower==2)
// MoverParameters->CompressorAllow=MoverParameters->ConverterFlag;

 //McZapkie-260202
 if ((MoverParameters->EnginePowerSource.SourceType==CurrentCollector)&&(MoverParameters->Power>1.0)) //aby rozrz�dczy nie opuszcza� silnikowemu
  if ((MechInside)||(MoverParameters->TrainType==dt_EZT))
  {
   //if ((!MoverParameters->PantCompFlag)&&(MoverParameters->CompressedVolume>=2.8))
   // MoverParameters->PantVolume=MoverParameters->CompressedVolume;
   if (MoverParameters->PantPress<(MoverParameters->TrainType==dt_EZT?2.4:3.5))
   {// 3.5 wg http://www.transportszynowy.pl/eu06-07pneumat.php
    //"Wy��czniki ci�nieniowe odbierak�w pr�du wy��czaj� sterowanie wy��cznika szybkiego oraz uniemo�liwiaj� podniesienie odbierak�w pr�du, gdy w instalacji rozrz�du ci�nienie spadnie poni�ej warto�ci 3,5 bara."
    //Ra 2013-12: Niebugoc�aw m�wi, �e w EZT podnosz� si� przy 2.5
    //if (!MoverParameters->PantCompFlag)
    // MoverParameters->PantVolume=MoverParameters->CompressedVolume;
    MoverParameters->PantFront(false); //opuszczenie pantograf�w przy niskim ci�nieniu
    MoverParameters->PantRear(false); //to idzie w ukrotnieniu, a nie powinno...
   }
   //Winger - automatyczne wylaczanie malej sprezarki
   else if (MoverParameters->PantPress>=4.8)
    MoverParameters->PantCompFlag=false;
  } //Ra: do Mover to trzeba przenie��, �eby AI te� mog�o sobie podpompowa�

    double dDOMoveLen;

    TLocation l;
    l.X=-vPosition.x; //przekazanie pozycji do fizyki
    l.Y=vPosition.z;
    l.Z=vPosition.y;
    TRotation r;
    r.Rx=r.Ry=r.Rz=0;
//McZapkie: parametry powinny byc pobierane z toru

    //TTrackShape ts;
    //ts.R=MyTrack->fRadius;
    //if (ABuGetDirection()<0) ts.R=-ts.R;
    ts.R=MyTrack->fRadius;
    if (bogieRot[0].z!=bogieRot[1].z) //wyliczenie promienia z obrot�w osi - modyfikacj� zg�osi� youBy
      ts.R=0.5*MoverParameters->BDist/sin(DegToRad(bogieRot[0].z-bogieRot[1].z)*0.5);
    //ts.R=ComputeRadius(Axle1.pPosition,Axle2.pPosition,Axle3.pPosition,Axle0.pPosition);
    //Ra: sk�adow� pochylenia wzd�u�nego mamy policzon� w jednostkowym wektorze vFront
    ts.Len=1.0; //Max0R(MoverParameters->BDist,MoverParameters->ADist);
    ts.dHtrack=-vFront.y; //Axle1.pPosition.y-Axle0.pPosition.y; //wektor mi�dzy skrajnymi osiami (!!!odwrotny)
    ts.dHrail=(Axle1.GetRoll()+Axle0.GetRoll())*0.5; //�rednia przechy�ka pud�a
    //TTrackParam tp;
    tp.Width=MyTrack->fTrackWidth;
//McZapkie-250202
    tp.friction=MyTrack->fFriction*Global::fFriction;
    tp.CategoryFlag=MyTrack->iCategoryFlag&15;
    tp.DamageFlag=MyTrack->iDamageFlag;
    tp.QualityFlag=MyTrack->iQualityFlag;
    if ((MoverParameters->Couplers[0].CouplingFlag>0)
      &&(MoverParameters->Couplers[1].CouplingFlag>0))
    {
     MoverParameters->InsideConsist=true;
    }
    else
    {
     MoverParameters->InsideConsist=false;
    }
   //napiecie sieci trakcyjnej

   TTractionParam tmpTraction;
   if ((MoverParameters->EnginePowerSource.SourceType==CurrentCollector)/*||(MoverParameters->TrainType==dt_EZT)*/)
   {
    if (Global::bLiveTraction)
    {//Ra 2013-12: to ni�ej jest chyba troch� bez sensu
     double v=MoverParameters->PantRearVolt;
     if (v==0.0)
     {v=MoverParameters->PantFrontVolt;
      if (v==0.0)
       if (MoverParameters->TrainType&(dt_EZT|dt_ET40|dt_ET41|dt_ET42)) //dwucz�ony mog� mie� sprz�g WN
        v=MoverParameters->GetTrainsetVoltage(); //ostatnia szansa
     }
     if (v!=0.0)
     {//je�li jest zasilanie
      NoVoltTime=0;
      tmpTraction.TractionVoltage=v;
     }
     else
     {NoVoltTime=NoVoltTime+dt;
      if (NoVoltTime>0.3) //je�li brak zasilania d�u�ej ni� przez 1 sekund�
       tmpTraction.TractionVoltage=0; //Ra 2013-12: po co tak?
       //pControlled->MainSwitch(false); //mo�e tak?
     }
    }
    else
     tmpTraction.TractionVoltage=0.95*MoverParameters->EnginePowerSource.MaxVoltage;
   }
   else
    tmpTraction.TractionVoltage=0.95*MoverParameters->EnginePowerSource.MaxVoltage;
   tmpTraction.TractionFreq=0;
   tmpTraction.TractionMaxCurrent=7500; //Ra: chyba za du�o? powinno wywala� przy 1500
   tmpTraction.TractionResistivity=0.3;




     /*if  ((Global::bLiveTraction) && ((MoverParameters->PantFront(true)) || (MoverParameters->PantRear(true))))
{
TGround::GetTraction;
}   */
//McZapkie: predkosc w torze przekazac do TrackParam
//McZapkie: Vel ma wymiar km/h (absolutny), V ma wymiar m/s , taka przyjalem notacje
    tp.Velmax=MyTrack->VelocityGet();

    if (Mechanik)
    {
     MoverParameters->EqvtPipePress= GetEPP(); //srednie cisnienie w PG

//yB: cos (AI) tu jest nie kompatybilne z czyms (hamulce)
//   if (Controller!=Humandriver)
//    if (Mechanik->LastReactionTime>0.5)
//     {
//      MoverParameters->BrakeCtrlPos=0;
//      Mechanik->LastReactionTime=0;
//     }

      //Mechanik->PhysicsLog(); //tymczasowo logowanie
      Mechanik->UpdateSituation(dt1); //przeb�yski �wiadomo�ci AI
      //Mechanik->PhysicsLog(); //tymczasowo logowanie
    }
//    else
//    { MoverParameters->SecuritySystemReset(); }
    //if (MoverParameters->ActiveCab==0)
    //    MoverParameters->SecuritySystemReset(); //Ra: to tu nie powinno by�, czuwak za��czany jest bateri�, ewentualnie rozrz�dem
//    else
//     if ((Controller!=Humandriver)&&(MoverParameters->BrakeCtrlPos<0)&&(!TestFlag(MoverParameters->BrakeStatus,1))&&((MoverParameters->CntrlPipePress)>0.51))
//       {//Ra: to jest do poprawienia przy okazji SPKS
////        MoverParameters->PipePress=0.50;
//        MoverParameters->BrakeLevelSet(0); //Ra: co to mia�o by� ???? to nie pozwala wyluzowa� AI w EN57 !!!!

//       }

    //fragment "z EXE Kursa"
    if (MoverParameters->Mains) //nie wchodzi� w funkcj� bez potrzeby
     if ((!MoverParameters->Battery)&&(Controller==Humandriver)&&(MoverParameters->EngineType!=DieselEngine)&&(MoverParameters->EngineType!=WheelsDriven))
     {//je�li bateria wy��czona, a nie diesel ani drezyna reczna
      if (MoverParameters->MainSwitch(false)) //wy��czy� zasilanie
       MoverParameters->EventFlag=true;
     }
    if (MoverParameters->TrainType==dt_ET42)
    {//powinny by� wszystkie dwucz�ony oraz EZT
/*
     //Ra: to jest bez sensu, bo wy��cza WS przy przechodzeniu przez "wewn�trzne" kabiny (z powodu ActiveCab)
     //trzeba to zrobi� inaczej, np. dla cz�onu A sprawdza�, czy jest B
     //albo sprawdza� w momencie za��czania WS i zmiany w sprz�gach
     if (((TestFlag(MoverParameters->Couplers[1].CouplingFlag,ctrain_controll))&&(MoverParameters->ActiveCab>0)&&(NextConnected->MoverParameters->TrainType!=dt_ET42))||((TestFlag(MoverParameters->Couplers[0].CouplingFlag,ctrain_controll))&&(MoverParameters->ActiveCab<0)&&(PrevConnected->MoverParameters->TrainType!=dt_ET42)))
     {//sprawdzenie, czy z ty�u kabiny mamy drugi cz�on
      if (MoverParameters->MainSwitch(false))
       MoverParameters->EventFlag=true;
     }
     if ((!(TestFlag(MoverParameters->Couplers[1].CouplingFlag,ctrain_controll))&&(MoverParameters->ActiveCab>0))||(!(TestFlag(MoverParameters->Couplers[0].CouplingFlag,ctrain_controll))&&(MoverParameters->ActiveCab<0)))
     {
      if (MoverParameters->MainSwitch(false))
       MoverParameters->EventFlag=true;
     }
*/
    }


//McZapkie-260202 - dMoveLen przyda sie przy stukocie kol
    dDOMoveLen=GetdMoveLen()+MoverParameters->ComputeMovement(dt,dt1,ts,tp,tmpTraction,l,r);
    MoverParameters->UpdateBatteryVoltage(dt); //jest ju� w Mover.cpp
    if (MoverParameters->EnginePowerSource.SourceType==CurrentCollector) //tylko je�li pantografuj�cy
     if (MoverParameters->Power>1.0) //w rozrz�dczym nie (jest b��d w FIZ!)
      MoverParameters->UpdatePantVolume(dt); //Ra: pneumatyka pantograf�w przeniesiona do Mover.cpp!
//yB: zeby zawsze wrzucalo w jedna strone zakretu
    MoverParameters->AccN*=-ABuGetDirection();
    //if (dDOMoveLen!=0.0) //Ra: nie mo�e by�, bo blokuje Event0
     Move(dDOMoveLen);
    if (!bEnabled) //usuwane pojazdy nie maj� toru
    {//pojazd do usuni�cia
     Global::pGround->bDynamicRemove=true; //sprawdzi�
     return false;
    }
    Global::ABuDebug=dDOMoveLen/dt1;
    ResetdMoveLen();
//McZapkie-260202
//tupot mew, tfu, stukot kol:
    DWORD stat;
//taka prowizorka zeby sciszyc stukot dalekiej lokomotywy
   double ObjectDist;
   double vol=0;
   //double freq; //Ra: nie u�ywane
   ObjectDist=SquareMagnitude(Global::pCameraPosition-vPosition);
//McZapkie-270202
   if (MyTrack->fSoundDistance!=-1)
   {
     if (ObjectDist<rsStukot[0].dSoundAtt*rsStukot[0].dSoundAtt*15.0)
      {
        vol= (20.0+MyTrack->iDamageFlag)/21;
        if (MyTrack->eEnvironment==e_tunnel)
         {
          vol*=1.1;
          //freq=1.02;
         }
        else
        if (MyTrack->eEnvironment==e_bridge)
         {
          vol*=1.2;
          //freq=0.99;                             //MC: stukot w zaleznosci od tego gdzie jest tor
         }
        if (MyTrack->fSoundDistance!=dRailLength)
         {
           dRailLength=MyTrack->fSoundDistance;
           for (int i=0; i<iAxles; i++)
           {
             dRailPosition[i]=dWheelsPosition[i]+MoverParameters->Dim.L;
           }
          }
        if (dRailLength!=-1)
        {
          if (abs(MoverParameters->V)>0)
           {
              for (int i=0; i<iAxles; i++)
              {
               dRailPosition[i]-=dDOMoveLen*Sign(dDOMoveLen);
               if (dRailPosition[i]<0)
                 {
  //McZapkie-040302
                   if (i==iAxles-1)
                    {
                      rsStukot[0].Stop();
                      MoverParameters->AccV+= 0.5*GetVelocity()/(1+MoverParameters->Vmax);
                    }
                   else
                    {rsStukot[i+1].Stop();}
                   rsStukot[i].Play(vol,0,MechInside,vPosition); //poprawic pozycje o uklad osi
                   if (i==1)
                      MoverParameters->AccV-= 0.5*GetVelocity()/(1+MoverParameters->Vmax);
                   dRailPosition[i]+=dRailLength;
                 }
              }
           }
        }
      }
   }
//McZapkie-260202 end

//yB: przyspieszacz (moze zadziala, ale dzwiek juz jest)
int flag=MoverParameters->Hamulec->GetSoundFlag();
if((bBrakeAcc)&&(TestFlag(flag,sf_Acc))&&(ObjectDist<2500))
  {
   sBrakeAcc->SetVolume(-ObjectDist*3-(FreeFlyModeFlag?0:2000));
   sBrakeAcc->Play(0,0,0);
   sBrakeAcc->SetPan(10000*sin(ModCamRot));
  }
if ((rsUnbrake.AM!=0)&&(ObjectDist<5000))
  {
   if ((TestFlag(flag,sf_CylU)) && ((MoverParameters->BrakePress*MoverParameters->MaxBrakePress[3])>0.05))
    {
     vol=Min0R(0.2+1.6*sqrt((MoverParameters->BrakePress>0?MoverParameters->BrakePress:0)/MoverParameters->MaxBrakePress[3]),1);
     vol=vol+(FreeFlyModeFlag?0:-0.5)-ObjectDist/5000;
     rsUnbrake.SetPan(10000*sin(ModCamRot));
     rsUnbrake.Play(vol,DSBPLAY_LOOPING,MechInside,GetPosition());
    }
   else
    rsUnbrake.Stop();
  }


   //fragment z EXE Kursa
      /* if (MoverParameters->TrainType==dt_ET42)
           {
           if ((MoverParameters->DynamicBrakeType=dbrake_switch) && ((MoverParameters->BrakePress > 0.2) || ( MoverParameters->PipePress < 0.36 )))
                        {
                        MoverParameters->StLinFlag=true;
                        }
           else
           if ((MoverParameters->DynamicBrakeType=dbrake_switch) && (MoverParameters->BrakePress < 0.1))
                        {
                        MoverParameters->StLinFlag=false;

                        }
           }   */
    if ((MoverParameters->TrainType==dt_ET40) || (MoverParameters->TrainType==dt_EP05))
    {//dla ET40 i EU06 automatyczne cofanie nastawnika - i tak nie b�dzie to dzia�a� dobrze...
     /* if ((MoverParameters->MainCtrlPos>MoverParameters->MainCtrlActualPos)&&(abs(MoverParameters->Im)>MoverParameters->IminHi))
        {
         MoverParameters->DecMainCtrl(1);
        } */
     if (( !Console::Pressed(Global::Keys[k_IncMainCtrl]))&&(MoverParameters->MainCtrlPos>MoverParameters->MainCtrlActualPos))
     {
      MoverParameters->DecMainCtrl(1);
     }
     if (( !Console::Pressed(Global::Keys[k_DecMainCtrl]))&&(MoverParameters->MainCtrlPos<MoverParameters->MainCtrlActualPos))
     {
      MoverParameters->IncMainCtrl(1);
     }
    }



 if (MoverParameters->Vel!=0)
 {//McZapkie-050402: krecenie kolami:
  dWheelAngle[0]+=114.59155902616464175359630962821*MoverParameters->V*dt1/MoverParameters->WheelDiameterL; //przednie toczne
  dWheelAngle[1]+=MoverParameters->nrot*dt1*360.0; //nap�dne
  dWheelAngle[2]+=114.59155902616464175359630962821*MoverParameters->V*dt1/MoverParameters->WheelDiameterT; //tylne toczne
  if (dWheelAngle[0]>360.0) dWheelAngle[0]-=360.0; //a w drug� stron� jak si� kr�c�?
  if (dWheelAngle[1]>360.0) dWheelAngle[1]-=360.0;
  if (dWheelAngle[2]>360.0) dWheelAngle[2]-=360.0;
 }
 if (pants) //pantograf mo�e by� w wagonie kuchennym albo poje�dzie rewizyjnym (np. SR61)
 {//przeliczanie k�t�w dla pantograf�w
  double k; //tymczasowy k�t
  double PantDiff;
  TAnimPant *p; //wska�nik do obiektu danych pantografu
  for (int i=0;i<iAnimType[ANIM_PANTS];++i)
  {//p�tla po wszystkich pantografach
   p=pants[i].fParamPants;
   if (p->PantWys<0)
   {//patograf zosta� po�amany, liczony nie b�dzie
    if (p->fAngleL>p->fAngleL0)
     p->fAngleL-=0.2*dt1; //nieco szybciej ni� jak dla opuszczania
    if (p->fAngleL<p->fAngleL0)
     p->fAngleL=p->fAngleL0; //k�t graniczny
    if (p->fAngleU<M_PI)
     p->fAngleU+=0.5*dt1; //g�rne si� musi rusza� szybciej.
    if (p->fAngleU>M_PI)
     p->fAngleU=M_PI;
    continue;
   }
   PantDiff=p->PantTraction-p->PantWys; //docelowy-aktualny
   switch (i) //numer pantografu
   {//trzeba usun�� to rozr�nienie
    case 0:
     if (Global::bLiveTraction?false:!p->hvPowerWire) //je�li nie ma drutu, mo�e pooszukiwa�
      MoverParameters->PantFrontVolt=(p->PantWys>=1.4)?0.95*MoverParameters->EnginePowerSource.MaxVoltage:0.0;
     else
      if (MoverParameters->PantFrontUp?(PantDiff<0.01):false)
      {
       if ((MoverParameters->PantFrontVolt==0.0)&&(MoverParameters->PantRearVolt==0.0))
        sPantUp.Play(vol,0,MechInside,vPosition);
       if (p->hvPowerWire) //TODO: wyliczy� trzeba pr�d przypadaj�cy na pantograf i wstawi� do GetVoltage()
        MoverParameters->PantFrontVolt=p->hvPowerWire->psPower?p->hvPowerWire->psPower->GetVoltage(0):p->hvPowerWire->NominalVoltage;
       else
        MoverParameters->PantFrontVolt=0.0;
      }
      else
       MoverParameters->PantFrontVolt=0.0;
    break;
    case 1:
     if (Global::bLiveTraction?false:!p->hvPowerWire) //je�li nie ma drutu, mo�e pooszukiwa�
      MoverParameters->PantRearVolt=(p->PantWys>=1.4)?0.95*MoverParameters->EnginePowerSource.MaxVoltage:0.0;
     else
      if (MoverParameters->PantRearUp?(PantDiff<0.01):false)
      {
       if ((MoverParameters->PantRearVolt==0.0)&&(MoverParameters->PantFrontVolt==0.0))
        sPantUp.Play(vol,0,MechInside,vPosition);
       if (p->hvPowerWire) //TODO: wyliczy� trzeba pr�d przypadaj�cy na pantograf i wstawi� do GetVoltage()
        MoverParameters->PantRearVolt=p->hvPowerWire->psPower?p->hvPowerWire->psPower->GetVoltage(0):p->hvPowerWire->NominalVoltage;
       else
        MoverParameters->PantRearVolt=0.0;
      }
      else
       MoverParameters->PantRearVolt=0.0;
    break;
   } //pozosta�e na razie nie obs�ugiwane
   if (MoverParameters->PantPress>(MoverParameters->TrainType==dt_EZT?2.5:3.3)) //Ra 2013-12: Niebugoc�aw m�wi, �e w EZT podnosz� si� przy 2.5
    pantspeedfactor=0.015*(MoverParameters->PantPress)*dt1; //z EXE Kursa  //Ra: wysoko�� zale�y od ci�nienia !!!
   else
    pantspeedfactor=0.0;
   if (pantspeedfactor<0) pantspeedfactor=0;
   k=p->fAngleL;
   if (i?MoverParameters->PantRearUp:MoverParameters->PantFrontUp) //je�li ma by� podniesiony
   {if (PantDiff>0.001) //je�li nie dolega do drutu
    {//je�li poprzednia wysoko�� jest mniejsza ni� po��dana, zwi�kszy� k�t dolnego ramienia zgodnie z ci�nieniem
     if (pantspeedfactor>0.55*PantDiff) //0.55 to oko�o pochodna k�ta po wysoko�ci
      k+=0.55*PantDiff; //ograniczenie "skoku" w danej klatce
     else
      k+=pantspeedfactor; //dolne rami�
     //je�li przekroczono k�t graniczny, zablokowa� pantograf (wymaga interwencji poci�gu sieciowego)
    }
    else if (PantDiff<-0.001)
    {//drut si� obni�y� albo zosta� podniesiony za wysoko
     //je�li wysoko�� jest zbyt du�a, wyznaczy� zmniejszenie k�ta
     //je�li zmniejszenie k�ta jest zbyt du�e, przej�� do trybu �amania pantografu
     //if (PantFrontDiff<-0.05) //skok w d� o 5cm daje z��manie pantografu
     k+=0.4*PantDiff; //mniej ni� pochodna k�ta po wysoko�ci
    } //je�li wysoko�� jest dobra, nic wi�cej nie liczy�
   }
   else
   {//je�li ma by� na dole
    if (k>p->fAngleL0) //je�li wy�ej ni� po�o�enie wyj�ciowe
     k-=0.15*dt1; //ruch w d�
    if (k<p->fAngleL0)
     k=p->fAngleL0; //po�o�enie minimalne
   }
   if (k!=p->fAngleL)
   {//�eby nie liczy� w kilku miejscach ani gdy nie potrzeba
    if (k+p->fAngleU<M_PI)
    {//o ile nie zosta� osi�gni�ty k�t maksymalny
     p->fAngleL=k; //zmieniony k�t
     //wyliczy� k�t g�rnego ramienia z wzoru (a)cosinusowego
     //=acos((b*cos()+c)/a)
     //p->dPantAngleT=acos((1.22*cos(k)+0.535)/1.755); //g�rne rami�
     p->fAngleU=acos((p->fLenL1*cos(k)+p->fHoriz)/p->fLenU1); //g�rne rami�
     //wyliczy� aktualn� wysoko�� z wzoru sinusowego
     //h=a*sin()+b*sin()
     p->PantWys=p->fLenL1*sin(k)+p->fLenU1*sin(p->fAngleU)+p->fHeight; //wysoko�� ca�o�ci
    }
   }
  } //koniec p�tli po pantografach
  if ((MoverParameters->PantFrontSP==false)&&(MoverParameters->PantFrontUp==false))
  {
   sPantDown.Play(vol,0,MechInside,vPosition);
   MoverParameters->PantFrontSP=true;
  }
  if ((MoverParameters->PantRearSP==false)&&(MoverParameters->PantRearUp==false))
  {
   sPantDown.Play(vol,0,MechInside,vPosition);
   MoverParameters->PantRearSP=true;
  }
  //Winger 240404 - wylaczanie sprezarki i przetwornicy przy braku napiecia
  if (tmpTraction.TractionVoltage==0)
  {
   MoverParameters->ConverterFlag=false;
   MoverParameters->CompressorFlag=false; //Ra: to jest w�tpliwe - wy��czenie spr�arki powinno by� w jednym miejscu!
  }
 }
 else if (MoverParameters->EnginePowerSource.SourceType==InternalSource)
  if (MoverParameters->EnginePowerSource.PowerType==SteamPower)
   //if (smPatykird1[0])
  {//Ra: animacja rozrz�du parowozu, na razie nieoptymalizowane
/* //Ra: tymczasowo wy��czone ze wzgl�du na porz�dkowanie animacji pantograf�w
   double fi,dx,c2,ka,kc;
   double sin_fi,cos_fi;
   double L1=1.6688888888888889;
   double L2=5.6666666666666667; //2550/450
   double Lc=0.4;
   double L=5.686422222; //2558.89/450
   double G1,G2,G3,ksi,sin_ksi,gam;
   double G1_2,G2_2,G3_2; //kwadraty
   //ruch t�ok�w oraz korbowod�w
   for (int i=0;i<=1;++i)
   {//obie strony w ten sam spos�b
    fi=DegToRad(dWheelAngle[1]+(i?pant2x:pant1x)); //k�t obrotu ko�a dla t�oka 1
    sin_fi=sin(fi);
    cos_fi=cos(fi);
    dx=panty*cos_fi+sqrt(panth*panth-panty*panty*sin_fi*sin_fi)-panth; //nieoptymalne
    if (smPatykird1[i]) //na razie zabezpieczenie
     smPatykird1[i]->SetTranslate(float3(dx,0,0));
    ka=-asin(panty/panth)*sin_fi;
    if (smPatykirg1[i]) //na razie zabezpieczenie
     smPatykirg1[i]->SetRotateXYZ(vector3(RadToDeg(ka),0,0));
    //smPatykirg1[0]->SetRotate(float3(0,1,0),RadToDeg(fi)); //obracamy
    //ruch dr��ka mimo�rodkowego oraz jarzma
    //korzysta�em z pliku PDF "mm.pdf" (opis czworoboku korbowo-wahaczowego):
    //"MECHANIKA MASZYN. Szkic wyk�adu i laboratorium komputerowego."
    //Prof. dr hab. in�. Jerzy Zaj�czkowski, 2007, Politechnika ��dzka
    //L1 - wysoko�� (w pionie) osi jarzma ponad osi� ko�a
    //L2 - odleg�o�� w poziomie osi jarzma od osi ko�a
    //Lc - d�ugo�� korby mimo�rodu na kole
    //Lr - promie� jarzma =1.0 (pozosta�e przeliczone proporcjonalnie)
    //L - d�ugo�� dr��ka mimo�rodowego
    //fi - k�t obrotu ko�a
    //ksi - k�t obrotu jarzma (od pionu)
    //gam - odchylenie dr��ka mimo�rodowego od poziomu
    //G1=(Lr*Lr+L1*L1+L2*L2+Kc*Lc-L*L-2.0*Lc*L2*cos(fi)+2.0*Lc*L1*sin(fi))/(Lr*Lr);
    //G2=2.0*(L2-Lc*cos(fi))/Lr;
    //G3=2.0*(L1-Lc*sin(fi))/Lr;
    fi=DegToRad(dWheelAngle[1]+(i?pant2x:pant1x)-96.77416667); //k�t obrotu ko�a dla t�oka 1
    //1) dla dWheelAngle[1]=0� korba jest w d�, a mimo�r�d w stron� jarzma, czyli fi=-7�
    //2) dla dWheelAngle[1]=90� korba jest do ty�u, a mimo�r�d w d�, czyli fi=83�
    sin_fi=sin(fi);
    cos_fi=cos(fi);
    G1=(1.0+L1*L1+L2*L2+Lc*Lc-L*L-2.0*Lc*L2*cos_fi+2.0*Lc*L1*sin_fi);
    G1_2=G1*G1;
    G2=2.0*(L2-Lc*cos_fi);
    G2_2=G2*G2;
    G3=2.0*(L1-Lc*sin_fi);
    G3_2=G3*G3;
    sin_ksi=(G1*G2-G3*_fm_sqrt(G2_2+G3_2-G1_2))/(G2_2+G3_2); //x1 (minus delta)
    ksi=asin(sin_ksi); //k�t jarzma
    if (smPatykirg2[i])
     smPatykirg2[i]->SetRotateXYZ(vector3(RadToDeg(ksi),0,0)); //obr�cenie jarzma
    //1) ksi=-23�, gam=
    //2) ksi=10�, gam=
    //gam=acos((L2-sin_ksi-Lc*cos_fi)/L); //k�t od poziomu, liczony wzgl�dem poziomu
    //gam=asin((L1-cos_ksi-Lc*sin_fi)/L); //k�t od poziomu, liczony wzgl�dem pionu
    gam=atan2((L1-cos(ksi)+Lc*sin_fi),(L2-sin_ksi+Lc*cos_fi)); //k�t od poziomu
    if (smPatykird2[i]) //na razie zabezpieczenie
     smPatykird2[i]->SetRotateXYZ(vector3(RadToDeg(-gam-ksi),0,0)); //obr�cenie dr��ka mimo�rodowego
   }
*/
  }

//NBMX Obsluga drzwi, MC: zuniwersalnione
   if ((dDoorMoveL<MoverParameters->DoorMaxShiftL) && (MoverParameters->DoorLeftOpened))
    dDoorMoveL+=dt1*0.5*MoverParameters->DoorOpenSpeed;
   if ((dDoorMoveL>0) && (!MoverParameters->DoorLeftOpened))
   {
    dDoorMoveL-=dt1*MoverParameters->DoorCloseSpeed;
    if (dDoorMoveL<0)
      dDoorMoveL=0;
   }
   if ((dDoorMoveR<MoverParameters->DoorMaxShiftR) && (MoverParameters->DoorRightOpened))
     dDoorMoveR+=dt1*0.5*MoverParameters->DoorOpenSpeed;
   if ((dDoorMoveR>0) && (!MoverParameters->DoorRightOpened))
   {
    dDoorMoveR-=dt1*MoverParameters->DoorCloseSpeed;
    if (dDoorMoveR<0)
     dDoorMoveR=0;
   }


//ABu-160303 sledzenie toru przed obiektem: *******************************
 //Z obserwacji: v>0 -> Coupler 0; v<0 ->coupler1 (Ra: pr�dko�� jest zwi�zana z pojazdem)
 //Rozroznienie jest tutaj, zeby niepotrzebnie nie skakac do funkcji. Nie jest uzaleznione
 //od obecnosci AI, zeby uwzglednic np. jadace bez lokomotywy wagony.
 //Ra: mo�na by przenie�� na poziom obiektu reprezentuj�cego sk�ad, aby nie sprawdza� �rodkowych
 if (CouplCounter>25) //licznik, aby nie robi� za ka�dym razem
 {//poszukiwanie czego� do zderzenia si�
  fTrackBlock=10000.0; //na razie nie ma przeszk�d (na wypadek nie uruchomienia skanowania)
  //je�li nie ma zwrotnicy po drodze, to tylko przeliczy� odleg�o��?
  if (MoverParameters->V>0.03) //[m/s] je�li jedzie do przodu (w kierunku Coupler 0)
  {if (MoverParameters->Couplers[0].CouplingFlag==ctrain_virtual) //brak pojazdu podpi�tego?
   {ABuScanObjects(1,300); //szukanie czego� do pod��czenia
    //WriteLog(asName+" - block 0: "+AnsiString(fTrackBlock));
   }
  }
  else if (MoverParameters->V<-0.03) //[m/s] je�li jedzie do ty�u (w kierunku Coupler 1)
   if (MoverParameters->Couplers[1].CouplingFlag==ctrain_virtual) //brak pojazdu podpi�tego?
   {ABuScanObjects(-1,300);
    //WriteLog(asName+" - block 1: "+AnsiString(fTrackBlock));
   }
  CouplCounter=random(20); //ponowne sprawdzenie po losowym czasie
 }
 if (MoverParameters->Vel>0.1) //[km/h]
  ++CouplCounter; //jazda sprzyja poszukiwaniu po��czenia
 else
 {CouplCounter=25; //a bezruch nie, ale trzeba zaktualizowa� odleg�o��, bo zawalidroga mo�e sobie pojecha�
/*
  //if (Mechanik) //mo�e by� z drugiej strony sk�adu
  {//to poni�ej jest istotne tylko dla AI, czekaj�cego na zwolninie drogi
   if (MoverParameters->Couplers[1-iDirection].CouplingFlag==ctrain_virtual)
   {if ((MoverParameters->CategoryFlag&1)?MoverParameters->Couplers[1-iDirection].Connected!=NULL:false)
    {//je�li jest pojazd kolejowy na sprz�gu wirtualnym - CoupleDist nieadekwatne dla samochod�w!
     fTrackBlock=MoverParameters->Couplers[1-iDirection].CoupleDist; //aktualizacja odleg�o�ci od niego
    }
    else //dla samochod�w pozostaje jedynie skanowanie uruchomi�
     if (fTrackBlock<1000.0) //je�eli pojazdu nie ma, a odleg�o�� jako� ma�a
      ABuScanObjects(iDirection?1:-1,300); //skanowanie sprawdzaj�ce
    //WriteLog(asName+" - block x: "+AnsiString(fTrackBlock));
   }
  }
*/
 }
 if (MoverParameters->DerailReason>0)
 {switch (MoverParameters->DerailReason)
  {case 1: WriteLog(asName+" derailed due to end of track"); break;
   case 2: WriteLog(asName+" derailed due to too high speed"); break;
   case 3: ErrorLog("Bad dynamic: "+asName+" derailed due to track width"); break; //b��d w scenerii
   case 4: ErrorLog("Bad dynamic: "+asName+" derailed due to wrong track type"); break; //b��d w scenerii
  }
  MoverParameters->DerailReason=0; //�eby tylko raz
 }
 if (MoverParameters->LoadStatus)
  LoadUpdate(); //zmiana modelu �adunku
 return true; //Ra: chyba tak?
}

bool __fastcall TDynamicObject::FastUpdate(double dt)
{
    if (dt==0.0) return true; //Ra: pauza
    double dDOMoveLen;
    if (!MoverParameters->PhysicActivation)
     return true;   //McZapkie: wylaczanie fizyki gdy nie potrzeba

    if (!bEnabled)
        return false;

    TLocation l;
    l.X=-vPosition.x;
    l.Y=vPosition.z;
    l.Z=vPosition.y;
    TRotation r;
    r.Rx=r.Ry=r.Rz= 0;

//McZapkie: parametry powinny byc pobierane z toru
    //ts.R=MyTrack->fRadius;
    //ts.Len= Max0R(MoverParameters->BDist,MoverParameters->ADist);
    //ts.dHtrack=Axle1.pPosition.y-Axle0.pPosition.y;
    //ts.dHrail=((Axle1.GetRoll())+(Axle0.GetRoll()))*0.5f;
    //tp.Width=MyTrack->fTrackWidth;
    //McZapkie-250202
    //tp.friction= MyTrack->fFriction;
    //tp.CategoryFlag= MyTrack->iCategoryFlag&15;
    //tp.DamageFlag=MyTrack->iDamageFlag;
    //tp.QualityFlag=MyTrack->iQualityFlag;
    dDOMoveLen=MoverParameters->FastComputeMovement(dt,ts,tp,l,r); // ,ts,tp,tmpTraction);
    MoverParameters->UpdateBatteryVoltage(dt); //jest ju� w Mover.cpp
    if (MoverParameters->EnginePowerSource.SourceType==CurrentCollector) //tylko je�li pantografuj�cy
     if (MoverParameters->Power>1.0) //w rozrz�dczym nie (jest b��d w FIZ!)
      MoverParameters->UpdatePantVolume(dt); //Ra: pneumatyka pantograf�w przeniesiona do Mover.cpp!
    //Move(dDOMoveLen);
    //ResetdMoveLen();
    FastMove(dDOMoveLen);

 if (MoverParameters->LoadStatus)
  LoadUpdate(); //zmiana modelu �adunku
 return true; //Ra: chyba tak?
}

//McZapkie-040402: liczenie pozycji uwzgledniajac wysokosc szyn itp.
//vector3 __fastcall TDynamicObject::GetPosition()
//{//Ra: pozycja pojazdu jest liczona zaraz po przesuni�ciu
// return vPosition;
//};

void __fastcall TDynamicObject::TurnOff()
{//wy��czenie rysowania submodeli zmiennych dla egemplarza pojazdu
 btnOn=false;
 btCoupler1.TurnOff();
 btCoupler2.TurnOff();
 btCPneumatic1.TurnOff();
 btCPneumatic1r.TurnOff();
 btCPneumatic2.TurnOff();
 btCPneumatic2r.TurnOff();
 btPneumatic1.TurnOff();
 btPneumatic1r.TurnOff();
 btPneumatic2.TurnOff();
 btPneumatic2r.TurnOff();
 btCCtrl1.TurnOff();
 btCCtrl2.TurnOff();
 btCPass1.TurnOff();
 btCPass2.TurnOff();
 btEndSignals11.TurnOff();
 btEndSignals13.TurnOff();
 btEndSignals21.TurnOff();
 btEndSignals23.TurnOff();
 btEndSignals1.TurnOff();
 btEndSignals2.TurnOff();
 btEndSignalsTab1.TurnOff();
 btEndSignalsTab2.TurnOff();
 btHeadSignals11.TurnOff();
 btHeadSignals12.TurnOff();
 btHeadSignals13.TurnOff();
 btHeadSignals21.TurnOff();
 btHeadSignals22.TurnOff();
 btHeadSignals23.TurnOff();
};

/*
#include "opengl/glew.h"
#include "opengl/glut.h"


void __fastcall Cone(vector3 p,double d,float fNr)
{//funkcja rysuj�ca sto�ek w miejscu osi
 glPushMatrix(); //matryca kamery
  glTranslatef(p.x,p.y+6,p.z); //6m ponad
  glRotated(RadToDeg(-d),0,1,0); //obr�t wzgl�dem osi OY
  //glRotated(RadToDeg(vAngles.z),0,1,0); //obr�t wzgl�dem osi OY
  glDisable(GL_LIGHTING);
  glColor3f(1.0-fNr,fNr,0); //czerwone dla 0, zielone dla 1
  //glutWireCone(promie� podstawy,wysoko��,k�tno�� podstawy,ilo�� segment�w na wysoko��)
  glutWireCone(0.5,2,4,1); //rysowanie sto�ka (ostros�upa o podstawie wieloboka)
  glEnable(GL_LIGHTING);
 glPopMatrix();
}
*/

void __fastcall TDynamicObject::Render()
{//rysowanie element�w nieprzezroczystych
 //youBy - sprawdzamy, czy jest sens renderowac
 double modelrotate;
 vector3 tempangle;
 // zmienne
 renderme=false;
 //przeklejka
 double ObjSqrDist=SquareMagnitude(Global::pCameraPosition-vPosition);
 //koniec przeklejki
 if (ObjSqrDist<500) //jak jest blisko - do 70m
  modelrotate=0.01f; //ma�y k�t, �eby nie znika�o
 else
 {//Global::pCameraRotation to k�t bewzgl�dny w �wiecie (zero - na p�noc)
  tempangle=(vPosition-Global::pCameraPosition); //wektor od kamery
  modelrotate=ABuAcos(tempangle); //okre�lenie k�ta
  //if (modelrotate>M_PI) modelrotate-=(2*M_PI);
  modelrotate+=Global::pCameraRotation;
 }
 if (modelrotate>M_PI) modelrotate-=(2*M_PI);
 if (modelrotate<-M_PI) modelrotate+=(2*M_PI);
 ModCamRot=modelrotate;

 modelrotate=abs(modelrotate);

 if (modelrotate<maxrot) renderme=true;

 if (renderme)
 {
  TSubModel::iInstance=(int)this; //�eby nie robi� cudzych animacji
  //AnsiString asLoadName="";
  double ObjSqrDist=SquareMagnitude(Global::pCameraPosition-vPosition);
  ABuLittleUpdate(ObjSqrDist); //ustawianie zmiennych submodeli dla wsp�lnego modelu

  //Cone(vCoulpler[0],modelRot.z,0);
  //Cone(vCoulpler[1],modelRot.z,1);

  //ActualTrack= GetTrack(); //McZapkie-240702

#if RENDER_CONE
  {//Ra: testowe renderowanie pozycji w�zk�w w postaci ostros�up�w, wymaga GLUT32.DLL
   double dir=RadToDeg(atan2(vLeft.z,vLeft.x));
   Axle0.Render(0);
   Axle1.Render(1); //bogieRot[0]
   //if (PrevConnected) //renderowanie po��czenia
  }
#endif

  glPushMatrix();
  //vector3 pos= vPosition;
  //double ObjDist= SquareMagnitude(Global::pCameraPosition-pos);
  if (this==Global::pUserDynamic)
  {//specjalne ustawienie, aby nie trz�s�o
   if (Global::bSmudge)
   {//jak smuga, to rysowa� po smudze
    glPopMatrix(); //to trzeba zebra� przed wy�ciem
    return;
   }
   //if (Global::pWorld->) //tu trzeba by ustawi� animacje na modelu zewn�trznym
   glLoadIdentity(); //zacz�� od macierzy jedynkowej
   Global::pCamera->SetCabMatrix(vPosition); //specjalne ustawienie kamery
  }
  else
   glTranslated(vPosition.x,vPosition.y,vPosition.z); //standardowe przesuni�cie wzgl�dem pocz�tku scenerii
  glMultMatrixd(mMatrix.getArray());
  if (fShade>0.0)
  {//Ra: zmiana oswietlenia w tunelu, wykopie
   GLfloat ambientLight[4]= {0.5f,0.5f,0.5f,1.0f};
   GLfloat diffuseLight[4]= {0.5f,0.5f,0.5f,1.0f};
   GLfloat specularLight[4]={0.5f,0.5f,0.5f,1.0f};
   //troch� problem z ambientem w wykopie...
   for (int li=0;li<3;li++)
   {
    ambientLight[li]= Global::ambientDayLight[li]*fShade;
    diffuseLight[li]= Global::diffuseDayLight[li]*fShade;
    specularLight[li]=Global::specularDayLight[li]*fShade;
   }
   glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);
   glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);
   glLightfv(GL_LIGHT0,GL_SPECULAR,specularLight);
  }
  if (Global::bUseVBO)
  {//wersja VBO
   if (mdLowPolyInt)
    if (FreeFlyModeFlag?true:!mdKabina)
     mdLowPolyInt->RaRender(ObjSqrDist,ReplacableSkinID,iAlpha);
   mdModel->RaRender(ObjSqrDist,ReplacableSkinID,iAlpha);
   if (mdLoad) //renderowanie nieprzezroczystego �adunku
    mdLoad->RaRender(ObjSqrDist,ReplacableSkinID,iAlpha);
   if (mdPrzedsionek)
    mdPrzedsionek->RaRender(ObjSqrDist,ReplacableSkinID,iAlpha);
  }
  else
  {//wersja Display Lists
   if (mdLowPolyInt)
    if (FreeFlyModeFlag?true:!mdKabina)
     mdLowPolyInt->Render(ObjSqrDist,ReplacableSkinID,iAlpha);
   mdModel->Render(ObjSqrDist,ReplacableSkinID,iAlpha);
   if (mdLoad) //renderowanie nieprzezroczystego �adunku
    mdLoad->Render(ObjSqrDist,ReplacableSkinID,iAlpha);
   if (mdPrzedsionek)
    mdPrzedsionek->Render(ObjSqrDist,ReplacableSkinID,iAlpha);
  }

  //Ra: czy ta kabina tu ma sens?
  //Ra: czy nie renderuje si� dwukrotnie?
  //Ra: dlaczego jest zablokowana w przezroczystych?
  if (mdKabina) //je�li ma model kabiny
  if ((mdKabina!=mdModel) && bDisplayCab && FreeFlyModeFlag)
  {//rendering kabiny gdy jest oddzielnym modelem i ma byc wyswietlana
   //ABu: tylko w trybie FreeFly, zwykly tryb w world.cpp
   //Ra: �wiet�a s� ustawione dla zewn�trza danego pojazdu
   //oswietlenie kabiny
   GLfloat  ambientCabLight[4]= { 0.5f,  0.5f, 0.5f, 1.0f };
   GLfloat  diffuseCabLight[4]= { 0.5f,  0.5f, 0.5f, 1.0f };
   GLfloat  specularCabLight[4]= { 0.5f,  0.5f, 0.5f, 1.0f };
   for (int li=0; li<3; li++)
   {
    ambientCabLight[li]= Global::ambientDayLight[li]*0.9;
    diffuseCabLight[li]= Global::diffuseDayLight[li]*0.5;
    specularCabLight[li]=Global::specularDayLight[li]*0.5;
   }
   switch (MyTrack->eEnvironment)
   {
    case e_canyon:
     {
       for (int li=0; li<3; li++)
        {
          diffuseCabLight[li]*= 0.6;
          specularCabLight[li]*= 0.7;
        }
     }
    break;
    case e_tunnel:
     {
       for (int li=0; li<3; li++)
        {
          ambientCabLight[li]*= 0.3;
          diffuseCabLight[li]*= 0.1;
          specularCabLight[li]*= 0.2;
        }
     }
    break;
   }
   glLightfv(GL_LIGHT0,GL_AMBIENT,ambientCabLight);
   glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseCabLight);
   glLightfv(GL_LIGHT0,GL_SPECULAR,specularCabLight);
   if (Global::bUseVBO)
    mdKabina->RaRender(ObjSqrDist,0);
   else
    mdKabina->Render(ObjSqrDist,0);
   glLightfv(GL_LIGHT0,GL_AMBIENT,Global::ambientDayLight);
   glLightfv(GL_LIGHT0,GL_DIFFUSE,Global::diffuseDayLight);
   glLightfv(GL_LIGHT0,GL_SPECULAR,Global::specularDayLight);
  }
  if (fShade!=0.0) //tylko je�li by�o zmieniane
  {//przywr�cenie standardowego o�wietlenia
   glLightfv(GL_LIGHT0,GL_AMBIENT,Global::ambientDayLight);
   glLightfv(GL_LIGHT0,GL_DIFFUSE,Global::diffuseDayLight);
   glLightfv(GL_LIGHT0,GL_SPECULAR,Global::specularDayLight);
  }
  glPopMatrix();
  if (btnOn) TurnOff(); //przywr�cenie domy�lnych pozycji submodeli
 } //yB - koniec mieszania z grafika
};

void __fastcall TDynamicObject::RenderSounds()
{//przeliczanie d�wi�k�w, bo b�dzie s�ycha� bez wy�wietlania sektora z pojazdem
 //McZapkie-010302: ulepszony dzwiek silnika
 double freq;
 double vol=0;
 double dt=Timer::GetDeltaTime();

//    double sounddist;
//    sounddist=SquareMagnitude(Global::pCameraPosition-vPosition);

    if (MoverParameters->Power>0)
     {
      if ((rsSilnik.AM!=0) && ((MoverParameters->Mains) || (MoverParameters->EngineType==DieselEngine)))   //McZapkie-280503: zeby dla dumb dzialal silnik na jalowych obrotach
       {
         if ((fabs(MoverParameters->enrot)>0.01)  || (MoverParameters->EngineType==Dumb)) //&& (MoverParameters->EnginePower>0.1))
          {
            freq=rsSilnik.FM*fabs(MoverParameters->enrot)+rsSilnik.FA;
            if (MoverParameters->EngineType==Dumb)
             freq=freq-0.2*MoverParameters->EnginePower/(1+MoverParameters->Power*1000);
            rsSilnik.AdjFreq(freq,dt);
            if (MoverParameters->EngineType==DieselEngine)
             {
               if (MoverParameters->enrot>0)
                {
                 if (MoverParameters->EnginePower>0)
                   vol=rsSilnik.AM*MoverParameters->dizel_fill+rsSilnik.AA;
                 else
                  vol=rsSilnik.AM*fabs(MoverParameters->enrot/MoverParameters->nmax)+rsSilnik.AA*0.9;
                }
               else
                vol=0;
             }
            else if (MoverParameters->EngineType==DieselElectric)
             vol=rsSilnik.AM*(MoverParameters->EnginePower/1000/MoverParameters->Power)+0.2*(MoverParameters->enrot*60)/(MoverParameters->DElist[MoverParameters->MainCtrlPosNo].RPM)+rsSilnik.AA;
            else
             vol=rsSilnik.AM*(MoverParameters->EnginePower/1000+fabs(MoverParameters->enrot)*60.0)+rsSilnik.AA;
//            McZapkie-250302 - natezenie zalezne od obrotow i mocy
            if ((vol<1) && (MoverParameters->EngineType==ElectricSeriesMotor) && (MoverParameters->EnginePower<100))
             {
              float volrnd=random(100)*MoverParameters->enrot/(1+MoverParameters->nmax);
               if (volrnd<2)
                 vol=vol+volrnd/200.0;
             }
            switch (MyTrack->eEnvironment)
            {
                case e_tunnel:
                 {
                  vol+=0.1;
                 }
                break;
                case e_canyon:
                 {
                  vol+=0.05;
                 }
                break;
            }
            if ((MoverParameters->DynamicBrakeFlag) && (MoverParameters->EnginePower>0.1)) //Szociu - 29012012 - je�eli uruchomiony jest  hamulec elektrodynamiczny, odtwarzany jest d�wi�k silnika
             vol +=0.8;

            if (enginevolume>0.0001)
              if (MoverParameters->EngineType!=DieselElectric)
               { rsSilnik.Play(enginevolume,DSBPLAY_LOOPING,MechInside,GetPosition()); }
              else
               {
                sConverter.UpdateAF(vol,freq,MechInside,GetPosition());

                float fincvol;
                fincvol=0;
                if ((MoverParameters->ConverterFlag)&&(MoverParameters->enrot*60>MoverParameters->DElist[0].RPM))
                 {
                  fincvol=(MoverParameters->DElist[MoverParameters->MainCtrlPos].RPM-(MoverParameters->enrot*60));
                  fincvol/=(0.05*MoverParameters->DElist[0].RPM);
                 };
                if (fincvol>0.02)
                  rsDiesielInc.Play(fincvol,DSBPLAY_LOOPING,MechInside,GetPosition());
                else
                  rsDiesielInc.Stop();
               }
          }
          else
           rsSilnik.Stop();
       }
       enginevolume=(enginevolume+vol)/2;
       if (enginevolume<0.01)
         rsSilnik.Stop();
       if (MoverParameters->EngineType==ElectricSeriesMotor && rsWentylator.AM!=0)
        {
          if (MoverParameters->RventRot>0.1)
           {
            freq=rsWentylator.FM*MoverParameters->RventRot+rsWentylator.FA;
            rsWentylator.AdjFreq(freq,dt);
            vol=rsWentylator.AM*MoverParameters->RventRot+rsWentylator.AA;
            rsWentylator.Play(vol,DSBPLAY_LOOPING,MechInside,GetPosition());
           }
          else
           rsWentylator.Stop();
        }
        if (MoverParameters->TrainType==dt_ET40)
        {
          if (MoverParameters->Vel>0.1)
           {
            freq=rsPrzekladnia.FM*(MoverParameters->Vel)+rsPrzekladnia.FA;
            rsPrzekladnia.AdjFreq(freq,dt);
            vol=rsPrzekladnia.AM*(MoverParameters->Vel)+rsPrzekladnia.AA;
            rsPrzekladnia.Play(vol,DSBPLAY_LOOPING,MechInside,GetPosition());
           }
          else
           rsPrzekladnia.Stop();
        }
     }

//youBy: dzwiek ostrych lukow i ciasnych zwrotek

     if ((ts.R*ts.R>1)&&(MoverParameters->Vel>0))
       vol=MoverParameters->AccN*MoverParameters->AccN;
     else
       vol=0;
//       vol+=(50000/ts.R*ts.R);

     if (vol>0.001)
      {
       rscurve.Play(2*vol,DSBPLAY_LOOPING,MechInside,GetPosition());
      }
     else
       rscurve.Stop();

//McZapkie-280302 - pisk mocno zacisnietych hamulcow - trzeba jeszcze zabezpieczyc przed brakiem deklaracji w mmedia.dta
    if (rsPisk.AM!=0)
     {
      if ((MoverParameters->Vel>(rsPisk.GetStatus()!=0?0.01:0.5)) && (!MoverParameters->SlippingWheels) && (MoverParameters->UnitBrakeForce>rsPisk.AM))
       {
        vol=MoverParameters->UnitBrakeForce/(rsPisk.AM+1)+rsPisk.AA;
        rsPisk.Play(vol,DSBPLAY_LOOPING,MechInside,GetPosition());
       }
      else
       rsPisk.Stop();
     }

//if ((MoverParameters->ConverterFlag==false) && (MoverParameters->TrainType!=dt_ET22))
//if ((MoverParameters->ConverterFlag==false)&&(MoverParameters->CompressorPower!=0))
// MoverParameters->CompressorFlag=false; //Ra: wywali� to st�d, tu tylko dla wy�wietlanych!
//Ra: no to ju� wiemy, dlaczego poci�gi je�d�� lepiej, gdy si� na nie patrzy!
//if (MoverParameters->CompressorPower==2)
// MoverParameters->CompressorAllow=MoverParameters->ConverterFlag;

// McZapkie! - dzwiek compressor.wav tylko gdy dziala sprezarka
    if (MoverParameters->VeselVolume!=0)
    {
      if (MoverParameters->CompressorFlag)
        sCompressor.TurnOn(MechInside,GetPosition());
      else
        sCompressor.TurnOff(MechInside,GetPosition());
      sCompressor.Update(MechInside,GetPosition());
    }
    if (MoverParameters->PantCompFlag)                // Winger 160404 - dzwiek malej sprezarki
        sSmallCompressor.TurnOn(MechInside,GetPosition());
     else
        sSmallCompressor.TurnOff(MechInside,GetPosition());
     sSmallCompressor.Update(MechInside,GetPosition());

//youBy - przenioslem, bo diesel tez moze miec turbo
if ((MoverParameters->MainCtrlPos)>=(MoverParameters->TurboTest))  //hunter-250312: dlaczego zakomentowane? Ra: bo nie dzia�a�o dobrze
{
          //udawanie turbo:  (6.66*(eng_vol-0.85))
    if (eng_turbo>6.66*(enginevolume-0.8)+0.2*dt)
         eng_turbo=eng_turbo-0.2*dt; //0.125
    else
    if (eng_turbo<6.66*(enginevolume-0.8)-0.4*dt)
         eng_turbo=eng_turbo+0.4*dt;  //0.333
    else
         eng_turbo=6.66*(enginevolume-0.8);

    sTurbo.TurnOn(MechInside,GetPosition());
    //sTurbo.UpdateAF(eng_turbo,0.7+(eng_turbo*0.6),MechInside,GetPosition());
    sTurbo.UpdateAF(3*eng_turbo-1,0.4+eng_turbo*0.4,MechInside,GetPosition());
//    eng_vol_act=enginevolume;
    //eng_frq_act=eng_frq;
}
else sTurbo.TurnOff(MechInside,GetPosition());



 if (MoverParameters->TrainType==dt_PseudoDiesel)
 {
    //ABu: udawanie woodwarda dla lok. spalinowych
    //jesli silnik jest podpiety pod dzwiek przetwornicy
    if (MoverParameters->ConverterFlag)                 //NBMX dzwiek przetwornicy
    {
       sConverter.TurnOn(MechInside,GetPosition());
    }
    else
       sConverter.TurnOff(MechInside,GetPosition());

    //glosnosc zalezy od stosunku mocy silnika el. do mocy max
       double eng_vol;
       if (MoverParameters->Power>1)
          //0.85+0.000015*(...)
          eng_vol=0.8+0.00002*(MoverParameters->EnginePower/MoverParameters->Power);
       else
          eng_vol=1;

       eng_dfrq=eng_dfrq+(eng_vol_act-eng_vol);
       if(eng_dfrq>0)
       {
          eng_dfrq=eng_dfrq-0.025*dt;
          if(eng_dfrq<0.025*dt)
             eng_dfrq=0;
       }
       else
       if(eng_dfrq<0)
       {
          eng_dfrq=eng_dfrq+0.025*dt;
          if(eng_dfrq>-0.025*dt)
             eng_dfrq=0;
       }
       double defrot;
       if (MoverParameters->MainCtrlPos!=0)
       {
          double CtrlPos=MoverParameters->MainCtrlPos;
          double CtrlPosNo=MoverParameters->MainCtrlPosNo;
          //defrot=1+0.4*(CtrlPos/CtrlPosNo);
          defrot=1+0.5*(CtrlPos/CtrlPosNo);
       }
       else
          defrot=1;

       if (eng_frq_act<defrot)
       {
          //if (MoverParameters->MainCtrlPos==1) eng_frq_act=eng_frq_act+0.1*dt;
          eng_frq_act=eng_frq_act+0.4*dt; //0.05
          if (eng_frq_act>defrot-0.4*dt)
            eng_frq_act=defrot;
       }
       else
       if (eng_frq_act>defrot)
       {
          eng_frq_act=eng_frq_act-0.1*dt; //0.05
          if (eng_frq_act<defrot+0.1*dt)
             eng_frq_act=defrot;
       }
       sConverter.UpdateAF(eng_vol_act,eng_frq_act+eng_dfrq,MechInside,GetPosition());
       //udawanie turbo:  (6.66*(eng_vol-0.85))
       if (eng_turbo>6.66*(eng_vol-0.8)+0.2*dt)
          eng_turbo=eng_turbo-0.2*dt; //0.125
       else
       if (eng_turbo<6.66*(eng_vol-0.8)-0.4*dt)
          eng_turbo=eng_turbo+0.4*dt;  //0.333
       else
          eng_turbo=6.66*(eng_vol-0.8);

       sTurbo.TurnOn(MechInside,GetPosition());
       //sTurbo.UpdateAF(eng_turbo,0.7+(eng_turbo*0.6),MechInside,GetPosition());
       sTurbo.UpdateAF(3*eng_turbo-1,0.4+eng_turbo*0.4,MechInside,GetPosition());
       eng_vol_act=eng_vol;
       //eng_frq_act=eng_frq;
 }
 else
 {
  if (MoverParameters->ConverterFlag)                 //NBMX dzwiek przetwornicy
   sConverter.TurnOn(MechInside,GetPosition());
  else
   sConverter.TurnOff(MechInside,GetPosition());
  sConverter.Update(MechInside,GetPosition());
 }
 if (MoverParameters->WarningSignal>0)
 {
  if (TestFlag(MoverParameters->WarningSignal,1))
   sHorn1.TurnOn(MechInside,GetPosition());
  else
   sHorn1.TurnOff(MechInside,GetPosition());
  if (TestFlag(MoverParameters->WarningSignal,2))
   sHorn2.TurnOn(MechInside,GetPosition());
  else
   sHorn2.TurnOff(MechInside,GetPosition());
 }
 else
 {
  sHorn1.TurnOff(MechInside,GetPosition());
  sHorn2.TurnOff(MechInside,GetPosition());
 }
 if (MoverParameters->DoorClosureWarning)
 {
  if (MoverParameters->DepartureSignal) //NBMX sygnal odjazdu, MC: pod warunkiem ze jest zdefiniowane w chk
   sDepartureSignal.TurnOn(MechInside,GetPosition());
  else
   sDepartureSignal.TurnOff(MechInside,GetPosition());
  sDepartureSignal.Update(MechInside,GetPosition());
 }
 sHorn1.Update(MechInside,GetPosition());
 sHorn2.Update(MechInside,GetPosition());
 //McZapkie: w razie wykolejenia
 if (MoverParameters->EventFlag)
 {
  if (TestFlag(MoverParameters->DamageFlag,dtrain_out) && GetVelocity()>0)
    rsDerailment.Play(1,0,true,GetPosition());
  if (GetVelocity()==0)
    rsDerailment.Stop();
 }
/* //Ra: dwa razy?
 if (MoverParameters->EventFlag)
 {
  if (TestFlag(MoverParameters->DamageFlag,dtrain_out) && GetVelocity()>0)
    rsDerailment.Play(1,0,true,GetPosition());
  if (GetVelocity()==0)
    rsDerailment.Stop();
 }
*/
};

void __fastcall TDynamicObject::RenderAlpha()
{//rysowanie element�w p�przezroczystych
 if (renderme)
 {
  TSubModel::iInstance=(int)this; //�eby nie robi� cudzych animacji
  double ObjSqrDist=SquareMagnitude(Global::pCameraPosition-vPosition);
  ABuLittleUpdate(ObjSqrDist); //ustawianie zmiennych submodeli dla wsp�lnego modelu
  glPushMatrix();
  if (this==Global::pUserDynamic)
  {//specjalne ustawienie, aby nie trz�s�o
   if (Global::bSmudge)
   {//jak smuga, to rysowa� po smudze
    glPopMatrix(); //to trzeba zebra� przed wy�ciem
    return;
   }
   glLoadIdentity(); //zacz�� od macierzy jedynkowej
   Global::pCamera->SetCabMatrix(vPosition); //specjalne ustawienie kamery
  }
  else
   glTranslated(vPosition.x,vPosition.y,vPosition.z); //standardowe przesuni�cie wzgl�dem pocz�tku scenerii
  glMultMatrixd(mMatrix.getArray());
  if (fShade>0.0)
  {//Ra: zmiana oswietlenia w tunelu, wykopie
   GLfloat ambientLight[4]= {0.5f,0.5f,0.5f,1.0f};
   GLfloat diffuseLight[4]= {0.5f,0.5f,0.5f,1.0f};
   GLfloat specularLight[4]={0.5f,0.5f,0.5f,1.0f};
   //troch� problem z ambientem w wykopie...
   for (int li=0;li<3;li++)
   {
    ambientLight[li]= Global::ambientDayLight[li]*fShade;
    diffuseLight[li]= Global::diffuseDayLight[li]*fShade;
    specularLight[li]=Global::specularDayLight[li]*fShade;
   }
   glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);
   glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);
   glLightfv(GL_LIGHT0,GL_SPECULAR,specularLight);
  }
  if (Global::bUseVBO)
  {//wersja VBO
   if (mdLowPolyInt)
    if (FreeFlyModeFlag?true:!mdKabina)
     mdLowPolyInt->RaRenderAlpha(ObjSqrDist,ReplacableSkinID,iAlpha);
   mdModel->RaRenderAlpha(ObjSqrDist,ReplacableSkinID,iAlpha);
   if (mdLoad)
    mdLoad->RaRenderAlpha(ObjSqrDist,ReplacableSkinID,iAlpha);
   //if (mdPrzedsionek) //Ra: przedsionk�w tu wcze�niej nie by�o - w��czy�?
   // mdPrzedsionek->RaRenderAlpha(ObjSqrDist,ReplacableSkinID,iAlpha);
  }
  else
  {//wersja Display Lists
   if (mdLowPolyInt)
    if (FreeFlyModeFlag?true:!mdKabina)
     mdLowPolyInt->RenderAlpha(ObjSqrDist,ReplacableSkinID,iAlpha);
   mdModel->RenderAlpha(ObjSqrDist,ReplacableSkinID,iAlpha);
   if (mdLoad)
    mdLoad->RenderAlpha(ObjSqrDist,ReplacableSkinID,iAlpha);
   //if (mdPrzedsionek) //Ra: przedsionk�w tu wcze�niej nie by�o - w��czy�?
   // mdPrzedsionek->RenderAlpha(ObjSqrDist,ReplacableSkinID,iAlpha);
  }
/* skoro false to mo�na wyci�c
    //ABu: Tylko w trybie freefly
    if (false)//((mdKabina!=mdModel) && bDisplayCab && FreeFlyModeFlag)
    {
//oswietlenie kabiny
      GLfloat  ambientCabLight[4]= { 0.5f,  0.5f, 0.5f, 1.0f };
      GLfloat  diffuseCabLight[4]= { 0.5f,  0.5f, 0.5f, 1.0f };
      GLfloat  specularCabLight[4]= { 0.5f,  0.5f, 0.5f, 1.0f };
      for (int li=0; li<3; li++)
       {
         ambientCabLight[li]= Global::ambientDayLight[li]*0.9;
         diffuseCabLight[li]= Global::diffuseDayLight[li]*0.5;
         specularCabLight[li]= Global::specularDayLight[li]*0.5;
       }
      switch (MyTrack->eEnvironment)
      {
       case e_canyon:
        {
          for (int li=0; li<3; li++)
           {
             diffuseCabLight[li]*= 0.6;
             specularCabLight[li]*= 0.8;
           }
        }
       break;
       case e_tunnel:
        {
          for (int li=0; li<3; li++)
           {
             ambientCabLight[li]*= 0.3;
             diffuseCabLight[li]*= 0.1;
             specularCabLight[li]*= 0.2;
           }
        }
       break;
      }
// dorobic swiatlo od drugiej strony szyby

      glLightfv(GL_LIGHT0,GL_AMBIENT,ambientCabLight);
      glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseCabLight);
      glLightfv(GL_LIGHT0,GL_SPECULAR,specularCabLight);

      mdKabina->RenderAlpha(ObjSqrDist,0);
//smierdzi
//      mdModel->RenderAlpha(SquareMagnitude(Global::pCameraPosition-pos),0);

      glLightfv(GL_LIGHT0,GL_AMBIENT,Global::ambientDayLight);
      glLightfv(GL_LIGHT0,GL_DIFFUSE,Global::diffuseDayLight);
      glLightfv(GL_LIGHT0,GL_SPECULAR,Global::specularDayLight);
    }
*/
  if (fShade!=0.0) //tylko je�li by�o zmieniane
  {//przywr�cenie standardowego o�wietlenia
   glLightfv(GL_LIGHT0,GL_AMBIENT,Global::ambientDayLight);
   glLightfv(GL_LIGHT0,GL_DIFFUSE,Global::diffuseDayLight);
   glLightfv(GL_LIGHT0,GL_SPECULAR,Global::specularDayLight);
  }
  glPopMatrix();
  if (btnOn) TurnOff(); //przywr�cenie domy�lnych pozycji submodeli
 }
 return;
} //koniec renderalpha


//McZapkie-250202
//wczytywanie pliku z danymi multimedialnymi (dzwieki)
void __fastcall TDynamicObject::LoadMMediaFile(AnsiString BaseDir,AnsiString TypeName,AnsiString ReplacableSkin)
{
 double dSDist;
 TFileStream *fs;
 Global::asCurrentDynamicPath=BaseDir;
 AnsiString asFileName=BaseDir+TypeName+".mmd";
 AnsiString asLoadName=BaseDir+MoverParameters->LoadType+".t3d";
 if (!FileExists(asFileName))
 {
  ErrorLog("Missed file: "+asFileName); //brak MMD
  return;
 }
 fs=new TFileStream(asFileName,fmOpenRead|fmShareCompat);
 if (!fs) return;
 int size=fs->Size;
 if (!size) {return delete fs;};
 AnsiString asAnimName="";
 bool Stop_InternalData=false;
 char* buf=new char[size+1]; //ci�g bajt�w o d�ugo�ci r�wnej rozmiwarowi pliku
 buf[size]='\0'; //zako�czony zerem na wszelki wypadek
 fs->Read(buf,size);
 delete fs;
 TQueryParserComp *Parser;
 Parser=new TQueryParserComp(NULL);
 Parser->TextToParse=AnsiString(buf);
 delete[] buf;
 AnsiString str;
 //Parser->LoadStringToParse(asFile);
 Parser->First();
 //DecimalSeparator= '.';
 pants=NULL; //wska�nik pierwszego obiektu animuj�cego dla pantograf�w
 while (!Parser->EndOfFile && !Stop_InternalData)
 {
     str=Parser->GetNextSymbol().LowerCase();
     if (str==AnsiString("models:"))                            //modele i podmodele
     {
       asModel=Parser->GetNextSymbol().LowerCase();
       asModel=BaseDir+asModel; //McZapkie-200702 - dynamics maja swoje modele w dynamics/basedir
       Global::asCurrentTexturePath=BaseDir;                    //biezaca sciezka do tekstur to dynamic/...
       mdModel=TModelsManager::GetModel(asModel.c_str(),true);
       if (ReplacableSkin!=AnsiString("none"))
       {//tekstura wymienna jest raczej jedynie w "dynamic\" 
        ReplacableSkin=Global::asCurrentTexturePath+ReplacableSkin;      //skory tez z dynamic/...
        ReplacableSkinID[1]=TTexturesManager::GetTextureID(NULL,NULL,ReplacableSkin.c_str(),Global::iDynamicFiltering);
        if (TTexturesManager::GetAlpha(ReplacableSkinID[1]))
         iAlpha=0x31310031; //tekstura -1 z kana�em alfa - nie renderowa� w cyklu nieprzezroczystych
        else
         iAlpha=0x30300030; //wszystkie tekstury nieprzezroczyste - nie renderowa� w cyklu przezroczystych
       }
  //Winger 040304 - ladowanie przedsionkow dla EZT
       if (MoverParameters->TrainType==dt_EZT)
       {
        asModel="przedsionki.t3d";
        asModel=BaseDir+asModel;
        mdPrzedsionek=TModelsManager::GetModel(asModel.c_str(),true);
       }
       if (!MoverParameters->LoadAccepted.IsEmpty())
       //if (MoverParameters->LoadAccepted!=AnsiString("")); // && MoverParameters->LoadType!=AnsiString("passengers"))
        if (MoverParameters->EnginePowerSource.SourceType==CurrentCollector)
        {//warto�� niby "pantstate" - nazwa dla formalno�ci, wa�na jest ilo��
         if (MoverParameters->Load==1)
          MoverParameters->PantFront(true);
         else if (MoverParameters->Load==2)
          MoverParameters->PantRear(true);
         else if (MoverParameters->Load==3)
         {
          MoverParameters->PantFront(true);
          MoverParameters->PantRear(true);
         }
         else if (MoverParameters->Load==4)
          MoverParameters->DoubleTr=-1;
         else if (MoverParameters->Load==5)
         {
          MoverParameters->DoubleTr=-1;
          MoverParameters->PantRear(true);
         }
         else if (MoverParameters->Load==6)
         {
          MoverParameters->DoubleTr=-1;
          MoverParameters->PantFront(true);
         }
         else if (MoverParameters->Load==7)
         {
          MoverParameters->DoubleTr=-1;
          MoverParameters->PantFront(true);
          MoverParameters->PantRear(true);
         }
        }
        else //Ra: tu wczytywanie modelu �adunku jest w porz�dku
         mdLoad=TModelsManager::GetModel(asLoadName.c_str(),true);  //ladunek
       Global::asCurrentTexturePath=AnsiString(szTexturePath); //z powrotem defaultowa sciezka do tekstur
       while (!Parser->EndOfFile && str!=AnsiString("endmodels"))
       {
        str=Parser->GetNextSymbol().LowerCase();
        if (str==AnsiString("animations:"))
        {//Ra: ustawienie ilo�ci poszczeg�lnych animacji - musi by� jako pierwsze, inaczej ilo�ci b�d� domy�lne
         if (!pAnimations)
         {//je�li nie ma jeszcze tabeli animacji, mo�na odczyta� nowe ilo�ci
          int co=0,ile;
          iAnimations=0;
          do
          {//kolejne liczby to ilo�� animacj, -1 to znacznik ko�ca
           ile=Parser->GetNextSymbol().ToIntDef(-1); //ilo�� danego typu animacji
           //if (co==ANIM_PANTS)
           // if (!Global::bLoadTraction)
           //  if (!DebugModeFlag) //w debugmode pantografy maj� "niby dzia�a�"
           //   ile=0; //wy��czenie animacji pantograf�w
           if (co<ANIM_TYPES)
            if (ile>=0)
            {iAnimType[co]=ile; //zapami�tanie
             iAnimations+=ile; //og�lna ilo�� animacji
            }
           ++co;
          } while (ile>=0); //-1 to znacznik ko�ca
          while (co<ANIM_TYPES) iAnimType[co++]=0; //zerowanie pozosta�ych
          str=Parser->GetNextSymbol().LowerCase();
         }
         //WriteLog("Total animations: "+AnsiString(iAnimations));
        }
        if (!pAnimations)
        {//Ra: tworzenie tabeli animacji, je�li jeszcze nie by�o
         if (!iAnimations) iAnimations=28; //tyle by�o kiedy� w ka�dym poje�dzie (2 wi�zary wypad�y)
         pAnimations=new TAnim[iAnimations];
         int i,j,k=0,sm=0;
         for (j=0;j<ANIM_TYPES;++j)
          for (i=0;i<iAnimType[j];++i)
          {
           if (j==ANIM_PANTS) //zliczamy poprzednie animacje
            if (!pants)
             if (iAnimType[ANIM_PANTS]) //o ile jakie� pantografy s� (a domy�lnie s�)
              pants=pAnimations+k; //zapami�tanie na potrzeby wyszukania submodeli
           pAnimations[k].iShift=sm; //przesuni�cie do przydzielenia wska�nika
           sm+=pAnimations[k++].TypeSet(j); //ustawienie typu animacji i zliczanie submodeli
          }
         if (sm) //o ile s� bardziej z�o�one animacje
         {pAnimated=new TSubModel*[sm]; //tabela na animowane submodele
          for (k=0;k<iAnimations;++k)
           pAnimations[k].smElement=pAnimated+pAnimations[k].iShift; //przydzielenie wska�nika do tabelki
         }
        }
        if (str==AnsiString("lowpolyinterior:")) //ABu: wnetrze lowpoly
        {
         asModel=Parser->GetNextSymbol().LowerCase();
         asModel=BaseDir+asModel; //McZapkie-200702 - dynamics maja swoje modele w dynamic/basedir
         Global::asCurrentTexturePath=BaseDir; //biezaca sciezka do tekstur to dynamic/...
         mdLowPolyInt=TModelsManager::GetModel(asModel.c_str(),true);
         //Global::asCurrentTexturePath=AnsiString(szTexturePath); //kiedy� uproszczone wn�trze miesza�o tekstury nieba
        }
        else if (str==AnsiString("animwheelprefix:"))
        {//prefiks kr�c�cych si� k�
         int i,j,k,m;
         str=Parser->GetNextSymbol();
         for (i=0;i<iAnimType[ANIM_WHEELS];++i) //liczba osi
         {//McZapkie-050402: wyszukiwanie kol o nazwie str*
          asAnimName=str+AnsiString(i+1);
          pAnimations[i].smAnimated=mdModel->GetFromName(asAnimName.c_str()); //ustalenie submodelu
          if (pAnimations[i].smAnimated)
          {//++iAnimatedAxles;
           pAnimations[i].smAnimated->WillBeAnimated(); //wy��czenie optymalizacji transformu
           pAnimations[i].yUpdate=UpdateAxle; //animacja osi
           pAnimations[i].fMaxDist=50*50; //nie kr�ci� w wi�kszej odleg�o�ci
          }
         }
         //Ra: ustawianie indeks�w osi
         for (i=0;i<iAnimType[ANIM_WHEELS];++i) //ilo�� osi (zabezpieczenie przed b��dami w CHK)
          pAnimations[i].dWheelAngle=dWheelAngle+1; //domy�lnie wska�nik na nap�dzaj�ce
         i=0; j=1; k=0; m=0; //numer osi; kolejny znak; ile osi danego typu; kt�ra �rednica
         if ((MoverParameters->WheelDiameterL!=MoverParameters->WheelDiameter)||(MoverParameters->WheelDiameterT!=MoverParameters->WheelDiameter))
         {//obs�uga r�nych �rednic, o ile wyst�puj�
          while ((i<iAnimType[ANIM_WHEELS])&&(j<=MoverParameters->AxleArangement.Length()))
          {//wersja ze wska�nikami jest bardziej elastyczna na nietypowe uk�ady
           if ((k>='A')&&(k<='J')) //10 chyba maksimum?
           {pAnimations[i++].dWheelAngle=dWheelAngle+1; //obr�t osi nap�dzaj�cych
            --k; //nast�pna b�dzie albo taka sama, albo bierzemy kolejny znak
            m=2; //nast�puj�ce toczne b�d� mia�y inn� �rednic�
           }
           else if ((k>='1')&&(k<='9'))
           {pAnimations[i++].dWheelAngle=dWheelAngle+m; //obr�t osi tocznych
            --k; //nast�pna b�dzie albo taka sama, albo bierzemy kolejny znak
           }
           else
            k=MoverParameters->AxleArangement[j++]; //pobranie kolejnego znaku
          }
         }
        }
        //else if (str==AnsiString("animrodprefix:")) //prefiks wiazarow dwoch
        // {
        //  str= Parser->GetNextSymbol();
        //  for (int i=1; i<=2; i++)
        //  {//McZapkie-050402: wyszukiwanie max 2 wiazarow o nazwie str*
        //   asAnimName=str+i;
        //   smWiazary[i-1]=mdModel->GetFromName(asAnimName.c_str());
        //   smWiazary[i-1]->WillBeAnimated();
        //  }
        // }
        else if (str==AnsiString("animpantprefix:"))
        {//Ra: pantografy po nowemu maj� literki i numerki
        }
//Pantografy - Winger 160204
        if (str==AnsiString("animpantrd1prefix:"))
        {//prefiks ramion dolnych 1
         str=Parser->GetNextSymbol();
         float4x4 m; //macierz do wyliczenia pozycji i wektora ruchu pantografu
         TSubModel *sm;
         if (pants)
          for (int i=0;i<iAnimType[ANIM_PANTS];i++)
          {//Winger 160204: wyszukiwanie max 2 patykow o nazwie str*
           asAnimName=str+AnsiString(i+1);
           sm=mdModel->GetFromName(asAnimName.c_str());
           pants[i].smElement[0]=sm; //jak NULL, to nie b�dzie animowany
           if (sm)
           {//w EP09 wywala�o si� tu z powodu NULL
            sm->WillBeAnimated();
            //sm->ParentMatrix(m); //pobranie macierzy transformacji
            m=float4x4(*sm->GetMatrix()); //skopiowanie, bo b�dziemy mno�y�
            //m(3)[1]=m[3][1]+0.054; //w g�r� o wysoko�� �lizgu (na razie tak)
            while (sm->Parent)
            {//przenie�� t� funkcj� do modelu...
             if (sm->Parent->GetMatrix())
              m=*sm->Parent->GetMatrix()*m;
             sm=sm->Parent;
            }
            if ((mdModel->Flags()&0x8000)==0) //je�li wczytano z T3D
            {//mo�e by� potrzebny dodatkowy obr�t, je�li wczytano z T3D, tzn. pred wykonaniem Init()
             m.InitialRotate();
            }
            pants[i].fParamPants->vPos.z=m[3][0]; //przesuni�cie w bok (asymetria)
            pants[i].fParamPants->vPos.y=m[3][1]; //przesuni�cie w g�r� odczytane z modelu
            if ((sm=pants[i].smElement[0]->ChildGet())!=NULL)
            {//je�li ma potomny, mo�na policzy� d�ugo�� (odleg�o�� potomnego od osi obrotu)
             m=float4x4(*sm->GetMatrix()); //wystarczy�by wska�nik, nie trzeba kopiowa�
             //mo�e trzeba: pobra� macierz dolnego ramienia, wyzerowa� przesuni�cie, przemno�y� przez macierz g�rnego
             pants[i].fParamPants->fHoriz=-fabs(m(3)[1]);
             pants[i].fParamPants->fLenL1=hypot(m(3)[1],m(3)[2]); //po osi OX nie potrzeba
             pants[i].fParamPants->fAngleL0=atan2(fabs(m(3)[2]),fabs(m(3)[1]));
             //if (pants[i].fParamPants->fAngleL0<M_PI_2) pants[i].fParamPants->fAngleL0+=M_PI; //gdyby w odwrotn� stron� wysz�o
             //if ((pants[i].fParamPants->fAngleL0<0.03)||(pants[i].fParamPants->fAngleL0>0.09)) //normalnie ok. 0.05
             // pants[i].fParamPants->fAngleL0=pants[i].fParamPants->fAngleL;
             pants[i].fParamPants->fAngleL=pants[i].fParamPants->fAngleL0; //pocz�tkowy k�t dolnego ramienia
             if ((sm=sm->ChildGet())!=NULL)
             {//je�li dalej jest �lizg, mo�na policzy� d�ugo�� g�rnego ramienia
              m=float4x4(*sm->GetMatrix()); //wystarczy�by wska�nik, nie trzeba kopiowa�
              //trzeba by uwzgl�dni� macierz dolnego ramienia, �eby uzyska� k�t do poziomu...
              pants[i].fParamPants->fHoriz+=fabs(m(3)[1]); //r�nica d�ugo�ci rzut�w ramion na p�aszczyzn� podstawy (jedna dodatnia, druga ujemna)
              pants[i].fParamPants->fLenU1=hypot(m(3)[1],m(3)[2]); //po osi OX nie potrzeba
              //pants[i].fParamPants->pantu=acos((1.22*cos(pants[i].fParamPants->fAngleL)+0.535)/1.755); //g�rne rami�
              //pants[i].fParamPants->fAngleU0=acos((1.176289*cos(pants[i].fParamPants->fAngleL)+0.54555075)/1.724482197); //g�rne rami�
              pants[i].fParamPants->fAngleU0=atan2(fabs(m(3)[2]),fabs(m(3)[1])); //pocz�tkowy k�t g�rnego ramienia, odczytany z modelu
              //if (pants[i].fParamPants->fAngleU0<M_PI_2) pants[i].fParamPants->fAngleU0+=M_PI; //gdyby w odwrotn� stron� wysz�o
              //if (pants[i].fParamPants->fAngleU0<0)
              // pants[i].fParamPants->fAngleU0=-pants[i].fParamPants->fAngleU0;
              //if ((pants[i].fParamPants->fAngleU0<0.00)||(pants[i].fParamPants->fAngleU0>0.09)) //normalnie ok. 0.07
              // pants[i].fParamPants->fAngleU0=acos((pants[i].fParamPants->fLenL1*cos(pants[i].fParamPants->fAngleL)+pants[i].fParamPants->fHoriz)/pants[i].fParamPants->fLenU1);
              pants[i].fParamPants->fAngleU=pants[i].fParamPants->fAngleU0; //pocz�tkowy k�t
             }
            }
           }
           else
            ErrorLog("Bad model: "+asFileName+" - missed submodel "+asAnimName); //brak ramienia
          }
        }
        else if (str==AnsiString("animpantrd2prefix:"))
        {//prefiks ramion dolnych 2
         str=Parser->GetNextSymbol();
         float4x4 m; //macierz do wyliczenia pozycji i wektora ruchu pantografu
         TSubModel *sm;
         if (pants)
          for (int i=0;i<iAnimType[ANIM_PANTS];i++)
          {//Winger 160204: wyszukiwanie max 2 patykow o nazwie str*
           asAnimName=str+AnsiString(i+1);
           sm=mdModel->GetFromName(asAnimName.c_str());
           pants[i].smElement[1]=sm; //jak NULL, to nie b�dzie animowany
           if (sm)
           {//w EP09 wywala�o si� tu z powodu NULL
            sm->WillBeAnimated();
            if (pants[i].fParamPants->vPos.y==0.0)
            {//je�li pierwsze rami� nie ustawi�o tej warto�ci, pr�bowa� drugim
             //!!!! docelowo zrobi� niezale�n� animacj� ramion z ka�dej strony
             m=float4x4(*sm->GetMatrix()); //skopiowanie, bo b�dziemy mno�y�
             m(3)[1]=m[3][1]+0.054; //w g�r� o wysoko�� �lizgu (na razie tak)
             while (sm->Parent)
             {
              if (sm->Parent->GetMatrix())
               m=*sm->Parent->GetMatrix()*m;
              sm=sm->Parent;
             }
             pants[i].fParamPants->vPos.z=m[3][0]; //przesuni�cie w bok (asymetria)
             pants[i].fParamPants->vPos.y=m[3][1]; //przesuni�cie w g�r� odczytane z modelu
            }
           }
           else
            ErrorLog("Bad model: "+asFileName+" - missed submodel "+asAnimName); //brak ramienia
          }
        }
        else if (str==AnsiString("animpantrg1prefix:"))
        {//prefiks ramion g�rnych 1
         str=Parser->GetNextSymbol();
         if (pants)
          for (int i=0;i<iAnimType[ANIM_PANTS];i++)
          {//Winger 160204: wyszukiwanie max 2 patykow o nazwie str*
           asAnimName=str+AnsiString(i+1);
           pants[i].smElement[2]=mdModel->GetFromName(asAnimName.c_str());
           pants[i].smElement[2]->WillBeAnimated();
          }
        }
        else
        if (str==AnsiString("animpantrg2prefix:"))
        {//prefiks ramion g�rnych 2
         str=Parser->GetNextSymbol();
         if (pants)
          for (int i=0;i<iAnimType[ANIM_PANTS];i++)
          {//Winger 160204: wyszukiwanie max 2 patykow o nazwie str*
           asAnimName=str+AnsiString(i+1);
           pants[i].smElement[3]=mdModel->GetFromName(asAnimName.c_str());
           pants[i].smElement[3]->WillBeAnimated();
          }
        }
        else if (str==AnsiString("animpantslprefix:"))
        {//prefiks �lizgaczy
         str=Parser->GetNextSymbol();
         if (pants)
          for (int i=0;i<iAnimType[ANIM_PANTS];i++)
          {//Winger 160204: wyszukiwanie max 2 patykow o nazwie str*
           asAnimName=str+AnsiString(i+1);
           pants[i].smElement[4]=mdModel->GetFromName(asAnimName.c_str());
           pants[i].smElement[4]->WillBeAnimated();
           pants[i].yUpdate=UpdatePant;
           pants[i].fMaxDist=300*300; //nie podnosi� w wi�kszej odleg�o�ci
           pants[i].iNumber=i;
          }
        }
        else if (str==AnsiString("pantfactors:"))
        {//Winger 010304: parametry pantografow
         double pant1x=Parser->GetNextSymbol().ToDouble();
         double pant2x=Parser->GetNextSymbol().ToDouble();
         double pant1h=Parser->GetNextSymbol().ToDouble(); //wysoko�� pierwszego �lizgu
         double pant2h=Parser->GetNextSymbol().ToDouble(); //wysoko�� drugiego �lizgu
         if (pant1h>0.5) pant1h=pant2h; //tu mo�e by� zbyt du�a warto��
         if ((pant1x<0)&&(pant2x>0)) //pierwsza powinna by� dodatnia, a druga ujemna
         {pant1x=-pant1x; pant2x=-pant2x;}
         if (pants)
          for (int i=0;i<iAnimType[ANIM_PANTS];++i)
          {//przepisanie wsp�czynnik�w do pantograf�w (na razie nie b�dzie lepiej)
           pants[i].fParamPants->fAngleL=pants[i].fParamPants->fAngleL0; //pocz�tkowy k�t dolnego ramienia
           pants[i].fParamPants->fAngleU=pants[i].fParamPants->fAngleU0; //pocz�tkowy k�t
           //pants[i].fParamPants->PantWys=1.22*sin(pants[i].fParamPants->fAngleL)+1.755*sin(pants[i].fParamPants->fAngleU); //wysoko�� pocz�tkowa
           //pants[i].fParamPants->PantWys=1.176289*sin(pants[i].fParamPants->fAngleL)+1.724482197*sin(pants[i].fParamPants->fAngleU); //wysoko�� pocz�tkowa
           pants[i].fParamPants->vPos.x=(i&1)?pant2x:pant1x;
           pants[i].fParamPants->fHeight=(i&1)?pant2h:pant1h; //wysoko�� �lizgu jest zapisana w MMD
           pants[i].fParamPants->PantWys=pants[i].fParamPants->fLenL1*sin(pants[i].fParamPants->fAngleL)+pants[i].fParamPants->fLenU1*sin(pants[i].fParamPants->fAngleU)+pants[i].fParamPants->fHeight; //wysoko�� pocz�tkowa
           //pants[i].fParamPants->vPos.y=panty-panth-pants[i].fParamPants->PantWys; //np. 4.429-0.097=4.332=~4.335
           //pants[i].fParamPants->vPos.z=0; //niezerowe dla pantograf�w asymetrycznych
           pants[i].fParamPants->PantTraction=pants[i].fParamPants->PantWys;
           //if (panty<3.0)
           // pants[i].fParamPants->fWidth=0.5*panty; //po�owa szeroko�ci �lizgu; jest w "Power: CSW="
          }
        }
        else if (str==AnsiString("animpistonprefix:"))
        {//prefiks t�oczysk - na razie u�ywamy modeli pantograf�w
         str=Parser->GetNextSymbol();
         for (int i=1;i<=2;i++)
         {
          //asAnimName=str+i;
          //smPatykird1[i-1]=mdModel->GetFromName(asAnimName.c_str());
          //smPatykird1[i-1]->WillBeAnimated();
         }
        }
        else if (str==AnsiString("animconrodprefix:"))
        {//prefiks korbowod�w - na razie u�ywamy modeli pantograf�w
         str=Parser->GetNextSymbol();
         for (int i=1;i<=2;i++)
         {
          //asAnimName=str+i;
          //smPatykirg1[i-1]=mdModel->GetFromName(asAnimName.c_str());
          //smPatykirg1[i-1]->WillBeAnimated();
         }
        }
        else if (str==AnsiString("pistonfactors:"))
        {//Ra: parametry silnika parowego (t�oka)
/* //Ra: tymczasowo wy��czone ze wzgl�du na porz�dkowanie animacji pantograf�w
         pant1x=Parser->GetNextSymbol().ToDouble(); //k�t przesuni�cia dla pierwszego t�oka
         pant2x=Parser->GetNextSymbol().ToDouble(); //k�t przesuni�cia dla drugiego t�oka
         panty=Parser->GetNextSymbol().ToDouble(); //d�ugo�� korby (r)
         panth=Parser->GetNextSymbol().ToDouble(); //d�ugo� korbowodu (k)
*/
         MoverParameters->EnginePowerSource.PowerType=SteamPower; //Ra: po chamsku, ale z CHK nie dzia�a
        }
        else if (str==AnsiString("animreturnprefix:"))
        {//prefiks dr��ka mimo�rodowego - na razie u�ywamy modeli pantograf�w
         str=Parser->GetNextSymbol();
         for (int i=1;i<=2;i++)
         {
          //asAnimName=str+i;
          //smPatykird2[i-1]=mdModel->GetFromName(asAnimName.c_str());
          //smPatykird2[i-1]->WillBeAnimated();
         }
        }
        else if (str==AnsiString("animexplinkprefix:")) //animreturnprefix:
        {//prefiks jarzma - na razie u�ywamy modeli pantograf�w
         str=Parser->GetNextSymbol();
         for (int i=1;i<=2;i++)
         {
          //asAnimName=str+i;
          //smPatykirg2[i-1]=mdModel->GetFromName(asAnimName.c_str());
          //smPatykirg2[i-1]->WillBeAnimated();
         }
        }
        else if (str==AnsiString("animpendulumprefix:"))
        {//prefiks wahaczy
         str=Parser->GetNextSymbol();
         asAnimName="";
         for (int i=1; i<=4; i++)
         {//McZapkie-050402: wyszukiwanie max 4 wahaczy o nazwie str*
          asAnimName=str+AnsiString(i);
          smWahacze[i-1]=mdModel->GetFromName(asAnimName.c_str());
          smWahacze[i-1]->WillBeAnimated();
         }
         str=Parser->GetNextSymbol().LowerCase();
         if (str==AnsiString("pendulumamplitude:"))
          fWahaczeAmp=Parser->GetNextSymbol().ToDouble();
        }
        else
        if (str==AnsiString("engineer:"))
        {//nazwa submodelu maszynisty
         str=Parser->GetNextSymbol();
         smMechanik0=mdModel->GetFromName(str.c_str());
         if (!smMechanik0)
         {//jak nie ma bez numerka, to mo�e jest z numerkiem?
          smMechanik0=mdModel->GetFromName(AnsiString(str+"1").c_str());
          smMechanik1=mdModel->GetFromName(AnsiString(str+"2").c_str());
         }
         //aby da�o si� go obraca�, musi mie� w��czon� animacj� w T3D!
         //if (!smMechanik1) //je�li drugiego nie ma
         // if (smMechanik0) //a jest pierwszy
         //  smMechanik0->WillBeAnimated(); //to b�dziemy go obraca�
        }
        else if (str==AnsiString("animdoorprefix:"))
        {//nazwa animowanych drzwi
         int i,j,k,m;
         str=Parser->GetNextSymbol();
         for (i=0,j=0;i<ANIM_DOORS;++i)
          j+=iAnimType[i]; //zliczanie wcze�niejszych animacji
         for (i=0;i<iAnimType[ANIM_DOORS];++i) //liczba drzwi
         {//NBMX wrzesien 2003: wyszukiwanie drzwi o nazwie str*
          asAnimName=str+AnsiString(i+1);
          pAnimations[i+j].smAnimated=mdModel->GetFromName(asAnimName.c_str()); //ustalenie submodelu
          if (pAnimations[i+j].smAnimated)
          {//++iAnimatedDoors;
           pAnimations[i+j].smAnimated->WillBeAnimated(); //wy��czenie optymalizacji transformu
           switch (MoverParameters->DoorOpenMethod)
           {//od razu zapinamy potrzebny typ animacji
            case 1: pAnimations[i+j].yUpdate=UpdateDoorTranslate; break;
            case 2: pAnimations[i+j].yUpdate=UpdateDoorRotate; break;
           }
           pAnimations[i+j].iNumber=i; //parzyste dzia�aj� inaczej ni� nieparzyste
           pAnimations[i+j].fMaxDist=300*300; //drzwi to z daleka wida�
           pAnimations[i+j].fSpeed=random(150); //oryginalny koncept z DoorSpeedFactor
           pAnimations[i+j].fSpeed=(pAnimations[i+j].fSpeed+100)/100;
           //Ra: te wsp�czynniki s� bez sensu, bo modyfikuj� wektor przesuni�cia
          }
         }
        }
       }
     }
     else
     if (str==AnsiString("sounds:")) //dzwieki
      while (!Parser->EndOfFile && str!=AnsiString("endsounds"))
      {
       str= Parser->GetNextSymbol().LowerCase();
       if (str==AnsiString("wheel_clatter:")) //polozenia osi w/m srodka pojazdu
        {
         dSDist=Parser->GetNextSymbol().ToDouble();
         for (int i=0; i<iAxles; i++)
          {
           str=Parser->GetNextSymbol();
           dWheelsPosition[i]=str.ToDouble();
           str= Parser->GetNextSymbol().LowerCase();
           if (str!=AnsiString("end"))
             rsStukot[i].Init(str.c_str(),dSDist,GetPosition().x,GetPosition().y+dWheelsPosition[i],GetPosition().z,true);
         }
         if (str!=AnsiString("end"))
          str= Parser->GetNextSymbol();
        }
       else
       if ((str==AnsiString("engine:")) && (MoverParameters->Power>0))   //plik z dzwiekiem silnika, mnozniki i ofsety amp. i czest.
        {
         str= Parser->GetNextSymbol();
         rsSilnik.Init(str.c_str(),Parser->GetNextSymbol().ToDouble(),GetPosition().x,GetPosition().y,GetPosition().z,true,true);
         if (rsSilnik.GetWaveTime()==0)
          ErrorLog("Missed sound: \""+str+"\" for "+asFileName);
         if (MoverParameters->EngineType==DieselEngine)
          rsSilnik.AM=Parser->GetNextSymbol().ToDouble()/(MoverParameters->Power+MoverParameters->nmax*60);
         else if (MoverParameters->EngineType==DieselElectric)
          rsSilnik.AM=Parser->GetNextSymbol().ToDouble()/(MoverParameters->Power*3);
         else
          rsSilnik.AM=Parser->GetNextSymbol().ToDouble()/(MoverParameters->Power+MoverParameters->nmax*60+MoverParameters->Power+MoverParameters->Power);
         rsSilnik.AA=Parser->GetNextSymbol().ToDouble();
         rsSilnik.FM=Parser->GetNextSymbol().ToDouble();//MoverParameters->nmax;
         rsSilnik.FA=Parser->GetNextSymbol().ToDouble();
        }
       else
       if ((str==AnsiString("ventilator:")) && (MoverParameters->EngineType==ElectricSeriesMotor))    //plik z dzwiekiem wentylatora, mnozniki i ofsety amp. i czest.
        {
         str= Parser->GetNextSymbol();
         rsWentylator.Init(str.c_str(),Parser->GetNextSymbol().ToDouble(),GetPosition().x,GetPosition().y,GetPosition().z,true,true);
         rsWentylator.AM=Parser->GetNextSymbol().ToDouble()/MoverParameters->RVentnmax;
         rsWentylator.AA=Parser->GetNextSymbol().ToDouble();
         rsWentylator.FM=Parser->GetNextSymbol().ToDouble()/MoverParameters->RVentnmax;
         rsWentylator.FA=Parser->GetNextSymbol().ToDouble();
        }
       else
       if ((str==AnsiString("transmission:")) && (MoverParameters->EngineType==ElectricSeriesMotor))    //plik z dzwiekiem, mnozniki i ofsety amp. i czest.
        {
         str= Parser->GetNextSymbol();
         rsPrzekladnia.Init(str.c_str(),Parser->GetNextSymbol().ToDouble(),GetPosition().x,GetPosition().y,GetPosition().z,true);
         rsPrzekladnia.AM=0.029;
         rsPrzekladnia.AA=0.1;
         rsPrzekladnia.FM=0.005;
         rsPrzekladnia.FA=1.0;
        }
       else
       if (str==AnsiString("brake:"))                      //plik z piskiem hamulca, mnozniki i ofsety amplitudy.
        {
         str= Parser->GetNextSymbol();
         rsPisk.Init(str.c_str(),Parser->GetNextSymbol().ToDouble(),GetPosition().x,GetPosition().y,GetPosition().z,true);
         rsPisk.AM=Parser->GetNextSymbol().ToDouble();
         rsPisk.AA=Parser->GetNextSymbol().ToDouble()*(105-random(10))/100;
         rsPisk.FM=1.0;
         rsPisk.FA=0.0;
        }
       else
       if (str==AnsiString("brakeacc:"))                      //plik z przyspieszaczem (upust po zlapaniu hamowania)
        {
         str= Parser->GetNextSymbol();
//         sBrakeAcc.Init(str.c_str(),Parser->GetNextSymbol().ToDouble(),GetPosition().x,GetPosition().y,GetPosition().z,true);
          sBrakeAcc=TSoundsManager::GetFromName(str.c_str(),true);
          bBrakeAcc=true;
//         sBrakeAcc.AM=1.0;
//         sBrakeAcc.AA=0.0;
//         sBrakeAcc.FM=1.0;
//         sBrakeAcc.FA=0.0;
        }
       else
       if (str==AnsiString("unbrake:"))                      //plik z piskiem hamulca, mnozniki i ofsety amplitudy.
        {
         str= Parser->GetNextSymbol();
         rsUnbrake.Init(str.c_str(),Parser->GetNextSymbol().ToDouble(),GetPosition().x,GetPosition().y,GetPosition().z,true);
         rsUnbrake.AM=1.0;
         rsUnbrake.AA=0.0;
         rsUnbrake.FM=1.0;
         rsUnbrake.FA=0.0;
        }
       else
       if (str==AnsiString("derail:"))                      //dzwiek przy wykolejeniu
        {
         str= Parser->GetNextSymbol();
         rsDerailment.Init(str.c_str(),Parser->GetNextSymbol().ToDouble(),GetPosition().x,GetPosition().y,GetPosition().z,true);
         rsDerailment.AM=1.0;
         rsDerailment.AA=0.0;
         rsDerailment.FM=1.0;
         rsDerailment.FA=0.0;
        }
       else
       if (str==AnsiString("dieselinc:"))                      //dzwiek przy wlazeniu na obroty woodwarda
        {
         str= Parser->GetNextSymbol();
         rsDiesielInc.Init(str.c_str(),Parser->GetNextSymbol().ToDouble(),GetPosition().x,GetPosition().y,GetPosition().z,true);
         rsDiesielInc.AM=1.0;
         rsDiesielInc.AA=0.0;
         rsDiesielInc.FM=1.0;
         rsDiesielInc.FA=0.0;
        }
       else
       if (str==AnsiString("curve:"))
        {
         str= Parser->GetNextSymbol();
         rscurve.Init(str.c_str(),Parser->GetNextSymbol().ToDouble(),GetPosition().x,GetPosition().y,GetPosition().z,true);
         rscurve.AM=1.0;
         rscurve.AA=0.0;
         rscurve.FM=1.0;
         rscurve.FA=0.0;
        }
       else
       if (str==AnsiString("horn1:"))                      //pliki z trabieniem
        {
         sHorn1.Load(Parser,GetPosition());
        }
       if (str==AnsiString("horn2:"))                      //pliki z trabieniem wysokoton.
        {
         sHorn2.Load(Parser,GetPosition());
         if (iHornWarning) iHornWarning=2; //numer syreny do u�ycia po otrzymaniu sygna�u do jazdy
        }
       if (str==AnsiString("departuresignal:"))            //pliki z sygnalem odjazdu
        {
         sDepartureSignal.Load(Parser,GetPosition());
         }
       if (str==AnsiString("pantographup:"))            //pliki dzwiekow pantografow
        {
         str=Parser->GetNextSymbol();
         sPantUp.Init(str.c_str(),50,GetPosition().x,GetPosition().y,GetPosition().z,true);
         sPantUp.AM=50000;
         sPantUp.AA=-1*(105-random(10))/100;
         sPantUp.FM=1.0;
         sPantUp.FA=0.0;
        }
       if (str==AnsiString("pantographdown:"))            //pliki dzwiekow pantografow
        {
         str= Parser->GetNextSymbol();
         sPantDown.Init(str.c_str(),50,GetPosition().x,GetPosition().y,GetPosition().z,true);
         sPantDown.AM=50000;
         sPantDown.AA=-1*(105-random(10))/100;
         sPantDown.FM=1.0;
         sPantDown.FA=0.0;
        }
       if (str==AnsiString("compressor:"))                      //pliki ze sprezarka
        {
         sCompressor.Load(Parser,GetPosition());
        }
       if (str==AnsiString("converter:"))                      //pliki z przetwornica
        {
         //if (MoverParameters->EngineType==DieselElectric) //b�dzie modulowany?
         sConverter.Load(Parser,GetPosition());
        }
       if (str==AnsiString("turbo:"))                      //pliki z turbogeneratorem
        {
         sTurbo.Load(Parser,GetPosition());
        }
       if (str==AnsiString("small-compressor:"))                      //pliki z przetwornica
        {
         sSmallCompressor.Load(Parser,GetPosition());
        }
      }
     else
     if (str==AnsiString("internaldata:"))                            //dalej nie czytaj
     {while (!Parser->EndOfFile)
      {//zbieranie informacji o kabinach
       str=Parser->GetNextSymbol().LowerCase();
       if (str=="cab0model:")
       {str=Parser->GetNextSymbol();
        if (str!="none") iCabs=2;
       }
       else if (str=="cab1model:")
       {str=Parser->GetNextSymbol();
        if (str!="none") iCabs=1;
       }
       else if (str=="cab2model:")
       {str=Parser->GetNextSymbol();
        if (str!="none") iCabs=4;
       }
      }
      Stop_InternalData=true;
     }
 }
 //ABu 050205 - tego wczesniej nie bylo i uciekala pamiec:
 delete Parser;
 if (mdModel) mdModel->Init(); //obr�cenie modelu oraz optymalizacja, r�wnie� zapisanie binarnego
 if (mdLoad) mdLoad->Init();
 if (mdPrzedsionek) mdPrzedsionek->Init();
 if (mdLowPolyInt) mdLowPolyInt->Init();
 //sHorn2.CopyIfEmpty(sHorn1); //�eby jednak tr�bi� te� drugim
 Global::asCurrentTexturePath=AnsiString(szTexturePath); //kiedy� uproszczone wn�trze miesza�o tekstury nieba
}

//---------------------------------------------------------------------------
void __fastcall TDynamicObject::RadioStop()
{//zatrzymanie pojazdu
 if (Mechanik) //o ile kto� go prowadzi
  if (MoverParameters->SecuritySystem.RadioStop) //je�li pojazd ma RadioStop i jest on aktywny
   Mechanik->PutCommand("Emergency_brake",1.0,1.0,&vPosition,stopRadio);
};

void __fastcall TDynamicObject::RaLightsSet(int head,int rear)
{//zapalenie �wiate� z przodu i z ty�u, zale�ne od kierunku pojazdu
 if (!MoverParameters) return; //mo�e tego nie by� na pocz�tku
 if (rear==2+32+64)
 {//je�li koniec poci�gu, to trzeba ustali�, czy jest tam czynna lokomotywa
  //EN57 mo�e nie mie� ko�c�wek od �rodka cz�onu
  if (MoverParameters->Power>1.0) //je�li ma moc nap�dow�
   if (!MoverParameters->ActiveDir) //je�li nie ma ustawionego kierunku
   {//je�li ma zar�wno �wiat�a jak i ko�c�wki, ustali�, czy jest w stanie aktywnym
    //np. lokomotywa na zimno b�dzie mie� ko�c�wki a nie �wiat�a
    rear=64; //tablice blaszane
    //trzeba to uzale�ni� od "za��czenia baterii" w poje�dzie
   }
  if (rear==2+32+64) //je�li nadal obydwie mo�liwo�ci
   if (iInventory&(iDirection?0x2A:0x15)) //czy ma jakie� �wiat�a czerowone od danej strony
    rear=2+32; //dwa �wiat�a czerwone
   else
    rear=64; //tablice blaszane
 }
 if (iDirection) //w zale�no�ci od kierunku pojazdu w sk�adzie
 {//jesli pojazd stoi sprz�giem 0 w stron� czo�a
  if (head>=0) iLights[0]=head;
  if (rear>=0) iLights[1]=rear;
 }
 else
 {//jak jest odwr�cony w sk�adzie (-1), to zapalamy odwrotnie
  if (head>=0) iLights[1]=head;
  if (rear>=0) iLights[0]=rear;
 }
};

int __fastcall TDynamicObject::DirectionSet(int d)
{//ustawienie kierunku w sk�adzie (wykonuje AI)
 iDirection=d>0?1:0; //d:1=zgodny,-1=przeciwny; iDirection:1=zgodny,0=przeciwny;
 CouplCounter=20; //�eby normalnie skanowa� kolizje, to musi ruszy� z miejsca
 if (MyTrack)
 {//podczas wczytywania wstawiane jest AI, ale mo�e jeszcze nie by� toru
  //AI ustawi kierunek ponownie po uruchomieniu silnika
  if (iDirection) //je�li w kierunku Coupler 0
  {if (MoverParameters->Couplers[0].CouplingFlag==ctrain_virtual) //brak pojazdu podpi�tego?
    ABuScanObjects(1,300); //szukanie czego� do pod��czenia
  }
  else
   if (MoverParameters->Couplers[1].CouplingFlag==ctrain_virtual) //brak pojazdu podpi�tego?
    ABuScanObjects(-1,300);
 }
 return 1-(iDirection?NextConnectedNo:PrevConnectedNo); //informacja o po�o�eniu nast�pnego
};

TDynamicObject* __fastcall TDynamicObject::PrevAny()
{//wska�nik na poprzedni, nawet wirtualny
 return iDirection?PrevConnected:NextConnected;
};
TDynamicObject* __fastcall TDynamicObject::Prev()
{
 if (MoverParameters->Couplers[iDirection^1].CouplingFlag)
  return iDirection?PrevConnected:NextConnected;
 return NULL; //gdy sprz�g wirtualny, to jakby nic nie by�o
};
TDynamicObject* __fastcall TDynamicObject::Next()
{
 if (MoverParameters->Couplers[iDirection].CouplingFlag)
  return iDirection?NextConnected:PrevConnected;
 return NULL; //gdy sprz�g wirtualny, to jakby nic nie by�o
};

TDynamicObject* __fastcall TDynamicObject::Neightbour(int &dir)
{//ustalenie nast�pnego (1) albo poprzedniego (0) w sk�adzie bez wzgl�du na prawid�owo�� iDirection
 int d=dir; //zapami�tanie kierunku
 dir=1-(dir?NextConnectedNo:PrevConnectedNo); //nowa warto��
 return (d?(MoverParameters->Couplers[1].CouplingFlag?NextConnected:NULL):(MoverParameters->Couplers[0].CouplingFlag?PrevConnected:NULL));
};

void __fastcall TDynamicObject::CoupleDist()
{//obliczenie odleg�o�ci sprz�g�w
 if (MyTrack?(MyTrack->iCategoryFlag&1):true) //je�li nie ma przypisanego toru, to liczy� jak dla kolei
 {//je�li jedzie po szynach (r�wnie� unimog), liczenie kul wystarczy
  MoverParameters->SetCoupleDist();
 }
 else
 {//na drodze trzeba uwzgl�dni� wektory ruchu
  double d0=MoverParameters->Couplers[0].CoupleDist;
  //double d1=MoverParameters->Couplers[1].CoupleDist; //sprz�g z ty�u samochodu mo�na ola�, dop�ki nie je�dzi na wstecznym
  vector3 p1,p2;
  double d,w; //dopuszczalny dystans w poprzek
  MoverParameters->SetCoupleDist(); //liczenie standardowe
  if (MoverParameters->Couplers[0].Connected) //je�li cokolwiek pod��czone
   if (MoverParameters->Couplers[0].CouplingFlag==0) //je�li wirtualny
    if (MoverParameters->Couplers[0].CoupleDist<300.0) //i mniej ni� 300m
    {//przez MoverParameters->Couplers[0].Connected nie da si� dosta� do DynObj, st�d prowizorka
     //WriteLog("Collision of "+AnsiString(MoverParameters->Couplers[0].CoupleDist)+"m detected by "+asName+":0.");
     w=0.5*(MoverParameters->Couplers[0].Connected->Dim.W+MoverParameters->Dim.W); //minimalna odleg�o�� mini�cia
     d=-DotProduct(vLeft,vCoulpler[0]); //odleg�o�� prostej ruchu od pocz�tku uk�adu wsp�rz�dnych
     d=fabs(DotProduct(vLeft,((TMoverParameters*)(MoverParameters->Couplers[0].Connected))->vCoulpler[MoverParameters->Couplers[0].ConnectedNr])+d);
     //WriteLog("Distance "+AnsiString(d)+"m from "+asName+":0.");
     if (d>w)
      MoverParameters->Couplers[0].CoupleDist=(d0<10?50:d0); //przywr�cenie poprzedniej
    }
  if (MoverParameters->Couplers[1].Connected) //je�li cokolwiek pod��czone
   if (MoverParameters->Couplers[1].CouplingFlag==0) //je�li wirtualny
    if (MoverParameters->Couplers[1].CoupleDist<300.0) //i mniej ni� 300m
    {
     //WriteLog("Collision of "+AnsiString(MoverParameters->Couplers[1].CoupleDist)+"m detected by "+asName+":1.");
     w=0.5*(MoverParameters->Couplers[1].Connected->Dim.W+MoverParameters->Dim.W); //minimalna odleg�o�� mini�cia
     d=-DotProduct(vLeft,vCoulpler[1]); //odleg�o�� prostej ruchu od pocz�tku uk�adu wsp�rz�dnych
     d=fabs(DotProduct(vLeft,((TMoverParameters*)(MoverParameters->Couplers[1].Connected))->vCoulpler[MoverParameters->Couplers[1].ConnectedNr])+d);
     //WriteLog("Distance "+AnsiString(d)+"m from "+asName+":1.");
     if (d>w)
      MoverParameters->Couplers[0].CoupleDist=(d0<10?50:d0); //przywr�cenie poprzedniej
    }
 }
};

TDynamicObject* __fastcall TDynamicObject::ControlledFind()
{//taka proteza: chc� pod��czy� kabin� EN57 bezpo�rednio z silnikowym, aby nie robi� tego przez ukrotnienie
 //drugi silnikowy i tak musi by� ukrotniony, podobnie jak kolejna jednostka
 //lepiej by by�o przesy�a� komendy sterowania, co jednak wymaga przebudowy transmisji komend (LD)
 //problem si� robi ze �wiat�ami, kt�re b�d� zapalane w silnikowym, ale musz� �wieci� si� w rozrz�dczych
 //dla EZT �wiat�� czo�owe b�d� "zapalane w silnikowym", ale widziane z rozrz�dczych
 //r�wnie� wczytywanie MMD powinno dotyczy� aktualnego cz�onu
 //problematyczna mo�e by� kwestia wybranej kabiny (w silnikowym...)
 //je�li silnikowy b�dzie zapi�ty odwrotnie (tzn. -1), to i tak powinno je�dzi� dobrze
 //r�wnie� hamowanie wykonuje si� zaworem w cz�onie, a nie w silnikowym...
 TDynamicObject *d=this; //zaczynamy od aktualnego
 if (d->MoverParameters->TrainType&dt_EZT) //na razie dotyczy to EZT
  if (d->NextConnected?d->MoverParameters->Couplers[1].AllowedFlag&ctrain_depot:false)
  {//gdy jest cz�on od sprz�gu 1, a sprz�g ��czony warsztatowo (powiedzmy)
   if ((d->MoverParameters->Power<1.0)&&(d->NextConnected->MoverParameters->Power>1.0)) //my nie mamy mocy, ale ten drugi ma
    d=d->NextConnected; //b�dziemy sterowa� tym z moc�
  }
  else if (d->PrevConnected?d->MoverParameters->Couplers[0].AllowedFlag&ctrain_depot:false)
  {//gdy jest cz�on od sprz�gu 0, a sprz�g ��czony warsztatowo (powiedzmy)
   if ((d->MoverParameters->Power<1.0)&&(d->PrevConnected->MoverParameters->Power>1.0)) //my nie mamy mocy, ale ten drugi ma
    d=d->PrevConnected; //b�dziemy sterowa� tym z moc�
  }
 return d;
};

