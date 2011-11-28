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

#define LOGVELOCITY 0
#define LOGSTOPS 1
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
//10. logowanie fizyki
//11. kasowanie czuwaka/SHP
//12. procedury wspomagajace "patrzenie" na odlegle semafory
//13. ulepszone procedury sterowania
//14. zglaszanie problemow z dlugim staniem na sygnale S1
//15. sterowanie EN57
//16. zmiana kierunku
//17. otwieranie/zamykanie drzwi

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

__fastcall TController::TController
(//const Mover::TLocation &LocInitial,
 //const Mover::TRotation &RotInitial,
 //const vector3 &vLocInitial,
 //const vector3 &vRotInitial,
 bool AI,
 TDynamicObject *NewControll,
 Mtable::TTrainParameters *NewTrainParams,
 bool InitPsyche
)
{
 EngineActive=false;
 //vMechLoc=vLocInitial;
 //vMechRot=vRotInitial;
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
 VehicleName=Controlling->Name;
 TrainParams=NewTrainParams;
 if (TrainParams)
  asNextStop=TrainParams->NextStop();
 //OrderCommand="";
 //OrderValue=0;
 OrderPos=0;
 OrderTop=1; //szczyt stosu rozkazów
 for (int b=0;b<maxorders;b++)
  OrderList[b]=Wait_for_orders;
 MaxVelFlag=false; MinVelFlag=false;
 iDirection=iDirectionOrder=0; //1=do przodu (w kierunku sprzêgu 0)
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
}

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
 if      (Order==Wait_for_orders)     return "Wait_for_orders";
 else if (Order==Prepare_engine)      return "Prepare_engine";
 else if (Order==Shunt)               return "Shunt";
 else if (Order==Connect)             return "Connect";
 else if (Order==Disconnect)          return "Disconnect";
 else if (Order==Change_direction)    return "Change_direction";
 else if (Order==Obey_train)          return "Obey_train";
 else if (Order==Release_engine)      return "Release_engine";
 else if (Order==Jump_to_first_order) return "Jump_to_first_order";
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

bool __fastcall TController::CheckVehicles()
{//sprawdzenie stanu posiadanych pojazdów w sk³adzie i zapalenie œwiate³
 //ZiomalCl: sprawdzanie i zmiana SKP w skladzie prowadzonym przez AI
 TDynamicObject* p; //zmienne robocze
 iVehicles=0; //iloœæ pojazdów w sk³adzie
 int d=iDirection>0?0:1; //kierunek szukania czo³a (numer sprzêgu)
 pVehicles[0]=p=pVehicle->FirstFind(d); //pojazd na czele sk³adu
 //liczenie i ustawianie kierunku
 d=1-d; //a dalej bêdziemy zliczaæ od czo³a do ty³u
 fLength=0.0;
 while (p)
 {
  if (TrainParams)
   if (p->asDestination.IsEmpty())
    p->asDestination=TrainParams->Relation2; //relacja docelowa, jeœli nie by³o
  p->RaLightsSet(0,0); //gasimy œwiat³a
  ++iVehicles; //jest jeden pojazd wiêcej
  pVehicles[1]=p; //zapamiêtanie ostatniego
  fLength+=p->MoverParameters->Dim.L; //dodanie d³ugoœci pojazdu
  d=p->DirectionSet(d?1:-1); //zwraca po³o¿enie nastêpnego (1=zgodny,0=odwrócony)
  //1=zgodny: sprzêg 0 od czo³a; 0=odwrócony: sprzêg 1 od czo³a
  p=p->Next(); //pojazd pod³¹czony od ty³u (licz¹c od czo³a)
 }
/* //tabelka z list¹ pojazdów jest na razie nie potrzebna
 if (i!=)
 {delete[] pVehicle
 }
*/
 if (OrderCurrentGet()==Obey_train) //jeœli jazda poci¹gowa
  Lights(1+4+16,2+32+64); //œwiat³a poci¹gowe (Pc1) i koñcówki (Pc5)
 else if (OrderCurrentGet()&(Shunt|Connect))
  Lights(16,(Controlling->TrainType&dt_EZT)?2+32:1); //œwiat³a manewrowe (Tb1)
 else if (OrderCurrentGet()==Disconnect)
  Lights(16,0); //œwiat³a manewrowe (Tb1) tylko z przodu, aby nie pozostawiæ sk³adu ze œwiat³em
 return true;
}

void __fastcall TController::Lights(int head,int rear)
{//zapalenie œwiate³ w sk³¹dzie
 pVehicles[0]->RaLightsSet(head,-1); //zapalenie przednich w pierwszym
 pVehicles[1]->RaLightsSet(-1,rear); //zapalenie koñcówek w ostatnim
}

