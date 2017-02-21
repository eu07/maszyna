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

#include <dsound.h> //_clear87() itp.

#include "opengl/glew.h"
#include "opengl/wglew.h"
#include "opengl/ARB_Multisample.h"

#include "Globals.h"
#include "Logs.h"
#include "Console.h"
#include "PyInt.h"
#include "World.h"
#include "Mover.h"

#pragma comment( lib, "glew32.lib" )
#pragma comment( lib, "glu32.lib" )
#pragma comment( lib, "opengl32.lib" )
#pragma comment( lib, "dsound.lib" )
#pragma comment( lib, "winmm.lib" )
#pragma comment( lib, "setupapi.lib" )
#pragma comment( lib, "python27.lib" )
#pragma comment (lib, "dbghelp.lib")

HDC hDC = NULL; // Private GDI Device Context
HGLRC hRC = NULL; // Permanent Rendering Context
HWND hWnd = NULL; // Holds Our Window Handle

TWorld World;

// bool active=TRUE;	//window active flag set to TRUE by default
bool fullscreen = TRUE; // fullscreen flag set to fullscreen mode by default
int WindowWidth = 800;
int WindowHeight = 600;
int Bpp = 32;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM); // Declaration For WndProc

//#include "dbgForm.h"
//---------------------------------------------------------------------------

int InitGL(GLvoid) // All Setup For OpenGL Goes Here
{
//    _clear87();
//    _control87(MCW_EM, MCW_EM);
    glewInit();

    if( !GLEW_VERSION_1_4 ) {
        // experimental: require at least openGL 1.4
        Error( "This application requires openGL version 1.4 (or better)" );
        return 0;
    }

    // hunter-271211: przeniesione
    // AllocConsole();
    // SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_GREEN);

    // ShaXbee-121209: Wlaczenie obslugi tablic wierzcholkow
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    Global::pWorld = &World; // Ra: wskaźnik potrzebny do usuwania pojazdów
    return World.Init(hWnd, hDC); // true jeśli wszystko pójdzie dobrze
}
//---------------------------------------------------------------------------

GLvoid ReSizeGLScene(GLsizei width, GLsizei height) // resize and initialize the GL Window
{
    Global::ScreenWidth = WindowWidth = width;
    Global::ScreenHeight = WindowHeight = height;
    if (height == 0) // prevent a divide by zero by
        height = 1; // making height equal one
    glViewport(0, 0, width, height); // Reset The Current Viewport
/*
    glMatrixMode(GL_PROJECTION); // select the Projection Matrix
    glLoadIdentity(); // reset the Projection Matrix
    // calculate the aspect ratio of the window
    gluPerspective(45.0f, (GLdouble)width / (GLdouble)height, 0.2f, 2500.0f);
    glMatrixMode(GL_MODELVIEW); // select the Modelview Matrix
    glLoadIdentity(); // reset the Modelview Matrix
*/
}

