#include "stdafx.h"
#include "scenery_list.h"
#include "imgui/imgui.h"
#include "utilities.h"
#include "renderer.h"
#include "McZapkie/MOVER.h"
#include <filesystem>

ui::scenerylist_panel::scenerylist_panel(scenery_scanner &scanner)
    : ui_panel(STR("Scenario list"), false), scanner(scanner)
{
}

void ui::scenerylist_panel::render()
{
	if (!is_open)
		return;

	auto const panelname{(title.empty() ? m_name : title) + "###" + m_name};

	if (ImGui::Begin(panelname.c_str(), &is_open)) {
		ImGui::Columns(3);

		if (ImGui::BeginChild("child1", ImVec2(0, -200))) {
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
					if (ImGui::Selectable(name.c_str(), &desc == selected_scenery)) {
						selected_scenery = &desc;
						selected_trainset = nullptr;
					}
					ImGui::Unindent(10.0f);
				}
			}
		} ImGui::EndChild();

		ImGui::NextColumn();

		if (ImGui::BeginChild("child2", ImVec2(0, -200))) {
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
			if (ImGui::BeginChild("child3", ImVec2(0, -200), false, ImGuiWindowFlags_NoScrollbar)) {
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
				if (selected_trainset)
					ImGui::Columns(2);

				if (ImGui::BeginChild("child5", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar)) {
					ImGuiListClipper clipper(selected_scenery->trainsets.size());
					while (clipper.Step()) for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
						auto const &trainset = selected_scenery->trainsets[i];
						ImGui::PushID(i);
						if (ImGui::Selectable("##set", selected_trainset == &trainset, 0, ImVec2(0, 30)))
							selected_trainset = &trainset;
						ImGui::SameLine();
						draw_trainset(trainset);
						ImGui::NewLine();
						ImGui::PopID();
						//ImGui::Selectable(z.c_str(), false);
					}
				} ImGui::EndChild();

				if (selected_trainset) {
					ImGui::NextColumn();
					ImGui::TextWrapped(selected_trainset->description.c_str());
					ImGui::Button(STR_C("Launch"), ImVec2(-1, 0));
				}
			}
		} ImGui::EndChild();
	}

	ImGui::End();
}

void ui::scenerylist_panel::draw_trainset(const trainset_desc &trainset)
{
	static std::unordered_map<coupling, std::string> coupling_names =
	{
	    { coupling::faux, STRN("faux") },
	    { coupling::coupler, STRN("coupler") },
	    { coupling::brakehose, STRN("brake hose") },
	    { coupling::control, STRN("control") },
	    { coupling::highvoltage, STRN("high voltage") },
	    { coupling::gangway, STRN("gangway") },
	    { coupling::mainhose, STRN("main hose") },
	    { coupling::heating, STRN("heating") },
	    { coupling::permanent, STRN("permanent") },
	    { coupling::uic, STRN("uic") }
	};

	draw_droptarget();
	ImGui::SameLine(15.0f);

	for (auto const &dyn_desc : trainset.vehicles) {
		if (dyn_desc.skin && dyn_desc.skin->mini.get() != -1) {
			ImGui::PushID(static_cast<const void*>(&dyn_desc));

			glm::ivec2 size = dyn_desc.skin->mini.size();
			float width = 30.0f / size.y * size.x;
			ImGui::Image(reinterpret_cast<void*>(dyn_desc.skin->mini.get()), ImVec2(width, 30), ImVec2(0, 1), ImVec2(1, 0));

			if (ImGui::IsItemHovered()) {
				ImGui::BeginTooltip();
				std::string name = (dyn_desc.vehicle->path.parent_path() / dyn_desc.vehicle->path.stem()).string();
				std::string skin = dyn_desc.skin->skin;
				ImGui::Text(STR_C("ID: %s"), dyn_desc.name.c_str());
				ImGui::NewLine();

				ImGui::Text(STR_C("Type: %s"), name.c_str());
				ImGui::Text(STR_C("Skin: %s"), skin.c_str());
				ImGui::NewLine();

				if (dyn_desc.drivertype != "nobody")
					ImGui::Text(STR_C("Occupied: %s"), dyn_desc.drivertype.c_str());
				if (dyn_desc.loadcount > 0)
					ImGui::Text(STR_C("Load: %s: %d"), dyn_desc.loadtype.c_str(), dyn_desc.loadcount);
				if (!dyn_desc.params.empty())
					ImGui::Text(STR_C("Parameters: %s"), dyn_desc.params.c_str());
				ImGui::NewLine();

				ImGui::TextUnformatted(STR_C("Coupling:"));

				for (int i = 1; i <= 0x100; i <<= 1) {
					bool dummy = true;

					if (dyn_desc.coupling & i) {
						std::string label = STRN("unknown");
						auto it = coupling_names.find(static_cast<coupling>(i));
						if (it != coupling_names.end())
							label = it->second;
						ImGui::Checkbox(Translations.lookup_c(label.c_str()), &dummy);
					}
				}

				ImGui::EndTooltip();
			}

			ImGui::SameLine(0, 0);
			float originX = ImGui::GetCursorPosX();

			ImGui::SameLine(originX - 7.5f);
			draw_droptarget();
			ImGui::SameLine(originX);

			ImGui::PopID();
		}
	}
}

void ui::scenerylist_panel::draw_droptarget()
{
	ImGui::Dummy(ImVec2(15, 30));
	if (ImGui::BeginDragDropTarget()) {
		ImGui::AcceptDragDropPayload("skin");
		ImGui::EndDragDropTarget();
	}
}
