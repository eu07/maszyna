unit hamulce;          {fizyka hamulcow dla symulatora}

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
    Brakes.
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

uses mctools,sysutils,friction;

CONST

   LocalBrakePosNo=10;         {ilosc nastaw hamulca recznego lub pomocniczego}
   MainBrakeMaxPos=10;          {max. ilosc nastaw hamulca zasadniczego}

   {nastawy hamulca}
   bdelay_G=1;    //G
   bdelay_P=2;    //P
   bdelay_R=4;    //R
   bdelay_M=8;    //R+Mg
   bdelay_GR=128; //G-R

   {status hamulca}
//   b_off=0;
//   b_on=1;
//   b_dmg=2;
//   b_release=4;
//   b_antislip=8;
//   b_epused=16;
//   b_Rused=32;
//   b_Ractive=64;

   {stan hamulca}
   b_off =  0;   //luzowanie
   b_hld =  1;   //trzymanie
   b_on  =  2;   //napelnianie
   b_rfl =  4;   //uzupelnianie
   b_rls =  8;   //odluzniacz
   b_ep  = 16;   //elektropneumatyczny
   b_dmg =128;   //wylaczony z dzialania

   {uszkodzenia hamulca}
   df_on  =  1;  //napelnianie
   df_off =  2;  //luzowanie
   df_br  =  4;  //wyplyw z ZP
   df_vv  =  8;  //wyplyw z komory wstepnej
   df_bc  = 16;  //wyplyw z silownika
   df_cv  = 32;  //wyplyw z ZS
   df_PP  = 64;  //zawsze niski stopien
   df_RR  =128;  //zawsze wysoki stopien

   {pary cierne}
   bp_P10Bg  =   0; //¿eliwo
   bp_P10Bgu =   1;
   bp_LLBg   =   2; //komp. b.n.t.
   bp_LLBgu  =   3;
   bp_LBg    =   4; //komp. n.t.
   bp_LBgu   =   5;
   bp_KBg    =   6; //komp. w.t.
   bp_KBgu   =   7;
   bp_D1     =   8; //tarcze
   bp_D2     =   9;
   bp_MHS    = 128; //magnetyczny hamulec szynowy

   Spg=0.5067;  //przekroj przewodu 1" w l/m
                //wyj: jednostka dosyc dziwna, ale wszystkie obliczenia
                //i pojemnosci sa podane w litrach (rozsadne wielkosci)
                //zas dlugosc pojazdow jest podana w metrach
                //a predkosc przeplywu w m/s

   Fc=0.2;      //cisnienie nacisku sprezyny powrotnej tloka w bar
                //wyj: wieksze tloki potrzebuja wiekszych (twardszych) sprezyn

   Fr=2;        //sila nastawiacza hamulca w kN


