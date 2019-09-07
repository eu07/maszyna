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
	std::atomic_bool exit_thread = true;
	std::thread workthread;

	uint8_t* image_ptr = nullptr;
	std::mutex mutex;

	std::optional<opengl_texture> texture;

	void workthread_func();

  public:
	cameraview_panel();
	~cameraview_panel();

	void render() override;
	void render_contents() override;
};
} // namespace ui
