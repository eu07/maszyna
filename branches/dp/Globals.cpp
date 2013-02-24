//---------------------------------------------------------------------------
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

*/

#include "system.hpp"
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

//parametry do u�ytku wewn�trznego
//double Global::tSinceStart=0;
TGround *Global::pGround=NULL;
//char Global::CreatorName1[30]="2001-2004 Maciej Czapkiewicz";
//char Global::CreatorName2[30]="2001-2003 Marcin Wo�niak <Marcin_EU>";
//char Global::CreatorName3[20]="2004-2005 Adam Bugiel <ABu>";
//char Global::CreatorName4[30]="2004 Arkadiusz �lusarczyk <Winger>";
//char Global::CreatorName5[30]="2003-2009 �ukasz Kirchner <Nbmx>";
AnsiString Global::asCurrentSceneryPath="scenery/";
AnsiString Global::asCurrentTexturePath=AnsiString(szDefaultTexturePath);
AnsiString Global::asCurrentDynamicPath="";
int Global::iSlowMotion=0; //info o malym FPS: 0-OK, 1-wy��czy� multisampling, 3-promie� 1.5km, 7-1km
TDynamicObject *Global::changeDynObj=NULL; //info o zmianie pojazdu
bool Global::detonatoryOK; //info o nowych detonatorach
double Global::ABuDebug=0;
AnsiString Global::asSky="1";
double Global::fOpenGL=0.0; //wersja OpenGL - do sprawdzania obecno�ci rozszerze�
bool Global::bOpenGL_1_5=false; //czy s� dost�pne funkcje OpenGL 1.5
double Global::fLuminance=1.0; //jasno�� �wiat�a do automatycznego zapalania
int Global::iReCompile=0; //zwi�kszany, gdy trzeba od�wie�y� siatki
HWND Global::hWnd=NULL; //uchwyt okna
int Global::iCameraLast=-1;
AnsiString Global::asRelease="13.2.764.407";
AnsiString Global::asVersion="Compilation 2013-02-24, release "+Global::asRelease+"."; //tutaj, bo wysy�any
int Global::iViewMode=0; //co aktualnie wida�: 0-kabina, 1-latanie, 2-sprz�gi, 3-dokumenty
int Global::iTextMode=0; //tryb pracy wy�wietlacza tekstowego
double Global::fSunDeclination=0.0; //deklinacja S�o�ca
double Global::fTimeAngleDeg=0.0; //godzina w postaci k�ta
char* Global::szTexturesTGA[4]={"tga","dds","tex","bmp"}; //lista tekstur od TGA
char* Global::szTexturesDDS[4]={"dds","tga","tex","bmp"}; //lista tekstur od DDS
int Global::iKeyLast=0; //ostatnio naci�ni�ty klawisz w celu logowania
GLuint Global::iTextureId=0; //ostatnio u�yta tekstura 2D
bool Global::bPause=false; //globalna pauza ruchu
bool Global::bActive=true; //czy jest aktywnym oknem
int Global::iErorrCounter=0; //licznik sprawdza� do �ledzenia b��d�w OpenGL
int Global::iTextures=0; //licznik u�ytych tekstur
TWorld* Global::pWorld=NULL;
Queryparsercomp::TQueryParserComp *Global::qParser=NULL;
cParser *Global::pParser=NULL;
int Global::iSegmentsRendered=90; //ilo�� segment�w do regulacji wydajno�ci
TCamera* Global::pCamera=NULL; //parametry kamery
TDynamicObject *Global::pUserDynamic=NULL; //pojazd u�ytkownika, renderowany bez trz�sienia
bool Global::bSmudge=false; //czy wy�wietla� smug�, a pojazd u�ytkownika na ko�cu

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
GLfloat Global::ambientLight[]    ={0.80f,0.80f,0.85f,1.0f}; //sta�e
GLfloat Global::diffuseLight[]    ={0.85f,0.85f,0.80f,1.0f};
GLfloat Global::specularLight[]   ={0.95f,0.94f,0.90f,1.0f};
GLfloat Global::whiteLight[]      ={1.00f,1.00f,1.00f,1.0f};
GLfloat Global::noLight[]         ={0.00f,0.00f,0.00f,1.0f};
GLfloat Global::darkLight[]       ={0.03f,0.03f,0.03f,1.0f}; //�ladowe
GLfloat Global::lightPos[4];
bool Global::bRollFix=true; //czy wykona� przeliczanie przechy�ki
bool Global::bJoinEvents=false; //czy grupowa� eventy o tych samych nazwach

