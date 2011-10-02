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
#include "ai_driver.hpp"
#include "Feedback.h"
#include <Controls.hpp>


//namespace Global {
int Global::Keys[MaxKeys];
vector3 Global::pCameraPosition;
double Global::pCameraRotation;
double Global::pCameraRotationDeg;
vector3 Global::pFreeCameraInit[10];
vector3 Global::pFreeCameraInitAngle[10];
int Global::iWindowWidth=800;
int Global::iWindowHeight=600;
int Global::iBpp=32;
bool Global::bFullScreen=false;
bool Global::bFreeFly=false;
bool Global::bWireFrame=false;
bool Global::bTimeChange=false;
bool Global::bSoundEnabled=true;
bool Global::bRenderAlpha=true; //Ra: wywalam t� flag�
bool Global::bWriteLogEnabled=true;
bool Global::bAdjustScreenFreq=true;
bool Global::bEnableTraction=true;
bool Global::bLoadTraction=true;
bool Global::bLiveTraction=true;
bool Global::bManageNodes=true;
bool Global::bnewAirCouplers=true;
bool Global::bDecompressDDS=false;

//bool Global::WFreeFly=false;
float Global::fMouseXScale=3.2;
float Global::fMouseYScale=0.5;
double Global::fFogStart=1300;
double Global::fFogEnd=2000;
//double Global::tSinceStart=0;
GLfloat Global::AtmoColor[]={0.6f,0.7f,0.8f};
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
TGround *Global::pGround=NULL;
//char Global::CreatorName1[30]="Maciej Czapkiewicz";
//char Global::CreatorName2[30]="Marcin Wozniak <Marcin_EU>";
//char Global::CreatorName3[20]="Adam Bugiel <ABu>";
//char Global::CreatorName4[30]="Arkadiusz Slusarczyk <Winger>";
//char Global::CreatorName5[30]="Lukasz Kirchner <Nbmx>";
char Global::szSceneryFile[256]="td.scn";
AnsiString Global::asCurrentSceneryPath="scenery/";
AnsiString Global::asHumanCtrlVehicle="EU07-424";
AnsiString Global::asCurrentTexturePath=AnsiString(szDefaultTexturePath);
int Global::iSlowMotion=0; //info o malym FPS: 0-OK, 1-wy��czy� multisampling 3-zmniejszenie promienia
bool Global::changeDynObj; //info o zmianie pojazdu
bool Global::detonatoryOK; //info o nowych detonatorach
double Global::ABuDebug=0;
AnsiString Global::asSky="1";
int Global::iDefaultFiltering=9; //domy�lne rozmywanie tekstur TGA bez alfa
int Global::iBallastFiltering=9; //domy�lne rozmywanie tekstur podsypki
int Global::iRailProFiltering=5; //domy�lne rozmywanie tekstur szyn
int Global::iDynamicFiltering=5; //domy�lne rozmywanie tekstur pojazd�w
bool Global::bReCompile=false; //czy od�wie�y� siatki
bool Global::bUseVBO=false; //czy jest VBO w karcie graficznej
int Global::iFeedbackMode=1; //tryb pracy informacji zwrotnej
double Global::fOpenGL=0.0; //wersja OpenGL - przyda si�
bool Global::bOpenGL_1_5=false; //czy s� dost�pne funkcje OpenGL 1.5
double Global::fLuminance=1.0; //jasno�� �wiat�a do automatycznego zapalania
int Global::iMultiplayer=0; //blokada dzia�ania niekt�rych funkcji na rzecz kominikacji
HWND Global::hWnd=NULL; //uchwyt okna
int Global::iCameraLast=-1;
AnsiString Global::asVersion="Compilation 2011-10-02, release 1.3.256.236."; //tutaj, bo wysy�any
int Global::iViewMode=0; //co aktualnie wida�: 0-kabina, 1-latanie, 2-sprz�gi, 3-dokumenty
GLint Global::iMaxTextureSize=16384;//maksymalny rozmiar tekstury
int Global::iTextMode=0; //tryb pracy wy�wietlacza tekstowego
bool Global::bDoubleAmbient=true; //podw�jna jasno�� ambient
double Global::fMoveLight=-1; //ruchome �wiat�o
bool Global::bSmoothTraction=false; //wyg�adzanie drut�w
double Global::fSunDeclination=0.0; //deklinacja S�o�ca
double Global::fSunSpeed=1.0; //pr�dko�� ruchu S�o�ca, zmienna do test�w
double Global::fTimeAngleDeg=0.0; //godzina w postaci k�ta
double Global::fLatitudeDeg=52.0; //szeroko�� geograficzna
char* Global::szTexturesTGA[4]={"tga","dds","tex","bmp"}; //lista tekstur od TGA
char* Global::szTexturesDDS[4]={"dds","tga","tex","bmp"}; //lista tekstur od DDS
char** Global::szDefaultExt=Global::szTexturesDDS; //domy�lnie od DDS
int Global::iMultisampling=2; //tryb antyaliasingu: 0=brak,1=2px,2=4px,3=8px,4=16px
bool Global::bGlutFont=false; //tekst generowany przez GLUT
int Global::iKeyLast=0; //ostatnio naci�ni�ty klawisz w celu logowania
GLuint Global::iTextureId=0; //ostatnio u�yta tekstura 2D
bool Global::bPause=false; //globalna pauza ruchu
bool Global::bActive=true; //czy jest aktywnym oknem
int Global::iConvertModels=2; //tworzenie plik�w binarnych, 2-optymalizacja transform�w
int Global::iErorrCounter=0; //licznik sprawdza� do �ledzenia b��d�w OpenGL
bool Global::bInactivePause=true; //automatyczna pauza, gdy okno nieaktywne
int Global::iTextures=0; //licznik u�ytych tekstur
int Global::iSlowMotionMask=-1; //maska wy��czanych w�a�ciwo�ci

