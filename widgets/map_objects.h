#include "simulation.h"
#include "event.h"
#include "scene.h"

namespace map {

    // semaphore description (only for minimap purposes)
    struct semaphore {
		std::string name;
		glm::vec3 location;

		std::vector<basic_event *> events;
	};

	// switch description (only for minimap purposes)
	struct track_switch {
		glm::vec3 location;
	};

	extern std::vector<semaphore> Semaphores;
	extern std::vector<track_switch> Switches;
}
