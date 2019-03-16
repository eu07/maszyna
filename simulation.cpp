/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "simulation.h"
#include "simulationtime.h"

#include "Globals.h"
#include "Event.h"
#include "MemCell.h"
#include "Track.h"
#include "Traction.h"
#include "TractionPower.h"
#include "sound.h"
#include "AnimModel.h"
#include "DynObj.h"
#include "lightarray.h"
#include "scene.h"
#include "Train.h"

namespace simulation {

state_manager State;
event_manager Events;
memory_table Memory;
path_table Paths;
traction_table Traction;
powergridsource_table Powergrid;
instance_table Instances;
vehicle_table Vehicles;
train_table Trains;
light_array Lights;
sound_table Sounds;
lua Lua;

scene::basic_region *Region { nullptr };
TTrain *Train { nullptr };

uint16_t prev_train_id { 0 };
bool is_ready { false };

std::shared_ptr<deserializer_state>
state_manager::deserialize_begin(std::string const &Scenariofile) {

	return m_serializer.deserialize_begin( Scenariofile );
}

bool
state_manager::deserialize_continue(std::shared_ptr<deserializer_state> state) {

	return m_serializer.deserialize_continue(state);
}

// stores class data in specified file, in legacy (text) format
void
state_manager::export_as_text( std::string const &Scenariofile ) const {

    return m_serializer.export_as_text( Scenariofile );
}

// legacy method, calculates changes in simulation state over specified time
void
state_manager::update( double const Deltatime, int Iterationcount ) {
    auto const totaltime { Deltatime * Iterationcount };
    // NOTE: we perform animations first, as they can determine factors like contact with powergrid
    TAnimModel::AnimUpdate( totaltime ); // wykonanie zakolejkowanych animacji

    simulation::Powergrid.update( totaltime );
    simulation::Vehicles.update( Deltatime, Iterationcount );
}

void state_manager::process_commands() {
	command_data commanddata;
	while( Commands.pop( commanddata, (uint32_t)command_target::simulation )) {
		if (commanddata.action == GLFW_RELEASE)
			continue;

		if (commanddata.command == user_command::debugtoggle)
			DebugModeFlag = !DebugModeFlag;

		if (commanddata.command == user_command::pausetoggle) {
			if( Global.iPause & 1 ) {
				// jeśli pauza startowa
				// odpauzowanie, gdy po wczytaniu miało nie startować
				Global.iPause ^= 1;
			}
			else {
				Global.iPause ^= 2; // zmiana stanu zapauzowania
			}
		}

		if (commanddata.command == user_command::focuspauseset) {
			if( commanddata.param1 == 1.0 )
				Global.iPause &= ~4; // odpauzowanie, gdy jest na pierwszym planie
			else
				Global.iPause |= 4; // włączenie pauzy, gdy nieaktywy
		}

		if (commanddata.command == user_command::entervehicle) {
			// przesiadka do innego pojazdu
			if (!commanddata.freefly)
				// only available in free fly mode
				continue;

			TDynamicObject *dynamic = std::get<TDynamicObject *>( simulation::Region->find_vehicle( commanddata.location, 50, true, false ) );

			if (!dynamic)
				continue;

			TTrain *train = simulation::Trains.find(dynamic->name());
			if (train)
				continue;

			train = new TTrain();
			if (train->Init(dynamic)) {
				simulation::Trains.insert(train, dynamic->name());
			}
			else {
				delete train;
				train = nullptr;
			}

		}

		if (commanddata.command == user_command::queueevent) {
			uint32_t id = std::round(commanddata.param1);
			basic_event *ev = Events.FindEventById(id); // TODO: depends on vector position
			Events.AddToQuery(ev, nullptr);
		}

		if (commanddata.command == user_command::setlight) {
			uint32_t id = commanddata.action;
			int light = std::round(commanddata.param1);
			float state = commanddata.param2;
			if (id < simulation::Instances.sequence().size())
				simulation::Instances.sequence()[id]->LightSet(light, state); // TODO: depends on vector position
		}

		if (commanddata.command == user_command::setdatetime) {
			int yearday = std::round(commanddata.param1);
			int minute = std::round(commanddata.param2 * 60.0);
			simulation::Time.set_time(yearday, minute);

			simulation::Environment.compute_season(yearday);
		}

		if (commanddata.command == user_command::setweather) {
			Global.fFogEnd = commanddata.param1;
			Global.Overcast = commanddata.param2;

			simulation::Environment.compute_weather();
		}

		if (commanddata.command == user_command::settemperature) {
			Global.AirTemperature = commanddata.param1;
		}

		if (DebugModeFlag) {
			if (commanddata.command == user_command::timejump) {
				Time.update(commanddata.param1);
			}
			else if (commanddata.command == user_command::timejumplarge) {
				Time.update(20.0 * 60.0);
			}
			else if (commanddata.command == user_command::timejumpsmall) {
				Time.update(5.0 * 60.0);
			}
		}
	}
}

TAnimModel * state_manager::create_model(const std::string &src, const std::string &name, const glm::dvec3 &position) {
	return m_serializer.create_model(src, name, position);
}

void state_manager::delete_model(TAnimModel *model) {
	Region->erase(model);
	Instances.purge(model);
}

void
state_manager::update_clocks() {

    // Ra 2014-07: przeliczenie kąta czasu (do animacji zależnych od czasu)
    auto const &time = simulation::Time.data();
    Global.fTimeAngleDeg = time.wHour * 15.0 + time.wMinute * 0.25 + ( ( time.wSecond + 0.001 * time.wMilliseconds ) / 240.0 );
    Global.fClockAngleDeg[ 0 ] = 36.0 * ( time.wSecond % 10 ); // jednostki sekund
    Global.fClockAngleDeg[ 1 ] = 36.0 * ( time.wSecond / 10 ); // dziesiątki sekund
    Global.fClockAngleDeg[ 2 ] = 36.0 * ( time.wMinute % 10 ); // jednostki minut
    Global.fClockAngleDeg[ 3 ] = 36.0 * ( time.wMinute / 10 ); // dziesiątki minut
    Global.fClockAngleDeg[ 4 ] = 36.0 * ( time.wHour % 10 ); // jednostki godzin
    Global.fClockAngleDeg[ 5 ] = 36.0 * ( time.wHour / 10 ); // dziesiątki godzin
}

// passes specified sound to all vehicles within range as a radio message broadcasted on specified channel
void
radio_message( sound_source *Message, int const Channel ) {

    if( Train != nullptr ) {
        Train->radio_message( Message, Channel );
    }
}

} // simulation

//---------------------------------------------------------------------------
