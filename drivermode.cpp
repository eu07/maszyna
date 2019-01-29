/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "drivermode.h"
#include "driveruilayer.h"

#include "Globals.h"
#include "application.h"
#include "simulation.h"
#include "simulationtime.h"
#include "simulationenvironment.h"
#include "lightarray.h"
#include "Train.h"
#include "Driver.h"
#include "DynObj.h"
#include "Model3d.h"
#include "Event.h"
#include "messaging.h"
#include "Timer.h"
#include "renderer.h"
#include "utilities.h"
#include "Logs.h"
/*
namespace input {

user_command command; // currently issued control command, if any

}
*/
void
driver_mode::drivermode_input::poll() {
    if (telemetry)
        telemetry->update();
    keyboard.poll();
    if( true == Global.InputMouse ) {
        mouse.poll();
    }
    if( true == Global.InputGamepad ) {
        gamepad.poll();
    }
    if( uart != nullptr ) {
        uart->poll();
    }
/*
    // TBD, TODO: wrap current command in object, include other input sources?
    input::command = (
        mouse.command() != user_command::none ?
            mouse.command() :
            keyboard.command() );
*/
}

bool
driver_mode::drivermode_input::init() {

    // initialize input devices
    auto result = (
        keyboard.init()
     && mouse.init() );
    if( true == Global.InputGamepad ) {
        gamepad.init();
    }
    if( true == Global.uart_conf.enable ) {
        uart = std::make_unique<uart_input>();
        uart->init();
    }
    if (Global.motiontelemetry_conf.enable)
        telemetry = std::make_unique<motiontelemetry>();

#ifdef _WIN32
    Console::On(); // włączenie konsoli
#endif

    return result;
}

driver_mode::driver_mode() {

    m_userinterface = std::make_shared<driver_ui>();
}

// initializes internal data structures of the mode. returns: true on success, false otherwise
bool
driver_mode::init() {

    return m_input.init();
}

