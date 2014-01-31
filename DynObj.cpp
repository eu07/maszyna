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
#include "Camera.h" //bo likwidujemy trzêsienie
#include "Console.h"
#include "Traction.h"
#pragma package(smart_init)

//Ra: taki zapis funkcjonuje lepiej, ale mo¿e nie jest optymalny
#define vWorldFront vector3(0,0,1)
#define vWorldUp vector3(0,1,0)
#define vWorldLeft CrossProduct(vWorldUp,vWorldFront)

//Ra: bo te poni¿ej to siê powiela³y w ka¿dym module odobno
//vector3 vWorldFront=vector3(0,0,1);
//vector3 vWorldUp=vector3(0,1,0);
//vector3 vWorldLeft=CrossProduct(vWorldUp,vWorldFront);

const float maxrot=(M_PI/3.0); //60°

//---------------------------------------------------------------------------
int __fastcall TAnim::TypeSet(int i)
{//ustawienie typu animacji i zale¿nej od niego iloœci animowanych submodeli
 fMaxDist=-1.0; //normalnie nie pokazywaæ
 switch (i)
 {//maska 0x00F: ile u¿ywa wskaŸników na submodele (0 gdy jeden, wtedy bez tablicy)
  //maska 0x0F0: 0-osie,1-drzwi,2-obracane,3-zderzaki,4-wózki,5-pantografy,6-t³oki
  //maska 0xF00: ile u¿ywa liczb float dla wspó³czynników i stanu
  case 0: iFlags=0x000; break; //0-oœ
  case 1: iFlags=0x010; break; //1-drzwi
  case 2: iFlags=0x020; break; //2-wahacz, dŸwignia itp.
  case 3: iFlags=0x030; break; //3-zderzak
  case 4: iFlags=0x040; break; //4-wózek
  case 5: //5-pantograf - 5 submodeli
   iFlags=0x055;
   fParamPants=new TAnimPant();
   fParamPants->vPos=vector3(0,0,0); //przypisanie domyœnych wspó³czynników do pantografów
   fParamPants->fLenL1=1.176289; //1.22;
   fParamPants->fLenU1=1.724482197; //1.755;
   fParamPants->fHoriz=0.54555075; //przesuniêcie œlizgu w d³ugoœci pojazdu wzglêdem osi obrotu dolnego ramienia
   fParamPants->fHeight=0.07; //wysokoœæ œlizgu ponad oœ obrotu
   fParamPants->fWidth=0.635; //po³owa szerokoœci œlizgu, 0.635 dla AKP-1 i AKP-4E
   fParamPants->fAngleL0=DegToRad(2.8547285515689267247882521833308);
   fParamPants->fAngleL=fParamPants->fAngleL0; //pocz¹tkowy k¹t dolnego ramienia
   //fParamPants->pantu=acos((1.22*cos(fParamPants->fAngleL)+0.535)/1.755); //górne ramiê
   fParamPants->fAngleU0=acos((fParamPants->fLenL1*cos(fParamPants->fAngleL)+fParamPants->fHoriz)/fParamPants->fLenU1); //górne ramiê
   fParamPants->fAngleU=fParamPants->fAngleU0; //pocz¹tkowy k¹t
   //fParamPants->PantWys=1.22*sin(fParamPants->fAngleL)+1.755*sin(fParamPants->fAngleU); //wysokoœæ pocz¹tkowa
   //fParamPants->PantWys=1.176289*sin(fParamPants->fAngleL)+1.724482197*sin(fParamPants->fAngleU); //wysokoœæ pocz¹tkowa
   fParamPants->PantWys=fParamPants->fLenL1*sin(fParamPants->fAngleL)+fParamPants->fLenU1*sin(fParamPants->fAngleU)+fParamPants->fHeight; //wysokoœæ pocz¹tkowa
   fParamPants->PantTraction=fParamPants->PantWys;
   fParamPants->hvPowerWire=NULL;
  break;
  case 6: iFlags=0x068; break; //6-t³ok i rozrz¹d - 8 submodeli
  default: iFlags=0;
 }
 yUpdate=NULL;
 return iFlags&15; //ile wskaŸników rezerwowaæ dla danego typu animacji
};
__fastcall TAnim::TAnim()
{//potrzebne to w ogóle?
};
__fastcall TAnim::~TAnim()
{//usuwanie animacji
 switch (iFlags&0xF0)
 {//usuwanie struktur, zale¿nie ile zosta³o stworzonych
  case 0x50: //5-pantograf
   delete fParamPants;
  break;
  case 0x60: //6-t³ok i rozrz¹d
  break;
 }
};
void __fastcall TAnim::Parovoz()
{//animowanie t³oka i rozrz¹du parowozu
};
//---------------------------------------------------------------------------
TDynamicObject* __fastcall TDynamicObject::FirstFind(int &coupler_nr)
{//szukanie skrajnego po³¹czonego pojazdu w pociagu
 //od strony sprzegu (coupler_nr) obiektu (start)
 TDynamicObject* temp=this;
 for (int i=0;i<300;i++) //ograniczenie do 300 na wypadek zapêtlenia sk³adu
 {
  if (!temp)
   return NULL; //Ra: zabezpieczenie przed ewentaulnymi b³êdami sprzêgów
  if (temp->MoverParameters->Couplers[coupler_nr].CouplingFlag==0)
   return temp; //nic nie ma ju¿ dalej pod³¹czone
  if (coupler_nr==0)
  {//je¿eli szukamy od sprzêgu 0
   if (temp->PrevConnected) //jeœli mamy coœ z przodu
   {
    if (temp->PrevConnectedNo==0) //jeœli pojazd od strony sprzêgu 0 jest odwrócony
     coupler_nr=1-coupler_nr; //to zmieniamy kierunek sprzêgu
    temp=temp->PrevConnected; //ten jest od strony 0
   }
   else
    return temp; //jeœli jednak z przodu nic nie ma
  }
  else
  {
   if (temp->NextConnected)
   {if (temp->NextConnectedNo==1) //jeœli pojazd od strony sprzêgu 1 jest odwrócony
     coupler_nr=1-coupler_nr; //to zmieniamy kierunek sprzêgu
    temp=temp->NextConnected; //ten pojazd jest od strony 1
   }
   else
    return temp; //jeœli jednak z ty³u nic nie ma
  }
 }
 return NULL; //to tylko po wyczerpaniu pêtli
};

//---------------------------------------------------------------------------
float __fastcall TDynamicObject::GetEPP()
{//szukanie skrajnego po³¹czonego pojazdu w pociagu
 //od strony sprzegu (coupler_nr) obiektu (start)
 TDynamicObject* temp=this;
 int coupler_nr=0;
 float eq=0,am=0;

 for (int i=0;i<300;i++) //ograniczenie do 300 na wypadek zapêtlenia sk³adu
 {
  if (!temp)
   break; //Ra: zabezpieczenie przed ewentaulnymi b³êdami sprzêgów
  eq+=temp->MoverParameters->PipePress*temp->MoverParameters->Dim.L;
  am+=temp->MoverParameters->Dim.L;
  if ((temp->MoverParameters->Couplers[coupler_nr].CouplingFlag&2)!=2)
   break; //nic nie ma ju¿ dalej pod³¹czone
  if (coupler_nr==0)
  {//je¿eli szukamy od sprzêgu 0
   if (temp->PrevConnected) //jeœli mamy coœ z przodu
   {
    if (temp->PrevConnectedNo==0) //jeœli pojazd od strony sprzêgu 0 jest odwrócony
     coupler_nr=1-coupler_nr; //to zmieniamy kierunek sprzêgu
    temp=temp->PrevConnected; //ten jest od strony 0
   }
   else
    break; //jeœli jednak z przodu nic nie ma
  }
  else
  {
   if (temp->NextConnected)
   {if (temp->NextConnectedNo==1) //jeœli pojazd od strony sprzêgu 1 jest odwrócony
     coupler_nr=1-coupler_nr; //to zmieniamy kierunek sprzêgu
    temp=temp->NextConnected; //ten pojazd jest od strony 1
   }
   else
    break; //jeœli jednak z ty³u nic nie ma
  }
 }

 temp=this;
 coupler_nr=1;
 for (int i=0;i<300;i++) //ograniczenie do 300 na wypadek zapêtlenia sk³adu
 {
  if (!temp)
   break; //Ra: zabezpieczenie przed ewentaulnymi b³êdami sprzêgów
  eq+=temp->MoverParameters->PipePress*temp->MoverParameters->Dim.L;
  am+=temp->MoverParameters->Dim.L;
  if ((temp->MoverParameters->Couplers[coupler_nr].CouplingFlag&2)!=2)
   break; //nic nie ma ju¿ dalej pod³¹czone
  if (coupler_nr==0)
  {//je¿eli szukamy od sprzêgu 0
   if (temp->PrevConnected) //jeœli mamy coœ z przodu
   {
    if (temp->PrevConnectedNo==0) //jeœli pojazd od strony sprzêgu 0 jest odwrócony
     coupler_nr=1-coupler_nr; //to zmieniamy kierunek sprzêgu
    temp=temp->PrevConnected; //ten jest od strony 0
   }
   else
    break; //jeœli jednak z przodu nic nie ma
  }
  else
  {
   if (temp->NextConnected)
   {if (temp->NextConnectedNo==1) //jeœli pojazd od strony sprzêgu 1 jest odwrócony
     coupler_nr=1-coupler_nr; //to zmieniamy kierunek sprzêgu
    temp=temp->NextConnected; //ten pojazd jest od strony 1
   }
   else
    break; //jeœli jednak z ty³u nic nie ma
  }
 }
 eq-=MoverParameters->PipePress*MoverParameters->Dim.L;
 am-=MoverParameters->Dim.L;
 return eq/am;
};


//---------------------------------------------------------------------------
TDynamicObject* __fastcall TDynamicObject::GetFirstDynamic(int cpl_type)
{//Szukanie skrajnego po³¹czonego pojazdu w pociagu
 //od strony sprzegu (cpl_type) obiektu szukajacego
 //Ra: wystarczy jedna funkcja do szukania w obu kierunkach
 return FirstFind(cpl_type); //u¿ywa referencji
};

/*
TDynamicObject* __fastcall TDynamicObject::GetFirstCabDynamic(int cpl_type)
{//ZiomalCl: szukanie skrajnego obiektu z kabin¹
 TDynamicObject* temp=this;
 int coupler_nr=cpl_type;
 for (int i=0;i<300;i++) //ograniczenie do 300 na wypadek zapêtlenia sk³adu
 {
  if (!temp)
   return NULL; //Ra: zabezpieczenie przed ewentaulnymi b³êdami sprzêgów
  if (temp->MoverParameters->CabNo!=0&&temp->MoverParameters->SandCapacity!=0)
    return temp; //nic nie ma ju¿ dalej pod³¹czone
  if (temp->MoverParameters->Couplers[coupler_nr].CouplingFlag==0)
   return NULL;
  if (coupler_nr==0)
  {//je¿eli szukamy od sprzêgu 0
   if (temp->PrevConnectedNo==0) //jeœli pojazd od strony sprzêgu 0 jest odwrócony
    coupler_nr=1-coupler_nr; //to zmieniamy kierunek sprzêgu
   if (temp->PrevConnected)
    temp=temp->PrevConnected; //ten jest od strony 0
  }
  else
  {
   if (temp->NextConnectedNo==1) //jeœli pojazd od strony sprzêgu 1 jest odwrócony
    coupler_nr=1-coupler_nr; //to zmieniamy kierunek sprzêgu
   if (temp->NextConnected)
    temp=temp->NextConnected; //ten pojazd jest od strony 1
  }
 }
 return NULL; //to tylko po wyczerpaniu pêtli
};
*/

void TDynamicObject::ABuSetModelShake(vector3 mShake)
{
 modelShake=mShake;
};

int __fastcall TDynamicObject::GetPneumatic(bool front, bool red)
{
 int x,y,z; //1=prosty, 2=skoœny
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
 z=0; //brak wê¿y?
 if ((x==1)&&(y==1)) z=3; //dwa proste
 if ((x==2)&&(y==0)) z=1; //lewy skoœny, brak prawego
 if ((x==0)&&(y==2)) z=2; //brak lewego, prawy skoœny

 return z;
}

