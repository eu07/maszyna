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

#include "Globals.h"
#include "Logs.h"
#include "Console.h"
#include "PyInt.h"
#include "World.h"
#include "Mover.h"

#pragma comment (lib, "glew32.lib")
#pragma comment (lib, "glu32.lib")
#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "dsound.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "setupapi.lib")
#pragma comment (lib, "python27.lib")
#pragma comment (lib, "dbghelp.lib")
#pragma comment (lib, "glfw3.lib") //static

TWorld World;

#ifdef _WINDOWS
void make_minidump( ::EXCEPTION_POINTERS* e ) {

    auto hDbgHelp = ::LoadLibraryA( "dbghelp" );
    if( hDbgHelp == nullptr )
        return;
    auto pMiniDumpWriteDump = (decltype( &MiniDumpWriteDump ))::GetProcAddress( hDbgHelp, "MiniDumpWriteDump" );
    if( pMiniDumpWriteDump == nullptr )
        return;

    char name[ MAX_PATH ];
    {
        auto nameEnd = name + ::GetModuleFileNameA( ::GetModuleHandleA( 0 ), name, MAX_PATH );
        ::SYSTEMTIME t;
        ::GetSystemTime( &t );
        wsprintfA( nameEnd - strlen( ".exe" ),
            "_crashdump_%4d%02d%02d_%02d%02d%02d.dmp",
            t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond );
    }

    auto hFile = ::CreateFileA( name, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0 );
    if( hFile == INVALID_HANDLE_VALUE )
        return;

    ::MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
    exceptionInfo.ThreadId = ::GetCurrentThreadId();
    exceptionInfo.ExceptionPointers = e;
    exceptionInfo.ClientPointers = FALSE;

    auto dumped = pMiniDumpWriteDump(
        ::GetCurrentProcess(),
        ::GetCurrentProcessId(),
        hFile,
        ::MINIDUMP_TYPE( ::MiniDumpWithIndirectlyReferencedMemory | ::MiniDumpScanMemory ),
        e ? &exceptionInfo : nullptr,
        nullptr,
        nullptr );

    ::CloseHandle( hFile );

    return;
}

