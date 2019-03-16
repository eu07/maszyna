#include "uilayer.h"
#include "translation.h"
#include "command.h"

namespace ui
{
class time_panel : public ui_panel
{
	command_relay m_relay;

	float time;
	int yearday;
	float fog;
	float overcast;
	float temperature;

  public:
	time_panel();

	void render_contents() override;
	void open();
};
} // namespace ui
