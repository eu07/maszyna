#include "stdafx.h"
#include "scenery_list.h"
#include "imgui/imgui.h"
#include "utilities.h"
#include "renderer.h"
#include "McZapkie/MOVER.h"
#include "application.h"
#include "Logs.h"
#include <filesystem>

ui::scenerylist_panel::scenerylist_panel(scenery_scanner &scanner)
    : ui_panel(STR("Scenario list"), false), scanner(scanner), placeholder_mini("textures/mini/other")
{
}

void ui::scenerylist_panel::draw_scenery_list()
{
	std::string prev_prefix;
	bool collapse_open = false;

	for (auto &desc : scanner.scenarios) {
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
}

void ui::scenerylist_panel::draw_scenery_info()
{
	if (selected_scenery) {
		ImGui::TextWrapped("%s", selected_scenery->name.c_str());
		ImGui::TextWrapped("%s", selected_scenery->description.c_str());

		ImGui::Separator();

		for (auto const &link : selected_scenery->links) {
			if (ImGui::Button(link.second.c_str(), ImVec2(-1, 0)))
				open_link(link.first);
		}
	}
}

void ui::scenerylist_panel::open_link(const std::string &link)
{
	std::string file = ToLower(link);
#ifdef _WIN32
	system(("start \"eu07_link\" \"" + file + "\"").c_str());
#elif __linux__
	system(("xdg-open \"" + file + "\"").c_str());
#elif __APPLE__
	system(("open \"" + file + "\"").c_str());
#endif
}

void ui::scenerylist_panel::draw_scenery_image()
{
	if (!selected_scenery->image_path.empty()) {
		scenery_desc *desc = const_cast<scenery_desc*>(selected_scenery);
        desc->image = GfxRenderer->Fetch_Texture(selected_scenery->image_path, true);
		desc->image_path.clear();
	}

	if (selected_scenery->image != null_handle) {
        opengl_texture &tex = GfxRenderer->Texture(selected_scenery->image);
		tex.create();

		if (tex.is_ready) {
			float avail_width = ImGui::GetContentRegionAvailWidth();
			float height = avail_width / tex.width() * tex.height();

			ImGui::Image(reinterpret_cast<void *>(tex.id), ImVec2(avail_width, height), ImVec2(0, 1), ImVec2(1, 0));
		}
	}
}

void ui::scenerylist_panel::draw_launch_box()
{
	ImGui::NextColumn();

	ImGui::TextWrapped(selected_trainset->description.c_str());

	if (ImGui::Button(STR_C("Launch"), ImVec2(-1, 0))) {
		if (!launch_simulation())
			ImGui::OpenPopup("missing_driver");
	}

	if (ImGui::BeginPopup("missing_driver")) {
		ImGui::TextUnformatted(STR_C("Trainset not occupied"));
		if (ImGui::Button(STR_C("OK"), ImVec2(-1, 0)))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
}

bool ui::scenerylist_panel::launch_simulation()
{
	bool found = false;
	for (auto &veh : selected_trainset->vehicles) {
		if (veh.drivertype.size() > 0 && veh.drivertype != "nobody") {
			Global.local_start_vehicle = ToLower(veh.name);
			Global.SceneryFile = selected_scenery->path.generic_string();

			Application.pop_mode();
			Application.push_mode(eu07_application::mode::scenarioloader);

			found = true;
			break;
		}
	}

	if (!found)
		return false;

	for (auto &desc : selected_scenery->trainsets)
		add_replace_entry(desc);

	return true;
}

void ui::scenerylist_panel::add_replace_entry(const trainset_desc &trainset)
{
	std::string set = "trainset ";
	set += trainset.name + " ";
	set += trainset.track + " ";
	set += std::to_string(trainset.offset) + " ";
	set += std::to_string(trainset.velocity) + "\n";
	for (const auto &veh : trainset.vehicles) {
		if (!veh.skin) {
			ErrorLog("trainset contains invalid vehicle " + veh.name);
			continue;
		}

		set += "node -1 0 " + veh.name + " dynamic ";
		set += veh.vehicle->path.parent_path().generic_string() + " ";
		set += veh.skin->skin + " ";
		set += veh.vehicle->path.stem().generic_string() + " ";
		set += std::to_string(veh.offset) + " " + veh.drivertype + " ";
		set += std::to_string(veh.coupling);
		if (veh.params.size() > 0)
			set += "." + veh.params;
		set += " " + std::to_string(veh.loadcount) + " ";
		if (veh.loadcount > 0)
			set += veh.loadtype + " ";
		set += "enddynamic\n";
	}
	set += "endtrainset\n";

	Global.trainset_overrides.emplace(trainset.file_bounds.first, set);
}

void ui::scenerylist_panel::draw_trainset_box()
{
	ImGuiListClipper clipper(selected_scenery->trainsets.size());
	while (clipper.Step()) for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
		draw_trainset(selected_scenery->trainsets[i]);
}

void ui::scenerylist_panel::render_contents()
{
	ImGui::Columns(3);

	if (ImGui::BeginChild("child1", ImVec2(0, -200)))
		draw_scenery_list();
	ImGui::EndChild();

	ImGui::NextColumn();

	if (ImGui::BeginChild("child2", ImVec2(0, -200)))
		draw_scenery_info();
	ImGui::EndChild();

	ImGui::NextColumn();

	if (selected_scenery) {
		if (ImGui::BeginChild("child3", ImVec2(0, -200), false, ImGuiWindowFlags_NoScrollbar))
			draw_scenery_image();
		ImGui::EndChild();
	}
	ImGui::Columns();
	ImGui::Separator();

	if (ImGui::BeginChild("child4")) {
		if (selected_scenery) {
			if (selected_trainset)
				ImGui::Columns(2);

			if (ImGui::BeginChild("child5", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
				draw_trainset_box();
			ImGui::EndChild();

			if (selected_trainset)
				draw_launch_box();
		}
	} ImGui::EndChild();
}

void ui::scenerylist_panel::draw_summary_tooltip(const dynamic_desc &dyn_desc)
{
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
		if (dyn_desc.coupling & i)
            ImGui::Text("+ %s", Translations.coupling_name(i).c_str());
	}
}

void ui::scenerylist_panel::draw_trainset(trainset_desc &trainset)
{
	ImGui::PushID(&trainset);
	if (ImGui::Selectable("##set", selected_trainset == &trainset, 0, ImVec2(0, 30)))
		selected_trainset = &trainset;
	ImGui::SameLine();

	ImGui::SetItemAllowOverlap();

	int position = 0;

	draw_droptarget(trainset, position++);
	ImGui::SameLine(15.0f);

	for (auto &dyn_desc : trainset.vehicles) {
		deferred_image *mini = nullptr;

		if (dyn_desc.skin && dyn_desc.skin->mini.get() != -1)
			mini = &dyn_desc.skin->mini;
		else
			mini = &placeholder_mini;

		ImGui::PushID(static_cast<const void*>(&dyn_desc));

		glm::ivec2 size = mini->size();
		float width = 30.0f / size.y * size.x;
		float beforeX = ImGui::GetCursorPosX();
		ImGui::Image(reinterpret_cast<void*>(mini->get()), ImVec2(width, 30), ImVec2(0, 1), ImVec2(1, 0));
		float afterX = ImGui::GetCursorPosX();

		ImGui::SameLine(beforeX);
		ImGui::InvisibleButton(dyn_desc.name.c_str(), ImVec2(width, 30));
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAutoExpirePayload)) {
			vehicle_moved data(trainset, dyn_desc, position - 1);

			ImGui::Image(reinterpret_cast<void*>(mini->get()), ImVec2(width, 30), ImVec2(0, 1), ImVec2(1, 0));

			ImGui::SetDragDropPayload("vehicle_set", &data, sizeof(vehicle_moved));
			ImGui::EndDragDropSource();
		}
		ImGui::SameLine(afterX);

		if (ImGui::IsItemDeactivated() && ImGui::IsItemHovered()) {
			selected_trainset = &trainset;
		}
		else if (ImGui::IsItemClicked(1)) {
			register_popup(std::make_unique<ui::dynamic_edit_popup>(*this, dyn_desc));
		}
		else if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			draw_summary_tooltip(dyn_desc);
			ImGui::EndTooltip();
		}

		ImGui::SameLine(0, 0);
		float originX = ImGui::GetCursorPosX();

		ImGui::SameLine(originX - 7.5f);
		draw_droptarget(trainset, position++);
		ImGui::SameLine(originX);

		ImGui::PopID();
	}

	ImGui::NewLine();
	ImGui::PopID();
}

