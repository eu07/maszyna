#include "stdafx.h"
#include "widgets/perfgraphs.h"
#include "Timer.h"

perfgraph_panel::perfgraph_panel()
    : ui_panel(STR("Performance"), false)
{

}

void perfgraph_panel::render_contents() {
	if (ImGui::BeginCombo(STR_C("Timer"), timer_label[current_timer].c_str())) // The second parameter is the label previewed before opening the combo.
	{
		for (size_t i = 0; i < (size_t)TIMER_MAX; i++)
		{
			bool is_selected = (current_timer == (timers_e)i);
			if (ImGui::Selectable(timer_label[i].c_str(), is_selected))
				current_timer = (timers_e)i;
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	Timer::stopwatch *stopwatch = nullptr;

	if (current_timer == gfx_total)
		stopwatch = &Timer::subsystem.gfx_total;
	else if (current_timer == gfx_color)
		stopwatch = &Timer::subsystem.gfx_color;
	else if (current_timer == gfx_shadows)
		stopwatch = &Timer::subsystem.gfx_shadows;
	else if (current_timer == gfx_reflections)
		stopwatch = &Timer::subsystem.gfx_reflections;
	else if (current_timer == gfx_swap)
		stopwatch = &Timer::subsystem.gfx_swap;
	else if (current_timer == gfx_gui)
		stopwatch = &Timer::subsystem.gfx_gui;
	else if (current_timer == sim_total)
		stopwatch = &Timer::subsystem.sim_total;
	else if (current_timer == sim_dynamics)
		stopwatch = &Timer::subsystem.sim_dynamics;
	else if (current_timer == sim_events)
		stopwatch = &Timer::subsystem.sim_events;
	else if (current_timer == sim_ai)
		stopwatch = &Timer::subsystem.sim_ai;
	else if (current_timer == mainloop_total)
		stopwatch = &Timer::subsystem.mainloop_total;

	if (!stopwatch)
		return;

	history[pos] = stopwatch->last().count() / 1000.0;
	const std::string label = std::to_string(history[pos]) + "ms";

	pos++;
	if (pos >= history.size())
		pos = 0;

	ImGui::SliderFloat(STR_C("Range"), &max, 0.1f, 250.0f);
	ImGui::PlotLines("##timer", &history[0], history.size(), pos, label.c_str(), 0.0f, max, ImVec2(500, 200));
}
