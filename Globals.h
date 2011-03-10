//---------------------------------------------------------------------------

#ifndef GlobalsH
#define GlobalsH

#include <string>
#include "opengl/glew.h"
#include "dumb3d.h"
#include "Logs.h"

using namespace Math3D;

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
public:
//    double Global::tSinceStart;
    static int Keys[MaxKeys];
    static vector3 pCameraPosition; //pozycja kamery w œwiecie
    static double pCameraRotation;  //kierunek bezwzglêdny kamery w œwiecie
    static vector3 pFreeCameraInit[10]; //pozycje kamery
    static vector3 pFreeCameraInitAngle[10];
    static int iWindowWidth;
    static int iWindowHeight;
    static int iBpp;
    static bool bFullScreen;
//    static bool bFreeFly;
    static bool bFreeFly;
//    float RunningTime;
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
    static std::string szDefaultExt;
    static char szSceneryFile[256];
    static char CreatorName1[20];
    static char CreatorName2[20];
    static char CreatorName3[20];
    static char CreatorName4[30];
    static char CreatorName5[30];
    static AnsiString  asCurrentSceneryPath;
    static AnsiString asCurrentTexturePath;
//McZapkie-170602: zewnetrzna definicja pojazdu uzytkownika
    static AnsiString asHumanCtrlVehicle;
    static void __fastcall LoadIniFile(AnsiString asFileName="eu07.ini");
    static void __fastcall InitKeys(AnsiString asFileName="keys.ini");
    static vector3 __fastcall GetCameraPosition();
    static void __fastcall SetCameraPosition(vector3 pNewCameraPosition);
    static void __fastcall SetCameraRotation(double Yaw);
    static bool bWriteLogEnabled;
//McZapkie-221002: definicja swiatla dziennego
    static GLfloat  AtmoColor[];
    static GLfloat  FogColor[];
    static bool bTimeChange;
    static GLfloat  ambientDayLight[];
    static GLfloat  diffuseDayLight[];
    static GLfloat  specularDayLight[];
    static GLfloat  whiteLight[];
    static GLfloat  noLight[];
    static GLfloat  lightPos[4];
    static bool slowmotion;
    static bool changeDynObj;
    static double ABuDebug;
    static bool detonatoryOK;
    static AnsiString asSky;
    static bool bnewAirCouplers;
 static int iDefaultFiltering; //domyœlne rozmywanie tekstur TGA
 static int iBallastFiltering; //domyœlne rozmywanie tekstury podsypki
 static int iRailProFiltering; //domyœlne rozmywanie tekstury szyn
 static int iDynamicFiltering; //domyœlne rozmywanie tekstur pojazdów
 static bool bReCompile; //czy odœwie¿yæ siatki
 static bool bUseVBO; //czy jest VBO w karcie graficznej
 static int iFeedbackMode; //tryb pracy informacji zwrotnej
 static double fOpenGL; //wersja OpenGL - przyda siê
 static bool bOpenGL_1_5; //czy s¹ dostêpne funkcje OpenGL 1.5
 static double fLuminance; //jasnoœæ œwiat³a do automatycznego zapalania
 static bool bMultiplayer; //blokada dzia³ania niektórych eventów na rzecz kominikacji
 static HWND	hWnd; //uchwyt okna
 static int iCameraLast;
 static AnsiString asVersion; 
 static int iViewMode; //co aktualnie widaæ: 0-kabina, 1-latanie, 2-sprzêgi, 3-dokumenty
};

//---------------------------------------------------------------------------
#endif
