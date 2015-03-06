//---------------------------------------------------------------------------

#ifndef DynObjH
#define DynObjH

#include "Classes.h"
#include "TrkFoll.h"
#include "QueryParserComp.hpp"
#include "Mover.h"
#include "TractionPower.h"
//McZapkie:
#include "MdlMngr.h"
#include "RealSound.h"
#include "AdvSound.h"
#include "Button.h"
#include "AirCoupler.h"
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
const ANIM_TYPES=7; //Ra: iloœæ typów animacji
const ANIM_WHEELS =0; //ko³a
const ANIM_DOORS  =1; //drzwi
const ANIM_LEVERS =2; //elementy obracane (wycieraczki, ko³a skrêtne, przestawiacze, klocki ham.)
const ANIM_BUFFERS=3; //elementy przesuwane (zderzaki)
const ANIM_BOOGIES=4; //wózki (s¹ skrêcane w dwóch osiach)
const ANIM_PANTS  =5; //pantografy
const ANIM_STEAMS =6; //napêd parowozu

class TAnim;
typedef void (__closure *TUpdate)(TAnim *pAnim); //typ funkcji aktualizuj¹cej po³o¿enie submodeli

//McZapkie-250202
const MaxAxles=16; //ABu 280105: zmienione z 8 na 16
//const MaxAnimatedAxles=16; //i to tez.
//const MaxAnimatedDoors=16;  //NBMX  wrzesien 2003
/*
Ra: Utworzyæ klasê wyposa¿enia opcjonalnego, z której bêd¹ dziedziczyæ klasy drzwi,
pantografów, napêdu parowozu i innych ruchomych czêœci pojazdów. Klasy powinny byæ
pseudo-wirtualne, bo wirtualne mog¹ obni¿aæ wydajnosœæ.
Przy wczytywaniu MMD utworzyæ tabelê wskaŸnikow na te dodatki. Przy wyœwietlaniu
pojazdu wykonywaæ Update() na kolejnych obiektach wyposa¿enia.
Rozwa¿yæ u¿ycie oddzielnych modeli dla niektórych pojazdów (np. lokomotywy), co
zaoszczêdzi³o by czas ustawiania animacji na modelu wspólnym dla kilku pojazdów,
szczególnie dla pojazdów w danej chwili nieruchomych (przy du¿ym zagêszczeniu
modeli na stacjach na ogó³ przewaga jest tych nieruchomych).
*/
class TAnimValveGear
{//wspó³czynniki do animacji parowozu (wartoœci przyk³adowe dla Pt47)
 int iValues; //iloœæ liczb (wersja):
 float fKorbowodR; //d³ugoœæ korby (pó³ skoku t³oka) [m]: 0.35
 float fKorbowodL; //d³ugoœæ korbowodu [m]: 3.8
 float fDrazekR;   //promieñ mimoœrodu (dr¹¿ka) [m]: 0.18
 float fDrazekL;   //d³. dr¹¿ka mimoœrodowego [m]: 2.55889
 float fJarzmoV;   //wysokoœæ w pionie osi jarzma od osi ko³a [m]: 0.751
 float fJarzmoH;   //odleg³oœæ w poziomie osi jarzma od osi ko³a [m]: 2.550
 float fJarzmoR;   //promieñ jarzma do styku z dr¹¿kiem [m]: 0.450
 float fJarzmoA;   //k¹t mimoœrodu wzglêdem k¹ta ko³a [°]: -96.77416667
 float fWdzidloL;  //d³ugoœæ wodzid³a [m]: 2.0
 float fWahaczH;   //d³ugoœæ wahacza (góra) [m]: 0.14
 float fSuwakH;    //wysokoœæ osi suwaka ponad osi¹ ko³a [m]: 0.62
 float fWahaczL;   //d³ugoœæ wahacza (dó³) [m]: 0.84
 float fLacznikL;  //d³ugoœæ ³¹cznika wahacza [m]: 0.75072
 float fRamieL;    //odleg³oœæ ramienia krzy¿ulca od osi ko³a [m]: 0.192
 float fSuwakL;    //odleg³oœæ œrodka t³oka/suwaka od osi ko³a [m]: 5.650
//do³o¿yæ parametry dr¹¿ka nastawnicy
//albo nawet zrobiæ dynamiczn¹ tablicê float[] i w ni¹ pakowaæ wszelkie wspó³czynniki, potem u¿ywaæ indeksów
//wspó³czynniki mog¹ byæ wspólne dla 2-4 t³oków, albo ka¿dy t³ok mo¿e mieæ odrêbne
};

