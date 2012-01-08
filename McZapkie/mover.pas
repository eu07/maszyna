unit mover;          {fizyka ruchu dla symulatora lokomotywy}

(*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Maciej Czapkiewicz and others

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
(C) McZapkie v.2004.02
Co brakuje:
6. brak pantografow itp  'Nie do konca, juz czesc zrobiona - Winger
7. brak efektu grzania oporow rozruchowych, silnika, osi
9. ulepszyc sprzeg sztywny oraz automatyczny
10. klopoty z EN57
...
n. Inne lokomotywy oprocz elektrowozu pradu stalego
*)
(*
Zrobione:
1. model szeregowego silnika elektrycznego sterowanego reostatem
2. fizyka ruchu - sily oporu, nachylenia, przyspieszenia styczne i normalne,
przyczepnosc/poslizg przy hamowaniu i rozruchu
3. docisk klockow hamulcowych -hamulec reczny oraz pomocniczy hamulec pneumatyczny
4. ubytki cisnienia wskutek hamowania, kompresor - ladowanie zbiornika
5. niektore uszkodzenia i awarie - logika rozmyta
a) silnik elektryczny - za duzy prad, napiecie, obroty
b) sprzegi - zerwanie gdy zbyt duze sily
6. flagi niektorych dzwiekow
7. hamulec zasadniczy - elektropneumatyczny (impulsowanie), pneumatyczny - zmiany cisnienia
zwiekszenie nacisku przy duzych predkosciach w hamulcach Oerlikona
8. sprzegi - sila, tlumiennosc
9. lok. el. - wylacznik glowny, odlacznik uszkodzonych silnikow
10. parametry trakcji
11. opoznienia w zalaczaniu stycznikow
12. trakcja wielokrotna
13. informacja na temat zrodla mocy dla silnikow trakcyjnych, ogrzewania, oswietlenia
14. dumb - uproszczony model lokomotywy (tylko dla AI)
15. ladunki - ilosc, typ, szybkosc na- i rozladunku, funkcje przetestowac!
16. ulepszony model fizyczny silnika elektrycznego (hamowanie silnikiem, ujemne napiecia)
17. poprawione hamulce - parametry cylindrow itp
18. ulepszone hamulce - napelnianie uderzeniowe, stopnie hamowania
19. poprawione hamulce - wyszczegolnenie cisnienia dla stopni hamowania
20. poprawione hamulce elektropneumatyczne
21. poprawione hamowanie przeciwposlizgowe i odluzniacz
22. dodany model mechanicznego napedu recznego (drezyna)
23. poprawiona szybkosc hamowania pomocniczego i przeciwposlizgowego, odlaczanie silnikow, odlaczanie bocznikow
24. wprowadzone systemy zabezpieczenia: SHP, Czuwak, sygnalizacja kabinowa (nie dzialaja w DebugMode).
25. poprawiona predkosc propagacji fali cisnienia (normalizacja przez kwadrat dlugosci) //to jest zdeka liniowe
26. uwzgledniona masa ladunku, przesuniecie w/m toru ruchu
27. lampy/sygnaly przednie/tylne
28. wprowadzona pozycja odciecia hamulca (yB: tja, ale oerlikona....)
29. rozne poprawki: slizganie, wykolejanie itp
30. model silnika spalinowego z przekladnia mechaniczna
31. otwieranie drzwi, sprezarka, przetwornica
32. wylaczanie obliczen dla nieruchomych wagonow
33. Zbudowany model rozdzielacza powietrza roznych systemow
34. Poprawiona pozycja napelniania uderzeniowego i hamulec EP
...
*)


interface uses mctools,sysutils;

CONST
   Go=True;
   Hold=False;                 {dla CheckLocomotiveParameters}
   ResArraySize=64;            {dla silnikow elektrycznych}
   MotorParametersArraySize=8;
   maxcc=4;                    {max. ilosc odbierakow pradu}
   LocalBrakePosNo=10;         {ilosc nastaw hamulca recznego lub pomocniczego}
   MainBrakeMaxPos=10;          {max. ilosc nastaw hamulca zasadniczego}

   {uszkodzenia toru}
   dtrack_railwear=2;
   dtrack_freerail=4;
   dtrack_thinrail=8;
   dtrack_railbend=16;
   dtrack_plants=32;
   dtrack_nomove=64;
   dtrack_norail=128;

   {uszkodzenia taboru}
   dtrain_thinwheel=1;     {dla lokomotyw}
   dtrain_loadshift=1;     {dla wagonow}
   dtrain_wheelwear=2;
   dtrain_bearing=4;
   dtrain_coupling=8;
   dtrain_ventilator=16;   {dla lokomotywy el.}
   dtrain_loaddamage=16;   {dla wagonow}
   dtrain_engine=32;       {dla lokomotyw}
   dtrain_loaddestroyed=32;{dla wagonow}
   dtrain_axle=64;
   dtrain_out=128;         {wykolejenie}

   {wagi prawdopodobienstwa dla funkcji FuzzyLogic}
   p_elengproblem = 0.01;
   p_elengdamage = 0.1;
   p_coupldmg = 0.02;
   p_derail=0.001;
   p_accn=0.1;
   p_slippdmg=0.001;

   {typ sprzegu}
   ctrain_virtual=0;        //gdy pojazdy na tym samym torze siê widz¹ wzajemnie
   ctrain_coupler=1;        //sprzeg fizyczny
   ctrain_pneumatic=2;      //przewody hamulcowe
   ctrain_controll=4;       //przewody steruj¹ce (ukrotnienie)
   ctrain_power=8;          //przewody zasilaj¹ce (WN)
   ctrain_passenger=16;     //mostek przejœciowy
   ctrain_scndpneumatic=32; //przewody 8 atm (¿ó³te; zasilanie powietrzem)
   ctrain_heating=64;       //ogrzewanie (elektryczne?)

   {typ hamulca elektrodynamicznego}
   dbrake_none=0;
   dbrake_passive=1;
   dbrake_switch=2;
   dbrake_reversal=4;
   dbrake_automatic=8;

   bdelay_P=0;
   bdelay_G=1;
   bdelay_R=2;
   bdelay_E=4;

   {status hamulca}
   b_off=0;
   b_on=1;
   b_dmg=2;
   b_release=4;
   b_antislip=8;
   b_epused=16;
   b_Rused=32;
   b_Ractive=64;

   {typ hamulca kolejowego}
   bp_classic=0;
   bp_diameter=1;
   bp_magnetic=2;

   {status czuwaka/SHP}
   s_waiting=1; //dzia³a
   s_aware=2;   //czuwak miga
   s_active=4;  //SHP œwieci
   s_alarm=8;   //buczy
   s_ebrake=16; //hamuje

   {dzwieki}
   sound_none=0;
   sound_loud=1;
   sound_couplerstretch=2;
   sound_bufferclamp=4;
   sound_bufferbump=8;
   sound_relay=16;
   sound_manyrelay=32;
   sound_brakeacc=64;

   Spg=0.5067;

   PhysicActivationFlag: boolean=False;

   //szczególne typy pojazdów (inna obs³uga) dla zmiennej TrainType
   //zamienione na flagi bitowe, aby szybko wybieraæ grupê (np. EZT+SZT)
   dt_Default=0;
   dt_EZT=1;
   dt_ET41=2;
   dt_ET42=4;
   dt_PseudoDiesel=8;
   dt_ET22=$10; //nie u¿ywane w warunkach, ale ustawiane z CHK
   dt_SN61=$20; //nie u¿ywane w warunkach, ale ustawiane z CHK
   dt_181=$40;

