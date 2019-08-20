/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "Classes.h"
#include "scene.h"

// particle specialized for drawing smoke
// given smoke features we can take certain shortcuts
// -- there's no need to sort the particles, can be drawn in any order with depth write turned off
// -- the colour remains consistent throughout, only opacity changes
// -- randomized particle rotation
// -- initial velocity reduced over time to slow drift upwards (drift speed depends on particle and air temperature difference)
// -- size increased over time
struct smoke_particle {

    glm::dvec3 position; // meters, 3d space;
    float rotation; // radians; local z axis angle
    glm::vec3 velocity; // meters per second, 3d space; current velocity
    float size; // multiplier, billboard size
//    glm::vec4 color; // 0-1 range, rgba; geometry color and opacity
    float opacity; // 0-1 range
//    glm::vec2 uv_offset; // 0-1 range, uv space; for texture animation
    float age; // seconds; time elapsed since creation
//    double distance; // meters; distance between particle and camera
};

enum value_limit {
    min = 0,
    max = 1
};

// helper, adjusts provided variable by fixed amount, keeping resulting value between limits
template <typename Type_>
class fixedstep_modifier {

public:
// methods
    void
        deserialize( cParser &Input );
    // updates state of provided variable
    void
        update( Type_ &Variable, double const Timedelta ) const;
    void
        bind( Type_ const *Modifier ) {
            m_valuechangemodifier = Modifier; }
    Type_
        value_change() const {
            return (
                m_valuechangemodifier == nullptr ?
                    m_valuechange :
                    m_valuechange / *( m_valuechangemodifier ) ); }

private:
//types
// methods
// members
//    Type_ m_intialvalue { Type_( 0 ) }; // meters per second; velocity applied to freshly spawned particles
    Type_ m_valuechange { Type_( 0 ) }; // meters per second; change applied to initial velocity
    Type_ m_valuelimits[ 2 ] { Type_( std::numeric_limits<Type_>::lowest() ), Type_( std::numeric_limits<Type_>::max() ) };
    Type_ const *m_valuechangemodifier{ nullptr }; // optional modifier applied to value change
};


// particle emitter
class smoke_source {
// located in scenery
// new particles emitted if distance of source < double view range
// existing particles are updated until dead no matter the range (presumed to have certain lifespan)
// during update pass dead particle slots are filled with new instances, if there's no particles queued the slot is swapped with the last particle in the list
// bounding box/sphere calculated based on position of all owned particles, used by the renderer to include/discard data in a draw pass
    friend class particle_manager;

public:
// types
    using particle_sequence = std::vector<smoke_particle>;
// methods
    bool
        deserialize( cParser &Input );
    void
        initialize();
    void
        bind( TDynamicObject const *Vehicle );
    void
        bind( TAnimModel const *Node );
    // updates state of owned particles
    void
        update( double const Timedelta, bool const Onlydespawn );
    glm::vec3 const &
        color() const {
            return m_emitter.color; }
    glm::dvec3
        location() const;
    // provides access to bounding area data
    scene::bounding_area const &
        area() const {
            return m_area; }
    particle_sequence const &
        sequence() const {
            return m_particles; }

private:
// types
    enum class owner_type {
        none = 0,
        vehicle,
        node
    };

    struct particle_emitter {
        float inclination[ 2 ] { 0.f, 0.f };
        float velocity[ 2 ] { 1.f, 1.f };
        float size[ 2 ] { 1.f, 1.f };
        float opacity[ 2 ] { 1.f, 1.f };
        glm::vec3 color { 16.f / 255.f };

        void deserialize( cParser &Input );
        void initialize( smoke_particle &Particle );
    };

