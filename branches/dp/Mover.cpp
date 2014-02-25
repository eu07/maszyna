//---------------------------------------------------------------------------

#include "Mover.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
// Ra: tu nale�y przenosi� funcje z mover.pas, kt�re nie s� z niego wywo�ywane.
// Je�li jakie� zmienne nie s� u�ywane w mover.pas, te� mo�na je przenosi�.
// Przeniesienie wszystkiego na raz zrobi�o by zbyt wielki chaos do ogarni�cia.

const dEpsilon=0.01; //1cm (zale�y od typu sprz�gu...)

__fastcall TMoverParameters::TMoverParameters(double VelInitial,AnsiString TypeNameInit,AnsiString NameInit,
 int LoadInitial,AnsiString LoadTypeInitial,int Cab)
 : T_MoverParameters(VelInitial,TypeNameInit,NameInit,LoadInitial,LoadTypeInitial,Cab)
{//g��wny konstruktor
 DimHalf.x=0.5*Dim.W; //po�owa szeroko�ci, OX jest w bok?
 DimHalf.y=0.5*Dim.L; //po�owa d�ugo�ci, OY jest do przodu?
 DimHalf.z=0.5*Dim.H; //po�owa wysoko�ci, OZ jest w g�r�?
 BrakeLevelSet(-2); //Pascal ustawia na 0, przestawimy na odci�cie (CHK jest jeszcze nie wczytane!)
 bPantKurek3=true; //domy�lnie zbiornik pantografu po��czony jest ze zbiornikiem g��wnym
 iProblem=0; //pojazd w pe�ni gotowy do ruchu
 iLights[0]=iLights[1]=0; //�wiat�a zgaszone
};


double __fastcall TMoverParameters::Distance(const TLocation &Loc1,const TLocation &Loc2,const TDimension &Dim1,const TDimension &Dim2)
{//zwraca odleg�o�� pomi�dzy pojazdami (Loc1) i (Loc2) z uwzgl�dnieneim ich d�ugo�ci (kule!)
 return hypot(Loc2.X-Loc1.X,Loc1.Y-Loc2.Y)-0.5*(Dim2.L+Dim1.L);
};

double __fastcall TMoverParameters::Distance(const vector3 &s1, const vector3 &s2, const vector3 &d1, const vector3 &d2)
{//obliczenie odleg�o�ci prostopad�o�cian�w o �rodkach (s1) i (s2) i wymiarach (d1) i (d2)
 //return 0.0; //b�dzie zg�asza� warning - funkcja do usuni�cia, chyba �e si� przyda...
};

double __fastcall TMoverParameters::CouplerDist(Byte Coupler)
{//obliczenie odleg�o�ci pomi�dzy sprz�gami (kula!)
 return Couplers[Coupler].CoupleDist=Distance(Loc,Couplers[Coupler].Connected->Loc,Dim,Couplers[Coupler].Connected->Dim); //odleg�o�� pomi�dzy sprz�gami (kula!)
};

bool __fastcall TMoverParameters::Attach(Byte ConnectNo,Byte ConnectToNr,TMoverParameters *ConnectTo,Byte CouplingType,bool Forced)
{//��czenie do swojego sprz�gu (ConnectNo) pojazdu (ConnectTo) stron� (ConnectToNr)
 //Ra: zwykle wykonywane dwukrotnie, dla ka�dego pojazdu oddzielnie
 //Ra: trzeba by odr�ni� wym�g doci�ni�cia od uszkodzenia sprz�gu przy podczepianiu AI do sk�adu
 if (ConnectTo) //je�li nie pusty
 {
  if (ConnectToNr!=2) Couplers[ConnectNo].ConnectedNr=ConnectToNr; //2=nic nie pod��czone
  TCouplerType ct=ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr].CouplerType; //typ sprz�gu pod��czanego pojazdu
  Couplers[ConnectNo].Connected=ConnectTo; //tak podpi�� (do siebie) zawsze mo�na, najwy�ej b�dzie wirtualny
  CouplerDist(ConnectNo); //przeliczenie odleg�o�ci pomi�dzy sprz�gami
  if (CouplingType==ctrain_virtual) return false; //wirtualny wi�cej nic nie robi
  if (Forced?true:((Couplers[ConnectNo].CoupleDist<=dEpsilon)&&(Couplers[ConnectNo].CouplerType!=NoCoupler)&&(Couplers[ConnectNo].CouplerType==ct)))
  {//stykaja sie zderzaki i kompatybilne typy sprzegow, chyba �e ��czenie na starcie
   if (Couplers[ConnectNo].CouplingFlag==ctrain_virtual) //je�li wcze�niej nie by�o po��czone
   {//ustalenie z kt�rej strony rysowa� sprz�g
    Couplers[ConnectNo].Render=true; //tego rysowa�
    ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr].Render=false; //a tego nie
   };
   Couplers[ConnectNo].CouplingFlag=CouplingType; //ustawienie typu sprz�gu
   //if (CouplingType!=ctrain_virtual) //Ra: wirtualnego nie ��czymy zwrotnie!
   //{//je�li ��czenie sprz�giem niewirtualnym, ustawiamy po��czenie zwrotne
   ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr].CouplingFlag=CouplingType;
   ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr].Connected=this;
   ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr].CoupleDist=Couplers[ConnectNo].CoupleDist;
   return true;
   //}
   //pod��czenie nie uda�o si� - jest wirtualne
  }
 }
 return false; //brak pod��czanego pojazdu, zbyt du�a odleg�o��, niezgodny typ sprz�gu, brak sprz�gu, brak haka
};