TYPE

  //klasa obejmujaca pojedyncze zbiorniki
    TReservoir= class
      private
        Cap: real;
        Vol: real;
        dVol: real;
      public
        constructor Create;
        procedure CreateCap(Capacity: real);
        procedure CreatePress(Press: real);
        function pa: real;
        function P: real;
        procedure Flow(dv: real);
        procedure Act;
    end;

  //klasa obejmujaca uklad hamulca zespolonego pojazdu
    TBrake= class
      private
        BrakeCyl: TReservoir;      //silownik
        BrakeRes: TReservoir;      //ZP
        ValveRes: TReservoir;      //komora wstepna
        BCN: byte;                 //ilosc silownikow
        BCM: real;                 //przekladnia hamulcowa
        BCA: real;                 //laczny przekroj silownikow
        BrakeDelays: byte;         //dostepne opoznienia
        BrakeDelayFlag: byte;      //aktualna nastawa
        FM: TFricMat;              //material cierny
        MaxBP: real;               //najwyzsze cisnienie
        BA: byte;                  //osie hamowane
        NBpA: byte;                //klocki na os
        SizeBR: real;              //rozmiar^2 ZP (w stosunku do 14")
        SizeBC: real;              //rozmiar^2 CH (w stosunku do 14")


        BrakeStatus: byte;
      public
        constructor Create(i_mbp, i_bcr, i_bcd, i_brc: real;
                           i_bcn, i_BD, i_mat, i_ba, i_nbpa: byte);
                    //maksymalne cisnienie, promien, skok roboczy, pojemnosc ZP;
                    //ilosc cylindrow, opoznienia hamulca, material klockow, osie hamowane, klocki na os;

        function GetFC(Vel, N: real): real; virtual;        //wspolczynnik tarcia - hamulec wie lepiej
        function GetPF(PP, dt, Vel: real): real; virtual;     //przeplyw miedzy komora wstepna i PG
        function GetBCF: real;                           //sila tlokowa z tloka
        function GetHPFlow(HP, dt: real): real; virtual; //przeplyw - 8 bar
        function GetBCP: real; virtual;
        function GetBRP: real;
        function GetVRP: real;
        function GetCRP: real; virtual;
        procedure Init(PP, HPP, LPP, BP: real; BDF: byte); virtual;
    end;

    TWest= class(TBrake)
      public
        function GetPF(PP, dt, Vel: real): real; override;     //przeplyw miedzy komora wstepna i PG
        procedure Init(PP, HPP, LPP, BP: real; BDF: byte); override;
      end;

    TESt= class(TBrake)
      private
        CntrlRes: TReservoir;      //zbiornik steruj¹cy
        BVM: real;                 //przelozenie PG-CH
      public
        function GetPF(PP, dt, Vel: real): real; override;     //przeplyw miedzy komora wstepna i PG
        procedure EStParams(i_crc: real);                 //parametry charakterystyczne dla ESt
        procedure Init(PP, HPP, LPP, BP: real; BDF: byte); override;
        function GetCRP: real; override;
      end;

    TESt3= class(TESt)
      private
      public
        function GetPF(PP, dt, Vel: real): real; override;     //przeplyw miedzy komora wstepna i PG
      end;

    TESt4R= class(TESt)
      private
      public
        ImplsRes: TReservoir;      //komora impulsowa
        function GetPF(PP, dt, Vel: real): real; override;     //przeplyw miedzy komora wstepna i PG
        procedure Init(PP, HPP, LPP, BP: real; BDF: byte); override;
      end;

    TLSt= class(TESt4R)
      private
        LBP: real;     //cisnienie hamulca pomocniczego
      public
        procedure SetLBP(P: real);   //cisnienie z hamulca pomocniczego
        function GetPF(PP, dt, Vel: real): real; override;     //przeplyw miedzy komora wstepna i PG
        function GetHPFlow(HP, dt: real): real; override; //przeplyw - 8 bar
        procedure Init(PP, HPP, LPP, BP: real; BDF: byte); override;
      end;

    TEStEP= class(TLSt)
      public
        function GetPF(PP, dt, Vel: real): real; override;     //przeplyw miedzy komora wstepna i PG
        function GetHPFlow(HP, dt: real): real; override; //przeplyw - 8 bar
        procedure Init(PP, HPP, LPP, BP: real; BDF: byte); override;
      end;



  //klasa obejmujaca krany
    THandle= class
      private
        BCP: integer;
      public
        function GetPF(i_bcp, pp, dt: real): real; virtual;
      end;

    TFV4a= class(Thandle)
      private
        CP, TP, RP: real;      //zbiornik steruj¹cy, czasowy, redukcyjny
      public
        function GetPF(i_bcp, pp, dt: real): real; override;
      end;

function PF(P1,P2,S:real):real;

implementation


//---FUNKCJE OGOLNE---

function PR(p1,p2:real):real;
var ph,pl: real;
begin
  ph:=Max0R(p1,p2)+0.1;
  pl:=p1+p2-ph+0.2;
  PR:=(p2-p1)/(1.13*ph-pl);
end;

function PF(P1,P2,S:real):real;
var ph,pl: real;
begin
  PH:=Max0R(P1,P2)+1;
  PL:=P1+P2-PH+2;
  if P1=P2 then PF:=0 else
  if (PH-PL)<0.05 then
    PF:=20*(PH-PL)*(PH+1)*222*S*(P2-P1)/(1.13*ph-pl)
  else
    PF:=(PH+1)*222*S*(P2-P1)/(1.13*ph-pl);
end;


//---ZBIORNIKI---

function TReservoir.pa:real;
begin
  pa:=0.1*Vol/Cap;
end;

function TReservoir.P:real;
begin
  P:=Vol/Cap;
end;

procedure TReservoir.Flow(dv: real);
begin
  dVol:=dVol+dv;
end;

constructor TReservoir.Create;
begin
  inherited Create;
  Cap:=1;
  Vol:=0;
end;

procedure TReservoir.Act;
begin
  Vol:=Vol+dVol;
  dVol:=0;
end;

procedure TReservoir.CreateCap(Capacity:real);
begin
  Cap:=Capacity;
end;

