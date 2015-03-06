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
const ANIM_TYPES=7; //Ra: ilo�� typ�w animacji
const ANIM_WHEELS =0; //ko�a
const ANIM_DOORS  =1; //drzwi
const ANIM_LEVERS =2; //elementy obracane (wycieraczki, ko�a skr�tne, przestawiacze, klocki ham.)
const ANIM_BUFFERS=3; //elementy przesuwane (zderzaki)
const ANIM_BOOGIES=4; //w�zki (s� skr�cane w dw�ch osiach)
const ANIM_PANTS  =5; //pantografy
const ANIM_STEAMS =6; //nap�d parowozu

class TAnim;
typedef void (__closure *TUpdate)(TAnim *pAnim); //typ funkcji aktualizuj�cej po�o�enie submodeli

//McZapkie-250202
const MaxAxles=16; //ABu 280105: zmienione z 8 na 16
//const MaxAnimatedAxles=16; //i to tez.
//const MaxAnimatedDoors=16;  //NBMX  wrzesien 2003
/*
Ra: Utworzy� klas� wyposa�enia opcjonalnego, z kt�rej b�d� dziedziczy� klasy drzwi,
pantograf�w, nap�du parowozu i innych ruchomych cz�ci pojazd�w. Klasy powinny by�
pseudo-wirtualne, bo wirtualne mog� obni�a� wydajnos��.
Przy wczytywaniu MMD utworzy� tabel� wska�nikow na te dodatki. Przy wy�wietlaniu
pojazdu wykonywa� Update() na kolejnych obiektach wyposa�enia.
Rozwa�y� u�ycie oddzielnych modeli dla niekt�rych pojazd�w (np. lokomotywy), co
zaoszcz�dzi�o by czas ustawiania animacji na modelu wsp�lnym dla kilku pojazd�w,
szczeg�lnie dla pojazd�w w danej chwili nieruchomych (przy du�ym zag�szczeniu
modeli na stacjach na og� przewaga jest tych nieruchomych).
*/
class TAnimValveGear
{//wsp�czynniki do animacji parowozu (warto�ci przyk�adowe dla Pt47)
 int iValues; //ilo�� liczb (wersja):
 float fKorbowodR; //d�ugo�� korby (p� skoku t�oka) [m]: 0.35
 float fKorbowodL; //d�ugo�� korbowodu [m]: 3.8
 float fDrazekR;   //promie� mimo�rodu (dr��ka) [m]: 0.18
 float fDrazekL;   //d�. dr��ka mimo�rodowego [m]: 2.55889
 float fJarzmoV;   //wysoko�� w pionie osi jarzma od osi ko�a [m]: 0.751
 float fJarzmoH;   //odleg�o�� w poziomie osi jarzma od osi ko�a [m]: 2.550
 float fJarzmoR;   //promie� jarzma do styku z dr��kiem [m]: 0.450
 float fJarzmoA;   //k�t mimo�rodu wzgl�dem k�ta ko�a [�]: -96.77416667
 float fWdzidloL;  //d�ugo�� wodzid�a [m]: 2.0
 float fWahaczH;   //d�ugo�� wahacza (g�ra) [m]: 0.14
 float fSuwakH;    //wysoko�� osi suwaka ponad osi� ko�a [m]: 0.62
 float fWahaczL;   //d�ugo�� wahacza (d�) [m]: 0.84
 float fLacznikL;  //d�ugo�� ��cznika wahacza [m]: 0.75072
 float fRamieL;    //odleg�o�� ramienia krzy�ulca od osi ko�a [m]: 0.192
 float fSuwakL;    //odleg�o�� �rodka t�oka/suwaka od osi ko�a [m]: 5.650
//do�o�y� parametry dr��ka nastawnicy
//albo nawet zrobi� dynamiczn� tablic� float[] i w ni� pakowa� wszelkie wsp�czynniki, potem u�ywa� indeks�w
//wsp�czynniki mog� by� wsp�lne dla 2-4 t�ok�w, albo ka�dy t�ok mo�e mie� odr�bne
};

