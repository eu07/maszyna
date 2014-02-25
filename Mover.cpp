//---------------------------------------------------------------------------

#include "Mover.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
// Ra: tu nale¿y przenosiæ funcje z mover.pas, które nie s¹ z niego wywo³ywane.
// Jeœli jakieœ zmienne nie s¹ u¿ywane w mover.pas, te¿ mo¿na je przenosiæ.
// Przeniesienie wszystkiego na raz zrobi³o by zbyt wielki chaos do ogarniêcia.

const dEpsilon=0.01; //1cm (zale¿y od typu sprzêgu...)

__fastcall TMoverParameters::TMoverParameters(double VelInitial,AnsiString TypeNameInit,AnsiString NameInit,
 int LoadInitial,AnsiString LoadTypeInitial,int Cab)
 : T_MoverParameters(VelInitial,TypeNameInit,NameInit,LoadInitial,LoadTypeInitial,Cab)
{//g³ówny konstruktor
 DimHalf.x=0.5*Dim.W; //po³owa szerokoœci, OX jest w bok?
 DimHalf.y=0.5*Dim.L; //po³owa d³ugoœci, OY jest do przodu?
 DimHalf.z=0.5*Dim.H; //po³owa wysokoœci, OZ jest w górê?
 BrakeLevelSet(-2); //Pascal ustawia na 0, przestawimy na odciêcie (CHK jest jeszcze nie wczytane!)
 bPantKurek3=true; //domyœlnie zbiornik pantografu po³¹czony jest ze zbiornikiem g³ównym
 iProblem=0; //pojazd w pe³ni gotowy do ruchu
 iLights[0]=iLights[1]=0; //œwiat³a zgaszone
};


double __fastcall TMoverParameters::Distance(const TLocation &Loc1,const TLocation &Loc2,const TDimension &Dim1,const TDimension &Dim2)
{//zwraca odleg³oœæ pomiêdzy pojazdami (Loc1) i (Loc2) z uwzglêdnieneim ich d³ugoœci (kule!)
 return hypot(Loc2.X-Loc1.X,Loc1.Y-Loc2.Y)-0.5*(Dim2.L+Dim1.L);
};

double __fastcall TMoverParameters::Distance(const vector3 &s1, const vector3 &s2, const vector3 &d1, const vector3 &d2)
{//obliczenie odleg³oœci prostopad³oœcianów o œrodkach (s1) i (s2) i wymiarach (d1) i (d2)
 //return 0.0; //bêdzie zg³aszaæ warning - funkcja do usuniêcia, chyba ¿e siê przyda...
};

double __fastcall TMoverParameters::CouplerDist(Byte Coupler)
{//obliczenie odleg³oœci pomiêdzy sprzêgami (kula!)
 return Couplers[Coupler].CoupleDist=Distance(Loc,Couplers[Coupler].Connected->Loc,Dim,Couplers[Coupler].Connected->Dim); //odleg³oœæ pomiêdzy sprzêgami (kula!)
};

