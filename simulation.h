﻿/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "parser.h"
#include "scene.h"
#include "MemCell.h"
#include "EvLaunch.h"
#include "Track.h"
#include "Traction.h"
#include "TractionPower.h"
#include "sound.h"
#include "AnimModel.h"
#include "DynObj.h"
#include "Driver.h"
#include "lightarray.h"
#include "Event.h"
#include "lua.h"

namespace simulation
{
	class state_manager
	{
	public:
		// types

		// methods
		// legacy method, calculates changes in simulation state over specified time
		void update(double Deltatime, int Iterationcount);
		bool deserialize(std::string const& Scenariofile);
		// stores class data in specified file, in legacy (text) format
		void export_as_text(std::string const& Scenariofile) const;

	private:
		// methods
		// restores class data from provided stream
		void deserialize(cParser& Input, scene::scratch_data& Scratchpad);
		void deserialize_atmo(cParser& Input, scene::scratch_data& Scratchpad);
		void deserialize_camera(cParser& Input, scene::scratch_data& Scratchpad);
		void deserialize_config(cParser& Input, scene::scratch_data& Scratchpad);
		void deserialize_description(cParser& Input, scene::scratch_data& Scratchpad);
		void deserialize_event(cParser& Input, scene::scratch_data& Scratchpad);
		void deserialize_lua(cParser& Input, scene::scratch_data& Scratchpad);
		void deserialize_firstinit(cParser& Input, scene::scratch_data& Scratchpad);
		void deserialize_light(cParser& Input, scene::scratch_data& Scratchpad);
		void deserialize_node(cParser& Input, scene::scratch_data& Scratchpad);
		void deserialize_origin(cParser& Input, scene::scratch_data& Scratchpad);
		void deserialize_endorigin(cParser& Input, scene::scratch_data& Scratchpad);
		void deserialize_rotate(cParser& Input, scene::scratch_data& Scratchpad);
		void deserialize_sky(cParser& Input, scene::scratch_data& Scratchpad);
		void deserialize_test(cParser& Input, scene::scratch_data& Scratchpad);
		void deserialize_time(cParser& Input, scene::scratch_data& Scratchpad);
		void deserialize_trainset(cParser& Input, scene::scratch_data& Scratchpad);
		void deserialize_endtrainset(cParser& Input, scene::scratch_data& Scratchpad);
		TTrack* deserialize_path(cParser& Input, scene::scratch_data& Scratchpad, scene::node_data const& Nodedata);
		TTraction* deserialize_traction(cParser& Input, scene::scratch_data& Scratchpad, scene::node_data const& Nodedata);
		TTractionPowerSource* deserialize_tractionpowersource(cParser& Input, scene::scratch_data& Scratchpad, scene::node_data const& Nodedata);
		TMemCell* deserialize_memorycell(cParser& Input, scene::scratch_data& Scratchpad, scene::node_data const& Nodedata);
		TEventLauncher* deserialize_eventlauncher(cParser& Input, scene::scratch_data& Scratchpad, scene::node_data const& Nodedata);
		TAnimModel* deserialize_model(cParser& Input, scene::scratch_data& Scratchpad, scene::node_data const& Nodedata);
		TDynamicObject* deserialize_dynamic(cParser& Input, scene::scratch_data& Scratchpad, scene::node_data const& Nodedata);
		sound_source* deserialize_sound(cParser& Input, scene::scratch_data& Scratchpad, scene::node_data const& Nodedata);
		// skips content of stream until specified token
		void skip_until(cParser& Input, std::string const& Token);
		// transforms provided location by specifed rotation and offset
		glm::dvec3 transform(glm::dvec3 Location, scene::scratch_data const& Scratchpad);
	};

	extern state_manager State;
	extern event_manager Events;
	extern memory_table Memory;
	extern path_table Paths;
	extern traction_table Traction;
	extern powergridsource_table Powergrid;
	extern InstanceTable Instances;
	extern vehicle_table Vehicles;
	extern light_array Lights;
	extern sound_table Sounds;
	extern lua Lua;

	extern scene::basic_region* Region;

} // simulation

//---------------------------------------------------------------------------
