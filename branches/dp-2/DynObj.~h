//---------------------------------------------------------------------------

#ifndef DynObjH
#define DynObjH

#include "TrkFoll.h"
#include "Track.h"
#include "QueryParserComp.hpp"
#include "AnimModel.h"
#include "Mover.hpp"
#include "mtable.hpp"
#include "TractionPower.h"
//McZapkie:
#include "MdlMngr.h"
#include "RealSound.h"
#include "AdvSound.h"
#include "Button.h"
#include "AirCoupler.h"

//McZapkie-250202
  const MaxAxles=16; //ABu 280105: zmienione z 8 na 16
  const MaxAnimatedAxles=16; //i to tez.
  const MaxAnimatedDoors=8;  //NBMX  wrzesien 2003

class TDynamicObject
{
private:
    TTrackShape ts;
    TTrackParam tp;
    void ABuLittleUpdate(double ObjSqrDist);
    bool btnOn; //ABu: czy byly uzywane buttony, jesli tak, to po renderingu wylacz

                //bo ten sam model moze byc jeszcze wykorzystany przez inny obiekt!
 double __fastcall ComputeRadius(vector3 p1,vector3 p2,vector3 p3,vector3 p4);
 //vector3 pOldPos1; //Ra: nie u¿ywane
 //vector3 pOldPos4;
 vector3 vPosition; //Ra: pozycja pojazdu liczona zaraz po przesuniêciu
//McZapkie-050402 - do krecenia kolami
    int iAnimatedAxles;
    int iAnimatedDoors;
    double DoorSpeedFactor[MaxAnimatedDoors];
    double tempdoorfactor;
    double tempdoorfactor2;
    double pantspeedfactor;
    TSubModel *smAnimatedWheel[MaxAnimatedAxles];
    TSubModel *smAnimatedDoor[MaxAnimatedDoors];
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
//Winger 160204 - pantografy
    TSubModel *smPatykird1[2];
    TSubModel *smPatykird2[2];
    TSubModel *smPatykirg1[2];
    TSubModel *smPatykirg2[2];
    TSubModel *smPatykisl[2];
    TSubModel *smWiazary[2];
    TSubModel *smWahacze[4];
    double fWahaczeAmp;
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

// Q
    double cab_a_door_lp;
    double cab_a_door_rp;
    double cab_b_door_lp;
    double cab_b_door_rp;

//ABuWozki 060504
    vector3 bogieRot[2];   //Obroty wozkow w/m korpusu
    TSubModel *smBogie[2]; //Wyszukiwanie max 2 wozkow
    void __fastcall ABuBogies();
    void __fastcall ABuModelRoll();
    vector3 modelShake;

    bool renderme; //yB - czy renderowac
    char cp1, sp1, cp2, sp2; //ustawienia wezy
    TRealSound sBrakeAcc; //dzwiek przyspieszacza
 int iAxleFirst; //numer pierwszej oœ w kierunku ruchu
 int iInventory; //flagi bitowe posiadanych submodeli (np. œwiate³)
 //TDynamicObject *NewDynamic; //Ra: nie u¿ywane
 TDynamicObject* __fastcall ABuFindNearestObject(TTrack *Track,TDynamicObject *MyPointer,int &CouplNr);
protected:
    bool bEnabled;

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
 int iLights[2]; //bity zapalonych œwiate³
 double fTrackBlock; //odleg³oœæ do przeszkody do dalszego ruchu
 TDynamicObject* __fastcall Prev();
 TDynamicObject* __fastcall Next();
    void __fastcall SetdMoveLen(double dMoveLen) {MoverParameters->dMoveLen=dMoveLen;}
    void __fastcall ResetdMoveLen() {MoverParameters->dMoveLen=0;}
    double __fastcall GetdMoveLen() {return MoverParameters->dMoveLen;}

    //bool __fastcall EndSignalsLight1Active() {return btEndSignals11.Active();};
    //bool __fastcall EndSignalsLight2Active() {return btEndSignals21.Active();};
    //bool __fastcall EndSignalsLight1oldActive() {return btEndSignals1.Active();};
    //bool __fastcall EndSignalsLight2oldActive() {return btEndSignals2.Active();};
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
    TDynamicObject* GetLastDynamic(int cpl_type);

