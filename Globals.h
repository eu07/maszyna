/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <string>
#include <Windows.h>
#include "opengl/glew.h"
#include "dumb3d.h"

// definicje klawiszy
const int k_IncMainCtrl = 0; //[Num+]
const int k_IncMainCtrlFAST = 1; //[Num+] [Shift]
const int k_DecMainCtrl = 2; //[Num-]
const int k_DecMainCtrlFAST = 3; //[Num-] [Shift]
const int k_IncScndCtrl = 4; //[Num/]
const int k_IncScndCtrlFAST = 5;
const int k_DecScndCtrl = 6;
const int k_DecScndCtrlFAST = 7;
const int k_IncLocalBrakeLevel = 8;
const int k_IncLocalBrakeLevelFAST = 9;
const int k_DecLocalBrakeLevel = 10;
const int k_DecLocalBrakeLevelFAST = 11;
const int k_IncBrakeLevel = 12;
const int k_DecBrakeLevel = 13;
const int k_Releaser = 14;
const int k_EmergencyBrake = 15;
const int k_Brake3 = 16;
const int k_Brake2 = 17;
const int k_Brake1 = 18;
const int k_Brake0 = 19;
const int k_WaveBrake = 20;
const int k_AntiSlipping = 21;
const int k_Sand = 22;

const int k_Main = 23;
const int k_DirectionForward = 24;
const int k_DirectionBackward = 25;

const int k_Fuse = 26;
const int k_Compressor = 27;
const int k_Converter = 28;
const int k_MaxCurrent = 29;
const int k_CurrentAutoRelay = 30;
const int k_BrakeProfile = 31;

const int k_Czuwak = 32;
const int k_Horn = 33;
const int k_Horn2 = 34;

const int k_FailedEngineCutOff = 35;

const int k_MechUp = 36;
const int k_MechDown = 37;
const int k_MechLeft = 38;
const int k_MechRight = 39;
const int k_MechForward = 40;
const int k_MechBackward = 41;

const int k_CabForward = 42;
const int k_CabBackward = 43;

const int k_Couple = 44;
const int k_DeCouple = 45;

const int k_ProgramQuit = 46;
// const int k_ProgramPause= 47;
const int k_ProgramHelp = 48;
// NBMX
const int k_OpenLeft = 49;
const int k_OpenRight = 50;
const int k_CloseLeft = 51;
const int k_CloseRight = 52;
const int k_DepartureSignal = 53;
// NBMX
const int k_PantFrontUp = 54;
const int k_PantRearUp = 55;
const int k_PantFrontDown = 56;
const int k_PantRearDown = 57;

const int k_Heating = 58;

// const int k_FreeFlyMode= 59;

const int k_LeftSign = 60;
const int k_UpperSign = 61;
const int k_RightSign = 62;

const int k_SmallCompressor = 63;

const int k_StLinOff = 64;

const int k_CurrentNext = 65;

const int k_Univ1 = 66;
const int k_Univ2 = 67;
const int k_Univ3 = 68;
const int k_Univ4 = 69;
const int k_EndSign = 70;

const int k_Active = 71;
// Winger 020304
const int k_Battery = 72;
const int k_WalkMode = 73;
const int MaxKeys = 74;

// klasy dla wskaźników globalnych
class TGround;
class TWorld;
class TCamera;
class TDynamicObject;
class TAnimModel; // obiekt terenu
class cParser; // nowy (powolny!) parser
class TEvent;
class TTextSound;

class TTranscript
{ // klasa obsługująca linijkę napisu do dźwięku
  public:
    float fShow; // czas pokazania
    float fHide; // czas ukrycia/usunięcia
    std::string asText; // tekst gotowy do wyświetlenia (usunięte znaczniki czasu)
    bool bItalic; // czy kursywa (dźwięk nieistotny dla prowadzącego)
    int iNext; // następna używana linijka, żeby nie przestawiać fizycznie tabeli
};

/*
#define MAX_TRANSCRIPTS 30
*/
class TTranscripts
{ // klasa obsługująca napisy do dźwięków
/*
    TTranscript aLines[MAX_TRANSCRIPTS]; // pozycje na napisy do wyświetlenia
*/
public:
    std::deque<TTranscript> aLines;
/*
    int iCount; // liczba zajętych pozycji
    int iStart; // pierwsza istotna pozycja w tabeli, żeby sortować przestawiając numerki
*/
private:
    float fRefreshTime;

  public:
    TTranscripts();
    ~TTranscripts();
    void AddLine(std::string const &txt, float show, float hide, bool it);
    // dodanie tekstów, długość dźwięku, czy istotne
    void Add(std::string const &txt, float len, bool background = false);
    // usuwanie niepotrzebnych (ok. 10 razy na sekundę)
    void Update(); 
};

