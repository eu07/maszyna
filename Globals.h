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
#include "utilities.h"
#include "motiontelemetry.h"
#ifdef WITH_UART
#include "uart.h"
#endif
#ifdef WITH_ZMQ
#include "zmq_input.h"
#endif

struct global_settings {
// members
    // data items
    // TODO: take these out of the settings
    bool shiftState{ false }; //m7todo: brzydko
    bool ctrlState{ false };
    bool altState{ false };
    std::mt19937 random_engine;
    std::mt19937 local_random_engine;
    bool ready_to_load{ false };
    std::time_t starting_timestamp = 0; // starting time, in local timezone
    uint32_t random_seed = 0;
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
    int CurrentMaxTextureSize{ 4096 };
    bool UpdateMaterials{ true };
    // settings
    // filesystem
    bool bLoadTraction{ true };
    bool bSmoothTraction{ true };
    bool CreateSwitchTrackbeds{ true };
    std::string szTexturesTGA{ ".tga" }; // lista tekstur od TGA
    std::string szTexturesDDS{ ".dds" }; // lista tekstur od DDS
    std::string szDefaultExt{ szTexturesDDS };
	std::string SceneryFile;
    std::string local_start_vehicle{ "EU07-424" };
    int iConvertModels{ 0 }; // tworzenie plików binarnych
    int iConvertIndexRange{ 1000 }; // range of duplicate vertex scan
    bool file_binary_terrain{ true }; // enable binary terrain (de)serialization
    // logs
    int iWriteLogEnabled{ 3 }; // maska bitowa: 1-zapis do pliku, 2-okienko, 4-nazwy torów
    bool MultipleLogs{ false };
    unsigned int DisabledLogTypes{ 0 };
    bool ParserLogIncludes{ false };
    // simulation
    bool RealisticControlMode{ false }; // controls ability to steer the vehicle from outside views
    bool bEnableTraction{ true };
    float fFriction{ 1.f }; // mnożnik tarcia - KURS90
    float FrictionWeatherFactor{ 1.f };
    bool bLiveTraction{ true };
    float Overcast{ 0.1f }; // NOTE: all this weather stuff should be moved elsewhere
    glm::vec3 FogColor = { 0.6f, 0.7f, 0.8f };
    float fFogEnd{ 7500 };
    std::string Season{}; // season of the year, based on simulation date
    std::string Weather{ "cloudy:" }; // current weather
    std::string Period{}; // time of the day, based on sun position
    bool FullPhysics{ true }; // full calculations performed for each simulation step
    bool bnewAirCouplers{ true };
    float fMoveLight{ 0.f }; // numer dnia w roku albo -1
    bool FakeLight{ false }; // toggle between fixed and dynamic daylight
    double fTimeSpeed{ 1.0 }; // przyspieszenie czasu, zmienna do testów
	double default_timespeed { 1.0 }; // timescale loaded from config
    double fLatitudeDeg{ 52.0 }; // szerokość geograficzna
    float ScenarioTimeOverride{ std::numeric_limits<float>::quiet_NaN() }; // requested scenario start time
    float ScenarioTimeOffset{ 0.f }; // time shift (in hours) applied to train timetables
    bool ScenarioTimeCurrent{ false }; // automatic time shift to match scenario time with local clock
    bool bInactivePause{ true }; // automatyczna pauza, gdy okno nieaktywne
    int iSlowMotionMask{ -1 }; // maska wyłączanych właściwości
    bool bHideConsole{ false }; // hunter-271211: ukrywanie konsoli
    bool bRollFix{ true }; // czy wykonać przeliczanie przechyłki
    bool bJoinEvents{ false }; // czy grupować eventy o tych samych nazwach
    int iHiddenEvents{ 1 }; // czy łączyć eventy z torami poprzez nazwę toru
    bool AITrainman{ true }; // virtual assistant performing consist coupling/decoupling and other maintenance tasks
    // ui
    int PythonScreenUpdateRate{ 200 }; // delay between python-based screen updates, in milliseconds
    int iTextMode{ 0 }; // tryb pracy wyświetlacza tekstowego
    glm::vec4 UITextColor{ glm::vec4( 225.f / 255.f, 225.f / 255.f, 225.f / 255.f, 1.f ) }; // base color of UI text
    float UIBgOpacity{ 0.65f }; // opacity of ui windows
    std::string asLang{ "pl" }; // domyślny język - http://tools.ietf.org/html/bcp47
    // gfx
    glm::ivec2 window_size; // main window size in platform-specific virtual pixels
    glm::ivec2 cursor_pos; // cursor position in platform-specific virtual pixels
    glm::ivec2 fb_size; // main window framebuffer size

