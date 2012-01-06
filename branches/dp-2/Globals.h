//---------------------------------------------------------------------------

#ifndef GlobalsH
#define GlobalsH

#include <string>
#include "opengl/glew.h"
#include "dumb3d.h"
#include "Logs.h"

using namespace Math3D;

//Ra: taki zapis funkcjonuje lepiej, ale mo¿e nie jest optymalny
#define vWorldFront vector3(0,0,1)
#define vWorldUp vector3(0,1,0)
#define vWorldLeft CrossProduct(vWorldUp,vWorldFront)

//Ra: bo te poni¿ej to siê powiela³y w ka¿dym module odobno
//vector3 vWorldFront=vector3(0,0,1);
//vector3 vWorldUp=vector3(0,1,0);
//vector3 vWorldLeft=CrossProduct(vWorldUp,vWorldFront);

//definicje klawiszy
const int k_IncMainCtrl= 0; //[Num+]
const int k_IncMainCtrlFAST= 1; //[Num+] [Shift]
const int k_DecMainCtrl= 2; //[Num-]
const int k_DecMainCtrlFAST= 3; //[Num-] [Shift]
const int k_IncScndCtrl= 4; //[Num/]
const int k_IncScndCtrlFAST= 5;
const int k_DecScndCtrl= 6;
const int k_DecScndCtrlFAST= 7;
const int k_IncLocalBrakeLevel= 8;
const int k_IncLocalBrakeLevelFAST= 9;
const int k_DecLocalBrakeLevel=10 ;
const int k_DecLocalBrakeLevelFAST= 11;
const int k_IncBrakeLevel= 12;
const int k_DecBrakeLevel= 13;
const int k_Releaser= 14;
const int k_EmergencyBrake= 15;
const int k_Brake3= 16;
const int k_Brake2= 17;
const int k_Brake1= 18;
const int k_Brake0= 19;
const int k_WaveBrake= 20;
const int k_AntiSlipping= 21;
const int k_Sand= 22;

const int k_Main= 23;
const int k_DirectionForward= 24;
const int k_DirectionBackward= 25;

const int k_Fuse= 26;
const int k_Compressor= 27;
const int k_Converter= 28;
const int k_MaxCurrent= 29;
const int k_CurrentAutoRelay= 30;
const int k_BrakeProfile= 31;

const int k_Czuwak= 32;
const int k_Horn= 33;
const int k_Horn2= 34;

const int k_FailedEngineCutOff= 35;

const int k_MechUp= 36;
const int k_MechDown= 37;
const int k_MechLeft= 38;
const int k_MechRight= 39;
const int k_MechForward= 40;
const int k_MechBackward= 41;

const int k_CabForward= 42;
const int k_CabBackward= 43;

const int k_Couple= 44;
const int k_DeCouple= 45;

const int k_ProgramQuit= 46;
const int k_ProgramPause= 47;
const int k_ProgramHelp= 48;
                        //NBMX
const int k_OpenLeft= 49;
const int k_OpenRight= 50;
const int k_CloseLeft= 51;
const int k_CloseRight= 52;
const int k_DepartureSignal= 53;
                        //NBMX
const int k_PantFrontUp= 54;
const int k_PantRearUp= 55;
const int k_PantFrontDown= 56;
const int k_PantRearDown= 57;

const int k_Heating= 58;

const int k_FreeFlyMode= 59;

const int k_LeftSign = 60;
const int k_UpperSign= 61;
const int k_RightSign= 62;

const int k_SmallCompressor= 63;

const int k_StLinOff= 64;

const int k_CurrentNext=65;

const int k_Univ1=66;
const int k_Univ2=67;
const int k_Univ3=68;
const int k_Univ4=69;
const int k_EndSign=70;

const int k_Active=71;
                        //Winger 020304
const int k_WalkMode= 72;
const int MaxKeys= 73;

//klasy dla wskaŸników globalnych
class TGround;

