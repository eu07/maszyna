/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma	once

#include "Texture.h"
#include "gl/vao.h"
#include "gl/shader.h"

class opengl33_precipitation {

public:
// constructors
    opengl33_precipitation() = default;
// destructor
	~opengl33_precipitation();
// methods
    void
        update();
	void
        render();

private:
// methods
    void create( int const Tesselation );
// members
    std::vector<glm::vec3> m_vertices;
    std::vector<glm::vec2> m_uvs;
    std::vector<std::uint16_t> m_indices;
    texture_handle m_texture { -1 };
    float m_overcast { -1.f }; // cached overcast level, difference from current state triggers texture update
    std::optional<gl::buffer> m_vertexbuffer;
    std::optional<gl::buffer> m_uvbuffer;
    std::optional<gl::buffer> m_indexbuffer;
    std::optional<gl::program> m_shader;
    std::optional<gl::vao> m_vao;
};