procedure TReservoir.CreatePress(Press:real);
begin
  Vol:=Cap*Press;
  dVol:=0;
end;


//---HAMULEC---
(*
constructor TBrake.Create(i_mbp, i_bcr, i_bcd, i_brc: real; i_bcn, i_BD, i_mat, i_ba, i_nbpa: byte);
begin
  inherited Create;
  MaxBP:=i_mbp;
  BCN:=i_bcn;
  BCA:=i_bcn*i_bcr*i_bcr*pi;
  BA:=i_ba;
  NBpA:=i_nbpa;
  BrakeDelays:=i_BD;

//tworzenie zbiornikow
  BrakeCyl.CreateCap(i_bcd*BCA*1000);
  BrakeRes.CreateCap(i_brc);
  ValveRes.CreateCap(0.2);

//  FM.Free;
//materialy cierne
  case i_mat of
  bp_P10Bg:   FM:=TP10Bg.Create;
  bp_P10Bgu:  FM:=TP10Bgu.Create;
  else //domyslnie
  FM:=TP10.Create;
  end;


end  ; *)

constructor TBrake.Create(i_mbp, i_bcr, i_bcd, i_brc: real; i_bcn, i_BD, i_mat, i_ba, i_nbpa: byte);
begin
  inherited Create;
  MaxBP:=i_mbp*10;
  BCN:=i_bcn;
  BCA:=i_bcn*i_bcr*i_bcr*pi;
  BA:=i_ba;
  NBpA:=i_nbpa;
  BrakeDelays:=i_BD;
                               //210.88
//  SizeBR:=i_bcn*i_bcr*i_bcr*i_bcd*40.17*MaxBP/(5-MaxBP);  //objetosc ZP w stosunku do cylindra 14" i cisnienia 4.2 atm
  SizeBR:=i_brc*0.0128;
  SizeBC:=i_bcn*i_bcr*i_bcr*i_bcd*210.88*MaxBP/4.2;            //objetosc CH w stosunku do cylindra 14" i cisnienia 4.2 atm

  BrakeCyl:=TReservoir.Create;
  BrakeRes:=TReservoir.Create;
  ValveRes:=TReservoir.Create;

//tworzenie zbiornikow
  BrakeCyl.CreateCap(i_bcd*BCA*1000);
  BrakeRes.CreateCap(i_brc);
  ValveRes.CreateCap(0.25);

//  FM.Free;
//materialy cierne
  case i_mat of
  bp_P10Bg:   FM:=TP10Bg.Create;
  bp_P10Bgu:  FM:=TP10Bgu.Create;
  else //domyslnie
  FM:=TP10.Create;
  end;
end;

procedure TBrake.Init(PP, HPP, LPP, BP: real; BDF: byte);
begin

end;

function TBrake.GetFC(Vel, N: real): real;
begin
  GetFC:=FM.GetFC(N, Vel);
end;

function TBrake.GetBCP: real;
begin
  GetBCP:=BrakeCyl.pa;
end;

function TBrake.GetBRP: real;
begin
  GetBRP:=BrakeRes.Pa;
end;

function TBrake.GetVRP: real;
begin
  GetVRP:=ValveRes.Pa;
end;

function TBrake.GetCRP: real;
begin
  GetCRP:=0;
end;

function TBrake.GetPF(PP, dt, Vel: real): real;
begin
  ValveRes.Act;
  BrakeCyl.Act;
  BrakeRes.Act;
  GetPF:=0;
end;

function TBrake.GetHPFlow(HP, dt: real): real;
begin
  GetHPFlow:=0;
end;

function TBrake.GetBCF: real;
begin
  GetBCF:=BCA*100*BrakeCyl.P;
end;


//---WESTINGHOUSE---

procedure TWest.Init(PP, HPP, LPP, BP: real; BDF: byte);
begin
   ValveRes.CreatePress(PP);
   BrakeCyl.CreatePress(BP);
   BrakeRes.CreatePress(PP/2+HPP/2);
   BrakeStatus:=3*Byte(BP>0.1);
end;

function TWest.GetPF(PP, dt, Vel: real): real;
var dv, dv1:real;
    VVP, BVP, CVP: real;
