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
#include <unordered_set>

#include "parser.h"
#include "openglgeometrybank.h"
#include "scenenode.h"
#include "track.h"
#include "traction.h"
#include "sound.h"

namespace scene {

int const EU07_CELLSIZE = 250;
int const EU07_SECTIONSIZE = 1000;
int const EU07_REGIONSIDESECTIONCOUNT = 500; // number of sections along a side of square region

struct scratch_data {

    struct binary_data {

        bool terrain{ false };
    } binary;

    struct location_data {

        std::stack<glm::dvec3> offset;
        glm::vec3 rotation;
    } location;

    struct trainset_data {

        std::string name;
        std::string track;
        float offset { 0.f };
        float velocity { 0.f };
        std::vector<TDynamicObject *> vehicles;
        std::vector<int> couplings;
        TDynamicObject * driver { nullptr };
        bool is_open { false };
    } trainset;

    bool initialized { false };
};

// basic element of rudimentary partitioning scheme for the section. fixed size, no further subdivision
// TBD, TODO: replace with quadtree scheme?
class basic_cell {

    friend class opengl_renderer;

public:
// methods
    // legacy method, finds and assigns traction piece to specified pantograph of provided vehicle
    void
        update_traction( TDynamicObject *Vehicle, int const Pantographindex );
    // legacy method, polls event launchers within radius around specified point
    void
        update_events();
    // legacy method, updates sounds within radius around specified point
    void
        update_sounds();
    // legacy method, triggers radio-stop procedure for all vehicles located on paths in the cell
    void
        radio_stop();
    // legacy method, adds specified path to the list of pieces undergoing state change
    bool
        RaTrackAnimAdd( TTrack *Track );
    // legacy method, updates geometry for pieces in the animation list
    void
        RaAnimate( unsigned int const Framestamp );
    // sends content of the class to provided stream
    void
        serialize( std::ostream &Output ) const;
    // restores content of the class from provided stream
    void
        deserialize( std::istream &Input );
    // adds provided shape to the cell
    void
        insert( shape_node Shape );
    // adds provided lines to the cell
    void
        insert( lines_node Lines );
    // adds provided path to the cell
    void
        insert( TTrack *Path );
    // adds provided path to the cell
    void
        insert( TTraction *Traction );
    // adds provided model instance to the cell
    void
        insert( TAnimModel *Instance );
    // adds provided sound instance to the cell
    void
#ifdef EU07_USE_OLD_SOUNDCODE
        insert( TTextSound *Sound );
#else
        insert( sound_source *Sound );
#endif
    // adds provided event launcher to the cell
    void
        insert( TEventLauncher *Launcher );
    // registers provided path in the lookup directory of the cell
    void
        register_end( TTrack *Path );
    // registers provided traction piece in the lookup directory of the cell
    void
        register_end( TTraction *Traction );
    // find a vehicle located nearest to specified point, within specified radius. reurns: located vehicle and distance
    std::tuple<TDynamicObject *, float>
        find( glm::dvec3 const &Point, float const Radius, bool const Onlycontrolled, bool const Findbycoupler ) const;
    // finds a path with one of its ends located in specified point. returns: located path and id of the matching endpoint
    std::tuple<TTrack *, int>
        find( glm::dvec3 const &Point, TTrack const *Exclude ) const;
    // finds a traction piece with one of its ends located in specified point. returns: located traction piece and id of the matching endpoint
    std::tuple<TTraction *, int>
        find( glm::dvec3 const &Point, TTraction const *Exclude ) const;
    // finds a traction piece located nearest to specified point, sharing section with specified other piece and powered in specified direction. returns: located traction piece
    std::tuple<TTraction *, int, float>
        find( glm::dvec3 const &Point, TTraction const *Other, int const Currentdirection ) const;
    // sets center point of the cell
    void
        center( glm::dvec3 Center );
    // generates renderable version of held non-instanced geometry in specified geometry bank
    void
        create_geometry( gfx::geometrybank_handle const &Bank );
    // provides access to bounding area data
    bounding_area const &
        area() const {
            return m_area; }

private:
// types
    using shapenode_sequence = std::vector<shape_node>;
    using linesnode_sequence = std::vector<lines_node>;
    using path_sequence = std::vector<TTrack *>;
    using traction_sequence = std::vector<TTraction *>;
    using instance_sequence = std::vector<TAnimModel *>;
#ifdef EU07_USE_OLD_SOUNDCODE
    using sound_sequence = std::vector<TTextSound *>;
#else
    using sound_sequence = std::vector<sound_source *>;
#endif
    using eventlauncher_sequence = std::vector<TEventLauncher *>;
// methods
    void
        enclose_area( editor::basic_node *Node );
// members
    scene::bounding_area m_area { glm::dvec3(), static_cast<float>( 0.5 * M_SQRT2 * EU07_CELLSIZE ) };
    bool m_active { false }; // whether the cell holds any actual data
    // content
    shapenode_sequence m_shapesopaque; // opaque pieces of geometry
    shapenode_sequence m_shapestranslucent; // translucent pieces of geometry
    linesnode_sequence m_lines;
    path_sequence m_paths; // path pieces
    instance_sequence m_instancesopaque;
    instance_sequence m_instancetranslucent;
    traction_sequence m_traction;
    sound_sequence m_sounds;
    eventlauncher_sequence m_eventlaunchers;
    // search helpers
    struct lookup_data {
        path_sequence paths;
        traction_sequence traction;
    } m_directories;
    // animation of owned items (legacy code, clean up along with track refactoring)
    bool m_geometrycreated { false };
    unsigned int m_framestamp { 0 }; // id of last rendered gfx frame
    TTrack *tTrackAnim = nullptr; // obiekty do przeliczenia animacji
};

// basic scene partitioning structure, holds terrain geometry and collection of cells
class basic_section {

