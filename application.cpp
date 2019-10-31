/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "application.h"
#include "scenarioloadermode.h"
#include "drivermode.h"
#include "editormode.h"

#include "Globals.h"
#include "simulation.h"
#include "Train.h"
#include "dictionary.h"
#include "sceneeditor.h"
#include "openglrenderer.h"
#include "opengl33renderer.h"
#include "uilayer.h"
#include "translation.h"
#include "Logs.h"
#include "Timer.h"

#ifdef EU07_BUILD_STATIC
#pragma comment( lib, "glfw3.lib" )
#else
#ifdef _WIN32
#pragma comment( lib, "glfw3dll.lib" )
#else
#pragma comment( lib, "glfw3.lib" )
#endif
#endif // build_static
#pragma comment( lib, "opengl32.lib" )
#pragma comment( lib, "glu32.lib" )
#pragma comment( lib, "openal32.lib")
#pragma comment( lib, "setupapi.lib" )
#pragma comment( lib, "python27.lib" )
#pragma comment( lib, "libserialport-0.lib" )
#pragma comment (lib, "dbghelp.lib")
#pragma comment (lib, "version.lib")

extern "C" { _declspec( dllexport ) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001; }
extern "C" { _declspec( dllexport ) DWORD NvOptimusEnablement = 0x00000001; }

eu07_application Application;

ui_layer uilayerstaticinitializer;

#ifdef _WIN32
extern "C"
{
    GLFWAPI HWND glfwGetWin32Window( GLFWwindow* window );
}

LONG CALLBACK unhandled_handler( ::EXCEPTION_POINTERS* e );
LRESULT APIENTRY WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
extern HWND Hwnd;
extern WNDPROC BaseWindowProc;
#endif

// user input callbacks

void focus_callback( GLFWwindow *window, int focus ) {
    if( Global.bInactivePause ) // jeśli ma być pauzowanie okna w tle
        if( focus )
            Global.iPause &= ~4; // odpauzowanie, gdy jest na pierwszym planie
        else
            Global.iPause |= 4; // włączenie pauzy, gdy nieaktywy
}

void window_resize_callback( GLFWwindow *window, int w, int h ) {
    // NOTE: we have two variables which basically do the same thing as we don't have dynamic fullscreen toggle
    // TBD, TODO: merge them?
    Global.iWindowWidth = w;
    Global.iWindowHeight = h;
    Global.fDistanceFactor = std::max( 0.5f, h / 768.0f ); // not sure if this is really something we want to use
    glViewport( 0, 0, w, h );
}

void cursor_pos_callback( GLFWwindow *window, double x, double y ) {

    Application.on_cursor_pos( x, y );
}

void mouse_button_callback( GLFWwindow* window, int button, int action, int mods ) {

    Application.on_mouse_button( button, action, mods );
}

void scroll_callback( GLFWwindow* window, double xoffset, double yoffset ) {

    Application.on_scroll( xoffset, yoffset );
}

void key_callback( GLFWwindow *window, int key, int scancode, int action, int mods ) {

    Application.on_key( key, scancode, action, mods );
}

// public:

int
eu07_application::init( int Argc, char *Argv[] ) {

    int result { 0 };

    init_debug();
    init_files();
    if( ( result = init_settings( Argc, Argv ) ) != 0 ) {
        return result;
    }
    if( ( result = init_locale() ) != 0 ) {
        return result;
    }

    WriteLog( "Starting MaSzyna rail vehicle simulator (release: " + Global.asVersion + ")" );
    WriteLog( "For online documentation and additional files refer to: http://eu07.pl" );
    WriteLog( "Authors: Marcin_EU, McZapkie, ABu, Winger, Tolaris, nbmx, OLO_EU, Bart, Quark-t, "
        "ShaXbee, Oli_EU, youBy, KURS90, Ra, hunter, szociu, Stele, Q, firleju and others\n" );

    if( ( result = init_glfw() ) != 0 ) {
        return result;
    }
    if( ( result = init_gfx() ) != 0 ) {
        return result;
    }
    init_callbacks();
    if( ( result = init_audio() ) != 0 ) {
        return result;
    }
    if( ( result = init_data() ) != 0 ) {
        return result;
    }
    m_taskqueue.init();
    if( ( result = init_modes() ) != 0 ) {
        return result;
    }

    return result;
}