void ui::scenerylist_panel::draw_droptarget(trainset_desc &trainset, int position)
{
	ImGui::Dummy(ImVec2(15, 30));
	if (ImGui::BeginDragDropTarget()) {
		const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("vehicle_pure");
		if (payload) {
			skin_set *ptr = *(reinterpret_cast<skin_set**>(payload->Data));
			std::shared_ptr<skin_set> skin;
			for (auto &s : ptr->vehicle.lock()->matching_skinsets)
				if (s.get() == ptr)
					skin = s;

			trainset.vehicles.emplace(trainset.vehicles.begin() + position);
			dynamic_desc &desc = trainset.vehicles[position];

			desc.name = skin->skin + "_" + std::to_string((int)LocalRandom(0.0, 10000000.0));
			desc.vehicle = skin->vehicle.lock();
			desc.skin = skin;
		}

		payload = ImGui::AcceptDragDropPayload("vehicle_set");
		if (payload) {
			vehicle_moved *moved = reinterpret_cast<vehicle_moved*>(payload->Data);

			dynamic_desc desc_copy = moved->dynamic;
			int offset = 0;
			if (&trainset == &moved->source) {
				trainset.vehicles.erase(trainset.vehicles.begin() + moved->position);
				if (moved->position < position)
					offset = -1;
			} else {
				desc_copy.name = desc_copy.skin->skin + "_" + std::to_string((int)LocalRandom(0.0, 10000000.0));
			}

			trainset.vehicles.insert(trainset.vehicles.begin() + position + offset, desc_copy);
		}
		ImGui::EndDragDropTarget();
	}
}

