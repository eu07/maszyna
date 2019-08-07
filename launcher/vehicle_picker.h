#pragma once

#include "uilayer.h"

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

class deferred_image {
public:
	deferred_image() = default;
	deferred_image(const std::string &p) : path(p) { }
	deferred_image(const deferred_image&) = delete;
	deferred_image(deferred_image&&) = default;
	deferred_image &operator=(deferred_image&&) = default;
	operator bool() const;

	GLuint get() const;
	glm::ivec2 size() const;

private:
	mutable std::string path;
	mutable texture_handle image = 0;
};

struct skin_set {
	std::vector<std::filesystem::path> skins;
	deferred_image mini;
	std::string group;
};

struct vehicle_desc;
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

	std::vector<skin_set> matching_skinsets;
	std::vector<coupling_rule> coupling_rules;
	std::vector<texture_rule> texture_rules;
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
	void parse_coupling_rule(const std::string &target, const std::string &param);
	void parse_texture_rule(const std::string &target, const std::string &param);
	void parse_texture_info(const std::string &target, const std::string &param, const std::string &comment);

	std::shared_ptr<vehicle_desc> get_vehicle(const std::string &name);

	vehicle_type ctx_type = vehicle_type::unknown;
	std::filesystem::path ctx_path;
	std::string ctx_model;
};

class vehiclepicker_panel : public ui_panel
{
  public:
	vehiclepicker_panel();

	void render() override;

private:
	bool selectable_image(const char *desc, bool selected, const deferred_image *image);

	vehicle_type selected_type = vehicle_type::none;
	std::shared_ptr<const vehicle_desc> selected_vehicle;
	const std::string *selected_group = nullptr;
	const skin_set *selected_skinset = nullptr;
	bool display_by_groups = false;

	vehicles_bank bank;

	const deferred_image *lastdef = nullptr;
};
} // namespace ui
