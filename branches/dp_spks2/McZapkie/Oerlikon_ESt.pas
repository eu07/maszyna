unit Oerlikon_ESt;          {fizyka hamulcow Oerlikon ESt dla symulatora}

(*
    MaSzyna EU07 - SPKS
    Brakes. Oerlikon ESt.
    Copyright (C) 2007-2013 Maciej Cierniak
*)


(*
(C) youBy
Co jest:
- glowny przyrzad rozrzadczy
- napelniacz zbiornika pomocniczego
- napelniacz zbiornika sterujacego
- zawor podskoku
- nibyprzyspieszacz
- tylko 16",14",12",10"
- nieprzekladnik rura
- przekladnik 1:1
Co brakuje:
- dobry przyspieszacz
- przekladniki AL2
- rapid REL2
- mozliwosc zasilania z wysokiego cisnienia ESt4
- ep: EP1 i EP2
- HGB300
- przeciwposlizgi
- samoczynne ep
- PZZ dla dodatkowego
- inne srednice
*)

interface

uses mctools,sysutils,friction,hamulce;//,klasy_ham;

CONST
 dMAX= 8;
 dON = 0; //osobowy napelnianie (+ZP)
 dOO = 1; //osobowy oproznianie
 dTN = 2; //towarowy napelnianie (+ZP)
 dTO = 3; //towarowy oproznianie
 dP  = 4;  //zbiornik pomocniczy
 dSd = 5;  //zbiornik sterujacy
 dSm = 6;  //zbiornik sterujacy
 dPd = 7;
 dPm = 8;


TYPE
    TPrzekladnik= class(TReservoir) //przekladnik (powtarzacz)
      private
      public
        BrakeRes: PReservoir;
        Next: PReservoir;
        procedure Update(dt:real); virtual;
      end;

    TRura= class(TPrzekladnik) //nieprzekladnik, rura laczaca
      private
      public
        function P: real; override;
        procedure Update(dt:real); override;
      end;

    TRapid= class(TPrzekladnik) //przekladnik dwustopniowy
      private
        RapidStatus: boolean; //status rapidu
        RapidMult: real;      //przelozenie (w dol)
        Komora2: real;
      public
        procedure SetRapidParams(mult: real);
        procedure SetRapidStatus(rs: boolean);        
        procedure Update(dt:real); override;
      end;

    TPrzekCiagly= class(TPrzekladnik) //AL2
      private
      public
      end;

    TPrzekZalamany= class(TPrzekladnik) //Knicksventil
      private
      public
      end;

    TNESt3= class(TBrake)
      private
        Nozzles: array[0..dMAX] of real; //dysze
        CntrlRes: TReservoir;      //zbiornik steruj¹cy
        BVM: real;                 //przelozenie PG-CH
        ValveFlag: byte;           //polozenie roznych zaworkow
        Zamykajacy: boolean;       //pamiec zaworka zamykajacego
        Przys_wlot: boolean;       //wlot do komory przyspieszacza
        Przys_blok: boolean;       //blokada przyspieszacza
        Miedzypoj: TReservoir;     //pojemnosc posrednia (urojona) do napelniania ZP i ZS
        Przekladniki: array[1..3] of TPrzekladnik;
        RapidStatus: boolean;
        RapidStaly: boolean;
      public
        function GetPF(PP, dt, Vel: real): real; override;     //przeplyw miedzy komora wstepna i PG
        procedure EStParams(i_crc: real);                 //parametry charakterystyczne dla ESt
        procedure Init(PP, HPP, LPP, BP: real; BDF: byte); override;
        function GetCRP: real; override;
        procedure CheckState(BCP: real; var dV1: real); //glowny przyrzad rozrzadczy
        procedure CheckReleaser(dt: real); //odluzniacz
        function CVs(bp: real): real;      //napelniacz sterujacego
        function BVs(BCP: real): real;     //napelniacz pomocniczego
        procedure SetSize(size: integer);  //ustawianie dysz (rozmiaru ZR)
      end;


implementation

function d2A(d: real):real;
begin
  d2A:=(d*d)*0.7854/1000;
end;

// ------ RURA ------

function TRura.P: real;
begin
  P:=Next.P;
end;

procedure TRura.Update(dt: real);
begin
  Next.Flow(dVol);
  dVol:=0;
end;

// ------ PRZEKLADNIK ------

