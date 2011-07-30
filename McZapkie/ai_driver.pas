unit ai_driver;

//    MaSzyna EU07 locomotive simulator
//    Copyright (C) 2001-2004  Maciej Czapkiewicz and others

//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.

//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.

//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA




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

interface

uses
  mtable,mover,mctools;

const
  Aggressive=True;
  Easyman=False;
  AIdriver=True;
  Humandriver=False;
  maxorders=16;
  maxdriverfails=4;
  WriteLogFlag:boolean=False;

Type
   TOrders = (Wait_for_orders,Prepare_engine,Shunt,Change_direction,Obey_train,Release_engine,Jump_to_first_order);
   TController = class(TObject)
                  EngineActive: boolean; {ABu: Czy silnik byl juz zalaczony}
                  MechLoc: TLocation;
                  MechRot: TRotation;
                  Psyche: boolean;
                  ReactionTime: real;
                  Ready: boolean; {ABu: stan gotowosci do odjazdu - sprawdzenie odhamowania wagonow
                                        jest ustawiane w dynobj.cpp}
                  {czas reakcji}
                  LastUpdatedTime, ElapsedTime, deltalog: real;
                  {czas od ostatniego logu, czas od poczatku logu, przyrost czasu}
                  LastReactionTime: real;
                  {wystawiane True jesli cos niedobrego sie dzieje}
                  HelpMeFlag: boolean;
                  {adres gracza}
                {?  PlayerIP: IPAddress;  }
                  {rzeczywisty/wirtualny maszynista}
                  AIControllFlag: boolean;
                  {Czy jest na peronie}
                  OnStationFlag: boolean;
                  {jakim pojazdem steruje}
                  Controlling: PMoverParameters;
                  {do jakiego pociagu nalezy}
                  TrainSet: PTrainParameters;
                  {numer rozkladowy tego pociagu}
                  TrainNumber: integer;
                  {komenda pobierana z pojazdu}
                  OrderCommand: string;
                  {argument komendy}
                  OrderValue: real;
                  {preferowane przyspieszenie}
                  AccPreferred: real;
                  {chwilowe przyspieszenie, predkosc, predkosc dla manewrow}
                  AccDesired: real; VelDesired:real; VelforDriver: real;
                  {predkosc do ktorej dazy}
                  VelActual : real;
                  {predkosc przy nastepnym obiekcie}
                  VelNext: real;
                  {odleglosc od obiektu, ujemna jesli nieznana}
                  ProximityDist, ActualProximityDist: real;
                  {ustawia nowa predkosc do ktorej ma dazyc oraz predkosc przy nastepnym obiekcie}
                  CommandLocation: TLocation;
                  {polozenie wskaznika, sygnalizatora lub innego obiektu do ktorego odnosi sie komenda}
                  OrderList: array[0..maxorders] of TOrders; //lista rozkazów
                  OrderPos: byte; //aktualny rozkaz
                  LogFile: text;   {zapis parametrow fizycznych}
                  AILogFile: text; {log AI}
                  ScanMe: boolean; {flaga potrzeby skanowania toru dla DynObj.cpp}
{inne}
                  MaxVelFlag,MinVelFlag:boolean;
                  ChangeDir,ChangeDirOrder: integer;
                  VehicleCount: integer;
                  Prepare2press: boolean;
                  DriverFailCount: integer;
                  Need_TryAgain, Need_BrakeRelease: boolean;
                  MinProximityDist, MaxProximityDist: real;
									bCheckSKP: boolean; 
{funkcje}
                  procedure SetDriverPsyche;
                  function PrepareEngine: boolean;
                  function ReleaseEngine: boolean;
                  function IncBrake: boolean;
                  function DecBrake: boolean;
                  function IncSpeed: boolean;
                  function DecSpeed: boolean;
                  procedure RecognizeCommand; {odczytuje komende przekazana lokomotywie}
                  procedure PutCommand(NewCommand:string; NewValue1,NewValue2:real; NewLocation:TLocation);
                  function UpdateSituation(dt:real):boolean;   {!uruchamiac przynajmniej raz na sekunde}
                  {procedury dotyczace rozkazow dla maszynisty}
                  procedure SetVelocity(NewVel,NewVelNext:real);
                  {uaktualnia informacje o predkosci}
                  function SetProximityVelocity(NewDist,NewVelNext: real): boolean;
                  {uaktualnia informacje o predkosci przy nastepnym semaforze}
                  procedure JumpToNextOrder;
                  procedure JumpToFirstOrder;
                  procedure ChangeOrder(NewOrder:TOrders);
                  function GetCurrentOrder: TOrders;
                  procedure CloseLog;
                  constructor Init(LocInitial:TLocation; RotInitial:TRotation;
                                   AI:boolean; NewControll:PMoverParameters; NewTrainSet:PTRainParameters; InitPsyche:boolean);
                  function OrderCurrent:string;
private
                  VehicleName: string;
                  VelMargin: real;
                  WarningDuration: real;
                  WaitingTime, WaitingExpireTime:real;
                  function OrderDirectionChange(newdir:integer; Vehicle:PMoverParameters):integer;
end;


implementation

const
   EasyReactionTime=0.4;
   HardReactionTime=0.2;
   EasyAcceleration=0.5;
   HardAcceleration=0.9;
   PrepareTime=2.0;

function Order2Str(Order:TOrders):string;
begin
  case Order of
    Wait_for_orders: Order2Str:='Wait_for_orders';
    Prepare_engine: Order2Str:='Prepare_engine';
    Shunt: Order2Str:='Shunt';
    Change_direction: Order2Str:='Change_direction';
    Obey_train: Order2Str:='Obey_train';
    Release_engine: Order2Str:='Release_engine';
    Jump_to_first_order: Order2Str:='Jump_to_first_order';
   else
    Order2Str:='Undefined!';
  end;
end;

function TController.OrderCurrent:string;
//pobranie aktualnego rozkazu celem wyœwietlenia
begin
 OrderCurrent:=Order2Str(OrderList[OrderPos]);
end;

