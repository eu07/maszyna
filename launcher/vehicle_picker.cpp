#include "stdafx.h"
#include "launcher/vehicle_picker.h"
#include "renderer.h"

ui::vehiclepicker_panel::vehiclepicker_panel()
    : ui_panel(STR("Select vehicle"), false), placeholder_mini("textures/mini/other")
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

	if (ImGui::Begin(m_name.c_str(), &is_open)) {
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

		ImGui::TextUnformatted(STR_C("Group by: "));
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
						bool map_group_eq = (skinset->group == group);
						bool sel_group_eq = (selected_group && skinset->group == *selected_group);

						if (!model_added && map_group_eq) {
							model_list.push_back(&group);
							model_added = true;
						}

						if (map_sel_eq) {
							if (sel_group_eq)
								skinset_list.push_back(skinset.get());
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
						skinset_list.push_back(skin.get());
				}
			}

			if (ImGui::BeginChild("box2")) {
				ImGuiListClipper clipper(model_list.size());
				while (clipper.Step())
					for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
						auto &desc = model_list[i];

						const deferred_image *image = nullptr;
						for (auto const &skinset : desc->matching_skinsets) {
							if (skinset->mini) {
								image = &skinset->mini;
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

		ImGui::SetNextItemWidth(-1);
		ImGui::InputText("##search", search_query.data(), search_query.size());
		auto query_info = parse_search_query(search_query.data());
		if (!query_info.empty())
			skinset_list.erase(std::remove_if(skinset_list.begin(), skinset_list.end(),
			                                  std::bind(&vehiclepicker_panel::skin_filter, this, std::placeholders::_1, query_info)),
			                   skinset_list.end());

		if (ImGui::BeginChild("box3")) {
			ImGuiListClipper clipper(skinset_list.size());
			while (clipper.Step())
				for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
					auto skin = skinset_list[i];

					//std::string label = skin->skins[0].stem().string();
					std::string label = skin->skin;
					if (skin->meta && !skin->meta->name.empty() && skin->meta->name != "?")
						label = skin->meta->name;

					auto mini = skin->mini ? &skin->mini : &placeholder_mini;
					if (selectable_image(label.c_str(), skin == selected_skinset, mini, skin))
						selected_skinset = skin;

					if (skin->meta && ImGui::IsItemHovered()) {
						ImGui::BeginTooltip();
						ImGui::Text(STR_C("Skin: %s\nType: %s\nCarrier code: %s\nDepot: %s\nMaintenance: %s, by %s\nSkin author: %s\nPhoto author: %s"),
						            skin->skin.c_str(),
						            skin->vehicle.lock()->path.string().c_str(),
						            skin->meta->short_id.c_str(),
						            skin->meta->location.c_str(),
						            skin->meta->rev_date.c_str(),
						            skin->meta->rev_company.c_str(),
						            skin->meta->texture_author.c_str(),
						            skin->meta->photo_author.c_str());
						ImGui::EndTooltip();
					}
				}
		}
		ImGui::EndChild();

		ImGui::PopStyleVar();
	}
	ImGui::End();
}

std::vector<ui::vehiclepicker_panel::search_info> ui::vehiclepicker_panel::parse_search_query(const std::string &str)
{
	std::vector<search_info> info_list;
	std::istringstream query_stream(ToLower(str));

	std::string term;
	while (std::getline(query_stream, term, ' ')) {
		size_t offset = 0;
		search_info info;
		if (offset < term.size() && term[offset] == '|') {
			info.alternative = true;
			offset++;
		}
		if (offset < term.size() && term[offset] == '!') {
			info.negation = true;
			offset++;
		}
		if (offset < term.size() && term[offset] == '<') {
			info.mode = search_info::YEAR_MAX;
			offset++;
		}
		if (offset < term.size() && term[offset] == '>') {
			info.mode = search_info::YEAR_MIN;
			offset++;
		}

		term = term.substr(offset);
		if (term.empty())
			continue;

		if (info.mode == search_info::TEXT) {
			info.text = term;
		}
		else {
			std::istringstream num_parser(term);
			num_parser >> info.number;
		}

		info_list.push_back(info);
	}

	return info_list;
}

bool ui::vehiclepicker_panel::skin_filter(const skin_set *skin, std::vector<search_info> &info_list)
{
	bool any = false;
	bool alternative_present = false;

	for (auto const &term : info_list) {
		alternative_present |= term.alternative;
		bool found = false;

		if (term.mode == search_info::TEXT) {
			if (skin->skin.find(term.text) != -1)
				found = true;
			else if (skin->meta && skin->meta->search_lowered.find(term.text) != -1)
				found = true;
		} else if (skin->meta && skin->meta->rev_year != -1) {
			if (term.mode == search_info::YEAR_MIN) {
				if (skin->meta->rev_year >= term.number)
					found = true;
			} else if (term.mode == search_info::YEAR_MAX) {
				if (skin->meta->rev_year <= term.number)
					found = true;
			}
		}

		if (term.negation)
			found = !found;

		if (found && term.alternative)
			any = true;

		if (!found && !term.alternative)
			return true;
	}

	return alternative_present ? (!any) : false;
}

bool ui::vehiclepicker_panel::selectable_image(const char *desc, bool selected, const deferred_image* image, const skin_set *pickable)
{
	ImGui::PushID(pickable);

	bool ret = ImGui::Selectable(desc, selected, 0, ImVec2(0, 30));
	if (pickable)
		ImGui::SetItemAllowOverlap();

	if (!image) {
		ImGui::PopID();
		return ret;
	}

	GLuint tex = image->get();
	if (tex != -1) {
		glm::ivec2 size = image->size();
		float width = 30.0f / size.y * size.x;
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - width);
		ImGui::Image(reinterpret_cast<void*>(tex), ImVec2(width, 30), ImVec2(0, 1), ImVec2(1, 0));

		if (pickable) {
			ImGui::PushID((void*)image);
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - width);
			ImGui::InvisibleButton(desc, ImVec2(width, 30));

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAutoExpirePayload)) {
				ImGui::Image(reinterpret_cast<void*>(tex), ImVec2(width, 30), ImVec2(0, 1), ImVec2(1, 0));

				ImGui::SetDragDropPayload("vehicle_pure", &pickable, sizeof(skin_set*));
				ImGui::EndDragDropSource();
			}

			ImGui::PopID();
		}
	}

	ImGui::PopID();
	return ret;
}
