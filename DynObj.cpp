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
#include "MemCell.h"
#include "Ground.h"

const float maxrot=(M_PI/3); //60°

//---------------------------------------------------------------------------
TDynamicObject* TDynamicObject::GetFirstDynamic(int cpl_type)
{//Szukanie skrajnego polaczonego pojazdu w pociagu
 //od strony sprzegu (cpl_type) obiektu szukajacego
 //Ra: wystarczy jedna funkcja do szukania w obu kierunkach
 TDynamicObject* temp=this;
 int coupler_nr=cpl_type;
 for (int i=0;i<300;i++) //ograniczenie do 300 na wypadek zapêtlenia sk³adu
 {
  if (!temp)
   return NULL; //Ra: zabezpieczenie przed ewentaulnymi b³êdami sprzêgów
  if (temp->MoverParameters->Couplers[coupler_nr].CouplingFlag==0)
   return temp;
  if (coupler_nr==0)
  {//je¿eli szukamy od sprzêgu 0
   if (temp->PrevConnectedNo==0) //jeœli pojazd od strony sprzêgu 0 jest odwrócony
    coupler_nr=1-coupler_nr; //to zmieniamy kierunek sprzêgu
   if (temp->PrevConnected)
    temp=temp->PrevConnected; //ten jest od strony 0
   else
    return temp; //jeœli jednak z przodu nic nie ma
  }
  else
  {
   if (temp->NextConnectedNo==1) //jeœli pojazd od strony sprzêgu 1 jest odwrócony
    coupler_nr=1-coupler_nr; //to zmieniamy kierunek sprzêgu
   if (temp->NextConnected)
    temp=temp->NextConnected; //ten pojazd jest od strony 1
   else
    return temp; //jeœli jednak z ty³u nic nie ma
  }
 }
 return NULL; //to tylko po wyczerpaniu pêtli
};

void TDynamicObject::ABuSetModelShake(vector3 mShake)
{
   modelShake=mShake;
}


int __fastcall TDynamicObject::GetPneumatic(bool front, bool red)
{
  int x, y, z;
  if (front)
    if (red)
      {
        x= btCPneumatic1.GetStatus();
        y= btCPneumatic1r.GetStatus();
      }
    else
      {
        x= btPneumatic1.GetStatus();
        y= btPneumatic1r.GetStatus();
      }
  else
    if (red)
      {
        x= btCPneumatic2.GetStatus();
        y= btCPneumatic2r.GetStatus();
      }
    else
      {
        x= btPneumatic2.GetStatus();
        y= btPneumatic2r.GetStatus();
      }
  z=0;

  if ((x==1)&&(y==1)) z=3;
  if ((x==2)&&(y==0)) z=1;
  if ((x==0)&&(y==2)) z=2;

  return z;
}

void __fastcall TDynamicObject::SetPneumatic(bool front, bool red)
{
  int x, ten, tamten;
  ten = GetPneumatic(front, red);
  if (front)
   if (PrevConnected) //pojazd od strony sprzêgu 0
    tamten=PrevConnected->GetPneumatic((PrevConnectedNo==0?true:false),red);
  if (!front)
   if (NextConnected) //pojazd od strony sprzêgu 1
    tamten=NextConnected->GetPneumatic((NextConnectedNo==0?true:false),red);
  x=0;

  if (ten==tamten)
    switch (ten)
     {
      case 1: x=2;
      break;
      case 2: x=3;
      break;
      case 3:{ int bleee=0;
              if (!front) bleee=1;
              if (MoverParameters->Couplers[bleee].Render)
                x=1;
              else
                x=4;
             }
      break;
     }
  else
   {
   if (ten==2)
   x=4;
   if (ten==1)
   x=1;
   if (ten==3)
    if (tamten==1)
      x=4;
    else
      x=1;  
   }

  if(front)
    if(red)
      cp1=x;
    else
      sp1=x;
  else
    if(red)
      cp2=x;
    else
      sp2=x;
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
    (smAnimatedDoor[i]->SetTranslate(vector3(0,0,1)*dDoorMoveL*DoorSpeedFactor[i]));
    //dDoorMoveL=dDoorMoveL*DoorSpeedFactor[i];
    }
    else
    {
    (smAnimatedDoor[i]->SetTranslate(vector3(0,0,1)*dDoorMoveR*DoorSpeedFactor[i]));
    //dDoorMoveR=dDoorMoveR*DoorSpeedFactor[i];
    }
   }
   else
   if (MoverParameters->DoorOpenMethod==2) //obrotowe albo dwojlomne (trzeba kombinowac submodelami i ShiftL=90,R=180)
   {
    if ((i%2)==0)
     smAnimatedDoor[i]->SetRotate(vector3(1,0,0),dDoorMoveL);
    else
     smAnimatedDoor[i]->SetRotate(vector3(1,0,0),dDoorMoveR);
   }
  }
 }
 btnOn=false;

  if (ObjSqrDist<160000) //gdy bli¿ej ni¿ 400m
  {
   if (ObjSqrDist<2500) //gdy bli¿ej ni¿ 50m
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
    //if (Global::bEnableTraction) //Ra: bardziej potrzebne wózki ni¿ druty
     ABuBogies();
    //McZapkie-050402: obracanie kolami
    for (int i=0; i<iAnimatedAxles; i++)
     if (smAnimatedWheel[i])
      smAnimatedWheel[i]->SetRotate(vector3(1,0,0),dWheelAngle);
    //Mczapkie-100402: rysowanie lub nie - sprzegow
    //ABu-240105: Dodatkowy warunek: if (...).Render, zeby rysowal tylko jeden
    //z polaczonych sprzegow
    if ((TestFlag(MoverParameters->Couplers[0].CouplingFlag,ctrain_coupler))
       &&(MoverParameters->Couplers[0].Render))
     {btCoupler1.TurnOn(); btnOn=true;}
    else
     btCoupler1.TurnOff();
    if ((TestFlag(MoverParameters->Couplers[1].CouplingFlag,ctrain_coupler))
       &&(MoverParameters->Couplers[1].Render))
     {btCoupler2.TurnOn(); btnOn=true;}
    else
     btCoupler2.TurnOff();
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
      switch(cp1)
        {
         case 1: btCPneumatic1.TurnOn(); break;
         case 2: btCPneumatic1.TurnxOn(); break;
         case 3: btCPneumatic1r.TurnxOn(); break;
         case 4: btCPneumatic1r.TurnOn(); break;
        }
      btnOn=true;
     }
    else
     {
      btCPneumatic1.TurnOff();
      btCPneumatic1r.TurnOff();
     }

    if (TestFlag(MoverParameters->Couplers[1].CouplingFlag,ctrain_pneumatic))
     {
      switch(cp2)
        {
         case 1: btCPneumatic2.TurnOn(); break;
         case 2: btCPneumatic2.TurnxOn(); break;
         case 3: btCPneumatic2r.TurnxOn(); break;
         case 4: btCPneumatic2r.TurnOn(); break;
        }
      btnOn=true;
     }
    else
     {
      btCPneumatic2.TurnOff();
      btCPneumatic2r.TurnOff();
     }

    //przewody zasilajace, j.w. (yB)
    if (TestFlag(MoverParameters->Couplers[0].CouplingFlag,ctrain_scndpneumatic))
     {
      switch(sp1)
        {
         case 1: btPneumatic1.TurnOn(); break;
         case 2: btPneumatic1.TurnxOn(); break;
         case 3: btPneumatic1r.TurnxOn(); break;
         case 4: btPneumatic1r.TurnOn(); break;
        }
      btnOn=true;
     }
    else
     {
      btPneumatic1.TurnOff();
      btPneumatic1r.TurnOff();
     }

    if (TestFlag(MoverParameters->Couplers[1].CouplingFlag,ctrain_scndpneumatic))
     {
      switch(sp2)
        {
         case 1: btPneumatic2.TurnOn(); break;
         case 2: btPneumatic2.TurnxOn(); break;
         case 3: btPneumatic2r.TurnxOn(); break;
         case 4: btPneumatic2r.TurnOn(); break;
        }
      btnOn=true;
     }
    else
     {
      btPneumatic2.TurnOff();
      btPneumatic2r.TurnOff();
     }
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
    else
    {
     btCPneumatic1.TurnOff();
     btCPneumatic1r.TurnOff();
    }

    if (TestFlag(MoverParameters->Couplers[1].CouplingFlag,ctrain_pneumatic))
    {
       if (MoverParameters->Couplers[1].Render)
         btCPneumatic2.TurnOn();
       else
         btCPneumatic2r.TurnOn();
       btnOn=true;
    }
    else
    {
     btCPneumatic2.TurnOff();
     btCPneumatic2r.TurnOff();
    }

    //przewody powietrzne j.w., ABu: decyzja czy rysowac tylko na podstawie 'render' //yB - zasilajace
    if (TestFlag(MoverParameters->Couplers[0].CouplingFlag,ctrain_scndpneumatic))
    {
       if (MoverParameters->Couplers[0].Render)
         btPneumatic1.TurnOn();
       else
         btPneumatic1r.TurnOn();
       btnOn=true;
    }
    else
    {
     btPneumatic1.TurnOff();
     btPneumatic1r.TurnOff();
    }

    if (TestFlag(MoverParameters->Couplers[1].CouplingFlag,ctrain_scndpneumatic))
    {
       if (MoverParameters->Couplers[1].Render)
         btPneumatic2.TurnOn();
       else
         btPneumatic2r.TurnOn();
       btnOn=true;
    }
    else
    {
     btPneumatic2.TurnOff();
     btPneumatic2r.TurnOff();
    }
   }
//*************************************************************/// koniec wezykow
     // uginanie zderzakow
     for (int i=0; i<2; i++)
     {
       double dist=MoverParameters->Couplers[i].Dist/2;
       if (smBuforLewy[i]!=NULL)
         if (dist<0)
           smBuforLewy[i]->SetTranslate(vector3(dist,0,0));
       if (smBuforPrawy[i]!=NULL)
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
       smPatykird1[0]->SetRotate(vector3(1,0,0),dPantAngleF);
    if (smPatykird2[0])
       smPatykird2[0]->SetRotate(vector3(-1,0,0),dPantAngleF);
    if (smPatykirg1[0])
       smPatykirg1[0]->SetRotate(vector3(-1,0,0),dPantAngleF*1.81);
    if (smPatykirg2[0])
       smPatykirg2[0]->SetRotate(vector3(1,0,0),dPantAngleF*1.81);
    if (smPatykisl[0])
       smPatykisl[0]->SetRotate(vector3(1,0,0),dPantAngleF*0.81);
    //Tylny patyk
    if (smPatykird1[1])
       smPatykird1[1]->SetRotate(vector3(1,0,0),dPantAngleR);
    if (smPatykird2[1])
       smPatykird2[1]->SetRotate(vector3(-1,0,0),dPantAngleR);
    if (smPatykirg1[1])
       smPatykirg1[1]->SetRotate(vector3(-1,0,0),dPantAngleR*1.81);
    if (smPatykirg2[1])
       smPatykirg2[1]->SetRotate(vector3(1,0,0),dPantAngleR*1.81);
    if (smPatykisl[1])
       smPatykisl[1]->SetRotate(vector3(1,0,0),dPantAngleR*0.81);
   }
    if (smWiazary[0])
       smWiazary[0]->SetRotate(vector3(1,0,0),-dWheelAngle);
    if (smWiazary[1])
       smWiazary[1]->SetRotate(vector3(1,0,0),-dWheelAngle);