class TAnimPant
{//wsp�czynniki do animacji pantografu
public:
 vector3 vPos; //Ra: wsp�rz�dne punktu zerowego pantografu (X dodatnie dla przedniego)
 double fLenL1; //d�ugo�� dolnego ramienia 1, odczytana z modelu
 double fLenU1; //d�ugo�� g�rnego ramienia 1, odczytana z modelu
 double fLenL2; //d�ugo�� dolnego ramienia 2, odczytana z modelu
 double fLenU2; //d�ugo�� g�rnego ramienia 2, odczytana z modelu
 double fHoriz; //przesuni�cie �lizgu w d�ugo�ci pojazdu wzgl�dem osi obrotu dolnego ramienia
 double fHeight; //wysoko�� �lizgu ponad o� obrotu, odejmowana od wysoko�ci drutu
 double fWidth; //po�owa szeroko�ci roboczej �lizgu, do wykrycia ze�lizgni�cia si� drutu
 double fAngleL0; //Ra: pocz�tkowy k�t dolnego ramienia (odejmowany przy animacji)
 double fAngleU0; //Ra: pocz�tkowy k�t g�rnego ramienia (odejmowany przy animacji)
 double PantTraction; //Winger 170204: wysoko�� drutu ponad punktem na wysoko�ci vPos.y p.g.s.
 double PantWys; //Ra: aktualna wysoko�� uniesienia �lizgu do por�wnania z wysoko�ci� drutu
 double fAngleL; //Winger 160204: aktualny k�t ramienia dolnego
 double fAngleU; //Ra: aktualny k�t ramienia g�rnego
 double NoVoltTime; //czas od utraty kontaktu z drutem
 TTraction *hvPowerWire; //aktualnie podczepione druty, na razie tu
 float fWidthExtra; //dodatkowy rozmiar poziomy poza cz�� robocz� (fWidth)
 float fHeightExtra[5]; //�amana symuluj�ca kszta�t nabie�nika
 //double fHorizontal; //Ra 2015-01: po�o�enie drutu wzgl�dem osi pantografu 
 void __fastcall AKP_4E();
};

