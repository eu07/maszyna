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

#include "material.h"
#include "vertex.h"
#include "openglgeometrybank.h"
#include "parser.h"

struct lighting_data {

    glm::vec4 diffuse  { 0.8f, 0.8f, 0.8f, 1.0f };
    glm::vec4 ambient  { 0.2f, 0.2f, 0.2f, 1.0f };
    glm::vec4 specular { 0.0f, 0.0f, 0.0f, 1.0f };
};

inline
bool
operator==( lighting_data const &Left, lighting_data const &Right ) {
    return ( ( Left.diffuse == Right.diffuse )
          && ( Left.ambient == Right.ambient )
          && ( Left.specular == Right.specular ) ); }

inline
bool
operator!=( lighting_data const &Left, lighting_data const &Right ) {
    return !( Left == Right ); }

namespace scene {

struct bounding_area {

    glm::dvec3 center; // mid point of the rectangle
    float radius { -1.0f }; // radius of the bounding sphere

    bounding_area() = default;
    bounding_area( glm::dvec3 Center, float Radius ) :
        center( Center ),
        radius( Radius )
        {}
};
/*
enum nodetype {

    unknown,
    model,
    triangles,
    lines,
    dynamic,
    track,
    traction,
    powersource,
    sound,
    memorycell,
    eventlauncher
};

class node_manager {

};
*/

struct node_data {

    double range_min { 0.0 };
    double range_max { std::numeric_limits<double>::max() };
    std::string name;
    std::string type;
};

// holds unique piece of geometry, covered with single material
class shape_node {

    friend class basic_region; // region might want to modify node content when it's being inserted

public:
// types
    struct shapenode_data {
        // placement and visibility
        scene::bounding_area area; // bounding area, in world coordinates
        bool visible { true }; // visibility flag
        double rangesquared_min { 0.0 }; // visibility range, min
        double rangesquared_max { 0.0 }; // visibility range, max
        // material data
        material_handle material { 0 };
        lighting_data lighting;
        bool translucent { false }; // whether opaque or translucent
        // geometry data
        std::vector<world_vertex> vertices; // world space source data of the geometry
        glm::dvec3 origin; // world position of the relative coordinate system origin
        geometry_handle geometry { 0, 0 }; // relative origin-centered chunk of geometry held by gfx renderer
    };

// methods
    // restores content of the node from provded input stream
    shape_node &
        deserialize( cParser &Input, scene::node_data const &Nodedata );
    // adds content of provided node to already enclosed geometry. returns: true if merge could be performed
    bool
        merge( shape_node &Shape );
    // generates renderable version of held non-instanced geometry in specified geometry bank
    void
        create_geometry( geometrybank_handle const &Bank );
    // calculates shape's bounding radius
    void
        compute_radius();
    // set visibility
    void
        visible( bool State ) {
            m_data.visible = State; }
    // set origin point
    void
        origin( glm::dvec3 Origin ) {
            m_data.origin = Origin; }
    // data access
    shapenode_data const &
        data() const {
            return m_data; }

private:
// members
    std::string m_name;
    shapenode_data m_data;
};


/*
// holds geometry for specific piece of track/road/waterway
class path_node {

    friend class basic_region; // region might want to modify node content when it's being inserted

public:
// types
    // TODO: enable after track class refactoring
    struct pathnode_data {
        // placement and visibility
        bounding_area area; // bounding area, in world coordinates
        bool visible { true }; // visibility flag
        // material data
        material_handle material_1 { 0 };
        material_handle material_2 { 0 };
        lighting_data lighting;
        TEnvironmentType environment { e_flat };
        // geometry data
        std::vector<world_vertex> vertices; // world space source data of the geometry
        glm::dvec3 origin; // world position of the relative coordinate system origin
        using geometryhandle_sequence = std::vector<geometry_handle>;
        geometryhandle_sequence geometry_1; // geometry chunks textured with texture 1
        geometryhandle_sequence geometry_2; // geometry chunks textured with texture 2
    };
// methods
    // restores content of the node from provded input stream
    // TODO: implement
    path_node &
        deserialize( cParser &Input, node_data const &Nodedata );
    // binds specified track to the node
    // TODO: remove after track class refactoring
    void
        path( TTrack *Path ) {
            m_path = Path; }
    TTrack *
        path() {
            return m_path; }

private:
// members

//    // TODO: enable after track class refactoring
//    pathnode_data m_data;

    TTrack * m_path;
};
*/
/*
// holds reference to memory cell
class memorycell_node {

    friend class basic_region; // region might want to modify node content when it's being inserted

public:
// types
    struct memorynode_data {
        // placement and visibility
        bounding_area area; // bounding area, in world coordinates
        bool visible { false }; // visibility flag
    };
// methods
    // restores content of the node from provded input stream
    // TODO: implement
    memory_node &
        deserialize( cParser &Input, node_data const &Nodedata );
    void
        cell( TMemCell *Cell ) {
            m_memorycell = Cell; }
    TMemCell *
        cell() {
            return m_memorycell; }

private:
// members
    memorynode_data m_data;
    TMemCell * m_memorycell;
};
*/
} // scene

namespace editor {

// base interface for nodes which can be actvated in scenario editor
struct basic_node {

public:
// constructor
    basic_node() = default; // TODO: remove after refactor
    basic_node( scene::node_data const &Nodedata );
// destructor
    virtual ~basic_node() = default;
// methods
    std::string const &
        name() const {
            return m_name; }
    void
        location( glm::dvec3 const Location ) {
            m_location = Location; }
    glm::dvec3 const &
        location() const {
            return m_location; };
    void
        visible( bool const Visible ) {
            m_visible = Visible; }
    bool
        visible() const {
            return m_visible; }

protected:
// members
    glm::dvec3 m_location;
    bool m_visible { true };
    double m_rangesquaredmin { 0.0 }; // visibility range, min
    double m_rangesquaredmax { 0.0 }; // visibility range, max
    std::string m_name;
};

} // editor

//---------------------------------------------------------------------------
