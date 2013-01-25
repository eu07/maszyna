unit Oerlikon_ESt;          {fizyka hamulcow Oerlikon ESt dla symulatora}

(*
    MaSzyna EU07 - SPKS
    Brakes. Oerlikon ESt.
    Copyright (C) 2007-2012 Maciej Cierniak
*)


(*
(C) youBy
Co brakuje:
*)

interface

uses mctools,sysutils,hamulce;

CONST
 dMAX=5;
 dON=0; //osobowy napelnianie (+ZP)
 dOO=1; //osobowy oproznianie
 dTN=2; //towarowy napelnianie (+ZP)
 dTO=3; //towarowy oproznianie
 dP =4;  //zbiornik pomocniczy
 dS =5;  //zbiornik sterujacy


TYPE
    TNESt3= class(TBrake)
      private
        Nozzles: array[0..dMAX] of real; //dysze
        CntrlRes: TReservoir;      //zbiornik steruj¹cy
        BVM: real;                 //przelozenie PG-CH
        ValveFlag: byte;           //polozenie roznych zaworkow
      public
        function GetPF(PP, dt, Vel: real): real; override;     //przeplyw miedzy komora wstepna i PG
        procedure EStParams(i_crc: real);                 //parametry charakterystyczne dla ESt
        procedure Init(PP, HPP, LPP, BP: real; BDF: byte); override;
        function GetCRP: real; override;
        procedure CheckState(BCP: real; var dV1: real); //glowny przyrzad rozrzadczy
        procedure CheckReleaser(dt: real); //odluzniacz
        function CVs(bp: real): real;      //napelniacz sterujacego
        function BVs(BCP: real): real;     //napelniacz pomocniczego
        procedure SetSize(size: integer);     //ustawianie dysz (rozmiaru ZR)
      end;


implementation

function TNESt3.GetPF(PP, dt, Vel: real): real;     //przeplyw miedzy komora wstepna i PG
begin

end;


procedure TNESt3.EStParams(i_crc: real);                 //parametry charakterystyczne dla ESt
begin

end;


procedure TNESt3.Init(PP, HPP, LPP, BP: real; BDF: byte);
begin

end;


function TNESt3.GetCRP: real;
begin
  GetCRP:=CntrlRes.P;
end;


procedure TNESt3.CheckState(BCP: real; var dV1: real); //glowny przyrzad rozrzadczy
begin

end;


procedure TNESt3.CheckReleaser(dt: real); //odluzniacz
var
  VVP, CVP: real;
begin
  VVP:=ValveRes.P;
  CVP:=CntrlRes.P;

//odluzniacz automatyczny
 if(BrakeStatus and b_rls=b_rls)then
   if(CVP-VVP<0)then
    BrakeStatus:=BrakeStatus and 247
   else
    begin
     CntrlRes.Flow(+PF(CVP,0,0.1)*dt);
    end;
end;


function TNESt3.CVs(bp: real): real;      //napelniacz sterujacego
begin

end;


function TNESt3.BVs(BCP: real): real;     //napelniacz pomocniczego
begin

end;


procedure TNESt3.SetSize(size: integer);     //ustawianie dysz (rozmiaru ZR)
var
  i:integer;
begin
  case size of
    16:
     begin   //dON,dOO,dTN,dTO,dP,dS
      Nozzles[dON]:=5.0;
      Nozzles[dOO]:=3.4;
      Nozzles[dTN]:=2.0;
      Nozzles[dTO]:=1.75;
      Nozzles[dP]:=3.8;
      Nozzles[dS]:=1.1;
     end;
    else
     begin
      Nozzles[dON]:=0;
      Nozzles[dOO]:=0;
      Nozzles[dTN]:=0;
      Nozzles[dTO]:=0;
      Nozzles[dP]:=0;
      Nozzles[dS]:=0;
      end;
  end;
  //przeliczanie z mm^2 na l/m
  for i:=0 to dMAX do
   begin
    Nozzles[i]:=(Nozzles[i]*Nozzles[i])*0.7854/1000; //(/1000^2*pi/4*1000)
   end;
end;

end.