class Global
{
private:
 static GLuint iTextureId; //ostatnio u¿yta tekstura 2D
public:
 //double Global::tSinceStart;
 static int Keys[MaxKeys];
 static vector3 pCameraPosition; //pozycja kamery w œwiecie
 static vector3 pCameraRot;
 static double pCameraRotation;  //kierunek bezwzglêdny kamery w œwiecie
 static double pCameraRotationDeg;  //w stopniach, dla animacji billboard
 static vector3 pFreeCameraInit[10]; //pozycje kamery
 static vector3 pFreeCameraInitAngle[10];
 static int iWindowWidth;
 static int iWindowHeight;
 static int iBpp;
 static bool bFullScreen;
 static bool bFreeFly;
 //float RunningTime;
 static bool bWireFrame;
 static bool bSoundEnabled;
 //McZapkie-131202
 static bool bRenderAlpha;
 static bool bAdjustScreenFreq;
 static bool bEnableTraction;
 static bool bLoadTraction;
 static bool bLiveTraction;
 static bool bManageNodes;
 static bool bDecompressDDS;
 //    bool WFreeFly;
 static float Global::fMouseXScale;
 static float Global::fMouseYScale;
 static double fFogStart;
 static double fFogEnd;
 static TGround *pGround;
 static char** szDefaultExt;
 static char szSceneryFile[256];
 static char CreatorName1[20];
 static char CreatorName2[20];
 static char CreatorName3[20];
 static char CreatorName4[30];
 static char CreatorName5[30];
 static AnsiString asCurrentSceneryPath;
 static AnsiString asCurrentTexturePath;
 static AnsiString asCurrentDynamicPath;
 //McZapkie-170602: zewnetrzna definicja pojazdu uzytkownika
 static AnsiString asHumanCtrlVehicle;
 static void __fastcall LoadIniFile(AnsiString asFileName="eu07.ini");
 static void __fastcall InitKeys(AnsiString asFileName="keys.ini");
 inline static vector3 __fastcall GetCameraPosition()
 {return pCameraPosition;};
 static void __fastcall SetCameraPosition(vector3 pNewCameraPosition);
 static void __fastcall SetCameraRotation(double Yaw);
 static bool bWriteLogEnabled;
 //McZapkie-221002: definicja swiatla dziennego
 static GLfloat AtmoColor[];
 static GLfloat FogColor[];
 static bool bTimeChange;
 static GLfloat ambientDayLight[];
 static GLfloat diffuseDayLight[];
 static GLfloat specularDayLight[];
 static GLfloat ambientLight[];
 static GLfloat diffuseLight[];
 static GLfloat specularLight[];
 static GLfloat whiteLight[];
 static GLfloat noLight[];
 static GLfloat darkLight[];
 static GLfloat lightPos[4];
 static GLfloat light3Pos[4];
 static GLfloat light4Pos[4];
 static GLfloat light5Pos[4];
 static GLfloat light6Pos[4];
 static GLfloat SUNFLPos[4];
 static int iSlowMotion;
 static bool changeDynObj;
 static double ABuDebug;
 static bool detonatoryOK;
 static AnsiString asSky;
 static AnsiString asSkyH;
 static bool bnewAirCouplers;
 //Ra: nowe zmienne globalne
 static int iDefaultFiltering; //domyœlne rozmywanie tekstur TGA
 static int iBallastFiltering; //domyœlne rozmywanie tekstury podsypki
 static int iRailProFiltering; //domyœlne rozmywanie tekstury szyn
 static int iDynamicFiltering; //domyœlne rozmywanie tekstur pojazdów
 static int iReCompile; //zwiêkszany, gdy trzeba odœwie¿yæ siatki
 static bool bUseVBO; //czy jest VBO w karcie graficznej
 static int iFeedbackMode; //tryb pracy informacji zwrotnej
 static double fOpenGL; //wersja OpenGL - przyda siê
 static bool bOpenGL_1_5; //czy s¹ dostêpne funkcje OpenGL 1.5
 static double fLuminance; //jasnoœæ œwiat³a do automatycznego zapalania
 static int iMultiplayer; //blokada dzia³ania niektórych eventów na rzecz kominikacji
 static HWND hWnd; //uchwyt okna
 static int iCameraLast;
 static AnsiString asVersion;
 static int iViewMode; //co aktualnie widaæ: 0-kabina, 1-latanie, 2-sprzêgi, 3-dokumenty, 4-obwody
 static GLint iMaxTextureSize; //maksymalny rozmiar tekstury
 static int iTextMode; //tryb pracy wyœwietlacza tekstowego
 static bool bDoubleAmbient; //podwójna jasnoœæ ambient
 static double fMoveLight; //numer dnia w roku albo -1
 static bool bSmoothTraction; //wyg³adzanie drutów
 static double fSunDeclination; //deklinacja S³oñca
 static double fSunSpeed; //prêdkoœæ ruchu S³oñca, zmienna do testów
 static double fTimeAngleDeg; //godzina w postaci k¹ta
 static double fLatitudeDeg; //szerokoœæ geograficzna
 static char* szTexturesTGA[4]; //lista tekstur od TGA
 static char* szTexturesDDS[4]; //lista tekstur od DDS
 static int iMultisampling; //tryb antyaliasingu: 0=brak,1=2px,2=4px,3=8px,4=16px
 static bool bGlutFont; //tekst generowany przez GLUT
 static int iKeyLast; //ostatnio naciœniêty klawisz w celu logowania
 static bool bPause; //globalna pauza ruchu
 static bool bActive; //czy jest aktywnym oknem
 static void __fastcall BindTexture(GLuint t);
 static int iConvertModels; //tworzenie plików binarnych
 static int iErorrCounter; //licznik sprawdzañ do œledzenia b³êdów OpenGL
 static bool bInactivePause; //automatyczna pauza, gdy okno nieaktywne
 static int iTextures; //licznik u¿ytych tekstur
 static int iSlowMotionMask; //maska wy³¹czanych w³aœciwoœci
 static int iModifyTGA; //czy korygowaæ pliki TGA dla szybszego wczytywania

// QUEUEDEU VARS ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

