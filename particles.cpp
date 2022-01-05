/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "particles.h"

#include "Timer.h"
#include "Globals.h"
#include "AnimModel.h"
#include "simulationenvironment.h"
#include "Logs.h"


void
smoke_source::particle_emitter::deserialize( cParser &Input ) {

    if( Input.getToken<std::string>() != "{" ) { return; }

    std::unordered_map<std::string, float &> const variablemap {
        { "min_inclination:", inclination[ value_limit::min ] },
        { "max_inclination:", inclination[ value_limit::max ] },
        { "min_velocity:", velocity[ value_limit::min ] },
        { "max_velocity:", velocity[ value_limit::max ] },
        { "min_size:", size[ value_limit::min ] },
        { "max_size:", size[ value_limit::max ] },
        { "min_opacity:", opacity[ value_limit::min ] },
        { "max_opacity:", opacity[ value_limit::max ] } };
    std::string key;

    while( ( false == ( ( key = Input.getToken<std::string>( true, "\n\r\t  ,;[]" ) ).empty() ) )
        && ( key != "}" ) ) {

        if( key == "color:" ) {
            // special case, vec3 attribute type
            // TODO: variable table, if amount of vector attributes increases
            color = Input.getToken<glm::vec3>( true, "\n\r\t  ,;[]" );
            color =
                glm::clamp(
                    color / 255.f,
                    glm::vec3{ 0.f }, glm::vec3{ 1.f } );
        }
        else {
            // float type attributes
            auto const lookup { variablemap.find( key ) };
            if( lookup != variablemap.end() ) {
                lookup->second = Input.getToken<float>( true, "\n\r\t  ,;[]" );
            }
        }
    }
}

void
smoke_source::particle_emitter::initialize( smoke_particle &Particle ) {

    auto const polarangle { glm::radians( LocalRandom( inclination[ value_limit::min ], inclination[ value_limit::max ] ) ) }; // theta
    auto const azimuthalangle { glm::radians( LocalRandom( -180, 180 ) ) }; // phi
    // convert spherical coordinates to opengl coordinates
    auto const launchvector { glm::vec3(
        std::sin( polarangle ) * std::sin( azimuthalangle ) * -1,
        std::cos( polarangle ),
        std::sin( polarangle ) * std::cos( azimuthalangle ) ) };
        auto const launchvelocity { static_cast<float>( LocalRandom( velocity[ value_limit::min ], velocity[ value_limit::max ] ) ) };
    
    Particle.velocity = launchvector * launchvelocity;

    Particle.rotation = glm::radians( LocalRandom( 0, 360 ) );
    Particle.size = LocalRandom( size[ value_limit::min ], size[ value_limit::max ] );
    Particle.opacity = LocalRandom( opacity[ value_limit::min ], opacity[ value_limit::max ] ) / Global.SmokeFidelity;
    Particle.age = 0;
}

bool
smoke_source::deserialize( cParser &Input ) {

    if( false == Input.ok() ) { return false; }

    while( true == deserialize_mapping( Input ) ) {
        ; // all work done by while()
    }

    return true;
}

// imports member data pair from the config file
bool
smoke_source::deserialize_mapping( cParser &Input ) {

    // token can be a key or block end
    std::string const key { Input.getToken<std::string>( true, "\n\r\t  ,;[]" ) };

    if( ( true == key.empty() ) || ( key == "}" ) ) { return false; }

    // if not block end then the key is followed by assigned value or sub-block
    if( key == "spawn_rate:" ) {
        Input.getTokens();
        Input >> m_spawnrate;
    }
    else if( key == "initializer:" ) {
        m_emitter.deserialize( Input );
    }
/*
    else if( key == "velocity_change:" ) {
        m_velocitymodifier.deserialize( Input );
    }
*/
    else if( key == "size_change:" ) {
        m_sizemodifier.deserialize( Input );
    }
    else if( key == "opacity_change:" ) {
        m_opacitymodifier.deserialize( Input );
    }

    return true; // return value marks a [ key: value ] pair was extracted, nothing about whether it's recognized
}

