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
#include "launcher/scenery_list.h"
#include "launcher/keymapper.h"
#include "launcher/vehicle_picker.h"
#include "launcher/textures_scanner.h"
#include "launcher/scenery_scanner.h"

class launcher_ui : public ui_layer {
public:
	launcher_ui();
	bool on_key(const int Key, const int Action) override;

private:
	void render_() override;

	ui::vehicles_bank m_vehicles_bank;
	scenery_scanner m_scenery_scanner;

	ui::scenerylist_panel m_scenerylist_panel;
	ui::keymapper_panel m_keymapper_panel;
	ui::vehiclepicker_panel m_vehiclepicker_panel;
};
