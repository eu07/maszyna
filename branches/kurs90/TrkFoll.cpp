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

#include "system.hpp"
#include "classes.hpp"

#include "opengl/glew.h"
#include "opengl/glut.h"

#pragma hdrstop

#include "TrkFoll.h"
#include "Globals.h"
#include "DynObj.h"
#include "Ground.h"
#include "Event.h"

__fastcall TTrackFollower::TTrackFollower()
{
 pCurrentTrack=NULL;
 pCurrentSegment=NULL;
 fCurrentDistance=0;
 pPosition=vAngles=vector3(0,0,0);
 fDirection=1; //jest przodem do Point2
 fOffsetH=0.0; //na starcie stoi na œrodku
}

__fastcall TTrackFollower::~TTrackFollower()
{
}

bool __fastcall TTrackFollower::Init(TTrack *pTrack,TDynamicObject *NewOwner,double fDir)
{
 fDirection=fDir;
 Owner=NewOwner;
 SetCurrentTrack(pTrack,0);
 iEventFlag=3; //na torze startowym równie¿ wykonaæ eventy 1/2
 iEventallFlag=3;
 if ((pCurrentSegment))// && (pCurrentSegment->GetLength()<fFirstDistance))
  return false;
 return true;
}

void __fastcall TTrackFollower::SetCurrentTrack(TTrack *pTrack,int end)
{//przejechanie na inny odcinkek toru, z ewentualnym rozpruciem
 if (pTrack?pTrack->eType==tt_Switch:false) //jeœli zwrotnica, to przek³adamy j¹, aby uzyskaæ dobry segment
 {int i=(end?pCurrentTrack->iNextDirection:pCurrentTrack->iPrevDirection);
  if (i>0) //je¿eli wjazd z ostrza
   pTrack->SwitchForced(i>>1,Owner); //to prze³o¿enie zwrotnicy - rozprucie!
 }
 if (!pTrack)
 {//gdy nie ma toru w kierunku jazdy
  //if (pCurrentTrack->iCategoryFlag&1) //jeœli tor kolejowy
   pTrack=pCurrentTrack->NullCreate(end); //tworzenie toru wykolej¹cego na przed³u¿eniu pCurrentTrack
 }
 else
 {//najpierw +1, póŸniej -1, aby odcinek izolowany wspólny dla tych torów nie wykry³ zera
  pTrack->AxleCounter(+1,Owner); //zajêcie nowego toru
  if (pCurrentTrack) pCurrentTrack->AxleCounter(-1,Owner); //opuszczenie tamtego toru
 }
 pCurrentTrack=pTrack;
 pCurrentSegment=(pCurrentTrack?pCurrentTrack->CurrentSegment():NULL);
};