procedure TPrzekladnik.Update(dt: real);
var BCP, BVP, dV: real;
begin

  BCP:=Next.P;
  BVP:=BrakeRes.P;

  if(BCP>P)then
   dV:=-PFVd(BCP,0,d2A(10),P)*dt
  else
  if(BCP<P)then
   dV:=PFVa(BVP,BCP,d2A(10),P)*dt
  else dV:=0;

  Next.Flow(dV);
  if dV>0 then
  BrakeRes.Flow(-dV);  

end;

// ------ PRZEKLADNIK RAPID ------

procedure TRapid.SetRapidParams(mult: real);
begin
  RapidMult:=mult;
  RapidStatus:=false;
end;

procedure TRapid.SetRapidStatus(rs: boolean);
begin
  RapidStatus:=rs;
end;

procedure TRapid.Update(dt: real);
var BCP, BVP, dV, ActMult: real;
begin
  BVP:=BrakeRes.P;
  BCP:=Next.P;

  if(RapidStatus)then
   begin
    ActMult:=RapidMult;
   end
  else
   begin
    ActMult:=1;
   end;

  if(BCP*RapidMult>P*actMult)then
   dV:=-PFVd(BCP,0,d2A(5),P*actMult/RapidMult)*dt
  else
  if(BCP*RapidMult<P*ActMult)then
   dV:=PFVa(BVP,BCP,d2A(5),P*actMult/RapidMult)*dt
  else dV:=0;

  Next.Flow(dV);
  if dV>0 then
  BrakeRes.Flow(-dV);

end;

// ------ OERLIKON EST NA BOGATO ------

function TNESt3.GetPF(PP, dt, Vel: real): real;     //przeplyw miedzy komora wstepna i PG
var dv, dv1, temp:real;
    VVP, BVP, BCP, CVP, MPP, nastG: real;
    i: byte;
begin
  BVP:=BrakeRes.P;
  VVP:=ValveRes.P;
//  BCP:=BrakeCyl.P;
  BCP:=Przekladniki[1].P;
  CVP:=CntrlRes.P-0.0;
  MPP:=Miedzypoj.P;
  dV:=0; dV1:=0;

  nastG:=(BrakeDelayFlag and bdelay_G);

//sprawdzanie stanu
  CheckState(BCP, dV1);
  CheckReleaser(dt);

  CVP:=CntrlRes.P;
  VVP:=ValveRes.P;

//luzowanie
  if(BrakeStatus and b_hld)=b_off then
   dV:=PF(0,BCP,Nozzles[dTO]*nastG+(1-nastG)*Nozzles[dOO])*dt
  else dV:=0;
//  BrakeCyl.Flow(-dV);
  Przekladniki[1].Flow(-dV);
  if(BrakeStatus and b_on)=b_on then
   dV:=PF(BVP,BCP,Nozzles[dTN]*(nastG+2*Byte(BCP<0.7))+Nozzles[dON]*(1-nastG))*dt
  else dV:=0;
//  BrakeCyl.Flow(-dV);
  Przekladniki[1].Flow(-dV);
  BrakeRes.Flow(dV);

  for i:=1 to 3 do
   begin
    Przekladniki[i].Update(dt);
    if (Przekladniki[i] is TRapid) then
     begin
      RapidStatus:=(((BrakeDelayFlag and bdelay_R)=bdelay_R) and ((Vel>70) or ((RapidStatus) and (Vel>50)) or (RapidStaly)));
      (Przekladniki[i] as TRapid).SetRapidStatus(RapidStatus);
     end;
   end;


//przeplyw testowy miedzypojemnosci
  dV:=PF(MPP,VVP,BVs(BCP))+PF(MPP,CVP,CVs(BCP));
  if(MPP-0.05>BVP)then
    dV:=dV+PF(MPP,BVP,Nozzles[dTN]*nastG+(1-nastG)*Nozzles[dON]);
  if MPP>VVP then dV:=dV+PF(MPP,VVP,d2A(5));
  Miedzypoj.Flow(dV*dt*0.15);


//przeplyw ZS <-> PG
  temp:=CVs(BCP);
  dV:=PF(CVP,MPP,temp)*dt;
  CntrlRes.Flow(+dV);
  ValveRes.Flow(-0.02*dV);
  dV1:=dV1-0.98*dV;

