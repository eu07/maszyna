#pragma once

#include "uilayer.h"
#include "textures_scanner.h"

namespace ui {

class vehiclepicker_panel : public ui_panel
{
  public:
	vehiclepicker_panel();

	void render() override;

private:
	bool selectable_image(const char *desc, bool selected, const deferred_image *image, bool pickable = false);

	vehicle_type selected_type = vehicle_type::none;
	std::shared_ptr<const vehicle_desc> selected_vehicle;
	const std::string *selected_group = nullptr;
	const skin_set *selected_skinset = nullptr;
	bool display_by_groups = false;

	vehicles_bank bank;
};
} // namespace ui
