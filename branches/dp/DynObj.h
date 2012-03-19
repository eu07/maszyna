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
 const ANIM_TYPES=7; //Ra: ilo�� typ�w animacji
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
{//wsp�czynniki do animacji parowozu
};

class TAnim
{//klasa animowanej cz�ci pojazdu (ko�a, drzwi, pantografy, burty, nap�d parowozu, si�owniki itd.)
 union
 {
  TSubModel *smAnimated; //animowany submodel
  TSubModel **smElement; //je�li animowanych element�w jest wi�cej (pantograf, nap�d parowozu)
 };
 union
 {//kolejno: t�oczysko, korbow�d, dr��ek, jarzmo, trzon suwaka, wahacz, dr. wahacza, dr. wahacza
  TAnimValveGear *pValveGear; //wsp�czynniki do animacji parowozu
  double *pWheelAngle; //wska�nik na k�t obrotu osi
  struct
  {//dla
  };
 };
 int iFlags; //flagi animacji
public:
 int __fastcall TypeSet(int i); //ustawienie typu
 void __fastcall Parovoz(); //wykonanie oblicze� animacji
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

class TDynamicObject
{//klasa pojazdu
private:
 vector3 vPosition; //Ra: pozycja pojazdu liczona zaraz po przesuni�ciu
 //Ra: sprzatam animacje w poje�dzie
 int iAnimType[ANIM_TYPES]; //0-osie,1-wi�zary,2-w�zki,3-wahacze,4-pantografy,5-drzwi,6-t�oki
 int iAnimations; //og�lna ilo�� obiekt�w animuj�cych
 TAnim *pAnimations; //obiekty animuj�ce
 TSubModel **pAnimated; //lista animowanych submodeli (mo�e by� ich wi�cej ni� obiekt�w animuj�cych)
 double dWheelAngle[3]; //k�ty obrotu k�: 0=przednie toczne, 1=nap�dzaj�ce i wi�zary, 2=tylne toczne
private:
//Ra: pocz�tek animacji do ogarni�cia
 //McZapkie-050402 - do krecenia kolami
 int iAnimatedAxles; //ilo�� u�ywanych (kr�conych) osi
 TSubModel *smAnimatedWheel[MaxAnimatedAxles]; //submodele poszczeg�lnych osi
 double *pWheelAngle[MaxAnimatedAxles]; //wska�niki do odczytu k�ta obrotu danej osi
 //wi�zary
 TSubModel *smWiazary[2]; //mo�na zast�pi� je osiami
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
//Ra: koneic animacji do ogarni�cia
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
    char cp1, sp1, cp2, sp2; //ustawienia w�y
    TRealSound sBrakeAcc; //d�wi�k przyspieszacza
 int iAxleFirst; //numer pierwszej osi w kierunku ruchu
 int iInventory; //flagi bitowe posiadanych submodeli (np. �wiate�)
 TDynamicObject* __fastcall ABuFindNearestObject(TTrack *Track,TDynamicObject *MyPointer,int &CouplNr);
 void __fastcall TurnOff();
public:
 bool bEnabled; //Ra: wyjecha� na portal i ma by� usuni�ty
protected:

    //TTrackFollower Axle2; //dwie osie z czterech (te s� protected)
    //TTrackFollower Axle3; //Ra: wy��czy�em, bo k�ty s� liczone w Segment.cpp
    int iNumAxles; //ilo�� osi
    //Byte NextConnectedNo;
    //Byte PrevConnectedNo;
    int CouplCounter;
    AnsiString asModel;
    int iDirection; //kierunek wzgl�dem czo�a sk�adu (1=zgodny,0=przeciwny)
    void ABuScanObjects(int ScanDir,double ScanDist);
    void __fastcall ABuCheckMyTrack();

public:
 float fHalfMaxAxleDist; //rozstaw w�zk�w albo osi
 float fShade; //0:normalnie, -1:w ciemno�ci, +1:dodatkowe �wiat�o (brak koloru?)
 int iLights[2]; //bity zapalonych �wiate�
 double fTrackBlock; //odleg�o�� do przeszkody do dalszego ruchu
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

    TDynamicObject *NextConnected; //pojazd pod��czony od strony sprz�gu 1 (kabina -1)
    TDynamicObject *PrevConnected; //pojazd pod��czony od strony sprz�gu 0 (kabina 1)
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
    //TTrainParameters *TrainParams; //Ra: rozk�ad jest na poziomie Driver, a nie pojazdu
    bool MechInside;
    TTrackFollower Axle0; //o� z przodu (od sprz�gu 0)
    TTrackFollower Axle1; //o� z ty�u (od sprz�gu 1)
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
    (//zwraca d�ugo�� pojazdu albo 0, je�li b��d
     AnsiString Name, AnsiString BaseDir, AnsiString asReplacableSkin, AnsiString Type_Name,
     TTrack *Track, double fDist, AnsiString DriverType, double fVel, AnsiString TrainName,
     float Load, AnsiString LoadType,bool Reversed
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
 AnsiString asDestination; //dok�d pojazd ma by� kierowany "(stacja):(tor)"
 //McZapkie-260202
 void __fastcall LoadMMediaFile(AnsiString BaseDir, AnsiString TypeName, AnsiString ReplacableSkin);

 inline double __fastcall ABuGetDirection() //ABu.
 {
  return (Axle1.GetTrack()==MyTrack?Axle1.GetDirection():Axle0.GetDirection());
 };
 inline double __fastcall ABuGetTranslation() //ABu.
 {//zwraca przesuni�cie w�zka wzgl�dem Point1 toru
  return (Axle1.GetTrack()==MyTrack?Axle1.GetTranslation():Axle0.GetTranslation());
 };
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
 void __fastcall RaAxleEvent(TEvent *e);
 TDynamicObject* __fastcall FirstFind(int &coupler_nr);
 int __fastcall DirectionSet(int d); //ustawienie kierunku w sk�adzie
 int __fastcall DirectionGet() {return iDirection?1:-1;}; //ustawienie kierunku w sk�adzie
 bool DettachDistance(int dir);
 int Dettach(int dir,int cnt);
 TDynamicObject* __fastcall Neightbour(int &dir);
};


//---------------------------------------------------------------------------
#endif