/* Ra: trzeba by przerobi� na cParser, �eby to dzia�a�o w scenerii
void __fastcall Global::ParseConfig(TQueryParserComp *Parser)
{
};
*/
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
         str=Parser->GetNextSymbol();
         bWriteLogEnabled=(str.LowerCase()==AnsiString("yes"));
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
          szDefaultExt=szTexturesTGA; //domy�lnie od TGA
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
        else if (str==AnsiString("feedbackmode"))
         iFeedbackMode=Parser->GetNextSymbol().ToIntDef(1); //domy�lnie 1
        else if (str==AnsiString("multiplayer"))
         iMultiplayer=Parser->GetNextSymbol().ToIntDef(0); //domy�lnie 0
        else if (str==AnsiString("maxtexturesize"))
        {//wymuszenie przeskalowania tekstur
         i=Parser->GetNextSymbol().ToIntDef(16384); //domy�lnie du�e
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
         bDoubleAmbient=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else if (str==AnsiString("movelight")) //numer dnia w roku albo -1
        {fMoveLight=Parser->GetNextSymbol().ToIntDef(-1); //numer dnia 1..365
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
          //Declination=((0.322003-22.971*cos(t)-0.357898*cos(2*t)-0.14398*cos(3*t)+3.94638*sin(t)+0.019334*sin(2*t)+0.05928*sin(3*t)))*Pi/180
          //fSunDeclination=0.005620-0.4009196*cos(  fMoveLight)+0.0688773*sin(  fMoveLight)
          //                        -0.0062465*cos(2*fMoveLight)+0.0003374*sin(2*fMoveLight)
          //                        -0.0025129*cos(3*fMoveLight)+0.0010346*sin(3*fMoveLight);
         }
        }
        else if (str==AnsiString("smoothtraction")) //podw�jna jasno�� ambient
         bSmoothTraction=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else if (str==AnsiString("sunspeed")) //pr�dko�� ruchu S�o�ca, zmienna do test�w
         fSunSpeed=Parser->GetNextSymbol().ToIntDef(1);
        else if (str==AnsiString("multisampling")) //tryb antyaliasingu: 0=brak,1=2px,2=4px
         iMultisampling=Parser->GetNextSymbol().ToIntDef(2); //domy�lnie 2
        else if (str==AnsiString("glutfont")) //tekst generowany przez GLUT
         bGlutFont=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else if (str==AnsiString("latitude")) //szeroko�� geograficzna
         fLatitudeDeg=Parser->GetNextSymbol().ToDouble();
        else if (str==AnsiString("convertmodels")) //tworzenie plik�w binarnych
         iConvertModels=Parser->GetNextSymbol().ToIntDef(2); //domy�lnie 2
        else if (str==AnsiString("inactivepause")) //automatyczna pauza, gdy okno nieaktywne
         bInactivePause=(Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else if (str==AnsiString("slowmotion")) //tworzenie plik�w binarnych
         iSlowMotionMask=Parser->GetNextSymbol().ToIntDef(-1); //domy�lnie -1
    }
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
 Feedback::ModeSet(iFeedbackMode); //tryb pracy interfejsu zwrotnego
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
        Keys[k_IncLocalBrakeLevelFAST]=VK_END;  //VK_HOME;
        Keys[k_DecLocalBrakeLevel]=VK_NUMPAD7;  //VK_NUMPAD1;
        Keys[k_DecLocalBrakeLevelFAST]=VK_HOME; //VK_END;
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
        Keys[k_PantFrontUp]=VkKeyScan('o');
        Keys[k_PantFrontDown]=VkKeyScan('o');
        Keys[k_PantRearUp]=VkKeyScan('p');
        Keys[k_PantRearDown]=VkKeyScan('p');
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


//---------------------------------------------------------------------------

#pragma package(smart_init)