ui::dynamic_edit_popup::dynamic_edit_popup(ui_panel &panel, dynamic_desc &dynamic)
    : popup(panel), dynamic(dynamic)
{
	prepare_str(dynamic.name, name_buf);
	prepare_str(dynamic.loadtype, load_buf);
	prepare_str(dynamic.params, param_buf);
}

void ui::dynamic_edit_popup::render_content()
{
	static std::vector<std::string> occupancy_names = {
	    STRN("headdriver"),
	    STRN("reardriver"),
	    STRN("passenger"),
	    STRN("nobody")
	};

	std::string name = (dynamic.vehicle->path.parent_path() / dynamic.vehicle->path.stem()).string();

	ImGui::Text(STR_C("Type: %s"), name.c_str());
	ImGui::NewLine();

	if (ImGui::InputText(STR_C("Name"), name_buf.data(), name_buf.size()))
		dynamic.name = name_buf.data();

	if (ImGui::BeginCombo(STR_C("Skin"), dynamic.skin->skin.c_str(), ImGuiComboFlags_HeightLargest))
	{
		for (auto const &skin : dynamic.vehicle->matching_skinsets) {
			bool is_selected = (skin == dynamic.skin);
			if (ImGui::Selectable(skin->skin.c_str(), is_selected))
				dynamic.skin = skin;
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	if (ImGui::BeginCombo(STR_C("Occupancy"), Translations.lookup_c(dynamic.drivertype.c_str())))
	{
		for (auto const &str : occupancy_names) {
			bool is_selected = (str == dynamic.drivertype);
			if (ImGui::Selectable(Translations.lookup_c(str.c_str()), is_selected))
				dynamic.drivertype = str;
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	if (ImGui::InputText(STR_C("Load type"), load_buf.data(), load_buf.size())) {
		dynamic.loadtype = load_buf.data();
		if (!load_buf[0])
			dynamic.loadtype = "none";
	}

	if (dynamic.loadtype != "none")
		ImGui::InputInt(STR_C("Load count"), &dynamic.loadcount, 1, 10);
	else
		dynamic.loadcount = 0;

	if (dynamic.loadcount < 0)
		dynamic.loadcount = 0;

	if (ImGui::InputText(STR_C("Parameters"), param_buf.data(), param_buf.size()))
		dynamic.params = param_buf.data();

	ImGui::NewLine();
	ImGui::TextUnformatted(STR_C("Coupling:"));

	for (int i = 1; i <= 0x100; i <<= 1) {
		bool selected = dynamic.coupling & i;

		if (ImGui::Checkbox(Translations.coupling_name(i).c_str(), &selected))
			dynamic.coupling ^= i;
	}
}
