unit Oerlikon_ESt;          {fizyka hamulcow Oerlikon ESt dla symulatora}

(*
    MaSzyna EU07 - SPKS
    Brakes. Oerlikon ESt.
    Copyright (C) 2007-2014 Maciej Cierniak
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
- przekladniki AL2
- przeciwposlizgi
- rapid REL2
- HGB300
- inne srednice
Co brakuje:
- dobry przyspieszacz
- mozliwosc zasilania z wysokiego cisnienia ESt4
- ep: EP1 i EP2
- samoczynne ep
- PZZ dla dodatkowego
*)

interface

uses mctools,sysutils,friction,hamulce;//,klasy_ham;

CONST
 dMAX=10;  //dysze
 dON = 0;  //osobowy napelnianie (+ZP)
 dOO = 1;  //osobowy oproznianie
 dTN = 2;  //towarowy napelnianie (+ZP)
 dTO = 3;  //towarowy oproznianie
 dP  = 4;  //zbiornik pomocniczy
 dSd = 5;  //zbiornik sterujacy
 dSm = 6;  //zbiornik sterujacy
 dPd = 7;  //duzy przelot zamykajcego
 dPm = 8;  //maly przelot zamykajacego
 dPO = 9;  //zasilanie pomocniczego O
 dPT =10;  //zasilanie pomocniczego T

//przekladniki
  p_none  = 0;
  p_rapid = 1;
  p_pp    = 2;
  p_al2   = 3;
  p_ppz   = 4;
  P_ed    = 5;



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

    TPrzeciwposlizg= class(TRura) //przy napelnianiu - rura, przy poslizgu - upust
      private
        Poslizg: boolean;
      public
        procedure SetPoslizg(flag: boolean);
        procedure Update(dt:real); override;
      end;

    TRapid= class(TPrzekladnik) //przekladnik dwustopniowy
      private
        RapidStatus: boolean; //status rapidu
        RapidMult: real;      //przelozenie (w dol)
//        Komora2: real;
        DN,DL: real;          //srednice dysz napelniania i luzowania
      public
        procedure SetRapidParams(mult: real; size: real);
        procedure SetRapidStatus(rs: boolean);
        procedure Update(dt:real); override;
      end;

    TPrzekCiagly= class(TPrzekladnik) //AL2
      private
        Mult: real;
      public
        procedure SetMult(m: real);
        procedure Update(dt:real); override;
      end;

    TPrzek_PZZ= class(TPrzekladnik) //podwojny zawor zwrotny
      private
        LBP: real;
      public
        procedure SetLBP(P: real);
        procedure Update(dt:real); override;
      end;

    TPrzekZalamany= class(TPrzekladnik) //Knicksventil
      private
      public
      end;

    TPrzekED= class(TRura) //przy napelnianiu - rura, przy hamowaniu - upust
      private
        MaxP: real;
      public
        procedure SetP(P: real);
        procedure Update(dt:real); override;
      end;

    TNESt3= class(TBrake)
      private
        Nozzles: array[0..dMAX] of real; //dysze
        CntrlRes: TReservoir;      //zbiornik steruj¹cy
        BVM: real;                 //przelozenie PG-CH
//        ValveFlag: byte;           //polozenie roznych zaworkow
        Zamykajacy: boolean;       //pamiec zaworka zamykajacego
//        Przys_wlot: boolean;       //wlot do komory przyspieszacza
        Przys_blok: boolean;       //blokada przyspieszacza
        Miedzypoj: TReservoir;     //pojemnosc posrednia (urojona) do napelniania ZP i ZS
        Przekladniki: array[1..3] of TPrzekladnik;
        RapidStatus: boolean;
        RapidStaly: boolean;
        LoadC: real;
        TareM, LoadM: real;       //masa proznego i pelnego
        TareBP: real;             //cisnienie dla proznego
        HBG300: real;             //zawor ograniczajacy cisnienie
        Podskok: real;            //podskok preznosci poczatkowej
