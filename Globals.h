/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "Classes.h"
#include "Camera.h"
#include "dumb3d.h"
#include "Float3d.h"
#include "light.h"
#include "uart.h"
#include "utilities.h"
#include "motiontelemetry.h"
#include "version.h"

struct global_settings {
// members
    // data items
    // TODO: take these out of the settings
    bool shiftState{ false }; //m7todo: brzydko
    bool ctrlState{ false };
    bool altState{ false };
    std::mt19937 random_engine{ std::mt19937( static_cast<unsigned int>( std::time( NULL ) ) ) };
    TDynamicObject *changeDynObj{ nullptr };// info o zmianie pojazdu
    TCamera pCamera; // parametry kamery
    TCamera pDebugCamera;
    std::array<Math3D::vector3, 10> FreeCameraInit; // pozycje kamery
    std::array<Math3D::vector3, 10> FreeCameraInitAngle;
    int iCameraLast{ -1 };
    int iSlowMotion{ 0 }; // info o malym FPS: 0-OK, 1-wyłączyć multisampling, 3-promień 1.5km, 7-1km
    basic_light DayLight;
    float SunAngle{ 0.f }; // angle of the sun relative to horizon
    double fLuminance{ 1.0 }; // jasność światła do automatycznego zapalania
    double fTimeAngleDeg{ 0.0 }; // godzina w postaci kąta
    float fClockAngleDeg[ 6 ]; // kąty obrotu cylindrów dla zegara cyfrowego
    std::string LastGLError;
    float ZoomFactor{ 1.f }; // determines current camera zoom level. TODO: move it to the renderer
    bool CabWindowOpen{ false }; // controls sound attenuation between cab and outside
    bool ControlPicking{ true }; // indicates controls pick mode is active
    bool DLFont{ false }; // switch indicating presence of basic font
    bool bActive{ true }; // czy jest aktywnym oknem
    int iPause{ 0 }; // globalna pauza ruchu: b0=start,b1=klawisz,b2=tło,b3=lagi,b4=wczytywanie
    float AirTemperature{ 15.f };
    std::string asCurrentSceneryPath{ "scenery/" };
    std::string asCurrentTexturePath{ szTexturePath };
    std::string asCurrentDynamicPath;
    // settings
    // filesystem
    bool bLoadTraction{ true };
    bool CreateSwitchTrackbeds { true };
    std::string szTexturesTGA{ ".tga" }; // lista tekstur od TGA
    std::string szTexturesDDS{ ".dds" }; // lista tekstur od DDS
    std::string szDefaultExt{ szTexturesDDS };
    std::string SceneryFile{ "td.scn" };
    std::string asHumanCtrlVehicle{ "EU07-424" };
    int iConvertModels{ 0 }; // tworzenie plików binarnych
    // logs
    int iWriteLogEnabled{ 3 }; // maska bitowa: 1-zapis do pliku, 2-okienko, 4-nazwy torów
    bool MultipleLogs{ false };
    unsigned int DisabledLogTypes{ 0 };
    // simulation
    bool RealisticControlMode{ false }; // controls ability to steer the vehicle from outside views
    bool bEnableTraction{ true };
    float fFriction{ 1.f }; // mnożnik tarcia - KURS90
    float FrictionWeatherFactor { 1.f };
    bool bLiveTraction{ true };
    float Overcast{ 0.1f }; // NOTE: all this weather stuff should be moved elsewhere
    glm::vec3 FogColor = { 0.6f, 0.7f, 0.8f };
    double fFogStart{ 1700 };
    double fFogEnd{ 2000 };
    std::string Season{}; // season of the year, based on simulation date
    std::string Weather{ "cloudy:" }; // current weather
    bool FullPhysics{ true }; // full calculations performed for each simulation step
    bool bnewAirCouplers{ true };
    double fMoveLight{ -1 }; // numer dnia w roku albo -1
    bool FakeLight{ false }; // toggle between fixed and dynamic daylight
    double fTimeSpeed{ 1.0 }; // przyspieszenie czasu, zmienna do testów
    double fLatitudeDeg{ 52.0 }; // szerokość geograficzna
    float ScenarioTimeOverride { std::numeric_limits<float>::quiet_NaN() }; // requested scenario start time
    float ScenarioTimeOffset { 0.f }; // time shift (in hours) applied to train timetables
    bool ScenarioTimeCurrent { false }; // automatic time shift to match scenario time with local clock
    bool bInactivePause{ true }; // automatyczna pauza, gdy okno nieaktywne
    int iSlowMotionMask{ -1 }; // maska wyłączanych właściwości
    bool bHideConsole{ false }; // hunter-271211: ukrywanie konsoli
    bool bRollFix{ true }; // czy wykonać przeliczanie przechyłki
    bool bJoinEvents{ false }; // czy grupować eventy o tych samych nazwach
    int iHiddenEvents{ 1 }; // czy łączyć eventy z torami poprzez nazwę toru
    // ui
    int PythonScreenUpdateRate { 200 }; // delay between python-based screen updates, in milliseconds
    int iTextMode{ 0 }; // tryb pracy wyświetlacza tekstowego
    glm::vec4 UITextColor { glm::vec4( 225.f / 255.f, 225.f / 255.f, 225.f / 255.f, 1.f ) }; // base color of UI text
    float UIBgOpacity { 0.65f }; // opacity of ui windows
    std::string asLang{ "pl" }; // domyślny język - http://tools.ietf.org/html/bcp47
    // gfx
    int iWindowWidth{ 800 };
    int iWindowHeight{ 600 };