bool __fastcall TMoverParameters::Attach(Byte ConnectNo,Byte ConnectToNr,TMoverParameters *ConnectTo,Byte CouplingType,bool Forced)
{//³¹czenie do swojego sprzêgu (ConnectNo) pojazdu (ConnectTo) stron¹ (ConnectToNr)
 //Ra: zwykle wykonywane dwukrotnie, dla ka¿dego pojazdu oddzielnie
 //Ra: trzeba by odró¿niæ wymóg dociœniêcia od uszkodzenia sprzêgu przy podczepianiu AI do sk³adu
 if (ConnectTo) //jeœli nie pusty
 {
  if (ConnectToNr!=2) Couplers[ConnectNo].ConnectedNr=ConnectToNr; //2=nic nie pod³¹czone
  TCouplerType ct=ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr].CouplerType; //typ sprzêgu pod³¹czanego pojazdu
  Couplers[ConnectNo].Connected=ConnectTo; //tak podpi¹æ (do siebie) zawsze mo¿na, najwy¿ej bêdzie wirtualny
  CouplerDist(ConnectNo); //przeliczenie odleg³oœci pomiêdzy sprzêgami
  if (CouplingType==ctrain_virtual) return false; //wirtualny wiêcej nic nie robi
  if (Forced?true:((Couplers[ConnectNo].CoupleDist<=dEpsilon)&&(Couplers[ConnectNo].CouplerType!=NoCoupler)&&(Couplers[ConnectNo].CouplerType==ct)))
  {//stykaja sie zderzaki i kompatybilne typy sprzegow, chyba ¿e ³¹czenie na starcie
   if (Couplers[ConnectNo].CouplingFlag==ctrain_virtual) //jeœli wczeœniej nie by³o po³¹czone
   {//ustalenie z której strony rysowaæ sprzêg
    Couplers[ConnectNo].Render=true; //tego rysowaæ
    ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr].Render=false; //a tego nie
   };
   Couplers[ConnectNo].CouplingFlag=CouplingType; //ustawienie typu sprzêgu
   //if (CouplingType!=ctrain_virtual) //Ra: wirtualnego nie ³¹czymy zwrotnie!
   //{//jeœli ³¹czenie sprzêgiem niewirtualnym, ustawiamy po³¹czenie zwrotne
   ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr].CouplingFlag=CouplingType;
   ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr].Connected=this;
   ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr].CoupleDist=Couplers[ConnectNo].CoupleDist;
   return true;
   //}
   //pod³¹czenie nie uda³o siê - jest wirtualne
  }
 }
 return false; //brak pod³¹czanego pojazdu, zbyt du¿a odleg³oœæ, niezgodny typ sprzêgu, brak sprzêgu, brak haka
};

bool __fastcall TMoverParameters::Attach(Byte ConnectNo,Byte ConnectToNr,T_MoverParameters *ConnectTo,Byte CouplingType,bool Forced)
{//³¹czenie do (ConnectNo) pojazdu (ConnectTo) stron¹ (ConnectToNr)
 return Attach(ConnectNo,ConnectToNr,(TMoverParameters*)ConnectTo,CouplingType,Forced);
};

int __fastcall TMoverParameters::DettachStatus(Byte ConnectNo)
{//Ra: sprawdzenie, czy odleg³oœæ jest dobra do roz³¹czania
 //powinny byæ 3 informacje: =0 sprzêg ju¿ roz³¹czony, <0 da siê roz³¹czyæ. >0 nie da siê roz³¹czyæ
 if (!Couplers[ConnectNo].Connected)
  return 0; //nie ma nic, to roz³¹czanie jest OK
 if ((Couplers[ConnectNo].CouplingFlag&ctrain_coupler)==0)
  return -Couplers[ConnectNo].CouplingFlag; //hak nie po³¹czony - roz³¹czanie jest OK
 if (TestFlag(DamageFlag,dtrain_coupling))
  return -Couplers[ConnectNo].CouplingFlag; //hak urwany - roz³¹czanie jest OK
 //ABu021104: zakomentowane 'and (CouplerType<>Articulated)' w warunku, nie wiem co to bylo, ale za to teraz dziala odczepianie... :) }
 //if (CouplerType==Articulated) return false; //sprzêg nie do rozpiêcia - mo¿e byæ tylko urwany
 //Couplers[ConnectNo].CoupleDist=Distance(Loc,Couplers[ConnectNo].Connected->Loc,Dim,Couplers[ConnectNo].Connected->Dim);
 CouplerDist(ConnectNo);
 if (Couplers[ConnectNo].CouplerType==Screw?Couplers[ConnectNo].CoupleDist<0.0:true)
  return -Couplers[ConnectNo].CouplingFlag; //mo¿na roz³¹czaæ, jeœli dociœniêty
 return (Couplers[ConnectNo].CoupleDist>0.2)?-Couplers[ConnectNo].CouplingFlag:Couplers[ConnectNo].CouplingFlag;
};

