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
#include "Event.h"
#include "Train.h"
#include "Driver.h"

#define TEXTURE_FILTER_CONTROL_EXT      0x8500
#define TEXTURE_LOD_BIAS_EXT            0x8501
//---------------------------------------------------------------------------
#pragma package(smart_init)

typedef void (APIENTRY *FglutBitmapCharacter)(void *font,int character); //typ funkcji
FglutBitmapCharacter glutBitmapCharacterDLL=NULL; //deklaracja zmiennej
HINSTANCE hinstGLUT32=NULL; //wskaŸnik do GLUT32.DLL
//GLUTAPI void APIENTRY glutBitmapCharacterDLL(void *font, int character);


using namespace Timer;

const double fMaxDt= 0.01;

__fastcall TWorld::TWorld()
{
 //randomize();
 //Randomize();
 Train=NULL;
 Aspect=1;
 for (int i=0; i<10; i++)
  KeyEvents[i]=NULL; //eventy wyzwalane klawiszami cyfrowymi
 Global::iSlowMotion=0;
 Global::changeDynObj=false;
 lastmm=61; //ABu: =61, zeby zawsze inicjowac kolor czarnej mgly przy warunku (GlobalTime->mm!=lastmm) :)
 OutText1=""; //teksty wyœwietlane na ekranie
 OutText2="";
 OutText3="";
 iCheckFPS=0; //kiedy znów sprawdziæ FPS, ¿eby wy³¹czaæ optymalizacji od razu do zera
 pDynamicNearest=NULL;
}

__fastcall TWorld::~TWorld()
{
 Global::bManageNodes=false; //Ra: wy³¹czenie wyrejestrowania, bo siê sypie
 SafeDelete(Train);
 TSoundsManager::Free();
 TModelsManager::Free();
 TTexturesManager::Free();
 glDeleteLists(base,96);
 if (hinstGLUT32)
  FreeLibrary(hinstGLUT32);
}

GLvoid __fastcall TWorld::glPrint(const char *txt) //custom GL "Print" routine
{//wypisywanie tekstu 2D na ekranie
 if (!txt) return;
 if (Global::bGlutFont)
 {//tekst generowany przez GLUT
  int i,len=strlen(txt);
  for (i=0;i<len;i++)
   (glutBitmapCharacterDLL)(GLUT_BITMAP_8_BY_13,txt[i]); //funkcja linkowana dynamicznie
 }
 else
 {//generowanie przez Display Lists
  glPushAttrib(GL_LIST_BIT); //pushes the display list bits
  glListBase(base-32); //sets the base character to 32
  glCallLists(strlen(txt),GL_UNSIGNED_BYTE,txt); //draws the display list text
  glPopAttrib(); //pops the display list bits
 }
}

TDynamicObject *Controlled=NULL; //pojazd, który prowadzimy

bool __fastcall TWorld::Init(HWND NhWnd, HDC hDC)
{
 double time=(double)Now();
 Global::hWnd=NhWnd; //do WM_COPYDATA
 Global::detonatoryOK=true;
 WriteLog("--- MaSzyna ---"); //pierwsza linia jest gubiona
 WriteLog("Starting MaSzyna rail vehicle simulator.");
 WriteLog(Global::asVersion);
#if sizeof(TSubModel)!=256
 Error("Wrong sizeof(TSubModel) is "+AnsiString(sizeof(TSubModel)));
 return false;
#endif
 WriteLog("Online documentation and additional files on http://eu07.pl");
 WriteLog("Authors: Marcin_EU, McZapkie, ABu, Winger, Tolaris, nbmx_EU, OLO_EU, Bart, Quark-t, ShaXbee, Oli_EU, youBy, Ra and others");
 WriteLog("Renderer:");
 WriteLog( (char*) glGetString(GL_RENDERER));
 WriteLog("Vendor:");
//Winger030405: sprawdzanie sterownikow
 WriteLog( (char*) glGetString(GL_VENDOR));
 AnsiString glver=((char*)glGetString(GL_VERSION));
 WriteLog("OpenGL Version:");
 WriteLog(glver);
 if ((glver=="1.5.1") || (glver=="1.5.2"))
 {
  Error("Niekompatybilna wersja openGL - dwuwymiarowy tekst nie bedzie wyswietlany!");
  WriteLog("WARNING! This OpenGL version is not fully compatible with simulator!");
  WriteLog("UWAGA! Ta wersja OpenGL nie jest w pelni kompatybilna z symulatorem!");
  Global::detonatoryOK=false;
 }
 else
  Global::detonatoryOK=true;
 //Ra: umieszczone w EU07.cpp jakoœ nie chce dzia³aæ
 while (glver.LastDelimiter(".")>glver.Pos("."))
  glver=glver.SubString(1,glver.LastDelimiter(".")-1); //obciêcie od drugiej kropki
 try {Global::fOpenGL=glver.ToDouble();} catch (...) {Global::fOpenGL=0.0;}
 Global::bOpenGL_1_5=(Global::fOpenGL>=1.5);

 WriteLog("Supported extensions:");
 WriteLog((char*)glGetString(GL_EXTENSIONS));
 if (glewGetExtension("GL_ARB_vertex_buffer_object")) //czy jest VBO w karcie graficznej
 {
#ifdef USE_VBO
  if (AnsiString((char*)glGetString(GL_VENDOR)).Pos("Intel")) //wymuszenie tylko dla kart Intel
   Global::bUseVBO=true; //VBO w³¹czane tylko, jeœli jest obs³uga
#endif
  if (Global::bUseVBO)
   WriteLog("Ra: The VBO is found and will be used.");
  else
   WriteLog("Ra: The VBO is found, but Display Lists are selected.");
 }
 else
 {WriteLog("Ra: No VBO found - Display Lists used. Upgrade drivers or buy a newer graphics card!");
  Global::bUseVBO=false; //mo¿e byæ w³¹czone parametrem w INI
 }
 if (Global::iMultisampling)
  WriteLog("Used multisampling of "+AnsiString(Global::iMultisampling)+" samples.");
 {//ograniczenie maksymalnego rozmiaru tekstur - parametr dla skalowania tekstur
  GLint i;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE,&i);
  if (i<Global::iMaxTextureSize) Global::iMaxTextureSize=i;
  WriteLog("Max texture size: "+AnsiString(Global::iMaxTextureSize));
 }
/*-----------------------Render Initialization----------------------*/
 if (Global::fOpenGL>=1.2) //poni¿sze nie dzia³a w 1.1
  glTexEnvf(TEXTURE_FILTER_CONTROL_EXT,TEXTURE_LOD_BIAS_EXT,-1);
 GLfloat FogColor[]={1.0f,1.0f,1.0f,1.0f};
 glMatrixMode(GL_MODELVIEW); // Select The Modelview Matrix
 glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear screen and depth buffer
 glLoadIdentity();
 //WriteLog("glClearColor (FogColor[0], FogColor[1], FogColor[2], 0.0); ");
 //glClearColor (1.0, 0.0, 0.0, 0.0);                  // Background Color
 //glClearColor (FogColor[0], FogColor[1], FogColor[2], 0.0);                  // Background Color
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
    //if (Global::bRenderAlpha) //Ra: wywalam tê flagê
    {
      WriteLog("glEnable(GL_BLEND);");
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
/*
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
*/
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

    Global::lightPos[0]=lp.x;
    Global::lightPos[1]=lp.y;
    Global::lightPos[2]=lp.z;
    Global::lightPos[3]=0.0f;

    //Ra: œwiat³a by sensowniej by³o ustawiaæ po wczytaniu scenerii

    //Ra: szcz¹tkowe œwiat³o rozproszone - ¿eby by³o cokolwiek widaæ w ciemnoœci
    WriteLog("glLightModelfv(GL_LIGHT_MODEL_AMBIENT,darkLight);");
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT,Global::darkLight);

    //Ra: œwiat³o 0 - g³ówne œwiat³o zewnêtrzne (S³oñce, Ksiê¿yc)
    WriteLog("glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);");
    glLightfv(GL_LIGHT0,GL_AMBIENT,Global::ambientDayLight);
    WriteLog("glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);");
    glLightfv(GL_LIGHT0,GL_DIFFUSE,Global::diffuseDayLight);
    WriteLog("glLightfv(GL_LIGHT0,GL_SPECULAR,specularLight);");
    glLightfv(GL_LIGHT0,GL_SPECULAR,Global::specularDayLight);
    WriteLog("glLightfv(GL_LIGHT0,GL_POSITION,lightPos);");
    glLightfv(GL_LIGHT0,GL_POSITION,Global::lightPos);
    WriteLog("glEnable(GL_LIGHT0);");
    glEnable(GL_LIGHT0);


    //glColor() ma zmieniaæ kolor wybrany w glColorMaterial()
    WriteLog("glEnable(GL_COLOR_MATERIAL);");
    glEnable(GL_COLOR_MATERIAL);

    WriteLog("glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);");
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

//    WriteLog("glMaterialfv( GL_FRONT, GL_AMBIENT, whiteLight );");
//	glMaterialfv( GL_FRONT, GL_AMBIENT, Global::whiteLight );

    WriteLog("glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, whiteLight );");
	glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, Global::whiteLight );

