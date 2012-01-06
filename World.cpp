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
#include <gl/wglext.h>
#include <vector>
#include <string>
#include <cassert>
#include <sys/stat.h>
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
#include "screen.h"
#include "lights.h"
#include "shader.h"
#include "screen.h"
#include "misc.cpp"

//#include "LOG.h"
//#include "extensions/ARB_multitexture_extension.h"
//#include "extensions/EXT_compiled_vertex_array_extension.h"
//#include "extensions/EXT_texture_edge_clamp_extension.h"
//#include "extensions/NV_register_combiners_extension.h"
//#include "extensions/NV_vertex_program_extension.h"
//#include "normalisation_cube_map.h"
//#include "NV_vertex_program.h"
#include "screenshot.h"


#define TEXTURE_FILTER_CONTROL_EXT      0x8500
#define TEXTURE_LOD_BIAS_EXT            0x8501

//---------------------------------------------------------------------------
#pragma package(smart_init)

typedef void (APIENTRY *FglutBitmapCharacter)(void *font,int character); //typ funkcji
FglutBitmapCharacter glutBitmapCharacterDLL=NULL; //deklaracja zmiennej
HINSTANCE hinstGLUT32=NULL; //wskaŸnik do GLUT32.DLL
//GLUTAPI void APIENTRY glutBitmapCharacterDLL(void *font, int character);


AnsiString APPTITLE, CDATE, asSTARTLOAD, asENDLOAD, asBRIEFFILE;

HWND GL_HWND;   // QUEUEDZIO 130406 1808 - FOR SCREENSHOT
HDC GL_HDC;     // QUEUEDZIO 130406 1808 - FOR SCREENSHOT
HGLRC GL_hRC;   // QUEUEDZIO 130406 1808 - FOR SCREENSHOT

TSCREEN *SCR;
TWorld *W;
TLights LTS;
GLuint ttga;
GLuint LIGHTC;

int SCRMESSAGE =0;
double bysec = 0;

GLfloat  SPlane[4]  = {1, 0, 0, 0};
GLfloat  TPlane[4]  = {0, 1, 0, 0};
GLfloat  RPlane[4]  = {0, 0, 1, 0};
GLfloat  QPlane[4]  = {0, 0, 0, 1};


// -----------------------------------------------------------------------------


using namespace Timer;

const double fMaxDt= 0.01;

bool cdyn = false;


GLuint settexturefromchar(AnsiString CHR)
{
 GLuint tid;
 if (CHR == "a") tid = Global::cichar01;
 if (CHR == "b") tid = Global::cichar02;
 if (CHR == "c") tid = Global::cichar03;
 if (CHR == "d") tid = Global::cichar04;
 if (CHR == "e") tid = Global::cichar05;
 if (CHR == "f") tid = Global::cichar06;
 if (CHR == "g") tid = Global::cichar07;
 if (CHR == "h") tid = Global::cichar08;
 if (CHR == "i") tid = Global::cichar09;
 if (CHR == "j") tid = Global::cichar10;
 if (CHR == "k") tid = Global::cichar11;
 if (CHR == "l") tid = Global::cichar12;
 if (CHR == "m") tid = Global::cichar13;
 if (CHR == "n") tid = Global::cichar14;
 if (CHR == "o") tid = Global::cichar15;
 if (CHR == "p") tid = Global::cichar16;
 if (CHR == "q") tid = Global::cichar17;
 if (CHR == "r") tid = Global::cichar18;
 if (CHR == "s") tid = Global::cichar19;
 if (CHR == "t") tid = Global::cichar20;
 if (CHR == "u") tid = Global::cichar21;
 if (CHR == "v") tid = Global::cichar22;
 if (CHR == "w") tid = Global::cichar23;
 if (CHR == "x") tid = Global::cichar24;
 if (CHR == "y") tid = Global::cichar25;
 if (CHR == "z") tid = Global::cichar26;
 if (CHR == "1") tid = Global::cichar27;
 if (CHR == "2") tid = Global::cichar28;
 if (CHR == "3") tid = Global::cichar29;
 if (CHR == "4") tid = Global::cichar30;
 if (CHR == "5") tid = Global::cichar31;
 if (CHR == "6") tid = Global::cichar32;
 if (CHR == "7") tid = Global::cichar33;
 if (CHR == "8") tid = Global::cichar34;
 if (CHR == "9") tid = Global::cichar35;
 if (CHR == "0") tid = Global::cichar36;
 if (CHR == " ") tid = Global::cichar37;
 if (CHR == "_") tid = Global::cichar38;
 if (CHR == ":") tid = Global::cichar39;
 if (CHR == "!") tid = Global::cichar40;
 if (CHR == "?") tid = Global::cichar41;
 if (CHR == "*") tid = Global::cichar42;
 if (CHR == "/") tid = Global::cichar43;
 if (CHR == "\"") tid = Global::cichar44;
 if (CHR == "%") tid = Global::cichar45;
 if (CHR == "(") tid = Global::cichar46;
 if (CHR == ")") tid = Global::cichar47;
 if (CHR == ".") tid = Global::cichar48;
 if (CHR == "-") tid = Global::cichar49;
 if (CHR == "+") tid = Global::cichar50;
 if (CHR == "=") tid = Global::cichar51;
 return tid;
}

void __fastcall TWorld::setdirecttable(AnsiString DIRECTION)
{
 DIRECTION = LowerCase(DIRECTION);
 
 Global::dichar01 = settexturefromchar(DIRECTION[1]);
 Global::dichar02 = settexturefromchar(DIRECTION[2]);
 Global::dichar03 = settexturefromchar(DIRECTION[3]);
 Global::dichar04 = settexturefromchar(DIRECTION[4]);
 Global::dichar05 = settexturefromchar(DIRECTION[5]);
 Global::dichar06 = settexturefromchar(DIRECTION[6]);
 Global::dichar07 = settexturefromchar(DIRECTION[7]);
 Global::dichar08 = settexturefromchar(DIRECTION[8]);
 Global::dichar09 = settexturefromchar(DIRECTION[9]);
 Global::dichar10 = settexturefromchar(DIRECTION[10]);
 Global::dichar11 = settexturefromchar(DIRECTION[11]);
 Global::dichar12 = settexturefromchar(DIRECTION[12]);
 Global::dichar13 = settexturefromchar(DIRECTION[13]);
 Global::dichar14 = settexturefromchar(DIRECTION[14]);
 Global::dichar15 = settexturefromchar(DIRECTION[15]);
 Global::dichar16 = settexturefromchar(DIRECTION[16]);
 Global::dichar17 = settexturefromchar(DIRECTION[17]);
 Global::dichar18 = settexturefromchar(DIRECTION[18]);
 Global::dichar19 = settexturefromchar(DIRECTION[19]);
 Global::dichar20 = settexturefromchar(DIRECTION[20]);
 Global::dichar21 = settexturefromchar(DIRECTION[21]);
 Global::dichar22 = settexturefromchar(DIRECTION[22]);
 Global::dichar23 = settexturefromchar(DIRECTION[23]);
 Global::dichar24 = settexturefromchar(DIRECTION[24]);
 Global::dichar25 = settexturefromchar(DIRECTION[25]);
 Global::dichar26 = settexturefromchar(DIRECTION[26]);
 Global::dichar27 = settexturefromchar(DIRECTION[27]);
 Global::dichar28 = settexturefromchar(DIRECTION[28]);
 Global::dichar29 = settexturefromchar(DIRECTION[29]);
 Global::dichar30 = settexturefromchar(DIRECTION[30]);
 Global::dichar31 = settexturefromchar(DIRECTION[31]);
 Global::dichar32 = settexturefromchar(DIRECTION[32]);
}

// DLA ZEGARA DIODOWEGO
GLuint settexturefromid(int num)
{
 GLuint tid;
 if (num == 0) tid = Global::cifre_00;
 if (num == 1) tid = Global::cifre_01;
 if (num == 2) tid = Global::cifre_02;
 if (num == 3) tid = Global::cifre_03;
 if (num == 4) tid = Global::cifre_04;
 if (num == 5) tid = Global::cifre_05;
 if (num == 6) tid = Global::cifre_06;
 if (num == 7) tid = Global::cifre_07;
 if (num == 8) tid = Global::cifre_08;
 if (num == 9) tid = Global::cifre_09;
// if (num == ",") tid = Global::charx_01;
// if (num == ":") tid = Global::charx_02;
 return tid;
}

String  getexedateB()
    {
    AnsiString fn = ExtractFileName(  ParamStr(0));
    TDateTime Dt;
    Dt = FileDateToDateTime(FileAge(Global::g___APPDIR + fn));
    String Data = Dt.FormatString("yyyy-mm-dd");
    return Data;
    }

String  getexedateA()
    {
    AnsiString fn = ExtractFileName(  ParamStr(0));
    TDateTime Dt;
    Dt = FileDateToDateTime(FileAge(Global::g___APPDIR + fn));
    String Data = Dt.FormatString("ddmmyyyy-hhnnss");
    return Data;
    }

__int64 getexesize()
    {
    AnsiString fn = ExtractFileName(  ParamStr(0));
    TFileStream *Str = new TFileStream(Global::g___APPDIR + fn, fmOpenRead);

    __int64 rozmiar1, rozmiar2;
    rozmiar1 = Str->Size;
    rozmiar2 = Str->Size / 1024;  // wlasciwy odczyt rozmiaru pliku w KB
    delete Str;   // zwalnianie zasobu
    return rozmiar1;
    }

