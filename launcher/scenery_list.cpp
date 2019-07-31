#include "stdafx.h"
#include "scenery_list.h"
#include "imgui/imgui.h"
#include "utilities.h"
#include "renderer.h"
#include <filesystem>

ui::scenerylist_panel::scenerylist_panel()
    : ui_panel(STR("Scenario list"), true)
{
	scan_scenarios();

	std::sort(scenarios.begin(), scenarios.end(),
	          [](const scenery_desc &a, const scenery_desc &b) { return a.path < b.path; });
}

void ui::scenerylist_panel::render()
{
	if (!is_open)
		return;

	auto const panelname{(title.empty() ? m_name : title) + "###" + m_name};

	if (ImGui::Begin(panelname.c_str(), &is_open)) {
		ImGui::Columns(3);

		if (ImGui::BeginChild("child1")) {
			std::string prev_prefix;
			bool collapse_open = false;

			for (auto const &desc : scenarios) {
				std::string name = desc.path.stem();
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

		if (ImGui::BeginChild("child2")) {
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
			if (ImGui::BeginChild("child3", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar)) {
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

			if (ImGui::Begin(STR_C("Select trainset"))) {
				for (auto const &trainset : selected_scenery->trainsets) {
					ImGui::Selectable("Trainset", false);
				}
			}
			ImGui::End();
		}
	}

	ImGui::End();
}

void ui::scenerylist_panel::scan_scenarios()
{
	for (auto &f : std::filesystem::directory_iterator("scenery")) {
		std::filesystem::path path(f.path());

		if (*(path.filename().string().begin()) == '$')
			continue;

		if (string_ends_with(path, ".scn"))
			scan_scn(path);
	}
}

void ui::scenerylist_panel::scan_scn(std::string path)
{
	scenarios.emplace_back();
	scenery_desc &desc = scenarios.back();
	desc.path = path;

	std::ifstream stream(path, std::ios_base::binary | std::ios_base::in);

	std::string line;
	while (std::getline(stream, line))
	{
		if (line.size() < 6 || !string_starts_with(line, "//$"))
			continue;

		if (line[3] == 'i')
			desc.image_path = "scenery/images/" + line.substr(5);
		else if (line[3] == 'n')
			desc.name = line.substr(5);
		else if (line[3] == 'd')
			desc.description += line.substr(5) + '\n';
		else if (line[3] == 'f') {
			std::string lang;
			std::string file;
			std::string label;

			std::istringstream stream(line.substr(5));
			stream >> lang >> file;
			std::getline(stream, label);

			desc.links.push_back(std::make_pair(file, label));
		}
		else if (line[3] == 'o') {
			desc.trainsets.emplace_back();
			trainset_desc &trainset = desc.trainsets.back();
			trainset.name = line.substr(5);
		}
	}
}