/*
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

    //Ra: ustawienia testowe
    glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT,GL_NICEST);

/*--------------------Render Initialization End---------------------*/

 WriteLog("Font init"); //pocz¹tek inicjacji fontów 2D
 if (Global::bGlutFont) //jeœli wybrano GLUT font, próbujemy zlinkowaæ GLUT32.DLL
 {
  UINT mode=SetErrorMode(SEM_NOOPENFILEERRORBOX); //aby nie wrzeszcza³, ¿e znaleŸæ nie mo¿e
  hinstGLUT32=LoadLibrary(TEXT("GLUT32.DLL")); //get a handle to the DLL module
  SetErrorMode(mode);
  // If the handle is valid, try to get the function address.
  if (hinstGLUT32)
   glutBitmapCharacterDLL=(FglutBitmapCharacter)GetProcAddress(hinstGLUT32,"glutBitmapCharacter");
  else
   WriteLog("Missed GLUT32.DLL.");
  if (glutBitmapCharacterDLL)
   WriteLog("Used font from GLUT32.DLL.");
  else
   Global::bGlutFont=false; //nie uda³o siê, trzeba spróbowaæ na Display List
 }
 if (!Global::bGlutFont)
 {//jeœli bezGLUTowy font
  HFONT font;										// Windows Font ID
  base=glGenLists(96); //storage for 96 characters
  font=CreateFont(       -15, //height of font
                           0, //width of font
    		           0, //angle of escapement
		           0, //orientation angle
	             FW_BOLD, //font weight
	               FALSE, //italic
	               FALSE, //underline
     	               FALSE, //strikeout
                ANSI_CHARSET, //character set identifier
               OUT_TT_PRECIS, //output precision
         CLIP_DEFAULT_PRECIS, //clipping precision
         ANTIALIASED_QUALITY, //output quality
   FF_DONTCARE|DEFAULT_PITCH, //family and pitch
              "Courier New"); //font name
  SelectObject(hDC,font); //selects the font we want
  wglUseFontBitmapsA(hDC,32,96,base); //builds 96 characters starting at character 32
  WriteLog("Display Lists font used."); //+AnsiString(glGetError())
 }
 WriteLog("Font init OK"); //+AnsiString(glGetError())

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
        logo=TTexturesManager::GetTextureID("logo",6);
        glBindTexture(GL_TEXTURE_2D,logo);       // Select our texture

	glBegin(GL_QUADS);		        // Drawing using triangles
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.28f, -0.22f,  0.0f);	// Bottom left of the texture and quad
		glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.28f, -0.22f,  0.0f);	// Bottom right of the texture and quad
		glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.28f,  0.22f,  0.0f);	// Top right of the texture and quad
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.28f,  0.22f,  0.0f);	// Top left of the texture and quad
	glEnd();
        //~logo; Ra: to jest bez sensu zapis
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
    TSoundsManager::Init(hWnd);
    //TSoundsManager::LoadSounds( "" );
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
    if (Global::detonatoryOK)
    {
    glRasterPos2f(-0.25f, -0.15f);
    glPrint("OK.");
    }
    SwapBuffers(hDC);					// Swap Buffers (Double Buffering)

    WriteLog("Ground init");
    if (Global::detonatoryOK)
    {
    glRasterPos2f(-0.25f, -0.16f);
    glPrint("Sceneria / Scenery (can take a while)...");
    }
    SwapBuffers(hDC);					// Swap Buffers (Double Buffering)

    Ground.Init(Global::szSceneryFile);
//    Global::tSinceStart= 0;
    Clouds.Init();
    WriteLog("Ground init OK");
    if (Global::detonatoryOK)
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
      Camera.Init(Global::pFreeCameraInit[0],Global::pFreeCameraInitAngle[0]);

    char buff[255]= "Player train init: ";
    if (Global::detonatoryOK)
    {
    glRasterPos2f(-0.25f, -0.18f);
    glPrint("Inicjalizacja wybranego pojazdu do sterowania...");
    }
    SwapBuffers(hDC);					// Swap Buffers (Double Buffering)

    strcat(buff,Global::asHumanCtrlVehicle.c_str());
    WriteLog(buff);
    TGroundNode *PlayerTrain=Ground.FindDynamic(Global::asHumanCtrlVehicle);
    if (PlayerTrain)
    {
//   if (PlayerTrain->DynamicObject->MoverParameters->TypeName==AnsiString("machajka"))
//     Train= new TMachajka();
//   else
//   if (PlayerTrain->DynamicObject->MoverParameters->TypeName==AnsiString("303e"))
//     Train= new TEu07();
//   else
     Train=new TTrain();
     if (Train->Init(PlayerTrain->DynamicObject))
     {
      Controlled=Train->DynamicObject;
      WriteLog("Player train init OK");
      if (Global::detonatoryOK)
      {
       glRasterPos2f(-0.25f, -0.19f);
       glPrint("OK.");
      }
      SwapBuffers(hDC);					// Swap Buffers (Double Buffering)
     }
     else
     {
      Error("Player train init failed!");
      FreeFlyModeFlag=true; //Ra: automatycznie w³¹czone latanie
      if (Global::detonatoryOK)
      {
       glRasterPos2f(-0.25f, -0.20f);
       glPrint("Blad inicjalizacji sterowanego pojazdu!");
      }
      SwapBuffers(hDC);					// Swap Buffers (Double Buffering)
      Controlled=NULL;
      Camera.Type=tp_Free;
     }
    }
    else
    {
     if (Global::asHumanCtrlVehicle!=AnsiString("ghostview"))
      Error("Player train not exist!");
     FreeFlyModeFlag=true; //Ra: automatycznie w³¹czone latanie
     if (Global::detonatoryOK)
     {
      glRasterPos2f(-0.25f, -0.20f);
      glPrint("Wybrany pojazd nie istnieje w scenerii!");
     }
     SwapBuffers(hDC);					// Swap Buffers (Double Buffering)
     Controlled=NULL;
     Camera.Type=tp_Free;
    }
    glEnable(GL_DEPTH_TEST);
    //Ground.pTrain=Train;
    //if (!Global::bMultiplayer) //na razie w³¹czone
    {//eventy aktywowane z klawiatury tylko dla jednego u¿ytkownika
     KeyEvents[0]=Ground.FindEvent("keyctrl00");
     KeyEvents[1]=Ground.FindEvent("keyctrl01");
     KeyEvents[2]=Ground.FindEvent("keyctrl02");
     KeyEvents[3]=Ground.FindEvent("keyctrl03");
     KeyEvents[4]=Ground.FindEvent("keyctrl04");
     KeyEvents[5]=Ground.FindEvent("keyctrl05");
     KeyEvents[6]=Ground.FindEvent("keyctrl06");
     KeyEvents[7]=Ground.FindEvent("keyctrl07");
     KeyEvents[8]=Ground.FindEvent("keyctrl08");
     KeyEvents[9]=Ground.FindEvent("keyctrl09");
    }
    //matrix4x4 ident2;
    //ident2.Identity();

// glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);  //{Texture blends with object background}
 light=TTexturesManager::GetTextureID("smuga.tga");
// Camera.Reset();
 ResetTimers();
 WriteLog("Load time: "+AnsiString(0.1*floor(864000.0*((double)Now()-time)))+" seconds");
 return true;
};