int __fastcall TController::OrderDirectionChange(int newdir,Mover::TMoverParameters *Vehicle)
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
 MaxVelFlag=False;
 MinVelFlag=False;
 VelActual=NewVel;   //prêdkoœæ do której d¹¿y
 VelNext=NewVelNext; //prêdkoœæ przy nastêpnym obiekcie
 if ((NewVel>NewVelNext) //jeœli oczekiwana wiêksza ni¿ nastêpna
  || (NewVel<Controlling->Vel)) //albo aktualna jest mniejsza ni¿ aktualna
  fProximityDist=-800.0; //droga hamowania do zmiany prêdkoœci
 else
  fProximityDist=-300.0;
 if (NewVel==0.0) //jeœli ma stan¹æ
 {if (r!=stopNone) //a jest powód podany
  eStopReason=r; //to zapamiêtaæ nowy powód
 }
 else
  eStopReason=stopNone; //podana prêdkoœæ, to nie ma powodów do stania
}

bool __fastcall TController::SetProximityVelocity(double NewDist,double NewVelNext)
{//informacja o prêdkoœci w pewnej odleg³oœci
 if (NewVelNext==0.0)
  WaitingTime=0.0; //nie trzeba ju¿ czekaæ
 //if ((NewVelNext>=0)&&((VelNext>=0)&&(NewVelNext<VelNext))||(NewVelNext<VelActual))||(VelNext<0))
 {MaxVelFlag=False;
  MinVelFlag=False;
  VelNext=NewVelNext;
  fProximityDist=NewDist;
  return true;
 }
 //else return false
}

