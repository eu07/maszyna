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
#pragma hdrstop


#include "Globals.h"
#include "QueryParserComp.hpp"
#include "usefull.h"
#include "mover.hpp"
#include "ai_driver.hpp"
#include "Feedback.h"


//namespace Global {
int Global::Keys[MaxKeys];
vector3 Global::pCameraPosition;
double Global::pCameraRotation;
vector3 Global::pFreeCameraInit;
vector3 Global::pFreeCameraInitAngle= vector3(0, 0, 0);
int Global::iWindowWidth= 800;
int Global::iWindowHeight= 600;
int Global::iBpp= 16;
bool Global::bFullScreen= true;
bool Global::bFreeFly= false;
bool Global::bWireFrame= false;
bool Global::bTimeChange= false;
bool Global::bSoundEnabled= true;
bool Global::bRenderAlpha= true;
bool Global::bWriteLogEnabled= true;
bool Global::bAdjustScreenFreq= true;
bool Global::bEnableTraction= true;
bool Global::bLoadTraction= true;
bool Global::bLiveTraction= true;
bool Global::bManageNodes = true;
bool Global::bnewAirCouplers= false;
bool Global::bDecompressDDS = true;

//bool Global::WFreeFly= false;
float Global::fMouseXScale=3.2;
float Global::fMouseYScale=0.5;
double Global::fFogStart= 1000;
double Global::fFogEnd= 2000;
//double Global::tSinceStart= 0;
GLfloat  Global::AtmoColor[] = { 0.6f, 0.7f, 0.8f };
GLfloat  Global::FogColor[] = { 0.6f, 0.7f, 0.8f };
GLfloat  Global::ambientDayLight[]  = { 0.40f,  0.40f, 0.45f, 1.0f };
GLfloat  Global::diffuseDayLight[]  = { 0.55f,  0.54f, 0.50f, 1.0f };
GLfloat  Global::specularDayLight[] = { 0.95f,  0.94f, 0.90f, 1.0f };
GLfloat  Global::whiteLight[]    = { 1.0f,  1.0f, 1.0f, 1.0f };
GLfloat  Global::noLight[]    = { 0.0f,  0.0f, 0.0f, 1.0f };
GLfloat  Global::lightPos[4];
TGround *Global::pGround= NULL;
//char Global::CreatorName1[30]= "Maciej Czapkiewicz";
//char Global::CreatorName2[30]= "Marcin Wozniak <Marcin_EU>";
//char Global::CreatorName3[20]= "Adam Bugiel <ABu>";
//char Global::CreatorName4[30]= "Arkadiusz Slusarczyk <Winger>";
//char Global::CreatorName5[30]= "Lukasz Kirchner <Nbmx>";
char Global::szSceneryFile[256]= "scene.scn";
std::string Global::szDefaultExt("dds");
AnsiString Global::asCurrentSceneryPath= "scenery/";
AnsiString Global::asHumanCtrlVehicle= "EU07-424";
AnsiString Global::asCurrentTexturePath=AnsiString(szDefaultTexturePath);
bool Global::slowmotion; //Info o malym FPS... :)
bool Global::changeDynObj; //info o zmianie pojazdu
bool Global::detonatoryOK; //info o nowych detonatorach
double Global::ABuDebug=0;
AnsiString Global::asSky= "1";
int Global::iDefaultFiltering=9; //domyœlne rozmywanie tekstur TGA bez alfa
int Global::iBallastFiltering=9; //domyœlne rozmywanie tekstur podsypki
int Global::iRailProFiltering=6; //domyœlne rozmywanie tekstur szyn
int Global::iDynamicFiltering=5; //domyœlne rozmywanie tekstur pojazdów
bool Global::bReCompile=false; //czy odœwie¿yæ siatki
bool Global::bUseVBO=false; //czy jest VBO w karcie graficznej
int Global::iFeedbackMode=1; //tryb pracy informacji zwrotnej
double Global::fOpenGL=0.0; //wersja OpenGL - przyda siê
bool Global::bOpenGL_1_5=false; //czy s¹ dostêpne funkcje OpenGL 1.5

