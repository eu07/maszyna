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
- rozne typy wstawek zeliwnych
- hamulce tarczowe
- kompozyty
*)
(*
Zrobione:
1) zadeklarowane niektore typy
2) wzor jubaja na tarcie wstawek Bg i Bgu z zeliwa P10
*)

interface

//uses hamulce;
uses mctools,sysutils;

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

  TP10yBg = class(TFricMat)
    public
    function GetFC(N, Vel: real): real; override;
  end;

  TP10yBgu = class(TFricMat)
    public
    function GetFC(N, Vel: real): real; override;
  end;

  TP10 = class(TFricMat)
    public
    function GetFC(N, Vel: real): real; override;
  end;

  TFR513 = class(TFricMat)
    public
    function GetFC(N, Vel: real): real; override;
  end;

  TCosid = class(TFricMat)
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
//  GetFC:=0.60*((1.6*N+100)/(8.0*N+100))*((Vel+100)/(5*Vel+100))*(1.0032-0.0007*N-0.0001*N*N);
//  GetFC:=47/(2*Vel+100)*(2.145-0.0538*N+0.00074*N*N-0.00000536*N*N*N)/2.145;
//    GetFC:=46/(2*Vel+100)*(11.33-0.105*N)/(11.33+0.179*N);
//  GetFC:=49/(2*Vel+100)*(13.08-0.083*N)/(12.94+0.285*N);
//if Vel<20 then Vel:=20;
//  Vel:= Vel-20;
//  GetFC:=0.52*((1*Vel+100)/(5.0*Vel+100))*(13.08-0.083*N)/(12.94+0.285*N);
    GetFC:=Min0R(0.67*(1*(277-2.66*Vel)/(100+2.1*Vel)+0.23*(-686+8.27*Vel)/(100+1.16*Vel))*(13.08-0.083*N)/(12.94+0.285*N),0.4);
end;

function TP10Bgu.GetFC(N, Vel: real): real;
begin
//  GetFC:=0.60*((1.6*N+100)/(8.0*N+100))*((Vel+100)/(5*Vel+100));
//  GetFC:=47/(2*Vel+100)*(2.137-0.0514*N+0.000832*N*N-0.00000604*N*N*N)/2.137;
//  GetFC:=0.49*100/(2*Vel+100)*(11.33-0.013*N)/(11.33+0.280*N);
//  GetFC:=0.52*((Vel+100)/(5.0*Vel+100))*(11.33-0.013*N)/(11.33+0.280*N);
//if Vel<20 then Vel:=20;
//  Vel:= Vel-20;
//  GetFC:=0.52*((0.0*Vel+120)/(5*Vel+100))*(11.33-0.013*N)/(11.33+0.280*N);
//  GetFC:=0.49*100/(3*Vel+100)*(11.33-0.013*N)/(11.33+0.280*N);
    GetFC:=Min0R(0.67*(1*(277-2.66*Vel)/(100+2.1*Vel)+0.23*(-686+8.27*Vel)/(100+1.16*Vel))*(11.33-0.013*N)/(11.33+0.280*N),0.4);
end;

function TP10yBg.GetFC(N, Vel: real): real;
var A,C,u0, V0: real;
begin
    A:=2.135*exp(-0.03726*N)-0.5;
    C:=0.353-A*0.029;
    u0:=0.41-C;
    V0:=25.7+20*A;
    GetFC:=(u0+C*exp(-Vel/V0));
end;

function TP10yBgu.GetFC(N, Vel: real): real;
var A,C,u0, V0: real;
begin
    A:=1.68*exp(-0.02735*N)-0.5;
    C:=0.353-A*0.044;
    u0:=0.41-C;
    V0:=25.7+21*A;
    GetFC:=(u0+C*exp(-Vel/V0));
end;

function TP10.GetFC(N, Vel: real): real;
begin
  GetFC:=0.60*((1.6*N+100)/(8.0*N+100))*((Vel+100)/(5*Vel+100));
//  GetFC:=43/(2*Vel+100)*(2.145-0.0538*N+0.00074*N*N-0.00000536*N*N*N)/2.145
end;


function TFR513.GetFC(N, Vel: real): real;
begin
  GetFC:=0.3-Vel*0.00081;
//  GetFC:=43/(2*Vel+100)*(2.145-0.0538*N+0.00074*N*N-0.00000536*N*N*N)/2.145
end;

function TCosid.GetFC(N, Vel: real): real;
begin
//       0.26
//  GetFC:=0.259;
  GetFC:=0.269;
end;

end.

