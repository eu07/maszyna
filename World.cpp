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

#include    "system.hpp"
#include    "classes.hpp"

#include "opengl/glew.h"
#include "opengl/glut.h"
#pragma hdrstop

#include "Timer.h"
#include "mtable.hpp"
#include "Sound.h"
#include "World.h"
#include "logs.h"
#include "Globals.h"
#include "Camera.h"
#include "ResourceManager.h"

#define TEXTURE_FILTER_CONTROL_EXT      0x8500
#define TEXTURE_LOD_BIAS_EXT            0x8501

using namespace Timer;

const double fMaxDt= 0.01;

__fastcall TWorld::TWorld()
{

//    randomize();
  //  Randomize();
    Train= NULL;
    Aspect= 1;
    for (int i=0; i<10; i++)
        KeyEvents[i]= NULL;
    Global::slowmotion=false;
    Global::changeDynObj=false;
    lastmm=61; //ABu: =61, zeby zawsze inicjowac kolor czarnej mgly przy warunku (GlobalTime->mm!=lastmm) :)
    OutText1 = "";
    OutText2 = "";
    OutText3 = "";
}

__fastcall TWorld::~TWorld()
{
    SafeDelete(Train);
    TSoundsManager::Free();
    TModelsManager::Free();
    TTexturesManager::Free();
    glDeleteLists(base, 96);

}

GLvoid __fastcall TWorld::glPrint(const char *fmt)					// Custom GL "Print" Routine
{
//	char		text[256];								// Holds Our String
//	va_list		ap;										// Pointer To List Of Arguments

	if (fmt == NULL)									// If There's No Text
		return;											// Do Nothing

//	va_start(ap, fmt);									// Parses The String For Variables
//	    vsprintf(text, fmt, ap);						// And Converts Symbols To Actual Numbers
//	va_end(ap);											// Results Are Stored In Text

	glPushAttrib(GL_LIST_BIT);							// Pushes The Display List Bits
	glListBase(base - 32);								// Sets The Base Character to 32
	glCallLists(strlen(fmt), GL_UNSIGNED_BYTE, fmt);	// Draws The Display List Text
//	glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);	// Draws The Display List Text
	glPopAttrib();										// Pops The Display List Bits
}


TDynamicObject *Controlled= NULL;

__fastcall TWorld::Init(HWND NhWnd, HDC hDC)
{

    Global::detonatoryOK=true;
    WriteLog("Starting MaSzyna rail vehicle simulator.");
    WriteLog("Compilation 2010-12-11");
    WriteLog("Online documentation and additional files on http://www.eu07.pl");
    WriteLog("Authors: Marcin_EU, McZapkie, ABu, Winger, Tolaris, nbmx_EU, OLO_EU, Bart, Quark-t, ShaXbee, Oli_EU, youBy and others");
    WriteLog("Renderer:");
    WriteLog( (char*) glGetString(GL_RENDERER));
    WriteLog("Vendor:");
//    WriteLog( (char*) glGetString(GL_VENDOR));
//    WriteLog("OpenGl Version::");
//    WriteLog( (char*) glGetString(GL_VERSION));

//Winger030405: sprawdzanie sterownikow
    WriteLog( (char*) glGetString(GL_VENDOR));
    AnsiString glver=((char*)glGetString(GL_VERSION));
    WriteLog("OpenGL Version:");
    WriteLog(glver.c_str());
    if ((glver=="1.5.1") || (glver=="1.5.2"))
    {
       Error("Niekompatybilna wersja openGL - dwuwymiarowy tekst nie bedzie wyswietlany!");
       WriteLog("WARNING! This OpenGL version is not fully compatible with simulator!");
       WriteLog("UWAGA! Ta wersja OpenGL nie jest w pelni kompatybilna z symulatorem!");
       Global::detonatoryOK=false;
    }
    else
       Global::detonatoryOK=true;
    WriteLog("Supported Extensions:");
    WriteLog( (char*) glGetString(GL_EXTENSIONS));

/*-----------------------Render Initialization----------------------*/
        glTexEnvf(TEXTURE_FILTER_CONTROL_EXT,TEXTURE_LOD_BIAS_EXT,-1);
        GLfloat  FogColor[]    = { 1.0f,  1.0f, 1.0f, 1.0f };
        glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear screen and depth buffer
    glLoadIdentity();

    WriteLog("glClearColor (FogColor[0], FogColor[1], FogColor[2], 0.0); ");
//    glClearColor (1.0, 0.0, 0.0, 0.0);                  // Background Color
//    glClearColor (FogColor[0], FogColor[1], FogColor[2], 0.0);                  // Background Color
    glClearColor (0.2, 0.4, 0.33, 1.0);                  // Background Color

    WriteLog("glFogfv(GL_FOG_COLOR, FogColor);");
    glFogfv(GL_FOG_COLOR, FogColor);					// Set Fog Color

    WriteLog("glClearDepth(1.0f);  ");
    glClearDepth(1.0f);                                 // ZBuffer Value


  //  glEnable(GL_NORMALIZE);
//    glEnable(GL_RESCALE_NORMAL);

//    glEnable(GL_CULL_FACE);
    WriteLog("glEnable(GL_TEXTURE_2D);");
	glEnable(GL_TEXTURE_2D);							// Enable Texture Mapping
    WriteLog("glShadeModel(GL_SMOOTH);");
    glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
    WriteLog("glEnable(GL_DEPTH_TEST);");
    glEnable(GL_DEPTH_TEST);

//McZapkie:261102-uruchomienie polprzezroczystosci (na razie linie) pod kierunkiem Marcina
    if (Global::bRenderAlpha)
    {
      WriteLog("glEnable(GL_ALPHA_TEST);");
      glEnable(GL_BLEND);
      WriteLog("glEnable(GL_ALPHA_TEST);");
      glEnable(GL_ALPHA_TEST);
      WriteLog("glAlphaFunc(GL_GREATER,0.04);");
      glAlphaFunc(GL_GREATER,0.04);
      WriteLog("glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);");
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      WriteLog("glDepthFunc(GL_LEQUAL);");
      glDepthFunc(GL_LEQUAL);
    }
    else
    {
      WriteLog("glEnable(GL_ALPHA_TEST);");
      glEnable(GL_ALPHA_TEST);
      WriteLog("glAlphaFunc(GL_GREATER,0.5);");
      glAlphaFunc(GL_GREATER,0.5);
      WriteLog("glDepthFunc(GL_LEQUAL);");
      glDepthFunc(GL_LEQUAL);
      WriteLog("glDisable(GL_BLEND);");
      glDisable(GL_BLEND);
    }
/* zakomentowanie to co bylo kiedys mieszane
    WriteLog("glEnable(GL_ALPHA_TEST);");
    glEnable(GL_ALPHA_TEST);//glGetIntegerv()
    WriteLog("glAlphaFunc(GL_GREATER,0.5);");
//    glAlphaFunc(GL_LESS,0.5);
    glAlphaFunc(GL_GREATER,0.5);
//    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    WriteLog("glDepthFunc(GL_LEQUAL);");
    glDepthFunc(GL_LEQUAL);//EQUAL);								// The Type Of Depth Testing To Do
  //  glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
*/

    WriteLog("glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);");
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations


    WriteLog("glPolygonMode(GL_FRONT, GL_FILL);");
    glPolygonMode(GL_FRONT, GL_FILL);
    WriteLog("glFrontFace(GL_CCW);");
	glFrontFace(GL_CCW);		// Counter clock-wise polygons face out
    WriteLog("glEnable(GL_CULL_FACE);	");
	glEnable(GL_CULL_FACE);		// Cull back-facing triangles
    WriteLog("glLineWidth(1.0f);");
	glLineWidth(1.0f);
//	glLineWidth(2.0f);
    WriteLog("glPointSize(2.0f);");
	glPointSize(2.0f);

// ----------- LIGHTING SETUP -----------
	// Light values and coordinates

    vector3 lp= Normalize(vector3(-500,500,200));

    Global::lightPos[0]= lp.x;
    Global::lightPos[1]= lp.y;
    Global::lightPos[2]= lp.z;
    Global::lightPos[3]= 0.0f;

	// Setup and enable light 0
    WriteLog("glLightModelfv(GL_LIGHT_MODEL_AMBIENT,ambientLight);");
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT,Global::ambientDayLight);
//	glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);
    WriteLog("glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);");
	glLightfv(GL_LIGHT0,GL_DIFFUSE,Global::diffuseDayLight);
    WriteLog("glLightfv(GL_LIGHT0,GL_SPECULAR,specularLight);");
	glLightfv(GL_LIGHT0,GL_SPECULAR,Global::specularDayLight);
    WriteLog("glLightfv(GL_LIGHT0,GL_POSITION,lightPos);");
   	glLightfv(GL_LIGHT0,GL_POSITION,Global::lightPos);
    WriteLog("glEnable(GL_LIGHT0);");
	glEnable(GL_LIGHT0);


	// Enable color tracking
    WriteLog("glEnable(GL_COLOR_MATERIAL);");
	glEnable(GL_COLOR_MATERIAL);

	// Set Material properties to follow glColor values
