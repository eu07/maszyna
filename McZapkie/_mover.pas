unit _mover;          {fizyka ruchu dla symulatora lokomotywy}

(*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Maciej Czapkiewicz and others

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
35. Dodane baterie akumulatorow (KURS90)
36. Hamowanie elektrodynamiczne w ET42 i poprawione w EP09
37. jednokierunkowosc walu kulakowego w EZT (nie do konca)
38. wal kulakowy w ET40
39. poprawiona blokada nastawnika w ET40
...
*)


interface uses mctools,sysutils,hamulce,Oerlikon_ESt;

CONST
   Go=true;
   Hold=false;                 {dla CheckLocomotiveParameters}
   ResArraySize=64;            {dla silnikow elektrycznych}
   MotorParametersArraySize=10;
   maxcc=4;                    {max. ilosc odbierakow pradu}
   LocalBrakePosNo=10;         {ilosc nastaw hamulca pomocniczego}
   MainBrakeMaxPos=10;          {max. ilosc nastaw hamulca zasadniczego}
   ManualBrakePosNo=20;        {ilosc nastaw hamulca recznego}

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
   ctrain_heating=64;       //przewody ogrzewania WN
   ctrain_depot=128;        //nie roz³¹czalny podczas zwyk³ych manewrów (miêdzycz³onowy), we wpisie wartoœæ ujemna

   {typ hamulca elektrodynamicznego}
   dbrake_none=0;
   dbrake_passive=1;
   dbrake_switch=2;
   dbrake_reversal=4;
   dbrake_automatic=8;

   {status czuwaka/SHP}
   //hunter-091012: rozdzielenie alarmow, dodanie testu czuwaka
   s_waiting=1; //dzia³a
   s_aware=2;   //czuwak miga
   s_active=4;  //SHP œwieci
   s_CAalarm=8;    //buczy
   s_SHPalarm=16;  //buczy
   s_CAebrake=32;  //hamuje
   s_SHPebrake=64; //hamuje
   s_CAtest=128;
   
   {dzwieki}
   sound_none=0;
   sound_loud=1;
   sound_couplerstretch=2;
   sound_bufferclamp=4;
   sound_bufferbump=8;
   sound_relay=16;
   sound_manyrelay=32;
   sound_brakeacc=64;

   PhysicActivationFlag: boolean=false;

   //szczególne typy pojazdów (inna obs³uga) dla zmiennej TrainType
   //zamienione na flagi bitowe, aby szybko wybieraæ grupê (np. EZT+SZT)
   dt_Default=0;
   dt_EZT=1;
   dt_ET41=2;
   dt_ET42=4;
   dt_PseudoDiesel=8;
   dt_ET22=$10; //u¿ywane od Megapacka
   dt_SN61=$20; //nie u¿ywane w warunkach, ale ustawiane z CHK
   dt_EP05=$40;
   dt_ET40=$80;
   dt_181=$100;

//sta³e dla asynchronów
  eimc_s_dfic=0;
  eimc_s_dfmax=1;
  eimc_s_p=2;
  eimc_s_cfu=3;
  eimc_s_cim=4;
  eimc_s_icif=5;
  eimc_f_Uzmax=7;
  eimc_f_Uzh=8;
  eimc_f_DU=9;
  eimc_f_I0=10;
  eimc_f_cfu=11;
  eimc_p_F0=13;
  eimc_p_a1=14;
  eimc_p_Pmax=15;
  eimc_p_Fh=16;
  eimc_p_Ph=17;
  eimc_p_Vh0=18;
  eimc_p_Vh1=19;
  eimc_p_Imax=20;

//zmienne dla asynchronów
  eimv_FMAXMAX=0;
  eimv_Fmax=1;
  eimv_ks=2;
  eimv_df=3;
  eimv_fp=4;
  eimv_U=5;
  eimv_pole=6;
  eimv_Ic=7;
  eimv_If=8;
  eimv_M=9;
  eimv_Fr=10;
  eimv_Ipoj=11;
  eimv_Pm=12;
  eimv_Pe=13;
  eimv_eta=14;
  eimv_fkr=15;
  eimv_Uzsmax=16;
  eimv_Pmax=17;
  eimv_Fzad=18;
  eimv_Imax=19;


