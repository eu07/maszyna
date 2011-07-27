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
   bp_P10    =   0;
   bp_P10Bg  =   2; //�eliwo fosforowe P10
   bp_P10Bgu =   1;
   bp_LLBg   =   4; //komp. b.n.t.
   bp_LLBgu  =   3;
   bp_LBg    =   6; //komp. n.t.
   bp_LBgu   =   5;
   bp_KBg    =   8; //komp. w.t.
   bp_KBgu   =   7;
   bp_D1     =   9; //tarcze
   bp_D2     =  10;
   bp_FR513  =  11; //Frenoplast FR513
   bp_Cosid  =  12; //jakistam kompozyt :D
   bp_PKPBg  =  13; //�eliwo PKP
   bp_PKPBgu =  14;
   bp_MHS    = 128; //magnetyczny hamulec szynowy
   bp_P10yBg =  15; //�eliwo fosforowe P10
   bp_P10yBgu=  16;

   Spg=0.5067;  //przekroj przewodu 1" w l/m
                //wyj: jednostka dosyc dziwna, ale wszystkie obliczenia
                //i pojemnosci sa podane w litrach (rozsadne wielkosci)
                //zas dlugosc pojazdow jest podana w metrach
                //a predkosc przeplywu w m/s

//   BPT: array[-2..6] of array [0..1] of real= ((0, 5.0), (14, 5.4), (9, 5.0), (6, 4.6), (9, 4.5), (9, 4.0), (9, 3.5), (9, 2.8), (34, 2.8));
   BPT: array[-2..6] of array [0..1] of real= ((0, 5.0), (7.5, 5.4), (5, 5.0), (5, 4.6), (5, 4.5), (5, 4.0), (5, 3.5), (5, 2.8), (13, 2.8));
//   BPT: array[-2..6] of array [0..1] of real= ((0, 5.0), (12, 5.4), (9, 5.0), (9, 4.6), (9, 4.2), (9, 3.8), (9, 3.4), (9, 2.8), (34, 2.8));
//      BPT: array[-2..6] of array [0..1] of real= ((0, 0),(0, 0),(0, 0),(0, 0),(0, 0),(0, 0),(0, 0),(0, 0),(0, 0));
   i_bcpno= 6;

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
        function pa: real; virtual;
        function P: real; virtual;
        procedure Flow(dv: real);
        procedure Act;
    end;

    TBrakeCyl= class(TReservoir)
      public
        function pa: real; override;
        function P: real; override;
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
        function SetBDF(nBDF: byte): boolean;
        procedure Releaser(state: byte);
//        procedure
    end;

    TWest= class(TBrake)
      public
        function GetPF(PP, dt, Vel: real): real; override;     //przeplyw miedzy komora wstepna i PG
        procedure Init(PP, HPP, LPP, BP: real; BDF: byte); override;
      end;

    TESt= class(TBrake)
      private
        CntrlRes: TReservoir;      //zbiornik steruj�cy
        BVM: real;                 //przelozenie PG-CH
      public
        function GetPF(PP, dt, Vel: real): real; override;     //przeplyw miedzy komora wstepna i PG
        procedure EStParams(i_crc: real);                 //parametry charakterystyczne dla ESt
        procedure Init(PP, HPP, LPP, BP: real; BDF: byte); override;
        function GetCRP: real; override;
        procedure CheckState(BCP: real; var dV1: real);
        procedure CheckReleaser(dt: real); //odluzniacz
        function CVs(bp: real): real;      //napelniacz sterujacego
        function BVs(BCP: real): real;     //napelniacz pomocniczego
      end;

    TESt3= class(TESt)
      private
        CylFlowSpeed: array[0..1] of array [0..1] of real;
      public
        function GetPF(PP, dt, Vel: real): real; override;     //przeplyw miedzy komora wstepna i PG
      end;

    TESt3AL2= class(TESt3)
      private
        TareM, LoadM: real;  //masa proznego i pelnego
        TareBP: real;  //cisnienie dla proznego
        LoadC: real;
      public
        ImplsRes: TReservoir;      //komora impulsowa
        function GetPF(PP, dt, Vel: real): real; override;     //przeplyw miedzy komora wstepna i PG
        procedure PLC(mass: real);  //wspolczynnik cisnienia przystawki wazacej
        procedure SetLP(TM, LM, TBP: real);  //parametry przystawki wazacej
        procedure Init(PP, HPP, LPP, BP: real; BDF: byte); override;
      end;

    TESt4R= class(TESt)
      private
        RapidStatus: boolean;
      public
        ImplsRes: TReservoir;      //komora impulsowa
        function GetPF(PP, dt, Vel: real): real; override;     //przeplyw miedzy komora wstepna i PG
        procedure Init(PP, HPP, LPP, BP: real; BDF: byte); override;
      end;

    TLSt= class(TESt4R)
      private
        CylFlowSpeed: array[0..1] of array [0..1] of real;
        LBP: real;     //cisnienie hamulca pomocniczego
        RM: real;      //przelozenie rapida
      public
        procedure SetLBP(P: real);   //cisnienie z hamulca pomocniczego
        procedure SetRM(RMR: real);   //ustalenie przelozenia rapida
        function GetPF(PP, dt, Vel: real): real; override;     //przeplyw miedzy komora wstepna i PG
        function GetHPFlow(HP, dt: real): real; override; //przeplyw - 8 bar
        procedure Init(PP, HPP, LPP, BP: real; BDF: byte); override;
      end;

    TEStEP2= class(TLSt)
      private
        TareM, LoadM: real;  //masa proznego i pelnego
        TareBP: real;  //cisnienie dla proznego
        LoadC: real;
        EPS: real;
      public
        function GetPF(PP, dt, Vel: real): real; override;     //przeplyw miedzy komora wstepna i PG
        procedure Init(PP, HPP, LPP, BP: real; BDF: byte); override;  //inicjalizacja
        procedure PLC(mass: real);  //wspolczynnik cisnienia przystawki wazacej
        procedure SetEPS(nEPS: real);  //stan hamulca EP
        procedure SetLP(TM, LM, TBP: real);  //parametry przystawki wazacej
      end;

    TCV1= class(TBrake)
      private
        CntrlRes: TReservoir;      //zbiornik steruj�cy
        BVM: real;                 //przelozenie PG-CH
      public
        function GetPF(PP, dt, Vel: real): real; override;     //przeplyw miedzy komora wstepna i PG
        procedure Init(PP, HPP, LPP, BP: real; BDF: byte); override;
        function GetCRP: real; override;
        procedure CheckState(BCP: real; var dV1: real);
        function CVs(bp: real): real;
        function BVs(BCP: real): real;
      public
      end;

    TCV1R= class(TCV1)
      private
        ImplsRes: TReservoir;      //komora impulsowa
        RapidStatus: boolean;
      public
