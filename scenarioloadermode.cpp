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
#include "simulationenvironment.h"
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
	if (!Global.ready_to_load)
		// waiting for network connection
		return true;

	if (!state) {
		WriteLog("using simulation seed: " + std::to_string(Global.random_seed), logtype::generic);
        WriteLog("using simulation starting timestamp: " + std::to_string(Global.starting_timestamp), logtype::generic);

        Application.set_title( Global.AppName + " (" + Global.SceneryFile + ")" );
        WriteLog( "\nLoading scenario \"" + Global.SceneryFile + "\"..." );

		timestart = std::chrono::system_clock::now();
		state = simulation::State.deserialize_begin(Global.SceneryFile);
	}

	try {
		if (simulation::State.deserialize_continue(state))
			return true;
	}
	catch (invalid_scenery_exception &e) {
		ErrorLog( "Bad init: scenario loading failed" );
		Application.pop_mode();
	}

	WriteLog( "Scenario loading time: " + std::to_string( std::chrono::duration_cast<std::chrono::seconds>( ( std::chrono::system_clock::now() - timestart ) ).count() ) + " seconds" );
	// TODO: implement and use next mode cue

	Application.pop_mode();
	Application.push_mode( eu07_application::mode::driver );

    return true;
}

bool
scenarioloader_mode::is_command_processor() const {
	return false;
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
	m_userinterface->set_progress(STR("Loading scenery"));
    GfxRenderer->Render();
}

// maintenance method, called when the mode is deactivated
void
scenarioloader_mode::exit() {

    simulation::Time.init( Global.starting_timestamp );
    simulation::Environment.init();
}