    TDynamicObject* GetConsist_f(int cpl_type, TDynamicObject *lok);
    TDynamicObject* GetConsist_a(int cpl_type, TDynamicObject *lok);
    TDynamicObject* GetConsist_b(int cpl_type, TDynamicObject *lok);
    TDynamicObject* GET_LIGHTR_P(TDynamicObject *vech);
    TDynamicObject* GET_LIGHTR_D(TDynamicObject *vech);
    TDynamicObject* RENDER_SENSORS(TDynamicObject *vech);

    void ABuSetModelShake(vector3 mShake);
    TModel3d *mdLoad;
    TModel3d *mdPrzedsionek;
    TModel3d *mdModel;
    TModel3d *mdKabina; //McZapkie-030303: to z train.h
    TModel3d *mdLowPolyInt; //ABu 010305: wnetrze lowpoly

    TModel3d *mdDIRECT_TABLE;
    TModel3d *mdSRJ_TABLE;
    TModel3d *mdHASLER_A_A;
    TModel3d *mdHASLER_B_A;
    TModel3d *mdHASLER_A_E;
    TModel3d *mdHASLER_B_E;
    TModel3d *mdCZUWAK_A;
    TModel3d *mdCZUWAK_B;
    TModel3d *mdClock1;
    TModel3d *mdClock2;
    TModel3d *mdFotel1;
    TModel3d *mdFotel2;
    TModel3d *mdFotel3;
    TModel3d *mdFotel4;
    TModel3d *mdVentilator1;
    TModel3d *mdVentilator2;
    TModel3d *mdSRJTABLE1;
    TModel3d *mdSRJTABLE2;
    TModel3d *mdDIRTABLE1;
    TModel3d *mdDIRTABLE2;
    TModel3d *mdComputer;
    TModel3d *mdWycieraczkaAR;
    TModel3d *mdWycieraczkaAL;
    TModel3d *mdWindowNumber;
    TModel3d *mdNastawnikA;
    TModel3d *mdNastawnikB;
    TModel3d *mdKranZasadniczyA;
    TModel3d *mdKranPomocniczyA;
    TModel3d *mdOdbierakA1;
    TModel3d *mdOdbierakA2;
    TModel3d *mdOdbierakB1;
    TModel3d *mdOdbierakB2;
    TModel3d *mdZgarniaczA;
    TModel3d *mdZgarniaczB;
    TModel3d *mdGrzejnikSzybAR;
    TModel3d *mdGrzejnikSzybAL;
    TModel3d *mdStolikA;
    TModel3d *mdStolikB;
    TModel3d *mdMECH;
    TModel3d *mdMECHPOM;
    TModel3d *mdSTATIC01;
    TModel3d *mdSTATIC02;
    TModel3d *mdSTATIC03;
    TModel3d *mdSTATIC04;
    TModel3d *mdSTATIC05;
    TModel3d *mdSTATIC06;
    TModel3d *mdSTATIC07;
    TModel3d *mdSTATIC08;
    TModel3d *mdSTATIC09;
    TModel3d *mdSTATIC10;

    AnsiString SRJ1token;
    AnsiString SRJ2token;
    AnsiString directtabletoken;
    AnsiString roomslightstoken;
    AnsiString associatedtrain;
    
    bool pcabc1;        //Winger 040304 - zaleznosc pantografu od kabiny
    bool pcabc2;
    double pcabd1;
    double pcabd2;
    double pcabe1;
    double pcabe2;
    bool pcabc1x;
    double lastcabf;
    double dWheelAngle;
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
    bool bcab1light;
    bool bcab2light;
    bool headlA_L;
    bool headlA_R;
    bool headlA_U;
    bool headlB_L;
    bool headlB_R;
    bool headlB_U;

    vector3 vLIGHT_R_POS;
    vector3 vLIGHT_R_END;
    vector3 vLIGHT_R_OFF;
    vector3 vLIGHT_R_MOV;
    vector3 vLIGHT_R_SHK;
    vector3 vLIGHT_R_VEL;
    float   vLIGHT_R_SPR;
    float   vLIGHT_R_ROT;


//McZapkie-010302
    TController *Mechanik;
    TTrainParameters *TrainParams;
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
 int __fastcall DirectionSet(int d); //ustawienie kierunku w sk³adzie
 int __fastcall DirectionGet() {return iDirection?1:-1;}; //ustawienie kierunku w sk³adzie
 bool DettachDistance(int dir);
 int Dettach(int dir,int cnt);
};


//---------------------------------------------------------------------------
#endif