int getprogressfile()
    {
     Global::iNODES = 1000000;
     AnsiString asfile;
     AnsiString cscn = Global::szSceneryFile;
     asfile = "DATA\\loader\\" + cscn + ".TXT";

     if (FileExists(asfile))
        {
         Global::bfirstloadingscn = false;
         Global::OTHERS->LoadFromFile(asfile);
         Global::iNODES = StrToInt(Global::OTHERS->Strings[0]);
         if (Global::OTHERS->Strings[0] != "") return StrToInt(Global::OTHERS->Strings[0]);     // NA TYM SIE POTRAFI WYWALIC, CZEMU?

        }
       else return 1000000;
    }

int setprogressfile()
    {
     //Global::iNODES = 1000000;
     AnsiString asfile;
     AnsiString cscn = Global::szSceneryFile;
     asfile = Global::g___APPDIR + "DATA\\loader\\" + cscn + ".TXT";
     WriteLog("PROGRESS FILE: " + asfile);
     Global::OTHERS->Clear();
     Global::OTHERS->Add(IntToStr(Global::iPARSERBYTESPASSED));
     Global::OTHERS->SaveToFile(asfile);
     WriteLog("PROGRESS FILE UPDATED ");
    }

bool FEX(AnsiString filename)
 {
  if (FileExists(filename)) return 1; else return 0;
 }

__fastcall SETALPHA()
{
    glEnable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER,0.04);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);                        // COBY ZPOWROTEM WLACZYC POPRAWNE RENDEROWANIE ALPHA, PO INFORMACJACH
    glDepthFunc(GL_LEQUAL);                                                   // BEZ TEGO BYLO WIEKSZE PRZENIKANIE I OBWODKI NA KANALE ALPHA???
}


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

 Global::OTHERS = new TStringList();
 Global::CHANGES = new TStringList();
 Global::KEYBOARD = new TStringList();
 Global::CONSISTF = new TStringList;
 Global::CONSISTB = new TStringList;
 Global::CONSISTA = new TStringList;

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

//TDynamicObject *Controlled=NULL; //pojazd, który prowadzimy


AnsiString PrintFileVersion( AnsiString FILE )
{
DWORD      verHandle = NULL;
UINT       size      = 0;
LPBYTE     lpBuffer  = NULL;
DWORD      verSize   = GetFileVersionInfoSize( FILE.c_str(), &verHandle);
AnsiString VERSTR;

if (verSize != NULL)
{
    LPSTR verData = new char[verSize];

    if (GetFileVersionInfo( FILE.c_str(), verHandle, verSize, verData))
    {
        if (VerQueryValue(verData,"\\",(VOID FAR* FAR*)&lpBuffer,&size))
        {
                if (size)
                {
                        VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *)lpBuffer;
                        if (verInfo->dwSignature == 0xfeef04bd)
                        {
                                int major = HIWORD(verInfo->dwFileVersionMS);
                                int minor = LOWORD(verInfo->dwFileVersionMS);
                                int relea = HIWORD(verInfo->dwFileVersionLS);
                                int build = LOWORD(verInfo->dwFileVersionLS);

                                //WriteLog("MAJOR  : " + IntToStr( major ));
                                //WriteLog("MINOR  : " + IntToStr( minor ));
                                //WriteLog("RELEASE: " + IntToStr( relea ));
                                //WriteLog("BUILD  : " + IntToStr( build ));

                                VERSTR = IntToStr(major) + "." + IntToStr(minor) + "." + IntToStr(relea) + "." + IntToStr(build);
                                Global::APPCOMP = VERSTR;
                        }
                }
        }
    }

    return VERSTR;
}


}

bool __fastcall TWorld::Init(HWND NhWnd, HDC hDC)
{
    HANDLE proces = GetCurrentProcess();
    BOOL isSetPrioritySuccessful = SetPriorityClass( proces, REALTIME_PRIORITY_CLASS);

    Global::gtc1 =  GetTickCount();

    WriteLog("                                                                                                                                                        ");
    WriteLog("888b     d888           .d8888b.                                        8888888888 888     888  .d8888b. 8888888888          d8888   .d8888b.      d8888");
    WriteLog("8888b   d8888          d88P  Y88b                                       888        888     888 d88P  Y88b      d88P         d8P888  d88P  Y88b    d8P888");
    WriteLog("88888b.d88888          Y88b.                                            888        888     888 888    888     d88P         d8P 888         888   d8P 888");
    WriteLog("888Y88888P888  8888b.   'Y888b.  88888888 888  888 88888b.   8888b.     8888888    888     888 888    888    d88P         d8P  888       .d88P  d8P  888");
    WriteLog("888 Y888P 888     '88b     'Y88b.   d88P  888  888 888 '88b     '88b    888        888     888 888    888 88888888       d88   888   .od888P'  d88   888");
    WriteLog("888  Y8P  888 .d888888       '888  d88P   888  888 888  888 .d888888    888        888     888 888    888  d88P   888888 8888888888 d88P'      8888888888");
    WriteLog("888   '   888 888  888 Y88b  d88P d88P    Y88b 888 888  888 888  888    888        Y88b. .d88P Y88b  d88P d88P                 888  888'             888");
    WriteLog("888       888 'Y888888  'Y8888P' 88888888  'Y88888 888  888 'Y888888    8888888888  'Y88888P'   'Y8888P' d88P                  888  888888888        888");
    WriteLog("                                               888                      ");
    WriteLog("                                          Y8b d88P                      ");
    WriteLog("                                           'Y88P'                       ");

    WriteLog(" ");

    APPTITLE = "Symulator Pojazdow Trakcyjnych MaSzyna EP07-424 - WERSJA DEWELOPERSKA, ";

    GL_HWND = NhWnd;                                                            // GLOBAL WINDOW HANDLE
    GL_HDC = hDC;

    //WriteLog("POBIERANIE CZASU ROZPOCZECIA LADOWANIA SCENERII...");
    asSTARTLOAD = FormatDateTime("hh:mm:ss ", Now());
    CDATE = FormatDateTime("ddmmyy hhmmss", Now());

    //WriteLog("POBIERANIE SCIEZKI DO PLIKU EXE...");
    Global::g___APPDIR = ExtractFilePath(ParamStr(0));


    //WriteLog("USTAWIANIE KATALOGU DLA ZRZUTOW EKRANU...");                       // APPLICATION DIRECTORY CONTAINER
    Global::asSSHOTDIR = Global::g___APPDIR + "screenshots\\" + Global::asSSHOTSUBDIR;       // SCREENSHOTS DIRECTORY CONTAINER
    CreateDir(Global::g___APPDIR + "screenshots\\");
    CreateDir(Global::asSSHOTDIR);
    WriteLog(" ");

    CONSOLE__init();


    // POBIERANIE INFORMACJI O EXE ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

    AnsiString fn = ExtractFileName(  ParamStr(0));

    Global::iNODES = getprogressfile();                                         // LOADING PROGRESSBAR BYTES

    Global::APPDATE = getexedateA();                                            // DATA MODYFIKACJI

    Global::APPDAT2 = getexedateB();

    //Global::APPSIZE = AnsiString(getexesize()) + "B" ;                        // ROZMIAR PLIKU EXE

    Global::APPVERS = Global::APPDATE + ", release " + PrintFileVersion(ParamStr(0));    // WERSJA

    Controlled = NULL;
    SCR = new TSCREEN;
    double time = (double)Now();
    Global::hWnd = NhWnd;                                                          //do WM_COPYDATA
    Global::detonatoryOK = true;


#if sizeof(TSubModel)!=256
 Error("Wrong sizeof(TSubModel) is "+AnsiString(sizeof(TSubModel)));
 return false;
#endif

WriteLog(AnsiString(sizeof(TSubModel)));

    WriteLog("Starting MaSzyna rail vehicle simulator.");
    WriteLog(" ");
    //WriteLog(Global::asVersion);

    PrintFileVersion(ParamStr(0));
    WriteLog("Compilation " + Global::APPDATE + ", release " + PrintFileVersion(ParamStr(0)) + " - UNOFFICIAL <wersja niepubliczna>");
    WriteLog("Online documentation and additional files on http://www.eu07.pl");
    WriteLog("Authors: Marcin_EU, McZapkie, ABu, Winger, Tolaris, nbmx_EU, OlO_EU, Bart, Quark-t, ShaxBee, Oli_EU, youBy, Ra, QueuedEU, Bombardier and others");
    WriteLog(" ");
    WriteLog(" ");
    WriteLog(" ");
    WriteLog(" ");

    WriteLog("ZMIENNE SRODOWISKOWE");
    WriteLog(" ");

    //WriteLog("EXE " + Global::APPDATE + " " + Global::APPSIZE + " (" + AnsiString(getexesize()/1024) + "KB)");

    WriteLog(AnsiString("APPDIR= " + Global::g___APPDIR).c_str());

    WriteLog(AnsiString("SCRDIR= " + Global::asSSHOTDIR).c_str());

    WriteLog(AnsiString("SCENERY= ") + AnsiString(Global::szSceneryFile).c_str() );

    WriteLog(AnsiString("RUN DATE= " + CDATE));
    WriteLog(" ");

    WriteLog(" ");
    WriteLog(" ");
    WriteLog("Compile machine windows xp pro 3GB, DirextX 9.0c || GFX GF8600GT CORE 560Mhz 256MB PCIE,  BIOS VIDEO: 60.84.5E.00.00, driver rev 2009-01-16 6.14.11.8151 ");
    WriteLog(" ");




//Winger030405: sprawdzanie sterownikow

 
    AnsiString glrenderer= ((char*) glGetString(GL_RENDERER));
    AnsiString glvendor= ((char*) glGetString(GL_VENDOR));
    AnsiString glver=((char*)glGetString(GL_VERSION));

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

    WriteLog("Renderer: " + AnsiString(glrenderer.c_str()));
    WriteLog("Vendor: " + AnsiString(glvendor.c_str()));
    WriteLog("OpenGL ver: " + AnsiString(glver.c_str()));
    WriteLog("Supported Extensions:");

    Global::OTHERS->Clear();
    Global::OTHERS->Add((char*) glGetString(GL_EXTENSIONS));
    GLint maxtexw, maxtexh, texsize;

    AnsiString asext;
    AnsiString what1 = Global::OTHERS->Strings[0];
    int extnum = 0;
    for (int loop1=0; loop1<392; loop1++)
    {
     asext = what1.SubString(1, what1.Pos(" ")-1); what1.Delete(1,what1.Pos(" "));
     if (asext != "")
     {
      WriteLog(asext.c_str());
      extnum++;
     }
    }
    WriteLog("------------------------------" );
    WriteLog("EXTENSIONS: " + IntToStr(extnum));
    WriteLog(" ");

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
    WriteLog(" ");

/*-----------------------Render Initialization----------------------*/

    WriteLog(" ");
    WriteLog(" ");
    WriteLog("RENDER INITIALIZATION ******************************** ");
    WriteLog(" ");

    if (Global::fOpenGL >= 1.2) glTexEnvf(TEXTURE_FILTER_CONTROL_EXT,TEXTURE_LOD_BIAS_EXT,-1); // nie dzia³a w 1.1
    GLfloat FogColor[]={1.0f,1.0f,1.0f,1.0f};
    glMatrixMode(GL_MODELVIEW);                                                  // Select The Modelview Matrix
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);                          // Clear screen and depth buffer
    glLoadIdentity();

    WriteLog("glClearColor (FogColor[0], FogColor[1], FogColor[2], 0.0); ");
    glClearColor (0.2, 0.4, 0.33, 1.0);                                          // Background Color

    WriteLog("glFogfv(GL_FOG_COLOR, FogColor);");
    glFogfv(GL_FOG_COLOR, FogColor);					        // Set Fog Color

    WriteLog("glClearDepth(1.0f);  ");
    glClearDepth(1.0f);                                                         // ZBuffer Value


    glEnable(GL_NORMALIZE);
