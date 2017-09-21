/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

*/
#include "stdafx.h"

#include "Globals.h"
#include "usefull.h"
//#include "Mover.h"
#include "Console.h"
#include "Driver.h"
#include "Logs.h"
#include "PyInt.h"
#include "World.h"
#include "parser.h"

// namespace Global {

// parametry do użytku wewnętrznego
// double Global::tSinceStart=0;
TGround *Global::pGround = NULL;
std::string Global::AppName{ "EU07" };
std::string Global::asCurrentSceneryPath = "scenery/";
std::string Global::asCurrentTexturePath = std::string(szTexturePath);
std::string Global::asCurrentDynamicPath = "";
int Global::iSlowMotion = 0; // info o malym FPS: 0-OK, 1-wyłączyć multisampling, 3-promień 1.5km, 7-1km
TDynamicObject *Global::changeDynObj = NULL; // info o zmianie pojazdu
double Global::ABuDebug = 0;
std::string Global::asSky = "1";
double Global::fLuminance = 1.0; // jasność światła do automatycznego zapalania
float Global::SunAngle = 0.0f;
int Global::ScreenWidth = 1;
int Global::ScreenHeight = 1;
float Global::ZoomFactor = 1.0f;
float Global::FieldOfView = 45.0f;
GLFWwindow *Global::window;
bool Global::shiftState;
bool Global::ctrlState;
int Global::iCameraLast = -1;
std::string Global::asVersion = "UNKNOWN";
bool Global::ControlPicking = false; // indicates controls pick mode is enabled
bool Global::InputMouse = true; // whether control pick mode can be activated
int Global::iTextMode = 0; // tryb pracy wyświetlacza tekstowego
int Global::iScreenMode[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // numer ekranu wyświetlacza tekstowego
double Global::fTimeAngleDeg = 0.0; // godzina w postaci kąta
float Global::fClockAngleDeg[6]; // kąty obrotu cylindrów dla zegara cyfrowego
std::string Global::szTexturesTGA = ".tga"; // lista tekstur od TGA
std::string Global::szTexturesDDS = ".dds"; // lista tekstur od DDS
int Global::iPause = 0; // 0x10; // globalna pauza ruchu
bool Global::bActive = true; // czy jest aktywnym oknem
TWorld *Global::pWorld = NULL;
cParser *Global::pParser = NULL;
TCamera *Global::pCamera = NULL; // parametry kamery
TDynamicObject *Global::pUserDynamic = NULL; // pojazd użytkownika, renderowany bez trzęsienia
TTranscripts Global::tranTexts; // obiekt obsługujący stenogramy dźwięków na ekranie
float4 Global::UITextColor = float4( 225.0f / 255.0f, 225.0f / 255.0f, 225.0f / 255.0f, 1.0f );

// parametry scenerii
vector3 Global::pCameraPosition;
vector3 Global::DebugCameraPosition;
double Global::pCameraRotation;
double Global::pCameraRotationDeg;
std::vector<vector3> Global::FreeCameraInit;
std::vector<vector3> Global::FreeCameraInitAngle;
GLfloat Global::FogColor[] = {0.6f, 0.7f, 0.8f};
double Global::fFogStart = 1700;
double Global::fFogEnd = 2000;
float Global::Overcast{ 0.1f }; // NOTE: all this weather stuff should be moved elsewhere
int Global::DynamicLightCount = 7;
bool Global::ScaleSpecularValues = false;
float Global::BaseDrawRange { 2500.f };
bool Global::RenderShadows { false };
bool Global::BasicRenderer { false };
Global::shadowtune_t Global::shadowtune = { 2048, 250.f, 250.f, 500.f };
bool Global::bRollFix = true; // czy wykonać przeliczanie przechyłki
bool Global::bJoinEvents = false; // czy grupować eventy o tych samych nazwach
int Global::iHiddenEvents = 1; // czy łączyć eventy z torami poprzez nazwę toru

// parametry użytkowe (jak komu pasuje)
int Global::Keys[MaxKeys];
bool Global::RealisticControlMode{ false };
int Global::iWindowWidth = 800;
int Global::iWindowHeight = 600;
float Global::fDistanceFactor = Global::iWindowHeight / 768.0; // baza do przeliczania odległości dla LoD
int Global::iFeedbackMode = 1; // tryb pracy informacji zwrotnej
int Global::iFeedbackPort = 0; // dodatkowy adres dla informacji zwrotnych
bool Global::InputGamepad{ true };
bool Global::bFreeFly = false;
bool Global::bFullScreen = false;
bool Global::VSync{ false };
bool Global::bInactivePause = true; // automatyczna pauza, gdy okno nieaktywne
float Global::fMouseXScale = 1.5f;
float Global::fMouseYScale = 0.2f;
std::string Global::SceneryFile = "td.scn";
std::string Global::asHumanCtrlVehicle = "EU07-424";
int Global::iMultiplayer = 0; // blokada działania niektórych funkcji na rzecz komunikacji
double Global::fMoveLight = -1; // ruchome światło
bool Global::FakeLight{ false }; // toggle between fixed and dynamic daylight
double Global::fLatitudeDeg = 52.0; // szerokość geograficzna
float Global::fFriction = 1.0; // mnożnik tarcia - KURS90
double Global::fBrakeStep = 1.0; // krok zmiany hamulca dla klawiszy [Num3] i [Num9]
std::string Global::asLang = "pl"; // domyślny język - http://tools.ietf.org/html/bcp47

// parametry wydajnościowe (np. regulacja FPS, szybkość wczytywania)
bool Global::bAdjustScreenFreq = true;
bool Global::bEnableTraction = true;
bool Global::bLoadTraction = true;
bool Global::bLiveTraction = true;
float Global::AnisotropicFiltering = 8.0f; // requested level of anisotropic filtering. TODO: move it to renderer object
std::string Global::LastGLError;
GLint Global::iMaxTextureSize = 4096; // maksymalny rozmiar tekstury
bool Global::bSmoothTraction = false; // wygładzanie drutów starym sposobem
float Global::SplineFidelity { 1.f }; // determines segment size during conversion of splines to geometry
std::string Global::szDefaultExt = Global::szTexturesDDS; // domyślnie od DDS
int Global::iMultisampling = 2; // tryb antyaliasingu: 0=brak,1=2px,2=4px,3=8px,4=16px
bool Global::DLFont{ false }; // switch indicating presence of basic font
bool Global::bGlutFont = false; // czy tekst generowany przez GLUT32.DLL
//int Global::iConvertModels = 7; // tworzenie plików binarnych, +2-optymalizacja transformów
int Global::iConvertModels{ 0 }; // temporary override, to prevent generation of .e3d not compatible with old exe
int Global::iSlowMotionMask = -1; // maska wyłączanych właściwości dla zwiększenia FPS
// bool Global::bTerrainCompact=true; //czy zapisać teren w pliku
TAnimModel *Global::pTerrainCompact = NULL; // do zapisania terenu w pliku
std::string Global::asTerrainModel = ""; // nazwa obiektu terenu do zapisania w pliku
double Global::fFpsAverage = 20.0; // oczekiwana wartosć FPS
double Global::fFpsDeviation = 5.0; // odchylenie standardowe FPS
double Global::fFpsMin = 30.0; // dolna granica FPS, przy której promień scenerii będzie zmniejszany
double Global::fFpsMax = 65.0; // górna granica FPS, przy której promień scenerii będzie zwiększany
bool Global::FullPhysics { true }; // full calculations performed for each simulation step

// parametry testowe (do testowania scenerii i obiektów)
bool Global::bWireFrame = false;
bool Global::bSoundEnabled = true;
int Global::iWriteLogEnabled = 3; // maska bitowa: 1-zapis do pliku, 2-okienko, 4-nazwy torów
bool Global::MultipleLogs{ false };

// parametry do kalibracji
// kolejno współczynniki dla potęg 0, 1, 2, 3 wartości odczytanej z urządzenia
double Global::fCalibrateIn[6][6] = {{0, 1, 0, 0, 0, 0},
                                     {0, 1, 0, 0, 0, 0},
                                     {0, 1, 0, 0, 0, 0},
                                     {0, 1, 0, 0, 0, 0},
                                     {0, 1, 0, 0, 0, 0},
                                     {0, 1, 0, 0, 0, 0}};
double Global::fCalibrateOut[7][6] = {{0, 1, 0, 0, 0, 0},
                                      {0, 1, 0, 0, 0, 0},
                                      {0, 1, 0, 0, 0, 0},
                                      {0, 1, 0, 0, 0, 0},
                                      {0, 1, 0, 0, 0, 0},
                                      {0, 1, 0, 0, 0, 0},
                                      {0, 1, 0, 0, 0, 0}};
double Global::fCalibrateOutMax[7] = {0, 0, 0, 0, 0, 0, 0};
int Global::iCalibrateOutDebugInfo = -1;
int Global::iPoKeysPWM[7] = {0, 1, 2, 3, 4, 5, 6};
// parametry przejściowe (do usunięcia)
// bool Global::bTimeChange=false; //Ra: ZiomalCl wyłączył starą wersję nocy
// bool Global::bRenderAlpha=true; //Ra: wywaliłam tę flagę
bool Global::bnewAirCouplers = true;
double Global::fTimeSpeed = 1.0; // przyspieszenie czasu, zmienna do testów
bool Global::bHideConsole = false; // hunter-271211: ukrywanie konsoli

Global::uart_conf_t Global::uart_conf;

//randomizacja
std::mt19937 Global::random_engine = std::mt19937(std::time(NULL));

opengl_light Global::DayLight;
Global::soundmode_t Global::soundpitchmode = Global::linear;
Global::soundmode_t Global::soundgainmode = Global::linear;
Global::soundstopmode_t Global::soundstopmode = Global::queue;

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

std::string Global::GetNextSymbol()
{ // pobranie tokenu z aktualnego parsera

    std::string token;
    if (pParser != nullptr)
    {

        pParser->getTokens();
        *pParser >> token;
    };
    return token;
};

void Global::LoadIniFile(std::string asFileName)
{
    FreeCameraInit.resize( 10 );
    FreeCameraInitAngle.resize( 10 );
    cParser parser(asFileName, cParser::buffer_FILE);
    ConfigParse(parser);
};

void Global::ConfigParse(cParser &Parser)
{

    std::string token;
    do
    {

        token = "";
        Parser.getTokens();
        Parser >> token;

        if (token == "sceneryfile")
        {

            Parser.getTokens();
            Parser >> Global::SceneryFile;
        }
        else if (token == "humanctrlvehicle")
        {

            Parser.getTokens();
            Parser >> Global::asHumanCtrlVehicle;
        }
        else if( token == "fieldofview" ) {

            Parser.getTokens( 1, false );
            Parser >> Global::FieldOfView;
            // guard against incorrect values
            Global::FieldOfView = clamp( Global::FieldOfView, 15.0f, 75.0f );
        }
        else if (token == "width")
        {

            Parser.getTokens(1, false);
            Parser >> Global::iWindowWidth;
        }
        else if (token == "height")
        {

            Parser.getTokens(1, false);
            Parser >> Global::iWindowHeight;
        }
        else if (token == "heightbase")
        {

            Parser.getTokens(1, false);
            Parser >> Global::fDistanceFactor;
        }
        else if (token == "fullscreen")
        {

            Parser.getTokens();
            Parser >> Global::bFullScreen;
        }
        else if( token == "vsync" ) {

            Parser.getTokens();
            Parser >> Global::VSync;
        }
        else if (token == "freefly")
        { // Mczapkie-130302

            Parser.getTokens();
            Parser >> Global::bFreeFly;
            Parser.getTokens(3, false);
            Parser >>
                Global::FreeCameraInit[0].x,
                Global::FreeCameraInit[0].y,
                Global::FreeCameraInit[0].z;
        }
        else if (token == "wireframe")
        {

            Parser.getTokens();
            Parser >> Global::bWireFrame;
        }
        else if (token == "debugmode")
        { // McZapkie! - DebugModeFlag uzywana w mover.pas,
            // warto tez blokowac cheaty gdy false
            Parser.getTokens();
            Parser >> DebugModeFlag;
        }
        else if (token == "soundenabled")
        { // McZapkie-040302 - blokada dzwieku - przyda
            // sie do debugowania oraz na komp. bez karty
            // dzw.
            Parser.getTokens();
            Parser >> Global::bSoundEnabled;
        }
        // else if (str==AnsiString("renderalpha")) //McZapkie-1312302 - dwuprzebiegowe renderowanie
        // bRenderAlpha=(GetNextSymbol().LowerCase()==AnsiString("yes"));
        else if (token == "physicslog")
        { // McZapkie-030402 - logowanie parametrow
            // fizycznych dla kazdego pojazdu z maszynista
            Parser.getTokens();
            Parser >> WriteLogFlag;
        }
        else if (token == "fullphysics")
        { // McZapkie-291103 - usypianie fizyki

            Parser.getTokens();
            Parser >> Global::FullPhysics;
        }
        else if (token == "debuglog")
        {
            // McZapkie-300402 - wylaczanie log.txt
            Parser.getTokens();
            Parser >> token;
            if (token == "yes")
            {
                Global::iWriteLogEnabled = 3;
            }
            else if (token == "no")
            {
                Global::iWriteLogEnabled = 0;
            }
            else
            {
                Global::iWriteLogEnabled = stol_def(token,3);
            }
        }
        else if( token == "multiplelogs" ) {
            Parser.getTokens();
            Parser >> Global::MultipleLogs;
        }
        else if( token == "adjustscreenfreq" )
        {
            // McZapkie-240403 - czestotliwosc odswiezania ekranu
            Parser.getTokens();
            Parser >> Global::bAdjustScreenFreq;
        }
        else if (token == "mousescale")
        {
            // McZapkie-060503 - czulosc ruchu myszy (krecenia glowa)
            Parser.getTokens(2, false);
            Parser >> Global::fMouseXScale >> Global::fMouseYScale;
        }
        else if( token == "mousecontrol" ) {
            // whether control pick mode can be activated
            Parser.getTokens();
            Parser >> Global::InputMouse;
        }
        else if (token == "enabletraction")
        {
            // Winger 040204 - 'zywe' patyki dostosowujace sie do trakcji; Ra 2014-03: teraz łamanie
            Parser.getTokens();
            Parser >> Global::bEnableTraction;
        }
        else if (token == "loadtraction")
        {
            // Winger 140404 - ladowanie sie trakcji
            Parser.getTokens();
            Parser >> Global::bLoadTraction;
        }
        else if (token == "friction")
        { // mnożnik tarcia - KURS90

            Parser.getTokens(1, false);
            Parser >> Global::fFriction;
        }
        else if (token == "livetraction")
        {
            // Winger 160404 - zaleznosc napiecia loka od trakcji;
            // Ra 2014-03: teraz prąd przy braku sieci
            Parser.getTokens();
            Parser >> Global::bLiveTraction;
        }
        else if (token == "skyenabled")
        {
            // youBy - niebo
            Parser.getTokens();
            Parser >> token;
            Global::asSky = (token == "yes" ? "1" : "0");
        }
        else if (token == "defaultext")
        {
            // ShaXbee - domyslne rozszerzenie tekstur
            Parser.getTokens();
            Parser >> token;
            if (token == "tga")
            {
                // domyślnie od TGA
                Global::szDefaultExt = Global::szTexturesTGA;
            }
            else {
                Global::szDefaultExt =
                    ( token[ 0 ] == '.' ?
                        token :
                        "." + token );
            }
        }
        else if (token == "newaircouplers")
        {

            Parser.getTokens();
            Parser >> Global::bnewAirCouplers;
        }
        else if( token == "anisotropicfiltering" ) {

            Parser.getTokens( 1, false );
            Parser >> Global::AnisotropicFiltering;
        }
        else if (token == "feedbackmode")
        {

            Parser.getTokens(1, false);
            Parser >> Global::iFeedbackMode;
        }
        else if (token == "feedbackport")
        {

            Parser.getTokens(1, false);
            Parser >> Global::iFeedbackPort;
        }
        else if (token == "multiplayer")
        {

            Parser.getTokens(1, false);
            Parser >> Global::iMultiplayer;
        }
        else if (token == "maxtexturesize")
        {
            // wymuszenie przeskalowania tekstur
            Parser.getTokens(1, false);
            int size;
            Parser >> size;
                 if (size <= 64)   { Global::iMaxTextureSize = 64; }
            else if (size <= 128)  { Global::iMaxTextureSize = 128; }
            else if (size <= 256)  { Global::iMaxTextureSize = 256; }
            else if (size <= 512)  { Global::iMaxTextureSize = 512; }
            else if (size <= 1024) { Global::iMaxTextureSize = 1024; }
            else if (size <= 2048) { Global::iMaxTextureSize = 2048; }
            else if (size <= 4096) { Global::iMaxTextureSize = 4096; }
            else if (size <= 8192) { Global::iMaxTextureSize = 8192; }
            else                   { Global::iMaxTextureSize = 16384; }
        }
        else if (token == "movelight")
        {
            // numer dnia w roku albo -1
            Parser.getTokens(1, false);
            Parser >> Global::fMoveLight;
            if (Global::fMoveLight == 0.f)
            { // pobranie daty z systemu
                std::time_t timenow = std::time(0);
                std::tm *localtime = std::localtime(&timenow);
                Global::fMoveLight = localtime->tm_yday + 1; // numer bieżącego dnia w roku
            }
        }
        else if( token == "dynamiclights" ) {
            // number of dynamic lights in the scene
            Parser.getTokens( 1, false );
            Parser >> Global::DynamicLightCount;
            // clamp the light number
            // max 8 lights per opengl specs, minus one used for sun. at least one light for controlled vehicle
            Global::DynamicLightCount = clamp( Global::DynamicLightCount, 1, 7 ); 
        }
        else if( token == "scalespeculars" ) {
            // whether strength of specular highlights should be adjusted (generally needed for legacy 3d models)
            Parser.getTokens();
            Parser >> Global::ScaleSpecularValues;
        }
        else if( token == "gfxrenderer" ) {
            // shadow render toggle
            std::string gfxrenderer;
            Parser.getTokens();
            Parser >> gfxrenderer;
            Global::BasicRenderer = ( gfxrenderer == "simple" );
        }
        else if( token == "shadows" ) {
            // shadow render toggle
            Parser.getTokens();
            Parser >> Global::RenderShadows;
        }
        else if( token == "shadowtune" ) {
            Parser.getTokens( 4, false );
            Parser
                >> Global::shadowtune.map_size
                >> Global::shadowtune.width
                >> Global::shadowtune.depth
                >> Global::shadowtune.distance;
        }
		else if (token == "soundgainmode")
		{
			Parser.getTokens();
			Parser >> token;
			if (token == "linear")
				Global::soundgainmode = Global::linear;
			else if (token == "scaled")
				Global::soundgainmode = Global::scaled;
			else if (token == "compat")
				Global::soundgainmode = Global::compat;
		}
		else if (token == "soundstopmode")
		{
			Parser.getTokens();
			Parser >> token;
			if (token == "queue")
				Global::soundstopmode = Global::queue;
			else if (token == "playstop")
				Global::soundstopmode = Global::playstop;
			else if (token == "stop")
				Global::soundstopmode = Global::stop;
		}
		else if (token == "soundpitchmode")
		{
			Parser.getTokens();
			Parser >> token;
			if (token == "linear")
				Global::soundpitchmode = Global::linear;
			else if (token == "compat")
				Global::soundpitchmode = Global::compat;
		}
        else if (token == "smoothtraction")
        {
            // podwójna jasność ambient
            Parser.getTokens();
            Parser >> Global::bSmoothTraction;
        }
        else if( token == "splinefidelity" ) {
            // segment size during spline->geometry conversion
            float splinefidelity;
            Parser.getTokens();
            Parser >> splinefidelity;
            Global::SplineFidelity = clamp( splinefidelity, 1.f, 4.f );
        }
        else if (token == "timespeed")
        {
            // przyspieszenie czasu, zmienna do testów
            Parser.getTokens(1, false);
            Parser >> Global::fTimeSpeed;
        }
        else if (token == "multisampling")
        {
            // tryb antyaliasingu: 0=brak,1=2px,2=4px
            Parser.getTokens(1, false);
            Parser >> Global::iMultisampling;
        }
        else if (token == "glutfont")
        {
            // tekst generowany przez GLUT
            Parser.getTokens();
            Parser >> Global::bGlutFont;
        }
        else if (token == "latitude")
        {
            // szerokość geograficzna
            Parser.getTokens(1, false);
            Parser >> Global::fLatitudeDeg;
        }
        else if (token == "convertmodels")
        {
            // tworzenie plików binarnych
            Parser.getTokens(1, false);
            Parser >> Global::iConvertModels;
            // temporary override, to prevent generation of .e3d not compatible with old exe
            Global::iConvertModels =
                ( Global::iConvertModels > 128 ?
                    Global::iConvertModels - 128 :
                    0 );
        }
        else if (token == "inactivepause")
        {
            // automatyczna pauza, gdy okno nieaktywne
            Parser.getTokens();
            Parser >> Global::bInactivePause;
        }
        else if (token == "slowmotion")
        {
            // tworzenie plików binarnych
            Parser.getTokens(1, false);
            Parser >> Global::iSlowMotionMask;
        }
        else if (token == "hideconsole")
        {
            // hunter-271211: ukrywanie konsoli
            Parser.getTokens();
            Parser >> Global::bHideConsole;
        }
        else if (token == "rollfix")
        {
            // Ra: poprawianie przechyłki, aby wewnętrzna szyna była "pozioma"
            Parser.getTokens();
            Parser >> Global::bRollFix;
        }
        else if (token == "fpsaverage")
        {
            // oczekiwana wartość FPS
            Parser.getTokens(1, false);
            Parser >> Global::fFpsAverage;
        }
        else if (token == "fpsdeviation")
        {
            // odchylenie standardowe FPS
            Parser.getTokens(1, false);
            Parser >> Global::fFpsDeviation;
        }
        else if (token == "calibratein")
        {
            // parametry kalibracji wejść
            Parser.getTokens(1, false);
            int in;
            Parser >> in;
            if ((in < 0) || (in > 5))
            {
                in = 5; // na ostatni, bo i tak trzeba pominąć wartości
            }
            Parser.getTokens(4, false);
            Parser
                >> Global::fCalibrateIn[in][0] // wyraz wolny
                >> Global::fCalibrateIn[in][1] // mnożnik
                >> Global::fCalibrateIn[in][2] // mnożnik dla kwadratu
                >> Global::fCalibrateIn[in][3]; // mnożnik dla sześcianu
            Global::fCalibrateIn[in][4] = 0.0; // mnożnik 4 potęgi
            Global::fCalibrateIn[in][5] = 0.0; // mnożnik 5 potęgi
        }
        else if (token == "calibrate5din")
        {
            // parametry kalibracji wejść
            Parser.getTokens(1, false);
            int in;
            Parser >> in;
            if ((in < 0) || (in > 5))
            {
                in = 5; // na ostatni, bo i tak trzeba pominąć wartości
            }
            Parser.getTokens(6, false);
            Parser >> Global::fCalibrateIn[in][0] // wyraz wolny
                >> Global::fCalibrateIn[in][1] // mnożnik
                >> Global::fCalibrateIn[in][2] // mnożnik dla kwadratu
                >> Global::fCalibrateIn[in][3] // mnożnik dla sześcianu
                >> Global::fCalibrateIn[in][4] // mnożnik 4 potęgi
                >> Global::fCalibrateIn[in][5]; // mnożnik 5 potęgi
        }
        else if (token == "calibrateout")
        {
            // parametry kalibracji wyjść
            Parser.getTokens(1, false);
            int out;
            Parser >> out;
            if ((out < 0) || (out > 6))
            {
                out = 6; // na ostatni, bo i tak trzeba pominąć wartości
            }
            Parser.getTokens(4, false);
            Parser >> Global::fCalibrateOut[out][0] // wyraz wolny
                >> Global::fCalibrateOut[out][1] // mnożnik liniowy
                >> Global::fCalibrateOut[out][2] // mnożnik dla kwadratu
                >> Global::fCalibrateOut[out][3]; // mnożnik dla sześcianu
            Global::fCalibrateOut[out][4] = 0.0; // mnożnik dla 4 potęgi
            Global::fCalibrateOut[out][5] = 0.0; // mnożnik dla 5 potęgi
        }
        else if (token == "calibrate5dout")
        {
            // parametry kalibracji wyjść
            Parser.getTokens(1, false);
            int out;
            Parser >> out;
            if ((out < 0) || (out > 6))
            {
                out = 6; // na ostatni, bo i tak trzeba pominąć wartości
            }
            Parser.getTokens(6, false);
            Parser >> Global::fCalibrateOut[out][0] // wyraz wolny
                >> Global::fCalibrateOut[out][1] // mnożnik liniowy
                >> Global::fCalibrateOut[out][2] // mnożnik dla kwadratu
                >> Global::fCalibrateOut[out][3] // mnożnik dla sześcianu
                >> Global::fCalibrateOut[out][4] // mnożnik dla 4 potęgi
                >> Global::fCalibrateOut[out][5]; // mnożnik dla 5 potęgi
        }
        else if (token == "calibrateoutmaxvalues")
        {
            // maksymalne wartości jakie można wyświetlić na mierniku
            Parser.getTokens(7, false);
            Parser >> Global::fCalibrateOutMax[0] >> Global::fCalibrateOutMax[1] >>
                Global::fCalibrateOutMax[2] >> Global::fCalibrateOutMax[3] >>
                Global::fCalibrateOutMax[4] >> Global::fCalibrateOutMax[5] >>
                Global::fCalibrateOutMax[6];
        }
        else if (token == "calibrateoutdebuginfo")
        {
            // wyjście z info o przebiegu kalibracji
            Parser.getTokens(1, false);
            Parser >> Global::iCalibrateOutDebugInfo;
        }
        else if (token == "pwm")
        {
            // zmiana numerów wyjść PWM
            Parser.getTokens(2, false);
            int pwm_out, pwm_no;
            Parser >> pwm_out >> pwm_no;
            Global::iPoKeysPWM[pwm_out] = pwm_no;
        }
        else if (token == "brakestep")
        {
            // krok zmiany hamulca dla klawiszy [Num3] i [Num9]
            Parser.getTokens(1, false);
            Parser >> Global::fBrakeStep;
        }
        else if (token == "joinduplicatedevents")
        {
            // czy grupować eventy o tych samych nazwach
            Parser.getTokens();
            Parser >> Global::bJoinEvents;
        }
        else if (token == "hiddenevents")
        {
            // czy łączyć eventy z torami poprzez nazwę toru
            Parser.getTokens(1, false);
            Parser >> Global::iHiddenEvents;
        }
        else if (token == "pause")
        {
            // czy po wczytaniu ma być pauza?
            Parser.getTokens();
            Parser >> token;
            iPause |= (token == "yes" ? 1 : 0);
        }
        else if (token == "lang")
        {
            // domyślny język - http://tools.ietf.org/html/bcp47
            Parser.getTokens(1, false);
            Parser >> Global::asLang;
        }
        else if( token == "uitextcolor" ) {
            // color of the ui text. NOTE: will be obsolete once the real ui is in place
            Parser.getTokens( 3, false );
            Parser
                >> Global::UITextColor.x
                >> Global::UITextColor.y
                >> Global::UITextColor.z;
            Global::UITextColor.x = clamp( Global::UITextColor.x, 0.0f, 255.0f );
            Global::UITextColor.y = clamp( Global::UITextColor.y, 0.0f, 255.0f );
            Global::UITextColor.z = clamp( Global::UITextColor.z, 0.0f, 255.0f );
            Global::UITextColor = Global::UITextColor / 255.0f;
            Global::UITextColor.w = 1.0f;
        }
        else if( token == "input.gamepad" ) {
            // czy grupować eventy o tych samych nazwach
            Parser.getTokens();
            Parser >> Global::InputGamepad;
        }
        else if (token == "uart")
        {
            Parser.getTokens(3, false);
            Global::uart_conf.enable = true;
            Parser >> Global::uart_conf.port;
            Parser >> Global::uart_conf.baud;
            Parser >> Global::uart_conf.interval;
            Parser >> Global::uart_conf.updatetime;
        }
        else if (token == "uarttune")
        {
            Parser.getTokens(14);
            Parser >> Global::uart_conf.mainbrakemin
                    >> Global::uart_conf.mainbrakemax
                    >> Global::uart_conf.localbrakemin
                    >> Global::uart_conf.localbrakemax
                    >> Global::uart_conf.tankmax
                    >> Global::uart_conf.tankuart
                    >> Global::uart_conf.pipemax
                    >> Global::uart_conf.pipeuart
                    >> Global::uart_conf.brakemax
                    >> Global::uart_conf.brakeuart
                    >> Global::uart_conf.hvmax
                    >> Global::uart_conf.hvuart
                    >> Global::uart_conf.currentmax
                    >> Global::uart_conf.currentuart;
        }
    } while ((token != "") && (token != "endconfig")); //(!Parser->EndOfFile)
    // na koniec trochę zależności
    if (!bLoadTraction) // wczytywanie drutów i słupów
    { // tutaj wyłączenie, bo mogą nie być zdefiniowane w INI
        bEnableTraction = false; // false = pantograf się nie połamie
        bLiveTraction = false; // false = pantografy zawsze zbierają 95% MaxVoltage
    }
    if (iMultisampling)
    { // antyaliasing całoekranowy wyłącza rozmywanie drutów
        bSmoothTraction = false;
    }
    if (iMultiplayer > 0)
    {
        bInactivePause = false; // okno "w tle" nie może pauzować, jeśli włączona komunikacja
        // pauzowanie jest zablokowane dla (iMultiplayer&2)>0, więc iMultiplayer=1 da się zapauzować
        // (tryb instruktora)
    }
/*
    fFpsMin = fFpsAverage -
              fFpsDeviation; // dolna granica FPS, przy której promień scenerii będzie zmniejszany
    fFpsMax = fFpsAverage +
              fFpsDeviation; // górna granica FPS, przy której promień scenerii będzie zwiększany
*/
    if (iPause)
        iTextMode = GLFW_KEY_F1; // jak pauza, to pokazać zegar
    /*  this won't execute anymore with the old parser removed
            // TBD: remove, or launch depending on passed flag?
        if (qp)
    { // to poniżej wykonywane tylko raz, jedynie po wczytaniu eu07.ini*/
#ifdef _WIN32
		    Console::ModeSet(iFeedbackMode, iFeedbackPort); // tryb pracy konsoli sterowniczej
#endif
            /*iFpsRadiusMax = 0.000025 * fFpsRadiusMax *
                        fFpsRadiusMax; // maksymalny promień renderowania 3000.0 -> 225
            if (iFpsRadiusMax > 400)
                iFpsRadiusMax = 400;
            if (fDistanceFactor > 1.0)
            { // dla 1.0 specjalny tryb bez przeliczania
                fDistanceFactor =
                    iWindowHeight /
                fDistanceFactor; // fDistanceFactor>1.0 dla rozdzielczości większych niż bazowa
                fDistanceFactor *=
                    (iMultisampling + 1.0) *
                fDistanceFactor; // do kwadratu, bo większość odległości to ich kwadraty
            }
        }
    */
}

void Global::InitKeys()
{
    Keys[k_IncMainCtrl] = GLFW_KEY_KP_ADD;
    Keys[k_IncMainCtrlFAST] = GLFW_KEY_KP_ADD;
    Keys[k_DecMainCtrl] = GLFW_KEY_KP_SUBTRACT;
    Keys[k_DecMainCtrlFAST] = GLFW_KEY_KP_SUBTRACT;
    Keys[k_IncScndCtrl] = GLFW_KEY_KP_DIVIDE;
    Keys[k_IncScndCtrlFAST] = GLFW_KEY_KP_DIVIDE;
    Keys[k_DecScndCtrl] = GLFW_KEY_KP_MULTIPLY;
    Keys[k_DecScndCtrlFAST] = GLFW_KEY_KP_MULTIPLY;

    Keys[k_IncLocalBrakeLevel] = GLFW_KEY_KP_1;
    Keys[k_DecLocalBrakeLevel] = GLFW_KEY_KP_7;
    Keys[k_IncBrakeLevel] = GLFW_KEY_KP_3;
    Keys[k_DecBrakeLevel] = GLFW_KEY_KP_9;
    Keys[k_Releaser] = GLFW_KEY_KP_6;
    Keys[k_EmergencyBrake] = GLFW_KEY_KP_0;
    Keys[k_Brake3] = GLFW_KEY_KP_8;
    Keys[k_Brake2] = GLFW_KEY_KP_5;
    Keys[k_Brake1] = GLFW_KEY_KP_2;
    Keys[k_Brake0] = GLFW_KEY_KP_4;
    Keys[k_WaveBrake] = GLFW_KEY_KP_DECIMAL;

    Keys[k_AntiSlipping] = GLFW_KEY_KP_ENTER;
    Keys[k_Sand] = 'S';
    Keys[k_Main] = 'M';
    Keys[k_Active] = 'W';
    Keys[k_Battery] = 'J';
    Keys[k_DirectionForward] = 'D';
    Keys[k_DirectionBackward] = 'R';
    Keys[k_Fuse] = 'N';
    Keys[k_Compressor] = 'C';
    Keys[k_Converter] = 'X';
    Keys[k_MaxCurrent] = 'F';
    Keys[k_CurrentAutoRelay] = 'G';
    Keys[k_BrakeProfile] = 'B';
    Keys[k_CurrentNext] = 'Z';

    Keys[k_Czuwak] = ' ';
    Keys[k_Horn] = 'A';
    Keys[k_Horn2] = 'A';

    Keys[k_FailedEngineCutOff] = 'E';

    Keys[k_MechUp] = GLFW_KEY_PAGE_UP;
    Keys[k_MechDown] = GLFW_KEY_PAGE_DOWN;
    Keys[k_MechLeft] = GLFW_KEY_LEFT;
    Keys[k_MechRight] = GLFW_KEY_RIGHT;
    Keys[k_MechForward] = GLFW_KEY_UP;
    Keys[k_MechBackward] = GLFW_KEY_DOWN;

    Keys[k_CabForward] = GLFW_KEY_HOME;
    Keys[k_CabBackward] = GLFW_KEY_END;

    Keys[k_Couple] = GLFW_KEY_INSERT;
    Keys[k_DeCouple] = GLFW_KEY_DELETE;

    Keys[k_ProgramQuit] = GLFW_KEY_F10;
    Keys[k_ProgramHelp] = GLFW_KEY_F1;
    Keys[k_WalkMode] = GLFW_KEY_F5;

    Keys[k_OpenLeft] = ',';
    Keys[k_OpenRight] = '.';
    Keys[k_CloseLeft] = ',';
    Keys[k_CloseRight] = '.';
    Keys[k_DepartureSignal] = '/';

    // Winger 160204 - obsluga pantografow
    Keys[k_PantFrontUp] = 'P'; // Ra: zamieniony przedni z tylnym
    Keys[k_PantFrontDown] = 'P';
    Keys[k_PantRearUp] = 'O';
    Keys[k_PantRearDown] = 'O';
    // Winger 020304 - ogrzewanie
	Keys[k_Heating] = 'H';
    // headlights
	Keys[k_LeftSign] = 'Y';
    Keys[k_UpperSign] = 'U';
    Keys[k_RightSign] = 'I';
    Keys[k_DimHeadlights] = 'L';
    // tail lights
    Keys[k_EndSign] = 'T';

    Keys[k_SmallCompressor] = 'V';
    Keys[k_StLinOff] = 'L';
    // ABu 090305 - przyciski uniwersalne, do roznych bajerow :)
    Keys[k_Univ1] = '[';
    Keys[k_Univ2] = ']';
    Keys[k_Univ3] = ';';
    Keys[k_Univ4] = '\'';
}

/*
vector3 Global::GetCameraPosition()
{
    return pCameraPosition;
}
*/
void Global::SetCameraPosition(vector3 pNewCameraPosition)
{
    pCameraPosition = pNewCameraPosition;
}

void Global::SetCameraRotation(double Yaw)
{ // ustawienie bezwzględnego kierunku kamery z korekcją do przedziału <-M_PI,M_PI>
    pCameraRotation = Yaw;
    while (pCameraRotation < -M_PI)
        pCameraRotation += 2 * M_PI;
    while (pCameraRotation > M_PI)
        pCameraRotation -= 2 * M_PI;
    pCameraRotationDeg = pCameraRotation * 180.0 / M_PI;
}

void Global::TrainDelete(TDynamicObject *d)
{ // usunięcie pojazdu prowadzonego przez użytkownika
    if (pWorld)
        pWorld->TrainDelete(d);
};

TDynamicObject *Global::DynamicNearest()
{ // ustalenie pojazdu najbliższego kamerze
    return pGround->DynamicNearest(pCamera->Pos);
};

TDynamicObject *Global::CouplerNearest()
{ // ustalenie pojazdu najbliższego kamerze
    return pGround->CouplerNearest(pCamera->Pos);
};

bool Global::AddToQuery(TEvent *event, TDynamicObject *who)
{
    return pGround->AddToQuery(event, who);
};
//---------------------------------------------------------------------------

TTranscripts::TTranscripts()
{
/*
    iCount = 0; // brak linijek do wyświetlenia
    iStart = 0; // wypełniać od linijki 0
    for (int i = 0; i < MAX_TRANSCRIPTS; ++i)
    { // to do konstruktora można by dać
        aLines[i].fHide = -1.0; // wolna pozycja (czas symulacji, 360.0 to doba)
        aLines[i].iNext = -1; // nie ma kolejnej
    }
*/
    fRefreshTime = 360.0; // wartośc zaporowa
};
TTranscripts::~TTranscripts(){};

void TTranscripts::AddLine(std::string const &txt, float show, float hide, bool it)
{ // dodanie linii do tabeli, (show) i (hide) w [s] od aktualnego czasu
    if (show == hide)
        return; // komentarz jest ignorowany
    show = Global::fTimeAngleDeg + show / 240.0; // jeśli doba to 360, to 1s będzie równe 1/240
    hide = Global::fTimeAngleDeg + hide / 240.0;

    TTranscript transcript;
    transcript.asText = txt;
    transcript.fShow = show;
    transcript.fHide = hide;
    transcript.bItalic = it;
    aLines.emplace_back( transcript );
    // set the next refresh time while at it
    // TODO, TBD: sort the transcript lines? in theory, they should be coming arranged in the right order anyway
    // short of cases with multiple sounds overleaping
    fRefreshTime = aLines.front().fHide;
/*
    int i = iStart, j, k; // od czegoś trzeba zacząć
    while ((aLines[i].iNext >= 0) ? (aLines[aLines[i].iNext].fShow <= show) :
                                    false) // póki nie koniec i wcześniej puszczane
        i = aLines[i].iNext; // przejście do kolejnej linijki
*/
/*
    //(i) wskazuje na linię, po której należy wstawić dany tekst, chyba że
    while (txt ? *txt : false)
        for (j = 0; j < MAX_TRANSCRIPTS; ++j)
            if (aLines[j].fHide < 0.0)
            { // znaleziony pierwszy wolny
                aLines[j].iNext = aLines[i].iNext; // dotychczasowy następny będzie za nowym
                if (aLines[iStart].fHide < 0.0) // jeśli tablica jest pusta
                    iStart = j; // fHide trzeba sprawdzić przed ewentualnym nadpisaniem, gdy i=j=0
                else
                    aLines[i].iNext = j; // a nowy będzie za tamtym wcześniejszym
                aLines[j].fShow = show; // wyświetlać od
                aLines[j].fHide = hide; // wyświetlać do
                aLines[j].bItalic = it;
                aLines[j].asText = std::string(txt); // bez sensu, wystarczyłby wskaźnik
                if ((k = aLines[j].asText.find("|")) != std::string::npos)
                { // jak jest podział linijki na wiersze
                    aLines[j].asText = aLines[j].asText.substr(0, k - 1);
                    txt += k;
                    i = j; // kolejna linijka dopisywana będzie na koniec właśnie dodanej
                }
                else
                    txt = NULL; // koniec dodawania
                if (fRefreshTime > show) // jeśli odświeżacz ustawiony jest na później
                    fRefreshTime = show; // to odświeżyć wcześniej
                break; // więcej już nic
            }
*/
};
void TTranscripts::Add(std::string const &txt, float len, bool backgorund)
{ // dodanie tekstów, długość dźwięku, czy istotne
    if (true == txt.empty())
        return; // pusty tekst
/*
    int i = 0, j = int(0.5 + 10.0 * len); //[0.1s]
    if (*txt == '[')
    { // powinny być dwa nawiasy
        while (*++txt ? *txt != ']' : false)
            if ((*txt >= '0') && (*txt <= '9'))
                i = 10 * i + int(*txt - '0'); // pierwsza liczba aż do ]
        if (*txt ? *++txt == '[' : false)
        {
            j = 0; // drugi nawias określa czas zakończenia wyświetlania
            while (*++txt ? *txt != ']' : false)
                if ((*txt >= '0') && (*txt <= '9'))
                    j = 10 * j + int(*txt - '0'); // druga liczba aż do ]
            if (*txt)
                ++txt; // pominięcie drugiego ]
        }
    }
*/
    std::string asciitext{ txt }; win1250_to_ascii( asciitext ); // TODO: launch relevant conversion table based on language
    cParser parser( asciitext );
    while( true == parser.getTokens( 3, false, "[]\n" ) ) {

        float begin, end;
        std::string transcript;
        parser
            >> begin
            >> end
            >> transcript;
        AddLine( transcript, 0.10 * begin, 0.12 * end, false );
    }
    // try to handle malformed(?) cases with no show/hide times
    std::string transcript; parser >> transcript;
    while( false == transcript.empty() ) {

        WriteLog( "Transcript text with no display/hide times: \"" + transcript + "\"" );
        AddLine( transcript, 0.0, 0.12 * transcript.size(), false );
        transcript = ""; parser >> transcript;
    }
};
void TTranscripts::Update()
{ // usuwanie niepotrzebnych (nie częściej niż 10 razy na sekundę)
    if( Global::fTimeAngleDeg < fRefreshTime )
        return; // nie czas jeszcze na zmiany
    // czas odświeżenia można ustalić wg tabelki, kiedy coś się w niej zmienia
 //   fRefreshTime = Global::fTimeAngleDeg + 360.0; // wartość zaporowa

    while( ( false == aLines.empty() )
        && ( Global::fTimeAngleDeg >= aLines.front().fHide ) ) {
        // remove expired lines
        aLines.pop_front();
    }
    // update next refresh time
    if( false == aLines.empty() ) { fRefreshTime = aLines.front().fHide; }
    else                          { fRefreshTime = 360.0f; }
/*
    int i = iStart, j = -1; // od czegoś trzeba zacząć
    bool change = false; // czy zmieniać napisy?
    do
    {
        if (aLines[i].fHide >= 0.0) // o ile aktywne
            if (aLines[i].fHide < Global::fTimeAngleDeg)
            { // gdy czas wyświetlania upłynął
                aLines[i].fHide = -1.0; // teraz będzie wolną pozycją
                if (i == iStart)
                    iStart = aLines[i].iNext >= 0 ? aLines[i].iNext : 0; // przestawienie pierwszego
                else if (j >= 0)
                    aLines[j].iNext = aLines[i].iNext; // usunięcie ze środka
                change = true;
            }
            else
            { // gdy ma być pokazane
                if (aLines[i].fShow > Global::fTimeAngleDeg) // będzie pokazane w przyszłości
                    if (fRefreshTime > aLines[i].fShow) // a nie ma nic wcześniej
                        fRefreshTime = aLines[i].fShow;
                if (fRefreshTime > aLines[i].fHide)
                    fRefreshTime = aLines[i].fHide;
            }
        // można by jeszcze wykrywać, które nowe mają być pokazane
        j = i;
        i = aLines[i].iNext; // kolejna linijka
    } while (i >= 0); // póki po tablicy
    change = true; // bo na razie nie ma warunku, że coś się dodało
    if (change)
    { // aktualizacja linijek ekranowych
        i = iStart;
        j = -1;
        do
        {
            if (aLines[i].fHide > 0.0) // jeśli nie ukryte
                if (aLines[i].fShow < Global::fTimeAngleDeg) // to dodanie linijki do wyświetlania
                    if (j < 5 - 1) // ograniczona liczba linijek
                        Global::asTranscript[++j] = aLines[i].asText; // skopiowanie tekstu
            i = aLines[i].iNext; // kolejna linijka
        } while (i >= 0); // póki po tablicy
        for (++j; j < 5; ++j)
            Global::asTranscript[j] = ""; // i czyszczenie nieużywanych linijek
    }
*/
};

// Ra: tymczasowe rozwiązanie kwestii zagranicznych (czeskich) napisów
char bezogonkowo[] = "E?,?\"_++?%S<STZZ?`'\"\".--??s>stzz"
                        " ^^L$A|S^CS<--RZo±,l'uP.,as>L\"lz"
                     "RAAAALCCCEEEEIIDDNNOOOOxRUUUUYTB"
                     "raaaalccceeeeiiddnnoooo-ruuuuyt?";

std::string Global::Bezogonkow(std::string str, bool _)
{ // wycięcie liter z ogonkami, bo OpenGL nie umie wyświetlić
    for (unsigned int i = 1; i < str.length(); ++i)
        if (str[i] & 0x80)
            str[i] = bezogonkowo[str[i] & 0x7F];
        else if (str[i] < ' ') // znaki sterujące nie są obsługiwane
            str[i] = ' ';
        else if (_)
            if (str[i] == '_') // nazwy stacji nie mogą zawierać spacji
                str[i] = ' '; // więc trzeba wyświetlać inaczej
    return str;
};

double Global::Min0RSpeed(double vel1, double vel2)
{ // rozszerzenie funkcji Min0R o wartości -1.0
    if (vel1 == -1.0)
    {
        vel1 = std::numeric_limits<double>::max();
    }
    if (vel2 == -1.0)
    {
        vel2 = std::numeric_limits<double>::max();
    }
    return std::min(vel1, vel2);
};
