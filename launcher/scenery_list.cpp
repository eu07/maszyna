#include "stdafx.h"
#include "scenery_list.h"
#include "imgui/imgui.h"
#include "utilities.h"
#include "renderer.h"
#include <filesystem>

ui::scenerylist_panel::scenerylist_panel()
    : ui_panel(STR("Scenario list"), false)
{
	scanner.scan();
}

void ui::scenerylist_panel::render()
{
	if (!is_open)
		return;

	auto const panelname{(title.empty() ? m_name : title) + "###" + m_name};

	if (ImGui::Begin(panelname.c_str(), &is_open)) {
		ImGui::Columns(3);

		if (ImGui::BeginChild("child1", ImVec2(0, -100))) {
			std::string prev_prefix;
			bool collapse_open = false;

			for (auto const &desc : scanner.scenarios) {
				std::string name = desc.path.stem().string();
				std::string prefix = name.substr(0, name.find_first_of("-_"));
				if (prefix.empty())
					prefix = name;

				bool just_opened = false;
				if (prefix != prev_prefix) {
					collapse_open = ImGui::CollapsingHeader(prefix.c_str());
					just_opened = ImGui::IsItemDeactivated();
					prev_prefix = prefix;
				}

				if (collapse_open) {
					if (just_opened)
						selected_scenery = &desc;

					ImGui::Indent(10.0f);
					if (ImGui::Selectable(name.c_str(), &desc == selected_scenery))
						selected_scenery = &desc;
					ImGui::Unindent(10.0f);
				}
			}
		} ImGui::EndChild();

		ImGui::NextColumn();

		if (ImGui::BeginChild("child2", ImVec2(0, -100))) {
			if (selected_scenery) {
				ImGui::TextWrapped("%s", selected_scenery->name.c_str());
				ImGui::TextWrapped("%s", selected_scenery->description.c_str());

				ImGui::Separator();

				for (auto const &link : selected_scenery->links) {
					if (ImGui::Button(link.second.c_str(), ImVec2(-1, 0))) {
						std::string file = ToLower(link.first);
#ifdef _WIN32
						system(("start \"" + file + "\"").c_str());
#elif __linux__
						system(("xdg-open \"" + file + "\"").c_str());
#elif __APPLE__
						system(("open \"" + file + "\"").c_str());
#endif
					}
				}
			}
		} ImGui::EndChild();

		ImGui::NextColumn();

		if (selected_scenery) {
			if (ImGui::BeginChild("child3", ImVec2(0, -100), false, ImGuiWindowFlags_NoScrollbar)) {
				if (!selected_scenery->image_path.empty()) {
					scenery_desc *desc = const_cast<scenery_desc*>(selected_scenery);
					desc->image = GfxRenderer.Fetch_Texture(selected_scenery->image_path, true);
					desc->image_path.clear();
				}

				if (selected_scenery->image != null_handle) {
					opengl_texture &tex = GfxRenderer.Texture(selected_scenery->image);
					tex.create();

					if (tex.is_ready) {
						float avail_width = ImGui::GetContentRegionAvailWidth();
						float height = avail_width / tex.width() * tex.height();

						ImGui::Image(reinterpret_cast<void *>(tex.id), ImVec2(avail_width, height), ImVec2(0, 1), ImVec2(1, 0));
					}
				}
			} ImGui::EndChild();
		}
		ImGui::Columns();
		ImGui::Separator();

		if (ImGui::BeginChild("child4")) {
			if (selected_scenery) {
				ImGui::Columns(2);

				if (ImGui::BeginChild("child5", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar)) {
					for (auto const &trainset : selected_scenery->trainsets) {
						std::string z = trainset.name;
						for (auto const &dyn_desc : trainset.vehicles)
							z += dyn_desc.name + "+";
						ImGui::Selectable(z.c_str(), false);
					}
				} ImGui::EndChild();

				ImGui::NextColumn();
				//ImGui::Button(STR_C("Launch"));
			}
		} ImGui::EndChild();
	}

	ImGui::End();
}
