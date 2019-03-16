#pragma once

#include "simulation.h"
#include "Event.h"
#include "scene.h"

namespace map
{

struct map_object
{
	std::string name;
	glm::vec3 location;

	virtual ~map_object() = default;
};

using object_list = std::vector<std::shared_ptr<map::map_object>>;
using sorted_object_list = std::map<float, std::shared_ptr<map::map_object>>;

// semaphore description (only for minimap purposes)
struct semaphore : public map_object
{
	std::vector<TAnimModel *> models;

	std::vector<basic_event *> events;
};

// switch description (only for minimap purposes)
struct track_switch : public map_object
{
	basic_event *straight_event = nullptr;
	basic_event *divert_event = nullptr;
};

// training obstacle description
struct obstacle : public map_object
{
	TAnimModel *model;
};

struct objects
{
	std::vector<std::shared_ptr<map_object>> entries;

	// returns objects in range from vec3, NaN in Y ignores it
	sorted_object_list find_in_range(glm::vec3 from, float distance);
};

extern objects Objects;
} // namespace map
