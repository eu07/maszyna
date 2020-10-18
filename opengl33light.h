/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "light.h"

struct opengl33_light : public basic_light {

	GLuint id{(GLuint)-1};

	float factor;

	void apply_intensity(float const Factor = 1.0f);
	void apply_angle();

	opengl33_light &operator=(basic_light const &Right) {
		basic_light::operator=(Right);
		return *this; }
};

//---------------------------------------------------------------------------