//przeplyw ZP <-> MPJ
  if(MPP-0.05>BVP)then
   dV:=PF(BVP,MPP,Nozzles[dTN]*nastG+(1-nastG)*Nozzles[dON])*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  dV1:=dV1+dV*0.98;
  ValveRes.Flow(-0.02*dV);
//przeplyw PG <-> rozdzielacz
  dV:=PF(PP,VVP,0.005)*dt;    //0.01
  ValveRes.Flow(-dV);


  ValveRes.Act;
  BrakeCyl.Act;
  BrakeRes.Act;
  CntrlRes.Act;
  Miedzypoj.Act;
  Przekladniki[1].Act;
  Przekladniki[2].Act;
  Przekladniki[3].Act;
  GetPF:=dV-dV1;
end;


procedure TNESt3.EStParams(i_crc: real);                 //parametry charakterystyczne dla ESt
begin

end;


procedure TNESt3.Init(PP, HPP, LPP, BP: real; BDF: byte);
begin
   ValveRes.CreatePress(1*PP);
   BrakeCyl.CreatePress(1*BP);
   BrakeRes.CreatePress(1*PP);
   CntrlRes:=TReservoir.Create;
   CntrlRes.CreateCap(15);
   CntrlRes.CreatePress(1*HPP);
   BrakeStatus:=0;
   Miedzypoj:=TReservoir.Create;
   Miedzypoj.CreateCap(5);
   Miedzypoj.CreatePress(1*PP);

   BVM:=1/(HPP-0.1-LPP)*MaxBP;

   BrakeDelayFlag:=BDF;

   Zamykajacy:=false;

   if not ((FM is TDisk1) or (FM is TDisk2)) then   //jesli zeliwo to schodz
     RapidStaly:=false
   else
     RapidStaly:=true;

end;


function TNESt3.GetCRP: real;
begin
//  GetCRP:=CntrlRes.P;
  GetCRP:=Przekladniki[1].P;
//  GetCRP:=Miedzypoj.P;
end;


procedure TNESt3.CheckState(BCP: real; var dV1: real); //glowny przyrzad rozrzadczy
var VVP, BVP, CVP, MPP: real;
begin
  BVP:=BrakeRes.P;  //-> tu ma byc komora rozprezna
  VVP:=ValveRes.P;
  CVP:=CntrlRes.P;
  MPP:=Miedzypoj.P;

  if(BCP<0.25)and(VVP+0.08>CVP)then Przys_blok:=false;

//sprawdzanie stanu
// if ((BrakeStatus and 1)=1)and(BCP>0.25)then
   if(VVP+0.002+BCP/BVM<CVP-0.15)then
     BrakeStatus:=(BrakeStatus or 3) //hamowanie stopniowe
   else if(VVP-0.002+(BCP-0.05)/BVM>CVP-0.15) then
     BrakeStatus:=(BrakeStatus and 252) //luzowanie
   else if(VVP+BCP/BVM>CVP-0.15) then
     BrakeStatus:=(BrakeStatus and 253) //zatrzymanie napelaniania
   else if(VVP+(BCP-0.05)/BVM<CVP-0.15)then //zatrzymanie luzowanie
     BrakeStatus:=(BrakeStatus or 1);

 if (BrakeStatus and 1)=0 then
   SoundFlag:=SoundFlag or sf_CylU;

 if(VVP+0.10<CVP)and(BCP<0.25)then    //poczatek hamowania
   if (not Przys_blok) then
    begin
     ValveRes.CreatePress(0.2*VVP);
     SoundFlag:=SoundFlag or sf_Acc;
     ValveRes.Act;
     Przys_blok:=true;
    end;


 if(BCP>0.5)then
   Zamykajacy:=true
 else if(VVP-0.6<MPP) then
   Zamykajacy:=false;

end;


procedure TNESt3.CheckReleaser(dt: real); //odluzniacz
var
  VVP, CVP: real;
begin
  VVP:=ValveRes.P;
  CVP:=CntrlRes.P;

//odluzniacz automatyczny
 if(BrakeStatus and b_rls=b_rls)then
   if(CVP<VVP+0.3)then
    BrakeStatus:=BrakeStatus and 247
   else
    begin
     CntrlRes.Flow(+PF(CVP,0,0.02)*dt);
    end;
end;


