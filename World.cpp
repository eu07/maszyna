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
#include "World.h"

#include "Globals.h"
#include "Logs.h"
#include "MdlMngr.h"
#include "renderer.h"
#include "Timer.h"
#include "mtable.h"
#include "sound.h"
#include "Camera.h"
#include "ResourceManager.h"
#include "Event.h"
#include "Train.h"
#include "Driver.h"
#include "Console.h"
#include "color.h"
#include "uilayer.h"
#include "translation.h"

//---------------------------------------------------------------------------

TDynamicObject *Controlled = NULL; // pojazd, który prowadzimy

std::shared_ptr<ui_panel> UIHeader = std::make_shared<ui_panel>( 20, 20 ); // header ui panel
std::shared_ptr<ui_panel> UITable = std::make_shared<ui_panel>( 20, 100 ); // schedule or scan table
std::shared_ptr<ui_panel> UITranscripts = std::make_shared<ui_panel>( 85, 600 ); // voice transcripts

namespace simulation {

simulation_time Time;

}

#ifdef _WIN32
extern "C"
{
    GLFWAPI HWND glfwGetWin32Window( GLFWwindow* window );
}
#endif

void
simulation_time::init() {

    char monthdaycounts[ 2 ][ 13 ] = {
        { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
        { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 } };
    ::memcpy( m_monthdaycounts, monthdaycounts, sizeof( monthdaycounts ) );

    // cache requested elements, if any
    WORD const requestedhour = m_time.wHour;
    WORD const requestedminute = m_time.wMinute;

#ifdef __linux__
	timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	tm *tms = localtime(&ts.tv_sec);
	m_time.wYear = tms->tm_year;
	m_time.wMonth = tms->tm_mon;
	m_time.wDayOfWeek = tms->tm_wday;
	m_time.wDay = tms->tm_mday;
	m_time.wHour = tms->tm_hour;
	m_time.wMinute = tms->tm_min;
	m_time.wSecond = tms->tm_sec;
	m_time.wMilliseconds = ts.tv_nsec / 1000000;
#elif _WIN32
    ::GetLocalTime( &m_time );
#endif

    if( Global::fMoveLight > 0.0 ) {
        // day and month of the year can be overriden by scenario setup
        daymonth( m_time.wDay, m_time.wMonth, m_time.wYear, static_cast<WORD>( Global::fMoveLight ) );
    }

    if( requestedhour != (WORD)-1 )   { m_time.wHour   = clamp( requestedhour,   static_cast<WORD>( 0 ), static_cast<WORD>( 23 ) ); }
    if( requestedminute != (WORD)-1 ) { m_time.wMinute = clamp( requestedminute, static_cast<WORD>( 0 ), static_cast<WORD>( 59 ) ); }
    // if the time is taken from the local clock leave the seconds intact, otherwise set them to zero
    if( ( requestedhour != (WORD)-1 ) || ( requestedminute != (WORD)-1 ) ) {
        m_time.wSecond = 0;
    }

    m_yearday = yearday( m_time.wDay, m_time.wMonth, m_time.wYear );
}

void
simulation_time::update( double const Deltatime ) {

    m_milliseconds += ( 1000.0 * Deltatime );
    while( m_milliseconds >= 1000.0 ) {

        ++m_time.wSecond;
        m_milliseconds -= 1000.0;
    }
    m_time.wMilliseconds = std::floor( m_milliseconds );
    while( m_time.wSecond >= 60 ) {

        ++m_time.wMinute;
        m_time.wSecond -= 60;
    }
    while( m_time.wMinute >= 60 ) {

        ++m_time.wHour;
        m_time.wMinute -= 60;
    }
    while( m_time.wHour >= 24 ) {

        ++m_time.wDay;
        ++m_time.wDayOfWeek;
        if( m_time.wDayOfWeek >= 7 ) {
            m_time.wDayOfWeek -= 7;
        }
        m_time.wHour -= 24;
    }
    int leap = ( m_time.wYear % 4 == 0 ) && ( m_time.wYear % 100 != 0 ) || ( m_time.wYear % 400 == 0 );
    while( m_time.wDay > m_monthdaycounts[ leap ][ m_time.wMonth ] ) {

        m_time.wDay -= m_monthdaycounts[ leap ][ m_time.wMonth ];
        ++m_time.wMonth;
        // unlikely but we might've entered a new year
        if( m_time.wMonth > 12 ) {

            ++m_time.wYear;
            leap = ( m_time.wYear % 4 == 0 ) && ( m_time.wYear % 100 != 0 ) || ( m_time.wYear % 400 == 0 );
            m_time.wMonth -= 12;
        }
    }
}

int
simulation_time::yearday( int Day, const int Month, const int Year ) {

    char daytab[ 2 ][ 13 ] = {
        { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
        { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
    };
    int i, leap;

    leap = ( Year % 4 == 0 ) && ( Year % 100 != 0 ) || ( Year % 400 == 0 );
    for( i = 1; i < Month; ++i )
        Day += daytab[ leap ][ i ];

    return Day;
}

void
simulation_time::daymonth( WORD &Day, WORD &Month, WORD const Year, WORD const Yearday ) {

    WORD daytab[ 2 ][ 13 ] = {
        { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
        { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
    };

    int leap = ( Year % 4 == 0 ) && ( Year % 100 != 0 ) || ( Year % 400 == 0 );
    WORD idx = 1;
    while( ( idx < 13 ) && ( Yearday >= daytab[ leap ][ idx ] ) ) {

        ++idx;
    }
    Month = idx;
    Day = Yearday - daytab[ leap ][ idx - 1 ];
}

int
simulation_time::julian_day() const {

    int yy = ( m_time.wYear < 0 ? m_time.wYear + 1 : m_time.wYear ) - std::floor( ( 12 - m_time.wMonth ) / 10.f );
    int mm = m_time.wMonth + 9;
    if( mm >= 12 ) { mm -= 12; }

    int K1 = std::floor( 365.25 * ( yy + 4712 ) );
    int K2 = std::floor( 30.6 * mm + 0.5 );

    // for dates in Julian calendar
    int JD = K1 + K2 + m_time.wDay + 59;
    // for dates in Gregorian calendar; 2299160 is October 15th, 1582
    const int gregorianswitchday = 2299160;
    if( JD > gregorianswitchday ) {

        int K3 = std::floor( std::floor( ( yy * 0.01 ) + 49 ) * 0.75 ) - 38;
        JD -= K3;
    }

    return JD;
}

TWorld::TWorld()
{
    // randomize();
    // Randomize();
    Train = NULL;
    // Aspect=1;
    for (int i = 0; i < 10; ++i)
        KeyEvents[i] = NULL; // eventy wyzwalane klawiszami cyfrowymi
    Global::iSlowMotion = 0;
    // Global::changeDynObj=NULL;
    pDynamicNearest = NULL;
    fTimeBuffer = 0.0; // bufor czasu aktualizacji dla stałego kroku fizyki
    fMaxDt = 0.01; //[s] początkowy krok czasowy fizyki
    fTime50Hz = 0.0; // bufor czasu dla komunikacji z PoKeys
}

TWorld::~TWorld()
{
    Global::bManageNodes = false; // Ra: wyłączenie wyrejestrowania, bo się sypie
    TrainDelete();
    // Ground.Free(); //Ra: usunięcie obiektów przed usunięciem dźwięków - sypie się
}

void TWorld::TrainDelete(TDynamicObject *d)
{ // usunięcie pojazdu prowadzonego przez użytkownika
    if (d)
        if (Train)
            if (Train->Dynamic() != d)
                return; // nie tego usuwać
    delete Train; // i nie ma czym sterować
    Train = NULL;
    Controlled = NULL; // tego też już nie ma
    mvControlled = NULL;
    Global::pUserDynamic = NULL; // tego też nie ma
};

/* Ra: do opracowania: wybor karty graficznej ~Intel gdy są dwie...
BOOL GetDisplayMonitorInfo(int nDeviceIndex, LPSTR lpszMonitorInfo)
{
    FARPROC EnumDisplayDevices;
    HINSTANCE  hInstUser32;
    DISPLAY_DEVICE DispDev;
    char szSaveDeviceName[33];  // 32 + 1 for the null-terminator
    BOOL bRet = TRUE;
        HRESULT hr;

    hInstUser32 = LoadLibrary("c:\\windows\User32.DLL");
    if (!hInstUser32) return FALSE;

    // Get the address of the EnumDisplayDevices function
    EnumDisplayDevices = (FARPROC)GetProcAddress(hInstUser32,"EnumDisplayDevicesA");
    if (!EnumDisplayDevices) {
        FreeLibrary(hInstUser32);
        return FALSE;
    }

    ZeroMemory(&DispDev, sizeof(DispDev));
    DispDev.cb = sizeof(DispDev);

    // After the first call to EnumDisplayDevices,
    // DispDev.DeviceString is the adapter name
    if (EnumDisplayDevices(NULL, nDeviceIndex, &DispDev, 0))
        {
                hr = StringCchCopy(szSaveDeviceName, 33, DispDev.DeviceName);
                if (FAILED(hr))
                {
                // TODO: write error handler
                }

        // After second call, DispDev.DeviceString is the
        // monitor name for that device
        EnumDisplayDevices(szSaveDeviceName, 0, &DispDev, 0);

                // In the following, lpszMonitorInfo must be 128 + 1 for
                // the null-terminator.
                hr = StringCchCopy(lpszMonitorInfo, 129, DispDev.DeviceString);
                if (FAILED(hr))
                {
                // TODO: write error handler
                }

    } else    {
        bRet = FALSE;
    }

    FreeLibrary(hInstUser32);

    return bRet;
}
*/

bool TWorld::Init( GLFWwindow *Window ) {

    auto timestart = std::chrono::system_clock::now();

    window = Window;
    Global::window = Window; // do WM_COPYDATA
    Global::pCamera = &Camera; // Ra: wskaźnik potrzebny do likwidacji drgań

    WriteLog( "\nStarting MaSzyna rail vehicle simulator (release: " + Global::asVersion + ")" );
    WriteLog( "For online documentation and additional files refer to: http://eu07.pl");

    UILayer.set_background( "logo" );

    glfwSetWindowTitle( window, ( Global::AppName + " (" + Global::SceneryFile + ")" ).c_str() ); // nazwa scenerii
    UILayer.set_progress(0.01);
    UILayer.set_progress( "Loading scenery / Wczytywanie scenerii" );

    GfxRenderer.Render();

    WriteLog( "World setup..." );
    if( true == Ground.Init( Global::SceneryFile ) ) {
        WriteLog( "...world setup done" );
    }
    else {
        ErrorLog( "...world setup failed" );
        return false;
    }

    simulation::Time.init();

    Environment.init();
    Camera.Init(Global::FreeCameraInit[0], Global::FreeCameraInitAngle[0]);

    UILayer.set_progress( "Preparing train / Przygotowanie kabiny" );
    WriteLog( "Player train init: " + Global::asHumanCtrlVehicle );

    TGroundNode *nPlayerTrain = NULL;
    if (Global::asHumanCtrlVehicle != "ghostview")
        nPlayerTrain = Ground.DynamicFind(Global::asHumanCtrlVehicle); // szukanie w tych z obsadą
    if (nPlayerTrain)
    {
        Train = new TTrain();
        if (Train->Init(nPlayerTrain->DynamicObject))
        {
            Controlled = Train->Dynamic();
            mvControlled = Controlled->ControlledFind()->MoverParameters;
            Global::pUserDynamic = Controlled; // renerowanie pojazdu względem kabiny
            WriteLog("Player train init OK");

            glfwSetWindowTitle( window, ( Global::AppName + " (" + Controlled->MoverParameters->Name + " @ " + Global::SceneryFile + ")" ).c_str() );

            FollowView();
        }
        else
        {
            Error("Player train init failed!");
            FreeFlyModeFlag = true; // Ra: automatycznie włączone latanie
            Controlled = NULL;
            mvControlled = NULL;
            Camera.Type = tp_Free;
        }
    }
    else
    {
        if (Global::asHumanCtrlVehicle != "ghostview")
        {
            Error("Player train doesn't exist!");
        }
        FreeFlyModeFlag = true; // Ra: automatycznie włączone latanie
        glfwSwapBuffers( window );
        Controlled = NULL;
        mvControlled = NULL;
        Camera.Type = tp_Free;
    }

    // if (!Global::bMultiplayer) //na razie włączone
    { // eventy aktywowane z klawiatury tylko dla jednego użytkownika
        KeyEvents[0] = Ground.FindEvent("keyctrl00");
        KeyEvents[1] = Ground.FindEvent("keyctrl01");
        KeyEvents[2] = Ground.FindEvent("keyctrl02");
        KeyEvents[3] = Ground.FindEvent("keyctrl03");
        KeyEvents[4] = Ground.FindEvent("keyctrl04");
        KeyEvents[5] = Ground.FindEvent("keyctrl05");
        KeyEvents[6] = Ground.FindEvent("keyctrl06");
        KeyEvents[7] = Ground.FindEvent("keyctrl07");
        KeyEvents[8] = Ground.FindEvent("keyctrl08");
        KeyEvents[9] = Ground.FindEvent("keyctrl09");
    }

    WriteLog( "Load time: " +
		std::to_string( std::chrono::duration_cast<std::chrono::seconds>(( std::chrono::system_clock::now() - timestart )).count() )
		+ " seconds");
/*
    if (DebugModeFlag) // w Debugmode automatyczne włączenie AI
        if (Train)
            if (Train->Dynamic()->Mechanik)
                Train->Dynamic()->Mechanik->TakeControl(true);
*/
    UILayer.set_progress();
    UILayer.set_progress( "" );
    UILayer.set_background( "" );
    UILayer.clear_texts();

    Timer::ResetTimers();

    // make 4 empty lines for the ui header, to cut down on work down the road
    UIHeader->text_lines.emplace_back( "", Global::UITextColor );
    UIHeader->text_lines.emplace_back( "", Global::UITextColor );
    UIHeader->text_lines.emplace_back( "", Global::UITextColor );
    UIHeader->text_lines.emplace_back( "", Global::UITextColor );
    // bind the panels with ui object. maybe not the best place for this but, eh
    UILayer.push_back( UIHeader );
    UILayer.push_back( UITable );
    UILayer.push_back( UITranscripts );

    return true;
};

void TWorld::OnKeyDown(int cKey)
{
    // dump keypress info in the log
    if( !Global::iPause ) {
        // podczas pauzy klawisze nie działają
        std::string keyinfo;
        auto keyname = glfwGetKeyName( cKey, 0 );
        if( keyname != nullptr ) {
            keyinfo += std::string( keyname );
        }
        else {
            switch( cKey ) {

                case GLFW_KEY_SPACE: { keyinfo += "Space"; break; }
                case GLFW_KEY_ENTER: { keyinfo += "Enter"; break; }
                case GLFW_KEY_ESCAPE: { keyinfo += "Esc"; break; }
                case GLFW_KEY_TAB: { keyinfo += "Tab"; break; }
                case GLFW_KEY_INSERT: { keyinfo += "Insert"; break; }
                case GLFW_KEY_DELETE: { keyinfo += "Delete"; break; }
                case GLFW_KEY_HOME: { keyinfo += "Home"; break; }
                case GLFW_KEY_END: { keyinfo += "End"; break; }
                case GLFW_KEY_F1: { keyinfo += "F1"; break; }
                case GLFW_KEY_F2: { keyinfo += "F2"; break; }
                case GLFW_KEY_F3: { keyinfo += "F3"; break; }
                case GLFW_KEY_F4: { keyinfo += "F4"; break; }
                case GLFW_KEY_F5: { keyinfo += "F5"; break; }
                case GLFW_KEY_F6: { keyinfo += "F6"; break; }
                case GLFW_KEY_F7: { keyinfo += "F7"; break; }
                case GLFW_KEY_F8: { keyinfo += "F8"; break; }
                case GLFW_KEY_F9: { keyinfo += "F9"; break; }
                case GLFW_KEY_F10: { keyinfo += "F10"; break; }
                case GLFW_KEY_F11: { keyinfo += "F11"; break; }
                case GLFW_KEY_F12: { keyinfo += "F12"; break; }
            }
        }
        if( keyinfo.empty() == false ) {

            std::string keymodifiers;
            if( Global::shiftState )
                keymodifiers += "[Shift]+";
            if( Global::ctrlState )
                keymodifiers += "[Ctrl]+";

            WriteLog( "Key pressed: " + keymodifiers + "[" + keyinfo + "]" );
        }
    }

    // actual key processing
    // TODO: redo the input system
    if( ( cKey >= GLFW_KEY_0 ) && ( cKey <= GLFW_KEY_9 ) ) // klawisze cyfrowe
    {
        int i = cKey - GLFW_KEY_0; // numer klawisza
        if (Global::shiftState)
        { // z [Shift] uruchomienie eventu
            if (!Global::iPause) // podczas pauzy klawisze nie działają
                if (KeyEvents[i])
                    Ground.AddToQuery(KeyEvents[i], NULL);
        }
        else // zapamiętywanie kamery może działać podczas pauzy
            if (FreeFlyModeFlag) // w trybie latania można przeskakiwać do ustawionych kamer
                if( ( Global::iTextMode != GLFW_KEY_F12 ) &&
                    ( Global::iTextMode != GLFW_KEY_F3 ) ) // ograniczamy użycie kamer
            {
                if ((!Global::FreeCameraInit[i].x)
                 && (!Global::FreeCameraInit[i].y)
                 && (!Global::FreeCameraInit[i].z))
                { // jeśli kamera jest w punkcie zerowym, zapamiętanie współrzędnych i kątów
                    Global::FreeCameraInit[i] = Camera.Pos;
                    Global::FreeCameraInitAngle[i].x = Camera.Pitch;
                    Global::FreeCameraInitAngle[i].y = Camera.Yaw;
                    Global::FreeCameraInitAngle[i].z = Camera.Roll;
                    // logowanie, żeby można było do scenerii przepisać
                    WriteLog(
                        "camera " + std::to_string( Global::FreeCameraInit[i].x ) + " "
                        + std::to_string(Global::FreeCameraInit[i].y ) + " "
                        + std::to_string(Global::FreeCameraInit[i].z ) + " "
                        + std::to_string(RadToDeg(Global::FreeCameraInitAngle[i].x)) + " "
                        + std::to_string(RadToDeg(Global::FreeCameraInitAngle[i].y)) + " "
                        + std::to_string(RadToDeg(Global::FreeCameraInitAngle[i].z)) + " "
                        + std::to_string(i) + " endcamera");
                }
                else // również przeskakiwanie
                { // Ra: to z tą kamerą (Camera.Pos i Global::pCameraPosition) jest trochę bez sensu
                    Ground.Silence( Global::pCameraPosition ); // wyciszenie wszystkiego z poprzedniej pozycji
                    Global::SetCameraPosition( Global::FreeCameraInit[i] ); // nowa pozycja dla generowania obiektów
                    Camera.Init(Global::FreeCameraInit[i],
                                Global::FreeCameraInitAngle[i]); // przestawienie
                }
            }
        // będzie jeszcze załączanie sprzęgów z [Ctrl]
    }
    else if( ( cKey >= GLFW_KEY_F1 ) && ( cKey <= GLFW_KEY_F12 ) )
    {
        switch (cKey) {
            
            case GLFW_KEY_F1: {
                if( DebugModeFlag ) {
                    // additional time speedup keys in debug mode
                    if( Global::ctrlState ) {
                        // ctrl-f3
                        simulation::Time.update( 20.0 * 60.0 );
                    }
                    else if( Global::shiftState ) {
                        // shift-f3
                        simulation::Time.update( 5.0 * 60.0 );
                    }
                }
                if( ( false == Global::ctrlState )
                 && ( false == Global::shiftState ) ) {
                    // czas i relacja
                    if( Global::iTextMode == cKey ) { ++Global::iScreenMode[ cKey - GLFW_KEY_F1 ]; }
                    if( Global::iScreenMode[ cKey - GLFW_KEY_F1 ] > 1 ) {
                        // wyłączenie napisów
                        Global::iTextMode = 0;
                        Global::iScreenMode[ cKey - GLFW_KEY_F1 ] = 0;
                    }
                    else {
                        Global::iTextMode = cKey;
                    }
                }
                break;
            }
            case GLFW_KEY_F2: {
                // parametry pojazdu
                if( Global::iTextMode == cKey ) { ++Global::iScreenMode[ cKey - GLFW_KEY_F1 ]; }
                if( Global::iScreenMode[ cKey - GLFW_KEY_F1 ] > 1 ) {
                    // wyłączenie napisów
                    Global::iTextMode = 0;
                    Global::iScreenMode[ cKey - GLFW_KEY_F1 ] = 0;
                }
                else {
                    Global::iTextMode = cKey;
                }
                break;
            }
            case GLFW_KEY_F3: {
                // timetable
                if( Global::iTextMode == cKey ) { ++Global::iScreenMode[ cKey - GLFW_KEY_F1 ]; }
                if( Global::iScreenMode[ cKey - GLFW_KEY_F1 ] > 1 ) {
                    // wyłączenie napisów
                    Global::iTextMode = 0;
                    Global::iScreenMode[ cKey - GLFW_KEY_F1 ] = 0;
                }
                else {
                    Global::iTextMode = cKey;
                }
                break;
            }
            case GLFW_KEY_F4: {

                InOutKey( !Global::shiftState ); // distant view with Shift, short distance step out otherwise
                break;
            }
            case GLFW_KEY_F5: {
                // przesiadka do innego pojazdu
                if( false == FreeFlyModeFlag ) {
                    // only available in free fly mode
                    break;
                }

                TDynamicObject *tmp = Ground.DynamicNearest( Camera.Pos, 50, true ); //łapiemy z obsadą
                if( ( tmp != nullptr )
                 && ( tmp != Controlled ) ) {

                    if( Controlled ) // jeśli mielismy pojazd
                        if( Controlled->Mechanik ) // na skutek jakiegoś błędu może czasem zniknąć
                            Controlled->Mechanik->TakeControl( true ); // oddajemy dotychczasowy AI

                    if( DebugModeFlag
                     || (tmp->MoverParameters->Vel <= 5.0) ) {
                        // works always in debug mode, or for stopped/slow moving vehicles otherwise
                        Controlled = tmp; // przejmujemy nowy
                        mvControlled = Controlled->ControlledFind()->MoverParameters;
                        if( Train )
                            Train->Silence(); // wyciszenie dźwięków opuszczanego pojazdu
                        else
                            Train = new TTrain(); // jeśli niczym jeszcze nie jeździlismy
                        if( Train->Init( Controlled ) ) { // przejmujemy sterowanie
                            if( !DebugModeFlag ) // w DebugMode nadal prowadzi AI
                                Controlled->Mechanik->TakeControl( false );
                        }
                        else
                            SafeDelete( Train ); // i nie ma czym sterować
                        // Global::pUserDynamic=Controlled; //renerowanie pojazdu względem kabiny
                        // Global::iTextMode=VK_F4;
                        if( Train )
                            InOutKey(); // do kabiny
                    }
                }
//                Global::iTextMode = cKey;
                break;
            }
            case GLFW_KEY_F6: {
//                Global::iTextMode = cKey;
                // przyspieszenie symulacji do testowania scenerii... uwaga na FPS!
                if( DebugModeFlag ) { 

                    if( Global::ctrlState ) { Global::fTimeSpeed = ( Global::shiftState ? 60.0 : 20.0 ); }
                    else                    { Global::fTimeSpeed = ( Global::shiftState ? 5.0 : 1.0 ); }
                }
                break;
            }
            case GLFW_KEY_F7: {
                // debug mode functions
                if( DebugModeFlag ) {

                    if( Global::ctrlState ) {
                        // ctrl + f7 toggles static daylight
                        ToggleDaylight();
                        break;
                    }
                    else {
                        // f7: wireframe toggle
                        Global::bWireFrame = !Global::bWireFrame;
                        if( true == Global::bWireFrame ) {
                            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
                        }
                        else {
                            glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
                        }
                    }
                }
                break;
            }
            case GLFW_KEY_F8: {
                if( Global::ctrlState
                 && Global::shiftState ) {
                    DebugCameraFlag = !DebugCameraFlag; // taka opcjonalna funkcja, może się czasem przydać
                }
                else {
                    Global::iTextMode = cKey;
                }
                break;
            }
            case GLFW_KEY_F9: {
                Global::iTextMode = cKey;
                // wersja
                break;
            }
            case GLFW_KEY_F10: {
                if( Global::iTextMode == cKey ) {
                    Global::iTextMode =
                        ( Global::iPause && ( cKey != GLFW_KEY_F1 ) ?
                            GLFW_KEY_F1 :
                            0 ); // wyłączenie napisów, chyba że pauza
                }
                else {
                    Global::iTextMode = cKey;
                }
                break;
            }
            case GLFW_KEY_F12: {
                // coś tam jeszcze
                if( Global::ctrlState
                 && Global::shiftState )
                    DebugModeFlag = !DebugModeFlag; // taka opcjonalna funkcja, może się czasem przydać
                else
                    Global::iTextMode = cKey;
                break;
            }
        }
        // if (cKey!=VK_F4)
        return; // nie są przekazywane do pojazdu wcale
    }
    if( Global::iTextMode == GLFW_KEY_F10 ) // wyświetlone napisy klawiszem F10
    { // i potwierdzenie
        if( cKey == GLFW_KEY_Y ) {
            // flaga wyjścia z programu
			glfwSetWindowShouldClose(window, 1);
        }
        return; // nie przekazujemy do pociągu
    }
    else if ((Global::iTextMode == GLFW_KEY_F12) ? (cKey >= '0') && (cKey <= '9') : false)
    { // tryb konfiguracji debugmode (przestawianie kamery już wyłączone
        if (!Global::shiftState) // bez [Shift]
        {
            if (cKey == GLFW_KEY_1)
                Global::iWriteLogEnabled ^= 1; // włącz/wyłącz logowanie do pliku
#ifdef _WIN32
            else if (cKey == GLFW_KEY_2)
            { // włącz/wyłącz okno konsoli
                Global::iWriteLogEnabled ^= 2;
                if ((Global::iWriteLogEnabled & 2) == 0) // nie było okienka
                { // otwarcie okna
                    AllocConsole(); // jeśli konsola już jest, to zwróci błąd; uwalniać nie ma po
                    // co, bo się odłączy
                    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN);
                }
            }
#endif
            // else if (cKey=='3') Global::iWriteLogEnabled^=4; //wypisywanie nazw torów
        }
    }
    else if( cKey == GLFW_KEY_ESCAPE ) {
        // toggle pause
        if( Global::iPause & 1 ) // jeśli pauza startowa
            Global::iPause &= ~1; // odpauzowanie, gdy po wczytaniu miało nie startować
        else if( !( Global::iMultiplayer & 2 ) ) // w multiplayerze pauza nie ma sensu
            Global::iPause ^= 2; // zmiana stanu zapauzowania
        if( Global::iPause ) {// jak pauza
            Global::iTextMode = GLFW_KEY_F1; // to wyświetlić zegar i informację
        }
    }
    else if( ( cKey == GLFW_KEY_PAUSE ) && ( Global::ctrlState ) && ( Global::shiftState ) ) {
        //[Ctrl]+[Break] hamowanie wszystkich pojazdów w okolicy // added shift to prevent odd issue with glfw producing pause presses on its own
		if (Controlled->MoverParameters->Radio)
			Ground.RadioStop(Camera.Pos);
	}
    else if (!Global::iPause) //||(cKey==VK_F4)) //podczas pauzy sterownaie nie działa, F4 tak
        if (Train)
            if (Controlled)
                if ((Controlled->Controller == Humandriver) ? true : DebugModeFlag || (cKey == GLFW_KEY_Q))
                    Train->OnKeyDown(cKey); // przekazanie klawisza do kabiny
    if (FreeFlyModeFlag) // aby nie odluźniało wagonu za lokomotywą
    { // operacje wykonywane na dowolnym pojeździe, przeniesione tu z kabiny
/*
        // NOTE: disabled so it doesn't interfere with new system controls in outside view
        // TODO: implement as part of the new system
        if (cKey == Global::Keys[k_Releaser]) // odluźniacz
        { // działa globalnie, sprawdzić zasięg
            TDynamicObject *temp = Global::DynamicNearest();
            if (temp)
            {
                if (Global::ctrlState) // z ctrl odcinanie
                {
                    temp->MoverParameters->Hamulec->SetBrakeStatus( temp->MoverParameters->Hamulec->GetBrakeStatus() ^ 128 );
                }
                else if (temp->MoverParameters->BrakeReleaser(1))
                {
                    // temp->sBrakeAcc->
                    // dsbPneumaticRelay->SetVolume(DSBVOLUME_MAX);
                    // dsbPneumaticRelay->Play(0,0,0); //temp->Position()-Camera.Pos //???
                }
            }
        }
        else
*/
            if (cKey == Global::Keys[k_Heating]) // Ra: klawisz nie jest najszczęśliwszy
        { // zmiana próżny/ładowny; Ra: zabrane z kabiny
            TDynamicObject *temp = Global::DynamicNearest();
            if (temp)
            {
                if (Global::shiftState ? temp->MoverParameters->IncBrakeMult() :
                                         temp->MoverParameters->DecBrakeMult())
                    if (Train)
                    { // dźwięk oczywiście jest w kabinie
                        Train->dsbSwitch->gain(1.0f).play();
                    }
            }
        }
        else if (cKey == Global::Keys[k_EndSign])
        { // Ra 2014-07: zabrane z kabiny
            TDynamicObject *tmp = Global::CouplerNearest(); // domyślnie wyszukuje do 20m
            if (tmp)
            {
                int CouplNr = (LengthSquared3(tmp->HeadPosition() - Camera.Pos) >
                                       LengthSquared3(tmp->RearPosition() - Camera.Pos) ?
                                   1 :
                                   -1) *
                              tmp->DirectionGet();
                if (CouplNr < 0)
                    CouplNr = 0; // z [-1,1] zrobić [0,1]
                int mask, set = 0; // Ra: [Shift]+[Ctrl]+[T] odpala mi jakąś idiotyczną zmianę
                // tapety pulpitu :/
                if (Global::shiftState) // z [Shift] zapalanie
                    set = mask = 64; // bez [Ctrl] założyć tabliczki
                else if (Global::ctrlState)
                    set = mask = 2 + 32; // z [Ctrl] zapalić światła czerwone
                else
                    mask = 2 + 32 + 64; // wyłączanie ściąga wszystko
                if (((tmp->iLights[CouplNr]) & mask) != set)
                {
                    tmp->iLights[CouplNr] = (tmp->iLights[CouplNr] & ~mask) | set;
                    if (Train)
                    { // Ra: ten dźwięk z kabiny to przegięcie, ale na razie zostawiam
                        Train->dsbSwitch->gain(1.0f).play();
                    }
                }
            }
        }
        else if (cKey == Global::Keys[k_IncLocalBrakeLevel])
        { // zahamowanie dowolnego pojazdu
            TDynamicObject *temp = Global::DynamicNearest();
            if (temp)
            {
                if (Global::ctrlState)
                    if ((temp->MoverParameters->LocalBrake == ManualBrake) ||
                        (temp->MoverParameters->MBrake == true))
                        temp->MoverParameters->IncManualBrakeLevel(1);
                    else
                        ;
                else if (temp->MoverParameters->LocalBrake != ManualBrake)
                    if (temp->MoverParameters->IncLocalBrakeLevelFAST())
                        if (Train)
                        { // dźwięk oczywiście jest w kabinie
                            Train->dsbPneumaticRelay->gain(0.5f).play();
                        }
            }
        }
        else if (cKey == Global::Keys[k_DecLocalBrakeLevel])
        { // odhamowanie dowolnego pojazdu
            TDynamicObject *temp = Global::DynamicNearest();
            if (temp)
            {
                if (Global::ctrlState)
                    if ((temp->MoverParameters->LocalBrake == ManualBrake) ||
                        (temp->MoverParameters->MBrake == true))
                        temp->MoverParameters->DecManualBrakeLevel(1);
                    else
                        ;
                else if (temp->MoverParameters->LocalBrake != ManualBrake)
                    if (temp->MoverParameters->DecLocalBrakeLevelFAST())
                        if (Train)
                        { // dźwięk oczywiście jest w kabinie
                            Train->dsbPneumaticRelay->gain(0.5f).play();
                        }
            }
        }
    }
    // switch (cKey)
    //{case 'a': //ignorowanie repetycji
    // case 'A': Global::iKeyLast=cKey; break;
    // default: Global::iKeyLast=0;
    //}
}

void TWorld::OnMouseMove(double x, double y)
{ // McZapkie:060503-definicja obracania myszy
    Camera.OnCursorMove(x * Global::fMouseXScale / Global::ZoomFactor, -y * Global::fMouseYScale / Global::ZoomFactor);
}

void TWorld::InOutKey( bool const Near )
{ // przełączenie widoku z kabiny na zewnętrzny i odwrotnie
    FreeFlyModeFlag = !FreeFlyModeFlag; // zmiana widoku
    if (FreeFlyModeFlag)
    { // jeżeli poza kabiną, przestawiamy w jej okolicę - OK
        Global::pUserDynamic = NULL; // bez renderowania względem kamery
        if (Train)
        { // Train->Dynamic()->ABuSetModelShake(vector3(0,0,0));
            Train->Silence(); // wyłączenie dźwięków kabiny
            Train->Dynamic()->bDisplayCab = false;
            DistantView( Near );
        }
    }
    else
    { // jazda w kabinie
        if (Train)
        {
            Global::pUserDynamic = Controlled; // renerowanie względem kamery
            Train->Dynamic()->bDisplayCab = true;
            Train->Dynamic()->ABuSetModelShake(
                vector3(0, 0, 0)); // zerowanie przesunięcia przed powrotem?
            // Camera.Stop(); //zatrzymanie ruchu
            Train->MechStop();
            FollowView(); // na pozycję mecha
        }
        else
            FreeFlyModeFlag = true; // nadal poza kabiną
    }
    // update window title to reflect the situation
    glfwSetWindowTitle(
        window,
        ( Global::AppName
        + " ("
        + ( Controlled != nullptr ?
            Controlled->MoverParameters->Name :
            "" )
        + " @ "
        + Global::SceneryFile
        + ")" ).c_str() );
};

// places camera outside the controlled vehicle, or nearest if nothing is under control
// depending on provided switch the view is placed right outside, or at medium distance
void TWorld::DistantView( bool const Near )
{ // ustawienie widoku pojazdu z zewnątrz

    TDynamicObject const *vehicle{ nullptr };
         if( nullptr != Controlled )      { vehicle = Controlled; }
    else if( nullptr != pDynamicNearest ) { vehicle = pDynamicNearest; }
    else                                  { return; }

    auto const cab =
        ( vehicle->MoverParameters->ActiveCab == 0 ?
            1 :
            vehicle->MoverParameters->ActiveCab );
    auto const left = vehicle->VectorLeft() * cab;

    if( true == Near ) {

        Camera.Pos =
            vector3( Camera.Pos.x, vehicle->GetPosition().y, Camera.Pos.z )
            + left * vehicle->GetWidth()
            + vector3( 1.25 * left.x, 1.6, 1.25 * left.z );
    }
    else {

        Camera.Pos =
            vehicle->GetPosition()
            + vehicle->VectorFront() * vehicle->MoverParameters->ActiveCab * 50.0
            + vector3( -10.0 * left.x, 1.6, -10.0 * left.z );
    }

    Camera.LookAt = vehicle->GetPosition();
    Camera.RaLook(); // jednorazowe przestawienie kamery
};

// ustawienie śledzenia pojazdu
void TWorld::FollowView(bool wycisz) {

    Camera.Reset(); // likwidacja obrotów - patrzy horyzontalnie na południe

    if (Controlled) // jest pojazd do prowadzenia?
    {
        Ground.Silence( Camera.Pos ); // wyciszenie dźwięków z poprzedniej pozycji

        if (FreeFlyModeFlag)
        { // jeżeli poza kabiną, przestawiamy w jej okolicę - OK
            if( Train ) {
                // wyłączenie trzęsienia na siłę?
                Train->Dynamic()->ABuSetModelShake( vector3() );
            }

            DistantView(); // przestawienie kamery
            //żeby nie bylo numerów z 'fruwajacym' lokiem - konsekwencja bujania pudła
            // tu ustawić nową, bo od niej liczą się odległości
            Global::SetCameraPosition( Camera.Pos );
        }
        else if (Train)
        { // korekcja ustawienia w kabinie - OK
            if( wycisz ) {
                // wyciszenie dźwięków z poprzedniej pozycji
                // trzymanie prawego w kabinie daje marny efekt
                Ground.Silence( Camera.Pos );
            }
            Camera.Pos = Train->pMechPosition;
            Camera.Roll = std::atan(Train->pMechShake.x * Train->fMechRoll); // hustanie kamery na boki
            Camera.Pitch -= 0.5 * std::atan(Train->vMechVelocity.z * Train->fMechPitch); // hustanie kamery przod tyl

            if( Train->Dynamic()->MoverParameters->ActiveCab == 0 ) {
                Camera.LookAt =
                      Train->pMechPosition
                    + Train->GetDirection() * 5.0;
            }
            else {
                // patrz w strone wlasciwej kabiny
                Camera.LookAt =
                      Train->pMechPosition
                    + Train->GetDirection() * 5.0 * Train->Dynamic()->MoverParameters->ActiveCab;
            }
            Train->pMechOffset = Train->pMechSittingPosition;
        }
    }
    else
        DistantView();
};

bool TWorld::Update()
{
#ifdef USE_SCENERY_MOVING
    vector3 tmpvector = Global::GetCameraPosition();
    tmpvector = vector3(-int(tmpvector.x) + int(tmpvector.x) % 10000,
                        -int(tmpvector.y) + int(tmpvector.y) % 10000,
                        -int(tmpvector.z) + int(tmpvector.z) % 10000);
    if (tmpvector.x || tmpvector.y || tmpvector.z)
    {
        WriteLog("Moving scenery");
        Ground.MoveGroundNode(tmpvector);
        WriteLog("Scenery moved");
    };
#endif

    Timer::UpdateTimers(Global::iPause != 0);

    if( (Global::iPause == false)
     || (m_init == false) ) {
        // jak pauza, to nie ma po co tego przeliczać
        simulation::Time.update( Timer::GetDeltaTime() );
        // Ra 2014-07: przeliczenie kąta czasu (do animacji zależnych od czasu)
        auto const &time = simulation::Time.data();
        Global::fTimeAngleDeg = time.wHour * 15.0 + time.wMinute * 0.25 + ( ( time.wSecond + 0.001 * time.wMilliseconds ) / 240.0 );
        Global::fClockAngleDeg[ 0 ] = 36.0 * ( time.wSecond % 10 ); // jednostki sekund
        Global::fClockAngleDeg[ 1 ] = 36.0 * ( time.wSecond / 10 ); // dziesiątki sekund
        Global::fClockAngleDeg[ 2 ] = 36.0 * ( time.wMinute % 10 ); // jednostki minut
        Global::fClockAngleDeg[ 3 ] = 36.0 * ( time.wMinute / 10 ); // dziesiątki minut
        Global::fClockAngleDeg[ 4 ] = 36.0 * ( time.wHour % 10 ); // jednostki godzin
        Global::fClockAngleDeg[ 5 ] = 36.0 * ( time.wHour / 10 ); // dziesiątki godzin

        Update_Environment();
    } // koniec działań niewykonywanych podczas pauzy

    // fixed step, simulation time based updates

    double dt = Timer::GetDeltaTime(); // 0.0 gdy pauza
/*
    fTimeBuffer += dt; //[s] dodanie czasu od poprzedniej ramki
*/
    m_primaryupdateaccumulator += dt;
    m_secondaryupdateaccumulator += dt;
/*
    if (fTimeBuffer >= fMaxDt) // jest co najmniej jeden krok; normalnie 0.01s
    { // Ra: czas dla fizyki jest skwantowany - fizykę lepiej przeliczać stałym krokiem
        // tak można np. moc silników itp., ale ruch musi być przeliczany w każdej klatce, bo
        // inaczej skacze
        Global::tranTexts.Update(); // obiekt obsługujący stenogramy dźwięków na ekranie
#ifdef _WIN32
        Console::Update(); // obsługa cykli PoKeys (np. aktualizacja wyjść analogowych)
#endif
        double iter =
            ceil(fTimeBuffer / fMaxDt); // ile kroków się zmieściło od ostatniego sprawdzania?
        int n = int(iter); // ile kroków jako int
        fTimeBuffer -= iter * fMaxDt; // reszta czasu na potem (do bufora)
        if (n > 20)
            n = 20; // Ra: jeżeli FPS jest zatrważająco niski, to fizyka nie może zająć całkowicie procesora
    }
*/
/*
    // NOTE: until we have no physics state interpolation during render, we need to rely on the old code,
    // as doing fixed step calculations but flexible step render results in ugly mini jitter
    // core routines (physics)
    int updatecount = 0;
    while( ( m_primaryupdateaccumulator >= m_primaryupdaterate )
         &&( updatecount < 20 ) ) {
        // no more than 20 updates per single pass, to keep physics from hogging up all run time
        Ground.Update( m_primaryupdaterate, 1 );
        ++updatecount;
        m_primaryupdateaccumulator -= m_primaryupdaterate;
    }
*/
    int updatecount = 1;
    if( dt > m_primaryupdaterate ) // normalnie 0.01s
    {
/*
        // NOTE: experimentally disabled physics update cap
        auto const iterations = std::ceil(dt / m_primaryupdaterate);
        updatecount = std::min( 20, static_cast<int>( iterations ) );
*/
        updatecount = std::ceil( dt / m_primaryupdaterate );
/*
        // NOTE: changing dt wrecks things further down the code. re-acquire proper value later or cleanup here
        dt = dt / iterations; // Ra: fizykę lepiej by było przeliczać ze stałym krokiem
*/
    }
    // NOTE: updates are limited to 20, but dt is distributed over potentially many more iterations
    // this means at count > 20 simulation and render are going to desync. is that right?
    // NOTE: experimentally changing this to prevent the desync.
    // TODO: test what happens if we hit more than 20 * 0.01 sec slices, i.e. less than 5 fps
    for( int updateidx = 0; updateidx < updatecount; ++updateidx ) {
        Ground.Update( dt / updatecount, 1 ); // tu zrobić tylko coklatkową aktualizację przesunięć
    }

    // yB dodał przyspieszacz fizyki
    if( (true == DebugModeFlag)
     && (true == Global::bActive) // nie przyspieszać, gdy jedzie w tle :)
     && ( glfwGetKey( window, GLFW_KEY_PAUSE ) == GLFW_PRESS ) ) {

        Ground.Update( dt, 1 );
        Ground.Update( dt, 1 );
        Ground.Update( dt, 1 );
        Ground.Update( dt, 1 ); // 5 razy
    }
    // secondary fixed step simulation time routines

    while( m_secondaryupdateaccumulator >= m_secondaryupdaterate ) {

        Global::tranTexts.Update(); // obiekt obsługujący stenogramy dźwięków na ekranie

        // awaria PoKeys mogła włączyć pauzę - przekazać informację
        if( Global::iMultiplayer ) // dajemy znać do serwera o wykonaniu
            if( iPause != Global::iPause ) { // przesłanie informacji o pauzie do programu nadzorującego
                Ground.WyslijParam( 5, 3 ); // ramka 5 z czasem i stanem zapauzowania
                iPause = Global::iPause;
            }

        // fixed step part of the camera update
        if( ( Train != nullptr )
         && ( Camera.Type == tp_Follow )
         && ( false == DebugCameraFlag ) ) {
            // jeśli jazda w kabinie, przeliczyć trzeba parametry kamery
            Train->UpdateMechPosition( m_secondaryupdaterate );
        }

        m_secondaryupdateaccumulator -= m_secondaryupdaterate; // these should be inexpensive enough we have no cap
    }

    // variable step simulation time routines

    if( Global::changeDynObj ) {
        // ABu zmiana pojazdu - przejście do innego
        ChangeDynamic();
    }

    if( Train != nullptr ) {
        TSubModel::iInstance = reinterpret_cast<size_t>( Train->Dynamic() );
        Train->Update( dt );
    }
    else {
        TSubModel::iInstance = 0;
    }

    Ground.CheckQuery();

    Ground.Update_Lights();

	{
		glm::dmat4 cam_matrix;
		Camera.SetMatrix(cam_matrix);

		glm::vec3 pos(Camera.Pos.x, Camera.Pos.y, Camera.Pos.z);
	    glm::vec3 at = glm::vec3(0.0, 0.0, -1.0) * glm::mat3(cam_matrix);
	    glm::vec3 up = glm::vec3(0.0, 1.0, 0.0) * glm::mat3(cam_matrix);

	    sound_man->set_listener(pos, at, up);
	    sound_man->update(dt);
	}

    // render time routines follow:

    dt = Timer::GetDeltaRenderTime(); // nie uwzględnia pauzowania ani mnożenia czasu

    // fixed step render time routines

    fTime50Hz += dt; // w pauzie też trzeba zliczać czas, bo przy dużym FPS będzie problem z odczytem ramek
    while( fTime50Hz >= 1.0 / 50.0 ) {
#ifdef _WIN32
        Console::Update(); // to i tak trzeba wywoływać
#endif
        Update_UI();
        // decelerate camera
        Camera.Velocity *= 0.65;
        if( std::abs( Camera.Velocity.x ) < 0.01 ) { Camera.Velocity.x = 0.0; }
        if( std::abs( Camera.Velocity.y ) < 0.01 ) { Camera.Velocity.y = 0.0; }
        if( std::abs( Camera.Velocity.z ) < 0.01 ) { Camera.Velocity.z = 0.0; }
        // decelerate debug camera too
        DebugCamera.Velocity *= 0.65;
        if( std::abs( DebugCamera.Velocity.x ) < 0.01 ) { DebugCamera.Velocity.x = 0.0; }
        if( std::abs( DebugCamera.Velocity.y ) < 0.01 ) { DebugCamera.Velocity.y = 0.0; }
        if( std::abs( DebugCamera.Velocity.z ) < 0.01 ) { DebugCamera.Velocity.z = 0.0; }

        fTime50Hz -= 1.0 / 50.0;
    }

    // variable step render time routines

    Update_Camera( dt );

    GfxRenderer.Update( dt );
    ResourceSweep();

    m_init = true;
  
    return true;
};

void
TWorld::Update_Camera( double const Deltatime ) {

    if( false == Global::ControlPicking ) {
        if( glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_LEFT ) == GLFW_PRESS ) {
            Camera.Reset(); // likwidacja obrotów - patrzy horyzontalnie na południe
            if( Controlled && LengthSquared3( Controlled->GetPosition() - Camera.Pos ) < ( 1500 * 1500 ) ) {
                // gdy bliżej niż 1.5km
                Camera.LookAt = Controlled->GetPosition();
            }
            else {
                TDynamicObject *d =
                    Ground.DynamicNearest( Camera.Pos, 300 ); // szukaj w promieniu 300m
                if( !d )
                    d = Ground.DynamicNearest( Camera.Pos, 1000 ); // dalej szukanie, jesli bliżej nie ma
                if( d && pDynamicNearest ) {
                    // jeśli jakiś jest znaleziony wcześniej
                    if( 100.0 * LengthSquared3( d->GetPosition() - Camera.Pos ) > LengthSquared3( pDynamicNearest->GetPosition() - Camera.Pos ) ) {
                        d = pDynamicNearest; // jeśli najbliższy nie jest 10 razy bliżej niż
                    }
                }
                // poprzedni najbliższy, zostaje poprzedni
                if( d )
                    pDynamicNearest = d; // zmiana na nowy, jeśli coś znaleziony niepusty
                if( pDynamicNearest )
                    Camera.LookAt = pDynamicNearest->GetPosition();
            }
            if( FreeFlyModeFlag )
                Camera.RaLook(); // jednorazowe przestawienie kamery
        }
        else if( glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_RIGHT ) == GLFW_PRESS ) {
            FollowView( false ); // bez wyciszania dźwięków
        }
        else if( glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_MIDDLE ) == GLFW_PRESS ) {
            // middle mouse button controls zoom.
            Global::ZoomFactor = std::min( 4.5f, Global::ZoomFactor + 15.0f * static_cast<float>( Deltatime ) );
        }
        else if( glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_MIDDLE ) != GLFW_PRESS ) {
            // reset zoom level if the button is no longer held down.
            // NOTE: yes, this is terrible way to go about it. it'll do for now.
            Global::ZoomFactor = std::max( 1.0f, Global::ZoomFactor - 15.0f * static_cast<float>( Deltatime ) );
        }
    }

    if( DebugCameraFlag ) { DebugCamera.Update(); }
    else                  { Camera.Update(); } // uwzględnienie ruchu wywołanego klawiszami

    if( ( Train != nullptr )
     && ( Camera.Type == tp_Follow )
     && ( false == DebugCameraFlag ) ) {
        // jeśli jazda w kabinie, przeliczyć trzeba parametry kamery
        vector3 tempangle = Controlled->VectorFront() * ( Controlled->MoverParameters->ActiveCab == -1 ? -1 : 1 );
        double modelrotate = atan2( -tempangle.x, tempangle.z );

        if( (Global::ctrlState)
         && ( (Console::Pressed( Global::Keys[ k_MechLeft ])
           || (Console::Pressed( Global::Keys[ k_MechRight ]))))) {
            // jeśli lusterko lewe albo prawe (bez rzucania na razie)
            bool lr = Console::Pressed( Global::Keys[ k_MechLeft ] );
            // Camera.Yaw powinno być wyzerowane, aby po powrocie patrzeć do przodu
            Camera.Pos = Controlled->GetPosition() + Train->MirrorPosition( lr ); // pozycja lusterka
            Camera.Yaw = 0; // odchylenie na bok od Camera.LookAt
            if( Train->Dynamic()->MoverParameters->ActiveCab == 0 )
                Camera.LookAt = Camera.Pos - Train->GetDirection(); // gdy w korytarzu
            else if( Global::shiftState ) {
                // patrzenie w bok przez szybę
                Camera.LookAt = Camera.Pos - ( lr ? -1 : 1 ) * Train->Dynamic()->VectorLeft() * Train->Dynamic()->MoverParameters->ActiveCab;
                Global::SetCameraRotation( -modelrotate );
            }
            else { // patrzenie w kierunku osi pojazdu, z uwzględnieniem kabiny - jakby z lusterka,
                // ale bez odbicia
                Camera.LookAt = Camera.Pos - Train->GetDirection() * Train->Dynamic()->MoverParameters->ActiveCab; //-1 albo 1
                Global::SetCameraRotation( M_PI - modelrotate ); // tu już trzeba uwzględnić lusterka
            }
            Camera.Roll = std::atan( Train->pMechShake.x * Train->fMechRoll ); // hustanie kamery na boki
            Camera.Pitch = 0.5 * std::atan( Train->vMechVelocity.z * Train->fMechPitch ); // hustanie kamery przod tyl
            Camera.vUp = Controlled->VectorUp();
        }
        else {
            // patrzenie standardowe
            Camera.Pos = Train->GetWorldMechPosition(); // Train.GetPosition1();
            if( !Global::iPause ) { // podczas pauzy nie przeliczać kątów przypadkowymi wartościami
                Camera.Roll = atan( Train->pMechShake.x * Train->fMechRoll ); // hustanie kamery na boki
                Camera.Pitch -= 0.5 * atan( Train->vMechVelocity.z * Train->fMechPitch ); // hustanie kamery przod tyl //Ra: tu
                // jest uciekanie kamery w górę!!!
            }
            // ABu011104: rzucanie pudlem
/*
            vector3 temp;
            if( abs( Train->pMechShake.y ) < 0.25 )
                temp = vector3( 0, 0, 6 * Train->pMechShake.y );
            else if( ( Train->pMechShake.y ) > 0 )
                temp = vector3( 0, 0, 6 * 0.25 );
            else
                temp = vector3( 0, 0, -6 * 0.25 );
            if( Controlled )
                Controlled->ABuSetModelShake( temp );
            // ABu: koniec rzucania
*/

            if( Train->Dynamic()->MoverParameters->ActiveCab == 0 )
                Camera.LookAt = Train->GetWorldMechPosition() + Train->GetDirection() * 5.0; // gdy w korytarzu
            else // patrzenie w kierunku osi pojazdu, z uwzględnieniem kabiny
                Camera.LookAt = Train->GetWorldMechPosition() + Train->GetDirection() * 5.0 * Train->Dynamic()->MoverParameters->ActiveCab; //-1 albo 1
            Camera.vUp = Train->GetUp();
            Global::SetCameraRotation( Camera.Yaw - modelrotate ); // tu już trzeba uwzględnić lusterka
        }
    }
    else { // kamera nieruchoma
        Global::SetCameraRotation( Camera.Yaw - M_PI );
    }
    // all done, update camera position to the new value
    Global::pCameraPosition = Camera.Pos;
    Global::DebugCameraPosition = DebugCamera.Pos;
}

