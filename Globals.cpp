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
#include "Globals.h"

#include "simulation.h"
#include "simulationenvironment.h"
#include "Driver.h"
#include "Logs.h"
#include "Console.h"
#include "PyInt.h"

global_settings Global;

void
global_settings::LoadIniFile(std::string asFileName) {

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
            Parser >> FreeFlyModeFlag;
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
            AudioVolume = clamp( AudioVolume, 0.0f, 2.f );
        }
		else if (token == "sound.maxsources") {
			Parser.getTokens();
			Parser >> audio_max_sources;
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
            simulation::Environment.compute_season( fMoveLight );
        }
        else if( token == "dynamiclights" ) {
            // number of dynamic lights in the scene
            Parser.getTokens( 1, false );
            Parser >> DynamicLightCount;
            // clamp the light number
            // max 8 lights per opengl specs, minus one used for sun. at least one light for controlled vehicle
            DynamicLightCount = clamp( DynamicLightCount, 1, 7 ); 
        }
        else if( token == "scenario.time.override" ) {
            // shift (in hours) applied to train timetables
            Parser.getTokens( 1, false );
			std::string token;
			Parser >> token;
			std::istringstream stream(token);
			if (token.find(':')) {
				float a, b;
				char s;
				stream >> a >> s >> b;
				ScenarioTimeOverride = a + b / 60.0;
			}
			else
				stream >> ScenarioTimeOverride;
            ScenarioTimeOverride = clamp( ScenarioTimeOverride, 0.f, 24 * 1439 / 1440.f );
        }
        else if( token == "scenario.time.offset" ) {
            // shift (in hours) applied to train timetables
            Parser.getTokens( 1, false );
            Parser >> ScenarioTimeOffset;
        }
        else if( token == "scenario.time.current" ) {
            // sync simulation time with local clock
            Parser.getTokens( 1, false );
            Parser >> ScenarioTimeCurrent;
        }
        else if( token == "scenario.weather.temperature" ) {
            // selected device for audio renderer
            Parser.getTokens();
            Parser >> AirTemperature;
            if( false == DebugModeFlag ) {
                AirTemperature = clamp( AirTemperature, -15.f, 45.f );
            }
        }
        else if( token == "scalespeculars" ) {
            // whether strength of specular highlights should be adjusted (generally needed for legacy 3d models)
            Parser.getTokens();
            Parser >> ScaleSpecularValues;
        }
        else if( token == "shadowtune" ) {
            Parser.getTokens( 4, false );
            Parser
                >> shadowtune.map_size
                >> shadowtune.width
                >> shadowtune.depth
                >> shadowtune.distance;
        }
        else if( token == "splinefidelity" ) {
            // segment size during spline->geometry conversion
            float splinefidelity;
            Parser.getTokens();
            Parser >> splinefidelity;
            SplineFidelity = clamp( splinefidelity, 1.f, 4.f );
        }
        else if( token == "createswitchtrackbeds" ) {
            // podwójna jasność ambient
            Parser.getTokens();
            Parser >> CreateSwitchTrackbeds;
        }
        else if( token == "gfx.resource.sweep" ) {

            Parser.getTokens();
            Parser >> ResourceSweep;
        }
        else if( token == "gfx.resource.move" ) {

            Parser.getTokens();
            Parser >> ResourceMove;
        }
        else if( token == "gfx.reflections.framerate" ) {

            auto const updatespersecond { std::abs( Parser.getToken<double>() ) };
            ReflectionUpdatesPerSecond = (
                updatespersecond > 0 ?
                    1000 / std::min( 30.0, updatespersecond ) :
                    0 );
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
            Parser.getTokens(1, false);
            Parser >> fBrakeStep;
        }
        else if (token == "brakespeed")
        {
            Parser.getTokens(1, false);
            Parser >> brake_speed;
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
        else if( token == "pyscreenrendererpriority" )
        {
            // old variable, repurposed as update rate of python screen renderer
            Parser.getTokens();
            Parser >> token;
            auto const priority { ToLower( token ) };
            PythonScreenUpdateRate = (
                priority == "lower" ? 500 :
                priority == "lowest" ? 1000 :
                200 );
        }
        else if( token == "uitextcolor" ) {
            // color of the ui text. NOTE: will be obsolete once the real ui is in place
            Parser.getTokens( 3, false );
            Parser
                >> UITextColor.r
                >> UITextColor.g
                >> UITextColor.b;
            glm::clamp( UITextColor, 0.f, 255.f );
            UITextColor = UITextColor / 255.f;
            UITextColor.a = 1.f;
        }
        else if( token == "ui.bg.opacity" ) {
            // czy grupować eventy o tych samych nazwach
            Parser.getTokens();
            Parser >> UIBgOpacity;
            UIBgOpacity = clamp( UIBgOpacity, 0.f, 1.f );
        }
        else if( token == "input.gamepad" ) {
            // czy grupować eventy o tych samych nazwach
            Parser.getTokens();
            Parser >> InputGamepad;
		}
		else if (token == "motiontelemetry")
		{
			Parser.getTokens(8);
			Global.motiontelemetry_conf.enable = true;
			Parser >> Global.motiontelemetry_conf.proto;
			Parser >> Global.motiontelemetry_conf.address;
			Parser >> Global.motiontelemetry_conf.port;
			Parser >> Global.motiontelemetry_conf.updatetime;
			Parser >> Global.motiontelemetry_conf.includegravity;
			Parser >> Global.motiontelemetry_conf.fwdposbased;
			Parser >> Global.motiontelemetry_conf.latposbased;
			Parser >> Global.motiontelemetry_conf.axlebumpscale;
		}
		else if (token == "screenshotsdir")
		{
			Parser.getTokens(1);
			Parser >> Global.screenshot_dir;
		}
        else if( token == "uart" ) {
            uart_conf.enable = true;
            Parser.getTokens( 3, false );
            Parser
                >> uart_conf.port
                >> uart_conf.baud
                >> uart_conf.updatetime;
        }
        else if( token == "uarttune" ) {
            Parser.getTokens( 14 );
            Parser
                >> uart_conf.mainbrakemin
                >> uart_conf.mainbrakemax
                >> uart_conf.localbrakemin
                >> uart_conf.localbrakemax
                >> uart_conf.tankmax
                >> uart_conf.tankuart
                >> uart_conf.pipemax
                >> uart_conf.pipeuart
                >> uart_conf.brakemax
                >> uart_conf.brakeuart
                >> uart_conf.hvmax
                >> uart_conf.hvuart
                >> uart_conf.currentmax
                >> uart_conf.currentuart;
        }
        else if( token == "uartfeature" ) {
            Parser.getTokens( 4 );
            Parser
                >> uart_conf.mainenable
                >> uart_conf.scndenable
                >> uart_conf.trainenable
                >> uart_conf.localenable;
        }
        else if( token == "uartdebug" ) {
            Parser.getTokens( 1 );
            Parser >> uart_conf.debug;
        }
		else if (token == "loadinglog") {
            Parser.getTokens( 1 );
            Parser >> loading_log;
		}
		else if (token == "ddsupperorigin") {
            Parser.getTokens( 1 );
            Parser >> dds_upper_origin;			
		}
		else if (token == "compresstex") {
            Parser.getTokens( 1 );
            Parser >> compress_tex;
		}
        else if (token == "captureonstart") {
            Parser.getTokens( 1 );
            Parser >> captureonstart;
        }
        else if (token == "gfx.framebuffer.width")
        {
            Parser.getTokens(1, false);
            Parser >> gfx_framebuffer_width;
        }
        else if (token == "gfx.framebuffer.height")
        {
            Parser.getTokens(1, false);
            Parser >> gfx_framebuffer_height;
        }
        else if (token == "gfx.shadowmap.enabled")
        {
            Parser.getTokens(1);
            Parser >> gfx_shadowmap_enabled;
        }
        else if (token == "gfx.envmap.enabled")
        {
            Parser.getTokens(1);
            Parser >> gfx_envmap_enabled;
        }
        else if (token == "gfx.postfx.motionblur.enabled")
        {
            Parser.getTokens(1);
            Parser >> gfx_postfx_motionblur_enabled;
        }
        else if (token == "gfx.postfx.motionblur.shutter")
        {
            Parser.getTokens(1);
            Parser >> gfx_postfx_motionblur_shutter;
        }
        else if (token == "gfx.postfx.motionblur.format")
        {
            Parser.getTokens(1);
            std::string token;
            Parser >> token;
            if (token == "rg16f")
                gfx_postfx_motionblur_format = GL_RG16F;
            else if (token == "rg32f")
                gfx_postfx_motionblur_format = GL_RG32F;
        }
        else if (token == "gfx.format.color")
        {
            Parser.getTokens(1);
            std::string token;
            Parser >> token;
            if (token == "rgb8")
                gfx_format_color = GL_RGB8;
            else if (token == "rgb16f")
                gfx_format_color = GL_RGB16F;
            else if (token == "rgb32f")
                gfx_format_color = GL_RGB32F;
            else if (token == "r11f_g11f_b10f")
                gfx_format_color = GL_R11F_G11F_B10F;
        }
        else if (token == "gfx.format.depth")
        {
            Parser.getTokens(1);
            std::string token;
            Parser >> token;
            if (token == "z16")
                gfx_format_depth = GL_DEPTH_COMPONENT16;
            else if (token == "z24")
                gfx_format_depth = GL_DEPTH_COMPONENT24;
            else if (token == "z32")
                gfx_format_depth = GL_DEPTH_COMPONENT32;
            else if (token == "z32f")
                gfx_format_depth = GL_DEPTH_COMPONENT32F;
        }
        else if (token == "gfx.skippipeline")
        {
            Parser.getTokens(1);
            Parser >> gfx_skippipeline;
        }
        else if (token == "gfx.usegles")
        {
            Parser.getTokens(1);
            Parser >> gfx_usegles;
        }
        else if (token == "gfx.shadergamma")
        {
            Parser.getTokens(1);
            Parser >> gfx_shadergamma;
        }
        else if (token == "map.enabled")
        {
            Parser.getTokens(1);
            Parser >> map_enabled;
        }
		else if (token == "python.displaywindows")
		{
			Parser.getTokens(1);
			Parser >> python_displaywindows;
		}
		else if (token == "python.threadedupload")
		{
			Parser.getTokens(1);
			Parser >> python_threadedupload;
		}
		else if (token == "python.mipmaps")
		{
			Parser.getTokens(1);
			Parser >> python_mipmaps;
		}
    } while ((token != "") && (token != "endconfig")); //(!Parser->EndOfFile)
    // na koniec trochę zależności
    if (!bLoadTraction) // wczytywanie drutów i słupów
    { // tutaj wyłączenie, bo mogą nie być zdefiniowane w INI
        bEnableTraction = false; // false = pantograf się nie połamie
        bLiveTraction = false; // false = pantografy zawsze zbierają 95% MaxVoltage
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
#ifdef _WIN32
		    Console::ModeSet(iFeedbackMode, iFeedbackPort); // tryb pracy konsoli sterowniczej
#endif
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