// mode-specific update of simulation data. returns: false on error, true otherwise
bool
driver_mode::update() {

    Timer::UpdateTimers(Global.iPause != 0);
    Timer::subsystem.sim_total.start();

    double const deltatime = Timer::GetDeltaTime(); // 0.0 gdy pauza

    if( Global.iPause == 0 ) {
        // jak pauza, to nie ma po co tego przeliczać
        simulation::Time.update( deltatime );
    }
    simulation::State.update_clocks();
    simulation::Environment.update();

    // fixed step, simulation time based updates
//  m_primaryupdateaccumulator += dt; // unused for the time being
    m_secondaryupdateaccumulator += deltatime;
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
    if( deltatime > m_primaryupdaterate ) // normalnie 0.01s
    {
/*
        // NOTE: experimentally disabled physics update cap
        auto const iterations = std::ceil(dt / m_primaryupdaterate);
        updatecount = std::min( 20, static_cast<int>( iterations ) );
*/
        updatecount = std::ceil( deltatime / m_primaryupdaterate );
/*
        // NOTE: changing dt wrecks things further down the code. re-acquire proper value later or cleanup here
        dt = dt / iterations; // Ra: fizykę lepiej by było przeliczać ze stałym krokiem
*/
    }
    auto const stepdeltatime { deltatime / updatecount };
    // NOTE: updates are limited to 20, but dt is distributed over potentially many more iterations
    // this means at count > 20 simulation and render are going to desync. is that right?
    // NOTE: experimentally changing this to prevent the desync.
    // TODO: test what happens if we hit more than 20 * 0.01 sec slices, i.e. less than 5 fps
    Timer::subsystem.sim_dynamics.start();
    if( true == Global.FullPhysics ) {
        // mixed calculation mode, steps calculated in ~0.05s chunks
        while( updatecount >= 5 ) {
            simulation::State.update( stepdeltatime, 5 );
            updatecount -= 5;
        }
        if( updatecount ) {
            simulation::State.update( stepdeltatime, updatecount );
        }
    }
    else {
        // simplified calculation mode; faster but can lead to errors
        simulation::State.update( stepdeltatime, updatecount );
    }
    Timer::subsystem.sim_dynamics.stop();

    // secondary fixed step simulation time routines
    while( m_secondaryupdateaccumulator >= m_secondaryupdaterate ) {

        // awaria PoKeys mogła włączyć pauzę - przekazać informację
        if( Global.iMultiplayer ) // dajemy znać do serwera o wykonaniu
            if( iPause != Global.iPause ) { // przesłanie informacji o pauzie do programu nadzorującego
                multiplayer::WyslijParam( 5, 3 ); // ramka 5 z czasem i stanem zapauzowania
                iPause = Global.iPause;
            }

        // TODO: generic shake update pass for vehicles within view range
        if( Camera.m_owner != nullptr ) {
            Camera.m_owner->update_shake( m_secondaryupdaterate );
        }

        m_secondaryupdateaccumulator -= m_secondaryupdaterate; // these should be inexpensive enough we have no cap
    }

    // variable step simulation time routines

    if( ( simulation::Train == nullptr ) && ( false == FreeFlyModeFlag ) ) {
        // intercept cases when the driven train got removed after entering portal
        InOutKey();
    }

    if( Global.changeDynObj ) {
        // ABu zmiana pojazdu - przejście do innego
        ChangeDynamic();
    }

    if( simulation::Train != nullptr ) {
        TSubModel::iInstance = reinterpret_cast<std::uintptr_t>( simulation::Train->Dynamic() );
        simulation::Train->Update( deltatime );
    }
    else {
        TSubModel::iInstance = 0;
    }

    simulation::Events.update();
    simulation::Region->update_events();
    simulation::Lights.update();

    // render time routines follow:

    auto const deltarealtime = Timer::GetDeltaRenderTime(); // nie uwzględnia pauzowania ani mnożenia czasu

    // fixed step render time routines

    fTime50Hz += deltarealtime; // w pauzie też trzeba zliczać czas, bo przy dużym FPS będzie problem z odczytem ramek
    while( fTime50Hz >= 1.0 / 50.0 ) {
#ifdef _WIN32
        Console::Update(); // to i tak trzeba wywoływać
#endif
        ui::Transcripts.Update(); // obiekt obsługujący stenogramy dźwięków na ekranie
        m_userinterface->update();
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

    update_camera( deltarealtime );

    simulation::Environment.update_precipitation(); // has to be launched after camera step to work properly

    Timer::subsystem.sim_total.stop();

    simulation::Region->update_sounds();
    audio::renderer.update( Global.iPause ? 0.0 : deltarealtime );

    GfxRenderer.Update( deltarealtime );

    simulation::is_ready = true;

    return true;
}

// maintenance method, called when the mode is activated
void
driver_mode::enter() {

    TDynamicObject *nPlayerTrain { (
        ( Global.asHumanCtrlVehicle != "ghostview" ) ?
            simulation::Vehicles.find( Global.asHumanCtrlVehicle ) :
            nullptr ) };

    Camera.Init(Global.FreeCameraInit[0], Global.FreeCameraInitAngle[0], nPlayerTrain );
    Global.pCamera = Camera;
    Global.pDebugCamera = DebugCamera;

    if (nPlayerTrain)
    {
        WriteLog( "Initializing player train, \"" + Global.asHumanCtrlVehicle + "\"" );

        if( simulation::Train == nullptr ) {
            simulation::Train = new TTrain();
        }
        if( simulation::Train->Init( nPlayerTrain ) )
        {
            WriteLog("Player train initialization OK");

            Application.set_title( Global.AppName + " (" + simulation::Train->Controlled()->Name + " @ " + Global.SceneryFile + ")" );

            CabView();
        }
        else
        {
            Error("Bad init: player train initialization failed");
            FreeFlyModeFlag = true; // Ra: automatycznie włączone latanie
            SafeDelete( simulation::Train );
            Camera.m_owner = nullptr;
        }
    }
    else
    {
        if (Global.asHumanCtrlVehicle != "ghostview")
        {
            Error("Bad scenario: failed to locate player train, \"" + Global.asHumanCtrlVehicle + "\"" );
        }
        FreeFlyModeFlag = true; // Ra: automatycznie włączone latanie
        Camera.m_owner = nullptr;
        DebugCamera = Camera;
    }

    // if (!Global.bMultiplayer) //na razie włączone
    { // eventy aktywowane z klawiatury tylko dla jednego użytkownika
        KeyEvents[ 0 ] = simulation::Events.FindEvent( "keyctrl00" );
        KeyEvents[ 1 ] = simulation::Events.FindEvent( "keyctrl01" );
        KeyEvents[ 2 ] = simulation::Events.FindEvent( "keyctrl02" );
        KeyEvents[ 3 ] = simulation::Events.FindEvent( "keyctrl03" );
        KeyEvents[ 4 ] = simulation::Events.FindEvent( "keyctrl04" );
        KeyEvents[ 5 ] = simulation::Events.FindEvent( "keyctrl05" );
        KeyEvents[ 6 ] = simulation::Events.FindEvent( "keyctrl06" );
        KeyEvents[ 7 ] = simulation::Events.FindEvent( "keyctrl07" );
        KeyEvents[ 8 ] = simulation::Events.FindEvent( "keyctrl08" );
        KeyEvents[ 9 ] = simulation::Events.FindEvent( "keyctrl09" );
    }

    //ui_log->enabled = false;

    Timer::ResetTimers();

    set_picking( Global.ControlPicking );
}

// maintenance method, called when the mode is deactivated
void
driver_mode::exit() {

}

void
driver_mode::on_key( int const Key, int const Scancode, int const Action, int const Mods ) {

    Global.shiftState = ( Mods & GLFW_MOD_SHIFT ) ? true : false;
    Global.ctrlState = ( Mods & GLFW_MOD_CONTROL ) ? true : false;
    Global.altState = ( Mods & GLFW_MOD_ALT ) ? true : false;

    bool anyModifier = Mods & (GLFW_MOD_SHIFT | GLFW_MOD_CONTROL | GLFW_MOD_ALT);

    // give the ui first shot at the input processing...
    if( !anyModifier && true == m_userinterface->on_key( Key, Action ) ) { return; }
    // ...if the input is left untouched, pass it on
    if( true == m_input.keyboard.key( Key, Action ) ) { return; }

    if( ( true == Global.InputMouse )
     && ( ( Key == GLFW_KEY_LEFT_ALT )
       || ( Key == GLFW_KEY_RIGHT_ALT ) ) ) {
        // if the alt key was pressed toggle control picking mode and set matching cursor behaviour
        if( Action == GLFW_PRESS ) {
            // toggle picking mode
            set_picking( !Global.ControlPicking );
        }
    }

    if( Action != GLFW_RELEASE ) {
        OnKeyDown( Key );
    }
}

void
driver_mode::on_cursor_pos( double const Horizontal, double const Vertical ) {

    // give the ui first shot at the input processing...
    if( true == m_userinterface->on_cursor_pos( Horizontal, Vertical ) ) { return; }

    if( false == Global.ControlPicking ) {
        // in regular view mode keep cursor on screen
        Application.set_cursor_pos( 0, 0 );
    }
    // give the potential event recipient a shot at it, in the virtual z order
    m_input.mouse.move( Horizontal, Vertical );
}

void
driver_mode::on_mouse_button( int const Button, int const Action, int const Mods ) {

    // give the ui first shot at the input processing...
    if( true == m_userinterface->on_mouse_button( Button, Action ) ) { return; }

    // give the potential event recipient a shot at it, in the virtual z order
    m_input.mouse.button( Button, Action );
}

void
driver_mode::on_scroll( double const Xoffset, double const Yoffset ) {

    m_input.mouse.scroll( Xoffset, Yoffset );
}

void
driver_mode::on_event_poll() {

    m_input.poll();
}

void
driver_mode::update_camera( double const Deltatime ) {

    auto *controlled = (
        simulation::Train ?
            simulation::Train->Dynamic() :
            nullptr );

    if( false == Global.ControlPicking ) {

        if( m_input.mouse.button( GLFW_MOUSE_BUTTON_LEFT ) == GLFW_PRESS ) {
            Camera.Reset(); // likwidacja obrotów - patrzy horyzontalnie na południe
            if( Camera.m_owner == nullptr ) {
                if( controlled && LengthSquared3( controlled->GetPosition() - Camera.Pos ) < ( 1500 * 1500 ) ) {
                    // gdy bliżej niż 1.5km
                    Camera.LookAt = controlled->GetPosition();
                }
                else {
                    TDynamicObject *d = std::get<TDynamicObject *>( simulation::Region->find_vehicle( Global.pCamera.Pos, 300, false, false ) );
                    if( !d )
                        d = std::get<TDynamicObject *>( simulation::Region->find_vehicle( Global.pCamera.Pos, 1000, false, false ) ); // dalej szukanie, jesli bliżej nie ma

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
                Camera.RaLook(); // jednorazowe przestawienie kamery
            }
            else {
                if( false == FreeFlyModeFlag ) {
                // reset cached view angle in the cab
                    simulation::Train->pMechViewAngle = { Camera.Angle.x, Camera.Angle.y };
                }
            }
        }
        else if( m_input.mouse.button( GLFW_MOUSE_BUTTON_RIGHT ) == GLFW_PRESS ) {
            CabView();
        }
    }

    if( m_input.mouse.button( GLFW_MOUSE_BUTTON_MIDDLE ) == GLFW_PRESS ) {
        // middle mouse button controls zoom.
        Global.ZoomFactor = std::min( 4.5f, Global.ZoomFactor + 15.0f * static_cast<float>( Deltatime ) );
    }
    else if( m_input.mouse.button( GLFW_MOUSE_BUTTON_MIDDLE ) != GLFW_PRESS ) {
        // reset zoom level if the button is no longer held down.
        // NOTE: yes, this is terrible way to go about it. it'll do for now.
        Global.ZoomFactor = std::max( 1.0f, Global.ZoomFactor - 15.0f * static_cast<float>( Deltatime ) );
    }

    // uwzględnienie ruchu wywołanego klawiszami
    if( false == DebugCameraFlag ) {
        // regular camera
        if( ( false == FreeFlyModeFlag )
         && ( false == Global.CabWindowOpen ) ) {
            // if in cab potentially alter camera placement based on changes in train object
            Camera.m_owneroffset = simulation::Train->pMechOffset;
            Camera.Angle.x = simulation::Train->pMechViewAngle.x;
            Camera.Angle.y = simulation::Train->pMechViewAngle.y;
        }

        Camera.Update();

        if( false == FreeFlyModeFlag ) {
            // keep the camera within cab boundaries
            Camera.m_owneroffset = simulation::Train->clamp_inside( Camera.m_owneroffset );
        }

        if( ( false == FreeFlyModeFlag )
         && ( false == Global.CabWindowOpen ) ) {
            // cache cab camera in case of view type switch
            simulation::Train->pMechViewAngle = { Camera.Angle.x, Camera.Angle.y };
            simulation::Train->pMechOffset = Camera.m_owneroffset;
        }

        if( ( true == FreeFlyModeFlag )
         && ( Camera.m_owner != nullptr ) ) {
            // cache external view config
            auto &externalviewconfig { m_externalviewconfigs[ m_externalviewmode ] };
            externalviewconfig.owner = Camera.m_owner;
            externalviewconfig.offset = Camera.m_owneroffset;
            externalviewconfig.angle = Camera.Angle;
        }
    }
    else {
        // debug camera
        DebugCamera.Update();
    }
                                               
    // reset window state, it'll be set again if applicable in a check below
    Global.CabWindowOpen = false;

    if( ( simulation::Train != nullptr )
     && ( Camera.m_owner != nullptr )
     && ( false == DebugCameraFlag ) ) {
        // jeśli jazda w kabinie, przeliczyć trzeba parametry kamery
/*
        auto tempangle = controlled->VectorFront() * ( controlled->MoverParameters->ActiveCab == -1 ? -1 : 1 );
        double modelrotate = atan2( -tempangle.x, tempangle.z );
*/
        if( ( false == FreeFlyModeFlag )
         && ( true == Global.ctrlState )
         && ( ( m_input.keyboard.key( GLFW_KEY_LEFT ) != GLFW_RELEASE )
           || ( m_input.keyboard.key( GLFW_KEY_RIGHT ) != GLFW_RELEASE ) ) ) {
            // jeśli lusterko lewe albo prawe (bez rzucania na razie)
            Global.CabWindowOpen = true;

            auto const lr { m_input.keyboard.key( GLFW_KEY_LEFT ) != GLFW_RELEASE };
            // Camera.Yaw powinno być wyzerowane, aby po powrocie patrzeć do przodu
            Camera.Pos = controlled->GetPosition() + simulation::Train->MirrorPosition( lr ); // pozycja lusterka
            Camera.Angle.y = 0; // odchylenie na bok od Camera.LookAt
            if( simulation::Train->Occupied()->ActiveCab == 0 ) {
                // gdy w korytarzu
                Camera.LookAt = Camera.Pos - simulation::Train->GetDirection();
            }
            else if( Global.shiftState ) {
                // patrzenie w bok przez szybę
                Camera.LookAt = Camera.Pos - ( lr ? -1 : 1 ) * controlled->VectorLeft() * simulation::Train->Occupied()->ActiveCab;
            }
            else { // patrzenie w kierunku osi pojazdu, z uwzględnieniem kabiny - jakby z lusterka,
                // ale bez odbicia
                Camera.LookAt = Camera.Pos - simulation::Train->GetDirection() * simulation::Train->Occupied()->ActiveCab; //-1 albo 1
            }
            auto const shakeangles { simulation::Train->Dynamic()->shake_angles() };
            Camera.Angle.x = 0.5 * shakeangles.second; // hustanie kamery przod tyl
            Camera.Angle.z = shakeangles.first; // hustanie kamery na boki
/*
            Camera.Roll = std::atan( simulation::Train->pMechShake.x * simulation::Train->BaseShake.angle_scale.x ); // hustanie kamery na boki
            Camera.Pitch = 0.5 * std::atan( simulation::Train->vMechVelocity.z * simulation::Train->BaseShake.angle_scale.z ); // hustanie kamery przod tyl
*/
            Camera.vUp = controlled->VectorUp();
        }
        else {
            // patrzenie standardowe
            if( false == FreeFlyModeFlag ) {
                // potentially restore view angle after returning from external view
                // TODO: mirror view toggle as separate method
                Camera.Angle.x = simulation::Train->pMechViewAngle.x;
                Camera.Angle.y = simulation::Train->pMechViewAngle.y;
            }

            auto const shakescale { FreeFlyModeFlag ? 5.0 : 1.0 };
            auto shakencamerapos {
                    Camera.m_owneroffset
                    + shakescale * Math3D::vector3(
                        1.5 * Camera.m_owner->ShakeState.offset.x,
                        2.0 * Camera.m_owner->ShakeState.offset.y,
                        1.5 * Camera.m_owner->ShakeState.offset.z ) };

            Camera.Pos = (
                Camera.m_owner->GetWorldPosition (
                    FreeFlyModeFlag ?
                        shakencamerapos : // TODO: vehicle collision box for the external vehicle camera
                        simulation::Train->clamp_inside( shakencamerapos ) ) );

            if( !Global.iPause ) {
                // podczas pauzy nie przeliczać kątów przypadkowymi wartościami
                auto const shakeangles { Camera.m_owner->shake_angles() };
                Camera.Angle.x -= 0.5 * shakeangles.second; // hustanie kamery przod tyl
                Camera.Angle.z = shakeangles.first; // hustanie kamery na boki
/*
                Camera.Roll = std::atan( simulation::Train->vMechVelocity.x * simulation::Train->BaseShake.angle_scale.x ); // hustanie kamery na boki
                Camera.Pitch -= 0.5 * atan( simulation::Train->vMechVelocity.z * simulation::Train->BaseShake.angle_scale.z ); // hustanie kamery przod tyl
*/
            }
/*
            // ABu011104: rzucanie pudlem
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
            if( simulation::Train->Occupied()->ActiveCab == 0 ) {
                // gdy w korytarzu
                Camera.LookAt =
                    Camera.m_owner->GetWorldPosition( Camera.m_owneroffset )
                    + Camera.m_owner->VectorFront() * 5.0;
            }
            else {
                // patrzenie w kierunku osi pojazdu, z uwzględnieniem kabiny
                Camera.LookAt =
                    Camera.m_owner->GetWorldPosition( Camera.m_owneroffset )
                    + Camera.m_owner->VectorFront() * 5.0
                    * simulation::Train->Occupied()->ActiveCab; //-1 albo 1
            }
            Camera.vUp = simulation::Train->GetUp();
        }
    }
    // all done, update camera position to the new value
    Global.pCamera = Camera;
}

void
driver_mode::OnKeyDown(int cKey) {
    // dump keypress info in the log
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
            case GLFW_KEY_PAUSE: { keyinfo += "Pause"; break; }
        }
    }
    if( keyinfo.empty() == false ) {

        std::string keymodifiers;
        if( Global.shiftState )
            keymodifiers += "[Shift]+";
        if( Global.ctrlState )
            keymodifiers += "[Ctrl]+";

        WriteLog( "Key pressed: " + keymodifiers + "[" + keyinfo + "]" );
    }

    // actual key processing
    // TODO: redo the input system
    if( ( cKey >= GLFW_KEY_0 ) && ( cKey <= GLFW_KEY_9 ) ) {
        // klawisze cyfrowe
        int i = cKey - GLFW_KEY_0; // numer klawisza
        if (Global.shiftState) {
            // z [Shift] uruchomienie eventu
            if( ( false == Global.iPause ) // podczas pauzy klawisze nie działają
             && ( KeyEvents[ i ] != nullptr ) ) {
                simulation::Events.AddToQuery( KeyEvents[ i ], NULL );
            }
        }
        else if( Global.ctrlState ) {
            // zapamiętywanie kamery może działać podczas pauzy
            if( FreeFlyModeFlag ) {
                // w trybie latania można przeskakiwać do ustawionych kamer
                if( ( Global.FreeCameraInit[ i ].x == 0.0 )
                 && ( Global.FreeCameraInit[ i ].y == 0.0 )
                 && ( Global.FreeCameraInit[ i ].z == 0.0 ) ) {
                    // jeśli kamera jest w punkcie zerowym, zapamiętanie współrzędnych i kątów
                    Global.FreeCameraInit[ i ] = Camera.Pos;
                    Global.FreeCameraInitAngle[ i ] = Camera.Angle;
                    // logowanie, żeby można było do scenerii przepisać
                    WriteLog(
                        "camera " + std::to_string( Global.FreeCameraInit[ i ].x ) + " "
                        + std::to_string( Global.FreeCameraInit[ i ].y ) + " "
                        + std::to_string( Global.FreeCameraInit[ i ].z ) + " "
                        + std::to_string( RadToDeg( Global.FreeCameraInitAngle[ i ].x ) ) + " "
                        + std::to_string( RadToDeg( Global.FreeCameraInitAngle[ i ].y ) ) + " "
                        + std::to_string( RadToDeg( Global.FreeCameraInitAngle[ i ].z ) ) + " "
                        + std::to_string( i ) + " endcamera" );
                }
                else // również przeskakiwanie
                { // Ra: to z tą kamerą (Camera.Pos i Global.pCameraPosition) jest trochę bez sensu
                    Global.pCamera.Pos = Global.FreeCameraInit[ i ]; // nowa pozycja dla generowania obiektów
                    Camera.Init( Global.FreeCameraInit[ i ], Global.FreeCameraInitAngle[ i ], nullptr ); // przestawienie
                }
            }
        }
        return;
    }

    switch (cKey) {

        case GLFW_KEY_F1: {

            if( DebugModeFlag ) {
                // additional simulation clock jump keys in debug mode
                if( Global.ctrlState ) {
                    // ctrl-f1
                    simulation::Time.update( 20.0 * 60.0 );
                }
                else if( Global.shiftState ) {
                    // shift-f1
                    simulation::Time.update( 5.0 * 60.0 );
                }
            }
            break;
        }
            
        case GLFW_KEY_F4: {
            
            if( Global.shiftState ) { ExternalView(); } // with Shift, cycle through external views 
            else                    { InOutKey(); } // without, step out of the cab or return to it
            break;
        }
        case GLFW_KEY_F5: {
            // przesiadka do innego pojazdu
            if( false == FreeFlyModeFlag ) {
                // only available in free fly mode
                break;
            }

            TDynamicObject *tmp = std::get<TDynamicObject *>( simulation::Region->find_vehicle( Global.pCamera.Pos, 50, true, false ) );

            if( tmp != nullptr ) {

                if( simulation::Train ) {// jeśli mielismy pojazd
                    if( simulation::Train->Dynamic()->Mechanik ) { // na skutek jakiegoś błędu może czasem zniknąć
                        simulation::Train->Dynamic()->Mechanik->TakeControl( true ); // oddajemy dotychczasowy AI
                    }
                }

                if( ( true == DebugModeFlag )
                 || ( tmp->MoverParameters->Vel <= 5.0 ) ) {
                    // works always in debug mode, or for stopped/slow moving vehicles otherwise
                    if( simulation::Train == nullptr ) {
                        simulation::Train = new TTrain(); // jeśli niczym jeszcze nie jeździlismy
                    }
                    if( simulation::Train->Init( tmp ) ) { // przejmujemy sterowanie
                        simulation::Train->Dynamic()->Mechanik->TakeControl( false );
                    }
                    else {
                        SafeDelete( simulation::Train ); // i nie ma czym sterować
                    }
                    if( simulation::Train ) {
                        InOutKey(); // do kabiny
                    }
                }
            }
            break;
        }
        case GLFW_KEY_F6: {
            // przyspieszenie symulacji do testowania scenerii... uwaga na FPS!
            if( DebugModeFlag ) { 

                if( Global.ctrlState ) { Global.fTimeSpeed = ( Global.shiftState ? 60.0 : 20.0 ); }
                else                   { Global.fTimeSpeed = ( Global.shiftState ? 5.0 : Global.fTimeSpeed ); }
            }
            break;
        }
        case GLFW_KEY_F7: {
            // debug mode functions
            if( DebugModeFlag ) {

                if( Global.ctrlState
                 && Global.shiftState ) {
                    // shift + ctrl + f7 toggles between debug and regular camera
                    DebugCameraFlag = !DebugCameraFlag;
                }
                else if( Global.ctrlState ) {
                    // ctrl + f7 toggles static daylight
                    simulation::Environment.toggle_daylight();
                    break;
                }
                else if( Global.shiftState ) {
                    // shift + f7 is currently unused
                }
                else {
                    // f7: wireframe toggle
                    Global.bWireFrame = !Global.bWireFrame;
                }
            }
            break;
        }
        case GLFW_KEY_F11: {
            // editor mode
            if( ( false == Global.ctrlState )
             && ( false == Global.shiftState ) ) {
                Application.push_mode( eu07_application::mode::editor );
            }
            break;
        }
        case GLFW_KEY_F12: {
            // quick debug mode toggle
            if( Global.ctrlState
             && Global.shiftState ) {
                DebugModeFlag = !DebugModeFlag;
            }
            break;
        }

        case GLFW_KEY_LEFT_BRACKET:
        case GLFW_KEY_RIGHT_BRACKET:
        case GLFW_KEY_TAB: {
            // consist movement in debug mode
            if( ( true == DebugModeFlag )
             && ( false == Global.shiftState )
             && ( true == Global.ctrlState )
             && ( simulation::Train != nullptr )
             && ( simulation::Train->Dynamic()->Controller == Humandriver ) ) {

                if( DebugModeFlag ) {
                    // przesuwanie składu o 100m
                    auto *vehicle { simulation::Train->Dynamic() };
                    TDynamicObject *d = vehicle;
                    if( cKey == GLFW_KEY_LEFT_BRACKET ) {
                        while( d ) {
                            d->Move( 100.0 * d->DirectionGet() );
                            d = d->Next(); // pozostałe też
                        }
                        d = vehicle->Prev();
                        while( d ) {
                            d->Move( 100.0 * d->DirectionGet() );
                            d = d->Prev(); // w drugą stronę też
                        }
                    }
                    else if( cKey == GLFW_KEY_RIGHT_BRACKET ) {
                        while( d ) {
                            d->Move( -100.0 * d->DirectionGet() );
                            d = d->Next(); // pozostałe też
                        }
                        d = vehicle->Prev();
                        while( d ) {
                            d->Move( -100.0 * d->DirectionGet() );
                            d = d->Prev(); // w drugą stronę też
                        }
                    }
                    else if( cKey == GLFW_KEY_TAB ) {
                        while( d ) {
                            d->MoverParameters->V += d->DirectionGet()*2.78;
                            d = d->Next(); // pozostałe też
                        }
                        d = vehicle->Prev();
                        while( d ) {
                            d->MoverParameters->V += d->DirectionGet()*2.78;
                            d = d->Prev(); // w drugą stronę też
                        }
                    }
                }
            }
            break;
        }

        default: {
            break;
        }
    }

    return; // nie są przekazywane do pojazdu wcale
}

// places camera outside the controlled vehicle, or nearest if nothing is under control
// depending on provided switch the view is placed right outside, or at medium distance
void
driver_mode::DistantView( bool const Near ) {

    TDynamicObject const *vehicle = { (
        ( simulation::Train != nullptr ) ?
            simulation::Train->Dynamic() :
            pDynamicNearest ) };
    
    if( vehicle == nullptr ) { return; }

    auto const cab =
        ( vehicle->MoverParameters->ActiveCab == 0 ?
            1 :
            vehicle->MoverParameters->ActiveCab );
    auto const left = vehicle->VectorLeft() * cab;

    if( true == Near ) {

        Camera.Pos =
            Math3D::vector3( Camera.Pos.x, vehicle->GetPosition().y, Camera.Pos.z )
            + left * vehicle->GetWidth()
            + Math3D::vector3( 1.25 * left.x, 1.6, 1.25 * left.z );
    }
    else {

        Camera.Pos =
            vehicle->GetPosition()
            + vehicle->VectorFront() * vehicle->MoverParameters->ActiveCab * 50.0
            + Math3D::vector3( -10.0 * left.x, 1.6, -10.0 * left.z );
    }

    Camera.m_owner = nullptr;
    Camera.LookAt = vehicle->GetPosition();
    Camera.RaLook(); // jednorazowe przestawienie kamery
}

void
driver_mode::ExternalView() {

    auto *train { simulation::Train };
    if( train == nullptr ) { return; }

    auto *vehicle { train->Dynamic() };

    // disable detailed cab in external view modes
    vehicle->bDisplayCab = false;

    if( true == m_externalview ) {
        // we're already in some external view mode, so select next one on the list
        m_externalviewmode = clamp_circular( ++m_externalviewmode, static_cast<int>( view::count_ ) );
    }

    FreeFlyModeFlag = true;
    m_externalview = true;

    Camera.Reset();
    // configure camera placement for the selected view mode
    switch( m_externalviewmode ) {
        case view::consistfront: {
            // bind camera with the vehicle
            auto *owner { vehicle->Mechanik->Vehicle( side::front ) };

            Camera.m_owner = owner;

            auto const &viewconfig { m_externalviewconfigs[ m_externalviewmode ] };
            if( owner == viewconfig.owner ) {
                // restore view config for previous owner
                Camera.m_owneroffset = viewconfig.offset;
                Camera.Angle = viewconfig.angle;
            }
            else {
                // default view setup
                auto const offsetflip{
                    ( vehicle->MoverParameters->ActiveCab == 0 ? 1 : vehicle->MoverParameters->ActiveCab )
                  * ( vehicle->MoverParameters->ActiveDir == 0 ? 1 : vehicle->MoverParameters->ActiveDir ) };

                Camera.m_owneroffset = {
                      1.5 * owner->MoverParameters->Dim.W * offsetflip,
                      std::max( 5.0, 1.25 * owner->MoverParameters->Dim.H ),
                    -0.4 * owner->MoverParameters->Dim.L * offsetflip };

                Camera.Angle.y = glm::radians( ( vehicle->MoverParameters->ActiveDir < 0 ? 180.0 : 0.0 ) );
            }
            auto const shakeangles{ owner->shake_angles() };
            Camera.Angle.x -= 0.5 * shakeangles.second; // hustanie kamery przod tyl
            Camera.Angle.z = shakeangles.first; // hustanie kamery na boki

            break;
        }
        case view::consistrear: {
            // bind camera with the vehicle
            auto *owner { vehicle->Mechanik->Vehicle( side::rear ) };

            Camera.m_owner = owner;

            auto const &viewconfig{ m_externalviewconfigs[ m_externalviewmode ] };
            if( owner == viewconfig.owner ) {
                // restore view config for previous owner
                Camera.m_owneroffset = viewconfig.offset;
                Camera.Angle = viewconfig.angle;
            }
            else {
                // default view setup
                auto const offsetflip{
                    ( vehicle->MoverParameters->ActiveCab == 0 ? 1 : vehicle->MoverParameters->ActiveCab )
                  * ( vehicle->MoverParameters->ActiveDir == 0 ? 1 : vehicle->MoverParameters->ActiveDir )
                  * -1 };

                Camera.m_owneroffset = {
                    1.5 * owner->MoverParameters->Dim.W * offsetflip,
                    std::max( 5.0, 1.25 * owner->MoverParameters->Dim.H ),
                    0.2 * owner->MoverParameters->Dim.L * offsetflip };

                Camera.Angle.y = glm::radians( ( vehicle->MoverParameters->ActiveDir < 0 ? 0.0 : 180.0 ) );
            }
            auto const shakeangles { owner->shake_angles() };
            Camera.Angle.x -= 0.5 * shakeangles.second; // hustanie kamery przod tyl
            Camera.Angle.z = shakeangles.first; // hustanie kamery na boki
            break;
        }
        case view::bogie: {
            auto *owner { vehicle->Mechanik->Vehicle( side::front ) };

            Camera.m_owner = owner;

            auto const &viewconfig{ m_externalviewconfigs[ m_externalviewmode ] };
            if( owner == viewconfig.owner ) {
                // restore view config for previous owner
                Camera.m_owneroffset = viewconfig.offset;
                Camera.Angle = viewconfig.angle;
            }
            else {
                // default view setup
                auto const offsetflip{
                    ( vehicle->MoverParameters->ActiveCab == 0 ? 1 : vehicle->MoverParameters->ActiveCab )
                  * ( vehicle->MoverParameters->ActiveDir == 0 ? 1 : vehicle->MoverParameters->ActiveDir ) };

                Camera.m_owneroffset = {
                    -0.65 * owner->MoverParameters->Dim.W * offsetflip,
                      0.90,
                      0.15 * owner->MoverParameters->Dim.L * offsetflip };

                Camera.Angle.y = glm::radians( ( vehicle->MoverParameters->ActiveDir < 0 ? 180.0 : 0.0 ) );
            }
            auto const shakeangles { owner->shake_angles() };
            Camera.Angle.x -= 0.5 * shakeangles.second; // hustanie kamery przod tyl
            Camera.Angle.z = shakeangles.first; // hustanie kamery na boki

            break;
        }
        case view::driveby: {
            DistantView( false );
            break;
        }
        default: {
            break;
        }
    }
}

// ustawienie śledzenia pojazdu
void
driver_mode::CabView() {

    // TODO: configure owner and camera placement depending on the view mode
    if( true == FreeFlyModeFlag ) { return; }

    auto *train { simulation::Train };
    if( train == nullptr ) { return; }

    m_externalview = false;

    // likwidacja obrotów - patrzy horyzontalnie na południe
    Camera.Reset(); 

    // bind camera with the vehicle
    Camera.m_owner = train->Dynamic();
    // potentially restore cached camera setup
    Camera.m_owneroffset = train->pMechSittingPosition;
    Camera.Angle.x = train->pMechViewAngle.x;
    Camera.Angle.y = train->pMechViewAngle.y;

    auto const shakeangles { Camera.m_owner->shake_angles() };
    Camera.Angle.x -= 0.5 * shakeangles.second; // hustanie kamery przod tyl
    Camera.Angle.z = shakeangles.first; // hustanie kamery na boki

    if( train->Occupied()->ActiveCab == 0 ) {
        Camera.LookAt =
            Camera.m_owner->GetWorldPosition( Camera.m_owneroffset )
            + Camera.m_owner->VectorFront() * 5.0;
    }
    else {
        // patrz w strone wlasciwej kabiny
        Camera.LookAt =
            Camera.m_owner->GetWorldPosition( Camera.m_owneroffset )
            + Camera.m_owner->VectorFront() * 5.0
            * Camera.m_owner->MoverParameters->ActiveCab;
    }
    train->pMechOffset = Camera.m_owneroffset;
}

void
driver_mode::ChangeDynamic() {

    auto *train { simulation::Train };
    if( train == nullptr ) { return; }

    auto *vehicle { train->Dynamic() };
    auto *occupied { train->Occupied() };
    auto *driver { vehicle->Mechanik };
    // Ra: to nie może być tak robione, to zbytnia proteza jest
    if( driver ) {
        // AI może sobie samo pójść
        if( false == driver->AIControllFlag ) {
            // tylko jeśli ręcznie prowadzony
            // jeśli prowadzi AI, to mu nie robimy dywersji!
            occupied->CabDeactivisation();
            occupied->ActiveCab = 0;
            occupied->BrakeLevelSet( occupied->Handle->GetPos( bh_NP ) ); //rozwala sterowanie hamulcem GF 04-2016
            vehicle->MechInside = false;
            vehicle->Controller = AIdriver;
        }
    }
    TDynamicObject *temp = Global.changeDynObj;
    vehicle->bDisplayCab = false;
    vehicle->ABuSetModelShake( {} );

    if( driver ) // AI może sobie samo pójść
        if( false == driver->AIControllFlag ) {
            // tylko jeśli ręcznie prowadzony
            // przsunięcie obiektu zarządzającego
            driver->MoveTo( temp );
        }

    train->DynamicSet( temp );
    // update helpers
    train = simulation::Train;
    vehicle = train->Dynamic();
    occupied = train->Occupied();
    driver = vehicle->Mechanik;
    Global.asHumanCtrlVehicle = vehicle->name();
    if( driver ) // AI może sobie samo pójść
        if( false == driver->AIControllFlag ) // tylko jeśli ręcznie prowadzony
        {
            occupied->LimPipePress = occupied->PipePress;
            occupied->CabActivisation(); // załączenie rozrządu (wirtualne kabiny)
            vehicle->MechInside = true;
            vehicle->Controller = Humandriver;
        }
    train->InitializeCab(
        occupied->CabNo,
        vehicle->asBaseDir + occupied->TypeName + ".mmd" );
    if( false == FreeFlyModeFlag ) {
        vehicle->bDisplayCab = true;
        vehicle->ABuSetModelShake( {} ); // zerowanie przesunięcia przed powrotem?
        CabView(); // na pozycję mecha
    }
    Global.changeDynObj = nullptr;
}

void
driver_mode::InOutKey()
{ // przełączenie widoku z kabiny na zewnętrzny i odwrotnie
    FreeFlyModeFlag = !FreeFlyModeFlag; // zmiana widoku

    auto *train { simulation::Train };

    if( train == nullptr ) {
        FreeFlyModeFlag = true; // nadal poza kabiną
        Camera.m_owner = nullptr; // detach camera from the vehicle
        return;
    }

    auto *vehicle { train->Dynamic() };

    if (FreeFlyModeFlag) {
        // jeżeli poza kabiną, przestawiamy w jej okolicę - OK
        // cache current cab position so there's no need to set it all over again after each out-in switch
        train->pMechSittingPosition = train->pMechOffset;

        vehicle->bDisplayCab = false;
        DistantView( true );

        DebugCamera = Camera;
    }
    else {
        // jazda w kabinie
        // zerowanie przesunięcia przed powrotem?
        vehicle->ABuSetModelShake( { 0, 0, 0 } );
        vehicle->bDisplayCab = true;
        CabView(); // na pozycję mecha
    }
    // update window title to reflect the situation
    Application.set_title( Global.AppName + " (" + ( train != nullptr ? train->Occupied()->Name : "" ) + " @ " + Global.SceneryFile + ")" );
}

void
driver_mode::set_picking( bool const Picking ) {

    if( Picking ) {
        // enter picking mode
        Application.set_cursor_pos( m_input.mouse_pickmodepos.x, m_input.mouse_pickmodepos.y );
        Application.set_cursor( GLFW_CURSOR_NORMAL );
    }
    else {
        // switch off
        Application.get_cursor_pos( m_input.mouse_pickmodepos.x, m_input.mouse_pickmodepos.y );
        Application.set_cursor( GLFW_CURSOR_DISABLED );
        Application.set_cursor_pos( 0, 0 );
    }
    // actually toggle the mode
    Global.ControlPicking = Picking;
}