void TWorld::Update_Environment() {

    Environment.update();
}

void TWorld::ResourceSweep()
{
/*
    ResourceManager::Sweep( Timer::GetSimulationTime() );
*/
};

void
TWorld::Update_UI() {

    UITable->text_lines.clear();
    std::string  uitextline1, uitextline2, uitextline3, uitextline4;
    UILayer.set_tooltip( "" );

    if( ( Train != nullptr ) && ( false == FreeFlyModeFlag ) ) {
        if( false == DebugModeFlag ) {
            // in regular mode show control functions, for defined controls
            UILayer.set_tooltip( locale::label_cab_control( Train->GetLabel( GfxRenderer.Pick_Control() ) ) );
        }
        else {
            // in debug mode show names of submodels, to help with cab setup and/or debugging
            auto const cabcontrol = GfxRenderer.Pick_Control();
            UILayer.set_tooltip( ( cabcontrol ? cabcontrol->pName : "" ) );
        }
    }
    if( ( true == Global::ControlPicking ) && ( true == FreeFlyModeFlag ) && ( true == DebugModeFlag ) ) {
        auto const scenerynode = GfxRenderer.Pick_Node();
        UILayer.set_tooltip( ( scenerynode ? scenerynode->asName : "" ) );
    }

    switch( Global::iTextMode ) {

        case( GLFW_KEY_F1 ) : {
            // f1, default mode: current time and timetable excerpt
            auto const &time = simulation::Time.data();
            uitextline1 =
                "Time: "
                + to_string( time.wHour ) + ":"
                + ( time.wMinute < 10 ? "0" : "" ) + to_string( time.wMinute ) + ":"
                + ( time.wSecond < 10 ? "0" : "" ) + to_string( time.wSecond );
            if( Global::iPause ) {
                uitextline1 += " (paused)";
            }

            if( Controlled && ( Controlled->Mechanik != nullptr ) ) {

                auto const &mover = Controlled->MoverParameters;
                auto const &driver = Controlled->Mechanik;

                uitextline2 = "Throttle: " + to_string( driver->Controlling()->MainCtrlPos, 0, 2 ) + "+" + std::to_string( driver->Controlling()->ScndCtrlPos );
                     if( mover->ActiveDir > 0 ) { uitextline2 += " D"; }
                else if( mover->ActiveDir < 0 ) { uitextline2 += " R"; }
                else                            { uitextline2 += " N"; }

                uitextline3 = "Brakes:" + to_string( mover->fBrakeCtrlPos, 1, 5 ) + "+" + std::to_string( mover->LocalBrakePos ) + ( mover->SlippingWheels ? " !" : "  " );

                if( Global::iScreenMode[ Global::iTextMode - GLFW_KEY_F1 ] == 1 ) {
                    // detail mode on second key press
                    uitextline2 +=
                        " Speed: " + std::to_string( static_cast<int>( std::floor( mover->Vel ) ) ) + " km/h"
                        + " (limit: " + std::to_string( static_cast<int>( std::floor( driver->VelDesired ) ) ) + " km/h"
                        + ", next limit: " + std::to_string( static_cast<int>( std::floor( Controlled->Mechanik->VelNext ) ) ) + " km/h"
                        + " in " + to_string( Controlled->Mechanik->ActualProximityDist * 0.001, 1 ) + " km)";
                    uitextline3 +=
                        " Pressure: " + to_string( mover->BrakePress * 100.0, 2 ) + " kPa"
                        + " (train pipe: " + to_string( mover->PipePress * 100.0, 2 ) + " kPa)";
                }
            }

            break;
        }

        case( GLFW_KEY_F2 ) : {
            // timetable
            TDynamicObject *tmp =
                ( FreeFlyModeFlag ?
                Ground.DynamicNearest( Camera.Pos ) :
                Controlled ); // w trybie latania lokalizujemy wg mapy

            if( tmp == nullptr ) { break; }
            // if the nearest located vehicle doesn't have a direct driver, try to query its owner
            auto const owner = (
                ( ( tmp->Mechanik != nullptr ) && ( tmp->Mechanik->Primary() ) ) ?
                    tmp->Mechanik :
                    tmp->ctOwner );
            if( owner == nullptr ){ break; }

            auto const table = owner->Timetable();
            if( table == nullptr ) { break; }

            auto const &time = simulation::Time.data();
            uitextline1 =
                "Time: "
                + to_string( time.wHour ) + ":"
                + ( time.wMinute < 10 ? "0" : "" ) + to_string( time.wMinute ) + ":"
                + ( time.wSecond < 10 ? "0" : "" ) + to_string( time.wSecond );
            if( Global::iPause ) {
                uitextline1 += " (paused)";
            }

            uitextline2 = Global::Bezogonkow( owner->Relation(), true ) + " (" + Global::Bezogonkow( owner->Timetable()->TrainName, true ) + ")";
            auto const nextstation = Global::Bezogonkow( owner->NextStop(), true );
            if( !nextstation.empty() ) {
                // jeśli jest podana relacja, to dodajemy punkt następnego zatrzymania
                uitextline3 = " -> " + nextstation;
            }

            if( Global::iScreenMode[ Global::iTextMode - GLFW_KEY_F1 ] == 1 ) {

                if( 0 == table->StationCount ) {
                    // only bother if there's stations to list
                    UITable->text_lines.emplace_back( "(no timetable)", Global::UITextColor );
                } 
                else {
                    // header
                    UITable->text_lines.emplace_back( "+----------------------------+-------+-------+-----+", Global::UITextColor );

                    TMTableLine *tableline;
                    for( int i = owner->iStationStart; i <= std::min( owner->iStationStart + 15, table->StationCount ); ++i ) {
                        // wyświetlenie pozycji z rozkładu
                        tableline = table->TimeTable + i; // linijka rozkładu

                        std::string station =
                            ( tableline->StationName + "                          " ).substr( 0, 26 );
                        std::string arrival =
                            ( tableline->Ah >= 0 ?
                            to_string( int( 100 + tableline->Ah ) ).substr( 1, 2 ) + ":" + to_string( int( 100 + tableline->Am ) ).substr( 1, 2 ) :
                            "     " );
                        std::string departure =
                            ( tableline->Dh >= 0 ?
                            to_string( int( 100 + tableline->Dh ) ).substr( 1, 2 ) + ":" + to_string( int( 100 + tableline->Dm ) ).substr( 1, 2 ) :
                            "     " );
                        std::string vmax =
                            "   "
                            + to_string( tableline->vmax, 0 );
                        vmax = vmax.substr( vmax.length() - 3, 3 ); // z wyrównaniem do prawej

                        UITable->text_lines.emplace_back(
                            Global::Bezogonkow( "| " + station + " | " + arrival + " | " + departure + " | " + vmax + " | " + tableline->StationWare, true ),
                            ( ( owner->iStationStart < table->StationIndex )
                           && ( i < table->StationIndex )
                           && ( ( time.wHour * 60 + time.wMinute ) >= ( tableline->Dh * 60 + tableline->Dm ) ) ?
                                float4( 0.0f, 1.0f, 0.0f, 1.0f ) :// czas minął i odjazd był, to nazwa stacji będzie na zielono
                                Global::UITextColor ) );
                        // divider/footer
                        UITable->text_lines.emplace_back( "+----------------------------+-------+-------+-----+", Global::UITextColor );
                    }
                }
            }

            break;
        }

        case( GLFW_KEY_F3 ) : {

            TDynamicObject *tmp =
                ( FreeFlyModeFlag ?
                Ground.DynamicNearest( Camera.Pos ) :
                Controlled ); // w trybie latania lokalizujemy wg mapy

            if( tmp != nullptr ) {
                // 
                // jeśli domyślny ekran po pierwszym naciśnięciu
                uitextline1 = "Vehicle name: " + tmp->MoverParameters->Name;

                if( ( tmp->Mechanik == nullptr ) && ( tmp->ctOwner ) ) {
                    // for cars other than leading unit indicate the leader
                    uitextline1 += ", owned by " + tmp->ctOwner->OwnerName();
                }
                uitextline1 += "; Status: " + tmp->MoverParameters->EngineDescription( 0 );
                // informacja o sprzęgach
                uitextline1 +=
                    "; C0:" +
                    ( tmp->PrevConnected ?
                        tmp->PrevConnected->GetName() + ":" + to_string( tmp->MoverParameters->Couplers[ 0 ].CouplingFlag ) + (
                            tmp->MoverParameters->Couplers[ 0 ].CouplingFlag == 0 ?
                                " (" + to_string( tmp->MoverParameters->Couplers[ 0 ].CoupleDist, 1 ) + " m)" :
                                "" ) :
                        "none" );
                uitextline1 +=
                    " C1:" +
                    ( tmp->NextConnected ?
                        tmp->NextConnected->GetName() + ":" + to_string( tmp->MoverParameters->Couplers[ 1 ].CouplingFlag ) + (
                            tmp->MoverParameters->Couplers[ 1 ].CouplingFlag == 0 ?
                                " (" + to_string( tmp->MoverParameters->Couplers[ 1 ].CoupleDist, 1 ) + " m)" :
                                "" ) :
                        "none" );

                // equipment flags
                uitextline2  = ( tmp->MoverParameters->Battery ? "B" : "." );
                uitextline2 += ( tmp->MoverParameters->Mains ? "M" : "." );
                uitextline2 += ( tmp->MoverParameters->PantRearUp ? ( tmp->MoverParameters->PantRearVolt > 0.0 ? "O" : "o" ) : "." );;
                uitextline2 += ( tmp->MoverParameters->PantFrontUp ? ( tmp->MoverParameters->PantFrontVolt > 0.0 ? "P" : "p" ) : "." );;
                uitextline2 += ( tmp->MoverParameters->PantPressLockActive ? "!" : ( tmp->MoverParameters->PantPressSwitchActive ? "*" : "." ) );
                uitextline2 += ( false == tmp->MoverParameters->ConverterAllowLocal ? "-" : ( tmp->MoverParameters->ConverterAllow ? ( tmp->MoverParameters->ConverterFlag ? "X" : "x" ) : "." ) );
                uitextline2 += ( tmp->MoverParameters->ConvOvldFlag ? "!" : "." );
                uitextline2 += ( false == tmp->MoverParameters->CompressorAllowLocal ? "-" : ( tmp->MoverParameters->CompressorAllow ? ( tmp->MoverParameters->CompressorFlag ? "C" : "c" ) : "." ) );
                uitextline2 += ( tmp->MoverParameters->CompressorGovernorLock ? "!" : "." );
/*
                uitextline2 +=
                    " AnlgB: " + to_string( tmp->MoverParameters->AnPos, 1 )
                    + "+"
                    + to_string( tmp->MoverParameters->LocalBrakePosA, 1 )
*/
                uitextline2 += " Bdelay: ";
                if( ( tmp->MoverParameters->BrakeDelayFlag & bdelay_G ) == bdelay_G )
                    uitextline2 += "G";
                if( ( tmp->MoverParameters->BrakeDelayFlag & bdelay_P ) == bdelay_P )
                    uitextline2 += "P";
                if( ( tmp->MoverParameters->BrakeDelayFlag & bdelay_R ) == bdelay_R )
                    uitextline2 += "R";
                if( ( tmp->MoverParameters->BrakeDelayFlag & bdelay_M ) == bdelay_M )
                    uitextline2 += "+Mg";

                uitextline2 += ", Load: " + to_string( tmp->MoverParameters->LoadFlag, 0 );

                uitextline2 +=
                    "; Pant: "
                    + to_string( tmp->MoverParameters->PantPress, 2 )
                    + ( tmp->MoverParameters->bPantKurek3 ? "-ZG" : "|ZG" );

                uitextline2 +=
                    "; Ft: " + to_string( tmp->MoverParameters->Ft * 0.001f * tmp->MoverParameters->ActiveCab, 1 )
                    + ", Fb: " + to_string( tmp->MoverParameters->Fb * 0.001f, 1 )
                    + ", Fr: " + to_string( tmp->MoverParameters->RunningTrack.friction, 2 )
                    + ( tmp->MoverParameters->SlippingWheels ? " (!)" : "" );

                uitextline2 +=
                    "; TC:"
                    + to_string( tmp->MoverParameters->TotalCurrent, 0 );
                auto const frontcouplerhighvoltage =
                    to_string( tmp->MoverParameters->Couplers[ TMoverParameters::side::front ].power_high.voltage, 0 )
                    + "@"
                    + to_string( tmp->MoverParameters->Couplers[ TMoverParameters::side::front ].power_high.current, 0 );
                auto const rearcouplerhighvoltage =
                    to_string( tmp->MoverParameters->Couplers[ TMoverParameters::side::rear ].power_high.voltage, 0 )
                    + "@"
                    + to_string( tmp->MoverParameters->Couplers[ TMoverParameters::side::rear ].power_high.current, 0 );
                uitextline2 += ", HV: ";
                if( tmp->MoverParameters->Couplers[ TMoverParameters::side::front ].power_high.local == false ) {
                    uitextline2 +=
                            "(" + frontcouplerhighvoltage + ")-"
                        + ":F" + ( tmp->DirectionGet() ? "<<" : ">>" ) + "R:"
                        + "-(" + rearcouplerhighvoltage + ")";
                }
                else {
                    uitextline2 +=
                            frontcouplerhighvoltage
                        + ":F" + ( tmp->DirectionGet() ? "<<" : ">>" ) + "R:"
                        + rearcouplerhighvoltage;
                }

                uitextline3 +=
                    "TrB: " + to_string( tmp->MoverParameters->BrakePress, 2 )
                    + ", " + to_hex_str( tmp->MoverParameters->Hamulec->GetBrakeStatus(), 2 );

                uitextline3 +=
                    "; LcB: " + to_string( tmp->MoverParameters->LocBrakePress, 2 )
                    + "; pipes: " + to_string( tmp->MoverParameters->PipePress, 2 )
                    + "/" + to_string( tmp->MoverParameters->ScndPipePress, 2 )
                    + "/" + to_string( tmp->MoverParameters->EqvtPipePress, 2 )
                    + ", MT: " + to_string( tmp->MoverParameters->CompressedVolume, 3 )
                    + ", BT: " + to_string( tmp->MoverParameters->Volume, 3 )
                    + ", CtlP: " + to_string( tmp->MoverParameters->CntrlPipePress, 3 )
                    + ", CtlT: " + to_string( tmp->MoverParameters->Hamulec->GetCRP(), 3 );

                if( tmp->MoverParameters->ManualBrakePos > 0 ) {

                    uitextline3 += "; manual brake on";
                }
/*
                if( tmp->MoverParameters->LocalBrakePos > 0 ) {

                    uitextline3 += ", local brake on";
                }
                else {

                    uitextline3 += ", local brake off";
                }
*/
                if( tmp->Mechanik ) {
                    // o ile jest ktoś w środku
                    std::string flags = "cpapcplhhndoiefgvdpseil "; // flagi AI (definicja w Driver.h)
                    for( int i = 0, j = 1; i < 23; ++i, j <<= 1 )
                        if( false == ( tmp->Mechanik->DrivigFlags() & j ) ) // jak bit ustawiony
                            flags[ i ] = '.';// std::toupper( flags[ i ] ); // ^= 0x20; // to zmiana na wielką literę

                    uitextline4 = flags;

                    uitextline4 +=
                        "Driver: Vd=" + to_string( tmp->Mechanik->VelDesired, 0 )
                        + " Ad=" + to_string( tmp->Mechanik->AccDesired, 2 )
						+ " Ah=" + to_string( tmp->Mechanik->fAccThreshold, 2 )
						+ "@" + to_string( tmp->Mechanik->fBrake_a0[0], 2 )
						+ "+" + to_string( tmp->Mechanik->fBrake_a1[0], 2 )
                        + " Bd=" + to_string( tmp->Mechanik->fBrakeDist, 0 )
                        + " Pd=" + to_string( tmp->Mechanik->ActualProximityDist, 0 )
                        + " Vn=" + to_string( tmp->Mechanik->VelNext, 0 )
                        + " VSl=" + to_string( tmp->Mechanik->VelSignalLast, 0 )
                        + " VLl=" + to_string( tmp->Mechanik->VelLimitLast, 0 )
                        + " VRd=" + to_string( tmp->Mechanik->VelRoad, 0 );

                    if( ( tmp->Mechanik->VelNext == 0.0 )
                     && ( tmp->Mechanik->eSignNext ) ) {
                        // jeśli ma zapamiętany event semafora, nazwa eventu semafora
                        uitextline4 +=
                            " ("
                            + Global::Bezogonkow( tmp->Mechanik->eSignNext->asName )
                            + ")";
                    }

                    // biezaca komenda dla AI
                    uitextline4 += ", command: " + tmp->Mechanik->OrderCurrent();
                }

                if( Global::iScreenMode[ Global::iTextMode - GLFW_KEY_F1 ] == 1 ) {
                    // f2 screen, track scan mode
                    if( tmp->Mechanik == nullptr ) {
                        //żeby była tabelka, musi być AI
                        break;
                    }

                    std::size_t i = 0; std::size_t const speedtablesize = clamp( static_cast<int>( tmp->Mechanik->TableSize() ) - 1, 0, 30 );
                    do {
                        std::string scanline = tmp->Mechanik->TableText( i );
                        if( scanline.empty() ) { break; }
                        UITable->text_lines.emplace_back( Global::Bezogonkow( scanline ), Global::UITextColor );
                        ++i;
                    } while( i < speedtablesize ); // TController:iSpeedTableSize TODO: change when the table gets recoded
                }
            }
            else {
                // wyświetlenie współrzędnych w scenerii oraz kąta kamery, gdy nie mamy wskaźnika
                uitextline1 =
                    "Camera position: "
                    + to_string( Camera.Pos.x, 2 ) + " "
                    + to_string( Camera.Pos.y, 2 ) + " "
                    + to_string( Camera.Pos.z, 2 )
                    + ", azimuth: "
                    + to_string( 180.0 - RadToDeg( Camera.Yaw ), 0 ) // ma być azymut, czyli 0 na północy i rośnie na wschód
                    + " "
                    + std::string( "S SEE NEN NWW SW" )
                    .substr( 0 + 2 * floor( fmod( 8 + ( Camera.Yaw + 0.5 * M_PI_4 ) / M_PI_4, 8 ) ), 2 );
                // current luminance level
                uitextline2 = "Light level: " + to_string( Global::fLuminance, 3 );
                if( Global::FakeLight ) { uitextline2 += "(*)"; }
            }

            break;
        }

        case( GLFW_KEY_F8 ) : {
            // gfx renderer data
            uitextline1 =
                "FoV: " + to_string( Global::FieldOfView / Global::ZoomFactor, 1 )
                + ", Draw range x " + to_string( Global::fDistanceFactor, 1 )
//                + "; sectors: " + std::to_string( GfxRenderer.m_drawcount )
//                + ", FPS: " + to_string( Timer::GetFPS(), 2 );
                + ", FPS: " + std::to_string( static_cast<int>(std::round(GfxRenderer.Framerate())) );
            if( Global::iSlowMotion ) {
                uitextline1 += " (slowmotion " + to_string( Global::iSlowMotion ) + ")";
            }



            // dump last opengl error, if any
            GLenum glerror = ::glGetError();
            if( glerror != GL_NO_ERROR ) {
                std::string glerrorstring( (char *)::gluErrorString( glerror ) );
                win1250_to_ascii( glerrorstring );
                Global::LastGLError = std::to_string( glerror ) + " (" + glerrorstring + ")";
            }
            if( false == Global::LastGLError.empty() ) {
                uitextline2 +=
                    "Last openGL error: "
                    + Global::LastGLError;
            }
            // renderer stats
            uitextline3 = GfxRenderer.Info();

            break;
        }

        case( GLFW_KEY_F9 ) : {
            // informacja o wersji
            uitextline1 = "MaSzyna " + Global::asVersion; // informacja o wersji
            if( Global::iMultiplayer ) {
                uitextline1 += " (multiplayer mode is active)";
            }

            break;
        }

        case( GLFW_KEY_F10 ) : {

            uitextline1 = ( "Press [Y] key to quit / Aby zakonczyc program, przycisnij klawisz [Y]." );

            break;
        }

        case( GLFW_KEY_F12 ) : {
            // opcje włączenia i wyłączenia logowania
            uitextline1 = "[0] Debugmode " + std::string( DebugModeFlag ? "(on)" : "(off)" );
            uitextline2 = "[1] log.txt " + std::string( ( Global::iWriteLogEnabled & 1 ) ? "(on)" : "(off)" );
            uitextline3 = "[2] Console " + std::string( ( Global::iWriteLogEnabled & 2 ) ? "(on)" : "(off)" );

            break;
        }

        default: {
            // uncovered cases, nothing to do here...
            // ... unless we're in debug mode
            if( DebugModeFlag ) {

                TDynamicObject *tmp =
                    ( FreeFlyModeFlag ?
                        Ground.DynamicNearest( Camera.Pos ) :
                        Controlled ); // w trybie latania lokalizujemy wg mapy
                if( tmp == nullptr ) {
                    break;
                }

                uitextline1 =
                    "vel: " + to_string( tmp->GetVelocity(), 2 ) + " km/h" + ( tmp->MoverParameters->SlippingWheels ? " (!)" : "" )
                    + "; dist: " + to_string( tmp->MoverParameters->DistCounter, 2 ) + " km"
                    + "; pos: ("
                    + to_string( tmp->GetPosition().x, 2 ) + ", "
                    + to_string( tmp->GetPosition().y, 2 ) + ", "
                    + to_string( tmp->GetPosition().z, 2 )
                    + ")";

                uitextline2 =
                    "HamZ=" + to_string( tmp->MoverParameters->fBrakeCtrlPos, 2 )
                    + "; HamP=" + std::to_string( tmp->MoverParameters->LocalBrakePos ) + "/" + to_string( tmp->MoverParameters->LocalBrakePosA, 2 )
                    + "; NasJ=" + std::to_string( tmp->MoverParameters->MainCtrlPos ) + "(" + std::to_string( tmp->MoverParameters->MainCtrlActualPos ) + ")"
                    + "; NasB=" + std::to_string( tmp->MoverParameters->ScndCtrlPos ) + "(" + std::to_string( tmp->MoverParameters->ScndCtrlActualPos ) + ")"
                    + "; I=" +
                    ( tmp->MoverParameters->TrainType == dt_EZT ?
                        std::to_string( int( tmp->MoverParameters->ShowCurrent( 0 ) ) ) :
                       std::to_string( int( tmp->MoverParameters->Im ) ) )
                    + "; U=" + to_string( int( tmp->MoverParameters->RunningTraction.TractionVoltage + 0.5 ) )
                    + "; R=" +
                    ( std::abs( tmp->MoverParameters->RunningShape.R ) > 10000.0 ?
                        "~0.0" :
                        to_string( tmp->MoverParameters->RunningShape.R, 1 ) )
                    + " An=" + to_string( tmp->MoverParameters->AccN, 2 ); // przyspieszenie poprzeczne

                if( tprev != simulation::Time.data().wSecond ) {
                    tprev = simulation::Time.data().wSecond;
                    Acc = ( tmp->MoverParameters->Vel - VelPrev ) / 3.6;
                    VelPrev = tmp->MoverParameters->Vel;
                }
                uitextline2 += ( "; As=" ) + to_string( Acc, 2 ); // przyspieszenie wzdłużne

                uitextline3 =
                    "cyl.ham. " + to_string( tmp->MoverParameters->BrakePress, 2 )
                    + "; prz.gl. " + to_string( tmp->MoverParameters->PipePress, 2 )
                    + "; zb.gl. " + to_string( tmp->MoverParameters->CompressedVolume, 2 )
                    // youBy - drugi wezyk
                    + "; p.zas. " + to_string( tmp->MoverParameters->ScndPipePress, 2 );
 
                // McZapkie: warto wiedziec w jakim stanie sa przelaczniki
                if( tmp->MoverParameters->ConvOvldFlag )
                    uitextline3 += " C! ";
                else if( tmp->MoverParameters->FuseFlag )
                    uitextline3 += " F! ";
                else if( !tmp->MoverParameters->Mains )
                    uitextline3 += " () ";
                else {
                    switch(
                        tmp->MoverParameters->ActiveDir *
                        ( tmp->MoverParameters->Imin == tmp->MoverParameters->IminLo ?
                            1 :
                            2 ) ) {
                        case  2: { uitextline3 += " >> "; break; }
                        case  1: { uitextline3 += " -> "; break; }
                        case  0: { uitextline3 += " -- "; break; }
                        case -1: { uitextline3 += " <- "; break; }
                        case -2: { uitextline3 += " << "; break; }
                    }
                }
                // McZapkie: predkosc szlakowa
                if( tmp->MoverParameters->RunningTrack.Velmax == -1 ) {
                    uitextline3 += " Vtrack=Vmax";
                }
                else {
                    uitextline3 += " Vtrack " + to_string( tmp->MoverParameters->RunningTrack.Velmax, 2 );
                }

                if( ( tmp->MoverParameters->EnginePowerSource.SourceType == CurrentCollector )
                    || ( tmp->MoverParameters->TrainType == dt_EZT ) ) {
                    uitextline3 +=
                        "; pant. " + to_string( tmp->MoverParameters->PantPress, 2 )
                        + ( tmp->MoverParameters->bPantKurek3 ? "=" : "^" ) + "ZG";
                }

                // McZapkie: komenda i jej parametry
                if( tmp->MoverParameters->CommandIn.Command != ( "" ) ) {
                    uitextline4 =
                        "C:" + tmp->MoverParameters->CommandIn.Command
                        + " V1=" + to_string( tmp->MoverParameters->CommandIn.Value1, 0 )
                        + " V2=" + to_string( tmp->MoverParameters->CommandIn.Value2, 0 );
                }
                if( ( tmp->Mechanik )
                 && ( tmp->Mechanik->AIControllFlag == AIdriver ) ) {
                    uitextline4 +=
                        "AI: Vd=" + to_string( tmp->Mechanik->VelDesired, 0 )
                        + " ad=" + to_string(tmp->Mechanik->AccDesired, 2)
						+ "/" + to_string(tmp->Mechanik->AccDesired*tmp->Mechanik->BrakeAccFactor(), 2)
						+ " atrain=" + to_string(tmp->Mechanik->fBrake_a0[0], 2)
						+ "+" + to_string(tmp->Mechanik->fBrake_a1[0], 2)
						+ " aS=" + to_string(tmp->Mechanik->AbsAccS_pub, 2)
                        + " Pd=" + to_string( tmp->Mechanik->ActualProximityDist, 0 )
                        + " Vn=" + to_string( tmp->Mechanik->VelNext, 0 );
                }

                // induction motor data
                if( tmp->MoverParameters->EngineType == ElectricInductionMotor ) {

                    UITable->text_lines.emplace_back( "      eimc:            eimv:            press:", Global::UITextColor );
                    for( int i = 0; i <= 20; ++i ) {

                        std::string parameters =
                            tmp->MoverParameters->eimc_labels[ i ] + to_string( tmp->MoverParameters->eimc[ i ], 2, 9 )
                            + " | "
                            + tmp->MoverParameters->eimv_labels[ i ] + to_string( tmp->MoverParameters->eimv[ i ], 2, 9 );

                        if( i < 10 ) {
                            parameters += " | " + Train->fPress_labels[i] + to_string( Train->fPress[ i ][ 0 ], 2, 9 );
                        }
                        else if( i == 12 ) {
                            parameters += "        med:";
                        }
                        else if( i >= 13 ) {
                            parameters += " | " + tmp->MED_labels[ i - 13 ] + to_string( tmp->MED[ 0 ][ i - 13 ], 2, 9 );
                        }

                        UITable->text_lines.emplace_back( parameters, Global::UITextColor );
                    }
                }

            } // if( DebugModeFlag && Controlled )

            break;
        }
    }

#ifdef EU07_USE_OLD_UI_CODE
    if( Controlled && DebugModeFlag && !Global::iTextMode ) {

        uitextline1 +=
            ( "; d_omega " ) + to_string( Controlled->MoverParameters->dizel_engagedeltaomega, 3 );

        if( Controlled->MoverParameters->EngineType == ElectricInductionMotor ) {

            for( int i = 0; i <= 8; i++ ) {
                for( int j = 0; j <= 9; j++ ) {
                    glRasterPos2f( 0.05f + 0.03f * i, 0.16f - 0.01f * j );
                    uitextline4 = to_string( Train->fEIMParams[ i ][ j ], 2 );
                }
            }
        }
    }
#endif

    // update the ui header texts
    auto &headerdata = UIHeader->text_lines;
    headerdata[ 0 ].data = uitextline1;
    headerdata[ 1 ].data = uitextline2;
    headerdata[ 2 ].data = uitextline3;
    headerdata[ 3 ].data = uitextline4;

    // stenogramy dźwięków (ukryć, gdy tabelka skanowania lub rozkład?)
    auto &transcripts = UITranscripts->text_lines;
    transcripts.clear();
    for( auto const &transcript : Global::tranTexts.aLines ) {

        if( Global::fTimeAngleDeg >= transcript.fShow ) {

            cParser parser( transcript.asText );
            while( true == parser.getTokens( 1, false, "|" ) ) {

                std::string transcriptline; parser >> transcriptline;
                transcripts.emplace_back( transcriptline, float4( 1.0f, 1.0f, 0.0f, 1.0f ) );
            }
        }
    }
}