void __fastcall TWorld::OnKeyPress(int cKey)
{//(cKey) to kod klawisza, cyfrowe i literowe siê zgadzaj¹
 if (!Global::bPause)
 {//podczas pauzy klawisze nie dzia³aj¹
  AnsiString info="Key pressed: [";
  if (Pressed(VK_SHIFT)) info+="Shift]+[";
  if (Pressed(VK_CONTROL)) info+="Ctrl]+[";
  if (cKey>123) //coœ tam jeszcze jest?
   WriteLog(info+AnsiString((char)(cKey-128))+"]");
  else if (cKey>=112) //funkcyjne
   WriteLog(info+"F"+AnsiString(cKey-111)+"]");
  else if (cKey>=96)
   WriteLog(info+"Num"+AnsiString("0123456789*+?-./").SubString(cKey-95,1)+"]");
  else if (((cKey>='0')&&(cKey<='9'))||((cKey>='A')&&(cKey<='Z'))||(cKey==' '))
   WriteLog(info+AnsiString((char)(cKey))+"]");
  else if (cKey=='-')
   WriteLog(info+"Insert]");
  else if (cKey=='.')
   WriteLog(info+"Delete]");
  else if (cKey=='$')
   WriteLog(info+"Home]");
  else if (cKey=='#')
   WriteLog(info+"End]");
  else if (cKey>'Z') //¿eby nie logowaæ kursorów
   WriteLog(info+AnsiString(cKey)+"]"); //numer klawisza
 }
 if ((cKey<='9')?(cKey>='0'):false) //klawisze cyfrowe
 {int i=cKey-'0'; //numer klawisza
  if (Pressed(VK_SHIFT))
  {//z [Shift] uruchomienie eventu
   if (!Global::bPause) //podczas pauzy klawisze nie dzia³aj¹
    if (KeyEvents[i])
     Ground.AddToQuery(KeyEvents[i],NULL);
  }
   else //zapamiêtywanie kamery mo¿e dzia³aæ podczas pauzy
    if (FreeFlyModeFlag) //w trybie latania mo¿na przeskakiwaæ do ustawionych kamer
    {if ((!Global::pFreeCameraInit[i].x&&!Global::pFreeCameraInit[i].y&&!Global::pFreeCameraInit[i].z))
    {//jeœli kamera jest w punkcie zerowym, zapamiêtanie wspó³rzêdnych i k¹tów
     Global::pFreeCameraInit[i]=Camera.Pos;
     Global::pFreeCameraInitAngle[i].x=Camera.Pitch;
     Global::pFreeCameraInitAngle[i].y=Camera.Yaw;
     Global::pFreeCameraInitAngle[i].z=Camera.Roll;
     //logowanie, ¿eby mo¿na by³o do scenerii przepisaæ
     WriteLog("camera "
      +AnsiString(Global::pFreeCameraInit[i].x)+" "
      +AnsiString(Global::pFreeCameraInit[i].y)+" "
      +AnsiString(Global::pFreeCameraInit[i].z)+" "
      +AnsiString(RadToDeg(Global::pFreeCameraInitAngle[i].x))+" "
      +AnsiString(RadToDeg(Global::pFreeCameraInitAngle[i].y))+" "
      +AnsiString(RadToDeg(Global::pFreeCameraInitAngle[i].z))+" "
      +AnsiString(i)+" endcamera");
    }
    else //równie¿ przeskaliwanie
     Camera.Init(Global::pFreeCameraInit[i],Global::pFreeCameraInitAngle[i]);
   }
  //bêdzie jeszcze za³¹czanie sprzêgów z [Ctrl]
 }
 else if ((cKey>=VK_F1)?(cKey<=VK_F12):false)
 {if (Global::iTextMode==cKey)
   Global::iTextMode=0; //wy³¹czenie napisów
  else
   switch (cKey)
   {case VK_F1: //czas i relacja
    case VK_F2: //parametry pojazdu
    case VK_F3:
    case VK_F5: //przesiadka do innego pojazdu
    case VK_F8: //FPS
    case VK_F9: //wersja, typ wyœwietlania, b³êdy OpenGL
    case VK_F10:
    case VK_F12: //coœ tam jeszcze
     Global::iTextMode=cKey;
   }
  if (cKey!=VK_F4) return; //nie s¹ przekazywane do pojazdu
 }
 if (Global::iTextMode==VK_F10) //wyœwietlone napisy klawiszem F10
 {//i potwierdzenie
  Global::iTextMode=(cKey=='Y')?-1:0; //flaga wyjœcia z programu
  return; //nie przekazujemy do poci¹gu
 }
 else if (cKey==3) //[Ctrl]+[Break]
 {//hamowanie wszystkich pojazdów w okolicy
  Ground.RadioStop(Camera.Pos);
 }
 else if (!Global::bPause||(cKey==VK_F4)) //podczas pauzy sterownaie nie dzia³a, F4 tak
  if (Train)
   if (Controlled)
    if ((Controlled->Controller==Humandriver)?true:DebugModeFlag||(cKey=='Q')||(cKey==VK_F4))
     Train->OnKeyPress(cKey); //przekazanie klawisza do pojazdu
 //switch (cKey)
 //{case 'a': //ignorowanie repetycji
 // case 'A': Global::iKeyLast=cKey; break;
 // default: Global::iKeyLast=0;
 //}
}


void __fastcall TWorld::OnMouseMove(double x, double y)
{//McZapkie:060503-definicja obracania myszy
 Camera.OnCursorMove(x*Global::fMouseXScale,-y*Global::fMouseYScale);
}

AnsiString __fastcall Bezogonkow(AnsiString str)
{//wyciêcie liter z ogonkami, bo OpenGL nie umie wyœwietliæ
 for (int i=1;i<=str.Length();++i)
  switch (str[i])
  {//bo komunikaty po polsku s¹...
   case '¹': str[i]='a'; break;
   case 'æ': str[i]='c'; break;
   case 'ê': str[i]='e'; break;
   case '³': str[i]='l'; break;
   case 'ñ': str[i]='n'; break;
   case 'ó': str[i]='o'; break;
   case 'œ': str[i]='s'; break;
   case '¿': str[i]='z'; break;
   case 'Ÿ': str[i]='z'; break;
   case '¥': str[i]='A'; break;
   case 'Æ': str[i]='C'; break;
   case 'Ê': str[i]='E'; break;
   case '£': str[i]='L'; break;
   case 'Ñ': str[i]='N'; break;
   case 'Ó': str[i]='O'; break;
   case 'Œ': str[i]='S'; break;
   case '¯': str[i]='Z'; break;
   case '': str[i]='Z'; break;
   default:
    if (str[i]&128) str[i]='?';
    else if (str[i]<' ') str[i]=' ';
  }
 return str;
}