//---------------------------------------------------------------------------
GLvoid KillGLWindow(GLvoid) // properly kill the window
{
    if (hRC) // Do We Have A Rendering Context?
    {
        if (!wglMakeCurrent(NULL, NULL)) // are we able to release the DC and RC contexts?
        {
            ErrorLog("Fail: window releasing");
            MessageBox(NULL, "Release of DC and RC failed.", "SHUTDOWN ERROR",
                       MB_OK | MB_ICONINFORMATION);
        }

        if (!wglDeleteContext(hRC)) // are we able to delete the RC?
        {
            ErrorLog("Fail: rendering context releasing");
            MessageBox(NULL, "Release rendering context failed.", "SHUTDOWN ERROR",
                       MB_OK | MB_ICONINFORMATION);
        }
        hRC = NULL; // set RC to NULL
    }

    if (hDC && !ReleaseDC(hWnd, hDC)) // are we able to release the DC?
    {
        ErrorLog("Fail: device context releasing");
        MessageBox(NULL, "Release device context failed.", "SHUTDOWN ERROR",
                   MB_OK | MB_ICONINFORMATION);
        hDC = NULL; // set DC to NULL
    }

    if (hWnd && !DestroyWindow(hWnd)) // are we able to destroy the window?
    {
        ErrorLog("Fail: window destroying");
        MessageBox(NULL, "Could not release hWnd.", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
        hWnd = NULL; // set hWnd to NULL
    }

    if (fullscreen) // Are We In Fullscreen Mode?
    {
        ChangeDisplaySettings(NULL, 0); // if so switch back to the desktop
        ShowCursor(TRUE); // show mouse pointer
    }
    //    KillFont();
}

/*	This code creates our OpenGL Window.  Parameters are:			*
 *	title			- title to appear at the top of the window	*
 *	width			- width of the GL Window or fullscreen mode	*
 *	height			- height of the GL Window or fullscreen mode	*
 *	bits			- number of bits to use for color (8/16/24/32)	*
 *	fullscreenflag	- use fullscreen mode (TRUE) or windowed mode (FALSE)	*/

BOOL CreateGLWindow(char *title, int width, int height, int bits, bool fullscreenflag)
{
    GLuint PixelFormat; // holds the results after searching for a match
    HINSTANCE hInstance; // holds the instance of the application
    WNDCLASS wc; // windows class structure
    DWORD dwExStyle; // window extended style
    DWORD dwStyle; // window style
    RECT WindowRect; // grabs rectangle upper left / lower right values
    WindowRect.left = (long)0; // set left value to 0
    WindowRect.right = (long)width; // set right value to requested width
    WindowRect.top = (long)0; // set top value to 0
    WindowRect.bottom = (long)height; // set bottom value to requested height

    fullscreen = fullscreenflag; // set the global fullscreen flag

    hInstance = GetModuleHandle(NULL); // grab an instance for our window
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // redraw on size, and own DC for window.
    wc.lpfnWndProc = (WNDPROC)WndProc; // wndproc handles messages
    wc.cbClsExtra = 0; // no extra window data
    wc.cbWndExtra = 0; // no extra window data
    wc.hInstance = hInstance; // set the instance
    wc.hIcon = LoadIcon(NULL, IDI_WINLOGO); // load the default icon
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); // load the arrow pointer
    wc.hbrBackground = NULL; // no background required for GL
    wc.lpszMenuName = NULL; // we don't want a menu
    wc.lpszClassName = "EU07"; // nazwa okna do komunikacji zdalnej
    // // Set The Class Name

    if (!arbMultisampleSupported) // tylko dla pierwszego okna
        if (!RegisterClass(&wc)) // Attempt To Register The Window Class
        {
            ErrorLog("Fail: window class registeration");
            MessageBox(NULL, "Failed to register the window class.", "ERROR",
                       MB_OK | MB_ICONEXCLAMATION);
            return FALSE; // Return FALSE
        }

    if (fullscreen) // Attempt Fullscreen Mode?
    {
        DEVMODE dmScreenSettings; // device mode
        memset(&dmScreenSettings, 0, sizeof(dmScreenSettings)); // makes sure memory's cleared
        dmScreenSettings.dmSize = sizeof(dmScreenSettings); // size of the devmode structure

        // tolaris-240403: poprawka na odswiezanie monitora
        // locate primary monitor...
        if (Global::bAdjustScreenFreq)
        {
            POINT point;
            point.x = 0;
            point.y = 0;
            MONITORINFOEX monitorinfo;
            monitorinfo.cbSize = sizeof(MONITORINFOEX);
            ::GetMonitorInfo(::MonitorFromPoint(point, MONITOR_DEFAULTTOPRIMARY), &monitorinfo);
            //  ..and query for highest supported refresh rate
            unsigned int refreshrate = 0;
            int i = 0;
            while (::EnumDisplaySettings(monitorinfo.szDevice, i, &dmScreenSettings))
            {
                if (i > 0)
                    if (dmScreenSettings.dmPelsWidth == (unsigned int)width)
                        if (dmScreenSettings.dmPelsHeight == (unsigned int)height)
                            if (dmScreenSettings.dmBitsPerPel == (unsigned int)bits)
                                if (dmScreenSettings.dmDisplayFrequency > refreshrate)
                                    refreshrate = dmScreenSettings.dmDisplayFrequency;
                ++i;
            }
            // fill refresh rate info for screen mode change
            dmScreenSettings.dmDisplayFrequency = refreshrate;
            dmScreenSettings.dmFields = DM_DISPLAYFREQUENCY;
        }
        dmScreenSettings.dmPelsWidth = width; // selected screen width
        dmScreenSettings.dmPelsHeight = height; // selected screen height
        dmScreenSettings.dmBitsPerPel = bits; // selected bits per pixel
        dmScreenSettings.dmFields =
            dmScreenSettings.dmFields | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

        // Try to set selected mode and get results.  NOTE: CDS_FULLSCREEN gets rid of start bar.
        if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
        {
            // If the mode fails, offer two options.  Quit or use windowed mode.
            ErrorLog("Fail: full screen");
            if (MessageBox(NULL, "The requested fullscreen mode is not supported by\nyour video "
                                 "card. Use windowed mode instead?",
                           "EU07", MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
            {
                fullscreen = FALSE; // Windowed Mode Selected.  Fullscreen = FALSE
            }
            else
            {
                // Pop Up A Message Box Letting User Know The Program Is Closing.
                Error("Program will now close.");
                return FALSE; // Return FALSE
            }
        }
    }

    if (fullscreen) // Are We Still In Fullscreen Mode?
    {
        dwExStyle = WS_EX_APPWINDOW; // Window Extended Style
        dwStyle = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN; // Windows Style
        ShowCursor(FALSE); // Hide Mouse Pointer
    }
    else
    {
        dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE; // Window Extended Style
        dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN; // Windows Style
    }

    AdjustWindowRectEx(&WindowRect, dwStyle, FALSE,
                       dwExStyle); // Adjust Window To True Requested Size

    // Create The Window
    if (NULL ==
        (hWnd = CreateWindowEx(dwExStyle, // Extended Style For The Window
                               "EU07", // Class Name
                               title, // Window Title
                               dwStyle | // Defined Window Style
                                   WS_CLIPSIBLINGS | // Required Window Style
                                   WS_CLIPCHILDREN, // Required Window Style
                               0,
                               0, // Window Position
                               WindowRect.right - WindowRect.left, // Calculate Window Width
                               WindowRect.bottom - WindowRect.top, // Calculate Window Height
                               NULL, // No Parent Window
                               NULL, // No Menu
                               hInstance, // Instance
                               NULL))) // Dont Pass Anything To WM_CREATE
    {
        KillGLWindow(); // Reset The Display
        ErrorLog("Fail: window creation");
        MessageBox(NULL, "Window creation error.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
        return FALSE; // Return FALSE
    }

    static PIXELFORMATDESCRIPTOR pfd = // pfd Tells Windows How We Want Things To Be
        {
         sizeof(PIXELFORMATDESCRIPTOR), // Size Of This Pixel Format Descriptor
         1, // Version Number
         PFD_DRAW_TO_WINDOW | // Format Must Support Window
             PFD_SUPPORT_OPENGL | // Format Must Support OpenGL
             PFD_DOUBLEBUFFER, // Must Support Double Buffering
         PFD_TYPE_RGBA, // Request An RGBA Format
         bits, // Select Our Color Depth
         0,
         0, 0, 0, 0, 0, // Color Bits Ignored
         0, // No Alpha Buffer
         0, // Shift Bit Ignored
         0, // No Accumulation Buffer
         0, 0, 0, 0, // Accumulation Bits Ignored
         24, // 32Bit Z-Buffer (Depth Buffer)
         0, // No Stencil Buffer
         0, // No Auxiliary Buffer
         PFD_MAIN_PLANE, // Main Drawing Layer
         0, // Reserved
         0, 0, 0 // Layer Masks Ignored
        };

    if (NULL == (hDC = GetDC(hWnd))) // Did We Get A Device Context?
    {
        KillGLWindow(); // Reset The Display
        ErrorLog("Fail: device context");
        MessageBox(NULL, "Can't create a GL device context.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
        return FALSE; // Return FALSE
    }

    /*
     Our first pass, Multisampling hasn't been created yet, so we create a window normally
     If it is supported, then we're on our second pass
     that means we want to use our pixel format for sampling
     so set PixelFormat to arbMultiSampleformat instead
    */
    if (!arbMultisampleSupported)
    {
        if (NULL == (PixelFormat =
                         ChoosePixelFormat(hDC, &pfd))) // Did Windows Find A Matching Pixel Format?
        {
            KillGLWindow(); // Reset The Display
            ErrorLog("Fail: pixelformat");
            MessageBox(NULL, "Can't find a suitable pixelformat.", "ERROR",
                       MB_OK | MB_ICONEXCLAMATION);
            return FALSE; // Return FALSE
        }
    }
    else
        PixelFormat = arbMultisampleFormat;

    if (!SetPixelFormat(hDC, PixelFormat, &pfd)) // Are We Able To Set The Pixel Format?
    {
        KillGLWindow(); // Reset The Display
        ErrorLog("Fail: pixelformat");
        MessageBox(NULL, "Can't set the pixelformat.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
        return FALSE; // Return FALSE
    }

    if (NULL == (hRC = wglCreateContext(hDC))) // Are We Able To Get A Rendering Context?
    {
        KillGLWindow(); // Reset The Display
        ErrorLog("Fail: OpenGL rendering context creation");
        MessageBox(NULL, "Can't create a GL rendering context.", "ERROR",
                   MB_OK | MB_ICONEXCLAMATION);
        return FALSE; // Return FALSE
    }

    if (!wglMakeCurrent(hDC, hRC)) // Try To Activate The Rendering Context
    {
        KillGLWindow(); // Reset The Display
        ErrorLog("Fail: OpenGL rendering context activation");
        MessageBox(NULL, "Can't activate the GL rendering context.", "ERROR",
                   MB_OK | MB_ICONEXCLAMATION);
        return FALSE; // Return FALSE
    }

    /*
    Now that our window is created, we want to queary what samples are available
    we call our InitMultiSample window
    if we return a valid context, we want to destroy our current window
    and create a new one using the multisample interface.
    */
    if (Global::iMultisampling)
        if (!arbMultisampleSupported)
            if ((Global::iMultisampling =
                     InitMultisample(hInstance, hWnd, pfd, 1 << Global::iMultisampling)) != 0)
            {
                // WriteConsoleOnly("Opening second window for multisampling of
                // "+AnsiString(Global::iMultisampling)+" samples.");
                KillGLWindow(); // reset the display
                return CreateGLWindow(title, width, height, bits, fullscreenflag); // rekurencja
            }

    ShowWindow(hWnd, SW_SHOW); // show the window
    SetForegroundWindow(hWnd); // slightly higher priority
    SetFocus(hWnd); // sets keyboard focus to the window
    ReSizeGLScene(width, height); // set up our perspective GL screen

    if (!InitGL()) // initialize our newly created GL Window
    {
        KillGLWindow(); // reset the display
        ErrorLog("Fail: OpenGL initialization");
        MessageBox(NULL, "Initialization Failed.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
        return FALSE; // return FALSE
    }
    return TRUE; // success
}

static int mx = 0, my = 0;
static POINT mouse;

static int test = 0;
/**/
// ************ Globals ************
//
#define MYDISPLAY 1

PCOPYDATASTRUCT pDane;

LRESULT CALLBACK WndProc(HWND hWnd, // handle for this window
                         UINT uMsg, // message for this window
                         WPARAM wParam, // additional message information
                         LPARAM lParam) // additional message information
{
    RECT rect;
    switch (uMsg) // check for windows messages
    {
    case WM_PASTE: //[Ctrl]+[V] potrzebujemy do innych celów
        return 0;
    case WM_COPYDATA: // obsługa danych przesłanych przez program sterujący
        pDane = (PCOPYDATASTRUCT)lParam;
        if (pDane->dwData == 'EU07') // sygnatura danych
            World.OnCommandGet((DaneRozkaz *)(pDane->lpData));
        break;
    case WM_ACTIVATE: // watch for window activate message
        // case WM_ACTIVATEAPP:
        { // Ra: uzależnienie aktywności od bycia na wierzchu
            Global::bActive = (LOWORD(wParam) != WA_INACTIVE);
            if (Global::bInactivePause) // jeśli ma być pauzowanie okna w tle
                if (Global::bActive)
                    Global::iPause &= ~4; // odpauzowanie, gdy jest na pierwszym planie
                else
                    Global::iPause |= 4; // włączenie pauzy, gdy nieaktywy
            if (Global::bActive)
                SetCursorPos(mx, my);
            ShowCursor(!Global::bActive);
            /*
               if (!HIWORD(wParam))	//check minimization state
                    active=TRUE;	//program is active
               else
                    active=FALSE;	//program is no longer active
            */
            return 0; // return to the message loop
        }
    case WM_SYSCOMMAND: // intercept system commands
    {
        switch (wParam) // check system calls
        {
        case 61696: // F10
            World.OnKeyDown(VK_F10);
            return 0;
        case SC_SCREENSAVE: // screensaver trying to start?
        case SC_MONITORPOWER: // monitor trying to enter powersave?
            return 0; // prevent from happening
        }
        break; // exit
    }
    case WM_CLOSE: // did we receive a close message?
    {
        PostQuitMessage(0); // send a quit message [Alt]+[F4]
        return 0; // jump back
    }
    case WM_MOUSEMOVE:
    {
        // mx= 100;//Global::iWindowWidth/2;
        // my= 100;//Global::iWindowHeight/2;
        // SetCursorPos(Global::iWindowWidth/2,Global::iWindowHeight/2);
        // m_x= LOWORD(lParam);
        // m_y= HIWORD(lParam);
        GetCursorPos(&mouse);
        if (Global::bActive && ((mouse.x != mx) || (mouse.y != my)))
        {
            World.OnMouseMove(double(mouse.x - mx) * 0.005, double(mouse.y - my) * 0.01);
            SetCursorPos(mx, my);
        }
        return 0; // jump back
    }
    case WM_KEYUP:
        if (Global::bActive)
        {
            World.OnKeyUp(wParam);
            return 0;
        }
    case WM_KEYDOWN:
        if (Global::bActive)
        {
            if (wParam != 17) // bo naciśnięcia [Ctrl] nie ma po co przekazywać
                if (wParam != 145) //[Scroll Lock] też nie
                    World.OnKeyDown(wParam);
            switch (wParam)
            {
            case VK_ESCAPE: //[Esc] pauzuje tylko bez Debugmode
                if (DebugModeFlag)
                    break;
            case 19: //[Pause]
                if (Global::iPause & 1) // jeśli pauza startowa
                    Global::iPause &= ~1; // odpauzowanie, gdy po wczytaniu miało nie startować
                else if (!(Global::iMultiplayer & 2)) // w multiplayerze pauza nie ma sensu
                    if (!Console::Pressed(VK_CONTROL)) // z [Ctrl] to radiostop jest
                        // Ra: poniższe nie ma sensu, bo brak komunikacji natychmiast zapauzuje
                        // ponownie
                        // if (Global::iPause&8) //jeśli pauza związana z brakiem komunikacji z
                        // PoKeys
                        // Global::iPause&=~10; //odpauzowanie pauzy PoKeys (chyba nic nie da) i
                        // ewentualnie klawiszowej również
                        // else
                        Global::iPause ^= 2; // zmiana stanu zapauzowania
                if (Global::iPause) // jak pauza
                    Global::iTextMode = VK_F1; // to wyświetlić zegar i informację
                break;
            case VK_F7:
                if (DebugModeFlag)
                { // siatki wyświetlane tyko w trybie testowym
                    Global::bWireFrame = !Global::bWireFrame;
                    ++Global::iReCompile; // odświeżyć siatki
                    // Ra: jeszcze usunąć siatki ze skompilowanych obiektów!
                }
                break;
            }
        }
        return 0; // jump back
    case WM_CHAR:
    {
        /*
           switch ((TCHAR) wParam)
           {
            // case 'q':
            //  done= true;
            //  KillGLWindow();
            //  PostQuitMessage(0);
            //  DestroyWindow( hWnd );
            //  break;
           };
        */
        return 0;
    }
    case WM_SIZE: // resize the OpenGL window
    {
        ReSizeGLScene(LOWORD(lParam), HIWORD(lParam)); // LoWord=Width, HiWord=Height
        if (GetWindowRect(hWnd, &rect))
        { // Ra: zmiana rozmiaru okna bez przesuwania myszy
            // mx=WindowWidth/2+rect.left;    // horizontal position
            // my=WindowHeight/2+rect.top;    // vertical position
            // SetCursorPos(mx,my);
        }
        return 0; // jump back
    }
    case WM_MOVE: // przesuwanie okna?
    {
        mx = WindowWidth / 2 + LOWORD(lParam); // horizontal position
        my = WindowHeight / 2 + HIWORD(lParam); // vertical position
        // SetCursorPos(mx,my);
        break;
    }
    case WM_PAINT:
    { // odrysowanie okna
        break;
    }
    // case WM_ERASEBKGND: //Process this message to keep Windows from erasing background.
    case MM_JOY1BUTTONDOWN:
    {
        // WriteLog("Joystick button "+AnsiString(wParam));
        break;
    }
    case WM_CREATE:
        /* Capture the joystick. If this fails, beep and display
         * error, then quit.
         */
        if (joySetCapture(hWnd, JOYSTICKID1, 0, FALSE))
        {
            // MessageBeep(MB_ICONEXCLAMATION);
            // MessageBox(hWnd,"Couldn't capture the joystick",NULL,MB_OK|MB_ICONEXCLAMATION);
            // return -1;
        }
        break;
    }
    // pass all unhandled messages to DefWindowProc
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
};

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

int WINAPI WinMain(HINSTANCE hInstance, // instance
                   HINSTANCE hPrevInstance, // previous instance
                   LPSTR lpCmdLine, // command line parameters
                   int nCmdShow) // window show state
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

    MSG msg; // windows message structure
    BOOL done = FALSE; // bool variable to exit loop
    fullscreen = true;
    DeleteFile("errors.txt"); // usunięcie starego
    Global::LoadIniFile("eu07.ini"); // teraz dopiero można przejrzeć plik z ustawieniami
    Global::InitKeys("keys.ini"); // wczytanie mapowania klawiszy - jest na stałe

	// hunter-271211: ukrywanie konsoli
    if (Global::iWriteLogEnabled & 2)
    {
        AllocConsole();
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN);
    }
    std::string commandline( lpCmdLine ); // parametry uruchomienia
    if( false == commandline.empty() )
    { // analizowanie parametrďż˝w
		cParser parser( commandline );
		std::string token;
		do {
			parser.getTokens();
			token.clear();
			parser >> token;

            if( token == "-s" )
            { // nazwa scenerii
				parser.getTokens();
				parser >> Global::SceneryFile;
            }
            else if( token == "-v")
            { // nazwa wybranego pojazdu
				parser.getTokens();
				parser >> Global::asHumanCtrlVehicle;
            }
            else if( token == "-modifytga" )
            { // wykonanie modyfikacji wszystkich plików TGA
                Global::iModifyTGA = -1; // specjalny tryb wykonania totalnej modyfikacji
            }
            else if( token == "-e3d" )
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
		while( false == token.empty() );
    }
    /* MC: usunalem tymczasowo bo sie gryzlo z nowym parserem - 8.6.2003
        AnsiString csp=AnsiString(Global::szSceneryFile);
        csp=csp.Delete(csp.Pos(AnsiString(strrchr(Global::szSceneryFile,'/')))+1,csp.Length());
        Global::asCurrentSceneryPath=csp;
    */

    fullscreen = Global::bFullScreen;
    WindowWidth = Global::iWindowWidth;
    WindowHeight = Global::iWindowHeight;
    Bpp = Global::iBpp;
    if (Bpp != 32)
        Bpp = 16;
    // create our OpenGL window
    if (!CreateGLWindow(const_cast<char*>(Global::asHumanCtrlVehicle.c_str()), WindowWidth, WindowHeight, Bpp,
                        fullscreen))
        return 0; // quit if window was not created
    SetForegroundWindow(hWnd);
    // McZapkie: proba przeplukania klawiatury
    Console *pConsole = new Console(); // Ra: nie wiem, czy ma to sens, ale jakoś zainicjowac trzeba
    while (Console::Pressed(VK_F10))
        Error("Keyboard buffer problem - press F10"); // na Windows 98 lubi się to pojawiać
    int iOldSpeed, iOldDelay;
    SystemParametersInfo(SPI_GETKEYBOARDSPEED, 0, &iOldSpeed, 0);
    SystemParametersInfo(SPI_GETKEYBOARDDELAY, 0, &iOldDelay, 0);
    SystemParametersInfo(SPI_SETKEYBOARDSPEED, 20, NULL, 0);
    // SystemParametersInfo(SPI_SETKEYBOARDDELAY,10,NULL,0);
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
        // else
        //{//główna pętla programu
        Console::On(); // włączenie konsoli
        while (!done) // loop that runs while done=FALSE
        {
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) // is there a message waiting?
            {
                if (msg.message == WM_QUIT) // have we received a quit message?
                    done = TRUE; // if so
                else // if not, deal with window messages
                {
                    // if (msg.message==WM_CHAR)
                    // World.OnKeyDown(msg.wParam);
                    TranslateMessage(&msg); // translate the message
                    DispatchMessage(&msg); // dispatch the message
                }
            }
            else // if there are no messages
            {
                // draw the scene, watch for quit messages
                // DrawGLScene()
                // if (!pause)
                // if (Global::bInactivePause?Global::bActive:true) //tak nie, bo spada z góry
                if (World.Update()) // Was There A Quit Received?
                    SwapBuffers(hDC); // Swap Buffers (Double Buffering)
                else
                    done = true; //[F10] or DrawGLScene signalled a quit
            }
        }
        Console::Off(); // wyłączenie konsoli (komunikacji zwrotnej)
    }
    SystemParametersInfo(SPI_SETKEYBOARDSPEED, iOldSpeed, NULL, 0);
    SystemParametersInfo(SPI_SETKEYBOARDDELAY, iOldDelay, NULL, 0);
    delete pConsole; // deaktywania sterownika
	TPythonInterpreter::killInstance();

    // shutdown
    KillGLWindow(); // kill the window
    return (msg.wParam); // exit the program
}
