/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "scenenode.h"

#include "Model3d.h"
#include "renderer.h"
#include "Logs.h"
#include "sn_utils.h"

// stores content of the struct in provided output stream
void
lighting_data::serialize( std::ostream &Output ) const {

    sn_utils::s_vec4( Output, diffuse );
    sn_utils::s_vec4( Output, ambient );
    sn_utils::s_vec4( Output, specular );
}

// restores content of the struct from provided input stream
void
lighting_data::deserialize( std::istream &Input ) {

    diffuse = sn_utils::d_vec4( Input );
    ambient = sn_utils::d_vec4( Input );
    specular = sn_utils::d_vec4( Input );
}

namespace scene {

// stores content of the struct in provided output stream
void
bounding_area::serialize( std::ostream &Output ) const {
    // center
    sn_utils::s_dvec3( Output, center );
    // radius
    sn_utils::ls_float32( Output, radius );
}

// restores content of the struct from provided input stream
void
bounding_area::deserialize( std::istream &Input ) {

    center = sn_utils::d_dvec3( Input );
    radius = sn_utils::ld_float32( Input );
}



// sends content of the struct to provided stream
void
shape_node::shapenode_data::serialize( std::ostream &Output ) const {
    // bounding area
    area.serialize( Output );
    // visibility
    sn_utils::ls_float64( Output, rangesquared_min );
    sn_utils::ls_float64( Output, rangesquared_max );
    sn_utils::s_bool( Output, visible );
    // material
    sn_utils::s_bool( Output, translucent );
    // NOTE: material handle is created dynamically on load
    sn_utils::s_str(
        Output,
        ( material != null_handle ?
            GfxRenderer.Material( material ).name :
            "" ) );
    lighting.serialize( Output );
    // geometry
    sn_utils::s_dvec3( Output, origin );
    // NOTE: geometry handle is created dynamically on load
    // vertex count, followed by vertex data
    sn_utils::ls_uint32( Output, vertices.size() );
    for( auto const &vertex : vertices ) {
        gfx::basic_vertex(
            glm::vec3{ vertex.position - origin },
            vertex.normal,
            vertex.texture )
                .serialize( Output );
    }
}

// restores content of the struct from provided input stream
void
shape_node::shapenode_data::deserialize( std::istream &Input ) {
    // bounding area
    area.deserialize( Input );
    // visibility
    rangesquared_min = sn_utils::ld_float64( Input );
    rangesquared_max = sn_utils::ld_float64( Input );
    visible = sn_utils::d_bool( Input );
    // material
    translucent = sn_utils::d_bool( Input );
    auto const materialname { sn_utils::d_str( Input ) };
    if( false == materialname.empty() ) {
        material = GfxRenderer.Fetch_Material( materialname );
    }
    lighting.deserialize( Input );
    // geometry
    origin = sn_utils::d_dvec3( Input );
    // NOTE: geometry handle is acquired during geometry creation
    // vertex data
    vertices.resize( sn_utils::ld_uint32( Input ) );
    gfx::basic_vertex localvertex;
    for( auto &vertex : vertices ) {
        localvertex.deserialize( Input );
        vertex.position = origin + glm::dvec3{ localvertex.position };
        vertex.normal = localvertex.normal;
        vertex.texture = localvertex.texture;
    }
}


// sends content of the class to provided stream
void
shape_node::serialize( std::ostream &Output ) const {
    // name
    sn_utils::s_str( Output, m_name );
    // node data
    m_data.serialize( Output );
}

// restores content of the node from provided input stream
shape_node &
shape_node::deserialize( std::istream &Input ) {
    // name
    m_name = sn_utils::d_str( Input );
    // node data
    m_data.deserialize( Input );

    return *this;
}

// restores content of the node from provided input stream
shape_node &
shape_node::import( cParser &Input, scene::node_data const &Nodedata ) {

    // import common data
    m_name = Nodedata.name;
    m_data.rangesquared_min = Nodedata.range_min * Nodedata.range_min;
    m_data.rangesquared_max = (
        Nodedata.range_max >= 0.0 ?
            Nodedata.range_max * Nodedata.range_max :
            std::numeric_limits<double>::max() );

    std::string token = Input.getToken<std::string>();
    if( token == "material" ) {
        // lighting settings
        token = Input.getToken<std::string>();
        while( token != "endmaterial" ) {

            if( token == "ambient:" ) {
                Input.getTokens( 3 );
                Input
                    >> m_data.lighting.ambient.r
                    >> m_data.lighting.ambient.g
                    >> m_data.lighting.ambient.b;
                m_data.lighting.ambient /= 255.f;
                m_data.lighting.ambient.a = 1.f;
            }
            else if( token == "diffuse:" ) {
                Input.getTokens( 3 );
                Input
                    >> m_data.lighting.diffuse.r
                    >> m_data.lighting.diffuse.g
                    >> m_data.lighting.diffuse.b;
                m_data.lighting.diffuse /= 255.f;
                m_data.lighting.diffuse.a = 1.f;
            }
            else if( token == "specular:" ) {
                Input.getTokens( 3 );
                Input
                    >> m_data.lighting.specular.r
                    >> m_data.lighting.specular.g
                    >> m_data.lighting.specular.b;
                m_data.lighting.specular /= 255.f;
                m_data.lighting.specular.a = 1.f;
            }
            token = Input.getToken<std::string>();
        }
        token = Input.getToken<std::string>();
    }

    // assigned material
    m_data.material = GfxRenderer.Fetch_Material( token );

    // determine way to proceed from the assigned diffuse texture
    // TBT, TODO: add methods to material manager to access these simpler
    auto const texturehandle = (
        m_data.material != null_handle ?
            GfxRenderer.Material( m_data.material ).textures[0] :
            null_handle );
    auto const &texture = (
        texturehandle ?
            GfxRenderer.Texture( texturehandle ) :
            opengl_texture() ); // dirty workaround for lack of better api
    bool const clamps = (
        texturehandle ?
            texture.traits.find( 's' ) != std::string::npos :
            false );
    bool const clampt = (
        texturehandle ?
            texture.traits.find( 't' ) != std::string::npos :
            false );

    // remainder of legacy 'problend' system -- geometry assigned a texture with '@' in its name is treated as translucent, opaque otherwise
    if( texturehandle != null_handle ) {
        m_data.translucent = (
            ( ( texture.name.find( '@' ) != std::string::npos )
           && ( true == texture.has_alpha ) ) ?
                true :
                false );
    }
    else {
        m_data.translucent = false;
    }

    // geometry
    enum subtype {
        triangles,
        triangle_strip,
        triangle_fan
    };

    subtype const nodetype = (
        Nodedata.type == "triangles" ?      triangles :
        Nodedata.type == "triangle_strip" ? triangle_strip :
                                            triangle_fan );
    std::size_t vertexcount{ 0 };
    world_vertex vertex, vertex1, vertex2;
    do {
        Input.getTokens( 8, false );
        Input
            >> vertex.position.x
            >> vertex.position.y
            >> vertex.position.z
            >> vertex.normal.x
            >> vertex.normal.y
            >> vertex.normal.z
            >> vertex.texture.s
            >> vertex.texture.t;
        // clamp texture coordinates if texture wrapping is off
        if( true == clamps ) { vertex.texture.s = clamp( vertex.texture.s, 0.001f, 0.999f ); }
        if( true == clampt ) { vertex.texture.t = clamp( vertex.texture.t, 0.001f, 0.999f ); }
        // convert all data to gl_triangles to allow data merge for matching nodes
        switch( nodetype ) {
            case triangles: {

                     if( vertexcount == 0 ) { vertex1 = vertex; }
                else if( vertexcount == 1 ) { vertex2 = vertex; }
                else if( vertexcount >= 2 ) {
                    if( false == degenerate( vertex1.position, vertex2.position, vertex.position ) ) {
                        m_data.vertices.emplace_back( vertex1 );
                        m_data.vertices.emplace_back( vertex2 );
                        m_data.vertices.emplace_back( vertex );
                    }
                    else {
                        ErrorLog(
                            "Bad geometry: degenerate triangle encountered"
                            + ( m_name != "" ? " in node \"" + m_name + "\"" : "" )
                            + " (vertices: " + to_string( vertex1.position ) + " + " + to_string( vertex2.position ) + " + " + to_string( vertex.position ) + ")" );
                    }
                }
                ++vertexcount;
                if( vertexcount > 2 ) { vertexcount = 0; } // start new triangle if needed
                break;
            }
            case triangle_fan: {

                     if( vertexcount == 0 ) { vertex1 = vertex; }
                else if( vertexcount == 1 ) { vertex2 = vertex; }
                else if( vertexcount >= 2 ) {
                    if( false == degenerate( vertex1.position, vertex2.position, vertex.position ) ) {
                        m_data.vertices.emplace_back( vertex1 );
                        m_data.vertices.emplace_back( vertex2 );
                        m_data.vertices.emplace_back( vertex );
                        vertex2 = vertex;
                    }
                    else {
                        ErrorLog(
                            "Bad geometry: degenerate triangle encountered"
                            + ( m_name != "" ? " in node \"" + m_name + "\"" : "" )
                            + " (vertices: " + to_string( vertex1.position ) + " + " + to_string( vertex2.position ) + " + " + to_string( vertex.position ) + ")" );
                    }
                }
                ++vertexcount;
                break;
            }
            case triangle_strip: {

                     if( vertexcount == 0 ) { vertex1 = vertex; }
                else if( vertexcount == 1 ) { vertex2 = vertex; }
                else if( vertexcount >= 2 ) {
                    if( false == degenerate( vertex1.position, vertex2.position, vertex.position ) ) {
                        // swap order every other triangle, to maintain consistent winding
                        if( vertexcount % 2 == 0 ) {
                            m_data.vertices.emplace_back( vertex1 );
                            m_data.vertices.emplace_back( vertex2 );
                        }
                        else {
                            m_data.vertices.emplace_back( vertex2 );
                            m_data.vertices.emplace_back( vertex1 );
                        }
                        m_data.vertices.emplace_back( vertex );

                        vertex1 = vertex2;
                        vertex2 = vertex;
                    }
                    else {
                        ErrorLog(
                            "Bad geometry: degenerate triangle encountered"
                            + ( m_name != "" ? " in node \"" + m_name + "\"" : "" )
                            + " (vertices: " + to_string( vertex1.position ) + " + " + to_string( vertex2.position ) + " + " + to_string( vertex.position ) + ")" );
                    }
                }
                ++vertexcount;
                break;
            }
            default: { break; }
        }
        token = Input.getToken<std::string>();

    } while( token != "endtri" );

    return *this;
}

// imports data from provided submodel
shape_node &
shape_node::convert( TSubModel const *Submodel ) {

    m_name = Submodel->pName;
    m_data.lighting.ambient = Submodel->f4Ambient;
    m_data.lighting.diffuse = Submodel->f4Diffuse;
    m_data.lighting.specular = Submodel->f4Specular;
    m_data.material = Submodel->m_material;
    m_data.translucent = ( GfxRenderer.Material( m_data.material ).get_or_guess_opacity() == 0.0f );
    // NOTE: we set unlimited view range typical for terrain, because we don't expect to convert any other 3d models
    m_data.rangesquared_max = std::numeric_limits<double>::max();

    if( Submodel->m_geometry == null_handle ) { return *this; }

    int vertexcount { 0 };
    std::vector<world_vertex> importedvertices;
    world_vertex vertex, vertex1, vertex2;
    for( auto const &sourcevertex : GfxRenderer.Vertices( Submodel->m_geometry ) ) {
        vertex.position = sourcevertex.position;
        vertex.normal   = sourcevertex.normal;
        vertex.texture  = sourcevertex.texture;
             if( vertexcount == 0 ) { vertex1 = vertex; }
        else if( vertexcount == 1 ) { vertex2 = vertex; }
        else if( vertexcount >= 2 ) {
            if( false == degenerate( vertex1.position, vertex2.position, vertex.position ) ) {
                importedvertices.emplace_back( vertex1 );
                importedvertices.emplace_back( vertex2 );
                importedvertices.emplace_back( vertex );
            }
            // start a new triangle
            vertexcount = -1;
        }
        ++vertexcount;
    }

    if( true == importedvertices.empty() ) { return *this; }

    // assign imported geometry to the node...
    m_data.vertices.swap( importedvertices );
    // ...and calculate center...
    for( auto const &vertex : m_data.vertices ) {
        m_data.area.center += vertex.position;
    }
    m_data.area.center /= m_data.vertices.size();
    // ...and bounding area
    double squareradius { 0.0 };
    for( auto const &vertex : m_data.vertices ) {
        squareradius = std::max(
            squareradius,
            glm::length2( vertex.position - m_data.area.center ) );
    }
    m_data.area.radius = std::max(
        m_data.area.radius,
        static_cast<float>( std::sqrt( squareradius ) ) );

    return *this;
}

// adds content of provided node to already enclosed geometry. returns: true if merge could be performed
bool
shape_node::merge( shape_node &Shape ) {

    if( ( m_data.material != Shape.m_data.material )
     || ( m_data.lighting != Shape.m_data.lighting ) ) {
        // can't merge nodes with different appearance
        return false;
    }
    // add geometry from provided node
    m_data.area.center =
        interpolate(
            m_data.area.center, Shape.m_data.area.center,
            static_cast<double>( Shape.m_data.vertices.size() ) / ( Shape.m_data.vertices.size() + m_data.vertices.size() ) );
    m_data.vertices.insert(
        std::end( m_data.vertices ),
        std::begin( Shape.m_data.vertices ), std::end( Shape.m_data.vertices ) );
	invalidate_radius();

    return true;
}

// generates renderable version of held non-instanced geometry in specified geometry bank
void
shape_node::create_geometry( gfx::geometrybank_handle const &Bank ) {

    gfx::vertex_array vertices; vertices.reserve( m_data.vertices.size() );

    for( auto const &vertex : m_data.vertices ) {
        vertices.emplace_back(
            vertex.position - m_data.origin,
            vertex.normal,
            vertex.texture );
    }
    m_data.geometry = GfxRenderer.Insert( vertices, Bank, GL_TRIANGLES );
    std::vector<world_vertex>().swap( m_data.vertices ); // hipster shrink_to_fit
}

// calculates shape's bounding radius
void
shape_node::compute_radius() {

    auto squaredradius { 0.0 };
    for( auto const &vertex : m_data.vertices ) {
        squaredradius = std::max(
            squaredradius,
            glm::length2( vertex.position - m_data.area.center ) );
    }
    m_data.area.radius = static_cast<float>( std::sqrt( squaredradius ) );
}

void shape_node::invalidate_radius() {
	m_data.area.radius = -1.0f;
}

float shape_node::radius() {
	if (m_data.area.radius == -1.0f)
		compute_radius();

	return m_data.area.radius;
}

// sends content of the struct to provided stream
void
lines_node::linesnode_data::serialize( std::ostream &Output ) const {
    // bounding area
    area.serialize( Output );
    // visibility
    sn_utils::ls_float64( Output, rangesquared_min );
    sn_utils::ls_float64( Output, rangesquared_max );
    sn_utils::s_bool( Output, visible );
    // material
    sn_utils::ls_float32( Output, line_width );
    lighting.serialize( Output );
    // geometry
    sn_utils::s_dvec3( Output, origin );
    // NOTE: geometry handle is created dynamically on load
    // vertex count, followed by vertex data
    sn_utils::ls_uint32( Output, vertices.size() );
    for( auto const &vertex : vertices ) {
        gfx::basic_vertex(
            glm::vec3{ vertex.position - origin },
            vertex.normal,
            vertex.texture )
                .serialize( Output );
    }
}

// restores content of the struct from provided input stream
void
lines_node::linesnode_data::deserialize( std::istream &Input ) {
    // bounding area
    area.deserialize( Input );
    // visibility
    rangesquared_min = sn_utils::ld_float64( Input );
    rangesquared_max = sn_utils::ld_float64( Input );
    visible = sn_utils::d_bool( Input );
    // material
    line_width = sn_utils::ld_float32( Input );
    lighting.deserialize( Input );
    // geometry
    origin = sn_utils::d_dvec3( Input );
    // NOTE: geometry handle is acquired during geometry creation
    // vertex data
    vertices.resize( sn_utils::ld_uint32( Input ) );
    gfx::basic_vertex localvertex;
    for( auto &vertex : vertices ) {
        localvertex.deserialize( Input );
        vertex.position = origin + glm::dvec3{ localvertex.position };
        vertex.normal = localvertex.normal;
        vertex.texture = localvertex.texture;
    }
}



// sends content of the class to provided stream
void
lines_node::serialize( std::ostream &Output ) const {
    // name
    sn_utils::s_str( Output, m_name );
    // node data
    m_data.serialize( Output );
}

// restores content of the node from provided input stream
lines_node &
lines_node::deserialize( std::istream &Input ) {
    // name
    m_name = sn_utils::d_str( Input );
    // node data
    m_data.deserialize( Input );

    return *this;
}

// restores content of the node from provded input stream
lines_node &
lines_node::import( cParser &Input, scene::node_data const &Nodedata ) {

    // import common data
    m_name = Nodedata.name;
    m_data.rangesquared_min = Nodedata.range_min * Nodedata.range_min;
    m_data.rangesquared_max = (
        Nodedata.range_max >= 0.0 ?
            Nodedata.range_max * Nodedata.range_max :
            std::numeric_limits<double>::max() );

    // material
    Input.getTokens( 3, false );
    Input
        >> m_data.lighting.diffuse.r
        >> m_data.lighting.diffuse.g
        >> m_data.lighting.diffuse.b;
    m_data.lighting.diffuse /= 255.f;
    m_data.lighting.diffuse.a = 1.f;
    Input.getTokens( 1, false );
    Input
        >> m_data.line_width;
    m_data.line_width = std::min( 30.f, m_data.line_width ); // 30 pix equals rougly width of a signal pole viewed from ~1m away

    // geometry
    enum subtype {
        lines,
        line_strip,
        line_loop
    };
    
    subtype const nodetype = (
        Nodedata.type == "lines" ?      lines :
        Nodedata.type == "line_strip" ? line_strip :
                                        line_loop );
    std::size_t vertexcount { 0 };
    world_vertex vertex, vertex0, vertex1;
    std::string token = Input.getToken<std::string>();
    do {
        vertex.position.x = std::atof( token.c_str() );
        Input.getTokens( 2, false );
        Input
            >> vertex.position.y
            >> vertex.position.z;
            // convert all data to gl_lines to allow data merge for matching nodes
            switch( nodetype ) {
                case lines: {
                    m_data.vertices.emplace_back( vertex );
                    break;
                }
                case line_strip: {
                    if( vertexcount > 0 ) {
                        m_data.vertices.emplace_back( vertex1 );
                        m_data.vertices.emplace_back( vertex );
                    }
                    vertex1 = vertex;
                    ++vertexcount;
                    break;
                }
                case line_loop: {
                    if( vertexcount == 0 ) {
                        vertex0 = vertex;
                        vertex1 = vertex;
                    }
                    else {
                        m_data.vertices.emplace_back( vertex1 );
                        m_data.vertices.emplace_back( vertex );
                    }
                    vertex1 = vertex;
                    ++vertexcount;
                    break;
                }
                default: { break; }
            }
        token = Input.getToken<std::string>();

    } while( token != "endline" );
    // add closing line for the loop
    if( ( nodetype == line_loop )
     && ( vertexcount > 2 ) ) {
        m_data.vertices.emplace_back( vertex1 );
        m_data.vertices.emplace_back( vertex0 );
    }
    if( m_data.vertices.size() % 2 != 0 ) {
        ErrorLog( "Lines node specified odd number of vertices, defined in file \"" + Input.Name() + "\" (line " + std::to_string( Input.Line() - 1 ) + ")" );
        m_data.vertices.pop_back();
    }

    return *this;
}

// adds content of provided node to already enclosed geometry. returns: true if merge could be performed
bool
lines_node::merge( lines_node &Lines ) {

    if( ( m_data.line_width != Lines.m_data.line_width )
     || ( m_data.lighting != Lines.m_data.lighting ) ) {
        // can't merge nodes with different appearance
        return false;
    }
    // add geometry from provided node
    m_data.area.center =
        interpolate(
            m_data.area.center, Lines.m_data.area.center,
            static_cast<double>( Lines.m_data.vertices.size() ) / ( Lines.m_data.vertices.size() + m_data.vertices.size() ) );
    m_data.vertices.insert(
        std::end( m_data.vertices ),
        std::begin( Lines.m_data.vertices ), std::end( Lines.m_data.vertices ) );
    // NOTE: we could recalculate radius with something other than brute force, but it'll do
    compute_radius();

    return true;
}

// generates renderable version of held non-instanced geometry in specified geometry bank
void
lines_node::create_geometry( gfx::geometrybank_handle const &Bank ) {

    gfx::vertex_array vertices; vertices.reserve( m_data.vertices.size() );

    for( auto const &vertex : m_data.vertices ) {
        vertices.emplace_back(
            vertex.position - m_data.origin,
            vertex.normal,
            vertex.texture );
    }
    m_data.geometry = GfxRenderer.Insert( vertices, Bank, GL_LINES );
    std::vector<world_vertex>().swap( m_data.vertices ); // hipster shrink_to_fit
}

// calculates node's bounding radius
void
lines_node::compute_radius() {

    auto squaredradius { 0.0 };
    for( auto const &vertex : m_data.vertices ) {
        squaredradius = std::max(
            squaredradius,
            glm::length2( vertex.position - m_data.area.center ) );
    }
    m_data.area.radius = static_cast<float>( std::sqrt( squaredradius ) );
}



/*
memory_node &
memory_node::deserialize( cParser &Input, node_data const &Nodedata ) {

    // import common data
    m_name = Nodedata.name;

    Input.getTokens( 3 );
    Input
        >> m_data.area.center.x
        >> m_data.area.center.y
        >> m_data.area.center.z;

    TMemCell memorycell( Nodedata.name );
    memorycell.Load( &Input );
}
*/



basic_node::basic_node( scene::node_data const &Nodedata ) :
    m_name( Nodedata.name )
{
    m_rangesquaredmin = Nodedata.range_min * Nodedata.range_min;
    m_rangesquaredmax = (
        Nodedata.range_max >= 0.0 ?
            Nodedata.range_max * Nodedata.range_max :
            std::numeric_limits<double>::max() );
}

// sends content of the class to provided stream
void
basic_node::serialize( std::ostream &Output ) const {
    // bounding area
    m_area.serialize( Output );
    // visibility
    sn_utils::ls_float64( Output, m_rangesquaredmin );
    sn_utils::ls_float64( Output, m_rangesquaredmax );
    sn_utils::s_bool( Output, m_visible );
    // name
    sn_utils::s_str( Output, m_name );
    // template method implementation
    serialize_( Output );
}

// restores content of the class from provided stream
void
basic_node::deserialize( std::istream &Input ) {
    // bounding area
    m_area.deserialize( Input );
    // visibility
    m_rangesquaredmin = sn_utils::ld_float64( Input );
    m_rangesquaredmax = sn_utils::ld_float64( Input );
    m_visible = sn_utils::d_bool( Input );
    // name
    m_name = sn_utils::d_str( Input );
    // template method implementation
    deserialize_( Input );
}

// sends basic content of the class in legacy (text) format to provided stream
void
basic_node::export_as_text( std::ostream &Output ) const {

    Output
        // header
        << "node"
        // visibility
        << ' ' << ( m_rangesquaredmax < std::numeric_limits<double>::max() ? std::sqrt( m_rangesquaredmax ) : -1 )
        << ' ' << std::sqrt( m_rangesquaredmin )
        // name
        << ' ' << m_name << ' ';
    // template method implementation
    export_as_text_( Output );
}

void
basic_node::export_as_text( std::string &Output ) const {

    std::stringstream converter;
    export_as_text( converter );
    Output += converter.str();
}

float const &
basic_node::radius() {

    if( m_area.radius == -1.0 ) {
        // calculate if needed
        m_area.radius = radius_();
    }
    return m_area.radius;
}

// radius() subclass details, calculates node's bounding radius
// by default nodes are 'virtual don't extend from their center point
float
basic_node::radius_() {
    
    return 0.f;
}

} // scene

//---------------------------------------------------------------------------
