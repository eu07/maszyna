/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include <MATH.H>
#include <FLOAT.H>
#include <typeinfo>
#include <fstream> // std::ifstream
#include <sstream>
#include <istream> 
#include <iostream>
#include <stdio.h> // sprintf()
#include <stdlib.h>
#include <math.h>
#include <cmath>
#include <stdio.h>

#include "Mover.h"
#include "../globals.h"
//#include "../qutils.h"
#include <mctools.h>
#include "../logs.h"
#include "hamulce.h"
#include "Oerlikon_ESt.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

// Ra: tu nale¿y przenosiæ funcje z mover.pas, które nie s¹ z niego wywo³ywane.
// Jeœli jakieœ zmienne nie s¹ u¿ywane w mover.pas, te¿ mo¿na je przenosiæ.
// Przeniesienie wszystkiego na raz zrobi³o by zbyt wielki chaos do ogarniêcia.

const double dEpsilon = 0.01; // 1cm (zale¿y od typu sprzêgu...)
const double CouplerTune = 0.1; // skalowanie tlumiennosci

long Trunc(float f)
{
    return (long)f;
}

long ROUND(float f)
{
    return Trunc(f + 0.5);
}

double sqr(double val) // SQR() zle liczylo w current() ...
{
    return val * val;
}

double ComputeCollision(double v1, double v2, double m1, double m2, double beta, bool vc)
{ // oblicza zmiane predkosci i przyrost pedu wskutek kolizji
	if (v1 < v2 && vc)
		return 0;
	else
	{
		double sum = m1 + m2;
		double w1 = ((m1 - m2) * v1 + 2 * m2 * v2) / sum;
		double w2 = ((m2 - m1) * v2 + 2 * m1 * v1) / sum;
		v1 = w1 * sqrt(1 - beta); // niejawna zmiana predkosci wskutek zderzenia
		v2 = w2 * sqrt(1 - beta);
		return m1 * (w2 - w1) * (1 - beta);
	}
}

int DirPatch(int Coupler1, int Coupler2)
{
	return (Coupler1 != Coupler2 ? 1 : -1);
}

int DirF(int CouplerN)
{
	switch (CouplerN)
	{
	case 0:
		return -1;
	case 1:
		return 1;
	default:
		return 0;
	}
}

// *************************************************************************************************
// Q: 20160716
// *************************************************************************************************
double TMoverParameters::current(double n, double U)
{
    // wazna funkcja - liczy prad plynacy przez silniki polaczone szeregowo lub rownolegle
    // w zaleznosci od polozenia nastawnikow MainCtrl i ScndCtrl oraz predkosci obrotowej n
    // a takze wywala bezpiecznik nadmiarowy gdy za duzy prad lub za male napiecie
    // jest takze mozliwosc uszkodzenia silnika wskutek nietypowych parametrow
    const float ep09resED = 5.8; // TODO: dobrac tak aby sie zgadzalo ze wbudzeniem

    double R, MotorCurrent;
    double Rz, Delta, Isf;
    int Mn;
    double Bn;
    int SP;
    double U1; // napiecie z korekta

    MotorCurrent = 0;
    // i dzialanie hamulca ED w EP09
    //-  if (DynamicBrakeType == dbrake_automatic)
    //-   {
    //-    if ((( static_cast<TLSt*>(Hamulec)->GetEDBCP() < 0.25) && (Vadd < 1)) || (BrakePress >
    //2.1))
    //-      DynamicBrakeFlag = false;
    //-    else if ((BrakePress > 0.25) && (static_cast<TLSt*>(Hamulec)->GetEDBCP() > 0.25))
    //-      DynamicBrakeFlag = true;
    //-    DynamicBrakeFlag = (DynamicBrakeFlag && ConverterFlag);
    //-   }

    // wylacznik cisnieniowy yBARC - to jest chyba niepotrzebne tutaj   Q: no to usuwam...

    // BrakeSubsystem = ss_LSt;
    // if (BrakeSubsystem == ss_LSt) WriteLog("LSt");
    if (BrakeSubsystem == ss_LSt)
        if (DynamicBrakeFlag)
        {
            static_cast<TLSt *>(Hamulec)
                ->SetED(abs(Im / 350)); // hamulec ED na EP09 dziala az do zatrzymania lokomotywy
            //-     WriteLog("A");
        }
        else
        {
            static_cast<TLSt *>(Hamulec)->SetED(0);
            //-     WriteLog("B");
        }

    ResistorsFlag = (RList[MainCtrlActualPos].R > 0.01); // and (!DelayCtrlFlag)
    ResistorsFlag =
        (ResistorsFlag || ((DynamicBrakeFlag == true) && (DynamicBrakeType == dbrake_automatic)));

    if ((TrainType == dt_ET22) && (DelayCtrlFlag) && (MainCtrlActualPos > 1))
        Bn = 1 - 1 / RList[MainCtrlActualPos].Bn;
    else
        Bn = 1; // to jest wykonywane dla EU07

    R = RList[MainCtrlActualPos].R * Bn + CircuitRes;

    if ((TrainType != dt_EZT) || (Imin != IminLo) ||
        (!ScndS)) // yBARC - boczniki na szeregu poprawnie
        Mn = RList[MainCtrlActualPos].Mn; // to jest wykonywane dla EU07
    else
        Mn = RList[MainCtrlActualPos].Mn * RList[MainCtrlActualPos].Bn;

    //  writepaslog("#",
    //  "C++-----------------------------------------------------------------------------");
    //  writepaslog("MCAP           ", IntToStr(MainCtrlActualPos));
    //  writepaslog("SCAP           ", IntToStr(ScndCtrlActualPos));
    //  writepaslog("n              ", FloatToStr(n));
    //  writepaslog("StLinFlag      ", BoolToYN(StLinFlag));
    //  writepaslog("DelayCtrlFlag  ", booltoYN(DelayCtrlFlag));
    //  writepaslog("Bn             ", FloatToStr(Bn));
    //  writepaslog("R              ", FloatToStr(R));
    //  writepaslog("Mn             ", IntToStr(Mn));
    //  writepaslog("RList[MCAP].Bn ", FloatToStr(RList[MainCtrlActualPos].Bn));
    //  writepaslog("RList[MCAP].Mn ", FloatToStr(RList[MainCtrlActualPos].Mn));
    //  writepaslog("RList[MCAP].R  ", FloatToStr(RList[MainCtrlActualPos].R));

    // z Megapacka ... bylo tutaj zakomentowane    Q: no to usuwam...

    //-  if (DynamicBrakeFlag && (!FuseFlag) && (DynamicBrakeType == dbrake_automatic) &&
    //ConverterFlag && Mains)    //hamowanie EP09   //TUHEX
    //-     {
    //-       WriteLog("TUHEX");
    //-       MotorCurrent = -Max0R(MotorParam[0].fi * (Vadd / (Vadd + MotorParam[0].Isat) -
    //MotorParam[0].fi0), 0) * n * 2 / ep09resED; //TODO: zrobic bardziej uniwersalne nie tylko dla
    //EP09
    //-     }
    //-  else
    if ((RList[MainCtrlActualPos].Bn == 0) || (!StLinFlag))
        MotorCurrent = 0; // wylaczone
    else
    { // wlaczone...
        SP = ScndCtrlActualPos;

        if (ScndCtrlActualPos < 255) // tak smiesznie bede wylaczal
        {
            if (ScndInMain)
                if (!(RList[MainCtrlActualPos].ScndAct == 255))
                    SP = RList[MainCtrlActualPos].ScndAct;

            Rz = Mn * WindingRes + R;

            //-        if (DynamicBrakeFlag) //hamowanie
            //-         {
            //-
            //-           if (DynamicBrakeType > 1)
            //-            {
            //-
            //-            if ((DynamicBrakeType == dbrake_switch) && (TrainType == dt_ET42))
            //-             { //z Megapacka
            //-               Rz = WindingRes + R;
            //-               MotorCurrent = -MotorParam[SP].fi * n / Rz;
            ////{hamowanie silnikiem na oporach rozruchowych}
            //-             }
            //-            }
            //-           else
            //-             MotorCurrent = 0; //odciecie pradu od silnika
            //-         }
            //-        else
            {
                U1 = U + Mn * n * MotorParam[SP].fi0 * MotorParam[SP].fi;
                //          writepaslog("U1             ", FloatToStr(U1));
                //          writepaslog("Isat           ", FloatToStr(MotorParam[SP].Isat));
                //          writepaslog("fi             ", FloatToStr(MotorParam[SP].fi));
                Isf = Sign(U1) * MotorParam[SP].Isat;
                //          writepaslog("Isf            ", FloatToStr(Isf));
                Delta = sqr(Isf * Rz + Mn * MotorParam[SP].fi * n - U1) +
                        4 * U1 * Isf * Rz; // 105 * 1.67 + Mn * 140.9 * 20.532 - U1
                //          DeltaQ = Isf * Rz + Mn * MotorParam[SP].fi * n - U1 + 4 * U1 * Isf * Rz;
                //          writepaslog("Delta          ", FloatToStr(Delta));
                //          writepaslog("DeltaQ         ", FloatToStr(DeltaQ));
                //          writepaslog("U              ", FloatToStr(U));
                if (Mains)
                {
                    if (U > 0)
                        MotorCurrent =
                            (U1 - Isf * Rz - Mn * MotorParam[SP].fi * n + sqrt(Delta)) / (2.0 * Rz);
                    else
                        MotorCurrent =
                            (U1 - Isf * Rz - Mn * MotorParam[SP].fi * n - sqrt(Delta)) / (2.0 * Rz);
                }
                else
                    MotorCurrent = 0;
            } // else DBF

        } // 255
        else
            MotorCurrent = 0;
    }
    //  writepaslog("MotorCurrent   ", FloatToStr(MotorCurrent));

    //-  if ((DynamicBrakeType == dbrake_switch) && ((BrakePress > 2.0) || (PipePress < 3.6)))
    //-   {
    //-    WriteLog("DynamicBrakeType=" + IntToStr( dbrake_switch ));
    //-    Im = 0;
    //-    MotorCurrent = 0;
    //-    Itot = 0;
    //-   }
    //-  else
    {
        Im = MotorCurrent;
    }

    EnginePower = abs(Itot) * (1 + RList[MainCtrlActualPos].Mn) * abs(U);

    // awarie
    MotorCurrent = abs(Im); // zmienna pomocnicza

    if (MotorCurrent > 0)
    {

        //-if FuzzyLogic(Abs(n),nmax*1.1,p_elengproblem) then
        //- if MainSwitch(false) then
        //-   EventFlag:=true; {zbyt duze obroty - wywalanie wskutek ognia okreznego}
        //-if TestFlag(DamageFlag,dtrain_engine) then
        //- if FuzzyLogic(MotorCurrent,ImaxLo/10.0,p_elengproblem) then
        //-  if MainSwitch(false) then
        //-   EventFlag:=true;
        // uszkodzony silnik (uplywy)
        //-if ((FuzzyLogic(abs(Im), Imax * 2, p_elengproblem) || FuzzyLogic(abs(n), nmax * 1.11,
        //p_elengproblem)))

        //-if (SetFlag(DamageFlag, dtrain_engine))
        //-   EventFlag = true;

        //! dorobic grzanie oporow rozruchowych i silnika
    }

    return Im;
}

// *************************************************************************************************
//  g³ówny konstruktor
// *************************************************************************************************
TMoverParameters::TMoverParameters(double VelInitial, std::string TypeNameInit,
                                   std::string NameInit, int LoadInitial,
                                   std::string LoadTypeInitial,
                                   int Cab) //: T_MoverParameters(VelInitial, TypeNameInit,
                                            //NameInit, LoadInitial, LoadTypeInitial, Cab)
{
    int b, k;
    WriteLog(
        "----------------------------------------------------------------------------------------");
    WriteLog("init default physic values for " + NameInit + ", [" + TypeNameInit + "], [" +
             LoadTypeInitial + "]");

    DimHalf.x = 0.5 * Dim.W; // po³owa szerokoœci, OX jest w bok?
    DimHalf.y = 0.5 * Dim.L; // po³owa d³ugoœci, OY jest do przodu?
    DimHalf.z = 0.5 * Dim.H; // po³owa wysokoœci, OZ jest w górê?
    // BrakeLevelSet(-2); //Pascal ustawia na 0, przestawimy na odciêcie (CHK jest jeszcze nie
    // wczytane!)
    bPantKurek3 = true; // domyœlnie zbiornik pantografu po³¹czony jest ze zbiornikiem g³ównym
    iProblem = 0; // pojazd w pe³ni gotowy do ruchu
    iLights[0] = iLights[1] = 0; //œwiat³a zgaszone

    // inicjalizacja stalych
    dMoveLen = 0;
    CategoryFlag = 1;
    EngineType = None;
    for (b = 0; b < ResArraySize; b++)
    {
        RList[b].Relay = 0;
        RList[b].R = 0;
        RList[b].Bn = 0;
        RList[b].Mn = 0;
        RList[b].AutoSwitch = false;
    }
    WheelDiameter = 1.0;
    BrakeCtrlPosNo = 0;
    LightsPosNo = 0;
    LightsDefPos = 1;
    for (k = -1; k < MainBrakeMaxPos; k++)
    {
        BrakePressureTable[k].PipePressureVal = 0;
        BrakePressureTable[k].BrakePressureVal = 0;
        BrakePressureTable[k].FlowSpeedVal = 0;
    }

    // with BrakePressureTable[-2] do  {pozycja odciecia}
    {
        BrakePressureTable[-2].PipePressureVal = -1;
        BrakePressureTable[-2].BrakePressureVal = -1;
        BrakePressureTable[-2].FlowSpeedVal = 0;
    }
    Transmision.Ratio = 1;
    NBpA = 0;
    DynamicBrakeType = 0;
    ASBType = 0;
    AutoRelayType = 0;
    for (b = 0; b < 1; b++) // Ra: kto tu zrobi³ "for b:=1 to 2 do" ???
    {
        Couplers[b].CouplerType = NoCoupler;
        Couplers[b].SpringKB = 1;
        Couplers[b].SpringKC = 1;
        Couplers[b].DmaxB = 0.1;
        Couplers[b].FmaxB = 1000;
        Couplers[b].DmaxC = 0.1;
        Couplers[b].FmaxC = 1000;
    }
    Power = 0;
    MaxLoad = 0;
    LoadAccepted = "";
    LoadSpeed = 0;
    UnLoadSpeed = 0;
    HeatingPower = 0;
    LightPower = 0;

    HeatingPowerSource.MaxVoltage = 0;
    HeatingPowerSource.MaxCurrent = 0;
    HeatingPowerSource.IntR = 0.001;
    HeatingPowerSource.SourceType = NotDefined;
    HeatingPowerSource.PowerType = NoPower;
    HeatingPowerSource.RPowerCable.PowerTrans = NoPower;

    AlterHeatPowerSource.MaxVoltage = 0;
    AlterHeatPowerSource.MaxCurrent = 0;
    AlterHeatPowerSource.IntR = 0.001;
    AlterHeatPowerSource.SourceType = NotDefined;
    AlterHeatPowerSource.PowerType = NoPower;
    AlterHeatPowerSource.RPowerCable.PowerTrans = NoPower;

    LightPowerSource.MaxVoltage = 0;
    LightPowerSource.MaxCurrent = 0;
    LightPowerSource.IntR = 0.001;
    LightPowerSource.SourceType = NotDefined;
    LightPowerSource.PowerType = NoPower;
    LightPowerSource.RPowerCable.PowerTrans = NoPower;

    AlterLightPowerSource.MaxVoltage = 0;
    AlterLightPowerSource.MaxCurrent = 0;
    AlterLightPowerSource.IntR = 0.001;
    AlterLightPowerSource.SourceType = NotDefined;
    AlterLightPowerSource.PowerType = NoPower;
    AlterLightPowerSource.RPowerCable.PowerTrans = NoPower;

    TypeName = TypeNameInit;
    HighPipePress = 0;
    LowPipePress = 0;
    DeltaPipePress = 0;
    BrakeCylNo = 0;
    BrakeCylRadius = 0;
    BrakeCylDist = 0;
    for (b = 0; b < 3; b++)
        BrakeCylMult[b] = 0;
    VeselVolume = 0;
    BrakeVolume = 0;
    RapidMult = 1;
    dizel_Mmax = 1;
    dizel_nMmax = 1;
    dizel_Mnmax = 2;
    dizel_nmax = 2;
    dizel_nominalfill = 0;
    dizel_Mstand = 0;
    dizel_nmax_cutoff = 0;
    dizel_nmin = 0;
    dizel_minVelfullengage = 0;
    dizel_AIM = 1;
    dizel_engageDia = 0.5;
    dizel_engageMaxForce = 6000;
    dizel_engagefriction = 0.5;
    DoorOpenCtrl = 0;
    DoorCloseCtrl = 0;
    DoorStayOpen = 0;
    DoorClosureWarning = false;
    DoorOpenSpeed = 1;
    DoorCloseSpeed = 1;
    DoorMaxShiftL = 0.5;
    DoorMaxShiftR = 0.5;
    DoorOpenMethod = 2;
    DepartureSignal = false;
    InsideConsist = false;
    CompressorPower = 1;
    SmallCompressorPower = 0;

    ScndInMain = false;

    Vhyp = 1;
    Vadd = 1;
    PowerCorRatio = 1;

    // inicjalizacja zmiennych}
    // Loc:=LocInitial; //Ra: to i tak trzeba potem przesun¹æ, po ustaleniu pozycji na torze
    // (potrzebna d³ugoœæ)
    // Rot:=RotInitial;
    for (b = 0; b < 1; b++)
    {
        Couplers[b].AllowedFlag = 3; // domyœlnie hak i hamulec, inne trzeba w³¹czyæ jawnie w FIZ
        Couplers[b].CouplingFlag = 0;
        Couplers[b].Connected = NULL;
        Couplers[b].ConnectedNr = 0; // Ra: to nie ma znaczenia jak nie pod³¹czony
        Couplers[b].Render = false;
        Couplers[b].CForce = 0;
        Couplers[b].Dist = 0;
        Couplers[b].CheckCollision = false;
    }
    ScanCounter = 0;
    BrakeCtrlPos = -2; // to nie ma znaczenia, konstruktor w Mover.cpp zmienia na -2
    BrakeDelayFlag = 0;
    BrakeStatus = b_off;
    EmergencyBrakeFlag = false;
    MainCtrlPos = 0;
    ScndCtrlPos = 0;
    MainCtrlActualPos = 0;
    ScndCtrlActualPos = 0;
    Heating = false;
    Mains = false;
    ActiveDir = 0; // kierunek nie ustawiony
    CabNo = 0; // sterowania nie ma, ustawiana przez CabActivization()
    ActiveCab = Cab; // obsada w podanej kabinie
    DirAbsolute = 0;
    SlippingWheels = false;
    SandDose = false;
    FuseFlag = false;
    ConvOvldFlag = false; // hunter-251211
    StLinFlag = false;
    ResistorsFlag = false;
    RventRot = 0;
    enrot = 0;
    nrot = 0;
    Itot = 0;
    EnginePower = 0;
    BrakePress = 0;
    Compressor = 0;
    ConverterFlag = false;
    CompressorAllow = false;
    DoorLeftOpened = false;
    DoorRightOpened = false;
    Battery = false;
    EpFuse = true;
    Signalling = false;
    Radio = true;
    DoorSignalling = false;
    UnBrake = false;
    // Winger 160204
    PantVolume =
        0.48; // aby podniesione pantografy opad³y w krótkim czasie przy wy³¹czonej sprê¿arce
    PantFrontUp = false;
    PantRearUp = false;
    PantFrontStart = 0;
    PantRearStart = 0;
    PantFrontSP = true;
    PantRearSP = true;
    DoubleTr = 1;
    BrakeSlippingTimer = 0;
    dpBrake = 0;
    dpPipe = 0;
    dpMainValve = 0;
    dpLocalValve = 0;
    MBPM = 1;
    DynamicBrakeFlag = false;
    BrakeSystem = Individual;
    BrakeSubsystem = ss_None;
    Ft = 0;
    Ff = 0;
    Fb = 0;
    FTotal = 0;
    FStand = 0;
    FTrain = 0;
    AccS = 0;
    AccN = 0;
    AccV = 0;
    EventFlag = false;
    SoundFlag = 0;
    Vel = abs(VelInitial);
    V = VelInitial / 3.6;
    LastSwitchingTime = 0;
    LastRelayTime = 0;
    DistCounter = 0;
    PulseForce = 0;
    PulseForceTimer = 0;
    PulseForceCount = 0;
    eAngle = 1.5;
    dizel_fill = 0;
    dizel_engagestate = 0;
    dizel_engage = 0;
    dizel_automaticgearstatus = 0;
    dizel_enginestart = false;
    dizel_engagedeltaomega = 0;
    PhysicActivation = true;

    RunningShape.R = 0;
    RunningShape.Len = 1;
    RunningShape.dHtrack = 0;
    RunningShape.dHrail = 0;

    RunningTrack.CategoryFlag = CategoryFlag;
    RunningTrack.Width = TrackW;
    RunningTrack.friction = Steel2Steel_friction;
    RunningTrack.QualityFlag = 20;
    RunningTrack.DamageFlag = 0;
    RunningTrack.Velmax = 100; // dla uzytku maszynisty w ai_driver}

    RunningTraction.TractionVoltage = 0;
    RunningTraction.TractionFreq = 0;
    RunningTraction.TractionMaxCurrent = 0;
    RunningTraction.TractionResistivity = 1;

    OffsetTrackH = 0;
    OffsetTrackV = 0;

    CommandIn.Command = "";
    CommandIn.Value1 = 0;
    CommandIn.Value2 = 0;
    CommandIn.Location.X = 0;
    CommandIn.Location.Y = 0;
    CommandIn.Location.Z = 0;

    // czesciowo stale, czesciowo zmienne}

    SecuritySystem.SystemType = 0;
    SecuritySystem.AwareDelay = -1;
    SecuritySystem.SoundSignalDelay = -1;
    SecuritySystem.EmergencyBrakeDelay = -1;
    SecuritySystem.Status = 0;
    SecuritySystem.SystemTimer = 0;
    SecuritySystem.SystemBrakeCATimer = 0;
    SecuritySystem.SystemBrakeSHPTimer = 0; // hunter-091012
    SecuritySystem.VelocityAllowed = -1;
    SecuritySystem.NextVelocityAllowed = -1;
    SecuritySystem.RadioStop = false; // domyœlnie nie ma
    SecuritySystem.AwareMinSpeed = 0.1 * Vmax;

    // ABu 240105:
    // CouplerNr[0]:=1;
    // CouplerNr[1]:=0;

    // TO POTEM TU UAKTYWNIC A WYWALIC Z CHECKPARAM}
    //{
    //  if Pos(LoadTypeInitial,LoadAccepted)>0 then
    //   begin
    //}
    LoadType = LoadTypeInitial;
    Load = LoadInitial;
    LoadStatus = 0;
    LastLoadChangeTime = 0;
    //{
    //   end
    //  else Load:=0;
    // }

    Name = NameInit;
    DerailReason = 0; // Ra: powód wykolejenia
    TotalCurrent = 0;
    ShuntModeAllow = false;
    ShuntMode = false;
};

double TMoverParameters::Distance(const TLocation &Loc1, const TLocation &Loc2,
                                  const TDimension &Dim1, const TDimension &Dim2)
{ // zwraca odleg³oœæ pomiêdzy pojazdami (Loc1) i (Loc2) z uwzglêdnieneim ich d³ugoœci (kule!)
    return hypot(Loc2.X - Loc1.X, Loc1.Y - Loc2.Y) - 0.5 * (Dim2.L + Dim1.L);
};

double TMoverParameters::Distance(const vector3 &s1, const vector3 &s2, const vector3 &d1,
                                  const vector3 &d2){
    // obliczenie odleg³oœci prostopad³oœcianów o œrodkach (s1) i (s2) i wymiarach (d1) i (d2)
    // return 0.0; //bêdzie zg³aszaæ warning - funkcja do usuniêcia, chyba ¿e siê przyda...
};

double TMoverParameters::CouplerDist(int Coupler)
{ // obliczenie odleg³oœci pomiêdzy sprzêgami (kula!)
    return Couplers[Coupler].CoupleDist =
               Distance(Loc, Couplers[Coupler].Connected->Loc, Dim,
                        Couplers[Coupler].Connected->Dim); // odleg³oœæ pomiêdzy sprzêgami (kula!)
};

bool TMoverParameters::AttachA(int ConnectNo, int ConnectToNr, TMoverParameters *ConnectTo,
                               int CouplingType, bool Forced)
{ //³¹czenie do swojego sprzêgu (ConnectNo) pojazdu (ConnectTo) stron¹ (ConnectToNr)
    // Ra: zwykle wykonywane dwukrotnie, dla ka¿dego pojazdu oddzielnie
    // Ra: trzeba by odró¿niæ wymóg dociœniêcia od uszkodzenia sprzêgu przy podczepianiu AI do
    // sk³adu
    if (ConnectTo) // jeœli nie pusty
    {
        if (ConnectToNr != 2)
            Couplers[ConnectNo].ConnectedNr = ConnectToNr; // 2=nic nie pod³¹czone
        TCouplerType ct = ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr]
                              .CouplerType; // typ sprzêgu pod³¹czanego pojazdu
        Couplers[ConnectNo].Connected =
            ConnectTo; // tak podpi¹æ (do siebie) zawsze mo¿na, najwy¿ej bêdzie wirtualny
        CouplerDist(ConnectNo); // przeliczenie odleg³oœci pomiêdzy sprzêgami
        if (CouplingType == ctrain_virtual)
            return false; // wirtualny wiêcej nic nie robi
        if (Forced ? true : ((Couplers[ConnectNo].CoupleDist <= dEpsilon) &&
                             (Couplers[ConnectNo].CouplerType != NoCoupler) &&
                             (Couplers[ConnectNo].CouplerType == ct)))
        { // stykaja sie zderzaki i kompatybilne typy sprzegow, chyba ¿e ³¹czenie na starcie
            if (Couplers[ConnectNo].CouplingFlag ==
                ctrain_virtual) // jeœli wczeœniej nie by³o po³¹czone
            { // ustalenie z której strony rysowaæ sprzêg
                Couplers[ConnectNo].Render = true; // tego rysowaæ
                ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr].Render = false; // a tego nie
            };
            Couplers[ConnectNo].CouplingFlag = CouplingType; // ustawienie typu sprzêgu
            // if (CouplingType!=ctrain_virtual) //Ra: wirtualnego nie ³¹czymy zwrotnie!
            //{//jeœli ³¹czenie sprzêgiem niewirtualnym, ustawiamy po³¹czenie zwrotne
            ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr].CouplingFlag = CouplingType;
            ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr].Connected = this;
            ConnectTo->Couplers[Couplers[ConnectNo].ConnectedNr].CoupleDist =
                Couplers[ConnectNo].CoupleDist;
            return true;
            //}
            // pod³¹czenie nie uda³o siê - jest wirtualne
        }
    }
    return false; // brak pod³¹czanego pojazdu, zbyt du¿a odleg³oœæ, niezgodny typ sprzêgu, brak
    // sprzêgu, brak haka
};

bool TMoverParameters::Attach(int ConnectNo, int ConnectToNr, TMoverParameters *ConnectTo,
                              int CouplingType, bool Forced)
{ //³¹czenie do (ConnectNo) pojazdu (ConnectTo) stron¹ (ConnectToNr)
    return AttachA(ConnectNo, ConnectToNr, (TMoverParameters *)ConnectTo, CouplingType, Forced);
};

int TMoverParameters::DettachStatus(int ConnectNo)
{ // Ra: sprawdzenie, czy odleg³oœæ jest dobra do roz³¹czania
    // powinny byæ 3 informacje: =0 sprzêg ju¿ roz³¹czony, <0 da siê roz³¹czyæ. >0 nie da siê
    // roz³¹czyæ
    if (!Couplers[ConnectNo].Connected)
        return 0; // nie ma nic, to roz³¹czanie jest OK
    if ((Couplers[ConnectNo].CouplingFlag & ctrain_coupler) == 0)
        return -Couplers[ConnectNo].CouplingFlag; // hak nie po³¹czony - roz³¹czanie jest OK
    if (TestFlag(DamageFlag, dtrain_coupling))
        return -Couplers[ConnectNo].CouplingFlag; // hak urwany - roz³¹czanie jest OK
    // ABu021104: zakomentowane 'and (CouplerType<>Articulated)' w warunku, nie wiem co to bylo, ale
    // za to teraz dziala odczepianie... :) }
    // if (CouplerType==Articulated) return false; //sprzêg nie do rozpiêcia - mo¿e byæ tylko urwany
    // Couplers[ConnectNo].CoupleDist=Distance(Loc,Couplers[ConnectNo].Connected->Loc,Dim,Couplers[ConnectNo].Connected->Dim);
    CouplerDist(ConnectNo);
    if (Couplers[ConnectNo].CouplerType == Screw ? Couplers[ConnectNo].CoupleDist < 0.0 : true)
        return -Couplers[ConnectNo].CouplingFlag; // mo¿na roz³¹czaæ, jeœli dociœniêty
    return (Couplers[ConnectNo].CoupleDist > 0.2) ? -Couplers[ConnectNo].CouplingFlag :
                                                    Couplers[ConnectNo].CouplingFlag;
};

bool TMoverParameters::Dettach(int ConnectNo)
{ // rozlaczanie
    if (!Couplers[ConnectNo].Connected)
        return true; // nie ma nic, to odczepiono
    // with Couplers[ConnectNo] do
    int i = DettachStatus(ConnectNo); // stan sprzêgu
    if (i < 0)
    { // gdy scisniete zderzaki, chyba ze zerwany sprzeg (wirtualnego nie odpinamy z drugiej strony)
        // Couplers[ConnectNo].Connected=NULL; //lepiej zostawic bo przeciez trzeba kontrolowac
        // zderzenia odczepionych
        Couplers[ConnectNo].Connected->Couplers[Couplers[ConnectNo].ConnectedNr].CouplingFlag =
            0; // pozostaje sprzêg wirtualny
        Couplers[ConnectNo].CouplingFlag = 0; // pozostaje sprzêg wirtualny
        return true;
    }
    else if (i > 0)
    { // od³¹czamy wê¿e i resztê, pozostaje sprzêg fizyczny, który wymaga dociœniêcia (z wirtualnym
        // nic)
        Couplers[ConnectNo].CouplingFlag &= ctrain_coupler;
        Couplers[ConnectNo].Connected->Couplers[Couplers[ConnectNo].ConnectedNr].CouplingFlag =
            Couplers[ConnectNo].CouplingFlag;
    }
    return false; // jeszcze nie roz³¹czony
};

void TMoverParameters::SetCoupleDist()
{ // przeliczenie odleg³oœci sprzêgów
    if (Couplers[0].Connected)
    {
        CouplerDist(0);
        if (CategoryFlag & 2)
        { // Ra: dla samochodów zderzanie kul to za ma³o
        }
    }
    if (Couplers[1].Connected)
    {
        CouplerDist(1);
        if (CategoryFlag & 2)
        { // Ra: dla samochodów zderzanie kul to za ma³o
        }
    }
};

bool TMoverParameters::DirectionForward()
{
    if ((MainCtrlPosNo > 0) && (ActiveDir < 1) && (MainCtrlPos == 0))
    {
        ++ActiveDir;
        DirAbsolute = ActiveDir * CabNo;
        if (DirAbsolute)
            if (Battery) // jeœli bateria jest ju¿ za³¹czona
                BatterySwitch(true); // to w ten oto durny sposób aktywuje siê CA/SHP
        SendCtrlToNext("Direction", ActiveDir, CabNo);
        return true;
    }
    else if ((ActiveDir == 1) && (MainCtrlPos == 0) && (TrainType == dt_EZT))
        return MinCurrentSwitch(true); //"wysoki rozruch" EN57
    return false;
};

// Nastawianie hamulców

void TMoverParameters::BrakeLevelSet(double b)
{ // ustawienie pozycji hamulca na wartoœæ (b) w zakresie od -2 do BrakeCtrlPosNo
    // jedyny dopuszczalny sposób przestawienia hamulca zasadniczego
    if (fBrakeCtrlPos == b)
        return; // nie przeliczaæ, jak nie ma zmiany
    fBrakeCtrlPos = b;
    BrakeCtrlPosR = b;
    if (fBrakeCtrlPos < Handle->GetPos(bh_MIN))
        fBrakeCtrlPos = Handle->GetPos(bh_MIN); // odciêcie
    else if (fBrakeCtrlPos > Handle->GetPos(bh_MAX))
        fBrakeCtrlPos = Handle->GetPos(bh_MAX);
    int x = floor(fBrakeCtrlPos); // jeœli odwo³ujemy siê do BrakeCtrlPos w poœrednich, to musi byæ
    // obciête a nie zaokr¹gone
    while ((x > BrakeCtrlPos) && (BrakeCtrlPos < BrakeCtrlPosNo)) // jeœli zwiêkszy³o siê o 1
        if (!IncBrakeLevelOld()) // T_MoverParameters::
            break; // wyjœcie awaryjne
    while ((x < BrakeCtrlPos) && (BrakeCtrlPos >= -1)) // jeœli zmniejszy³o siê o 1
        if (!DecBrakeLevelOld()) // T_MoverParameters::
            break;
    BrakePressureActual = BrakePressureTable[BrakeCtrlPos + 2]; // skopiowanie pozycji
    /*
    //youBy: obawiam sie, ze tutaj to nie dziala :P
    //Ra 2014-03: by³o tak zrobione, ¿e dzia³a³o - po ka¿dej zmianie pozycji by³a wywo³ywana ta
    funkcja
    // if (BrakeSystem==Pneumatic?BrakeSubsystem==Oerlikon:false) //tylko Oerlikon akceptuje u³amki
     if(false)
      if (fBrakeCtrlPos>0.0)
      {//wartoœci poœrednie wyliczamy tylko dla hamowania
       double u=fBrakeCtrlPos-double(x); //u³amek ponad wartoœæ ca³kowit¹
       if (u>0.0)
       {//wyliczamy wartoœci wa¿one
        BrakePressureActual.PipePressureVal+=-u*BrakePressureActual.PipePressureVal+u*BrakePressureTable[BrakeCtrlPos+1+2].PipePressureVal;
        //BrakePressureActual.BrakePressureVal+=-u*BrakePressureActual.BrakePressureVal+u*BrakePressureTable[BrakeCtrlPos+1].BrakePressureVal;
    //to chyba nie bêdzie tak dzia³aæ, zw³aszcza w EN57
        BrakePressureActual.FlowSpeedVal+=-u*BrakePressureActual.FlowSpeedVal+u*BrakePressureTable[BrakeCtrlPos+1+2].FlowSpeedVal;
       }
      }
    */
};

bool TMoverParameters::BrakeLevelAdd(double b)
{ // dodanie wartoœci (b) do pozycji hamulca (w tym ujemnej)
    // zwraca false, gdy po dodaniu by³o by poza zakresem
    BrakeLevelSet(fBrakeCtrlPos + b);
    return b > 0.0 ? (fBrakeCtrlPos < BrakeCtrlPosNo) :
                     (BrakeCtrlPos > -1.0); // true, jeœli mo¿na kontynuowaæ
};

bool TMoverParameters::IncBrakeLevel()
{ // nowa wersja na u¿ytek AI, false gdy osi¹gniêto pozycjê BrakeCtrlPosNo
    return BrakeLevelAdd(1.0);
};

bool TMoverParameters::DecBrakeLevel()
{
    return BrakeLevelAdd(-1.0);
}; // nowa wersja na u¿ytek AI, false gdy osi¹gniêto pozycjê -1

bool TMoverParameters::ChangeCab(int direction)
{ // zmiana kabiny i resetowanie ustawien
    if (abs(ActiveCab + direction) < 2)
    {
        //  if (ActiveCab+direction=0) then LastCab:=ActiveCab;
        ActiveCab = ActiveCab + direction;
        if ((BrakeSystem == Pneumatic) && (BrakeCtrlPosNo > 0))
        {
            //    if (BrakeHandle==FV4a)   //!!!POBIERAÆ WARTOŒÆ Z KLASY ZAWORU!!!
            //     BrakeLevelSet(-2); //BrakeCtrlPos=-2;
            //    else if ((BrakeHandle==FVel6)||(BrakeHandle==St113))
            //     BrakeLevelSet(2);
            //    else
            //     BrakeLevelSet(1);
            BrakeLevelSet(Handle->GetPos(bh_NP));
            LimPipePress = PipePress;
            ActFlowSpeed = 0;
        }
        else
            // if (TrainType=dt_EZT) and (BrakeCtrlPosNo>0) then
            //  BrakeCtrlPos:=5; //z Megapacka
            // else
            //    BrakeLevelSet(0); //BrakeCtrlPos=0;
            BrakeLevelSet(Handle->GetPos(bh_NP));
        //   if not TestFlag(BrakeStatus,b_dmg) then
        //    BrakeStatus:=b_off; //z Megapacka
        MainCtrlPos = 0;
        ScndCtrlPos = 0;
        // Ra: to poni¿ej jest bez sensu - mo¿na przejœæ nie wy³¹czaj¹c
        // if ((EngineType!=DieselEngine)&&(EngineType!=DieselElectric))
        //{
        // Mains=false;
        // CompressorAllow=false;
        // ConverterAllow=false;
        //}
        // ActiveDir=0;
        // DirAbsolute=0;
        return true;
    }
    return false;
};

bool TMoverParameters::CurrentSwitch(int direction)
{ // rozruch wysoki (true) albo niski (false)
    // Ra: przenios³em z Train.cpp, nie wiem czy ma to sens
    if (MaxCurrentSwitch(direction))
    {
        if (TrainType != dt_EZT)
            return (MinCurrentSwitch(direction));
    }
    if (EngineType == DieselEngine) // dla 2Ls150
        if (ShuntModeAllow)
            if (ActiveDir == 0) // przed ustawieniem kierunku
                ShuntMode = direction;
    return false;
};

