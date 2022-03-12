/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "geometrybank.h"

#include "sn_utils.h"
#include "Logs.h"
#include "Globals.h"

namespace gfx {

void
basic_vertex::serialize( std::ostream &s, bool const Tangent ) const {

    sn_utils::s_vec3( s, position );
    sn_utils::s_vec3( s, normal );
    sn_utils::ls_float32( s, texture.x );
    sn_utils::ls_float32( s, texture.y );
    if( Tangent ) {
        sn_utils::s_vec4( s, tangent );
    }
}

void
basic_vertex::deserialize( std::istream &s, bool const Tangent ) {

    position = sn_utils::d_vec3( s );
    normal = sn_utils::d_vec3( s );
    texture.x = sn_utils::ld_float32( s );
    texture.y = sn_utils::ld_float32( s );
    if( Tangent ) {
        tangent = sn_utils::d_vec4( s );
    }
}

void
basic_vertex::serialize_packed( std::ostream &s, bool const Tangent ) const {

    sn_utils::ls_uint64( s, glm::packHalf4x16( { position, 0.f } ) );
    sn_utils::ls_uint32( s, glm::packSnorm3x10_1x2( { normal, 0.f } ) );
    sn_utils::ls_uint16( s, glm::packHalf1x16( texture.x ) );
    sn_utils::ls_uint16( s, glm::packHalf1x16( texture.y ) );
    if( Tangent ) {
        sn_utils::ls_uint32( s, glm::packSnorm3x10_1x2( tangent ) );
    }
}

void
basic_vertex::deserialize_packed( std::istream &s, bool const Tangent ) {

    position = glm::unpackHalf4x16( sn_utils::ld_uint64( s ) );
    normal = glm::unpackSnorm3x10_1x2( sn_utils::ld_uint32( s ) );
    texture.x = glm::unpackHalf1x16( sn_utils::ld_uint16( s ) );
    texture.y = glm::unpackHalf1x16( sn_utils::ld_uint16( s ) );
    if( Tangent ) {
        tangent = glm::unpackSnorm3x10_1x2( sn_utils::ld_uint32( s ) );
    }
}

// based on
// Lengyel, Eric. “Computing Tangent Space Basis Vectors for an Arbitrary Mesh”.
// Terathon Software, 2001. http://terathon.com/code/tangent.html
void calculate_tangents(vertex_array &vertices, index_array const &indices, int const type)
{
    size_t vertex_count = vertices.size();

    if (!vertex_count || vertices[0].tangent.w != 0.0f)
        return;

    size_t triangle_count;
    size_t tri_count_base = indices.empty() ? vertex_count : indices.size();

    if (type == GL_TRIANGLES)
        triangle_count = tri_count_base / 3;
    else if (type == GL_TRIANGLE_STRIP)
        triangle_count = tri_count_base - 2;
    else if (type == GL_TRIANGLE_FAN)
        triangle_count = tri_count_base - 2;
    else
        return;

    std::vector<glm::vec3> tan(vertex_count * 2);

    for (size_t a = 0; a < triangle_count; a++)
    {
        size_t i1, i2, i3;
        if (type == GL_TRIANGLES)
        {
            i1 = a * 3;
            i2 = a * 3 + 1;
            i3 = a * 3 + 2;
        }
        else if (type == GL_TRIANGLE_STRIP)
        {
            if (a % 2 == 0)
            {
                i1 = a;
                i2 = a + 1;
            }
            else
            {
                i1 = a + 1;
                i2 = a;
            }
            i3 = a + 2;
        }
        else if (type == GL_TRIANGLE_FAN)
        {
            i1 = 0;
            i2 = a + 1;
            i3 = a + 2;
        }

        if (!indices.empty()) {
            i1 = indices[i1];
            i2 = indices[i2];
            i3 = indices[i3];
        }

        const glm::vec3 &v1 = vertices[i1].position;
        const glm::vec3 &v2 = vertices[i2].position;
        const glm::vec3 &v3 = vertices[i3].position;

        const glm::vec2 &w1 = vertices[i1].texture;
        const glm::vec2 &w2 = vertices[i2].texture;
        const glm::vec2 &w3 = vertices[i3].texture;

        float x1 = v2.x - v1.x;
        float x2 = v3.x - v1.x;
        float y1 = v2.y - v1.y;
        float y2 = v3.y - v1.y;
        float z1 = v2.z - v1.z;
        float z2 = v3.z - v1.z;

        float s1 = w2.x - w1.x;
        float s2 = w3.x - w1.x;
        float t1 = w2.y - w1.y;
        float t2 = w3.y - w1.y;

		float ri = (s1 * t2 - s2 * t1);
		if (ri == 0.0f) {
			//ErrorLog("Bad model: failed to generate tangent vectors for vertices: " +
			//         std::to_string(i1) + ", " + std::to_string(i2) + ", " + std::to_string(i3));
			// useless error, as we don't have name of problematic model here
			// why does it happen?
			ri = 1.0f;
		}

		float r = 1.0f / ri;
        glm::vec3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r,
            (t2 * z1 - t1 * z2) * r);
        glm::vec3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r,
            (s1 * z2 - s2 * z1) * r);

