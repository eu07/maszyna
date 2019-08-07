#include "stdafx.h"
#include "launcher/vehicle_picker.h"
#include "renderer.h"

GLuint ui::deferred_image::get() const
{
	if (!path.empty()) {
		image = GfxRenderer.Fetch_Texture(path, true);
		path.clear();
	}

	if (image != null_handle) {
		opengl_texture &tex = GfxRenderer.Texture(image);
		tex.create();

		if (tex.is_ready)
			return tex.id;
	}

	return -1;
}

ui::deferred_image::operator bool() const
{
	return image != null_handle || !path.empty();
}

glm::ivec2 ui::deferred_image::size() const
{
	if (image != null_handle) {
		opengl_texture &tex = GfxRenderer.Texture(image);
		return glm::ivec2(tex.width(), tex.height());
	}
	return glm::ivec2();
}

ui::vehiclepicker_panel::vehiclepicker_panel()
    : ui_panel(STR("Select vehicle"), true)
{
	bank.scan_textures();
}

void ui::vehiclepicker_panel::render()
{
	if (!is_open)
		return;

	static std::map<vehicle_type, std::string> type_names =
	{
	    { vehicle_type::electric_loco, STRN("Electric locos") },
	    { vehicle_type::diesel_loco, STRN("Diesel locos") },
	    { vehicle_type::steam_loco, STRN("Steam locos") },
	    { vehicle_type::railcar, STRN("Railcars") },
	    { vehicle_type::emu, STRN("EMU") },
	    { vehicle_type::utility, STRN("Utility") },
	    { vehicle_type::draisine, STRN("Draisines") },
	    { vehicle_type::tram, STRN("Trams") },
	    { vehicle_type::carriage, STRN("Carriages") },
	    { vehicle_type::truck, STRN("Trucks") },
	    { vehicle_type::bus, STRN("Buses") },
	    { vehicle_type::car, STRN("Cars") },
	    { vehicle_type::man, STRN("People") },
	    { vehicle_type::animal, STRN("Animals") },
	    { vehicle_type::unknown, STRN("Unknown") }
	};

	if (ImGui::Begin(m_name.c_str())) {
		ImGui::Columns(3);

		ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.0f, 0.5f));

		if (ImGui::BeginChild("box1")) {
			for (auto const &e : type_names) {
				deferred_image *image = nullptr;
				auto it = bank.category_icons.find(e.first);
				if (it != bank.category_icons.end())
					image = &it->second;

				if (selectable_image(Translations.lookup_s(e.second).c_str(), e.first == selected_type, image))
					selected_type = e.first;
			}
		}
		ImGui::EndChild();
		ImGui::NextColumn();

		ImGui::TextUnformatted("Group by: ");
		ImGui::SameLine();
		if (ImGui::RadioButton(STR_C("type"), !display_by_groups))
			display_by_groups = false;
		ImGui::SameLine();
		if (ImGui::RadioButton(STR_C("texture group"), display_by_groups))
			display_by_groups = true;

		std::vector<const skin_set*> skinset_list;

		if (display_by_groups) {
			std::vector<const std::string*> model_list;

			for (auto const &kv : bank.group_map) {
				const std::string &group = kv.first;

				bool model_added = false;
				bool can_break = false;
				bool map_sel_eq = (selected_group && group == *selected_group);

				for (auto const &vehicle : kv.second) {
					if (vehicle->type != selected_type)
						continue;

					for (auto const &skinset : vehicle->matching_skinsets) {
						bool map_group_eq = (skinset.group == group);
						bool sel_group_eq = (selected_group && skinset.group == *selected_group);

						if (!model_added && map_group_eq) {
							model_list.push_back(&group);
							model_added = true;
						}

						if (map_sel_eq) {
							if (sel_group_eq)
								skinset_list.push_back(&skinset);
						}
						else if (model_added) {
							can_break = true;
							break;
						}
					}

					if (can_break)
						break;
				}
			}

			if (ImGui::BeginChild("box2")) {
				ImGuiListClipper clipper(model_list.size());
				while (clipper.Step())
					for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
						auto group = model_list[i];

						deferred_image *image = nullptr;
						auto it = bank.group_icons.find(*group);
						if (it != bank.group_icons.end())
							image = &it->second;

						if (selectable_image(group->c_str(), group == selected_group, image))
							selected_group = group;
					}
			}
		}
		else {
			std::vector<std::shared_ptr<const vehicle_desc>> model_list;

			for (auto const &v : bank.vehicles) {
				auto desc = v.second;

				if (selected_type == desc->type && desc->matching_skinsets.size() > 0)
					model_list.push_back(desc);

				if (selected_vehicle == desc && selected_type == desc->type) {
					for (auto &skin : desc->matching_skinsets)
						skinset_list.push_back(&skin);
				}
			}

			if (ImGui::BeginChild("box2")) {
				ImGuiListClipper clipper(model_list.size());
				while (clipper.Step())
					for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
						auto &desc = model_list[i];

						const deferred_image *image = nullptr;
						for (auto const &skinset : desc->matching_skinsets) {
							if (skinset.mini) {
								image = &skinset.mini;
								break;
							}
						}

						std::string label = desc->path.stem().string();
						if (selectable_image(label.c_str(), desc == selected_vehicle, image))
							selected_vehicle = desc;
					}
			}
		}

		ImGui::EndChild();
		ImGui::NextColumn();

		if (ImGui::BeginChild("box3")) {
			ImGuiListClipper clipper(skinset_list.size());
			while (clipper.Step())
				for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
					auto skin = skinset_list[i];

					std::string label = skin->skins[0].stem().string();
					if (selectable_image(label.c_str(), skin == selected_skinset, &skin->mini))
						selected_skinset = skin;
				}
		}
		ImGui::EndChild();

		ImGui::PopStyleVar();
	}
	ImGui::End();

	if (!lastdef)
		return;

	ImGui::Begin("target", nullptr, ImGuiWindowFlags_HorizontalScrollbar);

	int originX = ImGui::GetCursorPosX();

	ImGui::SameLine(originX - 5);
	ImGui::Dummy(ImVec2(15, 30));
	if (ImGui::BeginDragDropTarget()) {
		ImGui::AcceptDragDropPayload("skin");
		ImGui::EndDragDropTarget();
	}
	ImGui::SameLine(originX + 5);

	for (int i = 0; i < 20; i++) {
		ImGui::PushID(i);

		GLuint tex = lastdef->get();
		if (tex != -1) {
			glm::ivec2 size = lastdef->size();
			float width = 30.0f / size.y * size.x;
			ImGui::Image(reinterpret_cast<void*>(tex), ImVec2(width, 30), ImVec2(0, 1), ImVec2(1, 0));
		}

		ImGui::SameLine(0, 0);

		int originX = ImGui::GetCursorPosX();

		ImGui::SameLine(originX - 5);
		ImGui::Dummy(ImVec2(15, 30));
		if (ImGui::BeginDragDropTarget()) {
			ImGui::AcceptDragDropPayload("skin");
			ImGui::EndDragDropTarget();
		}
		ImGui::SameLine(originX + 5);

		ImGui::PopID();
	}

	ImGui::End();
}