//---------------------------------------------------------------------------
void TWorld::OnCommandGet(DaneRozkaz *pRozkaz)
{ // odebranie komunikatu z serwera
    if (pRozkaz->iSygn == MAKE_ID4('E','U','0','7') )
        switch (pRozkaz->iComm)
        {
        case 0: // odesłanie identyfikatora wersji
			CommLog( Now() + " " + std::to_string(pRozkaz->iComm) + " version" + " rcvd");
			Ground.WyslijString(Global::asVersion, 0); // przedsatwienie się
            break;
        case 1: // odesłanie identyfikatora wersji
			CommLog( Now() + " " + std::to_string(pRozkaz->iComm) + " scenery" + " rcvd");
			Ground.WyslijString(Global::SceneryFile, 1); // nazwa scenerii
            break;
        case 2: // event
            CommLog( Now() + " " + std::to_string(pRozkaz->iComm) + " " +
                    std::string(pRozkaz->cString + 1, (unsigned)(pRozkaz->cString[0])) + " rcvd");
            if (Global::iMultiplayer)
            { // WriteLog("Komunikat: "+AnsiString(pRozkaz->Name1));
                TEvent *e = Ground.FindEvent(
                    std::string(pRozkaz->cString + 1, (unsigned)(pRozkaz->cString[0])));
                if (e)
                    if ((e->Type == tp_Multiple) || (e->Type == tp_Lights) ||
                        (e->evJoined != 0))  // tylko jawne albo niejawne Multiple
                        Ground.AddToQuery(e, NULL); // drugi parametr to dynamic wywołujący - tu brak
            }
            break;
        case 3: // rozkaz dla AI
            if (Global::iMultiplayer)
            {
                int i =
                    int(pRozkaz->cString[8]); // długość pierwszego łańcucha (z przodu dwa floaty)
                CommLog(
                    Now() + " " + to_string(pRozkaz->iComm) + " " +
                    std::string(pRozkaz->cString + 11 + i, (unsigned)(pRozkaz->cString[10 + i])) +
                    " rcvd");
                TGroundNode *t = Ground.DynamicFind(
                    std::string(pRozkaz->cString + 11 + i,
                               (unsigned)pRozkaz->cString[10 + i])); // nazwa pojazdu jest druga
                if (t)
                    if (t->DynamicObject->Mechanik)
                    {
                        t->DynamicObject->Mechanik->PutCommand(std::string(pRozkaz->cString + 9, i),
                                                               pRozkaz->fPar[0], pRozkaz->fPar[1],
                                                               NULL, stopExt); // floaty są z przodu
                        WriteLog("AI command: " + std::string(pRozkaz->cString + 9, i));
                    }
            }
            break;
        case 4: // badanie zajętości toru
        {
            CommLog(Now() + " " + to_string(pRozkaz->iComm) + " " +
                    std::string(pRozkaz->cString + 1, (unsigned)(pRozkaz->cString[0])) + " rcvd");
            TGroundNode *t = Ground.FindGroundNode(
                std::string(pRozkaz->cString + 1, (unsigned)(pRozkaz->cString[0])), TP_TRACK);
            if (t)
                if (t->pTrack->IsEmpty())
                    Ground.WyslijWolny(t->asName);
        }
        break;
        case 5: // ustawienie parametrów
        {
            CommLog(Now() + " " + to_string(pRozkaz->iComm) + " params " +
                    to_string(*pRozkaz->iPar) + " rcvd");
            if (*pRozkaz->iPar == 0) // sprawdzenie czasu
                if (*pRozkaz->iPar & 1) // ustawienie czasu
                {
                    double t = pRozkaz->fPar[1];
                    simulation::Time.data().wDay = std::floor(t); // niby nie powinno być dnia, ale...
                    if (Global::fMoveLight >= 0)
                        Global::fMoveLight = t; // trzeba by deklinację Słońca przeliczyć
                    simulation::Time.data().wHour = std::floor(24 * t) - 24.0 * simulation::Time.data().wDay;
                    simulation::Time.data().wMinute = std::floor(60 * 24 * t) - 60.0 * (24.0 * simulation::Time.data().wDay + simulation::Time.data().wHour);
                    simulation::Time.data().wSecond = std::floor( 60 * 60 * 24 * t ) - 60.0 * ( 60.0 * ( 24.0 * simulation::Time.data().wDay + simulation::Time.data().wHour ) + simulation::Time.data().wMinute );
                }
            if (*pRozkaz->iPar & 2)
            { // ustawienie flag zapauzowania
                Global::iPause = pRozkaz->fPar[2]; // zakładamy, że wysyłający wie, co robi
            }
        }
        break;
        case 6: // pobranie parametrów ruchu pojazdu
            if (Global::iMultiplayer)
            { // Ra 2014-12: to ma działać również dla pojazdów bez obsady
                CommLog(Now() + " " + to_string(pRozkaz->iComm) + " " +
                        std::string(pRozkaz->cString + 1, (unsigned)(pRozkaz->cString[0])) +
                        " rcvd");
                if (pRozkaz->cString[0]) // jeśli długość nazwy jest niezerowa
                { // szukamy pierwszego pojazdu o takiej nazwie i odsyłamy parametry ramką #7
                    TGroundNode *t;
                    if (pRozkaz->cString[1] == '*')
                        t = Ground.DynamicFind(
                            Global::asHumanCtrlVehicle); // nazwa pojazdu użytkownika
                    else
                        t = Ground.DynamicFindAny(std::string(
                            pRozkaz->cString + 1, (unsigned)pRozkaz->cString[0])); // nazwa pojazdu
                    if (t)
                        Ground.WyslijNamiary(t); // wysłanie informacji o pojeździe
                }
                else
                { // dla pustego wysyłamy ramki 6 z nazwami pojazdów AI (jeśli potrzebne wszystkie,
                    // to rozpoznać np. "*")
                    Ground.DynamicList();
                }
            }
            break;
        case 8: // ponowne wysłanie informacji o zajętych odcinkach toru
			CommLog(Now() + " " + to_string(pRozkaz->iComm) + " all busy track" + " rcvd");
			Ground.TrackBusyList();
            break;
        case 9: // ponowne wysłanie informacji o zajętych odcinkach izolowanych
			CommLog(Now() + " " + to_string(pRozkaz->iComm) + " all busy isolated" + " rcvd");
			Ground.IsolatedBusyList();
            break;
        case 10: // badanie zajętości jednego odcinka izolowanego
            CommLog(Now() + " " + to_string(pRozkaz->iComm) + " " +
                    std::string(pRozkaz->cString + 1, (unsigned)(pRozkaz->cString[0])) + " rcvd");
            Ground.IsolatedBusy(std::string(pRozkaz->cString + 1, (unsigned)(pRozkaz->cString[0])));
            break;
        case 11: // ustawienie parametrów ruchu pojazdu
            //    Ground.IsolatedBusy(AnsiString(pRozkaz->cString+1,(unsigned)(pRozkaz->cString[0])));
            break;
		case 12: // skrocona ramka parametrow pojazdow AI (wszystkich!!)
			CommLog(Now() + " " + to_string(pRozkaz->iComm) + " obsadzone" + " rcvd");
			Ground.WyslijObsadzone();
			//    Ground.IsolatedBusy(AnsiString(pRozkaz->cString+1,(unsigned)(pRozkaz->cString[0])));
			break;
		case 13: // ramka uszkodzenia i innych stanow pojazdu, np. wylaczenie CA, wlaczenie recznego itd.
				 //            WriteLog("Przyszlo 13!");
				 //            WriteLog(pRozkaz->cString);
                    CommLog(Now() + " " + to_string(pRozkaz->iComm) + " " +
                            std::string(pRozkaz->cString + 1, (unsigned)(pRozkaz->cString[0])) +
                            " rcvd");
                    if (pRozkaz->cString[1]) // jeśli długość nazwy jest niezerowa
                        { // szukamy pierwszego pojazdu o takiej nazwie i odsyłamy parametry ramką #13
				TGroundNode *t;
				if (pRozkaz->cString[2] == '*')
					t = Ground.DynamicFind(
						Global::asHumanCtrlVehicle); // nazwa pojazdu użytkownika
				else
					t = Ground.DynamicFindAny(
						std::string(pRozkaz->cString + 2,
							(unsigned)pRozkaz->cString[1])); // nazwa pojazdu
				if (t)
				{
					TDynamicObject *d = t->DynamicObject;
					while (d)
					{
						d->Damage(pRozkaz->cString[0]);
						d = d->Next(); // pozostałe też
					}
					d = t->DynamicObject->Prev();
					while (d)
					{
						d->Damage(pRozkaz->cString[0]);
						d = d->Prev(); // w drugą stronę też
					}
					Ground.WyslijUszkodzenia(t->asName, t->DynamicObject->MoverParameters->EngDmgFlag); // zwrot informacji o pojeździe
				}
			}
			//    Ground.IsolatedBusy(AnsiString(pRozkaz->cString+1,(unsigned)(pRozkaz->cString[0])));
			break;
		}
};