function TController.OrderDirectionChange(newdir:integer; Vehicle:PMoverParameters):integer;
var testd:integer;
begin
  testd:=newdir;
  with Vehicle^ do
   begin
    if Vel<0.1 then
     begin
      case newdir of
      -1 : if not DirectionBackward then
            testd:=0;
       1 : if not DirectionForward then
            testd:=0;
      end;
      if testd=0 then
       VelforDriver:=-1;
     end
    else
    VelforDriver:=0;
    if (ActiveDir=0) and (VelforDriver<Vel) then
     IncBrake;
    if ActiveDir=testd then
     VelforDriver:=-1;
     if (ActiveDir<>0) and (TrainType=dt_EZT) then Imin:=IminHi;
   end;
  OrderDirectionChange:=Round(VelforDriver);
end;

procedure TController.SetVelocity(NewVel,NewVelNext:real);
//ustawienie nowej prêdkoœci
begin
 WaitingTime:=-WaitingExpireTime;
 MaxVelFlag:=False; MinVelFlag:=False;
 VelActual:=NewVel;   //prêdkoœæ oczekiwana
 VelNext:=NewVelNext; //prêdkoœæ nastêpna
 if (NewVel>NewVelNext) //jeœli oczekiwana wiêksza ni¿ nastêpna
  or (NewVel<Controlling^.Vel) //albo aktualna jest mniejsza ni¿ aktualna
 then
  ProximityDist:=-800 //droga hamowania do zmiany prêdkoœci
 else
  ProximityDist:=-300;
end;

function TController.SetProximityVelocity(NewDist,NewVelNext:real):boolean;
//informacja o ograniczeniu w pobli¿u
begin
 if NewVelNext=0 then
   WaitingTime:=0;
{if ((NewVelNext>=0) and ((VelNext>=0) and (NewVelNext<VelNext)) or (NewVelNext<VelActual)) or (VelNext<0) then
  begin }
   MaxVelFlag:=False; MinVelFlag:=False;
   VelNext:=NewVelNext;
   ProximityDist:=NewDist;
   SetProximityVelocity:=true;
{end
  else
   SetProximityVelocity:=false; }
end;

procedure TController.SetDriverPsyche;
const maxdist=0.5; {skalowanie dystansu od innego pojazdu, zmienic to!!!}
begin
  if (Psyche=Aggressive) and (OrderList[OrderPos]=Obey_train) then
   begin
     ReactionTime:=HardReactionTime; {w zaleznosci od charakteru maszynisty}
     AccPreferred:=HardAcceleration; {agresywny}
     if Controlling<>nil then
      if Controlling^.CategoryFlag=2 then
       WaitingExpireTime:=11
      else
       WaitingExpireTime:=61
   end
  else
   begin
     ReactionTime:=EasyReactionTime; {spokojny}
     AccPreferred:=EasyAcceleration;
     WaitingExpireTime:=65
   end;
  if Controlling<>nil then
   with Controlling^ do
    begin
      if MainCtrlPos<3 then
        ReactionTime:=InitialCtrlDelay+ReactionTime;
      if BrakeCtrlPos>1 then
       ReactionTime:=0.5*ReactionTime;
      if (V>0.1) and (Couplers[0].Connected<>NIL) then  {dopisac to samo dla V<-0.1 i zaleznie od Psyche}
       if Couplers[0].CouplingFlag=0 then
        begin
         AccPreferred:=(SQR(Couplers[0].Connected^.V)-SQR(V))/(25.92*(Couplers[0].Dist-maxdist*abs(v)));
         AccPreferred:=Min0R(AccPreferred,EasyAcceleration);
        end;
    end;
end;

function TController.PrepareEngine: boolean;
var OK:boolean;
var voltfront, voltrear: boolean;
begin
  voltfront:=false;
  voltrear:=false;
  LastReactionTime:=0;
  ReactionTime:=PrepareTime;
  with Controlling^ do
   begin
   if ((EnginePowerSource.SourceType=CurrentCollector) or (TrainType=dt_EZT)) then
     begin
       if(GetTrainsetVoltage) then
          begin
             voltfront:=true;
             voltrear:=true;
          end;
     end
//   begin
//     if Couplers[0].Connected<>nil then
//     begin
//       if Couplers[0].Connected^.PantFrontVolt or Couplers[0].Connected^.PantRearVolt then
//         voltfront:=true
//       else
//         voltfront:=false;
//     end
//     else
//        voltfront:=false;
//     if Couplers[1].Connected<>nil then
//     begin
//      if Couplers[1].Connected^.PantFrontVolt or Couplers[1].Connected^.PantRearVolt then
//        voltrear:=true
//      else
//        voltrear:=false;
//     end
//     else
//        voltrear:=false;
//   end
   else
      //if EnginePowerSource.SourceType<>CurrentCollector then
      if TrainType<>dt_EZT then
       voltfront:=true;
     if EnginePowerSource.SourceType=CurrentCollector then
      begin
       PantFront(true);
       PantRear(true);
      end;
     if TrainType=dt_EZT then
      begin
       PantFront(true);
       PantRear(true);
      end;
     if DoorOpenCtrl=1 then
      if not DoorRightOpened then
        DoorRight(true);  //McZapkie: taka prowizorka bo powinien wiedziec gdzie peron

//voltfront:=true;
if (PantFrontVolt or PantRearVolt or voltfront or voltrear) then
  begin
     if not Mains then
       begin
       //if TrainType=dt_SN61 then
       //   begin
       //      OK:=(OrderDirectionChange(ChangeDir,Controlling)=-1);
       //      OK:=IncMainCtrl(1);
       //   end;
          OK:=MainSwitch(true);
       end
     else
       begin
         OK:=(OrderDirectionChange(ChangeDir,Controlling)=-1);
         CompressorSwitch(true);
         ConverterSwitch(true);
         CompressorSwitch(true);
       end;
      if v=0 then
       begin
         if Couplers[1].CouplingFlag=ctrain_virtual then
          if PantFrontVolt or PantRearVolt or voltfront or voltrear then
           ChangeDir:=-1;
         if Couplers[0].CouplingFlag=ctrain_virtual then
          if PantFrontVolt or PantRearVolt or voltfront or voltrear then
           ChangeDir:=1;
       end
      else
       if v<0 then
        if PantFrontVolt or PantRearVolt or voltfront or voltrear then
         ChangeDir:=-1
       else
        if PantFrontVolt or PantRearVolt or voltfront or voltrear then
         ChangeDir:=1;
  end
else
  OK:=false;
     OK:=OK and (ActiveDir<>0) and (CompressorAllow);
     if OK then
      begin
            with Controlling^ do
                PrepareEngine:=true;
            EngineActive:=true;
      end
     else
      begin
            with Controlling^ do
                PrepareEngine:=false;
            EngineActive:=false;
      end;
   end;
