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
#include "simulationenvironment.h"

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
#include "particles.h"
#include "scene.h"
#include "Train.h"
#include "application.h"
#include "Logs.h"
#include "Driver.h"

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
particle_manager Particles;

#ifdef WITH_LUA
lua Lua;
#endif

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

void
state_manager::init_scripting_interface() {

    // create scenario data memory cells
    {
        auto *memorycell = new TMemCell( {
            0, -1,
            "__simulation.weather",
            "memcell" } );
        simulation::Memory.insert( memorycell );
        simulation::Region->insert( memorycell );
    }
    {
        auto *memorycell = new TMemCell( {
            0, -1,
            "__simulation.time",
            "memcell" } );
        simulation::Memory.insert( memorycell );
        simulation::Region->insert( memorycell );
    }
    {
        auto *memorycell = new TMemCell( {
            0, -1,
            "__simulation.date",
            "memcell" } );
        simulation::Memory.insert( memorycell );
        simulation::Region->insert( memorycell );
    }
}

// legacy method, calculates changes in simulation state over specified time
void
state_manager::update( double const Deltatime, int Iterationcount ) {
    // aktualizacja animacji krokiem FPS: dt=krok czasu [s], dt*iter=czas od ostatnich przeliczeń
    if (Deltatime == 0.0) { return; }

    auto const totaltime { Deltatime * Iterationcount };
    // NOTE: we perform animations first, as they can determine factors like contact with powergrid
    TAnimModel::AnimUpdate( totaltime ); // wykonanie zakolejkowanych animacji

    simulation::Powergrid.update( totaltime );
    simulation::Vehicles.update( Deltatime, Iterationcount );
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

void
state_manager::update_scripting_interface() {

    auto *weather{ Memory.find( "__simulation.weather" ) };
    auto *time{ Memory.find( "__simulation.time" ) };
    auto *date{ Memory.find( "__simulation.date" ) };

    if( simulation::is_ready ) {
        // potentially adjust weather
        if( weather->Value1() != m_scriptinginterface.weather->Value1() ) {
            Global.Overcast = clamp<float>( weather->Value1(), 0, 2 );
            simulation::Environment.compute_weather();
        }
        if( weather->Value2() != m_scriptinginterface.weather->Value2() ) {
            Global.fFogEnd = clamp<float>( weather->Value2(), 10, 25000 );
        }
    }
    else {
        m_scriptinginterface.weather = std::make_shared<TMemCell>( scene::node_data() );
        m_scriptinginterface.date = std::make_shared<TMemCell>( scene::node_data() );
        m_scriptinginterface.time = std::make_shared<TMemCell>( scene::node_data() );
    }

    // update scripting interface
    weather->UpdateValues(
        Global.Weather,
        Global.Overcast,
        Global.fFogEnd,
        basic_event::flags::text | basic_event::flags::value1 | basic_event::flags::value2 );

    time->UpdateValues(
        Global.Period,
        Time.data().wHour,
        Time.data().wMinute,
        basic_event::flags::text | basic_event::flags::value1 | basic_event::flags::value2 );

    date->UpdateValues(
        Global.Season,
        Time.year_day(),
        0,
        basic_event::flags::text | basic_event::flags::value1 );

    // cache cell state to detect potential script-issued changes on next cycle
    *m_scriptinginterface.weather = *weather;
    *m_scriptinginterface.time = *time;
    *m_scriptinginterface.date = *date;
}

void state_manager::process_commands() {
	command_data commanddata;
	while( Commands.pop( commanddata, (uint32_t)command_target::simulation )) {
		if (commanddata.command == user_command::consistreleaser) {
			TDynamicObject *found_vehicle = simulation::Vehicles.find(commanddata.payload);
			TDynamicObject *vehicle = found_vehicle;

			while (vehicle) {
				vehicle->MoverParameters->Hamulec->Releaser(commanddata.action != GLFW_RELEASE ? 1 : 0);
				vehicle = vehicle->Next();
			}

			vehicle = found_vehicle;
			while (vehicle) {
				vehicle->MoverParameters->Hamulec->Releaser(commanddata.action != GLFW_RELEASE ? 1 : 0);
				vehicle = vehicle->Prev();
			}
		}

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
            if( commanddata.payload == "ghostview" ) {
                continue;
            }

            if (!commanddata.freefly)
                // only available in free fly mode
                continue;

            // NOTE: because malformed scenario can have vehicle name duplicates we first try to locate vehicle in world, with name search as fallback
            TDynamicObject *targetvehicle = std::get<TDynamicObject *>( simulation::Region->find_vehicle( commanddata.location, 50, false, false ) );
            if( ( targetvehicle == nullptr ) || ( targetvehicle->name() != commanddata.payload ) ) {
                targetvehicle = simulation::Vehicles.find( commanddata.payload );
            }

			if (!targetvehicle)
				continue;

            auto *senderlocaltrain { simulation::Trains.find_id( static_cast<std::uint16_t>( commanddata.param2 ) ) };
            if( senderlocaltrain  ) {
                auto *currentvehicle { senderlocaltrain->Dynamic() };
                auto const samevehicle { currentvehicle == targetvehicle };

                if( samevehicle ) {
                    // we already control desired vehicle so don't overcomplicate things
                    continue;
                }

                auto const sameconsist{
                    ( targetvehicle->ctOwner == currentvehicle->Mechanik )
                 || ( targetvehicle->ctOwner == currentvehicle->ctOwner ) };
                auto const isincharge{ currentvehicle->Mechanik->primary() };
                auto const aidriveractive{ currentvehicle->Mechanik->AIControllFlag };
                // TODO: support for primary mode request passed as commanddata.param1
                if( !sameconsist && isincharge ) {
                    // oddajemy dotychczasowy AI
                    if (currentvehicle->Mechanik != nullptr)
                        currentvehicle->Mechanik->TakeControl( true );
                }

                if( sameconsist && !aidriveractive ) {
                    // since we're consist owner we can simply move to the destination vehicle
                    senderlocaltrain->MoveToVehicle( targetvehicle );
                    if (senderlocaltrain->Dynamic()->Mechanik != nullptr)
                        senderlocaltrain->Dynamic()->Mechanik->TakeControl( false, true );
                }
            }

            auto *train { simulation::Trains.find( targetvehicle->name() ) };

            if (train)
                continue;

			train = new TTrain();
			if (train->Init(targetvehicle)) {
				simulation::Trains.insert(train);
			}
			else {
				delete train;
				train = nullptr;
                if( targetvehicle->name() == Global.local_start_vehicle ) {
                    ErrorLog( "Failed to initialize player train, \"" + Global.local_start_vehicle + "\"" );
                    Global.local_start_vehicle = "ghostview";
                }
			}

        }

		if (commanddata.command == user_command::queueevent) {
			std::istringstream ss(commanddata.payload);

			std::string event_name;
			std::string vehicle_name;
			std::getline(ss, event_name, '%');
			std::getline(ss, vehicle_name, '%');

			basic_event *ev = Events.FindEvent(event_name);
			TDynamicObject *vehicle = nullptr;
			if (!vehicle_name.empty())
				vehicle = simulation::Vehicles.find(vehicle_name);

			if (ev)
				Events.AddToQuery(ev, vehicle);
		}

		if (commanddata.command == user_command::setlight) {
			int light = std::round(commanddata.param1);
			float state = commanddata.param2;
			TAnimModel *model = simulation::Instances.find(commanddata.payload);
			if (model)
				model->LightSet(light, state);
		}

		if (commanddata.command == user_command::setdatetime) {
			int yearday = std::round(commanddata.param1);
			int minute = std::round(commanddata.param2);
			simulation::Time.set_time(yearday, minute);

            auto const weather { Global.Weather };
            simulation::Environment.compute_season(yearday);
            if( weather != Global.Weather ) {
                // HACK: force re-calculation of precipitation
                Global.Overcast = clamp( Global.Overcast - 0.0001f, 0.0f, 2.0f );
            }

            simulation::Environment.update_moon();
        }

		if (commanddata.command == user_command::setweather) {
			Global.fFogEnd = commanddata.param1;
			Global.Overcast = commanddata.param2;

			simulation::Environment.compute_weather();
		}

		if (commanddata.command == user_command::settemperature) {
			Global.AirTemperature = commanddata.param1;
		}

		if (commanddata.command == user_command::insertmodel) {
			std::istringstream ss(commanddata.payload);

			std::string name;
			std::string data;
			std::getline(ss, name, ':');
			std::getline(ss, data, ':');

			TAnimModel *model = simulation::State.create_model(data, name, commanddata.location);
            simulation::State.create_eventlauncher("node -1 0 launcher eventlauncher 0 0 0 0.8 none -10000.0 obstacle_collision traintriggered end", name + "_snd", commanddata.location);
		}

		if (commanddata.command == user_command::deletemodel) {
			simulation::State.delete_model(simulation::Instances.find(commanddata.payload));
			simulation::State.delete_eventlauncher(simulation::Events.FindEventlauncher(commanddata.payload + "_snd"));
		}

		if (commanddata.command == user_command::globalradiostop) {
			simulation::Region->RadioStop( commanddata.location );
		}

		if (commanddata.command == user_command::resetconsist) {
			TDynamicObject *found_vehicle = simulation::Vehicles.find(commanddata.payload);
			TDynamicObject *vehicle = found_vehicle;

			while (vehicle) {
				if (vehicle->Next())
					vehicle = vehicle->Next();
				else
					break;
			}

			while (vehicle) {
				vehicle->MoverParameters->DamageFlag = 0;
				vehicle->MoverParameters->EngDmgFlag = 0;
				vehicle->MoverParameters->V = 0.000001; // HACK: force vehicle position re-calculation
				vehicle->MoverParameters->DistCounter = 0.0;
				vehicle->MoverParameters->WheelFlat = 0.0;
				vehicle->MoverParameters->AlarmChainFlag = false;
				vehicle->MoverParameters->OffsetTrackH = 0.0;
				vehicle->MoverParameters->OffsetTrackV = 0.0;

				// pantographs
				for( auto idx = 0; idx < vehicle->iAnimType[ ANIM_PANTS ]; ++idx ) {
					auto &pantograph { *( vehicle->pants[ idx ].fParamPants ) };
					if( pantograph.PantWys >= 0.0 )  // negative value means pantograph is broken
						continue;
					pantograph.fAngleL = pantograph.fAngleL0;
					pantograph.fAngleU = pantograph.fAngleU0;
					pantograph.PantWys =
					pantograph.fLenL1 * std::sin( pantograph.fAngleL )
						+ pantograph.fLenU1 * std::sin( pantograph.fAngleU )
						+ pantograph.fHeight; 
					vehicle->MoverParameters->EnginePowerSource.CollectorParameters.CollectorsNo;
				}

				vehicle = vehicle->Prev();
			}
		}

		if (commanddata.command == user_command::fillcompressor) {
			TDynamicObject *vehicle = simulation::Vehicles.find(commanddata.payload);
			vehicle->MoverParameters->CompressedVolume = 8.0f * vehicle->MoverParameters->VeselVolume;
		}

		if (commanddata.command == user_command::dynamicmove) {
			TDynamicObject *vehicle = simulation::Vehicles.find(commanddata.payload);
			if (vehicle)
				vehicle->move_set(commanddata.param1);
		}

		if (commanddata.command == user_command::consistteleport) {
			std::istringstream ss(commanddata.payload);

			std::string track_name;
			std::string vehicle_name;
			std::getline(ss, vehicle_name, '%');
			std::getline(ss, track_name, '%');

			TTrack *track = simulation::Paths.find(track_name);
			TDynamicObject *vehicle = simulation::Vehicles.find(vehicle_name);

			while (vehicle) {
				if (vehicle->Next())
					vehicle = vehicle->Next();
				else
					break;
			}

			double offset = 0.0;

			while (vehicle) {
				offset += vehicle->MoverParameters->Dim.L;
				vehicle->place_on_track(track, offset, false);
				vehicle = vehicle->Prev();
			}
		}

		if (commanddata.command == user_command::spawntrainset) {

		}

		if (commanddata.command == user_command::pullalarmchain) {
			TDynamicObject *vehicle = simulation::Vehicles.find(commanddata.payload);
			if (vehicle)
				vehicle->MoverParameters->AlarmChainSwitch(true);
		}

		if (commanddata.command == user_command::sendaicommand) {
			std::istringstream ss(commanddata.payload);

			std::string vehicle_name;
			std::string command;
			std::getline(ss, vehicle_name, '%');
			std::getline(ss, command, '%');

			TDynamicObject *vehicle = simulation::Vehicles.find(vehicle_name);
			glm::dvec3 location = commanddata.location;
			if (vehicle && vehicle->Mechanik)
				vehicle->Mechanik->PutCommand(command, commanddata.param1, commanddata.param2, &location);
		}

		if (commanddata.command == user_command::quitsimulation) {
            // TBD: allow clients to go into offline mode?
            Application.queue_quit(true);
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

TEventLauncher * state_manager::create_eventlauncher(const std::string &src, const std::string &name, const glm::dvec3 &position) {
	return m_serializer.create_eventlauncher(src, name, position);
}

void state_manager::delete_model(TAnimModel *model) {
	Region->erase(model);
	Instances.purge(model);
}

void state_manager::delete_eventlauncher(TEventLauncher *launcher) {
	launcher->dRadius = 0.0f; // disable it
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