//parametry u�ytkowe (jak komu pasuje)
int Global::Keys[MaxKeys];
int Global::iWindowWidth=800;
int Global::iWindowHeight=600;
int Global::iFeedbackMode=1; //tryb pracy informacji zwrotnej
int Global::iFeedbackPort=0; //dodatkowy adres dla informacji zwrotnych
bool Global::bFreeFly=false;
bool Global::bFullScreen=false;
bool Global::bInactivePause=true; //automatyczna pauza, gdy okno nieaktywne
float Global::fMouseXScale=1.5;
float Global::fMouseYScale=0.2;
char Global::szSceneryFile[256]="td.scn";
AnsiString Global::asHumanCtrlVehicle="EU07-424";
int Global::iMultiplayer=0; //blokada dzia�ania niekt�rych funkcji na rzecz kominikacji
double Global::fMoveLight=-1; //ruchome �wiat�o
double Global::fLatitudeDeg=52.0; //szeroko�� geograficzna
float Global::fFriction=1.0; //mno�nik tarcia - KURS90
double Global::fBrakeStep=1.0; //krok zmiany hamulca dla klawiszy [Num3] i [Num9]


//parametry wydajno�ciowe (np. regulacja FPS, szybko�� wczytywania)
bool Global::bAdjustScreenFreq=true;
bool Global::bEnableTraction=true;
bool Global::bLoadTraction=true;
bool Global::bLiveTraction=true;
int Global::iDefaultFiltering=9; //domy�lne rozmywanie tekstur TGA bez alfa
int Global::iBallastFiltering=9; //domy�lne rozmywanie tekstur podsypki
int Global::iRailProFiltering=5; //domy�lne rozmywanie tekstur szyn
int Global::iDynamicFiltering=5; //domy�lne rozmywanie tekstur pojazd�w
bool Global::bUseVBO=false; //czy jest VBO w karcie graficznej (czy u�y�)
GLint Global::iMaxTextureSize=16384; //maksymalny rozmiar tekstury
bool Global::bSmoothTraction=false; //wyg�adzanie drut�w starym sposobem
char** Global::szDefaultExt=Global::szTexturesDDS; //domy�lnie od DDS
int Global::iMultisampling=2; //tryb antyaliasingu: 0=brak,1=2px,2=4px,3=8px,4=16px
bool Global::bGlutFont=false; //czy tekst generowany przez GLUT32.DLL
int Global::iConvertModels=6; //tworzenie plik�w binarnych, 2-optymalizacja transform�w
int Global::iSlowMotionMask=-1; //maska wy��czanych w�a�ciwo�ci dla zwi�kszenia FPS
int Global::iModifyTGA=7; //czy korygowa� pliki TGA dla szybszego wczytywania
//bool Global::bTerrainCompact=true; //czy zapisa� teren w pliku
TAnimModel *Global::pTerrainCompact=NULL; //do zapisania terenu w pliku
AnsiString Global::asTerrainModel=""; //nazwa obiektu terenu do zapisania w pliku
double Global::fFpsAverage=20.0; //oczekiwana wartos� FPS
double Global::fFpsDeviation=5.0; //odchylenie standardowe FPS
double Global::fFpsMin=0.0; //dolna granica FPS, przy kt�rej promie� scenerii b�dzie zmniejszany
double Global::fFpsMax=0.0; //g�rna granica FPS, przy kt�rej promie� scenerii b�dzie zwi�kszany
double Global::fFpsRadiusMax=3000.0; //maksymalny promie� renderowania
int Global::iFpsRadiusMax=225; //maksymalny promie� renderowania
double Global::fRadiusFactor=1.1; //wsp�czynnik jednorazowej zmiany promienia scenerii

