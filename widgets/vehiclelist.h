#include "uilayer.h"

namespace ui
{
class vehiclelist_panel : public ui_panel
{
  public:
	vehiclelist_panel() : ui_panel("Vehicle list", true) {}

	void render_contents() override;
};
} // namespace ui
