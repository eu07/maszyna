/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "scene.h"

#include "renderer.h"
#include "logs.h"

namespace scene {

// adds provided shape to the cell
void
basic_cell::insert( shape_node Shape ) {

    m_active = true;

    auto const &shapedata { Shape.data() };
    auto &shapes = (
        shapedata.translucent ?
            m_shapestranslucent :
            m_shapesopaque );
    for( auto &targetshape : shapes ) {
        // try to merge shapes with matching view ranges...
        auto const &targetshapedata { targetshape.data() };
        if( ( shapedata.rangesquared_min == targetshapedata.rangesquared_min )
         && ( shapedata.rangesquared_max == targetshapedata.rangesquared_max )
        // ...and located close to each other (within arbitrary limit of 25m)
         && ( glm::length( shapedata.area.center - targetshapedata.area.center ) < 25.0 ) ) {

            if( true == targetshape.merge( Shape ) ) {
                // if the shape was merged there's nothing left to do
                return;
            }
        }
    }
    // otherwise add the shape to the relevant list
    Shape.origin( m_area.center );
    shapes.emplace_back( Shape );
}

// adds provided path to the cell
void
basic_cell::insert( TTrack *Path ) {

    m_active = true;

    // TODO: add animation hook
    Path->origin( m_area.center );
    m_paths.emplace_back( Path );
}

// adds provided traction piece to the cell
void
basic_cell::insert( TTraction *Traction ) {

    m_active = true;

    Traction->origin( m_area.center );
    m_traction.emplace_back( Traction );
}

// adds provided model instance to the cell
void
basic_cell::insert( TAnimModel *Instance ) {

    m_active = true;

    auto const flags = Instance->Flags();
    auto alpha =
        ( Instance->Material() != nullptr ?
            Instance->Material()->textures_alpha :
            0x30300030 );

    // assign model to appropriate render phases
    if( alpha & flags & 0x2F2F002F ) {
        // translucent pieces
        m_instancetranslucent.emplace_back( Instance );
    }
    alpha ^= 0x0F0F000F; // odwrócenie flag tekstur, aby wyłapać nieprzezroczyste
    if( alpha & flags & 0x1F1F001F ) {
        // opaque pieces
        m_instancesopaque.emplace_back( Instance );
    }
}

// generates renderable version of held non-instanced geometry
void
basic_cell::create_geometry( geometrybank_handle const &Bank ) {

    if( false == m_active ) { return; } // nothing to do here

    for( auto &shape : m_shapesopaque )      { shape.create_geometry( Bank ); }
    for( auto &shape : m_shapestranslucent ) { shape.create_geometry( Bank ); }
#ifndef EU07_USE_OLD_GROUNDCODE
    for( auto *path : m_paths )              { path->create_geometry( Bank ); }
    for( auto *traction : m_traction )       { traction->create_geometry( Bank ); }
#endif
}

// sets center point of the section
void
basic_cell::center( glm::dvec3 Center ) {

    m_area.center = Center;
    // NOTE: we should also update origin point for the contained nodes, but in practice we can skip this
    // as all nodes will be added only after the proper center point was set, and won't change
}



// adds provided shape to the section
void
basic_section::insert( shape_node Shape ) {

    m_active = true;

    auto const &shapedata = Shape.data();
    if( ( true == shapedata.translucent )
     || ( shapedata.rangesquared_max <= 90000.0 )
     || ( shapedata.rangesquared_min > 0.0 ) ) {
        // small, translucent or not always visible shapes are placed in the sub-cells
        cell( shapedata.area.center ).insert( Shape );
    }
    else {
        // large, opaque shapes are placed on section level
        for( auto &shape : m_shapes ) {
            // check first if the shape can't be merged with one of the shapes already present in the section
            if( true == shape.merge( Shape ) ) {
                // if the shape was merged there's nothing left to do
                return;
            }
        }
        // otherwise add the shape to the section's list
        Shape.origin( m_area.center );
        m_shapes.emplace_back( Shape );
    }
}

// adds provided path to the section
void
basic_section::insert( TTrack *Path ) {

    m_active = true;
    // pass the node to the appropriate partitioning cell
    // NOTE: bounding area isn't present/filled until track class and wrapper refactoring is done
    cell( Path->location() ).insert( Path );
}

// adds provided path to the section
void
basic_section::insert( TTraction *Traction ) {

    m_active = true;
    // pass the node to the appropriate partitioning cell
    // NOTE: bounding area isn't present/filled until track class and wrapper refactoring is done
    cell( Traction->location() ).insert( Traction );
}

// adds provided model instance to the section
void
basic_section::insert( TAnimModel *Instance ) {

    m_active = true;
    // pass the node to the appropriate partitioning cell
    // NOTE: bounding area isn't present/filled until track class and wrapper refactoring is done
    cell( Instance->location() ).insert( Instance );
}

// sets center point of the section
void
basic_section::center( glm::dvec3 Center ) {

    m_area.center = Center;
    // set accordingly center points of the section's partitioning cells
    // NOTE: we should also update origin point for the contained nodes, but in practice we can skip this
    // as all nodes will be added only after the proper center point was set, and won't change
    auto const centeroffset = -( EU07_SECTIONSIZE / EU07_CELLSIZE / 2 * EU07_CELLSIZE ) + EU07_CELLSIZE / 2;
    glm::dvec3 sectioncornercenter { m_area.center + glm::dvec3{ centeroffset, 0, centeroffset } };
    auto row { 0 }, column { 0 };
    for( auto &cell : m_cells ) {
        cell.center( sectioncornercenter + glm::dvec3{ column * EU07_CELLSIZE, 0.0, row * EU07_CELLSIZE } );
        if( ++column >= EU07_SECTIONSIZE / EU07_CELLSIZE ) {
            ++row;
            column = 0;
        }
    }
}

// generates renderable version of held non-instanced geometry
void
basic_section::create_geometry() {

    if( true == m_geometrycreated ) { return; }
    else {
        // mark it done for future checks
        m_geometrycreated = true;
    }

    if( false == m_active ) { return; } // nothing to do here

    // since sections can be empty, we're doing lazy initialization of the geometry bank, when something may actually use it
    if( m_geometrybank == null_handle ) {
        m_geometrybank = GfxRenderer.Create_Bank();
    }

    for( auto &shape : m_shapes ) {
        shape.create_geometry( m_geometrybank );
    }
    for( auto &cell : m_cells ) {
        cell.create_geometry( m_geometrybank );
    }
}

// provides access to section enclosing specified point
basic_cell &
basic_section::cell( glm::dvec3 const &Location ) {

    auto const column = static_cast<int>( std::floor( ( Location.x - ( m_area.center.x - EU07_SECTIONSIZE / 2 ) ) / EU07_CELLSIZE ) );
    auto const row = static_cast<int>( std::floor( ( Location.z - ( m_area.center.z - EU07_SECTIONSIZE / 2 ) ) / EU07_CELLSIZE ) );

    return
        m_cells[
              clamp( row,    0, ( EU07_SECTIONSIZE / EU07_CELLSIZE ) - 1 ) * ( EU07_SECTIONSIZE / EU07_CELLSIZE )
            + clamp( column, 0, ( EU07_SECTIONSIZE / EU07_CELLSIZE ) - 1 ) ] ;
}



basic_region::basic_region() {

    // initialize centers of sections:
    // calculate center of 'top left' region section...
    auto const centeroffset = -( EU07_REGIONSIDESECTIONCOUNT / 2 * EU07_SECTIONSIZE ) + EU07_SECTIONSIZE / 2;
    glm::dvec3 regioncornercenter { centeroffset, 0, centeroffset };
    auto row { 0 }, column { 0 };
    // ...move through section array assigning centers left to right, front/top to back/bottom
    for( auto &section : m_sections ) {
        section.center( regioncornercenter + glm::dvec3{ column * EU07_SECTIONSIZE, 0.0, row * EU07_SECTIONSIZE } );
        if( ++column >= EU07_REGIONSIDESECTIONCOUNT ) {
            ++row;
            column = 0;
        }
    }
}

void
basic_region::insert_shape( shape_node Shape, scratch_data &Scratchpad ) {

    // shape might need to be split into smaller pieces, so we create list of nodes instead of just single one
    // using deque so we can do single pass iterating and addding generated pieces without invalidating anything
    std::deque<shape_node> shapes { Shape };
    auto &shape = shapes.front();
    if( shape.m_data.vertices.empty() ) { return; }

    // adjust input if necessary:
    if( Scratchpad.location_rotation != glm::vec3( 0, 0, 0 ) ) {
        // rotate...
        auto const rotation = glm::radians( Scratchpad.location_rotation );
        for( auto &vertex : shape.m_data.vertices ) {
            vertex.position = glm::rotateZ<double>( vertex.position, rotation.z );
            vertex.position = glm::rotateX<double>( vertex.position, rotation.x );
            vertex.position = glm::rotateY<double>( vertex.position, rotation.y );
            vertex.normal = glm::rotateZ( vertex.normal, rotation.z );
            vertex.normal = glm::rotateX( vertex.normal, rotation.x );
            vertex.normal = glm::rotateY( vertex.normal, rotation.y );
        }
    }
    if( ( false == Scratchpad.location_offset.empty() )
     && ( Scratchpad.location_offset.top() != glm::dvec3( 0, 0, 0 ) ) ) {
        // ...and move
        auto const offset = Scratchpad.location_offset.top();
        for( auto &vertex : shape.m_data.vertices ) {
            vertex.position += offset;
        }
    }
    // calculate bounding area
    for( auto const &vertex : shape.m_data.vertices ) {
        shape.m_data.area.center += vertex.position;
    }
    shape.m_data.area.center /= shape.m_data.vertices.size();
    // trim the shape if needed. trimmed parts will be added to list as separate nodes
    for( std::size_t index = 0; index < shapes.size(); ++index ) {
        while( true == RaTriangleDivider( shapes[ index ], shapes ) ) {
            ; // all work is done during expression check
        }
        // with the trimming done we can calculate shape's bounding radius
        shape.compute_radius();
    }
    // move the data into appropriate section(s)
    for( auto &shape : shapes ) {

        if( point_inside( shape.m_data.area.center ) ) {
            // NOTE: nodes placed outside of region boundaries are discarded
            section( shape.m_data.area.center ).insert( shape );
        }
        else {
            ErrorLog(
                "Bad scenario: shape node" + (
                    shape.m_name.empty() ?
                        "" :
                        " \"" + shape.m_name + "\"" )
                + " placed in location outside region bounds (" + to_string( shape.m_data.area.center ) + ")" );
        }
    }
}

// inserts provided track in the region
void
basic_region::insert_path( TTrack *Path, scratch_data &Scratchpad ) {

    // NOTE: bounding area isn't present/filled until track class and wrapper refactoring is done
    auto center = Path->location();

    if( point_inside( center ) ) {
        // NOTE: nodes placed outside of region boundaries are discarded
        section( center ).insert( Path );
    }
    else {
        // tracks are guaranteed to hava a name so we can skip the check
        ErrorLog( "Bad scenario: track node \"" + Path->name() + "\" placed in location outside region bounds (" + to_string( center ) + ")" );
    }
}

// inserts provided track in the region
void
basic_region::insert_traction( TTraction *Traction, scratch_data &Scratchpad ) {

    // NOTE: bounding area isn't present/filled until track class and wrapper refactoring is done
    auto center = Traction->location();

    if( point_inside( center ) ) {
        // NOTE: nodes placed outside of region boundaries are discarded
        section( center ).insert( Traction );
    }
    else {
        // tracks are guaranteed to hava a name so we can skip the check
        ErrorLog( "Bad scenario: traction node \"" + Traction->name() + "\" placed in location outside region bounds (" + to_string( center ) + ")" );
    }
}

// inserts provided instance of 3d model in the region
void
basic_region::insert_instance( TAnimModel *Instance, scratch_data &Scratchpad ) {

    // NOTE: bounding area isn't present/filled until track class and wrapper refactoring is done
    auto center = Instance->location();

    if( point_inside( center ) ) {
        // NOTE: nodes placed outside of region boundaries are discarded
        section( center ).insert( Instance );
    }
    else {
        // tracks are guaranteed to hava a name so we can skip the check
        ErrorLog( "Bad scenario: model node \"" + Instance->name() + "\" placed in location outside region bounds (" + to_string( center ) + ")" );
    }
}

// checks whether specified point is within boundaries of the region
bool
basic_region::point_inside( glm::dvec3 const &Location ) {

    double const regionboundary = EU07_REGIONSIDESECTIONCOUNT / 2 * EU07_SECTIONSIZE;
    return ( ( Location.x > -regionboundary ) && ( Location.x < regionboundary )
          && ( Location.z > -regionboundary ) && ( Location.z < regionboundary ) );
}

// trims provided shape to fit into a section, adds trimmed part at the end of provided list
// NOTE: legacy function. TBD, TODO: clean it up?
bool
basic_region::RaTriangleDivider( shape_node &Shape, std::deque<shape_node> &Shapes ) {

    if( Shape.m_data.vertices.size() != 3 ) {
        // tylko gdy jeden trójkąt
        return false;
    }

    auto const margin { 200.0 };
    auto x0 = EU07_SECTIONSIZE * std::floor( 0.001 * Shape.m_data.area.center.x ) - margin;
    auto x1 = x0 + EU07_SECTIONSIZE + margin * 2;
    auto z0 = EU07_SECTIONSIZE * std::floor( 0.001 * Shape.m_data.area.center.z ) - margin;
    auto z1 = z0 + EU07_SECTIONSIZE + margin * 2;

    if( ( Shape.m_data.vertices[ 0 ].position.x >= x0 ) && ( Shape.m_data.vertices[ 0 ].position.x <= x1 )
     && ( Shape.m_data.vertices[ 0 ].position.z >= z0 ) && ( Shape.m_data.vertices[ 0 ].position.z <= z1 )
     && ( Shape.m_data.vertices[ 1 ].position.x >= x0 ) && ( Shape.m_data.vertices[ 1 ].position.x <= x1 )
     && ( Shape.m_data.vertices[ 1 ].position.z >= z0 ) && ( Shape.m_data.vertices[ 1 ].position.z <= z1 )
     && ( Shape.m_data.vertices[ 2 ].position.x >= x0 ) && ( Shape.m_data.vertices[ 2 ].position.x <= x1 )
     && ( Shape.m_data.vertices[ 2 ].position.z >= z0 ) && ( Shape.m_data.vertices[ 2 ].position.z <= z1 ) ) {
        // trójkąt wystający mniej niż 200m z kw. kilometrowego jest do przyjęcia
        return false;
    }
    // Ra: przerobić na dzielenie na 2 trójkąty, podział w przecięciu z siatką kilometrową
    // Ra: i z rekurencją będzie dzielić trzy trójkąty, jeśli będzie taka potrzeba
    int divide { -1 }; // bok do podzielenia: 0=AB, 1=BC, 2=CA; +4=podział po OZ; +8 na x1/z1
    double
        min { 0.0 },
        mul; // jeśli przechodzi przez oś, iloczyn będzie ujemny
    x0 += margin;
    x1 -= margin; // przestawienie na siatkę
    z0 += margin;
    z1 -= margin;
    // AB na wschodzie
    mul = ( Shape.m_data.vertices[ 0 ].position.x - x0 ) * ( Shape.m_data.vertices[ 1 ].position.x - x0 );
    if( mul < min ) {
        min = mul;
        divide = 0;
    }
    // BC na wschodzie
    mul = ( Shape.m_data.vertices[ 1 ].position.x - x0 ) * ( Shape.m_data.vertices[ 2 ].position.x - x0 );
    if( mul < min ) {
        min = mul;
        divide = 1;
    }
    // CA na wschodzie
    mul = ( Shape.m_data.vertices[ 2 ].position.x - x0 ) * ( Shape.m_data.vertices[ 0 ].position.x - x0 );
    if( mul < min ) {
        min = mul;
        divide = 2;
    }
    // AB na zachodzie
    mul = ( Shape.m_data.vertices[ 0 ].position.x - x1 ) * ( Shape.m_data.vertices[ 1 ].position.x - x1 );
    if( mul < min ) {
        min = mul;
        divide = 8;
    }
    // BC na zachodzie
    mul = ( Shape.m_data.vertices[ 1 ].position.x - x1 ) * ( Shape.m_data.vertices[ 2 ].position.x - x1 );
    if( mul < min ) {
        min = mul;
        divide = 9;
    }
    // CA na zachodzie
    mul = ( Shape.m_data.vertices[ 2 ].position.x - x1 ) * ( Shape.m_data.vertices[ 0 ].position.x - x1 );
    if( mul < min ) {
        min = mul;
        divide = 10;
    }
    // AB na południu
    mul = ( Shape.m_data.vertices[ 0 ].position.z - z0 ) * ( Shape.m_data.vertices[ 1 ].position.z - z0 );
    if( mul < min ) {
        min = mul;
        divide = 4;
    }
    // BC na południu
    mul = ( Shape.m_data.vertices[ 1 ].position.z - z0 ) * ( Shape.m_data.vertices[ 2 ].position.z - z0 );
    if( mul < min ) {
        min = mul;
        divide = 5;
    }
    // CA na południu
    mul = ( Shape.m_data.vertices[ 2 ].position.z - z0 ) * ( Shape.m_data.vertices[ 0 ].position.z - z0 );
    if( mul < min ) {
        min = mul;
        divide = 6;
    }
    // AB na północy
    mul = ( Shape.m_data.vertices[ 0 ].position.z - z1 ) * ( Shape.m_data.vertices[ 1 ].position.z - z1 );
    if( mul < min ) {
        min = mul;
        divide = 12;
    }
    // BC na północy
    mul = ( Shape.m_data.vertices[ 1 ].position.z - z1 ) * ( Shape.m_data.vertices[ 2 ].position.z - z1 );
    if( mul < min ) {
        min = mul;
        divide = 13;
    }
    // CA na północy
    mul = (Shape.m_data.vertices[2].position.z - z1) * (Shape.m_data.vertices[0].position.z - z1);
    if( mul < min ) {
        divide = 14;
    }

    // tworzymy jeden dodatkowy trójkąt, dzieląc jeden bok na przecięciu siatki kilometrowej
    Shapes.emplace_back( Shape ); // copy current shape
    auto &newshape = Shapes.back();

    switch (divide & 3) {
        // podzielenie jednego z boków, powstaje wierzchołek D
        case 0: {
            // podział AB (0-1) -> ADC i DBC
            newshape.m_data.vertices[ 2 ] = Shape.m_data.vertices[ 2 ]; // wierzchołek C jest wspólny
            newshape.m_data.vertices[ 1 ] = Shape.m_data.vertices[ 1 ]; // wierzchołek B przechodzi do nowego
            if( divide & 4 ) {
                Shape.m_data.vertices[ 1 ].set_from_z(
                    Shape.m_data.vertices[ 0 ],
                    Shape.m_data.vertices[ 1 ],
                    ( ( divide & 8 ) ?
                        z1 :
                        z0 ) );
            }
            else {
                Shape.m_data.vertices[ 1 ].set_from_x(
                    Shape.m_data.vertices[ 0 ],
                    Shape.m_data.vertices[ 1 ],
                    ( ( divide & 8 ) ?
                        x1 :
                        x0 ) );
            }
            newshape.m_data.vertices[ 0 ] = Shape.m_data.vertices[ 1 ]; // wierzchołek D jest wspólny
            break;
        }
        case 1: {
            // podział BC (1-2) -> ABD i ADC
            newshape.m_data.vertices[ 0 ] = Shape.m_data.vertices[ 0 ]; // wierzchołek A jest wspólny
            newshape.m_data.vertices[ 2 ] = Shape.m_data.vertices[ 2 ]; // wierzchołek C przechodzi do nowego
            if( divide & 4 ) {
                Shape.m_data.vertices[ 2 ].set_from_z(
                    Shape.m_data.vertices[ 1 ],
                    Shape.m_data.vertices[ 2 ],
                    ( ( divide & 8 ) ?
                        z1 :
                        z0 ) );
            }
            else {
                Shape.m_data.vertices[ 2 ].set_from_x(
                    Shape.m_data.vertices[ 1 ],
                    Shape.m_data.vertices[ 2 ],
                    ( ( divide & 8 ) ?
                        x1 :
                        x0 ) );
            }
            newshape.m_data.vertices[ 1 ] = Shape.m_data.vertices[ 2 ]; // wierzchołek D jest wspólny
            break;
        }
        case 2: {
            // podział CA (2-0) -> ABD i DBC
            newshape.m_data.vertices[ 1 ] = Shape.m_data.vertices[ 1 ]; // wierzchołek B jest wspólny
            newshape.m_data.vertices[ 2 ] = Shape.m_data.vertices[ 2 ]; // wierzchołek C przechodzi do nowego
            if( divide & 4 ) {
                Shape.m_data.vertices[ 2 ].set_from_z(
                    Shape.m_data.vertices[ 2 ],
                    Shape.m_data.vertices[ 0 ],
                    ( ( divide & 8 ) ?
                        z1 :
                        z0 ) );
            }
            else {
                Shape.m_data.vertices[ 2 ].set_from_x(
                    Shape.m_data.vertices[ 2 ],
                    Shape.m_data.vertices[ 0 ],
                    ( ( divide & 8 ) ?
                        x1 :
                        x0 ) );
            }
            newshape.m_data.vertices[ 0 ] = Shape.m_data.vertices[ 2 ]; // wierzchołek D jest wspólny
            break;
        }
    }
    // przeliczenie środków ciężkości obu
       Shape.m_data.area.center = (    Shape.m_data.vertices[ 0 ].position +    Shape.m_data.vertices[ 1 ].position +    Shape.m_data.vertices[ 2 ].position ) / 3.0;
    newshape.m_data.area.center = ( newshape.m_data.vertices[ 0 ].position + newshape.m_data.vertices[ 1 ].position + newshape.m_data.vertices[ 2 ].position ) / 3.0;

    return true;
}

// provides access to section enclosing specified point
basic_section &
basic_region::section( glm::dvec3 const &Location ) {

    auto const column = static_cast<int>( std::floor( Location.x / EU07_SECTIONSIZE + EU07_REGIONSIDESECTIONCOUNT / 2 ) );
    auto const row = static_cast<int>( std::floor( Location.z / EU07_SECTIONSIZE + EU07_REGIONSIDESECTIONCOUNT / 2 ) );

    return
        m_sections[
              clamp( row,    0, EU07_REGIONSIDESECTIONCOUNT - 1 ) * EU07_REGIONSIDESECTIONCOUNT
            + clamp( column, 0, EU07_REGIONSIDESECTIONCOUNT - 1 ) ] ;
}

} // scene

//---------------------------------------------------------------------------
