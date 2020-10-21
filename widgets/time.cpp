#include "stdafx.h"
#include "widgets/time.h"
#include "simulationtime.h"
#include "Globals.h"

ui::time_panel::time_panel() : ui_panel(STR_C("Time and environment"), false)
{
	size.x = 450;
}

void ui::time_panel::render_contents()
{
	ImGui::SliderFloat(STR_C("Time"), &time, 0.0f, 24.0f, "%.1f");
	ImGui::SliderInt(STR_C("Day in year"), &yearday, 1, 365);
	ImGui::SliderFloat(STR_C("Visibility"), &fog, 50.0f, 3000.0f, "%.0f");
	ImGui::SliderFloat(STR_C("Overcast and precipitation"), &overcast, 0.0f, 2.0f, "%.1f");
	ImGui::SliderFloat(STR_C("Temperature"), &temperature, -20.0f, 40.0f, "%.0f");

	if (ImGui::Button(STR_C("Apply")))
	{
        m_relay.post(user_command::setdatetime, (double)yearday, time * 60.0, 1, 0);
		m_relay.post(user_command::setweather, fog, overcast, 1, 0);
		m_relay.post(user_command::settemperature, temperature, 0.0, 1, 0);
	}
}

void ui::time_panel::open()
{
	auto &data = simulation::Time.data();
	time = (float)data.wHour + (float)data.wMinute / 60.0f;

	yearday = simulation::Time.year_day();
	fog = Global.fFogEnd;

	overcast = Global.Overcast;
	temperature = Global.AirTemperature;

	is_open = true;
}