//    WriteLog("glColorMaterial(GL_FRONT, GL_DIFFUSE);");
//	glColorMaterial(GL_FRONT, GL_DIFFUSE);

    WriteLog("glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);");
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

//    WriteLog("glMaterialfv( GL_FRONT, GL_AMBIENT, whiteLight );");
//	glMaterialfv( GL_FRONT, GL_AMBIENT, Global::whiteLight );

    WriteLog("glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, whiteLight );");
	glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, Global::whiteLight );

/*
    WriteLog("glMaterialfv( GL_FRONT, GL_DIFFUSE, whiteLight );");
	glMaterialfv( GL_FRONT, GL_DIFFUSE, Global::whiteLight );

    WriteLog("glMaterialfv( GL_FRONT, GL_SPECULAR, noLight );");
	glMaterialfv( GL_FRONT, GL_SPECULAR, Global::noLight );
*/

    WriteLog("glEnable(GL_LIGHTING);");
  	glEnable(GL_LIGHTING);


    WriteLog("glFogi(GL_FOG_MODE, GL_LINEAR);");
	glFogi(GL_FOG_MODE, GL_LINEAR);			        // Fog Mode
    WriteLog("glFogfv(GL_FOG_COLOR, FogColor);");
	glFogfv(GL_FOG_COLOR, FogColor);					// Set Fog Color
//	glFogf(GL_FOG_DENSITY, 0.594f);						// How Dense Will The Fog Be
//	glHint(GL_FOG_HINT, GL_NICEST);					    // Fog Hint Value
    WriteLog("glFogf(GL_FOG_START, 1000.0f);");
	glFogf(GL_FOG_START, 10.0f);						// Fog Start Depth
    WriteLog("glFogf(GL_FOG_END, 2000.0f);");
	glFogf(GL_FOG_END, 200.0f);							// Fog End Depth
    WriteLog("glEnable(GL_FOG);");
	glEnable(GL_FOG);									// Enables GL_FOG

/*--------------------Render Initialization End---------------------*/

    WriteLog("Font init");
	HFONT	font;										// Windows Font ID

	base = glGenLists(96);								// Storage For 96 Characters

	font = CreateFont(	-15,							// Height Of Font
						0,								// Width Of Font
						0,								// Angle Of Escapement
						0,								// Orientation Angle
						FW_BOLD,						// Font Weight
						FALSE,							// Italic
						FALSE,							// Underline
						FALSE,							// Strikeout
						ANSI_CHARSET,					// Character Set Identifier
						OUT_TT_PRECIS,					// Output Precision
						CLIP_DEFAULT_PRECIS,			// Clipping Precision
						ANTIALIASED_QUALITY,			// Output Quality
						FF_DONTCARE|DEFAULT_PITCH,		// Family And Pitch
						"Courier New");					// Font Name

	SelectObject(hDC, font);							// Selects The Font We Want

	wglUseFontBitmaps(hDC, 32, 96, base);				// Builds 96 Characters Starting At Character 32


    WriteLog("Font init OK");

    Timer::ResetTimers();

    hWnd= NhWnd;
    glColor4f(1.0f,3.0f,3.0f,0.0f);
//    SwapBuffers(hDC);					// Swap Buffers (Double Buffering)
//    glClear(GL_COLOR_BUFFER_BIT);
//    glFlush();

    SetForegroundWindow(hWnd);
    WriteLog("Sound Init");

    glLoadIdentity();
//    glColor4f(0.3f,0.0f,0.0f,0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glTranslatef(0.0f,0.0f,-0.50f);
//    glRasterPos2f(-0.25f, -0.10f);
    glDisable(GL_DEPTH_TEST);			// Disables depth testing
    glColor3f(3.0f,3.0f,3.0f);

        GLuint logo;
        logo= TTexturesManager::GetTextureID("logo.bmp");
        glBindTexture(GL_TEXTURE_2D, logo);       // Select our texture

	glBegin(GL_QUADS);		        // Drawing using triangles
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.28f, -0.22f,  0.0f);	// Bottom left of the texture and quad
		glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.28f, -0.22f,  0.0f);	// Bottom right of the texture and quad
		glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.28f,  0.22f,  0.0f);	// Top right of the texture and quad
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.28f,  0.22f,  0.0f);	// Top left of the texture and quad
	glEnd();
        ~logo;
    glColor3f(0.0f,0.0f,100.0f);
    if(Global::detonatoryOK)
    {
    glRasterPos2f(-0.25f, -0.09f);
    glPrint("Uruchamianie / Initializing...");
    glRasterPos2f(-0.25f, -0.10f);
    glPrint("Dzwiek / Sound...");
    }
    SwapBuffers(hDC);					// Swap Buffers (Double Buffering)

  	glEnable(GL_LIGHTING);
/*-----------------------Sound Initialization-----------------------*/
    TSoundsManager::Init( hWnd );
    TSoundsManager::LoadSounds( "" );
/*---------------------Sound Initialization End---------------------*/
    WriteLog("Sound Init OK");
    if(Global::detonatoryOK)
    {
    glRasterPos2f(-0.25f, -0.11f);
    glPrint("OK.");
    }
    SwapBuffers(hDC);					// Swap Buffers (Double Buffering)

    int i;

    Paused= true;
    WriteLog("Textures init");
    if(Global::detonatoryOK)
    {
    glRasterPos2f(-0.25f, -0.12f);
    glPrint("Tekstury / Textures...");
    }
    SwapBuffers(hDC);					// Swap Buffers (Double Buffering)

    TTexturesManager::Init();
    WriteLog("Textures init OK");
    if(Global::detonatoryOK)
    {
    glRasterPos2f(-0.25f, -0.13f);
    glPrint("OK.");
    }
    SwapBuffers(hDC);					// Swap Buffers (Double Buffering)

    WriteLog("Models init");
    if(Global::detonatoryOK)
    {
    glRasterPos2f(-0.25f, -0.14f);
    glPrint("Modele / Models...");
    }
    SwapBuffers(hDC);					// Swap Buffers (Double Buffering)
//McZapkie: dodalem sciezke zeby mozna bylo definiowac skad brac modele ale to malo eleganckie
//    TModelsManager::LoadModels(asModelsPatch);
    TModelsManager::Init();
    WriteLog("Models init OK");
    if(Global::detonatoryOK)
    {
    glRasterPos2f(-0.25f, -0.15f);
    glPrint("OK.");
    }
    SwapBuffers(hDC);					// Swap Buffers (Double Buffering)

    WriteLog("Ground init");
    if(Global::detonatoryOK)
    {
    glRasterPos2f(-0.25f, -0.16f);
    glPrint("Sceneria / Scenery (can take a while)...");
    }
    SwapBuffers(hDC);					// Swap Buffers (Double Buffering)

    Ground.Init(Global::szSceneryFile);
//    Global::tSinceStart= 0;
    Clouds.Init();
    WriteLog("Ground init OK");
    if(Global::detonatoryOK)
    {
    glRasterPos2f(-0.25f, -0.17f);
    glPrint("OK.");
    }
    SwapBuffers(hDC);					// Swap Buffers (Double Buffering)

//    TTrack *Track= Ground.FindGroundNode("train_start",TP_TRACK)->pTrack;

