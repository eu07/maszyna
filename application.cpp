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

#include "Globals.h"
#include "keyboardinput.h"
#include "mouseinput.h"
#include "gamepadinput.h"
#include "Console.h"
#include "simulation.h"
#include "World.h"
#include "PyInt.h"
#include "sceneeditor.h"
#include "renderer.h"
#include "uilayer.h"
#include "Logs.h"
#include "screenshot.h"
#include "motiontelemetry.h"

#pragma comment (lib, "glu32.lib")
#pragma comment (lib, "dsound.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "setupapi.lib")
#pragma comment (lib, "dbghelp.lib")
#pragma comment (lib, "version.lib")

#ifdef __linux__
#include <unistd.h>
#include <sys/stat.h>
#endif

eu07_application Application;
screenshot_manager screenshot_man;

namespace input {

gamepad_input Gamepad;
mouse_input Mouse;
glm::dvec2 mouse_pickmodepos;  // stores last mouse position in control picking mode
keyboard_input Keyboard;
std::unique_ptr<uart_input> uart;
user_command command; // currently issued control command, if any
std::unique_ptr<motiontelemetry> motiontelemetry;
#ifdef _WIN32
Console console;
#endif
}

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

void window_resize_callback( GLFWwindow *window, int w, int h ) {
    // NOTE: we have two variables which basically do the same thing as we don't have dynamic fullscreen toggle
    // TBD, TODO: merge them?
    Global.iWindowWidth = w;
    Global.iWindowHeight = h;
    Global.fDistanceFactor = std::max( 0.5f, h / 768.0f ); // not sure if this is really something we want to use
    glViewport( 0, 0, w, h );
}

void cursor_pos_callback( GLFWwindow *window, double x, double y ) {
    if( false == Global.ControlPicking ) {
        glfwSetCursorPos( window, 0, 0 );
    }

    // give the potential event recipient a shot at it, in the virtual z order
    if( true == scene::Editor.on_mouse_move( x, y ) ) { return; }
    input::Mouse.move( x, y );
}

void mouse_button_callback( GLFWwindow* window, int button, int action, int mods ) {

    if( ( button != GLFW_MOUSE_BUTTON_LEFT )
        && ( button != GLFW_MOUSE_BUTTON_RIGHT ) ) {
        // we don't care about other mouse buttons at the moment
        return;
    }
    // give the potential event recipient a shot at it, in the virtual z order
    if( true == scene::Editor.on_mouse_button( button, action ) ) { return; }
    input::Mouse.button( button, action );
}

void key_callback( GLFWwindow *window, int key, int scancode, int action, int mods ) {

    Global.shiftState = ( mods & GLFW_MOD_SHIFT ) ? true : false;
    Global.ctrlState = ( mods & GLFW_MOD_CONTROL ) ? true : false;
    Global.altState = ( mods & GLFW_MOD_ALT ) ? true : false;

    // give the ui first shot at the input processing...
    if( true == UILayer.on_key( key, action ) ) { return; }
    if( true == scene::Editor.on_key( key, action ) ) { return; }
    // ...if the input is left untouched, pass it on
    input::Keyboard.key( key, action );

    if( ( true == Global.InputMouse )
        && ( ( key == GLFW_KEY_LEFT_ALT )
            || ( key == GLFW_KEY_RIGHT_ALT ) ) ) {
        // if the alt key was pressed toggle control picking mode and set matching cursor behaviour
        if( action == GLFW_RELEASE ) {

            if( Global.ControlPicking ) {
                // switch off
                Application.get_cursor_pos( input::mouse_pickmodepos.x, input::mouse_pickmodepos.y );
                Application.set_cursor( GLFW_CURSOR_DISABLED );
                Application.set_cursor_pos( 0, 0 );
            }
            else {
                // enter picking mode
                Application.set_cursor_pos( input::mouse_pickmodepos.x, input::mouse_pickmodepos.y );
                Application.set_cursor( GLFW_CURSOR_NORMAL );
            }
            // actually toggle the mode
            Global.ControlPicking = !Global.ControlPicking;
        }
    }

    if( ( key == GLFW_KEY_LEFT_SHIFT )
        || ( key == GLFW_KEY_LEFT_CONTROL )
        || ( key == GLFW_KEY_LEFT_ALT )
        || ( key == GLFW_KEY_RIGHT_SHIFT )
        || ( key == GLFW_KEY_RIGHT_CONTROL )
        || ( key == GLFW_KEY_RIGHT_ALT ) ) {
        // don't bother passing these
        return;
    }

    if( action == GLFW_PRESS || action == GLFW_REPEAT ) {

        World.OnKeyDown( key );

        switch( key ) {
            case GLFW_KEY_PRINT_SCREEN: {
                Application.queue_screenshot();
                break;
            }
            default: { break; }
        }
    }
}
void focus_callback( GLFWwindow *window, int focus ) {
    if( Global.bInactivePause ) // jeśli ma być pauzowanie okna w tle
        if( focus )
            Global.iPause &= ~4; // odpauzowanie, gdy jest na pierwszym planie
        else
            Global.iPause |= 4; // włączenie pauzy, gdy nieaktywy
}