begin

 BVP:=BrakeRes.P;
 VVP:=ValveRes.P;
 CVP:=BrakeCyl.P;

 if (BrakeStatus and 1)=1 then
   if(VVP+0.03<BVP)then
     BrakeStatus:=(BrakeStatus or 2)
   else if(VVP>BVP+0.1) then
     BrakeStatus:=(BrakeStatus and 252)
   else if(VVP>BVP) then
     BrakeStatus:=(BrakeStatus and 253)
   else
 else
   if(VVP+0.25<BVP) then
     BrakeStatus:=(BrakeStatus or 3);



  if(BrakeStatus and b_hld)=b_off then
   dV:=PF(0,CVP,0.0068*sizeBC)*dt
  else dV:=0;
  BrakeCyl.Flow(-dV);
//przeplyw ZP <-> silowniki
  if(BrakeStatus and b_on)=b_on then
   dV:=PF(BVP,CVP,0.017*sizeBC)*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  BrakeCyl.Flow(-dV);
//przeplyw ZP <-> rozdzielacz
  if(BrakeStatus and b_hld)=b_off then
   dV:=PF(BVP,VVP,0.0011*sizeBR)*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  dV1:=dV*0.95;
  ValveRes.Flow(-0.05*dV);
//przeplyw PG <-> rozdzielacz
  dV:=PF(PP,VVP,0.01*sizeBR)*dt;
  ValveRes.Flow(-dV);


  ValveRes.Act;
  BrakeCyl.Act;
  BrakeRes.Act;
  GetPF:=dV-dV1;
end;

//---OERLIKON EST4---

function TESt.GetPF(PP, dt, Vel: real): real;
var dv, dv1, temp:real;
    VVP, BVP, BCP, CVP: real;
begin
 BVP:=BrakeRes.P;
 VVP:=Min0R(ValveRes.P,BVP+0.05);
 BCP:=BrakeCyl.P;
 CVP:=CntrlRes.P;

 dV:=0; dV1:=0;

//sprawdzanie stanu
 if(BrakeStatus and 4=4)and(CVP-VVP<0)then
  BrakeStatus:=BrakeStatus and 251;

 if (BrakeStatus and 1)=1 then
   if(VVP+0.003+BCP/BVM<CVP)then
     BrakeStatus:=(BrakeStatus or 2) //hamowanie stopniowe
   else if(VVP-0.003+BCP/BVM>CVP) then
     BrakeStatus:=(BrakeStatus and 252) //luzowanie
   else if(VVP+BCP/BVM>CVP) then
     BrakeStatus:=(BrakeStatus and 253) //zatrzymanie napelaniania
   else
 else
   if(VVP+0.15<CVP)and(BCP<0.1) then    //poczatek hamowania
    begin
     BrakeStatus:=(BrakeStatus or 3);
//     Pipe.Vol:=Pipe.Vol*Pipe.Cap/(Pipe.Cap+0.5);
     dV1:=1;
    end
   else if(VVP+BCP/BVM<CVP)and(BCP>0.3) then //zatrzymanie luzowanie
     BrakeStatus:=(BrakeStatus or 1);



  VVP:=ValveRes.P;
//przeplyw ZS <-> PG
  if(BVP<CVP-0.2)or(BrakeStatus>0)then
    temp:=0
  else
    if(VVP>CVP+0.4)then
      if(BVP>CVP+0.2)then
        temp:=0.23
      else
        temp:=0.05
    else
      if(BVP>CVP-0.1)then
        temp:=1
      else
        temp:=0.3;
  dV:=PF(CVP,VVP,0.0015*temp)*dt;
  CntrlRes.Flow(+dV);
  ValveRes.Flow(-0.04*dV);
  dV1:=dV1-0.96*dV;


//luzowanie
  if(BrakeStatus and b_hld)=b_off then
   dV:=PF(0,BCP,0.0058*sizeBC)*dt
  else dV:=0;
  BrakeCyl.Flow(-dV);
//przeplyw ZP <-> silowniki
  if(BrakeStatus and b_on)=b_on then
   dV:=PF(BVP,BCP,0.017*sizeBC)*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  BrakeCyl.Flow(-dV);
//przeplyw ZP <-> rozdzielacz
  if(BVP<CVP-0.2)then
    temp:=1
  else
    if(VVP>CVP+0.4)then
      temp:=0.1
    else
//      if(BVP>CVP+0.1)then
//        temp:=0.3
//      else
        temp:=0.3;
//  if(BrakeStatus and b_hld)=b_off then
  if(BCP<0.3)or(VVP+0.05>BVP)then
   dV:=PF(BVP,VVP,0.02*sizeBR*temp/1.87)*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  dV1:=dV1+dV*0.96;
  ValveRes.Flow(-0.04*dV);