int
eu07_application::run() {

    // main application loop
    while (!glfwWindowShouldClose( m_windows.front() ) && !m_modestack.empty())
    {
        Timer::subsystem.mainloop_total.start();

        if( !m_modes[ m_modestack.top() ]->update() )
            break;

        if (!GfxRenderer->Render())
            break;

        glfwPollEvents();

        if (m_modestack.empty())
            return 0;

        m_modes[ m_modestack.top() ]->on_event_poll();

		Timer::subsystem.mainloop_total.stop();
    }

	return 0;
}

// issues request for a worker thread to perform specified task. returns: true if task was scheduled
bool
eu07_application::request( python_taskqueue::task_request const &Task ) {

    auto const result { m_taskqueue.insert( Task ) };
    if( ( false == result )
     && ( Task.input != nullptr ) ) {
        // clean up allocated resources since the worker won't
        delete Task.input;
    }
    return result;
}

// ensures the main thread holds the python gil and can safely execute python calls
void
eu07_application::acquire_python_lock() {

    m_taskqueue.acquire_lock();
}

// frees the python gil and swaps out the main thread
void
eu07_application::release_python_lock() {

    m_taskqueue.release_lock();
}

void
eu07_application::exit() {

    SafeDelete( simulation::Train );
    SafeDelete( simulation::Region );

    ui_layer::shutdown();

    for( auto *window : m_windows ) {
        glfwDestroyWindow( window );
    }
    glfwTerminate();
    m_taskqueue.exit();
}

void
eu07_application::render_ui() {

    if( m_modestack.empty() ) { return; }

    m_modes[ m_modestack.top() ]->render_ui();
}

bool
eu07_application::pop_mode() {

    if( m_modestack.empty() ) { return false; }

    m_modes[ m_modestack.top() ]->exit();
    m_modestack.pop();
    return true;
}

bool
eu07_application::push_mode( eu07_application::mode const Mode ) {

    if( Mode >= mode::count_ ) { return false; }

    m_modes[ Mode ]->enter();
    m_modestack.push( Mode );

    return true;
}

void
eu07_application::set_title( std::string const &Title ) {

    glfwSetWindowTitle( m_windows.front(), Title.c_str() );
}

void
eu07_application::set_progress( float const Progress, float const Subtaskprogress ) {

    if( m_modestack.empty() ) { return; }

    m_modes[ m_modestack.top() ]->set_progress( Progress, Subtaskprogress );
}

void
eu07_application::set_cursor( int const Mode ) {

    ui_layer::set_cursor( Mode );
}

void
eu07_application::set_cursor_pos( double const Horizontal, double const Vertical ) {

    glfwSetCursorPos( m_windows.front(), Horizontal, Vertical );
}

glm::dvec2
eu07_application::get_cursor_pos() const {

    glm::dvec2 pos;
    if( !m_windows.empty() ) {
        glfwGetCursorPos( m_windows.front(), &pos.x, &pos.y );
    }
    return pos;
}

void
eu07_application::get_cursor_pos( double &Horizontal, double &Vertical ) const {

    glfwGetCursorPos( m_windows.front(), &Horizontal, &Vertical );
}

void
eu07_application::on_key( int const Key, int const Scancode, int const Action, int const Mods ) {

    if( m_modestack.empty() ) { return; }

    m_modes[ m_modestack.top() ]->on_key( Key, Scancode, Action, Mods );
}

void
eu07_application::on_cursor_pos( double const Horizontal, double const Vertical ) {

    if( m_modestack.empty() ) { return; }

    m_modes[ m_modestack.top() ]->on_cursor_pos( Horizontal, Vertical );
}

void
eu07_application::on_mouse_button( int const Button, int const Action, int const Mods ) {

    if( m_modestack.empty() ) { return; }

    m_modes[ m_modestack.top() ]->on_mouse_button( Button, Action, Mods );
}

void
eu07_application::on_scroll( double const Xoffset, double const Yoffset ) {

    if( m_modestack.empty() ) { return; }

    m_modes[ m_modestack.top() ]->on_scroll( Xoffset, Yoffset );
}

GLFWwindow *
eu07_application::window( int const Windowindex ) {

    if( Windowindex >= 0 ) {
        return (
            Windowindex < m_windows.size() ?
                m_windows[ Windowindex ] :
                nullptr );
    }
    // for index -1 create a new child window
    glfwWindowHint( GLFW_VISIBLE, GL_FALSE );
    auto *childwindow = glfwCreateWindow( 1, 1, "eu07helper", nullptr, m_windows.front() );
    if( childwindow != nullptr ) {
        m_windows.emplace_back( childwindow );
    }
    return childwindow;
}