    float fDistanceFactor{ iWindowHeight / 768.f }; // baza do przeliczania odległości dla LoD
    bool bFullScreen{ false };
    bool VSync{ false };
    bool bWireFrame{ false };
    bool bAdjustScreenFreq{ true };
    float BaseDrawRange{ 2500.f };
    int DynamicLightCount{ 8 };
    bool ScaleSpecularValues{ true };
    struct shadowtune_t {
        unsigned int map_size{ 2048 };
        float width{ 250.f }; // no longer used
        float depth{ 250.f };
        float distance{ 500.f }; // no longer used
    } shadowtune;
    int ReflectionUpdatesPerSecond{ static_cast<int>( 1000 / ( 1.0 / 300.0 ) ) };
    float AnisotropicFiltering{ 8.f }; // requested level of anisotropic filtering. TODO: move it to renderer object
    float FieldOfView{ 45.f }; // vertical field of view for the camera. TODO: move it to the renderer
    GLint iMaxTextureSize{ 4096 }; // maksymalny rozmiar tekstury
    int iMultisampling{ 2 }; // tryb antyaliasingu: 0=brak,1=2px,2=4px,3=8px,4=16px
    float SplineFidelity{ 1.f }; // determines segment size during conversion of splines to geometry
    bool ResourceSweep{ true }; // gfx resource garbage collection
    bool ResourceMove{ false }; // gfx resources are moved between cpu and gpu side instead of sending a copy
    bool compress_tex{ true }; // all textures are compressed on gpu side
    std::string asSky{ "1" };
    double fFpsAverage{ 20.0 }; // oczekiwana wartosć FPS
    double fFpsDeviation{ 5.0 }; // odchylenie standardowe FPS
    double fFpsMin{ 30.0 }; // dolna granica FPS, przy której promień scenerii będzie zmniejszany
    double fFpsMax{ 65.0 }; // górna granica FPS, przy której promień scenerii będzie zwiększany
    // audio
    bool bSoundEnabled{ true };
    float AudioVolume{ 1.25f };
	int audio_max_sources = 30;
    std::string AudioRenderer;
    // input
    float fMouseXScale{ 1.5f };
    float fMouseYScale{ 0.2f };
    int iFeedbackMode{ 1 }; // tryb pracy informacji zwrotnej
    int iFeedbackPort{ 0 }; // dodatkowy adres dla informacji zwrotnych
    bool InputGamepad{ true }; // whether gamepad support is enabled
    bool InputMouse{ true }; // whether control pick mode can be activated
    double fBrakeStep { 1.0 }; // krok zmiany hamulca innych niż FV4a dla klawiszy [Num3] i [Num9]
    double brake_speed { 3.0 }; // prędkość przesuwu hamulca dla FV4a
    // parametry kalibracyjne wejść z pulpitu
    double fCalibrateIn[ 6 ][ 6 ] = {
        { 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0 } };
    // parametry kalibracyjne wyjść dla pulpitu
    double fCalibrateOut[ 7 ][ 6 ] = {
        { 0, 1, 0, 0, 0, 0 },
        { 0, 1, 0, 0, 0, 0 },
        { 0, 1, 0, 0, 0, 0 },
        { 0, 1, 0, 0, 0, 0 },
        { 0, 1, 0, 0, 0, 0 },
        { 0, 1, 0, 0, 0, 0 },
        { 0, 1, 0, 0, 0, 0 } };
    // wartości maksymalne wyjść dla pulpitu
    double fCalibrateOutMax[ 7 ] = {
        0, 0, 0, 0, 0, 0, 0 };
    int iCalibrateOutDebugInfo { -1 }; // numer wyjścia kalibrowanego dla którego wyświetlać informacje podczas kalibracji
    int iPoKeysPWM[ 7 ] = { 0, 1, 2, 3, 4, 5, 6 }; // numery wejść dla PWM
    uart_input::conf_t uart_conf;
    // multiplayer
    int iMultiplayer{ 0 }; // blokada działania niektórych eventów na rzecz kominikacji
    // other
    std::string AppName{ "EU07" };
    std::string asVersion{ VERSION_INFO }; // z opisem
	motiontelemetry::conf_t motiontelemetry_conf;
	std::string screenshot_dir;
	bool loading_log = true;
	bool dds_upper_origin = false;
    bool captureonstart = true;

	bool python_mipmaps = true;
	bool python_displaywindows = false;
	bool python_threadedupload = true;
	bool map_enabled = true;

    int gfx_framebuffer_width = -1;
    int gfx_framebuffer_height = -1;
    bool gfx_shadowmap_enabled = true;
    bool gfx_envmap_enabled = true;
    bool gfx_postfx_motionblur_enabled = true;
    float gfx_postfx_motionblur_shutter = 0.01f;
    GLenum gfx_postfx_motionblur_format = GL_RG16F;
    GLenum gfx_format_color = GL_RGB16F;
    GLenum gfx_format_depth = GL_DEPTH_COMPONENT32F;
    bool gfx_skippipeline = false;
    bool gfx_shadergamma = false;
    bool gfx_usegles = false;

// methods
    void LoadIniFile( std::string asFileName );
    void ConfigParse( cParser &parser );
};

extern global_settings Global;