//        function GetPF(PP, dt, Vel: real): real; override;     //przeplyw miedzy komora wstepna i PG
//        procedure Init(PP, HPP, LPP, BP: real; BDF: byte); override;
      end;

    TCV1L_TR= class(TCV1)
      private
        ImplsRes: TReservoir;      //komora impulsowa
        LBP: real;     //cisnienie hamulca pomocniczego        
      public
        function GetPF(PP, dt, Vel: real): real; override;     //przeplyw miedzy komora wstepna i PG
        procedure Init(PP, HPP, LPP, BP: real; BDF: byte); override;
        procedure SetLBP(P: real);   //cisnienie z hamulca pomocniczego
        function GetHPFlow(HP, dt: real): real; override; //przeplyw - 8 bar
      end;



  //klasa obejmujaca krany
    THandle= class
      private
//        BCP: integer;
      public
        function GetPF(i_bcp:real; pp, hp, dt, ep: real): real; virtual;
        procedure Init(press: real); virtual;
        function GetCP(): real; virtual;
      end;

    TFV4a= class(THandle)
      private
        CP, TP, RP: real;      //zbiornik steruj�cy, czasowy, redukcyjny
      public
        function GetPF(i_bcp:real; pp, hp, dt, ep: real): real; override;
        procedure Init(press: real); override;
      end;

    Ttest= class(THandle)
      private
        CP: real;
      public
        function GetPF(i_bcp:real; pp, hp, dt, ep: real): real; override;
        procedure Init(press: real); override;
      end;

    TFD1= class(THandle)
      private
        MaxBP: real;  //najwyzsze cisnienie
        BP: real;     //aktualne cisnienie
      public
        function GetPF(i_bcp:real; pp, hp, dt, ep: real): real; override;
        procedure Init(press: real); override;
        function GetCP(): real; override;
//        procedure Init(press: real; MaxBP: real); overload;
      end;

    TFVel6= class(THandle)
      private
        EPS: real;
      public
        function GetPF(i_bcp:real; pp, hp, dt, ep: real): real; override;
        function GetCP(): real; override;
      end;

function PF(P1,P2,S:real):real;

implementation

uses mover;
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
  if  PH-PL<0.0001 then PF:=0 else
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

//---SILOWNIK---

function TBrakeCyl.pa:real;
var VtoC: real;
begin
  VtoC:=Vol/Cap;
  pa:=P*0.1
end;

function TBrakeCyl.P:real;
var VtoC: real;
begin
  VtoC:=Vol/Cap;
  if VtoC<0.06 then P:=VtoC/4
  else if VtoC>0.88 then P:=0.5+(VtoC-0.88)*1.043-0.064*(VtoC-0.88)*(VtoC-0.88)
  else P:=0.15+0.35/0.82*(VtoC-0.06);
end;

