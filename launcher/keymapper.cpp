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
	// F10 unbinds the key.
	if (it->second.compare("f10") == 0)
		key = 0;

	// Replace the binding with a new key.
	auto it2 = keyboard.bindings().find(bind_active);
	if (it2 != keyboard.bindings().end()) {
		it2->second = std::tuple<int, std::string>(key, std::get<std::string>(it2->second));
	}
	bind_active = user_command::none;
	keyboard.bind();

	return true;
}

void ui::keymapper_panel::render_contents()
{
	if (ImGui::Button(STR_C("Load")))
		keyboard.init();

	ImGui::SameLine();

	if (ImGui::Button(STR_C("Save")))
		keyboard.dump_bindings();

	for (const std::pair<user_command, std::tuple<int, std::string>> &binding : keyboard.bindings()) {
		// Binding ID
		ImGui::AlignTextToFramePadding();
		ImGui::Text(simulation::Commands_descriptions[static_cast<std::size_t>(binding.first)].name.c_str());

		// Binding description
		std::string description = std::get<std::string>(binding.second);
		ImGui::SameLine(260);
		ImGui::Text((description.size() > 0 ? description : "(No description)").c_str());

		// Binding key button
		int keycode = std::get<int>(binding.second);
		std::string label;

		if (keycode & keyboard_input::keymodifier::control)
			label += "ctrl + ";
		if (keycode & keyboard_input::keymodifier::shift)
			label += "shift + ";

		auto it = keyboard.keytonamemap.find(keycode & 0xFFFF);
		if (it != keyboard.keytonamemap.end())
			label += it->second;

		label = ToUpper(label);

		bool styles_pushed = false;
		if (bind_active == binding.first) {
			label = "Press key (F10 to unbind)";
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