 static TStringList *OTHERS;
 static TStringList *CHANGES;
 static TStringList *KEYBOARD;
 static TStringList *CONSISTF;
 static TStringList *CONSISTB;
 static TStringList *CONSISTA;

 static AnsiString g___APPDIR;
 static AnsiString asSSHOTDIR;
 static AnsiString asSSHOTSUBDIR;
 static AnsiString APPVERS;
 static AnsiString APPVER2;
 static AnsiString APPDATE;
 static AnsiString APPDAT2;
 static AnsiString APPSIZE;
 static AnsiString APPCOMP;
 static AnsiString asMechanikVech;
 static AnsiString asCurrentNodeName;
 static AnsiString asCurrentIncName;
 static AnsiString asKeyboardFile;
 static AnsiString gtime;
 static AnsiString timeh;
 static AnsiString timem;
 static AnsiString times;
 static AnsiString asnearestdyn;
 static AnsiString asnearestobj;
 static AnsiString asSCREENSHOTFILE;
 static AnsiString asWINTITLE;

 static bool SCNLOADED;
 static bool bQueuedAdvLog;
 static bool scrfilter;
 static bool bpanview;
 static bool bTUTORIAL;
 static bool bCONSOLE;
 static bool bnearestengaged;
 static bool bnearestobjengaged;
 static bool bfirstloadingscn;
 static bool bloaderbriefing;
 static bool bRenderSky;
 static bool problend;
 static bool makeQ3D;
 static bool seekfacenormal;
 static bool bloadnormals;
 static bool bsemlenses;
 static bool bswitchpod;
 static bool blogincludes;
 static bool bignoreinclude;
 static bool bsun;
 static bool bCabALight;
 static bool bSCREENSHOT;
 static bool bSnowEnabled;
 static bool ventilator1on;
 static bool ventilator2on;

 static GLuint loaderbrief;
 static GLuint loaderlogo;
 static GLuint bfonttex;
 static GLuint texmenu_head1;
 static GLuint texmenu_backg;
 static GLuint texmenu_setti_0;
 static GLuint texmenu_setti_1;
 static GLuint texmenu_start_0;
 static GLuint texmenu_start_1;
 static GLuint texmenu_backw;
 static GLuint texmenu_exitt_0;
 static GLuint texmenu_exitt_1;
 static GLuint texmenu_about_0;
 static GLuint texmenu_about_1;
 static GLuint texmenu_cbox0;
 static GLuint texmenu_cbox1;
 static GLuint texmenu_rbox0;
 static GLuint texmenu_rbox1;
 static GLuint texmenu_leave;
 static GLuint texmenu_okeyy;
 static GLuint texmenu_write;
 static GLuint texmenu_point;
 static GLuint texmenu_editb;
 static GLuint texmenu_lboxa;
 static GLuint texmenu_lboxb;
 static GLuint texmenu_lboxu;
 static GLuint texmenu_lboxd;
 static GLuint texmenu_chcon;
 static GLuint texmenu_adlok;
 static GLuint texsett_backg;
 static GLuint texpointer;
 static GLuint semlense1;
 static GLuint semlense2;
 static GLuint texturetab[16];
 static GLuint texsun1;
 static GLuint texsun2;
 static GLuint texsun3;
 static GLuint m_GlowTexture;
 static GLuint m_BigGlowTexture;
 static GLuint m_HaloTexture;
 static GLuint m_StreakTexture;
 static GLuint texconsole;
 static GLuint texcompass;
 static GLuint texcompassarr;
 static GLuint SCRFILTER;

     // TEKSTURY CYFR I ZNAKOW  (HASLER CYFROWY)
 static GLuint dhcifre_00;
 static GLuint dhcifre_01;
 static GLuint dhcifre_02;
 static GLuint dhcifre_03;
 static GLuint dhcifre_04;
 static GLuint dhcifre_05;
 static GLuint dhcifre_06;
 static GLuint dhcifre_07;
 static GLuint dhcifre_08;
 static GLuint dhcifre_09;