void
smoke_source::initialize() {

    m_max_particles =
        // put a cap on number of particles in a single source. TBD, TODO: make it part of he source configuration?
        std::min(
            static_cast<int>( 500 * Global.SmokeFidelity ),
            // NOTE: given nature of the smoke we're presuming opacity decreases over time and the particle is killed when it reaches 0
            // this gives us estimate of longest potential lifespan of single particle, and how many particles total can there be at any given time
            // TBD, TODO: explicit lifespan variable as part of the source configuration?
            static_cast<int>( m_spawnrate / std::abs( m_opacitymodifier.value_change() ) ) );
}

void
smoke_source::bind( TDynamicObject const *Vehicle ) {

    m_owner.vehicle = Vehicle;
    m_ownertype = (
        m_owner.vehicle != nullptr ?
            owner_type::vehicle :
            owner_type::none );
}

void
smoke_source::bind( TAnimModel const *Node ) {

    m_owner.node = Node;
    m_ownertype = (
        m_owner.node != nullptr ?
            owner_type::node :
            owner_type::none );
}

// updates state of owned particles
void
smoke_source::update( double const Timedelta, bool const Onlydespawn ) {

    // prepare bounding box for new pass
    // TODO: include bounding box in the bounding_area class
    bounding_box boundingbox {
        glm::dvec3{ std::numeric_limits<double>::max() },
        glm::dvec3{ std::numeric_limits<double>::lowest() } };

    m_spawncount = (
        ( ( false == Global.Smoke ) || ( true == Onlydespawn ) ) ?
            0.f :
            std::min<float>(
                m_spawncount + ( m_spawnrate * Timedelta * Global.SmokeFidelity ),
                m_max_particles ) );
    // consider special spawn rate cases
    if( m_ownertype == owner_type::vehicle ) {
        // HACK: don't spawn particles in tunnels, to prevent smoke clipping through 'terrain' outside
        if( m_owner.vehicle->RaTrackGet()->eEnvironment == e_tunnel ) {
            m_spawncount = 0.f;
        }
        if( false == m_owner.vehicle->bEnabled ) {
            // don't spawn particles for vehicles which left the scenario
            m_spawncount = 0.f;
        }
    }
    // update spawned particles
    for( auto particleiterator { std::begin( m_particles ) }; particleiterator != std::end( m_particles ); ++particleiterator ) {

        auto &particle { *particleiterator };
        bool particleisalive;

        while( ( false == ( particleisalive = update( particle, boundingbox, Timedelta ) ) )
            && ( m_spawncount >= 1.f ) ) {
            // replace dead particle with a new one
            m_spawncount -= 1.f;
            initialize( particle );
        }
        if( false == particleisalive ) {
            // we have a dead particle and no pending spawn requests, (try to) move the last particle here
            do {
                if( std::next( particleiterator ) == std::end( m_particles ) ) { break; } // already at last particle
                particle = m_particles.back();
                m_particles.pop_back();
            } while( false == ( particleisalive = update( particle, boundingbox, Timedelta ) ) );
        }
        if( false == particleisalive ) {
            // NOTE: if we're here it means the iterator is at last container slot which holds a dead particle about to be eliminated...
            m_particles.pop_back();
            // ...since this effectively makes the iterator now point at end() and the advancement at the end of the loop will move it past end()
            // we have to break the loop manually (could use < comparison but with both ways being ugly, this is 
            break;
        }
    }
    // spawn pending particles in remaining container slots
    while( ( m_spawncount >= 1.f )
        && ( m_particles.size() < m_max_particles ) ) {

        m_spawncount -= 1.f;
        // work with a temporary copy in case initial update renders the particle dead
        smoke_particle newparticle;
        initialize( newparticle );
        if( true == update( newparticle, boundingbox, Timedelta ) ) {
            // if the new particle didn't die immediately place it in the container...
            m_particles.emplace_back( newparticle );
        }
    }
    // if we still have pending requests after filling entire container replace older particles
    if( m_spawncount >= 1.f ) {
        // sort all particles from most to least transparent, oldest to youngest if it's a tie
        std::sort(
            std::begin( m_particles ),
            std::end( m_particles ),
            []( smoke_particle const &Left, smoke_particle const &Right ) {
                return ( Left.opacity != Right.opacity ?
                            Left.opacity < Right.opacity :
                            Left.age > Right.age ); } );
        // replace old particles with new ones until we run out of either requests or room
        for( auto &particle : m_particles ) {

            while( m_spawncount >= 1.f ) {
                m_spawncount -= 1.f;
                // work with a temporary copy so we don't wind up with replacing a good particle with a dead on arrival one
                smoke_particle newparticle;
                initialize( newparticle );
                if( true == update( newparticle, boundingbox, Timedelta ) ) {
                    // if the new particle didn't die immediately place it in the container...
                    particle = newparticle;
                    // ...and move on to the next slot
                    break;
                }
            }
        }
        // discard pending spawn requests our container couldn't fit
        m_spawncount -= std::floor( m_spawncount );
    }
    // determine bounding area from calculated bounding box
    if( false == m_particles.empty() ) {
        m_area.center = interpolate( boundingbox[ value_limit::min ], boundingbox[ value_limit::max ], 0.5 );
        m_area.radius = 0.5 * ( glm::length( boundingbox[ value_limit::max ] - boundingbox[ value_limit::min ] ) );
    }
    else {
        m_area.center = location();
        m_area.radius = 0;
    }
}

