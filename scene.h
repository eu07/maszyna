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
#include <deque>
#include <array>
#include <stack>

#include "parser.h"
#include "openglgeometrybank.h"
#include "scenenode.h"
#include "track.h"
#include "traction.h"

namespace scene {

int const EU07_CELLSIZE = 250;
int const EU07_SECTIONSIZE = 1000;
int const EU07_REGIONSIDESECTIONCOUNT = 500; // number of 1km sections along a side of square region

struct scratch_data {

    std::stack<glm::dvec3> location_offset;
    glm::vec3 location_rotation;

    struct trainset_data {

        std::string name;
        std::string track;
        float offset { 0.f };
        float velocity { 0.f };
        std::vector<TDynamicObject *> vehicles;
        std::vector<std::int8_t> couplings;
        TDynamicObject * driver { nullptr };
        bool is_open { false };
    } trainset;
};

// basic element of rudimentary partitioning scheme for the section. fixed size, no further subdivision
// TBD, TODO: replace with quadtree scheme?
class basic_cell {

    friend class opengl_renderer;

public:
// methods
    // legacy method, updates sounds and polls event launchers within radius around specified point
    void
        update();
    // adds provided shape to the cell
    void
        insert( shape_node Shape );
    // adds provided path to the cell
    void
        insert( TTrack *Path );
    // adds provided path to the cell
    void
        insert( TTraction *Traction );
    // adds provided model instance to the cell
    void
        insert( TAnimModel *Instance );
    // registers provided path in the lookup directory of the cell
    void
        register_end( TTrack *Path );
    // registers provided traction piece in the lookup directory of the cell
    void
        register_end( TTraction *Traction );
    // find a vehicle located nearest to specified point, within specified radius, optionally ignoring vehicles without drivers. reurns: located vehicle and distance
    std::tuple<TDynamicObject *, float>
        find( glm::dvec3 const &Point, float const Radius, bool const Onlycontrolled );
    // finds a path with one of its ends located in specified point. returns: located path and id of the matching endpoint
    std::tuple<TTrack *, int>
        find( glm::dvec3 const &Point, TTrack const *Exclude );
    // finds a traction piece with one of its ends located in specified point. returns: located traction piece and id of the matching endpoint
    std::tuple<TTraction *, int>
        find( glm::dvec3 const &Point, TTraction const *Exclude );
    // finds a traction piece located nearest to specified point, sharing section with specified other piece and powered in specified direction. returns: located traction piece
    std::tuple<TTraction *, int, float>
        find( glm::dvec3 const &Point, TTraction const *Other, int const Currentdirection );
    // sets center point of the cell
    void
        center( glm::dvec3 Center );
    // generates renderable version of held non-instanced geometry in specified geometry bank
    void
        create_geometry( geometrybank_handle const &Bank );
    // provides access to bounding area data
    bounding_area const &
        area() const {
            return m_area; }

private:
// types
    using shapenode_sequence = std::vector<shape_node>;
    using path_sequence = std::vector<TTrack *>;
    using path_set = std::set<TTrack *>;
    using traction_sequence = std::vector<TTraction *>;
    using traction_set = std::set<TTraction *>;
    using instance_sequence = std::vector<TAnimModel *>;
// members
    scene::bounding_area m_area { glm::dvec3(), static_cast<float>( 0.5 * M_SQRT2 * EU07_CELLSIZE + 0.25 * EU07_CELLSIZE ) };
    bool m_active { false }; // whether the cell holds any actual data
    // content
    shapenode_sequence m_shapesopaque; // opaque pieces of geometry
    shapenode_sequence m_shapestranslucent; // translucent pieces of geometry
    path_sequence m_paths; // path pieces
    instance_sequence m_instancesopaque;
    instance_sequence m_instancetranslucent;
    traction_sequence m_traction;
    // search helpers
    struct lookup_data {
        path_set paths;
        traction_set traction;
    } m_directories;
};

// basic scene partitioning structure, holds terrain geometry and collection of cells
class basic_section {

