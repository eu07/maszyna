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
#include "mover.hpp"
#include "Driver.h"
#include "Feedback.h"
#include <Controls.hpp>


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
AnsiString Global::asSkyH="1";
double Global::fOpenGL=0.0; //wersja OpenGL - do sprawdzania obecnoœci rozszerzeñ
bool Global::bOpenGL_1_5=false; //czy s¹ dostêpne funkcje OpenGL 1.5
double Global::fLuminance=1.0; //jasnoœæ œwiat³a do automatycznego zapalania
int Global::iReCompile=0; //zwiêkszany, gdy trzeba odœwie¿yæ siatki
HWND Global::hWnd=NULL; //uchwyt okna
int Global::iCameraLast=-1;
AnsiString Global::asVersion="Compilation 2011-12-26, release 1.6.291.294."; //tutaj, bo wysy³any
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

//parametry scenerii
vector3 Global::pCameraPosition;
vector3 Global::pCameraRot;
double Global::pCameraRotation;
double Global::pCameraRotationDeg;
vector3 Global::pFreeCameraInit[10];
vector3 Global::pFreeCameraInitAngle[10];
double Global::fFogStart=1300;
double Global::fFogEnd=2000;
GLfloat Global::AtmoColor[]={0.6f,0.7f,0.8f};
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
GLfloat Global::SUNFLPos[4];
GLfloat Global::light3Pos[4];
GLfloat Global::light4Pos[4];
GLfloat Global::light5Pos[4];
GLfloat Global::light6Pos[4];

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

//parametry testowe (do testowania scenerii i obiektów)
bool Global::bWireFrame=false;
bool Global::bSoundEnabled=true;
bool Global::bWriteLogEnabled=false;
bool Global::bManageNodes=true;
bool Global::bDecompressDDS=false;

//parametry przejœciowe (do usuniêcia)
bool Global::bTimeChange=false; //Ra: ZiomalCl wy³¹czy³ star¹ wersjê nocy
bool Global::bRenderAlpha=true; //Ra: wywali³am tê flagê
bool Global::bnewAirCouplers=true;
bool Global::bDoubleAmbient=true; //podwójna jasnoœæ ambient
double Global::fSunSpeed=1.0; //prêdkoœæ ruchu S³oñca, zmienna do testów


// QUEUEDEU VARS  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

TStringList *Global::OTHERS;
TStringList *Global::CHANGES;
TStringList *Global::KEYBOARD;
TStringList *Global::CONSISTF;
TStringList *Global::CONSISTB;
TStringList *Global::CONSISTA;

AnsiString Global::g___APPDIR = "";
AnsiString Global::asSSHOTDIR = "";
AnsiString Global::asSSHOTSUBDIR = "99\\";
AnsiString Global::APPVERS = "";
AnsiString Global::APPVER2 = "";
AnsiString Global::APPDATE = "";
AnsiString Global::APPDAT2 = "";
AnsiString Global::APPSIZE = "";
AnsiString Global::APPCOMP = "";
AnsiString Global::asMechanikVech = "";
AnsiString Global::asCurrentNodeName = "none";
AnsiString Global::asCurrentIncName = "none";
AnsiString Global::asKeyboardFile = "none";
AnsiString Global::gtime = "000000";
AnsiString Global::timeh = "00";
AnsiString Global::timem = "00";
AnsiString Global::times = "00";
AnsiString Global::asnearestdyn= "";
AnsiString Global::asnearestobj= "";
AnsiString Global::asSCREENSHOTFILE = "";
AnsiString Global::asWINTITLE = "";

bool Global::SCNLOADED = false;
bool Global::bQueuedAdvLog = false;
bool Global::scrfilter = false;
bool Global::bpanview = false;
bool Global::bTUTORIAL = false;
bool Global::bCONSOLE = false;
bool Global::bnearestengaged = false;
bool Global::bnearestobjengaged = false;
bool Global::bfirstloadingscn = true;
bool Global::bloaderbriefing = true;
bool Global::bRenderSky= true;
bool Global::problend = false;
bool Global::seekfacenormal = true;
bool Global::makeQ3D = false;
bool Global::bloadnormals = false;
bool Global::bsemlenses = false;
bool Global::bswitchpod = false;
bool Global::blogincludes = false;
bool Global::bignoreinclude = false;
bool Global::bsun = false;
bool Global::bCabALight = false;
bool Global::bSCREENSHOT = false;
bool Global::bSnowEnabled = false;
bool Global::ventilator1on = false;
bool Global::ventilator2on = false;

