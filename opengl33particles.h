/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "gl/buffer.h"
#include "gl/vao.h"
#include "gl/shader.h"

class opengl_camera;

// particle data visualizer
class opengl33_particles {
public:
// constructors
	opengl33_particles() = default;

// methods
	void
	    update( opengl_camera const &Camera );
    std::size_t
	    render( );
private:
// types
	struct particle_vertex {
		glm::vec3 position; // 3d space
		glm::vec4 color; // rgba, unsigned byte format
		glm::vec2 texture; // uv space
	};
/*
	using sourcedistance_pair = std::pair<smoke_source *, float>;
	using source_sequence = std::vector<sourcedistance_pair>;
*/
	using particlevertex_sequence = std::vector<particle_vertex>;
// methods
// members
/*
	source_sequence m_sources; // list of particle sources visible in current render pass, with their respective distances to the camera
*/
	particlevertex_sequence m_particlevertices; // geometry data of visible particles, generated on the cpu end
	std::optional<gl::buffer> m_buffer;
	std::optional<gl::vao> m_vao;
	std::unique_ptr<gl::program> m_shader;

	std::size_t m_buffercapacity{ 0 }; // total capacity of the last established buffer
};

//---------------------------------------------------------------------------
