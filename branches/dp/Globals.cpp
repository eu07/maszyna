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
#pragma hdrstop


#include "Globals.h"
#include "QueryParserComp.hpp"
#include "usefull.h"
#include "Mover.h"
#include "Driver.h"
#include "Console.h"
#include <Controls.hpp> //do odczytu daty
#include "World.h"


//namespace Global {

//parametry do u¿ytku wewnêtrznego
//double Global::tSinceStart=0;
TGround *Global::pGround=NULL;
//char Global::CreatorName1[30]="Maciej Czapkiewicz";
//char Global::CreatorName2[30]="Marcin Wozniak <Marcin_EU>";
//char Global::CreatorName3[20]="Adam Bugiel <ABu>";
//char Global::CreatorName4[30]="Arkadiusz Slusarczyk <Winger>";
//char Global::CreatorName5[30]="Lukasz Kirchner <Nbmx>";
AnsiString Global::asCurrentSceneryPath="scenery/";
AnsiString Global::asCurrentTexturePath=AnsiString(szDefaultTexturePath);
AnsiString Global::asCurrentDynamicPath="";
int Global::iSlowMotion=0; //info o malym FPS: 0-OK, 1-wy³¹czyæ multisampling, 3-promieñ 1.5km, 7-1km
bool Global::changeDynObj; //info o zmianie pojazdu
bool Global::detonatoryOK; //info o nowych detonatorach
double Global::ABuDebug=0;
AnsiString Global::asSky="1";
double Global::fOpenGL=0.0; //wersja OpenGL - do sprawdzania obecnoœci rozszerzeñ
bool Global::bOpenGL_1_5=false; //czy s¹ dostêpne funkcje OpenGL 1.5
double Global::fLuminance=1.0; //jasnoœæ œwiat³a do automatycznego zapalania
int Global::iReCompile=0; //zwiêkszany, gdy trzeba odœwie¿yæ siatki
HWND Global::hWnd=NULL; //uchwyt okna
int Global::iCameraLast=-1;
AnsiString Global::asRelease="1.7.563.345";
AnsiString Global::asVersion="Compilation 2012-04-24, release "+Global::asRelease+"."; //tutaj, bo wysy³any
int Global::iViewMode=0; //co aktualnie widaæ: 0-kabina, 1-latanie, 2-sprzêgi, 3-dokumenty
int Global::iTextMode=0; //tryb pracy wyœwietlacza tekstowego
double Global::fSunDeclination=0.0; //deklinacja S³oñca
double Global::fTimeAngleDeg=0.0; //godzina w postaci k¹ta
char* Global::szTexturesTGA[4]={"tga","dds","tex","bmp"}; //lista tekstur od TGA
char* Global::szTexturesDDS[4]={"dds","tga","tex","bmp"}; //lista tekstur od DDS
int Global::iKeyLast=0; //ostatnio naciœniêty klawisz w celu logowania
GLuint Global::iTextureId=0; //ostatnio u¿yta tekstura 2D
bool Global::bPause=false; //globalna pauza ruchu
bool Global::bActive=true; //czy jest aktywnym oknem
int Global::iErorrCounter=0; //licznik sprawdzañ do œledzenia b³êdów OpenGL
int Global::iTextures=0; //licznik u¿ytych tekstur
TWorld* Global::pWorld=NULL;
Queryparsercomp::TQueryParserComp *Global::qParser=NULL;
cParser *Global::pParser=NULL;
int Global::iSegmentsRendered=90; //iloœæ segmentów do regulacji wydajnoœci
TCamera* Global::pCamera=NULL; //parametry kamery
TDynamicObject *Global::pUserDynamic=NULL; //pojazd u¿ytkownika, renderowany bez trzêsienia

