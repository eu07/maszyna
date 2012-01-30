//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

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

#pragma package(smart_init)

const float maxrot=(M_PI/3.0); //60�

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
TDynamicObject* __fastcall TDynamicObject::GetFirstDynamic(int cpl_type)
{//Szukanie skrajnego po��czonego pojazdu w pociagu
 //od strony sprzegu (cpl_type) obiektu szukajacego
 //Ra: wystarczy jedna funkcja do szukania w obu kierunkach
 return FirstFind(cpl_type);
};

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


void TDynamicObject::ABuSetModelShake(vector3 mShake)
{
   modelShake=mShake;
}


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

//ABu 29.01.05 przeklejone z render i renderalpha: *********************
void __inline TDynamicObject::ABuLittleUpdate(double ObjSqrDist)
{//ABu290105: pozbierane i uporzadkowane powtarzajace sie rzeczy z Render i RenderAlpha
 //dodatkowy warunek, if (ObjSqrDist<...) zeby niepotrzebnie nie zmianiec w obiektach,
 //ktorych i tak nie widac
 //NBMX wrzesien, MC listopad: zuniwersalnione
 for (int i=0;i<iAnimatedDoors;i++)
 {
  //WriteLog("Dla drzwi nr:", i);
  //WriteLog("Wspolczynnik", DoorSpeedFactor[i]);
  if (smAnimatedDoor[i]!=NULL)
  {
   if (MoverParameters->DoorOpenMethod==1) //przesuwne
   {
    if ((i%2)==0)
    {
     smAnimatedDoor[i]->SetTranslate(vector3(0,0,dDoorMoveL*DoorSpeedFactor[i]));
    //dDoorMoveL=dDoorMoveL*DoorSpeedFactor[i];
    }
    else
    {
     smAnimatedDoor[i]->SetTranslate(vector3(0,0,dDoorMoveR*DoorSpeedFactor[i]));
    //dDoorMoveR=dDoorMoveR*DoorSpeedFactor[i];
    }
   }
   else
   if (MoverParameters->DoorOpenMethod==2) //obrotowe albo dwojlomne (trzeba kombinowac submodelami i ShiftL=90,R=180)
   {
    if ((i%2)==0)
     smAnimatedDoor[i]->SetRotate(float3(1,0,0),dDoorMoveL);
    else
     smAnimatedDoor[i]->SetRotate(float3(1,0,0),dDoorMoveR);
   }
  }
 } //for (int i=0;i<iAnimatedDoors;i++)
 btnOn=false; //czy przywr�ci� stan domy�lny po renderowaniu

 if (ObjSqrDist<160000) //gdy bli�ej ni� 400m
 {
  if (ObjSqrDist<2500) //gdy bli�ej ni� 50m
  {
   //ABu290105: rzucanie pudlem
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
   for (int i=0; i<iAnimatedAxles; i++)
    if (smAnimatedWheel[i])
     smAnimatedWheel[i]->SetRotate(float3(1,0,0),*pWheelAngle[i]);
     //smAnimatedWheel[i]->SetRotate(float3(1,0,0),dWheelAngle[1]);
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
// Przedni patyk
//    if (dPantAngleF>0)
//     dPantAngleF=0;

  if (Global::bLoadTraction)
  {
   if (smPatykird1[0])
      smPatykird1[0]->SetRotate(float3(1,0,0),dPantAngleF);
   if (smPatykird2[0])
      smPatykird2[0]->SetRotate(float3(-1,0,0),dPantAngleF);
   if (smPatykirg1[0])
      smPatykirg1[0]->SetRotate(float3(-1,0,0),dPantAngleF*1.81);
   if (smPatykirg2[0])
      smPatykirg2[0]->SetRotate(float3(1,0,0),dPantAngleF*1.81);
   if (smPatykisl[0])
      smPatykisl[0]->SetRotate(float3(1,0,0),dPantAngleF*0.81);
   //Tylny patyk
   if (smPatykird1[1])
      smPatykird1[1]->SetRotate(float3(1,0,0),dPantAngleR);
   if (smPatykird2[1])
      smPatykird2[1]->SetRotate(float3(-1,0,0),dPantAngleR);
   if (smPatykirg1[1])
      smPatykirg1[1]->SetRotate(float3(-1,0,0),dPantAngleR*1.81);
   if (smPatykirg2[1])
      smPatykirg2[1]->SetRotate(float3(1,0,0),dPantAngleR*1.81);
   if (smPatykisl[1])
      smPatykisl[1]->SetRotate(float3(1,0,0),dPantAngleR*0.81);
  }
  if (smWiazary[0])
     smWiazary[0]->SetRotate(float3(1,0,0),-dWheelAngle[1]);
  if (smWiazary[1])
     smWiazary[1]->SetRotate(float3(1,0,0),-dWheelAngle[1]);
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
  //sygnaly konca pociagu
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
  //tablice blaszane:
  if (TestFlag(iLights[0],64))
   {btEndSignalsTab1.TurnOn(); btnOn=true;}
  //else btEndSignalsTab1.TurnOff();
  if (TestFlag(iLights[1],64))
   {btEndSignalsTab2.TurnOn(); btnOn=true;}
  //else btEndSignalsTab2.TurnOff();
  //McZapkie-181002: krecenie wahaczem (korzysta z kata obrotu silnika)
  for (int i=0; i<4; i++)
   if (smWahacze[i])
    smWahacze[i]->SetRotate(float3(1,0,0),fWahaczeAmp*cos(MoverParameters->eAngle));
  if (smMechanik)
  {
   if (Mechanik&&(Controller!=Humandriver))  //rysowanie figurki mechanika
    smMechanik->Visible=true;
   else
    smMechanik->Visible=false;
  }
  //ABu: Przechyly na zakretach
  if (ObjSqrDist<80000) ABuModelRoll(); //przechy�ki od 400m
 }
 //sygnaly czola pociagu //Ra: wy�wietlamy bez ogranicze� odleg�o�ci, by by�y widoczne z daleka
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
//ABu 29.01.05 koniec przeklejenia *************************************

double __fastcall ABuAcos(vector3 calc_temp)
{  //Odpowiednik funkcji Arccos, bo cos mi tam nie dzialalo.
   //Dziala w zakresie calych 360 stopni.
   //Wejscie: wektor, dla ktorego liczymy obrot wzgl. osi y
   //Wyjscie: obrot
   bool calc_sin;
   double calc_angle;
   if(fabs(calc_temp.x)>fabs(calc_temp.z)) {calc_sin=false; }
                                      else {calc_sin=true;  };
   double calc_dist=sqrt((calc_temp.x*calc_temp.x)+(calc_temp.z*calc_temp.z));
   if (calc_dist!=0)
   {
        if(calc_sin)
        {
            calc_angle=asin(calc_temp.x/calc_dist);
            if(calc_temp.z>0) {calc_angle=-calc_angle;    } //ok (1)
                         else {calc_angle=M_PI+calc_angle;} // (2)
        }
        else
        {
            calc_angle=acos(calc_temp.z/calc_dist);
            if(calc_temp.x>0) {calc_angle=M_PI+M_PI-calc_angle;} // (3)
                         else {calc_angle=calc_angle;          } // (4)
        }
        return calc_angle;
    }
    return 0;
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
  if (ScanDir>=0) ActDist=Track->Length()-ABuGetTranslation(); //???-przesuni�cie w�zka wzgl�dem Point1 toru
             else ActDist=ABuGetTranslation(); //przesuni�cie w�zka wzgl�dem Point1 toru
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
 double modelRoll=RadToDeg(0.5*(Axle0.GetRoll()+Axle1.GetRoll())); //Ra: tu nie by�o DegToRad
 //if (ABuGetDirection()<0) modelRoll=-modelRoll;
 if (modelRoll!=0.0)
 {//jak nie ma przechy�ki, to nie ma po co przechyla� modeli
  if (mdModel)
   mdModel->GetSMRoot()->SetRotateXYZ(vector3(0,modelRoll,0));
  if (mdKabina)
   if (MoverParameters->ActiveCab==-1)
    mdKabina->GetSMRoot()->SetRotateXYZ(vector3(0,-modelRoll,0));
   else
    mdKabina->GetSMRoot()->SetRotateXYZ(vector3(0,modelRoll,0));
  if (mdLoad)
   mdLoad->GetSMRoot()->SetRotateXYZ(vector3(0,modelRoll,0));
  if (mdLowPolyInt)
   mdLowPolyInt->GetSMRoot()->SetRotateXYZ(vector3(0,modelRoll,0));
  if (mdPrzedsionek)
   mdPrzedsionek->GetSMRoot()->SetRotateXYZ(vector3(0,modelRoll,0));
 }
}

//ABu 06.05.04 poczatek wyliczania obrotow wozkow **********************

void __fastcall TDynamicObject::ABuBogies()
{//Obracanie wozkow na zakretach. Na razie uwzgl�dnia tylko zakr�ty,
 //bez zadnych gorek i innych przeszkod.
 if ((smBogie[0]!=NULL)&&(smBogie[1]!=NULL))
 {
  modelRot.z=ABuAcos(Axle0.pPosition-Axle1.pPosition);
  //bogieRot[0].z=ABuAcos(Axle0.pPosition-Axle3.pPosition);
  bogieRot[0].z=Axle0.vAngles.z;
  //bogieRot[1].z=ABuAcos(Axle2.pPosition-Axle1.pPosition);
  bogieRot[1].z=Axle1.vAngles.z;
  bogieRot[0]=RadToDeg(modelRot-bogieRot[0]); //mno�enie wektora przez sta��
  bogieRot[1]=RadToDeg(modelRot-bogieRot[1]);
  smBogie[0]->SetRotateXYZ(bogieRot[0]);
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
TDynamicObject* __fastcall ABuFindObject(TTrack *Track,TDynamicObject *MyPointer,int ScanDir,Byte &CouplFound)
{//Zwraca wska�nik najbli�szego obiektu znajduj�cego si�
 //na torze w okre�lonym kierunku, ale tylko wtedy, kiedy
 //obiekty mog� si� zderzy�, tzn. nie mijaj� si�.

 //WE: Track      - tor, na ktorym odbywa sie poszukiwanie,
 //    MyPointer  - wskaznik do obiektu szukajacego.
 //    ScanDir    - kierunek szukania na torze (b�dzie inne, je�li tor jest odwrotnie)
 //    MyScanDir  - kierunek szukania obiektu szukajacego (na jego torze); Ra: nie potrzebne
 //    MyCouplFound - nr sprzegu obiektu szukajacego; Ra: nie potrzebne

 //WY: wskaznik do znalezionego obiektu.
 //    CouplFound - nr sprzegu znalezionego obiektu
 if ((Track->iNumDynamics)>0)
 {//sens szukania na tym torze jest tylko, gdy s� na nim pojazdy
  double ObjTranslation; //pozycja najblizszego obiektu na torze
  double MyTranslation; //pozycja szukaj�cego na torze
  double MinDist=Track->Length(); //najmniejsza znaleziona odleglo�� (zaczynamy od d�ugo�ci toru)
  double TestDist;
  int iMinDist=-1;  //indeks wykrytego obiektu
  if (MyPointer->MyTrack==Track) //gdy szukanie na tym samym torze
   MyTranslation=MyPointer->ABuGetTranslation(); //po�o�enie w�zka wzgl�dem Point1 toru
  else //gdy szukanie na innym torze
   if (ScanDir>0)
    MyTranslation=0; //szukanie w kierunku Point2 (od zera) - jeste�my w Point1
   else
    MyTranslation=MinDist; //szukanie w kierunku Point1 (do zera) - jeste�my w Point2
  if (ScanDir>=0)
  {//je�li szukanie w kierunku Point2
   for (int i=0;i<Track->iNumDynamics;i++)
   {//p�tla po pojazdach
    if (MyPointer!=Track->Dynamics[i]) //szukaj�cy si� nie liczy
    {
     TestDist=(Track->Dynamics[i]->ABuGetTranslation())-MyTranslation; //odleg�og�o�� tamtego od szukaj�cego
     if ((TestDist>0)&&(TestDist<=MinDist))
     {//gdy jest po w�a�ciwej stronie i bli�ej ni� jaki� wcze�niejszy
      CouplFound=(Track->Dynamics[i]->ABuGetDirection()>0)?1:0; //to, bo (ScanDir>=0)
      if (Track->iCategoryFlag&254) //trajektoria innego typu ni� tor kolejowy
      {//dla tor�w nie ma sensu tego sprawdza�, rzadko co jedzie po jednej szynie i si� mija
       //Ra: mijanie samochod�w wcale nie jest proste
       // Przesuniecie wzgledne pojazdow. Wyznaczane, zeby sprawdzic,
       // czy pojazdy faktycznie sie zderzaja (moga byc przesuniete
       // w/m siebie tak, ze nie zachodza na siebie i wtedy sie mijaja).
       double RelOffsetH; //wzajemna odleg�o�� poprzeczna
       if (CouplFound) //my na tym torze by�my byli w kierunku Point2
        //dla CouplFound=1 s� zwroty zgodne - istotna r�nica przesuni��
        RelOffsetH=(MyPointer->MoverParameters->OffsetTrackH-Track->Dynamics[i]->MoverParameters->OffsetTrackH);
       else
        //dla CouplFound=0 s� zwroty przeciwne - przesuni�cia sumuj� si�
        RelOffsetH=(MyPointer->MoverParameters->OffsetTrackH+Track->Dynamics[i]->MoverParameters->OffsetTrackH);
       if (RelOffsetH<0) RelOffsetH=-RelOffsetH;
       if (RelOffsetH+RelOffsetH>MyPointer->MoverParameters->Dim.W)+(Track->Dynamics[i]->MoverParameters->Dim.W);
        continue; //odleg�o�� wi�ksza od po�owy sumy szeroko�ci - kolizji nie b�dzie
      }
      iMinDist=i; //potencjalna kolizja
      MinDist=TestDist;
     }
    }
   }
  }
  else //(ScanDir<0)
  {
   for (int i=0; i<Track->iNumDynamics;i++)
   {
    if (MyPointer!=Track->Dynamics[i])
    {
     TestDist=MyTranslation-(Track->Dynamics[i]->ABuGetTranslation()); //???-przesuni�cie w�zka wzgl�dem Point1 toru
     if ((TestDist>0)&&(TestDist<MinDist))
     {
      CouplFound=(Track->Dynamics[i]->ABuGetDirection()>0)?0:1; //odwrotnie, bo (ScanDir<0)
      if (Track->iCategoryFlag&254) //trajektoria innego typu ni� tor kolejowy
      {//dla tor�w nie ma sensu tego sprawdza�, rzadko co jedzie po jednej szynie i si� mija
       //Ra: mijanie samochod�w wcale nie jest proste
       // Przesuniecie wzgledne pojazdow. Wyznaczane, zeby sprawdzic,
       // czy pojazdy faktycznie sie zderzaja (moga byc przesuniete
       // w/m siebie tak, ze nie zachodza na siebie i wtedy sie mijaja).
       double RelOffsetH; //wzajemna odleg�o�� poprzeczna
       if (CouplFound) //my na tym torze by�my byli w kierunku Point1
        //dla CouplFound=1 s� zwroty zgodne - istotna r�nica przesuni��
        RelOffsetH=(MyPointer->MoverParameters->OffsetTrackH-Track->Dynamics[i]->MoverParameters->OffsetTrackH);
       else
        //dla CouplFound=0 s� zwroty przeciwne - przesuni�cia sumuj� si�
        RelOffsetH=(MyPointer->MoverParameters->OffsetTrackH+Track->Dynamics[i]->MoverParameters->OffsetTrackH);
       if (RelOffsetH<0) RelOffsetH=-RelOffsetH;
       if (RelOffsetH+RelOffsetH>MyPointer->MoverParameters->Dim.W)+(Track->Dynamics[i]->MoverParameters->Dim.W);
        continue; //odleg�o�� wi�ksza od po�owy sumy szeroko�ci - kolizji nie b�dzie
      }
      iMinDist=i; //potencjalna kolizja
      MinDist=TestDist;
     }
    }
   }
  }
  return (iMinDist>=0)?Track->Dynamics[iMinDist]:NULL;
 }
 return NULL; //nie ma pojazd�w na torze, to jest NULL
}

bool TDynamicObject::DettachDistance(int dir)
{//sprawdzenie odleg�o�ci sprz�g�w rzeczywistych od strony (dir): 0=prz�d,1=ty�
 if (!MoverParameters->Couplers[dir].CouplingFlag)
  return true; //je�li nic nie pod��czone, to jest OK
 return (MoverParameters->DettachDistance(dir)); //czy jest w odpowiedniej odleg�o�ci?
}

int TDynamicObject::Dettach(int dir,int cnt)
{//roz��czenie sprz�g�w rzeczywistych od strony (dir): 0=prz�d,1=ty�
 //zwraca mask� bitow� aktualnych sprzeg�w (0 je�li roz��czony)
 if (MoverParameters->Couplers[dir].CouplingFlag) //zapami�tanie co by�o pod��czone
  MoverParameters->Dettach(dir);
 return MoverParameters->Couplers[dir].CouplingFlag; //sprz�g po roz��czaniu
}

void TDynamicObject::CouplersDettach(double MinDist,int MyScanDir)
{//funkcja roz��czajaca pod��czone sprz�gi
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
{//skanowanie toru w poszukiwaniu pojazd�w
 //ScanDir - okre�la kierunek poszukiwania zale�nie od zwrotu pr�dko�ci pojazdu
 // ScanDir=1 - od strony Coupler0, ScanDir=-1 - od strony Coupler1
 int MyScanDir=ScanDir;  //zapami�tanie kierunku poszukiwa� na torze wyj�ciowym
 TTrackFollower *FirstAxle=(MyScanDir>0?&Axle0:&Axle1); //mo�na by to trzyma� w trainset
 TTrack *Track=FirstAxle->GetTrack(); //tor na kt�rym "stoi" skrajny w�zek (mo�e by� inny ni� pojazdu)
 if (FirstAxle->GetDirection()<0) //czy o� jest ustawiona w stron� Point1?
  ScanDir=-ScanDir; //je�li tak, to kierunek szukania b�dzie przeciwny (teraz wzgl�dem toru)
 Byte MyCouplFound; //numer sprz�gu do pod��czenia w obiekcie szukajacym
 MyCouplFound=(MyScanDir<0)?1:0;
 Byte CouplFound; //numer sprz�gu w znalezionym obiekcie (znaleziony wype�ni)
 TDynamicObject *FoundedObj; //znaleziony obiekt
 FoundedObj=ABuFindObject(Track,this,ScanDir,CouplFound); //zaczynamy szuka� na tym samym torze

/*
 if (FoundedObj) //jak co� znajdzie, to �ledzimy
 {//powt�rzenie wyszukiwania tylko do zastawiania pu�epek podczas test�w
  if (ABuGetDirection()<0) ScanDir=ScanDir; //ustalenie kierunku wzgl�dem toru
  FoundedObj=ABuFindObject(Track,this,ScanDir,CouplFound);
 }
*/

 if (FoundedObj) //kod s�u��cy do logowania b��d�w
  if (CouplFound==0)
  {
   if (FoundedObj->PrevConnected)
    if (FoundedObj->PrevConnected!=this) //od�wie�enie tego samego si� nie liczy
     WriteLog("0! Coupler error on "+asName+":"+AnsiString(MyCouplFound)+" - "+FoundedObj->asName+":0 connected to "+FoundedObj->PrevConnected->asName+":"+AnsiString(FoundedObj->PrevConnectedNo));
  }
  else
  {
   if (FoundedObj->NextConnected)
    if (FoundedObj->NextConnected!=this) //od�wie�enie tego samego si� nie liczy
     WriteLog("0! Coupler error on "+asName+":"+AnsiString(MyCouplFound)+" - "+FoundedObj->asName+":1 connected to "+FoundedObj->NextConnected->asName+":"+AnsiString(FoundedObj->NextConnectedNo));
  }


 if (FoundedObj==NULL) //je�li nie ma na tym samym, szukamy po okolicy
 {//szukanie najblizszego toru z jakims obiektem
  //praktycznie przeklejone z TraceRoute()...
  double ActDist;    //przeskanowana odleglo��
  double CurrDist=0; //aktualna dlugosc toru
  if (ScanDir>=0) //uwzgl�dniamy kawalek przeanalizowanego wy�ej toru
   ActDist=Track->Length()-FirstAxle->GetTranslation(); //odleg�o�� osi od Point2 toru
  else
   ActDist=FirstAxle->GetTranslation(); //odleg�o�� osi od Point1 toru
  while (ActDist<ScanDist)
  {
   ActDist+=CurrDist;
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
    CurrDist=Track->Length(); //doliczenie tego toru do przejrzanego dystandu
    FoundedObj=ABuFindObject(Track,this,ScanDir,CouplFound); //przejrzenie pojazd�w tego toru
    if (FoundedObj)
    {
     //if((Mechanik)&&(!fTrackBlock))
     //{
     //   fTrackBlock=true;
     //   //Mechanik->SetProximityVelocity(0,20);
     //   Mechanik->SetVelocity(0,0,stopBlock);
     //}
     ActDist=ScanDist; //wyj�cie z p�tli poszukiwania
    }
   }
   else //je�li toru nie ma, to wychodzimy
   {
/* //Ra: sprawdzanie ko�ca toru nie mo�e by� tutaj, bo trzeba te� uwzgl�dni� tory z pr�dko�ci� 0 i uszkodzone
    if ((Mechanik)&&(fTrackBlock>50.0))
    {
     fTrackBlock=50.0;
     Mechanik->SetProximityVelocity(ActDist-50,0);
     //Mechanik->SetVelocity(0,0,stopBlock);
    }
*/
    ActDist=ScanDist; //koniec przegl�dania tor�w
   }
  }
 } // Koniec szukania najbli�szego toru z jakim� obiektem.
 //teraz odczepianie i je�li co� si� znalaz�o, doczepianie.
 if (MyScanDir>0?PrevConnected:NextConnected)
  if ((MyScanDir>0?PrevConnected:NextConnected)!=FoundedObj)
   CouplersDettach(1,MyScanDir);
 // i ��czenie sprz�giem wirtualnym
 if (FoundedObj)
 {//siebie mo�na bezpiecznie pod��czy� jednostronnie do znalezionego
  MoverParameters->Attach(MyCouplFound,CouplFound,&(FoundedObj->MoverParameters),ctrain_virtual);
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
   FoundedObj->MoverParameters->Attach(CouplFound,MyCouplFound,&(this->MoverParameters),ctrain_virtual);
   //FoundedObj->MoverParameters->Couplers[CouplFound].Render=false; //tamtemu nie ma co zmienia� stanu sprz�g�w
   if (CouplFound==0)
   {
    if (FoundedObj->PrevConnected)
     if (FoundedObj->PrevConnected!=this)
      WriteLog("1! Coupler error on "+asName+":"+AnsiString(MyCouplFound)+" - "+FoundedObj->asName+":0 connected to "+FoundedObj->PrevConnected->asName+":"+AnsiString(FoundedObj->PrevConnectedNo));
    FoundedObj->PrevConnected=this;
    FoundedObj->PrevConnectedNo=MyCouplFound;
   }
   else
   {
    if (FoundedObj->NextConnected)
     if (FoundedObj->NextConnected!=this)
      WriteLog("1! Coupler error on "+asName+":"+AnsiString(MyCouplFound)+" - "+FoundedObj->asName+":1 connected to "+FoundedObj->NextConnected->asName+":"+AnsiString(FoundedObj->NextConnectedNo));
    FoundedObj->NextConnected=this;
    FoundedObj->NextConnectedNo=MyCouplFound;
   }
  }
  fTrackBlock=MoverParameters->Couplers[MyCouplFound].CoupleDist; //odleg�o�� do najbli�szego pojazdu
 }
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
 TrainParams=NULL; //Ra: wywali� to st�d!
 //McZapkie-270202
 Controller=AIdriver;
 bDisplayCab=false; //030303
 NextConnected=PrevConnected= NULL;
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
 StartTime=0;
 NoVoltTime=0;
 dPantAngleF=0.0;
 dPantAngleR=0.0;
 PantTraction1=10;
 PantTraction2=10;
 if (!Global::bEnableTraction)
 {
  PantTraction1=5.8;
  PantTraction2=5.8;
 }
 dPantAngleFT=0.0;
 dPantAngleRT=0.0;
 PantWysF=0.0;
 PantWysR=0.0;
 smPatykird1[0]=smPatykird1[1]=NULL;
 smPatykird2[0]=smPatykird2[1]=NULL;
 smPatykirg1[0]=smPatykirg1[1]=NULL;
 smPatykirg2[0]=smPatykirg2[1]=NULL;
 smPatykisl[0]=smPatykisl[1]=NULL;
 pant1x=0;
 pant2x=0;
 panty=0;
 panth=0;
 dDoorMoveL=0.0;
 dDoorMoveR=0.0;
 for (int i=0;i<8;i++)
 {
  DoorSpeedFactor[i]=random(150);
  DoorSpeedFactor[i]=(DoorSpeedFactor[i]+100)/100;
 }
 iAnimatedAxles=0; //ilo�� obracanych osi
 iAnimatedDoors=0;
 for (int i=0;i<MaxAnimatedAxles;i++)
  smAnimatedWheel[i]=NULL;
 for (int i=0;i<MaxAnimatedDoors;i++)
  smAnimatedDoor[i]=NULL;
 mdModel=NULL;
 mdKabina=NULL;
 ReplacableSkinID[0]=0;
 ReplacableSkinID[1]=0;
 ReplacableSkinID[2]=0;
 ReplacableSkinID[3]=0;
 ReplacableSkinID[4]=0;
 iAlpha=0x30300030; //tak gdy tekstury wymienne nie maj� przezroczysto�ci
 smWiazary[0]=smWiazary[1]=NULL;
 smWahacze[0]=smWahacze[1]=smWahacze[2]=smWahacze[3]=NULL;
 fWahaczeAmp=0;
 mdLoad=NULL;
 mdLowPolyInt=NULL;
 mdPrzedsionek=NULL;
 smMechanik=NULL;
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
 iDirection=1; //stoi w kierunku tradycyjnym
 iAxleFirst=0; //numer pierwszej osi w kierunku ruchu (na og�)
 iInventory=0; //flagi bitowe posiadanych submodeli
 RaLightsSet(0,0); //pocz�tkowe zerowanie stanu �wiate�
}

__fastcall TDynamicObject::~TDynamicObject()
{//McZapkie-250302 - zamykanie logowania parametrow fizycznych
 SafeDelete(Mechanik);
 SafeDelete(MoverParameters);
 SafeDelete(TrainParams); //Ra: wywali� to st�d!
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
}

double __fastcall TDynamicObject::Init(
 AnsiString Name, //nazwa pojazdu, np. "EU07-424"
 AnsiString BaseDir, //z kt�rego katalogu wczytany, np. "PKP/EU07"
 AnsiString asReplacableSkin, //nazwa wymiennej tekstury
 AnsiString Type_Name, //nazwa CHK/MMD, np. "303E"
 TTrack *Track, //tor pocz�tkowy wstwawienia (pocz�tek sk�adu)
 double fDist, //dystans wzgl�dem punktu 2
 AnsiString DriverType, //typ obsady
 double fVel, //pr�dko�� pocz�tkowa
 AnsiString TrainName, //nazwa sk�adu, np. "PE2307"
 float Load, //ilo�� �adunku
 AnsiString LoadType, //nazwa �adunku
 bool Reversed //true, je�li ma sta� odwrotnie w sk�adzie
)
{//Ustawienie pocz�tkowe pojazdu
 iDirection=(Reversed?0:1); //Ra: 0, je�li ma by� wstawiony jako obr�cony ty�em
 asBaseDir="dynamic\\"+BaseDir+"\\"; //McZapkie-310302
 asName=Name;
 AnsiString asAnimName=""; //zmienna robocza do wyszukiwania osi i w�zk�w
 TLocation l; //wsp�rz�dne w scenerii
 l.X=l.Y=l.Z=0;
 TRotation r;
 r.Rx=r.Ry=r.Rz=0;
 int Cab; //numer kabiny z obsad� (nie mo�na zaj�� obu)
 if (DriverType==AnsiString("headdriver")) //od przodu sk�adu
  Cab=iDirection?1:-1;
 else if (DriverType==AnsiString("reardriver")) //od ty�u sk�adu
  Cab=iDirection?-1:1;
 else if (DriverType==AnsiString("connected")) //uaktywnianie wirtualnej kabiny
  Cab=iDirection?1:-1;
 else if (DriverType==AnsiString("passenger"))
 {
  if (random(6)<3) Cab=1; else Cab=-1; //losowy przydzia� kabiny
 }
 else if (DriverType==AnsiString("nobody"))
  Cab=0;
 else
 {//obsada nie rozpoznana
  Cab=0;  //McZapkie-010303: w przyszlosci dac tez pomocnika, palacza, konduktora itp.
  Error("Unknown DriverType description: "+DriverType);
  DriverType="nobody";
 }
 //utworzenie parametr�w fizyki
 MoverParameters=new TMoverParameters(l,r,iDirection?fVel:-fVel,Type_Name,asName,Load,LoadType,Cab);
 //McZapkie: TypeName musi byc nazw� CHK/MMD pojazdu
 if (!MoverParameters->LoadChkFile(asBaseDir))
 {//jak wczytanie CHK si� nie uda, to b��d
  Error("Cannot load dynamic object "+asName+" from:\r\n"+BaseDir+"\\"+Type_Name+"\r\nError "+ConversionError+" in line "+LineCount);
  return 0.0;
 }
 bool driveractive=(fVel!=0.0); //je�li pr�dko�� niezerowa, to aktywujemy ruch
 if (!MoverParameters->CheckLocomotiveParameters(driveractive,iDirection?1:-1)) //jak jedzie lub obsadzony to gotowy do drogi
 {
  Error("Parameters mismatch: dynamic object "+asName+" from\n"+BaseDir+"\\"+Type_Name);
  return 0.0;
 }
 if (MoverParameters->CategoryFlag&2) //je�li samoch�d
 {//ustawianie samochodow na poboczu albo na �rodku drogi
  if (Track->fTrackWidth<3.5) //je�li droga w�ska
   MoverParameters->OffsetTrackH=0.0; //to stawiamy na �rodku, niezale�nie od stanu ruchu
  else
  if (driveractive) //od 3.5m do 6.0m jedzie po �rodku pasa, dla szerszych w odleg�o�ci 1.5m
   MoverParameters->OffsetTrackH=Track->fTrackWidth<6.0?-Track->fTrackWidth*0.25:-1.5;
  else //jak stoi, to ko�em na poboczu i pobieramy szeroko�� razem z poboczem, ale nie z chodnikiem
   MoverParameters->OffsetTrackH=-Track->WidthTotal()*0.5+MoverParameters->Dim.W;
 }
 //w wagonie tez niech jedzie
 //if (MoverParameters->MainCtrlPosNo>0 &&
 // if (MoverParameters->CabNo!=0)
 if (DriverType!="nobody")
 {//McZapkie-040602: je�li co� siedzi w poje�dzie
  if (Name==AnsiString(Global::asHumanCtrlVehicle)) //je�li pojazd wybrany do prowadzenia
  {
   if (MoverParameters->EngineType!=Dumb) //i nie jest dumbem
    Controller=Humandriver; //wsadzamy tam steruj�cego
  }
  //McZapkie-151102: rozk�ad jazdy czytany z pliku *.txt z katalogu w kt�rym jest sceneria
  if ((DriverType=="headdriver")||(DriverType=="reardriver"))
  {//McZapkie-110303: mechanik i rozklad tylko gdy jest obsada
   MoverParameters->ActiveCab=MoverParameters->CabNo; //ustalenie aktywnej kabiny (rozrz�d)
   if (MoverParameters->CabNo==-1)
   {//ZiomalCl: je�li AI prowadzi sk�ad w drugiej kabinie (inny kierunek),
    //to musimy zmieni� kabiny (kierunki) w pozosta�ych wagonach/cz�onach
    //inaczej np. cz�on A ET41 b�dzie jecha� w jedn� stron�, a cz�on B w drug�
    MoverParameters->CabDeactivisation();
    MoverParameters->CabActivisation();
   }
   TrainParams=new TTrainParameters(TrainName); //rozk��d jazdy //Ra: wywali� to st�d!
   if (TrainName!="none")
    if (!TrainParams->LoadTTfile(Global::asCurrentSceneryPath))
     Error("Cannot load timetable file "+TrainName+"\r\nError "+ConversionError+" in position "+TrainParams->StationCount);
    else
    {//inicjacja pierwszego przystanku i pobranie jego nazwy
     TrainParams->UpdateMTable(GlobalTime->hh,GlobalTime->mm,TrainParams->NextStationName);
     TrainParams->StationIndexInc(); //przej�cie do nast�pnej
    }
   Mechanik=new TController(Controller,this,TrainParams,Aggressive);
   Mechanik->OrdersInit(fVel); //ustalenie tabelki komend wg rozk�adu
  }
  else
   if (DriverType=="passenger")
   {//obserwator w charakterze pasazera
    //Ra: to jest niebezpieczne, bo w razie co b�dzie pomaga� hamulcem bezpiecze�stwa
    TrainParams=new TTrainParameters(TrainName); //Ra: wywali� to st�d!
    Mechanik=new TController(Controller,this,TrainParams,Easyman);
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
 btHeadSignals11.Init("headlamp13",mdModel,false);
 btHeadSignals12.Init("headlamp11",mdModel,false);
 btHeadSignals13.Init("headlamp12",mdModel,false);
 btHeadSignals21.Init("headlamp23",mdModel,false);
 btHeadSignals22.Init("headlamp21",mdModel,false);
 btHeadSignals23.Init("headlamp22",mdModel,false);
 TurnOff(); //resetowanie zmiennych submodeli
 //wyszukiwanie zderzakow
 for (int i=0;i<2;i++)
 {
  asAnimName=AnsiString("buffer_left0")+(i+1);
  smBuforLewy[i]=mdModel->GetFromName(asAnimName.c_str());
  smBuforLewy[i]->WillBeAnimated(); //ustawienie flagi animacji
  asAnimName=AnsiString("buffer_right0")+(i+1);
  smBuforPrawy[i]=mdModel->GetFromName(asAnimName.c_str());
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
 double HalfMaxAxleDist=Max0R(MoverParameters->BDist,MoverParameters->ADist)*0.5;
 fDist-=0.5*MoverParameters->Dim.L; //dodajemy p� d�ugo�ci pojazdu
 switch (iNumAxles)
 {//Ra: pojazdy wstawiane s� na tor pocz�tkowy, a potem przesuwane
  case 2: //ustawianie osi na torze
   Axle0.Init(Track,this,iDirection?1:-1);
   Axle0.Move((iDirection?fDist:-fDist)+HalfMaxAxleDist+0.01,false);
   Axle1.Init(Track,this,iDirection?1:-1);
   Axle1.Move((iDirection?fDist:-fDist)-HalfMaxAxleDist-0.01,false); //false, �eby nie generowa� event�w
   //Axle2.Init(Track,this,iDirection?1:-1);
   //Axle2.Move((iDirection?fDist:-fDist)-HalfMaxAxleDist+0.01),false);
   //Axle3.Init(Track,this,iDirection?1:-1);
   //Axle3.Move((iDirection?fDist:-fDist)+HalfMaxAxleDist-0.01),false);
  break;
  case 4:
   Axle0.Init(Track,this,iDirection?1:-1);
   Axle0.Move((iDirection?fDist:-fDist)+(HalfMaxAxleDist+MoverParameters->ADist*0.5),false);
   Axle1.Init(Track,this,iDirection?1:-1);
   Axle1.Move((iDirection?fDist:-fDist)-(HalfMaxAxleDist+MoverParameters->ADist*0.5),false);
   //Axle2.Init(Track,this,iDirection?1:-1);
   //Axle2.Move((iDirection?fDist:-fDist)-(HalfMaxAxleDist-MoverParameters->ADist*0.5),false);
   //Axle3.Init(Track,this,iDirection?1:-1);
   //Axle3.Move((iDirection?fDist:-fDist)+(HalfMaxAxleDist-MoverParameters->ADist*0.5),false);
  break;
 }
 Move(0.0); //potrzebne do wyliczenia aktualnej pozycji
 //teraz jeszcze trzeba przypisa� pojazdy do nowego toru,
 //bo przesuwanie pocz�tkowe osi nie zrobi�o tego
 //if (fDist!=0.0) //Ra: taki ma�y patent, �eby lokomotywa ruszy�a, mimo �e stoi na innym torze
 ABuCheckMyTrack(); //zmiana toru na ten, co o� Axle0 (o� z przodu)
 //pOldPos4=Axle1.pPosition; //Ra: nie u�ywane
 //pOldPos1=Axle0.pPosition;
 //ActualTrack= GetTrack(); //McZapkie-030303
 //ABuWozki 060504
 smBogie[0]=mdModel->GetFromName("bogie1"); //Ra: bo nazwy s� ma�ymi
 smBogie[1]=mdModel->GetFromName("bogie2");
 if (!smBogie[0])
  smBogie[0]=mdModel->GetFromName("boogie01"); //Ra: alternatywna nazwa
 if (!smBogie[1])
  smBogie[1]=mdModel->GetFromName("boogie02"); //Ra: alternatywna nazwa
 smBogie[0]->WillBeAnimated();
 smBogie[1]->WillBeAnimated();
 //ABu: zainicjowanie zmiennej, zeby nic sie nie ruszylo
 //w pierwszej klatce, potem juz liczona prawidlowa wartosc masy
 MoverParameters->ComputeConstans();
 if (fVel==0.0) //je�li stoi
  if (MoverParameters->CabNo!=0) //i ma kogo� w kabinie
   if (Track->Event0) //a jest w tym torze event od stania
    RaAxleEvent(Track->Event0); //dodanie eventu stania do kolejki
 return MoverParameters->Dim.L; //d�ugo�� wi�ksza od zera oznacza OK
}
/*
bool __fastcall TDynamicObject::Move(double fDistance)
{
//    fDistance*= fDirection;
//    switch (iNumAxles)
//    {
//        case 4:
//            bEnabled&= Axle3.Move(fDistance,MoverParameters->V<0);
//            bEnabled&= Axle2.Move(fDistance,MoverParameters->V>=0);
//        case 2:
            bEnabled&=Axle1.Move(fDistance,MoverParameters->V>=0);
            bEnabled&=Axle0.Move(fDistance,MoverParameters->V<0);
            Axle3.Move(fDistance,false);
            Axle2.Move(fDistance,false);
//        break;
//    }
}
*/

void __fastcall TDynamicObject::FastMove(double fDistance)
{
   MoverParameters->dMoveLen=MoverParameters->dMoveLen+fDistance;
}

void __fastcall TDynamicObject::Move(double fDistance)
{//przesuwanie pojazdu po trajektorii polega na przesuwaniu poszczeg�lnych osi
 //Ra: warto�� pr�dko�ci 2km/h ma ograniczy� aktywacj� event�w w przypadku drga�
 if (Axle0.GetTrack()==Axle1.GetTrack())
 {//powi�zanie pojazdu z osi� mo�na zmieni� tylko wtedy, gdy skrajne osie s� na tym samym torze
  if (MoverParameters->Vel>2) //|[km/h]| nie ma sensu zmiana osi, jesli pojazd drga na postoju
   iAxleFirst=(MoverParameters->V>=0.0)?1:0; //[m/s] ?1:0 - aktywna druga o� w kierunku jazdy
  //aktualnie eventy aktywuje druga o�, �eby AI nie wy��cza�o sobie semafora za szybko
 }
 bEnabled&=Axle0.Move(fDistance,!iAxleFirst); //o� z przodu pojazdu
 bEnabled&=Axle1.Move(fDistance,iAxleFirst); //o� z ty�u pojazdu
 //Axle2.Move(fDistance,false); //te nigdy pierwsze nie s�
 //Axle3.Move(fDistance,false);
 //liczenie pozycji pojazdu tutaj, bo jest u�ywane w wielu miejscach
 vPosition=0.5*(Axle1.pPosition+Axle0.pPosition); //�rodek mi�dzy skrajnymi osiami
 if (MoverParameters->CategoryFlag&2)
 {
  vPosition.x+=MoverParameters->OffsetTrackH*vLeft.x; //dodanie przesuni�cia w bok
  vPosition.z+=MoverParameters->OffsetTrackH*vLeft.z; //vLeft jest wektorem poprzecznym
  //if () na przechy�ce b�dzie dodatkowo zmiana wysoko�ci samochodu
  vPosition.y+=MoverParameters->OffsetTrackV;   //te offsety sa liczone przez moverparam
 }
};

void __fastcall TDynamicObject::AttachPrev(TDynamicObject *Object, int iType)
{//Ra: doczepia Object na ko�cu sk�adu (nazwa funkcji mo�e by� myl�ca)
 //Ra: u�ywane tylko przy wczytywaniu scenerii
 TLocation loc;
 loc.X=-vPosition.x;
 loc.Y=vPosition.z;
 loc.Z=vPosition.y;                   /* TODO -cBUG : Remove this */
 MoverParameters->Loc=loc; //Ra: ustawienie siebie? po co?
 loc.X=-Object->vPosition.x;
 loc.Y=Object->vPosition.z;
 loc.Z=Object->vPosition.y;
 Object->MoverParameters->Loc=loc; //ustawienie dodawanego pojazdu
 MoverParameters->Attach(iDirection,Object->iDirection^1,&(Object->MoverParameters),iType);
 MoverParameters->Couplers[iDirection].Render=false;
 Object->MoverParameters->Attach(Object->iDirection^1,iDirection,&MoverParameters,iType);
 Object->MoverParameters->Couplers[Object->iDirection^1].Render=true; //rysowanie sprz�gu w do��czanym
 if (!iDirection)
 {//��czenie odwrotne
  PrevConnected=Object; //doczepiamy go sobie do sprz�gu 0, gdy stoimy odwrotnie
  PrevConnectedNo=Object->iDirection^1;
 }
 else
 {//��czenie standardowe
  NextConnected=Object; //doczepiamy go sobie do sprz�gu 1
  NextConnectedNo=Object->iDirection^1;
 }
 if (!Object->iDirection)
 {//do��czany jest odwrotnie ustawiany
  Object->NextConnected=this; //on ma nas z ty�u
  Object->NextConnectedNo=iDirection;
 }
 else
 {//do��czany jest normalnie ustawiany
  Object->PrevConnected=this; //on ma nas z przodu
  Object->PrevConnectedNo=iDirection;
 }
 return;// r;
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
     MoverParameters->ComputeTotalForce(dt, dt1, FullVer);
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
  Global::asCurrentTexturePath=AnsiString(szDefaultTexturePath); //z powrotem defaultowa sciezka do tekstur
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

bool __fastcall TDynamicObject::Update(double dt, double dt1)
{
#ifdef _DEBUG
    if (dt==0) return true; //Ra: pauza
/*
    {
        Error("dt==0");
        dt= 0.001;
    }
*/
#endif
if (!MoverParameters->PhysicActivation)
     return true;   //McZapkie: wylaczanie fizyki gdy nie potrzeba

    if (!bEnabled)
        return false;

//McZapkie-260202
  MoverParameters->BatteryVoltage=90;
  if (MoverParameters->EnginePowerSource.SourceType==CurrentCollector)
   if ((MechInside)||(MoverParameters->TrainType==dt_EZT))
   {
    if ((!MoverParameters->PantCompFlag)&&(MoverParameters->CompressedVolume>=2.8))
     MoverParameters->PantVolume=MoverParameters->CompressedVolume;
    if ((MoverParameters->CompressedVolume<2) && (MoverParameters->PantVolume<3))
    {
     if (!MoverParameters->PantCompFlag)
      MoverParameters->PantVolume=MoverParameters->CompressedVolume;
     MoverParameters->PantFront(false); //opuszczenie pantograf�w przy niskim ci�nieniu
     MoverParameters->PantRear(false);
    }
    //Winger - automatyczne wylaczanie malej sprezarki.
    if (MoverParameters->PantVolume>=5)
     MoverParameters->PantCompFlag=false;
   }

    double dDOMoveLen;

    TLocation l;
    l.X=-vPosition.x;
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
    ts.Len=Max0R(MoverParameters->BDist,MoverParameters->ADist);
    ts.dHtrack=Axle1.pPosition.y-Axle0.pPosition.y; //wektor mi�dzy skrajnymi osiami (!!!odwrotny)
    ts.dHrail=(Axle1.GetRoll()+Axle0.GetRoll())*0.5; //�rednia przechy�ka pud�a
    //TTrackParam tp;
    tp.Width=MyTrack->fTrackWidth;
//McZapkie-250202
    tp.friction=MyTrack->fFriction;
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
   if ((MoverParameters->EnginePowerSource.SourceType==CurrentCollector)||(MoverParameters->TrainType==dt_EZT))
   {
    if (Global::bLiveTraction)
    {
     if ((MoverParameters->PantFrontVolt)||(MoverParameters->PantRearVolt)||
      //ABu: no i sprawdzenie dla EZT:
      ((MoverParameters->TrainType==dt_EZT)&&(MoverParameters->GetTrainsetVoltage())))
      NoVoltTime=0;
     else
      NoVoltTime=NoVoltTime+dt;
     if (NoVoltTime>1.0) //je�li brak zasilania d�u�ej ni� przez 1 sekund�
      tmpTraction.TractionVoltage=0;
     else
      tmpTraction.TractionVoltage=3400; //3550
    }
    else
     tmpTraction.TractionVoltage=3400;
   }
   else
    tmpTraction.TractionVoltage=3400;
   tmpTraction.TractionFreq=0;
   tmpTraction.TractionMaxCurrent=7500; //Ra: chyba za du�o? powinno wywala� przy 1500
   tmpTraction.TractionResistivity=0.3;




//McZapkie: predkosc w torze przekazac do TrackParam
//McZapkie: Vel ma wymiar km/h (absolutny), V ma wymiar m/s , taka przyjalem notacje
    tp.Velmax=MyTrack->fVelocity;

    if (Mechanik)
    {
/*
     //ABu: proba szybkiego naprawienia bledu z zatrzymujacymi sie bez powodu skladami
     if ((MoverParameters->CabNo!=0)&&(Controller!=Humandriver)&&(!MoverParameters->Mains)&&(Mechanik->EngineActive))
     {//Ra: wywali� to st�d!!!!
      MoverParameters->PantRear(false);
      MoverParameters->PantFront(false);
      MoverParameters->PantRear(true);
      MoverParameters->PantFront(true);
      MoverParameters->DecMainCtrl(2); //?
      MoverParameters->MainSwitch(true);
     };
*/
//yB: cos (AI) tu jest nie kompatybilne z czyms (hamulce)
//   if (Controller!=Humandriver)
//    if (Mechanik->LastReactionTime>0.5)
//     {
//      MoverParameters->BrakeCtrlPos=0;
//      Mechanik->LastReactionTime=0;
//     }

      if (Mechanik->UpdateSituation(dt1))  //czuwanie AI
//    if (Mechanik->ScanMe)
      {
       //if (Mechanik) //bo teraz mo�e si� przesia��
       // Mechanik->ScanEventTrack(); //tor pocz�tkowy zale�y od po�o�enia w�zk�w
//       if(MoverParameters->BrakeCtrlPos>0)
//         MoverParameters->BrakeCtrlPos=MoverParameters->BrakeCtrlPosNo;
//       Mechanik->ScanMe= false;
      }
    }
//    else
//    { MoverParameters->SecuritySystemReset(); }
    if (MoverParameters->ActiveCab==0)
        MoverParameters->SecuritySystemReset();
    else
     if ((Controller!=Humandriver)&&(MoverParameters->BrakeCtrlPos<0)&&(!TestFlag(MoverParameters->BrakeStatus,1))&&((MoverParameters->CntrlPipePress)>0.51))
//       {
////        MoverParameters->PipePress=0.50;
        MoverParameters->BrakeCtrlPos=0;
//       }


/* //z EXE Kursa
    if ((MoverParameters->Battery==false)&&(Controller==Humandriver)&& (MoverParameters->EngineType!=DieselEngine) && (MoverParameters->EngineType!=WheelsDriven))
     {
     if (MoverParameters->MainSwitch(False))
      MoverParameters->EventFlag=True;
      }
    if (MoverParameters->TrainType=="et42"){
     if (((TestFlag(MoverParameters->Couplers[1].CouplingFlag,ctrain_controll))&&(MoverParameters->ActiveCab>0)&&(NextConnected-> MoverParameters->TrainType!="et42"))||((TestFlag(MoverParameters->Couplers[0].CouplingFlag,ctrain_controll))&&(MoverParameters->ActiveCab<0)&&(PrevConnected-> MoverParameters->TrainType!="et42")))
     {
     if (MoverParameters->MainSwitch(False))
      MoverParameters->EventFlag=True;
      }
     if ((!(TestFlag(MoverParameters->Couplers[1].CouplingFlag,ctrain_controll))&&(MoverParameters->ActiveCab>0))||(!(TestFlag(MoverParameters->Couplers[0].CouplingFlag,ctrain_controll))&&(MoverParameters->ActiveCab<0)))
      {
     if (MoverParameters->MainSwitch(False))
      MoverParameters->EventFlag=True;
      }
      }
*/


//McZapkie-260202 - dMoveLen przyda sie przy stukocie kol
    dDOMoveLen=GetdMoveLen()+MoverParameters->ComputeMovement(dt,dt1,ts,tp,tmpTraction,l,r);
//yB: zeby zawsze wrzucalo w jedna strone zakretu
    MoverParameters->AccN*=-ABuGetDirection();
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
   double freq;
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
          freq=1.02;
         }
        else
        if (MyTrack->eEnvironment==e_bridge)
         {
          vol*=1.2;
          freq=0.99;                             //MC: stukot w zaleznosci od tego gdzie jest tor
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
if(ObjectDist<50000)
 if(TestFlag(MoverParameters->SoundFlag,sound_brakeacc))
   sBrakeAcc.Play(-1,0,MechInside,vPosition);
 else;
// if(MoverParameters->BrakePress=0)
//   sBrakeAcc.Stop();
else
  sBrakeAcc.Stop();

SetFlag(MoverParameters->SoundFlag,-sound_brakeacc);

/*//z EXE Kursa

      /* if (MoverParameters->TrainType=="et42")
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
/*
        if ((MoverParameters->TrainType=="et40") || (MoverParameters->TrainType=="ep05"))
        {
       /* if ((MoverParameters->MainCtrlPos>MoverParameters->MainCtrlActualPos)&&(abs(MoverParameters->Im)>MoverParameters->IminHi))
          {
          MoverParameters->DecMainCtrl(1);
          } */
/*
          if (( !Pressed(Global::Keys[k_IncMainCtrl]))&&(MoverParameters->MainCtrlPos>MoverParameters->MainCtrlActualPos))
          {
          MoverParameters->DecMainCtrl(1);
          }
          if (( !Pressed(Global::Keys[k_DecMainCtrl]))&&(MoverParameters->MainCtrlPos<MoverParameters->MainCtrlActualPos))
          {
          MoverParameters->IncMainCtrl(1);
          }
        }
*/

 if (MoverParameters->Vel!=0)
 {//McZapkie-050402: krecenie kolami:
  dWheelAngle[0]+=114.59155902616464175359630962821*MoverParameters->V*dt1/MoverParameters->WheelDiameterL; //przednie toczne
  dWheelAngle[1]+=MoverParameters->nrot*dt1*360.0; //nap�dne
  dWheelAngle[2]+=114.59155902616464175359630962821*MoverParameters->V*dt1/MoverParameters->WheelDiameterT; //tylne toczne
  if (dWheelAngle[0]>360.0) dWheelAngle[0]-=360.0; //a w drug� stron� jak si� kr�c�?
  if (dWheelAngle[1]>360.0) dWheelAngle[1]-=360.0;
  if (dWheelAngle[2]>360.0) dWheelAngle[2]-=360.0;
 }
//Winger 160204 - pantografy
if (MoverParameters->EnginePowerSource.SourceType==CurrentCollector)
{
lastcabf=(MoverParameters->CabNo*MoverParameters->DoubleTr);
if (MoverParameters->TrainType==dt_EZT)
  lastcabf=1;
if (lastcabf==0)
 lastcabf=MoverParameters->LastCab;
if (lastcabf==1)
{
pcabc1=MoverParameters->PantFrontUp;
pcabc2=MoverParameters->PantRearUp;
pcabd1=MoverParameters->PantFrontStart;
pcabd2=MoverParameters->PantRearStart;
pcp1p=MoverParameters->PantFrontVolt;
pcp2p=MoverParameters->PantRearVolt;
}
if (lastcabf==-1)
{
pcabc1=MoverParameters->PantRearUp;
pcabc2=MoverParameters->PantFrontUp;
pcabd1=MoverParameters->PantRearStart;
pcabd2=MoverParameters->PantFrontStart;
pcp1p=MoverParameters->PantRearVolt;
pcp2p=MoverParameters->PantFrontVolt;
}

   //double ObjectDist2;
   //double vol2=0;
   double TempPantVol;
   double PantFrontDiff;
   double PantRearDiff;

//   double StartTime= Timer::GetfSinceStart();
   StartTime= StartTime+dt1;
   PantFrontDiff=dPantAngleFT-dPantAngleF;
   if (PantFrontDiff<0)
   PantFrontDiff=-PantFrontDiff;
   if (PantFrontDiff<1)
    {
    if ((pcp1p==false) && (pcp2p==false) && (pcabc1==true))
     sPantUp.Play(vol,0,MechInside,vPosition);
    pcp1p=true;
    }
   else
    pcp1p=false;

   PantRearDiff=dPantAngleRT-dPantAngleR;
   if (PantRearDiff<0)
   PantRearDiff=-PantRearDiff;
   if (PantRearDiff<1)
    {
    if ((pcp2p==false) && (pcp1p==false) && (pcabc2==true))
     sPantUp.Play(vol,0,MechInside,vPosition);
    pcp2p=true;
    }
   else
    pcp2p=false;

   //ObjectDist2=SquareMagnitude(Global::pCameraPosition-vPosition)/100;
   //vol2=255-ObjectDist2;
   //if ((MoverParameters->CompressedVolume<3.3)) //&& (MoverParameters->PantVolume<5.2))
// //  if (MoverParameters->PantVolume<5.2) &&
   // TempPantVol= MoverParameters->PantVolume;
   //else
   // TempPantVol= MoverParameters->CompressedVolume;
   //if (TempPantVol>6)
   // TempPantVol=6;
   //if (MoverParameters->TrainType==dt_EZT)
   // TempPantVol+= 2;
    //if (vol2<0) vol2=0; //Ra: vol2 nie u�ywane dalej
   if (StartTime<2)
    pantspeedfactor=10;
   else
    //ABu: uniezaleznienie od TempPantVol, bo sie krzaczylo...
    pantspeedfactor = 100*dt1;
    //pantspeedfactor = ((TempPantVol-2.5)/5)*180*dt1;

//z EXE Kursa
  //pantspeedfactor = (MoverParameters->PantPress)*20*dt1;

    pantspeedfactor*=abs(MoverParameters->CabNo);
   //if ((PantTraction1==5.8171) && (PantTraction2==5.8171))
   // pantspeedfactor=10;
   if (pantspeedfactor<0)
    pantspeedfactor=0;

//Przedni
   if (PantTraction1>600)
   pcabd1=0;
   dPantAngleFT= -(PantTraction1*28.9-136.938); //*-30+135);
   if ((dPantAngleF>dPantAngleFT) && (pcabc1))
    {
    //dPantAngleF-=5*0.05*pantspeedfactor*(pcabd1*5*(dPantAngleF-dPantAngleFT)+1);
    dPantAngleF-=5*0.05*pantspeedfactor*(pcabd1*5*PantFrontDiff+1);
    if (dPantAngleF<dPantAngleFT)
       {
       dPantAngleF=dPantAngleFT;
       pcabd1=1;
       }
    }
   if (dPantAngleF<-70)
     pcabc1=false;
   if ((dPantAngleF<dPantAngleFT) && (pcabc1) && ((dPantAngleF-dPantAngleFT>0.2) || (dPantAngleFT-dPantAngleF>0.2)))
    {
    //dPantAngleF+=5*0.05*(5*(dPantAngleFT-dPantAngleF)+1)*40*dt1;
    dPantAngleF+=5*0.05*(5*PantFrontDiff+1)*40*dt1;
    if (dPantAngleF>dPantAngleFT)
       {
       dPantAngleF=dPantAngleFT;
       }
    }
   if ((dPantAngleF<0) && (!pcabc1))
    dPantAngleF+=5*0.05*40*dt1;

//Tylny
   if (PantTraction2>600)
   pcabd2=0;
   dPantAngleRT= -(PantTraction2*28.9-136.938);
//ABu: ponizej tylko dla testow:
//    dPantAngleR=dPantAngleRT;

   if ((dPantAngleR>dPantAngleRT) && (pcabc2))
   {
    dPantAngleR-=5*0.05*pantspeedfactor*(pcabd2*5*(dPantAngleR-dPantAngleRT)+1);
    if (dPantAngleR<dPantAngleRT)
    {
     dPantAngleR=dPantAngleRT;
     pcabd2=1;
    }
   }
   if (dPantAngleR<-70)
     pcabc2=false;
   if ((dPantAngleR<dPantAngleRT) && (pcabc2) && ((dPantAngleR-dPantAngleRT>0.2) || (dPantAngleRT-dPantAngleR>0.2)))
    {
     dPantAngleR+=5*0.05*(5*(dPantAngleRT-dPantAngleR)+1)*40*dt1;
     if (dPantAngleR>dPantAngleRT)
      dPantAngleR=dPantAngleRT;
    }
   if ((dPantAngleR<0) && (!pcabc2))
    dPantAngleR+=5*0.05*40*dt1;

if (lastcabf==1)
{
MoverParameters->PantFrontUp=pcabc1;
MoverParameters->PantRearUp=pcabc2;
MoverParameters->PantFrontStart=pcabd1;
MoverParameters->PantRearStart=pcabd2;
MoverParameters->PantFrontVolt=pcp1p;
MoverParameters->PantRearVolt=pcp2p;
}
if (lastcabf==-1)
{
MoverParameters->PantRearUp=pcabc1;
MoverParameters->PantFrontUp=pcabc2;
MoverParameters->PantRearStart=pcabd1;
MoverParameters->PantFrontStart=pcabd2;
MoverParameters->PantRearVolt=pcp1p;
MoverParameters->PantFrontVolt=pcp2p;
}

if ((MoverParameters->PantFrontSP==false) && (MoverParameters->PantFrontUp==false))
    {
    sPantDown.Play(vol,0,MechInside,vPosition);
    MoverParameters->PantFrontSP=true;
    }
if ((MoverParameters->PantRearSP==false) && (MoverParameters->PantRearUp==false))
    {
    sPantDown.Play(vol,0,MechInside,vPosition);
    MoverParameters->PantRearSP=true;
    }

//Winger 240404 - wylaczanie sprezarki i przetwornicy przy braku napiecia
if (tmpTraction.TractionVoltage==0)
    {
    MoverParameters->ConverterFlag=false;
    MoverParameters->CompressorFlag=false;
    }
}
 else if (MoverParameters->EnginePowerSource.SourceType==InternalSource)
  if (MoverParameters->EnginePowerSource.PowerType==SteamPower)
  {//Ra: animacja rozrz�du parowozu, na razie nieoptymalizowane
   double fi,dx,c2,ka,kc;
   double sin_fi,cos_fi;
   double L1=1.6688888888888889;
   double L2=5.6666666666666667; //2550/450
   double Lc=0.4;
   double L=5.686422222; //2558.89/450
   double G1,G2,G3,ksi,sin_ksi,gam;
   double G1_2,G2_2,G3_2; //kwadraty
   //ruch t�ok�w oraz korbowod�w
   fi=DegToRad(dWheelAngle[1]+pant1x); //k�t obrotu ko�a dla t�oka 1
   sin_fi=sin(fi);
   cos_fi=cos(fi);
   dx=panty*cos_fi+sqrt(panth*panth-panty*panty*sin_fi*sin_fi)-panth; //nieoptymalne
   if (smPatykird1[0]) //na razie zabezpieczenie
    smPatykird1[0]->SetTranslate(float3(dx,0,0));
   ka=-asin(panty/panth)*sin_fi;
   if (smPatykirg1[0]) //na razie zabezpieczenie
    smPatykirg1[0]->SetRotateXYZ(vector3(RadToDeg(ka),0,0));
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
   fi=DegToRad(dWheelAngle[1]+pant1x-96.77416667); //k�t obrotu ko�a dla t�oka 1
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
   if (smPatykirg2[0])
    smPatykirg2[0]->SetRotateXYZ(vector3(RadToDeg(ksi),0,0)); //obr�cenie jarzma
   //1) ksi=-23�, gam=
   //2) ksi=10�, gam=
   //gam=acos((L2-sin_ksi-Lc*cos_fi)/L); //k�t od poziomu, liczony wzgl�dem poziomu
   //gam=asin((L1-cos_ksi-Lc*sin_fi)/L); //k�t od poziomu, liczony wzgl�dem pionu
   gam=atan2((L1-cos(ksi)+Lc*sin_fi),(L2-sin_ksi-Lc*cos_fi)); //k�t od poziomu
   //fi4=acos((l1+l2*cos(fi2)+l3*cos(fi3))/-l4); //k�t obrotu dr��ka mimo�rodowego wzgl�dem jarzma
   //c2=rm*rm*sin(fi)*sin(fi)+(d-rm*cos(fi))*(d-rm*cos(fi)); //kw. odleg�o�ci osi mimo�rodu od osi jarzma
   //ka=acos((-a*a+b*b+c2)/(2.0*b*sqrt(c)))+kj; //k�t jarzma
   //kc=acos((-c2+b*b+a*a)/(2.0*b*a))+kd; //k�t dr��ka mimo�rodowego (jest zaczepiony do jarzma)
   if (smPatykird2[0]) //na razie zabezpieczenie
    smPatykird2[0]->SetRotateXYZ(vector3(-90.0+RadToDeg(+gam+ksi),0,0)); //obr�cenie dr��ka mimo�rodowego
//--- druga strona---
   fi=DegToRad(dWheelAngle[1]+pant2x); //k�t obrotu ko�a dla t�oka 1
   sin_fi=sin(fi);
   cos_fi=cos(fi);
   dx=panty*cos_fi+sqrt(panth*panth-panty*panty*sin_fi*sin_fi)-panth; //nieoptymalne
   if (smPatykird1[1]) //na razie zabezpieczenie
    smPatykird1[1]->SetTranslate(float3(dx,0,0));
   ka=-asin(panty/panth)*sin_fi;
   if (smPatykirg1[1]) //na razie zabezpieczenie
    smPatykirg1[1]->SetRotateXYZ(vector3(RadToDeg(ka),0,0));
   //smPatykirg1[1]->SetRotate(float3(0,1,0),RadToDeg(fi));
   fi=DegToRad(dWheelAngle[1]+pant2x+96.77416667); //k�t obrotu ko�a dla t�oka 1
   sin_fi=sin(fi);
   cos_fi=cos(fi);
   G1=(1.0+L1*L1+L2*L2+Lc*Lc-L*L-2.0*Lc*L2*cos_fi+2.0*Lc*L1*sin_fi);
   G1_2=G1*G1;
   G2=2.0*(L2-Lc*cos_fi);
   G2_2=G2*G2;
   G3=2.0*(L1-Lc*sin_fi);
   G3_2=G3*G3;
   ksi=asin((G1*G2-G3*_fm_sqrt(G2_2+G3_2-G1_2))/(G2_2+G3_2)); //k�t jarzma
   if (smPatykirg2[1])
    smPatykirg2[1]->SetRotateXYZ(vector3(RadToDeg(ksi),0,0)); //obr�cenie jarzma
   gam=atan2((L1-cos(ksi)+Lc*sin_fi),(L2-sin_ksi-Lc*cos_fi)); //k�t od poziomu
   if (smPatykird2[1]) //na razie zabezpieczenie
    smPatykird2[1]->SetRotateXYZ(vector3(-90.0+RadToDeg(+gam+ksi),0,0)); //obr�cenie dr��ka mimo�rodowego
  }

//NBMX Obsluga drzwi, MC: zuniwersalnione
//   if (tempdoorfactor2!=120)
//    tempdoorfactor=random(100);
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
  fTrackBlock=10000.0; //na razie nie ma przeszk�d
  //je�li nie ma zwrotnicy po drodze, to tylko przeliczy� odleg�o��?
  if (MoverParameters->V>0.1) //je�li jedzie do przodu (w kierunku Coupler 0)
  {if (MoverParameters->Couplers[0].CouplingFlag==ctrain_virtual) //brak pojazdu podpi�tego?
   {ABuScanObjects(1,300); //szukanie czego� do pod��czenia
    //WriteLog(asName+" - block 0: "+AnsiString(fTrackBlock));
   }
  }
  else if (MoverParameters->V<-0.1) //je�li jedzie do ty�u (w kierunku Coupler 1)
   if (MoverParameters->Couplers[1].CouplingFlag==ctrain_virtual) //brak pojazdu podpi�tego?
   {ABuScanObjects(-1,300);
    //WriteLog(asName+" - block 1: "+AnsiString(fTrackBlock));
   }
  CouplCounter=random(20); //ponowne sprawdzenie po losowym czasie
 }
 if (MoverParameters->V!=0.0)
  ++CouplCounter; //jazda sprzyja poszukiwaniu po��czenia
 else
 {CouplCounter=25; //a bezruch nie, ale mo�na zaktualizowa� odleg�o��
  if (MoverParameters->Couplers[1-iDirection].CouplingFlag==ctrain_virtual)
  {if (MoverParameters->Couplers[1-iDirection].Connected) //je�li jest pojazd na sprz�gu wirtualnym
    fTrackBlock=MoverParameters->Couplers[1-iDirection].CoupleDist; //aktualizacja odleg�o�ci od niego
   else
    if (fTrackBlock<=50.0) //je�eli pojazdu nie ma, a odleg�o�� jako� ma�a
     ABuScanObjects(iDirection?1:-1,300); //skanowanie sprawdzaj�ce
   //WriteLog(asName+" - block x: "+AnsiString(fTrackBlock));
  }
 }
 if (MoverParameters->DerailReason>0)
 {switch (MoverParameters->DerailReason)
  {case 1: WriteLog(asName+" derailed due to end of track"); break;
   case 2: WriteLog(asName+" derailed due to too high speed"); break;
   case 3: WriteLog(asName+" derailed due to track width"); break;
   case 4: WriteLog(asName+" derailed due to wrong track type"); break;
  }
  MoverParameters->DerailReason=0; //�eby tylko raz
 }
 if (MoverParameters->LoadStatus)
  LoadUpdate(); //zmiana modelu �adunku
 return true; //Ra: chyba tak?
}

bool __fastcall TDynamicObject::FastUpdate(double dt)
{
#ifdef _DEBUG
    if (dt==0.0) return true; //Ra: pauza
/*
    {
        Error("dt==0");
        dt= 0.001;
    }
*/
#endif
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
    //if (Mechanik)
    //{
    //
    //    if (Mechanik->UpdateSituation(dt))  //czuwanie AI
//  //       if (Mechanik->ScanMe)
    //      {
    //        ScanEventTrack();
//  //          Mechanik->ScanMe= false;
    //      }
    //
    //}
    dDOMoveLen=MoverParameters->FastComputeMovement(dt,ts,tp,l,r); // ,ts,tp,tmpTraction);
    //Move(dDOMoveLen);
    //ResetdMoveLen();
    FastMove(dDOMoveLen);

//yB: przyspieszacz (moze zadziala, ale dzwiek juz jest)
double ObjectDist;
ObjectDist=SquareMagnitude(Global::pCameraPosition-vPosition);
if(ObjectDist<50000)
 if(TestFlag(MoverParameters->SoundFlag,sound_brakeacc))
   sBrakeAcc.Play(-1,0,MechInside,vPosition);
 else;
// if(MoverParameters->BrakePress=0)
//   sBrakeAcc.Stop();
else
  sBrakeAcc.Stop();

SetFlag(MoverParameters->SoundFlag,-sound_brakeacc);
 if (MoverParameters->LoadStatus)
  LoadUpdate(); //zmiana modelu �adunku
 return true; //Ra: chyba tak?
}

//McZapkie-040402: liczenie pozycji uwzgledniajac wysokosc szyn itp.
vector3 inline __fastcall TDynamicObject::GetPosition()
{//Ra: pozycja pojazdu jest liczona zaraz po przesuni�ciu
 return vPosition;
}

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

bool __fastcall TDynamicObject::Render()
{//rysowanie element�w nieprzezroczystych
 //youBy - sprawdzamy, czy jest sens renderowac
 double modelrotate;
 vector3 tempangle;
 // zmienne
 renderme=false;
 //przeklejka
 vector3 pos=vPosition;
 double ObjSqrDist=SquareMagnitude(Global::pCameraPosition-pos);
 //koniec przeklejki
 if (ObjSqrDist<500) //jak jest blisko - do 70m
  modelrotate=0.01f; //ma�y k�t, �eby nie znika�o
 else
 {//Global::pCameraRotation to k�t bewzgl�dny w �wiecie (zero - na po�udnie)
  tempangle=(pos-Global::pCameraPosition); //wektor od kamery
  modelrotate=ABuAcos(tempangle); //okre�lenie k�ta
  if (modelrotate>M_PI) modelrotate-=(2*M_PI);
  modelrotate+=Global::pCameraRotation;
 }
 if (modelrotate>M_PI) modelrotate-=(2*M_PI);
 if (modelrotate<-M_PI) modelrotate+=(2*M_PI);

 modelrotate=abs(modelrotate);

 if (modelrotate<maxrot) renderme=true;

 if (renderme)
 {
  TSubModel::iInstance=(int)this; //�eby nie robi� cudzych animacji
  AnsiString asLoadName="";
  //przej�cie na uk�ad wsp�rz�dnych modelu - tu si� zniekszta�ca?
  vFront=GetDirection();
  if ((MoverParameters->CategoryFlag&2) && (MoverParameters->CabNo<0)) //TODO: zrobic to eleganciej z plynnym zawracaniem
   vFront=-vFront;
  vUp=vWorldUp; //sta�a
  vFront.Normalize();
  vLeft=CrossProduct(vUp,vFront);
  vUp=CrossProduct(vFront,vLeft);
  matrix4x4 mat;
  mat.Identity();

  mat.BasisChange(vLeft,vUp,vFront);
  mMatrix=Inverse(mat);

  vector3 pos=vPosition;
  double ObjSqrDist=SquareMagnitude(Global::pCameraPosition-pos);
  ABuLittleUpdate(ObjSqrDist); //ustawianie zmiennych submodeli

  //ActualTrack= GetTrack(); //McZapkie-240702

#if RENDER_CONE
  {//Ra: testowe renderowanie pozycji w�zk�w w postaci ostros�up�w, wymaga GLUT32.DLL
   double dir=RadToDeg(atan2(vLeft.z,vLeft.x));
   Axle0.Render(0);
   Axle1.Render(1); //bogieRot[0]
   //if (PrevConnected) //renderowanie po��czenia
  }
#endif

  glPushMatrix ( );
  //vector3 pos= vPosition;
  //double ObjDist= SquareMagnitude(Global::pCameraPosition-pos);
  glTranslated(pos.x,pos.y,pos.z);
  glMultMatrixd(mMatrix.getArray());
  if (mdLowPolyInt)
   if (FreeFlyModeFlag?true:!mdKabina)
#ifdef USE_VBO
    if (Global::bUseVBO)
     mdLowPolyInt->RaRender(ObjSqrDist,ReplacableSkinID,iAlpha);
    else
#endif
     mdLowPolyInt->Render(ObjSqrDist,ReplacableSkinID,iAlpha);

#ifdef USE_VBO
  if (Global::bUseVBO)
   mdModel->RaRender(ObjSqrDist,ReplacableSkinID,iAlpha);
  else
#endif
   mdModel->Render(ObjSqrDist,ReplacableSkinID,iAlpha);
  if (mdLoad) //renderowanie nieprzezroczystego �adunku
#ifdef USE_VBO
   if (Global::bUseVBO)
    mdLoad->RaRender(ObjSqrDist,ReplacableSkinID,iAlpha);
   else
#endif
    mdLoad->Render(ObjSqrDist,ReplacableSkinID,iAlpha);

//rendering przedsionkow o ile istnieja
  if (mdPrzedsionek)
   //if (MoverParameters->filename==asBaseDir+"6ba.chk") //Ra: to tu bez sensu by�o
#ifdef USE_VBO
   if (Global::bUseVBO)
    mdPrzedsionek->RaRender(ObjSqrDist,ReplacableSkinID,iAlpha);
   else
#endif
    mdPrzedsionek->Render(ObjSqrDist,ReplacableSkinID,iAlpha);
//rendering kabiny gdy jest oddzielnym modelem i ma byc wyswietlana
//ABu: tylko w trybie FreeFly, zwykly tryb w world.cpp

  if ((mdKabina!=mdModel) && bDisplayCab && FreeFlyModeFlag)
  {//Ra: a �wiet�a nie zosta�y ju� ustawione dla toru?
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
#ifdef USE_VBO
   if (Global::bUseVBO)
    mdKabina->RaRender(ObjSqrDist,0);
   else
#endif
    mdKabina->Render(ObjSqrDist,0);
   glLightfv(GL_LIGHT0,GL_AMBIENT,Global::ambientDayLight);
   glLightfv(GL_LIGHT0,GL_DIFFUSE,Global::diffuseDayLight);
   glLightfv(GL_LIGHT0,GL_SPECULAR,Global::specularDayLight);
  }
  glPopMatrix();
  if (btnOn) TurnOff(); //przywr�cenie domy�lnych pozycji submodeli
 } //yB - koniec mieszania z grafika


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
            if (MoverParameters->DynamicBrakeFlag==true) //Szociu - 29012012 - je�eli uruchomiony jest  hamulec elektrodynamiczny, odtwarzany jest d�wi�k silnika
            {
             vol +=0.8;
            }
            else
            {
            }
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
      if ((MoverParameters->Vel>0.01) && (!MoverParameters->SlippingWheels) && (MoverParameters->UnitBrakeForce>rsPisk.AM))
       {
        vol=MoverParameters->UnitBrakeForce/(rsPisk.AM+1)+rsPisk.AA;
        rsPisk.Play(vol,DSBPLAY_LOOPING,MechInside,GetPosition());
       }
      else
       rsPisk.Stop();
     }

//if ((MoverParameters->ConverterFlag==false) && (MoverParameters->TrainType!=dt_ET22))
if ((MoverParameters->ConverterFlag==false)&&(MoverParameters->CompressorPower!=0))
 MoverParameters->CompressorFlag=false;
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
//if ((MoverParameters->MainCtrlPos)>=(MoverParameters->TurboTest))
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
//else sTurbo.TurnOff(MechInside,GetPosition());



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
 return true;
};

bool __fastcall TDynamicObject::RenderAlpha()
{//rysowanie element�w p�przezroczystych
 if (renderme)
 {
  TSubModel::iInstance=(int)this; //�eby nie robi� cudzych animacji
  vFront= GetDirection();
  if ((MoverParameters->CategoryFlag&2) && (MoverParameters->CabNo<0)) //TODO: zrobic to eleganciej z plynnym zawracaniem
   vFront=-vFront;
  vUp=vWorldUp; //Ra: je�li to wskazuje pionowo w g�r�
  vFront.Normalize(); //a to w d� lub w g�r�, to mamy problem z ortogonalno�ci� i skalowaniem
  vLeft=CrossProduct(vUp,vFront);
  vUp=CrossProduct(vFront,vLeft);
  matrix4x4 mat;
  mat.Identity();
  mat.BasisChange(vLeft,vUp,vFront);
  mMatrix=Inverse(mat);
  vector3 pos=GetPosition();
  double ObjSqrDist=SquareMagnitude(Global::pCameraPosition-pos);
  ABuLittleUpdate(ObjSqrDist);
    glPushMatrix ( );
    glTranslated(pos.x,pos.y,pos.z);
    glMultMatrixd(mMatrix.getArray());

#ifdef USE_VBO
    if (Global::bUseVBO)
    {glEnable(GL_BLEND); //dodane zabezpieczenie przed problemami z wy�wietlaniem cieni przy VBO
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER,0.04);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_LEQUAL); 
    mdModel->RaRenderAlpha(ObjSqrDist,ReplacableSkinID,iAlpha);}
    else
#endif
     mdModel->RenderAlpha(ObjSqrDist,ReplacableSkinID,iAlpha);

    if (mdLoad) //Ra: dodane renderowanie przezroczystego �adunku
#ifdef USE_VBO
     if (Global::bUseVBO)
      mdLoad->RaRenderAlpha(ObjSqrDist,ReplacableSkinID,iAlpha);
     else
#endif
      mdLoad->RenderAlpha(ObjSqrDist,ReplacableSkinID,iAlpha);

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
  glPopMatrix ( );
  if (btnOn) TurnOff(); //przywr�cenie domy�lnych pozycji submodeli
 }
 return true;
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
 fs=new TFileStream(asFileName,fmOpenRead|fmShareCompat);
 AnsiString str="";
 int size=fs->Size;
 AnsiString asAnimName="";
 bool Stop_InternalData=false;
 str.SetLength(size);
 fs->Read(str.c_str(),size);
 str+="";
 delete fs;
 TQueryParserComp *Parser;
 Parser=new TQueryParserComp(NULL);
 Parser->TextToParse=str;
 //Parser->LoadStringToParse(asFile);
 Parser->First();
 //DecimalSeparator= '.';
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
       {
        ReplacableSkin=Global::asCurrentTexturePath+ReplacableSkin;      //skory tez z dynamic/...
        ReplacableSkinID[1]=TTexturesManager::GetTextureID(ReplacableSkin.c_str(),Global::iDynamicFiltering);
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
       Global::asCurrentTexturePath=AnsiString(szDefaultTexturePath); //z powrotem defaultowa sciezka do tekstur
       while (!Parser->EndOfFile && str!=AnsiString("endmodels"))
       {
        str=Parser->GetNextSymbol().LowerCase();
        if (str==AnsiString("lowpolyinterior:")) //ABu: wnetrze lowpoly
        {
         asModel=Parser->GetNextSymbol().LowerCase();
         asModel=BaseDir+asModel; //McZapkie-200702 - dynamics maja swoje modele w dynamics/basedir
         Global::asCurrentTexturePath=BaseDir;                    //biezaca sciezka do tekstur to dynamic/...
         mdLowPolyInt=TModelsManager::GetModel(asModel.c_str(),true);
        }
        else if (str==AnsiString("animwheelprefix:")) //prefiks krecacych sie kol
        {
         int i,j,k,m;
         str=Parser->GetNextSymbol();
         for (i=1; i<=MaxAnimatedAxles; i++)
         {//McZapkie-050402: wyszukiwanie kol o nazwie str*
          asAnimName=str+i;
          smAnimatedWheel[i-1]=mdModel->GetFromName(asAnimName.c_str());
          if (smAnimatedWheel[i-1])
          {++iAnimatedAxles;
           smAnimatedWheel[i-1]->WillBeAnimated(); //wy��czenie optymalizacji transformu
          }
          else break; //wyj�cie z p�tli
         }
         //Ra: ustawianie indeks�w osi
         for (i=0;i<=MaxAnimatedAxles;++i) //zabezpieczenie przed b��dami w CHK
          pWheelAngle[i]=dWheelAngle+1; //domy�lnie wska�nik na nap�dzaj�ce
         i=0; j=1; k=0; m=0; //numer osi; kolejny znak; ile osi danego typu; kt�ra �rednica
         if ((MoverParameters->WheelDiameterL!=MoverParameters->WheelDiameter)||(MoverParameters->WheelDiameterT!=MoverParameters->WheelDiameter))
          while ((i<iAnimatedAxles)&&(j<=MoverParameters->AxleArangement.Length()))
          {//wersja ze wska�nikami jest bardziej elatyczna na nietypowe uk�ady
           if ((k>='A')&&(k<='J')) //10 chyba maksimum?
           {pWheelAngle[i++]=dWheelAngle+1; //obr�t osi nap�dzaj�cych
            --k; //nast�pna b�dzie albo taka sama, albo bierzemy kolejny znak
            m=2; //nast�puj�ce toczne b�d� mia�y inn� �rednic�
           }
           else if ((k>='1')&&(k<='9'))
           {pWheelAngle[i++]=dWheelAngle+m; //obr�t osi tocznych
            --k; //nast�pna b�dzie albo taka sama, albo bierzemy kolejny znak
           }
           else
            k=MoverParameters->AxleArangement[j++]; //pobranie kolejnego znaku
          } //Ra: p�tla uruchamiana tylko je�li s� r�ne �rednice
        }
        else if (str==AnsiString("animrodprefix:")) //prefiks wiazarow dwoch
         {
          str= Parser->GetNextSymbol();
          for (int i=1; i<=2; i++)
           {
 //McZapkie-050402: wyszukiwanie max 2 wiazarow o nazwie str*
            asAnimName=str+i;
            smWiazary[i-1]=mdModel->GetFromName(asAnimName.c_str());
            smWiazary[i-1]->WillBeAnimated();
           }
         }
        else
//Pantografy - Winger 160204
        if (str==AnsiString("animpantrd1prefix:"))              //prefiks ramion dolnych 1
         {
          str= Parser->GetNextSymbol();
          for (int i=1; i<=2; i++)
           {
 //Winger 160204: wyszukiwanie max 2 patykow o nazwie str*
            asAnimName=str+i;
            smPatykird1[i-1]=mdModel->GetFromName(asAnimName.c_str());
            smPatykird1[i-1]->WillBeAnimated();
           }
         }
        else
        if (str==AnsiString("animpantrd2prefix:"))              //prefiks ramion dolnych 2
         {
          str= Parser->GetNextSymbol();
          for (int i=1; i<=2; i++)
           {
 //Winger 160204: wyszukiwanie max 2 patykow o nazwie str*
            asAnimName=str+i;
            smPatykird2[i-1]=mdModel->GetFromName(asAnimName.c_str());
            smPatykird2[i-1]->WillBeAnimated();
           }
         }
        else
        if (str==AnsiString("animpantrg1prefix:"))              //prefiks ramion gornych 1
         {
          str= Parser->GetNextSymbol();
          for (int i=1; i<=2; i++)
           {
 //Winger 160204: wyszukiwanie max 2 patykow o nazwie str*
            asAnimName=str+i;
            smPatykirg1[i-1]=mdModel->GetFromName(asAnimName.c_str());
            smPatykirg1[i-1]->WillBeAnimated();
           }
         }
        else
        if (str==AnsiString("animpantrg2prefix:"))              //prefiks ramion gornych 2
         {
          str= Parser->GetNextSymbol();
          for (int i=1; i<=2; i++)
           {
 //Winger 160204: wyszukiwanie max 2 patykow o nazwie str*
            asAnimName=str+i;
            smPatykirg2[i-1]=mdModel->GetFromName(asAnimName.c_str());
            smPatykirg2[i-1]->WillBeAnimated();
           }
         }
        else
        if (str==AnsiString("animpantslprefix:"))              //prefiks slizgaczy
         {
          str= Parser->GetNextSymbol();
          for (int i=1; i<=2; i++)
           {
 //Winger 160204: wyszukiwanie max 2 patykow o nazwie str*
            asAnimName=str+i;
            smPatykisl[i-1]=mdModel->GetFromName(asAnimName.c_str());
            smPatykisl[i-1]->WillBeAnimated();
           }
         }
        else if (str==AnsiString("pantfactors:"))
        {//Winger 010304: parametry pantografow
         pant1x=Parser->GetNextSymbol().ToDouble();
         pant2x=Parser->GetNextSymbol().ToDouble();
         panty=Parser->GetNextSymbol().ToDouble();
         panth=Parser->GetNextSymbol().ToDouble();
        }
        else if (str==AnsiString("animpistonprefix:"))
        {//prefiks t�oczysk - na razie u�ywamy modeli pantograf�w
         str=Parser->GetNextSymbol();
         for (int i=1;i<=2;i++)
         {
          asAnimName=str+i;
          smPatykird1[i-1]=mdModel->GetFromName(asAnimName.c_str());
          smPatykird1[i-1]->WillBeAnimated();
         }
        }
        else if (str==AnsiString("animconrodprefix:"))
        {//prefiks korbowod�w - na razie u�ywamy modeli pantograf�w
         str=Parser->GetNextSymbol();
         for (int i=1;i<=2;i++)
         {
          asAnimName=str+i;
          smPatykirg1[i-1]=mdModel->GetFromName(asAnimName.c_str());
          smPatykirg1[i-1]->WillBeAnimated();
         }
        }
        else if (str==AnsiString("pistonfactors:"))
        {//Ra: parametry silnika parowego (t�oka)
         pant1x=Parser->GetNextSymbol().ToDouble(); //k�t przesuni�cia dla pierwszego t�oka
         pant2x=Parser->GetNextSymbol().ToDouble(); //k�t przesuni�cia dla drugiego t�oka
         panty=Parser->GetNextSymbol().ToDouble(); //d�ugo�� korby (r)
         panth=Parser->GetNextSymbol().ToDouble(); //d�ugo� korbowodu (k)
         MoverParameters->EnginePowerSource.PowerType=SteamPower; //Ra: po chamsku, ale z CHK nie dzia�a
        }
        else if (str==AnsiString("animreturnprefix:"))
        {//prefiks dr��ka mimo�rodowego - na razie u�ywamy modeli pantograf�w
         str=Parser->GetNextSymbol();
         for (int i=1;i<=2;i++)
         {
          asAnimName=str+i;
          smPatykird2[i-1]=mdModel->GetFromName(asAnimName.c_str());
          smPatykird2[i-1]->WillBeAnimated();
         }
        }
        else if (str==AnsiString("animexplinkprefix:")) //animreturnprefix:
        {//prefiks jarzma - na razie u�ywamy modeli pantograf�w
         str=Parser->GetNextSymbol();
         for (int i=1;i<=2;i++)
         {
          asAnimName=str+i;
          smPatykirg2[i-1]=mdModel->GetFromName(asAnimName.c_str());
          smPatykirg2[i-1]->WillBeAnimated();
         }
        }
        else if (str==AnsiString("animpendulumprefix:"))
        {//prefiks wahaczy
          str= Parser->GetNextSymbol();
          asAnimName="";
          for (int i=1; i<=4; i++)
           {
 //McZapkie-050402: wyszukiwanie max 4 wahaczy o nazwie str*
            asAnimName=str+i;
            smWahacze[i-1]=mdModel->GetFromName(asAnimName.c_str());
            smWahacze[i-1]->WillBeAnimated();
           }
          str= Parser->GetNextSymbol().LowerCase();
          if (str==AnsiString("pendulumamplitude:"))
           fWahaczeAmp= Parser->GetNextSymbol().ToDouble();
         }
        else
        if (str==AnsiString("engineer:"))              //nazwa submodelu maszynisty
         {
          str=Parser->GetNextSymbol();
          smMechanik=mdModel->GetFromName(str.c_str());
         }
        else
        if (str==AnsiString("animdoorprefix:"))           //nazwa animowanych dzwi
        {
         str= Parser->GetNextSymbol();
         asAnimName="";
         for (int i=1; i<=MaxAnimatedDoors; i++)
         {//NBMX wrzesien 2003: wyszukiwanie drzwi o nazwie str*
          asAnimName=str+i;
          smAnimatedDoor[i-1]=mdModel->GetFromName(asAnimName.c_str());
          if (smAnimatedDoor[i-1])
          {++iAnimatedDoors+=1;
           smAnimatedDoor[i-1]->WillBeAnimated();
          }
          else
           i=MaxAnimatedDoors+1;
         }
        }
       }
     }
     else
     if (str==AnsiString("sounds:"))                            //dzwieki
      while (!Parser->EndOfFile && str!=AnsiString("endsounds"))
      {
       str= Parser->GetNextSymbol().LowerCase();
       if (str==AnsiString("wheel_clatter:"))                    //polozenia osi w/m srodka pojazdu
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
         rsSilnik.Init(str.c_str(),Parser->GetNextSymbol().ToDouble(),GetPosition().x,GetPosition().y,GetPosition().z,true);
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
         rsWentylator.Init(str.c_str(),Parser->GetNextSymbol().ToDouble(),GetPosition().x,GetPosition().y,GetPosition().z,true);
         rsWentylator.AM=Parser->GetNextSymbol().ToDouble()/MoverParameters->RVentnmax;
         rsWentylator.AA=Parser->GetNextSymbol().ToDouble();
         rsWentylator.FM=Parser->GetNextSymbol().ToDouble()/MoverParameters->RVentnmax;
         rsWentylator.FA=Parser->GetNextSymbol().ToDouble();
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
         sBrakeAcc.Init(str.c_str(),Parser->GetNextSymbol().ToDouble(),GetPosition().x,GetPosition().y,GetPosition().z,true);
         sBrakeAcc.AM=1.0;
         sBrakeAcc.AA=0.0;
         sBrakeAcc.FM=1.0;
         sBrakeAcc.FA=0.0;
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
        }
       if (str==AnsiString("departuresignal:"))            //pliki z sygnalem odjazdu
        {
         sDepartureSignal.Load(Parser,GetPosition());
         }
       if (str==AnsiString("pantographup:"))            //pliki dzwiekow pantografow
        {
         str= Parser->GetNextSymbol();
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
      Stop_InternalData= true;
 }
 //ABu 050205 - tego wczesniej nie bylo i uciekala pamiec:
 delete Parser;
 if (mdModel) mdModel->Init(); //obr�cenie modelu oraz optymalizacja, r�wnie� zapisanie binarnego
 if (mdLoad) mdLoad->Init();
 if (mdPrzedsionek) mdPrzedsionek->Init();
 if (mdLowPolyInt) mdLowPolyInt->Init();
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
 if (rear==2+32+64)
 {//je�li koniec poci�gu, to trzeba ustali�, czy jest tam czynna lokomotywa
  //EN57 mo�e nie mie� ko�c�wek od �rodka cz�onu
  //je�li ma zar�wno �wiat�a jak i ko�c�wki, ustali�, czy jest w stanie aktywnym
  //np. lokomotywa na zimno b�dzie mie� ko�c�wki a nie �wiat�a
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

void __fastcall TDynamicObject::RaAxleEvent(TEvent *e)
{//obs�uga eventu wykrytego przez w�zek - je�li steruj�cy pr�dko�ci�, to ignorujemy
 //bo mamy w�asny mechanizm obs�ugi tych event�w i nie musz� i�� do kolejki
 if (Mechanik) //tylko je�li ma obsad�
 {if (!Mechanik->CheckEvent(e,true)) //je�li nie jest ustawiaj�cym pr�dko��
   Global::pGround->AddToQuery(e,this); //dodanie do kolejki
  else
 //if (Mechanik) //tylko je�li ma obsad�
  {if (Controller!=Humandriver) //i nie u�ytkownik (na razie)
    Mechanik->ScanEventTrack(); //dla pewno�ci robimy skanowanie
   if (Global::iMultiplayer) //potwierdzenie wykonania dla serwera - najcz�ciej odczyt semafora
    Global::pGround->WyslijEvent(e->asName,GetName());
  }
 }
 else
  if (!Mechanik->CheckEvent(e,true)) //czy dodawany do kolejki, funkcja prawie statyczna
   Global::pGround->AddToQuery(e,this); //dodanie do kolejki
};

int __fastcall TDynamicObject::DirectionSet(int d)
{//ustawienie kierunku w sk�adzie (wykonuje AI)
 iDirection=d>0?1:0; //d:1=zgodny,-1=przeciwny; iDirection:1=zgodny,0=przeciwny; 
 CouplCounter=26; //do przeskanowania s� kolizje
 return 1-(iDirection?NextConnectedNo:PrevConnectedNo); //informacja o po�o�eniu nast�pnego
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
{//ustalenie nast�pnego w sk�adzie bez wzgl�du na prawid�owo�� iDirection
 int d=1-(iDirection?NextConnectedNo:PrevConnectedNo);
 switch (d)
 {case  0:
   dir=(iDirection?NextConnectedNo:PrevConnectedNo)?1:-1;
   return (iDirection>0)?NextConnected:PrevConnected;
  case  1: d=iDirection?1:-1;
   return d>0?NextConnected:PrevConnected;
  case -1: d=iDirection?1:-1;
   return d>0?NextConnected:PrevConnected;
 }
 return NULL;
};