void TMoverParameters::UpdatePantVolume(double dt)
{ // KURS90 - sprê¿arka pantografów; Ra 2014-07: teraz jest to zbiornik rozrz¹du, chocia¿ to jeszcze
    // nie tak
    if (EnginePowerSource.SourceType == CurrentCollector) // tylko jeœli pantografuj¹cy
    {
        // Ra 2014-07: zasadniczo, to istnieje zbiornik rozrz¹du i zbiornik pantografów - na razie
        // mamy razem
        // Ra 2014-07: kurek trójdrogowy ³¹czy spr.pom. z pantografami i wy³¹cznikiem ciœnieniowym
        // WS
        // Ra 2014-07: zbiornika rozrz¹du nie pompuje siê tu, tylko pantografy; potem mo¿na zamkn¹æ
        // WS i odpaliæ resztê
        if ((TrainType == dt_EZT) ? (PantPress < ScndPipePress) :
                                    bPantKurek3) // kurek zamyka po³¹czenie z ZG
        { // zbiornik pantografu po³¹czony ze zbiornikiem g³ównym - ma³¹ sprê¿ark¹ siê tego nie
            // napompuje
            // Ra 2013-12: Niebugoc³aw mówi, ¿e w EZT nie ma potrzeby odcinaæ kurkiem
            PantPress = EnginePowerSource.CollectorParameters
                            .MaxPress; // ograniczenie ciœnienia do MaxPress (tylko w pantografach!)
            if (PantPress > ScndPipePress)
                PantPress = ScndPipePress; // oraz do ScndPipePress
            PantVolume = (PantPress + 1) * 0.1; // objêtoœæ, na wypadek odciêcia kurkiem
        }
        else
        { // zbiornik g³ówny odciêty, mo¿na pompowaæ pantografy
            if (PantCompFlag && Battery) // w³¹czona bateria i ma³a sprê¿arka
                PantVolume += dt * (TrainType == dt_EZT ? 0.003 : 0.005) *
                              (2 * 0.45 - ((0.1 / PantVolume / 10) - 0.1)) /
                              0.45; // nape³nianie zbiornika pantografów
            // Ra 2013-12: Niebugoc³aw mówi, ¿e w EZT nabija 1.5 raz wolniej ni¿ jak by³o 0.005
            PantPress = (10.0 * PantVolume) - 1.0; // tu by siê przyda³a objêtoœæ zbiornika
        }
        if (!PantCompFlag && (PantVolume > 0.1))
            PantVolume -= dt * 0.0003; // nieszczelnoœci: 0.0003=0.3l/s
        if (Mains) // nie wchodziæ w funkcjê bez potrzeby
            if (EngineType == ElectricSeriesMotor) // nie dotyczy... czego w³aœciwie?
                if (PantPress < EnginePowerSource.CollectorParameters.MinPress)
                    if ((TrainType & (dt_EZT | dt_ET40 | dt_ET41 | dt_ET42)) ?
                            (GetTrainsetVoltage() < EnginePowerSource.CollectorParameters.MinV) :
                            true) // to jest trochê proteza; zasilanie cz³onu mo¿e byæ przez sprzêg
                        // WN
                        if (MainSwitch(false))
                            EventFlag = true; // wywalenie szybkiego z powodu niskiego ciœnienia
        if (TrainType != dt_EZT) // w EN57 pompuje siê tylko w silnikowym
            // pierwotnie w CHK pantografy mia³y równie¿ rozrz¹dcze EZT
            for (int b = 0; b <= 1; ++b)
                if (TestFlag(Couplers[b].CouplingFlag, ctrain_controll))
                    if (Couplers[b].Connected->PantVolume <
                        PantVolume) // bo inaczej trzeba w obydwu cz³onach przestawiaæ
                        Couplers[b].Connected->PantVolume =
                            PantVolume; // przekazanie ciœnienia do s¹siedniego cz³onu
        // czy np. w ET40, ET41, ET42 pantografy cz³onów maj¹ po³¹czenie pneumatyczne?
        // Ra 2014-07: raczej nie - najpierw siê za³¹cza jeden cz³on, a potem mo¿na podnieœæ w
        // drugim
    }
    else
    { // a tu coœ dla SM42 i SM31, aby pokazywaæ na manometrze
        PantPress = CntrlPipePress;
    }
};