void __fastcall Global::LoadIniFile(AnsiString asFileName)
{
    TFileStream *fs;
    fs= new TFileStream(asFileName , fmOpenRead	| fmShareCompat	);
    AnsiString str= "";
    int size= fs->Size;
    str.SetLength(size);
    fs->Read(str.c_str(),size);
    str+= "";
    delete fs;
    TQueryParserComp *Parser;
    Parser= new TQueryParserComp(NULL);
    Parser->TextToParse= str;
//    Parser->LoadStringToParse(asFile);
    Parser->First();

    while (!Parser->EndOfFile)
    {
        str= Parser->GetNextSymbol().LowerCase();
        if (str==AnsiString("sceneryfile"))
         {
           str= Parser->GetNextSymbol().LowerCase();
           strcpy(szSceneryFile,str.c_str());
         }
        else
        if (str==AnsiString("humanctrlvehicle"))
         {
           str= Parser->GetNextSymbol().LowerCase();
           asHumanCtrlVehicle= str;
         }
        else
        if (str==AnsiString("width"))
            iWindowWidth= Parser->GetNextSymbol().ToInt();
        else
        if (str==AnsiString("height"))
            iWindowHeight= Parser->GetNextSymbol().ToInt();
        else
        if (str==AnsiString("bpp"))
            iBpp= ((Parser->GetNextSymbol().LowerCase()==AnsiString("32")) ? 32 : 16 );
        else
        if (str==AnsiString("fullscreen"))
            bFullScreen= (Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else
        if (str==AnsiString("freefly")) //Mczapkie-130302
         {
           bFreeFly= (Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
           pFreeCameraInit.x= Parser->GetNextSymbol().ToDouble();
           pFreeCameraInit.y= Parser->GetNextSymbol().ToDouble();
           pFreeCameraInit.z= Parser->GetNextSymbol().ToDouble();
         }
        else
        if (str==AnsiString("wireframe"))
            bWireFrame= (Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
// McZapkie! - DebugModeFlag uzywana w mover.pas, warto tez blokowac cheaty gdy false
        else
        if (str==AnsiString("debugmode"))
            DebugModeFlag= (Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else
//McZapkie-040302 - blokada dzwieku - przyda sie do debugowania oraz na komp. bez karty dzw.
        if (str==AnsiString("soundenabled"))
            bSoundEnabled= (Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else
//McZapkie-1312302 - dwuprzebiegowe renderowanie
        if (str==AnsiString("renderalpha"))
            bRenderAlpha= (Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else
//McZapkie-030402 - logowanie parametrow fizycznych dla kazdego pojazdu z maszynista
        if (str==AnsiString("physicslog"))
            WriteLogFlag= (Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else
//McZapkie-291103 - usypianie fizyki
        if (str==AnsiString("physicsdeactivation"))
            PhysicActivationFlag= (Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
        else
//McZapkie-300402 - wylaczanie log.txt
        if (str==AnsiString("debuglog"))
         {
          str=Parser->GetNextSymbol();
          bWriteLogEnabled= (str.LowerCase()==AnsiString("yes"));
         }
        else
//McZapkie-240403 - czestotliwosc odswiezania ekranu
        if (str==AnsiString("adjustscreenfreq"))
         {
          str=Parser->GetNextSymbol();
          bAdjustScreenFreq= (str.LowerCase()==AnsiString("yes"));
         }
        else
//McZapkie-060503 - czulosc ruchu myszy (krecenia glowa)
        if (str==AnsiString("mousescale"))
         {
          str= Parser->GetNextSymbol();
          fMouseXScale= str.ToDouble();
          str= Parser->GetNextSymbol();
          fMouseYScale= str.ToDouble();
         }
        else
//Winger 040204 - 'zywe' patyki dostosowujace sie do trakcji
        if (str==AnsiString("enabletraction"))
         {
          bEnableTraction= (Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
         }
        else
//Winger 140404 - ladowanie sie trakcji
        if (str==AnsiString("loadtraction"))
         {
          bLoadTraction= (Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
          if (bLoadTraction==false)
           bEnableTraction=false;
         }
        else
//Winger 160404 - zaleznosc napiecia loka od trakcji
        if (str==AnsiString("livetraction"))
         {
          bLiveTraction= (Parser->GetNextSymbol().LowerCase()==AnsiString("yes"));
          if (bLoadTraction==false)
           bLiveTraction=false;
         }
//youBy - niebo
        else if (str==AnsiString("skyenabled"))
         {
          if (Parser->GetNextSymbol().LowerCase()==AnsiString("yes"))
          { asSky="1"; } else { asSky="0"; }
         }

        else if (str==AnsiString("managenodes"))
        {
            bManageNodes = (Parser->GetNextSymbol().LowerCase() == AnsiString("yes"));
        }

        else if (str==AnsiString("decompressdds"))
        {
            bDecompressDDS = (Parser->GetNextSymbol().LowerCase() == AnsiString("yes"));
        }

// ShaXbee - domyslne rozszerzenie tekstur
        else if (str==AnsiString("defaultext"))
            szDefaultExt = std::string(Parser->GetNextSymbol().LowerCase().c_str());

        else if (str==AnsiString("newaircouplers"))
         bnewAirCouplers=true;
        else if (str==AnsiString("defaultfiltering"))
         iDefaultFiltering=Parser->GetNextSymbol().ToIntDef(-1);
        else if (str==AnsiString("ballastfiltering"))
         iBallastFiltering=Parser->GetNextSymbol().ToIntDef(-1);
        else if (str==AnsiString("railprofiltering"))
         iRailProFiltering=Parser->GetNextSymbol().ToIntDef(-1);
        else if (str==AnsiString("dynamicfiltering"))
         iDynamicFiltering=Parser->GetNextSymbol().ToIntDef(-1);
        else if (str==AnsiString("feedbackmode"))
         iFeedbackMode=Parser->GetNextSymbol().ToIntDef(1); //domyœlnie 1
    }

 Feedback::ModeSet(iFeedbackMode); //tryb pracy interfejsu zwrotnego
}

void __fastcall Global::InitKeys(AnsiString asFileName)
{
//    if (FileExists(asFileName))
//    {
//       Error("Chwilowo plik keys.ini nie jest obs³ugiwany. £adujê standardowe ustawienia.\nKeys.ini file is temporarily not functional, loading default keymap...");
/*        TQueryParserComp *Parser;
        Parser= new TQueryParserComp(NULL);
        Parser->LoadStringToParse(asFileName);

        for (int keycount=0; keycount<MaxKeys; keycount++)
         {
          Keys[keycount]= Parser->GetNextSymbol().ToInt();
         }

        delete Parser;
*/
//    }
//    else
    {
        Keys[k_IncMainCtrl]= VK_ADD;
        Keys[k_IncMainCtrlFAST]= VK_ADD;
        Keys[k_DecMainCtrl]= VK_SUBTRACT;
        Keys[k_DecMainCtrlFAST]= VK_SUBTRACT;
        Keys[k_IncScndCtrl]= VK_DIVIDE;
        Keys[k_IncScndCtrlFAST]= VK_DIVIDE;
        Keys[k_DecScndCtrl]= VK_MULTIPLY;
        Keys[k_DecScndCtrlFAST]= VK_MULTIPLY;
///*NORMALNE
        Keys[k_IncLocalBrakeLevel]= VK_NUMPAD1;  //VK_NUMPAD7;
        Keys[k_IncLocalBrakeLevelFAST]= VK_END;  //VK_HOME;
        Keys[k_DecLocalBrakeLevel]= VK_NUMPAD7;  //VK_NUMPAD1;
        Keys[k_DecLocalBrakeLevelFAST]= VK_HOME; //VK_END;
        Keys[k_IncBrakeLevel]= VK_NUMPAD3;  //VK_NUMPAD9;
        Keys[k_DecBrakeLevel]= VK_NUMPAD9;   //VK_NUMPAD3;
        Keys[k_Releaser]= VK_NUMPAD6;
        Keys[k_EmergencyBrake]= VK_NUMPAD0;
        Keys[k_Brake3]= VK_NUMPAD8;
        Keys[k_Brake2]= VK_NUMPAD5;
        Keys[k_Brake1]= VK_NUMPAD2;
        Keys[k_Brake0]= VK_NUMPAD4;
        Keys[k_WaveBrake]= VK_DECIMAL;
//*/
/*MOJE
        Keys[k_IncLocalBrakeLevel]= VK_NUMPAD3;  //VK_NUMPAD7;
        Keys[k_IncLocalBrakeLevelFAST]= VK_NUMPAD3;  //VK_HOME;
        Keys[k_DecLocalBrakeLevel]= VK_DECIMAL;  //VK_NUMPAD1;
        Keys[k_DecLocalBrakeLevelFAST]= VK_DECIMAL; //VK_END;
        Keys[k_IncBrakeLevel]= VK_NUMPAD6;  //VK_NUMPAD9;
        Keys[k_DecBrakeLevel]= VK_NUMPAD9;   //VK_NUMPAD3;
        Keys[k_Releaser]= VK_NUMPAD5;
        Keys[k_EmergencyBrake]= VK_NUMPAD0;
        Keys[k_Brake3]= VK_NUMPAD2;
        Keys[k_Brake2]= VK_NUMPAD1;
        Keys[k_Brake1]= VK_NUMPAD4;
        Keys[k_Brake0]= VK_NUMPAD7;
        Keys[k_WaveBrake]= VK_NUMPAD8;
*/
        Keys[k_AntiSlipping]= VK_RETURN;
        Keys[k_Sand]= VkKeyScan('s');
        Keys[k_Main]= VkKeyScan('m');
        Keys[k_Active]= VkKeyScan('w');
        Keys[k_DirectionForward]= VkKeyScan('d');
        Keys[k_DirectionBackward]= VkKeyScan('r');
        Keys[k_Fuse]= VkKeyScan('n');
        Keys[k_Compressor]= VkKeyScan('c');
        Keys[k_Converter]= VkKeyScan('x');
        Keys[k_MaxCurrent]= VkKeyScan('f');
        Keys[k_CurrentAutoRelay]= VkKeyScan('g');
        Keys[k_BrakeProfile]= VkKeyScan('b');
        Keys[k_CurrentNext]= VkKeyScan('z');

        Keys[k_Czuwak]= VkKeyScan(' ');
        Keys[k_Horn]= VkKeyScan('a');
        Keys[k_Horn2]= VkKeyScan('a');

        Keys[k_FailedEngineCutOff]= VkKeyScan('e');

        Keys[k_MechUp]= VK_PRIOR;
        Keys[k_MechDown]= VK_NEXT ;
        Keys[k_MechLeft]= VK_LEFT ;
        Keys[k_MechRight]= VK_RIGHT;
        Keys[k_MechForward]= VK_UP;
        Keys[k_MechBackward]= VK_DOWN;

        Keys[k_CabForward]= VK_HOME;
        Keys[k_CabBackward]= VK_END;

        Keys[k_Couple]= VK_INSERT;
        Keys[k_DeCouple]= VK_DELETE;

        Keys[k_ProgramQuit]= VK_F10;
        Keys[k_ProgramPause]= VK_F3;
        Keys[k_ProgramHelp]= VK_F1;
        Keys[k_FreeFlyMode]= VK_F4;

        Keys[k_OpenLeft]= VkKeyScan(',');
        Keys[k_OpenRight]= VkKeyScan('.');
        Keys[k_CloseLeft]= VkKeyScan(',');
        Keys[k_CloseRight]= VkKeyScan('.');
        Keys[k_DepartureSignal]= VkKeyScan('/');

//Winger 160204 - obsluga pantografow
        Keys[k_PantFrontUp]= VkKeyScan('o');
        Keys[k_PantFrontDown]= VkKeyScan('o');
        Keys[k_PantRearUp]= VkKeyScan('p');
        Keys[k_PantRearDown]= VkKeyScan('p');
//Winger 020304 - ogrzewanie
        Keys[k_Heating]= VkKeyScan('h');
        Keys[k_LeftSign]= VkKeyScan('y');
        Keys[k_UpperSign]= VkKeyScan('u');
        Keys[k_RightSign]= VkKeyScan('i');
        Keys[k_EndSign]= VkKeyScan('t');

        Keys[k_SmallCompressor]= VkKeyScan('v');
        Keys[k_StLinOff]= VkKeyScan('l');
//ABu 090305 - przyciski uniwersalne, do roznych bajerow :)
        Keys[k_Univ1] = VkKeyScan('[');
        Keys[k_Univ2]= VkKeyScan(']');
        Keys[k_Univ3] = VkKeyScan(';');
        Keys[k_Univ4] = VkKeyScan('\'');
    }
}

vector3 __fastcall Global::GetCameraPosition()
{
    return pCameraPosition;
}

void __fastcall Global::SetCameraPosition(vector3 pNewCameraPosition)
{
    pCameraPosition= pNewCameraPosition;
}

void __fastcall Global::SetCameraRotation(double Yaw)
{
    pCameraRotation= Yaw;
}

//}

//---------------------------------------------------------------------------

#pragma package(smart_init)