bool __fastcall TMoverParameters::Dettach(Byte ConnectNo)
{//rozlaczanie
 if (!Couplers[ConnectNo].Connected) return true; //nie ma nic, to odczepiono
 //with Couplers[ConnectNo] do
 int i=DettachStatus(ConnectNo); //stan sprzêgu
 if (i<0)
 {//gdy scisniete zderzaki, chyba ze zerwany sprzeg (wirtualnego nie odpinamy z drugiej strony)
  //Couplers[ConnectNo].Connected=NULL; //lepiej zostawic bo przeciez trzeba kontrolowac zderzenia odczepionych
  Couplers[ConnectNo].Connected->Couplers[Couplers[ConnectNo].ConnectedNr].CouplingFlag=0; //pozostaje sprzêg wirtualny
  Couplers[ConnectNo].CouplingFlag=0; //pozostaje sprzêg wirtualny
  return true;
 }
 else if (i>0)
 {//od³¹czamy wê¿e i resztê, pozostaje sprzêg fizyczny, który wymaga dociœniêcia (z wirtualnym nic)
  Couplers[ConnectNo].CouplingFlag&=ctrain_coupler;
  Couplers[ConnectNo].Connected->Couplers[Couplers[ConnectNo].ConnectedNr].CouplingFlag=Couplers[ConnectNo].CouplingFlag;
 }
 return false; //jeszcze nie roz³¹czony
};

void __fastcall TMoverParameters::SetCoupleDist()
{//przeliczenie odleg³oœci sprzêgów
 if (Couplers[0].Connected)
 {
  CouplerDist(0);
  if (CategoryFlag&2)
  {//Ra: dla samochodów zderzanie kul to za ma³o
  }
 }
 if (Couplers[1].Connected)
 {
  CouplerDist(1);
  if (CategoryFlag&2)
  {//Ra: dla samochodów zderzanie kul to za ma³o
  }
 }
};

bool __fastcall TMoverParameters::DirectionForward()
{
 if ((MainCtrlPosNo>0)&&(ActiveDir<1)&&(MainCtrlPos==0))
 {
  ++ActiveDir;
  DirAbsolute=ActiveDir*CabNo;
  SendCtrlToNext("Direction",ActiveDir,CabNo);
  return true;
 }
 else if ((ActiveDir==1)&&(MainCtrlPos==0)&&(TrainType==dt_EZT))
  return MinCurrentSwitch(true); //"wysoki rozruch" EN57
 return false;
};

// Nastawianie hamulców

void __fastcall TMoverParameters::BrakeLevelSet(double b)
{//ustawienie pozycji hamulca na wartoœæ (b) w zakresie od -2 do BrakeCtrlPosNo
 //jedyny dopuszczalny sposób przestawienia hamulca zasadniczego
 if (fBrakeCtrlPos==b) return; //nie przeliczaæ, jak nie ma zmiany
 fBrakeCtrlPos=b;
 BrakeCtrlPosR=b;
 if (fBrakeCtrlPos<-2.0)
  fBrakeCtrlPos=-2.0; //odciêcie
 else
  if (fBrakeCtrlPos>double(BrakeCtrlPosNo))
   fBrakeCtrlPos=BrakeCtrlPosNo;
 int x=floor(fBrakeCtrlPos); //jeœli odwo³ujemy siê do BrakeCtrlPos w poœrednich, to musi byæ obciête a nie zaokr¹gone
 while ((x>BrakeCtrlPos)&&(BrakeCtrlPos<BrakeCtrlPosNo)) //jeœli zwiêkszy³o siê o 1
  if (!T_MoverParameters::IncBrakeLevelOld()) break; //wyjœcie awaryjne
 while ((x<BrakeCtrlPos)&&(BrakeCtrlPos>=-1)) //jeœli zmniejszy³o siê o 1
  if (!T_MoverParameters::DecBrakeLevelOld()) break;
 BrakePressureActual=BrakePressureTable[BrakeCtrlPos+2]; //skopiowanie pozycji
// if (BrakeSystem==Pneumatic?BrakeSubsystem==Oerlikon:false) //tylko Oerlikon akceptuje u³amki     //youBy: obawiam sie, ze tutaj to nie dziala :P
 if(false)
  if (fBrakeCtrlPos>0.0)
  {//wartoœci poœrednie wyliczamy tylko dla hamowania
   double u=fBrakeCtrlPos-double(x); //u³amek ponad wartoœæ ca³kowit¹
   if (u>0.0)
   {//wyliczamy wartoœci wa¿one
    BrakePressureActual.PipePressureVal+=-u*BrakePressureActual.PipePressureVal+u*BrakePressureTable[BrakeCtrlPos+1+2].PipePressureVal;
    //BrakePressureActual.BrakePressureVal+=-u*BrakePressureActual.BrakePressureVal+u*BrakePressureTable[BrakeCtrlPos+1].BrakePressureVal;  //to chyba nie bêdzie tak dzia³aæ, zw³aszcza w EN57
    BrakePressureActual.FlowSpeedVal+=-u*BrakePressureActual.FlowSpeedVal+u*BrakePressureTable[BrakeCtrlPos+1+2].FlowSpeedVal;
   }
  }
};