GLuint Global::loaderbrief;
GLuint Global::loaderlogo;
GLuint Global::bfonttex;
GLuint Global::texmenu_head1;
GLuint Global::texmenu_backg;
GLuint Global::texmenu_setti_0;
GLuint Global::texmenu_setti_1;
GLuint Global::texmenu_start_0;
GLuint Global::texmenu_start_1;
GLuint Global::texmenu_backw;
GLuint Global::texmenu_about_0;
GLuint Global::texmenu_about_1;
GLuint Global::texmenu_exitt_0;
GLuint Global::texmenu_exitt_1;
GLuint Global::texmenu_cbox0;
GLuint Global::texmenu_cbox1;
GLuint Global::texmenu_rbox0;
GLuint Global::texmenu_rbox1;
GLuint Global::texmenu_leave;
GLuint Global::texmenu_write;
GLuint Global::texmenu_okeyy;
GLuint Global::texmenu_point;
GLuint Global::texmenu_editb;
GLuint Global::texmenu_lboxa;
GLuint Global::texmenu_lboxb;
GLuint Global::texmenu_lboxu;
GLuint Global::texmenu_lboxd;
GLuint Global::texmenu_chcon;
GLuint Global::texmenu_adlok;
GLuint Global::texsett_backg;
GLuint Global::texpointer;
GLuint Global::semlense1;
GLuint Global::semlense2;
GLuint Global::texconsole;
GLuint Global::texcompass;
GLuint Global::texcompassarr;
GLuint Global::SCRFILTER;
GLuint Global::texturetab[16];
GLuint Global::texsun1;
GLuint Global::texsun2;
GLuint Global::texsun3;
GLuint Global::m_GlowTexture;
GLuint Global::m_BigGlowTexture;
GLuint Global::m_HaloTexture;
GLuint Global::m_StreakTexture;

GLuint Global::dhcifre_00=NULL;
GLuint Global::dhcifre_01=NULL;
GLuint Global::dhcifre_02=NULL;
GLuint Global::dhcifre_03=NULL;
GLuint Global::dhcifre_04=NULL;
GLuint Global::dhcifre_05=NULL;
GLuint Global::dhcifre_06=NULL;
GLuint Global::dhcifre_07=NULL;
GLuint Global::dhcifre_08=NULL;
GLuint Global::dhcifre_09=NULL;


// 36 ALFANUM CHARS CONTAINER
GLuint Global::cichar01=NULL;
GLuint Global::cichar02=NULL;
GLuint Global::cichar03=NULL;
GLuint Global::cichar04=NULL;
GLuint Global::cichar05=NULL;
GLuint Global::cichar06=NULL;
GLuint Global::cichar07=NULL;
GLuint Global::cichar08=NULL;
GLuint Global::cichar09=NULL;
GLuint Global::cichar10=NULL;
GLuint Global::cichar11=NULL;
GLuint Global::cichar12=NULL;
GLuint Global::cichar13=NULL;
GLuint Global::cichar14=NULL;
GLuint Global::cichar15=NULL;
GLuint Global::cichar16=NULL;
GLuint Global::cichar17=NULL;
GLuint Global::cichar18=NULL;
GLuint Global::cichar19=NULL;
GLuint Global::cichar20=NULL;
GLuint Global::cichar21=NULL;
GLuint Global::cichar22=NULL;
GLuint Global::cichar23=NULL;
GLuint Global::cichar24=NULL;
GLuint Global::cichar25=NULL;
GLuint Global::cichar26=NULL;
GLuint Global::cichar27=NULL;
GLuint Global::cichar28=NULL;
GLuint Global::cichar29=NULL;
GLuint Global::cichar30=NULL;
GLuint Global::cichar31=NULL;
GLuint Global::cichar32=NULL;
GLuint Global::cichar33=NULL;
GLuint Global::cichar34=NULL;
GLuint Global::cichar35=NULL;
GLuint Global::cichar36=NULL;
GLuint Global::cichar37=NULL;
GLuint Global::cichar38=NULL;
GLuint Global::cichar39=NULL;
GLuint Global::cichar40=NULL;
GLuint Global::cichar41=NULL;
GLuint Global::cichar42=NULL;
GLuint Global::cichar43=NULL;
GLuint Global::cichar44=NULL;
GLuint Global::cichar45=NULL;
GLuint Global::cichar46=NULL;
GLuint Global::cichar47=NULL;
GLuint Global::cichar48=NULL;
GLuint Global::cichar49=NULL;
GLuint Global::cichar50=NULL;
GLuint Global::cichar51=NULL;