    friend class opengl_renderer;

public:
// methods
// legacy method, finds and assigns traction piece to specified pantograph of provided vehicle
    void
        update_traction( TDynamicObject *Vehicle, int const Pantographindex );
    // legacy method, updates sounds and polls event launchers within radius around specified point
    void
        update_events( glm::dvec3 const &Location, float const Radius );
    // legacy method, updates sounds and polls event launchers within radius around specified point
    void
        update_sounds( glm::dvec3 const &Location, float const Radius );
    // legacy method, triggers radio-stop procedure for all vehicles in 2km radius around specified location
    void
        radio_stop( glm::dvec3 const &Location, float const Radius );
    // sends content of the class to provided stream
    void
        serialize( std::ostream &Output ) const;
    // restores content of the class from provided stream
    void
        deserialize( std::istream &Input );
    // adds provided shape to the section
    void
        insert( shape_node Shape );
    // adds provided lines to the section
    void
        insert( lines_node Lines );
    // adds provided node to the section
    template <class Type_>
    void
        insert( Type_ *Node ) {
            auto &targetcell { cell( Node->location() ) };
            targetcell.insert( Node );
            // some node types can extend bounding area of the target cell
            m_area.radius = std::max(
                m_area.radius,
                static_cast<float>( glm::length( m_area.center - targetcell.area().center ) + targetcell.area().radius ) ); }
    // registers provided node in the lookup directory of the section enclosing specified point
    template <class Type_>
    void
        register_node( Type_ *Node, glm::dvec3 const &Point ) {
            cell( Point ).register_end( Node ); }
    // find a vehicle located nearest to specified point, within specified radius. reurns: located vehicle and distance
    std::tuple<TDynamicObject *, float>
        find( glm::dvec3 const &Point, float const Radius, bool const Onlycontrolled, bool const Findbycoupler );
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
    scene::bounding_area m_area { glm::dvec3(), static_cast<float>( 0.5 * M_SQRT2 * EU07_SECTIONSIZE ) };
    // content
    cell_array m_cells; // partitioning scheme
    shapenode_sequence m_shapes; // large pieces of opaque geometry and (legacy) terrain
    // TODO: implement dedicated, higher fidelity, fixed resolution terrain mesh item
    // gfx renderer data
    gfx::geometrybank_handle m_geometrybank;
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
// legacy method, finds and assigns traction piece to specified pantograph of provided vehicle
    void
        update_traction( TDynamicObject *Vehicle, int const Pantographindex );
    // legacy method, polls event launchers around camera
    void
        update_events();
    // legacy method, updates sounds around camera
    void
        update_sounds();
    // stores content of the class in file with specified name
    void
        serialize( std::string const &Scenariofile ) const;
    // restores content of the class from file with specified name. returns: true on success, false otherwise
    bool
        deserialize( std::string const &Scenariofile );
    // legacy method, links specified path piece with potential neighbours
    void
        TrackJoin( TTrack *Track );
    // legacy method, triggers radio-stop procedure for all vehicles in 2km radius around specified location
    void
        RadioStop( glm::dvec3 const &Location );
    // inserts provided shape in the region
    void
        insert_shape( shape_node Shape, scratch_data &Scratchpad, bool const Transform );
    // inserts provided lines in the region
    void
        insert_lines( lines_node Lines, scratch_data &Scratchpad );
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
#ifdef EU07_USE_OLD_SOUNDCODE
        insert_sound( TTextSound *Sound, scratch_data &Scratchpad );
#else
        insert_sound( sound_source *Sound, scratch_data &Scratchpad );
#endif
    // inserts provided event launcher in the region
    void
        insert_launcher( TEventLauncher *Launcher, scratch_data &Scratchpad );
    // find a vehicle located nearest to specified point, within specified radius. reurns: located vehicle and distance
    std::tuple<TDynamicObject *, float>
        find_vehicle( glm::dvec3 const &Point, float const Radius, bool const Onlycontrolled, bool const Findbycoupler );
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
    static
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