class TAnimPant
{//wspó³czynniki do animacji pantografu
public:
 vector3 vPos; //Ra: wspó³rzêdne punktu zerowego pantografu (X dodatnie dla przedniego)
 double fLenL1; //d³ugoœæ dolnego ramienia 1, odczytana z modelu
 double fLenU1; //d³ugoœæ górnego ramienia 1, odczytana z modelu
 double fLenL2; //d³ugoœæ dolnego ramienia 2, odczytana z modelu
 double fLenU2; //d³ugoœæ górnego ramienia 2, odczytana z modelu
 double fHoriz; //przesuniêcie œlizgu w d³ugoœci pojazdu wzglêdem osi obrotu dolnego ramienia
 double fHeight; //wysokoœæ œlizgu ponad oœ obrotu, odejmowana od wysokoœci drutu
 double fWidth; //po³owa szerokoœci roboczej œlizgu, do wykrycia zeœlizgniêcia siê drutu
 double fAngleL0; //Ra: pocz¹tkowy k¹t dolnego ramienia (odejmowany przy animacji)
 double fAngleU0; //Ra: pocz¹tkowy k¹t górnego ramienia (odejmowany przy animacji)
 double PantTraction; //Winger 170204: wysokoœæ drutu ponad punktem na wysokoœci vPos.y p.g.s.
 double PantWys; //Ra: aktualna wysokoœæ uniesienia œlizgu do porównania z wysokoœci¹ drutu
 double fAngleL; //Winger 160204: aktualny k¹t ramienia dolnego
 double fAngleU; //Ra: aktualny k¹t ramienia górnego
 double NoVoltTime; //czas od utraty kontaktu z drutem
 TTraction *hvPowerWire; //aktualnie podczepione druty, na razie tu
 float fWidthExtra; //dodatkowy rozmiar poziomy poza czêœæ robocz¹ (fWidth)
 float fHeightExtra[5]; //³amana symuluj¹ca kszta³t nabie¿nika
 //double fHorizontal; //Ra 2015-01: po³o¿enie drutu wzglêdem osi pantografu 
 void __fastcall AKP_4E();
};