bool __fastcall TWorld::Update()
{
#ifdef USE_SCENERY_MOVING
 vector3 tmpvector = Global::GetCameraPosition();
 tmpvector = vector3(
  -int(tmpvector.x)+int(tmpvector.x)%10000,
  -int(tmpvector.y)+int(tmpvector.y)%10000,
  -int(tmpvector.z)+int(tmpvector.z)%10000);
 if(tmpvector.x || tmpvector.y || tmpvector.z)
 {
  WriteLog("Moving scenery");
  Ground.MoveGroundNode(tmpvector);
  WriteLog("Scenery moved");
 };
#endif
 if (iCheckFPS)
  --iCheckFPS;
 else
 {//jak dosz³o do zera, to sprawdzamy wydajnoœæ
  if ((GetFPS()<16)&&(Global::iSlowMotion<7))
  {Global::iSlowMotion=(Global::iSlowMotion<<1)+1; //zapalenie kolejnego bitu
   if (Global::iSlowMotionMask&1)
    if (Global::iMultisampling) //a multisampling jest w³¹czony
     glDisable(GL_MULTISAMPLE); //wy³¹czenie multisamplingu powinno poprawiæ FPS
  }
  else if ((GetFPS()>25)&&Global::iSlowMotion)
  {//FPS siê zwiêkszy³, mo¿na w³¹czyæ bajery
   Global::iSlowMotion=(Global::iSlowMotion>>1); //zgaszenie bitu
   if (Global::iSlowMotion==0) //jeœli jest pe³na prêdkoœæ
    if (Global::iMultisampling) //a multisampling jest w³¹czony
     glEnable(GL_MULTISAMPLE);
  }
  iCheckFPS=0.5*GetFPS(); //tak ze 2 sekundy poczekaæ - zacina
 }
 UpdateTimers(Global::bPause);
 if (!Global::bPause)
 {//jak pauza, to nie ma po co tego przeliczaæ
  GlobalTime->UpdateMTableTime(GetDeltaTime()); //McZapkie-300302: czas rozkladowy
  //Ra: przeliczenie k¹ta czasu (do animacji zale¿nych od czasu)
  Global::fTimeAngleDeg=GlobalTime->hh*15.0+GlobalTime->mm*0.25+GlobalTime->mr/240.0;
  if (Global::fMoveLight>=0.0)
  {//testowo ruch œwiat³a
   //double a=Global::fTimeAngleDeg/180.0*M_PI-M_PI; //k¹t godzinny w radianach
   double a=fmod(Global::fSunSpeed*Global::fTimeAngleDeg,360.0)/180.0*M_PI-M_PI; //k¹t godzinny w radianach
   double L=Global::fLatitudeDeg/180.0*M_PI; //szerokoœæ geograficzna
   double H=asin(cos(L)*cos(Global::fSunDeclination)*cos(a)+sin(L)*sin(Global::fSunDeclination));
   //double A=asin(cos(d)*sin(M_PI-a)/cos(H));
   //Declination=((0.322003-22.971*cos(t)-0.357898*cos(2*t)-0.14398*cos(3*t)+3.94638*sin(t)+0.019334*sin(2*t)+0.05928*sin(3*t)))*Pi/180
   //Altitude=asin(sin(Declination)*sin(latitude)+cos(Declination)*cos(latitude)*cos((15*(time-12))*(Pi/180)));
   //Azimuth=(acos((cos(latitude)*sin(Declination)-cos(Declination)*sin(latitude)*cos((15*(time-12))*(Pi/180)))/cos(Altitude)));
   //double A=acos(cos(L)*sin(d)-cos(d)*sin(L)*cos(M_PI-a)/cos(H));
   //dAzimuth = atan2(-sin( dHourAngle ),tan( dDeclination )*dCos_Latitude - dSin_Latitude*dCos_HourAngle );
   double A=atan2(sin(a),tan(Global::fSunDeclination)*cos(L)-sin(L)*cos(a));
   vector3 lp=vector3(sin(A),tan(H),cos(A));
   lp=Normalize(lp); //przeliczenie na wektor d³ugoœci 1.0
   Global::lightPos[0]=(float)lp.x;
   Global::lightPos[1]=(float)lp.y;
   Global::lightPos[2]=(float)lp.z;
   glLightfv(GL_LIGHT0,GL_POSITION,Global::lightPos);        //daylight position
   if (H>0)
   {//s³oñce ponad horyzontem
    Global::ambientDayLight[0]=Global::ambientLight[0];
    Global::ambientDayLight[1]=Global::ambientLight[1];
    Global::ambientDayLight[2]=Global::ambientLight[2];
    if (H>0.02)
    {Global::diffuseDayLight[0]=Global::diffuseLight[0]; //od wschodu do zachodu maksimum ???
     Global::diffuseDayLight[1]=Global::diffuseLight[1];
     Global::diffuseDayLight[2]=Global::diffuseLight[2];
     Global::specularDayLight[0]=Global::specularLight[0]; //podobnie specular
     Global::specularDayLight[1]=Global::specularLight[1];
     Global::specularDayLight[2]=Global::specularLight[2];
    }
    else
    {Global::diffuseDayLight[0]=50*H*Global::diffuseLight[0]; //wschód albo zachód
     Global::diffuseDayLight[1]=50*H*Global::diffuseLight[1];
     Global::diffuseDayLight[2]=50*H*Global::diffuseLight[2];
     Global::specularDayLight[0]=50*H*Global::specularLight[0]; //podobnie specular
     Global::specularDayLight[1]=50*H*Global::specularLight[1];
     Global::specularDayLight[2]=50*H*Global::specularLight[2];
    }
   }
   else
   {//s³oñce pod horyzontem
    GLfloat lum=2.0*(H>-0.314159?0.314159+H:0.0); //po zachodzie ambient siê œciemnia
    Global::ambientDayLight[0]=lum;
    Global::ambientDayLight[1]=lum;
    Global::ambientDayLight[2]=lum;
    Global::diffuseDayLight[0]=Global::noLight[0]; //od zachodu do wschodu nie ma diffuse
    Global::diffuseDayLight[1]=Global::noLight[1];
    Global::diffuseDayLight[2]=Global::noLight[2];
    Global::specularDayLight[0]=Global::noLight[0]; //ani specular
    Global::specularDayLight[1]=Global::noLight[1];
    Global::specularDayLight[2]=Global::noLight[2];
   }
   // Calculate sky colour according to time of day.
   //GLfloat sin_t = sin(PI * time_of_day / 12.0);
   //back_red = 0.3 * (1.0 - sin_t);
   //back_green = 0.9 * sin_t;
   //back_blue = sin_t + 0.4, 1.0;
   //aktualizacja œwiate³
   glLightfv(GL_LIGHT0,GL_AMBIENT,Global::ambientDayLight);
   glLightfv(GL_LIGHT0,GL_DIFFUSE,Global::diffuseDayLight);
   glLightfv(GL_LIGHT0,GL_SPECULAR,Global::specularDayLight);
  }
  Global::fLuminance= //to pos³u¿y równie¿ do zapalania latarñ
   +0.150*(Global::diffuseDayLight[0]+Global::ambientDayLight[0])  //R
   +0.295*(Global::diffuseDayLight[1]+Global::ambientDayLight[1])  //G
   +0.055*(Global::diffuseDayLight[2]+Global::ambientDayLight[2]); //B
  if (Global::fMoveLight>=0.0)
  {//przeliczenie koloru nieba
   vector3 sky=vector3(Global::AtmoColor[0],Global::AtmoColor[1],Global::AtmoColor[2]);
   if (Global::fLuminance<0.25)
   {//przyspieszenie zachodu/wschodu
    sky*=4.0*Global::fLuminance; //nocny kolor nieba
    GLfloat fog[3];
    fog[0]=Global::FogColor[0]*4.0*Global::fLuminance;
    fog[1]=Global::FogColor[1]*4.0*Global::fLuminance;
    fog[2]=Global::FogColor[2]*4.0*Global::fLuminance;
    glFogfv(GL_FOG_COLOR,fog); //nocny kolor mg³y
   }
   else
    glFogfv(GL_FOG_COLOR,Global::FogColor); //kolor mg³y
   glClearColor(sky.x,sky.y,sky.z,0.0); //kolor nieba
  }
 } //koniec dzia³añ niewykonywanych podczas pauzy
 if (Global::bActive)
 {//obs³uga ruchu kamery tylko gdy okno jest aktywne
  if (Pressed(VK_LBUTTON))
  {
   Camera.Reset(); //likwidacja obrotów - patrzy horyzontalnie na po³udnie
   //if (!FreeFlyModeFlag) //jeœli wewn¹trz - patrzymy do ty³u
   // Camera.LookAt=Train->pMechPosition-Normalize(Train->GetDirection())*10;
   if (Controlled?LengthSquared3(Controlled->GetPosition()-Camera.Pos)<2250000:false) //gdy bli¿ej ni¿ 1.5km
    Camera.LookAt=Controlled->GetPosition();
   else
   {TDynamicObject *d=Ground.DynamicNearest(Camera.Pos,300); //szukaj w promieniu 300m
    if (!d)
     d=Ground.DynamicNearest(Camera.Pos,1000); //dalej szukanie, jesli bli¿ej nie ma
    if (d&&pDynamicNearest) //jeœli jakiœ jest znaleziony wczeœniej
     if (100.0*LengthSquared3(d->GetPosition()-Camera.Pos)>LengthSquared3(pDynamicNearest->GetPosition()-Camera.Pos))
      d=pDynamicNearest; //jeœli najbli¿szy nie jest 10 razy bli¿ej ni¿ poprzedni najbli¿szy, zostaje poprzedni
    if (d) pDynamicNearest=d; //zmiana nanowy, jeœli coœ znaleziony niepusty
    if (pDynamicNearest) Camera.LookAt=pDynamicNearest->GetPosition();
   }
   if (FreeFlyModeFlag)
    Camera.RaLook(); //jednorazowe przestawienie kamery
  }
  else if (Pressed(VK_RBUTTON)||Pressed(VK_F4))
  {//ABu 180404 powrot mechanika na siedzenie albo w okolicê pojazdu
   //if (Pressed(VK_F4)) Global::iViewMode=VK_F4;
   //Ra: na zewn¹trz wychodzimy w Train.cpp
   Camera.Reset(); //likwidacja obrotów - patrzy horyzontalnie na po³udnie
   if (Controlled) //jest pojazd do prowadzenia?
   {
    if (FreeFlyModeFlag)
    {//je¿eli poza kabin¹, przestawiamy w jej okolicê - OK
     //Camera.Pos=Train->pMechPosition+Normalize(Train->GetDirection())*20;
     Camera.Pos=Controlled->GetPosition()+(Controlled->MoverParameters->ActiveCab>=0?30:-30)*Normalize(Controlled->GetDirection())+vector3(0,5,0);
     Camera.LookAt=Controlled->GetPosition();
     Camera.RaLook(); //jednorazowe przestawienie kamery
     //¿eby nie bylo numerów z 'fruwajacym' lokiem - konsekwencja bujania pud³a
     if (Train)
     {Train->DynamicObject->ABuSetModelShake(vector3(0,0,0));
      Train->DynamicObject->bDisplayCab=false;
     }
    }
    else if (Train)
    {//korekcja ustawienia w kabinie - OK
     //Ra: czy to tu jest potrzebne, bo przelicza siê kawa³ek dalej?
     Camera.Pos=Train->pMechPosition;//Train.GetPosition1();
     Camera.Roll=atan(Train->pMechShake.x*Train->fMechRoll);       //hustanie kamery na boki
     Camera.Pitch-=atan(Train->vMechVelocity.z*Train->fMechPitch);  //hustanie kamery przod tyl
     if (Train->DynamicObject->MoverParameters->ActiveCab==0)
      Camera.LookAt=Train->pMechPosition+Train->GetDirection();
     else //patrz w strone wlasciwej kabiny
      Camera.LookAt=Train->pMechPosition+Train->GetDirection()*Train->DynamicObject->MoverParameters->ActiveCab;
     Train->pMechOffset.x=Train->pMechSittingPosition.x;
     Train->pMechOffset.y=Train->pMechSittingPosition.y;
     Train->pMechOffset.z=Train->pMechSittingPosition.z;
     Train->DynamicObject->bDisplayCab=true;
    }
   }
   else if (pDynamicNearest) //jeœli jest pojazd wykryty blisko
   {//patrzenie na najbli¿szy pojazd
    Camera.Pos=pDynamicNearest->GetPosition()+(pDynamicNearest->MoverParameters->ActiveCab>=0?30:-30)*Normalize(pDynamicNearest->GetDirection())+vector3(0,5,0);
    Camera.LookAt=pDynamicNearest->GetPosition();
    Camera.RaLook(); //jednorazowe przestawienie kamery
   }
  }
  else if (Global::iTextMode==-1)
  {//tu mozna dodac dopisywanie do logu przebiegu lokomotywy
   WriteLog("Number of textures used: "+AnsiString(Global::iTextures));
   return false;
  }
  Camera.Update(); //uwzglêdnienie ruchu wywo³anego klawiszami
 } //koniec bloku pomijanego przy nieaktywnym oknie
 double dt=GetDeltaTime();
 double iter;
 int n=1;
 if (dt>fMaxDt) //normalnie 0.01s
 {
  iter=ceil(dt/fMaxDt);
  n=iter;
  dt=dt/iter; //Ra: fizykê lepiej by by³o przeliczaæ ze sta³ym krokiem
 }
 if (n>20) n=20; //McZapkie-081103: przesuniecie granicy FPS z 10 na 5
 //blablabla
 Ground.Update(dt,n); //ABu: zamiast 'n' bylo: 'Camera.Type==tp_Follow'
 if (DebugModeFlag)
  if (GetAsyncKeyState(VK_ESCAPE)<0)
  {//yB doda³ przyspieszacz fizyki
   Ground.Update(dt,n);
   Ground.Update(dt,n);
   Ground.Update(dt,n);
   Ground.Update(dt,n); //5 razy
   //Ground.Update(dt,n); //jak jest za du¿o, to gubi eventy
   //Ground.Update(dt,n);
   //Ground.Update(dt,n);
   //Ground.Update(dt,n);
   //Ground.Update(dt,n); //10 razy
  }
 //Ground.Update(0.01,Camera.Type==tp_Follow);
 dt=GetDeltaTime();
 if (Train?Camera.Type==tp_Follow:false)
 {
  Train->UpdateMechPosition(dt);
  Camera.Pos=Train->pMechPosition;//Train.GetPosition1();
  Camera.Roll=atan(Train->pMechShake.x*Train->fMechRoll);       //hustanie kamery na boki
  Camera.Pitch-=atan(Train->vMechVelocity.z*Train->fMechPitch);  //hustanie kamery przod tyl
  //ABu011104: rzucanie pudlem
  vector3 temp;
  if (abs(Train->pMechShake.y)<0.25)
   temp=vector3(0,0,6*Train->pMechShake.y);
  else
   if ((Train->pMechShake.y)>0)
    temp=vector3(0,0,6*0.25);
   else
    temp=vector3(0,0,-6*0.25);
  if (Controlled) Controlled->ABuSetModelShake(temp);
  //ABu: koniec rzucania

  if (Train->DynamicObject->MoverParameters->ActiveCab==0)
   Camera.LookAt=Train->pMechPosition+Train->GetDirection();
  else  //patrz w strone wlasciwej kabiny
   Camera.LookAt=Train->pMechPosition+Train->GetDirection()*Train->DynamicObject->MoverParameters->ActiveCab; //-1 albo 1
  Camera.vUp=Train->GetUp();
 }
 Ground.CheckQuery();

 if (!Render()) return false;

//**********************************************************************************************************

 if (Train)
 {//rendering kabiny gdy jest oddzielnym modelem i ma byc wyswietlana
  vector3 vFront=Train->DynamicObject->GetDirection();
  if ((Train->DynamicObject->MoverParameters->CategoryFlag&2) && (Train->DynamicObject->MoverParameters->ActiveCab<0)) //TODO: zrobic to eleganciej z plynnym zawracaniem
     vFront=-vFront;
  vector3 vUp=vWorldUp; //sta³a
  vFront.Normalize();
  vector3 vLeft=CrossProduct(vUp,vFront);
  vUp= CrossProduct(vFront,vLeft);
  matrix4x4 mat;
  mat.Identity();
  mat.BasisChange(vLeft,vUp,vFront);
  Train->DynamicObject->mMatrix= Inverse(mat);
  glPushMatrix();
  //ABu: Rendering kabiny jako ostatniej, zeby bylo widac przez szyby, tylko w widoku ze srodka
  if ((Train->DynamicObject->mdKabina!=Train->DynamicObject->mdModel) && Train->DynamicObject->bDisplayCab && !FreeFlyModeFlag)
  {
    vector3 pos=Train->DynamicObject->GetPosition();
    glTranslatef(pos.x,pos.y,pos.z);
    glMultMatrixd(Train->DynamicObject->mMatrix.getArray());

//*yB: moje smuuugi 1
  //float lightsum=0;
  //int i;
  //for(i=1;i<3;i++)
  // {
  //  lightsum+=Global::diffuseDayLight[i];
  //  lightsum+=Global::ambientDayLight[i];
  // }
  if (Global::fLuminance<=0.25)
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
     if (Train->DynamicObject->iLights[0]&21)
     {//wystarczy jeden zapalony z przodu
      glTexCoord2f(0,0); glVertex3f( 15.0,0.0,  15.0);
      glTexCoord2f(1,0); glVertex3f(-15.0,0.0,  15.0);
      glTexCoord2f(1,1); glVertex3f(-15.0,2.5, 250.0);
      glTexCoord2f(0,1); glVertex3f( 15.0,2.5, 250.0);
     }
     if (Train->DynamicObject->iLights[1]&21)
     {//wystarczy jeden zapalony z ty³u
      glTexCoord2f(0,0); glVertex3f(-15.0,0.0, -15.0);
      glTexCoord2f(1,0); glVertex3f( 15.0,0.0, -15.0);
      glTexCoord2f(1,1); glVertex3f( 15.0,2.5,-250.0);
      glTexCoord2f(0,1); glVertex3f(-15.0,2.5,-250.0);
     }
    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_FOG);
   }
//yB: moje smuuugi 1 - koniec*/

    //oswietlenie kabiny
      GLfloat ambientCabLight[4]= { 0.5f,  0.5f, 0.5f, 1.0f };
      GLfloat diffuseCabLight[4]= { 0.5f,  0.5f, 0.5f, 1.0f };
      GLfloat specularCabLight[4]={ 0.5f,  0.5f, 0.5f, 1.0f };
      for (int li=0; li<3; li++)
      {//przyciemnienie standardowe
       ambientCabLight[li]= Global::ambientDayLight[li]*0.9;
       diffuseCabLight[li]= Global::diffuseDayLight[li]*0.5;
       specularCabLight[li]=Global::specularDayLight[li]*0.5;
      }
      switch (Train->DynamicObject->MyTrack->eEnvironment)
      {
       case e_canyon:
       {
        for (int li=0; li<3; li++)
        {
         diffuseCabLight[li] *=0.6;
         specularCabLight[li]*=0.7;
        }
       }
       break;
       case e_tunnel:
       {
        for (int li=0; li<3; li++)
        {
         ambientCabLight[li] *=0.3;
         diffuseCabLight[li] *=0.1;
         specularCabLight[li]*=0.2;
        }
       }
       break;
      }
      glLightfv(GL_LIGHT0,GL_AMBIENT,ambientCabLight);
      glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseCabLight);
      glLightfv(GL_LIGHT0,GL_SPECULAR,specularCabLight);