void scroll_callback( GLFWwindow* window, double xoffset, double yoffset ) {

    if( Global.ctrlState ) {
        // ctrl + scroll wheel adjusts fov in debug mode
        Global.FieldOfView = clamp( static_cast<float>( Global.FieldOfView - yoffset * 20.0 / Global.fFpsAverage ), 15.0f, 75.0f );
    }
}

// public:

void eu07_application::queue_screenshot()
{
	screenshot_queued = true;
}

int
eu07_application::init( int Argc, char *Argv[] ) {

    int result { 0 };

    init_debug();
    init_files();
    if( ( result = init_settings( Argc, Argv ) ) != 0 ) {
        return result;
    }
    if( ( result = init_glfw() ) != 0 ) {
        return result;
    }
    init_callbacks();
    if( ( result = init_gfx() ) != 0 ) {
        return result;
    }
    if( ( result = init_audio() ) != 0 ) {
        return result;
    }

    return result;
}

int
eu07_application::run()
{
    // HACK: prevent mouse capture before simulation starts
    Global.ControlPicking = true;
    // TODO: move input sources and their initializations to the application mode member
    input::Keyboard.init();
    input::Mouse.init();
    input::Gamepad.init();
    if( true == Global.uart_conf.enable ) {
        input::uart = std::make_unique<uart_input>();
        input::uart->init();
    }
	if (Global.motiontelemetry_conf.enable)
		input::motiontelemetry = std::make_unique<motiontelemetry>();
#ifdef _WIN32
    Console::On(); // włączenie konsoli
#endif

    Global.pWorld = &World; // Ra: wskaźnik potrzebny do usuwania pojazdów

    if( false == World.Init( m_window ) ) {
        ErrorLog( "Bad init: simulation setup failed" );
        return -1;
    }

    if( Global.iConvertModels < 0 ) {
        // generate binary files for all 3d models
        Global.iConvertModels = -Global.iConvertModels;
        World.CreateE3D( szModelPath ); // rekurencyjne przeglądanie katalogów
        World.CreateE3D( szDynamicPath, true );
        // auto-close when you're done
        WriteLog( "Binary 3d model generation completed" );
        return 0;
    }

    set_cursor( GLFW_CURSOR_DISABLED );
    set_cursor_pos( 0, 0 );
    Global.ControlPicking = false;

    // main application loop
    // TODO: split into parts and delegate these to application mode member
    while( ( false == glfwWindowShouldClose( m_window ) )
        && ( true == World.Update() )
        && ( true == GfxRenderer.Render() ) ) {
        glfwPollEvents();
        input::Keyboard.poll();
		simulation::Commands.update();
		if (input::motiontelemetry)
			input::motiontelemetry->update();
		if (screenshot_queued)
		{
			screenshot_queued = false;
			screenshot_man.make_screenshot();
		}
        if( true == Global.InputMouse )   { input::Mouse.poll(); }
        if( true == Global.InputGamepad ) { input::Gamepad.poll(); }
        if( input::uart != nullptr )      { input::uart->poll(); }
        // TODO: wrap current command in object, include other input sources
        input::command = (
            input::Mouse.command() != user_command::none ?
                input::Mouse.command() :
                input::Keyboard.command() );
    }

    return 0;
}

