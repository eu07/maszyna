/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma	once

#include "gl/vao.h"
#include "gl/shader.h"
#include "Texture.h"

// based on "Rendering Falling Rain and Snow"
// by Niniane Wang, Bretton Wade

class basic_precipitation {

public:
// constructors
    basic_precipitation() = default;
// destructor
	~basic_precipitation();
// methods
    bool
        init();
	void
	    update_weather();
    void
        update();
	void
        render();
	float
	    get_textureoffset();

    glm::dvec3 m_cameramove{ 0.0 };

private:
// methods
    void create( int const Tesselation );

// members
    std::vector<glm::vec3> m_vertices;
    std::vector<glm::vec2> m_uvs;
    std::vector<std::uint16_t> m_indices;
    texture_handle m_texture { -1 };
    float m_textureoffset { 0.f };
    float m_moverate { 30 * 0.001f };
    float m_moverateweathertypefactor { 1.f }; // medium-dependent; 1.0 for snow, faster for rain
    glm::dvec3 m_camerapos { 0.0 };
    bool m_freeflymode { true };
    bool m_windowopen { true };
    int m_activecab{ 0 };
    glm::dvec3 m_cabcameramove{ 0.0 };

	std::optional<gl::buffer> m_vertexbuffer;
	std::optional<gl::buffer> m_uvbuffer;
	std::optional<gl::buffer> m_indexbuffer;
	std::optional<gl::program> m_shader;
	std::optional<gl::vao> m_vao;
};