//przewody sterowania ukrotnionego
    if (TestFlag(MoverParameters->Couplers[0].CouplingFlag,ctrain_controll))
     {btCCtrl1.TurnOn(); btnOn=true;}
    else
     btCCtrl1.TurnOff();
    if (TestFlag(MoverParameters->Couplers[1].CouplingFlag,ctrain_controll))
     {btCCtrl2.TurnOn(); btnOn=true;}
    else
     btCCtrl2.TurnOff();
//McZapkie-181103: mostki przejsciowe
    if (TestFlag(MoverParameters->Couplers[0].CouplingFlag,ctrain_passenger))
     {btCPass1.TurnOn(); btnOn=true;}
    else
     btCPass1.TurnOff();
    if (TestFlag(MoverParameters->Couplers[1].CouplingFlag,ctrain_passenger))
     {btCPass2.TurnOn(); btnOn=true;}
    else
     btCPass2.TurnOff();

// sygnaly konca pociagu
    if (btEndSignals1.Active())
    {
       if (TestFlag(MoverParameters->HeadSignalsFlag,2)
         ||TestFlag(MoverParameters->HeadSignalsFlag,32))
        {btEndSignals1.TurnOn(); btnOn=true;}
       else
        btEndSignals1.TurnOff();
    }
    else
    {
       if (TestFlag(MoverParameters->HeadSignalsFlag,2))
        {btEndSignals11.TurnOn(); btnOn=true;}
       else
         btEndSignals11.TurnOff();
       if (TestFlag(MoverParameters->HeadSignalsFlag,32))
        {btEndSignals13.TurnOn(); btnOn=true;}
       else
        btEndSignals13.TurnOff();
    }

    if (btEndSignals2.Active())
    {
       if (TestFlag(MoverParameters->EndSignalsFlag,2)
         ||TestFlag(MoverParameters->EndSignalsFlag,32))
        {btEndSignals2.TurnOn(); btnOn=true;}
       else
        btEndSignals2.TurnOff();
    }
    else
    {
       if (TestFlag(MoverParameters->EndSignalsFlag,2))
        {btEndSignals21.TurnOn(); btnOn=true;}
       else
        btEndSignals21.TurnOff();
       if (TestFlag(MoverParameters->EndSignalsFlag,32))
        {btEndSignals23.TurnOn(); btnOn=true;}
       else
        btEndSignals23.TurnOff();
    }

    //tablice blaszane:
    if (TestFlag(MoverParameters->HeadSignalsFlag,64))
     {btEndSignalsTab1.TurnOn(); btnOn=true;}
    else
     btEndSignalsTab1.TurnOff();
    if (TestFlag(MoverParameters->EndSignalsFlag,64))
     {btEndSignalsTab2.TurnOn(); btnOn=true;}
    else
     btEndSignalsTab2.TurnOff();

// sygnaly czola pociagu
    if (TestFlag(MoverParameters->HeadSignalsFlag,1))
     {btHeadSignals11.TurnOn(); btnOn=true;}
    else
     btHeadSignals11.TurnOff();
    if (TestFlag(MoverParameters->HeadSignalsFlag,4))
     {btHeadSignals12.TurnOn(); btnOn=true;}
    else
     btHeadSignals12.TurnOff();
    if (TestFlag(MoverParameters->HeadSignalsFlag,16))
     {btHeadSignals13.TurnOn(); btnOn=true;}
    else
     btHeadSignals13.TurnOff();

    if (TestFlag(MoverParameters->EndSignalsFlag,1))
     {btHeadSignals21.TurnOn(); btnOn=true;}
    else
     btHeadSignals21.TurnOff();
    if (TestFlag(MoverParameters->EndSignalsFlag,4))
     {btHeadSignals22.TurnOn(); btnOn=true;}
    else
     btHeadSignals22.TurnOff();
    if (TestFlag(MoverParameters->EndSignalsFlag,16))
     {btHeadSignals23.TurnOn(); btnOn=true;}
    else
     btHeadSignals23.TurnOff();

//McZapkie-181002: krecenie wahaczem (korzysta z kata obrotu silnika)
    for (int i=0; i<4; i++)
     if (smWahacze[i])
      smWahacze[i]->SetRotate(vector3(1,0,0),fWahaczeAmp*cos(MoverParameters->eAngle));

    if (smMechanik!=NULL)
     {
      if ((Mechanik!=NULL) && (Controller!=Humandriver))  //rysowanie figurki mechanika
       smMechanik->Visible= true;
      else
       smMechanik->Visible= false;
     }
    //ABu: Przechyly na zakretach
    if(ObjSqrDist<80000) ABuModelRoll();
 }
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