bool __fastcall TMoverParameters::Attach(Byte ConnectNo,Byte ConnectToNr,T_MoverParameters *ConnectTo,Byte CouplingType,bool Forced)
{//��czenie do (ConnectNo) pojazdu (ConnectTo) stron� (ConnectToNr)
 return Attach(ConnectNo,ConnectToNr,(TMoverParameters*)ConnectTo,CouplingType,Forced);
};

int __fastcall TMoverParameters::DettachStatus(Byte ConnectNo)
{//Ra: sprawdzenie, czy odleg�o�� jest dobra do roz��czania
 //powinny by� 3 informacje: =0 sprz�g ju� roz��czony, <0 da si� roz��czy�. >0 nie da si� roz��czy�
 if (!Couplers[ConnectNo].Connected)
  return 0; //nie ma nic, to roz��czanie jest OK
 if ((Couplers[ConnectNo].CouplingFlag&ctrain_coupler)==0)
  return -Couplers[ConnectNo].CouplingFlag; //hak nie po��czony - roz��czanie jest OK
 if (TestFlag(DamageFlag,dtrain_coupling))
  return -Couplers[ConnectNo].CouplingFlag; //hak urwany - roz��czanie jest OK
 //ABu021104: zakomentowane 'and (CouplerType<>Articulated)' w warunku, nie wiem co to bylo, ale za to teraz dziala odczepianie... :) }
 //if (CouplerType==Articulated) return false; //sprz�g nie do rozpi�cia - mo�e by� tylko urwany
 //Couplers[ConnectNo].CoupleDist=Distance(Loc,Couplers[ConnectNo].Connected->Loc,Dim,Couplers[ConnectNo].Connected->Dim);
 CouplerDist(ConnectNo);
 if (Couplers[ConnectNo].CouplerType==Screw?Couplers[ConnectNo].CoupleDist<0.0:true)
  return -Couplers[ConnectNo].CouplingFlag; //mo�na roz��cza�, je�li doci�ni�ty
 return (Couplers[ConnectNo].CoupleDist>0.2)?-Couplers[ConnectNo].CouplingFlag:Couplers[ConnectNo].CouplingFlag;
};

bool __fastcall TMoverParameters::Dettach(Byte ConnectNo)
{//rozlaczanie
 if (!Couplers[ConnectNo].Connected) return true; //nie ma nic, to odczepiono
 //with Couplers[ConnectNo] do
 int i=DettachStatus(ConnectNo); //stan sprz�gu
 if (i<0)
 {//gdy scisniete zderzaki, chyba ze zerwany sprzeg (wirtualnego nie odpinamy z drugiej strony)
  //Couplers[ConnectNo].Connected=NULL; //lepiej zostawic bo przeciez trzeba kontrolowac zderzenia odczepionych
  Couplers[ConnectNo].Connected->Couplers[Couplers[ConnectNo].ConnectedNr].CouplingFlag=0; //pozostaje sprz�g wirtualny
  Couplers[ConnectNo].CouplingFlag=0; //pozostaje sprz�g wirtualny
  return true;
 }
 else if (i>0)
 {//od��czamy w�e i reszt�, pozostaje sprz�g fizyczny, kt�ry wymaga doci�ni�cia (z wirtualnym nic)
  Couplers[ConnectNo].CouplingFlag&=ctrain_coupler;
  Couplers[ConnectNo].Connected->Couplers[Couplers[ConnectNo].ConnectedNr].CouplingFlag=Couplers[ConnectNo].CouplingFlag;
 }
 return false; //jeszcze nie roz��czony
};

