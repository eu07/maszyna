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
 "Error", //stopError //z powodu b³êdu w obliczeniu drogi hamowania
};

__fastcall TController::TController
(const Mover::TLocation &LocInitial,
 const Mover::TRotation &RotInitial,
 bool AI,
 TDynamicObject *NewControll,
 Mtable::TTrainParameters *NewTrainSet,
 bool InitPsyche
)
{
 EngineActive=false;
 MechLoc=LocInitial;
 MechRot=RotInitial;
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
 CommandLocation.X=0;
 CommandLocation.Y=0;
 CommandLocation.Z=0;
 VelActual=0.0; //normalnie na pocz¹tku ma staæ, no chyba ¿e jedzie
 VelNext=120.0;
 AIControllFlag=AI;
 pVehicle=NewControll;
 Controlling=pVehicle->MoverParameters; //skrót do obiektu parametrów
 Vehicles[0]=pVehicle->GetFirstDynamic(0); //pierwszy w kierunku jazdy (Np. Pc1)
 Vehicles[1]=pVehicle->GetFirstDynamic(1); //ostatni w kierunku jazdy (koñcówki)
/*
 switch (Controlling->CabNo)
 {
  case -1: SendCtrlBroadcast("CabActivisation",1); break;
  case  1: SendCtrlBroadcast("CabActivisation",2); break;
  default: AIControllFlag:=False; //na wszelki wypadek
 }
*/
 VehicleName=Controlling->Name;
 TrainSet=NewTrainSet;
 OrderCommand="";
 OrderValue=0;
 OrderPos=0;
 OrderTop=1; //szczyt stosu rozkazów
 for (int b=0;b<=maxorders;b++)
  OrderList[b]=Wait_for_orders;
 MaxVelFlag=false; MinVelFlag=false;
 iDirection=1; //normalnie do przodu (w kierunku sprzêgu 0)
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
 WaitingExpireTime=31.0;
 WaitingTime=0.0;
 fMinProximityDist=30.0; //stawanie miêdzy 30 a 60 m przed przeszkod¹
 fMaxProximityDist=50.0;
 iVehicleCount=-2;
 Prepare2press=false;
 eStopReason=stopSleep; //na pocz¹tku œpi
 fLength=0.0;
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
 return "Undefined!";
}

AnsiString __fastcall TController::OrderCurrent()
{//pobranie aktualnego rozkazu celem wyœwietlenia
 return AnsiString(OrderPos)+". "+Order2Str(OrderList[OrderPos]);
};

bool __fastcall TController::CheckSKP()
{//sprawdzenie stanu posiadanych pojazdów w sk³adzie i zapalenie œwiate³
 if (!bCheckVehicles) return false; //nie trzeba sprawdzaæ œwiate³
 //ZiomalCl: sprawdzanie i zmiana SKP w skladzie prowadzonym przez AI
 ResetSKP(); //nie trzeba po raz kolejny
 TDynamicObject* p; //zmienne robocze
 int i=0; //iloœæ pojazdów w sk³adzie
 int d=iDirection>0?0:1; //kierunek szukania czo³a (numer sprzêgu)
 Vehicles[0]=p=pVehicle->FirstFind(d); //pojazd na czele sk³adu
 //liczenie i ustawianie kierunku
 d=1-d; //bêdziemy zliczaæ od czo³a do ty³u
 fLength=0.0;
 while (p)
 {
  ++i; //jest jeden pojazd wiêcej
  Vehicles[1]=p; //zapamiêtanie ostatniego
  fLength+=p->MoverParameters->Dim.L; //dodanie d³ugoœci pojazdu
  p->RaLightsSet(0,0); //gasimy œwiat³a
  d=p->DirectionSet(d); //zwraca po³o¿enie nastêpnego (1=zgodny,0=odwrócony)
  p=p->Next(); //pojazd pod³¹czony od ty³u (licz¹c od czo³a)
 }
/* //tabelka z list¹ pojazdów jest na razie nie potrzebna
 if (i!=)
 {delete[] pVehicle
 }
*/
 if (OrderCurrentGet()==Obey_train) //jeœli jazda poci¹gowa
 {Vehicles[0]->RaLightsSet(1+4+16,-1); //zapalenie przednich w pierwszym
  Vehicles[1]->RaLightsSet(-1,2+32); //zapalenie koñcówek w ostatnim
 }
 else if (OrderCurrentGet()>Wait_for_orders)
 {Vehicles[0]->RaLightsSet(16,-1); //zapalenie przednich w pierwszym
  Vehicles[1]->RaLightsSet(-1,0); //zapalenie koñcówek w ostatnim
 }
 return bCheckVehicles;
}

