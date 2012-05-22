//---------------------------------------------------------------------------

#ifndef DynObjH
#define DynObjH

#include "Classes.h"
#include "TrkFoll.h"
//#include "Track.h"
#include "QueryParserComp.hpp"
#include "AnimModel.h"
#include "Mover.hpp"
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
//McZapkie-250202
 const MaxAxles=16; //ABu 280105: zmienione z 8 na 16
 const MaxAnimatedAxles=16; //i to tez.
 const MaxAnimatedDoors=8;  //NBMX  wrzesien 2003
 const ANIM_TYPES=7; //Ra: iloœæ typów animacji
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
{//wspó³czynniki do animacji parowozu
};

class TAnim
{//klasa animowanej czêœci pojazdu (ko³a, drzwi, pantografy, burty, napêd parowozu, si³owniki itd.)
 union
 {
  TSubModel *smAnimated; //animowany submodel
  TSubModel **smElement; //jeœli animowanych elementów jest wiêcej (pantograf, napêd parowozu)
 };
 union
 {//kolejno: t³oczysko, korbowód, dr¹¿ek, jarzmo, trzon suwaka, wahacz, dr. wahacza, dr. wahacza
  TAnimValveGear *pValveGear; //wspó³czynniki do animacji parowozu
  double *pWheelAngle; //wskaŸnik na k¹t obrotu osi
  struct
  {//dla
  };
 };
 int iFlags; //flagi animacji
public:
 int __fastcall TypeSet(int i); //ustawienie typu
 void __fastcall Parovoz(); //wykonanie obliczeñ animacji
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

class TDynamicObject
{//klasa pojazdu
private:
 vector3 vPosition; //Ra: pozycja pojazdu liczona zaraz po przesuniêciu
 //Ra: sprzatam animacje w pojeŸdzie
 int iAnimType[ANIM_TYPES]; //0-osie,1-wi¹zary,2-wózki,3-wahacze,4-pantografy,5-drzwi,6-t³oki
 int iAnimations; //ogólna iloœæ obiektów animuj¹cych
 TAnim *pAnimations; //obiekty animuj¹ce
 TSubModel **pAnimated; //lista animowanych submodeli (mo¿e byæ ich wiêcej ni¿ obiektów animuj¹cych)
 double dWheelAngle[3]; //k¹ty obrotu kó³: 0=przednie toczne, 1=napêdzaj¹ce i wi¹zary, 2=tylne toczne
private:
//Ra: pocz¹tek animacji do ogarniêcia
 //McZapkie-050402 - do krecenia kolami
 int iAnimatedAxles; //iloœæ u¿ywanych (krêconych) osi
 TSubModel *smAnimatedWheel[MaxAnimatedAxles]; //submodele poszczególnych osi
 double *pWheelAngle[MaxAnimatedAxles]; //wska¿niki do odczytu k¹ta obrotu danej osi
 //wi¹zary
 TSubModel *smWiazary[2]; //mo¿na zast¹piæ je osiami
 //ABuWozki 060504
 vector3 bogieRot[2];   //Obroty wozkow w/m korpusu
 TSubModel *smBogie[2]; //Wyszukiwanie max 2 wozkow
 //wahacze
 TSubModel *smWahacze[4];
 double fWahaczeAmp;
 //drzwi
    int iAnimatedDoors;
 TSubModel *smAnimatedDoor[MaxAnimatedDoors];
 double DoorSpeedFactor[MaxAnimatedDoors];
 //double tempdoorfactor;
 //double tempdoorfactor2;
 //Winger 160204 - pantografy
 double pantspeedfactor;
 TSubModel *smPatykird1[2];
 TSubModel *smPatykird2[2];
 TSubModel *smPatykirg1[2];
 TSubModel *smPatykirg2[2];
 TSubModel *smPatykisl[2];
//Ra: koneic animacji do ogarniêcia
private:
 TTrackShape ts;
 TTrackParam tp;
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
    TSubModel *smMechanik;
    TSubModel *smBuforLewy[2];
    TSubModel *smBuforPrawy[2];
    double enginevolume; //MC: pomocnicze zeby gladziej silnik buczal

    int iAxles; //McZapkie: to potem mozna skasowac i zastapic iNumAxles
    double dRailLength;
    double dRailPosition[MaxAxles];   //licznik pozycji osi w/m szyny
    double dWheelsPosition[MaxAxles]; //pozycja osi w/m srodka pojazdu
//McZapkie-270202
    TRealSound rsStukot[MaxAxles];   //dzwieki poszczegolnych osi
//McZapkie-010302 - silnik
    TRealSound rsSilnik;
//McZapkie-030302
    TRealSound rsWentylator;
//McZapkie-260302
    TRealSound rsPisk;
//McZapkie-051202
    TRealSound rsDerailment;

