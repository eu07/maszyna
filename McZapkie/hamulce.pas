unit hamulce;          {fizyka hamulcow dla symulatora}

(*
    MaSzyna EU07 - SPKS
    Brakes.
    Copyright (C) 2007-2014 Maciej Cierniak
*)                                                                                        


(*
(C) youBy
Co brakuje:
moze jeszcze jakis SW
*)
(*
Zrobione:
ESt3, ESt3AL2, ESt4R, LSt, FV4a, FD1, EP2, prosty westinghouse
duzo wersji ¿eliwa
KE
Tarcze od 152A
Magnetyki (implementacja w mover.pas)
Matrosow 394
H14K1 (zasadniczy), H1405 (pomocniczy), St113 (ep)
Knorr/West EP - ¿eby by³
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
   bdelay_M=8;    //Mg
   bdelay_GR=128; //G-R


   {stan hamulca}
   b_off =  0;   //luzowanie
   b_hld =  1;   //trzymanie
   b_on  =  2;   //napelnianie
   b_rfl =  4;   //uzupelnianie
   b_rls =  8;   //odluzniacz
   b_ep  = 16;   //elektropneumatyczny
   b_asb = 32;   //elektropneumatyczny   
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

   {indeksy dzwiekow FV4a}
   s_fv4a_b = 0; //hamowanie
   s_fv4a_u = 1; //luzowanie
   s_fv4a_e = 2; //hamowanie nagle
   s_fv4a_x = 3; //wyplyw sterujacego fala
   s_fv4a_t = 4; //wyplyw z czasowego

   {pary cierne}
   bp_P10    =   0;
   bp_P10Bg  =   2; //¿eliwo fosforowe P10
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
   bp_PKPBg  =  13; //¿eliwo PKP
   bp_PKPBgu =  14;
   bp_MHS    = 128; //magnetyczny hamulec szynowy
   bp_P10yBg =  15; //¿eliwo fosforowe P10
   bp_P10yBgu=  16;

   sf_Acc  = 1;  //przyspieszacz
   sf_BR   = 2;  //przekladnia
   sf_CylB = 4;  //cylinder - napelnianie
   sf_CylU = 8;  //cylinder - oproznianie
   sf_rel  = 16; //odluzniacz
   sf_ep   = 32; //zawory ep

   bh_MIN= 0;  //minimalna pozycja
   bh_MAX= 1;  //maksymalna pozycja
   bh_FS = 2;  //napelnianie uderzeniowe //jesli nie ma, to jazda
   bh_RP = 3;  //jazda
   bh_NP = 4;  //odciecie - podwojna trakcja
   bh_MB = 5;  //odciecie - utrzymanie stopnia hamowania/pierwszy 1 stopien hamowania
   bh_FB = 6;  //pelne
   bh_EB = 7;  //nagle
   bh_EPR= 8;  //ep - luzowanie  //pelny luz dla ep k¹towego
   bh_EPN= 9;  //ep - utrzymanie //jesli rowne luzowaniu, wtedy sterowanie przyciskiem
   bh_EPB=10;  //ep - hamowanie  //pelne hamowanie dla ep k¹towego


   SpgD=0.7917;
   SpO=0.5067;  //przekroj przewodu 1" w l/m
                //wyj: jednostka dosyc dziwna, ale wszystkie obliczenia
                //i pojemnosci sa podane w litrach (rozsadne wielkosci)
                //zas dlugosc pojazdow jest podana w metrach
                //a predkosc przeplywu w m/s                           //3.5
                                                                       //7//1.5
//   BPT: array[-2..6] of array [0..1] of real= ((0, 5.0), (14, 5.4), (9, 5.0), (6, 4.6), (9, 4.5), (9, 4.0), (9, 3.5), (9, 2.8), (34, 2.8));
//   BPT: array[-2..6] of array [0..1] of real= ((0, 5.0), (7, 5.0), (2.0, 5.0), (4.5, 4.6), (4.5, 4.2), (4.5, 3.8), (4.5, 3.4), (4.5, 2.8), (8, 2.8));
   BPT: array[-2..6] of array [0..1] of real= ((0, 5.0), (7, 5.0), (2.0, 5.0), (4.5, 4.6), (4.5, 4.2), (4.5, 3.8), (4.5, 3.4), (4.5, 2.8), (8, 2.8));
   BPT_394: array[-1..5] of array [0..1] of real= ((13, 10.0), (5, 5.0), (0, -1), (5, -1), (5, 0.0), (5, 0.0), (18, 0.0));
//   BPT: array[-2..6] of array [0..1] of real= ((0, 5.0), (12, 5.4), (9, 5.0), (9, 4.6), (9, 4.2), (9, 3.8), (9, 3.4), (9, 2.8), (34, 2.8));
//      BPT: array[-2..6] of array [0..1] of real= ((0, 0),(0, 0),(0, 0),(0, 0),(0, 0),(0, 0),(0, 0),(0, 0),(0, 0));
   i_bcpno= 6;

TYPE
  //klasa obejmujaca pojedyncze zbiorniki
    TReservoir= class
      protected
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

    PReservoir=^TReservoir;

    TBrakeCyl= class(TReservoir)
      public
        function pa: real; override;
        function P: real; override;
    end;

  //klasa obejmujaca uklad hamulca zespolonego pojazdu
    TBrake= class
      protected
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
        DCV: boolean;              //podwojny zawor zwrotny
        ASBP: real;                //cisnienie hamulca pp

        BrakeStatus: byte; //flaga stanu
        SoundFlag: byte;
      public
        constructor Create(i_mbp, i_bcr, i_bcd, i_brc: real;
                           i_bcn, i_BD, i_mat, i_ba, i_nbpa: byte);
                    //maksymalne cisnienie, promien, skok roboczy, pojemnosc ZP;
                    //ilosc cylindrow, opoznienia hamulca, material klockow, osie hamowane, klocki na os;

        function GetFC(Vel, N: real): real; virtual;        //wspolczynnik tarcia - hamulec wie lepiej
        function GetPF(PP, dt, Vel: real): real; virtual;     //przeplyw miedzy komora wstepna i PG
        function GetBCF: real;                           //sila tlokowa z tloka
        function GetHPFlow(HP, dt: real): real; virtual; //przeplyw - 8 bar
        function GetBCP: real; virtual; //cisnienie cylindrow hamulcowych
        function GetBRP: real; //cisnienie zbiornika pomocniczego
        function GetVRP: real; //cisnienie komory wstepnej rozdzielacza
        function GetCRP: real; virtual; //cisnienie zbiornika sterujacego
        procedure Init(PP, HPP, LPP, BP: real; BDF: byte); virtual; //inicjalizacja hamulca
        function SetBDF(nBDF: byte): boolean; //nastawiacz GPRM
        procedure Releaser(state: byte); //odluzniacz
        procedure SetEPS(nEPS:real); virtual;//hamulec EP
        procedure ASB(state: byte); //hamulec przeciwposlizgowy
        function GetStatus(): byte; //flaga statusu, moze sie przydac do odglosow
        procedure SetASBP(press: real); //ustalenie cisnienia pp
        procedure ForceEmptiness(); virtual;
        function GetSoundFlag: byte;
//        procedure
    end;

    TWest= class(TBrake)
      private
        LBP: real;           //cisnienie hamulca pomocniczego
        dVP: real;           //pobor powietrza wysokiego cisnienia
        EPS: real;           //stan elektropneumatyka
        TareM, LoadM: real;  //masa proznego i pelnego
        TareBP: real;        //cisnienie dla proznego
        LoadC: real;         //wspolczynnik przystawki wazacej
      public
        procedure SetLBP(P: real);   //cisnienie z hamulca pomocniczego
        function GetPF(PP, dt, Vel: real): real; override;     //przeplyw miedzy komora wstepna i PG
        procedure Init(PP, HPP, LPP, BP: real; BDF: byte); override;
        function GetHPFlow(HP, dt: real): real; override;
        procedure PLC(mass: real);  //wspolczynnik cisnienia przystawki wazacej
        procedure SetEPS(nEPS: real); override; //stan hamulca EP
        procedure SetLP(TM, LM, TBP: real);  //parametry przystawki wazacej
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
        procedure CheckState(BCP: real; var dV1: real); //glowny przyrzad rozrzadczy
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
        RapidTemp: real;           //akrualne, zmienne przelozenie
      public
        ImplsRes: TReservoir;      //komora impulsowa
        function GetPF(PP, dt, Vel: real): real; override;     //przeplyw miedzy komora wstepna i PG
        procedure Init(PP, HPP, LPP, BP: real; BDF: byte); override;
      end;

    TLSt= class(TESt4R)
      private
        CylFlowSpeed: array[0..1] of array [0..1] of real;
        LBP: real;       //cisnienie hamulca pomocniczego
        RM: real;        //przelozenie rapida
        EDFlag: real; //luzowanie hamulca z powodu zalaczonego ED
      public
        procedure SetLBP(P: real);   //cisnienie z hamulca pomocniczego
        procedure SetRM(RMR: real);   //ustalenie przelozenia rapida
        function GetPF(PP, dt, Vel: real): real; override;     //przeplyw miedzy komora wstepna i PG
        function GetHPFlow(HP, dt: real): real; override; //przeplyw - 8 bar
        procedure Init(PP, HPP, LPP, BP: real; BDF: byte); override;
        function GetEDBCP: real; virtual;   //cisnienie tylko z hamulca zasadniczego, uzywane do hamulca ED w EP09
        procedure SetED(EDstate: real); //stan hamulca ED do luzowania
      end;

    TEStED= class(TLSt)  //zawor z EP09 - Est4 z oddzielnym przekladnikiem, kontrola rapidu i takie tam
      private
        Nozzles: array[0..10] of real; //dysze
        Zamykajacy: boolean;       //pamiec zaworka zamykajacego
        Przys_blok: boolean;       //blokada przyspieszacza
        Miedzypoj: TReservoir;     //pojemnosc posrednia (urojona) do napelniania ZP i ZS
      public
        procedure Init(PP, HPP, LPP, BP: real; BDF: byte); override;
        function GetPF(PP, dt, Vel: real): real; override;     //przeplyw miedzy komora wstepna i PG
        function GetEDBCP: real; override;   //cisnienie tylko z hamulca zasadniczego, uzywane do hamulca ED w EP09
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
        procedure SetEPS(nEPS: real); override; //stan hamulca EP
        procedure SetLP(TM, LM, TBP: real);  //parametry przystawki wazacej
      end;

    TCV1= class(TBrake)
      private
        CntrlRes: TReservoir;      //zbiornik steruj¹cy
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

    TKE= class(TBrake) //Knorr Einheitsbauart — jeden do wszystkiego
      private
        RapidStatus: boolean;
        ImplsRes: TReservoir;      //komora impulsowa
        CntrlRes: TReservoir;      //zbiornik steruj¹cy
        Brak2Res: TReservoir;      //zbiornik pomocniczy 2        
        BVM: real;                 //przelozenie PG-CH
        TareM, LoadM: real;        //masa proznego i pelnego
        TareBP: real;              //cisnienie dla proznego
        LoadC: real;               //wspolczynnik zaladowania
        RM: real;                  //przelozenie rapida
        LBP: real;                 //cisnienie hamulca pomocniczego
      public
        procedure SetRM(RMR: real);   //ustalenie przelozenia rapida
        function GetPF(PP, dt, Vel: real): real; override;     //przeplyw miedzy komora wstepna i PG
        procedure Init(PP, HPP, LPP, BP: real; BDF: byte); override;
        function GetHPFlow(HP, dt: real): real; override; //przeplyw - 8 bar
        function GetCRP: real; override;
        procedure CheckState(BCP: real; var dV1: real);
        procedure CheckReleaser(dt: real); //odluzniacz
        function CVs(bp: real): real;      //napelniacz sterujacego
        function BVs(BCP: real): real;     //napelniacz pomocniczego
        procedure PLC(mass: real);  //wspolczynnik cisnienia przystawki wazacej
        procedure SetLP(TM, LM, TBP: real);  //parametry przystawki wazacej
        procedure SetLBP(P: real);   //cisnienie z hamulca pomocniczego
      end;




  //klasa obejmujaca krany
    THandle= class
      private
//        BCP: integer;
      public
        Time: boolean;
        TimeEP: boolean;
        Sounds: array[0..4] of real;       //wielkosci przeplywow dla dzwiekow              
        function GetPF(i_bcp:real; pp, hp, dt, ep: real): real; virtual;
        procedure Init(press: real); virtual;
        function GetCP(): real; virtual;
        procedure SetReductor(nAdj: real); virtual;
        function GetSound(i: byte): real; virtual;
        function GetPos(i: byte): real; virtual;
      end;

    TFV4a= class(THandle)
      private
        CP, TP, RP: real;      //zbiornik steruj¹cy, czasowy, redukcyjny
      public
        function GetPF(i_bcp:real; pp, hp, dt, ep: real): real; override;
        procedure Init(press: real); override;
      end;

    TFV4aM= class(THandle)
      private
        CP, TP, RP: real;      //zbiornik steruj¹cy, czasowy, redukcyjny
        XP: real;              //komora powietrzna w reduktorze — jest potrzebna do odwzorowania fali
        RedAdj: real;          //dostosowanie reduktora cisnienia (krecenie kapturkiem)
//        Sounds: array[0..4] of real;       //wielkosci przeplywow dla dzwiekow
        Fala: boolean;
      public
        function GetPF(i_bcp:real; pp, hp, dt, ep: real): real; override;
        procedure Init(press: real); override;
        procedure SetReductor(nAdj: real); override;
        function GetSound(i: byte): real; override;
        function GetPos(i: byte): real; override;
      end;

{    FBS2= class(THandle)
      private
        CP, TP, RP: real;      //zbiornik steruj¹cy, czasowy, redukcyjny
        XP: real;              //komora powietrzna w reduktorze — jest potrzebna do odwzorowania fali
        RedAdj: real;          //dostosowanie reduktora cisnienia (krecenie kapturkiem)
//        Sounds: array[0..4] of real;       //wielkosci przeplywow dla dzwiekow
        Fala: boolean;
      public
        function GetPF(i_bcp:real; pp, hp, dt, ep: real): real; override;
        procedure Init(press: real); override;
        procedure SetReductor(nAdj: real); override;
        function GetSound(i: byte): real; override;
        function GetPos(i: byte): real; override;
      end;                    }

{    TD2= class(THandle)
      private
        CP, TP, RP: real;      //zbiornik steruj¹cy, czasowy, redukcyjny
        XP: real;              //komora powietrzna w reduktorze — jest potrzebna do odwzorowania fali
        RedAdj: real;          //dostosowanie reduktora cisnienia (krecenie kapturkiem)
//        Sounds: array[0..4] of real;       //wielkosci przeplywow dla dzwiekow
        Fala: boolean;
      public
        function GetPF(i_bcp:real; pp, hp, dt, ep: real): real; override;
        procedure Init(press: real); override;
        procedure SetReductor(nAdj: real); override;
        function GetSound(i: byte): real; override;
        function GetPos(i: byte): real; override;
      end;}

    TM394= class(THandle)
      private
        CP: real;      //zbiornik steruj¹cy, czasowy, redukcyjny
        RedAdj: real;          //dostosowanie reduktora cisnienia (krecenie kapturkiem)
      public
        function GetPF(i_bcp:real; pp, hp, dt, ep: real): real; override;
        procedure Init(press: real); override;
        procedure SetReductor(nAdj: real); override;
        function GetCP: real; override;
        function GetPos(i: byte): real; override;
      end;

    TH14K1= class(THandle)
      private
        CP: real;      //zbiornik steruj¹cy, czasowy, redukcyjny
        RedAdj: real;          //dostosowanie reduktora cisnienia (krecenie kapturkiem)
      public
        function GetPF(i_bcp:real; pp, hp, dt, ep: real): real; override;
        procedure Init(press: real); override;
        procedure SetReductor(nAdj: real); override;
        function GetCP: real; override;
        function GetPos(i: byte): real; override;
      end;

    TSt113= class(TH14K1)
      private
        EPS: real;
      public
        function GetPF(i_bcp:real; pp, hp, dt, ep: real): real; override;
        function GetCP: real; override;
        function GetPos(i: byte): real; override;
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

    TH1405= class(THandle)
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
        function GetPos(i: byte): real; override;
        function GetSound(i: byte): real; override;
        procedure Init(press: real); override;
      end;

function PF(P1,P2,S:real;DP:real = 0.25):real;
function PF1(P1,P2,S:real):real;

function PFVa(PH,PL,S,LIM:real;DP:real = 0.1):real; //zawor napelniajacy z PH do PL, PL do LIM
function PFVd(PH,PL,S,LIM:real;DP:real = 0.1):real; //zawor wypuszczajacy z PH do PL, PH do LIM

implementation

uses _mover;
//---FUNKCJE OGOLNE---

const DPL=0.25;

function PR(p1,p2:real):real;
var ph,pl: real;
begin
  ph:=Max0R(p1,p2)+0.1;
  pl:=p1+p2-ph+0.2;
  PR:=(p2-p1)/(1.13*ph-pl);
end;

function PF_old(P1,P2,S:real):real;
var ph,pl: real;
begin
  PH:=Max0R(P1,P2)+1;
  PL:=P1+P2-PH+2;
  if  PH-PL<0.0001 then PF_old:=0 else
  if (PH-PL)<0.05 then
    PF_old:=20*(PH-PL)*(PH+1)*222*S*(P2-P1)/(1.13*ph-pl)
  else
    PF_old:=(PH+1)*222*S*(P2-P1)/(1.13*ph-pl);
end;

function PF(P1,P2,S:real;DP:real = 0.25):real;
var ph,pl,sg,fm: real;
begin
  PH:=Max0R(P1,P2)+1; //wyzsze cisnienie absolutne
  PL:=P1+P2-PH+2;  //nizsze cisnienie absolutne
  sg:=PL/PH; //bezwymiarowy stosunek cisnien
  fm:=PH*197*S*sign(P2-P1); //najwyzszy mozliwy przeplyw, wraz z kierunkiem
  if (SG>0.5) then //jesli ponizej stosunku krytycznego
    if (PH-PL)<DP then //niewielka roznica cisnien
      PF:=(1-sg)/DPL*fm*2*SQRT((DP)*(PH-DP))
//      PF:=1/DPL*(PH-PL)*fm*2*SQRT((sg)*(1-sg))
    else
      PF:=fm*2*SQRT((sg)*(1-sg))
  else             //powyzej stosunku krytycznego
    PF:=fm;
end;

function PF1(P1,P2,S:real):real;
var ph,pl,sg,fm: real;
const DPS=0.001;
begin
  PH:=Max0R(P1,P2)+1; //wyzsze cisnienie absolutne
  PL:=P1+P2-PH+2;  //nizsze cisnienie absolutne
  sg:=PL/PH; //bezwymiarowy stosunek cisnien
  fm:=PH*197*S*sign(P2-P1); //najwyzszy mozliwy przeplyw, wraz z kierunkiem
  if (SG>0.5) then //jesli ponizej stosunku krytycznego
    if (SG<DPS) then //niewielka roznica cisnien
      PF1:=(1-SG)/DPS*fm*2*SQRT((DPS)*(1-DPS))
    else
      PF1:=fm*2*SQRT((sg)*(1-sg))
  else             //powyzej stosunku krytycznego
    PF1:=fm;
end;

function PFVa(PH,PL,S,LIM:real;DP:real = 0.1):real; //zawor napelniajacy z PH do PL, PL do LIM
var sg,fm: real;
begin
  if LIM>PL then
   begin
    LIM:=LIM+1;
    PH:=PH+1; //wyzsze cisnienie absolutne
    PL:=PL+1;  //nizsze cisnienie absolutne
    sg:=PL/PH; //bezwymiarowy stosunek cisnien
    fm:=PH*197*S; //najwyzszy mozliwy przeplyw, wraz z kierunkiem
    if (LIM-PL)<DP then fm:=fm*(LIM-PL)/DP; //jesli jestesmy przy nastawieniu, to zawor sie przymyka
    if (SG>0.5) then //jesli ponizej stosunku krytycznego
      if (PH-PL)<DPL then //niewielka roznica cisnien
        PFVa:=(PH-PL)/DPL*fm*2*SQRT((sg)*(1-sg))
      else
        PFVa:=fm*2*SQRT((sg)*(1-sg))
    else             //powyzej stosunku krytycznego
      PFVa:=fm;
   end
  else
    PFVa:=0;
end;

function PFVd(PH,PL,S,LIM:real;DP:real = 0.1):real; //zawor wypuszczajacy z PH do PL, PH do LIM
var sg,fm: real;
begin
  if LIM<PH then
   begin
    LIM:=LIM+1;
    PH:=PH+1; //wyzsze cisnienie absolutne
    PL:=PL+1;  //nizsze cisnienie absolutne
    sg:=PL/PH; //bezwymiarowy stosunek cisnien
    fm:=PH*197*S; //najwyzszy mozliwy przeplyw, wraz z kierunkiem
    if (PH-LIM)<0.1 then fm:=fm*(PH-LIM)/DP; //jesli jestesmy przy nastawieniu, to zawor sie przymyka
    if (SG>0.5) then //jesli ponizej stosunku krytycznego
    if (PH-PL)<DPL then //niewielka roznica cisnien
        PFVd:=(PH-PL)/DPL*fm*2*SQRT((sg)*(1-sg))
      else
        PFVd:=fm*2*SQRT((sg)*(1-sg))
    else             //powyzej stosunku krytycznego
      PFVd:=fm;
   end
  else
    PFVd:=0;
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
//var VtoC: real;
begin
//  VtoC:=Vol/Cap;
  pa:=P*0.1
end;


(* NOWSZA WERSJA - maksymalne ciœnienie to ok. 4,75 bar, co powoduje
//                 problemy przy rapidzie w lokomotywach, gdyz jest3
//                 osiagany wierzcholek paraboli
function TBrakeCyl.P:real;
var VtoC: real;
begin
  VtoC:=Vol/Cap;
  if VtoC<0.06 then P:=VtoC/4
  else if VtoC>0.88 then P:=0.5+(VtoC-0.88)*1.043-0.064*(VtoC-0.88)*(VtoC-0.88)
  else P:=0.15+0.35/0.82*(VtoC-0.06);
end; *)

//(* STARA WERSJA
function TBrakeCyl.P:real;
var VtoC: real; //stosunek cisnienia do objetosci
const
  VS = 0.005;
  pS = 0.05;
  VD = 1.40;
  cD = 1;
  pD = VD-cD;
begin
  VtoC:=Vol/Cap;
//  P:=VtoC;
  if VtoC<VS then P:=VtoC*pS/VS           //objetosc szkodliwa
  else if VtoC>VD then P:=VtoC-cD        //caly silownik
  else P:=pS+(VtoC-VS)/(VD-VS)*(pD-pS); //wysuwanie tloka
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
  i_mat:=i_mat and (255-bp_MHS);
  case i_mat of
  bp_P10Bg:   FM:=TP10Bg.Create;
  bp_P10Bgu:  FM:=TP10Bgu.Create;
  bp_FR513:   FM:=TFR513.Create;
  bp_Cosid:   FM:=TCosid.Create;
  bp_P10yBg:  FM:=TP10yBg.Create;
  bp_P10yBgu: FM:=TP10yBgu.Create;
  bp_D1:      FM:=TDisk1.Create;
  bp_D2:      FM:=TDisk2.Create;  
  else //domyslnie
  FM:=TP10.Create;
  end;
end;

//inicjalizacja hamulca (stan poczatkowy)
procedure TBrake.Init(PP, HPP, LPP, BP: real; BDF: byte);
begin

end;

//pobranie wspolczynnika tarcia materialu
function TBrake.GetFC(Vel, N: real): real;
begin
  GetFC:=FM.GetFC(N, Vel);
end;

//cisnienie cylindra hamulcowego
function TBrake.GetBCP: real;
begin
  GetBCP:=BrakeCyl.P;
end;

//cisnienie zbiornika pomocniczego
function TBrake.GetBRP: real;
begin
  GetBRP:=BrakeRes.P;
end;

//cisnienie komory wstepnej
function TBrake.GetVRP: real;
begin
  GetVRP:=ValveRes.P;
end;

//cisnienie zbiornika sterujacego
function TBrake.GetCRP: real;
begin
  GetCRP:=0;
end;

//przeplyw z przewodu glowneg
function TBrake.GetPF(PP, dt, Vel: real): real;
begin
  ValveRes.Act;
  BrakeCyl.Act;
  BrakeRes.Act;
  GetPF:=0;
end;

//przeplyw z przewodu zasilajacego
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

procedure TBrake.SetEPS(nEPS: real);
begin

end;

procedure TBrake.ASB(state: byte);
begin                           //255-b_asb(32)
  BrakeStatus:=(BrakeStatus and 223) or state*b_asb;
end;

function TBrake.GetStatus(): byte;
begin
  GetStatus:=BrakeStatus;
end;

function TBrake.GetSoundFlag: byte;
begin
  GetSoundFlag:=SoundFlag;
  SoundFlag:=0;
end;

procedure TBrake.SetASBP(press: real);
begin
  ASBP:=press;
end;

procedure TBrake.ForceEmptiness();
begin
  ValveRes.CreatePress(0);
  BrakeRes.CreatePress(0);
  ValveRes.Act();
  BrakeRes.Act();
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
    VVP, BVP, CVP, BCP: real;
    temp: real;
begin

 BVP:=BrakeRes.P;
 VVP:=ValveRes.P;
 CVP:=BrakeCyl.P;
 BCP:=BrakeCyl.P;

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

  if((BrakeStatus and b_hld)=b_off) and (not DCV) then
   dV:=PF(0,CVP,0.0068*sizeBC)*dt
  else dV:=0;
  BrakeCyl.Flow(-dV);

  if(BCP>LBP+0.01) and (DCV) then
   dV:=PF(0,CVP,0.1*sizeBC)*dt
  else dV:=0;
  BrakeCyl.Flow(-dV);

//hamulec EP
  temp:=BVP*Byte(EPS>0);
  dV:=PF(temp,LBP,0.0015)*dt*EPS*EPS*Byte(LBP*EPS<MaxBP*LoadC);
  LBP:=LBP-dV;
  dV:=0;

//przeplyw ZP <-> silowniki
  if((BrakeStatus and b_on)=b_on)and((TareBP<0.1)or(BCP<MaxBP*LoadC))then
    if(BVP>LBP)then
     begin
      DCV:=false;
      dV:=PF(BVP,CVP,0.017*sizeBC)*dt;
     end
    else dV:=0
  else dV:=0;
  BrakeRes.Flow(dV);
  BrakeCyl.Flow(-dV);
  if(DCV)then
   dVP:=PF(LBP,BCP,0.01*sizeBC)*dt
  else dVP:=0;
  BrakeCyl.Flow(-dVP);
  if(dVP>0)then dVP:=0;
//przeplyw ZP <-> rozdzielacz
  if((BrakeStatus and b_hld)=b_off)then
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

function TWest.GetHPFlow(HP, dt: real): real;
begin
  GetHPFlow:=dVP;
end;

procedure TWest.SetLBP(P: real);
begin
  LBP:=P;
  if P>BrakeCyl.P then
//   begin
    DCV:=true;
//   end
//  else
//    LBP:=P;  
end;

procedure TWest.SetEPS(nEPS: real);
var
  BCP: real;
begin
  BCP:=BrakeCyl.P;
  if nEPS>0 then
    DCV:=true
  else if nEPS=0 then
   begin
    if(EPS<>0)then
     begin
      if(LBP>0.4)then
       LBP:=BrakeCyl.P;
      if(LBP<0.15)then
       LBP:=0;
     end;
   end;
  EPS:=nEPS;
end;

procedure TWest.PLC(mass: real);
begin
  LoadC:=1+Byte(Mass<LoadM)*((TareBP+(MaxBP-TareBP)*(mass-TareM)/(LoadM-TareM))/MaxBP-1);
end;

procedure TWest.SetLP(TM, LM, TBP: real);
begin
  TareM:=TM;
  LoadM:=LM;
  TareBP:=TBP;
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
  VVP:=ValveRes.P;
//  if (BVP<VVP) then
//    VVP:=(BVP+VVP)/2;
  CVP:=CntrlRes.P-0.0;

//sprawdzanie stanu
 if ((BrakeStatus and 1)=1)and(BCP>0.25)then
   if(VVP+0.003+BCP/BVM<CVP)then
     BrakeStatus:=(BrakeStatus or 2) //hamowanie stopniowe
   else if(VVP-0.003+(BCP-0.1)/BVM>CVP) then
     BrakeStatus:=(BrakeStatus and 252) //luzowanie
   else if(VVP+BCP/BVM>CVP) then
     BrakeStatus:=(BrakeStatus and 253) //zatrzymanie napelaniania
   else
 else
   if(VVP+0.10<CVP)and(BCP<0.25)then    //poczatek hamowania
    begin
     if (BrakeStatus and 1)=0 then
      begin
       ValveRes.CreatePress(0.02*VVP);
       SoundFlag:=SoundFlag or sf_Acc;
       ValveRes.Act;
      end;
     BrakeStatus:=(BrakeStatus or 3);

//     ValveRes.CreatePress(0);
//     dV1:=1;
    end
   else if(VVP+(BCP-0.1)/BVM<CVP)and((CVP-VVP)*BVM>0.25)and(BCP>0.25)then //zatrzymanie luzowanie
     BrakeStatus:=(BrakeStatus or 1);

 if (BrakeStatus and 1)=0 then
   SoundFlag:=SoundFlag or sf_CylU;
end;

function TESt.CVs(bp: real): real;
var VVP, BVP, CVP: real;
begin
  BVP:=BrakeRes.P;
  CVP:=CntrlRes.P;
  VVP:=ValveRes.P;

//przeplyw ZS <-> PG
  if(VVP<CVP-0.12)or(BVP<CVP-0.3)or(bp>0.4)then
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
  if(BVP<CVP-0.3)then
    BVs:=0.6
  else
    if(BCP<0.5) then
      if(VVP>CVP+0.4)then
        BVs:=0.1
      else
        BVs:=0.3
    else
      BVs:=0;
end;





function TESt.GetPF(PP, dt, Vel: real): real;
var dv, dv1, temp:real;
    VVP, BVP, BCP, CVP: real;
begin
 BVP:=BrakeRes.P;
 VVP:=ValveRes.P;
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
  if(VVP-0.05>BVP)then
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
  CVP:=CntrlRes.P; //110115 - konsultacje warszawa1

  dV:=0; dV1:=0;

//odluzniacz
  CheckReleaser(dt);

//sprawdzanie stanu
 if ((BrakeStatus and 1)=1)and(BCP>0.25) then
   if(VVP+0.003+BCP/BVM<CVP-0.12)then
     BrakeStatus:=(BrakeStatus or 2) //hamowanie stopniowe
   else if(VVP-0.003+BCP/BVM>CVP-0.12) then
     BrakeStatus:=(BrakeStatus and 252) //luzowanie
   else if(VVP+BCP/BVM>CVP-0.12) then
     BrakeStatus:=(BrakeStatus and 253) //zatrzymanie napelaniania
   else
 else
   if(VVP+0.10<CVP-0.12)and(BCP<0.25) then    //poczatek hamowania
    begin
     if (BrakeStatus and 1)=0 then
      begin
//       ValveRes.CreatePress(0.5*VVP);  //110115 - konsultacje warszawa1
//       SoundFlag:=SoundFlag or sf_Acc;
//       ValveRes.Act;
      end;
     BrakeStatus:=(BrakeStatus or 3);
    end
   else if(VVP+BCP/BVM<CVP-0.12)and(BCP>0.25) then //zatrzymanie luzowanie
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
  dV:=PF(temp,LBP,0.00053+0.00060*Byte(EPS<0))*dt*EPS*EPS*Byte(LBP*EPS<MaxBP*LoadC);
  LBP:=LBP-dV;

//luzowanie KI
  if(BrakeStatus and b_hld)=b_off then
   dV:=PF(0,BCP,0.00083)*dt
  else dV:=0;
  ImplsRes.Flow(-dV);
//przeplyw ZP <-> KI
  if ((BrakeStatus and b_on)=b_on) and (BCP<MaxBP*LoadC) then
   dV:=PF(BVP,BCP,0.0006)*dt
  else dV:=0;
  BrakeRes.Flow(dV);
  ImplsRes.Flow(-dV);
//przeplyw PG <-> rozdzielacz
  dV:=PF(PP,VVP,0.01*sizeBR)*dt;
  ValveRes.Flow(-dV);

  GetPF:=dV-dV1;

  temp:=Max0R(BCP,LBP);

  if(ImplsRes.P>LBP+0.01)then
    LBP:=0;

//luzowanie CH
  if(BrakeCyl.P>temp+0.005)or(Max0R(ImplsRes.P,8*LBP)<0.05) then
   dV:=PF(0,BrakeCyl.P,0.25*sizeBC*(0.01+(BrakeCyl.P-temp)))*dt
  else dV:=0;
  BrakeCyl.Flow(-dV);
//przeplyw ZP <-> CH
  if(BrakeCyl.P<temp-0.005)and(Max0R(ImplsRes.P,8*LBP)>0.10)and(Max0R(BCP,LBP)<MaxBP*LoadC)then
   dV:=PF(BVP,BrakeCyl.P,0.35*sizeBC*(0.01-(BrakeCyl.P-temp)))*dt
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
  if (EPS>0) and (LBP+0.01<BrakeCyl.P) then
    LBP:=BrakeCyl.P
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
  VVP:=ValveRes.P;
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
  if(VVP-0.05>BVP)then
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
 VVP:=ValveRes.P;
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
   dV:=PF(0,BCP,0.00037*1.14*15/19)*dt
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
  if(BVP<VVP-0.05)then  //or((PP<CVP)and(CVP<PP-0.1)
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

  RapidTemp:=RapidTemp+(0.9*Byte(RapidStatus)-RapidTemp)*dt/2;
  temp:=1.9-RapidTemp;
  if((BrakeStatus and b_asb)=b_asb)then
    temp:=1000;
//luzowanie CH
  if(BrakeCyl.P*temp>ImplsRes.P+0.005)or(ImplsRes.P<0.25) then
   if((BrakeStatus and b_asb)=b_asb)then
     dV:=PFVd(BrakeCyl.P,0,0.115*sizeBC*4,ImplsRes.P/temp)*dt
   else
     dV:=PFVd(BrakeCyl.P,0,0.115*sizeBC,ImplsRes.P/temp)*dt
//   dV:=PF(0,BrakeCyl.P,0.115*sizeBC/2)*dt
//   dV:=PFVd(BrakeCyl.P,0,0.015*sizeBC/2,ImplsRes.P/temp)*dt
  else dV:=0;
  BrakeCyl.Flow(-dV);
//przeplyw ZP <-> CH
  if(BrakeCyl.P*temp<ImplsRes.P-0.005)and(ImplsRes.P>0.3) then
//   dV:=PFVa(BVP,BrakeCyl.P,0.020*sizeBC,ImplsRes.P/temp)*dt
   dV:=PFVa(BVP,BrakeCyl.P,0.60*sizeBC,ImplsRes.P/temp)*dt
  else dV:=0;
  BrakeRes.Flow(-dV);
  BrakeCyl.Flow(+dV);

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
 VVP:=ValveRes.P;
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
  if(VVP-0.05>BVP)then
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
   if(CVP<1*0)then
     BrakeStatus:=BrakeStatus and 247
   else
    begin           //008
     dV:=PF1(CVP,BCP,0.024)*dt;
     CntrlRes.Flow(+dV);
//     dV1:=+dV; //minus potem jest
//     ImplsRes.Flow(-dV1);
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

  dV:=PF1(CVP,VVP,0.0015*temp/1.8/2)*dt;
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
    dV:=PF(VVP,BCP,0.00043*(1.5-Byte(((CVP-BCP)*BVM>1)and(BrakeDelayFlag=bdelay_G))),0.1)*dt
   else if (CVP-BCP)<1.5 then
    dV:=PF(VVP,BCP,0.001472*(1.36-Byte(((CVP-BCP)*BVM>1)and(BrakeDelayFlag=bdelay_G))),0.1)*dt
  else dV:=0;

  ImplsRes.Flow(-dV);
  ValveRes.Flow(+dV);
//przeplyw PG <-> rozdzielacz
  dV:=PF(PP,VVP,0.01,0.1)*dt;
  ValveRes.Flow(-dV);

  GetPF:=dV-dV1;

//  if Vel>55 then temp:=0.72 else
//    temp:=1;{R}
//cisnienie PP
  RapidTemp:=RapidTemp+(RM*Byte((Vel>55)and(BrakeDelayFlag=bdelay_R))-RapidTemp)*dt/2;
  temp:=1-RapidTemp;
  if EDFlag>0.2 then temp:=10000;

//powtarzacz — podwojny zawor zwrotny
  temp:=Max0R(((CVP-BCP)*BVM+ASBP*Byte((BrakeStatus and b_asb)=b_asb))/temp,LBP);
//luzowanie CH
  if(BrakeCyl.P>temp+0.005)or(temp<0.28) then
//   dV:=PF(0,BrakeCyl.P,0.0015*3*sizeBC)*dt
//   dV:=PF(0,BrakeCyl.P,0.005*3*sizeBC)*dt
   dV:=PFVd(BrakeCyl.P,0,0.005*7*sizeBC,temp)*dt
  else dV:=0;
  BrakeCyl.Flow(-dV);
//przeplyw ZP <-> CH
  if(BrakeCyl.P<temp-0.005)and(temp>0.29) then
//   dV:=PF(BVP,BrakeCyl.P,0.002*3*sizeBC*2)*dt
   dV:=-PFVa(BVP,BrakeCyl.P,0.002*7*sizeBC*2,temp)*dt
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

  EDFlag:=0;

  BrakeDelayFlag:=BDF;
end;

procedure TLSt.SetLBP(P: real);
begin
  LBP:=P;
end;

function TLSt.GetEDBCP: real;
var CVP, BCP: real;
begin
  CVP:=CntrlRes.P;
  BCP:=ImplsRes.P;
  GetEDBCP:=(CVP-BCP)*BVM;
end;

procedure TLSt.SetED(EDstate: real);
begin
  EDFlag:=EDstate;
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

//---EStED---

function TEStED.GetPF(PP, dt, Vel: real): real;
var dv, dv1, temp:real;
    VVP, BVP, BCP, CVP, MPP, nastG: real;
    i: byte;
begin
  BVP:=BrakeRes.P;
  VVP:=ValveRes.P;
  BCP:=ImplsRes.P;
  CVP:=CntrlRes.P-0.0;
  MPP:=Miedzypoj.P;
  dV1:=0;

  nastG:=(BrakeDelayFlag and bdelay_G);

//sprawdzanie stanu
  if(BCP<0.25)and(VVP+0.08>CVP)then Przys_blok:=false;

//sprawdzanie stanu
   if(VVP+0.002+BCP/BVM<CVP-0.05)and(Przys_blok)then
     BrakeStatus:=(BrakeStatus or 3) //hamowanie stopniowe
   else if(VVP-0.002+(BCP-0.1)/BVM>CVP-0.05) then
     BrakeStatus:=(BrakeStatus and 252) //luzowanie
   else if(VVP+BCP/BVM>CVP-0.05) then
     BrakeStatus:=(BrakeStatus and 253) //zatrzymanie napelaniania
   else if(VVP+(BCP-0.1)/BVM<CVP-0.05)and(BCP>0.25)then //zatrzymanie luzowania
     BrakeStatus:=(BrakeStatus or 1);

 if(VVP+0.10<CVP)and(BCP<0.25)then    //poczatek hamowania
   if (not Przys_blok) then
    begin
     ValveRes.CreatePress(0.75*VVP);
     SoundFlag:=SoundFlag or sf_Acc;
     ValveRes.Act;
     Przys_blok:=true;
    end;


 if(BCP>0.5)then
   Zamykajacy:=true
 else if(VVP-0.6<MPP) then
   Zamykajacy:=false;

 if(BrakeStatus and b_rls=b_rls)then
  begin
   dV:=PF(CVP,BCP,0.024)*dt;
   CntrlRes.Flow(+dV);
  end; 

//luzowanie
  if(BrakeStatus and b_hld)=b_off then
   dV:=PF(0,BCP,Nozzles[3]*nastG+(1-nastG)*Nozzles[1])*dt
  else dV:=0;
  ImplsRes.Flow(-dV);
  if((BrakeStatus and b_on)=b_on)and(BCP<MaxBP)then
   dV:=PF(BVP,BCP,Nozzles[2]*(nastG+2*Byte(BCP<0.8))+Nozzles[0]*(1-nastG))*dt
  else dV:=0;
  ImplsRes.Flow(-dV);
  BrakeRes.Flow(dV);

//przeplyw testowy miedzypojemnosci
  if(MPP<CVP-0.3)then
    temp:=Nozzles[4]
  else
    if(BCP<0.5) then
      if(Zamykajacy)then
        temp:=Nozzles[8]  //1.25
      else
        temp:=Nozzles[7]
    else
      temp:=0;
  dV:=PF(MPP,VVP,temp);

  if(MPP<CVP-0.17)then
    temp:=0
  else
  if(MPP>CVP-0.08)then
    temp:=Nozzles[5]
  else
    temp:=Nozzles[6];
  dV:=dV+PF(MPP,CVP,temp);

  if(MPP-0.05>BVP)then
    dV:=dV+PF(MPP-0.05,BVP,Nozzles[10]*nastG+(1-nastG)*Nozzles[9]);
  if MPP>VVP then dV:=dV+PF(MPP,VVP,0.02);
  Miedzypoj.Flow(dV*dt*0.15);


  RapidTemp:=RapidTemp+(RM*Byte((Vel>55)and(BrakeDelayFlag=bdelay_R))-RapidTemp)*dt/2;
  temp:=1-RapidTemp;
//  if EDFlag then temp:=1000;
//  temp:=temp/(1-);

//powtarzacz — podwojny zawor zwrotny
  temp:=Max0R(BCP/temp*Min0R(Max0R(1-EDFlag,0),1),LBP);

  if(BrakeCyl.P>temp)then
   dV:=-PFVd(BrakeCyl.P,0,0.02,temp)*dt
  else
  if(BrakeCyl.P<temp)then
   dV:=PFVa(BVP,BrakeCyl.P,0.02,temp)*dt
  else dV:=0;

  BrakeCyl.Flow(dV);
  if dV>0 then
  BrakeRes.Flow(-dV);


//przeplyw ZS <-> PG
  if(MPP<CVP-0.17)then
    temp:=0
  else
  if(MPP>CVP-0.08)then
    temp:=Nozzles[5]
  else
    temp:=Nozzles[6];
  dV:=PF(CVP,MPP,temp)*dt;
  CntrlRes.Flow(+dV);
  ValveRes.Flow(-0.02*dV);
  dV1:=dV1+0.98*dV;

//przeplyw ZP <-> MPJ
  if(MPP-0.05>BVP)then
   dV:=PF(BVP,MPP-0.05,Nozzles[10]*nastG+(1-nastG)*Nozzles[9])*dt
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
  ImplsRes.Act;
  GetPF:=dV-dV1;
end;

procedure TEStED.Init(PP, HPP, LPP, BP: real; BDF: byte);
var i:integer;
begin
  inherited;

  ValveRes.CreatePress(1*PP);
  BrakeCyl.CreatePress(1*BP);

//  CntrlRes:=TReservoir.Create;
//  CntrlRes.CreateCap(15);
//  CntrlRes.CreatePress(1*HPP);

  BrakeStatus:=Byte(BP>1)*1;
  Miedzypoj:=TReservoir.Create;
  Miedzypoj.CreateCap(5);
  Miedzypoj.CreatePress(PP);

  ImplsRes.CreateCap(1);
  ImplsRes.CreatePress(BP);

  BVM:=1/(HPP-0.05-LPP)*MaxBP;

  BrakeDelayFlag:=BDF;
  Zamykajacy:=false;
  EDFlag:=0;

  Nozzles[0]:=1.250/1.7;
  Nozzles[1]:=0.907;
  Nozzles[2]:=0.510/1.7;
  Nozzles[3]:=0.524/1.17;
  Nozzles[4]:=7.4;
  Nozzles[7]:=5.3;
  Nozzles[8]:=2.5;
  Nozzles[9]:=7.28;
  Nozzles[10]:=2.96;
  Nozzles[5]:=1.1;
  Nozzles[6]:=0.9;

  for i:=0 to 10 do
   begin
    Nozzles[i]:=Nozzles[i]*Nozzles[i]*3.14159/4000;
   end;

end;

function TEStED.GetEDBCP: real;
begin
  GetEDBCP:=ImplsRes.P;
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
  dV:=Min0R(0,dV);
  BrakeRes.Flow(-dV);
  GetHPFlow:=dV;
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



//--- KNORR KE ---
procedure TKE.CheckReleaser(dt: real);
var VVP, CVP: real;
begin
  VVP:=ValveRes.P;
  CVP:=CntrlRes.P;

//odluzniacz
 if(BrakeStatus and b_rls=b_rls)then
   if(CVP-VVP<0)then
    BrakeStatus:=BrakeStatus and 247
   else
    begin
     CntrlRes.Flow(+PF(CVP,0,0.1)*dt);
    end;
end;

procedure TKE.CheckState(BCP: real; var dV1: real);
var VVP, BVP, CVP: real;
begin
  BVP:=BrakeRes.P;
  VVP:=ValveRes.P;
  CVP:=CntrlRes.P;

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
     ValveRes.CreatePress(0.8*VVP); //przyspieszacz
    end
   else if(VVP+BCP/BVM<CVP)and((CVP-VVP)*BVM>0.25) then //zatrzymanie luzowanie
     BrakeStatus:=(BrakeStatus or 1);
end;

function TKE.CVs(bp: real): real;
var VVP, BVP, CVP: real;
begin
  BVP:=BrakeRes.P;
  CVP:=CntrlRes.P;
  VVP:=ValveRes.P;

//przeplyw ZS <-> PG
  if(bp>0.2)then
    CVs:=0
  else
    if(VVP>CVP+0.4)then
        CVs:=0.05
      else
        CVs:=0.23
end;

function TKE.BVs(BCP: real): real;
var VVP, BVP, CVP: real;
begin
  BVP:=BrakeRes.P;
  CVP:=CntrlRes.P;
  VVP:=ValveRes.P;

//przeplyw ZP <-> rozdzielacz
  if (BVP>VVP) then
    BVs:=0
  else
  if(BVP<CVP-0.3)then
    BVs:=0.6
  else
    BVs:=0.13
end;

function TKE.GetPF(PP, dt, Vel: real): real;
var dv, dv1, temp:real;
    VVP, BVP, BCP, IMP, CVP: real;
begin
 BVP:=BrakeRes.P;
 VVP:=ValveRes.P;
 BCP:=BrakeCyl.P;
 IMP:=ImplsRes.P;
 CVP:=CntrlRes.P;

 dV:=0; dV1:=0;

//sprawdzanie stanu
  CheckState(IMP, dV1);
  CheckReleaser(dt);

//przeplyw ZS <-> PG
  temp:=CVs(IMP);
  dV:=PF(CVP,VVP,0.0015*temp)*dt;
  CntrlRes.Flow(+dV);
  ValveRes.Flow(-0.04*dV);
  dV1:=dV1-0.96*dV;

//luzowanie
  if(BrakeStatus and b_hld)=b_off then
   begin
    if ((BrakeDelayFlag and bdelay_G)=0) then
      temp:=0.283+0.139
    else
      temp:=0.139;
    dV:=PF(0,IMP,0.001*temp)*dt
   end
  else dV:=0;
  ImplsRes.Flow(-dV);

//przeplyw ZP <-> silowniki
  if((BrakeStatus and b_on)=b_on)and(IMP<MaxBP)then
   begin
    temp:=0.113;
    if ((BrakeDelayFlag and bdelay_G)=0) then
      temp:=temp+0.636;
    if (BCP<0.5) then
      temp:=temp+0.785;
    dV:=PF(BVP,IMP,0.001*temp)*dt
   end
  else dV:=0;
  BrakeRes.Flow(dV);
  ImplsRes.Flow(-dV);


//rapid
  if not ((FM is TDisk1) or (FM is TDisk2)) then   //jesli zeliwo to schodz
    RapidStatus:=((BrakeDelayFlag and bdelay_R)=bdelay_R)and(((Vel>50)and(RapidStatus))or(Vel>70))
  else                         //jesli tarczowki, to zostan
    RapidStatus:=((BrakeDelayFlag and bdelay_R)=bdelay_R);

//  temp:=1.9-0.9*Byte(RapidStatus);

  if(RM*RM>0.1)then //jesli jest rapid
    if(RM>0)then //jesli dodatni (naddatek);
      temp:=1-RM*Byte(RapidStatus)
    else
      temp:=1-RM*(1-Byte(RapidStatus))
  else
    temp:=1;
  temp:=temp/LoadC;
//luzowanie CH
//  temp:=Max0R(BCP,LBP);
  IMP:=Max0R(IMP/temp,Max0R(LBP,ASBP*Byte((BrakeStatus and b_asb)=b_asb)));

//luzowanie CH
  if(BCP>IMP+0.005)or(Max0R(ImplsRes.P,8*LBP)<0.25) then
   dV:=PFVd(BCP,0,0.05,IMP)*dt
  else dV:=0;
  BrakeCyl.Flow(-dV);
  if(BCP<IMP-0.005)and(Max0R(ImplsRes.P,8*LBP)>0.3) then
   dV:=PFVa(BVP,BCP,0.05,IMP)*dt
  else dV:=0;
  BrakeRes.Flow(-dV);
  BrakeCyl.Flow(+dV);

//przeplyw ZP <-> rozdzielacz
  temp:=BVs(IMP);
//  if(BrakeStatus and b_hld)=b_off then
  if(IMP<0.25)or(VVP+0.05>BVP)then
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
  ImplsRes.Act;
  GetPF:=dV-dV1;
end;

procedure TKE.Init(PP, HPP, LPP, BP: real; BDF: byte);
begin
   ValveRes.CreatePress(PP);
   BrakeCyl.CreatePress(BP);
   BrakeRes.CreatePress(PP);

   CntrlRes:=TReservoir.Create; //komora sterujaca
   CntrlRes.CreateCap(5);
   CntrlRes.CreatePress(HPP);

   ImplsRes:=TReservoir.Create; //komora zastepcza silownika
   ImplsRes.CreateCap(1);
   ImplsRes.CreatePress(BP);

   BrakeStatus:=0;

   BVM:=1/(HPP-LPP)*MaxBP;

   BrakeDelayFlag:=BDF;
end;

function TKE.GetCRP: real;
begin
  GetCRP:=CntrlRes.P;
end;

function TKE.GetHPFlow(HP, dt: real): real;
var dV: real;
begin
  dV:=PF(HP,BrakeRes.P,0.01)*dt;
  dV:=Min0R(0,dV);
  BrakeRes.Flow(-dV);
  GetHPFlow:=dV;
end;

procedure TKE.PLC(mass: real);
begin
  LoadC:=1+Byte(Mass<LoadM)*((TareBP+(MaxBP-TareBP)*(mass-TareM)/(LoadM-TareM))/MaxBP-1);
end;

procedure TKE.SetLP(TM, LM, TBP: real);
begin
  TareM:=TM;
  LoadM:=LM;
  TareBP:=TBP;
end;

procedure TKE.SetRM(RMR: real);
begin
  RM:=1-RMR;
end;

procedure TKE.SetLBP(P: real);
begin
  LBP:=P;
end;


//---KRANY---

function THandle.GetPF(i_bcp:real; pp, hp, dt, ep: real): real;
begin
  GetPF:=0;
end;

procedure THandle.Init(press: real);
begin
  Time:=false;
  TimeEP:=false;
end;

procedure THandle.SetReductor(nAdj: real);
begin

end;

function THandle.GetCP: real;
begin
  GetCP:=0;
end;

function THandle.GetSound(i: byte): real;
begin
  GetSound:=0;
end;

function THandle.GetPos(i: byte): real;
begin
  GetPos:=0;
end;

//---FV4a---

function TFV4a.GetPF(i_bcp:real; pp, hp, dt, ep: real): real;
const
  LBDelay = 100;
var
  LimPP, dpPipe, dpMainValve, ActFlowSpeed: real;
begin
          ep:=pp; //SPKS!!
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
              dpMainValve:=PF(Min0R(HP,17.1),pp,(ActFlowSpeed)/(LBDelay))*dt
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
              tp:=tp-dt/12/2;
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
  Time:= false;
  TimeEP:=false;
end;


//---FV4a/M--- nowonapisany kran bez poprawki IC

function TFV4aM.GetPF(i_bcp:real; pp, hp, dt, ep: real): real;
function LPP_RP(pos: real): real; //cisnienie z zaokraglonej pozycji;
var i_pos: integer;
begin
  i_pos:=Round(pos-0.5); //zaokraglone w dol
  LPP_RP:=BPT[i_pos][1]+(BPT[i_pos+1][1]-BPT[i_pos][1])*(pos-i_pos); //interpolacja liniowa
end;
function EQ(pos: real; i_pos: real): boolean;
begin
  EQ:=(pos<=i_pos+0.5)and(pos>i_pos-0.5);
end;
const
  LBDelay = 100;
  xpM = 0.3; //mnoznik membrany komory pod
var
  LimPP, dpPipe, dpMainValve, ActFlowSpeed, dp: real;
  pom: real;
  i: byte;
begin
          ep:=pp/2*1.5+ep/2*0.5; //SPKS!!
//          ep:=pp;
//          ep:=cp/3+pp/3+ep/3;
//          ep:=cp;

          for i:=0 to 4 do
            Sounds[i]:=0;
          dp:=0;

          i_bcp:=Max0R(Min0R(i_bcp,5.999),-1.999); //na wszelki wypadek, zeby nie wyszlo poza zakres

          if(tp>0)then
           begin  //jesli czasowy jest niepusty
//            dp:=0.07; //od cisnienia 5 do 0 w 60 sekund ((5-0)*dt/75)
            dp:=0.045;  //2.5 w 55 sekund (5,35->5,15 w PG)
            tp:=tp-dp*dt;
            Sounds[s_fv4a_t]:=dp;
           end
          else       //.08
           begin
            tp:=0;
           end;

          if(xp>0)then //jesli komora pod niepusta jest niepusty
           begin
            dp:=2.5;
            Sounds[s_fv4a_x]:=dp*xp;
            xp:=xp-dt*dp*2; //od cisnienia 5 do 0 w 10 sekund ((5-0)*dt/10)
           end
          else       //.75
            xp:=0;         //jak pusty, to pusty

          Limpp:=Min0R(LPP_RP(i_bcp)+tp*0.08+RedAdj,HP); //pozycja + czasowy lub zasilanie
          ActFlowSpeed:=BPT[Round(i_bcp)][0];

          if(EQ(i_bcp,-1))then pom:=Min0R(HP,5.4+RedAdj) else pom:=Min0R(cp,HP);

          if(pom>rp+0.25)then Fala:=true;
          if(Fala)then
            if(pom>rp+0.3)then
//              if(ep>rp+0.11)then
                xp:=xp-20*PR(pom,xp)*dt
//              else
//                xp:=xp-16*(ep-(ep+0.01))/(0.1)*PR(ep,xp)*dt
            else Fala:=false;

          if(Limpp>cp)then //podwyzszanie szybkie
            cp:=cp+5*60*Min0R(abs(Limpp-cp),0.05)*PR(cp,Limpp)*dt //zbiornik sterujacy
          else
            cp:=cp+13*Min0R(abs(Limpp-cp),0.05)*PR(cp,Limpp)*dt; //zbiornik sterujacy

          Limpp:=pom; //cp
          dpPipe:=Min0R(HP,Limpp+xp*xpM);

          if dpPipe>pp then
            dpMainValve:=-PFVa(HP,pp,ActFlowSpeed/(LBDelay),dpPipe,0.4)
          else
            dpMainValve:=PFVd(pp,0,ActFlowSpeed/(LBDelay),dpPipe,0.4);

          if EQ(i_bcp,-1) then
           begin
            if(tp<5)then tp:=tp+dt; //5/10
            if(tp<1)then tp:=tp-0.5*dt; //5/10            
//            dpMainValve:=dpMainValve*2;//+1*PF(dpPipe,pp,(ActFlowSpeed)/(LBDelay))//coby nie przeszkadzal przy ladowaniu z zaworu obok
           end;

          if EQ(i_bcp,0) then
           begin
            if(tp>2)then
              dpMainValve:=dpMainValve*1.5;//+0.5*PF(dpPipe,pp,(ActFlowSpeed)/(LBDelay))//coby nie przeszkadzal przy ladowaniu z zaworu obok
           end;





          ep:=dpPipe;
          if(EQ(i_bcp,0)or(rp>ep))then
            rp:=rp+PF(rp,ep,0.0007)*dt //powolne wzrastanie, ale szybsze na jezdzie
          else
            rp:=rp+PF(rp,ep,0.000093/2*2)*dt; //powolne wzrastanie i to bardzo  //jednak trzeba wydluzyc, bo obecnie zle dziala
          if (rp<ep) and (rp<BPT[Round(i_bcpNo)][1])then //jesli jestesmy ponizej cisnienia w sterujacym (2.9 bar)
            rp:=rp+PF(rp,cp,0.005)*dt; //przypisz cisnienie w PG - wydluzanie napelniania o czas potrzebny do napelnienia PG

          if(EQ(i_bcp,i_bcpNo))or(EQ(i_bcp,-2))then
           begin
            dp:=PF(0,pp,(ActFlowSpeed)/(LBDelay));
            dpMainValve:=dp;
            Sounds[s_fv4a_e]:=dp;
            Sounds[s_fv4a_u]:=0;
            Sounds[s_fv4a_b]:=0;
            Sounds[s_fv4a_x]:=0;
           end
          else
           begin
            if dpMainValve>0 then
              Sounds[s_fv4a_b]:=dpMainValve
            else
              Sounds[s_fv4a_u]:=-dpMainValve;
           end;

  GetPF:=dpMainValve*dt;
end;

procedure TFV4aM.Init(press: real);
begin
  CP:= press;
  TP:= 0;
  RP:= press;
  XP:= 0;
  Time:=false;
  TimeEP:=false;
end;

procedure TFV4aM.SetReductor(nAdj: real);
begin
  RedAdj:= nAdj;
end;

function TFV4aM.GetSound(i: byte): real;
begin
  if i>4 then
    GetSound:=0
  else
    GetSound:=Sounds[i];  
end;

function TFV4aM.GetPos(i: byte): real;
const
  table: array[0..10] of real = (-2,6,-1,0,-2,1,4,6,0,0,0);
begin
  GetPos:=table[i];
end;

//---M394--- Matrosow

function TM394.GetPF(i_bcp:real; pp, hp, dt, ep: real): real;
const
  LBDelay = 65;
var
  LimPP, dpPipe, dpMainValve, ActFlowSpeed: real;
  bcp: integer;
begin
  bcp:=Round(i_bcp);
  if bcp<-1 then bcp:=1;

  Limpp:=Min0R(BPT_394[bcp][1],HP);
  ActFlowSpeed:=BPT_394[bcp][0];
  if (bcp=1)or(bcp=i_bcpNo) then
    Limpp:=pp;
  if (bcp=0) then
    Limpp:=Limpp+RedAdj;
  if (bcp<>2) then
    if cp<Limpp then
      cp:=cp+4*Min0R(abs(Limpp-cp),0.05)*PR(cp,Limpp)*dt //zbiornik sterujacy
//      cp:=cp+6*(2+Byte(bcp<0))*Min0R(abs(Limpp-cp),0.05)*PR(cp,Limpp)*dt //zbiornik sterujacy
    else
      if bcp=0 then
        cp:=cp-0.2*dt/100
      else
        cp:=cp+4*(1+Byte(bcp<>3)+Byte(bcp>4))*Min0R(abs(Limpp-cp),0.05)*PR(cp,Limpp)*dt; //zbiornik sterujacy

  Limpp:=cp;
  dpPipe:=Min0R(HP,Limpp);

//  if(dpPipe>pp)then //napelnianie
//    dpMainValve:=PF(dpPipe,pp,ActFlowSpeed/(LBDelay))*dt
//  else //spuszczanie
    dpMainValve:=PF(dpPipe,pp,ActFlowSpeed/(LBDelay))*dt;

          if bcp=-1 then
//           begin
              dpMainValve:=PF(HP,pp,(ActFlowSpeed)/(LBDelay))*dt;
//           end;

          if bcp=i_bcpNo then
//           begin
            dpMainValve:=PF(0,pp,(ActFlowSpeed)/(LBDelay))*dt;
//           end;

  GetPF:=dpMainValve;
end;

procedure TM394.Init(press: real);
begin
  CP:= press;
  RedAdj:= 0;
  Time:=true;
  TimeEP:=false;
end;

procedure TM394.SetReductor(nAdj: real);
begin
  RedAdj:= nAdj;
end;

function TM394.GetCP: real;
begin
  GetCP:= CP;
end;

function TM394.GetPos(i: byte): real;
const
  table: array[0..10] of real = (-1,5,-1,0,1,2,4,5,0,0,0);
begin
  GetPos:=table[i];
end;

//---H14K1-- Knorr

function TH14K1.GetPF(i_bcp:real; pp, hp, dt, ep: real): real;
const
  LBDelay = 100;                                    //szybkosc + zasilanie sterujacego
  BPT_K: array[-1..4] of array [0..1] of real= ((10, 0), (4, 1), (0, 1), (4, 0), (4, -1), (15, -1));
  NomPress = 5.0;
var
  LimPP, dpPipe, dpMainValve, ActFlowSpeed: real;
  bcp: integer;
begin
  bcp:=Round(i_bcp);
  if i_bcp<-1 then bcp:=1;
  Limpp:=BPT_K[bcp][1];
  if Limpp<0 then
    Limpp:=0.5*pp
  else if Limpp>0 then
    Limpp:=pp
  else
    Limpp:=cp;
  ActFlowSpeed:=BPT_K[bcp][0];

  cp:=cp+6*Min0R(abs(Limpp-cp),0.05)*PR(cp,Limpp)*dt; //zbiornik sterujacy

  dpMainValve:=0;

  if bcp=-1 then
    dpMainValve:=PF(HP,pp,(ActFlowSpeed)/(LBDelay))*dt;
  if(bcp=0)then
    dpMainValve:=-PFVa(HP,pp,(ActFlowSpeed)/(LBDelay),NomPress+RedAdj)*dt;
  if (bcp>1)and(pp>cp) then
    dpMainValve:=PFVd(pp,0,(ActFlowSpeed)/(LBDelay),cp)*dt;
  if bcp=i_bcpNo then
    dpMainValve:=PF(0,pp,(ActFlowSpeed)/(LBDelay))*dt;

  GetPF:=dpMainValve;
end;

procedure TH14K1.Init(press: real);
begin
  CP:= press;
  RedAdj:= 0;
  Time:=true;
  TimeEP:=true;
end;

procedure TH14K1.SetReductor(nAdj: real);
begin
  RedAdj:= nAdj;
end;

function TH14K1.GetCP: real;
begin
  GetCP:= CP;
end;

function TH14K1.GetPos(i: byte): real;
const
  table: array[0..10] of real = (-1,4,-1,0,1,2,3,4,0,0,0);
begin
  GetPos:=table[i];
end;

//---St113-- Knorr EP

function TSt113.GetPF(i_bcp:real; pp, hp, dt, ep: real): real;
const
  LBDelay = 100;                                    //szybkosc + zasilanie sterujacego
  BPT_K: array[-1..4] of array [0..1] of real= ((10, 0), (4, 1), (0, 1), (4, 0), (4, -1), (15, -1));
  BEP_K: array[-1..5] of real= (0, -1, 1, 0, 0, 0, 0);
  NomPress = 5.0;
var
  LimPP, dpPipe, dpMainValve, ActFlowSpeed: real;
  bcp: integer;
begin
  bcp:=Round(i_bcp);

  EPS:=BEP_K[bcp];

  if bcp>0 then bcp:=bcp-1;

  if bcp<-1 then bcp:=1;
  Limpp:=BPT_K[bcp][1];
  if Limpp<0 then
    Limpp:=0.5*pp
  else if Limpp>0 then
    Limpp:=pp
  else
    Limpp:=cp;
  ActFlowSpeed:=BPT_K[bcp][0];

  cp:=cp+6*Min0R(abs(Limpp-cp),0.05)*PR(cp,Limpp)*dt; //zbiornik sterujacy

  dpMainValve:=0;

  if bcp=-1 then
    dpMainValve:=PF(HP,pp,(ActFlowSpeed)/(LBDelay))*dt;
  if(bcp=0)then
    dpMainValve:=-PFVa(HP,pp,(ActFlowSpeed)/(LBDelay),NomPress+RedAdj)*dt;
  if (bcp>1)and(pp>cp) then
    dpMainValve:=PFVd(pp,0,(ActFlowSpeed)/(LBDelay),cp)*dt;
  if bcp=i_bcpNo then
    dpMainValve:=PF(0,pp,(ActFlowSpeed)/(LBDelay))*dt;

  GetPF:=dpMainValve;
end;

function TSt113.GetCP: real;
begin
  GetCP:=EPS;
end;

function TSt113.GetPos(i: byte): real;
const
  table: array[0..10] of real = (-1,5,-1,0,2,3,4,5,0,0,1);
begin
  GetPos:=table[i];
end;

procedure TSt113.Init(press: real);
begin
  Time:=true;
  TimeEP:=true;
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
//  temp:=Min0R(i_bcp*MaxBP,Min0R(5.0,HP));
  temp:=Min0R(i_bcp*MaxBP,HP);              //0011
  dp:=10*Min0R(abs(temp-BP),0.1)*PF(temp,BP,0.0006*(2+Byte(temp>BP)))*dt;
  BP:=BP-dp;
  GetPF:=-dp;
end;

procedure TFD1.Init(press: real);
begin
  BP:=0;
  MaxBP:=press;
  Time:=false;
  TimeEP:=false;
end;

function TFD1.GetCP: real;
begin
  GetCP:=BP;
end;


//---KNORR---

function TH1405.GetPF(i_bcp:real; pp, hp, dt, ep: real): real;
var dp, temp, A: real;
begin
  pp:=Min0R(pp,MaxBP);
  if i_bcp>0.5 then
   begin
    temp:=Min0R(MaxBP,HP);
    A:=2*(i_bcp-0.5)*0.0011;
    BP:=Max0R(BP,pp);
   end
  else
   begin
    temp:=0;
    A:=0.2*(0.5-i_bcp)*0.0033;
    BP:=Min0R(BP,pp);
   end;
  dp:=PF(temp,BP,A)*dt;
  BP:=BP-dp;
  GetPF:=-dp;
end;

procedure TH1405.Init(press: real);
begin
  BP:=0;
  MaxBP:=press;
  Time:=true;
  TimeEP:=false;
end;

function TH1405.GetCP: real;
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
  Limpp:=Min0R(5*Byte(i_bcp<3.5),hp);
  if (i_bcp>=3.5) and ((i_bcp<4.3)or(i_bcp>5.5)) then
    ActFlowSpeed:=0
  else if (i_bcp>4.3) and (i_bcp<4.8) then
    ActFlowSpeed:=4*(i_bcp-4.3) //konsultacje wawa1 - bylo 8
  else if (i_bcp<4) then
    ActFlowSpeed:=2
  else
    ActFlowSpeed:=4;
  dpMainValve:=PF(Limpp,pp,ActFlowSpeed/(LBDelay))*dt;


  Sounds[s_fv4a_e]:=0;
  Sounds[s_fv4a_u]:=0;
  Sounds[s_fv4a_b]:=0;
  if(i_bcp<3.5)then Sounds[s_fv4a_u]:=-dpMainValve else
  if(i_bcp<4.8)then Sounds[s_fv4a_b]:=dpMainValve else
  if(i_bcp<5.5)then Sounds[s_fv4a_e]:=dpMainValve;

  GetPF:=dpMainValve;
  if(i_bcp<-0.5)then
    EPS:=-1
  else if(i_bcp>0.5)and(i_bcp<4.7)then
    EPS:=1
  else
    EPS:=0;
//    EPS:=i_bcp*Byte(i_bcp<2)
end;

function TFVel6.GetCP: real;
begin
  GetCP:=EPS;
end;

function TFVel6.GetPos(i: byte): real;
const
  table: array[0..10] of real = (-1,6,-1,0,6,4,4.7,5,-1,0,1);
begin
  GetPos:=table[i];
end;

function TFVel6.GetSound(i: byte): real;
begin
  if i>2 then
    GetSound:=0
  else
    GetSound:=Sounds[i];
end;

procedure TFVel6.Init(press: real);
begin
  Time:=true;
  TimeEP:=true;
end;

end.