class TAnim
{//klasa animowanej czêœci pojazdu (ko³a, drzwi, pantografy, burty, napêd parowozu, si³owniki itd.)
public:
 union
 {
  TSubModel *smAnimated; //animowany submodel (jeœli tylko jeden, np. oœ)
  TSubModel **smElement; //jeœli animowanych elementów jest wiêcej (pantograf, napêd parowozu)
  int iShift; //przesuniêcie przed przydzieleniem wskaŸnika
 };
 union
 {//parametry animacji
  TAnimValveGear *pValveGear; //wspó³czynniki do animacji parowozu
  double *dWheelAngle; //wskaŸnik na k¹t obrotu osi
  float *fParam; //ró¿ne parametry dla animacji
  TAnimPant *fParamPants; //ró¿ne parametry dla animacji
 };
 //void _fastcall Update(); //wskaŸnik do funkcji aktualizacji animacji
 int iFlags; //flagi animacji
 float fMaxDist; //do jakiej odleg³oœci wykonywana jest animacja
 float fSpeed; //parametr szybkoœci animacji
 int iNumber; //numer kolejny obiektu
public:
 __fastcall TAnim();
 __fastcall ~TAnim();
 TUpdate yUpdate; //metoda TDynamicObject aktualizuj¹ca animacjê
 int __fastcall TypeSet(int i); //ustawienie typu
 void __fastcall Parovoz(); //wykonanie obliczeñ animacji
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

class TDynamicObject
{//klasa pojazdu
private: //po³o¿enie pojazdu w œwiecie oraz parametry ruchu
 vector3 vPosition; //Ra: pozycja pojazdu liczona zaraz po przesuniêciu
 vector3 vCoulpler[2]; //wspó³rzêdne sprzêgów do liczenia zderzeñ czo³owych
 vector3 vUp,vFront,vLeft; //wektory jednostkowe ustawienia pojazdu
 int iDirection; //kierunek pojazdu wzglêdem czo³a sk³adu (1=zgodny,0=przeciwny)
 TTrackShape ts; //parametry toru przekazywane do fizyki
 TTrackParam tp; //parametry toru przekazywane do fizyki
 TTrackFollower Axle0; //oœ z przodu (od sprzêgu 0)
 TTrackFollower Axle1; //oœ z ty³u (od sprzêgu 1)
 int iAxleFirst; //numer pierwszej osi w kierunku ruchu (oœ wi¹¿¹ca pojazd z torem i wyzwalaj¹ca eventy)
 float fAxleDist; //rozstaw wózków albo osi do liczenia proporcji zacienienia
 vector3 modelRot; //obrot pud³a wzglêdem œwiata - do przeanalizowania, czy potrzebne!!!
 //bool bCameraNear; //blisko kamer s¹ potrzebne dodatkowe obliczenia szczegó³ów
 TDynamicObject* __fastcall ABuFindNearestObject(TTrack *Track,TDynamicObject *MyPointer,int &CouplNr);
public: //parametry po³o¿enia pojazdu dostêpne publicznie
 AnsiString asTrack; //nazwa toru pocz¹tkowego; wywaliæ?
 AnsiString asDestination; //dok¹d pojazd ma byæ kierowany "(stacja):(tor)"
 matrix4x4 mMatrix; //macierz przekszta³cenia do renderowania modeli
 TMoverParameters *MoverParameters; //parametry fizyki ruchu oraz przeliczanie
 //TMoverParameters *pControlled; //wskaŸnik do sterowanego cz³onu silnikowego
 TDynamicObject *NextConnected; //pojazd pod³¹czony od strony sprzêgu 1 (kabina -1)
 TDynamicObject *PrevConnected; //pojazd pod³¹czony od strony sprzêgu 0 (kabina 1)
 int NextConnectedNo; //numer sprzêgu pod³¹czonego z ty³u
 int PrevConnectedNo; //numer sprzêgu pod³¹czonego z przodu
 double fScanDist; //odleg³oœæ skanowania torów na obecnoœæ innych pojazdów 

public: //modele sk³adowe pojazdu
 TModel3d *mdModel; //model pud³a
 TModel3d *mdLoad; //model zmiennego ³adunku
 TModel3d *mdPrzedsionek; //model przedsionków dla EZT - mo¿e u¿yæ mdLoad zamiast?
 TModel3d *mdKabina; //model kabiny dla u¿ytkownika; McZapkie-030303: to z train.h
 TModel3d *mdLowPolyInt; //ABu 010305: wnetrze lowpoly
 float fShade; //zacienienie: 0:normalnie, -1:w ciemnoœci, +1:dodatkowe œwiat³o (brak koloru?)

private: //zmienne i metody do animacji submodeli; Ra: sprzatam animacje w pojeŸdzie
public: //tymczasowo udostêpnione do wyszukiwania drutu
 int iAnimType[ANIM_TYPES]; //0-osie,1-drzwi,2-obracane,3-zderzaki,4-wózki,5-pantografy,6-t³oki
private:
 int iAnimations; //liczba obiektów animuj¹cych
 TAnim *pAnimations; //obiekty animuj¹ce (zawieraj¹ wskaŸnik do funkcji wykonuj¹cej animacjê)
 TSubModel **pAnimated; //lista animowanych submodeli (mo¿e byæ ich wiêcej ni¿ obiektów animuj¹cych)
 double dWheelAngle[3]; //k¹ty obrotu kó³: 0=przednie toczne, 1=napêdzaj¹ce i wi¹zary, 2=tylne toczne
 void UpdateNone(TAnim *pAnim) {}; //animacja pusta (funkcje ustawiania submodeli, gdy blisko kamery)
 void UpdateAxle(TAnim *pAnim); //animacja osi
 void UpdateBoogie(TAnim *pAnim); //animacja wózka
 void UpdateDoorTranslate(TAnim *pAnim); //animacja drzwi - przesuw
 void UpdateDoorRotate(TAnim *pAnim); //animacja drzwi - obrót
 void UpdateDoorFold(TAnim *pAnim); //animacja drzwi - sk³adanie
 void UpdatePant(TAnim *pAnim); //animacja pantografu
private: //Ra: ci¹g dalszy animacji, dopiero do ogarniêcia
 //ABuWozki 060504
 vector3 bogieRot[2];   //Obroty wozkow w/m korpusu
 TSubModel *smBogie[2]; //Wyszukiwanie max 2 wozkow
 TSubModel *smWahacze[4]; //wahacze (np. nogi, dŸwignia w drezynie)
 TSubModel *smBrakeMode; //Ra 15-01: nastawa hamulca te¿
 TSubModel *smLoadMode; //Ra 15-01: nastawa pró¿ny/³adowny
 double fWahaczeAmp;
 //Winger 160204 - pantografy
 double pantspeedfactor;
 //animacje typu przesuw
 TSubModel *smBuforLewy[2];
 TSubModel *smBuforPrawy[2];
 TAnimValveGear *pValveGear;
 vector3 vFloor; //pod³oga dla ³adunku
public:
 TAnim *pants; //indeks obiektu animuj¹cego dla pantografu 0
 double NoVoltTime; //czas od utraty zasilania
 double dDoorMoveL; //NBMX
 double dDoorMoveR; //NBMX
 TSubModel *smBrakeSet; //nastawa hamulca (wajcha)
 TSubModel *smLoadSet; //nastawa ³adunku (wajcha)
 TSubModel *smWiper; //wycieraczka (poniek¹d te¿ wajcha)
//Ra: koneic animacji do ogarniêcia

private:
 void ABuLittleUpdate(double ObjSqrDist);
 bool btnOn; //ABu: czy byly uzywane buttony, jesli tak, to po renderingu wylacz
             //bo ten sam model moze byc jeszcze wykorzystany przez inny obiekt!
 double __fastcall ComputeRadius(vector3 p1,vector3 p2,vector3 p3,vector3 p4);