TYPE
    PMoverParameters=^TMoverParameters;

        {ogolne}
    TLocation = record
                  X,Y,Z: real;    {lokacja}
                end;
    TRotation = record
                  Rx,Ry,Rz: real; {rotacja}
                end;
    TDimension= record
                  W,L,H: real;    {wymiary}
                end;

    TCommand = record
                 Command: string;         {komenda}
                 Value1,Value2: real;     {argumenty komendy}
                 Location: TLocation;
               end;

    {tory}
    TTrackShape= record      {ksztalt odcinka - promien, dlugosc, nachylenie, przechylka}
                  R, Len, dHtrack, dHrail: real;
                end;

    TTrackParam= record     {parametry odcinka - szerokosc, tarcie statyczne, kategoria,  obciazalnosc w t/os, uszkodzenia}
                  Width, friction: real;
                  CategoryFlag, QualityFlag, DamageFlag: byte;
                  Velmax:real; {dla uzytku maszynisty w ai_driver}
                end;
    TTractionParam= record
                      TractionVoltage: real; {napiecie}
                      TractionFreq: real;    {czestotliwosc}
                      TractionMaxCurrent: real; {obciazalnosc}
                      TractionResistivity: real; {rezystancja styku}
                    end;
    {powyzsze parametry zwiazane sa z torem po ktorym aktualnie pojazd jedzie}

    {sprzegi}
    TCouplerType=(NoCoupler,Articulated,Bare,Chain,Screw,Automatic);
    TCoupling = record
                  {parametry}
                  SpringKB,SpringKC,beta:real;   {stala sprezystosci zderzaka/sprzegu, %tlumiennosci }
                  DmaxB,FmaxB,DmaxC,FmaxC: real; {tolerancja scisku/rozciagania, sila rozerwania}
                  CouplerType: TCouplerType;     {typ sprzegu}
                  {zmienne}
                  CouplingFlag : byte; {0 - wirtualnie, 1 - sprzegi, 2 - pneumatycznie, 4 - sterowanie, 8 - kabel mocy}
                  AllowedFlag : byte;          //Ra: znaczenie jak wy¿ej, maska dostêpnych
                  Render: boolean;             {ABu: czy rysowac jak zaczepiony sprzeg}
                  CoupleDist: real;            {ABu: optymalizacja - liczenie odleglosci raz na klatkê, bez iteracji}
                  Connected: PMoverParameters; {co jest podlaczone}
                  ConnectedNr: byte;           //Ra: od której strony pod³¹czony do (Connected): 0=przód, 1=ty³
                  CForce: real;                {sila z jaka dzialal}
                  Dist: real;                  {strzalka ugiecia zderzaków}
                  CheckCollision: boolean;     {czy sprawdzac sile czy pedy}
                end;

    TCouplers= array[0..1] of TCoupling;
    //TCouplerNr= array[0..1] of byte; //ABu: nr sprzegu z ktorym polaczony; Ra: wrzuciæ do TCoupling

    {typy hamulcow zespolonych}
    TBrakeSystem = (Individual, Pneumatic, ElectroPneumatic);
    {podtypy hamulcow zespolonych}
    TBrakeSubsystem = (Standard, WeLu, Knorr, KE, Hik, Kk, Oerlikon);
    {typy hamulcow indywidualnych}
    TLocalBrake = (NoBrake, ManualBrake, PneumaticBrake, HydraulicBrake);

    TBrakeDelayTable= array[1..4] of real;
    {dla osob/towar: opoznienie hamowania/odhamowania}

    TBrakePressure= record
                      PipePressureVal: real;
                      BrakePressureVal: real;
                      FlowSpeedVal: real;
                      BrakeType: TBrakeSystem;
                    end;
    TBrakePressureTable= array[-2..MainBrakeMaxPos] of TBrakePressure;

    {typy napedow}                                                       {EZT tymczasowe ¿eby kibel dzialal}
    TEngineTypes = (None, Dumb, WheelsDriven, ElectricSeriesMotor, DieselEngine, SteamEngine, DieselElectric); { EZT); }
    {postac dostarczanej energii}
    TPowerType = (NoPower, BioPower, MechPower, ElectricPower, SteamPower);
    {rodzaj paliwa}
    TFuelType = (Undefined, Coal, Oil);
    {rodzaj rusztu}
    TGrateType = record
                   FuelType: TFuelType;
                   GrateSurface: real;
                   FuelTransportSpeed: real;
                   IgnitionTemperature: real;
                   MaxTemperature: real;
                 end;
    {rodzaj kotla}
    TBoilerType = record
                    BoilerVolume: real;
                    BoilerHeatSurface: real;
                    SuperHeaterSurface: real;
                    MaxWaterVolume,MinWaterVolume: real;
                    MaxPressure: real;
                  end;
    {rodzaj odbieraka pradu}
    TCurrentCollector = record
                          MinH,MaxH: real;
                          CSW: real;      {szerokosc slizgacza}
                        end;
    {typy zrodel mocy}
    TPowerSource = (NotDefined, InternalSource, Transducer, Generator, Accumulator, CurrentCollector, PowerCable, Heater);
    {parametry zrodel mocy}
    TPowerParameters = record
                         MaxVoltage: real;
                         MaxCurrent: real;
                         IntR: real;
                         case SourceType: TPowerSource of
                          NotDefined,InternalSource : (PowerType: TPowerType);
                          Transducer : (InputVoltage: real);
                          Generator  : (GeneratorEngine: TEngineTypes);
                          Accumulator: (MaxCapacity: real; RechargeSource:TPowerSource);
                          CurrentCollector: (CollectorsNo: byte; CollectorParameters: TCurrentCollector);
                          PowerCable : (PowerTrans:TPowerType; SteamPressure: real);
                          Heater : (Grate: TGrateType; Boiler: TBoilerType);
                       end;

    {dla lokomotyw elektrycznych:}
    TScheme= record
               Relay: byte; {numer pozycji rozruchu samoczynnego}
               R: real;    {opornik rozruchowy}                   {dla dizla napelnienie}
               Bn,Mn:byte; {ilosc galezi i silnikow w galezi}     {dla dizla Mn: czy luz czy nie}
               AutoSwitch: boolean; {czy dana pozycja nastawniana jest recznie czy autom.}
               ScndAct: byte; {jesli ma bocznik w nastawniku, to ktory bocznik na ktorej pozycji}
             end;
    TSchemeTable = array[0..ResArraySize] of TScheme; {tablica rezystorow rozr.}
    TDEScheme= record
               RPM:   real;  {obroty diesla}
               GenPower: real;  {moc maksymalna}
               Umax:  real;  {napiecie maksymalne}
               Imax:  real;  {prad maksymalny}
             end;
    TDESchemeTable = array[0..32] of TDEScheme; {tablica rezystorow rozr.}
    TShuntScheme = record
                Umin: real;
                Umax: real;
                Pmin: real;
                Pmax: real;
                end;
    TShuntSchemeTable = array[0..32] of TShuntScheme;            
    TMPTRelay = record {lista przekaznikow bocznikowania}
                Iup: real;
                Idown: real;
                end;
    TMPTRelayTable = array[0..7] of TMPTRelay;

    TMotorParameters = record
                        mfi,mIsat:real; { aproksymacja M(I) silnika} {dla dizla mIsat=przekladnia biegu}
                        fi,Isat: real;  { aproksymacja E(n)=fi*n}    {dla dizla fi, mfi: predkosci przelozenia biegu <-> }
                        AutoSwitch: boolean;
                       end;

    TSecuritySystem= record
                       SystemType: byte; {0: brak, 1: czuwak aktywny, 2: SHP/sygnalizacja kabinowa}
                       AwareDelay,SoundSignalDelay,EmergencyBrakeDelay:real;
                       Status: byte;     {0: wylaczony, 1: wlaczony, 2: czuwak, 4: shp, 8: alarm, 16: hamowanie awaryjne}
                       SystemTimer, SystemSoundTimer, SystemBrakeTimer: real;
                       VelocityAllowed, NextVelocityAllowed: integer; {predkosc pokazywana przez sygnalizacje kabinowa}
                       RadioStop:boolean; //czy jest RadioStop
                     end;

    {----------------------------------------------}
    {glowny obiekt}
    TMoverParameters = class(TObject)
               dMoveLen:real;
               filename:string;
               {---opis lokomotywy, wagonu itp}
                    {--opis serii--}
               CategoryFlag: byte;       {1 - pociag, 2 - samochod, 4 - statek, 8 - samolot}
                    {--sekcja stalych typowych parametrow}
               TypeName: string;         {nazwa serii/typu}
               //TrainType: string;       {typ: EZT/elektrowoz - Winger 040304}
               TrainType: integer; {Ra: powinno byæ szybciej ni¿ string}
               EngineType: TEngineTypes;               {typ napedu}
               EnginePowerSource: TPowerParameters;    {zrodlo mocy dla silnikow}
               SystemPowerSource: TPowerParameters;    {zrodlo mocy dla systemow sterowania/przetwornic/sprezarek}
               HeatingPowerSource: TPowerParameters;   {zrodlo mocy dla ogrzewania}
               AlterHeatPowerSource: TPowerParameters; {alternatywne zrodlo mocy dla ogrzewania}
               LightPowerSource: TPowerParameters;     {zrodlo mocy dla oswietlenia}
               AlterLightPowerSource: TPowerParameters;{alternatywne mocy dla oswietlenia}
               Vmax,Mass,Power: real;    {max. predkosc kontrukcyjna, masa wlasna, moc}
               Mred: real;    {Ra: zredukowane masy wiruj¹ce; potrzebne do obliczeñ hamowania}
               TotalMass:real; {wyliczane przez ComputeMass}
               HeatingPower, LightPower: real; {moc pobierana na ogrzewanie/oswietlenie}
               BatteryVoltage: integer;        {Winger - baterie w elektrykach}
               Dim: TDimension;          {wymiary}
               Cx: real;                 {wsp. op. aerodyn.}
               WheelDiameter : real;     {srednica kol napednych}
               WheelDiameterL : real;    //Ra: srednica kol tocznych przednich
               WheelDiameterT : real;    //Ra: srednica kol tocznych tylnych
               TrackW: real;             {nominalna szerokosc toru [m]}
               AxleInertialMoment: real; {moment bezwladnosci zestawu kolowego}
               AxleArangement : string;  {uklad osi np. Bo'Bo' albo 1'C}
               NPoweredAxles : byte;     {ilosc osi napednych liczona z powyzszego}
               NAxles : byte;            {ilosc wszystkich osi j.w.}
               BearingType: byte;        {lozyska: 0 - slizgowe, 1 - toczne}
               ADist, BDist: real;       {odlegosc osi oraz czopow skretu}
               {hamulce:}
               NBpA : byte;              {ilosc el. ciernych na os: 0 1 2 lub 4}
               SandCapacity: integer;    {zasobnik piasku [kg]}
               BrakeSystem: TBrakeSystem;{rodzaj hamulca zespolonego}
               BrakeSubsystem: TBrakeSubsystem;
               MBPM: real; {masa najwiekszego cisnienia}
//               BrakeVariety: byte;

               LocalBrake: TLocalBrake;  {rodzaj hamulca indywidualnego}
               BrakePressureTable: TBrakePressureTable; {wyszczegolnienie cisnien w rurze}
               ASBType: byte;            {0: brak hamulca przeciwposlizgowego, 1: reczny, 2: automat}
               TurboTest: byte;
               MaxBrakeForce: real;      {maksymalna sila nacisku hamulca}
               MaxBrakePress,P2FTrans: real;
               TrackBrakeForce: real;    {sila nacisku hamulca szynowego}
               BrakeMethod: byte;        {flaga rodzaju hamulca}
               {max. cisnienie w cyl. ham., stala proporcjonalnosci p-K}
               HighPipePress,LowPipePress,DeltaPipePress: real;
               {max. i min. robocze cisnienie w przewodzie glownym oraz roznica miedzy nimi}
               CntrlPipePress: real;
               {cisnienie z zbiorniku sterujacym}
               BrakeVolume, BrakeVVolume, VeselVolume: real;
               {pojemnosc powietrza w ukladzie hamulcowym, w ukladzie glownej sprezarki [m^3] }
               BrakeCylNo: integer; {ilosc cylindrow ham.}
               BrakeCylRadius, BrakeCylDist: real;
               BrakeCylMult: array[0..3] of real;
               BCMFlag: byte;
               {promien cylindra, skok cylindra, przekladnia hamulcowa}
               MinCompressor,MaxCompressor,CompressorSpeed:real;
               {cisnienie wlaczania, zalaczania sprezarki, wydajnosc sprezarki}
               BrakeDelay: TBrakeDelayTable; {opoznienie hamowania/odhamowania t/o}
               BrakeCtrlPosNo:byte;     {ilosc pozycji hamulca}
               {nastawniki:}
               MainCtrlPosNo: byte;     {ilosc pozycji nastawnika}
               ScndCtrlPosNo: byte;
               ScndInMain: boolean;     {zaleznosc bocznika od nastawnika}
               SecuritySystem: TSecuritySystem;

                       {-sekcja parametrow dla lokomotywy elektrycznej}
               RList: TSchemeTable;     {lista rezystorow rozruchowych i polaczen silnikow, dla dizla: napelnienia}
               RlistSize: integer;
               MotorParam: array[0..MotorParametersArraySize] of TMotorParameters;
                                        {rozne parametry silnika przy bocznikowaniach}
                                        {dla lokomotywy spalinowej z przekladnia mechaniczna: przelozenia biegow}
               Transmision : record   {liczba zebow przekladni}
                               NToothM, NToothW : byte;
                               Ratio: real;  {NToothW/NToothM}
                             end;
               NominalVoltage: real;   {nominalne napiecie silnika}
               WindingRes : real;
               CircuitRes : real;      {rezystancje silnika i obwodu}
               IminLo,IminHi: integer; {prady przelacznika automatycznego rozruchu, uzywane tez przez ai_driver}
               ImaxLo,ImaxHi: integer; {maksymalny prad niskiego i wysokiego rozruchu}
               nmax: real;             {maksymalna dop. ilosc obrotow /s}
               InitialCtrlDelay,       {ile sek. opoznienia po wl. silnika}
               CtrlDelay: real;        { -//-  -//- miedzy kolejnymi poz.}
               AutoRelayType: byte;    {0 -brak, 1 - jest, 2 - opcja}
               CoupledCtrl: boolean;   {czy mainctrl i scndctrl sa sprzezone}
               //CouplerNr: TCouplerNr;  {ABu: nr sprzegu podlaczonego w drugim obiekcie}
               IsCoupled: boolean;     {czy jest sprzezony ale jedzie z tylu}
               DynamicBrakeType: byte; {patrz dbrake_*}
               RVentType: byte;        {0 - brak, 1 - jest, 2 - automatycznie wlaczany}
               RVentnmax: real;      {maks. obroty wentylatorow oporow rozruchowych}
               RVentCutOff: real;      {rezystancja wylaczania wentylatorow dla RVentType=2}
               CompressorPower: integer; {0: bezp. z obwodow silnika, 1: z przetwornicy, reczne, 2: w przetwornicy, stale}
               SmallCompressorPower: integer; {Winger ZROBIC}

                      {-sekcja parametrow dla lokomotywy spalinowej z przekladnia mechaniczna}
                dizel_Mmax, dizel_nMmax, dizel_Mnmax, dizel_nmax, dizel_nominalfill: real;
                 {parametry aproksymacji silnika spalinowego}
                dizel_Mstand: real; {moment oporow ruchu silnika dla enrot=0}
{                dizel_auto_min, dizel_auto_max: real; {predkosc obrotowa przelaczania automatycznej skrzyni biegow}
                dizel_nmax_cutoff: real; {predkosc obrotowa zadzialania ogranicznika predkosci}
                dizel_nmin: real; {najmniejsza dopuszczalna predkosc obrotowa}
                dizel_minVelfullengage: real; {najmniejsza predkosc przy jezdzie ze sprzeglem bez poslizgu}
                dizel_AIM: real; {moment bezwladnosci walu itp}
                dizel_engageDia, dizel_engageMaxForce, dizel_engagefriction: real; {parametry sprzegla}

                {- dla lokomotyw spalinowo-elektrycznych -}
                AnPos: real; // pozycja sterowania dokladnego (analogowego)
                AnalogCtrl: boolean; //
                AnMainCtrl: boolean; //
                ShuntModeAllow: boolean;
                ShuntMode: boolean;

                Flat: boolean;
                Vhyp: real;
                DElist: TDESchemeTable;
                Vadd: real;
                MPTRelay: TMPTRelayTable;
                RelayType: byte;
                SST: TShuntSchemeTable;
                PowerCorRatio: real; //Wspolczynnik korekcyjny


                {- dla uproszczonego modelu silnika (dumb) oraz dla drezyny}
                Ftmax:real;

                {-dla wagonow}
                MaxLoad: longint;           {masa w T lub ilosc w sztukach - ladownosc}
                LoadAccepted, LoadQuantity: string; {co moze byc zaladowane, jednostki miary}
                OverLoadFactor: real;       {ile razy moze byc przekroczona ladownosc}
                LoadSpeed,UnLoadSpeed: real;{szybkosc na- i rozladunku jednostki/s}
                DoorOpenCtrl,DoorCloseCtrl: byte; {0: przez pasazera, 1: przez maszyniste, 2: samoczynne (zamykanie)}
                DoorStayOpen: real;               {jak dlugo otwarte w przypadku DoorCloseCtrl=2}
                DoorClosureWarning: boolean;      {czy jest ostrzeganie przed zamknieciem}
                DoorOpenSpeed, DoorCloseSpeed: real;      {predkosc otwierania i zamykania w j.u. }
                DoorMaxShiftL,DoorMaxShiftR: real;{szerokosc otwarcia lub kat}
                DoorOpenMethod: byte;             {sposob otwarcia - 1: przesuwne, 2: obrotowe}

                        {--sekcja zmiennych}
                        {--opis konkretnego egzemplarza taboru}
                Loc: TLocation;
                Rot: TRotation;
                Name: string;                       {nazwa wlasna}
                Couplers: TCouplers;                {urzadzenia zderzno-sprzegowe, polaczenia miedzy wagonami}
                ScanCounter: integer;   {pomocnicze do skanowania sprzegow}
                EventFlag: boolean;                 {!o True jesli cos nietypowego sie wydarzy}
                SoundFlag: byte;                    {!o patrz stale sound_ }
                DistCounter: real;                  {! licznik kilometrow }
                V, Vel, AccS, AccN, AccV: real;
                {! predkosc w m/s i jej modul w km/h, przyspieszenie styczne i normalne w m/s^2}
                nrot: real;
                {! rotacja kol [obr/s]}
                EnginePower: real;                  {! chwilowa moc silnikow}
                dL, Fb, Ff : real;                  {przesuniecie, sila hamowania i tarcia}
                FTrain,FStand: real;                {! sila pociagowa i oporow ruchu}
                FTotal: real;                       {! calkowita sila dzialajaca na pojazd}
                UnitBrakeForce: real;               {!s si³a hamowania przypadaj¹ca na jeden element}
                SlippingWheels,SandDose: boolean;   {! poslizg kol, sypanie piasku}
                Sand: real;                         {ilosc piasku}
                BrakeSlippingTimer:real;            {pomocnicza zmienna do wylaczania przeciwposlizgu}
                dpBrake,dpPipe,dpMainValve,dpLocalValve: real;
                {! przyrosty cisnienia w kroku czasowym}
                ScndPipePress: real;                {cisnienie w przewodzie zasilajacym}
                BrakePress:real;                    {!o cisnienie w cylindrach hamulcowych}
                LocBrakePress:real;                 {!o cisnienie w cylindrach hamulcowych z pomocniczego}
                PipeBrakePress:real;                {!o cisnienie w cylindrach hamulcowych z przewodu}
                PipePress: real;                    {!o cisnienie w przewodzie glownym}
                PPP: real;                          {!o poprzednie cisnienie w przewodzie glownym}
                Volume: real;                       {objetosc spr. powietrza w zbiorniku hamulca}
                CompressedVolume:real;              {objetosc spr. powietrza w ukl. zasilania}
                PantVolume:real;                    {objetosc spr. powietrza w ukl. pantografu}
                Compressor: real;                   {! cisnienie w ukladzie zasilajacym}
                CompressorFlag: boolean;             {!o czy wlaczona sprezarka}
                PantCompFlag: boolean;             {!o czy wlaczona sprezarka pantografow}
                CompressorAllow: boolean;            {! zezwolenie na uruchomienie sprezarki  NBMX}
                ConverterFlag: boolean;              {!  czy wlaczona przetwornica NBMX}
                ConverterAllow: boolean;             {zezwolenie na prace przetwornicy NBMX}
                BrakeCtrlPos:integer;               {nastawa hamulca zespolonego}
                LocalBrakePos:byte;                 {nastawa hamulca indywidualnego}
                BrakeStatus: byte; {0 - odham, 1 - ham., 2 - uszk., 4 - odluzniacz, 8 - antyposlizg, 16 - uzyte EP, 32 - pozycja R, 64 - powrot z R}
                EmergencyBrakeFlag: boolean;        {hamowanie nagle}
                BrakeDelayFlag: byte;               {nastawa opoznienia ham. osob/towar/posp/exp 0/1/2/4}
                BrakeDelays: byte;                   {nastawy mozliwe do uzyskania}
                DynamicBrakeFlag: boolean;          {czy wlaczony hamulec elektrodymiczny}
//                NapUdWsp: integer;
                LimPipePress: real;                 {stabilizator cisnienia}
                ActFlowSpeed: real;                 {szybkosc stabilizatora}


                DamageFlag: byte;  //kombinacja bitowa stalych dtrain_* }
                DerailReason: byte; //przyczyna wykolejenia

                //EndSignalsFlag: byte;  {ABu 060205: zmiany - koncowki: 1/16 - swiatla prz/tyl, 2/31 - blachy prz/tyl}
                //HeadSignalsFlag: byte; {ABu 060205: zmiany - swiatla: 1/2/4 - przod, 16/32/63 - tyl}
                CommandIn: TCommand;
                {komenda przekazywana przez PutCommand}
                {i wykonywana przez RunInternalCommand}
                CommandOut: string;        {komenda przekazywana przez ExternalCommand}
                CommandLast: string; //Ra: ostatnio wykonana komenda do podgl¹du
                ValueOut: real;            {argument komendy która ma byc przekazana na zewnatrz}

                RunningShape:TTrackShape;{geometria toru po ktorym jedzie pojazd}
                RunningTrack:TTrackParam;{parametry toru po ktorym jedzie pojazd}
                OffsetTrackH, OffsetTrackV: real;  {przesuniecie poz. i pion. w/m toru}

                {-zmienne dla lokomotyw}
                Mains: boolean;    {polozenie glownego wylacznika}
                MainCtrlPos: byte; {polozenie glownego nastawnika}
                ScndCtrlPos: byte; {polozenie dodatkowego nastawnika}
                ActiveDir: integer; //czy lok. jest wlaczona i w ktorym kierunku:
                //wzglêdem wybranej kabiny: -1 - do tylu, +1 - do przodu, 0 - wylaczona
                CabNo: integer;    {! numer kabiny: 1 lub -1. W przeciwnym razie brak sterowania - rozrzad}
                DirAbsolute: integer; //zadany kierunek jazdy wzglêdem sprzêgów (1=w strone 0,-1=w stronê 1) 
                ActiveCab: integer; {! numer kabiny, w ktorej sie jest}
                LastCab: integer;       { numer kabiny przed zmiana }
                LastSwitchingTime: real; {czas ostatniego przelaczania czegos}
                WarningSignal: byte;     {0: nie trabi, 1,2: trabi}
                DepartureSignal: boolean; {sygnal odjazdu}
                InsideConsist: boolean;
                {-zmienne dla lokomotywy elektrycznej}
                RunningTraction:TTractionParam;{parametry sieci trakcyjnej najblizej lokomotywy}
                enrot, Im, Itot, Mm, Mw, Fw, Ft: real;
                {ilosc obrotow, prad silnika i calkowity, momenty, sily napedne}
                Imin,Imax: integer;      {prad przelaczania automatycznego rozruchu, prad bezpiecznika}
                Voltage: real;           {aktualne napiecie sieci zasilajacej}
                MainCtrlActualPos: byte; {wskaznik Rlist}
                ScndCtrlActualPos: byte; {wskaznik MotorParam}
                DelayCtrlFlag: boolean;  {opoznienie w zalaczaniu}
                LastRelayTime: real;     {czas ostatniego przelaczania stycznikow}
                AutoRelayFlag: boolean;  {mozna zmieniac jesli AutoRelayType=2}
                FuseFlag: boolean;       {!o bezpiecznik nadmiarowy}
                ConvOvldFlag: boolean;              {!  nadmiarowy przetwornicy i ogrzewania}
                StLinFlag: boolean;       {!o styczniki liniowe}
                ResistorsFlag: boolean;  {!o jazda rezystorowa}
                RventRot: real;          {!s obroty wentylatorow rozruchowych}
                UnBrake: boolean;       {w EZT - nacisniete odhamowywanie}

                {-zmienne dla lokomotywy spalinowej z przekladnia mechaniczna}
                dizel_fill: real; {napelnienie}
                dizel_engagestate: real; {sprzeglo skrzyni biegow: 0 - luz, 1 - wlaczone, 0.5 - wlaczone 50% (z poslizgiem)}
                dizel_engage: real; {sprzeglo skrzyni biegow: aktualny docisk}
                dizel_automaticgearstatus: real; {0 - bez zmiany, -1 zmiana na nizszy +1 zmiana na wyzszy}
                dizel_enginestart: boolean;      {czy trwa rozruch silnika}
                dizel_engagedeltaomega: real;    {roznica predkosci katowych tarcz sprzegla}

                {-zmienne dla drezyny}
                PulseForce :real;        {przylozona sila}
                PulseForceTimer: real;
                PulseForceCount: integer;

                {dla drezyny, silnika spalinowego i parowego}
                eAngle: real;

                {-dla wagonow}
                Load: longint;      {masa w T lub ilosc w sztukach - zaladowane}
                LoadType: string;   {co jest zaladowane}
                LoadStatus: byte; //+1=trwa rozladunek,+2=trwa zaladunek,+4=zakoñczono,0=zaktualizowany model
                LastLoadChangeTime: real; //raz (roz)³adowania

                DoorLeftOpened: boolean;  //stan drzwi
                DoorRightOpened: boolean;
                PantFrontUp: boolean;  //stan patykow 'Winger 160204
                PantRearUp: boolean;
                PantFrontSP: boolean;  //dzwiek patykow 'Winger 010304
                PantRearSP: boolean;
                PantFrontStart: integer;  //stan patykow 'Winger 160204
                PantRearStart: integer;
                PantFrontVolt: boolean;   //pantograf pod napieciem? 'Winger 160404
                PantRearVolt: boolean;
                PantSwitchType: string;
                ConvSwitchType: string;

                Heating: boolean; //ogrzewanie 'Winger 020304
                DoubleTr: integer; //trakcja ukrotniona - przedni pojazd 'Winger 160304

                PhysicActivation: boolean;

                {ABu: stale dla wyznaczania sil (i nie tylko) po optymalizacji}
                   FrictConst1:  real;
                   FrictConst2s: real;
                   FrictConst2d: real;
                   TotalMassxg     : real; {TotalMass*g}


                {--funkcje}


                function GetTrainsetVoltage: boolean;
                function Physic_ReActivation: boolean;
                procedure PantCheck; //pomocnicza, do sprawdzania pantografow

{                function BrakeRatio: real;  }
                function LocalBrakeRatio: real;
                function PipeRatio: real; {ile napelniac}
                function RealPipeRatio: real; {jak szybko}
                function BrakeVP: real;                
                function DynamicBrakeSwitch(Switch:boolean):boolean;

                {! przesylanie komend sterujacych}
                function SendCtrlBroadcast(CtrlCommand:string; ctrlvalue:real):boolean;
                function SendCtrlToNext(CtrlCommand:string;ctrlvalue,dir:real):boolean;
                function CabActivisation:boolean;
                function CabDeactivisation:boolean;

               {! funkcje zwiekszajace/zmniejszajace nastawniki}
               {! glowny nastawnik:}
                function IncMainCtrl(CtrlSpeed:integer): boolean;
                function DecMainCtrl(CtrlSpeed:integer): boolean;
               {! pomocniczy nastawnik:}
                function IncScndCtrl(CtrlSpeed:integer): boolean;
                function DecScndCtrl(CtrlSpeed:integer): boolean;

                function AddPulseForce(Multipler:integer): boolean; {dla drezyny}

                {wlacza/wylacza sypanie piasku}
                function SandDoseOn : boolean;

               {! zbijanie czuwaka/SHP}
                function SecuritySystemReset:boolean;
                {test czuwaka/SHP}
                procedure SecuritySystemCheck(dt:real);

               {! stopnie hamowania - hamulec zasadniczy}
                function IncBrakeLevel:boolean;
                function DecBrakeLevel:boolean;
               {! stopnie hamowania - hamulec pomocniczy}
                function IncLocalBrakeLevel(CtrlSpeed:byte):boolean;
                function DecLocalBrakeLevel(CtrlSpeed:byte):boolean;
               {! ABu 010205: - skrajne polozenia ham. pomocniczego}
                function IncLocalBrakeLevelFAST:boolean;
                function DecLocalBrakeLevelFAST:boolean;

               {! hamulec bezpieczenstwa}
                function EmergencyBrakeSwitch(Switch:boolean): boolean;
               {hamulec przeciwposlizgowy}
                function AntiSlippingBrake: boolean;
               {odluzniacz}
                function BrakeReleaser: boolean;
                function SwitchEPBrake(state: byte):boolean;

               {! reczny wlacznik urzadzen antyposlizgowych}
                function AntiSlippingButton: boolean;

               {funkcje dla ukladow pneumatycznych}
                function IncBrakePress(var brake:real;PressLimit,dp:real):boolean;
                function DecBrakePress(var brake:real;PressLimit,dp:real):boolean;
                function BrakeDelaySwitch(BDS:byte): boolean; {! przelaczanie nastawy opoznienia}
                function IncBrakeMult(): boolean; {przelaczanie prozny/ladowny}
                function DecBrakeMult(): boolean;
                {pomocnicze funkcje dla ukladow pneumatycznych}
                procedure UpdateBrakePressure(dt: real);
                procedure UpdatePipePressure(dt:real);
                procedure CompressorCheck(dt:real); {wlacza, wylacza kompresor, laduje zbiornik}
                procedure UpdatePantVolume(dt:real); {jw ale uklad zasilania pantografow}
                procedure UpdateScndPipePressure(dt:real);

               {! funkcje laczace/rozlaczajace sprzegi}
               function Attach(ConnectNo: byte; ConnectToNr: byte; ConnectTo:PMoverParameters; CouplingType: byte):boolean; {laczenie}
               function DettachDistance(ConnectNo: byte):boolean; //odleg³oœæ roz³¹czania
               function Dettach(ConnectNo: byte):boolean;                    {rozlaczanie}

               {funkcje obliczajace sily}
                procedure ComputeConstans; //ABu: wczesniejsze wyznaczenie stalych dla liczenia sil
                procedure SetCoupleDist;   //ABu: wyznaczenie odleglosci miedzy wagonami
                function ComputeMass:real;
                function Adhesive(staticfriction:real): real;
                function TractionForce(dt:real):real;
                function FrictionForce(R:real;TDamage:byte):real;
                function BrakeForce(Track:TTrackParam):real;
                function CouplerForce(CouplerN:byte; dt:real):real;
                procedure CollisionDetect(CouplerN:byte; dt:real);

                {obrot kol uwzgledniajacy poslizg}
                function ComputeRotatingWheel(WForce,dt,n:real): real;

                {komendy}
                function SetInternalCommand(NewCommand:string; NewValue1,NewValue2:real):boolean;
               {! do wpisywania komend przez zewnetrzne zdarzenia}
                function GetExternalCommand(var Command:string):real;
                function RunCommand(command:string; CValue1,CValue2:real):boolean;
                function RunInternalCommand:boolean;
                procedure PutCommand(NewCommand:string; NewValue1,NewValue2:real; NewLocation:TLocation);

                {--funkcje dla lokomotyw}
               {! kierunek ruchu}
                function DirectionForward: boolean;
                function DirectionBackward: boolean;
               {! wylacznik glowny}
                function MainSwitch(State:boolean): boolean;
//                procedure SwitchMainKey;
               {! zmiana kabiny i resetowanie ustawien}
                function ChangeCab(direction:integer): boolean;

               {! wl/wyl przetwornicy}
                function ConverterSwitch(State:boolean):boolean;
                {! wl/wyl sprezarki}
                function CompressorSwitch(State:boolean):boolean;
{                function SmallCompressorSwitch(State:boolean):boolean;}
                procedure ConverterCheck;

                {-funkcje typowe dla lokomotywy elektrycznej}
                {! wlaczanie bezpiecznika nadmiarowego}
                function FuseOn: boolean;
                function FuseFlagCheck: boolean; {sprawdzanei flagi nadmiarowego}
                procedure FuseOff; {wywalanie bezpiecznika nadmiarowego}
                {!o pokazuje bezwgl. wartosc pradu na wybranym amperomierzu}
                function ShowCurrent(AmpN:byte): integer;
                {!o pokazuje bezwgl. wartosc obrotow na obrotomierzu jednego z 3 pojazdow}
                function ShowEngineRotation(VehN:byte): integer;
                {funkcje uzalezniajace sile pociagowa od predkosci: v2n, n2r, current, momentum}
                function v2n:real;
                function current(n,U:real): real;
                function Momentum(I:real): real;
               {! odlaczanie uszkodzonych silnikow}
                function CutOffEngine: boolean;
               {! przelacznik pradu wysokiego rozruchu}
                function MaxCurrentSwitch(State:boolean):boolean;
                {sprawdzanie kontrolki rezystorow rozrochowych NBMX}
                function ResistorsFlagCheck: boolean;

               {! przelacznik pradu automatycznego rozruchu}
                function MinCurrentSwitch(State:boolean):boolean;
               {! przelacznik automatycznego rozruchu}
                function AutoRelaySwitch(State:boolean):boolean;
               {symulacja automatycznego rozruchu}
                function AutoRelayCheck: boolean;

                {-funkcje typowe dla lokomotywy spalinowej z przekladnia mechaniczna}
                function dizel_EngageSwitch(state: real): boolean;
                function dizel_EngageChange(dt: real): boolean;
                function dizel_AutoGearCheck: boolean;
                function dizel_fillcheck(mcp:byte): real;
{                function dizel_rotateengine(Momentum,dt,n,engage:real): real;  }
                function dizel_Momentum(dizel_fill,n,dt:real): real;
                function dizel_Update(dt:real): boolean;

               {-funkcje dla wagonow}
                function LoadingDone(LSpeed: real; LoadInit:string):boolean;

                {! WAZNA FUNKCJA - oblicza sile wypadkowa}
                procedure ComputeTotalForce(dt: real; dt1: real; FullVer: boolean);

                {! BARDZO WAZNA FUNKCJA - oblicza przesuniecie lokomotywy lub wagonu}
                function ComputeMovement(dt:real; dt1:real; Shape:TTrackShape; var Track:TTrackParam; var ElectricTraction:TTractionParam; NewLoc:TLocation; var NewRot:TRotation):real;
                {dt powinno byc 10ms lub mniejsze}
                {NewRot na wejsciu daje aktualna orientacje pojazdu, na wyjsciu daje dodatkowe wychylenia od tej orientacji}

                {! JESZCZE WAZNIEJSZA FUNKCJA - ABu: - mocno zoptymalizowanana ComputeMovement}
                function FastComputeMovement(dt:real; Shape:TTrackShape; var Track:TTrackParam; NewLoc:TLocation; var NewRot:TRotation):real; //Shape:TTrackShape; var Track:TTrackParam; var ElectricTraction:TTractionParam; NewLoc:TLocation; var NewRot:TRotation):real;
                {dt powinno byc 10ms lub mniejsze}
                {NewRot na wejsciu daje aktualna orientacje pojazdu, na wyjsciu daje dodatkowe wychylenia od tej orientacji}


                {dla samochodow}
                function ChangeOffsetH(DeltaOffset:real): boolean;

                {! procedury I/O}
                constructor Init(LocInitial:TLocation; RotInitial:TRotation;
                                 VelInitial:real; TypeNameInit, NameInit: string; LoadInitial:longint; LoadTypeInitial: string; Cab:integer);
                                                              {wywolac najpierw to}
                function LoadChkFile(chkpath:string):Boolean;   {potem zaladowac z pliku}
                function CheckLocomotiveParameters(ReadyFlag:boolean;Dir:longint): boolean;  {a nastepnie sprawdzic}
                function EngineDescription(what:integer): string;  {opis stanu lokomotywy}

   {obsluga drzwi}
                function DoorLeft(State: Boolean): Boolean;    {obsluga dzwi lewych}
                function DoorRight(State: Boolean): Boolean;
   {obsluga pantografow - Winger 160204}
                function PantFront(State: Boolean): Boolean;
                function PantRear(State: Boolean): Boolean;
   {ogrzewanie - Winger 020304}
               //function Heating(State: Boolean): Boolean;

                //private
                //function TotalMass:real;
{                function GetFTotal(CN:byte): real;
                function GetMass(CN:byte): real;
                procedure SendAVL(AccS,AccN,V,dL:real; CN:byte); }
              end;    {ufff}

{   PMoverParameters=^TMoverParameters; }
function Distance(Loc1,Loc2: TLocation; Dim1,Dim2:TDimension) : real;

{----------------------------------------------------------------}
{========================McZapkie'2001===========================}
{----------------------------------------------------------------}
implementation

const
   Steel2Steel_friction=0.15;  {tarcie statyczne}
   g=9.81;                     {przyspieszenie ziemskie}
   SandSpeed=0.1;              {ile kg/s}
   RVentSpeed=0.4;             {rozpedzanie sie wentylatora obr/s^2}
   RVentMinI=50.0;             {przy jakim pradzie sie wylaczaja}
   CouplerTune:real=0.1;       {skalowanie tlumiennosci}
   Pirazy2=Pi*2;

{------------------------------------------}
{funkcje pomocnicze}

function s2NPW(s:string):byte;
{wylicza ilosc osi napednych ze stringu opisujacego uklad osi}
{np. 2'A' daje 1, Bo'Bo' daje 4, 1'E daje 5 itp}
const A=ord('A')-1;
var k,NPW:byte;
begin
  NPW:=0;
  for k:=1 to Length(s) do
   begin
     if s[k] in ['A'..'Z'] then
      NPW:=NPW+Ord(s[k])-A;
   end;
  s2NPW:=NPW;
end;

function s2NNW(s:string):byte;
{wylicza ilosc osi nienapednych ze stringu opisujacego uklad osi}
{np. 2'A' daje 2, Bo'Bo' daje 0, 1'E daje 1 itp}
const Zero=ord('0');
var k,NNW:byte;
begin
  NNW:=0;
  for k:=1 to Length(s) do
   begin
     if s[k] in ['1'..'9'] then
      NNW:=NNW+Ord(s[k])-Zero;
   end;
  s2NNW:=NNW;
end;

function Distance(Loc1,Loc2:TLocation;Dim1,Dim2:TDimension):real;
{zwraca odleg³oœæ pomiêdzy pojazdami (Loc1) i (Loc2) z uwzglêdnieneim ich d³ugoœci}
var Dist:real;
begin
 Dist:=SQRT(SQR(Loc2.x-Loc1.x)+SQR(Loc1.y-Loc2.y));
 Distance:=Dist-Dim2.L/2.0-Dim1.L/2.0;
{  Distance:=Hypot(Loc2.x-Loc1.x,Loc2.y-Loc1.y)-(Dim2.L+Dim1.L)/2; }
end;

{oblicza zmiane predkosci i przyrost pedu wskutek kolizji}
function ComputeCollision(var v1,v2:real; m1,m2,beta:real;vc:boolean):real;
var CCP,w1,w2,m1m2:real;
begin
  if (v1<v2) and vc then
   CCP:=0
  else
   begin
     m1m2:=m1+m2;
     w1:=((m1-m2)*v1+2*m2*v2)/m1m2;
     w2:=((m2-m1)*v2+2*m1*v1)/m1m2;
     v1:=w1*sqrt(1-beta); {niejawna zmiana predkosci wskutek zderzenia}
     v2:=w2*sqrt(1-beta);
     CCP:=m1*(w2-w1)*(1-beta);
   end;
  ComputeCollision:=CCP;
end;

{-----zderzaki,sprzegi----}

{ABu: ta funkcja jest ignorowana, bo nie uwzgledniaja przylaczenia
 sprzegow 0 <-> 0 oraz 1 <-> 1. Zamiast niej tablica: CouplerNr[CouplerN];}
{
function CouplerNext(CouplerN:byte):byte;
begin
  case CouplerN of
    0: CouplerNext:=1;
    1: CouplerNext:=0;
  end;
end;
}

{poprawka dla liczenia sil przy ustawieniu przeciwnym obiektow:}
function DirPatch(Coupler1:byte; Coupler2:byte): real;
begin
  if (Coupler1<>Coupler2) then DirPatch:=1
                        else DirPatch:=-1;
end;


function DirF(CouplerN:byte): real;
begin
  case CouplerN of
   0: DirF:=-1;
   1: DirF:=1
   else DirF:=0;
  end;
end;

{yB: funkcje dodatkowe hamulcowe}

function PR(p1,p2:real): real;
var ph,pl:real;
begin
  ph:=Max0R(p1,p2)+0.1;
  pl:=p1+p2-ph+0.2;
  PR:=(ph-pl)/(1.13*ph-pl);
end;

function SPR(ph,pl:real): real;
begin
  SPR:=0.674*(ph-pl)/(1.13*ph-pl+0.013);
end;
{---------rozwiniecie deklaracji metod obiektu TMoverParameters--------}
function TMoverParameters.GetTrainsetVoltage: boolean;
{ABu: funkcja zwracajaca napiecie dla calego skladu, przydatna dla EZT}
var voltf: boolean;
    voltr: boolean;
  begin
    if Couplers[0].Connected<>nil then
      begin
        if Couplers[0].Connected^.PantFrontVolt or Couplers[0].Connected^.PantRearVolt then
          voltf:=true
        else
          voltf:=false;
      end
      else
        voltf:=false;
      if Couplers[1].Connected<>nil then
      begin
        if Couplers[1].Connected^.PantFrontVolt or Couplers[1].Connected^.PantRearVolt then
          voltr:=true
        else
          voltr:=false;
      end
      else
        voltr:=false;
    GetTrainsetVoltage:=voltf or voltr;
  end;



function TMoverParameters.Physic_ReActivation: boolean;
 begin
   if PhysicActivation then
    Physic_Reactivation:=false
   else
    begin
      PhysicActivation:=true;
      LastSwitchingTime:=0;
      Physic_Reactivation:=true;
    end;
 end;


{stosunek obecnego nastawienia sily hamowania do maksymalnej nastawy}
{
function TMoverParameters.BrakeRatio: real;
 begin
   if BrakeCtrlPosNo>0 then
    begin
      if BrakeCtrlPos<0 then
       BrakeRatio:=1
      else
       BrakeRatio:=1-BrakeCtrlPos/BrakeCtrlPosNo
    end
   else
    BrakeRatio:=0;
 end;
}

function TMoverParameters.LocalBrakeRatio: real;
var LBR:real;
begin
  if LocalBrakePosNo>0 then
   LBR:=LocalBrakePos/LocalBrakePosNo
  else
   LBR:=0;
 if TestFlag(BrakeStatus,b_antislip) then
   LBR:=Max0R(LBR,PipeRatio)+0.4;
 LocalBrakeRatio:=LBR;
end;

function TMoverParameters.PipeRatio: real;
var pr:real;
begin
  if DeltaPipePress>0 then
   if (BrakeSubsystem=Oerlikon) then
    begin
     if (3*PipePress)>(HighPipePress+LowPipePress+LowPipePress) then
      pr:=(HighPipePress-Min0R(HighPipePress,PipePress))/(DeltaPipePress*4.0/3.0)
     else
      pr:=(HighPipePress-1.0/3.0*DeltaPipePress-Max0R(LowPipePress,PipePress))/(DeltaPipePress*2.0/3.0);
     if (not TestFlag(BrakeStatus,b_Ractive)) and (BrakeMethod and 1 = 0) and TestFlag(BrakeDelays,bdelay_R) and (Power<1) and (BrakeCtrlPos<1) then
      pr:=Min0R(0.5,pr);
    end
   else
     pr:=(HighPipePress-Max0R(LowPipePress,Min0R(HighPipePress,PipePress)))/DeltaPipePress
  else
    pr:=0;
  PipeRatio:=pr;
end;

function TMoverParameters.RealPipeRatio: real;
var rpp:real;
begin
  if DeltaPipePress>0 then
    rpp:=(CntrlPipePress-PipePress)/(DeltaPipePress)
  else
    rpp:=0;
  RealPipeRatio:=rpp;
end;

function TMoverParameters.BrakeVP: real;
begin
  if BrakeVVolume>0 then
   BrakeVP:=Volume/(10.0*BrakeVVolume)
  else BrakeVP:=0;
end;

{reczne przelaczanie hamulca elektrodynamicznego}
function TMoverParameters.DynamicBrakeSwitch(Switch:boolean):boolean;
var b:byte;
begin
  if (DynamicBrakeType=dbrake_switch) and (MainCtrlPos=0) then
   begin
     DynamicBrakeFlag:=Switch;
     DynamicBrakeSwitch:=True;
     for b:=0 to 1 do
      with Couplers[b] do
       if TestFlag(CouplingFlag,ctrain_controll) then
        Connected^.DynamicBrakeFlag:=DynamicBrakeFlag;
   end
  else
   DynamicBrakeSwitch:=False;
end;


function TMoverParameters.SendCtrlBroadcast(CtrlCommand:string;ctrlvalue:real):boolean;
var b:byte; OK:boolean;
begin
  OK:=((CtrlCommand<>CommandIn.Command) and (ctrlvalue<>CommandIn.Value1));
  if OK then
   for b:=0 to 1 do
    with Couplers[b] do
     if TestFlag(CouplingFlag,ctrain_controll) then
      if Connected^.SetInternalCommand(CtrlCommand,ctrlvalue,DirF(b)) then
       OK:=Connected^.RunInternalCommand or OK;
  SendCtrlBroadcast:=OK;
end;


function TMoverParameters.SendCtrlToNext(CtrlCommand:string;ctrlvalue,dir:real):boolean;
//wys³anie komendy w kierunku dir (1=przod,-1=ty³)
var
 OK:Boolean;
 d:Integer;
begin
//Ra: by³ problem z propagacj¹, jeœli w sk³adzie jest pojazd wstawiony odwrotnie
//Ra: problem jest równie¿, jeœli AI bêdzie na koñcu sk³adu
 OK:=(dir<>0); // and Mains;
 d:=(1+Sign(dir)) div 2; //-1=>0, 1=>1 - wysy³anie tylko w ty³
 if OK then
  with Couplers[d] do //w³asny sprzêg od strony (d)
   if TestFlag(CouplingFlag,ctrain_controll) then
    if ConnectedNr<>d then //jeœli ten nastpêny jest zgodny z aktualnym
     begin
      if Connected^.SetInternalCommand(CtrlCommand,ctrlvalue,dir) then
       OK:=Connected^.RunInternalCommand and OK;
     end
     else //jeœli nastêpny jest ustawiony przeciwnie, zmieniamy kierunek
      if Connected^.SetInternalCommand(CtrlCommand,ctrlvalue,-dir) then
       OK:=Connected^.RunInternalCommand and OK;
 SendCtrlToNext:=OK;
end;

procedure TMoverParameters.PantCheck;
var c: boolean;
begin
  if (CabNo*LastCab<0) and (CabNo<>0) then
   begin
     c:=PantFrontUp;
     PantFrontUp:=PantRearUp;
     PantRearUp:=c;
   end;
end;

function TMoverParameters.CabActivisation:boolean;
//za³¹czenie rozrz¹du
var OK:boolean;
begin
  OK:=(CabNo=0);
  if(OK)then
   begin
     CabNo:=ActiveCab;
     DirAbsolute:=ActiveDir*CabNo;
     SendCtrlToNext('CabActivisation',CabNo,ActiveCab);
     PantCheck;
   end;
  CabActivisation:=OK;
end;

function TMoverParameters.CabDeactivisation:boolean;
//wy³¹czenie rozrz¹du
var OK:boolean;
begin
 OK:=(CabNo=ActiveCab);
 if(OK)then
  begin
   LastCab:=CabNo;
   CabNo:=0;
   DirAbsolute:=ActiveDir*CabNo;
   SendCtrlToNext('CabActivisation',0,ActiveCab);
  end;
 CabDeactivisation:=OK;
end;

{
procedure MainCtrlBroadcast(Couplers:TCouplers; MainCtrlPos,MainCtrlActualPos:byte);
var b:byte;
begin

  for b:=0 to 1 do
   with Couplers[b] do
    if TestFlag(CouplingFlag,ctrain_controll) then
     begin
       Connected^.MainCtrlPos:=MainCtrlPos;
       Connected^.MainCtrlActualPos:=MainCtrlActualPos;
     end;
poprawic to!!!
end;

procedure ScndCtrlBroadcast(Couplers:TCouplers; ScndCtrlPos,ScndCtrlActualPos:byte);
var b:byte;
begin
  for b:=0 to 1 do
   with Couplers[b] do
    if TestFlag(CouplingFlag,ctrain_controll) then
     begin
       Connected^.ScndCtrlPos:=ScndCtrlPos;
       Connected^.ScndCtrlActualPos:=ScndCtrlActualPos;
     end;
end;
}

{funkcje zwiekszajace/zmniejszajace nastawniki}

function TMoverParameters.IncMainCtrl(CtrlSpeed:integer): boolean;
var //b:byte;
    OK:boolean;
begin
 OK:=false;
 if (MainCtrlPosNo>0) and (CabNo<>0) then
  begin
   if MainCtrlPos<MainCtrlPosNo then
    case EngineType of
     None, Dumb, DieselElectric:      { EZT:}
      if CtrlSpeed=1 then
       begin
        inc(MainCtrlPos);
        OK:=True;
       end
      else
       if CtrlSpeed>1 then
         OK:=IncMainCtrl(1) and IncMainCtrl(CtrlSpeed-1);
     ElectricSeriesMotor:
      if (CtrlSpeed=1) and (ActiveDir<>0) then
       begin
        inc(MainCtrlPos);
        OK:=True;
         if Imax=ImaxHi then
          if RList[MainCtrlPos].Bn>1 then
           begin
            if(TrainType=dt_ET42)then
             begin
              dec(MainCtrlPos);
              OK:=false;
             end;
            if MaxCurrentSwitch(false) then
              SetFlag(SoundFlag,sound_relay); {wylaczanie wysokiego rozruchu}
{         if (EngineType=ElectricSeriesMotor) and (MainCtrlPos=1) then
          MainCtrlActualPos:=1;
}          end;
        if(DynamicBrakeFlag)then
          if(TrainType=dt_ET42)then
            if MainCtrlPos>20 then
             begin
              dec(MainCtrlPos);
              OK:=false;
             end;
       end
      else
       if (CtrlSpeed>1) and (ActiveDir<>0) then
        begin
          while (RList[MainCtrlPos].R>0) and IncMainCtrl(1) do
           OK:=True ; {takie chamskie, potem poprawie}
         if(DynamicBrakeFlag)then
           if(TrainType=dt_ET42)then
             while(MainCtrlPos>20)do
               dec(MainCtrlPos);
         OK:=false;
        end;
     DieselEngine:
      if CtrlSpeed=1 then
       begin
        inc(MainCtrlPos);
        OK:=True;
        if MainCtrlPos>0 then
         CompressorAllow:=true
        else
         CompressorAllow:=false;
       end
      else
       if CtrlSpeed>1 then
        begin
          while (MainCtrlPos<MainCtrlPosNo) do
           IncMainCtrl(1);
          OK:=True ;
        end;
     WheelsDriven:
      OK:=AddPulseForce(CtrlSpeed);
    end {case EngineType}
   else
    if CoupledCtrl then {wspolny wal}
      begin //Ra:tu jest coœ bez sensu, OK=False i EN57 stoi
        if ScndCtrlPos<ScndCtrlPosNo then //3<3 -> false
          begin
            inc(ScndCtrlPos);
            OK:=True;
          end
         else OK:=False;
      end;
   if OK then
    begin
     {OK:=}SendCtrlToNext('MainCtrl',MainCtrlPos,CabNo);  {???}
     {OK:=}SendCtrlToNext('ScndCtrl',ScndCtrlPos,CabNo);
    end;
  end
 else {nie ma sterowania}
  OK:=False;
 if OK then LastRelayTime:=0;
 IncMainCtrl:=OK;
end;


function TMoverParameters.DecMainCtrl(CtrlSpeed:integer): boolean;
var //b:byte;
    OK:boolean;
begin
  OK:=false;
  if (MainCtrlPosNo>0) and (CabNo<>0) then
   begin
      if MainCtrlPos>0 then
       begin
         if CoupledCtrl and (ScndCtrlPos>0) then
          begin
            dec(ScndCtrlPos); {wspolny wal}
            OK:=True;
          end
         else
          case EngineType of
            None, Dumb, DieselElectric:   { EZT:}
             if CtrlSpeed=1 then
              begin
                dec(MainCtrlPos);
                OK:=True;
              end
             else
              if CtrlSpeed>1 then
               OK:=DecMainCtrl(1) and DecMainCtrl(CtrlSpeed-1);
            ElectricSeriesMotor:
             if CtrlSpeed=1 then
              begin
                dec(MainCtrlPos);
                if (MainCtrlPos=0) and (ScndCtrlPos=0) then
                 StLinFlag:=false;
                if (MainCtrlPos=0) then
                 MainCtrlActualPos:=0;
                OK:=True;
              end
             else
              if CtrlSpeed>1 then
               begin
                 OK:=True;
                  if (RList[MainCtrlPos].R=0)
                   then DecMainCtrl(1);
                  while (RList[MainCtrlPos].R>0) and DecMainCtrl(1) do ; {takie chamskie, potem poprawie}
               end;
            DieselEngine:
             if CtrlSpeed=1 then
              begin
                dec(MainCtrlPos);
                OK:=True;
              end
              else
              if CtrlSpeed>1 then
               begin
                 while (MainCtrlPos>0) or (RList[MainCtrlPos].Mn>0) do
                  DecMainCtrl(1);
                 OK:=True ;
               end;
          end;
       end
      else
       if EngineType=WheelsDriven then
        OK:=AddPulseForce(-CtrlSpeed)
       else
        OK:=False;
      if OK then
       begin
          {OK:=}SendCtrlToNext('MainCtrl',MainCtrlPos,CabNo);  {hmmmm...???!!!}
          {OK:=}SendCtrlToNext('ScndCtrl',ScndCtrlPos,CabNo);
       end;
   end
  else
   OK:=False;
  if OK then LastRelayTime:=0;
  DecMainCtrl:=OK;
end;

function TMoverParameters.IncScndCtrl(CtrlSpeed:integer): boolean;
var //b:byte;
    OK:boolean;
begin
  if (MainCtrlPos=0)and(CabNo<>0)and(TrainType=dt_ET42)and(ScndCtrlPos=0)and(DynamicBrakeFlag)then
   begin
    OK:=DynamicBrakeSwitch(False);
   end
  else
 if (ScndCtrlPosNo>0) and (CabNo<>0) and not ((TrainType=dt_ET42)and((Imax=ImaxHi)or((DynamicBrakeFlag)and(MainCtrlPos>0)))) then
  begin
{      if (RList[MainCtrlPos].R=0) and (MainCtrlPos>0) and (ScndCtrlPos<ScndCtrlPosNo) and (not CoupledCtrl) then}
   if (ScndCtrlPos<ScndCtrlPosNo) and (not CoupledCtrl) and ((EngineType<>DieselElectric) or (not AutoRelayFlag)) then
    begin
      if CtrlSpeed=1 then
       begin
         inc(ScndCtrlPos);
       end
      else
       if CtrlSpeed>1 then
        begin
          ScndCtrlPos:=ScndCtrlPosNo; {takie chamskie, potem poprawie}
        end;
      OK:=True;
    end
   else {nie mozna zmienic}
    OK:=False;
   if OK then
    begin
      {OK:=}SendCtrlToNext('MainCtrl',MainCtrlPos,CabNo);   {???}
      {OK:=}SendCtrlToNext('ScndCtrl',ScndCtrlPos,CabNo);
    end;
  end
 else {nie ma sterowania}
  OK:=False;
 if OK then LastRelayTime:=0;
 IncScndCtrl:=OK;
end;

function TMoverParameters.DecScndCtrl(CtrlSpeed:integer): boolean;
var //b:byte;
    OK:boolean;
begin
  if (MainCtrlPos=0)and(CabNo<>0)and(TrainType=dt_ET42)and(ScndCtrlPos=0)and not(DynamicBrakeFlag)then
   begin
    OK:=DynamicBrakeSwitch(true);
   end
  else
  if (ScndCtrlPosNo>0) and (CabNo<>0) then
   begin
    if (ScndCtrlPos>0) and (not CoupledCtrl) and ((EngineType<>DieselElectric) or (not AutoRelayFlag))then
     begin
      if CtrlSpeed=1 then
       begin
         dec(ScndCtrlPos);
       end
      else
       if CtrlSpeed>1 then
        begin
          ScndCtrlPos:=0; {takie chamskie, potem poprawie}
        end;
      OK:=True;
     end
    else
     OK:=False;
    if OK then
     begin
      {OK:=}SendCtrlToNext('MainCtrl',MainCtrlPos,CabNo);  {???}
      {OK:=}SendCtrlToNext('ScndCtrl',ScndCtrlPos,CabNo);
     end;
   end
  else
   OK:=False;
 if OK then LastRelayTime:=0;
 DecScndCtrl:=OK;
end;

function TMoverParameters.DirectionForward: boolean;
begin
  if (MainCtrlPosNo>0) and (ActiveDir<1) and (MainCtrlPos=0) then
   begin
     inc(ActiveDir);
     DirAbsolute:=ActiveDir*CabNo;
     DirectionForward:=True;
     SendCtrlToNext('Direction',ActiveDir,CabNo);
   end
  else if (ActiveDir=1) and (MainCtrlPos=0) and (TrainType=dt_EZT) then
    DirectionForward:=MinCurrentSwitch(true)
  else
    DirectionForward:=False;
end;

function TMoverParameters.DirectionBackward: boolean;
begin
  if (ActiveDir=1) and (MainCtrlPos=0) and (TrainType=dt_EZT) then
    if MinCurrentSwitch(false) then
    begin
      DirectionBackward:=True;
      Exit;
    end;
  if (MainCtrlPosNo>0) and (ActiveDir>-1) and (MainCtrlPos=0) then
   begin
    if EngineType=WheelsDriven then
     dec(CabNo);
{    else}
    dec(ActiveDir);
    DirAbsolute:=ActiveDir*CabNo;
    DirectionBackward:=True;
    SendCtrltoNext('Direction',ActiveDir,CabNo);
   end
  else
   DirectionBackward:=False;
end;

function TMoverParameters.MainSwitch(State:boolean): boolean;
begin
 MainSwitch:=False; //Ra: przeniesione z koñca
  if ((Mains<>State) and (MainCtrlPosNo>0)) then
   begin
    if (State=False) or ({(MainCtrlPos=0) and} (ScndCtrlPos=0) and (ConvOvldFlag=false) and (LastSwitchingTime>CtrlDelay) and not TestFlag(DamageFlag,dtrain_out)) then
     begin
       if Mains then //jeœli by³ za³¹czony
        SendCtrlToNext('MainSwitch',ord(State),CabNo); //wys³anie wy³¹czenia do pozosta³ych?
       Mains:=State;
       if Mains then //jeœli zosta³ za³¹czony
        SendCtrlToNext('MainSwitch',ord(State),CabNo); //wyslanie po wlaczeniu albo przed wylaczeniem
       MainSwitch:=True; //wartoœæ zwrotna
       LastSwitchingTime:=0;
       if (EngineType=DieselEngine) and Mains then
        begin
          dizel_enginestart:=State;
        end;
       if (State=False) then //jeœli wy³¹czony
        begin
         SetFlag(SoundFlag,sound_relay);
         SecuritySystem.Status:=0; //deaktywacja czuwaka
        end
       else
        SecuritySystem.Status:=s_waiting; //aktywacja czuwaka
     end
   end
  //else MainSwitch:=False;
end;

function TMoverParameters.ChangeCab(direction:integer): boolean;
//var //b:byte;
//    c:boolean;
begin
 if Abs(ActiveCab+direction)<2 then
  begin
//  if (ActiveCab+direction=0) then LastCab:=ActiveCab;
   ActiveCab:=ActiveCab+direction;
   ChangeCab:=True;
   if (BrakeSystem=Pneumatic) and (BrakeCtrlPosNo>0) then
    begin
     BrakeCtrlPos:=-2;
     LimPipePress:=PipePress;
     ActFlowSpeed:= 0;
    end
   else
    BrakeCtrlPos:=0;
//   if not TestFlag(BrakeStatus,b_dmg) then
//    BrakeStatus:=b_off;
   MainCtrlPos:=0;
   ScndCtrlPos:=0;
   if (EngineType<>DieselEngine) and (EngineType<>DieselElectric) then
    begin
     Mains:=False;
     CompressorAllow:=false;
     ConverterAllow:=false;
    end;
//   if (ActiveCab<>LastCab) and (ActiveCab<>0) then
//    begin
//     c:=PantFrontUp;
//     PantFrontUp:=PantRearUp;
//     PantRearUp:=c;
//    end; //yB: poszlo do wylacznika rozrzadu
   ActiveDir:=0;
   DirAbsolute:=0;
  end
 else
  ChangeCab:=False;
end;

{wl/wyl przetwornicy}
function TMoverParameters.ConverterSwitch(State:boolean):boolean;
begin
 ConverterSwitch:=false; //Ra: normalnie chyba false?
 if (ConverterAllow<>State) then
 begin
   ConverterAllow:=State;
   ConverterSwitch:=true;
   if CompressorPower=2 then
    CompressorAllow:=ConverterAllow;
 end;
 if (ConverterAllow=true) then SendCtrlToNext('ConverterSwitch',1,CabNo)
 else SendCtrlToNext('ConverterSwitch',0,CabNo)
end;

{wl/wyl sprezarki}
function TMoverParameters.CompressorSwitch(State:boolean):boolean;
begin
 CompressorSwitch:=false; //Ra: normalnie chyba tak?
// if State=true then
//  if ((CompressorPower=2) and (not ConverterAllow)) then
//   State:=false; //yB: to juz niepotrzebne
 if (CompressorAllow<>State)and(CompressorPower<2)then
 begin
   CompressorAllow:=State;
   CompressorSwitch:=true;
 end;
 if (CompressorAllow=true) then SendCtrlToNext('CompressorSwitch',1,CabNo)
 else SendCtrlToNext('CompressorSwitch',0,CabNo)
end;

{wl/wyl malej sprezarki}
{function TMoverParameters.SmallCompressorSwitch(State:boolean):boolean;
begin
 if State=true then
  if ((SmallCompressorPower=1) then
   State:=false;
 if (SmallCompressorAllow<>State) then
 begin
   SmallCompressorAllow:=State;
   SmallCompressorSwitch:=true;
 end;
end;
}
{sypanie piasku}
function TMoverParameters.SandDoseOn : boolean;
begin
  if (SandCapacity>0) then
   begin
     SandDoseOn:=True;
     if SandDose then
      SandDose:=False
     else
      if Sand>0 then SandDose:=True;
     if CabNo<>0 then
       SendCtrlToNext('SandDoseOn',1,CabNo);
   end
  else
   SandDoseOn:=False;
end;

function TMoverParameters.SecuritySystemReset : boolean;
//zbijanie czuwaka/SHP
 procedure Reset;
  begin
    SecuritySystem.SystemTimer:=0;
    SecuritySystem.SystemBrakeTimer:=0;
    SecuritySystem.SystemSoundTimer:=0;
    SecuritySystem.Status:=s_waiting; //aktywacja czuwaka
    SecuritySystem.VelocityAllowed:=-1;
  end;
begin
  with SecuritySystem do
    if (SystemType>0) and (Status>0) then
      begin
        SecuritySystemReset:=True;
        if (Status<s_ebrake) then
         Reset
        else
          if EmergencyBrakeSwitch(False) then
           Reset;
      end
    else
     SecuritySystemReset:=False;
//  SendCtrlToNext('SecurityReset',0,CabNo);
end;

{testowanie czuwaka/SHP}
procedure TMoverParameters.SecuritySystemCheck(dt:real);
begin
  with SecuritySystem do
   begin
     if (SystemType>0) and (Status>0) then
      begin
        SystemTimer:=SystemTimer+dt;
        if TestFlag(Status,s_aware) or TestFlag(Status,s_active) then //jeœli œwieci albo miga
         SystemSoundTimer:=SystemSoundTimer+dt;
        if TestFlag(Status,s_alarm) then //jeœli buczy
         SystemBrakeTimer:=SystemBrakeTimer+dt;
        if TestFlag(SystemType,1) then
         if (SystemTimer>AwareDelay) and (AwareDelay>=0) then  {-1 blokuje}
           if not SetFlag(Status,s_aware) then {juz wlaczony sygnal swietlny}
             if (SystemSoundTimer>SoundSignalDelay) and (SoundSignalDelay>=0) then
               if not SetFlag(Status,s_alarm) then {juz wlaczony sygnal dzwiekowy}
                 if (SystemBrakeTimer>EmergencyBrakeDelay) and (EmergencyBrakeDelay>=0) then
                   SetFlag(Status,s_ebrake);        {przeterminowanie czuwaka, hamowanie awaryjne}
        if TestFlag(SystemType,2) and TestFlag(Status,s_active) then
         if (Vel>VelocityAllowed) and (VelocityAllowed>=0) then
          SetFlag(Status,s_ebrake)
         else
          if ((SystemSoundTimer>SoundSignalDelay) and (SoundSignalDelay>=0)) or ((Vel>NextVelocityAllowed) and (NextVelocityAllowed>=0)) then
            if not SetFlag(Status,s_alarm) then {juz wlaczony sygnal dzwiekowy}
              if (SystemBrakeTimer>EmergencyBrakeDelay) and (EmergencyBrakeDelay>=0) then
               SetFlag(Status,s_ebrake);
        if TestFlag(Status,s_ebrake) then
         if not EmergencyBrakeFlag then
           EmergencyBrakeSwitch(True);
      end;
   end;
end;


{nastawy hamulca}

function TMoverParameters.IncBrakeLevel:boolean;
//var b:byte;
begin
  if (BrakeCtrlPosNo>0) {and (LocalBrakePos=0)} then
   begin
     if BrakeCtrlPos<BrakeCtrlPosNo then
      begin
        inc(BrakeCtrlPos);

//youBy: wywalilem to, jak jest EP, to sa przenoszone sygnaly nt. co ma robic, a nie poszczegolne pozycje;
//       wystarczy spojrzec na Knorra i Oerlikona EP w EN57; mogly ze soba wspolapracowac
{
        if (BrakeSystem=ElectroPneumatic) then
          if (BrakePressureTable[BrakeCtrlPos].BrakeType=ElectroPneumatic) then
           begin
//             BrakeStatus:=ord(BrakeCtrlPos>0);
             SendCtrlToNext('BrakeCtrl',BrakeCtrlPos,CabNo);
           end
          else SendCtrlToNext('BrakeCtrl',-2,CabNo);
//        else
//         if not TestFlag(BrakeStatus,b_dmg) then
//          BrakeStatus:=b_on;}

//youBy: EP po nowemu

        IncBrakeLevel:=True;
        if (BrakePressureTable[BrakeCtrlPos].PipePressureVal<0)and(BrakePressureTable[BrakeCtrlPos-1].PipePressureVal>0) then
          LimPipePress:=PipePress;

        if (BrakeSystem=ElectroPneumatic) then
          if (BrakeSubSystem<>Knorr) then
           begin
             if (BrakeCtrlPos*BrakeCtrlPos)=1 then
              begin
                SendCtrlToNext('Brake',BrakeCtrlPos,CabNo);
                SetFlag(BrakeStatus,b_epused);
              end
             else
              begin
                SendCtrlToNext('Brake',0,CabNo);
                SetFlag(BrakeStatus,-b_epused);
           end;
           end;

        //yB: dla Oerlikona jest zdeka ulanskie napelnianie
        if(BrakeSubsystem=Oerlikon)and(BrakeSystem=Pneumatic)then
         if(BrakeCtrlPos=-1)then
          with(BrakePressureTable[BrakeCtrlPos])do
           begin
            LimPipePress:=0.85;
            ActFlowSpeed:=FlowSpeedVal;
           end;

      end
     else
      begin
        IncBrakeLevel:=False;
{        if BrakeSystem=Pneumatic then
         EmergencyBrakeSwitch(True); }
      end;
   end
  else
   IncBrakeLevel:=False;
end;

function TMoverParameters.DecBrakeLevel:boolean;
//var b:byte;
begin
  if (BrakeCtrlPosNo>0) {and (LocalBrakePos=0)} then
   begin
     if (BrakeCtrlPos>-1) or ((BrakeSubsystem=Oerlikon) and (BrakeCtrlPos>(-2+Byte(BrakeSystem=ElectroPneumatic)))) then
      begin
        dec(BrakeCtrlPos);
        if EmergencyBrakeFlag then
          begin
             EmergencyBrakeFlag:=false; {!!!}
             SendCtrlToNext('Emergency_brake',0,CabNo);
          end;

//youBy: wywalilem to, jak jest EP, to sa przenoszone sygnaly nt. co ma robic, a nie poszczegolne pozycje;
//       wystarczy spojrzec na Knorra i Oerlikona EP w EN57; mogly ze soba wspolapracowac
{
        if (BrakeSystem=ElectroPneumatic) then
          if BrakePressureTable[BrakeCtrlPos].BrakeType=ElectroPneumatic then
           begin
//             BrakeStatus:=ord(BrakeCtrlPos>0);
             SendCtrlToNext('BrakeCtrl',BrakeCtrlPos,CabNo);
           end
          else SendCtrlToNext('BrakeCtrl',-2,CabNo);
//        else}
//         if (not TestFlag(BrakeStatus,b_dmg) and (not TestFlag(BrakeStatus,b_release))) then
//          BrakeStatus:=b_off;   {luzowanie jesli dziala oraz nie byl wlaczony odluzniacz}

//youBy: EP po nowemu
        DecBrakeLevel:=True;
        if (BrakePressureTable[BrakeCtrlPos].PipePressureVal<0.0)and(BrakePressureTable[BrakeCtrlPos+1].PipePressureVal>0) then
          LimPipePress:=PipePress;

        if (BrakeSystem=ElectroPneumatic) then
          if (BrakeSubSystem<>Knorr) then
           begin
             if (BrakeCtrlPos*BrakeCtrlPos)=1 then
              begin
                SendCtrlToNext('Brake',BrakeCtrlPos,CabNo);
                SetFlag(BrakeStatus,b_epused);
              end
             else
              begin
                SendCtrlToNext('Brake',0,CabNo);
                SetFlag(BrakeStatus,-b_epused);
              end;
              end;

        //yB: dla Oerlikona jest zdeka ulanskie napelnianie
        if(BrakeSubsystem=Oerlikon)and(BrakeSystem=Pneumatic)then
         if(BrakeCtrlPos=-1)then
          with(BrakePressureTable[BrakeCtrlPos])do
           begin
            LimPipePress:=0.85;
            ActFlowSpeed:=FlowSpeedVal;
           end;
(*    for b:=0 to 1 do  {poprawic to!}
     with Couplers[b] do
      if CouplingFlag and ctrain_controll=ctrain_controll then
       Connected^.BrakeCtrlPos:=BrakeCtrlPos;
*)
      end
     else
      DecBrakeLevel:=False;
   end
     else
      DecBrakeLevel:=False;
end;

function TMoverParameters.IncLocalBrakeLevelFAST:boolean;
begin
  if (LocalBrakePos<LocalBrakePosNo) then
   begin
     LocalBrakePos:=LocalBrakePosNo;
     IncLocalBrakeLevelFAST:=True;
   end
  else
     IncLocalBrakeLevelFAST:=False;
UnBrake:=True;
end;

function TMoverParameters.IncLocalBrakeLevel(CtrlSpeed:byte):boolean;
begin
  if (LocalBrakePos<LocalBrakePosNo) {and (BrakeCtrlPos<1)} then
   begin
     while (LocalBrakePos<LocalBrakePosNo) and (CtrlSpeed>0) do
      begin
       inc(LocalBrakePos);
       dec(CtrlSpeed);
      end;
     IncLocalBrakeLevel:=True;
   end
  else
     IncLocalBrakeLevel:=False;
UnBrake:=True;
end;

function TMoverParameters.DecLocalBrakeLevelFAST():boolean;
begin
  if LocalBrakePos>0 then
   begin
     LocalBrakePos:=0;
     DecLocalBrakeLevelFAST:=True;
   end
  else
     DecLocalBrakeLevelFAST:=False;
UnBrake:=True;
end;

function TMoverParameters.DecLocalBrakeLevel(CtrlSpeed:byte):boolean;
begin
  if LocalBrakePos>0 then
   begin
     while (CtrlSpeed>0) and (LocalBrakePos>0) do
      begin
        dec(LocalBrakePos);
        dec(CtrlSpeed);
      end;
     DecLocalBrakeLevel:=True;
   end
  else
     DecLocalBrakeLevel:=False;
UnBrake:=True;
end;

function TMoverParameters.EmergencyBrakeSwitch(Switch:boolean): boolean;
begin
  if (BrakeSystem<>Individual) and (BrakeCtrlPosNo>0) then
   begin
     if (not EmergencyBrakeFlag) and Switch then
      begin
        EmergencyBrakeFlag:=Switch;
        EmergencyBrakeSwitch:=True;
      end
     else
      begin
        if (Abs(V)<0.1) and (Switch=False) then  {odblokowanie hamulca bezpieczenistwa tylko po zatrzymaniu}
         begin
           EmergencyBrakeFlag:=Switch;
           EmergencyBrakeSwitch:=True;
         end
        else
         EmergencyBrakeSwitch:=False;
      end;
   end
  else
   EmergencyBrakeSwitch:=False;   {nie ma hamulca bezpieczenstwa gdy nie ma hamulca zesp.}
end;

function TMoverParameters.AntiSlippingBrake: boolean;
begin
 AntiSlippingBrake:=False; //Ra: przeniesione z koñca
  if ASBType=1 then
   begin
     if SetFlag(BrakeStatus,b_antislip) then
       AntiSlippingBrake:=True;
     BrakeSlippingTimer:=0;
   end
end;

function TMoverParameters.AntiSlippingButton: boolean;
var OK:boolean;
begin
  OK:=SandDoseOn;
  AntiSlippingButton:=(AntiSlippingBrake or OK);
end;

function TMoverParameters.BrakeDelaySwitch(BDS:byte): boolean;
begin
//  if BrakeCtrlPosNo>0 then
   if (BrakeDelayFlag<>BDS)and(TestFlag(BrakeDelays,BDS)) then
    begin
      BrakeDelayFlag:=BDS;
      BrakeDelaySwitch:=True;
      BrakeStatus:=(BrakeStatus and 191);
      if CabNo<>0 then
       SendCtrlToNext('BrakeDelay',BrakeDelayFlag,CabNo);
    end
  else
   BrakeDelaySwitch:=False;
end;

function TMoverParameters.IncBrakeMult(): boolean;
begin
  if (BCMFlag<3) and (BrakeCylMult[BCMFlag+1]>0) then
   begin
    BCMFlag:=BCMFlag+1;
    P2FTrans:=1000*Pi*SQR(BrakeCylRadius)*BrakeCylMult[BCMFlag]; {w kN/MPa}
    IncBrakeMult:=true;
   end else
 IncBrakeMult:=false;
end;

function TMoverParameters.DecBrakeMult(): boolean;
begin
  if (BCMFlag>0) and (BrakeCylMult[BCMFlag-1]>0) then
   begin
    BCMFlag:=BCMFlag-1;
    P2FTrans:=1000*Pi*SQR(BrakeCylRadius)*BrakeCylMult[BCMFlag]; {w kN/MPa}
    DecBrakeMult:=true;
   end else
 DecBrakeMult:=false;
end;

function TMoverParameters.BrakeReleaser: boolean;
var OK:boolean;
begin
  if (BrakeCtrlPosNo>-1) and (PipePress<CntrlPipePress) and {(BrakeStatus=b_on)}(not TestFlag(BrakeStatus,b_dmg)) then
   begin
     OK:=SetFlag(BrakeStatus,b_release);
     CntrlPipePress:=PipePress+0.015-0.0075*ord((BrakeSubsystem=Oerlikon)or(BrakeSubsystem=KE));
     if (BrakeCtrlPosNo=0) and (Power<1) and (BrakeSystem<>ElectroPneumatic) then
      Volume:=BrakeVVolume*CntrlPipePress*10;
     SetFlag(BrakeStatus,-b_on);
   end
  else
   OK:=False;
  if CabNo<>0 then //rekurencyjne wys³anie do nastêpnego
   OK:=OK and SendCtrlToNext('BrakeReleaser',0,CabNo);
  BrakeReleaser:=OK;
end;

function TMoverParameters.SwitchEPBrake(state: byte):boolean;
var
  OK:boolean;
begin
  //OK:=false;
  OK:=SetFlag(BrakeStatus,((2*State-1)*b_epused));
  SendCtrlToNext('Brake',(state*(2*BrakeCtrlPos-1)),CabNo);
  SwitchEPBrake:=OK;
end;

{uklady pneumatyczne}

function TMoverParameters.IncBrakePress(var brake:real;PressLimit,dp:real):boolean;
begin
//  if (DynamicBrakeType<>dbrake_switch) and (DynamicBrakeType<>dbrake_none) and ((BrakePress>0.2) or (PipePress<0.37{(LowPipePress+0.05)})) then
    if (DynamicBrakeType<>dbrake_switch) and (DynamicBrakeType<>dbrake_none) and (BrakePress>0.2) and (TrainType<>dt_EZT) then //yB radzi nie sprawdzaæ ciœnienia w przewodzie
    //hunter-301211: dla EN57 silnikow nie odlaczamy
     begin
       DynamicBrakeFlag:=True;                 {uruchamianie hamulca ED albo odlaczanie silnikow}
       if (DynamicBrakeType=dbrake_automatic) and (abs(Im)>60) then            {nie napelniaj wiecej, jak na EP09}
         dp:=0.0;
     end;
    if brake+dp<PressLimit then
     begin
       brake:=brake+dp;
       IncBrakePress:=True;
     end
    else
     begin
       IncBrakePress:=False;
       brake:=PressLimit;
     end;
end;

function TMoverParameters.DecBrakePress(var brake:real;PressLimit,dp:real):boolean;
begin
    if brake-dp>PressLimit then
     begin
       brake:=brake-dp;
       DecBrakePress:=True;
     end
    else
     begin
       DecBrakePress:=False;
       brake:=PressLimit;
     end;
//  if (DynamicBrakeType<>dbrake_switch) and ((BrakePress<0.1) and (PipePress>0.45{(LowPipePress+0.06)})) then
    if (DynamicBrakeType<>dbrake_switch) and (BrakePress<0.1) then //yB radzi nie sprawdzaæ ciœnienia w przewodzie
     DynamicBrakeFlag:=False;                {wylaczanie hamulca ED i/albo zalaczanie silnikow}
end;

procedure TMoverParameters.UpdateBrakePressure(dt:real);
const LBDelay=5.0; {stala czasowa hamulca}
var Rate,Speed,dp,sm:real;
begin
  dpLocalValve:=0;
  dpBrake:=0;
  if (MaxBrakePress>0) and (not TestFlag(BrakeStatus,b_dmg)) then
   begin
       {hydrauliczny hamulec pomocniczy}
       if (LocalBrake=HydraulicBrake) and (BrakeStatus=b_off) then
        begin
          LocBrakePress:=MaxBrakePress*LocalBrakeRatio;
        end;

       {elektropneumatyczny hamulec zasadniczy}
       if (BrakePressureTable[BrakeCtrlPos].BrakeType=ElectroPneumatic) and Mains and (ActiveDir<>0)then
         with BrakePressureTable[BrakeCtrlPos] do
          if BrakePressureVal<>-1 then
           begin
             if TestFlag(BrakeStatus,b_epused) then
              begin
                if (MBPM>TotalMass) then
                  sm:=sm*TotalMass/MBPM //sk¹d pierwsza wartoœæ?
                else
                  sm:=1;
                dp:=Min0R(BrakeVP,sm*BrakePressureVal);
//              case BrakeSubSystem of
//               Oerlikon,WeLu:
//                 dpLocalValve:=1.3*sign(dp-LocBrakePress)*PR(dp,LocBrakePress)*dt/(BrakeDelay[1+ord(dp>LocBrakePress)+2*BrakeDelayFlag]);
//               Knorr:
//                 dpBrake:=FlowSpeedVal*(dp-LocBrakePress)*dt/(1+BrakeDelay[1+ord(BrakePressureVal>LocBrakePress)+2*BrakeDelayFlag]);
//              end;
//                dpBrake:=FlowSpeedVal*(dp-LocBrakePress)*dt/BrakeDelay[1+ord(dp>LocBrakePress)+2*BrakeDelayFlag];
                  dpBrake:=MaxBrakePress*FlowSpeedVal/5.0*sign(dp-LocBrakePress)*PR(dp,LocBrakePress)*dt/(1.0+BrakeDelay[1+ord(dp>LocBrakePress)+2*BrakeDelayFlag]);
                //             dpLocalValve:=FlowSpeedVal/6*sign(dp-LocBrakePress)*PR(dp,LocBrakePress)*dt/(1+BrakeDelay[1+ord(BrakeStatus and b_on)+2*BrakeDelayFlag]);
//             if (dpBrake>0) then
//              Volume:=Volume-10*dpBrake*BrakeVolume;
                LocBrakePress:=LocBrakePress+dpBrake;
              end;
           end; {sterowanie cisnieniem}

       {pneumatyczny hamulec zasadniczy}
       if ((BrakeCtrlPosNo>0) and (BrakePressureTable[BrakeCtrlPos].BrakeType=Pneumatic)) or
          (BrakeSystem=Pneumatic) or (BrakeSystem=ElectroPneumatic)then
        begin
          if TestFlag(BrakeStatus,b_release)or((TrainType=dt_ET42)and(ScndCtrlActualPos<255)and(DynamicBrakeFlag))then  //odluzniacz
           begin
//               dpBrake:=(PipeBrakePress+sd)*dt/2.5;
             dpBrake:=MaxBrakePress*(PipeBrakePress)/(1.13*PipeBrakePress+0.013)*dt/2.5;
             DecBrakePress(PipeBrakePress,0,dpBrake);
             if (PipeBrakePress<0.01) then begin {SetFlag(BrakeStatus,-101);} BrakeStatus:=(BrakeStatus and 154); PipeBrakePress:=0; end;
           end
          else  //normalna praca
           begin
            sm:=1.0;
            //test pozycji pospiesznej
            if TestFlag(BrakeDelays,bdelay_R)then
              if(not TestFlag(BrakeDelayFlag,bdelay_R)and(BrakeCtrlPosNo=0)and((BrakeMethod and 1)=1))then
                sm:=0.72  {1/1.4}
              else if(TestFlag(BrakeStatus,b_Ractive)and(BrakeCtrlPosNo>0))then
                sm:=1.5;

            if (MBPM>TotalMass) then
              sm:=sm*TotalMass/MBPM;

            Rate:=PipeRatio*sm;
            if TestFlag(BrakeDelays,bdelay_R)then
              if(not TestFlag(BrakeStatus,b_Ractive)and(BrakeCtrlPosNo=0)and((BrakeMethod and 1)=0))then
                sm:=sm*0.5;

            Speed:=RealPipeRatio*sm;

                                                        {(Volume/(10*BrakeVVol ume+0.1)>MaxBrakePress/2))}
            if ((Rate*MaxBrakePress>PipeBrakePress) and (BrakeVP>PipeBrakePress) and TestFlag(BrakeStatus,b_on)) then
              begin
                if(PipeBrakePress>MaxBrakePress*0.15)and(BrakeDelayFlag=bdelay_G)then
                 dpBrake:=(0.2+Speed)/1.2*sm*MaxBrakePress*PR(MaxBrakePress*sm,PipeBrakePress)*dt/(BrakeDelay[4])
                else
                 dpBrake:=(0.2+Speed)/1.2*sm*MaxBrakePress*PR(MaxBrakePress*sm,PipeBrakePress)*dt/(BrakeDelay[2]);
                IncBrakePress(PipeBrakePress,Rate*MaxBrakePress,dpBrake)
              end;
            if (PipeBrakePress>Rate*MaxBrakePress){and(BrakeStatus=b_off)}then
              begin
                if(TestFlag(BrakeStatus,b_Rused))and not(TestFlag(BrakeStatus,b_Ractive))then
                 begin
                  SetFlag(BrakeStatus,-b_Rused);
                  sm:=1.0+0.5*byte(TestFlag(BrakeStatus,b_Rused)and(BrakeCtrlPosNo>0));
                  PipeBrakePress:=PipeBrakePress*(1.5-sm)+(sm-1.0)/sm*PipeBrakePress*2.0
//                  dpBrake:=(PipeBrakePress-MaxBrakePress*(1.5*sm);
                 end
                else if(BrakeDelayFlag=bdelay_G)then
                 dpBrake:=(1.2-Speed)/1.2*sm*MaxBrakePress*PR(PipeBrakePress,0)*dt/(BrakeDelay[3])
                else
                 dpBrake:=(1.2-Speed)/1.2*sm*MaxBrakePress*PR(PipeBrakePress,0)*dt/(BrakeDelay[1]);

                 if(dpBrake<0)then dpBrake:=0;

                DecBrakePress(PipeBrakePress,Rate*MaxBrakePress,dpBrake);
              end;
           end;

           if (BrakeSystem<>ElectroPneumatic) then
           begin
            Rate:=LocalBrakeRatio;
            if ((Rate*MaxBrakePress>LocBrakePress) and (Compressor>LocBrakePress)) then
              begin
                dpLocalValve:=(0.01+Rate)*MaxBrakePress*PR(MaxBrakePress,LocBrakePress)*dt/4.0;//5*2
                IncBrakePress(LocBrakePress,Rate*MaxBrakePress,dpLocalValve);
              end;
            if LocBrakePress>Rate*MaxBrakePress then
              begin
                dpLocalValve:=(1.01-Rate)*MaxBrakePress*PR(LocBrakePress,0)*dt/7.0;//8*2
                DecBrakePress(LocBrakePress,Rate*MaxBrakePress,dpLocalValve);
              end;
           end;
        end;{Pneumatic}
   if (BrakeSystem=Pneumatic) or (BrakeSystem=ElectroPneumatic) then
    begin
     Speed:=BrakePress;  //yB: uzycie jednej zmiennej - optymalka pamieci
     dp:=Max0R(PipeBrakePress,LocBrakePress);
     dpBrake:=sign(dp-BrakePress)*MaxBrakePress*dt/3.0;
     if(dpBrake>0)then
       begin
         IncBrakePress(BrakePress,dp,dpBrake);
         dpBrake:=BrakePress-Speed;
         if (PipeBrakePress<LocBrakePress)and((BrakeSystem=Pneumatic)or((BrakeSystem=ElectroPneumatic)and(BrakeSubsystem<>Oerlikon))) then
           CompressedVolume:=CompressedVolume-10*dpBrake*BrakeVolume
         else
           Volume:=Volume-10*dpBrake*BrakeVolume;
       end
     else
       begin
        DecBrakePress(BrakePress,dp,-dpBrake);
        dpBrake:=BrakePress-Speed;
       end;
    end;
   end; {cylindry hamulcowe}
end;  {updatebrakepressure}


procedure TMoverParameters.UpdatePipePressure(dt:real);
const LBDelay=15.0;kL=0.5;
var {b: byte;} dV{,PWSpeed}:real; c: PMoverParameters;
begin
  //ABu: zmieniam na zlecenie Olo_EU
  //PWSpeed:=800/sqr(Dim.L); a
//  PWSpeed:=1600/sqr(Dim.L); b
//yB: zmieniam po konsulatacjach z Olo
//  PWSpeed:=180/sqrt(Dim.L); c
//yB: to wynika z ksiazek, ze conajmniej 250 m/s ma fala hamowania
//  PWSpeed:=280/Dim.L; // d 280

  dpMainValve:=0;

  if (HighPipePress>0) or (LowPipePress<0) then
   begin
     if (BrakeCtrlPosNo>0)and(ActiveCab<>0)then
      begin
       case (BrakeSubSystem) of
        Oerlikon:
          with BrakePressureTable[BrakeCtrlPos] do
           begin
            if(BrakeSystem=ElectroPneumatic)then
             begin
              ActFlowSpeed:=FlowSpeedVal*Byte(BrakeType<>ElectroPneumatic)+4*Byte(BrakeType=ElectroPneumatic);
              if(PipePressureVal>0)then
               LimPipePress:=PipePressureVal;
             end
            else
             begin
              if(BrakeCtrlPos>0)or(BrakeCtrlPos=-2)then
               begin
                if PipePressureVal>=0 then
                    LimPipePress:=PipePressureVal;
                ActFlowSpeed:=FlowSpeedVal;
               end
              else
              if(BrakeCtrlPos=0)then
                begin
                  LimPipePress:=PipePressureVal;
                    if (PipePress>PipePressureVal) then
//                      if (LimPipePress>0.55) then
//                        ActFlowSpeed:=FlowSpeedVal*1.5
//                      else
                        ActFlowSpeed:=Min0R(FlowSpeedVal,(0.05*LBDelay/PR(PipePress,PipePressureVal)))
                    else
                      ActFlowSpeed:=FlowSpeedVal;
                end
              else
              if(BrakeCtrlPos=-1)then
               if(PipePress>0.699)then
                begin
                  LimPipePress:=0.54;
                  ActFlowSpeed:=5;
                end;
             end;
           end;
        Knorr:
          with BrakePressureTable[BrakeCtrlPos] do
            begin
             if not(PipePressureVal=-1)then
               LimPipePress:=PipePressureVal;
             if(BrakeCtrlPos=0)and(PipePress>LimPipePress)then
               ActFlowSpeed:=2
             else
               ActFlowSpeed:=FlowSpeedVal;
             end;
       else; {case}
       end;

//       if (BrakeCtrlPos>-2) then
//        begin
          dpPipe:=Min0R(Compressor,LimPipePress);
//!yb          deltap:=sign(dp-PipePress)*sqrt(abs((dp+0.2)*dp-(PipePress+0.2)*PipePress));
          dpMainValve:=ActFlowSpeed*sign(dpPipe-PipePress)*PR(dpPipe,PipePress)*dt/(LBDelay)*(1-ord(EmergencyBrakeFlag));
//          dpMainValve:=ActFlowSpeed*(dp-PipePress)*dt/(LBDelay);
          if (dpMainValve>0) then             {50}
            CompressedVolume:=CompressedVolume-dpMainValve*Dim.L*Spg/100.0*(1+kL*byte((BrakeCtrlPosNo>0)and(Power>1)and(BrakeSystem=Pneumatic)));
//!yb            CompressedVolume:=CompressedVolume-20*dpMainValve*BrakeVolume{*(compressor-0.3)};//{/2{3}{*6}

//!yb          PipePress:=PipePress+dpMainValve;
//        end;
      end; {sterowanie cisnieniem}

     if EmergencyBrakeFlag then         {ulepszony hamulec bezp.}
        dpMainValve:=-PipePress*2*dt;

//!yb     dpPipe:=0; {fala hamowania}
//!yb     dp:=0; {napelnianie zbiornikow pomocniczych hamulca}
(*!yb
     for b:=0 to 1 do
      begin
        if Couplers[b].Connected<>nil then
         if TestFlag(Couplers[b].CouplingFlag,ctrain_pneumatic) then
          //ABu: ponizej zakomentowalem, bo nagly nie dzialal
          //if (Couplers[b].Connected^.PipePress>=LowPipePress) or (BrakeCtrlPosNo=0) then
           begin
             dpPipe:=dpPipe+(Couplers[b].Connected^.PipePress-PipePress)*dt*PWSpeed;
             if not Couplers[b].Connected^.PhysicActivation then
              if Abs(dpPipe)>0.000000001 then
               Couplers[b].Connected^.Physic_ReActivation;  {aktywacja fizyki podlaczonego skladu}
            end;
      end; !yb*)


(*     if (BrakeVP<PipePress)and(BrakeVP<0.5) then
      begin
        dpPipe:=(BrakeVP-PipePress)*dt*2;{10}
        if dpPipe>0 then Volume:=Volume-dpPipe*BrakeVolume
        else dpPipe:=0; //yB: chwilowo, poki napelniacz pomocniczego nie bedzie lepszy
      end;   {0.0189458}  {0.035} *)
     dpPipe:=0;
//yB: napelnianie zbiornika pomocniczego pojazdu
     case BrakeSubSystem of
     Oerlikon:
        if (BrakeSystem=Pneumatic) and (Power<1) then
        begin
          if BrakeVP>PipePress then
           begin         //SPR(BrakeVP,PipePress);(-PipePress+BrakeVP)
             dpPipe:=(1-(BrakeStatus and b_on))*SPR(BrakeVP,PipePress)*(Dim.L*Spg/(Dim.L*Spg+100.0*BrakeVVolume))*dt/120.0;
           end
          else if BrakeVP<PipePress then
           if(BrakeVP<CntrlPipePress)then
            begin
//             dpPipe:=-PR(PipePress,BrakeVP)*dt/100
//                       SPR(PipePress,BrakeVP)
             dpPipe:=-SPR(PipePress,BrakeVP)*(Dim.L*Spg/(Dim.L*Spg+100.0*BrakeVVolume))*dt/BrakeDelay[1+2*byte(BrakeDelayFlag=1)];
            end      {PR(PipePress,BrakeVP)}
           else
            dpPipe:=-SPR(PipePress,BrakeVP)*(Dim.L*Spg/(Dim.L*Spg+100.0*BrakeVVolume))*dt/100.0/(1.0+3.5*ord(PipePress-0.04>BrakeVP));
        end;
     KE:
        if(BrakeVP<CntrlPipePress-0.03)and(BrakeVP<PipePress)then
         dpPipe:=-(1-(BrakeStatus and b_on))*SPR(PipePress,BrakeVP)*(Dim.L*Spg/(Dim.L*Spg+100.0*BrakeVVolume))*dt/BrakeDelay[1+2*byte(BrakeDelayFlag=1)]
        else                                 //PR(PipePress,BrakeVP)
         dpPipe:=(1-(BrakeStatus and b_on))*sign(BrakeVP-PipePress)*PR(PipePress,BrakeVP)*(Dim.L*Spg/(Dim.L*Spg+100.0*BrakeVVolume))*dt/75.0;
     else if not TestFlag(BrakeStatus,b_on) then
         dpPipe:=sign(BrakeVP-PipePress)*PR(PipePress,BrakeVP)*(Dim.L*Spg/(Dim.L*Spg+100*BrakeVVolume))*dt/50.0;
     end;        {PR*sign}
      dpPipe:=dpPipe*5;
      dpPipe:=(dpPipe)*(10.0*BrakeVVolume)/(Spg*Dim.L);
      if (dpPipe<>0) and ((BrakeVP-PipePress)/(dpPipe)<2.0) then
        dpPipe:=(BrakeVP-PipePress)/5.0;
      Volume:=Volume-dpPipe*Spg*Dim.L/10.0;


//yB: liczenie roznicy w sprzegach (przeplywy objetosci) (usunalem Spg dla optymalizacji)
      if Couplers[0].Connected<>nil then
       if TestFlag(Couplers[0].CouplingFlag,ctrain_pneumatic) then
         begin
           c:=Couplers[0].Connected; //skrot
           dV:=PR(PipePress,c^.PipePress)*240.0*dt*sign(PipePress-c^.PipePress);
           c^.PipePress:=c^.PipePress+dV/(10.0*c^.Dim.L);
           if c^.PipePress<0 then begin dV:=dV+PipePress*10*c^.Dim.L*Spg; c^.PipePress:=0;end;
           dpPipe:=dpPipe-dV/(10.0*Dim.L);
           if not c^.PhysicActivation then
            if Abs(dV)>0.000000001 then
             c^.Physic_ReActivation;  {aktywacja fizyki podlaczonego skladu}
          end;
      if Couplers[1].Connected<>nil then
       if TestFlag(Couplers[1].CouplingFlag,ctrain_pneumatic) then
         begin
           c:=Couplers[1].Connected; //skrot
           dV:=PR(PipePress,c^.PipePress)*240.0*dt*sign(PipePress-c^.PipePress);
           c^.PipePress:=c^.PipePress+dV/(10.0*c^.Dim.L);
           if c^.PipePress<0 then begin dV:=dV+PipePress*10*c^.Dim.L*Spg; c^.PipePress:=0;end;
           dpPipe:=dpPipe-dV/(10.0*Dim.L);
           if not c^.PhysicActivation then
            if Abs(dV)>0.000000001 then
             c^.Physic_ReActivation;  {aktywacja fizyki podlaczonego skladu}
     end;
                    //0.02+Random/140
      dpPipe:=(dpPipe-0.028*dt/(Dim.L*Spg*10.0))/(1+kL*byte((BrakeCtrlPosNo>0)and(Power>1)and(BrakeSystem=Pneumatic)));  //nieszczelnosci

//yB: jednokrokowe liczenie tego wszystkiego
     PipePress:=PipePress+dpPipe+dpMainValve;


//     if 0.5<PipePress then
//        PipePress:=PipePress-(PipePress-0.5)*dt*10;
     if PipePress<0 then PipePress:=0;
{        if PipePress>NominalPipePress then PipePress:=NominalPipePress; } {pomyslec nad tym}
   end; {PipePress}

(************* yB: ROZDZIELACZ POWIETRZA *******************************)
  if(BrakeDelayFlag=bdelay_R)then
   if(BrakeCtrlPosNo>0)then
     if(Vel>55)then
       SetFlag(BrakeStatus,+b_Rused+b_Ractive)
     else
       SetFlag(BrakeStatus,-b_Ractive)
   else
     if(Vel>70)then
       SetFlag(BrakeStatus,+b_Rused+b_Ractive)
     else if(Vel<50)then
       SetFlag(BrakeStatus,-b_Ractive);

{ //przyspieszacz wlaczany inaczej :)
  if ((PPP-PipePress)/dt>0.002) then
//   if not(TestFlag(BrakeStatus,b_on)) then
     if(ABS(HighPipePress-PPP)<0.000001)then
      if(BrakeCtrlPosNo=0)then
       begin
         SetFlag(SoundFlag,+sound_brakeacc);
//         PipePress:=PipePress*(Dim.L*Spg/(Dim.L*Spg+0.5));
         PipeBrakePress:=0.0045;
         HighPipePress:=HighPipePress+0.0007;
       end;    }


//  if((PPP-PipePress)/dt<0.001)then
//    SetFlag(BrakeStatus,-b_on);     //nie napelniaj
(*  if((PPP-PipePress)/dt>0.0029)then  //przyspieszacz
    if not(TestFlag(BrakeStatus,b_on))then
     if(BrakeCtrlPosNo=0)then
      begin
        SetFlag(SoundFlag,sound_brakeacc);
        PipePress:=PipePress*(Dim.L*Spg/(Dim.L*Spg+1));
      end;*)
//  if((PPP-PipePress)/dt>0.003)then
//    SetFlag(BrakeStatus,+b_on);     //napelniaj


    //  else //if (BrakeStatus=b_off) or (BrakeStatus=b_release) then
  if(TestFlag(BrakeStatus,b_on))then
    case BrakeSubsystem of
     Oerlikon, Hik, KE:
      if ((PipePress>HighPipePress) and (PPP>HighPipePress))or(PipePress>CntrlPipePress) then
       begin
        SetFlag(BrakeStatus,-b_on);
        HighPipePress:=0.00002;
        LowPipePress:= 0.00001;
       end
      else
     else
      if(BrakeVP<PipePress-0.01)then
       begin
        BrakeStatus:=(BrakeStatus and 254);
        HighPipePress:=0.00002;
        LowPipePress:= 0.00001;
       end
      else
       if(dpBrake<0)then
        BrakePress:=BrakePress-dpBrake;
    end

(*    if(ABS(HighPipePress-PPP)<0.000001)then
     if(BrakeCtrlPosNo=0)then
      begin
        SetFlag(SoundFlag,sound_brakeacc);
        PipePress:=PipePress*(Dim.L*Spg/(Dim.L*Spg+0.5));
      end; *)
  else
   begin
    if PipeBrakePress<0.0002 then
      case BrakeSubsystem of
        Hik, KE:
         begin
          if CntrlPipePress>PipePress then
           CntrlPipePress:=CntrlPipePress-PR(PipePress,CntrlPipePress)*dt/50.0;
           CntrlPipePress:=Max0R(CntrlPipePress,BrakeVP);
         end;
        Oerlikon:
         begin
          if CntrlPipePress>PipePress then
           CntrlPipePress:=CntrlPipePress-PR(PipePress,CntrlPipePress)*dt/50.0
          else
           if (Power<1)and(BrakeSystem=Pneumatic) then
             CntrlPipePress:=Max0R(CntrlPipePress,BrakeVP)
           else
         begin
             if (CabNo<>0) and (BrakeSystem=Pneumatic) then
              begin
               CntrlPipePress:=PipePress;
               if (CntrlPipePress>BrakePressureTable[0].PipePressureVal) then
                 CntrlPipePress:=BrakePressureTable[0].PipePressureVal
              end
             else
               CntrlPipePress:=CntrlPipePress+SPR(PipePress,CntrlPipePress)*dt/50.0;
            end;
         end
        else
         CntrlPipePress:=BrakeVP;
      end;

//        HighPipePress:=0;
//        LowPipePress:=0;

    if (CntrlPipePress-0.03+0.015*ord((BrakeSubsystem=Oerlikon)or(BrakeSubsystem=KE))>PipePress)and(CntrlPipePress-0.03+0.015*ord((BrakeSubsystem=Oerlikon)or(BrakeSubsystem=KE))>PPP) then
      begin
        SetFlag(BrakeStatus,b_on);
        case BrakeSubsystem of
        Oerlikon, Hik, KE:
         begin
          HighPipePress:=PipePress;
          LowPipePress:=PipePress-deltaPipePress;
         end
        else
      begin
          HighPipePress:=BrakeVP;
          LowPipePress:=Volume/(10.0*(BrakeVVolume+BrakeVolume));
          deltaPipePRess:=HighPipePress-LowPipePress;
         end;
        end;
//        HighPipePress:=PipePress;
//        LowPipePress:=PipePress-deltaPipePress;
        if(BrakeCtrlPosNo=0)and(PipeBrakePress<0.002)and(BrakeVP>(CntrlPipePress-0.005))then
         begin
          SetFlag(SoundFlag,+sound_brakeacc);
          PipePress:=PipePress*(Dim.L*Spg/(Dim.L*Spg+0.5));
         end;
      end;


(*    PipeBrakePress:=0;
       if(BrakeSubSystem=Oerlikon)and(HighPipePress>0.48)then
         HighPipePress:=Min0R(PipePress,HighPipePress+dt/500)
       else
         HighPipePress:=Min0R(PipePress,HighPipePress+dt/250);
       LowPipePress:=HighPipePress-deltaPipePress;
//     if BrakeStatus<>b_release then
//       BrakeStatus:=b_off;
     end
    else if (PipePress-PPP)<0 then
     begin
       HighPipePress:=HighPipePress-PPP+PipePress;
       LowPipePress:=HighPipePress-deltaPipePress;
     end;
//    if (BrakeStatus<>b_release) and ((PipePress-PPP)/dt>0.003) then
//      BrakeStatus:=b_off;

//    else *)
   end;
//  else
{   if(ABS(HighPipePress-PPP)<0.000001)then
    if(BrakeCtrlPosNo=0)then
     begin
       SetFlag(SoundFlag,sound_brakeacc);
       PipePress:=PipePress*(Dim.L*Spg/(Dim.L*Spg+0.5));
     end;
}
//  if (PPP-PipePress)<0 then
//    Brakestatus:=b_off;
(************* yB: ROZDZIELACZ POWIETRZA - KONIEC **********************)
//yB: na chwile zakomentuje, zobaczymy, czy sie na tym sypalo
//  if(TrainType=dt_ET42)and(DynamicBrakeFlag)then
//    if(PipePress<0.3)then ScndCtrlActualPos:=255
//    else if(PipePress>0.45)and(ScndCtrlActualPos=255)then ScndCtrlActualPos:=0;

  if CompressedVolume<0 then
   CompressedVolume:=0;

  PPP:=PipePress;
end;  {updatepipepressure}


procedure TMoverParameters.CompressorCheck(dt:real);
 begin
   if VeselVolume>0 then
    begin
     if MaxCompressor-MinCompressor<0.0001 then
      begin
{        if Mains and (MainCtrlPos>1) then}
        if CompressorAllow and Mains and (MainCtrlPos>0) then
         begin
           if (Compressor<MaxCompressor) then
            if (EngineType=DieselElectric) and (CompressorPower>0) then
             CompressedVolume:=CompressedVolume+dt*CompressorSpeed*(2*MaxCompressor-Compressor)/MaxCompressor*(DEList[MainCtrlPos].rpm/DEList[MainCtrlPosNo].rpm)
            else
             CompressedVolume:=CompressedVolume+dt*CompressorSpeed*(2*MaxCompressor-Compressor)/MaxCompressor
           else
            begin
              CompressedVolume:=CompressedVolume*0.8;
              SetFlag(SoundFlag,sound_relay);
              SetFlag(SoundFlag,sound_loud);
            end;
         end;
      end
     else
      begin
 {        if (not CompressorFlag) and (Compressor<MinCompressor) and Mains and (LastSwitchingTime>CtrlDelay) then}
         if (not CompressorFlag) and (Compressor<MinCompressor) and Mains and (CompressorAllow) and ((ConverterFlag) or (CompressorPower=0)) and (LastSwitchingTime>CtrlDelay) then
          begin
            CompressorFlag:=True;
            LastSwitchingTime:=0;
          end;
         if (Compressor>MaxCompressor) or not CompressorAllow or ((CompressorPower<>0) and (not ConverterFlag)) or not Mains then
         CompressorFlag:=False;
         if CompressorFlag then
          if (EngineType=DieselElectric) and (CompressorPower>0) then
           CompressedVolume:=CompressedVolume+dt*CompressorSpeed*(2*MaxCompressor-Compressor)/MaxCompressor*(DEList[MainCtrlPos].rpm/DEList[MainCtrlPosNo].rpm)
          else
           CompressedVolume:=CompressedVolume+dt*CompressorSpeed*(2*MaxCompressor-Compressor)/MaxCompressor;
      end;
    end;
 end;

procedure TMoverParameters.UpdatePantVolume(dt:real);
 begin
   if PantCompFlag=true then
    begin
     PantVolume:=PantVolume+dt/30.0;
    end;
 {Winger - ZROBIC}
end;


procedure TMoverParameters.ConverterCheck;       {sprawdzanie przetwornicy}
begin
if (ConverterAllow=true)and(Mains=true) then
 ConverterFlag:=true
 else
 ConverterFlag:=false;
end;

//youBy - przewod zasilajacy
procedure TMoverParameters.UpdateScndPipePressure(dt: real);
const Spz=0.5067;
var USPP, dpSPipe,dpSpipeBV: real;
    c: PMoverParameters;
begin

  if (VeselVolume>0) then
    USPP:=-sign(ScndPipePress-Compressor)*10*PR(Compressor,ScndPipePress)*dt
  else
    USPP:=0;

  ScndPipePress:=ScndPipePress+USPP;
  CompressedVolume:=CompressedVolume-USPP*Dim.L*Spz/100.0;

  dpSpipe:=0;

  if Couplers[0].Connected<>nil then
   if TestFlag(Couplers[0].CouplingFlag,ctrain_scndpneumatic) then
    begin
      c:=Couplers[0].Connected; //skrot
      USPP:=PR(ScndPipePress,c^.ScndPipePress)*240.0*dt*sign(ScndPipePress-c^.ScndPipePress);
      c^.ScndPipePress:=c^.ScndPipePress+USPP/(10.0*c^.Dim.L);
      if c^.ScndPipePress<0 then begin USPP:=ScndPipePress*10*c^.Dim.L*Spz; c^.ScndPipePress:=0;end;
      dpSpipe:=dpSpipe-USPP/(10.0*Dim.L);
      if not c^.PhysicActivation then
       if Abs(USPP)>0.000000001 then
        c^.Physic_ReActivation;  {aktywacja fizyki podlaczonego skladu}
    end;
  if Couplers[1].Connected<>nil then
   if TestFlag(Couplers[1].CouplingFlag,ctrain_scndpneumatic) then
     begin
       c:=Couplers[1].Connected; //skrot
       USPP:=PR(ScndPipePress,c^.ScndPipePress)*240.0*dt*sign(ScndPipePress-c^.ScndPipePress);
       c^.ScndPipePress:=c^.ScndPipePress+USPP/(10.0*c^.Dim.L);
       if c^.ScndPipePress<0 then begin USPP:=ScndPipePress*10*c^.Dim.L*Spz; c^.ScndPipePress:=0;end;
       dpSpipe:=dpSpipe-USPP/(10.0*Dim.L);
       if not c^.PhysicActivation then
        if Abs(USPP)>0.000000001 then
         c^.Physic_ReActivation;  {aktywacja fizyki podlaczonego skladu}
      end;

  dpSpipeBV:=0;

  if (((Power>1)or(BrakeSystem=Electropneumatic))and(BrakeSubsystem=Oerlikon)) then
    if(BrakeVP<ScndPipePress)then
     begin
      dpSpipeBV:=sign(BrakeVP-ScndPipePress)*PR(ScndPipePress,BrakeVP)*(Dim.L*Spz)/(Dim.L*Spz+BrakeVVolume)*dt/50.0;
      Volume:=Volume-dpSpipeBV*BrakeVVolume*10.0;
      dpSpipeBV:=(dpSpipeBV*1000.0*BrakeVVolume)/(Spz*Dim.L);
     end;

  ScndPipePress:=ScndPipePress+dpSpipe+dpSpipeBV;
  if ScndPipePress<0 then ScndPipePress:=0

end;


{sprzegi}

function TMoverParameters.Attach(ConnectNo:byte;ConnectToNr:byte;ConnectTo:PMoverParameters;CouplingType:byte):boolean;
//³¹czenie do (ConnectNo) pojazdu (ConnectTo) stron¹ (ConnectToNr)
//Ra: zwykle wykonywane dwukrotnie, dla ka¿dego pojazdu oddzielnie
const dEpsilon=0.001;
var ct:TCouplerType;
begin
 with Couplers[ConnectNo] do
  begin
   if (ConnectTo<>nil) then
    begin
     if (ConnectToNr<>2) then ConnectedNr:=ConnectToNr; {2=nic nie pod³¹czone}
     ct:=ConnectTo^.Couplers[ConnectedNr].CouplerType; //typ sprzêgu pod³¹czanego pojazdu
     CoupleDist:=Distance(Loc,ConnectTo^.Loc,Dim,ConnectTo^.Dim); //odleg³oœæ pomiêdzy sprzêgami
     if (((CoupleDist<=dEpsilon) and (CouplerType<>NoCoupler) and (CouplerType=ct))
        or (CouplingType and ctrain_coupler=0))
     then
      begin  {stykaja sie zderzaki i kompatybilne typy sprzegow chyba ze wirtualnie}
        Connected:=ConnectTo;
        if CouplingFlag=ctrain_virtual then //jeœli wczeœniej nie by³o po³¹czone
         begin //ustalenie z której strony rysowaæ sprzêg
          Render:=True; //tego rysowaæ
          Connected.Couplers[ConnectedNr].Render:=false; //a tego nie
         end;
        CouplingFlag:=CouplingType;
        if (CouplingType<>ctrain_virtual) //Ra: wirtualnego nie ³¹czymy zwrotnie!
        then //jeœli ³¹czenie sprzêgiem niewirtualnym, ustawiamy po³¹czenie zwrotne
         Connected.Couplers[ConnectedNr].CouplingFlag:=CouplingType;
        Attach:=True;
      end
     else
      Attach:=False;
    end
   else
    Attach:=False;
  end;
end;

function TMoverParameters.DettachDistance(ConnectNo: byte): boolean;
//Ra: sprawdzenie, czy odleg³oœæ jest dobra do roz³¹czania
begin
 with Couplers[ConnectNo] do
  if (Connected<>nil) and
  {ABu021104: zakomentowane 'and (CouplerType<>Articulated)' w warunku, nie wiem co to bylo, ale za to teraz dziala odczepianie... :) }
  (((Distance(Loc,Connected^.Loc,Dim,Connected^.Dim)>0) {and (CouplerType<>Articulated)}) or
   (TestFlag(DamageFlag,dtrain_coupling) or (CouplingFlag and ctrain_coupler=0)))
   then
    DettachDistance:=FALSE
   else
    DettachDistance:=TRUE;
end;

function TMoverParameters.Dettach(ConnectNo: byte): boolean; {rozlaczanie}
begin
 with Couplers[ConnectNo] do
  begin
   if (Connected<>nil) and
   {ABu021104: zakomentowane 'and (CouplerType<>Articulated)' w warunku, nie wiem co to bylo, ale za to teraz dziala odczepianie... :) }
   (((Distance(Loc,Connected^.Loc,Dim,Connected^.Dim)<=0) {and (CouplerType<>Articulated)}) or
    (TestFlag(DamageFlag,dtrain_coupling) or (CouplingFlag and ctrain_coupler=0)))
    then
     begin  {gdy podlaczony oraz scisniete zderzaki chyba ze zerwany sprzeg albo tylko wirtualnie}
{       Connected:=nil;  } {lepiej zostawic bo przeciez trzeba kontrolowac zderzenia odczepionych}
       CouplingFlag:=0; //pozostaje sprzêg wirtualny
       Connected.Couplers[ConnectedNr].CouplingFlag:=0; //pozostaje sprzêg wirtualny
       Dettach:=True;
     end
   else
     begin //od³¹czamy wê¿e i resztê, pozostaje sprzêg fizyczny
       CouplingFlag:=CouplingFlag and ctrain_coupler;
       Connected.Couplers[ConnectedNr].CouplingFlag:=CouplingFlag;
       Dettach:=False; //jeszcze nie roz³¹czony
     end
  end;
end;

{lokomotywy}

function TMoverParameters.ComputeRotatingWheel(WForce,dt,n:real): real;
var newn,eps:real;
begin
  if (n=0) and (WForce*Sign(V)<0) then
   newn:=0
  else
   begin
     eps:=WForce*WheelDiameter/(2.0*AxleInertialMoment);
     newn:=n+eps*dt;
     if (newn*n<=0) and (eps*n<0) then
      newn:=0;
   end;
  ComputeRotatingWheel:=newn;
end;

{----------------------}
{LOKOMOTYWA ELEKTRYCZNA}

function TMoverParameters.FuseFlagCheck: boolean;
var b:byte;
begin
 FuseFlagCheck:=false;
 if Power>0.01 then FuseFlagCheck:=FuseFlag
 else               {pobor pradu jezeli niema mocy}
  for b:=0 to 1 do
   with Couplers[b] do
    if TestFlag(CouplingFlag,ctrain_controll) then
     if Connected^.Power>0.01 then
      FuseFlagCheck:=Connected^.FuseFlagCheck();
end;

function TMoverParameters.FuseOn: boolean;
begin
 FuseOn:=False;
 if (MainCtrlPos=0) and (ScndCtrlPos=0) and Mains then
  begin
   SendCtrlToNext('FuseSwitch',1,CabNo);
   if (EngineType=ElectricSeriesMotor) and FuseFlag then
    begin
     FuseFlag:=False;  {wlaczenie ponowne obwodu}
     FuseOn:=True;
     SetFlag(SoundFlag,sound_relay); SetFlag(SoundFlag,sound_loud);
    end;
  end;
end;

procedure TMoverParameters.FuseOff;
begin
 if not FuseFlag then
  begin
   FuseFlag:=True;
   EventFlag:=True;
   SetFlag(SoundFlag,sound_relay);
   SetFlag(SoundFlag,sound_loud);
  end;
end;


function TMoverParameters.ShowCurrent(AmpN:byte): integer;
var b,Bn:byte;
begin
  ClearPendingExceptions;
  ShowCurrent:=0;
  Bn:=RList[MainCtrlActualPos].Bn;
  if (DynamicBrakeType=dbrake_automatic) and (DynamicBrakeFlag) then
   Bn:=2;
  if Power>0.01 then
   begin
     if AmpN>0 then
      begin
       if Bn>=AmpN then
        ShowCurrent:=Trunc(Abs(Im));
      end
     else
       ShowCurrent:=Trunc(Abs(Itot));
    end
  else               {pobor pradu jezeli niema mocy}
   for b:=0 to 1 do
    with Couplers[b] do
     if TestFlag(CouplingFlag,ctrain_controll) then
      if Connected^.Power>0.01 then
       ShowCurrent:=Connected^.ShowCurrent(AmpN);
end;

function TMoverParameters.ShowEngineRotation(VehN:byte): integer;
var b:Byte; //,Bn:byte;
begin
  ClearPendingExceptions;
  ShowEngineRotation:=0;
  case VehN of
   1: ShowEngineRotation:=Trunc(Abs(enrot));
   2: for b:=0 to 1 do
       with Couplers[b] do
        if TestFlag(CouplingFlag,ctrain_controll) then
         if Connected^.Power>0.01 then
          ShowEngineRotation:=Trunc(Abs(Connected^.enrot));
   3: if Couplers[1].Connected<>nil then
       if TestFlag(Couplers[1].CouplingFlag,ctrain_controll) then
        if Couplers[1].Connected^.Couplers[1].Connected<>nil then
         if TestFlag(Couplers[1].Connected^.Couplers[1].CouplingFlag,ctrain_controll) then
          if Couplers[1].Connected^.Couplers[1].Connected^.Power>0.01 then
           ShowEngineRotation:=Trunc(Abs(Couplers[1].Connected^.Couplers[1].Connected^.enrot));
  end;
end;


{funkcje uzalezniajace sile pociagowa od predkosci: V2n, n2R, Current, Momentum}
{----------------}

function TMoverParameters.V2n:real;
{przelicza predkosc liniowa na obrotowa}
const dmgn=0.5;
var n,deltan:real;
begin
  n:=V/(Pi*WheelDiameter); //predkosc obrotowa wynikajaca z liniowej [obr/s]
  deltan:=n-nrot; //"pochodna" prêdkoœci obrotowej
  if SlippingWheels then
   if Abs(deltan)<0.1 then
    SlippingWheels:=False; {wygaszenie poslizgu}
  if SlippingWheels then   {nie ma zwiazku z predkoscia liniowa V}
   begin                   {McZapkie-221103: uszkodzenia kol podczas poslizgu}
     if deltan>dmgn then
      if FuzzyLogic(deltan,dmgn,p_slippdmg) then
        if SetFlag(DamageFlag,dtrain_wheelwear)   {podkucie}
          then EventFlag:=True;
     if deltan<-dmgn then
      if FuzzyLogic(-deltan,dmgn,p_slippdmg) then
        if SetFlag(DamageFlag,dtrain_thinwheel)  {wycieranie sie obreczy}
          then EventFlag:=True;
     n:=nrot;               {predkosc obrotowa nie zalezy od predkosci liniowej}
   end;
  V2n:=n;
end;

function TMoverParameters.Current(n,U:real): real;
{wazna funkcja - liczy prad plynacy przez silniki polaczone szeregowo lub rownolegle}
{w zaleznosci od polozenia nastawnikow MainCtrl i ScndCtrl oraz predkosci obrotowej n}
{a takze wywala bezpiecznik nadmiarowy gdy za duzy prad lub za male napiecie}
{jest takze mozliwosc uszkodzenia silnika wskutek nietypowych parametrow}
const ep09resED=5.8; {TODO: dobrac tak aby sie zgadzalo ze wbudzeniem}
var //Rw,
    R,MotorCurrent:real;
    Rz,Delta,Isf:real;
    Mn: integer;
    SP: byte;
begin
  MotorCurrent:=0;
  ResistorsFlag:=(RList[MainCtrlActualPos].R>0.01) and (not DelayCtrlFlag);
  ResistorsFlag:=ResistorsFlag or ((DynamicBrakeFlag=true) and (DynamicBrakeType=dbrake_automatic));
  R:=RList[MainCtrlActualPos].R+CircuitRes;
  Mn:=RList[MainCtrlActualPos].Mn;
  if DynamicBrakeFlag and (not FuseFlag) and (DynamicBrakeType=dbrake_automatic) and Mains then     {hamowanie EP09}
   with MotorParam[ScndCtrlPosNo] do
     begin
       MotorCurrent:=-fi*n/ep09resED; {TODO: zrobic bardziej uniwersalne nie tylko dla EP09}
       if abs(MotorCurrent)>500 then
        MotorCurrent:=sign(MotorCurrent)*500 {TODO: zrobic bardziej finezyjnie}
        else
         if abs(MotorCurrent)<40 then
          MotorCurrent:=0;
     end
  else
  if (RList[MainCtrlActualPos].Bn=0) or FuseFlag or StLinFlag or DelayCtrlFlag then
    MotorCurrent:=0                    {wylaczone}
  else                                 {wlaczone}
   begin
   SP:=ScndCtrlActualPos;
   if(ScndCtrlActualPos<255)then  {tak smiesznie bede wylaczal }
    begin
     if(ScndInMain)then
      if not (Rlist[MainCtrlActualPos].ScndAct=255) then
       SP:=Rlist[MainCtrlActualPos].ScndAct;
      with MotorParam[SP] do
       begin
        Rz:=Mn*WindingRes+R;
        if DynamicBrakeFlag then     {hamowanie}
         begin
           if DynamicBrakeType>1 then
            begin
              if DynamicBrakeType<>dbrake_automatic then
               MotorCurrent:=-fi*n/Rz  {hamowanie silnikiem na oporach rozruchowych}
(*               begin
                 U:=0;
                 Isf:=Isat;
                 Delta:=SQR(Isf*Rz+Mn*fi*n-U)+4*U*Isf*Rz;
                 MotorCurrent:=(U-Isf*Rz-Mn*fi*n+SQRT(Delta))/(2*Rz)
               end*)
            end
           else
             MotorCurrent:=0;        {odciecie pradu od silnika}
         end
        else
         begin
          Isf:=Sign(U)*Isat;
          Delta:=SQR(Isf*Rz+Mn*fi*n-U)+4*U*Isf*Rz;
          if Mains then
           begin
            if U>0 then
             MotorCurrent:=(U-Isf*Rz-Mn*fi*n+SQRT(Delta))/(2.0*Rz)
            else
             MotorCurrent:=(U-Isf*Rz-Mn*fi*n-SQRT(Delta))/(2.0*Rz)
           end
          else
           MotorCurrent:=0;
         end;{else DBF}
       end;{with}
    end{255}
   else
    MotorCurrent:=0;
   end;
{  if Abs(CabNo)<2 then Im:=MotorCurrent*ActiveDir*CabNo
   else Im:=0;
}
  Im:=MotorCurrent;
  Current:=Im; {prad brany do liczenia sily trakcyjnej}
{  EnginePower:=Im*Im*RList[MainCtrlActualPos].Bn*RList[MainCtrlActualPos].Mn*WindingRes;}
  EnginePower:=Abs(Itot)*(1+RList[MainCtrlActualPos].Mn)*Abs(U);

  {awarie}
  MotorCurrent:=Abs(Im); {zmienna pomocnicza}
  if Motorcurrent>0 then
  begin
    if FuzzyLogic(Abs(n),nmax*1.1,p_elengproblem) then
     if MainSwitch(False) then
       EventFlag:=True;                                      {zbyt duze obroty - wywalanie wskutek ognia okreznego}
    if TestFlag(DamageFlag,dtrain_engine) then
     if FuzzyLogic(MotorCurrent,ImaxLo/10.0,p_elengproblem) then
      if MainSwitch(False) then
       EventFlag:=True;                                      {uszkodzony silnik (uplywy)}
    if (FuzzyLogic(Abs(Im),Imax*2,p_elengproblem) or FuzzyLogic(Abs(n),nmax*1.11,p_elengproblem)) then
{       or FuzzyLogic(Abs(U/Mn),2*NominalVoltage,1)) then } {poprawic potem}
     if (SetFlag(DamageFlag,dtrain_engine)) then
       EventFlag:=True;
  {! dorobic grzanie oporow rozruchowych i silnika}
  end;
end;

function TMoverParameters.Momentum(I:real): real;
{liczy moment sily wytwarzany przez silnik elektryczny}
var SP: byte;
begin
   SP:=ScndCtrlActualPos;
   if(ScndInMain)then
    if not (Rlist[MainCtrlActualPos].ScndAct=255) then
     SP:=Rlist[MainCtrlActualPos].ScndAct;
    with MotorParam[SP] do
     Momentum:=mfi*I*(1-1.0/(Abs(I)/mIsat+1));
end;

function TMoverParameters.dizel_Momentum(dizel_fill,n,dt:real): real;
{liczy moment sily wytwarzany przez silnik spalinowy}
var Moment, enMoment, eps, newn, friction: real;
begin
{  friction:=dizel_engagefriction*(11-2*random)/10; }
  friction:=dizel_engagefriction;
  if enrot>0 then
   begin
     Moment:=dizel_Mmax*dizel_fill-(dizel_Mmax-dizel_Mnmax*dizel_fill)*sqr(enrot/(dizel_nmax-dizel_nMmax*dizel_fill))-dizel_Mstand;
{     Moment:=Moment*(1+sin(eAngle*4))-dizel_Mstand*(1+cos(eAngle*4));}
   end
   else Moment:=-dizel_Mstand;
  if enrot<dizel_nmin/10.0 then
   if (eAngle<Pi/2.0) then Moment:=Moment-dizel_Mstand; {wstrzymywanie przy malych obrotach}
  //!! abs
  if abs(abs(n)-enrot)<0.1 then
   begin
    if (Moment)>(dizel_engageMaxForce*dizel_engage*dizel_engageDia*friction*2) then {zerwanie przyczepnosci sprzegla}
     enrot:=enrot+dt*Moment/dizel_AIM
    else
     begin
       dizel_engagedeltaomega:=0;
       enrot:=abs(n);                          {jest przyczepnosc tarcz}
     end
   end
  else
   begin
     if (enrot=0) and (Moment<0) then
      newn:=0
     else
      begin
         //!! abs
         dizel_engagedeltaomega:=enrot-n;     {sliganie tarcz}
         enMoment:=Moment-sign(dizel_engagedeltaomega)*dizel_engageMaxForce*dizel_engage*dizel_engageDia*friction;
         Moment:=sign(dizel_engagedeltaomega)*dizel_engageMaxForce*dizel_engage*dizel_engageDia*friction;
         dizel_engagedeltaomega:=abs(enrot-abs(n));
         eps:=enMoment/dizel_AIM;
         newn:=enrot+eps*dt;
         if (newn*enrot<=0) and (eps*enrot<0) then
          newn:=0;
      end;
     enrot:=newn;
   end;
  dizel_Momentum:=Moment;
  if (enrot=0) and (not dizel_enginestart) then
   Mains:=False;
end;

function TMoverParameters.CutOffEngine: boolean; {wylacza uszkodzony silnik}
begin
  if (NPoweredAxles>0) and (CabNo=0) and (EngineType=ElectricSeriesMotor) then
   begin
     if SetFlag(DamageFlag,-dtrain_engine) then
      begin
        NPoweredAxles:=NPoweredAxles div 2;
        CutOffEngine:=True;
      end;
   end
  else
   CutOffEngine:=False;
end;

{przelacznik pradu wysokiego rozruchu}
function TMoverParameters.MaxCurrentSwitch(State:boolean):boolean;
begin
  MaxCurrentSwitch:=False;
  if (EngineType=ElectricSeriesMotor) then
   if (ImaxHi>ImaxLo) then
   begin
     if State and (Imax=ImaxLo) and (RList[MainCtrlPos].Bn<2) and not ((TrainType=dt_ET42)and(MainctrlPos>0)) then
      begin
        Imax:=ImaxHi;
        MaxCurrentSwitch:=True;
        if CabNo<>0 then
         SendCtrlToNext('MaxCurrentSwitch',1,CabNo);
      end;
     if (not State) then
     if (Imax=ImaxHi) then
     if not ((TrainType=dt_ET42)and(MainctrlPos>0)) then
      begin
        Imax:=ImaxLo;
        MaxCurrentSwitch:=True;
        if CabNo<>0 then
         SendCtrlToNext('MaxCurrentSwitch',0,CabNo);
      end;
   end;
end;


{przelacznik pradu automatycznego rozruchu}
function TMoverParameters.MinCurrentSwitch(State:boolean):boolean;
begin
  MinCurrentSwitch:=False;
  if ((EngineType=ElectricSeriesMotor) and (IminHi>IminLo)) or (TrainType=dt_EZT) then
   begin
     if State and (Imin=IminLo) then
      begin
        Imin:=IminHi;
        MinCurrentSwitch:=True;
        if CabNo<>0 then
         SendCtrlToNext('MinCurrentSwitch',1,CabNo);
      end;
     if (not State) and (Imin=IminHi) then
      begin
        Imin:=IminLo;
        MinCurrentSwitch:=True;
        if CabNo<>0 then
         SendCtrlToNext('MinCurrentSwitch',0,CabNo);
      end;
   end;
end;


{reczne przelaczanie samoczynnego rozruchu}
function TMoverParameters.AutoRelaySwitch(State:boolean):boolean;
 begin
   if (AutoRelayType=2) and (AutoRelayFlag<>State) then
    begin
      AutoRelayFlag:=State;
      AutoRelaySwitch:=True;
      SendCtrlToNext('AutoRelaySwitch',ord(State),CabNo);
    end
   else
    AutoRelaySwitch:=False;
 end;

function TMoverParameters.AutoRelayCheck: boolean;
var OK:boolean; //b:byte;
begin
//  if ((TrainType=dt_EZT{) or (TrainType=dt_ET22)}) and (Imin=IminLo)) or ((ActiveDir<0) and (TrainType<>dt_PseudoDiesel')) then
//     if Rlist[MainCtrlActualPos].Bn>1 then
//      begin
//        dec(MainCtrlActualPos);
//        AutoRelayCheck:=False;
//        Exit;
//      end;
//yB: wychodzenie przy odcietym pradzie
  if (ScndCtrlActualPos=255)then
   begin
    AutoRelayCheck:=False;
    MainCtrlActualPos:=0;
   end
  else
  if (not Mains) or (FuseFlag) or (StLinFlag) then   //hunter-111211: wylacznik cisnieniowy
   begin
     AutoRelayCheck:=False;
     MainCtrlActualPos:=0;
     ScndCtrlActualPos:=0;
   end
  else
   begin
    OK:=False;
    if DelayCtrlFlag and (MainCtrlPos=1) and (MainCtrlActualPos=1) and (LastRelayTime>InitialCtrlDelay) then
     begin
       DelayCtrlFlag:=False;
       SetFlag(SoundFlag,sound_relay); SetFlag(SoundFlag,sound_loud);
     end;
    if (LastRelayTime>CtrlDelay) and not DelayCtrlFlag then
     begin
       if MainCtrlPos=0 then
        DelayCtrlFlag:=True;
       if (((RList[MainCtrlActualPos].R=0) and ((not CoupledCtrl) or (Imin=IminLo))) or (MainCtrlActualPos=RListSize))
          and ((ScndCtrlActualPos>0) or (ScndCtrlPos>0)) then
        begin   {zmieniaj scndctrlactualpos}
          if (not AutoRelayFlag) or (not MotorParam[ScndCtrlActualPos].AutoSwitch) then
           begin                                                {scnd bez samoczynnego rozruchu}
             OK:=True;
             if (ScndCtrlActualPos<ScndCtrlPos) then
              inc(ScndCtrlActualPos)
             else
              if ScndCtrlActualPos>ScndCtrlPos then
               dec(ScndCtrlActualPos)
             else OK:=False;
           end
          else
           begin                                                {scnd z samoczynnym rozruchem}
             if ScndCtrlPos<ScndCtrlActualPos then
              begin
                dec(ScndCtrlActualPos);
                OK:=True;
              end
             else
              if (ScndCtrlPos>ScndCtrlActualPos) then
               if Abs(Im)<Imin then
                if MotorParam[ScndCtrlActualPos].AutoSwitch then
                 begin
                   inc(ScndCtrlActualPos);
                   OK:=True
                 end;
           end;
        end
       else
        begin          {zmieniaj mainctrlactualpos}
          if ((TrainType=dt_EZT{) or (TrainType=dt_ET22)}) and (Imin=IminLo)) or ((ActiveDir<0) and (TrainType<>dt_PseudoDiesel)) then
           if Rlist[MainCtrlActualPos+1].Bn>1 then
            begin
              AutoRelayCheck:=False;
              Exit;
           end;
          if (not AutoRelayFlag) or (not RList[MainCtrlActualPos].AutoSwitch) then
           begin                                                {main bez samoczynnego rozruchu}
             OK:=True;
             if Rlist[MainCtrlActualPos].Relay<MainCtrlPos then
              begin
                inc(MainCtrlActualPos);
                //---------
                //hunter-111211: poprawki
                if MainCtrlActualPos>0 then
                 if (Rlist[MainCtrlActualPos].R=0) and (not (MainCtrlActualPos=MainCtrlPosNo)) then  //wejscie na bezoporowa
                  begin
                   SetFlag(SoundFlag,sound_manyrelay); SetFlag(SoundFlag,sound_loud);
                  end
                 else if (Rlist[MainCtrlActualPos].R>0) and (Rlist[MainCtrlActualPos-1].R=0) then //wejscie na drugi uklad
                  begin
                   SetFlag(SoundFlag,sound_manyrelay);
                  end;
              end
             else if Rlist[MainCtrlActualPos].Relay>MainCtrlPos then
              begin
                dec(MainCtrlActualPos);
                if MainCtrlActualPos>0 then  //hunter-111211: poprawki
                 if Rlist[MainCtrlActualPos].R=0 then  {dzwieki schodzenia z bezoporowej}
                  begin
                   SetFlag(SoundFlag,sound_manyrelay);
                  end;
              end
             else
              if (Rlist[MainCtrlActualPos].R>0) and (ScndCtrlActualPos>0) then
               Dec(ScndCtrlActualPos) {boczniki nie dzialaja na poz. oporowych}
              else
               OK:=False;
           end
          else                                                  {main z samoczynnym rozruchem}
           begin
             OK:=False;
             if MainCtrlPos<Rlist[MainCtrlActualPos].Relay then
              begin
                dec(MainCtrlActualPos);
                OK:=True;
              end
             else
              if (MainCtrlPos>Rlist[MainCtrlActualPos].Relay)
                  or ((MainCtrlActualPos<RListSize) and (MainCtrlPos=Rlist[MainCtrlActualPos+1].Relay)) then
               if Abs(Im)<Imin then
                 begin
                   inc(MainCtrlActualPos);
                   OK:=True
                 end;
           end;
        end;
     end
    else  {DelayCtrlFlag}
     if ((MainCtrlPos>1) and (MainCtrlActualPos>0) and DelayCtrlFlag) then
      begin
        MainCtrlActualPos:=0;
        OK:=True;
      end
     else
      if (MainCtrlPos=1) and (MainCtrlActualPos=0) then
       MainCtrlActualPos:=1
      else
       if (MainCtrlPos=0) and (MainCtrlActualPos>0) then
        begin
         dec(MainCtrlActualPos);
         OK:=true;
        end;

    if OK then LastRelayTime:=0;
    AutoRelayCheck:=OK;
  end;
end;

function TMoverParameters.ResistorsFlagCheck:boolean;  {sprawdzanie wskaznika oporow}
var b:byte;
begin
  ResistorsFlagCheck:=false;
  if Power>0.01 then ResistorsFlagCheck:=ResistorsFlag
  else               {pobor pradu jezeli niema mocy}
   for b:=0 to 1 do
    with Couplers[b] do
     if TestFlag(CouplingFlag,ctrain_controll) then
      if Connected^.Power>0.01 then
       ResistorsFlagCheck:=Connected^.ResistorsFlagCheck();
end;

function TMoverParameters.dizel_EngageSwitch(state: real): boolean;
{zmienia parametr do ktorego dazy sprzeglo}
begin
 if (EngineType=DieselEngine) and (state<=1) and (state>=0) and (state<>dizel_engagestate) then
  begin
    dizel_engagestate:=state;
    dizel_EngageSwitch:=True
  end
 else
  dizel_EngageSwitch:=False;
end;

function TMoverParameters.dizel_EngageChange(dt: real): boolean;
{zmienia parametr do ktorego dazy sprzeglo}
const engagedownspeed=0.9; engageupspeed=0.5;
var engagespeed:real; //OK:boolean;
begin
 dizel_EngageChange:=false;
 if dizel_engage-dizel_engagestate>0 then
  engagespeed:=engagedownspeed
 else
  engagespeed:=engageupspeed;
 if dt>0.2 then
  dt:=0.1;
 if abs(dizel_engage-dizel_engagestate)<0.11 then
  begin
    if (dizel_engage<>dizel_engagestate) then
     begin
       dizel_EngageChange:=true;
       dizel_engage:=dizel_engagestate;
     end
    //else OK:=false; //ju¿ jest false
  end
 else
  begin
    dizel_engage:=dizel_engage+engagespeed*dt*(dizel_engagestate-dizel_engage);
    //OK:=false;
  end;
 //dizel_EngageChange:=OK;
end;

(*
function TMoverParameters.dizel_rotateengine(Momentum,dt,n,engage:real): real;
{oblicza obroty silnika}
var newn,eps:real;
begin
  if (Momentum<0) and (n=0) and (enrot=0) then
   newn:=0
  else
   begin
     if (enrot<dizel_nmin/16) and (Momentum>0) then
       Momentum:=0                                         {nie dziala przy b. malych obrotach}
     else
     if enrot<dizel_nmin/4 then
      begin
       if (eAngle>Pi/2) then Momentum:=Momentum-2*dizel_Mstand
      end
     else
     if enrot<dizel_nmin then
      if (eAngle>Pi) then Momentum:=Momentum-dizel_Mstand; {wstrzymywanie przy malych obrotach}

     eps:=Momentum/dizel_AIM;
     if engage<0.9 then {niepelny docisk - poslizg}
      begin
        newn:=(1-engage)*(enrot+eps*dt)+engage*(n+eps*dt);
        if (newn*enrot<=0) and (eps*enrot<0) then
         newn:=0;
      end
     else
      newn:=n; {bez poslizgu sprzegla}
   end;
  dizel_rotateengine:=newn;
{todo: uwzglednianie AIM przy ruszaniu}
end;
*)

function TMoverParameters.dizel_fillcheck(mcp:byte): real;
{oblicza napelnienie, uzwglednia regulator obrotow}
var realfill,nreg: real;
begin
  realfill:=0;
  nreg:=0;
  if Mains and (MainCtrlPosNo>0) then
   begin
     if dizel_enginestart and (LastSwitchingTime>0.9*InitialCtrlDelay) then {wzbogacenie przy rozruchu}
      realfill:=1
     else
      realfill:=RList[mcp].R;                                               {napelnienie zalezne od MainCtrlPos}
     if dizel_nmax_cutoff>0 then
      begin
       case RList[MainCtrlPos].Mn of
        0,1: nreg:=dizel_nmin;
        2:   nreg:=dizel_nmax;
        else realfill:=0; {sluczaj}
       end;
       if enrot>nreg then
        realfill:=realfill*(3.9-3.0*abs(enrot)/nreg);
       if enrot>dizel_nmax_cutoff then
       realfill:=realfill*(9.8-9.0*abs(enrot)/dizel_nmax_cutoff);
       if enrot<dizel_nmin then
        realfill:=realfill*(1+(dizel_nmin-abs(enrot))/dizel_nmin);
      end;
   end;
  if realfill<0 then
   realfill:=0;
  dizel_fillcheck:=realfill;
end;

function TMoverParameters.dizel_AutoGearCheck: boolean;
{automatycznie zmienia biegi gdy predkosc przekroczy widelki}
var OK: boolean;
begin
  OK:=false;
  if MotorParam[ScndCtrlActualPos].AutoSwitch and Mains then
   begin
     if (Rlist[MainCtrlPos].Mn=0) then
      begin
        if (dizel_engagestate>0) then
         dizel_EngageSwitch(0);
        if (MainCtrlPos=0) and (ScndCtrlActualPos>0) then
         dizel_automaticgearstatus:=-1
      end
     else
      begin
        if MotorParam[ScndCtrlActualPos].AutoSwitch and (dizel_automaticgearstatus=0) then {sprawdz czy zmienic biegi}
         begin
            if (Vel>MotorParam[ScndCtrlActualPos].mfi) and (ScndCtrlActualPos<ScndCtrlPosNo) then
             begin
              dizel_automaticgearstatus:=1;
              OK:=true
             end
            else
             if (Vel<MotorParam[ScndCtrlActualPos].fi) and (ScndCtrlActualPos>0) then
              begin
               dizel_automaticgearstatus:=-1;
               OK:=true
              end;
         end;
      end;
     if (dizel_engage<0.1) and (dizel_automaticgearstatus<>0) then
      begin
        if dizel_automaticgearstatus=1 then
         inc(ScndCtrlActualPos)
        else
         dec(ScndCtrlActualPos);
        dizel_automaticgearstatus:=0;
{}        dizel_EngageSwitch(1.0);   {}
        OK:=true;
      end
   end;
  if Mains then
   begin
        if (dizel_automaticgearstatus=0) then  {ustaw cisnienie w silowniku sprzegla}
        case Rlist[MainCtrlPos].Mn of
          1: dizel_EngageSwitch(0.5);
          2: dizel_EngageSwitch(1.0);
         else dizel_EngageSwitch(0.0)
         end
        else
         dizel_EngageSwitch(0.0);
        if not (MotorParam[ScndCtrlActualPos].mIsat>0) then
         dizel_EngageSwitch(0.0);  //wylacz sprzeglo na pozycjach neutralnych
        if not AutoRelayFlag then
         ScndCtrlActualPos:=ScndCtrlPos;
   end;
  dizel_AutoGearCheck:=OK;
end;

function TMoverParameters.dizel_Update(dt:real): boolean;
{odœwie¿a informacje o silniku}
//var OK:boolean;
const fillspeed=2;
begin
  //dizel_Update:=false;
  if dizel_enginestart and (LastSwitchingTime>InitialCtrlDelay) then
    begin
      dizel_enginestart:=false;
      LastSwitchingTime:=0;
      enrot:=dizel_nmin/2.0; {TODO: dac zaleznie od temperatury i baterii}
    end;
  {OK:=}dizel_EngageChange(dt);
//  if AutoRelayFlag then Poprawka na SM03
  dizel_Update:=dizel_AutoGearCheck;
{  dizel_fill:=(dizel_fill+dizel_fillcheck(MainCtrlPos))/2; }
  dizel_fill:=dizel_fill+fillspeed*dt*(dizel_fillcheck(MainCtrlPos)-dizel_fill);
  //dizel_Update:=OK;
end;


{przyczepnosc}
function TMoverParameters.Adhesive(staticfriction:real): real;
//var Adhesion: real;
begin
  {Adhesion:=0;
  case SlippingWheels of
   True:
      if SandDose then
       Adhesion:=0.48
      else
       Adhesion:=staticfriction*0.2;
   False:
      if SandDose then
       Adhesion:=Max0R(staticfriction*(100+Vel)/(50+Vel)*1.1,0.48)
      else
       Adhesion:=staticfriction*(100+Vel)/(50+Vel)
  end;
  Adhesion:=Adhesion*(11-2*random)/10;
  Adhesive:=Adhesion;}

  //ABu: male przerobki, tylko czy to da jakikolwiek skutek w FPS?
  //     w kazdym razie zaciemni kod na pewno :)
  if SlippingWheels=False then
     begin
        if SandDose then
           Adhesive:=(Max0R(staticfriction*(100.0+Vel)/((50.0+Vel)*11.0),0.048))*(11.0-2.0*random)
        else
           Adhesive:=(staticfriction*(100.0+Vel)/((50.0+Vel)*10))*(11.0-2.0*random);
     end
  else
     begin
        if SandDose then
           Adhesive:=(0.048)*(11-2*random)
        else
           Adhesive:=(staticfriction*0.02)*(11-2*random);
     end;
end;


{SILY}

function TMoverParameters.TractionForce(dt:real):real;
var PosRatio,dmoment,dtrans,tmp,tmpV: real;
    i: byte;
{oblicza sile trakcyjna lokomotywy (dla elektrowozu tez calkowity prad)}
begin
  Ft:=0;
  dtrans:=0;
  dmoment:=0;
//  tmpV:=Abs(nrot*WheelDiameter/2);
//youBy
  if (EngineType=DieselElectric) then
    begin
     tmp:=DEList[MainCtrlPos].rpm/60.0;
     if (Heating) and (MainCtrlPosNo>MainCtrlPos) then
     begin
       i:=MainCtrlPosNo;
       while (DEList[i-2].rpm/60.0>tmp) do
         i:=i-1;
       tmp:=DEList[i].rpm/60.0
     end;
     if enrot<>tmp*Byte(ConverterFlag) then
       if ABS(tmp*Byte(ConverterFlag) - enrot) < 0.001 then
         enrot:=tmp*Byte(ConverterFlag)
       else
         if (enrot<DEList[0].rpm*0.01) and (ConverterFlag) then
           enrot:=enrot + (tmp*Byte(ConverterFlag) - enrot)*dt/5.0
         else
           enrot:=enrot + (tmp*Byte(ConverterFlag) - enrot)*1.5*dt;
    end
  else
  if EngineType<>DieselEngine then
   enrot:=Transmision.Ratio*nrot
  else
   begin
     dtrans:=Transmision.Ratio*MotorParam[ScndCtrlActualPos].mIsat;
     dmoment:=dizel_Momentum(dizel_fill,ActiveDir*1*dtrans*nrot,dt); {oblicza tez enrot}
   end;
  eAngle:=eAngle+enrot*dt;
  if eAngle>Pirazy2 then
   //eAngle:=Pirazy2-eAngle; <- ABu: a nie czasem tak, jak nizej?
   eAngle:=eAngle-Pirazy2;

  if ActiveDir<>0 then
   case EngineType of
    Dumb:
     begin
       PosRatio:=(MainCtrlPos+ScndCtrlPos)/(MainCtrlPosNo+ScndCtrlPosNo+0.01);
       if Mains and (ActiveDir<>0) and (CabNo<>0) then
        begin
          if Vel>0.1 then
           begin
             Ft:=Min0R(1000.0*Power/Abs(V),Ftmax)*PosRatio;
           end
          else Ft:=Ftmax*PosRatio;
          Ft:=Ft*DirAbsolute; //ActiveDir*CabNo;
        end
       else Ft:=0;
       EnginePower:=1000*Power*PosRatio;
     end;
    WheelsDriven:
      begin
        if EnginePowerSource.SourceType=InternalSource then
         if EnginePowerSource.PowerType=BioPower then
          Ft:=sign(sin(eAngle))*PulseForce*Transmision.Ratio;
        PulseForceTimer:=PulseForceTimer+dt;
        if PulseForceTimer>CtrlDelay then
         begin
           PulseForce:=0;
           if PulseForceCount>0 then
            dec(PulseForceCount);
         end;
        EnginePower:=Ft*(1+Vel);
      end;
    ElectricSeriesMotor:
      begin
{        enrot:=Transmision.Ratio*nrot; }
        //yB: szereg dwoch sekcji w ET42
        if(TrainType=dt_ET42)and(Imax=ImaxHi)then
          Voltage:=Voltage/2.0;
        Mm:=Momentum(Current(enrot,Voltage)); {oblicza tez prad p/slinik}
        if(TrainType=dt_ET42)then
         begin
           if(Imax=ImaxHi)then
             Voltage:=Voltage*2;
           if(DynamicBrakeFlag)and(Abs(Im)>300)then
             FuseOff;
         end;
        if (Abs(Im)>Imax) then
         FuseOff;                     {wywalanie bezpiecznika z powodu przetezenia silnikow}
        if (Abs(Voltage)<EnginePowerSource.MaxVoltage/2.0) or (Abs(Voltage)>EnginePowerSource.MaxVoltage*2) then
         if MainSwitch(False) then
          EventFlag:=True;            {wywalanie szybkiego z powodu niewlasciwego napiecia}

        if ((DynamicBrakeType=dbrake_automatic) and (DynamicBrakeFlag)) then
         Itot:=Im*2   {2x2 silniki w EP09}
        else
         Itot:=Im*RList[MainCtrlActualPos].Bn;   {prad silnika * ilosc galezi}
        Mw:=Mm*Transmision.Ratio;
        Fw:=Mw*2.0/WheelDiameter;
        Ft:=Fw*NPoweredAxles;                {sila trakcyjna}
        case RVentType of {wentylatory rozruchowe}
        1: if ActiveDir<>0 then
            RventRot:=RventRot+(RVentnmax-RventRot)*RVentSpeed*dt
           else
            RventRot:=RventRot*(1-RVentSpeed*dt);
        2: if (Abs(Itot)>RVentMinI) and (RList[MainCtrlActualPos].R>RVentCutOff) then
            RventRot:=RventRot+(RVentnmax*Abs(Itot)/(ImaxLo*RList[MainCtrlActualPos].Bn)-RventRot)*RVentSpeed*dt
           else
            begin
              RventRot:=RventRot*(1-RVentSpeed*dt);
              if RventRot<0.1 then RventRot:=0;
            end;
        end; {case}
      end;
   DieselEngine: begin
                   EnginePower:=dmoment*enrot;
                   if MainCtrlPos>1 then
                    dmoment:=dmoment-dizel_Mstand*(0.2*enrot/nmax);  {dodatkowe opory z powodu sprezarki}
                   Mm:=dizel_engage*dmoment;
                   Mw:=Mm*dtrans;           {dmoment i dtrans policzone przy okazji enginerotation}
                   Fw:=Mw*2.0/WheelDiameter;
                   Ft:=Fw*NPoweredAxles;                {sila trakcyjna}
                   Ft:=Ft*DirAbsolute; //ActiveDir*CabNo;

                 end;
   DieselElectric:    //youBy
     begin
//       tmpV:=V*CabNo*ActiveDir;
         tmpV:=nrot*Pirazy2*0.5*WheelDiameter*DirAbsolute; //*CabNo*ActiveDir;
       //jazda manewrowa
       if (ShuntMode) then
        begin
         Voltage:=(SST[MainCtrlPos].Umax * AnPos) + (SST[MainCtrlPos].Umin * (1 - AnPos));
         tmp:=(SST[MainCtrlPos].Pmax * AnPos) + (SST[MainCtrlPos].Pmin * (1 - AnPos));
         Ft:=tmp * 1000.0 / (abs(tmpV)+1.6);
         PosRatio:=1;
        end
       else  //jazda ciapongowa
      begin

       tmp:=Min0R(DEList[MainCtrlPos].genpower,Power-HeatingPower*byte(Heating));

        PosRatio:=DEList[MainCtrlPos].genpower / DEList[MainCtrlPosNo].genpower;  {stosunek mocy teraz do mocy max}
        if (MainCtrlPos>0) and (ConverterFlag) then
          if tmpV < (Vhyp*(Power-HeatingPower*byte(Heating))/DEList[MainCtrlPosNo].genpower) then //czy na czesci prostej, czy na hiperboli
            Ft:=(Ftmax - ((Ftmax - 1000.0 * DEList[MainCtrlPosNo].genpower / (Vhyp+Vadd)) * (tmpV/Vhyp) / PowerCorRatio)) * PosRatio //posratio - bo sila jakos tam sie rozklada
          else //na hiperboli                             //1.107 - wspolczynnik sredniej nadwyzki Ft w symku nad charakterystyka
            Ft:=1000.0 * tmp / (tmpV+Vadd) / PowerCorRatio //tu jest zawarty stosunek mocy
        else Ft:=0; //jak nastawnik na zero, to sila tez zero

      PosRatio:=tmp/DEList[MainCtrlPosNo].genpower;

      end;
        Ft:=Ft*DirAbsolute; //ActiveDir * CabNo; //zwrot sily i jej wartosc
        Fw:=Ft/NPoweredAxles; //sila na obwodzie kola
        Mw:=Fw*WheelDiameter / 2.0; // moment na osi kola
        Mm:=Mw/Transmision.Ratio; // moment silnika trakcyjnego


       with MotorParam[ScndCtrlPos] do
         if ABS(Mm) > fi then
           Im:=NPoweredAxles * ABS(ABS(Mm) / mfi + mIsat)
         else
           Im:=NPoweredAxles * sqrt(ABS(Mm * Isat));

       if (ShuntMode) then
       begin
         EnginePower:=Voltage * Im/1000.0;
         if (EnginePower > tmp) then
         begin
           EnginePower:=tmp*1000.0;
           Voltage:=EnginePower/Im;
         end;
         if (EnginePower < tmp) then
         Ft:=Ft * EnginePower / tmp;
       end
       else
       begin
        if (ABS(Im) > DEList[MainCtrlPos].Imax) then
        begin //nie ma nadmiarowego, tylko Imax i zwarcie na pradnicy
          Ft:=Ft/Im*DEList[MainCtrlPos].Imax;Im:=DEList[MainCtrlPos].Imax;
        end;

        if (Im > 0) then //jak pod obciazeniem
          if (Flat) then //ograniczenie napiecia w pradnicy - plaszczak u gory
            Voltage:=1000.0 * tmp / ABS(Im)
          else  //charakterystyka pradnicy obcowzbudnej (elipsa) - twierdzenie Pitagorasa
          begin
            Voltage:=sqrt(ABS(sqr(DEList[MainCtrlPos].Umax)-sqr(DEList[MainCtrlPos].Umax*Im/DEList[MainCtrlPos].Imax)))*(MainCtrlPos-1)+
                     (1-Im/DEList[MainCtrlPos].Imax)*DEList[MainCtrlPos].Umax*(MainCtrlPosNo-MainCtrlPos);
            Voltage:=Voltage/(MainCtrlPosNo-1);
            Voltage:=Min0R(Voltage,(1000.0 * tmp / ABS(Im)));
            if Voltage<(Im*0.05) then Voltage:=Im*0.05;
          end;
        if (Voltage > DEList[MainCtrlPos].Umax) or (Im = 0) then //gdy wychodzi za duze napiecie
          Voltage:=DEList[MainCtrlPos].Umax * byte(ConverterFlag);     //albo przy biegu jalowym (jest cos takiego?)

        EnginePower:=Voltage * Im / 1000.0;

        if (tmpV>2) and (EnginePower<tmp) then
          Ft:=Ft*EnginePower/tmp;

       end;


     //przekazniki bocznikowania, kazdy inny dla kazdej pozycji
         if (MainCtrlPos = 0) or (ShuntMode) then
           ScndCtrlPos:=0
         else if AutoRelayFlag then
          case RelayType of
          0: begin
              if (Im <= (MPTRelay[ScndCtrlPos].Iup*PosRatio)) and (ScndCtrlPos<ScndCtrlPosNo) then
                inc(ScndCtrlPos);
              if (Im >= (MPTRelay[ScndCtrlPos].Idown*PosRatio)) and (ScndCtrlPos>0) then
                dec(ScndCtrlPos);
             end;
          1: begin
              if (MPTRelay[ScndCtrlPos].Iup<Vel) and (ScndCtrlPos<ScndCtrlPosNo) then
                inc(ScndCtrlPos);
              if (MPTRelay[ScndCtrlPos].Idown>Vel) and (ScndCtrlPos>0) then
                dec(ScndCtrlPos);
             end;
          2: begin
              if (MPTRelay[ScndCtrlPos].Iup<Vel) and (ScndCtrlPos<ScndCtrlPosNo) and (EnginePower<(tmp*0.99)) then
                inc(ScndCtrlPos);
              if (MPTRelay[ScndCtrlPos].Idown<Im) and (ScndCtrlPos>0) then
                dec(ScndCtrlPos);
             end;
          46:
             begin
              //wzrastanie
              if (MainCtrlPos>9) and (ScndCtrlPos<ScndCtrlPosNo) then
               if (ScndCtrlPos) mod 2 = 0 then
                if (MPTRelay[ScndCtrlPos].Iup>Im) then
                 inc(ScndCtrlPos)
                else
               else
                if (MPTRelay[ScndCtrlPos-1].Iup>Im) and (MPTRelay[ScndCtrlPos].Iup<Vel) then
                 inc(ScndCtrlPos);

              //malenie
              if (MainCtrlPos<10) and (ScndCtrlPos>0)then
               if (ScndCtrlPos) mod 2 = 0 then
                if (MPTRelay[ScndCtrlPos].Idown<Im)then
                 dec(ScndCtrlPos)
                else
               else
                if (MPTRelay[ScndCtrlPos+1].Idown<Im) and (MPTRelay[ScndCtrlPos].Idown>Vel)then
                 dec(ScndCtrlPos);
              if (MainCtrlPos<9)and(ScndCtrlPos>2) then ScndCtrlPos:=2;
              if (MainCtrlPos<6)and(ScndCtrlPos>0) then ScndCtrlPos:=0;              
             end
          else end;
     end;
   None: begin end;
   {EZT: begin end;}
   end; {case EngineType}
  TractionForce:=Ft
end;

function TMoverParameters.BrakeForce(Track:TTrackParam):real;
var u,K,Fb,NBrakeAxles,sm:real;
//const OerlikonForceFactor=1.5;
begin
  K:=0;
  if NPoweredAxles>0 then
   NBrakeAxles:=NPoweredAxles
  else
   NBrakeAxles:=NAxles;
  case LocalBrake of
   NoBrake :      K:=0;
   ManualBrake :  K:=MaxBrakeForce*LocalBrakeRatio;
   HydraulicBrake : K:=0;
   PneumaticBrake:if Compressor<MaxBrakePress then
                   K:=MaxBrakeForce*LocalBrakeRatio/2.0
                  else
                   K:=0;
  end;

  sm:=1;
  //test pozycji pospiesznej
  if TestFlag(BrakeDelays,bdelay_R)then
   if(not TestFlag(BrakeStatus,b_Ractive)and(BrakeCtrlPosNo=0))then
     sm:=0.5
   else if(TestFlag(BrakeStatus,b_Ractive)and(BrakeCtrlPosNo>0))then
     sm:=1.5;
                //0.03
  if (BrakePress)>(0.05*MaxBrakePress*sm) then         {nie luz}
   begin
//     if (BrakeSubsystem=Oerlikon) and (Vel>50) then //yB: to na razie komentuje, potem sie zrobi lepiej
//      K:=K+BrakePress*P2FTrans*OerlikonForceFactor  {w kN}
//     else
      K:=K+BrakePress*P2FTrans;                     {w kN}
     K:=K*BrakeCylNo/(NBrakeAxles*NBpA);            {w kN na os}
   end;
  if (BrakeMethod and 1 = 0) then
    u:=60*(0.16*K+100.0)/((0.8*K+100.0)*(3.0*Vel+100.0)) {wsp. tarcia}
 else
    u:=0.4*(0.4*K+100.0)*(Vel+80.0)/((0.8*K+100.0)*(2.0*Vel+80.0)); {wsp. tarcia}
  UnitBrakeForce:=u*K*1000;                     {sila na jeden klocek w N}
  if (NBpA*UnitBrakeForce>TotalMassxg*Adhesive(RunningTrack.friction)/NAxles) and (Abs(V)>0.001) then
   {poslizg}
   begin
{     Fb:=Adhesive(Track.friction)*Mass*g;  }
     SlippingWheels:=True;
   end;
{  else
   begin
{     SlippingWheels:=False;}
     if (LocalBrake=ManualBrake) and (BrakePress<0.05*MaxBrakePress) then
      Fb:=UnitBrakeForce*NBpA {ham. reczny dziala na jedna os}
     else
      Fb:=UnitBrakeForce*NBrakeAxles*NBpA;

 {  end; }
  BrakeForce:=Fb;
end;

procedure TMoverParameters.SetCoupleDist;
{przeliczenie odleg³oœci sprzêgów}
begin
 with Couplers[0] do
  if (Connected<>nil) then
   CoupleDist:=Distance(Loc,Connected^.Loc,Dim,Connected^.Dim);
 with Couplers[1] do
  if (Connected<>nil) then
   CoupleDist:=Distance(Loc,Connected^.Loc,Dim,Connected^.Dim);
end;

function TMoverParameters.CouplerForce(CouplerN:byte;dt:real):real;
//wyliczenie si³y na sprzêgu
var tempdist,newdist,distDelta,CF,dV,absdv,Fmax,BetaAvg:real; CNext:byte;
const MaxDist=405.0; {ustawione + 5 m, bo skanujemy do 400 m }
      MinDist=0.5;   {ustawione +.5 m, zeby nie rozlaczac przy malych odleglosciach}
      MaxCount=1500;
begin
  CF:=0;
  //distDelta:=0; //Ra: value never used
  CNext:=Couplers[CouplerN].ConnectedNr;
{  if Couplers[CouplerN].CForce=0 then  {nie bylo uzgadniane wiec policz}
   with Couplers[CouplerN] do
    begin
      CheckCollision:=False;
      newdist:=CoupleDist; //odleg³oœæ od sprzêgu s¹siada
      //newdist:=Distance(Loc,Connected^.Loc,Dim,Connected^.Dim);
      if (CouplerN=0) then
         begin
            //ABu: bylo newdist+10*((...
            tempdist:=((Connected^.dMoveLen*DirPatch(0,ConnectedNr))-dMoveLen);
            newdist:=newdist+10.0*tempdist;
            tempdist:=tempdist+CoupleDist; //ABu: proby szybkiego naprawienia bledu
         end
      else
         begin
            //ABu: bylo newdist+10*((...
            tempdist:=((dMoveLen-(Connected^.dMoveLen*DirPatch(1,ConnectedNr))));
            newdist:=newdist+10.0*tempdist;
            tempdist:=tempdist+CoupleDist; //ABu: proby szybkiego naprawienia bledu
         end;

      //blablabla
      //ABu: proby znalezienia problemu ze zle odbijajacymi sie skladami
      //***if (Couplers[CouplerN].CouplingFlag=ctrain_virtual) and (newdist>0) then
      if (Couplers[CouplerN].CouplingFlag=ctrain_virtual) and (CoupleDist>0) then
       begin
         CF:=0;              {kontrola zderzania sie - OK}
         inc(ScanCounter);
         if (newdist>MaxDist) or ((ScanCounter>MaxCount)and(newdist>MinDist)) then
         //***if (tempdist>MaxDist) or ((ScanCounter>MaxCount)and(tempdist>MinDist)) then
          begin {zerwij kontrolnie wirtualny sprzeg}
            Connected^.Couplers[CNext].Connected:=nil;
            Connected:=nil;
            ScanCounter:=Random(500);
          end;
       end
      else
       begin
         if CouplingFlag=ctrain_virtual then
          begin
            BetaAvg:=beta;
            Fmax:=(FmaxC+FmaxB)*CouplerTune;
          end
         else            {usrednij bo wspolny sprzeg}
          begin
            BetaAvg:=(beta+Connected^.Couplers[CNext].beta)/2.0;
            Fmax:=(FmaxC+FmaxB+Connected^.Couplers[CNext].FmaxC+Connected^.Couplers[CNext].FmaxB)*CouplerTune/2.0;
          end;
         dV:=V-DirPatch(CouplerN,CNext)*Connected^.V;
         absdv:=abs(dV);
         if (newdist<-0.001) and (Dist>=-0.001) and (absdv>0.010) then {090503: dzwieki pracy zderzakow}
          begin
            if SetFlag(SoundFlag,sound_bufferclamp) then
             if absdV>0.5 then
              SetFlag(SoundFlag,sound_loud);
          end
         else
         if (newdist>0.002) and (Dist<=0.002) and (absdv>0.005) then {090503: dzwieki pracy sprzegu}
          begin
            if CouplingFlag>0 then
             if SetFlag(SoundFlag,sound_couplerstretch) then
              if absdV>0.1 then
              SetFlag(SoundFlag,sound_loud);
          end;
         distDelta:=abs(newdist)-abs(Dist);  //McZapkie-191103: poprawka na histereze
         Dist:=newdist;
         if Dist>0 then
          begin
            if distDelta>0 then
              CF:=(-(SpringKC+Connected^.Couplers[CNext].SpringKC)*Dist/2.0)*DirF(CouplerN)-Fmax*dV*betaAvg
             else
              CF:=(-(SpringKC+Connected^.Couplers[CNext].SpringKC)*Dist/2.0)*DirF(CouplerN)*betaAvg-Fmax*dV*betaAvg;
                {liczenie sily ze sprezystosci sprzegu}
            if Dist>(DmaxC+Connected^.Couplers[CNext].DmaxC) then {zderzenie}
            //***if tempdist>(DmaxC+Connected^.Couplers[CNext].DmaxC) then {zderzenie}
             CheckCollision:=True;
          end;
         if Dist<0 then
          begin
            if distDelta>0 then
              CF:=(-(SpringKB+Connected^.Couplers[CNext].SpringKB)*Dist/2.0)*DirF(CouplerN)-Fmax*dV*betaAvg
            else
              CF:=(-(SpringKB+Connected^.Couplers[CNext].SpringKB)*Dist/2.0)*DirF(CouplerN)*betaAvg-Fmax*dV*betaAvg;
                {liczenie sily ze sprezystosci zderzaka}
           if -Dist>(DmaxB+Connected^.Couplers[CNext].DmaxB) then  {zderzenie}
            //***if -tempdist>(DmaxB+Connected^.Couplers[CNext].DmaxB)/10 then  {zderzenie}
             begin
               CheckCollision:=True;
               if (CouplerType=Automatic) and (CouplingFlag=0) then  {sprzeganie wagonow z samoczynnymi sprzegami}
                CouplingFlag:=ctrain_coupler+ctrain_pneumatic+ctrain_controll;
             end;
          end;
       end;
      if (CouplingFlag<>ctrain_virtual)
      then         {uzgadnianie prawa Newtona}
       Connected^.Couplers[1-CouplerN].CForce:=-CF;
    end;
  CouplerForce:=CF;
end;

procedure TMoverParameters.CollisionDetect(CouplerN:byte; dt:real);
var CCF,Vprev,VprevC:real; VirtualCoupling:boolean;
begin
     CCF:=0;
     with Couplers[CouplerN] do
      begin
        VirtualCoupling:=(CouplingFlag=ctrain_virtual);
        Vprev:=V;
        VprevC:=Connected^.V;
        case CouplerN of
         0 : CCF:=ComputeCollision(V,Connected^.V,TotalMass,Connected^.TotalMass,(beta+Connected^.Couplers[ConnectedNr].beta)/2.0,VirtualCoupling)/(dt{+0.01}); //yB: ej ej ej, a po
         1 : CCF:=ComputeCollision(Connected^.V,V,Connected^.TotalMass,TotalMass,(beta+Connected^.Couplers[ConnectedNr].beta)/2.0,VirtualCoupling)/(dt{+0.01}); //czemu tu jest +0.01??
        end;
        AccS:=AccS+(V-Vprev)/dt;
        Connected^.AccS:=Connected^.AccS+(Connected^.V-VprevC)/dt;
        if (Dist>0) and (not VirtualCoupling) then
         if FuzzyLogic(Abs(CCF),5*(FmaxC+1),p_coupldmg) then
          begin                            {! zerwanie sprzegu}
            if SetFlag(DamageFlag,dtrain_coupling) then
             EventFlag:=True;
            if (Couplers[CouplerN].CouplingFlag and ctrain_pneumatic>0) then
             EmergencyBrakeFlag:=True;   {hamowanie nagle - zerwanie przewodow hamulcowych}
            CouplingFlag:=0;
            case CouplerN of             {wyzerowanie flag podlaczenia ale ciagle sa wirtualnie polaczone}
             0 : Connected^.Couplers[1].CouplingFlag:=0;
             1 : Connected^.Couplers[0].CouplingFlag:=0;
            end
          end ;
      end;
end;

procedure TMoverParameters.ComputeConstans;
var BearingF,RollF,HideModifier: real;
begin
  TotalMass:=ComputeMass;
  TotalMassxg:=TotalMass*g; {TotalMass*g}
  BearingF:=2*(DamageFlag and dtrain_bearing);

  HideModifier:=Byte(Couplers[0].CouplingFlag>0)+Byte(Couplers[1].CouplingFlag>0);
  with Dim do
   begin
    if BearingType=0 then
     RollF:=0.05          {slizgowe}
    else
     RollF:=0.015;        {toczne}
    RollF:=RollF+BearingF/200.0;

(*    if NPoweredAxles>0 then
     RollF:=RollF*1.5;    {dodatkowe lozyska silnikow}*)
    if(NPoweredAxles>0)then //drobna optymalka
     begin
       RollF:=RollF+0.025;
       if(Ft*Ft<1)then
         HideModifier:=HideModifier-3;
     end;
    Ff:=TotalMassxg*(BearingF+RollF*V*V/10.0)/1000.0;
    {dorobic liczenie temperatury lozyska!}
    FrictConst1:=((TotalMassxg*RollF)/10000.0)+(Cx*W*H);
    FrictConst2s:= (TotalMassxg*(2.5 - HideModifier + 2*BearingF/dtrain_bearing))/1000.0;
    FrictConst2d:= (TotalMassxg*(2.0 - HideModifier + BearingF/dtrain_bearing))/1000.0;
   end;
end;

function TMoverParameters.FrictionForce(R:real;TDamage:byte):real;
begin
//ABu 240205: chyba juz ekstremalnie zoptymalizowana funkcja liczaca sily tarcia
   if (abs(V)>0.01) then
      FrictionForce:=(FrictConst1*V*V)+FrictConst2d
   else
      FrictionForce:=(FrictConst1*V*V)+FrictConst2s;
end;

function TMoverParameters.AddPulseForce(Multipler:integer): boolean; {dla drezyny}
begin
  if (EngineType=WheelsDriven) and (EnginePowerSource.SourceType=InternalSource) and (EnginePowerSource.PowerType=BioPower) then
   begin
     ActiveDir:=CabNo;
     DirAbsolute:=ActiveDir*CabNo;
     if Vel>0 then
       PulseForce:=Min0R(1000.0*Power/(Abs(V)+0.1),Ftmax)
      else PulseForce:=Ftmax;
     if PulseForceCount>1000.0 then
      PulseForce:=0
     else
       PulseForce:=PulseForce*Multipler;
     PulseForceCount:=PulseForceCount+Abs(Multipler);
     AddPulseForce:=(PulseForce>0);
   end
  else AddPulseForce:=False;
end;


function TMoverParameters.LoadingDone(LSpeed:real; LoadInit:string): boolean;
//test zakoñczenia za³adunku/roz³adunku
var LoadChange:longint;
begin
 ClearPendingExceptions; //zabezpieczenie dla Trunc()
 LoadingDone:=False; //nie zakoñczone
 if (LoadInit<>'') then //nazwa ³adunku niepusta
  begin
   if Load>MaxLoad then
    LoadChange:=Abs(Trunc(LSpeed*LastLoadChangeTime/2.0)) //prze³adowanie?
   else
    LoadChange:=Abs(Trunc(LSpeed*LastLoadChangeTime));
   if LSpeed<0 then //gdy roz³adunek
    begin
     LoadStatus:=2; //trwa roz³adunek (w³¹czenie naliczania czasu)
     if LoadChange<>0 then //jeœli coœ prze³adowano
      begin
       LastLoadChangeTime:=0; //naliczony czas zosta³ zu¿yty
       Load:=Load-LoadChange; //zmniejszenie iloœci ³adunku
       CommandIn.Value1:=CommandIn.Value1-LoadChange; //zmniejszenie iloœci do roz³adowania
       if Load<0 then
        Load:=0; //³adunek nie mo¿e byæ ujemny
       if (Load=0) or (CommandIn.Value1<0) then //pusto lub roz³adowano ¿¹dan¹ iloœæ
        LoadStatus:=4; //skoñczony roz³adunek
       if Load=0 then LoadType:=''; //jak nic nie ma, to nie ma te¿ nazwy
      end
    end
   else if LSpeed>0 then //gdy za³adunek
    begin
     LoadStatus:=1; //trwa za³adunek (w³¹czenie naliczania czasu)
     if LoadChange<>0 then //jeœli coœ prze³adowano
      begin
       LastLoadChangeTime:=0; //naliczony czas zosta³ zu¿yty
       LoadType:=LoadInit; //nazwa
       Load:=Load+LoadChange; //zwiêkszenie ³adunku
       CommandIn.Value1:=CommandIn.Value1-LoadChange;
       if (Load>=MaxLoad*(1+OverLoadFactor)) or (CommandIn.Value1<0) then
        LoadStatus:=4; //skoñczony za³adunek
      end
    end
   else LoadStatus:=4; //zerowa prêdkoœæ zmiany, to koniec
  end;
 LoadingDone:=(LoadStatus>=4);
end;

{-------------------------------------------------------------------}
{WAZNA FUNKCJA - oblicza sile wypadkowa}
procedure TMoverParameters.ComputeTotalForce(dt: real; dt1: real; FullVer: boolean);
var b: byte;
begin
  if PhysicActivation then
   begin
  {  EventFlag:=False;                                             {jesli cos sie zlego wydarzy to ustawiane na True}
  {  SoundFlag:=0;                                                 {jesli ma byc jakis dzwiek to zapalany jet odpowiedni bit}
  {to powinno byc zerowane na zewnatrz}

    //juz zoptymalizowane:
    FStand:=FrictionForce(RunningShape.R,RunningTrack.DamageFlag);
    Vel:=Abs(V)*3.6;
    nrot:=V2n(); //przeliczenie prêdkoœci liniowej na obrotow¹

    if TestFlag(BrakeMethod,2) and (PipePress<CntrlPipePress/3.0)then
      FStand:=Fstand+(RunningTrack.friction)*TrackBrakeForce;

//    if(FullVer=True) then
//    begin //ABu: to dla optymalizacji, bo chyba te rzeczy wystarczy sprawdzac 1 raz na klatke?
       LastSwitchingTime:=LastSwitchingTime+dt1;
       if EngineType=ElectricSeriesMotor then
          LastRelayTime:=LastRelayTime+dt1;
//    end;
       if Mains and (Abs(CabNo)<2) and (EngineType=ElectricSeriesMotor) then                              {potem ulepszyc! pantogtrafy!}
       begin
          if CabNo=0 then
             Voltage:=RunningTraction.TractionVoltage*ActiveDir
          else
             Voltage:=RunningTraction.TractionVoltage*DirAbsolute; //ActiveDir*CabNo;
          end {bo nie dzialalo}
       else
          Voltage:=0;
    //end;

    if Power>0 then
       FTrain:=TractionForce(dt)
    else
       FTrain:=0;
    Fb:=BrakeForce(RunningTrack);
    if Max0R(Abs(FTrain),Fb)>TotalMassxg*Adhesive(RunningTrack.friction) then    {poslizg}
     SlippingWheels:=True;
    if SlippingWheels then
     begin
  {     TrainForce:=TrainForce-Fb; }
       nrot:=ComputeRotatingWheel((FTrain-Fb*Sign(V)-Fstand)/NAxles-Sign(nrot*Pi*WheelDiameter-V)*Adhesive(RunningTrack.friction)*TotalMass,dt,nrot);
       FTrain:=sign(FTrain)*TotalMassxg*Adhesive(RunningTrack.friction);
       Fb:=Min0R(Fb,TotalMassxg*Adhesive(RunningTrack.friction));
     end;
  {  else SlippingWheels:=False; }
  {  FStand:=0; }
    for b:=0 to 1 do
     if (Couplers[b].Connected<>nil) {and (Couplers[b].CouplerType<>Bare) and (Couplers[b].CouplerType<>Articulated)} then
      begin
        Couplers[b].CForce:=CouplerForce(b,dt);
        FTrain:=FTrain+Couplers[b].CForce
      end
     else Couplers[b].CForce:=0;
    //FStand:=Fb+FrictionForce(RunningShape.R,RunningTrack.DamageFlag);
    FStand:=Fb+Fstand;
    FTrain:=FTrain+TotalMassxg*RunningShape.dHtrack/RunningShape.Len;
    {!niejawne przypisanie zmiennej!}
    FTotal:=FTrain-Sign(V)*FStand;
   end;

   {McZapkie-031103: sprawdzanie czy warto liczyc fizyke i inne updaty}
   {ABu 300105: cos tu mieszalem , dziala teraz troche lepiej, wiec zostawiam
        zakomentowalem PhysicActivationFlag bo cos nie dzialalo i fizyka byla
        liczona zawsze.}
   //if PhysicActivationFlag then
   // begin
     if (CabNo=0) and (Vel<0.0001) and (abs(AccS)<0.0001) and (TrainType<>dt_EZT) then
      begin
        if not PhysicActivation then
         begin
          if Couplers[0].Connected<>nil then
           if (Couplers[0].Connected.Vel>0.0001) or (Abs(Couplers[0].Connected.AccS)>0.0001) then
            Physic_ReActivation;
          if Couplers[1].Connected<>nil then
          if (Couplers[1].Connected.Vel>0.0001) or (Abs(Couplers[1].Connected.AccS)>0.0001) then
            Physic_ReActivation;
         end;
        if LastSwitchingTime>5 then //10+Random(100) then
         PhysicActivation:=false; {zeby nie brac pod uwage braku V po uruchomieniu programu}
      end
     else
      PhysicActivation:=true;
   // end;

end;

{------------------------------------------------------------}
{OBLICZENIE PRZEMIESZCZENIA}
function TMoverParameters.ComputeMovement(dt:real; dt1:real; Shape:TTrackShape; var Track:TTrackParam; var ElectricTraction:TTractionParam; NewLoc:TLocation; var NewRot:TRotation):real;
var b:byte;
    Vprev,AccSprev:real;
const Vepsilon=1e-5; Aepsilon=1e-3; ASBSpeed=0.8;
begin
  ClearPendingExceptions;
  if not TestFlag(DamageFlag,dtrain_out) then
   begin
     RunningShape:=Shape;
     RunningTrack:=Track;
     RunningTraction:=ElectricTraction;
     with ElectricTraction do
      if not DynamicBrakeFlag then
       RunningTraction.TractionVoltage:=TractionVoltage-Abs(TractionResistivity*Itot)
      else
       RunningTraction.TractionVoltage:=TractionVoltage-Abs(TractionResistivity*Itot);
   end;
  if CategoryFlag=4 then
   OffsetTrackV:=TotalMass/(Dim.L*Dim.W*1000.0)
  else
   if TestFlag(CategoryFlag,1) and TestFlag(RunningTrack.CategoryFlag,1) then
    if TestFlag(DamageFlag,dtrain_out) then
     begin
       OffsetTrackV:=-0.2; OffsetTrackH:=sign(RunningShape.R)*0.2;
     end;
  Loc:=NewLoc;
  Rot:=NewRot;
  with NewRot do begin Rx:=0; Ry:=0; Rz:=0; end;
  if dL=0 then {oblicz przesuniecie}
   begin
     Vprev:=V;
     AccSprev:=AccS;
     {dt:=ActualTime-LastUpdatedTime;}                               {przyrost czasu}
     {przyspieszenie styczne}
     AccS:=(FTotal/TotalMass+AccSprev)/2.0;                            {prawo Newtona ale z wygladzaniem}

     if TestFlag(DamageFlag,dtrain_out) then
      AccS:=-Sign(V)*g*Random;
     {przyspieszenie normalne}
     if Abs(Shape.R)>0.01 then
      AccN:=SQR(V)/Shape.R+g*Shape.dHrail/TrackW
     else AccN:=g*Shape.dHrail/TrackW;

     {szarpanie}
     if FuzzyLogic((10+Track.DamageFlag)*Mass*Vel/Vmax,500000,p_accn) then {Ra: czemu tu masa bez ³adunku?}
       AccV:=sqrt((1+Track.DamageFlag)*random(Trunc(50*Mass/1000000.0))*Vel/(Vmax*(10+(Track.QualityFlag and 31))))
     else
      AccV:=AccV/2.0;
     if AccV>1.0 then
      AccN:=AccN+(7-random(5))*(100.0+Track.DamageFlag/2.0)*AccV/2000.0;

     {wykolejanie na luku oraz z braku szyn}
     if TestFlag(CategoryFlag,1) then
      begin
       if FuzzyLogic((AccN/g)*(1+0.1*(Track.DamageFlag and dtrack_freerail)),TrackW/Dim.H,1)
       or TestFlag(Track.DamageFlag,dtrack_norail) then
        if SetFlag(DamageFlag,dtrain_out) then
         begin
           EventFlag:=True;
           MainS:=False;
           RunningShape.R:=0;
           if (TestFlag(Track.DamageFlag,dtrack_norail)) then
            DerailReason:=1 //Ra: powód wykolejenia: brak szyn
           else
            DerailReason:=2; //Ra: powód wykolejenia: przewrócony na ³uku
         end;
     {wykolejanie na poszerzeniu toru}
        if FuzzyLogic(Abs(Track.Width-TrackW),TrackW/10.0,1) then
         if SetFlag(DamageFlag,dtrain_out) then
          begin
            EventFlag:=True;
            MainS:=False;
            RunningShape.R:=0;
            DerailReason:=3; //Ra: powód wykolejenia: za szeroki tor
          end;
      end;
     {wykolejanie wkutek niezgodnosci kategorii toru i pojazdu}
     if not TestFlag(RunningTrack.CategoryFlag,CategoryFlag) then
      if SetFlag(DamageFlag,dtrain_out) then
        begin
          EventFlag:=True;
          MainS:=False;
          DerailReason:=4; //Ra: powód wykolejenia: nieodpowiednia trajektoria
        end;

     V:=V+(3*AccS-AccSprev)*dt/2.0;                                  {przyrost predkosci}

     if TestFlag(DamageFlag,dtrain_out) then
      if Vel<1 then
       begin
         V:=0;
         AccS:=0;
       end;

     if (V*Vprev<=0) and (Abs(FStand)>Abs(FTrain)) then            {tlumienie predkosci przy hamowaniu}
       begin     {zahamowany}
         V:=0;
         AccS:=0;
       end;
   (*
     else
      if (Abs(V)<Vepsilon) and (Abs(AccS)<Aepsilon) then
       begin
         V:=0;
         AccS:=0;
       end;
   *)
   { dL:=(V+AccS*dt/2)*dt;                                      {przyrost dlugosci czyli przesuniecie}
     dL:=(3*V-Vprev)*dt/2.0;                                       {metoda Adamsa-Bashfortha}
    {ale jesli jest kolizja (zas. zach. pedu) to...}
    for b:=0 to 1 do
     if Couplers[b].CheckCollision then
      CollisionDetect(b,dt);  {zmienia niejawnie AccS, V !!!}
(*
    for b:=0 to 1 do
     begin
       Couplers[b].CForce:=0;  {!}
       SendAVL(AccS,AccN,V,dL,0);
     end;
*)
   end; {liczone dL, predkosc i przyspieszenie}


 if (EngineType=WheelsDriven) then
  ComputeMovement:=CabNo*dL {na chwile dla testu}
 else
  ComputeMovement:=dL;
 DistCounter:=DistCounter+abs(dL)/1000.0;
 dL:=0;

   {koniec procedury, tu nastepuja dodatkowe procedury pomocnicze}

{sprawdzanie i ewentualnie wykonywanie->kasowanie polecen}
 if (LoadStatus>0) then //czas doliczamy tylko jeœli trwa (roz)³adowanie
  LastLoadChangeTime:=LastLoadChangeTime+dt; //czas (roz)³adunku
 RunInternalCommand;

{automatyczny rozruch}
 if EngineType=ElectricSeriesMotor then
  begin
   if AutoRelayCheck then
    SetFlag(SoundFlag,sound_relay);
  end;
(*
 else                  {McZapkie-041003: aby slychac bylo przelaczniki w sterowniczym}
  if (EngineType=None) and (MainCtrlPosNo>0) then
   for b:=0 to 1 do
    with Couplers[b] do
     if TestFlag(CouplingFlag,ctrain_controll) then
      if Connected^.Power>0.01 then
       SoundFlag:=SoundFlag or Connected^.SoundFlag;
*)
 if EngineType=DieselEngine then
  if dizel_Update(dt) then
    SetFlag(SoundFlag,sound_relay);

{uklady hamulcowe:}
  if VeselVolume>0 then
   Compressor:=CompressedVolume/(10.0*VeselVolume)
  else
   begin
     Compressor:=0;
     CompressorFlag:=False;
   end;
 CompressorCheck(dt);
 UpdatePantVolume(dt);
 ConverterCheck;
 UpdateBrakePressure(dt);
 UpdatePipePressure(dt);

 UpdateScndPipePressure(dt); // druga rurka, youBy

{hamulec antyposlizgowy - wylaczanie}
 if BrakeSlippingTimer>ASBSpeed then
  SetFlag(BrakeStatus,-b_antislip);
 BrakeSlippingTimer:=BrakeSlippingTimer+dt;
{sypanie piasku - wylaczone i piasek sie nie konczy - bledy AI}
//if AIControllFlag then
// if SandDose then
//  if Sand>0 then
//   begin
//     Sand:=Sand-NPoweredAxles*SandSpeed*dt;
//     if Random<dt then SandDose:=False;
//   end
//  else
//   begin
//     SandDose:=False;
//     Sand:=0;
//   end;
{czuwak/SHP}
 if (Vel>10) and (not DebugmodeFlag) then
  SecuritySystemCheck(dt1);
end; {ComputeMovement}

{blablabla}
{OBLICZENIE PRZEMIESZCZENIA}
function TMoverParameters.FastComputeMovement(dt:real; Shape:TTrackShape; var Track:TTrackParam;NewLoc:TLocation; var NewRot:TRotation):real; //;var ElectricTraction:TTractionParam; NewLoc:TLocation; var NewRot:TRotation):real;
var b:byte;
    Vprev,AccSprev:real;
const Vepsilon=1e-5; Aepsilon=1e-3; ASBSpeed=0.8;
begin
  ClearPendingExceptions;
  Loc:=NewLoc;
  Rot:=NewRot;
  with NewRot do begin Rx:=0; Ry:=0; Rz:=0; end;
  if dL=0 then {oblicz przesuniecie}
   begin
     Vprev:=V;
     AccSprev:=AccS;
     {dt:=ActualTime-LastUpdatedTime;}                               {przyrost czasu}
     {przyspieszenie styczne}
     AccS:=(FTotal/TotalMass+AccSprev)/2.0;                            {prawo Newtona ale z wygladzaniem}

     if TestFlag(DamageFlag,dtrain_out) then
      AccS:=-Sign(V)*g*Random;
     {przyspieszenie normalne}
//     if Abs(Shape.R)>0.01 then
//      AccN:=SQR(V)/Shape.R+g*Shape.dHrail/TrackW
//     else AccN:=g*Shape.dHrail/TrackW;

     {szarpanie}
     if FuzzyLogic((10+Track.DamageFlag)*Mass*Vel/Vmax,500000,p_accn) then
       AccV:=sqrt((1+Track.DamageFlag)*random(Trunc(50*Mass/1000000.0))*Vel/(Vmax*(10+(Track.QualityFlag and 31))))
     else
      AccV:=AccV/2.0;
     if AccV>1.0 then
      AccN:=AccN+(7-random(5))*(100.0+Track.DamageFlag/2.0)*AccV/2000.0;

//     {wykolejanie na luku oraz z braku szyn}
//     if TestFlag(CategoryFlag,1) then
//      begin
//       if FuzzyLogic((AccN/g)*(1+0.1*(Track.DamageFlag and dtrack_freerail)),TrackW/Dim.H,1)
//       or TestFlag(Track.DamageFlag,dtrack_norail) then
//        if SetFlag(DamageFlag,dtrain_out) then
//         begin
//           EventFlag:=True;
//           MainS:=False;
//           RunningShape.R:=0;
//         end;
//     {wykolejanie na poszerzeniu toru}
//        if FuzzyLogic(Abs(Track.Width-TrackW),TrackW/10,1) then
//         if SetFlag(DamageFlag,dtrain_out) then
//          begin
//            EventFlag:=True;
//            MainS:=False;
//            RunningShape.R:=0;
//          end;
//      end;
//     {wykolejanie wkutek niezgodnosci kategorii toru i pojazdu}
//     if not TestFlag(RunningTrack.CategoryFlag,CategoryFlag) then
//      if SetFlag(DamageFlag,dtrain_out) then
//        begin
//          EventFlag:=True;
//          MainS:=False;
//        end;

     V:=V+(3.0*AccS-AccSprev)*dt/2.0;                                  {przyrost predkosci}

     if TestFlag(DamageFlag,dtrain_out) then
      if Vel<1 then
       begin
         V:=0;
         AccS:=0;
       end;

     if (V*Vprev<=0) and (Abs(FStand)>Abs(FTrain)) then            {tlumienie predkosci przy hamowaniu}
       begin     {zahamowany}
         V:=0;
         AccS:=0;
       end;
     dL:=(3*V-Vprev)*dt/2.0;                                       {metoda Adamsa-Bashfortha}
    {ale jesli jest kolizja (zas. zach. pedu) to...}
    for b:=0 to 1 do
     if Couplers[b].CheckCollision then
      CollisionDetect(b,dt);  {zmienia niejawnie AccS, V !!!}
   end; {liczone dL, predkosc i przyspieszenie}

 if (EngineType=WheelsDriven) then
  FastComputeMovement:=CabNo*dL {na chwile dla testu}
 else
  FastComputeMovement:=dL;
 DistCounter:=DistCounter+abs(dL)/1000.0;
 dL:=0;

   {koniec procedury, tu nastepuja dodatkowe procedury pomocnicze}

{sprawdzanie i ewentualnie wykonywanie->kasowanie polecen}
 if (LoadStatus>0) then //czas doliczamy tylko jeœli trwa (roz)³adowanie
  LastLoadChangeTime:=LastLoadChangeTime+dt; //czas (roz)³adunku
 RunInternalCommand;

 if EngineType=DieselEngine then
  if dizel_Update(dt) then
    SetFlag(SoundFlag,sound_relay);

{uklady hamulcowe:}
  if VeselVolume>0 then
   Compressor:=CompressedVolume/(10.0*VeselVolume)
  else
   begin
     Compressor:=0;
     CompressorFlag:=False;
   end;
 CompressorCheck(dt);
 UpdatePantVolume(dt);
 ConverterCheck;
 UpdateBrakePressure(dt);
 UpdatePipePressure(dt);
 UpdateScndPipePressure(dt); // druga rurka, youBy

{hamulec antyposlizgowy - wylaczanie}
 if BrakeSlippingTimer>ASBSpeed then
  SetFlag(BrakeStatus,-b_antislip);
 BrakeSlippingTimer:=BrakeSlippingTimer+dt;
end; {FastComputeMovement}



function TMoverParameters.ComputeMass: real;
var M:real;
begin
 if Load>0 then
  begin
   if LoadQuantity='tonns' then
    M:=Load*1000
   else if LowerCase(LoadType)=LowerCase('Passengers') then
    M:=Load*80
   else if LoadType='Luggage' then
    M:=Load*100
   else if LoadType='Cars' then
    M:=Load*800
   else if LoadType='Containers' then
    M:=Load*8000
   else if LoadType='Transformers' then
    M:=Load*50000
   else
    M:=Load*1000;
  end
 else
  M:=0;
 {Ra: na razie tak, ale nie wszêdzie masy wiruj¹ce siê wliczaj¹}
 ComputeMass:=Mass+M+Mred;
end;



{-----------------------------------------------------------------}
{funkcje wejscia/wyjscia}

{przekazywanie komend i parametrow}

function TMoverParameters.SetInternalCommand(NewCommand:string; NewValue1,NewValue2:real):boolean;
begin
  if (CommandIn.Command=Newcommand) and (commandIn.Value1=NewValue1) and (commandIn.Value2=NewValue2) then
   SetInternalCommand:=False
  else
   with CommandIn do
    begin
     Command:=NewCommand;
     Value1:=NewValue1;
     Value2:=NewValue2;
     SetInternalCommand:=True;
     LastLoadChangeTime:=0; //zerowanie czasu (roz)³adowania
    end;
end;

function TMoverParameters.GetExternalCommand(var Command:string):real;
begin
  Command:=CommandOut;
  GetExternalCommand:=ValueOut;
end;

function TMoverParameters.RunCommand(command:string; CValue1,CValue2:real):boolean;
var OK:boolean; testload:string;
Begin
{$B+} {cholernie mi sie nie podoba ta dyrektywa!!!}
  OK:=False;
  ClearPendingExceptions();

  {test komend sterowania ukrotnionego}
  if command='MainCtrl' then
   begin
     if MainCtrlPosNo>=Trunc(CValue1) then
      MainCtrlPos:=Trunc(CValue1);
     OK:=SendCtrlToNext(command,CValue1,CValue2);
   end
  else if command='ScndCtrl' then
   begin
     if ScndCtrlPosNo>=Trunc(CValue1) then
      ScndCtrlPos:=Trunc(CValue1);
     OK:=SendCtrlToNext(command,CValue1,CValue2);
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
     case BrakeSubsystem of
      Oerlikon,WeLu:
       begin
         BrakeCtrlPos:=Trunc(Cvalue1);
         if Trunc(CValue1)=0 then
           SetFlag(BrakeStatus,-b_epused)
         else
           SetFlag(BrakeStatus,b_epused);
       end;
      Knorr:
       begin
         case Trunc(CValue1) of
         -1:
           begin
             BrakeCtrlPos:=0;
             SetFlag(BrakeStatus,b_epused);
           end;
          0: SetFlag(BrakeStatus,-b_epused);
          1:
           begin
             BrakeCtrlPos:=1;
             SetFlag(BrakeStatus,b_epused);
           end;
          end;
       end;
     end;
     OK:=SendCtrlToNext(command,CValue1,CValue2);
   end //youby - odluzniacz hamulcow, przyda sie
  else if command='BrakeReleaser' then
   begin
     OK:=BrakeReleaser; //samo siê przesy³a dalej
     //OK:=SendCtrlToNext(command,CValue1,CValue2); //to robi³o kaskadê 2^n
   end
  else if command='MainSwitch' then
   begin
     if CValue1=1 then
      begin
        Mains:=True;
        if (EngineType=DieselEngine) and Mains then
          dizel_enginestart:=true;
      end
     else Mains:=False;
     OK:=SendCtrlToNext(command,CValue1,CValue2);
   end
  else if command='Direction' then
   begin
     ActiveDir:=Trunc(CValue1);
     DirAbsolute:=ActiveDir*CabNo;
     OK:=SendCtrlToNext(command,CValue1,CValue2);
   end
  else if command='CabActivisation' then
   begin
//  OK:=Power>0.01;
//  if OK then
    if (CabNo<>0) then
     LastCab:=CabNo;
    case Trunc(CValue1) of
      1 : CabNo:= 1;
     -1 : CabNo:=-1;
    else CabNo:=0;
    end;
    DirAbsolute:=ActiveDir*CabNo;
    PantCheck; //ewentualnie automatyczna zamiana podniesionych pantografów
    OK:=SendCtrlToNext(command,CValue1,CValue2);
   end
  else if command='AutoRelaySwitch' then
   begin
     if (CValue1=1) and (AutoRelayType=2) then AutoRelayFlag:=True
     else AutoRelayFlag:=False;
     OK:=SendCtrlToNext(command,CValue1,CValue2);
   end
  else if command='FuseSwitch' then
   begin
      if (EngineType=ElectricSeriesMotor) and FuseFlag and (CValue1=1) and
         (MainCtrlActualPos=0) and (ScndCtrlActualPos=0) and Mains then
{      if (EngineType=ElectricSeriesMotor) and (CValue1=1) and
         (MainCtrlActualPos=0) and (ScndCtrlActualPos=0) and Mains then}
        FuseFlag:=False;  {wlaczenie ponowne obwodu}
      OK:=SendCtrlToNext(command,CValue1,CValue2);
   end
  else if command='ConverterSwitch' then         {NBMX}
   begin
     if (CValue1=1) then ConverterAllow:=true
     else if (CValue1=0) then ConverterAllow:=false;
     OK:=SendCtrlToNext(command,CValue1,CValue2);
   end
  else if command='CompressorSwitch' then         {NBMX}
   begin
     if (CValue1=1) then CompressorAllow:=true
     else if (CValue1=0) then CompressorAllow:=false;
     OK:=SendCtrlToNext(command,CValue1,CValue2);
   end
  else if command='DoorLeft' then         {NBMX}
   begin
     if (CValue1=1) then DoorLeftOpened:=true
     else if (CValue1=0) then DoorLeftOpened:=false;
     OK:=SendCtrlToNext(command,CValue1,CValue2);
   end
  else if command='DoorRight' then         {NBMX}
   begin
     if (CValue1=1) then DoorRightOpened:=true
     else if (CValue1=0) then DoorRightOpened:=false;
     OK:=SendCtrlToNext(command,CValue1,CValue2);
   end
else if command='PantFront' then         {Winger 160204}
   begin
     if (TrainType=dt_EZT) then
     begin {'ezt'}
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
     begin {nie 'ezt'}
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
     end;
     OK:=SendCtrlToNext(command,CValue1,CValue2);
   end
  else if command='PantRear' then         {Winger 160204, ABu 310105 i 030305}
   begin
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
     end;
     OK:=SendCtrlToNext(command,CValue1,CValue2);
   end
  else if command='MaxCurrentSwitch' then
   begin
      OK:=MaxCurrentSwitch(CValue1=1);
   end
  else if command='MinCurrentSwitch' then
   begin
      OK:=MinCurrentSwitch(CValue1=1);
   end
{test komend oddzialywujacych na tabor}
  else if command='SetDamage' then
   begin
     if CValue2=1 then OK:=SetFlag(DamageFlag,Trunc(CValue1));
     if CValue2=-1 then OK:=SetFlag(DamageFlag,-Trunc(CValue1));
   end
  else if command='Emergency_brake' then
   begin
     if EmergencyBrakeSwitch(Trunc(CValue1)=1) then
       OK:=True
     else OK:=False;
   end
  else if command='BrakeDelay' then
   begin
      BrakeDelayFlag:=Trunc(CValue1);
      OK:=True;
   end
  else if command='SandDoseOn' then
   begin
     if SandDoseOn then
       OK:=True
     else OK:=False;
   end
  else if command='CabSignal' then {SHP,Indusi}
   begin
     if SecuritySystem.SystemType>1 then
      with SecuritySystem do
       begin
         VelocityAllowed:=Trunc(CValue1);
         NextVelocityAllowed:=Trunc(CValue2);
         SystemSoundTimer:=0;
         SetFlag(Status,s_active);
         OK:=True;
       end
      else OK:=False;
   end
 {naladunek/rozladunek}
  else if Pos('Load=',command)=1 then
   begin
    OK:=False; //bêdzie powtarzane a¿ siê za³aduje
    if (Vel=0) and (MaxLoad>0) and (Load<MaxLoad*(1+OverLoadFactor)) then //czy mo¿na ³adowac?
     if Distance(Loc,CommandIn.Location,Dim,Dim)<10 then //ten peron/rampa
      begin
       testload:=LowerCase(DUE(command));
       if Pos(testload,LoadAccepted)>0 then //nazwa jest obecna w CHK
        OK:=LoadingDone(Min0R(CValue2,LoadSpeed),testload); //zmienia LoadStatus
      end;
    //if OK then LoadStatus:=0; //nie udalo sie w ogole albo juz skonczone
   end
  else if Pos('UnLoad=',command)=1 then
   begin
    OK:=False; //bêdzie powtarzane a¿ siê roz³aduje
    if (Vel=0) and (Load>0) then //czy jest co rozladowac?
     if Distance(Loc,CommandIn.Location,Dim,Dim)<10 then //ten peron
      begin
       testload:=DUE(command); //zgodnoœæ nazwy ³adunku z CHK
       if LoadType=testload then {mozna to rozladowac}
        OK:=LoadingDone(-Min0R(CValue2,LoadSpeed),testload);
      end;
    //if OK then LoadStatus:=0;
   end;
  RunCommand:=OK; //dla true komenda bêdzie usuniêta, dla false wykonana ponownie
{$B-}
End;

function TMoverParameters.RunInternalCommand:boolean;
var OK:boolean;
begin
  if CommandIn.Command<>'' then
   begin
     OK:=RunCommand(CommandIn.Command,CommandIn.Value1,CommandIn.Value2);
     if OK then
      with CommandIn do
       begin
         Command:=''; {kasowanie bo rozkaz wykonany}
         Value1:=0;
         Value2:=0;
         Location.x:=0; Location.y:=0; Location.z:=0;
         if not PhysicActivation then
          Physic_ReActivation;
      end;
   end
  else
   OK:=False;
  RunInternalCommand:=OK;
end;

procedure TMoverParameters.PutCommand(NewCommand:string; NewValue1,NewValue2:real; NewLocation:TLocation);
begin
 CommandLast:=NewCommand; //zapamiêtanie komendy
  with CommandIn do
   begin
     Command:=NewCommand;
     Value1:=NewValue1;
     Value2:=NewValue2;
     Location:=NewLocation;
   end;                         {czy uruchomic tu RunInternalCommand? nie wiem}
end;

{dla samochodow}
function TMoverParameters.ChangeOffsetH(DeltaOffset:real):boolean;
begin
  if TestFlag(CategoryFlag,2) and TestFlag(RunningTrack.CategoryFlag,2) then
   begin
     OffsetTrackH:=OffsetTrackH+DeltaOffset;
     if abs(OffsetTrackH)>(RunningTrack.Width/1.95-TrackW/2.0) then
      ChangeOffsetH:=False  {kola na granicy drogi}
     else
      ChangeOffsetH:=True;
   end
  else ChangeOffsetH:=False;
end;

{inicjalizacja}

constructor TMoverParameters.Init(LocInitial:TLocation; RotInitial:TRotation;
                                  VelInitial:real; TypeNameInit, NameInit: string; LoadInitial:longint; LoadTypeInitial: string; Cab:integer);
                                 {predkosc pocz. w km/h, ilosc ladunku, typ ladunku, numer kabiny}
var b,k:integer;
begin
  {inicjalizacja stalych}
  dMoveLen:=0;
  CategoryFlag:=1;
  EngineType:=None;
  for b:=1 to ResArraySize do
   begin
     RList[b].Relay:=0;
     RList[b].R:=0;
     RList[b].Bn:=0;
     RList[b].Mn:=0;
     RList[b].AutoSwitch:=False;
   end;
  WheelDiameter:=1.0;
  BrakeCtrlPosNo:=0;
  for k:=-1 to MainBrakeMaxPos do
   with BrakePressureTable[k] do
    begin
      PipePressureVal:=0;
      BrakePressureVal:=0;
      FlowSpeedVal:=0;
    end;
   with BrakePressureTable[-2] do  {pozycja odciecia}
    begin
      PipePressureVal:=-1;
      BrakePressureVal:=-1;
      FlowSpeedVal:=0;
    end;
  Transmision.Ratio:=1;
  NBpA:=0;
  DynamicBrakeType:=0;
  ASBType:=0;
  AutoRelayType:=0;
  for b:=0 to 1 do //Ra: kto tu zrobi³ "for b:=1 to 2 do" ???
   with Couplers[b] do
    begin
      CouplerType:=NoCoupler;
      SpringkB:=1; SpringkC:=1;
      DmaxB:=0.1; FmaxB:=1000;
      DmaxC:=0.1; FmaxC:=1000;
    end;
  Power:=0;
  MaxLoad:=0; LoadAccepted:='';
  LoadSpeed:=0; UnLoadSpeed:=0;
  HeatingPower:=0; LightPower:=0;
  with HeatingPowerSource do
   begin
     MaxVoltage:=0; MaxCurrent:=0; IntR:=0.001;
     SourceType:=NotDefined; PowerType:=NoPower; PowerTrans:=NoPower;
   end;
  with AlterHeatPowerSource do
   begin
     MaxVoltage:=0; MaxCurrent:=0; IntR:=0.001;
     SourceType:=NotDefined; PowerType:=NoPower; PowerTrans:=NoPower;
   end;
  with LightPowerSource do
   begin
     MaxVoltage:=0; MaxCurrent:=0; IntR:=0.001;
     SourceType:=NotDefined; PowerType:=NoPower; PowerTrans:=NoPower;
   end;
  with AlterLightPowerSource do
   begin
     MaxVoltage:=0; MaxCurrent:=0; IntR:=0.001;
     SourceType:=NotDefined; PowerType:=NoPower; PowerTrans:=NoPower;
   end;
  TypeName:=TypeNameInit;
  HighPipePress:=0;
  LowPipePress:=0;
  DeltaPipePress:=0;
  BrakeCylNo:=0;
  BrakeCylRadius:=0;
  BrakeCylDist:=0;
  for b:=0 to 3 do
    BrakeCylMult[b]:=0;
  VeselVolume:=0;
  BrakeVolume:=0;
  dizel_Mmax:=1; dizel_nMmax:=1; dizel_Mnmax:=2; dizel_nmax:=2; dizel_nominalfill:=0;
  dizel_Mstand:=0;
  dizel_nmax_cutoff:=0;
  dizel_nmin:=0;
  dizel_minVelfullengage:=0;
  dizel_AIM:=1;
  dizel_engageDia:=0.5; dizel_engageMaxForce:=6000; dizel_engagefriction:=0.5;
  DoorOpenCtrl:=0; DoorCloseCtrl:=0;
  DoorStayOpen:=0;
  DoorClosureWarning:=false;
  DoorOpenSpeed:=1; DoorCloseSpeed:=1;
  DoorMaxShiftL:=0.5; DoorMaxShiftR:=0.5;
  DoorOpenMethod:=2;
  DepartureSignal:=false;
  InsideConsist:=false;
  CompressorPower:=1;
  SmallCompressorPower:=0;

  ScndInMain:=false;

  Vhyp:=1;
  Vadd:=1;
  PowerCorRatio:=1;

  {inicjalizacja zmiennych}
  Loc:=LocInitial;
  Rot:=RotInitial;
  for b:=0 to 1 do
   with Couplers[b] do
    begin
      AllowedFlag:=255; //domyœlnie wszystkie
      CouplingFlag:=0;
      Connected:=nil;
      ConnectedNr:=0; //Ra: to nie ma znaczenia jak nie pod³¹czony
      Render:=false;
      CForce:=0;
      Dist:=0;
      CheckCollision:=False;
    end;
  ScanCounter:=0;
  BrakeCtrlPos:=0;
  BrakeDelayFlag:=0;
  BrakeStatus:=b_off;
  EmergencyBrakeFlag:=False;
  MainCtrlPos:=0;
  ScndCtrlPos:=0;
  MainCtrlActualPos:=0;
  ScndCtrlActualPos:=0;
  Heating:=false;
  Mains:=False;
  ActiveDir:=0; CabNo:=Cab;
  DirAbsolute:=0;
  LastCab:=Cab;
  SlippingWheels:=False;
  SandDose:=False;
  FuseFlag:=False;
  ConvOvldFlag:=false;   //hunter-251211
  StLinFlag:=False;
  ResistorsFlag:=False;
  Rventrot:=0;
  enrot:=0;
  nrot:=0;
  Itot:=0;
  EnginePower:=0;
  BrakePress:=0;
  Compressor:=0;
  ConverterFlag:=false;
  CompressorAllow:=False;
  DoorLeftOpened:=False;
  DoorRightOpened:=false;
  UnBrake:=false;
//Winger 160204
  PantVolume:=3.5;
  PantFrontUp:=false;
  PantRearUp:=false;
  PantFrontStart:=0;
  PantRearStart:=0;
  PantFrontSP:=true;
  PantRearSP:=true;
  DoubleTr:=1;
  BrakeSlippingTimer:=0;
  dpBrake:=0; dpPipe:=0; dpMainValve:=0; dpLocalValve:=0;
  MBPM:=1;
  DynamicBrakeFlag:=False;
  BrakeSystem:=Individual;
  BrakeSubsystem:=Standard;
  Ft:=0; Ff:=0; Fb:=0;
  Ftotal:=0; FStand:=0; FTrain:=0;
  AccS:=0; AccN:=0; AccV:=0;
  EventFlag:=False; SoundFlag:=0;
  Vel:=Abs(VelInitial); V:=VelInitial/3.6;
  LastSwitchingTime:=0;
  LastRelayTime:=0;
  //EndSignalsFlag:=0;
  //HeadSignalsFlag:=0;
  DistCounter:=0;
  PulseForce:=0;
  PulseForceTimer:=0;
  PulseForceCount:=0;
  eAngle:=1.5;
  dizel_fill:=0;
  dizel_engagestate:=0;
  dizel_engage:=0;
  dizel_automaticgearstatus:=0;
  dizel_enginestart:=false;
  dizel_engagedeltaomega:=0;
  PhysicActivation:=true;

  with RunningShape do
   begin
     R:=0; Len:=100; dHtrack:=0; dHrail:=0;
   end;
 RunningTrack.CategoryFlag:=CategoryFlag;
  with RunningTrack do
   begin
     Width:=TrackW; friction:=Steel2Steel_friction;
     QualityFlag:=20;
     DamageFlag:=0;
     Velmax:=100; {dla uzytku maszynisty w ai_driver}
   end;
  with RunningTraction do
   begin
      TractionVoltage:=0;
      TractionFreq:=0;
      TractionMaxCurrent:=0;
      TractionResistivity:=1;
   end;
  OffsetTrackH:=0; OffsetTrackV:=0;
  with CommandIn do
   begin
     Command:='';
     Value1:=0; Value2:=0;
     Location.x:=0; Location.y:=0; Location.z:=0;
   end;

   {czesciowo stale, czesciowo zmienne}
   with SecuritySystem do
    begin
      SystemType:=0;
      AwareDelay:=-1; SoundSignalDelay:=-1; EmergencyBrakeDelay:=-1;
      Status:=0;
      SystemTimer:=0; SystemBrakeTimer:=0;
      VelocityAllowed:=-1; NextVelocityAllowed:=-1;
      RadioStop:=false; //domyœlnie nie ma
    end;
    //ABu 240105:
    //CouplerNr[0]:=1;
    //CouplerNr[1]:=0;

{TO POTEM TU UAKTYWNIC A WYWALIC Z CHECKPARAM}
{
  if Pos(LoadTypeInitial,LoadAccepted)>0 then
   begin
}
     LoadType:=LoadTypeInitial;
     Load:=LoadInitial;
     LoadStatus:=0;
     LastLoadChangeTime:=0;
{
   end
  else Load:=0;
 }

  Name:=NameInit;
 DerailReason:=0; //Ra: powód wykolejenia
end;

function TMoverParameters.EngineDescription(what:integer): string;  {opis stanu lokomotywy}
var outstr: string;
begin
  outstr:='';
  case what of
   0: begin
        if DamageFlag=255 then
         outstr:='Totally destroyed!'
        else
         begin
           if TestFlag(DamageFlag,dtrain_thinwheel) then
            if Power>0.1 then
             outstr:='Thin wheel,'
            else
             outstr:='Load shifted,';
           if TestFlag(DamageFlag,dtrain_wheelwear) then
             outstr:='Wheel wear,';
           if TestFlag(DamageFlag,dtrain_bearing) then
             outstr:='Bearing damaged,';
           if TestFlag(DamageFlag,dtrain_coupling) then
             outstr:='Coupler broken,';
           if TestFlag(DamageFlag,dtrain_loaddamage) then
            if Power>0.1 then
             outstr:='Ventilator damaged,'
            else
             outstr:='Load damaged,';
           if TestFlag(DamageFlag,dtrain_loaddestroyed) then
            if Power>0.1 then
             outstr:='Engine damaged,'
            else
             outstr:='Load destroyed!,';
           if TestFlag(DamageFlag,dtrain_axle) then
             outstr:='Axle broken,';
           if TestFlag(DamageFlag,dtrain_out) then
             outstr:='DERAILED!';
           if outstr='' then
            outstr:='OK!';
         end;
      end;
   else outstr:='Invalid qualifier';
  end;
  EngineDescription:=outstr;
end;

function TMoverParameters.CheckLocomotiveParameters(ReadyFlag:boolean;Dir:longint): boolean;
var //k:integer;
    OK:boolean;
    b:byte;
    bcmsno: byte;
    bcmss: byte;
const
    DefBrakeTable:array[1..8]of Byte=(15,4,25,25,13,3,12,2);
begin
  OK:=True;
  AutoRelayFlag:=(AutoRelayType=1);
  Sand:=SandCapacity;
  if (Pos('o',AxleArangement)>0) and (EngineType=ElectricSeriesMotor) then
   OK:=(Rlist[1].Bn*Rlist[1].Mn=NPoweredAxles); {test poprawnosci ilosci osi indywidualnie napedzanych}
  if (Pos(LoadType,LoadAccepted)=0) and (LoadType<>'') then
   begin
     Load:=0;
     OK:=False;
   end;
  if BrakeSystem=Individual then
   if BrakeSubSystem<>Standard then
    OK:=False; {!}

 if(BrakeVVolume=0)and(MaxBrakePress>0)and(BrakeSystem<>Individual)then
   BrakeVVolume:=MaxBrakePress/(CntrlPipePress-MaxBrakePress)*BrakeVolume;
 if BrakeVVolume=0 then BrakeVVolume:=0.01;

 {to dac potem do init}
  if ReadyFlag then     {gotowy do drogi}
   begin
     Volume:=BrakeVVolume*CntrlPipePress*10;
     if (BrakeSubsystem=Oerlikon) and (((BrakeSystem=Pneumatic)and(Power>1))or(BrakeSystem=ElectroPneumatic)) then
       Volume:=BrakeVVolume*MinCompressor*10;
     CompressedVolume:=VeselVolume*MinCompressor*(9.8+Random);
     ScndPipePress:=MinCompressor*(9.8+Random)/10.0;
     PipePress:=CntrlPipePress;
     LocalBrakePos:=0; //wyluzowany hamulec pomocniczy
     if (BrakeSystem=Pneumatic) and (BrakeCtrlPosNo>0) then
      if CabNo=0 then
       BrakeCtrlPos:=-2; //odciêcie na zespolonym
     MainSwitch(false);
     PantFront(true);
     PantRear(true);
     MainSwitch(true);
     ActiveDir:=Dir;
     DirAbsolute:=ActiveDir*CabNo;
     LimPipePress:=CntrlPipePress;
   end
  else
   begin                {zahamowany}
     Volume:=BrakeVVolume*(MaxBrakePress+CntrlPipePress)*5;
     CompressedVolume:=VeselVolume*MinCompressor*5.5;
     ScndPipePress:=0.5;
     PipePress:=LowPipePress;
     PipeBrakePress:=MaxBrakePress*0.5;
     BrakePress:=MaxBrakePress*0.5;
     LocalBrakePos:=0;
     if (BrakeSystem=Pneumatic) and (BrakeCtrlPosNo>0) then
      BrakeCtrlPos:=-2;
     LimPipePress:=LowPipePress;
     BrakeStatus:=b_on;
   end;
  ActFlowSpeed:=0;

  bcmsno:=0;
  bcmss:=0;

  ComputeConstans();

  for b:=3 downto 0 do
    if BrakeCylMult[b]>0 then
     begin
      bcmsno:=bcmsno+1;
      bcmss:=b;
     end;
  case bcmsno of
   1: BCMFlag:=bcmss;
   2: if (Load>MaxLoad*0.55) then BCMFlag:=bcmss+1 else BCMFlag:=bcmss;
   3: if (Load>MaxLoad*0.70) then BCMFlag:=bcmss+2 else if (Load>MaxLoad*0.35) then BCMFlag:=bcmss+1 else BCMFlag:=bcmss;
   4: if (Load>MaxLoad*0.80) then BCMFlag:=bcmss+3 else if (Load>MaxLoad*0.55) then BCMFlag:=bcmss+2 else if (Load>MaxLoad*0.3) then BCMFlag:=bcmss+1 else BCMFlag:=bcmss;
  end;
   P2FTrans:=1000*Pi*SQR(BrakeCylRadius)*BrakeCylMult[BCMFlag]; {w kN/MPa}

//taki mini automat - powinno byc ladnie dobrze :)
  if(TestFlag(BrakeDelays,bdelay_G))and not(TestFlag(BrakeDelays,bdelay_R))then
    BrakeDelayFlag:=bdelay_G;
  if(TestFlag(BrakeDelays,bdelay_R))and not(TestFlag(BrakeDelays,bdelay_G))then
    BrakeDelayFlag:=bdelay_R;

//yB: jesli pojazdy nie maja zadeklarowanych czasow, to wsadz z przepisow +-16,(6)%
  for b:=1 to 4 do
   begin
     if BrakeDelay[b]=0 then
       BrakeDelay[b]:=DefBrakeTable[b];
     BrakeDelay[b]:=BrakeDelay[b]*(2.5+Random)/3.0;
   end;

  if(TypeName='et22')then
    CompressorPower:=0;

  CheckLocomotiveParameters:=OK;
end;

function TMoverParameters.DoorLeft(State: Boolean):Boolean;
begin
 if (DoorLeftOpened<>State) then
 begin
  DoorLeft:=true;
  DoorLeftOpened:=State;
  if (State=true) then
   begin
     SendCtrlToNext('DoorLeft',1,CabNo);
   end
  else
   begin
    SendCtrlToNext('DoorLeft',0,CabNo);
   end;
  end
  else
    DoorLeft:=false;
end;

function TMoverParameters.DoorRight(State: Boolean):Boolean;
begin
 if (DoorRightOpened<>State) then
  begin
  DoorRight:=true;
  DoorRightOpened:=State;
  if (State=true) then
   begin
     SendCtrlToNext('DoorRight',1,CabNo);
   end
  else
   begin
    SendCtrlToNext('DoorRight',0,CabNo);
   end;
 end
 else
  DoorRight:=false;
end;

//Winger 160204 - pantografy
function TMoverParameters.PantFront(State: Boolean):Boolean;
var pf1: Real;
begin
 PantFront:=true;
 if (State=true) then pf1:=1
  else pf1:=0;
 if (PantFrontUp<>State) then
  begin
  PantFrontUp:=State;
  if (State=true) then
   begin
      PantFrontStart:=0;
      SendCtrlToNext('PantFront',1,CabNo);
   end
  else
   begin
      PantFront:=false;
      PantFrontStart:=1;
      SendCtrlToNext('PantFront',0,CabNo);
      if (TrainType=dt_EZT) then
       begin
        PantRearUp:=false;
        PantRearStart:=1;
        SendCtrlToNext('PantRear',0,CabNo);
       end;
   end;
 end
 else
 SendCtrlToNext('PantFront',pf1,CabNo);
end;

function TMoverParameters.PantRear(State: Boolean):Boolean;
var pf1: Real;
begin
 PantRear:=true;
 if (State=true) then pf1:=1
 else pf1:=0;
 if (PantRearUp<>State) then
  begin
  PantRearUp:=State;
  if (State=true) then
   begin
     PantRearStart:=0;
     SendCtrlToNext('PantRear',1,CabNo);
   end
  else
   begin
     PantRear:=false;
     PantRearStart:=1;
     SendCtrlToNext('PantRear',0,CabNo);
   end;
 end
 else
  SendCtrlToNext('PantRear',pf1,CabNo);
end;



{function TMoverParameters.Heating(State: Boolean):Boolean;
begin
 if (Heating<>State) then
  begin
  Heating:=State;
{  if (State=true) then
   begin
//     SendCtrlToNext('PantRear',1,CabNo);
   end
  else
   begin
//    SendCtrlToNext('PantRear',0,CabNo);
   end; }
{   end
{ else
  SendCtrlToNext('PantRear',0,CabNo); }
{end;}

function TMoverParameters.LoadChkFile(chkpath:string):Boolean;
const param_ok=1; wheels_ok=2; dimensions_ok=4;
var
  lines,s: string;
  bl,i,k:integer;
  b,OKflag:byte;
  fin: text;
  OK: boolean;
function EngineDecode(s:string):TEngineTypes;
 begin
   if s='ElectricSeriesMotor' then
    EngineDecode:=ElectricSeriesMotor
   else if s='DieselEngine' then
    EngineDecode:=DieselEngine
   else if s='SteamEngine' then
    EngineDecode:=SteamEngine
   else if s='WheelsDriven' then
    EngineDecode:=WheelsDriven
   else if s='Dumb' then
    EngineDecode:=Dumb
   else if s='DieselElectric' then
    EngineDecode:=DieselElectric    //youBy: spal-ele
{   else if s='EZT' then {dla kibla}
 {   EngineDecode:=EZT      }
   else EngineDecode:=None;
 end;
function PowerSourceDecode(s:string):TPowerSource;
 begin
   if s='Transducer' then PowerSourceDecode:=Transducer
   else if s='Generator' then PowerSourceDecode:=Generator
   else if s='Accu' then PowerSourceDecode:=Accumulator
   else if s='CurrentCollector' then PowerSourceDecode:=CurrentCollector
   else if s='PowerCable' then PowerSourceDecode:=PowerCable
   else if s='Heater' then PowerSourceDecode:=Heater
   else if s='Internal' then PowerSourceDecode:=InternalSource
   else PowerSourceDecode:=NotDefined;
 end;
function PowerDecode(s:string): TPowerType;
 begin
   if s='BioPower' then PowerDecode:=BioPower
   else if s='BioPower' then PowerDecode:=BioPower
   else if s='MechPower' then PowerDecode:=MechPower
   else if s='ElectricPower' then PowerDecode:=ElectricPower
   else if s='SteamPower' then PowerDecode:=SteamPower
   else PowerDecode:=NoPower;
 end;
 procedure PowerParamDecode(lines,prefix:string; var PowerParamDecode:TPowerParameters);
  begin
    with PowerParamDecode do
     begin
       case SourceType of
        NotDefined : PowerType:=PowerDecode(DUE(ExtractKeyWord(lines,prefix+'PowerType=')));
        InternalSource : PowerType:=PowerDecode(DUE(ExtractKeyWord(lines,prefix+'PowerType=')));
        Transducer : InputVoltage:=s2rE(DUE(ExtractKeyWord(lines,prefix+'TransducerInputV=')));
        Generator  : GeneratorEngine:=EngineDecode(DUE(ExtractKeyWord(lines,prefix+'GeneratorEngine=')));
        Accumulator: begin
                       MaxCapacity:=s2r(DUE(ExtractKeyWord(lines,prefix+'Cap=')));
                       s:=DUE(ExtractKeyWord(lines,prefix+'RS='));
                       RechargeSource:=PowerSourceDecode(s);
                     end;
        CurrentCollector: begin
                            CollectorsNo:=s2bE(DUE(ExtractKeyWord(lines,'CollectorsNo=')));
                            with CollectorParameters do
                             begin
                               MinH:=s2rE(DUE(ExtractKeyWord(lines,'MinH=')));
                               MaxH:=s2rE(DUE(ExtractKeyWord(lines,'MaxH=')));
                               CSW:=s2rE(DUE(ExtractKeyWord(lines,'CSW=')));
                             end;
                          end;
        PowerCable : begin
                       PowerTrans:=PowerDecode(DUE(ExtractKeyWord(lines,prefix+'PowerTrans=')));
                       if PowerTrans=SteamPower then
                        SteamPressure:=s2r(DUE(ExtractKeyWord(lines,prefix+'SteamPress=')));
                     end;
        Heater : begin
                   {jeszcze nie skonczone!}
                 end;
       end;
       if (SourceType<>Heater) and (SourceType<>InternalSource) then
        if not ((SourceType=PowerCable) and (PowerTrans=SteamPower)) then
        begin
{          PowerTrans:=ElectricPower; }
          MaxVoltage:=s2rE(DUE(ExtractKeyWord(lines,prefix+'MaxVoltage=')));
          MaxCurrent:=s2r(DUE(ExtractKeyWord(lines,prefix+'MaxCurrent=')));
          IntR:=s2r(DUE(ExtractKeyWord(lines,prefix+'IntR=')));
        end;
     end;
  end;
begin
  //OK:=True;
  OKflag:=0;
  LineCount:=0;
  ConversionError:=666;
  Mass:=0;
  filename:=chkpath+TypeName+'.chk';
  assignfile(fin,filename);
{$I-}
  reset(fin);
{$I+}
  if IOresult<>0 then
   begin
     OK:=False;
     ConversionError:=-8;
   end
  else
   begin
     ConversionError:=0;
     while not (eof(fin) or (ConversionError<>0)) do
      begin
        readln(fin,lines);
        inc(LineCount);
        if Pos('/',lines)=0 then {nie komentarz}
        begin
          if Pos('Param.',lines)>0 then      {stale parametry}
            begin
              SetFlag(OKFlag,param_ok);
              s:=DUE(ExtractKeyWord(lines,'Category='));
              if s='train' then
               CategoryFlag:=1
              else if s='road' then
               CategoryFlag:=2
              else if s='ship' then
               CategoryFlag:=4
              else if s='airplane' then
               CategoryFlag:=8
              else if s='unimog' then
               CategoryFlag:=3
              else
               ConversionError:=-7;
              s:=ExtractKeyWord(lines,'M=');
              Mass:=s2rE(DUE(s));         {w kg}
              s:=ExtractKeyWord(lines,'Mred='); {zredukowane masy wiruj¹ce}
              if s='' then
               Mred:=0
              else
               Mred:=s2rE(DUE(s));         {w kg}
              s:=ExtractKeyWord(lines,'Vmax=');
              Vmax:=s2rE(DUE(s));     {w km/h}
              s:=ExtractKeyWord(lines,'PWR=');
              Power:=s2r(DUE(s));         {w kW}
              s:=ExtractKeyWord(lines,'HeatingP=');
              HeatingPower:=s2r(DUE(s));  {w kW}
              s:=ExtractKeyWord(lines,'LightP=');
              LightPower:=s2r(DUE(s));    {w kW}
              s:=ExtractKeyWord(lines,'SandCap=');
              SandCapacity:=s2i(DUE(s));  {w kg}
              TrainType:=dt_Default;
              s:=Ups(DUE(ExtractKeyWord(lines,'Type='))); //wielkimi
              if s='EZT' then
               begin
                TrainType:=dt_EZT;
                IminLo:=1;IminHi:=2;Imin:=1; //wirtualne wartoœci dla rozrz¹dczego
               end
              else if s='ET41' then TrainType:=dt_ET41
              else if s='ET42' then TrainType:=dt_ET42
              else if s='ET22' then TrainType:=dt_ET22
              else if s='SN61' then TrainType:=dt_SN61
              else if s='PSEUDODIESEL' then TrainType:=dt_PseudoDiesel
              else if s='181' then TrainType:=dt_181
              else if s='182' then TrainType:=dt_181; {na razie tak}
            end
          else if Pos('Load:',lines)>0 then      {stale parametry}
            begin
              LoadAccepted:=LowerCase(DUE(ExtractKeyWord(lines,'LoadAccepted=')));
              if LoadAccepted<>'' then
               begin
                 s:=ExtractKeyWord(lines,'MaxLoad=');
                 MaxLoad:=s2lE(DUE(s));
                 LoadQuantity:=DUE(ExtractKeyWord(lines,'LoadQ='));
                 s:=ExtractKeyWord(lines,'OverLoadFactor=');
                 OverLoadFactor:=s2r(DUE(s));
                 s:=ExtractKeyWord(lines,'LoadSpeed=');
                 LoadSpeed:=s2rE(DUE(s));
                 s:=ExtractKeyWord(lines,'UnLoadSpeed=');
                 UnLoadSpeed:=s2rE(DUE(s));
               end;
            end
          else if Pos('Dimensions:',lines)>0 then      {wymiary}
           begin
            with Dim do
             begin
              SetFlag(OKFlag,dimensions_ok);
              s:=ExtractKeyWord(lines,'L=');
              L:=s2rE(DUE(s));
              s:=ExtractKeyWord(lines,'H=');
              H:=s2rE(DUE(s));
              s:=ExtractKeyWord(lines,'W=');
              W:=s2rE(DUE(s));
              s:=ExtractKeyWord(lines,'Cx=');
              Cx:=s2r(DUE(s));
              if Cx=0 then Cx:=0.3;
             end;
           end
          else if Pos('Wheels:',lines)>0 then      {kola}
            begin
{              SetFlag(OKFlag,wheels_ok);}
              s:=ExtractKeyWord(lines,'D='); //œrednica osi napêdzaj¹cych
              WheelDiameter:=s2rE(DUE(s));
              s:=ExtractKeyWord(lines,'Dl='); //œrednica przednich osi tocznych
              if s='' then
               WheelDiameterL:=WheelDiameter //gdyby nie by³o parametru, lepsze to ni¿ zero
              else
               WheelDiameterL:=s2rE(DUE(s));
              s:=ExtractKeyWord(lines,'Dt='); //œrednica tylnych osi tocznych
              if s='' then
               WheelDiameterT:=WheelDiameter //gdyby nie by³o parametru, lepsze to ni¿ zero
              else
               WheelDiameterT:=s2rE(DUE(s));
              s:=ExtractKeyWord(lines,'Tw=');
              TrackW:=s2rE(DUE(s));
              s:=ExtractKeyWord(lines,'AIM=');
              AxleInertialMoment:=s2r(DUE(s));
              if AxleInertialMoment<=0 then AxleInertialMoment:=1;
              AxleArangement:=DUE(ExtractKeyWord(lines,'Axle='));
              NPoweredAxles:=s2NPW(AxleArangement);
              NAxles:=NPoweredAxles+s2NNW(AxleArangement);
              if NAxles=0 then ConversionError:=-1;
              s:=DUE(ExtractKeyWord(lines,'BearingType='));
              if s='Roll' then BearingType:=1
              else BearingType:=0;
              s:=ExtractKeyWord(lines,'Ad=');
              ADist:=s2rE(DUE(s));
              s:=ExtractKeyWord(lines,'Bd=');
              BDist:=s2r(DUE(s));
            end
          else if Pos('Brake:',lines)>0 then      {hamulce}
            begin
              s:=DUE(ExtractKeyWord(lines,'BrakeType='));
              if s='Oerlikon' then BrakeSubSystem:=Oerlikon
              else
               if s='Knorr' then BrakeSubSystem:=Knorr
               else
                if s='WeLu' then BrakeSubSystem:=WeLu
                else
                 if s='KE' then BrakeSubSystem:=KE
                 else
                 if s='Hik' then BrakeSubSystem:=Hik
                  else
                  if s='Kunze-Knorr' then BrakeSubSystem:=Kk
                   else BrakeSubSystem:=Standard;
              s:=ExtractKeyWord(lines,'NBpA=');
              NBpA:=s2b(DUE(s));
              s:=ExtractKeyWord(lines,'MBF=');
              MaxBrakeForce:=s2r(DUE(s));
              s:=ExtractKeyWord(lines,'TBF=');
              TrackBrakeForce:=s2r(DUE(s))*1000;
              s:=ExtractKeyWord(lines,'MaxBP=');
              MaxBrakePress:=s2r(DUE(s));
              if MaxBrakePress>0 then
               begin
                 s:=ExtractKeyWord(lines,'BCN=');
                 BrakeCylNo:=s2iE(DUE(s));
                 if BrakeCylNo>0 then
                  begin
                   s:=ExtractKeyWord(lines,'BCR=');
                   BrakeCylRadius:=s2rE(DUE(s));
                   s:=ExtractKeyWord(lines,'BCD=');
                   BrakeCylDist:=s2r(DUE(s));
                   s:=ExtractKeyWord(lines,'BCM0=');
                   BrakeCylMult[0]:=s2r(DUE(s));
                   s:=ExtractKeyWord(lines,'BCMlo=');
                   BrakeCylMult[1]:=s2r(DUE(s));
                   s:=ExtractKeyWord(lines,'BCM=');
                   BrakeCylMult[2]:=s2r(DUE(s));
                   s:=ExtractKeyWord(lines,'BCMmax=');
                   BrakeCylMult[3]:=s2r(DUE(s));
                   P2FTrans:=1000*Pi*SQR(BrakeCylRadius)*BrakeCylMult[2]; {w kN/MPa}
                   BCMFlag:=2;
                   BrakeVolume:=Pi*SQR(BrakeCylRadius)*BrakeCylDist*BrakeCylNo;
                  end
                 else ConversionError:=-5;
               end
              else
               P2FTrans:=0;
              s:=ExtractKeyWord(lines,'HiPP=');
              if s<>'' then CntrlPipePress:=s2r(DUE(s))
               else CntrlPipePress:=0.5;
              HighPipePress:=CntrlPipePress-0.03+0.015*ord((BrakeSubsystem=Oerlikon)or(BrakeSubsystem=KE));
              s:=ExtractKeyWord(lines,'LoPP=');
              if s<>'' then LowPipePress:=s2r(DUE(s))
               else LowPipePress:=Min0R(HighPipePress,0.35);
              DeltaPipePress:=HighPipePress-LowPipePress;
              s:=ExtractKeyWord(lines,'Vv=');
              VeselVolume:=s2r(DUE(s));
              if VeselVolume=0 then
               VeselVolume:=0.01;
              s:=ExtractKeyWord(lines,'MinCP=');
              MinCompressor:=s2r(DUE(s));
              s:=ExtractKeyWord(lines,'MaxCP=');
              MaxCompressor:=s2r(DUE(s));
              s:=ExtractKeyWord(lines,'CompressorSpeed=');
              CompressorSpeed:=s2r(DUE(s));
              s:=DUE(ExtractKeyWord(lines,'CompressorPower='));
              if s='Converter' then
               CompressorPower:=2
              else if s='Main' then
               CompressorPower:=0;
            end
          else if Pos('Doors:',lines)>0 then      {drzwi}
            begin
              s:=DUE(ExtractKeyWord(lines,'OpenCtrl='));
              if s='DriverCtrl' then DoorOpenCtrl:=1
              else
               DoorOpenCtrl:=0;
              s:=DUE(ExtractKeyWord(lines,'CloseCtrl='));
              if s='DriverCtrl' then DoorCloseCtrl:=1
              else
               if s='AutomaticCtrl' then DoorCloseCtrl:=2
                else
                 DoorCloseCtrl:=0;
              if DoorCloseCtrl=2 then
               begin
                 s:=ExtractKeyWord(lines,'DoorStayOpen=');
                 DoorStayOpen:=s2rE(DUE(s));
               end;
              s:=ExtractKeyWord(lines,'OpenSpeed=');
              DoorOpenSpeed:=s2r(DUE(s));
              s:=ExtractKeyWord(lines,'CloseSpeed=');
              DoorCloseSpeed:=s2r(DUE(s));
              s:=ExtractKeyWord(lines,'DoorMaxShiftL=');
              DoorMaxShiftL:=s2r(DUE(s));
              s:=ExtractKeyWord(lines,'DoorMaxShiftR=');
              DoorMaxShiftR:=s2r(DUE(s));
              s:=DUE(ExtractKeyWord(lines,'DoorOpenMethod='));
              if s='Shift' then DoorOpenMethod:=1
              else
               DoorOpenMethod:=2;
              s:=DUE(ExtractKeyWord(lines,'DoorClosureWarning='));
              if s='Yes' then
               DoorClosureWarning:=true
              else
               DoorClosureWarning:=false;
            end
          else if (Pos('BuffCoupl.',lines)>0) or (Pos('BuffCoupl1.',lines)>0) then  {zderzaki i sprzegi}
            begin
             with Couplers[0] do
              begin
                s:=DUE(ExtractKeyWord(lines,'CType='));
                if s='Automatic' then
                 CouplerType:=Automatic
                else
                if s='Screw' then
                 CouplerType:=Screw
                else
                if s='Chain' then
                 CouplerType:=Chain
                else
                if s='Bare' then
                 CouplerType:=Bare
                else
                if s='Articulated' then
                 CouplerType:=Articulated
                else
                 CouplerType:=NoCoupler;
                s:=ExtractKeyWord(lines,'AllowedFlag=');
                if s<>'' then AllowedFlag:=s2NNW(DUE(s));
                if (CouplerType<>NoCoupler) and (CouplerType<>Bare) and (CouplerType<>Articulated) then
                 begin
                   s:=ExtractKeyWord(lines,'kC=');
                   SpringKC:=s2rE(DUE(s))*1000;
                   s:=ExtractKeyWord(lines,'DmaxC=');
                   DmaxC:=s2rE(DUE(s));
                   s:=ExtractKeyWord(lines,'FmaxC=');
                   FmaxC:=s2rE(DUE(s))*1000;
                   s:=ExtractKeyWord(lines,'kB=');
                   SpringKB:=s2rE(DUE(s))*1000;
                   s:=ExtractKeyWord(lines,'DmaxB=');
                   DmaxB:=s2rE(DUE(s));
                   s:=ExtractKeyWord(lines,'FmaxB=');
                   FmaxB:=s2rE(DUE(s))*1000;
                   s:=ExtractKeyWord(lines,'beta=');
                   beta:=s2r(DUE(s));
                 end
                else
                 if (CouplerType=Bare) then {Ra: czemu masy bez ³adunków?}
                  begin
                    SpringKC:=50*Mass+Ftmax/0.05;
                    DmaxC:=0.05;
                    FmaxC:=100*Mass+2*Ftmax;
                    SpringKB:=60*Mass+Ftmax/0.05;
                    DmaxB:=0.05;
                    FmaxB:=50*Mass+2*Ftmax;
                    beta:=0.3;
                  end
                else
                 if (CouplerType=Articulated) then
                  begin
                    SpringKC:=60*Mass+1000;
                    DmaxC:=0.05;
                    FmaxC:=20000000+2*Ftmax;
                    SpringKB:=70*Mass+1000;
                    DmaxB:=0.05;
                    FmaxB:=4000000+2*Ftmax;
                    beta:=0.55;
                  end;
                end;
                if (Pos('BuffCoupl.',lines)>0) then
                 begin
                   Couplers[1].SpringKC:=Couplers[0].SpringKC;
                   Couplers[1].DmaxC:=Couplers[0].DmaxC;
                   Couplers[1].FmaxC:=Couplers[0].FmaxC;
                   Couplers[1].SpringKB:=Couplers[0].SpringKB;
                   Couplers[1].DmaxB:=Couplers[0].DmaxB;
                   Couplers[1].FmaxB:=Couplers[0].FmaxB;
                   Couplers[1].beta:=Couplers[0].beta;
                   Couplers[1].CouplerType:=Couplers[0].CouplerType;
                 end;
{                CouplerTune:=(1+Mass)/100000; }
              end
          else if (Pos('BuffCoupl2.',lines)>0) then  {zderzaki i sprzegi jesli sa rozne}
            with Couplers[1] do
              begin
                s:=DUE(ExtractKeyWord(lines,'CType='));
                if s='Automatic' then
                 CouplerType:=Automatic
                else
                if s='Screw' then
                 CouplerType:=Screw
                else
                if s='Chain' then
                 CouplerType:=Chain
                else
                if s='Bare' then
                 CouplerType:=Bare
                else
                if s='Articulated' then
                 CouplerType:=Articulated
                else
                 CouplerType:=NoCoupler;
                s:=ExtractKeyWord(lines,'AllowedFlag=');
                if s<>'' then AllowedFlag:=s2NNW(DUE(s));
                if (CouplerType<>NoCoupler) and (CouplerType<>Bare) and (CouplerType<>Articulated) then
                 begin
                   s:=ExtractKeyWord(lines,'kC=');
                   SpringKC:=s2rE(DUE(s))*1000;
                   s:=ExtractKeyWord(lines,'DmaxC=');
                   DmaxC:=s2rE(DUE(s));
                   s:=ExtractKeyWord(lines,'FmaxC=');
                   FmaxC:=s2rE(DUE(s))*1000;
                   s:=ExtractKeyWord(lines,'kB=');
                   SpringKB:=s2rE(DUE(s))*1000;
                   s:=ExtractKeyWord(lines,'DmaxB=');
                   DmaxB:=s2rE(DUE(s));
                   s:=ExtractKeyWord(lines,'FmaxB=');
                   FmaxB:=s2rE(DUE(s))*1000;
                   s:=ExtractKeyWord(lines,'beta=');
                   beta:=s2r(DUE(s));
                 end
                else
                 if (CouplerType=Bare) then
                  begin
                    SpringKC:=50*Mass+1000;
                    DmaxC:=0.05;
                    FmaxC:=20*Mass+2*Ftmax;
                    SpringKB:=60*Mass+1000;
                    DmaxB:=0.05;
                    FmaxB:=10*Mass+2*Ftmax;
                    beta:=0.3;
                  end
                else
                 if (CouplerType=Articulated) then
                  begin
                    SpringKC:=60*Mass+1000;
                    DmaxC:=0.05;
                    FmaxC:=20000000+2*Ftmax;
                    SpringKB:=70*Mass+1000;;
                    DmaxB:=0.05;
                    FmaxB:=4000000+2*Ftmax;
                    beta:=0.55;
                  end;
{                CouplerTune:=(1+Mass)/100000; }
              end
          else if Pos('TurboPos:',lines)>0 then      {turbo zalezne od pozycji nastawnika}
           begin
            s:=ExtractKeyWord(lines,'TurboPos=');
            TurboTest:=s2b(DUE(s));
           end
          else if Pos('Cntrl.',lines)>0 then      {nastawniki}
            begin
              s:=DUE(ExtractKeyWord(lines,'BrakeSystem='));
              if s='Pneumatic' then BrakeSystem:=Pneumatic
               else if s='ElectroPneumatic' then BrakeSystem:=ElectroPneumatic
                else BrakeSystem:=Individual;
              if BrakeSystem<>Individual then
               begin
                 s:=ExtractKeyWord(lines,'BCPN=');
                 BrakeCtrlPosNo:=s2b(DUE(s));

                 for b:=1 to 4 do
                  begin
                    s:=ExtractKeyWord(lines,'BDelay'+Cut_Space(b2s(b),CutLeft)+'=');
                    BrakeDelay[b]:=s2b(DUE(s));
                  end;

                 s:=DUE(ExtractKeyWord(lines,'BrakeDelays='));
                 if s='GPR' then Brakedelays:=bdelay_G+bdelay_R
                 else if s='PR' then Brakedelays:=bdelay_R
                 else if s='GP' then Brakedelays:=bdelay_G
                 else if s='DPR+Mg' then begin Brakedelays:=bdelay_R; BrakeMethod:=3; end
                 else if s='DGPR+Mg' then begin Brakedelays:=bdelay_G+bdelay_R; BrakeMethod:=3; end
                 else if s='DGPR' then begin Brakedelays:=bdelay_G+bdelay_R; BrakeMethod:=1; end
                 else if s='DPR' then begin Brakedelays:=bdelay_R; BrakeMethod:=1; end;
//                  else if s='GPR' then Brakedelays=bdelay_G+bdelay_R

                 s:=DUE(ExtractKeyWord(lines,'MaxBPMass='));
                 if s<>'' then
                   MBPM:=s2rE(s)*1000;


                 if BrakeCtrlPosNo>0 then
                  begin
                    s:=DUE(ExtractKeyWord(lines,'ASB='));
                    if s='Manual' then ASBType:=1 else
                     if s='Automatic' then ASBType:=2;
                  end;
               end;
              s:=DUE(ExtractKeyWord(lines,'LocalBrake='));
              if s='ManualBrake' then LocalBrake:=ManualBrake
               else
                if s='PneumaticBrake' then LocalBrake:=PneumaticBrake
                  else
                    if s='HydraulicBrake' then LocalBrake:=HydraulicBrake
                      else LocalBrake:=NoBrake;
             s:=DUE(ExtractKeyWord(lines,'DynamicBrake='));
             if s='Passive' then DynamicBrakeType:=dbrake_passive
              else
               if s='Switch' then DynamicBrakeType:=dbrake_switch
                else
                 if s='Reversal' then DynamicBrakeType:=dbrake_reversal
                  else
                   if s='Automatic' then DynamicBrakeType:=dbrake_automatic
                    else DynamicBrakeType:=dbrake_none;
              s:=ExtractKeyWord(lines,'MCPN=');
              MainCtrlPosNo:=s2b(DUE(s));
              s:=ExtractKeyWord(lines,'SCPN=');
              ScndCtrlPosNo:=s2b(DUE(s));
              s:=ExtractKeyWord(lines,'SCIM=');
              ScndinMain:=boolean(s2b(DUE(s)));
              s:=DUE(ExtractKeyWord(lines,'AutoRelay='));
              if s='Optional' then
               AutoRelayType:=2
              else
               if s='Yes' then
                AutoRelayType:=1
               else
                AutoRelayType:=0;
              s:=DUE(ExtractKeyWord(lines,'CoupledCtrl='));
              if s='Yes' then
               CoupledCtrl:=True    {wspolny wal}
              else CoupledCtrl:=False;
              s:=ExtractKeyWord(lines,'IniCDelay=');
              InitialCtrlDelay:=s2r(DUE(s));
              s:=ExtractKeyWord(lines,'SCDelay=');
              CtrlDelay:=s2r(DUE(s));
              if BrakeCtrlPosNo>0 then
               for i:=0 to BrakeCtrlPosNo+1 do
                begin
                  s:=ReadWord(fin);
                  k:=s2iE(s);
                  if ConversionError=0 then
                   with BrakePressureTable[k] do
                    begin
                      read(fin,PipePressureVal,BrakePressureVal,FlowSpeedVal);
                      s:=ReadWord(fin);
                      if s='Pneumatic' then BrakeType:=Pneumatic
                       else if s='ElectroPneumatic' then BrakeType:=ElectroPneumatic
                        else BrakeType:=Individual;
{                      readln(fin); }
                    end;
                end;
            end
          else if Pos('Light:',lines)>0 then      {zrodlo mocy dla oswietlenia}
            begin
              s:=DUE(ExtractKeyWord(lines,'Light='));
              if s<>'' then
               begin
                 LightPowerSource.SourceType:=PowerSourceDecode(s);
                 PowerParamDecode(lines,'L',LightPowerSource);
                 s:=DUE(ExtractKeyWord(lines,'AlterLight='));
                 if s<>'' then
                  begin
                    AlterLightPowerSource.SourceType:=PowerSourceDecode(s);
                    PowerParamDecode(lines,'AlterL',AlterLightPowerSource)
                  end
                 else AlterLightPowerSource.SourceType:=NotDefined;
               end
              else LightPowerSource.SourceType:=NotDefined;
            end
          else if Pos('Security:',lines)>0 then      {zrodlo mocy dla oswietlenia}
            with SecuritySystem do
             begin
               s:=DUE(ExtractKeyWord(lines,'AwareSystem='));
               if Pos('Active',s)>0 then
                SetFlag(SystemType,1);
               if Pos('CabSignal',s)>0 then
                SetFlag(SystemType,2);
               s:=ExtractKeyWord(lines,'AwareDelay=');
               if s<>'' then
                AwareDelay:=s2r(DUE(s));
               s:=ExtractKeyWord(lines,'SoundSignalDelay=');
               if s<>'' then
                 SoundSignalDelay:=s2r(DUE(s));
                s:=ExtractKeyWord(lines,'EmergencyBrakeDelay=');
               if s<>'' then
                EmergencyBrakeDelay:=s2r(DUE(s));
               s:=ExtractKeyWord(lines,'RadioStop=');
               if s<>'' then
                if Pos('Yes',s)>0 then
                 RadioStop:=true;
             end
          else if Pos('Clima:',lines)>0 then      {zrodlo mocy dla ogrzewania itp}
            begin
              s:=DUE(ExtractKeyWord(lines,'Heating='));
              if s<>'' then
               begin
                 HeatingPowerSource.SourceType:=PowerSourceDecode(s);
                 PowerParamDecode(lines,'H',HeatingPowerSource);
                 s:=DUE(ExtractKeyWord(lines,'AlterHeating='));
                 if s<>'' then
                  begin
                    AlterHeatPowerSource.SourceType:=PowerSourceDecode(s);
                    PowerParamDecode(lines,'AlterH',AlterHeatPowerSource)
                  end
                 else AlterHeatPowerSource.SourceType:=NotDefined;
               end
              else HeatingPowerSource.SourceType:=NotDefined;
            end
          else if Pos('Power:',lines)>0 then      {zrodlo mocy dla silnikow trakcyjnych itp}
            begin
              s:=DUE(ExtractKeyWord(lines,'EnginePower='));
              if s<>'' then
               begin
                 EnginePowerSource.SourceType:=PowerSourceDecode(s);
                 PowerParamDecode(lines,'',EnginePowerSource);
                 if (EnginePowerSource.SourceType=Generator) and (EnginePowerSource.GeneratorEngine=WheelsDriven) then
                  ConversionError:=-666;  {perpetuum mobile?}
               end
              else EnginePowerSource.SourceType:=NotDefined;
              if EnginePowerSource.SourceType=NotDefined then
               begin
                 s:=DUE(ExtractKeyWord(lines,'SystemPower='));
                 if s<>'' then
                  begin
                    SystemPowerSource.SourceType:=PowerSourceDecode(s);
                    PowerParamDecode(lines,'',SystemPowerSource);
                  end
                 else EnginePowerSource.SourceType:=NotDefined;
               end
              else SystemPowerSource.SourceType:=InternalSource;
            end
          else if Pos('Engine:',lines)>0 then    {stale parametry silnika}
          begin
           EngineType:=EngineDecode(DUE(ExtractKeyWord(lines,'EngineType=')));
           case EngineType of
             ElectricSeriesMotor:
               begin
                 s:=ExtractKeyWord(lines,'Volt=');
                 NominalVoltage:=s2r(DUE(s));
                 s:=DUE(ExtractKeyWord(lines,'Trans='));
                 Transmision.NToothW:=s2b(copy(s,Pos(':',s)+1,Length(s)));
                 Transmision.NToothM:=s2b(copy(s,1,Pos(':',s)-1));
                 if s<>'' then
                    with Transmision do
                     if NToothM>0 then
                      Ratio:=NToothW/NToothM
                     else Ratio:=1;
                 s:=ExtractKeyWord(lines,'WindingRes=');
                 WindingRes:=s2r(DUE(s));
                 if WindingRes=0 then WindingRes:=0.01;
                 s:=ExtractKeyWord(lines,'nmax=');
                 nmax:=s2rE(DUE(s))/60.0;
               end;
             WheelsDriven:
               begin
                 s:=DUE(ExtractKeyWord(lines,'Trans='));
                 Transmision.NToothW:=s2b(copy(s,Pos(':',s)+1,Length(s)));
                 Transmision.NToothM:=s2b(copy(s,1,Pos(':',s)-1));
                 if s<>'' then
                    with Transmision do
                     if NToothM>0 then
                      Ratio:=NToothW/NToothM
                     else Ratio:=1;
                 s:=ExtractKeyWord(lines,'Ftmax=');
                 Ftmax:=s2rE(DUE(s));
               end;
             Dumb:
               begin
                 s:=ExtractKeyWord(lines,'Ftmax=');
                 Ftmax:=s2rE(DUE(s));
               end;
             DieselEngine:
               begin
                 s:=DUE(ExtractKeyWord(lines,'Trans='));
                 Transmision.NToothW:=s2b(copy(s,Pos(':',s)+1,Length(s)));
                 Transmision.NToothM:=s2b(copy(s,1,Pos(':',s)-1));
                 if s<>'' then
                    with Transmision do
                     if NToothM>0 then
                      Ratio:=NToothW/NToothM
                     else Ratio:=1;
                 s:=ExtractKeyWord(lines,'nmin=');
                 dizel_nmin:=s2r(DUE(s))/60.0;
                 s:=ExtractKeyWord(lines,'nmax=');
                 nmax:=s2rE(DUE(s))/60.0;
                 s:=ExtractKeyWord(lines,'nmax_cutoff=');
                 dizel_nmax_cutoff:=s2r(DUE(s))/60.0;
                 s:=ExtractKeyWord(lines,'AIM=');
                 dizel_AIM:=s2r(DUE(s));
               end;
             DieselElectric: //youBy
               begin
                 s:=DUE(ExtractKeyWord(lines,'Trans='));
                 Transmision.NToothW:=s2b(copy(s,Pos(':',s)+1,Length(s)));
                 Transmision.NToothM:=s2b(copy(s,1,Pos(':',s)-1));
                 if s<>'' then
                    with Transmision do
                     if NToothM>0 then
                      Ratio:=NToothW/NToothM
                     else Ratio:=1;
                 s:=ExtractKeyWord(lines,'Ftmax=');
                 Ftmax:=s2rE(DUE(s));
                 s:=ExtractKeyWord(lines,'Flat=');
                 Flat:=Boolean(s2b(DUE(s)));
                 s:=ExtractKeyWord(lines,'Vhyp=');
                 Vhyp:=s2rE(DUE(s))/3.6;
                 s:=ExtractKeyWord(lines,'Vadd=');
                 Vadd:=s2rE(DUE(s))/3.6;
                 s:=ExtractKeyWord(lines,'Cr=');
                 PowerCorRatio:=s2rE(DUE(s));
                 s:=ExtractKeyWord(lines,'RelayType=');
                 RelayType:=s2b(DUE(s));
                 s:=ExtractKeyWord(lines,'ShuntMode=');
                 ShuntModeAllow:=Boolean(s2b(DUE(s)));
                 if (ShuntModeAllow) then
                 begin
                   ShuntModeAllow:=True;
                   ShuntMode:=False;
                   AnPos:=0;
                   ImaxHi:=2;
                   ImaxLo:=1;
                 end;
               end;               
             { EZT:  {NBMX ¿eby kibel dzialal}
              { begin
               end;}
           else ConversionError:=-13; {not implemented yet!}
           end;
          end
           { Typy przelacznika 'Winger 010304 }
          else if Pos('Switches:',lines)>0 then
          begin
           s:=DUE(ExtractKeyWord(lines,'Pantograph='));
           PantSwitchType:=s;
           s:=DUE(ExtractKeyWord(lines,'Converter='));
           ConvSwitchType:=s;
           {case PantSwitchType of
             Continuos:
               begin
               PantSwitchType:='continuos';
               end;
             Impulse:
               begin
               PantSwitchType:='impulse';
               end;     }
          end

          else if Pos('MotorParamTable:',lines)>0 then
           case EngineType of
            ElectricSeriesMotor:
             begin
               for k:=0 to ScndCtrlPosNo do
                begin
                  with MotorParam[k] do
                  read(fin, bl, mfi,mIsat, fi,Isat);
                  if AutoRelayType=0 then
                   MotorParam[k].AutoSwitch:=False
                  else begin
                         readln(fin,i);
                         MotorParam[k].AutoSwitch:=Boolean(i);
                       end;
                  if bl<>k then
                   ConversionError:=-2
                end;
             end;
//youBy
            DieselElectric:
             begin
//               WW_MPTRelayNo:= ScndCtrlPosNo;
               for k:=0 to ScndCtrlPosNo do
                begin
                  with MotorParam[k] do
                  readln(fin, bl, mfi, mIsat, fi, Isat, MPTRelay[k].Iup, MPTRelay[k].Idown);
                  if bl<>k then
                   ConversionError:=-2
                end;
             end;

            DieselEngine:
             begin
               s:=ExtractKeyWord(lines,'minVelfullengage=');
               dizel_minVelfullengage:=s2r(DUE(s));
               s:=ExtractKeyWord(lines,'engageDia=');
               dizel_engageDia:=s2r(DUE(s));
               s:=ExtractKeyWord(lines,'engageMaxForce=');
               dizel_engageMaxForce:=s2r(DUE(s));
               s:=ExtractKeyWord(lines,'engagefriction=');
               dizel_engagefriction:=s2r(DUE(s));

               for k:=0 to ScndCtrlPosNo do
                begin
                  with MotorParam[k] do
                  read(fin, bl, mIsat, fi,mfi);
                  if AutoRelayType=0 then
                   MotorParam[k].AutoSwitch:=False
                  else begin
                         readln(fin,i);
                         MotorParam[k].AutoSwitch:=Boolean(i);
                       end;
                  if bl<>k then
                   ConversionError:=-2
                end;
             end
            else ConversionError:=-3;
           end
          else if Pos('Circuit:',lines)>0 then
           begin
             s:=ExtractKeyWord(lines,'CircuitRes=');
             CircuitRes:=s2r(DUE(s));
             s:=ExtractKeyWord(lines,'IminLo=');
             IminLo:=s2iE(DUE(s));
             s:=ExtractKeyWord(lines,'IminHi=');
             IminHi:=s2i(DUE(s));
             s:=ExtractKeyWord(lines,'ImaxLo=');
             ImaxLo:=s2iE(DUE(s));
             s:=ExtractKeyWord(lines,'ImaxHi=');
             ImaxHi:=s2i(DUE(s));
             Imin:=IminLo;
             Imax:=ImaxLo;
           end
          else if Pos('RList:',lines)>0 then
           begin
             s:=DUE(ExtractKeyWord(lines,'RVent='));
             if s='Automatic' then RventType:=2
             else
             if s='Yes' then RventType:=1
             else
              RventType:=0;
             if RventType>0 then
              begin
                s:=ExtractKeyWord(lines,'RVentnmax=');
                RVentnmax:=s2rE(DUE(s))/60.0;
                s:=ExtractKeyWord(lines,'RVentCutOff=');
                RVentCutOff:=s2r(DUE(s));
              end;
             RlistSize:=s2b(DUE(ExtractKeyWord(lines,'Size=')));
             if RlistSize>ResArraySize then
              ConversionError:=-4
             else
              for k:=0 to RlistSize do
               begin
                 read(fin, Rlist[k].Relay, Rlist[k].R, Rlist[k].Bn, Rlist[k].Mn);
{                 if AutoRelayType=0 then
                  Rlist[k].AutoSwitch:=False
                 else begin  }  {to sie przyda do pozycji przejsciowych}
                 read(fin,i);
                 Rlist[k].AutoSwitch:=Boolean(i);
                 if(ScndInMain)then
                  begin
                   readln(fin,i);
                   Rlist[k].ScndAct:=i;
                  end
                 else readln(fin);
               end;
           end
          else if Pos('DList:',lines)>0 then  {dla spalinowego silnika}
           begin
             s:=ExtractKeyWord(lines,'Mmax=');
             dizel_Mmax:=s2rE(DUE(s));
             s:=ExtractKeyWord(lines,'nMmax=');
             dizel_nMmax:=s2rE(DUE(s));
             s:=ExtractKeyWord(lines,'Mnmax=');
             dizel_Mnmax:=s2rE(DUE(s));
             s:=ExtractKeyWord(lines,'nmax=');
             dizel_nmax:=s2rE(DUE(s));
             s:=ExtractKeyWord(lines,'nominalfill=');
             dizel_nominalfill:=s2rE(DUE(s));
             s:=ExtractKeyWord(lines,'Mstand=');
             dizel_Mstand:=s2rE(DUE(s));
             RlistSize:=s2b(DUE(ExtractKeyWord(lines,'Size=')));
             if RlistSize>ResArraySize then
              ConversionError:=-4
             else
              for k:=0 to RlistSize do
               begin
                 readln(fin, Rlist[k].Relay, Rlist[k].R, Rlist[k].Mn);
               end;
           end
//youBy
          else if (Pos('WWList:',lines)>0) then  {dla spal-ele}
          begin
            RlistSize:=s2b(DUE(ExtractKeyWord(lines,'Size=')));
            for k:=0 to RlistSize do
            begin
              if not (ShuntModeAllow) then
                readln(fin, DEList[k].rpm, DEList[k].genpower, DEList[k].Umax, DEList[k].Imax)
              else
              begin
                readln(fin, DEList[k].rpm, DEList[k].genpower, DEList[k].Umax, DEList[k].Imax, SST[k].Umin, SST[k].Umax, SST[k].Pmax);
                SST[k].Pmin:=sqrt(sqr(SST[k].Umin)/47.6);
                SST[k].Pmax:=Min0R(SST[k].Pmax,sqr(SST[k].Umax)/47.6);
              end;
            end;
          end;
        end;
      end; {koniec filtru importu parametrow}
     if ConversionError=0 then
      OK:=True
     else
      OK:=False;
   end; {otwarty plik}
  if OK then
   if OKflag<>param_ok{+wheels_ok}+dimensions_ok then
    begin
      OK:=False;
      ConversionError:=-OKflag; {brakowalo jakiejs waznej linii z parametrami}
    end;
  if OK and (Mass<=0) then
   begin
     OK:=False;
     ConversionError:=-666;
   end;
  LoadChkFile:=OK;
  if ConversionError<>-8 then
   close(fin);
end; {loadchkfile}


END.



