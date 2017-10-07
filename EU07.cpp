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
/*
Authors:
MarcinW, McZapkie, Shaxbee, ABu, nbmx, youBy, Ra, winger, mamut, Q424,
Stele, firleju, szociu, hunter, ZiomalCl, OLI_EU and others
*/
#include "stdafx.h"
#ifdef CAN_I_HAS_LIBPNG
#include <png.h>
#endif

#include "Globals.h"
#include "Logs.h"
#include "keyboardinput.h"
#include "mouseinput.h"
#include "gamepadinput.h"
#include "Console.h"
#include "PyInt.h"
#include "World.h"
#include "Mover.h"
#include "usefull.h"
#include "timer.h"
#include "uilayer.h"

#ifdef EU07_BUILD_STATIC
#pragma comment( lib, "glfw3.lib" )
#pragma comment( lib, "glew32s.lib" )
#else
#ifdef _WINDOWS
#pragma comment( lib, "glfw3dll.lib" )
#else
#pragma comment( lib, "glfw3.lib" )
#endif
#pragma comment( lib, "glew32.lib" )
#endif // build_static
#pragma comment( lib, "opengl32.lib" )
#pragma comment( lib, "glu32.lib" )
#pragma comment( lib, "dsound.lib" )
#pragma comment( lib, "winmm.lib" )
#pragma comment( lib, "setupapi.lib" )
#pragma comment( lib, "python27.lib" )
#pragma comment (lib, "dbghelp.lib")
#pragma comment (lib, "version.lib")
#ifdef CAN_I_HAS_LIBPNG
#pragma comment (lib, "libpng16.lib")
#endif

#ifdef _MSC_VER
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif

TWorld World;

namespace input {

keyboard_input Keyboard;
mouse_input Mouse;
gamepad_input Gamepad;
glm::dvec2 mouse_pickmodepos;  // stores last mouse position in control picking mode

}

#ifdef CAN_I_HAS_LIBPNG
void screenshot_save_thread( char *img )
{
	png_image png;
	memset(&png, 0, sizeof(png_image));
	png.version = PNG_IMAGE_VERSION;
	png.width = Global::iWindowWidth;
	png.height = Global::iWindowHeight;
	png.format = PNG_FORMAT_RGB;

	char datetime[64];
	time_t timer;
	struct tm* tm_info;
	time(&timer);
	tm_info = localtime(&timer);
	strftime(datetime, 64, "%Y-%m-%d_%H-%M-%S", tm_info);

	uint64_t perf;
	QueryPerformanceCounter((LARGE_INTEGER*)&perf);

	std::string filename = "screenshots/" + std::string(datetime) +
	                       "_" + std::to_string(perf) + ".png";

	if (png_image_write_to_file(&png, filename.c_str(), 0, img, -Global::iWindowWidth * 3, nullptr) == 1)
		WriteLog("saved " + filename + ".");
	else
		WriteLog("failed to save screenshot.");

	delete[] img;
}

void make_screenshot()
{
	char *img = new char[Global::iWindowWidth * Global::iWindowHeight * 3];
	glReadPixels(0, 0, Global::iWindowWidth, Global::iWindowHeight, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)img);

	std::thread t(screenshot_save_thread, img);
	t.detach();
}
#endif

void window_resize_callback(GLFWwindow *window, int w, int h)
{
    // NOTE: we have two variables which basically do the same thing as we don't have dynamic fullscreen toggle
    // TBD, TODO: merge them?
	Global::iWindowWidth = w;
	Global::iWindowHeight = h;
    Global::fDistanceFactor = std::max( 0.5f, h / 768.0f ); // not sure if this is really something we want to use
	glViewport(0, 0, w, h);
}

void cursor_pos_callback(GLFWwindow *window, double x, double y)
{
    input::Mouse.move( x, y );

    if( true == Global::ControlPicking ) {
        glfwSetCursorPos( window, x, y );
    }
    else {
        glfwSetCursorPos( window, 0, 0 );
    }
}

void mouse_button_callback( GLFWwindow* window, int button, int action, int mods ) {

    if( ( button == GLFW_MOUSE_BUTTON_LEFT )
     || ( button == GLFW_MOUSE_BUTTON_RIGHT ) ) {
        // we don't care about other mouse buttons at the moment
        input::Mouse.button( button, action );
    }
}

