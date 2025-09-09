#include "stdafx.h"
#include "launcher/launcheruilayer.h"
#include "application.h"
#include "translation.h"

launcher_ui::launcher_ui()
    : m_scenery_scanner(m_vehicles_bank), m_scenerylist_panel(m_scenery_scanner)
{
	m_vehicles_bank.scan_textures();
	m_scenery_scanner.scan();

	add_external_panel(&m_scenerylist_panel);
	add_external_panel(&m_keymapper_panel);
	add_external_panel(&m_vehiclepicker_panel);

	open_panel(&m_vehiclepicker_panel);
	m_suppress_menu = true;

	load_random_background();
}

bool launcher_ui::on_key(const int Key, const int Action)
{
	if (m_scenerylist_panel.on_key(Key, Action))
		return true;
	if (m_keymapper_panel.key(Key))
		return true;

	return ui_layer::on_key(Key, Action);
}

void launcher_ui::render_()
{
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove;
	const float topbar_height = 50 * Global.ui_scale;
	const ImVec2 topbar_button_size = ImVec2(150 * Global.ui_scale, topbar_height - 16);
	ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(Global.window_size.x, topbar_height));

	if (ImGui::Begin(STR_C("Main menu"), nullptr, flags)) {
		if (ImGui::Button(STR_C("Scenario list"), topbar_button_size))
			open_panel(&m_scenerylist_panel);
		ImGui::SameLine();
		if (ImGui::Button(STR_C("Vehicle list"), topbar_button_size))
			open_panel(&m_vehiclepicker_panel);
		ImGui::SameLine();
		if (ImGui::Button(STR_C("Keymapper"), topbar_button_size))
			open_panel(&m_keymapper_panel);
		ImGui::SameLine();
        if (ImGui::Button(STR_C("Quit"), topbar_button_size))
            Application.queue_quit(false);
	}
	ImGui::End();
}

void launcher_ui::close_panels()
{
	m_scenerylist_panel.is_open = false;
	m_vehiclepicker_panel.is_open = false;
	m_keymapper_panel.is_open = false;
}

void launcher_ui::open_panel(ui_panel* panel)
{
	close_panels();
	const float topbar_height = 50 * Global.ui_scale;
	panel->pos = glm::vec2(0, topbar_height);
	panel->size = glm::vec2(Global.window_size.x, Global.window_size.y - topbar_height);
	panel->no_title_bar = true;
	panel->is_open = true;
}