//przeplyw PG <-> rozdzielacz
  dV:=PF(PP,VVP,0.01*sizeBR)*dt;
  ValveRes.Flow(-dV);

  ValveRes.Act;
  BrakeCyl.Act;
  BrakeRes.Act;
  CntrlRes.Act;
  GetPF:=dV-dV1;
end;

procedure TESt.Init(PP, HPP, LPP, BP: real; BDF: byte);
begin
   ValveRes.CreatePress(PP);
   BrakeCyl.CreatePress(BP);
   BrakeRes.CreatePress(PP/2+HPP/2);
   CntrlRes:=TReservoir.Create;
   CntrlRes.CreateCap(15);
   CntrlRes.CreatePress(HPP);
   BrakeStatus:=3*Byte(BP>0.1);

   BVM:=1/(HPP-LPP)*MaxBP;
end;


procedure TESt.EStParams(i_crc: real);
begin

end;


function TESt.GetCRP: real;
begin
  GetCRP:=CntrlRes.P;
end;

//---EP---

procedure TEStEP.Init(PP, HPP, LPP, BP: real; BDF: byte);
begin
   inherited;
   ImplsRes.CreateCap(1);
   ImplsRes.CreatePress(BP);
end;


function TEStEP.GetPF(PP, dt, Vel: real): real;
var dv, dv1, temp:real;
    VVP, BVP, BCP, CVP: real;
begin
 BVP:=BrakeRes.P;
 VVP:=Min0R(ValveRes.P,BVP+0.05);
 BCP:=ImplsRes.P;
 CVP:=CntrlRes.P;

 dV:=0; dV1:=0;

//sprawdzanie stanu
 if(BrakeStatus and 4=4)and(CVP-VVP<0)then
  BrakeStatus:=BrakeStatus and 251;

 if (BrakeStatus and 1)=1 then
   if(VVP+0.003+BCP/BVM<CVP)then
     BrakeStatus:=(BrakeStatus or 2) //hamowanie stopniowe
   else if(VVP-0.003+BCP/BVM>CVP) then
     BrakeStatus:=(BrakeStatus and 252) //luzowanie
   else if(VVP+BCP/BVM>CVP) then
     BrakeStatus:=(BrakeStatus and 253) //zatrzymanie napelniania
   else
 else
   if(VVP+0.15<CVP)and(BCP<0.1) then    //poczatek hamowania
    begin
     BrakeStatus:=(BrakeStatus or 3);
//     Pipe.Vol:=Pipe.Vol*Pipe.Cap/(Pipe.Cap+0.5);
     dV1:=1;
    end
   else if(VVP+BCP/BVM<CVP)and(BCP>0.3) then //zatrzymanie luzowanie
     BrakeStatus:=(BrakeStatus or 1);



  VVP:=ValveRes.P;
//przeplyw ZS <-> PG
  if(BVP<CVP-0.2)or(BrakeStatus>0)then
    temp:=0
  else
    if(VVP>CVP+0.4)then
      if(BVP>CVP+0.2)then
        temp:=0.23
      else
        temp:=0.05
    else
      if(BVP>CVP-0.1)then
        temp:=1
      else
        temp:=0.3;
  dV:=PF(CVP,VVP,0.0015*temp/1.8)*dt;
  CntrlRes.Flow(+dV);
  ValveRes.Flow(-0.04*dV);
  dV1:=dV1-0.96*dV;


//luzowanie KI
  if(BrakeStatus and b_hld)=b_off then
//   dV:=PF(0,BCP,0.00037*0.27)*dt
   dV:=PF(0,BCP,0.00037)*dt
  else dV:=0;
  ImplsRes.Flow(-dV);
//przeplyw ZP <-> KI
  if(BrakeStatus and b_on)=b_on then
//   dV:=PF(BVP,BCP,0.0014*(1.15-Byte(BCP>0.65)))*dt
   dV:=PF(BVP,BCP,0.0014)*dt
  else dV:=0;
//  BrakeRes.Flow(dV);
  ImplsRes.Flow(-dV);
//przeplyw ZP <-> rozdzielacz
  if(BVP<CVP-0.3)then
    temp:=1//*0.65
  else
    if(VVP>CVP+0.4)then
      temp:=0.1
    else
//      if(BVP>CVP+0.1)then
//        temp:=0.3
//      else
        temp:=0.3;
  if(BVP<PP+0.01)or(BCP<0.3)then  //or((PP<CVP)and(CVP<PP-0.1)
   dV:=PF(BVP,VVP,0.02*sizeBR*temp/1.87)*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  dV1:=dV1+dV*0.96;
  ValveRes.Flow(-0.04*dV);
