#include "uilayer.h"

class perfgraph_panel : public ui_panel
{
	std::array<float, 400> history = { 0 };
	size_t pos = 0;

	enum timers_e {
		gfx_total = 0,
		gfx_color,
		gfx_shadows,
		gfx_reflections,
		gfx_swap,
		gfx_gui,
		sim_total,
		sim_dynamics,
		sim_events,
		sim_ai,
		mainloop_total,
		TIMER_MAX
	};

	float max = 50.0f;
	timers_e current_timer = mainloop_total;

	std::vector<std::string> timer_label =
	{
	    "gfx_total",
	    "gfx_color",
	    "gfx_shadows",
	    "gfx_reflections",
	    "gfx_swap",
	    "gfx_gui",
	    "sim_total",
	    "sim_dynamics",
	    "sim_events",
	    "sim_ai",
	    "mainloop_total"
	};

  public:
	perfgraph_panel();

	void render_contents() override;
};
