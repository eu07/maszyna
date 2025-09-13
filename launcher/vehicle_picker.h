#pragma once

#include "uilayer.h"
#include "textures_scanner.h"
#include "translation.h"

namespace ui {

class vehiclepicker_panel : public ui_panel
{
  public:
	vehiclepicker_panel();

	void render_contents() override;

private:
	bool selectable_image(const char *desc, bool selected, const deferred_image *image, const skin_set *pickable = nullptr);

	vehicle_type selected_type = vehicle_type::none;
	std::shared_ptr<const vehicle_desc> selected_vehicle;
	const std::string *selected_group = nullptr;
	const skin_set *selected_skinset = nullptr;
	bool display_by_groups = true;
	deferred_image placeholder_mini;
	std::array<char, 128> search_query = { 0 };
	std::map<vehicle_type, std::string> type_names =
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

	vehicles_bank bank;

	struct search_info
	{
		enum
		{
			TEXT,
			YEAR_MIN,
			YEAR_MAX
		} mode = TEXT;
		bool alternative = false;
		bool negation = false;

		std::string text;
		int number;
	};

	std::vector<search_info> parse_search_query(const std::string &str);
	bool skin_filter(const skin_set *skin, std::vector<search_info> &info_list);
};
} // namespace ui
