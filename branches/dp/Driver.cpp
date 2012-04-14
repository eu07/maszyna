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

#include "Driver.h"
#include <mtable.hpp>
#include "DynObj.h"
#include <math.h>
#include "Globals.h"
#include "Event.h"
#include "Ground.h"
#include "MemCell.h"

#define LOGVELOCITY 1
#define LOGSTOPS 1
#define TESTTABLE 1
/*

Modu� obs�uguj�cy sterowanie pojazdami (sk�adami poci�g�w, samochodami).
Ma dzia�a� zar�wno jako AI oraz przy prowadzeniu przez cz�owieka. W tym
drugim przypadku jedynie informuje za pomoc� napis�w o tym, co by zrobi�
w tym pierwszym. Obejmuje zar�wno maszynist� jak i kierownika poci�gu
(dawanie sygna�u do odjazdu).

Przeniesiona tutaj zosta�a zawarto�� ai_driver.pas przerobione na C++.
R�wnie� niekt�re funkcje dotycz�ce sk�ad�w z DynObj.cpp.

*/


//zrobione:
//0. pobieranie komend z dwoma parametrami
//1. przyspieszanie do zadanej predkosci, ew. hamowanie jesli przekroczona
//2. hamowanie na zadanym odcinku do zadanej predkosci (ze stabilizacja przyspieszenia)
//3. wychodzenie z sytuacji awaryjnych: bezpiecznik nadmiarowy, poslizg
//4. przygotowanie pojazdu do drogi, zmiana kierunku ruchu
//5. dwa sposoby jazdy - manewrowy i pociagowy
//6. dwa zestawy psychiki: spokojny i agresywny
//7. przejscie na zestaw spokojny jesli wystepuje duzo poslizgow lub wybic nadmiarowego.
//8. lagodne ruszanie (przedluzony czas reakcji na 2 pierwszych nastawnikach)
//9. unikanie jazdy na oporach rozruchowych
//10. logowanie fizyki //Ra: nie przeniesione do C++
//11. kasowanie czuwaka/SHP
//12. procedury wspomagajace "patrzenie" na odlegle semafory
//13. ulepszone procedury sterowania
//14. zglaszanie problemow z dlugim staniem na sygnale S1
//15. sterowanie EN57
//16. zmiana kierunku //Ra: z przesiadk� po ukrotnieniu
//17. otwieranie/zamykanie drzwi
//18. Ra: odczepianie z zahamowaniem i podczepianie

//do zrobienia:
//1. kierownik pociagu
//2. madrzejsze unikanie grzania oporow rozruchowych i silnika
//3. unikanie szarpniec, zerwania pociagu itp
//4. obsluga innych awarii
//5. raportowanie problemow, usterek nie do rozwiazania
//6. dla Humandriver: tasma szybkosciomierza - zapis do pliku!
//7. samouczacy sie algorytm hamowania

//sta�e
const double EasyReactionTime=0.4;
const double HardReactionTime=0.2;
const double EasyAcceleration=0.5;
const double HardAcceleration=0.9;
const double PrepareTime     =2.0;
bool WriteLogFlag=false;