//parametry scenerii
vector3 Global::pCameraPosition;
double Global::pCameraRotation;
double Global::pCameraRotationDeg;
vector3 Global::pFreeCameraInit[10];
vector3 Global::pFreeCameraInitAngle[10];
double Global::fFogStart=1700;
double Global::fFogEnd=2000;
GLfloat Global::AtmoColor[]={0.423f,0.702f,1.0f};
GLfloat Global::FogColor[]={0.6f,0.7f,0.8f};
GLfloat Global::ambientDayLight[] ={0.40f,0.40f,0.45f,1.0f}; //robocze
GLfloat Global::diffuseDayLight[] ={0.55f,0.54f,0.50f,1.0f};
GLfloat Global::specularDayLight[]={0.95f,0.94f,0.90f,1.0f};
GLfloat Global::ambientLight[]    ={0.80f,0.80f,0.85f,1.0f}; //sta³e
GLfloat Global::diffuseLight[]    ={0.85f,0.85f,0.80f,1.0f};
GLfloat Global::specularLight[]   ={0.95f,0.94f,0.90f,1.0f};
GLfloat Global::whiteLight[]      ={1.00f,1.00f,1.00f,1.0f};
GLfloat Global::noLight[]         ={0.00f,0.00f,0.00f,1.0f};
GLfloat Global::darkLight[]       ={0.03f,0.03f,0.03f,1.0f}; //œladowe
GLfloat Global::lightPos[4];
bool Global::bRollFix=true; //czy wykonaæ przeliczanie przechy³ki

//parametry u¿ytkowe (jak komu pasuje)
int Global::Keys[MaxKeys];
int Global::iWindowWidth=800;
int Global::iWindowHeight=600;
int Global::iBpp=32;
int Global::iFeedbackMode=1; //tryb pracy informacji zwrotnej
bool Global::bFreeFly=false;
bool Global::bFullScreen=false;
bool Global::bInactivePause=true; //automatyczna pauza, gdy okno nieaktywne
float Global::fMouseXScale=3.2;
float Global::fMouseYScale=0.5;
char Global::szSceneryFile[256]="td.scn";
AnsiString Global::asHumanCtrlVehicle="EU07-424";
int Global::iMultiplayer=0; //blokada dzia³ania niektórych funkcji na rzecz kominikacji
double Global::fMoveLight=-1; //ruchome œwiat³o
double Global::fLatitudeDeg=52.0; //szerokoœæ geograficzna
double Global::fRadiusLoFPS=16.0; //dolna granica FPS, przy której promieñ scenerii bêdzie zmniejszany
double Global::fRadiusHiFPS=25.0; //górna granica FPS, przy której promieñ scenerii bêdzie zwiêkszany
double Global::fRadiusFactor=1.1; //wspó³czynnik jednorazowej zmiany promienia scenerii


//parametry wydajnoœciowe (np. regulacja FPS, szybkoœæ wczytywania)
bool Global::bAdjustScreenFreq=true;
bool Global::bEnableTraction=true;
bool Global::bLoadTraction=true;
bool Global::bLiveTraction=true;
int Global::iDefaultFiltering=9; //domyœlne rozmywanie tekstur TGA bez alfa
int Global::iBallastFiltering=9; //domyœlne rozmywanie tekstur podsypki
int Global::iRailProFiltering=5; //domyœlne rozmywanie tekstur szyn
int Global::iDynamicFiltering=5; //domyœlne rozmywanie tekstur pojazdów
bool Global::bUseVBO=false; //czy jest VBO w karcie graficznej (czy u¿yæ)
GLint Global::iMaxTextureSize=16384; //maksymalny rozmiar tekstury
bool Global::bSmoothTraction=false; //wyg³adzanie drutów starym sposobem
char** Global::szDefaultExt=Global::szTexturesDDS; //domyœlnie od DDS
int Global::iMultisampling=2; //tryb antyaliasingu: 0=brak,1=2px,2=4px,3=8px,4=16px
bool Global::bGlutFont=false; //czy tekst generowany przez GLUT32.DLL
int Global::iConvertModels=2; //tworzenie plików binarnych, 2-optymalizacja transformów
int Global::iSlowMotionMask=-1; //maska wy³¹czanych w³aœciwoœci dla zwiêkszenia FPS
int Global::iModifyTGA=7; //czy korygowaæ pliki TGA dla szybszego wczytywania
//bool Global::bTerrainCompact=true; //czy zapisaæ teren w pliku
TAnimModel *Global::pTerrainCompact=NULL; //do zapisania terenu w pliku
AnsiString Global::asTerrainModel=""; //nazwa obiektu terenu do zapisania w pliku