function TNESt3.CVs(bp: real): real;      //napelniacz sterujacego
var VVP, BVP, CVP, MPP: real;
begin
  BVP:=BrakeRes.P;
  CVP:=CntrlRes.P;
  VVP:=ValveRes.P;
  MPP:=Miedzypoj.P;

//przeplyw ZS <-> PG
  if(MPP<CVP-0.17)then
    CVS:=0
  else
  if(MPP>CVP-0.08)then
    CVs:=d2A(1.1)
  else
    CVs:=d2A(0.9);
end;


function TNESt3.BVs(BCP: real): real;     //napelniacz pomocniczego
var VVP, BVP, CVP, MPP: real;
begin
  BVP:=BrakeRes.P;
  CVP:=CntrlRes.P;
  VVP:=ValveRes.P;
  MPP:=Miedzypoj.P;

//przeplyw ZP <-> rozdzielacz
  if(MPP<CVP-0.3)then
    BVs:=d2A(3.8)
  else
    if(BCP<0.5) then
      if(Zamykajacy)then
        BVs:=d2A(1.0)  //1.25
      else
        BVs:=d2A(2.5)
    else
      BVs:=0;
end;


procedure TNESt3.SetSize(size: integer);     //ustawianie dysz (rozmiaru ZR)
var
  i:integer;
begin
//  Przekladniki[1]:=TPrzekladnik.Create;
  Przekladniki[1]:=TRura.Create;
//  Przekladniki[1].CreateCap(1);
//  Przekladniki[1].CreatePress(BrakeCyl.P);
  Przekladniki[2]:=TRapid.Create;
  Przekladniki[2].CreateCap(1);
  Przekladniki[2].CreatePress(BrakeCyl.P);
  (Przekladniki[2] as TRapid).SetRapidParams(2);

  Przekladniki[3]:=TRura.Create;

  Przekladniki[1].Next:=@Przekladniki[2];
  Przekladniki[1].BrakeRes:=@BrakeRes;

  Przekladniki[2].Next:=@Przekladniki[3];
  Przekladniki[2].BrakeRes:=@BrakeRes;

  Przekladniki[3].Next:=@BrakeCyl;
  Przekladniki[3].BrakeRes:=@BrakeRes;
  case size of
    16:
     begin   //dON,dOO,dTN,dTO,dP,dS
      Nozzles[dON]:=1.25;//5.0;
      Nozzles[dOO]:=0.907;//3.4;
      Nozzles[dTN]:=2.0;
      Nozzles[dTO]:=1.75;
      Nozzles[dP]:=3.8;
      Nozzles[dPd]:=2.70;
      Nozzles[dPm]:=1.25;
     end;
    14:
     begin   //dON,dOO,dTN,dTO,dP,dS
      Nozzles[dON]:=4.3;
      Nozzles[dOO]:=1.83;
      Nozzles[dTN]:=2.85;
      Nozzles[dTO]:=1.57;
      Nozzles[dP]:=3.4;
      Nozzles[dPd]:=2.20;
      Nozzles[dPm]:=1.10;
     end;
    12:
     begin   //dON,dOO,dTN,dTO,dP,dS
      Nozzles[dON]:=3.7;
      Nozzles[dOO]:=1.62;
      Nozzles[dTN]:=2.50;
      Nozzles[dTO]:=1.39;
      Nozzles[dP]:=2.65;
      Nozzles[dPd]:=1.80;
      Nozzles[dPm]:=0.85;
     end;
    10:
     begin   //dON,dOO,dTN,dTO,dP,dS
      Nozzles[dON]:=3.1;
      Nozzles[dOO]:=1.35;
      Nozzles[dTN]:=2.0;
      Nozzles[dTO]:=1.13;
      Nozzles[dP]:=1.6;
      Nozzles[dPd]:=1.55;
      Nozzles[dPm]:=0.7;
     end;
    else
     begin
      Nozzles[dON]:=0;
      Nozzles[dOO]:=0;
      Nozzles[dTN]:=0;
      Nozzles[dTO]:=0;
      Nozzles[dP]:=0;
      Nozzles[dPd]:=0;
      Nozzles[dPm]:=0;
      end;
  end;

  Nozzles[dSd]:=1.1;
  Nozzles[dSm]:=0.9;

  //przeliczanie z mm^2 na l/m
  for i:=0 to dMAX do
   begin
    Nozzles[i]:=d2A(Nozzles[i]); //(/1000^2*pi/4*1000)
   end;
end;

end.
