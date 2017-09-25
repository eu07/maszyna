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
//#include <Windows.h>
#include "renderer.h"
#include <GLFW/glfw3.h>
#include <GL/glew.h>
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
int const k_DimHeadlights = 74;
const int MaxKeys = 75;

// klasy dla wskaźników globalnych
class TGround;
class TWorld;
class TCamera;
class TDynamicObject;
class TAnimModel; // obiekt terenu
class cParser; // nowy (powolny!) parser
class TEvent;
class sound;

class TTranscript
{ // klasa obsługująca linijkę napisu do dźwięku
  public:
    float fShow; // czas pokazania
    float fHide; // czas ukrycia/usunięcia
    std::string asText; // tekst gotowy do wyświetlenia (usunięte znaczniki czasu)
    bool bItalic; // czy kursywa (dźwięk nieistotny dla prowadzącego)
};

class TTranscripts
{ // klasa obsługująca napisy do dźwięków
public:
    std::deque<TTranscript> aLines;

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
  public:
    static int Keys[MaxKeys];
    static bool RealisticControlMode; // controls ability to steer the vehicle from outside views
    static Math3D::vector3 pCameraPosition; // pozycja kamery w świecie
    static Math3D::vector3 DebugCameraPosition; // pozycja kamery w świecie
    static double
        pCameraRotation; // kierunek bezwzględny kamery w świecie: 0=północ, 90°=zachód (-azymut)
    static double pCameraRotationDeg; // w stopniach, dla animacji billboard
    static std::vector<Math3D::vector3> FreeCameraInit; // pozycje kamery
    static std::vector<Math3D::vector3> FreeCameraInitAngle;
    static int iWindowWidth;
    static int iWindowHeight;
    static float fDistanceFactor;
    static bool bFullScreen;
    static bool VSync;
    static bool bFreeFly;
    static bool bWireFrame;
    static bool bSoundEnabled;
    // McZapkie-131202
    static bool bAdjustScreenFreq;
    static bool bEnableTraction;
    static bool bLoadTraction;
    static float fFriction;
    static bool bLiveTraction;
    static float fMouseXScale;
    static float fMouseYScale;
    static double fFogStart;
    static double fFogEnd;
    static TGround *pGround;
    static std::string szDefaultExt;
    static std::string SceneryFile;
    static std::string AppName;
    static std::string asCurrentSceneryPath;
    static std::string asCurrentTexturePath;
    static std::string asCurrentDynamicPath;
    // McZapkie-170602: zewnetrzna definicja pojazdu uzytkownika
    static std::string asHumanCtrlVehicle;
    static void LoadIniFile(std::string asFileName);
    static void InitKeys();
    inline static Math3D::vector3 GetCameraPosition() { return pCameraPosition; };
    static void SetCameraPosition(Math3D::vector3 pNewCameraPosition);
    static void SetCameraRotation(double Yaw);
    static int iWriteLogEnabled; // maska bitowa: 1-zapis do pliku, 2-okienko
    static bool MultipleLogs;
    // McZapkie-221002: definicja swiatla dziennego
    static GLfloat FogColor[];
    static float Overcast;

    // TODO: put these things in the renderer
    static float BaseDrawRange;
    static int DynamicLightCount;
    static bool ScaleSpecularValues;
    static bool BasicRenderer;
    static bool RenderShadows;
    static struct shadowtune_t {
        unsigned int map_size;
        float width;
        float depth;
        float distance;
    } shadowtune;
    static bool bUseVBO; // czy jest VBO w karcie graficznej
    static float AnisotropicFiltering; // requested level of anisotropic filtering. TODO: move it to renderer object
    static int ScreenWidth; // current window dimensions. TODO: move it to renderer
    static int ScreenHeight;
    static float ZoomFactor; // determines current camera zoom level. TODO: move it to the renderer
    static float FieldOfView; // vertical field of view for the camera. TODO: move it to the renderer
    static GLint iMaxTextureSize; // maksymalny rozmiar tekstury
    static int iMultisampling; // tryb antyaliasingu: 0=brak,1=2px,2=4px,3=8px,4=16px

