//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "system.hpp"
#include "classes.hpp"
#include "Globals.h"
#include "opengl/glut.h"
#pragma hdrstop

USERES("EU07.res");
USEUNIT("dumb3d.cpp");
USEUNIT("Camera.cpp");
USEUNIT("Texture.cpp");
USEUNIT("World.cpp");
USELIB("opengl\glut32.lib");
USEUNIT("Model3d.cpp");
USEUNIT("geometry.cpp");
USEUNIT("MdlMngr.cpp");
USEUNIT("Train.cpp");
USEUNIT("wavread.cpp");
USEUNIT("Timer.cpp");
USEUNIT("Event.cpp");
USEUNIT("MemCell.cpp");
USEUNIT("Logs.cpp");
USELIB("DirectX\Dsound.lib");
USEUNIT("Spring.cpp");
USEUNIT("Button.cpp");
USEUNIT("Globals.cpp");
USEUNIT("Gauge.cpp");
USEUNIT("AnimModel.cpp");
USEUNIT("Ground.cpp");
USEUNIT("TrkFoll.cpp");
USEUNIT("Segment.cpp");
USEUNIT("McZapkie\mover.pas");
USEUNIT("Sound.cpp");
USEUNIT("AdvSound.cpp");
USEUNIT("McZapkie\ai_driver.pas");
USEUNIT("Track.cpp");
USEUNIT("DynObj.cpp");
USEUNIT("RealSound.cpp");
USEUNIT("EvLaunch.cpp");
USEUNIT("QueryParserComp.pas");
USEUNIT("Scenery.cpp");
USEUNIT("FadeSound.cpp");
USEUNIT("Traction.cpp");
USEUNIT("TractionPower.cpp");
USEUNIT("parser.cpp");
USEUNIT("sky.cpp");
USEUNIT("AirCoupler.cpp");
USEUNIT("glew.c");
//---------------------------------------------------------------------------
#include "World.h"

HDC		hDC=NULL;			// Private GDI Device Context
HGLRC	hRC=NULL;			// Permanent Rendering Context
HWND	hWnd=NULL;			// Holds Our Window Handle

TWorld World;


bool	active=TRUE;		// Window Active Flag Set To TRUE By Default
bool	fullscreen=TRUE;	// Fullscreen Flag Set To Fullscreen Mode By Default
int WindowWidth= 800;
int WindowHeight= 600;
int Bpp= 32;

LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// Declaration For WndProc

//#include "dbgForm.h"
//---------------------------------------------------------------------------

int InitGL(GLvoid)										// All Setup For OpenGL Goes Here
{
    AllocConsole();
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_GREEN);

#ifdef USE_VERTEX_ARRAYS
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
#endif

    World.Init(hWnd,hDC);
//	if (!LoadGLTextures())								// Jump To Texture Loading Routine
	{
//  		return FALSE;									// If Texture Didn't Load Return FALSE
	}

//    TimerInit(); //initialize timer

//	glClearColor(0.9019f, 0.8588f, 0.7882f, 1);				// Black Background
//	glClearDepth(1.0f);		   							// Depth Buffer Setup

/*	glEnable(GL_TEXTURE_2D);							// Enable Texture Mapping
    glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
	glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations
//  */
/*
	glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK,GL_SPECULAR);

    double m_ambient[4] =  {1,1,1,1};
    double m_diffuse[4] =  {1,1,1,1};
    double m_specular[4] = {1,1,1,1};
    double m_shininess = 10.f;
	glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT, m_ambient );
	glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE, m_diffuse );
	glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, m_specular );
	glMaterialf ( GL_FRONT_AND_BACK, GL_SHININESS, m_shininess );
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);		// Setup The Ambient Light
	glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);		// Setup The Diffuse Light
	glLightfv(GL_LIGHT1, GL_SPECULAR,LightSpecular);	// Setup The Diffuse Light
	glLightfv(GL_LIGHT1, GL_POSITION,LightPosition);	// Position The Light
    //glLightf (GL_LIGHT1, GL_SPOT_CUTOFF, 15.f);       // SPOTLIGHT
    //glLightf (GL_LIGHT1, GL_SPOT_EXPONENT, 128.f);       // SPOTLIGHT

	glEnable(GL_LIGHT1);								// Enable Light One
	glEnable(GL_LIGHTING);
*/
/*
	glFogi(GL_FOG_MODE, fogMode[2]);			        // Fog Mode
	glFogfv(GL_FOG_COLOR, fogColor);					// Set Fog Color
	glFogf(GL_FOG_DENSITY, 0.594f);						// How Dense Will The Fog Be
	glHint(GL_FOG_HINT, GL_NICEST);					    // Fog Hint Value
	glFogf(GL_FOG_START, 10.0f);						// Fog Start Depth
	glFogf(GL_FOG_END, 60.0f);							// Fog End Depth
	glEnable(GL_FOG);									// Enables GL_FOG
  */

    return true;
}
//---------------------------------------------------------------------------