    TButton btCoupler1;     //sprzegi
    TButton btCoupler2;
    TAirCoupler btCPneumatic1;  //sprzegi powietrzne //yB - zmienione z Button na AirCoupler - krzyzyki
    TAirCoupler btCPneumatic2;
    TAirCoupler btCPneumatic1r; //ABu: to zeby nie bylo problemow przy laczeniu wagonow,
    TAirCoupler btCPneumatic2r; //     jesli beda polaczone sprzegami 1<->1 lub 0<->0
    TAirCoupler btPneumatic1;   //ABu: sprzegi powietrzne zolte
    TAirCoupler btPneumatic2;
    TAirCoupler btPneumatic1r;   //ABu: analogicznie jak 4 linijki wyzej
    TAirCoupler btPneumatic2r;

    TButton btCCtrl1;       //sprzegi sterowania
    TButton btCCtrl2;
    TButton btCPass1;       //mostki przejsciowe
    TButton btCPass2;
    char cp1, sp1, cp2, sp2; //ustawienia wê¿y

    TButton btEndSignals11;  //sygnalu konca pociagu
    TButton btEndSignals13;
    TButton btEndSignals21;
    TButton btEndSignals23;
    TButton btEndSignals1;   //zeby bylo kompatybilne ze starymi modelami...
    TButton btEndSignals2;
    TButton btEndSignalsTab1;  //sygnaly konca pociagu (blachy)
    TButton btEndSignalsTab2;
    TButton btHeadSignals11;  //oswietlenie czolowe - przod
    TButton btHeadSignals12;
    TButton btHeadSignals13;
    TButton btHeadSignals21;  //oswietlenie czolowe - tyl
    TButton btHeadSignals22;
    TButton btHeadSignals23;
    TSubModel *smMechanik0; //Ra: mechanik wbudowany w model jako submodel?
    TSubModel *smMechanik1; //mechanik od strony sprzêgu 1
    double enginevolume; //MC: pomocnicze zeby gladziej silnik buczal

    int iAxles; //McZapkie: to potem mozna skasowac i zastapic iNumAxles
    double dRailLength;
    double dRailPosition[MaxAxles];   //licznik pozycji osi w/m szyny
    double dWheelsPosition[MaxAxles]; //pozycja osi w/m srodka pojazdu
    TRealSound rsStukot[MaxAxles];   //dzwieki poszczegolnych osi //McZapkie-270202
    TRealSound rsSilnik; //McZapkie-010302 - silnik
    TRealSound rsWentylator; //McZapkie-030302
    TRealSound rsPisk; //McZapkie-260302
    TRealSound rsDerailment; //McZapkie-051202
    TRealSound rsPrzekladnia;
    TAdvancedSound sHorn1;
    TAdvancedSound sHorn2;
    TAdvancedSound sCompressor; //NBMX wrzesien 2003
    TAdvancedSound sConverter;
    TAdvancedSound sSmallCompressor;
    TAdvancedSound sDepartureSignal;
    TAdvancedSound sTurbo;

//Winger 010304
//    TRealSound rsPanTup; //PSound sPantUp;
    TRealSound sPantUp;
    TRealSound sPantDown;
    TRealSound rsDoorOpen; //Ra: przeniesione z kabiny
    TRealSound rsDoorClose;