//przeplyw PG <-> rozdzielacz
  dV:=PF(PP,VVP,0.01*sizeBR)*dt;
  ValveRes.Flow(-dV);

  GetPF:=dV-dV1;

//luzowanie CH
  if(BrakeCyl.P>Max0R(ImplsRes.P,LBP)+0.005)or(Max0R(ImplsRes.P,8*LBP)<0.25) then
   dV:=PF(0,BrakeCyl.P,0.015*sizeBC)*dt
  else dV:=0;
  BrakeCyl.Flow(-dV);
//przeplyw ZP <-> CH
  if(BrakeCyl.P<Max0R(ImplsRes.P,LBP)-0.005)and(Max0R(ImplsRes.P,8*LBP)>0.3)and(Max0R(ImplsRes.P,LBP)<MaxBP)then
   dV:=PF(BVP,BrakeCyl.P,0.02*sizeBC)*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  BrakeCyl.Flow(-dV);

  ImplsRes.Act;
  ValveRes.Act;
  BrakeCyl.Act;
  BrakeRes.Act;
  CntrlRes.Act;
end;

function TEStEP.GetHPFlow(HP, dt: real): real;
var dV: real;
begin
  dV:=PF(HP,BrakeRes.P,0.01)*dt;
  BrakeRes.Flow(-dV);
  GetHPFlow:=Max0R(0,dV);
end;

//---EST3--

function TESt3.GetPF(PP, dt, Vel: real): real;
var dv, dv1, temp:real;
    VVP, BVP, BCP, CVP: real;
begin
 BVP:=BrakeRes.P;
 VVP:=Min0R(ValveRes.P,BVP+0.05);
 BCP:=BrakeCyl.P;
 CVP:=CntrlRes.P;

 dV:=0; dV1:=0;

//sprawdzanie stanu
 if(BrakeStatus and 4=4)and(CVP-VVP<0)then
  BrakeStatus:=BrakeStatus and 251;

 if (BrakeStatus and 1)=1 then
   if(VVP+0.003+BCP/BVM<CVP)then
     BrakeStatus:=(BrakeStatus or 2) //hamowanie stopniowe
   else if(VVP-0.003+BCP/BVM>CVP) then
     BrakeStatus:=(BrakeStatus and 252) //luzowanie
   else if(VVP+BCP/BVM>CVP) then
     BrakeStatus:=(BrakeStatus and 253) //zatrzymanie napelaniania
   else
 else
   if(VVP+0.15<CVP)and(BCP<0.1) then    //poczatek hamowania
    begin
     BrakeStatus:=(BrakeStatus or 3);
//     Pipe.Vol:=Pipe.Vol*Pipe.Cap/(Pipe.Cap+0.5);
     dV1:=1;
    end
   else if(VVP+BCP/BVM<CVP)and(BCP>0.3) then //zatrzymanie luzowanie
     BrakeStatus:=(BrakeStatus or 1);



  VVP:=ValveRes.P;
//przeplyw ZS <-> PG
  if(BVP<CVP-0.2)or(BrakeStatus>0)then
    temp:=0
  else
    if(VVP>CVP+0.4)then
      if(BVP>CVP+0.2)then
        temp:=0.23
      else
        temp:=0.05
    else
      if(BVP>CVP-0.1)then
        temp:=1
      else
        temp:=0.3;
  dV:=PF(CVP,VVP,0.0015*temp)*dt;
  CntrlRes.Flow(+dV);
  ValveRes.Flow(-0.04*dV);
  dV1:=dV1-0.96*dV;


//luzowanie
  if(BrakeStatus and b_hld)=b_off then
   dV:=PF(0,BCP,0.0058*0.27*sizeBC)*dt
  else dV:=0;
  BrakeCyl.Flow(-dV);
//przeplyw ZP <-> silowniki
  if(BrakeStatus and b_on)=b_on then
   dV:=PF(BVP,BCP,0.017*(1.13-Byte(BCP>0.65))*sizeBC)*dt 
  else dV:=0;
  BrakeRes.Flow(dV);
  BrakeCyl.Flow(-dV);
//przeplyw ZP <-> rozdzielacz
  if(BVP<CVP-0.2)then
    temp:=1
  else
    if(VVP>CVP+0.4)then
      temp:=0.1
    else