bool ui::vehiclepicker_panel::selectable_image(const char *desc, bool selected, const deferred_image* image)
{
	bool ret = ImGui::Selectable(desc, selected, 0, ImVec2(0, 30));
	ImGui::SetItemAllowOverlap();

	if (!image)
		return ret;

	GLuint tex = image->get();
	if (tex != -1) {
		glm::ivec2 size = image->size();
		float width = 30.0f / size.y * size.x;
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - width);
		ImGui::Image(reinterpret_cast<void*>(tex), ImVec2(width, 30), ImVec2(0, 1), ImVec2(1, 0));
		lastdef = image;

		ImGui::PushID((void*)image);
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - width);
		ImGui::InvisibleButton(desc, ImVec2(width, 30));

		if (ImGui::BeginDragDropSource()) {
			ImGui::Image(reinterpret_cast<void*>(tex), ImVec2(width, 30), ImVec2(0, 1), ImVec2(1, 0));

			ImGui::SetDragDropPayload("skin", "aaaa", 5);
			ImGui::EndDragDropSource();
		}

		ImGui::PopID();
	}

	return ret;
}

void ui::vehicles_bank::scan_textures()
{
	for (auto &f : std::filesystem::recursive_directory_iterator("dynamic")) {
		if (f.path().filename() != "textures.txt")
			continue;

		std::fstream stream(f.path(), std::ios_base::binary | std::ios_base::in);
		ctx_path = f.path().parent_path();

		std::string line;
		while (std::getline(stream, line))
		{
			if (line.size() < 3)
				continue;
			if (line.back() == '\r')
				line.pop_back();

			parse_entry(line);
		}
	}
}