class Global
{
  private:
    static GLuint iTextureId; // ostatnio użyta tekstura 2D
  public:
    // double Global::tSinceStart;
    static int Keys[MaxKeys];
    static Math3D::vector3 pCameraPosition; // pozycja kamery w świecie
    static double
        pCameraRotation; // kierunek bezwzględny kamery w świecie: 0=północ, 90°=zachód (-azymut)
    static double pCameraRotationDeg; // w stopniach, dla animacji billboard
    static Math3D::vector3 pFreeCameraInit[10]; // pozycje kamery
    static Math3D::vector3 pFreeCameraInitAngle[10];
    static int iWindowWidth;
    static int iWindowHeight;
    static float fDistanceFactor;
    static int iBpp;
    static bool bFullScreen;
    static bool bFreeFly;
    // float RunningTime;
    static bool bWireFrame;
    static bool bSoundEnabled;
    // McZapkie-131202
    // static bool bRenderAlpha;
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
    static std::string szDefaultExt;
    static std::string SceneryFile;
    static char CreatorName1[20];
    static char CreatorName2[20];
    static char CreatorName3[20];
    static char CreatorName4[30];
    static char CreatorName5[30];
    static std::string asCurrentSceneryPath;
    static std::string asCurrentTexturePath;
    static std::string asCurrentDynamicPath;
    // McZapkie-170602: zewnetrzna definicja pojazdu uzytkownika
    static std::string asHumanCtrlVehicle;
    static void LoadIniFile(std::string asFileName);
    static void InitKeys(std::string asFileName);
    inline static Math3D::vector3 GetCameraPosition()
    {
        return pCameraPosition;
    };
    static void SetCameraPosition(Math3D::vector3 pNewCameraPosition);
    static void SetCameraRotation(double Yaw);
    static int iWriteLogEnabled; // maska bitowa: 1-zapis do pliku, 2-okienko
    // McZapkie-221002: definicja swiatla dziennego
	static float Background[3];
	static GLfloat AtmoColor[];
    static GLfloat FogColor[];
    // static bool bTimeChange;
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
    static std::string asSky;
    static bool bnewAirCouplers;
    // Ra: nowe zmienne globalne
    static float AnisotropicFiltering; // requested level of anisotropic filtering. TODO: move it to renderer object
    static int iDefaultFiltering; // domyślne rozmywanie tekstur TGA
    static int iBallastFiltering; // domyślne rozmywanie tekstury podsypki
    static int iRailProFiltering; // domyślne rozmywanie tekstury szyn
    static int iDynamicFiltering; // domyślne rozmywanie tekstur pojazdów
    static int iReCompile; // zwiększany, gdy trzeba odświeżyć siatki
    static bool bUseVBO; // czy jest VBO w karcie graficznej
    static int iFeedbackMode; // tryb pracy informacji zwrotnej
    static int iFeedbackPort; // dodatkowy adres dla informacji zwrotnych
    static double fOpenGL; // wersja OpenGL - przyda się
/*
    static bool bOpenGL_1_5; // czy są dostępne funkcje OpenGL 1.5
*/
    static double fLuminance; // jasność światła do automatycznego zapalania
    static int iMultiplayer; // blokada działania niektórych eventów na rzecz kominikacji
    static HWND hWnd; // uchwyt okna
    static int iCameraLast;
    static std::string asRelease; // numer
    static std::string asVersion; // z opisem
    static int
        iViewMode; // co aktualnie widać: 0-kabina, 1-latanie, 2-sprzęgi, 3-dokumenty, 4-obwody
    static GLint iMaxTextureSize; // maksymalny rozmiar tekstury
    static int iTextMode; // tryb pracy wyświetlacza tekstowego
    static int iScreenMode[12]; // numer ekranu wyświetlacza tekstowego
    static bool bDoubleAmbient; // podwójna jasność ambient
    static double fMoveLight; // numer dnia w roku albo -1
    static bool bSmoothTraction; // wygładzanie drutów
    static double fSunDeclination; // deklinacja Słońca
    static double fTimeSpeed; // przyspieszenie czasu, zmienna do testów
    static double fTimeAngleDeg; // godzina w postaci kąta
    static float fClockAngleDeg[6]; // kąty obrotu cylindrów dla zegara cyfrowego
    static double fLatitudeDeg; // szerokość geograficzna
    static std::string szTexturesTGA; // lista tekstur od TGA
    static std::string szTexturesDDS; // lista tekstur od DDS
    static int iMultisampling; // tryb antyaliasingu: 0=brak,1=2px,2=4px,3=8px,4=16px
    static bool bGlutFont; // tekst generowany przez GLUT
    static int iKeyLast; // ostatnio naciśnięty klawisz w celu logowania
    static int iPause; // globalna pauza ruchu: b0=start,b1=klawisz,b2=tło,b3=lagi,b4=wczytywanie
    static bool bActive; // czy jest aktywnym oknem
    static void BindTexture(GLuint t);
    static int iConvertModels; // tworzenie plików binarnych
    static int iErorrCounter; // licznik sprawdzań do śledzenia błędów OpenGL
    static bool bInactivePause; // automatyczna pauza, gdy okno nieaktywne
    static int iTextures; // licznik użytych tekstur
    static int iSlowMotionMask; // maska wyłączanych właściwości
    static int iModifyTGA; // czy korygować pliki TGA dla szybszego wczytywania
    static bool bHideConsole; // hunter-271211: ukrywanie konsoli
	static bool bOldSmudge; // Używanie starej smugi
	
