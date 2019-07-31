#pragma once

#include "uilayer.h"
#include <filesystem>

struct trainset_desc {
	std::string description;

	std::string name;
	std::string track;

	float offset { 0.f };
	float velocity { 0.f };

	std::vector<std::string> vehicles;
	std::vector<int> couplings;
};

struct scenery_desc {
	std::filesystem::path path;
	std::string name;
	std::string description;
	std::string image_path;
	texture_handle image = 0;

	std::vector<std::pair<std::string, std::string>> links;
	std::vector<trainset_desc> trainsets;

	std::string title() {
		if (!name.empty())
			return name;
		else
			return path.stem().string();
	}
};

namespace ui
{
class scenerylist_panel : public ui_panel
{
  public:
	scenerylist_panel();

	void render() override;

private:
	std::vector<scenery_desc> scenarios;
	scenery_desc const *selected_scenery = nullptr;

	void scan_scenarios();
	void scan_scn(std::string path);
};
} // namespace ui