        tan[i1] += sdir;
        tan[i2] += sdir;
        tan[i3] += sdir;

        tan[vertex_count + i1] += tdir;
        tan[vertex_count + i2] += tdir;
        tan[vertex_count + i3] += tdir;
    }

    for (size_t a = 0; a < vertex_count; a++)
    {
        const glm::vec3 &n = vertices[a].normal;
        const glm::vec3 &t = tan[a];
        const glm::vec3 &t2 = tan[vertex_count + a];

        vertices[a].tangent =
            glm::vec4(
                glm::normalize((t - n * glm::dot(n, t))),
                (glm::dot(glm::cross(n, t), t2) < 0.0F) ?
                   -1.0F :
                    1.0F);
    }
}

void calculate_indices( index_array &Indices, vertex_array &Vertices, float tolerancescale ) {

    Indices.resize( Vertices.size() );
    std::iota( std::begin( Indices ), std::end( Indices ), 0 );
    // gather instances of used vertices, replace the original vertex bank with it after you're done
    vertex_array indexedvertices;
    indexedvertices.reserve( std::max<size_t>( 100, Vertices.size() / 3 ) ); // optimistic guesstimate, but should reduce re-allocation somewhat
    auto const matchtolerance { 1e-5f * tolerancescale };
    for( auto idx = 0; idx < Indices.size(); ++idx ) {
        if( Indices[ idx ] < idx ) {
            // this index is pointing to a vertex out of linear order, i.e. it's an already processed duplicate we can skip
            continue;
        }
        // due to duplicate removal our vertex will likely have different index in the processed set
        Indices[ idx ] = indexedvertices.size();
        // see if there's any pointers in the remaining index subrange to similar enough vertices
        // if found, remap these to use our current vertex instead
        auto vertex { Vertices[ idx ] };
        auto matchiter { std::cbegin( Vertices ) + idx };
        for( auto matchidx = idx + 1; matchidx < Indices.size() && matchidx < idx + Global.iConvertIndexRange; ++matchidx ) {
            ++matchiter;
            if( ( glm::all( glm::epsilonEqual( vertex.position, matchiter->position, matchtolerance ) ) )
             && ( glm::all( glm::epsilonEqual( vertex.normal,   matchiter->normal, matchtolerance ) ) )
             && ( glm::all( glm::epsilonEqual( vertex.texture,  matchiter->texture, matchtolerance ) ) ) ) {
                Indices[ matchidx ] = Indices[ idx ];
            }
        }
        indexedvertices.emplace_back( vertex );
    }
    // done indexing, swap the source vertex bank with the processed one
    Vertices.swap( indexedvertices );
}

// generic geometry bank class, allows storage, update and drawing of geometry chunks

// creates a new geometry chunk of specified type from supplied data. returns: handle to the chunk or NULL
gfx::geometry_handle
geometry_bank::create( gfx::vertex_array &Vertices, unsigned int const Type ) {

    if( true == Vertices.empty() ) { return { 0, 0 }; }

    m_chunks.emplace_back( Vertices, Type );
    // NOTE: handle is effectively (index into chunk array + 1) this leaves value of 0 to serve as error/empty handle indication
    gfx::geometry_handle chunkhandle { 0, static_cast<std::uint32_t>(m_chunks.size()) };
    // template method implementation
    create_( chunkhandle );
    // all done
    return chunkhandle;
}

// creates a new indexed geometry chunk of specified type from supplied data. returns: handle to the chunk or NULL
gfx::geometry_handle
geometry_bank::create( gfx::index_array &Indices, gfx::vertex_array &Vertices, unsigned int const Type ) {

    if( true == Vertices.empty() ) { return { 0, 0 }; }

    m_chunks.emplace_back( Indices, Vertices, Type );
    // NOTE: handle is effectively (index into chunk array + 1) this leaves value of 0 to serve as error/empty handle indication
    gfx::geometry_handle chunkhandle { 0, static_cast<std::uint32_t>(m_chunks.size()) };
    // template method implementation
    create_( chunkhandle );
    // all done
    return chunkhandle;
}

