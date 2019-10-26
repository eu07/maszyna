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

class opengl_precipitation {

public:
// constructors
    opengl_precipitation() = default;
// destructor
	~opengl_precipitation();
// methods
    inline
    void
        set_unit( GLint const Textureunit ) {
            m_textureunit = Textureunit; }
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
    GLuint m_vertexbuffer { (GLuint)-1 };
    GLuint m_uvbuffer { (GLuint)-1 };
    GLuint m_indexbuffer { (GLuint)-1 };
    GLint m_textureunit { 0 };
    texture_handle m_texture { -1 };
    float m_overcast { -1.f }; // cached overcast level, difference from current state triggers texture update
};