bool __fastcall TMoverParameters::BrakeLevelAdd(double b)
{//dodanie wartoœci (b) do pozycji hamulca (w tym ujemnej)
 //zwraca false, gdy po dodaniu by³o by poza zakresem
 BrakeLevelSet(fBrakeCtrlPos+b);
 return b>0.0?(fBrakeCtrlPos<BrakeCtrlPosNo):(BrakeCtrlPos>-1.0); //true, jeœli mo¿na kontynuowaæ
};

bool __fastcall TMoverParameters::IncBrakeLevel()
{//nowa wersja na u¿ytek AI, false gdy osi¹gniêto pozycjê BrakeCtrlPosNo
 return BrakeLevelAdd(1.0);
};

bool __fastcall TMoverParameters::DecBrakeLevel()
{//nowa wersja na u¿ytek AI, false gdy osi¹gniêto pozycjê -1
 return BrakeLevelAdd(-1.0);
};

bool __fastcall TMoverParameters::ChangeCab(int direction)
{//zmiana kabiny i resetowanie ustawien
 if (abs(ActiveCab+direction)<2)
 {
//  if (ActiveCab+direction=0) then LastCab:=ActiveCab;
   ActiveCab=ActiveCab+direction;
   if ((BrakeSystem==Pneumatic)&&(BrakeCtrlPosNo>0))
   {
    if (BrakeHandle==FV4a)   //!!!POBIERAÆ WARTOŒÆ Z KLASY ZAWORU!!!
     BrakeLevelSet(-2); //BrakeCtrlPos=-2;
    else if ((BrakeHandle==FVel6)||(BrakeHandle==St113))
     BrakeLevelSet(2);
    else
     BrakeLevelSet(1);
    LimPipePress=PipePress;
    ActFlowSpeed=0;
   }
   else
    //if (TrainType=dt_EZT) and (BrakeCtrlPosNo>0) then
    //  BrakeCtrlPos:=5; //z Megapacka
    //else
    BrakeLevelSet(0); //BrakeCtrlPos=0;
//   if not TestFlag(BrakeStatus,b_dmg) then
//    BrakeStatus:=b_off; //z Megapacka
   MainCtrlPos=0;
   ScndCtrlPos=0;
   //Ra: to poni¿ej jest bez sensu - mo¿na przejœæ nie wy³¹czaj¹c
   //if ((EngineType!=DieselEngine)&&(EngineType!=DieselElectric))
   //{
   // Mains=false;
   // CompressorAllow=false;
   // ConverterAllow=false;
   //}
  //ActiveDir=0;
  //DirAbsolute=0;
  return true;
 }
 return false;
};

bool __fastcall TMoverParameters::CurrentSwitch(int direction)
{//rozruch wysoki (true) albo niski (false)
 //Ra: przenios³em z Train.cpp, nie wiem czy ma to sens
 if (MaxCurrentSwitch(direction))
 {if (TrainType!=dt_EZT)
   return (MinCurrentSwitch(direction));
 }
 return false;
};