//---------------------------------------------------------------------------
void TWorld::ModifyTGA(const std::string &dir)
{ // rekurencyjna modyfikacje plików TGA
/*  TODO: implement version without Borland stuff
	TSearchRec sr;
    if (FindFirst(dir + "*.*", faDirectory | faArchive, sr) == 0)
    {
        do
        {
            if (sr.Name[1] != '.')
                if ((sr.Attr & faDirectory)) // jeśli katalog, to rekurencja
                    ModifyTGA(dir + sr.Name + "/");
                else if (sr.Name.LowerCase().SubString(sr.Name.Length() - 3, 4) == ".tga")
                    TTexturesManager::GetTextureID(NULL, NULL, AnsiString(dir + sr.Name).c_str());
        } while (FindNext(sr) == 0);
        FindClose(sr);
    }
*/
};

//---------------------------------------------------------------------------

void TWorld::CreateE3D(std::string const &Path, bool Dynamic)
{ // rekurencyjna generowanie plików E3D

    std::string last; // zmienne używane w rekurencji
    TTrack *trk{ nullptr };
    double at{ 0.0 };
    double shift{ 0.0 };

#ifdef _WINDOWS

	std::string searchpattern( "*.*" );

	::WIN32_FIND_DATA filedata;
	::HANDLE search = ::FindFirstFile( ( Path + searchpattern ).c_str(), &filedata );

	if( search == INVALID_HANDLE_VALUE ) { return; } // if nothing to do, bail out

	do {

		std::string filename = filedata.cFileName;

		if( filedata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
			// launch recursive search for sub-directories...
			if( filename == "." ) { continue; }
			if( filename == ".." ) { continue; }
			CreateE3D( Path + filename + "\\", Dynamic );
		}
		else {
            // process the file
            if( filename.size() < 4 ) { continue; }
            std::string const filetype = ToLower( filename.substr( filename.size() - 4, 4 ) );
            if( filetype == ".mmd" ) {
                if( false == Dynamic ) { continue; }

                // konwersja pojazdów będzie ułomna, bo nie poustawiają się animacje na submodelach określonych w MMD
                if( last != Path ) { // utworzenie toru dla danego pojazdu
                    last = Path;
                    trk = TTrack::Create400m( 1, shift );
                    shift += 10.0; // następny tor będzie deczko dalej, aby nie zabić FPS
                    at = 400.0;
                }
                TGroundNode *tmp = new TGroundNode();
                tmp->DynamicObject = new TDynamicObject();

                at -= tmp->DynamicObject->Init(
                    "",
                    Path.substr( 8, Path.size() - 9 ), // skip leading "dynamic/" and trailing slash
                    "none",
                    filename.substr( 0, filename.size() - 4 ),
                    trk,
                    at,
                    "nobody", 0.0, "none", 0.0, "", false, "" );

                // po wczytaniu CHK zrobić pętlę po ładunkach, aby każdy z nich skonwertować
                cParser loadparser( tmp->DynamicObject->MoverParameters->LoadAccepted ); // typy ładunków
                std::string loadname;
                loadparser.getTokens( 1, true, "," ); loadparser >> loadname;
                while( loadname != "" ) {

                    if( ( true == FileExists( Path + loadname + ".t3d" ) )
                     && ( false == FileExists( Path + loadname + ".e3d" ) ) ) {
                            // a nie ma jeszcze odpowiednika binarnego
                            at -= tmp->DynamicObject->Init(
                                "",
                                Path.substr( 8, Path.size() - 9 ), // skip leading "dynamic/" and trailing slash
                                "none",
                                filename.substr( 0, filename.size() - 4 ),
                                trk,
                                at,
                                "nobody", 0.0, "none", 1.0, loadname, false, "" );
                        }

                    loadname = "";
                    loadparser.getTokens( 1, true, "," ); loadparser >> loadname;
                }

                if( tmp->DynamicObject->iCabs ) { // jeśli ma jakąkolwiek kabinę
                    delete Train;
                    Train = new TTrain();
                    if( tmp->DynamicObject->iCabs & 1 ) {
                        tmp->DynamicObject->MoverParameters->ActiveCab = 1;
                        Train->Init( tmp->DynamicObject, true );
                    }
                    if( tmp->DynamicObject->iCabs & 4 ) {
                        tmp->DynamicObject->MoverParameters->ActiveCab = -1;
                        Train->Init( tmp->DynamicObject, true );
                    }
                    if( tmp->DynamicObject->iCabs & 2 ) {
                        tmp->DynamicObject->MoverParameters->ActiveCab = 0;
                        Train->Init( tmp->DynamicObject, true );
                    }
                }
                // z powrotem defaultowa sciezka do tekstur
                Global::asCurrentTexturePath = ( szTexturePath );
            }
            else if( filetype == ".t3d" ) {
                // z modelami jest prościej
                    Global::asCurrentTexturePath = Path;
                    TModelsManager::GetModel( Path + filename, false );
            }
		}

	} while( ::FindNextFile( search, &filedata ) );

	// all done, clean up
	::FindClose( search );

#endif
};