void __fastcall TDynamicObject::SetPneumatic(bool front,bool red)
{
 int x=0,ten,tamten;
 ten=GetPneumatic(front,red); //1=lewy skos,2=prawy skos,3=dwa proste
 if (front)
  if (PrevConnected) //pojazd od strony sprzêgu 0
   tamten=PrevConnected->GetPneumatic((PrevConnectedNo==0?true:false),red);
 if (!front)
  if (NextConnected) //pojazd od strony sprzêgu 1
   tamten=NextConnected->GetPneumatic((NextConnectedNo==0?true:false),red);
 if (ten==tamten) //jeœli uk³ad jest symetryczny
  switch (ten)
   {
    case 1: x=2; break; //mamy lewy skos, daæ lewe skosy
    case 2: x=3; break; //mamy prawy skos, daæ prawe skosy
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
 {if (red) cp1=x; else sp1=x;} //który pokazywaæ z przodu
 else
 {if (red) cp2=x; else sp2=x;} //który pokazywaæ z ty³u
}

void TDynamicObject::UpdateAxle(TAnim *pAnim)
{//animacja osi
 pAnim->smAnimated->SetRotate(float3(1,0,0),*pAnim->dWheelAngle);
};

void TDynamicObject::UpdateBoogie(TAnim *pAnim)
{//animacja wózka
 pAnim->smAnimated->SetRotate(float3(1,0,0),*pAnim->dWheelAngle);
};

void TDynamicObject::UpdateDoorTranslate(TAnim *pAnim)
{//animacja drzwi - przesuw
 //WriteLog("Dla drzwi nr:", i);
 //WriteLog("Wspolczynnik", DoorSpeedFactor[i]);
 //Ra: te wspó³czynniki s¹ bez sensu, bo modyfikuj¹ wektor przesuniêcia
 //w efekcie drzwi otwierane na zewn¹trz bêd¹ odlatywac dowolnie daleko :)
 //ograniczy³em zakres ruchu funkcj¹ max
 if (pAnim->smAnimated)
 {
  if (pAnim->iNumber&1)
   pAnim->smAnimated->SetTranslate(vector3(0,0,Max0R(dDoorMoveR*pAnim->fSpeed,dDoorMoveR)));
  else
   pAnim->smAnimated->SetTranslate(vector3(0,0,Max0R(dDoorMoveL*pAnim->fSpeed,dDoorMoveL)));
 }
};

void TDynamicObject::UpdateDoorRotate(TAnim *pAnim)
{//animacja drzwi - obrót
 if (pAnim->smAnimated)
 {//if (MoverParameters->DoorOpenMethod==2) //obrotowe albo dwój³omne (trzeba kombinowac submodelami i ShiftL=90,R=180)
  if (pAnim->iNumber&1)
   pAnim->smAnimated->SetRotate(float3(1,0,0),dDoorMoveR);
  else
   pAnim->smAnimated->SetRotate(float3(1,0,0),dDoorMoveL);
 }
};

void TDynamicObject::UpdatePant(TAnim *pAnim)
{//animacja pantografu - 4 obracane ramiona, œlizg pi¹ty
 float a,b,c;
 a=RadToDeg(pAnim->fParamPants->fAngleL-pAnim->fParamPants->fAngleL0);
 b=RadToDeg(pAnim->fParamPants->fAngleU-pAnim->fParamPants->fAngleU0);
 c=a+b;
 if (pAnim->smElement[0]) pAnim->smElement[0]->SetRotate(float3(-1,0,0),a); //dolne ramiê
 if (pAnim->smElement[1]) pAnim->smElement[1]->SetRotate(float3(1,0,0),a);
 if (pAnim->smElement[2]) pAnim->smElement[2]->SetRotate(float3(1,0,0),c); //górne ramiê
 if (pAnim->smElement[3]) pAnim->smElement[3]->SetRotate(float3(-1,0,0),c);
 if (pAnim->smElement[4]) pAnim->smElement[4]->SetRotate(float3(-1,0,0),b); //œlizg
};

//ABu 29.01.05 przeklejone z render i renderalpha: *********************
void __inline TDynamicObject::ABuLittleUpdate(double ObjSqrDist)
{//ABu290105: pozbierane i uporzadkowane powtarzajace sie rzeczy z Render i RenderAlpha
 //dodatkowy warunek, if (ObjSqrDist<...) zeby niepotrzebnie nie zmianiec w obiektach,
 //ktorych i tak nie widac
 //NBMX wrzesien, MC listopad: zuniwersalnione
 btnOn=false; //czy przywróciæ stan domyœlny po renderowaniu

 if (ObjSqrDist<160000) //gdy bli¿ej ni¿ 400m
 {
  for (int i=0;i<iAnimations;++i) //wykonanie kolejnych animacji
   if (ObjSqrDist<pAnimations[i].fMaxDist)
    if (pAnimations[i].yUpdate) //jeœli zdefiniowana funkcja
     pAnimations[i].yUpdate(pAnimations+i); //aktualizacja animacji (po³o¿enia submodeli
  if (ObjSqrDist<2500) //gdy bli¿ej ni¿ 50m
  {
   //ABu290105: rzucanie pudlem
   //te animacje wymagaj¹ bananów w modelach!
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
   if (smMechanik0) //mechanik od strony sprzêgu 0
    if (smMechanik1) //jak jest drugi, to tego tylko pokazujemy
     smMechanik0->iVisible=MoverParameters->ActiveCab>0;
    else
    {//jak jest tylko jeden, to do drugiej kabiny go obracamy
     smMechanik0->iVisible=(MoverParameters->ActiveCab!=0);
     smMechanik0->SetRotate(float3(0,0,1),MoverParameters->ActiveCab>=0?0:180); //obrót wzglêdem osi Z
    }
   if (smMechanik1) //mechanik od strony sprzêgu 1
    smMechanik1->iVisible=MoverParameters->ActiveCab<0;
  }
  //ABu: Przechyly na zakretach
  //Ra: przechy³kê za³atwiamy na etapie przesuwania modelu
  //if (ObjSqrDist<80000) ABuModelRoll(); //przechy³ki od 400m
 }
 if (MoverParameters->Battery)
 {//sygna³y czo³a pociagu //Ra: wyœwietlamy bez ograniczeñ odleg³oœci, by by³y widoczne z daleka
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
 return atan2(-calc_temp.x,calc_temp.z); //Ra: tak proœciej
}

TDynamicObject* __fastcall TDynamicObject::ABuFindNearestObject(TTrack *Track,TDynamicObject *MyPointer,int &CouplNr)
{//zwraca wskaznik do obiektu znajdujacego sie na torze (Track), którego sprzêg jest najblizszy kamerze
 //s³u¿y np. do ³¹czenia i rozpinania sprzêgów
 //WE: Track      - tor, na ktorym odbywa sie poszukiwanie
 //    MyPointer  - wskaznik do obiektu szukajacego
 //WY: CouplNr    - który sprzêg znalezionego obiektu jest bli¿szy kamerze

 //Uwaga! Jesli CouplNr==-2 to szukamy njblizszego obiektu, a nie sprzegu!!!

 if ((Track->iNumDynamics)>0)
 {//o ile w ogóle jest co przegl¹daæ na tym torze
  vector3 poz; //pozycja pojazdu XYZ w scenerii
  vector3 kon; //wektor czo³a wzglêdem œrodka pojazdu wzglêem pocz¹tku toru
  vector3 tmp; //wektor pomiêdzy kamer¹ i sprzêgiem
  double dist; //odleg³oœæ
  for (int i=0;i<Track->iNumDynamics;i++)
  {
   if (CouplNr==-2)
   {//wektor [kamera-obiekt] - poszukiwanie obiektu
    tmp=Global::GetCameraPosition()-Track->Dynamics[i]->vPosition;
    dist=tmp.x*tmp.x+tmp.y*tmp.y+tmp.z*tmp.z; //odleg³oœæ do kwadratu
    if (dist<100.0) //10 metrów
     return Track->Dynamics[i];
   }
   else //jeœli (CouplNr) inne niz -2, szukamy sprzêgu
   {//wektor [kamera-sprzeg0], potem [kamera-sprzeg1]
    //Powinno byc wyliczone, ale nie zaszkodzi drugi raz:
    //(bo co, jesli nie wykonuje sie obrotow wozkow?)
    Track->Dynamics[i]->modelRot.z=ABuAcos(Track->Dynamics[i]->Axle0.pPosition-Track->Dynamics[i]->Axle1.pPosition);
    poz=Track->Dynamics[i]->vPosition; //pozycja œrodka pojazdu
    kon=vector3( //po³o¿enie przodu wzglêdem œrodka
     -((0.5*Track->Dynamics[i]->MoverParameters->Dim.L)*sin(Track->Dynamics[i]->modelRot.z)),
     0, //yyy... jeœli du¿e pochylenie i d³ugi pojazd, to mo¿e byæ problem
     +((0.5*Track->Dynamics[i]->MoverParameters->Dim.L)*cos(Track->Dynamics[i]->modelRot.z))
    );
    tmp=Global::GetCameraPosition()-poz-kon;
    dist=tmp.x*tmp.x+tmp.y*tmp.y+tmp.z*tmp.z; //odleg³oœæ do kwadratu
    if (dist<25.0) //5 metrów
    {
     CouplNr=0;
     return Track->Dynamics[i];
    }
    tmp=Global::GetCameraPosition()-poz+kon;
    dist=tmp.x*tmp.x+tmp.y*tmp.y+tmp.z*tmp.z; //odleg³oœæ do kwadratu
    if (dist<25.0) //5 metrów
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
 //double MyScanDir=ScanDir;  //Moja orientacja na torze.  //Ra: nie u¿ywane
 if (ABuGetDirection()<0) ScanDir=-ScanDir;
 TDynamicObject* FoundedObj;
 FoundedObj=ABuFindNearestObject(Track,this,CouplNr); //zwraca numer sprzêgu znalezionego pojazdu
 if (FoundedObj==NULL)
 {
  double ActDist;    //Przeskanowana odleglosc.
  double CurrDist=0; //Aktualna dlugosc toru.
  if (ScanDir>=0) ActDist=Track->Length()-RaTranslationGet(); //???-przesuniêcie wózka wzglêdem Point1 toru
             else ActDist=RaTranslationGet(); //przesuniêcie wózka wzglêdem Point1 toru
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
   else //do ty³u
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
{//ustawienie przechy³ki pojazdu i jego zawartoœci
// Ra: przechy³kê za³atwiamy na etapie przesuwania modelu
}

//ABu 06.05.04 poczatek wyliczania obrotow wozkow **********************

void __fastcall TDynamicObject::ABuBogies()
{//Obracanie wozkow na zakretach. Na razie uwzglêdnia tylko zakrêty,
 //bez zadnych gorek i innych przeszkod.
 if ((smBogie[0]!=NULL)&&(smBogie[1]!=NULL))
 {
  modelRot.z=ABuAcos(Axle0.pPosition-Axle1.pPosition); //k¹t obrotu pojazdu
  //bogieRot[0].z=ABuAcos(Axle0.pPosition-Axle3.pPosition);
  bogieRot[0].z=Axle0.vAngles.z;
  bogieRot[0]=RadToDeg(modelRot-bogieRot[0]); //mno¿enie wektora przez sta³¹
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
 iAxleFirst=0; //pojazd powi¹zany z przedni¹ osi¹ - Axle0
}

//Ra: w poni¿szej funkcji jest problem ze sprzêgami
TDynamicObject* __fastcall TDynamicObject::ABuFindObject(TTrack *Track,int ScanDir,Byte &CouplFound,double &dist)
{//Zwraca wskaŸnik najbli¿szego obiektu znajduj¹cego siê
 //na torze w okreœlonym kierunku, ale tylko wtedy, kiedy
 //obiekty mog¹ siê zderzyæ, tzn. nie mijaj¹ siê.

 //WE: Track      - tor, na ktorym odbywa sie poszukiwanie,
 //    MyPointer  - wskaznik do obiektu szukajacego. //Ra: zamieni³em na "this"
 //    ScanDir    - kierunek szukania na torze (+1:w stronê Point2, -1:w stronê Point1)
 //    MyScanDir  - kierunek szukania obiektu szukajacego (na jego torze); Ra: nie potrzebne
 //    MyCouplFound - nr sprzegu obiektu szukajacego; Ra: nie potrzebne

 //WY: wskaznik do znalezionego obiektu.
 //    CouplFound - nr sprzegu znalezionego obiektu
 if (Track->iNumDynamics>0)
 {//sens szukania na tym torze jest tylko, gdy s¹ na nim pojazdy
  double ObjTranslation; //pozycja najblizszego obiektu na torze
  double MyTranslation; //pozycja szukaj¹cego na torze
  double MinDist=Track->Length(); //najmniejsza znaleziona odlegloœæ (zaczynamy od d³ugoœci toru)
  double TestDist; //robocza odleg³oœæ od kolejnych pojazdów na danym odcinku
  int iMinDist=-1;  //indeks wykrytego obiektu
  //if (Track->iNumDynamics>1)
  // iMinDist+=0; //tymczasowo pu³apka
  if (MyTrack==Track) //gdy szukanie na tym samym torze
   MyTranslation=RaTranslationGet(); //po³o¿enie wózka wzglêdem Point1 toru
  else //gdy szukanie na innym torze
   if (ScanDir>0)
    MyTranslation=0; //szukanie w kierunku Point2 (od zera) - jesteœmy w Point1
   else
    MyTranslation=MinDist; //szukanie w kierunku Point1 (do zera) - jesteœmy w Point2
  if (ScanDir>=0)
  {//jeœli szukanie w kierunku Point2
   for (int i=0;i<Track->iNumDynamics;i++)
   {//pêtla po pojazdach
    if (Track->Dynamics[i]!=this) //szukaj¹cy siê nie liczy
    {
     TestDist=(Track->Dynamics[i]->RaTranslationGet())-MyTranslation; //odleg³og³oœæ tamtego od szukaj¹cego
     if ((TestDist>0)&&(TestDist<=MinDist))
     {//gdy jest po w³aœciwej stronie i bli¿ej ni¿ jakiœ wczeœniejszy
      CouplFound=(Track->Dynamics[i]->RaDirectionGet()>0)?1:0; //to, bo (ScanDir>=0)
      if (Track->iCategoryFlag&254) //trajektoria innego typu ni¿ tor kolejowy
      {//dla torów nie ma sensu tego sprawdzaæ, rzadko co jedzie po jednej szynie i siê mija
       //Ra: mijanie samochodów wcale nie jest proste
       // Przesuniecie wzgledne pojazdow. Wyznaczane, zeby sprawdzic,
       // czy pojazdy faktycznie sie zderzaja (moga byc przesuniete
       // w/m siebie tak, ze nie zachodza na siebie i wtedy sie mijaja).
       double RelOffsetH; //wzajemna odleg³oœæ poprzeczna
       if (CouplFound) //my na tym torze byœmy byli w kierunku Point2
        //dla CouplFound=1 s¹ zwroty zgodne - istotna ró¿nica przesuniêæ
        RelOffsetH=(MoverParameters->OffsetTrackH-Track->Dynamics[i]->MoverParameters->OffsetTrackH);
       else
        //dla CouplFound=0 s¹ zwroty przeciwne - przesuniêcia sumuj¹ siê
        RelOffsetH=(MoverParameters->OffsetTrackH+Track->Dynamics[i]->MoverParameters->OffsetTrackH);
       if (RelOffsetH<0) RelOffsetH=-RelOffsetH;
       if (RelOffsetH+RelOffsetH>MoverParameters->Dim.W+Track->Dynamics[i]->MoverParameters->Dim.W)
        continue; //odleg³oœæ wiêksza od po³owy sumy szerokoœci - kolizji nie bêdzie
       //jeœli zahaczenie jest niewielkie, a jest miejsce na poboczu, to zjechaæ na pobocze
      }
      iMinDist=i; //potencjalna kolizja
      MinDist=TestDist; //odlegloœæ pomiêdzy aktywnymi osiami pojazdów
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
     TestDist=MyTranslation-(Track->Dynamics[i]->RaTranslationGet()); //???-przesuniêcie wózka wzglêdem Point1 toru
     if ((TestDist>0)&&(TestDist<MinDist))
     {
      CouplFound=(Track->Dynamics[i]->RaDirectionGet()>0)?0:1; //odwrotnie, bo (ScanDir<0)
      if (Track->iCategoryFlag&254) //trajektoria innego typu ni¿ tor kolejowy
      {//dla torów nie ma sensu tego sprawdzaæ, rzadko co jedzie po jednej szynie i siê mija
       //Ra: mijanie samochodów wcale nie jest proste
       // Przesuniêcie wzglêdne pojazdów. Wyznaczane, ¿eby sprawdziæ,
       // czy pojazdy faktycznie siê zderzaj¹ (mog¹ byæ przesuniête
       // w/m siebie tak, ¿e nie zachodz¹ na siebie i wtedy sie mijaj¹).
       double RelOffsetH; //wzajemna odleg³oœæ poprzeczna
       if (CouplFound) //my na tym torze byœmy byli w kierunku Point1
        //dla CouplFound=1 s¹ zwroty zgodne - istotna ró¿nica przesuniêæ
        RelOffsetH=(MoverParameters->OffsetTrackH-Track->Dynamics[i]->MoverParameters->OffsetTrackH);
       else
        //dla CouplFound=0 s¹ zwroty przeciwne - przesuniêcia sumuj¹ siê
        RelOffsetH=(MoverParameters->OffsetTrackH+Track->Dynamics[i]->MoverParameters->OffsetTrackH);
       if (RelOffsetH<0) RelOffsetH=-RelOffsetH;
       if (RelOffsetH+RelOffsetH>MoverParameters->Dim.W+Track->Dynamics[i]->MoverParameters->Dim.W)
        continue; //odleg³oœæ wiêksza od po³owy sumy szerokoœci - kolizji nie bêdzie
      }
      iMinDist=i; //potencjalna kolizja
      MinDist=TestDist; //odlegloœæ pomiêdzy aktywnymi osiami pojazdów
     }
    }
   }
  }
  dist+=MinDist; //doliczenie odleg³oœci przeszkody albo d³ugoœci odcinka do przeskanowanej odleg³oœci
  return (iMinDist>=0)?Track->Dynamics[iMinDist]:NULL;
 }
 dist+=Track->Length(); //doliczenie d³ugoœci odcinka do przeskanowanej odleg³oœci
 return NULL; //nie ma pojazdów na torze, to jest NULL
}

int TDynamicObject::DettachStatus(int dir)
{//sprawdzenie odleg³oœci sprzêgów rzeczywistych od strony (dir): 0=przód,1=ty³
 //Ra: dziwne, ¿e ta funkcja nie jest u¿ywana
 if (!MoverParameters->Couplers[dir].CouplingFlag)
  return 0; //jeœli nic nie pod³¹czone, to jest OK
 return (MoverParameters->DettachStatus(dir)); //czy jest w odpowiedniej odleg³oœci?
}

int TDynamicObject::Dettach(int dir)
{//roz³¹czenie sprzêgów rzeczywistych od strony (dir): 0=przód,1=ty³
 //zwraca maskê bitow¹ aktualnych sprzegów (0 jeœli roz³¹czony)
 if (MoverParameters->Couplers[dir].CouplingFlag) //odczepianie, o ile coœ pod³¹czone
  MoverParameters->Dettach(dir);
 return MoverParameters->Couplers[dir].CouplingFlag; //sprzêg po roz³¹czaniu (czego siê nie da odpi¹æ
}

void TDynamicObject::CouplersDettach(double MinDist,int MyScanDir)
{//funkcja roz³¹czajaca pod³¹czone sprzêgi, jeœli odleg³oœæ przekracza (MinDist)
 //MinDist - dystans minimalny, dla ktorego mozna roz³¹czaæ
 if (MyScanDir>0)
 {
  if (PrevConnected) //pojazd od strony sprzêgu 0
  {
   if (MoverParameters->Couplers[0].CoupleDist>MinDist) //sprzêgi wirtualne zawsze przekraczaj¹
   {
    if ((PrevConnectedNo?PrevConnected->NextConnected:PrevConnected->PrevConnected)==this)
    {//Ra: nie roz³¹czamy znalezionego, je¿eli nie do nas pod³¹czony (mo¿e jechaæ w innym kierunku)
     PrevConnected->MoverParameters->Couplers[PrevConnectedNo].Connected=NULL;
     if (PrevConnectedNo==0)
     {
      PrevConnected->PrevConnectedNo=2; //sprzêg 0 nie pod³¹czony
      PrevConnected->PrevConnected=NULL;
     }
     else if (PrevConnectedNo==1)
     {
      PrevConnected->NextConnectedNo=2; //sprzêg 1 nie pod³¹czony
      PrevConnected->NextConnected=NULL;
     }
    }
    //za to zawsze od³¹czamy siebie
    PrevConnected=NULL;
    PrevConnectedNo=2; //sprzêg 0 nie pod³¹czony
    MoverParameters->Couplers[0].Connected=NULL;
   }
  }
 }
 else
 {
  if (NextConnected) //pojazd od strony sprzêgu 1
  {
   if (MoverParameters->Couplers[1].CoupleDist>MinDist) //sprzêgi wirtualne zawsze przekraczaj¹
   {
    if ((NextConnectedNo?NextConnected->NextConnected:NextConnected->PrevConnected)==this)
    {//Ra: nie roz³¹czamy znalezionego, je¿eli nie do nas pod³¹czony (mo¿e jechaæ w innym kierunku)
     NextConnected->MoverParameters->Couplers[NextConnectedNo].Connected=NULL;
     if (NextConnectedNo==0)
     {
      NextConnected->PrevConnectedNo=2; //sprzêg 0 nie pod³¹czony
      NextConnected->PrevConnected=NULL;
     }
     else if (NextConnectedNo==1)
     {
      NextConnected->NextConnectedNo=2; //sprzêg 1 nie pod³¹czony
      NextConnected->NextConnected=NULL;
     }
    }
    NextConnected=NULL;
    NextConnectedNo=2; //sprzêg 1 nie pod³¹czony
    MoverParameters->Couplers[1].Connected=NULL;
   }
  }
 }
}

void TDynamicObject::ABuScanObjects(int ScanDir,double ScanDist)
{//skanowanie toru w poszukiwaniu koliduj¹cych pojazdów
 //ScanDir - okreœla kierunek poszukiwania zale¿nie od zwrotu prêdkoœci pojazdu
 // ScanDir=1 - od strony Coupler0, ScanDir=-1 - od strony Coupler1
 int MyScanDir=ScanDir;  //zapamiêtanie kierunku poszukiwañ na torze pocz¹tkowym, wzglêdem sprzêgów
 TTrackFollower *FirstAxle=(MyScanDir>0?&Axle0:&Axle1); //mo¿na by to trzymaæ w trainset
 TTrack *Track=FirstAxle->GetTrack(); //tor na którym "stoi" skrajny wózek (mo¿e byæ inny ni¿ tor pojazdu)
 if (FirstAxle->GetDirection()<0) //czy oœ jest ustawiona w stronê Point1?
  ScanDir=-ScanDir; //jeœli tak, to kierunek szukania bêdzie przeciwny (teraz wzglêdem toru)
 Byte MyCouplFound; //numer sprzêgu do pod³¹czenia w obiekcie szukajacym
 MyCouplFound=(MyScanDir<0)?1:0;
 Byte CouplFound; //numer sprzêgu w znalezionym obiekcie (znaleziony wype³ni)
 TDynamicObject *FoundedObj; //znaleziony obiekt
 double ActDist=0; //przeskanowana odlegloœæ; odleg³oœæ do zawalidrogi
 FoundedObj=ABuFindObject(Track,ScanDir,CouplFound,ActDist); //zaczynamy szukaæ na tym samym torze

/*
 if (FoundedObj) //jak coœ znajdzie, to œledzimy
 {//powtórzenie wyszukiwania tylko do zastawiania pu³epek podczas testów
  if (ABuGetDirection()<0) ScanDir=ScanDir; //ustalenie kierunku wzglêdem toru
  FoundedObj=ABuFindObject(Track,this,ScanDir,CouplFound);
 }
*/

 if (DebugModeFlag)
 if (FoundedObj) //kod s³u¿¹cy do logowania b³êdów
  if (CouplFound==0)
  {
   if (FoundedObj->PrevConnected)
    if (FoundedObj->PrevConnected!=this) //odœwie¿enie tego samego siê nie liczy
     WriteLog("0! Coupler warning on "+asName+":"+AnsiString(MyCouplFound)+" - "+FoundedObj->asName+":0 connected to "+FoundedObj->PrevConnected->asName+":"+AnsiString(FoundedObj->PrevConnectedNo));
  }
  else
  {
   if (FoundedObj->NextConnected)
    if (FoundedObj->NextConnected!=this) //odœwie¿enie tego samego siê nie liczy
     WriteLog("0! Coupler warning on "+asName+":"+AnsiString(MyCouplFound)+" - "+FoundedObj->asName+":1 connected to "+FoundedObj->NextConnected->asName+":"+AnsiString(FoundedObj->NextConnectedNo));
  }


 if (FoundedObj==NULL) //jeœli nie ma na tym samym, szukamy po okolicy
 {//szukanie najblizszego toru z jakims obiektem
  //praktycznie przeklejone z TraceRoute()...
  //double CurrDist=0; //aktualna dlugosc toru
  if (ScanDir>=0) //uwzglêdniamy kawalek przeanalizowanego wczeœniej toru
   ActDist=Track->Length()-FirstAxle->GetTranslation(); //odleg³oœæ osi od Point2 toru
  else
   ActDist=FirstAxle->GetTranslation(); //odleg³oœæ osi od Point1 toru
  while (ActDist<ScanDist)
  {
   //ActDist+=CurrDist; //odleg³oœæ ju¿ przeanalizowana
   if (ScanDir>0) //w kierunku Point2 toru
   {
    if (Track?Track->iNextDirection:false) //jeœli nastêpny tor jest podpiêty od Point2
     ScanDir=-ScanDir; //to zmieniamy kierunek szukania na tym torze
    Track=Track->CurrentNext(); //potem dopiero zmieniamy wskaŸnik
   }
   else //w kierunku Point1
   {
    if (Track?!Track->iPrevDirection:true) //jeœli poprzedni tor nie jest podpiêty od Point2
     ScanDir=-ScanDir; //to zmieniamy kierunek szukania na tym torze
    Track=Track->CurrentPrev(); //potem dopiero zmieniamy wskaŸnik
   }
   if (Track)
   {//jesli jest kolejny odcinek toru
    //CurrDist=Track->Length(); //doliczenie tego toru do przejrzanego dystandu
    FoundedObj=ABuFindObject(Track,ScanDir,CouplFound,ActDist); //przejrzenie pojazdów tego toru
    if (FoundedObj)
    {
     //ActDist=ScanDist; //wyjœcie z pêtli poszukiwania
     break;
    }
   }
   else //jeœli toru nie ma, to wychodzimy
   {
    ActDist=ScanDist+1.0; //koniec przegl¹dania torów
    break;
   }
  }
 } // Koniec szukania najbli¿szego toru z jakimœ obiektem.
 //teraz odczepianie i jeœli coœ siê znalaz³o, doczepianie.
 if (MyScanDir>0?PrevConnected:NextConnected)
  if ((MyScanDir>0?PrevConnected:NextConnected)!=FoundedObj)
   CouplersDettach(1.0,MyScanDir); //od³¹czamy, jeœli dalej ni¿ metr
 // i ³¹czenie sprzêgiem wirtualnym
 if (FoundedObj)
 {//siebie mo¿na bezpiecznie pod³¹czyæ jednostronnie do znalezionego
  MoverParameters->Attach(MyCouplFound,CouplFound,FoundedObj->MoverParameters,ctrain_virtual);
  //MoverParameters->Couplers[MyCouplFound].Render=false; //wirtualnego nie renderujemy
  if (MyCouplFound==0)
  {
   PrevConnected=FoundedObj; //pojazd od strony sprzêgu 0
   PrevConnectedNo=CouplFound;
  }
  else
  {
   NextConnected=FoundedObj; //pojazd od strony sprzêgu 1
   NextConnectedNo=CouplFound;
  }
  if (FoundedObj->MoverParameters->Couplers[CouplFound].CouplingFlag==ctrain_virtual)
  {//Ra: wpinamy siê wirtualnym tylko jeœli znaleziony ma wirtualny sprzêg
   FoundedObj->MoverParameters->Attach(CouplFound,MyCouplFound,this->MoverParameters,ctrain_virtual);
   if (CouplFound==0) //jeœli widoczny sprzêg 0 znalezionego
   {
    if (DebugModeFlag)
    if (FoundedObj->PrevConnected)
     if (FoundedObj->PrevConnected!=this)
      WriteLog("1! Coupler warning on "+asName+":"+AnsiString(MyCouplFound)+" - "+FoundedObj->asName+":0 connected to "+FoundedObj->PrevConnected->asName+":"+AnsiString(FoundedObj->PrevConnectedNo));
    FoundedObj->PrevConnected=this;
    FoundedObj->PrevConnectedNo=MyCouplFound;
   }
   else //jeœli widoczny sprzêg 1 znalezionego
   {
    if (DebugModeFlag)
    if (FoundedObj->NextConnected)
     if (FoundedObj->NextConnected!=this)
      WriteLog("1! Coupler warning on "+asName+":"+AnsiString(MyCouplFound)+" - "+FoundedObj->asName+":1 connected to "+FoundedObj->NextConnected->asName+":"+AnsiString(FoundedObj->NextConnectedNo));
    FoundedObj->NextConnected=this;
    FoundedObj->NextConnectedNo=MyCouplFound;
   }
  }
  //Ra: jeœli dwa samochody siê mijaj¹ na odcinku przed zawrotk¹, to odleg³oœæ miêdzy nimi nie mo¿e byæ liczona w linii prostej!
  fTrackBlock=MoverParameters->Couplers[MyCouplFound].CoupleDist; //odleg³oœæ do najbli¿szego pojazdu w linii prostej
  if (Track->iCategoryFlag>1) //jeœli samochód
   if (ActDist>MoverParameters->Dim.L+FoundedObj->MoverParameters->Dim.L) //przeskanowana odleg³oœæ wiêksza od d³ugoœci pojazdów
  //else if (ActDist<ScanDist) //dla samochodów musi byæ uwzglêdniona droga do zawrócenia
    fTrackBlock=ActDist; //ta odleg³oœæ jest wiecej warta
  //if (fTrackBlock<500.0)
  // WriteLog("Collision of "+AnsiString(fTrackBlock)+"m detected by "+asName+":"+AnsiString(MyCouplFound)+" with "+FoundedObj->asName);
 }
 else //nic nie znalezione, to nie ma przeszkód
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
 NextConnectedNo=PrevConnectedNo=2; //ABu: Numery sprzegow. 2=nie pod³¹czony
 CouplCounter=50; //bêdzie sprawdzaæ na pocz¹tku
 asName="";
 bEnabled=true;
 MyTrack=NULL;
 //McZapkie-260202
 dRailLength=25.0;
 for (int i=0;i<MaxAxles;i++)
  dRailPosition[i]=0.0;
 for (int i=0;i<MaxAxles;i++)
  dWheelsPosition[i]=0.0; //bêdzie wczytane z MMD
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
 //iAnimatedAxles=0; //iloœæ obracanych osi
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
 iAlpha=0x30300030; //tak gdy tekstury wymienne nie maj¹ przezroczystoœci
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
 iDirection=1; //stoi w kierunku tradycyjnym (0, gdy jest odwrócony)
 iAxleFirst=0; //numer pierwszej osi w kierunku ruchu (prze³¹czenie nastêpuje, gdy osie sa na tym samym torze)
 iInventory=0; //flagi bitowe posiadanych submodeli (zaktualizuje siê po wczytaniu MMD)
 RaLightsSet(0,0); //pocz¹tkowe zerowanie stanu œwiate³
 //Ra: domyœlne iloœci animacji dla zgodnoœci wstecz (gdy brak iloœci podanych w MMD)
 iAnimType[ANIM_WHEELS ]=8; //0-osie (8)
 iAnimType[ANIM_DOORS  ]=8; //1-drzwi (8)
 iAnimType[ANIM_LEVERS ]=4; //2-wahacze (4) - np. nogi konia
 iAnimType[ANIM_BUFFERS]=4; //3-zderzaki (4)
 iAnimType[ANIM_BOOGIES]=2; //4-wózki (2)
 iAnimType[ANIM_PANTS  ]=2; //5-pantografy (2)
 iAnimType[ANIM_STEAMS ]=0; //6-t³oki (napêd parowozu)
 iAnimations=0; //na razie nie ma ¿adnego
 pAnimations=NULL;
 pAnimated=NULL;
 fShade=0.0; //standardowe oœwietlenie na starcie
 iHornWarning=1; //numer syreny do u¿ycia po otrzymaniu sygna³u do jazdy
 asDestination="none"; //stoj¹cy nigdzie nie jedzie
 pValveGear=NULL; //Ra: tymczasowo
 iCabs=0; //maski bitowe modeli kabin
 smBrakeSet=NULL; //nastawa hamulca (wajcha)
 smLoadSet=NULL; //nastawa ³adunku (wajcha)
 smWiper=NULL; //wycieraczka (poniek¹d te¿ wajcha)
}

__fastcall TDynamicObject::~TDynamicObject()
{//McZapkie-250302 - zamykanie logowania parametrow fizycznych
 SafeDelete(Mechanik);
 SafeDelete(MoverParameters);
 //Ra: wy³¹czanie dŸwiêków powinno byæ dodane w ich destruktorach, ale siê sypie
/* to te¿ siê sypie
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
 delete[] pAnimations; //obiekty obs³uguj¹ce animacjê
 delete[] pAnimated; //lista animowanych submodeli
}

double __fastcall TDynamicObject::Init(
 AnsiString Name, //nazwa pojazdu, np. "EU07-424"
 AnsiString BaseDir, //z którego katalogu wczytany, np. "PKP/EU07"
 AnsiString asReplacableSkin, //nazwa wymiennej tekstury
 AnsiString Type_Name, //nazwa CHK/MMD, np. "303E"
 TTrack *Track, //tor pocz¹tkowy wstwawienia (pocz¹tek sk³adu)
 double fDist, //dystans wzglêdem punktu 1
 AnsiString DriverType, //typ obsady
 double fVel, //prêdkoœæ pocz¹tkowa
 AnsiString TrainName, //nazwa sk³adu, np. "PE2307"
 float Load, //iloœæ ³adunku
 AnsiString LoadType, //nazwa ³adunku
 bool Reversed, //true, jeœli ma staæ odwrotnie w sk³adzie
 AnsiString MoreParams //dodatkowe parametry wczytywane w postaci tekstowej
)
{//Ustawienie pocz¹tkowe pojazdu
 iDirection=(Reversed?0:1); //Ra: 0, jeœli ma byæ wstawiony jako obrócony ty³em
 asBaseDir="dynamic\\"+BaseDir+"\\"; //McZapkie-310302
 asName=Name;
 AnsiString asAnimName=""; //zmienna robocza do wyszukiwania osi i wózków
 //Ra: zmieniamy znaczenie obsady na jednoliterowe, ¿eby dosadziæ kierownika
 if (DriverType=="headdriver") DriverType="h"; //steruj¹cy kabin¹ +1
 else if (DriverType=="reardriver") DriverType="r"; //steruj¹cy kabin¹ -1
 //else if (DriverType=="connected") DriverType="c"; //tego trzeba siê pozbyæ na rzecz ukrotnienia
 else if (DriverType=="passenger") DriverType="p"; //to do przemyœlenia
 else if (DriverType=="nobody") DriverType=""; //nikt nie siedzi
 int Cab; //numer kabiny z obsad¹ (nie mo¿na zaj¹æ obu)
 if (DriverType.Pos("h")||DriverType.Pos("1")) //od przodu sk³adu
  Cab=1;//iDirection?1:-1; //iDirection=1 gdy normalnie, =0 odwrotnie
 else if (DriverType.Pos("r")||DriverType.Pos("2")) //od ty³u sk³adu
  Cab=-1;//iDirection?-1:1;
 //else if (DriverType=="c") //uaktywnianie wirtualnej kabiny
 // Cab=0; //iDirection?1:-1; //to przestawi steruj¹cy
 else if (DriverType=="p")
 {
  if (random(6)<3) Cab=1; else Cab=-1; //losowy przydzia³ kabiny
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
 //utworzenie parametrów fizyki
 MoverParameters=new TMoverParameters(iDirection?fVel:-fVel,Type_Name,asName,Load,LoadType,Cab);
 iLights=MoverParameters->iLights; //wska¿nik na stan w³asnych œwiate³ (zmienimy dla rozrz¹dczych EZT)
 //McZapkie: TypeName musi byc nazw¹ CHK/MMD pojazdu
 if (!MoverParameters->LoadChkFile(asBaseDir))
 {//jak wczytanie CHK siê nie uda, to b³¹d
  if (ConversionError==-8)
   ErrorLog("Missed file: "+BaseDir+"\\"+Type_Name);
  Error("Cannot load dynamic object "+asName+" from:\r\n"+BaseDir+"\\"+Type_Name+"\r\nError "+ConversionError+" in line "+LineCount);
  return 0.0; //zerowa d³ugoœæ to brak pojazdu
 }
 bool driveractive=(fVel!=0.0); //jeœli prêdkoœæ niezerowa, to aktywujemy ruch
 if (!MoverParameters->CheckLocomotiveParameters(driveractive,(fVel>0?1:-1)*Cab*(iDirection?1:-1))) //jak jedzie lub obsadzony to gotowy do drogi
 {
  Error("Parameters mismatch: dynamic object "+asName+" from\n"+BaseDir+"\\"+Type_Name);
  return 0.0; //zerowa d³ugoœæ to brak pojazdu
 }
 MoverParameters->BrakeLevelSet(MoverParameters->BrakeCtrlPos); //poprawienie hamulca po ewentualnym przestawieniu przez Pascal

//dodatkowe parametry yB
 MoreParams+="."; //wykonuje o jedn¹ iteracjê za ma³o, wiêc trzeba mu dodaæ kropkê na koniec
 int kropka=MoreParams.Pos("."); //znajdŸ kropke
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
   if (ActPar.Pos("H")>0) //ladowny I (dla P-£ dalej prozny)
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

 if (MoverParameters->CategoryFlag&2) //jeœli samochód
 {//ustawianie samochodow na poboczu albo na œrodku drogi
  if (Track->fTrackWidth<3.5) //jeœli droga w¹ska
   MoverParameters->OffsetTrackH=0.0; //to stawiamy na œrodku, niezale¿nie od stanu ruchu
  else
  if (driveractive) //od 3.5m do 8.0m jedzie po œrodku pasa, dla szerszych w odleg³oœci 1.5m
   MoverParameters->OffsetTrackH=Track->fTrackWidth<=8.0?-Track->fTrackWidth*0.25:-1.5;
  else //jak stoi, to ko³em na poboczu i pobieramy szerokoœæ razem z poboczem, ale nie z chodnikiem
   MoverParameters->OffsetTrackH=-0.5*(Track->WidthTotal()-MoverParameters->Dim.W)+0.05;
  iHornWarning=0; //nie bêdzie tr¹bienia po podaniu zezwolenia na jazdê
 }
 //w wagonie tez niech jedzie
 //if (MoverParameters->MainCtrlPosNo>0 &&
 // if (MoverParameters->CabNo!=0)
 if (DriverType!="")
 {//McZapkie-040602: jeœli coœ siedzi w pojeŸdzie
  if (Name==AnsiString(Global::asHumanCtrlVehicle)) //jeœli pojazd wybrany do prowadzenia
  {
   if (DebugModeFlag?false:MoverParameters->EngineType!=Dumb) //jak nie Debugmode i nie jest dumbem
    Controller=Humandriver; //wsadzamy tam steruj¹cego
   else //w przeciwnym razie trzeba w³¹czyæ pokazywanie kabiny
    bDisplayCab=true;
  }
  //McZapkie-151102: rozk³ad jazdy czytany z pliku *.txt z katalogu w którym jest sceneria
  if (DriverType.Pos("h")||DriverType.Pos("r"))
  {//McZapkie-110303: mechanik i rozklad tylko gdy jest obsada
   //MoverParameters->ActiveCab=MoverParameters->CabNo; //ustalenie aktywnej kabiny (rozrz¹d)
   Mechanik=new TController(Controller,this,Aggressive);
   if (TrainName.IsEmpty()) //jeœli nie w sk³adzie
   {
    Mechanik->DirectionInitial();  //za³¹czenie rozrz¹du (wirtualne kabiny) itd.
    Mechanik->PutCommand("Timetable:",iDirection?-fVel:fVel,0,NULL); //tryb poci¹gowy z ustalon¹ prêdkoœci¹ (wzglêdem sprzêgów)
   }
   //if (TrainName!="none")
   // Mechanik->PutCommand("Timetable:"+TrainName,fVel,0,NULL);
  }
  else
   if (DriverType=="p")
   {//obserwator w charakterze pasa¿era
    //Ra: to jest niebezpieczne, bo w razie co bêdzie pomaga³ hamulcem bezpieczeñstwa
    Mechanik=new TController(Controller,this,Easyman,false);
   }
 }
 // McZapkie-250202
 iAxles=(MaxAxles<MoverParameters->NAxles)?MaxAxles:MoverParameters->NAxles; //iloœæ osi
 //wczytywanie z pliku nazwatypu.mmd, w tym model
 LoadMMediaFile(asBaseDir,Type_Name,asReplacableSkin);
 //McZapkie-100402: wyszukiwanie submodeli sprzegów
 btCoupler1.Init("coupler1",mdModel,false); //false - ma byæ wy³¹czony
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
 iInventory|=btEndSignals11.Active()  ?0x01:0; //informacja, czy ma poszczególne œwiat³a
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
 btHeadSignals12.Init("headlamp11",mdModel,false); //górne
 btHeadSignals13.Init("headlamp12",mdModel,false); //prawe
 btHeadSignals21.Init("headlamp23",mdModel,false);
 btHeadSignals22.Init("headlamp21",mdModel,false);
 btHeadSignals23.Init("headlamp22",mdModel,false);
 TurnOff(); //resetowanie zmiennych submodeli
 //wyszukiwanie zderzakow
 if (mdModel) //jeœli ma w czym szukaæ
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
 for (int i=0;i<iAxles;i++) //wyszukiwanie osi (0 jest na koñcu, dlatego dodajemy d³ugoœæ?)
  dRailPosition[i]=(Reversed?-dWheelsPosition[i]:(dWheelsPosition[i]+MoverParameters->Dim.L))+fDist;
 //McZapkie-250202 end.
 Track->AddDynamicObject(this); //wstawiamy do toru na pozycjê 0, a potem przesuniemy
 //McZapkie: zmieniono na ilosc osi brane z chk
 //iNumAxles=(MoverParameters->NAxles>3 ? 4 : 2 );
 iNumAxles=2;
 //McZapkie-090402: odleglosc miedzy czopami skretu lub osiami
 fAxleDist=Max0R(MoverParameters->BDist,MoverParameters->ADist);
 if (fAxleDist>MoverParameters->Dim.L-0.2) //nie mog¹ byæ za daleko
  fAxleDist=MoverParameters->Dim.L-0.2; //bo bêdzie "walenie w mur"
 float fAxleDistHalf=fAxleDist*0.5;
 //WriteLog("Dynamic "+Type_Name+" of length "+MoverParameters->Dim.L+" at "+AnsiString(fDist));
 //if (Cab) //jeœli ma obsadê - zgodnoœæ wstecz, jeœli tor startowy ma Event0
 // if (Track->Event0) //jeœli tor ma Event0
 //  if (fDist>=0.0) //jeœli jeœli w starych sceneriach pocz¹tek sk³adu by³by wysuniêty na ten tor
 //   if (fDist<=0.5*MoverParameters->Dim.L+0.2) //ale nie jest wysuniêty
 //    fDist+=0.5*MoverParameters->Dim.L+0.2; //wysun¹æ go na ten tor
 //przesuwanie pojazdu tak, aby jego pocz¹tek by³ we wskazanym miejcu
 fDist-=0.5*MoverParameters->Dim.L; //dodajemy pó³ d³ugoœci pojazdu, bo ustawiamy jego œrodek (zliczanie na minus)
 switch (iNumAxles)
 {//Ra: pojazdy wstawiane s¹ na tor pocz¹tkowy, a potem przesuwane
  case 2: //ustawianie osi na torze
   Axle0.Init(Track,this,iDirection?1:-1);
   Axle0.Move((iDirection?fDist:-fDist)+fAxleDistHalf+0.01,false);
   Axle1.Init(Track,this,iDirection?1:-1);
   Axle1.Move((iDirection?fDist:-fDist)-fAxleDistHalf-0.01,false); //false, ¿eby nie generowaæ eventów
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
 Move(0.0001); //potrzebne do wyliczenia aktualnej pozycji; nie mo¿e byæ zero, bo nie przeliczy pozycji
 //teraz jeszcze trzeba przypisaæ pojazdy do nowego toru, bo przesuwanie pocz¹tkowe osi nie zrobi³o tego
 ABuCheckMyTrack(); //zmiana toru na ten, co oœ Axle0 (oœ z przodu)
 TLocation loc; //Ra: ustawienie pozycji do obliczania sprzêgów
 loc.X=-vPosition.x;
 loc.Y=vPosition.z;
 loc.Z=vPosition.y;
 MoverParameters->Loc=loc; //normalnie przesuwa ComputeMovement() w Update()
 //pOldPos4=Axle1.pPosition; //Ra: nie u¿ywane
 //pOldPos1=Axle0.pPosition;
 //ActualTrack= GetTrack(); //McZapkie-030303
 //ABuWozki 060504
 if (mdModel) //jeœli ma w czym szukaæ
 {
  smBogie[0]=mdModel->GetFromName("bogie1"); //Ra: bo nazwy s¹ ma³ymi
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
 /*Ra: to nie dzia³a - Event0 musi byæ wykonywany ci¹gle
 if (fVel==0.0) //jeœli stoi
  if (MoverParameters->CabNo!=0) //i ma kogoœ w kabinie
   if (Track->Event0) //a jest w tym torze event od stania
    RaAxleEvent(Track->Event0); //dodanie eventu stania do kolejki
 */
 return MoverParameters->Dim.L; //d³ugoœæ wiêksza od zera oznacza OK; 2mm docisku?
}

void __fastcall TDynamicObject::FastMove(double fDistance)
{
 MoverParameters->dMoveLen=MoverParameters->dMoveLen+fDistance;
}

void __fastcall TDynamicObject::Move(double fDistance)
{//przesuwanie pojazdu po trajektorii polega na przesuwaniu poszczególnych osi
 //Ra: wartoœæ prêdkoœci 2km/h ma ograniczyæ aktywacjê eventów w przypadku drgañ
 if (Axle0.GetTrack()==Axle1.GetTrack()) //przed przesuniêciem
 {//powi¹zanie pojazdu z osi¹ mo¿na zmieniæ tylko wtedy, gdy skrajne osie s¹ na tym samym torze
  if (MoverParameters->Vel>2) //|[km/h]| nie ma sensu zmiana osi, jesli pojazd drga na postoju
   iAxleFirst=(MoverParameters->V>=0.0)?1:0; //[m/s] ?1:0 - aktywna druga oœ w kierunku jazdy
  //aktualnie eventy aktywuje druga oœ, ¿eby AI nie wy³¹cza³o sobie semafora za szybko
 }
 bEnabled&=Axle0.Move(fDistance,!iAxleFirst); //oœ z przodu pojazdu
 bEnabled&=Axle1.Move(fDistance,iAxleFirst); //oœ z ty³u pojazdu
 //Axle2.Move(fDistance,false); //te nigdy pierwsze nie s¹
 //Axle3.Move(fDistance,false);
 if (fDistance!=0.0) //nie liczyæ ponownie, jeœli stoi
 {//liczenie pozycji pojazdu tutaj, bo jest u¿ywane w wielu miejscach
  vPosition=0.5*(Axle1.pPosition+Axle0.pPosition); //œrodek miêdzy skrajnymi osiami
  vFront=Normalize(Axle0.pPosition-Axle1.pPosition); //kierunek ustawienia pojazdu (wektor jednostkowy)
  vLeft=Normalize(CrossProduct(vWorldUp,vFront)); //wektor poziomy w lewo, normalizacja potrzebna z powodu pochylenia (vFront)
  vUp=CrossProduct(vFront,vLeft); //wektor w górê, bêdzie jednostkowy
  double a=((Axle1.GetRoll()+Axle0.GetRoll())); //suma przechy³ek
  if (a!=0.0)
  {//wyznaczanie przechylenia tylko jeœli jest przechy³ka
   //mo¿na by pobraæ wektory normalne z toru...
   mMatrix.Identity(); //ta macierz jest potrzebna g³ównie do wyœwietlania
   mMatrix.Rotation(a*0.5,vFront); //obrót wzd³u¿ osi o przechy³kê
   vUp=mMatrix*vUp; //wektor w górê pojazdu (przekrêcenie na przechy³ce)
   //vLeft=mMatrix*DynamicObject->vLeft;
   //vUp=CrossProduct(vFront,vLeft); //wektor w górê
   //vLeft=Normalize(CrossProduct(vWorldUp,vFront)); //wektor w lewo
   vLeft=Normalize(CrossProduct(vUp,vFront)); //wektor w lewo
   //vUp=CrossProduct(vFront,vLeft); //wektor w górê
  }
  mMatrix.Identity(); //to te¿ mo¿na by od razu policzyæ, ale potrzebne jest do wyœwietlania
  mMatrix.BasisChange(vLeft,vUp,vFront); //przesuwanie jest jednak rzadziej ni¿ renderowanie
  mMatrix=Inverse(mMatrix); //wyliczenie macierzy dla pojazdu (potrzebna tylko do wyœwietlania?)
  //if (MoverParameters->CategoryFlag&2)
  {//przesuniêcia s¹ u¿ywane po wyrzuceniu poci¹gu z toru
   vPosition.x+=MoverParameters->OffsetTrackH*vLeft.x; //dodanie przesuniêcia w bok
   vPosition.z+=MoverParameters->OffsetTrackH*vLeft.z; //vLeft jest wektorem poprzecznym
   //if () na przechy³ce bêdzie dodatkowo zmiana wysokoœci samochodu
   vPosition.y+=MoverParameters->OffsetTrackV;   //te offsety s¹ liczone przez moverparam
  }
  //Ra: skopiowanie pozycji do fizyki, tam potrzebna do zrywania sprzêgów
  //MoverParameters->Loc.X=-vPosition.x; //robi to {Fast}ComputeMovement()
  //MoverParameters->Loc.Y= vPosition.z;
  //MoverParameters->Loc.Z= vPosition.y;
  //obliczanie pozycji sprzêgów do liczenia zderzeñ
  vector3 dir=(0.5*MoverParameters->Dim.L)*vFront; //wektor sprzêgu
  vCoulpler[0]=vPosition+dir; //wspó³rzêdne sprzêgu na pocz¹tku
  vCoulpler[1]=vPosition-dir; //wspó³rzêdne sprzêgu na koñcu
  MoverParameters->vCoulpler[0]=vCoulpler[0]; //tymczasowo kopiowane na inny poziom
  MoverParameters->vCoulpler[1]=vCoulpler[1];
  //bCameraNear=
  //if (bCameraNear) //jeœli istotne s¹ szczegó³y (blisko kamery)
  {//przeliczenie cienia
   TTrack *t0=Axle0.GetTrack(); //ju¿ po przesuniêciu
   TTrack *t1=Axle1.GetTrack();
   if ((t0->eEnvironment==e_flat)&&(t1->eEnvironment==e_flat)) //mo¿e byæ e_bridge...
    fShade=0.0; //standardowe oœwietlenie
   else
   {//je¿eli te tory maj¹ niestandardowy stopieñ zacienienia (e_canyon, e_tunnel)
    if (t0->eEnvironment==t1->eEnvironment)
    {switch (t0->eEnvironment)
     {//typ zmiany oœwietlenia
      case e_canyon: fShade=0.65; break; //zacienienie w kanionie
      case e_tunnel: fShade=0.20; break; //zacienienie w tunelu
     }
    }
    else //dwa ró¿ne
    {//liczymy proporcjê
     double d=Axle0.GetTranslation(); //aktualne po³o¿enie na torze
     if (Axle0.GetDirection()<0)
      d=t0->fTrackLength-d; //od drugiej strony liczona d³ugoœæ
     d/=fAxleDist; //rozsataw osi procentowe znajdowanie siê na torze
     switch (t0->eEnvironment)
     {//typ zmiany oœwietlenia - zak³adam, ¿e drugi tor ma e_flat
      case e_canyon: fShade=(d*0.65)+(1.0-d); break; //zacienienie w kanionie
      case e_tunnel: fShade=(d*0.20)+(1.0-d); break; //zacienienie w tunelu
     }
     switch (t1->eEnvironment)
     {//typ zmiany oœwietlenia - zak³adam, ¿e pierwszy tor ma e_flat
      case e_canyon: fShade=d+(1.0-d)*0.65; break; //zacienienie w kanionie
      case e_tunnel: fShade=d+(1.0-d)*0.20; break; //zacienienie w tunelu
     }
    }
   }
  }
 }
};

void __fastcall TDynamicObject::AttachPrev(TDynamicObject *Object, int iType)
{//Ra: doczepia Object na koñcu sk³adu (nazwa funkcji mo¿e byæ myl¹ca)
 //Ra: u¿ywane tylko przy wczytywaniu scenerii
 /*
 //Ra: po wstawieniu pojazdu do scenerii nie mia³ on ustawionej pozycji, teraz ju¿ ma
 TLocation loc;
 loc.X=-vPosition.x;
 loc.Y=vPosition.z;
 loc.Z=vPosition.y;
 MoverParameters->Loc=loc; //Ra: do obliczania sprzêgów, na starcie nie s¹ przesuniête
 loc.X=-Object->vPosition.x;
 loc.Y=Object->vPosition.z;
 loc.Z=Object->vPosition.y;
 Object->MoverParameters->Loc=loc; //ustawienie dodawanego pojazdu
 */
 MoverParameters->Attach(iDirection,Object->iDirection^1,Object->MoverParameters,iType,true);
 MoverParameters->Couplers[iDirection].Render=false;
 Object->MoverParameters->Attach(Object->iDirection^1,iDirection,MoverParameters,iType,true);
 Object->MoverParameters->Couplers[Object->iDirection^1].Render=true; //rysowanie sprzêgu w do³¹czanym
 if (iDirection)
 {//³¹czenie standardowe
  NextConnected=Object; //normalnie doczepiamy go sobie do sprzêgu 1
  NextConnectedNo=Object->iDirection^1;
 }
 else
 {//³¹czenie odwrotne
  PrevConnected=Object; //doczepiamy go sobie do sprzêgu 0, gdy stoimy odwrotnie
  PrevConnectedNo=Object->iDirection^1;
 }
 if (Object->iDirection)
 {//do³¹czany jest normalnie ustawiany
  Object->PrevConnected=this; //on ma nas z przodu
  Object->PrevConnectedNo=iDirection;
 }
 else
 {//do³¹czany jest odwrotnie ustawiany
  Object->NextConnected=this; //on ma nas z ty³u
  Object->NextConnectedNo=iDirection;
 }
 if (MoverParameters->TrainType&dt_EZT) //w przypadku ³¹czenia cz³onów, œwiat³a w rozrz¹dczym zale¿¹ od stanu w silnikowym
  if (MoverParameters->Couplers[iDirection].AllowedFlag&ctrain_depot) //gdy sprzêgi ³¹czone warsztatowo (powiedzmy)
   if ((MoverParameters->Power<1.0)&&(Object->MoverParameters->Power>1.0)) //my nie mamy mocy, ale ten drugi ma
    iLights=Object->MoverParameters->iLights; //to w tym z moc¹ bêd¹ œwiat³a za³¹czane, a w tym bez tylko widoczne
   else if ((MoverParameters->Power>1.0)&&(Object->MoverParameters->Power<1.0)) //my mamy moc, ale ten drugi nie ma
    Object->iLights=MoverParameters->iLights; //to w tym z moc¹ bêd¹ œwiat³a za³¹czane, a w tym bez tylko widoczne
 return;
 //SetPneumatic(1,1); //Ra: to i tak siê nie wykonywa³o po return
 //SetPneumatic(1,0);
 //SetPneumatic(0,1);
 //SetPneumatic(0,0);
}

bool __fastcall TDynamicObject::UpdateForce(double dt, double dt1, bool FullVer)
{
 if (!bEnabled)
  return false;
 if (dt>0)
  MoverParameters->ComputeTotalForce(dt,dt1,FullVer); //wywalenie WS zale¿y od ustawienia kierunku
 return true;
}

void __fastcall TDynamicObject::LoadUpdate()
{//prze³adowanie modelu ³adunku
 // Ra: nie próbujemy wczytywaæ modeli miliony razy podczas renderowania!!!
 if ((mdLoad==NULL)&&(MoverParameters->Load>0))
 {
  AnsiString asLoadName=asBaseDir+MoverParameters->LoadType+".t3d"; //zapamiêtany katalog pojazdu
  //asLoadName=MoverParameters->LoadType;
  //if (MoverParameters->LoadType!=AnsiString("passengers"))
  Global::asCurrentTexturePath=asBaseDir; //bie¿¹ca œcie¿ka do tekstur to dynamic/...
  mdLoad=TModelsManager::GetModel(asLoadName.c_str()); //nowy ³adunek
  Global::asCurrentTexturePath=AnsiString(szTexturePath); //z powrotem defaultowa sciezka do tekstur
  //Ra: w MMD mo¿na by zapisaæ po³o¿enie modelu ³adunku (np. wêgiel) w zale¿noœci od za³adowania
 }
 else if (MoverParameters->Load==0)
  mdLoad=NULL; //nie ma ³adunku
 //if ((mdLoad==NULL)&&(MoverParameters->Load>0))
 // {
 //  mdLoad=NULL; //Ra: to jest tu bez sensu - co autor mia³ na myœli?
 // }
 MoverParameters->LoadStatus&=3; //po zakoñczeniu bêdzie równe zero
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
  Powinny byæ dwie funkcje wykonuj¹ce aktualizacjê fizyki. Jedna wykonuj¹ca
krok obliczeñ, powtarzana odpowiedni¹ liczbê razy, a druga wykonuj¹ca zbiorcz¹
aktualzacjê mniej istotnych elementów.
  Ponadto nale¿a³o by ustaliæ odleg³oœæ sk³adów od kamery i jeœli przekracza
ona np. 10km, to traktowaæ sk³ady jako uproszczone, np. bez wnikania w si³y
na sprzêgach, opóŸnienie dzia³ania hamulca itp. Oczywiœcie musi mieæ to pewn¹
histerezê czasow¹, aby te tryby pracy nie prze³¹cza³y siê zbyt szybko.
*/


bool __fastcall TDynamicObject::Update(double dt, double dt1)
{
 if (dt==0) return true; //Ra: pauza
 if (!MoverParameters->PhysicActivation&&!MechInside) //to drugie, bo bêd¹c w maszynowym blokuje siê fizyka
  return true;   //McZapkie: wylaczanie fizyki gdy nie potrzeba
 if (!MyTrack)
  return false; //pojazdy postawione na torach portalowych maj¹ MyTrack==NULL
 if (!bEnabled)
  return false; //a normalnie powinny mieæ bEnabled==false

//Ra: przenios³em - no ju¿ lepiej tu, ni¿ w wyœwietlaniu!
//if ((MoverParameters->ConverterFlag==false) && (MoverParameters->TrainType!=dt_ET22))
//Ra: to nie mo¿e tu byæ, bo wy³¹cza sprê¿arkê w rozrz¹dczym EZT!
//if ((MoverParameters->ConverterFlag==false)&&(MoverParameters->CompressorPower!=0))
// MoverParameters->CompressorFlag=false;
//if (MoverParameters->CompressorPower==2)
// MoverParameters->CompressorAllow=MoverParameters->ConverterFlag;

 //McZapkie-260202
 if ((MoverParameters->EnginePowerSource.SourceType==CurrentCollector)&&(MoverParameters->Power>1.0)) //aby rozrz¹dczy nie opuszcza³ silnikowemu
  if ((MechInside)||(MoverParameters->TrainType==dt_EZT))
  {
   //if ((!MoverParameters->PantCompFlag)&&(MoverParameters->CompressedVolume>=2.8))
   // MoverParameters->PantVolume=MoverParameters->CompressedVolume;
   if (MoverParameters->PantPress<(MoverParameters->TrainType==dt_EZT?2.4:3.5))
   {// 3.5 wg http://www.transportszynowy.pl/eu06-07pneumat.php
    //"Wy³¹czniki ciœnieniowe odbieraków pr¹du wy³¹czaj¹ sterowanie wy³¹cznika szybkiego oraz uniemo¿liwiaj¹ podniesienie odbieraków pr¹du, gdy w instalacji rozrz¹du ciœnienie spadnie poni¿ej wartoœci 3,5 bara."
    //Ra 2013-12: Niebugoc³aw mówi, ¿e w EZT podnosz¹ siê przy 2.5
    //if (!MoverParameters->PantCompFlag)
    // MoverParameters->PantVolume=MoverParameters->CompressedVolume;
    MoverParameters->PantFront(false); //opuszczenie pantografów przy niskim ciœnieniu
    MoverParameters->PantRear(false); //to idzie w ukrotnieniu, a nie powinno...
   }
   //Winger - automatyczne wylaczanie malej sprezarki
   else if (MoverParameters->PantPress>=4.8)
    MoverParameters->PantCompFlag=false;
  } //Ra: do Mover to trzeba przenieœæ, ¿eby AI te¿ mog³o sobie podpompowaæ

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
    if (bogieRot[0].z!=bogieRot[1].z) //wyliczenie promienia z obrotów osi - modyfikacjê zg³osi³ youBy
      ts.R=0.5*MoverParameters->BDist/sin(DegToRad(bogieRot[0].z-bogieRot[1].z)*0.5);
    //ts.R=ComputeRadius(Axle1.pPosition,Axle2.pPosition,Axle3.pPosition,Axle0.pPosition);
    //Ra: sk³adow¹ pochylenia wzd³u¿nego mamy policzon¹ w jednostkowym wektorze vFront
    ts.Len=1.0; //Max0R(MoverParameters->BDist,MoverParameters->ADist);
    ts.dHtrack=-vFront.y; //Axle1.pPosition.y-Axle0.pPosition.y; //wektor miêdzy skrajnymi osiami (!!!odwrotny)
    ts.dHrail=(Axle1.GetRoll()+Axle0.GetRoll())*0.5; //œrednia przechy³ka pud³a
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
    {//Ra 2013-12: to ni¿ej jest chyba trochê bez sensu
     double v=MoverParameters->PantRearVolt;
     if (v==0.0)
     {v=MoverParameters->PantFrontVolt;
      if (v==0.0)
       if (MoverParameters->TrainType&(dt_EZT|dt_ET40|dt_ET41|dt_ET42)) //dwucz³ony mog¹ mieæ sprzêg WN
        v=MoverParameters->GetTrainsetVoltage(); //ostatnia szansa
     }
     if (v!=0.0)
     {//jeœli jest zasilanie
      NoVoltTime=0;
      tmpTraction.TractionVoltage=v;
     }
     else
     {NoVoltTime=NoVoltTime+dt;
      if (NoVoltTime>0.3) //jeœli brak zasilania d³u¿ej ni¿ przez 1 sekundê
       tmpTraction.TractionVoltage=0; //Ra 2013-12: po co tak?
       //pControlled->MainSwitch(false); //mo¿e tak?
     }
    }
    else
     tmpTraction.TractionVoltage=0.95*MoverParameters->EnginePowerSource.MaxVoltage;
   }
   else
    tmpTraction.TractionVoltage=0.95*MoverParameters->EnginePowerSource.MaxVoltage;
   tmpTraction.TractionFreq=0;
   tmpTraction.TractionMaxCurrent=7500; //Ra: chyba za du¿o? powinno wywalaæ przy 1500
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
      Mechanik->UpdateSituation(dt1); //przeb³yski œwiadomoœci AI
      //Mechanik->PhysicsLog(); //tymczasowo logowanie
    }
//    else
//    { MoverParameters->SecuritySystemReset(); }
    //if (MoverParameters->ActiveCab==0)
    //    MoverParameters->SecuritySystemReset(); //Ra: to tu nie powinno byæ, czuwak za³¹czany jest bateri¹, ewentualnie rozrz¹dem
//    else
//     if ((Controller!=Humandriver)&&(MoverParameters->BrakeCtrlPos<0)&&(!TestFlag(MoverParameters->BrakeStatus,1))&&((MoverParameters->CntrlPipePress)>0.51))
//       {//Ra: to jest do poprawienia przy okazji SPKS
////        MoverParameters->PipePress=0.50;
//        MoverParameters->BrakeLevelSet(0); //Ra: co to mia³o byæ ???? to nie pozwala wyluzowaæ AI w EN57 !!!!

//       }

    //fragment "z EXE Kursa"
    if (MoverParameters->Mains) //nie wchodziæ w funkcjê bez potrzeby
     if ((!MoverParameters->Battery)&&(Controller==Humandriver)&&(MoverParameters->EngineType!=DieselEngine)&&(MoverParameters->EngineType!=WheelsDriven))
     {//jeœli bateria wy³¹czona, a nie diesel ani drezyna reczna
      if (MoverParameters->MainSwitch(false)) //wy³¹czyæ zasilanie
       MoverParameters->EventFlag=true;
     }
    if (MoverParameters->TrainType==dt_ET42)
    {//powinny byæ wszystkie dwucz³ony oraz EZT
/*
     //Ra: to jest bez sensu, bo wy³¹cza WS przy przechodzeniu przez "wewnêtrzne" kabiny (z powodu ActiveCab)
     //trzeba to zrobiæ inaczej, np. dla cz³onu A sprawdzaæ, czy jest B
     //albo sprawdzaæ w momencie za³¹czania WS i zmiany w sprzêgach
     if (((TestFlag(MoverParameters->Couplers[1].CouplingFlag,ctrain_controll))&&(MoverParameters->ActiveCab>0)&&(NextConnected->MoverParameters->TrainType!=dt_ET42))||((TestFlag(MoverParameters->Couplers[0].CouplingFlag,ctrain_controll))&&(MoverParameters->ActiveCab<0)&&(PrevConnected->MoverParameters->TrainType!=dt_ET42)))
     {//sprawdzenie, czy z ty³u kabiny mamy drugi cz³on
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
    MoverParameters->UpdateBatteryVoltage(dt); //jest ju¿ w Mover.cpp
    if (MoverParameters->EnginePowerSource.SourceType==CurrentCollector) //tylko jeœli pantografuj¹cy
     if (MoverParameters->Power>1.0) //w rozrz¹dczym nie (jest b³¹d w FIZ!)
      MoverParameters->UpdatePantVolume(dt); //Ra: pneumatyka pantografów przeniesiona do Mover.cpp!
//yB: zeby zawsze wrzucalo w jedna strone zakretu
    MoverParameters->AccN*=-ABuGetDirection();
    //if (dDOMoveLen!=0.0) //Ra: nie mo¿e byæ, bo blokuje Event0
     Move(dDOMoveLen);
    if (!bEnabled) //usuwane pojazdy nie maj¹ toru
    {//pojazd do usuniêcia
     Global::pGround->bDynamicRemove=true; //sprawdziæ
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
   //double freq; //Ra: nie u¿ywane
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
    {//dla ET40 i EU06 automatyczne cofanie nastawnika - i tak nie bêdzie to dzia³aæ dobrze...
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
  dWheelAngle[1]+=MoverParameters->nrot*dt1*360.0; //napêdne
  dWheelAngle[2]+=114.59155902616464175359630962821*MoverParameters->V*dt1/MoverParameters->WheelDiameterT; //tylne toczne
  if (dWheelAngle[0]>360.0) dWheelAngle[0]-=360.0; //a w drug¹ stronê jak siê krêc¹?
  if (dWheelAngle[1]>360.0) dWheelAngle[1]-=360.0;
  if (dWheelAngle[2]>360.0) dWheelAngle[2]-=360.0;
 }
 if (pants) //pantograf mo¿e byæ w wagonie kuchennym albo pojeŸdzie rewizyjnym (np. SR61)
 {//przeliczanie k¹tów dla pantografów
  double k; //tymczasowy k¹t
  double PantDiff;
  TAnimPant *p; //wskaŸnik do obiektu danych pantografu
  for (int i=0;i<iAnimType[ANIM_PANTS];++i)
  {//pêtla po wszystkich pantografach
   p=pants[i].fParamPants;
   if (p->PantWys<0)
   {//patograf zosta³ po³amany, liczony nie bêdzie
    if (p->fAngleL>p->fAngleL0)
     p->fAngleL-=0.2*dt1; //nieco szybciej ni¿ jak dla opuszczania
    if (p->fAngleL<p->fAngleL0)
     p->fAngleL=p->fAngleL0; //k¹t graniczny
    if (p->fAngleU<M_PI)
     p->fAngleU+=0.5*dt1; //górne siê musi ruszaæ szybciej.
    if (p->fAngleU>M_PI)
     p->fAngleU=M_PI;
    continue;
   }
   PantDiff=p->PantTraction-p->PantWys; //docelowy-aktualny
   switch (i) //numer pantografu
   {//trzeba usun¹æ to rozró¿nienie
    case 0:
     if (Global::bLiveTraction?false:!p->hvPowerWire) //jeœli nie ma drutu, mo¿e pooszukiwaæ
      MoverParameters->PantFrontVolt=(p->PantWys>=1.4)?0.95*MoverParameters->EnginePowerSource.MaxVoltage:0.0;
     else
      if (MoverParameters->PantFrontUp?(PantDiff<0.01):false)
      {
       if ((MoverParameters->PantFrontVolt==0.0)&&(MoverParameters->PantRearVolt==0.0))
        sPantUp.Play(vol,0,MechInside,vPosition);
       if (p->hvPowerWire) //TODO: wyliczyæ trzeba pr¹d przypadaj¹cy na pantograf i wstawiæ do GetVoltage()
        MoverParameters->PantFrontVolt=p->hvPowerWire->psPower?p->hvPowerWire->psPower->GetVoltage(0):p->hvPowerWire->NominalVoltage;
       else
        MoverParameters->PantFrontVolt=0.0;
      }
      else
       MoverParameters->PantFrontVolt=0.0;
    break;
    case 1:
     if (Global::bLiveTraction?false:!p->hvPowerWire) //jeœli nie ma drutu, mo¿e pooszukiwaæ
      MoverParameters->PantRearVolt=(p->PantWys>=1.4)?0.95*MoverParameters->EnginePowerSource.MaxVoltage:0.0;
     else
      if (MoverParameters->PantRearUp?(PantDiff<0.01):false)
      {
       if ((MoverParameters->PantRearVolt==0.0)&&(MoverParameters->PantFrontVolt==0.0))
        sPantUp.Play(vol,0,MechInside,vPosition);
       if (p->hvPowerWire) //TODO: wyliczyæ trzeba pr¹d przypadaj¹cy na pantograf i wstawiæ do GetVoltage()
        MoverParameters->PantRearVolt=p->hvPowerWire->psPower?p->hvPowerWire->psPower->GetVoltage(0):p->hvPowerWire->NominalVoltage;
       else
        MoverParameters->PantRearVolt=0.0;
      }
      else
       MoverParameters->PantRearVolt=0.0;
    break;
   } //pozosta³e na razie nie obs³ugiwane
   if (MoverParameters->PantPress>(MoverParameters->TrainType==dt_EZT?2.5:3.3)) //Ra 2013-12: Niebugoc³aw mówi, ¿e w EZT podnosz¹ siê przy 2.5
    pantspeedfactor=0.015*(MoverParameters->PantPress)*dt1; //z EXE Kursa  //Ra: wysokoœæ zale¿y od ciœnienia !!!
   else
    pantspeedfactor=0.0;
   if (pantspeedfactor<0) pantspeedfactor=0;
   k=p->fAngleL;
   if (i?MoverParameters->PantRearUp:MoverParameters->PantFrontUp) //jeœli ma byæ podniesiony
   {if (PantDiff>0.001) //jeœli nie dolega do drutu
    {//jeœli poprzednia wysokoœæ jest mniejsza ni¿ po¿¹dana, zwiêkszyæ k¹t dolnego ramienia zgodnie z ciœnieniem
     if (pantspeedfactor>0.55*PantDiff) //0.55 to oko³o pochodna k¹ta po wysokoœci
      k+=0.55*PantDiff; //ograniczenie "skoku" w danej klatce
     else
      k+=pantspeedfactor; //dolne ramiê
     //jeœli przekroczono k¹t graniczny, zablokowaæ pantograf (wymaga interwencji poci¹gu sieciowego)
    }
    else if (PantDiff<-0.001)
    {//drut siê obni¿y³ albo zosta³ podniesiony za wysoko
     //jeœli wysokoœæ jest zbyt du¿a, wyznaczyæ zmniejszenie k¹ta
     //jeœli zmniejszenie k¹ta jest zbyt du¿e, przejœæ do trybu ³amania pantografu
     //if (PantFrontDiff<-0.05) //skok w dó³ o 5cm daje z³¹manie pantografu
     k+=0.4*PantDiff; //mniej ni¿ pochodna k¹ta po wysokoœci
    } //jeœli wysokoœæ jest dobra, nic wiêcej nie liczyæ
   }
   else
   {//jeœli ma byæ na dole
    if (k>p->fAngleL0) //jeœli wy¿ej ni¿ po³o¿enie wyjœciowe
     k-=0.15*dt1; //ruch w dó³
    if (k<p->fAngleL0)
     k=p->fAngleL0; //po³o¿enie minimalne
   }
   if (k!=p->fAngleL)
   {//¿eby nie liczyæ w kilku miejscach ani gdy nie potrzeba
    if (k+p->fAngleU<M_PI)
    {//o ile nie zosta³ osi¹gniêty k¹t maksymalny
     p->fAngleL=k; //zmieniony k¹t
     //wyliczyæ k¹t górnego ramienia z wzoru (a)cosinusowego
     //=acos((b*cos()+c)/a)
     //p->dPantAngleT=acos((1.22*cos(k)+0.535)/1.755); //górne ramiê
     p->fAngleU=acos((p->fLenL1*cos(k)+p->fHoriz)/p->fLenU1); //górne ramiê
     //wyliczyæ aktualn¹ wysokoœæ z wzoru sinusowego
     //h=a*sin()+b*sin()
     p->PantWys=p->fLenL1*sin(k)+p->fLenU1*sin(p->fAngleU)+p->fHeight; //wysokoœæ ca³oœci
    }
   }
  } //koniec pêtli po pantografach
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
   MoverParameters->CompressorFlag=false; //Ra: to jest w¹tpliwe - wy³¹czenie sprê¿arki powinno byæ w jednym miejscu!
  }
 }
 else if (MoverParameters->EnginePowerSource.SourceType==InternalSource)
  if (MoverParameters->EnginePowerSource.PowerType==SteamPower)
   //if (smPatykird1[0])
  {//Ra: animacja rozrz¹du parowozu, na razie nieoptymalizowane
/* //Ra: tymczasowo wy³¹czone ze wzglêdu na porz¹dkowanie animacji pantografów
   double fi,dx,c2,ka,kc;
   double sin_fi,cos_fi;
   double L1=1.6688888888888889;
   double L2=5.6666666666666667; //2550/450
   double Lc=0.4;
   double L=5.686422222; //2558.89/450
   double G1,G2,G3,ksi,sin_ksi,gam;
   double G1_2,G2_2,G3_2; //kwadraty
   //ruch t³oków oraz korbowodów
   for (int i=0;i<=1;++i)
   {//obie strony w ten sam sposób
    fi=DegToRad(dWheelAngle[1]+(i?pant2x:pant1x)); //k¹t obrotu ko³a dla t³oka 1
    sin_fi=sin(fi);
    cos_fi=cos(fi);
    dx=panty*cos_fi+sqrt(panth*panth-panty*panty*sin_fi*sin_fi)-panth; //nieoptymalne
    if (smPatykird1[i]) //na razie zabezpieczenie
     smPatykird1[i]->SetTranslate(float3(dx,0,0));
    ka=-asin(panty/panth)*sin_fi;
    if (smPatykirg1[i]) //na razie zabezpieczenie
     smPatykirg1[i]->SetRotateXYZ(vector3(RadToDeg(ka),0,0));
    //smPatykirg1[0]->SetRotate(float3(0,1,0),RadToDeg(fi)); //obracamy
    //ruch dr¹¿ka mimoœrodkowego oraz jarzma
    //korzysta³em z pliku PDF "mm.pdf" (opis czworoboku korbowo-wahaczowego):
    //"MECHANIKA MASZYN. Szkic wyk³adu i laboratorium komputerowego."
    //Prof. dr hab. in¿. Jerzy Zaj¹czkowski, 2007, Politechnika £ódzka
    //L1 - wysokoœæ (w pionie) osi jarzma ponad osi¹ ko³a
    //L2 - odleg³oœæ w poziomie osi jarzma od osi ko³a
    //Lc - d³ugoœæ korby mimoœrodu na kole
    //Lr - promieñ jarzma =1.0 (pozosta³e przeliczone proporcjonalnie)
    //L - d³ugoœæ dr¹¿ka mimoœrodowego
    //fi - k¹t obrotu ko³a
    //ksi - k¹t obrotu jarzma (od pionu)
    //gam - odchylenie dr¹¿ka mimoœrodowego od poziomu
    //G1=(Lr*Lr+L1*L1+L2*L2+Kc*Lc-L*L-2.0*Lc*L2*cos(fi)+2.0*Lc*L1*sin(fi))/(Lr*Lr);
    //G2=2.0*(L2-Lc*cos(fi))/Lr;
    //G3=2.0*(L1-Lc*sin(fi))/Lr;
    fi=DegToRad(dWheelAngle[1]+(i?pant2x:pant1x)-96.77416667); //k¹t obrotu ko³a dla t³oka 1
    //1) dla dWheelAngle[1]=0° korba jest w dó³, a mimoœród w stronê jarzma, czyli fi=-7°
    //2) dla dWheelAngle[1]=90° korba jest do ty³u, a mimoœród w dó³, czyli fi=83°
    sin_fi=sin(fi);
    cos_fi=cos(fi);
    G1=(1.0+L1*L1+L2*L2+Lc*Lc-L*L-2.0*Lc*L2*cos_fi+2.0*Lc*L1*sin_fi);
    G1_2=G1*G1;
    G2=2.0*(L2-Lc*cos_fi);
    G2_2=G2*G2;
    G3=2.0*(L1-Lc*sin_fi);
    G3_2=G3*G3;
    sin_ksi=(G1*G2-G3*_fm_sqrt(G2_2+G3_2-G1_2))/(G2_2+G3_2); //x1 (minus delta)
    ksi=asin(sin_ksi); //k¹t jarzma
    if (smPatykirg2[i])
     smPatykirg2[i]->SetRotateXYZ(vector3(RadToDeg(ksi),0,0)); //obrócenie jarzma
    //1) ksi=-23°, gam=
    //2) ksi=10°, gam=
    //gam=acos((L2-sin_ksi-Lc*cos_fi)/L); //k¹t od poziomu, liczony wzglêdem poziomu
    //gam=asin((L1-cos_ksi-Lc*sin_fi)/L); //k¹t od poziomu, liczony wzglêdem pionu
    gam=atan2((L1-cos(ksi)+Lc*sin_fi),(L2-sin_ksi+Lc*cos_fi)); //k¹t od poziomu
    if (smPatykird2[i]) //na razie zabezpieczenie
     smPatykird2[i]->SetRotateXYZ(vector3(RadToDeg(-gam-ksi),0,0)); //obrócenie dr¹¿ka mimoœrodowego
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
 //Z obserwacji: v>0 -> Coupler 0; v<0 ->coupler1 (Ra: prêdkoœæ jest zwi¹zana z pojazdem)
 //Rozroznienie jest tutaj, zeby niepotrzebnie nie skakac do funkcji. Nie jest uzaleznione
 //od obecnosci AI, zeby uwzglednic np. jadace bez lokomotywy wagony.
 //Ra: mo¿na by przenieœæ na poziom obiektu reprezentuj¹cego sk³ad, aby nie sprawdzaæ œrodkowych
 if (CouplCounter>25) //licznik, aby nie robiæ za ka¿dym razem
 {//poszukiwanie czegoœ do zderzenia siê
  fTrackBlock=10000.0; //na razie nie ma przeszkód (na wypadek nie uruchomienia skanowania)
  //jeœli nie ma zwrotnicy po drodze, to tylko przeliczyæ odleg³oœæ?
  if (MoverParameters->V>0.03) //[m/s] jeœli jedzie do przodu (w kierunku Coupler 0)
  {if (MoverParameters->Couplers[0].CouplingFlag==ctrain_virtual) //brak pojazdu podpiêtego?
   {ABuScanObjects(1,300); //szukanie czegoœ do pod³¹czenia
    //WriteLog(asName+" - block 0: "+AnsiString(fTrackBlock));
   }
  }
  else if (MoverParameters->V<-0.03) //[m/s] jeœli jedzie do ty³u (w kierunku Coupler 1)
   if (MoverParameters->Couplers[1].CouplingFlag==ctrain_virtual) //brak pojazdu podpiêtego?
   {ABuScanObjects(-1,300);
    //WriteLog(asName+" - block 1: "+AnsiString(fTrackBlock));
   }
  CouplCounter=random(20); //ponowne sprawdzenie po losowym czasie
 }
 if (MoverParameters->Vel>0.1) //[km/h]
  ++CouplCounter; //jazda sprzyja poszukiwaniu po³¹czenia
 else
 {CouplCounter=25; //a bezruch nie, ale trzeba zaktualizowaæ odleg³oœæ, bo zawalidroga mo¿e sobie pojechaæ
/*
  //if (Mechanik) //mo¿e byæ z drugiej strony sk³adu
  {//to poni¿ej jest istotne tylko dla AI, czekaj¹cego na zwolninie drogi
   if (MoverParameters->Couplers[1-iDirection].CouplingFlag==ctrain_virtual)
   {if ((MoverParameters->CategoryFlag&1)?MoverParameters->Couplers[1-iDirection].Connected!=NULL:false)
    {//jeœli jest pojazd kolejowy na sprzêgu wirtualnym - CoupleDist nieadekwatne dla samochodów!
     fTrackBlock=MoverParameters->Couplers[1-iDirection].CoupleDist; //aktualizacja odleg³oœci od niego
    }
    else //dla samochodów pozostaje jedynie skanowanie uruchomiæ
     if (fTrackBlock<1000.0) //je¿eli pojazdu nie ma, a odleg³o¿æ jakoœ ma³a
      ABuScanObjects(iDirection?1:-1,300); //skanowanie sprawdzaj¹ce
    //WriteLog(asName+" - block x: "+AnsiString(fTrackBlock));
   }
  }
*/
 }
 if (MoverParameters->DerailReason>0)
 {switch (MoverParameters->DerailReason)
  {case 1: WriteLog(asName+" derailed due to end of track"); break;
   case 2: WriteLog(asName+" derailed due to too high speed"); break;
   case 3: ErrorLog("Bad dynamic: "+asName+" derailed due to track width"); break; //b³¹d w scenerii
   case 4: ErrorLog("Bad dynamic: "+asName+" derailed due to wrong track type"); break; //b³¹d w scenerii
  }
  MoverParameters->DerailReason=0; //¿eby tylko raz
 }
 if (MoverParameters->LoadStatus)
  LoadUpdate(); //zmiana modelu ³adunku
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
    MoverParameters->UpdateBatteryVoltage(dt); //jest ju¿ w Mover.cpp
    if (MoverParameters->EnginePowerSource.SourceType==CurrentCollector) //tylko jeœli pantografuj¹cy
     if (MoverParameters->Power>1.0) //w rozrz¹dczym nie (jest b³¹d w FIZ!)
      MoverParameters->UpdatePantVolume(dt); //Ra: pneumatyka pantografów przeniesiona do Mover.cpp!
    //Move(dDOMoveLen);
    //ResetdMoveLen();
    FastMove(dDOMoveLen);

 if (MoverParameters->LoadStatus)
  LoadUpdate(); //zmiana modelu ³adunku
 return true; //Ra: chyba tak?
}

//McZapkie-040402: liczenie pozycji uwzgledniajac wysokosc szyn itp.
//vector3 __fastcall TDynamicObject::GetPosition()
//{//Ra: pozycja pojazdu jest liczona zaraz po przesuniêciu
// return vPosition;
//};

void __fastcall TDynamicObject::TurnOff()
{//wy³¹czenie rysowania submodeli zmiennych dla egemplarza pojazdu
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
{//funkcja rysuj¹ca sto¿ek w miejscu osi
 glPushMatrix(); //matryca kamery
  glTranslatef(p.x,p.y+6,p.z); //6m ponad
  glRotated(RadToDeg(-d),0,1,0); //obrót wzglêdem osi OY
  //glRotated(RadToDeg(vAngles.z),0,1,0); //obrót wzglêdem osi OY
  glDisable(GL_LIGHTING);
  glColor3f(1.0-fNr,fNr,0); //czerwone dla 0, zielone dla 1
  //glutWireCone(promieñ podstawy,wysokoœæ,k¹tnoœæ podstawy,iloœæ segmentów na wysokoœæ)
  glutWireCone(0.5,2,4,1); //rysowanie sto¿ka (ostros³upa o podstawie wieloboka)
  glEnable(GL_LIGHTING);
 glPopMatrix();
}
*/

void __fastcall TDynamicObject::Render()
{//rysowanie elementów nieprzezroczystych
 //youBy - sprawdzamy, czy jest sens renderowac
 double modelrotate;
 vector3 tempangle;
 // zmienne
 renderme=false;
 //przeklejka
 double ObjSqrDist=SquareMagnitude(Global::pCameraPosition-vPosition);
 //koniec przeklejki
 if (ObjSqrDist<500) //jak jest blisko - do 70m
  modelrotate=0.01f; //ma³y k¹t, ¿eby nie znika³o
 else
 {//Global::pCameraRotation to k¹t bewzglêdny w œwiecie (zero - na pó³noc)
  tempangle=(vPosition-Global::pCameraPosition); //wektor od kamery
  modelrotate=ABuAcos(tempangle); //okreœlenie k¹ta
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
  TSubModel::iInstance=(int)this; //¿eby nie robiæ cudzych animacji
  //AnsiString asLoadName="";
  double ObjSqrDist=SquareMagnitude(Global::pCameraPosition-vPosition);
  ABuLittleUpdate(ObjSqrDist); //ustawianie zmiennych submodeli dla wspólnego modelu

  //Cone(vCoulpler[0],modelRot.z,0);
  //Cone(vCoulpler[1],modelRot.z,1);

  //ActualTrack= GetTrack(); //McZapkie-240702

#if RENDER_CONE
  {//Ra: testowe renderowanie pozycji wózków w postaci ostros³upów, wymaga GLUT32.DLL
   double dir=RadToDeg(atan2(vLeft.z,vLeft.x));
   Axle0.Render(0);
   Axle1.Render(1); //bogieRot[0]
   //if (PrevConnected) //renderowanie po³¹czenia
  }
#endif

  glPushMatrix();
  //vector3 pos= vPosition;
  //double ObjDist= SquareMagnitude(Global::pCameraPosition-pos);
  if (this==Global::pUserDynamic)
  {//specjalne ustawienie, aby nie trzês³o
   if (Global::bSmudge)
   {//jak smuga, to rysowaæ po smudze
    glPopMatrix(); //to trzeba zebraæ przed wyœciem
    return;
   }
   //if (Global::pWorld->) //tu trzeba by ustawiæ animacje na modelu zewnêtrznym
   glLoadIdentity(); //zacz¹æ od macierzy jedynkowej
   Global::pCamera->SetCabMatrix(vPosition); //specjalne ustawienie kamery
  }
  else
   glTranslated(vPosition.x,vPosition.y,vPosition.z); //standardowe przesuniêcie wzglêdem pocz¹tku scenerii
  glMultMatrixd(mMatrix.getArray());
  if (fShade>0.0)
  {//Ra: zmiana oswietlenia w tunelu, wykopie
   GLfloat ambientLight[4]= {0.5f,0.5f,0.5f,1.0f};
   GLfloat diffuseLight[4]= {0.5f,0.5f,0.5f,1.0f};
   GLfloat specularLight[4]={0.5f,0.5f,0.5f,1.0f};
   //trochê problem z ambientem w wykopie...
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
   if (mdLoad) //renderowanie nieprzezroczystego ³adunku
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
   if (mdLoad) //renderowanie nieprzezroczystego ³adunku
    mdLoad->Render(ObjSqrDist,ReplacableSkinID,iAlpha);
   if (mdPrzedsionek)
    mdPrzedsionek->Render(ObjSqrDist,ReplacableSkinID,iAlpha);
  }

  //Ra: czy ta kabina tu ma sens?
  //Ra: czy nie renderuje siê dwukrotnie?
  //Ra: dlaczego jest zablokowana w przezroczystych?
  if (mdKabina) //jeœli ma model kabiny
  if ((mdKabina!=mdModel) && bDisplayCab && FreeFlyModeFlag)
  {//rendering kabiny gdy jest oddzielnym modelem i ma byc wyswietlana
   //ABu: tylko w trybie FreeFly, zwykly tryb w world.cpp
   //Ra: œwiet³a s¹ ustawione dla zewnêtrza danego pojazdu
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
  if (fShade!=0.0) //tylko jeœli by³o zmieniane
  {//przywrócenie standardowego oœwietlenia
   glLightfv(GL_LIGHT0,GL_AMBIENT,Global::ambientDayLight);
   glLightfv(GL_LIGHT0,GL_DIFFUSE,Global::diffuseDayLight);
   glLightfv(GL_LIGHT0,GL_SPECULAR,Global::specularDayLight);
  }
  glPopMatrix();
  if (btnOn) TurnOff(); //przywrócenie domyœlnych pozycji submodeli
 } //yB - koniec mieszania z grafika
};

void __fastcall TDynamicObject::RenderSounds()
{//przeliczanie dŸwiêków, bo bêdzie s³ychaæ bez wyœwietlania sektora z pojazdem
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
            if ((MoverParameters->DynamicBrakeFlag) && (MoverParameters->EnginePower>0.1)) //Szociu - 29012012 - je¿eli uruchomiony jest  hamulec elektrodynamiczny, odtwarzany jest dŸwiêk silnika
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
// MoverParameters->CompressorFlag=false; //Ra: wywaliæ to st¹d, tu tylko dla wyœwietlanych!
//Ra: no to ju¿ wiemy, dlaczego poci¹gi je¿d¿¹ lepiej, gdy siê na nie patrzy!
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
if ((MoverParameters->MainCtrlPos)>=(MoverParameters->TurboTest))  //hunter-250312: dlaczego zakomentowane? Ra: bo nie dzia³a³o dobrze
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
{//rysowanie elementów pó³przezroczystych
 if (renderme)
 {
  TSubModel::iInstance=(int)this; //¿eby nie robiæ cudzych animacji
  double ObjSqrDist=SquareMagnitude(Global::pCameraPosition-vPosition);
  ABuLittleUpdate(ObjSqrDist); //ustawianie zmiennych submodeli dla wspólnego modelu
  glPushMatrix();
  if (this==Global::pUserDynamic)
  {//specjalne ustawienie, aby nie trzês³o
   if (Global::bSmudge)
   {//jak smuga, to rysowaæ po smudze
    glPopMatrix(); //to trzeba zebraæ przed wyœciem
    return;
   }
   glLoadIdentity(); //zacz¹æ od macierzy jedynkowej
   Global::pCamera->SetCabMatrix(vPosition); //specjalne ustawienie kamery
  }
  else
   glTranslated(vPosition.x,vPosition.y,vPosition.z); //standardowe przesuniêcie wzglêdem pocz¹tku scenerii
  glMultMatrixd(mMatrix.getArray());
  if (fShade>0.0)
  {//Ra: zmiana oswietlenia w tunelu, wykopie
   GLfloat ambientLight[4]= {0.5f,0.5f,0.5f,1.0f};
   GLfloat diffuseLight[4]= {0.5f,0.5f,0.5f,1.0f};
   GLfloat specularLight[4]={0.5f,0.5f,0.5f,1.0f};
   //trochê problem z ambientem w wykopie...
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
   //if (mdPrzedsionek) //Ra: przedsionków tu wczeœniej nie by³o - w³¹czyæ?
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
   //if (mdPrzedsionek) //Ra: przedsionków tu wczeœniej nie by³o - w³¹czyæ?
   // mdPrzedsionek->RenderAlpha(ObjSqrDist,ReplacableSkinID,iAlpha);
  }
/* skoro false to mo¿na wyci¹c
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
  if (fShade!=0.0) //tylko jeœli by³o zmieniane
  {//przywrócenie standardowego oœwietlenia
   glLightfv(GL_LIGHT0,GL_AMBIENT,Global::ambientDayLight);
   glLightfv(GL_LIGHT0,GL_DIFFUSE,Global::diffuseDayLight);
   glLightfv(GL_LIGHT0,GL_SPECULAR,Global::specularDayLight);
  }
  glPopMatrix();
  if (btnOn) TurnOff(); //przywrócenie domyœlnych pozycji submodeli
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
 char* buf=new char[size+1]; //ci¹g bajtów o d³ugoœci równej rozmiwarowi pliku
 buf[size]='\0'; //zakoñczony zerem na wszelki wypadek
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
 pants=NULL; //wskaŸnik pierwszego obiektu animuj¹cego dla pantografów
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
         iAlpha=0x31310031; //tekstura -1 z kana³em alfa - nie renderowaæ w cyklu nieprzezroczystych
        else
         iAlpha=0x30300030; //wszystkie tekstury nieprzezroczyste - nie renderowaæ w cyklu przezroczystych
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
        {//wartoœæ niby "pantstate" - nazwa dla formalnoœci, wa¿na jest iloœæ
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
        else //Ra: tu wczytywanie modelu ³adunku jest w porz¹dku
         mdLoad=TModelsManager::GetModel(asLoadName.c_str(),true);  //ladunek
       Global::asCurrentTexturePath=AnsiString(szTexturePath); //z powrotem defaultowa sciezka do tekstur
       while (!Parser->EndOfFile && str!=AnsiString("endmodels"))
       {
        str=Parser->GetNextSymbol().LowerCase();
        if (str==AnsiString("animations:"))
        {//Ra: ustawienie iloœci poszczególnych animacji - musi byæ jako pierwsze, inaczej iloœci bêd¹ domyœlne
         if (!pAnimations)
         {//jeœli nie ma jeszcze tabeli animacji, mo¿na odczytaæ nowe iloœci
          int co=0,ile;
          iAnimations=0;
          do
          {//kolejne liczby to iloœæ animacj, -1 to znacznik koñca
           ile=Parser->GetNextSymbol().ToIntDef(-1); //iloœæ danego typu animacji
           //if (co==ANIM_PANTS)
           // if (!Global::bLoadTraction)
           //  if (!DebugModeFlag) //w debugmode pantografy maj¹ "niby dzia³aæ"
           //   ile=0; //wy³¹czenie animacji pantografów
           if (co<ANIM_TYPES)
            if (ile>=0)
            {iAnimType[co]=ile; //zapamiêtanie
             iAnimations+=ile; //ogólna iloœæ animacji
            }
           ++co;
          } while (ile>=0); //-1 to znacznik koñca
          while (co<ANIM_TYPES) iAnimType[co++]=0; //zerowanie pozosta³ych
          str=Parser->GetNextSymbol().LowerCase();
         }
         //WriteLog("Total animations: "+AnsiString(iAnimations));
        }
        if (!pAnimations)
        {//Ra: tworzenie tabeli animacji, jeœli jeszcze nie by³o
         if (!iAnimations) iAnimations=28; //tyle by³o kiedyœ w ka¿dym pojeŸdzie (2 wi¹zary wypad³y)
         pAnimations=new TAnim[iAnimations];
         int i,j,k=0,sm=0;
         for (j=0;j<ANIM_TYPES;++j)
          for (i=0;i<iAnimType[j];++i)
          {
           if (j==ANIM_PANTS) //zliczamy poprzednie animacje
            if (!pants)
             if (iAnimType[ANIM_PANTS]) //o ile jakieœ pantografy s¹ (a domyœlnie s¹)
              pants=pAnimations+k; //zapamiêtanie na potrzeby wyszukania submodeli
           pAnimations[k].iShift=sm; //przesuniêcie do przydzielenia wskaŸnika
           sm+=pAnimations[k++].TypeSet(j); //ustawienie typu animacji i zliczanie submodeli
          }
         if (sm) //o ile s¹ bardziej z³o¿one animacje
         {pAnimated=new TSubModel*[sm]; //tabela na animowane submodele
          for (k=0;k<iAnimations;++k)
           pAnimations[k].smElement=pAnimated+pAnimations[k].iShift; //przydzielenie wskaŸnika do tabelki
         }
        }
        if (str==AnsiString("lowpolyinterior:")) //ABu: wnetrze lowpoly
        {
         asModel=Parser->GetNextSymbol().LowerCase();
         asModel=BaseDir+asModel; //McZapkie-200702 - dynamics maja swoje modele w dynamic/basedir
         Global::asCurrentTexturePath=BaseDir; //biezaca sciezka do tekstur to dynamic/...
         mdLowPolyInt=TModelsManager::GetModel(asModel.c_str(),true);
         //Global::asCurrentTexturePath=AnsiString(szTexturePath); //kiedyœ uproszczone wnêtrze miesza³o tekstury nieba
        }
        else if (str==AnsiString("animwheelprefix:"))
        {//prefiks krêc¹cych siê kó³
         int i,j,k,m;
         str=Parser->GetNextSymbol();
         for (i=0;i<iAnimType[ANIM_WHEELS];++i) //liczba osi
         {//McZapkie-050402: wyszukiwanie kol o nazwie str*
          asAnimName=str+AnsiString(i+1);
          pAnimations[i].smAnimated=mdModel->GetFromName(asAnimName.c_str()); //ustalenie submodelu
          if (pAnimations[i].smAnimated)
          {//++iAnimatedAxles;
           pAnimations[i].smAnimated->WillBeAnimated(); //wy³¹czenie optymalizacji transformu
           pAnimations[i].yUpdate=UpdateAxle; //animacja osi
           pAnimations[i].fMaxDist=50*50; //nie krêciæ w wiêkszej odleg³oœci
          }
         }
         //Ra: ustawianie indeksów osi
         for (i=0;i<iAnimType[ANIM_WHEELS];++i) //iloœæ osi (zabezpieczenie przed b³êdami w CHK)
          pAnimations[i].dWheelAngle=dWheelAngle+1; //domyœlnie wskaŸnik na napêdzaj¹ce
         i=0; j=1; k=0; m=0; //numer osi; kolejny znak; ile osi danego typu; która œrednica
         if ((MoverParameters->WheelDiameterL!=MoverParameters->WheelDiameter)||(MoverParameters->WheelDiameterT!=MoverParameters->WheelDiameter))
         {//obs³uga ró¿nych œrednic, o ile wystêpuj¹
          while ((i<iAnimType[ANIM_WHEELS])&&(j<=MoverParameters->AxleArangement.Length()))
          {//wersja ze wskaŸnikami jest bardziej elastyczna na nietypowe uk³ady
           if ((k>='A')&&(k<='J')) //10 chyba maksimum?
           {pAnimations[i++].dWheelAngle=dWheelAngle+1; //obrót osi napêdzaj¹cych
            --k; //nastêpna bêdzie albo taka sama, albo bierzemy kolejny znak
            m=2; //nastêpuj¹ce toczne bêd¹ mia³y inn¹ œrednicê
           }
           else if ((k>='1')&&(k<='9'))
           {pAnimations[i++].dWheelAngle=dWheelAngle+m; //obrót osi tocznych
            --k; //nastêpna bêdzie albo taka sama, albo bierzemy kolejny znak
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
        {//Ra: pantografy po nowemu maj¹ literki i numerki
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
           pants[i].smElement[0]=sm; //jak NULL, to nie bêdzie animowany
           if (sm)
           {//w EP09 wywala³o siê tu z powodu NULL
            sm->WillBeAnimated();
            //sm->ParentMatrix(m); //pobranie macierzy transformacji
            m=float4x4(*sm->GetMatrix()); //skopiowanie, bo bêdziemy mno¿yæ
            //m(3)[1]=m[3][1]+0.054; //w górê o wysokoœæ œlizgu (na razie tak)
            while (sm->Parent)
            {//przenieœæ tê funkcjê do modelu...
             if (sm->Parent->GetMatrix())
              m=*sm->Parent->GetMatrix()*m;
             sm=sm->Parent;
            }
            if ((mdModel->Flags()&0x8000)==0) //jeœli wczytano z T3D
            {//mo¿e byæ potrzebny dodatkowy obrót, jeœli wczytano z T3D, tzn. pred wykonaniem Init()
             m.InitialRotate();
            }
            pants[i].fParamPants->vPos.z=m[3][0]; //przesuniêcie w bok (asymetria)
            pants[i].fParamPants->vPos.y=m[3][1]; //przesuniêcie w górê odczytane z modelu
            if ((sm=pants[i].smElement[0]->ChildGet())!=NULL)
            {//jeœli ma potomny, mo¿na policzyæ d³ugoœæ (odleg³oœæ potomnego od osi obrotu)
             m=float4x4(*sm->GetMatrix()); //wystarczy³by wskaŸnik, nie trzeba kopiowaæ
             //mo¿e trzeba: pobraæ macierz dolnego ramienia, wyzerowaæ przesuniêcie, przemno¿yæ przez macierz górnego
             pants[i].fParamPants->fHoriz=-fabs(m(3)[1]);
             pants[i].fParamPants->fLenL1=hypot(m(3)[1],m(3)[2]); //po osi OX nie potrzeba
             pants[i].fParamPants->fAngleL0=atan2(fabs(m(3)[2]),fabs(m(3)[1]));
             //if (pants[i].fParamPants->fAngleL0<M_PI_2) pants[i].fParamPants->fAngleL0+=M_PI; //gdyby w odwrotn¹ stronê wysz³o
             //if ((pants[i].fParamPants->fAngleL0<0.03)||(pants[i].fParamPants->fAngleL0>0.09)) //normalnie ok. 0.05
             // pants[i].fParamPants->fAngleL0=pants[i].fParamPants->fAngleL;
             pants[i].fParamPants->fAngleL=pants[i].fParamPants->fAngleL0; //pocz¹tkowy k¹t dolnego ramienia
             if ((sm=sm->ChildGet())!=NULL)
             {//jeœli dalej jest œlizg, mo¿na policzyæ d³ugoœæ górnego ramienia
              m=float4x4(*sm->GetMatrix()); //wystarczy³by wskaŸnik, nie trzeba kopiowaæ
              //trzeba by uwzglêdniæ macierz dolnego ramienia, ¿eby uzyskaæ k¹t do poziomu...
              pants[i].fParamPants->fHoriz+=fabs(m(3)[1]); //ró¿nica d³ugoœci rzutów ramion na p³aszczyznê podstawy (jedna dodatnia, druga ujemna)
              pants[i].fParamPants->fLenU1=hypot(m(3)[1],m(3)[2]); //po osi OX nie potrzeba
              //pants[i].fParamPants->pantu=acos((1.22*cos(pants[i].fParamPants->fAngleL)+0.535)/1.755); //górne ramiê
              //pants[i].fParamPants->fAngleU0=acos((1.176289*cos(pants[i].fParamPants->fAngleL)+0.54555075)/1.724482197); //górne ramiê
              pants[i].fParamPants->fAngleU0=atan2(fabs(m(3)[2]),fabs(m(3)[1])); //pocz¹tkowy k¹t górnego ramienia, odczytany z modelu
              //if (pants[i].fParamPants->fAngleU0<M_PI_2) pants[i].fParamPants->fAngleU0+=M_PI; //gdyby w odwrotn¹ stronê wysz³o
              //if (pants[i].fParamPants->fAngleU0<0)
              // pants[i].fParamPants->fAngleU0=-pants[i].fParamPants->fAngleU0;
              //if ((pants[i].fParamPants->fAngleU0<0.00)||(pants[i].fParamPants->fAngleU0>0.09)) //normalnie ok. 0.07
              // pants[i].fParamPants->fAngleU0=acos((pants[i].fParamPants->fLenL1*cos(pants[i].fParamPants->fAngleL)+pants[i].fParamPants->fHoriz)/pants[i].fParamPants->fLenU1);
              pants[i].fParamPants->fAngleU=pants[i].fParamPants->fAngleU0; //pocz¹tkowy k¹t
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
           pants[i].smElement[1]=sm; //jak NULL, to nie bêdzie animowany
           if (sm)
           {//w EP09 wywala³o siê tu z powodu NULL
            sm->WillBeAnimated();
            if (pants[i].fParamPants->vPos.y==0.0)
            {//jeœli pierwsze ramiê nie ustawi³o tej wartoœci, próbowaæ drugim
             //!!!! docelowo zrobiæ niezale¿n¹ animacjê ramion z ka¿dej strony
             m=float4x4(*sm->GetMatrix()); //skopiowanie, bo bêdziemy mno¿yæ
             m(3)[1]=m[3][1]+0.054; //w górê o wysokoœæ œlizgu (na razie tak)
             while (sm->Parent)
             {
              if (sm->Parent->GetMatrix())
               m=*sm->Parent->GetMatrix()*m;
              sm=sm->Parent;
             }
             pants[i].fParamPants->vPos.z=m[3][0]; //przesuniêcie w bok (asymetria)
             pants[i].fParamPants->vPos.y=m[3][1]; //przesuniêcie w górê odczytane z modelu
            }
           }
           else
            ErrorLog("Bad model: "+asFileName+" - missed submodel "+asAnimName); //brak ramienia
          }
        }
        else if (str==AnsiString("animpantrg1prefix:"))
        {//prefiks ramion górnych 1
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
        {//prefiks ramion górnych 2
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
        {//prefiks œlizgaczy
         str=Parser->GetNextSymbol();
         if (pants)
          for (int i=0;i<iAnimType[ANIM_PANTS];i++)
          {//Winger 160204: wyszukiwanie max 2 patykow o nazwie str*
           asAnimName=str+AnsiString(i+1);
           pants[i].smElement[4]=mdModel->GetFromName(asAnimName.c_str());
           pants[i].smElement[4]->WillBeAnimated();
           pants[i].yUpdate=UpdatePant;
           pants[i].fMaxDist=300*300; //nie podnosiæ w wiêkszej odleg³oœci
           pants[i].iNumber=i;
          }
        }
        else if (str==AnsiString("pantfactors:"))
        {//Winger 010304: parametry pantografow
         double pant1x=Parser->GetNextSymbol().ToDouble();
         double pant2x=Parser->GetNextSymbol().ToDouble();
         double pant1h=Parser->GetNextSymbol().ToDouble(); //wysokoœæ pierwszego œlizgu
         double pant2h=Parser->GetNextSymbol().ToDouble(); //wysokoœæ drugiego œlizgu
         if (pant1h>0.5) pant1h=pant2h; //tu mo¿e byæ zbyt du¿a wartoœæ
         if ((pant1x<0)&&(pant2x>0)) //pierwsza powinna byæ dodatnia, a druga ujemna
         {pant1x=-pant1x; pant2x=-pant2x;}
         if (pants)
          for (int i=0;i<iAnimType[ANIM_PANTS];++i)
          {//przepisanie wspó³czynników do pantografów (na razie nie bêdzie lepiej)
           pants[i].fParamPants->fAngleL=pants[i].fParamPants->fAngleL0; //pocz¹tkowy k¹t dolnego ramienia
           pants[i].fParamPants->fAngleU=pants[i].fParamPants->fAngleU0; //pocz¹tkowy k¹t
           //pants[i].fParamPants->PantWys=1.22*sin(pants[i].fParamPants->fAngleL)+1.755*sin(pants[i].fParamPants->fAngleU); //wysokoœæ pocz¹tkowa
           //pants[i].fParamPants->PantWys=1.176289*sin(pants[i].fParamPants->fAngleL)+1.724482197*sin(pants[i].fParamPants->fAngleU); //wysokoœæ pocz¹tkowa
           pants[i].fParamPants->vPos.x=(i&1)?pant2x:pant1x;
           pants[i].fParamPants->fHeight=(i&1)?pant2h:pant1h; //wysokoœæ œlizgu jest zapisana w MMD
           pants[i].fParamPants->PantWys=pants[i].fParamPants->fLenL1*sin(pants[i].fParamPants->fAngleL)+pants[i].fParamPants->fLenU1*sin(pants[i].fParamPants->fAngleU)+pants[i].fParamPants->fHeight; //wysokoœæ pocz¹tkowa
           //pants[i].fParamPants->vPos.y=panty-panth-pants[i].fParamPants->PantWys; //np. 4.429-0.097=4.332=~4.335
           //pants[i].fParamPants->vPos.z=0; //niezerowe dla pantografów asymetrycznych
           pants[i].fParamPants->PantTraction=pants[i].fParamPants->PantWys;
           //if (panty<3.0)
           // pants[i].fParamPants->fWidth=0.5*panty; //po³owa szerokoœci œlizgu; jest w "Power: CSW="
          }
        }
        else if (str==AnsiString("animpistonprefix:"))
        {//prefiks t³oczysk - na razie u¿ywamy modeli pantografów
         str=Parser->GetNextSymbol();
         for (int i=1;i<=2;i++)
         {
          //asAnimName=str+i;
          //smPatykird1[i-1]=mdModel->GetFromName(asAnimName.c_str());
          //smPatykird1[i-1]->WillBeAnimated();
         }
        }
        else if (str==AnsiString("animconrodprefix:"))
        {//prefiks korbowodów - na razie u¿ywamy modeli pantografów
         str=Parser->GetNextSymbol();
         for (int i=1;i<=2;i++)
         {
          //asAnimName=str+i;
          //smPatykirg1[i-1]=mdModel->GetFromName(asAnimName.c_str());
          //smPatykirg1[i-1]->WillBeAnimated();
         }
        }
        else if (str==AnsiString("pistonfactors:"))
        {//Ra: parametry silnika parowego (t³oka)
/* //Ra: tymczasowo wy³¹czone ze wzglêdu na porz¹dkowanie animacji pantografów
         pant1x=Parser->GetNextSymbol().ToDouble(); //k¹t przesuniêcia dla pierwszego t³oka
         pant2x=Parser->GetNextSymbol().ToDouble(); //k¹t przesuniêcia dla drugiego t³oka
         panty=Parser->GetNextSymbol().ToDouble(); //d³ugoœæ korby (r)
         panth=Parser->GetNextSymbol().ToDouble(); //d³ugoœ korbowodu (k)
*/
         MoverParameters->EnginePowerSource.PowerType=SteamPower; //Ra: po chamsku, ale z CHK nie dzia³a
        }
        else if (str==AnsiString("animreturnprefix:"))
        {//prefiks dr¹¿ka mimoœrodowego - na razie u¿ywamy modeli pantografów
         str=Parser->GetNextSymbol();
         for (int i=1;i<=2;i++)
         {
          //asAnimName=str+i;
          //smPatykird2[i-1]=mdModel->GetFromName(asAnimName.c_str());
          //smPatykird2[i-1]->WillBeAnimated();
         }
        }
        else if (str==AnsiString("animexplinkprefix:")) //animreturnprefix:
        {//prefiks jarzma - na razie u¿ywamy modeli pantografów
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
         {//jak nie ma bez numerka, to mo¿e jest z numerkiem?
          smMechanik0=mdModel->GetFromName(AnsiString(str+"1").c_str());
          smMechanik1=mdModel->GetFromName(AnsiString(str+"2").c_str());
         }
         //aby da³o siê go obracaæ, musi mieæ w³¹czon¹ animacjê w T3D!
         //if (!smMechanik1) //jeœli drugiego nie ma
         // if (smMechanik0) //a jest pierwszy
         //  smMechanik0->WillBeAnimated(); //to bêdziemy go obracaæ
        }
        else if (str==AnsiString("animdoorprefix:"))
        {//nazwa animowanych drzwi
         int i,j,k,m;
         str=Parser->GetNextSymbol();
         for (i=0,j=0;i<ANIM_DOORS;++i)
          j+=iAnimType[i]; //zliczanie wczeœniejszych animacji
         for (i=0;i<iAnimType[ANIM_DOORS];++i) //liczba drzwi
         {//NBMX wrzesien 2003: wyszukiwanie drzwi o nazwie str*
          asAnimName=str+AnsiString(i+1);
          pAnimations[i+j].smAnimated=mdModel->GetFromName(asAnimName.c_str()); //ustalenie submodelu
          if (pAnimations[i+j].smAnimated)
          {//++iAnimatedDoors;
           pAnimations[i+j].smAnimated->WillBeAnimated(); //wy³¹czenie optymalizacji transformu
           switch (MoverParameters->DoorOpenMethod)
           {//od razu zapinamy potrzebny typ animacji
            case 1: pAnimations[i+j].yUpdate=UpdateDoorTranslate; break;
            case 2: pAnimations[i+j].yUpdate=UpdateDoorRotate; break;
           }
           pAnimations[i+j].iNumber=i; //parzyste dzia³aj¹ inaczej ni¿ nieparzyste
           pAnimations[i+j].fMaxDist=300*300; //drzwi to z daleka widaæ
           pAnimations[i+j].fSpeed=random(150); //oryginalny koncept z DoorSpeedFactor
           pAnimations[i+j].fSpeed=(pAnimations[i+j].fSpeed+100)/100;
           //Ra: te wspó³czynniki s¹ bez sensu, bo modyfikuj¹ wektor przesuniêcia
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
         if (iHornWarning) iHornWarning=2; //numer syreny do u¿ycia po otrzymaniu sygna³u do jazdy
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
         //if (MoverParameters->EngineType==DieselElectric) //bêdzie modulowany?
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
 if (mdModel) mdModel->Init(); //obrócenie modelu oraz optymalizacja, równie¿ zapisanie binarnego
 if (mdLoad) mdLoad->Init();
 if (mdPrzedsionek) mdPrzedsionek->Init();
 if (mdLowPolyInt) mdLowPolyInt->Init();
 //sHorn2.CopyIfEmpty(sHorn1); //¿eby jednak tr¹bi³ te¿ drugim
 Global::asCurrentTexturePath=AnsiString(szTexturePath); //kiedyœ uproszczone wnêtrze miesza³o tekstury nieba
}

//---------------------------------------------------------------------------
void __fastcall TDynamicObject::RadioStop()
{//zatrzymanie pojazdu
 if (Mechanik) //o ile ktoœ go prowadzi
  if (MoverParameters->SecuritySystem.RadioStop) //jeœli pojazd ma RadioStop i jest on aktywny
   Mechanik->PutCommand("Emergency_brake",1.0,1.0,&vPosition,stopRadio);
};

void __fastcall TDynamicObject::RaLightsSet(int head,int rear)
{//zapalenie œwiate³ z przodu i z ty³u, zale¿ne od kierunku pojazdu
 if (!MoverParameters) return; //mo¿e tego nie byæ na pocz¹tku
 if (rear==2+32+64)
 {//jeœli koniec poci¹gu, to trzeba ustaliæ, czy jest tam czynna lokomotywa
  //EN57 mo¿e nie mieæ koñcówek od œrodka cz³onu
  if (MoverParameters->Power>1.0) //jeœli ma moc napêdow¹
   if (!MoverParameters->ActiveDir) //jeœli nie ma ustawionego kierunku
   {//jeœli ma zarówno œwiat³a jak i koñcówki, ustaliæ, czy jest w stanie aktywnym
    //np. lokomotywa na zimno bêdzie mieæ koñcówki a nie œwiat³a
    rear=64; //tablice blaszane
    //trzeba to uzale¿niæ od "za³¹czenia baterii" w pojeŸdzie
   }
  if (rear==2+32+64) //jeœli nadal obydwie mo¿liwoœci
   if (iInventory&(iDirection?0x2A:0x15)) //czy ma jakieœ œwiat³a czerowone od danej strony
    rear=2+32; //dwa œwiat³a czerwone
   else
    rear=64; //tablice blaszane
 }
 if (iDirection) //w zale¿noœci od kierunku pojazdu w sk³adzie
 {//jesli pojazd stoi sprzêgiem 0 w stronê czo³a
  if (head>=0) iLights[0]=head;
  if (rear>=0) iLights[1]=rear;
 }
 else
 {//jak jest odwrócony w sk³adzie (-1), to zapalamy odwrotnie
  if (head>=0) iLights[1]=head;
  if (rear>=0) iLights[0]=rear;
 }
};

int __fastcall TDynamicObject::DirectionSet(int d)
{//ustawienie kierunku w sk³adzie (wykonuje AI)
 iDirection=d>0?1:0; //d:1=zgodny,-1=przeciwny; iDirection:1=zgodny,0=przeciwny;
 CouplCounter=20; //¿eby normalnie skanowaæ kolizje, to musi ruszyæ z miejsca
 if (MyTrack)
 {//podczas wczytywania wstawiane jest AI, ale mo¿e jeszcze nie byæ toru
  //AI ustawi kierunek ponownie po uruchomieniu silnika
  if (iDirection) //jeœli w kierunku Coupler 0
  {if (MoverParameters->Couplers[0].CouplingFlag==ctrain_virtual) //brak pojazdu podpiêtego?
    ABuScanObjects(1,300); //szukanie czegoœ do pod³¹czenia
  }
  else
   if (MoverParameters->Couplers[1].CouplingFlag==ctrain_virtual) //brak pojazdu podpiêtego?
    ABuScanObjects(-1,300);
 }
 return 1-(iDirection?NextConnectedNo:PrevConnectedNo); //informacja o po³o¿eniu nastêpnego
};

TDynamicObject* __fastcall TDynamicObject::PrevAny()
{//wskaŸnik na poprzedni, nawet wirtualny
 return iDirection?PrevConnected:NextConnected;
};
TDynamicObject* __fastcall TDynamicObject::Prev()
{
 if (MoverParameters->Couplers[iDirection^1].CouplingFlag)
  return iDirection?PrevConnected:NextConnected;
 return NULL; //gdy sprzêg wirtualny, to jakby nic nie by³o
};
TDynamicObject* __fastcall TDynamicObject::Next()
{
 if (MoverParameters->Couplers[iDirection].CouplingFlag)
  return iDirection?NextConnected:PrevConnected;
 return NULL; //gdy sprzêg wirtualny, to jakby nic nie by³o
};

TDynamicObject* __fastcall TDynamicObject::Neightbour(int &dir)
{//ustalenie nastêpnego (1) albo poprzedniego (0) w sk³adzie bez wzglêdu na prawid³owoœæ iDirection
 int d=dir; //zapamiêtanie kierunku
 dir=1-(dir?NextConnectedNo:PrevConnectedNo); //nowa wartoœæ
 return (d?(MoverParameters->Couplers[1].CouplingFlag?NextConnected:NULL):(MoverParameters->Couplers[0].CouplingFlag?PrevConnected:NULL));
};

void __fastcall TDynamicObject::CoupleDist()
{//obliczenie odleg³oœci sprzêgów
 if (MyTrack?(MyTrack->iCategoryFlag&1):true) //jeœli nie ma przypisanego toru, to liczyæ jak dla kolei
 {//jeœli jedzie po szynach (równie¿ unimog), liczenie kul wystarczy
  MoverParameters->SetCoupleDist();
 }
 else
 {//na drodze trzeba uwzglêdniæ wektory ruchu
  double d0=MoverParameters->Couplers[0].CoupleDist;
  //double d1=MoverParameters->Couplers[1].CoupleDist; //sprzêg z ty³u samochodu mo¿na olaæ, dopóki nie jeŸdzi na wstecznym
  vector3 p1,p2;
  double d,w; //dopuszczalny dystans w poprzek
  MoverParameters->SetCoupleDist(); //liczenie standardowe
  if (MoverParameters->Couplers[0].Connected) //jeœli cokolwiek pod³¹czone
   if (MoverParameters->Couplers[0].CouplingFlag==0) //jeœli wirtualny
    if (MoverParameters->Couplers[0].CoupleDist<300.0) //i mniej ni¿ 300m
    {//przez MoverParameters->Couplers[0].Connected nie da siê dostaæ do DynObj, st¹d prowizorka
     //WriteLog("Collision of "+AnsiString(MoverParameters->Couplers[0].CoupleDist)+"m detected by "+asName+":0.");
     w=0.5*(MoverParameters->Couplers[0].Connected->Dim.W+MoverParameters->Dim.W); //minimalna odleg³oœæ miniêcia
     d=-DotProduct(vLeft,vCoulpler[0]); //odleg³oœæ prostej ruchu od pocz¹tku uk³adu wspó³rzêdnych
     d=fabs(DotProduct(vLeft,((TMoverParameters*)(MoverParameters->Couplers[0].Connected))->vCoulpler[MoverParameters->Couplers[0].ConnectedNr])+d);
     //WriteLog("Distance "+AnsiString(d)+"m from "+asName+":0.");
     if (d>w)
      MoverParameters->Couplers[0].CoupleDist=(d0<10?50:d0); //przywrócenie poprzedniej
    }
  if (MoverParameters->Couplers[1].Connected) //jeœli cokolwiek pod³¹czone
   if (MoverParameters->Couplers[1].CouplingFlag==0) //jeœli wirtualny
    if (MoverParameters->Couplers[1].CoupleDist<300.0) //i mniej ni¿ 300m
    {
     //WriteLog("Collision of "+AnsiString(MoverParameters->Couplers[1].CoupleDist)+"m detected by "+asName+":1.");
     w=0.5*(MoverParameters->Couplers[1].Connected->Dim.W+MoverParameters->Dim.W); //minimalna odleg³oœæ miniêcia
     d=-DotProduct(vLeft,vCoulpler[1]); //odleg³oœæ prostej ruchu od pocz¹tku uk³adu wspó³rzêdnych
     d=fabs(DotProduct(vLeft,((TMoverParameters*)(MoverParameters->Couplers[1].Connected))->vCoulpler[MoverParameters->Couplers[1].ConnectedNr])+d);
     //WriteLog("Distance "+AnsiString(d)+"m from "+asName+":1.");
     if (d>w)
      MoverParameters->Couplers[0].CoupleDist=(d0<10?50:d0); //przywrócenie poprzedniej
    }
 }
};

TDynamicObject* __fastcall TDynamicObject::ControlledFind()
{//taka proteza: chcê pod³¹czyæ kabinê EN57 bezpoœrednio z silnikowym, aby nie robiæ tego przez ukrotnienie
 //drugi silnikowy i tak musi byæ ukrotniony, podobnie jak kolejna jednostka
 //lepiej by by³o przesy³aæ komendy sterowania, co jednak wymaga przebudowy transmisji komend (LD)
 //problem siê robi ze œwiat³ami, które bêd¹ zapalane w silnikowym, ale musz¹ œwieciæ siê w rozrz¹dczych
 //dla EZT œwiat³¹ czo³owe bêd¹ "zapalane w silnikowym", ale widziane z rozrz¹dczych
 //równie¿ wczytywanie MMD powinno dotyczyæ aktualnego cz³onu
 //problematyczna mo¿e byæ kwestia wybranej kabiny (w silnikowym...)
 //jeœli silnikowy bêdzie zapiêty odwrotnie (tzn. -1), to i tak powinno jeŸdziæ dobrze
 //równie¿ hamowanie wykonuje siê zaworem w cz³onie, a nie w silnikowym...
 TDynamicObject *d=this; //zaczynamy od aktualnego
 if (d->MoverParameters->TrainType&dt_EZT) //na razie dotyczy to EZT
  if (d->NextConnected?d->MoverParameters->Couplers[1].AllowedFlag&ctrain_depot:false)
  {//gdy jest cz³on od sprzêgu 1, a sprzêg ³¹czony warsztatowo (powiedzmy)
   if ((d->MoverParameters->Power<1.0)&&(d->NextConnected->MoverParameters->Power>1.0)) //my nie mamy mocy, ale ten drugi ma
    d=d->NextConnected; //bêdziemy sterowaæ tym z moc¹
  }
  else if (d->PrevConnected?d->MoverParameters->Couplers[0].AllowedFlag&ctrain_depot:false)
  {//gdy jest cz³on od sprzêgu 0, a sprzêg ³¹czony warsztatowo (powiedzmy)
   if ((d->MoverParameters->Power<1.0)&&(d->PrevConnected->MoverParameters->Power>1.0)) //my nie mamy mocy, ale ten drugi ma
    d=d->PrevConnected; //bêdziemy sterowaæ tym z moc¹
  }
 return d;
};

