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
 if (BrakeSystem==Pneumatic?BrakeSubsystem==Oerlikon:false) //tylko Oerlikon akceptuje u³amki
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
//var //b:byte;
//    c:boolean;
 if (abs(ActiveCab+direction)<2)
 {
//  if (ActiveCab+direction=0) then LastCab:=ActiveCab;
   ActiveCab=ActiveCab+direction;
   //ChangeCab=true;
   if ((BrakeSystem==Pneumatic)&&(BrakeCtrlPosNo>0))
   {
    BrakeLevelSet(-2); //BrakeCtrlPos=-2;
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
   if ((EngineType!=DieselEngine)&&(EngineType!=DieselElectric))
   {
    Mains=false;
    CompressorAllow=false;
    ConverterAllow=false;
   }
//   if (ActiveCab<>LastCab) and (ActiveCab<>0) then
//    begin
//     c:=PantFrontUp;
//     PantFrontUp:=PantRearUp;
//     PantRearUp:=c;
//    end; //yB: poszlo do wylacznika rozrzadu
  ActiveDir=0;
  DirAbsolute=0;
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