#ifdef USE_VBO
      if (Global::bUseVBO)
      {//renderowanie z u¿yciem VBO
       Train->DynamicObject->mdKabina->RaRender(SquareMagnitude(Global::pCameraPosition-pos),Train->DynamicObject->ReplacableSkinID,Train->DynamicObject->iAlpha);
       Train->DynamicObject->mdKabina->RaRenderAlpha(SquareMagnitude(Global::pCameraPosition-pos),Train->DynamicObject->ReplacableSkinID,Train->DynamicObject->iAlpha);
      }
      else
#endif
      {//renderowanie z Display List
       Train->DynamicObject->mdKabina->Render(SquareMagnitude(Global::pCameraPosition-pos),Train->DynamicObject->ReplacableSkinID,Train->DynamicObject->iAlpha);
       Train->DynamicObject->mdKabina->RenderAlpha(SquareMagnitude(Global::pCameraPosition-pos),Train->DynamicObject->ReplacableSkinID,Train->DynamicObject->iAlpha);
      }
      //przywrócenie standardowych, bo zawsze s¹ zmieniane
      glLightfv(GL_LIGHT0,GL_AMBIENT,Global::ambientDayLight);
      glLightfv(GL_LIGHT0,GL_DIFFUSE,Global::diffuseDayLight);
      glLightfv(GL_LIGHT0,GL_SPECULAR,Global::specularDayLight);
    }
    glPopMatrix ( );
//**********************************************************************************************************
 } //koniec if (Train)
