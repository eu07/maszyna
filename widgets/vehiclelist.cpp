#include "stdafx.h"
#include "widgets/vehiclelist.h"
#include "simulation.h"
#include "Driver.h"

void ui::vehiclelist_panel::render_contents() {
	for (TDynamicObject* vehicle : simulation::Vehicles.sequence()) {
		if (vehicle->Prev())
			continue;

		std::string name = vehicle->name();
		double speed = vehicle->GetVelocity();
		std::string timetable;
		if (vehicle->Mechanik)
			timetable = vehicle->Mechanik->TrainName() + ", ";

		std::string label = std::string(name + ", " + timetable + std::to_string(speed) + " km/h");
		if (!vehicle->Next())
			ImGui::TextUnformatted(label.c_str());
		else if (ImGui::TreeNode(vehicle, label.c_str())) {
			vehicle = vehicle->Next();
			while (vehicle) {
				ImGui::TextUnformatted(vehicle->name().c_str());
				vehicle = vehicle->Next();
			}
			ImGui::TreePop();
		}
	}

	//ImGui::ShowDemoWindow();
}