//---------------------------------------------------------------------------
void TWorld::CabChange(TDynamicObject *old, TDynamicObject *now)
{ // ewentualna zmiana kabiny użytkownikowi
    if (Train)
        if (Train->Dynamic() == old)
            Global::changeDynObj = now; // uruchomienie protezy
};

void TWorld::ChangeDynamic() {

    // Ra: to nie może być tak robione, to zbytnia proteza jest
    Train->Silence(); // wyłączenie dźwięków opuszczanej kabiny
    if( Train->Dynamic()->Mechanik ) // AI może sobie samo pójść
        if( !Train->Dynamic()->Mechanik->AIControllFlag ) // tylko jeśli ręcznie prowadzony
        { // jeśli prowadzi AI, to mu nie robimy dywersji!
            Train->Dynamic()->MoverParameters->CabDeactivisation();
            Train->Dynamic()->Controller = AIdriver;
            // Train->Dynamic()->MoverParameters->SecuritySystem.Status=0; //rozwala CA w EZT
            Train->Dynamic()->MoverParameters->ActiveCab = 0;
            Train->Dynamic()->MoverParameters->BrakeLevelSet(
                Train->Dynamic()->MoverParameters->Handle->GetPos(
                bh_NP ) ); //rozwala sterowanie hamulcem GF 04-2016
            Train->Dynamic()->MechInside = false;
        }
    TDynamicObject *temp = Global::changeDynObj;
    Train->Dynamic()->bDisplayCab = false;
    Train->Dynamic()->ABuSetModelShake( vector3( 0, 0, 0 ) );

    if( Train->Dynamic()->Mechanik ) // AI może sobie samo pójść
        if( !Train->Dynamic()->Mechanik->AIControllFlag ) // tylko jeśli ręcznie prowadzony
            Train->Dynamic()->Mechanik->MoveTo( temp ); // przsunięcie obiektu zarządzającego

    Train->DynamicSet( temp );
    Controlled = temp;
    mvControlled = Controlled->ControlledFind()->MoverParameters;
    Global::asHumanCtrlVehicle = Train->Dynamic()->GetName();
    if( Train->Dynamic()->Mechanik ) // AI może sobie samo pójść
        if( !Train->Dynamic()->Mechanik->AIControllFlag ) // tylko jeśli ręcznie prowadzony
        {
            Train->Dynamic()->MoverParameters->LimPipePress =
                Controlled->MoverParameters->PipePress;
            Train->Dynamic()
                ->MoverParameters->CabActivisation(); // załączenie rozrządu (wirtualne kabiny)
            Train->Dynamic()->Controller = Humandriver;
            Train->Dynamic()->MechInside = true;
        }
    Train->InitializeCab( Train->Dynamic()->MoverParameters->CabNo,
        Train->Dynamic()->asBaseDir +
        Train->Dynamic()->MoverParameters->TypeName + ".mmd" );
    if( !FreeFlyModeFlag ) {
        Global::pUserDynamic = Controlled; // renerowanie względem kamery
        Train->Dynamic()->bDisplayCab = true;
        Train->Dynamic()->ABuSetModelShake(
            vector3( 0, 0, 0 ) ); // zerowanie przesunięcia przed powrotem?
        Train->MechStop();
        FollowView(); // na pozycję mecha
    }
    Global::changeDynObj = NULL;
}
//---------------------------------------------------------------------------

