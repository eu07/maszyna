#pragma once

#include "uilayer.h"
#include "scenery_scanner.h"

namespace ui
{
class scenerylist_panel : public ui_panel
{
  public:
	scenerylist_panel();

	void render() override;

private:
	scenery_scanner scanner;
	scenery_desc const *selected_scenery = nullptr;
};
} // namespace ui