void __fastcall TMoverParameters::UpdatePantVolume(double dt)
{//KURS90 - sprê¿arka pantografów
 if ((TrainType==dt_EZT)?(PantPress<ScndPipePress):bPantKurek3) //kurek zamyka po³¹czenie z ZG
 {//zbiornik pantografu po³¹czony ze zbiornikiem g³ównym - ma³¹ sprê¿ark¹ siê tego nie napompuje
  //Ra 2013-12: Niebugoc³aw mówi, ¿e w EZT nie ma potrzeby odcinaæ kurkiem
  PantPress=ScndPipePress;
  PantVolume=(ScndPipePress+1)*0.1; //objêtoœæ, na wypadek odciêcia kurkiem
 }
 else
 {//zbiornik g³ówny odciêty, mo¿na pompowaæ pantografy
  if (PantCompFlag&&Battery) //w³¹czona bateria i ma³a sprê¿arka
   PantVolume+=dt*(TrainType==dt_EZT?0.003:0.005)*(2*0.45-((0.1/PantVolume/10)-0.1))/0.45; //nape³nianie zbiornika pantografów
  //Ra 2013-12: Niebugoc³aw mówi, ¿e w EZT nabija 1.5 raz wolniej ni¿ jak by³o 0.005
  PantPress=(10*PantVolume)-1; //tu by siê przyda³a objêtoœæ zbiornika
 }
 if (!PantCompFlag&&(PantVolume>0.1))
  PantVolume-=dt*0.0003; //nieszczelnoœci: 0.0003=0.3l/s
 if (Mains) //nie wchodziæ w funkcjê bez potrzeby
  if (EngineType==ElectricSeriesMotor) //nie dotyczy... czego w³aœciwie?
   if (PantPress<EnginePowerSource.CollectorParameters.MinPress)
    if ((TrainType&(dt_EZT|dt_ET40|dt_ET41|dt_ET42))?(GetTrainsetVoltage()<EnginePowerSource.CollectorParameters.MinV):true) //to jest trochê proteza; zasilanie cz³onu mo¿e byæ przez sprzêg WN
     if (MainSwitch(false))
      EventFlag=true; //wywalenie szybkiego z powodu niskiego ciœnienia
 if (TrainType!=dt_EZT) //w EN57 pompuje siê tylko w silnikowym
 //pierwotnie w CHK pantografy mia³y równie¿ rozrz¹dcze EZT 
 for (int b=0;b<=1;++b)
  if (TestFlag(Couplers[b].CouplingFlag,ctrain_controll))
   if (Couplers[b].Connected->PantVolume<PantVolume) //bo inaczej trzeba w obydwu cz³onach przestawiaæ
    Couplers[b].Connected->PantVolume=PantVolume; //przekazanie ciœnienia do s¹siedniego cz³onu
 //czy np. w ET40, ET41, ET42 pantografy cz³onów maj¹ po³¹czenie pneumatyczne?
};

void __fastcall TMoverParameters::UpdateBatteryVoltage(double dt)
{//przeliczenie obci¹¿enia baterii
 double sn1,sn2,sn3,sn4,sn5; //Ra: zrobiæ z tego amperomierz NN
 if ((BatteryVoltage>0)&&(EngineType!=DieselEngine)&&(EngineType!=WheelsDriven)&&(NominalBatteryVoltage>0))
 {
  if ((NominalBatteryVoltage/BatteryVoltage<1.22)&&Battery)
  {//110V
   if (!ConverterFlag)
    sn1=(dt*10); //szybki spadek do ok 90V
   else sn1=0;
   if (ConverterFlag)
    sn2=-(dt*10); //szybki wzrost do 110V
   else sn2=0;
   if (Mains)
    sn3=(dt*0.05);
   else sn3=0;
   if (iLights[0]&63) //64=blachy, nie ci¹gn¹ pr¹du //rozpisaæ na poszczególne ¿arówki...
    sn4=dt*0.003;
   else sn4=0;
   if (iLights[1]&63) //64=blachy, nie ci¹gn¹ pr¹du
    sn5=dt*0.001;
   else sn5=0;
  };
  if ((NominalBatteryVoltage/BatteryVoltage>=1.22)&&Battery)
  {//90V
   if (PantCompFlag)
    sn1=(dt*0.0046);
   else sn1=0;
   if (ConverterFlag)
    sn2=-(dt*50); //szybki wzrost do 110V
   else sn2=0;
   if (Mains)
    sn3=(dt*0.001);
   else sn3=0;
   if (iLights[0]&63) //64=blachy, nie ci¹gn¹ pr¹du
    sn4=(dt*0.0030);
   else sn4=0;
   if (iLights[1]&63) //64=blachy, nie ci¹gn¹ pr¹du
    sn5=(dt*0.0010);
   else sn5=0;
  };
  if (!Battery)
  {
   if (NominalBatteryVoltage/BatteryVoltage<1.22)
    sn1=dt*50;
   else
    sn1=0;
    sn2=dt*0.000001;
    sn3=dt*0.000001;
    sn4=dt*0.000001;
    sn5=dt*0.000001; //bardzo powolny spadek przy wy³¹czonych bateriach
  };
  BatteryVoltage-=(sn1+sn2+sn3+sn4+sn5);
  if (NominalBatteryVoltage/BatteryVoltage>1.57)
    if (MainSwitch(false)&&(EngineType!=DieselEngine)&&(EngineType!=WheelsDriven))
      EventFlag=true; //wywalanie szybkiego z powodu zbyt niskiego napiecia
  if (BatteryVoltage>NominalBatteryVoltage)
   BatteryVoltage=NominalBatteryVoltage; //wstrzymanie ³adowania pow. 110V
  if (BatteryVoltage<0.01)
   BatteryVoltage=0.01;
 }
 else
  if (NominalBatteryVoltage==0)
   BatteryVoltage=0;
  else
   BatteryVoltage=90;
};

