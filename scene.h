/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#ifndef SCENE_H_24_09_18
#define SCENE_H_24_09_18

#include <vector>
#include <deque>
#include <array>
#include <stack>
#include <unordered_set>

#include "parser.h"
#include "openglgeometrybank.h"
#include "scenenode.h"
#include "Track.h"
#include "Traction.h"
#include "sound.h"

class opengl_renderer;
namespace scene
{
    int const CELL_SIZE = 250;
    int const SECTION_SIZE = 1000;
    int const REGION_SIDE_SECTION_COUNT = 500; // number of sections along a side of square region

    struct scratch_data
    {
        struct binary_data
        {
            bool terrain{ false };
        } binary;

        struct location_data
        {
            std::stack<glm::dvec3> offset;
            glm::vec3 rotation;
        } location;

        struct trainset_data
        {
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
    class basic_cell
    {
        friend opengl_renderer;

      public:
        // constructors
        basic_cell() = default;

        // methods
        // potentially activates event handler with the same name as provided node, and within handler activation range
        void on_click( TAnimModel const *Instance );

        // legacy method, finds and assigns traction piece to specified pantograph of provided vehicle
        void update_traction( TDynamicObject *Vehicle, int const Pantographindex );

        // legacy method, polls event launchers within radius around specified point
        void update_events();

        // legacy method, updates sounds within radius around specified point
        void update_sounds();

        // legacy method, triggers radio-stop procedure for all vehicles located on paths in the cell
        void radio_stop();

        // legacy method, adds specified path to the list of pieces undergoing state change
        bool RaTrackAnimAdd( TTrack *Track );

        // legacy method, updates geometry for pieces in the animation list
        void RaAnimate( unsigned int const Framestamp );

        // sends content of the class to provided stream
        void serialize( std::ostream &Output ) const;

        // restores content of the class from provided stream
        void deserialize( std::istream &Input );

        // sends content of the class in legacy (text) format to provided stream
        void export_as_text( std::ostream &Output ) const;

        // adds provided shape to the cell
        void insert( shape_node Shape );

        // adds provided lines to the cell
        void insert( lines_node Lines );

        // adds provided path to the cell
        void insert( TTrack *Path );

        // adds provided path to the cell
        void insert( TTraction *Traction );

        // adds provided model instance to the cell
        void insert( TAnimModel *Instance );

        // adds provided sound instance to the cell
        void insert( sound_source *Sound );

        // adds provided event launcher to the cell
        void insert( TEventLauncher *Launcher );

        // adds provided memory cell to the cell
        void insert( TMemCell *Memorycell );

        // registers provided path in the lookup directory of the cell
        void register_end( TTrack *Path );

        // registers provided traction piece in the lookup directory of the cell
        void register_end( TTraction *Traction );

        // removes provided model instance from the cell
        void erase( TAnimModel *Instance );

        // removes provided memory cell from the cell
        void erase( TMemCell *Memorycell );

        // find a vehicle located nearest to specified point, within specified radius. reurns: located vehicle and distance
        auto find( glm::dvec3 const &Point, float const Radius, bool const Onlycontrolled, bool const Findbycoupler ) const
        -> std::tuple<TDynamicObject *, float>;

        // finds a path with one of its ends located in specified point. returns: located path and id of the matching endpoint
        auto find( glm::dvec3 const &Point, TTrack const *Exclude ) const
        -> std::tuple<TTrack *, int>;

        // finds a traction piece with one of its ends located in specified point. returns: located traction piece and id of the matching endpoint
        auto find( glm::dvec3 const &Point, TTraction const *Exclude ) const
        -> std::tuple<TTraction *, int>;

        // finds a traction piece located nearest to specified point, sharing section with specified other piece and powered in specified direction. returns: located traction piece
        auto find( glm::dvec3 const &Point, TTraction const *Other, int const Currentdirection ) const
        -> std::tuple<TTraction *, int, float>;

        // sets center point of the cell
        void center( glm::dvec3 Center );

        // generates renderable version of held non-instanced geometry in specified geometry bank
        void create_geometry( gfx::geometrybank_handle const &Bank );

        // provides access to bounding area data
        auto area() const -> bounding_area const& { return m_area; };

      private:
        // types
        using path_sequence = std::vector<TTrack *>;
        using shapenode_sequence = std::vector<shape_node>;
        using linesnode_sequence = std::vector<lines_node>;
        using traction_sequence = std::vector<TTraction *>;
        using instance_sequence = std::vector<TAnimModel *>;
        using sound_sequence = std::vector<sound_source *>;
        using eventlauncher_sequence = std::vector<TEventLauncher *>;
        using memorycell_sequence = std::vector<TMemCell *>;

        // methods
        void launch_event( TEventLauncher *Launcher );
        void enclose_area( scene::basic_node *Node );
        
