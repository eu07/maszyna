/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak, Maciej Czapkiewicz and others

*/
#include "stdafx.h"
#include "globals.h"

#include "world.h"
#include "simulation.h"
#include "logs.h"
#include "Console.h"
#include "PyInt.h"

global_settings Global;

void
global_settings::LoadIniFile(std::string asFileName) {

    FreeCameraInit.resize( 10 );
    FreeCameraInitAngle.resize( 10 );
    cParser parser(asFileName, cParser::buffer_FILE);
    ConfigParse(parser);
};

void
global_settings::ConfigParse(cParser &Parser) {

    std::string token;
    do
    {

        token = "";
        Parser.getTokens();
        Parser >> token;

        if (token == "sceneryfile")
        {

            Parser.getTokens();
            Parser >> SceneryFile;
        }
        else if (token == "humanctrlvehicle")
        {

            Parser.getTokens();
            Parser >> asHumanCtrlVehicle;
        }
        else if( token == "fieldofview" ) {

            Parser.getTokens( 1, false );
            Parser >> FieldOfView;
            // guard against incorrect values
            FieldOfView = clamp( FieldOfView, 15.0f, 75.0f );
        }
        else if (token == "width")
        {

            Parser.getTokens(1, false);
            Parser >> iWindowWidth;
        }
        else if (token == "height")
        {

            Parser.getTokens(1, false);
            Parser >> iWindowHeight;
        }
        else if (token == "heightbase")
        {

            Parser.getTokens(1, false);
            Parser >> fDistanceFactor;
        }
        else if (token == "fullscreen")
        {

            Parser.getTokens();
            Parser >> bFullScreen;
        }
        else if( token == "vsync" ) {

            Parser.getTokens();
            Parser >> VSync;
        }
        else if (token == "freefly")
        { // Mczapkie-130302

            Parser.getTokens();
            Parser >> bFreeFly;
            Parser.getTokens(3, false);
            Parser >>
                FreeCameraInit[0].x,
                FreeCameraInit[0].y,
                FreeCameraInit[0].z;
        }
        else if (token == "wireframe")
        {

            Parser.getTokens();
            Parser >> bWireFrame;
        }
        else if (token == "debugmode")
        { // McZapkie! - DebugModeFlag uzywana w mover.pas,
            // warto tez blokowac cheaty gdy false
            Parser.getTokens();
            Parser >> DebugModeFlag;
        }
        else if (token == "soundenabled")
        { // McZapkie-040302 - blokada dzwieku - przyda
            // sie do debugowania oraz na komp. bez karty
            // dzw.
            Parser.getTokens();
            Parser >> bSoundEnabled;
        }
        else if( token == "sound.openal.renderer" ) {
            // selected device for audio renderer
            AudioRenderer = Parser.getToken<std::string>( false ); // case-sensitive
        }
        else if( token == "sound.volume" ) {
            // selected device for audio renderer
            Parser.getTokens();
            Parser >> AudioVolume;
            AudioVolume = clamp( AudioVolume, 1.f, 4.f );
        }
        // else if (str==AnsiString("renderalpha")) //McZapkie-1312302 - dwuprzebiegowe renderowanie
        // bRenderAlpha=(GetNextSymbol().LowerCase()==AnsiString("yes"));
        else if (token == "physicslog")
        { // McZapkie-030402 - logowanie parametrow
            // fizycznych dla kazdego pojazdu z maszynista
            Parser.getTokens();
            Parser >> WriteLogFlag;
        }
        else if (token == "fullphysics")
        { // McZapkie-291103 - usypianie fizyki

            Parser.getTokens();
            Parser >> FullPhysics;
        }
        else if (token == "debuglog")
        {
            // McZapkie-300402 - wylaczanie log.txt
            Parser.getTokens();
            Parser >> token;
            if (token == "yes")
            {
                iWriteLogEnabled = 3;
            }
            else if (token == "no")
            {
                iWriteLogEnabled = 0;
            }
            else
            {
                iWriteLogEnabled = stol_def(token,3);
            }
        }
        else if( token == "multiplelogs" ) {
            Parser.getTokens();
            Parser >> MultipleLogs;
        }
        else if( token == "logs.filter" ) {
            Parser.getTokens();
            Parser >> DisabledLogTypes;
        }
        else if( token == "adjustscreenfreq" )
        {
            // McZapkie-240403 - czestotliwosc odswiezania ekranu
            Parser.getTokens();
            Parser >> bAdjustScreenFreq;
        }
        else if (token == "mousescale")
        {
            // McZapkie-060503 - czulosc ruchu myszy (krecenia glowa)
            Parser.getTokens(2, false);
            Parser >> fMouseXScale >> fMouseYScale;
        }
        else if( token == "mousecontrol" ) {
            // whether control pick mode can be activated
            Parser.getTokens();
            Parser >> InputMouse;
        }
        else if (token == "enabletraction")
        {
            // Winger 040204 - 'zywe' patyki dostosowujace sie do trakcji; Ra 2014-03: teraz łamanie
            Parser.getTokens();
            Parser >> bEnableTraction;
        }
        else if (token == "loadtraction")
        {
            // Winger 140404 - ladowanie sie trakcji
            Parser.getTokens();
            Parser >> bLoadTraction;
        }
        else if (token == "friction")
        { // mnożnik tarcia - KURS90

            Parser.getTokens(1, false);
            Parser >> fFriction;
        }
        else if (token == "livetraction")
        {
            // Winger 160404 - zaleznosc napiecia loka od trakcji;
            // Ra 2014-03: teraz prąd przy braku sieci
            Parser.getTokens();
            Parser >> bLiveTraction;
        }
        else if (token == "skyenabled")
        {
            // youBy - niebo
            Parser.getTokens();
            Parser >> token;
            asSky = (token == "yes" ? "1" : "0");
        }
        else if (token == "defaultext")
        {
            // ShaXbee - domyslne rozszerzenie tekstur
            Parser.getTokens();
            Parser >> token;
            if (token == "tga")
            {
                // domyślnie od TGA
                szDefaultExt = szTexturesTGA;
            }
            else {
                szDefaultExt =
                    ( token[ 0 ] == '.' ?
                        token :
                        "." + token );
            }
        }
        else if (token == "newaircouplers")
        {

            Parser.getTokens();
            Parser >> bnewAirCouplers;
        }
        else if( token == "anisotropicfiltering" ) {

            Parser.getTokens( 1, false );
            Parser >> AnisotropicFiltering;
        }
        else if( token == "usevbo" )
        {

            Parser.getTokens();
            Parser >> bUseVBO;
        }
        else if (token == "feedbackmode")
        {

            Parser.getTokens(1, false);
            Parser >> iFeedbackMode;
        }
        else if (token == "feedbackport")
        {

            Parser.getTokens(1, false);
            Parser >> iFeedbackPort;
        }
        else if (token == "multiplayer")
        {

            Parser.getTokens(1, false);
            Parser >> iMultiplayer;
        }
        else if (token == "maxtexturesize")
        {
            // wymuszenie przeskalowania tekstur
            Parser.getTokens(1, false);
            int size;
            Parser >> size;
                 if (size <= 64)   { iMaxTextureSize = 64; }
            else if (size <= 128)  { iMaxTextureSize = 128; }
            else if (size <= 256)  { iMaxTextureSize = 256; }
            else if (size <= 512)  { iMaxTextureSize = 512; }
            else if (size <= 1024) { iMaxTextureSize = 1024; }
            else if (size <= 2048) { iMaxTextureSize = 2048; }
            else if (size <= 4096) { iMaxTextureSize = 4096; }
            else if (size <= 8192) { iMaxTextureSize = 8192; }
            else                   { iMaxTextureSize = 16384; }
        }
        else if (token == "movelight")
        {
            // numer dnia w roku albo -1
            Parser.getTokens(1, false);
            Parser >> fMoveLight;
            if (fMoveLight == 0.f)
            { // pobranie daty z systemu
                std::time_t timenow = std::time(0);
                std::tm *localtime = std::localtime(&timenow);
                fMoveLight = localtime->tm_yday + 1; // numer bieżącego dnia w roku
            }
            pWorld->compute_season( fMoveLight );
        }
        else if( token == "dynamiclights" ) {
            // number of dynamic lights in the scene
            Parser.getTokens( 1, false );
            Parser >> DynamicLightCount;
            // clamp the light number
            // max 8 lights per opengl specs, minus one used for sun. at least one light for controlled vehicle
            DynamicLightCount = clamp( DynamicLightCount, 1, 7 ); 
        }
        else if( token == "scalespeculars" ) {
            // whether strength of specular highlights should be adjusted (generally needed for legacy 3d models)
            Parser.getTokens();
            Parser >> ScaleSpecularValues;
        }
        else if( token == "gfxrenderer" ) {
            // shadow render toggle
            std::string gfxrenderer;
            Parser.getTokens();
            Parser >> gfxrenderer;
            BasicRenderer = ( gfxrenderer == "simple" );
        }
        else if( token == "shadows" ) {
            // shadow render toggle
            Parser.getTokens();
            Parser >> RenderShadows;
        }
        else if( token == "shadowtune" ) {
            Parser.getTokens( 4, false );
            Parser
                >> shadowtune.map_size
                >> shadowtune.width
                >> shadowtune.depth
                >> shadowtune.distance;
        }
        else if (token == "smoothtraction")
        {
            // podwójna jasność ambient
            Parser.getTokens();
            Parser >> bSmoothTraction;
        }
        else if( token == "splinefidelity" ) {
            // segment size during spline->geometry conversion
            float splinefidelity;
            Parser.getTokens();
            Parser >> splinefidelity;
            SplineFidelity = clamp( splinefidelity, 1.f, 4.f );
        }
        else if( token == "gfx.resource.sweep" ) {

            Parser.getTokens();
            Parser >> ResourceSweep;
        }
        else if( token == "gfx.resource.move" ) {

            Parser.getTokens();
            Parser >> ResourceMove;
        }
        else if (token == "timespeed")
        {
            // przyspieszenie czasu, zmienna do testów
            Parser.getTokens(1, false);
            Parser >> fTimeSpeed;
        }
        else if (token == "multisampling")
        {
            // tryb antyaliasingu: 0=brak,1=2px,2=4px
            Parser.getTokens(1, false);
            Parser >> iMultisampling;
        }
        else if (token == "glutfont")
        {
            // tekst generowany przez GLUT
            Parser.getTokens();
            Parser >> bGlutFont;
        }
        else if (token == "latitude")
        {
            // szerokość geograficzna
            Parser.getTokens(1, false);
            Parser >> fLatitudeDeg;
        }
        else if (token == "convertmodels")
        {
            // tworzenie plików binarnych
            Parser.getTokens(1, false);
            Parser >> iConvertModels;
            // temporary override, to prevent generation of .e3d not compatible with old exe
            iConvertModels =
                ( iConvertModels > 128 ?
                    iConvertModels - 128 :
                    0 );
        }
        else if (token == "inactivepause")
        {
            // automatyczna pauza, gdy okno nieaktywne
            Parser.getTokens();
            Parser >> bInactivePause;
        }
        else if (token == "slowmotion")
        {
            // tworzenie plików binarnych
            Parser.getTokens(1, false);
            Parser >> iSlowMotionMask;
        }
        else if (token == "hideconsole")
        {
            // hunter-271211: ukrywanie konsoli
            Parser.getTokens();
            Parser >> bHideConsole;
        }
        else if (token == "rollfix")
        {
            // Ra: poprawianie przechyłki, aby wewnętrzna szyna była "pozioma"
            Parser.getTokens();
            Parser >> bRollFix;
        }
        else if (token == "fpsaverage")
        {
            // oczekiwana wartość FPS
            Parser.getTokens(1, false);
            Parser >> fFpsAverage;
        }
        else if (token == "fpsdeviation")
        {
            // odchylenie standardowe FPS
            Parser.getTokens(1, false);
            Parser >> fFpsDeviation;
        }
        else if (token == "calibratein")
        {
            // parametry kalibracji wejść
            Parser.getTokens(1, false);
            int in;
            Parser >> in;
            if ((in < 0) || (in > 5))
            {
                in = 5; // na ostatni, bo i tak trzeba pominąć wartości
            }
            Parser.getTokens(4, false);
            Parser
                >> fCalibrateIn[in][0] // wyraz wolny
                >> fCalibrateIn[in][1] // mnożnik
                >> fCalibrateIn[in][2] // mnożnik dla kwadratu
                >> fCalibrateIn[in][3]; // mnożnik dla sześcianu
            fCalibrateIn[in][4] = 0.0; // mnożnik 4 potęgi
            fCalibrateIn[in][5] = 0.0; // mnożnik 5 potęgi
        }
        else if (token == "calibrate5din")
        {
            // parametry kalibracji wejść
            Parser.getTokens(1, false);
            int in;
            Parser >> in;
            if ((in < 0) || (in > 5))
            {
                in = 5; // na ostatni, bo i tak trzeba pominąć wartości
            }
            Parser.getTokens(6, false);
            Parser >> fCalibrateIn[in][0] // wyraz wolny
                >> fCalibrateIn[in][1] // mnożnik
                >> fCalibrateIn[in][2] // mnożnik dla kwadratu
                >> fCalibrateIn[in][3] // mnożnik dla sześcianu
                >> fCalibrateIn[in][4] // mnożnik 4 potęgi
                >> fCalibrateIn[in][5]; // mnożnik 5 potęgi
        }
        else if (token == "calibrateout")
        {
            // parametry kalibracji wyjść
            Parser.getTokens(1, false);
            int out;
            Parser >> out;
            if ((out < 0) || (out > 6))
            {
                out = 6; // na ostatni, bo i tak trzeba pominąć wartości
            }
            Parser.getTokens(4, false);
            Parser >> fCalibrateOut[out][0] // wyraz wolny
                >> fCalibrateOut[out][1] // mnożnik liniowy
                >> fCalibrateOut[out][2] // mnożnik dla kwadratu
                >> fCalibrateOut[out][3]; // mnożnik dla sześcianu
            fCalibrateOut[out][4] = 0.0; // mnożnik dla 4 potęgi
            fCalibrateOut[out][5] = 0.0; // mnożnik dla 5 potęgi
        }
        else if (token == "calibrate5dout")
        {
            // parametry kalibracji wyjść
            Parser.getTokens(1, false);
            int out;
            Parser >> out;
            if ((out < 0) || (out > 6))
            {
                out = 6; // na ostatni, bo i tak trzeba pominąć wartości
            }
            Parser.getTokens(6, false);
            Parser >> fCalibrateOut[out][0] // wyraz wolny
                >> fCalibrateOut[out][1] // mnożnik liniowy
                >> fCalibrateOut[out][2] // mnożnik dla kwadratu
                >> fCalibrateOut[out][3] // mnożnik dla sześcianu
                >> fCalibrateOut[out][4] // mnożnik dla 4 potęgi
                >> fCalibrateOut[out][5]; // mnożnik dla 5 potęgi
        }
        else if (token == "calibrateoutmaxvalues")
        {
            // maksymalne wartości jakie można wyświetlić na mierniku
            Parser.getTokens(7, false);
            Parser >> fCalibrateOutMax[0] >> fCalibrateOutMax[1] >>
                fCalibrateOutMax[2] >> fCalibrateOutMax[3] >>
                fCalibrateOutMax[4] >> fCalibrateOutMax[5] >>
                fCalibrateOutMax[6];
        }
        else if (token == "calibrateoutdebuginfo")
        {
            // wyjście z info o przebiegu kalibracji
            Parser.getTokens(1, false);
            Parser >> iCalibrateOutDebugInfo;
        }
        else if (token == "pwm")
        {
            // zmiana numerów wyjść PWM
            Parser.getTokens(2, false);
            int pwm_out, pwm_no;
            Parser >> pwm_out >> pwm_no;
            iPoKeysPWM[pwm_out] = pwm_no;
        }
        else if (token == "brakestep")
        {
            // krok zmiany hamulca dla klawiszy [Num3] i [Num9]
            Parser.getTokens(1, false);
            Parser >> fBrakeStep;
        }
        else if (token == "joinduplicatedevents")
        {
            // czy grupować eventy o tych samych nazwach
            Parser.getTokens();
            Parser >> bJoinEvents;
        }
        else if (token == "hiddenevents")
        {
            // czy łączyć eventy z torami poprzez nazwę toru
            Parser.getTokens(1, false);
            Parser >> iHiddenEvents;
        }
        else if (token == "pause")
        {
            // czy po wczytaniu ma być pauza?
            Parser.getTokens();
            Parser >> token;
            iPause |= (token == "yes" ? 1 : 0);
        }
        else if (token == "lang")
        {
            // domyślny język - http://tools.ietf.org/html/bcp47
            Parser.getTokens(1, false);
            Parser >> asLang;
        }
        else if( token == "uitextcolor" ) {
            // color of the ui text. NOTE: will be obsolete once the real ui is in place
            Parser.getTokens( 3, false );
            Parser
                >> UITextColor.x
                >> UITextColor.y
                >> UITextColor.z;
            UITextColor.x = clamp( UITextColor.x, 0.0f, 255.0f );
            UITextColor.y = clamp( UITextColor.y, 0.0f, 255.0f );
            UITextColor.z = clamp( UITextColor.z, 0.0f, 255.0f );
            UITextColor = UITextColor / 255.0f;
            UITextColor.w = 1.0f;
        }
        else if (token == "pyscreenrendererpriority")
        {
            // priority of python screen renderer
            Parser.getTokens();
            Parser >> token;
            TPythonInterpreter::getInstance()->setScreenRendererPriority(token.c_str());
        }
        else if( token == "input.gamepad" ) {
            // czy grupować eventy o tych samych nazwach
            Parser.getTokens();
            Parser >> InputGamepad;
        }
        // maciek001: ustawienia MWD
		else if (token == "mwdmasterenable") {         // główne włączenie maszyny!
			Parser.getTokens();
			Parser >> bMWDmasterEnable;
			if (bMWDdebugEnable) WriteLog("SerialPort Master Enable");
		}
		else if (token == "mwddebugenable") {         // logowanie pracy
			Parser.getTokens();
			Parser >> bMWDdebugEnable;
			if (bMWDdebugEnable) WriteLog("MWD Debug Mode On");
		}
		else if (token == "mwddebugmode") {           // co ma być debugowane?
			Parser.getTokens(1, false);
			Parser >> iMWDDebugMode;
			if (bMWDdebugEnable) WriteLog("Debug Mode = " + to_string(iMWDDebugMode));
		}
		else if (token == "mwdcomportname") {         // nazwa portu COM
			Parser.getTokens();
			Parser >> sMWDPortId;
			if (bMWDdebugEnable) WriteLog("PortName " + sMWDPortId);
		}
		else if (token == "mwdbaudrate") {            // prędkość transmisji danych
			Parser.getTokens(1, false);
			Parser >> iMWDBaudrate;
			if (bMWDdebugEnable) WriteLog("Baud rate = " + to_string((int)(iMWDBaudrate / 1000)) + (" kbps"));
		}
		else if (token == "mwdinputenable") {         // włącz wejścia
			Parser.getTokens();
			Parser >> bMWDInputEnable;
			if (bMWDdebugEnable && bMWDInputEnable) WriteLog("MWD Input Enable");
		}
		else if (token == "mwdbreakenable") {         // włącz obsługę hamulców
			Parser.getTokens();
			Parser >> bMWDBreakEnable;
			if (bMWDdebugEnable && bMWDBreakEnable) WriteLog("MWD Break Enable");
		}
		else if (token == "mwdmainbreakconfig") {      // ustawienia hamulca zespolonego
			Parser.getTokens(2, false);
			Parser >> fMWDAnalogInCalib[0][0] >> fMWDAnalogInCalib[0][1];
			if (bMWDdebugEnable) WriteLog("Main break settings: " + to_string(fMWDAnalogInCalib[0][0]) + (" ") + to_string(fMWDAnalogInCalib[0][1]));
		}
		else if (token == "mwdlocbreakconfig") {	// ustawienia hamulca lokomotywy
			Parser.getTokens(2, false);
			Parser >> fMWDAnalogInCalib[1][0] >> fMWDAnalogInCalib[1][1];
			if (bMWDdebugEnable) WriteLog("Locomotive break settings: " + to_string(fMWDAnalogInCalib[1][0]) + (" ") + to_string(fMWDAnalogInCalib[1][1]));
		}
		else if (token == "mwdanalogin1config") {      // ustawienia hamulca zespolonego
			Parser.getTokens(2, false);
			Parser >> fMWDAnalogInCalib[2][0] >> fMWDAnalogInCalib[2][1];
			if (bMWDdebugEnable) WriteLog("Analog input 1 settings: " + to_string(fMWDAnalogInCalib[2][0]) + (" ") + to_string(fMWDAnalogInCalib[2][1]));
		}
		else if (token == "mwdanalogin2config") {	// ustawienia hamulca lokomotywy
			Parser.getTokens(2, false);
			Parser >> fMWDAnalogInCalib[3][0] >> fMWDAnalogInCalib[3][1];
			if (bMWDdebugEnable) WriteLog("Analog input 2 settings: " + to_string(fMWDAnalogInCalib[3][0]) + (" ") + to_string(fMWDAnalogInCalib[3][1]));
		}
		else if (token == "mwdmaintankpress") {        // max ciśnienie w zbiorniku głownym i rozdzielczość
			Parser.getTokens(2, false);
			Parser >> fMWDzg[0] >> fMWDzg[1];
			if (bMWDdebugEnable) WriteLog("MainAirTank settings: " + to_string(fMWDzg[0]) + (" ") + to_string(fMWDzg[1]));
		}
		else if (token == "mwdmainpipepress") {        // max ciśnienie w przewodzie głownym i rozdzielczość
			Parser.getTokens(2, false);
			Parser >> fMWDpg[0] >> fMWDpg[1];
			if (bMWDdebugEnable) WriteLog("MainAirPipe settings: " + to_string(fMWDpg[0]) + (" ") + to_string(fMWDpg[1]));
		}
		else if (token == "mwdbreakpress") {           // max ciśnienie w hamulcach i rozdzielczość
			Parser.getTokens(2, false);
			Parser >> fMWDph[0] >> fMWDph[1];
			if (bMWDdebugEnable) WriteLog("AirPipe settings: " + to_string(fMWDph[0]) + (" ") + to_string(fMWDph[1]));
		}
		else if (token == "mwdhivoltmeter") {          // max napięcie na woltomierzu WN
			Parser.getTokens(2, false);
			Parser >> fMWDvolt[0] >> fMWDvolt[1];
			if (bMWDdebugEnable) WriteLog("VoltMeter settings: " + to_string(fMWDvolt[0]) + (" ") + to_string(fMWDvolt[1]));
		}
		else if (token == "mwdhiampmeter") {
			Parser.getTokens(2, false);
			Parser >> fMWDamp[0] >> fMWDamp[1];
			if (bMWDdebugEnable) WriteLog("Amp settings: " + to_string(fMWDamp[0]) + (" ") + to_string(fMWDamp[1]));
		}
		else if (token == "mwdlowvoltmeter") {
			Parser.getTokens(2, false);
			Parser >> fMWDlowVolt[0] >> fMWDlowVolt[1];
			if (bMWDdebugEnable) WriteLog("Low VoltMeter settings: " + to_string(fMWDlowVolt[0]) + (" ") + to_string(fMWDlowVolt[1]));
		}
		else if (token == "mwddivider") {
			Parser.getTokens(1, false);
			Parser >> iMWDdivider;
			if (iMWDdivider == 0)
			{
				WriteLog("Dzielnik nie może być równy ZERO! Ustawiam na 1!");
				iMWDdivider = 1;
			}
			if (bMWDdebugEnable) WriteLog("Divider = " + to_string(iMWDdivider));
		}
    } while ((token != "") && (token != "endconfig")); //(!Parser->EndOfFile)
    // na koniec trochę zależności
    if (!bLoadTraction) // wczytywanie drutów i słupów
    { // tutaj wyłączenie, bo mogą nie być zdefiniowane w INI
        bEnableTraction = false; // false = pantograf się nie połamie
        bLiveTraction = false; // false = pantografy zawsze zbierają 95% MaxVoltage
    }
    if (iMultisampling)
    { // antyaliasing całoekranowy wyłącza rozmywanie drutów
        bSmoothTraction = false;
    }
    if (iMultiplayer > 0)
    {
        bInactivePause = false; // okno "w tle" nie może pauzować, jeśli włączona komunikacja
        // pauzowanie jest zablokowane dla (iMultiplayer&2)>0, więc iMultiplayer=1 da się zapauzować
        // (tryb instruktora)
    }
/*
    fFpsMin = fFpsAverage -
              fFpsDeviation; // dolna granica FPS, przy której promień scenerii będzie zmniejszany
    fFpsMax = fFpsAverage +
              fFpsDeviation; // górna granica FPS, przy której promień scenerii będzie zwiększany
*/
    if (iPause)
        iTextMode = GLFW_KEY_F1; // jak pauza, to pokazać zegar
    /*  this won't execute anymore with the old parser removed
            // TBD: remove, or launch depending on passed flag?
        if (qp)
    { // to poniżej wykonywane tylko raz, jedynie po wczytaniu eu07.ini*/
    Console::ModeSet(iFeedbackMode, iFeedbackPort); // tryb pracy konsoli sterowniczej
            /*iFpsRadiusMax = 0.000025 * fFpsRadiusMax *
                        fFpsRadiusMax; // maksymalny promień renderowania 3000.0 -> 225
            if (iFpsRadiusMax > 400)
                iFpsRadiusMax = 400;
            if (fDistanceFactor > 1.0)
            { // dla 1.0 specjalny tryb bez przeliczania
                fDistanceFactor =
                    iWindowHeight /
                fDistanceFactor; // fDistanceFactor>1.0 dla rozdzielczości większych niż bazowa
                fDistanceFactor *=
                    (iMultisampling + 1.0) *
                fDistanceFactor; // do kwadratu, bo większość odległości to ich kwadraty
            }
        }
    */
}