// 36 ALFANUM CHARS DIRECT TABLE
GLuint Global::dichar01=NULL;
GLuint Global::dichar02=NULL;
GLuint Global::dichar03=NULL;
GLuint Global::dichar04=NULL;
GLuint Global::dichar05=NULL;
GLuint Global::dichar06=NULL;
GLuint Global::dichar07=NULL;
GLuint Global::dichar08=NULL;
GLuint Global::dichar09=NULL;
GLuint Global::dichar10=NULL;
GLuint Global::dichar11=NULL;
GLuint Global::dichar12=NULL;
GLuint Global::dichar13=NULL;
GLuint Global::dichar14=NULL;
GLuint Global::dichar15=NULL;
GLuint Global::dichar16=NULL;
GLuint Global::dichar17=NULL;
GLuint Global::dichar18=NULL;
GLuint Global::dichar19=NULL;
GLuint Global::dichar20=NULL;
GLuint Global::dichar21=NULL;
GLuint Global::dichar22=NULL;
GLuint Global::dichar23=NULL;
GLuint Global::dichar24=NULL;
GLuint Global::dichar25=NULL;
GLuint Global::dichar26=NULL;
GLuint Global::dichar27=NULL;
GLuint Global::dichar28=NULL;
GLuint Global::dichar29=NULL;
GLuint Global::dichar30=NULL;
GLuint Global::dichar31=NULL;
GLuint Global::dichar32=NULL;
GLuint Global::dichar33=NULL;
GLuint Global::dichar34=NULL;
GLuint Global::dichar35=NULL;
GLuint Global::dichar36=NULL;

GLuint Global::cifre_00=NULL;
GLuint Global::cifre_01=NULL;
GLuint Global::cifre_02=NULL;
GLuint Global::cifre_03=NULL;
GLuint Global::cifre_04=NULL;
GLuint Global::cifre_05=NULL;
GLuint Global::cifre_06=NULL;
GLuint Global::cifre_07=NULL;
GLuint Global::cifre_08=NULL;
GLuint Global::cifre_09=NULL;
GLuint Global::charx_01=NULL;
GLuint Global::charx_02=NULL;
GLuint Global::charx_03=NULL;
GLuint Global::charx_04=NULL;
GLuint Global::charx_05=NULL;
GLuint Global::charx_06=NULL;
GLuint Global::charx_07=NULL;

GLuint Global::cdigit01=NULL;
GLuint Global::cdigit02=NULL;
GLuint Global::cdigit03=NULL;
GLuint Global::cdigit04=NULL;
GLuint Global::cdigit05=NULL;
GLuint Global::cdigit06=NULL;
GLuint Global::cdigit07=NULL;
GLuint Global::cdigit08=NULL;
GLuint Global::cdigit09=NULL;
GLuint Global::cdigit10=NULL;



int Global::iNODES = 100000;
int Global::iPARSERBYTESPASSED = 0;
int Global::iPARSERBYTESTOTAL = 0;
int Global::postep = 1;
int Global::infotype = 0;
int Global::aspectratio = 43;
int Global::loaderrefresh = 5;
int Global::iSnowFlakesNum = 26000;

float Global::ffov = 45.0;
float Global::fps = 0;
float Global::upd = 0;
float Global::ren = 0;
float Global::crt = 0;
float Global::gtc1 = 0;
float Global::gtc2 = 0;
float Global::lsec = 0;
float Global::cablightint = 1.0;
float Global::rtim = 0;

AnsiString Global::debuginfo1 = "";
AnsiString Global::debuginfo2 = "";
AnsiString Global::debuginfo3 = "";
AnsiString Global::debuginfo4 = "";
AnsiString Global::debuginfo5 = "";

