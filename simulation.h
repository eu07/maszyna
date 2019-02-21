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
#include "Train.h"

namespace simulation {

class state_manager {

public:
// methods
    // legacy method, calculates changes in simulation state over specified time
    void
        update( double Deltatime, int Iterationcount );
    void
        update_clocks();
	// starts deserialization from specified file, returns context pointer on success, throws otherwise
	std::shared_ptr<deserializer_state>
	    deserialize_begin(std::string const &Scenariofile);
	// continues deserialization for given context, amount limited by time, returns true if needs to be called again
	bool
	    deserialize_continue(std::shared_ptr<deserializer_state> state);
    // stores class data in specified file, in legacy (text) format
    void
        export_as_text( std::string const &Scenariofile ) const;
	// process input commands
	void
	    process_commands();

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
extern train_table Trains;
extern light_array Lights;
extern sound_table Sounds;
extern lua Lua;

extern scene::basic_region *Region;
extern TTrain *Train;

extern uint16_t prev_train_id;
extern bool is_ready;

struct deserializer_state;

} // simulation

//---------------------------------------------------------------------------