void __fastcall TController::SetDriverPsyche()
{
 double maxdist=0.5; //skalowanie dystansu od innego pojazdu, zmienic to!!!
 if ((Psyche==Aggressive)&&(OrderList[OrderPos]==Obey_train))
 {
  ReactionTime=HardReactionTime; //w zaleznosci od charakteru maszynisty
  AccPreferred=HardAcceleration; //agresywny
  if (Controlling!=NULL)
   if (Controlling->CategoryFlag==2)
    WaitingExpireTime=11; //tyle ma czekaæ, zanim siê ruszy samochód
   else
    WaitingExpireTime=61; //tyle ma czekaæ, zanim siê ruszy
 }
 else
 {
  ReactionTime=EasyReactionTime; //spokojny
  AccPreferred=EasyAcceleration;
  WaitingExpireTime=65; //tyle ma czekaæ, zanim siê ruszy
 }
 if (Controlling!=NULL)
 {//with Controlling do
  if (Controlling->MainCtrlPos<3)
   ReactionTime=Controlling->InitialCtrlDelay+ReactionTime;
  if (Controlling->BrakeCtrlPos>1)
   ReactionTime=0.5*ReactionTime;
  if ((Controlling->V>0.1)&&(Controlling->Couplers[0].Connected)) //dopisac to samo dla V<-0.1 i zaleznie od Psyche
   if (Controlling->Couplers[0].CouplingFlag==0)
   {//Ra: funkcje s¹ odpowiednie?
    AccPreferred=(*Controlling->Couplers[0].Connected)->V; //tymczasowa wartoœæ
    AccPreferred=(AccPreferred*AccPreferred-Controlling->V*Controlling->V)/(25.92*(Controlling->Couplers[0].Dist-maxdist*fabs(Controlling->V)));
    AccPreferred=Min0R(AccPreferred,EasyAcceleration);
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
    {if ((Controlling->BrakeSubsystem==ss_W))
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
     if (Controlling->BrakePress<0.3)
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
{//odczytuje i wykonuje komendê przekazana lokomotywie
 TCommand *c=&Controlling->CommandIn;
 PutCommand(c->Command,c->Value1,c->Value2,c->Location,stopComm);
 c->Command=""; //usuniêcie obs³u¿onej komendy
}


void __fastcall TController::PutCommand(AnsiString NewCommand,double NewValue1,double NewValue2,const Mover::TLocation &NewLocation,TStopReason reason)
{//analiza komendy
 vector3 sl;
 sl.x=-NewLocation.X;
 sl.z= NewLocation.Y;
 sl.y= NewLocation.Z;
 PutCommand(NewCommand,NewValue1,NewValue2,&sl,reason);
}


void __fastcall TController::PutCommand(AnsiString NewCommand,double NewValue1,double NewValue2,const vector3 *NewLocation,TStopReason reason)
{//analiza komendy
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
  if (SetProximityVelocity(NewValue1,NewValue2))
   if (NewLocation)
    vCommandLocation=*NewLocation;
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
  if (NewValue1>0)
   TrainNumber=floor(NewValue1); //i co potem ???
  OrderCheck(); //jeœli jazda poci¹gowa teraz, to wykonaæ niezbêdne operacje
 }
 else if (NewCommand.Pos("Timetable:")==1)
 {//przypisanie nowego rozk³adu jazdy
  NewCommand.Delete(1,10); //zostanie nazwa pliku z rozk³adem
  TrainParams->NewName(NewCommand);
  if (NewCommand!="none")
  {if (!TrainParams->LoadTTfile(Global::asCurrentSceneryPath))
    Error("Cannot load timetable file "+NewCommand+"\r\nError "+ConversionError+" in position "+TrainParams->StationCount);
   else
   {//inicjacja pierwszego przystanku i pobranie jego nazwy
    if (!EngineActive)
     OrderNext(Prepare_engine); //trzeba odpaliæ silnik najpierw
    OrderNext(Obey_train); //¿e do jazdy
    TrainParams->UpdateMTable(GlobalTime->hh,GlobalTime->mm,TrainParams->NextStationName);
    TrainParams->StationIndexInc(); //przejœcie do nastêpnej
    asNextStop=TrainParams->NextStop();
    WriteLog("/* Timetable: "+TrainParams->ShowRelation());
    TMTableLine *t;
    for (int i=0;i<=TrainParams->StationCount;++i)
    {t=TrainParams->TimeTable+i;
     WriteLog(AnsiString(t->StationName)+" "+AnsiString((int)t->Ah)+":"+AnsiString((int)t->Am)+", "+AnsiString((int)t->Dh)+":"+AnsiString((int)t->Dm));
    }
    WriteLog("*/");
   }
  }
  if (NewValue1>0)
   TrainNumber=floor(NewValue1); //i co potem ???
 }
 else if (NewCommand=="Shunt")
 {//NewValue1 - iloœæ wagonów (-1=wszystkie), NewValue2: 0=odczep, 1=do³¹cz, -1=bez zmian
  //-1,-1 - tryb manewrowy bez zmian w sk³adzie
  //-1, 1 - pod³¹czyæ do ca³ego stoj¹cego sk³adu (do skutku)
  // 1, 1 - pod³¹czyæ do pierwszego wagonu w sk³adzie i odczepiæ go od reszty
  //-1, 0 - tryb manewrowy bez zmian (odczepianie z pozostawieniem wagonów nie ma sensu)
  // 0, 0 - odczepienie lokomotywy
  // 1, 0 - odczepienie lokomotywy z jednym wagonem
  if (!EngineActive)
   OrderNext(Prepare_engine); //trzeba odpaliæ silnik najpierw
  if (NewValue2>0)
   OrderNext(Connect); //po³¹cz (NewValue1) wagonów
  else if (NewValue2==0.0)
   if (NewValue1>=0.0) //jeœli iloœæ wagonów inna ni¿ wszystkie
    OrderNext(Disconnect); //odczep (NewValue1) wagonów
  OrderNext(Shunt); //potem manewruj dalej
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
 else if (NewCommand=="Emergency_brake") //wymuszenie zatrzymania
 {//Ra: no nadal nie jest zbyt piêknie
  SetVelocity(0,0,reason);
  Controlling->PutCommand("Emergency_brake",1.0,1.0,Controlling->Loc);
 }
};

const TDimension SignalDim={1,1,1};

bool __fastcall TController::UpdateSituation(double dt)
{//uruchamiaæ przynajmniej raz na sekundê
 double AbsAccS,VelReduced;
 bool UpdateOK=false;
 //yb: zeby EP nie musial sie bawic z ciesnieniem w PG
 if (AIControllFlag)
 {
//  if (Controlling->BrakeSystem==ElectroPneumatic)
//   Controlling->PipePress=0.5; //yB: w SPKS s¹ poprawnie zrobione pozycje
  if (Controlling->SlippingWheels)
  {
   Controlling->SandDoseOn();
   Controlling->SlippingWheels=false;
  }
 }
 {//ABu-160305 Testowanie gotowoœci do jazdy
  //Ra: przeniesione z DynObj
  if (Controlling->BrakePress<0.3)
   Ready=true; //wstêpnie gotowy
  //Ra: trzeba by sprawdziæ wszystkie, a nie tylko skrajne
  //sprawdzenie odhamowania skrajnych pojazdów
  if (pVehicles[1]->MoverParameters->BrakePress>0.3)
   Ready=false; //nie gotowy
  if (pVehicles[0]->MoverParameters->BrakePress>0.3)
   Ready=false; //nie gotowy
  //if (Ready)
  // Mechanik->CheckSKP(); //sprawdzenie sk³adu - czy tu potrzebne?
 }
 HelpMeFlag=false;
 //Winger 020304
 if (Controlling->Vel>0.0)
 {if (AIControllFlag)
   if (Controlling->DoorOpenCtrl==1)
    if (Controlling->DoorRightOpened)
     Controlling->DoorRight(false);  //Winger 090304 - jak jedzie to niech zamyka drzwi
  if (Controlling->EnginePowerSource.SourceType==CurrentCollector)
   if (AIControllFlag)
    Controlling->PantFront(true);
  if (TestFlag(Controlling->CategoryFlag,2)) //jeœli samochód
   if (fabs(Controlling->OffsetTrackH)<Controlling->Dim.W)
    if (!Controlling->ChangeOffsetH(-0.1*Controlling->Vel*dt))
     Controlling->ChangeOffsetH(0.1*Controlling->Vel*dt);
  if (Controlling->Vel>30)
   if (Controlling->EnginePowerSource.SourceType==CurrentCollector)
    if (AIControllFlag)
     Controlling->PantRear(false);
 }
 ElapsedTime+=dt;
 WaitingTime+=dt;
 fStopTime+=dt;
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
   LastUpdatedTime= LastUpdatedTime+dt;
 }
 if (AIControllFlag)
 {
  //UpdateSituation:=True;
  //tu bedzie logika sterowania
  //Ra: nie wiem czemu ReactionTime potrafi dostaæ 12 sekund, to jest przegiêcie, bo prze¿yna STÓJ
  //yB: otó¿ jest to jedna trzecia czasu nape³niania na towarowym; mo¿e siê przydaæ przy wdra¿aniu hamowania, ¿eby nie rusza³o kranem jak g³upie
  //Ra: ale nie mo¿e siê budziæ co pó³ minuty, bo prze¿yna semafory 
  if ((LastReactionTime>Min0R(ReactionTime,2.0)))
  {
   vMechLoc=pVehicles[0]->GetPosition(); //uwzglednic potem polozenie kabiny
   if (fProximityDist>=0) //jeœli jest znane
    ActualProximityDist=Min0R(fProximityDist,hypot(vMechLoc.x-vCommandLocation.x,vMechLoc.z-vCommandLocation.z)-0.5*(Controlling->Dim.L+SignalDim.L));
   else
    if (fProximityDist<0)
     ActualProximityDist=fProximityDist;
   if (Controlling->CommandIn.Command!="")
    if (!Controlling->RunInternalCommand()) //rozpoznaj komende bo lokomotywa jej nie rozpoznaje
     RecognizeCommand();
   if (Controlling->SecuritySystem.Status>1)
    if (!Controlling->SecuritySystemReset())
     if (TestFlag(Controlling->SecuritySystem.Status,s_ebrake)&&(Controlling->BrakeCtrlPos==0)&&(AccDesired>0.0))
      Controlling->DecBrakeLevel();
   switch (OrderList[OrderPos])
   {
    case Connect:
     break;
    case Disconnect: //20.07.03 - manewrowanie wagonami
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
     //na jaka odleglosc i z jaka predkoscia ma podjechac
     if (Controlling->CategoryFlag==1) //jazda pociagowa
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
      JumpToNextOrder(); //w nastêpnym jest Shunt albo Obey_train
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
      OrderPos=1; //w zerowym jest czekanie
     else
      ++OrderPos;
     break;
    case Shunt:
    case Connect:
    case Disconnect:
    case Obey_train:
    case Change_direction:
     //if (OrderList[OrderPos]&(Shunt|Connect|Disconnect)) //spokojne manewry
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
      SetDriverPsyche();
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
       PrepareEngine(); //zmieni ustawiony kierunek
       SetVelocity(20,20); //jak siê nasta³, to niech jedzie
       WaitingTime=0.0;
       fWarningDuration=1.5; //a zatr¹biæ trochê
       Controlling->WarningSignal=1;
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
       //if (iDirectionOrder==0) //jeœli na przeciwny
       // iDirection=-(Controlling->ActiveDir*Controlling->CabNo);
       //else
       iDirection=iDirectionOrder; //kierunek w³aœnie zosta³ zmieniony
       TDynamicObject *d=pVehicle; //w tym siedzi AI
       Controlling->MainSwitch(false); //dezaktywacja czuwaka
       Controlling->CabDeactivisation(); //tak jest w Train.cpp
       Controlling->DecLocalBrakeLevel(10); //zwolnienie hamulca
       //!!!! przejœæ na drug¹ stronê EN57, ET41 itp., bo inaczej jazda do ty³u?
       while (TestFlag(d->MoverParameters->Couplers[iDirectionOrder*d->DirectionGet()<0?1:0].CouplingFlag,ctrain_controll))
       {//jeœli pojazd z przodu jest ukrotniony, to przechodzimy do niego
        d=iDirectionOrder<0?d->Next():d->Prev(); //przechodzimy do nastêpnego cz³onu
        if (d?!d->Mechanik:false)
        {d->Mechanik=this; //na razie bilokacja
         if (d->DirectionGet()!=pVehicle->DirectionGet()) //jeœli s¹ przeciwne do siebie
          iDirection=-iDirection; //to bêdziemy jechaæ w drug¹ stronê wzglêdem zasiedzianego pojazdu
         pVehicle->Mechanik=NULL; //tam ju¿ nikogo nie ma
         //pVehicle->MoverParameters->CabNo=iDirection; //aktywacja wirtualnych kabin po drodze
         //pVehicle->MoverParameters->ActiveCab=0;
         //pVehicle->MoverParameters->DirAbsolute=pVehicle->MoverParameters->ActiveDir*pVehicle->MoverParameters->CabNo;
         pVehicle=d; //a mechu ma nowy pojazd (no, cz³on)
        }
        else break; //jak zajête, albo koniec sk³adu, to mechanik dalej nie idzie
       }
       Controlling=pVehicle->MoverParameters; //skrót do obiektu parametrów, moze byæ nowy
       //Ra: to prze³¹czanie poni¿ej jest tu bez sensu
       Controlling->ActiveCab=iDirection;
       //Controlling->CabNo=iDirection; //ustawienie aktywnej kabiny w prowadzonym poje¿dzie
       //Controlling->ActiveDir=0; //¿eby sam ustawi³ kierunek
       Controlling->CabActivisation();
       Controlling->IncLocalBrakeLevel(10); //zaci¹gniêcie hamulca
       PrepareEngine();
       //DirectionSet(true); //po zmianie kabiny jedziemy do przodu
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
     {
      if (fStopTime<=0) //czas postoju przed dalsz¹ jazd¹ (np. na przystanku)
       VelDesired=0.0; //jak ma czekaæ, to nie ma jazdy
      else if (VelActual<0)
       VelDesired=Controlling->Vmax; //ile fabryka dala
      else
       VelDesired=Min0R(Controlling->Vmax,VelActual);
      if (Controlling->RunningTrack.Velmax>=0)
       VelDesired=Min0R(VelDesired,Controlling->RunningTrack.Velmax); //uwaga na ograniczenia szlakowej!
      if (VelforDriver>=0)
       VelDesired=Min0R(VelDesired,VelforDriver);
      if (TrainParams->CheckTrainLatency()<10.0)
       if (TrainParams->TTVmax>0.0)
        VelDesired=Min0R(VelDesired,TrainParams->TTVmax); //jesli nie spozniony to nie szybciej niz rozkladowa
      AbsAccS=Controlling->AccS; //czy sie rozpedza czy hamuje
      if (Controlling->V<0.0) AbsAccS=-AbsAccS;
      else if (Controlling->V==0.0) AbsAccS=0.0;
      //ustalanie zadanego przyspieszenia
      if ((VelNext>=0.0)&&(ActualProximityDist>=0)&&(Controlling->Vel>=VelNext)) //gdy zbliza sie i jest za szybko do NOWEGO
      {
       if (Controlling->Vel>0.0) //jesli nie stoi
       {
        if ((Controlling->Vel<VelReduced+VelNext)&&(ActualProximityDist>fMaxProximityDist*(1+0.1*Controlling->Vel))) //dojedz do semafora/przeszkody
        {
         if (AccPreferred>0.0)
          AccDesired=0.5*AccPreferred;
          //VelDesired:=Min0R(VelDesired,VelReduced+VelNext);
        }
        else if (ActualProximityDist>fMinProximityDist)
        {//25.92 - skorektowane, zeby ladniej hamowal
         if ((VelNext>Controlling->Vel-35.0)) //dwustopniowe hamowanie - niski przy ma³ej ró¿nicy
         {if ((VelNext>0)&&(VelNext>Controlling->Vel-3.0)) //jeœli powolna i niewielkie przekroczenie
            AccDesired=0.0; //to olej (zacznij luzowaæ)
          else  //w przeciwnym wypadku
           if ((VelNext==0.0)&&(Controlling->Vel*Controlling->Vel<0.4*ActualProximityDist)) //jeœli stójka i niewielka prêdkoœæ
           {if (Controlling->Vel<30.0)  //trzymaj 30 km/h
             AccDesired=0.5;
            else
             AccDesired=0.0;
           }
           else // prostu hamuj (niski stopieñ)
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
       if (VelNext<Controlling->Vel-100.0) //lepiej zaczac hamowac}
        AccDesired=-0.2;
       else
        if ((VelNext<Controlling->Vel-70))
         AccDesired=0.0; //nie spiesz sie bo bedzie hamowanie
      //koniec wybiegu i hamowania
      //wlaczanie bezpiecznika
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
      if (Controlling->BrakeSystem==Pneumatic) //napelnianie uderzeniowe
       if (Controlling->BrakeHandle==FV4a)
       {
        if (Controlling->BrakeCtrlPos==-2)
         Controlling->BrakeCtrlPos=0;
        if ((Controlling->BrakeCtrlPos==0)&&(AbsAccS<0.0)&&(AccDesired>0.0))
        //if FuzzyLogicAI(CntrlPipePress-PipePress,0.01,1))
         if (Controlling->BrakePress>0.4)//{((Volume/BrakeVVolume/10)<0.485)})
          Controlling->DecBrakeLevel();
         else
          if (Need_BrakeRelease)
          {
           Need_BrakeRelease=false;
           //DecBrakeLevel(); //z tym by jeszcze mia³o jakiœ sens
          }
        if ((Controlling->BrakeCtrlPos<0)&&(Controlling->BrakePress<0.3))//{(CntrlPipePress-(Volume/BrakeVVolume/10)<0.01)})
         Controlling->IncBrakeLevel();          
       }

      if (AccDesired>=0.0)
       if (Prepare2press)
        Controlling->BrakeReleaser(); //wyluzuj lokomotywê
       else
        while (DecBrake());  //jeœli przyspieszamy, to nie hamujemy
             //Ra: zmieni³em 0.95 na 1.0 - trzeba ustaliæ, sk¹d sie takie wartoœci bior¹
      if ((AccDesired<=0.0)||(Controlling->Vel+VelMargin>VelDesired*1.0))
       while (DecSpeed()); //jeœli hamujemy, to nie przyspieszamy
      //yB: usuniête ró¿ne dziwne warunki, oddzielamy czêœæ zadaj¹c¹ od wykonawczej
      //zwiekszanie predkosci
      if ((AbsAccS<AccDesired)&&(Controlling->Vel<VelDesired*0.95-VelMargin)&&(AccDesired>=0.0))
      //Ra: zmieni³em 0.85 na 0.95 - trzeba ustaliæ, sk¹d sie takie wartoœci bior¹
      //if (!MaxVelFlag)
       if (!IncSpeed())
        MaxVelFlag=true;
       else
        MaxVelFlag=false;
      //if (Vel<VelDesired*0.85) and (AccDesired>0) and (EngineType=ElectricSeriesMotor)
      // and (RList[MainCtrlPos].R>0.0) and (not DelayCtrlFlag))
      // if (Im<Imin) and Ready=True {(BrakePress<0.01*MaxBrakePress)})
      //  IncMainCtrl(1); {zwieksz nastawnik skoro mozesz - tak aby sie ustawic na bezoporowej*/

      //yB: usuniête ró¿ne dziwne warunki, oddzielamy czêœæ zadaj¹c¹ od wykonawczej
      //zmniejszanie predkosci
      if ((AccDesired<-0.1)&&(AbsAccS>AccDesired+0.05))
      //if not MinVelFlag)
       if (!IncBrake())
        MinVelFlag=true;
       else
       {MinVelFlag=false;
        ReactionTime=3+0.5*(Controlling->BrakeDelay[2+2*Controlling->BrakeDelayFlag]-3);
       }
      if ((AccDesired<-0.05)&&(AbsAccS<AccDesired-0.2))
      {//jak hamuje, to nie tykaj kranu za czêsto
       DecBrake();
       ReactionTime=(Controlling->BrakeDelay[1+2*Controlling->BrakeDelayFlag])/3.0;
      }
      //Mietek-end1

      //zapobieganie poslizgowi w czlonie silnikowym
      if (Controlling->Couplers[0].Connected!=NULL)
       if (TestFlag(Controlling->Couplers[0].CouplingFlag,ctrain_controll))
        if ((*Controlling->Couplers[0].Connected)->SlippingWheels)
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
     //odhamowywanie skladu po zatrzymaniu i zabezpieczanie lokomotywy
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
  if (UpdateOK)
   if ((OrderList[OrderPos]&~(Obey_train|Shunt))==0) //tylko gdy jedzie albo czeka na rozkazy
    ScanEventTrack(); //skanowanie torów przeniesione z DynObj
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
  OrderPos=0; //przeskok do zerowej pozycji
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
 //if (Controller==AIdriver)
 //ustawienie kolejnoœci komend, niezale¿nie kto prowadzi
 //Mechanik->OrderPush(Wait_for_orders); //czekanie na lepsze czasy
 OrderPos=OrderTop=0; //wype³niamy od pozycji 0
 OrderPush(Prepare_engine); //najpierw odpalenie silnika
 if (TrainParams->TrainName==AnsiString("none"))
  OrderPush(Shunt); //jeœli nie ma rozk³adu, to manewruje
 else
 {//jeœli z rozk³adem, to jedzie na szlak
  //Mechanik->PutCommand("Timetable:"+TrainName,0,0);
  if (TrainParams?
   (AnsiString(TrainParams->TimeTable[1].StationWare).Pos("@")? //jeœli obrót na pierwszym przystanku
   (Controlling->TrainType&(dt_EZT)? //SZT równie¿! SN61 zale¿nie od wagonów...
   (AnsiString(TrainParams->TimeTable[1].StationName)==TrainParams->Relation1):false):false):true)
   OrderPush(Shunt); //a teraz start bêdzie w manewrowym, a tryb poci¹gowy w³¹czy W4
  else
  //jeœli start z pierwszej stacji i jednoczeœnie jest na niej zmiana kierunku, to EZT ma mieæ Shunt
   OrderPush(Obey_train); //dla starych scenerii start w trybie pociagowym
  WriteLog("/* "+TrainParams->ShowRelation());
  TMTableLine *t;
  for (int i=0;i<=TrainParams->StationCount;++i)
  {t=TrainParams->TimeTable+i;
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
  WriteLog("*/");
  OrderPush(Shunt); //po wykonaniu rozk³adu prze³¹czy siê na manewry
 }
 //McZapkie-100302 - to ma byc wyzwalane ze scenerii
 //Mechanik->JumpToFirstOrder();
 if (fVel==0.0)
  SetVelocity(0,0,stopSleep); //jeœli nie ma prêdkoœci pocz¹tkowej, to œpi
 else
 {
  if (fVel>=1.0) //jeœli jedzie
   iDrivigFlags|=moveStopCloser; //to do nastêpnego W4 ma podjechaæ blisko
  SetVelocity(fVel,-1); //ma ustawiæ ¿¹dan¹ prêdkoœæ
  JumpToFirstOrder();
 }
 OrdersDump(); //wypisanie kontrolne tabelki rozkazów
 //McZapkie! - zeby w ogole AI ruszyl to musi wykonac powyzsze rozkazy
 //Ale mozna by je zapodac ze scenerii
};


AnsiString __fastcall TController::StopReasonText()
{//informacja tekstowa o przyczynie zatrzymania
 return StopReasonTable[eStopReason];
};

//----------------McZapkie: skanowanie semaforow:

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
  Track->ScannedFlag=false; //do pokazywania przeskanowanych torów
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
 //Ra: AI mo¿e siê stoczyæ w przeciwnym kierunku, ni¿ oczekiwana jazda !!!! 
 vector3 sl;
 //jeœli z przodu od kierunku ruchu jest jakiœ pojazd ze sprzêgiem wirtualnym
 if (pVehicles[0]->fTrackBlock<=50.0) //jak bli¿ej ni¿ 50m, to stop
 {SetVelocity(0,0,stopBlock); //zatrzymaæ
  return; //i dalej nie ma co analizowaæ innych przypadków (!!!! do przemyœlenia)
 }
 else
  if (0.5*VelDesired*VelDesired>pVehicles[0]->fTrackBlock) //droga hamowania wiêksza ni¿ odleg³oœæ
   SetProximityVelocity(pVehicles[0]->fTrackBlock-50,0); //spowolnienie jazdy
 int startdir=iDirection; //kierunek jazdy wzglêdem pojazdu, w którym siedzi AI (1=przód,-1=ty³)
 if (startdir==0) //jeœli kabina i kierunek nie jest okreslony
  return; //nie robimy nic
 if (OrderList[OrderPos]==Shunt?OrderList[OrderPos+1]==Change_direction:false)
 {//jeœli jedzie manewrowo, ala nastêpnie w planie zmianê kierunku, to skanujemy te¿ w drug¹ stronê
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
  double scanmax=(Controlling->Vel>0.0)?150+0.1*Controlling->Vel*Controlling->Vel:500; //fabs(Mechanik->ProximityDist);
  double scandist=scanmax; //zmodyfikuje na rzeczywiœcie przeskanowane
  //Ra: znaleziony semafor trzeba zapamiêtaæ, bo mo¿e byæ wpisany we wczeœniejszy tor
  //Ra: oprócz semafora szukamy najbli¿szego ograniczenia (koniec/brak toru to ograniczenie do zera)
  TEvent *ev=NULL; //event potencjalnie od semafora
  TTrack *scantrack=TraceRoute(scandist,scandir,pVehicles[0]->RaTrackGet(),ev); //wg drugiej osi w kierunku ruchu
  if (!scantrack) //jeœli wykryto koniec toru albo zerow¹ prêdkoœæ
  {
   {//if (!Mechanik->SetProximityVelocity(0.7*fabs(scandist),0))
    // Mechanik->SetVelocity(Mechanik->VelActual*0.9,0);
    if (pVehicles[0]->fTrackBlock>50.0) //je¿eli nie ma zawalidrogi w tej odleg³oœci
    {
     //scandist=(scandist>5?scandist-5:0); //10m do zatrzymania
     vector3 pos=pVehicles[0]->AxlePositionGet()+scandist*SafeNormalize(startdir*pVehicles[0]->GetDirection());
#if LOGVELOCITY
     WriteLog("End of track:");
#endif
     if (scandist>10) //jeœli zosta³o wiêcej ni¿ 10m do koñca toru
      SetProximityVelocity(scandist,0,&pos); //informacja o zbli¿aniu siê do koñca
     else
     {PutCommand("SetVelocity",0,0,&pos,stopEnd); //na koñcu toru ma staæ
#if LOGVELOCITY
      WriteLog("-> SetVelocity 0 0");
#endif
     }
     return;
    }
   }
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
    if (pVehicles[0]->fTrackBlock>50.0)
    {
     //najpierw sprawdzamy, czy semafor czy inny znak zosta³ przejechany
     //bo jeœli tak, to trzeba szukaæ nastêpnego, a ten mo¿e go zas³aniaæ
     vector3 pos=pVehicles[0]->GetPosition(); //aktualna pozycja, potrzebna do liczenia wektorów
     vector3 dir=startdir*pVehicles[0]->GetDirection(); //wektor w kierunku jazdy/szukania
     vector3 sem; //wektor do sygna³u
     if (e->Type==tp_GetValues)
     {//przes³aæ info o zbli¿aj¹cym siê semaforze
#if LOGVELOCITY
      edir+="("+(e->Params[8].asGroundNode->asName)+"): ";
#endif
      //sem=*e->PositionGet()-pos; //wektor do komórki pamiêci
      sem=e->Params[8].asGroundNode->pCenter-pos; //wektor do komórki pamiêci
      if (dir.x*sem.x+dir.z*sem.z<0)
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
      scandist=(pos-e->Params[8].asGroundNode->pCenter).Length()-0.5*Controlling->Dim.L-10; //10m luzu
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
        if (pVehicles[0]->fTrackBlock>50.0) //je¿eli nie ma zawalidrogi w tej odleg³oœci
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
        if (pVehicles[0]->fTrackBlock>50.0) //je¿eli nie ma zawalidrogi w tej odleg³oœci
        {if ((scandist>fMinProximityDist)?(Controlling->Vel>0.0)||(vmechmax==0.0):false)
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
        }
       }
      }
     }
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
       sl.x=e->Params[3].asdouble;
       sl.y=e->Params[4].asdouble;
       sl.z=e->Params[5].asdouble;
       if (pVehicles[0]->fTrackBlock>50.0) //je¿eli nie ma zawalidrogi w tej odleg³oœci
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
        vmechmax=0.0; //ma stan¹æ na W4 - informacja dla dalszego kodu
        sl.x=e->Params[3].asdouble; //wyliczenie wspó³rzêdnych zatrzymania
        sl.y=e->Params[4].asdouble;
        sl.z=e->Params[5].asdouble;
        eSignLast=(scandist<200.0)?e:NULL; //zapamiêtanie na wypadek przejechania albo Ÿle podpiêtego toru
        if ((scandist>fMinProximityDist)?(Controlling->Vel>0.0):false)
        //if ((scandist>Mechanik->MinProximityDist)?(fSignSpeed>0.0):false)
        {//jeœli jedzie, informujemy o zatrzymaniu na wykrytym stopie
         //Mechanik->PutCommand("SetProximityVelocity",scandist,0,sl);
         if (pVehicles[0]->fTrackBlock>50.0) //je¿eli nie ma zawalidrogi w tej odleg³oœci
         {//informacjê wysy³amy, jeœli nic nie ma na kolizyjnym
#if LOGVELOCITY
         //WriteLog(edir+"SetProximityVelocity "+AnsiString(scandist)+" 0");
          WriteLog(edir);
#endif
          //SetProximityVelocity(scandist,0,&sl); //staje 300m oe W4
          SetProximityVelocity(scandist,scandist>100.0?25:0,&sl); //Ra: taka proteza
         }
        }
        else //jeœli jest blisko, albo stoi
         if ((iDrivigFlags&moveStopCloser)&&(scandist>25.0)) //jeœli ma podjechac pod W4, a jest daleko
         {
          if (pVehicles[0]->fTrackBlock>50.0) //je¿eli nie ma zawalidrogi w tej odleg³oœci
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
              iDrivigFlags|=moveStopCloser; //do nastêpnego W4 podjechaæ blisko
              vmechmax=vtrackmax; //odjazd po zatrzymaniu - informacja dla dalszego kodu
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
     } //if (e->Type==tp_PutValues)
    }
   }
   else //jeœli nic nie znaleziono
    if (OrderCurrentGet()==Shunt) //a jest w trybie manewrowym
     eSignSkip=NULL; //przywrócenie widocznoœci ewentualnie pominiêtej tarczy
   if (scandist<=scanmax) //jeœli ograniczenie jest dalej, ni¿ skanujemy, mo¿na je zignorowaæ
    if (vtrackmax>0.0) //jeœli w torze jest dodatnia
     if ((vmechmax<0) || (vtrackmax<vmechmax)) //i mniejsza od tej drugiej
     {//tutaj jest wykrywanie ograniczenia prêdkoœci w torze
      vector3 pos=pVehicles[0]->GetPosition();
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
      if (pVehicles[0]->fTrackBlock>50.0)
      {//Mechanik->PutCommand("SetProximityVelocity",dist1,vtrackmax,sl);
#if LOGVELOCITY
       //WriteLog("Track Velocity: SetProximityVelocity "+AnsiString(dist1)+" "+AnsiString(vtrackmax));
        WriteLog("Track velocity:");
#endif
       SetProximityVelocity(dist1,vtrackmax,&sl);
      }
     }
  }  // !null
 }
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
 if (yes&&(AIControllFlag!=AIdriver))
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
  SetVelocity(pVehicle->GetVelocity(),-1); //utrzymanie dotychczasowej?
  CheckVehicles(); //ustawienie œwiate³
 }
 else if (AIControllFlag==AIdriver)
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