    static bool FullPhysics; // full calculations performed for each simulation step
    static int iSlowMotion;
    static TDynamicObject *changeDynObj;
    static double ABuDebug;
    static std::string asSky;
    static bool bnewAirCouplers;
    // Ra: nowe zmienne globalne
    static std::string LastGLError;
    static int iFeedbackMode; // tryb pracy informacji zwrotnej
    static int iFeedbackPort; // dodatkowy adres dla informacji zwrotnych
    static bool InputGamepad; // whether gamepad support is enabled
    static double fLuminance; // jasność światła do automatycznego zapalania
    static float SunAngle; // angle of the sun relative to horizon
    static int iMultiplayer; // blokada działania niektórych eventów na rzecz kominikacji
	static GLFWwindow *window;
	static bool shiftState; //m7todo: brzydko
	static bool ctrlState;
    static int iCameraLast;
    static std::string asVersion; // z opisem
    static bool ControlPicking; // indicates controls pick mode is active
    static bool InputMouse; // whether control pick mode can be activated
    static int iTextMode; // tryb pracy wyświetlacza tekstowego
    static int iScreenMode[12]; // numer ekranu wyświetlacza tekstowego
    static double fMoveLight; // numer dnia w roku albo -1
    static bool FakeLight; // toggle between fixed and dynamic daylight
    static bool bSmoothTraction; // wygładzanie drutów
    static float SplineFidelity; // determines segment size during conversion of splines to geometry
    static double fTimeSpeed; // przyspieszenie czasu, zmienna do testów
    static double fTimeAngleDeg; // godzina w postaci kąta
    static float fClockAngleDeg[6]; // kąty obrotu cylindrów dla zegara cyfrowego
    static double fLatitudeDeg; // szerokość geograficzna
    static std::string szTexturesTGA; // lista tekstur od TGA
    static std::string szTexturesDDS; // lista tekstur od DDS
    static bool DLFont; // switch indicating presence of basic font
    static bool bGlutFont; // tekst generowany przez GLUT
    static int iPause; // globalna pauza ruchu: b0=start,b1=klawisz,b2=tło,b3=lagi,b4=wczytywanie
	static bool bActive;

    static int iConvertModels; // tworzenie plików binarnych
    static bool bInactivePause; // automatyczna pauza, gdy okno nieaktywne
    static int iSlowMotionMask; // maska wyłączanych właściwości
    static bool bHideConsole; // hunter-271211: ukrywanie konsoli
	
    static TWorld *pWorld; // wskaźnik na świat do usuwania pojazdów
    static TAnimModel *pTerrainCompact; // obiekt terenu do ewentualnego zapisania w pliku
    static std::string asTerrainModel; // nazwa obiektu terenu do zapisania w pliku
    static bool bRollFix; // czy wykonać przeliczanie przechyłki
    static cParser *pParser;
    static double fFpsAverage; // oczekiwana wartosć FPS
    static double fFpsDeviation; // odchylenie standardowe FPS
    static double fFpsMin; // dolna granica FPS, przy której promień scenerii będzie zmniejszany
    static double fFpsMax; // górna granica FPS, przy której promień scenerii będzie zwiększany
    static TCamera *pCamera; // parametry kamery
    static TDynamicObject *pUserDynamic; // pojazd użytkownika, renderowany bez trzęsienia
    static double fCalibrateIn[6][6]; // parametry kalibracyjne wejść z pulpitu
    static double fCalibrateOut[7][6]; // parametry kalibracyjne wyjść dla pulpitu
	static double fCalibrateOutMax[7]; // wartości maksymalne wyjść dla pulpitu
	static int iCalibrateOutDebugInfo; // numer wyjścia kalibrowanego dla którego wyświetlać
									   // informacje podczas kalibracji
    static double fBrakeStep; // krok zmiany hamulca dla klawiszy [Num3] i [Num9]
    static bool bJoinEvents; // czy grupować eventy o tych samych nazwach
    static TTranscripts tranTexts; // obiekt obsługujący stenogramy dźwięków na ekranie
    static float4 UITextColor; // base color of UI text
    static std::string asLang; // domyślny język - http://tools.ietf.org/html/bcp47
    static int iHiddenEvents; // czy łączyć eventy z torami poprzez nazwę toru
    static sound *tsRadioBusy[10]; // zajętość kanałów radiowych (wskaźnik na odgrywany dźwięk)
	static int iPoKeysPWM[7]; // numery wejść dla PWM

    //randomizacja
    static std::mt19937 random_engine;

    struct uart_conf_t
    {
        bool enable = false;
        std::string port;
        int baud;
        int interval;
        float updatetime;

        float mainbrakemin = 0.0f;
        float mainbrakemax = 65535.0f;
        float localbrakemin = 0.0f;
        float localbrakemax = 65535.0f;
        float tankmax = 10.0f;
        float tankuart = 65535.0f;
        float pipemax = 10.0f;
        float pipeuart = 65535.0f;
        float brakemax = 10.0f;
        float brakeuart = 65535.0f;
        float hvmax = 100000.0f;
        float hvuart = 65535.0f;
        float currentmax = 10000.0f;
        float currentuart = 65535.0f;
    };
    static uart_conf_t uart_conf;

	// metody
    static void TrainDelete(TDynamicObject *d);
    static void ConfigParse(cParser &parser);
    static std::string GetNextSymbol();
    static TDynamicObject * DynamicNearest();
    static TDynamicObject * CouplerNearest();
    static bool AddToQuery(TEvent *event, TDynamicObject *who);
    static std::string Bezogonkow(std::string str, bool _ = false);
	static double Min0RSpeed(double vel1, double vel2);

	static opengl_light DayLight;

	enum soundmode_t
	{
		linear,
		scaled,
		compat,
	};

	enum soundstopmode_t
	{
		queue,
		playstop,
		stop
	};

	static soundmode_t soundpitchmode;
	static soundmode_t soundgainmode;
	static soundstopmode_t soundstopmode;
};
//---------------------------------------------------------------------------
