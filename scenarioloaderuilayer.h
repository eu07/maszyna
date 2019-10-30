/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "uilayer.h"

class scenarioloader_ui : public ui_layer {
public:
	scenarioloader_ui() : ui_layer() {
		load_random_background();
	}
    // TODO: implement mode-specific elements
};