void __fastcall TMoverParameters::SetCoupleDist()
{//przeliczenie odleg�o�ci sprz�g�w
 if (Couplers[0].Connected)
 {
  CouplerDist(0);
  if (CategoryFlag&2)
  {//Ra: dla samochod�w zderzanie kul to za ma�o
  }
 }
 if (Couplers[1].Connected)
 {
  CouplerDist(1);
  if (CategoryFlag&2)
  {//Ra: dla samochod�w zderzanie kul to za ma�o
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

// Nastawianie hamulc�w

void __fastcall TMoverParameters::BrakeLevelSet(double b)
{//ustawienie pozycji hamulca na warto�� (b) w zakresie od -2 do BrakeCtrlPosNo
 //jedyny dopuszczalny spos�b przestawienia hamulca zasadniczego
 if (fBrakeCtrlPos==b) return; //nie przelicza�, jak nie ma zmiany
 fBrakeCtrlPos=b;
 BrakeCtrlPosR=b;
 if (fBrakeCtrlPos<-2.0)
  fBrakeCtrlPos=-2.0; //odci�cie
 else
  if (fBrakeCtrlPos>double(BrakeCtrlPosNo))
   fBrakeCtrlPos=BrakeCtrlPosNo;
 int x=floor(fBrakeCtrlPos); //je�li odwo�ujemy si� do BrakeCtrlPos w po�rednich, to musi by� obci�te a nie zaokr�gone
 while ((x>BrakeCtrlPos)&&(BrakeCtrlPos<BrakeCtrlPosNo)) //je�li zwi�kszy�o si� o 1
  if (!T_MoverParameters::IncBrakeLevelOld()) break; //wyj�cie awaryjne
 while ((x<BrakeCtrlPos)&&(BrakeCtrlPos>=-1)) //je�li zmniejszy�o si� o 1
  if (!T_MoverParameters::DecBrakeLevelOld()) break;
 BrakePressureActual=BrakePressureTable[BrakeCtrlPos+2]; //skopiowanie pozycji
// if (BrakeSystem==Pneumatic?BrakeSubsystem==Oerlikon:false) //tylko Oerlikon akceptuje u�amki     //youBy: obawiam sie, ze tutaj to nie dziala :P
 if(false)
  if (fBrakeCtrlPos>0.0)
  {//warto�ci po�rednie wyliczamy tylko dla hamowania
   double u=fBrakeCtrlPos-double(x); //u�amek ponad warto�� ca�kowit�
   if (u>0.0)
   {//wyliczamy warto�ci wa�one
    BrakePressureActual.PipePressureVal+=-u*BrakePressureActual.PipePressureVal+u*BrakePressureTable[BrakeCtrlPos+1+2].PipePressureVal;
    //BrakePressureActual.BrakePressureVal+=-u*BrakePressureActual.BrakePressureVal+u*BrakePressureTable[BrakeCtrlPos+1].BrakePressureVal;  //to chyba nie b�dzie tak dzia�a�, zw�aszcza w EN57
    BrakePressureActual.FlowSpeedVal+=-u*BrakePressureActual.FlowSpeedVal+u*BrakePressureTable[BrakeCtrlPos+1+2].FlowSpeedVal;
   }
  }
};

bool __fastcall TMoverParameters::BrakeLevelAdd(double b)
{//dodanie warto�ci (b) do pozycji hamulca (w tym ujemnej)
 //zwraca false, gdy po dodaniu by�o by poza zakresem
 BrakeLevelSet(fBrakeCtrlPos+b);
 return b>0.0?(fBrakeCtrlPos<BrakeCtrlPosNo):(BrakeCtrlPos>-1.0); //true, je�li mo�na kontynuowa�
};

bool __fastcall TMoverParameters::IncBrakeLevel()
{//nowa wersja na u�ytek AI, false gdy osi�gni�to pozycj� BrakeCtrlPosNo
 return BrakeLevelAdd(1.0);
};

bool __fastcall TMoverParameters::DecBrakeLevel()
{//nowa wersja na u�ytek AI, false gdy osi�gni�to pozycj� -1
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
    if (BrakeHandle==FV4a)   //!!!POBIERA� WARTO�� Z KLASY ZAWORU!!!
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
   //Ra: to poni�ej jest bez sensu - mo�na przej�� nie wy��czaj�c
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
 //Ra: przenios�em z Train.cpp, nie wiem czy ma to sens
 if (MaxCurrentSwitch(direction))
 {if (TrainType!=dt_EZT)
   return (MinCurrentSwitch(direction));
 }
 return false;
};

void __fastcall TMoverParameters::UpdatePantVolume(double dt)
{//KURS90 - spr�arka pantograf�w
 if ((TrainType==dt_EZT)?(PantPress<ScndPipePress):bPantKurek3) //kurek zamyka po��czenie z ZG
 {//zbiornik pantografu po��czony ze zbiornikiem g��wnym - ma�� spr�ark� si� tego nie napompuje
  //Ra 2013-12: Niebugoc�aw m�wi, �e w EZT nie ma potrzeby odcina� kurkiem
  PantPress=ScndPipePress;
  PantVolume=(ScndPipePress+1)*0.1; //obj�to��, na wypadek odci�cia kurkiem
 }
 else
 {//zbiornik g��wny odci�ty, mo�na pompowa� pantografy
  if (PantCompFlag&&Battery) //w��czona bateria i ma�a spr�arka
   PantVolume+=dt*(TrainType==dt_EZT?0.003:0.005)*(2*0.45-((0.1/PantVolume/10)-0.1))/0.45; //nape�nianie zbiornika pantograf�w
  //Ra 2013-12: Niebugoc�aw m�wi, �e w EZT nabija 1.5 raz wolniej ni� jak by�o 0.005
  PantPress=(10*PantVolume)-1; //tu by si� przyda�a obj�to�� zbiornika
 }
 if (!PantCompFlag&&(PantVolume>0.1))
  PantVolume-=dt*0.0003; //nieszczelno�ci: 0.0003=0.3l/s
 if (Mains) //nie wchodzi� w funkcj� bez potrzeby
  if (EngineType==ElectricSeriesMotor) //nie dotyczy... czego w�a�ciwie?
   if (PantPress<EnginePowerSource.CollectorParameters.MinPress)
    if ((TrainType&(dt_EZT|dt_ET40|dt_ET41|dt_ET42))?(GetTrainsetVoltage()<EnginePowerSource.CollectorParameters.MinV):true) //to jest troch� proteza; zasilanie cz�onu mo�e by� przez sprz�g WN
     if (MainSwitch(false))
      EventFlag=true; //wywalenie szybkiego z powodu niskiego ci�nienia
 if (TrainType!=dt_EZT) //w EN57 pompuje si� tylko w silnikowym
 //pierwotnie w CHK pantografy mia�y r�wnie� rozrz�dcze EZT 
 for (int b=0;b<=1;++b)
  if (TestFlag(Couplers[b].CouplingFlag,ctrain_controll))
   if (Couplers[b].Connected->PantVolume<PantVolume) //bo inaczej trzeba w obydwu cz�onach przestawia�
    Couplers[b].Connected->PantVolume=PantVolume; //przekazanie ci�nienia do s�siedniego cz�onu
 //czy np. w ET40, ET41, ET42 pantografy cz�on�w maj� po��czenie pneumatyczne?
};

void __fastcall TMoverParameters::UpdateBatteryVoltage(double dt)
{//przeliczenie obci��enia baterii
 double sn1,sn2,sn3,sn4,sn5; //Ra: zrobi� z tego amperomierz NN
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
   if (iLights[0]&63) //64=blachy, nie ci�gn� pr�du //rozpisa� na poszczeg�lne �ar�wki...
    sn4=dt*0.003;
   else sn4=0;
   if (iLights[1]&63) //64=blachy, nie ci�gn� pr�du
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
   if (iLights[0]&63) //64=blachy, nie ci�gn� pr�du
    sn4=(dt*0.0030);
   else sn4=0;
   if (iLights[1]&63) //64=blachy, nie ci�gn� pr�du
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
    sn5=dt*0.000001; //bardzo powolny spadek przy wy��czonych bateriach
  };
  BatteryVoltage-=(sn1+sn2+sn3+sn4+sn5);
  if (NominalBatteryVoltage/BatteryVoltage>1.57)
    if (MainSwitch(false)&&(EngineType!=DieselEngine)&&(EngineType!=WheelsDriven))
      EventFlag=true; //wywalanie szybkiego z powodu zbyt niskiego napiecia
  if (BatteryVoltage>NominalBatteryVoltage)
   BatteryVoltage=NominalBatteryVoltage; //wstrzymanie �adowania pow. 110V
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
1 //uk�ad szeregowy
2 //uk�ad r�wnoleg�y
3 //bocznik 1
4 //bocznik 2
5 //bocznik 3
6 //do przodu
7 //do ty�u
8 //1 przyspieszenie
9 //minus obw. 2 przyspieszenia
10 //jazda na oporach
11 //SHP
12A //podnoszenie pantografu przedniego
12B //podnoszenie pantografu tylnego
13A //opuszczanie pantografu przedniego
13B //opuszczanie wszystkich pantograf�w
14 //za��czenie WS
15 //rozrz�d (WS, PSR, wa� ku�akowy)
16 //odblok PN
18 //sygnalizacja przetwornicy g��wnej
19 //luzowanie EP
20 //hamowanie EP
21 //rezerwa** (1900+: zamykanie drzwi prawych)
22 //za�. przetwornicy g��wnej
23 //wy�. przetwornicy g��wnej
24 //za�. przetw. o�wietlenia
25 //wy�. przetwornicy o�wietlenia
26 //sygnalizacja WS
28 //spr�arka
29 //ogrzewanie
30 //rezerwa* (1900+: zamykanie drzwi lewych)
31 //otwieranie drzwi prawych
32H //zadzia�anie PN siln. trakcyjnych
33 //sygna� odjazdu
34 //rezerwa (sygnalizacja po�lizgu)
35 //otwieranie drzwi lewych
ZN //masa
*/

double __fastcall TMoverParameters::ComputeMovement
(double dt,double dt1,
 const TTrackShape &Shape,TTrackParam &Track,TTractionParam &ElectricTraction,
 const TLocation &NewLoc,TRotation &NewRot
)
{//trzeba po ma�u przenosi� tu t� funkcj�
 double d=T_MoverParameters::ComputeMovement(dt,dt1,Shape,Track,ElectricTraction,NewLoc,NewRot);
 if (EnginePowerSource.SourceType==CurrentCollector) //tylko je�li pantografuj�cy
  if (Power>1.0) //w rozrz�dczym nie (jest b��d w FIZ!)
   UpdatePantVolume(dt); //Ra: pneumatyka pantograf�w przeniesiona do Mover.cpp!
 //sprawdzanie i ewentualnie wykonywanie->kasowanie polece�
 if (LoadStatus>0) //czas doliczamy tylko je�li trwa (roz)�adowanie
  LastLoadChangeTime=LastLoadChangeTime+dt; //czas (roz)�adunku
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
 if (CompressorSpeed>0.0) //spr�arka musi mie� jak�� niezerow� wydajno��
  CompressorCheck(dt); //�eby rozwa�a� jej za��czenie i prac�
 UpdateBrakePressure(dt);
 UpdatePipePressure(dt);
 UpdateBatteryVoltage(dt);
 UpdateScndPipePressure(dt); // druga rurka, youBy
 //hamulec antypo�lizgowy - wy��czanie
 if ((BrakeSlippingTimer>0.8)&&(ASBType!=128)) //ASBSpeed=0.8
  Hamulec->ASB(0);
 //SetFlag(BrakeStatus,-b_antislip);
 BrakeSlippingTimer=BrakeSlippingTimer+dt;
 //sypanie piasku - wy��czone i piasek si� nie ko�czy - b��dy AI
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
{//trzeba po ma�u przenosi� tu t� funkcj�
 double d=T_MoverParameters::FastComputeMovement(dt,Shape,Track,NewLoc,NewRot);
 if (EnginePowerSource.SourceType==CurrentCollector) //tylko je�li pantografuj�cy
  if (Power>1.0) //w rozrz�dczym nie (jest b��d w FIZ!)
   UpdatePantVolume(dt); //Ra: pneumatyka pantograf�w przeniesiona do Mover.cpp!
 //sprawdzanie i ewentualnie wykonywanie->kasowanie polece�
 if (LoadStatus>0) //czas doliczamy tylko je�li trwa (roz)�adowanie
  LastLoadChangeTime=LastLoadChangeTime+dt; //czas (roz)�adunku
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
 if (CompressorSpeed>0.0) //spr�arka musi mie� jak�� niezerow� wydajno��
  CompressorCheck(dt); //�eby rozwa�a� jej za��czenie i prac�
 UpdateBrakePressure(dt);
 UpdatePipePressure(dt);
 UpdateScndPipePressure(dt); // druga rurka, youBy
 UpdateBatteryVoltage(dt);
 //hamulec antyposlizgowy - wy��czanie
 if ((BrakeSlippingTimer>0.8)&&(ASBType!=128)) //ASBSpeed=0.8
  Hamulec->ASB(0);
 BrakeSlippingTimer=BrakeSlippingTimer+dt;
 return d;
};