    static TWorld *pWorld; // wskaźnik na świat do usuwania pojazdów
    static TAnimModel *pTerrainCompact; // obiekt terenu do ewentualnego zapisania w pliku
    static std::string asTerrainModel; // nazwa obiektu terenu do zapisania w pliku
    static bool bRollFix; // czy wykonać przeliczanie przechyłki
    static cParser *pParser;
    static int iSegmentsRendered; // ilość segmentów do regulacji wydajności
    static double fFpsAverage; // oczekiwana wartosć FPS
    static double fFpsDeviation; // odchylenie standardowe FPS
    static double fFpsMin; // dolna granica FPS, przy której promień scenerii będzie zmniejszany
    static double fFpsMax; // górna granica FPS, przy której promień scenerii będzie zwiększany
    static double fFpsRadiusMax; // maksymalny promień renderowania
    static int iFpsRadiusMax; // maksymalny promień renderowania w rozmiarze tabeli sektorów
    static double fRadiusFactor; // współczynnik zmiany promienia
    static TCamera *pCamera; // parametry kamery
    static TDynamicObject *pUserDynamic; // pojazd użytkownika, renderowany bez trzęsienia
    static double fCalibrateIn[6][6]; // parametry kalibracyjne wejść z pulpitu
    static double fCalibrateOut[7][6]; // parametry kalibracyjne wyjść dla pulpitu
	static double fCalibrateOutMax[7]; // wartości maksymalne wyjść dla pulpitu
	static int iCalibrateOutDebugInfo; // numer wyjścia kalibrowanego dla którego wyświetlać
									   // informacje podczas kalibracji
    static double fBrakeStep; // krok zmiany hamulca dla klawiszy [Num3] i [Num9]
    static bool bJoinEvents; // czy grupować eventy o tych samych nazwach
    static bool bSmudge; // czy wyświetlać smugę, a pojazd użytkownika na końcu
/*
    static std::string asTranscript[5]; // napisy na ekranie (widoczne)
*/
    static TTranscripts tranTexts; // obiekt obsługujący stenogramy dźwięków na ekranie
    static std::string asLang; // domyślny język - http://tools.ietf.org/html/bcp47
    static int iHiddenEvents; // czy łączyć eventy z torami poprzez nazwę toru
    static TTextSound *tsRadioBusy[10]; // zajętość kanałów radiowych (wskaźnik na odgrywany dźwięk)
	static int iPoKeysPWM[7]; // numery wejść dla PWM

    //randomizacja
    static std::mt19937 random_engine;

	// metody
    static void TrainDelete(TDynamicObject *d);
    static void ConfigParse(cParser &parser);
    static std::string GetNextSymbol();
    static TDynamicObject * DynamicNearest();
    static TDynamicObject * CouplerNearest();
    static bool AddToQuery(TEvent *event, TDynamicObject *who);
    static bool DoEvents();
    static std::string Bezogonkow(std::string str, bool _ = false);
	static double Min0RSpeed(double vel1, double vel2);
	static double CutValueToRange(double min, double value, double max);

    // maciek001: zmienne dla MWD
	static bool bMWDmasterEnable;           // główne włączenie portu COM
	static bool bMWDdebugEnable;            // logowanie pracy
	static int iMWDDebugMode;
	static std::string sMWDPortId;           // nazwa portu COM
	static unsigned long int iMWDBaudrate;  // prędkość transmisji
	static bool bMWDInputEnable;            // włącz wejścia
	static bool bMWDBreakEnable;            // włącz wejścia analogowe (hamulce)
	static double fMWDAnalogInCalib[4][2];  // ustawienia kranów hamulca zespolonego i dodatkowego - min i max
	static double fMWDzg[2];                // max wartość wskazywana i max wartość generowana (rozdzielczość)
	static double fMWDpg[2];
	static double fMWDph[2];
	static double fMWDvolt[2];
	static double fMWDamp[2];
	static int iMWDdivider;
};
//---------------------------------------------------------------------------
