/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "editormode.h"
#include "editoruilayer.h"

#include "application.h"
#include "Globals.h"
#include "simulation.h"
#include "simulationtime.h"
#include "simulationenvironment.h"
#include "Timer.h"
#include "Console.h"
#include "renderer.h"
#include "audiorenderer.h"

bool
editor_mode::editormode_input::init() {

    return (
        mouse.init()
     && keyboard.init() );
}

void
editor_mode::editormode_input::poll() {

    keyboard.poll();
}

editor_mode::editor_mode() {

    m_userinterface = std::make_shared<editor_ui>();
}

// initializes internal data structures of the mode. returns: true on success, false otherwise
bool
editor_mode::init() {

    Camera.Init( { 0, 15, 0 }, { glm::radians( -30.0 ), glm::radians( 180.0 ), 0 }, TCameraType::tp_Free );

    return m_input.init();
}

// mode-specific update of simulation data. returns: false on error, true otherwise
bool
editor_mode::update() {

    Timer::UpdateTimers( true );

    simulation::State.update_clocks();
    simulation::Environment.update();

    // render time routines follow:
    auto const deltarealtime = Timer::GetDeltaRenderTime(); // nie uwzględnia pauzowania ani mnożenia czasu

    // fixed step render time routines:
    fTime50Hz += deltarealtime; // w pauzie też trzeba zliczać czas, bo przy dużym FPS będzie problem z odczytem ramek
    while( fTime50Hz >= 1.0 / 50.0 ) {
#ifdef _WIN32
        Console::Update(); // to i tak trzeba wywoływać
#endif
        m_userinterface->update();
        // decelerate camera
        Camera.Velocity *= 0.65;
        if( std::abs( Camera.Velocity.x ) < 0.01 ) { Camera.Velocity.x = 0.0; }
        if( std::abs( Camera.Velocity.y ) < 0.01 ) { Camera.Velocity.y = 0.0; }
        if( std::abs( Camera.Velocity.z ) < 0.01 ) { Camera.Velocity.z = 0.0; }

        fTime50Hz -= 1.0 / 50.0;
    }

    // variable step render time routines:
    update_camera( deltarealtime );

    simulation::Region->update_sounds();
    audio::renderer.update( Global.iPause ? 0.0 : deltarealtime );

    GfxRenderer.Update( deltarealtime );

    simulation::is_ready = true;

    return true;
}

void
editor_mode::update_camera( double const Deltatime ) {

    // uwzględnienie ruchu wywołanego klawiszami
    Camera.Update();
    // reset window state, it'll be set again if applicable in a check below
    Global.CabWindowOpen = false;
    // all done, update camera position to the new value
    Global.pCamera = Camera;
}

// maintenance method, called when the mode is activated
void
editor_mode::enter() {

    m_statebackup = { Global.pCamera, FreeFlyModeFlag, Global.ControlPicking };

    Global.pCamera = Camera;
    FreeFlyModeFlag = true;
    Global.ControlPicking = true;
    EditorModeFlag = true;

    Application.set_cursor( GLFW_CURSOR_NORMAL );
}

// maintenance method, called when the mode is deactivated
void
editor_mode::exit() {

    EditorModeFlag = false;
    Global.ControlPicking = m_statebackup.picking;
    FreeFlyModeFlag = m_statebackup.freefly;
    Global.pCamera = m_statebackup.camera;

    Application.set_cursor(
        ( Global.ControlPicking ?
            GLFW_CURSOR_NORMAL :
            GLFW_CURSOR_DISABLED ) );

    if( false == Global.ControlPicking ) {
        Application.set_cursor_pos( 0, 0 );
    }
}

void
editor_mode::on_key( int const Key, int const Scancode, int const Action, int const Mods ) {

    Global.shiftState = ( Mods & GLFW_MOD_SHIFT ) ? true : false;
    Global.ctrlState = ( Mods & GLFW_MOD_CONTROL ) ? true : false;
    Global.altState = ( Mods & GLFW_MOD_ALT ) ? true : false;

    // give the ui first shot at the input processing...
    if( true == m_userinterface->on_key( Key, Action ) ) { return; }
    // ...if the input is left untouched, pass it on
    if( true == m_input.keyboard.key( Key, Action ) ) { return; }

    if( Action == GLFW_RELEASE ) { return; }

    // legacy hardcoded keyboard commands
    // TODO: replace with current command system, move to input object(s)
    switch( Key ) {

        case GLFW_KEY_F11: {

            if( Action != GLFW_PRESS ) { break; }
            // mode switch
            // TODO: unsaved changes warning
            if( ( false == Global.ctrlState )
             && ( false == Global.shiftState ) ) {
                Application.pop_mode();
            }
            // scenery export
            if( Global.ctrlState
             && Global.shiftState ) {
                simulation::State.export_as_text( Global.SceneryFile );
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

        default: {
            break;
        }
    }
}

void
editor_mode::on_cursor_pos( double const Horizontal, double const Vertical ) {

    auto const mousemove { glm::dvec2{ Horizontal, Vertical } - m_input.mouse.position() };
    m_input.mouse.position( Horizontal, Vertical );

    if( m_input.mouse.button( GLFW_MOUSE_BUTTON_LEFT ) == GLFW_RELEASE ) { return; }
    if( m_node == nullptr ) { return; }

    if( m_takesnapshot ) {
        // take a snapshot of selected node(s)
        // TODO: implement this
        m_takesnapshot = false;
    }

    if( mode_translation() ) {
        // move selected node
        if( mode_translation_vertical() ) {
            auto const translation { mousemove.y * -0.01f };
            m_editor.translate( m_node, translation );
        }
        else {
            auto const mouseworldposition { Camera.Pos + GfxRenderer.Mouse_Position() };
            m_editor.translate( m_node, mouseworldposition, mode_snap() );
        }
    }
    else {
        // rotate selected node
        auto const rotation { glm::vec3 { mousemove.y, mousemove.x, 0 } * 0.25f };
        auto const quantization { (
            mode_snap() ?
                15.f : // TODO: put quantization value in a variable
                0.f ) };
        m_editor.rotate( m_node, rotation, quantization );
    }

}

void
editor_mode::on_mouse_button( int const Button, int const Action, int const Mods ) {

    if( Button == GLFW_MOUSE_BUTTON_LEFT ) {

        if( Action == GLFW_PRESS ) {
            // left button press
            m_node = GfxRenderer.Update_Pick_Node();
            if( m_node ) {
                Application.set_cursor( GLFW_CURSOR_DISABLED );
            }
            dynamic_cast<editor_ui*>( m_userinterface.get() )->set_node( m_node );
        }
        else {
            // left button release
            if( m_node ) {
                Application.set_cursor( GLFW_CURSOR_NORMAL );
            }
            // prime history stack for another snapshot
            m_takesnapshot = true;
        }
    }

    m_input.mouse.button( Button, Action );
}

void
editor_mode::on_event_poll() {

    m_input.poll();
}

bool
editor_mode::mode_translation() const {

    return ( false == Global.altState );
}

bool
editor_mode::mode_translation_vertical() const {

    return ( true == Global.shiftState );
}

bool
editor_mode::mode_rotation() const {

    return ( true == Global.altState );
}

bool
editor_mode::mode_snap() const {

    return ( true == Global.ctrlState );
}