//    Camera.Init(vector3(2700,10,6500),0,M_PI,0);
//    Camera.Init(vector3(00,40,000),0,M_PI,0);
//    Camera.Init(vector3(1500,5,-4000),0,M_PI,0);
//McZapkie-130302 - coby nie przekompilowywac:
//      Camera.Init(Global::pFreeCameraInit,0,M_PI,0);
      Camera.Init(Global::pFreeCameraInit,Global::pFreeCameraInitAngle);

    char buff[255]= "Player train init: ";
    if(Global::detonatoryOK)
    {
    glRasterPos2f(-0.25f, -0.18f);
    glPrint("Inicjalizacja wybranego pojazdu do sterowania...");
    }
    SwapBuffers(hDC);					// Swap Buffers (Double Buffering)

    strcat(buff,Global::asHumanCtrlVehicle.c_str());
    WriteLog(buff);
    TGroundNode *PlayerTrain= Ground.FindDynamic(Global::asHumanCtrlVehicle);
    if (PlayerTrain)
     {
//      if (PlayerTrain->DynamicObject->MoverParameters->TypeName==AnsiString("machajka"))
//        Train= new TMachajka();
//      else
//      if (PlayerTrain->DynamicObject->MoverParameters->TypeName==AnsiString("303e"))
//        Train= new TEu07();
//      else
        Train= new TTrain();
      if (Train->Init(PlayerTrain->DynamicObject))
       {
        Controlled= Train->DynamicObject;
        WriteLog("Player train init OK");
        if(Global::detonatoryOK)
        {
        glRasterPos2f(-0.25f, -0.19f);
        glPrint("OK.");
        }
        SwapBuffers(hDC);					// Swap Buffers (Double Buffering)

       }
      else
       {
        Error("Player train init failed!");
        if(Global::detonatoryOK)
        {
        glRasterPos2f(-0.25f, -0.20f);
        glPrint("Blad inicjalizacji sterowanego pojazdu!");
        }
        SwapBuffers(hDC);					// Swap Buffers (Double Buffering)

         Controlled= NULL;
         Camera.Type= tp_Free;
       }
     }
    else
     {
       if (Global::asHumanCtrlVehicle!=AnsiString("ghostview"))
         Error("Player train not exist!");
       if(Global::detonatoryOK)
       {
       glRasterPos2f(-0.25f, -0.20f);
       glPrint("Wybrany pojazd nie istnieje w scenerii!");
       }
       SwapBuffers(hDC);					// Swap Buffers (Double Buffering)

       Controlled= NULL;
       Camera.Type= tp_Free;
     }

    glEnable(GL_DEPTH_TEST);
    Ground.pTrain= Train;
    KeyEvents[0]= Ground.FindEvent("keyctrl00");
    KeyEvents[1]= Ground.FindEvent("keyctrl01");
    KeyEvents[2]= Ground.FindEvent("keyctrl02");
    KeyEvents[3]= Ground.FindEvent("keyctrl03");
    KeyEvents[4]= Ground.FindEvent("keyctrl04");
    KeyEvents[5]= Ground.FindEvent("keyctrl05");
    KeyEvents[6]= Ground.FindEvent("keyctrl06");
    KeyEvents[7]= Ground.FindEvent("keyctrl07");
    KeyEvents[8]= Ground.FindEvent("keyctrl08");
    KeyEvents[9]= Ground.FindEvent("keyctrl09");

    matrix4x4 ident2;
    ident2.Identity();

//  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);  //{Texture blends with object background}
  light= TTexturesManager::GetTextureID("smuga");

//    Camera.Reset();
    ResetTimers();
};


void __fastcall TWorld::OnKeyPress(int cKey)
{

    TGroundNode *tmp;

    if (Pressed(VK_SHIFT) && (cKey>='0' && cKey<='9' && KeyEvents[cKey-'0']))
        Ground.AddToQuery(KeyEvents[cKey-'0'],NULL);

     if (Controlled)
      if ((Controlled->Controller==Humandriver) || DebugModeFlag)
        Train->OnKeyPress(cKey);
}


void __fastcall TWorld::OnMouseMove(double x, double y)
{
//McZapkie:060503-definicja obracania myszy
    Camera.OnCursorMove(x*Global::fMouseXScale,-y*Global::fMouseYScale);
}

//#include <dstring.h>