    friend class opengl_renderer;

public:
// methods
    // legacy method, updates sounds and polls event launchers within radius around specified point
    void
        update( glm::dvec3 const &Location, float const Radius );
    // adds provided shape to the section
    void
        insert( shape_node Shape );
    // adds provided path to the section
    void
        insert( TTrack *Path );
    // adds provided path to the section
    void
        insert( TTraction *Traction );
    // adds provided model instance to the section
    void
        insert( TAnimModel *Instance );
    // registers specified end point of the provided path in the lookup directory of the region
    void
        register_end( TTrack *Path, glm::dvec3 const &Point );
    // registers specified end point of the provided traction piece in the lookup directory of the region
    void
        register_end( TTraction *Traction, glm::dvec3 const &Point );
    // find a vehicle located nearest to specified point, within specified radius, optionally ignoring vehicles without drivers. reurns: located vehicle and distance
    std::tuple<TDynamicObject *, float>
        find( glm::dvec3 const &Point, float const Radius, bool const Onlycontrolled );
    // finds a path with one of its ends located in specified point. returns: located path and id of the matching endpoint
    std::tuple<TTrack *, int>
        find( glm::dvec3 const &Point, TTrack const *Exclude );
    // finds a traction piece with one of its ends located in specified point. returns: located traction piece and id of the matching endpoint
    std::tuple<TTraction *, int>
        find( glm::dvec3 const &Point, TTraction const *Exclude );
    // finds a traction piece located nearest to specified point, sharing section with specified other piece and powered in specified direction. returns: located traction piece
    std::tuple<TTraction *, int, float>
        find( glm::dvec3 const &Point, TTraction const *Other, int const Currentdirection );
    // sets center point of the section
    void
        center( glm::dvec3 Center );
    // generates renderable version of held non-instanced geometry
    void
        create_geometry();
    // provides access to bounding area data
    bounding_area const &
        area() const {
            return m_area; }

private:
// types
    using cell_array = std::array<basic_cell, (EU07_SECTIONSIZE / EU07_CELLSIZE) * (EU07_SECTIONSIZE / EU07_CELLSIZE)>;
    using shapenode_sequence = std::vector<shape_node>;
// methods
    // provides access to section enclosing specified point
    basic_cell &
        cell( glm::dvec3 const &Location );
// members
    // placement and visibility
    scene::bounding_area m_area { glm::dvec3(), static_cast<float>( 0.5 * M_SQRT2 * EU07_SECTIONSIZE + 0.25 * EU07_SECTIONSIZE ) };
    // content
    cell_array m_cells; // partitioning scheme
    shapenode_sequence m_shapes; // large pieces of opaque geometry and (legacy) terrain
    // TODO: implement dedicated, higher fidelity, fixed resolution terrain mesh item
    // gfx renderer data
    geometrybank_handle m_geometrybank;
    bool m_geometrycreated { false };
};

// top-level of scene spatial structure, holds collection of sections
class basic_region {

    friend class opengl_renderer;

public:
// constructors
    basic_region();
// destructor
    ~basic_region();
// methods
    // legacy method, updates sounds and polls event launchers around camera
    void
        update();
    // inserts provided shape in the region
    void
        insert_shape( shape_node Shape, scratch_data &Scratchpad );
    // inserts provided track in the region
    void
        insert_path( TTrack *Path, scratch_data &Scratchpad );
    // inserts provided track in the region
    void
        insert_traction( TTraction *Traction, scratch_data &Scratchpad );
    // inserts provided instance of 3d model in the region
    void
        insert_instance( TAnimModel *Instance, scratch_data &Scratchpad );
    // inserts provided sound in the region
    void
        insert_sound( TTextSound *Sound, scratch_data &Scratchpad );
    // find a vehicle located nearest to specified point, within specified radius, optionally ignoring vehicles without drivers. reurns: located vehicle and distance
    std::tuple<TDynamicObject *, float>
        find_vehicle( glm::dvec3 const &Point, float const Radius, bool const Onlycontrolled );
    // finds a path with one of its ends located in specified point. returns: located path and id of the matching endpoint
    std::tuple<TTrack *, int>
        find_path( glm::dvec3 const &Point, TTrack const *Exclude );
    // finds a traction piece with one of its ends located in specified point. returns: located traction piece and id of the matching endpoint
    std::tuple<TTraction *, int>
        find_traction( glm::dvec3 const &Point, TTraction const *Exclude );
    // finds a traction piece located nearest to specified point, sharing section with specified other piece and powered in specified direction. returns: located traction piece
    std::tuple<TTraction *, int>
        find_traction( glm::dvec3 const &Point, TTraction const *Other, int const Currentdirection );
    // finds sections inside specified sphere. returns: list of sections
    std::vector<basic_section *> const &
        sections( glm::dvec3 const &Point, float const Radius );

private:
// types
    using section_array = std::array<basic_section *, EU07_REGIONSIDESECTIONCOUNT * EU07_REGIONSIDESECTIONCOUNT>;

    struct region_scratchpad {

        std::vector<basic_section *> sections;
    };

// methods
    // registers specified end point of the provided path in the lookup directory of the region
    void
        register_path( TTrack *Path, glm::dvec3 const &Point );
    // registers specified end point of the provided traction piece in the lookup directory of the region
    void
        register_traction( TTraction *Traction, glm::dvec3 const &Point );
    // checks whether specified point is within boundaries of the region
    bool
        point_inside( glm::dvec3 const &Location );
    // legacy method, trims provided shape to fit into a section. adds trimmed part at the end of provided list, returns true if changes were made
    bool
        RaTriangleDivider( shape_node &Shape, std::deque<shape_node> &Shapes );
    // provides access to section enclosing specified point
    basic_section &
        section( glm::dvec3 const &Location );

// members
    section_array m_sections;
    region_scratchpad m_scratchpad;

};

} // scene

//---------------------------------------------------------------------------
