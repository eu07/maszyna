/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "GL/glew.h"
#ifdef _WIN32
#include "GL/wglew.h"
#endif
#include "ResourceManager.h"
#include "gl/vao.h"

namespace gfx {

struct basic_vertex {

    glm::vec3 position; // 3d space
    glm::vec3 normal; // 3d space
    glm::vec2 texture; // uv space
    glm::vec4 tangent; // xyz - tangent, w - handedness

    basic_vertex() = default;
    basic_vertex( glm::vec3 Position,  glm::vec3 Normal,  glm::vec2 Texture ) :
                  position( Position ),  normal( Normal ), texture( Texture )
    {}
    void serialize( std::ostream& ) const;
    void deserialize( std::istream& );
};

typedef std::vector<basic_vertex> vertex_array;

void calculate_tangent(vertex_array &vertices, int type);

// generic geometry bank class, allows storage, update and drawing of geometry chunks

struct geometry_handle {
// constructors
    geometry_handle() :
        bank( 0 ), chunk( 0 )
    {}
    geometry_handle( std::uint32_t Bank, std::uint32_t Chunk ) :
                             bank( Bank ),      chunk( Chunk )
    {}
// methods
    inline
    operator std::uint64_t() const {
/*
        return bank << 14 | chunk; }
*/
        return ( std::uint64_t { bank } << 32 | chunk );
    }

// members
/*
    std::uint32_t
        bank  : 18, // 250k banks
        chunk : 14; // 16k chunks per bank
*/
    std::uint32_t bank;
    std::uint32_t chunk;
};

class geometry_bank {

public:
// types:

// constructors:

// destructor:
    virtual
        ~geometry_bank() {}

// methods:
    // creates a new geometry chunk of specified type from supplied vertex data. returns: handle to the chunk or NULL
    gfx::geometry_handle
        create( gfx::vertex_array const &Vertices, unsigned int const Type );
    // replaces data of specified chunk with the supplied vertex data, starting from specified offset
    bool
        replace( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, std::size_t const Offset = 0 );
    // adds supplied vertex data at the end of specified chunk
    bool
        append( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry );
    // draws geometry stored in specified chunk
    void
        draw( gfx::geometry_handle const &Geometry );
    // draws geometry stored in supplied list of chunks
    void draw(std::vector<gfx::geometry_handle>::iterator begin, std::vector<gfx::geometry_handle>::iterator end)
    {
        draw_(begin, end);
    }
    // frees subclass-specific resources associated with the bank, typically called when the bank wasn't in use for a period of time
    void
        release();
    // provides direct access to vertex data of specfied chunk
    gfx::vertex_array const &
        vertices( gfx::geometry_handle const &Geometry ) const;

protected:
// types:
    struct geometry_chunk {
        unsigned int type; // kind of geometry used by the chunk
        gfx::vertex_array vertices; // geometry data
        geometry_chunk( gfx::vertex_array const &Vertices, unsigned int Type ) :
                                                            type( Type )
        {
            vertices = Vertices;
        }
    };

    typedef std::vector<geometry_chunk> geometrychunk_sequence;

// methods
    inline
    geometry_chunk &
        chunk( gfx::geometry_handle const Geometry ) {
            return m_chunks[ Geometry.chunk - 1 ]; }
    inline
    geometry_chunk const &
        chunk( gfx::geometry_handle const Geometry ) const {
            return m_chunks[ Geometry.chunk - 1 ]; }

// members:
    geometrychunk_sequence m_chunks;

private:
// methods:
    // create() subclass details
    virtual void create_( gfx::geometry_handle const &Geometry ) = 0;
    // replace() subclass details
    virtual void replace_( gfx::geometry_handle const &Geometry ) = 0;
    // draw() subclass details
    virtual void draw_( gfx::geometry_handle const &Geometry ) = 0;
    virtual void draw_(const std::vector<gfx::geometry_handle>::iterator begin, const std::vector<gfx::geometry_handle>::iterator end) = 0;
    // resource release subclass details
    virtual void release_() = 0;
};

// opengl vbo-based variant of the geometry bank

class opengl_vbogeometrybank : public geometry_bank {

public:
// constructors:
    opengl_vbogeometrybank() = default;
// destructor
    ~opengl_vbogeometrybank() {
        delete_buffer(); }
// methods:
    static
    void
        reset() { }

private:
// types:
    struct chunk_record{
        std::size_t offset{ 0 }; // beginning of the chunk data as offset from the beginning of the last established buffer
        std::size_t size{ 0 }; // size of the chunk in the last established buffer
        bool is_good{ false }; // true if local content of the chunk matches the data on the opengl end
    };