//        HPBR: real;               //zasilanie ZP z wysokiego cisnienia
        autom: boolean;           //odluzniacz samoczynny
        LBP: real;                //cisnienie hamulca pomocniczego        
      public
        function GetPF(PP, dt, Vel: real): real; override;          //przeplyw miedzy komora wstepna i PG
        procedure EStParams(i_crc: real);                           //parametry charakterystyczne dla ESt
        procedure Init(PP, HPP, LPP, BP: real; BDF: byte); override;
        function GetCRP: real; override;
        procedure CheckState(BCP: real; var dV1: real);             //glowny przyrzad rozrzadczy
        procedure CheckReleaser(dt: real);                          //odluzniacz
        function CVs(bp: real): real;                               //napelniacz sterujacego
        function BVs(BCP: real): real;                              //napelniacz pomocniczego
        procedure SetSize(size: integer; params: string);           //ustawianie dysz (rozmiaru ZR), przekladniki
        procedure PLC(mass: real);                                  //wspolczynnik cisnienia przystawki wazacej
        procedure SetLP(TM, LM, TBP: real);                         //parametry przystawki wazacej
        procedure ForceEmptiness(); override;                       //wymuszenie bycia pustym
        procedure SetLBP(P: real);                                  //cisnienie z hamulca pomocniczego
      end;

function d2A(d: real):real;

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

// ------ PRZECIWPOSLIG ------

procedure TPrzeciwposlizg.SetPoslizg(flag: boolean);
begin
  Poslizg:=flag;
end;

procedure TPrzeciwposlizg.Update(dt: real);
begin
  if (Poslizg) then
   begin
    BrakeRes.Flow(dVol);
    Next.Flow(PF(Next.P,0,d2A(10))*dt);
   end
  else
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

procedure TRapid.SetRapidParams(mult: real; size: real);
begin
  RapidMult:=mult;
  RapidStatus:=false;
  if (size>0.1) then //dopasowywanie srednicy przekladnika
   begin
    DN:=D2A(size*0.4);
    DL:=D2A(size*0.4);
   end
  else
   begin
    DN:=D2A(5);
    DL:=D2A(5);
   end;
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
   dV:=-PFVd(BCP,0,DL,P*actMult/RapidMult)*dt
  else
  if(BCP*RapidMult<P*ActMult)then
   dV:=PFVa(BVP,BCP,DN,P*actMult/RapidMult)*dt
  else dV:=0;

  Next.Flow(dV);
  if dV>0 then
  BrakeRes.Flow(-dV);

end;

// ------ PRZEK£ADNIK CI¥G£Y ------

procedure TPrzekCiagly.SetMult(m:real);
begin
  Mult:=m;
end;

procedure TPrzekCiagly.Update(dt: real);
var BCP, BVP, dV: real;
begin
  BVP:=BrakeRes.P;
  BCP:=Next.P;

  if(BCP>P*Mult)then
   dV:=-PFVd(BCP,0,d2A(8),P*Mult)*dt
  else
  if(BCP<P*Mult)then
   dV:=PFVa(BVP,BCP,d2A(8),P*Mult)*dt
  else dV:=0;

  Next.Flow(dV);
  if dV>0 then
  BrakeRes.Flow(-dV);

end;

// ------ PRZEK£ADNIK CI¥G£Y ------

procedure TPrzek_PZZ.SetLBP(P:real);
begin
  LBP:=P;
end;

procedure TPrzek_PZZ.Update(dt: real);
var BCP, BVP, dV, Pgr: real;
begin
  BVP:=BrakeRes.P;
  BCP:=Next.P;

  Pgr:=Max0R(LBP,P);

  if(BCP>Pgr)then
   dV:=-PFVd(BCP,0,d2A(8),Pgr)*dt
  else
  if(BCP<Pgr)then
   dV:=PFVa(BVP,BCP,d2A(8),Pgr)*dt
  else dV:=0;

  Next.Flow(dV);
  if dV>0 then
  BrakeRes.Flow(-dV);

end;

// ------ PRZECIWPOSLIG ------

procedure TPrzekED.SetP(P: real);
begin
  MaxP:=P;
end;

procedure TPrzekED.Update(dt: real);
begin
  if Next.P>MaxP then
   begin
    BrakeRes.Flow(dVol);
    Next.Flow(PFVd(Next.P,0,d2A(10)*dt,MaxP));
   end
  else
    Next.Flow(dVol);
  dVol:=0;
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
  dV1:=0;

  nastG:=(BrakeDelayFlag and bdelay_G);

//sprawdzanie stanu
  CheckState(BCP, dV1);
  CheckReleaser(dt);

