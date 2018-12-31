/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "scenarioloadermode.h"

#include "Globals.h"
#include "simulation.h"
#include "simulationtime.h"
#include "Timer.h"
#include "application.h"
#include "scenarioloaderuilayer.h"
#include "renderer.h"
#include "Logs.h"
#include "translation.h"

scenarioloader_mode::scenarioloader_mode() {

    m_userinterface = std::make_shared<scenarioloader_ui>();
}

// initializes internal data structures of the mode. returns: true on success, false otherwise
bool
scenarioloader_mode::init() {
    // nothing to do here
    return true;
}

// mode-specific update of simulation data. returns: false on error, true otherwise
bool
scenarioloader_mode::update() {

    WriteLog( "\nLoading scenario \"" + Global.SceneryFile + "\"..." );

    auto timestart = std::chrono::system_clock::now();

    if( true == simulation::State.deserialize( Global.SceneryFile ) ) {
        WriteLog( "Scenario loading time: " + std::to_string( std::chrono::duration_cast<std::chrono::seconds>( ( std::chrono::system_clock::now() - timestart ) ).count() ) + " seconds" );
        // TODO: implement and use next mode cue
        Application.pop_mode();
        Application.push_mode( eu07_application::mode::driver );
    }
    else {
        ErrorLog( "Bad init: scenario loading failed" );
        Application.pop_mode();
    }

    return true;
}

// maintenance method, called when the mode is activated
void
scenarioloader_mode::enter() {

    // TBD: hide cursor in fullscreen mode?
    Application.set_cursor( GLFW_CURSOR_NORMAL );

    simulation::is_ready = false;

    m_userinterface->set_background( "logo" );
    Application.set_title( Global.AppName + " (" + Global.SceneryFile + ")" );
    m_userinterface->set_progress();
    m_userinterface->set_progress(locale::strings[locale::string::ui_loading_scenery]);
    GfxRenderer.Render();
}

// maintenance method, called when the mode is deactivated
void
scenarioloader_mode::exit() {

    simulation::Time.init();
    simulation::Environment.init();
}