/* Ukrotnienie EN57:
1 //uk³ad szeregowy
2 //uk³ad równoleg³y
3 //bocznik 1
4 //bocznik 2
5 //bocznik 3
6 //do przodu
7 //do ty³u
8 //1 przyspieszenie
9 //minus obw. 2 przyspieszenia
10 //jazda na oporach
11 //SHP
12A //podnoszenie pantografu przedniego
12B //podnoszenie pantografu tylnego
13A //opuszczanie pantografu przedniego
13B //opuszczanie wszystkich pantografów
14 //za³¹czenie WS
15 //rozrz¹d (WS, PSR, wa³ ku³akowy)
16 //odblok PN
18 //sygnalizacja przetwornicy g³ównej
19 //luzowanie EP
20 //hamowanie EP
21 //rezerwa** (1900+: zamykanie drzwi prawych)
22 //za³. przetwornicy g³ównej
23 //wy³. przetwornicy g³ównej
24 //za³. przetw. oœwietlenia
25 //wy³. przetwornicy oœwietlenia
26 //sygnalizacja WS
28 //sprê¿arka
29 //ogrzewanie
30 //rezerwa* (1900+: zamykanie drzwi lewych)
31 //otwieranie drzwi prawych
32H //zadzia³anie PN siln. trakcyjnych
33 //sygna³ odjazdu
34 //rezerwa (sygnalizacja poœlizgu)
35 //otwieranie drzwi lewych
ZN //masa
*/

