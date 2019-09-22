#pragma once

#include "deferred_image.h"

namespace ui {
enum class vehicle_type {
	none = -1,
	electric_loco,
	diesel_loco,
	steam_loco,
	railcar,
	emu,
	utility,
	draisine,
	tram,
	carriage,
	truck,
	bus,
	car,
	man,
	animal,
	unknown
};

struct vehicle_desc;

struct skin_meta {
	std::string name;
	std::string short_id;
	std::string location;

	std::string rev_date;
	int rev_year = -1;

	std::string rev_company;
	std::string texture_author;
	std::string photo_author;

	std::string search_lowered;
};

struct skin_set {
	std::weak_ptr<vehicle_desc> vehicle;

	std::shared_ptr<skin_meta> meta;
	std::string skin;
	//std::vector<std::filesystem::path> skins;
	deferred_image mini;
	std::string group;
};

struct texture_rule {
	std::shared_ptr<vehicle_desc> previous_vehicle;
	std::vector<std::pair<std::string, std::string>> replace_rules;
};

struct coupling_rule {
	std::shared_ptr<vehicle_desc> coupled_vehicle;
	int coupling_flag;
};

struct vehicle_desc {
	vehicle_type type;
	std::filesystem::path path;

	std::vector<std::shared_ptr<skin_set>> matching_skinsets;
	std::vector<coupling_rule> coupling_rules;
	std::vector<texture_rule> texture_rules;

	bool controllable;
};

class vehicles_bank {
public:
	std::unordered_map<vehicle_type, deferred_image> category_icons;
	std::map<std::string, deferred_image> group_icons;

	std::map<std::filesystem::path, std::shared_ptr<vehicle_desc>> vehicles;
	std::map<std::string, std::set<std::shared_ptr<vehicle_desc>>> group_map;
	void scan_textures();

private:
	void parse_entry(const std::string &line);
	void parse_category_entry(const std::string &param);
	void parse_controllable_entry(const std::string &target, const std::string &param);
	void parse_coupling_rule(const std::string &target, const std::string &param);
	void parse_texture_rule(const std::string &target, const std::string &param);
	void parse_texture_info(const std::string &target, const std::string &param, std::shared_ptr<skin_meta> meta);

	std::shared_ptr<skin_meta> parse_meta(const std::string &str);
	std::shared_ptr<vehicle_desc> get_vehicle(const std::string &name);

	vehicle_type ctx_type = vehicle_type::unknown;
	std::filesystem::path ctx_path;
	std::string ctx_model;
};
}
