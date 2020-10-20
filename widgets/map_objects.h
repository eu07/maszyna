#pragma once

#include "simulation.h"
#include "Event.h"
#include "scene.h"
#include "MemCell.h"

namespace map
{

struct map_object
{
	std::string name;
	glm::vec3 location;

	virtual ~map_object() = default;
	virtual gfx::basic_vertex vertex() {
		return gfx::basic_vertex(location, glm::vec3(), glm::vec2(0.0f, 0.20f));
	}
};

using object_list = std::vector<std::shared_ptr<map::map_object>>;
using sorted_object_list = std::map<float, std::shared_ptr<map::map_object>>;

// semaphore description (only for minimap purposes)
struct semaphore : public map_object
{
	std::vector<TAnimModel *> models;
	std::vector<basic_event *> events;
	TMemCell *memcell = nullptr;

	virtual gfx::basic_vertex vertex() override {
		bool stop_signal = memcell->IsVelocity() && (memcell->Value1() == 0.0);
		return gfx::basic_vertex(location, glm::vec3(),
		    (!stop_signal) ? glm::vec2(0.0f, 0.2f) : glm::vec2(0.2f, 0.4f));
	}
};

// event launcher description (only for minimap purposes)
struct launcher : public map_object
{
	basic_event *first_event = nullptr;
	basic_event *second_event = nullptr;

	enum type_e {
		track_switch,
		level_crossing
	} type;

	virtual gfx::basic_vertex vertex() {
		return gfx::basic_vertex(location, glm::vec3(),
		    type == track_switch ? glm::vec2(0.4f, 0.6f) : glm::vec2(0.6f, 0.8f));
	}
};

// switch description (only for minimap purposes)
struct track_switch : public map_object
{
    std::array<basic_event*, 4> action = { nullptr };
    std::array<char[4], 4> preview;
    std::array<TTrack*, 4> track = { nullptr };

    virtual gfx::basic_vertex vertex() {
        return gfx::basic_vertex(location, glm::vec3(), glm::vec2(0.4f, 0.6f));
    }
};

// training obstacle description
struct obstacle : public map_object
{
	std::string model_name;

	virtual gfx::basic_vertex vertex() {
		return gfx::basic_vertex(location, glm::vec3(), glm::vec2(0.8f, 1.0f));
	}
};

// vehicle wrapper for map display
struct vehicle : public map_object
{
	TDynamicObject *dynobj = nullptr;
};

struct objects
{
	std::vector<std::shared_ptr<map_object>> entries;
	bool poi_dirty = true;

	// returns objects in range from vec3, NaN in Y ignores it
	sorted_object_list find_in_range(glm::vec3 from, float distance);
};

extern objects Objects;
} // namespace map
