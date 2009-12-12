unit mover;          {fizyka ruchu dla symulatora lokomotywy}

(*
(C) McZapkie v.2003.04a
Co brakuje:
3. Nie przetestowany hamulec elektrodynamiczny w roznych konfiguracjach
6. brak pantografow itp
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
25. poprawiona predkosc propagacji fali cisnienia (normalizacja przez kwadrat dlugosci)
26. uwzgledniona masa ladunku, przesuniecie w/m toru ruchu
27. lampy/sygnaly przednie/tylne
28. wprowadzona pozycja odciecia hamulca
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
   p_coupldmg = 1.0;
   p_derail=0.001;
   p_accn=0.1;   

   {typ sprzegu}
   ctrain_virtual=0;
   ctrain_coupler=1;       {sprzeg fizyczny}
   ctrain_pneumatic=2;     {przewody hamulcowe}
   ctrain_controll=4;      {przewody sterujace}
   ctrain_power=8;        {przewody zasilajace}
   ctrain_passenger=16;    {mostek przejsciowy}

   {typ hamulca elektrodynamicznego}
   dbrake_none=0;
   dbrake_passive=1;
   dbrake_switch=2;
   dbrake_reversal=4;
   dbrake_automatic=8;

   {status hamulca}
   b_off=0;
   b_on=1;
   b_dmg=2;
   b_release=4;
   b_antislip=8;

   {status czuwaka/SHP}
   s_waiting=1;
   s_aware=2;
   s_active=4;
   s_alarm=8;
   s_ebrake=16;

   {dzwieki}
   sound_none=0;
   sound_loud=1;
   sound_couplerstretch=2;
   sound_bufferclamp=4;
   sound_bufferbump=8;
   sound_relay=16;

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
                  Connected: PMoverParameters; {co jest podlaczone}
                  CForce: real;                {sila z jaka dzialal}
                  Dist: real;                  {strzalka ugiecia}
                  CheckCollision: boolean;     {czy sprawdzac sile czy pedy}
                end;

    TCouplers= array[0..1] of TCoupling;

    {typy hamulcow zespolonych}
    TBrakeSystem = (Individual, Pneumatic, ElectroPneumatic);
    {podtypy hamulcow zespolonych}
    TBrakeSubsystem = (Standard, WeLu, Knorr, Oerlikon);
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
    TEngineTypes = (None, Dumb, WheelsDriven, ElectricSeriesMotor, DieselEngine, SteamEngine);
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
               R: real;    {opornik rozruchowy}
               Bn,Mn:byte; {ilosc galezi i silnikow w galezi}
               AutoSwitch: boolean; {czy dana pozycja nastawniana jest recznie czy autom.}
             end;
    TSchemeTable = array[0..ResArraySize] of TScheme; {tablica rezystorow rozr.}
    TMotorParameters = record
                        mfi,mIsat:real; { aproksymacja M(I) silnika}
                        fi,Isat: real;  { aproksymacja E(n)=fi*n}
                        AutoSwitch: boolean;
                       end;

    TSecuritySystem= record
                       SystemType: byte; {0: brak, 1: czuwak aktywny, 2: SHP/sygnalizacja kabinowa}
                       AwareDelay,SoundSignalDelay,EmergencyBrakeDelay:real;
                       Status: byte;     {0: wylaczony, 1: wlaczony, 2: czuwak, 4: shp, 8: alarm, 16: hamowanie awaryjne}
                       SystemTimer, SystemSoundTimer, SystemBrakeTimer: real;
                       VelocityAllowed, NextVelocityAllowed: integer; {predkosc pokazywana przez sygnalizacje kabinowa}
                     end;

    {----------------------------------------------}
    {glowny obiekt}
    TMoverParameters = class(TObject)
               filename:string;
               {---opis lokomotywy, wagonu itp}
                    {--opis serii--}
               CategoryFlag: byte;       {1 - pociag, 2 - samochod, 4 - statek, 8 - samolot}
                    {--sekcja stalych typowych parametrow}
               TypeName: string;         {nazwa serii/typu}
               EngineType: TEngineTypes;               {typ napedu}
               EnginePowerSource: TPowerParameters;    {zrodlo mocy dla silnikow}
               SystemPowerSource: TPowerParameters;    {zrodlo mocy dla systemow sterowania/przetwornic/sprezarek}
               HeatingPowerSource: TPowerParameters;   {zrodlo mocy dla ogrzewania}
               AlterHeatPowerSource: TPowerParameters; {alternatywne zrodlo mocy dla ogrzewania}
               LightPowerSource: TPowerParameters;     {zrodlo mocy dla oswietlenia}
               AlterLightPowerSource: TPowerParameters;{alternatywne mocy dla oswietlenia}
               Vmax,Mass,Power: real;    {max. predkosc kontrukcyjna, masa wlasna, moc}
               HeatingPower, LightPower: real; {moc pobierana na ogrzewanie/oswietlenie}
               Dim: TDimension;          {wymiary}
               Cx: real;                 {wsp. op. aerodyn.}
               WheelDiameter : real;     {srednica kol napednych}
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
               LocalBrake: TLocalBrake;  {rodzaj hamulca indywidualnego}
               BrakePressureTable: TBrakePressureTable; {wyszczegolnienie cisnien w rurze}
               ASBType: byte;            {0: brak hamulca przeciwposlizgowego, 1: reczny, 2: automat}
               MaxBrakeForce: real;      {maksymalna sila nacisku hamulca}
               MaxBrakePress,P2FTrans: real;
               {max. cisnienie w cyl. ham., stala proporcjonalnosci p-K}
               HighPipePress,LowPipePress,DeltaPipePress: real;
               {max. i min. robocze cisnienie w przewodzie glownym oraz roznica miedzy nimi}
               BrakeVolume, VeselVolume: real;
               {pojemnosc powietrza w ukladzie hamulcowym, w ukladzie glownej sprezarki [m^3] }
               BrakeCylNo: integer; {ilosc cylindrow ham.}
               BrakeCylRadius, BrakeCylDist, BrakeCylMult: real;
               {promien cylindra, skok cylindra, przekladnia hamulcowa}
               MinCompressor,MaxCompressor,CompressorSpeed:real;
               {cisnienie wlaczania, zalaczania sprezarki, wydajnosc sprezarki}
               BrakeDelay: TBrakeDelayTable; {opoznienie hamowania/odhamowania t/o}
               BrakeCtrlPosNo:byte;     {ilosc pozycji hamulca}
               {nastawniki:}
               MainCtrlPosNo: byte;     {ilosc pozycji nastawnika}
               ScndCtrlPosNo: byte;
               SecuritySystem:TSecuritySystem;

                       {-sekcja parametrow dla lokomotywy elektrycznej}
               RList: TSchemeTable;     {lista rezystorow rozruchowych i polaczen silnikow}
               RlistSize: integer;
               MotorParam: array[0..MotorParametersArraySize] of TMotorParameters;
                                        {rozne parametry silnika przy bocznikowaniach}
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
               DynamicBrakeType: byte; {0 - brak, 1 - przelaczany, 2 - rewersyjny, 3 - automatyczny}
               RVentType: byte;        {0 - brak, 1 - jest, 2 - automatycznie wlaczany}
               RVentnmax: real;      {maks. obroty wentylatorow oporow rozruchowych}
               RVentCutOff: real;      {rezystancja wylaczania wentylatorow dla RVentType=2}

                {- dla uproszczonego modelu silnika (dumb) oraz dla drezyny}
                Ftmax:real;

                {-dla wagonow}
                MaxLoad: longint;           {masa w T lub ilosc w sztukach - ladownosc}
                LoadAccepted, LoadQuantity: string; {co moze byc zaladowane, jednostki miary}
                OverLoadFactor: real;       {ile razy moze byc przekroczona ladownosc}
                LoadSpeed,UnLoadSpeed: real;{szybkosc na- i rozladunku jednostki/s}

                        {--sekcja zmiennych}
                        {--opis konkretnego egzemplarza taboru}
                Loc: TLocation;
                Rot: TRotation;
                Name: string;                       {nazwa wlasna}
                Couplers: TCouplers;                {urzadzenia zderzno-sprzegowe, polaczenia miedzy wagonami}
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
                BrakePress:real;                    {!o cisnienie w cylindrach hamulcowych}
                PipePress: real;                    {!o cisnienie w przewodzie glownym}
                Volume: real;                       {objetosc spr. powietrza w zbiorniku hamulca}
                CompressedVolume:real;              {objetosc spr. powietrza w ukl. zasilania}
                Compressor: real;                   {! cisnienie w ukladzie zasilajacym}
                CompressorFlag:boolean;             {!o czy wlaczona sprezarka}
                BrakeCtrlPos:integer;               {nastawa hamulca zespolonego}
                LocalBrakePos:byte;                 {nastawa hamulca indywidualnego}
                BrakeStatus: byte; {0 - odham, 1 - ham., 2 - uszk., 4 - odluzniacz, 8 - antyposlizg}
                EmergencyBrakeFlag: boolean;        {hamowanie nagle}
                BrakeDelayFlag: byte;               {nastawa opoznienia ham. osob/towar 0/1}
                DynamicBrakeFlag: boolean;          {czy wlaczony hamulec elektrodymiczny}

                DamageFlag: byte;  {kombinacja bitowa stalych dtrain_* }

                EndSignalsFlag: byte;  {0 = brak, 1,2 - przod lub tyl osygnalizowane}
                HeadSignalsFlag: byte;  {0 = brak, 1szy bit: przod, 2gi bit: tyl, 3,4,5 bit: ktore swiatlo zapalone}
                CommandIn: TCommand;
                {komenda przekazywana przez PutCommand}
                {i wykonywana przez RunInternalCommand}
                CommandOut: string;        {komenda przekazywana przez ExternalCommand}
                ValueOut: real;            {argument komendy ktora ma byc przekazana na zewnatrz}

                RunningShape:TTrackShape;{geometria toru po ktorym jedzie pojazd}
                RunningTrack:TTrackParam;{parametry toru po ktorym jedzie pojazd}
                OffsetTrackH, OffsetTrackV: real;  {przesuniecie poz. i pion. w/m toru}

                {-zmienne dla lokomotyw}
                Mains: boolean;    {polozenie glownego wylacznika}
                MainCtrlPos: byte; {polozenie glownego nastawnika}
                ScndCtrlPos: byte; {polozenie dodatkowego nastawnika}
                ActiveDir: integer;{czy lok. jest wlaczona i w ktorym kierunku}
                {-1 - do tylu, +1 - do przodu, 0 - wylaczona}
                CabNo: integer;    {! numer kabiny: 1 lub -1. W przeciwnym razie brak sterowania}
                LastSwitchingTime: real; {czas ostatniego przelaczania czegos}
                WarningSignal: byte;     {0: nie trabi, 1,2: trabi)

                {-zmienne dla lokomotywy elektrycznej}
                RunningTraction:TTractionParam;{parametry sieci trakcyjnej najblizej lokomotywy}
                enrot, Im, Itot, Mm, Mw, Fw, Ft: real;
                {ilosc obrotow, prad silnika i calkowity, momenty, sily napedne}
                Imin,Imax: integer;      {prad przelaczania automatycznego rozruchu, prad bezpiecznika}
                Voltage: real;       {aktualne napiecie sieci zasilajacej}
                MainCtrlActualPos: byte; {wskaznik Rlist}
                ScndCtrlActualPos: byte; {wskaznik MotorParam}
                DelayCtrlFlag: boolean;  {opoznienie w zalaczaniu}
                LastRelayTime: real;     {czas ostatniego przelaczania stycznikow}
                AutoRelayFlag: boolean;  {mozna zmieniac jesli AutoRelayType=2}
                FuseFlag: boolean;       {!o bezpiecznik nadmiarowy}
                ResistorsFlag: boolean;  {!o jazda rezystorowa}
                RventRot: real;          {!s obroty wentylatorow rozruchowych}

                {-zmienne dla drezyny}
                PulseForce :real;        {przylozona sila}
                PulseForceTimer: real;
                PulseForceCount: integer;

                {dla drezyny, silnika spalinowego i parowego}
                eAngle: real;

                {-dla wagonow}
                Load: longint;      {masa w T lub ilosc w sztukach - zaladowane}
                LoadType: string;   {co jest zaladowane}
                LoadStatus: integer;{-1 - trwa rozladunek, 1 - trwa naladunek, 0 - gotowe}
                LastLoadChangeTime: real;

                {--funkcje}

{                function BrakeRatio: real;  }
                function LocalBrakeRatio: real;
                function PipeRatio: real;
                function DynamicBrakeSwitch(Switch:boolean):boolean;

                {! przesylanie komend sterujacych}
                function SendCtrlBroadcast(CtrlCommand:string; ctrlvalue:real):boolean;
                function SendCtrlToNext(CtrlCommand:string;ctrlvalue,dir:real):boolean;

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
               {! hamulec bezpieczenstwa}
                function EmergencyBrakeSwitch(Switch:boolean): boolean;
               {hamulec przeciwposlizgowy}
                function AntiSlippingBrake: boolean;
               {odluzniacz}
                function BrakeReleaser: boolean;

               {! reczny wlacznik urzadzen antyposlizgowych}
                function AntiSlippingButton: boolean;

               {funkcje dla ukladow pneumatycznych}
                function IncBrakePress(PressLimit,dp:real):boolean;
                function DecBrakePress(PressLimit,dp:real):boolean;
                function BrakeDelaySwitch(BDS:byte): boolean; {! przelaczanie nastawy opoznienia}
                {pomocnicze funkcje dla ukladow pneumatycznych}
                procedure UpdateBrakePressure(dt: real);
                procedure UpdatePipePressure(dt:real);
                procedure CompressorCheck(dt:real); {wlacza, wylacza kompresor, laduje zbiornik}

               {! funkcje laczace/rozlaczajace sprzegi}
               function Attach(ConnectNo: byte; ConnectTo:PMoverParameters; CouplingType: byte):boolean; {laczenie}
               function Dettach(ConnectNo: byte):boolean;                    {rozlaczanie}

               {funkcje obliczajace sily}
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
               {! zmiana kabiny i resetowanie ustawien}
                function ChangeCab(direction:integer): boolean;

                {-funkcje typowe dla lokomotywy elektrycznej}
                {! wlaczanie bezpiecznika nadmiarowego}
                function FuseOn: boolean;
                procedure FuseOff; {wywalanie bezpiecznika nadmiarowego}
                {!o pokazuje bezwgl. wartosc pradu na wybranym amperomierzu}
                function ShowCurrent(AmpN:byte): integer;
                {funkcje uzalezniajace sile pociagowa od predkosci: v2n, n2r, current, momentum}
                function v2n:real;
                function current(n,U:real): real;
                function Momentum(I:real): real;
               {! odlaczanie uszkodzonych silnikow}
                function CutOffEngine: boolean;
               {! przelacznik pradu wysokiego rozruchu}
                function MaxCurrentSwitch(State:boolean):boolean;
               {! przelacznik pradu automatycznego rozruchu}
                function MinCurrentSwitch(State:boolean):boolean;
               {! przelacznik automatycznego rozruchu}
                function AutoRelaySwitch(State:boolean):boolean;
               {symulacja automatycznego rozruchu}
                function AutoRelayCheck: boolean;

               {-funkcje dla wagonow}
                function LoadingDone(LSpeed: real; LoadInit:string):boolean;

                {! WAZNA FUNKCJA - oblicza sile wypadkowa}
                procedure ComputeTotalForce(dt: real);

                {! BARDZO WAZNA FUNKCJA - oblicza przesuniecie lokomotywy lub wagonu}
                function ComputeMovement(dt:real; Shape:TTrackShape; var Track:TTrackParam; var ElectricTraction:TTractionParam; NewLoc:TLocation; var NewRot:TRotation):real;
                {dt powinno byc 10ms lub mniejsze}
                {NewRot na wejsciu daje aktualna orientacje pojazdu, na wyjsciu daje dodatkowe wychylenia od tej orientacji}

                {dla samochodow}
                function ChangeOffsetH(DeltaOffset:real): boolean;

                {! procedury I/O}
                constructor Init(LocInitial:TLocation; RotInitial:TRotation;
                                 VelInitial:real; TypeNameInit, NameInit: string; LoadInitial:longint; LoadTypeInitial: string; Cab:integer);
                                                              {wywolac najpierw to}
                function LoadChkFile(chkpath:string):Boolean;   {potem zaladowac z pliku}
                function CheckLocomotiveParameters(ReadyFlag:boolean): boolean;  {a nastepnie sprawdzic}
                private
                function TotalMass:real;
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
   SandSpeed=0.2;              {ile kg/s}
   RVentSpeed=0.8;             {rozpedzanie sie wentylatora obr/s^2}
   RVentMinI=50.0;             {przy jakim pradzie sie wylaczaja}
   CouplerTune=1.0;            {skalowanie tlumiennosci}
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

function Distance(Loc1,Loc2: TLocation; Dim1,Dim2:TDimension) : real;
var Dist:real;
begin
  Dist:=SQRT(SQR(Loc2.x-Loc1.x)+SQR(Loc1.y-Loc2.y));
  Distance:=Dist-Dim2.L/2-Dim1.L/2;
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
function CouplerNext(CouplerN:byte):byte;
begin
  case CouplerN of
   0: CouplerNext:=1;
   1: CouplerNext:=0;
  end;
end;

function DirF(CouplerN:byte): real;
begin
  case CouplerN of
   0: DirF:=-1;
   1: DirF:=1
   else DirF:=0;
  end;
end;


{---------rozwiniecie deklaracji metod obiektu TMoverParameters--------}

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
 begin
   if LocalBrakePosNo>0 then
    LocalBrakeRatio:=LocalBrakePos/LocalBrakePosNo
   else
    LocalBrakeRatio:=0;
 end;

function TMoverParameters.PipeRatio: real;
var PR:real;
 begin
   if DeltaPipePress>0 then
    PR:=(HighPipePress-Max0R(LowPipePress,Min0R(HighPipePress,PipePress)))/DeltaPipePress
   else
    PR:=0;
   if TestFlag(BrakeStatus,b_antislip) then
    PR:=PR+0.4;
   PipeRatio:=PR;
 end;

{reczne przelaczanie hamulca elektrodynamicznego}
function TMoverParameters.DynamicBrakeSwitch(Switch:boolean):boolean;
var b:byte;
 begin
   if (DynamicBrakeType=dbrake_switch) and (MainCtrlActualPos=0) then
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
var OK:boolean;
begin
   OK:=(dir<>0) and Mains;
   if OK then
    with Couplers[(1+Sign(dir)) div 2] do
     if TestFlag(CouplingFlag,ctrain_controll) then
      if Connected^.SetInternalCommand(CtrlCommand,ctrlvalue,dir) then
       OK:=Connected^.RunInternalCommand and OK;
   SendCtrlToNext:=OK;
end;

procedure MainCtrlBroadcast(Couplers:TCouplers; MainCtrlPos,MainCtrlActualPos:byte);
var b:byte;
begin
{
  for b:=0 to 1 do
   with Couplers[b] do
    if TestFlag(CouplingFlag,ctrain_controll) then
     begin
       Connected^.MainCtrlPos:=MainCtrlPos;
       Connected^.MainCtrlActualPos:=MainCtrlActualPos;
     end;
poprawic to!!!}
end;

procedure ScndCtrlBroadcast(Couplers:TCouplers; ScndCtrlPos,ScndCtrlActualPos:byte);
var b:byte;
begin
{
  for b:=0 to 1 do
   with Couplers[b] do
    if TestFlag(CouplingFlag,ctrain_controll) then
     begin
       Connected^.ScndCtrlPos:=ScndCtrlPos;
       Connected^.ScndCtrlActualPos:=ScndCtrlActualPos;
     end;
 }
end;


{funkcje zwiekszajace/zmniejszajace nastawniki}

function TMoverParameters.IncMainCtrl(CtrlSpeed:integer): boolean;
var b:byte; OK:boolean;
begin
  OK:=false;
  if (MainCtrlPosNo>0) and (CabNo<>0) then
    begin
      if MainCtrlPos<MainCtrlPosNo then
       case EngineType of
        None, Dumb:
         if CtrlSpeed=1 then
          begin
            inc(MainCtrlPos);
            OK:=True;
          end
         else
          if CtrlSpeed>1 then
            OK:=IncMainCtrl(1) and IncMainCtrl(CtrlSpeed-1);
        ElectricSeriesMotor:
         if CtrlSpeed=1 then
          begin
            inc(MainCtrlPos);
             if Imax=ImaxHi then
              if RList[MainCtrlPos].Bn>1 then
               if MaxCurrentSwitch(false) then
                 SetFlag(SoundFlag,sound_relay); {wylaczanie wysokiego rozruchu}
{            if (EngineType=ElectricSeriesMotor) and (MainCtrlPos=1) then
             MainCtrlActualPos:=1;
}
            OK:=True;
          end
          else
          if CtrlSpeed>1 then
           begin
             while (RList[MainCtrlPos].R>0) and IncMainCtrl(1) do
              OK:=True ; {takie chamskie, potem poprawie}
           end;
        WheelsDriven:
         OK:=AddPulseForce(CtrlSpeed);
       end {case EngineType}
      else
       if CoupledCtrl then {wspolny wal}
         begin
           if ScndCtrlPos<ScndCtrlPosNo then
             begin
               inc(ScndCtrlPos);
               OK:=True;
             end
            else OK:=False;
         end;
      if OK then
       begin
          {OK:=}SendCtrlToNext('MainCtrl',MainCtrlPos,CabNo);  {???}
          {OK:=}SendCtrlToNext('ScndCtrl',ScndCtrlPos,CabNo)
       end;
    end
  else {nie ma sterowania}
   OK:=False;
  if OK then LastRelayTime:=0;
  IncMainCtrl:=OK;
end;


function TMoverParameters.DecMainCtrl(CtrlSpeed:integer): boolean;
var b:byte; OK:boolean;
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
            None, Dumb:
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
          {OK:=}SendCtrlToNext('ScndCtrl',ScndCtrlPos,CabNo)
       end;
   end
  else
   OK:=False;
  if OK then LastRelayTime:=0;
  DecMainCtrl:=OK;
end;

function TMoverParameters.IncScndCtrl(CtrlSpeed:integer): boolean;
var b:byte; OK:boolean;
begin
 if (ScndCtrlPosNo>0) and (CabNo<>0) then
  begin
{      if (RList[MainCtrlPos].R=0) and (MainCtrlPos>0) and (ScndCtrlPos<ScndCtrlPosNo) and (not CoupledCtrl) then}
   if (ScndCtrlPos<ScndCtrlPosNo) and (not CoupledCtrl) then
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
      {OK:=}SendCtrlToNext('ScndCtrl',ScndCtrlPos,CabNo)
    end;
  end
 else {nie ma sterowania}
  OK:=False;
 if OK then LastRelayTime:=0;
 IncScndCtrl:=OK;
end;

function TMoverParameters.DecScndCtrl(CtrlSpeed:integer): boolean;
var b:byte; OK:boolean;
begin
  if (ScndCtrlPosNo>0) and (CabNo<>0) then
   begin
    if (ScndCtrlPos>0) and (not CoupledCtrl) then
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
      {OK:=}SendCtrlToNext('ScndCtrl',ScndCtrlPos,CabNo)
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
     DirectionForward:=True;
     SendCtrlToNext('Direction',ActiveDir,CabNo);
   end
  else
   DirectionForward:=False;
end;

function TMoverParameters.DirectionBackward: boolean;
begin
  if (MainCtrlPosNo>0) and (ActiveDir>-1) and (MainCtrlPos=0) then
   begin
     if EngineType=WheelsDriven then
      dec(CabNo);
{     else}
      dec(ActiveDir);
     DirectionBackward:=True;
     SendCtrltoNext('Direction',ActiveDir,CabNo);
   end
  else
   DirectionBackward:=False;
end;

function TMoverParameters.MainSwitch(State:boolean): boolean;
begin
  if ((Mains<>State) and (MainCtrlPosNo>0)) then
   begin
    if (State=False) or ((MainCtrlPos=0) and (ScndCtrlPos=0) and (LastSwitchingTime>CtrlDelay) and not TestFlag(DamageFlag,dtrain_out)) then
     begin
       if Mains then
        SendCtrlToNext('MainSwitch',ord(State),CabNo);
       Mains:=State;
       if Mains then
        SendCtrlToNext('MainSwitch',ord(State),CabNo); {wyslanie po wlaczeniu albo przed wylaczeniem}
       MainSwitch:=True;
       LastSwitchingTime:=0;
       if (State=False) then
        begin
          SetFlag(SoundFlag,sound_relay);
          SecuritySystem.Status:=0;
        end
       else
        SecuritySystem.Status:=s_waiting;
     end
   end
  else
   MainSwitch:=False;
end;

function TMoverParameters.ChangeCab(direction:integer): boolean;
var b:byte;
begin
  if Abs(CabNo+direction)<2 then
   begin
     CabNo:=CabNo+direction;
     ChangeCab:=True;
     if (BrakeSystem=Pneumatic) and (BrakeCtrlPosNo>0) then
      BrakeCtrlPos:=-2
     else
      BrakeCtrlPos:=0;
     if not TestFlag(BrakeStatus,b_dmg) then
      BrakeStatus:=b_off;
     MainCtrlPos:=0;
     ScndCtrlPos:=0;
     Mains:=False;
     ActiveDir:=0;
   end
  else
   ChangeCab:=False;
end;


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

{reset czuwaka/SHP}
function TMoverParameters.SecuritySystemReset : boolean;
 procedure Reset;
  begin
    SecuritySystem.SystemTimer:=0;
    SecuritySystem.SystemBrakeTimer:=0;
    SecuritySystem.SystemSoundTimer:=0;
    SecuritySystem.Status:=s_waiting;
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
end;

{testowanie czuwaka/SHP}
procedure TMoverParameters.SecuritySystemCheck(dt:real);
begin
  with SecuritySystem do
   begin
     if (SystemType>0) and (Status>0) then
      begin
        SystemTimer:=SystemTimer+dt;
        if TestFlag(Status,s_aware) or TestFlag(Status,s_active) then
         SystemSoundTimer:=SystemSoundTimer+dt;
        if TestFlag(Status,s_alarm) then
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
var b:byte;
begin
  if (BrakeCtrlPosNo>0) {and (LocalBrakePos=0)} then
   begin
     if BrakeCtrlPos<BrakeCtrlPosNo then
      begin
        inc(BrakeCtrlPos);
        if (BrakePressureTable[BrakeCtrlPos].BrakeType=ElectroPneumatic) then
         begin
           BrakeStatus:=ord(BrakeCtrlPos>0);
           SendCtrlToNext('BrakeCtrl',BrakeCtrlPos,CabNo);
         end
        else
         if not TestFlag(BrakeStatus,b_dmg) then
          BrakeStatus:=b_on;
        IncBrakeLevel:=True;
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
var b:byte;
begin
  if (BrakeCtrlPosNo>0) {and (LocalBrakePos=0)} then
   begin
     if (BrakeCtrlPos>0) or ((BrakeSubsystem=Oerlikon) and (BrakeCtrlPos>-2)) then
      begin
        dec(BrakeCtrlPos);
        if EmergencyBrakeFlag then
          begin
             EmergencyBrakeFlag:=false; {!!!}
             SendCtrlToNext('Emergency_brake',0,CabNo);
          end;
        if BrakePressureTable[BrakeCtrlPos].BrakeType=ElectroPneumatic then
         begin
           BrakeStatus:=ord(BrakeCtrlPos>0);
           SendCtrlToNext('BrakeCtrl',BrakeCtrlPos,CabNo);
         end
        else
         if (not TestFlag(BrakeStatus,b_dmg) and (not TestFlag(BrakeStatus,b_release))) then
          BrakeStatus:=b_off;   {luzowanie jesli dziala oraz nie byl wlaczony odluzniacz}
        DecBrakeLevel:=True;
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
  if ASBType=1 then
   begin
     if SetFlag(BrakeStatus,b_antislip) then
       AntiSlippingBrake:=True;
     BrakeSlippingTimer:=0;
   end
  else                          
   AntiSlippingBrake:=False;
end;

function TMoverParameters.AntiSlippingButton: boolean;
var OK:boolean;
begin
  OK:=SandDoseOn;
  AntiSlippingButton:=(AntiSlippingBrake or OK);
end;

function TMoverParameters.BrakeDelaySwitch(BDS:byte): boolean;
begin
  if BrakeCtrlPosNo>0 then
   if BrakeDelayFlag<>BDS then
    begin
      BrakeDelayFlag:=BDS;
      BrakeDelaySwitch:=True;
    end
  else
   BrakeDelaySwitch:=False;
end;

function TMoverParameters.BrakeReleaser: boolean;
begin
  if (BrakeCtrlPosNo>-1) and (not TestFlag(BrakeStatus,b_dmg)) then
   begin
     BrakeReleaser:=SetFlag(BrakeStatus,b_release);
   end
  else
   BrakeReleaser:=False;
end;

{uklady pneumatyczne}

function TMoverParameters.IncBrakePress(PressLimit,dp:real):boolean;
begin
    if BrakePress+dp<PressLimit then
     begin
       BrakePress:=BrakePress+dp;
       IncBrakePress:=True;
     end
    else
     begin
       IncBrakePress:=False;
       BrakePress:=PressLimit;
     end;
    if (DynamicBrakeType<>dbrake_switch) and ((BrakePress>0.2) or (PipePress<0.36)) then
     DynamicBrakeFlag:=True;                 {uruchamianie hamulca ED albo odlaczanie silnikow}
end;

function TMoverParameters.DecBrakePress(PressLimit,dp:real):boolean;
begin
    if BrakePress-dp>PressLimit then
     begin
       BrakePress:=BrakePress-dp;
       DecBrakePress:=True;
     end
    else
     begin
       DecBrakePress:=False;
       BrakePress:=PressLimit;
     end;
    if (DynamicBrakeType<>dbrake_switch) and ((BrakePress<0.16) and (PipePress>0.36)) then
     DynamicBrakeFlag:=False;                {wylacanie hamulca ED albo zalaczanie silnikow}
end;

procedure TMoverParameters.UpdateBrakePressure(dt:real);
const LBDelay=5.0; {stala czasowa hamulca}
var Rate,dp:real;
begin
  dpLocalValve:=0;
  dpBrake:=0;
  if (MaxBrakePress>0) and (not TestFlag(BrakeStatus,b_dmg)) then
   begin
{    if EmergencyBrakeFlag then
     BrakePress:=MaxBrakePress
    else
     begin
}
       {pneumatyczny hamulec pomocniczy}
{       Rate:=LocalBrakeRatio;
       if (LocalBrake=PneumaticBrake) and (Compressor>MaxBrakePress) and (BrakeStatus=0) then
        begin
          if Abs(Rate*MaxBrakePress-BrakePress)>0.75*MaxBrakePress then
           dpLocalValve:=MaxBrakePress*dt/LBDelay
          else
           dpLocalValve:=(Abs(Rate*MaxBrakePress-BrakePress)+0.1)*dt/LBDelay;
          if BrakePress<Rate*MaxBrakePress then
           if IncBrakePress(MaxBrakePress*Rate,dpLocalValve) then
            CompressedVolume:=CompressedVolume-10*dpLocalValve*BrakeVolume;
          if BrakePress>Rate*MaxBrakePress then
           if not DecBrakePress(MaxBrakePress*Rate,dpLocalValve) then
            BrakePress:=MaxBrakePress*Rate;
        end;
}

       {hydrauliczny hamulec pomocniczy}
       if (LocalBrake=HydraulicBrake) and (BrakeStatus=b_off) then
        begin
          BrakePress:=MaxBrakePress*LocalBrakeRatio;
        end;

       {elektropneumatyczny hamulec zasadniczy}
       if (BrakePressureTable[BrakeCtrlPos].BrakeType=ElectroPneumatic) and Mains then
         with BrakePressureTable[BrakeCtrlPos] do
          if BrakePressureVal<>-1 then
           begin
             dp:=Min0R(Compressor,BrakePressureVal);
             dpBrake:=FlowSpeedVal*(dp-BrakePress)*dt/(1+BrakeDelay[1+ord(BrakeStatus and b_on)+2*BrakeDelayFlag]);
             if (dpBrake>0) then
              CompressedVolume:=CompressedVolume-10*dpMainValve*BrakeVolume;
             BrakePress:=BrakePress+dpBrake;
           end; {sterowanie cisnieniem}

       {pneumatyczny hamulec zasadniczy}
       if ((BrakeCtrlPosNo>0) and (BrakePressureTable[BrakeCtrlPos].BrakeType=Pneumatic)) or
          (BrakeSystem=Pneumatic) then
        begin
          if TestFlag(BrakeStatus,b_release) then
           Rate:=LocalBrakeRatio {odluzniacz nie dziala na hamulec pomocniczy}
          else
           Rate:=Max0R(PipeRatio,LocalBrakeRatio);
          if ((Rate*MaxBrakePress>BrakePress) and (Volume/(10*BrakeVolume+0.1)>MaxBrakePress/2)) then
            begin
              dpBrake:=Rate*MaxBrakePress*dt/LBDelay;
              IncBrakePress(Rate*MaxBrakePress,dpBrake)
            end;
          if BrakePress>Rate*MaxBrakePress then
            begin
              dpBrake:=(1-Rate)*MaxBrakePress*dt/LBDelay;
              DecBrakePress(Rate*MaxBrakePress,dpBrake)
            end;
        end;
     {
     end;
     }
   end; {cylindry hamulcowe}
end;  {updatebrakepressure}


procedure TMoverParameters.UpdatePipePressure(dt:real);
var b: byte; dp,PWSpeed:real;
function BrakeVP:real;
begin
  if BrakeVolume>0 then
   BrakeVP:=Volume/(10*BrakeVolume)
  else BrakeVP:=0;
end;
begin
  PWSpeed:=800/sqr(Dim.L);
  dpMainValve:=0;
  if HighPipePress>0 then
   begin
{
     if EmergencyBrakeFlag then
      PipePress:=0
     else
}
      if (BrakeCtrlPosNo>0) then
       with BrakePressureTable[BrakeCtrlPos] do
        if PipePressureVal<>-1 then
         begin
           dp:=Min0R(Compressor,PipePressureVal);
           dpMainValve:=FlowSpeedVal*(dp-PipePress)*dt/(1+BrakeDelay[1+ord(BrakeStatus and b_on)+2*BrakeDelayFlag]);
           if (dpMainValve>0) then
            CompressedVolume:=CompressedVolume-10*dpMainValve*BrakeVolume;
           PipePress:=PipePress+dpMainValve;
         end; {sterowanie cisnieniem}
     if EmergencyBrakeFlag then         {ulepszony hamulec bezp.}
      begin
        dpMainValve:=-10*PipePress*dt;
        PipePress:=PipePress+dpMainValve;
        if PipePress<0 then PipePress:=0;
      end;
     dpPipe:=0; {fala hamowania}
     dp:=0; {napelnianie zbiornikow pomocniczych hamulca}
     for b:=0 to 1 do
      begin
      if TestFlag(Couplers[b].CouplingFlag,ctrain_pneumatic) then
       if (Couplers[b].Connected^.PipePress>=LowPipePress) or (BrakeCtrlPosNo=0) then
         dpPipe:=dpPipe+(Couplers[b].Connected^.PipePress-PipePress)*dt*PWSpeed;
      end;
     if BrakeVP<PipePress then
      begin
        dp:=(PipePress-BrakeVP)*dt;
        if dp>0 then Volume:=Volume+dp*10*BrakeVolume
        else dp:=0;
      end;
     PipePress:=PipePress+dpPipe-dp;
     if PipePress<0 then PipePress:=0;
{        if PipePress>NominalPipePress then PipePress:=NominalPipePress; } {pomyslec nad tym}
   end; {PipePress}
  if CompressedVolume<0 then
   CompressedVolume:=0;
end;  {updatepipepressure}


procedure TMoverParameters.CompressorCheck(dt:real);
 begin
   if VeselVolume>0 then
    begin
     if (not CompressorFlag) and (Compressor<MinCompressor) and Mains and (LastSwitchingTime>CtrlDelay) then
      begin
        CompressorFlag:=True;
        LastSwitchingTime:=0;
      end;
     if (Compressor>MaxCompressor) or not Mains then
      CompressorFlag:=False;
     if CompressorFlag then
      CompressedVolume:=CompressedVolume+dt*CompressorSpeed*(2*MaxCompressor-Compressor)/MaxCompressor;
    end;
 end;


{sprzegi}

function TMoverParameters.Attach(ConnectNo: byte; ConnectTo:PMoverParameters; CouplingType: byte): boolean; {laczenie}
const dEpsilon=0.001;
var ct:TCouplerType;
begin
 with Couplers[ConnectNo] do
  begin
   if (ConnectTo<>nil) then
    begin
      ct:=ConnectTo^.Couplers[CouplerNext(ConnectNo)].CouplerType;
      if (((Distance(Loc,ConnectTo^.Loc,Dim,ConnectTo^.Dim)<=dEpsilon) and (CouplerType<>NoCoupler) and (CouplerType=ct))
         or (CouplingType and ctrain_coupler=0))
       then
        begin  {stykaja sie zderzaki i kompatybilne typy sprzegow chyba ze wirtualnie}
          Connected:=ConnectTo;
          CouplingFlag:=CouplingType;
          Connected.Couplers[CouplerNext(ConnectNo)].CouplingFlag:=CouplingType;
          Attach:=True;
        end
       else
        Attach:=False;
    end
     else
      Attach:=False;
  end;
end;

function TMoverParameters.Dettach(ConnectNo: byte): boolean; {laczenie}
begin
 with Couplers[ConnectNo] do
  begin
   if (Connected<>nil) and
   (((Distance(Loc,Connected^.Loc,Dim,Connected^.Dim)<=0) and (CouplerType<>Articulated)) or
    (TestFlag(DamageFlag,dtrain_coupling) or (CouplingFlag and ctrain_coupler=0)))
    then
     begin  {gdy podlaczony oraz scisniete zderzaki chyba ze zerwany sprzeg albo tylko wirtualnie}
{       Connected:=nil;  } {lepiej zostawic bo przeciez trzeba kontrolowac zderzenia odczepionych}
       if CouplingFlag>0 then
        SoundFlag:=sound_couplerstretch; {dzwiek rozlaczania}
       CouplingFlag:=0;
       Connected.Couplers[CouplerNext(ConnectNo)].CouplingFlag:=0;
       Dettach:=True;
     end
   else
    Dettach:=False;
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
     eps:=WForce*WheelDiameter/(2*AxleInertialMoment);
     newn:=n+eps*dt;
     if (newn*n<=0) and (eps*n<0) then
      newn:=0;
   end;
  ComputeRotatingWheel:=newn;
end;

{----------------------}
{LOKOMOTYWA ELEKTRYCZNA}

function TMoverParameters.FuseOn: boolean;
begin
  if (EngineType=ElectricSeriesMotor) and FuseFlag and
     (MainCtrlPos=0) and (ScndCtrlPos=0) and Mains then
   begin
     FuseFlag:=False;  {wlaczenie ponowne obwodu}
     FuseOn:=True;
     SetFlag(SoundFlag,sound_relay); SetFlag(SoundFlag,sound_loud);
     SendCtrlToNext('FuseSwitch',1,CabNo);
   end
  else
   FuseOn:=False; {=     SendCtrlToNext('FuseSwitch',1,CabNo); ??? }
end;

procedure TMoverParameters.FuseOff;
begin
  if not FuseFlag then
   begin
     FuseFlag:=True; EventFlag:=True;
     SetFlag(SoundFlag,sound_relay); SetFlag(SoundFlag,sound_loud);
   end;
end;


function TMoverParameters.ShowCurrent(AmpN:byte): integer;
var b:byte;
begin
  ShowCurrent:=0;
  if Power>0.01 then
   begin
     if AmpN>0 then
      begin
       if RList[MainCtrlActualPos].Bn>=AmpN then
        ShowCurrent:=Trunc(Abs(Im));
      end
     else
       ShowCurrent:=Trunc(Abs(Itot));
    end
  else
   for b:=0 to 1 do
    with Couplers[b] do
     if TestFlag(CouplingFlag,ctrain_controll) then
      if Connected^.Power>0.01 then
       ShowCurrent:=Connected^.ShowCurrent(AmpN);
end;


{funkcje uzalezniajace sile pociagowa od predkosci: V2n, n2R, Current, Momentum}
{----------------}

function TMoverParameters.V2n:real;
{przelicza predkosc liniowa na obrotowa}
var n:real;
begin
  n:=V/(Pi*WheelDiameter);
  if SlippingWheels then
   if Abs(n-nrot)<0.1 then
    SlippingWheels:=False; {wygaszenie poslizgu}
  if SlippingWheels then   {nie ma zwiazku z predkoscia liniowa V}
   n:=nrot;
  V2n:=n;
end;

function TMoverParameters.Current(n,U:real): real;
{wazna funkcja - liczy prad plynacy przez silniki polaczone szeregowo lub rownolegle}
{w zaleznosci od polozenia nastawnikow MainCtrl i ScndCtrl oraz predkosci obrotowej n}
{a takze wywala bezpiecznik nadmiarowy gdy za duzy prad lub za male napiecie}
{jest takze mozliwosc uszkodzenia silnika wskutek nietypowych parametrow}
var Rw,R,MotorCurrent:real;
    Rz,Delta,Isf:real;
    Mn: integer;
begin
  ResistorsFlag:=(RList[MainCtrlActualPos].R>0.01) and (not DelayCtrlFlag);
  R:=RList[MainCtrlActualPos].R+CircuitRes;
  Mn:=RList[MainCtrlActualPos].Mn;
  if (RList[MainCtrlActualPos].Bn=0) or FuseFlag or DelayCtrlFlag then
    MotorCurrent:=0                    {wylaczone}
   else                           {wlaczone}
    with MotorParam[ScndCtrlActualPos] do
     begin
      Rz:=Mn*WindingRes+R;
      if DynamicBrakeFlag then     {hamowanie}
       begin
         if DynamicBrakeType>0 then
           MotorCurrent:=-fi*n/Rz  {hamowanie silnikiem}
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
           MotorCurrent:=(U-Isf*Rz-Mn*fi*n+SQRT(Delta))/(2*Rz)
          else
           MotorCurrent:=(U-Isf*Rz-Mn*fi*n-SQRT(Delta))/(2*Rz)
         end
        else
         MotorCurrent:=0;
       end;
     end;
{  if Abs(CabNo)<2 then Im:=MotorCurrent*ActiveDir*CabNo
   else Im:=0;
}
  Im:=MotorCurrent;
  Current:=Im; {prad brany do liczenia sily trakcyjnej}
{  EnginePower:=Im*Im*RList[MainCtrlActualPos].Bn*RList[MainCtrlActualPos].Mn*WindingRes;}
  EnginePower:=Abs(Itot)*RList[MainCtrlActualPos].Mn*Abs(U);

  {awarie}
  MotorCurrent:=Abs(Im); {zmienna pomocnicza}
  if Motorcurrent>0 then
  begin
    if FuzzyLogic(Abs(n),nmax*1.1,p_elengproblem) then
     if MainSwitch(False) then
       EventFlag:=True;                                      {zbyt duze obroty - wywalanie wskutek ognia okreznego}
    if TestFlag(DamageFlag,dtrain_engine) then
     if FuzzyLogic(MotorCurrent,ImaxLo/10,p_elengproblem) then
      if MainSwitch(False) then
       EventFlag:=True;                                      {uszkodzony silnik (uplywy)}
    if (FuzzyLogic(Abs(Im),Imax*2,p_elengproblem) or FuzzyLogic(Abs(n),nmax*1.11,p_elengproblem)
       or FuzzyLogic(Abs(U/Mn),2*NominalVoltage,1)) then
     if (SetFlag(DamageFlag,dtrain_engine)) then
       EventFlag:=True;
  {! dorobic grzanie oporow rozruchowych i silnika}
  end;
end;

function TMoverParameters.Momentum(I:real): real;
{liczy moment sily wytwarzany przez silnik}
begin
 with MotorParam[ScndCtrlActualPos] do
  Momentum:=mfi*I*(1-1/(Abs(I)/mIsat+1));
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
  if (EngineType=ElectricSeriesMotor) and (ImaxHi>ImaxLo) then
   begin
     if State and (Imax=ImaxLo) then
      begin
        Imax:=ImaxHi;
        MaxCurrentSwitch:=True;
        if CabNo<>0 then
         SendCtrlToNext('MaxCurrentSwitch',1,CabNo);
      end;
     if (not State) and (Imax=ImaxHi) then
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
  if (EngineType=ElectricSeriesMotor) and (IminHi>IminLo) then
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
var OK:boolean; b:byte;
begin
  if (not Mains) or (FuseFlag) then
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
       if (((RList[MainCtrlActualPos].R=0) and not CoupledCtrl) or (MainCtrlActualPos=RListSize))
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
        begin            {zmieniaj mainctrlactualpos}
          if (not AutoRelayFlag) or (not RList[MainCtrlActualPos].AutoSwitch) then
           begin                                                {main bez samoczynnego rozruchu}
             OK:=True;
             if Rlist[MainCtrlActualPos].Relay<MainCtrlPos then
              inc(MainCtrlActualPos)
             else if Rlist[MainCtrlActualPos].Relay>MainCtrlPos then
              dec(MainCtrlActualPos)
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


{przyczepnosc}

function TMoverParameters.Adhesive(staticfriction:real): real;
var Adhesion: real;
begin
  Adhesion:=0;
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
  Adhesive:=Adhesion;
end;


{SILY}

function TMoverParameters.TractionForce(dt:real):real;
var PosRatio: real;
{oblicza sile trakcyjna lokomotywy (dla elektrowozu tez calkowity prad)}
begin
  Ft:=0;
  enrot:=Transmision.Ratio*nrot;
  eAngle:=eAngle+enrot*dt;
  if eAngle>Pirazy2 then
   eAngle:=Pirazy2-eAngle;
  if ActiveDir<>0 then
   case EngineType of
    Dumb:
     begin
       PosRatio:=(MainCtrlPos+ScndCtrlPos)/(MainCtrlPosNo+ScndCtrlPosNo+0.01);
       if Mains and (ActiveDir<>0) and (CabNo<>0) then
        begin
          if Vel>0.1 then
           begin
             Ft:=Min0R(1000*Power/Abs(V),Ftmax)*PosRatio;
           end
          else Ft:=Ftmax*PosRatio;
          Ft:=Ft*ActiveDir*CabNo;
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
      end;
    ElectricSeriesMotor:
      begin
{        enrot:=Transmision.Ratio*nrot; }
        Mm:=Momentum(Current(enrot,Voltage)); {oblicza tez prad p/slinik}
        if (Abs(Im)>Imax) then
         FuseOff;                     {wywalanie bezpiecznika z powodu przetezenia silnikow}
        if (Abs(Voltage)<EnginePowerSource.MaxVoltage/2) or (Abs(Voltage)>EnginePowerSource.MaxVoltage*2) then
         if MainSwitch(False) then
          EventFlag:=True;            {wywalanie szybkiego z powodu niewlasciwego napiecia}

        Itot:=Im*RList[MainCtrlActualPos].Bn;   {prad silnika * ilosc galezi}
        Mw:=Mm*Transmision.Ratio;
        Fw:=Mw*2/WheelDiameter;
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
   None: begin end;
   end; {case}
  TractionForce:=Ft
end;

function TMoverParameters.BrakeForce(Track:TTrackParam):real;
var u,K,Fb,NBrakeAxles:real;
const OerlikonForceFactor=1.5;
begin
  if NPoweredAxles>0 then
   NBrakeAxles:=NPoweredAxles
  else
   NBrakeAxles:=NAxles;
  case LocalBrake of
   NoBrake :      K:=0;
   ManualBrake :  K:=MaxBrakeForce*LocalBrakeRatio;
   HydraulicBrake : K:=0;
   PneumaticBrake:if Compressor<MaxBrakePress then
                   K:=MaxBrakeForce*LocalBrakeRatio/2
                  else
                   K:=0;
  end;
  if BrakePress>0.05*MaxBrakePress then         {nie luz}
   begin
     if (BrakeSubsystem=Oerlikon) and (Vel>50) then
      K:=K+BrakePress*P2FTrans*OerlikonForceFactor  {w kN}
     else
      K:=K+BrakePress*P2FTrans;                     {w kN}
     K:=K*BrakeCylNo/(NBrakeAxles*NBpA);            {w kN na os}
   end;
  u:=60*(0.16*K+100)/((0.8*K+100)*(3*Vel+100)); {wsp. tarcia}
  UnitBrakeForce:=u*K*1000;                     {sila na jeden klocek w N}
  if (NBpA*UnitBrakeForce>Adhesive(Track.friction)*TotalMass*g/NAxles) and (Abs(V)>0.001) then
   {poslizg}
   begin
{     Fb:=Adhesive(Track.friction)*Mass*g;  }
     SlippingWheels:=True;
   end ;
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

function TMoverParameters.CouplerForce(CouplerN:byte; dt:real):real;
var CF,dV,Fmax,BetaAvg:real; CNext:byte;
const MaxDist=405.0; {ustawione +5 m, bo skanujemy do 400 m }
begin
  CF:=0;
  CNext:=CouplerNext(CouplerN);
{  if Couplers[CouplerN].CForce=0 then  {nie bylo uzgadniane wiec policz}
   with Couplers[CouplerN] do
    begin
      CheckCollision:=False;
      Dist:=Distance(Loc,Connected^.Loc,Dim,Connected^.Dim);
      if (Couplers[CouplerN].CouplingFlag=ctrain_virtual) and (Dist>0) then
       begin
         CF:=0;              {kontrola zderzania sie - OK}
         if (Dist>MaxDist) then
          begin {mozna juz nie sledzic}
            Connected^.Couplers[CNext].Connected:=nil;
            Connected:=nil;
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
            BetaAvg:=(beta+Connected^.Couplers[CNext].beta)/2;
            Fmax:=(FmaxC+FmaxB+Connected^.Couplers[CNext].FmaxC+Connected^.Couplers[CNext].FmaxB)*CouplerTune/2;
          end;
         dV:=V-Connected^.V;
         if Dist>0 then
          begin
            CF:=(-(SpringKC+Connected^.Couplers[CNext].SpringKC)*Dist/2)*DirF(CouplerN)-Fmax*dV*betaAvg;
                {liczenie sily ze sprezystosci sprzegu}
            if Dist>(DmaxC+Connected^.Couplers[CNext].DmaxC) then {zderzenie}
             CheckCollision:=True;
          end;
         if Dist<0 then
          begin
            CF:=(-(SpringKB+Connected^.Couplers[CNext].SpringKB)*Dist/2)*DirF(CouplerN)-Fmax*dV*betaAvg;
            if -Dist>(DmaxB+Connected^.Couplers[CNext].DmaxB) then  {zderzenie}
             begin
               CheckCollision:=True;
               if (CouplerType=Automatic) and (CouplingFlag=0) then  {sprzeganie wagonow z samoczynnymi sprzegami}
                CouplingFlag:=ctrain_coupler;
             end;
          end;
       end;
      if CouplingFlag<>ctrain_virtual then         {uzgadnianie prawa Newtona}
       Connected^.Couplers[1-CouplerN].CForce:=-CF;
    end;
{   else CF:=Couplers[CouplerN].CForce;   22.11.02: juz niepotrzebne}
  CouplerForce:=CF;
end;

procedure TMoverParameters.CollisionDetect(CouplerN:byte; dt:real);
var CCF,Vprev,VprevC:real; VirtualCoupling:boolean;
begin
     with Couplers[CouplerN] do
      begin
        VirtualCoupling:=(CouplingFlag=ctrain_virtual);
        Vprev:=V;
        VprevC:=Connected^.V;
        case CouplerN of
         0 : CCF:=ComputeCollision(V,Connected^.V,TotalMass,Connected^.TotalMass,(beta+Connected^.Couplers[CouplerNext(CouplerN)].beta)/2,VirtualCoupling)/(dt+0.01);
         1 : CCF:=ComputeCollision(Connected^.V,V,Connected^.TotalMass,TotalMass,(beta+Connected^.Couplers[CouplerNext(CouplerN)].beta)/2,VirtualCoupling)/(dt+0.01);
        end;
        AccS:=AccS+(V-Vprev)/dt;
        Connected^.AccS:=Connected^.AccS+(Connected^.V-VprevC)/dt;
        if (Dist>0) and (not VirtualCoupling) then
         if FuzzyLogic(Abs(CCF),FmaxC+1,p_coupldmg) then
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


function TMoverParameters.FrictionForce(R:real;TDamage:byte):real;
var A:real; BearingF,RollF: real;
begin
  BearingF:=2*(DamageFlag and dtrain_bearing);
  with Dim do
   begin
    A:=W*H;
    if BearingType=0 then
     RollF:=0.05          {slizgowe}
    else
     RollF:=0.015;        {toczne}
    RollF:=RollF+BearingF/200;
    if NPoweredAxles>0 then
     RollF:=RollF*1.5;    {dodatkowe lozyska silnikow}
    if Abs(V)<0.01 then
     BearingF:=2.5 + 2*BearingF/dtrain_bearing  {dodatkowe opory przy ruszaniu}
    else
     BearingF:=2.0 + BearingF/dtrain_bearing;
    Ff:=TotalMass*g*(BearingF+RollF*Sqr(V)/10)/1000;
{        Ff:=Mass*g*(BearingF+RollF*Vel/NAxles)/1000+Cx*A*Sqr(Vel);}
{dorobic liczenie temperatury lozyska!}
    Ff:=Ff+Cx*A*Sqr(V)
   end;
   FrictionForce:=Ff;
end;

function TMoverParameters.AddPulseForce(Multipler:integer): boolean; {dla drezyny}
begin
  if (EngineType=WheelsDriven) and (EnginePowerSource.SourceType=InternalSource) and (EnginePowerSource.PowerType=BioPower) then
   begin
     ActiveDir:=cabno;
     if Vel>0 then
       PulseForce:=Min0R(1000*Power/(Abs(V)+0.1),Ftmax)
      else PulseForce:=Ftmax;
     if PulseForceCount>1000 then
      PulseForce:=0
     else
       PulseForce:=PulseForce*Multipler;
     PulseForceCount:=PulseForceCount+Abs(Multipler);
     AddPulseForce:=(PulseForce>0);
   end
  else AddPulseForce:=False;
end;


function TMoverParameters.LoadingDone(LSpeed:real; LoadInit:string): boolean;
var LoadChange:longint;
begin
  LoadingDone:=False;
  if (LoadInit<>'') then
   begin
     if Load>MaxLoad then
      LoadChange:=Abs(Trunc(LSpeed*LastLoadChangeTime/2))
     else LoadChange:=Abs(Trunc(LSpeed*LastLoadChangeTime));
     if LoadChange<>0 then
      begin
        LastLoadChangeTime:=0;
        if LSpeed<0 then
          begin
            LoadStatus:=-1;
            Load:=Load-LoadChange;
            CommandIn.Value1:=CommandIn.Value1-LoadChange;
            if Load<0 then
             Load:=0;
            if (Load=0) or (CommandIn.Value1<0) then
             LoadingDone:=True;  {skonczony wyladunek}
            if Load=0 then LoadType:='';
          end
        else
         if LSpeed>0 then
          begin
            LoadType:=LoadInit;
            LoadStatus:=1;
            Load:=Load+LoadChange;
            CommandIn.Value1:=CommandIn.Value1-LoadChange;
            if (Load>=MaxLoad*(1+OverLoadFactor)) or (CommandIn.Value1<0) then
             LoadingDone:=True;      {skonczony zaladunek}
          end
         else LoadingDone:=True;
      end;
   end;
end;

{-------------------------------------------------------------------}
{WAZNA FUNKCJA - oblicza sile wypadkowa}
procedure TMoverParameters.ComputeTotalForce(dt:real);
var b: byte;
begin
{  EventFlag:=False;                                             {jesli cos sie zlego wydarzy to ustawiane na True}
{  SoundFlag:=0;                                                 {jesli ma byc jakis dzwiek to zapalany jet odpowiedni bit}
{to powinno byc zerowane na zewnatrz}
  LastSwitchingTime:=LastSwitchingTime+dt;
  if EngineType=ElectricSeriesMotor then
   LastRelayTime:=LastRelayTime+dt;
  Vel:=Abs(V)*3.6;
  nrot:=V2n;
  if Mains and (Abs(CabNo)<2) then                              {potem ulepszyc! pantogtrafy!}
   begin
     if CabNo=0 then
      Voltage:=RunningTraction.TractionVoltage*ActiveDir
     else
      Voltage:=RunningTraction.tractionVoltage*ActiveDir*CabNo;
   end {bo nie dzialalo}
  else
   Voltage:=0;
  if Power>0 then
   FTrain:=TractionForce(dt)
  else
   FTrain:=0;
  Fb:=BrakeForce(RunningTrack);
  if Max0R(Abs(FTrain),Fb)>Adhesive(RunningTrack.friction)*TotalMass*g then    {poslizg}
   SlippingWheels:=True;
  if SlippingWheels then
   begin
{     TrainForce:=TrainForce-Fb; }
     nrot:=ComputeRotatingWheel((FTrain-Fb*Sign(V))/NAxles-Sign(nrot*Pi*WheelDiameter-V)*Adhesive(RunningTrack.friction)*TotalMass,dt,nrot);
     FTrain:=sign(FTrain)*Adhesive(RunningTrack.friction)*TotalMass*g;
     Fb:=Min0R(Fb,Adhesive(RunningTrack.friction)*TotalMass*g);
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

  FStand:=Fb+FrictionForce(RunningShape.R,RunningTrack.DamageFlag);

  FTrain:=FTrain+TotalMass*g*RunningShape.dHtrack/RunningShape.Len;

  {!niejawne przypisanie zmiennej!}
  FTotal:=FTrain-Sign(V)*FStand;

end;


{------------------------------------------------------------}
{OBLICZENIE PRZEMIESZCZENIA}
function TMoverParameters.ComputeMovement(dt:real; Shape:TTrackShape; var Track:TTrackParam; var ElectricTraction:TTractionParam; NewLoc:TLocation; var NewRot:TRotation):real;
var b:byte;
    Vprev,AccSprev:real;
const Vepsilon=1e-5; Aepsilon=1e-3; ASBSpeed=0.8;
begin
  if not TestFlag(DamageFlag,dtrain_out) then
   begin
     RunningShape:=Shape;
     RunningTrack:=Track;
     RunningTraction:=ElectricTraction;
     with ElectricTraction do
      RunningTraction.TractionVoltage:=TractionVoltage*TractionMaxCurrent/(1+TractionMaxCurrent+Abs(Itot*(1+TractionResistivity)));
   end;
  if CategoryFlag=4 then
   OffsetTrackV:=TotalMass/(Dim.L*Dim.W*1000)
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
     AccS:=(FTotal/TotalMass+AccSprev)/2;                            {prawo Newtona ale z wygladzaniem}

     if TestFlag(DamageFlag,dtrain_out) then
      AccS:=-Sign(V)*g*Random;
     {przyspieszenie normalne}
     if Abs(Shape.R)>0.01 then
      AccN:=SQR(V)/Shape.R+g*Shape.dHrail/TrackW
     else AccN:=g*Shape.dHrail/TrackW;

     {szarpanie}
     if FuzzyLogic((10+Track.DamageFlag)*Mass*Vel/Vmax,500000,p_accn) then
       AccV:=sqrt((1+Track.DamageFlag)*random(100)*Vel/(2000*(1+(Track.QualityFlag and 31))))
     else
      AccV:=AccV/2;
     if AccV>1.0 then
      AccN:=AccN+(7-random(5))*AccV/20;

     {wykolejanie na luku oraz z braku szyn}
     if FuzzyLogic((AccN/g)*(1+0.1*(Track.DamageFlag and dtrack_freerail)),TrackW/Dim.H,1)
     or TestFlag(Track.DamageFlag,dtrack_norail) then
      if SetFlag(DamageFlag,dtrain_out) then
       begin
         EventFlag:=True;
         MainS:=False;
         RunningShape.R:=0;
       end;

     {wykolejanie na poszerzeniu toru}
     if CategoryFlag=1 then
      if FuzzyLogic(Abs(Track.Width-TrackW),TrackW/10,1) then
       if SetFlag(DamageFlag,dtrain_out) then
        begin
          EventFlag:=True;
          MainS:=False;
          RunningShape.R:=0;
        end;
     {wykolejanie wkutek niezgodnosci kategorii toru i pojazdu}
     if not TestFlag(RunningTrack.CategoryFlag,CategoryFlag) then
      if SetFlag(DamageFlag,dtrain_out) then
        begin
          EventFlag:=True;
          MainS:=False;
        end;

     V:=V+(3*AccS-AccSprev)*dt/2;                                  {przyrost predkosci}

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
     dL:=(3*V-Vprev)*dt/2;                                       {metoda Adamsa-Bashfortha}
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
 DistCounter:=DistCounter+abs(dL)/1000;
 dL:=0;

   {koniec procedury, tu nastepuja dodatkowe procedury pomocnicze}

{sprawdzanie i ewentualnie wykonywanie->kasowanie polecen}
 RunInternalCommand;
{automatyczny rozruch}
 if EngineType=ElectricSeriesMotor then
  if AutoRelayCheck then
    SetFlag(SoundFlag,sound_relay);
{uklady hamulcowe:}
  if VeselVolume>0 then
   Compressor:=CompressedVolume/(10*VeselVolume)
  else
   begin
     Compressor:=0;
     CompressorFlag:=False;
   end;
 CompressorCheck(dt);
 UpdateBrakePressure(dt);
 UpdatePipePressure(dt);
{hamulec antyposlizgowy - wylaczanie}
 if BrakeSlippingTimer>ASBSpeed then
  SetFlag(BrakeStatus,-b_antislip);
 BrakeSlippingTimer:=BrakeSlippingTimer+dt;
{sypanie piasku}
 if SandDose then
  if Sand>0 then
   begin
     Sand:=Sand-NPoweredAxles*SandSpeed*dt;
{     if Random<dt then SandDose:=False; }
   end
  else
   begin
     SandDose:=False;
     Sand:=0;
   end;
{czuwak/SHP}
 if (Vel>10) and (not DebugmodeFlag) then
  SecuritySystemCheck(dt);
end; {ComputeMovement}

function TMoverParameters.TotalMass: real;
var M:real;
begin
  if Load>0 then
   begin
    if LoadQuantity='tonns' then
     M:=Load*1000
    else
     if LoadType='Passengers' then
      M:=Load*80
     else
      if LoadType='Luggage' then
       M:=Load*100
     else
      if LoadType='Cars' then
       M:=Load*800
      else
       if LoadType='Containers' then
        M:=Load*8000
       else
        if LoadType='Transformers' then
         M:=Load*50000
        else
         M:=Load*1000;
   end
  else M:=0;
  TotalMass:=Mass+M;
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
     LastLoadChangeTime:=0;
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
  else if command='BrakeCtrl' then
   begin
     if BrakeCtrlPosNo>=Trunc(CValue1) then
      begin
        BrakeCtrlPos:=Trunc(CValue1);
        OK:=SendCtrlToNext(command,CValue1,CValue2);
      end;
   end
  else if command='MainSwitch' then
   begin
     if CValue1=1 then Mains:=True
     else Mains:=False;
     OK:=SendCtrlToNext(command,CValue1,CValue2);
   end
  else if command='Direction' then
   begin
     ActiveDir:=Trunc(CValue1);
     OK:=SendCtrlToNext(command,CValue1,CValue2);
   end
  else if command='CabActivisation' then
   begin
     OK:=Power>0.01;
     if OK then
      case Trunc(CValue1) of
      1 : CabNo:=-1;
      2 : CabNo:=1
      else CabNo:=0;
      end;
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
        FuseFlag:=False;  {wlaczenie ponowne obwodu}
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
  else if command='SandDoseOn' then
   begin
     if SandDoseOn then
       OK:=True
     else OK:=False;
   end
  else if command='CabSignal' then
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
     if CValue2=1 then OK:=SetFlag(DamageFlag,Trunc(CValue1));
     if CValue2=-1 then OK:=SetFlag(DamageFlag,-Trunc(CValue1));
   end
 {naladunek/rozladunek}
  else if Pos('Load=',command)=1 then
   begin
    OK:=True;
    if (V=0) and (MaxLoad>0) and (Load<MaxLoad*(1+OverLoadFactor)) then {czy mozna ladowac?}
     if Distance(Loc,CommandIn.Location,Dim,Dim)<3 then {ten peron/rampa}
      begin
        testload:=DUE(command);
        if Pos(testload,LoadAccepted)>0 then {mozna to zaladowac}
         OK:=LoadingDone(Min0R(CValue2,LoadSpeed),testload);
      end;
    if OK then LoadStatus:=0; {nie udalo sie w ogole albo juz skonczone}
   end
  else if Pos('UnLoad=',command)=1 then
   begin
    OK:=True;
    if (V=0) and (Load>0) then                {czy jest co rozladowac?}
     if Distance(Loc,CommandIn.Location,Dim,Dim)<3 then {ten peron}
      begin
        testload:=DUE(command);
        if LoadType=testload then {mozna to rozladowac}
         OK:=LoadingDone(-Min0R(CValue2,LoadSpeed),testload);
      end;
    if OK then LoadStatus:=0;
   end
    else begin end;
   RunCommand:=OK;
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
      end;
   end
  else
   OK:=False;
  RunInternalCommand:=OK;
end;

procedure TMoverParameters.PutCommand(NewCommand:string; NewValue1,NewValue2:real; NewLocation:TLocation);
begin
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
     if abs(OffsetTrackH)>(RunningTrack.Width-TrackW)/2 then
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
  for b:=1 to 2 do
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
  BrakeCylMult:=1;
  VeselVolume:=0;
  BrakeVolume:=0;

  {inicjalizacja zmiennych}
  Loc:=LocInitial;
  Rot:=RotInitial;
  for b:=0 to 1 do
   with Couplers[b] do
    begin
      CouplingFlag:=0;
      Connected:=nil;
      CForce:=0;
      CheckCollision:=False;
    end;
  BrakeCtrlPos:=0;
  BrakeDelayFlag:=0;
  BrakeStatus:=b_off;
  EmergencyBrakeFlag:=False;
  MainCtrlPos:=0;
  ScndCtrlPos:=0;
  MainCtrlActualPos:=0;
  ScndCtrlActualPos:=0;
  Mains:=False;
  ActiveDir:=0; CabNo:=Cab;
  SlippingWheels:=False;
  SandDose:=False;
  FuseFlag:=False;
  ResistorsFlag:=False;
  Rventrot:=0;
  enrot:=0;
  Itot:=0;
  EnginePower:=0;
  BrakePress:=0;
  Compressor:=0;
  BrakeSlippingTimer:=0;
  dpBrake:=0; dpPipe:=0; dpMainValve:=0; dpLocalValve:=0;
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
  EndSignalsFlag:=0;
  HeadSignalsFlag:=0;  
  DistCounter:=0;
  PulseForce:=0;
  PulseForceTimer:=0;
  PulseForceCount:=0;
  eAngle:=1.5;

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
    end;


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
end;


function TMoverParameters.CheckLocomotiveParameters(ReadyFlag:boolean): boolean;
var k:integer; OK:boolean;
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
 {to dac potem do init}
  if ReadyFlag then     {gotowy do drogi}
   begin
     Volume:=BrakeVolume*HighPipePress*10;
     CompressedVolume:=VeselVolume*MinCompressor*(9.8+Random);
     PipePress:=HighPipePress;
     LocalBrakePos:=0;
     if (BrakeSystem=Pneumatic) and (BrakeCtrlPosNo>0) then
      if CabNo=0 then
       BrakeCtrlPos:=-2;
   end
  else
   begin                {zahamowany}
     Volume:=BrakeVolume*HighPipePress*9;
     CompressedVolume:=VeselVolume*MinCompressor*5;
     PipePress:=LowPipePress;
     LocalBrakePos:=0;
     if (BrakeSystem=Pneumatic) and (BrakeCtrlPosNo>0) then
      BrakeCtrlPos:=-2;
   end;
  CheckLocomotiveParameters:=OK;
end;

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
  OK:=True;
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
              else
               if s='road' then
                CategoryFlag:=2
               else
                if s='ship' then
                 CategoryFlag:=4
                else
                if s='airplane' then
                 CategoryFlag:=8
                else
                if s='unimog' then
                 CategoryFlag:=3
                else
                 ConversionError:=-7;
              s:=ExtractKeyWord(lines,'M=');
              Mass:=s2rE(DUE(s));         {w kg}
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
            end;
          if Pos('Load:',lines)>0 then      {stale parametry}
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
            end;
          if Pos('Dimensions:',lines)>0 then      {wymiary}
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
          if Pos('Wheels:',lines)>0 then      {kola}
            begin
{              SetFlag(OKFlag,wheels_ok);}
              s:=ExtractKeyWord(lines,'D=');
              WheelDiameter:=s2rE(DUE(s));
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
            end;
          if Pos('Brake:',lines)>0 then      {hamulce}
            begin
              s:=DUE(ExtractKeyWord(lines,'BrakeType='));
              if s='Oerlikon' then BrakeSubSystem:=Oerlikon
              else
               if s='Knorr' then BrakeSubSystem:=Knorr
               else
                if s='WeLu' then BrakeSubSystem:=WeLu
                 else BrakeSubSystem:=Standard;
              s:=ExtractKeyWord(lines,'NBpA=');
              NBpA:=s2b(DUE(s));
              s:=ExtractKeyWord(lines,'MBF=');
              MaxBrakeForce:=s2r(DUE(s));
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
                   s:=ExtractKeyWord(lines,'BCM=');
                   BrakeCylMult:=s2r(DUE(s));
                   P2FTrans:=1000*Pi*SQR(BrakeCylRadius)*BrakeCylMult; {w kN/MPa}
                   BrakeVolume:=Pi*SQR(BrakeCylRadius)*BrakeCylDist;
                  end
                 else ConversionError:=-5;
               end
              else
               P2FTrans:=0;
              s:=ExtractKeyWord(lines,'HiPP=');
              if s<>'' then HighPipePress:=s2r(DUE(s))
               else HighPipePress:=0.5;
              s:=ExtractKeyWord(lines,'LoPP=');
              if s<>'' then LowPipePress:=s2r(DUE(s))
               else LowPipePress:=Min0R(HighPipePress,0.34);
              DeltaPipePress:=HighPipePress-LowPipePress;
              s:=ExtractKeyWord(lines,'Vv=');
              VeselVolume:=s2r(DUE(s));
              s:=ExtractKeyWord(lines,'MinCP=');
              MinCompressor:=s2r(DUE(s));
              s:=ExtractKeyWord(lines,'MaxCP=');
              MaxCompressor:=s2r(DUE(s));
              s:=ExtractKeyWord(lines,'CompressorSpeed=');
              CompressorSpeed:=s2r(DUE(s));
            end;
          if (Pos('BuffCoupl.',lines)>0) or (Pos('BuffCoupl1.',lines)>0) then  {zderzaki i sprzegi}
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
                    SpringKC:=5000000;
                    DmaxC:=0.05;
                    FmaxC:=200000;
                    SpringKB:=6000000;
                    DmaxB:=0.05;
                    FmaxB:=500000;
                    beta:=0.5;
                  end
                else
                 if (CouplerType=Articulated) then
                  begin
                    SpringKC:=6000000;
                    DmaxC:=0.05;
                    FmaxC:=2000000;
                    SpringKB:=7000000;
                    DmaxB:=0.05;
                    FmaxB:=4000000;
                    beta:=0.5;
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
              end;
          if (Pos('BuffCoupl2.',lines)>0) then  {zderzaki i sprzegi jesli sa rozne}
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
                    SpringKC:=5000000;
                    DmaxC:=0.05;
                    FmaxC:=200000;
                    SpringKB:=6000000;
                    DmaxB:=0.05;
                    FmaxB:=500000;
                    beta:=0.5;
                  end
                else
                 if (CouplerType=Articulated) then
                  begin
                    SpringKC:=6000000;
                    DmaxC:=0.05;
                    FmaxC:=2000000;
                    SpringKB:=7000000;
                    DmaxB:=0.05;
                    FmaxB:=4000000;
                    beta:=0.5;
                  end;
              end;
          if Pos('Cntrl.',lines)>0 then      {nastawniki}
            begin
              s:=DUE(ExtractKeyWord(lines,'BrakeSystem='));
              if s='Pneumatic' then BrakeSystem:=Pneumatic
               else if s='ElectroPneumatic' then BrakeSystem:=ElectroPneumatic
                else BrakeSystem:=Individual;
              if BrakeSystem<>Individual then
               begin
                 s:=ExtractKeyWord(lines,'BCPN=');
                 BrakeCtrlPosNo:=s2b(DUE(s));
                 if BrakeCtrlPosNo>0 then
                  begin
                    for b:=1 to 4 do
                     begin
                       s:=ExtractKeyWord(lines,'BDelay'+Cut_Space(b2s(b),CutLeft)+'=');
                       BrakeDelay[b]:=s2b(DUE(s));
                     end;
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
              s:=ExtractKeyWord(lines,'MCPN=');
              MainCtrlPosNo:=s2b(DUE(s));
              s:=ExtractKeyWord(lines,'SCPN=');
              ScndCtrlPosNo:=s2b(DUE(s));
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
            end;
          if Pos('Light:',lines)>0 then      {zrodlo mocy dla oswietlenia}
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
            end;
          if Pos('Security:',lines)>0 then      {zrodlo mocy dla oswietlenia}
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
             end;
          if Pos('Clima:',lines)>0 then      {zrodlo mocy dla ogrzewania itp}
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
            end;
          if Pos('Power:',lines)>0 then      {zrodlo mocy dla silnikow trakcyjnych itp}
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
            end;
          if Pos('Engine:',lines)>0 then    {stale parametry silnika}
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
                 nmax:=s2rE(DUE(s))/60;
                 s:=DUE(ExtractKeyWord(lines,'DynamicBrakeType='));
                 if s='Passive' then DynamicBrakeType:=dbrake_passive
                  else
                   if s='Switch' then DynamicBrakeType:=dbrake_switch
                    else
                     if s='Reversal' then DynamicBrakeType:=dbrake_reversal
                      else
                       if s='Automatic' then DynamicBrakeType:=dbrake_automatic
                        else DynamicBrakeType:=dbrake_none;
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
           else ConversionError:=-13; {not implemented yet!}
           end;
          end;
          if Pos('MotorParamTable:',lines)>0 then
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
             end
            else ConversionError:=-3;
           end;
          if Pos('Circuit:',lines)>0 then
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
           end;
          if Pos('RList:',lines)>0 then
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
                RVentnmax:=s2rE(DUE(s))/60;
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
                 readln(fin,i);
                 Rlist[k].AutoSwitch:=Boolean(i);
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



