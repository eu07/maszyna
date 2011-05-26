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

#include "TrkFoll.h"
#include "Globals.h"
#include "DynObj.h"
#include "Ground.h"

__fastcall TTrackFollower::TTrackFollower()
{
 pCurrentTrack=NULL;
 pCurrentSegment=NULL;
 fCurrentDistance=0;
 pPosition=vector3(0,0,0);
 fDirection=1;
}

__fastcall TTrackFollower::~TTrackFollower()
{
}

bool __fastcall TTrackFollower::Init(TTrack *pTrack,TDynamicObject *NewOwner,double fDir)
{
 fDirection=fDir;
 Owner=NewOwner;
 SetCurrentTrack(pTrack);
 iEventFlag=0;
 iEventallFlag=0;
 if ((pCurrentSegment))// && (pCurrentSegment->GetLength()<fFirstDistance))
  return false;
 return true;
}

bool __fastcall TTrackFollower::Move(double fDistance, bool bPrimary)
{//przesuwanie w�zka po torach o odleg�o�� (fDistance), z wyzwoleniem event�w
 fDistance*=fDirection; //dystans mno�nony przez kierunek
 double s;
 bool bCanSkip;
 while (true)
 {//p�tla przesuwaj�ca w�zek przez kolejne tory, a� do trafienia w jaki�
  if (!pCurrentTrack) return false; //nie ma toru, to nie ma przesuwania
  if (pCurrentTrack->iEvents) //sumaryczna informacja o eventach
  {//omijamy ca�y ten blok, gdy tor nie ma �adnych event�w (wi�kszo�c nie ma)
   if (fDistance<0)
   {
    if (Owner->MoverParameters->CabNo!=0) //McZapkie-280503: wyzwalanie event tylko dla pojazdow z obsada
     if (TestFlag(iEventFlag,1))
      if (iSetFlag(iEventFlag,-1))
       if (bPrimary && pCurrentTrack->Event1 && pCurrentTrack->Event1->fStartTime<=0)
        Global::pGround->AddToQuery(pCurrentTrack->Event1,Owner); //dodanie do kolejki
    if (TestFlag(iEventallFlag,1))        //McZapkie-280503: wyzwalanie eventall dla wszystkich pojazdow
     if (iSetFlag(iEventallFlag,-1))
      if (bPrimary && pCurrentTrack->Eventall1 && pCurrentTrack->Eventall1->fStartTime<=0)
       Global::pGround->AddToQuery(pCurrentTrack->Eventall1,Owner); //dodanie do kolejki
   }
   else if (fDistance>0)
   {
    if (Owner->MoverParameters->CabNo!=0)
     if (TestFlag(iEventFlag,2))
      if (iSetFlag(iEventFlag,-2))
       if (bPrimary && pCurrentTrack->Event2 && pCurrentTrack->Event2->fStartTime<=0)
        Global::pGround->AddToQuery(pCurrentTrack->Event2,Owner);
    if (TestFlag(iEventallFlag,2))
     if (iSetFlag(iEventallFlag,-2))
      if (bPrimary && pCurrentTrack->Eventall2 && pCurrentTrack->Eventall2->fStartTime<=0)
       Global::pGround->AddToQuery(pCurrentTrack->Eventall2,Owner);
   }
   else //if (fDistance==0) //McZapkie-140602: wyzwalanie zdarzenia gdy pojazd stoi
   {
    if (Owner->MoverParameters->CabNo!=0)
     if (pCurrentTrack->Event0)
      if (pCurrentTrack->Event0->fStartTime<=0 && pCurrentTrack->Event0->fDelay!=0)
       Global::pGround->AddToQuery(pCurrentTrack->Event0,Owner);
    if (pCurrentTrack->Eventall0)
     if (pCurrentTrack->Eventall0->fStartTime<=0 && pCurrentTrack->Eventall0->fDelay!=0)
      Global::pGround->AddToQuery(pCurrentTrack->Eventall0,Owner);
   }
  }
  if (!pCurrentSegment) //je�eli nie ma powi�zanego toru?
   return false;
  //if (fDistance==0.0) return true; //Ra: jak stoi, to chyba dalej nie ma co kombinowa�?
  s=fCurrentDistance+fDistance; //doliczenie przesuni�cia
  //(pCurrentTrack->eType); //Ra: to nic nie daje, bo to nie funkcja
  //Ra: W Point1 toru mo�e znajdowa� si� "dziura", kt�ra zamieni energi� kinetyczn�
  // ruchu wzd�u�nego na energi� potencjaln�, zamieniaj�c� si� potem na energi�
  // spr�ysto�ci na amortyzatorach. Nale�a�oby we wpisie toru umie�ci� wsp�czynnik
  // podzia�u energii kinetycznej. 
  if (s<0)
  {//je�li przekroczenie toru od strony Point1
   bCanSkip=bPrimary && pCurrentTrack->CheckDynamicObject(Owner);
   if (bCanSkip)
    pCurrentTrack->RemoveDynamicObject(Owner);
   if (pCurrentTrack->bPrevSwitchDirection)
   {//gdy zmiana kierunku toru (Point1-Point1)
    SetCurrentTrack(pCurrentTrack->CurrentPrev()); //ustawienie (pCurrentTrack)
    fCurrentDistance=0;
    fDistance=-s;
    fDirection=-fDirection;
    if (pCurrentTrack==NULL)
    {
     Error(Owner->MoverParameters->Name+" at NULL track");
     return false; //wyj�cie z b��dem
    }
   }
   else
   {//gdy kierunek bez zmiany (Point1-Point2)
    SetCurrentTrack(pCurrentTrack->CurrentPrev());
    if (pCurrentTrack==NULL)
    {
     Error(Owner->MoverParameters->Name+" at NULL track");
     return false; //wyj�cie z b��dem
    }
    fCurrentDistance=pCurrentSegment->GetLength();
    fDistance=s;
   }
   if (bCanSkip)
   {
    pCurrentTrack->AddDynamicObject(Owner);
    iEventFlag=3; //McZapkie-020602: umozliwienie uruchamiania event1,2 po zmianie toru
    iEventallFlag=3; //McZapkie-280503: jw, dla eventall1,2
   }
   //event1 przesuniete na gore
   continue;
  }
  else if (s>pCurrentSegment->GetLength())
  {//je�li przekroczenie toru od strony Point2
   bCanSkip=bPrimary && pCurrentTrack->CheckDynamicObject(Owner);
   if (bCanSkip)
    pCurrentTrack->RemoveDynamicObject(Owner);
   if (pCurrentTrack->bNextSwitchDirection)
   {//gdy zmiana kierunku toru (Point2-Point2)
    fDistance=-(s-pCurrentSegment->GetLength());
    SetCurrentTrack(pCurrentTrack->CurrentNext());
    if (pCurrentTrack==NULL)
    {
     Error(Owner->MoverParameters->Name+" at NULL track");
     return false; //wyj�cie z b��dem
    }
    fCurrentDistance=pCurrentSegment->GetLength();
    fDirection=-fDirection;
   }
   else
   {//gdy kierunek bez zmiany (Point2-Point1)
    fDistance=s-pCurrentSegment->GetLength();
    SetCurrentTrack(pCurrentTrack->CurrentNext());
    fCurrentDistance=0;
    if (pCurrentTrack==NULL)
    {
     Error(Owner->MoverParameters->Name+" at NULL track");
     return false; //wyj�cie z b��dem
    }
   }
   if (bCanSkip)
   {
    pCurrentTrack->AddDynamicObject(Owner);
    iEventFlag=3; //McZapkie-020602: umozliwienie uruchamiania event1,2 po zmianie toru
    iEventallFlag=3;
   }
   //event2 przesuniete na gore
   continue;
  }
  else
  {//gdy zostaje na tym samym torze (przesuwanie ju� nie zmienia toru)
   if (bPrimary)
   {
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
   return ComputatePosition(); //przeliczenie XYZ, true o ile nie wyjecha� na NULL
  }
 }
};

bool __fastcall TTrackFollower::ComputatePosition()
{//ustalenie wsp�rz�dnych XYZ
 if (pCurrentSegment) //o ile jest tor
 {
  pPosition=pCurrentSegment->GetPoint(fCurrentDistance); //wyliczenie z dystansu od Point1
  return true;
 }
 return false;
}

void __fastcall TTrackFollower::Render()
{//funkcja rysuj�ca sto�ek w miejscu w�zka
 glPushMatrix();
  glTranslatef(pPosition.x,pPosition.y,pPosition.z);
  glRotatef(-90,1,0,0);
  glutSolidCone(5,10,4,1); //rysowanie sto�ka
 glPopMatrix();
}

//---------------------------------------------------------------------------

#pragma package(smart_init)
