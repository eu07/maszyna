//---------------------------------------------------------------------------

#ifndef GlobalsH
#define GlobalsH

#include <string>
#include "system.hpp"
#include "opengl/glew.h"
#include "dumb3d.h"
//#include "Classes.h"

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
//const int k_ProgramPause= 47;
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

//const int k_FreeFlyMode= 59;

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
const int k_Battery=72;
const int k_WalkMode=73;
const int MaxKeys= 74;

//klasy dla wska�nik�w globalnych
class TGround;
class TWorld;
class TCamera;
class TDynamicObject;
class TAnimModel; //obiekt terenu
namespace Queryparsercomp
{
class TQueryParserComp; //stary(?) parser
}
class cParser; //nowy (powolny!) parser
class TEvent;

class Global
{
private:
 static GLuint iTextureId; //ostatnio u�yta tekstura 2D
public:
 //double Global::tSinceStart;
 static int Keys[MaxKeys];
 static vector3 pCameraPosition; //pozycja kamery w �wiecie
 static double pCameraRotation;  //kierunek bezwzgl�dny kamery w �wiecie: 0=p�noc, 90�=zach�d (-azymut)
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
 //static bool bRenderAlpha;
 static bool bAdjustScreenFreq;
 static bool bEnableTraction;
 static bool bLoadTraction;
 static float fFriction;
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
 static void __fastcall LoadIniFile(AnsiString asFileName);
 static void __fastcall InitKeys(AnsiString asFileName);
 inline static vector3 __fastcall GetCameraPosition()
 {return pCameraPosition;};
 static void __fastcall SetCameraPosition(vector3 pNewCameraPosition);
 static void __fastcall SetCameraRotation(double Yaw);
 static int iWriteLogEnabled; //maska bitowa: 1-zapis do pliku, 2-okienko
 //McZapkie-221002: definicja swiatla dziennego
 static GLfloat AtmoColor[];
 static GLfloat FogColor[];
 //static bool bTimeChange;
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
 static int iSlowMotion;
 static TDynamicObject *changeDynObj;
 static double ABuDebug;
 static bool detonatoryOK;
 static AnsiString asSky;
 static bool bnewAirCouplers;
 //Ra: nowe zmienne globalne
 static int iDefaultFiltering; //domy�lne rozmywanie tekstur TGA
 static int iBallastFiltering; //domy�lne rozmywanie tekstury podsypki
 static int iRailProFiltering; //domy�lne rozmywanie tekstury szyn
 static int iDynamicFiltering; //domy�lne rozmywanie tekstur pojazd�w
 static int iReCompile; //zwi�kszany, gdy trzeba od�wie�y� siatki
 static bool bUseVBO; //czy jest VBO w karcie graficznej
 static int iFeedbackMode; //tryb pracy informacji zwrotnej
 static int iFeedbackPort; //dodatkowy adres dla informacji zwrotnych
 static double fOpenGL; //wersja OpenGL - przyda si�
 static bool bOpenGL_1_5; //czy s� dost�pne funkcje OpenGL 1.5
 static double fLuminance; //jasno�� �wiat�a do automatycznego zapalania
 static int iMultiplayer; //blokada dzia�ania niekt�rych event�w na rzecz kominikacji
 static HWND hWnd; //uchwyt okna
 static int iCameraLast;
 static AnsiString asRelease; //numer
 static AnsiString asVersion; //z opisem
 static int iViewMode; //co aktualnie wida�: 0-kabina, 1-latanie, 2-sprz�gi, 3-dokumenty, 4-obwody
 static GLint iMaxTextureSize; //maksymalny rozmiar tekstury
 static int iTextMode; //tryb pracy wy�wietlacza tekstowego
 static int iScreenMode[12]; //numer ekranu wy�wietlacza tekstowego
 static bool bDoubleAmbient; //podw�jna jasno�� ambient
 static double fMoveLight; //numer dnia w roku albo -1
 static bool bSmoothTraction; //wyg�adzanie drut�w
 static double fSunDeclination; //deklinacja S�o�ca
 static double fTimeSpeed; //przyspieszenie czasu, zmienna do test�w
 static double fTimeAngleDeg; //godzina w postaci k�ta
 static double fLatitudeDeg; //szeroko�� geograficzna
 static char* szTexturesTGA[4]; //lista tekstur od TGA
 static char* szTexturesDDS[4]; //lista tekstur od DDS
 static int iMultisampling; //tryb antyaliasingu: 0=brak,1=2px,2=4px,3=8px,4=16px
 static bool bGlutFont; //tekst generowany przez GLUT
 static int iKeyLast; //ostatnio naci�ni�ty klawisz w celu logowania
 static int iPause; //globalna pauza ruchu: b0=start,b1=klawisz,b2=t�o,b3=lagi
 static bool bActive; //czy jest aktywnym oknem
 static void __fastcall BindTexture(GLuint t);
 static int iConvertModels; //tworzenie plik�w binarnych
 static int iErorrCounter; //licznik sprawdza� do �ledzenia b��d�w OpenGL
 static bool bInactivePause; //automatyczna pauza, gdy okno nieaktywne
 static int iTextures; //licznik u�ytych tekstur
 static int iSlowMotionMask; //maska wy��czanych w�a�ciwo�ci
 static int iModifyTGA; //czy korygowa� pliki TGA dla szybszego wczytywania
 static bool bHideConsole; //hunter-271211: ukrywanie konsoli
 static TWorld *pWorld; //wska�nik na �wiat do usuwania pojazd�w
 static TAnimModel *pTerrainCompact; //obiekt terenu do ewentualnego zapisania w pliku
 static AnsiString asTerrainModel; //nazwa obiektu terenu do zapisania w pliku
 static bool bRollFix; //czy wykona� przeliczanie przechy�ki
 static Queryparsercomp::TQueryParserComp *qParser;
 static cParser *pParser;
 static int iSegmentsRendered; //ilo�� segment�w do regulacji wydajno�ci
 static double fFpsAverage; //oczekiwana wartos� FPS
 static double fFpsDeviation; //odchylenie standardowe FPS
 static double fFpsMin; //dolna granica FPS, przy kt�rej promie� scenerii b�dzie zmniejszany
 static double fFpsMax; //g�rna granica FPS, przy kt�rej promie� scenerii b�dzie zwi�kszany
 static double fFpsRadiusMax; //maksymalny promie� renderowania
 static int iFpsRadiusMax; //maksymalny promie� renderowania w rozmiarze tabeli sektor�w
 static double fRadiusFactor; //wsp�czynnik zmiany promienia
 static TCamera *pCamera; //parametry kamery
 static TDynamicObject *pUserDynamic; //pojazd u�ytkownika, renderowany bez trz�sienia
 static double fCalibrateIn[6][4]; //parametry kalibracyjne wej�� z pulpitu
 static double fCalibrateOut[7][4]; //parametry kalibracyjne wyj�� dla pulpitu
 static double fBrakeStep; //krok zmiany hamulca dla klawiszy [Num3] i [Num9]
 static bool bJoinEvents; //czy grupowa� eventy o tych samych nazwach
 static bool bSmudge; //czy wy�wietla� smug�, a pojazd u�ytkownika na ko�cu
 //metody
 static void __fastcall TrainDelete(TDynamicObject *d);
 static void __fastcall ConfigParse(Queryparsercomp::TQueryParserComp *qp,cParser *cp=NULL);
 static AnsiString __fastcall GetNextSymbol();
 static TDynamicObject* __fastcall DynamicNearest();
 static bool __fastcall AddToQuery(TEvent *event,TDynamicObject *who);
};

//---------------------------------------------------------------------------
#endif