void TMoverParameters::UpdateBatteryVoltage(double dt)
{ // przeliczenie obci¹¿enia baterii
    double sn1, sn2, sn3, sn4, sn5; // Ra: zrobiæ z tego amperomierz NN
    if ((BatteryVoltage > 0) && (EngineType != DieselEngine) && (EngineType != WheelsDriven) &&
        (NominalBatteryVoltage > 0))
    {
        if ((NominalBatteryVoltage / BatteryVoltage < 1.22) && Battery)
        { // 110V
            if (!ConverterFlag)
                sn1 = (dt * 2.0); // szybki spadek do ok 90V
            else
                sn1 = 0;
            if (ConverterFlag)
                sn2 = -(dt * 2.0); // szybki wzrost do 110V
            else
                sn2 = 0;
            if (Mains)
                sn3 = (dt * 0.05);
            else
                sn3 = 0;
            if (iLights[0] & 63) // 64=blachy, nie ci¹gn¹ pr¹du //rozpisaæ na poszczególne
                // ¿arówki...
                sn4 = dt * 0.003;
            else
                sn4 = 0;
            if (iLights[1] & 63) // 64=blachy, nie ci¹gn¹ pr¹du
                sn5 = dt * 0.001;
            else
                sn5 = 0;
        };
        if ((NominalBatteryVoltage / BatteryVoltage >= 1.22) && Battery)
        { // 90V
            if (PantCompFlag)
                sn1 = (dt * 0.0046);
            else
                sn1 = 0;
            if (ConverterFlag)
                sn2 = -(dt * 50); // szybki wzrost do 110V
            else
                sn2 = 0;
            if (Mains)
                sn3 = (dt * 0.001);
            else
                sn3 = 0;
            if (iLights[0] & 63) // 64=blachy, nie ci¹gn¹ pr¹du
                sn4 = (dt * 0.0030);
            else
                sn4 = 0;
            if (iLights[1] & 63) // 64=blachy, nie ci¹gn¹ pr¹du
                sn5 = (dt * 0.0010);
            else
                sn5 = 0;
        };
        if (!Battery)
        {
            if (NominalBatteryVoltage / BatteryVoltage < 1.22)
                sn1 = dt * 50;
            else
                sn1 = 0;
            sn2 = dt * 0.000001;
            sn3 = dt * 0.000001;
            sn4 = dt * 0.000001;
            sn5 = dt * 0.000001; // bardzo powolny spadek przy wy³¹czonych bateriach
        };
        BatteryVoltage -= (sn1 + sn2 + sn3 + sn4 + sn5);
        if (NominalBatteryVoltage / BatteryVoltage > 1.57)
            if (MainSwitch(false) && (EngineType != DieselEngine) && (EngineType != WheelsDriven))
                EventFlag = true; // wywalanie szybkiego z powodu zbyt niskiego napiecia
        if (BatteryVoltage > NominalBatteryVoltage)
            BatteryVoltage = NominalBatteryVoltage; // wstrzymanie ³adowania pow. 110V
        if (BatteryVoltage < 0.01)
            BatteryVoltage = 0.01;
    }
    else if (NominalBatteryVoltage == 0)
        BatteryVoltage = 0;
    else
        BatteryVoltage = 90;
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

// *****************************************************************************
// Q: 20160714
// *****************************************************************************
double TMoverParameters::LocalBrakeRatio(void)
{
    double LBR;

    if (LocalBrakePosNo > 0)
        LBR = LocalBrakePos / LocalBrakePosNo;
    else
        LBR = 0;
    // if (TestFlag(BrakeStatus, b_antislip))
    //   LBR = Max0R(LBR, PipeRatio) + 0.4;
    return LBR;
}

// *****************************************************************************
// Q: 20160714
// *****************************************************************************
double TMoverParameters::ManualBrakeRatio(void)
{
    double MBR;

    if (ManualBrakePosNo > 0)
        MBR = ManualBrakePos / ManualBrakePosNo;
    else
        MBR = 0;
    return MBR;
}

// *****************************************************************************
// Q: 20160713
// *****************************************************************************
double TMoverParameters::BrakeVP(void)
{
    if (BrakeVVolume > 0)
        return Volume / (10.0 * BrakeVVolume);
    else
        return 0;
}

// *****************************************************************************
// Q: 20160713
// *****************************************************************************

double TMoverParameters::RealPipeRatio(void)
{
    double rpp;

    if (DeltaPipePress > 0)
        rpp = (CntrlPipePress - PipePress) / (DeltaPipePress);
    else
        rpp = 0;
    return rpp;
}

// *****************************************************************************
// Q: 20160713
// *****************************************************************************
double TMoverParameters::PipeRatio(void)
{
    double pr;

    if (DeltaPipePress > 0)
        if (false) // SPKS!!
        {
            if ((3 * PipePress) > (HighPipePress + LowPipePress + LowPipePress))
                pr = (HighPipePress - Min0R(HighPipePress, PipePress)) /
                     (DeltaPipePress * 4.0 / 3.0);
            else
                pr = (HighPipePress - 1.0 / 3.0 * DeltaPipePress - Max0R(LowPipePress, PipePress)) /
                     (DeltaPipePress * 2.0 / 3.0);
            //     if (not TestFlag(BrakeStatus,b_Ractive)) and (BrakeMethod and 1 = 0) and
            //     TestFlag(BrakeDelays,bdelay_R) and (Power<1) and (BrakeCtrlPos<1) then
            //      pr:=Min0R(0.5,pr);
            // if (Compressor>0.5) then
            // pr:=pr*1.333; //dziwny rapid wywalamy
        }
        else
            pr = (HighPipePress - Max0R(LowPipePress, Min0R(HighPipePress, PipePress))) /
                 DeltaPipePress;
    else
        pr = 0;
    return pr;
}

// *************************************************************************************************
// Q: 20160716
// *************************************************************************************************
void TMoverParameters::CollisionDetect(int CouplerN, double dt)
{
    double CCF, Vprev, VprevC;
    bool VirtualCoupling;

    CCF = 0;
    //   with Couplers[CouplerN] do
    if (Couplers[CouplerN].Connected != NULL)
    {
        VirtualCoupling = (Couplers[CouplerN].CouplingFlag == ctrain_virtual);
        Vprev = V;
        VprevC = Couplers[CouplerN].Connected->V;
        switch (CouplerN)
        {
        case 0:
            CCF =
                ComputeCollision(
                    V, Couplers[CouplerN].Connected->V, TotalMass,
                    Couplers[CouplerN].Connected->TotalMass,
                    (Couplers[CouplerN].beta +
                     Couplers[CouplerN].Connected->Couplers[Couplers[CouplerN].ConnectedNr].beta) /
                        2.0,
                    VirtualCoupling) /
                (dt);
            break; // yB: ej ej ej, a po
        case 1:
            CCF =
                ComputeCollision(
                    Couplers[CouplerN].Connected->V, V, Couplers[CouplerN].Connected->TotalMass,
                    TotalMass,
                    (Couplers[CouplerN].beta +
                     Couplers[CouplerN].Connected->Couplers[Couplers[CouplerN].ConnectedNr].beta) /
                        2.0,
                    VirtualCoupling) /
                (dt);
            break; // czemu tu jest +0.01??
        }
        AccS = AccS + (V - Vprev) / dt; // korekta przyspieszenia o si³y wynikaj¹ce ze zderzeñ?
        Couplers[CouplerN].Connected->AccS =
            Couplers[CouplerN].Connected->AccS + (Couplers[CouplerN].Connected->V - VprevC) / dt;
        if ((Couplers[CouplerN].Dist > 0) && (!VirtualCoupling))
            if (FuzzyLogic(abs(CCF), 5 * (Couplers[CouplerN].FmaxC + 1), p_coupldmg))
            { //! zerwanie sprzegu
                if (SetFlag(DamageFlag, dtrain_coupling))
                    EventFlag = true;

                if ((Couplers[CouplerN].CouplingFlag && ctrain_pneumatic > 0))
                    EmergencyBrakeFlag = true; // hamowanie nagle - zerwanie przewodow hamulcowych
                Couplers[CouplerN].CouplingFlag = 0;

                switch (CouplerN) // wyzerowanie flag podlaczenia ale ciagle sa wirtualnie polaczone
                {
                case 0:
                    Couplers[CouplerN].Connected->Couplers[1].CouplingFlag = 0;
                    break;
                case 1:
                    Couplers[CouplerN].Connected->Couplers[0].CouplingFlag = 0;
                    break;
                }
            }
    }
}

double TMoverParameters::ComputeMovement(double dt, double dt1, const TTrackShape &Shape,
                                         TTrackParam &Track, TTractionParam &ElectricTraction,
                                         const TLocation &NewLoc, TRotation &NewRot)
{
    const Vepsilon = 1e-5;
    const Aepsilon = 1e-3; // ASBSpeed=0.8;
    int b;
    double Vprev, AccSprev, d;

    // T_MoverParameters::ComputeMovement(dt, dt1, Shape, Track, ElectricTraction, NewLoc, NewRot);
    // // najpierw kawalek z funkcji w pliku mover.pas

    ClearPendingExceptions; // ma byc

    if (!TestFlag(DamageFlag, dtrain_out))
    { // Ra: to przepisywanie tu jest bez sensu
        RunningShape = Shape;
        RunningTrack = Track;
        RunningTraction = ElectricTraction;

        if (!DynamicBrakeFlag)
            RunningTraction.TractionVoltage = ElectricTraction.TractionVoltage -
                                              abs(ElectricTraction.TractionResistivity *
                                                  (Itot + HVCouplers[0][0] + HVCouplers[1][0]));
        else
            RunningTraction.TractionVoltage =
                ElectricTraction.TractionVoltage -
                abs(ElectricTraction.TractionResistivity * Itot *
                    0); // zasadniczo ED oporowe nie zmienia napiêcia w sieci
    }

    if (CategoryFlag == 4)
        OffsetTrackV = TotalMass / (Dim.L * Dim.W * 1000.0);
    else if (TestFlag(CategoryFlag, 1) && TestFlag(RunningTrack.CategoryFlag, 1))
        if (TestFlag(DamageFlag, dtrain_out))
        {
            OffsetTrackV = -0.2;
            OffsetTrackH = Sign(RunningShape.R) * 0.2;
        }

    Loc = NewLoc;
    Rot = NewRot;
    NewRot.Rx = 0;
    NewRot.Ry = 0;
    NewRot.Rz = 0;

    if (dL == 0) // oblicz przesuniecie}
    {
        Vprev = V;
        AccSprev = AccS;
        // dt:=ActualTime-LastUpdatedTime;                             //przyrost czasu
        // przyspieszenie styczne
        AccS = (FTotal / TotalMass + AccSprev) /
               2.0; // prawo Newtona ale z wygladzaniem (œrednia z poprzednim)

        if (TestFlag(DamageFlag, dtrain_out))
            AccS = -Sign(V) * g * 1; // random(0.0, 0.1)

        // przyspieszenie normalne
        if (abs(Shape.R) > 0.01)
            AccN = sqr(V) / Shape.R + g * Shape.dHrail / TrackW; // Q: zamieniam SQR() na sqr()
        else
            AccN = g * Shape.dHrail / TrackW;

        // szarpanie
        if (FuzzyLogic((10 + Track.DamageFlag) * Mass * Vel / Vmax, 500000,
                       p_accn)) // Ra: czemu tu masa bez ³adunku?
            AccV = sqrt((1 + Track.DamageFlag) * random(Trunc(50 * Mass / 1000000.0)) * Vel /
                        (Vmax * (10 + (Track.QualityFlag & 31)))); // TODO: Trunc na floor
        else
            AccV = AccV / 2.0;

        if (AccV > 1.0)
            AccN = AccN + (7 - random(2)) * (100.0 + Track.DamageFlag / 2.0) * AccV / 2000.0;

        // wykolejanie na luku oraz z braku szyn
        if (TestFlag(CategoryFlag, 1))
        {
            if (FuzzyLogic((AccN / g) * (1 + 0.1 * (Track.DamageFlag && dtrack_freerail)),
                           TrackW / Dim.H, 1) ||
                TestFlag(Track.DamageFlag, dtrack_norail))
                if (SetFlag(DamageFlag, dtrain_out))
                {
                    EventFlag = true;
                    Mains = false;
                    RunningShape.R = 0;
                    if (TestFlag(Track.DamageFlag, dtrack_norail))
                        DerailReason = 1; // Ra: powód wykolejenia: brak szyn
                    else
                        DerailReason = 2; // Ra: powód wykolejenia: przewrócony na ³uku
                }
            // wykolejanie na poszerzeniu toru
            if (FuzzyLogic(abs(Track.Width - TrackW), TrackW / 10.0, 1))
                if (SetFlag(DamageFlag, dtrain_out))
                {
                    EventFlag = true;
                    Mains = false;
                    RunningShape.R = 0;
                    DerailReason = 3; // Ra: powód wykolejenia: za szeroki tor
                }
        }
        // wykolejanie wkutek niezgodnosci kategorii toru i pojazdu
        if (!TestFlag(RunningTrack.CategoryFlag, CategoryFlag))
            if (SetFlag(DamageFlag, dtrain_out))
            {
                EventFlag = true;
                Mains = false;
                DerailReason = 4; // Ra: powód wykolejenia: nieodpowiednia trajektoria
            }

        V = V + (3 * AccS - AccSprev) * dt / 2.0; // przyrost predkosci

        if (TestFlag(DamageFlag, dtrain_out))
            if (Vel < 1)
            {
                V = 0;
                AccS = 0;
            }

        if ((V * Vprev <= 0) && (abs(FStand) > abs(FTrain))) // tlumienie predkosci przy hamowaniu
        { // zahamowany
            V = 0;
            // AccS:=0; //Ra 2014-03: ale si³a grawitacji dzia³a, wiêc nie mo¿e byæ zerowe
        }

        //   { dL:=(V+AccS*dt/2)*dt;                                      //przyrost dlugosci czyli
        //   przesuniecie
        dL = (3 * V - Vprev) * dt / 2.0; // metoda Adamsa-Bashfortha}
        // ale jesli jest kolizja (zas. zach. pedu) to...}
        for (b = 0; b < 1; b++)
            if (Couplers[b].CheckCollision)
                CollisionDetect(b, dt); // zmienia niejawnie AccS, V !!!

    } // liczone dL, predkosc i przyspieszenie

    if (Power > 1.0) // w rozrz¹dczym nie (jest b³¹d w FIZ!) - Ra 2014-07: teraz we wszystkich
        UpdatePantVolume(dt); // Ra 2014-07: obs³uga zbiornika rozrz¹du oraz pantografów

    if (EngineType == WheelsDriven)
        d = CabNo * dL; // na chwile dla testu
    else
        d = dL;
    DistCounter = DistCounter + fabs(dL) / 1000.0;
    dL = 0;

    // koniec procedury, tu nastepuja dodatkowe procedury pomocnicze

    // sprawdzanie i ewentualnie wykonywanie->kasowanie poleceñ
    if (LoadStatus > 0) // czas doliczamy tylko jeœli trwa (roz)³adowanie
        LastLoadChangeTime = LastLoadChangeTime + dt; // czas (roz)³adunku

    RunInternalCommand();

    // automatyczny rozruch
    if (EngineType == ElectricSeriesMotor)

        if (AutoRelayCheck())
            SetFlag(SoundFlag, sound_relay);

    if (EngineType == DieselEngine)
        if (dizel_Update(dt))
            SetFlag(SoundFlag, sound_relay);
    // uklady hamulcowe:
    if (VeselVolume > 0)
        Compressor = CompressedVolume / VeselVolume;
    else
    {
        Compressor = 0;
        CompressorFlag = false;
    };
    ConverterCheck();
    if (CompressorSpeed > 0.0) // sprê¿arka musi mieæ jak¹œ niezerow¹ wydajnoœæ
        CompressorCheck(dt); //¿eby rozwa¿aæ jej za³¹czenie i pracê
    UpdateBrakePressure(dt);
    UpdatePipePressure(dt);
    UpdateBatteryVoltage(dt);
    UpdateScndPipePressure(dt); // druga rurka, youBy
    // hamulec antypoœlizgowy - wy³¹czanie
    if ((BrakeSlippingTimer > 0.8) && (ASBType != 128)) // ASBSpeed=0.8
        Hamulec->ASB(0);
    // SetFlag(BrakeStatus,-b_antislip);
    BrakeSlippingTimer = BrakeSlippingTimer + dt;
    // sypanie piasku - wy³¹czone i piasek siê nie koñczy - b³êdy AI
    // if AIControllFlag then
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
    // czuwak/SHP
    // if (Vel>10) and (not DebugmodeFlag) then
    if (!DebugModeFlag)
        SecuritySystemCheck(dt1);
    return d;
};

double TMoverParameters::FastComputeMovement(double dt, const TTrackShape &Shape,
                                             TTrackParam &Track, const TLocation &NewLoc,
                                             TRotation &NewRot)
{ // trzeba po ma³u przenosiæ tu tê funkcjê

    double Vprev, AccSprev, d;
    int b;
    // T_MoverParameters::FastComputeMovement(dt, Shape, Track, NewLoc, NewRot);

    ClearPendingExceptions; // nie bylo

    Loc = NewLoc;
    Rot = NewRot;
    NewRot.Rx = 0;
    NewRot.Ry = 0;
    NewRot.Rz = 0;

    if (dL == 0) // oblicz przesuniecie
    {
        Vprev = V;
        AccSprev = AccS;
        // dt =ActualTime-LastUpdatedTime;                               //przyrost czasu
        // przyspieszenie styczne
        AccS = (FTotal / TotalMass + AccSprev) /
               2.0; // prawo Newtona ale z wygladzaniem (œrednia z poprzednim)

        if (TestFlag(DamageFlag, dtrain_out))
            AccS = -Sign(V) * g * 1; // * random(0.0, 0.1)
        // przyspieszenie normalne}
        //     if Abs(Shape.R)>0.01 then
        //      AccN:=SQR(V)/Shape.R+g*Shape.dHrail/TrackW
        //     else AccN:=g*Shape.dHrail/TrackW;

        // szarpanie}
        if (FuzzyLogic((10 + Track.DamageFlag) * Mass * Vel / Vmax, 500000, p_accn))
        {
            AccV = sqrt((1 + Track.DamageFlag) * random(int(floor(50 * Mass / 1000000.0))) * Vel /
                        (Vmax * (10 + (Track.QualityFlag & 31)))); // Trunc na floor, czy dobrze?
        }
        else
            AccV = AccV / 2.0;

        if (AccV > 1.0)
            AccN = AccN + (7 - random(2)) * (100.0 + Track.DamageFlag / 2.0) * AccV / 2000.0;

        //     {wykolejanie na luku oraz z braku szyn}
        //     if TestFlag(CategoryFlag,1) then
        //      begin
        //       if FuzzyLogic((AccN/g)*(1+0.1*(Track.DamageFlag and
        //       dtrack_freerail)),TrackW/Dim.H,1)
        //       or TestFlag(Track.DamageFlag,dtrack_norail) then
        //        if SetFlag(DamageFlag,dtrain_out) then
        //         begin
        //           EventFlag:=true;
        //           MainS:=false;
        //           RunningShape.R:=0;
        //         end;
        //     {wykolejanie na poszerzeniu toru}
        //        if FuzzyLogic(Abs(Track.Width-TrackW),TrackW/10,1) then
        //         if SetFlag(DamageFlag,dtrain_out) then
        //          begin
        //            EventFlag:=true;
        //            MainS:=false;
        //            RunningShape.R:=0;
        //          end;
        //      end;
        //     {wykolejanie wkutek niezgodnosci kategorii toru i pojazdu}
        //     if not TestFlag(RunningTrack.CategoryFlag,CategoryFlag) then
        //      if SetFlag(DamageFlag,dtrain_out) then
        //        begin
        //          EventFlag:=true;
        //          MainS:=false;
        //        end;

        V = V + (3.0 * AccS - AccSprev) * dt / 2.0; // przyrost predkosci

        if (TestFlag(DamageFlag, dtrain_out))
            if (Vel < 1)
            {
                V = 0;
                AccS = 0; // Ra 2014-03: ale si³a grawitacji dzia³a, wiêc nie mo¿e byæ zerowe
            }

        if ((V * Vprev <= 0) && (abs(FStand) > abs(FTrain))) // tlumienie predkosci przy hamowaniu
        { // zahamowany}
            V = 0;
            AccS = 0;
        }
        dL = (3 * V - Vprev) * dt / 2.0; // metoda Adamsa-Bashfortha
        // ale jesli jest kolizja (zas. zach. pedu) to...
        for (b = 0; b < 1; b++)
            if (Couplers[b].CheckCollision)
                CollisionDetect(b, dt); // zmienia niejawnie AccS, V !!!
    } // liczone dL, predkosc i przyspieszenie
    // QQQ

    if (Power > 1.0) // w rozrz¹dczym nie (jest b³¹d w FIZ!)
        UpdatePantVolume(dt); // Ra 2014-07: obs³uga zbiornika rozrz¹du oraz pantografów
    if (EngineType == WheelsDriven)
        d = CabNo * dL; // na chwile dla testu
    else
        d = dL;
    DistCounter = DistCounter + fabs(dL) / 1000.0;
    dL = 0;

    // koniec procedury, tu nastepuja dodatkowe procedury pomocnicze

    // sprawdzanie i ewentualnie wykonywanie->kasowanie poleceñ
    if (LoadStatus > 0) // czas doliczamy tylko jeœli trwa (roz)³adowanie
        LastLoadChangeTime = LastLoadChangeTime + dt; // czas (roz)³adunku

    RunInternalCommand();

    if (EngineType == DieselEngine)
        if (dizel_Update(dt))
            SetFlag(SoundFlag, sound_relay);
    // uklady hamulcowe:
    if (VeselVolume > 0)
        Compressor = CompressedVolume / VeselVolume;
    else
    {
        Compressor = 0;
        CompressorFlag = false;
    };
    ConverterCheck();
    if (CompressorSpeed > 0.0) // sprê¿arka musi mieæ jak¹œ niezerow¹ wydajnoœæ
        CompressorCheck(dt); //¿eby rozwa¿aæ jej za³¹czenie i pracê
    UpdateBrakePressure(dt);
    UpdatePipePressure(dt);
    UpdateScndPipePressure(dt); // druga rurka, youBy
    UpdateBatteryVoltage(dt);
    // hamulec antyposlizgowy - wy³¹czanie
    if ((BrakeSlippingTimer > 0.8) && (ASBType != 128)) // ASBSpeed=0.8
        Hamulec->ASB(0);
    BrakeSlippingTimer = BrakeSlippingTimer + dt;
    return d;
};

double TMoverParameters::ShowEngineRotation(int VehN)
{ // pokazywanie obrotów silnika, równie¿ dwóch dalszych pojazdów (3×SN61)
    int b;
    switch (VehN)
    { // numer obrotomierza
    case 1:
        return fabs(enrot);
    case 2:
        for (b = 0; b <= 1; ++b)
            if (TestFlag(Couplers[b].CouplingFlag, ctrain_controll))
                if (Couplers[b].Connected->Power > 0.01)
                    return fabs(Couplers[b].Connected->enrot);
        break;
    case 3: // to nie uwzglêdnia ewentualnego odwrócenia pojazdu w œrodku
        for (b = 0; b <= 1; ++b)
            if (TestFlag(Couplers[b].CouplingFlag, ctrain_controll))
                if (Couplers[b].Connected->Power > 0.01)
                    if (TestFlag(Couplers[b].Connected->Couplers[b].CouplingFlag, ctrain_controll))
                        if (Couplers[b].Connected->Couplers[b].Connected->Power > 0.01)
                            return fabs(Couplers[b].Connected->Couplers[b].Connected->enrot);
        break;
    };
    return 0.0;
};

void TMoverParameters::ConverterCheck()
{ // sprawdzanie przetwornicy
    if (ConverterAllow && Mains)
        ConverterFlag = true;
    else
        ConverterFlag = false;
};

int TMoverParameters::ShowCurrent(int AmpN)
{ // odczyt ampera¿u
    switch (EngineType)
    {
    case ElectricInductionMotor:
        switch (AmpN)
        { // do asynchronicznych
        case 1:
            return WindingRes * Mm / Vadd;
        case 2:
            return dizel_fill * WindingRes;
        default:
            return ShowCurrentP(AmpN); // T_MoverParameters::
        }
        break;
    case DieselElectric:
        return fabs(Im);
        break;
    default:
        return ShowCurrentP(AmpN); // T_MoverParameters::
    }
};

// *************************************************************************************************
// queuedEU
// *************************************************************************************************

// *************************************************************************************************
// Q: 20160710
// *************************************************************************************************

bool TMoverParameters::IncMainCtrl(int CtrlSpeed)
{
    bool OK = false;
    if ((MainCtrlPosNo > 0) && (CabNo != 0))
    {
        if (MainCtrlPos < MainCtrlPosNo)
        {
            if ((TrainType != dt_ET22) ||
                ((TrainType == dt_ET22) &&
                 (ScndCtrlPos ==
                  0))) // w ET22 nie da siê krêciæ nastawnikiem przy w³¹czonym boczniku
                switch (EngineType)
                {
                case None:
                case Dumb:
                case DieselElectric:
                case ElectricInductionMotor:
                {
                    if (((CtrlSpeed == 1) && (TrainType != dt_EZT)) ||
                        ((CtrlSpeed == 1) && (TrainType == dt_EZT) && (ActiveDir != 0)))
                    { // w EZT nie da siê za³¹czyæ pozycji bez ustawienia kierunku
                        MainCtrlPos++;
                        OK = true;
                    }
                    else if (((CtrlSpeed > 1) && (TrainType != dt_EZT)) ||
                             ((CtrlSpeed > 1) && (TrainType == dt_EZT) && (ActiveDir != 0)))
                        OK = (IncMainCtrl(1) && IncMainCtrl(CtrlSpeed - 1));
                    break;
                };

                case ElectricSeriesMotor:
                {
                    if ((CtrlSpeed == 1) && (ActiveDir != 0))
                    {
                        MainCtrlPos++;
                        OK = true;
                        if (Imax == ImaxHi)
                            if (RList[MainCtrlPos].Bn > 1)
                            {
                                if (TrainType == dt_ET42)
                                {
                                    MainCtrlPos--;
                                    OK = false;
                                }
                                if (MaxCurrentSwitch(false))
                                    SetFlag(
                                        SoundFlag,
                                        sound_relay); // wylaczanie wysokiego rozruchu    // Q TODO:
                                //         if (EngineType=ElectricSeriesMotor) and (MainCtrlPos=1)
                                //         then
                                //           MainCtrlActualPos:=1;
                                //
                            }
                        if ((CtrlSpeed == 1) && (ActiveDir == -1) && (RList[MainCtrlPos].Bn > 1) &&
                            (TrainType != dt_PseudoDiesel))
                        { // blokada wejœcia na równoleg³¹ podczas jazdy do ty³u
                            MainCtrlPos--;
                            OK = false;
                        }
                        //
                        // if (TrainType == "et40")
                        //  if (Abs(Im) > IminHi)
                        //   {
                        //    MainCtrlPos--; //Blokada nastawnika po przekroczeniu minimalnego pradu
                        //    OK = false;
                        //   }
                        //}
                        if (DynamicBrakeFlag)
                            if (TrainType == dt_ET42)
                                if (MainCtrlPos > 20)
                                {
                                    MainCtrlPos--;
                                    OK = false;
                                }
                    }
                    else if ((CtrlSpeed > 1) && (ActiveDir != 0) && (TrainType != dt_ET40))
                    { // szybkie przejœcie na bezoporow¹

                        while ((RList[MainCtrlPos].R > 0) && IncMainCtrl(1))

                            // OK:=true ; {takie chamskie, potem poprawie} <-Ra: kto mia³ to
                            // poprawiæ i po co?
                            if (ActiveDir == -1)
                                while ((RList[MainCtrlPos].Bn > 1) && IncMainCtrl(1))
                                    MainCtrlPos--;
                        OK = false;
                        // if (TrainType=dt_ET40)  then
                        // while Abs (Im)>IminHi do
                        //   dec(MainCtrlPos);
                        //  OK:=false ;
                        //
                        if (DynamicBrakeFlag)
                            if (TrainType == dt_ET42)
                                while (MainCtrlPos > 20)
                                    MainCtrlPos--;
                        OK = false;
                    }
                    // return OK;

                    break;
                };

                case DieselEngine:
                {
                    if (CtrlSpeed == 1)
                    {
                        MainCtrlPos++;
                        OK = true;
                        if (MainCtrlPos > 0)
                            CompressorAllow = true;
                        else
                            CompressorAllow = false;
                    }
                    else if (CtrlSpeed > 1)
                    {
                        while (MainCtrlPos < MainCtrlPosNo)
                            IncMainCtrl(1);
                        OK = true;
                    }
                    break;
                };

                case WheelsDriven:
                {
                    OK = AddPulseForce(CtrlSpeed);
                    break;
                };

                } // switch EngineType of
        }
        else // MainCtrlPos>=MainCtrlPosNo
            if (CoupledCtrl) // wspólny wa³ nastawnika jazdy i bocznikowania
        {
            if (ScndCtrlPos < ScndCtrlPosNo) // 3<3 -> false
            {
                ScndCtrlPos++;
                OK = true;
            }
            else
                OK = false;
        }
        if (OK)
        {
            SendCtrlToNext("MainCtrl", MainCtrlPos, CabNo); //???
            SendCtrlToNext("ScndCtrl", ScndCtrlPos, CabNo);
        }
    }
    else // nie ma sterowania
        OK = false;
    // if OK then LastRelayTime:=0;

    // hunter-101012: poprawka
    // poprzedni warunek byl niezbyt dobry, bo przez to przy trzymaniu +
    // styczniki tkwily na tej samej pozycji (LastRelayTime byl caly czas 0 i rosl
    // po puszczeniu plusa)

    if (OK)
    {
        if (DelayCtrlFlag)
        {
            if ((LastRelayTime >= InitialCtrlDelay) && (MainCtrlPos == 1))
                LastRelayTime = 0;
        }
        else if (LastRelayTime > CtrlDelay)
            LastRelayTime = 0;
    }
    return OK;
}

// *****************************************************************************
// Q: 20160710
// *****************************************************************************
bool TMoverParameters::DecMainCtrl(int CtrlSpeed)
{
    bool OK = false;
    if ((MainCtrlPosNo > 0) && (CabNo != 0))
    {
        if (MainCtrlPos > 0)
        {
            if ((TrainType != dt_ET22) ||
                (ScndCtrlPos == 0)) // Ra: ET22 blokuje nastawnik przy boczniku
            {
                if (CoupledCtrl && (ScndCtrlPos > 0))
                {
                    ScndCtrlPos--; // wspolny wal
                    OK = true;
                }
                else
                    switch (EngineType)
                    {
                    case None:
                    case Dumb:
                    case DieselElectric:
                    case ElectricInductionMotor:
                    {
                        if (((CtrlSpeed == 1) &&
                             /*(ScndCtrlPos==0) and*/ (EngineType != DieselElectric)) ||
                            ((CtrlSpeed == 1) && (EngineType == DieselElectric)))
                        {
                            MainCtrlPos--;
                            OK = true;
                        }
                        else if (CtrlSpeed > 1)
                            OK = (DecMainCtrl(1) && DecMainCtrl(2)); // CtrlSpeed-1);
                        break;
                    }

                    case ElectricSeriesMotor:
                    {
                        if (CtrlSpeed == 1) /*and (ScndCtrlPos=0)*/
                        {
                            MainCtrlPos--;
                            //                if (MainCtrlPos=0) and (ScndCtrlPos=0) and
                            //                (TrainType<>dt_ET40)and(TrainType<>dt_EP05) then
                            //                 StLinFlag:=false;
                            //                if (MainCtrlPos=0) and (TrainType<>dt_ET40) and
                            //                (TrainType<>dt_EP05) then
                            //                 MainCtrlActualPos:=0; //yBARC: co to tutaj robi? ;)
                            OK = true;
                        }
                        else if (CtrlSpeed > 1) /*and (ScndCtrlPos=0)*/
                        {
                            OK = true;
                            if (RList[MainCtrlPos].R == 0) // Q: tu zrobilem = ;]
                                DecMainCtrl(1);
                            while ((RList[MainCtrlPos].R > 0) && DecMainCtrl(1))
                                ; // takie chamskie, potem poprawie}
                        }
                        break;
                    }

                    case DieselEngine:
                    {
                        if (CtrlSpeed == 1)
                        {
                            MainCtrlPos--;
                            OK = true;
                        }
                        else if (CtrlSpeed > 1)
                        {
                            while ((MainCtrlPos > 0) || (RList[MainCtrlPos].Mn > 0))
                                DecMainCtrl(1);
                            OK = true;
                        }
                        break;
                    }
                    } // switch EngineType
            }
        }
        else if (EngineType == WheelsDriven)
            OK = AddPulseForce(-CtrlSpeed);
        else
            OK = false;

        if (OK)
        {
            /*OK:=*/SendCtrlToNext("MainCtrl", MainCtrlPos, CabNo); // hmmmm...???!!!
            /*OK:=*/SendCtrlToNext("ScndCtrl", ScndCtrlPos, CabNo);
        }
    }
    else
        OK = false;
    // if OK then LastRelayTime:=0;
    // hunter-101012: poprawka
    if (OK)
    {
        if (DelayCtrlFlag)
        {
            if (LastRelayTime >= InitialCtrlDelay)
                LastRelayTime = 0;
        }
        else if (LastRelayTime > CtrlDownDelay)
            LastRelayTime = 0;
    }
    return OK;
}

// *************************************************************************************************
// Q: 20160710
// *************************************************************************************************
bool TMoverParameters::IncScndCtrl(int CtrlSpeed)
{
    bool OK = false;

    if ((MainCtrlPos == 0) && (CabNo != 0) && (TrainType == dt_ET42) && (ScndCtrlPos == 0) &&
        (DynamicBrakeFlag))
    {
        OK = DynamicBrakeSwitch(false);
    }
    else if ((ScndCtrlPosNo > 0) && (CabNo != 0) &&
             !((TrainType == dt_ET42) &&
               ((Imax == ImaxHi) || ((DynamicBrakeFlag) && (MainCtrlPos > 0)))))
    {
        //     if (RList[MainCtrlPos].R=0) and (MainCtrlPos>0) and (ScndCtrlPos<ScndCtrlPosNo) and
        //     (not CoupledCtrl) then
        if ((ScndCtrlPos < ScndCtrlPosNo) && (!CoupledCtrl) &&
            ((EngineType != DieselElectric) || (!AutoRelayFlag)))
        {
            if (CtrlSpeed == 1)
            {
                ScndCtrlPos++;
            }
            else if (CtrlSpeed > 1)
            {
                ScndCtrlPos = ScndCtrlPosNo; // takie chamskie, potem poprawie
            }
            OK = true;
        }
        else // nie mozna zmienic
            OK = false;
        if (OK)
        {
            /*OK:=*/SendCtrlToNext("MainCtrl", MainCtrlPos, CabNo); //???
            /*OK:=*/SendCtrlToNext("ScndCtrl", ScndCtrlPos, CabNo);
        }
    }
    else // nie ma sterowania
        OK = false;
    // if OK then LastRelayTime:=0;
    // hunter-101012: poprawka
    if (OK)
        if (LastRelayTime > CtrlDelay)
            LastRelayTime = 0;

    return OK;
}

// *************************************************************************************************
// Q: 20160710
// *************************************************************************************************
bool TMoverParameters::DecScndCtrl(int CtrlSpeed)
{
    bool OK = false;

    if ((MainCtrlPos == 0) && (CabNo != 0) && (TrainType == dt_ET42) && (ScndCtrlPos == 0) &&
        !(DynamicBrakeFlag) && (CtrlSpeed == 1))
    {
        // Ra: AI wywo³uje z CtrlSpeed=2 albo gdy ScndCtrlPos>0
        OK = DynamicBrakeSwitch(true);
    }
    else if ((ScndCtrlPosNo > 0) && (CabNo != 0))
    {
        if ((ScndCtrlPos > 0) && (!CoupledCtrl) &&
            ((EngineType != DieselElectric) || (!AutoRelayFlag)))
        {
            if (CtrlSpeed == 1)
            {
                ScndCtrlPos--;
            }
            else if (CtrlSpeed > 1)
            {
                ScndCtrlPos = 0; // takie chamskie, potem poprawie
            }
            OK = true;
        }
        else
            OK = false;
        if (OK)
        {
            /*OK:=*/SendCtrlToNext("MainCtrl", MainCtrlPos, CabNo); //???
            /*OK:=*/SendCtrlToNext("ScndCtrl", ScndCtrlPos, CabNo);
        }
    }
    else
        OK = false;
    // if OK then LastRelayTime:=0;
    // hunter-101012: poprawka
    if (OK)
        if (LastRelayTime > CtrlDownDelay)
            LastRelayTime = 0;

    return OK;
}

// *************************************************************************************************
// Q: 20160710
// *************************************************************************************************
bool TMoverParameters::CabActivisation(void) // za³¹czenie rozrz¹du
{
    bool OK = false;

    OK = (CabNo == 0); // numer kabiny, z której jest sterowanie
    if (OK)
    {
        CabNo = ActiveCab; // sterowanie jest z kabiny z obsad¹
        DirAbsolute = ActiveDir * CabNo;
        SendCtrlToNext("CabActivisation", 1, CabNo);
    }
    return OK;
}

// *************************************************************************************************
// Q: 20160710
// *************************************************************************************************
bool TMoverParameters::CabDeactivisation(void) // wy³¹czenie rozrz¹du
{
    bool OK = false;

    OK = (CabNo == ActiveCab); // o ile obsada jest w kabinie ze sterowaniem
    if (OK)
    {
        CabNo = 0;
        DirAbsolute = ActiveDir * CabNo;
        DepartureSignal = false; // nie buczeæ z nieaktywnej kabiny
        SendCtrlToNext("CabActivisation", 0, ActiveCab); // CabNo==0!
    }
    return OK;
}

// *************************************************************************************************
// Q: 20160710
// *************************************************************************************************
bool TMoverParameters::AddPulseForce(int Multipler)
{
    bool APF;
    if ((EngineType == WheelsDriven) && (EnginePowerSource.SourceType == InternalSource) &&
        (EnginePowerSource.PowerType == BioPower))
    {
        ActiveDir = CabNo;
        DirAbsolute = ActiveDir * CabNo;
        if (Vel > 0)
            PulseForce = Min0R(1000.0 * Power / (abs(V) + 0.1), Ftmax);
        else
            PulseForce = Ftmax;
        if (PulseForceCount > 1000.0)
            PulseForce = 0;
        else
            PulseForce = PulseForce * Multipler;
        PulseForceCount = PulseForceCount + abs(Multipler);
        APF = (PulseForce > 0);
    }
    else
        APF = false;
    return APF;
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
bool TMoverParameters::SandDoseOn(void)
{
    bool SDO;
    if (SandCapacity > 0)
    {
        SDO = true;
        if (SandDose)
            SandDose = false;
        else if (Sand > 0)
            SandDose = true;
        if (CabNo != 0)
            SendCtrlToNext("SandDoseOn", 1, CabNo);
    }
    else
        SDO = false;

    return SDO;
}

void TMoverParameters::SSReset(void)
{
    SecuritySystem.SystemTimer = 0;

    if (TestFlag(SecuritySystem.Status, s_aware))
    {
        SecuritySystem.SystemBrakeCATimer = 0;
        SecuritySystem.SystemSoundCATimer = 0;
        SetFlag(SecuritySystem.Status, -s_aware);
        SetFlag(SecuritySystem.Status, -s_CAalarm);
        SetFlag(SecuritySystem.Status, -s_CAebrake);
        //   EmergencyBrakeFlag = false; //YB-HN
        SecuritySystem.VelocityAllowed = -1;
    }
    else if (TestFlag(SecuritySystem.Status, s_active))
    {
        SecuritySystem.SystemBrakeSHPTimer = 0;
        SecuritySystem.SystemSoundSHPTimer = 0;
        SetFlag(SecuritySystem.Status, -s_active);
        SetFlag(SecuritySystem.Status, -s_SHPalarm);
        SetFlag(SecuritySystem.Status, -s_SHPebrake);
        //   EmergencyBrakeFlag = false; //YB-HN
        SecuritySystem.VelocityAllowed = -1;
    }
}

// *****************************************************************************
// Q: 20160710
// *****************************************************************************
// hunter-091012: rozbicie alarmow, dodanie testu czuwaka
bool TMoverParameters::SecuritySystemReset(void) // zbijanie czuwaka/SHP
{
    // zbijanie czuwaka/SHP
    bool SSR = false;
    // with SecuritySystem do
    if ((SecuritySystem.SystemType > 0) && (SecuritySystem.Status > 0))
    {
        SSR = true;
        if ((TrainType == dt_EZT) ||
            (ActiveDir != 0)) // Ra 2014-03: w EZT nie trzeba ustawiaæ kierunku
            if (!TestFlag(SecuritySystem.Status, s_CAebrake) ||
                !TestFlag(SecuritySystem.Status, s_SHPebrake))
                SSReset();
        // else
        //  if EmergencyBrakeSwitch(false) then
        //   Reset;
    }
    else
        SSR = false;
    //  SendCtrlToNext('SecurityReset',0,CabNo);
    return SSR;
}

// *************************************************************************************************
// Q: 20160711
// *************************************************************************************************
void TMoverParameters::SecuritySystemCheck(double dt)
{
    // Ra: z CA/SHP w EZT jest ten problem, ¿e w rozrz¹dczym nie ma kierunku, a w silnikowym nie ma
    // obsady
    // poza tym jest zdefiniowany we wszystkich 3 cz³onach EN57
    // SecuritySystem SS;

    if ((SecuritySystem.SystemType > 0) && (SecuritySystem.Status > 0) &&
        (Battery)) // Ra: EZT ma teraz czuwak w rozrz¹dczym
    {
        // CA
        if (Vel >=
            SecuritySystem
                .AwareMinSpeed) // domyœlnie predkoœæ wiêksza od 10% Vmax, albo podanej jawnie w FIZ
        {
            SecuritySystem.SystemTimer = SecuritySystem.SystemTimer + dt;
            if (TestFlag(SecuritySystem.SystemType, 1) &&
                TestFlag(SecuritySystem.Status, s_aware)) // jeœli œwieci albo miga
                SecuritySystem.SystemSoundCATimer = SecuritySystem.SystemSoundCATimer + dt;
            if (TestFlag(SecuritySystem.SystemType, 1) &&
                TestFlag(SecuritySystem.Status, s_CAalarm)) // jeœli buczy
                SecuritySystem.SystemBrakeCATimer = SecuritySystem.SystemBrakeCATimer + dt;
            if (TestFlag(SecuritySystem.SystemType, 1))
                if ((SecuritySystem.SystemTimer > SecuritySystem.AwareDelay) &&
                    (SecuritySystem.AwareDelay >= 0)) //-1 blokuje
                    if (!SetFlag(SecuritySystem.Status, s_aware)) // juz wlaczony sygnal swietlny
                        if ((SecuritySystem.SystemSoundCATimer > SecuritySystem.SoundSignalDelay) &&
                            (SecuritySystem.SoundSignalDelay >= 0))
                            if (!SetFlag(SecuritySystem.Status,
                                         s_CAalarm)) // juz wlaczony sygnal dzwiekowy
                                if ((SecuritySystem.SystemBrakeCATimer >
                                     SecuritySystem.EmergencyBrakeDelay) &&
                                    (SecuritySystem.EmergencyBrakeDelay >= 0))
                                    SetFlag(SecuritySystem.Status, s_CAebrake);

            // SHP
            if (TestFlag(SecuritySystem.SystemType, 2) &&
                TestFlag(SecuritySystem.Status, s_active)) // jeœli œwieci albo miga
                SecuritySystem.SystemSoundSHPTimer = SecuritySystem.SystemSoundSHPTimer + dt;
            if (TestFlag(SecuritySystem.SystemType, 2) &&
                TestFlag(SecuritySystem.Status, s_SHPalarm)) // jeœli buczy
                SecuritySystem.SystemBrakeSHPTimer = SecuritySystem.SystemBrakeSHPTimer + dt;
            if (TestFlag(SecuritySystem.SystemType, 2) && TestFlag(SecuritySystem.Status, s_active))
                if ((Vel > SecuritySystem.VelocityAllowed) && (SecuritySystem.VelocityAllowed >= 0))
                    SetFlag(SecuritySystem.Status, s_SHPebrake);
                else if (((SecuritySystem.SystemSoundSHPTimer > SecuritySystem.SoundSignalDelay) &&
                          (SecuritySystem.SoundSignalDelay >= 0)) ||
                         ((Vel > SecuritySystem.NextVelocityAllowed) &&
                          (SecuritySystem.NextVelocityAllowed >= 0)))
                    if (!SetFlag(SecuritySystem.Status,
                                 s_SHPalarm)) // juz wlaczony sygnal dzwiekowy}
                        if ((SecuritySystem.SystemBrakeSHPTimer >
                             SecuritySystem.EmergencyBrakeDelay) &&
                            (SecuritySystem.EmergencyBrakeDelay >= 0))
                            SetFlag(SecuritySystem.Status, s_SHPebrake);

        } // else SystemTimer:=0;

        // TEST CA
        if (TestFlag(SecuritySystem.Status, s_CAtest)) // jeœli œwieci albo miga
            SecuritySystem.SystemBrakeCATestTimer = SecuritySystem.SystemBrakeCATestTimer + dt;
        if (TestFlag(SecuritySystem.SystemType, 1))
            if (TestFlag(SecuritySystem.Status, s_CAtest)) // juz wlaczony sygnal swietlny
                if ((SecuritySystem.SystemBrakeCATestTimer > SecuritySystem.EmergencyBrakeDelay) &&
                    (SecuritySystem.EmergencyBrakeDelay >= 0))
                    s_CAtestebrake = true;

        // wdrazanie hamowania naglego
        //        if TestFlag(Status,s_SHPebrake) or TestFlag(Status,s_CAebrake) or
        //        (s_CAtestebrake=true) then
        //         EmergencyBrakeFlag:=true;  //YB-HN
    }
    else if (!Battery)
    { // wy³¹czenie baterii deaktywuje sprzêt
        // SecuritySystem.Status = 0; //deaktywacja czuwaka
    }
}

// *************************************************************************************************
// Q: 20160710
// *************************************************************************************************
bool TMoverParameters::BatterySwitch(bool State)
{
    bool BS = false;
    // Ra: ukrotnienie za³¹czania baterii jest jak¹œ fikcj¹...
    if (Battery != State)
    {
        Battery = State;
    }
    if (Battery == true)
        SendCtrlToNext("BatterySwitch", 1, CabNo);
    else
        SendCtrlToNext("BatterySwitch", 0, CabNo);
    BS = true;

    if ((Battery) && (ActiveCab != 0)) /*|| (TrainType==dt_EZT)*/
        SecuritySystem.Status = (SecuritySystem.Status || s_waiting); // aktywacja czuwaka
    else
        SecuritySystem.Status = 0; // wy³¹czenie czuwaka

    return BS;
}

// *************************************************************************************************
// Q: 20160710
// *************************************************************************************************
bool TMoverParameters::EpFuseSwitch(bool State)
{
    if (EpFuse != State)
    {
        EpFuse = State;
        return true;
    }
    else
        return false;
    // if (EpFuse == true) SendCtrlToNext("EpFuseSwitch", 1, CabNo)
    //  else SendCtrlToNext("EpFuseSwitch", 0, CabNo);
}

// *************************************************************************************************
// Q: 20160710
// *************************************************************************************************
bool TMoverParameters::DirectionBackward(void)
{
    bool DB = false;
    if ((ActiveDir == 1) && (MainCtrlPos == 0) && (TrainType == dt_EZT))
        if (MinCurrentSwitch(false))
        {
            DB = true; //
            return DB; // exit;  TODO: czy dobrze przetlumaczone?
        }
    if ((MainCtrlPosNo > 0) && (ActiveDir > -1) && (MainCtrlPos == 0))
    {
        if (EngineType == WheelsDriven)
            CabNo--;
        //    else
        ActiveDir--;
        DirAbsolute = ActiveDir * CabNo;
        if (DirAbsolute != 0)
            if (Battery) // jeœli bateria jest ju¿ za³¹czona
                BatterySwitch(true); // to w ten oto durny sposób aktywuje siê CA/SHP
        DB = true;
        SendCtrlToNext("Direction", ActiveDir, CabNo);
    }
    else
        DB = false;
    return DB;
}

// *************************************************************************************************
// Q: 20160710
// *************************************************************************************************
bool TMoverParameters::AntiSlippingButton(void)
{
    bool OK = false;

    OK = SandDoseOn();
    return (AntiSlippingBrake() || OK);
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
bool TMoverParameters::MainSwitch(bool State)
{
    bool MS = false;

    MS = false; // Ra: przeniesione z koñca
    if ((Mains != State) && (MainCtrlPosNo > 0))
    {
        if ((State == false) ||
            ((ScndCtrlPos == 0) && (ConvOvldFlag == false) && (LastSwitchingTime > CtrlDelay) &&
             !TestFlag(DamageFlag, dtrain_out)))
        {
            if (Mains) // jeœli by³ za³¹czony
                SendCtrlToNext("MainSwitch", int(State),
                               CabNo); // wys³anie wy³¹czenia do pozosta³ych?
            Mains = State;
            if (Mains) // jeœli zosta³ za³¹czony
                SendCtrlToNext("MainSwitch", int(State),
                               CabNo); // wyslanie po wlaczeniu albo przed wylaczeniem
            MS = true; // wartoœæ zwrotna
            LastSwitchingTime = 0;
            if ((EngineType == DieselEngine) && Mains)
            {
                dizel_enginestart = State;
            }
            // if (State=false) then //jeœli wy³¹czony
            // begin
            // SetFlag(SoundFlag,sound_relay); //hunter-091012: przeniesione do Train.cpp, zeby sie
            // nie zapetlal
            // if (SecuritySystem.Status<>12) then
            //  SecuritySystem.Status:=0; //deaktywacja czuwaka; Ra: a nie bateri¹?
            // end
            // else
            // if (SecuritySystem.Status<>12) then
            // SecuritySystem.Status:=s_waiting; //aktywacja czuwaka
        }
    }
    // else MainSwitch:=false;
    return MS;
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
bool TMoverParameters::ConverterSwitch(bool State)
{
    bool CS = false; // Ra: normalnie chyba false?
    if (ConverterAllow != State)
    {
        ConverterAllow = State;
        CS = true;
        if (CompressorPower == 2)
            CompressorAllow = ConverterAllow;
    }
    if (ConverterAllow == true)
        SendCtrlToNext("ConverterSwitch", 1, CabNo);
    else
        SendCtrlToNext("ConverterSwitch", 0, CabNo);

    return CS;
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
bool TMoverParameters::CompressorSwitch(bool State)
{
    bool CS = false; // Ra: normalnie chyba tak?
    // if State=true then
    //  if ((CompressorPower=2) and (not ConverterAllow)) then
    //   State:=false; //yB: to juz niepotrzebne
    if ((CompressorAllow != State) && (CompressorPower < 2))
    {
        CompressorAllow = State;
        CS = true;
    }
    if (CompressorAllow == true)
        SendCtrlToNext("CompressorSwitch", 1, CabNo);
    else
        SendCtrlToNext("CompressorSwitch", 0, CabNo);

    return CS;
}

// *************************************************************************************************
// Q: 20160711
// *************************************************************************************************
bool TMoverParameters::IncBrakeLevelOld(void)
{
    bool IBLO = false;

    if ((BrakeCtrlPosNo > 0) /*and (LocalBrakePos=0)*/)
    {
        if (BrakeCtrlPos < BrakeCtrlPosNo)
        {
            BrakeCtrlPos++;
            //      BrakeCtrlPosR = BrakeCtrlPos;

            // youBy: wywalilem to, jak jest EP, to sa przenoszone sygnaly nt. co ma robic, a nie
            // poszczegolne pozycje;
            //       wystarczy spojrzec na Knorra i Oerlikona EP w EN57; mogly ze soba
            //       wspolapracowac
            //{
            //        if (BrakeSystem==ElectroPneumatic)
            //          if (BrakePressureActual.BrakeType==ElectroPneumatic)
            //           {
            //           BrakeStatus = ord(BrakeCtrlPos > 0);
            //             SendCtrlToNext("BrakeCtrl", BrakeCtrlPos, CabNo);
            //           }
            //          else SendCtrlToNext("BrakeCtrl", -2, CabNo);
            //        else
            //         if (!TestFlag(BrakeStatus,b_dmg))
            //          BrakeStatus = b_on;}

            // youBy: EP po nowemu

            IBLO = true;
            if ((BrakePressureActual.PipePressureVal < 0) &&
                (BrakePressureTable[BrakeCtrlPos - 1].PipePressureVal > 0))
                LimPipePress = PipePress;

            if (BrakeSystem == ElectroPneumatic)
                if (BrakeSubsystem != ss_K)
                {
                    if ((BrakeCtrlPos * BrakeCtrlPos) == 1)
                    {
                        //                SendCtrlToNext('Brake',BrakeCtrlPos,CabNo);
                        //                SetFlag(BrakeStatus,b_epused);
                    }
                    else
                    {
                        //                SendCtrlToNext('Brake',0,CabNo);
                        //                SetFlag(BrakeStatus,-b_epused);
                    }
                }
        }
        else
        {
            IBLO = false;
            //        if (BrakeSystem == Pneumatic)
            //         EmergencyBrakeSwitch(true);
        }
    }
    else
        IBLO = false;

    return IBLO;
}

// *****************************************************************************
// Q: 20160711
// *****************************************************************************
bool TMoverParameters::DecBrakeLevelOld(void)
{
    bool DBLO = false;

    if ((BrakeCtrlPosNo > 0) /*&& (LocalBrakePos == 0)*/)
    {
        if (BrakeCtrlPos > -1 - int(BrakeHandle == FV4a))
        {
            BrakeCtrlPos--;
            //        BrakeCtrlPosR:=BrakeCtrlPos;
            if (EmergencyBrakeFlag)
            {
                EmergencyBrakeFlag = false; //!!!
                SendCtrlToNext("Emergency_brake", 0, CabNo);
            }

            // youBy: wywalilem to, jak jest EP, to sa przenoszone sygnaly nt. co ma robic, a nie
            // poszczegolne pozycje;
            //       wystarczy spojrzec na Knorra i Oerlikona EP w EN57; mogly ze soba
            //       wspolapracowac
            /*
                    if (BrakeSystem == ElectroPneumatic)
                      if (BrakePressureActual.BrakeType == ElectroPneumatic)
                       {
            //             BrakeStatus =ord(BrakeCtrlPos > 0);
                         SendCtrlToNext("BrakeCtrl",BrakeCtrlPos,CabNo);
                       }
                      else SendCtrlToNext('BrakeCtrl',-2,CabNo);
            //        else}
            //         if (not TestFlag(BrakeStatus,b_dmg) and (not
            TestFlag(BrakeStatus,b_release))) then
            //          BrakeStatus:=b_off;   {luzowanie jesli dziala oraz nie byl wlaczony
            odluzniacz
            */

            // youBy: EP po nowemu
            DBLO = true;
            //        if ((BrakePressureTable[BrakeCtrlPos].PipePressureVal<0.0) &&
            //        (BrakePressureTable[BrakeCtrlPos+1].PipePressureVal > 0))
            //          LimPipePress:=PipePress;

            if (BrakeSystem == ElectroPneumatic)
                if (BrakeSubsystem != ss_K)
                {
                    if ((BrakeCtrlPos * BrakeCtrlPos) == 1)
                    {
                        //                SendCtrlToNext("Brake", BrakeCtrlPos, CabNo);
                        //                SetFlag(BrakeStatus, b_epused);
                    }
                    else
                    {
                        //                SendCtrlToNext("Brake", 0, CabNo);
                        //                SetFlag(BrakeStatus, -b_epused);
                    }
                }
            //    for b:=0 to 1 do  {poprawic to!}
            //     with Couplers[b] do
            //      if CouplingFlag and ctrain_controll=ctrain_controll then
            //       Connected^.BrakeCtrlPos:=BrakeCtrlPos;
            //
        }
        else
            DBLO = false;
    }
    else
        DBLO = false;

    return DBLO;
}

// *************************************************************************************************
// Q: 20160711
// *************************************************************************************************
bool TMoverParameters::IncLocalBrakeLevel(int CtrlSpeed)
{
    bool IBL;
    if ((LocalBrakePos < LocalBrakePosNo) /*and (BrakeCtrlPos<1)*/)
    {
        while ((LocalBrakePos < LocalBrakePosNo) && (CtrlSpeed > 0))
        {
            LocalBrakePos++;
            CtrlSpeed--;
        }
        IBL = true;
    }
    else
        IBL = false;
    UnBrake = true;

    return IBL;
}

// *************************************************************************************************
// Q: 20160711
// *************************************************************************************************
bool TMoverParameters::DecLocalBrakeLevel(int CtrlSpeed)
{
    bool DBL;
    if (LocalBrakePos > 0)
    {
        while ((CtrlSpeed > 0) && (LocalBrakePos > 0))
        {
            LocalBrakePos--;
            CtrlSpeed--;
        }
        DBL = true;
    }
    else
        DBL = false;
    UnBrake = true;

    return DBL;
}

// *************************************************************************************************
// Q: 20160711
// *************************************************************************************************
bool TMoverParameters::IncLocalBrakeLevelFAST(void)
{
    bool ILBLF;
    if (LocalBrakePos < LocalBrakePosNo)
    {
        LocalBrakePos = LocalBrakePosNo;
        ILBLF = true;
    }
    else
        ILBLF = false;
    UnBrake = true;
    return ILBLF;
}

// *************************************************************************************************
// Q: 20160711
// *************************************************************************************************
bool TMoverParameters::DecLocalBrakeLevelFAST(void)
{
    bool DLBLF;
    if (LocalBrakePos > 0)
    {
        LocalBrakePos = 0;
        DLBLF = true;
    }
    else
        DLBLF = false;
    UnBrake = true;
    return DLBLF;
}

// *************************************************************************************************
// Q: 20160711
// *************************************************************************************************
bool TMoverParameters::IncManualBrakeLevel(int CtrlSpeed)
{
    bool IMBL;
    if (ManualBrakePos < ManualBrakePosNo) /*and (BrakeCtrlPos<1)*/
    {
        while ((ManualBrakePos < ManualBrakePosNo) && (CtrlSpeed > 0))
        {
            ManualBrakePos++;
            CtrlSpeed--;
        }
        IMBL = true;
    }
    else
        IMBL = false;

    return IMBL;
}

// *************************************************************************************************
// Q: 20160711
// *************************************************************************************************
bool TMoverParameters::DecManualBrakeLevel(int CtrlSpeed)
{
    bool DMBL;
    if (ManualBrakePos > 0)
    {
        while ((CtrlSpeed > 0) && (ManualBrakePos > 0))
        {
            ManualBrakePos--;
            CtrlSpeed--;
        }
        DMBL = true;
    }
    else
        DMBL = false;
    return DMBL;
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
bool TMoverParameters::DynamicBrakeSwitch(bool Switch)
{
    int b;
    bool DBS;

    if ((DynamicBrakeType == dbrake_switch) && (MainCtrlPos == 0))
    {
        DynamicBrakeFlag = Switch;
        DBS = true;
        for (b = 0; b < 1; b++)
            //  with Couplers[b] do
            if (TestFlag(Couplers[b].CouplingFlag, ctrain_controll))
                Couplers[b].Connected->DynamicBrakeFlag = DynamicBrakeFlag;
        // end;
        // if (DynamicBrakeType=dbrake_passive) and (TrainType=dt_ET42) then
        // begin
        // DynamicBrakeFlag:=false;
        // DynamicBrakeSwitch:=false;
    }
    else
        DBS = false;

    return DBS;
}

// *************************************************************************************************
// Q: 20160711
// *************************************************************************************************
bool TMoverParameters::EmergencyBrakeSwitch(bool Switch)
{
    bool EBS;
    if ((BrakeSystem != Individual) && (BrakeCtrlPosNo > 0))
    {
        if ((!EmergencyBrakeFlag) && Switch)
        {
            EmergencyBrakeFlag = Switch;
            EBS = true;
        }
        else
        {
            if ((abs(V) < 0.1) &&
                (Switch == false)) // odblokowanie hamulca bezpieczenistwa tylko po zatrzymaniu
            {
                EmergencyBrakeFlag = Switch;
                EBS = true;
            }
            else
                EBS = false;
        }
    }
    else
        EBS = false; // nie ma hamulca bezpieczenstwa gdy nie ma hamulca zesp.

    return EBS;
}

// *************************************************************************************************
// Q: 20160710
// *************************************************************************************************
bool TMoverParameters::AntiSlippingBrake(void)
{
    bool ASB = false; // Ra: przeniesione z koñca
    if (ASBType == 1)
    {
        ASB = true; // SPKS!!
        Hamulec->ASB(1);
        BrakeSlippingTimer = 0;
    }
    return ASB;
}

// *************************************************************************************************
// Q: 20160711
// *************************************************************************************************
bool TMoverParameters::BrakeReleaser(int state)
{
    bool OK;
    Hamulec->Releaser(state);
    if (CabNo != 0) // rekurencyjne wys³anie do nastêpnego
        OK = SendCtrlToNext("BrakeReleaser", state, CabNo);
    return OK;
}

// *************************************************************************************************
// Q: 20160711
// *************************************************************************************************
bool TMoverParameters::SwitchEPBrake(int state)
{
    bool OK;
    double temp;

    OK = false;
    if ((BrakeHandle == St113) && (ActiveCab != 0))
    {
        // if(state > 0)
        //  temp = (Handle as TSt113).GetCP    // TODO: przetlumaczyc
        // else
        temp = 0;
        Hamulec->SetEPS(temp);
        SendCtrlToNext("Brake", temp, CabNo);
    }
    //  OK:=SetFlag(BrakeStatus,((2*State-1)*b_epused));
    //  SendCtrlToNext('Brake',(state*(2*BrakeCtrlPos-1)),CabNo);
    return OK;
}

// *************************************************************************************************
// Q: 20160711
// *************************************************************************************************
bool TMoverParameters::IncBrakePress(double &brake, double PressLimit, double dp)
{
    bool IBP;
    //  if (DynamicBrakeType<>dbrake_switch) and (DynamicBrakeType<>dbrake_none) and
    //  ((BrakePress>2.0) or (PipePress<3.7{(LowPipePress+0.5)})) then
    if ((DynamicBrakeType != dbrake_switch) && (DynamicBrakeType != dbrake_none) &&
        (BrakePress > 2.0) &&
        (TrainType != dt_EZT)) // yB radzi nie sprawdzaæ ciœnienia w przewodzie
    // hunter-301211: dla EN57 silnikow nie odlaczamy
    {
        DynamicBrakeFlag = true; // uruchamianie hamulca ED albo odlaczanie silnikow
        if ((DynamicBrakeType == dbrake_automatic) &&
            (abs(Im) > 60)) // nie napelniaj wiecej, jak na EP09
            dp = 0.0;
    }
    if (brake + dp < PressLimit)
    {
        brake = brake + dp;
        IBP = true;
    }
    else
    {
        IBP = false;
        brake = PressLimit;
    }
    return IBP;
}

// *************************************************************************************************
// Q: 20160711
// *************************************************************************************************
bool TMoverParameters::DecBrakePress(double &brake, double PressLimit, double dp)
{
    bool DBP;

    if (brake - dp > PressLimit)
    {
        brake = brake - dp;
        DBP = true;
    }
    else
    {
        DBP = false;
        brake = PressLimit;
    }
    //  if ((DynamicBrakeType != dbrake_switch) && ((BrakePress < 0.1) && (PipePress > 0.45
    //  /*(LowPipePress+0.06)*/ )))
    if ((DynamicBrakeType != dbrake_switch) &&
        (BrakePress < 0.1)) // yB radzi nie sprawdzaæ ciœnienia w przewodzie
        DynamicBrakeFlag = false; // wylaczanie hamulca ED i/albo zalaczanie silnikow

    return DBP;
}

// *************************************************************************************************
// Q: 20160711
// *************************************************************************************************
bool TMoverParameters::BrakeDelaySwitch(int BDS)
{
    bool rBDS;
    //  if (BrakeCtrlPosNo > 0)
    if (Hamulec->SetBDF(BDS))
    {
        BrakeDelayFlag = BDS;
        rBDS = true;
        BrakeStatus = (BrakeStatus & 191);
        // kopowanie nastawy hamulca do kolejnego czlonu - do przemyœlenia
        if (CabNo != 0)
            SendCtrlToNext("BrakeDelay", BrakeDelayFlag, CabNo);
    }
    else
        rBDS = false;
    return rBDS;
}

// *************************************************************************************************
// Q: 20160712
// *************************************************************************************************
bool TMoverParameters::IncBrakeMult(void)
{
    bool IBM;

    if ((LoadFlag > 0) && (MBPM < 2) && (LoadFlag < 3))
    {
        if ((MaxBrakePress[2] > 0) && (LoadFlag == 1))
            LoadFlag = 2;
        else
            LoadFlag = 3;
        IBM = true;
        if (BrakeCylMult[2] > 0)
            BrakeCylMult[0] = BrakeCylMult[2];
    }
    else
        IBM = false;

    return IBM;
}

// *************************************************************************************************
// Q: 20160712
// *************************************************************************************************
bool TMoverParameters::DecBrakeMult(void)
{
    bool DBM;
    if ((LoadFlag > 1) && (MBPM < 2))
    {
        if ((MaxBrakePress[2] > 0) && (LoadFlag == 3))
            LoadFlag = 2;
        else
            LoadFlag = 1;
        DBM = true;
        if (BrakeCylMult[1] > 0)
            BrakeCylMult[0] = BrakeCylMult[1];
    }
    else
        DBM = false;

    return DBM;
}

// *************************************************************************************************
// Q: 20160712
// *************************************************************************************************
void TMoverParameters::UpdateBrakePressure(double dt)
{
    const LBDelay = 5.0; // stala czasowa hamulca
    double Rate, Speed, dp, sm;

    dpLocalValve = 0;
    dpBrake = 0;

    BrakePress = Hamulec->GetBCP();
    //  BrakePress:=(Hamulec as TEst4).ImplsRes.pa;
    Volume = Hamulec->GetBRP();
}

// *************************************************************************************************
// Q: 20160712
// *************************************************************************************************
void TMoverParameters::CompressorCheck(double dt)
{
    // if (CompressorSpeed>0.0) then //ten warunek zosta³ sprawdzony przy wywo³aniu funkcji
    if (VeselVolume > 0)
    {
        if (MaxCompressor - MinCompressor < 0.0001)
        {
            //     if (Mains && (MainCtrlPos > 1))
            if (CompressorAllow && Mains && (MainCtrlPos > 0))
            {
                if (Compressor < MaxCompressor)
                    if ((EngineType == DieselElectric) && (CompressorPower > 0))
                        CompressedVolume = CompressedVolume +
                                           dt * CompressorSpeed * (2 * MaxCompressor - Compressor) /
                                               MaxCompressor * (DElist[MainCtrlPos].RPM /
                                                                DElist[MainCtrlPosNo].RPM);
                    else
                    {
                        CompressedVolume =
                            CompressedVolume +
                            dt * CompressorSpeed * (2 * MaxCompressor - Compressor) / MaxCompressor;
                        TotalCurrent =
                            0.0015 *
                            Voltage; // tymczasowo tylko obci¹¿enie sprê¿arki, tak z 5A na sprê¿arkê
                    }
                else
                {
                    CompressedVolume = CompressedVolume * 0.8;
                    SetFlag(SoundFlag, sound_relay);
                    SetFlag(SoundFlag, sound_loud);
                }
            }
        }
        else
        {
            if (CompressorFlag) // jeœli sprê¿arka za³¹czona
            { // sprawdziæ mo¿liwe warunki wy³¹czenia sprê¿arki
                if (CompressorPower == 5) // jeœli zasilanie z s¹siedniego cz³onu
                { // zasilanie sprê¿arki w cz³onie ra z cz³onu silnikowego (sprzêg 1)
                    if (Couplers[1].Connected != NULL)
                        CompressorFlag =
                            (Couplers[1].Connected->CompressorAllow &&
                             Couplers[1].Connected->ConverterFlag && Couplers[1].Connected->Mains);
                    else
                        CompressorFlag = false; // bez tamtego cz³onu nie zadzia³a
                }
                else if (CompressorPower == 4) // jeœli zasilanie z poprzedniego cz³onu
                { // zasilanie sprê¿arki w cz³onie ra z cz³onu silnikowego (sprzêg 1)
                    if (Couplers[0].Connected != NULL)
                        CompressorFlag =
                            (Couplers[0].Connected->CompressorAllow &&
                             Couplers[0].Connected->ConverterFlag && Couplers[0].Connected->Mains);
                    else
                        CompressorFlag = false; // bez tamtego cz³onu nie zadzia³a
                }
                else
                    CompressorFlag = ((CompressorAllow) &&
                                      ((ConverterFlag) || (CompressorPower == 0)) && (Mains));
                if (Compressor >
                    MaxCompressor) // wy³¹cznik ciœnieniowy jest niezale¿ny od sposobu zasilania
                    CompressorFlag = false;
            }
            else // jeœli nie za³¹czona
                if ((Compressor < MinCompressor) &&
                    (LastSwitchingTime > CtrlDelay)) // jeœli nie za³¹czona, a ciœnienie za ma³e
            { // za³¹czenie przy ma³ym ciœnieniu
                if (CompressorPower == 5) // jeœli zasilanie z nastêpnego cz³onu
                { // zasilanie sprê¿arki w cz³onie ra z cz³onu silnikowego (sprzêg 1)
                    if (Couplers[1].Connected != NULL)
                        CompressorFlag =
                            (Couplers[1].Connected->CompressorAllow &&
                             Couplers[1].Connected->ConverterFlag && Couplers[1].Connected->Mains);
                    else
                        CompressorFlag = false; // bez tamtego cz³onu nie zadzia³a
                }
                else if (CompressorPower == 4) // jeœli zasilanie z poprzedniego cz³onu
                { // zasilanie sprê¿arki w cz³onie ra z cz³onu silnikowego (sprzêg 1)
                    if (Couplers[0].Connected != NULL)
                        CompressorFlag =
                            (Couplers[0].Connected->CompressorAllow &&
                             Couplers[0].Connected->ConverterFlag && Couplers[0].Connected->Mains);
                    else
                        CompressorFlag = false; // bez tamtego cz³onu nie zadzia³a
                }
                else
                    CompressorFlag = ((CompressorAllow) &&
                                      ((ConverterFlag) || (CompressorPower == 0)) && (Mains));
                if (CompressorFlag) // jeœli zosta³a za³¹czona
                    LastSwitchingTime = 0; // to trzeba ograniczyæ ponowne w³¹czenie
            }
            //          for b:=0 to 1 do //z Megapacka
            //    with Couplers[b] do
            //     if TestFlag(CouplingFlag,ctrain_scndpneumatic) then
            //      Connected.CompressorFlag:=CompressorFlag;
            if (CompressorFlag)
                if ((EngineType == DieselElectric) && (CompressorPower > 0))
                    CompressedVolume = CompressedVolume +
                                       dt * CompressorSpeed * (2 * MaxCompressor - Compressor) /
                                           MaxCompressor *
                                           (DElist[MainCtrlPos].RPM / DElist[MainCtrlPosNo].RPM);
                else
                {
                    CompressedVolume =
                        CompressedVolume +
                        dt * CompressorSpeed * (2 * MaxCompressor - Compressor) / MaxCompressor;
                    if ((CompressorPower == 5) && (Couplers[1].Connected != NULL))
                        Couplers[1].Connected->TotalCurrent =
                            0.0015 * Couplers[1].Connected->Voltage; // tymczasowo tylko obci¹¿enie
                                                                     // sprê¿arki, tak z 5A na
                                                                     // sprê¿arkê
                    else if ((CompressorPower == 4) && (Couplers[0].Connected != NULL))
                        Couplers[0].Connected->TotalCurrent =
                            0.0015 * Couplers[0].Connected->Voltage; // tymczasowo tylko obci¹¿enie
                                                                     // sprê¿arki, tak z 5A na
                                                                     // sprê¿arkê
                    else
                        TotalCurrent =
                            0.0015 *
                            Voltage; // tymczasowo tylko obci¹¿enie sprê¿arki, tak z 5A na sprê¿arkê
                }
        }
    }
}

// *************************************************************************************************
// Q: 20160712
// *************************************************************************************************
void TMoverParameters::UpdatePipePressure(double dt)
{
    const LBDelay = 100;
    const kL = 0.5;
    double dV;
    TMoverParameters *c; // T_MoverParameters
    double temp;
    int b;

    PipePress = Pipe->P();
    //  PPP:=PipePress;

    dpMainValve = 0;

    if ((BrakeCtrlPosNo > 1) && (ActiveCab != 0))
    // with BrakePressureTable[BrakeCtrlPos] do
    {
        dpLocalValve = LocHandle->GetPF(LocalBrakePos / LocalBrakePosNo, Hamulec->GetBCP(),
                                        ScndPipePress, dt, 0);
        if ((BrakeHandle == FV4a) &&
            ((PipePress < 2.75) && ((Hamulec->GetStatus() && b_rls) == 0)) &&
            (BrakeSubsystem == ss_LSt) && (TrainType != dt_EZT))
            temp = PipePress + 0.00001;
        else
            temp = ScndPipePress;
        Handle->SetReductor(BrakeCtrlPos2);

        dpMainValve = Handle->GetPF(BrakeCtrlPosR, PipePress, temp, dt, EqvtPipePress);
        if (dpMainValve < 0) // && (PipePressureVal > 0.01)           //50
            if (Compressor > ScndPipePress)
            {
                CompressedVolume = CompressedVolume + dpMainValve / 1500;
                Pipe2->Flow(dpMainValve / 3);
            }
            else
                Pipe2->Flow(dpMainValve);
    }

    //      if(EmergencyBrakeFlag)and(BrakeCtrlPosNo=0)then         //ulepszony hamulec bezp.
    if ((EmergencyBrakeFlag) || (TestFlag(SecuritySystem.Status, s_SHPebrake)) ||
        (TestFlag(SecuritySystem.Status, s_CAebrake)) ||
        (s_CAtestebrake == true)) // ulepszony hamulec bezp.
        dpMainValve = dpMainValve / 1 +
                      PF(0, PipePress, 0.15, 0.25) *
                          dt; // Q: dodalem 4 paraMETR 0.25 bo wywalalo ze za malo parametrow
    // 0.2*Spg
    Pipe->Flow(-dpMainValve);
    Pipe->Flow(-(PipePress)*0.001 * dt);
    //      if Heating then
    //        Pipe.Flow(PF(PipePress, 0, d2A(7)) * dt);
    //      if ConverterFlag then
    //        Pipe.Flow(PF(PipePress, 0, d2A(12)) * dt);
    dpMainValve = dpMainValve / (Dim.L * Spg * 20);

    CntrlPipePress = Hamulec->GetVRP(); // ciœnienie komory wstêpnej rozdzielacza

    //      if (Hamulec is typeid(TWest)) return 0;

    switch (BrakeValve)
    {
    case W:
    {
        if (BrakeLocHandle != NoHandle)
        {
            LocBrakePress = LocHandle->GetCP();

            //(Hamulec as TWest).SetLBP(LocBrakePress);
            static_cast<TWest *>(Hamulec)->SetLBP(LocBrakePress);
        }
        if (MBPM < 2)
            //(Hamulec as TWest).PLC(MaxBrakePress[LoadFlag])
            static_cast<TWest *>(Hamulec)->PLC(MaxBrakePress[LoadFlag]);
        else
            //(Hamulec as TWest).PLC(TotalMass);
            static_cast<TWest *>(Hamulec)->PLC(TotalMass);
        break;
    }

    case LSt:
    case EStED:
    {
        LocBrakePress = LocHandle->GetCP();
        for (int b = 0; b < 1; b++)
            if (((TrainType && (dt_ET41 || dt_ET42)) > 0) &&
                (Couplers[b].Connected != NULL)) // nie podoba mi siê to rozwi¹zanie, chyba trzeba
                                                 // dodaæ jakiœ wpis do fizyki na to
                if (((Couplers[b].Connected->TrainType && (dt_ET41 || dt_ET42)) > 0) &&
                    ((Couplers[b].CouplingFlag & 36) == 36))
                    LocBrakePress = Max0R(Couplers[b].Connected->LocHandle->GetCP(), LocBrakePress);

        if ((DynamicBrakeFlag) && (EngineType == ElectricInductionMotor))
        {
            if (Vel > 10)
                LocBrakePress = 0;
            else if (Vel > 5)
                LocBrakePress = (10 - Vel) / 5 * LocBrakePress;
        }

        //(Hamulec as TLSt).SetLBP(LocBrakePress);
        static_cast<TLSt *>(Hamulec)->SetLBP(LocBrakePress);
        break;
    }

    case CV1_L_TR:
    {
        LocBrakePress = LocHandle->GetCP();
        //(Hamulec as TCV1L_TR).SetLBP(LocBrakePress);
        static_cast<TCV1L_TR *>(Hamulec)->SetLBP(LocBrakePress);
        break;
    }

    // EP2:      (Hamulec as TEStEP2).PLC(TotalMass);
    case ESt3AL2:
    case NESt3:
    case ESt4:
    case ESt3:
    {
        if (MBPM < 2)
            //(Hamulec as TNESt3).PLC(MaxBrakePress[LoadFlag])
            static_cast<TNESt3 *>(Hamulec)->PLC(MaxBrakePress[LoadFlag]);
        else
            //(Hamulec as TNESt3).PLC(TotalMass);
            static_cast<TNESt3 *>(Hamulec)->PLC(TotalMass);
        LocBrakePress = LocHandle->GetCP();
        //(Hamulec as TNESt3).SetLBP(LocBrakePress);
        static_cast<TNESt3 *>(Hamulec)->SetLBP(LocBrakePress);
        break;
    }
    case KE:
    {
        LocBrakePress = LocHandle->GetCP();
        //(Hamulec as TKE).SetLBP(LocBrakePress);
        static_cast<TKE *>(Hamulec)->SetLBP(LocBrakePress);
        if (MBPM < 2)
            //(Hamulec as TKE).PLC(MaxBrakePress[LoadFlag])
            static_cast<TKE *>(Hamulec)->PLC(MaxBrakePress[LoadFlag]);
        else
            //(Hamulec as TKE).PLC(TotalMass);
            static_cast<TKE *>(Hamulec)->PLC(TotalMass);
        break;
    }
    } // switch

    if ((BrakeHandle == FVel6) && (ActiveCab != 0))
    {
        if ((Battery) && (ActiveDir != 0) &&
            (EpFuse)) // tu powinien byc jeszcze bezpiecznik EP i baterie -
            // temp = (Handle as TFVel6).GetCP
            temp = static_cast<TFVel6 *>(Handle)->GetCP();
        else
            temp = 0;
        Hamulec->SetEPS(temp);
        SendCtrlToNext("Brake", temp,
                       CabNo); // Ra 2014-11: na tym siê wysypuje, ale nie wiem, w jakich warunkach
    }

    Pipe->Act();
    PipePress = Pipe->P();
    if ((BrakeStatus & 128) == 128) // jesli hamulec wy³¹czony
        temp = 0; // odetnij
    else
        temp = 1; // po³¹cz
    Pipe->Flow(temp * Hamulec->GetPF(temp * PipePress, dt, Vel) + GetDVc(dt));

    if (ASBType == 128)
        Hamulec->ASB(int(SlippingWheels));

    dpPipe = 0;

    // yB: jednokrokowe liczenie tego wszystkiego
    Pipe->Act();
    PipePress = Pipe->P();

    dpMainValve = dpMainValve / (100 * dt); // normalizacja po czasie do syczenia;

    if (PipePress < -1)
    {
        PipePress = -1;
        Pipe->CreatePress(-1);
        Pipe->Act();
    }

    if (CompressedVolume < 0)
        CompressedVolume = 0;
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
void TMoverParameters::UpdateScndPipePressure(double dt)
{
    const Spz = 0.5067;
    // T_MoverParameters *c;
    TMoverParameters *c;
    double dv1, dv2, dV;

    dv1 = 0;
    dv2 = 0;

    // sprzeg 1
    if (Couplers[0].Connected != NULL)
        if (TestFlag(Couplers[0].CouplingFlag, ctrain_scndpneumatic))
        {
            c = Couplers[0].Connected; // skrot
            dv1 = 0.5 * dt * PF(ScndPipePress, c->ScndPipePress, Spz * 0.75, 0.25);
            if (dv1 * dv1 > 0.00000000000001)
                c->Physic_ReActivation();
            c->Pipe2->Flow(-dv1);
        }
    // sprzeg 2
    if (Couplers[1].Connected != NULL)
        if (TestFlag(Couplers[1].CouplingFlag, ctrain_scndpneumatic))
        {
            c = Couplers[1].Connected; // skrot
            dv2 = 0.5 * dt * PF(ScndPipePress, c->ScndPipePress, Spz * 0.75, 0.25);
            if (dv2 * dv2 > 0.00000000000001)
                c->Physic_ReActivation();
            c->Pipe2->Flow(-dv2);
        }
    if ((Couplers[1].Connected != NULL) && (Couplers[0].Connected != NULL))
        if ((TestFlag(Couplers[0].CouplingFlag, ctrain_scndpneumatic)) &&
            (TestFlag(Couplers[1].CouplingFlag, ctrain_scndpneumatic)))
        {
            dV = 0.00025 * dt * PF(Couplers[0].Connected->ScndPipePress,
                                   Couplers[1].Connected->ScndPipePress, Spz * 0.25, 0.25);
            Couplers[0].Connected->Pipe2->Flow(+dV);
            Couplers[1].Connected->Pipe2->Flow(-dV);
        }

    Pipe2->Flow(Hamulec->GetHPFlow(ScndPipePress, dt));

    if (((Compressor > ScndPipePress) && (CompressorSpeed > 0.0001)) || (TrainType == dt_EZT))
    {
        dV = PF(Compressor, ScndPipePress, Spz, 0.25) * dt;
        CompressedVolume = CompressedVolume + dV / 1000;
        Pipe2->Flow(-dV);
    }

    Pipe2->Flow(dv1 + dv2);
    Pipe2->Act();
    ScndPipePress = Pipe2->P();

    if (ScndPipePress < -1)
    {
        ScndPipePress = -1;
        Pipe2->CreatePress(-1);
        Pipe2->Act();
    }
}

// *************************************************************************************************
// Q: 20160715
// *************************************************************************************************
double TMoverParameters::GetDVc(double dt)
{
    // T_MoverParameters *c;
    TMoverParameters *c;
    double dv1, dv2, dV;

    dv1 = 0;
    dv2 = 0;
    // sprzeg 1
    if (Couplers[0].Connected != NULL)
        if (TestFlag(Couplers[0].CouplingFlag, ctrain_pneumatic))
        { //*0.85
            c = Couplers[0].Connected; // skrot           //0.08           //e/D * L/D = e/D^2 * L
            dv1 = 0.5 * dt * PF(PipePress, c->PipePress, (Spg) / (1 + 0.015 / Spg * Dim.L), 0.25);
            if (dv1 * dv1 > 0.00000000000001)
                c->Physic_ReActivation();
            c->Pipe->Flow(-dv1);
        }
    // sprzeg 2
    if (Couplers[1].Connected != NULL)
        if (TestFlag(Couplers[1].CouplingFlag, ctrain_pneumatic))
        {
            c = Couplers[1].Connected; // skrot
            dv2 = 0.5 * dt * PF(PipePress, c->PipePress, (Spg) / (1 + 0.015 / Spg * Dim.L), 0.25);
            if (dv2 * dv2 > 0.00000000000001)
                c->Physic_ReActivation();
            c->Pipe->Flow(-dv2);
        }
    if ((Couplers[1].Connected != NULL) && (Couplers[0].Connected != NULL))
        if ((TestFlag(Couplers[0].CouplingFlag, ctrain_pneumatic)) &&
            (TestFlag(Couplers[1].CouplingFlag, ctrain_pneumatic)))
        {
            dV = 0.05 * dt * PF(Couplers[0].Connected->PipePress, Couplers[1].Connected->PipePress,
                                (Spg * 0.85) / (1 + 0.03 * Dim.L), 0.25) *
                 0;
            Couplers[0].Connected->Pipe->Flow(+dV);
            Couplers[1].Connected->Pipe->Flow(-dV);
        }
    // suma
    return dv2 + dv1;
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
void TMoverParameters::ComputeConstans(void)
{
    double BearingF, RollF, HideModifier;
    double Curvature; // Ra 2014-07: odwrotnoœæ promienia

    TotalCurrent = 0; // Ra 2014-04: tu zerowanie, aby EZT mog³o pobieraæ pr¹d innemu cz³onowi
    TotalMass = ComputeMass();
    TotalMassxg = TotalMass * g; // TotalMass*g
    BearingF = 2 * (DamageFlag && dtrain_bearing);

    HideModifier = 0; // int(Couplers[0].CouplingFlag>0)+int(Couplers[1].CouplingFlag>0);

    if (BearingType == 0)
        RollF = 0.05; // slizgowe
    else
        RollF = 0.015; // toczne
    RollF = RollF + BearingF / 200.0;

    //    if (NPoweredAxles > 0)
    //     RollF = RollF * 1.5;    //dodatkowe lozyska silnikow

    if (NPoweredAxles > 0) // drobna optymalka
    {
        RollF = RollF + 0.025;
        //       if (Ft * Ft < 1)
        //         HideModifier = HideModifier - 3;
    }
    Ff = TotalMassxg * (BearingF + RollF * V * V / 10.0) / 1000.0;
    // dorobic liczenie temperatury lozyska!
    FrictConst1 = ((TotalMassxg * RollF) / 10000.0) + (Cx * Dim.W * Dim.H);
    Curvature = abs(RunningShape.R); // zero oznacza nieskoñczony promieñ
    if (Curvature > 0.0)
        Curvature = 1.0 / Curvature;
    // opór sk³adu na ³uku (youBy): +(500*TrackW/R)*TotalMassxg*0.001 do FrictConst2s/d
    FrictConst2s = (TotalMassxg * ((500.0 * TrackW * Curvature) + 2.5 - HideModifier +
                                   2 * BearingF / dtrain_bearing)) *
                   0.001;
    FrictConst2d = (TotalMassxg * ((500.0 * TrackW * Curvature) + 2.0 - HideModifier +
                                   BearingF / dtrain_bearing)) *
                   0.001;
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
double TMoverParameters::ComputeMass(void)
{
    double M;

    if (Load > 0)
    { // zak³adamy, ¿e ³adunek jest pisany ma³ymi literami
        if (LoadQuantity == "tonns")
            M = Load * 1000;
        else if (LoadType == "passengers")
            M = Load * 80;
        else if (LoadType == "luggage")
            M = Load * 100;
        else if (LoadType == "cars")
            M = Load * 800;
        else if (LoadType == "containers")
            M = Load * 8000;
        else if (LoadType == "transformers")
            M = Load * 50000;
        else
            M = Load * 1000;
    }
    else
        M = 0;
    // Ra: na razie tak, ale nie wszêdzie masy wiruj¹ce siê wliczaj¹
    return Mass + M + Mred;
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
void TMoverParameters::ComputeTotalForce(double dt, double dt1, bool FullVer)
{
    int b;

    if (PhysicActivation)
    {
        //  EventFlag:=false;                                             {jesli cos sie zlego
        //  wydarzy to ustawiane na true}
        //  SoundFlag:=0;                                                 {jesli ma byc jakis dzwiek
        //  to zapalany jet odpowiedni bit}
        // to powinno byc zerowane na zewnatrz

        // juz zoptymalizowane:
        FStand = FrictionForce(RunningShape.R, RunningTrack.DamageFlag); // si³a oporów ruchu
        Vel = abs(V) * 3.6; // prêdkoœæ w km/h
        nrot = v2n(); // przeliczenie prêdkoœci liniowej na obrotow¹

        if (TestFlag(BrakeMethod, bp_MHS) && (PipePress < 3.0) && (Vel > 45) &&
            TestFlag(BrakeDelayFlag, bdelay_M)) // ustawione na sztywno na 3 bar
            FStand = FStand + /*(RunningTrack.friction)**/ TrackBrakeForce; // doliczenie hamowania
                                                                            // hamulcem szynowym
        // w charakterystykach jest wartoœæ si³y hamowania zamiast nacisku
        //    if(FullVer=true) then
        //    begin //ABu: to dla optymalizacji, bo chyba te rzeczy wystarczy sprawdzac 1 raz na
        //    klatke?
        LastSwitchingTime = LastSwitchingTime + dt1;
        if (EngineType == ElectricSeriesMotor)
            LastRelayTime = LastRelayTime + dt1;
        //    end;
        if (Mains && /*(abs(CabNo) < 2) &&*/ (EngineType ==
                                              ElectricSeriesMotor)) // potem ulepszyc! pantogtrafy!
        { // Ra 2014-03: uwzglêdnienie kierunku jazdy w napiêciu na silnikach, a powinien byæ
          // zdefiniowany nawrotnik
            if (CabNo == 0)
                Voltage = RunningTraction.TractionVoltage * ActiveDir;
            else
                Voltage = RunningTraction.TractionVoltage * DirAbsolute; // ActiveDir*CabNo;
        } // bo nie dzialalo
        else
            Voltage = 0;
        if (Mains && /*(abs(CabNo) < 2) &&*/ (
                         EngineType == ElectricInductionMotor)) // potem ulepszyc! pantogtrafy!
            Voltage = RunningTraction.TractionVoltage;
        // end;

        if (Power > 0)
            FTrain = TractionForce(dt);
        else
            FTrain = 0;
        Fb = BrakeForce(RunningTrack);
        if (Max0R(abs(FTrain), Fb) > TotalMassxg * Adhesive(RunningTrack.friction)) // poslizg
            SlippingWheels = true;
        if (SlippingWheels)
        {
            //     TrainForce:=TrainForce-Fb;
            nrot = ComputeRotatingWheel((FTrain - Fb * Sign(V) - FStand) / NAxles -
                                            Sign(nrot * PI * WheelDiameter - V) *
                                                Adhesive(RunningTrack.friction) * TotalMass,
                                        dt, nrot);
            FTrain = Sign(FTrain) * TotalMassxg * Adhesive(RunningTrack.friction);
            Fb = Min0R(Fb, TotalMassxg * Adhesive(RunningTrack.friction));
        }
        //  else SlippingWheels:=false;
        //  FStand:=0;
        for (b = 0; b < 1; b++)
            if (Couplers[b].Connected != NULL) /*and (Couplers[b].CouplerType<>Bare) and
                                                  (Couplers[b].CouplerType<>Articulated)*/
            { // doliczenie si³ z innych pojazdów
                Couplers[b].CForce = CouplerForce(b, dt);
                FTrain = FTrain + Couplers[b].CForce;
            }
            else
                Couplers[b].CForce = 0;
        // FStand:=Fb+FrictionForce(RunningShape.R,RunningTrack.DamageFlag);
        FStand = Fb + FStand;
        FTrain =
            FTrain + TotalMassxg * RunningShape.dHtrack; // doliczenie sk³adowej stycznej grawitacji
        //!niejawne przypisanie zmiennej!
        FTotal = FTrain - Sign(V) * FStand;
    }

    // McZapkie-031103: sprawdzanie czy warto liczyc fizyke i inne updaty
    // ABu 300105: cos tu mieszalem , dziala teraz troche lepiej, wiec zostawiam
    //     zakomentowalem PhysicActivationFlag bo cos nie dzialalo i fizyka byla liczona zawsze.
    // if (PhysicActivationFlag)
    //{
    if ((CabNo == 0) && (Vel < 0.0001) && (abs(AccS) < 0.0001) && (TrainType != dt_EZT))
    {
        if (!PhysicActivation)
        {
            if (Couplers[0].Connected != NULL)
                if ((Couplers[0].Connected->Vel > 0.0001) ||
                    (abs(Couplers[0].Connected->AccS) > 0.0001))
                    Physic_ReActivation();
            if (Couplers[1].Connected != NULL)
                if ((Couplers[1].Connected->Vel > 0.0001) ||
                    (abs(Couplers[1].Connected->AccS) > 0.0001))
                    Physic_ReActivation();
        }
        if (LastSwitchingTime > 5) // 10+Random(100) then
            PhysicActivation = false; // zeby nie brac pod uwage braku V po uruchomieniu programu
    }
    else
        PhysicActivation = true;
    //};
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
double TMoverParameters::BrakeForce(const TTrackParam &Track)
{
    double K, Fb, NBrakeAxles, sm;
    // const OerlikonForceFactor=1.5;

    K = 0;
    if (NPoweredAxles > 0)
        NBrakeAxles = NPoweredAxles;
    else
        NBrakeAxles = NAxles;
    switch (LocalBrake)
    {
    case NoBrake:
        K = 0;
    case ManualBrake:
        K = MaxBrakeForce * ManualBrakeRatio();
    case HydraulicBrake:
        K = MaxBrakeForce * LocalBrakeRatio();
    case PneumaticBrake:
        if (Compressor < MaxBrakePress[3])
            K = MaxBrakeForce * LocalBrakeRatio() / 2.0;
        else
            K = 0;
    }

    if (MBrake == true)
    {
        K = MaxBrakeForce * ManualBrakeRatio();
    }

    // 0.03

    u = ((BrakePress * P2FTrans) - BrakeCylSpring) * BrakeCylMult[0] - BrakeSlckAdj;
    if (u * BrakeRigEff > Ntotal) // histereza na nacisku klockow
        Ntotal = u * BrakeRigEff;
    else
    {
        u = (BrakePress * P2FTrans) * BrakeCylMult[0] - BrakeSlckAdj;
        if (u * (2 - 1 * BrakeRigEff) < Ntotal) // histereza na nacisku klockow
            Ntotal = u * (2 - 1 * BrakeRigEff);
    }

    if (NBrakeAxles * NBpA > 0)
    {
        if (Ntotal > 0) // nie luz
            K = K + Ntotal; // w kN
        K = K * BrakeCylNo / (NBrakeAxles * NBpA); // w kN na os
    }
    if ((BrakeSystem == Pneumatic) || (BrakeSystem == ElectroPneumatic))
    {
        u = Hamulec->GetFC(Vel, K);
        UnitBrakeForce = u * K * 1000; // sila na jeden klocek w N
    }
    else
        UnitBrakeForce = K * 1000;
    if ((NBpA * UnitBrakeForce > TotalMassxg * Adhesive(RunningTrack.friction) / NAxles) &&
        (abs(V) > 0.001))
    // poslizg
    {
        //     Fb = Adhesive(Track.friction) * Mass * g;
        SlippingWheels = true;
    }
    //{  else
    //   begin
    //{     SlippingWheels:=false;}
    //     if (LocalBrake=ManualBrake)or(MBrake=true)) and (BrakePress<0.3) then
    //      Fb:=UnitBrakeForce*NBpA {ham. reczny dziala na jedna os}
    //     else  //yB: to nie do konca ma sens, poniewa¿ rêczny w wagonie dzia³a na jeden cylinder
    //     hamulcowy/wózek, dlatego potrzebne s¹ oddzielnie liczone osie
    Fb = UnitBrakeForce * NBrakeAxles * Max0R(1, NBpA);

    //  u:=((BrakePress*P2FTrans)-BrakeCylSpring*BrakeCylMult[BCMFlag]/BrakeCylNo-0.83*BrakeSlckAdj/(BrakeCylNo))*BrakeCylNo;
    // {  end; }
    return Fb;
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
double TMoverParameters::FrictionForce(double R, int TDamage)
{
    double FF = 0;
    // ABu 240205: chyba juz ekstremalnie zoptymalizowana funkcja liczaca sily tarcia
    if (abs(V) > 0.01)
        FF = (FrictConst1 * V * V) + FrictConst2d;
    else
        FF = (FrictConst1 * V * V) + FrictConst2s;
    return FF;
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
double TMoverParameters::Adhesive(double staticfriction)
{
    double adhesive;

    // ABu: male przerobki, tylko czy to da jakikolwiek skutek w FPS?
    //     w kazdym razie zaciemni kod na pewno :)
    if (SlippingWheels == false)
    {
        if (SandDose)
            adhesive = (Max0R(staticfriction * (100.0 + Vel) / ((50.0 + Vel) * 11.0), 0.048)) *
                       (11.0 - 2.0 * Random(0.0, 1.0));
        else
            adhesive = (staticfriction * (100.0 + Vel) / ((50.0 + Vel) * 10)) *
                       (11.0 - 2.0 * Random(0.0, 1.0));
    }
    else
    {
        if (SandDose)
            adhesive = (0.048) * (11 - 2 * Random(0.0, 1.0));
        else
            adhesive = (staticfriction * 0.02) * (11 - 2 * Random(0.0, 1.0));
    }
    //  WriteLog(FloatToStr(adhesive));      // tutaj jest na poziomie 0.2 - 0.3
    return adhesive;
}

// poprawka dla liczenia sil przy ustawieniu przeciwnym obiektow:
/*
double DirPatch(int Coupler1, int Coupler2)
{
  if (Coupler1 != Coupler2) return 1;
   else return -1;
}


double DirF(int CouplerN)
{
 double rDirF;
  switch (CouplerN)
  {
   case 0: return -1; break;
   case 1: return 1; break;
   default: return 0;
  }
 // if (CouplerN == 0) return -1;
//   else if (CouplerN == 0) return 1;
//    else return 0;
}
*/

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
double TMoverParameters::CouplerForce(int CouplerN, double dt)
{
    // wyliczenie si³y na sprzêgu
    double tempdist, newdist, distDelta, CF, dV, absdV, Fmax, BetaAvg;
    int CNext;
    const MaxDist = 405.0; // ustawione + 5 m, bo skanujemy do 400 m
    const MinDist = 0.5; // ustawione +.5 m, zeby nie rozlaczac przy malych odleglosciach
    const MaxCount = 1500;
    bool rCF;
    CF = 0;
    // distDelta:=0; //Ra: value never used
    CNext = Couplers[CouplerN].ConnectedNr;
    //  if (Couplers[CouplerN].CForce == 0)  //nie bylo uzgadniane wiec policz

    Couplers[CouplerN].CheckCollision = false;
    newdist = Couplers[CouplerN].CoupleDist; // odleg³oœæ od sprzêgu s¹siada
    // newdist:=Distance(Loc,Connected^.Loc,Dim,Connected^.Dim);
    if (CouplerN == 0)
    {
        // ABu: bylo newdist+10*((...
        tempdist = ((Couplers[CouplerN].Connected->dMoveLen *
                     DirPatch(0, Couplers[CouplerN].ConnectedNr)) -
                    dMoveLen);
        newdist = newdist + 10.0 * tempdist;
        // tempdist:=tempdist+CoupleDist; //ABu: proby szybkiego naprawienia bledu
    }
    else
    {
        // ABu: bylo newdist+10*((...
        tempdist = ((dMoveLen - (Couplers[CouplerN].Connected->dMoveLen *
                                 DirPatch(1, Couplers[CouplerN].ConnectedNr))));
        newdist = newdist + 10.0 * tempdist;
        // tempdist:=tempdist+CoupleDist; //ABu: proby szybkiego naprawienia bledu
    }

    // blablabla
    // ABu: proby znalezienia problemu ze zle odbijajacymi sie skladami
    //***if (Couplers[CouplerN].CouplingFlag=ctrain_virtual) and (newdist>0) then
    if ((Couplers[CouplerN].CouplingFlag == ctrain_virtual) && (Couplers[CouplerN].CoupleDist > 0))
    {
        CF = 0; // kontrola zderzania sie - OK
        ScanCounter++;
        if ((newdist > MaxDist) || ((ScanCounter > MaxCount) && (newdist > MinDist)))
        //***if (tempdist>MaxDist) or ((ScanCounter>MaxCount)and(tempdist>MinDist)) then
        { // zerwij kontrolnie wirtualny sprzeg
            // Connected.Couplers[CNext].Connected:=nil; //Ra: ten pod³¹czony niekoniecznie jest
            // wirtualny
            Couplers[CouplerN].Connected = NULL;
            ScanCounter = random(500); // Q: TODO: cy dobrze przetlumaczone?
            // WriteLog(FloatToStr(ScanCounter));
        }
    }
    else
    {
        if (Couplers[CouplerN].CouplingFlag == ctrain_virtual)
        {
            BetaAvg = Couplers[CouplerN].beta;
            Fmax = (Couplers[CouplerN].FmaxC + Couplers[CouplerN].FmaxB) * CouplerTune;
        }
        else // usrednij bo wspolny sprzeg
        {
            BetaAvg =
                (Couplers[CouplerN].beta + Couplers[CouplerN].Connected->Couplers[CNext].beta) /
                2.0;
            Fmax = (Couplers[CouplerN].FmaxC + Couplers[CouplerN].FmaxB +
                    Couplers[CouplerN].Connected->Couplers[CNext].FmaxC +
                    Couplers[CouplerN].Connected->Couplers[CNext].FmaxB) *
                   CouplerTune / 2.0;
        }
        dV = V - DirPatch(CouplerN, CNext) * Couplers[CouplerN].Connected->V;
        absdV = abs(dV);
        if ((newdist < -0.001) && (Couplers[CouplerN].Dist >= -0.001) &&
            (absdV > 0.010)) // 090503: dzwieki pracy zderzakow
        {
            if (SetFlag(SoundFlag, sound_bufferclamp))
                if (absdV > 0.5)
                    SetFlag(SoundFlag, sound_loud);
        }
        else if ((newdist > 0.002) && (Couplers[CouplerN].Dist <= 0.002) &&
                 (absdV > 0.005)) // 090503: dzwieki pracy sprzegu
        {
            if (Couplers[CouplerN].CouplingFlag > 0)
                if (SetFlag(SoundFlag, sound_couplerstretch))
                    if (absdV > 0.1)
                        SetFlag(SoundFlag, sound_loud);
        }
        distDelta =
            abs(newdist) - abs(Couplers[CouplerN].Dist); // McZapkie-191103: poprawka na histereze
        Couplers[CouplerN].Dist = newdist;
        if (Couplers[CouplerN].Dist > 0)
        {
            if (distDelta > 0)
                CF = (-(Couplers[CouplerN].SpringKC +
                        Couplers[CouplerN].Connected->Couplers[CNext].SpringKC) *
                      Couplers[CouplerN].Dist / 2.0) *
                         DirF(CouplerN) -
                     Fmax * dV * BetaAvg;
            else
                CF = (-(Couplers[CouplerN].SpringKC +
                        Couplers[CouplerN].Connected->Couplers[CNext].SpringKC) *
                      Couplers[CouplerN].Dist / 2.0) *
                         DirF(CouplerN) * BetaAvg -
                     Fmax * dV * BetaAvg;
            // liczenie sily ze sprezystosci sprzegu
            if (Couplers[CouplerN].Dist >
                (Couplers[CouplerN].DmaxC +
                 Couplers[CouplerN].Connected->Couplers[CNext].DmaxC)) // zderzenie
                //***if tempdist>(DmaxC+Connected^.Couplers[CNext].DmaxC) then {zderzenie}
                Couplers[CouplerN].CheckCollision = true;
        }
        if (Couplers[CouplerN].Dist < 0)
        {
            if (distDelta > 0)
                CF = (-(Couplers[CouplerN].SpringKB +
                        Couplers[CouplerN].Connected->Couplers[CNext].SpringKB) *
                      Couplers[CouplerN].Dist / 2.0) *
                         DirF(CouplerN) -
                     Fmax * dV * BetaAvg;
            else
                CF = (-(Couplers[CouplerN].SpringKB +
                        Couplers[CouplerN].Connected->Couplers[CNext].SpringKB) *
                      Couplers[CouplerN].Dist / 2.0) *
                         DirF(CouplerN) * BetaAvg -
                     Fmax * dV * BetaAvg;
            // liczenie sily ze sprezystosci zderzaka
            if (-Couplers[CouplerN].Dist >
                (Couplers[CouplerN].DmaxB +
                 Couplers[CouplerN].Connected->Couplers[CNext].DmaxB)) // zderzenie
            //***if -tempdist>(DmaxB+Connected^.Couplers[CNext].DmaxB)/10 then  {zderzenie}
            {
                Couplers[CouplerN].CheckCollision = true;
                if ((Couplers[CouplerN].CouplerType == Automatic) &&
                    (Couplers[CouplerN].CouplingFlag ==
                     0)) // sprzeganie wagonow z samoczynnymi sprzegami}
                    // CouplingFlag:=ctrain_coupler+ctrain_pneumatic+ctrain_controll+ctrain_passenger+ctrain_scndpneumatic;
                    Couplers[CouplerN].CouplingFlag =
                        ctrain_coupler + ctrain_pneumatic + ctrain_controll; // EN57
            }
        }
    }
    if (Couplers[CouplerN].CouplingFlag != ctrain_virtual)
        // uzgadnianie prawa Newtona
        Couplers[CouplerN].Connected->Couplers[1 - CouplerN].CForce = -CF;

    return CF;
}

// *************************************************************************************************
// Q: 20160714
// *************************************************************************************************
double TMoverParameters::TractionForce(double dt)
{ // oblicza sile trakcyjna lokomotywy (dla elektrowozu tez calkowity prad)
    double PosRatio, dmoment, dtrans, tmp, tmpV;
    int i;

    Ft = 0;
    dtrans = 0;
    dmoment = 0;
    // tmpV =Abs(nrot * WheelDiameter / 2);
    // youBy
    if (EngineType == DieselElectric)
    {
        tmp = DElist[MainCtrlPos].RPM / 60.0;
        if ((Heating) && (HeatingPower > 0) && (MainCtrlPosNo > MainCtrlPos))
        {
            i = MainCtrlPosNo;
            while (DElist[i - 2].RPM / 60.0 > tmp)
                i = i - 1;
            tmp = DElist[i].RPM / 60.0;
        }

        if (enrot != tmp * int(ConverterFlag))
            if (abs(tmp * int(ConverterFlag) - enrot) < 0.001)
                enrot = tmp * int(ConverterFlag);
            else if ((enrot < DElist[0].RPM * 0.01) && (ConverterFlag))
                enrot = enrot + (tmp * int(ConverterFlag) - enrot) * dt / 5.0;
            else
                enrot = enrot + (tmp * int(ConverterFlag) - enrot) * 1.5 * dt;
    }
    else if (EngineType != DieselEngine)
        enrot = Transmision.Ratio * nrot;
    else // dla DieselEngine
    {
        if (ShuntMode) // dodatkowa przek³adnia np. dla 2Ls150
            dtrans = AnPos * Transmision.Ratio * MotorParam[ScndCtrlActualPos].mIsat;
        else
            dtrans = Transmision.Ratio * MotorParam[ScndCtrlActualPos].mIsat;
        dmoment = dizel_Momentum(dizel_fill, ActiveDir * 1 * dtrans * nrot, dt); // oblicza tez
                                                                                 // enrot
    }

    eAngle = eAngle + enrot * dt;
    if (eAngle > Pirazy2)
        // eAngle = Pirazy2 - eAngle; <- ABu: a nie czasem tak, jak nizej?
        eAngle = eAngle - Pirazy2;

    // hunter-091012: przeniesione z if ActiveDir<>0 (zeby po zejsciu z kierunku dalej spadala
    // predkosc wentylatorow)
    if (EngineType == ElectricSeriesMotor)
    {
        switch (RVentType) // wentylatory rozruchowe}
        {
        case 1:
        {
            if (ActiveDir != 0)
                RventRot = RventRot + (RVentnmax - RventRot) * RVentSpeed * dt;
            else
                RventRot = RventRot * (1 - RVentSpeed * dt);
            break;
        }
        case 2:
        {
            if ((abs(Itot) > RVentMinI) && (RList[MainCtrlActualPos].R > RVentCutOff))
                RventRot =
                    RventRot +
                    (RVentnmax * abs(Itot) / (ImaxLo * RList[MainCtrlActualPos].Bn) - RventRot) *
                        RVentSpeed * dt;
            else if ((DynamicBrakeType == dbrake_automatic) && (DynamicBrakeFlag))
                RventRot = RventRot + (RVentnmax * Im / ImaxLo - RventRot) * RVentSpeed * dt;
            else
            {
                RventRot = RventRot * (1 - RVentSpeed * dt);
                if (RventRot < 0.1)
                    RventRot = 0;
            }
            break;
        }
        break;
        }
    }

    if (ActiveDir != 0)
        switch (EngineType)
        {
        case Dumb:
        {
            PosRatio = (MainCtrlPos + ScndCtrlPos) / (MainCtrlPosNo + ScndCtrlPosNo + 0.01);
            if (Mains && (ActiveDir != 0) && (CabNo != 0))
            {
                if (Vel > 0.1)
                {
                    Ft = Min0R(1000.0 * Power / abs(V), Ftmax) * PosRatio;
                }
                else
                    Ft = Ftmax * PosRatio;
                Ft = Ft * DirAbsolute; // ActiveDir*CabNo;
            }
            else
                Ft = 0;
            EnginePower = 1000 * Power * PosRatio;
            break;
        } // Dumb

        case WheelsDriven:
        {
            if (EnginePowerSource.SourceType == InternalSource)
                if (EnginePowerSource.PowerType == BioPower)
                    Ft = Sign(sin(eAngle)) * PulseForce * Transmision.Ratio;
            PulseForceTimer = PulseForceTimer + dt;
            if (PulseForceTimer > CtrlDelay)
            {
                PulseForce = 0;
                if (PulseForceCount > 0)
                    PulseForceCount--;
            }
            EnginePower = Ft * (1 + Vel);
            break;
        } // WheelsDriven

        case ElectricSeriesMotor:
        {
            //        enrot:=Transmision.Ratio*nrot;
            // yB: szereg dwoch sekcji w ET42
            if ((TrainType == dt_ET42) && (Imax == ImaxHi))
                Voltage = Voltage / 2.0;
            Mm = Momentum(current(enrot, Voltage)); // oblicza tez prad p/slinik

            // double qMm = Momentum(qcurrent(enrot, Voltage));  // Q: funkcja w C++ - porownanie
            // poprawnosci

            if (TrainType == dt_ET42)
            {
                if (Imax == ImaxHi)
                    Voltage = Voltage * 2;
                if ((DynamicBrakeFlag) && (abs(Im) > 300)) // przeiesione do mover.cpp
                    FuseOff();
            }
            if ((DynamicBrakeType == dbrake_automatic) && (DynamicBrakeFlag))
            {
                if (((Vadd + abs(Im)) > 760) || (static_cast<TLSt *>(Hamulec)->GetEDBCP() < 0.25))
                {
                    Vadd = Vadd - 500 * dt;
                    if (Vadd < 1)
                        Vadd = 0;
                }
                else if ((DynamicBrakeFlag) && ((Vadd + abs(Im)) < 740))
                {
                    Vadd = Vadd + 70 * dt;
                    Vadd = Min0R(Max0R(Vadd, 60), 400);
                }
                if (Vadd > 0)
                    Mm = MomentumF(Im, Vadd, 0);
            }

            if ((TrainType == dt_ET22) && (DelayCtrlFlag)) // szarpanie przy zmianie uk³adu w byku
                Mm = Mm * RList[MainCtrlActualPos].Bn /
                     (RList[MainCtrlActualPos].Bn +
                      1); // zrobione w momencie, ¿eby nie dawac elektryki w przeliczaniu si³

            if (abs(Im) > Imax)
                Vhyp = Vhyp + dt * (abs(Im) / Imax - 0.9) * 10; // zwieksz czas oddzialywania na PN
            else
                Vhyp = 0;
            if (Vhyp > CtrlDelay / 2) // jesli czas oddzialywania przekroczony
                FuseOff(); // wywalanie bezpiecznika z powodu przetezenia silnikow

            //-if (Mains) then //nie wchodziæ w funkcjê bez potrzeby
            //- if (Abs(Voltage)<EnginePowerSource.CollectorParameters.MinV) or
            //(Abs(Voltage)>EnginePowerSource.CollectorParameters.MaxV) then
            //-  if MainSwitch(false) then
            //-   EventFlag:=true; //wywalanie szybkiego z powodu niew³aœciwego napiêcia

            if (((DynamicBrakeType == dbrake_automatic) || (DynamicBrakeType == dbrake_switch)) &&
                (DynamicBrakeFlag))
                Itot = Im * 2; // 2x2 silniki w EP09
            else if ((TrainType == dt_EZT) && (Imin == IminLo) &&
                     (ScndS)) // yBARC - boczniki na szeregu poprawnie
                Itot = Im;
            else
                Itot = Im * RList[MainCtrlActualPos].Bn; // prad silnika * ilosc galezi
            Mw = Mm * Transmision.Ratio;
            Fw = Mw * 2.0 / WheelDiameter;
            Ft = Fw * NPoweredAxles; // sila trakcyjna
            break;
        }

        case DieselEngine:
        {
            EnginePower = dmoment * enrot;
            if (MainCtrlPos > 1)
                dmoment =
                    dmoment -
                    dizel_Mstand * (0.2 * enrot / nmax); // dodatkowe opory z powodu sprezarki}
            Mm = dizel_engage * dmoment;
            Mw = Mm * dtrans; // dmoment i dtrans policzone przy okazji enginerotation
            Fw = Mw * 2.0 / WheelDiameter;
            Ft = Fw * NPoweredAxles; // sila trakcyjna
            Ft = Ft * DirAbsolute; // ActiveDir*CabNo;
            break;
        }
            /*
               case DieselElectric:    //youBy
                 {
            //       tmpV:=V*CabNo*ActiveDir;
                     tmpV =nrot*Pirazy2*0.5*WheelDiameter*DirAbsolute; //*CabNo*ActiveDir;
                   //jazda manewrowa
                   if (ShuntMode)
                    {
                     Voltage =(SST[MainCtrlPos].Umax * AnPos) + (SST[MainCtrlPos].Umin * (1 -
            AnPos));
                     tmp = (SST[MainCtrlPos].Pmax * AnPos) + (SST[MainCtrlPos].Pmin * (1 - AnPos));
                     Ft = tmp * 1000.0 / (abs(tmpV)+1.6);
                     PosRatio = 1;
                    }
                   else  //jazda ciapongowa
                   {

                    tmp = Min0R(DElist[MainCtrlPos].GenPower,Power-HeatingPower*int(Heating));

                    PosRatio = DElist[MainCtrlPos].GenPower / DElist[MainCtrlPosNo].GenPower;
            //stosunek mocy teraz do mocy max
                    if ((MainCtrlPos>0) && (ConverterFlag))
                      if (tmpV <
            (Vhyp*(Power-HeatingPower*int(Heating))/DElist[MainCtrlPosNo].GenPower)) //czy na czesci
            prostej, czy na hiperboli
                        Ft = (Ftmax - ((Ftmax - 1000.0 * DElist[MainCtrlPosNo].GenPower /
            (Vhyp+Vadd)) * (tmpV/Vhyp) / PowerCorRatio)) * PosRatio; //posratio - bo sila jakos tam
            sie rozklada
                        //Ft:=(Ftmax - (Ftmax - (1000.0 * DEList[MainCtrlPosNo].genpower /
            (Vhyp+Vadd) / PowerCorRatio)) * (tmpV/Vhyp)) * PosRatio //wersja z Megapacka
                      else //na hiperboli                             //1.107 - wspolczynnik
            sredniej nadwyzki Ft w symku nad charakterystyka
                        Ft = 1000.0 * tmp / (tmpV+Vadd) / PowerCorRatio; //tu jest zawarty stosunek
            mocy
                    else Ft = 0; //jak nastawnik na zero, to sila tez zero

                    PosRatio = tmp/DElist[MainCtrlPosNo].GenPower;

                   }

                    if (FuseFlag) Ft = 0; else
                    Ft =Ft*DirAbsolute; //ActiveDir * CabNo; //zwrot sily i jej wartosc
                    Fw =Ft/NPoweredAxles; //sila na obwodzie kola
                    Mw =Fw*WheelDiameter / 2.0; // moment na osi kola
                    Mm =Mw/Transmision.Ratio; // moment silnika trakcyjnego


                   //with MotorParam[ScndCtrlPos] do
                     if (abs(Mm) > MotorParam[ScndCtrlPos].fi)
                       Im = NPoweredAxles * abs(abs(Mm) / MotorParam[ScndCtrlPos].mfi +
            MotorParam[ScndCtrlPos].mIsat);
                     else
                       Im = NPoweredAxles * sqrt(abs(Mm * MotorParam[ScndCtrlPos].Isat));

                   if (ShuntMode)
                   {
                     EnginePower = Voltage * Im/1000.0;
                     if (EnginePower > tmp)
                     {
                       EnginePower = tmp*1000.0;
                       Voltage = EnginePower / Im;
                     }
                     if (EnginePower < tmp)
                     Ft = Ft * EnginePower / tmp;
                   }
                   else
                   {
                    if (abs(Im) > DElist[MainCtrlPos].Imax)
                    { //nie ma nadmiarowego, tylko Imax i zwarcie na pradnicy
                      Ft = Ft/Im*DElist[MainCtrlPos].Imax;
                      Im = DElist[MainCtrlPos].Imax;
                    }

                    if (Im > 0)  //jak pod obciazeniem
                      if (Flat)  //ograniczenie napiecia w pradnicy - plaszczak u gory
                        Voltage = 1000.0 * tmp / abs(Im);
                      else  //charakterystyka pradnicy obcowzbudnej (elipsa) - twierdzenie
            Pitagorasa
                      {                  //sqr()   TODO: sqr
                        Voltage = sqrt(abs(sqrt(DElist[MainCtrlPos].Umax) -
            sqrt(DElist[MainCtrlPos].Umax*Im/DElist[MainCtrlPos].Imax)))*(MainCtrlPos-1)+
                                 (1-Im/DElist[MainCtrlPos].Imax)*DElist[MainCtrlPos].Umax*(MainCtrlPosNo-MainCtrlPos);
                        Voltage = Voltage/(MainCtrlPosNo-1);
                        Voltage = Min0R(Voltage,(1000.0 * tmp / abs(Im)));
                        if (Voltage<(Im*0.05)) Voltage = Im*0.05;
                      }
                    if ((Voltage > DElist[MainCtrlPos].Umax) || (Im == 0)) //gdy wychodzi za duze
            napiecie
                      Voltage = DElist[MainCtrlPos].Umax * int(ConverterFlag);     //albo przy biegu
            jalowym (jest cos takiego?)

                    EnginePower = Voltage * Im / 1000.0;

                    if ((tmpV > 2) && (EnginePower < tmp))
                      Ft =Ft*EnginePower/tmp;

                   }

                     //--if (Imax>1) and (Im>Imax) then FuseOff;  //przeniesione do mover.cpp
                     if (FuseFlag) Voltage = 0;

                 //przekazniki bocznikowania, kazdy inny dla kazdej pozycji
                     if ((MainCtrlPos == 0) || (ShuntMode))
                       ScndCtrlPos = 0;

                     else if (AutoRelayFlag)
                      switch (RelayType)
                      {
                      case 0:
                         {
                          if (Im <= (MPTRelay[ScndCtrlPos].Iup*PosRatio)) and
            (ScndCtrlPos<ScndCtrlPosNo) then
                            inc(ScndCtrlPos);
                          if (Im >= (MPTRelay[ScndCtrlPos].Idown*PosRatio)) and (ScndCtrlPos>0) then
                            dec(ScndCtrlPos);
                         }
                      case 1:
                         {
                          if (MPTRelay[ScndCtrlPos].Iup<Vel) and (ScndCtrlPos<ScndCtrlPosNo) then
                            inc(ScndCtrlPos);
                          if (MPTRelay[ScndCtrlPos].Idown>Vel) and (ScndCtrlPos>0) then
                            dec(ScndCtrlPos);
                         }
                      case 2:
                         {
                          if (MPTRelay[ScndCtrlPos].Iup<Vel) and (ScndCtrlPos<ScndCtrlPosNo) and
            (EnginePower<(tmp*0.99)) then
                            inc(ScndCtrlPos);
                          if (MPTRelay[ScndCtrlPos].Idown<Im) and (ScndCtrlPos>0) then
                            dec(ScndCtrlPos);
                         }
                      case 41:
                         {
                          if (MainCtrlPos=MainCtrlPosNo) and (tmpV*3.6>MPTRelay[ScndCtrlPos].Iup)
            and (ScndCtrlPos<ScndCtrlPosNo)then
                            begin inc(ScndCtrlPos); enrot:=enrot*0.73; end;
                          if (Im>MPTRelay[ScndCtrlPos].Idown)and (ScndCtrlPos>0) then
                            dec(ScndCtrlPos);
                         }
                      case 45:
                         {
                          //wzrastanie
                          if (MainCtrlPos>11) and (ScndCtrlPos<ScndCtrlPosNo) then
                           if (ScndCtrlPos=0) then
                            if (MPTRelay[ScndCtrlPos].Iup>Im) then
                             inc(ScndCtrlPos)
                            else
                           else
                            if (MPTRelay[ScndCtrlPos].Iup<Vel) then
                             inc(ScndCtrlPos);

                          //malenie
                          if(ScndCtrlPos>0)and(MainCtrlPos<12)then
                          if (ScndCtrlPos=ScndCtrlPosNo)then
                            if (MPTRelay[ScndCtrlPos].Idown<Im)then
                             dec(ScndCtrlPos)
                            else
                           else
                            if (MPTRelay[ScndCtrlPos].Idown>Vel)then
                             dec(ScndCtrlPos);
                          if (MainCtrlPos<11)and(ScndCtrlPos>2) then ScndCtrlPos:=2;
                          if (MainCtrlPos<9)and(ScndCtrlPos>0) then ScndCtrlPos:=0;
                         }
                      case 46:
                         {
                          //wzrastanie
                          if (MainCtrlPos>9) and (ScndCtrlPos<ScndCtrlPosNo) then
                           if (ScndCtrlPos) mod 2 = 0 then
                            if (MPTRelay[ScndCtrlPos].Iup>Im) then
                             inc(ScndCtrlPos)
                            else
                           else
                            if (MPTRelay[ScndCtrlPos-1].Iup>Im) and (MPTRelay[ScndCtrlPos].Iup<Vel)
            then
                             inc(ScndCtrlPos);

                          //malenie
                          if (MainCtrlPos<10) and (ScndCtrlPos>0)then
                           if (ScndCtrlPos) mod 2 = 0 then
                            if (MPTRelay[ScndCtrlPos].Idown<Im)then
                             dec(ScndCtrlPos)
                            else
                           else
                            if (MPTRelay[ScndCtrlPos+1].Idown<Im) and
            (MPTRelay[ScndCtrlPos].Idown>Vel)then
                             dec(ScndCtrlPos);
                          if (MainCtrlPos<9)and(ScndCtrlPos>2) then ScndCtrlPos:=2;
                          if (MainCtrlPos<6)and(ScndCtrlPos>0) then ScndCtrlPos:=0;
                        }
                    } //switch RelayType
                 } // DieselElectric
            */
            /*
               case ElectricInductionMotor:
                 {
                   if (Voltage>1800) and (MainS) then
                    begin

                     if ((Hamulec as TLSt).GetEDBCP<0.25)and(LocHandle.GetCP<0.25) then
                       DynamicBrakeFlag:=false
                     else if ((BrakePress>0.25) and ((Hamulec as TLSt).GetEDBCP>0.25)) or
            (LocHandle.GetCP>0.25) then
                       DynamicBrakeFlag:=true;

                     if(DynamicBrakeFlag)then
                      begin
                       if eimv[eimv_Fmax]*sign(V)*DirAbsolute<-1 then
                         PosRatio:=-sign(V)*DirAbsolute*eimv[eimv_Fr]/(eimc[eimc_p_Fh]*Max0R((Hamulec
            as TLSt).GetEDBCP,LocHandle.GetCP)/MaxBrakePress[0]{dizel_fill})
                       else
                         PosRatio:=0;
                       PosRatio:=round(20*Posratio)/20;
                       if PosRatio<19.5/20 then PosRatio:=PosRatio*0.9;
                       (Hamulec as TLSt).SetED(PosRatio);
                       PosRatio:=-Max0R((Hamulec as
            TLSt).GetEDBCP,LocHandle.GetCP)/MaxBrakePress[0]*Max0R(0,Min0R(1,(Vel-eimc[eimc_p_Vh0])/(eimc[eimc_p_Vh1]-eimc[eimc_p_Vh0])));
                       tmp:=5;
                      end
                     else
                      begin
                       PosRatio:=(MainCtrlPos/MainCtrlPosNo);
                       PosRatio:=1.0*(PosRatio*0+1)*PosRatio;
                       (Hamulec as TLSt).SetED(0);
                       if (PosRatio>dizel_fill) then tmp:=1 else tmp:=4; //szybkie malenie, powolne
            wzrastanie
                      end;
                     if SlippingWheels
                      then
                       begin
                        PosRatio:=0;
                        tmp:=10;
                        //--SandDoseOn;       // przeniesione do mover.cpp
                       end;//przeciwposlizg

                     dizel_fill:=dizel_fill+Max0R(Min0R(PosRatio-dizel_fill,0.1),-0.1)*2*(tmp{2{+4*int(PosRatio<dizel_fill)})*dt;
            //wartoœæ zadana/procent czegoœ

                     if(DynamicBrakeFlag)then tmp:=eimc[eimc_f_Uzh] else tmp:=eimc[eimc_f_Uzmax];

                     eimv[eimv_Uzsmax]:=Min0R(Voltage-eimc[eimc_f_DU],tmp);
                     eimv[eimv_fkr]:=eimv[eimv_Uzsmax]/eimc[eimc_f_cfu];
                     if(DynamicBrakeFlag)then
                       eimv[eimv_Pmax]:=eimc[eimc_p_Ph]
                     else
                       eimv[eimv_Pmax]:=Min0R(eimc[eimc_p_Pmax],0.001*Voltage*(eimc[eimc_p_Imax]-eimc[eimc_f_I0])*Pirazy2*eimc[eimc_s_cim]/eimc[eimc_s_p]/eimc[eimc_s_cfu]);

                     eimv[eimv_FMAXMAX]:=0.001*SQR(Min0R(eimv[eimv_fkr]/Max0R(abs(enrot)*eimc[eimc_s_p]+eimc[eimc_s_dfmax]*eimv[eimv_ks],eimc[eimc_s_dfmax]),1)*eimc[eimc_f_cfu]/eimc[eimc_s_cfu])*(eimc[eimc_s_dfmax]*eimc[eimc_s_dfic]*eimc[eimc_s_cim])*Transmision.Ratio*NPoweredAxles*2/WheelDiameter;
                     if(DynamicBrakeFlag)then
            //           if(Vel>eimc[eimc_p_Vh0])then
                        eimv[eimv_Fmax]:=-sign(V)*(DirAbsolute)*Min0R(eimc[eimc_p_Ph]*3.6/Vel,-eimc[eimc_p_Fh]*dizel_fill)//*Min0R(1,(Vel-eimc[eimc_p_Vh0])/(eimc[eimc_p_Vh1]-eimc[eimc_p_Vh0]))
            //           else
            //            eimv[eimv_Fmax]:=0
                     else
                       eimv[eimv_Fmax]:=Min0R(Min0R(3.6*eimv[eimv_Pmax]/Max0R(Vel,1),eimc[eimc_p_F0]-Vel*eimc[eimc_p_a1]),eimv[eimv_FMAXMAX])*dizel_fill;

                     eimv[eimv_ks]:=eimv[eimv_Fmax]/eimv[eimv_FMAXMAX];
                     eimv[eimv_df]:=eimv[eimv_ks]*eimc[eimc_s_dfmax];
                     eimv[eimv_fp]:=DirAbsolute*enrot*eimc[eimc_s_p]+eimv[eimv_df];
            //
            eimv[eimv_U]:=Max0R(eimv[eimv_Uzsmax],Min0R(eimc[eimc_f_cfu]*eimv[eimv_fp],eimv[eimv_Uzsmax]));
            //         eimv[eimv_pole]:=eimv[eimv_U]/(eimv[eimv_fp]*eimc[eimc_s_cfu]);
                     if(abs(eimv[eimv_fp])<=eimv[eimv_fkr])then
                       eimv[eimv_pole]:=eimc[eimc_f_cfu]/eimc[eimc_s_cfu]
                     else
                       eimv[eimv_pole]:=eimv[eimv_Uzsmax]/eimc[eimc_s_cfu]/abs(eimv[eimv_fp]);
                     eimv[eimv_U]:=eimv[eimv_pole]*eimv[eimv_fp]*eimc[eimc_s_cfu];
                     eimv[eimv_Ic]:=(eimv[eimv_fp]-DirAbsolute*enrot*eimc[eimc_s_p])*eimc[eimc_s_dfic]*eimv[eimv_pole];
                     eimv[eimv_If]:=eimv[eimv_Ic]*eimc[eimc_s_icif];
                     eimv[eimv_M]:=eimv[eimv_pole]*eimv[eimv_Ic]*eimc[eimc_s_cim];
                     eimv[eimv_Ipoj]:=(eimv[eimv_Ic]*NPoweredAxles*eimv[eimv_U])/(Voltage-eimc[eimc_f_DU])+eimc[eimc_f_I0];
                     eimv[eimv_Pm]:=ActiveDir*eimv[eimv_M]*NPoweredAxles*enrot*Pirazy2/1000;
                     eimv[eimv_Pe]:=eimv[eimv_Ipoj]*Voltage/1000;
                     eimv[eimv_eta]:=eimv[eimv_Pm]/eimv[eimv_Pe];

                     Im:=eimv[eimv_If];
                     Itot:=eimv[eimv_Ipoj];


                     EnginePower:=Abs(eimv[eimv_Ic]*eimv[eimv_U]*NPoweredAxles)/1000;
                     tmpV:=eimv[eimv_fp];
                     if (abs(eimv[eimv_If])>1) or (abs(tmpV)>0.1) then
                      begin
                       if abs(tmpV)<32 then RventRot:=1.0 else
                       if abs(tmpV)<39 then RventRot:=0.33*abs(tmpV)/32 else
                         RventRot:=0.25*abs(tmpV)/39;//}
                      end
                     else              RVentRot:=0;

                     Mm:=eimv[eimv_M]*DirAbsolute;
                     Mw:=Mm*Transmision.Ratio;
                     Fw:=Mw*2.0/WheelDiameter;
                     Ft:=Fw*NPoweredAxles;
                     eimv[eimv_Fr]:=DirAbsolute*Ft/1000;
            //       RventRot;
                    end
                   else
                    begin
                     Im:=0;Mm:=0;Mw:=0;Fw:=0;Ft:=0;Itot:=0;dizel_fill:=0;EnginePower:=0;
                     (Hamulec as TLSt).SetED(0);RventRot:=0.0;
                    end;
                 }  // ElectricInductionMotor
            */

            // None: begin end;
        } // case EngineType
    return Ft;
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
double TMoverParameters::ComputeRotatingWheel(double WForce, double dt, double n)
{
    double newn, eps;
    bool CRW;
    if ((n == 0) && (WForce * Sign(V) < 0))
        newn = 0;
    else
    {
        eps = WForce * WheelDiameter / (2.0 * AxleInertialMoment);
        newn = n + eps * dt;
        if ((newn * n <= 0) && (eps * n < 0))
            newn = 0;
    }
    CRW = newn;
    return CRW;
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
bool TMoverParameters::FuseFlagCheck(void)
{
    int b;
    bool FFC;

    FFC = false;
    if (Power > 0.01)
        FFC = FuseFlag;
    // Q: TODO: zakomentowalem bo nie widzi funkcji
    //- else               //pobor pradu jezeli niema mocy
    //-  for (b=0; b < 1; b++)
    //-    if (TestFlag(Couplers[b].CouplingFlag, ctrain_controll))
    //-     if (Couplers[b].Connected->Power > 0.01)
    //-      FFC = Couplers[b].Connected->FuseFlagCheck();

    return FFC;
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
bool TMoverParameters::FuseOn(void)
{
    bool FO = false;
    if ((MainCtrlPos == 0) && (ScndCtrlPos == 0) && (TrainType != dt_ET40) && Mains)
    { // w ET40 jest blokada nastawnika, ale czy dzia³a dobrze?
        SendCtrlToNext("FuseSwitch", 1, CabNo);
        if (((EngineType == ElectricSeriesMotor) || ((EngineType == DieselElectric))) && FuseFlag)
        {
            FuseFlag = false; // wlaczenie ponowne obwodu
            FO = true;
            SetFlag(SoundFlag, sound_relay);
            SetFlag(SoundFlag, sound_loud);
        }
    }
    return FO;
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
void TMoverParameters::FuseOff(void)
{
    if (!FuseFlag)
    {
        FuseFlag = true;
        EventFlag = true;
        SetFlag(SoundFlag, sound_relay);
        SetFlag(SoundFlag, sound_loud);
    }
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
double TMoverParameters::v2n(void)
{
    // przelicza predkosc liniowa na obrotowa
    const dmgn = 0.5;
    double n, deltan;

    n = V / (PI * WheelDiameter); // predkosc obrotowa wynikajaca z liniowej [obr/s]
    deltan = n - nrot; //"pochodna" prêdkoœci obrotowej
    if (SlippingWheels)
        if (abs(deltan) < 0.01)
            SlippingWheels = false; // wygaszenie poslizgu
    if (SlippingWheels) // nie ma zwiazku z predkoscia liniowa V
    { // McZapkie-221103: uszkodzenia kol podczas poslizgu
        if (deltan > dmgn)
            if (FuzzyLogic(deltan, dmgn, p_slippdmg))
                if (SetFlag(DamageFlag, dtrain_wheelwear)) // podkucie
                    EventFlag = true;
        if (deltan < -dmgn)
            if (FuzzyLogic(-deltan, dmgn, p_slippdmg))
                if (SetFlag(DamageFlag, dtrain_thinwheel)) // wycieranie sie obreczy
                    EventFlag = true;
        n = nrot; // predkosc obrotowa nie zalezy od predkosci liniowej
    }
    return n;
}

// *************************************************************************************************
// Q: 20160714
// *************************************************************************************************
double TMoverParameters::Momentum(double I)
{
    // liczy moment sily wytwarzany przez silnik elektryczny}
    int SP;

    SP = ScndCtrlActualPos;
    if (ScndInMain)
        if (!(RList[MainCtrlActualPos].ScndAct == 255))
            SP = RList[MainCtrlActualPos].ScndAct;

    //     Momentum:=mfi*I*(1-1.0/(Abs(I)/mIsat+1));
    return (MotorParam[SP].mfi * I *
            (abs(I) / (abs(I) + MotorParam[SP].mIsat) - MotorParam[SP].mfi0));
}

// *************************************************************************************************
// Q: 20160714
// *************************************************************************************************
double TMoverParameters::MomentumF(double I, double Iw, int SCP)
{
    // umozliwia dokladne sterowanie wzbudzeniem

    return (MotorParam[SCP].mfi * I *
            Max0R(abs(Iw) / (abs(Iw) + MotorParam[SCP].mIsat) - MotorParam[SCP].mfi0, 0));
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
bool TMoverParameters::CutOffEngine(void)
{
    bool COE = false; // Ra: wartoœæ domyœlna, sprawdziæ to trzeba
    if ((NPoweredAxles > 0) && (CabNo == 0) && (EngineType == ElectricSeriesMotor))
    {
        if (SetFlag(DamageFlag, -dtrain_engine))
        {
            NPoweredAxles = NPoweredAxles / 2; // bylo div czyli mod?
            COE = true;
        }
    }
    return COE;
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
bool TMoverParameters::MaxCurrentSwitch(bool State)
{
    bool MCS = false;
    if (EngineType == ElectricSeriesMotor)
        if (ImaxHi > ImaxLo)
        {
            if (State && (Imax == ImaxLo) && (RList[MainCtrlPos].Bn < 2) &&
                !((TrainType == dt_ET42) && (MainCtrlPos > 0)))
            {
                Imax = ImaxHi;
                MCS = true;
                if (CabNo != 0)
                    SendCtrlToNext("MaxCurrentSwitch", 1, CabNo);
            }
            if (!State)
                if (Imax == ImaxHi)
                    if (!((TrainType == dt_ET42) && (MainCtrlPos > 0)))
                    {
                        Imax = ImaxLo;
                        MCS = true;
                        if (CabNo != 0)
                            SendCtrlToNext("MaxCurrentSwitch", 0, CabNo);
                    }
        }
    return MCS;
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
bool TMoverParameters::MinCurrentSwitch(bool State)
{
    bool MCS = false;
    if (((EngineType == ElectricSeriesMotor) && (IminHi > IminLo)) || (TrainType == dt_EZT))
    {
        if (State && (Imin == IminLo))
        {
            Imin = IminHi;
            MCS = true;
            if (CabNo != 0)
                SendCtrlToNext("MinCurrentSwitch", 1, CabNo);
        }
        if ((!State) && (Imin == IminHi))
        {
            Imin = IminLo;
            MCS = true;
            if (CabNo != 0)
                SendCtrlToNext("MinCurrentSwitch", 0, CabNo);
        }
    }
    return MCS;
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
bool TMoverParameters::ResistorsFlagCheck(void)
{
    int b;
    bool RFC = false;

    if (Power > 0.01)
        RFC = ResistorsFlag;
    // Q: TODO: zakomentowalem bo nie widzi funkcji, do czasu przenesienia TCoupling do mover.h
    else // pobor pradu jezeli niema mocy
        RFC = false; // po przeniesieniu usunac te linie
    //-   for (b=0; b<1; b++)
    //-    if (TestFlag(Couplers[b].CouplingFlag, ctrain_controll))
    //-     if (Couplers[b].Connected->Power > 0.01)
    //-       RFC = Couplers[b].Connected->ResistorsFlagCheck();
    return RFC;
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
bool TMoverParameters::AutoRelaySwitch(bool State)
{
    bool ARS;
    if ((AutoRelayType == 2) && (AutoRelayFlag != State))
    {
        AutoRelayFlag = State;
        ARS = true;
        SendCtrlToNext("AutoRelaySwitch", int(State), CabNo);
    }
    else
        ARS = false;

    return ARS;
}

// *************************************************************************************************
// Q: 20160724
// *************************************************************************************************

bool TMoverParameters::AutoRelayCheck(void)
{
    bool OK; // b:int;
    bool ARFASI, ARFASI2; // sprawdzenie wszystkich warunkow (AutoRelayFlag, AutoSwitch, Im<Imin)
    bool ARC;

    // Ra 2014-06: dla SN61 nie dzia³a prawid³owo
    // rozlaczanie stycznikow liniowych
    if ((!Mains) || (FuseFlag) || (MainCtrlPos == 0) || (BrakePress > 2.1) ||
        (ActiveDir == 0)) // hunter-111211: wylacznik cisnieniowy
    {
        StLinFlag = false; // yBARC - rozlaczenie stycznikow liniowych
        OK = false;
        if (!DynamicBrakeFlag)
        {
            Im = 0;
            Itot = 0;
            ResistorsFlag = false;
        }
    }

    ARFASI2 = ((!AutoRelayFlag) || ((MotorParam[ScndCtrlActualPos].AutoSwitch) &&
                                    (abs(Im) < Imin))); // wszystkie warunki w jednym
    ARFASI = ((!AutoRelayFlag) || ((RList[MainCtrlActualPos].AutoSwitch) && (abs(Im) < Imin)) ||
              ((!RList[MainCtrlActualPos].AutoSwitch) &&
               (RList[MainCtrlActualPos].Relay < MainCtrlPos))); // wszystkie warunki w jednym
    // brak PSR                   na tej pozycji dzia³a PSR i pr¹d poni¿ej progu
    // na tej pozycji nie dzia³a PSR i pozycja walu ponizej
    //                         chodzi w tym wszystkim o to, ¿eby mo¿na by³o zatrzymaæ rozruch na
    //                         jakiejœ pozycji wpisuj¹c Autoswitch=0 i wymuszaæ
    //                         przejœcie dalej przez danie nastawnika na dalsz¹ pozycjê - tak to do
    //                         tej pory dzia³a³o i na tym siê opiera fizyka ET22-2k
    {
        if (StLinFlag)
        {
            if ((RList[MainCtrlActualPos].R == 0) &&
                ((ScndCtrlActualPos > 0) || (ScndCtrlPos > 0)) &&
                (!(CoupledCtrl) || (RList[MainCtrlActualPos].Relay == MainCtrlPos)))
            { // zmieniaj scndctrlactualpos
                //{                                              //scnd bez samoczynnego rozruchu
                if (ScndCtrlActualPos < ScndCtrlPos)
                {
                    if ((LastRelayTime > CtrlDelay) && (ARFASI2))
                    {
                        ScndCtrlActualPos++;
                        OK = true;
                    }
                }
                else if (ScndCtrlActualPos > ScndCtrlPos)
                {
                    if ((LastRelayTime > CtrlDownDelay) && (TrainType != dt_EZT))
                    {
                        ScndCtrlActualPos--;
                        OK = true;
                    }
                }
                else
                    OK = false;
                // }
            }
            else
            { // zmieniaj mainctrlactualpos
                if ((ActiveDir < 0) && (TrainType != dt_PseudoDiesel))
                    if (RList[MainCtrlActualPos + 1].Bn > 1)
                    {
                        OK = false;
                        // return ARC;// bbylo exit; //Ra: to powoduje, ¿e EN57 nie wy³¹cza siê przy
                        // IminLo
                    }
                { // main bez samoczynnego rozruchu

                    if ((RList[MainCtrlActualPos].Relay < MainCtrlPos) ||
                        (RList[MainCtrlActualPos + 1].Relay == MainCtrlPos) ||
                        ((TrainType == dt_ET22) && (DelayCtrlFlag)))
                    {
                        if ((RList[MainCtrlPos].R == 0) && (MainCtrlPos > 0) &&
                            (!(MainCtrlPos == MainCtrlPosNo)) && (FastSerialCircuit == 1))
                        {
                            MainCtrlActualPos++;
                            //                 MainCtrlActualPos:=MainCtrlPos; //hunter-111012:
                            //                 szybkie wchodzenie na bezoporowa (303E)
                            OK = true;
                            SetFlag(SoundFlag, sound_manyrelay);
                            SetFlag(SoundFlag, sound_loud);
                        }
                        else if ((LastRelayTime > CtrlDelay) && (ARFASI))
                        {
                            // WriteLog("LRT = " + FloatToStr(LastRelayTime) + ", " +
                            // FloatToStr(CtrlDelay));
                            if ((TrainType == dt_ET22) && (MainCtrlPos > 1) &&
                                ((RList[MainCtrlActualPos].Bn < RList[MainCtrlActualPos + 1].Bn) ||
                                 (DelayCtrlFlag))) // et22 z walem grupowym
                                if (!DelayCtrlFlag) // najpierw przejscie
                                {
                                    MainCtrlActualPos++;
                                    DelayCtrlFlag = true; // tryb przejscia
                                    OK = true;
                                }
                                else if (LastRelayTime > 4 * CtrlDelay) // przejscie
                                {

                                    DelayCtrlFlag = false;
                                    OK = true;
                                }
                                else
                                    ;
                            else // nie ET22 z wa³em grupowym
                            {
                                MainCtrlActualPos++;
                                OK = true;
                            }
                            //---------
                            // hunter-111211: poprawki
                            if (MainCtrlActualPos > 0)
                                if ((RList[MainCtrlActualPos].R == 0) &&
                                    (!(MainCtrlActualPos ==
                                       MainCtrlPosNo))) // wejscie na bezoporowa
                                {
                                    SetFlag(SoundFlag, sound_manyrelay);
                                    SetFlag(SoundFlag, sound_loud);
                                }
                                else if ((RList[MainCtrlActualPos].R > 0) &&
                                         (RList[MainCtrlActualPos - 1].R ==
                                          0)) // wejscie na drugi uklad
                                {
                                    SetFlag(SoundFlag, sound_manyrelay);
                                }
                        }
                    }
                    else if (RList[MainCtrlActualPos].Relay > MainCtrlPos)
                    {
                        if ((RList[MainCtrlPos].R == 0) && (MainCtrlPos > 0) &&
                            (!(MainCtrlPos == MainCtrlPosNo)) && (FastSerialCircuit == 1))
                        {
                            MainCtrlActualPos--;
                            //                 MainCtrlActualPos:=MainCtrlPos; //hunter-111012:
                            //                 szybkie wchodzenie na bezoporowa (303E)
                            OK = true;
                            SetFlag(SoundFlag, sound_manyrelay);
                        }
                        else if (LastRelayTime > CtrlDownDelay)
                        {
                            if (TrainType != dt_EZT) // tutaj powinien byæ tryb sterowania wa³em
                            {
                                MainCtrlActualPos--;
                                OK = true;
                            }
                            if (MainCtrlActualPos > 0) // hunter-111211: poprawki
                                if (RList[MainCtrlActualPos].R ==
                                    0) // dzwieki schodzenia z bezoporowej}
                                {
                                    SetFlag(SoundFlag, sound_manyrelay);
                                }
                        }
                    }
                    else if ((RList[MainCtrlActualPos].R > 0) && (ScndCtrlActualPos > 0))
                    {
                        if (LastRelayTime > CtrlDownDelay)
                        {
                            ScndCtrlActualPos--; // boczniki nie dzialaja na poz. oporowych
                            OK = true;
                        }
                    }
                    else
                        OK = false;
                }
            }
        }
        else // not StLinFlag
        {
            OK = false;
            // ybARC - tutaj sa wszystkie warunki, jakie musza byc spelnione, zeby mozna byla
            // zalaczyc styczniki liniowe
            if (((MainCtrlPos == 1) || ((TrainType == dt_EZT) && (MainCtrlPos > 0))) &&
                (!FuseFlag) && (Mains) && /*(BrakePress < 1.0) &&*/ (MainCtrlActualPos == 0) &&
                (ActiveDir != 0))
            { //^^ TODO: sprawdzic BUG, prawdopodobnie w CreateBrakeSys()
                DelayCtrlFlag = true;
                if (LastRelayTime >= InitialCtrlDelay)
                {
                    StLinFlag = true; // ybARC - zalaczenie stycznikow liniowych
                    MainCtrlActualPos = 1;
                    DelayCtrlFlag = false;
                    SetFlag(SoundFlag, sound_relay);
                    SetFlag(SoundFlag, sound_loud);
                    OK = true;
                }
            }
            else
                DelayCtrlFlag = false;

            if ((!StLinFlag) && ((MainCtrlActualPos > 0) || (ScndCtrlActualPos > 0)))
                if ((TrainType == dt_EZT) && (CoupledCtrl)) // EN57 wal jednokierunkowy calosciowy
                {
                    if (MainCtrlActualPos == 1)
                    {
                        MainCtrlActualPos = 0;
                        OK = true;
                    }
                    else if (LastRelayTime > CtrlDownDelay)
                    {
                        if (MainCtrlActualPos < RlistSize)
                            MainCtrlActualPos++; // dojdz do konca
                        else if (ScndCtrlActualPos < ScndCtrlPosNo)
                            ScndCtrlActualPos++; // potem boki
                        else
                        { // i sie przewroc na koniec
                            MainCtrlActualPos = 0;
                            ScndCtrlActualPos = 0;
                        }
                        OK = true;
                    }
                }
                else if (CoupledCtrl) // wal kulakowy dwukierunkowy
                {
                    if (LastRelayTime > CtrlDownDelay)
                    {
                        if (ScndCtrlActualPos > 0)
                            ScndCtrlActualPos--;
                        else
                            MainCtrlActualPos--;
                        OK = true;
                    }
                }
                else
                {
                    MainCtrlActualPos = 0;
                    ScndCtrlActualPos = 0;
                    OK = true;
                }
        }
        if (OK)
            LastRelayTime = 0;

        return OK;
    }
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
bool TMoverParameters::PantFront(bool State)
{
    double pf1;
    bool PF;

    if ((Battery ==
         true) /* and ((TrainType<>dt_ET40)or ((TrainType=dt_ET40) and (EnginePowerSource.CollectorsNo>1)))*/)
    {
        PF = true;
        if (State == true)
            pf1 = 1;
        else
            pf1 = 0;
        if (PantFrontUp != State)
        {
            PantFrontUp = State;
            if (State == true)
            {
                PantFrontStart = 0;
                SendCtrlToNext("PantFront", 1, CabNo);
            }
            else
            {
                PF = false;
                PantFrontStart = 1;
                SendCtrlToNext("PantFront", 0, CabNo);
                //{Ra: nie ma potrzeby opuszczaæ obydwu na raz, jak mozemy ka¿dy osobno
                //      if (TrainType == dt_EZT) && (ActiveCab == 1)
                //       {
                //        PantRearUp = false;
                //        PantRearStart = 1;
                //        SendCtrlToNext("PantRear", 0, CabNo);
                //       }
                //}
            }
        }
        else
            SendCtrlToNext("PantFront", pf1, CabNo);
    }
    return PF;
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
bool TMoverParameters::PantRear(bool State)
{
    double pf1;
    bool PR;

    if (Battery == true)
    {
        PR = true;
        if (State == true)
            pf1 = 1;
        else
            pf1 = 0;
        if (PantRearUp != State)
        {
            PantRearUp = State;
            if (State == true)
            {
                PantRearStart = 0;
                SendCtrlToNext("PantRear", 1, CabNo);
            }
            else
            {
                PR = false;
                PantRearStart = 1;
                SendCtrlToNext("PantRear", 0, CabNo);
            }
        }
        else
            SendCtrlToNext("PantRear", pf1, CabNo);
    }
    return PR;
}

// *************************************************************************************************
// Q: 20160715
// *************************************************************************************************
bool TMoverParameters::dizel_EngageSwitch(double state)
{
    if ((EngineType == DieselEngine) && (state <= 1) && (state >= 0) &&
        (state != dizel_engagestate))
    {
        dizel_engagestate = state;
        return true;
    }
    else
        return false;
}

// *************************************************************************************************
// Q: 20160715
// *************************************************************************************************
bool TMoverParameters::dizel_EngageChange(double dt)
{
    // zmienia parametr do ktorego dazy sprzeglo
    const engagedownspeed = 0.9;
    const engageupspeed = 0.5;
    double engagespeed; // OK:boolean;
    bool DEC;

    DEC = false;
    if (dizel_engage - dizel_engagestate > 0)
        engagespeed = engagedownspeed;
    else
        engagespeed = engageupspeed;
    if (dt > 0.2)
        dt = 0.1;
    if (abs(dizel_engage - dizel_engagestate) < 0.11)
    {
        if (dizel_engage != dizel_engagestate)
        {
            DEC = true;
            dizel_engage = dizel_engagestate;
        }
        // else OK:=false; //ju¿ jest false
    }
    else
    {
        dizel_engage = dizel_engage + engagespeed * dt * (dizel_engagestate - dizel_engage);
        // OK:=false;
    }
    // dizel_EngageChange:=OK;
    return DEC;
}

// *************************************************************************************************
// Q: 20160715
// *************************************************************************************************
bool TMoverParameters::dizel_AutoGearCheck(void)
{
    bool OK;

    OK = false;
    if (MotorParam[ScndCtrlActualPos].AutoSwitch && Mains)
    {
        if (RList[MainCtrlPos].Mn == 0)
        {
            if (dizel_engagestate > 0)
                dizel_EngageSwitch(0);
            if ((MainCtrlPos == 0) && (ScndCtrlActualPos > 0))
                dizel_automaticgearstatus = -1;
        }
        else
        {
            if (MotorParam[ScndCtrlActualPos].AutoSwitch &&
                (dizel_automaticgearstatus == 0)) // sprawdz czy zmienic biegi
            {
                if ((Vel > MotorParam[ScndCtrlActualPos].mfi) &&
                    (ScndCtrlActualPos < ScndCtrlPosNo))
                {
                    dizel_automaticgearstatus = 1;
                    OK = true;
                }
                else if ((Vel < MotorParam[ScndCtrlActualPos].fi) && (ScndCtrlActualPos > 0))
                {
                    dizel_automaticgearstatus = -1;
                    OK = true;
                }
            }
        }
        if ((dizel_engage < 0.1) && (dizel_automaticgearstatus != 0))
        {
            if (dizel_automaticgearstatus == 1)
                ScndCtrlActualPos++;
            else
                ScndCtrlActualPos--;
            dizel_automaticgearstatus = 0;
            dizel_EngageSwitch(1.0);
            OK = true;
        }
    }

    if (Mains)
    {
        if (dizel_automaticgearstatus == 0) // ustaw cisnienie w silowniku sprzegla}
            switch (RList[MainCtrlPos].Mn)
            {
            case 1:
                dizel_EngageSwitch(0.5);
            case 2:
                dizel_EngageSwitch(1.0);
            default:
                dizel_EngageSwitch(0.0);
            }
        else
            dizel_EngageSwitch(0.0);
        if (!(MotorParam[ScndCtrlActualPos].mIsat > 0))
            dizel_EngageSwitch(0.0); // wylacz sprzeglo na pozycjach neutralnych
        if (!AutoRelayFlag)
            ScndCtrlActualPos = ScndCtrlPos;
    }
    return OK;
}

// *************************************************************************************************
// Q: 20160715
// *************************************************************************************************
bool TMoverParameters::dizel_Update(double dt)
{ // odœwie¿a informacje o silniku
    const fillspeed = 2;
    bool DU;

    // dizel_Update:=false;
    if (dizel_enginestart && (LastSwitchingTime >= InitialCtrlDelay))
    {
        dizel_enginestart = false;
        LastSwitchingTime = 0;
        enrot = dizel_nmin / 2.0; // TODO: dac zaleznie od temperatury i baterii
    }
    /*OK=*/dizel_EngageChange(dt);
    //  if AutoRelayFlag then Poprawka na SM03
    DU = dizel_AutoGearCheck();
    // dizel_fill:=(dizel_fill+dizel_fillcheck(MainCtrlPos))/2;
    dizel_fill = dizel_fill + fillspeed * dt * (dizel_fillcheck(MainCtrlPos) - dizel_fill);
    // dizel_Update:=OK;

    return DU;
}

// *************************************************************************************************
// Q: 20160715
// *************************************************************************************************
double TMoverParameters::dizel_fillcheck(int mcp)
{ // oblicza napelnienie, uzwglednia regulator obrotow
    double realfill, nreg;

    realfill = 0;
    nreg = 0;
    if (Mains && (MainCtrlPosNo > 0))
    {
        if (dizel_enginestart &&
            (LastSwitchingTime >= 0.9 * InitialCtrlDelay)) // wzbogacenie przy rozruchu
            realfill = 1;
        else
            realfill = RList[mcp].R; // napelnienie zalezne od MainCtrlPos
        if (dizel_nmax_cutoff > 0)
        {
            switch (RList[MainCtrlPos].Mn)
            {
            case 0:
            case 1:
                nreg = dizel_nmin;
            case 2:
                if (dizel_automaticgearstatus == 0)
                    nreg = dizel_nmax;
                else
                    nreg = dizel_nmin;
            default:
                realfill = 0; // sluczaj
            }
            if (enrot > nreg)
                realfill = realfill * (3.9 - 3.0 * abs(enrot) / nreg);
            if (enrot > dizel_nmax_cutoff)
                realfill = realfill * (9.8 - 9.0 * abs(enrot) / dizel_nmax_cutoff);
            if (enrot < dizel_nmin)
                realfill = realfill * (1 + (dizel_nmin - abs(enrot)) / dizel_nmin);
        }
    }
    if (realfill < 0)
        realfill = 0;
    if (realfill > 1)
        realfill = 1;
    return realfill;
}

// *************************************************************************************************
// Q: 20160715
// *************************************************************************************************
double TMoverParameters::dizel_Momentum(double dizel_fill, double n, double dt)
{ // liczy moment sily wytwarzany przez silnik spalinowy}
    double Moment, enMoment, eps, newn, friction;
    double DM;

    // friction =dizel_engagefriction*(11-2*random)/10;
    friction = dizel_engagefriction;
    if (enrot > 0)
    { // sqr TODO: sqr c++
        Moment = dizel_Mmax * dizel_fill -
                 (dizel_Mmax - dizel_Mnmax * dizel_fill) *
                     sqr(enrot / (dizel_nmax - dizel_nMmax * dizel_fill)) -
                 dizel_Mstand; // Q: zamieniam SQR() na sqr()
        //    Moment:=Moment*(1+sin(eAngle*4))-dizel_Mstand*(1+cos(eAngle*4));}
    }
    else
        Moment = -dizel_Mstand;
    if (enrot < dizel_nmin / 10.0)
        if (eAngle < PI / 2.0)
            Moment = Moment - dizel_Mstand; // wstrzymywanie przy malych obrotach
    //!! abs
    if (abs(abs(n) - enrot) < 0.1)
    {
        if ((Moment) > (dizel_engageMaxForce * dizel_engage * dizel_engageDia * friction *
                        2)) // zerwanie przyczepnosci sprzegla
            enrot = enrot + dt * Moment / dizel_AIM;
        else
        {
            dizel_engagedeltaomega = 0;
            enrot = abs(n); // jest przyczepnosc tarcz
        }
    }
    else
    {
        if ((enrot == 0) && (Moment < 0))
            newn = 0;
        else
        {
            //!! abs
            dizel_engagedeltaomega = enrot - n; // sliganie tarcz
            enMoment = Moment -
                       Sign(dizel_engagedeltaomega) * dizel_engageMaxForce * dizel_engage *
                           dizel_engageDia * friction;
            Moment = Sign(dizel_engagedeltaomega) * dizel_engageMaxForce * dizel_engage *
                     dizel_engageDia * friction;
            dizel_engagedeltaomega = abs(enrot - abs(n));
            eps = enMoment / dizel_AIM;
            newn = enrot + eps * dt;
            if ((newn * enrot <= 0) && (eps * enrot < 0))
                newn = 0;
        }
        enrot = newn;
    }
    DM = Moment;
    if ((enrot == 0) && (!dizel_enginestart))
        Mains = false;

    return DM;
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
bool TMoverParameters::LoadingDone(double LSpeed, std::string LoadInit)
{
    // test zakoñczenia za³adunku/roz³adunku
    long LoadChange;
    bool LD;

    ClearPendingExceptions; // zabezpieczenie dla Trunc()
    // LoadingDone:=false; //nie zakoñczone
    if (LoadInit != "") // nazwa ³adunku niepusta
    {
        if (Load > MaxLoad)
            LoadChange = abs(int(LSpeed * LastLoadChangeTime /
                                 2.0)); // prze³adowanie?      // trunc zamieniam na int()
        else
            LoadChange = abs(int(LSpeed * LastLoadChangeTime));
        if (LSpeed < 0) // gdy roz³adunek
        {
            LoadStatus = 2; // trwa roz³adunek (w³¹czenie naliczania czasu)
            if (LoadChange != 0) // jeœli coœ prze³adowano
            {
                LastLoadChangeTime = 0; // naliczony czas zosta³ zu¿yty
                Load = Load - LoadChange; // zmniejszenie iloœci ³adunku
                CommandIn.Value1 =
                    CommandIn.Value1 - LoadChange; // zmniejszenie iloœci do roz³adowania
                if (Load < 0)
                    Load = 0; //³adunek nie mo¿e byæ ujemny
                if ((Load == 0) || (CommandIn.Value1 < 0)) // pusto lub roz³adowano ¿¹dan¹ iloœæ
                    LoadStatus = 4; // skoñczony roz³adunek
                if (Load == 0)
                    LoadType = ""; // jak nic nie ma, to nie ma te¿ nazwy
            }
        }
        else if (LSpeed > 0) // gdy za³adunek
        {
            LoadStatus = 1; // trwa za³adunek (w³¹czenie naliczania czasu)
            if (LoadChange != 0) // jeœli coœ prze³adowano
            {
                LastLoadChangeTime = 0; // naliczony czas zosta³ zu¿yty
                LoadType = LoadInit; // nazwa
                Load = Load + LoadChange; // zwiêkszenie ³adunku
                CommandIn.Value1 = CommandIn.Value1 - LoadChange;
                if ((Load >= MaxLoad * (1 + OverLoadFactor)) || (CommandIn.Value1 < 0))
                    LoadStatus = 4; // skoñczony za³adunek
            }
        }
        else
            LoadStatus = 4; // zerowa prêdkoœæ zmiany, to koniec
    }
    LD = (LoadStatus >= 4);
    return LD;
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
bool TMoverParameters::DoorBlockedFlag(void)
{
    // if (DoorBlocked=true) and (Vel<5.0) then
    bool DBF = false;
    if ((DoorBlocked == true) && (Vel >= 5.0))
        DBF = true;

    return DBF;
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
bool TMoverParameters::DoorLeft(bool State)
{
    bool DL;
    if ((DoorLeftOpened != State) && (DoorBlockedFlag() == false) && (Battery == true))
    {
        DL = true;
        DoorLeftOpened = State;
        if (State == true)
        {
            if (CabNo > 0)
                SendCtrlToNext("DoorOpen", 1, CabNo); // 1=lewe, 2=prawe
            else
                SendCtrlToNext("DoorOpen", 2, CabNo); // zamiana
            CompressedVolume = CompressedVolume - 0.003;
        }
        else
        {
            if (CabNo > 0)
                SendCtrlToNext("DoorClose", 1, CabNo);
            else
                SendCtrlToNext("DoorClose", 2, CabNo);
        }
    }
    else
        DL = false;
    return DL;
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
bool TMoverParameters::DoorRight(bool State)
{
    bool DR;
    if ((DoorRightOpened != State) && (DoorBlockedFlag() == false) && (Battery == true))
    {
        DR = true;
        DoorRightOpened = State;
        if (State == true)
        {
            if (CabNo > 0)
                SendCtrlToNext("DoorOpen", 2, CabNo); // 1=lewe, 2=prawe
            else
                SendCtrlToNext("DoorOpen", 1, CabNo); // zamiana
            CompressedVolume = CompressedVolume - 0.003;
        }
        else
        {
            if (CabNo > 0)
                SendCtrlToNext("DoorClose", 2, CabNo);
            else
                SendCtrlToNext("DoorClose", 1, CabNo);
        }
    }
    else
        DR = false;

    return DR;
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
bool TMoverParameters::ChangeOffsetH(double DeltaOffset)
{
    bool COH;
    if (TestFlag(CategoryFlag, 2) && TestFlag(RunningTrack.CategoryFlag, 2))
    {
        OffsetTrackH = OffsetTrackH + DeltaOffset;
        //     if (abs(OffsetTrackH) > (RunningTrack.Width / 1.95 - TrackW / 2.0))
        if (abs(OffsetTrackH) >
            (0.5 * (RunningTrack.Width - Dim.W) - 0.05)) // Ra: mo¿e pó³ pojazdu od brzegu?
            COH = false; // kola na granicy drogi
        else
            COH = true;
    }
    else
        COH = false;

    return COH;
}

// *************************************************************************************************
// Q: 20160713
// *************************************************************************************************
std::string TMoverParameters::EngineDescription(int what)
{
    std::string outstr;

    outstr = "";
    switch (what)
    {
    case 0:
    {
        if (DamageFlag == 255)
            outstr = "Totally destroyed!";
        else
        {
            if (TestFlag(DamageFlag, dtrain_thinwheel))
                if (Power > 0.1)
                    outstr = "Thin wheel,";
                else
                    outstr = "Load shifted,";
            if (TestFlag(DamageFlag, dtrain_wheelwear))
                outstr = "Wheel wear,";
            if (TestFlag(DamageFlag, dtrain_bearing))
                outstr = "Bearing damaged,";
            if (TestFlag(DamageFlag, dtrain_coupling))
                outstr = "Coupler broken,";
            if (TestFlag(DamageFlag, dtrain_loaddamage))
                if (Power > 0.1)
                    outstr = "Ventilator damaged,";
                else
                    outstr = "Load damaged,";

            if (TestFlag(DamageFlag, dtrain_loaddestroyed))
                if (Power > 0.1)
                    outstr = "Engine damaged,";
                else
                    outstr = "Load destroyed!,";
            if (TestFlag(DamageFlag, dtrain_axle))
                outstr = "Axle broken,";
            if (TestFlag(DamageFlag, dtrain_out))
                outstr = "DERAILED!";
            if (outstr == "")
                outstr = "OK!";
        }
    };
    default:
        outstr = "Invalid qualifier";
    }
    return outstr;
}

// *************************************************************************************************
// Q: 20160709
// *************************************************************************************************
double TMoverParameters::GetTrainsetVoltage(void)
{
    double volt;

    volt = 0.0;
    if (Couplers[1].Connected != NULL)
        if (TestFlag(Couplers[1].CouplingFlag, ctrain_power)) // czy jest sprzêg WN
        { // najczêœciej silnikowy jest z ty³u
            if (Couplers[1].Connected->PantFrontVolt != 0.0)
                volt = Couplers[1].Connected->PantFrontVolt;
            else if (Couplers[1].Connected->PantRearVolt != 0.0)
                volt = Couplers[1].Connected->PantRearVolt;
        }
    if (volt == 0.0)
        if (Couplers[0].Connected != NULL)
            if (TestFlag(Couplers[0].CouplingFlag, ctrain_power)) // czy jest sprzêg WN
            {
                if (Couplers[0].Connected->PantFrontVolt != 0.0)
                    volt = Couplers[0].Connected->PantFrontVolt;
                else if (Couplers[0].Connected->PantRearVolt != 0.0)
                    volt = Couplers[0].Connected->PantRearVolt;
            }
    // WriteLog("GTV");
    return volt;
}

// *************************************************************************************************
//
// *************************************************************************************************
bool TMoverParameters::Physic_ReActivation(void) // DO PRZETLUMACZENIA NA KONCU
{
    bool pr;
    if (PhysicActivation)
        return false;
    else
    {
        PhysicActivation = true;
        LastSwitchingTime = 0;
        return true;
    }
}

// *************************************************************************************************
// FUNKCJE PARSERA WCZYTYWANIA PLIKU FIZYKI POJAZDU
// *************************************************************************************************
std::string p0, p1, p2, p3, p4, p5, p6, p7;
std::string xline, sectionname, lastsectionname;
std::string vS;
int vI;
double vD;
bool startBPT;
bool startMPT;
bool startRLIST;
int MPTLINE, RLISTLINE, BPTLINE;
std::vector<std::string> x;

std::string aCategory, aType;
double aMass, aMred, aVmax, aPWR, aHeatingP, aLightP;
int aSandCap;

std::string bLoadQ, bLoadAccepted;
double bLoadSpeed, bUnLoadSpeed, bOverLoadFactor, cL, cW, cH, cCx, cFloor;
int bMaxLoad;

std::string dAxle, dBearingType;
double dD, dDl, dDt, dAIM, dTw, dAd, dBd, dRmin;

std::string eBrakeValve, eBM, eCompressorPower;
double eMBF, eTBF, eMaxBP, eMedMaxBP, eTareMaxBP, eMaxLBP, eMaxASBP, eRM, eBCR, eBCD, eBCM, eBCMlo,
    eBCMhi, eVv, eMinCP, eMaxCP, eBCS, eBSA, eBRE, eHiPP, eLoPP, eCompressorSpeed;
int eNBpA, eBVV, eBCN, eSize;

std::string fCType;
double fkB, fDmaxB, fFmaxB, fkC, fDmaxC, fFmaxC, fbeta;
int fAllowedFlag;

std::string gBrakeSystem, gASB, gLocalBrake, gDynamicBrake, gManualBrake, gScndS, gFSCircuit,
    gAutoRelay, gBrakeDelays, gBrakeHandle, gLocBrakeHandle, gCoupledCtrl;
float gIniCDelay, gSCDelay, gSCDDelay, gMaxBPMass;
int gBCPN, gBDelay1, gBDelay2, gBDelay3, gBDelay4, gMCPN, gSCPN, gSCIM;

std::string hAwareSystem, hRadioStop;
double hAwareMinSpeed, hAwareDelay, hSoundSignalDelay, hEmergencyBrakeDelay;

std::string iLight, iLGeneratorEngine;
double iLMaxVoltage, iLMaxCurrent;

std::string jEnginePower, jSystemPower;
double jMaxVoltage, jMaxCurrent, jIntR, jMinH, jMaxH, jCSW, jMinV, jMaxV, jMinPress, jMaxPress;
int jCollectorsNo;

std::string kEngineType, kTrans;
double kWindingRes, knmax, kVolt;
// int kVolt;

double lCircuitRes;
int lImaxLo, lImaxHi, lIminLo, lIminHi;

std::string mRVent;
double mRVentnmax, mRVentCutOff;
int mSize;

double nMmax, nnMmax, nMnmax, nnmax, nnominalfill, nMstand;
int nSize;

/*inline int ti(std::string val)
{
    return atoi(val.c_str());
}

inline double td(std::string val)
{
    return atof(val.c_str());
}

std::string ts(std::string val)
{
    // WriteLog("["+ val + "]");

    return val;
    //   else return "unknown";
}

std::string tS(std::string val)
{
    return ToUpper(val);
}*/

// *************************************************************************************************
// Q: 20160717
// *************************************************************************************************
int Pos(std::string str_find, std::string in)
{
    size_t pos = in.find(str_find);
    return (pos != string::npos ? pos+1 : 0);
}

// *************************************************************************************************
// Q: 20160717
// *************************************************************************************************
bool issection(std::string name)
{
    sectionname = name;
    if (xline.find(name) != string::npos)
    {
        lastsectionname = name;
        return true;
    }
    else
        return false;
}

// *************************************************************************************************
// Q: 20160717
// *************************************************************************************************
// Pobieranie wartosci z klucza i przypisanie jej do wlasciwego typu danych 1 - string, 2 - int, 3 -
// double
std::string getkeyval(int rettype, std::string key)
{
    std::string keyname = key;
    key = key + "=";
    std::string kval, temp;
    temp = xline;
    int to;

    if (Pos(key, xline) > 0) // jezeli jest klucz w swkcji...
    {
        int klen = key.length();
        int kpos = Pos(key, xline) - 1;
        temp.erase(0, kpos + klen);
        if (temp.find(" ") != string::npos)
            to = temp.find(" ");
        else
            to = 255;
        kval = temp.substr(0, to);
        if (kval != "")
            kval = TrimSpace(kval); // wyciagnieta wartosc

        sectionname = ExchangeCharInString(sectionname, (char)":", (char)"");
        sectionname = ExchangeCharInString(sectionname, (char)".", (char)"");
        //--WriteLog(sectionname + "." + keyname + " val= [" + kval + "]");

        //    if (rettype == 1) vS = kval;
        //    if (kval != "" && rettype == 2) vI = StrToInt(kval);
        //    if (kval != "" && rettype == 3) vD = StrToFloat(kval);
    }
    else
        kval = ""; // gdy nie bylo klucza TODO: dodac do funkcji parametr z wartoscia fabryczna
    // UWAGA! 0 moze powodowac bledy, przemyslec zwracanie wartosci gdy nie ma klucza!!!
    // zwraca pusty klucz GF 2016-10-26
    return kval;
}

int MARKERROR(int code, std::string type, std::string msg)
{
    WriteLog(msg);
    return code;
}

int s2NPW(string s)
{ // wylicza ilosc osi napednych z opisu ukladu osi
	const char A = 64;
	int k;
	int NPW = 0;
	for (k = 0; k < s.length(); k++)
	{
		if (s[k] >= (char)65 && s[k] <= (char)90)
			NPW += s[k] - A;
	}
	return NPW;
}

int s2NNW(string s)
{ // wylicza ilosc osi nienapedzanych z opisu ukladu osi
	const char Zero = 48;
	int k;
	int NNW = 0;
	for (k = 0; k < s.length(); k++)
	{
		if (s[k] >= (char)49 && s[k] <= (char)57)
			NNW += s[k] - Zero;
	}
	return NNW;
}

// *************************************************************************************************
// Q: 20160717
// *************************************************************************************************
// parsowanie Motor Param Table
bool TMoverParameters::readMPT(int ln, std::string line)
{
    // bl, mfi, mIsat, mfi0, fi, Isat, fi0
    bool as;
    int bl;
    startMPT = true;

    if (ln > 0) // 0 to nazwa sekcji - MotorParamTable0:
    {
        //--WriteLog("MPT: " + xline);
        x = Split(line);

		if (x.size() != 7)
		{
			WriteLog("Read MPT: wrong argument number of arguments in line " + to_string(ln -1));
			return false;
		}


        p0 = TrimSpace(x[0]);
        p1 = TrimSpace(x[1]);
        p2 = TrimSpace(x[2]);
        p3 = TrimSpace(x[3]);
        p4 = TrimSpace(x[4]);
        p5 = TrimSpace(x[5]);
        p6 = TrimSpace(x[6]);

        if (AutoRelayType == 0)
            as = false;
        bl = atoi(p0.c_str()); // numer pozycji

        MotorParam[bl].mfi = atof(p1.c_str());
        MotorParam[bl].mIsat = atof(p2.c_str());
        MotorParam[bl].mfi0 = atof(p3.c_str());
        MotorParam[bl].fi = atof(p4.c_str());
        MotorParam[bl].Isat = atof(p5.c_str());
        MotorParam[bl].fi0 = atof(p6.c_str());
        MotorParam[bl].AutoSwitch = as;

        //--WriteLog(":::: " + p0 + "," + p1 + "," + p2 + "," + p3 + "," + p4 + "," + p5 + "," +
        //p6);
    }
    MPTLINE++;
    return true;
}

// *************************************************************************************************
// Q: 20160718
// *************************************************************************************************
// parsowanie RList
bool TMoverParameters::readRLIST(int ln, std::string line)
{
    char *xxx;
    startRLIST = true;

    if (ln > 0) // 0 to nazwa sekcji - RList:
    {
        // WriteLog("RLIST: " + xline);
        line = Tab2Sp(line); // zamieniamy taby na spacje  (ile tabow tyle spacji bedzie)

        xxx = TrimAndReduceSpaces(line.c_str()); // konwertujemy na *char i
                                                                   // ograniczamy spacje pomiedzy
                                                                   // parametrami do jednej

        x = Split(xxx); // split je wskaznik na char jak i std::string

        if (x.size() != 5)
        {
			WriteLog("Read RLIST: wrong argument number of arguments in line " + to_string(ln - 1));
            delete[] xxx;
            return false;
        }

        p0 = TrimSpace(x[0]);
        p1 = TrimSpace(x[1]);
        p2 = TrimSpace(x[2]);
        p3 = TrimSpace(x[3]);
        p4 = TrimSpace(x[4]);

        int k = ln - 1;

        RlistSize = (mSize);
        if (RlistSize > ResArraySize)
            ConversionError = -4;

        RList[k].Relay = atoi(p0.c_str()); // int
        RList[k].R = atof(p1.c_str()); // double
        RList[k].Bn = atoi(p2.c_str()); // int
        RList[k].Mn = atoi(p3.c_str()); // int
        RList[k].AutoSwitch = false; // p4.ToInt();

        //--WriteLog("RLIST: " + p0 + "," + p1 + "," + p2 + "," + p3 + "," + p4);
    }
	delete[] xxx;
    RLISTLINE++;
    return true;
}

// *************************************************************************************************
// Q: 20160721
// *************************************************************************************************
// parsowanie Brake Param Table
bool TMoverParameters::readBPT(int ln, std::string line)
{
    char *xxx;
    startBPT = true;
    int k;

    if (ln > 0) // 0 to nazwa sekcji - Cntrl. - po niej jest tablica hamulcow
    {
        // WriteLog("BPT: " + xline);
        line = Tab2Sp(line);
        xxx = TrimAndReduceSpaces(line.c_str());
        x = Split(xxx);

		if (x.size() != 5)
		{
			WriteLog("Read BPT: wrong argument number of arguments in line " + to_string(ln - 1));
			delete[] xxx;
			return false;
		}

        p0 = TrimSpace(x[0]);
        p1 = TrimSpace(x[1]);
        p2 = TrimSpace(x[2]);
        p3 = TrimSpace(x[3]);
        p4 = TrimSpace(x[4]);

        k = atoi(p0.c_str());
        BrakePressureTable[k].PipePressureVal = atof(p1.c_str());
        BrakePressureTable[k].BrakePressureVal = atof(p2.c_str());
        BrakePressureTable[k].FlowSpeedVal = atof(p3.c_str());
        if (p4 == "Pneumatic")
            BrakePressureTable[k].BrakeType = Pneumatic;
        else if (p4 == "ElectroPneumatic")
            BrakePressureTable[k].BrakeType = ElectroPneumatic;
        else
            BrakePressureTable[k].BrakeType = Individual;

        //-- WriteLog("BPT: " + p0 + "," + p1 + "," + p2 + "," + p3 + "," + p4);
    }
    delete[] xxx;
    BPTLINE++;
    return true;
}

// *************************************************************************************************
// Q: 20160719
// *************************************************************************************************
void TMoverParameters::BrakeValveDecode(std::string s)
{
    if (s == "W")
        BrakeValve = W;
    else if (s == "W_Lu_L")
        BrakeValve = W_Lu_L;
    else if (s == "W_Lu_XR")
        BrakeValve = W_Lu_XR;
    else if (s == "W_Lu_VI")
        BrakeValve = W_Lu_VI;
    else if (s == "K")
        BrakeValve = W;
    else if (s == "Kkg")
        BrakeValve = Kkg;
    else if (s == "Kkp")
        BrakeValve = Kkp;
    else if (s == "Kks")
        BrakeValve = Kks;
    else if (s == "Hikp1")
        BrakeValve = Hikp1;
    else if (s == "Hikss")
        BrakeValve = Hikss;
    else if (s == "Hikg1")
        BrakeValve = Hikg1;
    else if (s == "KE")
        BrakeValve = KE;
    else if (s == "EStED")
        BrakeValve = EStED;
    else if (Pos("ESt", s) > 0)
        BrakeValve = ESt3;
    else if (s == "LSt")
        BrakeValve = LSt;
    else if (s == "EP2")
        BrakeValve = EP2;
    else if (s == "EP1")
        BrakeValve = EP1;
    else if (s == "CV1")
        BrakeValve = CV1;
    else if (s == "CV1_L_TR")
        BrakeValve = CV1_L_TR;
    else
        BrakeValve = Other;
}

// *************************************************************************************************
// Q: 20160719
// *************************************************************************************************
void TMoverParameters::BrakeSubsystemDecode()
{
    BrakeSubsystem = ss_None;
    switch (BrakeValve)
    {
	case W:
	case W_Lu_L:
	case W_Lu_VI:
	case W_Lu_XR:
        BrakeSubsystem = ss_W;
        break;
	case ESt3:
	case ESt3AL2:
	case ESt4:
	case EP2:
	case EP1:
        BrakeSubsystem = ss_ESt;
        break;
    case KE:
        BrakeSubsystem = ss_KE;
        break;
	case CV1:
	case CV1_L_TR:
        BrakeSubsystem = ss_Dako;
        break;
	case LSt:
	case EStED:
        BrakeSubsystem = ss_LSt;
        break;
    }
}

// *************************************************************************************************
// Q: 20160721
// *************************************************************************************************
TEngineTypes TMoverParameters::EngineDecode(std::string s)
{
    if (s == "ElectricSeriesMotor")
        return ElectricSeriesMotor;
    else if (s == "DieselEngine")
        return DieselEngine;
    else if (s == "SteamEngine")
        return SteamEngine;
    else if (s == "WheelsDriven")
        return WheelsDriven;
    else if (s == "Dumb")
        return Dumb;
    else if (s == "DieselElectric")
        return DieselElectric;
    else // youBy: spal-ele
        if (s == "DumbDE")
        return DieselElectric;
    else // youBy: spal-ele
        if (s == "ElectricInductionMotor")
        return ElectricInductionMotor;
    //   else if s='EZT' then {dla kibla}
    //   EngineDecode:=EZT      }
    else
        return None;
}

// *************************************************************************************************
// Q: 20160719
// *************************************************************************************************
TPowerSource TMoverParameters::PowerSourceDecode(std::string s)
{

    if (s == "Transducer")
        return Transducer;
    else if (s == "Generator")
        return Generator;
    else if (s == "Accu")
        return Accumulator;
    else if (s == "CurrentCollector")
        return CurrentCollector;
    else if (s == "PowerCable")
        return PowerCable;
    else if (s == "Heater")
        return Heater;
    else if (s == "Internal")
        return InternalSource;
    else
        return NotDefined;
}

// *************************************************************************************************
// Q: 20160719
// *************************************************************************************************
TPowerType TMoverParameters::PowerDecode(std::string s)
{
    if (s == "BioPower")
        return BioPower;
    else if (s == "BioPower")
        return BioPower;
    else if (s == "MechPower")
        return MechPower;
    else if (s == "ElectricPower")
        return ElectricPower;
    else if (s == "SteamPower")
        return SteamPower;
    else
        return NoPower;
}

// *************************************************************************************************
// Q: 20160719
// *************************************************************************************************
void TMoverParameters::PowerParamDecode(std::string lines, std::string prefix,
                                        TPowerParameters &PowerParamDecode)
{
    // with PowerParamDecode do
    //   begin
    switch (PowerParamDecode.SourceType)
    {
    //--case NotDefined : PowerType = PowerDecode(DUE(ExtractKeyWord(lines,prefix+'PowerType=')));
    //--case InternalSource : PowerType =
    //PowerDecode(DUE(ExtractKeyWord(lines,prefix+'PowerType=')));
    //--case Transducer : InputVoltage =
    //s2rE(DUE(ExtractKeyWord(lines,prefix+'TransducerInputV=')));
    //--case Generator  :
    //GeneratorEngine:=EngineDecode(DUE(ExtractKeyWord(lines,prefix+'GeneratorEngine=')));
    //--case Accumulator:
    //--{
    //--               RAccumulator.MaxCapacity:=s2r(DUE(ExtractKeyWord(lines,prefix+'Cap=')));
    //--               s:=DUE(ExtractKeyWord(lines,prefix+'RS='));
    //--               RAccumulator.RechargeSource:=PowerSourceDecode(s);
    //--}

    case CurrentCollector:
    {

        PowerParamDecode.CollectorParameters.CollectorsNo = (jCollectorsNo);
        PowerParamDecode.CollectorParameters.MinH = (jMinH);
        PowerParamDecode.CollectorParameters.MaxH = (jMaxH);
        PowerParamDecode.CollectorParameters.CSW = (jCSW); // szerokoœæ czêœci roboczej
        PowerParamDecode.CollectorParameters.MaxV = (jMaxVoltage);

        // s:=jMinV; //napiêcie roz³¹czaj¹ce WS
        if (jMinV == 0)
            PowerParamDecode.CollectorParameters.MinV =
                0.5 * PowerParamDecode.CollectorParameters.MaxV; // gdyby parametr nie podany
        else
            PowerParamDecode.CollectorParameters.MinV = (jMinV);

        //-s:=ExtractKeyWord(lines,'InsetV='); //napiêcie wymagane do za³¹czenia WS
        //-if s='' then
        //- InsetV:=0.6*MaxV //gdyby parametr nie podany
        //-else
        //- InsetV:=s2rE(DUE(s));
        // s:=ExtractKeyWord(lines,'MinPress='); //ciœnienie roz³¹czaj¹ce WS
        if (jMinPress == 0)
            PowerParamDecode.CollectorParameters.MinPress = 2.0; // domyœlnie 2 bary do za³¹czenia
                                                                 // WS
        else
            PowerParamDecode.CollectorParameters.MinPress = (jMinPress);
        // s:=ExtractKeyWord(lines,'MaxPress='); //maksymalne ciœnienie za reduktorem
        if (jMaxPress == 0)
            PowerParamDecode.CollectorParameters.MaxPress = 5.0 + 0.001 * (Random(50) - Random(50));
        else
            PowerParamDecode.CollectorParameters.MaxPress = (jMaxPress);
    }
        // case PowerCable:
        //{
        //               RPowerCable.PowerTrans:=PowerDecode(DUE(ExtractKeyWord(lines,prefix+'PowerTrans=')));
        //               if RPowerCable.PowerTrans=SteamPower then
        //                RPowerCable.SteamPressure:=s2r(DUE(ExtractKeyWord(lines,prefix+'SteamPress=')));
        //}
        // case Heater :
        //{
        //            //jeszcze nie skonczone!
        //}
    }

    if ((PowerParamDecode.SourceType != Heater) && (PowerParamDecode.SourceType != InternalSource))
        if (!((PowerParamDecode.SourceType == PowerCable) &&
              (PowerParamDecode.RPowerCable.PowerTrans == SteamPower)))
        {
            //--MaxVoltage =s2rE(DUE(ExtractKeyWord(lines,prefix+'MaxVoltage=')));
            //--MaxCurrent =s2r(DUE(ExtractKeyWord(lines,prefix+'MaxCurrent=')));
            //--IntR =s2r(DUE(ExtractKeyWord(lines,prefix+'IntR=')));
        }
}

// *************************************************************************************************
// Q: 20160717
// Funkcja pelniaca role pierwotnej LoadChkFile wywolywana w dynobj.cpp w double
// TDynamicObject::Init()
// Po niej wykonywana jest CreateBrakeSys(), ktora jest odpowiednikiem CheckLocomotiveParameters()
// *************************************************************************************************
bool TMoverParameters::LoadFIZ(std::string chkpath)
{
    const int param_ok = 1;
    const int wheels_ok = 2;
    const int dimensions_ok = 4;

    int ishash;
    int bl, i, k;
    int b, OKFlag;
    std::string lines, s, appdir;
    std::string APPDIR, filetocheck, line, node, key, file, CERR;
    string wers;
    bool noexist = false;
    bool OK;

    OKFlag = 0;
    LineCount = 0;
    ConversionError = 666;
    BPTLINE = 0;
    MPTLINE = 0;
    RLISTLINE = 0;
    Mass = 0;
    file = chkpath + TypeName + ".fiz";

    WriteLog("LOAD FIZ FROM " + file);

    // if (!FileExists(file)) WriteLog("E8 - FIZ FILE NOT EXIST.");
    // if (!FileExists(file)) return false;

    // appdir = ExtractFilePath(ParamStr(0));

    ifstream in(file.c_str());
	if (!in.is_open())
	{
		WriteLog("E8 - FIZ FILE NOT EXIST.");
		return false;
	}

    bool secBPT = false, secMotorParamTable0 = false, secPower = false, secEngine = false,
		secParam = false, secLoad = false, secDimensions = false,
        secWheels = false, secBrake = false, secBuffCoupl = false, secCntrl = false,
		secSecurity = false, secLight = false, secCircuit = false, secRList = false,
        secDList = false, secWWList = false, secffList = false, secTurboPos = false;

    ConversionError = 0;

    // Zbieranie danych zawartych w pliku FIZ
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    while (getline(in, wers))
    {
        // wers.find('#');
        xline = wers.c_str();
        ishash = xline.find("#");
        // if ((ishash == 1)) WriteLog("zakomentowane " + xline);
        if ((ishash == string::npos))
        {

            if (xline.length() == 0)
                startBPT = false; // Tablica parametyrow hamulca nie ma znacznika konca :(
            if (issection("END-MPT"))
                startMPT = false;
            if (issection("END-RL"))
                startRLIST = false;

            if (issection("Param."))
            {
                secParam = true;
                SetFlag(OKFlag, param_ok);
                aCategory = getkeyval(1, "Category");
                aType = ToUpper(getkeyval(1, "Type"));
                aMass = atof(getkeyval(3, "M").c_str());
                aMred = atof(getkeyval(3, "Mred").c_str());
                aVmax = atof(getkeyval(3, "Vmax").c_str());
                aPWR = atof(getkeyval(3, "PWR").c_str());
                aSandCap = atoi(getkeyval(2, "SandCap").c_str());
                aHeatingP = atof(getkeyval(3, "HeatingP").c_str());
                aLightP = atof(getkeyval(3, "LightP").c_str());
            }

            if (issection("Load:"))
            {
                secLoad = true;
                bMaxLoad = atoi(getkeyval(2, "MaxLoad").c_str());
                bLoadQ = getkeyval(1, "LoadQ");
                bLoadAccepted = getkeyval(1, "LoadAccepted");
                bLoadSpeed = atof(getkeyval(3, "LoadSpeed").c_str());
                bUnLoadSpeed = atof(getkeyval(3, "UnLoadSpeed").c_str());
                bOverLoadFactor = atof(getkeyval(3, "OverLoadFactor").c_str());
            }

            if (issection("Dimensions:"))
            {
                secDimensions = true;
                SetFlag(OKFlag, dimensions_ok);
                cL = atof(getkeyval(3, "L").c_str());
                cH = atof(getkeyval(3, "H").c_str());
                cW = atof(getkeyval(3, "W").c_str());
                cCx = atof(getkeyval(3, "Cx").c_str());
                cFloor = atof(getkeyval(3, "Floor").c_str());
            }

            if (issection("Wheels:"))
            {
                secWheels = true;
                dD = atof(getkeyval(3, "D").c_str());
                dDl = atof(getkeyval(3, "Dl").c_str());
                dDt = atof(getkeyval(3, "Dt").c_str());
                dAIM = atof(getkeyval(3, "AIM").c_str());
                dTw = atof(getkeyval(3, "Tw").c_str());
                dAxle = getkeyval(1, "Axle");
                dAd = atof(getkeyval(3, "Ad").c_str());
                dBd = atof(getkeyval(3, "Bd").c_str());
                dRmin = atof(getkeyval(3, "Rmin").c_str());
                dBearingType = getkeyval(1, "BearingType");
            }

            if (issection("Brake:"))
            {
                secBrake = true;
                eBrakeValve = getkeyval(1, "BrakeValve");
                eNBpA = atoi(getkeyval(2, "NBpA").c_str());
                eMBF = atof(getkeyval(3, "MBF").c_str());
                eTBF = atof(getkeyval(3, "TBF").c_str());
                eSize = atoi(getkeyval(3, "Size").c_str());
                eMaxBP = atof(getkeyval(3, "MaxBP").c_str());
                eMedMaxBP = atof(getkeyval(3, "MedMaxBP").c_str());
                eTareMaxBP = atof(getkeyval(3, "TareMaxBP").c_str());
                eMaxLBP = atof(getkeyval(3, "MaxLBP").c_str());
                eMaxASBP = atof(getkeyval(3, "MaxASBP").c_str());
                eRM = atof(getkeyval(3, "RM").c_str());
                eBCN = atoi(getkeyval(2, "BCN").c_str());
                eBCR = atof(getkeyval(3, "BCR").c_str());
                eBCD = atof(getkeyval(3, "BCD").c_str());
                eBCM = atof(getkeyval(3, "BCM").c_str());
                eBCMlo = atof(getkeyval(3, "BCMlo").c_str());
                eBCMhi = atof(getkeyval(3, "BCMhi").c_str());
                eVv = atof(getkeyval(3, "Vv").c_str());
                eMinCP = atof(getkeyval(3, "MinCP").c_str());
                eMaxCP = atof(getkeyval(3, "MaxCP").c_str());
                eBCS = atof(getkeyval(3, "BCS").c_str());
                eBSA = atof(getkeyval(3, "BSA").c_str());
                eBM = atof(getkeyval(1, "BM").c_str());
                eBVV = atoi(getkeyval(2, "BVV").c_str());
                eBRE = atof(getkeyval(3, "BRE").c_str());
                eHiPP = atof(getkeyval(3, "HiPP").c_str());
                eLoPP = atof(getkeyval(3, "LoPP").c_str());
                eCompressorSpeed = atof(getkeyval(3, "CompressorSpeed").c_str());
                eCompressorPower = atof(getkeyval(1, "CompressorPower").c_str());
            }

            if (issection("BuffCoupl.") || issection("BuffCoupl1."))
            {
                secBuffCoupl = true;
                fCType = (getkeyval(1, "CType"));
                fkB = atof(getkeyval(3, "kB").c_str());
                fDmaxB = atof(getkeyval(3, "DmaxB").c_str());
                fFmaxB = atof(getkeyval(3, "FmaxB").c_str());
                fkC = atof(getkeyval(3, "kC").c_str());
                fDmaxC = atof(getkeyval(3, "DmaxC").c_str());
                fFmaxC = atof(getkeyval(3, "FmaxC").c_str());
                fbeta = atof(getkeyval(3, "beta").c_str());
                fAllowedFlag = atoi(getkeyval(2, "AllowedFlag").c_str());
            }

            if (issection("Cntrl."))
            {
                secCntrl = true;
                gBrakeSystem = (getkeyval(1, "BrakeSystem"));
                gBCPN = atoi(getkeyval(2, "BCPN").c_str());
                gBDelay1 = atoi(getkeyval(2, "BDelay1").c_str());
                gBDelay2 = atoi(getkeyval(2, "BDelay2").c_str());
                gBDelay3 = atoi(getkeyval(2, "BDelay3").c_str());
                gBDelay4 = atoi(getkeyval(2, "BDelay4").c_str());
                gASB = (getkeyval(1, "ASB"));
                gLocalBrake = (getkeyval(1, "LocalBrake"));
                gDynamicBrake = (getkeyval(1, "DynamicBrake"));
                // gManualBrake =         (getkeyval(1, "ManualBrake"));
                gFSCircuit = (getkeyval(1, "FSCircuit").c_str());
                gMCPN = atoi(getkeyval(2, "MCPN").c_str());
                gSCPN = atoi(getkeyval(2, "SCPN").c_str());
                gSCIM = atoi(getkeyval(2, "SCIM").c_str());
                gScndS = (getkeyval(1, "ScndS"));
                gCoupledCtrl = (getkeyval(1, "CoupledCtrl"));
                gAutoRelay = (getkeyval(1, "AutoRelay"));
                gIniCDelay = atof(getkeyval(3, "IniCDelay").c_str());
                gSCDelay = atof(getkeyval(3, "SCDelay").c_str());
                gSCDDelay = atof(getkeyval(3, "SCDDelay").c_str());
                gBrakeDelays = (getkeyval(1, "BrakeDelays"));
                gBrakeHandle = (getkeyval(1, "BrakeHandle"));
                gLocBrakeHandle = (getkeyval(1, "LocBrakeHandle"));
                gMaxBPMass = atof(getkeyval(3, "MaxBPMass").c_str());
            }

            if (issection("Security:"))
            {
                secSecurity = true;
                hAwareSystem = (getkeyval(1, "AwareSystem"));
                hAwareMinSpeed = atof(getkeyval(3, "AwareMinSpeed").c_str());
                hAwareDelay = atof(getkeyval(3, "AwareDelay").c_str());
                hSoundSignalDelay = atof(getkeyval(3, "SoundSignalDelay").c_str());
                hEmergencyBrakeDelay = atof(getkeyval(3, "EmergencyBrakeDelay").c_str());
                hRadioStop = (getkeyval(1, "RadioStop"));
            }

            if (issection("Light:"))
            {
                secLight = true;
                iLight = (getkeyval(1, "Light"));
                iLGeneratorEngine = (getkeyval(1, "LGeneratorEngine"));
                iLMaxVoltage = atof(getkeyval(3, "LMaxVoltage").c_str());
                iLMaxCurrent = atof(getkeyval(3, "LMaxCurrent").c_str());
            }

            if (issection("Power:"))
            {
                secPower = true;
                jEnginePower = (getkeyval(1, "EnginePower"));
                jSystemPower = (getkeyval(1, "SystemPower"));
                jCollectorsNo = atoi(getkeyval(2, "CollectorsNo").c_str());
                jMaxVoltage = atof(getkeyval(3, "MaxVoltage").c_str());
                jMaxCurrent = atof(getkeyval(3, "MaxCurrent").c_str());
                jIntR = atof(getkeyval(3, "IntR").c_str());
                jMinH = atof(getkeyval(3, "MinH").c_str());
                jMaxH = atof(getkeyval(3, "MaxH").c_str());
                jCSW = atof(getkeyval(3, "CSW").c_str());
                jMinV = atof(getkeyval(3, "MinV").c_str());
                jMinPress = atof(getkeyval(3, "MinPress").c_str());
                jMaxPress = atof(getkeyval(3, "MaxPress").c_str());
            }

            if (issection("Engine:"))
            {
                secEngine = true;
                kEngineType = (getkeyval(1, "EngineType"));
                kTrans = (getkeyval(1, "Trans"));
                kVolt = atof(getkeyval(3, "Volt").c_str());
                kWindingRes = atof(getkeyval(3, "WindingRes").c_str());
                knmax = atof(getkeyval(3, "nmax").c_str());
            }

            if (issection("Circuit:"))
            {
                secCircuit = true;
                lCircuitRes = atof(getkeyval(3, "CircuitRes").c_str());
                lImaxLo = atoi(getkeyval(2, "ImaxLo").c_str());
                lImaxHi = atoi(getkeyval(2, "ImaxHi").c_str());
                lIminLo = atoi(getkeyval(2, "IminLo").c_str());
                lIminHi = atoi(getkeyval(2, "IminHi").c_str());
            }

            if (issection("RList:"))
            {
                secRList = true;
                mSize = atoi(getkeyval(2, "Size").c_str());
                mRVent = (getkeyval(1, "RVent"));
                mRVentnmax = atof(getkeyval(3, "RVentnmax").c_str());
                mRVentCutOff = atof(getkeyval(3, "RVentCutOff").c_str());
            }

            if (issection("DList:"))
            {
                secDList = true;
                nMmax = atof(getkeyval(3, "Mmax").c_str());
                nnMmax = atof(getkeyval(3, "nMmax").c_str());
                nMnmax = atof(getkeyval(3, "Mnmax").c_str());
                nnmax = atof(getkeyval(3, "nmax").c_str());
                nnominalfill = atof(getkeyval(3, "nominalfill").c_str());
                nMstand = atof(getkeyval(3, "Mstand").c_str());
                nSize = atoi(getkeyval(2, "Size").c_str());
            }

            if (issection("WWList:"))
            {
                secWWList = true;
                getkeyval(2, "Size");
            }

            if (issection("ffList:"))
            {
                secffList = true;
                getkeyval(2, "Size");
            }

            if (issection("TurboPos:"))
            {
                secTurboPos = true;
                getkeyval(2, "TurboPos");
            }

            if (issection("Cntrl.") || startBPT)
            {
                secBPT = true;
                BrakeCtrlPosNo = gBCPN;
                if (BrakeCtrlPosNo > 0)
                    readBPT(BPTLINE, xline); // np wagony nie maja BPT
            }

            if (issection("MotorParamTable0:") || startMPT)
            {
                secMotorParamTable0 = true;
                readMPT(MPTLINE, xline);
            }

            if (issection("RList:") || startRLIST)
            {
                secRList = true;
                readRLIST(RLISTLINE, xline);
            }
        } // is hash
    } // while line
    in.close();
    // Sprawdzenie poprawnosci wczytanych parametrow
    // WriteLog("DATA TEST: " + aCategory + ", " + aType + ", " + bLoadQ + ", " + bLoadAccepted + ",
    // " + dAxle + ", " + dBearingType + ", " + eBrakeValve + ", " + eBM + ", " + jEnginePower + ",
    // " + kEngineType + ", " + mRVent  );
    // WriteLog(" ");
    // WriteLog(" ");

    // Operacje na zebranych parametrach - przypisywanie do wlasciwych zmiennych i ustawianie
    // zaleznosci

    if (aCategory == "train")
        CategoryFlag = 1;
    else if (aCategory == "road")
        CategoryFlag = 2;
    else if (aCategory == "ship")
        CategoryFlag = 4;
    else if (aCategory == "airplane")
        CategoryFlag = 8;
    else if (aCategory == "unimog")
        CategoryFlag = 3;
    else
        ConversionError = MARKERROR(-7, "1", "Improper vechicle category");

    Mass = aMass;
    Mred = aMred;
    Vmax = aVmax;
    Power = aPWR;
    HeatingPower = aHeatingP;
    LightPower = aLightP;
    SandCapacity = aSandCap;
    TrainType = dt_Default;
    if (aType == "PSEUDODIESEL")
        aType = "PDIS";

    if (aType == "EZT")
    {
        TrainType = dt_EZT;
        IminLo = 1;
        IminHi = 2;
        Imin = 1;
    }
    else // wirtualne wartoœci dla rozrz¹dczego
        if (aType == "ET41")
        TrainType = dt_ET41;
    else if (aType == "ET42")
        TrainType = dt_ET42;
    else if (aType == "ET22")
        TrainType = dt_ET22;
    else if (aType == "ET40")
        TrainType = dt_ET40;
    else if (aType == "EP05")
        TrainType = dt_EP05;
    else if (aType == "SN61")
        TrainType = dt_SN61;
    else if (aType == "PDIS")
        TrainType = dt_PseudoDiesel;
    else if (aType == "181")
        TrainType = dt_181;
    else if (aType == "182")
        TrainType = dt_181; // na razie tak

    MaxLoad = bMaxLoad;
    LoadQuantity = bLoadQ;
    OverLoadFactor = bOverLoadFactor;
    LoadSpeed = bLoadSpeed;
    UnLoadSpeed = bUnLoadSpeed;
    Dim.L = cL;
    Dim.W = cW;
    Dim.H = cH;
    Cx = cCx;

    if (Cx == 0)
        Cx = 0.3;
    if (cFloor == -1)
    {
        if (Dim.H <= 2.0)
            Floor = Dim.H; // gdyby nie by³o parametru, lepsze to ni¿ zero
        else
            Floor = 0.0; // zgodnoœæ wsteczna
    }
    else
        Floor = cFloor;

    // Axles
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    WheelDiameter = dD;
    WheelDiameterL = dDl;
    WheelDiameterT = dDt;
    TrackW = dTw;
    AxleInertialMoment = dAIM;
    AxleArangement = dAxle;
    NPoweredAxles = s2NPW(AxleArangement);
    NAxles = NPoweredAxles + s2NNW(AxleArangement);
    BearingType = 1;
    ADist = dAd;
    BDist = dBd;

    if (WheelDiameterL == -1) // gdyby nie by³o parametru...
        WheelDiameterL = WheelDiameter; //... lepsze to ni¿ zero
    else
        WheelDiameterL = dDl;
    if (WheelDiameterT == -1) // gdyby nie by³o parametru...
        WheelDiameterT = WheelDiameter; //... lepsze to ni¿ zero
    else
        WheelDiameterT = dDt;

    if (AxleInertialMoment <= 0)
        AxleInertialMoment = 1;

    if (NAxles == 0)
        ConversionError = MARKERROR(-1, "1", "0 axles, hover cat?");

    if (dBearingType == "Roll")
        BearingType = 1;
    else
        BearingType = 0;
    /*
     WriteLog("CompressorPower " + eCompressorPower);
     WriteLog("NAxles " + IntToStr(NAxles));
     WriteLog("BearingType " + dBearingType);
     WriteLog("params " + BrakeValveParams);
     WriteLog("NBpA " + IntToStr(NBpA));
     WriteLog("MaxBrakeForce " + FloatToStr(MaxBrakeForce));
     WriteLog("TrackBrakeForce " + FloatToStr(TrackBrakeForce));
     WriteLog("MaxBrakePress[3] " + FloatToStr(MaxBrakePress[3]));
     WriteLog("BrakeCylNo " + IntToStr(BrakeCylNo));
     WriteLog("BCD " + FloatToStr(eBCD));
     WriteLog("BCR " + FloatToStr(eBCR));
     WriteLog("BCS " + FloatToStr(eBCS));
     WriteLog("BrakeHandle " + gBrakeHandle);
     WriteLog("BrakeLocHandle " + gLocBrakeHandle);
    */

    // Brakes
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    if (secBrake)
    {
        BrakeValveParams = eBrakeValve;
        BrakeValveDecode(BrakeValveParams);
        BrakeSubsystemDecode();
        NBpA = eNBpA;
        MaxBrakeForce = eMBF;
        BrakeValveSize = eSize;
        TrackBrakeForce = eTBF * 1000;
        MaxBrakePress[3] = eMaxBP;
        // WriteLog("eMaxBP " + FloatToStr(MaxBrakePress[3]));
        if (MaxBrakePress[3] > 0)
        {
            BrakeCylNo = eBCN;
            if (BrakeCylNo > 0)
            {
                MaxBrakePress[0] = eMaxLBP;
                if (MaxBrakePress[0] < 0.01)
                    MaxBrakePress[0] = MaxBrakePress[3];
                MaxBrakePress[1] = eTareMaxBP;
                MaxBrakePress[2] = eMedMaxBP;
                MaxBrakePress[4] = eMaxASBP;
                if (MaxBrakePress[4] < 0.01)
                    MaxBrakePress[4] = 0;
                BrakeCylRadius = eBCR;
                BrakeCylDist = eBCD;
                BrakeCylSpring = eBCS;
                BrakeSlckAdj = eBSA;
                if (eBRE != 0)
                    BrakeRigEff = eBRE;
                else
                    BrakeRigEff = 1;
                BrakeCylMult[0] = eBCM;
                BrakeCylMult[1] = eBCMlo;
                BrakeCylMult[2] = eBCMhi;
                P2FTrans = 100 * PI * sqr(BrakeCylRadius); // w kN/bar    Q: zamieniam SQR() na
                                                           // sqr()
                if ((BrakeCylMult[1] > 0) || (MaxBrakePress[1] > 0))
                    LoadFlag = 1;
                else
                    LoadFlag = 0; //            Q: zamieniam SQR() na sqr()
                BrakeVolume = PI * sqr(BrakeCylRadius) * BrakeCylDist * BrakeCylNo;
                BrakeVVolume = eBVV;
                if (eBM == "P10-Bg")
                    BrakeMethod = bp_P10Bg;
                else if (eBM == "P10-Bgu")
                    BrakeMethod = bp_P10Bgu;
                else if (eBM == "FR513")
                    BrakeMethod = bp_FR513;
                else if (eBM == "Cosid")
                    BrakeMethod = bp_Cosid;
                else if (eBM == "P10yBg")
                    BrakeMethod = bp_P10yBg;
                else if (eBM == "P10yBgu")
                    BrakeMethod = bp_P10yBgu;
                else if (eBM == "Disk1")
                    BrakeMethod = bp_D1;
                else if (eBM == "Disk1+Mg")
                    BrakeMethod = bp_D1 + bp_MHS;
                else if (eBM == "Disk2")
                    BrakeMethod = bp_D2;
                else
                    BrakeMethod = 0;
                if (eRM != 0)
                    RapidMult = eRM;
                else
                    RapidMult = 1;
            }
            else
                ConversionError = MARKERROR(-5, "1", "0 brake cylinder units");
        }
        else
            P2FTrans = 0;

        // WriteLog("eBM=" + eBM + ", " + IntToStr(0));

        if (eHiPP != 0)
            CntrlPipePress = eHiPP;
        else
            CntrlPipePress =
                5 + 0.001 * (Random(10) - Random(10)); // Ra 2014-07: trochê niedok³adnoœci
        HighPipePress = CntrlPipePress;

        if (eHiPP != 0)
            LowPipePress = eLoPP;
        else
            LowPipePress = Min0R(HighPipePress, 3.5);

        DeltaPipePress = HighPipePress - LowPipePress;

        VeselVolume = eVv;
        if (VeselVolume == 0)
            VeselVolume = 0.01;

        MinCompressor = eMinCP;
        MaxCompressor = eMaxCP;
        CompressorSpeed = eCompressorSpeed;

        if (eCompressorPower == "Converter")
            CompressorPower = 2;
        else if (eCompressorPower == "Engine")
            CompressorPower = 3;
        else if (eCompressorPower == "Coupler1")
            CompressorPower = 4;
        else // w³¹czana w silnikowym EZT z przodu
            if (eCompressorPower == "Coupler2")
            CompressorPower = 5;
        else // w³¹czana w silnikowym EZT z ty³u
            if (eCompressorPower == "Main")
            CompressorPower = 0;

        // WriteLog("params " + BrakeValveParams);
        // WriteLog("NBpA " + IntToStr(NBpA));
        // WriteLog("MaxBrakeForce " + FloatToStr(MaxBrakeForce));
        // WriteLog("TrackBrakeForce " + FloatToStr(TrackBrakeForce));
        // WriteLog("MaxBrakePress[3] " + FloatToStr(MaxBrakePress[3]));
        // WriteLog("BrakeCylNo " + IntToStr(BrakeCylNo));
    }

    // Couplers
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

    if (fCType == "Automatic")
        Couplers[0].CouplerType = Automatic;
    else if (fCType == "Screw")
        Couplers[0].CouplerType = Screw;
    else if (fCType == "Chain")
        Couplers[0].CouplerType = Chain;
    else if (fCType == "Bare")
        Couplers[0].CouplerType = Bare;
    else if (fCType == "Articulated")
        Couplers[0].CouplerType = Articulated;
    else
        Couplers[0].CouplerType = NoCoupler;

    if (fAllowedFlag > 0)
        Couplers[0].AllowedFlag = fAllowedFlag;
    if (Couplers[0].AllowedFlag < 0)
        Couplers[0].AllowedFlag = ((-Couplers[0].AllowedFlag) || ctrain_depot);
    if ((Couplers[0].CouplerType != NoCoupler) && (Couplers[0].CouplerType != Bare) &&
        (Couplers[0].CouplerType != Articulated))
    {

        Couplers[0].SpringKC = fkC * 1000;
        Couplers[0].DmaxC = fDmaxC;
        Couplers[0].FmaxC = fFmaxC * 1000;
        Couplers[0].SpringKB = fkB * 1000;
        Couplers[0].DmaxB = fDmaxB;
        Couplers[0].FmaxB = fFmaxB * 1000;
        Couplers[0].beta = fbeta;
    }
    else if (Couplers[0].CouplerType == Bare)
    {
        Couplers[0].SpringKC = 50 * Mass + Ftmax / 0.05;
        Couplers[0].DmaxC = 0.05;
        Couplers[0].FmaxC = 100 * Mass + 2 * Ftmax;
        Couplers[0].SpringKB = 60 * Mass + Ftmax / 0.05;
        Couplers[0].DmaxB = 0.05;
        Couplers[0].FmaxB = 50 * Mass + 2 * Ftmax;
        Couplers[0].beta = 0.3;
    }
    else if (Couplers[0].CouplerType == Articulated)
    {
        Couplers[0].SpringKC = 60 * Mass + 1000;
        Couplers[0].DmaxC = 0.05;
        Couplers[0].FmaxC = 20000000 + 2 * Ftmax;
        Couplers[0].SpringKB = 70 * Mass + 1000;
        Couplers[0].DmaxB = 0.05;
        Couplers[0].FmaxB = 4000000 + 2 * Ftmax;
        Couplers[0].beta = 0.55;
    }
    Couplers[1].SpringKC = Couplers[0].SpringKC;
    Couplers[1].DmaxC = Couplers[0].DmaxC;
    Couplers[1].FmaxC = Couplers[0].FmaxC;
    Couplers[1].SpringKB = Couplers[0].SpringKB;
    Couplers[1].DmaxB = Couplers[0].DmaxB;
    Couplers[1].FmaxB = Couplers[0].FmaxB;
    Couplers[1].beta = Couplers[0].beta;
    Couplers[1].CouplerType = Couplers[0].CouplerType;
    Couplers[1].AllowedFlag = Couplers[0].AllowedFlag;

    // Controllers
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    if (secCntrl)
    {
        if (gBrakeSystem == "Pneumatic")
            BrakeSystem = Pneumatic;
        else if (gBrakeSystem == "ElectroPneumatic")
            BrakeSystem = ElectroPneumatic;
        else
            BrakeSystem = Individual;

        if (BrakeSystem != Individual)
        {

            BrakeCtrlPosNo = gBCPN;
            BrakeDelay[1] = gBDelay1;
            // BrakeDelay[2] = gBDelay2;
            // BrakeDelay[3] = gBDelay3;
            // BrakeDelay[4] = gBDelay4;

            if (gBrakeDelays == "GPR")
                BrakeDelays = bdelay_G + bdelay_P + bdelay_R;
            else if (gBrakeDelays == "PR")
                BrakeDelays = bdelay_P + bdelay_R;
            else if (gBrakeDelays == "GP")
                BrakeDelays = bdelay_G + bdelay_P;
            else if (gBrakeDelays == "R")
            {
                BrakeDelays = bdelay_R;
                BrakeDelayFlag = bdelay_R;
            }
            else if (gBrakeDelays == "P")
            {
                BrakeDelays = bdelay_P;
                BrakeDelayFlag = bdelay_P;
            }
            else if (gBrakeDelays == "G")
            {
                BrakeDelays = bdelay_G;
                BrakeDelayFlag = bdelay_G;
            }
            else if (gBrakeDelays == "GPR+Mg")
                BrakeDelays = bdelay_G + bdelay_P + bdelay_R + bdelay_M;
            else if (gBrakeDelays == "PR+Mg")
                BrakeDelays = bdelay_P + bdelay_R + bdelay_M;

            if (gBrakeHandle == "FV4a")
                BrakeHandle = FV4a;
            else if (gBrakeHandle == "test")
                BrakeHandle = testH;
            else if (gBrakeHandle == "D2")
                BrakeHandle = D2;
            else if (gBrakeHandle == "M394")
                BrakeHandle = M394;
            else if (gBrakeHandle == "Knorr")
                BrakeHandle = Knorr;
            else if (gBrakeHandle == "Westinghouse")
                BrakeHandle = West;
            else if (gBrakeHandle == "FVel6")
                BrakeHandle = FVel6;
            else if (gBrakeHandle == "St113")
                BrakeHandle = St113;

            if (gLocBrakeHandle == "FD1")
                BrakeLocHandle = FD1;
            else if (gLocBrakeHandle == "Knorr")
                BrakeLocHandle = Knorr;
            else if (gLocBrakeHandle == "Westinghouse")
                BrakeLocHandle = West;

            if (gMaxBPMass != 0)
                MBPM = gMaxBPMass * 1000;

            if (BrakeCtrlPosNo > 0)
            {
                if (gASB == "Manual")
                    ASBType = 1;
                else if (gASB == "Automatic")
                    ASBType = 2;
            }
            else
            {
                if (gASB == "Yes")
                    ASBType = 128;
            }
        } // BrakeSystem != individual

        //  WriteLog("gLocalBrake " + gLocalBrake);
        //  WriteLog("gManualBrake " + gManualBrake);
        //  WriteLog("gManualBrake " + gManualBrake);

        if (gLocalBrake == "ManualBrake")
            LocalBrake = ManualBrake;
        else if (gLocalBrake == "PneumaticBrake")
            LocalBrake = PneumaticBrake;
        else if (gLocalBrake == "HydraulicBrake")
            LocalBrake = HydraulicBrake;
        else
            LocalBrake = NoBrake;

        if (gManualBrake == "Yes")
            MBrake = true;
        else
            MBrake = false;

        if (gDynamicBrake == "Passive")
            DynamicBrakeType = dbrake_passive;
        else if (gDynamicBrake == "Switch")
            DynamicBrakeType = dbrake_switch;
        else if (gDynamicBrake == "Reversal")
            DynamicBrakeType = dbrake_reversal;
        else if (gDynamicBrake == "Automatic")
            DynamicBrakeType = dbrake_automatic;
        else
            DynamicBrakeType = dbrake_none;

        MainCtrlPosNo = gMCPN;

        ScndCtrlPosNo = gSCPN;

        ScndInMain = bool(gSCIM);

        if (gAutoRelay == "Optional")
            AutoRelayType = 2;
        else if (gAutoRelay == "Yes")
            AutoRelayType = 1;
        else
            AutoRelayType = 0;

        if (gCoupledCtrl == "Yes")
            CoupledCtrl = true;
        else // wspolny wal
            CoupledCtrl = false;

        if (gScndS == "Yes")
            ScndS = true; // brak pozycji rownoleglej przy niskiej nastawie PSR}
        else
            ScndS = false;

        InitialCtrlDelay = gIniCDelay;
        CtrlDelay = gSCDelay;

        if (gSCDDelay > 0)
            CtrlDownDelay = gSCDDelay;
        else
            CtrlDownDelay = CtrlDelay; // hunter-101012: jesli nie ma SCDDelay;

        if (gFSCircuit == "Yes")
            FastSerialCircuit = 1;
        else
            FastSerialCircuit = 0;
    }

    // Security System
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    if (secSecurity)
    {
        if (Pos("Active", hAwareSystem) > 0)
            SetFlag(SecuritySystem.SystemType, 1);
        if (Pos("CabSignal", hAwareSystem) > 0)
            SetFlag(SecuritySystem.SystemType, 2);
        if (hAwareDelay > 0)
            SecuritySystem.AwareDelay = hAwareDelay;
        if (hAwareMinSpeed > 0)
            SecuritySystem.AwareMinSpeed = hAwareMinSpeed;
        else
            SecuritySystem.AwareMinSpeed = 0.1 * Vmax; // domyœlnie 10% Vmax
        if (hSoundSignalDelay > 0)
            SecuritySystem.SoundSignalDelay = hSoundSignalDelay;
        if (hEmergencyBrakeDelay > 0)
            SecuritySystem.EmergencyBrakeDelay = hEmergencyBrakeDelay;
        if (Pos("Yes", hRadioStop) > 0)
            SecuritySystem.RadioStop = true;
    }

    // Power
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    if (secPower)
    {
        // WriteLog("EnginePower " + jEnginePower);

        if (jEnginePower != "")
        {
            // s:=DUE(ExtractKeyWord(lines,'EnginePower='));
            if (jEnginePower != "")
            {
                EnginePowerSource.SourceType = PowerSourceDecode(jEnginePower);
                PowerParamDecode(xline, "", EnginePowerSource);

                if ((EnginePowerSource.SourceType == Generator) &&
                    (EnginePowerSource.GeneratorEngine == WheelsDriven))
                    ConversionError = -666; // perpetuum mobile?}
                if (Power == 0) // jeœli nie ma mocy, np. rozrz¹dcze EZT
                    EnginePowerSource.SourceType = NotDefined; // to silnik nie ma zasilania
            }
            else
                EnginePowerSource.SourceType = NotDefined;

            if (jSystemPower != "")
            {
                SystemPowerSource.SourceType = PowerSourceDecode(jSystemPower);
                PowerParamDecode(xline, "", SystemPowerSource);
            }
            else
                SystemPowerSource.SourceType = NotDefined;
        }
        jEnginePower = ""; // zeby nastepny pojad mial zresetowane na poczatku wczytywania
    }

    // Engine
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

    if (secEngine)
    {
        if (kEngineType != "")
        {
            EngineType = EngineDecode(kEngineType);

            switch (EngineType)
            {

            case ElectricSeriesMotor:
            {
                NominalVoltage = kVolt;

                x = Split(kTrans, ':'); // 18:79

                if (x.size() != 2)
                {
                    WriteLog("Wrong transmition definition: " + kTrans);
                    break;
                }

                p0 = TrimSpace(x[0]);
                p1 = TrimSpace(x[1]);

                Transmision.NToothW = atoi(p1.c_str());
                Transmision.NToothM = atoi(p0.c_str());

                // ToothW to drugi parametr czyli 79
                // ToothM to pierwszy czyli 18

                // WriteLog("trans " + IntToStr(Transmision.NToothW ) + "/" +
                // IntToStr(Transmision.NToothM ));
                // if (kTrans != "")
                if (Transmision.NToothM > 0)
                    Transmision.Ratio = Transmision.NToothW / Transmision.NToothM;
                else
                    Transmision.Ratio = 1;

                WindingRes = kWindingRes;
                if (WindingRes == 0)
                    WindingRes = 0.01;
                // WriteLog("WindingRes " + FloatToStr(WindingRes));

                nmax = knmax / 60.0;
                // WriteLog("nmax " + FloatToStr(nmax ));
            }

            break;
            } // switch
        }
        kTrans = "";
    }

    // Circuit
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

    if (secCircuit)
    {
        // s := DUE(ExtractKeyWord(lines, 'CircuitRes=')); writepaslog('CircuitRes', s);
        CircuitRes = (lCircuitRes);

        // s := DUE(ExtractKeyWord(lines, 'IminLo=')); writepaslog('IminLo', s);
        IminLo = (lIminLo);

        // s := DUE(ExtractKeyWord(lines, 'IminHi=')); writepaslog('IminHi', s);
        IminHi = (lIminHi);

        // s := DUE(ExtractKeyWord(lines, 'ImaxLo=')); writepaslog('ImaxLo', s);
        ImaxLo = (lImaxLo);

        // s := DUE(ExtractKeyWord(lines, 'ImaxHi=')); writepaslog('ImaxHi', s);
        ImaxHi = (lImaxHi);
        Imin = IminLo;
        Imax = ImaxLo;
    }

    // RList
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

    if (secRList)
    {
        RlistSize = (mSize);

        if (mRVent == "Automatic")
            RVentType = 2;
        else if (mRVent == "Yes")
            RVentType = 1;
        else
            RVentType = 0;

        if (RVentType > 0)
        {
            RVentnmax = (mRVentnmax) / 60.0;

            RVentCutOff = (mRVentCutOff);
        }
    }

    if (ConversionError == 0)
        OK = true;
    else
        OK = false;

    // WriteLog("");
    // WriteLog("----------------------------------------------------------------------------------------");
    WriteLog("CERROR: " + to_string(ConversionError) + ", SUCCES: " + to_string(OK));
    // WriteLogSS();
    //WriteLog("");
    return OK;
} // LoadFIZ()

// *************************************************************************************************
// Q: 20160717
// *************************************************************************************************

bool TMoverParameters::CheckLocomotiveParameters(bool ReadyFlag, int Dir)
{
	WriteLog("check locomotive parameters...");
	int b;
	bool OK = true;

	AutoRelayFlag = (AutoRelayType == 1);

	Sand = SandCapacity;

	// WriteLog("aa = " + AxleArangement + " " + std::string( Pos("o", AxleArangement)) );

	if ((Pos("o", AxleArangement) > 0) && (EngineType == ElectricSeriesMotor))
		OK = (RList[1].Bn * RList[1].Mn ==
			NPoweredAxles); // test poprawnosci ilosci osi indywidualnie napedzanych
							// WriteLogSS("aa ok", BoolToYN(OK));

	if (BrakeSystem == Individual)
		if (BrakeSubsystem != ss_None)
			OK = false; //!

	if ((BrakeVVolume == 0) && (MaxBrakePress[3] > 0) && (BrakeSystem != Individual))
		BrakeVVolume = MaxBrakePress[3] / (5 - MaxBrakePress[3]) *
		(BrakeCylRadius * BrakeCylRadius * BrakeCylDist * BrakeCylNo * PI) * 1000;
	if (BrakeVVolume == 0)
		BrakeVVolume = 0.01;

	// WriteLog("BVV = "  + FloatToStr(BrakeVVolume));

	if ((TestFlag(BrakeDelays, bdelay_G)) &&
		((!TestFlag(BrakeDelays, bdelay_R)) ||
			(Power > 1))) // ustalanie srednicy przewodu glownego (lokomotywa lub napêdowy
		Spg = 0.792;
	else
		Spg = 0.507;

	// taki mini automat - powinno byc ladnie dobrze :)
	BrakeDelayFlag = bdelay_P;
	if ((TestFlag(BrakeDelays, bdelay_G)) && !(TestFlag(BrakeDelays, bdelay_R)))
		BrakeDelayFlag = bdelay_G;
	if ((TestFlag(BrakeDelays, bdelay_R)) && !(TestFlag(BrakeDelays, bdelay_G)))
		BrakeDelayFlag = bdelay_R;

	int DefBrakeTable[8] = { 15, 4, 25, 25, 13, 3, 12, 2 };

	// checking ready flag
	// to dac potem do init
	if (ReadyFlag) // gotowy do drogi
	{
		// WriteLog("Ready to depart");
		CompressedVolume = VeselVolume * MinCompressor * (9.8) / 10;
		ScndPipePress = CompressedVolume / VeselVolume;
		PipePress = CntrlPipePress;
		BrakePress = 0;
		LocalBrakePos = 0;

		if (CabNo == 0)
			BrakeCtrlPos = floor(Handle->GetPos(bh_NP)); // Q: TODO: Trunc na floor
		else
			BrakeCtrlPos = floor(Handle->GetPos(bh_RP));
		MainSwitch(false);
		PantFront(true);
		PantRear(true);
		MainSwitch(true);
		ActiveDir = 0; // Dir; //nastawnik kierunkowy - musi byæ ustawiane osobno!
		DirAbsolute = ActiveDir * CabNo; // kierunek jazdy wzglêdem sprzêgów
		LimPipePress = CntrlPipePress;
	}
	else
	{ // zahamowany}
		WriteLog("Bracked");
		Volume = BrakeVVolume * MaxBrakePress[3];
		CompressedVolume = VeselVolume * MinCompressor * 0.55;
		ScndPipePress = 5.1;
		PipePress = LowPipePress;
		PipeBrakePress = MaxBrakePress[3];
		BrakePress = MaxBrakePress[3];
		LocalBrakePos = 0;
		BrakeCtrlPos = Trunc(Handle->GetPos(bh_NP)); // Q: TODO: Trunc na floor
		LimPipePress = LowPipePress;
	}

	if (LoadFlag > 0)
	{
		if (Load < MaxLoad * 0.45)
		{
			IncBrakeMult();
			IncBrakeMult();
			DecBrakeMult(); // TODO: przeinesiono do mover.cpp
			if (Load < MaxLoad * 0.35)
				DecBrakeMult();
		}
		if (Load >= MaxLoad * 0.45)
		{
			IncBrakeMult(); // TODO: przeinesiono do mover.cpp
			if (Load >= MaxLoad * 0.55)
				IncBrakeMult();
		}
	}

	if (BrakeOpModes & bom_PS)
		BrakeOpModeFlag = bom_PS;
	else
		BrakeOpModeFlag = bom_PN;

	// yB: jesli pojazdy nie maja zadeklarowanych czasow, to wsadz z przepisow +-16,(6)%
	for (b = 1; b < 4; b++)
	{
		if (BrakeDelay[b] == 0)
			BrakeDelay[b] = DefBrakeTable[b];
		BrakeDelay[b] = BrakeDelay[b] * (2.5 + Random(0.0, 0.2)) / 3.0;
	}

	// WriteLog("SPG = " + FloatToStr(Spg));

	switch (BrakeValve)
	{
	case W:
	case K:
	{
		WriteLog("XBT W, K");
		Hamulec = new TWest(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume,
			BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA, PipePress,
			HighPipePress, LowPipePress, BrakePress, BrakeDelayFlag);
		if (MBPM < 2) // jesli przystawka wazaca
			Hamulec->SetLP(0, MaxBrakePress[3], 0);
		else
			Hamulec->SetLP(Mass, MBPM, MaxBrakePress[1]);
		break;
	}
	case KE:
	{
		WriteLog("XBT WKE");
		Hamulec = new TKE(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume, BrakeCylNo,
			BrakeDelays, BrakeMethod, NAxles, NBpA, PipePress, HighPipePress,
			LowPipePress, BrakePress, BrakeDelayFlag);
		Hamulec->SetRM(RapidMult);
		if (MBPM < 2) // jesli przystawka wazaca
			Hamulec->SetLP(0, MaxBrakePress[3], 0);
		else
			Hamulec->SetLP(Mass, MBPM, MaxBrakePress[1]);
		break;
	}
	case NESt3:
	case ESt3:
	case ESt3AL2:
	case ESt4:
	{
		WriteLog("XBT NESt3, ESt3, ESt3AL2, ESt4");
		Hamulec = new TNESt3(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume,
			BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA, PipePress,
			HighPipePress, LowPipePress, BrakePress, BrakeDelayFlag);
		(static_cast<TNESt3 *>(Hamulec))->SetSize(BrakeValveSize, BrakeValveParams);
		if (MBPM < 2) // jesli przystawka wazaca
			Hamulec->SetLP(0, MaxBrakePress[3], 0);
		else
			Hamulec->SetLP(Mass, MBPM, MaxBrakePress[1]);
		break;
	}

	case LSt:
	{
		WriteLog("XBT LSt");
		Hamulec = new TLSt(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume, BrakeCylNo,
			BrakeDelays, BrakeMethod, NAxles, NBpA, PipePress, HighPipePress,
			LowPipePress, BrakePress, BrakeDelayFlag);
		Hamulec->SetRM(RapidMult);
		break;
	}
	case EStED:
	{
		WriteLog("XBT EStED");
		Hamulec = new TEStED(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume,
			BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA, PipePress,
			HighPipePress, LowPipePress, BrakePress, BrakeDelayFlag);
		Hamulec->SetRM(RapidMult);
		break;
	}
	case EP2:
	{
		WriteLog("XBT EP2");
		Hamulec = new TEStEP2(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume,
			BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA, PipePress,
			HighPipePress, LowPipePress, BrakePress, BrakeDelayFlag);
		Hamulec->SetLP(Mass, MBPM, MaxBrakePress[1]);
		break;
	}

	case CV1:
	{
		WriteLog("XBT CV1");
		Hamulec = new TCV1(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume, BrakeCylNo,
			BrakeDelays, BrakeMethod, NAxles, NBpA, PipePress, HighPipePress,
			LowPipePress, BrakePress, BrakeDelayFlag);
		break;
	}
	case CV1_L_TR:
	{
		WriteLog("XBT CV1_L_T");
		Hamulec = new TCV1L_TR(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume,
			BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA, PipePress,
			HighPipePress, LowPipePress, BrakePress, BrakeDelayFlag);
		break;
	}

	default:
		Hamulec = new TBrake(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume,
			BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA, PipePress,
			HighPipePress, LowPipePress, BrakePress, BrakeDelayFlag);
	}

	Hamulec->SetASBP(MaxBrakePress[4]);

	switch (BrakeHandle)
	{
	case FV4a:
		Handle = new TFV4aM();
		break;
	case FVel6:
		Handle = new TFVel6();
		break;
	case testH:
		Handle = new Ttest();
		break;
	case M394:
		Handle = new TM394();
		break;
	case Knorr:
		Handle = new TH14K1();
		break;
	case St113:
		Handle = new TSt113();
		break;
	default:
		Handle = new TDriverHandle();
	}

	switch (BrakeLocHandle)
	{
	case FD1:
	{
		LocHandle = new TFD1();
		LocHandle->Init(MaxBrakePress[0]);
		break;
	}
	case Knorr:
	{
		LocHandle = new TH1405();
		LocHandle->Init(MaxBrakePress[0]);
		break;
	}
	default:
		LocHandle = new TDriverHandle();
	}

	Pipe = new TReservoir();
	Pipe2 = new TReservoir(); // zabezpieczenie, bo sie PG wywala... :(
	Pipe->CreateCap((Max0R(Dim.L, 14) + 0.5) * Spg * 1); // dlugosc x przekroj x odejscia i takie
														 // tam
	Pipe2->CreateCap((Max0R(Dim.L, 14) + 0.5) * Spg * 1);

	if (LightsPosNo > 0)
		LightsPos = LightsDefPos;

	ActFlowSpeed = 0;
	BrakeCtrlPosR = BrakeCtrlPos;

	if (BrakeLocHandle == Knorr)
		LocalBrakePos = 5;

	Pipe->CreatePress(PipePress);
	Pipe2->CreatePress(ScndPipePress);
	Pipe->Act();
	Pipe2->Act();

	EqvtPipePress = PipePress;

	Handle->Init(PipePress);

	ComputeConstans();

	if (TrainType == dt_ET22)
		CompressorPower = 0;

	// Hamulec->Init(PipePress, HighPipePress, LowPipePress, BrakePress, BrakeDelayFlag);

	ScndPipePress = Compressor;

	// WriteLogSS("OK=", BoolTo10(OK));
	// WriteLog("");

	return OK;
}

// *************************************************************************************************
// Q: 20160717
// Funkcja pelniaca role pierwotnej CheckLocomotiveParameters(bool ReadyFlag, int Dir)
// wywolywana w dynobj.cpp w double TDynamicObject::Init()
// *************************************************************************************************
bool TMoverParameters::CreateBrakeSys(bool ReadyFlag)
{
    WriteLog("check locomotive parameters...");
    int b;
    bool OK = true;

    AutoRelayFlag = (AutoRelayType == 1);

    Sand = SandCapacity;

    // WriteLog("aa = " + AxleArangement + " " + std::string( Pos("o", AxleArangement)) );

    if ((Pos("o", AxleArangement) > 0) && (EngineType == ElectricSeriesMotor))
        OK = (RList[1].Bn * RList[1].Mn ==
              NPoweredAxles); // test poprawnosci ilosci osi indywidualnie napedzanych
    // WriteLogSS("aa ok", BoolToYN(OK));

    if (BrakeSystem == Individual)
        if (BrakeSubsystem != ss_None)
            OK = false; //!

    if ((BrakeVVolume == 0) && (MaxBrakePress[3] > 0) && (BrakeSystem != Individual))
        BrakeVVolume = MaxBrakePress[3] / (5 - MaxBrakePress[3]) *
                       (BrakeCylRadius * BrakeCylRadius * BrakeCylDist * BrakeCylNo * PI) * 1000;
    if (BrakeVVolume == 0)
        BrakeVVolume = 0.01;

    // WriteLog("BVV = "  + FloatToStr(BrakeVVolume));

    if ((TestFlag(BrakeDelays, bdelay_G)) &&
        ((!TestFlag(BrakeDelays, bdelay_R)) ||
         (Power > 1))) // ustalanie srednicy przewodu glownego (lokomotywa lub napêdowy
        Spg = 0.792;
    else
        Spg = 0.507;

    // taki mini automat - powinno byc ladnie dobrze :)
    BrakeDelayFlag = bdelay_P;
    if ((TestFlag(BrakeDelays, bdelay_G)) && !(TestFlag(BrakeDelays, bdelay_R)))
        BrakeDelayFlag = bdelay_G;
    if ((TestFlag(BrakeDelays, bdelay_R)) && !(TestFlag(BrakeDelays, bdelay_G)))
        BrakeDelayFlag = bdelay_R;

    int DefBrakeTable[8] = {15, 4, 25, 25, 13, 3, 12, 2};

    // checking ready flag
    // to dac potem do init
    if (ReadyFlag) // gotowy do drogi
    {
        // WriteLog("Ready to depart");
        CompressedVolume = VeselVolume * MinCompressor * (9.8) / 10;
        ScndPipePress = CompressedVolume / VeselVolume;
        PipePress = CntrlPipePress;
        BrakePress = 0;
        LocalBrakePos = 0;

        if (CabNo == 0)
            BrakeCtrlPos = floor(Handle->GetPos(bh_NP)); // Q: TODO: Trunc na floor
        else
            BrakeCtrlPos = floor(Handle->GetPos(bh_RP));
        MainSwitch(false);
        PantFront(true);
        PantRear(true);
        MainSwitch(true);
        ActiveDir = 0; // Dir; //nastawnik kierunkowy - musi byæ ustawiane osobno!
        DirAbsolute = ActiveDir * CabNo; // kierunek jazdy wzglêdem sprzêgów
        LimPipePress = CntrlPipePress;
    }
    else
    { // zahamowany}
        WriteLog("Bracked");
        Volume = BrakeVVolume * MaxBrakePress[3];
        CompressedVolume = VeselVolume * MinCompressor * 0.55;
        ScndPipePress = 5.1;
        PipePress = LowPipePress;
        PipeBrakePress = MaxBrakePress[3];
        BrakePress = MaxBrakePress[3];
        LocalBrakePos = 0;
        BrakeCtrlPos = Trunc(Handle->GetPos(bh_NP)); // Q: TODO: Trunc na floor
        LimPipePress = LowPipePress;
    }

    if (LoadFlag > 0)
    {
        if (Load < MaxLoad * 0.45)
        {
            IncBrakeMult();
            IncBrakeMult();
            DecBrakeMult(); // TODO: przeinesiono do mover.cpp
            if (Load < MaxLoad * 0.35)
                DecBrakeMult();
        }
        if (Load >= MaxLoad * 0.45)
        {
            IncBrakeMult(); // TODO: przeinesiono do mover.cpp
            if (Load >= MaxLoad * 0.55)
                IncBrakeMult();
        }
    }

    if (BrakeOpModes & bom_PS)
        BrakeOpModeFlag = bom_PS;
    else
        BrakeOpModeFlag = bom_PN;

    // yB: jesli pojazdy nie maja zadeklarowanych czasow, to wsadz z przepisow +-16,(6)%
    for (b = 1; b < 4; b++)
    {
        if (BrakeDelay[b] == 0)
            BrakeDelay[b] = DefBrakeTable[b];
        BrakeDelay[b] = BrakeDelay[b] * (2.5 + Random(0.0, 0.2)) / 3.0;
    }

    // WriteLog("SPG = " + FloatToStr(Spg));

    switch (BrakeValve)
    {
    case W:
    case K:
    {
        WriteLog("XBT W, K");
        Hamulec = new TWest(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume,
                            BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA, PipePress,
                            HighPipePress, LowPipePress, BrakePress, BrakeDelayFlag);
        if (MBPM < 2) // jesli przystawka wazaca
            Hamulec->SetLP(0, MaxBrakePress[3], 0);
        else
            Hamulec->SetLP(Mass, MBPM, MaxBrakePress[1]);
        break;
    }
    case KE:
    {
        WriteLog("XBT WKE");
        Hamulec = new TKE(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume, BrakeCylNo,
                          BrakeDelays, BrakeMethod, NAxles, NBpA, PipePress, HighPipePress,
                          LowPipePress, BrakePress, BrakeDelayFlag);
        Hamulec->SetRM(RapidMult);
        if (MBPM < 2) // jesli przystawka wazaca
            Hamulec->SetLP(0, MaxBrakePress[3], 0);
        else
            Hamulec->SetLP(Mass, MBPM, MaxBrakePress[1]);
        break;
    }
    case NESt3:
    case ESt3:
    case ESt3AL2:
    case ESt4:
    {
        WriteLog("XBT NESt3, ESt3, ESt3AL2, ESt4");
        Hamulec = new TNESt3(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume,
                             BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA, PipePress,
                             HighPipePress, LowPipePress, BrakePress, BrakeDelayFlag);
        (static_cast<TNESt3 *>(Hamulec))->SetSize(BrakeValveSize, BrakeValveParams);
        if (MBPM < 2) // jesli przystawka wazaca
            Hamulec->SetLP(0, MaxBrakePress[3], 0);
        else
            Hamulec->SetLP(Mass, MBPM, MaxBrakePress[1]);
        break;
    }

    case LSt:
    {
        WriteLog("XBT LSt");
        Hamulec = new TLSt(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume, BrakeCylNo,
                           BrakeDelays, BrakeMethod, NAxles, NBpA, PipePress, HighPipePress,
                           LowPipePress, BrakePress, BrakeDelayFlag);
        Hamulec->SetRM(RapidMult);
        break;
    }
    case EStED:
    {
        WriteLog("XBT EStED");
        Hamulec = new TEStED(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume,
                             BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA, PipePress,
                             HighPipePress, LowPipePress, BrakePress, BrakeDelayFlag);
        Hamulec->SetRM(RapidMult);
        break;
    }
    case EP2:
    {
        WriteLog("XBT EP2");
        Hamulec = new TEStEP2(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume,
                              BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA, PipePress,
                              HighPipePress, LowPipePress, BrakePress, BrakeDelayFlag);
        Hamulec->SetLP(Mass, MBPM, MaxBrakePress[1]);
        break;
    }

    case CV1:
    {
        WriteLog("XBT CV1");
        Hamulec = new TCV1(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume, BrakeCylNo,
                           BrakeDelays, BrakeMethod, NAxles, NBpA, PipePress, HighPipePress,
                           LowPipePress, BrakePress, BrakeDelayFlag);
        break;
    }
    case CV1_L_TR:
    {
        WriteLog("XBT CV1_L_T");
        Hamulec = new TCV1L_TR(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume,
                               BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA, PipePress,
                               HighPipePress, LowPipePress, BrakePress, BrakeDelayFlag);
        break;
    }

    default:
        Hamulec = new TBrake(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume,
                             BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA, PipePress,
                             HighPipePress, LowPipePress, BrakePress, BrakeDelayFlag);
    }

    Hamulec->SetASBP(MaxBrakePress[4]);

    switch (BrakeHandle)
    {
    case FV4a:
        Handle = new TFV4aM();
        break;
    case FVel6:
        Handle = new TFVel6();
        break;
    case testH:
        Handle = new Ttest();
        break;
    case M394:
        Handle = new TM394();
        break;
    case Knorr:
        Handle = new TH14K1();
        break;
    case St113:
        Handle = new TSt113();
        break;
    default:
        Handle = new TDriverHandle();
    }

    switch (BrakeLocHandle)
    {
    case FD1:
    {
        LocHandle = new TFD1();
        LocHandle->Init(MaxBrakePress[0]);
        break;
    }
    case Knorr:
    {
        LocHandle = new TH1405();
        LocHandle->Init(MaxBrakePress[0]);
        break;
    }
    default:
        LocHandle = new TDriverHandle();
    }

    Pipe = new TReservoir();
    Pipe2 = new TReservoir(); // zabezpieczenie, bo sie PG wywala... :(
    Pipe->CreateCap((Max0R(Dim.L, 14) + 0.5) * Spg * 1); // dlugosc x przekroj x odejscia i takie
    // tam
    Pipe2->CreateCap((Max0R(Dim.L, 14) + 0.5) * Spg * 1);

    if (LightsPosNo > 0)
        LightsPos = LightsDefPos;

    ActFlowSpeed = 0;
    BrakeCtrlPosR = BrakeCtrlPos;

    if (BrakeLocHandle == Knorr)
        LocalBrakePos = 5;

    Pipe->CreatePress(PipePress);
    Pipe2->CreatePress(ScndPipePress);
    Pipe->Act();
    Pipe2->Act();

    EqvtPipePress = PipePress;

    Handle->Init(PipePress);

    ComputeConstans();

    if (TrainType == dt_ET22)
        CompressorPower = 0;

    // Hamulec->Init(PipePress, HighPipePress, LowPipePress, BrakePress, BrakeDelayFlag);

    ScndPipePress = Compressor;

    // WriteLogSS("OK=", BoolTo10(OK));
    // WriteLog("");

    return OK;
}

// *************************************************************************************************
// Q: 20160714
// *************************************************************************************************
void TMoverParameters::PutCommand(std::string NewCommand, double NewValue1, double NewValue2,
                                  const TLocation &NewLocation)
{
    CommandLast = NewCommand; // zapamiêtanie komendy

    CommandIn.Command = NewCommand;
    CommandIn.Value1 = NewValue1;
    CommandIn.Value2 = NewValue2;
    CommandIn.Location = NewLocation;
    // czy uruchomic tu RunInternalCommand? nie wiem
}

// *************************************************************************************************
// Q: 20160714
// *************************************************************************************************
double TMoverParameters::GetExternalCommand(std::string &Command)
{
    Command = CommandOut;
    return ValueOut;
}

// *************************************************************************************************
// Q: 20160714
// *************************************************************************************************
bool TMoverParameters::SendCtrlBroadcast(std::string CtrlCommand, double ctrlvalue)
{
    int b;
    bool OK;

    OK = ((CtrlCommand != CommandIn.Command) && (ctrlvalue != CommandIn.Value1));
    //  if (OK)
    //   for (b=0; b<1; b++ )
    //     if (TestFlag(Couplers[b].CouplingFlag, ctrain_controll))
    //      if (Couplers[b].Connected->SetInternalCommand(CtrlCommand, ctrlvalue, DirF(b)))
    //       OK = (Couplers[b].Connected->RunInternalCommand() || OK);

    return OK;
}

// *************************************************************************************************
// Q: 20160714
// *************************************************************************************************
bool TMoverParameters::SetInternalCommand(std::string NewCommand, double NewValue1,
                                          double NewValue2)
{
    bool SIC;
    if ((CommandIn.Command == NewCommand) && (CommandIn.Value1 == NewValue1) &&
        (CommandIn.Value2 == NewValue2))
        SIC = false;
    else
    {
        CommandIn.Command = NewCommand;
        CommandIn.Value1 = NewValue1;
        CommandIn.Value2 = NewValue2;
        SIC = true;
        LastLoadChangeTime = 0; // zerowanie czasu (roz)³adowania
    }

    return SIC;
}

// *************************************************************************************************
// Q: 20160714
// *************************************************************************************************
bool TMoverParameters::SendCtrlToNext(std::string CtrlCommand, double ctrlvalue, double dir)
{
    bool OK;
    int d; // numer sprzêgu w kierunku którego wysy³amy

    // Ra: by³ problem z propagacj¹, jeœli w sk³adzie jest pojazd wstawiony odwrotnie
    // Ra: problem jest równie¿, jeœli AI bêdzie na koñcu sk³adu
    OK = (dir != 0); // and Mains;
    d = (1 + Sign(dir)) / 2; // dir=-1=>d=0, dir=1=>d=1 - wysy³anie tylko w ty³
    //- if (OK) //musi byæ wybrana niezerowa kabina
    // with Couplers[d] do //w³asny sprzêg od strony (d)
    //-   if (TestFlag(Couplers[d].CouplingFlag, ctrain_controll))
    //-    if (Couplers[d].ConnectedNr != d) //jeœli ten nastpêny jest zgodny z aktualnym
    //-     {
    //-      if (Couplers[d].Connected->SetInternalCommand(CtrlCommand, ctrlvalue, dir))
    //-       OK = (Couplers[d].Connected->RunInternalCommand() && OK); //tu jest rekurencja
    //-     }
    //-     else //jeœli nastêpny jest ustawiony przeciwnie, zmieniamy kierunek
    //-      if (Couplers[d].Connected->SetInternalCommand(CtrlCommand, ctrlvalue, -dir))
    //-       OK = (Couplers[d].Connected->RunInternalCommand() && OK); //tu jest rekurencja
    return OK;
}

// *************************************************************************************************
// Q: 20160723
// *************************************************************************************************
// wys³anie komendy otrzymanej z kierunku CValue2 (wzglêdem sprzêgów: 1=przod,-1=ty³)
// Ra: Jest tu problem z rekurencj¹. Trzeba by oddzieliæ wykonywanie komend od mechanizmu
// ich propagacji w sk³adzie. Osobnym problemem mo¿e byæ propagacja tylko w jedn¹ stronê.
// Jeœli jakiœ cz³on jest wstawiony odwrotnie, to równie¿ odwrotnie musi wykonywaæ
// komendy zwi¹zane z kierunkami (PantFront, PantRear, DoorLeft, DoorRight).
// Komenda musi byæ zdefiniowana tutaj, a jeœli siê wywo³uje funkcjê, to ona nie mo¿e
// sama przesy³aæ do kolejnych pojazdów. Nale¿y te¿ siê zastanowiæ, czy dla uzyskania
// jakiejœ zmiany (np. IncMainCtrl) lepiej wywo³aæ funkcjê, czy od razu wys³aæ komendê.
bool TMoverParameters::RunCommand(std::string command, double CValue1, double CValue2)
{
    bool OK;
    std::string testload;
    OK = false;
    ClearPendingExceptions();
    /*
      {test komend sterowania ukrotnionego}
      if command='MainCtrl' then
       begin
         if MainCtrlPosNo>=Trunc(CValue1) then
          MainCtrlPos:=Trunc(CValue1);
         //--OK:=SendCtrlToNext(command,CValue1,CValue2);
       end
      else if command='ScndCtrl' then
       begin
         if ScndCtrlPosNo>=Trunc(CValue1) then
          ScndCtrlPos:=Trunc(CValue1);
         //--OK:=SendCtrlToNext(command,CValue1,CValue2);
       end
    (*  else if command='BrakeCtrl' then
       begin
         if BrakeCtrlPosNo>=Trunc(CValue1) then
          begin
            BrakeCtrlPos:=Trunc(CValue1);
            OK:=SendCtrlToNext(command,CValue1,CValue2);
          end;
       end *)
      else if command='Brake' then //youBy - jak sie EP hamuje, to trza sygnal wyslac...
       begin
         Hamulec.SetEPS(CValue1);
         //fBrakeCtrlPos:=BrakeCtrlPos; //to powinnno byæ w jednym miejscu, aktualnie w C++!!!
         BrakePressureActual:=BrakePressureTable[BrakeCtrlPos];
         //--OK:=SendCtrlToNext(command,CValue1,CValue2);
       end //youby - odluzniacz hamulcow, przyda sie
      else if command='BrakeReleaser' then
       begin
         //--OK:=BrakeReleaser(Round(CValue1)); //samo siê przesy³a dalej    // TODO: funkcja
    przeniesiona do mover.cpp
         //OK:=SendCtrlToNext(command,CValue1,CValue2); //to robi³o kaskadê 2^n
       end
      else if command='MainSwitch' then
       begin
         if CValue1=1 then
          begin
            Mains:=true;
            if (EngineType=DieselEngine) and Mains then
              dizel_enginestart:=true;
          end
         else Mains:=false;
         //--OK:=SendCtrlToNext(command,CValue1,CValue2);
       end
      else if command='Direction' then
       begin
         ActiveDir:=Trunc(CValue1);
         DirAbsolute:=ActiveDir*CabNo;
        //--OK:=SendCtrlToNext(command,CValue1,CValue2);
       end
      else if command='CabActivisation' then
       begin
    //  OK:=Power>0.01;
        case Trunc(CValue1*CValue2) of //CValue2 ma zmieniany znak przy niezgodnoœci sprzêgów
          1 : CabNo:= 1;
         -1 : CabNo:=-1;
        else CabNo:=0; //gdy CValue1==0
        end;
        DirAbsolute:=ActiveDir*CabNo;
        //--OK:=SendCtrlToNext(command,CValue1,CValue2);
       end
      else if command='AutoRelaySwitch' then
       begin
         if (CValue1=1) and (AutoRelayType=2) then AutoRelayFlag:=true
         else AutoRelayFlag:=false;
         //--OK:=SendCtrlToNext(command,CValue1,CValue2);
       end
      else if command='FuseSwitch' then
       begin
          if ((EngineType=ElectricSeriesMotor)or(EngineType=DieselElectric)) and FuseFlag and
    (CValue1=1) and
             (MainCtrlActualPos=0) and (ScndCtrlActualPos=0) and Mains then
    {      if (EngineType=ElectricSeriesMotor) and (CValue1=1) and
             (MainCtrlActualPos=0) and (ScndCtrlActualPos=0) and Mains then}
            FuseFlag:=false;  {wlaczenie ponowne obwodu}
         // if ((EngineType=ElectricSeriesMotor)or(EngineType=DieselElectric)) and not FuseFlag and
    (CValue1=0) and Mains then
         //   FuseFlag:=true;
          //--OK:=SendCtrlToNext(command,CValue1,CValue2);
       end
      else if command='ConverterSwitch' then         {NBMX}
       begin
         if (CValue1=1) then ConverterAllow:=true
         else if (CValue1=0) then ConverterAllow:=false;
         //--OK:=SendCtrlToNext(command,CValue1,CValue2);
       end
      else if command='BatterySwitch' then         {NBMX}
       begin
         //--if (CValue1=1) then Battery:=true
         //--else if (CValue1=0) then Battery:=false;
         //--if (Battery) and (ActiveCab<>0) {or (TrainType=dt_EZT)} then
         //-- SecuritySystem.Status:=SecuritySystem.Status or s_waiting //aktywacja czuwaka
         //--else
         //-- SecuritySystem.Status:=0; //wy³¹czenie czuwaka
         //--OK:=SendCtrlToNext(command,CValue1,CValue2);
       end
    //   else if command='EpFuseSwitch' then         {NBMX}
    //   begin
    //     if (CValue1=1) then EpFuse:=true
    //     else if (CValue1=0) then EpFuse:=false;
    //     OK:=SendCtrlToNext(command,CValue1,CValue2);
    //   end
      else if command='CompressorSwitch' then         {NBMX}
       begin
         if (CValue1=1) then CompressorAllow:=true
         else if (CValue1=0) then CompressorAllow:=false;
         //--OK:=SendCtrlToNext(command,CValue1,CValue2);
       end
      else if command='DoorOpen' then         {NBMX}
       begin //Ra: uwzglêdniæ trzeba jeszcze zgodnoœæ sprzêgów
        if (CValue2>0) then
         begin //normalne ustawienie pojazdu
          if (CValue1=1) OR (CValue1=3) then
           DoorLeftOpened:=true;
          if (CValue1=2) OR (CValue1=3) then
           DoorRightOpened:=true;
         end
        else
         begin //odwrotne ustawienie pojazdu
          if (CValue1=2) OR (CValue1=3) then
           DoorLeftOpened:=true;
          if (CValue1=1) OR (CValue1=3) then
           DoorRightOpened:=true;
         end;
        //--OK:=SendCtrlToNext(command,CValue1,CValue2);
       end
      else if command='DoorClose' then         {NBMX}
       begin //Ra: uwzglêdniæ trzeba jeszcze zgodnoœæ sprzêgów
        if (CValue2>0) then
         begin //normalne ustawienie pojazdu
          if (CValue1=1) OR (CValue1=3) then
           DoorLeftOpened:=false;
          if (CValue1=2) OR (CValue1=3) then
           DoorRightOpened:=false;
         end
        else
         begin //odwrotne ustawienie pojazdu
          if (CValue1=2) OR (CValue1=3) then
           DoorLeftOpened:=false;
          if (CValue1=1) OR (CValue1=3) then
           DoorRightOpened:=false;
         end;
        //--OK:=SendCtrlToNext(command,CValue1,CValue2);
       end
      else if command='PantFront' then         {Winger 160204}
       begin //Ra: uwzglêdniæ trzeba jeszcze zgodnoœæ sprzêgów
       //Czemu EZT ma byæ traktowane inaczej? Ukrotnienie ma, a cz³on mo¿e byæ odwrócony
         if (TrainType=dt_EZT) then
         begin //'ezt'
           if (CValue1=1) then
            begin
            PantFrontUp:=true;
            PantFrontStart:=0;
            end
           else if (CValue1=0) then
            begin
            PantFrontUp:=false;
            PantFrontStart:=1;
            end;
         end
         else
         begin //nie 'ezt' - odwrotne ustawienie pantografów: ^-.-^ zamiast ^-.^-
          (*
           if (CValue1=1) then
            if (TestFlag(Couplers[1].CouplingFlag,ctrain_controll)and(CValue2= 1))
             or(TestFlag(Couplers[0].CouplingFlag,ctrain_controll)and(CValue2=-1))
             then
             begin
              PantFrontUp:=true;
              PantFrontStart:=0;
             end
            else
             begin
              PantRearUp:=true;
              PantRearStart:=0;
             end
           else if (CValue1=0) then
            if (TestFlag(Couplers[1].CouplingFlag,ctrain_controll)and(CValue2= 1))
             or(TestFlag(Couplers[0].CouplingFlag,ctrain_controll)and(CValue2=-1))
            then
            begin
              PantFrontUp:=false;
              PantFrontStart:=1;
             end
            else
             begin
              PantRearUp:=false;
              PantRearStart:=1;
             end;
             *)
         end;
         //--OK:=SendCtrlToNext(command,CValue1,CValue2);
       end
      else if command='PantRear' then         {Winger 160204, ABu 310105 i 030305}
       begin //Ra: uwzglêdniæ trzeba jeszcze zgodnoœæ sprzêgów
         if (TrainType=dt_EZT) then
         begin {'ezt'}
          if (CValue1=1) then
            begin
            PantRearUp:=true;
            PantRearStart:=0;
            end
           else if (CValue1=0) then
            begin
            PantRearUp:=false;
            PantRearStart:=1;
            end;
         end
         else
         begin {nie 'ezt'}
         (*
          if (CValue1=1) then
           {if ostatni polaczony sprz. sterowania}
           if (TestFlag(Couplers[1].CouplingFlag,ctrain_controll)and(CValue2= 1))
            or(TestFlag(Couplers[0].CouplingFlag,ctrain_controll)and(CValue2=-1))
           then
            begin
             PantRearUp:=true;
             PantRearStart:=0;
            end
           else
            begin
             PantFrontUp:=true;
             PantFrontStart:=0;
            end
          else if (CValue1=0) then
           if (TestFlag(Couplers[1].CouplingFlag,ctrain_controll)and(CValue2= 1))
            or(TestFlag(Couplers[0].CouplingFlag,ctrain_controll)and(CValue2=-1))
           then
            begin
             PantRearUp:=false;
             PantRearStart:=1;
            end
           else
            begin
             PantFrontUp:=false;
             PantFrontStart:=1;
            end;
            *)
         end;
         //--OK:=SendCtrlToNext(command,CValue1,CValue2);
       end
      else if command='MaxCurrentSwitch' then
       begin
          //--OK:=MaxCurrentSwitch(CValue1=1);
       end
      else if command='MinCurrentSwitch' then
       begin
          //--OK:=MinCurrentSwitch(CValue1=1);
       end
    {test komend oddzialywujacych na tabor}
      else if command='SetDamage' then
       begin
         if CValue2=1 then OK:=SetFlag(DamageFlag,Trunc(CValue1));
         if CValue2=-1 then OK:=SetFlag(DamageFlag,-Trunc(CValue1));
       end
      else if command='Emergency_brake' then
       begin
        //-- if EmergencyBrakeSwitch(Trunc(CValue1)=1) then                // moe ma w pas TODO:
        //--   OK:=true
       //--  else OK:=false;
       end
      else if command='BrakeDelay' then
       begin
          BrakeDelayFlag:=Trunc(CValue1);
          OK:=true;
       end
      //--else if command='SandDoseOn' then
      //-- begin
      //--   if SandDoseOn then
      //--     OK:=true
      //--   else OK:=false;
      //-- end
      else if command='CabSignal' then {SHP,Indusi}
       begin //Ra: to powinno dzia³aæ tylko w cz³onie obsadzonym
      //--   if {(TrainType=dt_EZT)or} (ActiveCab<>0) and (Battery) and
    TestFlag(SecuritySystem.SystemType,2) then //jeœli kabina jest obsadzona (silnikowy w EZT?)
      //--    with SecuritySystem do
      //--     begin
      //--      VelocityAllowed:=Trunc(CValue1);
      //--      NextVelocityAllowed:=Trunc(CValue2);
      //--      SystemSoundSHPTimer:=0; //hunter-091012
      //--      SetFlag(Status,s_active);
      //--     end;
         //else OK:=false;
        OK:=true; //true, gdy mo¿na usun¹æ komendê
       end
     {naladunek/rozladunek}
      else if Pos('Load=',command)=1 then
       begin
        OK:=false; //bêdzie powtarzane a¿ siê za³aduje
        //-if (Vel=0) and (MaxLoad>0) and (Load<MaxLoad*(1+OverLoadFactor)) then //czy mo¿na
    ³adowac?
        //- if Distance(Loc,CommandIn.Location,Dim,Dim)<10 then //ten peron/rampa
        //-  begin
        //-   testload:=LowerCase(DUE(command));
           //-if Pos(testload,LoadAccepted)>0 then //nazwa jest obecna w CHK
           //- OK:=LoadingDone(Min0R(CValue2,LoadSpeed),testload); //zmienia LoadStatus       //
    przeniesione do mover.cpp
        //-  end;
        //if OK then LoadStatus:=0; //nie udalo sie w ogole albo juz skonczone
       end
      else if Pos('UnLoad=',command)=1 then
       begin
        OK:=false; //bêdzie powtarzane a¿ siê roz³aduje
        //-if (Vel=0) and (Load>0) then //czy jest co rozladowac?
        //- if Distance(Loc,CommandIn.Location,Dim,Dim)<10 then //ten peron
        //-  begin
        //-   testload:=DUE(command); //zgodnoœæ nazwy ³adunku z CHK
           //-if LoadType=testload then {mozna to rozladowac}
           //- OK:=LoadingDone(-Min0R(CValue2,LoadSpeed),testload);                    //
    przeniesione do mover.cpp
        //-  end;
        //if OK then LoadStatus:=0;
       end;
      RunCommand:=OK; //dla true komenda bêdzie usuniêta, dla false wykonana ponownie

      */
    return OK;
}

// *************************************************************************************************
// Q: 20160714
// *************************************************************************************************
bool TMoverParameters::RunInternalCommand(void)
{
    bool OK;

    if (CommandIn.Command != "")
    {
        OK = RunCommand(CommandIn.Command, CommandIn.Value1, CommandIn.Value2);
        if (OK)

        {
            CommandIn.Command = ""; // kasowanie bo rozkaz wykonany
            CommandIn.Value1 = 0;
            CommandIn.Value2 = 0;
            CommandIn.Location.X = 0;
            CommandIn.Location.Y = 0;
            CommandIn.Location.Z = 0;
            if (!PhysicActivation)
                Physic_ReActivation();
        }
    }
    else
        OK = false;
    return OK;
}

// *************************************************************************************************
// Q: 20160714
// *************************************************************************************************
int TMoverParameters::ShowCurrentP(int AmpN)
{
    int b, Bn;
    bool Grupowy;

    // ClearPendingExceptions;
    Grupowy = ((DelayCtrlFlag) && (TrainType == dt_ET22)); // przerzucanie walu grupowego w ET22;
    Bn = RList[MainCtrlActualPos].Bn; // ile równoleg³ych ga³êzi silników

    if ((DynamicBrakeType == dbrake_automatic) && (DynamicBrakeFlag))
        Bn = 2;
    if (Power > 0.01)
    {
        if (AmpN > 0) // podaæ pr¹d w ga³êzi
        {
            if ((Bn < AmpN) || ((Grupowy) && (AmpN == Bn - 1)))
                return 0;
            else // normalne podawanie pradu
                return floor(abs(Im));
        }
        else // podaæ ca³kowity
            return floor(abs(Itot));
    }
    else // pobor pradu jezeli niema mocy
        for (b = 0; b < 1; b++)
            // with Couplers[b] do
            if (TestFlag(Couplers[b].CouplingFlag, ctrain_controll))
                if (Couplers[b].Connected->Power > 0.01)
                    return Couplers[b].Connected->ShowCurrent(AmpN);
}