void
TWorld::ToggleDaylight() {

    Global::FakeLight = !Global::FakeLight;

    if( Global::FakeLight ) {
        // for fake daylight enter fixed hour
        Environment.time( 10, 30, 0 );
    }
    else {
        // local clock based calculation
        Environment.time();
    }
}

void
world_environment::init() {
//	m_skydome.init();
    m_sun.init();
    m_moon.init();
    m_stars.init();
    m_clouds.Init();
}

void
world_environment::update() {
    // move celestial bodies...
    m_sun.update();
    m_moon.update();
    // ...determine source of key light and adjust global state accordingly...
    // diffuse (sun) intensity goes down after twilight, and reaches minimum 18 degrees below horizon
    float twilightfactor = clamp( -m_sun.getAngle(), 0.0f, 18.0f ) / 18.0f;
    // NOTE: sun light receives extra padding to prevent moon from kicking in too soon
    auto const sunlightlevel = m_sun.getIntensity() + 0.05f * ( 1.f - twilightfactor );
    auto const moonlightlevel = m_moon.getIntensity() * 0.65f; // scaled down by arbitrary factor, it's pretty bright otherwise
    float keylightintensity;
    glm::vec3 keylightcolor;
    if( moonlightlevel > sunlightlevel ) {
        // rare situations when the moon is brighter than the sun, typically at night
        Global::SunAngle = m_moon.getAngle();
        Global::DayLight.set_position( m_moon.getPosition() );
        Global::DayLight.direction = -1.0f * m_moon.getDirection();
        keylightintensity = moonlightlevel;
        // if the moon is up, it overrides the twilight
        twilightfactor = 0.0f;
        keylightcolor = glm::vec3( 255.0f / 255.0f, 242.0f / 255.0f, 202.0f / 255.0f );
    }
    else {
        // regular situation with sun as the key light
        Global::SunAngle = m_sun.getAngle();
        Global::DayLight.set_position( m_sun.getPosition() );
        Global::DayLight.direction = -1.0f * m_sun.getDirection();
        keylightintensity = sunlightlevel;
        // include 'golden hour' effect in twilight lighting
        float const duskfactor = 1.0f - clamp( Global::SunAngle, 0.0f, 18.0f ) / 18.0f;
        keylightcolor = interpolate(
            glm::vec3( 255.0f / 255.0f, 242.0f / 255.0f, 231.0f / 255.0f ),
            glm::vec3( 235.0f / 255.0f, 140.0f / 255.0f, 36.0f / 255.0f ),
            duskfactor );
    }
    // ...update skydome to match the current sun position as well...
    m_skydome.SetOvercastFactor( Global::Overcast );
    m_skydome.Update( m_sun.getDirection() );
    // ...retrieve current sky colour and brightness...
    auto const skydomecolour = m_skydome.GetAverageColor();
    auto const skydomehsv = RGBtoHSV( skydomecolour );
    // sun strength is reduced by overcast level
    keylightintensity *= ( 1.0f - Global::Overcast * 0.65f );

    // intensity combines intensity of the sun and the light reflected by the sky dome
    // it'd be more technically correct to have just the intensity of the sun here,
    // but whether it'd _look_ better is something to be tested
    auto const intensity = std::min( 1.15f * ( 0.05f + keylightintensity + skydomehsv.z ), 1.25f );
    // the impact of sun component is reduced proportionally to overcast level, as overcast increases role of ambient light
    auto const diffuselevel = interpolate( keylightintensity, intensity * ( 1.0f - twilightfactor ), 1.0f - Global::Overcast * 0.75f );
    // ...update light colours and intensity.
    keylightcolor = keylightcolor * diffuselevel;
    Global::DayLight.diffuse = glm::vec4( keylightcolor, Global::DayLight.diffuse.a );
    Global::DayLight.specular = glm::vec4( keylightcolor * 0.85f, diffuselevel );

    // tonal impact of skydome color is inversely proportional to how high the sun is above the horizon
    // (this is pure conjecture, aimed more to 'look right' than be accurate)
    float const ambienttone = clamp( 1.0f - ( Global::SunAngle / 90.0f ), 0.0f, 1.0f );
    Global::DayLight.ambient[ 0 ] = interpolate( skydomehsv.z, skydomecolour.x, ambienttone );
    Global::DayLight.ambient[ 1 ] = interpolate( skydomehsv.z, skydomecolour.y, ambienttone );
    Global::DayLight.ambient[ 2 ] = interpolate( skydomehsv.z, skydomecolour.z, ambienttone );

    Global::fLuminance = intensity;

    // update the fog. setting it to match the average colour of the sky dome is cheap
    // but quite effective way to make the distant items blend with background better
    Global::FogColor[ 0 ] = skydomecolour.x;
    Global::FogColor[ 1 ] = skydomecolour.y;
    Global::FogColor[ 2 ] = skydomecolour.z;
}

void
world_environment::time( int const Hour, int const Minute, int const Second ) {

    m_sun.setTime( Hour, Minute, Second );
}
