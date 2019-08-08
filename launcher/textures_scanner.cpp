#include "stdafx.h"
#include "textures_scanner.h"
#include "renderer.h"
#include "utilities.h"

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
