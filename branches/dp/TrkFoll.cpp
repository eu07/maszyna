//---------------------------------------------------------------------------

/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak and others

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
 fOffsetH=0.0; //na starcie stoi na �rodku
}

__fastcall TTrackFollower::~TTrackFollower()
{
}

bool __fastcall TTrackFollower::Init(TTrack *pTrack,TDynamicObject *NewOwner,double fDir)
{
 fDirection=fDir;
 Owner=NewOwner;
 SetCurrentTrack(pTrack,0);
 iEventFlag=3; //na torze startowym r�wnie� wykona� eventy 1/2
 iEventallFlag=3;
 if ((pCurrentSegment))// && (pCurrentSegment->GetLength()<fFirstDistance))
  return false;
 return true;
}

TTrack* __fastcall TTrackFollower::SetCurrentTrack(TTrack *pTrack,int end)
{//przejechanie na inny odcinkek toru, z ewentualnym rozpruciem
 if (pTrack)
  switch (pTrack->eType)
  {case tt_Switch: //je�li zwrotnica, to przek�adamy j�, aby uzyska� dobry segment
   {int i=(end?pCurrentTrack->iNextDirection:pCurrentTrack->iPrevDirection);
    if (i>0) //je�eli wjazd z ostrza
     pTrack->SwitchForced(i>>1,Owner); //to prze�o�enie zwrotnicy - rozprucie!
   }
   break;
   case tt_Cross: //skrzy�owanie trzeba tymczasowo prze��czy�, aby wjecha� na w�a�ciwy tor
   {iSegment=Owner->RouteWish(pTrack); //nr segmentu zosta� ustalony podczas skanowania
    //Ra 2014-08: aby ustali� dalsz� tras�, nale�y zapyta� AI - trasa jest ustalana podczas skanowania
    //pTrack->CrossSegment(end?pCurrentTrack->iNextDirection:pCurrentTrack->iPrevDirection,i); //ustawienie w�a�ciwego wska�nika
    //powinno zwraca� kierunek do zapami�tania, bo segmenty mog� mie� r�ny kierunek
    //pTrack->SwitchForced(abs(iSegment)-1,NULL); //wyb�r wskazanego segmentu
    //if fDirection=(iSegment>0)?1.0:-1.0; //kierunek na tym segmencie jest ustalany bezpo�rednio
    if ((end?iSegment:-iSegment)<0)
     fDirection=-fDirection; //wt�rna zmiana
   }
   break;
  }
 if (!pTrack)
 {//gdy nie ma toru w kierunku jazdy
  pTrack=pCurrentTrack->NullCreate(end); //tworzenie toru wykolej�cego na przed�u�eniu pCurrentTrack
 }
 else
 {//najpierw +1, p�niej -1, aby odcinek izolowany wsp�lny dla tych tor�w nie wykry� zera
  pTrack->AxleCounter(+1,Owner); //zaj�cie nowego toru
  if (pCurrentTrack) pCurrentTrack->AxleCounter(-1,Owner); //opuszczenie tamtego toru
 }
 pCurrentTrack=pTrack;
 pCurrentSegment=(pCurrentTrack?pCurrentTrack->CurrentSegment():NULL);
 if (!pCurrentTrack)
  Error(Owner->MoverParameters->Name+" at NULL track");
 return pCurrentTrack;
};