//      if(BVP>CVP+0.1)then
//        temp:=0.3
//      else
        temp:=0.3;
//  if(BrakeStatus and b_hld)=b_off then
  if(BCP<0.3)or(VVP+0.05>BVP)then
   dV:=PF(BVP,VVP,0.02*sizeBR*temp/1.87)*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  dV1:=dV1+dV*0.96;
  ValveRes.Flow(-0.04*dV);
//przeplyw PG <-> rozdzielacz
  dV:=PF(PP,VVP,0.01*sizeBR)*dt;
  ValveRes.Flow(-dV);

  ValveRes.Act;
  BrakeCyl.Act;
  BrakeRes.Act;
  CntrlRes.Act;
  GetPF:=dV-dV1;
end;

//---EST4-RAPID---

function TESt4R.GetPF(PP, dt, Vel: real): real;
var dv, dv1, temp:real;
    VVP, BVP, BCP, CVP: real;
begin
 BVP:=BrakeRes.P;
 VVP:=Min0R(ValveRes.P,BVP+0.05);
 BCP:=ImplsRes.P;
 CVP:=CntrlRes.P;

 dV:=0; dV1:=0;

//sprawdzanie stanu
 if(BrakeStatus and 4=4)and(CVP-VVP<0)then
  BrakeStatus:=BrakeStatus and 251;

 if (BrakeStatus and 1)=1 then
   if(VVP+0.003+BCP/BVM<CVP)then
     BrakeStatus:=(BrakeStatus or 2) //hamowanie stopniowe
   else if(VVP-0.003+BCP/BVM>CVP) then
     BrakeStatus:=(BrakeStatus and 252) //luzowanie
   else if(VVP+BCP/BVM>CVP) then
     BrakeStatus:=(BrakeStatus and 253) //zatrzymanie napelniania
   else
 else
   if(VVP+0.15<CVP)and(BCP<0.1) then    //poczatek hamowania
    begin
     BrakeStatus:=(BrakeStatus or 3);
//     Pipe.Vol:=Pipe.Vol*Pipe.Cap/(Pipe.Cap+0.5);
     dV1:=1;
    end
   else if(VVP+BCP/BVM<CVP)and(BCP>0.3) then //zatrzymanie luzowanie
     BrakeStatus:=(BrakeStatus or 1);



  VVP:=ValveRes.P;
//przeplyw ZS <-> PG
  if(BVP<CVP-0.2)or(BrakeStatus>0)then
    temp:=0
  else
    if(VVP>CVP+0.4)then
      if(BVP>CVP+0.2)then
        temp:=0.23
      else
        temp:=0.05
    else
      if(BVP>CVP-0.1)then
        temp:=1
      else
        temp:=0.3;
  dV:=PF(CVP,VVP,0.0015*temp/1.8)*dt;
  CntrlRes.Flow(+dV);
  ValveRes.Flow(-0.04*dV);
  dV1:=dV1-0.96*dV;


//luzowanie KI
  if(BrakeStatus and b_hld)=b_off then
//   dV:=PF(0,BCP,0.00037*0.27)*dt
   dV:=PF(0,BCP,0.00037)*dt
  else dV:=0;
  ImplsRes.Flow(-dV);
//przeplyw ZP <-> KI
  if(BrakeStatus and b_on)=b_on then
//   dV:=PF(BVP,BCP,0.0014*(1.15-Byte(BCP>0.65)))*dt
   dV:=PF(BVP,BCP,0.0014)*dt
  else dV:=0;
//  BrakeRes.Flow(dV);
  ImplsRes.Flow(-dV);
//przeplyw ZP <-> rozdzielacz
  if(BVP<CVP-0.3)then
    temp:=1//*0.65
  else
    if(VVP>CVP+0.4)then
      temp:=0.1
    else
//      if(BVP>CVP+0.1)then
//        temp:=0.3
//      else
        temp:=0.3;
  if(BVP<PP+0.01)or(BCP<0.3)then  //or((PP<CVP)and(CVP<PP-0.1)
   dV:=PF(BVP,VVP,0.02*sizeBR*temp/1.87)*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  dV1:=dV1+dV*0.96;
  ValveRes.Flow(-0.04*dV);
//przeplyw PG <-> rozdzielacz
  dV:=PF(PP,VVP,0.01*sizeBR)*dt;
  ValveRes.Flow(-dV);

  GetPF:=dV-dV1;

  if Vel>55 then temp:=1 else temp:=2;