    double eng_vol_act;
    double eng_frq_act;
    double eng_dfrq;
    double eng_turbo;
    void __fastcall ABuBogies();
    void __fastcall ABuModelRoll();
    vector3 modelShake;

    bool renderme; //yB - czy renderowac
    //TRealSound sBrakeAcc; //dŸwiêk przyspieszacza
    PSound sBrakeAcc;
    bool bBrakeAcc;
    TRealSound rsUnbrake; //yB - odglos luzowania
    float ModCamRot;
 int iInventory; //flagi bitowe posiadanych submodeli (np. œwiate³)
 void __fastcall TurnOff();
public:
 int iHornWarning; //numer syreny do u¿ycia po otrzymaniu sygna³u do jazdy
 bool bEnabled; //Ra: wyjecha³ na portal i ma byæ usuniêty
protected:

    //TTrackFollower Axle2; //dwie osie z czterech (te s¹ protected)
    //TTrackFollower Axle3; //Ra: wy³¹czy³em, bo k¹ty s¹ liczone w Segment.cpp
    int iNumAxles; //iloœæ osi
    int CouplCounter;
    AnsiString asModel;
public:
    void ABuScanObjects(int ScanDir,double ScanDist);
protected:
    TDynamicObject* __fastcall ABuFindObject(TTrack *Track,int ScanDir,Byte &CouplFound,double &dist);
    void __fastcall ABuCheckMyTrack();

public:
 int *iLights; //wskaŸnik na bity zapalonych œwiate³ (w³asne albo innego cz³onu)
 double fTrackBlock; //odleg³oœæ do przeszkody do dalszego ruchu (wykrywanie kolizji z innym pojazdem)
 TDynamicObject* __fastcall PrevAny();
 TDynamicObject* __fastcall Prev();
 TDynamicObject* __fastcall Next();
 TDynamicObject* __fastcall NextC(int C); 
 double __fastcall NextDistance(double d=-1.0);
 void __fastcall SetdMoveLen(double dMoveLen) {MoverParameters->dMoveLen=dMoveLen;}
 void __fastcall ResetdMoveLen() {MoverParameters->dMoveLen=0;}
 double __fastcall GetdMoveLen() {return MoverParameters->dMoveLen;}

 int __fastcall GetPneumatic(bool front, bool red);
 void __fastcall SetPneumatic(bool front, bool red);
 AnsiString asName;
 AnsiString __fastcall GetName()
 {
  return this?asName:AnsiString("");
 };

    TRealSound rsDiesielInc; //youBy
    TRealSound rscurve; //youBy
//    std::ofstream PneuLogFile; //zapis parametrow pneumatycznych
//youBy - dym
    //TSmoke Smog;
    //float EmR;
    //vector3 smokeoffset;

