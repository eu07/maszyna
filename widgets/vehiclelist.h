#include "uilayer.h"

namespace ui
{
class vehiclelist_panel : public ui_panel
{
	ui_layer &m_parent;

  public:
	vehiclelist_panel(ui_layer &parent);

	void render_contents() override;
};
} // namespace ui