    TAdvancedSound sHorn1;
    TAdvancedSound sHorn2;
//NBMX wrzesien 2003
    TAdvancedSound sCompressor;
    TAdvancedSound sConverter;
    TAdvancedSound sSmallCompressor;
    TAdvancedSound sDepartureSignal;
    TAdvancedSound sTurbo;
//Winger 010304
//    TRealSound rsPanTup; //PSound sPantUp;
    TRealSound sPantUp;
    TRealSound sPantDown;

    double eng_vol_act;
    double eng_frq_act;
    double eng_dfrq;
    double eng_turbo;
    void __fastcall ABuBogies();
    void __fastcall ABuModelRoll();
    vector3 modelShake;

    bool renderme; //yB - czy renderowac
    char cp1, sp1, cp2, sp2; //ustawienia wê¿y
    TRealSound sBrakeAcc; //dŸwiêk przyspieszacza
 int iAxleFirst; //numer pierwszej osi w kierunku ruchu
 int iInventory; //flagi bitowe posiadanych submodeli (np. œwiate³)
 TDynamicObject* __fastcall ABuFindNearestObject(TTrack *Track,TDynamicObject *MyPointer,int &CouplNr);
 void __fastcall TurnOff();
public:
 bool bEnabled; //Ra: wyjecha³ na portal i ma byæ usuniêty
protected:

    //TTrackFollower Axle2; //dwie osie z czterech (te s¹ protected)
    //TTrackFollower Axle3; //Ra: wy³¹czy³em, bo k¹ty s¹ liczone w Segment.cpp
    int iNumAxles; //iloœæ osi
    //Byte NextConnectedNo;
    //Byte PrevConnectedNo;
    int CouplCounter;
    AnsiString asModel;
    int iDirection; //kierunek wzglêdem czo³a sk³adu (1=zgodny,0=przeciwny)
    void ABuScanObjects(int ScanDir,double ScanDist);
    void __fastcall ABuCheckMyTrack();

public:
 float fHalfMaxAxleDist; //rozstaw wózków albo osi
 float fShade; //0:normalnie, -1:w ciemnoœci, +1:dodatkowe œwiat³o (brak koloru?)
 int iLights[2]; //bity zapalonych œwiate³
 double fTrackBlock; //odleg³oœæ do przeszkody do dalszego ruchu
 TDynamicObject* __fastcall Prev();
 TDynamicObject* __fastcall Next();
    void __fastcall SetdMoveLen(double dMoveLen) {MoverParameters->dMoveLen=dMoveLen;}
    void __fastcall ResetdMoveLen() {MoverParameters->dMoveLen=0;}
    double __fastcall GetdMoveLen() {return MoverParameters->dMoveLen;}

    int __fastcall GetPneumatic(bool front, bool red);
    void __fastcall SetPneumatic(bool front, bool red);
		AnsiString asName;
    AnsiString __fastcall GetName()
       {
          return asName;
       };

//youBy
    TRealSound rsDiesielInc;
//youBy
    TRealSound rscurve;

//youBy - dym
    //TSmoke Smog;
    //float EmR;
    //vector3 smokeoffset;