TDynamicObject* __fastcall ABuFindNearestObject(TTrack *Track,TDynamicObject *MyPointer,int &CouplNr)
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
    tmp=Global::GetCameraPosition()-Track->Dynamics[i]->GetPosition();
    dist=tmp.x*tmp.x+tmp.y*tmp.y+tmp.z*tmp.z; //odleg³oœæ do kwadratu
    if (dist<100.0) //10 metrów
     return Track->Dynamics[i];
   }
   else //jeœli (CouplNr) inne niz -2, szukamy sprzêgu
   {//wektor [kamera-sprzeg0], potem [kamera-sprzeg1]
    //Powinno byc wyliczone, ale nie zaszkodzi drugi raz:
    //(bo co, jesli nie wykonuje sie obrotow wozkow?)
    Track->Dynamics[i]->modelRot.z=ABuAcos(Track->Dynamics[i]->Axle4.pPosition-Track->Dynamics[i]->Axle1.pPosition);
    poz=Track->Dynamics[i]->GetPosition(); //pozycja œrodka pojazdu
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



TDynamicObject* TDynamicObject::ABuScanNearestObject(TTrack *Track,double ScanDir,double ScanDist,int &CouplNr)
{//skanowanie toru w poszukiwaniu obiektu najblizszego kamerze
 //double MyScanDir=ScanDir;  //Moja orientacja na torze.  //Ra: nie u¿ywane
 if (ABuGetDirection()<0) ScanDir=-ScanDir;
 TDynamicObject* FoundedObj;
 FoundedObj=ABuFindNearestObject(Track,this,CouplNr); //zwraca numer sprzêgu znalezionego pojazdu
 if (FoundedObj==NULL)
 {
  double ActDist;    //Przeskanowana odleglosc.
  double CurrDist=0; //Aktualna dlugosc toru.
  if (ScanDir>=0) ActDist=Track->Length()-ABuGetTranslation(); //???-przesuniêcie wózka wzglêdem Point1 toru
             else ActDist=ABuGetTranslation(); //przesuniêcie wózka wzglêdem Point1 toru
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
 double modelRoll=RadToDeg(0.5*(Axle1.GetRoll()+Axle4.GetRoll())); //Ra: tu nie by³o DegToRad
 //if (ABuGetDirection()<0) modelRoll=-modelRoll;
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

//ABu 06.05.04 poczatek wyliczania obrotow wozkow **********************

void __fastcall TDynamicObject::ABuBogies()
{//Obracanie wozkow na zakretach. Na razie uwzglêdnia tylko zakrêty,
 //bez zadnych gorek i innych przeszkod.
 if((smBogie[0]!=NULL)&&(smBogie[1]!=NULL))
 {
  modelRot.z=ABuAcos(Axle4.pPosition-Axle1.pPosition);
  //bogieRot[0].z=ABuAcos(Axle4.pPosition-Axle3.pPosition);
  bogieRot[0].z=Axle4.vAngles.z;
  //bogieRot[1].z=ABuAcos(Axle2.pPosition-Axle1.pPosition);
  bogieRot[1].z=Axle1.vAngles.z;
  bogieRot[0]=RadToDeg(modelRot-bogieRot[0]); //mno¿enie wektora przez sta³¹
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
 TTrack* NewTrack=Axle4.GetTrack();
 if ((NewTrack!=OldTrack)&&OldTrack)
 {
  OldTrack->RemoveDynamicObject(this);
  NewTrack->AddDynamicObject(this);
 }
}

//Ra: w poni¿szej funkcji jest problem ze sprzêgami
TDynamicObject* __fastcall ABuFindObject(TTrack *Track,TDynamicObject *MyPointer,int ScanDir,Byte &CouplFound)
{//Zwraca wskaŸnik najbli¿szego obiektu znajduj¹cego siê
 //na torze w okreœlonym kierunku, ale tylko wtedy, kiedy
 //obiekty mog¹ siê zderzyæ, tzn. nie mijaj¹ siê.

 //WE: Track      - tor, na ktorym odbywa sie poszukiwanie,
 //    MyPointer  - wskaznik do obiektu szukajacego.
 //    ScanDir    - kierunek szukania na torze (bêdzie inne, jeœli tor jest odwrotnie)
 //    MyScanDir  - kierunek szukania obiektu szukajacego (na jego torze); Ra: nie potrzebne
 //    MyCouplFound - nr sprzegu obiektu szukajacego; Ra: nie potrzebne

 //WY: wskaznik do znalezionego obiektu.
 //    CouplFound - nr sprzegu znalezionego obiektu
 if ((Track->iNumDynamics)>0)
 {//sens szukania na tym torze jest tylko, gdy s¹ na nim pojazdy
  double ObjTranslation; //pozycja najblizszego obiektu na torze
  double MyTranslation; //pozycja szukaj¹cego na torze
  double MinDist=Track->Length(); //najmniejsza znaleziona odlegloœæ (zaczynamy od d³ugoœci toru)
  double TestDist;
  int iMinDist=-1;  //indeks wykrytego obiektu
  if (MyPointer->MyTrack==Track) //gdy szukanie na tym samym torze
   MyTranslation=MyPointer->ABuGetTranslation(); //po³o¿enie wózka wzglêdem Point1 toru
  else //gdy szukanie na innym torze
   if (ScanDir>0)
    MyTranslation=0; //szukanie w kierunku Point2 (od zera) - jesteœmy w Point1
   else
    MyTranslation=MinDist; //szukanie w kierunku Point1 (do zera) - jesteœmy w Point2
  if (ScanDir>=0)
  {//jeœli szukanie w kierunku Point2
   for (int i=0;i<Track->iNumDynamics;i++)
   {//pêtla po pojazdach
    if (MyPointer!=Track->Dynamics[i]) //szukaj¹cy siê nie liczy
    {
     TestDist=(Track->Dynamics[i]->ABuGetTranslation())-MyTranslation; //odleg³og³oœæ tamtego od szukaj¹cego
     if ((TestDist>0)&&(TestDist<=MinDist))
     {//gdy jest po w³aœciwej stronie i bli¿ej ni¿ jakiœ wczeœniejszy
      CouplFound=(Track->Dynamics[i]->ABuGetDirection()>0)?1:0; //to, bo (ScanDir>=0)
      if (Track->iCategoryFlag&254) //trajektoria innego typu ni¿ tor kolejowy
      {//dla torów nie ma sensu tego sprawdzaæ, rzadko co jedzie po jednej szynie i siê mija
       //Ra: mijanie samochodów wcale nie jest proste
       // Przesuniecie wzgledne pojazdow. Wyznaczane, zeby sprawdzic,
       // czy pojazdy faktycznie sie zderzaja (moga byc przesuniete
       // w/m siebie tak, ze nie zachodza na siebie i wtedy sie mijaja).
       double RelOffsetH; //wzajemna odleg³oœæ poprzeczna
       if (CouplFound) //my na tym torze byœmy byli w kierunku Point2
        //dla CouplFound=1 s¹ zwroty zgodne - istotna ró¿nica przesuniêæ
        RelOffsetH=(MyPointer->MoverParameters->OffsetTrackH-Track->Dynamics[i]->MoverParameters->OffsetTrackH);
       else
        //dla CouplFound=0 s¹ zwroty przeciwne - przesuniêcia sumuj¹ siê
        RelOffsetH=(MyPointer->MoverParameters->OffsetTrackH+Track->Dynamics[i]->MoverParameters->OffsetTrackH);
       if (RelOffsetH<0) RelOffsetH=-RelOffsetH;
       if (RelOffsetH+RelOffsetH>MyPointer->MoverParameters->Dim.W)+(Track->Dynamics[i]->MoverParameters->Dim.W);
        continue; //odleg³oœæ wiêksza od po³owy sumy szerokoœci - kolizji nie bêdzie
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
     TestDist=MyTranslation-(Track->Dynamics[i]->ABuGetTranslation()); //???-przesuniêcie wózka wzglêdem Point1 toru
     if ((TestDist>0)&&(TestDist<MinDist))
     {
      CouplFound=(Track->Dynamics[i]->ABuGetDirection()>0)?0:1; //odwrotnie, bo (ScanDir<0)
      if (Track->iCategoryFlag&254) //trajektoria innego typu ni¿ tor kolejowy
      {//dla torów nie ma sensu tego sprawdzaæ, rzadko co jedzie po jednej szynie i siê mija
       //Ra: mijanie samochodów wcale nie jest proste
       // Przesuniecie wzgledne pojazdow. Wyznaczane, zeby sprawdzic,
       // czy pojazdy faktycznie sie zderzaja (moga byc przesuniete
       // w/m siebie tak, ze nie zachodza na siebie i wtedy sie mijaja).
       double RelOffsetH; //wzajemna odleg³oœæ poprzeczna
       if (CouplFound) //my na tym torze byœmy byli w kierunku Point1
        //dla CouplFound=1 s¹ zwroty zgodne - istotna ró¿nica przesuniêæ
        RelOffsetH=(MyPointer->MoverParameters->OffsetTrackH-Track->Dynamics[i]->MoverParameters->OffsetTrackH);
       else
        //dla CouplFound=0 s¹ zwroty przeciwne - przesuniêcia sumuj¹ siê
        RelOffsetH=(MyPointer->MoverParameters->OffsetTrackH+Track->Dynamics[i]->MoverParameters->OffsetTrackH);
       if (RelOffsetH<0) RelOffsetH=-RelOffsetH;
       if (RelOffsetH+RelOffsetH>MyPointer->MoverParameters->Dim.W)+(Track->Dynamics[i]->MoverParameters->Dim.W);
        continue; //odleg³oœæ wiêksza od po³owy sumy szerokoœci - kolizji nie bêdzie
      }
      iMinDist=i; //potencjalna kolizja
      MinDist=TestDist;
     }
    }
   }
  }
  return (iMinDist>=0)?Track->Dynamics[iMinDist]:NULL;
 }
 return NULL; //nie ma pojazdów na torze, to jest NULL
}

void TDynamicObject::CouplersDettach(double MinDist,int MyScanDir)
{//funkcja roz³¹czajaca pod³¹czone sprzêgi
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
{//skanowanie toru w poszukiwaniu obiektow
 //ScanDir - okreœla kierunek poszukiwania zale¿nie od zwrotu prêdkoœci pojazdu
 // ScanDir=1 - od strony Coupler0, ScanDir=-1 - od strony Coupler1
 int MyScanDir=ScanDir;  //zapamiêtanie kierunku poszukiwañ na torze wyjœciowym
 TTrackFollower *FirstAxle=(MyScanDir>0?&Axle4:&Axle1); //mo¿na by to trzymaæ w trainset
 TTrack *Track=FirstAxle->GetTrack(); //tor na którym "stoi" skrajny wózek (mo¿e byæ inny ni¿ pojazdu)
 if (FirstAxle->GetDirection()<0) //czy oœ jest ustawiona w stronê Point1?
  ScanDir=-ScanDir; //jeœli tak, to kierunek szukania bêdzie przeciwny (teraz wzglêdem toru)
 Byte MyCouplFound; //numer sprzêgu do pod³¹czenia w obiekcie szukajacym
 MyCouplFound=(MyScanDir<0)?1:0;
 Byte CouplFound; //numer sprzêgu w znalezionym obiekcie (znaleziony wype³ni)
 TDynamicObject *FoundedObj; //znaleziony obiekt
 FoundedObj=ABuFindObject(Track,this,ScanDir,CouplFound); //zaczynamy szukaæ na tym samym torze

/*
 if (FoundedObj) //jak coœ znajdzie, to œledzimy
 {//powtórzenie wyszukiwania tylko do zastawiania pu³epek podczas testów
  if (ABuGetDirection()<0) ScanDir=ScanDir; //ustalenie kierunku wzglêdem toru
  FoundedObj=ABuFindObject(Track,this,ScanDir,CouplFound);
 }
*/

 if (FoundedObj) //kod s³u¿¹cy do logowania b³êdów
  if (CouplFound==0)
  {
   if (FoundedObj->PrevConnected)
    if (FoundedObj->PrevConnected!=this) //odœwie¿enie tego samego siê nie liczy
     WriteLog("0! Coupler error on "+asName+":"+AnsiString(MyCouplFound)+" - "+FoundedObj->asName+":0 is already connected");
  }
  else
  {
   if (FoundedObj->NextConnected)
    if (FoundedObj->NextConnected!=this) //odœwie¿enie tego samego siê nie liczy
     WriteLog("0! Coupler error on "+asName+":"+AnsiString(MyCouplFound)+" - "+FoundedObj->asName+":1 is already connected");
  }


 if (FoundedObj==NULL) //jeœli nie ma na tym samym, szukamy po okolicy
 {//szukanie najblizszego toru z jakims obiektem
  //praktycznie przeklejone z TraceRoute()...
  double ActDist;    //przeskanowana odlegloœæ
  double CurrDist=0; //aktualna dlugosc toru
  if (ScanDir>=0) //uwzglêdniamy kawalek przeanalizowanego wy¿ej toru
   ActDist=Track->Length()-FirstAxle->GetTranslation(); //odleg³oœæ osi od Point2 toru
  else
   ActDist=FirstAxle->GetTranslation(); //odleg³oœæ osi od Point1 toru
  while (ActDist<ScanDist)
  {
   ActDist+=CurrDist;
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
    CurrDist=Track->Length(); //doliczenie tego toru do przejrzanego dystandu
    FoundedObj=ABuFindObject(Track,this,ScanDir,CouplFound); //przejrzenie pojazdów tego toru
    if (FoundedObj)
    {
     //if((Mechanik)&&(!EndTrack))
     //{
     //   EndTrack=true;
     //   //Mechanik->SetProximityVelocity(0,20);
     //   Mechanik->SetVelocity(0,0);
     //}
     ActDist=ScanDist; //wyjœcie z pêtli poszukiwania
    }
   }
   else //Jesli nie ma, to wychodzimy.
   {
    if ((Mechanik)&&(!EndTrack))
    {
     EndTrack=true;
     Mechanik->SetProximityVelocity(ActDist-50,0);
     //Mechanik->SetVelocity(0,0);
    }
    ActDist=ScanDist;
   }
  }
 } // Koniec szukania najbli¿szego toru z jakimœ obiektem.
 //teraz odczepianie i jeœli coœ siê znalaz³o, doczepianie.
 if (MyScanDir>0?PrevConnected:NextConnected)
  if ((MyScanDir>0?PrevConnected:NextConnected)!=FoundedObj)
   CouplersDettach(1,MyScanDir);
 // i ³¹czenie sprzêgiem wirtualnym
 if (FoundedObj)
 {//siebie mo¿na bezpiecznie pod³¹czyæ jednostronnie do znalezionego
  MoverParameters->Attach(MyCouplFound,CouplFound,&(FoundedObj->MoverParameters),ctrain_virtual);
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
   FoundedObj->MoverParameters->Attach(CouplFound,MyCouplFound,&(this->MoverParameters),ctrain_virtual);
   //FoundedObj->MoverParameters->Couplers[CouplFound].Render=false; //tamtemu nie ma co zmieniaæ stanu sprzêgów
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
  if ((Mechanik)&&(!EndTrack))
  {
   EndTrack=true;
   if ((MoverParameters->Couplers[MyCouplFound].CoupleDist)<50)
    Mechanik->SetVelocity(0,0);
   else
    //Mechanik->SetVelocity(20,20);
    Mechanik->SetProximityVelocity((MoverParameters->Couplers[MyCouplFound].CoupleDist)-20,0);
  }
 }
}
//----------ABu: koniec skanowania pojazdow

//----------------McZapkie: skanowanie semaforow:

//pomocnicza funkcja sprawdzania czy do toru jest podpiety semafor
bool __fastcall CheckTrackEvent(double fDirection, TTrack *Track)
{
  if (fDirection>0)
   if (Track->Event2!=NULL)
    if ((Track->Event2->Type==tp_GetValues) || (Track->Event2->Type==tp_PutValues))
     return true;
  if (fDirection<0)
   if (Track->Event1!=NULL)
    if ((Track->Event1->Type==tp_GetValues) || (Track->Event1->Type==tp_PutValues))
     return true;
  return false;
}

TTrack* __fastcall TraceRoute(double &fDistance, double &fDirection, TTrack *Track)
{
//    fDistance*= fDirection;
    double fCurrentDistance= Track->Length();
    double s= 0;
    if (CheckTrackEvent(fDirection,Track))
     {
      fDistance= 0;   //to na tym torze stoimy
      return Track;
     }
    while (s<fDistance)
     {
        Track->ScannedFlag= false;
        s+= fCurrentDistance;
        if (fDirection>0)
        {
         if (Track->iNextDirection)
         {
          Track=Track->CurrentNext();
          fDirection=-fDirection;
         }
         else
          Track=Track->CurrentNext();
        }
        else if (fDirection<0)
        {
         if (Track->iPrevDirection)
          Track=Track->CurrentPrev();
         else
         {
          Track=Track->CurrentPrev();
          fDirection=-fDirection;
         }
        }
        else return NULL; //kierunek moze byc tylko +-

        if (Track==NULL)
        {
            fDistance= s;
            return NULL;
        }
        fCurrentDistance= Track->Length();
        if (CheckTrackEvent(fDirection,Track))
         {
         //ZiomalCl: teraz zwracany jest pierwszy event podajacy predkosc dla AI
         //a nie kazdy najblizszy event [AI sie gubilo gdy przed getval z SetVelocity
         //mialo np. PutValues z eventem od SHP]
           if(Track->Event1!=NULL)
            if(Track->Event1->Type==tp_GetValues)
              {
              AnsiString st1 = String(Track->Event1->Params[9].asMemCell->szText);
              if(st1=="SetVelocity"||st1=="ShuntVelocity")
                {
                fDistance= s;
                return Track;
                }
              }
            else if(Track->Event1->Type==tp_PutValues)
              {
              AnsiString st1 = String(Track->Event1->Params[0].asText);
              if (st1=="SetVelocity"||st1=="ShuntVelocity"||(st1.SubString(1,19)=="PassengerStopPoint:"))
                {
                fDistance= s;
                return Track;
                }
              }
           if(Track->Event2!=NULL)
            if(Track->Event2->Type==tp_GetValues)
              {
              AnsiString st1 = String(Track->Event2->Params[9].asMemCell->szText);
              if(st1=="SetVelocity"||st1=="ShuntVelocity")
                {
                fDistance= s;
                return Track;
                }
              }
            else if(Track->Event2->Type==tp_PutValues)
              {
              AnsiString st1 = String(Track->Event2->Params[0].asText);
              if (st1=="SetVelocity"||st1=="ShuntVelocity"||(st1.SubString(1,19)=="PassengerStopPoint:"))
                {
                fDistance= s;
                return Track;
                }
              }


         }
     }
    fDistance= s;
    return Track;
}


//sprawdzanie zdarzeñ semaforów i ograniczeñ szlakowych
void TDynamicObject::ScanEventTrack(TTrack *Track)
{
 double scandir=MoverParameters->CabNo*Axle1.GetDirection(); //kabina i zwrot na torze
 TLocation sl;
 if (MoverParameters->ActiveDir!=0)
  scandir=scandir*MoverParameters->ActiveDir;
 if (scandir!=0) //skanowanie toru w poszukiwaniu eventu GetValues
 {
  double scandist=120+random(MoverParameters->Vmax*5); //fabs(Mechanik->ProximityDist);
  TTrack *scantrack=TraceRoute(scandist,scandir,Track);
  if (scantrack==NULL) //jeœli wykryto koniec toru
  {
   if (!EndTrack)
    if (!Mechanik->SetProximityVelocity(0.5*fabs(scandist),0))
     Mechanik->SetVelocity(Mechanik->VelActual*0.9,0);
  }
  else
  {
   double vtrackmax=scantrack->fVelocity;  //ograniczenie szlakowe
   double vmechmax=-1;
   if ((scantrack->Event2!=NULL) && (scandir>0))
   {
    if (scantrack->Event2->Type==tp_GetValues)
    {
     if (strcmp(scantrack->Event2->Params[9].asMemCell->szText,"SetVelocity")==0)
     {//przeslac info o zblizajacym sie semaforze
      sl.X=-scantrack->Event2->Params[8].asGroundNode->pCenter.x;
      sl.Y= scantrack->Event2->Params[8].asGroundNode->pCenter.z;
      sl.Z= scantrack->Event2->Params[8].asGroundNode->pCenter.y;
      vmechmax=scantrack->Event2->Params[9].asMemCell->fValue1;
      if (fabs(scandist)>Mechanik->MinProximityDist) //semafor daleko
      {
       if (!EndTrack)
        Mechanik->PutCommand("SetProximityVelocity",fabs(scandist),vmechmax,sl);
      }
      else //semafor na tym torze
      {
       if (!EndTrack)
        Mechanik->PutCommand("SetVelocity",vmechmax,scantrack->Event2->Params[9].asMemCell->fValue2,sl);
      }
     }
    }
    else if (scantrack->Event2->Type==tp_PutValues)
     if (strcmp(scantrack->Event2->Params[0].asText,"SetVelocity")==0)
     {//statyczne ograniczenie predkosci
      sl.X=-scantrack->Event2->Params[3].asdouble;
      sl.Y= scantrack->Event2->Params[4].asdouble;
      sl.Z= scantrack->Event2->Params[5].asdouble;
      if (fabs(scandist)<Mechanik->MinProximityDist+1) //na tym torze
       if (!EndTrack)
        Mechanik->PutCommand("SetVelocity",scantrack->Event2->Params[1].asdouble,scantrack->Event2->Params[2].asdouble,sl);
     }
   }
   else if ((scantrack->Event1!=NULL) && (scandir<0))
   {
    if (scantrack->Event1->Type==tp_GetValues)
    {
     if (strcmp(scantrack->Event1->Params[9].asMemCell->szText,"SetVelocity")==0)
     {//to samo ale w druga stronê
      sl.X=-scantrack->Event1->Params[8].asGroundNode->pCenter.x;
      sl.Y= scantrack->Event1->Params[8].asGroundNode->pCenter.z;
      sl.Z= scantrack->Event1->Params[8].asGroundNode->pCenter.y;
      vmechmax=scantrack->Event1->Params[9].asMemCell->fValue1;
      if (fabs(scandist)>Mechanik->MinProximityDist) //semafor daleko
      {
       if (!EndTrack)
        Mechanik->PutCommand("SetProximityVelocity",fabs(scandist),vmechmax,sl);
      }
      else //semafor na tym torze
      {
       if (!EndTrack)
        Mechanik->PutCommand("SetVelocity",vmechmax,scantrack->Event1->Params[9].asMemCell->fValue2,sl);
      }
     }
    }
    else if (scantrack->Event1->Type==tp_PutValues)
     if (strcmp(scantrack->Event1->Params[0].asText,"SetVelocity")==0)
     {                         //statyczne ograniczenie predkosci
      sl.X=-scantrack->Event1->Params[3].asdouble;
      sl.Y= scantrack->Event1->Params[4].asdouble;
      sl.Z= scantrack->Event1->Params[5].asdouble;
      if (fabs(scandist)<Mechanik->MinProximityDist+1) //na tym torze
       if (!EndTrack)
        Mechanik->PutCommand("SetVelocity",scantrack->Event1->Params[1].asdouble,scantrack->Event1->Params[2].asdouble,sl);
     }
   }
   if (vtrackmax>0)
    if ((vmechmax<0) || (vtrackmax<vmechmax))
    {
     sl.X=-scantrack->CurrentSegment()->FastGetPoint(0.5).x;
     sl.Y= scantrack->CurrentSegment()->FastGetPoint(0.5).z;
     sl.Z= scantrack->CurrentSegment()->FastGetPoint(0.5).y;
     if (!EndTrack)
      Mechanik->PutCommand("SetProximityVelocity",fabs(scandist),vtrackmax,sl);
    }
  }  // !null
 }
};

//-----------koniec skanowania semaforow

__fastcall TDynamicObject::TDynamicObject()
{
 modelShake=vector3(0,0,0);
 EndTrack=false;
 btnOn=false;
 vUp=vWorldUp;
 vFront=vWorldFront;
 vLeft=vWorldLeft;
 iNumAxles=0;
 MoverParameters=NULL;
 Mechanik=NULL;
 MechInside=false;
 TrainParams=NULL;
 //McZapkie-270202
 Controller=AIdriver;
 bDisplayCab=false; //030303
 NextConnected=PrevConnected= NULL;
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
  dWheelsPosition[i]=0.0;
 iAxles=0;
 dWheelAngle=0.0;
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
 iAnimatedAxles=0;
 iAnimatedDoors=0;
 for (int i=0;i<MaxAnimatedAxles;i++)
  smAnimatedWheel[i]=NULL;
 for (int i=0;i<MaxAnimatedDoors;i++)
  smAnimatedDoor[i]=NULL;
 mdModel=NULL;
 mdKabina=NULL;
 ReplacableSkinID=0;
 bAlpha=false;
 smWiazary[0]=smWiazary[1]=NULL;
 smWahacze[0]=smWahacze[1]=smWahacze[2]=smWahacze[3]=NULL;
 fWahaczeAmp=0;
 iAnimatedAxles=0;
 iAnimatedDoors=0;
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
}

__fastcall TDynamicObject::~TDynamicObject()
{//McZapkie-250302 - zamykanie logowania parametrow fizycznych
 if (Mechanik)
  Mechanik->CloseLog();
 SafeDelete(Mechanik);
 SafeDelete(MoverParameters);
 SafeDelete(TrainParams);
}

bool __fastcall TDynamicObject::Init(
 AnsiString Name,AnsiString BaseDir,AnsiString asReplacableSkin,AnsiString Type_Name,
 TTrack *Track,double fDist,AnsiString DriverType,double fVel,AnsiString TrainName,
 int Load, AnsiString LoadType,bool Reversed)
{//McZapkie: Name to np. EU07-424, BaseDir to np. PKP/EU07, Type_Name to np. 303E, TrainName to np. PE2307

 iDirection=(Reversed?-1:1); //Ra: ujemne, jeœli ma byæ wstawiony do sk³adu jako obrócony
 //McZapkie-310302
 asBaseDir= "dynamic\\"+BaseDir+"\\";
 //Model.Init(asModel);

    if (Name!=AnsiString(""))
        asName= Name;

    AnsiString asAnimName="";

    TLocation l;
    l.X=l.Y=l.Z= 0;
    TRotation r;
    r.Rx=r.Ry=r.Rz= 0;
    int Cab;
    if (DriverType==AnsiString("headdriver"))
     Cab=1;
    else
    if (DriverType==AnsiString("reardriver"))
     Cab=-1;
    else
    if (DriverType==AnsiString("connected"))
     Cab=1;
    else
    if (DriverType==AnsiString("passenger"))
     {
      if (random(6)<3) Cab=1;
       else Cab=-1;
     }
    else
     if (DriverType==AnsiString("nobody"))
     Cab=0;
    else
     {
      Cab=0;  //McZapkie-010303: w przyszlosci dac tez pomocnika, palacza, konduktora itp.
      Error("Unknown DriverType description: "+DriverType);
      DriverType="nobody";
     }

    MoverParameters= new TMoverParameters(l,r,fVel,Type_Name,asName,Load,LoadType,Cab);
    //McZapkie: TypeNamenit musi byc nazwa typu pojazdu
    if (!MoverParameters->LoadChkFile(asBaseDir))
    {
        Error("Cannot load dynamic object "+asName+" from\n"+BaseDir+"\\"+Type_Name+" Error="+ConversionError+"@"+LineCount);
        return false;
    }
    bool driveractive=  (fVel!=0);
    if (!MoverParameters->CheckLocomotiveParameters(driveractive)) //jak jedzie lub obsadzony to gotowy do drogi
     {
        Error("Parameters mismatch: dynamic object "+asName+" from\n"+BaseDir+"\\"+Type_Name);
        return false;
     }
    if (MoverParameters->CategoryFlag==2)       //ustawianie samochodow na poboczu
     if (driveractive)
      MoverParameters->OffsetTrackH=-Track->fTrackWidth*0.2;
     else
      MoverParameters->OffsetTrackH=-Track->fTrackWidth*0.3;

//w wagonie tez niech jedzie
//    if (MoverParameters->MainCtrlPosNo>0 &&
//     if (MoverParameters->CabNo!=0)
     if (DriverType!=AnsiString("nobody"))
     {
//McZapkie-040602
       if (Name==AnsiString(Global::asHumanCtrlVehicle))
       {
        if (MoverParameters->EngineType!=Dumb)
         Controller=Humandriver;
       }

//McZapkie-151102: rozklad jazdy czytany z pliku *.tt z katalogu w ktorym jest sceneria
       if ((DriverType==AnsiString("headdriver")) || (DriverType==AnsiString("reardriver")))
//McZapkie-110303: mechanik&rozklad tylko gdy jest obsada
       {
         MoverParameters->ActiveCab=MoverParameters->CabNo;
         TrainParams= new TTrainParameters(TrainName);
         if (TrainName!=AnsiString("none"))
           if (!TrainParams->LoadTTfile(Global::asCurrentSceneryPath))
             Error("Cannot load timetable file "+TrainName+": Error="+ConversionError+"@"+TrainParams->StationCount);

         Mechanik= new TController(l,r,Controller,&MoverParameters,&TrainParams,Aggressive);

         if (Controller==AIdriver)
          {
            Mechanik->Ready=false;
            Mechanik->ChangeOrder(Prepare_engine);
            Mechanik->JumpToNextOrder();
            if (TrainName==AnsiString("none"))
             Mechanik->ChangeOrder(Shunt);
            else
             Mechanik->ChangeOrder(Obey_train);
            Mechanik->JumpToNextOrder();
            Mechanik->ChangeOrder(Shunt);
            Mechanik->JumpToNextOrder();
            Mechanik->ChangeOrder(Shunt);
            Mechanik->JumpToNextOrder();
    //McZapkie-100302 - to ma byc wyzwalane ze scenerii
    //  Mechanik->JumpToFirstOrder();
            if (fVel==0)
             {
               Mechanik->SetVelocity(0,0);
             }
            else
             {
              Mechanik->SetVelocity(fVel,-1);
              Mechanik->JumpToFirstOrder();
             }
          }
  // McZapkie! - zeby w ogole AI ruszyl to musi wykonac powyzsze rozkazy
  // ALe mozna by je zapodac ze scenerii
       }
      else
       if (DriverType==AnsiString("passenger"))  //obserwator w charakterze pasazera
        {
         TrainParams= new TTrainParameters(TrainName);
         Mechanik= new TController(l,r,Controller,&MoverParameters,&TrainParams,Easyman);
        }
     }


// McZapkie-250202
    iAxles= (MaxAxles<MoverParameters->NAxles)?MaxAxles:MoverParameters->NAxles;

//wczytywanie z pliku nazwatypu.mmd

    LoadMMediaFile(asBaseDir,Type_Name, asReplacableSkin);


//  asModel wczytywany w LoadMMediaFile


//McZapkie-100402: wyszukiwanie sprzegow
    btCoupler1.Init("coupler1",mdModel,false);
    btCoupler2.Init("coupler2",mdModel,false);
/********************************************************
    btCPneumatic1.Init("cpneumatic1",mdModel,false);
    btCPneumatic2.Init("cpneumatic2",mdModel,false);
    btCPneumatic1r.Init("cpneumatic1r",mdModel,false);
    btCPneumatic2r.Init("cpneumatic2r",mdModel,false);
    btPneumatic1.Init("pneumatic1",mdModel,false);
    btPneumatic2.Init("pneumatic2",mdModel,false);
    btPneumatic1r.Init("pneumatic1r",mdModel,false);
    btPneumatic2r.Init("pneumatic2r",mdModel,false);
********************************************************/
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
    btEndSignals11.Init("endsignal23",mdModel,false);
    btEndSignals13.Init("endsignal22",mdModel,false);
    btEndSignals21.Init("endsignal13",mdModel,false);
    btEndSignals23.Init("endsignal12",mdModel,false);
    //ABu: to niestety zostawione dla kompatybilnosci modeli:
    btEndSignals2.Init("endsignals1",mdModel,false);
    btEndSignals1.Init("endsignals2",mdModel,false);

    btEndSignalsTab1.Init("endtab1",mdModel,false);
    btEndSignalsTab2.Init("endtab2",mdModel,false);
    //ABu Uwaga! tu zmienic w modelu!
    btHeadSignals11.Init("headlamp23",mdModel,false);
    btHeadSignals12.Init("headlamp21",mdModel,false);
    btHeadSignals13.Init("headlamp22",mdModel,false);
    btHeadSignals21.Init("headlamp13",mdModel,false);
    btHeadSignals22.Init("headlamp11",mdModel,false);
    btHeadSignals23.Init("headlamp12",mdModel,false);
//wyszukiwanie zderzakow

    for (int i=0; i<2; i++)
     {
       asAnimName=AnsiString("buffer_left0")+(i+1);
       smBuforLewy[i]= mdModel->GetFromName(asAnimName.c_str());
       asAnimName=AnsiString("buffer_right0")+(i+1);
       smBuforPrawy[i]= mdModel->GetFromName(asAnimName.c_str());
     }

     for (int i=0; i<iAxles; i++)
      dRailPosition[i]=dWheelsPosition[i]+MoverParameters->Dim.L+fDist;
//McZapkie-250202 end.

    Track->AddDynamicObject(this);
    //McZapkie: zmieniono na ilosc osi brane z chk
    //iNumAxles=(MoverParameters->NAxles>3 ? 4 : 2 );
    iNumAxles=2;
    //McZapkie-090402: odleglosc miedzy czopami skretu lub osiami
    double HalfMaxAxleDist=Max0R(MoverParameters->BDist,MoverParameters->ADist)*0.5;
    switch (iNumAxles)
    {//Ra: pojazdy wstawiaj¹ siê odwrotnie, ale nie jestem do koñca pewien, czy dobrze
     case 2: //ustawianie osi na torze
      Axle1.Init(Track,this);
      Axle4.Init(Track,this);
      Axle1.Move(iDirection*(-HalfMaxAxleDist-0.01)+fDist);
      Axle4.Move(iDirection*(+HalfMaxAxleDist+0.01)+fDist);
      //Axle2.Init(Track,this);
      //Axle3.Init(Track,this);
      //Axle2.Move(iDirection*(-HalfMaxAxleDist+0.01)+fDist);
      //Axle3.Move(iDirection*(+HalfMaxAxleDist-0.01)+fDist);
     break;
     case 4:
      Axle1.Init(Track,this);
      //Axle2.Init(Track,this);
      //Axle3.Init(Track,this);
      Axle4.Init(Track,this);
      Axle1.Move(iDirection*(-(HalfMaxAxleDist+MoverParameters->ADist*0.5))+fDist);
      //Axle2.Move(iDirection*(-(HalfMaxAxleDist-MoverParameters->ADist*0.5))+fDist);
      //Axle3.Move(iDirection*(+(HalfMaxAxleDist-MoverParameters->ADist*0.5))+fDist);
      Axle4.Move(iDirection*(+(HalfMaxAxleDist+MoverParameters->ADist*0.5))+fDist);
     break;
    }
    //pOldPos4=Axle1.pPosition; //Ra: nie u¿ywane
    //pOldPos1=Axle4.pPosition;
    //ActualTrack= GetTrack(); //McZapkie-030303
    //ABuWozki 060504
    smBogie[0]=mdModel->GetFromName("bogie1"); //Ra: bo nazwy s¹ ma³ymi
    smBogie[1]=mdModel->GetFromName("bogie2");
    if (!smBogie[0])
     smBogie[0]=mdModel->GetFromName("boogie01"); //Ra: alternatywna nazwa
    if (!smBogie[1])
     smBogie[1]=mdModel->GetFromName("boogie02"); //Ra: alternatywna nazwa
    //ABu: zainicjowanie zmiennej, zeby nic sie nie ruszylo
    //w pierwszej klatce, potem juz liczona prawidlowa wartosc masy

    MoverParameters->ComputeConstans();
    return true;
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
            bEnabled&= Axle1.Move(fDistance,MoverParameters->V>=0);
            bEnabled&= Axle4.Move(fDistance,MoverParameters->V<0);
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
{
 bEnabled&= Axle1.Move(fDistance,MoverParameters->V>=0); //pierwszy, gdy jedziemy do przodu
 bEnabled&= Axle4.Move(fDistance,MoverParameters->V<0); //pierwszy, gdy jedziemy do ty³u
 //Axle3.Move(fDistance,false); //te nigdy pierwsze nie s¹
 //Axle2.Move(fDistance,false);
};

void __fastcall TDynamicObject::AttachPrev(TDynamicObject *Object, int iType)
{//Ra: doczepia Object na koñcu sk³adu (nazwa funkcji jest myl¹ca)
 //Ra: u¿ywane tylko przy wczytywaniu scenerii
 //ABu: ponizej chyba najprostszy i najszybszy sposob pozbycia sie bledu :)
 //(iType czasami powinno byc rowne 0, a wynosi 48. Cholera wie, dlaczego...)
 //if (iType==48)
 //Ra: pewnie ³apa³o zero jako kod znaku... ale to ju¿ chyba nieaktualne?
 double l=0;
 //obliczanie d³ugoœci sk³adu
 for (TDynamicObject *Current=this;Current;Current=Current->iDirection<0?Current->NextConnected:Current->PrevConnected)
  if (Current->iDirection<0?Current->NextConnected:Current->PrevConnected) //czy jeszcze coœ jest?
   l+=Current->GetLength(); //dodanie jego d³ugoœci do d³ugoœci sk³adu
  else
  {
   l+=Current->GetLength()*0.5; //pierwszy w sk³adzie liczony od po³owy
   break;
  }
 /*double r=*/  //Ra: by³o bool rzutowane na double i zwracane jako bool (?)
 Object->Move(-l-(Object->GetLength())*0.5f); //przesuniêcie wózków na w³aœciwe miejsce w szeregu
 TLocation loc;
 loc.X=-GetPosition().x;
 loc.Y=GetPosition().z;
 loc.Z=GetPosition().y;                   /* TODO -cBUG : Remove this */
 MoverParameters->Loc=loc; //Ra: ustawienie siebie? po co?
 loc.X=-Object->GetPosition().x;
 loc.Y=Object->GetPosition().z;
 loc.Z=Object->GetPosition().y;
 Object->MoverParameters->Loc=loc; //ustawienie dodawanego pojazdu
 MoverParameters->Attach(iDirection<0?0:1,Object->iDirection<0?1:0,&(Object->MoverParameters),iType);
 MoverParameters->Couplers[iDirection<0?0:1].Render=false;
 Object->MoverParameters->Attach(Object->iDirection<0?1:0,iDirection<0?0:1,&MoverParameters,iType);
 Object->MoverParameters->Couplers[Object->iDirection<0?1:0].Render=true; //rysowanie sprzêgu w do³¹czanym
 if (iDirection<0)
 {//³¹czenie odwrotne
  PrevConnected=Object; //doczepiamy go sobie do sprzêgu 0, gdy stoimy odwrotnie
  PrevConnectedNo=Object->iDirection<0?1:0;
 }
 else
 {//³¹czenie standardowe
  NextConnected=Object; //doczepiamy go sobie do sprzêgu 1
  NextConnectedNo=Object->iDirection<0?1:0;
 }
 if (Object->iDirection<0)
 {//do³¹czany jest odwrotnie ustawiany
  Object->NextConnected=this; //on ma nas z ty³u
  Object->NextConnectedNo=iDirection<0?0:1;
 }
 else
 {//do³¹czany jest normalnie ustawiany
  Object->PrevConnected=this; //on ma nas z przodu
  Object->PrevConnectedNo=iDirection<0?0:1;
 }
 //ABu: To mala poprawka - sprawdzenie tablic Dynamics dla obiektow,
 //bo wagony sa blednie rozmieszczane przy starcie symulatora.
 Object->ABuCheckMyTrack();
 return;// r;
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
     MoverParameters->ComputeTotalForce(dt, dt1, FullVer);
 return true;    
}


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
  d=sqrt(SquareMagnitude(Axle4.pPosition-Axle1.pPosition));
  L=Axle1.GetLength(Axle1.pPosition,Axle1.pPosition-Axle2.pPosition,Axle4.pPosition-Axle3.pPosition,Axle4.pPosition);

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

void __fastcall TDynamicObject::UpdatePos()
{
  MoverParameters->Loc.X= -GetPosition().x;
  MoverParameters->Loc.Y=  GetPosition().z;
  MoverParameters->Loc.Z=  GetPosition().y;
}

bool __fastcall TDynamicObject::Update(double dt, double dt1)
{
#ifdef _DEBUG
    if (dt==0)
    {
        Error("dt==0");
        dt= 0.001;
    }
#endif
if (!MoverParameters->PhysicActivation)
     return true;   //McZapkie: wylaczanie fizyki gdy nie potrzeba

    if (!bEnabled)
        return false;

//McZapkie-260202
    MoverParameters->BatteryVoltage=90;
if (MoverParameters->EnginePowerSource.SourceType==CurrentCollector)
    if ((MechInside) || (MoverParameters->TrainType==dt_EZT))
{
    if ((!MoverParameters->PantCompFlag) && (MoverParameters->CompressedVolume>=2.8))
      MoverParameters->PantVolume= MoverParameters->CompressedVolume;
    if ((MoverParameters->CompressedVolume<2) && (MoverParameters->PantVolume<3))
      {
      if (!MoverParameters->PantCompFlag)
       MoverParameters->PantVolume= MoverParameters->CompressedVolume;
      MoverParameters->PantFront(false);
      MoverParameters->PantRear(false);
      }
 //Winger - automatyczne wylaczanie malej sprezarki.
    if (MoverParameters->PantVolume>=5)
     MoverParameters->PantCompFlag=false;
}

//Winger - odhamowywanie w EZT
//    if ((MoverParameters->TrainType==dt_EZT) && (MoverParameters->BrakeCtrlPos==-1) && (!MoverParameters->UnBrake))
//     {
//     MoverParameters->IncBrakeLevel(); // >BrakeCtrlPosNo=0;
//     }

    double dDOMoveLen;

    TLocation l;
    l.X=-GetPosition().x;
    l.Y=GetPosition().z;
    l.Z=GetPosition().y;
    TRotation r;
    r.Rx=r.Ry=r.Rz=0;
//McZapkie: parametry powinny byc pobierane z toru

    //TTrackShape ts;
    ts.R=MyTrack->fRadius;
    if (ABuGetDirection()<0) ts.R=-ts.R;

    //ts.R=ComputeRadius(Axle1.pPosition,Axle2.pPosition,Axle3.pPosition,Axle4.pPosition);
    ts.Len=Max0R(MoverParameters->BDist,MoverParameters->ADist);
    ts.dHtrack=Axle1.pPosition.y-Axle4.pPosition.y; //wektor miêdzy skrajnymi osiami
    ts.dHrail=((Axle1.GetRoll())+(Axle4.GetRoll()))*0.5f; //œrednia przechy³ka pud³a
    //TTrackParam tp;
    tp.Width=MyTrack->fTrackWidth;
//McZapkie-250202
    tp.friction=MyTrack->fFriction;
    tp.CategoryFlag=MyTrack->iCategoryFlag;
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
     if ((MoverParameters->PantFrontVolt) || (MoverParameters->PantRearVolt) ||
        //ABu: no i sprawdzenie dla EZT:
        ((MoverParameters->TrainType==dt_EZT)&&(MoverParameters->GetTrainsetVoltage())))
      NoVoltTime=0;
     else
      NoVoltTime=NoVoltTime+dt;
     if (NoVoltTime>1)
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
     tmpTraction.TractionMaxCurrent=7500;
     tmpTraction.TractionResistivity=0.3;

     


//McZapkie: predkosc w torze przekazac do TrackParam
//McZapkie: Vel ma wymiar km/h, V ma wymiar m/s, taka przyjalem notacje
    tp.Velmax=MyTrack->fVelocity;

    if (Mechanik)
    {
        //ABu: proba szybkiego naprawienia bledu z zatrzymujacymi sie bez powodu skladami
        if ((MoverParameters->CabNo!=0)&&(Controller!=Humandriver)&&(!MoverParameters->Mains)&&(Mechanik->EngineActive))
        {
         MoverParameters->PantRear(false);
         MoverParameters->PantFront(false);
         MoverParameters->PantRear(true);
         MoverParameters->PantFront(true);
         MoverParameters->DecMainCtrl(2); //?
         MoverParameters->MainSwitch(true);
        };
//yB: cos (AI) tu jest nie kompatybilne z czyms (hamulce)
//        if (Controller!=Humandriver)
//         if (Mechanik->LastReactionTime>0.5)
//          {
//           MoverParameters->BrakeCtrlPos=0;
//           Mechanik->LastReactionTime=0;
//          }

        if (Mechanik->UpdateSituation(dt1))  //czuwanie AI
//         if (Mechanik->ScanMe)
           {
            ScanEventTrack(MyTrack);
//            if(MoverParameters->BrakeCtrlPos>0)
//              MoverParameters->BrakeCtrlPos=MoverParameters->BrakeCtrlPosNo;
//            Mechanik->ScanMe= false;
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
    Global::ABuDebug=dDOMoveLen/dt1;
    ResetdMoveLen();
//McZapkie-260202
//tupot mew, tfu, stukot kol:
    DWORD stat;
//taka prowizorka zeby sciszyc stukot dalekiej lokomotywy
   double ObjectDist;
   double vol=0;
   double freq;
   ObjectDist=SquareMagnitude(Global::pCameraPosition-GetPosition());
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
                   rsStukot[i].Play(vol,0,MechInside,GetPosition()); //poprawic pozycje o uklad osi
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
   sBrakeAcc.Play(-1,0,MechInside,GetPosition());
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

//McZapkie-050402: krecenie kolami:
if (MoverParameters->Vel!=0)
   dWheelAngle+=MoverParameters->nrot*dt1*360;

   if (dWheelAngle>360)
    dWheelAngle-=360;

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
     sPantUp.Play(vol,0,MechInside,GetPosition());
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
     sPantUp.Play(vol,0,MechInside,GetPosition());
    pcp2p=true;
    }
   else
    pcp2p=false;

   //ObjectDist2=SquareMagnitude(Global::pCameraPosition-GetPosition())/100;
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
    //if (vol2<0) vol2=0; //Ra: vol2 nie u¿ywane dalej
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
    sPantDown.Play(vol,0,MechInside,GetPosition());
    MoverParameters->PantFrontSP=true;
    }
if ((MoverParameters->PantRearSP==false) && (MoverParameters->PantRearUp==false))
    {
    sPantDown.Play(vol,0,MechInside,GetPosition());
    MoverParameters->PantRearSP=true;
    }

//Winger 240404 - wylaczanie sprezarki i przetwornicy przy braku napiecia
if (tmpTraction.TractionVoltage==0)
    {
    MoverParameters->ConverterFlag=false;
    MoverParameters->CompressorFlag=false;
    }
}

//NBMX Obsluga drzwi, MC: zuniwersalnione
//   if (tempdoorfactor2!=120)
//    tempdoorfactor=random(100);
   if ((dDoorMoveL<MoverParameters->DoorMaxShiftL) && (MoverParameters->DoorLeftOpened))
     dDoorMoveL+=dt1*MoverParameters->DoorOpenSpeed/2;
   if ((dDoorMoveL>0) && (!MoverParameters->DoorLeftOpened))
   {
     dDoorMoveL-=dt1*MoverParameters->DoorCloseSpeed;
     if (dDoorMoveL<0)
       dDoorMoveL=0;
    }
   if ((dDoorMoveR<MoverParameters->DoorMaxShiftR) && (MoverParameters->DoorRightOpened))
     dDoorMoveR+=dt1*MoverParameters->DoorOpenSpeed/2;
   if ((dDoorMoveR>0) && (!MoverParameters->DoorRightOpened))
   {
    dDoorMoveR-=dt1*MoverParameters->DoorCloseSpeed;
    if (dDoorMoveR<0)
     dDoorMoveR=0;
   }

   if (Mechanik)
   {//ABu-160305 Testowanie gotowosci do jazdy
    if (MoverParameters->BrakePress<0.03*MoverParameters->MaxBrakePress)
     Mechanik->Ready=true; //wstêpnie gotowy
    //Ra: trzeba by sprawdziæ wszystkie, a nie tylko skrajne
    //sprawdzenie odhamowania skrajnych pojazdów
    TDynamicObject *tmp;
    tmp=GetFirstDynamic(1); //szukanie od strony sprzêgu 1
    if (tmp?tmp!=this:false) //NULL zdarzy siê tylko w przypadku b³êdu
     if (tmp->MoverParameters->BrakePress>0.03*tmp->MoverParameters->MaxBrakePress)
      Mechanik->Ready=false; //nie gotowy
    tmp=GetFirstDynamic(0); //szukanie od strony sprzêgu 0
    if (tmp?tmp!=this:false)
     if (tmp->MoverParameters->BrakePress>0.03*tmp->MoverParameters->MaxBrakePress)
      Mechanik->Ready=false; //nie gotowy
   }

//ABu-160303 sledzenie toru przed obiektem: *******************************
 //Z obserwacji: v>0 -> Coupler 0; v<0 ->coupler1 (Ra: prêdkoœæ jest zwi¹zana z pojazdem)
 //Rozroznienie jest tutaj, zeby niepotrzebnie
 //nie skakac do funkcji. Nie jest uzaleznione
 //od obecnosci AI, zeby uwzglednic np. jadace
 //bez lokomotywy wagony.
 EndTrack=false;
 if (CouplCounter>25) //licznik, aby nie robiæ za ka¿dym razem
 {//poszukiwanie czegoœ do zderzenia siê
  if (MoverParameters->V>0) //jeœli jedzie do przodu (w kierunku Coupler 0)
  {if (MoverParameters->Couplers[0].CouplingFlag==ctrain_virtual)
    ABuScanObjects(1,300); //szukanie czegoœ do pod³¹czenia
  }
  else if (MoverParameters->V<0) //jeœli jedzie do ty³u (w kierunku Coupler 1)
   if (MoverParameters->Couplers[1].CouplingFlag==ctrain_virtual)
    ABuScanObjects(-1,300);
  CouplCounter=random(20); //ponowne sprawdzenie po losowym czasie
 }
 if (MoverParameters->V!=0)
  CouplCounter++; //jazda sprzyja poszukiwaniu po³¹czenia
 else
  CouplCounter=25; //a bezruch nie
 return true; //Ra: chyba tak?
}

bool __fastcall TDynamicObject::FastUpdate(double dt)
{
#ifdef _DEBUG
    if (dt==0)
    {
        Error("dt==0");
        dt= 0.001;
    }
#endif
    double dDOMoveLen;
    if (!MoverParameters->PhysicActivation)
     return true;   //McZapkie: wylaczanie fizyki gdy nie potrzeba

    if (!bEnabled)
        return false;

    TLocation l;
    l.X=-GetPosition().x;
    l.Y=GetPosition().z;
    l.Z=GetPosition().y;
    TRotation r;
    r.Rx=r.Ry=r.Rz= 0;

//McZapkie: parametry powinny byc pobierane z toru
    //ts.R=MyTrack->fRadius;
    //ts.Len= Max0R(MoverParameters->BDist,MoverParameters->ADist);
    //ts.dHtrack= Axle1.pPosition.y-Axle4.pPosition.y;
    //ts.dHrail= ((Axle1.GetRoll())+(Axle4.GetRoll()))*0.5f;
    //tp.Width= MyTrack->fTrackWidth;
    //McZapkie-250202
    //tp.friction= MyTrack->fFriction;
    //tp.CategoryFlag= MyTrack->iCategoryFlag;
    //tp.DamageFlag=MyTrack->iDamageFlag;
    //tp.QualityFlag=MyTrack->iQualityFlag;
    //if (Mechanik)
    //{
    //
    //    if (Mechanik->UpdateSituation(dt))  //czuwanie AI
//  //       if (Mechanik->ScanMe)
    //      {
    //        ScanEventTrack(MyTrack);
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
ObjectDist=SquareMagnitude(Global::pCameraPosition-GetPosition());
if(ObjectDist<50000)
 if(TestFlag(MoverParameters->SoundFlag,sound_brakeacc))
   sBrakeAcc.Play(-1,0,MechInside,GetPosition());
 else;
// if(MoverParameters->BrakePress=0)
//   sBrakeAcc.Stop();
else
  sBrakeAcc.Stop();

SetFlag(MoverParameters->SoundFlag,-sound_brakeacc);
 return true; //Ra: chyba tak?
}

//McZapkie-040402: liczenie pozycji uwzgledniajac wysokosc szyn itp.
vector3 inline __fastcall TDynamicObject::GetPosition()
{
 //if (!this) return vector3(0,0,0);
 vector3 pos=(Axle1.pPosition+Axle4.pPosition)*0.5f; //œrodek miêdzy skrajnymi osiami
 if (MoverParameters->CategoryFlag&1)
 {//gdy jest pojazdem poruszaj¹cym siê po szynach (rail albo unimog)
  pos.x+=MoverParameters->OffsetTrackH*vLeft.x; //dodanie przesuniêcia w bok
  pos.z+=MoverParameters->OffsetTrackH*vLeft.z; //vLeft jest wektorem poprzecznym
  pos.y+=MoverParameters->OffsetTrackV+0.18; //wypadaloby tu prawdziwa wysokosc szyny dorobic
 }                                   //0.2
 else
 {
  pos.x+=MoverParameters->OffsetTrackH*vLeft.x;
  pos.z+=MoverParameters->OffsetTrackH*vLeft.z;
  pos.y+=MoverParameters->OffsetTrackV;   //te offsety sa liczone przez moverparam
 }
 return pos;
}

bool __fastcall TDynamicObject::Render()
{
//youBy - sprawdzamy, czy jest sens renderowac
double modelrotate;
vector3 tempangle;
// zmienne
renderme=false;
//przeklejka
    vector3 pos=GetPosition();
    double ObjSqrDist=SquareMagnitude(Global::pCameraPosition-pos);
//koniec przeklejki

     if (ObjSqrDist<500) //jak jest blisko - do 70m
      modelrotate=0.01f; //ma³y k¹t, ¿eby nie znika³o
     else
     {//Global::pCameraRotation to k¹t bewzglêdny w œwiecie (zero - na po³udnie)
      tempangle=(pos-Global::pCameraPosition); //wektor od kamery
      modelrotate=ABuAcos(tempangle); //okreœlenie k¹ta
      if (modelrotate>M_PI) modelrotate-=(2*M_PI);
      modelrotate+=Global::pCameraRotation;
     }

    if (modelrotate>M_PI) modelrotate-=(2*M_PI);
    if (modelrotate<-M_PI) modelrotate+=(2*M_PI);

    modelrotate=abs(modelrotate);

    if (modelrotate<maxrot) renderme=true;

 if (renderme)
 {
    TSubModel::iInstance=(int)this; //¿eby nie robiæ cudzych animacji
    AnsiString asLoadName="";
    //przejœcie na uk³ad wspó³rzêdnych modelu - tu siê zniekszta³ca?
    vFront=GetDirection();
    if ((MoverParameters->CategoryFlag==2) && (MoverParameters->CabNo<0)) //TODO: zrobic to eleganciej z plynnym zawracaniem
     vFront=-vFront;
    vUp=vWorldUp; //sta³a
    vFront.Normalize();
    vLeft=CrossProduct(vUp,vFront);
    vUp=CrossProduct(vFront,vLeft);
    matrix4x4 mat;
    mat.Identity();

    mat.BasisChange(vLeft,vUp,vFront);
    mMatrix=Inverse(mat);

    vector3 pos=GetPosition();
    double ObjSqrDist=SquareMagnitude(Global::pCameraPosition-pos);
    ABuLittleUpdate(ObjSqrDist);

    //ActualTrack= GetTrack(); //McZapkie-240702

#if RENDER_CONE
    {// Ra: do testu - renderowanie pozycji wózków w postaci ostros³upów
     double dir=RadToDeg(atan2(vLeft.z,vLeft.x));
     Axle4.Render(0);
     Axle1.Render(1); //bogieRot[0]
     //if (PrevConnected) //renderowanie po³¹czenia
    }
#endif

    glPushMatrix ( );
    //vector3 pos= GetPosition();
    //double ObjDist= SquareMagnitude(Global::pCameraPosition-pos);
    glTranslated(pos.x,pos.y,pos.z);
    glMultMatrixd(mMatrix.getArray());
    if (mdLowPolyInt)
     if ((FreeFlyModeFlag)||((!FreeFlyModeFlag)&&(!mdKabina)))
#ifdef USE_VBO
      if (Global::bUseVBO)
       mdLowPolyInt->RaRender(ObjSqrDist,ReplacableSkinID,bAlpha);
      else
#endif
       mdLowPolyInt->Render(ObjSqrDist,ReplacableSkinID,bAlpha);

#ifdef USE_VBO
    if (Global::bUseVBO)
     mdModel->RaRender(ObjSqrDist,ReplacableSkinID,bAlpha);
    else
#endif
     mdModel->Render(ObjSqrDist,ReplacableSkinID,bAlpha);
/* Ra: nie próbujemy wczytywaæ modeli miliony razy podczas renderowania!!!
    if ((mdLoad==NULL) && (MoverParameters->Load>0))
    {
     asLoadName=asBaseDir+MoverParameters->LoadType+".t3d";
     //asLoadName=MoverParameters->LoadType;
     //if (MoverParameters->LoadType!=AnsiString("passengers"))
     Global::asCurrentTexturePath=asBaseDir; //biezaca sciezka do tekstur to dynamic/...
     mdLoad=TModelsManager::GetModel(asLoadName.c_str()); //nowy ladunek
     Global::asCurrentTexturePath=AnsiString(szDefaultTexturePath); //z powrotem defaultowa sciezka do tekstur
    }
    if ((mdLoad==NULL) && (MoverParameters->Load>0))
     {
      mdLoad=NULL; //Ra: to jest tu bez sensu - co autor mia³ na myœli?
     }
*/
    if (mdLoad) //renderowanie nieprzezroczystego ³adunku
#ifdef USE_VBO
     if (Global::bUseVBO)
      mdLoad->RaRender(ObjSqrDist,ReplacableSkinID,bAlpha);
     else
#endif
      mdLoad->Render(ObjSqrDist,ReplacableSkinID,bAlpha);

//rendering przedsionkow o ile istnieja
    if (mdPrzedsionek)
     if (MoverParameters->filename==asBaseDir+"6ba.chk")
#ifdef USE_VBO
      if (Global::bUseVBO)
       mdPrzedsionek->RaRender(ObjSqrDist,ReplacableSkinID,bAlpha);
      else
#endif
       mdPrzedsionek->Render(ObjSqrDist,ReplacableSkinID,bAlpha);
//rendering kabiny gdy jest oddzielnym modelem i ma byc wyswietlana
//ABu: tylko w trybie FreeFly, zwykly tryb w world.cpp

    if ((mdKabina!=mdModel) && bDisplayCab && FreeFlyModeFlag)
    {//Ra: a œwiet³a nie zosta³y ju¿ ustawione dla toru?
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
//smierdzi
//      mdModel->Render(SquareMagnitude(Global::pCameraPosition-pos),0);

      glLightfv(GL_LIGHT0,GL_AMBIENT,Global::ambientDayLight);
      glLightfv(GL_LIGHT0,GL_DIFFUSE,Global::diffuseDayLight);
      glLightfv(GL_LIGHT0,GL_SPECULAR,Global::specularDayLight);
    }
    glPopMatrix();
} //yB - koniec mieszania z grafika


//McZapkie-010302: ulepszony dzwiek silnika
    double freq;
    double vol=0;
    double dt=Timer::GetDeltaTime();

//    double sounddist;
//    sounddist=SquareMagnitude(Global::pCameraPosition-GetPosition());

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
   if (MoverParameters->EventFlag)
    {
     if (TestFlag(MoverParameters->DamageFlag,dtrain_out) && GetVelocity()>0)
       rsDerailment.Play(1,0,true,GetPosition());
     if (GetVelocity()==0)
       rsDerailment.Stop();
    }
    if (btnOn==true)
    {
     btnOn=false;
     btCoupler1.TurnOff();
     btCoupler2.TurnOff();
     btCPneumatic1.TurnOff();
     btCPneumatic1r.TurnOff();
     btCPneumatic2.TurnOff();
     btCPneumatic2r.TurnOff();
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
    }
    return true;
};

bool __fastcall TDynamicObject::RenderAlpha()
{
 if (renderme)
 {
  TSubModel::iInstance=(int)this; //¿eby nie robiæ cudzych animacji
  vFront= GetDirection();
  if ((MoverParameters->CategoryFlag==2) && (MoverParameters->CabNo<0)) //TODO: zrobic to eleganciej z plynnym zawracaniem
   vFront=-vFront;
  vUp=vWorldUp; //Ra: jeœli to wskazuje pionowo w górê
  vFront.Normalize(); //a to w dó³ lub w górê, to mamy problem z ortogonalnoœci¹ i skalowaniem
  vLeft=CrossProduct(vUp,vFront);
  vUp=CrossProduct(vFront,vLeft);
  matrix4x4 mat;
  mat.Identity();
  mat.BasisChange(vLeft,vUp,vFront);
  mMatrix=Inverse(mat);
  vector3 pos=GetPosition();
  double ObjSqrDist=SquareMagnitude(Global::pCameraPosition-pos);
  ABuLittleUpdate(ObjSqrDist);

/*    for (int i=0; i<iAnimatedAxles; i++)
     if (smAnimatedWheel[i])
      smAnimatedWheel[i]->SetRotate(vector3(1,0,0),dWheelAngle);
           //NBMX wrzesien 2003, MC zuniwersalnione
   for (int i=0; i<iAnimatedDoors ; i++)
    {
    if (smAnimatedDoor[i]!=NULL)
     {
      if (MoverParameters->DoorOpenMethod==1)
       {
        if ((i % 2)==0)
         (smAnimatedDoor[i]->SetTranslate(vector3(0,0,1)*dDoorMoveL));
        else
         (smAnimatedDoor[i]->SetTranslate(vector3(0,0,1)*dDoorMoveR));
       }
      else
      if (MoverParameters->DoorOpenMethod==2)
       {
        if ((i % 2)==0)
         smAnimatedDoor[i]->SetRotate(vector3(1,0,0),dDoorMoveL);
        else
         smAnimatedDoor[i]->SetRotate(vector3(1,0,0),dDoorMoveR);
       }
     }
    }
//
    if (smWiazary[0])
       smWiazary[0]->SetRotate(vector3(1,0,0),-dWheelAngle);
    if (smWiazary[1])
       smWiazary[1]->SetRotate(vector3(1,0,0),-dWheelAngle);

    if (TestFlag(MoverParameters->Couplers[0].CouplingFlag,ctrain_coupler))
     btCoupler1.TurnOn();
    else
     btCoupler1.TurnOff();
    if (TestFlag(MoverParameters->Couplers[1].CouplingFlag,ctrain_coupler))
     btCoupler2.TurnOn();
    else
     btCoupler2.TurnOff();*/
    glPushMatrix ( );
    //vector3 pos= GetPosition();
    //double ObjDist= SquareMagnitude(Global::pCameraPosition-pos);
    glTranslatef(pos.x,pos.y,pos.z);
    glMultMatrixd(mMatrix.getArray());

#ifdef USE_VBO
    if (Global::bUseVBO)
     mdModel->RaRenderAlpha(ObjSqrDist,ReplacableSkinID,bAlpha);
    else
#endif
     mdModel->RenderAlpha(ObjSqrDist,ReplacableSkinID,bAlpha);

    if (mdLoad) //Ra: dodane renderowanie przezroczystego ³adunku
#ifdef USE_VBO
     if (Global::bUseVBO)
      mdLoad->RaRenderAlpha(ObjSqrDist,ReplacableSkinID,bAlpha);
     else
#endif
      mdLoad->RenderAlpha(ObjSqrDist,ReplacableSkinID,bAlpha);

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
    glPopMatrix ( );
    if (btnOn==true)
    {
     btnOn=false;
     btCoupler1.TurnOff();
     btCoupler2.TurnOff();
     btCPneumatic1.TurnOff();
     btCPneumatic1r.TurnOff();
     btCPneumatic2.TurnOff();
     btCPneumatic2r.TurnOff();
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
    }
}
    return true;

} //koniec renderalpha


//McZapkie-250202
//wczytywanie pliku z danymi multimedialnymi (dzwieki)
void __fastcall TDynamicObject::LoadMMediaFile(AnsiString BaseDir,AnsiString TypeName,AnsiString ReplacableSkin)
{
    double dSDist;
    TFileStream *fs;
    AnsiString asFileName=BaseDir+TypeName+".mmd";
    AnsiString asLoadName=BaseDir+MoverParameters->LoadType+".t3d";
    fs= new TFileStream(asFileName,fmOpenRead|fmShareCompat);
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
//    Parser->LoadStringToParse(asFile);
    Parser->First();
//    DecimalSeparator= '.';
    while (!Parser->EndOfFile && !Stop_InternalData)
    {
        str=Parser->GetNextSymbol().LowerCase();
        if (str==AnsiString("models:"))                            //modele i podmodele
        {
          asModel=Parser->GetNextSymbol().LowerCase();
          asModel=BaseDir+asModel; //McZapkie-200702 - dynamics maja swoje modele w dynamics/basedir
          Global::asCurrentTexturePath=BaseDir;                    //biezaca sciezka do tekstur to dynamic/...
          mdModel=TModelsManager::GetModel(asModel.c_str(),true);
          //if (mdModel) mdModel->GetSMRoot()->WillBeAnimated(); //Ra: taka proteza, bo ABu zmodyfikowa³ te transformy
          if (ReplacableSkin!=AnsiString("none"))
          {
           ReplacableSkin=Global::asCurrentTexturePath+ReplacableSkin;      //skory tez z dynamic/...
           ReplacableSkinID=TTexturesManager::GetTextureID(ReplacableSkin.c_str(),Global::iDynamicFiltering);
           bAlpha=TTexturesManager::GetAlpha(ReplacableSkinID);
          }
//Winger 040304 - ladowanie przedsionkow dla EZT
          if (MoverParameters->TrainType==dt_EZT)
          {
           asModel="przedsionki.t3d";
           asModel=BaseDir+asModel;
           mdPrzedsionek=TModelsManager::GetModel(asModel.c_str(),true);
           //if (mdPrzedsionek) mdPrzedsionek->GetSMRoot()->WillBeAnimated(); //Ra: taka proteza, bo ABu zmodyfikowa³ te transformy
          }
          if (MoverParameters->LoadAccepted!=AnsiString(""))
          //           if (MoverParameters->LoadAccepted!=AnsiString("")); // && MoverParameters->LoadType!=AnsiString("passengers"))
           if (MoverParameters->EnginePowerSource.SourceType==CurrentCollector)
           {
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
           {mdLoad=TModelsManager::GetModel(asLoadName.c_str(),true);  //ladunek
            //if (mdLoad) mdLoad->GetSMRoot()->WillBeAnimated(); //Ra: taka proteza, bo ABu zmodyfikowa³ te transformy
           }
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
            //if (mdLowPolyInt) mdLowPolyInt->GetSMRoot()->WillBeAnimated(); //Ra: taka proteza, bo ABu zmodyfikowa³ te transformy
           }
           else
           if (str==AnsiString("animwheelprefix:"))              //prefiks krecacych sie kol
            {
             str=Parser->GetNextSymbol();
             asAnimName="";
             for (int i=1; i<=MaxAnimatedAxles; i++)
              {
    //McZapkie-050402: wyszukiwanie kol o nazwie str*
               asAnimName=str+i;
               smAnimatedWheel[i-1]= mdModel->GetFromName(asAnimName.c_str());
               if (smAnimatedWheel[i-1]!=NULL)
                iAnimatedAxles+=1;
               else
                i=MaxAnimatedAxles+1;
              }
            }
           else
           if (str==AnsiString("animrodprefix:"))              //prefiks wiazarow dwoch
            {
             str= Parser->GetNextSymbol();
             asAnimName="";
             for (int i=1; i<=2; i++)
              {
    //McZapkie-050402: wyszukiwanie max 2 wiazarow o nazwie str*
               asAnimName=str+i;
               smWiazary[i-1]= mdModel->GetFromName(asAnimName.c_str());
              }
            }
           else
//Pantografy - Winger 160204
           if (str==AnsiString("animpantrd1prefix:"))              //prefiks ramion dolnych 1
            {
             str= Parser->GetNextSymbol();
             asAnimName="";
             for (int i=1; i<=2; i++)
              {
    //Winger 160204: wyszukiwanie max 2 patykow o nazwie str*
               asAnimName=str+i;
               smPatykird1[i-1]= mdModel->GetFromName(asAnimName.c_str());
              }
            }
           else
           if (str==AnsiString("animpantrd2prefix:"))              //prefiks ramion dolnych 2
            {
             str= Parser->GetNextSymbol();
             asAnimName="";
             for (int i=1; i<=2; i++)
              {
    //Winger 160204: wyszukiwanie max 2 patykow o nazwie str*
               asAnimName=str+i;
               smPatykird2[i-1]= mdModel->GetFromName(asAnimName.c_str());
              }
            }
           else
           if (str==AnsiString("animpantrg1prefix:"))              //prefiks ramion gornych 1
            {
             str= Parser->GetNextSymbol();
             asAnimName="";
             for (int i=1; i<=2; i++)
              {
    //Winger 160204: wyszukiwanie max 2 patykow o nazwie str*
               asAnimName=str+i;
               smPatykirg1[i-1]= mdModel->GetFromName(asAnimName.c_str());
              }
            }
           else
           if (str==AnsiString("animpantrg2prefix:"))              //prefiks ramion gornych 2
            {
             str= Parser->GetNextSymbol();
             asAnimName="";
             for (int i=1; i<=2; i++)
              {
    //Winger 160204: wyszukiwanie max 2 patykow o nazwie str*
               asAnimName=str+i;
               smPatykirg2[i-1]= mdModel->GetFromName(asAnimName.c_str());
              }
            }
           else
           if (str==AnsiString("animpantslprefix:"))              //prefiks slizgaczy
            {
             str= Parser->GetNextSymbol();
             asAnimName="";
             for (int i=1; i<=2; i++)
              {
    //Winger 160204: wyszukiwanie max 2 patykow o nazwie str*
               asAnimName=str+i;
               smPatykisl[i-1]= mdModel->GetFromName(asAnimName.c_str());
              }
            }
           else
    //Winger 010304: parametry pantografow
           if (str==AnsiString("pantfactors:"))              //prefiks slizgaczy
            {
             pant1x= Parser->GetNextSymbol().ToDouble();
             pant2x= Parser->GetNextSymbol().ToDouble();
             panty= Parser->GetNextSymbol().ToDouble();
             panth= Parser->GetNextSymbol().ToDouble();
             //              asAnimName="";
            }
           else
           if (str==AnsiString("animpendulumprefix:"))              //prefiks wahaczy
            {
             str= Parser->GetNextSymbol();
             asAnimName="";
             for (int i=1; i<=4; i++)
              {
    //McZapkie-050402: wyszukiwanie max 4 wahaczy o nazwie str*
               asAnimName=str+i;
               smWahacze[i-1]= mdModel->GetFromName(asAnimName.c_str());
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
              {
    //NBMX wrzesien 2003: wyszukiwanie drzwi o nazwie str*
               asAnimName=str+i;
               smAnimatedDoor[i-1]= mdModel->GetFromName(asAnimName.c_str());
               if (smAnimatedDoor[i-1]!=NULL)
                iAnimatedDoors+=1;
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
                rsStukot[i].Init(str.c_str(),dSDist,GetPosition().x,GetPosition().y+dWheelsPosition[i],GetPosition().z);
            }
            if (str!=AnsiString("end"))
             str= Parser->GetNextSymbol();
           }
          else
          if ((str==AnsiString("engine:")) && (MoverParameters->Power>0))   //plik z dzwiekiem silnika, mnozniki i ofsety amp. i czest.
           {
            str= Parser->GetNextSymbol();
            rsSilnik.Init(str.c_str(),Parser->GetNextSymbol().ToDouble(),GetPosition().x,GetPosition().y,GetPosition().z);
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
            rsWentylator.Init(str.c_str(),Parser->GetNextSymbol().ToDouble(),GetPosition().x,GetPosition().y,GetPosition().z);
            rsWentylator.AM=Parser->GetNextSymbol().ToDouble()/MoverParameters->RVentnmax;
            rsWentylator.AA=Parser->GetNextSymbol().ToDouble();
            rsWentylator.FM=Parser->GetNextSymbol().ToDouble()/MoverParameters->RVentnmax;
            rsWentylator.FA=Parser->GetNextSymbol().ToDouble();
           }
          else
          if (str==AnsiString("brake:"))                      //plik z piskiem hamulca, mnozniki i ofsety amplitudy.
           {
            str= Parser->GetNextSymbol();
            rsPisk.Init(str.c_str(),Parser->GetNextSymbol().ToDouble(),GetPosition().x,GetPosition().y,GetPosition().z);
            rsPisk.AM=Parser->GetNextSymbol().ToDouble();
            rsPisk.AA=Parser->GetNextSymbol().ToDouble()*(105-random(10))/100;
            rsPisk.FM=1.0;
            rsPisk.FA=0.0;
           }
          else
          if (str==AnsiString("brakeacc:"))                      //plik z przyspieszaczem (upust po zlapaniu hamowania)
           {
            str= Parser->GetNextSymbol();
            sBrakeAcc.Init(str.c_str(),Parser->GetNextSymbol().ToDouble(),GetPosition().x,GetPosition().y,GetPosition().z);
            sBrakeAcc.AM=1.0;
            sBrakeAcc.AA=0.0;
            sBrakeAcc.FM=1.0;
            sBrakeAcc.FA=0.0;
           }
          else
          if (str==AnsiString("derail:"))                      //dzwiek przy wykolejeniu
           {
            str= Parser->GetNextSymbol();
            rsDerailment.Init(str.c_str(),Parser->GetNextSymbol().ToDouble(),GetPosition().x,GetPosition().y,GetPosition().z);
            rsDerailment.AM=1.0;
            rsDerailment.AA=0.0;
            rsDerailment.FM=1.0;
            rsDerailment.FA=0.0;
           }
          else
          if (str==AnsiString("dieselinc:"))                      //dzwiek przy wlazeniu na obroty woodwarda
           {
            str= Parser->GetNextSymbol();
            rsDiesielInc.Init(str.c_str(),Parser->GetNextSymbol().ToDouble(),GetPosition().x,GetPosition().y,GetPosition().z);
            rsDiesielInc.AM=1.0;
            rsDiesielInc.AA=0.0;
            rsDiesielInc.FM=1.0;
            rsDiesielInc.FA=0.0;
           }
          else
          if (str==AnsiString("curve:"))
           {
            str= Parser->GetNextSymbol();
            rscurve.Init(str.c_str(),Parser->GetNextSymbol().ToDouble(),GetPosition().x,GetPosition().y,GetPosition().z);
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
            sPantUp.Init(str.c_str(),50,GetPosition().x,GetPosition().y,GetPosition().z);
            sPantUp.AM=50000;
            sPantUp.AA=-1*(105-random(10))/100;
            sPantUp.FM=1.0;
            sPantUp.FA=0.0;
           }
          if (str==AnsiString("pantographdown:"))            //pliki dzwiekow pantografow
           {
            str= Parser->GetNextSymbol();
            sPantDown.Init(str.c_str(),50,GetPosition().x,GetPosition().y,GetPosition().z);
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
}



//---------------------------------------------------------------------------

#pragma package(smart_init)