AnsiString StopReasonTable[]=
{//przyczyny zatrzymania ruchu AI
 "", //stopNone, //nie ma powodu - powinien jecha�
 "Off", //stopSleep, //nie zosta� odpalony, to nie pojedzie
 "Semaphore", //stopSem, //semafor zamkni�ty
 "Time", //stopTime, //czekanie na godzin� odjazdu
 "End of track", //stopEnd, //brak dalszej cz�ci toru
 "Change direction", //stopDir, //trzeba stan��, by zmieni� kierunek jazdy
 "Joining", //stopJoin, //zatrzymanie przy (p)odczepianiu
 "Block", //stopBlock, //przeszkoda na drodze ruchu
 "A command", //stopComm, //otrzymano tak� komend� (niewiadomego pochodzenia)
 "Out of station", //stopOut, //komenda wyjazdu poza stacj� (raczej nie powinna zatrzymywa�!)
 "Radiostop", //stopRadio, //komunikat przekazany radiem (Radiostop)
 "External", //stopExt, //przes�any z zewn�trz
 "Error", //stopError //z powodu b��du w obliczeniu drogi hamowania
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

void __fastcall TSpeedPos::Clear()
{
 iFlags=0; //brak flag to brak reakcji
 fVelNext=-1.0; //pr�dko�� bez ogranicze�
 fDist=0.0;
 vPos=vector3(0,0,0);
 tTrack=NULL; //brak wska�nika
};

void __fastcall TSpeedPos::Update(vector3 *p,vector3 *dir,double len)
{//przeliczenie odleg�o�ci od pojazdu w punkcie (*p), jad�cego w kierunku (*d)
 vector3 v=vPos-*p; //wektor do punktu zmiany
 //v.y=0.0; //liczymy odleg�o�� bez uwzgl�dnienia wysoko�ci
 fDist=v.Length(); //d�ugo�� wektora to odleg�o�� pomi�dzy czo�em a sygna�em albo pocz�tkiem toru
 //v.SafeNormalize(); //normalizacja w celu okre�lenia znaku (nie potrzebna?)
 double iska=dir->x*v.x+dir->z*v.z; //iloczyn skalarny to rzut na chwilow� prost� ruchu
 if (iska<0.0) //iloczyn skalarny jest ujemny, gdy punkt jest z ty�u
 {//je�li co� jest z ty�u, to dok�adna odleg�o�� nie ma ju� wi�kszego znaczenia
  fDist=-fDist; //potrzebne do badania wyjechania sk�adem poza ograniczenie
  if (iFlags&32) //32+64?
  {//je�li mini�ty (musi by� mini�ty r�wnie� przez ko�c�wk� sk�adu)

  }
  else
  {iFlags^=32; //32-mini�ty - b�dziemy liczy� odleg�o�� wzgl�dem przeciwnego ko�ca toru (nadal mo�e by� z przodu i ogdanicza�)
   if ((iFlags&67)==3) //tylko je�li (istotny) tor, bo eventy s� punktowe
    if (tTrack) //mo�e by� NULL, je�li koniec toru (????)
     vPos=(iFlags&4)?tTrack->CurrentSegment()->FastGetPoint_0():tTrack->CurrentSegment()->FastGetPoint_1(); //drugi koniec istotny
  }
 }
 else if (fDist<50.0)
 {//przy du�ym k�cie �uku iloczyn skalarny bardziej zani�y odleg�o�� ni� ci�ciwa
  fDist=iska; //ale przy ma�ych odleg�o�ciach rzut na chwilow� prost� ruchu da dok�adniejsze warto�ci
 }
 if (iFlags&2) //je�li tor
 {
  if (tTrack) //mo�e by� NULL, je�li koniec toru
  {if (iFlags&8) //je�li odcinek zmienny
    if (bool(tTrack->GetSwitchState()&1)!=bool(iFlags&16)) //czy stan si� zmieni�?
    {//Ra: zak�adam, �e s� tylko 2 mo�liwe stany
     iFlags^=16;
     fVelNext=tTrack->fVelocity; //nowa pr�dko��
     //jeszcze trzeba skanowanie wykona� od tego toru 
    }
  }
  WriteLog("-> Dist="+FloatToStrF(fDist,ffFixed,7,1)+", Track="+tTrack->NameGet()+", Vel="+AnsiString(fVelNext)+", Flags="+AnsiString(iFlags));
 }
 else if (iFlags&0x100) //je�li event
 {//odczyt kom�rki pami�ci najlepiej by by�o zrobi� jako notyfikacj�, czyli zmiana kom�rki wywo�a jak�� podan� funkcj�
  AnsiString command=eEvent->CommandGet();
  double value1=eEvent->ValueGet(1);
  double value2=eEvent->ValueGet(2);
  if (command=="ShuntVelocity")
  {//pr�dko�� manewrow� zapisa�, najwy�ej AI zignoruje przy analizie tabelki
   fVelNext=value1;
   iFlags|=0x200;
  }
  else if (command=="SetVelocity")
  {//w semaforze typu "m" jest ShuntVelocity dla jazdy manewrowej i SetVelocity dla S1
   fVelNext=value1;
   iFlags&=~0xE00; //nie manewrowa, nie przystanek, nie zatrzyma� na SBL
   if (value1==0.0)
    if (value2!=0.0)
    {//S1 na SBL mo�na przejecha� po zatrzymaniu
     fVelNext=value2;
     iFlags|=0x800; //na pewno nie zezwoli na manewry
    }
  }
  else if (command.SubString(1,19)=="PassengerStopPoint:")
  {//przystanek, najwy�ej AI zignoruje przy analizie tabelki
   //je�li to przystanek, to zignorowa� po up�ywie czasu, wcze�niej sprawdzi�, czy si� pojazd zatrzyma�...
   iFlags|=0x400;
  }
  //fVelNext=tTrack->fVelocity; //nowa pr�dko��
  WriteLog("-> Dist="+FloatToStrF(fDist,ffFixed,7,1)+", Event="+eEvent->asName+", Vel="+AnsiString(fVelNext)+", Flags="+AnsiString(iFlags));
 }
};

bool __fastcall TSpeedPos::Set(TEvent *e,double d)
{//zapami�tanie zdarzenia
 fDist=d;
 iFlags=0x101; //event+istotny
 eEvent=e;
 vPos=e->PositionGet(); //wsp�rz�dne eventu albo kom�rki pami�ci (zrzutowa� na tor?)
 return false; //true gdy zatrzymanie, wtedy nie ma po co skanowa� dalej
};

void __fastcall TSpeedPos::Set(TTrack *t,double d,int f)
{//zapami�tanie zmiany pr�dko�ci w torze
 fDist=d; //odleg�o�� do pocz�tku toru
 tTrack=t; //TODO: (t) mo�e by� NULL i nie odczytamy ko�ca poprzedniego :/
 if (tTrack)
 {iFlags=f|(tTrack->eType==tt_Normal?2:10); //zapami�tanie kierunku wraz z typem
  fVelNext=tTrack->fVelocity;
  if (tTrack->iDamageFlag&128) fVelNext=0.0; //je�li uszkodzony, to te� st�j
  if (iFlags&64) fVelNext=0.0; //je�li koniec, to te� st�j
  vPos=(bool(iFlags&4)!=bool(iFlags&64))?tTrack->CurrentSegment()->FastGetPoint_1():tTrack->CurrentSegment()->FastGetPoint_0();
 }
 //WriteLog("Scaned dist="+AnsiString(fDist)+", Track="+t->NameGet()+", Vel="+AnsiString(fVelNext)+", Flags="+AnsiString(iFlag));
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

__fastcall TSpeedTable::TSpeedTable()
{
 iFirst=iLast=-1;
 iDirection=0; //nieznany
 for (int i=0;i<16;++i) //czyszczenie tabeli pr�dko�ci
  sSpeedTable[i].Clear();
 tLast=NULL;
};

__fastcall TSpeedTable::~TSpeedTable()
{
};

bool __fastcall TSpeedTable::CheckEvent(TEvent *e,bool prox)
{//sprawdzanie eventu, czy jest obs�ugiwany poza kolejk�
 //prox=true, gdy sprawdzanie, czy doda� event do kolejki
 //-> zwraca true, je�li event istotny dla AI
 //prox=false, gdy wyszukiwanie semafora przez AI
 //-> zwraca false, gdy event ma by� dodany do kolejki
 if (e)
 {if (!prox) if (e==eSignSkip) return false; //semafor przejechany jest ignorowany
  AnsiString command;
  switch (e->Type)
  {//to si� wykonuje r�wnie� sk�adu jad�cego bez obs�ugi
   case tp_GetValues:
    command=e->CommandGet();
    if (prox?true:oMode!=Obey_train) //tylko w trybie manewrowym albo sprawdzanie ignorowania
     if (command=="ShuntVelocity")
      return true;
    break;
   case tp_PutValues:
    command=e->CommandGet();
    break;
   default:
    return false; //inne eventy si� nie licz�
  }
  if (prox)
   if (command=="SetProximityVelocity")
    return true; //ten event z toru ma by� ignorowany
  if (command=="SetVelocity")
   return true; //jak podamy wyjazd, to prze��czy si� w jazd� poci�gow�
  if (prox) //pomijanie punkt�w zatrzymania
  {if (command.SubString(1,19)=="PassengerStopPoint:")
    return true;
  }
  else //nazwa stacji pobrana z rozk�adu - nie ma co sprawdza� raz po raz
   if (!asNextStop.IsEmpty())
    if (command.SubString(1,asNextStop.Length())==asNextStop)
     return true;
 }
 return false;
};

TEvent* __fastcall TSpeedTable::CheckTrackEvent(double fDirection,TTrack *Track)
{//sprawdzanie event�w na podanym torze
 //ZiomalCl: teraz zwracany jest pierwszy event podajacy predkosc dla AI
 //a nie kazdy najblizszy event [AI sie gubilo gdy przed getval z SetVelocity
 //mialo np. PutValues z eventem od SHP]
 TEvent* e=(fDirection>0)?Track->Event2:Track->Event1;
 return CheckEvent(e,false)?e:NULL; //sprawdzenie z pomini�ciem niepotrzebnych
};

bool __fastcall TSpeedTable::AddNew()
{//zwi�kszenie u�ytej tabelki o jeden rekord
 iLast=(iLast+1)%16;
 //TODO: jeszcze sprawdzi�, czy si� na iFirst nie na�o�y
 return true; //false gdy si� na�o�y
};

void __fastcall TSpeedTable::TraceRoute(double fDistance,int iDir,TDynamicObject *pVehicle)
{//skanowanie trajektorii na odleg�o�� (fDistance) w kierunku (iDir) i uzupe�nianie tabelki
 if (!iDir)
 {//je�li kierunek jazdy nie jest okreslony
  iDirection=0; //czekamy na ustawienie kierunku
 }
 TTrack *pTrack; //zaczynamy od ostatniego analizowanego toru
 //double fDistChVel=-1; //odleg�o�� do toru ze zmian� pr�dko�ci
 double fLength; //d�ugo�� aktualnego toru (kr�tsza dla pierwszego)
 double fCurrentDistance; //aktualna przeskanowana d�ugo��
 TEvent *pEvent;
 if (iDirection!=iDir)
 {//je�li zmiana kierunku, zaczynamy od toru z pierwszym pojazdem
  pTrack=pVehicle->RaTrackGet();
  fLastDir=iDir*pVehicle->RaDirectionGet(); //ustalenie kierunku skanowania na torze
  fCurrentDistance=0; //na razie nic nie przeskanowano
  fLength=pVehicle->RaTranslationGet(); //pozycja na tym torze (odleg�o�� od Point1)
  if (fLastDir>0) //je�li w kierunku Point2 toru
   fLength=pTrack->Length()-fLength; //przeskanowana zostanie odleg�o�� do Point2
  fLastVel=pTrack->fVelocity; //aktualna pr�dko��
  iDirection=iDir; //ustalenie w jakim kierunku jest wype�niana tabelka wzgl�dem pojazdu
  iFirst=iLast=0;
  tLast=NULL; //�aden nie sprawdzony
 }
 else
 {//kontynuacja skanowania od ostatnio sprawdzonego toru (w ostatniej pozycji zawsze jest tor)
  pTrack=sSpeedTable[iLast].tTrack; //ostatnio sprawdzony tor
  if (!pTrack) return; //koniec toru, to nie ma co sprawdza�
  fLastDir=sSpeedTable[iLast].iFlags&4?-1.0:1.0; //flaga ustawiona, gdy Point2 toru jest bli�ej
  fCurrentDistance=sSpeedTable[iLast].fDist; //aktualna odleg�o�� do jego Point1
  fLength=sSpeedTable[iLast].iFlags&96?0.0:pTrack->Length(); //nie dolicza� d�ugo�ci gdy: 32-mini�ty pocz�tek, 64-jazda do ko�ca toru
 }
 if (fCurrentDistance<fDistance)
 {//je�li w og�le jest po co analizowa�
  //TTrack *pTrackFrom; //odcinek poprzedni, do znajdywania �lepego ko�ca
  --iLast; //jak co� si� znajdzie, zostanie wpisane w t� pozycj�, kt�r� odczytano
  while (fCurrentDistance<fDistance)
  {
   if (pTrack!=tLast)
   {//je�li tor nie by� jeszcze sprawdzany
    if (pTrack) //zawsze chyba jest
     if ((pTrack->fVelocity==0.0)||(pTrack->iDamageFlag&128)||(pTrack->eType!=tt_Normal)||(pTrack->fVelocity!=fLastVel))
     {//gdy zerowa pr�dko�� albo uszkadza pojazd albo ruchomy tor albo pr�dkos� w kolejnym r�na od aktualnej
      if (AddNew())
       sSpeedTable[iLast].Set(pTrack,fCurrentDistance,fLastDir<0?5:1); //dodanie zmiany pr�dko�ci
     }
    if ((pEvent=CheckTrackEvent(fLastDir,pTrack))!=NULL) //je�li jest semafor na tym torze
     if (AddNew())
     {sSpeedTable[iLast].Set(pEvent,fCurrentDistance); //dodanie odczytu sygna�u
      //je�li sygna� stop, to nie ma potrzeby dalej sprawdza�
      //fDistance=fCurrentDistance;
     }
   }
   fCurrentDistance+=fLength; //doliczenie kolejnego odcinka do przeskanowanej d�ugo�ci
   tLast=pTrack; //odhaczenie, �e sprawdzony
   //Track->ScannedFlag=true; //do pokazywania przeskanowanych tor�w
   fLastVel=pTrack->fVelocity; //pr�dko�� na poprzednio sprawdzonym odcinku
   //pTrackFrom=pTrack; //zapami�tanie aktualnego odcinka aby rozpozna� samozap�tlenie
   if (fLastDir>0)
   {//je�li szukanie od Point1 w kierunku Point2
    pTrack=pTrack->CurrentNext(); //mo�e by� NULL
    if (pTrack) //je�li dalej brakuje toru, to zostajemy na tym samym, z t� sam� orientacj�
     if (tLast->iNextDirection)
      fLastDir=-fLastDir; //mo�na by zami�ta� i zmieni� tylko je�li jest pTrack
   }
   else //if (fDirection<0)
   {//je�li szukanie od Point2 w kierunku Point1
    pTrack=pTrack->CurrentPrev(); //mo�e by� NULL
    if (pTrack) //je�li dalej brakuje toru, to zostajemy na tym samym, z t� sam� orientacj�
     if (!tLast->iPrevDirection)
      fLastDir=-fLastDir;
   }
   if (pTrack)
    fLength=pTrack->Length(); //zwi�kszenie skanowanej odleg�o�ci tylko je�li istnieje dalszy tor
   else
   {//definitywny koniec skanowania, chyba �e dalej puszczamy samoch�d po gruncie...
    if (AddNew()) //kolejny, bo si� cofn�li�my o 1
     sSpeedTable[iLast].Set(tLast,fCurrentDistance,fLastDir<0?69:65); //zapisanie ostatniego sprawdzonego toru
    return; //to ostatnia pozycja, bo NULL nic nie da, a mo�e si� podpi�� obrotnica, czy jakie� transportery
   }
   //TODO: sprawdzi� jeszcze, czy tabelka si� nie zatka
  }
  if (AddNew())
   sSpeedTable[iLast].Set(pTrack,fCurrentDistance,fLastDir<0?4:0); //zapisanie ostatniego sprawdzonego toru
 }
};

void __fastcall TSpeedTable::Check(double fDistance,int iDir,TDynamicObject *pVehicle)
{//przeliczenie danych w tabelce, ewentualnie doskanowanie
 if (iDirection!=iDir)
  TraceRoute(fDistance,iDir,pVehicle); //jak zmiana kierunku, to skanujemy
 else
 {//trzeba sprawdzi�, czy co� si� zmieni�o
  vector3 dir=pVehicle->VectorFront()*pVehicle->DirectionGet(); //wektor kierunku jazdy
  for (int i=iFirst;i!=iLast;i=(i+1)%16)
  {//aktualizacja rekord�w z wyj�tkiem ostatniego
   sSpeedTable[i].Update(&pVehicle->HeadPosition(),&dir,fLength);
   if (sSpeedTable[i].fDist<-fLength) //sk�ad wyjecha� ca�� d�ugo�ci� poza
   {//degradacja pozycji
    sSpeedTable[i].iFlags&=~1; //nie liczy si�
    iFirst=(iFirst+1)%16; //kolejne sprawdzanie b�dzie ju� od nast�pnej pozycji
   }
  }
  sSpeedTable[iLast].Update(&pVehicle->HeadPosition(),&dir,fLength); //aktualizacja ostatniego
  if (sSpeedTable[iLast].fDist<fDistance)
   TraceRoute(fDistance,iDir,pVehicle); //doskanowanie dalszego odcinka
 }
};

void __fastcall TSpeedTable::Update(double fVel,double &fVelDes,double &fDist,double &fNext,double &fAcc)
{//ustalenie parametr�w
 //fVel - chwilowa pr�dko�� pojazdu jako warto�� odniesienia
 //fDist - dystans w jakim nale�y rozwa�y� ruch
 //fNext - pr�dko�� na ko�cu tego dystansu
 //fAcc - zalecane przyspieszenie w chwili obecnej - kryterium wyboru dystansu
 double a; //przyspieszenie
 double v; //pr�dko��
 double d; //droga
 int i,k=iLast-iFirst+1;
 if (k<0) k+=16; //ilo�� pozycji do przeanalizowania
 for (i=iFirst;k>0;--k,i=(i+1)%16)
 {//sprawdzenie rekord�w od (iFirst) do (iLast), o ile s� istotne
  if (sSpeedTable[i].iFlags&1) //badanie istotno�ci
  {//o ile dana pozycja tabelki jest istotna
   v=sSpeedTable[i].fVelNext;
   if (v>=0.0)
   {//pozycje z pr�dko�ci� -1 mo�na spokojnie pomija�
    d=sSpeedTable[i].fDist;
    if (d>0.0) //sygna� lub ograniczenie z przodu
     a=(v*v-fVel*fVel)/(25.92*d); //przyspieszenie: ujemne, gdy trzeba hamowa�
    else
     if (sSpeedTable[i].iFlags&2) //je�li tor
     {//tor ogranicza pr�dko��, dop�ki ca�y sk�ad nie przejedzie,
      d=fLength+d; //zamiana na d�ugo�� liczon� do przodu
      if (d<0.0) continue; //zap�tlenie, je�li ju� wyjecha� za ten odcinek
      if (v<fVelDes) fVelDes=v; //ograniczenie aktualnej pr�dko�ci a� do wyjechania za ograniczenie
      //if (v==0.0) fAcc=-0.9; //hamowanie je�li stop
      continue; //i tyle wystarczy
     }
     else //event trzyma tylko je�li VelNext=0, nawet po przejechaniu (nie powinno dotyczy� samochod�w?)
      a=(v==0.0?-1.0:fAcc); //ruszanie albo hamowanie
    if (a<fAcc)
    {//mniejsze przyspieszenie to mniejsza mo�liwo�� rozp�dzenia si� albo konieczno�� hamowania
     //je�li droga wolna, to mo�e by� a>1.0 i si� tu nie za�apuje
     fAcc=a; //zalecane przyspieszenie (nie musi by� uwzgl�dniane przez AI)
     fNext=v; //istotna jest pr�dko�� na ko�cu tego odcinka
     fDist=d; //dlugo�� odcinka
    }
    else if ((fAcc>0)&&(v>0)&&(v<=fNext))
    {//je�li nie ma wskaza� do hamowania, mo�na poda� drog� i pr�dko�� na jej ko�cu
     fNext=v; //istotna jest pr�dko�� na ko�cu tego odcinka
     fDist=d; //dlugo�� odcinka (kolejne pozycje mog� wyd�u�a� drog�, je�li pr�dko�� jest sta�a)
    }
   } //if (v>=0.0)
  } //if (sSpeedTable[i].iFlags&1)
 } //for
 //if (fAcc>-0.2)
 // if (fAcc<0.2)
 //  fAcc=0.0; //nie bawimy si� w jakie� delikatne hamowania czy rozp�dzania
};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

__fastcall TController::TController
(bool AI,
 TDynamicObject *NewControll,
 Mtable::TTrainParameters *NewTrainParams,
 bool InitPsyche
)
{
 EngineActive=false;
 LastUpdatedTime=0.0;
 ElapsedTime=0.0;
 //inicjalizacja zmiennych
 Psyche=InitPsyche;
 AccDesired=AccPreferred;
 VelDesired=0.0;
 VelforDriver=-1;
 LastReactionTime=0.0;
 HelpMeFlag=false;
 fProximityDist=1;
 ActualProximityDist=1;
 vCommandLocation.x=0;
 vCommandLocation.y=0;
 vCommandLocation.z=0;
 VelActual=0.0; //normalnie na pocz�tku ma sta�, no chyba �e jedzie
 VelNext=120.0;
 AIControllFlag=AI;
 pVehicle=NewControll;
 Controlling=pVehicle->MoverParameters; //skr�t do obiektu parametr�w
 pVehicles[0]=pVehicle->GetFirstDynamic(0); //pierwszy w kierunku jazdy (Np. Pc1)
 pVehicles[1]=pVehicle->GetFirstDynamic(1); //ostatni w kierunku jazdy (ko�c�wki)
/*
 switch (Controlling->CabNo)
 {
  case -1: SendCtrlBroadcast("CabActivisation",1); break;
  case  1: SendCtrlBroadcast("CabActivisation",2); break;
  default: AIControllFlag:=False; //na wszelki wypadek
 }
*/
 iDirection=0;
 iDirectionOrder=Controlling->CabNo; //1=do przodu (w kierunku sprz�gu 0)
 VehicleName=Controlling->Name;
 TrainParams=NewTrainParams;
 if (TrainParams)
  asNextStop=TrainParams->NextStop();
 else
  TrainParams=new TTrainParameters("none"); //rozk�ad jazdy
 //OrderCommand="";
 //OrderValue=0;
 OrdersClear();
 MaxVelFlag=false; MinVelFlag=false; //Ra: to nie jest u�ywane
 iDriverFailCount=0;
 Need_TryAgain=false;
 Need_BrakeRelease=true;
 deltalog=1.0;
/*
 if (WriteLogFlag)
 {
  assignfile(LogFile,VehicleName+".dat");
  rewrite(LogFile);
   writeln(LogFile,' Time [s]   Velocity [m/s]  Acceleration [m/ss]   Coupler.Dist[m]  Coupler.Force[N]  TractionForce [kN]  FrictionForce [kN]   BrakeForce [kN]    BrakePress [MPa]   PipePress [MPa]   MotorCurrent [A]    MCP SCP BCP LBP DmgFlag Command CVal1 CVal2');
 }
  if (WriteLogFlag)
  {
   assignfile(AILogFile,VehicleName+".txt");
   rewrite(AILogFile);
   writeln(AILogFile,"AI driver log: started OK");
   close(AILogFile);
  }
*/
 ScanMe=False;
 VelMargin=Controlling->Vmax*0.005;
 fWarningDuration=0.0; //nic do wytr�bienia
 WaitingExpireTime=31.0; //tyle ma czeka�, zanim si� ruszy
 WaitingTime=0.0;
 fMinProximityDist=30.0; //stawanie mi�dzy 30 a 60 m przed przeszkod�
 fMaxProximityDist=50.0;
 iVehicleCount=-2; //warto�� neutralna
 Prepare2press=false; //bez dociskania
 eStopReason=stopSleep; //na pocz�tku �pi
 fLength=0.0;
 eSignSkip=eSignLast=NULL; //mini�ty semafor
 fShuntVelocity=40; //domy�lna pr�dko�� manewrowa
 fStopTime=0.0; //czas postoju przed dalsz� jazd� (np. na przystanku)
 iDrivigFlags=moveStopPoint; //flagi bitowe ruchu
 Ready=false;
 SetDriverPsyche(); //na ko�cu, bo wymaga ustawienia zmiennych
 if (Controlling->CategoryFlag&2)
 {//samochody
  fDriverMass=0.02; //mno�one przez v^2 [km/h] daje drog� skanowania [m]
  fDriverDist=5.0; //10m - zachowywany odst�p przed kolizj�
 }
 else
 {//poci�gi i statki
  fDriverMass=0.1; //mno�one przez v^2 [km/h] daje drog� skanowania [m]
  fDriverDist=50.0; //50m - zachowywany odst�p przed kolizj�
 }
 fVelMax=-1; //ustalenie pr�dko�ci dla sk�adu
};

void __fastcall TController::CloseLog()
{
/*
 if (WriteLogFlag)
 {
  CloseFile(LogFile);
  //if WriteLogFlag)
  // CloseFile(AILogFile);
  append(AIlogFile);
  writeln(AILogFile,ElapsedTime5:2,": QUIT");
  close(AILogFile);
 }
*/
};

__fastcall TController::~TController()
{//wykopanie mechanika z roboty
 CloseLog();
};

AnsiString __fastcall TController::Order2Str(TOrders Order)
{//zamiana kodu rozkazu na opis
 if (Order==Wait_for_orders)     return "Wait_for_orders";
 if (Order==Prepare_engine)      return "Prepare_engine";
 if (Order==Shunt)               return "Shunt";
 if (Order==Connect)             return "Connect";
 if (Order==Disconnect)          return "Disconnect";
 if (Order==Change_direction)    return "Change_direction";
 if (Order==Obey_train)          return "Obey_train";
 if (Order==Release_engine)      return "Release_engine";
 if (Order==Jump_to_first_order) return "Jump_to_first_order";
/* Ra: wersja ze switch nie dzia�a prawid�owo (czemu?)
 switch (Order)
 {
  Wait_for_orders:     return "Wait_for_orders";
  Prepare_engine:      return "Prepare_engine";
  Shunt:               return "Shunt";
  Change_direction:    return "Change_direction";
  Obey_train:          return "Obey_train";
  Release_engine:      return "Release_engine";
  Jump_to_first_order: return "Jump_to_first_order";
 }
*/
 return "Undefined!";
}

AnsiString __fastcall TController::OrderCurrent()
{//pobranie aktualnego rozkazu celem wy�wietlenia
 return AnsiString(OrderPos)+". "+Order2Str(OrderList[OrderPos]);
};

void __fastcall TController::OrdersClear()
{//czyszczenie tabeli rozkaz�w na starcie albo po doj�ciu do ko�ca
 OrderPos=0;
 OrderTop=1; //szczyt stosu rozkaz�w
 for (int b=0;b<maxorders;b++)
  OrderList[b]=Wait_for_orders;
};

void __fastcall TController::Activation()
{//umieszczenie obsady w odpowiednim cz�onie
 iDirection=iDirectionOrder; //kierunek w�a�nie zosta� ustalony (zmieniony)
 if (iDirection)
 {//je�li jest ustalony kierunek
  TDynamicObject *d=pVehicle; //w tym siedzi AI
  int brake=Controlling->LocalBrake;
  if (TestFlag(d->MoverParameters->Couplers[iDirectionOrder*d->DirectionGet()<0?1:0].CouplingFlag,ctrain_controll))
  {Controlling->MainSwitch(false); //dezaktywacja czuwaka, je�li przej�cie do innego cz�onu
   Controlling->DecLocalBrakeLevel(10); //zwolnienie hamulca w opuszczanym poje�dzie
  }
  Controlling->CabDeactivisation(); //tak jest w Train.cpp
  //przej�cie AI na drug� stron� EN57, ET41 itp.
  while (TestFlag(d->MoverParameters->Couplers[iDirectionOrder*d->DirectionGet()<0?1:0].CouplingFlag,ctrain_controll))
  {//je�li pojazd z przodu jest ukrotniony, to przechodzimy do niego
   d=iDirectionOrder<0?d->Next():d->Prev(); //przechodzimy do nast�pnego cz�onu
   if (d?!d->Mechanik:false)
   {d->Mechanik=this; //na razie bilokacja
    if (d->DirectionGet()!=pVehicle->DirectionGet()) //je�li s� przeciwne do siebie
     iDirection=-iDirection; //to b�dziemy jecha� w drug� stron� wzgl�dem zasiedzianego pojazdu
    pVehicle->Mechanik=NULL; //tam ju� nikogo nie ma
    pVehicle->MoverParameters->CabNo=0; //wy��czanie kabin po drodze
    //pVehicle->MoverParameters->ActiveCab=0;
    //pVehicle->MoverParameters->DirAbsolute=pVehicle->MoverParameters->ActiveDir*pVehicle->MoverParameters->CabNo;
    pVehicle=d; //a mechu ma nowy pojazd (no, cz�on)
   }
   else break; //jak zaj�te, albo koniec sk�adu, to mechanik dalej nie idzie (wywali� drugiego?)
  }
  Controlling=pVehicle->MoverParameters; //skr�t do obiektu parametr�w, mo�e by� nowy
  //Ra: to prze��czanie poni�ej jest tu bez sensu
  Controlling->ActiveCab=iDirection; //aktywacja kabiny w prowadzonym poje�dzie
  //Controlling->CabNo=iDirection;
  //Controlling->ActiveDir=0; //�eby sam ustawi� kierunek
  Controlling->CabActivisation(); //uruchomienie kabin w cz�onach
  if (AIControllFlag) //je�li prowadzi komputer
   if (brake) //hamowanie tylko je�li by� wcze�niej zahamowany (bo mo�liwe, �e jedzie!)
    Controlling->IncLocalBrakeLevel(brake); //zahamuj jak wcze�niej
 }
};

bool __fastcall TController::CheckVehicles()
{//sprawdzenie stanu posiadanych pojazd�w w sk�adzie i zapalenie �wiate�
 //ZiomalCl: sprawdzanie i zmiana SKP w skladzie prowadzonym przez AI
 TDynamicObject* p; //roboczy wska�nik na pojazd
 iVehicles=0; //ilo�� pojazd�w w sk�adzie
 int d=iDirection>0?0:1; //kierunek szukania czo�a (numer sprz�gu)
 pVehicles[0]=p=pVehicle->FirstFind(d); //pojazd na czele sk�adu
 //liczenie pojazd�w w sk�adzie i ustawianie kierunku
 d=1-d; //a dalej b�dziemy zlicza� od czo�a do ty�u
 fLength=0.0; //d�ugo�� sk�adu do badania wyjechania za ograniczenie
 fVelMax=-1; //ustalenie pr�dko�ci dla sk�adu
/*
 bool main=true; //czy jest g��wnym steruj�cym
 int dir=????; //od pierwszego w drug� stron�
 while (p)
 {//sprawdzanie, czy jest g��wnym steruj�cym, �eby nie by�o konfliktu
  //kierunek pojazd�w w sk�adzie jest ustalany tylko dla gl�wnego steruj�cego
  if (p->Mechanik) //je�li ma obsad�
   if (p!=this) //ale chodzi o inny pojazd, ni� aktualnie sprawdzaj�cy
    if (p->Mechanik->iDrivigFlags&movePrimary) //a tamten ma priorytet
     main=false;
  p=p->Neighbour(dir); //pojazd pod��czony od wskazanej strony
 }
 p=pVehicle->FirstFind(d);
*/
 while (p)
 {
  if (TrainParams)
   if (p->asDestination.IsEmpty())
    p->asDestination=TrainParams->Relation2; //relacja docelowa, je�li nie by�o
  if (AIControllFlag) //je�li prowadzi komputer
   p->RaLightsSet(0,0); //gasimy �wiat�a
  ++iVehicles; //jest jeden pojazd wi�cej
  pVehicles[1]=p; //zapami�tanie ostatniego
  fLength+=p->MoverParameters->Dim.L; //dodanie d�ugo�ci pojazdu
  d=p->DirectionSet(d?1:-1); //zwraca po�o�enie nast�pnego (1=zgodny,0=odwr�cony)
  //1=zgodny: sprz�g 0 od czo�a; 0=odwr�cony: sprz�g 1 od czo�a
  if (fVelMax<0?true:p->MoverParameters->Vmax<fVelMax)
   fVelMax=p->MoverParameters->Vmax; //ustalenie maksymalnej pr�dko�ci dla sk�adu
  p=p->Next(); //pojazd pod��czony od ty�u (licz�c od czo�a)
 }
 sSpeedTable.fLength=fLength; //zapami�tanie d�ugo�ci sk�adu w celu wykrycia wyjechania za ograniczenie
/* //tabelka z list� pojazd�w jest na razie nie potrzebna
 if (i!=)
 {delete[] pVehicle
 }
*/
 if (AIControllFlag) //je�li prowadzi komputer
  if (OrderCurrentGet()==Obey_train) //je�li jazda poci�gowa
   Lights(1+4+16,2+32+64); //�wiat�a poci�gowe (Pc1) i ko�c�wki (Pc5)
  else if (OrderCurrentGet()&(Shunt|Connect))
   Lights(16,(pVehicles[1]->MoverParameters->ActiveCab)?1:0); //�wiat�a manewrowe (Tb1) na poje�dzie z nap�dem
  else if (OrderCurrentGet()==Disconnect)
   Lights(16,0); //�wiat�a manewrowe (Tb1) tylko z przodu, aby nie pozostawi� sk�adu ze �wiat�em
 return true;
}

void __fastcall TController::Lights(int head,int rear)
{//zapalenie �wiate� w sk��dzie
 pVehicles[0]->RaLightsSet(head,-1); //zapalenie przednich w pierwszym
 pVehicles[1]->RaLightsSet(-1,rear); //zapalenie ko�c�wek w ostatnim
}

int __fastcall TController::OrderDirectionChange(int newdir,TMoverParameters *Vehicle)
{//zmiana kierunku jazdy, niezale�nie od kabiny
 int testd;
 testd=newdir;
 if (Vehicle->Vel<0.1)
 {
  switch (newdir*Vehicle->CabNo)
  {//DirectionBackward() i DirectionForward() to zmiany wzgl�dem kabiny
   case -1: if (!Vehicle->DirectionBackward()) testd=0; break;
   case  1: if (!Vehicle->DirectionForward()) testd=0; break;
  }
  if (testd==0)
   VelforDriver=-1;
 }
 else
  VelforDriver=0;
 if ((Vehicle->ActiveDir==0)&&(VelforDriver<Vehicle->Vel))
  IncBrake();
 if (Vehicle->ActiveDir==testd*Vehicle->CabNo)
  VelforDriver=-1;
 if ((Vehicle->ActiveDir>0)&&(Vehicle->TrainType==dt_EZT))
  //if () //tylko je�li jazda poci�gowa
  Vehicle->DirectionForward(); //Ra: z przekazaniem do silnikowego
 return (int)VelforDriver;
}

void __fastcall TController::WaitingSet(double Seconds)
{//ustawienie odczekania po zatrzymaniu (ustawienie w trakcie jazdy zatrzyma)
 fStopTime=-Seconds; //ujemna warto�� oznacza oczekiwanie (potem >=0.0)
}

void __fastcall TController::SetVelocity(double NewVel,double NewVelNext,TStopReason r)
{//ustawienie nowej pr�dko�ci
 WaitingTime=-WaitingExpireTime; //no albo przypisujemy -WaitingExpireTime, albo por�wnujemy z WaitingExpireTime
 MaxVelFlag=False; //Ra: to nie jest u�ywane
 MinVelFlag=False; //Ra: to nie jest u�ywane
 VelActual=NewVel;   //pr�dko�� zezwolona na aktualnym odcinku
 VelNext=NewVelNext; //pr�dko�� przy nast�pnym obiekcie
 if ((NewVel>NewVelNext) //je�li oczekiwana wi�ksza ni� nast�pna
  || (NewVel<Controlling->Vel)) //albo aktualna jest mniejsza ni� aktualna
  fProximityDist=-800.0; //droga hamowania do zmiany pr�dko�ci
 else
  fProximityDist=-300.0; //Ra: ujemne warto�ci s� ignorowane
 if (NewVel==0.0) //je�li ma stan��
 {if (r!=stopNone) //a jest pow�d podany
  eStopReason=r; //to zapami�ta� nowy pow�d
 }
 else
  eStopReason=stopNone; //podana pr�dko��, to nie ma powod�w do stania
}

bool __fastcall TController::SetProximityVelocity(double NewDist,double NewVelNext)
{//informacja o pr�dko�ci w pewnej odleg�o�ci
#if !TESTTABLE
 if (NewVelNext==0.0)
  WaitingTime=0.0; //nie trzeba ju� czeka�
 //if ((NewVelNext>=0)&&((VelNext>=0)&&(NewVelNext<VelNext))||(NewVelNext<VelActual))||(VelNext<0))
 {MaxVelFlag=False; //Ra: to nie jest u�ywane
  MinVelFlag=False; //Ra: to nie jest u�ywane
  VelNext=NewVelNext;
  fProximityDist=NewDist; //dodatnie: przeliczy� do punktu; ujemne: wzi�� dos�ownie
  return true;
 }
 //else return false
#endif
}

void __fastcall TController::SetDriverPsyche()
{
 double maxdist=0.5; //skalowanie dystansu od innego pojazdu, zmienic to!!!
 if ((Psyche==Aggressive)&&(OrderList[OrderPos]==Obey_train))
 {
  ReactionTime=HardReactionTime; //w zaleznosci od charakteru maszynisty
  AccPreferred=HardAcceleration; //agresywny
  //if (Controlling)
  if (Controlling->CategoryFlag&2)
   WaitingExpireTime=1; //tyle ma czeka� samoch�d, zanim si� ruszy
  else
   WaitingExpireTime=61; //tyle ma czeka�, zanim si� ruszy
 }
 else
 {
  ReactionTime=EasyReactionTime; //spokojny
  AccPreferred=EasyAcceleration;
  if (Controlling->CategoryFlag&2)
   WaitingExpireTime=3; //tyle ma czeka� samoch�d, zanim si� ruszy
  else
   WaitingExpireTime=65; //tyle ma czeka�, zanim si� ruszy
 }
 if (Controlling)
 {//with Controlling do
  if (Controlling->MainCtrlPos<3)
   ReactionTime=Controlling->InitialCtrlDelay+ReactionTime;
  if (Controlling->BrakeCtrlPos>1)
   ReactionTime=0.5*ReactionTime;
  if (Controlling->Vel>0.1) //o ile jedziemy
   if (pVehicles[0]->MoverParameters->Couplers[pVehicles[0]->MoverParameters->DirAbsolute>0?0:1].Connected) //a mamy co� z przodu
    if (Controlling->Couplers[0].CouplingFlag==0) //je�li to co� jest pod��czone sprz�giem wirtualnym
    {//wyliczanie optymalnego przyspieszenia do jazdy na widoczno�� (Ra: na pewno tutaj?)
     double k=pVehicles[0]->MoverParameters->Couplers[pVehicles[0]->MoverParameters->DirAbsolute>0?0:1].Connected->Vel; //pr�dko�� pojazdu z przodu
     if (k<Controlling->Vel) //por�wnanie modu��w pr�dko�ci [km/h]
     {if (pVehicles[0]->fTrackBlock<fDriverDist)
       k=-AccPreferred; //hamowanie
      else
      {//je�li tamten jedzie szybciej, to nie potrzeba modyfikowa� przyspieszenia
       double d=25.92*(pVehicles[0]->fTrackBlock-maxdist*fabs(Controlling->Vel)-fDriverDist); //bezpieczna odleg�o�� za poprzednim
       //a=(v2*v2-v1*v1)/(25.92*(d-0.5*v1))
       //(v2*v2-v1*v1)/2 to r�nica energii kinetycznych na jednostk� masy
       //je�li v2=50km/h,v1=60km/h,d=200m => k=(192.9-277.8)/(25.92*(200-0.5*16.7)=-0.0171 [m/s^2]
       //je�li v2=50km/h,v1=60km/h,d=100m => k=(192.9-277.8)/(25.92*(100-0.5*16.7)=-0.0357 [m/s^2]
       //je�li v2=50km/h,v1=60km/h,d=50m  => k=(192.9-277.8)/(25.92*( 50-0.5*16.7)=-0.0786 [m/s^2]
       //je�li v2=50km/h,v1=60km/h,d=25m  => k=(192.9-277.8)/(25.92*( 25-0.5*16.7)=-0.1967 [m/s^2]
       if (d>0) //bo jak ujemne, to zacznie przyspiesza�, aby si� zderzy�
        k=(k*k-Controlling->Vel*Controlling->Vel)/d; //energia kinetyczna dzielona przez mas� i drog� daje przyspieszenie
       else
        k=-AccPreferred; //hamowanie
       //WriteLog(pVehicle->asName+" "+AnsiString(k));
      }
      AccPreferred=Min0R(k,AccPreferred);
     }
    }
 }
};

bool __fastcall TController::PrepareEngine()
{//odpalanie silnika
 bool OK;
 bool voltfront,voltrear;
 voltfront=false;
 voltrear=false;
 LastReactionTime=0.0;
 ReactionTime=PrepareTime;
 //with Controlling do
 if (((Controlling->EnginePowerSource.SourceType==CurrentCollector)||(Controlling->TrainType==dt_EZT)))
 {
  if (Controlling->GetTrainsetVoltage())
  {
   voltfront=true;
   voltrear=true;
  }
 }
//   begin
//     if Couplers[0].Connected<>nil)
//     begin
//       if Couplers[0].Connected^.PantFrontVolt or Couplers[0].Connected^.PantRearVolt)
//         voltfront:=true
//       else
//         voltfront:=false;
//     end
//     else
//        voltfront:=false;
//     if Couplers[1].Connected<>nil)
//     begin
//      if Couplers[1].Connected^.PantFrontVolt or Couplers[1].Connected^.PantRearVolt)
//        voltrear:=true
//      else
//        voltrear:=false;
//     end
//     else
//        voltrear:=false;
//   end
 else
  //if EnginePowerSource.SourceType<>CurrentCollector)
  if (Controlling->TrainType!=dt_EZT)
   voltfront=true;
 if (Controlling->EnginePowerSource.SourceType==CurrentCollector)
 {
  Controlling->PantFront(true);
  Controlling->PantRear(true);
 }
 if (Controlling->TrainType==dt_EZT)
 {
  Controlling->PantFront(true);
  Controlling->PantRear(true);
 }
 //Ra: rozdziawianie drzwi po odpaleniu nie jest potrzebne
 //if (Controlling->DoorOpenCtrl==1)
 // if (!Controlling->DoorRightOpened)
 //  Controlling->DoorRight(true);  //McZapkie: taka prowizorka bo powinien wiedziec gdzie peron
//voltfront:=true;
 if (Controlling->PantFrontVolt||Controlling->PantRearVolt||voltfront||voltrear)
 {//najpierw ustalamy kierunek, je�li nie zosta� ustalony
  if (!iDirection) //je�li nie ma ustalonego kierunku
   if (Controlling->V==0)
   {//ustalenie kierunku, gdy stoi
    iDirection=Controlling->CabNo; //wg wybranej kabiny
    if (!iDirection) //je�li nie ma ustalonego kierunku
     if (Controlling->PantFrontVolt||Controlling->PantRearVolt||voltfront||voltrear)
     {if (Controlling->Couplers[1].CouplingFlag==ctrain_virtual) //je�li z ty�u nie ma nic
       iDirection=-1; //jazda w kierunku sprz�gu 1
      if (Controlling->Couplers[0].CouplingFlag==ctrain_virtual) //je�li z przodu nie ma nic
       iDirection=1; //jazda w kierunku sprz�gu 0
     }
   }
   else //ustalenie kierunku, gdy jedzie
    if (Controlling->PantFrontVolt||Controlling->PantRearVolt||voltfront||voltrear)
     if (Controlling->V<0) //jedzie do ty�u
      iDirection=-1; //jazda w kierunku sprz�gu 1
     else //jak nie do ty�u, to do przodu
      iDirection=1; //jazda w kierunku sprz�gu 0
  if (!Controlling->Mains)
  {
   //if TrainType=dt_SN61)
   //   begin
   //      OK:=(OrderDirectionChange(ChangeDir,Controlling)=-1);
   //      OK:=IncMainCtrl(1);
   //   end;
   OK=Controlling->MainSwitch(true);
  }
  else
  {
   OK=(OrderDirectionChange(iDirection,Controlling)==-1);
   Controlling->CompressorSwitch(true);
   Controlling->ConverterSwitch(true);
   Controlling->CompressorSwitch(true);
  }
 }
 else
  OK=false;
 OK=OK&&(Controlling->ActiveDir!=0)&&(Controlling->CompressorAllow);
 if (OK)
 {
  if (eStopReason==stopSleep) //je�li dotychczas spa�
   eStopReason==stopNone; //teraz nie ma powodu do stania
  if (Controlling->Vel>=1.0) //je�li jedzie
   iDrivigFlags&=~moveAvaken; //pojazd przemie�ci� si� od w��czenia
  EngineActive=true;
  return true;
 }
 else
 {
  EngineActive=false;
  return false;
 }
};

bool __fastcall TController::ReleaseEngine()
{//wy��czanie silnika
 bool OK=false;
 LastReactionTime=0.0;
 ReactionTime=PrepareTime;
 if (Controlling->DoorOpenCtrl==1)
 {//zamykanie drzwi
  if (Controlling->DoorLeftOpened) Controlling->DoorLeft(false);
  if (Controlling->DoorRightOpened) Controlling->DoorRight(false);
 }
 if (Controlling->ActiveDir==0)
  if (Controlling->Mains)
  {
   Controlling->CompressorSwitch(false);
   Controlling->ConverterSwitch(false);
   if (Controlling->EnginePowerSource.SourceType==CurrentCollector)
   {
    Controlling->PantFront(false);
    Controlling->PantRear(false);
   }
   OK=Controlling->MainSwitch(false);
  }
  else
   OK=true;
 if (!Controlling->DecBrakeLevel())
  if (!Controlling->IncLocalBrakeLevel(1))
  {
   if (Controlling->ActiveDir==1)
    if (!Controlling->DecScndCtrl(2))
     if (!Controlling->DecMainCtrl(2))
      Controlling->DirectionBackward();
   if (Controlling->ActiveDir==-1)
    if (!Controlling->DecScndCtrl(2))
     if (!Controlling->DecMainCtrl(2))
      Controlling->DirectionForward();
  }
 OK=OK&&(Controlling->Vel<0.01);
 if (OK)
 {//je�li si� zatrzyma�
  EngineActive=false;
  iDrivigFlags|=moveAvaken; //po w��czeniu silnika pojazd nie przemie�ci� si�
  eStopReason=stopSleep; //stoimy z powodu wy��czenia
  eSignSkip=NULL; //zapominamy sygna� do pomini�cia
  eSignLast=NULL; //zapominamy ostatni sygna�
  Lights(0,0); //gasimy �wiat�a
  OrderNext(Wait_for_orders); //�eby nie pr�bowa� co� robi� dalej
 }
 return OK;
}

bool __fastcall TController::IncBrake()
{//zwi�kszenie hamowania
 bool OK=false;
 switch (Controlling->BrakeSystem)
 {
  case Individual:
   OK=Controlling->IncLocalBrakeLevel(1+floor(0.5+fabs(AccDesired)));
   break;
  case Pneumatic:
   if ((Controlling->Couplers[0].Connected==NULL)&&(Controlling->Couplers[1].Connected==NULL))
    OK=Controlling->IncLocalBrakeLevel(1+floor(0.5+fabs(AccDesired))); //hamowanie lokalnym bo luzem jedzie
   else
   {if (Controlling->BrakeCtrlPos+1==Controlling->BrakeCtrlPosNo)
    {
     if (AccDesired<-1.5) //hamowanie nagle
      OK=Controlling->IncBrakeLevel();
     else
      OK=false;
    }
    else
    {
/*
    if (AccDesired>-0.2) and ((Vel<20) or (Vel-VelNext<10)))
            begin
              if BrakeCtrlPos>0)
               OK:=IncBrakeLevel
              else;
               OK:=IncLocalBrakeLevel(1);   //finezyjne hamowanie lokalnym
             end
           else
*/
     OK=Controlling->IncBrakeLevel();
    }
   }
   break;
  case ElectroPneumatic:
   if (Controlling->BrakeCtrlPos<Controlling->BrakeCtrlPosNo)
    if (Controlling->BrakePressureTable[Controlling->BrakeCtrlPos+1+2].BrakeType==ElectroPneumatic) //+2 to indeks Pascala 
     OK=Controlling->IncBrakeLevel();
    else
     OK=false;
  }
 return OK;
}

bool __fastcall TController::DecBrake()
{//zmniejszenie si�y hamowania
 bool OK=false;
 switch (Controlling->BrakeSystem)
 {
  case Individual:
   OK=Controlling->DecLocalBrakeLevel(1+floor(0.5+fabs(AccDesired)));
   break;
  case Pneumatic:
   if (Controlling->BrakeCtrlPos>0)
    OK=Controlling->DecBrakeLevel();
   if (!OK)
    OK=Controlling->DecLocalBrakeLevel(2);
   Need_BrakeRelease=true;
   break;
  case ElectroPneumatic:
   if (Controlling->BrakeCtrlPos>-1)
    if (Controlling->BrakePressureTable[Controlling->BrakeCtrlPos-1+2].BrakeType==ElectroPneumatic) //+2 to indeks Pascala
     OK=Controlling->DecBrakeLevel();
    else
    {if ((Controlling->BrakeSubsystem==Knorr)||(Controlling->BrakeSubsystem==Hik)||(Controlling->BrakeSubsystem==Kk))
     {//je�li Knorr
      Controlling->SwitchEPBrake((Controlling->BrakePress>0.0)?1:0);
     }
     OK=false;
    }
    if (!OK)
     OK=Controlling->DecLocalBrakeLevel(2);
   break;
 }
 return OK;
};

bool __fastcall TController::IncSpeed()
{//zwi�kszenie pr�dko�ci
 bool OK=true;
 if ((Controlling->DoorOpenCtrl==1)&&(Controlling->Vel==0.0)&&(Controlling->DoorLeftOpened||Controlling->DoorRightOpened))  //AI zamyka drzwi przed odjazdem
 {
  if (Controlling->DoorClosureWarning)
   Controlling->DepartureSignal=true; //za��cenie bzyczka
  Controlling->DoorLeft(false); //zamykanie drzwi
  Controlling->DoorRight(false);
  //Ra: trzeba by ustawi� jaki� czas oczekiwania na zamkni�cie si� drzwi
  WaitingSet(1); //czekanie sekund�, mo�e troch� d�u�ej
 }
 else
 {
  Controlling->DepartureSignal=false;
  switch (Controlling->EngineType)
  {
   case None:
    if (Controlling->MainCtrlPosNo>0) //McZapkie-041003: wagon sterowniczy
    {
     //TODO: sprawdzanie innego czlonu //if (!FuseFlagCheck())
     if (Controlling->BrakePress<0.05*Controlling->MaxBrakePress)
     {
      if (Controlling->ActiveDir>0) Controlling->DirectionForward(); //zeby EN57 jechaly na drugiej nastawie
      OK=Controlling->IncMainCtrl(1);
     }
    }
    break;
   case ElectricSeriesMotor :
    if (!Controlling->FuseFlag)
     if ((Controlling->Im<=Controlling->Imin)&&(Ready||Prepare2press))
     {
      OK=Controlling->IncMainCtrl(1);
      if ((Controlling->MainCtrlPos>2)&&(Controlling->Im==0))
        Need_TryAgain=true;
       else
        if (!OK)
         OK=Controlling->IncScndCtrl(1); //TODO: dorobic boczniki na szeregowej przy ciezkich bruttach
     }
    break;
   case Dumb:
   case DieselElectric:
    if (Ready||Prepare2press) //{(BrakePress<=0.01*MaxBrakePress)}
    {
     OK=Controlling->IncMainCtrl(1);
     if (!OK)
      OK=Controlling->IncScndCtrl(1);
    }
    break;
   case WheelsDriven:
    if (sin(Controlling->eAngle)>0)
     Controlling->IncMainCtrl(1+floor(0.5+fabs(AccDesired)));
    else
     Controlling->DecMainCtrl(1+floor(0.5+fabs(AccDesired)));
    break;
   case DieselEngine :
    if ((Controlling->Vel>Controlling->dizel_minVelfullengage)&&(Controlling->RList[Controlling->MainCtrlPos].Mn>0))
     OK=Controlling->IncMainCtrl(1);
    if (Controlling->RList[Controlling->MainCtrlPos].Mn==0)
     OK=Controlling->IncMainCtrl(1);
    if (!Controlling->Mains)
    {
     Controlling->MainSwitch(true);
     Controlling->ConverterSwitch(true);
     Controlling->CompressorSwitch(true);
    }
    break;
  }
 }
 if (Controlling->Vel>=1.0)
  iDrivigFlags&=~moveAvaken; //pojazd przemie�ci� si� od w��czenia
 return OK;
}

bool __fastcall TController::DecSpeed()
{//zmniejszenie pr�dko�ci (ale nie hamowanie)
 bool OK=false; //domy�lnie false, aby wysz�o z p�tli while
 switch (Controlling->EngineType)
 {
  case None:
   if (Controlling->MainCtrlPosNo>0) //McZapkie-041003: wagon sterowniczy, np. EZT
    OK=Controlling->DecMainCtrl(1+(Controlling->MainCtrlPos>2?1:0));
   break;
  case ElectricSeriesMotor:
   OK=Controlling->DecScndCtrl(2);
   if (!OK)
    OK=Controlling->DecMainCtrl(1+(Controlling->MainCtrlPos>2?1:0));
   break;
  case Dumb:
  case DieselElectric:
   OK=Controlling->DecScndCtrl(2);
   if (!OK)
    OK=Controlling->DecMainCtrl(2+(Controlling->MainCtrlPos/2));
   break;
  //WheelsDriven :
   // begin
   //  OK:=False;
   // end;
  case DieselEngine:
   if ((Controlling->Vel>Controlling->dizel_minVelfullengage))
   {
    if (Controlling->RList[Controlling->MainCtrlPos].Mn>0)
     OK=Controlling->DecMainCtrl(1);
   }
   else
    while ((Controlling->RList[Controlling->MainCtrlPos].Mn>0)&&(Controlling->MainCtrlPos>1))
     OK=Controlling->DecMainCtrl(1);
   break;
 }
 return OK;
}

void __fastcall TController::RecognizeCommand()
{//odczytuje i wykonuje komend� przekazan� lokomotywie
 TCommand *c=&Controlling->CommandIn;
 PutCommand(c->Command,c->Value1,c->Value2,c->Location,stopComm);
 c->Command=""; //usuni�cie obs�u�onej komendy
}


void __fastcall TController::PutCommand(AnsiString NewCommand,double NewValue1,double NewValue2,const TLocation &NewLocation,TStopReason reason)
{//wys�anie komendy przez event PutValues, jak pojazd ma obsad�, to wysy�a tutaj, a nie do pojazdu bezpo�rednio
 vector3 sl;
 sl.x=-NewLocation.X; //zamiana na wsp�rz�dne scenerii
 sl.z= NewLocation.Y;
 sl.y= NewLocation.Z;
 if (!PutCommand(NewCommand,NewValue1,NewValue2,&sl,reason))
  Controlling->PutCommand(NewCommand,NewValue1,NewValue2,NewLocation);
}


bool __fastcall TController::PutCommand(AnsiString NewCommand,double NewValue1,double NewValue2,const vector3 *NewLocation,TStopReason reason)
{//analiza komendy
 if (NewCommand=="Emergency_brake") //wymuszenie zatrzymania, niezale�nie kto prowadzi
 {//Ra: no nadal nie jest zbyt pi�knie
  SetVelocity(0,0,reason);
  Controlling->PutCommand("Emergency_brake",1.0,1.0,Controlling->Loc);
  return true; //za�atwione
 }
 else if ((NewCommand.Pos("Timetable:")==1)||(NewCommand.Pos("Timetable=")==1))
 {//przypisanie nowego rozk�adu jazdy, r�wnie� prowadzonemu przez u�ytkownika
  NewCommand.Delete(1,10); //zostanie nazwa pliku z rozk�adem
  if (!TrainParams)
   TrainParams=new TTrainParameters(NewCommand); //rozk�ad jazdy
  else
   TrainParams->NewName(NewCommand); //czy�ci tabelk� przystank�w
  if (NewCommand!="none")
  {if (!TrainParams->LoadTTfile(Global::asCurrentSceneryPath,NewValue1))
   {
    if (ConversionError==-8)
     ErrorLog("Missed file: "+NewCommand);
    WriteLog("Cannot load timetable file "+NewCommand+"\r\nError "+ConversionError+" in position "+TrainParams->StationCount);
   }
   else
   {//inicjacja pierwszego przystanku i pobranie jego nazwy
    TrainParams->UpdateMTable(GlobalTime->hh,GlobalTime->mm,TrainParams->NextStationName);
    TrainParams->StationIndexInc(); //przej�cie do nast�pnej
    asNextStop=TrainParams->NextStop();
   }
  }
  //if (iDirectionOrder==0) //je�li kierunek ma nieokre�lony
  if (NewValue1!=0.0) //a ma jecha�
   iDirectionOrder=NewValue1>0?1:-1; //ustalenie kierunku jazdy
  Activation(); //umieszczenie obs�ugi we w�a�ciwym cz�onie
  CheckVehicles(); //sprawdzenie sk�adu, AI zapali �wiat�a
  OrdersInit(fabs(NewValue1)); //ustalenie tabelki komend wg rozk�adu oraz pr�dko�ci pocz�tkowej
  //if (NewValue1!=0.0) if (!AIControllFlag) DirectionForward(NewValue1>0.0); //ustawienie nawrotnika u�ytkownikowi (propaguje si� do cz�on�w)
  //if (NewValue1>0)
  // TrainNumber=floor(NewValue1); //i co potem ???
  return true; //za�atwione
 }
 if (AIControllFlag==Humandriver)
  return false; //na razie reakcja na komendy nie jest odpowiednia dla pojazdu prowadzonego r�cznie
 if (NewCommand=="SetVelocity")
 {
  if (NewLocation)
   vCommandLocation=*NewLocation;
  if ((NewValue1!=0.0)&&(OrderList[OrderPos]!=Obey_train))
  {//o ile jazda
   if (!EngineActive)
    OrderNext(Prepare_engine); //trzeba odpali� silnik najpierw, �wiat�a ustawi JumpToNextOrder()
   //if (OrderList[OrderPos]!=Obey_train) //je�li nie poci�gowa
   OrderNext(Obey_train); //to uruchomi� jazd� poci�gow� (od razu albo po odpaleniu silnika
   OrderCheck(); //je�li jazda poci�gowa teraz, to wykona� niezb�dne operacje
  }
  SetVelocity(NewValue1,NewValue2,reason); //bylo: nic nie rob bo SetVelocity zewnetrznie jest wywolywane przez dynobj.cpp
 }
 else if (NewCommand=="SetProximityVelocity")
 {
#if !TESTTABLE
  if (SetProximityVelocity(NewValue1,NewValue2))
   if (NewLocation)
    vCommandLocation=*NewLocation;
#endif
 }
 else if (NewCommand=="ShuntVelocity")
 {//uruchomienie jazdy manewrowej b�d� zmiana pr�dko�ci
  if (NewLocation)
   vCommandLocation=*NewLocation;
  //if (OrderList[OrderPos]=Obey_train) and (NewValue1<>0))
  if (!EngineActive)
   OrderNext(Prepare_engine); //trzeba odpali� silnik najpierw
  OrderNext(Shunt); //zamieniamy w aktualnej pozycji, albo dodajey za odpaleniem silnika
  if (NewValue1!=0.0)
  {
   //if (iVehicleCount>=0) WriteLog("Skasowano ilos� wagon�w w ShuntVelocity!");
   iVehicleCount=-2; //warto�� neutralna
   CheckVehicles(); //zabra� to do OrderCheck()
  }
  //dla pr�dko�ci ujemnej przestawi� nawrotnik do ty�u? ale -1=brak ograniczenia !!!!
  //if (Prepare2press) WriteLog("Skasowano docisk w ShuntVelocity!");
  Prepare2press=false; //bez dociskania
  SetVelocity(NewValue1,NewValue2,reason);
  if (fabs(NewValue1)>2.0) //o ile warto�� jest sensowna (-1 nie jest konkretn� warto�ci�)
   fShuntVelocity=fabs(NewValue1); //zapami�tanie obowi�zuj�cej pr�dko�ci dla manewr�w
 }
 else if (NewCommand=="Wait_for_orders")
 {//oczekiwanie; NewValue1 - czas oczekiwania, -1 = na inn� komend�
  if (NewValue1>0.0?NewValue1>fStopTime:false)
   fStopTime=NewValue1; //Ra: w��czenie czekania bez zmiany komendy
  else
   OrderList[OrderPos]=Wait_for_orders; //czekanie na komend� (albo da� OrderPos=0)
 }
 else if (NewCommand=="Prepare_engine")
 {//w��czenie albo wy��czenie silnika (w szerokim sensie)
  if (NewValue1==0.0)
   OrderNext(Release_engine); //wy��czy� silnik (przygotowa� pojazd do jazdy)
  else if (NewValue1>0.0)
   OrderNext(Prepare_engine); //odpali� silnik (wy��czy� wszystko, co si� da)
  //po za��czeniu przejdzie do kolejnej komendy, po wy��czeniu na Wait_for_orders
 }
 else if (NewCommand=="Change_direction")
 {
  TOrders o=OrderList[OrderPos]; //co robi� przed zmian� kierunku
  if (!EngineActive)
   OrderNext(Prepare_engine); //trzeba odpali� silnik najpierw
  if (NewValue1>0.0) iDirectionOrder=1;
  else if (NewValue1<0.0) iDirectionOrder=-1;
  else iDirectionOrder=-iDirection; //zmiana na przeciwny ni� obecny
  OrderNext(Change_direction); //zadanie komendy do wykonania
  if (o>=Shunt) //je�li jazda manewrowa albo poci�gowa
   OrderNext(o); //to samo robi� po zmianie
  else if (!o) //je�li wcze�niej by�o czekanie
   OrderNext(Shunt); //to dalej jazda manewrowa
  //Change_direction wykona si� samo i nast�pnie przejdzie do kolejnej komendy
 }
 else if (NewCommand=="Obey_train")
 {
  if (!EngineActive)
   OrderNext(Prepare_engine); //trzeba odpali� silnik najpierw
  OrderNext(Obey_train);
  //if (NewValue1>0) TrainNumber=floor(NewValue1); //i co potem ???
  OrderCheck(); //je�li jazda poci�gowa teraz, to wykona� niezb�dne operacje
 }
 else if (NewCommand=="Shunt")
 {//NewValue1 - ilo�� wagon�w (-1=wszystkie); NewValue2: 0=odczep, 1..63=do��cz, -1=bez zmian
  //-2,-y - pod��czy� do ca�ego stoj�cego sk�adu (sprz�giem y>=1), zmieni� kierunek i czeka�
  //-2, y - pod��czy� do ca�ego stoj�cego sk�adu (sprz�giem y>=1) i czeka�
  //-1,-y - pod��czy� do ca�ego stoj�cego sk�adu (sprz�giem y>=1) i jecha� w powrotn� stron�
  //-1, y - pod��czy� do ca�ego stoj�cego sk�adu (sprz�giem y>=1) i jecha� dalej
  //-1, 0 - tryb manewrowy bez zmian (odczepianie z pozostawieniem wagon�w nie ma sensu)
  // 0, 0 - odczepienie lokomotywy
  // 1, y - pod��czy� do pierwszego wagonu w sk�adzie (sprz�giem y>=1) i odczepi� go od reszty
  // 1, 0 - odczepienie lokomotywy z jednym wagonem
  if (!EngineActive)
   OrderNext(Prepare_engine); //trzeba odpali� silnik najpierw
  if (NewValue2!=0) //je�li podany jest sprz�g
  {iCoupler=floor(fabs(NewValue2)); //jakim sprz�giem
   OrderNext(Connect); //po��cz (NewValue1) wagon�w
   if (NewValue2<0.0) //je�li sprz�g ujemny, to zmiana kierunku
    OrderNext(Change_direction);
  }
  else if (NewValue2==0.0)
   if (NewValue1>=0.0) //je�li ilo�� wagon�w inna ni� wszystkie
    OrderNext(Disconnect); //odczep (NewValue1) wagon�w
  if (NewValue1>=-1.0) //je�li nie -2
   OrderNext(Shunt); //to potem manewruj dalej
  CheckVehicles(); //sprawdzi� �wiat�a
  //if ((iVehicleCount>=0)&&(NewValue1<0)) WriteLog("Skasowano ilos� wagon�w w Shunt!");
  if (NewValue1!=iVehicleCount)
   iVehicleCount=floor(NewValue1); //i co potem ? - trzeba zaprogramowac odczepianie
 }
 else if (NewCommand=="Jump_to_first_order")
  JumpToFirstOrder();
 else if (NewCommand=="Jump_to_order")
 {
  if (NewValue1==-1.0)
   JumpToNextOrder();
  else
   if ((NewValue1>=0)&&(NewValue1<maxorders))
   {OrderPos=floor(NewValue1);
    if (!OrderPos) OrderPos=1; //dopiero pierwsza uruchamia
   }
/*
  if (WriteLogFlag)
  {
   append(AIlogFile);
   writeln(AILogFile,ElapsedTime:5:2," - new order: ",Order2Str( OrderList[OrderPos])," @ ",OrderPos);
   close(AILogFile);
  }
*/
 }
 else if (NewCommand=="Warning_signal")
 {
  if (NewValue1>0)
  {
   fWarningDuration=NewValue1; //czas tr�bienia
   Controlling->WarningSignal=(NewValue2>1)?2:1; //wysoko�� tonu
  }
 }
 else if (NewCommand=="OutsideStation") //wskaznik W5
 {
  if (OrderList[OrderPos]==Obey_train)
   SetVelocity(NewValue1,NewValue2,stopOut); //koniec stacji - predkosc szlakowa
  else //manewry - zawracaj
  {
   iDirectionOrder=-iDirection; //zmiana na przeciwny ni� obecny
   OrderNext(Change_direction); //zmiana kierunku
   OrderNext(Shunt); //a dalej manewry
  }
 }
 else return false; //nierozpoznana - wys�a� bezpo�rednio do pojazdu
 return true; //komenda zosta�a przetworzona
};

const TDimension SignalDim={1,1,1};

bool __fastcall TController::UpdateSituation(double dt)
{//uruchamia� przynajmniej raz na sekund�
 double AbsAccS,VelReduced;
 bool UpdateOK=false;
 //yb: zeby EP nie musial sie bawic z ciesnieniem w PG
 if (AIControllFlag)
 {
  if (Controlling->BrakeSystem==ElectroPneumatic)
   Controlling->PipePress=0.5;
  if (Controlling->SlippingWheels)
  {
   Controlling->SandDoseOn();
   Controlling->SlippingWheels=false;
  }
 }
 //ABu-160305 Testowanie gotowo�ci do jazdy
 //Ra: przeniesione z DynObj, sk�ad u�ytkownika te� jest testowany, �eby mu przekaza�, �e ma odhamowa�
 TDynamicObject* p=pVehicles[0]; //pojazd na czole sk�adu
 Ready=true; //wst�pnie gotowy
 while (p)
 {//sprawdzenie odhamowania wszystkich po��czonych pojazd�w
  if (p->MoverParameters->BrakePress>=0.03*p->MoverParameters->MaxBrakePress)
  {Ready=false; //nie gotowy
   break; //dalej nie ma co sprawdza�
  }
  p=p->Next(); //pojazd pod��czony z ty�u (patrz�c od czo�a)
 }
 HelpMeFlag=false;
 //Winger 020304
 if (Controlling->Vel>0.0)
 {//je�eli jedzie
  //przy prowadzeniu samochodu trzeba ka�d� o� odsuwa� oddzielnie, inaczej kicha wychodzi
  if (Controlling->CategoryFlag&2) //je�li samoch�d
   //if (fabs(Controlling->OffsetTrackH)<Controlling->Dim.W) //Ra: szeroko�� drogi tu powinna by�?
    if (!Controlling->ChangeOffsetH(-0.01*Controlling->Vel*dt)) //ruch w poprzek drogi
     Controlling->ChangeOffsetH(0.01*Controlling->Vel*dt); //Ra: co to mia�o by�, to nie wiem
  if (Controlling->EnginePowerSource.SourceType==CurrentCollector)
  {if (AIControllFlag)
   {Controlling->PantRear(true); //jazda na tylnym
    if (Controlling->Vel>30) //opuszczenie przedniego po rozp�dzeniu si�
    //if (Controlling->EnginePowerSource.SourceType==CurrentCollector)
    //if (AIControllFlag)
     Controlling->PantFront(false);
   }
  }
 }
 ElapsedTime+=dt;
 WaitingTime+=dt;
 fStopTime+=dt; //zliczanie czasu postoju, nie ruszy dop�ki ujemne
 if (WriteLogFlag)
 {
  if (LastUpdatedTime>deltalog)
  {//zapis do pliku DAT
/*
     writeln(LogFile,ElapsedTime," ",abs(11.31*WheelDiameter*nrot)," ",AccS," ",Couplers[1].Dist," ",Couplers[1].CForce," ",Ft," ",Ff," ",Fb," ",BrakePress," ",PipePress," ",Im," ",MainCtrlPos,"   ",ScndCtrlPos,"   ",BrakeCtrlPos,"   ",LocalBrakePos,"   ",ActiveDir,"   ",CommandIn.Command," ",CommandIn.Value1," ",CommandIn.Value2," ",SecuritySystem.status);
*/
   if (fabs(Controlling->V)>0.1)
    deltalog=0.2;
   else deltalog=1.0;
   LastUpdatedTime=0.0;
  }
  else
   LastUpdatedTime=LastUpdatedTime+dt;
 }
 if (AIControllFlag)
 {
  //tu bedzie logika sterowania
  //Ra: nie wiem czemu ReactionTime potrafi dosta� 12 sekund, to jest przegi�cie, bo prze�yna ST�J
  //yB: ot� jest to jedna trzecia czasu nape�niania na towarowym; mo�e si� przyda� przy wdra�aniu hamowania, �eby nie rusza�o kranem jak g�upie
  //Ra: ale nie mo�e si� budzi� co p� minuty, bo prze�yna semafory
  if ((LastReactionTime>Min0R(ReactionTime,2.0)))
  {//Ra: trzeba by tak:
#if TESTTABLE
   // 1. Ustali� istotn� odleg�o�� zainteresowania (np. 3�droga hamowania z V.max).
   double scanmax=(Controlling->Vel>0.0)?3*fDriverDist+fDriverMass*Controlling->Vel*Controlling->Vel:500; //fabs(Mechanik->ProximityDist);
   // 2. Sprawdzi�, czy tabelka pokrywa za�o�ony odcinek (nie musi, je�li jest STOP).
   // 3. Sprawdzi�, czy trajektoria ruchu przechodzi przez zwrotnice - je�li tak, to sprawdzi�, czy stan si� nie zmieni�.
   // 4. Ewentualnie uzupe�ni� tabelk� informacjami o sygna�ach i ograniczeniach, je�li si� "zu�y�a".
   WriteLog("");
   WriteLog("Scan table:");
   sSpeedTable.Check(scanmax,iDirection,pVehicles[0]); //wype�nianie tabelki i aktualizacja odleg�o�ci
   // 5. Sprawdzi� stany sygnalizacji zapisanej w tabelce, wyznaczy� pr�dko�ci.
   // 6. Z tabelki wyznaczy� krytyczn� odleg�o�� i pr�dko�� (najmniejsze przyspieszenie).
   // 7. Je�li jest inny pojazd z przodu, ewentualnie skorygowa� odleg�o�� i pr�dko��.
   // 8. Ustali� cz�stotliwo�� �wiadomo�ci AI (zatrzymanie precyzyjne - cz�ciej, brak atrakcji - rzadziej).
#else
   vector3 vMechLoc=pVehicles[0]->HeadPosition(); //uwzglednic potem polozenie kabiny
   if (fProximityDist>=0) //przeliczy� od zapami�tanego punktu
    ActualProximityDist=Min0R(fProximityDist,hypot(vMechLoc.x-vCommandLocation.x,vMechLoc.z-vCommandLocation.z)-0.5*(Controlling->Dim.L+SignalDim.L));
   else
    if (fProximityDist<0)
     ActualProximityDist=fProximityDist; //odleg�o�� ujemna podana bezpo�rednio (powinno by� ...=-...)
#endif
   if (Controlling->CommandIn.Command!="")
    if (!Controlling->RunInternalCommand()) //rozpoznaj komende bo lokomotywa jej nie rozpoznaje
     RecognizeCommand(); //samo czyta komend� wstawion� do pojazdu?
   if (Controlling->SecuritySystem.Status>1)
    if (!Controlling->SecuritySystemReset())
     if (TestFlag(Controlling->SecuritySystem.Status,s_ebrake)&&(Controlling->BrakeCtrlPos==0)&&(AccDesired>0.0))
      Controlling->DecBrakeLevel();
   switch (OrderList[OrderPos])
   {//ustalenie pr�dko�ci przy doczepianiu i odczepianiu, dystans�w w pozosta�ych przypadkach
    case Connect: //pod��czanie do sk�adu
    {//sprz�gi sprawdzamy w pierwszej kolejno�ci, bo jak po��czony, to koniec
     fMinProximityDist=-0.01; fMaxProximityDist=0.0; //[m] dojecha� maksymalnie
     bool ok=false;
     if (pVehicles[0]->MoverParameters->DirAbsolute>0) //je�li sprz�g 0
     {//sprz�g 0 - pr�ba podczepienia
      if (iDrivigFlags&moveConnect)
       if (pVehicles[0]->MoverParameters->Couplers[0].Connected) //je�li jest co� wykryte (a chyba jest, nie?)
        if (pVehicles[0]->MoverParameters->Attach(0,2,pVehicles[0]->MoverParameters->Couplers[0].Connected,iCoupler))
        {
         //dsbCouplerAttach->SetVolume(DSBVOLUME_MAX);
         //dsbCouplerAttach->Play(0,0,0);
        }
      //WriteLog("CoupleDist[0]="+AnsiString(pVehicles[0]->MoverParameters->Couplers[0].CoupleDist)+", Connected[0]="+AnsiString(pVehicles[0]->MoverParameters->Couplers[0].CouplingFlag));
      if (pVehicles[0]->MoverParameters->Couplers[0].CouplingFlag==iCoupler) //uda�o si�? (mog�o cz�ciowo)
       ok=true; //zaczepiony zgodnie z �yczeniem!
     }
     else if (pVehicles[0]->MoverParameters->DirAbsolute<0) //je�li sprz�g 1
     {//sprz�g 1 - pr�ba podczepienia
      if (iDrivigFlags&moveConnect)
       if (pVehicles[0]->MoverParameters->Couplers[1].Connected) //je�li jest co� wykryte (a chyba jest, nie?)
        if (pVehicles[0]->MoverParameters->Attach(1,2,pVehicles[0]->MoverParameters->Couplers[1].Connected,iCoupler))
        {
         //dsbCouplerAttach->SetVolume(DSBVOLUME_MAX);
         //dsbCouplerAttach->Play(0,0,0);
        }
      //WriteLog("CoupleDist[1]="+AnsiString(Controlling->Couplers[1].CoupleDist)+", Connected[0]="+AnsiString(Controlling->Couplers[1].CouplingFlag));
      if (pVehicles[0]->MoverParameters->Couplers[1].CouplingFlag==iCoupler) //uda�o si�? (mog�o cz�ciowo)
       ok=true; //zaczepiony zgodnie z �yczeniem!
     }
     if (ok)
     {//je�eli zosta� pod��czony
      iDrivigFlags&=~moveConnect; //zdj�cie flagi doczepiania
      SetVelocity(0,0,stopJoin); //wy��czy� przyspieszanie
      CheckVehicles(); //sprawdzi� �wiat�a nowego sk�adu
      JumpToNextOrder(); //wykonanie nast�pnej komendy
     }
     else if ((iDrivigFlags&moveConnect)==0) //je�li nie zbli�y� si� dostatecznie
      if (pVehicles[0]->fTrackBlock>100.0) //ta odleg�o�� mo�e by� rzadko od�wie�ana
      {SetVelocity(20.0,5.0); //jazda w ustawionym kierunku z pr�dko�ci� 20
       //SetProximityVelocity(-pVehicles[0]->fTrackBlock+50,0); //to nie dzia�a dobrze
       //SetProximityVelocity(pVehicles[0]->fTrackBlock,0); //to te� nie dzia�a dobrze
       //vCommandLocation=pVehicles[0]->AxlePositionGet()+pVehicles[0]->fTrackBlock*SafeNormalize(iDirection*pVehicles[0]->GetDirection());
       //WriteLog(AnsiString(vCommandLocation.x)+" "+AnsiString(vCommandLocation.z));
      }
      else if (pVehicles[0]->fTrackBlock>20.0)
        SetVelocity(0.1*pVehicles[0]->fTrackBlock,2.0); //jazda w ustawionym kierunku z pr�dko�ci� 5
      else
      {SetVelocity(2.0,0.0); //jazda w ustawionym kierunku z pr�dko�ci� 2 (18s)
       if (pVehicles[0]->fTrackBlock<2.0) //przy zderzeniu fTrackBlock nie jest miarodajne
        iDrivigFlags|=moveConnect; //wy��czenie sprawdzania fTrackBlock
      }
    } //nawias bo by�a zmienna lokalna
    break;
    case Disconnect: //20.07.03 - manewrowanie wagonami
     fMinProximityDist=1.0; fMaxProximityDist=10.0; //[m]
     VelReduced=5; //[km/h]
     if (iVehicleCount>=0) //je�li by�a podana ilo�� wagon�w
     {
      if (Prepare2press) //je�li dociskanie w celu odczepienia
      {
       SetVelocity(3,0); //jazda w ustawionym kierunku z pr�dko�ci� 3
       if (Controlling->MainCtrlPos>0) //je�li jazda
       {
        //WriteLog("Odczepianie w kierunku "+AnsiString(Controlling->DirAbsolute));
        if ((Controlling->DirAbsolute>0)&&(Controlling->Couplers[0].CouplingFlag>0))
        {
         if (!pVehicle->Dettach(0,iVehicleCount)) //zwraca mask� bitow� po��czenia
         {//tylko je�li odepnie
          //WriteLog("Odczepiony od strony 0");
          iVehicleCount=-2;
         } //a jak nie, to dociska� dalej
        }
        else if ((Controlling->DirAbsolute<0)&&(Controlling->Couplers[1].CouplingFlag>0))
        {
         if (!pVehicle->Dettach(1,iVehicleCount)) //zwraca mask� bitow� po��czenia
         {//tylko je�li odepnie
          //WriteLog("Odczepiony od strony 1");
          iVehicleCount=-2;
         } //a jak nie, to dociska� dalej
        }
        else
        {//jak nic nie jest podpi�te od ustawionego kierunku ruchu, to nic nie odczepimy
         //WriteLog("Nie ma co odczepia�?");
         HelpMeFlag=true; //cos nie tak z kierunkiem dociskania
         iVehicleCount=-2;
        }
       }
       if (iVehicleCount>=0) //zmieni si� po odczepieniu
        if (!Controlling->DecLocalBrakeLevel(1))
        {//doci�nij sklad
         //WriteLog("Dociskanie");
         Controlling->BrakeReleaser(); //wyluzuj lokomotyw�
         //Ready=true; //zamiast sprawdzenia odhamowania ca�ego sk�adu
         IncSpeed(); //dla (Ready)==false nie ruszy
        }
      }
      if ((Controlling->Vel==0.0)&&!Prepare2press)
      {//2. faza odczepiania: zmie� kierunek na przeciwny i doci�nij
       //za rad� yB ustawiamy pozycj� 3 kranu (na razie 4, bo gdzie� si� DecBrake() wykonuje)
       //WriteLog("Zahamowanie sk�adu");
       while ((Controlling->BrakeCtrlPos>4)&&Controlling->DecBrakeLevel());
       while ((Controlling->BrakeCtrlPos<4)&&Controlling->IncBrakeLevel());
       if (Controlling->PipePress-Controlling->BrakePressureTable[Controlling->BrakeCtrlPos-1+2].PipePressureVal<0.01)
       {//je�li w miar� zosta� zahamowany
        //WriteLog("Luzowanie lokomotywy i zmiana kierunku");
        Controlling->BrakeReleaser(); //wyluzuj lokomotyw�; a ST45?
        Controlling->DecLocalBrakeLevel(10); //zwolnienie hamulca
        Prepare2press=true; //nast�pnie b�dzie dociskanie
        DirectionForward(iDirection<0); //zmiana kierunku jazdy na przeciwny
        CheckVehicles(); //od razu zmieni� �wiat�a (zgasi�)
        fStopTime=0.0; //nie ma na co czeka� z odczepianiem
       }
      }
     } //odczepiania
     else //to poni�ej je�li ilo�� wagon�w ujemna
      if (Prepare2press)
      {//4. faza odczepiania: zwolnij i zmie� kierunek
       SetVelocity(0,0,stopJoin); //wy��czy� przyspieszanie
       if (!DecSpeed()) //je�li ju� bardziej wy��czy� si� nie da
       {//ponowna zmiana kierunku
        //WriteLog("Ponowna zmiana kierunku");
        DirectionForward(iDirection>=0); //zmiana kierunku jazdy na w�a�ciwy
        Prepare2press=false; //koniec dociskania
        CheckVehicles(); //od razu zmieni� �wiat�a
        JumpToNextOrder();
        SetVelocity(fShuntVelocity,fShuntVelocity); //ustawienie pr�dko�ci jazdy
       }
      }
     break;
    case Shunt:
     //na jaka odleglosc i z jaka predkoscia ma podjechac
     fMinProximityDist=1.0; fMaxProximityDist=10.0; //[m]
     VelReduced=5; //[km/h]
     break;
    case Obey_train:
     //na jaka odleglosc i z jaka predkoscia ma podjechac do przeszkody
     if (Controlling->CategoryFlag&1) //je�li poci�g
     {
      fMinProximityDist=30.0; fMaxProximityDist=60.0; //[m]
     }
     else //samochod
     {
      fMinProximityDist=5.0; fMaxProximityDist=10.0; //[m]
     }
     VelReduced=4; //[km/h]
     break;
    default:
     fMinProximityDist=0.01; fMaxProximityDist=2.0; //[m]
     VelReduced=1; //[km/h]
   } //switch
   switch (OrderList[OrderPos])
   {//co robi maszynista
    case Prepare_engine: //odpala silnik
     if (PrepareEngine())
     {//gotowy do drogi?
      SetDriverPsyche();
      //OrderList[OrderPos]:=Shunt; //Ra: to nie mo�e tak by�, bo scenerie robi� Jump_to_first_order i przechodzi w manewrowy
      JumpToNextOrder(); //w nast�pnym jest Shunt albo Obey_train, moze te� by� Change_direction, Connect albo Disconnect
      //if OrderList[OrderPos]<>Wait_for_Orders)
      // if BrakeSystem=Pneumatic)  //napelnianie uderzeniowe na wstepie
      //  if BrakeSubsystem=Oerlikon)
      //   if (BrakeCtrlPos=0))
      //    DecBrakeLevel;
     }
    break;
    case Release_engine:
     if (ReleaseEngine()) //zdana maszyna?
      JumpToNextOrder();
    break;
    case Jump_to_first_order:
     if (OrderPos>1)
      OrderPos=1; //w zerowym zawsze jest czekanie
     else
      ++OrderPos;
    break;
    case Shunt:
    case Connect:
    case Disconnect:
    case Obey_train:
    case Change_direction: //tryby wymagaj�ce jazdy
     if (OrderList[OrderPos]!=Obey_train) //spokojne manewry
     {
      VelActual=Min0R(VelActual,40); //je�li manewry, to ograniczamy pr�dko��
      if (iVehicleCount>=0)
       if (!Prepare2press)
        if (Controlling->Vel>0.0)
        {SetVelocity(0,0,stopJoin); //1. faza odczepiania: zatrzymanie
         //WriteLog("Zatrzymanie w celu odczepienia");
        }
     }
     else
      SetDriverPsyche(); //Ra: by�o w PrepareEngine(), potrzebne tu?
     //no albo przypisujemy -WaitingExpireTime, albo por�wnujemy z WaitingExpireTime
     //if ((VelActual==0.0)&&(WaitingTime>WaitingExpireTime)&&(Controlling->RunningTrack.Velmax!=0.0))
     if (OrderList[OrderPos]&(Shunt|Obey_train)) //odjecha� sam mo�e tylko je�li jest w trybie jazdy
     {//automatyczne ruszanie po odstaniu albo spod SBL
      if ((VelActual==0.0)&&(WaitingTime>0.0)&&(Controlling->RunningTrack.Velmax!=0.0))
      {//je�li stoi, a up�yn�� czas oczekiwania i tor ma niezerow� pr�dko��
/*
      if (WriteLogFlag)
       {
         append(AIlogFile);
         writeln(AILogFile,ElapsedTime:5:2,": ",Name," V=0 waiting time expired! (",WaitingTime:4:1,")");
         close(AILogFile);
       }
*/
       if (Controlling->CategoryFlag&1)
       {//je�li poci�g
        PrepareEngine(); //zmieni ustawiony kierunek
        SetVelocity(20,20); //jak si� nasta�, to niech jedzie 20km/h
        WaitingTime=0.0;
        fWarningDuration=1.5; //a zatr�bi� troch�
        Controlling->WarningSignal=1;
       }
       else
       {//samoch�d ma sta�, a� dostanie odjazd, chyba �e stoi przez kolizj�
        if (eStopReason==stopBlock)
         if (pVehicles[0]->fTrackBlock>fDriverDist)
         {PrepareEngine(); //zmieni ustawiony kierunek
          SetVelocity(-1,-1); //jak si� nasta�, to niech jedzie
          WaitingTime=0.0;
         }
       }
      }
      else if ((VelActual==0.0)&&(VelNext>0.0)&&(Controlling->Vel<1.0))
       SetVelocity(VelNext,VelNext,stopSem); //omijanie SBL
     }
     if ((HelpMeFlag)||(Controlling->DamageFlag>0))
     {
      HelpMeFlag=false;
/*
      if (WriteLogFlag)
       with Controlling do
        {
          append(AIlogFile);
          writeln(AILogFile,ElapsedTime:5:2,": ",Name," HelpMe! (",DamageFlag,")");
          close(AILogFile);
        }
*/
     }
     if (OrderList[OrderPos]==Change_direction)
     {//sprobuj zmienic kierunek
      SetVelocity(0,0,stopDir); //najpierw trzeba si� zatrzyma�
      if (Controlling->Vel<0.1)
      {//je�li si� zatrzyma�, to zmieniamy kierunek jazdy, a nawet kabin�/cz�on
       Activation(); //ustawienie zadanego wcze�niej kierunku i ewentualne przemieszczenie AI
       PrepareEngine();
       JumpToNextOrder(); //nast�pnie robimy, co jest do zrobienia (Shunt albo Obey_train)
       if (OrderList[OrderPos]==Shunt) //je�li dalej mamy manewry
        SetVelocity(fShuntVelocity,fShuntVelocity); //to od razu jedziemy
       eSignSkip=NULL; //nie ignorujemy przy poszukiwaniu nowego sygnalizatora
       eSignLast=NULL; //�eby jaki� nowy by� poszukiwany
/*
       if (WriteLogFlag)
       {
        append(AIlogFile);
        writeln(AILogFile,ElapsedTime:5:2,": ",Name," Direction changed!");
        close(AILogFile);
       }
*/
      }
      //else
      // VelActual:=0.0; //na wszelki wypadek niech zahamuje
     } //Change_direction
     //ustalanie zadanej predkosci
     if ((Controlling->ActiveDir!=0))
     {//je�li jest wybrany kierunek jazdy, sprawdzamy mo�liwe ograniczenia pr�dko�ci
      if (fStopTime<=0) //czas postoju przed dalsz� jazd� (np. na przystanku)
       VelDesired=0.0; //jak ma czeka�, to nie ma jazdy
      else if (VelActual<0)
       VelDesired=fVelMax; //ile fabryka dala (Ra: uwzgl�dione wagony)
      else
       VelDesired=Min0R(fVelMax,VelActual); //VelActual>0 jest ograniczeniem pr�dko�ci (z r�nyc �r�de�)
      if (Controlling->RunningTrack.Velmax>=0) //ograniczenie pr�dko�ci z trajektorii ruchu
       VelDesired=Min0R(VelDesired,Controlling->RunningTrack.Velmax); //uwaga na ograniczenia szlakowej!
      if (VelforDriver>=0) //tu jest zero przy zmianie kierunku jazdy
       VelDesired=Min0R(VelDesired,VelforDriver); //Ra: tu mo�e by� 40, je�li mechanik nie ma znajomo�ci szlaaku, albo kierowca je�dzi 70
      if (TrainParams)
       if (TrainParams->CheckTrainLatency()<10.0)
        if (TrainParams->TTVmax>0.0)
         VelDesired=Min0R(VelDesired,TrainParams->TTVmax); //jesli nie spozniony to nie przekracza� rozkladowej
      //Ra: tu by jeszcze trzeba by�o wstawi� uzale�nienie (VelDesired) od odleg�o�ci od przeszkody
      // no chyba �eby to uwzgldni� ju� w (ActualProximityDist)
#if TESTTABLE
      //Ra: odczyt (ActualProximityDist), (VelNext) i (AccPreferred) z tabelki pr�dkosci
      SetDriverPsyche(); //ustawia AccPreferred
      VelNext=VelDesired; //maksymalna pr�dko�� wynikaj�ca z innych czynnik�w ni� trajektoria ruchu
      ActualProximityDist=scanmax; //funkcja Update() mo�e pozostawi� warto�ci bez zmian
      sSpeedTable.Update(Controlling->Vel,VelDesired,ActualProximityDist,VelNext,AccPreferred); //szukanie optymalnych warto�ci
#endif

      AbsAccS=Controlling->AccS; //czy sie rozpedza czy hamuje
      if (Controlling->V<0.0) AbsAccS=-AbsAccS;
      else if (Controlling->V==0.0) AbsAccS=0.0;
#if LOGVELOCITY
      //WriteLog("VelDesired="+AnsiString(VelDesired)+", VelActual="+AnsiString(VelActual));
      WriteLog("Vel="+AnsiString(Controlling->Vel)+", AbsAccS="+AnsiString(AbsAccS));
#endif
      //ustalanie zadanego przyspieszenia
      if ((VelNext>=0.0)&&(ActualProximityDist>=0)&&(Controlling->Vel>=VelNext)) //gdy zbliza sie i jest za szybko do NOWEGO
      {
       if (Controlling->Vel>0.0) //jesli nie stoi
       {
        if ((Controlling->Vel<VelReduced+VelNext)&&(ActualProximityDist>fMaxProximityDist*(1+0.1*Controlling->Vel))) //dojedz do semafora/przeszkody
        {//je�li jedzie wolniej ni� mo�na i jest wystarczaj�co daleko, to mo�na przyspieszy�
         if (AccPreferred>0.0)
          AccDesired=0.5*AccPreferred;
          //VelDesired:=Min0R(VelDesired,VelReduced+VelNext);
        }
        else if (ActualProximityDist>fMinProximityDist)
        {//25.92 (=3.6*3.6*2) - skorektowane, zeby ladniej hamowal
         if ((VelNext>Controlling->Vel-35.0)) //dwustopniowe hamowanie - niski przy ma�ej r�nicy
         {if ((VelNext>0)&&(VelNext>Controlling->Vel-3.0)) //je�li powolna i niewielkie przekroczenie
            AccDesired=0.0; //to olej (zacznij luzowa�)
          else  //w przeciwnym wypadku
           if ((VelNext==0.0)&&(Controlling->V*Controlling->V<0.4*ActualProximityDist)) //je�li st�jka i niewielka pr�dko��
           {if (Controlling->Vel<30.0)  //trzymaj 30 km/h
             AccDesired=0.5*AccPreferred; //jak jest tu 0.5, to samochody si� dobijaj� do siebie
            else
             AccDesired=0.0;
           }
           else // prostu hamuj (niski stopie�)
            if ((ActualProximityDist<fMaxProximityDist)&&(VelNext==0.0))
              AccDesired=-1;
            else
            AccDesired=(VelNext*VelNext-Controlling->Vel*Controlling->Vel)/(25.92*ActualProximityDist+0.1); //hamuj proporcjonalnie //mniejsze op�nienie przy ma�ej r�nicy
         }
         else  //przy du�ej r�nicy wysoki stopie� (1,25 potrzebnego opoznienia)
          AccDesired=(VelNext*VelNext-Controlling->Vel*Controlling->Vel)/(20.73*ActualProximityDist+0.1); //hamuj proporcjonalnie //najpierw hamuje mocniej, potem zluzuje
         if (AccPreferred<AccDesired)
          AccDesired=AccPreferred; //(1+abs(AccDesired))
         ReactionTime=0.5*Controlling->BrakeDelay[2+2*Controlling->BrakeDelayFlag]; //aby szybkosc hamowania zalezala od przyspieszenia i opoznienia hamulcow
        }
        else
         VelDesired=Min0R(VelDesired,VelNext); //utrzymuj predkosc bo juz blisko
       }
       else  //zatrzymany
        if ((VelNext>0.0)||(ActualProximityDist>fMaxProximityDist*1.2))
         if (AccPreferred>0)
          AccDesired=0.5*AccPreferred; //dociagnij do semafora
         else
          VelDesired=0.0; //stoj
      }
      else //gdy jedzie wolniej, to jest normalnie
       AccDesired=(VelDesired!=0.0?AccPreferred:-0.01); //normalna jazda
   	//koniec predkosci nastepnej
      if ((VelDesired>=0)&&(Controlling->Vel>VelDesired)) //jesli jedzie za szybko do AKTUALNEGO
       if (VelDesired==0.0) //jesli stoj, to hamuj, ale i tak juz za pozno :)
        AccDesired=-0.5;
       else
        if ((Controlling->Vel<VelDesired+5)) //o 5 km/h to olej
        {if ((AccDesired>0.0))
          AccDesired=0.0;
         //else
        }
        else
         AccDesired=-0.2; //hamuj tak �rednio
      //if AccDesired>-AccPreferred)
      // Accdesired:=-AccPreferred;
      //koniec predkosci aktualnej
      if ((AccDesired>0)&&(VelNext>=0)) //wybieg b�d� lekkie hamowanie, warunki byly zamienione
       if (VelNext<Controlling->Vel-100.0) //lepiej zaczac hamowac
        AccDesired=-0.2;
       else
        if ((VelNext<Controlling->Vel-70))
         AccDesired=0.0; //nie spiesz si�, bo b�dzie hamowanie
      //koniec wybiegu i hamowania
      //w��czanie bezpiecznika
      if (Controlling->EngineType==ElectricSeriesMotor)
       if (Controlling->FuseFlag||Need_TryAgain)
        if (!Controlling->DecScndCtrl(1))
         if (!Controlling->DecMainCtrl(2))
          if (!Controlling->DecMainCtrl(1))
           if (!Controlling->FuseOn())
            HelpMeFlag=true;
           else
           {
            ++iDriverFailCount;
            if (iDriverFailCount>maxdriverfails)
             Psyche=Easyman;
            if (iDriverFailCount>maxdriverfails*2)
             SetDriverPsyche();
           }
      if (Controlling->BrakeSystem==Pneumatic) //nape�nianie uderzeniowe
       if (Controlling->BrakeSubsystem==Oerlikon)
       {
        if (Controlling->BrakeCtrlPos==-2)
         Controlling->BrakeCtrlPos=0;
        if ((Controlling->BrakeCtrlPos<0)&&(Controlling->PipeBrakePress<0.01))//{(CntrlPipePress-(Volume/BrakeVVolume/10)<0.01)})
         Controlling->IncBrakeLevel();
        if ((Controlling->BrakeCtrlPos==0)&&(AbsAccS<0.0)&&(AccDesired>0.0))
        //if FuzzyLogicAI(CntrlPipePress-PipePress,0.01,1))
         if (Controlling->PipeBrakePress>0.01)//{((Volume/BrakeVVolume/10)<0.485)})
          Controlling->DecBrakeLevel();
         else
          if (Need_BrakeRelease)
          {
           Need_BrakeRelease=false;
           //DecBrakeLevel(); //z tym by jeszcze mia�o jaki� sens
          }
       }
#if LOGVELOCITY
      WriteLog("Dist="+FloatToStrF(ActualProximityDist,ffFixed,7,1)+", VelDesired="+FloatToStrF(VelDesired,ffFixed,7,1)+", AccDesired="+FloatToStrF(AccDesired,ffFixed,7,3)+", VelActual="+AnsiString(VelActual)+", VelNext="+AnsiString(VelNext));
#endif

      if (AccDesired>=0.0)
       if (Prepare2press)
        Controlling->BrakeReleaser(); //wyluzuj lokomotyw�
       else
        while (DecBrake());  //je�li przyspieszamy, to nie hamujemy
      //Ra: zmieni�em 0.95 na 1.0 - trzeba ustali�, sk�d sie takie warto�ci bior�
      //margines dla pr�dko�ci jest doliczany tylko je�li oczekiwana pr�dko�� jest wi�ksza od 5km/h
      if ((AccDesired<=0.0)||(Controlling->Vel+(VelDesired>5.0?VelMargin:0.0)>VelDesired*1.0))
       while (DecSpeed()); //je�li hamujemy, to nie przyspieszamy
      //yB: usuni�te r�ne dziwne warunki, oddzielamy cz�� zadaj�c� od wykonawczej
      //zwiekszanie predkosci
      if ((AbsAccS<AccDesired)&&(Controlling->Vel<VelDesired*0.95-VelMargin)&&(AccDesired>=0.0))
      //Ra: zmieni�em 0.85 na 0.95 - trzeba ustali�, sk�d sie takie warto�ci bior�
      //if (!MaxVelFlag) //Ra: to nie jest u�ywane
       if (!IncSpeed())
        MaxVelFlag=true; //Ra: to nie jest u�ywane
       else
        MaxVelFlag=false; //Ra: to nie jest u�ywane
      //if (Vel<VelDesired*0.85) and (AccDesired>0) and (EngineType=ElectricSeriesMotor)
      // and (RList[MainCtrlPos].R>0.0) and (not DelayCtrlFlag))
      // if (Im<Imin) and Ready=True {(BrakePress<0.01*MaxBrakePress)})
      //  IncMainCtrl(1); //zwieksz nastawnik skoro mo�esz - tak aby si� ustawic na bezoporowej

      //yB: usuni�te r�ne dziwne warunki, oddzielamy cz�� zadaj�c� od wykonawczej
      //zmniejszanie predkosci
      if ((AccDesired<-0.35)&&(AbsAccS>AccDesired+0.05))
      //if not MinVelFlag)
       if (!IncBrake())
        MinVelFlag=true;
       else
       {MinVelFlag=false;
        ReactionTime=3+0.5*(Controlling->BrakeDelay[2+2*Controlling->BrakeDelayFlag]-3);
       }
      if ((AccDesired<-0.05)&&(AbsAccS<AccDesired-0.2))
      //if ((AccDesired<0.0)&&(AbsAccS<AccDesired-0.1)) //ST44 nie hamuje na czas, 2-4km/h po mini�ciu tarczy
      {//jak hamuje, to nie tykaj kranu za cz�sto
       //yB: luzuje hamulec dopiero przy r�nicy op�nie� rz�du 0.2
       DecBrake();
       ReactionTime=(Controlling->BrakeDelay[1+2*Controlling->BrakeDelayFlag])/3.0;
      }
      //Mietek-end1

      //zapobieganie poslizgowi w czlonie silnikowym
      if (Controlling->Couplers[0].Connected!=NULL)
       if (TestFlag(Controlling->Couplers[0].CouplingFlag,ctrain_controll))
        if (Controlling->Couplers[0].Connected->SlippingWheels)
         if (!Controlling->DecScndCtrl(1))
         {
          if (!Controlling->DecMainCtrl(1))
           if (Controlling->BrakeCtrlPos==Controlling->BrakeCtrlPosNo)
            Controlling->DecBrakeLevel();
          ++iDriverFailCount;
         }
      //zapobieganie poslizgowi u nas
      if (Controlling->SlippingWheels)
      {
       if (!Controlling->DecScndCtrl(1))
        if (!Controlling->DecMainCtrl(1))
         if (Controlling->BrakeCtrlPos==Controlling->BrakeCtrlPosNo)
          Controlling->DecBrakeLevel();
         else
          Controlling->AntiSlippingButton();
       ++iDriverFailCount;
      }
      if (iDriverFailCount>maxdriverfails)
      {
       Psyche=Easyman;
       if (iDriverFailCount>maxdriverfails*2)
        SetDriverPsyche();
      }
     } //kierunek r�ny od zera
     //odhamowywanie sk�adu po zatrzymaniu i zabezpieczanie lokomotywy
     if ((Controlling->V==0.0)&&((VelDesired==0.0)||(AccDesired==0.0)))
      if ((Controlling->BrakeCtrlPos<1)||!Controlling->DecBrakeLevel())
       Controlling->IncLocalBrakeLevel(1);

     //??? sprawdzanie przelaczania kierunku
     //if (ChangeDir!=0)
     // if (OrderDirectionChange(ChangeDir)==-1)
     //  VelActual=VelProximity;
     break; //rzeczy robione przy jezdzie
   } //switch (OrderList[OrderPos])
   //kasowanie licznika czasu
   LastReactionTime=0.0;
   //ewentualne skanowanie toru
   if (!ScanMe)
    ScanMe=true;
   UpdateOK=true;
  }
  else
   LastReactionTime+=dt;
  if (fWarningDuration>0.0) //je�li pozosta�o co� do wytr�bienia
  {//tr�bienie trwa nadal
   fWarningDuration=fWarningDuration-dt;
   if (fWarningDuration<0.1)
    Controlling->WarningSignal=0.0; //a tu si� ko�czy
  }
#if !TESTTABLE
  if (UpdateOK) //stary system skanowania z wysy�aniem komend
   ScanEventTrack(); //skanowanie tor�w przeniesione z DynObj
#endif
  return UpdateOK;
 }
 else //if (AIControllFlag)
  return false; //AI nie obs�uguje
}

void __fastcall TController::JumpToNextOrder()
{
 if (OrderList[OrderPos]!=Wait_for_orders)
 {
  if (OrderPos<maxorders-1)
   ++OrderPos;
  else
   OrderPos=0;
 }
 OrderCheck();
};

void __fastcall TController::JumpToFirstOrder()
{//taki relikt
 OrderPos=1;
 if (OrderTop==0) OrderTop=1;
 OrderCheck();
};

void __fastcall TController::OrderCheck()
{//reakcja na zmian� rozkazu
 if (OrderList[OrderPos]==Shunt)
  CheckVehicles();
 if (OrderList[OrderPos]==Obey_train)
 {CheckVehicles();
  iDrivigFlags|=moveStopPoint; //W4 s� widziane
 }
 else if (OrderList[OrderPos]==Change_direction)
  iDirectionOrder=-iDirection; //trzeba zmieni� jawnie, bo si� nie domy�li
 else if (OrderList[OrderPos]==Disconnect)
  iVehicleCount=0; //odczepianie lokomotywy
 else if (OrderList[OrderPos]==Wait_for_orders)
  OrdersClear(); //czyszczenie rozkaz�w i przeskok do zerowej pozycji
}

void __fastcall TController::OrderNext(TOrders NewOrder)
{//ustawienie rozkazu do wykonania jako nast�pny
 if (OrderList[OrderPos]==NewOrder)
  return; //je�li robi to, co trzeba, to koniec
 if (!OrderPos) OrderPos=1; //na pozycji zerowej pozostaje czekanie
 OrderTop=OrderPos; //ale mo�e jest czym� zaj�ty na razie
 if (NewOrder>=Shunt) //je�li ma jecha�
 {//ale mo�e by� zaj�ty chwilowymi operacjami
  while (OrderList[OrderTop]?OrderList[OrderTop]<Shunt:false) //je�li co� robi
   ++OrderTop; //pomijamy wszystkie tymczasowe prace
 }
 else
 {//je�li ma ustawion� jazd�, to wy��czamy na rzecz operacji
  while (OrderList[OrderTop]?(OrderList[OrderTop]<Shunt)&&(OrderList[OrderTop]!=NewOrder):false) //je�li co� robi
   ++OrderTop; //pomijamy wszystkie tymczasowe prace
 }
 OrderList[OrderTop++]=NewOrder; //dodanie rozkazu jako nast�pnego
}

void __fastcall TController::OrderPush(TOrders NewOrder)
{//zapisanie na stosie kolejnego rozkazu do wykonania
 if (OrderPos==OrderTop) //je�li mia�by by� zapis na aktalnej pozycji
  if (OrderList[OrderPos]<Shunt) //ale nie jedzie
   ++OrderTop; //niekt�re operacje musz� zosta� najpierw doko�czone => zapis na kolejnej
 if (OrderList[OrderTop]!=NewOrder) //je�li jest to samo, to nie dodajemy
  OrderList[OrderTop++]=NewOrder; //dodanie rozkazu na stos
 //if (OrderTop<OrderPos) OrderTop=OrderPos;
}

void __fastcall TController::OrdersDump()
{//wypisanie kolejnych rozkaz�w do logu
 for (int b=0;b<maxorders;b++)
  WriteLog(AnsiString(b)+": "+Order2Str(OrderList[b])+(OrderPos==b?" <-":""));
};

TOrders __fastcall TController::OrderCurrentGet()
{
 return OrderList[OrderPos];
}

TOrders __fastcall TController::OrderNextGet()
{
 return OrderList[OrderPos+1];
}

void __fastcall TController::OrdersInit(double fVel)
{//wype�nianie tabelki rozkaz�w na podstawie rozk�adu
 //ustawienie kolejno�ci komend, niezale�nie kto prowadzi
 //Mechanik->OrderPush(Wait_for_orders); //czekanie na lepsze czasy
 //OrderPos=OrderTop=0; //wype�niamy od pozycji 0
 OrdersClear(); //usuni�cie poprzedniej tabeli
 OrderPush(Prepare_engine); //najpierw odpalenie silnika
 if (TrainParams->TrainName==AnsiString("none"))
  OrderPush(Shunt); //je�li nie ma rozk�adu, to manewruje
 else
 {//je�li z rozk�adem, to jedzie na szlak
  if (TrainParams?
   (TrainParams->TimeTable[1].StationWare.Pos("@")? //je�li obr�t na pierwszym przystanku
   (Controlling->TrainType&(dt_EZT)? //SZT r�wnie�! SN61 zale�nie od wagon�w...
   (TrainParams->TimeTable[1].StationName==TrainParams->Relation1):false):false):true)
   OrderPush(Shunt); //a teraz start b�dzie w manewrowym, a tryb poci�gowy w��czy W4
  else
  //je�li start z pierwszej stacji i jednocze�nie jest na niej zmiana kierunku, to EZT ma mie� Shunt
   OrderPush(Obey_train); //dla starych scenerii start w trybie pociagowym
  if (DebugModeFlag) //normalnie nie ma po co tego wypisywa�
   WriteLog("/* Timetable: "+TrainParams->ShowRelation());
  TMTableLine *t;
  for (int i=0;i<=TrainParams->StationCount;++i)
  {t=TrainParams->TimeTable+i;
   if (DebugModeFlag) //normalnie nie ma po co tego wypisywa�
    WriteLog(AnsiString(t->StationName)+" "+AnsiString((int)t->Ah)+":"+AnsiString((int)t->Am)+", "+AnsiString((int)t->Dh)+":"+AnsiString((int)t->Dm)+" "+AnsiString(t->StationWare));
   if (AnsiString(t->StationWare).Pos("@"))
   {//zmiana kierunku i dalsza jazda wg rozk�adu
    if (Controlling->TrainType&(dt_EZT)) //SZT r�wnie�! SN61 zale�nie od wagon�w...
    {//je�li sk�ad zespolony, wystarczy zmieni� kierunek jazdy
     OrderPush(Change_direction); //zmiana kierunku
    }
    else
    {//dla zwyk�ego sk�adu wagonowego odczepiamy lokomotyw�
     OrderPush(Disconnect); //odczepienie lokomotywy
     OrderPush(Shunt); //a dalej manewry
     OrderPush(Change_direction); //zmiana kierunku
     OrderPush(Shunt); //jazda na drug� stron� sk�adu
     OrderPush(Change_direction); //zmiana kierunku
     OrderPush(Connect); //jazda pod wagony
    }
    if (i<TrainParams->StationCount) //jak nie ostatnia stacja
     OrderPush(Obey_train); //to dalej wg rozk�adu
   }
  }
  if (DebugModeFlag) //normalnie nie ma po co tego wypisywa�
   WriteLog("*/");
  OrderPush(Shunt); //po wykonaniu rozk�adu prze��czy si� na manewry
 }
 //McZapkie-100302 - to ma byc wyzwalane ze scenerii
 if (fVel==0.0)
  SetVelocity(0,0,stopSleep); //je�li nie ma pr�dko�ci pocz�tkowej, to �pi
 else
 {//je�li podana niezerowa pr�dko��
  if (fVel>=1.0) //je�li ma jecha�
   iDrivigFlags|=moveStopCloser; //to do nast�pnego W4 ma podjecha� blisko
  JumpToFirstOrder();
  SetVelocity(fVel,-1); //ma ustawi� ��dan� pr�dko��
 }
 if (DebugModeFlag) //normalnie nie ma po co tego wypisywa�
  OrdersDump(); //wypisanie kontrolne tabelki rozkaz�w
 //McZapkie! - zeby w ogole AI ruszyl to musi wykonac powyzsze rozkazy
 //Ale mozna by je zapodac ze scenerii
};


AnsiString __fastcall TController::StopReasonText()
{//informacja tekstowa o przyczynie zatrzymania
 if (eStopReason!=7)
  return StopReasonTable[eStopReason];
 else
  return "Blocked by "+(pVehicles[0]->PrevAny()->GetName());
};

//----------------McZapkie: skanowanie semaforow:

double __fastcall TController::Distance(vector3 &p1,vector3 &n,vector3 &p2)
{//Ra:obliczenie odleg�o�ci punktu (p1) od p�aszczyzny o wektorze normalnym (n) przechodz�cej przez (p2)
 return n.x*(p1.x-p2.x)+n.y*(p1.y-p2.y)+n.z*(p1.z-p2.z); //ax1+by1+cz1+d, gdzie d=-(ax2+by2+cz2)
};

//pomocnicza funkcja sprawdzania czy do toru jest podpiety semafor
bool __fastcall TController::CheckEvent(TEvent *e,bool prox)
{//sprawdzanie eventu, czy jest obs�ugiwany poza kolejk�
 //prox=true, gdy sprawdzanie, czy doda� event do kolejki
 //-> zwraca true, je�li event istotny dla AI
 //prox=false, gdy wyszukiwanie semafora przez AI
 //-> zwraca false, gdy event ma by� dodany do kolejki
 if (e)
 {if (!prox) if (e==eSignSkip) return false; //semafor przejechany jest ignorowany
  AnsiString command;
  switch (e->Type)
  {//to si� wykonuje r�wnie� sk�adu jad�cego bez obs�ugi
   case tp_GetValues:
    command=e->CommandGet();
    if (prox?true:OrderCurrentGet()!=Obey_train) //tylko w trybie manewrowym albo sprawdzanie ignorowania
     if (command=="ShuntVelocity")
      return true;
    break;
   case tp_PutValues:
    command=e->CommandGet();
    break;
   default:
    return false; //inne eventy si� nie licz�
  }
  if (prox)
   if (command=="SetProximityVelocity")
    return true; //ten event z toru ma by� ignorowany
  if (command=="SetVelocity")
   return true; //jak podamy wyjazd, to prze��czy si� w jazd� poci�gow�
  if (prox) //pomijanie punkt�w zatrzymania
  {if (command.SubString(1,19)=="PassengerStopPoint:")
    return true;
  }
  else //nazwa stacji pobrana z rozk�adu - nie ma co sprawdza� raz po raz
   if (!asNextStop.IsEmpty())
    if (command.SubString(1,asNextStop.Length())==asNextStop)
     return true;
 }
 return false;
}

TEvent* __fastcall TController::CheckTrackEvent(double fDirection,TTrack *Track)
{//sprawdzanie event�w na podanym torze
 //ZiomalCl: teraz zwracany jest pierwszy event podajacy predkosc dla AI
 //a nie kazdy najblizszy event [AI sie gubilo gdy przed getval z SetVelocity
 //mialo np. PutValues z eventem od SHP]
 TEvent* e=(fDirection>0)?Track->Event2:Track->Event1;
 return CheckEvent(e,false)?e:NULL; //sprawdzenie z pomini�ciem niepotrzebnych
}

TTrack* __fastcall TController::TraceRoute(double &fDistance,double &fDirection,TTrack *Track,TEvent*&Event)
{//szukanie semafora w kierunku jazdy (eventu odczytu kom�rki pami�ci)
 //albo ustawienia innej pr�dko�ci albo ko�ca toru
 TTrack *pTrackChVel=Track; //tor ze zmian� pr�dko�ci
 TTrack *pTrackFrom; //odcinek poprzedni, do znajdywania ko�ca dr�g
 double fDistChVel=-1; //odleg�o�� do toru ze zmian� pr�dko�ci
 double fCurrentDistance=pVehicle->RaTranslationGet(); //aktualna pozycja na torze
 double s=0;
 if (fDirection>0) //je�li w kierunku Point2 toru
  fCurrentDistance=Track->Length()-fCurrentDistance;
 if ((Event=CheckTrackEvent(fDirection,Track))!=NULL)
 {//je�li jest semafor na tym torze
  fDistance=0; //to na tym torze stoimy
  return Track;
 }
 if ((Track->fVelocity==0.0)||(Track->iDamageFlag&128))
 {
  fDistance=0; //to na tym torze stoimy
  return NULL; //stop
 }
 while (s<fDistance)
 {
  //Track->ScannedFlag=true; //do pokazywania przeskanowanych tor�w
  pTrackFrom=Track; //zapami�tanie aktualnego odcinka
  s+=fCurrentDistance; //doliczenie kolejnego odcinka do przeskanowanej d�ugo�ci
  if (fDirection>0)
  {//je�li szukanie od Point1 w kierunku Point2
   if (Track->iNextDirection)
    fDirection=-fDirection;
   Track=Track->CurrentNext(); //mo�e by� NULL
  }
  else //if (fDirection<0)
  {//je�li szukanie od Point2 w kierunku Point1
   if (!Track->iPrevDirection)
    fDirection=-fDirection;
   Track=Track->CurrentPrev(); //mo�e by� NULL
  }
  if (Track==pTrackFrom) Track=NULL; //koniec, tak jak dla tor�w
  if (Track?(Track->fVelocity==0.0)||(Track->iDamageFlag&128):true)
  {//gdy dalej toru nie ma albo zerowa pr�dko��, albo uszkadza pojazd
   fDistance=s;
   return NULL; //zwraca NULL, ewentualnie tor z zerow� pr�dko�ci�
  }
  if (fDistChVel<0? //gdy pierwsza zmiana pr�dko�ci
   (Track->fVelocity!=pTrackChVel->fVelocity): //to pr�dkos� w kolejnym torze ma by� r�na od aktualnej
   ((pTrackChVel->fVelocity<0.0)? //albo je�li by�a mniejsza od zera (maksymalna)
    (Track->fVelocity>=0.0): //to wystarczy, �e nast�pna b�dzie nieujemna
    (Track->fVelocity<pTrackChVel->fVelocity))) //albo dalej by� mniejsza ni� poprzednio znaleziona dodatnia
  {
   fDistChVel=s; //odleg�o�� do zmiany pr�dko�ci
   pTrackChVel=Track; //zapami�tanie toru
  }
  fCurrentDistance=Track->Length();
  if ((Event=CheckTrackEvent(fDirection,Track))!=NULL)
  {//znaleziony tor z eventem
   fDistance=s;
   return Track;
  }
 }
 Event=NULL; //jak dojdzie tu, to nie ma semafora
 if (fDistChVel<0)
 {//zwraca ostatni sprawdzony tor
  fDistance=s;
  return Track;
 }
 fDistance=fDistChVel; //odleg�o�� do zmiany pr�dko�ci
 return pTrackChVel; //i tor na kt�rym si� zmienia
}


//sprawdzanie zdarze� semafor�w i ogranicze� szlakowych
void __fastcall TController::SetProximityVelocity(double dist,double vel,const vector3 *pos)
{//Ra:przeslanie do AI pr�dko�ci
/*
 //!!!! zast�pi� prawid�ow� reakcj� AI na SetProximityVelocity !!!!
 if (vel==0)
 {//je�li zatrzymanie, to zmniejszamy dystans o 10m
  dist-=10.0;
 };
 if (dist<0.0) dist=0.0;
 if ((vel<0)?true:dist>0.1*(MoverParameters->Vel*MoverParameters->Vel-vel*vel)+50)
 {//je�li jest dalej od umownej drogi hamowania
*/
  PutCommand("SetProximityVelocity",dist,vel,pos);
#if LOGVELOCITY
  WriteLog("-> SetProximityVelocity "+AnsiString(floor(dist))+" "+AnsiString(vel));
#endif
/*
 }
 else
 {//je�li jest zagro�enie, �e przekroczy
  Mechanik->SetVelocity(floor(0.2*sqrt(dist)+vel),vel,stopError);
#if LOGVELOCITY
  WriteLog("-> SetVelocity "+AnsiString(floor(0.2*sqrt(dist)+vel))+" "+AnsiString(vel));
#endif
 }
 */
}

int komenda[24]=
{//tabela reakcji na sygna�y podawane semaforem albo tarcz� manewrow�
 //0: nic, 1: SetProximityVelocity, 2: SetVelocity, 3: ShuntVelocity
 3, //[ 0]= sta�   stoi   blisko manewr. ShuntV.
 3, //[ 1]= jecha� stoi   blisko manewr. ShuntV.
 3, //[ 2]= sta�   jedzie blisko manewr. ShuntV.
 3, //[ 3]= jecha� jedzie blisko manewr. ShuntV.
 0, //[ 4]= sta�   stoi   deleko manewr. ShuntV. - podjedzie sam
 3, //[ 5]= jecha� stoi   deleko manewr. ShuntV.
 1, //[ 6]= sta�   jedzie deleko manewr. ShuntV.
 1, //[ 7]= jecha� jedzie deleko manewr. ShuntV.
 2, //[ 8]= sta�   stoi   blisko poci�g. SetV.
 2, //[ 9]= jecha� stoi   blisko poci�g. SetV.
 2, //[10]= sta�   jedzie blisko poci�g. SetV.
 2, //[11]= jecha� jedzie blisko poci�g. SetV.
 2, //[12]= sta�   stoi   deleko poci�g. SetV.
 2, //[13]= jecha� stoi   deleko poci�g. SetV.
 1, //[14]= sta�   jedzie deleko poci�g. SetV.
 1, //[15]= jecha� jedzie deleko poci�g. SetV.
 3, //[16]= sta�   stoi   blisko manewr. SetV.
 2, //[17]= jecha� stoi   blisko manewr. SetV. - zmiana na jazd� poci�gow�
 3, //[18]= sta�   jedzie blisko manewr. SetV.
 2, //[19]= jecha� jedzie blisko manewr. SetV.
 0, //[20]= sta�   stoi   deleko manewr. SetV. - niech podjedzie
 2, //[21]= jecha� stoi   deleko manewr. SetV.
 1, //[22]= sta�   jedzie deleko manewr. SetV.
 2  //[23]= jecha� jedzie deleko manewr. SetV.
};

void __fastcall TController::ScanEventTrack()
{//sprawdzanie zdarze� semafor�w i ogranicze� szlakowych
 if ((OrderList[OrderPos]&~(Obey_train|Shunt)))
  return; //skanowanie sygna��w tylko gdy jedzie albo czeka na rozkazy
 //Ra: AI mo�e si� stoczy� w przeciwnym kierunku, ni� oczekiwana jazda !!!!
 vector3 sl;
 //je�li z przodu od kierunku ruchu jest jaki� pojazd ze sprz�giem wirtualnym
 if (pVehicles[0]->fTrackBlock<=fDriverDist) //jak odleg�o�� kolizyjna, to sprawdzi�
 {
  pVehicles[0]->ABuScanObjects(pVehicles[0]->DirectionGet(),300); //skanowanie sprawdzaj�ce
  if (pVehicles[0]->fTrackBlock<=fDriverDist) //jak potwierdzona odleg�o�� kolizyjna, to stop
  {
   SetVelocity(0,0,stopBlock); //zatrzyma� //nie potrzeba tu zatrzymywa�, AI sobie samo sprawdzi odleg�o��
   return; //i dalej nie ma co analizowa� innych przypadk�w (!!!! do przemy�lenia)
  }
 }
 else
  if (fDriverMass*VelDesired*VelDesired>pVehicles[0]->fTrackBlock) //droga hamowania wi�ksza ni� odleg�o�� kolizyjna
   SetProximityVelocity(pVehicles[0]->fTrackBlock-fDriverDist,0); //spowolnienie jazdy
 int startdir=iDirection; //kierunek jazdy wzgl�dem pojazdu, w kt�rym siedzi AI (1=prz�d,-1=ty�)
 if (startdir==0) //je�li kabina i kierunek nie jest okre�lony
  return; //nie robimy nic
 if (OrderList[OrderPos]==Shunt?OrderList[OrderPos+1]==Change_direction:false)
 {//je�li jedzie manewrowo, ale nast�pnie ma w planie zmian� kierunku, to skanujemy te� w drug� stron�
  if (iDrivigFlags&moveBackwardLook) //je�li ostatnio by�o skanowanie do ty�u
   iDrivigFlags&=~moveBackwardLook; //do przodu popatrze� trzeba
  else
  {iDrivigFlags|=moveBackwardLook; //albo na zmian� do ty�u
   //startdir=-startdir;
   //co dalej, to trzeba jeszcze przemy�le�
  }
 }
  //startdir=2*random(2)-1; //wybieramy losowy kierunek - trzeba by jeszcze ustawi� kierunek ruchu
 //iAxleFirst - kt�ra o� jest z przodu w kierunku jazdy: =0-przednia >0-tylna
 double scandir=startdir*pVehicles[0]->RaDirectionGet(); //szukamy od pierwszej osi w kierunku ruchu
 if (scandir!=0.0) //skanowanie toru w poszukiwaniu event�w GetValues/PutValues
 {//Ra: skanowanie drogi proporcjonalnej do kwadratu aktualnej pr�dko�ci (+150m), no chyba �e stoi (wtedy 500m)
  //Ra: tymczasowo, dla wi�kszej zgodno�ci wstecz, szukanie semafora na postoju zwi�kszone do 2km
  double scanmax=(Controlling->Vel>0.0)?3*fDriverDist+fDriverMass*Controlling->Vel*Controlling->Vel:500; //fabs(Mechanik->ProximityDist);
  double scandist=scanmax; //zmodyfikuje na rzeczywi�cie przeskanowane
  //Ra: znaleziony semafor trzeba zapami�ta�, bo mo�e by� wpisany we wcze�niejszy tor
  //Ra: opr�cz semafora szukamy najbli�szego ograniczenia (koniec/brak toru to ograniczenie do zera)
  TEvent *ev=NULL; //event potencjalnie od semafora
  TTrack *scantrack=TraceRoute(scandist,scandir,pVehicles[0]->RaTrackGet(),ev); //wg drugiej osi w kierunku ruchu
  vector3 dir=startdir*pVehicles[0]->VectorFront(); //wektor w kierunku jazdy/szukania
  if (!scantrack) //je�li wykryto koniec toru albo zerow� pr�dko��
  {
   //if (!Mechanik->SetProximityVelocity(0.7*fabs(scandist),0))
    // Mechanik->SetVelocity(Mechanik->VelActual*0.9,0);
     //scandist=(scandist>5?scandist-5:0); //10m do zatrzymania
    vector3 pos=pVehicles[0]->AxlePositionGet()+scandist*dir;
#if LOGVELOCITY
    WriteLog("End of track:");
#endif
    double vend=(Controlling->CategoryFlag&1)?0:20; //poci�g ma stan��, samoch�d zwolni�
    if (scandist>10) //je�li zosta�o wi�cej ni� 10m do ko�ca toru
     SetProximityVelocity(scandist,vend,&pos); //informacja o zbli�aniu si� do ko�ca
    else
    {PutCommand("SetVelocity",vend,vend,&pos,stopEnd); //na ko�cu toru ma sta�
#if LOGVELOCITY
     WriteLog("-> SetVelocity 0 0");
#endif
    }
    return;
  }
  else
  {//je�li s� dalej tory
   double vtrackmax=scantrack->fVelocity;  //ograniczenie szlakowe
   double vmechmax=-1; //pr�dko�� ustawiona semaforem
   TEvent *e=eSignLast; //poprzedni sygna� nadal si� liczy
   if (!e) //je�li nie by�o �adnego sygna�u
    e=ev; //ten ewentualnie znaleziony (scandir>0)?scantrack->Event2:scantrack->Event1; //pobranie nowego
   if (e)
   {//je�li jest jaki� sygna� na widoku
#if LOGVELOCITY + LOGSTOPS
    AnsiString edir=pVehicle->asName+" - "+AnsiString((scandir>0)?"Event2 ":"Event1 ");
#endif
    //najpierw sprawdzamy, czy semafor czy inny znak zosta� przejechany
    //bo je�li tak, to trzeba szuka� nast�pnego, a ten mo�e go zas�ania�
    //vector3 pos=pVehicles[0]->GetPosition(); //aktualna pozycja, potrzebna do liczenia wektor�w
    vector3 pos=pVehicles[0]->HeadPosition();//+0.5*pVehicles[0]->MoverParameters->Dim.L*dir; //pozycja czo�a
    vector3 sem; //wektor do sygna�u
    if (e->Type==tp_GetValues)
    {//przes�a� info o zbli�aj�cym si� semaforze
#if LOGVELOCITY
     edir+="("+(e->Params[8].asGroundNode->asName)+"): ";
#endif
     //sem=*e->PositionGet()-pos; //wektor do kom�rki pami�ci
     sem=e->Params[8].asGroundNode->pCenter-pos; //wektor do kom�rki pami�ci
     if (dir.x*sem.x+dir.z*sem.z<0) //je�li zosta� mini�ty
      if ((Controlling->CategoryFlag&1)?(VelNext!=0.0):true) //dla poci�gu wymagany sygna� zezwalaj�cy
      {//iloczyn skalarny jest ujemny, gdy sygna� stoi z ty�u
       eSignSkip=e; //wtedy uznajemy go za ignorowany przy poszukiwaniu nowego
       eSignLast=NULL; //�eby jaki� nowy by� poszukiwany
#if LOGVELOCITY
       WriteLog(edir+"- will be ignored as passed by");
#endif
       return;
      }
     sl=e->Params[8].asGroundNode->pCenter;
     vmechmax=e->Params[9].asMemCell->fValue1; //pr�dko�� przy tym semaforze
     //przeliczamy odleg�o�� od semafora - potrzebne by by�y wsp�rz�dne pocz�tku sk�adu
     //scandist=(pos-e->Params[8].asGroundNode->pCenter).Length()-0.5*Controlling->Dim.L-10; //10m luzu
     scandist=(pos-e->Params[8].asGroundNode->pCenter).Length()-10; //10m luzu
     if (scandist<0) scandist=0; //ujemnych nie ma po co wysy�a�

     //Ra: takie rozpisanie wcale nie robi pro�ciej
     int mode=(vmechmax!=0.0)?1:0; //tryb jazdy - pr�dko�� podawana sygna�em
     mode|=(Controlling->Vel>0.0)?2:0; //stan aktualny: jedzie albo stoi
     mode|=(scandist>fMinProximityDist)?4:0;
     if (OrderList[OrderPos]!=Obey_train)
     {mode|=8; //tryb manewrowy
      if (strcmp(e->Params[9].asMemCell->szText,"ShuntVelocity")==0)
       mode|=16; //ustawienie pr�dko�ci manewrowej
     }
     if ((mode&16)==0) //o ile nie podana pr�dko�� manewrowa
      if (strcmp(e->Params[9].asMemCell->szText,"SetVelocity")==0)
       mode=32; //ustawienie pr�dko�ci poci�gowej
     //warto�ci 0..15 nie przekazuj� komendy
     //warto�ci 16..23 - ignorowanie sygna��w maewrowych w trybie poc�gowym
     mode-=24;
     if (mode>=0)
     {
      //+4: dodatkowo: - sygna� ma by� ignorowany
      switch (komenda[mode]) //pobranie kodu komendy
      {//komendy mo�liwe do przekazania:
       case 0: //nic - sygna� nie wysy�a komendy
        break;
       case 1: //SetProximityVelocity - informacja o zmienie pr�dko�ci
        break;
       case 2: //SetVelocity - nadanie pr�dko�ci do jazdy poci�gowej
        break;
       case 3: //ShuntVelocity - nadanie pr�dko�ci do jazdy manewrowej
        break;
      }
     }
     bool move=false; //czy AI w trybie manewerowym ma doci�gn�� pod S1
     if (strcmp(e->Params[9].asMemCell->szText,"SetVelocity")==0)
      if ((vmechmax==0.0)?(OrderCurrentGet()==Shunt):false)
       move=true; //AI w trybie manewerowym ma doci�gn�� pod S1
      else
      {//
       eSignLast=(scandist<200)?e:NULL; //zapami�tanie na wypadek przejechania albo �le podpi�tego toru
       if ((scandist>fMinProximityDist)?(Controlling->Vel>0.0)&&(OrderCurrentGet()!=Shunt):false)
       {//je�li semafor jest daleko, a pojazd jedzie, to informujemy o zmianie pr�dko�ci
        //je�li jedzie manewrowo, musi dosta� SetVelocity, �eby sie na poci�gowy prze��czy�
        //Mechanik->PutCommand("SetProximityVelocity",scandist,vmechmax,sl);
#if LOGVELOCITY
        //WriteLog(edir+"SetProximityVelocity "+AnsiString(scandist)+" "+AnsiString(vmechmax));
        WriteLog(edir);
#endif
        SetProximityVelocity(scandist,vmechmax,&sl);
        return;
       }
       else  //ustawiamy pr�dko�� tylko wtedy, gdy ma ruszy�, stan�� albo ma sta�
        //if ((MoverParameters->Vel==0.0)||(vmechmax==0.0)) //je�li stoi lub ma stan��/sta�
        {//semafor na tym torze albo lokomtywa stoi, a ma ruszy�, albo ma stan��, albo nie rusza�
         //stop trzeba powtarza�, bo inaczej zatr�bi i pojedzie sam
         PutCommand("SetVelocity",vmechmax,e->Params[9].asMemCell->fValue2,&sl,stopSem);
#if LOGVELOCITY
         WriteLog(edir+"SetVelocity "+AnsiString(vmechmax)+" "+AnsiString(e->Params[9].asMemCell->fValue2));
#endif
         return;
        }
      }
     if (OrderCurrentGet()==Shunt) //w Wait_for_orders te� widzi tarcze?
     {//reakcja AI w trybie manewrowym dodatkowo na sygna�y manewrowe
      if (move?true:strcmp(e->Params[9].asMemCell->szText,"ShuntVelocity")==0)
      {//je�li powy�ej by�o SetVelocity 0 0, to doci�gamy pod S1
       eSignLast=(scandist<200)?e:NULL; //zapami�tanie na wypadek przejechania albo �le podpi�tego toru
       if ((scandist>fMinProximityDist)?(Controlling->Vel>0.0)||(vmechmax==0.0):false)
       {//je�li tarcza jest daleko, to:
        //- jesli pojazd jedzie, to informujemy o zmianie pr�dko�ci
        //- je�li stoi, to z w�asnej inicjatywy mo�e podjecha� pod zamkni�t� tarcz�
        if (Controlling->Vel>0.0) //tylko je�li jedzie
        {//Mechanik->PutCommand("SetProximityVelocity",scandist,vmechmax,sl);
#if LOGVELOCITY
         //WriteLog(edir+"SetProximityVelocity "+AnsiString(scandist)+" "+AnsiString(vmechmax));
         WriteLog(edir);
#endif
         SetProximityVelocity(scandist,vmechmax,&sl);
         return;
        }
       }
       else //ustawiamy pr�dko�� tylko wtedy, gdy ma ruszy�, albo stan�� albo ma sta� pod tarcz�
       {//stop trzeba powtarza�, bo inaczej zatr�bi i pojedzie sam
        //if ((MoverParameters->Vel==0.0)||(vmechmax==0.0)) //je�li jedzie lub ma stan��/sta�
        {//nie dostanie komendy je�li jedzie i ma jecha�
         PutCommand("ShuntVelocity",vmechmax,e->Params[9].asMemCell->fValue2,&sl,stopSem);
#if LOGVELOCITY
         WriteLog(edir+"ShuntVelocity "+AnsiString(vmechmax)+" "+AnsiString(e->Params[9].asMemCell->fValue2));
#endif
         return;
        }
       }
       if ((vmechmax!=0.0)&&(scandist<100.0))
       {//je�li Tm w odleg�o�ci do 100m podaje zezwolenie na jazd�, to od razu j� ignorujemy, aby m�c szuka� kolejnej
        eSignSkip=e; //wtedy uznajemy ignorowan� przy poszukiwaniu nowej
        eSignLast=NULL; //�eby jaka� nowa by�a poszukiwana
#if LOGVELOCITY
        WriteLog(edir+"- will be ignored due to Ms2");
#endif
        return;
       }
      } //if (move?...
     } //if (OrderCurrentGet()==Shunt)
    } //if (e->Type==tp_GetValues)
    else if (e->Type==tp_PutValues?!(iDrivigFlags&moveBackwardLook):false)
    {//przy skanowaniu wstecz te eventy si� nie licz�
     sem=vector3(e->Params[3].asdouble,e->Params[4].asdouble,e->Params[5].asdouble)-pos; //wektor do kom�rki pami�ci
     if (strcmp(e->Params[0].asText,"SetVelocity")==0)
     {//statyczne ograniczenie predkosci
      if (dir.x*sem.x+dir.z*sem.z<0)
      {//iloczyn skalarny jest ujemny, gdy sygna� stoi z ty�u
       eSignSkip=e; //wtedy uznajemy go za ignorowany przy poszukiwaniu nowego
       eSignLast=NULL; //�eby jaki� nowy by� poszukiwany
#if LOGVELOCITY
       WriteLog(edir+"- will be ignored as passed by");
#endif
       return;
      }
      vmechmax=e->Params[1].asdouble;
      sl.x=e->Params[3].asdouble; //wsp�rz�dne w scenerii
      sl.y=e->Params[4].asdouble;
      sl.z=e->Params[5].asdouble;
      if (scandist<fMinProximityDist+1) //tylko na tym torze
      {
       eSignLast=(scandist<200)?e:NULL; //zapami�tanie na wypadek przejechania albo �le podpi�tego toru
       PutCommand("SetVelocity",vmechmax,e->Params[2].asdouble,&sl,stopSem);
#if LOGVELOCITY
       WriteLog("PutValues: SetVelocity "+AnsiString(vmechmax)+" "+AnsiString(e->Params[2].asdouble));
#endif
       return;
      }
      else
      {//Mechanik->PutCommand("SetProximityVelocity",scandist,e->Params[1].asdouble,sl);
#if LOGVELOCITY
       //WriteLog("PutValues: SetProximityVelocity "+AnsiString(scandist)+" "+AnsiString(e->Params[1].asdouble));
       WriteLog("PutValues:");
#endif
       SetProximityVelocity(scandist,e->Params[1].asdouble,&sl);
       return;
      }
     }
     else if ((iDrivigFlags&moveStopPoint)?strcmp(e->Params[0].asText,asNextStop.c_str())==0:false)
     {//je�li W4 z nazw� jak w rozk�adzie, to aktualizacja rozk�adu i pobranie kolejnego
      scandist=sem.Length()-0.5*Controlling->Dim.L-3; //dok�adniejsza d�ugo��, 3m luzu
      if ((scandist<0)?true:dir.x*sem.x+dir.z*sem.z<0)
       scandist=0; //ujemnych nie ma po co wysy�a�, je�li mini�ty, to r�wnie� 0
      if (!TrainParams->IsStop())
      {//je�li nie ma tu postoju
       if (scandist<500)
       {//zaliczamy posterunek w pewnej odleg�o�ci przed, bo W4 zas�ania semafor
#if LOGSTOPS
        WriteLog(edir+AnsiString(GlobalTime->hh)+":"+AnsiString(GlobalTime->mm)+" Skipped stop: "+asNextStop.SubString(20,asNextStop.Length())); //informacja
#endif
        TrainParams->UpdateMTable(GlobalTime->hh,GlobalTime->mm,asNextStop.SubString(20,asNextStop.Length()));
        TrainParams->StationIndexInc(); //przej�cie do nast�pnej
        asNextStop=TrainParams->NextStop(); //pobranie kolejnego miejsca zatrzymania
        eSignSkip=e; //wtedy uznajemy go za ignorowany przy poszukiwaniu nowego
        eSignLast=NULL; //�eby jaki� nowy by� poszukiwany
       }
      } //koniec obs�ugi przelotu na W4
      else
      {//zatrzymanie na W4
       //vmechmax=0.0; //ma stan�� na W4 - informacja dla dalszego kodu
       sl.x=e->Params[3].asdouble; //wyliczenie wsp�rz�dnych zatrzymania
       sl.y=e->Params[4].asdouble;
       sl.z=e->Params[5].asdouble;
       eSignLast=(scandist<200.0)?e:NULL; //zapami�tanie na wypadek przejechania albo �le podpi�tego toru
       if ((scandist>fMinProximityDist)?(Controlling->Vel>0.0):false)
       //if ((scandist>Mechanik->MinProximityDist)?(fSignSpeed>0.0):false)
       {//je�li jedzie, informujemy o zatrzymaniu na wykrytym stopie
        //Mechanik->PutCommand("SetProximityVelocity",scandist,0,sl);
#if LOGVELOCITY
        //WriteLog(edir+"SetProximityVelocity "+AnsiString(scandist)+" 0");
        WriteLog(edir);
#endif
        //SetProximityVelocity(scandist,0,&sl); //staje 300m oe W4
        SetProximityVelocity(scandist,scandist>100.0?30:0.2*scandist,&sl); //Ra: taka proteza
       }
       else //je�li jest blisko, albo stoi
        if ((iDrivigFlags&moveStopCloser)&&(scandist>25.0)) //je�li ma podjechac pod W4, a jest daleko
        {
         //if (pVehicles[0]->fTrackBlock>fDriverDist) //je�eli nie ma zawalidrogi w tej odleg�o�ci
         PutCommand("SetVelocity",scandist>100.0?40:0.4*scandist,0,&sl); //doci�ganie do przystanku
        } //koniec obs�ugi odleg�ego zatrzymania
        else
        {//je�li pierwotnie sta� lub zatrzyma� si� wystarczaj�co blisko
         PutCommand("SetVelocity",0,0,&sl,stopTime); //zatrzymanie na przystanku
#if LOGVELOCITY
         WriteLog(edir+"SetVelocity 0 0 ");
#endif
         if (Controlling->Vel==0.0)
         {//je�li si� zatrzyma� przy W4, albo sta� w momencie zobaczenia W4
          if (Controlling->TrainType==dt_EZT) //otwieranie drzwi w EN57
           if (!Controlling->DoorLeftOpened&&!Controlling->DoorRightOpened)
           {//otwieranie drzwi
            int i=floor(e->Params[2].asdouble); //p7=platform side (1:left, 2:right, 3:both)
            if (iDirection>=0)
            {if (i&1) Controlling->DoorLeft(true);
             if (i&2) Controlling->DoorRight(true);
            }
            else
            {//je�li jedzie do ty�u, to drzwi otwiera odwrotnie
             if (i&1) Controlling->DoorRight(true);
             if (i&2) Controlling->DoorLeft(true);
            }
            if (i&3) //�eby jeszcze poczeka� chwil�, zanim zamknie
             WaitingSet(10); //10 sekund (wzi�� z rozk�adu????)
           }
          if (TrainParams->UpdateMTable(GlobalTime->hh,GlobalTime->mm,asNextStop.SubString(20,asNextStop.Length())))
          {//to si� wykona tylko raz po zatrzymaniu na W4
           if (TrainParams->DirectionChange()) //je�li '@" w rozk�adzie, to wykonanie dalszych komend
           {//wykonanie kolejnej komendy, nie dotyczy ostatniej stacji
            JumpToNextOrder(); //przej�cie do kolejnego rozkazu (zmiana kierunku, odczepianie)
            //if (OrderCurrentGet()==Change_direction) //je�li ma zmieni� kierunek
            iDrivigFlags&=~moveStopCloser; //ma nie podje�d�a� pod W4 po przeciwnej stronie
            if (Controlling->TrainType!=dt_EZT) //
             iDrivigFlags&=~moveStopPoint; //pozwolenie na przejechanie za W4 przed czasem
            eSignLast=NULL; //niech skanuje od nowa, w przeciwnym kierunku
           }
          }
          if (OrderCurrentGet()==Shunt)
          {OrderNext(Obey_train); //uruchomi� jazd� poci�gow�
           CheckVehicles(); //zmieni� �wiat�a
          }
          if (TrainParams->StationIndex<TrainParams->StationCount)
          {//je�li s� dalsze stacje, czekamy do godziny odjazdu
#if LOGVELOCITY
           WriteLog(edir+asNextStop); //informacja o zatrzymaniu na stopie
#endif
           if (iDrivigFlags&moveStopPoint) //je�li pomijanie W4, to nie sprawdza czasu odjazdu
            if (TrainParams->IsTimeToGo(GlobalTime->hh,GlobalTime->mm))
            {//z dalsz� akcj� czekamy do godziny odjazdu
             TrainParams->StationIndexInc(); //przej�cie do nast�pnej
             asNextStop=TrainParams->NextStop(); //pobranie kolejnego miejsca zatrzymania
#if LOGSTOPS
             WriteLog(edir+AnsiString(GlobalTime->hh)+":"+AnsiString(GlobalTime->mm)+" Next stop: "+asNextStop.SubString(20,asNextStop.Length())); //informacja
#endif
             eSignSkip=e; //wtedy uznajemy go za ignorowany przy poszukiwaniu nowego
             eSignLast=NULL; //�eby jaki� nowy by� poszukiwany
             iDrivigFlags|=moveStopCloser; //do nast�pnego W4 podjecha� blisko (z doci�ganiem)
             //vmechmax=vtrackmax; //odjazd po zatrzymaniu - informacja dla dalszego kodu
             vmechmax=-1; //odczytywanie pr�dko�ci z toru ogranicza�o dalsz� jazd�
             PutCommand("SetVelocity",vmechmax,vmechmax,&sl);
#if LOGVELOCITY
             WriteLog(edir+"SetVelocity "+AnsiString(vtrackmax)+" "+AnsiString(vtrackmax));
#endif
            } //koniec startu z zatrzymania
          } //koniec obs�ugi pocz�tkowych stacji
          else
          {//je�li dojechali�my do ko�ca rozk�adu
           asNextStop=TrainParams->NextStop(); //informacja o ko�cu trasy
           eSignSkip=e; //wtedy W4 uznajemy za ignorowany
           eSignLast=NULL; //�eby jaki� nowy sygna� by� poszukiwany
           iDrivigFlags&=~moveStopCloser; //ma nie podje�d�a� pod W4
           WaitingSet(60); //tak ze 2 minuty, a� wszyscy wysi�d�
           JumpToNextOrder(); //wykonanie kolejnego rozkazu (Change_direction albo Shunt)
#if LOGSTOPS
           WriteLog(edir+AnsiString(GlobalTime->hh)+":"+AnsiString(GlobalTime->mm)+" Next stop: "+asNextStop.SubString(20,asNextStop.Length())); //informacja
#endif
          } //koniec obs�ugi ostatniej stacji
         } //if (MoverParameters->Vel==0.0)
        } //koniec obs�ugi W4 z zatrzymaniem
       return;
      } //koniec obs�ugi zatrzymania na W4
     } //koniec obs�ugi PassengerStopPoint
    } //if (e->Type==tp_PutValues...
   } //if (e)
   else //je�li nic nie znaleziono
    if (OrderCurrentGet()==Shunt) //a jest w trybie manewrowym
     eSignSkip=NULL; //przywr�cenie widoczno�ci ewentualnie pomini�tej tarczy
   if (iDrivigFlags&moveBackwardLook) //je�li skanowanie do ty�u
    return; //to nie analizujemy pr�dko�ci w torach
   if (scandist<=scanmax) //je�li ograniczenie jest dalej, ni� skanujemy, mo�na je zignorowa�
    if (vtrackmax>0.0) //je�li w torze jest dodatnia
     if ((vmechmax<0)||(vtrackmax<vmechmax)) //i mniejsza od tej drugiej
     {//tutaj jest wykrywanie ograniczenia pr�dko�ci w torze
      vector3 pos=pVehicles[0]->HeadPosition();
      double dist1=(scantrack->CurrentSegment()->FastGetPoint_0()-pos).Length();
      double dist2=(scantrack->CurrentSegment()->FastGetPoint_1()-pos).Length();
      if (dist2<dist1)
      {//Point2 jest bli�ej i jego wybieramy
       dist1=dist2;
       pos=scantrack->CurrentSegment()->FastGetPoint_1();
      }
      else
       pos=scantrack->CurrentSegment()->FastGetPoint_0();
      sl=pos;
      //Mechanik->PutCommand("SetProximityVelocity",dist1,vtrackmax,sl);
#if LOGVELOCITY
      //WriteLog("Track Velocity: SetProximityVelocity "+AnsiString(dist1)+" "+AnsiString(vtrackmax));
      WriteLog("Track velocity:");
#endif
      SetProximityVelocity(dist1,vtrackmax,&sl);
     } //if ((vmechmax<0)||(vtrackmax<vmechmax))
  } //if (scantrack)
 } //if (scandir!=0.0)
};

AnsiString __fastcall TController::NextStop()
{//informav\cja o nast�pnym zatrzymaniu
 if (asNextStop.IsEmpty()) return "";
 //doda� godzin� odjazdu
 return asNextStop.SubString(20,30);//+" "+AnsiString(
};

//-----------koniec skanowania semaforow

void __fastcall TController::TakeControl(bool yes)
{//przej�cie kontroli przez AI albo oddanie
 if (AIControllFlag==yes) return; //ju� jest jak ma by�
 if (yes) //�eby nie wykonywa� dwa razy
 {//teraz AI prowadzi
  AIControllFlag=AIdriver;
  pVehicle->Controller=AIdriver;
  if (OrderCurrentGet())
  {if (OrderCurrentGet()<Shunt)
   {OrderNext(Prepare_engine);
    if (pVehicle->iLights[Controlling->CabNo<0?1:0]&4) //g�rne �wiat�o
     OrderNext(Obey_train); //jazda poci�gowa
    else
     OrderNext(Shunt); //jazda manewrowa
   }
  }
  else //je�li jest w stanie Wait_for_orders
   JumpToFirstOrder(); //uruchomienie?
  // czy dac ponizsze? to problematyczne
  //SetVelocity(pVehicle->GetVelocity(),-1); //utrzymanie dotychczasowej?
  if (pVehicle->GetVelocity()>0.0)
   SetVelocity(-1,-1); //AI ustali sobie odpowiedni� pr�dko��
  CheckVehicles(); //ustawienie �wiate�
 }
 else
 {//a teraz u�ytkownik
  AIControllFlag=Humandriver;
  pVehicle->Controller=Humandriver;
 }
};

void __fastcall TController::DirectionForward(bool forward)
{//ustawienie jazdy w kierunku sprz�gu 0 dla true i 1 dla false
 if (forward)
  while (Controlling->ActiveDir<=0)
   Controlling->DirectionForward();
 else
  while (Controlling->ActiveDir>=0)
   Controlling->DirectionBackward();
};

AnsiString __fastcall TController::Relation()
{//zwraca relacj� poci�gu
 return TrainParams->ShowRelation();
};

AnsiString __fastcall TController::TrainName()
{//zwraca relacj� poci�gu
 return TrainParams->TrainName;
};

double __fastcall TController::StationCount()
{//zwraca ilo�� stacji (miejsc zatrzymania)
 return TrainParams->StationCount;
};

double __fastcall TController::StationIndex()
{//zwraca indeks aktualnej stacji (miejsca zatrzymania)
 return TrainParams->StationIndex;
};


