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
#include "Event.h"
#include "Train.h"
#include "particles.h"

#ifdef WITH_LUA
#include "lua.h"
#endif

namespace simulation {

class state_manager {

public:
// methods
    void
        init_scripting_interface();
    // legacy method, calculates changes in simulation state over specified time
    void
        update( double Deltatime, int Iterationcount );
    void
        update_clocks();
    void
        update_scripting_interface();
    // process input commands
    void
        process_commands();
 	// create model from node string
	TAnimModel *
	    create_model(const std::string &src, const std::string &name, const glm::dvec3 &position);
	// create eventlauncher from node string
	TEventLauncher *
	    create_eventlauncher(const std::string &src, const std::string &name, const glm::dvec3 &position);
	// delete TAnimModel instance
	void
	    delete_model(TAnimModel *model);
	// delete TEventLauncher instance
	void
	    delete_eventlauncher(TEventLauncher *launcher);
	// starts deserialization from specified file, returns context pointer on success, throws otherwise
	std::shared_ptr<deserializer_state>
	    deserialize_begin(std::string const &Scenariofile);
	// continues deserialization for given context, amount limited by time, returns true if needs to be called again
	bool
	    deserialize_continue(std::shared_ptr<deserializer_state> state);
    // stores class data in specified file, in legacy (text) format
    void
        export_as_text( std::string const &Scenariofile ) const;

private:
// members
    state_serializer m_serializer;
    struct {
        std::shared_ptr<TMemCell> weather, time, date;
    } m_scriptinginterface;
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
extern particle_manager Particles;
#ifdef WITH_LUA
extern lua Lua;
#endif

extern scene::basic_region *Region;
extern TTrain *Train;

extern uint16_t prev_train_id;
extern bool is_ready;

struct deserializer_state;

} // simulation

//---------------------------------------------------------------------------