bool __fastcall TWorld::Update()
{

    vector3 tmpvector = Global::GetCameraPosition();

    tmpvector = vector3(
        - int(tmpvector.x) + int(tmpvector.x) % 10000,
        - int(tmpvector.y) + int(tmpvector.y) % 10000,
        - int(tmpvector.z) + int(tmpvector.z) % 10000);

#ifdef USE_SCENERY_MOVING
    if(tmpvector.x || tmpvector.y || tmpvector.z)
    {
        WriteLog("Moving scenery");
        Ground.MoveGroundNode(tmpvector);
        WriteLog("Scenery moved");
    };
#endif

    if (GetFPS()<12)
          {  Global::slowmotion=true;  }
       else
          {
             if (GetFPS()>15)
                Global::slowmotion=false;
          };

    UpdateTimers();
    GlobalTime->UpdateMTableTime(GetDeltaTime()); //McZapkie-300302: czas rozkladowy

    //Winger - zmiana pory dnia
    //ABu - maly lifting tego, co bylo:
//if (Global::bTimeChange)
//{
    if(lastmm!=GlobalTime->mm)
    {
       double czas, arg;
       float p1, p2, p3, p4, p5, p6, p7;
       if ((GlobalTime->hh<GlobalTime->srh)||(GlobalTime->hh>(GlobalTime->ssh-1)))
          {czas=0; p1=0; arg=0;}
       else
          {
             czas=GlobalTime->mm+(GlobalTime->hh-GlobalTime->srh)*60;
           //p1=(1-cos(((czas/((1440-((24-(SR+(24-SS)))*60))/360))/360)*2*M_PI))*100;
             arg=(czas/((GlobalTime->ssh-GlobalTime->srh)*60))*2*M_PI;
             p1=(1-cos(arg))*10;
          }
       //p1 = GameTime/3000;
       //p2 = GameTime/6.2;
       if(arg<M_PI)
          p2 = p1*16*1.5;
       else
       if(arg<((3*M_PI)/2))
          p2 = 320*1.5;
       else
          p2 = (1-(cos(arg)*cos(arg)))*320*1.5;

       p3 = p1 + 0.0f;
       p4 = p1 + 0.05f;
       p5 = p1 + 0.2f;
       p6 = p2 + 10.0f; //1.5f
       p7 = (p2*3.45) + 125.0f;

       if (p1<0.6f)
       {
       GLfloat  FogColor2[]    = { p3, p4, p5, 1.0f };
       //dzien ladny:
       glClearColor (0.0+p1, 0.05+p1, 0.2+p1, 0.0);
       //dzien brzydki:
       //glClearColor (0.02+p1, 0.02+p1, 0.02+p1, 0.0);
       glFogfv(GL_FOG_COLOR, FogColor2);					// Set Fog Color
       }
       if (p6<300.0f)
       {
       glFogf(GL_FOG_START, p6);						// Fog Start Depth
       glFogf(GL_FOG_END, p7);							// Fog End Depth
       glEnable(GL_FOG);									// Enables GL_FOG
       }
       lastmm=GlobalTime->mm;
    }

    if (Pressed(VK_LBUTTON))// || Pressed(VK_NUMPAD5))
    {

        Camera.Reset();
    //    Camera.Pitch=Camera.Yaw=Camera.Roll= 0;
//        SetCursorPos(Global::iWindowWidth/2,Global::iWindowHeight/2);
    }
    else
    {

    }

   if ((Pressed(VK_RBUTTON)) || (Pressed(VK_F4)))
    {   //ABu 180404 powrot mechanika na siedzenie
        Camera.Reset();
        double dt= GetDeltaTime();
        if (Controlled)
        {
         Camera.Pos= Train->pMechPosition;//Train.GetPosition1();
         Camera.Roll= atan(Train->pMechShake.x*Train->fMechRoll);       //hustanie kamery na boki
         Camera.Pitch-= atan(Train->vMechVelocity.z*Train->fMechPitch);  //hustanie kamery przod tyl
         if (Train->DynamicObject->MoverParameters->ActiveCab==0)
           Camera.LookAt= Train->pMechPosition+Train->GetDirection();
         else  //patrz w strone wlasciwej kabiny
          Camera.LookAt= Train->pMechPosition+Train->GetDirection()*Train->DynamicObject->MoverParameters->ActiveCab;
           Train->pMechOffset.x=Train->pMechSittingPosition.x;
           Train->pMechOffset.y=Train->pMechSittingPosition.y;
           Train->pMechOffset.z=Train->pMechSittingPosition.z;
        if (FreeFlyModeFlag)
         {
         Camera.Pos= Train->pMechPosition+Train->GetDirection()*5+vector3(3,3,3);
         Camera.LookAt= Train->pMechPosition+Train->GetDirection()*-1;
         //Zeby nie bylo numerow z 'fruwajacym' lokiem - konsekwencja bujania pudla
         Train->DynamicObject->ABuSetModelShake(vector3(0,0,0));
         Train->DynamicObject->bDisplayCab= false;
         }
        else
         Train->DynamicObject->bDisplayCab= true;
        }
    }

    if (Pressed(VK_F10))
    {//tu mozna dodac dopisywanie do logu przebiegu lokomotywy
       return false;
    }
    Camera.Update();

    double dt= GetDeltaTime();
    double iter;
    int n= 1;
    if (dt>fMaxDt)
    {
        iter= ceil(dt/fMaxDt);
        n= iter;
        dt= dt/iter;
    }
    if (n>20)
        n= 20; //McZapkie-081103: przesuniecie granicy FPS z 10 na 5
    //blablabla
    //Sleep(50);
    Ground.Update(dt,n); //ABu: zamiast 'n' bylo: 'Camera.Type==tp_Follow'
//        Ground.Update(0.01,Camera.Type==tp_Follow);
    dt= GetDeltaTime();
    if (Camera.Type==tp_Follow)
    {
        Train->UpdateMechPosition(dt);
        Camera.Pos= Train->pMechPosition;//Train.GetPosition1();
        Camera.Roll= atan(Train->pMechShake.x*Train->fMechRoll);       //hustanie kamery na boki
        Camera.Pitch-= atan(Train->vMechVelocity.z*Train->fMechPitch);  //hustanie kamery przod tyl
        //ABu011104: rzucanie pudlem
            vector3 temp;
            if (abs(Train->pMechShake.y)<0.25)
               temp=vector3(0,0,6*Train->pMechShake.y);
            else
               if ((Train->pMechShake.y)>0)
                  temp=vector3(0,0,6*0.25);
               else
                  temp=vector3(0,0,-6*0.25);
            Controlled->ABuSetModelShake(temp);
        //ABu: koniec rzucania

        if (Train->DynamicObject->MoverParameters->ActiveCab==0)
          Camera.LookAt= Train->pMechPosition+Train->GetDirection();
        else  //patrz w strone wlasciwej kabiny
         Camera.LookAt= Train->pMechPosition+Train->GetDirection()*Train->DynamicObject->MoverParameters->ActiveCab;
        Camera.vUp= Train->GetUp();
    }

    Ground.CheckQuery();

    if (!Render())
        return false;

//**********************************************************************************************************
//rendering kabiny gdy jest oddzielnym modelem i ma byc wyswietlana
    vector3 vFront= Train->DynamicObject->GetDirection();
    if ((Train->DynamicObject->MoverParameters->CategoryFlag==2) && (Train->DynamicObject->MoverParameters->ActiveCab<0)) //TODO: zrobic to eleganciej z plynnym zawracaniem
       vFront= -vFront;
    vector3 vUp= vWorldUp;
    vFront.Normalize();
    vector3 vLeft= CrossProduct(vUp,vFront);
    vUp= CrossProduct(vFront,vLeft);
    matrix4x4 mat;
    mat.Identity();
    mat.BasisChange(vLeft,vUp,vFront);
    Train->DynamicObject->mMatrix= Inverse(mat);
    glPushMatrix ( );
    //ABu: Rendering kabiny jako ostatniej, zeby bylo widac przez szyby, tylko w widoku ze srodka
    if ((Train->DynamicObject->mdKabina!=Train->DynamicObject->mdModel) && Train->DynamicObject->bDisplayCab && !FreeFlyModeFlag)
    {
      vector3 pos= Train->DynamicObject->GetPosition();
      glTranslatef(pos.x,pos.y,pos.z);
      glMultMatrixd(Train->DynamicObject->mMatrix.getArray());
      
//*yB: moje smuuugi 1
  float lightsum=0;
  int i;
  for(i=1;i<3;i++)
   {
    lightsum+=Global::diffuseDayLight[i];
    lightsum+=Global::ambientDayLight[i];
   }
  if(lightsum<1)
   {
    glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_ONE);
//    glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_DST_COLOR);
//    glBlendFunc(GL_SRC_ALPHA_SATURATE,GL_ONE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glColor4f(1.0f,1.0f,1.0f,1.0f);
      glBindTexture(GL_TEXTURE_2D, light);       // Select our texture
        glBegin(GL_QUADS);
         if(TestFlag(Train->DynamicObject->MoverParameters->EndSignalsFlag,21))
          {
           glTexCoord2f(0, 0);  glVertex3f( 15.0, 0.0, 15);
           glTexCoord2f(1, 0);  glVertex3f(-15.0, 0.0, 15);
           glTexCoord2f(1, 1);  glVertex3f(-15.0, 2.5, 250);
           glTexCoord2f(0, 1);  glVertex3f( 15.0, 2.5, 250);
          }
         if(TestFlag(Train->DynamicObject->MoverParameters->HeadSignalsFlag,21))
          {
           glTexCoord2f(0, 0);  glVertex3f(-15.0, 0.0, -15);
           glTexCoord2f(1, 0);  glVertex3f( 15.0, 0.0, -15);
           glTexCoord2f(1, 1);  glVertex3f( 15.0, 2.5, -250);
           glTexCoord2f(0, 1);  glVertex3f(-15.0, 2.5, -250);
          }
        glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_FOG);
   }
//yB: moje smuuugi 1 - koniec*/

    //oswietlenie kabiny
      GLfloat  ambientCabLight[4]= { 0.5f,  0.5f, 0.5f, 1.0f };
      GLfloat  diffuseCabLight[4]= { 0.5f,  0.5f, 0.5f, 1.0f };
      GLfloat  specularCabLight[4]= { 0.5f,  0.5f, 0.5f, 1.0f };
      for (int li=0; li<3; li++)
       {
         ambientCabLight[li]= Global::ambientDayLight[li]*0.9;
         diffuseCabLight[li]= Global::diffuseDayLight[li]*0.5;
         specularCabLight[li]= Global::specularDayLight[li]*0.5;
       }
      switch (Train->DynamicObject->MyTrack->eEnvironment)
      {
       case e_canyon:
        {
          for (int li=0; li<3; li++)
           {
             diffuseCabLight[li]*= 0.6;
             specularCabLight[li]*= 0.7;
           }
        }
       break;
       case e_tunnel:
        {
          for (int li=0; li<3; li++)
           {
             ambientCabLight[li]*= 0.3;
             diffuseCabLight[li]*= 0.1;
             specularCabLight[li]*= 0.2;
           }
        }
       break;
      }
      glLightfv(GL_LIGHT0,GL_AMBIENT,ambientCabLight);
      glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseCabLight);
      glLightfv(GL_LIGHT0,GL_SPECULAR,specularCabLight);

      Train->DynamicObject->mdKabina->Render(SquareMagnitude(Global::pCameraPosition-pos),0);
      Train->DynamicObject->mdKabina->RenderAlpha(SquareMagnitude(Global::pCameraPosition-pos),0);
//smierdzi
//      mdModel->Render(SquareMagnitude(Global::pCameraPosition-pos),0);

      glLightfv(GL_LIGHT0,GL_AMBIENT,Global::ambientDayLight);
      glLightfv(GL_LIGHT0,GL_DIFFUSE,Global::diffuseDayLight);
      glLightfv(GL_LIGHT0,GL_SPECULAR,Global::specularDayLight);
    }
    glPopMatrix ( );
