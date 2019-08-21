#pragma once

#include "uilayer.h"
#include "scenery_scanner.h"

namespace ui
{
class scenerylist_panel : public ui_panel
{
  public:
	scenerylist_panel(scenery_scanner &scanner);

	void render() override;

private:
	scenery_scanner &scanner;
	scenery_desc const *selected_scenery = nullptr;
	trainset_desc const *selected_trainset = nullptr;

	void draw_trainset(const trainset_desc &trainset);
	void draw_droptarget();
};
} // namespace ui
