#include "stdafx.h"
#include "textures_scanner.h"
#include "utilities.h"

void ui::vehicles_bank::scan_textures()
{
	for (auto &f : std::filesystem::recursive_directory_iterator("dynamic")) {
		if (f.path().filename() != "textures.txt")
			continue;

		std::fstream stream(f.path(), std::ios_base::binary | std::ios_base::in);
		ctx_path = std::filesystem::relative(f.path().parent_path(), "dynamic/");

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
	std::shared_ptr<skin_meta> meta;

	size_t pos = line.find("//");
	if (pos != -1)
		meta = parse_meta(line.substr(pos));
	content = line.substr(0, pos);

	std::istringstream stream(content);

	std::string target;
	std::getline(stream, target, '=');

	std::string param;
	while (std::getline(stream, param, '=')) {
		if (param.back() == ' ')
			param.pop_back();

		if (line[0] == '!')
			parse_category_entry(param);
		else if (line[0] == '*' && line[1] == '*')
			parse_texture_rule(target.substr(2), param);
		else if (line[0] == '*')
			parse_coupling_rule(target.substr(1), param);
		else if (line[0] == '@')
			parse_controllable_entry(target.substr(1), param);
		else
			parse_texture_info(target, param, meta);
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

void ui::vehicles_bank::parse_controllable_entry(const std::string &target, const std::string &param)
{
	get_vehicle(target)->controllable =
	        (param.size() >= 1 && param[0] == '1');
}

void ui::vehicles_bank::parse_texture_info(const std::string &target, const std::string &param, std::shared_ptr<skin_meta> meta)
{
	std::istringstream stream(param);

	std::string model, mini, miniplus;

	std::getline(stream, model, ',');
	std::getline(stream, mini, ',');
	std::getline(stream, miniplus, ',');

	auto vehicle = get_vehicle(model);

	skin_set set;
	set.vehicle = vehicle;
	set.group = mini;
	set.meta = meta;

	if (!mini.empty())
		group_icons.emplace(mini, std::move(deferred_image("textures/mini/" + ToLower(mini))));

	if (!miniplus.empty())
		set.mini = std::move(deferred_image("textures/mini/" + ToLower(miniplus)));
	else if (!mini.empty())
		set.mini = std::move(deferred_image("textures/mini/" + ToLower(mini)));

	set.skin = ToLower(target);
	erase_extension(set.skin);
	/*
	std::istringstream tex_stream(target);
	std::string texture;
	while (std::getline(tex_stream, texture, '|')) {
		set.skins.push_back(ctx_path / ToLower(texture));
	}
	*/

	group_map[set.group].insert(vehicle);
	vehicle->matching_skinsets.push_back(std::make_shared<skin_set>(std::move(set)));
}

std::shared_ptr<ui::skin_meta> ui::vehicles_bank::parse_meta(const std::string &str)
{
	std::istringstream stream(str);

	auto meta = std::make_shared<skin_meta>();

	std::string version;
	std::getline(stream, version, ',');
	std::getline(stream, meta->name, ',');
	std::getline(stream, meta->short_id, ',');
	std::getline(stream, meta->location, ',');
	std::getline(stream, meta->rev_date, ',');
	std::getline(stream, meta->rev_company, ',');
	std::getline(stream, meta->texture_author, ',');
	std::getline(stream, meta->photo_author, ',');

	meta->name = win1250_to_utf8(meta->name);
	meta->short_id = win1250_to_utf8(meta->short_id);
	meta->location = win1250_to_utf8(meta->location);
	meta->rev_date = win1250_to_utf8(meta->rev_date);
	meta->rev_company = win1250_to_utf8(meta->rev_company);
	meta->texture_author = win1250_to_utf8(meta->texture_author);
	meta->photo_author = win1250_to_utf8(meta->photo_author);

	meta->search_lowered +=
	        ToLower("n:" + meta->name + ":i:" + meta->short_id + ":d:" + meta->location +
	                ":r:" + meta->rev_date + ":c:" + meta->rev_company + ":t:" + meta->texture_author + ":p:" + meta->photo_author);

	std::replace(std::begin(meta->location), std::end(meta->location), '_', ' ');
	std::replace(std::begin(meta->rev_company), std::end(meta->rev_company), '_', ' ');
	std::replace(std::begin(meta->texture_author), std::end(meta->texture_author), '_', ' ');
	std::replace(std::begin(meta->photo_author), std::end(meta->photo_author), '_', ' ');

	if (!meta->rev_date.empty() && meta->rev_date != "?") {
		std::istringstream stream(meta->rev_date);
		std::string day, month;

		std::getline(stream, day, '.');
		std::getline(stream, month, '.');

		stream >> meta->rev_year;
	}

	return meta;
}

void ui::vehicles_bank::parse_coupling_rule(const std::string &target, const std::string &param)
{
	auto target_vehicle = get_vehicle(target);

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
	rule.coupled_vehicle = target_vehicle;
	rule.coupling_flag = coupling_flag;

	get_vehicle(connected)->coupling_rules.push_back(rule);
}

void ui::vehicles_bank::parse_texture_rule(const std::string &target, const std::string &param)
{
	auto target_vehicle = get_vehicle(target);

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

		target_vehicle->texture_rules.push_back(rule);
	}
}

std::shared_ptr<ui::vehicle_desc> ui::vehicles_bank::get_vehicle(const std::string &name)
{
	auto path = ctx_path / ToLower(name);
	path = std::filesystem::path(path.generic_string());

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