//**********************************************************************************************************
    if (DebugModeFlag)
     {
       OutText1= "  FPS: ";
       OutText1+= FloatToStrF(GetFPS(),ffFixed,6,2);
       if(Global::slowmotion)
          {OutText1+= "S";}
       else
          {OutText1+= "N";};


       if (GetDeltaTime()>=0.2)
        {
            OutText1+= " Slowing Down !!! ";
        }
     }
    /*if (Pressed(VK_F5))
       {Global::slowmotion=true;};
    if (Pressed(VK_F6))
       {Global::slowmotion=false;};*/

    if (Pressed(VK_F8))
       {
          OutText1= "  FPS: ";
          OutText1+= FloatToStrF(GetFPS(),ffFixed,6,2);
       }

    //if (Pressed(VK_F7))
    //{
    //  OutText1=FloatToStrF(Controlled->MoverParameters->Couplers[0].CouplingFlag,ffFixed,2,0)+", ";
    //  OutText1+=FloatToStrF(Controlled->MoverParameters->Couplers[1].CouplingFlag,ffFixed,2,0);
    //}


    /*
    if (Pressed(VK_F5))
       {
          int line=2;
          OutText1="Time: "+FloatToStrF(GlobalTime->hh,ffFixed,2,0)+":"
                  +FloatToStrF(GlobalTime->mm,ffFixed,2,0)+", ";
          OutText1+="distance: ";
          OutText1+="34.94";
          OutText2="Next station: ";
          OutText2+=FloatToStrF(Controlled->TrainParams->TimeTable[line].km,ffFixed,2,2)+" km, ";
          OutText2+=AnsiString(Controlled->TrainParams->TimeTable[line].StationName)+", ";
          OutText2+=AnsiString(Controlled->TrainParams->TimeTable[line].StationWare);
          OutText3="Arrival: ";
          if(Controlled->TrainParams->TimeTable[line].Ah==-1)
          {
             OutText3+="--:--";
          }
          else
          {
             OutText3+=FloatToStrF(Controlled->TrainParams->TimeTable[line].Ah,ffFixed,2,0)+":";
             OutText3+=FloatToStrF(Controlled->TrainParams->TimeTable[line].Am,ffFixed,2,0)+" ";
          }
          OutText3+=" Departure: ";
          OutText3+=FloatToStrF(Controlled->TrainParams->TimeTable[line].Dh,ffFixed,2,0)+":";
          OutText3+=FloatToStrF(Controlled->TrainParams->TimeTable[line].Dm,ffFixed,2,0)+" ";
       };
//    */
    /*
    if (Pressed(VK_F6))
    {
       //GlobalTime->UpdateMTableTime(100);
       //OutText1=FloatToStrF(SquareMagnitude(Global::pCameraPosition-Controlled->GetPosition()),ffFixed,10,0);
       //OutText1=FloatToStrF(Global::TnijSzczegoly,ffFixed,7,0)+", ";
       //OutText1+=FloatToStrF(dta,ffFixed,2,4)+", ";
       OutText1+= FloatToStrF(GetFPS(),ffFixed,6,2);
       OutText1+= FloatToStrF(Global::ABuDebug,ffFixed,6,15);
    };
    */
    if (Pressed(VK_F6)&&(DebugModeFlag))
    {
       //OutText1=FloatToStrF(arg,ffFixed,2,4)+", ";
       //OutText1+=FloatToStrF(p2,ffFixed,7,4)+", ";
       GlobalTime->UpdateMTableTime(100);
    }


    if(Global::changeDynObj==true)
    {
       //ABu zmiana pojazdu
       Train->dsbHasler->Stop();
       Train->dsbBuzzer->Stop();
       Train->rsHiss.Stop();
       Train->rsSBHiss.Stop();
       Train->rsRunningNoise.Stop();
       Train->rsBrake.Stop();
       Train->rsEngageSlippery.Stop();
       Train->rsSlippery.Stop();
       Train->DynamicObject->MoverParameters->CabDeactivisation();
       int CabNr;
       TDynamicObject *temp;
       if (Train->DynamicObject->MoverParameters->ActiveCab==-1)
          {
             temp=Train->DynamicObject->NextConnected;
             if (Train->DynamicObject->NextConnectedNo==0)
                {CabNr= 1;}
             else
                {CabNr=-1;}
          }
       else
          {
             temp=Train->DynamicObject->PrevConnected;
             if (Train->DynamicObject->PrevConnectedNo==0)
                {CabNr= 1;}
             else
                {CabNr=-1;}
          }
/*
       TLocation l;
       l.X=l.Y=l.Z= 0;
       TRotation r;
       r.Rx=r.Ry=r.Rz= 0;
*/

       Train->DynamicObject->Controller=AIdriver;
       Train->DynamicObject->bDisplayCab=false;
       Train->DynamicObject->MechInside=false;
       Train->DynamicObject->MoverParameters->SecuritySystem.Status=0;
       Train->DynamicObject->ABuSetModelShake(vector3(0,0,0));
       Train->DynamicObject->MoverParameters->ActiveCab=0;
       Train->DynamicObject->MoverParameters->BrakeCtrlPos=-2;
///       Train->DynamicObject->MoverParameters->LimPipePress=-1;
///       Train->DynamicObject->MoverParameters->ActFlowSpeed=0;
///       Train->DynamicObject->Mechanik->CloseLog();
///       SafeDelete(Train->DynamicObject->Mechanik);

       //Train->DynamicObject->mdKabina=NULL;
       Train->DynamicObject=temp;
       Controlled=Train->DynamicObject;
       Global::asHumanCtrlVehicle=Train->DynamicObject->GetasName();
//       Train->DynamicObject->MoverParameters->BrakeCtrlPos=-2;
       Train->DynamicObject->MoverParameters->LimPipePress=Controlled->MoverParameters->PipePress;
//       Train->DynamicObject->MoverParameters->ActFlowSpeed=0;
       Train->DynamicObject->MoverParameters->SecuritySystem.Status=1;
       Train->DynamicObject->MoverParameters->ActiveCab=CabNr;
       Train->DynamicObject->MoverParameters->CabDeactivisation();
       Train->DynamicObject->Controller=Humandriver;
       Train->DynamicObject->MechInside=true;
///       Train->DynamicObject->Mechanik=new TController(l,r,Controlled->Controller,&Controlled->MoverParameters,&Controlled->TrainParams,Aggressive);
       //Train->InitializeCab(Train->DynamicObject->MoverParameters->CabNo,Train->DynamicObject->asBaseDir+Train->DynamicObject->MoverParameters->TypeName+".mmd");
       Train->InitializeCab(CabNr,Train->DynamicObject->asBaseDir+Train->DynamicObject->MoverParameters->TypeName+".mmd");
       if (!FreeFlyModeFlag) Train->DynamicObject->bDisplayCab=true;
       Global::changeDynObj=false;
    }

    if (Pressed(VK_F1))
    {
       OutText1="Time: "+FloatToStrF(GlobalTime->hh,ffFixed,2,0)+":";
       if((GlobalTime->mm)<10)
          OutText1=OutText1+"0"+FloatToStrF(GlobalTime->mm,ffFixed,2,0);
       else
          OutText1+=FloatToStrF(GlobalTime->mm,ffFixed,2,0);
       OutText2="";
       //double CtrlPos=Controlled->MoverParameters->MainCtrlPos;
       //double CtrlPosNo=Controlled->MoverParameters->MainCtrlPosNo;
       //OutText2="defrot="+FloatToStrF(1+0.4*(CtrlPos/CtrlPosNo),ffFixed,2,5);
       OutText3="";
    }
    if (Pressed(VK_F12))
    {
       //Takie male info :)
       OutText1= AnsiString("Online documentation (PL, ENG, DE, soon CZ): http://www.eu07.pl");
    }

    if (Pressed(VK_F2))
     {
       //ABu: info dla najblizszego pojazdu!
       TDynamicObject *tmp;
       if (FreeFlyModeFlag)
       {
       int CouplNr=-2;
       tmp=Controlled->ABuScanNearestObject(Controlled->GetTrack(), 1, 500, CouplNr);
          if(tmp==NULL)
          {
             tmp=Controlled->ABuScanNearestObject(Controlled->GetTrack(), -1, 500, CouplNr);
             //if(tmp==NULL) tmp=Controlled;
          }
       }
       else
       {
          tmp=Controlled;
       }

       if(tmp!=NULL)
       {
          OutText3="";
          OutText1="Vehicle Name:  "+AnsiString(tmp->MoverParameters->Name);
//yB          OutText1+="; d:  "+FloatToStrF(tmp->ABuGetDirection(),ffFixed,2,0);
          //OutText1=FloatToStrF(tmp->MoverParameters->Couplers[0].CouplingFlag,ffFixed,3,2)+", ";
          //OutText1+=FloatToStrF(tmp->MoverParameters->Couplers[1].CouplingFlag,ffFixed,3,2);
          OutText2="Damage status: "+tmp->MoverParameters->EngineDescription(0);//+" Engine status: ";
          OutText2+="; Brake delay: ";
          if(tmp->MoverParameters->BrakeDelayFlag>0)
           if(tmp->MoverParameters->BrakeDelayFlag>1)
            OutText2+="R";
           else
            OutText2+="G";
          else
           OutText2+="P";
          OutText2+=AnsiString(", BTP:")+FloatToStrF(tmp->MoverParameters->BCMFlag,ffFixed,5,0);
//          OutText2+= FloatToStrF(tmp->MoverParameters->CompressorPower,ffFixed,5,0)+AnsiString(", ");
//yB          if(tmp->MoverParameters->BrakeSubsystem==Knorr) OutText2+=" Knorr";
//yB          if(tmp->MoverParameters->BrakeSubsystem==Oerlikon) OutText2+=" Oerlikon";
//yB          if(tmp->MoverParameters->BrakeSubsystem==Hik) OutText2+=" Hik";
//yB          if(tmp->MoverParameters->BrakeSubsystem==WeLu) OutText2+=" £estingha³s";
          //OutText2= " GetFirst: "+AnsiString(tmp->GetFirstDynamic(1)->MoverParameters->Name)+" Damage status="+tmp->MoverParameters->EngineDescription(0)+" Engine status: ";
          //OutText2+= " GetLast: "+AnsiString(tmp->GetLastDynamic(1)->MoverParameters->Name)+" Damage status="+tmp->MoverParameters->EngineDescription(0)+" Engine status: ";
          OutText3= AnsiString("Brake press: ")+FloatToStrF(tmp->MoverParameters->BrakePress,ffFixed,5,2)+AnsiString(", ");
          OutText3+= AnsiString("Pipe press: ")+FloatToStrF(tmp->MoverParameters->PipePress,ffFixed,5,2)+AnsiString(", ");
          OutText3+= AnsiString("BVP: ")+FloatToStrF(tmp->MoverParameters->BrakeVP(),ffFixed,5,2)+AnsiString(", ");
          OutText3+= FloatToStrF(tmp->MoverParameters->CntrlPipePress,ffFixed,5,2)+AnsiString(", ");
//          OutText3+= FloatToStrF(tmp->MoverParameters->HighPipePress,ffFixed,5,2)+AnsiString(", ");
//          OutText3+= FloatToStrF(tmp->MoverParameters->LowPipePress,ffFixed,5,2)+AnsiString(", ");
          OutText3+= FloatToStrF(tmp->MoverParameters->BrakeStatus,ffFixed,5,0)+AnsiString(", ");          
          OutText3+= AnsiString("Pipe2 press: ")+FloatToStrF(tmp->MoverParameters->ScndPipePress,ffFixed,5,2)+AnsiString(". ");

          if ((tmp->MoverParameters->LocalBrakePos)>0)
             OutText3+= AnsiString("local brake active. ");
          else
             OutText3+= AnsiString("local brake inactive. ");
/*
            //OutText3+= AnsiString("LSwTim: ")+FloatToStrF(tmp->MoverParameters->LastSwitchingTime,ffFixed,5,2);
            //OutText3+= AnsiString(" Physic: ")+FloatToStrF(tmp->MoverParameters->PhysicActivation,ffFixed,5,2);
            //OutText3+= AnsiString(" ESF: ")+FloatToStrF(tmp->MoverParameters->EndSignalsFlag,ffFixed,5,0);
            OutText3+= AnsiString(" dPAngF: ")+FloatToStrF(tmp->dPantAngleF,ffFixed,5,0);
            OutText3+= AnsiString(" dPAngFT: ")+FloatToStrF(-(tmp->PantTraction1*28.9-136.938),ffFixed,5,0);
            if (tmp->lastcabf==1)
            {
            OutText3+= AnsiString(" pcabc1: ")+FloatToStrF(tmp->MoverParameters->PantFrontUp,ffFixed,5,0);
            OutText3+= AnsiString(" pcabc2: ")+FloatToStrF(tmp->MoverParameters->PantRearUp,ffFixed,5,0);
            }
            if (tmp->lastcabf==-1)
            {
            OutText3+= AnsiString(" pcabc1: ")+FloatToStrF(tmp->MoverParameters->PantRearUp,ffFixed,5,0);
            OutText3+= AnsiString(" pcabc2: ")+FloatToStrF(tmp->MoverParameters->PantFrontUp,ffFixed,5,0);
            }
*/

        }
        else
        {
          //OutText ="Camera Position: "+FloatToStrF(Camera.Pos.x,ffFixed,6,2)+" "+FloatToStrF(Camera.Pos.y,ffFixed,6,2)+" "+FloatToStrF(Camera.Pos.z,ffFixed,6,2);
          OutText1="";
          OutText2="";
          OutText3="";
        }
        //OutText3= AnsiString("  Online documentation (PL, ENG, DE, soon CZ): http://www.eu07.pl");
        //OutText3="enrot="+FloatToStrF(Controlled->MoverParameters->enrot,ffFixed,6,2);
        //OutText3="; n="+FloatToStrF(Controlled->MoverParameters->n,ffFixed,6,2);



     }
    else
    if (Controlled && DebugModeFlag && !Pressed(VK_F1))
    {
      OutText1+= AnsiString(";  vel ")+FloatToStrF(Controlled->GetVelocity(),ffFixed,6,2);
      OutText1+= AnsiString(";  pos ")+FloatToStrF(Controlled->GetPosition().x,ffFixed,6,2);
      OutText1+= AnsiString(" ; ")+FloatToStrF(Controlled->GetPosition().y,ffFixed,6,2);
      OutText1+= AnsiString(" ; ")+FloatToStrF(Controlled->GetPosition().z,ffFixed,6,2);
      OutText1+= AnsiString("; dist=")+FloatToStrF(Controlled->MoverParameters->DistCounter,ffFixed,8,4);

      double a= acos( DotProduct(Normalize(Controlled->GetDirection()),vWorldFront));
//      OutText+= AnsiString(";   angle ")+FloatToStrF(a/M_PI*180,ffFixed,6,2);
      OutText1+= AnsiString("; d_omega ")+FloatToStrF(Controlled->MoverParameters->dizel_engagedeltaomega,ffFixed,6,3);
      OutText2= AnsiString("ham zesp ")+FloatToStrF(Controlled->MoverParameters->BrakeCtrlPos,ffFixed,6,0);
      OutText2+= AnsiString("; ham pom ")+FloatToStrF(Controlled->MoverParameters->LocalBrakePos,ffFixed,6,0);
      Controlled->MoverParameters->MainCtrlPos;
      if (Controlled->MoverParameters->MainCtrlPos<0)
          OutText2+= AnsiString("; nastawnik 0");
//      if (Controlled->MoverParameters->MainCtrlPos>iPozSzereg)
          OutText2+= AnsiString("; nastawnik ") + (Controlled->MoverParameters->MainCtrlPos);
//      else
//          OutText2+= AnsiString("; nastawnik S") + Controlled->MoverParameters->MainCtrlPos;

      OutText2+= AnsiString("; bocznik:")+Controlled->MoverParameters->ScndCtrlPos;
      OutText2+= AnsiString("; I=")+FloatToStrF(Controlled->MoverParameters->Im,ffFixed,6,2);
  //    OutText2+= AnsiString("; I2=")+FloatToStrF(Controlled->NextConnected->MoverParameters->Im,ffFixed,6,2);
      OutText2+= AnsiString("; V=")+FloatToStrF(Controlled->MoverParameters->RunningTraction.TractionVoltage,ffFixed,5,1);
  //    OutText2+= AnsiString("; rvent=")+FloatToStrF(Controlled->MoverParameters->RventRot,ffFixed,6,2);
      OutText2+= AnsiString("; R=")+FloatToStrF(Controlled->MoverParameters->RunningShape.R,ffFixed,4,1);
      OutText2+= AnsiString(" An=")+FloatToStrF(Controlled->MoverParameters->AccN,ffFixed,4,2);
  //    OutText2+= AnsiString("; P=")+FloatToStrF(Controlled->MoverParameters->EnginePower,ffFixed,6,1);
      OutText3+= AnsiString("cyl.ham. ")+FloatToStrF(Controlled->MoverParameters->BrakePress,ffFixed,5,2);
      OutText3+= AnsiString("; prz.gl. ")+FloatToStrF(Controlled->MoverParameters->PipePress,ffFixed,5,2);
      OutText3+= AnsiString("; zb.gl. ")+FloatToStrF(Controlled->MoverParameters->CompressedVolume,ffFixed,6,2);
//youBy - drugi wezyk
      OutText3+= AnsiString("; p.zas. ")+FloatToStrF(Controlled->MoverParameters->ScndPipePress,ffFixed,6,2);

      //ABu: testy sprzegow-> (potem przeniesc te zmienne z public do protected!)
      //OutText3+= AnsiString("; EnginePwr=")+FloatToStrF(Controlled->MoverParameters->EnginePower,ffFixed,1,5);
      //OutText3+= AnsiString("; nn=")+FloatToStrF(Controlled->NextConnectedNo,ffFixed,1,0);
      //OutText3+= AnsiString("; PR=")+FloatToStrF(Controlled->dPantAngleR,ffFixed,3,0);
      //OutText3+= AnsiString("; PF=")+FloatToStrF(Controlled->dPantAngleF,ffFixed,3,0);
      //if(Controlled->bDisplayCab==true)
      //OutText3+= AnsiString("; Wysw. kab");//+Controlled->mdKabina->GetSMRoot()->Name;
      //else
      //OutText3+= AnsiString("; test:")+AnsiString(Controlled->MoverParameters->TrainType[1]);

      //OutText3+=FloatToStrF(Train->DynamicObject->MoverParameters->EndSignalsFlag,ffFixed,3,0);;

      //OutText3+=FloatToStrF(Train->DynamicObject->MoverParameters->EndSignalsFlag&byte(((((1+Train->DynamicObject->MoverParameters->CabNo)/2)*30)+2)),ffFixed,3,0);;

//      OutText3+= AnsiString("; Ftmax=")+FloatToStrF(Controlled->MoverParameters->Ftmax,ffFixed,3,0);
//      OutText3+= AnsiString("; FTotal=")+FloatToStrF(Controlled->MoverParameters->FTotal/1000.0f,ffFixed,3,2);
//      OutText3+= AnsiString("; FTrain=")+FloatToStrF(Controlled->MoverParameters->FTrain/1000.0f,ffFixed,3,2);
      //Controlled->mdModel->GetSMRoot()->SetTranslate(vector3(0,1,0));

      //McZapkie: warto wiedziec w jakim stanie sa przelaczniki
      if (!Controlled->MoverParameters->Mains)
       {OutText3+= "  ";}
      else
       {
        switch (Controlled->MoverParameters->ActiveDir)
        {
        case 0 : {OutText3+=" -- "; break;}
        case 1 : {OutText3+=" -> "; break;}
        case -1: {OutText3+=" <- "; break;}
        }
       }
  //    OutText3+= AnsiString("; dpLocal ")+FloatToStrF(Controlled->MoverParameters->dpLocalValve,ffFixed,10,8);
  //    OutText3+= AnsiString("; dpMain ")+FloatToStrF(Controlled->MoverParameters->dpMainValve,ffFixed,10,8);
  //McZapkie: predkosc szlakowa
      if (Controlled->MoverParameters->RunningTrack.Velmax==-1)
       {OutText3+= AnsiString(" Vtrack=Vmax ");}
      else
       {OutText3+= AnsiString(" Vtrack ")+FloatToStrF(Controlled->MoverParameters->RunningTrack.Velmax,ffFixed,8,2);}
//      WriteLog(Controlled->MoverParameters->TrainType.c_str());
      if ((Controlled->MoverParameters->EnginePowerSource.SourceType==CurrentCollector) || (Controlled->MoverParameters->TrainType==dt_EZT))
       {OutText3+= AnsiString(" zb.pant. ")+FloatToStrF(Controlled->MoverParameters->PantVolume,ffFixed,8,2);}
  //McZapkie: komenda i jej parametry
       if (Controlled->MoverParameters->CommandIn.Command!=AnsiString(""))
        OutText3+=AnsiString(" C:")+AnsiString(Controlled->MoverParameters->CommandIn.Command)
        +AnsiString(" V1=")+FloatToStrF(Controlled->MoverParameters->CommandIn.Value1,ffFixed,5,0)
        +AnsiString(" V2=")+FloatToStrF(Controlled->MoverParameters->CommandIn.Value2,ffFixed,5,0);
       if (Controlled->Mechanik && Controlled->Mechanik->AIControllFlag==AIdriver)
        {
          OutText3+=AnsiString(" Vd:")+FloatToStrF(Controlled->Mechanik->VelDesired,ffFixed,4,0)
          +AnsiString(" ad=")+FloatToStrF(Controlled->Mechanik->AccDesired,ffFixed,5,2)
          +AnsiString(" Pd=")+FloatToStrF(Controlled->Mechanik->ActualProximityDist,ffFixed,4,0)
          +AnsiString(" Vn=")+FloatToStrF(Controlled->Mechanik->VelNext,ffFixed,4,0);
        }
     }

  	glDisable(GL_LIGHTING);
    if (Controlled)
     {
      SetWindowText(hWnd,AnsiString(Controlled->MoverParameters->Name).c_str());
     }
    else
     {
      SetWindowText(hWnd,Global::szSceneryFile);
     }
    glBindTexture(GL_TEXTURE_2D, 0);
    glColor4f(1.0f,0.0f,0.0f,1.0f);
    glLoadIdentity();