 TDynamicObject* __fastcall ABuScanNearestObject(TTrack *Track, double ScanDir, double ScanDist, int &CouplNr);
 TDynamicObject* __fastcall GetFirstDynamic(int cpl_type);
 //TDynamicObject* __fastcall GetFirstCabDynamic(int cpl_type);
 void ABuSetModelShake(vector3 mShake);


//McZapkie-010302
    TController *Mechanik;
    TController *ctOwner; //wska¿nik na obiekt zarz¹dzaj¹cy sk³adem
    bool MechInside;
//McZapkie-270202
    bool Controller;
    bool bDisplayCab; //czy wyswietlac kabine w train.cpp
    int iCabs; //maski bitowe modeli kabin
    TTrack *MyTrack; //McZapkie-030303: tor na ktorym stoi, ABu
    AnsiString asBaseDir;
    GLuint ReplacableSkinID[5];  //McZapkie:zmienialne nadwozie
    int iAlpha; //maska przezroczystoœci tekstur
    int iMultiTex; //<0 tekstury wskazane wpisem, >0 tekstury z przecinkami, =0 jedna
    int iOverheadMask; //maska przydzielana przez AI pojazdom posiadaj¹cym pantograf, aby wymusza³y jazdê bezpr¹dow¹
    TTractionParam tmpTraction;
    double fAdjustment; //korekcja - docelowo przenieœæ do TrkFoll.cpp wraz z odleg³oœci¹ od poprzedniego
    __fastcall TDynamicObject();
    __fastcall ~TDynamicObject();
    double __fastcall TDynamicObject::Init
    (//zwraca d³ugoœæ pojazdu albo 0, jeœli b³¹d
     AnsiString Name, AnsiString BaseDir, AnsiString asReplacableSkin, AnsiString Type_Name,
     TTrack *Track, double fDist, AnsiString DriverType, double fVel, AnsiString TrainName,
     float Load, AnsiString LoadType,bool Reversed, AnsiString
    );
    void __fastcall AttachPrev(TDynamicObject *Object, int iType= 1);
    bool __fastcall UpdateForce(double dt, double dt1, bool FullVer);
    void __fastcall LoadUpdate();
    bool __fastcall Update(double dt, double dt1);
    bool __fastcall FastUpdate(double dt);
    void __fastcall Move(double fDistance);
    void __fastcall FastMove(double fDistance);
    void __fastcall Render();
    void __fastcall RenderAlpha();
    void __fastcall RenderSounds();
    inline vector3 __fastcall GetPosition() {return vPosition;};
    inline vector3 __fastcall HeadPosition() {return vCoulpler[iDirection^1];}; //pobranie wspó³rzêdnych czo³a
    inline vector3 __fastcall RearPosition() {return vCoulpler[iDirection];}; //pobranie wspó³rzêdnych ty³u
    inline vector3 __fastcall AxlePositionGet() {return iAxleFirst?Axle1.pPosition:Axle0.pPosition;};
    inline vector3 __fastcall VectorFront() {return vFront;};
    inline vector3 __fastcall VectorUp() {return vUp;};
    inline vector3 __fastcall VectorLeft() {return vLeft;};
    inline double* __fastcall Matrix() {return mMatrix.getArray();};
    inline double __fastcall GetVelocity() { return MoverParameters->Vel; };
    inline double __fastcall GetLength() { return MoverParameters->Dim.L; };
    inline double __fastcall GetWidth() { return MoverParameters->Dim.W; };
    inline TTrack* __fastcall GetTrack() { return (iAxleFirst?Axle1.GetTrack():Axle0.GetTrack()); };
    //void __fastcall UpdatePos();


 //McZapkie-260202
 void __fastcall LoadMMediaFile(AnsiString BaseDir, AnsiString TypeName, AnsiString ReplacableSkin);

 inline double __fastcall ABuGetDirection() //ABu.
 {
  return (Axle1.GetTrack()==MyTrack?Axle1.GetDirection():Axle0.GetDirection());
 };
// inline double __fastcall ABuGetTranslation() //ABu.
// {//zwraca przesuniêcie wózka wzglêdem Point1 toru
//  return (Axle1.GetTrack()==MyTrack?Axle1.GetTranslation():Axle0.GetTranslation());
// };
 inline double __fastcall RaDirectionGet()
 {//zwraca kierunek pojazdu na torze z aktywn¹ os¹
  return iAxleFirst?Axle1.GetDirection():Axle0.GetDirection();
 };
 inline double __fastcall RaTranslationGet()
 {//zwraca przesuniêcie wózka wzglêdem Point1 toru z aktywn¹ osi¹
  return iAxleFirst?Axle1.GetTranslation():Axle0.GetTranslation();
 };
 inline TTrack* __fastcall RaTrackGet()
 {//zwraca tor z aktywn¹ osi¹
  return iAxleFirst?Axle1.GetTrack():Axle0.GetTrack();
 };
 void CouplersDettach(double MinDist,int MyScanDir);
 void __fastcall RadioStop();
 void __fastcall RaLightsSet(int head,int rear);
 //void __fastcall RaAxleEvent(TEvent *e);
 TDynamicObject* __fastcall FirstFind(int &coupler_nr);
 float __fastcall GetEPP(); //wyliczanie sredniego cisnienia w PG
 int __fastcall DirectionSet(int d); //ustawienie kierunku w sk³adzie
 int __fastcall DirectionGet() {return iDirection+iDirection-1;}; //odczyt kierunku w sk³adzie
 int DettachStatus(int dir);
 int Dettach(int dir);
 TDynamicObject* __fastcall Neightbour(int &dir);
 void __fastcall CoupleDist();
 TDynamicObject* __fastcall ControlledFind();
 void __fastcall ParamSet(int what,int into);
 int __fastcall RouteWish(TTrack *tr); //zapytanie do AI, po którym segmencie skrzy¿owania jechaæ
 void __fastcall DestinationSet(AnsiString to);
 AnsiString __fastcall TextureTest(AnsiString &name);
 void __fastcall OverheadTrack(float o);
};


//---------------------------------------------------------------------------
#endif
