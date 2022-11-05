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
#include "Timer.h"
#include "vao.h"

global_settings Global;

void
global_settings::LoadIniFile(std::string asFileName) {

    // initialize season data in case the main config file doesn't
    std::time_t timenow = std::time( 0 );
    std::tm *localtime = std::localtime( &timenow );
    fMoveLight = localtime->tm_yday + 1; // numer bieżącego dnia w roku
    simulation::Environment.compute_season( fMoveLight );

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

        if( ConfigParse_gfx( Parser, token ) ) {
            continue;
        }
        else if (token == "sceneryfile")
        {
            Parser.getTokens();
            Parser >> SceneryFile;
        }
        else if (token == "humanctrlvehicle")
        {
            Parser.getTokens();
            Parser >> local_start_vehicle;
        }
        else if (token == "fieldofview")
        {
            Parser.getTokens(1, false);
            Parser >> FieldOfView;
            // guard against incorrect values
            FieldOfView = clamp(FieldOfView, 10.0f, 75.0f);
        }
        else if (token == "width")
        {
            Parser.getTokens(1, false);
            Parser >> window_size.x;
        }
        else if (token == "height")
        {
            Parser.getTokens(1, false);
            Parser >> window_size.y;
        }
        else if (token == "heightbase")
        {
            Parser.getTokens(1, false);
            Parser >> fDistanceFactor;
        }
        else if (token == "targetfps")
        {
            Parser.getTokens(1, false);
            Parser >> targetfps;
        }
        else if (token == "basedrawrange")
        {
            Parser.getTokens(1);
            Parser >> BaseDrawRange;
        }
        else if (token == "fullscreen")
        {
            Parser.getTokens();
            Parser >> bFullScreen;
        }
        else if (token == "fullscreenmonitor")
        {
            Parser.getTokens(1, false);
            Parser >> fullscreen_monitor;
        }
        else if (token == "fullscreenwindowed")
        {
            Parser.getTokens();
            Parser >> fullscreen_windowed;
        }
        else if (token == "vsync")
        {
            Parser.getTokens();
            Parser >> VSync;
        }
        else if (token == "freefly")
        { // Mczapkie-130302
            Parser.getTokens();
            Parser >> FreeFlyModeFlag;
            Parser.getTokens(3, false);
            Parser >> FreeCameraInit[0].x, FreeCameraInit[0].y, FreeCameraInit[0].z;
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
        else if (token == "sound.openal.renderer")
        {
            // selected device for audio renderer
            AudioRenderer = Parser.getToken<std::string>(false); // case-sensitive
        }
        else if (token == "sound.volume")
        {
            // selected device for audio renderer
            Parser.getTokens();
            Parser >> AudioVolume;
            AudioVolume = clamp(AudioVolume, 0.f, 2.f);
        }
        else if (token == "sound.volume.radio")
        {
            // selected device for audio renderer
            Parser.getTokens();
            Parser >> RadioVolume;
            RadioVolume = clamp(RadioVolume, 0.f, 1.f);
        }
		else if (token == "sound.maxsources") {
			Parser.getTokens();
			Parser >> audio_max_sources;
		}
        else if( token == "sound.volume.vehicle" ) {
            Parser.getTokens();
            Parser >> VehicleVolume;
            VehicleVolume = clamp(VehicleVolume, 0.f, 1.f);
        }
        else if (token == "sound.volume.positional")
        {
            Parser.getTokens();
            Parser >> EnvironmentPositionalVolume;
            EnvironmentPositionalVolume = clamp(EnvironmentPositionalVolume, 0.f, 1.f);
        }
        else if (token == "sound.volume.ambient")
        {
            Parser.getTokens();
            Parser >> EnvironmentAmbientVolume;
            EnvironmentAmbientVolume = clamp(EnvironmentAmbientVolume, 0.f, 1.f);
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
                iWriteLogEnabled = stol_def(token, 3);
            }
        }
        else if (token == "multiplelogs")
        {
            Parser.getTokens();
            Parser >> MultipleLogs;
        }
        else if (token == "logs.filter")
        {
            Parser.getTokens();
            Parser >> DisabledLogTypes;
        }
        else if (token == "mousescale")
        {
            // McZapkie-060503 - czulosc ruchu myszy (krecenia glowa)
            Parser.getTokens(2, false);
            Parser >> fMouseXScale >> fMouseYScale;
        }
        else if (token == "mousecontrol")
        {
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
            else
            {
                szDefaultExt = (token[0] == '.' ? token : "." + token);
            }
        }
        else if (token == "newaircouplers")
        {
            Parser.getTokens();
            Parser >> bnewAirCouplers;
        }
        else if (token == "anisotropicfiltering")
        {
            Parser.getTokens(1, false);
            Parser >> AnisotropicFiltering;
            if (AnisotropicFiltering < 1.0f)
                AnisotropicFiltering = 1.0f;
        }
        else if (token == "usevbo")
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
		else if (token == "isolatedtrainnumber")
		{
		Parser.getTokens(1, false);
		Parser >> bIsolatedTrainName;
		}
        else if (token == "maxtexturesize")
        {
            // wymuszenie przeskalowania tekstur
            Parser.getTokens(1, false);
            int size;
            Parser >> size;
            iMaxTextureSize = clamp_power_of_two(size, 512, 8192);
        }
        else if (token == "maxcabtexturesize")
        {
            // wymuszenie przeskalowania tekstur
            Parser.getTokens(1, false);
            int size;
            Parser >> size;
            iMaxCabTextureSize = clamp_power_of_two(size, 512, 8192);
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
            simulation::Environment.compute_season(fMoveLight);
        }
        else if (token == "dynamiclights")
        {
            // number of dynamic lights in the scene
            Parser.getTokens(1, false);
            Parser >> DynamicLightCount;
            // clamp the light number
            // max 8 lights per opengl specs, minus one used for sun. at least one light for
            // controlled vehicle
            DynamicLightCount = clamp(DynamicLightCount, 0, 7);
        }
        else if (token == "scenario.time.override")
        {
            // shift (in hours) applied to train timetables
            Parser.getTokens(1, false);
            std::string token;
            Parser >> token;
            std::istringstream stream(token);
            if (token.find(':') != -1)
            {
                float a, b;
                char s;
                stream >> a >> s >> b;
                ScenarioTimeOverride = a + b / 60.0;
            }
            else
                stream >> ScenarioTimeOverride;
            ScenarioTimeOverride = clamp(ScenarioTimeOverride, 0.f, 24 * 1439 / 1440.f);
        }
        else if (token == "scenario.time.offset")
        {
            // shift (in hours) applied to train timetables
            Parser.getTokens(1, false);
            Parser >> ScenarioTimeOffset;
        }
        else if (token == "scenario.time.current")
        {
            // sync simulation time with local clock
            Parser.getTokens(1, false);
            Parser >> ScenarioTimeCurrent;
        }
        else if (token == "scenario.weather.temperature")
        {
            // selected device for audio renderer
            Parser.getTokens();
            Parser >> AirTemperature;
            if (false == DebugModeFlag)
            {
                AirTemperature = clamp(AirTemperature, -15.f, 45.f);
            }
        }
        else if (token == "scalespeculars")
        {
            // whether strength of specular highlights should be adjusted (generally needed for
            // legacy 3d models)
            Parser.getTokens();
            Parser >> ScaleSpecularValues;
        }
        else if (token == "gfxrenderer")
        {
            // shadow render toggle
            Parser.getTokens();
            Parser >> GfxRenderer;
            if (GfxRenderer == "full")
            {
                GfxRenderer = "default";
            }
            BasicRenderer = (GfxRenderer == "simple");
            LegacyRenderer = (GfxRenderer != "default");
        }
        else if (token == "shadows")
        {
            // shadow render toggle
            Parser.getTokens();
            Parser >> RenderShadows;
        }
        else if (token == "shadowtune")
        {
            Parser.getTokens(4, false);
            float discard;
            Parser >> shadowtune.map_size >> discard >> shadowtune.range >> discard;
            shadowtune.map_size = clamp_power_of_two<unsigned int>(shadowtune.map_size, 512, 8192);
            // make sure we actually make effective use of all csm stages
            shadowtune.range =
                std::max((shadowtune.map_size <= 2048 ? 75.f : 75.f * shadowtune.map_size / 2048),
                         shadowtune.range);
        }
        else if (token == "smoothtraction")
        {
            // podwójna jasność ambient
            Parser.getTokens();
            Parser >> bSmoothTraction;
        }
        else if (token == "splinefidelity")
        {
            // segment size during spline->geometry conversion
            float splinefidelity;
            Parser.getTokens();
            Parser >> splinefidelity;
            SplineFidelity = clamp(splinefidelity, 1.f, 4.f);
        }
        else if (token == "rendercab")
        {
            Parser.getTokens();
            Parser >> render_cab;
        }
        else if (token == "createswitchtrackbeds")
        {
            // podwójna jasność ambient
            Parser.getTokens();
            Parser >> CreateSwitchTrackbeds;
        }
        else if (token == "timespeed")
        {
            // przyspieszenie czasu, zmienna do testów
            Parser.getTokens(1, false);
			Parser >> default_timespeed;
			fTimeSpeed = default_timespeed;
        }
		else if (token == "deltaoverride")
		{
			// for debug
			Parser.getTokens(1, false);
			float deltaoverride;
			Parser >> deltaoverride;
			Timer::set_delta_override(1.0f / deltaoverride);
		}
        else if (token == "multisampling")
        {
            // tryb antyaliasingu: 0=brak,1=2px,2=4px
            Parser.getTokens(1, false);
            Parser >> iMultisampling;
            iMultisampling = clamp(iMultisampling, 0, 3);
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
        }
        else if (token == "convertindexrange")
        {
            Parser.getTokens(1, false);
            Parser >> iConvertIndexRange;
        }
        else if (token == "file.binary.terrain")
        {
            // binary terrain (de)serialization
            Parser.getTokens(1, false);
            Parser >> file_binary_terrain;
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
            Parser >> fCalibrateIn[in][0] // wyraz wolny
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
            Parser >> fCalibrateOutMax[0] >> fCalibrateOutMax[1] >> fCalibrateOutMax[2] >>
                fCalibrateOutMax[3] >> fCalibrateOutMax[4] >> fCalibrateOutMax[5] >>
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
        else if (token == "ai.trainman")
        {
            // czy łączyć eventy z torami poprzez nazwę toru
            Parser.getTokens(1, false);
            Parser >> AITrainman;
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
		else if( token == "python.updatetime" )
        {
            Parser.getTokens();
            Parser >> PythonScreenUpdateRate;
        }
        else if (token == "uitextcolor")
        {
            // color of the ui text. NOTE: will be obsolete once the real ui is in place
            Parser.getTokens(3, false);
            Parser >> UITextColor.r >> UITextColor.g >> UITextColor.b;
            glm::clamp(UITextColor, 0.f, 255.f);
            UITextColor = UITextColor / 255.f;
            UITextColor.a = 1.f;
        }
        else if (token == "ui.bg.opacity")
        {
            // czy grupować eventy o tych samych nazwach
            Parser.getTokens();
            Parser >> UIBgOpacity;
            UIBgOpacity = clamp(UIBgOpacity, 0.f, 1.f);
        }
        else if (token == "input.gamepad")
        {
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
        if (token == "screenshotsdir")
		{
			Parser.getTokens(1);
			Parser >> Global.screenshot_dir;
		}
#ifdef WITH_UART
        else if (token == "uart")
        {
            uart_conf.enable = true;
            Parser.getTokens(3, false);
            Parser
                >> uart_conf.port
                >> uart_conf.baud
                >> uart_conf.updatetime;
        }
        else if (token == "uarttune")
        {
            Parser.getTokens(18);
            Parser
                >> uart_conf.mainbrakemin >> uart_conf.mainbrakemax
                >> uart_conf.localbrakemin >> uart_conf.localbrakemax
                >> uart_conf.tankmax >> uart_conf.tankuart
                >> uart_conf.pipemax >> uart_conf.pipeuart
                >> uart_conf.brakemax >> uart_conf.brakeuart
                >> uart_conf.pantographmax >> uart_conf.pantographuart
                >> uart_conf.hvmax >> uart_conf.hvuart
                >> uart_conf.currentmax >> uart_conf.currentuart
                >> uart_conf.lvmax >> uart_conf.lvuart;
        }
        else if (token == "uarttachoscale")
        {
            Parser.getTokens(1);
            Parser >> uart_conf.tachoscale;
        }
        else if (token == "uartfeature")
        {
            Parser.getTokens(1);
            std::string firstToken = Parser.peek();

            if(firstToken.find('|') != std::string::npos || firstToken == "none" || uartfeatures_map.count(firstToken)) {
                // new format (features delimited by pipe)

                // all settings should be disabled initially
                for(auto const &x : uartfeatures_map) {
                    *(x.second) = false;
                }

                // and only defined should be enabled
                std::string key;
                std::stringstream firstTokenStream = std::stringstream(firstToken);

                while(!firstTokenStream.eof()) {
                    std::getline(firstTokenStream, key, '|');

                    if(uartfeatures_map.count(key)) {
                        *(uartfeatures_map[key]) = true;
                    }
                }
            } else {
                // legacy format

                // first token is already parsed
                Parser >> uart_conf.mainenable;

                // the rest tokens must be fetched
                Parser.getTokens(3);
                Parser
                    >> uart_conf.scndenable
                    >> uart_conf.trainenable
                    >> uart_conf.localenable;
            }
        }
        else if( token == "uartdebug" ) {
            Parser.getTokens( 1 );
            Parser >> uart_conf.debug;
        }
        else if (token == "uartmainpercentage")
        {
            Parser.getTokens(1);
            Parser >> uart_conf.mainpercentage;
        }
#endif
#ifdef WITH_ZMQ
        else if( token == "zmq.address" ) {
            Parser.getTokens( 1 );
            Parser >> zmq_address;
        }
#endif
#ifdef USE_EXTCAM_CAMERA
		else if( token == "extcam.cmd" ) {
			Parser.getTokens( 1 );
			Parser >> extcam_cmd;
		}
		else if( token == "extcam.rec" ) {
			Parser.getTokens( 1 );
			Parser >> extcam_rec;
		}
		else if( token == "extcam.res" ) {
			Parser.getTokens( 2 );
			Parser >> extcam_res.x >> extcam_res.y;
		}
#endif
		else if (token == "loadinglog") {
            Parser.getTokens( 1 );
            Parser >> loading_log;
		}
		else if (token == "ddsupperorigin") {
            Parser.getTokens( 1 );
            Parser >> dds_upper_origin;
		}
        else if (token == "captureonstart") {
            Parser.getTokens( 1 );
            Parser >> captureonstart;
        }
		else if (token == "gui.defaultwindows") {
			Parser.getTokens(1);
			Parser >> gui_defaultwindows;
		}
		else if (token == "gui.showtranscripts") {
			Parser.getTokens(1);
			Parser >> gui_showtranscripts;
		}
        else if (token == "gui.trainingdefault") {
			Parser.getTokens(1);
            Parser >> gui_trainingdefault;
		}
        else if (token == "gfx.angleplatform")
        {
            Parser.getTokens(1);
            Parser >> gfx_angleplatform;
        }
        else if (token == "gfx.gldebug")
        {
            Parser.getTokens(1);
            Parser >> gfx_gldebug;
        }
        else if (token == "ui.fontsize")
        {
            Parser.getTokens(1);
            Parser >> ui_fontsize;
        }
        else if (token == "ui.scale")
        {
            Parser.getTokens(1);
            Parser >> ui_scale;
        }
        else if (token == "python.enabled")
        {
            Parser.getTokens(1);
            Parser >> python_enabled;
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
		else if (token == "python.sharectx")
		{
			Parser.getTokens(1);
			Parser >> python_sharectx;
		}
		else if (token == "python.uploadmain")
		{
			Parser.getTokens(1);
			Parser >> python_uploadmain;
		}
		else if (token == "python.fpslimit")
		{
			Parser.getTokens(1);
			float fpslimit;
			Parser >> fpslimit;
			python_minframetime = std::chrono::duration<float>(1.0f / fpslimit);
		}
		else if (token == "python.vsync")
		{
			Parser.getTokens(1);
			Parser >> python_vsync;
		}
		else if (token == "python.mipmaps")
		{
			Parser.getTokens(1);
			Parser >> python_mipmaps;
		}
		else if (token == "python.viewport")
		{
			Parser.getTokens(8, false);

			pythonviewport_config conf;
			Parser >> conf.surface >> conf.monitor;
			Parser >> conf.size.x >> conf.size.y;
			Parser >> conf.offset.x >> conf.offset.y;
			Parser >> conf.scale.x >> conf.scale.y;

			python_viewports.push_back(conf);
		}
        else if (token == "extraviewport")
		{
            Parser.getTokens(3 + 12, false);

			extraviewport_config conf;
			Parser >> conf.monitor >> conf.width >> conf.height;
			Parser >> conf.draw_range;

            Parser >> conf.projection.pa.x >> conf.projection.pa.y >> conf.projection.pa.z;
            Parser >> conf.projection.pb.x >> conf.projection.pb.y >> conf.projection.pb.z;
            Parser >> conf.projection.pc.x >> conf.projection.pc.y >> conf.projection.pc.z;
            Parser >> conf.projection.pe.x >> conf.projection.pe.y >> conf.projection.pe.z;

			extra_viewports.push_back(conf);
            if (gl::vao::use_vao && conf.monitor != "MAIN") {
				gl::vao::use_vao = false;
                WriteLog("using multiple windows, disabling vao!");
			}
		}
		else if (token == "map.highlightdistance") {
			Parser.getTokens(1);
			Parser >> map_highlight_distance;
		}
        else if (token == "headtrack") {
            Parser.getTokens(14);
            Parser >> headtrack_conf.joy;
            Parser >> headtrack_conf.magic_window;
            Parser >> headtrack_conf.move_axes[0] >> headtrack_conf.move_axes[1] >> headtrack_conf.move_axes[2];
            Parser >> headtrack_conf.move_mul[0] >> headtrack_conf.move_mul[1] >> headtrack_conf.move_mul[2];
            Parser >> headtrack_conf.rot_axes[0] >> headtrack_conf.rot_axes[1] >> headtrack_conf.rot_axes[2];
            Parser >> headtrack_conf.rot_mul[0] >> headtrack_conf.rot_mul[1] >> headtrack_conf.rot_mul[2];
        }
        else if (token == "vr.enabled") {
            Parser.getTokens(1);
            Parser >> vr;
        }
        else if (token == "vr.backend") {
            Parser.getTokens(1);
            Parser >> vr_backend;
        }
        else if (token == "compresstex")
        {
            Parser.getTokens(1);
            if (false == gfx_usegles)
            {
                // ogl es use requires compression to be disabled
                Parser >> compress_tex;
            }
        }
        else if (token == "fpslimit")
        {
            Parser.getTokens(1);
            float fpslimit;
            Parser >> fpslimit;
            minframetime = std::chrono::duration<float>(1.0f / fpslimit);
        }
        else if (token == "randomseed")
        {
            Parser.getTokens(1);
            Parser >> Global.random_seed;
        }
		else if (token == "network.server")
		{
			Parser.getTokens(2);

			std::string backend;
			std::string conf;
			Parser >> backend >> conf;

			network_servers.push_back(std::make_pair(backend, conf));
		}
		else if (token == "network.client")
		{
			Parser.getTokens(2);

			network_client.emplace();
			Parser >> network_client->first;
			Parser >> network_client->second;
		}
		else if (token == "execonexit") {
			Parser.getTokens(1);
			Parser >> exec_on_exit;
			std::replace(std::begin(exec_on_exit), std::end(exec_on_exit), '_', ' ');
		}
        else if (token == "map.manualswitchcontrol") {
            Parser.getTokens(1);
            Parser >> map_manualswitchcontrol;
        }
        else if (token == "prepend_scn") {
            Parser.getTokens(1);
            Parser >> prepend_scn;
        }
		else if (token == "crashdamage") {
			Parser.getTokens(1);
			Parser >> crash_damage;
		}

    } while ((token != "") && (token != "endconfig")); //(!Parser->EndOfFile)
    // na koniec trochę zależności
    if (!bLoadTraction) // wczytywanie drutów i słupów
    { // tutaj wyłączenie, bo mogą nie być zdefiniowane w INI
        bEnableTraction = false; // false = pantograf się nie połamie
        bLiveTraction = false; // false = pantografy zawsze zbierają 95% MaxVoltage
    }
    if (vr) {
        gfx_skippipeline = false;
        VSync = false;
    }
    /*
        fFpsMin = fFpsAverage -
                  fFpsDeviation; // dolna granica FPS, przy której promień scenerii będzie
       zmniejszany fFpsMax = fFpsAverage + fFpsDeviation; // górna granica FPS, przy której promień
       scenerii będzie zwiększany
    */
    if (iPause)
        iTextMode = GLFW_KEY_F1; // jak pauza, to pokazać zegar

#ifndef WITH_PYTHON
	python_enabled = false;
#endif

#ifdef _WIN32
		    Console::ModeSet(iFeedbackMode, iFeedbackPort); // tryb pracy konsoli sterowniczej
#endif
}

bool
global_settings::ConfigParse_gfx( cParser &Parser, std::string_view const Token ) {

    // TODO: move other graphics switches here
    auto tokenparsed { true };

    if (Token == "gfx.shadows.cab.range")
    {
        // shadow render toggle
        Parser.getTokens();
        Parser >> RenderCabShadowsRange;
    }
    else if (Token == "gfx.smoke")
    {
        // smoke visualization toggle
        Parser.getTokens();
        Parser >> Smoke;
    }
    else if (Token == "gfx.smoke.fidelity")
    {
        // smoke visualization fidelity
        float smokefidelity;
        Parser.getTokens();
        Parser >> smokefidelity;
        SmokeFidelity = clamp(smokefidelity, 1.f, 4.f);
    }
    else if (Token == "gfx.resource.sweep")
    {
        Parser.getTokens();
        Parser >> ResourceSweep;
    }
    else if (Token == "gfx.resource.move")
    {
        Parser.getTokens();
        Parser >> ResourceMove;
    }
    else if (Token == "gfx.reflections.framerate")
    {
        auto const updatespersecond{std::abs(Parser.getToken<double>())};
        reflectiontune.update_interval = 1.0 / updatespersecond;
    }
    else if (Token == "gfx.reflections.fidelity")
    {
        Parser.getTokens(1, false);
        Parser >> reflectiontune.fidelity;
        reflectiontune.fidelity = clamp(reflectiontune.fidelity, 0, 2);
    }
    else if (Token == "gfx.framebuffer.width")
    {
        Parser.getTokens(1, false);
        Parser >> gfx_framebuffer_width;
    }
    else if (Token == "gfx.framebuffer.height")
    {
        Parser.getTokens(1, false);
        Parser >> gfx_framebuffer_height;
    }
    else if (Token == "gfx.shadowmap.enabled")
    {
        Parser.getTokens(1);
        Parser >> gfx_shadowmap_enabled;
    }
    else if (Token == "gfx.envmap.enabled")
    {
        Parser.getTokens(1);
        Parser >> gfx_envmap_enabled;
    }
    else if (Token == "gfx.postfx.motionblur.enabled")
    {
        Parser.getTokens(1);
        Parser >> gfx_postfx_motionblur_enabled;
    }
    else if (Token == "gfx.postfx.motionblur.shutter")
    {
        Parser.getTokens(1);
        Parser >> gfx_postfx_motionblur_shutter;
    }
    else if (Token == "gfx.postfx.motionblur.format")
    {
        Parser.getTokens(1);
        std::string value;
        Parser >> value;
        if (value == "rg16f")
            gfx_postfx_motionblur_format = GL_RG16F;
        else if (value == "rg32f")
            gfx_postfx_motionblur_format = GL_RG32F;
    }
    else if (Token == "gfx.postfx.chromaticaberration.enabled")
    {
        Parser.getTokens(1);
        Parser >> gfx_postfx_chromaticaberration_enabled;
    }
    else if (Token == "gfx.format.color")
    {
        Parser.getTokens(1);
        std::string value;
        Parser >> value;
        if (value == "rgb8")
            gfx_format_color = GL_RGB8;
        else if (value == "rgb16f")
            gfx_format_color = GL_RGB16F;
        else if (value == "rgb32f")
            gfx_format_color = GL_RGB32F;
        else if (value == "r11f_g11f_b10f")
            gfx_format_color = GL_R11F_G11F_B10F;
    }
    else if (Token == "gfx.format.depth")
    {
        Parser.getTokens(1);
        std::string value;
        Parser >> value;
        if (value == "z16")
            gfx_format_depth = GL_DEPTH_COMPONENT16;
        else if (value == "z24")
            gfx_format_depth = GL_DEPTH_COMPONENT24;
        else if (value == "z32")
            gfx_format_depth = GL_DEPTH_COMPONENT32;
        else if (value == "z32f")
            gfx_format_depth = GL_DEPTH_COMPONENT32F;
    }
    else if (Token == "gfx.skiprendering")
    {
        Parser.getTokens(1);
        Parser >> gfx_skiprendering;
    }
    else if (Token == "gfx.skippipeline")
    {
        Parser.getTokens(1);
        Parser >> gfx_skippipeline;
    }
    else if (Token == "gfx.extraeffects")
    {
        Parser.getTokens(1);
        Parser >> gfx_extraeffects;
    }
    else if (Token == "gfx.usegles")
    {
        Parser.getTokens(1);
        Parser >> gfx_usegles;
    }
    else if (Token == "gfx.shadergamma")
    {
        Parser.getTokens(1);
        Parser >> gfx_shadergamma;
    }
    else if (Token == "gfx.drawrange.factor.max")
    {
        Parser.getTokens(1);
        Parser >> gfx_distance_factor_max;
    }
    else if (Token == "gfx.shadow.angle.min")
    {
        Parser.getTokens(1);
        Parser >> gfx_shadow_angle_min;
        // variable internally uses negative values, but is presented as positive in settings
        // so it's likely it'll be supplied as positive number by external launcher
        if( gfx_shadow_angle_min > 0 ) {
            gfx_shadow_angle_min *= -1;
        }
        gfx_shadow_angle_min = clamp(gfx_shadow_angle_min, -1.f, -0.2f);
    }
    else if (Token == "gfx.shadow.rank.cutoff")
    {
        Parser.getTokens(1);
        Parser >> gfx_shadow_rank_cutoff;
        gfx_shadow_rank_cutoff = clamp(gfx_shadow_rank_cutoff, 1, 3);
    }
    else
    {
        tokenparsed = false;
    }

    return tokenparsed;
}

void
global_settings::export_as_text( std::ostream &Output ) const {

    export_as_text( Output, "sceneryfile", SceneryFile );
    export_as_text( Output, "humanctrlvehicle", local_start_vehicle );
    export_as_text( Output, "fieldofview", FieldOfView );
    export_as_text( Output, "width", window_size.x );
    export_as_text( Output, "height", window_size.y );
    export_as_text( Output, "targetfps", targetfps );
    export_as_text( Output, "basedrawrange", BaseDrawRange );
    export_as_text( Output, "fullscreen", bFullScreen );
    export_as_text( Output, "fullscreenmonitor", fullscreen_monitor );
    export_as_text( Output, "fullscreenwindowed", fullscreen_windowed );
    export_as_text( Output, "vsync", VSync );
    // NOTE: values are changed dynamically during simulation. cache initial settings and export instead
    if( FreeFlyModeFlag ) {
        Output
            << "freefly yes "
            << FreeCameraInit[ 0 ].x << " "
            << FreeCameraInit[ 0 ].y << " "
            << FreeCameraInit[ 0 ].z << "\n";
    }
    else {
        export_as_text( Output, "freefly", FreeFlyModeFlag );
    }
    export_as_text( Output, "wireframe", bWireFrame );
    export_as_text( Output, "debugmode", DebugModeFlag );
    export_as_text( Output, "soundenabled", bSoundEnabled );
    export_as_text( Output, "sound.openal.renderer", AudioRenderer );
    export_as_text( Output, "sound.volume", AudioVolume );
    export_as_text( Output, "sound.volume.radio", RadioVolume );
    export_as_text( Output, "sound.volume.vehicle", VehicleVolume );
    export_as_text( Output, "sound.volume.positional", EnvironmentPositionalVolume );
    export_as_text( Output, "sound.volume.ambient", EnvironmentAmbientVolume );
    export_as_text( Output, "physicslog", WriteLogFlag );
    export_as_text( Output, "fullphysics", FullPhysics );
    export_as_text( Output, "debuglog", iWriteLogEnabled );
    export_as_text( Output, "multiplelogs", MultipleLogs );
    export_as_text( Output, "logs.filter", DisabledLogTypes );
    Output
        << "mousescale "
        << fMouseXScale << " "
        << fMouseYScale << "\n";
    export_as_text( Output, "mousecontrol", InputMouse );
    export_as_text( Output, "enabletraction", bEnableTraction );
    export_as_text( Output, "loadtraction", bLoadTraction );
    export_as_text( Output, "friction", fFriction );
    export_as_text( Output, "livetraction", bLiveTraction );
    export_as_text( Output, "skyenabled", asSky );
    export_as_text( Output, "defaultext", szDefaultExt );
    export_as_text( Output, "newaircouplers", bnewAirCouplers );
    export_as_text( Output, "anisotropicfiltering", AnisotropicFiltering );
    export_as_text( Output, "usevbo", bUseVBO );
    export_as_text( Output, "feedbackmode", iFeedbackMode );
    export_as_text( Output, "feedbackport", iFeedbackPort );
    export_as_text( Output, "multiplayer", iMultiplayer );
	export_as_text( Output, "isolatedtrainnumber", bIsolatedTrainName);
    export_as_text( Output, "maxtexturesize", iMaxTextureSize );
    export_as_text( Output, "maxcabtexturesize", iMaxCabTextureSize );
    export_as_text( Output, "movelight", fMoveLight );
    export_as_text( Output, "dynamiclights", DynamicLightCount );
    if( std::isnormal( ScenarioTimeOverride ) ) {
        export_as_text( Output, "scenario.time.override", ScenarioTimeOverride );
    }
    export_as_text( Output, "scenario.time.offset", ScenarioTimeOffset );
    export_as_text( Output, "scenario.time.current", ScenarioTimeCurrent );
    export_as_text( Output, "scenario.weather.temperature", AirTemperature );
    export_as_text( Output, "ai.trainman", AITrainman );
    export_as_text( Output, "scalespeculars", ScaleSpecularValues );
    export_as_text( Output, "gfxrenderer", GfxRenderer );
    export_as_text( Output, "shadows", RenderShadows );
    Output
        << "shadowtune "
        << shadowtune.map_size << " "
        << 0 << " "
        << shadowtune.range << " "
        << 0 << "\n";
    export_as_text( Output, "gfx.shadows.cab.range", RenderCabShadowsRange );
    export_as_text( Output, "gfx.smoke", Smoke );
    export_as_text( Output, "gfx.smoke.fidelity", SmokeFidelity );
    export_as_text( Output, "smoothtraction", bSmoothTraction );
    export_as_text( Output, "splinefidelity", SplineFidelity );
    export_as_text( Output, "rendercab", render_cab );
    export_as_text( Output, "createswitchtrackbeds", CreateSwitchTrackbeds );
    export_as_text( Output, "gfx.resource.sweep", ResourceSweep );
    export_as_text( Output, "gfx.resource.move", ResourceMove );
    export_as_text( Output, "gfx.reflections.framerate", 1.0 / reflectiontune.update_interval );
    export_as_text( Output, "gfx.reflections.fidelity", reflectiontune.fidelity );
    export_as_text( Output, "timespeed", fTimeSpeed );
    export_as_text( Output, "multisampling", iMultisampling );
    export_as_text( Output, "latitude", fLatitudeDeg );
    export_as_text( Output, "convertmodels", iConvertModels );
    export_as_text( Output, "file.binary.terrain", file_binary_terrain );
    export_as_text( Output, "inactivepause", bInactivePause );
    export_as_text( Output, "slowmotion", iSlowMotionMask );
    export_as_text( Output, "hideconsole", bHideConsole );
    export_as_text( Output, "rollfix", bRollFix );
    export_as_text( Output, "fpsaverage", fFpsAverage );
    export_as_text( Output, "fpsdeviation", fFpsDeviation );
    for( auto idx = 0; idx < 6; ++idx ) {
        Output
            << "calibrate5din "
            << idx << " "
            << fCalibrateIn[ idx ][ 0 ] << " "
            << fCalibrateIn[ idx ][ 1 ] << " "
            << fCalibrateIn[ idx ][ 2 ] << " "
            << fCalibrateIn[ idx ][ 3 ] << " "
            << fCalibrateIn[ idx ][ 4 ] << " "
            << fCalibrateIn[ idx ][ 5 ] << "\n";
    }
    for( auto idx = 0; idx < 6; ++idx ) {
        Output
            << "calibrate5dout "
            << idx << " "
            << fCalibrateOut[ idx ][ 0 ] << " "
            << fCalibrateOut[ idx ][ 1 ] << " "
            << fCalibrateOut[ idx ][ 2 ] << " "
            << fCalibrateOut[ idx ][ 3 ] << " "
            << fCalibrateOut[ idx ][ 4 ] << " "
            << fCalibrateOut[ idx ][ 5 ] << "\n";
    }
    Output
        << "calibrateoutmaxvalues "
        << fCalibrateOutMax[ 0 ] << " "
        << fCalibrateOutMax[ 1 ] << " "
        << fCalibrateOutMax[ 2 ] << " "
        << fCalibrateOutMax[ 3 ] << " "
        << fCalibrateOutMax[ 4 ] << " "
        << fCalibrateOutMax[ 5 ] << " "
        << fCalibrateOutMax[ 6 ] << "\n";
    export_as_text( Output, "calibrateoutdebuginfo", iCalibrateOutDebugInfo );
    for( auto idx = 0; idx < 7; ++idx ) {
        Output
            << "pwm "
            << idx << " "
            << iPoKeysPWM[ idx ] << "\n";
    }
    export_as_text( Output, "brakestep", fBrakeStep );
    export_as_text( Output, "joinduplicatedevents", bJoinEvents );
    export_as_text( Output, "hiddenevents", iHiddenEvents );
    export_as_text( Output, "pause", ( iPause & 1 ) != 0 );
    export_as_text( Output, "lang", asLang );
    export_as_text( Output, "python.updatetime", PythonScreenUpdateRate );
    Output
        << "uitextcolor "
        << UITextColor.r * 255 << " "
        << UITextColor.g * 255 << " "
        << UITextColor.b * 255 << "\n";
    export_as_text( Output, "ui.bg.opacity", UIBgOpacity );
    export_as_text( Output, "input.gamepad", InputGamepad );
#ifdef WITH_UART
    if( uart_conf.enable ) {
        Output
            << "uart "
            << uart_conf.port << " "
            << uart_conf.baud << " "
            << uart_conf.updatetime << "\n";
    }
    Output
        << "uarttune "
        << uart_conf.mainbrakemin << " "
        << uart_conf.mainbrakemax << " "
        << uart_conf.localbrakemin << " "
        << uart_conf.localbrakemax << " "
        << uart_conf.tankmax << " "
        << uart_conf.tankuart << " "
        << uart_conf.pipemax << " "
        << uart_conf.pipeuart << " "
        << uart_conf.brakemax << " "
        << uart_conf.brakeuart << " "
        << uart_conf.pantographmax << " "
        << uart_conf.pipemax << " "
        << uart_conf.hvmax << " "
        << uart_conf.hvuart << " "
        << uart_conf.currentmax << " "
        << uart_conf.currentuart << " "
        << uart_conf.lvmax << " "
        << uart_conf.lvuart << "\n";
    export_as_text( Output, "uarttachoscale", uart_conf.tachoscale );

    std::vector<std::string> enabled_uartfeatures;
    for(auto const &x : uartfeatures_map) {
        if(*(x.second)) {
            enabled_uartfeatures.push_back(x.first);
        }
    }
    Output << "uartfeature ";
    if(enabled_uartfeatures.empty()) {
        Output << "none\n";
    } else {
        for(auto const &feature : enabled_uartfeatures) {
            Output << feature;
            if(&feature != &enabled_uartfeatures.back()) {
                Output << "|";
            }
        }
        Output << "\n";
    }
    export_as_text( Output, "uartdebug", uart_conf.debug );
#endif

    export_as_text( Output, "extcam.cmd", extcam_cmd );
    export_as_text( Output, "extcam.rec", extcam_rec );
    Output
        << "extcam.res "
        << extcam_res.x << " "
        << extcam_res.y << "\n";

    export_as_text( Output, "compresstex", compress_tex );
    export_as_text( Output, "gfx.framebuffer.width", gfx_framebuffer_width );
    export_as_text( Output, "gfx.framebuffer.height", gfx_framebuffer_height );
    export_as_text( Output, "gfx.shadowmap.enabled", gfx_shadowmap_enabled );
    // TODO: export fpslimit
    export_as_text( Output, "randomseed", Global.random_seed );
    export_as_text( Output, "gfx.envmap.enabled", gfx_envmap_enabled );
    export_as_text( Output, "gfx.postfx.motionblur.enabled", gfx_postfx_motionblur_enabled );
    export_as_text( Output, "gfx.postfx.motionblur.shutter", gfx_postfx_motionblur_shutter );
    // TODO: export gfx_postfx_motionblur_format
    export_as_text( Output, "gfx.postfx.chromaticaberration.enabled", gfx_postfx_chromaticaberration_enabled );
    // TODO: export gfx_format_color
    // TODO: export gfx_format_depth
    export_as_text( Output, "gfx.skiprendering", gfx_skiprendering );
    export_as_text( Output, "gfx.skippipeline", gfx_skippipeline );
    export_as_text( Output, "gfx.extraeffects", gfx_extraeffects );
    export_as_text( Output, "gfx.shadergamma", gfx_shadergamma );
    export_as_text( Output, "gfx.shadow.angle.min", gfx_shadow_angle_min );
    export_as_text( Output, "gfx.shadow.rank.cutoff", gfx_shadow_rank_cutoff );
    export_as_text( Output, "python.enabled", python_enabled );
    export_as_text( Output, "python.threadedupload", python_threadedupload );
    export_as_text( Output, "python.uploadmain", python_uploadmain );
    export_as_text( Output, "python.mipmaps", python_mipmaps );
    for( auto const &server : network_servers ) {
        Output
            << "network.server "
            << server.first << " " << server.second << "\n";
    }
    if( network_client ) {
        Output
            << "network.client "
            << network_client->first << " " << network_client->second << "\n";
    }
    export_as_text( Output, "execonexit", exec_on_exit );
}

template <>
void
global_settings::export_as_text( std::ostream &Output, std::string const Key, std::string const &Value ) const {

    if( Value.empty() ) { return; }

    if( contains( Value, ' ' ) ) {
        Output << Key << " \"" << Value << "\"\n";
    }
    else {
        Output << Key << " " << Value << "\n";
    }
}

template <>
void
global_settings::export_as_text( std::ostream &Output, std::string const Key, bool const &Value ) const {

    Output << Key << " " << ( Value ? "yes" : "no" ) << "\n";
}