end;

function TController.ReleaseEngine: boolean;
var OK:boolean;
begin
  OK:=False;
  LastReactionTime:=0;
  ReactionTime:=PrepareTime;
  with Controlling^ do
   begin
     if DoorOpenCtrl=1 then
      begin
       if DoorLeftOpened then
        DoorLeft(false);
       if DoorRightOpened then
        DoorRight(false);
      end;
     if (ActiveDir=0) then
      if MainS then
       begin
       CompressorSwitch(false);
       ConverterSwitch(false);
       if EnginePowerSource.SourceType=CurrentCollector then
        begin
         PantFront(false);
         PantRear(false);
        end;
       OK:=MainSwitch(false);
       end
      else
       OK:=True;
     if not DecBrakeLevel then
      if not IncLocalBrakeLevel(1) then
      begin
        if ActiveDir=1 then
         if not decscndctrl(2) then
          if not decmainctrl(2) then
           DirectionBackward;
        if ActiveDir=-1 then
         if not decscndctrl(2) then
          if not decmainctrl(2) then
           DirectionForward;
      end;
    if (OK and (Vel<0.01)) then EngineActive:=false;
    ReleaseEngine:=OK and (Vel<0.01);
   end;
end;

function TController.IncBrake: boolean;
var OK:boolean;
begin
  ClearPendingExceptions;
  OK:=False;
  with Controlling^ do
   begin
     case BrakeSystem of
      Individual:
       begin
         OK:=IncLocalBrakeLevel(1+Trunc(abs(AccDesired)));
       end;
      Pneumatic:
       begin
         if (Couplers[0].Connected=nil) and (Couplers[1].Connected=nil) then
          OK:=IncLocalBrakeLevel(1+Trunc(abs(AccDesired))) {hamowanie lokalnym bo luzem jedzie}
         else
          begin
           if BrakeCtrlPos+1=BrakeCtrlPosNo then
            begin
              if AccDesired<-1.5 then  {hamowanie nagle}
               OK:=IncBrakeLevel
              else
               OK:=false;
            end
           else
            begin
{               if (AccDesired>-0.2) and ((Vel<20) or (Vel-VelNext<10)) then
                begin
                  if BrakeCtrlPos>0 then
                   OK:=IncBrakeLevel
                  else
                   OK:=IncLocalBrakeLevel(1);   {finezyjne hamowanie lokalnym}
{                end
               else }
                OK:=IncBrakeLevel;
            end;
          end;
       end;
      ElectroPneumatic:
       begin
         if BrakeCtrlPos<BrakeCtrlPosNo then
          if BrakePressureTable[BrakeCtrlPos+1].BrakeType=ElectroPneumatic then
           OK:=IncBrakeLevel
          else
            OK:=false;
       end;
     end;
   end;
  IncBrake:=OK;
end;

function TController.DecBrake: boolean;
var OK:boolean;
begin
  ClearPendingExceptions;
  OK:=False;
 with Controlling^ do
   begin
     case BrakeSystem of
      Individual:
       begin
         OK:=DecLocalBrakeLevel(1+Trunc(abs(AccDesired)));
       end;
      Pneumatic:
       begin
         if BrakeCtrlPos>0 then
           OK:=DecBrakeLevel;
         if not OK then
          OK:=DecLocalBrakeLevel(2);
         Need_BrakeRelease:=true;
       end;
      ElectroPneumatic:
       begin
         if BrakeCtrlPos>-1 then
          if BrakePressureTable[BrakeCtrlPos-1].BrakeType=ElectroPneumatic then
           OK:=DecBrakeLevel
          else
           OK:=false;
          if not OK then
            OK:=DecLocalBrakeLevel(2);
       end;
     end;
   end;
  DecBrake:=OK;
end;

function TController.IncSpeed: boolean;
var OK:boolean;
begin
  ClearPendingExceptions;
  OK:=True;
   with Controlling^ do
   begin
      if (DoorOpenCtrl=1) and (Vel=0) and (DoorLeftOpened or DoorRightOpened) then  //AI zamyka drzwi przed odjazdem
        begin
          DepartureSignal:=true;
          DoorLeft(false);
          DoorRight(false);
          DepartureSignal:=false;
        end
      else
      case EngineType of
        None : if (MainCtrlPosNo>0) then {McZapkie-041003: wagon sterowniczy}
                begin
{TODO: sprawdzanie innego czlonu                  if not FuseFlagCheck() then }
                    if (BrakePress<0.03*MaxBrakePress) then
                      begin
                        OK:=IncMainCtrl(1);
                      end;
              end;
        ElectricSeriesMotor :
               begin
                if not FuseFlag then
                 if (Im<=Imin) and Ready=True{(BrakePress<=0.01*MaxBrakePress)} then
                  begin
                    OK:=IncMainCtrl(1);
                    if (MainCtrlPos>2) and (Im=0)
                     then Need_TryAgain:=true
                     else
                      if not OK then
                      OK:=IncScndCtrl(1);  {TODO: dorobic boczniki na szeregowej przy ciezkich bruttach}
                  end;
               end;
        Dumb, DieselElectric : begin
                 if Ready=True{(BrakePress<=0.01*MaxBrakePress)} then
                  begin
                    OK:=IncMainCtrl(1);
                    if not OK then
                      OK:=IncScndCtrl(1);
                  end;
               end;
        WheelsDriven :
               begin
                 if sign(sin(eAngle))>0 then
                   IncMainCtrl(1+Trunc(abs(AccDesired)))
                  else
                   DecMainCtrl(1+Trunc(abs(AccDesired)));
               end;
        DieselEngine :
               begin
                   if (Vel>dizel_minVelfullengage) and (RList[MainCtrlPos].Mn>0) then
                    OK:=IncMainCtrl(1);
                   if RList[MainCtrlPos].Mn=0 then
                    OK:=IncMainCtrl(1);
                   if not Mains then
                     begin
                      MainSwitch(true);
                      ConverterSwitch(true);
                      CompressorSwitch(true);
                     end;
               end;
      end {case}
   end;
  IncSpeed:=OK;
end;

function TController.DecSpeed: boolean;
var OK:boolean;
begin
  OK:=True;
  with Controlling^ do
   begin
      case EngineType of
        None : if (MainCtrlPosNo>0) then  {McZapkie-041003: wagon sterowniczy}
                  OK:=DecMainCtrl(1+ord(MainCtrlPos>2));
        ElectricSeriesMotor:
               begin
                 OK:=DecScndCtrl(2);
                 if not OK then
                  OK:=DecMainCtrl(1+ord(MainCtrlPos>2));
               end;
        Dumb, DieselElectric : begin
                 OK:=DecScndCtrl(2);
                 if not OK then
                  OK:=DecMainCtrl(2+(MainCtrlPos div 2));
               end;
        WheelsDriven :
               begin
                 OK:=False;
               end;
        DieselEngine :
               begin
                   OK:=false;
                   if (Vel>dizel_minVelfullengage) then
                    begin
                      if RList[MainCtrlPos].Mn>0 then
                       OK:=DecMainCtrl(1)
                    end
                   else
                    while (RList[MainCtrlPos].Mn>0) and (MainCtrlPos>1) do
                     OK:=DecMainCtrl(1);
               end;
      end {case}
   end;
  DecSpeed:=OK;
end;


procedure TController.RecognizeCommand;
var OK:boolean; Order:TOrders;
begin
  ClearPendingExceptions;
  OK:=True;
  Order:=OrderList[OrderPos];
  with Controlling^.CommandIn do
   begin
     if Command='Wait_for_orders' then
      Order:=Wait_for_orders
     else if Command='Prepare_engine' then
      begin
        if Value1=0 then Order:=Release_engine
         else if Value1=1 then Order:=Prepare_engine
          else OK:=False;
      end
     else if Command='Change_direction' then
      case Trunc(Value1) of
       0,1,-1: begin
                 ChangeDirOrder:=Trunc(Value1);
                 Order:=Change_direction
               end
       else OK:=False;
      end
     else if Command='Obey_train' then
      begin
        Order:=Obey_train;
        if Value1>0 then
         TrainNumber:=Trunc(Value1); {i co potem ???}
      end
     else if Command='Shunt' then
      begin
        Order:=Shunt;
        if Value1<>VehicleCount then
         VehicleCount:=Trunc(Value1); {i co potem ? - trzeba zaprogramowac odczepianie}
      end
     else if Controlling^.CommandIn.Command='SetVelocity' then
      begin
        CommandLocation:=Location;
         SetVelocity(Controlling^.CommandIn.Value1,Controlling^.CommandIn.Value2);  {bylo: nic nie rob bo SetVelocity zewnetrznie jest wywolywane przez dynobj.cpp}
        if (Order=Wait_for_orders) and (Controlling^.CommandIn.Value1<>0) then
         JumpToFirstOrder
        else
         if (Order=Shunt) and (Value1<>0) then
					begin
						Order:=Obey_train;
						bCheckSKP:=true;
					end;
      end
     else if Controlling^.CommandIn.Command='SetProximityVelocity' then
      begin
        if SetProximityVelocity(Controlling^.CommandIn.Value1,Controlling^.CommandIn.Value2) then
          CommandLocation:=Location;
 {        if Order=Shunt then Order:=Obey_train;}
       end
     else if Command='ShuntVelocity' then
      begin
{        CommandLocation:=Location; }
        if Value1<>0 then
         if ((Order=Obey_train) or (Order=Wait_for_orders)) then
          Order:=Shunt;
        if (Order=Shunt) then
          SetVelocity(Value1,Value2);
      end
     else if Command='Jump_to_order' then
      begin
        if Value1=-1 then
         JumpToNextOrder
        else
         if (Value1>=0) and (Value1<=maxorders) then
          OrderPos:=Trunc(Value1);
       Order:=OrderList[OrderPos];
             if WriteLogFlag then
                begin
                append(AIlogFile);
                writeln(AILogFile,ElapsedTime:5:2,' - new order: ',Order2Str(Order),' @ ',OrderPos);
                close(AILogFile);
                end;
      end
     else if Controlling^.CommandIn.Command='Warning_signal' then
      begin
        if Value1>0 then
         begin
           WarningDuration:=Controlling^.CommandIn.Value1;
           if Controlling^.CommandIn.Value2>1 then
            Controlling^.WarningSignal:=2
           else
            Controlling^.WarningSignal:=1;
         end;
      end
     else if Controlling^.CommandIn.Command='OutsideStation' then  {wskaznik D5}
      begin
        if Order=Obey_train then
         SetVelocity(Value1,Value2) {koniec stacji - predkosc szlakowa}
        else                        {manewry - zawracaj}
         begin
             ChangeDirOrder:=0;
             Order:=Change_direction
         end;
      end
     else if Pos('PassengerStopPoint:',Controlling^.CommandIn.Command)=1 then  {wskaznik W4}
      begin
       if Order=Obey_train then
        begin
         TrainSet^.UpdateMTable(GlobalTime.hh,GlobalTime.mm,Copy(Controlling^.CommandIn.Command,20,Length(Controlling^.CommandIn.Command)-20));
        end 
      end

     else OK:=False;
     if OK then
      begin
        Command:='';
        OrderList[OrderPos]:=Order;
      end;
   end;
end;

procedure TController.PutCommand(NewCommand:string; NewValue1,NewValue2:real; NewLocation:TLocation);
begin
   if NewCommand='SetVelocity' then
    begin
      CommandLocation:=NewLocation;
      SetVelocity(NewValue1,NewValue2);
      if (OrderList[OrderPos]=Shunt) and (NewValue1<>0) then
				begin
					OrderList[OrderPos]:=Obey_train;
					bCheckSKP:=true;
				end;
    end
   else if NewCommand='ShuntVelocity' then
    begin
      CommandLocation:=NewLocation;
      SetVelocity(NewValue1,NewValue2);
      if (OrderList[OrderPos]=Obey_train) and (NewValue1<>0) then
       OrderList[OrderPos]:=Shunt;
    end
   else
    if NewCommand='SetProximityVelocity' then
     begin
       if SetProximityVelocity(NewValue1,NewValue2) then
          CommandLocation:=NewLocation;
 {        if Order=Shunt then Order:=Obey_train;}
      end
     else if NewCommand='ShuntVelocity' then
    begin
      if NewValue1<>0 then
      VehicleCount:=-2;
      Prepare2press:=false;
      CommandLocation:=NewLocation;
      if (OrderList[OrderPos]<>Obey_train) then
      SetVelocity(NewValue1,NewValue2);

    end
      else if NewCommand='Wait_for_orders' then
      OrderList[OrderPos]:=Wait_for_orders
     else if NewCommand='Prepare_engine' then
      begin
        if NewValue1=0 then OrderList[OrderPos]:=Release_engine
         else if NewValue1=1 then OrderList[OrderPos]:=Prepare_engine
      end
     else if NewCommand='Change_direction' then
      case Trunc(NewValue1) of
       0,1,-1: begin
                 ChangeDirOrder:=Trunc(NewValue1);
                 OrderList[OrderPos]:=Change_direction
               end
      end
     else if NewCommand='Obey_train' then
      begin
        OrderList[OrderPos]:=Obey_train;
        if NewValue1>0 then
         TrainNumber:=Trunc(NewValue1); {i co potem ???}
      end
     else if NewCommand='Shunt' then
      begin
        if NewValue1<>VehicleCount then
         VehicleCount:=Trunc(NewValue1); {i co potem ? - trzeba zaprogramowac odczepianie}
          OrderList[OrderPos]:=Shunt;
      end
     else if NewCommand='Jump_to_order' then
      begin
        if NewValue1=-1 then
         JumpToNextOrder
        else
         if (NewValue1>=0) and (NewValue1<=maxorders) then
          OrderPos:=Trunc(NewValue1);
             if WriteLogFlag then
                begin
                append(AIlogFile);
                writeln(AILogFile,ElapsedTime:5:2,' - new order: ',Order2Str( OrderList[OrderPos]),' @ ',OrderPos);
                close(AILogFile);
                end;
      end
     else if NewCommand='Warning_signal' then
      begin
        if NewValue1>0 then
         begin
           WarningDuration:=NewValue1;
           if NewValue2>1 then
            Controlling^.WarningSignal:=2
           else
            Controlling^.WarningSignal:=1;
         end;
      end
     else if NewCommand='OutsideStation' then  {wskaznik D5}
      begin
        if  OrderList[OrderPos]=Obey_train then
         SetVelocity(NewValue1,NewValue2) {koniec stacji - predkosc szlakowa}
        else                        {manewry - zawracaj}
         begin
             ChangeDirOrder:=0;
             OrderList[OrderPos]:=Change_direction;
         end;
      end;


end;




function TController.UpdateSituation(dt:real):boolean;
var AbsAccS,VelReduced:real;
const SignalDim:TDimension=(W:1;L:1;H:1);
begin
//yb: zeby EP nie musial sie bawic z ciesnieniem w PG
  if AIControllFlag then
   with(Controlling^)do
    begin
     if(BrakeSystem=Electropneumatic)then
       PipePress:=0.5;
     if(SlippingWheels)then
      begin
        SandDoseOn;
        SlippingWheels:=false;
      end;
    end;
    
  HelpMeFlag:=False;
//Winger 020304
  if Controlling^.Vel>0 then
   if AIControllFlag then
    if Controlling^.DoorOpenCtrl=1 then
     if Controlling^.DoorRightOpened then
      with Controlling^ do
       DoorRight(false);  //Winger 090304 - jak jedzie to niech zamyka drzwi
 if Controlling^.EnginePowerSource.SourceType=CurrentCollector then
  if Controlling^.Vel>0 then
   if AIControllFlag then
    with Controlling^ do
      PantFront(true);
 if Controlling^.EnginePowerSource.SourceType=CurrentCollector then
  if Controlling^.Vel>30 then
   if AIControllFlag then
    with Controlling^ do
     PantRear(false);
  if Controlling^.Vel>0 then
   with Controlling^ do
    if TestFlag(CategoryFlag,2) then
     if abs(OffsetTrackH)<Dim.W then
      if not ChangeOffsetH(-0.1*Vel*dt) then
       ChangeOffsetH(0.1*Vel*dt);
  ElapsedTime:=ElapsedTime+dt;
  WaitingTime:=WaitingTime+dt;
  if WriteLogFlag then
   begin
    if LastUpdatedTime>deltalog then
     begin
       with Controlling^ do
        writeln(LogFile,ElapsedTime,' ',abs(11.31*WheelDiameter*nrot),' ',AccS,' ',Couplers[1].Dist,' ',Couplers[1].CForce,' ',Ft,' ',Ff,' ',Fb,' ',BrakePress,' ',PipePress,' ',Im,' ',MainCtrlPos,'   ',ScndCtrlPos,'   ',BrakeCtrlPos,'   ',LocalBrakePos,'   ',ActiveDir,'   ',CommandIn.Command,' ',CommandIn.Value1,' ',CommandIn.Value2,' ',SecuritySystem.status);
       if Abs(Controlling^.V)>0.1 then deltalog:=0.2
          else deltalog:=1.0;
       LastUpdatedTime:=0;
     end
     else
      LastUpdatedTime:=LastUpdatedTime+dt;
   end;
  if AIControllFlag then
   begin
{     UpdateSituation:=True; }
     {tu bedzie logika sterowania}
     {Ra: nie wiem czemu ReactionTime potrafi dostaæ 12 sekund, to jest przegiêcie, bo prze¿yna STÓJ}
     if (LastReactionTime>Min0R(ReactionTime,2.0))  then
      with Controlling^ do
       begin
         MechLoc:=Loc; {uwzglednic potem polozenie kabiny}
         if (ProximityDist>=0) then
          ActualProximityDist:=Min0R(ProximityDist,Distance(MechLoc,CommandLocation,Dim,SignalDim))
         else
          if ProximityDist<0 then
           ActualProximityDist:=ProximityDist;
         if CommandIn.Command<>'' then
          if not RunInternalCommand then {rozpoznaj komende bo lokomotywa jej nie rozpoznaje}
           RecognizeCommand;
         if SecuritySystem.Status>1 then
          if not SecuritySystemReset then
           if TestFlag(SecuritySystem.Status,s_ebrake) and (BrakeCtrlPos=0) and (AccDesired>0) then
             DecBrakeLevel;
         case OrderList[OrderPos] of   {na jaka odleglosc i z jaka predkoscia ma podjechac}
           Shunt :      begin
                        MinProximityDist:=1; MaxProximityDist:=3; {m}
                          VelReduced:=5; {km/h}
                          if Vel=0 then
                            if (ActiveDir*CabNo<0) then
                               begin
                                  if (Couplers[1].Connected=NIL) then
                                     HeadSignalsFlag:=16;  {swiatla manewrowe}
                                  if (Couplers[0].Connected=NIL) then
                                     EndSignalsFlag:=1;
                               end
                               else
                               begin
                                  if (Couplers[1].Connected=NIL) then
                                     HeadSignalsFlag:=1;  {swiatla manewrowe}
                                  if (Couplers[0].Connected=NIL) then
                                     EndSignalsFlag:=16;
                               end;
                          {20.07.03 - manewrowanie wagonami:}
                          if VehicleCount>=0 then
                           begin
                             if Prepare2press then
                              begin
                                VelActual:=3;
                                if MainCtrlPos>0 then
                                 begin
                                   if (ActiveDir>0) and (Couplers[0].CouplingFlag>0) then
                                    begin
                                      Dettach(0);
                                      VehicleCount:=-2;
                                      VelActual:=0;
																			bCheckSKP:=true;
                                    end
                                   else
                                   if (ActiveDir<0) and (Couplers[1].CouplingFlag>0) then
                                    begin
                                      Dettach(1);
                                      VehicleCount:=-2;
                                      VelActual:=0;
																			bCheckSKP:=true;
                                    end
                                   else HelpMeFlag:=true; {cos nie tak z kierunkiem dociskania}
                                 end;
                                if VehicleCount>=0 then
                                 if not DecLocalBrakeLevel(1) then
                                  begin
                                   BrakeReleaser;
                                   IncSpeed;         {docisnij sklad}
                                 end;
                              end;
                             if (Vel=0) and (not Prepare2press) then
                              begin                    {zmien kierunek na przeciwny}
                                Prepare2press:=true;
                                if ActiveDir>=0 then
                                 while ActiveDir>=0 do
                                  DirectionBackward
                                else
                                 while ActiveDir<=0 do
                                  DirectionForward;
                              end;
                           end {odczepiania}
                           else
                             if Prepare2press and (not DecSpeed) then  {nie ma nic do odczepiania}
                              begin                                    {ponowna zmiana kierunku}
                                if ActiveDir>=0 then
                                 while ActiveDir>=0 do
                                  DirectionBackward
                                else
                                 while ActiveDir<=0 do
                                  DirectionForward;
                                Prepare2press:=false;
                                VelActual:=10;
                              end;
                        end;
           Obey_train : begin
                          if CategoryFlag=1 then  {jazda pociagowa}
                           begin
                             MinProximityDist:=30; MaxProximityDist:=50; {m}
                             if (ActiveDir*CabNo<0) then
                               begin
                                  if (Couplers[0].CouplingFlag=0) then
                                      HeadSignalsFlag:= 1+4+16; {wlacz trojkat}
                                  if (Couplers[1].CouplingFlag=0) then
                                      EndSignalsFlag:= 2+32;      {swiatla konca pociagu i/lub blachy}
                               end
                               else
                               begin
                                  if (Couplers[0].CouplingFlag=0) then
                                      EndSignalsFlag:= 1+4+16; {wlacz trojkat}
                                  if (Couplers[1].CouplingFlag=0) then
                                      HeadSignalsFlag:= 2+32;      {swiatla konca pociagu i/lub blachy}

                               end;
                                                        end
                          else      {samochod}
                           begin
                             MinProximityDist:=5; MaxProximityDist:=10; {m}
                           end;
                          VelReduced:=4; {km/h}
                        end;
           else
             begin
               MinProximityDist:=-0.01; MaxProximityDist:=2; {m}
               VelReduced:=1; {km/h}
             end;

         end; {case}

         case OrderList[OrderPos] of   {co robi maszynista}
            Prepare_engine :
              if PrepareEngine then    {gotowy do drogi?}
               begin
                 SetDriverPsyche;
                 //JumpToNextOrder;
                 Controlling^.CommandIn.Value1:=-1;
                 Controlling^.CommandIn.Value2:=-1;
                 OrderList[OrderPos]:=Shunt;

//                 if OrderList[OrderPos]<>Wait_for_Orders then
//                  if BrakeSystem=Pneumatic then  {napelnianie uderzeniowe na wstepie}
//                   if BrakeSubsystem=Oerlikon then
//                     if (BrakeCtrlPos=0) then
//                        DecBrakeLevel;
               end;
            Release_engine :
              if ReleaseEngine then    {zdana maszyna?}
               JumpToNextOrder;
            Jump_to_first_order:
             if OrderPos>1 then
              OrderPos:=1
             else
              Inc(OrderPos);
            Shunt,Obey_train,Change_direction:
            begin
              if OrderList[OrderPos]=Shunt then   {spokojne manewry}
               begin
                 VelActual:=Min0R(VelActual,30);
                 if (VehicleCount>=0) and (not Prepare2press) then
                  VelActual:=0; {dorobic potem doczepianie}
               end
              else
               SetDriverPsyche;
              if (VelActual=0) and (WaitingTime>WaitingExpireTime) and (Controlling^.RunningTrack.Velmax<>0) then
               begin
                  with Controlling^ do
                     begin
                        if WriteLogFlag then
                        begin
                           append(AIlogFile);
                           writeln(AILogFile,ElapsedTime:5:2,': ',Name,' V=0 waiting time expired! (',WaitingTime:4:1,')');
                           close(AILogFile);
                        end;
                        PrepareEngine;
                     end;
                  WaitingTime:=0;

                  VelActual:=20;
                  WarningDuration:=1.5;
                  Controlling.WarningSignal:=1;
               end
              else
                if (VelActual=0) and (VelNext>0) and (Vel<1) then
                 VelActual:=VelNext; {omijanie SBL}
              if (HelpMeFlag) or (DamageFlag>0) then
               begin
                   HelpMeFlag:=false;
                  if WriteLogFlag then
                    with Controlling^ do
                     begin
                        append(AIlogFile);
                        writeln(AILogFile,ElapsedTime:5:2,': ',Name,' HelpMe! (',DamageFlag,')');
                        close(AILogFile);
                     end;
               end;
              if OrderList[OrderPos]=Change_direction then {sprobuj zmienic kierunek}
              begin
              VelActual:=0;
                if VelActual < 0.1 then
                begin

                if ChangeDirOrder=0 then
                 ChangeDir:=-(ActiveDir*CabNo)
                else
                 ChangeDir:=ChangeDirOrder;
                ChangeDirOrder:=ChangeDir;
                CabNo:=ChangeDir;
                ActiveCab:=ChangeDir;
                PantFront(true);
                PantRear(true);
                Controlling^.CommandIn.Value1:=-1;
                Controlling^.CommandIn.Value2:=-1;
                OrderList[OrderPos]:=Shunt;
                SetVelocity(20,20);
                if WriteLogFlag then
                 begin
                  append(AIlogFile);
                  writeln(AILogFile,ElapsedTime:5:2,': ',Name,' Direction changed!');
                  close(AILogFile);
                 end;
                if (ActiveDir*CabNo<0) then
                               begin
                                     HeadSignalsFlag:=16;  {swiatla manewrowe}
                                     EndSignalsFlag:=1;
                               end
                               else
                               begin
                                     HeadSignalsFlag:=1;  {swiatla manewrowe}
                                     EndSignalsFlag:=16;
                               end;
                 end;
{                else
                 VelActual:=0; {na wszelki wypadek niech zahamuje}
              end;
             {ustalanie zadanej predkosci}
             if (Controlling^.ActiveDir<>0) then
              begin
                if VelActual<0 then
                 VelDesired:=Vmax {ile fabryka dala}
                else
                 VelDesired:=Min0R(Vmax,VelActual);
                if RunningTrack.Velmax>=0 then
                 VelDesired:=Min0R(VelDesired,RunningTrack.Velmax); {uwaga na ograniczenia szlakowej!}
                if VelforDriver>=0 then
                  VelDesired:=Min0R(VelDesired,VelforDriver);
                if TrainSet^.CheckTrainLatency<10 then
                 if TrainSet^.TTVmax>0 then
                  VelDesired:=Min0R(VelDesired,TrainSet^.TTVmax); {jesli nie spozniony to nie szybciej niz rozkladowa}
                AbsAccs:=Accs*sign(V); {czy sie rozpedza czy hamuje}
                {ustalanie zadanego przyspieszenia}
                if (VelNext>=0) and (ActualProximityDist>=0) and (Vel>=VelNext) then
                 begin
                   if Vel>0 then
                    begin
                     if (Vel<VelReduced+VelNext) and (ActualProximityDist>MaxProximityDist*(1+Vel/10)) then {dojedz do semafora/przeszkody}
                      begin
                        if AccPreferred>0 then
                         AccDesired:=AccPreferred/2;
{                        VelDesired:=Min0R(VelDesired,VelReduced+VelNext); }
                      end
                     else
                      if ActualProximityDist>MinProximityDist then
                       begin
                         AccDesired:=(SQR(VelNext)-SQR(Vel))/(25.92*ActualProximityDist+0.1); {hamuj proporcjonalnie}
                         if AccPreferred<AccDesired then
                          AccDesired:=AccPReferred;                  //(1+abs(AccDesired))
                         ReactionTime:=BrakeDelay[2+2*BrakeDelayFlag]/2; {aby szybkosc hamowania zalezala od przyspieszenia i opoznienia hamulcow}
                       end
                      else
                       VelDesired:=Min0R(VelDesired,VelNext); {utrzymuj predkosc bo juz blisko}
                    end
                   else                         {zatrzymany}
                    if (VelNext>0) or (ActualProximityDist>MaxProximityDist*1.2) then
                     if AccPreferred>0 then
                      AccDesired:=AccPreferred/2  {dociagnij do semafora}
                    else
                     begin
                       VelDesired:=0;            {stoj}
                     end;
                 end
                else
                 AccDesired:=AccPreferred;       {normalna jazda}
                if (VelDesired>=0) and (Vel>VelDesired) then
                 if AccDesired>-AccPreferred then
                  Accdesired:=-AccPreferred;
                if (AccDesired>0) and (VelNext>=0) then
                 if (VelNext<Vel-70) then        {lepiej zaczac hamowac}
                  AccDesired:=-0.2
                 else
                  if (VelNext<Vel-100) then
                   AccDesired:=0;                {nie spiesz sie bo bedzie hamowanie}
                {wlaczanie bezpiecznika}
                if EngineType=ElectricSeriesMotor then
                 if FuseFlag or Need_TryAgain then
                  if not DecScndCtrl(1) then
                   if not DecMainCtrl(2) then
                    if not DecMainCtrl(1) then
                     if not FuseOn then HelpMeFlag:=True
                      else
                       begin
                         inc(DriverFailCount);
                         if DriverFailCount>maxdriverfails then
                          Psyche:=EasyMan;
                         if DriverFailCount>maxdriverfails*2 then
                          SetDriverPsyche;
                       end;
                if BrakeSystem=Pneumatic then  {napelnianie uderzeniowe}
                 if BrakeSubsystem=Oerlikon then
                  begin
                    if BrakeCtrlPos=-2 then BrakeCtrlPos:=0;
                    if (BrakeCtrlPos<0)and (PipebrakePress<0.01){(CntrlPipePress-(Volume/BrakeVVolume/10)<0.01)} then
                     IncBrakeLevel;
                    if (BrakeCtrlPos=0) and (AbsAccS<0)and(AccDesired>0) then
//                     if FuzzyLogicAI(CntrlPipePress-PipePress,0.01,1) then
                     if PipebrakePress>0.01{((Volume/BrakeVVolume/10)<0.485)} then
                      DecBrakeLevel
                     else
                      if Need_BrakeRelease then
//                       begin
                         Need_BrakeRelease:=false;
//                         DecBrakeLevel
//                       end;
                  end;
                {zwiekszanie predkosci:}
                if ((Vel<VelDesired*0.7-VelMargin) or ((Vel<VelDesired*0.94-VelMargin) and (AbsAccS<0)))
               and (((AccDesired>0) and (AbsAccS<AccDesired)) or ((AccDesired<=0) and (AbsAccS<3/(1+BrakeDelay[1+2*BrakeDelayFlag]/10)*AccDesired))) then
       {          if not MaxVelFlag then }
                  begin
                   if not DecBrake then
                    if not IncSpeed then
                     MaxVelFlag:=True
                    else MaxVelFlag:=False;
                  end
                 else
                  if (Vel<VelDesired*0.85) and (AccDesired>0) and (EngineType=ElectricSeriesMotor)
                      and (RList[MainCtrlPos].R>0.0) and (not DelayCtrlFlag) then
                   if (Im<Imax) and Ready=True {(BrakePress<0.01*MaxBrakePress)} then
                    IncMainCtrl(1); {zwieksz nastawnik skoro mozesz - tak aby sie ustawic na bezoporowej}

                {zmniejszanie predkosci:}
                if ((Vel>VelDesired) or ((Vel>VelDesired*0.9) and (AbsAccS>0)))
                or ((AccDesired<0) and (AbsAccS>AccDesired)) then
       {          if not MinVelFlag then   }
                   begin
                      if not DecSpeed then
                       if (AccDesired<-0.1+AbsAccS) or ((Vel-VelMargin)>VelDesired) then
                        if not IncBrake then
                         MinVelFlag:=True
                        else MinVelFlag:=False;
                   end;

                {zapobieganie poslizgowi w czlonie silnikowym}
                if (Couplers[0].Connected<>NIL) then
                 if TestFlag(Couplers[0].CouplingFlag,ctrain_controll) then
                  if Couplers[0].Connected^.SlippingWheels then
                   if not DecScndCtrl(1) then
                    begin
                     if not DecMainCtrl(1) then
                      if BrakeCtrlPos=BrakeCtrlPosNo then
                       DecBrakeLevel;
                     inc(DriverFailCount);
                    end;
                {zapobieganie poslizgowi u nas}
                if SlippingWheels then
                 begin
                   if not DecScndCtrl(1) then
                    if not DecMainCtrl(1) then
                     if BrakeCtrlPos=BrakeCtrlPosNo then
                      DecBrakeLevel
                     else
                      AntiSlippingButton;
                   inc(DriverFailCount);
                 end;
                 if DriverFailCount>maxdriverfails then
                  begin
                    Psyche:=EasyMan;
                    if DriverFailCount>maxdriverfails*2 then
                     SetDriverPsyche;
                  end;
              end;  {kierunek rozny od zera}
             {odhamowywanie skladu po zatrzymaniu i zabezpieczanie lokomotywy}
             if (v=0) and ((VelDesired=0) or (AccDesired=0)) then
              if (BrakeCtrlPos<1) or (not DecBrakeLevel) then
               IncLocalBrakeLevel(1);

(*  ???           {sprawdzanie przelaczania kierunku}
             if ChangeDir<>0 then
              if OrderDirectionChange(ChangeDir)=-1 then
               VelActual:=VelProximity; *)
            end; {rzeczy robione przy jezdzie}
         end; {case Orderlist}
         {kasowanie licznika czasu}
         LastReactionTime:=0;
         {ewentualne skanowanie toru}
         if (not ScanMe) then
          ScanMe:=true;
         UpdateSituation:=True;
       end {with controlling^}
     else
      begin
        LastReactionTime:=LastReactionTime+dt;
        UpdateSituation:=False;
      end;
            if WarningDuration>0.1 then
          begin
           WarningDuration:=WarningDuration-dt;
           if WarningDuration<0.2 then
            Controlling^.WarningSignal:=0;
          end;
  end
  else
   UpdateSituation:=False;
end;

procedure TController.JumpToNextOrder;
begin
  if OrderList[OrderPos]<>Wait_for_orders then
   begin
    if OrderPos<maxorders then
     inc(OrderPos)
    else
     OrderPos:=0;
   end;
end;

procedure TController.JumpToFirstOrder;
begin
  OrderPos:=0;
end;

procedure TController.ChangeOrder(NewOrder:TOrders);
begin
  OrderList[OrderPos]:=NewOrder;
end;

function TController.GetCurrentOrder:TOrders;
begin
  GetCurrentOrder:=OrderList[OrderPos];
end;

constructor TController.Init(LocInitial:TLocation; RotInitial:TRotation;
             AI:boolean; NewControll:PMoverParameters; NewTrainSet:PTRainParameters; InitPsyche:boolean);
var b: byte;
begin
  EngineActive:=false;
  MechLoc:=LocInitial;
  MechRot:=RotInitial;
  LastUpdatedTime:=0;
  ElapsedTime:=0;
  {inicjalizacja zmiennych}
  Psyche:=InitPsyche;
  SetDriverPsyche;
  AccDesired:=AccPreferred;
  veldesired:=0;
  velfordriver:=-1;
  LastReactionTime:=0;
  HelpMeFlag:=False;
  Proximitydist:=1;
  ActualProximitydist:=1;
  with CommandLocation do
   begin x:=0; y:=0; z:=0; end;
  VelActual:=0;
  VelNext:=120;
  AIControllFlag:=AI;
  Controlling:=NewControll;
(*
  with Controlling^ do
   case CabNo of
  -1: SendCtrlBroadcast('CabActivisation',1);
   1: SendCtrlBroadcast('CabActivisation',2)
   else AIControllFlag:=False; {na wszelki wypadek}
   end;
*)
  VehicleName:=Controlling^.Name;
  TrainSet:=NewTrainSet;
  OrderCommand:='';
  OrderValue:=0;
  OrderPos:=0;
  for b:=0 to maxorders do
   OrderList[b]:=Wait_for_orders;
  MaxVelFlag:=False; MinVelFlag:=False;
  ChangeDir:=1;
  DriverFailCount:=0;
  Need_TryAgain:=false;
  Need_BrakeRelease:=true;
  deltalog:=1.0;
  if WriteLogFlag then
   begin
    assignfile(LogFile,VehicleName+'.dat');
    rewrite(LogFile);
    writeln(LogFile,
' Time [s]   Velocity [m/s]  Acceleration [m/ss]   Coupler.Dist[m]  Coupler.Force[N]  TractionForce [kN]  FrictionForce [kN]   BrakeForce [kN]    BrakePress [MPa]   PipePress [MPa]   MotorCurrent [A]    MCP SCP BCP LBP DmgFlag Command CVal1 CVal2');
   end;
   if WriteLogFlag then
   begin
    assignfile(AILogFile,VehicleName+'.txt');
    rewrite(AILogFile);
    writeln(AILogFile,'AI driver log: started OK');
    close(AILogFile);
   end;
  ScanMe:=False;
  VelMargin:=Controlling^.Vmax*0.005;
  WarningDuration:=-1;
  WaitingExpireTime:=31;
  WaitingTime:=0;
  MinProximityDist:=30;
  MaxProximityDist:=50;
  VehicleCount:=-2;
  Prepare2press:=false;
end;

procedure TController.CloseLog;
begin
  if WriteLogFlag then
   begin
   CloseFile(LogFile);
{  if WriteLogFlag then  }
{   CloseFile(AILogFile);}
   append(AIlogFile);
   writeln(AILogFile,ElapsedTime:5:2,': QUIT');
   close(AILogFile);
   end;
end;

END.