/*
 if (Global::fMoveLight>=0)
 {//Ra: tymczasowy "zegar s³oneczny"
  float x=0.0f,y=10.0f,z=0.0f; //œrodek tarczy
  glColor3f(1.0f,1.0f,1.0f);
  glDisable(GL_LIGHTING);
  glBegin(GL_LINES); //linia
   x+=1.0*Global::lightPos[0];
   y+=1.0*Global::lightPos[1];
   z+=1.0*Global::lightPos[2];
   glVertex3f(x,y,z); //pocz¹tek wskazówki
   x+=10.0*Global::lightPos[0];
   y+=10.0*Global::lightPos[1];
   z+=10.0*Global::lightPos[2];
   glVertex3f(x,y,z); //koniec wskazuje kierunek S³oñca
  glEnd();
  glEnable(GL_LIGHTING);
 }
*/
    if (DebugModeFlag&&!Global::iTextMode)
     {
       OutText1="  FPS: ";
       OutText1+=FloatToStrF(GetFPS(),ffFixed,6,2);
       OutText1+=Global::iSlowMotion?"S":"N";


       if (GetDeltaTime()>=0.2)
        {
            OutText1+= " Slowing Down !!! ";
        }
     }
    /*if (Pressed(VK_F5))
       {Global::slowmotion=true;};
    if (Pressed(VK_F6))
       {Global::slowmotion=false;};*/

    if (Global::iTextMode==VK_F8)
    {
     Global::iViewMode=VK_F8;
     OutText1="  FPS: ";
     OutText1+=FloatToStrF(GetFPS(),ffFixed,6,2);
     if (Global::iSlowMotion)
      OutText1+=" (slowmotion "+AnsiString(Global::iSlowMotion)+")";
     OutText1+=", sectors: ";
     OutText1+=AnsiString(Ground.iRendered);
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
     Global::iViewMode=VK_F6;
       //OutText1=FloatToStrF(arg,ffFixed,2,4)+", ";
       //OutText1+=FloatToStrF(p2,ffFixed,7,4)+", ";
       GlobalTime->UpdateMTableTime(100);
    }


    if (Global::changeDynObj==true)
    {//ABu zmiana pojazdu - przejœcie do innego
       Train->dsbHasler->Stop();
       Train->dsbBuzzer->Stop();
       if (Train->dsbSlipAlarm) Train->dsbSlipAlarm->Stop(); //dŸwiêk alarmu przy poœlizgu
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
        temp=Train->DynamicObject->NextConnected; //pojazd od strony sprzêgu 1
        CabNr=(Train->DynamicObject->NextConnectedNo==0)?1:-1;
       }
       else
       {
        temp=Train->DynamicObject->PrevConnected; //pojazd od strony sprzêgu 0
        CabNr=(Train->DynamicObject->PrevConnectedNo==0)?1:-1;
       }
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
       Global::asHumanCtrlVehicle=Train->DynamicObject->GetName();
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

    if (Global::iTextMode==VK_F1)
    {//tekst pokazywany po wciœniêciu [F1]
     //Global::iViewMode=VK_F1;
     OutText1="Time: "+AnsiString((int)GlobalTime->hh)+":";
     int i=GlobalTime->mm; //bo inaczej potrafi zrobiæ "hh:010"
     if (i<10) OutText1+="0";
     OutText1+=AnsiString(i); //minuty
     OutText1+=":";
     i=floor(GlobalTime->mr); //bo inaczej potrafi zrobiæ "hh:mm:010"
     if (i<10) OutText1+="0";
     OutText1+=AnsiString(i);
     if (Global::bPause) OutText1+=" - paused";
     if (Controlled)
      if (Controlled->TrainParams)
      {OutText2=Controlled->TrainParams->ShowRelation();
       if (!OutText2.IsEmpty())
        if (Controlled->Mechanik)
         OutText2=Bezogonkow(OutText2+": -> "+Controlled->Mechanik->NextStop()); //dopisanie punktu zatrzymania
      }
     //double CtrlPos=Controlled->MoverParameters->MainCtrlPos;
     //double CtrlPosNo=Controlled->MoverParameters->MainCtrlPosNo;
     //OutText2="defrot="+FloatToStrF(1+0.4*(CtrlPos/CtrlPosNo),ffFixed,2,5);
     OutText3=""; //Pomoc w sterowaniu - [F9]";
    }
    else if (Global::iTextMode==VK_F12)
    {
     //Global::iViewMode=VK_F12;

       //Takie male info :)
       OutText1= AnsiString("Online documentation (PL): http://eu07.pl");
    }

    else if (Global::iTextMode==VK_F2)
    {//ABu: info dla najblizszego pojazdu!
     TDynamicObject *tmp;
     if (FreeFlyModeFlag)
     {//w trybie latania lokalizujemy wg mapy
/*
      int CouplNr=-2;
      tmp=Controlled->ABuScanNearestObject(Controlled->GetTrack(), 1, 500, CouplNr);
      if (tmp==NULL)
      {
       tmp=Controlled->ABuScanNearestObject(Controlled->GetTrack(), -1, 500, CouplNr);
       //if(tmp==NULL) tmp=Controlled;
      }
*/
      tmp=Ground.DynamicNearest(Camera.Pos);
     }
     else
      tmp=Controlled;
     if (tmp)
     {
      OutText3="";
      OutText1="Vehicle name:  "+AnsiString(tmp->MoverParameters->Name);
//yB      OutText1+="; d:  "+FloatToStrF(tmp->ABuGetDirection(),ffFixed,2,0);
      //OutText1=FloatToStrF(tmp->MoverParameters->Couplers[0].CouplingFlag,ffFixed,3,2)+", ";
      //OutText1+=FloatToStrF(tmp->MoverParameters->Couplers[1].CouplingFlag,ffFixed,3,2);
      if (tmp->Mechanik) //jeœli jest prowadz¹cy
      {//ostatnia komenda dla AI
       OutText1+=", command: "+tmp->Mechanik->OrderCurrent();
      }
      if (!tmp->MoverParameters->CommandLast.IsEmpty())
       OutText1+=AnsiString(", put: ")+tmp->MoverParameters->CommandLast;
      OutText2="Damage status: "+tmp->MoverParameters->EngineDescription(0);//+" Engine status: ";
      OutText2+="; Brake delay: ";
      if(tmp->MoverParameters->BrakeDelayFlag>1)
       if(tmp->MoverParameters->BrakeDelayFlag>2)
        OutText2+="R";
       else
       OutText2+="P";
      else
       OutText2+="G";
      OutText2+=AnsiString(", BTP:")+FloatToStrF(tmp->MoverParameters->LoadFlag,ffFixed,5,0);

//          OutText2+=AnsiString(", u:")+FloatToStrF(tmp->MoverParameters->u,ffFixed,3,3);
//          OutText2+=AnsiString(", N:")+FloatToStrF((tmp->MoverParameters->BrakePress*tmp->MoverParameters->P2FTrans-tmp->MoverParameters->BrakeCylSpring)*tmp->MoverParameters->BrakeCylNo*tmp->MoverParameters->BrakeCylMult[0]-tmp->MoverParameters->BrakeSlckAdj,ffFixed,4,0);
//          OutText3= AnsiString("BP: ")+FloatToStrF(tmp->MoverParameters->BrakePress,ffFixed,5,2)+AnsiString(", ");
//          OutText3+= AnsiString("PP: ")+FloatToStrF(tmp->MoverParameters->PipePress,ffFixed,5,2)+AnsiString(", ");
//          OutText3+= AnsiString("BVP: ")+FloatToStrF(tmp->MoverParameters->Volume,ffFixed,5,3)+AnsiString(", ");
//          OutText3+= FloatToStrF(tmp->MoverParameters->CntrlPipePress,ffFixed,5,3)+AnsiString(", ");
//          OutText3+= FloatToStrF(tmp->MoverParameters->Hamulec->GetCRP(),ffFixed,5,3)+AnsiString(", ");
//          OutText3+= FloatToStrF(tmp->MoverParameters->BrakeStatus,ffFixed,5,0)+AnsiString(", ");
//          OutText3+= AnsiString("HP: ")+FloatToStrF(tmp->MoverParameters->ScndPipePress,ffFixed,5,2)+AnsiString(". ");


//      OutText2+= FloatToStrF(tmp->MoverParameters->CompressorPower,ffFixed,5,0)+AnsiString(", ");
//yB      if(tmp->MoverParameters->BrakeSubsystem==Knorr) OutText2+=" Knorr";
//yB      if(tmp->MoverParameters->BrakeSubsystem==Oerlikon) OutText2+=" Oerlikon";
//yB      if(tmp->MoverParameters->BrakeSubsystem==Hik) OutText2+=" Hik";
//yB      if(tmp->MoverParameters->BrakeSubsystem==WeLu) OutText2+=" £estingha³s";
      //OutText2= " GetFirst: "+AnsiString(tmp->GetFirstDynamic(1)->MoverParameters->Name)+" Damage status="+tmp->MoverParameters->EngineDescription(0)+" Engine status: ";
      //OutText2+= " GetLast: "+AnsiString(tmp->GetLastDynamic(1)->MoverParameters->Name)+" Damage status="+tmp->MoverParameters->EngineDescription(0)+" Engine status: ";
      OutText3= AnsiString("BP: ")+FloatToStrF(tmp->MoverParameters->BrakePress,ffFixed,5,2)+AnsiString(", ");
      OutText3+= FloatToStrF(tmp->MoverParameters->BrakeStatus,ffFixed,5,0)+AnsiString(", ");
      OutText3+= AnsiString("PP: ")+FloatToStrF(tmp->MoverParameters->PipePress,ffFixed,5,2)+AnsiString("/");
      OutText3+= FloatToStrF(tmp->MoverParameters->ScndPipePress,ffFixed,5,2)+AnsiString(", ");
      OutText3+= AnsiString("BVP: ")+FloatToStrF(tmp->MoverParameters->Volume,ffFixed,5,3)+AnsiString(", ");
      OutText3+= FloatToStrF(tmp->MoverParameters->CntrlPipePress,ffFixed,5,3)+AnsiString(", ");
      OutText3+= FloatToStrF(tmp->MoverParameters->Hamulec->GetCRP(),ffFixed,5,3)+AnsiString(", ");
      OutText3+= FloatToStrF(tmp->MoverParameters->BrakeStatus,ffFixed,5,0)+AnsiString(", ");
//      OutText3+= AnsiString("BVP: ")+FloatToStrF(tmp->MoverParameters->BrakeVP(),ffFixed,5,2)+AnsiString(", ");

//      OutText3+= FloatToStrF(tmp->MoverParameters->CntrlPipePress,ffFixed,5,2)+AnsiString(", ");
//      OutText3+= FloatToStrF(tmp->MoverParameters->HighPipePress,ffFixed,5,2)+AnsiString(", ");
//      OutText3+= FloatToStrF(tmp->MoverParameters->LowPipePress,ffFixed,5,2)+AnsiString(", ");

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
       OutText4="";
       //OutText4+="Coupler 0: "+(tmp->PrevConnected?tmp->PrevConnected->GetName():AnsiString("NULL"))+" ("+AnsiString(tmp->MoverParameters->Couplers[0].CouplingFlag)+"), ";
       //OutText4+="Coupler 1: "+(tmp->NextConnected?tmp->NextConnected->GetName():AnsiString("NULL"))+" ("+AnsiString(tmp->MoverParameters->Couplers[1].CouplingFlag)+")";
       if (tmp->Mechanik)
       {//o ile jest ktoœ w œrodku
        OutText4=tmp->Mechanik->StopReasonText();
        if (tmp->Mechanik->eSignLast)
        {//jeœli ma zapamiêtany event semafora
         if (!OutText4.IsEmpty()) OutText4+=", "; //aby ³adniejszy odstêp by³
         OutText4+="Control event: "+Bezogonkow(tmp->Mechanik->eSignLast->asName); //nazwa eventu semafora
        }
         //OutText4+="  LPTI@A: "+IntToStr(tmp->Mechanik->LPTI)+"@"+IntToStr(tmp->Mechanik->LPTA);
       }
       if (Pressed(VK_F2))
       {WriteLog(OutText1);
        WriteLog(OutText2);
        WriteLog(OutText3);
        WriteLog(OutText4);
       }
      }
      else
      {
        OutText1="Camera position: "+FloatToStrF(Camera.Pos.x,ffFixed,6,2)+" "+FloatToStrF(Camera.Pos.y,ffFixed,6,2)+" "+FloatToStrF(Camera.Pos.z,ffFixed,6,2);
      }
      //OutText3= AnsiString("  Online documentation (PL, ENG, DE, soon CZ): http://www.eu07.pl");
      //OutText3="enrot="+FloatToStrF(Controlled->MoverParameters->enrot,ffFixed,6,2);
      //OutText3="; n="+FloatToStrF(Controlled->MoverParameters->n,ffFixed,6,2);
     }
    else if (Global::iTextMode==VK_F5)
    {//przesiadka do innego pojazdu
     if (FreeFlyModeFlag) //jeœli tryb latania
     {TDynamicObject *tmp=Ground.DynamicNearest(Camera.Pos,50,true); //³apiemy z obsad¹
      if (tmp)
       if (tmp!=Controlled)
       {if (Controlled) //jeœli mielismy pojazd
         Controlled->Mechanik->TakeControl(true); //oddajemy dotychczasowy AI
        if (DebugModeFlag?true:tmp->MoverParameters->Vel<=5.0)
        {Controlled=tmp; //przejmujemy nowy
         if (!Train) //jeœli niczym jeszcze nie jeŸdzilismy
          Train=new TTrain();
         if (Train->Init(Controlled))
          Controlled->Mechanik->TakeControl(false); //przejmujemy sterowanie
         else
          SafeDelete(Train); //i nie ma czym sterowaæ
        }
       }
      Global::iTextMode=0; //tryb neutralny
     }
/*

     OutText1=OutText2=OutText3=OutText4="";
     AnsiString flag[10]={"vmax", "tory", "smfr", "pjzd", "mnwr", "pstk", "brak", "brak", "brak", "brak"};
     if(tmp)
     if(tmp->Mechanik)
     {
      for(int i=0;i<15;i++)
      {
       int tmppar=floor(tmp->Mechanik->ProximityTable[i].Vel);
       OutText2+=(tmppar<1000?(tmppar<100?((tmppar<10)&&(tmppar>=0)?"   ":"  "):" "):"")+IntToStr(tmppar)+" ";
       tmppar=floor(tmp->Mechanik->ProximityTable[i].Dist);
       OutText3+=(tmppar<1000?(tmppar<100?((tmppar<10)&&(tmppar>=0)?"   ":"  "):" "):"")+IntToStr(tmppar)+" ";
       OutText1+=flag[tmp->Mechanik->ProximityTable[i].Flag]+" ";
      }
      for(int i=0;i<6;i++)
      {
       int tmppar=floor(tmp->Mechanik->ReducedTable[i]);
       OutText4+=flag[i]+":"+(tmppar<1000?(tmppar<100?((tmppar<10)&&(tmppar>=0)?"   ":"  "):" "):"")+IntToStr(tmppar)+" ";
      }
     }
*/
    }
    else if (Global::iTextMode==VK_F10)
    {//tu mozna dodac dopisywanie do logu przebiegu lokomotywy
     //Global::iViewMode=VK_F10;
     //return false;
     OutText1=AnsiString("To quit press [Y] key.");
     OutText3=AnsiString("Aby zakonczyc program, przycisnij klawisz [Y].");
    }
    else
    if (Controlled && DebugModeFlag && !Global::iTextMode)
    {
      OutText1+=AnsiString(";  vel ")+FloatToStrF(Controlled->GetVelocity(),ffFixed,6,2);
      OutText1+=AnsiString(";  pos ")+FloatToStrF(Controlled->GetPosition().x,ffFixed,6,2);
      OutText1+=AnsiString(" ; ")+FloatToStrF(Controlled->GetPosition().y,ffFixed,6,2);
      OutText1+=AnsiString(" ; ")+FloatToStrF(Controlled->GetPosition().z,ffFixed,6,2);
      OutText1+=AnsiString("; dist=")+FloatToStrF(Controlled->MoverParameters->DistCounter,ffFixed,8,4);

      //double a= acos( DotProduct(Normalize(Controlled->GetDirection()),vWorldFront));
//      OutText+= AnsiString(";   angle ")+FloatToStrF(a/M_PI*180,ffFixed,6,2);
      OutText1+=AnsiString("; d_omega ")+FloatToStrF(Controlled->MoverParameters->dizel_engagedeltaomega,ffFixed,6,3);
      OutText2 =AnsiString("ham zesp ")+FloatToStrF(Controlled->MoverParameters->BrakeCtrlPos,ffFixed,6,0);
      OutText2+=AnsiString("; ham pom ")+FloatToStrF(Controlled->MoverParameters->LocalBrakePos,ffFixed,6,0);
      //Controlled->MoverParameters->MainCtrlPos;
      //if (Controlled->MoverParameters->MainCtrlPos<0)
      //    OutText2+= AnsiString("; nastawnik 0");
//      if (Controlled->MoverParameters->MainCtrlPos>iPozSzereg)
          OutText2+= AnsiString("; nastawnik ") + (Controlled->MoverParameters->MainCtrlPos);
//      else
//          OutText2+= AnsiString("; nastawnik S") + Controlled->MoverParameters->MainCtrlPos;

      OutText2+=AnsiString("; bocznik:")+Controlled->MoverParameters->ScndCtrlPos;
      OutText2+=AnsiString("; I=")+FloatToStrF(Controlled->MoverParameters->Im,ffFixed,6,2);
      //OutText2+=AnsiString("; I2=")+FloatToStrF(Controlled->NextConnected->MoverParameters->Im,ffFixed,6,2);
      OutText2+=AnsiString("; V=")+FloatToStrF(Controlled->MoverParameters->RunningTraction.TractionVoltage,ffFixed,5,1);
      //OutText2+=AnsiString("; rvent=")+FloatToStrF(Controlled->MoverParameters->RventRot,ffFixed,6,2);
      OutText2+=AnsiString("; R=")+FloatToStrF(Controlled->MoverParameters->RunningShape.R,ffFixed,4,1);
      OutText2+=AnsiString(" An=")+FloatToStrF(Controlled->MoverParameters->AccN,ffFixed,4,2);
      //OutText2+=AnsiString("; P=")+FloatToStrF(Controlled->MoverParameters->EnginePower,ffFixed,6,1);
      OutText3+=AnsiString("cyl.ham. ")+FloatToStrF(Controlled->MoverParameters->BrakePress,ffFixed,5,2);
      OutText3+=AnsiString("; prz.gl. ")+FloatToStrF(Controlled->MoverParameters->PipePress,ffFixed,5,2);
      OutText3+=AnsiString("; zb.gl. ")+FloatToStrF(Controlled->MoverParameters->CompressedVolume,ffFixed,6,2);
//youBy - drugi wezyk
      OutText3+=AnsiString("; p.zas. ")+FloatToStrF(Controlled->MoverParameters->ScndPipePress,ffFixed,6,2);

      //ABu: testy sprzegow-> (potem przeniesc te zmienne z public do protected!)
      //OutText3+=AnsiString("; EnginePwr=")+FloatToStrF(Controlled->MoverParameters->EnginePower,ffFixed,1,5);
      //OutText3+=AnsiString("; nn=")+FloatToStrF(Controlled->NextConnectedNo,ffFixed,1,0);
      //OutText3+=AnsiString("; PR=")+FloatToStrF(Controlled->dPantAngleR,ffFixed,3,0);
      //OutText3+=AnsiString("; PF=")+FloatToStrF(Controlled->dPantAngleF,ffFixed,3,0);
      //if(Controlled->bDisplayCab==true)
      //OutText3+=AnsiString("; Wysw. kab");//+Controlled->mdKabina->GetSMRoot()->Name;
      //else
      //OutText3+=AnsiString("; test:")+AnsiString(Controlled->MoverParameters->TrainType[1]);

      //OutText3+=FloatToStrF(Train->DynamicObject->MoverParameters->EndSignalsFlag,ffFixed,3,0);;

      //OutText3+=FloatToStrF(Train->DynamicObject->MoverParameters->EndSignalsFlag&byte(((((1+Train->DynamicObject->MoverParameters->CabNo)/2)*30)+2)),ffFixed,3,0);;

      //OutText3+=AnsiString("; Ftmax=")+FloatToStrF(Controlled->MoverParameters->Ftmax,ffFixed,3,0);
      //OutText3+=AnsiString("; FTotal=")+FloatToStrF(Controlled->MoverParameters->FTotal/1000.0f,ffFixed,3,2);
      //OutText3+=AnsiString("; FTrain=")+FloatToStrF(Controlled->MoverParameters->FTrain/1000.0f,ffFixed,3,2);
      //Controlled->mdModel->GetSMRoot()->SetTranslate(vector3(0,1,0));

      //McZapkie: warto wiedziec w jakim stanie sa przelaczniki
      if (!Controlled->MoverParameters->Mains)
       OutText3+="  ";
      else
      {
       switch (Controlled->MoverParameters->ActiveDir)
       {
        case  1: {OutText3+=" -> "; break;}
        case  0: {OutText3+=" -- "; break;}
        case -1: {OutText3+=" <- "; break;}
       }
      }
      //OutText3+=AnsiString("; dpLocal ")+FloatToStrF(Controlled->MoverParameters->dpLocalValve,ffFixed,10,8);
      //OutText3+=AnsiString("; dpMain ")+FloatToStrF(Controlled->MoverParameters->dpMainValve,ffFixed,10,8);
      //McZapkie: predkosc szlakowa
      if (Controlled->MoverParameters->RunningTrack.Velmax==-1)
       {OutText3+=AnsiString(" Vtrack=Vmax ");}
      else
       {OutText3+=AnsiString(" Vtrack ")+FloatToStrF(Controlled->MoverParameters->RunningTrack.Velmax,ffFixed,8,2);}
//      WriteLog(Controlled->MoverParameters->TrainType.c_str());
      if ((Controlled->MoverParameters->EnginePowerSource.SourceType==CurrentCollector) || (Controlled->MoverParameters->TrainType==dt_EZT))
       {OutText3+=AnsiString(" zb.pant. ")+FloatToStrF(Controlled->MoverParameters->PantVolume,ffFixed,8,2);}
  //McZapkie: komenda i jej parametry
       if (Controlled->MoverParameters->CommandIn.Command!=AnsiString(""))
        OutText4=AnsiString("C:")+AnsiString(Controlled->MoverParameters->CommandIn.Command)
        +AnsiString(" V1=")+FloatToStrF(Controlled->MoverParameters->CommandIn.Value1,ffFixed,5,0)
        +AnsiString(" V2=")+FloatToStrF(Controlled->MoverParameters->CommandIn.Value2,ffFixed,5,0);
       if (Controlled->Mechanik && (Controlled->Mechanik->AIControllFlag==AIdriver))
        OutText4+=AnsiString("AI: Vd=")+FloatToStrF(Controlled->Mechanik->VelDesired,ffFixed,4,0)
        +AnsiString(" ad=")+FloatToStrF(Controlled->Mechanik->AccDesired,ffFixed,5,2)
        +AnsiString(" Pd=")+FloatToStrF(Controlled->Mechanik->ActualProximityDist,ffFixed,4,0)
        +AnsiString(" Vn=")+FloatToStrF(Controlled->Mechanik->VelNext,ffFixed,4,0);
     }

 glDisable(GL_LIGHTING);
 if (Controlled)
  SetWindowText(hWnd,AnsiString(Controlled->MoverParameters->Name).c_str());
 else
  SetWindowText(hWnd,Global::szSceneryFile); //nazwa scenerii
 glBindTexture(GL_TEXTURE_2D, 0);
 glColor4f(1.0f,0.0f,0.0f,1.0f);
 glLoadIdentity();

