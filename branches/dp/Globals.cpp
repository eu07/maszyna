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

//parametry do u¿ytku wewnêtrznego
//double Global::tSinceStart=0;
TGround *Global::pGround=NULL;
//char Global::CreatorName1[30]="2001-2004 Maciej Czapkiewicz <McZapkie>";
//char Global::CreatorName2[30]="2001-2003 Marcin WoŸniak <Marcin_EU>";
//char Global::CreatorName3[20]="2004-2005 Adam Bugiel <ABu>";
//char Global::CreatorName4[30]="2004 Arkadiusz Œlusarczyk <Winger>";
//char Global::CreatorName5[30]="2003-2009 £ukasz Kirchner <Nbmx>";
AnsiString Global::asCurrentSceneryPath="scenery/";
AnsiString Global::asCurrentTexturePath=AnsiString(szTexturePath);
AnsiString Global::asCurrentDynamicPath="";
int Global::iSlowMotion=0; //info o malym FPS: 0-OK, 1-wy³¹czyæ multisampling, 3-promieñ 1.5km, 7-1km
TDynamicObject *Global::changeDynObj=NULL; //info o zmianie pojazdu
bool Global::detonatoryOK; //info o nowych detonatorach
double Global::ABuDebug=0;
AnsiString Global::asSky="1";
double Global::fOpenGL=0.0; //wersja OpenGL - do sprawdzania obecnoœci rozszerzeñ
bool Global::bOpenGL_1_5=false; //czy s¹ dostêpne funkcje OpenGL 1.5
double Global::fLuminance=1.0; //jasnoœæ œwiat³a do automatycznego zapalania
int Global::iReCompile=0; //zwiêkszany, gdy trzeba odœwie¿yæ siatki
HWND Global::hWnd=NULL; //uchwyt okna
int Global::iCameraLast=-1;
AnsiString Global::asRelease="14.4.1012.434";
AnsiString Global::asVersion="Compilation 2014-04-02, release "+Global::asRelease+"."; //tutaj, bo wysy³any
int Global::iViewMode=0; //co aktualnie widaæ: 0-kabina, 1-latanie, 2-sprzêgi, 3-dokumenty
int Global::iTextMode=0; //tryb pracy wyœwietlacza tekstowego
int Global::iScreenMode[12]={0,0,0,0,0,0,0,0,0,0,0,0}; //numer ekranu wyœwietlacza tekstowego
double Global::fSunDeclination=0.0; //deklinacja S³oñca
double Global::fTimeAngleDeg=0.0; //godzina w postaci k¹ta
char* Global::szTexturesTGA[4]={"tga","dds","tex","bmp"}; //lista tekstur od TGA
char* Global::szTexturesDDS[4]={"dds","tga","tex","bmp"}; //lista tekstur od DDS
int Global::iKeyLast=0; //ostatnio naciœniêty klawisz w celu logowania
GLuint Global::iTextureId=0; //ostatnio u¿yta tekstura 2D
int Global::iPause=0; //globalna pauza ruchu
bool Global::bActive=true; //czy jest aktywnym oknem
int Global::iErorrCounter=0; //licznik sprawdzañ do œledzenia b³êdów OpenGL
int Global::iTextures=0; //licznik u¿ytych tekstur
TWorld* Global::pWorld=NULL;
Queryparsercomp::TQueryParserComp *Global::qParser=NULL;
cParser *Global::pParser=NULL;
int Global::iSegmentsRendered=90; //iloœæ segmentów do regulacji wydajnoœci
TCamera* Global::pCamera=NULL; //parametry kamery
TDynamicObject *Global::pUserDynamic=NULL; //pojazd u¿ytkownika, renderowany bez trzêsienia
bool Global::bSmudge=false; //czy wyœwietlaæ smugê, a pojazd u¿ytkownika na koñcu
AnsiString Global::asTranscript[5]; //napisy na ekranie (widoczne)
TTranscripts Global::tranTexts; //obiekt obs³uguj¹cy stenogramy dŸwiêków na ekranie

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
bool Global::bJoinEvents=false; //czy grupowaæ eventy o tych samych nazwach