(* STARA WERSJA
function TBrakeCyl.P:real;
var VtoC: real;
begin
  VtoC:=Vol/Cap;
//  P:=VtoC;
  if VtoC<0.11 then P:=VtoC
  else if VtoC>1.11 then P:=VtoC-1
  else P:=0.11
end;  //*)


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
  MaxBP:=i_mbp;
  BCN:=i_bcn;
  BCA:=i_bcn*i_bcr*i_bcr*pi;
  BA:=i_ba;
  NBpA:=i_nbpa;
  BrakeDelays:=i_BD;
                               //210.88
//  SizeBR:=i_bcn*i_bcr*i_bcr*i_bcd*40.17*MaxBP/(5-MaxBP);  //objetosc ZP w stosunku do cylindra 14" i cisnienia 4.2 atm
  SizeBR:=i_brc*0.0128;
  SizeBC:=i_bcn*i_bcr*i_bcr*i_bcd*210.88*MaxBP/4.2;            //objetosc CH w stosunku do cylindra 14" i cisnienia 4.2 atm

//  BrakeCyl:=TReservoir.Create;
  BrakeCyl:=TBrakeCyl.Create;
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
  bp_FR513:   FM:=TFR513.Create;
  bp_Cosid:   FM:=TCosid.Create;
  bp_P10yBg:  FM:=TP10yBg.Create;
  bp_P10yBgu: FM:=TP10yBgu.Create;
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
  GetBCP:=BrakeCyl.P;
end;

function TBrake.GetBRP: real;
begin
  GetBRP:=BrakeRes.P;
end;

function TBrake.GetVRP: real;
begin
  GetVRP:=ValveRes.P;
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

function TBrake.SetBDF(nBDF: byte): boolean;
begin
  if ((nBDF and BrakeDelays)=nBDF)and(nBDF<>BrakeDelayFlag) then
   begin
    BrakeDelayFlag:=nBDF;
    SetBDF:=true;
   end
  else
    SetBDF:=false;
end;

procedure TBrake.Releaser(state: byte);
begin
  BrakeStatus:=(BrakeStatus and 247) or state*b_rls;
end;

//---WESTINGHOUSE---

procedure TWest.Init(PP, HPP, LPP, BP: real; BDF: byte);
begin
   ValveRes.CreatePress(PP);
   BrakeCyl.CreatePress(BP);
   BrakeRes.CreatePress(PP/2+HPP/2);
//   BrakeStatus:=3*Byte(BP>0.1);
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
procedure TESt.CheckReleaser(dt: real);
var VVP, BVP, CVP: real;
begin
  VVP:=Min0R(ValveRes.P,BrakeRes.P+0.05);
  CVP:=CntrlRes.P-0.0;

//odluzniacz
 if(BrakeStatus and b_rls=b_rls)then
   if(CVP-VVP<0)then
    BrakeStatus:=BrakeStatus and 247
   else
    begin
     CntrlRes.Flow(+PF(CVP,0,0.1)*dt);
    end;
end;

procedure TESt.CheckState(BCP: real; var dV1: real);
var VVP, BVP, CVP: real;
begin
  BVP:=BrakeRes.P;
  VVP:=Min0R(ValveRes.P,BVP+0.05);
  CVP:=CntrlRes.P-0.0;

//sprawdzanie stanu
 if (BrakeStatus and 1)=1 then
   if(VVP+0.003+BCP/BVM<CVP)then
     BrakeStatus:=(BrakeStatus or 2) //hamowanie stopniowe
   else if(VVP-0.003+BCP/BVM>CVP) then
     BrakeStatus:=(BrakeStatus and 252) //luzowanie
   else if(VVP+BCP/BVM>CVP) then
     BrakeStatus:=(BrakeStatus and 253) //zatrzymanie napelaniania
   else
 else
   if(VVP+0.10<CVP)and(BCP<0.1)then    //poczatek hamowania
    begin
     BrakeStatus:=(BrakeStatus or 3);
     ValveRes.CreatePress(0.8*VVP);
//       dV1:=0.5;
    end
   else if(VVP+BCP/BVM<CVP)and((CVP-VVP)*BVM>0.25) then //zatrzymanie luzowanie
     BrakeStatus:=(BrakeStatus or 1);
end;

function TESt.CVs(bp: real): real;
var VVP, BVP, CVP: real;
begin
  BVP:=BrakeRes.P;
  CVP:=CntrlRes.P;
  VVP:=ValveRes.P;

//przeplyw ZS <-> PG
  if(BVP<CVP-0.2)or(BrakeStatus>0)or(bp>0.25)then
    CVs:=0
  else
    if(VVP>CVP+0.4)then
      if(BVP>CVP+0.2)then
        CVs:=0.23
      else
        CVs:=0.05
    else
      if(BVP>CVP-0.1)then
        CVs:=1
      else
        CVs:=0.3;
end;

function TESt.BVs(BCP: real): real;
var VVP, BVP, CVP: real;
begin
  BVP:=BrakeRes.P;
  CVP:=CntrlRes.P;
  VVP:=ValveRes.P;

//przeplyw ZP <-> rozdzielacz
  if(BVP<CVP-0.2)then
    BVs:=1
  else
    if(BCP<0.5) then
      if(VVP>CVP+0.4)then
        BVs:=0.1
      else
//        if(BVP>CVP+0.1)then
//          temp:=0.3
//        else
          BVs:=0.2*(1.5-Byte(BVP>VVP))
    else
      BVs:=0;
end;





function TESt.GetPF(PP, dt, Vel: real): real;
var dv, dv1, temp:real;
    VVP, BVP, BCP, CVP: real;
begin
 BVP:=BrakeRes.P;
 VVP:=Min0R(ValveRes.P,BVP+0.05);
 BCP:=BrakeCyl.P;
 CVP:=CntrlRes.P-0.0;

 dV:=0; dV1:=0;

//sprawdzanie stanu
  CheckState(BCP, dV1);
  CheckReleaser(dt);

  CVP:=CntrlRes.P;
  VVP:=ValveRes.P;
//przeplyw ZS <-> PG
  temp:=CVs(BCP);
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
   dV:=PF(BVP,BCP,0.016*sizeBC)*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  BrakeCyl.Flow(-dV);

//przeplyw ZP <-> rozdzielacz
  temp:=BVs(BCP);
//  if(BrakeStatus and b_hld)=b_off then
  if(BCP<0.25)or(VVP+0.05>BVP)then
   dV:=PF(BVP,VVP,0.02*sizeBR*temp/1.87)*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  dV1:=dV1+dV*0.96;
  ValveRes.Flow(-0.04*dV);
//przeplyw PG <-> rozdzielacz
  dV:=PF(PP,VVP,0.01)*dt;
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
   BrakeRes.CreatePress(PP);
   CntrlRes:=TReservoir.Create;
   CntrlRes.CreateCap(15);
   CntrlRes.CreatePress(HPP);
   BrakeStatus:=0;

   BVM:=1/(HPP-LPP)*MaxBP;

   BrakeDelayFlag:=BDF;
end;


procedure TESt.EStParams(i_crc: real);
begin

end;


function TESt.GetCRP: real;
begin
  GetCRP:=CntrlRes.P;
end;

//---EP2---

procedure TEStEP2.Init(PP, HPP, LPP, BP: real; BDF: byte);
begin
   inherited;
   ImplsRes.CreateCap(1);
   ImplsRes.CreatePress(BP);

   BrakeRes.CreatePress(PP);

   BrakeDelayFlag:=bdelay_P;
   BrakeDelays:=bdelay_P;
end;


function TEStEP2.GetPF(PP, dt, Vel: real): real;
var dv, dv1, temp:real;
    VVP, BVP, BCP, CVP: real;
begin
  BVP:=BrakeRes.P;
  VVP:=ValveRes.P;
  BCP:=ImplsRes.P;
  CVP:=CntrlRes.P-0.0;

  dV:=0; dV1:=0;

//odluzniacz
  CheckReleaser(dt);

//sprawdzanie stanu
 if (BrakeStatus and 1)=1 then
   if(VVP+0.003+BCP/BVM<CVP)then
     BrakeStatus:=(BrakeStatus or 2) //hamowanie stopniowe
   else if(VVP-0.003+BCP/BVM>CVP) then
     BrakeStatus:=(BrakeStatus and 252) //luzowanie
   else if(VVP+BCP/BVM>CVP) then
     BrakeStatus:=(BrakeStatus and 253) //zatrzymanie napelaniania
   else
 else
   if(VVP+0.10<CVP)and(BCP<0.1) then    //poczatek hamowania
    begin
     BrakeStatus:=(BrakeStatus or 3);
     dV1:=1.25;
    end
   else if(VVP+BCP/BVM<CVP)and(BCP>0.25) then //zatrzymanie luzowanie
     BrakeStatus:=(BrakeStatus or 1);

//przeplyw ZS <-> PG
  if(BVP<CVP-0.2)or(BrakeStatus>0)or(BCP>0.25)then
    temp:=0
  else
    if(VVP>CVP+0.4)then
      temp:=0.1
    else
      temp:=0.5;
      
  dV:=PF(CVP,VVP,0.0015*temp/1.8)*dt;
  CntrlRes.Flow(+dV);
  ValveRes.Flow(-0.04*dV);
  dV1:=dV1-0.96*dV;

//hamulec EP
  temp:=BVP*Byte(EPS>0);
  dV:=PF(temp,LBP,0.0015)*dt*EPS*EPS*Byte(LBP*EPS<MaxBP);
  LBP:=LBP-dV;

//luzowanie KI
  if(BrakeStatus and b_hld)=b_off then
   dV:=PF(0,BCP,0.00037)*dt
  else dV:=0;
  ImplsRes.Flow(-dV);
//przeplyw ZP <-> KI
  if ((BrakeStatus and b_on)=b_on) and (BCP<MaxBP) then
   dV:=PF(BVP,BCP,0.0014)*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  ImplsRes.Flow(-dV);
//przeplyw PG <-> rozdzielacz
  dV:=PF(PP,VVP,0.001*sizeBR)*dt;
  ValveRes.Flow(-dV);

  GetPF:=dV-dV1;

  temp:=Max0R(BCP,LBP)*LoadC;

//luzowanie CH
  if(BrakeCyl.P>temp+0.005)or(Max0R(ImplsRes.P,8*LBP)<0.25) then
   dV:=PF(0,BrakeCyl.P,0.025*sizeBC)*dt
  else dV:=0;
  BrakeCyl.Flow(-dV);
//przeplyw ZP <-> CH
  if(BrakeCyl.P<temp-0.005)and(Max0R(ImplsRes.P,8*LBP)>0.3)and(Max0R(BCP,LBP)<MaxBP)then
   dV:=PF(BVP,BrakeCyl.P,0.035*sizeBC)*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  BrakeCyl.Flow(-dV);

  ImplsRes.Act;
  ValveRes.Act;
  BrakeCyl.Act;
  BrakeRes.Act;
  CntrlRes.Act;
end;

procedure TEStEP2.PLC(mass: real);
begin
  LoadC:=1+Byte(Mass<LoadM)*((TareBP+(MaxBP-TareBP)*(mass-TareM)/(LoadM-TareM))/MaxBP-1);
end;

procedure TEStEP2.SetEPS(nEPS: real);
begin
  EPS:=nEPS;
end;

procedure TEStEP2.SetLP(TM, LM, TBP: real);
begin
  TareM:=TM;
  LoadM:=LM;
  TareBP:=TBP;
end;

//---EST3--

function TESt3.GetPF(PP, dt, Vel: real): real;
var dv, dv1, temp:real;
    VVP, BVP, BCP, CVP: real;
begin
 BVP:=BrakeRes.P;
 VVP:=Min0R(ValveRes.P,BVP+0.05);
 BCP:=BrakeCyl.P;
 CVP:=CntrlRes.P-0.0;

 dV:=0; dV1:=0;

//sprawdzanie stanu
  CheckState(BCP, dV1);
  CheckReleaser(dt);  

  CVP:=CntrlRes.P;
  VVP:=ValveRes.P;
//przeplyw ZS <-> PG
  temp:=CVs(BCP);
  dV:=PF(CVP,VVP,0.0015*temp)*dt;
  CntrlRes.Flow(+dV);
  ValveRes.Flow(-0.04*dV);
  dV1:=dV1-0.96*dV;


//luzowanie
  if(BrakeStatus and b_hld)=b_off then
   dV:=PF(0,BCP,0.0042*(1.37-Byte(BrakeDelayFlag=bdelay_G))*sizeBC)*dt
  else dV:=0;
  BrakeCyl.Flow(-dV);
//przeplyw ZP <-> silowniki
  if(BrakeStatus and b_on)=b_on then
   dV:=PF(BVP,BCP,0.017*(1+Byte((BCP<0.58)and(BrakeDelayFlag=bdelay_G)))*(1.13-Byte((BCP>0.6)and(BrakeDelayFlag=bdelay_G)))*sizeBC)*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  BrakeCyl.Flow(-dV);
//przeplyw ZP <-> rozdzielacz
  temp:=BVs(BCP);
  if(BCP<0.25)or(VVP+0.05>BVP)then
   dV:=PF(BVP,VVP,0.02*sizeBR*temp/1.87)*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  dV1:=dV1+dV*0.96;
  ValveRes.Flow(-0.04*dV);
//przeplyw PG <-> rozdzielacz
  dV:=PF(PP,VVP,0.01)*dt;
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
 CVP:=CntrlRes.P-0.0;

 dV:=0; dV1:=0;

//sprawdzanie stanu
  CheckState(BCP, dV1);
  CheckReleaser(dt);

  CVP:=CntrlRes.P;
  VVP:=ValveRes.P;
//przeplyw ZS <-> PG
  temp:=CVs(BCP);
  dV:=PF(CVP,VVP,0.0015*temp/1.8)*dt;
  CntrlRes.Flow(+dV);
  ValveRes.Flow(-0.04*dV);
  dV1:=dV1-0.96*dV;


//luzowanie KI
  if(BrakeStatus and b_hld)=b_off then
   dV:=PF(0,BCP,0.00037*1.14)*dt
  else dV:=0;
  ImplsRes.Flow(-dV);
//przeplyw ZP <-> KI
  if(BrakeStatus and b_on)=b_on then
   dV:=PF(BVP,BCP,0.0014)*dt
  else dV:=0;
//  BrakeRes.Flow(dV);
  ImplsRes.Flow(-dV);
//przeplyw ZP <-> rozdzielacz
  temp:=BVs(BCP);
  if(BVP<PP+0.01)or(BCP<0.25)then  //or((PP<CVP)and(CVP<PP-0.1)
   dV:=PF(BVP,VVP,0.02*sizeBR*temp/1.87)*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  dV1:=dV1+dV*0.96;
  ValveRes.Flow(-0.04*dV);
//przeplyw PG <-> rozdzielacz
  dV:=PF(PP,VVP,0.01*sizeBR)*dt;
  ValveRes.Flow(-dV);

  GetPF:=dV-dV1;



  RapidStatus:=(BrakeDelayFlag=bdelay_R)and(((Vel>55)and(RapidStatus))or(Vel>70));

  temp:=2-Byte(RapidStatus);

//luzowanie CH
  if(BrakeCyl.P*temp>ImplsRes.P+0.005)or(ImplsRes.P<0.25) then
   dV:=PF(0,BrakeCyl.P,0.015*sizeBC)*dt
  else dV:=0;
  BrakeCyl.Flow(-dV);
//przeplyw ZP <-> CH
  if(BrakeCyl.P*temp<ImplsRes.P-0.005)and(ImplsRes.P>0.3) then
   dV:=PF(BVP,BrakeCyl.P,0.020*sizeBC)*dt
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

   BrakeDelayFlag:=bdelay_R;
end;


//---EST3/AL2---

function TESt3AL2.GetPF(PP, dt, Vel: real): real;
var dv, dv1, temp:real;
    VVP, BVP, BCP, CVP: real;
begin
 BVP:=BrakeRes.P;
 VVP:=Min0R(ValveRes.P,BVP+0.05);
 BCP:=ImplsRes.P;
 CVP:=CntrlRes.P-0.0;

 dV:=0; dV1:=0;

//sprawdzanie stanu
  CheckState(BCP, dV1);
  CheckReleaser(dt);  

  VVP:=ValveRes.P;
//przeplyw ZS <-> PG
  temp:=CVs(BCP);
  dV:=PF(CVP,VVP,0.0015*temp)*dt;
  CntrlRes.Flow(+dV);
  ValveRes.Flow(-0.04*dV);
  dV1:=dV1-0.96*dV;

//luzowanie KI
  if(BrakeStatus and b_hld)=b_off then
   dV:=PF(0,BCP,0.00017*(1.37-Byte(BrakeDelayFlag=bdelay_G)))*dt
  else dV:=0;
  ImplsRes.Flow(-dV);
//przeplyw ZP <-> KI
  if ((BrakeStatus and b_on)=b_on) and (BCP<MaxBP) then
   dV:=PF(BVP,BCP,0.0008*(1+Byte((BCP<0.58)and(BrakeDelayFlag=bdelay_G)))*(1.13-Byte((BCP>0.6)and(BrakeDelayFlag=bdelay_G))))*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  ImplsRes.Flow(-dV);
//przeplyw ZP <-> rozdzielacz
  temp:=BVs(BCP);
  if(BCP<0.25)or(VVP+0.05>BVP)then
   dV:=PF(BVP,VVP,0.02*sizeBR*temp/1.87)*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  dV1:=dV1+dV*0.96;
  ValveRes.Flow(-0.04*dV);
//przeplyw PG <-> rozdzielacz
  dV:=PF(PP,VVP,0.01)*dt;
  ValveRes.Flow(-dV);
  GetPF:=dV-dV1;

//luzowanie CH
  if(BrakeCyl.P>ImplsRes.P*LoadC+0.005)or(ImplsRes.P<0.15) then
   dV:=PF(0,BrakeCyl.P,0.015*sizeBC)*dt
  else dV:=0;
  BrakeCyl.Flow(-dV);

//przeplyw ZP <-> CH
  if(BrakeCyl.P<ImplsRes.P*LoadC-0.005)and(ImplsRes.P>0.15) then
   dV:=PF(BVP,BrakeCyl.P,0.020*sizeBC)*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  BrakeCyl.Flow(-dV);

  ImplsRes.Act;
  ValveRes.Act;
  BrakeCyl.Act;
  BrakeRes.Act;
  CntrlRes.Act;
end;

procedure TESt3AL2.PLC(mass: real);
begin
  LoadC:=1+Byte(Mass<LoadM)*((TareBP+(MaxBP-TareBP)*(mass-TareM)/(LoadM-TareM))/MaxBP-1);
end;

procedure TESt3AL2.SetLP(TM, LM, TBP: real);
begin
  TareM:=TM;
  LoadM:=LM;
  TareBP:=TBP;
end;

procedure TESt3AL2.Init(PP, HPP, LPP, BP: real; BDF: byte);
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

// ValveRes.CreatePress(LBP);
// LBP:=0;

 BVP:=BrakeRes.P;
 VVP:=ValveRes.P;
 BCP:=ImplsRes.P;
 CVP:=CntrlRes.P;

 dV:=0; dV1:=0;

//sprawdzanie stanu
 if(BrakeStatus and b_rls=b_rls)then
   if(CVP<1)then
     BrakeStatus:=BrakeStatus and 247
   else
    begin
     dV:=PF(CVP,BCP,0.1)*dt;
     CntrlRes.Flow(+dV);
     ImplsRes.Flow(-dV);
    end;

  VVP:=ValveRes.P;
//przeplyw ZS <-> PG
  if((CVP-BCP)*BVM>0.5)then
    temp:=0
  else
    if(VVP>CVP+0.4)then
        temp:=0.5
    else
        temp:=0.5;
  dV:=PF(CVP,VVP,0.0015*temp/1.8)*dt;
  CntrlRes.Flow(+dV);
  ValveRes.Flow(-0.04*dV);
  dV1:=dV1-0.96*dV;


//luzowanie KI  {G}
//   if VVP>BCP then
//    dV:=PF(VVP,BCP,0.00004)*dt
//   else if (CVP-BCP)<1.5 then
//    dV:=PF(VVP,BCP,0.00020*(1.33-Byte((CVP-BCP)*BVM>0.65)))*dt
//  else dV:=0;      0.00025 P
                {P}
   if VVP>BCP then
    dV:=PF(VVP,BCP,0.00043*(1.5-Byte(((CVP-BCP)*BVM>1)and(BrakeDelayFlag=bdelay_G))))*dt
   else if (CVP-BCP)<1.5 then
    dV:=PF(VVP,BCP,0.001472*(1.36-Byte(((CVP-BCP)*BVM>1)and(BrakeDelayFlag=bdelay_G))))*dt
  else dV:=0;

  ImplsRes.Flow(-dV);
  ValveRes.Flow(+dV);
//przeplyw PG <-> rozdzielacz
  dV:=PF(PP,VVP,0.01)*dt;
  ValveRes.Flow(-dV);

  GetPF:=dV-dV1;

//  if Vel>55 then temp:=0.72 else
//    temp:=1;{R}
  temp:=1-RM*Byte((Vel>55)and(BrakeDelayFlag=bdelay_R));

//powtarzacz � podwojny zawor zwrotny
  temp:=Max0R((CVP-BCP)*BVM/temp,LBP);
//luzowanie CH
  if(BrakeCyl.P>temp+0.005)or(temp<0.15) then
//   dV:=PF(0,BrakeCyl.P,0.0015*3*sizeBC)*dt
   dV:=PF(0,BrakeCyl.P,0.005*3*sizeBC)*dt
  else dV:=0;
  BrakeCyl.Flow(-dV);
//przeplyw ZP <-> CH
  if(BrakeCyl.P<temp-0.005)and(temp>0.2) then
   dV:=PF(BVP,BrakeCyl.P,0.002*3*sizeBC*2)*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  BrakeCyl.Flow(-dV);

  ImplsRes.Act;
  ValveRes.Act;
  BrakeCyl.Act;
  BrakeRes.Act;                                      
  CntrlRes.Act;
//  LBP:=ValveRes.P;
//  ValveRes.CreatePress(ImplsRes.P);
end;

procedure TLSt.Init(PP, HPP, LPP, BP: real; BDF: byte);
begin
  inherited;
  ValveRes.CreateCap(1);
  ImplsRes.CreateCap(8);
  ImplsRes.CreatePress(PP);
  BrakeRes.CreatePress(8);
  ValveRes.CreatePress(PP);

  BrakeDelayFlag:=BDF;
end;

procedure TLSt.SetLBP(P: real);
begin
  LBP:=P;
end;

procedure TLSt.SetRM(RMR: real);
begin
  RM:=1-RMR;
end;

function TLSt.GetHPFlow(HP, dt: real): real;
var dV: real;
begin
  dV:=Min0R(PF(HP,BrakeRes.P,0.01*dt),0);
  BrakeRes.Flow(-dV);
  GetHPFlow:=dV;
end;


//---DAKO CV1---

procedure TCV1.CheckState(BCP: real; var dV1: real);
var VVP, BVP, CVP: real;
begin
  BVP:=BrakeRes.P;
  VVP:=Min0R(ValveRes.P,BVP+0.05);
  CVP:=CntrlRes.P-0.0;

//odluzniacz
 if(BrakeStatus and b_rls=b_rls)and(CVP-VVP<0)then
  BrakeStatus:=BrakeStatus and 247;

//sprawdzanie stanu
 if (BrakeStatus and 1)=1 then
   if(VVP+0.003+BCP/BVM<CVP)then
     BrakeStatus:=(BrakeStatus or 2) //hamowanie stopniowe
   else if(VVP-0.003+BCP/BVM>CVP) then
     BrakeStatus:=(BrakeStatus and 252) //luzowanie
   else if(VVP+BCP/BVM>CVP) then
     BrakeStatus:=(BrakeStatus and 253) //zatrzymanie napelaniania
   else
 else
   if(VVP+0.10<CVP)and(BCP<0.1) then    //poczatek hamowania
    begin
     BrakeStatus:=(BrakeStatus or 3);
     dV1:=1.25;
    end
   else if(VVP+BCP/BVM<CVP)and(BCP>0.25) then //zatrzymanie luzowanie
     BrakeStatus:=(BrakeStatus or 1);
end;

function TCV1.CVs(bp: real): real;
begin
//przeplyw ZS <-> PG
  if(bp>0.05)then
    CVs:=0
  else
    CVs:=0.23
end;

function TCV1.BVs(BCP: real): real;
var VVP, BVP, CVP: real;
begin
  BVP:=BrakeRes.P;
  CVP:=CntrlRes.P;
  VVP:=ValveRes.P;

//przeplyw ZP <-> rozdzielacz
  if(BVP<CVP-0.1)then
    BVs:=1
  else
    if(BCP>0.05)then
      BVs:=0
    else
      BVs:=0.2*(1.5-Byte(BVP>VVP));
end;

function TCV1.GetPF(PP, dt, Vel: real): real;
var dv, dv1, temp:real;
    VVP, BVP, BCP, CVP: real;
begin
 BVP:=BrakeRes.P;
 VVP:=Min0R(ValveRes.P,BVP+0.05);
 BCP:=BrakeCyl.P;
 CVP:=CntrlRes.P-0.0;

 dV:=0; dV1:=0;

//sprawdzanie stanu
  CheckState(BCP, dV1);

  VVP:=ValveRes.P;
//przeplyw ZS <-> PG
  temp:=CVs(BCP);
  dV:=PF(CVP,VVP,0.0015*temp)*dt;
  CntrlRes.Flow(+dV);
  ValveRes.Flow(-0.04*dV);
  dV1:=dV1-0.96*dV;


//luzowanie
  if(BrakeStatus and b_hld)=b_off then
   dV:=PF(0,BCP,0.0042*(1.37-Byte(BrakeDelayFlag=bdelay_G))*sizeBC)*dt
  else dV:=0;
  BrakeCyl.Flow(-dV);

//przeplyw ZP <-> silowniki
  if(BrakeStatus and b_on)=b_on then
   dV:=PF(BVP,BCP,0.017*(1+Byte((BCP<0.58)and(BrakeDelayFlag=bdelay_G)))*(1.13-Byte((BCP>0.6)and(BrakeDelayFlag=bdelay_G)))*sizeBC)*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  BrakeCyl.Flow(-dV);

//przeplyw ZP <-> rozdzielacz
  temp:=BVs(BCP);
  if(VVP+0.05>BVP)then
   dV:=PF(BVP,VVP,0.02*sizeBR*temp/1.87)*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  dV1:=dV1+dV*0.96;
  ValveRes.Flow(-0.04*dV);
//przeplyw PG <-> rozdzielacz
  dV:=PF(PP,VVP,0.01)*dt;
  ValveRes.Flow(-dV);

  ValveRes.Act;
  BrakeCyl.Act;
  BrakeRes.Act;
  CntrlRes.Act;
  GetPF:=dV-dV1;
end;

procedure TCV1.Init(PP, HPP, LPP, BP: real; BDF: byte);
begin
   ValveRes.CreatePress(PP);
   BrakeCyl.CreatePress(BP);
   BrakeRes.CreatePress(PP);
   CntrlRes:=TReservoir.Create;
   CntrlRes.CreateCap(15);
   CntrlRes.CreatePress(HPP);
   BrakeStatus:=0;

   BVM:=1/(HPP-LPP)*MaxBP;

   BrakeDelayFlag:=BDF;
end;

function TCV1.GetCRP: real;
begin
  GetCRP:=CntrlRes.P;
end;


//---CV1-L-TR---

procedure TCV1L_TR.SetLBP(P: real);
begin
  LBP:=P;
end;

function TCV1L_TR.GetHPFlow(HP, dt: real): real;
var dV: real;
begin
  dV:=PF(HP,BrakeRes.P,0.01)*dt;
  BrakeRes.Flow(-dV);
  GetHPFlow:=Max0R(0,dV);
end;

procedure TCV1L_TR.Init(PP, HPP, LPP, BP: real; BDF: byte);
begin
   inherited;
   ImplsRes:=TReservoir.Create;
   ImplsRes.CreateCap(2.5);
   ImplsRes.CreatePress(BP);
end;

function TCV1L_TR.GetPF(PP, dt, Vel: real): real;
var dv, dv1, temp:real;
    VVP, BVP, BCP, CVP: real;
begin
 BVP:=BrakeRes.P;
 VVP:=Min0R(ValveRes.P,BVP+0.05);
 BCP:=ImplsRes.P;
 CVP:=CntrlRes.P-0.0;

 dV:=0; dV1:=0;

//sprawdzanie stanu
  CheckState(BCP, dV1);

  VVP:=ValveRes.P;
//przeplyw ZS <-> PG
  temp:=CVs(BCP);
  dV:=PF(CVP,VVP,0.0015*temp)*dt;
  CntrlRes.Flow(+dV);
  ValveRes.Flow(-0.04*dV);
  dV1:=dV1-0.96*dV;


//luzowanie KI
  if(BrakeStatus and b_hld)=b_off then
   dV:=PF(0,BCP,0.000425*(1.37-Byte(BrakeDelayFlag=bdelay_G)))*dt
  else dV:=0;
  ImplsRes.Flow(-dV);
//przeplyw ZP <-> KI
  if ((BrakeStatus and b_on)=b_on) and (BCP<MaxBP) then
   dV:=PF(BVP,BCP,0.002*(1+Byte((BCP<0.58)and(BrakeDelayFlag=bdelay_G)))*(1.13-Byte((BCP>0.6)and(BrakeDelayFlag=bdelay_G))))*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  ImplsRes.Flow(-dV);

//przeplyw ZP <-> rozdzielacz
  temp:=BVs(BCP);
  if(VVP+0.05>BVP)then
   dV:=PF(BVP,VVP,0.02*sizeBR*temp/1.87)*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  dV1:=dV1+dV*0.96;
  ValveRes.Flow(-0.04*dV);
//przeplyw PG <-> rozdzielacz
  dV:=PF(PP,VVP,0.01)*dt;
  GetPF:=dV-dV1;

  temp:=Max0R(BCP,LBP);

//luzowanie CH
  if(BrakeCyl.P>temp+0.005)or(Max0R(ImplsRes.P,8*LBP)<0.25) then
   dV:=PF(0,BrakeCyl.P,0.015*sizeBC)*dt
  else dV:=0;
  BrakeCyl.Flow(-dV);

//przeplyw ZP <-> CH
  if(BrakeCyl.P<temp-0.005)and(Max0R(ImplsRes.P,8*LBP)>0.3)and(Max0R(BCP,LBP)<MaxBP)then
   dV:=PF(BVP,BrakeCyl.P,0.020*sizeBC)*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  BrakeCyl.Flow(-dV);

  ImplsRes.Act;
  ValveRes.Act;
  BrakeCyl.Act;
  BrakeRes.Act;
  CntrlRes.Act;
end;


//---KRANY---

function THandle.GetPF(i_bcp:real; pp, hp, dt, ep: real): real;
begin
  GetPF:=0;
end;

procedure THandle.Init(press: real);
begin
end;

function THandle.GetCP: real;
begin
  GetCP:=0;
end;


//---FV4a---

function TFV4a.GetPF(i_bcp:real; pp, hp, dt, ep: real): real;
const
  LBDelay = 100;
var
  LimPP, dpPipe, dpMainValve, ActFlowSpeed: real;
begin

          Limpp:=Min0R(BPT[Round(i_bcp)][1],HP);
          ActFlowSpeed:=BPT[Round(i_bcp)][0];

          if(i_bcp=i_bcpNo)then limpp:=2.9;

          cp:=cp+20*Min0R(abs(Limpp-cp),0.05)*PR(cp,Limpp)*dt/1;
          rp:=rp+20*Min0R(abs(ep-rp),0.05)*PR(rp,ep)*dt/2.5;


          Limpp:=cp;
          dpPipe:=Min0R(HP,Limpp);

          dpMainValve:=PF(dpPipe,pp,ActFlowSpeed/(LBDelay))*dt;
          if(cp>rp+0.05)then
            dpMainValve:=PF(Min0R(cp+0.1,HP),pp,1.1*(ActFlowSpeed)/(LBDelay))*dt;
          if(cp<rp-0.05)then
            dpMainValve:=PF(cp-0.1,pp,1.1*(ActFlowSpeed)/(LBDelay))*dt;

          if Round(i_bcp)=-1 then
           begin
            cp:=cp+5*Min0R(abs(Limpp-cp),0.2)*PR(cp,Limpp)*dt/2;
            if(cp<rp+0.03)then
              if(tp<5)then tp:=tp+dt;
//            if(cp+0.03<5.4)then
            if(rp+0.03<5.4)or(cp+0.03<5.4)then //fala
              dpMainValve:=PF(Min0R(HP,7.1),pp,(ActFlowSpeed)/(LBDelay))*dt
//              dpMainValve:=20*Min0R(abs(ep-7.1),0.05)*PF(HP,pp,(ActFlowSpeed)/(LBDelay))*dt
            else
             begin
             rp:=5.45;
              if (cp<pp-0.01) then                  //: /34*9
                dpMainValve:=PF(dpPipe,pp,(ActFlowSpeed)/34*9/(LBDelay))*dt
              else
                dpMainValve:=PF(dpPipe,pp,(ActFlowSpeed)/(LBDelay))*dt
             end;
           end;

          if(Round(i_bcp)=0)then
           begin
            if(tp>0.1)then
            begin
              cp:=5+(tp-0.1)*0.08;
              tp:=tp-dt/12;
            end;
            if(cp>rp+0.1)and(cp<=5)then
              dpMainValve:=PF(Min0R(cp+0.25,HP),pp,2*(ActFlowSpeed)/(LBDelay))*dt
            else
            if cp>5 then
              dpMainValve:=PF(Min0R(cp,HP),pp,2*(ActFlowSpeed)/(LBDelay))*dt
            else
              dpMainValve:=PF(dpPipe,pp,(ActFlowSpeed)/(LBDelay))*dt
           end;

          if(Round(i_bcp)=i_bcpNo)then
           begin
            dpMainValve:=PF(0,pp,(ActFlowSpeed)/(LBDelay))*dt;
           end;

  GetPF:=dpMainValve;
end;

procedure TFV4a.Init(press: real);
begin
  CP:= press;
  TP:= 0;
  RP:= press;
end;


//--- test ---

function Ttest.GetPF(i_bcp:real; pp, hp, dt, ep: real): real;
const
  LBDelay = 100;
var
  LimPP, dpPipe, dpMainValve, ActFlowSpeed: real;
begin

          Limpp:=BPT[Round(i_bcp)][1];
          ActFlowSpeed:=BPT[Round(i_bcp)][0];

          if(i_bcp=i_bcpNo)then limpp:=0.0;

          if(i_bcp=-1)then limpp:=7;

          cp:=cp+20*Min0R(abs(Limpp-cp),0.05)*PR(cp,Limpp)*dt/1;

          Limpp:=cp;
          dpPipe:=Min0R(HP,Limpp);

          dpMainValve:=PF(dpPipe,pp,ActFlowSpeed/(LBDelay))*dt;

          if(Round(i_bcp)=i_bcpNo)then
           begin
            dpMainValve:=PF(0,pp,(ActFlowSpeed)/(LBDelay))*dt;
           end;

  GetPF:=dpMainValve;
end;

procedure Ttest.Init(press: real);
begin
  CP:= press;
end;


//---FD1---

function TFD1.GetPF(i_bcp:real; pp, hp, dt, ep: real): real;
var dp, temp: real;
begin
//  MaxBP:=4;
  temp:=Min0R(i_bcp*MaxBP,Min0R(5.0,HP));
  dp:=10*Min0R(abs(temp-BP),0.1)*PF(temp,BP,0.0011*(2+Byte(temp>BP)))*dt;
  BP:=BP-dp;
  GetPF:=-dp;
end;

procedure TFD1.Init(press: real);
begin
//  BP:=press;
  MaxBP:=press;
end;

function TFD1.GetCP: real;
begin
  GetCP:=BP;
end;


//---FVel6---

function TFVel6.GetPF(i_bcp:real; pp, hp, dt, ep: real): real;
const
  LBDelay = 100;
var
  LimPP, dpPipe, dpMainValve, ActFlowSpeed: real;
begin
  Limpp:=Min0R(5*Byte(i_bcp<3),hp);
  ActFlowSpeed:=2*Byte((i_bcp<3) or (i_bcp=4));
  dpMainValve:=PF(Limpp,pp,ActFlowSpeed/(LBDelay))*dt;
  GetPF:=dpMainValve;
  EPS:=i_bcp*Byte(i_bcp<2);
end;

function TFVel6.GetCP: real;
begin
  GetCP:=EPS;
end;

end.