//ABu 150205: prosty help, zeby sie na forum nikt nie pytal, jak ma ruszyc :)

 if (Global::detonatoryOK)
 {
  //if (Pressed(VK_F9)) ShowHints(); //to nie dzia³a prawid³owo - prosili wy³¹czyæ
  if (Global::iTextMode==VK_F9)
  {//informacja o wersji, sposobie wyœwietlania i b³êdach OpenGL
   //Global::iViewMode=VK_F9;
   OutText1=Global::asVersion; //informacja o wersji
   OutText2=AnsiString("Rendering mode: ")+(Global::bUseVBO?"VBO":"Display Lists");
   if (Global::iMultiplayer) OutText2+=". Multiplayer is active";
   OutText2+=".";
   GLenum err=glGetError();
   if (err!=GL_NO_ERROR)
   {
    OutText3="OpenGL error "+AnsiString(err)+": "+Bezogonkow(AnsiString((char *)gluErrorString(err)));
   }
  }
  if (OutText1!="")
  {//ABu: i od razu czyszczenie tego, co bylo napisane
   glTranslatef(0.0f,0.0f,-0.50f);
   glRasterPos2f(-0.25f,0.20f);
   glPrint(OutText1.c_str());
   OutText1="";
   if (OutText2!="")
   {glRasterPos2f(-0.25f,0.19f);
    glPrint(OutText2.c_str());
    OutText2="";
   }
   if (OutText3!="")
   {glRasterPos2f(-0.25f,0.18f);
    glPrint(OutText3.c_str());
    OutText3="";
    if (OutText4!="")
    {glRasterPos2f(-0.25f, 0.17f);
     glPrint(OutText4.c_str());
     OutText4="";
    }
   }
  }
 }
 //if (Global::iViewMode!=Global::iTextMode)
 //{//Ra: taka maksymalna prowizorka na razie
 // WriteLog("Pressed function key F"+AnsiString(Global::iViewMode-111));
 // Global::iTextMode=Global::iViewMode;
 //}
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
   double calc_dist=hypot(calc_temp.x,calc_temp.z);
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

    //wyliczenie bezwzglêdnego kierunku kamery (do znikania niepotrzebnych pojazdów)
    if (FreeFlyModeFlag|!Controlled)
     Global::SetCameraRotation(Camera.Yaw-M_PI);
    else
    {//kierunek pojazdu oraz kierunek wzglêdny
     vector3 tempangle;
     double modelrotate;
     tempangle=Controlled->GetDirection()*(Controlled->MoverParameters->ActiveCab==-1 ? -1 : 1);
     modelrotate=ABuAcos(tempangle);
     Global::SetCameraRotation(Camera.Yaw-modelrotate);
     }
