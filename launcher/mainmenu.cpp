#include "stdafx.h"
#include "launcher/mainmenu.h"

ui::mainmenu_panel::mainmenu_panel()
    : ui_panel(STR("Main menu"), true)
{

}

void ui::mainmenu_panel::render()
{
	if (!is_open)
		return;

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse;
	ImVec2 display_size = ImGui::GetIO().DisplaySize;
	ImGui::SetNextWindowPos(ImVec2(display_size.x * 0.66f, display_size.y / 2.0f));
	ImGui::SetNextWindowSize(ImVec2(200, -1));

	if (ImGui::Begin(m_name.c_str(), nullptr, flags)) {
		ImGui::Button(STR_C("Scenario list"), ImVec2(-1, 0));
		ImGui::Button(STR_C("Settings"), ImVec2(-1, 0));
		ImGui::Button(STR_C("Quit"), ImVec2(-1, 0));
	}
	ImGui::End();
}
