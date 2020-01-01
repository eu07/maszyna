#pragma once

#include "uilayer.h"
#include "translation.h"
#include "command.h"
#include "renderer.h"

namespace ui
{
class vehicleparams_panel : public ui_panel
{
	std::string m_vehicle_name;
	command_relay m_relay;
	texture_handle vehicle_mini;

	void draw_infobutton(const char *str, ImVec2 pos = ImVec2(-1.0f, -1.0f), const ImVec4 color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
	void draw_mini(const TMoverParameters &mover);
  public:
	vehicleparams_panel(const std::string &vehicle);

	void render_contents() override;
};
} // namespace ui