        // members
        scene::bounding_area m_area { glm::dvec3(), static_cast<float>( 0.5 * M_SQRT2 * CELL_SIZE ) };
        bool m_active { false }; // whether the cell holds any actual data content
        shapenode_sequence m_shapesopaque; // opaque pieces of geometry
        shapenode_sequence m_shapestranslucent; // translucent pieces of geometry
        linesnode_sequence m_lines;
        path_sequence m_paths; // path pieces
        instance_sequence m_instancesopaque;
        instance_sequence m_instancetranslucent;
        traction_sequence m_traction;
        sound_sequence m_sounds;
        eventlauncher_sequence m_eventlaunchers;
        memorycell_sequence m_memorycells;

        // search helpers
        struct lookup_data
        {
            path_sequence paths;
            traction_sequence traction;
        } m_directories;

        // animation of owned items (legacy code, clean up along with track refactoring)
        bool m_geometrycreated { false };
        unsigned int m_framestamp { 0 }; // id of last rendered gfx frame
        TTrack *tTrackAnim = nullptr; // obiekty do przeliczenia animacji
    };

    // basic scene partitioning structure, holds terrain geometry and collection of cells
    class basic_section
    {
        friend opengl_renderer;
      public:
        // constructors
        basic_section() = default;
        // methods
        // potentially activates event handler with the same name as provided node, and within handler activation range
        void on_click( TAnimModel const *Instance );

        // legacy method, finds and assigns traction piece to specified pantograph of provided vehicle
        void update_traction( TDynamicObject *Vehicle, int const Pantographindex );

        // legacy method, updates sounds and polls event launchers within radius around specified point
        void update_events( glm::dvec3 const &Location, float const Radius );

        // legacy method, updates sounds and polls event launchers within radius around specified point
        void update_sounds( glm::dvec3 const &Location, float const Radius );

        // legacy method, triggers radio-stop procedure for all vehicles in 2km radius around specified location
        void radio_stop( glm::dvec3 const &Location, float const Radius );

        // sends content of the class to provided stream
        void serialize( std::ostream &Output ) const;

        // restores content of the class from provided stream
        void deserialize( std::istream &Input );

        // sends content of the class in legacy (text) format to provided stream
        void export_as_text( std::ostream &Output ) const;

        // adds provided shape to the section
        void insert( shape_node Shape );

        // adds provided lines to the section
        void insert( lines_node Lines );
        
        // adds provided node to the section
        template <class Type_>
        void insert( Type_ *Node )
        {
            auto &targetcell { cell( Node->location() ) };
            targetcell.insert( Node );
            // some node types can extend bounding area of the target cell
            m_area.radius = std::max(
                m_area.radius,
                static_cast<float>( glm::length( m_area.center - targetcell.area().center ) + targetcell.area().radius ) ); }

        // erases provided node from the section
        template <class Type_>
        void erase( Type_ *Node )
        {
            auto &targetcell { cell( Node->location() ) };
            // TODO: re-calculate bounding area after removal
            targetcell.erase( Node );
        }

        // registers provided node in the lookup directory of the section enclosing specified point
        template <class Type_>
        void register_node( Type_ *Node, glm::dvec3 const &Point )
        {
            cell( Point ).register_end( Node );
        }

        // find a vehicle located nearest to specified point, within specified radius. reurns: located vehicle and distance
        auto find( glm::dvec3 const &Point, float const Radius, bool const Onlycontrolled, bool const Findbycoupler )
        -> std::tuple<TDynamicObject *, float>;

        // finds a path with one of its ends located in specified point. returns: located path and id of the matching endpoint
        auto find( glm::dvec3 const &Point, TTrack const *Exclude )
        -> std::tuple<TTrack *, int>;

        // finds a traction piece with one of its ends located in specified point. returns: located traction piece and id of the matching endpoint
        auto find( glm::dvec3 const &Point, TTraction const *Exclude )
        -> std::tuple<TTraction *, int>;

        // finds a traction piece located nearest to specified point, sharing section with specified other piece and powered in specified direction. returns: located traction piece
        auto find( glm::dvec3 const &Point, TTraction const *Other, int const Currentdirection )
        -> std::tuple<TTraction *, int, float>;

        // sets center point of the section
        void center( glm::dvec3 Center );

        // generates renderable version of held non-instanced geometry
        void create_geometry();

        // provides access to bounding area data
        auto area() const -> bounding_area const& { return m_area; };

      private:
        // types
        using cell_array = std::array<basic_cell, (SECTION_SIZE / CELL_SIZE) * (SECTION_SIZE / CELL_SIZE)>;
        using shapenode_sequence = std::vector<shape_node>;

        // methods
        // provides access to section enclosing specified point
        auto cell( glm::dvec3 const &Location )
        -> basic_cell&;