//parametry u¿ytkowe (jak komu pasuje)
int Global::Keys[MaxKeys];
int Global::iWindowWidth=800;
int Global::iWindowHeight=600;
float Global::fDistanceFactor=768.0; //baza do przeliczania odleg³oœci dla LoD
int Global::iFeedbackMode=1; //tryb pracy informacji zwrotnej
int Global::iFeedbackPort=0; //dodatkowy adres dla informacji zwrotnych
bool Global::bFreeFly=false;
bool Global::bFullScreen=false;
bool Global::bInactivePause=true; //automatyczna pauza, gdy okno nieaktywne
float Global::fMouseXScale=1.5;
float Global::fMouseYScale=0.2;
char Global::szSceneryFile[256]="td.scn";
AnsiString Global::asHumanCtrlVehicle="EU07-424";
int Global::iMultiplayer=0; //blokada dzia³ania niektórych funkcji na rzecz komunikacji
double Global::fMoveLight=-1; //ruchome œwiat³o
double Global::fLatitudeDeg=52.0; //szerokoœæ geograficzna
float Global::fFriction=1.0; //mno¿nik tarcia - KURS90
double Global::fBrakeStep=1.0; //krok zmiany hamulca dla klawiszy [Num3] i [Num9]
AnsiString Global::asLang="pl"; //domyœlny jêzyk - http://tools.ietf.org/html/bcp47


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
int Global::iConvertModels=7; //tworzenie plików binarnych, 2-optymalizacja transformów
int Global::iSlowMotionMask=-1; //maska wy³¹czanych w³aœciwoœci dla zwiêkszenia FPS
int Global::iModifyTGA=7; //czy korygowaæ pliki TGA dla szybszego wczytywania
//bool Global::bTerrainCompact=true; //czy zapisaæ teren w pliku
TAnimModel *Global::pTerrainCompact=NULL; //do zapisania terenu w pliku
AnsiString Global::asTerrainModel=""; //nazwa obiektu terenu do zapisania w pliku
double Global::fFpsAverage=20.0; //oczekiwana wartosæ FPS
double Global::fFpsDeviation=5.0; //odchylenie standardowe FPS
double Global::fFpsMin=0.0; //dolna granica FPS, przy której promieñ scenerii bêdzie zmniejszany
double Global::fFpsMax=0.0; //górna granica FPS, przy której promieñ scenerii bêdzie zwiêkszany
double Global::fFpsRadiusMax=3000.0; //maksymalny promieñ renderowania
int Global::iFpsRadiusMax=225; //maksymalny promieñ renderowania
double Global::fRadiusFactor=1.1; //wspó³czynnik jednorazowej zmiany promienia scenerii

//parametry testowe (do testowania scenerii i obiektów)
bool Global::bWireFrame=false;
bool Global::bSoundEnabled=true;
int Global::iWriteLogEnabled=3; //maska bitowa: 1-zapis do pliku, 2-okienko, 4-nazwy torów
bool Global::bManageNodes=true;
bool Global::bDecompressDDS=false; //czy programowa dekompresja DDS

//parametry do kalibracji
//kolejno wspó³czynniki dla potêg 0, 1, 2, 3 wartoœci odczytanej z urz¹dzenia
double Global::fCalibrateIn[6][4]={{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}};
double Global::fCalibrateOut[7][4]={{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}};