//luzowanie CH
  if(BrakeCyl.P*temp>ImplsRes.P+0.005)or(ImplsRes.P<0.25) then
   dV:=PF(0,BrakeCyl.P,0.015*sizeBC)*dt
  else dV:=0;
  BrakeCyl.Flow(-dV);
//przeplyw ZP <-> CH
  if(BrakeCyl.P*temp<ImplsRes.P-0.005)and(ImplsRes.P>0.3) then
   dV:=PF(BVP,BrakeCyl.P,0.02*sizeBC)*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  BrakeCyl.Flow(-dV);

  ImplsRes.Act;
  ValveRes.Act;
  BrakeCyl.Act;
  BrakeRes.Act;
  CntrlRes.Act;
end;

procedure TESt4R.Init(PP, HPP, LPP, BP: real; BDF: byte);
begin
   inherited;
   ImplsRes:=TReservoir.Create;
   ImplsRes.CreateCap(1);
   ImplsRes.CreatePress(BP);
end;


//---LSt---

function TLSt.GetPF(PP, dt, Vel: real): real;
var dv, dv1, temp:real;
    VVP, BVP, BCP, CVP: real;
begin

 ValveRes.CreatePress(LBP);
 LBP:=0;

 BVP:=BrakeRes.P;
 VVP:=ValveRes.P;
 BCP:=ImplsRes.P;
 CVP:=CntrlRes.P;

 dV:=0; dV1:=0;

//sprawdzanie stanu
(* if(BrakeStatus and 4=4)and(CVP-VVP<0)then
  BrakeStatus:=BrakeStatus and 251; *)

  VVP:=ValveRes.P;
//przeplyw ZS <-> PG
  if(CVP>VVP+0.2)then
    temp:=0
  else
    if(VVP>CVP+0.4)then
        temp:=0.05
    else
        temp:=0.5;
  dV:=PF(CVP,VVP,0.0015*temp/1.8)*dt;
  CntrlRes.Flow(+dV);
  ValveRes.Flow(-0.04*dV);
  dV1:=dV1-0.96*dV;


//luzowanie KI
   if VVP>BCP then
    dV:=PF(VVP,BCP,0.00004)*dt
   else if (CVP-BCP)<1.5 then
    dV:=PF(VVP,BCP,0.00020*(1.33-Byte((CVP-BCP)*BVM>0.65)))*dt
  else dV:=0;

  ImplsRes.Flow(-dV);
  ValveRes.Flow(+dV);
//przeplyw PG <-> rozdzielacz
  dV:=PF(PP,VVP,0.01)*dt;
  ValveRes.Flow(-dV);

  GetPF:=dV-dV1;

  if Vel>70 then temp:=0.72 else temp:=1;

//luzowanie CH
  if(BrakeCyl.P>Max0R((CVP-BCP)*BVM/temp,LBP)+0.005)or(Max0R((CVP-BCP)*BVM/temp,LBP)<0.25) then
   dV:=PF(0,BrakeCyl.P,0.0015*3*sizeBC)*dt
  else dV:=0;
  BrakeCyl.Flow(-dV);
//przeplyw ZP <-> CH
  if(BrakeCyl.P<Max0R((CVP-BCP)*BVM/temp,LBP)-0.005)and(Max0R((CVP-BCP)*BVM/temp,LBP)>0.3) then
   dV:=PF(BVP,BrakeCyl.P,0.002*3*sizeBC)*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  BrakeCyl.Flow(-dV);

  ImplsRes.Act;
  ValveRes.Act;
  BrakeCyl.Act;
//  BrakeRes.Act;
  CntrlRes.Act;
  LBP:=ValveRes.P;
  ValveRes.CreatePress(ImplsRes.P);
end;

procedure TLSt.Init(PP, HPP, LPP, BP: real; BDF: byte);
begin
ValveRes.CreateCap(1);
inherited;
BrakeRes.CreatePress(8);
ImplsRes.CreatePress(5);
end;

procedure TLSt.SetLBP(P: real);
begin
  LBP:=P;
end;

function TLSt.GetHPFlow(HP, dt: real): real;
var dV: real;
begin
  dV:=PF(HP,BrakeRes.P,0.01)*dt;
  BrakeRes.Flow(-dV);
  GetHPFlow:=Max0R(0,dV);
end;

//---KRANY---

function THandle.GetPF(i_bcp, pp, dt: real): real;
begin
  GetPF:=0;
end;

function TFV4a.GetPF(i_bcp, pp, dt: real): real;
begin
  GetPF:=0;
end;


end.