TYPE
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

    {typy hamulcow zespolonych}
    TBrakeSystem = (Individual, Pneumatic, ElectroPneumatic);
    {podtypy hamulcow zespolonych}
    TBrakeSubSystem = (ss_None, ss_W, ss_K, ss_KK, ss_Hik, ss_ESt, ss_KE, ss_LSt, ss_MT, ss_Dako);
    TBrakeValve = (NoValve, W, W_Lu_VI, W_Lu_L, W_Lu_XR, K, Kg, Kp, Kss, Kkg, Kkp, Kks, Hikg1, Hikss, Hikp1, KE, SW, EStED, NESt3, ESt3, LSt, ESt4, ESt3AL2, EP1, EP2, M483, CV1_L_TR, CV1, CV1_R, Other);
    TBrakeHandle = (NoHandle, West, FV4a, M394, M254, FVel1, FVel6, D2, Knorr, FD1, BS2, testH, St113);
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

    {typy napedow}
    TEngineTypes = (None, Dumb, WheelsDriven, ElectricSeriesMotor, ElectricInductionMotor, DieselEngine, SteamEngine, DieselElectric);
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
                          CollectorsNo: longint; //musi byæ tu, bo inaczej siê kopie
                          MinH,MaxH: real; //zakres ruchu pantografu, nigdzie nie u¿ywany
                          CSW: real;       //szerokoœæ czêœci roboczej (styku) œlizgacza
                          MinV,MaxV: real; //minimalne i maksymalne akceptowane napiêcie
                          InsetV: real;    //minimalne napiêcie wymagane do za³¹czenia
                          MinPress: real;  //minimalne ciœnienie do za³¹czenia WS
                          MaxPress: real;  //maksymalne ciœnienie za reduktorem
                        end;
    {typy Ÿróde³ mocy}
    TPowerSource = (NotDefined, InternalSource, Transducer, Generator, Accumulator, CurrentCollector, PowerCable, Heater);
    {parametry Ÿróde³ mocy}
    TPowerParameters = record
                         MaxVoltage: real;
                         MaxCurrent: real;
                         IntR: real;
                         case SourceType: TPowerSource of
                          NotDefined,InternalSource : (PowerType: TPowerType);
                          Transducer : (InputVoltage: real);
                          Generator  : (GeneratorEngine: TEngineTypes);
                          Accumulator: (RAccumulator:record MaxCapacity:real; RechargeSource:TPowerSource end);
                          CurrentCollector: (CollectorParameters: TCurrentCollector);
                          PowerCable : (RPowerCable:record PowerTrans:TPowerType; SteamPressure:real end);
                          Heater : (RHeater:record Grate:TGrateType; Boiler:TBoilerType end);
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
                        mfi,mIsat,mfi0:real; // aproksymacja M(I) silnika} {dla dizla mIsat=przekladnia biegu
                        fi,Isat,fi0: real;  // aproksymacja E(n)=fi*n}    {dla dizla fi, mfi: predkosci przelozenia biegu <->
                        AutoSwitch: boolean;
                       end;

    TSecuritySystem= record
                       SystemType:byte; {0: brak, 1: czuwak aktywny, 2: SHP/sygnalizacja kabinowa}
                       AwareDelay:real; //czas powtarzania czuwaka
                       AwareMinSpeed:real; //minimalna prêdkoœæ za³¹czenia czuwaka, normalnie 10% Vmax
                       SoundSignalDelay,EmergencyBrakeDelay:real;
                       Status: byte;     {0: wylaczony, 1: wlaczony, 2: czuwak, 4: shp, 8: alarm, 16: hamowanie awaryjne}
                       //SystemTimer, SystemSoundTimer, SystemBrakeTimer: real;
                       SystemTimer, SystemSoundCATimer, SystemSoundSHPTimer, SystemBrakeCATimer, SystemBrakeSHPTimer, SystemBrakeCATestTimer: real; //hunter-091012
                       VelocityAllowed, NextVelocityAllowed: integer; {predkosc pokazywana przez sygnalizacje kabinowa}
                       RadioStop:boolean; //czy jest RadioStop
                     end;

 //Ra: to oddzielnie, bo siê kompilatorowi miesza
 TTransmision=
  record  //liczba zebow przekladni}
   NToothM, NToothW : byte;
   Ratio: real;  {NToothW/NToothM}
  end;

 {sprzegi}
 TCouplerType=(NoCoupler,Articulated,Bare,Chain,Screw,Automatic);
 //Ra: T_MoverParameters jako klasa jest ju¿ wskaŸnikiem na poziomie Pascala
 //P_MoverParameters=^T_MoverParameters
 T_MoverParameters=class;
 TCoupling = record
               {parametry}
               SpringKB,SpringKC,beta:real;   {stala sprezystosci zderzaka/sprzegu, %tlumiennosci }
               DmaxB,FmaxB,DmaxC,FmaxC: real; {tolerancja scisku/rozciagania, sila rozerwania}
               CouplerType: TCouplerType;     {typ sprzegu}
               {zmienne}
               CouplingFlag : byte; {0 - wirtualnie, 1 - sprzegi, 2 - pneumatycznie, 4 - sterowanie, 8 - kabel mocy}
               AllowedFlag : Integer;       //Ra: znaczenie jak wy¿ej, maska dostêpnych
               Render: boolean;             {ABu: czy rysowac jak zaczepiony sprzeg}
               CoupleDist: real;            {ABu: optymalizacja - liczenie odleglosci raz na klatkê, bez iteracji}
               Connected: T_MoverParameters; {co jest podlaczone}
               ConnectedNr: byte;           //Ra: od której strony pod³¹czony do (Connected): 0=przód, 1=ty³
               CForce: real;                {sila z jaka dzialal}
               Dist: real;                  {strzalka ugiecia zderzaków}
               CheckCollision: boolean;     {czy sprawdzac sile czy pedy}
             end;

 //TCouplers= array[0..1] of TCoupling;
 //TCouplerNr= array[0..1] of byte; //ABu: nr sprzegu z ktorym polaczony; Ra: wrzuciæ do TCoupling

    {----------------------------------------------}
    {glowny obiekt}
    T_MoverParameters = class(TObject) //Ra: dodana kreska w celu zrobienia wrappera
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
               BatteryVoltage: real;        {Winger - baterie w elektrykach}
               Battery: boolean; {Czy sa zalavzone baterie}
               EpFuse: boolean; {Czy sa zalavzone baterie}
               Signalling: boolean;         {Czy jest zalaczona sygnalizacja hamowania ostatniego wagonu}
               DoorSignalling: boolean;         {Czy jest zalaczona sygnalizacja blokady drzwi}
               Radio: boolean;         {Czy jest zalaczony radiotelefon}
               NominalBatteryVoltage: real;        {Winger - baterie w elektrykach}
               Dim: TDimension;          {wymiary}
               Cx: real;                 {wsp. op. aerodyn.}
               Floor: real;              //poziom pod³ogi dla ³adunków
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
               BrakeValve: TBrakeValve;
               BrakeHandle: TBrakeHandle;
               BrakeLocHandle: TBrakeHandle;
               MBPM: real; {masa najwiekszego cisnienia}

               Hamulec: TBrake;
               Handle: THandle;
               LocHandle: THandle;
               Pipe, Pipe2: TReservoir;

               LocalBrake: TLocalBrake;  {rodzaj hamulca indywidualnego}
               BrakePressureTable: TBrakePressureTable; {wyszczegolnienie cisnien w rurze}
               BrakePressureActual: TBrakePressure; //wartoœci wa¿one dla aktualnej pozycji kranu
               ASBType: byte;            {0: brak hamulca przeciwposlizgowego, 1: reczny, 2: automat}
               TurboTest: byte;
               MaxBrakeForce: real;      {maksymalna sila nacisku hamulca}
               MaxBrakePress: array[0..4] of real; //pomocniczy, proz, sred, lad, pp
               P2FTrans: real;
               TrackBrakeForce: real;    {sila nacisku hamulca szynowego}
               BrakeMethod: byte;        {flaga rodzaju hamulca}
               {max. cisnienie w cyl. ham., stala proporcjonalnosci p-K}
               HighPipePress,LowPipePress,DeltaPipePress: real;
               {max. i min. robocze cisnienie w przewodzie glownym oraz roznica miedzy nimi}
               CntrlPipePress: real; //ciœnienie z zbiorniku steruj¹cym
               BrakeVolume, BrakeVVolume, VeselVolume: real;
               {pojemnosc powietrza w ukladzie hamulcowym, w ukladzie glownej sprezarki [m^3] }
               BrakeCylNo: integer; {ilosc cylindrow ham.}
               BrakeCylRadius, BrakeCylDist: real;
               BrakeCylMult: array[0..2] of real;
               LoadFlag: byte;
               {promien cylindra, skok cylindra, przekladnia hamulcowa}
               BrakeCylSpring: real; {suma nacisku sprezyn powrotnych, kN}
               BrakeSlckAdj: real; {opor nastawiacza skoku tloka, kN}
               BrakeRigEff: real; {sprawnosc przekladni dzwigniowej}               
               RapidMult: real; {przelozenie rapidu}
               BrakeValveSize: integer;
               BrakeValveParams: string;
               Spg: real;
               MinCompressor,MaxCompressor,CompressorSpeed:real;
               {cisnienie wlaczania, zalaczania sprezarki, wydajnosc sprezarki}
               BrakeDelay: TBrakeDelayTable; {opoznienie hamowania/odhamowania t/o}
               BrakeCtrlPosNo:byte;     {ilosc pozycji hamulca}
               {nastawniki:}
               MainCtrlPosNo: byte;     {ilosc pozycji nastawnika}
               ScndCtrlPosNo: byte;
               ScndInMain: boolean;     {zaleznosc bocznika od nastawnika}
               MBrake: boolean;     {Czy jest hamulec reczny}
               SecuritySystem: TSecuritySystem;

                       {-sekcja parametrow dla lokomotywy elektrycznej}
               RList: TSchemeTable;     {lista rezystorow rozruchowych i polaczen silnikow, dla dizla: napelnienia}
               RlistSize: integer;
               MotorParam: array[0..MotorParametersArraySize] of TMotorParameters;
                                        {rozne parametry silnika przy bocznikowaniach}
                                        {dla lokomotywy spalinowej z przekladnia mechaniczna: przelozenia biegow}
               Transmision : TTransmision;
                // record   {liczba zebow przekladni}
                //  NToothM, NToothW : byte;
                //  Ratio: real;  {NToothW/NToothM}
                // end;
               NominalVoltage: real;   {nominalne napiecie silnika}
               WindingRes : real;
               u: real; //wspolczynnik tarcia yB wywalic!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
               CircuitRes : real;      {rezystancje silnika i obwodu}
               IminLo,IminHi: integer; {prady przelacznika automatycznego rozruchu, uzywane tez przez ai_driver}
               ImaxLo,ImaxHi: integer; {maksymalny prad niskiego i wysokiego rozruchu}
               nmax: real;             {maksymalna dop. ilosc obrotow /s}
               InitialCtrlDelay,       {ile sek. opoznienia po wl. silnika}
               CtrlDelay: real;        { -//-  -//- miedzy kolejnymi poz.}
               CtrlDownDelay: real;    { -//-  -//- przy schodzeniu z poz.} {hunter-101012}
               FastSerialCircuit: byte;{0 - po kolei zamyka styczniki az do osiagniecia szeregowej, 1 - natychmiastowe wejscie na szeregowa} {hunter-111012}
               AutoRelayType: byte;    {0 -brak, 1 - jest, 2 - opcja}
               CoupledCtrl: boolean;   {czy mainctrl i scndctrl sa sprzezone}
               //CouplerNr: TCouplerNr;  {ABu: nr sprzegu podlaczonego w drugim obiekcie}
               IsCoupled: boolean;     {czy jest sprzezony ale jedzie z tylu}
               DynamicBrakeType: byte; {patrz dbrake_*}
               RVentType: byte;        {0 - brak, 1 - jest, 2 - automatycznie wlaczany}
               RVentnmax: real;      {maks. obroty wentylatorow oporow rozruchowych}
               RVentCutOff: real;      {rezystancja wylaczania wentylatorow dla RVentType=2}
               CompressorPower: integer; {0: bezp. z obwodow silnika, 1: z przetwornicy, reczne, 2: w przetwornicy, stale, 5: z silnikowego}
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

                {- dla lokomotyw z silnikami indukcyjnymi -}
                eimc: array [0..20] of real;

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
                DoorOpenMethod: byte;             {sposob otwarcia - 1: przesuwne, 2: obrotowe, 3: trójelementowe}
                ScndS: boolean; {Czy jest bocznikowanie na szeregowej}

                        {--sekcja zmiennych}
                        {--opis konkretnego egzemplarza taboru}
                Loc: TLocation; //pozycja pojazdów do wyznaczenia odleg³oœci pomiêdzy sprzêgami
                Rot: TRotation;
                Name: string;                       {nazwa wlasna}
                Couplers: array[0..1] of TCoupling;  //urzadzenia zderzno-sprzegowe, polaczenia miedzy wagonami
                HVCouplers: array[0..1] of array[0..1] of real; //przewod WN
                ScanCounter: integer;   {pomocnicze do skanowania sprzegow}
                EventFlag: boolean;                 {!o true jesli cos nietypowego sie wydarzy}
                SoundFlag: byte;                    {!o patrz stale sound_ }
                DistCounter: real;                  {! licznik kilometrow }
                V: real;    //predkosc w [m/s] wzglêdem sprzêgów (dodania gdy jedzie w stronê 0)
                Vel: real;  //modu³ prêdkoœci w [km/h], u¿ywany przez AI
                AccS: real; //efektywne przyspieszenie styczne w [m/s^2] (wszystkie si³y)
                AccN: real; //przyspieszenie normalne w [m/s^2]
                AccV: real;
                nrot: real;
                {! rotacja kol [obr/s]}
                EnginePower: real;                  {! chwilowa moc silnikow}
                dL, Fb, Ff : real;                  {przesuniecie, sila hamowania i tarcia}
                FTrain,FStand: real;                {! sila pociagowa i oporow ruchu}
                FTotal: real;                       {! calkowita sila dzialajaca na pojazd}
                UnitBrakeForce: real;               {!s si³a hamowania przypadaj¹ca na jeden element}
                Ntotal: real;               {!s si³a nacisku klockow}
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
                EqvtPipePress: real;                {!o cisnienie w przewodzie glownym skladu}
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
                BrakeCtrlPosR:real;                 {nastawa hamulca zespolonego - plynna dla FV4a}
                BrakeCtrlPos2:real;                 {nastawa hamulca zespolonego - kapturek dla FV4a}
                LocalBrakePos:byte;                 {nastawa hamulca indywidualnego}
                ManualBrakePos:byte;                 {nastawa hamulca recznego}
                LocalBrakePosA: real;
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
                OffsetTrackH, OffsetTrackV: real;  {przesuniecie poz. i pion. w/m osi toru}

                {-zmienne dla lokomotyw}
                Mains: boolean;    {polozenie glownego wylacznika}
                MainCtrlPos: byte; {polozenie glownego nastawnika}
                ScndCtrlPos: byte; {polozenie dodatkowego nastawnika}
                ActiveDir: integer; //czy lok. jest wlaczona i w ktorym kierunku:
                //wzglêdem wybranej kabiny: -1 - do tylu, +1 - do przodu, 0 - wylaczona
                CabNo: integer; //numer kabiny, z której jest sterowanie: 1 lub -1; w przeciwnym razie brak sterowania - rozrzad
                DirAbsolute: integer; //zadany kierunek jazdy wzglêdem sprzêgów (1=w strone 0,-1=w stronê 1)
                ActiveCab: integer; //numer kabiny, w ktorej jest obsada (zwykle jedna na sk³ad)
                LastSwitchingTime: real; {czas ostatniego przelaczania czegos}
                //WarningSignal: byte;     {0: nie trabi, 1,2: trabi}
                DepartureSignal: boolean; {sygnal odjazdu}
                InsideConsist: boolean;
                {-zmienne dla lokomotywy elektrycznej}
                RunningTraction:TTractionParam;{parametry sieci trakcyjnej najblizej lokomotywy}
                enrot, Im, Itot,IHeating,ITraction, TotalCurrent, Mm, Mw, Fw, Ft: real;
                {ilosc obrotow, prad silnika i calkowity, momenty, sily napedne}
                //Ra: Im jest ujemny, jeœli lok jedzie w stronê sprzêgu 1
                //a ujemne powinien byæ przy odwróconej polaryzacji sieci...
                //w wielu miejscach jest u¿ywane abs(Im)
                Imin,Imax: integer;      {prad przelaczania automatycznego rozruchu, prad bezpiecznika}
                Voltage: real;           {aktualne napiecie sieci zasilajacej}
                MainCtrlActualPos: byte; {wskaznik RList}
                ScndCtrlActualPos: byte; {wskaznik MotorParam}
                DelayCtrlFlag: boolean;  //czy czekanie na 1. pozycji na za³¹czenie?
                LastRelayTime: real;     {czas ostatniego przelaczania stycznikow}
                AutoRelayFlag: boolean;  {mozna zmieniac jesli AutoRelayType=2}
                FuseFlag: boolean;       {!o bezpiecznik nadmiarowy}
                ConvOvldFlag: boolean;              {!  nadmiarowy przetwornicy i ogrzewania}
                StLinFlag: boolean;       {!o styczniki liniowe}
                ResistorsFlag: boolean;  {!o jazda rezystorowa}
                RventRot: real;          {!s obroty wentylatorow rozruchowych}
                UnBrake: boolean;       {w EZT - nacisniete odhamowywanie}
                PantPress: real; {Cisnienie w zbiornikach pantografow}
                s_CAtestebrake: boolean; //hunter-091012: zmienna dla testu ca


                {-zmienne dla lokomotywy spalinowej z przekladnia mechaniczna}
                dizel_fill: real; {napelnienie}
                dizel_engagestate: real; {sprzeglo skrzyni biegow: 0 - luz, 1 - wlaczone, 0.5 - wlaczone 50% (z poslizgiem)}
                dizel_engage: real; {sprzeglo skrzyni biegow: aktualny docisk}
                dizel_automaticgearstatus: real; {0 - bez zmiany, -1 zmiana na nizszy +1 zmiana na wyzszy}
                dizel_enginestart: boolean;      {czy trwa rozruch silnika}
                dizel_engagedeltaomega: real;    {roznica predkosci katowych tarcz sprzegla}

                {- zmienne dla lokomotyw z silnikami indukcyjnymi -}
                eimv: array [0..20] of real;

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

                DoorBlocked: boolean;    //Czy jest blokada drzwi
                DoorLeftOpened: boolean;  //stan drzwi
                DoorRightOpened: boolean;
                PantFrontUp: boolean;  //stan patykow 'Winger 160204
                PantRearUp: boolean;
                PantFrontSP: boolean;  //dzwiek patykow 'Winger 010304
                PantRearSP: boolean;
                PantFrontStart: integer;  //stan patykow 'Winger 160204
                PantRearStart: integer;
                PantFrontVolt: real;   //pantograf pod napieciem? 'Winger 160404
                PantRearVolt: real;
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


                function GetTrainsetVoltage: real;
                function Physic_ReActivation: boolean;

{                function BrakeRatio: real;  }
                function LocalBrakeRatio: real;
                function ManualBrakeRatio: real;
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
                function BatterySwitch(State:boolean):boolean;
                function EpFuseSwitch(State:boolean):boolean;

               {! stopnie hamowania - hamulec zasadniczy}
                function IncBrakeLevelOld:boolean;
                function DecBrakeLevelOld:boolean;
               {! stopnie hamowania - hamulec pomocniczy}
                function IncLocalBrakeLevel(CtrlSpeed:byte):boolean;
                function DecLocalBrakeLevel(CtrlSpeed:byte):boolean;
               {! ABu 010205: - skrajne polozenia ham. pomocniczego}
                function IncLocalBrakeLevelFAST:boolean;
                function DecLocalBrakeLevelFAST:boolean;

                {! stopnie hamowania - hamulec reczny}
                function IncManualBrakeLevel(CtrlSpeed:byte):boolean;
                function DecManualBrakeLevel(CtrlSpeed:byte):boolean;

               {! hamulec bezpieczenstwa}
                function EmergencyBrakeSwitch(Switch:boolean): boolean;
               {hamulec przeciwposlizgowy}
                function AntiSlippingBrake: boolean;
               {odluzniacz}
                function BrakeReleaser(state: byte):boolean;
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
                procedure UpdateScndPipePressure(dt:real);
                //procedure UpdateBatteryVoltage(dt:real);
                function GetDVc(dt:real):real;

               {! funkcje laczace/rozlaczajace sprzegi}
               //Ra: przeniesione do C++

               {funkcje obliczajace sily}
                procedure ComputeConstans; //ABu: wczesniejsze wyznaczenie stalych dla liczenia sil
                //procedure SetCoupleDist;   //ABu: wyznaczenie odleglosci miedzy wagonami
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
                //function DirectionForward: boolean;
                function DirectionBackward: boolean;
               {! wylacznik glowny}
                function MainSwitch(State:boolean): boolean;
//                procedure SwitchMainKey;
               {! zmiana kabiny i resetowanie ustawien}
                //function ChangeCab(direction:integer): boolean;

               {! wl/wyl przetwornicy}
                function ConverterSwitch(State:boolean):boolean;
                {! wl/wyl sprezarki}
                function CompressorSwitch(State:boolean):boolean;
{                function SmallCompressorSwitch(State:boolean):boolean;}
                {procedure ConverterCheck; ->C++}

                {-funkcje typowe dla lokomotywy elektrycznej}
                {! wlaczanie bezpiecznika nadmiarowego}
                function FuseOn: boolean;
                function FuseFlagCheck: boolean; {sprawdzanei flagi nadmiarowego}
                procedure FuseOff; {wywalanie bezpiecznika nadmiarowego}
                {!o pokazuje bezwgl. wartosc pradu na wybranym amperomierzu}
                function ShowCurrent(AmpN:byte): integer;
                {!o pokazuje bezwgl. wartosc obrotow na obrotomierzu jednego z 3 pojazdow}
                {function ShowEngineRotation(VehN:byte): integer; //Ra 2014-06: przeniesione do C++}
                {funkcje uzalezniajace sile pociagowa od predkosci: v2n, n2r, current, momentum}
                function v2n:real;
                function current(n,U:real): real;
                function Momentum(I:real): real;
                function MomentumF(I, Iw :real; SCP:byte ): real;                
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
                constructor Init(//LocInitial:TLocation; RotInitial:TRotation;
                                 VelInitial:real; TypeNameInit, NameInit: string; LoadInitial:longint; LoadTypeInitial: string; Cab:integer);
                                                              {wywolac najpierw to}
                function LoadChkFile(chkpath:string):Boolean;   {potem zaladowac z pliku}
                function CheckLocomotiveParameters(ReadyFlag:boolean;Dir:longint): boolean;  {a nastepnie sprawdzic}
                function EngineDescription(what:integer): string;  {opis stanu lokomotywy}

   {obsluga drzwi}
                function DoorLeft(State: Boolean): Boolean;    {obsluga dzwi lewych}
                function DoorRight(State: Boolean): Boolean;
                function DoorBlockedFlag: Boolean; //czy blokada drzwi jest aktualnie za³¹czona

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
//zwraca odleg³oœæ pomiêdzy pojazdami (Loc1) i (Loc2) z uwzglêdnieneim ich d³ugoœci (kule!)
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

{---------rozwiniecie deklaracji metod obiektu T_MoverParameters--------}

function T_MoverParameters.GetTrainsetVoltage: real;
//ABu: funkcja zwracajaca napiecie dla calego skladu, przydatna dla EZT
var volt: real;
begin
 volt:=0.0;
  if Couplers[1].Connected<>nil then
   if (TestFlag(Couplers[1].CouplingFlag,ctrain_power)) then //czy jest sprzêg WN
    begin //najczêœciej silnikowy jest z ty³u
     if (Couplers[1].Connected.PantFrontVolt<>0.0) then
      volt:=Couplers[1].Connected.PantFrontVolt
     else
      if (Couplers[1].Connected.PantRearVolt<>0.0) then
       volt:=Couplers[1].Connected.PantRearVolt;
    end;
  if (volt=0.0) then
   if Couplers[0].Connected<>nil then
    if (TestFlag(Couplers[0].CouplingFlag,ctrain_power)) then //czy jest sprzêg WN
     begin
      if (Couplers[0].Connected.PantFrontVolt<>0.0) then
       volt:=Couplers[0].Connected.PantFrontVolt
      else
       if (Couplers[0].Connected.PantRearVolt<>0.0) then
        volt:=Couplers[0].Connected.PantRearVolt;
     end;
  GetTrainsetVoltage:=volt;
end;



function T_MoverParameters.Physic_ReActivation: boolean;
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
function T_MoverParameters.BrakeRatio: real;
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

function T_MoverParameters.LocalBrakeRatio: real;
var LBR:real;
begin
  if LocalBrakePosNo>0 then
   LBR:=LocalBrakePos/LocalBrakePosNo
  else
   LBR:=0;
// if TestFlag(BrakeStatus,b_antislip) then
//   LBR:=Max0R(LBR,PipeRatio)+0.4;
 LocalBrakeRatio:=LBR;
end;

function T_MoverParameters.ManualBrakeRatio: real;
var MBR:real;
begin
  if ManualBrakePosNo>0 then
   MBR:=ManualBrakePos/ManualBrakePosNo
  else
   MBR:=0;
 ManualBrakeRatio:=MBR;
end;

function T_MoverParameters.PipeRatio: real;
var pr:real;
begin
  if DeltaPipePress>0 then
   if (false) then //SPKS!!
    begin
     if (3*PipePress)>(HighPipePress+LowPipePress+LowPipePress) then
      pr:=(HighPipePress-Min0R(HighPipePress,PipePress))/(DeltaPipePress*4.0/3.0)
     else
      pr:=(HighPipePress-1.0/3.0*DeltaPipePress-Max0R(LowPipePress,PipePress))/(DeltaPipePress*2.0/3.0);
//     if (not TestFlag(BrakeStatus,b_Ractive)) and (BrakeMethod and 1 = 0) and TestFlag(BrakeDelays,bdelay_R) and (Power<1) and (BrakeCtrlPos<1) then
//      pr:=Min0R(0.5,pr);
      //if (Compressor>0.5) then
      // pr:=pr*1.333; //dziwny rapid wywalamy
    end
   else
     pr:=(HighPipePress-Max0R(LowPipePress,Min0R(HighPipePress,PipePress)))/DeltaPipePress
  else
    pr:=0;
  PipeRatio:=pr;
end;

function T_MoverParameters.RealPipeRatio: real;
var rpp:real;
begin
  if DeltaPipePress>0 then
    rpp:=(CntrlPipePress-PipePress)/(DeltaPipePress)
  else
    rpp:=0;
  RealPipeRatio:=rpp;
end;

function T_MoverParameters.BrakeVP: real;
begin
  if BrakeVVolume>0 then
   BrakeVP:=Volume/(10.0*BrakeVVolume)
  else BrakeVP:=0;
end;

{reczne przelaczanie hamulca elektrodynamicznego}
function T_MoverParameters.DynamicBrakeSwitch(Switch:boolean):boolean;
var b:byte;
begin
  if (DynamicBrakeType=dbrake_switch) and (MainCtrlPos=0) then
   begin
     DynamicBrakeFlag:=Switch;
     DynamicBrakeSwitch:=true;
     for b:=0 to 1 do
      with Couplers[b] do
       if TestFlag(CouplingFlag,ctrain_controll) then
        Connected.DynamicBrakeFlag:=DynamicBrakeFlag;
   {end;
   if (DynamicBrakeType=dbrake_passive) and (TrainType=dt_ET42) then
   begin
   DynamicBrakeFlag:=false;
   DynamicBrakeSwitch:=false;}
   end
  else
   DynamicBrakeSwitch:=false;
end;


function T_MoverParameters.SendCtrlBroadcast(CtrlCommand:string;ctrlvalue:real):boolean;
var b:byte; OK:boolean;
begin
  OK:=((CtrlCommand<>CommandIn.Command) and (ctrlvalue<>CommandIn.Value1));
  if OK then
   for b:=0 to 1 do
    with Couplers[b] do
     if TestFlag(CouplingFlag,ctrain_controll) then
      if Connected.SetInternalCommand(CtrlCommand,ctrlvalue,DirF(b)) then
       OK:=Connected.RunInternalCommand or OK;
  SendCtrlBroadcast:=OK;
end;


function T_MoverParameters.SendCtrlToNext(CtrlCommand:string;ctrlvalue,dir:real):boolean;
//wys³anie komendy w kierunku dir (1=przod,-1=ty³) do kolejnego pojazdu (jednego)
var
 OK:Boolean;
 d:Integer; //numer sprzêgu w kierunku którego wysy³amy
begin
//Ra: by³ problem z propagacj¹, jeœli w sk³adzie jest pojazd wstawiony odwrotnie
//Ra: problem jest równie¿, jeœli AI bêdzie na koñcu sk³adu
 OK:=(dir<>0); // and Mains;
 d:=(1+Sign(dir)) div 2; //dir=-1=>d=0, dir=1=>d=1 - wysy³anie tylko w ty³
 if OK then //musi byæ wybrana niezerowa kabina
  with Couplers[d] do //w³asny sprzêg od strony (d)
   if TestFlag(CouplingFlag,ctrain_controll) then
    if ConnectedNr<>d then //jeœli ten nastpêny jest zgodny z aktualnym
     begin
      if Connected.SetInternalCommand(CtrlCommand,ctrlvalue,dir) then
       OK:=Connected.RunInternalCommand and OK; //tu jest rekurencja
     end
     else //jeœli nastêpny jest ustawiony przeciwnie, zmieniamy kierunek
      if Connected.SetInternalCommand(CtrlCommand,ctrlvalue,-dir) then
       OK:=Connected.RunInternalCommand and OK; //tu jest rekurencja
 SendCtrlToNext:=OK;
end;

function T_MoverParameters.CabActivisation:boolean;
//za³¹czenie rozrz¹du
var OK:boolean;
begin
  OK:=(CabNo=0); //numer kabiny, z której jest sterowanie
  if (OK) then
   begin
     CabNo:=ActiveCab; //sterowanie jest z kabiny z obsad¹
     DirAbsolute:=ActiveDir*CabNo;
     SendCtrlToNext('CabActivisation',1,CabNo);
   end;
  CabActivisation:=OK;
end;

function T_MoverParameters.CabDeactivisation:boolean;
//wy³¹czenie rozrz¹du
var OK:boolean;
begin
 OK:=(CabNo=ActiveCab); //o ile obsada jest w kabinie ze sterowaniem
 if (OK) then
  begin
   CabNo:=0;
   DirAbsolute:=ActiveDir*CabNo;
   DepartureSignal:=false; //nie buczeæ z nieaktywnej kabiny
   SendCtrlToNext('CabActivisation',0,ActiveCab); //CabNo==0!
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

function T_MoverParameters.IncMainCtrl(CtrlSpeed:integer): boolean;
var //b:byte;
    OK:boolean;
begin
 OK:=false;
 if (MainCtrlPosNo>0) and (CabNo<>0) then
  begin
   if MainCtrlPos<MainCtrlPosNo then
    begin
     if (TrainType<>dt_ET22) or ((TrainType=dt_ET22) and (ScndCtrlPos=0)) then //w ET22 nie da siê krêciæ nastawnikiem przy w³¹czonym boczniku
     case EngineType of
      None, Dumb, DieselElectric, ElectricInductionMotor:
       if ((CtrlSpeed=1) and (TrainType<>dt_EZT)) or ((CtrlSpeed=1) and (TrainType=dt_EZT)and (activedir<>0)) then
        begin //w EZT nie da siê za³¹czyæ pozycji bez ustawienia kierunku
         inc(MainCtrlPos);
         OK:=true;
        end
       else
        if ((CtrlSpeed>1) and (TrainType<>dt_EZT)) or ((CtrlSpeed>1) and (TrainType=dt_EZT)and (activedir<>0)) then
          OK:=IncMainCtrl(1) and IncMainCtrl(CtrlSpeed-1);
      ElectricSeriesMotor:
       if (CtrlSpeed=1) and (ActiveDir<>0) then
        begin
         inc(MainCtrlPos);
         OK:=true;
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
 }
            end;
            if (CtrlSpeed=1) and (ActiveDir=-1) and (RList[MainCtrlPos].Bn>1) and (TrainType<>dt_pseudodiesel) then
             begin //blokada wejœcia na równoleg³¹ podczas jazdy do ty³u
                   dec(MainCtrlPos);
                   OK:=false;
                 end;
             {if (TrainType='et40') then
               if Abs(Im)>IminHi then
                 begin
                 dec(MainCtrlPos); //Blokada nastawnika po przekroczeniu minimalnego pradu
                 OK:=false;
                 end; }
         if(DynamicBrakeFlag)then
           if(TrainType=dt_ET42)then
             if MainCtrlPos>20 then
              begin
               dec(MainCtrlPos);
               OK:=false;
              end;
        end
       else
        if (CtrlSpeed>1) and (ActiveDir<>0) {and (ScndCtrlPos=0)} and (TrainType<>dt_ET40) then
         begin //szybkie przejœcie na bezoporow¹
           while (RList[MainCtrlPos].R>0) and IncMainCtrl(1) do ;
            //OK:=true ; {takie chamskie, potem poprawie} <-Ra: kto mia³ to poprawiæ i po co?
            if  ActiveDir=-1 then
             while (RList[MainCtrlPos].Bn>1) and IncMainCtrl(1) do
             dec(MainCtrlPos);
             OK:=false;
             {if (TrainType=dt_ET40)  then
              while Abs (Im)>IminHi do
                dec(MainCtrlPos);
               OK:=false ;  }
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
         OK:=true;
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
           OK:=true;
         end;
      WheelsDriven:
       OK:=AddPulseForce(CtrlSpeed);
     end //case EngineType of
    end
   else //MainCtrlPos>=MainCtrlPosNo
    if (CoupledCtrl) then //wspólny wa³ nastawnika jazdy i bocznikowania
     begin
      if (ScndCtrlPos<ScndCtrlPosNo) then //3<3 -> false
       begin
        inc(ScndCtrlPos);
        OK:=true;
       end
      else OK:=false;
     end;
   if OK then
    begin
     {OK:=}SendCtrlToNext('MainCtrl',MainCtrlPos,CabNo);  {???}
     {OK:=}SendCtrlToNext('ScndCtrl',ScndCtrlPos,CabNo);
    end;
  end
 else {nie ma sterowania}
  OK:=false;
 //if OK then LastRelayTime:=0;

 //hunter-101012: poprawka
 //poprzedni warunek byl niezbyt dobry, bo przez to przy trzymaniu +
 //styczniki tkwily na tej samej pozycji (LastRelayTime byl caly czas 0 i rosl
 //po puszczeniu plusa)

 if OK then
  begin
   if (DelayCtrlFlag) then
    begin
     if (LastRelayTime>=InitialCtrlDelay)and(MainCtrlPos=1) then
      LastRelayTime:=0
    end
   else if (LastRelayTime>CtrlDelay) then
    LastRelayTime:=0;
  end;
 IncMainCtrl:=OK;
end;


function T_MoverParameters.DecMainCtrl(CtrlSpeed:integer): boolean;
var //b:byte;
    OK:boolean;
begin
  OK:=false;
  if (MainCtrlPosNo>0) and (CabNo<>0) then
   begin
      if MainCtrlPos>0 then
       begin
       if ((TrainType<>dt_ET22) or (ScndCtrlPos=0)) then //Ra: ET22 blokuje nastawnik przy boczniku
       begin
         if CoupledCtrl and (ScndCtrlPos>0) then
          begin
            dec(ScndCtrlPos); {wspolny wal}
            OK:=true;
          end
         else
          case EngineType of
            None, Dumb, DieselElectric, ElectricInductionMotor:
             if ((CtrlSpeed=1) and {(ScndCtrlPos=0) and} (EngineType<>DieselElectric)) or ((CtrlSpeed=1)and(EngineType=DieselElectric))then
              begin
                dec(MainCtrlPos);
                OK:=true;
              end
             else
              if CtrlSpeed>1 then
               OK:=DecMainCtrl(1) and DecMainCtrl(2); //CtrlSpeed-1);
            ElectricSeriesMotor:
             if (CtrlSpeed=1) {and (ScndCtrlPos=0)} then
              begin
                dec(MainCtrlPos);
//                if (MainCtrlPos=0) and (ScndCtrlPos=0) and (TrainType<>dt_ET40)and(TrainType<>dt_EP05) then
//                 StLinFlag:=false;
//                if (MainCtrlPos=0) and (TrainType<>dt_ET40) and (TrainType<>dt_EP05) then
//                 MainCtrlActualPos:=0; //yBARC: co to tutaj robi? ;)
                OK:=true;
              end
             else
              if (CtrlSpeed>1) {and (ScndCtrlPos=0)} then
               begin
                 OK:=true;
                  if (RList[MainCtrlPos].R=0)
                   then DecMainCtrl(1);
                  while (RList[MainCtrlPos].R>0) and DecMainCtrl(1) do ; {takie chamskie, potem poprawie}
               end;
            DieselEngine:
             if CtrlSpeed=1 then
              begin
                dec(MainCtrlPos);
                OK:=true;
              end
              else
              if CtrlSpeed>1 then
               begin
                 while (MainCtrlPos>0) or (RList[MainCtrlPos].Mn>0) do
                  DecMainCtrl(1);
                 OK:=true;
               end;
          end;
          end;
       end
      else
       if EngineType=WheelsDriven then
        OK:=AddPulseForce(-CtrlSpeed)
       else
        OK:=false;
      if OK then
       begin
          {OK:=}SendCtrlToNext('MainCtrl',MainCtrlPos,CabNo);  {hmmmm...???!!!}
          {OK:=}SendCtrlToNext('ScndCtrl',ScndCtrlPos,CabNo);
       end;
   end
  else
   OK:=false;
 //if OK then LastRelayTime:=0;
 //hunter-101012: poprawka
 if OK then
  begin
   if DelayCtrlFlag then
    begin
     if (LastRelayTime>=InitialCtrlDelay) then
      LastRelayTime:=0;
    end
   else if (LastRelayTime>CtrlDownDelay) then
    LastRelayTime:=0;
  end;
 DecMainCtrl:=OK;
end;

function T_MoverParameters.IncScndCtrl(CtrlSpeed:integer): boolean;
var //b:byte;
    OK:boolean;
begin
  if (MainCtrlPos=0)and(CabNo<>0)and(TrainType=dt_ET42)and(ScndCtrlPos=0)and(DynamicBrakeFlag)then
   begin
    OK:=DynamicBrakeSwitch(false);
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
      OK:=true;
    end
   else {nie mozna zmienic}
    OK:=false;
   if OK then
    begin
      {OK:=}SendCtrlToNext('MainCtrl',MainCtrlPos,CabNo);   {???}
      {OK:=}SendCtrlToNext('ScndCtrl',ScndCtrlPos,CabNo);
    end;
  end
 else {nie ma sterowania}
  OK:=false;
 //if OK then LastRelayTime:=0;
 //hunter-101012: poprawka
 if OK then
  if (LastRelayTime>CtrlDelay) then
   LastRelayTime:=0;

 IncScndCtrl:=OK;
end;

function T_MoverParameters.DecScndCtrl(CtrlSpeed:integer): boolean;
var //b:byte;
    OK:boolean;
begin
  if (MainCtrlPos=0)and(CabNo<>0)and(TrainType=dt_ET42)and(ScndCtrlPos=0)and not(DynamicBrakeFlag)and(CtrlSpeed=1)then
   begin
    //Ra: AI wywo³uje z CtrlSpeed=2 albo gdy ScndCtrlPos>0
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
      OK:=true;
     end
    else
     OK:=false;
    if OK then
     begin
      {OK:=}SendCtrlToNext('MainCtrl',MainCtrlPos,CabNo);  {???}
      {OK:=}SendCtrlToNext('ScndCtrl',ScndCtrlPos,CabNo);
     end;
   end
  else
   OK:=false;
 //if OK then LastRelayTime:=0;
 //hunter-101012: poprawka
 if OK then
  if (LastRelayTime>CtrlDownDelay) then
   LastRelayTime:=0;
 DecScndCtrl:=OK;
end;


function T_MoverParameters.DirectionBackward: boolean;
begin
  if (ActiveDir=1) and (MainCtrlPos=0) and (TrainType=dt_EZT) then
    if MinCurrentSwitch(false) then
    begin
      DirectionBackward:=true; //
      Exit;
    end;
  if (MainCtrlPosNo>0) and (ActiveDir>-1) and (MainCtrlPos=0) then
   begin
    if EngineType=WheelsDriven then
     dec(CabNo);
{    else}
    dec(ActiveDir);
    DirAbsolute:=ActiveDir*CabNo;
    if (DirAbsolute<>0) then
     if (Battery) then //jeœli bateria jest ju¿ za³¹czona
      BatterySwitch(true); //to w ten oto durny sposób aktywuje siê CA/SHP
    DirectionBackward:=true;
    SendCtrltoNext('Direction',ActiveDir,CabNo);
   end
  else
   DirectionBackward:=false;
end;

function T_MoverParameters.MainSwitch(State:boolean): boolean;
begin
 MainSwitch:=false; //Ra: przeniesione z koñca
  if ((Mains<>State) and (MainCtrlPosNo>0)) then
   begin
    if (State=false) or ({(MainCtrlPos=0) and} (ScndCtrlPos=0) and (ConvOvldFlag=false) and (LastSwitchingTime>CtrlDelay) and not TestFlag(DamageFlag,dtrain_out)) then
     begin
       if Mains then //jeœli by³ za³¹czony
        SendCtrlToNext('MainSwitch',ord(State),CabNo); //wys³anie wy³¹czenia do pozosta³ych?
       Mains:=State;
       if Mains then //jeœli zosta³ za³¹czony
        SendCtrlToNext('MainSwitch',ord(State),CabNo); //wyslanie po wlaczeniu albo przed wylaczeniem
       MainSwitch:=true; //wartoœæ zwrotna
       LastSwitchingTime:=0;
       if (EngineType=DieselEngine) and Mains then
        begin
          dizel_enginestart:=State;
        end;
       //if (State=false) then //jeœli wy³¹czony
       // begin
         //SetFlag(SoundFlag,sound_relay); //hunter-091012: przeniesione do Train.cpp, zeby sie nie zapetlal
         // if (SecuritySystem.Status<>12) then
       //  SecuritySystem.Status:=0; //deaktywacja czuwaka; Ra: a nie bateri¹?
       // end
       //else
       //if (SecuritySystem.Status<>12) then
       // SecuritySystem.Status:=s_waiting; //aktywacja czuwaka
     end
   end
  //else MainSwitch:=false;
end;

function T_MoverParameters.BatterySwitch(State:boolean):boolean;
begin
 //Ra: ukrotnienie za³¹czania baterii jest jak¹œ fikcj¹...
 if (Battery<>State) then
  begin
   Battery:=State;
  end;
 if (Battery=true) then SendCtrlToNext('BatterySwitch',1,CabNo)
  else SendCtrlToNext('BatterySwitch',0,CabNo);
 BatterySwitch:=true;
 if (Battery) and (ActiveCab<>0) {or (TrainType=dt_EZT)} then
  SecuritySystem.Status:=SecuritySystem.Status or s_waiting //aktywacja czuwaka
 else
  SecuritySystem.Status:=0; //wy³¹czenie czuwaka
end;

function T_MoverParameters.EpFuseSwitch(State:boolean):boolean;
begin
 if (EpFuse<>State) then
  begin
   EpFuse:=State;
   EpFuseSwitch:=true;
  end
 else
  EpFuseSwitch:=false;  
// if (EpFuse=true) then SendCtrlToNext('EpFuseSwitch',1,CabNo)
//  else SendCtrlToNext('EpFuseSwitch',0,CabNo);
end;

{wl/wyl przetwornicy}
function T_MoverParameters.ConverterSwitch(State:boolean):boolean;
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
function T_MoverParameters.CompressorSwitch(State:boolean):boolean;
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
{function T_MoverParameters.SmallCompressorSwitch(State:boolean):boolean;
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
function T_MoverParameters.SandDoseOn : boolean;
begin
  if (SandCapacity>0) then
   begin
     SandDoseOn:=true;
     if SandDose then
      SandDose:=false
     else
      if Sand>0 then SandDose:=true;
     if CabNo<>0 then
       SendCtrlToNext('SandDoseOn',1,CabNo);
   end
  else
   SandDoseOn:=false;
end;

(*
//Ra: to jest do przejrzenia i uporz¹dkowania
function T_MoverParameters.SecuritySystemReset : boolean;
//zbijanie czuwaka/SHP
 procedure Reset;
  begin
  with SecuritySystem do
    if not(TestFlag(SystemType,2) and TestFlag(Status,s_active)) then
    SecuritySystem.SystemTimer:=0;
    SecuritySystem.SystemBrakeTimer:=0;
    SecuritySystem.SystemSoundTimer:=0;
    SecuritySystem.Status:=s_waiting; //aktywacja czuwaka
    SecuritySystem.VelocityAllowed:=-1;
  end;
  procedure ResetSHP;
  begin
    SecuritySystem.SystemBrakeTimer:=0;
    SecuritySystem.SystemSoundTimer:=0;
    SecuritySystem.VelocityAllowed:=-1;
    SecuritySystem.Status:=s_waiting;
  end;
begin
  with SecuritySystem do
    begin
    if (Status=3) or (Status=2) or (Status=11) or (Status=26)or(Status=27) then
      begin
        SecuritySystemReset:=true;
        if (Status<s_ebrake) then
         Reset
        else
          if EmergencyBrakeSwitch(false) then
           Reset;
      end;

      if (Status=5) or (Status=13) or (Status=29) or (Status=4) or ((Status=12) and (activedir<>0)) or (Status=28) then  {Wlaczone tylko SHP}
    {  begin
        SecuritySystemReset:=true;
        if (Status<s_ebrake) then
         ResetSHP
        else
          if EmergencyBrakeSwitch(false) then
           ResetSHP;
      end;
      if (Status=7) or (Status=15) or (Status=30) or (Status=31) then  {Wlaczone SHP i CA}
     { begin
        SecuritySystemReset:=true;
        if (Status<s_ebrake) then
         begin
         ResetSHP;
         SecuritySystem.Status:=3;
         end
        else
          if EmergencyBrakeSwitch(false) then
           ResetSHP;
      end
    else
     SecuritySystemReset:=false;
     end;
//  SendCtrlToNext('SecurityReset',0,CabNo);
end;

function T_MoverParameters.SecuritySystemReset : boolean;
//zbijanie czuwaka/SHP
 procedure Reset;
  begin
   SecuritySystem.SystemTimer:=0;
   if TestFlag(SecuritySystem.Status,s_aware) then
    begin
     SecuritySystem.SystemBrakeCATimer:=0;
     SecuritySystem.SystemSoundCATimer:=0;
     SetFlag(SecuritySystem.Status,-s_aware);
     SetFlag(SecuritySystem.Status,-s_CAalarm);
     SetFlag(SecuritySystem.Status,-s_CAebrake);
     EmergencyBrakeFlag:=false;
     SecuritySystem.VelocityAllowed:=-1;
    end
   else if TestFlag(SecuritySystem.Status,s_active) then
    begin
     SecuritySystem.SystemBrakeSHPTimer:=0;
     SecuritySystem.SystemSoundSHPTimer:=0;
     SetFlag(SecuritySystem.Status,-s_active);
     SetFlag(SecuritySystem.Status,-s_SHPalarm);
     SetFlag(SecuritySystem.Status,-s_SHPebrake);
     EmergencyBrakeFlag:=false;
     SecuritySystem.VelocityAllowed:=-1;
    end;
  end;
begin
  with SecuritySystem do
    if (SystemType>0) and (Status>0) and ((status<>12) or ((status=12) and (activedir<>0))) then
    //if (SystemType>0) and (Status>0) then
      begin
        SecuritySystemReset:=true;
        if not (ActiveDir=0) then
         if not TestFlag(Status,s_CAebrake) or not TestFlag(Status,s_SHPebrake) then
          Reset;
        //else
        //  if EmergencyBrakeSwitch(false) then
        //   Reset;
      end
    else
     SecuritySystemReset:=false;
//  SendCtrlToNext('SecurityReset',0,CabNo);
end;

{testowanie czuwaka/SHP}
procedure T_MoverParameters.SecuritySystemCheck(dt:real);
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
           EmergencyBrakeSwitch(true);
      end;
   end;
end;
*)

//hunter-091012: rozbicie alarmow, dodanie testu czuwaka
function T_MoverParameters.SecuritySystemReset : boolean;
//zbijanie czuwaka/SHP
 procedure Reset;
  begin
   SecuritySystem.SystemTimer:=0;

   if TestFlag(SecuritySystem.Status,s_aware) then
    begin
     SecuritySystem.SystemBrakeCATimer:=0;
     SecuritySystem.SystemSoundCATimer:=0;
     SetFlag(SecuritySystem.Status,-s_aware);
     SetFlag(SecuritySystem.Status,-s_CAalarm);
     SetFlag(SecuritySystem.Status,-s_CAebrake);
//     EmergencyBrakeFlag:=false; //YB-HN
     SecuritySystem.VelocityAllowed:=-1;
    end
   else if TestFlag(SecuritySystem.Status,s_active) then
    begin
     SecuritySystem.SystemBrakeSHPTimer:=0;
     SecuritySystem.SystemSoundSHPTimer:=0;
     SetFlag(SecuritySystem.Status,-s_active);
     SetFlag(SecuritySystem.Status,-s_SHPalarm);
     SetFlag(SecuritySystem.Status,-s_SHPebrake);
//     EmergencyBrakeFlag:=false; //YB-HN
     SecuritySystem.VelocityAllowed:=-1;
    end;
  end;
begin
  with SecuritySystem do
    if (SystemType>0) and (Status>0) then
      begin
        SecuritySystemReset:=true;
        if ((TrainType=dt_EZT)or(ActiveDir<>0)) then //Ra 2014-03: w EZT nie trzeba ustawiaæ kierunku
         if not TestFlag(Status,s_CAebrake) or not TestFlag(Status,s_SHPebrake) then
          Reset;
        //else
        //  if EmergencyBrakeSwitch(false) then
        //   Reset;
      end
    else
     SecuritySystemReset:=false;
//  SendCtrlToNext('SecurityReset',0,CabNo);
end;

procedure T_MoverParameters.SecuritySystemCheck(dt:real);
begin
//Ra: z CA/SHP w EZT jest ten problem, ¿e w rozrz¹dczym nie ma kierunku, a w silnikowym nie ma obsady
//poza tym jest zdefiniowany we wszystkich 3 cz³onach EN57
  with SecuritySystem do
   begin
     if (SystemType>0) and (Status>0) and (Battery) then //Ra: EZT ma teraz czuwak w rozrz¹dczym
      begin
       //CA
       if (Vel>=AwareMinSpeed) then  //domyœlnie predkoœæ wiêksza od 10% Vmax, albo podanej jawnie w FIZ
       begin
        SystemTimer:=SystemTimer+dt;
        if TestFlag(SystemType,1) and TestFlag(Status,s_aware) then //jeœli œwieci albo miga
         SystemSoundCATimer:=SystemSoundCATimer+dt;
        if TestFlag(SystemType,1) and TestFlag(Status,s_CAalarm) then //jeœli buczy
         SystemBrakeCATimer:=SystemBrakeCATimer+dt;
        if TestFlag(SystemType,1) then
         if (SystemTimer>AwareDelay) and (AwareDelay>=0) then  {-1 blokuje}
           if not SetFlag(Status,s_aware) then {juz wlaczony sygnal swietlny}
             if (SystemSoundCATimer>SoundSignalDelay) and (SoundSignalDelay>=0) then
               if not SetFlag(Status,s_CAalarm) then {juz wlaczony sygnal dzwiekowy}
                 if (SystemBrakeCATimer>EmergencyBrakeDelay) and (EmergencyBrakeDelay>=0) then
                   SetFlag(Status,s_CAebrake);


       //SHP
        if TestFlag(SystemType,2) and TestFlag(Status,s_active) then //jeœli œwieci albo miga
         SystemSoundSHPTimer:=SystemSoundSHPTimer+dt;
        if TestFlag(SystemType,2) and TestFlag(Status,s_SHPalarm) then //jeœli buczy
         SystemBrakeSHPTimer:=SystemBrakeSHPTimer+dt;
        if TestFlag(SystemType,2) and TestFlag(Status,s_active) then
         if (Vel>VelocityAllowed) and (VelocityAllowed>=0) then
          SetFlag(Status,s_SHPebrake)
         else
          if ((SystemSoundSHPTimer>SoundSignalDelay) and (SoundSignalDelay>=0)) or ((Vel>NextVelocityAllowed) and (NextVelocityAllowed>=0)) then
            if not SetFlag(Status,s_SHPalarm) then {juz wlaczony sygnal dzwiekowy}
              if (SystemBrakeSHPTimer>EmergencyBrakeDelay) and (EmergencyBrakeDelay>=0) then
               SetFlag(Status,s_SHPebrake);

       end; //else SystemTimer:=0;

       //TEST CA
        if TestFlag(Status,s_CAtest) then //jeœli œwieci albo miga
         SystemBrakeCATestTimer:=SystemBrakeCATestTimer+dt;
        if TestFlag(SystemType,1) then
           if TestFlag(Status,s_CAtest) then {juz wlaczony sygnal swietlny}
               if (SystemBrakeCATestTimer>EmergencyBrakeDelay) and (EmergencyBrakeDelay>=0) then
                   s_CAtestebrake:=true;

       //wdrazanie hamowania naglego
//        if TestFlag(Status,s_SHPebrake) or TestFlag(Status,s_CAebrake) or (s_CAtestebrake=true) then
//         EmergencyBrakeFlag:=true;  //YB-HN
      end
     else if not (Battery) then
      begin //wy³¹czenie baterii deaktywuje sprzêt
       //SecuritySystem.Status:=0; //deaktywacja czuwaka
      end;
   end;
end;

{nastawy hamulca}

function T_MoverParameters.IncBrakeLevelOld:boolean;
//var b:byte;
begin
  if (BrakeCtrlPosNo>0) {and (LocalBrakePos=0)} then
   begin
     if BrakeCtrlPos<BrakeCtrlPosNo then
      begin
        inc(BrakeCtrlPos);
//        BrakeCtrlPosR:=BrakeCtrlPos;

//youBy: wywalilem to, jak jest EP, to sa przenoszone sygnaly nt. co ma robic, a nie poszczegolne pozycje;
//       wystarczy spojrzec na Knorra i Oerlikona EP w EN57; mogly ze soba wspolapracowac
{
        if (BrakeSystem=ElectroPneumatic) then
          if (BrakePressureActual.BrakeType=ElectroPneumatic) then
           begin
//             BrakeStatus:=ord(BrakeCtrlPos>0);
             SendCtrlToNext('BrakeCtrl',BrakeCtrlPos,CabNo);
           end
          else SendCtrlToNext('BrakeCtrl',-2,CabNo);
//        else
//         if not TestFlag(BrakeStatus,b_dmg) then
//          BrakeStatus:=b_on;}

//youBy: EP po nowemu

        IncBrakeLevelOld:=true;
        if (BrakePressureActual.PipePressureVal<0)and(BrakePressureTable[BrakeCtrlPos-1].PipePressureVal>0) then
          LimPipePress:=PipePress;

        if (BrakeSystem=ElectroPneumatic) then
          if (BrakeSubSystem<>ss_K) then
           begin
             if (BrakeCtrlPos*BrakeCtrlPos)=1 then
              begin
//                SendCtrlToNext('Brake',BrakeCtrlPos,CabNo);
//                SetFlag(BrakeStatus,b_epused);
              end
             else
              begin
//                SendCtrlToNext('Brake',0,CabNo);
//                SetFlag(BrakeStatus,-b_epused);
              end;
           end;

      end
     else
      begin
        IncBrakeLevelOld:=false;
{        if BrakeSystem=Pneumatic then
         EmergencyBrakeSwitch(true); }
      end;
   end
  else
   IncBrakeLevelOld:=false;
end;

function T_MoverParameters.DecBrakeLevelOld:boolean;
//var b:byte;
begin
  if (BrakeCtrlPosNo>0) {and (LocalBrakePos=0)} then
   begin
     if (BrakeCtrlPos>-1-Byte(BrakeHandle=FV4a)) then
      begin
        dec(BrakeCtrlPos);
//        BrakeCtrlPosR:=BrakeCtrlPos;
        if EmergencyBrakeFlag then
          begin
             EmergencyBrakeFlag:=false; {!!!}
             SendCtrlToNext('Emergency_brake',0,CabNo);
          end;

//youBy: wywalilem to, jak jest EP, to sa przenoszone sygnaly nt. co ma robic, a nie poszczegolne pozycje;
//       wystarczy spojrzec na Knorra i Oerlikona EP w EN57; mogly ze soba wspolapracowac
{
        if (BrakeSystem=ElectroPneumatic) then
          if BrakePressureActual.BrakeType=ElectroPneumatic then
           begin
//             BrakeStatus:=ord(BrakeCtrlPos>0);
             SendCtrlToNext('BrakeCtrl',BrakeCtrlPos,CabNo);
           end
          else SendCtrlToNext('BrakeCtrl',-2,CabNo);
//        else}
//         if (not TestFlag(BrakeStatus,b_dmg) and (not TestFlag(BrakeStatus,b_release))) then
//          BrakeStatus:=b_off;   {luzowanie jesli dziala oraz nie byl wlaczony odluzniacz}

//youBy: EP po nowemu
        DecBrakeLevelOld:=true;
//        if (BrakePressureTable[BrakeCtrlPos].PipePressureVal<0.0)and(BrakePressureTable[BrakeCtrlPos+1].PipePressureVal>0) then
//          LimPipePress:=PipePress;

        if (BrakeSystem=ElectroPneumatic) then
          if (BrakeSubSystem<>ss_K) then
           begin
             if (BrakeCtrlPos*BrakeCtrlPos)=1 then
              begin
//                SendCtrlToNext('Brake',BrakeCtrlPos,CabNo);
//                SetFlag(BrakeStatus,b_epused);
              end
             else
              begin
//                SendCtrlToNext('Brake',0,CabNo);
//                SetFlag(BrakeStatus,-b_epused);
              end;
           end;
(*    for b:=0 to 1 do  {poprawic to!}
     with Couplers[b] do
      if CouplingFlag and ctrain_controll=ctrain_controll then
       Connected^.BrakeCtrlPos:=BrakeCtrlPos;
*)
      end
     else
      DecBrakeLevelOld:=false;
   end
     else
      DecBrakeLevelOld:=false;
end;

function T_MoverParameters.IncLocalBrakeLevelFAST:boolean;
begin
  if (LocalBrakePos<LocalBrakePosNo) then
   begin
     LocalBrakePos:=LocalBrakePosNo;
     IncLocalBrakeLevelFAST:=true;
   end
  else
     IncLocalBrakeLevelFAST:=false;
UnBrake:=true;
end;

function T_MoverParameters.IncLocalBrakeLevel(CtrlSpeed:byte):boolean;
begin
  if (LocalBrakePos<LocalBrakePosNo) {and (BrakeCtrlPos<1)} then
   begin
     while (LocalBrakePos<LocalBrakePosNo) and (CtrlSpeed>0) do
      begin
       inc(LocalBrakePos);
       dec(CtrlSpeed);
      end;
     IncLocalBrakeLevel:=true;
   end
  else
     IncLocalBrakeLevel:=false;
UnBrake:=true;
end;

function T_MoverParameters.IncManualBrakeLevel(CtrlSpeed:byte):boolean;
begin
  if (ManualBrakePos<ManualBrakePosNo) {and (BrakeCtrlPos<1)} then
   begin
     while (ManualBrakePos<ManualBrakePosNo) and (CtrlSpeed>0) do
      begin
       inc(ManualBrakePos);
       dec(CtrlSpeed);
      end;
     IncManualBrakeLevel:=true;
   end
  else
     IncManualBrakeLevel:=false;
end;

function T_MoverParameters.DecLocalBrakeLevelFAST():boolean;
begin
  if LocalBrakePos>0 then
   begin
     LocalBrakePos:=0;
     DecLocalBrakeLevelFAST:=true;
   end
  else
     DecLocalBrakeLevelFAST:=false;
UnBrake:=true;
end;

function T_MoverParameters.DecLocalBrakeLevel(CtrlSpeed:byte):boolean;
begin
  if LocalBrakePos>0 then
   begin
     while (CtrlSpeed>0) and (LocalBrakePos>0) do
      begin
        dec(LocalBrakePos);
        dec(CtrlSpeed);
      end;
     DecLocalBrakeLevel:=true;
   end
  else
     DecLocalBrakeLevel:=false;
UnBrake:=true;
end;

function T_MoverParameters.DecManualBrakeLevel(CtrlSpeed:byte):boolean;
begin
  if ManualBrakePos>0 then
   begin
     while (CtrlSpeed>0) and (ManualBrakePos>0) do
      begin
        dec(ManualBrakePos);
        dec(CtrlSpeed);
      end;
     DecManualBrakeLevel:=true;
   end
  else
     DecManualBrakeLevel:=false;

end;

function T_MoverParameters.EmergencyBrakeSwitch(Switch:boolean): boolean;
begin
  if (BrakeSystem<>Individual) and (BrakeCtrlPosNo>0) then
   begin
     if (not EmergencyBrakeFlag) and Switch then
      begin
        EmergencyBrakeFlag:=Switch;
        EmergencyBrakeSwitch:=true;
      end
     else
      begin
        if (Abs(V)<0.1) and (Switch=false) then  {odblokowanie hamulca bezpieczenistwa tylko po zatrzymaniu}
         begin
           EmergencyBrakeFlag:=Switch;
           EmergencyBrakeSwitch:=true;
         end
        else
         EmergencyBrakeSwitch:=false;
      end;
   end
  else
   EmergencyBrakeSwitch:=false;   {nie ma hamulca bezpieczenstwa gdy nie ma hamulca zesp.}
end;

function T_MoverParameters.AntiSlippingBrake: boolean;
begin
 AntiSlippingBrake:=false; //Ra: przeniesione z koñca
  if ASBType=1 then
   begin
     AntiSlippingBrake:=true;  //SPKS!!
     Hamulec.ASB(1);
     BrakeSlippingTimer:=0;
   end
end;

function T_MoverParameters.AntiSlippingButton: boolean;
var OK:boolean;
begin
  OK:=SandDoseOn;
  AntiSlippingButton:=(AntiSlippingBrake or OK);
end;

function T_MoverParameters.BrakeDelaySwitch(BDS:byte): boolean;
begin
//  if BrakeCtrlPosNo>0 then
   if Hamulec.SetBDF(BDS) then
    begin
      BrakeDelayFlag:=BDS;
      BrakeDelaySwitch:=true;
      BrakeStatus:=(BrakeStatus and 191);
      //kopowanie nastawy hamulca do kolejnego czlonu - do przemyœlenia
      if CabNo<>0 then
       SendCtrlToNext('BrakeDelay',BrakeDelayFlag,CabNo);
    end
  else
   BrakeDelaySwitch:=false;
end;

function T_MoverParameters.IncBrakeMult(): boolean;
begin
  if (LoadFlag>0) and (MBPM<2) and (LoadFlag<3) then
   begin
    if (MaxBrakePress[2]>0) and (LoadFlag=1) then
      LoadFlag:=2
    else
      LoadFlag:=3;
    IncBrakeMult:=true;
    if BrakeCylMult[2]>0 then BrakeCylMult[0]:=BrakeCylMult[2];
   end
  else
 IncBrakeMult:=false;
end;

function T_MoverParameters.DecBrakeMult(): boolean;
begin
  if (LoadFlag>1) and (MBPM<2) then
   begin
    if (MaxBrakePress[2]>0) and (LoadFlag=3) then
      LoadFlag:=2
    else
      LoadFlag:=1;
    DecBrakeMult:=true;
    if BrakeCylMult[1]>0 then BrakeCylMult[0]:=BrakeCylMult[1];
   end
  else
 DecBrakeMult:=false;
end;

function T_MoverParameters.BrakeReleaser(state: byte): boolean;
var OK:boolean;
begin
  Hamulec.Releaser(state);
  if CabNo<>0 then //rekurencyjne wys³anie do nastêpnego
   OK:=SendCtrlToNext('BrakeReleaser',state,CabNo);
  BrakeReleaser:=OK;
end;

function T_MoverParameters.SwitchEPBrake(state: byte):boolean;
var
  OK:boolean;
  temp: real;
begin
  OK:=false;
  if (BrakeHandle = St113) and (ActiveCab<>0) then
   begin
    if(state>0)then
      temp:=(Handle as TSt113).GetCP
    else
      temp:=0;
    Hamulec.SetEPS(temp);
    SendCtrlToNext('Brake',temp,CabNo);
   end;
//  OK:=SetFlag(BrakeStatus,((2*State-1)*b_epused));
//  SendCtrlToNext('Brake',(state*(2*BrakeCtrlPos-1)),CabNo);
  SwitchEPBrake:=OK;
end;

{uklady pneumatyczne}

function T_MoverParameters.IncBrakePress(var brake:real;PressLimit,dp:real):boolean;
begin
//  if (DynamicBrakeType<>dbrake_switch) and (DynamicBrakeType<>dbrake_none) and ((BrakePress>2.0) or (PipePress<3.7{(LowPipePress+0.5)})) then
    if (DynamicBrakeType<>dbrake_switch) and (DynamicBrakeType<>dbrake_none) and (BrakePress>2.0) and (TrainType<>dt_EZT) then //yB radzi nie sprawdzaæ ciœnienia w przewodzie
    //hunter-301211: dla EN57 silnikow nie odlaczamy
     begin
       DynamicBrakeFlag:=true;                 {uruchamianie hamulca ED albo odlaczanie silnikow}
       if (DynamicBrakeType=dbrake_automatic) and (abs(Im)>60) then            {nie napelniaj wiecej, jak na EP09}
         dp:=0.0;
     end;
    if brake+dp<PressLimit then
     begin
       brake:=brake+dp;
       IncBrakePress:=true;
     end
    else
     begin
       IncBrakePress:=false;
       brake:=PressLimit;
     end;
end;

function T_MoverParameters.DecBrakePress(var brake:real;PressLimit,dp:real):boolean;
begin
    if brake-dp>PressLimit then
     begin
       brake:=brake-dp;
       DecBrakePress:=true;
     end
    else
     begin
       DecBrakePress:=false;
       brake:=PressLimit;
     end;
//  if (DynamicBrakeType<>dbrake_switch) and ((BrakePress<0.1) and (PipePress>0.45{(LowPipePress+0.06)})) then
    if (DynamicBrakeType<>dbrake_switch) and (BrakePress<0.1) then //yB radzi nie sprawdzaæ ciœnienia w przewodzie
     DynamicBrakeFlag:=false;                {wylaczanie hamulca ED i/albo zalaczanie silnikow}
end;

procedure T_MoverParameters.UpdateBrakePressure(dt:real);
const LBDelay=5.0; {stala czasowa hamulca}
var Rate,Speed,dp,sm:real;
begin
  dpLocalValve:=0;
  dpBrake:=0;

  BrakePress:=Hamulec.GetBCP;
//  BrakePress:=(Hamulec as TEst4).ImplsRes.pa;
  Volume:=Hamulec.GetBRP;

end;  {updatebrakepressure}


function T_MoverParameters.GetDVc(dt:real):real;
var c:T_MoverParameters;dv1,dv2,dv:real;
begin
  dv1:=0;
  dv2:=0;
//sprzeg 1
  if Couplers[0].Connected<>nil then
    if TestFlag(Couplers[0].CouplingFlag,ctrain_pneumatic) then
     begin                                        //*0.85
      c:=Couplers[0].Connected; //skrot           //0.08           //e/D * L/D = e/D^2 * L
      dv1:=0.5*dt*PF(PipePress,c.PipePress,(Spg)/(1+0.015/Spg*Dim.L));
      if (dv1*dv1>0.00000000000001) then c.Physic_Reactivation;
      c.Pipe.Flow(-dv1);
     end;
//sprzeg 2
  if Couplers[1].Connected<>nil then
    if TestFlag(Couplers[1].CouplingFlag,ctrain_pneumatic) then
     begin
      c:=Couplers[1].Connected; //skrot
      dv2:=0.5*dt*PF(PipePress,c.PipePress,(Spg)/(1+0.015/Spg*Dim.L));
      if (dv2*dv2>0.00000000000001) then c.Physic_Reactivation;
      c.Pipe.Flow(-dv2);
     end;
  if (Couplers[1].Connected<>nil)and(Couplers[0].Connected<>nil) then
    if (TestFlag(Couplers[0].CouplingFlag,ctrain_pneumatic))and(TestFlag(Couplers[1].CouplingFlag,ctrain_pneumatic))then
     begin
      dv:=0.05*dt*PF(Couplers[0].Connected.PipePress,Couplers[1].Connected.PipePress,(Spg*0.85)/(1+0.03*Dim.L))*0;
      Couplers[0].Connected.Pipe.Flow(+dv);
      Couplers[1].Connected.Pipe.Flow(-dv);
     end;
//suma
  GetDVc:=dv2+dv1;
end;

procedure T_MoverParameters.UpdatePipePressure(dt:real);
const LBDelay=100;kL=0.5;
var {b: byte;} dV{,PWSpeed}{,PPP}:real; c: T_MoverParameters;
     temp: real;
     b: byte;
begin

  PipePress:=Pipe.P;
//  PPP:=PipePress;

  dpMainValve:=0;

if (BrakeCtrlPosNo>1) and (ActiveCab<>0)then
with BrakePressureTable[BrakeCtrlPos] do
   begin
          dpLocalValve:=LocHandle.GetPF(LocalBrakePos/LocalBrakePosNo, Hamulec.GetBCP, ScndPipePress, dt, 0);
          if(BrakeHandle=FV4a)and((PipePress<2.75)and((Hamulec.GetStatus and b_rls)=0))and(BrakeSubsystem=ss_LSt)and(TrainType<>dt_EZT)then
            temp:=PipePress+0.00001
            else
            temp:=ScndPipePress;
          Handle.SetReductor(BrakeCtrlPos2);

          dpMainValve:=Handle.GetPF(BrakeCtrlPosR, PipePress, temp, dt, EqvtPipePress);
          if (dpMainValve<0){and(PipePressureVal>0.01)} then             {50}
            if Compressor>ScndPipePress then
               begin
              CompressedVolume:=CompressedVolume+dpMainValve/1500;
              Pipe2.Flow(dpMainValve/3);
               end
              else
              Pipe2.Flow(dpMainValve);
end;

//      if(EmergencyBrakeFlag)and(BrakeCtrlPosNo=0)then         {ulepszony hamulec bezp.}
      if(EmergencyBrakeFlag)or TestFlag(SecuritySystem.Status,s_SHPebrake) or TestFlag(SecuritySystem.Status,s_CAebrake) or (s_CAtestebrake=true)then         {ulepszony hamulec bezp.}
        dpMainValve:=dpMainValve/1+PF(0,PipePress,0.15)*dt;
                                                //0.2*Spg
      Pipe.Flow(-dpMainValve);
      Pipe.Flow(-(PipePress)*0.001*dt);
//      if Heating then
//        Pipe.Flow(PF(PipePress,0,d2A(7))*dt);
//      if ConverterFlag then
//        Pipe.Flow(PF(PipePress,0,d2A(12))*dt);
      dpMainValve:=dpMainValve/(Dim.L*Spg*20);

      CntrlPipePress:=Hamulec.GetVRP; //ciœnienie komory wstêpnej rozdzielacza

      case BrakeValve of
      W:
      begin
          if (BrakeLocHandle<>NoHandle) then
           begin
            LocBrakePress:=LocHandle.GetCP;
           (Hamulec as TWest).SetLBP(LocBrakePress);
           end;
          if MBPM<2 then
           (Hamulec as TWest).PLC(MaxBrakePress[LoadFlag])
          else
           (Hamulec as TWest).PLC(TotalMass);
         end;
      LSt,EStED:
         begin
        LocBrakePress:=LocHandle.GetCP;
        for b:=0 to 1 do
         if((TrainType and (dt_ET41 or dt_ET42))>0)and(Couplers[b].Connected<>nil)then //nie podoba mi siê to rozwi¹zanie, chyba trzeba dodaæ jakiœ wpis do fizyki na to
          if((Couplers[b].Connected.TrainType and (dt_ET41 or dt_ET42))>0)and((Couplers[b].CouplingFlag and 36) = 36)then
           LocBrakePress:=Max0R(Couplers[b].Connected.LocHandle.GetCP,LocBrakePress);

         if(DynamicBrakeFlag)and(EngineType=ElectricInductionMotor)then
          begin
           if(Vel>10)then LocBrakePress:=0 else
           if(Vel>5)then LocBrakePress:=(10-Vel)/5*LocBrakePress
          end;           

        (Hamulec as TLSt).SetLBP(LocBrakePress);
       end;
      CV1_L_TR:
      begin
        LocBrakePress:=LocHandle.GetCP;
        (Hamulec as TCV1L_TR).SetLBP(LocBrakePress);
     end;
       EP2:      (Hamulec as TEStEP2).PLC(TotalMass);
       ESt3AL2,NESt3,ESt4,ESt3:
       begin
          if MBPM<2 then
            (Hamulec as TNESt3).PLC(MaxBrakePress[LoadFlag])
      else
            (Hamulec as TNESt3).PLC(TotalMass);
          LocBrakePress:=LocHandle.GetCP;
          (Hamulec as TNESt3).SetLBP(LocBrakePress);
        end;
     KE:
      begin
         LocBrakePress:=LocHandle.GetCP;
        (Hamulec as TKE).SetLBP(LocBrakePress);
         if MBPM<2 then
          (Hamulec as TKE).PLC(MaxBrakePress[LoadFlag])
        else
          (Hamulec as TKE).PLC(TotalMass);
         end;
     end;

      if (BrakeHandle = FVel6) and (ActiveCab<>0) then
      begin
        if (Battery) and (ActiveDir<>0) and (EpFuse) then //tu powinien byc jeszcze bezpiecznik EP i baterie -
          temp:=(Handle as TFVel6).GetCP
        else
          temp:=0;  
        Hamulec.SetEPS(temp);
        SendCtrlToNext('Brake',temp,CabNo); //Ra 2014-11: na tym siê wysypuje, ale nie wiem, w jakich warunkach
       end;

      Pipe.Act;
      PipePress:=Pipe.P;
      if (BrakeStatus and 128)=128 then //jesli hamulec wy³¹czony
        temp:=0  //odetnij
       else
        temp:=1; //po³¹cz
      Pipe.Flow(temp*Hamulec.GetPF(temp*PipePress,dt,Vel)+GetDVc(dt));

      if ASBType=128 then
          Hamulec.ASB(Byte(SlippingWheels));

      dpPipe:=0;

//yB: jednokrokowe liczenie tego wszystkiego
      Pipe.Act;
      PipePress:=Pipe.P;

      dpMainValve:=dpMainValve/(100*dt); //normalizacja po czasie do syczenia;


  if PipePress<-1 then
     begin
    PipePress:=-1;
    Pipe.CreatePress(-1);
    Pipe.Act;
     end;

  if CompressedVolume<0 then
   CompressedVolume:=0;
end;  {updatepipepressure}


procedure T_MoverParameters.CompressorCheck(dt:real);
begin
 //if (CompressorSpeed>0.0) then //ten warunek zosta³ sprawdzony przy wywo³aniu funkcji
 if (VeselVolume>0) then
  begin
   if MaxCompressor-MinCompressor<0.0001 then
    begin
{     if Mains and (MainCtrlPos>1) then}
     if CompressorAllow and Mains and (MainCtrlPos>0) then
      begin
        if (Compressor<MaxCompressor) then
         if (EngineType=DieselElectric) and (CompressorPower>0) then
          CompressedVolume:=CompressedVolume+dt*CompressorSpeed*(2*MaxCompressor-Compressor)/MaxCompressor*(DEList[MainCtrlPos].rpm/DEList[MainCtrlPosNo].rpm)
         else
          begin
           CompressedVolume:=CompressedVolume+dt*CompressorSpeed*(2*MaxCompressor-Compressor)/MaxCompressor;
           TotalCurrent:=0.0015*Voltage; //tymczasowo tylko obci¹¿enie sprê¿arki, tak z 5A na sprê¿arkê
          end
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
     if (CompressorFlag) then //jeœli sprê¿arka za³¹czona
      begin //sprawdziæ mo¿liwe warunki wy³¹czenia sprê¿arki
       if (CompressorPower=5) then //jeœli zasilanie z s¹siedniego cz³onu
        begin //zasilanie sprê¿arki w cz³onie ra z cz³onu silnikowego (sprzêg 1)
         if (Couplers[1].Connected<>NIL) then
          CompressorFlag:=Couplers[1].Connected.CompressorAllow and Couplers[1].Connected.ConverterFlag and Couplers[1].Connected.Mains
         else
          CompressorFlag:=false; //bez tamtego cz³onu nie zadzia³a
        end
       else if (CompressorPower=4) then //jeœli zasilanie z poprzedniego cz³onu
        begin //zasilanie sprê¿arki w cz³onie ra z cz³onu silnikowego (sprzêg 1)
         if (Couplers[0].Connected<>NIL) then
          CompressorFlag:=Couplers[0].Connected.CompressorAllow and Couplers[0].Connected.ConverterFlag and Couplers[0].Connected.Mains
         else
          CompressorFlag:=false; //bez tamtego cz³onu nie zadzia³a
        end
       else
        CompressorFlag:=(CompressorAllow) and ((ConverterFlag) or (CompressorPower=0)) and (Mains);
      if (Compressor>MaxCompressor) then //wy³¹cznik ciœnieniowy jest niezale¿ny od sposobu zasilania
       CompressorFlag:=false;
      end
     else //jeœli nie za³¹czona
      if (Compressor<MinCompressor) and (LastSwitchingTime>CtrlDelay) then //jeœli nie za³¹czona, a ciœnienie za ma³e
       begin //za³¹czenie przy ma³ym ciœnieniu
        if (CompressorPower=5) then //jeœli zasilanie z nastêpnego cz³onu
         begin //zasilanie sprê¿arki w cz³onie ra z cz³onu silnikowego (sprzêg 1)
          if (Couplers[1].Connected<>NIL) then
           CompressorFlag:=Couplers[1].Connected.CompressorAllow and Couplers[1].Connected.ConverterFlag and Couplers[1].Connected.Mains
          else
           CompressorFlag:=false; //bez tamtego cz³onu nie zadzia³a
         end
        else if (CompressorPower=4) then //jeœli zasilanie z poprzedniego cz³onu
         begin //zasilanie sprê¿arki w cz³onie ra z cz³onu silnikowego (sprzêg 1)
          if (Couplers[0].Connected<>NIL) then
           CompressorFlag:=Couplers[0].Connected.CompressorAllow and Couplers[0].Connected.ConverterFlag and Couplers[0].Connected.Mains
          else
           CompressorFlag:=false; //bez tamtego cz³onu nie zadzia³a
         end
        else
         CompressorFlag:=(CompressorAllow) and ((ConverterFlag) or (CompressorPower=0)) and (Mains);
        if (CompressorFlag) then //jeœli zosta³a za³¹czona
         LastSwitchingTime:=0; //to trzeba ograniczyæ ponowne w³¹czenie
       end;
//          for b:=0 to 1 do //z Megapacka
//    with Couplers[b] do
//     if TestFlag(CouplingFlag,ctrain_scndpneumatic) then
//      Connected.CompressorFlag:=CompressorFlag;
     if CompressorFlag then
      if (EngineType=DieselElectric) and (CompressorPower>0) then
       CompressedVolume:=CompressedVolume+dt*CompressorSpeed*(2*MaxCompressor-Compressor)/MaxCompressor*(DEList[MainCtrlPos].rpm/DEList[MainCtrlPosNo].rpm)
      else
       begin
        CompressedVolume:=CompressedVolume+dt*CompressorSpeed*(2*MaxCompressor-Compressor)/MaxCompressor;
        if (CompressorPower=5)and(Couplers[1].Connected<>NIL) then
         Couplers[1].Connected.TotalCurrent:=0.0015*Couplers[1].Connected.Voltage //tymczasowo tylko obci¹¿enie sprê¿arki, tak z 5A na sprê¿arkê
        else if (CompressorPower=4)and(Couplers[0].Connected<>NIL) then
         Couplers[0].Connected.TotalCurrent:=0.0015*Couplers[0].Connected.Voltage //tymczasowo tylko obci¹¿enie sprê¿arki, tak z 5A na sprê¿arkê
        else
         TotalCurrent:=0.0015*Voltage; //tymczasowo tylko obci¹¿enie sprê¿arki, tak z 5A na sprê¿arkê
       end
    end;
  end;
end;
{Ra 2014-07: do C++
procedure T_MoverParameters.ConverterCheck;
begin //sprawdzanie przetwornicy
if (ConverterAllow=true)and(Mains=true) then
 ConverterFlag:=true
 else
 ConverterFlag:=false;
end;
}
//youBy - przewod zasilajacy
procedure T_MoverParameters.UpdateScndPipePressure(dt: real);
const Spz=0.5067;
var c:T_MoverParameters;dv1,dv2,dv:real;
begin
  dv1:=0;
  dv2:=0;

//sprzeg 1
  if Couplers[0].Connected<>nil then
   if TestFlag(Couplers[0].CouplingFlag,ctrain_scndpneumatic) then
    begin
      c:=Couplers[0].Connected; //skrot
       dv1:=0.5*dt*PF(ScndPipePress,c.ScndPipePress,Spz*0.75);
       if (dv1*dv1>0.00000000000001) then c.Physic_Reactivation;
       c.Pipe2.Flow(-dv1);
    end;
//sprzeg 2
  if Couplers[1].Connected<>nil then
   if TestFlag(Couplers[1].CouplingFlag,ctrain_scndpneumatic) then
     begin
       c:=Couplers[1].Connected; //skrot
       dv2:=0.5*dt*PF(ScndPipePress,c.ScndPipePress,Spz*0.75);
       if (dv2*dv2>0.00000000000001) then c.Physic_Reactivation;
       c.Pipe2.Flow(-dv2);
      end;
  if(Couplers[1].Connected<>nil)and(Couplers[0].Connected<>nil)then
    if (TestFlag(Couplers[0].CouplingFlag,ctrain_scndpneumatic))and(TestFlag(Couplers[1].CouplingFlag,ctrain_scndpneumatic))then
     begin
      dv:=0.00025*dt*PF(Couplers[0].Connected.ScndPipePress,Couplers[1].Connected.ScndPipePress,Spz*0.25);
      Couplers[0].Connected.Pipe2.Flow(+dv);
      Couplers[1].Connected.Pipe2.Flow(-dv);
      end;

  Pipe2.Flow(Hamulec.GetHPFlow(ScndPipePress, dt));

  if ((Compressor>ScndPipePress) and (CompressorSpeed>0.0001)) or (TrainType=dt_EZT) then
   begin
    dV:=PF(Compressor,ScndPipePress,Spz)*dt;
    CompressedVolume:=CompressedVolume+dV/1000;
    Pipe2.Flow(-dV);
   end;

  Pipe2.Flow(dv1+dv2);
  Pipe2.Act;
  ScndPipePress:=Pipe2.P;

  if ScndPipePress<-1 then
   begin
    ScndPipePress:=-1;
    Pipe2.CreatePress(-1);
    Pipe2.Act;
   end;


end;


{lokomotywy}

function T_MoverParameters.ComputeRotatingWheel(WForce,dt,n:real): real;
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

function T_MoverParameters.FuseFlagCheck: boolean;
var b:byte;
begin
 FuseFlagCheck:=false;
 if Power>0.01 then FuseFlagCheck:=FuseFlag
 else               {pobor pradu jezeli niema mocy}
  for b:=0 to 1 do
   with Couplers[b] do
    if TestFlag(CouplingFlag,ctrain_controll) then
     if Connected.Power>0.01 then
      FuseFlagCheck:=Connected.FuseFlagCheck();
end;

function T_MoverParameters.FuseOn: boolean;
begin
 FuseOn:=false;
 if (MainCtrlPos=0) and (ScndCtrlPos=0) and (TrainType<>dt_ET40) and Mains then
  begin //w ET40 jest blokada nastawnika, ale czy dzia³a dobrze?
   SendCtrlToNext('FuseSwitch',1,CabNo);
   if ((EngineType=ElectricSeriesMotor)or((EngineType=DieselElectric))) and FuseFlag then
    begin
     FuseFlag:=false;  {wlaczenie ponowne obwodu}
     FuseOn:=true;
     SetFlag(SoundFlag,sound_relay); SetFlag(SoundFlag,sound_loud);
    end;
  end;
end;

procedure T_MoverParameters.FuseOff;
begin
 if not FuseFlag then
  begin
   FuseFlag:=true;
   EventFlag:=true;
   SetFlag(SoundFlag,sound_relay);
   SetFlag(SoundFlag,sound_loud);
  end;
end;


function T_MoverParameters.ShowCurrent(AmpN:byte): integer;
var b,Bn:byte;
    Grupowy: boolean;
begin
  ClearPendingExceptions;
  ShowCurrent:=0;
  Grupowy:=(DelayCtrlFlag) and (TrainType=dt_et22); //przerzucanie walu grupowego w ET22;
  Bn:=RList[MainCtrlActualPos].Bn; //ile równoleg³ych ga³êzi silników

  if (DynamicBrakeType=dbrake_automatic) and (DynamicBrakeFlag) then
   Bn:=2;
  if Power>0.01 then
   begin
     if AmpN>0 then //podaæ pr¹d w ga³êzi
      begin
       if (Bn<AmpN)or((Grupowy) and (AmpN=Bn-1)) then
         ShowCurrent:=0
        else //normalne podawanie pradu
         ShowCurrent:=Trunc(Abs(Im));
      end
     else //podaæ ca³kowity
       ShowCurrent:=Trunc(Abs(Itot));
    end
  else               {pobor pradu jezeli niema mocy}
   for b:=0 to 1 do
    with Couplers[b] do
     if TestFlag(CouplingFlag,ctrain_controll) then
      if Connected.Power>0.01 then
       ShowCurrent:=Connected.ShowCurrent(AmpN);
end;

{Ra 2014-06: przeniesione do C++
function T_MoverParameters.ShowEngineRotation(VehN:byte): integer;
var b:Byte; //,Bn:byte;
begin
  ClearPendingExceptions;
  ShowEngineRotation:=0;
  case VehN of
   1: ShowEngineRotation:=Trunc(Abs(enrot));
   2: for b:=0 to 1 do
       with Couplers[b] do
        if TestFlag(CouplingFlag,ctrain_controll) then
         if Connected.Power>0.01 then
          ShowEngineRotation:=Trunc(Abs(Connected.enrot));
   3: if Couplers[1].Connected<>nil then
       if TestFlag(Couplers[1].CouplingFlag,ctrain_controll) then
        if Couplers[1].Connected.Couplers[1].Connected<>nil then
         if TestFlag(Couplers[1].Connected.Couplers[1].CouplingFlag,ctrain_controll) then
          if Couplers[1].Connected.Couplers[1].Connected.Power>0.01 then
           ShowEngineRotation:=Trunc(Abs(Couplers[1].Connected.Couplers[1].Connected.enrot));
  end;
end;
}

{funkcje uzalezniajace sile pociagowa od predkosci: V2n, n2R, Current, Momentum}
{----------------}

function T_MoverParameters.V2n:real;
{przelicza predkosc liniowa na obrotowa}
const dmgn=0.5;
var n,deltan:real;
begin
  n:=V/(Pi*WheelDiameter); //predkosc obrotowa wynikajaca z liniowej [obr/s]
  deltan:=n-nrot; //"pochodna" prêdkoœci obrotowej
  if SlippingWheels then
   if Abs(deltan)<0.01 then
    SlippingWheels:=false; {wygaszenie poslizgu}
  if SlippingWheels then   {nie ma zwiazku z predkoscia liniowa V}
   begin                   {McZapkie-221103: uszkodzenia kol podczas poslizgu}
     if deltan>dmgn then
      if FuzzyLogic(deltan,dmgn,p_slippdmg) then
        if SetFlag(DamageFlag,dtrain_wheelwear)   {podkucie}
          then EventFlag:=true;
     if deltan<-dmgn then
      if FuzzyLogic(-deltan,dmgn,p_slippdmg) then
        if SetFlag(DamageFlag,dtrain_thinwheel)  {wycieranie sie obreczy}
          then EventFlag:=true;
     n:=nrot;               {predkosc obrotowa nie zalezy od predkosci liniowej}
   end;
  V2n:=n;
end;

function T_MoverParameters.Current(n,U:real): real;
{wazna funkcja - liczy prad plynacy przez silniki polaczone szeregowo lub rownolegle}
{w zaleznosci od polozenia nastawnikow MainCtrl i ScndCtrl oraz predkosci obrotowej n}
{a takze wywala bezpiecznik nadmiarowy gdy za duzy prad lub za male napiecie}
{jest takze mozliwosc uszkodzenia silnika wskutek nietypowych parametrow}
const ep09resED=5.8; {TODO: dobrac tak aby sie zgadzalo ze wbudzeniem}
var //Rw,
    R,MotorCurrent:real;
    Rz,Delta,Isf:real;
    Mn: integer;
    Bn: real;
    SP: byte;
    U1: real; //napiecie z korekta
begin
  MotorCurrent:=0;
//i dzialanie hamulca ED w EP09
  if (DynamicBrakeType=dbrake_automatic) then
   begin
    if (((Hamulec as TLSt).GetEDBCP<0.25)and(Vadd<1))or(BrakePress>2.1) then
      DynamicBrakeFlag:=false
    else if (BrakePress>0.25) and ((Hamulec as TLSt).GetEDBCP>0.25) then
      DynamicBrakeFlag:=true;
    DynamicBrakeFlag:=DynamicBrakeFlag and ConverterFlag;   
   end;
//wylacznik cisnieniowy yBARC - to jest chyba niepotrzebne tutaj
//  if BrakePress>2 then
//   begin
//    StLinFlag:=true;
//    DelayCtrlFlag:=(TrainType<>dt_EZT); //EN57 nie ma czekania na 1. pozycji
//    DynamicBrakeFlag:=false;
//   end;
  if(BrakeSubSystem=ss_LSt)then
    if(DynamicBrakeFlag)then
     (Hamulec as TLSt).SetED(Abs(Im/350)) //hamulec ED na EP09 dziala az do zatrzymania lokomotywy
    else
     (Hamulec as TLSt).SetED(0);

  ResistorsFlag:=(RList[MainCtrlActualPos].R>0.01){ and (not DelayCtrlFlag)};
  ResistorsFlag:=ResistorsFlag or ((DynamicBrakeFlag=true) and (DynamicBrakeType=dbrake_automatic));

  if(TrainType=dt_ET22)and(DelayCtrlFlag)and(MainCtrlActualPos>1)then
    Bn:=1-1/RList[MainCtrlActualPos].Bn
  else
    Bn:=1;
  R:=RList[MainCtrlActualPos].R*Bn+CircuitRes;

  if (TrainType<>dt_EZT)or(Imin<>IminLo)or(not ScndS) then //yBARC - boczniki na szeregu poprawnie
   Mn:=RList[MainCtrlActualPos].Mn
  else
   Mn:=RList[MainCtrlActualPos].Mn*RList[MainCtrlActualPos].Bn;
//z Megapacka
//  if DynamicBrakeFlag and (TrainType=dt_ET42) then { KURS90 azeby mozna bylo hamowac przy opuszczonych pantografach }
//  SP:=ScndCtrlActualPos;
//   if(ScndInMain)then
//    if not (RList[MainCtrlActualPos].ScndAct=255) then
//     SP:=RList[MainCtrlActualPos].ScndAct;
//    with MotorParam[SP] do
//  begin
//  Rz:=WindingRes+R;
//  MotorCurrent:=-fi*n/Rz;
//  end;
  if DynamicBrakeFlag and (not FuseFlag) and (DynamicBrakeType=dbrake_automatic) and ConverterFlag and Mains then     {hamowanie EP09}
   with MotorParam[0] do //TUHEX
     begin
       MotorCurrent:=-Max0R(fi*(Vadd/(Vadd+Isat)-fi0),0)*n*2/ep09resED; {TODO: zrobic bardziej uniwersalne nie tylko dla EP09}
//       if abs(MotorCurrent)>500 then
//        MotorCurrent:=sign(MotorCurrent)*500 {TODO: zrobic bardziej finezyjnie}
     end
  else
  if (RList[MainCtrlActualPos].Bn=0) {or FuseFlag} or (not StLinFlag) {or DelayCtrlFlag} then
  //if (RList[MainCtrlActualPos].Bn=0) or FuseFlag or StLinFlag or DelayCtrlFlag or ((TrainType=dt_ET42)and(not(ConverterFlag)and not(DynamicBrakeFlag)))  then //z Megapacka
    MotorCurrent:=0                    {wylaczone}
  else                                 {wlaczone}
   begin
   SP:=ScndCtrlActualPos;
   if(ScndCtrlActualPos<255)then  {tak smiesznie bede wylaczal }
    begin
     if(ScndInMain)then
      if not (RList[MainCtrlActualPos].ScndAct=255) then
       SP:=RList[MainCtrlActualPos].ScndAct;
      with MotorParam[SP] do
       begin
        Rz:=Mn*WindingRes+R;
        if DynamicBrakeFlag then     {hamowanie}
         begin
           if DynamicBrakeType>1 then
            begin
              //if DynamicBrakeType<>dbrake_automatic then
              // MotorCurrent:=-fi*n/Rz  {hamowanie silnikiem na oporach rozruchowych}
(*               begin
                 U:=0;
                 Isf:=Isat;
                 Delta:=SQR(Isf*Rz+Mn*fi*n-U)+4*U*Isf*Rz;
                 MotorCurrent:=(U-Isf*Rz-Mn*fi*n+SQRT(Delta))/(2*Rz)
               end*)
            if (DynamicBrakeType=dbrake_switch) and (TrainType=dt_ET42) then
             begin //z Megapacka
             Rz:=WindingRes+R;
             MotorCurrent:=-fi*n/Rz;  //{hamowanie silnikiem na oporach rozruchowych}
               end;
            end
           else
             MotorCurrent:=0;        {odciecie pradu od silnika}
         end
        else
         begin
          U1:=U+Mn*n*fi0*fi;
          Isf:=Sign(U1)*Isat;
          Delta:=SQR(Isf*Rz+Mn*fi*n-U1)+4*U1*Isf*Rz;
          if Mains then
           begin
            if U>0 then
             MotorCurrent:=(U1-Isf*Rz-Mn*fi*n+SQRT(Delta))/(2.0*Rz)
            else
             MotorCurrent:=(U1-Isf*Rz-Mn*fi*n-SQRT(Delta))/(2.0*Rz)
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

  if (DynamicBrakeType=dbrake_switch) and ((BrakePress>2.0) or (PipePress<3.6)) then
   begin
    Im:=0;
    MotorCurrent:=0;
    //Im:=0;
    Itot:=0;
   end
  else
   Im:=MotorCurrent;
  Current:=Im; {prad brany do liczenia sily trakcyjnej}
{  EnginePower:=Im*Im*RList[MainCtrlActualPos].Bn*RList[MainCtrlActualPos].Mn*WindingRes;}
  EnginePower:=Abs(Itot)*(1+RList[MainCtrlActualPos].Mn)*Abs(U);

  {awarie}
  MotorCurrent:=Abs(Im); {zmienna pomocnicza}
  if Motorcurrent>0 then
  begin
    if FuzzyLogic(Abs(n),nmax*1.1,p_elengproblem) then
     if MainSwitch(false) then
       EventFlag:=true;                                      {zbyt duze obroty - wywalanie wskutek ognia okreznego}
    if TestFlag(DamageFlag,dtrain_engine) then
     if FuzzyLogic(MotorCurrent,ImaxLo/10.0,p_elengproblem) then
      if MainSwitch(false) then
       EventFlag:=true;                                      {uszkodzony silnik (uplywy)}
    if (FuzzyLogic(Abs(Im),Imax*2,p_elengproblem) or FuzzyLogic(Abs(n),nmax*1.11,p_elengproblem)) then
{       or FuzzyLogic(Abs(U/Mn),2*NominalVoltage,1)) then } {poprawic potem}
     if (SetFlag(DamageFlag,dtrain_engine)) then
       EventFlag:=true;
  {! dorobic grzanie oporow rozruchowych i silnika}
  end;
end;

function T_MoverParameters.Momentum(I:real): real;
{liczy moment sily wytwarzany przez silnik elektryczny}
var SP: byte;
begin
   SP:=ScndCtrlActualPos;
   if (ScndInMain) then
    if not (RList[MainCtrlActualPos].ScndAct=255) then
     SP:=RList[MainCtrlActualPos].ScndAct;
    with MotorParam[SP] do
//     Momentum:=mfi*I*(1-1.0/(Abs(I)/mIsat+1));
     Momentum:=mfi*I*(Abs(I)/(Abs(I)+mIsat)-mfi0);
end;

function T_MoverParameters.MomentumF(I, Iw :real; SCP: byte): real;
begin
//umozliwia dokladne sterowanie wzbudzeniem
  with MotorParam[SCP] do
   MomentumF:=mfi*I*Max0R(Abs(Iw)/(Abs(Iw)+mIsat)-mfi0,0);
end;

function T_MoverParameters.dizel_Momentum(dizel_fill,n,dt:real): real;
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
(*
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
   Mains:=false;
end;

function T_MoverParameters.CutOffEngine: boolean; {wylacza uszkodzony silnik}
begin
 CutOffEngine:=false; //Ra: wartoœæ domyœlna, sprawdziæ to trzeba
 if (NPoweredAxles>0) and (CabNo=0) and (EngineType=ElectricSeriesMotor) then
  begin
   if SetFlag(DamageFlag,-dtrain_engine) then
    begin
     NPoweredAxles:=NPoweredAxles div 2;
     CutOffEngine:=true;
    end;
  end
end;

{przelacznik pradu wysokiego rozruchu}
function T_MoverParameters.MaxCurrentSwitch(State:boolean):boolean;
begin
  MaxCurrentSwitch:=false;
  if (EngineType=ElectricSeriesMotor) then
   if (ImaxHi>ImaxLo) then
   begin
     if State and (Imax=ImaxLo) and (RList[MainCtrlPos].Bn<2) and not ((TrainType=dt_ET42)and(MainctrlPos>0)) then
      begin
        Imax:=ImaxHi;
        MaxCurrentSwitch:=true;
        if CabNo<>0 then
         SendCtrlToNext('MaxCurrentSwitch',1,CabNo);
      end;
     if (not State) then
     if (Imax=ImaxHi) then
     if not ((TrainType=dt_ET42)and(MainctrlPos>0)) then
      begin
        Imax:=ImaxLo;
        MaxCurrentSwitch:=true;
        if CabNo<>0 then
         SendCtrlToNext('MaxCurrentSwitch',0,CabNo);
      end;
   end;
end;


{przelacznik pradu automatycznego rozruchu}
function T_MoverParameters.MinCurrentSwitch(State:boolean):boolean;
begin
  MinCurrentSwitch:=false;
  if ((EngineType=ElectricSeriesMotor) and (IminHi>IminLo)) or (TrainType=dt_EZT) then
   begin
     if State and (Imin=IminLo) then
      begin
        Imin:=IminHi;
        MinCurrentSwitch:=true;
        if CabNo<>0 then
         SendCtrlToNext('MinCurrentSwitch',1,CabNo);
      end;
     if (not State) and (Imin=IminHi) then
      begin
        Imin:=IminLo;
        MinCurrentSwitch:=true;
        if CabNo<>0 then
         SendCtrlToNext('MinCurrentSwitch',0,CabNo);
      end;
   end;
end;


{reczne przelaczanie samoczynnego rozruchu}
function T_MoverParameters.AutoRelaySwitch(State:boolean):boolean;
 begin
   if (AutoRelayType=2) and (AutoRelayFlag<>State) then
    begin
      AutoRelayFlag:=State;
      AutoRelaySwitch:=true;
      SendCtrlToNext('AutoRelaySwitch',ord(State),CabNo);
    end
   else
    AutoRelaySwitch:=false;
 end;

function T_MoverParameters.AutoRelayCheck: boolean;
var OK:boolean; b:byte;
begin
//  if ((TrainType=dt_EZT{) or (TrainType=dt_ET22)}) and (Imin=IminLo)) or ((ActiveDir<0) and (TrainType<>dt_PseudoDiesel')) then
//     if RList[MainCtrlActualPos].Bn>1 then
//      begin
//        dec(MainCtrlActualPos);
//        AutoRelayCheck:=false;
//        Exit;
//      end;
//yB: wychodzenie przy odcietym pradzie
  if (ScndCtrlActualPos=255)then
   begin
    AutoRelayCheck:=false;
    MainCtrlActualPos:=0;
   end
  else
  //if (not Mains) or (FuseFlag) or (StLinFlag) then   //hunter-111211: wylacznik cisnieniowy
  //if ((not Mains) or (FuseFlag)) and not((DynamicBrakeFlag) and (TrainType=dt_ET42)) then
  if ((not Mains) or (FuseFlag)  or (StLinFlag)) and not((DynamicBrakeFlag) and (TrainType=dt_ET42)) then  //hunter-111211: wylacznik cisnieniowy
   begin
     AutoRelayCheck:=false;
     MainCtrlActualPos:=0;
     ScndCtrlActualPos:=0;
   end
  else
   begin
    OK:=false;
    if DelayCtrlFlag and (MainCtrlPos=1) and (MainCtrlActualPos=1) and (LastRelayTime>InitialCtrlDelay) then
     begin
       DelayCtrlFlag:=false;
       SetFlag(SoundFlag,sound_relay); SetFlag(SoundFlag,sound_loud);
     end;
    //if (TrainType<>dt_EZT) then //Ra: w EZT mo¿na daæ od razu na S albo R, wa³ ku³akowy sobie dokrêci
    if (LastRelayTime>CtrlDelay) and not DelayCtrlFlag then
     begin
      if (MainCtrlPos=0) and (TrainType<>dt_ET40) and (TrainType<>dt_EP05) then
        DelayCtrlFlag:=true;  //(TrainType<>dt_EZT); //EN57 nie ma czekania na 1. pozycji
      if (MainCtrlPos=0) and ((TrainType=dt_ET40)or(TrainType=dt_EP05)) and (MainCtrlActualPos=0) then
      DelayCtrlFlag:=true; //(TrainType<>dt_EZT); //EN57 nie ma czekania na 1. pozycji

      if (((RList[MainCtrlActualPos].R=0) and ((not CoupledCtrl) or ((Imin=IminLo) and (ScndS=true)))) or (MainCtrlActualPos=RListSize))
          and ((ScndCtrlActualPos>0) or (ScndCtrlPos>0)) then
        begin   {zmieniaj scndctrlactualpos}
          if (not AutoRelayFlag) or (not MotorParam[ScndCtrlActualPos].AutoSwitch) then
           begin                                                {scnd bez samoczynnego rozruchu}
             OK:=true;
             if (ScndCtrlActualPos<ScndCtrlPos) then
              inc(ScndCtrlActualPos)
             else
              if (ScndCtrlActualPos>ScndCtrlPos) and (TrainType<>dt_EZT) then
               dec(ScndCtrlActualPos)

              else
              if (ScndCtrlActualPos>ScndCtrlPos) and (TrainType=dt_EZT) then

               Exit       {utkniecie walu kulakowego}
             else OK:=false;
           end
          else
           begin                                                {scnd z samoczynnym rozruchem}
             if (ScndCtrlPos<ScndCtrlActualPos) and (TrainType<>dt_EZT) then
              begin
                dec(ScndCtrlActualPos);
                OK:=true;
              end
              else
              if (ScndCtrlPos<ScndCtrlActualPos) and (TrainType=dt_EZT) then
              begin
                Exit;    {utkniecie walu kulakowego}
              end
             else
              if (ScndCtrlPos>ScndCtrlActualPos) then
               if Abs(Im)<Imin then
                if MotorParam[ScndCtrlActualPos].AutoSwitch then
                 begin
                   inc(ScndCtrlActualPos);
                   OK:=true
                 end;
           end;
        end
       else
        begin          {zmieniaj mainctrlactualpos}
          if ((TrainType=dt_EZT) and (Scnds=true) and (Imin=IminLo)) then
           if RList[MainCtrlActualPos+1].Bn>1 then
            begin
              AutoRelayCheck:=false;
              Exit;
           end;
          if (not AutoRelayFlag) or (not RList[MainCtrlActualPos].AutoSwitch) then
           begin                                                {main bez samoczynnego rozruchu}
             OK:=true;
             if RList[MainCtrlActualPos].Relay<MainCtrlPos then
                if ((RList[MainCtrlActualPos].Mn>RList[MainCtrlPos].Mn)and (RList[MainCtrlActualPos+1].Mn<RList[MainCtrlActualPos].Mn) and (TrainType=dt_ET22) and (LastRelayTime<InitialCtrlDelay))then
                 begin
                 Itot:=0;
                 Im:=0;
                 OK:=false;             {odciecie pradu, przy przelaczaniu silnikow w ET22}
                 end
                 else
                 //if (RList[MainCtrlActualPos].R=0) and (RList[MainCtrlPos].R>0) and (TrainType=dt_ET22) and (LastRelayTime<InitialCtrlDelay) then

              begin
                inc(MainCtrlActualPos);
                //---------
                //hunter-111211: poprawki
                if MainCtrlActualPos>0 then
                 //if (RList[MainCtrlActualPos].R=0) and ((RList[MainCtrlActualPos].ScndAct=0) or (RList[MainCtrlActualPos].ScndAct=255)) then  {dzwieki przechodzenia na bezoporowa}
                 if (RList[MainCtrlActualPos].R=0) and (not (MainCtrlActualPos=MainCtrlPosNo)) then  //wejscie na bezoporowa
                  begin
                   SetFlag(SoundFlag,sound_manyrelay); SetFlag(SoundFlag,sound_loud);
                  end
                 else if (RList[MainCtrlActualPos].R>0) and (RList[MainCtrlActualPos-1].R=0) then //wejscie na drugi uklad
                  begin
                   SetFlag(SoundFlag,sound_manyrelay);
                  end;

              end
             else if (RList[MainCtrlActualPos].Relay>MainCtrlPos) and (TrainType<>dt_EZT) then
              begin
              if not ((RList[MainCtrlActualPos].Mn<RList[MainCtrlPos].Mn) and (RList[MainCtrlActualPos-1].Mn>RList[MainCtrlActualPos].Mn)and (TrainType=dt_ET22) and (LastRelayTime<InitialCtrlDelay))then
              begin
                dec(MainCtrlActualPos);
                if MainCtrlActualPos>0 then  //hunter-111211: poprawki
                 //if (RList[MainCtrlActualPos+1].R=0) and ((RList[MainCtrlActualPos+1].ScndAct=0) or (RList[MainCtrlActualPos+1].ScndAct=255)) then  {dzwieki schodzenia z bezoporowej}
                 if RList[MainCtrlActualPos].R=0 then  {dzwieki schodzenia z bezoporowej}
                  begin
                   SetFlag(SoundFlag,sound_manyrelay);
                  end;
              end
             else
               begin
                 Itot:=0;
                 Im:=0;
                 OK:=false;           {odciecie pradu, przy przelaczaniu silnikow w ET22}
                 end;
              end
              else if (RList[MainCtrlActualPos].Relay>MainCtrlPos) and (TrainType=dt_EZT) and (MainCtrlPos>0) then     //K90
              begin
              Exit;      {utkniecie walu kulakowego}
              end
             else if  (TrainType=dt_EZT) and (MainCtrlPos=0) then
              MainCtrlActualPos:=0
              else
              if (RList[MainCtrlActualPos].R>0) and (ScndCtrlActualPos>0) and (TrainType<>dt_EZT) then
               Dec(ScndCtrlActualPos) {boczniki nie dzialaja na poz. oporowych}
              else
              if (ScndCtrlPos<ScndCtrlActualPos) and (TrainType=dt_EZT) then
               Exit {boczniki nie dzialaja na poz. oporowych}
              else
               OK:=false;
           end
          else                                                  {main z samoczynnym rozruchem}
           begin
             OK:=false;
             if (MainCtrlPos<RList[MainCtrlActualPos].Relay) and (TrainType<>dt_EZT) then
             begin
             if not ((RList[MainCtrlActualPos].Mn<RList[MainCtrlPos].Mn)and (RList[MainCtrlActualPos-1].Mn>RList[MainCtrlActualPos].Mn) and (TrainType=dt_ET22) and (LastRelayTime<InitialCtrlDelay))then
              begin
                dec(MainCtrlActualPos);
                OK:=true;
                if MainCtrlActualPos>0 then
                   if (RList[MainCtrlActualPos+1].R=0) and ((RList[MainCtrlActualPos+1].ScndAct=0) or (RList[MainCtrlActualPos+1].ScndAct=255)) then  {dzwieki schodzenia z bezoporowej}
                    begin
                    SetFlag(SoundFlag,sound_manyrelay);
                    end;
              end
             else
               begin
               Itot:=0;
               Im:=0;
               OK:=false;      {odciecie pradu, przy przelaczaniu silnikow w ET22}
               end;
     end
             else
             if (MainCtrlPos<RList[MainCtrlActualPos].Relay) and (MainCtrlPos>0) and (TrainType=dt_EZT)  then
              begin
                AutoRelayCheck:=false;
                Exit;    {utkniecie walu}
              end
             else
             if (TrainType=dt_EZT) and (MainCtrlPos=0) then
              begin
                MainCtrlActualPos:=0;
                ScndCtrlActualPos:=0;
              end
              else
             if ((ScndCtrlPos<ScndCtrlActualPos) and (TrainType=dt_EZT)) then
              begin
                Exit;  {utkniecie walu}
              end
             else
              if (MainCtrlPos>RList[MainCtrlActualPos].Relay)  or ((MainCtrlActualPos<RListSize) and (MainCtrlPos=RList[MainCtrlActualPos+1].Relay)) then
              begin
              if ((RList[MainCtrlActualPos].Mn>RList[MainCtrlPos].Mn) and(RList[MainCtrlActualPos+1].Mn<RList[MainCtrlActualPos].Mn) and (TrainType=dt_ET22) and (LastRelayTime<InitialCtrlDelay))then
               begin
               Itot:=0;
               Im:=0;
               OK:=false; {odciecie pradu, przy przelaczaniu silnikow w ET22}
              end
             else
               if Abs(Im)<Imin then
                 begin
                   inc(MainCtrlActualPos);

                   OK:=true;
                   if MainCtrlActualPos>0 then
                    if (RList[MainCtrlActualPos].R=0) and ((RList[MainCtrlActualPos].ScndAct=0) or (RList[MainCtrlActualPos].ScndAct=255)) then  {dzwieki przechodzenia na bezoporowa}
                     begin
                      SetFlag(SoundFlag,sound_manyrelay); SetFlag(SoundFlag,sound_loud);
                     end;
                 end;
               end;
           end;
        end;
     end
    else  {DelayCtrlFlag}
     if ((MainCtrlPos>1) and (MainCtrlActualPos>0) and DelayCtrlFlag) then
      begin
       //if (TrainType<>dt_EZT) then //Ra: w EZT mo¿na daæ od razu na S albo R, wa³ ku³akowy sobie dokrêci
       MainCtrlActualPos:=0; //Ra: tu jest chyba wy³¹czanie przy zbyt szybkim wejœciu na drug¹ pozycjê
       OK:=true;
      end
      else
     if ((MainCtrlPos=0) and (TrainType=dt_EZT)) then
      begin
        MainCtrlActualPos:=0;
        ScndCtrlActualPos:=0; {zejscie walu kulakowego do 0 po ustawieniu nastawnika na 0}
        OK:=true;
      end
     else
      if (MainCtrlPos=1) and (MainCtrlActualPos=0) then
       MainCtrlActualPos:=1
      else
       if (MainCtrlPos=0) and (MainCtrlActualPos>0) and (TrainType<>dt_EZT) and (TrainType<>dt_ET40) and (TrainType<>dt_EP05) then
        begin
         dec(MainCtrlActualPos);
         OK:=true;
        end;
       if (MainCtrlPos=0) and (MainCtrlActualPos>0) and ((TrainType=dt_ET40)or(TrainType=dt_EP05)) and (LastRelayTime>(InitialCtrlDelay*2)) then
        begin
         dec(MainCtrlActualPos);
         OK:=true; {wal kulakowy w ET40}
        end;
    if OK then LastRelayTime:=0;
    AutoRelayCheck:=OK;
  end;
end;
*)
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
   Mains:=false;
end;

function T_MoverParameters.CutOffEngine: boolean; {wylacza uszkodzony silnik}
begin
 CutOffEngine:=false; //Ra: wartoœæ domyœlna, sprawdziæ to trzeba
 if (NPoweredAxles>0) and (CabNo=0) and (EngineType=ElectricSeriesMotor) then
  begin
   if SetFlag(DamageFlag,-dtrain_engine) then
    begin
     NPoweredAxles:=NPoweredAxles div 2;
     CutOffEngine:=true;
    end;
  end
end;

{przelacznik pradu wysokiego rozruchu}
function T_MoverParameters.MaxCurrentSwitch(State:boolean):boolean;
begin
  MaxCurrentSwitch:=false;
  if (EngineType=ElectricSeriesMotor) then
   if (ImaxHi>ImaxLo) then
   begin
     if State and (Imax=ImaxLo) and (RList[MainCtrlPos].Bn<2) and not ((TrainType=dt_ET42)and(MainctrlPos>0)) then
      begin
        Imax:=ImaxHi;
        MaxCurrentSwitch:=true;
        if CabNo<>0 then
         SendCtrlToNext('MaxCurrentSwitch',1,CabNo);
      end;
     if (not State) then
     if (Imax=ImaxHi) then
     if not ((TrainType=dt_ET42)and(MainctrlPos>0)) then
      begin
        Imax:=ImaxLo;
        MaxCurrentSwitch:=true;
        if CabNo<>0 then
         SendCtrlToNext('MaxCurrentSwitch',0,CabNo);
      end;
   end;
end;


{przelacznik pradu automatycznego rozruchu}
function T_MoverParameters.MinCurrentSwitch(State:boolean):boolean;
begin
  MinCurrentSwitch:=false;
  if ((EngineType=ElectricSeriesMotor) and (IminHi>IminLo)) or (TrainType=dt_EZT) then
   begin
     if State and (Imin=IminLo) then
      begin
        Imin:=IminHi;
        MinCurrentSwitch:=true;
        if CabNo<>0 then
         SendCtrlToNext('MinCurrentSwitch',1,CabNo);
      end;
     if (not State) and (Imin=IminHi) then
      begin
        Imin:=IminLo;
        MinCurrentSwitch:=true;
        if CabNo<>0 then
         SendCtrlToNext('MinCurrentSwitch',0,CabNo);
      end;
   end;
end;


{reczne przelaczanie samoczynnego rozruchu}
function T_MoverParameters.AutoRelaySwitch(State:boolean):boolean;
 begin
   if (AutoRelayType=2) and (AutoRelayFlag<>State) then
    begin
      AutoRelayFlag:=State;
      AutoRelaySwitch:=true;
      SendCtrlToNext('AutoRelaySwitch',ord(State),CabNo);
    end
   else
    AutoRelaySwitch:=false;
 end;

function T_MoverParameters.AutoRelayCheck: boolean;
var OK:boolean; //b:byte;
 ARFASI, ARFASI2 :boolean; //sprawdzenie wszystkich warunkow (AutoRelayFlag, AutoSwitch, Im<Imin)
begin
 //Ra 2014-06: dla SN61 nie dzia³a prawid³owo
 //rozlaczanie stycznikow liniowych
  if (not Mains) or (FuseFlag) or (MainCtrlPos=0) or (BrakePress>2.1) or (ActiveDir=0) then   //hunter-111211: wylacznik cisnieniowy
   begin
     StLinFlag:=false; //yBARC - rozlaczenie stycznikow liniowych
     AutoRelayCheck:=false;
     if not DynamicBrakeFlag then
      begin
       Im:=0;
       Itot:=0;
       ResistorsFlag:=false;
      end; 
   end;

   ARFASI2:=(not AutoRelayFlag) or ((MotorParam[ScndCtrlActualPos].AutoSwitch) and (Abs(Im)<Imin)); //wszystkie warunki w jednym
   ARFASI :=(not AutoRelayFlag) or ((RList[MainCtrlActualPos].AutoSwitch) and (Abs(Im)<Imin)) or ((not RList[MainCtrlActualPos].AutoSwitch) and (RList[MainCtrlActualPos].Relay<MainCtrlPos)); //wszystkie warunki w jednym
           //brak PSR                   na tej pozycji dzia³a PSR i pr¹d poni¿ej progu                         na tej pozycji nie dzia³a PSR i pozycja walu ponizej
          //                         chodzi w tym wszystkim o to, ¿eby mo¿na by³o zatrzymaæ rozruch na jakiejœ pozycji wpisuj¹c Autoswitch=0 i wymuszaæ
          //                         przejœcie dalej przez danie nastawnika na dalsz¹ pozycjê - tak to do tej pory dzia³a³o i na tym siê opiera fizyka ET22-2k
   begin
    if (StLinFlag) then
     begin
       if (RList[MainCtrlActualPos].R=0)and ((ScndCtrlActualPos>0) or (ScndCtrlPos>0)) and (not(CoupledCtrl)or(RList[MainCtrlActualPos].Relay=MainCtrlPos))then
        begin //zmieniaj scndctrlactualpos
           begin                                                {scnd bez samoczynnego rozruchu}
             if (ScndCtrlActualPos<ScndCtrlPos) then
              begin
               if (LastRelayTime>CtrlDelay)and(ARFASI2) then
                begin
                 inc(ScndCtrlActualPos);
                 OK:=true;
                end
              end
             else
              if ScndCtrlActualPos>ScndCtrlPos then
               begin
                if (LastRelayTime>CtrlDownDelay)and(TrainType<>dt_EZT) then
                 begin
                  dec(ScndCtrlActualPos);
                  OK:=true;
                 end
               end
             else OK:=false;
           end;
        end
       else
        begin //zmieniaj mainctrlactualpos
          if ((ActiveDir<0) and (TrainType<>dt_PseudoDiesel)) then
           if RList[MainCtrlActualPos+1].Bn>1 then
            begin
              AutoRelayCheck:=false;
              Exit; //Ra: to powoduje, ¿e EN57 nie wy³¹cza siê przy IminLo
            end;
           begin //main bez samoczynnego rozruchu
             if (RList[MainCtrlActualPos].Relay<MainCtrlPos)or(RList[MainCtrlActualPos+1].Relay=MainCtrlPos)or((TrainType=dt_ET22)and(DelayCtrlFlag)) then
              begin
               if (RList[MainCtrlPos].R=0) and (MainCtrlPos>0) and (not (MainCtrlPos=MainCtrlPosNo)) and (FastSerialCircuit=1) then
                begin
                 inc(MainCtrlActualPos);
//                 MainCtrlActualPos:=MainCtrlPos; //hunter-111012: szybkie wchodzenie na bezoporowa (303E)
                 OK:=true;
                 SetFlag(SoundFlag,sound_manyrelay); SetFlag(SoundFlag,sound_loud);
                end
               else if (LastRelayTime>CtrlDelay)and(ARFASI) then
                begin
                 if (TrainType=dt_ET22)and(MainCtrlPos>1)and((RList[MainCtrlActualPos].Bn<RList[MainCtrlActualPos+1].Bn)or(DelayCtrlFlag))then //et22 z walem grupowym
                  if (not DelayCtrlFlag) then //najpierw przejscie
                   begin
                    inc(MainCtrlActualPos);
                    DelayCtrlFlag:=true; //tryb przejscia
                    OK:=true;
                   end
                  else if(LastRelayTime>4*CtrlDelay) then //przejscie
                   begin
                    DelayCtrlFlag:=false;
                    OK:=true;                    
                   end
                  else 
                 else //nie ET22 z wa³em grupowym
                  begin
                   inc(MainCtrlActualPos);
                   OK:=true;
                  end;
                 //---------
                 //hunter-111211: poprawki
                 if MainCtrlActualPos>0 then
                  if (RList[MainCtrlActualPos].R=0) and (not (MainCtrlActualPos=MainCtrlPosNo)) then  //wejscie na bezoporowa
                   begin
                    SetFlag(SoundFlag,sound_manyrelay); SetFlag(SoundFlag,sound_loud);
                   end
                  else if (RList[MainCtrlActualPos].R>0) and (RList[MainCtrlActualPos-1].R=0) then //wejscie na drugi uklad
                   begin
                    SetFlag(SoundFlag,sound_manyrelay);
                   end;
                end
              end
             else if RList[MainCtrlActualPos].Relay>MainCtrlPos then
              begin
               if (RList[MainCtrlPos].R=0) and (MainCtrlPos>0) and (not (MainCtrlPos=MainCtrlPosNo)) and (FastSerialCircuit=1) then
                begin
                 dec(MainCtrlActualPos);
//                 MainCtrlActualPos:=MainCtrlPos; //hunter-111012: szybkie wchodzenie na bezoporowa (303E)
                 OK:=true;
                 SetFlag(SoundFlag,sound_manyrelay);
                end
               else if (LastRelayTime>CtrlDownDelay) then
                begin
                 if (TrainType<>dt_EZT) then //tutaj powinien byæ tryb sterowania wa³em
                  begin
                   dec(MainCtrlActualPos);
                   OK:=true;
                  end;
                 if MainCtrlActualPos>0 then  //hunter-111211: poprawki
                  if RList[MainCtrlActualPos].R=0 then  {dzwieki schodzenia z bezoporowej}
                   begin
                    SetFlag(SoundFlag,sound_manyrelay);
                   end;
                end
              end
             else
              if (RList[MainCtrlActualPos].R>0) and (ScndCtrlActualPos>0) then
               begin
                if (LastRelayTime>CtrlDownDelay) then
                 begin
                  Dec(ScndCtrlActualPos); {boczniki nie dzialaja na poz. oporowych}
                  OK:=true;
                 end
               end
              else
               OK:=false;
           end;
        end;
     end
    else  {not StLinFlag}
     begin
      OK:=false;
      //ybARC - tutaj sa wszystkie warunki, jakie musza byc spelnione, zeby mozna byla zalaczyc styczniki liniowe
      if ((MainCtrlPos=1)or((TrainType=dt_EZT)and(MainCtrlPos>0)))and(not FuseFlag)and(Mains)and(BrakePress<1.0)and(MainCtrlActualPos=0)and(ActiveDir<>0) then
       begin
         DelayCtrlFlag:=true;
         if (LastRelayTime>=InitialCtrlDelay) then
          begin
           StLinFlag:=true; //ybARC - zalaczenie stycznikow liniowych
           MainCtrlActualPos:=1;
           DelayCtrlFlag:=false;
           SetFlag(SoundFlag,sound_relay); SetFlag(SoundFlag,sound_loud);
           OK:=true;
          end;
       end
      else
       DelayCtrlFlag:=false;

      if (not StLinFlag)and((MainCtrlActualPos>0)or(ScndCtrlActualPos>0)) then
       if (TrainType=dt_EZT)and(CoupledCtrl)then //EN57 wal jednokierunkowy calosciowy
        begin
         if(MainCtrlActualPos=1)then
          begin
           MainCtrlActualPos:=0;
           OK:=true;
          end
         else  
         if(LastRelayTime>CtrlDownDelay)then
          begin
           if(MainCtrlActualPos<RListSize)then
            inc(MainCtrlActualPos) //dojdz do konca
           else if(ScndCtrlActualPos<ScndCtrlPosNo)then
            inc(ScndCtrlActualPos) //potem boki
           else
            begin                  //i sie przewroc na koniec
             MainCtrlActualPos:=0;
             ScndCtrlActualPos:=0;
            end;
           OK:=true;
          end
        end
       else if(CoupledCtrl)then //wal kulakowy dwukierunkowy
        begin
         if (LastRelayTime>CtrlDownDelay) then
          begin
           if (ScndCtrlActualPos>0) then
            dec(ScndCtrlActualPos)
           else dec(MainCtrlActualPos);
           OK:=true;
          end
        end
       else
        begin
         MainCtrlActualPos:=0;
         ScndCtrlActualPos:=0;
         OK:=true;
        end;

     end;
    if OK then LastRelayTime:=0;
    AutoRelayCheck:=OK;
   end;
end;

function T_MoverParameters.ResistorsFlagCheck:boolean;  {sprawdzanie wskaznika oporow}
var b:byte;
begin
  ResistorsFlagCheck:=false;
  if Power>0.01 then ResistorsFlagCheck:=ResistorsFlag
  else               {pobor pradu jezeli niema mocy}
   for b:=0 to 1 do
    with Couplers[b] do
     if TestFlag(CouplingFlag,ctrain_controll) then
      if Connected.Power>0.01 then
       ResistorsFlagCheck:=Connected.ResistorsFlagCheck();
end;

function T_MoverParameters.dizel_EngageSwitch(state: real): boolean;
{zmienia parametr do ktorego dazy sprzeglo}
begin
 if (EngineType=DieselEngine) and (state<=1) and (state>=0) and (state<>dizel_engagestate) then
  begin
    dizel_engagestate:=state;
    dizel_EngageSwitch:=true
  end
 else
  dizel_EngageSwitch:=false;
end;

function T_MoverParameters.dizel_EngageChange(dt: real): boolean;
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
function T_MoverParameters.dizel_rotateengine(Momentum,dt,n,engage:real): real;
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

function T_MoverParameters.dizel_fillcheck(mcp:byte): real;
{oblicza napelnienie, uzwglednia regulator obrotow}
var realfill,nreg: real;
begin
  realfill:=0;
  nreg:=0;
  if Mains and (MainCtrlPosNo>0) then
   begin
     if dizel_enginestart and (LastSwitchingTime>=0.9*InitialCtrlDelay) then {wzbogacenie przy rozruchu}
      realfill:=1
     else
      realfill:=RList[mcp].R;                                               {napelnienie zalezne od MainCtrlPos}
     if dizel_nmax_cutoff>0 then
      begin
       case RList[MainCtrlPos].Mn of
        0,1: nreg:=dizel_nmin;
        2:   if(dizel_automaticgearstatus=0)then
              nreg:=dizel_nmax
             else
              nreg:=dizel_nmin;
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
  if realfill>1 then
   realfill:=1;
  dizel_fillcheck:=realfill;
end;

function T_MoverParameters.dizel_AutoGearCheck: boolean;
{automatycznie zmienia biegi gdy predkosc przekroczy widelki}
var OK: boolean;
begin
  OK:=false;
  if MotorParam[ScndCtrlActualPos].AutoSwitch and Mains then
   begin
     if (RList[MainCtrlPos].Mn=0) then
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
        case RList[MainCtrlPos].Mn of
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

function T_MoverParameters.dizel_Update(dt:real): boolean;
{odœwie¿a informacje o silniku}
//var OK:boolean;
const fillspeed=2;
begin
  //dizel_Update:=false;
  if dizel_enginestart and (LastSwitchingTime>=InitialCtrlDelay) then
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
function T_MoverParameters.Adhesive(staticfriction:real): real;
//var Adhesion: real;
begin
  {Adhesion:=0;
  case SlippingWheels of
   true:
      if SandDose then
       Adhesion:=0.48
      else
       Adhesion:=staticfriction*0.2;
   false:
      if SandDose then
       Adhesion:=Max0R(staticfriction*(100+Vel)/(50+Vel)*1.1,0.48)
      else
       Adhesion:=staticfriction*(100+Vel)/(50+Vel)
  end;
  Adhesion:=Adhesion*(11-2*random)/10;
  Adhesive:=Adhesion;}

  //ABu: male przerobki, tylko czy to da jakikolwiek skutek w FPS?
  //     w kazdym razie zaciemni kod na pewno :)
  if SlippingWheels=false then
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

function T_MoverParameters.TractionForce(dt:real):real;
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
     if (Heating) and (HeatingPower>0) and (MainCtrlPosNo>MainCtrlPos) then
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
  else //dla DieselEngine
   begin
     if (ShuntMode) then //dodatkowa przek³adnia np. dla 2Ls150
      dtrans:=AnPos*Transmision.Ratio*MotorParam[ScndCtrlActualPos].mIsat
     else
      dtrans:=Transmision.Ratio*MotorParam[ScndCtrlActualPos].mIsat;
     dmoment:=dizel_Momentum(dizel_fill,ActiveDir*1*dtrans*nrot,dt); {oblicza tez enrot}
   end;
  eAngle:=eAngle+enrot*dt;
  if eAngle>Pirazy2 then                                                       
   //eAngle:=Pirazy2-eAngle; <- ABu: a nie czasem tak, jak nizej?
   eAngle:=eAngle-Pirazy2;

  //hunter-091012: przeniesione z if ActiveDir<>0 (zeby po zejsciu z kierunku dalej spadala predkosc wentylatorow)
  if (EngineType=ElectricSeriesMotor) then
   begin
        case RVentType of {wentylatory rozruchowe}
        1: if ActiveDir<>0 then
            RventRot:=RventRot+(RVentnmax-RventRot)*RVentSpeed*dt
           else
            RventRot:=RventRot*(1-RVentSpeed*dt);
        2: if (Abs(Itot)>RVentMinI) and (RList[MainCtrlActualPos].R>RVentCutOff) then
            RventRot:=RventRot+(RVentnmax*Abs(Itot)/(ImaxLo*RList[MainCtrlActualPos].Bn)-RventRot)*RVentSpeed*dt
           else
           if (DynamicBrakeType=dbrake_automatic) and (DynamicBrakeFlag) then
            RventRot:=RventRot+(RVentnmax*Im/ImaxLo-RventRot)*RVentSpeed*dt           
           else
            begin
              RventRot:=RventRot*(1-RVentSpeed*dt);
              if RventRot<0.1 then RventRot:=0;
            end;
        end; {case}
   end; {if}

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
        if (DynamicBrakeType=dbrake_automatic)and(DynamicBrakeFlag) then
         begin
          if ((Vadd+Abs(Im))>760)or((Hamulec as TLSt).GetEDBCP<0.25) then
           begin
            Vadd:=Vadd-500*dt;
            if(Vadd<1)then
             Vadd:=0;
           end
          else if (DynamicBrakeFlag)and((Vadd+Abs(Im))<740) then
           begin
            Vadd:=Vadd+70*dt;
            Vadd:=Min0R(Max0R(Vadd,60),400);
           end;
          if(Vadd>0)then Mm:=MomentumF(Im,Vadd,0);
         end; 

        if(TrainType=dt_ET22)and(DelayCtrlFlag)then //szarpanie przy zmianie uk³adu w byku
          Mm:=Mm*RList[MainCtrlActualPos].Bn/(RList[MainCtrlActualPos].Bn+1); //zrobione w momencie, ¿eby nie dawac elektryki w przeliczaniu si³

        if (Abs(Im)>Imax) then
         Vhyp:=Vhyp+dt*(Abs(Im)/Imax-0.9)*10 //zwieksz czas oddzialywania na PN
        else
         Vhyp:=0;
        if (Vhyp>CtrlDelay/2) then  //jesli czas oddzialywania przekroczony
         FuseOff;                     {wywalanie bezpiecznika z powodu przetezenia silnikow}
        if (Mains) then //nie wchodziæ w funkcjê bez potrzeby
         if (Abs(Voltage)<EnginePowerSource.CollectorParameters.MinV) or (Abs(Voltage)>EnginePowerSource.CollectorParameters.MaxV) then
          if MainSwitch(false) then
           EventFlag:=true; //wywalanie szybkiego z powodu niew³aœciwego napiêcia

        if (((DynamicBrakeType=dbrake_automatic)or(DynamicBrakeType=dbrake_switch)) and (DynamicBrakeFlag)) then
         Itot:=Im*2   {2x2 silniki w EP09}
        else if (TrainType=dt_EZT)and(Imin=IminLo)and(ScndS) then //yBARC - boczniki na szeregu poprawnie
         Itot:=Im
        else
         Itot:=Im*RList[MainCtrlActualPos].Bn;   {prad silnika * ilosc galezi}
        Mw:=Mm*Transmision.Ratio;
        Fw:=Mw*2.0/WheelDiameter;
        Ft:=Fw*NPoweredAxles;                {sila trakcyjna}
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
            //Ft:=(Ftmax - (Ftmax - (1000.0 * DEList[MainCtrlPosNo].genpower / (Vhyp+Vadd) / PowerCorRatio)) * (tmpV/Vhyp)) * PosRatio //wersja z Megapacka
          else //na hiperboli                             //1.107 - wspolczynnik sredniej nadwyzki Ft w symku nad charakterystyka
            Ft:=1000.0 * tmp / (tmpV+Vadd) / PowerCorRatio //tu jest zawarty stosunek mocy
        else Ft:=0; //jak nastawnik na zero, to sila tez zero

      PosRatio:=tmp/DEList[MainCtrlPosNo].genpower;

      end;
        if (FuseFlag) then Ft:=0 else
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

         if (Imax>1) and (Im>Imax) then FuseOff;
         if FuseFlag then Voltage:=0;

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
          41:
            begin
              if (MainCtrlPos=MainCtrlPosNo) and (tmpV*3.6>MPTRelay[ScndCtrlPos].Iup) and (ScndCtrlPos<ScndCtrlPosNo)then
                begin inc(ScndCtrlPos); enrot:=enrot*0.73; end;
              if (Im>MPTRelay[ScndCtrlPos].Idown)and (ScndCtrlPos>0) then
                dec(ScndCtrlPos);
            end;
          45:
             begin
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
             end;
        end;
     end;
   ElectricInductionMotor:
     begin
       if (Voltage>1800) and (MainS) then
        begin

         if ((Hamulec as TLSt).GetEDBCP<0.25)and(LocHandle.GetCP<0.25) then
           DynamicBrakeFlag:=false
         else if ((BrakePress>0.25) and ((Hamulec as TLSt).GetEDBCP>0.25)) or (LocHandle.GetCP>0.25) then
           DynamicBrakeFlag:=true;

         if(DynamicBrakeFlag)then
          begin
           if eimv[eimv_Fmax]*sign(V)*DirAbsolute<-1 then
             PosRatio:=-sign(V)*DirAbsolute*eimv[eimv_Fr]/(eimc[eimc_p_Fh]*Max0R((Hamulec as TLSt).GetEDBCP,LocHandle.GetCP)/MaxBrakePress[0]{dizel_fill})
           else
             PosRatio:=0;
           PosRatio:=round(20*Posratio)/20;
           if PosRatio<19.5/20 then PosRatio:=PosRatio*0.9;
           (Hamulec as TLSt).SetED(PosRatio);
           PosRatio:=-Max0R((Hamulec as TLSt).GetEDBCP,LocHandle.GetCP)/MaxBrakePress[0]*Max0R(0,Min0R(1,(Vel-eimc[eimc_p_Vh0])/(eimc[eimc_p_Vh1]-eimc[eimc_p_Vh0])));
           tmp:=5;
          end
         else
          begin
           PosRatio:=(MainCtrlPos/MainCtrlPosNo);
           PosRatio:=1.0*(PosRatio*0+1)*PosRatio;
           (Hamulec as TLSt).SetED(0);
           if (PosRatio>dizel_fill) then tmp:=1 else tmp:=4; //szybkie malenie, powolne wzrastanie
          end;
         if SlippingWheels then begin PosRatio:=0; tmp:=10; SandDoseOn; end;//przeciwposlizg

         dizel_fill:=dizel_fill+Max0R(Min0R(PosRatio-dizel_fill,0.1),-0.1)*2*(tmp{2{+4*byte(PosRatio<dizel_fill)})*dt; //wartoœæ zadana/procent czegoœ

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
//         eimv[eimv_U]:=Max0R(eimv[eimv_Uzsmax],Min0R(eimc[eimc_f_cfu]*eimv[eimv_fp],eimv[eimv_Uzsmax]));
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
     end;
   None: begin end;
   end; {case EngineType}
  TractionForce:=Ft
end;

function T_MoverParameters.BrakeForce(Track:TTrackParam):real;
var K,Fb,NBrakeAxles,sm:real;
//const OerlikonForceFactor=1.5;
begin
  K:=0;
  if NPoweredAxles>0 then
   NBrakeAxles:=NPoweredAxles
  else
   NBrakeAxles:=NAxles;
  case LocalBrake of
   NoBrake :      K:=0;
   ManualBrake :  K:=MaxBrakeForce*ManualBrakeRatio;
   HydraulicBrake : K:=MaxBrakeForce*LocalBrakeRatio;
   PneumaticBrake:if Compressor<MaxBrakePress[3] then
                   K:=MaxBrakeForce*LocalBrakeRatio/2.0
                  else
                   K:=0;
  end;
  if MBrake=true then
  begin
   K:=MaxBrakeForce*ManualBrakeRatio;
   end;

                 //0.03

  u:=((BrakePress*P2FTrans)-BrakeCylSpring)*BrakeCylMult[0]-BrakeSlckAdj;
  if (u*BrakeRigEff>Ntotal)then //histereza na nacisku klockow
    Ntotal:=u*BrakeRigEff
  else
   begin
    u:=(BrakePress*P2FTrans)*BrakeCylMult[0]-BrakeSlckAdj;   
    if (u*(2-1*BrakeRigEff)<Ntotal)then //histereza na nacisku klockow
     Ntotal:=u*(2-1*BrakeRigEff);
   end;
   
  if (NBrakeAxles*NBpA>0) then
    begin
      if Ntotal>0 then         {nie luz}
        K:=K+Ntotal;                     {w kN}
      K:=K*BrakeCylNo/(NBrakeAxles*NBpA);            {w kN na os}
    end;
  if (BrakeSystem=Pneumatic)or(BrakeSystem=ElectroPneumatic) then
  begin
    u:=Hamulec.GetFC(Vel, K);
  UnitBrakeForce:=u*K*1000;                     {sila na jeden klocek w N}
   end
  else
    UnitBrakeForce:=K*1000;
  if (NBpA*UnitBrakeForce>TotalMassxg*Adhesive(RunningTrack.friction)/NAxles) and (Abs(V)>0.001) then
   {poslizg}
   begin
{     Fb:=Adhesive(Track.friction)*Mass*g;  }
     SlippingWheels:=true;
   end;
{  else
   begin
{     SlippingWheels:=false;}
//     if (LocalBrake=ManualBrake)or(MBrake=true)) and (BrakePress<0.3) then
//      Fb:=UnitBrakeForce*NBpA {ham. reczny dziala na jedna os}
//     else  //yB: to nie do konca ma sens, poniewa¿ rêczny w wagonie dzia³a na jeden cylinder hamulcowy/wózek, dlatego potrzebne s¹ oddzielnie liczone osie
      Fb:=UnitBrakeForce*NBrakeAxles*NBpA;

//  u:=((BrakePress*P2FTrans)-BrakeCylSpring*BrakeCylMult[BCMFlag]/BrakeCylNo-0.83*BrakeSlckAdj/(BrakeCylNo))*BrakeCylNo;
 {  end; }
  BrakeForce:=Fb;
end;

function T_MoverParameters.CouplerForce(CouplerN:byte;dt:real):real;
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
      CheckCollision:=false;
      newdist:=CoupleDist; //odleg³oœæ od sprzêgu s¹siada
      //newdist:=Distance(Loc,Connected^.Loc,Dim,Connected^.Dim);
      if (CouplerN=0) then
         begin
            //ABu: bylo newdist+10*((...
            tempdist:=((Connected.dMoveLen*DirPatch(0,ConnectedNr))-dMoveLen);
            newdist:=newdist+10.0*tempdist;
            //tempdist:=tempdist+CoupleDist; //ABu: proby szybkiego naprawienia bledu
         end
      else
         begin
            //ABu: bylo newdist+10*((...
            tempdist:=((dMoveLen-(Connected.dMoveLen*DirPatch(1,ConnectedNr))));
            newdist:=newdist+10.0*tempdist;
            //tempdist:=tempdist+CoupleDist; //ABu: proby szybkiego naprawienia bledu
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
            //Connected.Couplers[CNext].Connected:=nil; //Ra: ten pod³¹czony niekoniecznie jest wirtualny
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
            BetaAvg:=(beta+Connected.Couplers[CNext].beta)/2.0;
            Fmax:=(FmaxC+FmaxB+Connected.Couplers[CNext].FmaxC+Connected.Couplers[CNext].FmaxB)*CouplerTune/2.0;
          end;
         dV:=V-DirPatch(CouplerN,CNext)*Connected.V;
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
              CF:=(-(SpringKC+Connected.Couplers[CNext].SpringKC)*Dist/2.0)*DirF(CouplerN)-Fmax*dV*betaAvg
             else
              CF:=(-(SpringKC+Connected.Couplers[CNext].SpringKC)*Dist/2.0)*DirF(CouplerN)*betaAvg-Fmax*dV*betaAvg;
                {liczenie sily ze sprezystosci sprzegu}
            if Dist>(DmaxC+Connected.Couplers[CNext].DmaxC) then {zderzenie}
            //***if tempdist>(DmaxC+Connected^.Couplers[CNext].DmaxC) then {zderzenie}
             CheckCollision:=true;
          end;
         if Dist<0 then
          begin
            if distDelta>0 then
              CF:=(-(SpringKB+Connected.Couplers[CNext].SpringKB)*Dist/2.0)*DirF(CouplerN)-Fmax*dV*betaAvg
            else
              CF:=(-(SpringKB+Connected.Couplers[CNext].SpringKB)*Dist/2.0)*DirF(CouplerN)*betaAvg-Fmax*dV*betaAvg;
                {liczenie sily ze sprezystosci zderzaka}
           if -Dist>(DmaxB+Connected.Couplers[CNext].DmaxB) then  {zderzenie}
            //***if -tempdist>(DmaxB+Connected^.Couplers[CNext].DmaxB)/10 then  {zderzenie}
             begin
               CheckCollision:=true;
               if (CouplerType=Automatic) and (CouplingFlag=0) then  {sprzeganie wagonow z samoczynnymi sprzegami}
                //CouplingFlag:=ctrain_coupler+ctrain_pneumatic+ctrain_controll+ctrain_passenger+ctrain_scndpneumatic;
                CouplingFlag:=ctrain_coupler+ctrain_pneumatic+ctrain_controll; //EN57
             end;
          end;
       end;
      if (CouplingFlag<>ctrain_virtual)
      then         {uzgadnianie prawa Newtona}
       Connected.Couplers[1-CouplerN].CForce:=-CF;
    end;
  CouplerForce:=CF;
end;

procedure T_MoverParameters.CollisionDetect(CouplerN:byte; dt:real);
var CCF,Vprev,VprevC:real; VirtualCoupling:boolean;
begin
     CCF:=0;
     with Couplers[CouplerN] do
      if (Connected<>nil) then
      begin
        VirtualCoupling:=(CouplingFlag=ctrain_virtual);
        Vprev:=V;
        VprevC:=Connected.V;
        case CouplerN of
         0 : CCF:=ComputeCollision(V,Connected.V,TotalMass,Connected.TotalMass,(beta+Connected.Couplers[ConnectedNr].beta)/2.0,VirtualCoupling)/(dt{+0.01}); //yB: ej ej ej, a po
         1 : CCF:=ComputeCollision(Connected.V,V,Connected.TotalMass,TotalMass,(beta+Connected.Couplers[ConnectedNr].beta)/2.0,VirtualCoupling)/(dt{+0.01}); //czemu tu jest +0.01??
        end;
        AccS:=AccS+(V-Vprev)/dt; //korekta przyspieszenia o si³y wynikaj¹ce ze zderzeñ?
        Connected.AccS:=Connected.AccS+(Connected.V-VprevC)/dt;
        if (Dist>0) and (not VirtualCoupling) then
         if FuzzyLogic(Abs(CCF),5*(FmaxC+1),p_coupldmg) then
          begin                            {! zerwanie sprzegu}
            if SetFlag(DamageFlag,dtrain_coupling) then
             EventFlag:=true;
            if (Couplers[CouplerN].CouplingFlag and ctrain_pneumatic>0) then
             EmergencyBrakeFlag:=true;   {hamowanie nagle - zerwanie przewodow hamulcowych}
            CouplingFlag:=0;
            case CouplerN of             {wyzerowanie flag podlaczenia ale ciagle sa wirtualnie polaczone}
             0 : Connected.Couplers[1].CouplingFlag:=0;
             1 : Connected.Couplers[0].CouplingFlag:=0;
            end
          end ;
      end;
end;

procedure T_MoverParameters.ComputeConstans;
var BearingF,RollF,HideModifier: real;
 Curvature: real; //Ra 2014-07: odwrotnoœæ promienia
begin
 TotalCurrent:=0; //Ra 2014-04: tu zerowanie, aby EZT mog³o pobieraæ pr¹d innemu cz³onowi
  TotalMass:=ComputeMass;
  TotalMassxg:=TotalMass*g; {TotalMass*g}
  BearingF:=2*(DamageFlag and dtrain_bearing);

  HideModifier:=0;//Byte(Couplers[0].CouplingFlag>0)+Byte(Couplers[1].CouplingFlag>0);
  with Dim do
   begin
    if BearingType=0 then
     RollF:=0.05          {slizgowe}
    else
     RollF:=0.015;        {toczne}
    RollF:=RollF+BearingF/200.0;

(*    if NPoweredAxles>0 then
     RollF:=RollF*1.5;    {dodatkowe lozyska silnikow}*)
    if (NPoweredAxles>0) then //drobna optymalka
     begin
       RollF:=RollF+0.025;
{       if(Ft*Ft<1)then
         HideModifier:=HideModifier-3; }
     end;
    Ff:=TotalMassxg*(BearingF+RollF*V*V/10.0)/1000.0;
    {dorobic liczenie temperatury lozyska!}
    FrictConst1:=((TotalMassxg*RollF)/10000.0)+(Cx*W*H);
    Curvature:=abs(RunningShape.R); //zero oznacza nieskoñczony promieñ
    if (Curvature>0.0) then Curvature:=1.0/Curvature;
    //opór sk³adu na ³uku (youBy): +(500*TrackW/R)*TotalMassxg*0.001 do FrictConst2s/d
    FrictConst2s:= (TotalMassxg*((500.0*TrackW*Curvature)+ 2.5 - HideModifier + 2*BearingF/dtrain_bearing))*0.001;
    FrictConst2d:= (TotalMassxg*((500.0*TrackW*Curvature)+ 2.0 - HideModifier + BearingF/dtrain_bearing))*0.001;
   end;
end;

(* poprawione opory ruchu czekajace na lepsze cx w chk
procedure T_MoverParameters.ComputeConstans2(R:real);
var BearingF,RollF,HideModifier: real;
begin
  TotalMass:=ComputeMass;
  TotalMassxg:=TotalMass*g; {TotalMass*g}
  BearingF:=2*(DamageFlag and dtrain_bearing);

  HideModifier:=Byte(Couplers[0].CouplingFlag>0)+Byte(Couplers[1].CouplingFlag>0);
  with Dim do
   begin
    if BearingType=0 then
     RollF:=10          {slizgowe}
    else
     RollF:=8;        {toczne}
    if NPoweredAxles>0 then
     RollF:=RollF+2;    {dodatkowe lozyska silnikow}

    RollF:=RollF+BearingF/100.0; //stala do ustalenia

    {dorobic liczenie temperatury lozyska!}
    //stale do wzoru CNTK
    FrictConst1:=(Cx*W*H)*(6.0-2.5*HideModifier)/100; //dwa czola, kazde ma +2.5 (lokomotywa i wagon)  //V2
    FrictConst2s:=Totalmass*0.001*0.15;                                                                //V1
    if(R<TrackW)then //jakakolwiek wartosc dodatnia, np. szerokosc albo rozstaw
      FrictConst2d:=RollF*TotalMassxg*0.001+NAxles*150;                                                //V0 na prostej
    else
      FrictConst2d:=(RollF+500*TrackW/R)*TotalMassxg*0.001+NAxles*150;                                 //V0 na luku
   end;
end; *)


function T_MoverParameters.FrictionForce(R:real;TDamage:byte):real;
begin
//ABu 240205: chyba juz ekstremalnie zoptymalizowana funkcja liczaca sily tarcia
   if (abs(V)>0.01) then
      FrictionForce:=(FrictConst1*V*V)+FrictConst2d
   else
      FrictionForce:=(FrictConst1*V*V)+FrictConst2s;
end;

(* poprawione opory ruchu czekajace na lepsze cx w chk
function T_MoverParameters.FrictionForce2(R:real;TDamage:byte):real;
begin
//ABu 240205: chyba juz ekstremalnie zoptymalizowana funkcja liczaca sily tarcia
    FrictionForce2:=(FrictConst1*Vel*Vel)+(FrictConst2s*Vel)+FrictConst2d;
end; *)

function T_MoverParameters.AddPulseForce(Multipler:integer): boolean; {dla drezyny}
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
  else AddPulseForce:=false;
end;


function T_MoverParameters.LoadingDone(LSpeed:real; LoadInit:string): boolean;
//test zakoñczenia za³adunku/roz³adunku
var LoadChange:longint;
begin
 ClearPendingExceptions; //zabezpieczenie dla Trunc()
 //LoadingDone:=false; //nie zakoñczone
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
{WAZNA FUNKCJA - oblicza si³ê wypadkow¹}
procedure T_MoverParameters.ComputeTotalForce(dt: real; dt1: real; FullVer: boolean);
var b: byte;
begin
  if PhysicActivation then
   begin
  {  EventFlag:=false;                                             {jesli cos sie zlego wydarzy to ustawiane na true}
  {  SoundFlag:=0;                                                 {jesli ma byc jakis dzwiek to zapalany jet odpowiedni bit}
  {to powinno byc zerowane na zewnatrz}

    //juz zoptymalizowane:
    FStand:=FrictionForce(RunningShape.R,RunningTrack.DamageFlag); //si³a oporów ruchu
    Vel:=Abs(V)*3.6; //prêdkoœæ w km/h
    nrot:=V2n(); //przeliczenie prêdkoœci liniowej na obrotow¹

    if TestFlag(BrakeMethod,bp_MHS) and (PipePress<3.0) and (Vel>45) and TestFlag(BrakeDelayFlag,bdelay_M) then //ustawione na sztywno na 3 bar
      FStand:=Fstand+{(RunningTrack.friction)*}TrackBrakeForce; //doliczenie hamowania hamulcem szynowym
                   //w charakterystykach jest wartoœæ si³y hamowania zamiast nacisku
//    if(FullVer=true) then
//    begin //ABu: to dla optymalizacji, bo chyba te rzeczy wystarczy sprawdzac 1 raz na klatke?
       LastSwitchingTime:=LastSwitchingTime+dt1;
       if EngineType=ElectricSeriesMotor then
          LastRelayTime:=LastRelayTime+dt1;
//    end;
       if Mains and {(Abs(CabNo)<2) and} (EngineType=ElectricSeriesMotor) then //potem ulepszyc! pantogtrafy!
       begin //Ra 2014-03: uwzglêdnienie kierunku jazdy w napiêciu na silnikach, a powinien byæ zdefiniowany nawrotnik
          if CabNo=0 then
             Voltage:=RunningTraction.TractionVoltage*ActiveDir
          else
             Voltage:=RunningTraction.TractionVoltage*DirAbsolute; //ActiveDir*CabNo;
          end {bo nie dzialalo}
       else
          Voltage:=0;
       if Mains and {(Abs(CabNo)<2) and} (EngineType=ElectricInductionMotor) then //potem ulepszyc! pantogtrafy!
          Voltage:=RunningTraction.TractionVoltage;
    //end;

    if Power>0 then
       FTrain:=TractionForce(dt)
    else
       FTrain:=0;
    Fb:=BrakeForce(RunningTrack);
    if Max0R(Abs(FTrain),Fb)>TotalMassxg*Adhesive(RunningTrack.friction) then    {poslizg}
     SlippingWheels:=true;
    if SlippingWheels then
     begin
  {     TrainForce:=TrainForce-Fb; }
       nrot:=ComputeRotatingWheel((FTrain-Fb*Sign(V)-Fstand)/NAxles-Sign(nrot*Pi*WheelDiameter-V)*Adhesive(RunningTrack.friction)*TotalMass,dt,nrot);
       FTrain:=sign(FTrain)*TotalMassxg*Adhesive(RunningTrack.friction);
       Fb:=Min0R(Fb,TotalMassxg*Adhesive(RunningTrack.friction));
     end;
  {  else SlippingWheels:=false; }
  {  FStand:=0; }
    for b:=0 to 1 do
     if (Couplers[b].Connected<>nil) {and (Couplers[b].CouplerType<>Bare) and (Couplers[b].CouplerType<>Articulated)} then
      begin //doliczenie si³ z innych pojazdów
        Couplers[b].CForce:=CouplerForce(b,dt);
        FTrain:=FTrain+Couplers[b].CForce
      end
     else Couplers[b].CForce:=0;
    //FStand:=Fb+FrictionForce(RunningShape.R,RunningTrack.DamageFlag);
    FStand:=Fb+FStand;
    FTrain:=FTrain+TotalMassxg*RunningShape.dHtrack; //doliczenie sk³adowej stycznej grawitacji
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
function T_MoverParameters.ComputeMovement(dt:real; dt1:real; Shape:TTrackShape; var Track:TTrackParam; var ElectricTraction:TTractionParam; NewLoc:TLocation; var NewRot:TRotation):real;
var b:byte;
    Vprev,AccSprev:real;
//    Iheat:real; prad ogrzewania
const Vepsilon=1e-5; Aepsilon=1e-3; //ASBSpeed=0.8;
begin
{
    for b:=0 to 1 do //przekazywanie napiec
     if ((Couplers[b].CouplingFlag and ctrain_power) = ctrain_power)or(((Couplers[b].CouplingFlag and ctrain_heating) = ctrain_heating)and(Heating)) then
      begin
        HVCouplers[1-b][1]:=Max0R(Abs(Voltage),Couplers[b].Connected.HVCouplers[Couplers[b].ConnectedNr][1]);
      end
     else
        HVCouplers[1-b][1]:=Max0R(Abs(Voltage),0);
      end;

    for b:=0 to 1 do //przekazywanie pradow
     if ((Couplers[b].CouplingFlag and ctrain_power) = ctrain_power)or(((Couplers[b].CouplingFlag and ctrain_heating) = ctrain_heating)and(Heating)) then //jesli spiety
      begin
        if (HVCouplers[b][1]) > 1 then //przewod pod napiecie
          HVCouplers[b][0]:=Couplers[b].Connected.HVCouplers[1-Couplers[b].ConnectedNr][0]+Iheat//obci¹¿enie
        else
          HVCouplers[b][0]:=0;
      end
     else //pierwszy pojazd
      begin
        if (HVCouplers[b][1]) > 1 then //pod napieciem
          HVCouplers[b][0]:=0+Iheat//obci¹¿enie
        else
          HVCouplers[b][0]:=0;
      end;
}

  ClearPendingExceptions;
  if not TestFlag(DamageFlag,dtrain_out) then
   begin //Ra: to przepisywanie tu jest bez sensu
     RunningShape:=Shape;
     RunningTrack:=Track;
     RunningTraction:=ElectricTraction;
     with ElectricTraction do
      if not DynamicBrakeFlag then
       RunningTraction.TractionVoltage:=TractionVoltage-Abs(TractionResistivity*(Itot+HVCouplers[0][0]+HVCouplers[1][0]))
      else
       RunningTraction.TractionVoltage:=TractionVoltage-Abs(TractionResistivity*Itot*0); //zasadniczo ED oporowe nie zmienia napiêcia w sieci
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
     AccS:=(FTotal/TotalMass+AccSprev)/2.0; //prawo Newtona ale z wygladzaniem (œrednia z poprzednim)

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
           EventFlag:=true;
           MainS:=false;
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
            EventFlag:=true;
            MainS:=false;
            RunningShape.R:=0;
            DerailReason:=3; //Ra: powód wykolejenia: za szeroki tor
          end;
      end;
     {wykolejanie wkutek niezgodnosci kategorii toru i pojazdu}
     if not TestFlag(RunningTrack.CategoryFlag,CategoryFlag) then
      if SetFlag(DamageFlag,dtrain_out) then
        begin
          EventFlag:=true;
          MainS:=false;
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
         //AccS:=0; //Ra 2014-03: ale si³a grawitacji dzia³a, wiêc nie mo¿e byæ zerowe
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

   {Ra: reszta przeniesiona do Mover.cpp}
end; {ComputeMovement}

{blablabla}
{OBLICZENIE PRZEMIESZCZENIA}
function T_MoverParameters.FastComputeMovement(dt:real; Shape:TTrackShape; var Track:TTrackParam;NewLoc:TLocation; var NewRot:TRotation):real; //;var ElectricTraction:TTractionParam; NewLoc:TLocation; var NewRot:TRotation):real;
var b:byte;
    Vprev,AccSprev:real;
const Vepsilon=1e-5; Aepsilon=1e-3; //ASBSpeed=0.8;
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
     AccS:=(FTotal/TotalMass+AccSprev)/2.0; //prawo Newtona ale z wygladzaniem (œrednia z poprzednim)

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

     V:=V+(3.0*AccS-AccSprev)*dt/2.0;                                  {przyrost predkosci}

     if TestFlag(DamageFlag,dtrain_out) then
      if Vel<1 then
       begin
         V:=0;
         AccS:=0; //Ra 2014-03: ale si³a grawitacji dzia³a, wiêc nie mo¿e byæ zerowe
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

   {Ra: reszta przeniesiona do Mover.cpp}
end; {FastComputeMovement}



function T_MoverParameters.ComputeMass: real;
var M:real;
begin
 if Load>0 then
  begin //zak³adamy, ¿e ³adunek jest pisany ma³ymi literami
   if LoadQuantity='tonns' then
    M:=Load*1000
   else if (LoadType='passengers') then
    M:=Load*80
   else if LoadType='luggage' then
    M:=Load*100
   else if LoadType='cars' then
    M:=Load*800
   else if LoadType='containers' then
    M:=Load*8000
   else if LoadType='transformers' then
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

function T_MoverParameters.SetInternalCommand(NewCommand:string; NewValue1,NewValue2:real):boolean;
begin
  if (CommandIn.Command=Newcommand) and (commandIn.Value1=NewValue1) and (commandIn.Value2=NewValue2) then
   SetInternalCommand:=false
  else
   with CommandIn do
    begin
     Command:=NewCommand;
     Value1:=NewValue1;
     Value2:=NewValue2;
     SetInternalCommand:=true;
     LastLoadChangeTime:=0; //zerowanie czasu (roz)³adowania
    end;
end;

function T_MoverParameters.GetExternalCommand(var Command:string):real;
begin
  Command:=CommandOut;
  GetExternalCommand:=ValueOut;
end;

function T_MoverParameters.DoorBlockedFlag:Boolean;
begin
 //if (DoorBlocked=true) and (Vel<5.0) then
 DoorBlockedFlag:=false;
 if (DoorBlocked=true) and (Vel>=5.0) then
  DoorBlockedFlag:=true
end;

function T_MoverParameters.RunCommand(command:string; CValue1,CValue2:real):boolean;
//wys³anie komendy otrzymanej z kierunku CValue2 (wzglêdem sprzêgów: 1=przod,-1=ty³)
// Ra: Jest tu problem z rekurencj¹. Trzeba by oddzieliæ wykonywanie komend od mechanizmu
// ich propagacji w sk³adzie. Osobnym problemem mo¿e byæ propagacja tylko w jedn¹ stronê.
// Jeœli jakiœ cz³on jest wstawiony odwrotnie, to równie¿ odwrotnie musi wykonywaæ
// komendy zwi¹zane z kierunkami (PantFront, PantRear, DoorLeft, DoorRight).
// Komenda musi byæ zdefiniowana tutaj, a jeœli siê wywo³uje funkcjê, to ona nie mo¿e
// sama przesy³aæ do kolejnych pojazdów. Nale¿y te¿ siê zastanowiæ, czy dla uzyskania
// jakiejœ zmiany (np. IncMainCtrl) lepiej wywo³aæ funkcjê, czy od razu wys³aæ komendê.
var OK:boolean; testload:string;
Begin
{$B+} {cholernie mi sie nie podoba ta dyrektywa!!!}
  OK:=false;
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
     Hamulec.SetEPS(CValue1);
     //fBrakeCtrlPos:=BrakeCtrlPos; //to powinnno byæ w jednym miejscu, aktualnie w C++!!!
     BrakePressureActual:=BrakePressureTable[BrakeCtrlPos];
     OK:=SendCtrlToNext(command,CValue1,CValue2);
   end //youby - odluzniacz hamulcow, przyda sie
  else if command='BrakeReleaser' then
   begin
     OK:=BrakeReleaser(Round(CValue1)); //samo siê przesy³a dalej
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
    case Trunc(CValue1*CValue2) of //CValue2 ma zmieniany znak przy niezgodnoœci sprzêgów
      1 : CabNo:= 1;
     -1 : CabNo:=-1;
    else CabNo:=0; //gdy CValue1==0
    end;
    DirAbsolute:=ActiveDir*CabNo;
    OK:=SendCtrlToNext(command,CValue1,CValue2);
   end
  else if command='AutoRelaySwitch' then
   begin
     if (CValue1=1) and (AutoRelayType=2) then AutoRelayFlag:=true
     else AutoRelayFlag:=false;
     OK:=SendCtrlToNext(command,CValue1,CValue2);
   end
  else if command='FuseSwitch' then
   begin
      if ((EngineType=ElectricSeriesMotor)or(EngineType=DieselElectric)) and FuseFlag and (CValue1=1) and
         (MainCtrlActualPos=0) and (ScndCtrlActualPos=0) and Mains then
{      if (EngineType=ElectricSeriesMotor) and (CValue1=1) and
         (MainCtrlActualPos=0) and (ScndCtrlActualPos=0) and Mains then}
        FuseFlag:=false;  {wlaczenie ponowne obwodu}
     // if ((EngineType=ElectricSeriesMotor)or(EngineType=DieselElectric)) and not FuseFlag and (CValue1=0) and Mains then
     //   FuseFlag:=true;
      OK:=SendCtrlToNext(command,CValue1,CValue2);
   end
  else if command='ConverterSwitch' then         {NBMX}
   begin
     if (CValue1=1) then ConverterAllow:=true
     else if (CValue1=0) then ConverterAllow:=false;
     OK:=SendCtrlToNext(command,CValue1,CValue2);
   end
  else if command='BatterySwitch' then         {NBMX}
   begin
     if (CValue1=1) then Battery:=true
     else if (CValue1=0) then Battery:=false;
     if (Battery) and (ActiveCab<>0) {or (TrainType=dt_EZT)} then
      SecuritySystem.Status:=SecuritySystem.Status or s_waiting //aktywacja czuwaka
     else
      SecuritySystem.Status:=0; //wy³¹czenie czuwaka
     OK:=SendCtrlToNext(command,CValue1,CValue2);
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
     OK:=SendCtrlToNext(command,CValue1,CValue2);
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
    OK:=SendCtrlToNext(command,CValue1,CValue2);
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
    OK:=SendCtrlToNext(command,CValue1,CValue2);
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
       OK:=true
     else OK:=false;
   end
  else if command='BrakeDelay' then
   begin
      BrakeDelayFlag:=Trunc(CValue1);
      OK:=true;
   end
  else if command='SandDoseOn' then
   begin
     if SandDoseOn then
       OK:=true
     else OK:=false;
   end
  else if command='CabSignal' then {SHP,Indusi}
   begin //Ra: to powinno dzia³aæ tylko w cz³onie obsadzonym
     if {(TrainType=dt_EZT)or} (ActiveCab<>0) and (Battery) and TestFlag(SecuritySystem.SystemType,2) then //jeœli kabina jest obsadzona (silnikowy w EZT?)
      with SecuritySystem do
       begin
        VelocityAllowed:=Trunc(CValue1);
        NextVelocityAllowed:=Trunc(CValue2);
        SystemSoundSHPTimer:=0; //hunter-091012
        SetFlag(Status,s_active);
       end;
     //else OK:=false;
    OK:=true; //true, gdy mo¿na usun¹æ komendê
   end
 {naladunek/rozladunek}
  else if Pos('Load=',command)=1 then
   begin
    OK:=false; //bêdzie powtarzane a¿ siê za³aduje
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
    OK:=false; //bêdzie powtarzane a¿ siê roz³aduje
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

function T_MoverParameters.RunInternalCommand:boolean;
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
   OK:=false;
  RunInternalCommand:=OK;
end;

procedure T_MoverParameters.PutCommand(NewCommand:string; NewValue1,NewValue2:real; NewLocation:TLocation);
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

{dla samochodow - kolej nie wê¿ykuje}
function T_MoverParameters.ChangeOffsetH(DeltaOffset:real):boolean;
begin
  if TestFlag(CategoryFlag,2) and TestFlag(RunningTrack.CategoryFlag,2) then
   begin
     OffsetTrackH:=OffsetTrackH+DeltaOffset;
//     if abs(OffsetTrackH)>(RunningTrack.Width/1.95-TrackW/2.0) then
     if abs(OffsetTrackH)>(0.5*(RunningTrack.Width-Dim.W)-0.05) then //Ra: mo¿e pó³ pojazdu od brzegu?
      ChangeOffsetH:=false  {kola na granicy drogi}
     else
      ChangeOffsetH:=true;
   end
  else ChangeOffsetH:=false;
end;

{inicjalizacja}

constructor T_MoverParameters.Init(//LocInitial:TLocation; RotInitial:TRotation;
                                  VelInitial:real; TypeNameInit, NameInit: string; LoadInitial:longint; LoadTypeInitial: string; Cab:integer);
                                 {predkosc pocz. w km/h, ilosc ladunku, typ ladunku, numer kabiny}
var b,k:integer;
begin
  {inicjalizacja stalych}
  dMoveLen:=0;
  CategoryFlag:=1;
  EngineType:=None;
  for b:=0 to ResArraySize do
   begin
     RList[b].Relay:=0;
     RList[b].R:=0;
     RList[b].Bn:=0;
     RList[b].Mn:=0;
     RList[b].AutoSwitch:=false;
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
     SourceType:=NotDefined; PowerType:=NoPower; RPowerCable.PowerTrans:=NoPower;
   end;
  with AlterHeatPowerSource do
   begin
     MaxVoltage:=0; MaxCurrent:=0; IntR:=0.001;
     SourceType:=NotDefined; PowerType:=NoPower; RPowerCable.PowerTrans:=NoPower;
   end;
  with LightPowerSource do
   begin
     MaxVoltage:=0; MaxCurrent:=0; IntR:=0.001;
     SourceType:=NotDefined; PowerType:=NoPower; RPowerCable.PowerTrans:=NoPower;
   end;
  with AlterLightPowerSource do
   begin
     MaxVoltage:=0; MaxCurrent:=0; IntR:=0.001;
     SourceType:=NotDefined; PowerType:=NoPower; RPowerCable.PowerTrans:=NoPower;
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
  RapidMult:=1;
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
  //Loc:=LocInitial; //Ra: to i tak trzeba potem przesun¹æ, po ustaleniu pozycji na torze (potrzebna d³ugoœæ)
  //Rot:=RotInitial;
  for b:=0 to 1 do
   with Couplers[b] do
    begin
      AllowedFlag:=3; //domyœlnie hak i hamulec, inne trzeba w³¹czyæ jawnie w FIZ
      CouplingFlag:=0;
      Connected:=nil;
      ConnectedNr:=0; //Ra: to nie ma znaczenia jak nie pod³¹czony
      Render:=false;
      CForce:=0;
      Dist:=0;
      CheckCollision:=false;
    end;
  ScanCounter:=0;
  BrakeCtrlPos:=-2; //to nie ma znaczenia, konstruktor w Mover.cpp zmienia na -2
  BrakeDelayFlag:=0;
  BrakeStatus:=b_off;
  EmergencyBrakeFlag:=false;
  MainCtrlPos:=0;
  ScndCtrlPos:=0;
  MainCtrlActualPos:=0;
  ScndCtrlActualPos:=0;
  Heating:=false;
  Mains:=false;
  ActiveDir:=0; //kierunek nie ustawiony
  CabNo:=0; //sterowania nie ma, ustawiana przez CabActivization()
  ActiveCab:=Cab;  //obsada w podanej kabinie
  DirAbsolute:=0;
  SlippingWheels:=false;
  SandDose:=false;
  FuseFlag:=false;
  ConvOvldFlag:=false;   //hunter-251211
  StLinFlag:=false;
  ResistorsFlag:=false;
  Rventrot:=0;
  enrot:=0;
  nrot:=0;
  Itot:=0;
  EnginePower:=0;
  BrakePress:=0;
  Compressor:=0;
  ConverterFlag:=false;
  CompressorAllow:=false;
  DoorLeftOpened:=false;
  DoorRightOpened:=false;
  battery:=false;
  EpFuse:=true;
  Signalling:=false;
  Radio:=true;
  DoorSignalling:=false;
  UnBrake:=false;
//Winger 160204
  PantVolume:=0.48; //aby podniesione pantografy opad³y w krótkim czasie przy wy³¹czonej sprê¿arce
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
  DynamicBrakeFlag:=false;
  BrakeSystem:=Individual;
  BrakeSubsystem:=ss_None;
  Ft:=0; Ff:=0; Fb:=0;
  Ftotal:=0; FStand:=0; FTrain:=0;
  AccS:=0; AccN:=0; AccV:=0;
  EventFlag:=false; SoundFlag:=0;
  Vel:=Abs(VelInitial); V:=VelInitial/3.6;
  LastSwitchingTime:=0;
  LastRelayTime:=0;
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
     R:=0; Len:=1; dHtrack:=0; dHrail:=0;
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
      SystemTimer:=0; SystemBrakeCATimer:=0; SystemBrakeSHPTimer:=0; //hunter-091012
      VelocityAllowed:=-1; NextVelocityAllowed:=-1;
      RadioStop:=false; //domyœlnie nie ma
      AwareMinSpeed:=0.1*Vmax;
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
 TotalCurrent:=0;
 ShuntModeAllow:=false;
 ShuntMode:=false;
end;

function T_MoverParameters.EngineDescription(what:integer): string;  {opis stanu lokomotywy}
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

function T_MoverParameters.CheckLocomotiveParameters(ReadyFlag:boolean;Dir:longint): boolean;
var //k:integer;
    OK:boolean;
    b:byte;
    bcmsno: byte;
    bcmss: byte;
const
    DefBrakeTable:array[1..8]of Byte=(15,4,25,25,13,3,12,2);
begin
  OK:=true;
  AutoRelayFlag:=(AutoRelayType=1);
  Sand:=SandCapacity;
  if (Pos('o',AxleArangement)>0) and (EngineType=ElectricSeriesMotor) then
   OK:=(RList[1].Bn*RList[1].Mn=NPoweredAxles); {test poprawnosci ilosci osi indywidualnie napedzanych}
  if (Pos(LoadType,LoadAccepted)=0) and (LoadType<>'') then
   begin
     Load:=0;
     OK:=false;
   end;
  if BrakeSystem=Individual then
   if BrakeSubSystem<>ss_None then
    OK:=false; {!}

 if(BrakeVVolume=0)and(MaxBrakePress[3]>0)and(BrakeSystem<>Individual)then
   BrakeVVolume:=MaxBrakePress[3]/(5-MaxBrakePress[3])*(BrakeCylRadius*BrakeCylRadius*BrakeCylDist*BrakeCylNo*pi)*1000;
 if BrakeVVolume=0 then BrakeVVolume:=0.01;

//  Hamulec.Free;
//(i_mbp, i_bcr, i_bcd, i_brc: real; i_bcn, i_BD, i_mat, i_ba, i_nbpa: byte)

case BrakeValve of
  W, K : begin
          Hamulec := TWest.Create(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume, BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA);
          if(MBPM<2)then //jesli przystawka wazaca
           (Hamulec as TWest).SetLP(0,MaxBrakePress[3],0)
          else
           (Hamulec as TWest).SetLP(Mass, MBPM, MaxBrakePress[1]);
         end;
  KE :   begin
          Hamulec := TKE.Create(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume, BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA);
         (Hamulec as TKE).SetRM(RapidMult);
          if(MBPM<2)then //jesli przystawka wazaca
           (Hamulec as TKE).SetLP(0,MaxBrakePress[3],0)
         else
           (Hamulec as TKE).SetLP(Mass, MBPM, MaxBrakePress[1])
         end;
  NEst3, ESt3, ESt3AL2, ESt4:
         begin
          Hamulec := TNESt3.Create(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume, BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA);
         (Hamulec as TNEst3).SetSize(BrakeValveSize,BrakeValveParams);
          if(MBPM<2)then //jesli przystawka wazaca
           (Hamulec as TNESt3).SetLP(0,MaxBrakePress[3],0)
         else
           (Hamulec as TNESt3).SetLP(Mass, MBPM, MaxBrakePress[1])
         end;
{  ESt3, ESt3AL2: if (MaxBrakePress[1])>0 then
         begin
          Hamulec := TESt3AL2.Create(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume, BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA);
          if(MBPM<2)then
         (Hamulec as TESt3AL2).SetLP(0,MaxBrakePress[3],0)
         else
         (Hamulec as TESt3AL2).SetLP(Mass, MBPM, MaxBrakePress[1])
         end
        else
          Hamulec := TESt3.Create(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume, BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA);}
  LSt   :begin
          Hamulec :=  TLSt.Create(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume, BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA);
          (Hamulec as TLSt).SetRM(RapidMult);
         end;
  EStED :begin
          Hamulec :=  TEStED.Create(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume, BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA);
          (Hamulec as TEStED).SetRM(RapidMult);
         end;
  EP2:begin
         Hamulec :=TEStEP2.Create(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume, BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA);
         (Hamulec as TEStEP2).SetLP(Mass, MBPM, MaxBrakePress[1]);
        end;
{  ESt4  :if (BrakeDelays and bdelay_R)=bdelay_R then
           Hamulec:=TESt4R.Create(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume, BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA)
         else
           Hamulec:= TESt.Create(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume, BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA);}
  CV1  :  Hamulec:= TCV1.Create(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume, BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA);
  CV1_L_TR:Hamulec:= TCV1L_TR.Create(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume, BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA)
else
  Hamulec :=  TBrake.Create(MaxBrakePress[3], BrakeCylRadius, BrakeCylDist, BrakeVVolume, BrakeCylNo, BrakeDelays, BrakeMethod, NAxles, NBpA);
end;
Hamulec.SetASBP(MaxBrakePress[4]);

case BrakeHandle of
  FV4a: Handle := TFV4aM.Create;
  FVel6: Handle := TFVel6.Create;
  testH: Handle := Ttest.Create;
  M394: Handle := TM394.Create;
  Knorr: Handle := TH14K1.Create;
  St113: Handle := TSt113.Create;
else
  Handle := THandle.Create;
end;

case BrakeLocHandle of
  FD1:
   begin
    LocHandle := TFD1.Create;
    LocHandle.Init(MaxBrakePress[0]);
   end;
  Knorr:
   begin
    LocHandle := TH1405.Create;
    LocHandle.Init(MaxBrakePress[0]);
   end;
else
  LocHandle := THandle.Create;
end;

//ustalanie srednicy przewodu glownego (lokomotywa lub napêdowy
  if (TestFlag(BrakeDelays,bdelay_G))and((not TestFlag(BrakeDelays,bdelay_R)) or (Power>1))then
    Spg:=0.792
  else
    Spg:=0.507;

  Pipe:= TReservoir.Create;
  Pipe2:= TReservoir.Create;  //zabezpieczenie, bo sie PG wywala... :(
  Pipe.CreateCap((Max0R(Dim.L,14)+0.5)*Spg*1); //dlugosc x przekroj x odejscia i takie tam
  Pipe2.CreateCap((Max0R(Dim.L,14)+0.5)*Spg*1);

 {to dac potem do init}
  if ReadyFlag then     {gotowy do drogi}
   begin
     CompressedVolume:=VeselVolume*MinCompressor*(9.8+Random)/10;
     ScndPipePress:=CompressedVolume/VeselVolume;
     PipePress:=CntrlPipePress;
     BrakePress:=0;
     LocalBrakePos:=0;
//     if (BrakeSystem=Pneumatic) and (BrakeCtrlPosNo>0) then
      if CabNo=0 then
//       BrakeCtrlPos:=-2; //odciêcie na zespolonym; Ra: hamulec jest poprawiany w DynObj.cpp
       BrakeCtrlPos:=Trunc(Handle.GetPos(bh_NP))
     else
       BrakeCtrlPos:=Trunc(Handle.GetPos(bh_RP));
     MainSwitch(false);
     PantFront(true);
     PantRear(true);
     MainSwitch(true);
     ActiveDir:=0; //Dir; //nastawnik kierunkowy - musi byæ ustawiane osobno!
     DirAbsolute:=ActiveDir*CabNo; //kierunek jazdy wzglêdem sprzêgów
     LimPipePress:=CntrlPipePress;
   end
  else
   begin                {zahamowany}
     Volume:=BrakeVVolume*MaxBrakePress[3];
     CompressedVolume:=VeselVolume*MinCompressor*0.55;
     ScndPipePress:=5.1;
     PipePress:=LowPipePress;
     PipeBrakePress:=MaxBrakePress[3];
     BrakePress:=MaxBrakePress[3];
     LocalBrakePos:=0;
//     if (BrakeSystem=Pneumatic) and (BrakeCtrlPosNo>0) then
//      BrakeCtrlPos:=-2; //Ra: hamulec jest poprawiany w DynObj.cpp
     BrakeCtrlPos:=Trunc(Handle.GetPos(bh_NP));
     LimPipePress:=LowPipePress;
   end;
  ActFlowSpeed:=0;
  BrakeCtrlPosR:=BrakeCtrlPos;

  if(BrakeLocHandle=Knorr)then
    LocalBrakePos:=5;

  Pipe.CreatePress(PipePress);
  Pipe2.CreatePress(ScndPipePress);
  Pipe.Act;Pipe2.Act;

  EqvtPipePress:=PipePress;

////  fv4ac:=Pipe.P;
////  fv4ar:=Pipe.P;

  Handle.Init(PipePress);

  ComputeConstans();


  if(LoadFlag>0)then
     begin
    if(Load<MaxLoad*0.45)then
     begin
      IncBrakeMult();IncBrakeMult();DecBrakeMult();
      if(Load<MaxLoad*0.35)then
        DecBrakeMult();
     end;
    if(Load>=MaxLoad*0.45)then
     begin
      IncBrakeMult();
      if(Load>=MaxLoad*0.55)then
        IncBrakeMult();
     end;
  end;

//taki mini automat - powinno byc ladnie dobrze :)
    BrakeDelayFlag:=bdelay_P;
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

  if(TrainType=dt_ET22)then
    CompressorPower:=0;

//  Hamulec.Init(10*PipePress, 10*HighPipePress-0.15, 10*LowPipePress-0.15, 10*BrakePress, BrakeDelayFlag);
  Hamulec.Init(PipePress, HighPipePress, LowPipePress, BrakePress, BrakeDelayFlag);
//  Hamulec.Init(0, 0, -1.5, 0, BrakeDelayFlag);
//  PipePress:=PipePress;
  ScndPipePress:=Compressor;
  CheckLocomotiveParameters:=OK;
end;

function T_MoverParameters.DoorLeft(State: Boolean):Boolean;
begin
 if (DoorLeftOpened<>State) and (DoorBlockedFlag=false) and (Battery=true) then
 begin
  DoorLeft:=true;
  DoorLeftOpened:=State;
  if (State=true) then
   begin
    if (CabNo>0) then
     SendCtrlToNext('DoorOpen',1,CabNo) //1=lewe, 2=prawe
    else
     SendCtrlToNext('DoorOpen',2,CabNo); //zamiana
    CompressedVolume:=CompressedVolume-0.003;
   end
  else
   begin
    if (CabNo>0) then
     SendCtrlToNext('DoorClose',1,CabNo)
    else
     SendCtrlToNext('DoorClose',2,CabNo);
   end;
  end
  else
    DoorLeft:=false;
end;

function T_MoverParameters.DoorRight(State: Boolean):Boolean;
begin
 if (DoorRightOpened<>State) and (DoorBlockedFlag=false) and (Battery=true) then
  begin
  DoorRight:=true;
  DoorRightOpened:=State;
  if (State=true) then
   begin
    if (CabNo>0) then
     SendCtrlToNext('DoorOpen',2,CabNo) //1=lewe, 2=prawe
    else
     SendCtrlToNext('DoorOpen',1,CabNo); //zamiana
    CompressedVolume:=CompressedVolume-0.003;
   end
  else
   begin
    if (CabNo>0) then
     SendCtrlToNext('DoorClose',2,CabNo)
    else
     SendCtrlToNext('DoorClose',1,CabNo);
   end;
 end
 else
  DoorRight:=false;
end;

//Winger 160204 - pantografy
function T_MoverParameters.PantFront(State: Boolean):Boolean;
var pf1: Real;
begin
if (battery=true){ and ((TrainType<>dt_ET40)or ((TrainType=dt_ET40) and (EnginePowerSource.CollectorsNo>1)))}then
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
{Ra: nie ma potrzeby opuszczaæ obydwu na raz, jak mozemy ka¿dy osobno
      if (TrainType=dt_EZT) and (ActiveCab=1) then
       begin
        PantRearUp:=false;
        PantRearStart:=1;
        SendCtrlToNext('PantRear',0,CabNo);
       end;
}
   end;
 end
 else
 SendCtrlToNext('PantFront',pf1,CabNo);
end
end;

function T_MoverParameters.PantRear(State: Boolean):Boolean;
var pf1: Real;
begin
if battery=true then
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
 end
end;



{function T_MoverParameters.Heating(State: Boolean):Boolean;
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

function T_MoverParameters.LoadChkFile(chkpath:string):Boolean;
const param_ok=1; wheels_ok=2; dimensions_ok=4;
var
  lines,s: string;
  bl,i,k:integer;
  b,OKflag:byte;
  fin: text;
  OK: boolean;
procedure BrakeValveDecode(s:string);
begin
   if s='W' then
     BrakeValve:=W
   else if s='W_Lu_L' then
     BrakeValve:=W_Lu_L
   else if s='W_Lu_XR' then
     BrakeValve:=W_Lu_XR
   else if s='W_Lu_VI' then
     BrakeValve:=W_Lu_VI
   else if s='K' then
     BrakeValve:=W
   else if s='Kkg' then
     BrakeValve:=Kkg
   else if s='Kkp' then
     BrakeValve:=Kkp
   else if s='Kks' then
     BrakeValve:=Kks
   else if s='Hikp1' then
     BrakeValve:=Hikp1
   else if s='Hikss' then
     BrakeValve:=Hikss
   else if s='Hikg1' then
     BrakeValve:=Hikg1
   else if s='KE' then
     BrakeValve:=KE
   else if s='EStED' then
     BrakeValve:=EStED
   else if Pos('ESt',s)>0 then
     BrakeValve:=ESt3
   else if s='LSt' then
     BrakeValve:=LSt
   else if s='EP2' then
     BrakeValve:=EP2
   else if s='EP1' then
     BrakeValve:=EP1
   else if s='CV1' then
     BrakeValve:=CV1
   else if s='CV1_L_TR' then
     BrakeValve:=CV1_L_TR
   else
     BrakeValve:=Other;
end;
procedure BrakeSubsystemDecode;
begin
  case BrakeValve of
    W,W_Lu_L,W_Lu_VI,W_Lu_XR: BrakeSubsystem:=ss_W;
    ESt3,ESt3AL2,ESt4,EP2,EP1: BrakeSubsystem:=ss_ESt;                                                
    KE: BrakeSubsystem:=ss_KE;
    CV1, CV1_L_TR: BrakeSubsystem:=ss_Dako;
    LSt, EStED: BrakeSubsystem:=ss_LSt;
  else
    BrakeSubsystem:=ss_None;
  end;
end;
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
   else if s='DumbDE' then
    EngineDecode:=DieselElectric    //youBy: spal-ele
   else if s='ElectricInductionMotor' then
    EngineDecode:=ElectricInductionMotor
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
                       RAccumulator.MaxCapacity:=s2r(DUE(ExtractKeyWord(lines,prefix+'Cap=')));
                       s:=DUE(ExtractKeyWord(lines,prefix+'RS='));
                       RAccumulator.RechargeSource:=PowerSourceDecode(s);
                     end;
        CurrentCollector: begin
                            with CollectorParameters do
                             begin
                               CollectorsNo:=s2lE(DUE(ExtractKeyWord(lines,'CollectorsNo=')));
                               MinH:=s2rE(DUE(ExtractKeyWord(lines,'MinH=')));
                               MaxH:=s2rE(DUE(ExtractKeyWord(lines,'MaxH=')));
                               CSW:=s2rE(DUE(ExtractKeyWord(lines,'CSW='))); //szerokoœæ czêœci roboczej
                               MaxV:=s2rE(DUE(ExtractKeyWord(lines,'MaxVoltage=')));
                               s:=ExtractKeyWord(lines,'MinV='); //napiêcie roz³¹czaj¹ce WS
                               if s='' then
                                MinV:=0.5*MaxV //gdyby parametr nie podany
                               else
                                MinV:=s2rE(DUE(s));
                               s:=ExtractKeyWord(lines,'InsetV='); //napiêcie wymagane do za³¹czenia WS
                               if s='' then
                                InsetV:=0.6*MaxV //gdyby parametr nie podany
                               else
                                InsetV:=s2rE(DUE(s));
                               s:=ExtractKeyWord(lines,'MinPress='); //ciœnienie roz³¹czaj¹ce WS
                               if s='' then
                                MinPress:=2.0 //domyœlnie 2 bary do za³¹czenia WS
                               else
                                MinPress:=s2rE(DUE(s));
                               s:=ExtractKeyWord(lines,'MaxPress='); //maksymalne ciœnienie za reduktorem
                               if s='' then
                                MaxPress:=5.0+0.001*(random(50)-random(50))
                               else
                                MaxPress:=s2rE(DUE(s));
                             end;
                          end;
        PowerCable : begin
                       RPowerCable.PowerTrans:=PowerDecode(DUE(ExtractKeyWord(lines,prefix+'PowerTrans=')));
                       if RPowerCable.PowerTrans=SteamPower then
                        RPowerCable.SteamPressure:=s2r(DUE(ExtractKeyWord(lines,prefix+'SteamPress=')));
                     end;
        Heater : begin
                   {jeszcze nie skonczone!}
                 end;
       end;
       if (SourceType<>Heater) and (SourceType<>InternalSource) then
        if not ((SourceType=PowerCable) and (RPowerCable.PowerTrans=SteamPower)) then
        begin
{          PowerTrans:=ElectricPower; }
          MaxVoltage:=s2rE(DUE(ExtractKeyWord(lines,prefix+'MaxVoltage=')));
          MaxCurrent:=s2r(DUE(ExtractKeyWord(lines,prefix+'MaxCurrent=')));
          IntR:=s2r(DUE(ExtractKeyWord(lines,prefix+'IntR=')));
        end;
     end;
  end;
begin
  //OK:=true;
  OKflag:=0;
  LineCount:=0;
  ConversionError:=666;
  Mass:=0;
  filename:=chkpath+TypeName+'.fiz';//'.chk';
  assignfile(fin,filename);
{$I-}
  reset(fin);
{$I+}
  if IOresult<>0 then
   begin
     OK:=false;
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
              else if s='ET40' then TrainType:=dt_ET40
              else if s='EP05' then TrainType:=dt_EP05
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
              s:=ExtractKeyWord(lines,'Floor=');
              if s='' then
               begin
                if (H<=2.0) then
                 Floor:=H //gdyby nie by³o parametru, lepsze to ni¿ zero
                else
                 Floor:=0.0; //zgodnoœæ wsteczna
               end
              else
               Floor:=s2rE(DUE(s));
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
              BrakeValveParams:=DUE(ExtractKeyWord(lines,'BrakeValve='));
              BrakeValveDecode(BrakeValveParams);
              BrakeSubSystemDecode;
              s:=ExtractKeyWord(lines,'NBpA=');
              NBpA:=s2b(DUE(s));
              s:=ExtractKeyWord(lines,'MBF=');
              MaxBrakeForce:=s2r(DUE(s));
              s:=ExtractKeyWord(lines,'Size=');
              BrakeValveSize:=s2i(DUE(s));
              s:=ExtractKeyWord(lines,'TBF=');
              TrackBrakeForce:=s2r(DUE(s))*1000;
              s:=ExtractKeyWord(lines,'MaxBP=');
              MaxBrakePress[3]:=s2r(DUE(s));
              if MaxBrakePress[3]>0 then
               begin
                 s:=ExtractKeyWord(lines,'BCN=');
                 BrakeCylNo:=s2iE(DUE(s));
                 if BrakeCylNo>0 then
                  begin
                   s:=ExtractKeyWord(lines,'MaxLBP=');
                   MaxBrakePress[0]:=s2r(DUE(s));
                   if MaxBrakePress[0]<0.01 then
                   MaxBrakePress[0]:=MaxBrakePress[3];
                   s:=DUE(ExtractKeyWord(lines,'TareMaxBP='));
                   MaxBrakePress[1]:=s2r(s);
                   s:=DUE(ExtractKeyWord(lines,'MedMaxBP='));
                   MaxBrakePress[2]:=s2r(s);
                   s:=ExtractKeyWord(lines,'MaxASBP=');
                   MaxBrakePress[4]:=s2r(DUE(s));
                   if MaxBrakePress[4]<0.01 then
                   MaxBrakePress[4]:=0;
                   s:=ExtractKeyWord(lines,'BCR=');
                   BrakeCylRadius:=s2rE(DUE(s));
                   s:=ExtractKeyWord(lines,'BCD=');
                   BrakeCylDist:=s2r(DUE(s));
                   s:=ExtractKeyWord(lines,'BCS=');
                   BrakeCylSpring:=s2r(DUE(s));
                   s:=ExtractKeyWord(lines,'BSA=');
                   BrakeSlckAdj:=s2r(DUE(s));
                   s:=ExtractKeyWord(lines,'BRE=');
                   if s<>'' then
                     BrakeRigEff:=s2r(DUE(s))
                   else
                     BrakeRigEff:=1;
                   s:=ExtractKeyWord(lines,'BCM=');
                   BrakeCylMult[0]:=s2r(DUE(s));
                   s:=ExtractKeyWord(lines,'BCMlo=');
                   BrakeCylMult[1]:=s2r(DUE(s));
                   s:=ExtractKeyWord(lines,'BCMHi=');
                   BrakeCylMult[2]:=s2r(DUE(s));
                   P2FTrans:=100*Pi*SQR(BrakeCylRadius); {w kN/bar}
                   if (BrakeCylMult[1]>0) or (MaxBrakePress[1]>0) then LoadFlag:=1 else LoadFlag:=0;
                   BrakeVolume:=Pi*SQR(BrakeCylRadius)*BrakeCylDist*BrakeCylNo;
                   s:=ExtractKeyWord(lines,'BVV=');
                   BrakeVVolume:=s2R(DUE(s));

                   s:=DUE(ExtractKeyWord(lines,'BM='));
                   if s='P10-Bg'    then BrakeMethod:=bp_P10Bg else
                   if s='P10-Bgu'   then BrakeMethod:=bp_P10Bgu else
                   if s='FR513'     then BrakeMethod:=bp_FR513 else
                   if s='Cosid'     then BrakeMethod:=bp_Cosid else
                   if s='P10yBg'    then BrakeMethod:=bp_P10yBg else
                   if s='P10yBgu'   then BrakeMethod:=bp_P10yBgu else
                   if s='Disk1'     then BrakeMethod:=bp_D1 else
                   if s='Disk1+Mg'  then BrakeMethod:=bp_D1+bp_MHS else
                   if s='Disk2'     then BrakeMethod:=bp_D2 else                                                         
                                         BrakeMethod:=0;

                   s:=ExtractKeyWord(lines,'RM=');
                   if s<>'' then
                     RapidMult:=s2r(DUE(s))
                   else
                     RapidMult:=1;


                  end
                 else ConversionError:=-5;
               end
              else
               P2FTrans:=0;
              s:=ExtractKeyWord(lines,'HiPP=');
              if s<>'' then CntrlPipePress:=s2r(DUE(s))
               else CntrlPipePress:=5+0.001*(random(10)-random(10)); //Ra 2014-07: trochê niedok³adnoœci
              HighPipePress:=CntrlPipePress;
              s:=ExtractKeyWord(lines,'LoPP=');
              if s<>'' then LowPipePress:=s2r(DUE(s))
               else LowPipePress:=Min0R(HighPipePress,3.5);
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
              else if s='Engine' then
               CompressorPower:=3
              else if s='Coupler1' then
               CompressorPower:=4 //w³¹czana w silnikowym EZT z przodu
              else if s='Coupler2' then
               CompressorPower:=5 //w³¹czana w silnikowym EZT z ty³u
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
              if s='Shift' then DoorOpenMethod:=1 //przesuw
              else if s='Fold' then DoorOpenMethod:=3 //3 submodele siê obracaj¹
              else
               DoorOpenMethod:=2; //obrót
              s:=DUE(ExtractKeyWord(lines,'DoorClosureWarning='));
              if s='Yes' then
               DoorClosureWarning:=true
              else
               DoorClosureWarning:=false;
               s:=DUE(ExtractKeyWord(lines,'DoorBlocked='));
              if s='Yes' then DoorBlocked:=true
              else
                DoorBlocked:=false;
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
                if s<>'' then AllowedFlag:=s2i(DUE(s));
                if (AllowedFlag<0) then AllowedFlag:=(-AllowedFlag) OR ctrain_depot;
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
                   Couplers[1].AllowedFlag:=Couplers[0].AllowedFlag;
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
                if s<>'' then AllowedFlag:=s2i(DUE(s));
                if (AllowedFlag<0) then AllowedFlag:=(-AllowedFlag) OR ctrain_depot;
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
                 if s='GPR' then BrakeDelays:=bdelay_G+bdelay_P+bdelay_R
                 else if s='PR' then BrakeDelays:=bdelay_P+bdelay_R
                 else if s='GP' then BrakeDelays:=bdelay_G+bdelay_P
                 else if s='R' then begin BrakeDelays:=bdelay_R; BrakeDelayFlag:=bdelay_R; end
                 else if s='P' then begin BrakeDelays:=bdelay_P; BrakeDelayFlag:=bdelay_P; end
                 else if s='G' then begin BrakeDelays:=bdelay_G; BrakeDelayFlag:=bdelay_G; end
                 else if s='GPR+Mg' then BrakeDelays:=bdelay_G+bdelay_P+bdelay_R+bdelay_M
                 else if s='PR+Mg' then BrakeDelays:=bdelay_P+bdelay_R+bdelay_M;

{                 else if s='DPR+Mg' then begin Brakedelays:=bdelay_R; BrakeMethod:=3; end
                 else if s='DGPR+Mg' then begin Brakedelays:=bdelay_G+bdelay_R; BrakeMethod:=3; end
                 else if s='DGPR' then begin Brakedelays:=bdelay_G+bdelay_R; BrakeMethod:=1; end
                 else if s='DPR' then begin Brakedelays:=bdelay_R; BrakeMethod:=1; end
                 else if s='DGP' then begin Brakedelays:=bdelay_G; BrakeMethod:=1; end;}
//                  else if s='GPR' then Brakedelays=bdelay_G+bdelay_R

                 s:=DUE(ExtractKeyWord(lines,'BrakeHandle='));
                      if s='FV4a' then BrakeHandle:=FV4a
                 else if s='test' then BrakeHandle:=testH
                 else if s='D2' then BrakeHandle:=D2
                 else if s='M394' then BrakeHandle:=M394
                 else if s='Knorr' then BrakeHandle:=Knorr
                 else if s='Westinghouse' then BrakeHandle:=West
                 else if s='FVel6' then BrakeHandle:=FVel6
                 else if s='St113' then BrakeHandle:=St113;

                 s:=DUE(ExtractKeyWord(lines,'LocBrakeHandle='));
                      if s='FD1' then BrakeLocHandle:=FD1
                 else if s='Knorr' then BrakeLocHandle:=Knorr
                 else if s='Westinghouse' then BrakeLocHandle:=West;

                 s:=DUE(ExtractKeyWord(lines,'MaxBPMass='));
                 if s<>'' then
                   MBPM:=s2rE(s)*1000;


                 if BrakeCtrlPosNo>0 then
                  begin
                    s:=DUE(ExtractKeyWord(lines,'ASB='));
                    if s='Manual' then ASBType:=1 else
                     if s='Automatic' then ASBType:=2;
                  end
                 else
                  begin
                    s:=DUE(ExtractKeyWord(lines,'ASB='));
                    if s='Yes' then ASBType:=128;
                  end;
               end;
              s:=DUE(ExtractKeyWord(lines,'LocalBrake='));
              if s='ManualBrake' then LocalBrake:=ManualBrake
               else
                if s='PneumaticBrake' then LocalBrake:=PneumaticBrake
                  else
                    if s='HydraulicBrake' then LocalBrake:=HydraulicBrake
                      else LocalBrake:=NoBrake;
              s:=DUE(ExtractKeyWord(lines,'ManualBrake='));
              if s='Yes' then MBrake:=true
              else MBrake:=false;
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
               CoupledCtrl:=true    {wspolny wal}
              else CoupledCtrl:=false;
              s:=DUE(ExtractKeyWord(lines,'ScndS='));
              if s='Yes' then
               ScndS:=true    {brak pozycji rownoleglej przy niskiej nastawie PSR}
              else ScndS:=false;
              s:=ExtractKeyWord(lines,'IniCDelay=');
              InitialCtrlDelay:=s2r(DUE(s));
              s:=ExtractKeyWord(lines,'SCDelay=');
              CtrlDelay:=s2r(DUE(s));
              s:=ExtractKeyWord(lines,'SCDDelay=');
              if s<>'' then CtrlDownDelay:=s2r(DUE(s)) else CtrlDownDelay:=CtrlDelay; //hunter-101012: jesli nie ma SCDDelay;
              s:=DUE(ExtractKeyWord(lines,'FSCircuit=')); //hunter-111012: dla siodemek 303E
              if s='Yes' then
               FastSerialCircuit:=1
              else
               FastSerialCircuit:=0;

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
               end ;
               s:=ExtractKeyWord(lines,'Volt=');
                 if s<>'' then
                 NominalVoltage:=s2r(DUE(s));

             s:=ExtractKeyWord(lines,'LMaxVoltage=');
                 if s<>'' then
                 begin
                 BatteryVoltage:=s2r(DUE(s));
                 NominalBatteryVoltage:=s2r(DUE(s));
               end;
              //else LightPowerSource.SourceType:=NotDefined;
            end
          else if Pos('Security:',lines)>0 then
           begin
            //if (TrainType<>dt_EZT)or(Power>1) then //dla EZT tylko w silnikowym
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
               s:=ExtractKeyWord(lines,'AwareMinSpeed=');
               if s<>'' then
                AwareMinSpeed:=s2r(DUE(s))
               else
                AwareMinSpeed:=0.1*Vmax; //domyœlnie 10% Vmax
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
                 if (Power=0) then //jeœli nie ma mocy, np. rozrz¹dcze EZT
                  EnginePowerSource.SourceType:=NotDefined; //to silnik nie ma zasilania
               end
              else EnginePowerSource.SourceType:=NotDefined;
              //if EnginePowerSource.SourceType=NotDefined then
               begin
                 s:=DUE(ExtractKeyWord(lines,'SystemPower='));
                 if s<>'' then
                  begin
                    SystemPowerSource.SourceType:=PowerSourceDecode(s);
                    PowerParamDecode(lines,'',SystemPowerSource);
                  end
                 else SystemPowerSource.SourceType:=NotDefined;
               end
              //else SystemPowerSource.SourceType:=InternalSource;
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
                 s:=ExtractKeyWord(lines,'ShuntMode=');
                 if s<>'' then
                  begin //dodatkowa przek³adnia dla SM03 (2Ls150)
                   ShuntModeAllow:=true;
                   ShuntMode:=false;
                   AnPos:=s2rE(DUE(s)); //dodatkowe prze³o¿enie
                   if (AnPos<1.0) then //"rozruch wysoki" ma dawaæ wiêksz¹ si³ê
                    AnPos:=1.0/AnPos; //im wiêksza liczba, tym wolniej jedzie
                  end;
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
                   ShuntModeAllow:=true;
                   ShuntMode:=false;
                   AnPos:=0;
                   ImaxHi:=2;
                   ImaxLo:=1;
                 end;
               end;
             ElectricInductionMotor:
               begin
                 rventnmax:=1;
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
                 s:=ExtractKeyWord(lines,'dfic='); eimc[eimc_s_dfic]:=s2r(DUE(s));
                 s:=ExtractKeyWord(lines,'dfmax='); eimc[eimc_s_dfmax]:=s2r(DUE(s));
                 s:=ExtractKeyWord(lines,'p='); eimc[eimc_s_p]:=s2r(DUE(s));
                 s:=ExtractKeyWord(lines,'cfu='); eimc[eimc_s_cfu]:=s2r(DUE(s));
                 s:=ExtractKeyWord(lines,'cim='); eimc[eimc_s_cim]:=s2r(DUE(s));
                 s:=ExtractKeyWord(lines,'icif='); eimc[eimc_s_icif]:=s2r(DUE(s));
                 s:=ExtractKeyWord(lines,'Uzmax='); eimc[eimc_f_Uzmax]:=s2r(DUE(s));
                 s:=ExtractKeyWord(lines,'Uzh='); eimc[eimc_f_Uzh]:=s2r(DUE(s));
                 s:=ExtractKeyWord(lines,'DU='); eimc[eimc_f_DU]:=s2r(DUE(s));
                 s:=ExtractKeyWord(lines,'I0='); eimc[eimc_f_I0]:=s2r(DUE(s));
                 s:=ExtractKeyWord(lines,'fcfu='); eimc[eimc_f_cfu]:=s2r(DUE(s));
                 s:=ExtractKeyWord(lines,'F0='); eimc[eimc_p_F0]:=s2r(DUE(s));
                 s:=ExtractKeyWord(lines,'a1='); eimc[eimc_p_a1]:=s2r(DUE(s));
                 s:=ExtractKeyWord(lines,'Pmax='); eimc[eimc_p_Pmax]:=s2r(DUE(s));
                 s:=ExtractKeyWord(lines,'Fh='); eimc[eimc_p_Fh]:=s2r(DUE(s));
                 s:=ExtractKeyWord(lines,'Ph='); eimc[eimc_p_Ph]:=s2r(DUE(s));
                 s:=ExtractKeyWord(lines,'Vh0='); eimc[eimc_p_Vh0]:=s2r(DUE(s));
                 s:=ExtractKeyWord(lines,'Vh1='); eimc[eimc_p_Vh1]:=s2r(DUE(s));
                 s:=ExtractKeyWord(lines,'Imax='); eimc[eimc_p_Imax]:=s2r(DUE(s));

               end;

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
                   MotorParam[k].AutoSwitch:=false
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
                   MotorParam[k].AutoSwitch:=false
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
          //dwa dodatkowe parametry korekcyjne
          else if Pos('MotorParamTable0:',lines)>0 then
             begin
               for k:=0 to ScndCtrlPosNo do
                begin
                  with MotorParam[k] do
                  read(fin, bl, mfi,mIsat,mfi0, fi,Isat,fi0);
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
             RListSize:=s2b(DUE(ExtractKeyWord(lines,'Size=')));
             if RListSize>ResArraySize then
              ConversionError:=-4
             else
              for k:=0 to RListSize do
               begin
                 read(fin, RList[k].Relay, RList[k].R, RList[k].Bn, RList[k].Mn);
{                 if AutoRelayType=0 then
                  RList[k].AutoSwitch:=false
                 else begin  }  {to sie przyda do pozycji przejsciowych}
                 read(fin,i);
                 RList[k].AutoSwitch:=Boolean(i);
                 if(ScndInMain)then
                  begin
                   readln(fin,i);
                   RList[k].ScndAct:=i;
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
             RListSize:=s2b(DUE(ExtractKeyWord(lines,'Size=')));
             if RListSize>ResArraySize then
              ConversionError:=-4
             else
              for k:=0 to RListSize do
               begin
                 readln(fin, RList[k].Relay, RList[k].R, RList[k].Mn);
               end;
           end
//youBy
          else if (Pos('WWList:',lines)>0) then  {dla spal-ele}
          begin
            RListSize:=s2b(DUE(ExtractKeyWord(lines,'Size=')));
            for k:=0 to RListSize do
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
      OK:=true
     else
      OK:=false;
   end; {otwarty plik}
  if OK then
   if OKflag<>param_ok{+wheels_ok}+dimensions_ok then
    begin
      OK:=false;
      ConversionError:=-OKflag; {brakowalo jakiejs waznej linii z parametrami}
    end;
  if OK and (Mass<=0) then
   begin
     OK:=false;
     ConversionError:=-666;
   end;
  LoadChkFile:=OK;
  if ConversionError<>-8 then
   close(fin);
end; {loadchkfile}


END.