// private:

void
eu07_application::init_debug() {

#if defined(_MSC_VER) && defined (_DEBUG)
    // memory leaks
    _CrtSetDbgFlag( _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ) | _CRTDBG_LEAK_CHECK_DF );
    // floating point operation errors
    /*
    auto state { _clearfp() };
    state = _control87( 0, 0 );
    // this will turn on FPE for #IND and zerodiv
    state = _control87( state & ~( _EM_ZERODIVIDE | _EM_INVALID ), _MCW_EM );
    */
#endif
#ifdef _WIN32
    ::SetUnhandledExceptionFilter( unhandled_handler );
#endif
}

void
eu07_application::init_files() {

#ifdef _WIN32
    DeleteFile( "log.txt" );
    DeleteFile( "errors.txt" );
    _mkdir( "logs" );
#endif
}

int
eu07_application::init_settings( int Argc, char *Argv[] ) {

    Global.LoadIniFile( "eu07.ini" );
    if( ( Global.iWriteLogEnabled & 2 ) != 0 ) {
        // show output console if requested
        AllocConsole();
    }
/*
    std::string executable( argv[ 0 ] ); auto const pathend = executable.rfind( '\\' );
    Global.ExecutableName =
        ( pathend != std::string::npos ?
            executable.substr( executable.rfind( '\\' ) + 1 ) :
            executable );
*/
#ifdef _WIN32
    // retrieve product version from the file's version data table
    {
        auto const fileversionsize { ::GetFileVersionInfoSize( Argv[ 0 ], NULL ) };
        std::vector<BYTE>fileversiondata; fileversiondata.resize( fileversionsize );
        if( ::GetFileVersionInfo( Argv[ 0 ], 0, fileversionsize, fileversiondata.data() ) ) {

            struct lang_codepage {
                WORD language;
                WORD codepage;
            } *langcodepage;
            UINT datasize;

            ::VerQueryValue(
                fileversiondata.data(),
                TEXT( "\\VarFileInfo\\Translation" ),
                (LPVOID*)&langcodepage,
                &datasize );

            std::string subblock; subblock.resize( 50 );
            ::StringCchPrintf(
                &subblock[0], subblock.size(),
                TEXT( "\\StringFileInfo\\%04x%04x\\ProductVersion" ),
                langcodepage->language,
                langcodepage->codepage );

            VOID *stringdata;
            if( ::VerQueryValue(
                    fileversiondata.data(),
                    subblock.data(),
                    &stringdata,
                    &datasize ) ) {

                Global.asVersion = std::string( reinterpret_cast<char*>(stringdata) );
            }
        }
    }
#endif
    // process command line arguments
    for( int i = 1; i < Argc; ++i ) {

        std::string token { Argv[ i ] };

        if( token == "-s" ) {
            if( i + 1 < Argc ) {
                Global.SceneryFile = ToLower( Argv[ ++i ] );
            }
        }
        else if( token == "-v" ) {
            if( i + 1 < Argc ) {
                Global.asHumanCtrlVehicle = ToLower( Argv[ ++i ] );
            }
        }
        else {
            std::cout
                << "usage: " << std::string( Argv[ 0 ] )
                << " [-s sceneryfilepath]"
                << " [-v vehiclename]"
                << std::endl;
            return -1;
        }
    }

    return 0;
}

int
eu07_application::init_locale() {

    locale::init();

    return 0;
}