//    glEnable(GL_RESCALE_NORMAL);

    glEnable(GL_CULL_FACE);
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

    WriteLog("glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);");
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	                        // Really Nice Perspective Calculations
    glHint(GL_PERSPECTIVE_CORRECTION_HINT |GL_POINT_SMOOTH_HINT | GL_POLYGON_SMOOTH_HINT, GL_NICEST);         // , GL_DONT_CARE

    WriteLog("glPolygonMode(GL_FRONT, GL_FILL);");
    glPolygonMode(GL_FRONT, GL_FILL);
    WriteLog("glFrontFace(GL_CCW);");
    glFrontFace(GL_CCW);		// Counter clock-wise polygons face out
    WriteLog("glEnable(GL_CULL_FACE);	");
    glEnable(GL_CULL_FACE);		// Cull back-facing triangles
    WriteLog("glLineWidth(1.0f);");
    glLineWidth(1.0f);
    WriteLog("glPointSize(2.0f);");
    glPointSize(2.0f);

    // LINES ANITIALIASING
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);                                     // GL_NICEST, GL_FASTEST
    glLineWidth(0.5);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_ARB_point_sprite);


// ----------- LIGHTING SETUP -----------

    WriteLog(" ");
    WriteLog(" ");
    WriteLog("LIGHTING SETUP *************************************** ");
    WriteLog(" ");

    vector3 lp= Normalize(vector3(-500, 500, 200));

    Global::lightPos[0]= lp.x;
    Global::lightPos[1]= lp.y;
    Global::lightPos[2]= lp.z;
    Global::lightPos[3]= 0.0f;

    Global::SUNROTX = 0.0;
    Global::SUNROTY = 0.0;

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

    LTS.InitLights();    // LS0002



    //glColor() ma zmieniaæ kolor wybrany w glColorMaterial()
    WriteLog("glEnable(GL_COLOR_MATERIAL);");
    glEnable(GL_COLOR_MATERIAL);

    // Set Material properties to follow glColor values
    WriteLog("glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);");
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    WriteLog("glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, whiteLight );");
    glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, Global::whiteLight );

    WriteLog("glEnable(GL_LIGHTING);");
    glEnable(GL_LIGHTING);

    WriteLog("glHint(GL_FOG_HINT, GL_NICEST);");
    glHint(GL_FOG_HINT, GL_NICEST);					        // Fog Hint Value
    WriteLog("glFogi(GL_FOG_MODE, GL_LINEAR);");
    glFogi(GL_FOG_MODE, GL_LINEAR);			                        // Fog Mode
    WriteLog("glFogfv(GL_FOG_COLOR, FogColor);");
    glFogfv(GL_FOG_COLOR, FogColor);
    WriteLog("glFogf(GL_FOG_DENSITY, 0.594f);");
    glFogf(GL_FOG_DENSITY, 0.594f);						// How Dense Will The Fog Be				        // Set Fog Color
    WriteLog("glFogf(GL_FOG_START, 1000.0f);");
    glFogf(GL_FOG_START, 10.0f);						// Fog Start Depth
    WriteLog("glFogf(GL_FOG_END, 2000.0f);");
    glFogf(GL_FOG_END, 100.0f);							// Fog End Depth
    WriteLog("glEnable(GL_FOG);");
    glEnable(GL_FOG);								// Enables GL_FOG

    Global::fFogStart = 10.0f;
    Global::fFogEnd = 100.0f;
    
    //Ra: ustawienia testowe
    glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT,GL_NICEST);

    SetForegroundWindow(hWnd);

    
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

    SetForegroundWindow(hWnd);

    SwapBuffers(hDC);
    
    glLoadIdentity();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glTranslatef(0.0f,0.0f,-0.50f);
    glDisable(GL_DEPTH_TEST);			// Disables depth testing
    glColor3f(3.0f,3.0f,3.0f);

        GLuint logo;
        logo=TTexturesManager::GetTextureID("logo",6);
