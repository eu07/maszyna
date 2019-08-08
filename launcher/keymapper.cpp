#include "stdafx.h"
#include "keymapper.h"
#include "simulation.h"

ui::keymapper_panel::keymapper_panel()
    : ui_panel(STR("Keymapper"), false)
{
	keyboard.init();
}

bool ui::keymapper_panel::key(int key)
{
	if (!is_open || bind_active == user_command::none)
		return false;

	auto it = keyboard.keytonamemap.find(key);
	if (it == keyboard.keytonamemap.end())
		return false;

	if (Global.shiftState)
		key |= keyboard_input::keymodifier::shift;
	if (Global.ctrlState)
		key |= keyboard_input::keymodifier::control;

	keyboard.bindings().insert_or_assign(bind_active, key);
	bind_active = user_command::none;
	keyboard.bind();

	return true;
}

void ui::keymapper_panel::render()
{
	if (!is_open)
		return;

	auto const panelname{(title.empty() ? m_name : title) + "###" + m_name};

	if (ImGui::Begin(panelname.c_str(), &is_open)) {
		if (ImGui::Button(STR_C("Load")))
			keyboard.init();

		ImGui::SameLine();

		if (ImGui::Button(STR_C("Save")))
			keyboard.dump_bindings();

		for (const std::pair<user_command, int> &binding : keyboard.bindings()) {
			ImGui::AlignTextToFramePadding();
			ImGui::Text(simulation::Commands_descriptions[static_cast<std::size_t>(binding.first)].name.c_str());

			std::string label;

			if (binding.second & keyboard_input::keymodifier::control)
				label += "ctrl + ";
			if (binding.second & keyboard_input::keymodifier::shift)
				label += "shift + ";

			auto it = keyboard.keytonamemap.find(binding.second & 0xFFFF);
			if (it != keyboard.keytonamemap.end())
				label += it->second;

			label = ToUpper(label);

			bool styles_pushed = false;
			if (bind_active == binding.first) {
				label = "?";
				ImVec4 active_col = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
				ImGui::PushStyleColor(ImGuiCol_Button, active_col);
				styles_pushed = true;
			}

			label += "##" + std::to_string((int)binding.first);

			ImGui::SameLine(ImGui::GetContentRegionAvailWidth() - 150);
			if (ImGui::Button(label.c_str(), ImVec2(150, 0))) {
				if (bind_active == binding.first)
					bind_active = user_command::none;
				else
					bind_active = binding.first;
			}

			if (styles_pushed)
				ImGui::PopStyleColor();
		}
	}

	ImGui::End();
}
