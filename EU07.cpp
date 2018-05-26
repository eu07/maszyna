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
#include <png.h>
#include <thread>

#include "World.h"
#include "simulation.h"
#include "Globals.h"
#include "Timer.h"
#include "Logs.h"
#include "renderer.h"
#include "uilayer.h"
#include "audiorenderer.h"
#include "keyboardinput.h"
#include "mouseinput.h"
#include "gamepadinput.h"
#include "Console.h"
#include "uart.h"
#include "PyInt.h"
#include "World.h"
#include "MOVER.h"
#include "utilities.h"
#include "Timer.h"
#include "resource.h"
#include "uilayer.h"
#include "uart.h"
#include "motiontelemetry.h"

#pragma comment (lib, "glu32.lib")
#pragma comment (lib, "dsound.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "setupapi.lib")
#pragma comment (lib, "dbghelp.lib")
#pragma comment (lib, "version.lib")

#ifdef _MSC_VER 
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup") 
#endif 

TWorld World;

namespace input {

keyboard_input Keyboard;
mouse_input Mouse;
gamepad_input Gamepad;
glm::dvec2 mouse_pickmodepos;  // stores last mouse position in control picking mode
std::unique_ptr<uart_input> uart;
std::unique_ptr<motiontelemetry> motiontelemetry;
}

void screenshot_save_thread( char *img )
{
	png_image png;
	memset(&png, 0, sizeof(png_image));
	png.version = PNG_IMAGE_VERSION;
	png.width = Global.iWindowWidth;
	png.height = Global.iWindowHeight;
	png.format = PNG_FORMAT_RGB;

	char datetime[64];
	time_t timer;
	struct tm* tm_info;
	time(&timer);
	tm_info = localtime(&timer);
	strftime(datetime, 64, "%Y-%m-%d_%H-%M-%S", tm_info);

	uint64_t perf;
#ifdef _WIN32
	QueryPerformanceCounter((LARGE_INTEGER*)&perf);
#elif __linux__
	timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	perf = ts.tv_nsec;
#endif

	std::string filename = Global.screenshot_dir + "/" + std::string(datetime) +
	                       "_" + std::to_string(perf) + ".png";

	if (png_image_write_to_file(&png, filename.c_str(), 0, img, -Global.iWindowWidth * 3, nullptr) == 1)
		WriteLog("saved " + filename + ".");
	else
		WriteLog("failed to save screenshot.");

	delete[] img;
}

void make_screenshot()
{
	char *img = new char[Global.iWindowWidth * Global.iWindowHeight * 3];
	glReadPixels(0, 0, Global.iWindowWidth, Global.iWindowHeight, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)img);

	std::thread t(screenshot_save_thread, img);
	t.detach();
}

void window_resize_callback(GLFWwindow *window, int w, int h)
{
    // NOTE: we have two variables which basically do the same thing as we don't have dynamic fullscreen toggle
    // TBD, TODO: merge them?
	Global.iWindowWidth = w;
	Global.iWindowHeight = h;
    Global.fDistanceFactor = std::max( 0.5f, h / 768.0f ); // not sure if this is really something we want to use
	glViewport(0, 0, w, h);
}