void key_callback( GLFWwindow *window, int key, int scancode, int action, int mods ) {

    input::Keyboard.key( key, action );

    Global::shiftState = ( mods & GLFW_MOD_SHIFT ) ? true : false;
    Global::ctrlState = ( mods & GLFW_MOD_CONTROL ) ? true : false;

    if( ( true == Global::InputMouse )
     && ( ( key == GLFW_KEY_LEFT_ALT )
       || ( key == GLFW_KEY_RIGHT_ALT ) ) ) {
        // if the alt key was pressed toggle control picking mode and set matching cursor behaviour
        if( action == GLFW_RELEASE ) {

            if( Global::ControlPicking ) {
                // switch off
                glfwGetCursorPos( window, &input::mouse_pickmodepos.x, &input::mouse_pickmodepos.y );
                glfwSetInputMode( window, GLFW_CURSOR, GLFW_CURSOR_DISABLED );
                glfwSetCursorPos( window, 0, 0 );
            }
            else {
                // enter picking mode
                glfwSetInputMode( window, GLFW_CURSOR, GLFW_CURSOR_NORMAL );
                glfwSetCursorPos( window, input::mouse_pickmodepos.x, input::mouse_pickmodepos.y );
            }
            // actually toggle the mode
            Global::ControlPicking = !Global::ControlPicking;
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

#ifdef CAN_I_HAS_LIBPNG
        switch( key )
        {
            case GLFW_KEY_F11: {
                make_screenshot();
                break;
            }
            default: { break; }
        }
#endif
    }
}

void focus_callback( GLFWwindow *window, int focus )
{
    if( Global::bInactivePause ) // jeśli ma być pauzowanie okna w tle
        if( focus )
            Global::iPause &= ~4; // odpauzowanie, gdy jest na pierwszym planie
        else
            Global::iPause |= 4; // włączenie pauzy, gdy nieaktywy
}

void scroll_callback( GLFWwindow* window, double xoffset, double yoffset ) {

    if( Global::ctrlState ) {
        // ctrl + scroll wheel adjusts fov in debug mode
        Global::FieldOfView = clamp( static_cast<float>(Global::FieldOfView - yoffset * 20.0 / Global::fFpsAverage), 15.0f, 75.0f );
    }
}

#ifdef _WINDOWS
extern "C"
{
    GLFWAPI HWND glfwGetWin32Window(GLFWwindow* window);
}

LONG CALLBACK unhandled_handler(::EXCEPTION_POINTERS* e);
LRESULT APIENTRY WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern HWND Hwnd;
extern WNDPROC BaseWindowProc;
#endif

int main(int argc, char *argv[])
{
#if defined(_MSC_VER) && defined (_DEBUG)
    // memory leaks
    _CrtSetDbgFlag( _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ) | _CRTDBG_LEAK_CHECK_DF );
    // floating point operation errors
    auto state = _clearfp();
    state = _control87( 0, 0 );
    // this will turn on FPE for #IND and zerodiv
    state = _control87( state & ~( _EM_ZERODIVIDE | _EM_INVALID ), _MCW_EM );
#endif
#ifdef _WINDOWS
    ::SetUnhandledExceptionFilter( unhandled_handler );
#endif

	if (!glfwInit())
		return -1;

#ifdef _WINDOWS
    DeleteFile( "log.txt" );
    DeleteFile( "errors.txt" );
    _mkdir("logs");
#endif
    Global::LoadIniFile("eu07.ini");
    Global::InitKeys();

    // hunter-271211: ukrywanie konsoli
    if( Global::iWriteLogEnabled & 2 )
	{
        AllocConsole();
        SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), FOREGROUND_GREEN );
    }