//luzowanie
  if(BrakeStatus and b_hld)=b_off then
   dV:=PF(0,BCP,Nozzles[dTO]*nastG+(1-nastG)*Nozzles[dOO])*dt*(0.1+4.9*Min0R(0.2,BCP-((CVP-0.05-VVP)*BVM+0.1)))
  else dV:=0;
//  BrakeCyl.Flow(-dV);
  Przekladniki[1].Flow(-dV);
  if((BrakeStatus and b_on)=b_on)and(Przekladniki[1].P*HBG300<MaxBP)then
   dV:=PF(BVP,BCP,Nozzles[dTN]*(nastG+2*Byte(BCP<Podskok))+Nozzles[dON]*(1-nastG))*dt*(0.1+4.9*Min0R(0.2,(CVP-0.05-VVP)*BVM-BCP))
  else dV:=0;
//  BrakeCyl.Flow(-dV);
  Przekladniki[1].Flow(-dV);
  BrakeRes.Flow(dV);

  for i:=1 to 3 do
   begin
    Przekladniki[i].Update(dt);
    if (Przekladniki[i] is TRapid) then
     begin
      RapidStatus:=(((BrakeDelayFlag and bdelay_R)=bdelay_R) and ((Abs(Vel)>70) or ((RapidStatus) and (Abs(Vel)>50)) or (RapidStaly)));
      (Przekladniki[i] as TRapid).SetRapidStatus(RapidStatus);
     end
    else
    if (Przekladniki[i] is TPrzeciwposlizg) then
      (Przekladniki[i] as TPrzeciwposlizg).SetPoslizg((BrakeStatus and b_asb)=b_asb)
    else
    if (Przekladniki[i] is TPrzekED) then
      if (Vel<-15) then
       (Przekladniki[i] as TPrzekED).SetP(0)
      else (Przekladniki[i] as TPrzekED).SetP(MaxBP*3)
    else
    if (Przekladniki[i] is TPrzekCiagly) then
      (Przekladniki[i] as TPrzekCiagly).SetMult(LoadC)
    else
    if (Przekladniki[i] is TPrzek_PZZ) then
      (Przekladniki[i] as TPrzek_PZZ).SetLBP(LBP);
   end;


//przeplyw testowy miedzypojemnosci
  dV:=PF(MPP,VVP,BVs(BCP))+PF(MPP,CVP,CVs(BCP));
  if(MPP-0.05>BVP)then
    dV:=dV+PF(MPP-0.05,BVP,Nozzles[dPT]*nastG+(1-nastG)*Nozzles[dPO]);
  if MPP>VVP then dV:=dV+PF(MPP,VVP,d2A(5));
  Miedzypoj.Flow(dV*dt*0.15);


//przeplyw ZS <-> PG
  temp:=CVs(BCP);
  dV:=PF(CVP,MPP,temp)*dt;
  CntrlRes.Flow(+dV);
  ValveRes.Flow(-0.02*dV);
  dV1:=dV1+0.98*dV;

//przeplyw ZP <-> MPJ
  if(MPP-0.05>BVP)then
   dV:=PF(BVP,MPP-0.05,Nozzles[dPT]*nastG+(1-nastG)*Nozzles[dPO])*dt
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
   BrakeStatus:=Byte(BP>1)*1;
   Miedzypoj:=TReservoir.Create;
   Miedzypoj.CreateCap(5);
   Miedzypoj.CreatePress(PP);

   BVM:=1/(HPP-0.05-LPP)*MaxBP;

   BrakeDelayFlag:=BDF;

   Zamykajacy:=false;

   if not ((FM is TDisk1) or (FM is TDisk2)) then   //jesli zeliwo to schodz
     RapidStaly:=false
   else
     RapidStaly:=true;

end;


function TNESt3.GetCRP: real;
begin
  GetCRP:=CntrlRes.P;