class TAnim
{//klasa animowanej cz�ci pojazdu (ko�a, drzwi, pantografy, burty, nap�d parowozu, si�owniki itd.)
public:
 union
 {
  TSubModel *smAnimated; //animowany submodel (je�li tylko jeden, np. o�)
  TSubModel **smElement; //je�li animowanych element�w jest wi�cej (pantograf, nap�d parowozu)
  int iShift; //przesuni�cie przed przydzieleniem wska�nika
 };
 union
 {//parametry animacji
  TAnimValveGear *pValveGear; //wsp�czynniki do animacji parowozu
  double *dWheelAngle; //wska�nik na k�t obrotu osi
  float *fParam; //r�ne parametry dla animacji
  TAnimPant *fParamPants; //r�ne parametry dla animacji
 };
 //void _fastcall Update(); //wska�nik do funkcji aktualizacji animacji
 int iFlags; //flagi animacji
 float fMaxDist; //do jakiej odleg�o�ci wykonywana jest animacja
 float fSpeed; //parametr szybko�ci animacji
 int iNumber; //numer kolejny obiektu
public:
 __fastcall TAnim();
 __fastcall ~TAnim();
 TUpdate yUpdate; //metoda TDynamicObject aktualizuj�ca animacj�
 int __fastcall TypeSet(int i); //ustawienie typu
 void __fastcall Parovoz(); //wykonanie oblicze� animacji
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

class TDynamicObject
{//klasa pojazdu
private: //po�o�enie pojazdu w �wiecie oraz parametry ruchu
 vector3 vPosition; //Ra: pozycja pojazdu liczona zaraz po przesuni�ciu
 vector3 vCoulpler[2]; //wsp�rz�dne sprz�g�w do liczenia zderze� czo�owych
 vector3 vUp,vFront,vLeft; //wektory jednostkowe ustawienia pojazdu
 int iDirection; //kierunek pojazdu wzgl�dem czo�a sk�adu (1=zgodny,0=przeciwny)
 TTrackShape ts; //parametry toru przekazywane do fizyki
 TTrackParam tp; //parametry toru przekazywane do fizyki
 TTrackFollower Axle0; //o� z przodu (od sprz�gu 0)
 TTrackFollower Axle1; //o� z ty�u (od sprz�gu 1)
 int iAxleFirst; //numer pierwszej osi w kierunku ruchu (o� wi���ca pojazd z torem i wyzwalaj�ca eventy)
 float fAxleDist; //rozstaw w�zk�w albo osi do liczenia proporcji zacienienia
 vector3 modelRot; //obrot pud�a wzgl�dem �wiata - do przeanalizowania, czy potrzebne!!!
 //bool bCameraNear; //blisko kamer s� potrzebne dodatkowe obliczenia szczeg��w
 TDynamicObject* __fastcall ABuFindNearestObject(TTrack *Track,TDynamicObject *MyPointer,int &CouplNr);
public: //parametry po�o�enia pojazdu dost�pne publicznie
 AnsiString asTrack; //nazwa toru pocz�tkowego; wywali�?
 AnsiString asDestination; //dok�d pojazd ma by� kierowany "(stacja):(tor)"
 matrix4x4 mMatrix; //macierz przekszta�cenia do renderowania modeli
 TMoverParameters *MoverParameters; //parametry fizyki ruchu oraz przeliczanie
 //TMoverParameters *pControlled; //wska�nik do sterowanego cz�onu silnikowego
 TDynamicObject *NextConnected; //pojazd pod��czony od strony sprz�gu 1 (kabina -1)
 TDynamicObject *PrevConnected; //pojazd pod��czony od strony sprz�gu 0 (kabina 1)
 int NextConnectedNo; //numer sprz�gu pod��czonego z ty�u
 int PrevConnectedNo; //numer sprz�gu pod��czonego z przodu
 double fScanDist; //odleg�o�� skanowania tor�w na obecno�� innych pojazd�w 

public: //modele sk�adowe pojazdu
 TModel3d *mdModel; //model pud�a
 TModel3d *mdLoad; //model zmiennego �adunku
 TModel3d *mdPrzedsionek; //model przedsionk�w dla EZT - mo�e u�y� mdLoad zamiast?
 TModel3d *mdKabina; //model kabiny dla u�ytkownika; McZapkie-030303: to z train.h
 TModel3d *mdLowPolyInt; //ABu 010305: wnetrze lowpoly
 float fShade; //zacienienie: 0:normalnie, -1:w ciemno�ci, +1:dodatkowe �wiat�o (brak koloru?)

private: //zmienne i metody do animacji submodeli; Ra: sprzatam animacje w poje�dzie
public: //tymczasowo udost�pnione do wyszukiwania drutu
 int iAnimType[ANIM_TYPES]; //0-osie,1-drzwi,2-obracane,3-zderzaki,4-w�zki,5-pantografy,6-t�oki
private:
 int iAnimations; //liczba obiekt�w animuj�cych
 TAnim *pAnimations; //obiekty animuj�ce (zawieraj� wska�nik do funkcji wykonuj�cej animacj�)
 TSubModel **pAnimated; //lista animowanych submodeli (mo�e by� ich wi�cej ni� obiekt�w animuj�cych)
 double dWheelAngle[3]; //k�ty obrotu k�: 0=przednie toczne, 1=nap�dzaj�ce i wi�zary, 2=tylne toczne
 void UpdateNone(TAnim *pAnim) {}; //animacja pusta (funkcje ustawiania submodeli, gdy blisko kamery)
 void UpdateAxle(TAnim *pAnim); //animacja osi
 void UpdateBoogie(TAnim *pAnim); //animacja w�zka
 void UpdateDoorTranslate(TAnim *pAnim); //animacja drzwi - przesuw
 void UpdateDoorRotate(TAnim *pAnim); //animacja drzwi - obr�t
 void UpdateDoorFold(TAnim *pAnim); //animacja drzwi - sk�adanie
 void UpdatePant(TAnim *pAnim); //animacja pantografu
private: //Ra: ci�g dalszy animacji, dopiero do ogarni�cia
 //ABuWozki 060504
 vector3 bogieRot[2];   //Obroty wozkow w/m korpusu
 TSubModel *smBogie[2]; //Wyszukiwanie max 2 wozkow
 TSubModel *smWahacze[4]; //wahacze (np. nogi, d�wignia w drezynie)
 TSubModel *smBrakeMode; //Ra 15-01: nastawa hamulca te�
 TSubModel *smLoadMode; //Ra 15-01: nastawa pr�ny/�adowny
 double fWahaczeAmp;
 //Winger 160204 - pantografy
 double pantspeedfactor;
 //animacje typu przesuw
 TSubModel *smBuforLewy[2];
 TSubModel *smBuforPrawy[2];
 TAnimValveGear *pValveGear;
 vector3 vFloor; //pod�oga dla �adunku
public:
 TAnim *pants; //indeks obiektu animuj�cego dla pantografu 0
 double NoVoltTime; //czas od utraty zasilania
 double dDoorMoveL; //NBMX
 double dDoorMoveR; //NBMX
 TSubModel *smBrakeSet; //nastawa hamulca (wajcha)
 TSubModel *smLoadSet; //nastawa �adunku (wajcha)
 TSubModel *smWiper; //wycieraczka (poniek�d te� wajcha)
//Ra: koneic animacji do ogarni�cia

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
    char cp1, sp1, cp2, sp2; //ustawienia w�y

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
    TSubModel *smMechanik1; //mechanik od strony sprz�gu 1
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
    //TRealSound sBrakeAcc; //d�wi�k przyspieszacza
    PSound sBrakeAcc;
    bool bBrakeAcc;
    TRealSound rsUnbrake; //yB - odglos luzowania
    float ModCamRot;
 int iInventory; //flagi bitowe posiadanych submodeli (np. �wiate�)
 void __fastcall TurnOff();
public:
 int iHornWarning; //numer syreny do u�ycia po otrzymaniu sygna�u do jazdy
 bool bEnabled; //Ra: wyjecha� na portal i ma by� usuni�ty
protected:

