#pragma once

#include "uilayer.h"
#include "driverkeyboardinput.h"

namespace ui
{
class keymapper_panel : public ui_panel
{
  public:
	keymapper_panel();

	void render() override;
	bool key(int key);

private:
	driverkeyboard_input keyboard;

	user_command bind_active = user_command::none;
};
} // namespace ui
