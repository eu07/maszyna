/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/
#pragma once

#include "Classes.h"

class AirCoupler
{
public:
	AirCoupler();
	~AirCoupler();
	//Reset members.
	void clear();
	//Looks for submodels.
	void init(const std::string& asName, TModel3d* model);
	//Loads info about coupler.
	void load(cParser* parser, TModel3d* model);
	int getStatus();
	//Turns on straight coupler.
	inline void turnOn()
	{
		on = true;
		xOn = false;
		update();
	};
	//Turns on disconnected coupler.
	inline void turnOff()
	{
		on = false;
		xOn = false;
		update();
	};
	//Turns on slanted coupler.
	inline void turnxOn()
	{
		on = false;
		xOn = true;
		update();
	};

private:
	TSubModel *modelOn, *modelOff, *modelxOn;
	bool on;
	bool xOn;
	void update();
};
