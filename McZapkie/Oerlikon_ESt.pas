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

TYPE
    TNESt3= class(TBrake)
      private
        Nozzles: array[0..1] of array [0..1] of real;
        CntrlRes: TReservoir;      //zbiornik steruj¹cy
        BVM: real;                 //przelozenie PG-CH
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
end;
procedure TNESt3.CheckState(BCP: real; var dV1: real); //glowny przyrzad rozrzadczy
begin
end;
procedure TNESt3.CheckReleaser(dt: real); //odluzniacz
begin
end;
function TNESt3.CVs(bp: real): real;      //napelniacz sterujacego
begin
end;
function TNESt3.BVs(BCP: real): real;     //napelniacz pomocniczego
begin
end;
procedure TNESt3.SetSize(size: integer);     //ustawianie dysz (rozmiaru ZR)
begin
end;

end.