//parametry przejœciowe (do usuniêcia)
//bool Global::bTimeChange=false; //Ra: ZiomalCl wy³¹czy³ star¹ wersjê nocy
//bool Global::bRenderAlpha=true; //Ra: wywali³am tê flagê
bool Global::bnewAirCouplers=true;
bool Global::bDoubleAmbient=false; //podwójna jasnoœæ ambient
double Global::fTimeSpeed=1.0; //przyspieszenie czasu, zmienna do testów
bool Global::bHideConsole=false; //hunter-271211: ukrywanie konsoli
int Global::iBpp=32; //chyba ju¿ nie u¿ywa siê kart, na których 16bpp coœ poprawi

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
  else if (str==AnsiString("heightbase"))
   fDistanceFactor=GetNextSymbol().ToInt();
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
  {//Winger 040204 - 'zywe' patyki dostosowujace sie do trakcji; Ra 2014-03: teraz ³amanie
   bEnableTraction=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  }
  else if (str==AnsiString("loadtraction"))
  {//Winger 140404 - ladowanie sie trakcji
   bLoadTraction=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  }
  else if (str==AnsiString("friction")) //mno¿nik tarcia - KURS90
   fFriction=GetNextSymbol().ToDouble();
  else if (str==AnsiString("livetraction"))
  {//Winger 160404 - zaleznosc napiecia loka od trakcji; Ra 2014-03: teraz pr¹d przy braku sieci
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
  else if (str==AnsiString("feedbackport"))
   iFeedbackPort=GetNextSymbol().ToIntDef(0); //domyœlnie 0
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
  else if (str==AnsiString("timespeed")) //przyspieszenie czasu, zmienna do testów
   fTimeSpeed=GetNextSymbol().ToIntDef(1);
  else if (str==AnsiString("multisampling")) //tryb antyaliasingu: 0=brak,1=2px,2=4px
   iMultisampling=GetNextSymbol().ToIntDef(2); //domyœlnie 2
  else if (str==AnsiString("glutfont")) //tekst generowany przez GLUT
   bGlutFont=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  else if (str==AnsiString("latitude")) //szerokoœæ geograficzna
   fLatitudeDeg=GetNextSymbol().ToDouble();
  else if (str==AnsiString("convertmodels")) //tworzenie plików binarnych
   iConvertModels=GetNextSymbol().ToIntDef(7); //domyœlnie 7
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
  else if (str==AnsiString("fpsaverage")) //oczekiwana wartosæ FPS
   fFpsAverage=GetNextSymbol().ToDouble();
  else if (str==AnsiString("fpsdeviation")) //odchylenie standardowe FPS
   fFpsDeviation=GetNextSymbol().ToDouble();
  else if (str==AnsiString("fpsradiusmax")) //maksymalny promieñ renderowania
   fFpsRadiusMax=GetNextSymbol().ToDouble();
  else if (str==AnsiString("calibratein")) //parametry kalibracji wejœæ
  {//
   i=GetNextSymbol().ToIntDef(-1); //numer wejœcia
   if ((i<0)||(i>5)) i=5; //na ostatni, bo i tak trzeba pomin¹æ wartoœci
   fCalibrateIn[i][0]=GetNextSymbol().ToDouble(); //wyraz wolny
   fCalibrateIn[i][1]=GetNextSymbol().ToDouble(); //mno¿nik
   fCalibrateIn[i][2]=GetNextSymbol().ToDouble(); //mno¿nik dla kwadratu
   fCalibrateIn[i][3]=GetNextSymbol().ToDouble(); //mno¿nik dla szeœcianu
  }
  else if (str==AnsiString("calibrateout")) //parametry kalibracji wyjœæ
  {//
   i=GetNextSymbol().ToIntDef(-1); //numer wejœcia
   if ((i<0)||(i>6)) i=6; //na ostatni, bo i tak trzeba pomin¹æ wartoœci
   fCalibrateOut[i][0]=GetNextSymbol().ToDouble(); //wyraz wolny
   fCalibrateOut[i][1]=GetNextSymbol().ToDouble(); //mno¿nik liniowy
   fCalibrateOut[i][2]=GetNextSymbol().ToDouble(); //mno¿nik dla kwadratu
   fCalibrateOut[i][3]=GetNextSymbol().ToDouble(); //mno¿nik dla szeœcianu
  }
  else if (str==AnsiString("brakestep")) //krok zmiany hamulca dla klawiszy [Num3] i [Num9]
   fBrakeStep=GetNextSymbol().ToDouble();
  else if (str==AnsiString("joinduplicatedevents")) //czy grupowaæ eventy o tych samych nazwach
   bJoinEvents=(GetNextSymbol().LowerCase()==AnsiString("yes"));
  else if (str==AnsiString("pause")) //czy po wczytaniu ma byæ pauza?
   iPause|=(GetNextSymbol().LowerCase()==AnsiString("yes"))?1:0;
  else if (str==AnsiString("lang"))
   asLang=GetNextSymbol(); //domyœlny jêzyk - http://tools.ietf.org/html/bcp47
 }
 while (str!="endconfig"); //(!Parser->EndOfFile)
 //na koniec trochê zale¿noœci
 if (!bLoadTraction) //wczytywanie drutów i s³upów
 {//tutaj wy³¹czenie, bo mog¹ nie byæ zdefiniowane w INI
  bEnableTraction=false; //false = pantograf siê nie po³amie
  bLiveTraction=false; //false = pantografy zawsze zbieraj¹ 95% MaxVoltage
 }
 //if (fMoveLight>0) bDoubleAmbient=false; //wtedy tylko jedno œwiat³o ruchome
 //if (fOpenGL<1.3) iMultisampling=0; //mo¿na by z góry wy³¹czyæ, ale nie mamy jeszcze fOpenGL
 if (iMultisampling)
 {//antyaliasing ca³oekranowy wy³¹cza rozmywanie drutów
  bSmoothTraction=false;
 }
 if (iMultiplayer>0)
  bInactivePause=false; //pauza nieaktywna, jeœli w³¹czona komunikacja
 Console::ModeSet(iFeedbackMode,iFeedbackPort); //tryb pracy konsoli sterowniczej
 fFpsMin=fFpsAverage-fFpsDeviation; //dolna granica FPS, przy której promieñ scenerii bêdzie zmniejszany
 fFpsMax=fFpsAverage+fFpsDeviation; //górna granica FPS, przy której promieñ scenerii bêdzie zwiêkszany
 iFpsRadiusMax=0.000025*fFpsRadiusMax*fFpsRadiusMax; //maksymalny promieñ renderowania 3000.0 -> 225
 if (iFpsRadiusMax>400) iFpsRadiusMax=400;
 if (iPause) iTextMode=VK_F1; //jak pauza, to pokazaæ zegar
 fDistanceFactor/=iWindowHeight; //>1.0 dla rozdzielczoœci mniejszych ni¿ bazowa; doliczyæ wspó³czynnik multisamplingu
 fDistanceFactor*=fDistanceFactor; //do kwadratu, bo wiêkszoœæ odleg³oœci to ich kwadraty 
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