#ifdef USE_VBO
    if (Global::bUseVBO)
    {//renderowanie przez VBO
     if (!Ground.RaRender(Camera.Pos)) return false;
     //if (Global::bRenderAlpha) //Ra: wywalam tê flagê
      if (!Ground.RaRenderAlpha(Camera.Pos))
       return false;
    }
    else
#endif
    {//renderowanie przez Display List
     if (!Ground.Render(Camera.Pos)) return false;
     //if (Global::bRenderAlpha) //Ra: wywalam tê flagê
      if (!Ground.RenderAlpha(Camera.Pos))
       return false;
    }
    TSubModel::iInstance=(int)(Train?Train->DynamicObject:0); //¿eby nie robiæ cudzych animacji
//    if (Camera.Type==tp_Follow)
    if (Train)
        Train->Update();
//    if (Global::bRenderAlpha)
//     if (Controlled)
//        Train->RenderAlpha();
    glFlush();
    //Global::bReCompile=false; //Ra: ju¿ zrobiona rekompilacja

    ResourceManager::Sweep(Timer::GetSimulationTime());

    return true;
};

void TWorld::ShowHints(void)
{//Ra: nie u¿ywaæ tego, bo Ÿle dzia³a
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
      if (Controlled->GetFirstDynamic(1)->MoverParameters->BrakePress>0)
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
void __fastcall TWorld::OnCommandGet(DaneRozkaz *pRozkaz)
{//odebranie komunikatu z serwera
 if (pRozkaz->iSygn=='EU07')
  switch (pRozkaz->iComm)
  {
   case 0: //odes³anie identyfikatora wersji
    Ground.WyslijString(Global::asVersion,0); //przedsatwienie siê
    break;
   case 1: //odes³anie identyfikatora wersji
    Ground.WyslijString(Global::szSceneryFile,1); //nazwa scenerii
    break;
   case 2: //event
    if (Global::iMultiplayer)
    {//WriteLog("Komunikat: "+AnsiString(pRozkaz->Name1));
     TEvent *e=Ground.FindEvent(AnsiString(pRozkaz->cString+1,(unsigned)(pRozkaz->cString[0])));
     if (e)
      if (e->Type==tp_Multiple) //szybciej by by³o szukaæ tylko po tp_Multiple
       Ground.AddToQuery(e,NULL); //drugi parametr to dynamic wywo³uj¹cy - tu brak
    }
    break;
   case 3: //rozkaz dla AI
    if (Global::iMultiplayer)
    {int i=int(pRozkaz->cString[8]); //d³ugoœæ pierwszego ³añcucha (z przodu dwa floaty)
     TGroundNode* t=Ground.FindDynamic(AnsiString(pRozkaz->cString+11+i,(unsigned)pRozkaz->cString[10+i])); //nazwa pojazdu jest druga
     if (t)
     {
      t->DynamicObject->Mechanik->PutCommand(AnsiString(pRozkaz->cString+9,i),pRozkaz->fPar[0],pRozkaz->fPar[1],NULL,stopExt); //floaty s¹ z przodu
      WriteLog("AI command: "+AnsiString(pRozkaz->cString+9,i));
     }
    }
    break;
   case 4: //badanie zajêtoœci toru
    {
     TGroundNode* t=Ground.FindGroundNode(AnsiString(pRozkaz->cString+1,(unsigned)(pRozkaz->cString[0])),TP_TRACK);
     if (t)
      if (t->pTrack->IsEmpty())
       Ground.WyslijWolny(t->asName);
    }
    break;
   case 5: //ustawienie parametrów
    {
     if (*pRozkaz->iPar&0) //ustawienie czasu
     {double t=pRozkaz->fPar[1];
      GlobalTime->dd=floor(t); //niby nie powinno byæ dnia, ale...
      if (Global::fMoveLight>=0)
       Global::fMoveLight=t; //trzeba by deklinacjê S³oñca przeliczyæ
      GlobalTime->hh=floor(24*t)-24.0*GlobalTime->dd;
      GlobalTime->mm=floor(60*24*t)-60.0*(24.0*GlobalTime->dd+GlobalTime->hh);
      GlobalTime->mr=floor(60*60*24*t)-60.0*(60.0*(24.0*GlobalTime->dd+GlobalTime->hh)+GlobalTime->mm);
     }
    }
    break;
   case 6: //pobranie parametrów ruchu pojazdu
    if (Global::iMultiplayer)
    {TGroundNode* t=Ground.FindDynamic(AnsiString(pRozkaz->cString+1,(unsigned)pRozkaz->cString[0])); //nazwa pojazdu
     if (t)
      Ground.WyslijNamiary(t); //wys³anie informacji o pojeŸdzie
    }
    break;
  }
};

//---------------------------------------------------------------------------