    TDynamicObject *NextConnected; //pojazd pod³¹czony od strony sprzêgu 1 (kabina -1)
    TDynamicObject *PrevConnected; //pojazd pod³¹czony od strony sprzêgu 0 (kabina 1)
    int NextConnectedNo;
    int PrevConnectedNo;
    vector3 modelRot;      //Obrot pudla w/m swiata
    TDynamicObject* __fastcall ABuScanNearestObject(TTrack *Track, double ScanDir, double ScanDist, int &CouplNr);
    TDynamicObject* __fastcall GetFirstDynamic(int cpl_type);
    TDynamicObject* __fastcall GetFirstCabDynamic(int cpl_type);
    void ABuSetModelShake(vector3 mShake);
    TModel3d *mdLoad;
    TModel3d *mdPrzedsionek;
    TModel3d *mdModel;
    TModel3d *mdKabina; //McZapkie-030303: to z train.h
    TModel3d *mdLowPolyInt; //ABu 010305: wnetrze lowpoly
    bool pcabc1;        //Winger 040304 - zaleznosc pantografu od kabiny
    bool pcabc2;
    double pcabd1;
    double pcabd2;
    double pcabe1;
    double pcabe2;
    bool pcabc1x;
    double lastcabf;
    double StartTime;
    double PantTraction1; //Winger 170204
    double PantTraction2; //Winger 170204
    double PantWysF;      //Winger 180204
    double PantWysR;      //Winger 180204
    double dPantAngleF;  //Winger 160204
    double dPantAngleR;  //Winger 160204
    double dPantAngleFT;  //Winger 170204
    double dPantAngleRT;  //Winger 170204
    double pant1x;      //Winger 010304
    double pant2x;      //Winger 010304
    double panty;       //Winger 010304
    double panth;       //Winger 010304
    double NoVoltTime;
    bool pcp1p;
    bool pcp2p;
    double dDoorMoveL; //NBMX
    double dDoorMoveR; //NBMX


//McZapkie-010302
    TController *Mechanik;
    //TTrainParameters *TrainParams; //Ra: rozk³ad jest na poziomie Driver, a nie pojazdu
    bool MechInside;
    TTrackFollower Axle0; //oœ z przodu (od sprzêgu 0)
    TTrackFollower Axle1; //oœ z ty³u (od sprzêgu 1)
//McZapkie-270202
    bool Controller;
    bool bDisplayCab; //czy wyswietlac kabine w train.cpp
    TTrack *MyTrack; //McZapkie-030303: tor na ktorym stoi, ABu
    AnsiString asBaseDir;
    GLuint ReplacableSkinID[5];  //McZapkie:zmienialne nadwozie
    int iAlpha; //czy tekstura przezroczysta
    __fastcall TDynamicObject();
    __fastcall ~TDynamicObject();
    double __fastcall TDynamicObject::Init
    (//zwraca d³ugoœæ pojazdu albo 0, jeœli b³¹d
     AnsiString Name, AnsiString BaseDir, AnsiString asReplacableSkin, AnsiString Type_Name,
     TTrack *Track, double fDist, AnsiString DriverType, double fVel, AnsiString TrainName,
     float Load, AnsiString LoadType,bool Reversed, AnsiString MoreParams
    );
    void __fastcall AttachPrev(TDynamicObject *Object, int iType= 1);
    bool __fastcall UpdateForce(double dt, double dt1, bool FullVer);
    void __fastcall LoadUpdate();
    bool __fastcall Update(double dt, double dt1);
    bool __fastcall FastUpdate(double dt);
    void __fastcall Move(double fDistance);
    void __fastcall FastMove(double fDistance);
    bool __fastcall Render();
    bool __fastcall RenderAlpha();
    vector3 inline __fastcall GetPosition();
    inline vector3 __fastcall AxlePositionGet() { return iAxleFirst?Axle1.pPosition:Axle0.pPosition; };
    inline vector3 __fastcall GetDirection() { return Axle0.pPosition-Axle1.pPosition; };
    inline double __fastcall GetVelocity() { return MoverParameters->Vel; };
    inline double __fastcall GetLength() { return MoverParameters->Dim.L; };
    inline double __fastcall GetWidth() { return MoverParameters->Dim.W; };
    inline TTrack* __fastcall GetTrack() { return (iAxleFirst?Axle1.GetTrack():Axle0.GetTrack()); };
    //void __fastcall UpdatePos();

 Mover::TMoverParameters *MoverParameters;

 vector3 vUp,vFront,vLeft;
 matrix4x4 mMatrix;
 AnsiString asTrack;
 AnsiString asDestination; //dok¹d pojazd ma byæ kierowany "(stacja):(tor)"
 //McZapkie-260202
 void __fastcall LoadMMediaFile(AnsiString BaseDir, AnsiString TypeName, AnsiString ReplacableSkin);

 inline double __fastcall ABuGetDirection() //ABu.
 {
  return (Axle1.GetTrack()==MyTrack?Axle1.GetDirection():Axle0.GetDirection());
 };
 inline double __fastcall ABuGetTranslation() //ABu.
 {//zwraca przesuniêcie wózka wzglêdem Point1 toru
  return (Axle1.GetTrack()==MyTrack?Axle1.GetTranslation():Axle0.GetTranslation());
 };
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
 void __fastcall RaAxleEvent(TEvent *e);
 TDynamicObject* __fastcall FirstFind(int &coupler_nr);
 float __fastcall GetEPP(); //wyliczanie sredniego cisnienia w PG
 int __fastcall DirectionSet(int d); //ustawienie kierunku w sk³adzie
 int __fastcall DirectionGet() {return iDirection?1:-1;}; //ustawienie kierunku w sk³adzie
 bool DettachDistance(int dir);
 int Dettach(int dir,int cnt);
 TDynamicObject* __fastcall Neightbour(int &dir);
};


//---------------------------------------------------------------------------
#endif