    typedef std::vector<chunk_record> chunkrecord_sequence;
    void setup_buffer();

    // vectors for glMultiDrawArrays in class scope
    // to don't waste time on reallocating
    std::vector<GLint> m_offsets;
    std::vector<GLsizei> m_counts;

// methods:
    // create() subclass details
    void
        create_( gfx::geometry_handle const &Geometry );
    // replace() subclass details
    void
        replace_( gfx::geometry_handle const &Geometry );
    // draw() subclass details
    void
        draw_( gfx::geometry_handle const &Geometry );
    void draw_(const std::vector<gfx::geometry_handle>::iterator begin, const std::vector<gfx::geometry_handle>::iterator end);
    // release() subclass details
    void
        release_();
    void
        delete_buffer();

// members:
    GLuint m_buffer { 0 }; // id of the buffer holding data on the opengl end
    std::unique_ptr<gl::vao> m_vao;
    std::size_t m_buffercapacity{ 0 }; // total capacity of the last established buffer
    chunkrecord_sequence m_chunkrecords; // helper data for all stored geometry chunks, in matching order

};

// geometry bank manager, holds collection of geometry banks

typedef geometry_handle geometrybank_handle;

class geometrybank_manager {

public:
// methods:
    // performs a resource sweep
    void update();
    // creates a new geometry bank. returns: handle to the bank or NULL
    gfx::geometrybank_handle
        create_bank();
    // creates a new geometry chunk of specified type from supplied vertex data, in specified bank. returns: handle to the chunk or NULL
    gfx::geometry_handle
        create_chunk( gfx::vertex_array const &Vertices, gfx::geometrybank_handle const &Geometry, int const Type );
    // replaces data of specified chunk with the supplied vertex data, starting from specified offset
    bool
        replace( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, std::size_t const Offset = 0 );
    // adds supplied vertex data at the end of specified chunk
    bool
        append( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry );
    // draws geometry stored in specified chunk
    void
        draw( gfx::geometry_handle const &Geometry);
    void draw(const std::vector<gfx::geometry_handle>::iterator begin, const std::vector<gfx::geometry_handle>::iterator end);
    // provides direct access to vertex data of specfied chunk
    gfx::vertex_array const &
        vertices( gfx::geometry_handle const &Geometry ) const;

private:
// types:
    typedef std::pair<
        std::shared_ptr<geometry_bank>,
        resource_timestamp > geometrybanktimepoint_pair;

    typedef std::deque< geometrybanktimepoint_pair > geometrybanktimepointpair_sequence;

    // members:
    geometrybanktimepointpair_sequence m_geometrybanks;
    garbage_collector<geometrybanktimepointpair_sequence> m_garbagecollector { m_geometrybanks, 60, 120, "geometry buffer" };

// methods
    inline
    bool
        valid( gfx::geometry_handle const &Geometry ) const {
            return ( ( Geometry.bank != 0 )
                  && ( Geometry.bank <= m_geometrybanks.size() ) ); }
    inline
    geometrybanktimepointpair_sequence::value_type &
        bank( gfx::geometry_handle const Geometry ) {
            return m_geometrybanks[ Geometry.bank - 1 ]; }
    inline
    geometrybanktimepointpair_sequence::value_type const &
        bank( gfx::geometry_handle const Geometry ) const {
            return m_geometrybanks[ Geometry.bank - 1 ]; }

};

} // namespace gfx
