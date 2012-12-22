//---------------------------------------------------------------------------

#ifndef DynObjH
#define DynObjH

#include "TrkFoll.h"
#include "Track.h"
#include "QueryParserComp.hpp"
#include "AnimModel.h"
#include "Mover.hpp"
#include "ai_driver.hpp"
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
    bool EndTrack; //Info o tym, czy jest koniec trasy
    TTrackShape ts;
    TTrackParam tp;
    void ABuLittleUpdate(double ObjSqrDist);
    bool btnOn; //ABu: czy byly uzywane buttony, jesli tak, to po renderingu wylacz
                //bo ten sam model moze byc jeszcze wykorzystany przez inny obiekt!
    double __fastcall ComputeRadius(vector3 p1, vector3 p2, vector3 p3, vector3 p4);
    vector3 pOldPos1;
    vector3 pOldPos4;
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
    TRealSound rsPrzekladnia;
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
    //ABuWozki 060504
    vector3 bogieRot[2];   //Obroty wozkow w/m korpusu
    TSubModel *smBogie[2]; //Wyszukiwanie max 2 wozkow
    void __fastcall ABuBogies();
    void __fastcall ABuModelRoll();
    vector3 modelShake;

    bool renderme; //yB - czy renderowac
    char cp1, sp1, cp2, sp2; //ustawienia wezy
    TRealSound sBrakeAcc; //dzwiek przyspieszacza
    
protected:
    bool bEnabled;
    AnsiString asName;
    TTrackFollower Axle2;
    TTrackFollower Axle3;
    int iNumAxles;
    //Byte NextConnectedNo;
    //Byte PrevConnectedNo;
    int CouplCounter;
    AnsiString asModel;
    void ScanEventTrack(TTrack *Track);
    void ABuScanObjects(TTrack *Track, double ScanDir, double ScanDist);
    void __fastcall ABuCheckMyTrack();

public:
    void __fastcall SetdMoveLen(double dMoveLen) {MoverParameters->dMoveLen=dMoveLen;}
    void __fastcall ResetdMoveLen() {MoverParameters->dMoveLen=0;}
    double __fastcall GetdMoveLen() {return MoverParameters->dMoveLen;}

    bool __fastcall EndSignalsLight1Active() {return btEndSignals11.Active();};
    bool __fastcall EndSignalsLight2Active() {return btEndSignals21.Active();};
    bool __fastcall EndSignalsLight1oldActive() {return btEndSignals1.Active();};
    bool __fastcall EndSignalsLight2oldActive() {return btEndSignals2.Active();};
    int __fastcall GetPneumatic(bool front, bool red);
    void __fastcall SetPneumatic(bool front, bool red);

    AnsiString __fastcall GetasName()
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

    TDynamicObject *NextConnected;
    TDynamicObject *PrevConnected;
    Byte NextConnectedNo;
    Byte PrevConnectedNo;
    vector3 modelRot;      //Obrot pudla w/m swiata
    TDynamicObject* ABuScanNearestObject(TTrack *Track, double ScanDir, double ScanDist, int &CouplNr);
    TDynamicObject* GetFirstDynamic(int cpl_type);
    TDynamicObject* GetLastDynamic(int cpl_type);
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


//McZapkie-010302
    TController *Mechanik;
    TTrainParameters *TrainParams;
    bool MechInside;
    TTrackFollower Axle1;
    TTrackFollower Axle4;
//McZapkie-270202
    bool Controller;
    bool bDisplayCab; //czy wyswietlac kabine w train.cpp
    TTrack *MyTrack; //McZapkie-030303: tor na ktorym stoi, ABu
    AnsiString asBaseDir;
    GLuint ReplacableSkinID;  //McZapkie:zmienialne nadwozie
    __fastcall TDynamicObject();
    __fastcall ~TDynamicObject();
    bool __fastcall TDynamicObject::Init(AnsiString Name, AnsiString BaseDir, AnsiString asReplacableSkin, AnsiString Type_Name,
                                     TTrack *Track, double fDist, AnsiString DriverType, double fVel, AnsiString TrainName, int Load, AnsiString LoadType);
    void __fastcall AttachPrev(TDynamicObject *Object, int iType= 1);
    bool __fastcall UpdateForce(double dt, double dt1, bool FullVer);
    bool __fastcall Update(double dt, double dt1);
    bool __fastcall FastUpdate(double dt);
    void __fastcall Move(double fDistance);
    void __fastcall FastMove(double fDistance);
    bool __fastcall Render();
    bool __fastcall RenderAlpha();
    vector3 inline __fastcall GetPosition();
    inline vector3 __fastcall GetDirection() { return Axle4.pPosition-Axle1.pPosition; };
    inline double __fastcall GetVelocity() { return MoverParameters->Vel; };
    inline double __fastcall GetLength() { return MoverParameters->Dim.L; };
    inline double __fastcall GetWidth() { return MoverParameters->Dim.W; };
    inline TTrack* __fastcall GetTrack() { return (MoverParameters->ActiveDir<0?Axle1.GetTrack():Axle4.GetTrack()); };
    void __fastcall UpdatePos();    

    TMoverParameters *MoverParameters;

    vector3 vUp,vFront,vLeft;
    matrix4x4 mMatrix;
    AnsiString asTrack;

    //McZapkie-260202
    void __fastcall LoadMMediaFile(AnsiString BaseDir, AnsiString TypeName, AnsiString ReplacableSkin);

    inline double __fastcall ABuGetDirection() //ABu.
           {
              return (Axle1.GetTrack()==MyTrack?Axle1.GetDirection():Axle4.GetDirection());
           };
    inline double __fastcall ABuGetTranslation() //ABu.
           {
              return (Axle1.GetTrack()==MyTrack?Axle1.GetTranslation():Axle4.GetTranslation());
           };
    void CouplersDettach(int MinDist, double MyScanDir);

};


//---------------------------------------------------------------------------
#endif