    float fDistanceFactor{ 1.f }; // baza do przeliczania odległości dla LoD
    float targetfps{ 0.0f };
    bool bFullScreen{ false };
    bool VSync{ false };
    bool bWireFrame{ false };
    bool bAdjustScreenFreq{ true };
    float BaseDrawRange{ 2500.f };
    int DynamicLightCount{ 7 };
    bool ScaleSpecularValues{ true };
    std::string GfxRenderer{ "default" };
    bool LegacyRenderer{ false };
    bool BasicRenderer{ false };
    bool RenderShadows{ true };
    int RenderCabShadowsRange{ 0 };
    struct shadowtune_t {
        unsigned int map_size{ 2048 };
        float range{ 250.f };
    } shadowtune;
    struct reflectiontune_t {
        double update_interval{ 300.0 };
        int fidelity{ 0 }; // 0: sections, 1: +static models, 2: +vehicles
        float range_instances{ 750.f };
        float range_vehicles{ 250.f };
    } reflectiontune;
    bool bUseVBO{ true }; // czy jest VBO w karcie graficznej (czy użyć)
    float AnisotropicFiltering{ 8.f }; // requested level of anisotropic filtering. TODO: move it to renderer object
    float FieldOfView{ 45.f }; // vertical field of view for the camera. TODO: move it to the renderer
    GLint iMaxTextureSize{ 4096 }; // maksymalny rozmiar tekstury
    GLint iMaxCabTextureSize{ 4096 }; // largest allowed texture in vehicle cab
    int iMultisampling{ 2 }; // tryb antyaliasingu: 0=brak,1=2px,2=4px,3=8px,4=16px
    float SplineFidelity{ 1.f }; // determines segment size during conversion of splines to geometry
    bool Smoke{ true }; // toggles smoke simulation and visualization
    float SmokeFidelity{ 1.f }; // determines amount of generated smoke particles
    bool ResourceSweep{ true }; // gfx resource garbage collection
    bool ResourceMove{ false }; // gfx resources are moved between cpu and gpu side instead of sending a copy
    bool compress_tex{ true }; // all textures are compressed on gpu side
    std::string asSky{ "1" };
    float fFpsAverage{ 0.f }; // oczekiwana wartosć FPS
    float fFpsDeviation{ 5.f }; // odchylenie standardowe FPS
    double fFpsMin{ 30.0 }; // dolna granica FPS, przy której promień scenerii będzie zmniejszany
    double fFpsMax{ 65.0 }; // górna granica FPS, przy której promień scenerii będzie zwiększany
    // audio
    bool bSoundEnabled{ true };
    float AudioVolume{ 1.f };
    float RadioVolume{ 0.75f };
    float VehicleVolume{ 1.0f };
    float EnvironmentPositionalVolume{ 1.0f };
    float EnvironmentAmbientVolume{ 1.0f };
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
    int iCalibrateOutDebugInfo{ -1 }; // numer wyjścia kalibrowanego dla którego wyświetlać informacje podczas kalibracji
    int iPoKeysPWM[ 7 ] = { 0, 1, 2, 3, 4, 5, 6 }; // numery wejść dla PWM
#ifdef WITH_UART
    uart_input::conf_t uart_conf;
    std::map<std::string, bool *> uartfeatures_map = {
        {"main", &uart_conf.mainenable},
        {"scnd", &uart_conf.scndenable},
        {"train", &uart_conf.trainenable},
        {"local", &uart_conf.localenable},
        {"radiovolume", &uart_conf.radiovolumeenable},
        {"radiochannel", &uart_conf.radiochannelenable}
    };
#endif
#ifdef WITH_ZMQ
    std::string zmq_address;
#endif
    // multiplayer
    int iMultiplayer{ 0 }; // blokada działania niektórych eventów na rzecz kominikacji
	bool bIsolatedTrainName{ false }; //wysyłanie zajęcia odcinka izolowanego z nazwą pociągu
    // other
    std::string AppName{ "EU07" };
    std::string asVersion; // z opisem
	motiontelemetry::conf_t motiontelemetry_conf;
	std::string screenshot_dir;
	bool loading_log = true;
	bool dds_upper_origin = false;
    bool captureonstart = true;
	bool render_cab = true;
	bool crash_damage = true;
	bool gui_defaultwindows = true;
	bool gui_showtranscripts = true;
    bool gui_trainingdefault = false;