glm::dvec3
smoke_source::location() const {

    glm::dvec3 location;

    switch( m_ownertype ) {
        case owner_type::vehicle: {
            location = glm::dvec3 {
                  m_offset.x * m_owner.vehicle->VectorLeft()
                + m_offset.y * m_owner.vehicle->VectorUp()
                + m_offset.z * m_owner.vehicle->VectorFront() };
            location += glm::dvec3{ m_owner.vehicle->GetPosition() };
            break;
        }
        case owner_type::node: {
            auto const rotationx { glm::angleAxis( glm::radians( m_owner.node->Angles().x ), glm::vec3{ 1.f, 0.f, 0.f } ) };
            auto const rotationy { glm::angleAxis( glm::radians( m_owner.node->Angles().y ), glm::vec3{ 0.f, 1.f, 0.f } ) };
            auto const rotationz { glm::angleAxis( glm::radians( m_owner.node->Angles().z ), glm::vec3{ 0.f, 0.f, 1.f } ) };
            location = rotationy * rotationx * rotationz * glm::vec3{ m_offset };
            location += m_owner.node->location();
            break;
        }
        default: {
            location = m_offset;
            break;
        }
    }

    return location;
}

// sets particle state to fresh values
void
smoke_source::initialize( smoke_particle &Particle ) {

    m_emitter.initialize( Particle );

    Particle.position = location();

    if( m_ownertype == owner_type::vehicle ) {
        Particle.opacity *= m_owner.vehicle->MoverParameters->dizel_fill;
        auto const enginerevolutionsfactor { 0.5f }; // high engine revolutions increase initial particle velocity
        switch( m_owner.vehicle->MoverParameters->EngineType ) {
            case TEngineType::DieselElectric: {
                Particle.velocity *= 1.0 + enginerevolutionsfactor * m_owner.vehicle->MoverParameters->enrot / ( m_owner.vehicle->MoverParameters->DElist[ m_owner.vehicle->MoverParameters->MainCtrlPosNo ].RPM / 60.0 );
                break;
            }
            case TEngineType::DieselEngine: {
                Particle.velocity *= 1.0 + enginerevolutionsfactor * m_owner.vehicle->MoverParameters->enrot / m_owner.vehicle->MoverParameters->nmax;
                break;
            }
            default: {
                break;
            }
        }
    }
}