//parametry testowe (do testowania scenerii i obiekt�w)
bool Global::bWireFrame=false;
bool Global::bSoundEnabled=true;
int Global::iWriteLogEnabled=3; //maska bitowa: 1-zapis do pliku, 2-okienko, 4-nazwy tor�w
bool Global::bManageNodes=true;
bool Global::bDecompressDDS=false; //czy programowa dekompresja DDS

//parametry do kalibracji
//kolejno wsp�czynniki dla pot�g 0, 1, 2, 3 warto�ci odczytanej z urz�dzenia
double Global::fCalibrateIn[6][4]={{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}};
double Global::fCalibrateOut[6][4]={{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}};

//parametry przej�ciowe (do usuni�cia)
//bool Global::bTimeChange=false; //Ra: ZiomalCl wy��czy� star� wersj� nocy
//bool Global::bRenderAlpha=true; //Ra: wywali�am t� flag�
bool Global::bnewAirCouplers=true;
bool Global::bDoubleAmbient=false; //podw�jna jasno�� ambient
double Global::fTimeSpeed=1.0; //przyspieszenie czasu, zmienna do test�w
bool Global::bHideConsole=false; //hunter-271211: ukrywanie konsoli
int Global::iBpp=32; //chyba ju� nie u�ywa si� kart, na kt�rych 16bpp co� poprawi

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
  pFreeCameraInit[i]=vector3(0,0,0); //wsp�rz�dne w scenerii
  pFreeCameraInitAngle[i]=vector3(0,0,0); //k�ty obrotu w radianach
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
 delete Parser; //Ra: tego jak zwykle nie by�o wcze�niej :]
};