//ABu 150205: prosty help, zeby sie na forum nikt nie pytal, jak ma ruszyc :)

if(Global::detonatoryOK)
{
   if (Pressed(VK_F9)) ShowHints();

    glTranslatef(0.0f,0.0f,-0.50f);
    glRasterPos2f(-0.25f, 0.20f);
//    glRasterPos2f(-0.25f, 0.20f);
    if(OutText1!="")
    {
      glPrint(OutText1.c_str());
      glRasterPos2f(-0.25f, 0.19f);
      glPrint(OutText2.c_str());
      glRasterPos2f(-0.25f, 0.18f);
      glPrint(OutText3.c_str());
      //ABu: i od razu czyszczenie tego, co bylo napisane
      OutText3 = "";
      OutText2 = "";
      OutText1 = "";
    }
}

//    glRasterPos2f(-0.25f, 0.17f);
//    glPrint(OutText4.c_str());
  	glEnable(GL_LIGHTING);

    return (true);
};



//youBy - skopiowane co nieco
double __fastcall ABuAcos(vector3 calc_temp)
{
   bool calc_sin;
   double calc_angle;
   if(fabs(calc_temp.x)>fabs(calc_temp.z)) {calc_sin=false; }
                                      else {calc_sin=true;  };
   double calc_dist=sqrt((calc_temp.x*calc_temp.x)+(calc_temp.z*calc_temp.z));
   if (calc_dist!=0)
   {
        if(calc_sin)
        {
            calc_angle=asin(calc_temp.x/calc_dist);
            if(calc_temp.z>0) {calc_angle=-calc_angle;    } //ok (1)
                         else {calc_angle=M_PI+calc_angle;} // (2)
        }
        else
        {
            calc_angle=acos(calc_temp.z/calc_dist);
            if(calc_temp.x>0) {calc_angle=M_PI+M_PI-calc_angle;} // (3)
                         else {calc_angle=calc_angle;          } // (4)
        }
        return calc_angle;
    }
    return 0;
}


