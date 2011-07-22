unit friction;          {fizyka ruchu dla symulatora lokomotywy}

(*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2010  Maciej Czapkiewicz and others

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
*)

(*
    MaSzyna EU07 - SPKS
    Friction coefficient.
    Copyright (C) 2007-2010 Maciej Cierniak
*)


(*
(C) youBy
Co brakuje:
Wszystko
*)
(*
Zrobione:
nic
*)

interface

//uses hamulce;

CONST

   bp_P10    =   0;
   bp_P10Bg  =   2; //¿eliwo
   bp_P10Bgu =   1;
   bp_LLBg   =   4; //komp. b.n.t.
   bp_LLBgu  =   3;
   bp_LBg    =   6; //komp. n.t.
   bp_LBgu   =   5;
   bp_KBg    =   8; //komp. w.t.
   bp_KBgu   =   7;
   bp_D1     =   9; //tarcze
   bp_D2     =  10;
   bp_MHS    = 128; //magnetyczny hamulec szynowy

TYPE

  TFricMat = class
    public
    function GetFC(N, Vel: real): real; virtual;
  end;

  TP10Bg = class(TFricMat)
    public
    function GetFC(N, Vel: real): real; override;
  end;

  TP10Bgu = class(TFricMat)
    public
    function GetFC(N, Vel: real): real; override;
  end;

  TP10 = class(TFricMat)
    public
    function GetFC(N, Vel: real): real; override;
  end;



implementation

function TFricMat.GetFC(N, Vel: real): real;
begin
  GetFC:=0;
end;

function TP10Bg.GetFC(N, Vel: real): real;
begin
  GetFC:=0.60*((1.6*N+100)/(8.0*N+100))*((Vel+100)/(5*Vel+100))*(1.0032-0.0007*N-0.0001*N*N);
end;

function TP10Bgu.GetFC(N, Vel: real): real;
begin
  GetFC:=0.60*((1.6*N+100)/(8.0*N+100))*((Vel+100)/(5*Vel+100));
end;

function TP10.GetFC(N, Vel: real): real;
begin
  GetFC:=0.60*((1.6*N+100)/(8.0*N+100))*((Vel+100)/(5*Vel+100));
end;

end.