void __fastcall Global::ConfigParse(TQueryParserComp *qp,cParser *cp)
{//Ra: trzeba by przerobi� na cParser, �eby to dzia�a�o w scenerii
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
   str=GetNextSymbol().LowerCase();
   if (str=="yes") iWriteLogEnabled=3;
   else if (str=="no") iWriteLogEnabled=0;
   else iWriteLogEnabled=str.ToIntDef(3);
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
  else if (str==AnsiString("friction")) //mno�nik tarcia - KURS90
   fFriction=GetNextSymbol().ToDouble();
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
    szDefaultExt=szTexturesTGA; //domy�lnie od TGA
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
   iFeedbackMode=GetNextSymbol().ToIntDef(1); //domy�lnie 1
  else if (str==AnsiString("feedbackport"))
   iFeedbackPort=GetNextSymbol().ToIntDef(0); //domy�lnie 0
  else if (str==AnsiString("multiplayer"))
   iMultiplayer=GetNextSymbol().ToIntDef(0); //domy�lnie 0
  else if (str==AnsiString("maxtexturesize"))
  {//wymuszenie przeskalowania tekstur
   i=GetNextSymbol().ToIntDef(16384); //domy�lnie du�e
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
  else if (str==AnsiString("doubleambient")) //podw�jna jasno�� ambient
   bDoubleAmbient=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  else if (str==AnsiString("movelight")) //numer dnia w roku albo -1
  {fMoveLight=GetNextSymbol().ToIntDef(-1); //numer dnia 1..365
   if (fMoveLight==0.0)
   {//pobranie daty z systemu
    unsigned short y,m,d;
    TDate date=Now();
    date.DecodeDate(&y,&m,&d);
    fMoveLight=(double)date-(double)TDate(y,1,1)+1; //numer bie��cego dnia w roku
   }
   if (fMoveLight>0.0) //tu jest nadal zwi�kszone o 1
   {//obliczenie deklinacji wg http://en.wikipedia.org/wiki/Declination (XX wiek)
    fMoveLight=M_PI/182.5*(Global::fMoveLight-1.0); //numer dnia w postaci k�ta
    fSunDeclination=0.006918-0.3999120*cos(  fMoveLight)+0.0702570*sin(  fMoveLight)
                            -0.0067580*cos(2*fMoveLight)+0.0009070*sin(2*fMoveLight)
                            -0.0026970*cos(3*fMoveLight)+0.0014800*sin(3*fMoveLight);
   }
  }
  else if (str==AnsiString("smoothtraction")) //podw�jna jasno�� ambient
   bSmoothTraction=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  else if (str==AnsiString("timespeed")) //przyspieszenie czasu, zmienna do test�w
   fTimeSpeed=GetNextSymbol().ToIntDef(1);
  else if (str==AnsiString("multisampling")) //tryb antyaliasingu: 0=brak,1=2px,2=4px
   iMultisampling=GetNextSymbol().ToIntDef(2); //domy�lnie 2
  else if (str==AnsiString("glutfont")) //tekst generowany przez GLUT
   bGlutFont=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  else if (str==AnsiString("latitude")) //szeroko�� geograficzna
   fLatitudeDeg=GetNextSymbol().ToDouble();
  else if (str==AnsiString("convertmodels")) //tworzenie plik�w binarnych
   iConvertModels=GetNextSymbol().ToIntDef(6); //domy�lnie 6
  else if (str==AnsiString("inactivepause")) //automatyczna pauza, gdy okno nieaktywne
   bInactivePause=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  else if (str==AnsiString("slowmotion")) //tworzenie plik�w binarnych
   iSlowMotionMask=GetNextSymbol().ToIntDef(-1); //domy�lnie -1
  else if (str==AnsiString("modifytga")) //czy korygowa� pliki TGA dla szybszego wczytywania
   iModifyTGA=GetNextSymbol().ToIntDef(0); //domy�lnie 0
  else if (str==AnsiString("hideconsole")) //hunter-271211: ukrywanie konsoli
   bHideConsole=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  else if (str==AnsiString("rollfix")) //Ra: poprawianie przechy�ki, aby wewn�trzna szyna by�a "pozioma"
   bRollFix=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  else if (str==AnsiString("fpsaverage")) //oczekiwana wartos� FPS
   fFpsAverage=GetNextSymbol().ToDouble();
  else if (str==AnsiString("fpsdeviation")) //odchylenie standardowe FPS
   fFpsDeviation=GetNextSymbol().ToDouble();
  else if (str==AnsiString("fpsradiusmax")) //maksymalny promie� renderowania
   fFpsRadiusMax=GetNextSymbol().ToDouble();
  else if (str==AnsiString("calibratein")) //parametry kalibracji wej��
  {//
   i=GetNextSymbol().ToIntDef(-1); //numer wej�cia
   if ((i<0)||(i>5)) i=5; //na ostatni, bo i tak trzeba pomin�� warto�ci
   fCalibrateIn[i][0]=GetNextSymbol().ToDouble(); //wyraz wolny
   fCalibrateIn[i][1]=GetNextSymbol().ToDouble(); //mno�nik
   fCalibrateIn[i][2]=GetNextSymbol().ToDouble(); //mno�nik dla kwadratu
   fCalibrateIn[i][3]=GetNextSymbol().ToDouble(); //mno�nik dla sze�cianu
  }
  else if (str==AnsiString("calibrateout")) //parametry kalibracji wyj��
  {//
   i=GetNextSymbol().ToIntDef(-1); //numer wej�cia
   if ((i<0)||(i>5)) i=5; //na ostatni, bo i tak trzeba pomin�� warto�ci
   fCalibrateOut[i][0]=GetNextSymbol().ToDouble(); //wyraz wolny
   fCalibrateOut[i][1]=GetNextSymbol().ToDouble(); //mno�nik
   fCalibrateOut[i][2]=GetNextSymbol().ToDouble(); //mno�nik dla kwadratu
   fCalibrateOut[i][3]=GetNextSymbol().ToDouble(); //mno�nik dla sze�cianu
  }
  else if (str==AnsiString("brakestep")) //krok zmiany hamulca dla klawiszy [Num3] i [Num9]
   fBrakeStep=GetNextSymbol().ToDouble();
  else if (str==AnsiString("joinduplicatedevents")) //czy grupowa� eventy o tych samych nazwach
   bJoinEvents=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  else if (str==AnsiString("pause")) //czy po wczytaniu ma by� pauza?
   bPause=(GetNextSymbol().LowerCase()==AnsiString("yes"));
 }
 while (str!="endconfig"); //(!Parser->EndOfFile)
 //na koniec troch� zale�no�ci
 if (!bLoadTraction)
 {//tutaj wy��czenie, bo mog� nie by� zdefiniowane w INI
  bEnableTraction=false;
  bLiveTraction=false;
 }
 //if (fMoveLight>0) bDoubleAmbient=false; //wtedy tylko jedno �wiat�o ruchome
 //if (fOpenGL<1.3) iMultisampling=0; //mo�na by z g�ry wy��czy�, ale nie mamy jeszcze fOpenGL
 if (iMultisampling)
 {//antyaliasing ca�oekranowy wy��cza rozmywanie drut�w
  bSmoothTraction=false;
 }
 if (iMultiplayer>0)
  bInactivePause=false; //pauza nieaktywna, je�li w��czona komunikacja
 Console::ModeSet(iFeedbackMode,iFeedbackPort); //tryb pracy konsoli sterowniczej
 fFpsMin=fFpsAverage-fFpsDeviation; //dolna granica FPS, przy kt�rej promie� scenerii b�dzie zmniejszany
 fFpsMax=fFpsAverage+fFpsDeviation; //g�rna granica FPS, przy kt�rej promie� scenerii b�dzie zwi�kszany
 iFpsRadiusMax=0.000025*fFpsRadiusMax*fFpsRadiusMax; //maksymalny promie� renderowania 3000.0 -> 225
 if (iFpsRadiusMax>400) iFpsRadiusMax=400;
 if (bPause) iTextMode=VK_F1; //jak pauza, to pokaza� zegar
}

