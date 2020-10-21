/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "ResourceManager.h"

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
    void serialize( std::ostream&, bool const Tangent = false ) const;
    void deserialize( std::istream&, bool const Tangent = false );
    void serialize_packed( std::ostream&, bool const Tangent = false ) const;
    void deserialize_packed( std::istream&, bool const Tangent = false );
};

// data streams carried in a vertex
enum stream {
    none     = 0x0,
    position = 0x1,
    normal   = 0x2,
    color    = 0x4, // currently normal and colour streams are stored in the same slot, and mutually exclusive
    texture  = 0x8
};

unsigned int const basic_streams { stream::position | stream::normal | stream::texture };
unsigned int const color_streams { stream::position | stream::color | stream::texture };

struct stream_units {

    std::vector<GLint> texture { GL_TEXTURE0 }; // unit associated with main texture data stream. TODO: allow multiple units per stream
};

using basic_index = std::uint32_t;

using vertex_array = std::vector<basic_vertex>;
using index_array = std::vector<basic_index>;

void calculate_tangents( vertex_array &vertices, index_array const &indices, int const type );
void calculate_indices( index_array &Indices, vertex_array &Vertices, float tolerancescale = 1.0f );

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
    // creates a new geometry chunk of specified type from supplied data. returns: handle to the chunk or NULL
    auto create( gfx::vertex_array &Vertices, unsigned int const Type ) -> gfx::geometry_handle;
    // creates a new indexed geometry chunk of specified type from supplied data. returns: handle to the chunk or NULL
    auto create( gfx::index_array &Indices, gfx::vertex_array &Vertices, unsigned int const Type ) -> gfx::geometry_handle;
    // replaces vertex data of specified chunk with the supplied data, starting from specified offset
    auto replace( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, std::size_t const Offset = 0 ) -> bool;
    // adds supplied vertex data at the end of specified chunk
    auto append( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry ) -> bool;
    // draws geometry stored in specified chunk
    auto draw( gfx::geometry_handle const &Geometry, gfx::stream_units const &Units, unsigned int const Streams = basic_streams ) -> std::size_t;
    // draws geometry stored in supplied list of chunks
    template <typename Iterator_>
    auto draw( Iterator_ First, Iterator_ Last, gfx::stream_units const &Units, unsigned int const Streams = basic_streams ) ->std::size_t {
            std::size_t count { 0 };
            while( First != Last ) {
                count += draw( *First, Units, Streams ); ++First; }
            return count; }
    // frees subclass-specific resources associated with the bank, typically called when the bank wasn't in use for a period of time
    void release();
    // provides direct access to index data of specfied chunk
    auto indices( gfx::geometry_handle const &Geometry ) const -> gfx::index_array const &;
    // provides direct access to vertex data of specfied chunk
    auto vertices( gfx::geometry_handle const &Geometry ) const -> gfx::vertex_array const &;

protected:
// types:
    struct geometry_chunk {
        unsigned int type; // kind of geometry used by the chunk
        gfx::vertex_array vertices; // geometry data
        gfx::index_array indices; // index data
        // NOTE: constructor doesn't copy provided geometry data, but moves it
        geometry_chunk( gfx::vertex_array &Vertices, unsigned int Type ) :
                                                            type( Type )
        {
            vertices.swap( Vertices );
        }
        // NOTE: constructor doesn't copy provided geometry data, but moves it
        geometry_chunk( gfx::index_array &Indices, gfx::vertex_array &Vertices, unsigned int Type ) :
                                                                                       type( Type )
        {
            vertices.swap( Vertices );
            indices.swap( Indices );
        }
    };

    using geometrychunk_sequence = std::vector<geometry_chunk>;

// methods
    inline
    auto chunk( gfx::geometry_handle const Geometry ) -> geometry_chunk & {
            return m_chunks[ Geometry.chunk - 1 ]; }
    inline
    auto chunk( gfx::geometry_handle const Geometry ) const -> geometry_chunk const & {
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
    virtual auto draw_( gfx::geometry_handle const &Geometry, gfx::stream_units const &Units, unsigned int const Streams ) -> std::size_t = 0;
    // resource release subclass details
    virtual void release_() = 0;
};

// geometry bank manager, holds collection of geometry banks

using geometrybank_handle = geometry_handle;

class geometrybank_manager {

public:
// constructors
    geometrybank_manager() = default;
// methods:
    // performs a resource sweep
    void update();
    // registers a new geometry bank. returns: handle to the bank
    auto register_bank(std::unique_ptr<geometry_bank> bank) -> gfx::geometrybank_handle;
    // creates a new geometry chunk of specified type from supplied data, in specified bank. returns: handle to the chunk or NULL
    auto create_chunk( gfx::vertex_array &Vertices, gfx::geometrybank_handle const &Geometry, int const Type ) -> gfx::geometry_handle;
    // creates a new indexed geometry chunk of specified type from supplied data, in specified bank. returns: handle to the chunk or NULL
    auto create_chunk( gfx::index_array &Indices, gfx::vertex_array &Vertices, gfx::geometrybank_handle const &Geometry, unsigned int const Type ) -> gfx::geometry_handle;
    // replaces data of specified chunk with the supplied vertex data, starting from specified offset
    auto replace( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, std::size_t const Offset = 0 ) -> bool;
    // adds supplied vertex data at the end of specified chunk
    auto append( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry ) -> bool;
    // draws geometry stored in specified chunk
    void draw( gfx::geometry_handle const &Geometry, unsigned int const Streams = basic_streams );
    template <typename Iterator_>
    void draw( Iterator_ First, Iterator_ Last, unsigned int const Streams = basic_streams ) {
            while( First != Last ) { 
                draw( *First, Streams );
                ++First; } }
    // provides direct access to index data of specfied chunk
    auto indices( gfx::geometry_handle const &Geometry ) const -> gfx::index_array const &;
    // provides direct access to vertex data of specfied chunk
    auto vertices( gfx::geometry_handle const &Geometry ) const -> gfx::vertex_array const &;
    // sets target texture unit for the texture data stream
    auto units() -> gfx::stream_units & { return m_units; }
    // provides access to primitives count
    auto primitives_count() const -> std::size_t const & { return m_primitivecount; }
    auto primitives_count() -> std::size_t & { return m_primitivecount; }

private:
// types:
    using geometrybanktimepoint_pair = std::pair< std::shared_ptr<geometry_bank>, resource_timestamp >;
    using geometrybanktimepointpair_sequence = std::deque< geometrybanktimepoint_pair >;

    // members:
    geometrybanktimepointpair_sequence m_geometrybanks;
    garbage_collector<geometrybanktimepointpair_sequence> m_garbagecollector { m_geometrybanks, 60, 120, "geometry buffer" };
    gfx::stream_units m_units;

// methods
    inline
    auto valid( gfx::geometry_handle const &Geometry ) const -> bool {
            return ( ( Geometry.bank != 0 )
                  && ( Geometry.bank <= m_geometrybanks.size() ) ); }
    inline
    auto bank( gfx::geometry_handle const Geometry ) -> geometrybanktimepointpair_sequence::value_type & {
            return m_geometrybanks[ Geometry.bank - 1 ]; }
    inline
    auto bank( gfx::geometry_handle const Geometry ) const -> geometrybanktimepointpair_sequence::value_type const & {
            return m_geometrybanks[ Geometry.bank - 1 ]; }

// members:
    std::size_t m_primitivecount { 0 }; // number of drawn primitives
};

} // namespace gfx