//  GetCRP:=Przekladniki[1].P;
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
   if(VVP+0.01+BCP/BVM<CVP-0.05)and(Przys_blok)then
     BrakeStatus:=(BrakeStatus or 3) //hamowanie stopniowe
   else if(VVP-0.01+(BCP-0.1)/BVM>CVP-0.05) then
     BrakeStatus:=(BrakeStatus and 252) //luzowanie
   else if(VVP+BCP/BVM>CVP-0.05) then
     BrakeStatus:=(BrakeStatus and 253) //zatrzymanie napelaniania
   else if(VVP+(BCP-0.1)/BVM<CVP-0.05)and(BCP>0.25)then //zatrzymanie luzowania
     BrakeStatus:=(BrakeStatus or 1);

 if (BrakeStatus and 1)=0 then
   SoundFlag:=SoundFlag or sf_CylU;

 if(VVP+0.10<CVP)and(BCP<0.25)then    //poczatek hamowania
   if (not Przys_blok) then
    begin
     ValveRes.CreatePress(0.1*VVP);
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
  begin
   CntrlRes.Flow(+PF(CVP,0,0.02)*dt);
   if(CVP<VVP+0.3)or(not autom)then
     BrakeStatus:=BrakeStatus and 247;
  end;
end;


function TNESt3.CVs(bp: real): real;      //napelniacz sterujacego
var CVP, MPP: real;
begin
  CVP:=CntrlRes.P;
  MPP:=Miedzypoj.P;

//przeplyw ZS <-> PG
  if(MPP<CVP-0.17)then
    CVS:=0
  else
  if(MPP>CVP-0.08)then
    CVs:=Nozzles[dSD]
  else
    CVs:=Nozzles[dSm];
end;


function TNESt3.BVs(BCP: real): real;     //napelniacz pomocniczego
var CVP, MPP: real;
begin
  CVP:=CntrlRes.P;
  MPP:=Miedzypoj.P;

//przeplyw ZP <-> rozdzielacz
  if(MPP<CVP-0.3)then
    BVs:=Nozzles[dP]
  else
    if(BCP<0.5) then
      if(Zamykajacy)then
        BVs:=Nozzles[dPm]  //1.25
      else
        BVs:=Nozzles[dPD]
    else
      BVs:=0;
end;

procedure TNESt3.PLC(mass: real);
begin
  LoadC:=1+Byte(Mass<LoadM)*((TareBP+(MaxBP-TareBP)*(mass-TareM)/(LoadM-TareM))/MaxBP-1);
end;

procedure TNESt3.ForceEmptiness();
begin
  ValveRes.CreatePress(0);
  BrakeRes.CreatePress(0);
  Miedzypoj.CreatePress(0);
  CntrlRes.CreatePress(0);

  BrakeStatus:=0;

  ValveRes.Act();
  BrakeRes.Act();
  Miedzypoj.Act();
  CntrlRes.Act();
end;


procedure TNESt3.SetLP(TM, LM, TBP: real);
begin
  TareM:=TM;
  LoadM:=LM;
  TareBP:=TBP;
end;


procedure TNESt3.SetLBP(P: real);
begin
  LBP:=P;
end;



procedure TNESt3.SetSize(size: integer; params: string);     //ustawianie dysz (rozmiaru ZR)
const
  dNO1l = 1.250;
  dNT1l = 0.510;
  dOO1l = 0.907;
  dOT1l = 0.524;
var
  i:integer;