//parametry testowe (do testowania scenerii i obiektów)
bool Global::bWireFrame=false;
bool Global::bSoundEnabled=true;
bool Global::bWriteLogEnabled=true;
bool Global::bManageNodes=true;
bool Global::bDecompressDDS=false;

//parametry przejœciowe (do usuniêcia)
bool Global::bTimeChange=false; //Ra: ZiomalCl wy³¹czy³ star¹ wersjê nocy
bool Global::bRenderAlpha=true; //Ra: wywali³am tê flagê
bool Global::bnewAirCouplers=true;
bool Global::bDoubleAmbient=false; //podwójna jasnoœæ ambient
double Global::fSunSpeed=1.0; //prêdkoœæ ruchu S³oñca, zmienna do testów
bool Global::bHideConsole=false; //hunter-271211: ukrywanie konsoli

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

AnsiString __fastcall Global::GetNextSymbol()
{//pobranie tokenu z aktualnego parsera
 if (qParser) return qParser->EndOfFile?AnsiString("endconfig"):qParser->GetNextSymbol();
 if (pParser)
 {std::string token;
  pParser->getTokens();
  *pParser >> token;
  return AnsiString(token.c_str());
 };
 return "";
};

void __fastcall Global::LoadIniFile(AnsiString asFileName)
{
 int i;
 for (i=0;i<10;++i)
 {//zerowanie pozycji kamer
  pFreeCameraInit[i]=vector3(0,0,0); //wspó³rzêdne w scenerii
  pFreeCameraInitAngle[i]=vector3(0,0,0); //k¹ty obrotu w radianach
 }
 TFileStream *fs;
 fs=new TFileStream(asFileName,fmOpenRead|fmShareCompat);
 if (!fs) return;
 AnsiString str="";
 int size=fs->Size;
 str.SetLength(size);
 fs->Read(str.c_str(),size);
 //str+="";
 delete fs;
 TQueryParserComp *Parser;
 Parser=new TQueryParserComp(NULL);
 Parser->TextToParse=str;
 //Parser->LoadStringToParse(asFile);
 Parser->First();
 ConfigParse(Parser);
 delete Parser; //Ra: tego jak zwykle nie by³o wczeœniej :]
};