//        glBindTexture(GL_TEXTURE_2D,logo);       // Select our texture
//	glBegin(GL_QUADS);		        // Drawing using triangles
//		glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.28f, -0.22f,  0.0f);	// Bottom left of the texture and quad
//		glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.28f, -0.22f,  0.0f);	// Bottom right of the texture and quad
//		glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.28f,  0.22f,  0.0f);	// Top right of the texture and quad
//		glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.28f,  0.22f,  0.0f);	// Top left of the texture and quad
//	glEnd();

    AnsiString cscn = Global::szSceneryFile;
    AnsiString clok = Global::asHumanCtrlVehicle;
    asBRIEFFILE = "DATA\\briefs\\" + cscn + "-" + clok + ".tga";

    Global::asWINTITLE = Global::asHumanCtrlVehicle;

    if (!FEX(asBRIEFFILE)) Global::bloaderbriefing = false;

    WriteLog(asBRIEFFILE);

    Global::bfonttex        = TTexturesManager::GetTextureID(AnsiString("data\\menu\\menu_xfont.bmp").c_str());
    Global::texconsole      = TTexturesManager::GetTextureID(AnsiString("data\\menu\\cons_backg.bmp").c_str());
    Global::loaderlogo      = TTexturesManager::GetTextureID(AnsiString("data\\mlogo.bmp").c_str());
    Global::loaderbrief     = TTexturesManager::GetTextureID(AnsiString(asBRIEFFILE).c_str());
    Global::texmenu_backg   = TTexturesManager::GetTextureID(AnsiString("data\\lbackg.bmp").c_str());
    ttga                    = TTexturesManager::GetTextureID(AnsiString("data\\radiation_box.tga").c_str());
    LIGHTC                  = TTexturesManager::GetTextureID(AnsiString("data\\art\\texture1.bmp").c_str());
    Global::semlense1       = TTexturesManager::GetTextureID(AnsiString("data\\art\\semlense1.bmp").c_str());
    Global::SCRFILTER       = TTexturesManager::GetTextureID(AnsiString("data\\art\\scrfilter1.bmp").c_str());

    Global::texturetab[3]   = TTexturesManager::GetTextureID(AnsiString("data\\art\\particle.bmp").c_str());
    Global::texturetab[4]   = TTexturesManager::GetTextureID(AnsiString("data\\art\\sun5.bmp").c_str());
    Global::texturetab[5]   = TTexturesManager::GetTextureID(AnsiString("data\\art\\sun3.bmp").c_str());
    Global::texturetab[6]   = TTexturesManager::GetTextureID(AnsiString("data\\art\\sun4.bmp").c_str());
    Global::texturetab[7]   = TTexturesManager::GetTextureID(AnsiString("data\\art\\glow11.bmp").c_str());
    Global::texturetab[8]   = TTexturesManager::GetTextureID(AnsiString("data\\art\\Streaks4.bmp").c_str());
    Global::texturetab[9]   = TTexturesManager::GetTextureID(AnsiString("data\\art\\moon1.bmp").c_str());

    Global::cifre_00 = TTexturesManager::GetTextureID(AnsiString("data\\digit\\0.bmp").c_str());;
    Global::cifre_01 = TTexturesManager::GetTextureID(AnsiString("data\\digit\\1.bmp").c_str());
    Global::cifre_02 = TTexturesManager::GetTextureID(AnsiString("data\\digit\\2.bmp").c_str());
    Global::cifre_03 = TTexturesManager::GetTextureID(AnsiString("data\\digit\\3.bmp").c_str());
    Global::cifre_04 = TTexturesManager::GetTextureID(AnsiString("data\\digit\\4.bmp").c_str());
    Global::cifre_05 = TTexturesManager::GetTextureID(AnsiString("data\\digit\\5.bmp").c_str());
    Global::cifre_06 = TTexturesManager::GetTextureID(AnsiString("data\\digit\\6.bmp").c_str());
    Global::cifre_07 = TTexturesManager::GetTextureID(AnsiString("data\\digit\\7.bmp").c_str());
    Global::cifre_08 = TTexturesManager::GetTextureID(AnsiString("data\\digit\\8.bmp").c_str());
    Global::cifre_09 = TTexturesManager::GetTextureID(AnsiString("data\\digit\\9.bmp").c_str());
    Global::charx_01 = TTexturesManager::GetTextureID(AnsiString("data\\digit\\,.bmp").c_str());
    Global::charx_02 = TTexturesManager::GetTextureID(AnsiString("data\\digit\\_.bmp").c_str());

    Global::dhcifre_00 = TTexturesManager::GetTextureID(AnsiString("data\\digit\\hd0.bmp").c_str());;
    Global::dhcifre_01 = TTexturesManager::GetTextureID(AnsiString("data\\digit\\hd1.bmp").c_str());
    Global::dhcifre_02 = TTexturesManager::GetTextureID(AnsiString("data\\digit\\hd2.bmp").c_str());
    Global::dhcifre_03 = TTexturesManager::GetTextureID(AnsiString("data\\digit\\hd3.bmp").c_str());
    Global::dhcifre_04 = TTexturesManager::GetTextureID(AnsiString("data\\digit\\hd4.bmp").c_str());
    Global::dhcifre_05 = TTexturesManager::GetTextureID(AnsiString("data\\digit\\hd5.bmp").c_str());
    Global::dhcifre_06 = TTexturesManager::GetTextureID(AnsiString("data\\digit\\hd6.bmp").c_str());
    Global::dhcifre_07 = TTexturesManager::GetTextureID(AnsiString("data\\digit\\hd7.bmp").c_str());
    Global::dhcifre_08 = TTexturesManager::GetTextureID(AnsiString("data\\digit\\hd8.bmp").c_str());
    Global::dhcifre_09 = TTexturesManager::GetTextureID(AnsiString("data\\digit\\hd9.bmp").c_str());

    Global::cichar01 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\a.bmp").c_str());
    Global::cichar02 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\b.bmp").c_str());
    Global::cichar03 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\c.bmp").c_str());
    Global::cichar04 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\d.bmp").c_str());
    Global::cichar05 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\e.bmp").c_str());
    Global::cichar06 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\f.bmp").c_str());
    Global::cichar07 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\g.bmp").c_str());
    Global::cichar08 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\h.bmp").c_str());
    Global::cichar09 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\i.bmp").c_str());
    Global::cichar10 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\j.bmp").c_str());
    Global::cichar11 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\k.bmp").c_str());
    Global::cichar12 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\l.bmp").c_str());
    Global::cichar13 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\m.bmp").c_str());
    Global::cichar14 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\n.bmp").c_str());
    Global::cichar15 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\o.bmp").c_str());
    Global::cichar16 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\p.bmp").c_str());
    Global::cichar17 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\q.bmp").c_str());
    Global::cichar18 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\r.bmp").c_str());
    Global::cichar19 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\s.bmp").c_str());
    Global::cichar20 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\t.bmp").c_str());
    Global::cichar21 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\u.bmp").c_str());
    Global::cichar22 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\v.bmp").c_str());
    Global::cichar23 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\w.bmp").c_str());
    Global::cichar24 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\x.bmp").c_str());
    Global::cichar25 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\y.bmp").c_str());
    Global::cichar26 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\z.bmp").c_str());

    Global::cichar27 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\1.bmp").c_str());
    Global::cichar28 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\2.bmp").c_str());
    Global::cichar29 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\3.bmp").c_str());
    Global::cichar30 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\4.bmp").c_str());
    Global::cichar31 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\5.bmp").c_str());
    Global::cichar32 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\6.bmp").c_str());
    Global::cichar33 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\7.bmp").c_str());
    Global::cichar34 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\8.bmp").c_str());
    Global::cichar35 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\9.bmp").c_str());
    Global::cichar36 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\0.bmp").c_str());

    Global::cichar37 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\_.bmp").c_str());
    Global::cichar38 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\_.bmp").c_str());
    Global::cichar39 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\divide.bmp").c_str());
    Global::cichar40 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\exclamation.bmp").c_str());
    Global::cichar41 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\question.bmp").c_str());
    Global::cichar42 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\star.bmp").c_str());
    Global::cichar43 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\slash.bmp").c_str());
    Global::cichar44 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\bslash.bmp").c_str());
    Global::cichar45 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\percent.bmp").c_str());
    Global::cichar46 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\breacketco.bmp").c_str());
    Global::cichar47 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\breacketcc.bmp").c_str());
    Global::cichar48 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\point.bmp").c_str());
    Global::cichar49 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\-.bmp").c_str());
    Global::cichar50 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\+.bmp").c_str());
    Global::cichar51 = TTexturesManager::GetTextureID(AnsiString("data\\alpha1\\=.bmp").c_str());

    //Global::texmenu_backg  = logo;

    //~logo; Ra: to jest bez sensu zapis
    glColor3f(0.0f,0.0f,100.0f);
//    if(Global::detonatoryOK)
//    {
//    glRasterPos2f(-0.25f, -0.09f);
//    glPrint("Uruchamianie / Initializing...");
//    glRasterPos2f(-0.25f, -0.10f);
//    glPrint("Dzwiek / Sound...");
//    }
    SwapBuffers(hDC);					// Swap Buffers (Double Buffering)

    glEnable(GL_LIGHTING);

    LOADLOADERCONFIG();

/*-----------------------Sound Initialization---------------------------------*/

    RenderLoader(77, "SOUND INITIALIZATION...");
    WriteLog(" ");
    WriteLog(" ");
    WriteLog("SOUND INITIALIZATION ********************************* ");
    WriteLog(" ");


    TSoundsManager::Init(hWnd);

    
    WriteLog("Sound Init OK");
//    if(Global::detonatoryOK)
//    {
//    glRasterPos2f(-0.25f, -0.11f);
//    glPrint("OK.");
//    }
    SwapBuffers(hDC);					// Swap Buffers (Double Buffering)

    int i;

    Paused= true;

    RenderLoader(77, "TEXTURES INITIALIZATION...");
    WriteLog(" ");
    WriteLog(" ");
    WriteLog("TEXTURES INITIALIZATION ****************************** ");
    WriteLog(" ");

//    if(Global::detonatoryOK)
//    {
//    glRasterPos2f(-0.25f, -0.12f);
//    glPrint("Tekstury / Textures...");
//    }
    SwapBuffers(hDC);					// Swap Buffers (Double Buffering)

    TTexturesManager::Init();
    WriteLog("Textures init OK");


//    if(Global::detonatoryOK)
//    {
//    glRasterPos2f(-0.25f, -0.13f);
//    glPrint("OK.");
//    }
    SwapBuffers(hDC);					// Swap Buffers (Double Buffering)

    WriteLog("Models init");
//    if(Global::detonatoryOK)
//    {
//    glRasterPos2f(-0.25f, -0.14f);
//    glPrint("Modele / Models...");
//    }
//    SwapBuffers(hDC);					// Swap Buffers (Double Buffering)
//McZapkie: dodalem sciezke zeby mozna bylo definiowac skad brac modele ale to malo eleganckie
//    TModelsManager::LoadModels(asModelsPatch);

    RenderLoader(77, "MODELS INITIALIZATION...");
    WriteLog(" ");
    WriteLog(" ");
    WriteLog("MODELS INITIALIZATION ******************************** ");
    WriteLog(" ");

    TModelsManager::Init();
    WriteLog("Models init OK");
//    if (Global::detonatoryOK)
//    {
//    glRasterPos2f(-0.25f, -0.15f);
//    glPrint("OK.");
//    }
//    SwapBuffers(hDC);					// Swap Buffers (Double Buffering)

    RenderLoader(77, "GROUND INIT...");
    WriteLog(" ");
    WriteLog(" ");
    WriteLog("GROUND INIT ****************************************** ");
    WriteLog(" ");

