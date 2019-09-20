/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "stdafx.h"
#include "uilayer.h"

namespace ui
{
class cameraview_panel : public ui_panel
{
public:
	enum state_e {
		IDLE,
		PREVIEW,
		RECORDING
	};

private:
	std::atomic_bool exit_thread = true;
	std::thread workthread;

	uint8_t* image_ptr = nullptr;
	std::mutex mutex;

	state_e state = IDLE;

	std::optional<opengl_texture> texture;

	void workthread_func();

  public:
	cameraview_panel();
	~cameraview_panel();

	void render() override;
	void render_contents() override;
	bool set_state(state_e e);

	std::string rec_name;
};
} // namespace ui