int
eu07_application::init_glfw() {

    if( glfwInit() == GLFW_FALSE ) {
        ErrorLog( "Bad init: failed to initialize glfw" );
        return -1;
    }
    // match requested video mode to current to allow for
    // fullwindow creation when resolution is the same
    auto *monitor { glfwGetPrimaryMonitor() };
    auto const *vmode { glfwGetVideoMode( monitor ) };

    glfwWindowHint( GLFW_RED_BITS, vmode->redBits );
    glfwWindowHint( GLFW_GREEN_BITS, vmode->greenBits );
    glfwWindowHint( GLFW_BLUE_BITS, vmode->blueBits );
    glfwWindowHint( GLFW_REFRESH_RATE, vmode->refreshRate );

    glfwWindowHint( GLFW_AUTO_ICONIFY, GLFW_FALSE );
    if( Global.iMultisampling > 0 ) {
        glfwWindowHint( GLFW_SAMPLES, 1 << Global.iMultisampling );
    }

    if( Global.bFullScreen ) {
        // match screen dimensions with selected monitor, for 'borderless window' in fullscreen mode
        Global.iWindowWidth = vmode->width;
        Global.iWindowHeight = vmode->height;
    }

    auto *window {
        glfwCreateWindow(
            Global.iWindowWidth,
            Global.iWindowHeight,
            Global.AppName.c_str(),
            ( Global.bFullScreen ?
                monitor :
                nullptr ),
            nullptr ) };

    if( window == nullptr ) {
        ErrorLog( "Bad init: failed to create glfw window" );
        return -1;
    }

    glfwMakeContextCurrent( window );
    glfwSwapInterval( Global.VSync ? 1 : 0 ); //vsync

#ifdef _WIN32
// setup wrapper for base glfw window proc, to handle copydata messages
    Hwnd = glfwGetWin32Window( window );
    BaseWindowProc = ( WNDPROC )::SetWindowLongPtr( Hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc );
    // switch off the topmost flag
    ::SetWindowPos( Hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
#endif
    m_windows.emplace_back( window );

    return 0;
}

void
eu07_application::init_callbacks() {

    auto *window { m_windows.front() };
    glfwSetFramebufferSizeCallback( window, window_resize_callback );
    glfwSetCursorPosCallback( window, cursor_pos_callback );
    glfwSetMouseButtonCallback( window, mouse_button_callback );
    glfwSetKeyCallback( window, key_callback );
    glfwSetScrollCallback( window, scroll_callback );
    glfwSetWindowFocusCallback( window, focus_callback );
    {
        int width, height;
        glfwGetFramebufferSize( window, &width, &height );
        window_resize_callback( window, width, height );
    }
}

int
eu07_application::init_gfx() {

    if (Global.gfx_usegles)
    {
        if( 0 == gladLoadGLES2Loader( (GLADloadproc)glfwGetProcAddress ) ) {
            ErrorLog( "Bad init: failed to initialize glad" );
            return -1;
        }
    }
    else
    {
        if( 0 == gladLoadGLLoader( (GLADloadproc)glfwGetProcAddress ) ) {
            ErrorLog( "Bad init: failed to initialize glad" );
            return -1;
        }
    }

    if( Global.GfxRenderer == "default" ) {
        // default render path
        GfxRenderer = std::make_unique<opengl33_renderer>();
    }
    else {
        // legacy render path
        GfxRenderer = std::make_unique<opengl_renderer>();
        Global.GfxFramebufferSRGB = false;
        Global.DisabledLogTypes |= logtype::material;
    }

    if( false == GfxRenderer->Init( m_windows.front() ) ) {
        return -1;
    }
    if( false == ui_layer::init( m_windows.front() ) ) {
        return -1;
    }
/*
	for (const global_settings::extraviewport_config &conf : Global.extra_viewports)
		if (!GfxRenderer.AddViewport(conf))
			return -1;
*/
    return 0;
}

int
eu07_application::init_audio() {

    if( Global.bSoundEnabled ) {
        Global.bSoundEnabled &= audio::renderer.init();
    }
    // NOTE: lack of audio isn't deemed a failure serious enough to throw in the towel
    return 0;
}

int
eu07_application::init_data() {

    // HACK: grab content of the first {} block in load_unit_weights using temporary parser, then parse it normally. on any error our weight list will be empty string
    auto loadweights { cParser( cParser( "data/load_weights.txt", cParser::buffer_FILE ).getToken<std::string>( true, "{}" ), cParser::buffer_TEXT ) };
    while( true == loadweights.getTokens( 2 ) ) {
        std::pair<std::string, float> weightpair;
        loadweights
            >> weightpair.first
            >> weightpair.second;
        weightpair.first.erase( weightpair.first.end() - 1 ); // trim trailing ':' from the key
        simulation::Weights.emplace( weightpair.first, weightpair.second );
    }

    return 0;
}

int
eu07_application::init_modes() {

    // NOTE: we could delay creation/initialization until transition to specific mode is requested,
    // but doing it in one go at the start saves us some error checking headache down the road

    // create all application behaviour modes
    m_modes[ mode::scenarioloader ] = std::make_shared<scenarioloader_mode>();
    m_modes[ mode::driver ] = std::make_shared<driver_mode>();
    m_modes[ mode::editor ] = std::make_shared<editor_mode>();
    // initialize the mode objects
    for( auto &mode : m_modes ) {
        if( false == mode->init() ) {
            return -1;
        }
    }
    // activate the default mode
    push_mode( mode::scenarioloader );

    return 0;
}
