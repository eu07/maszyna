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

Modu³ obs³uguj¹cy sterowanie pojazdami (sk³adami poci¹gów, samochodami).
Ma dzia³aæ zarówno jako AI oraz przy prowadzeniu przez cz³owieka. W tym
drugim przypadku jedynie informuje za pomoc¹ napisów o tym, co by zrobi³
w tym pierwszym. Obejmuje zarówno maszynistê jak i kierownika poci¹gu
(dawanie sygna³u do odjazdu).

Przeniesiona tutaj zosta³a zawartoœæ ai_driver.pas przerobione na C++.
Równie¿ niektóre funkcje dotycz¹ce sk³adów z DynObj.cpp.

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
//16. zmiana kierunku //Ra: z przesiadk¹ po ukrotnieniu
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

//sta³e
const double EasyReactionTime=0.4;
const double HardReactionTime=0.2;
const double EasyAcceleration=0.5;
const double HardAcceleration=0.9;
const double PrepareTime     =2.0;
bool WriteLogFlag=false;

AnsiString StopReasonTable[]=
{//przyczyny zatrzymania ruchu AI
 "", //stopNone, //nie ma powodu - powinien jechaæ
 "Off", //stopSleep, //nie zosta³ odpalony, to nie pojedzie
 "Semaphore", //stopSem, //semafor zamkniêty
 "Time", //stopTime, //czekanie na godzinê odjazdu
 "End of track", //stopEnd, //brak dalszej czêœci toru
 "Change direction", //stopDir, //trzeba stan¹æ, by zmieniæ kierunek jazdy
 "Joining", //stopJoin, //zatrzymanie przy (p)odczepianiu
 "Block", //stopBlock, //przeszkoda na drodze ruchu
 "A command", //stopComm, //otrzymano tak¹ komendê (niewiadomego pochodzenia)
 "Out of station", //stopOut, //komenda wyjazdu poza stacjê (raczej nie powinna zatrzymywaæ!)
 "Radiostop", //stopRadio, //komunikat przekazany radiem (Radiostop)
 "External", //stopExt, //przes³any z zewn¹trz
 "Error", //stopError //z powodu b³êdu w obliczeniu drogi hamowania
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

void __fastcall TSpeedPos::Clear()
{
 iFlags=0; //brak flag to brak reakcji
 fVelNext=-1.0; //prêdkoœæ bez ograniczeñ
 fDist=0.0;
 vPos=vector3(0,0,0);
 tTrack=NULL; //brak wskaŸnika
};

void __fastcall TSpeedPos::Update(vector3 *p,vector3 *dir,double len)
{//przeliczenie odleg³oœci od pojazdu w punkcie (*p), jad¹cego w kierunku (*d)
 vector3 v=vPos-*p; //wektor do punktu zmiany
 //v.y=0.0; //liczymy odleg³oœæ bez uwzglêdnienia wysokoœci
 fDist=v.Length(); //d³ugoœæ wektora to odleg³oœæ pomiêdzy czo³em a sygna³em albo pocz¹tkiem toru
 //v.SafeNormalize(); //normalizacja w celu okreœlenia znaku (nie potrzebna?)
 double iska=dir->x*v.x+dir->z*v.z; //iloczyn skalarny to rzut na chwilow¹ prost¹ ruchu
 if (iska<0.0) //iloczyn skalarny jest ujemny, gdy punkt jest z ty³u
 {//jeœli coœ jest z ty³u, to dok³adna odleg³oœæ nie ma ju¿ wiêkszego znaczenia
  fDist=-fDist; //potrzebne do badania wyjechania sk³adem poza ograniczenie
  if (iFlags&32) //32+64?
  {//jeœli miniêty (musi byæ miniêty równie¿ przez koñcówkê sk³adu)

  }
  else
  {iFlags^=32; //32-miniêty - bêdziemy liczyæ odleg³oœæ wzglêdem przeciwnego koñca toru (nadal mo¿e byæ z przodu i ogdaniczaæ)
   if ((iFlags&67)==3) //tylko jeœli (istotny) tor, bo eventy s¹ punktowe
    if (tTrack) //mo¿e byæ NULL, jeœli koniec toru (????)
     vPos=(iFlags&4)?tTrack->CurrentSegment()->FastGetPoint_0():tTrack->CurrentSegment()->FastGetPoint_1(); //drugi koniec istotny
  }
 }
 else if (fDist<50.0)
 {//przy du¿ym k¹cie ³uku iloczyn skalarny bardziej zani¿y odleg³oœæ ni¿ ciêciwa
  fDist=iska; //ale przy ma³ych odleg³oœciach rzut na chwilow¹ prost¹ ruchu da dok³adniejsze wartoœci
 }
 if (iFlags&2) //jeœli tor
 {
  if (tTrack) //mo¿e byæ NULL, jeœli koniec toru
  {if (iFlags&8) //jeœli odcinek zmienny
    if (bool(tTrack->GetSwitchState()&1)!=bool(iFlags&16)) //czy stan siê zmieni³?
    {//Ra: zak³adam, ¿e s¹ tylko 2 mo¿liwe stany
     iFlags^=16;
     fVelNext=tTrack->fVelocity; //nowa prêdkoœæ
     //jeszcze trzeba skanowanie wykonaæ od tego toru 
    }
  }
  WriteLog("-> Dist="+FloatToStrF(fDist,ffFixed,7,1)+", Track="+tTrack->NameGet()+", Vel="+AnsiString(fVelNext)+", Flags="+AnsiString(iFlags));
 }
 else if (iFlags&0x100) //jeœli event
 {//odczyt komórki pamiêci najlepiej by by³o zrobiæ jako notyfikacjê, czyli zmiana komórki wywo³a jak¹œ podan¹ funkcjê
  AnsiString command=eEvent->CommandGet();
  double value1=eEvent->ValueGet(1);
  double value2=eEvent->ValueGet(2);
  if (command=="ShuntVelocity")
  {//prêdkoœæ manewrow¹ zapisaæ, najwy¿ej AI zignoruje przy analizie tabelki
   fVelNext=value1;
   iFlags|=0x200;
  }
  else if (command=="SetVelocity")
  {//w semaforze typu "m" jest ShuntVelocity dla jazdy manewrowej i SetVelocity dla S1
   fVelNext=value1;
   iFlags&=~0xE00; //nie manewrowa, nie przystanek, nie zatrzymaæ na SBL
   if (value1==0.0)
    if (value2!=0.0)
    {//S1 na SBL mo¿na przejechaæ po zatrzymaniu
     fVelNext=value2;
     iFlags|=0x800; //na pewno nie zezwoli na manewry
    }
  }
  else if (command.SubString(1,19)=="PassengerStopPoint:")
  {//przystanek, najwy¿ej AI zignoruje przy analizie tabelki
   //jeœli to przystanek, to zignorowaæ po up³ywie czasu, wczeœniej sprawdziæ, czy siê pojazd zatrzyma³...
   iFlags|=0x400;
  }
  //fVelNext=tTrack->fVelocity; //nowa prêdkoœæ
  WriteLog("-> Dist="+FloatToStrF(fDist,ffFixed,7,1)+", Event="+eEvent->asName+", Vel="+AnsiString(fVelNext)+", Flags="+AnsiString(iFlags));
 }
};

bool __fastcall TSpeedPos::Set(TEvent *e,double d)
{//zapamiêtanie zdarzenia
 fDist=d;
 iFlags=0x101; //event+istotny
 eEvent=e;
 vPos=e->PositionGet(); //wspó³rzêdne eventu albo komórki pamiêci (zrzutowaæ na tor?)
 return false; //true gdy zatrzymanie, wtedy nie ma po co skanowaæ dalej
};

void __fastcall TSpeedPos::Set(TTrack *t,double d,int f)
{//zapamiêtanie zmiany prêdkoœci w torze
 fDist=d; //odleg³oœæ do pocz¹tku toru
 tTrack=t; //TODO: (t) mo¿e byæ NULL i nie odczytamy koñca poprzedniego :/
 if (tTrack)
 {iFlags=f|(tTrack->eType==tt_Normal?2:10); //zapamiêtanie kierunku wraz z typem
  fVelNext=tTrack->fVelocity;
  if (tTrack->iDamageFlag&128) fVelNext=0.0; //jeœli uszkodzony, to te¿ stój
  if (iFlags&64) fVelNext=0.0; //jeœli koniec, to te¿ stój
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
 for (int i=0;i<16;++i) //czyszczenie tabeli prêdkoœci
  sSpeedTable[i].Clear();
 tLast=NULL;
};

__fastcall TSpeedTable::~TSpeedTable()
{
};

bool __fastcall TSpeedTable::CheckEvent(TEvent *e,bool prox)
{//sprawdzanie eventu, czy jest obs³ugiwany poza kolejk¹
 //prox=true, gdy sprawdzanie, czy dodaæ event do kolejki
 //-> zwraca true, jeœli event istotny dla AI
 //prox=false, gdy wyszukiwanie semafora przez AI
 //-> zwraca false, gdy event ma byæ dodany do kolejki
 if (e)
 {if (!prox) if (e==eSignSkip) return false; //semafor przejechany jest ignorowany
  AnsiString command;
  switch (e->Type)
  {//to siê wykonuje równie¿ sk³adu jad¹cego bez obs³ugi
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
    return false; //inne eventy siê nie licz¹
  }
  if (prox)
   if (command=="SetProximityVelocity")
    return true; //ten event z toru ma byæ ignorowany
  if (command=="SetVelocity")
   return true; //jak podamy wyjazd, to prze³¹czy siê w jazdê poci¹gow¹
  if (prox) //pomijanie punktów zatrzymania
  {if (command.SubString(1,19)=="PassengerStopPoint:")
    return true;
  }
  else //nazwa stacji pobrana z rozk³adu - nie ma co sprawdzaæ raz po raz
   if (!asNextStop.IsEmpty())
    if (command.SubString(1,asNextStop.Length())==asNextStop)
     return true;
 }
 return false;
};

TEvent* __fastcall TSpeedTable::CheckTrackEvent(double fDirection,TTrack *Track)
{//sprawdzanie eventów na podanym torze
 //ZiomalCl: teraz zwracany jest pierwszy event podajacy predkosc dla AI
 //a nie kazdy najblizszy event [AI sie gubilo gdy przed getval z SetVelocity
 //mialo np. PutValues z eventem od SHP]
 TEvent* e=(fDirection>0)?Track->Event2:Track->Event1;
 return CheckEvent(e,false)?e:NULL; //sprawdzenie z pominiêciem niepotrzebnych
};

bool __fastcall TSpeedTable::AddNew()
{//zwiêkszenie u¿ytej tabelki o jeden rekord
 iLast=(iLast+1)%16;
 //TODO: jeszcze sprawdziæ, czy siê na iFirst nie na³o¿y
 return true; //false gdy siê na³o¿y
};

void __fastcall TSpeedTable::TraceRoute(double fDistance,int iDir,TDynamicObject *pVehicle)
{//skanowanie trajektorii na odleg³oœæ (fDistance) w kierunku (iDir) i uzupe³nianie tabelki
 if (!iDir)
 {//jeœli kierunek jazdy nie jest okreslony
  iDirection=0; //czekamy na ustawienie kierunku
 }
 TTrack *pTrack; //zaczynamy od ostatniego analizowanego toru
 //double fDistChVel=-1; //odleg³oœæ do toru ze zmian¹ prêdkoœci
 double fLength; //d³ugoœæ aktualnego toru (krótsza dla pierwszego)
 double fCurrentDistance; //aktualna przeskanowana d³ugoœæ
 TEvent *pEvent;
 if (iDirection!=iDir)
 {//jeœli zmiana kierunku, zaczynamy od toru z pierwszym pojazdem
  pTrack=pVehicle->RaTrackGet();
  fLastDir=iDir*pVehicle->RaDirectionGet(); //ustalenie kierunku skanowania na torze
  fCurrentDistance=0; //na razie nic nie przeskanowano
  fLength=pVehicle->RaTranslationGet(); //pozycja na tym torze (odleg³oœæ od Point1)
  if (fLastDir>0) //jeœli w kierunku Point2 toru
   fLength=pTrack->Length()-fLength; //przeskanowana zostanie odleg³oœæ do Point2
  fLastVel=pTrack->fVelocity; //aktualna prêdkoœæ
  iDirection=iDir; //ustalenie w jakim kierunku jest wype³niana tabelka wzglêdem pojazdu
  iFirst=iLast=0;
  tLast=NULL; //¿aden nie sprawdzony
 }
 else
 {//kontynuacja skanowania od ostatnio sprawdzonego toru (w ostatniej pozycji zawsze jest tor)
  pTrack=sSpeedTable[iLast].tTrack; //ostatnio sprawdzony tor
  if (!pTrack) return; //koniec toru, to nie ma co sprawdzaæ
  fLastDir=sSpeedTable[iLast].iFlags&4?-1.0:1.0; //flaga ustawiona, gdy Point2 toru jest bli¿ej
  fCurrentDistance=sSpeedTable[iLast].fDist; //aktualna odleg³oœæ do jego Point1
  fLength=sSpeedTable[iLast].iFlags&96?0.0:pTrack->Length(); //nie doliczaæ d³ugoœci gdy: 32-miniêty pocz¹tek, 64-jazda do koñca toru
 }
 if (fCurrentDistance<fDistance)
 {//jeœli w ogóle jest po co analizowaæ
  //TTrack *pTrackFrom; //odcinek poprzedni, do znajdywania œlepego koñca
  --iLast; //jak coœ siê znajdzie, zostanie wpisane w tê pozycjê, któr¹ odczytano
  while (fCurrentDistance<fDistance)
  {
   if (pTrack!=tLast)
   {//jeœli tor nie by³ jeszcze sprawdzany
    if (pTrack) //zawsze chyba jest
     if ((pTrack->fVelocity==0.0)||(pTrack->iDamageFlag&128)||(pTrack->eType!=tt_Normal)||(pTrack->fVelocity!=fLastVel))
     {//gdy zerowa prêdkoœæ albo uszkadza pojazd albo ruchomy tor albo prêdkosæ w kolejnym ró¿na od aktualnej
      if (AddNew())
       sSpeedTable[iLast].Set(pTrack,fCurrentDistance,fLastDir<0?5:1); //dodanie zmiany prêdkoœci
     }
    if ((pEvent=CheckTrackEvent(fLastDir,pTrack))!=NULL) //jeœli jest semafor na tym torze
     if (AddNew())
     {sSpeedTable[iLast].Set(pEvent,fCurrentDistance); //dodanie odczytu sygna³u
      //jeœli sygna³ stop, to nie ma potrzeby dalej sprawdzaæ
      //fDistance=fCurrentDistance;
     }
   }
   fCurrentDistance+=fLength; //doliczenie kolejnego odcinka do przeskanowanej d³ugoœci
   tLast=pTrack; //odhaczenie, ¿e sprawdzony
   //Track->ScannedFlag=true; //do pokazywania przeskanowanych torów
   fLastVel=pTrack->fVelocity; //prêdkoœæ na poprzednio sprawdzonym odcinku
   //pTrackFrom=pTrack; //zapamiêtanie aktualnego odcinka aby rozpoznaæ samozapêtlenie
   if (fLastDir>0)
   {//jeœli szukanie od Point1 w kierunku Point2
    pTrack=pTrack->CurrentNext(); //mo¿e byæ NULL
    if (pTrack) //jeœli dalej brakuje toru, to zostajemy na tym samym, z t¹ sam¹ orientacj¹
     if (tLast->iNextDirection)
      fLastDir=-fLastDir; //mo¿na by zamiêtaæ i zmieniæ tylko jeœli jest pTrack
   }
   else //if (fDirection<0)
   {//jeœli szukanie od Point2 w kierunku Point1
    pTrack=pTrack->CurrentPrev(); //mo¿e byæ NULL
    if (pTrack) //jeœli dalej brakuje toru, to zostajemy na tym samym, z t¹ sam¹ orientacj¹
     if (!tLast->iPrevDirection)
      fLastDir=-fLastDir;
   }
   if (pTrack)
    fLength=pTrack->Length(); //zwiêkszenie skanowanej odleg³oœci tylko jeœli istnieje dalszy tor
   else
   {//definitywny koniec skanowania, chyba ¿e dalej puszczamy samochód po gruncie...
    if (AddNew()) //kolejny, bo siê cofnêliœmy o 1
     sSpeedTable[iLast].Set(tLast,fCurrentDistance,fLastDir<0?69:65); //zapisanie ostatniego sprawdzonego toru
    return; //to ostatnia pozycja, bo NULL nic nie da, a mo¿e siê podpi¹æ obrotnica, czy jakieœ transportery
   }
   //TODO: sprawdziæ jeszcze, czy tabelka siê nie zatka
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
 {//trzeba sprawdziæ, czy coœ siê zmieni³o
  vector3 dir=pVehicle->VectorFront()*pVehicle->DirectionGet(); //wektor kierunku jazdy
  for (int i=iFirst;i!=iLast;i=(i+1)%16)
  {//aktualizacja rekordów z wyj¹tkiem ostatniego
   sSpeedTable[i].Update(&pVehicle->HeadPosition(),&dir,fLength);
   if (sSpeedTable[i].fDist<-fLength) //sk³ad wyjecha³ ca³¹ d³ugoœci¹ poza
   {//degradacja pozycji
    sSpeedTable[i].iFlags&=~1; //nie liczy siê
    iFirst=(iFirst+1)%16; //kolejne sprawdzanie bêdzie ju¿ od nastêpnej pozycji
   }
  }
  sSpeedTable[iLast].Update(&pVehicle->HeadPosition(),&dir,fLength); //aktualizacja ostatniego
  if (sSpeedTable[iLast].fDist<fDistance)
   TraceRoute(fDistance,iDir,pVehicle); //doskanowanie dalszego odcinka
 }
};

void __fastcall TSpeedTable::Update(double fVel,double &fVelDes,double &fDist,double &fNext,double &fAcc)
{//ustalenie parametrów
 //fVel - chwilowa prêdkoœæ pojazdu jako wartoœæ odniesienia
 //fDist - dystans w jakim nale¿y rozwa¿yæ ruch
 //fNext - prêdkoœæ na koñcu tego dystansu
 //fAcc - zalecane przyspieszenie w chwili obecnej - kryterium wyboru dystansu
 double a; //przyspieszenie
 double v; //prêdkoœæ
 double d; //droga
 int i,k=iLast-iFirst+1;
 if (k<0) k+=16; //iloœæ pozycji do przeanalizowania
 for (i=iFirst;k>0;--k,i=(i+1)%16)
 {//sprawdzenie rekordów od (iFirst) do (iLast), o ile s¹ istotne
  if (sSpeedTable[i].iFlags&1) //badanie istotnoœci
  {//o ile dana pozycja tabelki jest istotna
   v=sSpeedTable[i].fVelNext;
   if (v>=0.0)
   {//pozycje z prêdkoœci¹ -1 mo¿na spokojnie pomijaæ
    d=sSpeedTable[i].fDist;
    if (d>0.0) //sygna³ lub ograniczenie z przodu
     a=(v*v-fVel*fVel)/(25.92*d); //przyspieszenie: ujemne, gdy trzeba hamowaæ
    else
     if (sSpeedTable[i].iFlags&2) //jeœli tor
     {//tor ogranicza prêdkoœæ, dopóki ca³y sk³ad nie przejedzie,
      d=fLength+d; //zamiana na d³ugoœæ liczon¹ do przodu
      if (d<0.0) continue; //zapêtlenie, jeœli ju¿ wyjecha³ za ten odcinek
      if (v<fVelDes) fVelDes=v; //ograniczenie aktualnej prêdkoœci a¿ do wyjechania za ograniczenie
      //if (v==0.0) fAcc=-0.9; //hamowanie jeœli stop
      continue; //i tyle wystarczy
     }
     else //event trzyma tylko jeœli VelNext=0, nawet po przejechaniu (nie powinno dotyczyæ samochodów?)
      a=(v==0.0?-1.0:fAcc); //ruszanie albo hamowanie
    if (a<fAcc)
    {//mniejsze przyspieszenie to mniejsza mo¿liwoœæ rozpêdzenia siê albo koniecznoœæ hamowania
     //jeœli droga wolna, to mo¿e byæ a>1.0 i siê tu nie za³apuje
     fAcc=a; //zalecane przyspieszenie (nie musi byæ uwzglêdniane przez AI)
     fNext=v; //istotna jest prêdkoœæ na koñcu tego odcinka
     fDist=d; //dlugoœæ odcinka
    }
    else if ((fAcc>0)&&(v>0)&&(v<=fNext))
    {//jeœli nie ma wskazañ do hamowania, mo¿na podaæ drogê i prêdkoœæ na jej koñcu
     fNext=v; //istotna jest prêdkoœæ na koñcu tego odcinka
     fDist=d; //dlugoœæ odcinka (kolejne pozycje mog¹ wyd³u¿aæ drogê, jeœli prêdkoœæ jest sta³a)
    }
   } //if (v>=0.0)
  } //if (sSpeedTable[i].iFlags&1)
 } //for
 //if (fAcc>-0.2)
 // if (fAcc<0.2)
 //  fAcc=0.0; //nie bawimy siê w jakieœ delikatne hamowania czy rozpêdzania
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
 VelActual=0.0; //normalnie na pocz¹tku ma staæ, no chyba ¿e jedzie
 VelNext=120.0;
 AIControllFlag=AI;
 pVehicle=NewControll;
 Controlling=pVehicle->MoverParameters; //skrót do obiektu parametrów
 pVehicles[0]=pVehicle->GetFirstDynamic(0); //pierwszy w kierunku jazdy (Np. Pc1)
 pVehicles[1]=pVehicle->GetFirstDynamic(1); //ostatni w kierunku jazdy (koñcówki)
/*
 switch (Controlling->CabNo)
 {
  case -1: SendCtrlBroadcast("CabActivisation",1); break;
  case  1: SendCtrlBroadcast("CabActivisation",2); break;
  default: AIControllFlag:=False; //na wszelki wypadek
 }
*/
 iDirection=0;
 iDirectionOrder=Controlling->CabNo; //1=do przodu (w kierunku sprzêgu 0)
 VehicleName=Controlling->Name;
 TrainParams=NewTrainParams;
 if (TrainParams)
  asNextStop=TrainParams->NextStop();
 else
  TrainParams=new TTrainParameters("none"); //rozk³ad jazdy
 //OrderCommand="";
 //OrderValue=0;
 OrdersClear();
 MaxVelFlag=false; MinVelFlag=false; //Ra: to nie jest u¿ywane
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
 fWarningDuration=0.0; //nic do wytr¹bienia
 WaitingExpireTime=31.0; //tyle ma czekaæ, zanim siê ruszy
 WaitingTime=0.0;
 fMinProximityDist=30.0; //stawanie miêdzy 30 a 60 m przed przeszkod¹
 fMaxProximityDist=50.0;
 iVehicleCount=-2; //wartoœæ neutralna
 Prepare2press=false; //bez dociskania
 eStopReason=stopSleep; //na pocz¹tku œpi
 fLength=0.0;
 eSignSkip=eSignLast=NULL; //miniêty semafor
 fShuntVelocity=40; //domyœlna prêdkoœæ manewrowa
 fStopTime=0.0; //czas postoju przed dalsz¹ jazd¹ (np. na przystanku)
 iDrivigFlags=moveStopPoint; //flagi bitowe ruchu
 Ready=false;
 SetDriverPsyche(); //na koñcu, bo wymaga ustawienia zmiennych
 if (Controlling->CategoryFlag&2)
 {//samochody
  fDriverMass=0.02; //mno¿one przez v^2 [km/h] daje drogê skanowania [m]
  fDriverDist=5.0; //10m - zachowywany odstêp przed kolizj¹
 }
 else
 {//poci¹gi i statki
  fDriverMass=0.1; //mno¿one przez v^2 [km/h] daje drogê skanowania [m]
  fDriverDist=50.0; //50m - zachowywany odstêp przed kolizj¹
 }
 fVelMax=-1; //ustalenie prêdkoœci dla sk³adu
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
/* Ra: wersja ze switch nie dzia³a prawid³owo (czemu?)
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
{//pobranie aktualnego rozkazu celem wyœwietlenia
 return AnsiString(OrderPos)+". "+Order2Str(OrderList[OrderPos]);
};

void __fastcall TController::OrdersClear()
{//czyszczenie tabeli rozkazów na starcie albo po dojœciu do koñca
 OrderPos=0;
 OrderTop=1; //szczyt stosu rozkazów
 for (int b=0;b<maxorders;b++)
  OrderList[b]=Wait_for_orders;
};

void __fastcall TController::Activation()
{//umieszczenie obsady w odpowiednim cz³onie
 iDirection=iDirectionOrder; //kierunek w³aœnie zosta³ ustalony (zmieniony)
 if (iDirection)
 {//jeœli jest ustalony kierunek
  TDynamicObject *d=pVehicle; //w tym siedzi AI
  int brake=Controlling->LocalBrake;
  if (TestFlag(d->MoverParameters->Couplers[iDirectionOrder*d->DirectionGet()<0?1:0].CouplingFlag,ctrain_controll))
  {Controlling->MainSwitch(false); //dezaktywacja czuwaka, jeœli przejœcie do innego cz³onu
   Controlling->DecLocalBrakeLevel(10); //zwolnienie hamulca w opuszczanym pojeŸdzie
  }
  Controlling->CabDeactivisation(); //tak jest w Train.cpp
  //przejœcie AI na drug¹ stronê EN57, ET41 itp.
  while (TestFlag(d->MoverParameters->Couplers[iDirectionOrder*d->DirectionGet()<0?1:0].CouplingFlag,ctrain_controll))
  {//jeœli pojazd z przodu jest ukrotniony, to przechodzimy do niego
   d=iDirectionOrder<0?d->Next():d->Prev(); //przechodzimy do nastêpnego cz³onu
   if (d?!d->Mechanik:false)
   {d->Mechanik=this; //na razie bilokacja
    if (d->DirectionGet()!=pVehicle->DirectionGet()) //jeœli s¹ przeciwne do siebie
     iDirection=-iDirection; //to bêdziemy jechaæ w drug¹ stronê wzglêdem zasiedzianego pojazdu
    pVehicle->Mechanik=NULL; //tam ju¿ nikogo nie ma
    pVehicle->MoverParameters->CabNo=0; //wy³¹czanie kabin po drodze
    //pVehicle->MoverParameters->ActiveCab=0;
    //pVehicle->MoverParameters->DirAbsolute=pVehicle->MoverParameters->ActiveDir*pVehicle->MoverParameters->CabNo;
    pVehicle=d; //a mechu ma nowy pojazd (no, cz³on)
   }
   else break; //jak zajête, albo koniec sk³adu, to mechanik dalej nie idzie (wywaliæ drugiego?)
  }
  Controlling=pVehicle->MoverParameters; //skrót do obiektu parametrów, mo¿e byæ nowy
  //Ra: to prze³¹czanie poni¿ej jest tu bez sensu
  Controlling->ActiveCab=iDirection; //aktywacja kabiny w prowadzonym poje¿dzie
  //Controlling->CabNo=iDirection;
  //Controlling->ActiveDir=0; //¿eby sam ustawi³ kierunek
  Controlling->CabActivisation(); //uruchomienie kabin w cz³onach
  if (AIControllFlag) //jeœli prowadzi komputer
   if (brake) //hamowanie tylko jeœli by³ wczeœniej zahamowany (bo mo¿liwe, ¿e jedzie!)
    Controlling->IncLocalBrakeLevel(brake); //zahamuj jak wczeœniej
 }
};

bool __fastcall TController::CheckVehicles()
{//sprawdzenie stanu posiadanych pojazdów w sk³adzie i zapalenie œwiate³
 //ZiomalCl: sprawdzanie i zmiana SKP w skladzie prowadzonym przez AI
 TDynamicObject* p; //roboczy wskaŸnik na pojazd
 iVehicles=0; //iloœæ pojazdów w sk³adzie
 int d=iDirection>0?0:1; //kierunek szukania czo³a (numer sprzêgu)
 pVehicles[0]=p=pVehicle->FirstFind(d); //pojazd na czele sk³adu
 //liczenie pojazdów w sk³adzie i ustawianie kierunku
 d=1-d; //a dalej bêdziemy zliczaæ od czo³a do ty³u
 fLength=0.0; //d³ugoœæ sk³adu do badania wyjechania za ograniczenie
 fVelMax=-1; //ustalenie prêdkoœci dla sk³adu
/*
 bool main=true; //czy jest g³ównym steruj¹cym
 int dir=????; //od pierwszego w drug¹ stronê
 while (p)
 {//sprawdzanie, czy jest g³ównym steruj¹cym, ¿eby nie by³o konfliktu
  //kierunek pojazdów w sk³adzie jest ustalany tylko dla glównego steruj¹cego
  if (p->Mechanik) //jeœli ma obsadê
   if (p!=this) //ale chodzi o inny pojazd, ni¿ aktualnie sprawdzaj¹cy
    if (p->Mechanik->iDrivigFlags&movePrimary) //a tamten ma priorytet
     main=false;
  p=p->Neighbour(dir); //pojazd pod³¹czony od wskazanej strony
 }
 p=pVehicle->FirstFind(d);
*/
 while (p)
 {
  if (TrainParams)
   if (p->asDestination.IsEmpty())
    p->asDestination=TrainParams->Relation2; //relacja docelowa, jeœli nie by³o
  if (AIControllFlag) //jeœli prowadzi komputer
   p->RaLightsSet(0,0); //gasimy œwiat³a
  ++iVehicles; //jest jeden pojazd wiêcej
  pVehicles[1]=p; //zapamiêtanie ostatniego
  fLength+=p->MoverParameters->Dim.L; //dodanie d³ugoœci pojazdu
  d=p->DirectionSet(d?1:-1); //zwraca po³o¿enie nastêpnego (1=zgodny,0=odwrócony)
  //1=zgodny: sprzêg 0 od czo³a; 0=odwrócony: sprzêg 1 od czo³a
  if (fVelMax<0?true:p->MoverParameters->Vmax<fVelMax)
   fVelMax=p->MoverParameters->Vmax; //ustalenie maksymalnej prêdkoœci dla sk³adu
  p=p->Next(); //pojazd pod³¹czony od ty³u (licz¹c od czo³a)
 }
 sSpeedTable.fLength=fLength; //zapamiêtanie d³ugoœci sk³adu w celu wykrycia wyjechania za ograniczenie
/* //tabelka z list¹ pojazdów jest na razie nie potrzebna
 if (i!=)
 {delete[] pVehicle
 }
*/
 if (AIControllFlag) //jeœli prowadzi komputer
  if (OrderCurrentGet()==Obey_train) //jeœli jazda poci¹gowa
   Lights(1+4+16,2+32+64); //œwiat³a poci¹gowe (Pc1) i koñcówki (Pc5)
  else if (OrderCurrentGet()&(Shunt|Connect))
   Lights(16,(pVehicles[1]->MoverParameters->ActiveCab)?1:0); //œwiat³a manewrowe (Tb1) na pojeŸdzie z napêdem
  else if (OrderCurrentGet()==Disconnect)
   Lights(16,0); //œwiat³a manewrowe (Tb1) tylko z przodu, aby nie pozostawiæ sk³adu ze œwiat³em
 return true;
}

void __fastcall TController::Lights(int head,int rear)
{//zapalenie œwiate³ w sk³¹dzie
 pVehicles[0]->RaLightsSet(head,-1); //zapalenie przednich w pierwszym
 pVehicles[1]->RaLightsSet(-1,rear); //zapalenie koñcówek w ostatnim
}

int __fastcall TController::OrderDirectionChange(int newdir,TMoverParameters *Vehicle)
{//zmiana kierunku jazdy, niezale¿nie od kabiny
 int testd;
 testd=newdir;
 if (Vehicle->Vel<0.1)
 {
  switch (newdir*Vehicle->CabNo)
  {//DirectionBackward() i DirectionForward() to zmiany wzglêdem kabiny
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
  //if () //tylko jeœli jazda poci¹gowa
  Vehicle->DirectionForward(); //Ra: z przekazaniem do silnikowego
 return (int)VelforDriver;
}

void __fastcall TController::WaitingSet(double Seconds)
{//ustawienie odczekania po zatrzymaniu (ustawienie w trakcie jazdy zatrzyma)
 fStopTime=-Seconds; //ujemna wartoœæ oznacza oczekiwanie (potem >=0.0)
}

void __fastcall TController::SetVelocity(double NewVel,double NewVelNext,TStopReason r)
{//ustawienie nowej prêdkoœci
 WaitingTime=-WaitingExpireTime; //no albo przypisujemy -WaitingExpireTime, albo porównujemy z WaitingExpireTime
 MaxVelFlag=False; //Ra: to nie jest u¿ywane
 MinVelFlag=False; //Ra: to nie jest u¿ywane
 VelActual=NewVel;   //prêdkoœæ zezwolona na aktualnym odcinku
 VelNext=NewVelNext; //prêdkoœæ przy nastêpnym obiekcie
 if ((NewVel>NewVelNext) //jeœli oczekiwana wiêksza ni¿ nastêpna
  || (NewVel<Controlling->Vel)) //albo aktualna jest mniejsza ni¿ aktualna
  fProximityDist=-800.0; //droga hamowania do zmiany prêdkoœci
 else
  fProximityDist=-300.0; //Ra: ujemne wartoœci s¹ ignorowane
 if (NewVel==0.0) //jeœli ma stan¹æ
 {if (r!=stopNone) //a jest powód podany
  eStopReason=r; //to zapamiêtaæ nowy powód
 }
 else
  eStopReason=stopNone; //podana prêdkoœæ, to nie ma powodów do stania
}

bool __fastcall TController::SetProximityVelocity(double NewDist,double NewVelNext)
{//informacja o prêdkoœci w pewnej odleg³oœci
#if !TESTTABLE
 if (NewVelNext==0.0)
  WaitingTime=0.0; //nie trzeba ju¿ czekaæ
 //if ((NewVelNext>=0)&&((VelNext>=0)&&(NewVelNext<VelNext))||(NewVelNext<VelActual))||(VelNext<0))
 {MaxVelFlag=False; //Ra: to nie jest u¿ywane
  MinVelFlag=False; //Ra: to nie jest u¿ywane
  VelNext=NewVelNext;
  fProximityDist=NewDist; //dodatnie: przeliczyæ do punktu; ujemne: wzi¹æ dos³ownie
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
   WaitingExpireTime=1; //tyle ma czekaæ samochód, zanim siê ruszy
  else
   WaitingExpireTime=61; //tyle ma czekaæ, zanim siê ruszy
 }
 else
 {
  ReactionTime=EasyReactionTime; //spokojny
  AccPreferred=EasyAcceleration;
  if (Controlling->CategoryFlag&2)
   WaitingExpireTime=3; //tyle ma czekaæ samochód, zanim siê ruszy
  else
   WaitingExpireTime=65; //tyle ma czekaæ, zanim siê ruszy
 }
 if (Controlling)
 {//with Controlling do
  if (Controlling->MainCtrlPos<3)
   ReactionTime=Controlling->InitialCtrlDelay+ReactionTime;
  if (Controlling->BrakeCtrlPos>1)
   ReactionTime=0.5*ReactionTime;
  if (Controlling->Vel>0.1) //o ile jedziemy
   if (pVehicles[0]->MoverParameters->Couplers[pVehicles[0]->MoverParameters->DirAbsolute>0?0:1].Connected) //a mamy coœ z przodu
    if (Controlling->Couplers[0].CouplingFlag==0) //jeœli to coœ jest pod³¹czone sprzêgiem wirtualnym
    {//wyliczanie optymalnego przyspieszenia do jazdy na widocznoœæ (Ra: na pewno tutaj?)
     double k=pVehicles[0]->MoverParameters->Couplers[pVehicles[0]->MoverParameters->DirAbsolute>0?0:1].Connected->Vel; //prêdkoœæ pojazdu z przodu
     if (k<Controlling->Vel) //porównanie modu³ów prêdkoœci [km/h]
     {if (pVehicles[0]->fTrackBlock<fDriverDist)
       k=-AccPreferred; //hamowanie
      else
      {//jeœli tamten jedzie szybciej, to nie potrzeba modyfikowaæ przyspieszenia
       double d=25.92*(pVehicles[0]->fTrackBlock-maxdist*fabs(Controlling->Vel)-fDriverDist); //bezpieczna odleg³oœæ za poprzednim
       //a=(v2*v2-v1*v1)/(25.92*(d-0.5*v1))
       //(v2*v2-v1*v1)/2 to ró¿nica energii kinetycznych na jednostkê masy
       //jeœli v2=50km/h,v1=60km/h,d=200m => k=(192.9-277.8)/(25.92*(200-0.5*16.7)=-0.0171 [m/s^2]
       //jeœli v2=50km/h,v1=60km/h,d=100m => k=(192.9-277.8)/(25.92*(100-0.5*16.7)=-0.0357 [m/s^2]
       //jeœli v2=50km/h,v1=60km/h,d=50m  => k=(192.9-277.8)/(25.92*( 50-0.5*16.7)=-0.0786 [m/s^2]
       //jeœli v2=50km/h,v1=60km/h,d=25m  => k=(192.9-277.8)/(25.92*( 25-0.5*16.7)=-0.1967 [m/s^2]
       if (d>0) //bo jak ujemne, to zacznie przyspieszaæ, aby siê zderzyæ
        k=(k*k-Controlling->Vel*Controlling->Vel)/d; //energia kinetyczna dzielona przez masê i drogê daje przyspieszenie
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
 {//najpierw ustalamy kierunek, jeœli nie zosta³ ustalony
  if (!iDirection) //jeœli nie ma ustalonego kierunku
   if (Controlling->V==0)
   {//ustalenie kierunku, gdy stoi
    iDirection=Controlling->CabNo; //wg wybranej kabiny
    if (!iDirection) //jeœli nie ma ustalonego kierunku
     if (Controlling->PantFrontVolt||Controlling->PantRearVolt||voltfront||voltrear)
     {if (Controlling->Couplers[1].CouplingFlag==ctrain_virtual) //jeœli z ty³u nie ma nic
       iDirection=-1; //jazda w kierunku sprzêgu 1
      if (Controlling->Couplers[0].CouplingFlag==ctrain_virtual) //jeœli z przodu nie ma nic
       iDirection=1; //jazda w kierunku sprzêgu 0
     }
   }
   else //ustalenie kierunku, gdy jedzie
    if (Controlling->PantFrontVolt||Controlling->PantRearVolt||voltfront||voltrear)
     if (Controlling->V<0) //jedzie do ty³u
      iDirection=-1; //jazda w kierunku sprzêgu 1
     else //jak nie do ty³u, to do przodu
      iDirection=1; //jazda w kierunku sprzêgu 0
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
  if (eStopReason==stopSleep) //jeœli dotychczas spa³
   eStopReason==stopNone; //teraz nie ma powodu do stania
  if (Controlling->Vel>=1.0) //jeœli jedzie
   iDrivigFlags&=~moveAvaken; //pojazd przemieœci³ siê od w³¹czenia
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
{//wy³¹czanie silnika
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
 {//jeœli siê zatrzyma³
  EngineActive=false;
  iDrivigFlags|=moveAvaken; //po w³¹czeniu silnika pojazd nie przemieœci³ siê
  eStopReason=stopSleep; //stoimy z powodu wy³¹czenia
  eSignSkip=NULL; //zapominamy sygna³ do pominiêcia
  eSignLast=NULL; //zapominamy ostatni sygna³
  Lights(0,0); //gasimy œwiat³a
  OrderNext(Wait_for_orders); //¿eby nie próbowa³ coœ robiæ dalej
 }
 return OK;
}

bool __fastcall TController::IncBrake()
{//zwiêkszenie hamowania
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
{//zmniejszenie si³y hamowania
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
     {//jeœli Knorr
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
{//zwiêkszenie prêdkoœci
 bool OK=true;
 if ((Controlling->DoorOpenCtrl==1)&&(Controlling->Vel==0.0)&&(Controlling->DoorLeftOpened||Controlling->DoorRightOpened))  //AI zamyka drzwi przed odjazdem
 {
  if (Controlling->DoorClosureWarning)
   Controlling->DepartureSignal=true; //za³¹cenie bzyczka
  Controlling->DoorLeft(false); //zamykanie drzwi
  Controlling->DoorRight(false);
  //Ra: trzeba by ustawiæ jakiœ czas oczekiwania na zamkniêcie siê drzwi
  WaitingSet(1); //czekanie sekundê, mo¿e trochê d³u¿ej
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
  iDrivigFlags&=~moveAvaken; //pojazd przemieœci³ siê od w³¹czenia
 return OK;
}

bool __fastcall TController::DecSpeed()
{//zmniejszenie prêdkoœci (ale nie hamowanie)
 bool OK=false; //domyœlnie false, aby wysz³o z pêtli while
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
{//odczytuje i wykonuje komendê przekazan¹ lokomotywie
 TCommand *c=&Controlling->CommandIn;
 PutCommand(c->Command,c->Value1,c->Value2,c->Location,stopComm);
 c->Command=""; //usuniêcie obs³u¿onej komendy
}


void __fastcall TController::PutCommand(AnsiString NewCommand,double NewValue1,double NewValue2,const TLocation &NewLocation,TStopReason reason)
{//wys³anie komendy przez event PutValues, jak pojazd ma obsadê, to wysy³a tutaj, a nie do pojazdu bezpoœrednio
 vector3 sl;
 sl.x=-NewLocation.X; //zamiana na wspó³rzêdne scenerii
 sl.z= NewLocation.Y;
 sl.y= NewLocation.Z;
 if (!PutCommand(NewCommand,NewValue1,NewValue2,&sl,reason))
  Controlling->PutCommand(NewCommand,NewValue1,NewValue2,NewLocation);
}


bool __fastcall TController::PutCommand(AnsiString NewCommand,double NewValue1,double NewValue2,const vector3 *NewLocation,TStopReason reason)
{//analiza komendy
 if (NewCommand=="Emergency_brake") //wymuszenie zatrzymania, niezale¿nie kto prowadzi
 {//Ra: no nadal nie jest zbyt piêknie
  SetVelocity(0,0,reason);
  Controlling->PutCommand("Emergency_brake",1.0,1.0,Controlling->Loc);
  return true; //za³atwione
 }
 else if ((NewCommand.Pos("Timetable:")==1)||(NewCommand.Pos("Timetable=")==1))
 {//przypisanie nowego rozk³adu jazdy, równie¿ prowadzonemu przez u¿ytkownika
  NewCommand.Delete(1,10); //zostanie nazwa pliku z rozk³adem
  if (!TrainParams)
   TrainParams=new TTrainParameters(NewCommand); //rozk³ad jazdy
  else
   TrainParams->NewName(NewCommand); //czyœci tabelkê przystanków
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
    TrainParams->StationIndexInc(); //przejœcie do nastêpnej
    asNextStop=TrainParams->NextStop();
   }
  }
  //if (iDirectionOrder==0) //jeœli kierunek ma nieokreœlony
  if (NewValue1!=0.0) //a ma jechaæ
   iDirectionOrder=NewValue1>0?1:-1; //ustalenie kierunku jazdy
  Activation(); //umieszczenie obs³ugi we w³aœciwym cz³onie
  CheckVehicles(); //sprawdzenie sk³adu, AI zapali œwiat³a
  OrdersInit(fabs(NewValue1)); //ustalenie tabelki komend wg rozk³adu oraz prêdkoœci pocz¹tkowej
  //if (NewValue1!=0.0) if (!AIControllFlag) DirectionForward(NewValue1>0.0); //ustawienie nawrotnika u¿ytkownikowi (propaguje siê do cz³onów)
  //if (NewValue1>0)
  // TrainNumber=floor(NewValue1); //i co potem ???
  return true; //za³atwione
 }
 if (AIControllFlag==Humandriver)
  return false; //na razie reakcja na komendy nie jest odpowiednia dla pojazdu prowadzonego rêcznie
 if (NewCommand=="SetVelocity")
 {
  if (NewLocation)
   vCommandLocation=*NewLocation;
  if ((NewValue1!=0.0)&&(OrderList[OrderPos]!=Obey_train))
  {//o ile jazda
   if (!EngineActive)
    OrderNext(Prepare_engine); //trzeba odpaliæ silnik najpierw, œwiat³a ustawi JumpToNextOrder()
   //if (OrderList[OrderPos]!=Obey_train) //jeœli nie poci¹gowa
   OrderNext(Obey_train); //to uruchomiæ jazdê poci¹gow¹ (od razu albo po odpaleniu silnika
   OrderCheck(); //jeœli jazda poci¹gowa teraz, to wykonaæ niezbêdne operacje
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
 {//uruchomienie jazdy manewrowej b¹dŸ zmiana prêdkoœci
  if (NewLocation)
   vCommandLocation=*NewLocation;
  //if (OrderList[OrderPos]=Obey_train) and (NewValue1<>0))
  if (!EngineActive)
   OrderNext(Prepare_engine); //trzeba odpaliæ silnik najpierw
  OrderNext(Shunt); //zamieniamy w aktualnej pozycji, albo dodajey za odpaleniem silnika
  if (NewValue1!=0.0)
  {
   //if (iVehicleCount>=0) WriteLog("Skasowano ilosæ wagonów w ShuntVelocity!");
   iVehicleCount=-2; //wartoœæ neutralna
   CheckVehicles(); //zabraæ to do OrderCheck()
  }
  //dla prêdkoœci ujemnej przestawiæ nawrotnik do ty³u? ale -1=brak ograniczenia !!!!
  //if (Prepare2press) WriteLog("Skasowano docisk w ShuntVelocity!");
  Prepare2press=false; //bez dociskania
  SetVelocity(NewValue1,NewValue2,reason);
  if (fabs(NewValue1)>2.0) //o ile wartoœæ jest sensowna (-1 nie jest konkretn¹ wartoœci¹)
   fShuntVelocity=fabs(NewValue1); //zapamiêtanie obowi¹zuj¹cej prêdkoœci dla manewrów
 }
 else if (NewCommand=="Wait_for_orders")
 {//oczekiwanie; NewValue1 - czas oczekiwania, -1 = na inn¹ komendê
  if (NewValue1>0.0?NewValue1>fStopTime:false)
   fStopTime=NewValue1; //Ra: w³¹czenie czekania bez zmiany komendy
  else
   OrderList[OrderPos]=Wait_for_orders; //czekanie na komendê (albo daæ OrderPos=0)
 }
 else if (NewCommand=="Prepare_engine")
 {//w³¹czenie albo wy³¹czenie silnika (w szerokim sensie)
  if (NewValue1==0.0)
   OrderNext(Release_engine); //wy³¹czyæ silnik (przygotowaæ pojazd do jazdy)
  else if (NewValue1>0.0)
   OrderNext(Prepare_engine); //odpaliæ silnik (wy³¹czyæ wszystko, co siê da)
  //po za³¹czeniu przejdzie do kolejnej komendy, po wy³¹czeniu na Wait_for_orders
 }
 else if (NewCommand=="Change_direction")
 {
  TOrders o=OrderList[OrderPos]; //co robi³ przed zmian¹ kierunku
  if (!EngineActive)
   OrderNext(Prepare_engine); //trzeba odpaliæ silnik najpierw
  if (NewValue1>0.0) iDirectionOrder=1;
  else if (NewValue1<0.0) iDirectionOrder=-1;
  else iDirectionOrder=-iDirection; //zmiana na przeciwny ni¿ obecny
  OrderNext(Change_direction); //zadanie komendy do wykonania
  if (o>=Shunt) //jeœli jazda manewrowa albo poci¹gowa
   OrderNext(o); //to samo robiæ po zmianie
  else if (!o) //jeœli wczeœniej by³o czekanie
   OrderNext(Shunt); //to dalej jazda manewrowa
  //Change_direction wykona siê samo i nastêpnie przejdzie do kolejnej komendy
 }
 else if (NewCommand=="Obey_train")
 {
  if (!EngineActive)
   OrderNext(Prepare_engine); //trzeba odpaliæ silnik najpierw
  OrderNext(Obey_train);
  //if (NewValue1>0) TrainNumber=floor(NewValue1); //i co potem ???
  OrderCheck(); //jeœli jazda poci¹gowa teraz, to wykonaæ niezbêdne operacje
 }
 else if (NewCommand=="Shunt")
 {//NewValue1 - iloœæ wagonów (-1=wszystkie); NewValue2: 0=odczep, 1..63=do³¹cz, -1=bez zmian
  //-2,-y - pod³¹czyæ do ca³ego stoj¹cego sk³adu (sprzêgiem y>=1), zmieniæ kierunek i czekaæ
  //-2, y - pod³¹czyæ do ca³ego stoj¹cego sk³adu (sprzêgiem y>=1) i czekaæ
  //-1,-y - pod³¹czyæ do ca³ego stoj¹cego sk³adu (sprzêgiem y>=1) i jechaæ w powrotn¹ stronê
  //-1, y - pod³¹czyæ do ca³ego stoj¹cego sk³adu (sprzêgiem y>=1) i jechaæ dalej
  //-1, 0 - tryb manewrowy bez zmian (odczepianie z pozostawieniem wagonów nie ma sensu)
  // 0, 0 - odczepienie lokomotywy
  // 1, y - pod³¹czyæ do pierwszego wagonu w sk³adzie (sprzêgiem y>=1) i odczepiæ go od reszty
  // 1, 0 - odczepienie lokomotywy z jednym wagonem
  if (!EngineActive)
   OrderNext(Prepare_engine); //trzeba odpaliæ silnik najpierw
  if (NewValue2!=0) //jeœli podany jest sprzêg
  {iCoupler=floor(fabs(NewValue2)); //jakim sprzêgiem
   OrderNext(Connect); //po³¹cz (NewValue1) wagonów
   if (NewValue2<0.0) //jeœli sprzêg ujemny, to zmiana kierunku
    OrderNext(Change_direction);
  }
  else if (NewValue2==0.0)
   if (NewValue1>=0.0) //jeœli iloœæ wagonów inna ni¿ wszystkie
    OrderNext(Disconnect); //odczep (NewValue1) wagonów
  if (NewValue1>=-1.0) //jeœli nie -2
   OrderNext(Shunt); //to potem manewruj dalej
  CheckVehicles(); //sprawdziæ œwiat³a
  //if ((iVehicleCount>=0)&&(NewValue1<0)) WriteLog("Skasowano ilosæ wagonów w Shunt!");
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
   fWarningDuration=NewValue1; //czas tr¹bienia
   Controlling->WarningSignal=(NewValue2>1)?2:1; //wysokoœæ tonu
  }
 }
 else if (NewCommand=="OutsideStation") //wskaznik W5
 {
  if (OrderList[OrderPos]==Obey_train)
   SetVelocity(NewValue1,NewValue2,stopOut); //koniec stacji - predkosc szlakowa
  else //manewry - zawracaj
  {
   iDirectionOrder=-iDirection; //zmiana na przeciwny ni¿ obecny
   OrderNext(Change_direction); //zmiana kierunku
   OrderNext(Shunt); //a dalej manewry
  }
 }
 else return false; //nierozpoznana - wys³aæ bezpoœrednio do pojazdu
 return true; //komenda zosta³a przetworzona
};

const TDimension SignalDim={1,1,1};

bool __fastcall TController::UpdateSituation(double dt)
{//uruchamiaæ przynajmniej raz na sekundê
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
 //ABu-160305 Testowanie gotowoœci do jazdy
 //Ra: przeniesione z DynObj, sk³ad u¿ytkownika te¿ jest testowany, ¿eby mu przekazaæ, ¿e ma odhamowaæ
 TDynamicObject* p=pVehicles[0]; //pojazd na czole sk³adu
 Ready=true; //wstêpnie gotowy
 while (p)
 {//sprawdzenie odhamowania wszystkich po³¹czonych pojazdów
  if (p->MoverParameters->BrakePress>=0.03*p->MoverParameters->MaxBrakePress)
  {Ready=false; //nie gotowy
   break; //dalej nie ma co sprawdzaæ
  }
  p=p->Next(); //pojazd pod³¹czony z ty³u (patrz¹c od czo³a)
 }
 HelpMeFlag=false;
 //Winger 020304
 if (Controlling->Vel>0.0)
 {//je¿eli jedzie
  //przy prowadzeniu samochodu trzeba ka¿d¹ oœ odsuwaæ oddzielnie, inaczej kicha wychodzi
  if (Controlling->CategoryFlag&2) //jeœli samochód
   //if (fabs(Controlling->OffsetTrackH)<Controlling->Dim.W) //Ra: szerokoœæ drogi tu powinna byæ?
    if (!Controlling->ChangeOffsetH(-0.01*Controlling->Vel*dt)) //ruch w poprzek drogi
     Controlling->ChangeOffsetH(0.01*Controlling->Vel*dt); //Ra: co to mia³o byæ, to nie wiem
  if (Controlling->EnginePowerSource.SourceType==CurrentCollector)
  {if (AIControllFlag)
   {Controlling->PantRear(true); //jazda na tylnym
    if (Controlling->Vel>30) //opuszczenie przedniego po rozpêdzeniu siê
    //if (Controlling->EnginePowerSource.SourceType==CurrentCollector)
    //if (AIControllFlag)
     Controlling->PantFront(false);
   }
  }
 }
 ElapsedTime+=dt;
 WaitingTime+=dt;
 fStopTime+=dt; //zliczanie czasu postoju, nie ruszy dopóki ujemne
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
  //Ra: nie wiem czemu ReactionTime potrafi dostaæ 12 sekund, to jest przegiêcie, bo prze¿yna STÓJ
  //yB: otó¿ jest to jedna trzecia czasu nape³niania na towarowym; mo¿e siê przydaæ przy wdra¿aniu hamowania, ¿eby nie rusza³o kranem jak g³upie
  //Ra: ale nie mo¿e siê budziæ co pó³ minuty, bo prze¿yna semafory
  if ((LastReactionTime>Min0R(ReactionTime,2.0)))
  {//Ra: trzeba by tak:
#if TESTTABLE
   // 1. Ustaliæ istotn¹ odleg³oœæ zainteresowania (np. 3×droga hamowania z V.max).
   double scanmax=(Controlling->Vel>0.0)?3*fDriverDist+fDriverMass*Controlling->Vel*Controlling->Vel:500; //fabs(Mechanik->ProximityDist);
   // 2. Sprawdziæ, czy tabelka pokrywa za³o¿ony odcinek (nie musi, jeœli jest STOP).
   // 3. Sprawdziæ, czy trajektoria ruchu przechodzi przez zwrotnice - jeœli tak, to sprawdziæ, czy stan siê nie zmieni³.
   // 4. Ewentualnie uzupe³niæ tabelkê informacjami o sygna³ach i ograniczeniach, jeœli siê "zu¿y³a".
   WriteLog("");
   WriteLog("Scan table:");
   sSpeedTable.Check(scanmax,iDirection,pVehicles[0]); //wype³nianie tabelki i aktualizacja odleg³oœci
   // 5. Sprawdziæ stany sygnalizacji zapisanej w tabelce, wyznaczyæ prêdkoœci.
   // 6. Z tabelki wyznaczyæ krytyczn¹ odleg³oœæ i prêdkoœæ (najmniejsze przyspieszenie).
   // 7. Jeœli jest inny pojazd z przodu, ewentualnie skorygowaæ odleg³oœæ i prêdkoœæ.
   // 8. Ustaliæ czêstotliwoœæ œwiadomoœci AI (zatrzymanie precyzyjne - czêœciej, brak atrakcji - rzadziej).
#else
   vector3 vMechLoc=pVehicles[0]->HeadPosition(); //uwzglednic potem polozenie kabiny
   if (fProximityDist>=0) //przeliczyæ od zapamiêtanego punktu
    ActualProximityDist=Min0R(fProximityDist,hypot(vMechLoc.x-vCommandLocation.x,vMechLoc.z-vCommandLocation.z)-0.5*(Controlling->Dim.L+SignalDim.L));
   else
    if (fProximityDist<0)
     ActualProximityDist=fProximityDist; //odleg³oœæ ujemna podana bezpoœrednio (powinno byæ ...=-...)
#endif
   if (Controlling->CommandIn.Command!="")
    if (!Controlling->RunInternalCommand()) //rozpoznaj komende bo lokomotywa jej nie rozpoznaje
     RecognizeCommand(); //samo czyta komendê wstawion¹ do pojazdu?
   if (Controlling->SecuritySystem.Status>1)
    if (!Controlling->SecuritySystemReset())
     if (TestFlag(Controlling->SecuritySystem.Status,s_ebrake)&&(Controlling->BrakeCtrlPos==0)&&(AccDesired>0.0))
      Controlling->DecBrakeLevel();
   switch (OrderList[OrderPos])
   {//ustalenie prêdkoœci przy doczepianiu i odczepianiu, dystansów w pozosta³ych przypadkach
    case Connect: //pod³¹czanie do sk³adu
    {//sprzêgi sprawdzamy w pierwszej kolejnoœci, bo jak po³¹czony, to koniec
     fMinProximityDist=-0.01; fMaxProximityDist=0.0; //[m] dojechaæ maksymalnie
     bool ok=false;
     if (pVehicles[0]->MoverParameters->DirAbsolute>0) //jeœli sprzêg 0
     {//sprzêg 0 - próba podczepienia
      if (iDrivigFlags&moveConnect)
       if (pVehicles[0]->MoverParameters->Couplers[0].Connected) //jeœli jest coœ wykryte (a chyba jest, nie?)
        if (pVehicles[0]->MoverParameters->Attach(0,2,pVehicles[0]->MoverParameters->Couplers[0].Connected,iCoupler))
        {
         //dsbCouplerAttach->SetVolume(DSBVOLUME_MAX);
         //dsbCouplerAttach->Play(0,0,0);
        }
      //WriteLog("CoupleDist[0]="+AnsiString(pVehicles[0]->MoverParameters->Couplers[0].CoupleDist)+", Connected[0]="+AnsiString(pVehicles[0]->MoverParameters->Couplers[0].CouplingFlag));
      if (pVehicles[0]->MoverParameters->Couplers[0].CouplingFlag==iCoupler) //uda³o siê? (mog³o czêœciowo)
       ok=true; //zaczepiony zgodnie z ¿yczeniem!
     }
     else if (pVehicles[0]->MoverParameters->DirAbsolute<0) //jeœli sprzêg 1
     {//sprzêg 1 - próba podczepienia
      if (iDrivigFlags&moveConnect)
       if (pVehicles[0]->MoverParameters->Couplers[1].Connected) //jeœli jest coœ wykryte (a chyba jest, nie?)
        if (pVehicles[0]->MoverParameters->Attach(1,2,pVehicles[0]->MoverParameters->Couplers[1].Connected,iCoupler))
        {
         //dsbCouplerAttach->SetVolume(DSBVOLUME_MAX);
         //dsbCouplerAttach->Play(0,0,0);
        }
      //WriteLog("CoupleDist[1]="+AnsiString(Controlling->Couplers[1].CoupleDist)+", Connected[0]="+AnsiString(Controlling->Couplers[1].CouplingFlag));
      if (pVehicles[0]->MoverParameters->Couplers[1].CouplingFlag==iCoupler) //uda³o siê? (mog³o czêœciowo)
       ok=true; //zaczepiony zgodnie z ¿yczeniem!
     }
     if (ok)
     {//je¿eli zosta³ pod³¹czony
      iDrivigFlags&=~moveConnect; //zdjêcie flagi doczepiania
      SetVelocity(0,0,stopJoin); //wy³¹czyæ przyspieszanie
      CheckVehicles(); //sprawdziæ œwiat³a nowego sk³adu
      JumpToNextOrder(); //wykonanie nastêpnej komendy
     }
     else if ((iDrivigFlags&moveConnect)==0) //jeœli nie zbli¿y³ siê dostatecznie
      if (pVehicles[0]->fTrackBlock>100.0) //ta odleg³oœæ mo¿e byæ rzadko odœwie¿ana
      {SetVelocity(20.0,5.0); //jazda w ustawionym kierunku z prêdkoœci¹ 20
       //SetProximityVelocity(-pVehicles[0]->fTrackBlock+50,0); //to nie dzia³a dobrze
       //SetProximityVelocity(pVehicles[0]->fTrackBlock,0); //to te¿ nie dzia³a dobrze
       //vCommandLocation=pVehicles[0]->AxlePositionGet()+pVehicles[0]->fTrackBlock*SafeNormalize(iDirection*pVehicles[0]->GetDirection());
       //WriteLog(AnsiString(vCommandLocation.x)+" "+AnsiString(vCommandLocation.z));
      }
      else if (pVehicles[0]->fTrackBlock>20.0)
        SetVelocity(0.1*pVehicles[0]->fTrackBlock,2.0); //jazda w ustawionym kierunku z prêdkoœci¹ 5
      else
      {SetVelocity(2.0,0.0); //jazda w ustawionym kierunku z prêdkoœci¹ 2 (18s)
       if (pVehicles[0]->fTrackBlock<2.0) //przy zderzeniu fTrackBlock nie jest miarodajne
        iDrivigFlags|=moveConnect; //wy³¹czenie sprawdzania fTrackBlock
      }
    } //nawias bo by³a zmienna lokalna
    break;
    case Disconnect: //20.07.03 - manewrowanie wagonami
     fMinProximityDist=1.0; fMaxProximityDist=10.0; //[m]
     VelReduced=5; //[km/h]
     if (iVehicleCount>=0) //jeœli by³a podana iloœæ wagonów
     {
      if (Prepare2press) //jeœli dociskanie w celu odczepienia
      {
       SetVelocity(3,0); //jazda w ustawionym kierunku z prêdkoœci¹ 3
       if (Controlling->MainCtrlPos>0) //jeœli jazda
       {
        //WriteLog("Odczepianie w kierunku "+AnsiString(Controlling->DirAbsolute));
        if ((Controlling->DirAbsolute>0)&&(Controlling->Couplers[0].CouplingFlag>0))
        {
         if (!pVehicle->Dettach(0,iVehicleCount)) //zwraca maskê bitow¹ po³¹czenia
         {//tylko jeœli odepnie
          //WriteLog("Odczepiony od strony 0");
          iVehicleCount=-2;
         } //a jak nie, to dociskaæ dalej
        }
        else if ((Controlling->DirAbsolute<0)&&(Controlling->Couplers[1].CouplingFlag>0))
        {
         if (!pVehicle->Dettach(1,iVehicleCount)) //zwraca maskê bitow¹ po³¹czenia
         {//tylko jeœli odepnie
          //WriteLog("Odczepiony od strony 1");
          iVehicleCount=-2;
         } //a jak nie, to dociskaæ dalej
        }
        else
        {//jak nic nie jest podpiête od ustawionego kierunku ruchu, to nic nie odczepimy
         //WriteLog("Nie ma co odczepiaæ?");
         HelpMeFlag=true; //cos nie tak z kierunkiem dociskania
         iVehicleCount=-2;
        }
       }
       if (iVehicleCount>=0) //zmieni siê po odczepieniu
        if (!Controlling->DecLocalBrakeLevel(1))
        {//dociœnij sklad
         //WriteLog("Dociskanie");
         Controlling->BrakeReleaser(); //wyluzuj lokomotywê
         //Ready=true; //zamiast sprawdzenia odhamowania ca³ego sk³adu
         IncSpeed(); //dla (Ready)==false nie ruszy
        }
      }
      if ((Controlling->Vel==0.0)&&!Prepare2press)
      {//2. faza odczepiania: zmieñ kierunek na przeciwny i dociœnij
       //za rad¹ yB ustawiamy pozycjê 3 kranu (na razie 4, bo gdzieœ siê DecBrake() wykonuje)
       //WriteLog("Zahamowanie sk³adu");
       while ((Controlling->BrakeCtrlPos>4)&&Controlling->DecBrakeLevel());
       while ((Controlling->BrakeCtrlPos<4)&&Controlling->IncBrakeLevel());
       if (Controlling->PipePress-Controlling->BrakePressureTable[Controlling->BrakeCtrlPos-1+2].PipePressureVal<0.01)
       {//jeœli w miarê zosta³ zahamowany
        //WriteLog("Luzowanie lokomotywy i zmiana kierunku");
        Controlling->BrakeReleaser(); //wyluzuj lokomotywê; a ST45?
        Controlling->DecLocalBrakeLevel(10); //zwolnienie hamulca
        Prepare2press=true; //nastêpnie bêdzie dociskanie
        DirectionForward(iDirection<0); //zmiana kierunku jazdy na przeciwny
        CheckVehicles(); //od razu zmieniæ œwiat³a (zgasiæ)
        fStopTime=0.0; //nie ma na co czekaæ z odczepianiem
       }
      }
     } //odczepiania
     else //to poni¿ej jeœli iloœæ wagonów ujemna
      if (Prepare2press)
      {//4. faza odczepiania: zwolnij i zmieñ kierunek
       SetVelocity(0,0,stopJoin); //wy³¹czyæ przyspieszanie
       if (!DecSpeed()) //jeœli ju¿ bardziej wy³¹czyæ siê nie da
       {//ponowna zmiana kierunku
        //WriteLog("Ponowna zmiana kierunku");
        DirectionForward(iDirection>=0); //zmiana kierunku jazdy na w³aœciwy
        Prepare2press=false; //koniec dociskania
        CheckVehicles(); //od razu zmieniæ œwiat³a
        JumpToNextOrder();
        SetVelocity(fShuntVelocity,fShuntVelocity); //ustawienie prêdkoœci jazdy
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
     if (Controlling->CategoryFlag&1) //jeœli poci¹g
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
      //OrderList[OrderPos]:=Shunt; //Ra: to nie mo¿e tak byæ, bo scenerie robi¹ Jump_to_first_order i przechodzi w manewrowy
      JumpToNextOrder(); //w nastêpnym jest Shunt albo Obey_train, moze te¿ byæ Change_direction, Connect albo Disconnect
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
    case Change_direction: //tryby wymagaj¹ce jazdy
     if (OrderList[OrderPos]!=Obey_train) //spokojne manewry
     {
      VelActual=Min0R(VelActual,40); //jeœli manewry, to ograniczamy prêdkoœæ
      if (iVehicleCount>=0)
       if (!Prepare2press)
        if (Controlling->Vel>0.0)
        {SetVelocity(0,0,stopJoin); //1. faza odczepiania: zatrzymanie
         //WriteLog("Zatrzymanie w celu odczepienia");
        }
     }
     else
      SetDriverPsyche(); //Ra: by³o w PrepareEngine(), potrzebne tu?
     //no albo przypisujemy -WaitingExpireTime, albo porównujemy z WaitingExpireTime
     //if ((VelActual==0.0)&&(WaitingTime>WaitingExpireTime)&&(Controlling->RunningTrack.Velmax!=0.0))
     if (OrderList[OrderPos]&(Shunt|Obey_train)) //odjechaæ sam mo¿e tylko jeœli jest w trybie jazdy
     {//automatyczne ruszanie po odstaniu albo spod SBL
      if ((VelActual==0.0)&&(WaitingTime>0.0)&&(Controlling->RunningTrack.Velmax!=0.0))
      {//jeœli stoi, a up³yn¹³ czas oczekiwania i tor ma niezerow¹ prêdkoœæ
/*
      if (WriteLogFlag)
       {
         append(AIlogFile);
         writeln(AILogFile,ElapsedTime:5:2,": ",Name," V=0 waiting time expired! (",WaitingTime:4:1,")");
         close(AILogFile);
       }
*/
       if (Controlling->CategoryFlag&1)
       {//jeœli poci¹g
        PrepareEngine(); //zmieni ustawiony kierunek
        SetVelocity(20,20); //jak siê nasta³, to niech jedzie 20km/h
        WaitingTime=0.0;
        fWarningDuration=1.5; //a zatr¹biæ trochê
        Controlling->WarningSignal=1;
       }
       else
       {//samochód ma staæ, a¿ dostanie odjazd, chyba ¿e stoi przez kolizjê
        if (eStopReason==stopBlock)
         if (pVehicles[0]->fTrackBlock>fDriverDist)
         {PrepareEngine(); //zmieni ustawiony kierunek
          SetVelocity(-1,-1); //jak siê nasta³, to niech jedzie
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
      SetVelocity(0,0,stopDir); //najpierw trzeba siê zatrzymaæ
      if (Controlling->Vel<0.1)
      {//jeœli siê zatrzyma³, to zmieniamy kierunek jazdy, a nawet kabinê/cz³on
       Activation(); //ustawienie zadanego wczeœniej kierunku i ewentualne przemieszczenie AI
       PrepareEngine();
       JumpToNextOrder(); //nastêpnie robimy, co jest do zrobienia (Shunt albo Obey_train)
       if (OrderList[OrderPos]==Shunt) //jeœli dalej mamy manewry
        SetVelocity(fShuntVelocity,fShuntVelocity); //to od razu jedziemy
       eSignSkip=NULL; //nie ignorujemy przy poszukiwaniu nowego sygnalizatora
       eSignLast=NULL; //¿eby jakiœ nowy by³ poszukiwany
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
     {//jeœli jest wybrany kierunek jazdy, sprawdzamy mo¿liwe ograniczenia prêdkoœci
      if (fStopTime<=0) //czas postoju przed dalsz¹ jazd¹ (np. na przystanku)
       VelDesired=0.0; //jak ma czekaæ, to nie ma jazdy
      else if (VelActual<0)
       VelDesired=fVelMax; //ile fabryka dala (Ra: uwzglêdione wagony)
      else
       VelDesired=Min0R(fVelMax,VelActual); //VelActual>0 jest ograniczeniem prêdkoœci (z ró¿nyc Ÿróde³)
      if (Controlling->RunningTrack.Velmax>=0) //ograniczenie prêdkoœci z trajektorii ruchu
       VelDesired=Min0R(VelDesired,Controlling->RunningTrack.Velmax); //uwaga na ograniczenia szlakowej!
      if (VelforDriver>=0) //tu jest zero przy zmianie kierunku jazdy
       VelDesired=Min0R(VelDesired,VelforDriver); //Ra: tu mo¿e byæ 40, jeœli mechanik nie ma znajomoœci szlaaku, albo kierowca jeŸdzi 70
      if (TrainParams)
       if (TrainParams->CheckTrainLatency()<10.0)
        if (TrainParams->TTVmax>0.0)
         VelDesired=Min0R(VelDesired,TrainParams->TTVmax); //jesli nie spozniony to nie przekraczaæ rozkladowej
      //Ra: tu by jeszcze trzeba by³o wstawiæ uzale¿nienie (VelDesired) od odleg³oœci od przeszkody
      // no chyba ¿eby to uwzgldniæ ju¿ w (ActualProximityDist)
#if TESTTABLE
      //Ra: odczyt (ActualProximityDist), (VelNext) i (AccPreferred) z tabelki prêdkosci
      SetDriverPsyche(); //ustawia AccPreferred
      VelNext=VelDesired; //maksymalna prêdkoœæ wynikaj¹ca z innych czynników ni¿ trajektoria ruchu
      ActualProximityDist=scanmax; //funkcja Update() mo¿e pozostawiæ wartoœci bez zmian
      sSpeedTable.Update(Controlling->Vel,VelDesired,ActualProximityDist,VelNext,AccPreferred); //szukanie optymalnych wartoœci
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
        {//jeœli jedzie wolniej ni¿ mo¿na i jest wystarczaj¹co daleko, to mo¿na przyspieszyæ
         if (AccPreferred>0.0)
          AccDesired=0.5*AccPreferred;
          //VelDesired:=Min0R(VelDesired,VelReduced+VelNext);
        }
        else if (ActualProximityDist>fMinProximityDist)
        {//25.92 (=3.6*3.6*2) - skorektowane, zeby ladniej hamowal
         if ((VelNext>Controlling->Vel-35.0)) //dwustopniowe hamowanie - niski przy ma³ej ró¿nicy
         {if ((VelNext>0)&&(VelNext>Controlling->Vel-3.0)) //jeœli powolna i niewielkie przekroczenie
            AccDesired=0.0; //to olej (zacznij luzowaæ)
          else  //w przeciwnym wypadku
           if ((VelNext==0.0)&&(Controlling->V*Controlling->V<0.4*ActualProximityDist)) //jeœli stójka i niewielka prêdkoœæ
           {if (Controlling->Vel<30.0)  //trzymaj 30 km/h
             AccDesired=0.5*AccPreferred; //jak jest tu 0.5, to samochody siê dobijaj¹ do siebie
            else
             AccDesired=0.0;
           }
           else // prostu hamuj (niski stopieñ)
            if ((ActualProximityDist<fMaxProximityDist)&&(VelNext==0.0))
              AccDesired=-1;
            else
            AccDesired=(VelNext*VelNext-Controlling->Vel*Controlling->Vel)/(25.92*ActualProximityDist+0.1); //hamuj proporcjonalnie //mniejsze opóŸnienie przy ma³ej ró¿nicy
         }
         else  //przy du¿ej ró¿nicy wysoki stopieñ (1,25 potrzebnego opoznienia)
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
         AccDesired=-0.2; //hamuj tak œrednio
      //if AccDesired>-AccPreferred)
      // Accdesired:=-AccPreferred;
      //koniec predkosci aktualnej
      if ((AccDesired>0)&&(VelNext>=0)) //wybieg b¹dŸ lekkie hamowanie, warunki byly zamienione
       if (VelNext<Controlling->Vel-100.0) //lepiej zaczac hamowac
        AccDesired=-0.2;
       else
        if ((VelNext<Controlling->Vel-70))
         AccDesired=0.0; //nie spiesz siê, bo bêdzie hamowanie
      //koniec wybiegu i hamowania
      //w³¹czanie bezpiecznika
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
      if (Controlling->BrakeSystem==Pneumatic) //nape³nianie uderzeniowe
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
           //DecBrakeLevel(); //z tym by jeszcze mia³o jakiœ sens
          }
       }
#if LOGVELOCITY
      WriteLog("Dist="+FloatToStrF(ActualProximityDist,ffFixed,7,1)+", VelDesired="+FloatToStrF(VelDesired,ffFixed,7,1)+", AccDesired="+FloatToStrF(AccDesired,ffFixed,7,3)+", VelActual="+AnsiString(VelActual)+", VelNext="+AnsiString(VelNext));
#endif

      if (AccDesired>=0.0)
       if (Prepare2press)
        Controlling->BrakeReleaser(); //wyluzuj lokomotywê
       else
        while (DecBrake());  //jeœli przyspieszamy, to nie hamujemy
      //Ra: zmieni³em 0.95 na 1.0 - trzeba ustaliæ, sk¹d sie takie wartoœci bior¹
      //margines dla prêdkoœci jest doliczany tylko jeœli oczekiwana prêdkoœæ jest wiêksza od 5km/h
      if ((AccDesired<=0.0)||(Controlling->Vel+(VelDesired>5.0?VelMargin:0.0)>VelDesired*1.0))
       while (DecSpeed()); //jeœli hamujemy, to nie przyspieszamy
      //yB: usuniête ró¿ne dziwne warunki, oddzielamy czêœæ zadaj¹c¹ od wykonawczej
      //zwiekszanie predkosci
      if ((AbsAccS<AccDesired)&&(Controlling->Vel<VelDesired*0.95-VelMargin)&&(AccDesired>=0.0))
      //Ra: zmieni³em 0.85 na 0.95 - trzeba ustaliæ, sk¹d sie takie wartoœci bior¹
      //if (!MaxVelFlag) //Ra: to nie jest u¿ywane
       if (!IncSpeed())
        MaxVelFlag=true; //Ra: to nie jest u¿ywane
       else
        MaxVelFlag=false; //Ra: to nie jest u¿ywane
      //if (Vel<VelDesired*0.85) and (AccDesired>0) and (EngineType=ElectricSeriesMotor)
      // and (RList[MainCtrlPos].R>0.0) and (not DelayCtrlFlag))
      // if (Im<Imin) and Ready=True {(BrakePress<0.01*MaxBrakePress)})
      //  IncMainCtrl(1); //zwieksz nastawnik skoro mo¿esz - tak aby siê ustawic na bezoporowej

      //yB: usuniête ró¿ne dziwne warunki, oddzielamy czêœæ zadaj¹c¹ od wykonawczej
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
      //if ((AccDesired<0.0)&&(AbsAccS<AccDesired-0.1)) //ST44 nie hamuje na czas, 2-4km/h po miniêciu tarczy
      {//jak hamuje, to nie tykaj kranu za czêsto
       //yB: luzuje hamulec dopiero przy ró¿nicy opóŸnieñ rzêdu 0.2
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
     } //kierunek ró¿ny od zera
     //odhamowywanie sk³adu po zatrzymaniu i zabezpieczanie lokomotywy
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
  if (fWarningDuration>0.0) //jeœli pozosta³o coœ do wytr¹bienia
  {//tr¹bienie trwa nadal
   fWarningDuration=fWarningDuration-dt;
   if (fWarningDuration<0.1)
    Controlling->WarningSignal=0.0; //a tu siê koñczy
  }
#if !TESTTABLE
  if (UpdateOK) //stary system skanowania z wysy³aniem komend
   ScanEventTrack(); //skanowanie torów przeniesione z DynObj
#endif
  return UpdateOK;
 }
 else //if (AIControllFlag)
  return false; //AI nie obs³uguje
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
{//reakcja na zmianê rozkazu
 if (OrderList[OrderPos]==Shunt)
  CheckVehicles();
 if (OrderList[OrderPos]==Obey_train)
 {CheckVehicles();
  iDrivigFlags|=moveStopPoint; //W4 s¹ widziane
 }
 else if (OrderList[OrderPos]==Change_direction)
  iDirectionOrder=-iDirection; //trzeba zmieniæ jawnie, bo siê nie domyœli
 else if (OrderList[OrderPos]==Disconnect)
  iVehicleCount=0; //odczepianie lokomotywy
 else if (OrderList[OrderPos]==Wait_for_orders)
  OrdersClear(); //czyszczenie rozkazów i przeskok do zerowej pozycji
}

void __fastcall TController::OrderNext(TOrders NewOrder)
{//ustawienie rozkazu do wykonania jako nastêpny
 if (OrderList[OrderPos]==NewOrder)
  return; //jeœli robi to, co trzeba, to koniec
 if (!OrderPos) OrderPos=1; //na pozycji zerowej pozostaje czekanie
 OrderTop=OrderPos; //ale mo¿e jest czymœ zajêty na razie
 if (NewOrder>=Shunt) //jeœli ma jechaæ
 {//ale mo¿e byæ zajêty chwilowymi operacjami
  while (OrderList[OrderTop]?OrderList[OrderTop]<Shunt:false) //jeœli coœ robi
   ++OrderTop; //pomijamy wszystkie tymczasowe prace
 }
 else
 {//jeœli ma ustawion¹ jazdê, to wy³¹czamy na rzecz operacji
  while (OrderList[OrderTop]?(OrderList[OrderTop]<Shunt)&&(OrderList[OrderTop]!=NewOrder):false) //jeœli coœ robi
   ++OrderTop; //pomijamy wszystkie tymczasowe prace
 }
 OrderList[OrderTop++]=NewOrder; //dodanie rozkazu jako nastêpnego
}

void __fastcall TController::OrderPush(TOrders NewOrder)
{//zapisanie na stosie kolejnego rozkazu do wykonania
 if (OrderPos==OrderTop) //jeœli mia³by byæ zapis na aktalnej pozycji
  if (OrderList[OrderPos]<Shunt) //ale nie jedzie
   ++OrderTop; //niektóre operacje musz¹ zostaæ najpierw dokoñczone => zapis na kolejnej
 if (OrderList[OrderTop]!=NewOrder) //jeœli jest to samo, to nie dodajemy
  OrderList[OrderTop++]=NewOrder; //dodanie rozkazu na stos
 //if (OrderTop<OrderPos) OrderTop=OrderPos;
}

void __fastcall TController::OrdersDump()
{//wypisanie kolejnych rozkazów do logu
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
{//wype³nianie tabelki rozkazów na podstawie rozk³adu
 //ustawienie kolejnoœci komend, niezale¿nie kto prowadzi
 //Mechanik->OrderPush(Wait_for_orders); //czekanie na lepsze czasy
 //OrderPos=OrderTop=0; //wype³niamy od pozycji 0
 OrdersClear(); //usuniêcie poprzedniej tabeli
 OrderPush(Prepare_engine); //najpierw odpalenie silnika
 if (TrainParams->TrainName==AnsiString("none"))
  OrderPush(Shunt); //jeœli nie ma rozk³adu, to manewruje
 else
 {//jeœli z rozk³adem, to jedzie na szlak
  if (TrainParams?
   (TrainParams->TimeTable[1].StationWare.Pos("@")? //jeœli obrót na pierwszym przystanku
   (Controlling->TrainType&(dt_EZT)? //SZT równie¿! SN61 zale¿nie od wagonów...
   (TrainParams->TimeTable[1].StationName==TrainParams->Relation1):false):false):true)
   OrderPush(Shunt); //a teraz start bêdzie w manewrowym, a tryb poci¹gowy w³¹czy W4
  else
  //jeœli start z pierwszej stacji i jednoczeœnie jest na niej zmiana kierunku, to EZT ma mieæ Shunt
   OrderPush(Obey_train); //dla starych scenerii start w trybie pociagowym
  if (DebugModeFlag) //normalnie nie ma po co tego wypisywaæ
   WriteLog("/* Timetable: "+TrainParams->ShowRelation());
  TMTableLine *t;
  for (int i=0;i<=TrainParams->StationCount;++i)
  {t=TrainParams->TimeTable+i;
   if (DebugModeFlag) //normalnie nie ma po co tego wypisywaæ
    WriteLog(AnsiString(t->StationName)+" "+AnsiString((int)t->Ah)+":"+AnsiString((int)t->Am)+", "+AnsiString((int)t->Dh)+":"+AnsiString((int)t->Dm)+" "+AnsiString(t->StationWare));
   if (AnsiString(t->StationWare).Pos("@"))
   {//zmiana kierunku i dalsza jazda wg rozk³adu
    if (Controlling->TrainType&(dt_EZT)) //SZT równie¿! SN61 zale¿nie od wagonów...
    {//jeœli sk³ad zespolony, wystarczy zmieniæ kierunek jazdy
     OrderPush(Change_direction); //zmiana kierunku
    }
    else
    {//dla zwyk³ego sk³adu wagonowego odczepiamy lokomotywê
     OrderPush(Disconnect); //odczepienie lokomotywy
     OrderPush(Shunt); //a dalej manewry
     OrderPush(Change_direction); //zmiana kierunku
     OrderPush(Shunt); //jazda na drug¹ stronê sk³adu
     OrderPush(Change_direction); //zmiana kierunku
     OrderPush(Connect); //jazda pod wagony
    }
    if (i<TrainParams->StationCount) //jak nie ostatnia stacja
     OrderPush(Obey_train); //to dalej wg rozk³adu
   }
  }
  if (DebugModeFlag) //normalnie nie ma po co tego wypisywaæ
   WriteLog("*/");
  OrderPush(Shunt); //po wykonaniu rozk³adu prze³¹czy siê na manewry
 }
 //McZapkie-100302 - to ma byc wyzwalane ze scenerii
 if (fVel==0.0)
  SetVelocity(0,0,stopSleep); //jeœli nie ma prêdkoœci pocz¹tkowej, to œpi
 else
 {//jeœli podana niezerowa prêdkoœæ
  if (fVel>=1.0) //jeœli ma jechaæ
   iDrivigFlags|=moveStopCloser; //to do nastêpnego W4 ma podjechaæ blisko
  JumpToFirstOrder();
  SetVelocity(fVel,-1); //ma ustawiæ ¿¹dan¹ prêdkoœæ
 }
 if (DebugModeFlag) //normalnie nie ma po co tego wypisywaæ
  OrdersDump(); //wypisanie kontrolne tabelki rozkazów
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
{//Ra:obliczenie odleg³oœci punktu (p1) od p³aszczyzny o wektorze normalnym (n) przechodz¹cej przez (p2)
 return n.x*(p1.x-p2.x)+n.y*(p1.y-p2.y)+n.z*(p1.z-p2.z); //ax1+by1+cz1+d, gdzie d=-(ax2+by2+cz2)
};

//pomocnicza funkcja sprawdzania czy do toru jest podpiety semafor
bool __fastcall TController::CheckEvent(TEvent *e,bool prox)
{//sprawdzanie eventu, czy jest obs³ugiwany poza kolejk¹
 //prox=true, gdy sprawdzanie, czy dodaæ event do kolejki
 //-> zwraca true, jeœli event istotny dla AI
 //prox=false, gdy wyszukiwanie semafora przez AI
 //-> zwraca false, gdy event ma byæ dodany do kolejki
 if (e)
 {if (!prox) if (e==eSignSkip) return false; //semafor przejechany jest ignorowany
  AnsiString command;
  switch (e->Type)
  {//to siê wykonuje równie¿ sk³adu jad¹cego bez obs³ugi
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
    return false; //inne eventy siê nie licz¹
  }
  if (prox)
   if (command=="SetProximityVelocity")
    return true; //ten event z toru ma byæ ignorowany
  if (command=="SetVelocity")
   return true; //jak podamy wyjazd, to prze³¹czy siê w jazdê poci¹gow¹
  if (prox) //pomijanie punktów zatrzymania
  {if (command.SubString(1,19)=="PassengerStopPoint:")
    return true;
  }
  else //nazwa stacji pobrana z rozk³adu - nie ma co sprawdzaæ raz po raz
   if (!asNextStop.IsEmpty())
    if (command.SubString(1,asNextStop.Length())==asNextStop)
     return true;
 }
 return false;
}

TEvent* __fastcall TController::CheckTrackEvent(double fDirection,TTrack *Track)
{//sprawdzanie eventów na podanym torze
 //ZiomalCl: teraz zwracany jest pierwszy event podajacy predkosc dla AI
 //a nie kazdy najblizszy event [AI sie gubilo gdy przed getval z SetVelocity
 //mialo np. PutValues z eventem od SHP]
 TEvent* e=(fDirection>0)?Track->Event2:Track->Event1;
 return CheckEvent(e,false)?e:NULL; //sprawdzenie z pominiêciem niepotrzebnych
}

TTrack* __fastcall TController::TraceRoute(double &fDistance,double &fDirection,TTrack *Track,TEvent*&Event)
{//szukanie semafora w kierunku jazdy (eventu odczytu komórki pamiêci)
 //albo ustawienia innej prêdkoœci albo koñca toru
 TTrack *pTrackChVel=Track; //tor ze zmian¹ prêdkoœci
 TTrack *pTrackFrom; //odcinek poprzedni, do znajdywania koñca dróg
 double fDistChVel=-1; //odleg³oœæ do toru ze zmian¹ prêdkoœci
 double fCurrentDistance=pVehicle->RaTranslationGet(); //aktualna pozycja na torze
 double s=0;
 if (fDirection>0) //jeœli w kierunku Point2 toru
  fCurrentDistance=Track->Length()-fCurrentDistance;
 if ((Event=CheckTrackEvent(fDirection,Track))!=NULL)
 {//jeœli jest semafor na tym torze
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
  //Track->ScannedFlag=true; //do pokazywania przeskanowanych torów
  pTrackFrom=Track; //zapamiêtanie aktualnego odcinka
  s+=fCurrentDistance; //doliczenie kolejnego odcinka do przeskanowanej d³ugoœci
  if (fDirection>0)
  {//jeœli szukanie od Point1 w kierunku Point2
   if (Track->iNextDirection)
    fDirection=-fDirection;
   Track=Track->CurrentNext(); //mo¿e byæ NULL
  }
  else //if (fDirection<0)
  {//jeœli szukanie od Point2 w kierunku Point1
   if (!Track->iPrevDirection)
    fDirection=-fDirection;
   Track=Track->CurrentPrev(); //mo¿e byæ NULL
  }
  if (Track==pTrackFrom) Track=NULL; //koniec, tak jak dla torów
  if (Track?(Track->fVelocity==0.0)||(Track->iDamageFlag&128):true)
  {//gdy dalej toru nie ma albo zerowa prêdkoœæ, albo uszkadza pojazd
   fDistance=s;
   return NULL; //zwraca NULL, ewentualnie tor z zerow¹ prêdkoœci¹
  }
  if (fDistChVel<0? //gdy pierwsza zmiana prêdkoœci
   (Track->fVelocity!=pTrackChVel->fVelocity): //to prêdkosæ w kolejnym torze ma byæ ró¿na od aktualnej
   ((pTrackChVel->fVelocity<0.0)? //albo jeœli by³a mniejsza od zera (maksymalna)
    (Track->fVelocity>=0.0): //to wystarczy, ¿e nastêpna bêdzie nieujemna
    (Track->fVelocity<pTrackChVel->fVelocity))) //albo dalej byæ mniejsza ni¿ poprzednio znaleziona dodatnia
  {
   fDistChVel=s; //odleg³oœæ do zmiany prêdkoœci
   pTrackChVel=Track; //zapamiêtanie toru
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
 fDistance=fDistChVel; //odleg³oœæ do zmiany prêdkoœci
 return pTrackChVel; //i tor na którym siê zmienia
}


//sprawdzanie zdarzeñ semaforów i ograniczeñ szlakowych
void __fastcall TController::SetProximityVelocity(double dist,double vel,const vector3 *pos)
{//Ra:przeslanie do AI prêdkoœci
/*
 //!!!! zast¹piæ prawid³ow¹ reakcj¹ AI na SetProximityVelocity !!!!
 if (vel==0)
 {//jeœli zatrzymanie, to zmniejszamy dystans o 10m
  dist-=10.0;
 };
 if (dist<0.0) dist=0.0;
 if ((vel<0)?true:dist>0.1*(MoverParameters->Vel*MoverParameters->Vel-vel*vel)+50)
 {//jeœli jest dalej od umownej drogi hamowania
*/
  PutCommand("SetProximityVelocity",dist,vel,pos);
#if LOGVELOCITY
  WriteLog("-> SetProximityVelocity "+AnsiString(floor(dist))+" "+AnsiString(vel));
#endif
/*
 }
 else
 {//jeœli jest zagro¿enie, ¿e przekroczy
  Mechanik->SetVelocity(floor(0.2*sqrt(dist)+vel),vel,stopError);
#if LOGVELOCITY
  WriteLog("-> SetVelocity "+AnsiString(floor(0.2*sqrt(dist)+vel))+" "+AnsiString(vel));
#endif
 }
 */
}

int komenda[24]=
{//tabela reakcji na sygna³y podawane semaforem albo tarcz¹ manewrow¹
 //0: nic, 1: SetProximityVelocity, 2: SetVelocity, 3: ShuntVelocity
 3, //[ 0]= staæ   stoi   blisko manewr. ShuntV.
 3, //[ 1]= jechaæ stoi   blisko manewr. ShuntV.
 3, //[ 2]= staæ   jedzie blisko manewr. ShuntV.
 3, //[ 3]= jechaæ jedzie blisko manewr. ShuntV.
 0, //[ 4]= staæ   stoi   deleko manewr. ShuntV. - podjedzie sam
 3, //[ 5]= jechaæ stoi   deleko manewr. ShuntV.
 1, //[ 6]= staæ   jedzie deleko manewr. ShuntV.
 1, //[ 7]= jechaæ jedzie deleko manewr. ShuntV.
 2, //[ 8]= staæ   stoi   blisko poci¹g. SetV.
 2, //[ 9]= jechaæ stoi   blisko poci¹g. SetV.
 2, //[10]= staæ   jedzie blisko poci¹g. SetV.
 2, //[11]= jechaæ jedzie blisko poci¹g. SetV.
 2, //[12]= staæ   stoi   deleko poci¹g. SetV.
 2, //[13]= jechaæ stoi   deleko poci¹g. SetV.
 1, //[14]= staæ   jedzie deleko poci¹g. SetV.
 1, //[15]= jechaæ jedzie deleko poci¹g. SetV.
 3, //[16]= staæ   stoi   blisko manewr. SetV.
 2, //[17]= jechaæ stoi   blisko manewr. SetV. - zmiana na jazdê poci¹gow¹
 3, //[18]= staæ   jedzie blisko manewr. SetV.
 2, //[19]= jechaæ jedzie blisko manewr. SetV.
 0, //[20]= staæ   stoi   deleko manewr. SetV. - niech podjedzie
 2, //[21]= jechaæ stoi   deleko manewr. SetV.
 1, //[22]= staæ   jedzie deleko manewr. SetV.
 2  //[23]= jechaæ jedzie deleko manewr. SetV.
};

void __fastcall TController::ScanEventTrack()
{//sprawdzanie zdarzeñ semaforów i ograniczeñ szlakowych
 if ((OrderList[OrderPos]&~(Obey_train|Shunt)))
  return; //skanowanie sygna³ów tylko gdy jedzie albo czeka na rozkazy
 //Ra: AI mo¿e siê stoczyæ w przeciwnym kierunku, ni¿ oczekiwana jazda !!!!
 vector3 sl;
 //jeœli z przodu od kierunku ruchu jest jakiœ pojazd ze sprzêgiem wirtualnym
 if (pVehicles[0]->fTrackBlock<=fDriverDist) //jak odleg³oœæ kolizyjna, to sprawdziæ
 {
  pVehicles[0]->ABuScanObjects(pVehicles[0]->DirectionGet(),300); //skanowanie sprawdzaj¹ce
  if (pVehicles[0]->fTrackBlock<=fDriverDist) //jak potwierdzona odleg³oœæ kolizyjna, to stop
  {
   SetVelocity(0,0,stopBlock); //zatrzymaæ //nie potrzeba tu zatrzymywaæ, AI sobie samo sprawdzi odleg³oœæ
   return; //i dalej nie ma co analizowaæ innych przypadków (!!!! do przemyœlenia)
  }
 }
 else
  if (fDriverMass*VelDesired*VelDesired>pVehicles[0]->fTrackBlock) //droga hamowania wiêksza ni¿ odleg³oœæ kolizyjna
   SetProximityVelocity(pVehicles[0]->fTrackBlock-fDriverDist,0); //spowolnienie jazdy
 int startdir=iDirection; //kierunek jazdy wzglêdem pojazdu, w którym siedzi AI (1=przód,-1=ty³)
 if (startdir==0) //jeœli kabina i kierunek nie jest okreœlony
  return; //nie robimy nic
 if (OrderList[OrderPos]==Shunt?OrderList[OrderPos+1]==Change_direction:false)
 {//jeœli jedzie manewrowo, ale nastêpnie ma w planie zmianê kierunku, to skanujemy te¿ w drug¹ stronê
  if (iDrivigFlags&moveBackwardLook) //jeœli ostatnio by³o skanowanie do ty³u
   iDrivigFlags&=~moveBackwardLook; //do przodu popatrzeæ trzeba
  else
  {iDrivigFlags|=moveBackwardLook; //albo na zmianê do ty³u
   //startdir=-startdir;
   //co dalej, to trzeba jeszcze przemyœleæ
  }
 }
  //startdir=2*random(2)-1; //wybieramy losowy kierunek - trzeba by jeszcze ustawiæ kierunek ruchu
 //iAxleFirst - która oœ jest z przodu w kierunku jazdy: =0-przednia >0-tylna
 double scandir=startdir*pVehicles[0]->RaDirectionGet(); //szukamy od pierwszej osi w kierunku ruchu
 if (scandir!=0.0) //skanowanie toru w poszukiwaniu eventów GetValues/PutValues
 {//Ra: skanowanie drogi proporcjonalnej do kwadratu aktualnej prêdkoœci (+150m), no chyba ¿e stoi (wtedy 500m)
  //Ra: tymczasowo, dla wiêkszej zgodnoœci wstecz, szukanie semafora na postoju zwiêkszone do 2km
  double scanmax=(Controlling->Vel>0.0)?3*fDriverDist+fDriverMass*Controlling->Vel*Controlling->Vel:500; //fabs(Mechanik->ProximityDist);
  double scandist=scanmax; //zmodyfikuje na rzeczywiœcie przeskanowane
  //Ra: znaleziony semafor trzeba zapamiêtaæ, bo mo¿e byæ wpisany we wczeœniejszy tor
  //Ra: oprócz semafora szukamy najbli¿szego ograniczenia (koniec/brak toru to ograniczenie do zera)
  TEvent *ev=NULL; //event potencjalnie od semafora
  TTrack *scantrack=TraceRoute(scandist,scandir,pVehicles[0]->RaTrackGet(),ev); //wg drugiej osi w kierunku ruchu
  vector3 dir=startdir*pVehicles[0]->VectorFront(); //wektor w kierunku jazdy/szukania
  if (!scantrack) //jeœli wykryto koniec toru albo zerow¹ prêdkoœæ
  {
   //if (!Mechanik->SetProximityVelocity(0.7*fabs(scandist),0))
    // Mechanik->SetVelocity(Mechanik->VelActual*0.9,0);
     //scandist=(scandist>5?scandist-5:0); //10m do zatrzymania
    vector3 pos=pVehicles[0]->AxlePositionGet()+scandist*dir;
#if LOGVELOCITY
    WriteLog("End of track:");
#endif
    double vend=(Controlling->CategoryFlag&1)?0:20; //poci¹g ma stan¹æ, samochód zwolniæ
    if (scandist>10) //jeœli zosta³o wiêcej ni¿ 10m do koñca toru
     SetProximityVelocity(scandist,vend,&pos); //informacja o zbli¿aniu siê do koñca
    else
    {PutCommand("SetVelocity",vend,vend,&pos,stopEnd); //na koñcu toru ma staæ
#if LOGVELOCITY
     WriteLog("-> SetVelocity 0 0");
#endif
    }
    return;
  }
  else
  {//jeœli s¹ dalej tory
   double vtrackmax=scantrack->fVelocity;  //ograniczenie szlakowe
   double vmechmax=-1; //prêdkoœæ ustawiona semaforem
   TEvent *e=eSignLast; //poprzedni sygna³ nadal siê liczy
   if (!e) //jeœli nie by³o ¿adnego sygna³u
    e=ev; //ten ewentualnie znaleziony (scandir>0)?scantrack->Event2:scantrack->Event1; //pobranie nowego
   if (e)
   {//jeœli jest jakiœ sygna³ na widoku
#if LOGVELOCITY + LOGSTOPS
    AnsiString edir=pVehicle->asName+" - "+AnsiString((scandir>0)?"Event2 ":"Event1 ");
#endif
    //najpierw sprawdzamy, czy semafor czy inny znak zosta³ przejechany
    //bo jeœli tak, to trzeba szukaæ nastêpnego, a ten mo¿e go zas³aniaæ
    //vector3 pos=pVehicles[0]->GetPosition(); //aktualna pozycja, potrzebna do liczenia wektorów
    vector3 pos=pVehicles[0]->HeadPosition();//+0.5*pVehicles[0]->MoverParameters->Dim.L*dir; //pozycja czo³a
    vector3 sem; //wektor do sygna³u
    if (e->Type==tp_GetValues)
    {//przes³aæ info o zbli¿aj¹cym siê semaforze
#if LOGVELOCITY
     edir+="("+(e->Params[8].asGroundNode->asName)+"): ";
#endif
     //sem=*e->PositionGet()-pos; //wektor do komórki pamiêci
     sem=e->Params[8].asGroundNode->pCenter-pos; //wektor do komórki pamiêci
     if (dir.x*sem.x+dir.z*sem.z<0) //jeœli zosta³ miniêty
      if ((Controlling->CategoryFlag&1)?(VelNext!=0.0):true) //dla poci¹gu wymagany sygna³ zezwalaj¹cy
      {//iloczyn skalarny jest ujemny, gdy sygna³ stoi z ty³u
       eSignSkip=e; //wtedy uznajemy go za ignorowany przy poszukiwaniu nowego
       eSignLast=NULL; //¿eby jakiœ nowy by³ poszukiwany
#if LOGVELOCITY
       WriteLog(edir+"- will be ignored as passed by");
#endif
       return;
      }
     sl=e->Params[8].asGroundNode->pCenter;
     vmechmax=e->Params[9].asMemCell->fValue1; //prêdkoœæ przy tym semaforze
     //przeliczamy odleg³oœæ od semafora - potrzebne by by³y wspó³rzêdne pocz¹tku sk³adu
     //scandist=(pos-e->Params[8].asGroundNode->pCenter).Length()-0.5*Controlling->Dim.L-10; //10m luzu
     scandist=(pos-e->Params[8].asGroundNode->pCenter).Length()-10; //10m luzu
     if (scandist<0) scandist=0; //ujemnych nie ma po co wysy³aæ

     //Ra: takie rozpisanie wcale nie robi proœciej
     int mode=(vmechmax!=0.0)?1:0; //tryb jazdy - prêdkoœæ podawana sygna³em
     mode|=(Controlling->Vel>0.0)?2:0; //stan aktualny: jedzie albo stoi
     mode|=(scandist>fMinProximityDist)?4:0;
     if (OrderList[OrderPos]!=Obey_train)
     {mode|=8; //tryb manewrowy
      if (strcmp(e->Params[9].asMemCell->szText,"ShuntVelocity")==0)
       mode|=16; //ustawienie prêdkoœci manewrowej
     }
     if ((mode&16)==0) //o ile nie podana prêdkoœæ manewrowa
      if (strcmp(e->Params[9].asMemCell->szText,"SetVelocity")==0)
       mode=32; //ustawienie prêdkoœci poci¹gowej
     //wartoœci 0..15 nie przekazuj¹ komendy
     //wartoœci 16..23 - ignorowanie sygna³ów maewrowych w trybie poc¹gowym
     mode-=24;
     if (mode>=0)
     {
      //+4: dodatkowo: - sygna³ ma byæ ignorowany
      switch (komenda[mode]) //pobranie kodu komendy
      {//komendy mo¿liwe do przekazania:
       case 0: //nic - sygna³ nie wysy³a komendy
        break;
       case 1: //SetProximityVelocity - informacja o zmienie prêdkoœci
        break;
       case 2: //SetVelocity - nadanie prêdkoœci do jazdy poci¹gowej
        break;
       case 3: //ShuntVelocity - nadanie prêdkoœci do jazdy manewrowej
        break;
      }
     }
     bool move=false; //czy AI w trybie manewerowym ma doci¹gn¹æ pod S1
     if (strcmp(e->Params[9].asMemCell->szText,"SetVelocity")==0)
      if ((vmechmax==0.0)?(OrderCurrentGet()==Shunt):false)
       move=true; //AI w trybie manewerowym ma doci¹gn¹æ pod S1
      else
      {//
       eSignLast=(scandist<200)?e:NULL; //zapamiêtanie na wypadek przejechania albo Ÿle podpiêtego toru
       if ((scandist>fMinProximityDist)?(Controlling->Vel>0.0)&&(OrderCurrentGet()!=Shunt):false)
       {//jeœli semafor jest daleko, a pojazd jedzie, to informujemy o zmianie prêdkoœci
        //jeœli jedzie manewrowo, musi dostaæ SetVelocity, ¿eby sie na poci¹gowy prze³¹czy³
        //Mechanik->PutCommand("SetProximityVelocity",scandist,vmechmax,sl);
#if LOGVELOCITY
        //WriteLog(edir+"SetProximityVelocity "+AnsiString(scandist)+" "+AnsiString(vmechmax));
        WriteLog(edir);
#endif
        SetProximityVelocity(scandist,vmechmax,&sl);
        return;
       }
       else  //ustawiamy prêdkoœæ tylko wtedy, gdy ma ruszyæ, stan¹æ albo ma staæ
        //if ((MoverParameters->Vel==0.0)||(vmechmax==0.0)) //jeœli stoi lub ma stan¹æ/staæ
        {//semafor na tym torze albo lokomtywa stoi, a ma ruszyæ, albo ma stan¹æ, albo nie ruszaæ
         //stop trzeba powtarzaæ, bo inaczej zatr¹bi i pojedzie sam
         PutCommand("SetVelocity",vmechmax,e->Params[9].asMemCell->fValue2,&sl,stopSem);
#if LOGVELOCITY
         WriteLog(edir+"SetVelocity "+AnsiString(vmechmax)+" "+AnsiString(e->Params[9].asMemCell->fValue2));
#endif
         return;
        }
      }
     if (OrderCurrentGet()==Shunt) //w Wait_for_orders te¿ widzi tarcze?
     {//reakcja AI w trybie manewrowym dodatkowo na sygna³y manewrowe
      if (move?true:strcmp(e->Params[9].asMemCell->szText,"ShuntVelocity")==0)
      {//jeœli powy¿ej by³o SetVelocity 0 0, to doci¹gamy pod S1
       eSignLast=(scandist<200)?e:NULL; //zapamiêtanie na wypadek przejechania albo Ÿle podpiêtego toru
       if ((scandist>fMinProximityDist)?(Controlling->Vel>0.0)||(vmechmax==0.0):false)
       {//jeœli tarcza jest daleko, to:
        //- jesli pojazd jedzie, to informujemy o zmianie prêdkoœci
        //- jeœli stoi, to z w³asnej inicjatywy mo¿e podjechaæ pod zamkniêt¹ tarczê
        if (Controlling->Vel>0.0) //tylko jeœli jedzie
        {//Mechanik->PutCommand("SetProximityVelocity",scandist,vmechmax,sl);
#if LOGVELOCITY
         //WriteLog(edir+"SetProximityVelocity "+AnsiString(scandist)+" "+AnsiString(vmechmax));
         WriteLog(edir);
#endif
         SetProximityVelocity(scandist,vmechmax,&sl);
         return;
        }
       }
       else //ustawiamy prêdkoœæ tylko wtedy, gdy ma ruszyæ, albo stan¹æ albo ma staæ pod tarcz¹
       {//stop trzeba powtarzaæ, bo inaczej zatr¹bi i pojedzie sam
        //if ((MoverParameters->Vel==0.0)||(vmechmax==0.0)) //jeœli jedzie lub ma stan¹æ/staæ
        {//nie dostanie komendy jeœli jedzie i ma jechaæ
         PutCommand("ShuntVelocity",vmechmax,e->Params[9].asMemCell->fValue2,&sl,stopSem);
#if LOGVELOCITY
         WriteLog(edir+"ShuntVelocity "+AnsiString(vmechmax)+" "+AnsiString(e->Params[9].asMemCell->fValue2));
#endif
         return;
        }
       }
       if ((vmechmax!=0.0)&&(scandist<100.0))
       {//jeœli Tm w odleg³oœci do 100m podaje zezwolenie na jazdê, to od razu j¹ ignorujemy, aby móc szukaæ kolejnej
        eSignSkip=e; //wtedy uznajemy ignorowan¹ przy poszukiwaniu nowej
        eSignLast=NULL; //¿eby jakaœ nowa by³a poszukiwana
#if LOGVELOCITY
        WriteLog(edir+"- will be ignored due to Ms2");
#endif
        return;
       }
      } //if (move?...
     } //if (OrderCurrentGet()==Shunt)
    } //if (e->Type==tp_GetValues)
    else if (e->Type==tp_PutValues?!(iDrivigFlags&moveBackwardLook):false)
    {//przy skanowaniu wstecz te eventy siê nie licz¹
     sem=vector3(e->Params[3].asdouble,e->Params[4].asdouble,e->Params[5].asdouble)-pos; //wektor do komórki pamiêci
     if (strcmp(e->Params[0].asText,"SetVelocity")==0)
     {//statyczne ograniczenie predkosci
      if (dir.x*sem.x+dir.z*sem.z<0)
      {//iloczyn skalarny jest ujemny, gdy sygna³ stoi z ty³u
       eSignSkip=e; //wtedy uznajemy go za ignorowany przy poszukiwaniu nowego
       eSignLast=NULL; //¿eby jakiœ nowy by³ poszukiwany
#if LOGVELOCITY
       WriteLog(edir+"- will be ignored as passed by");
#endif
       return;
      }
      vmechmax=e->Params[1].asdouble;
      sl.x=e->Params[3].asdouble; //wspó³rzêdne w scenerii
      sl.y=e->Params[4].asdouble;
      sl.z=e->Params[5].asdouble;
      if (scandist<fMinProximityDist+1) //tylko na tym torze
      {
       eSignLast=(scandist<200)?e:NULL; //zapamiêtanie na wypadek przejechania albo Ÿle podpiêtego toru
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
     {//jeœli W4 z nazw¹ jak w rozk³adzie, to aktualizacja rozk³adu i pobranie kolejnego
      scandist=sem.Length()-0.5*Controlling->Dim.L-3; //dok³adniejsza d³ugoœæ, 3m luzu
      if ((scandist<0)?true:dir.x*sem.x+dir.z*sem.z<0)
       scandist=0; //ujemnych nie ma po co wysy³aæ, jeœli miniêty, to równie¿ 0
      if (!TrainParams->IsStop())
      {//jeœli nie ma tu postoju
       if (scandist<500)
       {//zaliczamy posterunek w pewnej odleg³oœci przed, bo W4 zas³ania semafor
#if LOGSTOPS
        WriteLog(edir+AnsiString(GlobalTime->hh)+":"+AnsiString(GlobalTime->mm)+" Skipped stop: "+asNextStop.SubString(20,asNextStop.Length())); //informacja
#endif
        TrainParams->UpdateMTable(GlobalTime->hh,GlobalTime->mm,asNextStop.SubString(20,asNextStop.Length()));
        TrainParams->StationIndexInc(); //przejœcie do nastêpnej
        asNextStop=TrainParams->NextStop(); //pobranie kolejnego miejsca zatrzymania
        eSignSkip=e; //wtedy uznajemy go za ignorowany przy poszukiwaniu nowego
        eSignLast=NULL; //¿eby jakiœ nowy by³ poszukiwany
       }
      } //koniec obs³ugi przelotu na W4
      else
      {//zatrzymanie na W4
       //vmechmax=0.0; //ma stan¹æ na W4 - informacja dla dalszego kodu
       sl.x=e->Params[3].asdouble; //wyliczenie wspó³rzêdnych zatrzymania
       sl.y=e->Params[4].asdouble;
       sl.z=e->Params[5].asdouble;
       eSignLast=(scandist<200.0)?e:NULL; //zapamiêtanie na wypadek przejechania albo Ÿle podpiêtego toru
       if ((scandist>fMinProximityDist)?(Controlling->Vel>0.0):false)
       //if ((scandist>Mechanik->MinProximityDist)?(fSignSpeed>0.0):false)
       {//jeœli jedzie, informujemy o zatrzymaniu na wykrytym stopie
        //Mechanik->PutCommand("SetProximityVelocity",scandist,0,sl);
#if LOGVELOCITY
        //WriteLog(edir+"SetProximityVelocity "+AnsiString(scandist)+" 0");
        WriteLog(edir);
#endif
        //SetProximityVelocity(scandist,0,&sl); //staje 300m oe W4
        SetProximityVelocity(scandist,scandist>100.0?30:0.2*scandist,&sl); //Ra: taka proteza
       }
       else //jeœli jest blisko, albo stoi
        if ((iDrivigFlags&moveStopCloser)&&(scandist>25.0)) //jeœli ma podjechac pod W4, a jest daleko
        {
         //if (pVehicles[0]->fTrackBlock>fDriverDist) //je¿eli nie ma zawalidrogi w tej odleg³oœci
         PutCommand("SetVelocity",scandist>100.0?40:0.4*scandist,0,&sl); //doci¹ganie do przystanku
        } //koniec obs³ugi odleg³ego zatrzymania
        else
        {//jeœli pierwotnie sta³ lub zatrzyma³ siê wystarczaj¹co blisko
         PutCommand("SetVelocity",0,0,&sl,stopTime); //zatrzymanie na przystanku
#if LOGVELOCITY
         WriteLog(edir+"SetVelocity 0 0 ");
#endif
         if (Controlling->Vel==0.0)
         {//jeœli siê zatrzyma³ przy W4, albo sta³ w momencie zobaczenia W4
          if (Controlling->TrainType==dt_EZT) //otwieranie drzwi w EN57
           if (!Controlling->DoorLeftOpened&&!Controlling->DoorRightOpened)
           {//otwieranie drzwi
            int i=floor(e->Params[2].asdouble); //p7=platform side (1:left, 2:right, 3:both)
            if (iDirection>=0)
            {if (i&1) Controlling->DoorLeft(true);
             if (i&2) Controlling->DoorRight(true);
            }
            else
            {//jeœli jedzie do ty³u, to drzwi otwiera odwrotnie
             if (i&1) Controlling->DoorRight(true);
             if (i&2) Controlling->DoorLeft(true);
            }
            if (i&3) //¿eby jeszcze poczeka³ chwilê, zanim zamknie
             WaitingSet(10); //10 sekund (wzi¹æ z rozk³adu????)
           }
          if (TrainParams->UpdateMTable(GlobalTime->hh,GlobalTime->mm,asNextStop.SubString(20,asNextStop.Length())))
          {//to siê wykona tylko raz po zatrzymaniu na W4
           if (TrainParams->DirectionChange()) //jeœli '@" w rozk³adzie, to wykonanie dalszych komend
           {//wykonanie kolejnej komendy, nie dotyczy ostatniej stacji
            JumpToNextOrder(); //przejœcie do kolejnego rozkazu (zmiana kierunku, odczepianie)
            //if (OrderCurrentGet()==Change_direction) //jeœli ma zmieniæ kierunek
            iDrivigFlags&=~moveStopCloser; //ma nie podje¿d¿aæ pod W4 po przeciwnej stronie
            if (Controlling->TrainType!=dt_EZT) //
             iDrivigFlags&=~moveStopPoint; //pozwolenie na przejechanie za W4 przed czasem
            eSignLast=NULL; //niech skanuje od nowa, w przeciwnym kierunku
           }
          }
          if (OrderCurrentGet()==Shunt)
          {OrderNext(Obey_train); //uruchomiæ jazdê poci¹gow¹
           CheckVehicles(); //zmieniæ œwiat³a
          }
          if (TrainParams->StationIndex<TrainParams->StationCount)
          {//jeœli s¹ dalsze stacje, czekamy do godziny odjazdu
#if LOGVELOCITY
           WriteLog(edir+asNextStop); //informacja o zatrzymaniu na stopie
#endif
           if (iDrivigFlags&moveStopPoint) //jeœli pomijanie W4, to nie sprawdza czasu odjazdu
            if (TrainParams->IsTimeToGo(GlobalTime->hh,GlobalTime->mm))
            {//z dalsz¹ akcj¹ czekamy do godziny odjazdu
             TrainParams->StationIndexInc(); //przejœcie do nastêpnej
             asNextStop=TrainParams->NextStop(); //pobranie kolejnego miejsca zatrzymania
#if LOGSTOPS
             WriteLog(edir+AnsiString(GlobalTime->hh)+":"+AnsiString(GlobalTime->mm)+" Next stop: "+asNextStop.SubString(20,asNextStop.Length())); //informacja
#endif
             eSignSkip=e; //wtedy uznajemy go za ignorowany przy poszukiwaniu nowego
             eSignLast=NULL; //¿eby jakiœ nowy by³ poszukiwany
             iDrivigFlags|=moveStopCloser; //do nastêpnego W4 podjechaæ blisko (z doci¹ganiem)
             //vmechmax=vtrackmax; //odjazd po zatrzymaniu - informacja dla dalszego kodu
             vmechmax=-1; //odczytywanie prêdkoœci z toru ogranicza³o dalsz¹ jazdê
             PutCommand("SetVelocity",vmechmax,vmechmax,&sl);
#if LOGVELOCITY
             WriteLog(edir+"SetVelocity "+AnsiString(vtrackmax)+" "+AnsiString(vtrackmax));
#endif
            } //koniec startu z zatrzymania
          } //koniec obs³ugi pocz¹tkowych stacji
          else
          {//jeœli dojechaliœmy do koñca rozk³adu
           asNextStop=TrainParams->NextStop(); //informacja o koñcu trasy
           eSignSkip=e; //wtedy W4 uznajemy za ignorowany
           eSignLast=NULL; //¿eby jakiœ nowy sygna³ by³ poszukiwany
           iDrivigFlags&=~moveStopCloser; //ma nie podje¿d¿aæ pod W4
           WaitingSet(60); //tak ze 2 minuty, a¿ wszyscy wysi¹d¹
           JumpToNextOrder(); //wykonanie kolejnego rozkazu (Change_direction albo Shunt)
#if LOGSTOPS
           WriteLog(edir+AnsiString(GlobalTime->hh)+":"+AnsiString(GlobalTime->mm)+" Next stop: "+asNextStop.SubString(20,asNextStop.Length())); //informacja
#endif
          } //koniec obs³ugi ostatniej stacji
         } //if (MoverParameters->Vel==0.0)
        } //koniec obs³ugi W4 z zatrzymaniem
       return;
      } //koniec obs³ugi zatrzymania na W4
     } //koniec obs³ugi PassengerStopPoint
    } //if (e->Type==tp_PutValues...
   } //if (e)
   else //jeœli nic nie znaleziono
    if (OrderCurrentGet()==Shunt) //a jest w trybie manewrowym
     eSignSkip=NULL; //przywrócenie widocznoœci ewentualnie pominiêtej tarczy
   if (iDrivigFlags&moveBackwardLook) //jeœli skanowanie do ty³u
    return; //to nie analizujemy prêdkoœci w torach
   if (scandist<=scanmax) //jeœli ograniczenie jest dalej, ni¿ skanujemy, mo¿na je zignorowaæ
    if (vtrackmax>0.0) //jeœli w torze jest dodatnia
     if ((vmechmax<0)||(vtrackmax<vmechmax)) //i mniejsza od tej drugiej
     {//tutaj jest wykrywanie ograniczenia prêdkoœci w torze
      vector3 pos=pVehicles[0]->HeadPosition();
      double dist1=(scantrack->CurrentSegment()->FastGetPoint_0()-pos).Length();
      double dist2=(scantrack->CurrentSegment()->FastGetPoint_1()-pos).Length();
      if (dist2<dist1)
      {//Point2 jest bli¿ej i jego wybieramy
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
{//informav\cja o nastêpnym zatrzymaniu
 if (asNextStop.IsEmpty()) return "";
 //dodaæ godzinê odjazdu
 return asNextStop.SubString(20,30);//+" "+AnsiString(
};

//-----------koniec skanowania semaforow

void __fastcall TController::TakeControl(bool yes)
{//przejêcie kontroli przez AI albo oddanie
 if (AIControllFlag==yes) return; //ju¿ jest jak ma byæ
 if (yes) //¿eby nie wykonywaæ dwa razy
 {//teraz AI prowadzi
  AIControllFlag=AIdriver;
  pVehicle->Controller=AIdriver;
  if (OrderCurrentGet())
  {if (OrderCurrentGet()<Shunt)
   {OrderNext(Prepare_engine);
    if (pVehicle->iLights[Controlling->CabNo<0?1:0]&4) //górne œwiat³o
     OrderNext(Obey_train); //jazda poci¹gowa
    else
     OrderNext(Shunt); //jazda manewrowa
   }
  }
  else //jeœli jest w stanie Wait_for_orders
   JumpToFirstOrder(); //uruchomienie?
  // czy dac ponizsze? to problematyczne
  //SetVelocity(pVehicle->GetVelocity(),-1); //utrzymanie dotychczasowej?
  if (pVehicle->GetVelocity()>0.0)
   SetVelocity(-1,-1); //AI ustali sobie odpowiedni¹ prêdkoœæ
  CheckVehicles(); //ustawienie œwiate³
 }
 else
 {//a teraz u¿ytkownik
  AIControllFlag=Humandriver;
  pVehicle->Controller=Humandriver;
 }
};

void __fastcall TController::DirectionForward(bool forward)
{//ustawienie jazdy w kierunku sprzêgu 0 dla true i 1 dla false
 if (forward)
  while (Controlling->ActiveDir<=0)
   Controlling->DirectionForward();
 else
  while (Controlling->ActiveDir>=0)
   Controlling->DirectionBackward();
};

AnsiString __fastcall TController::Relation()
{//zwraca relacjê poci¹gu
 return TrainParams->ShowRelation();
};

AnsiString __fastcall TController::TrainName()
{//zwraca relacjê poci¹gu
 return TrainParams->TrainName;
};

double __fastcall TController::StationCount()
{//zwraca iloœæ stacji (miejsc zatrzymania)
 return TrainParams->StationCount;
};

double __fastcall TController::StationIndex()
{//zwraca indeks aktualnej stacji (miejsca zatrzymania)
 return TrainParams->StationIndex;
};