 // 36 ALFANUM CHARS CONTAINER
 static GLuint cichar01;
 static GLuint cichar02;
 static GLuint cichar03;
 static GLuint cichar04;
 static GLuint cichar05;
 static GLuint cichar06;
 static GLuint cichar07;
 static GLuint cichar08;
 static GLuint cichar09;
 static GLuint cichar10;
 static GLuint cichar11;
 static GLuint cichar12;
 static GLuint cichar13;
 static GLuint cichar14;
 static GLuint cichar15;
 static GLuint cichar16;
 static GLuint cichar17;
 static GLuint cichar18;
 static GLuint cichar19;
 static GLuint cichar20;
 static GLuint cichar21;
 static GLuint cichar22;
 static GLuint cichar23;
 static GLuint cichar24;
 static GLuint cichar25;
 static GLuint cichar26;
 static GLuint cichar27;
 static GLuint cichar28;
 static GLuint cichar29;
 static GLuint cichar30;
 static GLuint cichar31;
 static GLuint cichar32;
 static GLuint cichar33;
 static GLuint cichar34;
 static GLuint cichar35;
 static GLuint cichar36;
 static GLuint cichar37;
 static GLuint cichar38;
 static GLuint cichar39;
 static GLuint cichar40;
 static GLuint cichar41;
 static GLuint cichar42;
 static GLuint cichar43;
 static GLuint cichar44;
 static GLuint cichar45;
 static GLuint cichar46;
 static GLuint cichar47;
 static GLuint cichar48;
 static GLuint cichar49;
 static GLuint cichar50;
 static GLuint cichar51;

 // 36 ALFANUM CHARS DIRECT TABLE
 static GLuint dichar01;
 static GLuint dichar02;
 static GLuint dichar03;
 static GLuint dichar04;
 static GLuint dichar05;
 static GLuint dichar06;
 static GLuint dichar07;
 static GLuint dichar08;
 static GLuint dichar09;
 static GLuint dichar10;
 static GLuint dichar11;
 static GLuint dichar12;
 static GLuint dichar13;
 static GLuint dichar14;
 static GLuint dichar15;
 static GLuint dichar16;
 static GLuint dichar17;
 static GLuint dichar18;
 static GLuint dichar19;
 static GLuint dichar20;
 static GLuint dichar21;
 static GLuint dichar22;
 static GLuint dichar23;
 static GLuint dichar24;
 static GLuint dichar25;
 static GLuint dichar26;
 static GLuint dichar27;
 static GLuint dichar28;
 static GLuint dichar29;
 static GLuint dichar30;
 static GLuint dichar31;
 static GLuint dichar32;
 static GLuint dichar33;
 static GLuint dichar34;
 static GLuint dichar35;
 static GLuint dichar36;

 // TEKSTURY CYFR I ZNAKOW  (ZEGAR CYFROWY)
 static GLuint cifre_00;
 static GLuint cifre_01;
 static GLuint cifre_02;
 static GLuint cifre_03;
 static GLuint cifre_04;
 static GLuint cifre_05;
 static GLuint cifre_06;
 static GLuint cifre_07;
 static GLuint cifre_08;
 static GLuint cifre_09;
 static GLuint charx_01;
 static GLuint charx_02;
 static GLuint charx_03;
 static GLuint charx_04;
 static GLuint charx_05;
 static GLuint charx_06;
 static GLuint charx_07;

    // ZEGAR CYFROWY
    static GLuint cdigit01;
    static GLuint cdigit02;
    static GLuint cdigit03;
    static GLuint cdigit04;
    static GLuint cdigit05;
    static GLuint cdigit06;
    static GLuint cdigit07;
    static GLuint cdigit08;
    static GLuint cdigit09;
    static GLuint cdigit10;

 static int iNODES;
 static int iPARSERBYTESPASSED;
 static int iPARSERBYTESTOTAL;
 static int postep;
 static int infotype;
 static int aspectratio;
 static int loaderrefresh;
 static int iSnowFlakesNum;



 static float ffov;
 static float fps;
 static float upd;
 static float ren;
 static float crt;
 static float gtc1;
 static float gtc2;
 static float lsec;
 static float rtim;
 static float cablightint;

 static AnsiString debuginfo1;
 static AnsiString debuginfo2;
 static AnsiString debuginfo3;
 static AnsiString debuginfo4;
 static AnsiString debuginfo5;

 static double panviewtrans;
 static double screenfade;
 static double timepassed;
 static double railmaxdistance;
 static double podsmaxdistance;
 static double SUNROTX;
 static double SUNROTY;
 static double ventilator1acc;
 static double ventilator2acc;

 static float consistlen;
 static float consistmass;
 static float consistcars;
 static float flnearestdyndist;

 static float P[20];

};

//---------------------------------------------------------------------------
#endif