void ui::vehicles_bank::parse_entry(const std::string &line)
{
	std::string content;
	std::string comments;

	size_t pos = line.find("//");
	if (pos != -1)
		comments = line.substr(pos);
	content = line.substr(0, pos);

	std::istringstream stream(content);

	std::string target;
	std::getline(stream, target, '=');

	std::string param;
	while (std::getline(stream, param, '=')) {
		if (line[0] == '!')
			parse_category_entry(param);
		else if (line[0] == '*' && line[1] == '*')
			parse_texture_rule(target.substr(2), param);
		else if (line[0] == '*')
			parse_coupling_rule(target.substr(1), param);
		else if (line[0] != '@')
			parse_texture_info(target, param, comments);
	}
}

void ui::vehicles_bank::parse_category_entry(const std::string &param)
{
	static std::unordered_map<char, vehicle_type> type_map = {
	    { 'e', vehicle_type::electric_loco },
	    { 's', vehicle_type::diesel_loco },
	    { 'p', vehicle_type::steam_loco },
	    { 'a', vehicle_type::railcar },
	    { 'z', vehicle_type::emu },
	    { 'r', vehicle_type::utility },
	    { 'd', vehicle_type::draisine },
	    { 't', vehicle_type::tram },
	    { 'c', vehicle_type::truck },
	    { 'b', vehicle_type::bus },
	    { 'o', vehicle_type::car },
	    { 'h', vehicle_type::man },
	    { 'f', vehicle_type::animal },
	};

	ctx_type = vehicle_type::unknown;

	std::istringstream stream(param);

	std::string tok;
	std::getline(stream, tok, ',');

	if (tok.size() < 1)
		return;

	auto it = type_map.find(tok[0]);
	if (it != type_map.end())
		ctx_type = it->second;
	else if (tok[0] >= 'A' && tok[0] <= 'Z')
		ctx_type = vehicle_type::carriage;

	std::string mini;
	std::getline(stream, mini, ',');

	category_icons.emplace(ctx_type, "textures/mini/" + ToLower(mini));
}

void ui::vehicles_bank::parse_texture_info(const std::string &target, const std::string &param, const std::string &comment)
{
	std::istringstream stream(param);

	std::string model, mini, miniplus;

	std::getline(stream, model, ',');
	std::getline(stream, mini, ',');
	std::getline(stream, miniplus, ',');

	skin_set set;
	set.group = mini;

	if (!mini.empty())
		group_icons.emplace(mini, std::move(deferred_image("textures/mini/" + ToLower(mini))));

	if (!miniplus.empty())
		set.mini = std::move(deferred_image("textures/mini/" + ToLower(miniplus)));

	std::istringstream tex_stream(target);
	std::string texture;
	while (std::getline(tex_stream, texture, '|')) {
		auto path = ctx_path;
		path.append(texture);
		set.skins.push_back(path);
	}

	auto vehicle = get_vehicle(model);
	group_map[set.group].insert(vehicle);
	vehicle->matching_skinsets.push_back(std::move(set));
}

void ui::vehicles_bank::parse_coupling_rule(const std::string &target, const std::string &param)
{
	if (param == "?")
		return;

	std::istringstream stream(param);

	int coupling_flag;
	stream >> coupling_flag;
	stream.ignore(1000, ',');

	std::string connected;
	std::getline(stream, connected, ',');

	std::string param1, param2;
	std::getline(stream, param1, ',');
	std::getline(stream, param2, ',');

	coupling_rule rule;
	rule.coupled_vehicle = get_vehicle(target);
	rule.coupling_flag = coupling_flag;

	get_vehicle(connected)->coupling_rules.push_back(rule);
}

void ui::vehicles_bank::parse_texture_rule(const std::string &target, const std::string &param)
{
	std::istringstream stream(param);

	std::string prev;
	std::getline(stream, prev, ',');

	std::string replace_rule;
	while (std::getline(stream, replace_rule, ',')) {
		if (replace_rule == "-")
			break;

		std::istringstream rule_stream(replace_rule);

		std::string src, dst;
		std::getline(rule_stream, src, '-');
		std::getline(rule_stream, dst, '-');

		texture_rule rule;
		rule.previous_vehicle = get_vehicle(prev);
		rule.replace_rules.emplace_back(src, dst);

		get_vehicle(target)->texture_rules.push_back(rule);
	}
}

std::shared_ptr<ui::vehicle_desc> ui::vehicles_bank::get_vehicle(const std::string &name)
{
	auto path = ctx_path;
	path.append(name);
	auto it = vehicles.find(path);
	if (it != vehicles.end()) {
		return it->second;
	}
	else {
		auto desc = std::make_shared<vehicle_desc>();
		desc->type = ctx_type;
		desc->path = path;
		vehicles.emplace(path, desc);
		return desc;
	}
}