TDynamicObject* __fastcall Global::DynamicNearest()
{//ustalenie pojazdu najbli¿szego kamerze
 return pGround->DynamicNearest(pCamera->Pos);
};

bool __fastcall Global::AddToQuery(TEvent *event,TDynamicObject *who)
{
 return pGround->AddToQuery(event,who);
};
//---------------------------------------------------------------------------

__fastcall TTranscripts::TTranscripts()
{
 iCount=0; //brak linijek do wyœwietlenia
 iStart=0; //wype³niaæ od linijki 0
 for (int i=0;i<MAX_TRANSCRIPTS;++i)
 {//to do konstruktora mo¿na by daæ
  aLines[i].fHide=-1.0; //wolna pozycja (czas symulacji, 360.0 to doba)
  aLines[i].iNext=-1; //nie ma kolejnej
 }
 fRefreshTime=360.0; //wartoœc zaporowa
};
__fastcall TTranscripts::~TTranscripts()
{
};
void __fastcall TTranscripts::AddLine(char *txt,float show,float hide,bool it)
{//dodanie linii do tabeli, (show) i (hide) w [s] od aktualnego czasu
 if (show==hide) return; //komentarz jest ignorowany
 show=Global::fTimeAngleDeg+show/240.0; //jeœli doba to 360, to 1s bêdzie równe 1/240
 hide=Global::fTimeAngleDeg+hide/240.0;
 int i=iStart,j,k; //od czegoœ trzeba zacz¹æ
 while ((aLines[i].iNext>=0)?(aLines[aLines[i].iNext].fShow<=show):false) //póki nie koniec i wczeœniej puszczane
  i=aLines[i].iNext; //przejœcie do kolejnej linijki
 //(i) wskazuje na liniê, po której nale¿y wstawiæ dany tekst, chyba ¿e
 while (txt?*txt:false)
  for (j=0;j<MAX_TRANSCRIPTS;++j)
   if (aLines[j].fHide<0.0)
   {//znaleziony pierwszy wolny
    aLines[j].iNext=aLines[i].iNext; //dotychczasowy nastêpny bêdzie za nowym
    if (aLines[iStart].fHide<0.0) //jeœli tablica jest pusta
     iStart=j; //fHide trzeba sprawdziæ przed ewentualnym nadpisaniem, gdy i=j=0
    else
     aLines[i].iNext=j; //a nowy bêdzie za tamtym wczeœniejszym
    aLines[j].fShow=show; //wyœwietlaæ od
    aLines[j].fHide=hide; //wyœwietlaæ do
    aLines[j].bItalic=it;
    aLines[j].asText=AnsiString(txt); //bez sensu, wystarczy³by wskaŸnik
    if ((k=aLines[j].asText.Pos("|"))>0)
    {//jak jest podzia³ linijki na wiersze
     aLines[j].asText=aLines[j].asText.SubString(1,k-1);
     txt+=k;
     i=j; //kolejna linijka dopisywana bêdzie na koniec w³aœnie dodanej
    }
    else
     txt=NULL; //koniec dodawania
    if (fRefreshTime>show) //jeœli odœwie¿acz ustawiony jest na póŸniej
     fRefreshTime=show; //to odœwie¿yæ wczeœniej
    break; //wiêcej ju¿ nic
   }
};
void __fastcall TTranscripts::Add(char *txt,float len,bool backgorund)
{//dodanie tekstów, d³ugoœæ dŸwiêku, czy istotne
 if (!txt) return; //pusty tekst
 int i=0,j=int(0.5+10.0*len); //[0.1s]
 if (*txt=='[')
 {//powinny byæ dwa nawiasy
  while (*++txt?*txt!=']':false)
   if ((*txt>='0')&&(*txt<='9'))
    i=10*i+int(*txt-'0'); //pierwsza liczba a¿ do ]
  if (*txt?*++txt=='[':false)
  {j=0; //drugi nawias okreœla czas zakoñczenia wyœwietlania
   while (*++txt?*txt!=']':false)
    if ((*txt>='0')&&(*txt<='9'))
     j=10*j+int(*txt-'0'); //druga liczba a¿ do ]
   if (*txt) ++txt; //pominiêcie drugiego ]
  }
 }
 AddLine(txt,0.1*i,0.1*j,false);
};
void __fastcall TTranscripts::Update()
{//usuwanie niepotrzebnych (nie czêœciej ni¿ 10 razy na sekundê)
 if (fRefreshTime>Global::fTimeAngleDeg)
  return; //nie czas jeszcze na zmiany
 //czas odœwie¿enia mo¿na ustaliæ wg tabelki, kiedy coœ siê w niej zmienia
 fRefreshTime=Global::fTimeAngleDeg+360.0; //wartoœæ zaporowa
 int i=iStart,j=-1; //od czegoœ trzeba zacz¹æ
 bool change=false; //czy zmieniaæ napisy?
 do
 {
  if (aLines[i].fHide>=0.0) //o ile aktywne
   if (aLines[i].fHide<Global::fTimeAngleDeg)
   {//gdy czas wyœwietlania up³yn¹³
    aLines[i].fHide=-1.0; //teraz bêdzie woln¹ pozycj¹
    if (i==iStart)
     iStart=aLines[i].iNext>=0?aLines[i].iNext:0; //przestawienie pierwszego
    else if (j>=0)
     aLines[j].iNext=aLines[i].iNext; //usuniêcie ze œrodka
    change=true;
   }
   else
   {//gdy ma byæ pokazane
    if (aLines[i].fShow>Global::fTimeAngleDeg) //bêdzie pokazane w przysz³oœci
     if (fRefreshTime>aLines[i].fShow) //a nie ma nic wczeœniej
      fRefreshTime=aLines[i].fShow;
    if (fRefreshTime>aLines[i].fHide)
     fRefreshTime=aLines[i].fHide;
   }
  //mo¿na by jeszcze wykrywaæ, które nowe maj¹ byæ pokazane
  j=i;
  i=aLines[i].iNext; //kolejna linijka
 } while (i>=0); //póki po tablicy
 change=true; //bo na razie nie ma warunku, ¿e coœ siê doda³o
 if (change)
 {//aktualizacja linijek ekranowych
  i=iStart; j=-1;
  do
  {
   if (aLines[i].fHide>0.0) //jeœli nie ukryte
    if (aLines[i].fShow<Global::fTimeAngleDeg) //to dodanie linijki do wyœwietlania
     if (j<5-1) //ograniczona liczba linijek
      Global::asTranscript[++j]=aLines[i].asText; //skopiowanie tekstu
   i=aLines[i].iNext; //kolejna linijka
  } while (i>=0); //póki po tablicy
  for (++j;j<5;++j)
   Global::asTranscript[j]=""; //i czyszczenie nieu¿ywanych linijek
 }
};

#pragma package(smart_init)
