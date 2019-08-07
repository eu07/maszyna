/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "simulationstateserializer.h"
#include "Classes.h"
#include "lua.h"
#include "Event.h"

namespace simulation {

class state_manager {

public:
// methods
    // legacy method, calculates changes in simulation state over specified time
    void
        update( double Deltatime, int Iterationcount );
    void
        update_clocks();
    // restores simulation data from specified file. returns: true on success, false otherwise
    bool
        deserialize( std::string const &Scenariofile );
    // stores class data in specified file, in legacy (text) format
    void
        export_as_text( std::string const &Scenariofile ) const;

private:
// members
    state_serializer m_serializer;
};

// passes specified sound to all vehicles within range as a radio message broadcasted on specified channel
void radio_message( sound_source *Message, int const Channel );

extern state_manager State;
extern event_manager Events;
extern memory_table Memory;
extern path_table Paths;
extern traction_table Traction;
extern powergridsource_table Powergrid;
extern instance_table Instances;
extern vehicle_table Vehicles;
extern light_array Lights;
extern sound_table Sounds;
extern lua Lua;
extern particle_manager Particles;

extern scene::basic_region *Region;
extern TTrain *Train;

extern bool is_ready;

} // simulation

//---------------------------------------------------------------------------