//    if (Global::detonatoryOK)
//    {
//    glRasterPos2f(-0.25f, -0.16f);
//    glPrint("Sceneria / Scenery (can take a while)...");
//    }
//    SwapBuffers(hDC);					// Swap Buffers (Double Buffering)

    Ground.Init(Global::szSceneryFile);
    WriteLog(" ");
    WriteLog(" ");

    
    WriteLog(" ");
    WriteLog(" ");
    WriteLog("CLOUDS INIT ****************************************** ");
    WriteLog(" ");
    RenderLoader(77, AnsiString("Init sky..." ).c_str());
    Clouds.Init();
    if (Global::bsun) SUN.Init();
    WriteLog("Ground init OK");
//    if (Global::detonatoryOK)
//    {
//     glRasterPos2f(-0.25f, -0.17f);
//     glPrint("OK.");
//    }
//    SwapBuffers(hDC);					// Swap Buffers (Double Buffering)

//    TTrack *Track= Ground.FindGroundNode("train_start",TP_TRACK)->pTrack;

//    Camera.Init(vector3(2700,10,6500),0,M_PI,0);
//    Camera.Init(vector3(00,40,000),0,M_PI,0);
//    Camera.Init(vector3(1500,5,-4000),0,M_PI,0);
//McZapkie-130302 - coby nie przekompilowywac:
//      Camera.Init(Global::pFreeCameraInit,0,M_PI,0);
      Camera.Init(Global::pFreeCameraInit[0],Global::pFreeCameraInitAngle[0]);

    WriteLog(" ");
    WriteLog(" ");
    WriteLog("PLAYER TRAIN INITIALIZATION ************************** ");
    WriteLog(" ");

    RenderLoader(77, "Initialization player train...");
    char buff[255]= "Player train init: ";
//    if (Global::detonatoryOK)
//    {
//    glRasterPos2f(-0.25f, -0.18f);
//    glPrint("Inicjalizacja wybranego pojazdu do sterowania...");
//    }
//    SwapBuffers(hDC);					// Swap Buffers (Double Buffering)

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
//      if (Global::detonatoryOK)
//      {
//       glRasterPos2f(-0.25f, -0.19f);
//       glPrint("OK.");
//      }
//      SwapBuffers(hDC);					// Swap Buffers (Double Buffering)
     }
     else
     {
      Error("Player train init failed!");
      FreeFlyModeFlag=true; //Ra: automatycznie w³¹czone latanie
//      if (Global::detonatoryOK)
//      {
//       glRasterPos2f(-0.25f, -0.20f);
//       glPrint("Blad inicjalizacji sterowanego pojazdu!");
//      }
//      SwapBuffers(hDC);					// Swap Buffers (Double Buffering)
      Controlled=NULL;
      Camera.Type=tp_Free;
     }
    }
    else
    {
     if (Global::asHumanCtrlVehicle!=AnsiString("ghostview"))
      Error("Player train not exist!");
     FreeFlyModeFlag=true; //Ra: automatycznie w³¹czone latanie
//     if (Global::detonatoryOK)
//     {
//      glRasterPos2f(-0.25f, -0.20f);
//      glPrint("Wybrany pojazd nie istnieje w scenerii!");
//     }
//     SwapBuffers(hDC);					// Swap Buffers (Double Buffering)
     Controlled=NULL;
     Camera.Type=tp_Free;
    }
    glEnable(GL_DEPTH_TEST);
    //Ground.pTrain=Train;

    if (Global::bSnowEnabled) UstawPoczatek(Camera.Pos);  // SNOW

    LTS.setcablights(Train->DynamicObject->MoverParameters->TypeName, Train->DynamicObject->MoverParameters->CabNo);


    RenderLoader(77, "FINDING KEY EVENTS...");
    WriteLog(" ");
    WriteLog(" ");
    WriteLog("FINDING KEY EVENTS ******************************** ");
    WriteLog(" ");

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

    WriteLog("Finding events ok");
    //matrix4x4 ident2;
    //ident2.Identity();
    ~logo; //Ra: to jest bez sensu zapis
// glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);  //{Texture blends with object background}
    light=TTexturesManager::GetTextureID("smuga.tga");

    Global::bWriteLogEnabled = true;
    ResetTimers();
    WriteLog("Reset timers ok");
    WriteLog("Done.");

    setprogressfile();

    asENDLOAD = FormatDateTime("hh:mm:ss ", Now());

    Global::gtc2 = GetTickCount();

    Global::lsec = Global::gtc2 - Global::gtc1;


    RenderLoader(77, "Retrieving consist...");
    WriteLog("Retrieving consist...");
    if(Controlled) Controlled->GetConsist_f(1, Controlled);

    setdirecttable("                                                            ");


    LTS.loadlights();
    
//  WriteLog("Load time: "+AnsiString(0.1*floor(864000.0*((double)Now()-time)))+" seconds");
    WriteLog(AnsiString("Load time: " + asSTARTLOAD + " - " + asENDLOAD + " " + FormatFloat("0.000", (Global::lsec) / 1000) + "s").c_str());
    WriteLog(" ");
    WriteLog(" ");
    WriteLog(" ");
    WriteLog("START ----------------------------------------------------------------------------------------");
    RenderLoader(77, "READY!");

    SETALPHA();
    isSetPrioritySuccessful = SetPriorityClass( proces, NORMAL_PRIORITY_CLASS);

    //gluPerspective(45.0, (GLdouble)Global::iWindowWidth/(GLdouble)Global::iWindowHeight, 0.1f, 234566.0f);  //1999950600.0f
    delete Global::OTHERS;
    delete Global::CHANGES;
    ~Global::loaderlogo;
    ~Global::loaderbrief;
    ~Global::texmenu_backg;

 return true;
};


