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

namespace simulation {

state_manager State;
event_manager Events;
memory_table Memory;
path_table Paths;
traction_table Traction;
powergridsource_table Powergrid;
sound_table Sounds;
instance_table Instances;
vehicle_table Vehicles;
light_array Lights;
particle_manager Particles;

scene::basic_region *Region { nullptr };
TTrain *Train { nullptr };

bool is_ready { false };

bool
state_manager::deserialize( std::string const &Scenariofile ) {

    return m_serializer.deserialize( Scenariofile );
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
    if (Deltatime == 0.0) {
        return;
    }

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
        basic_event::flags::text | basic_event::flags::value_1 | basic_event::flags::value_2 );

    time->UpdateValues(
        Global.Period,
        Time.data().wHour,
        Time.data().wMinute,
        basic_event::flags::text | basic_event::flags::value_1 | basic_event::flags::value_2 );

    date->UpdateValues(
        Global.Season,
        Time.year_day(),
        0,
        basic_event::flags::text | basic_event::flags::value_1 );

    // cache cell state to detect potential script-issued changes on next cycle
    *m_scriptinginterface.weather = *weather;
    *m_scriptinginterface.time = *time;
    *m_scriptinginterface.date = *date;
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