    using bounding_box = glm::dvec3[ 2 ]; // bounding box of owned particles

// methods
    // imports member data pair from the config file
    bool
        deserialize_mapping( cParser &Input );
    void
        initialize( smoke_particle &Particle );
    // updates state of provided particle and bounding box. returns: true if particle is still alive afterwards, false otherwise
    bool
        update( smoke_particle &Particle, bounding_box &Boundingbox, double const Timedelta );
// members
    // config/inputs
    // TBD: union and indicator, or just plain owner variables?
    owner_type m_ownertype { owner_type::none };
    union {
        TDynamicObject const * vehicle;
        TAnimModel const * node;
    } m_owner { nullptr }; // optional, scene item carrying this source
    glm::dvec3 m_offset; // meters, 3d space; relative position of the source, either from the owner or the region centre
    float m_spawnrate { 0.f }; // number of particles to spawn per second
    particle_emitter m_emitter;
//    bool m_inheritvelocity { false }; // whether spawned particle should receive velocity of its owner
    // TODO: replace modifiers with configurable interpolator item allowing keyframe-based changes over time
    fixedstep_modifier<float> m_sizemodifier; // particle billboard size
//    fixedstep_modifier<glm::vec3> m_colormodifier; // particle billboard color and opacity
    fixedstep_modifier<float> m_opacitymodifier;
//    texture_handle m_texture { -1 }; // texture assigned to particle billboards
    // current state
    float m_spawncount { 0.f }; // number of particles to spawn during next update
    particle_sequence m_particles; // collection of spawned particles
    std::size_t m_max_particles; // maximum number of particles existing
    scene::bounding_area m_area; // bounding sphere of owned particles
};


// holds all particle emitters defined in the scene and updates their state
class particle_manager {

    friend opengl_renderer;

public:
// types
    using source_sequence = std::vector<smoke_source>;
// constructors
    particle_manager() = default;
// destructor
//    ~particle_manager();
// methods
    // adds a new particle source of specified type, placing it in specified world location. returns: true on success, false if the specified type definition couldn't be located
    bool
        insert( std::string const &Sourcetemplate, glm::dvec3 const Location );
    bool
        insert( std::string const &Sourcetemplate, TDynamicObject const *Vehicle, glm::dvec3 const Location );
    bool
        insert( std::string const &Sourcetemplate, TAnimModel const *Node, glm::dvec3 const Location );
    // updates state of all owned emitters
    void
        update();
    // data access
    source_sequence &
        sequence() {
            return m_sources; }

// members

private:
// types
    using source_map = std::unordered_map<std::string, smoke_source>;
// methods
    smoke_source *
        find( std::string const &Template );
// members
    source_map m_sourcetemplates; // cached particle emitter configurations
    source_sequence m_sources; // all owned particle emitters
};



template <typename Type_>
void
fixedstep_modifier<Type_>::update( Type_ &Variable, double const Timedelta ) const {
    // HACK: float cast to avoid vec3 and double mismatch
    // TBD, TODO: replace with vector types specialization
    auto const valuechange { (
        m_valuechangemodifier == nullptr ?
            m_valuechange :
            m_valuechange / *( m_valuechangemodifier ) ) };
    Variable += ( valuechange * static_cast<float>( Timedelta ) );
    // clamp down to allowed value range
    Variable = glm::max( Variable, m_valuelimits[ value_limit::min ] );
    Variable = glm::min( Variable, m_valuelimits[ value_limit::max ] );
}

template <typename Type_>
void
fixedstep_modifier<Type_>::deserialize( cParser &Input ) {

    if( Input.getToken<std::string>() != "{" ) { return; }

    std::unordered_map<std::string, Type_ &> const variablemap {
        { "step:", m_valuechange },
        { "min:", m_valuelimits[ value_limit::min ] },
        { "max:", m_valuelimits[ value_limit::max ] } };

    std::string key;

    while( ( false == ( ( key = Input.getToken<std::string>( true, "\n\r\t  ,;[]" ) ).empty() ) )
        && ( key != "}" ) ) {

        auto const lookup { variablemap.find( key ) };
        if( lookup == variablemap.end() ) { continue; }

        lookup->second = Input.getToken<Type_>( true, "\n\r\t  ,;[]" );
    }
}