void __fastcall TWorld::OnKeyPress(int cKey)
{//(cKey) to kod klawisza, cyfrowe i literowe siê zgadzaj¹
 if (!Global::bPause)
 {//podczas pauzy klawisze nie dzia³aj¹
  AnsiString info="Key pressed: [";
  if (Pressed(VK_SHIFT)) info+="Shift]+[";
  if (Pressed(VK_CONTROL)) info+="Ctrl]+[";
  if (Pressed(VK_TAB)) info="[TAB]";

  if (Pressed(VK_CONTROL)) Global::iTextMode=0;
  if (Pressed(VK_CONTROL) && Pressed(VK_F6)) Global::bpanview = !Global::bpanview;
  if (Pressed(VK_CONTROL) && Pressed(VK_F7)) Global::bsemlenses = !Global::bsemlenses;
  if (Pressed(VK_CONTROL) && cKey == VkKeyScan('s')) SUN.LOADCONFIG();
  if (Pressed(VK_CONTROL) && cKey == VkKeyScan('l')) LTS.loadlights();
  if (Pressed(VK_CONTROL) && cKey == VkKeyScan('d')) DebugModeFlag = !DebugModeFlag;
  if (Pressed(VK_CONTROL) && cKey == VkKeyScan('h')) Global::bTUTORIAL = !Global::bTUTORIAL;
  if (Pressed(VK_CONTROL) && cKey == VkKeyScan('f')) Global::scrfilter = !Global::scrfilter;
  if (Pressed(VK_CONTROL) && cKey == VkKeyScan('1')) Global::infotype = 1;
  if (Pressed(VK_CONTROL) && cKey == VkKeyScan('2')) Global::infotype = 2;
  if (Pressed(VK_CONTROL) && cKey == VkKeyScan('3')) Global::infotype = 3;
  if (Pressed(VK_CONTROL) && cKey == VkKeyScan('4')) Global::infotype = 4;
  if (Pressed(VK_CONTROL) && cKey == VkKeyScan('5')) Global::infotype = 5;
  if (Pressed(VK_CONTROL) && cKey == VkKeyScan('6')) Global::infotype = 6;
  if (Pressed(VK_CONTROL) && cKey == VkKeyScan('7')) Global::infotype = 7;
  if (Pressed(VK_CONTROL) && cKey == VkKeyScan('8')) Global::infotype = 8;
  if (Pressed(VK_CONTROL) && cKey == VkKeyScan('9')) Global::infotype = 9;

  if ( Pressed(VK_F12)) MakeScreenShot(GL_HWND, GL_HDC, NULL);
  
  if (cKey==VkKeyScan('`')) {Global::bCONSOLE = !Global::bCONSOLE; TWorld::bCONSOLEMODE = Global::bCONSOLE;}
  if (Global::bCONSOLE && TWorld::cph > 990) TWorld::CONSOLE__addkey(cKey);
  if (Global::bCONSOLE && TWorld::cph > 990)  if (Pressed(VK_RETURN)) TWorld::CONSOLE__execute();
  if (Global::bCONSOLE && cKey==Global::Keys[k_MechForward]) CONSOLE__fastnextcmd();
  if (Global::bCONSOLE && cKey==Global::Keys[k_MechBackward]) CONSOLE__fastprevcmd();
  if (Global::bCONSOLE) return;  

  if (Pressed(VK_CONTROL) && cKey == VkKeyScan('=')) GlobalTime->UpdateMTableTime(60);    //Global::iViewMode=VK_F6;
  if (Pressed(VK_CONTROL) && cKey == VkKeyScan('-')) GlobalTime->UpdateMTableTime(5);
  if (Pressed(VK_CONTROL) && !Pressed(VK_SHIFT) && cKey == VkKeyScan('z')) {Controlled->bcab1light = !Controlled->bcab1light; }
  if (Pressed(VK_CONTROL) && !Pressed(VK_SHIFT) && cKey == VkKeyScan('c')) if (Global::cablightint > 0.01){Global::cablightint -= 0.05; LTS.cablight_0();}
  if (Pressed(VK_CONTROL) && !Pressed(VK_SHIFT) && cKey == VkKeyScan('v')) if (Global::cablightint < 1.00){Global::cablightint += 0.05; LTS.cablight_0();}
  if (Pressed(VK_CONTROL) &&  Pressed(VK_SHIFT) && cKey == VkKeyScan('c')) Global::ventilator1on = !Global::ventilator1on;
  if (Pressed(VK_CONTROL) &&  Pressed(VK_SHIFT) && cKey == VkKeyScan('v')) Global::ventilator2on = !Global::ventilator2on;

  if (Pressed(VK_CONTROL)) return;

  if (!Pressed(VK_CONTROL) && Pressed(VK_BACK)) if (Global::infotype > 0) Global::infotype--;
  if (!Pressed(VK_CONTROL) && Pressed(VK_TAB))  Global::infotype++; if (Global::infotype ==  10) Global::infotype = 0;   // ZMIANA TYPU WYSWIETLANYCH INFORMACJI


//  if ( Pressed(VK_F8)) {OnCmd("changelok", Global::asnearestdyn , "FF", "none", "none", "none");  cdyn = true;}


  if (info == "[TAB]" && Global::infotype == 7) Global::asKeyboardFile =  Global::g___APPDIR + "DATA\\KEYBOARDS\\" + Controlled->MoverParameters->TypeName + ".txt";
  if (info == "[TAB]" && Global::infotype == 7) WriteLog(Global::asKeyboardFile);
  if (info == "[TAB]" && Global::infotype == 7) if(!FEX(Global::asKeyboardFile))Global::KEYBOARD->Clear();
  if (info == "[TAB]" && Global::infotype == 7) if(FEX(Global::asKeyboardFile)) Global::KEYBOARD->LoadFromFile(Global::asKeyboardFile);


  
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
  Global::bpanview = 0;
  Global::scrfilter = 0;
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

     if (Controlled) Controlled->GetConsist_f(1, Controlled);                    // POBIERANIE SKLADU
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

void __fastcall TWorld::OnMouseWheel(int zDelta)
{
   if (zDelta == -120) if (Pressed(VK_CONTROL) ) SCR->FOVADD();//{ if (Global::panviewtrans > 0.1) Global::panviewtrans -= 0.01; }                               // OBROTNICA -
   if (zDelta ==  120) if (Pressed(VK_CONTROL) ) SCR->FOVREM();//{ if (Global::panviewtrans < 1.1) Global::panviewtrans += 0.01; }                               // OBROTNICA +
}

void __fastcall TWorld::OnMouseLpush(double x, double y)
{
 WriteLog("MOUSE L DOWN");
}

void __fastcall TWorld::OnMouseRpush(double x, double y)
{
 WriteLog("MOUSE R DOWN");
}

void __fastcall TWorld::OnMouseMpush(double x, double y)
{
 WriteLog("MOUSE M DOWN");
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

bool __fastcall SHOWTIME()
{
     AnsiString ctime;
     ctime =AnsiString((int)GlobalTime->hh)+":";
     int i=GlobalTime->mm; //bo inaczej potrafi zrobiæ "hh:010"
     if (i<10) ctime+="0";
     ctime+=AnsiString(i); //minuty
     ctime+=":";
     i=floor(GlobalTime->mr); //bo inaczej potrafi zrobiæ "hh:mm:010"
     if (i<10) ctime+="0";
     ctime+=AnsiString(i);
    Global::gtime = ctime;
}

bool __fastcall TWorld::Update()
{

if (Global::bSCREENSHOT)
 {
  SCRMESSAGE++;
  Global::infotype = 66;
  if (SCRMESSAGE>500)  {Global::bSCREENSHOT = false; SCRMESSAGE = 0; Global::infotype = 0;}
 }

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
 Global::fps = GetFPS();
 
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
   //lp=Normalize(lp); //przeliczenie na wektor d³ugoœci 1.0
   Global::lightPos[0]=(float)lp.x*1000000;
   Global::lightPos[1]=(float)lp.y*1000000;
   Global::lightPos[2]=(float)lp.z*1000000;

 //Global::SUNFLPos[0]=(float)lp.x*1500;
 //Global::SUNFLPos[1]=(float)lp.y*1500;
 //Global::SUNFLPos[2]=(float)lp.z*1500;

   glLightfv(GL_LIGHT0,GL_POSITION,Global::lightPos);        //daylight position
   if (H>0)
   {//s³oñce ponad horyzontem
    Global::ambientDayLight[0]=Global::ambientLight[0];
    Global::ambientDayLight[1]=Global::ambientLight[1];
    Global::ambientDayLight[2]=Global::ambientLight[2];

    Global::specularDayLight[0]=Global::specularLight[0]; //podobnie specular
    Global::specularDayLight[1]=Global::specularLight[1];
    Global::specularDayLight[2]=Global::specularLight[2];

    if (H>0.02)
    {
     Global::diffuseDayLight[0]=Global::diffuseLight[0]; //od wschodu do zachodu maksimum ???
     Global::diffuseDayLight[1]=Global::diffuseLight[1];
     Global::diffuseDayLight[2]=Global::diffuseLight[2];
     Global::specularDayLight[0]=Global::specularLight[0]; //podobnie specular
     Global::specularDayLight[1]=Global::specularLight[1];
     Global::specularDayLight[2]=Global::specularLight[2];
    }
    else
    {
     Global::diffuseDayLight[0]=50*H*Global::diffuseLight[0]; //wschód albo zachód
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
    if (!Pressed(VK_CONTROL) ) Global::ffov = 45.0;
    if (!Pressed(VK_CONTROL) ) SCR->ReSizeGLSceneEx(Global::ffov, Global::iWindowWidth, Global::iWindowHeight);
    if (!Pressed(VK_CONTROL) ) Camera.Reset();
    if ( Pressed(VK_CONTROL) ) SCR->FOVADD();
  }
  if (Pressed(VK_RBUTTON))
  {
    if (Pressed(VK_CONTROL) ) SCR->FOVREM();
  }
  if (Pressed(VK_MBUTTON))
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
  else if (Pressed(VK_RBUTTON) || Pressed(VK_F4))
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

     Camera.Reset();
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
//  if (Controlled) Controlled->ABuSetModelShake(temp);        // BO SIE WSTAWIANE PRZEDMIOTY WALALY PO KABINIE :)
  //ABu: koniec rzucania

  if (Train->DynamicObject->MoverParameters->ActiveCab==0)
   Camera.LookAt=Train->pMechPosition+Train->GetDirection();
  else  //patrz w strone wlasciwej kabiny
   Camera.LookAt=Train->pMechPosition+Train->GetDirection()*Train->DynamicObject->MoverParameters->ActiveCab; //-1 albo 1
  Camera.vUp=Train->GetUp();
 }
 Ground.CheckQuery();

bysec += dt;
if (bysec > 1.0)   // DAC CO SEKUNDE COBY WIEKSZY FPS BYL...
    {
    if (GlobalTime->s1  ==  6) GlobalTime->s1=0;       //M
    if (GlobalTime->s2  ==  0) GlobalTime->s2=0;       //M

    Global::cdigit01 = settexturefromid(GlobalTime->h1);       // H
    Global::cdigit02 = settexturefromid(GlobalTime->h2);       // H
    Global::cdigit03 = Global::charx_01;                       // :
    Global::cdigit04 = settexturefromid(GlobalTime->m1);       // M
    Global::cdigit05 = settexturefromid(GlobalTime->m2);       // M
    Global::cdigit06 = Global::charx_01;                       // :
    Global::cdigit07 = settexturefromid(GlobalTime->s1);       // M
    Global::cdigit08 = settexturefromid(GlobalTime->s2);       // M

    if (GlobalTime->mm == 59) Global::cdigit01 = settexturefromid(GlobalTime->h1);       //H
    if (GlobalTime->mm == 59) Global::cdigit02 = settexturefromid(GlobalTime->h2);       //H
    if (GlobalTime->ss  <  2) Global::cdigit04 = settexturefromid(GlobalTime->m1);       //M
    if (GlobalTime->ss  <  2) Global::cdigit05 = settexturefromid(GlobalTime->m2);       //M

    bysec = 0;
  }

         
 if (!Render()) return false;

//**********************************************************************************************************

// TU BYLO RENDEROWANIE KABINY - PRZENIESIONE DO FUNKCJI RenderCab();

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
       GlobalTime->UpdateMTableTime(50);
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
    
// -----------------------------------------------------------------------------
// PRZECHODZENIE DO INNEGO POJAZDU ---------------------------------------------
// -----------------------------------------------------------------------------
    if (Global::changeDynObj==false && cdyn==true && Global::asnearestdyn != "x0" && FreeFlyModeFlag == true)
    {
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

       WriteLog("Chaning vechicle...");
       TGroundNode *tmp;
       TDynamicObject *DYNOBJ;
       tmp = Ground.FindDynamic(Global::asnearestdyn);
       if (tmp) DYNOBJ= tmp->DynamicObject;



       Train->DynamicObject->Controller=AIdriver;
       Train->DynamicObject->bDisplayCab=false;
       Train->DynamicObject->MechInside=false;
       Train->DynamicObject->MoverParameters->SecuritySystem.Status=0;
       Train->DynamicObject->ABuSetModelShake(vector3(0,0,0));
       Train->DynamicObject->MoverParameters->ActiveCab=0;
       Train->DynamicObject->MoverParameters->BrakeCtrlPos=-2;


       Train->DynamicObject=temp;
       if (cdyn) Train->DynamicObject=DYNOBJ;

       Controlled=Train->DynamicObject;
       Global::asHumanCtrlVehicle=Train->DynamicObject->GetName();

       Train->DynamicObject->MoverParameters->LimPipePress=Controlled->MoverParameters->PipePress;
       Train->DynamicObject->MoverParameters->SecuritySystem.Status=1;
       Train->DynamicObject->MoverParameters->ActiveCab=CabNr;
       Train->DynamicObject->MoverParameters->CabDeactivisation();
       Train->DynamicObject->Controller=Humandriver;
       Train->DynamicObject->MechInside=true;
///       Train->DynamicObject->Mechanik=new TController(l,r,Controlled->Controller,&Controlled->MoverParameters,&Controlled->TrainParams,Aggressive);
       //Train->InitializeCab(Train->DynamicObject->MoverParameters->CabNo,Train->DynamicObject->asBaseDir+Train->DynamicObject->MoverParameters->TypeName+".mmd");

       if (!cdyn) Train->InitializeCab(CabNr,Train->DynamicObject->asBaseDir+Train->DynamicObject->MoverParameters->TypeName+".mmd");
       if (!FreeFlyModeFlag) Train->DynamicObject->bDisplayCab=true;


       Train->DynamicObject->MoverParameters->ActiveCab=1;
       Train->DynamicObject->MoverParameters->CabDeactivisation();
       Train->DynamicObject->MoverParameters->ActiveCab=-1;
       Train->DynamicObject->MoverParameters->CabDeactivisation();

       if (cdyn) Train->InitializeCab(CabNr,Train->DynamicObject->asBaseDir+Train->DynamicObject->MoverParameters->TypeName+".mmd");
       if (cdyn) Train->DynamicObject->MoverParameters->CabActivisation();
       if (cdyn) Camera.LookAt=Train->pMechPosition+Train->GetDirection()*Train->DynamicObject->MoverParameters->ActiveCab;
       if (cdyn) Camera.Pos=Train->pMechPosition;//Train.GetPosition1();
       if (cdyn) Camera.Roll=atan(Train->pMechShake.x*Train->fMechRoll);        //hustanie kamery na boki
       if (cdyn) Camera.Pitch-=atan(Train->vMechVelocity.z*Train->fMechPitch);  //hustanie kamery przod tyl
       
       if (cdyn) FreeFlyModeFlag = false;
       if (cdyn) Train->DynamicObject->bDisplayCab = true;
       if (cdyn) if (Train->DynamicObject->MoverParameters->ActiveCab==0) Camera.LookAt=Train->pMechPosition+Train->GetDirection(); else Camera.LookAt=Train->pMechPosition+Train->GetDirection()*Train->DynamicObject->MoverParameters->ActiveCab;
       if (cdyn) Train->pMechOffset.x=Train->pMechSittingPosition.x;
       if (cdyn) Train->pMechOffset.y=Train->pMechSittingPosition.y;
       if (cdyn) Train->pMechOffset.z=Train->pMechSittingPosition.z;


       Camera.Reset();

       Global::changeDynObj=false;

       cdyn = false;
    }



     SHOWTIME();

    if (Global::iTextMode==VK_F1)
    {//tekst pokazywany po wciœniêciu [F1]
     //Global::iViewMode=VK_F1;
    OutText1 = "Time: " + Global::gtime;

     if (Global::bPause) OutText1 += " - paused";
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
      if (tmp->MoverParameters->BrakeDelayFlag>0)
       if (tmp->MoverParameters->BrakeDelayFlag>1)
        OutText2+="R";
       else
       OutText2+="G";
      else
       OutText2+="P";
      OutText2+=AnsiString(", BTP:")+FloatToStrF(tmp->MoverParameters->BCMFlag,ffFixed,5,0);
//      OutText2+= FloatToStrF(tmp->MoverParameters->CompressorPower,ffFixed,5,0)+AnsiString(", ");
//yB      if(tmp->MoverParameters->BrakeSubsystem==Knorr) OutText2+=" Knorr";
//yB      if(tmp->MoverParameters->BrakeSubsystem==Oerlikon) OutText2+=" Oerlikon";
//yB      if(tmp->MoverParameters->BrakeSubsystem==Hik) OutText2+=" Hik";
//yB      if(tmp->MoverParameters->BrakeSubsystem==WeLu) OutText2+=" £estingha³s";
      //OutText2= " GetFirst: "+AnsiString(tmp->GetFirstDynamic(1)->MoverParameters->Name)+" Damage status="+tmp->MoverParameters->EngineDescription(0)+" Engine status: ";
      //OutText2+= " GetLast: "+AnsiString(tmp->GetLastDynamic(1)->MoverParameters->Name)+" Damage status="+tmp->MoverParameters->EngineDescription(0)+" Engine status: ";
      OutText3= AnsiString("Brake press: ")+FloatToStrF(tmp->MoverParameters->BrakePress,ffFixed,5,2)+AnsiString(", ");
      OutText3+= AnsiString("Pipe press: ")+FloatToStrF(tmp->MoverParameters->PipePress,ffFixed,5,2)+AnsiString(", ");
      OutText3+= AnsiString("BVP: ")+FloatToStrF(tmp->MoverParameters->BrakeVP(),ffFixed,5,2)+AnsiString(", ");
      OutText3+= FloatToStrF(tmp->MoverParameters->CntrlPipePress,ffFixed,5,2)+AnsiString(", ");
//      OutText3+= FloatToStrF(tmp->MoverParameters->HighPipePress,ffFixed,5,2)+AnsiString(", ");
//      OutText3+= FloatToStrF(tmp->MoverParameters->LowPipePress,ffFixed,5,2)+AnsiString(", ");
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
// if (Controlled)
//  SetWindowText(hWnd,AnsiString(Controlled->MoverParameters->Name).c_str());
// else
//  SetWindowText(hWnd,Global::szSceneryFile); //nazwa scenerii
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


//**********************************************************************************************************
//rendering kabiny gdy jest oddzielnym modelem i ma byc wyswietlana

bool __fastcall TWorld::RenderCab()
{

    // CABLIGHT
    LTS.setcablights(Train->DynamicObject->MoverParameters->TypeName, Train->DynamicObject->MoverParameters->CabNo);
    if (Controlled->bcab1light) glEnable(GL_LIGHT2); else glDisable(GL_LIGHT2);         // CAB LIGHT

//    glDisable(GL_LIGHT1);
//    glDisable(GL_LIGHT3);
//    glDisable(GL_LIGHT4);

 if (Train)
 {//rendering kabiny gdy jest oddzielnym modelem i ma byc wyswietlana
//  vector3 vFront=Train->DynamicObject->GetDirection();
//  if ((Train->DynamicObject->MoverParameters->CategoryFlag&2) && (Train->DynamicObject->MoverParameters->ActiveCab<0)) //TODO: zrobic to eleganciej z plynnym zawracaniem
//     vFront=-vFront;
//  vector3 vUp=vWorldUp; //sta³a
//  vFront.Normalize();
//  vector3 vLeft=CrossProduct(vUp,vFront);
//  vUp= CrossProduct(vFront,vLeft);
//  matrix4x4 mat;
//  mat.Identity();
//  mat.BasisChange(vLeft,vUp,vFront);
//  Train->DynamicObject->mMatrix= Inverse(mat);

  glPushMatrix();

  vector3 pos=Train->DynamicObject->GetPosition();
  glTranslatef(pos.x,pos.y,pos.z);
  glMultMatrixd(Train->DynamicObject->mMatrix.getArray());

  if (Global::fLuminance<=0.25)
   {
//    glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_ONE);            // ok
//    glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_DST_COLOR);
//    glBlendFunc(GL_SRC_ALPHA_SATURATE,GL_ONE);
//    glDisable(GL_DEPTH_TEST);
    if (FreeFlyModeFlag) glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glColor4f(1.0f,1.0f,1.0f,1.0f);
//    glBindTexture(GL_TEXTURE_2D, light);       // Select our texture


//  glEnable(GL_BLEND); //Enable blending so that the Light Texture Blends with the Cube and Room we rendered earlier
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);    // COBY EFEKT SWIATEL BYL LEPSZY
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);


  glBindTexture(GL_TEXTURE_2D, LIGHTC);

//  glDisable(GL_BLEND);//Disable the blend so we can render other poly's normally
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_DEPTH_TEST);
    
 if ( FreeFlyModeFlag ) glEnable(GL_DEPTH_TEST);

// POJEDYNCZE REFLEKTORY

    glColor4f(0.4, 0.3, 0.2, 0.4);
    // PRAWE
    glPushMatrix();
     if (Train->DynamicObject->headlA_R==true)
     {
      glTranslatef(-1.2,0.1,0);
      glBegin(GL_QUADS);
      glTexCoord2f(0,0); glVertex3f(  3.0, 0.0,  15.0);
      glTexCoord2f(1,0); glVertex3f( -3.0, 0.0,  15.0);
      glTexCoord2f(1,1); glVertex3f( -7.0, 2.5, 250.0); // do zewnatrz na koncu
      glTexCoord2f(0,1); glVertex3f( 12.0, 2.5, 250.0);  // do srodka
      glEnd();
     }
     if (Train->DynamicObject->headlB_R==true)
     {
      glTranslatef(1.2,0.2,0);
      glBegin(GL_QUADS);
      glTexCoord2f(0,0); glVertex3f( -3.0,0.0, -15.0);
      glTexCoord2f(1,0); glVertex3f(  3.0,0.0, -15.0);
      glTexCoord2f(1,1); glVertex3f(  7.0,2.5,-250.0);
      glTexCoord2f(0,1); glVertex3f(-12.0,2.5,-250.0);   // do srodka na koncu
      glEnd();
     }
    glPopMatrix ( );

     // LEWE
    glPushMatrix();
     if (Train->DynamicObject->headlA_L==true)
     {
      glTranslatef(1.2,0.2,0);
      glBegin(GL_QUADS);
      glTexCoord2f(0,0); glVertex3f(  3.0,0.0,  15.0);
      glTexCoord2f(1,0); glVertex3f( -3.0,0.0,  15.0);
      glTexCoord2f(1,1); glVertex3f( -0.0,2.5, 250.0);
      glTexCoord2f(0,1); glVertex3f( 12.0,2.5, 250.0);
      glEnd();
     }
     if (Train->DynamicObject->headlB_L==true)
     {
      glTranslatef(-1.2,0.1,0);
      glBegin(GL_QUADS);
      glTexCoord2f(0,0); glVertex3f( -3.0,0.0, -15.0);
      glTexCoord2f(1,0); glVertex3f( 3,0.0, -15.0);
      glTexCoord2f(1,1); glVertex3f( 0.0,2.5,-250.0);
      glTexCoord2f(0,1); glVertex3f(-12.0,2.5,-250.0);
      glEnd();
     }
  glPopMatrix ( );

  // GORNE
    glPushMatrix();
     if (Train->DynamicObject->headlA_U==true)
     {
      glTranslatef(0.0,0.3,0);
      glBegin(GL_QUADS);
      glTexCoord2f(0,0); glVertex3f( 3.0,0.0,  20.0);
      glTexCoord2f(1,0); glVertex3f(-3.0,0.0,  20.0);
      glTexCoord2f(1,1); glVertex3f(-4.0,2.6, 270.0);
      glTexCoord2f(0,1); glVertex3f( 7.0,2.6, 270.0);
      glEnd();
     }
     if (Train->DynamicObject->headlB_U==true)
     {
      glTranslatef(0.0,0.3,0);
      glBegin(GL_QUADS);
      glTexCoord2f(0,0); glVertex3f(-3.0,0.0, -15.0);
      glTexCoord2f(1,0); glVertex3f( 3.0,0.0, -15.0);
      glTexCoord2f(1,1); glVertex3f( 4.0,2.5,-250.0);
      glTexCoord2f(0,1); glVertex3f(-7.0,2.5,-250.0);
      glEnd();
     }
  glPopMatrix ( );


// POSWIATA ZA SZYBA

  if (!FreeFlyModeFlag)      // TYLKO Z WIDOKU Z KABINY
     {
     if (Train->DynamicObject->headlA_U==true)
     {
      glColor4f(0.4, 0.35, 0.3, 0.3);
      glPushMatrix();
      glTranslatef(0,1.2,0);
      glRotatef(180,1,0,0);

      glBegin(GL_QUADS);
      glTexCoord2f(0, 0);  glVertex3f(-3.5,-3.5, -16);
      glTexCoord2f(1, 0);  glVertex3f( 3.5,-3.5, -16);
      glTexCoord2f(1, 1);  glVertex3f( 3.5, 1.5, -16);
      glTexCoord2f(0, 1);  glVertex3f(-3.5, 1.5, -16);
      glEnd();

      glPopMatrix ( );
      }
     if (Train->DynamicObject->headlA_R==true)
     {
      glColor4f(0.4, 0.35, 0.3, 0.3);
      glPushMatrix();
      glTranslatef(-1.6,1.2,0);
      glRotatef(180,1,0,0);

      glBegin(GL_QUADS);
      glTexCoord2f(0, 0);  glVertex3f(-3.5,-3.5, -15.1);
      glTexCoord2f(1, 0);  glVertex3f( 3.5,-3.5, -15.1);
      glTexCoord2f(1, 1);  glVertex3f( 3.5, 1.5, -15.1);
      glTexCoord2f(0, 1);  glVertex3f(-3.5, 1.5, -15.1);
      glEnd();

      glPopMatrix ( );
      }
     if (Train->DynamicObject->headlA_L==true)
     {
      glColor4f(0.4, 0.35, 0.3, 0.3);
      glPushMatrix();
      glTranslatef(1.4,1.2,0);
      glRotatef(180,1,0,0);

      glBegin(GL_QUADS);
      glTexCoord2f(0, 0);  glVertex3f(-3.5,-3.5, -15);
      glTexCoord2f(1, 0);  glVertex3f( 3.5,-3.5, -15);
      glTexCoord2f(1, 1);  glVertex3f( 3.5, 1.5, -15);
      glTexCoord2f(0, 1);  glVertex3f(-3.5, 1.5, -15);
      glEnd();

      glPopMatrix ( );
      }

     if (Train->DynamicObject->headlB_U==true)
     {
      glColor4f(0.4, 0.35, 0.3, 0.3);
      glFrontFace(GL_CW);
      glPushMatrix();
      glTranslatef(0,1.2,0);
      glRotatef(180,1,0,0);

      glBegin(GL_QUADS);
      glTexCoord2f(0, 0);  glVertex3f(-3.5,-3.5, 16);
      glTexCoord2f(1, 0);  glVertex3f( 3.5,-3.5, 16);
      glTexCoord2f(1, 1);  glVertex3f( 3.5, 1.5, 16);
      glTexCoord2f(0, 1);  glVertex3f(-3.5, 1.5, 16);
      glEnd();

      glPopMatrix ( );
      glFrontFace(GL_CCW);
      }
     }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_FOG);
   }

   glDepthFunc(GL_LEQUAL); 

//yB: moje smuuugi 1 - koniec*/

  //ABu: Rendering kabiny jako ostatniej, zeby bylo widac przez szyby, tylko w widoku ze srodka
  if ((Train->DynamicObject->mdKabina!=Train->DynamicObject->mdModel) && Train->DynamicObject->bDisplayCab && !FreeFlyModeFlag)
  {


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

      // SWIATLO W KABINIE ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

      //if (((Train->DynamicObject->MoverParameters->CabNo == 1)) && (Global::bCabALight)) LTS.setcablightA(0);      // ZAHASHOWANE DAJE SWIATLO W CALEJ KABINIE
      //if (((Train->DynamicObject->MoverParameters->CabNo == -1)) && (Global::bCabALight)) LTS.setcablightA(0);     // ZAHASHOWANE DAJE SWIATLO W CALEJ KABINIE

      if (!Controlled->bcab1light) glDisable(GL_LIGHT2);
      if (Controlled->bcab1light) glEnable(GL_LIGHT2);

      // SETALPHA();

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

     glDisable(GL_LIGHT1);
     glDisable(GL_LIGHT2);



    //Global::light3Pos[0]= pos.x;
    //Global::light3Pos[1]= pos.y;
    //Global::light3Pos[2]= pos.z;
    //Global::light3Pos[3]= 0.0f;

    // glLightfv(GL_LIGHT3, GL_POSITION, Global::light3Pos);

 } //koniec if (Train)

//           LTS.setheadlightL(1, Train->DynamicObject->vLIGHT_R_POS, Train->DynamicObject->vLIGHT_R_END);

}

bool __fastcall TWorld::RenderX()
{
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
     if (!Ground.RaRenderAlpha(Camera.Pos)) return false;
    }
    else
#endif
    {//renderowanie przez Display List
     if (!Ground.Render(Camera.Pos)) return false;

     if (!Ground.RenderAlpha(Camera.Pos)) return false;
    }
    TSubModel::iInstance=(int)(Train?Train->DynamicObject:0); //¿eby nie robiæ cudzych animacji

    if (Train) Train->Update();

}

bool __fastcall TWorld::Render()
{
    double dt= GetDeltaTime();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(TSCREEN::CFOV, (GLdouble)Global::iWindowWidth/(GLdouble)Global::iWindowHeight, 0.1f, 13234566.0f);  //1999950600.0f
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    Camera.SetMatrix();

    glLightfv(GL_LIGHT0,GL_POSITION,Global::lightPos);


    if (Global::bRenderSky)  Clouds.RenderA();

    if (Global::bsun) if (Global::lightPos[1] > -400000) SUN.Render_A(dt);

    RenderX();


    //if (Global::bSnowEnabled) draw_snow(0, 0, Camera.Pos);
    SETALPHA();


    if (Controlled) RenderCab();

    if (Global::iTextMode == VK_F10) RenderEXITQUERY(0.40f);

    if (Global::scrfilter) RenderFILTER(0.20f);
    
    if (Global::bpanview) renderpanview(0.90, 100, 2);

    if (Global::infotype)  RenderINFOX(Global::infotype);

    if (Global::bTUTORIAL) ShowHints();

    if (Global::bCONSOLE) RenderConsole(50, dt);

    SETALPHA();
    glFlush();
    //Global::bReCompile=false; //Ra: ju¿ zrobiona rekompilacja

    ResourceManager::Sweep(Timer::GetSimulationTime());

    return true;
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