bool __fastcall TWorld::Render()
{

    glColor3b(255, 255, 255);
//    glColor3b(255, 0, 255);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
    glLoadIdentity();
    Camera.SetMatrix();
    glLightfv(GL_LIGHT0,GL_POSITION,Global::lightPos);

    glDisable(GL_FOG);
      Clouds.Render();
    glEnable(GL_FOG);

    if (Controlled!=NULL)
     {
      vector3 tempangle;
      double modelrotate;
      if (!FreeFlyModeFlag)
       {
        tempangle= Controlled->GetDirection()*(Controlled->MoverParameters->ActiveCab==-1 ? -1 : 1);
        modelrotate= ABuAcos(tempangle);
        while (modelrotate<-M_PI) modelrotate+=(2*M_PI);
        while (modelrotate>M_PI) modelrotate-=(2*M_PI);
       }
      else
       modelrotate=-M_PI;

      Global::SetCameraRotation(Global::pCameraRotation-modelrotate);
      while (Global::pCameraRotation<-M_PI) Global::SetCameraRotation(Global::pCameraRotation+2*M_PI);
      while (Global::pCameraRotation>M_PI) Global::SetCameraRotation(Global::pCameraRotation-2*M_PI);
     }


    if (!Ground.Render(Camera.Pos))
          return false;
    if (Global::bRenderAlpha)
    {
       if (!Ground.RenderAlpha(Camera.Pos))
          return false;
    }

//    if (Camera.Type==tp_Follow)
    if (Controlled)
        Train->Update();
//    if (Global::bRenderAlpha)
//     if (Controlled)
//        Train->RenderAlpha();
    glFlush();

    ResourceManager::Sweep(Timer::GetSimulationTime());

    return true;
};