double __fastcall TMoverParameters::ComputeMovement
(double dt,double dt1,
 const TTrackShape &Shape,TTrackParam &Track,TTractionParam &ElectricTraction,
 const TLocation &NewLoc,TRotation &NewRot
)
{//trzeba po ma³u przenosiæ tu tê funkcjê
 double d=T_MoverParameters::ComputeMovement(dt,dt1,Shape,Track,ElectricTraction,NewLoc,NewRot);
 if (EnginePowerSource.SourceType==CurrentCollector) //tylko jeœli pantografuj¹cy
  if (Power>1.0) //w rozrz¹dczym nie (jest b³¹d w FIZ!)
   UpdatePantVolume(dt); //Ra: pneumatyka pantografów przeniesiona do Mover.cpp!
 //sprawdzanie i ewentualnie wykonywanie->kasowanie poleceñ
 if (LoadStatus>0) //czas doliczamy tylko jeœli trwa (roz)³adowanie
  LastLoadChangeTime=LastLoadChangeTime+dt; //czas (roz)³adunku
 RunInternalCommand();
 //automatyczny rozruch
 if (EngineType==ElectricSeriesMotor)
  if (AutoRelayCheck())
   SetFlag(SoundFlag,sound_relay);
/*
 else                  {McZapkie-041003: aby slychac bylo przelaczniki w sterowniczym}
  if (EngineType=None) and (MainCtrlPosNo>0) then
   for b:=0 to 1 do
    with Couplers[b] do
     if TestFlag(CouplingFlag,ctrain_controll) then
      if Connected^.Power>0.01 then
       SoundFlag:=SoundFlag or Connected^.SoundFlag;
*/
 if (EngineType==DieselEngine)
  if (dizel_Update(dt))
    SetFlag(SoundFlag,sound_relay);
 //uklady hamulcowe:
  if (VeselVolume>0)
   Compressor=CompressedVolume/VeselVolume;
  else
  {
   Compressor=0;
   CompressorFlag=false;
  };
 ConverterCheck();
 if (CompressorSpeed>0.0) //sprê¿arka musi mieæ jak¹œ niezerow¹ wydajnoœæ
  CompressorCheck(dt); //¿eby rozwa¿aæ jej za³¹czenie i pracê
 UpdateBrakePressure(dt);
 UpdatePipePressure(dt);
 UpdateBatteryVoltage(dt);
 UpdateScndPipePressure(dt); // druga rurka, youBy
 //hamulec antypoœlizgowy - wy³¹czanie
 if ((BrakeSlippingTimer>0.8)&&(ASBType!=128)) //ASBSpeed=0.8
  Hamulec->ASB(0);
 //SetFlag(BrakeStatus,-b_antislip);
 BrakeSlippingTimer=BrakeSlippingTimer+dt;
 //sypanie piasku - wy³¹czone i piasek siê nie koñczy - b³êdy AI
 //if AIControllFlag then
 // if SandDose then
 //  if Sand>0 then
 //   begin
 //     Sand:=Sand-NPoweredAxles*SandSpeed*dt;
 //     if Random<dt then SandDose:=false;
 //   end
 //  else
 //   begin
 //     SandDose:=false;
 //     Sand:=0;
 //   end;
 //czuwak/SHP
 //if (Vel>10) and (not DebugmodeFlag) then
 if (!DebugModeFlag)
  SecuritySystemCheck(dt1);
 return d;
};

double __fastcall TMoverParameters::FastComputeMovement
(double dt,
 const TTrackShape &Shape,TTrackParam &Track,
 const TLocation &NewLoc,TRotation &NewRot
)
{//trzeba po ma³u przenosiæ tu tê funkcjê
 double d=T_MoverParameters::FastComputeMovement(dt,Shape,Track,NewLoc,NewRot);
 if (EnginePowerSource.SourceType==CurrentCollector) //tylko jeœli pantografuj¹cy
  if (Power>1.0) //w rozrz¹dczym nie (jest b³¹d w FIZ!)
   UpdatePantVolume(dt); //Ra: pneumatyka pantografów przeniesiona do Mover.cpp!
 //sprawdzanie i ewentualnie wykonywanie->kasowanie poleceñ
 if (LoadStatus>0) //czas doliczamy tylko jeœli trwa (roz)³adowanie
  LastLoadChangeTime=LastLoadChangeTime+dt; //czas (roz)³adunku
 RunInternalCommand();
 if (EngineType==DieselEngine)
  if (dizel_Update(dt))
   SetFlag(SoundFlag,sound_relay);
 //uklady hamulcowe:
 if (VeselVolume>0)
  Compressor=CompressedVolume/VeselVolume;
 else
 {
  Compressor=0;
  CompressorFlag=false;
 };
 ConverterCheck();
 if (CompressorSpeed>0.0) //sprê¿arka musi mieæ jak¹œ niezerow¹ wydajnoœæ
  CompressorCheck(dt); //¿eby rozwa¿aæ jej za³¹czenie i pracê
 UpdateBrakePressure(dt);
 UpdatePipePressure(dt);
 UpdateScndPipePressure(dt); // druga rurka, youBy
 UpdateBatteryVoltage(dt);
 //hamulec antyposlizgowy - wy³¹czanie
 if ((BrakeSlippingTimer>0.8)&&(ASBType!=128)) //ASBSpeed=0.8
  Hamulec->ASB(0);
 BrakeSlippingTimer=BrakeSlippingTimer+dt;
 return d;
};