// updates state of provided particle and bounding box. returns: true if particle is still alive afterwards, false otherwise
bool
smoke_source::update( smoke_particle &Particle, bounding_box &Boundingbox, double const Timedelta ) {

    m_opacitymodifier.update( Particle.opacity, Timedelta );
    // if the particle is dead we can bail out early...
    if( Particle.opacity <= 0.f ) { return false; }
    // ... otherwise proceed with full update
    m_sizemodifier.update( Particle.size, Timedelta );

    // crude smoke dispersion simulation
    // http://www.auburn.edu/academic/forestry_wildlife/fire/smoke_guide/smoke_dispersion.htm
    Particle.velocity.y += ( 0.005 * Particle.velocity.y ) * std::min( 0.f, Global.AirTemperature - 10 ) * Timedelta; // decelerate faster in cold weather
    Particle.velocity.y -= ( 0.050 * Particle.velocity.y ) * Global.Overcast * Timedelta; // decelerate faster with high air humidity and/or precipitation
    Particle.velocity.y = std::max<float>( 0.25 * ( 2.f - Global.Overcast ), Particle.velocity.y ); // put a cap on deceleration

    Particle.position += Particle.velocity * static_cast<float>( Timedelta );
    Particle.position += 0.1f * Particle.age * simulation::Environment.wind() * static_cast<float>( Timedelta );
//    m_velocitymodifier.update( Particle.velocity, Timedelta );

    Particle.age += Timedelta;

    // update bounding box
    Boundingbox[ value_limit::min ] = glm::min( Boundingbox[ value_limit::min ], Particle.position - glm::dvec3{ Particle.size } );
    Boundingbox[ value_limit::max ] = glm::max( Boundingbox[ value_limit::max ], Particle.position + glm::dvec3{ Particle.size } );

    return true;
}



// adds a new particle source of specified type, placing it in specified world location
// returns: true on success, false if the specified type definition couldn't be located
bool
particle_manager::insert( std::string const &Sourcetemplate, glm::dvec3 const Location ) {

    auto const *sourcetemplate { find( Sourcetemplate ) };

    if( sourcetemplate == nullptr ) { return false; }

    // ...if template lookup didn't fail put template clone on the source list and initialize it
    m_sources.emplace_back( *sourcetemplate );
    auto &source { m_sources.back() };
    source.initialize();
    source.m_offset = Location;

    return true;
}

bool
particle_manager::insert( std::string const &Sourcetemplate, TDynamicObject const *Vehicle, glm::dvec3 const Location ) {

    if( false == insert( Sourcetemplate, Location ) ) { return false; }
        
    // attach the source to specified vehicle
    auto &source { m_sources.back() };
    source.bind( Vehicle );

    return true;
}

bool
particle_manager::insert( std::string const &Sourcetemplate, TAnimModel const *Node, glm::dvec3 const Location ) {

    if( false == insert( Sourcetemplate, Location ) ) { return false; }
        
    // attach the source to specified node
    auto &source { m_sources.back() };
    source.bind( Node );

    return true;
}

// updates state of all owned emitters
void
particle_manager::update() {

    auto const timedelta { Timer::GetDeltaTime() };

    if( timedelta == 0.0 ) { return; }

    auto const distancethreshold { 2 * Global.BaseDrawRange * Global.fDistanceFactor }; // to reduce workload distant enough sources won't spawn new particles

    for( auto &source : m_sources ) {

        auto const viewerdistance { glm::length( source.area().center - glm::dvec3{ Global.pCamera.Pos } ) - source.area().radius };

        source.update( timedelta, viewerdistance > distancethreshold );
    }
}

smoke_source *
particle_manager::find( std::string const &Template ) {

    auto const templatepath { "data/" };
    auto const templatename { ToLower( Template ) };

    // try to locate specified rail profile...
    auto const lookup { m_sourcetemplates.find( templatename ) };
    if( lookup != m_sourcetemplates.end() ) {
        // ...if it works, we're done...
        return &(lookup->second);
    }
    // ... and if it fails try to add the template to the database from a data file
    smoke_source source;
    cParser sound_parser( templatepath + templatename + ".txt", cParser::buffer_FILE );
    if( source.deserialize( sound_parser ) ) {
        // if deserialization didn't fail finish source setup...
        source.m_opacitymodifier.bind( &Global.SmokeFidelity );
        // ...then cache the source as template for future instances
        m_sourcetemplates.emplace( templatename, source );
        // should be 'safe enough' to return lookup result directly afterwards
        return &( m_sourcetemplates.find( templatename )->second );
    }
    else {
        ErrorLog( "Bad file: failed to locate particle source configuration file \"" + std::string( templatepath + templatename + ".txt" ) + "\"", logtype::file );
    }
    // if fetching data from the file fails too, give up
    return nullptr;
}
