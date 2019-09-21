#pragma once

#include "uilayer.h"
#include "scenery_scanner.h"
#include "widgets/popup.h"

namespace ui
{
class dynamic_edit_popup : public popup
{
	dynamic_desc &dynamic;

	std::array<char, 128> name_buf;
	std::array<char, 128> load_buf;
	std::array<char, 128> param_buf;

	template <size_t n>
	void prepare_str(std::string &str, std::array<char, n> &array) {
		if (str.size() > array.size() - 1)
			str.resize(array.size() - 1);

		std::copy(str.begin(), str.end(), array.data());
		array[str.size()] = 0;
	}

  public:
	dynamic_edit_popup(ui_panel &panel, dynamic_desc &dynamic);

	virtual void render_content() override;
};

class scenerylist_panel : public ui_panel
{
  public:
	scenerylist_panel(scenery_scanner &scanner);

	void render_contents() override;

private:
	struct vehicle_moved {
		trainset_desc &source;
		dynamic_desc &dynamic;
		int position;

		vehicle_moved(trainset_desc &source, dynamic_desc &dynamic, int position)
		    : source(source), dynamic(dynamic), position(position) {}
	};

	scenery_scanner &scanner;
	scenery_desc *selected_scenery = nullptr;
	trainset_desc *selected_trainset = nullptr;
	deferred_image placeholder_mini;

	void draw_scenery_list();
	void draw_scenery_info();
	void draw_scenery_image();
	void draw_launch_box();
	void draw_trainset_box();
	bool launch_simulation();
	void add_replace_entry(const trainset_desc &trainset);
	void draw_summary_tooltip(const dynamic_desc &dyn_desc);

	void open_link(const std::string &link);
	void draw_trainset(trainset_desc &trainset);
	void draw_droptarget(trainset_desc &trainset, int position);
};
} // namespace ui
