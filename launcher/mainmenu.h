#pragma once

#include "uilayer.h"

namespace ui
{
class mainmenu_panel : public ui_panel
{
  public:
	mainmenu_panel();

	void render() override;
};
} // namespace ui