GLvoid ReSizeGLScene(GLsizei width, GLsizei height)		// Resize And Initialize The GL Window
{
    WindowWidth= width;
    WindowHeight= height;
	if (height==0)										// Prevent A Divide By Zero By
	{
		height=1;										// Making Height Equal One
	}

	glViewport(0,0,width,height);						// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f,(GLdouble)width/(GLdouble)height,0.2f,2000.0f);

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
}


//---------------------------------------------------------------------------
GLvoid KillGLWindow(GLvoid)								// Properly Kill The Window
{
	if (hRC)											// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL,NULL))					// Are We Able To Release The DC And RC Contexts?
		{
			MessageBox(NULL,"Release Of DC And RC Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))						// Are We Able To Delete The RC?
		{
			MessageBox(NULL,"Release Rendering Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;										// Set RC To NULL
	}

	if (hDC && !ReleaseDC(hWnd,hDC))					// Are We Able To Release The DC
	{
		MessageBox(NULL,"Release Device Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hDC=NULL;										// Set DC To NULL
	}

	if (hWnd && !DestroyWindow(hWnd))					// Are We Able To Destroy The Window?
	{
		MessageBox(NULL,"Could Not Release hWnd.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hWnd=NULL;										// Set hWnd To NULL
	}

	if (fullscreen)										// Are We In Fullscreen Mode?
	{
         ChangeDisplaySettings(NULL,0);					// If So Switch Back To The Desktop
	 ShowCursor(TRUE);								// Show Mouse Pointer
	}
//    KillFont();
}

/*	This Code Creates Our OpenGL Window.  Parameters Are:					*
 *	title			- Title To Appear At The Top Of The Window				*
 *	width			- Width Of The GL Window Or Fullscreen Mode				*
 *	height			- Height Of The GL Window Or Fullscreen Mode			*
 *	bits			- Number Of Bits To Use For Color (8/16/24/32)			*
 *	fullscreenflag	- Use Fullscreen Mode (TRUE) Or Windowed Mode (FALSE)	*/

BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag)
{
	GLuint		PixelFormat;			// Holds The Results After Searching For A Match
	HINSTANCE	hInstance;				// Holds The Instance Of The Application
	WNDCLASS	wc;						// Windows Class Structure
	DWORD		dwExStyle;				// Window Extended Style
	DWORD		dwStyle;				// Window Style
	RECT		WindowRect;				// Grabs Rectangle Upper Left / Lower Right Values
	WindowRect.left=(long)0;			// Set Left Value To 0
	WindowRect.right=(long)width;		// Set Right Value To Requested Width
	WindowRect.top=(long)0;				// Set Top Value To 0
	WindowRect.bottom=(long)height;		// Set Bottom Value To Requested Height

	fullscreen=fullscreenflag;			// Set The Global Fullscreen Flag

	hInstance			= GetModuleHandle(NULL);				// Grab An Instance For Our Window
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraw On Size, And Own DC For Window.
	wc.lpfnWndProc		= (WNDPROC) WndProc;					// WndProc Handles Messages
	wc.cbClsExtra		= 0;									// No Extra Window Data
	wc.cbWndExtra		= 0;									// No Extra Window Data
	wc.hInstance		= hInstance;							// Set The Instance
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);			// Load The Default Icon
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
	wc.hbrBackground	= NULL;									// No Background Required For GL
	wc.lpszMenuName		= NULL;									// We Don't Want A Menu
	wc.lpszClassName	= "OpenGL";								// Set The Class Name

	if (!RegisterClass(&wc))									// Attempt To Register The Window Class
	{
		MessageBox(NULL,"Failed To Register The Window Class.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;											// Return FALSE
	}

	if (fullscreen)												// Attempt Fullscreen Mode?
	{
		DEVMODE dmScreenSettings;								// Device Mode
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));	// Makes Sure Memory's Cleared
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);		// Size Of The Devmode Structure

		//tolaris-240403: poprawka na odswiezanie monitora
		// locate primary monitor...
		if (Global::bAdjustScreenFreq)
		{
			POINT point; point.x = 0; point.y = 0;
			MONITORINFOEX monitorinfo; monitorinfo.cbSize = sizeof( MONITORINFOEX);
			::GetMonitorInfo( ::MonitorFromPoint( point, MONITOR_DEFAULTTOPRIMARY), &monitorinfo );
			//  ..and query for highest supported refresh rate
			unsigned int refreshrate = 0;
			int i = 0;
			while( ::EnumDisplaySettings( monitorinfo.szDevice, i, &dmScreenSettings ) )
			{
				if( i > 0 )
					if ( dmScreenSettings.dmPelsWidth  == width &&
						dmScreenSettings.dmPelsHeight == height &&
						dmScreenSettings.dmBitsPerPel == (unsigned int)bits
						)
						if( dmScreenSettings.dmDisplayFrequency > refreshrate )
							refreshrate = dmScreenSettings.dmDisplayFrequency;
				++i;
			}
			// fill refresh rate info for screen mode change
			dmScreenSettings.dmDisplayFrequency = refreshrate;
			dmScreenSettings.dmFields = DM_DISPLAYFREQUENCY;
		}                
		dmScreenSettings.dmPelsWidth	= width;				// Selected Screen Width
		dmScreenSettings.dmPelsHeight	= height;				// Selected Screen Height
		dmScreenSettings.dmBitsPerPel	= bits;					// Selected Bits Per Pixel
		dmScreenSettings.dmFields = dmScreenSettings.dmFields | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		// Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
		if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
		{
			// If The Mode Fails, Offer Two Options.  Quit Or Use Windowed Mode.
			if (MessageBox(NULL,"The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?","EU07",MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
			{
				fullscreen=FALSE;		// Windowed Mode Selected.  Fullscreen = FALSE
			}
			else
			{
				// Pop Up A Message Box Letting User Know The Program Is Closing.
				Error("Program Will Now Close.");
				return FALSE;									// Return FALSE
			}
		}
	}

	if (fullscreen)												// Are We Still In Fullscreen Mode?
	{
		dwExStyle=WS_EX_APPWINDOW;								// Window Extended Style
		dwStyle=WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;	// Windows Style
		ShowCursor(FALSE);										// Hide Mouse Pointer
	}
	else
	{
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;						// Window Extended Style
		dwStyle=WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;	// Windows Style
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);		// Adjust Window To True Requested Size

	// Create The Window
	if (!(hWnd=CreateWindowEx(	dwExStyle,							// Extended Style For The Window
								"OpenGL",							// Class Name
								title,								// Window Title
								dwStyle |							// Defined Window Style
								WS_CLIPSIBLINGS |					// Required Window Style
								WS_CLIPCHILDREN,					// Required Window Style
								0, 0,								// Window Position
								WindowRect.right-WindowRect.left,	// Calculate Window Width
								WindowRect.bottom-WindowRect.top,	// Calculate Window Height
								NULL,								// No Parent Window
								NULL,								// No Menu
								hInstance,							// Instance
								NULL)))								// Dont Pass Anything To WM_CREATE
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Window Creation Error.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	static	PIXELFORMATDESCRIPTOR pfd=				// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
		1,											// Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,							// Must Support Double Buffering
		PFD_TYPE_RGBA,								// Request An RGBA Format
		bits,										// Select Our Color Depth
		0, 0, 0, 0, 0, 0,							// Color Bits Ignored
		0,											// No Alpha Buffer
		0,											// Shift Bit Ignored
		0,											// No Accumulation Buffer
		0, 0, 0, 0,									// Accumulation Bits Ignored
		24,			        							// 32Bit Z-Buffer (Depth Buffer)
		0,											// No Stencil Buffer
		0,											// No Auxiliary Buffer
		PFD_MAIN_PLANE,								// Main Drawing Layer
		0,											// Reserved
		0, 0, 0										// Layer Masks Ignored
	};

	if (!(hDC=GetDC(hWnd)))							// Did We Get A Device Context?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Create A GL Device Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))	// Did Windows Find A Matching Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if(!SetPixelFormat(hDC,PixelFormat,&pfd))		// Are We Able To Set The Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!(hRC=wglCreateContext(hDC)))				// Are We Able To Get A Rendering Context?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if(!wglMakeCurrent(hDC,hRC))					// Try To Activate The Rendering Context
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	ShowWindow(hWnd,SW_SHOW);						// Show The Window
	SetForegroundWindow(hWnd);						// Slightly Higher Priority
	SetFocus(hWnd);									// Sets Keyboard Focus To The Window
	ReSizeGLScene(width, height);					// Set Up Our Perspective GL Screen

	if (!InitGL())									// Initialize Our Newly Created GL Window
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Initialization Failed.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}



	return TRUE;									// Success
}

static int mx=0, my=0;
static POINT mouse;

static int test= 0;

LRESULT CALLBACK WndProc(	HWND	hWnd,			// Handle For This Window
							UINT	uMsg,			// Message For This Window
							WPARAM	wParam,			// Additional Message Information
							LPARAM	lParam)			// Additional Message Information
{
    TRect rect;
	switch (uMsg)									// Check For Windows Messages
	{
		case WM_ACTIVATE:							// Watch For Window Activate Message
		{
            active= (LOWORD(wParam)!=WA_INACTIVE);
            if (active)
                SetCursorPos(mx,my);
            ShowCursor(!active);
/*
			if (!HIWORD(wParam))					// Check Minimization State
			{
				active=TRUE;						// Program Is Active
			}
			else
			{
				active=FALSE;						// Program Is No Longer Active
			}*/

			return 0;								// Return To The Message Loop
		}

		case WM_SYSCOMMAND:							// Intercept System Commands
		{
			switch (wParam)							// Check System Calls
			{
				case SC_SCREENSAVE:					// Screensaver Trying To Start?
				case SC_MONITORPOWER:				// Monitor Trying To Enter Powersave?
				return 0;							// Prevent From Happening
			}
			break;									// Exit
		}

		case WM_CLOSE:								// Did We Receive A Close Message?
		{
//            delete Form1;
			PostQuitMessage(0);						// Send A Quit Message
			return 0;								// Jump Back
		}

        case WM_MOUSEMOVE:
        {
//            mx= 100;//Global::iWindowWidth/2;
  //          my= 100;//Global::iWindowHeight/2;
        //        SetCursorPos(Global::iWindowWidth/2,Global::iWindowHeight/2);
//            m_x= LOWORD(lParam);
  //          m_y= HIWORD(lParam);
            GetCursorPos(&mouse);



            if (active && ((mouse.x!=mx) || (mouse.y!=my)))
            {
                World.OnMouseMove(double(mouse.x-mx)*0.005,double(mouse.y-my)*0.01);
                SetCursorPos(mx,my);
            }
			return 0;								// Jump Back
        };

        case WM_KEYUP :
        {
            return 0;
        };
                
        case WM_KEYDOWN :
        {
            World.OnKeyPress(wParam);
            switch (wParam)
            {
                case VK_F7:
                    Global::bWireFrame= !Global::bWireFrame;
                break;
            }
			return 0;								// Jump Back
        };
        case WM_CHAR:
        {
            switch ((TCHAR) wParam)
            {
                case VK_ESCAPE:
                    active= !active;
                    break;
//                case 'q':
//                    done= true;
//                    KillGLWindow();
//                    PostQuitMessage(0);
//                    DestroyWindow( hWnd );
//                    break;
            };
            return 0;
        }


		case WM_SIZE:								// Resize The OpenGL Window
		{
			ReSizeGLScene(LOWORD(lParam),HIWORD(lParam));  // LoWord=Width, HiWord=Height
            if (GetWindowRect(hWnd,&rect))
            {
                mx= WindowWidth/2+rect.left;    // horizontal position
                my= WindowHeight/2+rect.top;    // vertical position
//                SetCursorPos(mx,my);
            }
			return 0;								// Jump Back
		}

		case WM_MOVE:
		{
            mx= WindowWidth/2+LOWORD(lParam);    // horizontal position
            my= WindowHeight/2+HIWORD(lParam);    // vertical position
//            SetCursorPos(mx,my);
        }

	}

	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}




int WINAPI WinMain(	HINSTANCE	hInstance,			// Instance
					HINSTANCE	hPrevInstance,		// Previous Instance
					LPSTR		lpCmdLine,			// Command Line Parameters
					int			nCmdShow)			// Window Show State
{

    MSG		msg;									// Windows Message Structure
	BOOL	done=FALSE;     						// Bool Variable To Exit Loop

//    Form1= new TForm1(NULL);
  //  Form1->Show();

    fullscreen=true;

    DecimalSeparator= '.';

    Global::LoadIniFile();


    Global::InitKeys();

//    if (FileExists(lpCmdLine))
    AnsiString str;
    str=lpCmdLine;
    if (str!=AnsiString(""))
     {
      TQueryParserComp *Parser;
      Parser= new TQueryParserComp(NULL);
      Parser->TextToParse= lpCmdLine;
      Parser->First();
      while (!Parser->EndOfFile)
          {
              str= Parser->GetNextSymbol().LowerCase();
              if (str==AnsiString("-s"))
               {
                 str= Parser->GetNextSymbol().LowerCase();
                 strcpy(Global::szSceneryFile,str.c_str());
               }
              else
              if (str==AnsiString("-v"))
               {
                 str= Parser->GetNextSymbol().LowerCase();
                 Global::asHumanCtrlVehicle= str;
               }
              else
               Error("Program usage: EU07 [-s sceneryfilepath] [-v vehiclename]");
          }
          //ABu 050205: tego wczesniej nie bylo:
          delete Parser;
     }

/* MC: usunalem tymczasowo bo sie gryzlo z nowym parserem - 8.6.2003
    AnsiString csp=AnsiString(Global::szSceneryFile);
    csp=csp.Delete(csp.Pos(AnsiString(strrchr(Global::szSceneryFile,'/')))+1,csp.Length());
    Global::asCurrentSceneryPath= csp;
*/

    fullscreen= Global::bFullScreen;
    WindowWidth= Global::iWindowWidth;
    WindowHeight= Global::iWindowHeight;
    Bpp= Global::iBpp;
    if (Bpp!=32)
        Bpp= 16;
    // Create Our OpenGL Window
	if (!CreateGLWindow(Global::asHumanCtrlVehicle.c_str(),WindowWidth,WindowHeight,Bpp,fullscreen))
	{
		return 0;									// Quit If Window Was Not Created
	}
    SetForegroundWindow(hWnd);

    glewInit();
//McZapkie: proba przeplukania klawiatury
    while (Pressed(VK_F10))
     {
      Error("Keyboard buffer problem - press F10");
     }

    int iOldSpeed, iOldDelay;
    SystemParametersInfo(SPI_GETKEYBOARDSPEED,0,&iOldSpeed,0);
    SystemParametersInfo(SPI_GETKEYBOARDDELAY,0,&iOldDelay,0);

    SystemParametersInfo(SPI_SETKEYBOARDSPEED,20,NULL,0);
//    SystemParametersInfo(SPI_SETKEYBOARDDELAY,10,NULL,0);


	while(!done)									// Loop That Runs While done=FALSE
	{
		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))	// Is There A Message Waiting?
		{
			if (msg.message==WM_QUIT)				// Have We Received A Quit Message?
			{
				done=TRUE;							// If So done=TRUE
			}
			else									// If Not, Deal With Window Messages
			{
//                if (msg.message==WM_CHAR)
  //                  World.OnKeyPress(msg.wParam);
				TranslateMessage(&msg);				// Translate The Message
				DispatchMessage(&msg);				// Dispatch The Message
			}
		}
		else										// If There Are No Messages
		{

            // Draw The Scene.  Watch for Quit Messages
//            DrawGLScene()
            if (active)
    			if (World.Update()) 	                // Was There A Quit Received?
    				SwapBuffers(hDC);					// Swap Buffers (Double Buffering)
    			else
		    		done=TRUE;							// ESC or DrawGLScene Signalled A Quit
		}
	}

    SystemParametersInfo(SPI_SETKEYBOARDSPEED,iOldSpeed,NULL,0);
    SystemParametersInfo(SPI_SETKEYBOARDDELAY,iOldDelay,NULL,0);

	// Shutdown
	KillGLWindow();									// Kill The Window
    return (msg.wParam);							// Exit The Program
}