void __fastcall Global::ConfigParse(TQueryParserComp *qp,cParser *cp)
{//Ra: trzeba by przerobiæ na cParser, ¿eby to dzia³a³o w scenerii
 pParser=cp;
 qParser=qp;
 AnsiString str;
 int i;
 do
 {
  str=GetNextSymbol().LowerCase();
  if (str==AnsiString("sceneryfile"))
   {
     str=GetNextSymbol().LowerCase();
     strcpy(szSceneryFile,str.c_str());
   }
  else
  if (str==AnsiString("humanctrlvehicle"))
   {
     str=GetNextSymbol().LowerCase();
     asHumanCtrlVehicle=str;
   }
  else if (str==AnsiString("width"))
   iWindowWidth=GetNextSymbol().ToInt();
  else if (str==AnsiString("height"))
   iWindowHeight=GetNextSymbol().ToInt();
  else if (str==AnsiString("bpp"))
   iBpp=((GetNextSymbol().LowerCase()==AnsiString("32")) ? 32 : 16 );
  else if (str==AnsiString("fullscreen"))
   bFullScreen=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  else if (str==AnsiString("freefly")) //Mczapkie-130302
  {
   bFreeFly=(GetNextSymbol().LowerCase()==AnsiString("yes"));
   pFreeCameraInit[0].x=GetNextSymbol().ToDouble();
   pFreeCameraInit[0].y=GetNextSymbol().ToDouble();
   pFreeCameraInit[0].z=GetNextSymbol().ToDouble();
  }
  else if (str==AnsiString("wireframe"))
   bWireFrame=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  else if (str==AnsiString("debugmode")) //McZapkie! - DebugModeFlag uzywana w mover.pas, warto tez blokowac cheaty gdy false
   DebugModeFlag=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  else if (str==AnsiString("soundenabled")) //McZapkie-040302 - blokada dzwieku - przyda sie do debugowania oraz na komp. bez karty dzw.
   bSoundEnabled=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  //else if (str==AnsiString("renderalpha")) //McZapkie-1312302 - dwuprzebiegowe renderowanie
  // bRenderAlpha=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  else if (str==AnsiString("physicslog")) //McZapkie-030402 - logowanie parametrow fizycznych dla kazdego pojazdu z maszynista
   WriteLogFlag=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  else if (str==AnsiString("physicsdeactivation")) //McZapkie-291103 - usypianie fizyki
   PhysicActivationFlag=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  else if (str==AnsiString("debuglog"))
  {//McZapkie-300402 - wylaczanie log.txt
   str=GetNextSymbol();
   bWriteLogEnabled=(str.LowerCase()==AnsiString("yes"));
  }
  else if (str==AnsiString("adjustscreenfreq"))
  {//McZapkie-240403 - czestotliwosc odswiezania ekranu
   str=GetNextSymbol();
   bAdjustScreenFreq=(str.LowerCase()==AnsiString("yes"));
  }
  else if (str==AnsiString("mousescale"))
  {//McZapkie-060503 - czulosc ruchu myszy (krecenia glowa)
   str=GetNextSymbol();
   fMouseXScale=str.ToDouble();
   str=GetNextSymbol();
   fMouseYScale=str.ToDouble();
  }
  else if (str==AnsiString("enabletraction"))
  {//Winger 040204 - 'zywe' patyki dostosowujace sie do trakcji
   bEnableTraction=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  }
  else if (str==AnsiString("loadtraction"))
  {//Winger 140404 - ladowanie sie trakcji
   bLoadTraction=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  }
  else if (str==AnsiString("livetraction"))
  {//Winger 160404 - zaleznosc napiecia loka od trakcji
   bLiveTraction=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  }
  else if (str==AnsiString("skyenabled"))
  {//youBy - niebo
   if (GetNextSymbol().LowerCase()==AnsiString("yes"))
    asSky="1"; else asSky="0";
  }
  else if (str==AnsiString("managenodes"))
  {
   bManageNodes=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  }
  else if (str==AnsiString("decompressdds"))
  {
   bDecompressDDS=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  }
// ShaXbee - domyslne rozszerzenie tekstur
  else if (str==AnsiString("defaultext"))
  {str=GetNextSymbol().LowerCase(); //rozszerzenie
   if (str=="tga")
    szDefaultExt=szTexturesTGA; //domyœlnie od TGA
   //szDefaultExt=std::string(Parser->GetNextSymbol().LowerCase().c_str());
  }
  else if (str==AnsiString("newaircouplers"))
   bnewAirCouplers=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  else if (str==AnsiString("defaultfiltering"))
   iDefaultFiltering=GetNextSymbol().ToIntDef(-1);
  else if (str==AnsiString("ballastfiltering"))
   iBallastFiltering=GetNextSymbol().ToIntDef(-1);
  else if (str==AnsiString("railprofiltering"))
   iRailProFiltering=GetNextSymbol().ToIntDef(-1);
  else if (str==AnsiString("dynamicfiltering"))
   iDynamicFiltering=GetNextSymbol().ToIntDef(-1);
  else if (str==AnsiString("usevbo"))
   bUseVBO=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  else if (str==AnsiString("feedbackmode"))
   iFeedbackMode=GetNextSymbol().ToIntDef(1); //domyœlnie 1
  else if (str==AnsiString("multiplayer"))
   iMultiplayer=GetNextSymbol().ToIntDef(0); //domyœlnie 0
  else if (str==AnsiString("maxtexturesize"))
  {//wymuszenie przeskalowania tekstur
   i=GetNextSymbol().ToIntDef(16384); //domyœlnie du¿e
   if (i<=  64) iMaxTextureSize=  64; else
   if (i<= 128) iMaxTextureSize= 128; else
   if (i<= 256) iMaxTextureSize= 256; else
   if (i<= 512) iMaxTextureSize= 512; else
   if (i<=1024) iMaxTextureSize=1024; else
   if (i<=2048) iMaxTextureSize=2048; else
   if (i<=4096) iMaxTextureSize=4096; else
   if (i<=8192) iMaxTextureSize=8192; else
    iMaxTextureSize=16384;
  }
  else if (str==AnsiString("doubleambient")) //podwójna jasnoœæ ambient
   bDoubleAmbient=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  else if (str==AnsiString("movelight")) //numer dnia w roku albo -1
  {fMoveLight=GetNextSymbol().ToIntDef(-1); //numer dnia 1..365
   if (fMoveLight==0.0)
   {//pobranie daty z systemu
    unsigned short y,m,d;
    TDate date=Now();
    date.DecodeDate(&y,&m,&d);
    fMoveLight=(double)date-(double)TDate(y,1,1)+1; //numer bie¿¹cego dnia w roku
   }
   if (fMoveLight>0.0) //tu jest nadal zwiêkszone o 1
   {//obliczenie deklinacji wg http://en.wikipedia.org/wiki/Declination (XX wiek)
    fMoveLight=M_PI/182.5*(Global::fMoveLight-1.0); //numer dnia w postaci k¹ta
    fSunDeclination=0.006918-0.3999120*cos(  fMoveLight)+0.0702570*sin(  fMoveLight)
                            -0.0067580*cos(2*fMoveLight)+0.0009070*sin(2*fMoveLight)
                            -0.0026970*cos(3*fMoveLight)+0.0014800*sin(3*fMoveLight);
   }
  }
  else if (str==AnsiString("smoothtraction")) //podwójna jasnoœæ ambient
   bSmoothTraction=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  else if (str==AnsiString("sunspeed")) //prêdkoœæ ruchu S³oñca, zmienna do testów
   fSunSpeed=GetNextSymbol().ToIntDef(1);
  else if (str==AnsiString("multisampling")) //tryb antyaliasingu: 0=brak,1=2px,2=4px
   iMultisampling=GetNextSymbol().ToIntDef(2); //domyœlnie 2
  else if (str==AnsiString("glutfont")) //tekst generowany przez GLUT
   bGlutFont=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  else if (str==AnsiString("latitude")) //szerokoœæ geograficzna
   fLatitudeDeg=GetNextSymbol().ToDouble();
  else if (str==AnsiString("convertmodels")) //tworzenie plików binarnych
   iConvertModels=GetNextSymbol().ToIntDef(2); //domyœlnie 2
  else if (str==AnsiString("inactivepause")) //automatyczna pauza, gdy okno nieaktywne
   bInactivePause=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  else if (str==AnsiString("slowmotion")) //tworzenie plików binarnych
   iSlowMotionMask=GetNextSymbol().ToIntDef(-1); //domyœlnie -1
  else if (str==AnsiString("modifytga")) //czy korygowaæ pliki TGA dla szybszego wczytywania
   iModifyTGA=GetNextSymbol().ToIntDef(0); //domyœlnie 0
  else if (str==AnsiString("hideconsole")) //hunter-271211: ukrywanie konsoli
   bHideConsole=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  else if (str==AnsiString("rollfix")) //Ra: poprawianie przechy³ki, aby wewnêtrzna szyna by³a "pozioma"
   bRollFix=(GetNextSymbol().LowerCase()==AnsiString("yes"));
 }
 while (str!="endconfig"); //(!Parser->EndOfFile)
 //na koniec trochê zale¿noœci
 if (!bLoadTraction)
 {//tutaj wy³¹czenie, bo mog¹ nie byæ zdefiniowane w INI
  bEnableTraction=false;
  bLiveTraction=false;
 }
 //if (fMoveLight>0) bDoubleAmbient=false; //wtedy tylko jedno œwiat³o ruchome
 //if (fOpenGL<1.3) iMultisampling=0; //mo¿na by z góry wy³¹czyæ, ale nie mamy jeszcze fOpenGL
 if (iMultisampling)
 {//antyaliasing ca³oekranowy wy³¹cza rozmywanie drutów
  bSmoothTraction=false;
 }
 if (iMultiplayer>0)
  bInactivePause=false; //pauza nieaktywna, jeœli w³¹czona komunikacja
 Console::ModeSet(iFeedbackMode); //tryb pracy konsoli sterowniczej
}