    //TTrackFollower Axle2; //dwie osie z czterech (te s� protected)
    //TTrackFollower Axle3; //Ra: wy��czy�em, bo k�ty s� liczone w Segment.cpp
    int iNumAxles; //ilo�� osi
    int CouplCounter;
    AnsiString asModel;
public:
    void ABuScanObjects(int ScanDir,double ScanDist);
protected:
    TDynamicObject* __fastcall ABuFindObject(TTrack *Track,int ScanDir,Byte &CouplFound,double &dist);
    void __fastcall ABuCheckMyTrack();

public:
 int *iLights; //wska�nik na bity zapalonych �wiate� (w�asne albo innego cz�onu)
 double fTrackBlock; //odleg�o�� do przeszkody do dalszego ruchu (wykrywanie kolizji z innym pojazdem)
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
    TController *ctOwner; //wska�nik na obiekt zarz�dzaj�cy sk�adem
    bool MechInside;
//McZapkie-270202
    bool Controller;
    bool bDisplayCab; //czy wyswietlac kabine w train.cpp
    int iCabs; //maski bitowe modeli kabin
    TTrack *MyTrack; //McZapkie-030303: tor na ktorym stoi, ABu
    AnsiString asBaseDir;
    GLuint ReplacableSkinID[5];  //McZapkie:zmienialne nadwozie
    int iAlpha; //maska przezroczysto�ci tekstur
    int iMultiTex; //<0 tekstury wskazane wpisem, >0 tekstury z przecinkami, =0 jedna
    int iOverheadMask; //maska przydzielana przez AI pojazdom posiadaj�cym pantograf, aby wymusza�y jazd� bezpr�dow�
    TTractionParam tmpTraction;
    double fAdjustment; //korekcja - docelowo przenie�� do TrkFoll.cpp wraz z odleg�o�ci� od poprzedniego
    __fastcall TDynamicObject();
    __fastcall ~TDynamicObject();
    double __fastcall TDynamicObject::Init
    (//zwraca d�ugo�� pojazdu albo 0, je�li b��d
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
    inline vector3 __fastcall HeadPosition() {return vCoulpler[iDirection^1];}; //pobranie wsp�rz�dnych czo�a
    inline vector3 __fastcall RearPosition() {return vCoulpler[iDirection];}; //pobranie wsp�rz�dnych ty�u
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
// {//zwraca przesuni�cie w�zka wzgl�dem Point1 toru
//  return (Axle1.GetTrack()==MyTrack?Axle1.GetTranslation():Axle0.GetTranslation());
// };
 inline double __fastcall RaDirectionGet()
 {//zwraca kierunek pojazdu na torze z aktywn� os�
  return iAxleFirst?Axle1.GetDirection():Axle0.GetDirection();
 };
 inline double __fastcall RaTranslationGet()
 {//zwraca przesuni�cie w�zka wzgl�dem Point1 toru z aktywn� osi�
  return iAxleFirst?Axle1.GetTranslation():Axle0.GetTranslation();
 };
 inline TTrack* __fastcall RaTrackGet()
 {//zwraca tor z aktywn� osi�
  return iAxleFirst?Axle1.GetTrack():Axle0.GetTrack();
 };
 void CouplersDettach(double MinDist,int MyScanDir);
 void __fastcall RadioStop();
 void __fastcall RaLightsSet(int head,int rear);
 //void __fastcall RaAxleEvent(TEvent *e);
 TDynamicObject* __fastcall FirstFind(int &coupler_nr);
 float __fastcall GetEPP(); //wyliczanie sredniego cisnienia w PG
 int __fastcall DirectionSet(int d); //ustawienie kierunku w sk�adzie
 int __fastcall DirectionGet() {return iDirection+iDirection-1;}; //odczyt kierunku w sk�adzie
 int DettachStatus(int dir);
 int Dettach(int dir);
 TDynamicObject* __fastcall Neightbour(int &dir);
 void __fastcall CoupleDist();
 TDynamicObject* __fastcall ControlledFind();
 void __fastcall ParamSet(int what,int into);
 int __fastcall RouteWish(TTrack *tr); //zapytanie do AI, po kt�rym segmencie skrzy�owania jecha�
 void __fastcall DestinationSet(AnsiString to);
 AnsiString __fastcall TextureTest(AnsiString &name);
 void __fastcall OverheadTrack(float o);
};


//---------------------------------------------------------------------------
#endif
