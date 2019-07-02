#include "stdafx.h"
#include "widgets/vehiclelist.h"
#include "simulation.h"
#include "Driver.h"
#include "widgets/vehicleparams.h"

ui::vehiclelist_panel::vehiclelist_panel(ui_layer &parent)
    : ui_panel(STR_C(vehiclelist_window), false), m_parent(parent)
{

}

void ui::vehiclelist_panel::render_contents()
{
	for (TDynamicObject *vehicle : simulation::Vehicles.sequence())
	{
		if (!vehicle->Mechanik)
			continue;

		std::string name = vehicle->name();
		double speed = vehicle->GetVelocity();
		std::string timetable;
		if (vehicle->Mechanik)
			timetable = vehicle->Mechanik->TrainName() + ", ";

		std::string label = std::string(name + ", " + timetable + std::to_string(speed) + " km/h###");

		ImGui::PushID(vehicle);
		if (ImGui::Button(label.c_str())) {
			ui_panel *panel = new vehicleparams_panel(vehicle->name());
			m_parent.add_owned_panel(panel);
		}
		ImGui::PopID();
	}
}