void __fastcall Global::InitKeys(AnsiString asFileName)
{
//    if (FileExists(asFileName))
//    {
//       Error("Chwilowo plik keys.ini nie jest obs³ugiwany. £adujê standardowe ustawienia.\nKeys.ini file is temporarily not functional, loading default keymap...");
/*        TQueryParserComp *Parser;
        Parser=new TQueryParserComp(NULL);
        Parser->LoadStringToParse(asFileName);

        for (int keycount=0; keycount<MaxKeys; keycount++)
         {
          Keys[keycount]=Parser->GetNextSymbol().ToInt();
         }

        delete Parser;
*/
//    }
//    else
    {
        Keys[k_IncMainCtrl]=VK_ADD;
        Keys[k_IncMainCtrlFAST]=VK_ADD;
        Keys[k_DecMainCtrl]=VK_SUBTRACT;
        Keys[k_DecMainCtrlFAST]=VK_SUBTRACT;
        Keys[k_IncScndCtrl]=VK_DIVIDE;
        Keys[k_IncScndCtrlFAST]=VK_DIVIDE;
        Keys[k_DecScndCtrl]=VK_MULTIPLY;
        Keys[k_DecScndCtrlFAST]=VK_MULTIPLY;
///*NORMALNE
        Keys[k_IncLocalBrakeLevel]=VK_NUMPAD1;  //VK_NUMPAD7;
        //Keys[k_IncLocalBrakeLevelFAST]=VK_END;  //VK_HOME;
        Keys[k_DecLocalBrakeLevel]=VK_NUMPAD7;  //VK_NUMPAD1;
        //Keys[k_DecLocalBrakeLevelFAST]=VK_HOME; //VK_END;
        Keys[k_IncBrakeLevel]=VK_NUMPAD3;  //VK_NUMPAD9;
        Keys[k_DecBrakeLevel]=VK_NUMPAD9;   //VK_NUMPAD3;
        Keys[k_Releaser]=VK_NUMPAD6;
        Keys[k_EmergencyBrake]=VK_NUMPAD0;
        Keys[k_Brake3]=VK_NUMPAD8;
        Keys[k_Brake2]=VK_NUMPAD5;
        Keys[k_Brake1]=VK_NUMPAD2;
        Keys[k_Brake0]=VK_NUMPAD4;
        Keys[k_WaveBrake]=VK_DECIMAL;
//*/
/*MOJE
        Keys[k_IncLocalBrakeLevel]=VK_NUMPAD3;  //VK_NUMPAD7;
        Keys[k_IncLocalBrakeLevelFAST]=VK_NUMPAD3;  //VK_HOME;
        Keys[k_DecLocalBrakeLevel]=VK_DECIMAL;  //VK_NUMPAD1;
        Keys[k_DecLocalBrakeLevelFAST]=VK_DECIMAL; //VK_END;
        Keys[k_IncBrakeLevel]=VK_NUMPAD6;  //VK_NUMPAD9;
        Keys[k_DecBrakeLevel]=VK_NUMPAD9;   //VK_NUMPAD3;
        Keys[k_Releaser]=VK_NUMPAD5;
        Keys[k_EmergencyBrake]=VK_NUMPAD0;
        Keys[k_Brake3]=VK_NUMPAD2;
        Keys[k_Brake2]=VK_NUMPAD1;
        Keys[k_Brake1]=VK_NUMPAD4;
        Keys[k_Brake0]=VK_NUMPAD7;
        Keys[k_WaveBrake]=VK_NUMPAD8;
*/
        Keys[k_AntiSlipping]=VK_RETURN;
        Keys[k_Sand]=VkKeyScan('s');
        Keys[k_Main]=VkKeyScan('m');
        Keys[k_Active]=VkKeyScan('w');
        Keys[k_DirectionForward]=VkKeyScan('d');
        Keys[k_DirectionBackward]=VkKeyScan('r');
        Keys[k_Fuse]=VkKeyScan('n');
        Keys[k_Compressor]=VkKeyScan('c');
        Keys[k_Converter]=VkKeyScan('x');
        Keys[k_MaxCurrent]=VkKeyScan('f');
        Keys[k_CurrentAutoRelay]=VkKeyScan('g');
        Keys[k_BrakeProfile]=VkKeyScan('b');
        Keys[k_CurrentNext]=VkKeyScan('z');

        Keys[k_Czuwak]=VkKeyScan(' ');
        Keys[k_Horn]=VkKeyScan('a');
        Keys[k_Horn2]=VkKeyScan('a');

        Keys[k_FailedEngineCutOff]=VkKeyScan('e');

        Keys[k_MechUp]=VK_PRIOR;
        Keys[k_MechDown]=VK_NEXT ;
        Keys[k_MechLeft]=VK_LEFT ;
        Keys[k_MechRight]=VK_RIGHT;
        Keys[k_MechForward]=VK_UP;
        Keys[k_MechBackward]=VK_DOWN;

        Keys[k_CabForward]=VK_HOME;
        Keys[k_CabBackward]=VK_END;

        Keys[k_Couple]=VK_INSERT;
        Keys[k_DeCouple]=VK_DELETE;

        Keys[k_ProgramQuit]=VK_F10;
        //Keys[k_ProgramPause]=VK_F3;
        Keys[k_ProgramHelp]=VK_F1;
        //Keys[k_FreeFlyMode]=VK_F4;
        Keys[k_WalkMode]=VK_F5;

        Keys[k_OpenLeft]=VkKeyScan(',');
        Keys[k_OpenRight]=VkKeyScan('.');
        Keys[k_CloseLeft]=VkKeyScan(',');
        Keys[k_CloseRight]=VkKeyScan('.');
        Keys[k_DepartureSignal]=VkKeyScan('/');

//Winger 160204 - obsluga pantografow
        Keys[k_PantFrontUp]=VkKeyScan('p'); //Ra: zamieniony przedni z tylnym
        Keys[k_PantFrontDown]=VkKeyScan('p');
        Keys[k_PantRearUp]=VkKeyScan('o');
        Keys[k_PantRearDown]=VkKeyScan('o');
//Winger 020304 - ogrzewanie
        Keys[k_Heating]=VkKeyScan('h');
        Keys[k_LeftSign]=VkKeyScan('y');
        Keys[k_UpperSign]=VkKeyScan('u');
        Keys[k_RightSign]=VkKeyScan('i');
        Keys[k_EndSign]=VkKeyScan('t');

        Keys[k_SmallCompressor]=VkKeyScan('v');
        Keys[k_StLinOff]=VkKeyScan('l');
//ABu 090305 - przyciski uniwersalne, do roznych bajerow :)
        Keys[k_Univ1]=VkKeyScan('[');
        Keys[k_Univ2]=VkKeyScan(']');
        Keys[k_Univ3]=VkKeyScan(';');
        Keys[k_Univ4]=VkKeyScan('\'');
    }
}
/*
vector3 __fastcall Global::GetCameraPosition()
{
    return pCameraPosition;
}
*/
void __fastcall Global::SetCameraPosition(vector3 pNewCameraPosition)
{
    pCameraPosition=pNewCameraPosition;
}

void __fastcall Global::SetCameraRotation(double Yaw)
{//ustawienie bezwzglêdnego kierunku kamery z korekcj¹ do przedzia³u <-M_PI,M_PI>
 pCameraRotation=Yaw;
 while (pCameraRotation<-M_PI) pCameraRotation+=2*M_PI;
 while (pCameraRotation> M_PI) pCameraRotation-=2*M_PI;
 pCameraRotationDeg=pCameraRotation*180.0/M_PI;
}

void __fastcall Global::BindTexture(GLuint t)
{//ustawienie aktualnej tekstury, tylko gdy siê zmienia
 if (t!=iTextureId)
 {iTextureId=t;
 }
};

void __fastcall Global::TrainDelete(TDynamicObject *d)
{//usuniêcie pojazdu prowadzonego przez u¿ytkownika
 if (pWorld) pWorld->TrainDelete(d);
};


//---------------------------------------------------------------------------

#pragma package(smart_init)