bool __fastcall TTrackFollower::Move(double fDistance,bool bPrimary)
{//przesuwanie w�zka po torach o odleg�o�� (fDistance), z wyzwoleniem event�w
 //bPrimary=true - jest pierwsz� osi� w poje�dzie, czyli generuje eventy i przepisuje pojazd
 //Ra: zwraca false, je�li pojazd ma by� usuni�ty
 fDistance*=fDirection; //dystans mno�nony przez kierunek
 double s; //roboczy dystans
 double dir; //zapami�tany kierunek do sprawdzenia, czy si� zmieni�
 bool bCanSkip; //czy przemie�ci� pojazd na inny tor
 while (true)  //p�tla wychodzi, gdy przesuni�cie wyjdzie zerowe
 {//p�tla przesuwaj�ca w�zek przez kolejne tory, a� do trafienia w jaki�
  if (!pCurrentTrack) return false; //nie ma toru, to nie ma przesuwania
  if (pCurrentTrack->iEvents) //sumaryczna informacja o eventach
  {//omijamy ca�y ten blok, gdy tor nie ma on �adnych event�w (wi�kszo�� nie ma)
   if (fDistance<0)
   {
    if (iSetFlag(iEventFlag,-1)) //zawsze zeruje flag� sprawdzenia, jak mechanik dosi�dzie, to si� nie wykona
     if (Owner->Mechanik) //tylko dla jednego cz�onu
      //if (TestFlag(iEventFlag,1)) //McZapkie-280503: wyzwalanie event tylko dla pojazdow z obsada
      if (bPrimary && pCurrentTrack->evEvent1 && (!pCurrentTrack->evEvent1->iQueued))
       Global::AddToQuery(pCurrentTrack->evEvent1,Owner); //dodanie do kolejki
       //Owner->RaAxleEvent(pCurrentTrack->Event1); //Ra: dynamic zdecyduje, czy doda� do kolejki
    //if (TestFlag(iEventallFlag,1))
    if (iSetFlag(iEventallFlag,-1)) //McZapkie-280503: wyzwalanie eventall dla wszystkich pojazdow
     if (bPrimary && pCurrentTrack->evEventall1 && (!pCurrentTrack->evEventall1->iQueued))
      Global::AddToQuery(pCurrentTrack->evEventall1,Owner); //dodanie do kolejki
      //Owner->RaAxleEvent(pCurrentTrack->Eventall1); //Ra: dynamic zdecyduje, czy doda� do kolejki
   }
   else if (fDistance>0)
   {
    if (iSetFlag(iEventFlag,-2)) //zawsze ustawia flag� sprawdzenia, jak mechanik dosi�dzie, to si� nie wykona
     if (Owner->Mechanik) //tylko dla jednego cz�onu
      //if (TestFlag(iEventFlag,2)) //sprawdzanie jest od razu w pierwszym warunku
      if (bPrimary && pCurrentTrack->evEvent2 && (!pCurrentTrack->evEvent2->iQueued))
       Global::AddToQuery(pCurrentTrack->evEvent2,Owner);
       //Owner->RaAxleEvent(pCurrentTrack->Event2); //Ra: dynamic zdecyduje, czy doda� do kolejki
    //if (TestFlag(iEventallFlag,2))
    if (iSetFlag(iEventallFlag,-2)) //sprawdza i zeruje na przysz�o��, true je�li zmieni z 2 na 0
     if (bPrimary && pCurrentTrack->evEventall2 && (!pCurrentTrack->evEventall2->iQueued))
      Global::AddToQuery(pCurrentTrack->evEventall2,Owner);
      //Owner->RaAxleEvent(pCurrentTrack->Eventall2); //Ra: dynamic zdecyduje, czy doda� do kolejki
   }
   else //if (fDistance==0) //McZapkie-140602: wyzwalanie zdarzenia gdy pojazd stoi
   {
    if (Owner->Mechanik) //tylko dla jednego cz�onu
     if (pCurrentTrack->evEvent0)
      if (!pCurrentTrack->evEvent0->iQueued)
       Global::AddToQuery(pCurrentTrack->evEvent0,Owner);
       //Owner->RaAxleEvent(pCurrentTrack->Event0); //Ra: dynamic zdecyduje, czy doda� do kolejki
    if (pCurrentTrack->evEventall0)
     if (!pCurrentTrack->evEventall0->iQueued)
      Global::AddToQuery(pCurrentTrack->evEventall0,Owner);
      //Owner->RaAxleEvent(pCurrentTrack->Eventall0); //Ra: dynamic zdecyduje, czy doda� do kolejki
   }
  }
  if (!pCurrentSegment) //je�eli nie ma powi�zanego segmentu toru?
   return false;
  //if (fDistance==0.0) return true; //Ra: jak stoi, to chyba dalej nie ma co kombinowa�?
  s=fCurrentDistance+fDistance; //doliczenie przesuni�cia
  //Ra: W Point2 toru mo�e znajdowa� si� "dziura", kt�ra zamieni energi� kinetyczn�
  // ruchu wzd�u�nego na energi� potencjaln�, zamieniaj�c� si� potem na energi�
  // spr�ysto�ci na amortyzatorach. Nale�a�oby we wpisie toru umie�ci� wsp�czynnik
  // podzia�u energii kinetycznej.
  // Wsp�czynnik normalnie 1, z dziur� np. 0.99, a -1 b�dzie oznacza�o 100% odbicia (kozio�).
  // Albo w postaci k�ta: normalnie 0�, a 180� oznacza 100% odbicia (cosinus powy�szego).
/*
  if (pCurrentTrack->eType==tt_Cross)
  {//zjazdu ze skrzy�owania nie da si� okre�li� przez (iPrevDirection) i (iNextDirection)
   //int segment=Owner->RouteWish(pCurrentTrack); //numer segmentu dla skrzy�owa�
   //pCurrentTrack->SwitchForced(abs(segment)-1,NULL); //tymczasowo ustawienie tego segmentu
   //pCurrentSegment=pCurrentTrack->CurrentSegment(); //od�wie�y� sobie wska�nik segmentu (?)
  }
*/
  if (s<0)
  {//je�li przekroczenie toru od strony Point1
   bCanSkip=bPrimary?pCurrentTrack->CheckDynamicObject(Owner):false;
   if (bCanSkip) //tylko g��wna o� przenosi pojazd do innego toru
    Owner->MyTrack->RemoveDynamicObject(Owner); //zdejmujemy pojazd z dotychczasowego toru
   dir=fDirection;
   if (pCurrentTrack->eType==tt_Cross)
   {if (!SetCurrentTrack(pCurrentTrack->Neightbour(iSegment,fDirection),0))
     return false; //wyj�cie z b��dem
   }
   else if (!SetCurrentTrack(pCurrentTrack->Neightbour(-1,fDirection),0)) //ustawia fDirection
    return false; //wyj�cie z b��dem
   if (dir==fDirection) //(pCurrentTrack->iPrevDirection)
   {//gdy kierunek bez zmiany (Point1->Point2)
    fCurrentDistance=pCurrentSegment->GetLength();
    fDistance=s;
   }
   else
   {//gdy zmiana kierunku toru (Point1->Point1)
    fCurrentDistance=0;
    fDistance=-s;
   }
   if (bCanSkip)
   {//jak g��wna o�, to dodanie pojazdu do nowego toru
    pCurrentTrack->AddDynamicObject(Owner);
    iEventFlag=3; //McZapkie-020602: umozliwienie uruchamiania event1,2 po zmianie toru
    iEventallFlag=3; //McZapkie-280503: jw, dla eventall1,2
    if (!Owner->MyTrack) return false;
   }
   continue;
  }
  else if (s>pCurrentSegment->GetLength())
  {//je�li przekroczenie toru od strony Point2
   bCanSkip=bPrimary?pCurrentTrack->CheckDynamicObject(Owner):false;
   if (bCanSkip) //tylko g��wna o� przenosi pojazd do innego toru
    Owner->MyTrack->RemoveDynamicObject(Owner); //zdejmujemy pojazd z dotychczasowego toru
   fDistance=s-pCurrentSegment->GetLength();
   dir=fDirection;
   if (pCurrentTrack->eType==tt_Cross)
   {if (!SetCurrentTrack(pCurrentTrack->Neightbour(iSegment,fDirection),1))
     return false; //wyj�cie z b��dem
   }
   else if (!SetCurrentTrack(pCurrentTrack->Neightbour(1,fDirection),1)) //ustawia fDirection
    return false; //wyj�cie z b��dem
   if (dir!=fDirection) //(pCurrentTrack->iNextDirection)
   {//gdy zmiana kierunku toru (Point2->Point2)
    fDistance=-fDistance; //(s-pCurrentSegment->GetLength());
    fCurrentDistance=pCurrentSegment->GetLength();
   }
   else //gdy kierunek bez zmiany (Point2->Point1)
    fCurrentDistance=0;
   if (bCanSkip)
   {//jak g��wna o�, to dodanie pojazdu do nowego toru
    pCurrentTrack->AddDynamicObject(Owner);
    iEventFlag=3; //McZapkie-020602: umozliwienie uruchamiania event1,2 po zmianie toru
    iEventallFlag=3;
    if (!Owner->MyTrack) return false;
   }
   continue;
  }
  else
  {//gdy zostaje na tym samym torze (przesuwanie ju� nie zmienia toru)
   if (bPrimary)
   {//tylko gdy pocz�tkowe ustawienie, dodajemy eventy stania do kolejki
    if (Owner->MoverParameters->ActiveCab!=0)
    //if (Owner->MoverParameters->CabNo!=0)
    {
     if (pCurrentTrack->evEvent1 && pCurrentTrack->evEvent1->fDelay<=-1.0f)
      Global::AddToQuery(pCurrentTrack->evEvent1,Owner);
     if (pCurrentTrack->evEvent2 && pCurrentTrack->evEvent2->fDelay<=-1.0f)
      Global::AddToQuery(pCurrentTrack->evEvent2,Owner);
    }
    if (pCurrentTrack->evEventall1 && pCurrentTrack->evEventall1->fDelay<=-1.0f)
     Global::AddToQuery(pCurrentTrack->evEventall1,Owner);
    if (pCurrentTrack->evEventall2 && pCurrentTrack->evEventall2->fDelay<=-1.0f)
     Global::AddToQuery(pCurrentTrack->evEventall2,Owner);
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
  pCurrentSegment->RaPositionGet(fCurrentDistance,pPosition,vAngles);
  if (fDirection<0) //k�ty zale�� jeszcze od zwrotu na torze
  {//k�ty s� w przedziale <-M_PI;M_PI>
   vAngles.x=-vAngles.x; //przechy�ka jest w przecinw� stron�
   vAngles.y=-vAngles.y; //pochylenie jest w przecinw� stron�
   vAngles.z+=(vAngles.z>=M_PI)?-M_PI:M_PI; //ale kierunek w planie jest obr�cony o 180�
  }
  if (fOffsetH!=0.0)
  {//je�li przesuni�cie wzgl�dem osi toru, to je doliczy�

  }
  return true;
 }
 return false;
}

void __fastcall TTrackFollower::Render(float fNr)
{//funkcja rysuj�ca sto�ek w miejscu osi
 glPushMatrix(); //matryca kamery
  glTranslatef(pPosition.x,pPosition.y+6,pPosition.z); //6m ponad
  glRotated(RadToDeg(-vAngles.z),0,1,0); //obr�t wzgl�dem osi OY
  //glRotated(RadToDeg(vAngles.z),0,1,0); //obr�t wzgl�dem osi OY
  glDisable(GL_LIGHTING);
  glColor3f(1.0-fNr,fNr,0); //czerwone dla 0, zielone dla 1
  //glutWireCone(promie� podstawy,wysoko��,k�tno�� podstawy,ilo�� segment�w na wysoko��)
  glutWireCone(0.5,2,4,1); //rysowanie sto�ka (ostros�upa o podstawie wieloboka)
  glEnable(GL_LIGHTING);
 glPopMatrix();
}

//---------------------------------------------------------------------------

#pragma package(smart_init)