double Global::panviewtrans= 0.80;
double Global::screenfade= 1.0;
double Global::timepassed;
double Global::railmaxdistance = 170*1000;     // 470m
double Global::podsmaxdistance = 200*1000;     // 400m
double Global::SUNROTX;
double Global::SUNROTY;
double Global::ventilator1acc = 0;
double Global::ventilator2acc = 0;

float Global::consistlen=0;
float Global::consistmass=0;
float Global::consistcars=0;
float Global::flnearestdyndist=0;
float Global::P[20];

/* Ra: trzeba by przerobiæ na cParser, ¿eby to dzia³a³o w scenerii
void __fastcall Global::ParseConfig(TQueryParserComp *Parser)
{
};
*/
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

    while (!Parser->EndOfFile)
    {
        str=Parser->GetNextSymbol().LowerCase();
        if (str==AnsiString("sceneryfile"))
         {
           str=Parser->GetNextSymbol().LowerCase();
           strcpy(szSceneryFile,str.c_str());
         }
        else
        if (str==AnsiString("humanctrlvehicle"))
         {
           str=Parser->GetNextSymbol().LowerCase();
           asHumanCtrlVehicle=str;
         }
        else if (str==AnsiString("width"))
         iWindowWidth=Parser->GetNextSymbol().ToInt();
        else if (str==AnsiString("height"))
         iWindowHeight=Parser->GetNextSymbol().ToInt();
        else if (str==AnsiString("bpp"))
         iBpp=((Parser->GetNextSymbol().LowerCase()==AnsiString("32")) ? 32 : 16 );
        else if (str==AnsiString("fullscreen"))
         bFullScreen=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else if (str==AnsiString("aspectratio"))
         Global::aspectratio=Parser->GetNextSymbol().ToInt();
        else if (str==AnsiString("freefly")) //Mczapkie-130302
        {
         bFreeFly=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
         pFreeCameraInit[0].x=Parser->GetNextSymbol().ToDouble();
         pFreeCameraInit[0].y=Parser->GetNextSymbol().ToDouble();
         pFreeCameraInit[0].z=Parser->GetNextSymbol().ToDouble();
        }
        else if (str==AnsiString("wireframe"))
         bWireFrame=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else if (str==AnsiString("debugmode")) //McZapkie! - DebugModeFlag uzywana w mover.pas, warto tez blokowac cheaty gdy false
         DebugModeFlag=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else if (str==AnsiString("soundenabled")) //McZapkie-040302 - blokada dzwieku - przyda sie do debugowania oraz na komp. bez karty dzw.
         bSoundEnabled=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        //else if (str==AnsiString("renderalpha")) //McZapkie-1312302 - dwuprzebiegowe renderowanie
        // bRenderAlpha=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else if (str==AnsiString("physicslog")) //McZapkie-030402 - logowanie parametrow fizycznych dla kazdego pojazdu z maszynista
         WriteLogFlag=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else if (str==AnsiString("physicsdeactivation")) //McZapkie-291103 - usypianie fizyki
         PhysicActivationFlag=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else if (str==AnsiString("debuglog"))
        {//McZapkie-300402 - wylaczanie log.txt
        // str=Parser->GetNextSymbol();
         bWriteLogEnabled=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        }
        else if (str==AnsiString("adjustscreenfreq"))
        {//McZapkie-240403 - czestotliwosc odswiezania ekranu
         str=Parser->GetNextSymbol();
         bAdjustScreenFreq=(str.LowerCase()==AnsiString("yes"));
        }
        else if (str==AnsiString("mousescale"))
        {//McZapkie-060503 - czulosc ruchu myszy (krecenia glowa)
         str=Parser->GetNextSymbol();
         fMouseXScale=str.ToDouble();
         str=Parser->GetNextSymbol();
         fMouseYScale=str.ToDouble();
        }
        else if (str==AnsiString("enabletraction"))
        {//Winger 040204 - 'zywe' patyki dostosowujace sie do trakcji
         bEnableTraction=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        }
        else if (str==AnsiString("loadtraction"))
        {//Winger 140404 - ladowanie sie trakcji
         bLoadTraction=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        }
        else if (str==AnsiString("livetraction"))
        {//Winger 160404 - zaleznosc napiecia loka od trakcji
         bLiveTraction=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        }
        else if (str==AnsiString("skyenabled"))
        {//youBy - niebo
         if (Parser->GetNextSymbol().LowerCase()==AnsiString("yes"))
          asSky="1"; else asSky="0";
        }
        else if (str==AnsiString("managenodes"))
        {
         bManageNodes=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        }
        else if (str==AnsiString("decompressdds"))
        {
         bDecompressDDS=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        }
// ShaXbee - domyslne rozszerzenie tekstur
        else if (str==AnsiString("defaultext"))
        {str=Parser->GetNextSymbol().LowerCase(); //rozszerzenie
         if (str=="tga")
          szDefaultExt=szTexturesTGA; //domyœlnie od TGA
         //szDefaultExt=std::string(Parser->GetNextSymbol().LowerCase().c_str());
        }
        else if (str==AnsiString("newaircouplers"))
         bnewAirCouplers=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else if (str==AnsiString("defaultfiltering"))
         iDefaultFiltering=Parser->GetNextSymbol().ToIntDef(-1);
        else if (str==AnsiString("ballastfiltering"))
         iBallastFiltering=Parser->GetNextSymbol().ToIntDef(-1);
        else if (str==AnsiString("railprofiltering"))
         iRailProFiltering=Parser->GetNextSymbol().ToIntDef(-1);
        else if (str==AnsiString("dynamicfiltering"))
         iDynamicFiltering=Parser->GetNextSymbol().ToIntDef(-1);
        else if (str==AnsiString("usevbo"))
         bUseVBO=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else if (str==AnsiString("makeq3d"))
         Global::makeQ3D=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else if (str==AnsiString("lenses"))
         Global::bsemlenses=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else if (str==AnsiString("switchpods"))
         Global::bswitchpod=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else if (str==AnsiString("logincludes"))
         Global::bsun=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else if (str==AnsiString("sun"))
         Global::bsun=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else if (str==AnsiString("feedbackmode"))
         iFeedbackMode=Parser->GetNextSymbol().ToIntDef(1); //domyœlnie 1
        else if (str==AnsiString("multiplayer"))
         iMultiplayer=Parser->GetNextSymbol().ToIntDef(0); //domyœlnie 0
        else if (str==AnsiString("maxtexturesize"))
        {//wymuszenie przeskalowania tekstur
         i=Parser->GetNextSymbol().ToIntDef(16384); //domyœlnie du¿e
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
         bDoubleAmbient=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else if (str==AnsiString("movelight")) //numer dnia w roku albo -1
        {fMoveLight=Parser->GetNextSymbol().ToIntDef(-1); //numer dnia 1..365
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
         bSmoothTraction=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else if (str==AnsiString("sunspeed")) //prêdkoœæ ruchu S³oñca, zmienna do testów
         fSunSpeed=Parser->GetNextSymbol().ToIntDef(1);
        else if (str==AnsiString("multisampling")) //tryb antyaliasingu: 0=brak,1=2px,2=4px
         iMultisampling=Parser->GetNextSymbol().ToIntDef(2); //domyœlnie 2
        else if (str==AnsiString("glutfont")) //tekst generowany przez GLUT
         bGlutFont=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else if (str==AnsiString("latitude")) //szerokoœæ geograficzna
         fLatitudeDeg=Parser->GetNextSymbol().ToDouble();
        else if (str==AnsiString("convertmodels")) //tworzenie plików binarnych
         iConvertModels=Parser->GetNextSymbol().ToIntDef(2); //domyœlnie 2
        else if (str==AnsiString("inactivepause")) //automatyczna pauza, gdy okno nieaktywne
         bInactivePause=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else if (str==AnsiString("slowmotion")) //tworzenie plików binarnych
         iSlowMotionMask=Parser->GetNextSymbol().ToIntDef(-1); //domyœlnie -1
        else if (str==AnsiString("modifytga")) //czy korygowaæ pliki TGA dla szybszego wczytywania
         iModifyTGA=Parser->GetNextSymbol().ToIntDef(0); //domyœlnie 0
    }
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
 Feedback::ModeSet(iFeedbackMode); //tryb pracy interfejsu zwrotnego
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
        Keys[k_ProgramPause]=VK_F3;
        Keys[k_ProgramHelp]=VK_F1;
        Keys[k_FreeFlyMode]=VK_F4;
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


//---------------------------------------------------------------------------

#pragma package(smart_init)