void __fastcall TController::ResetSKP()
{//skasowanie potrzeby za³o¿enia SKP
 bCheckVehicles=false;
}

int __fastcall TController::OrderDirectionChange(int newdir,Mover::TMoverParameters *Vehicle)
{//zmiana kierunku
 int testd;
 testd=newdir;
 if (Vehicle->Vel<0.1)
 {
  switch (newdir)
  {
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
 if (Vehicle->ActiveDir==testd)
  VelforDriver=-1;
 if ((Vehicle->ActiveDir>0)&&(Vehicle->TrainType==dt_EZT))
  Vehicle->DirectionForward(); //Ra: z przekazaniem do silnikowego
 return (int)VelforDriver;
}

void __fastcall TController::WaitingSet(double Seconds)
{//ustawienie odczekania
 WaitingTime=-Seconds;
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
    WaitingExpireTime=11;
   else
    WaitingExpireTime=61;
 }
 else
 {
  ReactionTime=EasyReactionTime; //spokojny
  AccPreferred=EasyAcceleration;
  WaitingExpireTime=65;
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
    AccPreferred=((Mover::TMoverParameters*)(Controlling->Couplers[0].Connected))->V; //tymczasowa wartoœæ
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
 {
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
  if (Controlling->V==0)
  {//ustalenie kierunku, gdy stoi
   if (Controlling->Couplers[1].CouplingFlag==ctrain_virtual) //jeœli z ty³u nie ma nic
    if (Controlling->PantFrontVolt||Controlling->PantRearVolt||voltfront||voltrear)
     iDirection=-1; //jazda w kierunku sprzêgu 1
   if (Controlling->Couplers[0].CouplingFlag==ctrain_virtual) //jeœli z przodu nie ma nic
    if (Controlling->PantFrontVolt||Controlling->PantRearVolt||voltfront||voltrear)
     iDirection=1; //jazda w kierunku sprzêgu 0
  }
  else //ustalenie kierunku, gdy jedzie
   if (Controlling->V<0) //jedzie do ty³u
    if (Controlling->PantFrontVolt||Controlling->PantRearVolt||voltfront||voltrear)
     iDirection=-1; //jazda w kierunku sprzêgu 1
    else //jak nie do ty³u, to do przodu
     if (Controlling->PantFrontVolt||Controlling->PantRearVolt||voltfront||voltrear)
      iDirection=1; //jazda w kierunku sprzêgu 0
 }
 else
  OK=false;
 OK=OK&&(Controlling->ActiveDir!=0)&&(Controlling->CompressorAllow);
 if (OK)
 {
  if (eStopReason==stopSleep) //jeœli dotychczas spa³
   eStopReason==stopNone; //teraz nie ma powodu do stania
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
 if (OK&&(Controlling->Vel<0.01)) EngineActive=false;
 eStopReason=stopSleep; //stoimy z powodu wy³¹czenia
 return OK&&(Controlling->Vel<0.01);
}

bool __fastcall TController::IncBrake()
{//zwiêkszenie hamowania
 bool OK=false;
 //ClearPendingExceptions;
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
 //ClearPendingExceptions;
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
 //ClearPendingExceptions;
 if ((Controlling->DoorOpenCtrl==1)&&(Controlling->Vel==0)&&(Controlling->DoorLeftOpened||Controlling->DoorRightOpened))  //AI zamyka drzwi przed odjazdem
 {
  Controlling->DepartureSignal=true; //za³¹cenie bzyczka
  Controlling->DoorLeft(false); //zamykanie drzwi
  Controlling->DoorRight(false);
  //DepartureSignal:=false;
  //Ra: trzeba by ustawiæ jakiœ czas oczekiwania na zamkniêcie siê drzwi
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
     if ((Controlling->Im<=Controlling->Imin)&&Ready) //{(Controlling->BrakePress<=0.01*Controlling->MaxBrakePress)}
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
    if (Ready) //{(BrakePress<=0.01*MaxBrakePress)}
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
 if (NewCommand=="SetVelocity")
 {
  CommandLocation=NewLocation;
  if (NewValue1!=0.0)
  {//o ile jazda
   if (!EngineActive)
    OrderNext(Prepare_engine); //trzeba odpaliæ silnik najpierw
   OrderNext(Obey_train); //a nastêpnie uruchomiæ jazdê
   bCheckVehicles=true; //sprawdziæ stan posiadania i zapaliæ œwiat³a
  }
  SetVelocity(NewValue1,NewValue2,reason); //bylo: nic nie rob bo SetVelocity zewnetrznie jest wywolywane przez dynobj.cpp
 }
 else if (NewCommand=="SetProximityVelocity")
 {
  if (SetProximityVelocity(NewValue1,NewValue2))
   CommandLocation=NewLocation;
  //if Order=Shunt) Order=Obey_train;
 }
 else if (NewCommand=="ShuntVelocity")
 {//uruchomienie jazdy manewrowej b¹dŸ zmiana prêdkoœci
  CommandLocation=NewLocation;
  //if (OrderList[OrderPos]=Obey_train) and (NewValue1<>0))
  if (!EngineActive)
   OrderNext(Prepare_engine); //trzeba odpaliæ silnik najpierw
  OrderNext(Shunt); //zamieniamy w aktualnej pozycji
  if (NewValue1!=0)
   iVehicleCount=-2;
  Prepare2press=false;
  SetVelocity(NewValue1,NewValue2,reason);
  if (NewValue1!=0.0)
  {bCheckVehicles=true; //sprawdziæ œwiat³a
   fShuntVelocity=fabs(NewValue1); //zapamiêtanie obowi¹zuj¹cej prêdkoœci dla manewrów
  }
 }
 else if (NewCommand=="Wait_for_orders")
 {//oczekiwanie
  OrderList[OrderPos]=Wait_for_orders;
  //NewValue1 - czas oczekiwania, -1 - na inn¹ komendê
 }
 else if (NewCommand=="Prepare_engine")
 {
  if (NewValue1==0.0)
   OrderNext(Release_engine); //wy³¹czyæ silnik
  else if (NewValue1>0.0)
   OrderNext(Prepare_engine); //odpaliæ silnik
 }
 else if (NewCommand=="Change_direction")
 {
  if (!EngineActive)
   OrderNext(Prepare_engine); //trzeba odpaliæ silnik najpierw
  if (NewValue1>0.0) iDirectionOrder=1;
  else if (NewValue1<0.0) iDirectionOrder=-1;
  else iDirectionOrder=-iDirection; //zmiana na przeciwny ni¿ obecny
  OrderNext(Change_direction);
 }
 else if (NewCommand=="Obey_train")
 {
  if (!EngineActive)
   OrderNext(Prepare_engine); //trzeba odpaliæ silnik najpierw
  OrderNext(Obey_train);
  if (NewValue1>0)
   TrainNumber=floor(NewValue1); //i co potem ???
 }
 else if (NewCommand.Pos("Obey:")==1)
 {//przypisanie nowego rozk³adu jazdy
  if (!EngineActive)
   OrderNext(Prepare_engine); //trzeba odpaliæ silnik najpierw
  OrderNext(Obey_train); //¿e do jazdy
  NewCommand.Delete(1,5); //zostanie nazwa pliku z rozk³adem
  TrainSet->NewName(NewCommand);
  if (NewCommand!="none")
   if (!TrainSet->LoadTTfile(Global::asCurrentSceneryPath))
    Error("Cannot load timetable file "+NewCommand+"\r\nError "+ConversionError+" in position "+TrainSet->StationCount);
   else
   {//inicjacja pierwszego przystanku i pobranie jego nazwy
    TrainSet->UpdateMTable(GlobalTime->hh,GlobalTime->mm,TrainSet->NextStationName);
    pVehicle->asNextStop=TrainSet->NextStop();
    WriteLog("/* Timetable: "+TrainSet->ShowRelation());
    TMTableLine *t;
    for (int i=0;i<=TrainSet->StationCount;++i)
    {t=TrainSet->TimeTable+i;
     WriteLog(AnsiString(t->StationName)+" "+AnsiString((int)t->Ah)+":"+AnsiString((int)t->Am)+", "+AnsiString((int)t->Dh)+":"+AnsiString((int)t->Dm));
    }
    WriteLog("*/");
   }
  if (NewValue1>0)
   TrainNumber=floor(NewValue1); //i co potem ???
 }
 else if (NewCommand=="Shunt")
 {//NewValue1 - iloœæ wagonów (-1=wszystkie), NewValue2: 0=odczep, 1=do³¹cz, -1=bez zmian
  if (!EngineActive)
   OrderNext(Prepare_engine); //trzeba odpaliæ silnik najpierw
  if (NewValue2>0)
   OrderNext(Connect); //po³¹cz (NewValue2) wagonów
  else if (NewValue2==0.0)
   OrderNext(Disconnect); //odczep (NewValue2) wagonów
  OrderNext(Shunt); //i manewruj dalej
  bCheckVehicles=true; //sprawdziæ œwiat³a
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
   if ((NewValue1>=0)&&(NewValue1<=maxorders))
    OrderPos=floor(NewValue1);
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
{//uruchamiac przynajmniej raz na sekunde
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
 HelpMeFlag=false;
 //Winger 020304
 if (Controlling->Vel>0)
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
 }
 if (Controlling->EnginePowerSource.SourceType==CurrentCollector)
  if (Controlling->Vel>30)
   if (AIControllFlag)
    Controlling->PantRear(false);
 ElapsedTime=ElapsedTime+dt;
 WaitingTime=WaitingTime+dt;
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
  if ((LastReactionTime>Min0R(ReactionTime,2.0)))
  {
   MechLoc=Controlling->Loc; //uwzglednic potem polozenie kabiny
   if (fProximityDist>=0) //jeœli jest znane
    ActualProximityDist=Min0R(fProximityDist,Distance(MechLoc,CommandLocation,Controlling->Dim,SignalDim));
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
   {//na jaka odleglosc i z jaka predkoscia ma podjechac
    case Shunt:
     fMinProximityDist=1.0; fMaxProximityDist=10.0; //[m]
     VelReduced=5; //[km/h]
/*
     if (Controlling->Vel==0)
      if (Controlling->ActiveDir*Controlling->CabNo<0)
      {
       if (Controlling->Couplers[1].Connected==NULL)
        Controlling->HeadSignalsFlag=16; //swiatla manewrowe
       if (Controlling->Couplers[0].Connected==NULL)
        Controlling->EndSignalsFlag=1;
      }
      else
      {
       if (Controlling->Couplers[1].Connected==NULL)
        Controlling->HeadSignalsFlag=1; //swiatla manewrowe
       if (Controlling->Couplers[0].Connected==NULL)
        Controlling->EndSignalsFlag=16;
      }
*/
      //20.07.03 - manewrowanie wagonami
      if (iVehicleCount>=0)
      {
       if (Prepare2press)
       {
        SetVelocity(3,0);
        if (Controlling->MainCtrlPos>0)
        {
         if ((Controlling->ActiveDir>0)&&(Controlling->Couplers[0].CouplingFlag>0))
         {
          Controlling->Dettach(0);
          iVehicleCount=-2;
          SetVelocity(0,0,stopJoin);
          bCheckVehicles=true;
         }
         else
          if ((Controlling->ActiveDir<0)&&(Controlling->Couplers[1].CouplingFlag>0))
          {
           Controlling->Dettach(1);
           iVehicleCount=-2;
           SetVelocity(0,0,stopJoin);
          }
          else HelpMeFlag=true; //cos nie tak z kierunkiem dociskania
        }
        if (iVehicleCount>=0)
         if (!Controlling->DecLocalBrakeLevel(1))
         {
          Controlling->BrakeReleaser();
          IncSpeed(); //docisnij sklad
         }
       }
       if ((Controlling->Vel==0)&&!Prepare2press)
       {//zmieñ kierunek na przeciwny
        Prepare2press=true;
        if (Controlling->ActiveDir>=0)
         while (Controlling->ActiveDir>=0)
          Controlling->DirectionBackward();
        else
         while (Controlling->ActiveDir<=0)
          Controlling->DirectionForward();
       }
      } //odczepiania
      else
       if (Prepare2press&&!DecSpeed()) //nie ma nic do odczepiania
        {//ponowna zmiana kierunku
         if (Controlling->ActiveDir>=0)
          while (Controlling->ActiveDir>=0)
           Controlling->DirectionBackward();
          else
           while (Controlling->ActiveDir<=0)
            Controlling->DirectionForward();
         Prepare2press=false;
         SetVelocity(10,10);
        }
     break;
    case Obey_train:
     if (Controlling->CategoryFlag==1) //jazda pociagowa
     {
      fMinProximityDist=30.0; fMaxProximityDist=60.0; //[m]
/*
      if ((Controlling->ActiveDir*Controlling->CabNo<0))
      {
       if ((Controlling->Couplers[0].CouplingFlag==0))
        Controlling->HeadSignalsFlag=1+4+16; //wlacz trojkat
       if ((Controlling->Couplers[1].CouplingFlag==0))
        Controlling->EndSignalsFlag=2+32; //swiatla konca pociagu i/lub blachy
      }
      else
      {
       if ((Controlling->Couplers[0].CouplingFlag==0))
        Controlling->EndSignalsFlag=1+4+16; //wlacz trojkat
       if ((Controlling->Couplers[1].CouplingFlag==0))
        Controlling->HeadSignalsFlag=2+32; //swiatla konca pociagu i/lub blachy
      }
*/
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
    case Obey_train:
    case Change_direction:
     if (OrderList[OrderPos]==Shunt) //spokojne manewry
     {
      VelActual=Min0R(VelActual,40); //jeœli manewry, to ograniczamy prêdkoœæ
      if ((iVehicleCount>=0)&&!Prepare2press)
       SetVelocity(0,0,stopJoin); //dorobic potem doczepianie
     }
     else
      SetDriverPsyche();
     //no albo przypisujemy -WaitingExpireTime, albo porównujemy z WaitingExpireTime
     if ((VelActual==0.0)&&(WaitingTime>WaitingExpireTime)&&(Controlling->RunningTrack.Velmax!=0.0))
     {//jeœli stoi, a up³yn¹³ czas oczekiwania i tor ma niezerow¹ prêdkoœæ
/*
      if (WriteLogFlag)
       {
         append(AIlogFile);
         writeln(AILogFile,ElapsedTime:5:2,": ",Name," V=0 waiting time expired! (",WaitingTime:4:1,")");
         close(AILogFile);
       }
*/
      PrepareEngine();
      WaitingTime=0.0;
      SetVelocity(20,20); //jak siê nasta³, to niech jedzie
      fWarningDuration=1.5; //a zatr¹biæ trochê
      Controlling->WarningSignal=1;
     }
     else if ((VelActual==0.0)&&(VelNext>0.0)&&(Controlling->Vel<1.0))
      SetVelocity(VelNext,VelNext,stopSem); //omijanie SBL
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
      if (Controlling->Vel<0.1) //     if (VelActual<0.1)
      {//jeœli siê zatrzyma³, to zmieniamy kierunek jazdy, a nawet kabinê/cz³on
       //if (iDirectionOrder==0) //jeœli na przeciwny
       // iDirection=-(Controlling->ActiveDir*Controlling->CabNo);
       //else
       iDirection=iDirectionOrder; //kierunek w³aœnie zosta³ zmieniony
       //!!!! przejœæ na drug¹ stronê EN57
       Controlling->CabNo=iDirection;
       Controlling->ActiveCab=iDirection;
       Controlling->PantFront(true);
       Controlling->PantRear(true);
       //Controlling->CommandIn.Value1=-1;
       //Controlling->CommandIn.Value2=-1;
       JumpToNextOrder(); //nastêpnie robimy, co jest do zrobienia
       if (OrderList[OrderPos]==Shunt) //jeœli dalej mamy manewry
        SetVelocity(fShuntVelocity,fShuntVelocity); //to od razu jedziemy
/*
       if (WriteLogFlag)
       {
        append(AIlogFile);
        writeln(AILogFile,ElapsedTime:5:2,": ",Name," Direction changed!");
        close(AILogFile);
       }
*/
/*
       if ((Controlling->ActiveDir*Controlling->CabNo<0))
       {
        Controlling->HeadSignalsFlag=16; //swiatla manewrowe
        Controlling->EndSignalsFlag= 1;
       }
       else
       {
        Controlling->HeadSignalsFlag=1; //swiatla manewrowe
        Controlling->EndSignalsFlag=16;
       }
*/
      }
      //else
      // VelActual:=0.0; //na wszelki wypadek niech zahamuje
     } //Change_direction
     //ustalanie zadanej predkosci
     if ((Controlling->ActiveDir!=0))
     {
      if (VelActual<0)
       VelDesired=Controlling->Vmax; //ile fabryka dala
      else
       VelDesired=Min0R(Controlling->Vmax,VelActual);
      if (Controlling->RunningTrack.Velmax>=0)
       VelDesired=Min0R(VelDesired,Controlling->RunningTrack.Velmax); //uwaga na ograniczenia szlakowej!
      if (VelforDriver>=0)
       VelDesired=Min0R(VelDesired,VelforDriver);
      if (TrainSet->CheckTrainLatency()<10.0)
       if (TrainSet->TTVmax>0.0)
        VelDesired=Min0R(VelDesired,TrainSet->TTVmax); //jesli nie spozniony to nie szybciej niz rozkladowa
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
       AccDesired=AccPreferred; //normalna jazda
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
      if (AccDesired>=0.0)
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
        if (((Mover::TMoverParameters*)(Controlling->Couplers[0].Connected))->SlippingWheels)
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
   LastReactionTime=LastReactionTime+dt;
  if (fWarningDuration>0.0) //jeœli pozosta³o coœ do wytr¹bienia
  {//tr¹bienie trwa nadal
   fWarningDuration=fWarningDuration-dt;
   if (fWarningDuration<0.1)
    Controlling->WarningSignal=0.0; //a tu siê koñczy
  }
  return UpdateOK;
 }
 else //if (AIControllFlag)
  return false; //AI nie obs³uguje
}

void __fastcall TController::JumpToNextOrder()
{
 if (OrderList[OrderPos]!=Wait_for_orders)
 {
  if (OrderPos<maxorders)
   ++OrderPos;
  else
   OrderPos=0;
 }
 if (OrderList[OrderPos]>=Shunt)
  bCheckVehicles=true;
};

void __fastcall TController::JumpToFirstOrder()
{//taki relikt
 OrderPos=1;
 if (OrderTop==0) OrderTop=1;
}

void __fastcall TController::OrderNext(TOrders NewOrder)
{//ustawienie rozkazu do wykonania jako nastêpny
 if (OrderList[OrderPos]==NewOrder)
  return; //jeœli robi to, co trzeba, to koniec
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
  switch (OrderList[OrderPos])
  {//niektóre operacje musz¹ zostaæ dokoñczone
   case Prepare_engine: //jeœli w trakcie odpalania
   case Change_direction: //jeœli w trakcie zmiany kierunku
   case Release_engine: //wy³¹czenie silnika
    ++OrderTop; //wstawimy do nastêpnej pozycji
    break;
   case Shunt: //jeœli doczepianie/odczepianie w trakcie wykonywania
    break;
  }
 if (OrderList[OrderTop]!=NewOrder) //jeœli jest to samo, to nie dodajemy
  OrderList[OrderTop++]=NewOrder; //dodanie rozkazu na stos
 //if (OrderTop<OrderPos) OrderTop=OrderPos;
}

TOrders __fastcall TController::OrderCurrentGet()
{
 return OrderList[OrderPos];
}

AnsiString __fastcall TController::StopReasonText()
{//informacja tekstowa o przyczynie zatrzymania
 return StopReasonTable[eStopReason];
};


