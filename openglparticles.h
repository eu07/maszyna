/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

class opengl_camera;

// particle data visualizer
class opengl_particles {
public:
// constructors
    opengl_particles() = default;
// destructor
    ~opengl_particles() {
        if( m_buffer != -1 ) {
            ::glDeleteBuffers( 1, &m_buffer ); } }
// methods
    void
        update( opengl_camera const &Camera );
    std::size_t
        render( GLint const Textureunit );
private:
// types
    struct particle_vertex {
        glm::vec3 position; // 3d space
        std::uint8_t color[ 4 ]; // rgba, unsigned byte format
        glm::vec2 texture; // uv space
        float padding[ 2 ]; // experimental, some gfx hardware allegedly works better with 32-bit aligned data blocks
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
    GLuint m_buffer{ (GLuint)-1 }; // id of the buffer holding geometry data on the opengl end
    std::size_t m_buffercapacity{ 0 }; // total capacity of the last established buffer
};

//---------------------------------------------------------------------------