void cursor_pos_callback(GLFWwindow *window, double x, double y)
{
	if (!window)
		return;

    input::Mouse.move( x, y );

    if( !Global.ControlPicking ) {
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

    Global.shiftState = ( mods & GLFW_MOD_SHIFT ) ? true : false;
    Global.ctrlState = ( mods & GLFW_MOD_CONTROL ) ? true : false;

    // give the ui first shot at the input processing...
    if( true == UILayer.on_key( key, action ) ) { return; }
    // ...if the input is left untouched, pass it on
    input::Keyboard.key( key, action );

    if( ( true == Global.InputMouse )
     && ( ( key == GLFW_KEY_LEFT_ALT )
       || ( key == GLFW_KEY_RIGHT_ALT ) ) ) {
        // if the alt key was pressed toggle control picking mode and set matching cursor behaviour
        if( action == GLFW_RELEASE ) {

            if( Global.ControlPicking ) {
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

        switch( key )
        {
            case GLFW_KEY_PRINT_SCREEN: {
                make_screenshot();
                break;
            }
            default: { break; }
        }
    }
}

void focus_callback( GLFWwindow *window, int focus )
{
    if( Global.bInactivePause ){ // jeśli ma być pauzowanie okna w tle
        if( focus ){
            Global.set_pause_flag( Global.PAUSES::INACTIVE_STATE, false );
            Global.set_pause_flag( Global.PAUSES::PAUSE_RENDER, false );
            Global.set_pause_flag( Global.PAUSES::PAUSE_SOUND, false );
        } else {
            Global.set_pause_flag( Global.PAUSES::INACTIVE_STATE, true );
            Global.set_pause_flag( Global.PAUSES::PAUSE_RENDER, true );
            Global.set_pause_flag( Global.PAUSES::PAUSE_SOUND, true );
        }
    }
}

void scroll_callback( GLFWwindow* window, double xoffset, double yoffset ) {

    if( Global.ctrlState ) {
        // ctrl + scroll wheel adjusts fov in debug mode
        Global.FieldOfView = clamp( static_cast<float>(Global.FieldOfView - yoffset * 20.0 / Global.fFpsAverage), 15.0f, 75.0f );
    }
}

#ifdef _WIN32
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
#ifdef _WIN32
    ::SetUnhandledExceptionFilter( unhandled_handler );
#endif

	if (!glfwInit())
		return -1;

#ifdef _WIN32
    DeleteFile( "log.txt" );
    DeleteFile( "errors.txt" );
    CreateDirectory("logs", NULL);
#endif
    Global.LoadIniFile("eu07.ini");

#ifdef _WIN32
    // hunter-271211: ukrywanie konsoli
    if( Global.iWriteLogEnabled & 2 )
	{
        AllocConsole();
        SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), FOREGROUND_GREEN );
    }
#endif

	for (int i = 1; i < argc; ++i)
	{
		std::string token(argv[i]);

		if (token == "-e3d") {
			if (Global.iConvertModels > 0)
				Global.iConvertModels = -Global.iConvertModels;
			else
				Global.iConvertModels = -7; // z optymalizacją, bananami i prawidłowym Opacity
		}
		else if (i + 1 < argc && token == "-s")
		{
			Global.SceneryFile = std::string(argv[++i]);
			std::replace(Global.SceneryFile.begin(), Global.SceneryFile.end(), '\\', '/');
		}
		else if (i + 1 < argc && token == "-v")
		{
			std::string v(argv[++i]);
			std::transform(v.begin(), v.end(), v.begin(), ::tolower);
			Global.asHumanCtrlVehicle = v;
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
    if( Global.iMultisampling > 0 ) {
        glfwWindowHint( GLFW_SAMPLES, 1 << Global.iMultisampling );
    }

    if (Global.bFullScreen)
	{
        // match screen dimensions with selected monitor, for 'borderless window' in fullscreen mode
        Global.iWindowWidth = vmode->width;
        Global.iWindowHeight = vmode->height;
    }

    GLFWwindow *window =
        glfwCreateWindow(
            Global.iWindowWidth,
            Global.iWindowHeight,
            Global.AppName.c_str(),
            ( Global.bFullScreen ?
                monitor :
                nullptr),
            nullptr );

    if (!window)
	{
        std::cout << "failed to create window" << std::endl;
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(Global.VSync ? 1 : 0); //vsync
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

#ifdef _WIN32
    // setup wrapper for base glfw window proc, to handle copydata messages
    Hwnd = glfwGetWin32Window( window );
    BaseWindowProc = (WNDPROC)::SetWindowLongPtr( Hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc );
    // switch off the topmost flag
    ::SetWindowPos( Hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
#endif

	try {
		Global.ControlPicking = true;

		if ((false == GfxRenderer.Init(window))
			|| (false == UILayer.init(window)))
			return -1;

    if( Global.bSoundEnabled ) {
        Global.bSoundEnabled &= audio::renderer.init();
    }

		audio::renderer.init();
		input::Keyboard.init();
		input::Mouse.init();
		input::Gamepad.init();
        if (Global.uart_conf.enable)
            input::uart = std::make_unique<uart_input>();
		if (Global.motiontelemetry_conf.enable)
			input::motiontelemetry = std::make_unique<motiontelemetry>();

		Global.pWorld = &World;
		if( false == World.Init( window ) ) {
            ErrorLog( "Simulation setup failed" );
            return -1;
        }
    }
    catch( std::bad_alloc const &Error )
	{
        ErrorLog( "Critical error, memory allocation failure: " + std::string( Error.what() ) );
        return -1;
    }
	catch (std::runtime_error e)
	{
        ErrorLog(e.what());
#ifdef _WIN32
		MessageBox(NULL, e.what(), "MaSzyna", MB_OK);
#endif
		return -1;
	}

#ifdef _WIN32
    Console *pConsole = new Console(); // Ra: nie wiem, czy ma to sens, ale jakoś zainicjowac trzeba
#endif
/*
    if( !joyGetNumDevs() )
        WriteLog( "No joystick" );
*/
    if( Global.iConvertModels < 0 ) {
        Global.iConvertModels = -Global.iConvertModels;
        World.CreateE3D( szModelPath ); // rekurencyjne przeglądanie katalogów
        World.CreateE3D( szDynamicPath, true );
    } // po zrobieniu E3D odpalamy normalnie scenerię, by ją zobaczyć

#ifdef _WIN32
    Console::On(); // włączenie konsoli
#endif

	Global.ControlPicking = false;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); //capture cursor

    try {
        while( ( false == glfwWindowShouldClose( window ) )
            && ( true == World.Update() )
            && ( true == GfxRenderer.Render() ) ) {
			if (input::motiontelemetry)
				input::motiontelemetry->update();
            glfwPollEvents();
            input::Keyboard.poll();
			simulation::Commands.update();
            if( true == Global.InputMouse )   { input::Mouse.poll(); }
            if( true == Global.InputGamepad ) { input::Gamepad.poll(); }
            if( input::uart != nullptr )      { input::uart->poll(); }
        }
	}
	catch (std::runtime_error e)
    {
    	ErrorLog(e.what());
#ifdef _WIN32
		MessageBox(NULL, e.what(), "MaSzyna", MB_OK);
#endif
		return -1;
	}
	catch( std::bad_alloc const &Error ) {
		ErrorLog( "Critical error, memory allocation failure: " + std::string( Error.what() ) );
		return -1;
	}

#ifdef _WIN32
    Console::Off(); // wyłączenie konsoli (komunikacji zwrotnej)
    SafeDelete( pConsole );
#endif    
    SafeDelete( simulation::Region );

	glfwDestroyWindow(window);
	glfwTerminate();

    TPythonInterpreter::killInstance();

	_exit(0); // skip destructors, there are ordering errors which causes segfaults
	return 0;
}