bool __fastcall TTrackFollower::Move(double fDistance,bool bPrimary)
{//przesuwanie wózka po torach o odleg³oœæ (fDistance), z wyzwoleniem eventów
 //bPrimary=true - jest pierwsz¹ osi¹ w pojeŸdzie, czyli generuje eventy i przepisuje pojazd
 //Ra: zwraca false, jeœli pojazd ma byæ usuniêty
 fDistance*=fDirection; //dystans mno¿nony przez kierunek
 double s;
 bool bCanSkip; //czy przemieœciæ pojazd na inny tor
 while (true)  //pêtla wychodzi, gdy przesuniêcie wyjdzie zerowe
 {//pêtla przesuwaj¹ca wózek przez kolejne tory, a¿ do trafienia w jakiœ
  if (!pCurrentTrack) return false; //nie ma toru, to nie ma przesuwania
  if (pCurrentTrack->iEvents) //sumaryczna informacja o eventach
  {//omijamy ca³y ten blok, gdy tor nie ma on ¿adnych eventów (wiêkszoœc nie ma)
   if (fDistance<0)
   {
    if (iSetFlag(iEventFlag,-1)) //zawsze zeruje flagê sprawdzenia, jak mechanik dosi¹dzie, to siê nie wykona
     if (Owner->Mechanik) //tylko dla jednego cz³onu
      //if (TestFlag(iEventFlag,1)) //McZapkie-280503: wyzwalanie event tylko dla pojazdow z obsada
      if (bPrimary && pCurrentTrack->Event1 && (pCurrentTrack->Event1->fStartTime<=0))
        //Global::pGround->AddToQuery(pCurrentTrack->Event1,Owner); //dodanie do kolejki
        Owner->RaAxleEvent(pCurrentTrack->Event1); //Ra: dynamic zdecyduje, czy dodaæ do kolejki
    //if (TestFlag(iEventallFlag,1))
    if (iSetFlag(iEventallFlag,-1)) //McZapkie-280503: wyzwalanie eventall dla wszystkich pojazdow
     if (bPrimary && pCurrentTrack->Eventall1 && (pCurrentTrack->Eventall1->fStartTime<=0))
      //Global::pGround->AddToQuery(pCurrentTrack->Eventall1,Owner); //dodanie do kolejki
      Owner->RaAxleEvent(pCurrentTrack->Eventall1); //Ra: dynamic zdecyduje, czy dodaæ do kolejki
   }
   else if (fDistance>0)
   {
    if (iSetFlag(iEventFlag,-2)) //zawsze ustawia flagê sprawdzenia, jak mechanik dosi¹dzie, to siê nie wykona
     if (Owner->Mechanik) //tylko dla jednego cz³onu
      //if (TestFlag(iEventFlag,2)) //sprawdzanie jest od razu w pierwszym warunku
      if (bPrimary && pCurrentTrack->Event2 && (pCurrentTrack->Event2->fStartTime<=0))
       //Global::pGround->AddToQuery(pCurrentTrack->Event2,Owner);
       Owner->RaAxleEvent(pCurrentTrack->Event2); //Ra: dynamic zdecyduje, czy dodaæ do kolejki
    //if (TestFlag(iEventallFlag,2))
    if (iSetFlag(iEventallFlag,-2)) //sprawdza i zeruje na przysz³oœæ, true jeœli zmieni z 2 na 0
     if (bPrimary && pCurrentTrack->Eventall2 && (pCurrentTrack->Eventall2->fStartTime<=0))
      //Global::pGround->AddToQuery(pCurrentTrack->Eventall2,Owner);
      Owner->RaAxleEvent(pCurrentTrack->Eventall2); //Ra: dynamic zdecyduje, czy dodaæ do kolejki
   }
   else //if (fDistance==0) //McZapkie-140602: wyzwalanie zdarzenia gdy pojazd stoi
   {
    if (Owner->Mechanik) //tylko dla jednego cz³onu
     if (pCurrentTrack->Event0)
      if ((pCurrentTrack->Event0->fStartTime<=0)&&(pCurrentTrack->Event0->fDelay!=0))
       //Global::pGround->AddToQuery(pCurrentTrack->Event0,Owner);
       Owner->RaAxleEvent(pCurrentTrack->Event0); //Ra: dynamic zdecyduje, czy dodaæ do kolejki
    if (pCurrentTrack->Eventall0)
     if ((pCurrentTrack->Eventall0->fStartTime<=0)&&(pCurrentTrack->Eventall0->fDelay!=0))
      //Global::pGround->AddToQuery(pCurrentTrack->Eventall0,Owner);
      Owner->RaAxleEvent(pCurrentTrack->Eventall0); //Ra: dynamic zdecyduje, czy dodaæ do kolejki
   }
  }
  if (!pCurrentSegment) //je¿eli nie ma powi¹zanego segmentu toru?
   return false;
  //if (fDistance==0.0) return true; //Ra: jak stoi, to chyba dalej nie ma co kombinowaæ?
  s=fCurrentDistance+fDistance; //doliczenie przesuniêcia
  //Ra: W Point1 toru mo¿e znajdowaæ siê "dziura", która zamieni energiê kinetyczn¹
  // ruchu wzd³u¿nego na energiê potencjaln¹, zamieniaj¹c¹ siê potem na energiê
  // sprê¿ystoœci na amortyzatorach. Nale¿a³oby we wpisie toru umieœciæ wspó³czynnik
  // podzia³u energii kinetycznej.
  if (s<0)
  {//jeœli przekroczenie toru od strony Point1
   bCanSkip=bPrimary?pCurrentTrack->CheckDynamicObject(Owner):false;
   if (bCanSkip) //tylko g³ówna oœ przenosi pojazd do innego toru
    Owner->MyTrack->RemoveDynamicObject(Owner); //zdejmujemy pojazd z dotychczasowego toru
   if (pCurrentTrack->iPrevDirection)
   {//gdy kierunek bez zmiany (Point1->Point2)
    SetCurrentTrack(pCurrentTrack->CurrentPrev(),0);
    if (pCurrentTrack==NULL)
    {
     Error(Owner->MoverParameters->Name+" at NULL track");
     return false; //wyjœcie z b³êdem
    }
    fCurrentDistance=pCurrentSegment->GetLength();
    fDistance=s;
   }
   else
   {//gdy zmiana kierunku toru (Point1->Point1)
    SetCurrentTrack(pCurrentTrack->CurrentPrev(),0); //ustawienie (pCurrentTrack)
    fCurrentDistance=0;
    fDistance=-s;
    fDirection=-fDirection;
    if (pCurrentTrack==NULL)
    {
     Error(Owner->MoverParameters->Name+" at NULL track");
     return false; //wyjœcie z b³êdem
    }
   }
   if (bCanSkip)
   {//jak g³ówna oœ, to dodanie pojazdu do nowego toru
    pCurrentTrack->AddDynamicObject(Owner);
    iEventFlag=3; //McZapkie-020602: umozliwienie uruchamiania event1,2 po zmianie toru
    iEventallFlag=3; //McZapkie-280503: jw, dla eventall1,2
    if (!Owner->MyTrack) return false;
   }
   continue;
  }
  else if (s>pCurrentSegment->GetLength())
  {//jeœli przekroczenie toru od strony Point2
   bCanSkip=bPrimary?pCurrentTrack->CheckDynamicObject(Owner):false;
   if (bCanSkip) //tylko g³ówna oœ przenosi pojazd do innego toru
    Owner->MyTrack->RemoveDynamicObject(Owner); //zdejmujemy pojazd z dotychczasowego toru
   if (pCurrentTrack->iNextDirection)
   {//gdy zmiana kierunku toru (Point2->Point2)
    fDistance=-(s-pCurrentSegment->GetLength());
    SetCurrentTrack(pCurrentTrack->CurrentNext(),1);
    if (pCurrentTrack==NULL)
    {
     Error(Owner->MoverParameters->Name+" at NULL track");
     return false; //wyjœcie z b³êdem
    }
    fCurrentDistance=pCurrentSegment->GetLength();
    fDirection=-fDirection;
   }
   else
   {//gdy kierunek bez zmiany (Point2->Point1)
    fDistance=s-pCurrentSegment->GetLength();
    SetCurrentTrack(pCurrentTrack->CurrentNext(),1);
    fCurrentDistance=0;
    if (pCurrentTrack==NULL)
    {
     Error(Owner->MoverParameters->Name+" at NULL track");
     return false; //wyjœcie z b³êdem
    }
   }
   if (bCanSkip)
   {//jak g³ówna oœ, to dodanie pojazdu do nowego toru
    pCurrentTrack->AddDynamicObject(Owner);
    iEventFlag=3; //McZapkie-020602: umozliwienie uruchamiania event1,2 po zmianie toru
    iEventallFlag=3;
    if (!Owner->MyTrack) return false;
   }
   continue;
  }
  else
  {//gdy zostaje na tym samym torze (przesuwanie ju¿ nie zmienia toru)
   if (bPrimary)
   {//tylko gdy pocz¹tkowe ustawienie, dodajemy eventy stania do kolejki
    if (Owner->MoverParameters->CabNo!=0)
    {
     if (pCurrentTrack->Event1 && pCurrentTrack->Event1->fDelay<=-1.0f)
      Global::pGround->AddToQuery(pCurrentTrack->Event1,Owner);
     if (pCurrentTrack->Event2 && pCurrentTrack->Event2->fDelay<=-1.0f)
      Global::pGround->AddToQuery(pCurrentTrack->Event2,Owner);
    }
    if (pCurrentTrack->Eventall1 && pCurrentTrack->Eventall1->fDelay<=-1.0f)
     Global::pGround->AddToQuery(pCurrentTrack->Eventall1,Owner);
    if (pCurrentTrack->Eventall2 && pCurrentTrack->Eventall2->fDelay<=-1.0f)
     Global::pGround->AddToQuery(pCurrentTrack->Eventall2,Owner);
   }
   fCurrentDistance=s;
   //fDistance=0;
   return ComputatePosition(); //przeliczenie XYZ, true o ile nie wyjecha³ na NULL
  }
 }
};

