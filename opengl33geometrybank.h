/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "geometrybank.h"
#include "gl/buffer.h"
#include "gl/vao.h"

namespace gfx {

// opengl vao/vbo-based variant of the geometry bank

class opengl33_vaogeometrybank : public geometry_bank {

public:
// constructors:
    opengl33_vaogeometrybank() = default;
// destructor
    ~opengl33_vaogeometrybank() {
        delete_buffer(); }
// methods:
    static
    void
        reset() {;}

private:
// types:
    struct chunk_record {
        std::size_t vertex_offset{ 0 }; // beginning of the chunk vertex data as offset from the beginning of the last established buffer
        std::size_t vertex_count{ 0 }; // size of the chunk in the last established buffer
        std::size_t index_offset{ 0 };
        std::size_t index_count{ 0 };
        bool is_good{ false }; // true if local content of the chunk matches the data on the opengl end
    };

    typedef std::vector<chunk_record> chunkrecord_sequence;

// methods:
    // create() subclass details
    void
        create_( gfx::geometry_handle const &Geometry ) override;
    // replace() subclass details
    void
        replace_( gfx::geometry_handle const &Geometry ) override;
    // draw() subclass details
    auto
        draw_( gfx::geometry_handle const &Geometry, gfx::stream_units const &Units, unsigned int const Streams ) -> std::size_t override;
    // release() subclass details
    void
        release_() override;
    void
        setup_buffer();
    void
        setup_attrib(size_t offset = 0);
    void
        delete_buffer();

// members:
	std::optional<gl::buffer> m_vertexbuffer; // vertex buffer data on the opengl end
    std::optional<gl::buffer> m_indexbuffer; // index buffer data on the opengl end
	std::optional<gl::vao> m_vao;
    chunkrecord_sequence m_chunkrecords; // helper data for all stored geometry chunks, in matching order
};

} // namespace gfx
