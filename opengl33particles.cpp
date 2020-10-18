/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "opengl33particles.h"

#include "particles.h"
#include "openglcamera.h"
#include "simulation.h"
#include "simulationenvironment.h"

std::vector<std::pair<glm::vec3, glm::vec2>> const billboard_vertices {

	{ { -0.5f, -0.5f, 0.f }, { 0.f, 0.f } },
	{ {  0.5f, -0.5f, 0.f }, { 1.f, 0.f } },
	{ {  0.5f,  0.5f, 0.f }, { 1.f, 1.f } },
	{ { -0.5f, -0.5f, 0.f }, { 0.f, 0.f } },
	{ {  0.5f,  0.5f, 0.f }, { 1.f, 1.f } },
	{ { -0.5f,  0.5f, 0.f }, { 0.f, 1.f } },
};

void
opengl33_particles::update( opengl_camera const &Camera ) {

	if (!Global.Smoke)
		return;

	m_particlevertices.clear();
	// build a list of visible smoke sources
	// NOTE: arranged by distance to camera, if we ever need sorting and/or total amount cap-based culling
	std::multimap<float, smoke_source const &> sources;

	for( auto const &source : simulation::Particles.sequence() ) {
		if( false == Camera.visible( source.area() ) ) { continue; }
		// NOTE: the distance is negative when the camera is inside the source's bounding area
		sources.emplace(
		    static_cast<float>( glm::length( Camera.position() - source.area().center ) - source.area().radius ),
		    source );
	}

	if( true == sources.empty() ) { return; }

	// build billboard data for particles from visible sources
	auto const camerarotation { glm::mat3( Camera.modelview() ) };
	particle_vertex vertex;
	for( auto const &source : sources ) {

		auto const particlecolor {
			glm::clamp(
			    source.second.color()
			    * ( glm::vec3 { Global.DayLight.ambient }
                + 0.35f * glm::vec3{ Global.DayLight.diffuse } ) * simulation::Environment.light_intensity(),
			    glm::vec3{ 0.f }, glm::vec3{ 1.f } ) };
		auto const &particles { source.second.sequence() };
		// TODO: put sanity cap on the overall amount of particles that can be drawn
		auto const sizestep { 256.0 * billboard_vertices.size() };
		m_particlevertices.reserve(
		    sizestep * std::ceil( m_particlevertices.size() + ( particles.size() * billboard_vertices.size() ) / sizestep ) );
		for( auto const &particle : particles ) {
			// TODO: particle color support
			vertex.color[ 0 ] = particlecolor.r;
			vertex.color[ 1 ] = particlecolor.g;
			vertex.color[ 2 ] = particlecolor.b;
			vertex.color.a = clamp(particle.opacity, 0.0f, 1.0f);

			auto const offset { glm::vec3{ particle.position - Camera.position() } };
			auto const rotation { glm::angleAxis( particle.rotation, glm::vec3{ 0.f, 0.f, 1.f } ) };

			for( auto const &billboardvertex : billboard_vertices ) {
				vertex.position = offset + ( rotation * billboardvertex.first * particle.size ) * camerarotation;
				vertex.texture = billboardvertex.second;

				m_particlevertices.emplace_back( vertex );
			}
		}
	}

	// ship the billboard data to the gpu:
	// make sure we have enough room...
	if( m_buffercapacity < m_particlevertices.size() ) {
		m_buffercapacity = m_particlevertices.size();
		if (!m_buffer)
			m_buffer.emplace();

		m_buffer->allocate(gl::buffer::ARRAY_BUFFER,
		                   m_buffercapacity * sizeof(particle_vertex), GL_STREAM_DRAW);
	}

	if (m_buffer) {
		// ...send the data...
		m_buffer->upload(gl::buffer::ARRAY_BUFFER,
		                 m_particlevertices.data(), 0, m_particlevertices.size() * sizeof(particle_vertex));
	}
}

std::size_t
opengl33_particles::render() {

    if( false == Global.Smoke ) { return 0; }
	if( m_buffercapacity == 0 ) { return 0; }
	if( m_particlevertices.empty() ) { return 0; }

	if (!m_vao) {
		m_vao.emplace();

		m_vao->setup_attrib(*m_buffer, 0, 3, GL_FLOAT, sizeof(particle_vertex), 0);
		m_vao->setup_attrib(*m_buffer, 1, 4, GL_FLOAT, sizeof(particle_vertex), 12);
		m_vao->setup_attrib(*m_buffer, 2, 2, GL_FLOAT, sizeof(particle_vertex), 28);

		m_buffer->unbind(gl::buffer::ARRAY_BUFFER);
		m_vao->unbind();
	}

	if (!m_shader) {
		gl::shader vert("smoke.vert");
		gl::shader frag("smoke.frag");
		gl::program *prog = new gl::program({vert, frag});
		m_shader = std::unique_ptr<gl::program>(prog);
	}

	m_buffer->bind(gl::buffer::ARRAY_BUFFER);
	m_shader->bind();
	m_vao->bind();

	glDrawArrays(GL_TRIANGLES, 0, m_particlevertices.size());

	m_shader->unbind();
	m_vao->unbind();
	m_buffer->unbind(gl::buffer::ARRAY_BUFFER);

    return m_particlevertices.size() / 6;
}

//---------------------------------------------------------------------------