begin

  if Pos('ESt3',params)>0 then
   begin
     Podskok:=0.7;
     Przekladniki[1]:=TRura.Create;
     Przekladniki[3]:=TRura.Create;
   end
  else
   begin
    Podskok:=-1;
    Przekladniki[1]:=TRapid.Create;
    if Pos('-s216',params)>0 then
      (Przekladniki[1] as TRapid).SetRapidParams(2,16)
    else
      (Przekladniki[1] as TRapid).SetRapidParams(2,0);
    Przekladniki[3]:=TPrzeciwposlizg.Create;
    if Pos('-ED',params)>0 then
     begin
      Przekladniki[3].Free();
      (Przekladniki[1] as TRapid).SetRapidParams(2,18);
      Przekladniki[3]:=TPrzekED.Create;
     end;
   end;

  if Pos('AL2',params)>0 then
    Przekladniki[2]:=TPrzekCiagly.Create
  else
  if Pos('PZZ',params)>0 then
   Przekladniki[2]:=TPrzek_PZZ.Create
  else
   Przekladniki[2]:=TRura.Create;

  if (Pos('3d',params)+Pos('4d',params)>0) then autom:=false else autom:=true;
  if (Pos('HBG300',params)>0) then HBG300:=1 else HBG300:=0;

  case size of
    16:
     begin   //dON,dOO,dTN,dTO,dP,dS
      Nozzles[dON]:=5.0;//5.0;
      Nozzles[dOO]:=3.4;//3.4;
      Nozzles[dTN]:=2.0;
      Nozzles[dTO]:=1.75;
      Nozzles[dP]:=3.8;
      Nozzles[dPd]:=2.70;
      Nozzles[dPm]:=1.25;
      Nozzles[dPO]:=Nozzles[dON];
      Nozzles[dPT]:=Nozzles[dTN];
     end;
    14:
     begin   //dON,dOO,dTN,dTO,dP,dS
      Nozzles[dON]:=4.3;
      Nozzles[dOO]:=2.85;
      Nozzles[dTN]:=1.83;
      Nozzles[dTO]:=1.57;
      Nozzles[dP]:=3.4;
      Nozzles[dPd]:=2.20;
      Nozzles[dPm]:=1.10;
      Nozzles[dPO]:=Nozzles[dON];
      Nozzles[dPT]:=Nozzles[dTN];
     end;
    12:
     begin   //dON,dOO,dTN,dTO,dP,dS
      Nozzles[dON]:=3.7;
      Nozzles[dOO]:=2.50;
      Nozzles[dTN]:=1.65;
      Nozzles[dTO]:=1.39;
      Nozzles[dP]:=2.65;
      Nozzles[dPd]:=1.80;
      Nozzles[dPm]:=0.85;
      Nozzles[dPO]:=Nozzles[dON];
      Nozzles[dPT]:=Nozzles[dTN];
     end;
    10:
     begin   //dON,dOO,dTN,dTO,dP,dS
      Nozzles[dON]:=3.1;
      Nozzles[dOO]:=2.0;
      Nozzles[dTN]:=1.35;
      Nozzles[dTO]:=1.13;
      Nozzles[dP]:=1.6;
      Nozzles[dPd]:=1.55;
      Nozzles[dPm]:=0.7;
      Nozzles[dPO]:=Nozzles[dON];
      Nozzles[dPT]:=Nozzles[dTN];
     end;
    200:
     begin   //dON,dOO,dTN,dTO,dP,dS
      Nozzles[dON]:=dNO1l;
      Nozzles[dOO]:=dOO1l/1.15;
      Nozzles[dTN]:=dNT1l;
      Nozzles[dTO]:=dOT1l;
      Nozzles[dP]:=7.4;
      Nozzles[dPd]:=5.3;
      Nozzles[dPm]:=2.5;
      Nozzles[dPO]:=7.28;
      Nozzles[dPT]:=2.96;
     end;
    375:
     begin   //dON,dOO,dTN,dTO,dP,dS
      Nozzles[dON]:=dNO1l;
      Nozzles[dOO]:=dOO1l/1.15;
      Nozzles[dTN]:=dNT1l;
      Nozzles[dTO]:=dOT1l;
      Nozzles[dP]:=13.0;
      Nozzles[dPd]:=9.6;
      Nozzles[dPm]:=4.4;
      Nozzles[dPO]:=9.92;
      Nozzles[dPT]:=3.99;
     end;
    150:
     begin   //dON,dOO,dTN,dTO,dP,dS
      Nozzles[dON]:=dNO1l;
      Nozzles[dOO]:=dOO1l;
      Nozzles[dTN]:=dNT1l;
      Nozzles[dTO]:=dOT1l;
      Nozzles[dP]:=5.8;
      Nozzles[dPd]:=4.1;
      Nozzles[dPm]:=1.9;
      Nozzles[dPO]:=6.33;
      Nozzles[dPT]:=2.58;
     end;
    100:
     begin   //dON,dOO,dTN,dTO,dP,dS
      Nozzles[dON]:=dNO1l;
      Nozzles[dOO]:=dOO1l;
      Nozzles[dTN]:=dNT1l;
      Nozzles[dTO]:=dOT1l;
      Nozzles[dP]:=4.2;
      Nozzles[dPd]:=2.9;
      Nozzles[dPm]:=1.4;
      Nozzles[dPO]:=5.19;
      Nozzles[dPT]:=2.14;
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

  for i:=1 to 3 do
   begin
    Przekladniki[i].BrakeRes:=@BrakeRes;
    Przekladniki[i].CreateCap(i);
    Przekladniki[i].CreatePress(BrakeCyl.P);
    if i<3 then
      Przekladniki[i].Next:=@Przekladniki[i+1]
    else
      Przekladniki[i].Next:=@BrakeCyl;
   end
end;

end.