/*
    std::string executable( argv[ 0 ] ); auto const pathend = executable.rfind( '\\' );
    Global::ExecutableName =
        ( pathend != std::string::npos ?
            executable.substr( executable.rfind( '\\' ) + 1 ) :
            executable );
*/
    // retrieve product version from the file's version data table
    {
        auto const fileversionsize = ::GetFileVersionInfoSize( argv[ 0 ], NULL );
        std::vector<BYTE>fileversiondata; fileversiondata.resize( fileversionsize );
        if( ::GetFileVersionInfo( argv[ 0 ], 0, fileversionsize, fileversiondata.data() ) ) {

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

                Global::asVersion = std::string( reinterpret_cast<char*>(stringdata) );
            }
        }
    }

	for (int i = 1; i < argc; ++i)
	{
		std::string token(argv[i]);

		if (token == "-e3d") {
			if (Global::iConvertModels > 0)
				Global::iConvertModels = -Global::iConvertModels;
			else
				Global::iConvertModels = -7; // z optymalizacją, bananami i prawidłowym Opacity
		}
		else if (i + 1 < argc && token == "-s")
			Global::SceneryFile = std::string(argv[++i]);
		else if (i + 1 < argc && token == "-v")
		{
			std::string v(argv[++i]);
			std::transform(v.begin(), v.end(), v.begin(), ::tolower);
			Global::asHumanCtrlVehicle = v;
		}
		else
		{
			std::cout
                << "usage: " << std::string(argv[0])
                << " [-s sceneryfilepath]"
                << " [-v vehiclename]"
                << " [-e3d]"
                << std::endl;
			return -1;
		}
	}

    // match requested video mode to current to allow for
    // fullwindow creation when resolution is the same
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *vmode = glfwGetVideoMode(monitor);

    glfwWindowHint(GLFW_RED_BITS, vmode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, vmode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, vmode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, vmode->refreshRate);

    glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
    if( Global::iMultisampling > 0 ) {
        glfwWindowHint( GLFW_SAMPLES, 1 << Global::iMultisampling );
    }

    if (Global::bFullScreen)
	{
        // match screen dimensions with selected monitor, for 'borderless window' in fullscreen mode
        Global::iWindowWidth = vmode->width;
        Global::iWindowHeight = vmode->height;
    }

    GLFWwindow *window =
        glfwCreateWindow(
            Global::iWindowWidth,
            Global::iWindowHeight,
            Global::AppName.c_str(),
            ( Global::bFullScreen ?
                monitor :
                nullptr),
            nullptr );

    if (!window)
	{
        std::cout << "failed to create window" << std::endl;
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(Global::VSync ? 1 : 0); //vsync
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); //capture cursor
    glfwSetCursorPos(window, 0.0, 0.0);
    glfwSetFramebufferSizeCallback(window, window_resize_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetMouseButtonCallback( window, mouse_button_callback );
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback( window, scroll_callback );
    glfwSetWindowFocusCallback(window, focus_callback);
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        window_resize_callback(window, width, height);
    }

    if (glewInit() != GLEW_OK)
	{
        std::cout << "failed to init GLEW" << std::endl;
        return -1;
    }

#ifdef _WINDOWS
    // setup wrapper for base glfw window proc, to handle copydata messages
    Hwnd = glfwGetWin32Window( window );
    BaseWindowProc = (WNDPROC)::SetWindowLongPtr( Hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc );
    // switch off the topmost flag
    ::SetWindowPos( Hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
#endif

    if( ( false == GfxRenderer.Init( window ) )
     || ( false == UILayer.init( window ) ) ) {

        return -1;
    }
    input::Keyboard.init();
    input::Mouse.init();
    input::Gamepad.init();

    Global::pWorld = &World; // Ra: wskaźnik potrzebny do usuwania pojazdów
    try {
        if( false == World.Init( window ) ) {
            ErrorLog( "Simulation setup failed" );
            return -1;
        }
    }
    catch( std::bad_alloc const &Error ) {

        ErrorLog( "Critical error, memory allocation failure: " + std::string( Error.what() ) );
        return -1;
    }

    Console *pConsole = new Console(); // Ra: nie wiem, czy ma to sens, ale jakoś zainicjowac trzeba
/*
    if( !joyGetNumDevs() )
        WriteLog( "No joystick" );
*/
    if( Global::iConvertModels < 0 ) {
        Global::iConvertModels = -Global::iConvertModels;
        World.CreateE3D( "models\\" ); // rekurencyjne przeglądanie katalogów
        World.CreateE3D( "dynamic\\", true );
    } // po zrobieniu E3D odpalamy normalnie scenerię, by ją zobaczyć

    Console::On(); // włączenie konsoli

    try {
        while( ( false == glfwWindowShouldClose( window ) )
            && ( true == World.Update() )
            && ( true == GfxRenderer.Render() ) ) {
            glfwPollEvents();
            input::Keyboard.poll();
            if( true == Global::InputMouse )   { input::Mouse.poll(); }
            if( true == Global::InputGamepad ) { input::Gamepad.poll(); }
        }
    }
    catch( std::bad_alloc const &Error ) {

        ErrorLog( "Critical error, memory allocation failure: " + std::string( Error.what() ) );
        return -1;
    }

    Console::Off(); // wyłączenie konsoli (komunikacji zwrotnej)

	TPythonInterpreter::killInstance();
	SafeDelete( pConsole );
    SafeDelete( simulation::Region );

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
