#include "stdafx.h"
#include "widgets/map_objects.h"
#include "simulation.h"
#include "DynObj.h"

map::objects map::Objects;

map::sorted_object_list map::objects::find_in_range(glm::vec3 from, float distance)
{
	sorted_object_list items;

	float max_distance2 = distance * distance;

	for (auto const entry : entries)
	{
		glm::vec3 entry_location = entry->location;
		glm::vec3 search_point = from;

		if (glm::isnan(from.y))
		{
			entry_location.y = 0.0f;
			search_point.y = 0.0f;
		}

		float dist = glm::distance2(entry_location, search_point);
		if (dist < max_distance2)
		{
			items.emplace(dist, std::move(entry));
		}
	}

	for (TDynamicObject *dynobj : simulation::Vehicles.sequence()) {
		if (!dynobj->Controller || dynobj->Prev())
			continue;

		glm::vec3 entry_location = dynobj->GetPosition();
		glm::vec3 search_point = from;

		if (glm::isnan(from.y))
		{
			entry_location.y = 0.0f;
			search_point.y = 0.0f;
		}

		float dist = glm::distance2(entry_location, search_point);
		if (dist < max_distance2)
		{
			auto entry = std::make_shared<map::vehicle>();
			entry->dynobj = dynobj;
			entry->name = dynobj->name();
			items.emplace(dist, std::move(entry));
		}
	}

	return items;
}