	std::string extcam_cmd;
	std::string extcam_rec;
	glm::ivec2 extcam_res{800, 600};

	std::chrono::duration<float> minframetime {0.0f};
    std::string fullscreen_monitor;
    bool fullscreen_windowed{ false };

    bool python_enabled = true;
	bool python_mipmaps = true;
	bool python_displaywindows = false;
	bool python_threadedupload = true;
	bool python_vsync = true;
	bool python_sharectx = true;
	bool python_uploadmain = true;
	std::chrono::duration<float> python_minframetime {0.01f};

    bool gfx_skiprendering = false;
    int gfx_framebuffer_width = -1;
    int gfx_framebuffer_height = -1;
    bool gfx_shadowmap_enabled = true;
    bool gfx_envmap_enabled = true;
    bool gfx_postfx_motionblur_enabled = true;
    float gfx_postfx_motionblur_shutter = 0.01f;
    GLenum gfx_postfx_motionblur_format = GL_RG16F;
    GLenum gfx_format_color = GL_RGB16F;
    GLenum gfx_format_depth = GL_DEPTH_COMPONENT32F;
    bool gfx_postfx_chromaticaberration_enabled = true;
    bool gfx_skippipeline = false;
    bool gfx_extraeffects = true;
    bool gfx_shadergamma = false;
    bool gfx_usegles = false;
    std::string gfx_angleplatform;
    bool gfx_gldebug = false;
    bool vr = false;
    std::string vr_backend;

    float gfx_distance_factor_max { 3.f };
    float gfx_shadow_angle_min { -0.2f };
    int gfx_shadow_rank_cutoff { 3 };

    float ui_fontsize = 13.0f;
    float ui_scale = 1.0f;

	float map_highlight_distance = 3000.0f;

	std::string exec_on_exit;
    std::string prepend_scn;

	struct extraviewport_config {
		std::string monitor;

		int width, height;
		float draw_range;

        viewport_proj_config projection;
	};
	std::vector<extraviewport_config> extra_viewports;

	struct pythonviewport_config {
		std::string surface;

		std::string monitor;
		glm::ivec2 size;

		glm::vec2 offset;
		glm::vec2 scale;
	};
	std::vector<pythonviewport_config> python_viewports;

    struct headtrack_config {
        std::string joy;

        bool magic_window = false;

        glm::ivec3 move_axes;
        glm::vec3 move_mul;

        glm::ivec3 rot_axes;
        glm::vec3 rot_mul;
    };
    headtrack_config headtrack_conf;

    glm::vec3 viewport_move;
    glm::mat3 viewport_rotate;

    bool map_manualswitchcontrol = false;;

	std::vector<std::pair<std::string, std::string>> network_servers;
	std::optional<std::pair<std::string, std::string>> network_client;
	float desync = 0.0f;

	std::unordered_map<int, std::string> trainset_overrides;

    float m_skysaturationcorrection{ 1.65f };
    float m_skyhuecorrection{ 0.5f };

// methods
    void LoadIniFile( std::string asFileName );
    void ConfigParse( cParser &parser );
    bool ConfigParse_gfx( cParser &parser, std::string_view const Token );
    // sends basic content of the class in legacy (text) format to provided stream
    void
        export_as_text( std::ostream &Output ) const;
    template <typename Type_>
    void
        export_as_text( std::ostream &Output, std::string const Key, Type_ const &Value ) const;
};

template <typename Type_>
void
global_settings::export_as_text( std::ostream &Output, std::string const Key, Type_ const &Value ) const {

    Output << Key << " " << Value << "\n";
}

template <>
void
global_settings::export_as_text( std::ostream &Output, std::string const Key, std::string const &Value ) const;

template <>
void
global_settings::export_as_text( std::ostream &Output, std::string const Key, bool const &Value ) const;

extern global_settings Global;
