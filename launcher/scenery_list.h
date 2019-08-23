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
	scenery_desc *selected_scenery = nullptr;
	trainset_desc *selected_trainset = nullptr;
	deferred_image placeholder_mini;

	void draw_trainset(trainset_desc &trainset);
	void draw_droptarget(trainset_desc &trainset, int position);
};
} // namespace ui