LONG CALLBACK unhandled_handler( ::EXCEPTION_POINTERS* e ) {
    make_minidump( e );
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

void window_resize_callback(GLFWwindow *window, int w, int h)
{
	glViewport(0, 0, w, h); // Reset The Current Viewport
	glMatrixMode(GL_PROJECTION); // select the Projection Matrix
	glLoadIdentity(); // reset the Projection Matrix
					  // calculate the aspect ratio of the window
	gluPerspective(45.0f, (GLdouble)w / (GLdouble)h, 0.2f, 2500.0f);
	glMatrixMode(GL_MODELVIEW); // select the Modelview Matrix
	glLoadIdentity(); // reset the Modelview Matrix
}

void cursor_pos_callback(GLFWwindow *window, double x, double y)
{
	if (Global::bActive)
		World.OnMouseMove(x * 0.005, y * 0.01);
	glfwSetCursorPos(window, 0.0, 0.0);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	//m7todo: bActive teraz pewnie nie potrzebne
	if (!Global::bActive)
		return;

	Global::shiftState = (mods & GLFW_MOD_SHIFT) ? true : false;
	Global::ctrlState = (mods & GLFW_MOD_CONTROL) ? true : false;

	if (action == GLFW_PRESS || action == GLFW_REPEAT)
	{
		World.OnKeyDown(key);

		switch (key)
		{
		case GLFW_KEY_PAUSE:
			if (Global::iPause & 1)
				Global::iPause &= ~1;
			else if (!(Global::iMultiplayer & 2) &&
				     (mods & GLFW_MOD_CONTROL))
				Global::iPause ^= 2;
			if (Global::iPause)
				Global::iTextMode = GLFW_KEY_F1;
			break;

		case GLFW_KEY_F7:
			Global::bWireFrame = !Global::bWireFrame;
			++Global::iReCompile;
			break;
		}
	}
	else if (action == GLFW_RELEASE)
	{
		World.OnKeyUp(key);
	}
}

void focus_callback(GLFWwindow *window, int focus)
{
	Global::bActive = (focus == GLFW_TRUE ? 1 : 0);
	if (Global::bInactivePause) // jeśli ma być pauzowanie okna w tle
		if (Global::bActive)
			Global::iPause &= ~4; // odpauzowanie, gdy jest na pierwszym planie
		else
			Global::iPause |= 4; // włączenie pauzy, gdy nieaktywy
}

int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR lpCmdLine, int)
{
#ifdef _WINDOWS
	::SetUnhandledExceptionFilter(unhandled_handler);
#endif

	if (!glfwInit())
		return -1;

    DeleteFile("errors.txt"); // usunięcie starego
    Global::LoadIniFile("eu07.ini"); // teraz dopiero można przejrzeć plik z ustawieniami
    Global::InitKeys("keys.ini"); // wczytanie mapowania klawiszy - jest na stałe

	// hunter-271211: ukrywanie konsoli
    if (Global::iWriteLogEnabled & 2)
    {
        AllocConsole();
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN);
    }

    std::string commandline(lpCmdLine);
    if (!commandline.empty())
    {
		cParser parser(commandline);
		std::string token;
		do
		{
			parser.getTokens();
			token.clear();
			parser >> token;

            if (token == "-s")
            { // nazwa scenerii
				parser.getTokens();
				parser >> Global::SceneryFile;
            }
            else if (token == "-v")
            { // nazwa wybranego pojazdu
				parser.getTokens();
				parser >> Global::asHumanCtrlVehicle;
            }
            else if (token == "-modifytga")
            { // wykonanie modyfikacji wszystkich plików TGA
                Global::iModifyTGA = -1; // specjalny tryb wykonania totalnej modyfikacji
            }
            else if (token == "-e3d")
            { // wygenerowanie wszystkich plików E3D
                if (Global::iConvertModels > 0)
                    Global::iConvertModels = -Global::iConvertModels; // specjalny tryb
                else
                    Global::iConvertModels = -7; // z optymalizacją, bananami i prawidłowym Opacity
            }
            else
                Error(
                    "Program usage: EU07 [-s sceneryfilepath] [-v vehiclename] [-modifytga] [-e3d]",
                    !Global::iWriteLogEnabled);
		}
		while (!token.empty());
    }

	glfwWindowHint(GL_SAMPLES, Global::iMultisampling);

	// match requested video mode to current to allow for
	// fullwindow creation when resolution is the same
	GLFWmonitor *monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode *vmode = glfwGetVideoMode(monitor);

	glfwWindowHint(GLFW_RED_BITS, vmode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, vmode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, vmode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, vmode->refreshRate);

	GLFWwindow *window =
		glfwCreateWindow(Global::iWindowWidth, Global::iWindowHeight,
		"EU07++", Global::bFullScreen ? monitor : nullptr, nullptr);

	if (!window)
		return -1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); //vsync
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); //capture cursor
	glfwSetCursorPos(window, 0.0, 0.0);
	glfwSetFramebufferSizeCallback(window, window_resize_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetWindowFocusCallback(window, focus_callback);
	glfwGetFramebufferSize(window, &width, &height);
	window_resize_callback(window, width, height);

	if (glewInit() != GLEW_OK)
		return -1;

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	Global::pWorld = &World; // Ra: wskaźnik potrzebny do usuwania pojazdów
	if (!World.Init(window))
		return -1;

    Console *pConsole = new Console(); // Ra: nie wiem, czy ma to sens, ale jakoś zainicjowac trzeba

    if (!joyGetNumDevs())
        WriteLog("No joystick");
    if (Global::iModifyTGA < 0)
    { // tylko modyfikacja TGA, bez uruchamiania symulacji
        Global::iMaxTextureSize = 64; //żeby nie zamulać pamięci
        World.ModifyTGA(); // rekurencyjne przeglądanie katalogów
    }
    else
    {
        if (Global::iConvertModels < 0)
        {
            Global::iConvertModels = -Global::iConvertModels;
            World.CreateE3D("models\\"); // rekurencyjne przeglądanie katalogów
            World.CreateE3D("dynamic\\", true);
        } // po zrobieniu E3D odpalamy normalnie scenerię, by ją zobaczyć

        Console::On(); // włączenie konsoli
        while (!glfwWindowShouldClose(window) && World.Update())
        {
			glfwSwapBuffers(window);
			glfwPollEvents();
        }
        Console::Off(); // wyłączenie konsoli (komunikacji zwrotnej)
    }

    delete pConsole;
	TPythonInterpreter::killInstance();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
