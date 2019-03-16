#include "uilayer.h"
#include "translation.h"
#include "command.h"

namespace ui
{
class vehicleparams_panel : public ui_panel
{
	std::string m_vehicle_name;

  public:
	vehicleparams_panel();

	void render_contents() override;
};
} // namespace ui
