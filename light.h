/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "color.h"

// a simple light source, either omni- or directional
struct basic_light {

    glm::vec4 ambient { colors::none };
    glm::vec4 diffuse { colors::white };
    glm::vec4 specular { colors::white };
    glm::vec3 position;
    glm::vec3 direction;
    bool is_directional { true };
};