void __fastcall Global::InitKeys(AnsiString asFileName)
{
//    if (FileExists(asFileName))
//    {
//       Error("Chwilowo plik keys.ini nie jest obs�ugiwany. �aduj� standardowe ustawienia.\nKeys.ini file is temporarily not functional, loading default keymap...");
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
        Keys[k_Battery]=VkKeyScan('j');
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
{//ustawienie bezwzgl�dnego kierunku kamery z korekcj� do przedzia�u <-M_PI,M_PI>
 pCameraRotation=Yaw;
 while (pCameraRotation<-M_PI) pCameraRotation+=2*M_PI;
 while (pCameraRotation> M_PI) pCameraRotation-=2*M_PI;
 pCameraRotationDeg=pCameraRotation*180.0/M_PI;
}

void __fastcall Global::BindTexture(GLuint t)
{//ustawienie aktualnej tekstury, tylko gdy si� zmienia
 if (t!=iTextureId)
 {iTextureId=t;
 }
};

void __fastcall Global::TrainDelete(TDynamicObject *d)
{//usuni�cie pojazdu prowadzonego przez u�ytkownika
 if (pWorld) pWorld->TrainDelete(d);
};

TDynamicObject* __fastcall Global::DynamicNearest()
{//ustalenie pojazdu najbli�szego kamerze
 return pGround->DynamicNearest(pCamera->Pos);
};

bool __fastcall Global::AddToQuery(TEvent *event,TDynamicObject *who)
{
 return pGround->AddToQuery(event,who);
};
//---------------------------------------------------------------------------

#pragma package(smart_init)