// replaces data of specified chunk with the supplied vertex data, starting from specified offset
bool
geometry_bank::replace( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, std::size_t const Offset ) {

    if( ( Geometry.chunk == 0 ) || ( Geometry.chunk > m_chunks.size() ) ) { return false; }

    auto &chunk = gfx::geometry_bank::chunk( Geometry );

    if( ( Offset == 0 )
     && ( Vertices.size() == chunk.vertices.size() ) ) {
        // check first if we can get away with a simple swap...
        chunk.vertices.swap( Vertices );
    }
    else {
        // ...otherwise we need to do some legwork
        // NOTE: if the offset is larger than existing size of the chunk, it'll bridge the gap with 'blank' vertices
        // TBD: we could bail out with an error instead if such request occurs
        chunk.vertices.resize( Offset + Vertices.size(), gfx::basic_vertex() );
        chunk.vertices.insert( std::end( chunk.vertices ), std::begin( Vertices ), std::end( Vertices ) );
    }
    // template method implementation
    replace_( Geometry );
    // all done
    return true;
}

// adds supplied vertex data at the end of specified chunk
bool
geometry_bank::append( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry ) {

    if( ( Geometry.chunk == 0 ) || ( Geometry.chunk > m_chunks.size() ) ) { return false; }

    return replace( Vertices, Geometry, gfx::geometry_bank::chunk( Geometry ).vertices.size() );
}

// draws geometry stored in specified chunk
std::size_t
geometry_bank::draw( gfx::geometry_handle const &Geometry, gfx::stream_units const &Units, unsigned int const Streams ) {
    // template method implementation
    return draw_( Geometry, Units, Streams );
}

// frees subclass-specific resources associated with the bank, typically called when the bank wasn't in use for a period of time
void
geometry_bank::release() {
    // template method implementation
    release_();
}

// provides direct access to indexdata of specfied chunk
index_array const &
geometry_bank::indices( gfx::geometry_handle const &Geometry ) const {

    return geometry_bank::chunk( Geometry ).indices;
}

// provides direct access to vertex data of specfied chunk
vertex_array const &
geometry_bank::vertices( gfx::geometry_handle const &Geometry ) const {

    return geometry_bank::chunk( Geometry ).vertices;
}

// geometry bank manager, holds collection of geometry banks

// performs a resource sweep
void
geometrybank_manager::update() {

    m_garbagecollector.sweep();
}

// creates a new geometry bank. returns: handle to the bank or NULL
gfx::geometrybank_handle
geometrybank_manager::register_bank(std::unique_ptr<geometry_bank> bank) {
    m_geometrybanks.emplace_back(std::move(bank), std::chrono::steady_clock::time_point() );

    // NOTE: handle is effectively (index into chunk array + 1) this leaves value of 0 to serve as error/empty handle indication
    return { static_cast<std::uint32_t>( m_geometrybanks.size() ), 0 };
}

// creates a new geometry chunk of specified type from supplied data, in specified bank. returns: handle to the chunk or NULL
gfx::geometry_handle
geometrybank_manager::create_chunk(vertex_array &Vertices, gfx::geometrybank_handle const &Geometry, int const Type ) {

    auto const newchunkhandle = bank( Geometry ).first->create( Vertices, Type );

    if( newchunkhandle.chunk != 0 ) { return { Geometry.bank, newchunkhandle.chunk }; }
    else                            { return { 0, 0 }; }
}

// creates a new indexed geometry chunk of specified type from supplied data, in specified bank. returns: handle to the chunk or NULL
gfx::geometry_handle
geometrybank_manager::create_chunk( gfx::index_array &Indices, gfx::vertex_array &Vertices, gfx::geometrybank_handle const &Geometry, unsigned int const Type ) {

    auto const newchunkhandle = bank( Geometry ).first->create( Indices, Vertices, Type );

    if( newchunkhandle.chunk != 0 ) { return { Geometry.bank, newchunkhandle.chunk }; }
    else                            { return { 0, 0 }; }
}

// replaces data of specified chunk with the supplied vertex data, starting from specified offset
bool
geometrybank_manager::replace( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry, std::size_t const Offset ) {

    return bank( Geometry ).first->replace( Vertices, Geometry, Offset );
}

// adds supplied vertex data at the end of specified chunk
bool
geometrybank_manager::append( gfx::vertex_array &Vertices, gfx::geometry_handle const &Geometry ) {

    return bank( Geometry ).first->append( Vertices, Geometry );
}
// draws geometry stored in specified chunk
void
geometrybank_manager::draw( gfx::geometry_handle const &Geometry, unsigned int const Streams ) {

    if( Geometry == null_handle ) { return; }

    auto &bankrecord = bank( Geometry );

    bankrecord.second = m_garbagecollector.timestamp();
    m_primitivecount += bankrecord.first->draw( Geometry, m_units, Streams );
}

// provides direct access to index data of specfied chunk
gfx::index_array const &
geometrybank_manager::indices( gfx::geometry_handle const &Geometry ) const {

    return bank( Geometry ).first->indices( Geometry );
}

// provides direct access to vertex data of specfied chunk
gfx::vertex_array const &
geometrybank_manager::vertices( gfx::geometry_handle const &Geometry ) const {

    return bank( Geometry ).first->vertices( Geometry );
}

} // namespace gfx