        // members
        // placement and visibility
        scene::bounding_area m_area { glm::dvec3(), static_cast<float>( 0.5 * M_SQRT2 * SECTION_SIZE ) };

        // content
        cell_array m_cells; // partitioning scheme
        shapenode_sequence m_shapes; // large pieces of opaque geometry and (legacy) terrain

        // TODO: implement dedicated, higher fidelity, fixed resolution terrain mesh item
        // gfx renderer data
        gfx::geometrybank_handle m_geometrybank;
        bool m_geometrycreated { false };
    };

    // top-level of scene spatial structure, holds collection of sections
    class basic_region
    {
        friend opengl_renderer;

      public:
        // constructors
        basic_region();

        // destructor
        ~basic_region();
        // methods
        // potentially activates event handler with the same name as provided node, and within handler activation range
        void on_click( TAnimModel const *Instance );

        // legacy method, finds and assigns traction piece to specified pantograph of provided vehicle
        void update_traction( TDynamicObject *Vehicle, int const Pantographindex );

        // legacy method, polls event launchers around camera
        void update_events();

        // legacy method, updates sounds around camera
        void update_sounds();

        // stores content of the class in file with specified name
        void serialize( std::string const &Scenariofile ) const;

        // restores content of the class from file with specified name. returns: true on success, false otherwise
        bool deserialize( std::string const &Scenariofile );

        // sends content of the class in legacy (text) format to provided stream
        void export_as_text( std::ostream &Output ) const;

        // legacy method, links specified path piece with potential neighbours
        void TrackJoin( TTrack *Track );

        // legacy method, triggers radio-stop procedure for all vehicles in 2km radius around specified location
        void RadioStop( glm::dvec3 const &Location );

        // inserts provided shape in the region
        void insert( shape_node Shape, scratch_data &Scratchpad, bool const Transform );

        // inserts provided lines in the region
        void insert( lines_node Lines, scratch_data &Scratchpad );

        // inserts provided node in the region
        template <class Type_>
        void insert( Type_ *Node )
        {
            auto const location { Node->location() };
            if( false == point_inside( location ) ) {
                // NOTE: nodes placed outside of region boundaries are discarded
                // TBD, TODO: clamp coordinates to region boundaries?
                return; }
            section( location ).insert( Node );
        }

        // inserts provided node in the region and registers its ends in lookup directory
        template <class Type_>
        void insert_and_register( Type_ *Node )
        {
            insert( Node );
            for( auto const &point : Node->endpoints() ) {
                if( point_inside( point ) ) {
                    section( point ).register_node( Node, point ); } }
        }
                
        // removes specified node from the region
        template <class Type_>
        void erase( Type_ *Node )
        {
            auto const location{ Node->location() };
            if( point_inside( location ) ) {
                section( location ).erase( Node ); }
            }
            
        // find a vehicle located nearest to specified point, within specified radius. reurns: located vehicle and distance
        auto find_vehicle( glm::dvec3 const &Point, float const Radius, bool const Onlycontrolled, bool const Findbycoupler )
        -> std::tuple<TDynamicObject *, float>;

        // finds a path with one of its ends located in specified point. returns: located path and id of the matching endpoint
        auto find_path( glm::dvec3 const &Point, TTrack const *Exclude )
        -> std::tuple<TTrack *, int>;

        // finds a traction piece with one of its ends located in specified point. returns: located traction piece and id of the matching endpoint
        auto find_traction( glm::dvec3 const &Point, TTraction const *Exclude )
        -> std::tuple<TTraction *, int>;

        // finds a traction piece located nearest to specified point, sharing section with specified other piece and powered in specified direction. returns: located traction piece
        auto find_traction( glm::dvec3 const &Point, TTraction const *Other, int const Currentdirection )
        -> std::tuple<TTraction *, int>;

        // finds sections inside specified sphere. returns: list of sections
        auto sections( glm::dvec3 const &Point, float const Radius )
        -> std::vector<basic_section *> const&;

      private:
        // types
        using section_array = std::array<basic_section *, REGION_SIDE_SECTION_COUNT * REGION_SIDE_SECTION_COUNT>;

        struct region_scratchpad
        {
            std::vector<basic_section *> sections;
        };

        // methods
        // checks whether specified point is within boundaries of the region
        bool point_inside( glm::dvec3 const &Location );

        // legacy method, trims provided shape to fit into a section. adds trimmed part at the end of provided list, returns true if changes were made
        static bool RaTriangleDivider( shape_node &Shape, std::deque<shape_node> &Shapes );

        // provides access to section enclosing specified point
        auto section( glm::dvec3 const &Location ) -> basic_section&;

        // members
        section_array m_sections;
        region_scratchpad m_scratchpad;
    };
}

#endif //! SCENE_H_24_09_18