bool __fastcall TTrackFollower::ComputatePosition()
{//ustalenie wspó³rzêdnych XYZ
 if (pCurrentSegment) //o ile jest tor
 {
  //pPosition=pCurrentSegment->GetPoint(fCurrentDistance); //wyliczenie z dystansu od Point1
  pCurrentSegment->RaPositionGet(fCurrentDistance,pPosition,vAngles);
  if (fDirection<0) //k¹ty zale¿¹ jeszcze od zwrotu na torze
  {vAngles.x=-vAngles.x; //przechy³ka jest w przecinw¹ stronê
   vAngles.y=-vAngles.y; //pochylenie jest w przecinw¹ stronê
   vAngles.z+=M_PI; //ale kierunek w planie jest obrócony o 180° 
  }
  if (fOffsetH!=0.0)
  {//jeœli przesuniêcie wzglêdem osi toru, to je doliczyæ

  }
  return true;
 }
 return false;
}

void __fastcall TTrackFollower::Render(float fNr)
{//funkcja rysuj¹ca sto¿ek w miejscu osi
 glPushMatrix(); //matryca kamery
  glTranslatef(pPosition.x,pPosition.y+6,pPosition.z); //6m ponad
  glRotated(RadToDeg(-vAngles.z),0,1,0); //obrót wzglêdem osi OY
  //glRotated(RadToDeg(vAngles.z),0,1,0); //obrót wzglêdem osi OY
  glDisable(GL_LIGHTING);
  glColor3f(1.0-fNr,fNr,0); //czerwone dla 0, zielone dla 1
  //glutWireCone(promieñ podstawy,wysokoœæ,k¹tnoœæ podstawy,iloœæ segmentów na wysokoœæ)
  glutWireCone(0.5,2,4,1); //rysowanie sto¿ka (ostros³upa o podstawie wieloboka)
  glEnable(GL_LIGHTING);
 glPopMatrix();
}

//---------------------------------------------------------------------------

#pragma package(smart_init)
