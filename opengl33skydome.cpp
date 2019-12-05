/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "opengl33skydome.h"

#include "simulationenvironment.h"

void opengl33_skydome::update() {

    if (!m_shader) {

        gl::shader vert("vbocolor.vert");
        gl::shader frag("color.frag");
		m_shader.emplace(std::vector<std::reference_wrapper<const gl::shader>>({vert, frag}));
    }

    auto &skydome { simulation::Environment.skydome() };

	if (!m_vertexbuffer) {
		m_vao.emplace();
		m_vao->bind();

		m_vertexbuffer.emplace();
		m_vertexbuffer->allocate(gl::buffer::ARRAY_BUFFER, skydome.vertices().size() * sizeof( glm::vec3 ), GL_STATIC_DRAW);
		m_vertexbuffer->upload(gl::buffer::ARRAY_BUFFER, skydome.vertices().data(), 0, skydome.vertices().size() * sizeof( glm::vec3 ));

		m_vao->setup_attrib(*m_vertexbuffer, 0, 3, GL_FLOAT, sizeof(glm::vec3), 0);

		m_coloursbuffer.emplace();
		m_coloursbuffer->allocate(gl::buffer::ARRAY_BUFFER, skydome.colors().size() * sizeof( glm::vec3 ), GL_STATIC_DRAW);
		m_coloursbuffer->upload(gl::buffer::ARRAY_BUFFER, skydome.colors().data(), 0, skydome.colors().size() * sizeof( glm::vec3 ));

		m_vao->setup_attrib(*m_coloursbuffer, 1, 3, GL_FLOAT, sizeof(glm::vec3), 0);

		m_indexbuffer.emplace();
		m_indexbuffer->allocate(gl::buffer::ELEMENT_ARRAY_BUFFER, skydome.indices().size() * sizeof( unsigned short ), GL_STATIC_DRAW);
		m_indexbuffer->upload(gl::buffer::ELEMENT_ARRAY_BUFFER, skydome.indices().data(), 0, skydome.indices().size() * sizeof( unsigned short ));
		m_vao->setup_ebo(*m_indexbuffer);

		m_vao->unbind();
    }
    // ship the current dynamic data to the gpu
    if( true == skydome.is_dirty() ) {
        if( m_coloursbuffer ) {
            // the colour buffer was already initialized, so on this run we update its content
            m_coloursbuffer->upload( gl::buffer::ARRAY_BUFFER, skydome.colors().data(), 0, skydome.colors().size() * sizeof( glm::vec3 ) );
            skydome.is_dirty() = false;
        }
    }
}

// render skydome to screen
void opengl33_skydome::render() {

    if( ( !m_shader) || ( !m_vao ) ) { return; }

    m_shader->bind();
    m_vao->bind();

    auto const &skydome { simulation::Environment.skydome() };
    ::glDrawElements( GL_TRIANGLES, static_cast<GLsizei>( skydome.indices().size() ), GL_UNSIGNED_SHORT, reinterpret_cast<void const*>( 0 ) );
}
