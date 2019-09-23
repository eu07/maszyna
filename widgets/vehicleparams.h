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

  public:
	vehicleparams_panel(const std::string &vehicle);

	void render_contents() override;
};
} // namespace ui