void
eu07_application::exit() {

#ifdef _WIN32
    Console::Off(); // wyłączenie konsoli (komunikacji zwrotnej)
#endif    
    SafeDelete( simulation::Region );

    glfwDestroyWindow( m_window );
    glfwTerminate();

    TPythonInterpreter::killInstance();
}

void
eu07_application::set_cursor( int const Mode ) {

    UILayer.set_cursor( Mode );
}

void
eu07_application::set_cursor_pos( double const X, double const Y ) {

    if( m_window != nullptr ) {
        glfwSetCursorPos( m_window, X, Y );
    }
}

void
eu07_application::get_cursor_pos( double &X, double &Y ) const {

    if( m_window != nullptr ) {
        glfwGetCursorPos( m_window, &X, &Y );
    }
}

// private:

void
eu07_application::init_debug() {

#if defined(_MSC_VER) && defined (_DEBUG)
    // memory leaks
    _CrtSetDbgFlag( _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ) | _CRTDBG_LEAK_CHECK_DF );
    // floating point operation errors
    auto state { _clearfp() };
    state = _control87( 0, 0 );
    // this will turn on FPE for #IND and zerodiv
    state = _control87( state & ~( _EM_ZERODIVIDE | _EM_INVALID ), _MCW_EM );
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
    CreateDirectory( "logs", NULL );
#elif __linux__
	unlink("log.txt");
	unlink("errors.txt");
	mkdir("logs", 0664);
#endif
}

int
eu07_application::init_settings( int Argc, char *Argv[] ) {

    Global.LoadIniFile( "eu07.ini" );
#ifdef _WIN32
    if( ( Global.iWriteLogEnabled & 2 ) != 0 ) {
        // show output console if requested
        AllocConsole();
    }
#endif

	Global.asVersion = VERSION_INFO;

    // process command line arguments
    for( int i = 1; i < Argc; ++i ) {

        std::string token { Argv[ i ] };

        if( token == "-e3d" ) {
            Global.iConvertModels = (
                Global.iConvertModels > 0 ?
                    -Global.iConvertModels :
                    -7 ); // z optymalizacją, bananami i prawidłowym Opacity
        }
        else if( token == "-s" ) {
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
                << " [-e3d]"
                << std::endl;
            return -1;
        }
    }

    return 0;
}

int
eu07_application::init_glfw()
{
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
    // TBD, TODO: move the global pointer to a more appropriate place
    m_window = window;

    return 0;
}

void
eu07_application::init_callbacks() {

    glfwSetFramebufferSizeCallback( m_window, window_resize_callback );
    glfwSetCursorPosCallback( m_window, cursor_pos_callback );
    glfwSetMouseButtonCallback( m_window, mouse_button_callback );
    glfwSetKeyCallback( m_window, key_callback );
    glfwSetScrollCallback( m_window, scroll_callback );
    glfwSetWindowFocusCallback( m_window, focus_callback );
    {
        int width, height;
        glfwGetFramebufferSize( m_window, &width, &height );
        window_resize_callback( m_window, width, height );
    }
}

int
eu07_application::init_gfx() {

    if( glewInit() != GLEW_OK ) {
        ErrorLog( "Bad init: failed to initialize glew" );
        return -1;
    }

    if( ( false == GfxRenderer.Init( m_window ) )
     || ( false == UILayer.init( m_window ) ) ) {
        return -1;
    }

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