void TWorld::ShowHints(void)
{
   glBindTexture(GL_TEXTURE_2D, 0);
   glColor4f(0.3f,1.0f,0.3f,1.0f);
   glLoadIdentity();
   glTranslatef(0.0f,0.0f,-0.50f);
   //glRasterPos2f(-0.25f, 0.20f);
   //OutText1="Uruchamianie lokomotywy - pomoc dla niezaawansowanych";
   //glPrint(OutText1.c_str());
   if(TestFlag(Controlled->MoverParameters->SecuritySystem.Status,s_ebrake))
      {
        OutText1="Gosciu, ale refleks to ty masz szachisty. Teraz zaczekaj.";
        OutText2="W tej sytuacji czuwak mozesz zbic dopiero po zatrzymaniu pociagu. ";
        if(Controlled->MoverParameters->Vel==0)
        OutText3="   (mozesz juz nacisnac spacje)";
      }
   else
   if(TestFlag(Controlled->MoverParameters->SecuritySystem.Status,s_alarm))
      {
        OutText1="Natychmiast zbij czuwak, bo pociag sie zatrzyma!";
        OutText2="   (szybko nacisnij spacje!)";
      }
   else
   if(TestFlag(Controlled->MoverParameters->SecuritySystem.Status,s_aware))
      {
        OutText1="Zbij czuwak, zeby udowodnic, ze nie spisz :) ";
        OutText2="   (nacisnij spacje)";
      }
   else
   if(Controlled->MoverParameters->FuseFlag)
      {
        OutText1="Czlowieku, delikatniej troche! Gdzie sie spieszysz?";
        OutText2="Wybilo Ci bezpiecznik nadmiarowy, teraz musisz wlaczyc go ponownie.";
        OutText3="   ('N', wczesniej nastawnik i boczniki na zero -> '-' oraz '*' do oporu)";

      }
   else
   if (Controlled->MoverParameters->V==0)
   {
      if (!(Controlled->MoverParameters->PantFrontVolt||Controlled->MoverParameters->PantRearVolt))
         {
         OutText1="Jezdziles juz kiedys lokomotywa? Pierwszy raz? Dobra, to zaczynamy.";
         OutText2="No to co, trzebaby chyba podniesc pantograf?";
         OutText3="   (wcisnij 'shift+P' - przedni, 'shift+O' - tylny)";
         }
      else
      if (!Controlled->MoverParameters->Mains)
         {
         OutText1="Dobra, mozemy uruchomic glowny obwod lokomotywy.";
         OutText2="   (wcisnij 'shift+M')";
         }
      else
      if (!Controlled->MoverParameters->ConverterAllow)
         {
         OutText1="Teraz wlacz przetwornice.";
         OutText2="   (wcisnij 'shift+X')";
         }
      else
      if (!Controlled->MoverParameters->CompressorAllow)
         {
         OutText1="Teraz wlacz sprezarke.";
         OutText2="   (wcisnij 'shift+C')";
         }
      else
      if (Controlled->MoverParameters->ActiveDir==0)
         {
         OutText1="Ustaw nawrotnik na kierunek, w ktorym chcesz jechac.";
         OutText2="   ('d' - do przodu, 'r' - do tylu)";
         }
      else
      if (Controlled->GetLastDynamic(1)->MoverParameters->BrakePress>0)
         {
         OutText1="Odhamuj sklad i zaczekaj az Ci powiem - to moze troche potrwac.";
         OutText2="   ('.' na klawiaturze numerycznej)";
         }
      else
      if (Controlled->MoverParameters->BrakeCtrlPos!=0)
         {
         OutText1="Przelacz kran hamulca w pozycje 'jazda'.";
         OutText2="   ('4' na klawiaturze numerycznej)";
         }
      else
      if (Controlled->MoverParameters->MainCtrlPos==0)
         {
         OutText1="Teraz juz mozesz ruszyc ustawiajac pierwsza pozycje na nastawniku.";
         OutText2="   (jeden raz '+' na klawiaturze numerycznej)";
         }
      else
      if((Controlled->MoverParameters->MainCtrlPos>0)&&(Controlled->MoverParameters->ShowCurrent(1)!=0))
         {
         OutText1="Dobrze, mozesz teraz wlaczac kolejne pozycje nastawnika.";
         OutText2="   ('+' na klawiaturze numerycznej, tylko z wyczuciem)";
         }
      if((Controlled->MoverParameters->MainCtrlPos>1)&&(Controlled->MoverParameters->ShowCurrent(1)==0))
         {
         OutText1="Spieszysz sie gdzies? Zejdz nastawnikiem na zero i probuj jeszcze raz!";
         OutText2="   (teraz do oporu '-' na klawiaturze numerycznej)";
         }
   }
   else
   {
      OutText1="Aby przyspieszyc mozesz wrzucac kolejne pozycje nastawnika.";
      if(Controlled->MoverParameters->MainCtrlPos==28)
      {OutText1="Przy tym ustawienu mozesz bocznikowac silniki - sprobuj: '/' i '*' ";}
      if(Controlled->MoverParameters->MainCtrlPos==43)
      {OutText1="Przy tym ustawienu mozesz bocznikowac silniki - sprobuj: '/' i '*' ";}

      OutText2="Aby zahamowac zejdz nastawnikiem do 0 ('-' do oporu) i ustaw kran hamulca";
      OutText3="w zaleznosci od sily hamowania, jakiej potrzebujesz ('2', '5' lub '8' na kl. num.)";

      //else
      //if() OutText1="teraz mozesz ruszyc naciskajac jeden raz '+' na klawiaturze numerycznej";
      //else
      //if() OutText1="teraz mozesz ruszyc naciskajac jeden raz '+' na klawiaturze numerycznej";
   }
   //OutText3=FloatToStrF(Controlled->MoverParameters->SecuritySystem.Status,ffFixed,3,0);

   if(OutText1!="")
   {
      glRasterPos2f(-0.25f, 0.19f);
      glPrint(OutText1.c_str());
      OutText1="";
   }
   if(OutText2!="")
   {
      glRasterPos2f(-0.25f, 0.18f);
      glPrint(OutText2.c_str());
      OutText2="";
   }
   if(OutText3!="")
   {
      glRasterPos2f(-0.25f, 0.17f);
      glPrint(OutText3.c_str());
      OutText3="";
   }
};





//---------------------------------------------------------------------------
#pragma package(smart_init)







