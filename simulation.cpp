/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "simulation.h"

#include "globals.h"
#include "logs.h"

namespace simulation {

state_manager State;
event_manager Events;
memory_manager Memory;
path_table Paths;
traction_table Traction;
instance_manager Instances;
light_array Lights;

scene::basic_region *Region { nullptr };

bool
state_manager::deserialize( std::string const &Scenariofile ) {

    // TODO: move initialization to separate routine so we can reuse it
    SafeDelete( Region );
    Region = new scene::basic_region();

    // TODO: check first for presence of serialized binary files
    // if this fails, fall back on the legacy text format
    cParser scenarioparser( Scenariofile, cParser::buffer_FILE, Global::asCurrentSceneryPath, Global::bLoadTraction );

    if( false == scenarioparser.ok() ) { return false; }

    deserialize( scenarioparser );
    // TODO: initialize links between loaded nodes

    return true;
}

// restores class data from provided stream
void
state_manager::deserialize( cParser &Input ) {

    scene::scratch_data importscratchpad;
    // prepare deserialization function table
    // since all methods use the same objects, we can have simple, hard-coded binds or lambdas for the task
    using deserializefunction = void(state_manager::*)(cParser &, scene::scratch_data &);
    std::vector<
        std::pair<
            std::string,
            deserializefunction> > functionlist = {
                { "atmo",        &state_manager::deserialize_atmo },
                { "camera",      &state_manager::deserialize_camera },
                { "config",      &state_manager::deserialize_config },
                { "description", &state_manager::deserialize_description },
                { "event",       &state_manager::deserialize_event },
                { "firstinit",   &state_manager::deserialize_firstinit },
                { "light",       &state_manager::deserialize_light },
                { "node",        &state_manager::deserialize_node },
                { "origin",      &state_manager::deserialize_origin },
                { "endorigin",   &state_manager::deserialize_endorigin },
                { "rotate",      &state_manager::deserialize_rotate },
                { "sky",         &state_manager::deserialize_sky },
                { "test",        &state_manager::deserialize_test },
                { "time",        &state_manager::deserialize_time },
                { "trainset",    &state_manager::deserialize_trainset },
                { "endtrainset", &state_manager::deserialize_endtrainset } };
    using deserializefunctionbind = std::function<void()>;
    std::unordered_map<
        std::string,
        deserializefunctionbind> functionmap;
    for( auto &function : functionlist ) {
        functionmap.emplace( function.first, std::bind( function.second, this, std::ref( Input ), std::ref( importscratchpad ) ) );
    }
    // deserialize content from the provided input
    std::string token { Input.getToken<std::string>() };
    while( false == token.empty() ) {

        auto lookup = functionmap.find( token );
        if( lookup != functionmap.end() ) {
            lookup->second();
        }
        else {
            ErrorLog( "Bad scenario: unexpected token \"" + token + "\" encountered in file \"" + Input.Name() + "\" (line " + std::to_string( Input.Line() - 1 ) + ")" );
        }

        token = Input.getToken<std::string>();
    }
}

void
state_manager::deserialize_atmo( cParser &Input, scene::scratch_data &Scratchpad ) {

    // NOTE: parameter system needs some decent replacement, but not worth the effort if we're moving to built-in editor
    // atmosphere color; legacy parameter, no longer used
    Input.getTokens( 3 );
    // fog range
    Input.getTokens( 2 );
    Input
        >> Global::fFogStart
        >> Global::fFogEnd;

    if( Global::fFogEnd > 0.0 ) {
        // fog colour; optional legacy parameter, no longer used
        Input.getTokens( 3 );
    }

    std::string token { Input.getToken<std::string>() };
    if( token != "endatmo" ) {
        // optional overcast parameter
        Global::Overcast = clamp( std::stof( token ), 0.f, 1.f );
    }
    while( ( false == token.empty() )
        && ( token != "endatmo" ) ) {
        // anything else left in the section has no defined meaning
        token = Input.getToken<std::string>();
    }
}

void
state_manager::deserialize_camera( cParser &Input, scene::scratch_data &Scratchpad ) {

    glm::dvec3 xyz, abc;
    int i = -1, into = -1; // do której definicji kamery wstawić
    std::string token;
    do { // opcjonalna siódma liczba określa numer kamery, a kiedyś były tylko 3
        Input.getTokens();
        Input >> token;
        switch( ++i ) { // kiedyś camera miało tylko 3 współrzędne
            case 0: { xyz.x = atof( token.c_str() ); break; }
            case 1: { xyz.y = atof( token.c_str() ); break; }
            case 2: { xyz.z = atof( token.c_str() ); break; }
            case 3: { abc.x = atof( token.c_str() ); break; }
            case 4: { abc.y = atof( token.c_str() ); break; }
            case 5: { abc.z = atof( token.c_str() ); break; }
            case 6: { into = atoi( token.c_str() ); break; } // takie sobie, bo można wpisać -1
            default: { break; }
        }
    } while( token.compare( "endcamera" ) != 0 );
    if( into < 0 )
        into = ++Global::iCameraLast;
    if( into < 10 ) { // przepisanie do odpowiedniego miejsca w tabelce
        Global::FreeCameraInit[ into ] = xyz;
        Global::FreeCameraInitAngle[ into ] =
            Math3D::vector3(
                glm::radians( abc.x ),
                glm::radians( abc.y ),
                glm::radians( abc.z ) );
        Global::iCameraLast = into; // numer ostatniej
    }
/*
    // cleaned up version of the above.
    // NOTE: no longer supports legacy mode where some parameters were optional
    Input.getTokens( 7 );
    glm::vec3
        position,
        rotation;
    int index;
    Input
        >> position.x
        >> position.y
        >> position.z
        >> rotation.x
        >> rotation.y
        >> rotation.z
        >> index;

    skip_until( Input, "endcamera" );

    // TODO: finish this
*/
}

void
state_manager::deserialize_config( cParser &Input, scene::scratch_data &Scratchpad ) {

    // config parameters (re)definition
    Global::ConfigParse( Input );
}

void
state_manager::deserialize_description( cParser &Input, scene::scratch_data &Scratchpad ) {

    // legacy section, never really used;
    skip_until( Input, "enddescription" );
}

void
state_manager::deserialize_event( cParser &Input, scene::scratch_data &Scratchpad ) {

    // TODO: refactor event class and its de/serialization. do offset and rotation after deserialization is done
    auto *event = new TEvent();
    Math3D::vector3 offset = (
        Scratchpad.location_offset.empty() ?
            Math3D::vector3() :
            Math3D::vector3(
                Scratchpad.location_offset.top().x,
                Scratchpad.location_offset.top().y,
                Scratchpad.location_offset.top().z ) );
    event->Load( &Input, offset );

    if( false == simulation::Events.insert( event ) ) {
        delete event;
    }
}

void
state_manager::deserialize_firstinit( cParser &Input, scene::scratch_data &Scratchpad ) {

    // TODO: implement
    simulation::Paths.InitTracks();
    simulation::Traction.InitTraction();
    simulation::Events.InitEvents();
}

void
state_manager::deserialize_light( cParser &Input, scene::scratch_data &Scratchpad ) {

    // legacy section, no longer used nor supported;
    skip_until( Input, "endlight" );
}

void
state_manager::deserialize_node( cParser &Input, scene::scratch_data &Scratchpad ) {

    scene::node_data nodedata;
    // common data and node type indicator
    Input.getTokens( 4 );
    Input
        >> nodedata.range_max
        >> nodedata.range_min
        >> nodedata.name
        >> nodedata.type;
    // type-based deserialization. not elegant but it'll do
    if( nodedata.type == "dynamic" ) {
        // TODO: implement
        skip_until( Input, "enddynamic" );
    }
    else if( nodedata.type == "track" ) {

        auto const inputline = Input.Line(); // cache in case we need to report error

        auto *path { deserialize_path( Input, Scratchpad, nodedata ) };
        // duplicates of named tracks are currently experimentally allowed
        if( simulation::Paths.insert( path ) ) {
            simulation::Region->insert_path( path, Scratchpad );
        }
        else {
            ErrorLog( "Bad scenario: track with duplicate name, \"" + path->name() + "\" encountered in file \"" + Input.Name() + "\" (line " + std::to_string( inputline ) + ")" );
/*
            delete path;
            delete pathnode;
*/
        }
    }
    else if( nodedata.type == "traction" ) {

        auto const inputline = Input.Line(); // cache in case we need to report error

        auto *traction { deserialize_traction( Input, Scratchpad, nodedata ) };
        // duplicates of named tracks are currently discarded
        if( simulation::Traction.insert( traction ) ) {
            simulation::Region->insert_traction( traction, Scratchpad );
        }
        else {
            ErrorLog( "Bad scenario: traction piece with duplicate name, \"" + traction->name() + "\" encountered in file \"" + Input.Name() + "\" (line " + std::to_string( inputline ) + ")" );
        }
    }
    else if( nodedata.type == "tractionpowersource" ) {
        // TODO: implement
        skip_until( Input, "end" );
    }
    else if( nodedata.type == "model" ) {

        if( nodedata.range_min < 0.0 ) {
            // convert and import 3d terrain
        }
        else {
            // regular instance of 3d mesh
            auto const inputline = Input.Line(); // cache in case we need to report error

            auto *instance { deserialize_model( Input, Scratchpad, nodedata ) };
            // model import can potentially fail
            if( instance == nullptr ) { return; }

            if( simulation::Instances.insert( instance ) ) {
                   simulation::Region->insert_instance( instance, Scratchpad );
            }
            else {
                ErrorLog( "Bad scenario: 3d model instance with duplicate name, \"" + instance->name() + "\" encountered in file \"" + Input.Name() + "\" (line " + std::to_string( inputline ) + ")" );
            }
        }
    }
    else if( ( nodedata.type == "triangles" )
          || ( nodedata.type == "triangle_strip" )
          || ( nodedata.type == "triangle_fan" ) ) {

        simulation::Region->insert_shape( scene::shape_node().deserialize( Input, nodedata ), Scratchpad );
    }
    else if( ( nodedata.type == "lines" )
          || ( nodedata.type == "line_strip" )
          || ( nodedata.type == "line_loop" ) ) {

        // TODO: implement
        skip_until( Input, "endline" );
    }
    else if( nodedata.type == "memcell" ) {

        auto const inputline = Input.Line(); // cache in case we need to report error

        auto *memorycell { deserialize_memorycell( Input, Scratchpad, nodedata ) };
        // duplicates of named tracks are currently discarded
        if( simulation::Memory.insert( memorycell ) ) {
/*
            // TODO: implement this
            simulation::Region.insert_memorycell( memorycell, Scratchpad );
*/
        }
        else {
            ErrorLog( "Bad scenario: memory cell with duplicate name, \"" + memorycell->name() + "\" encountered in file \"" + Input.Name() + "\" (line " + std::to_string( inputline ) + ")" );
/*
            delete memorycell;
            delete memorycellnode;
*/
        }
    }
    else if( nodedata.type == "eventlauncher" ) {
        // TODO: implement
        skip_until( Input, "end" );
    }
    else if( nodedata.type == "sound" ) {
        // TODO: implement
        skip_until( Input, "endsound" );
    }

}

void
state_manager::deserialize_origin( cParser &Input, scene::scratch_data &Scratchpad ) {

    glm::dvec3 offset;
    Input.getTokens( 3 );
    Input
        >> offset.x
        >> offset.y
        >> offset.z;
    // sumowanie całkowitego przesunięcia
    Scratchpad.location_offset.emplace(
        offset + (
            Scratchpad.location_offset.empty() ?
                glm::dvec3() :
                Scratchpad.location_offset.top() ) );
}

void
state_manager::deserialize_endorigin( cParser &Input, scene::scratch_data &Scratchpad ) {

    if( false == Scratchpad.location_offset.empty() ) {
        Scratchpad.location_offset.pop();
    }
    else {
        ErrorLog( "Bad origin: endorigin instruction with empty origin stack in file \"" + Input.Name() + "\" (line " + to_string( Input.Line() - 1 ) + ")" );
    }
}

void
state_manager::deserialize_rotate( cParser &Input, scene::scratch_data &Scratchpad ) {

    Input.getTokens( 3 );
    Input
        >> Scratchpad.location_rotation.x
        >> Scratchpad.location_rotation.y
        >> Scratchpad.location_rotation.z;
}

void
state_manager::deserialize_sky( cParser &Input, scene::scratch_data &Scratchpad ) {

    // sky model
    Input.getTokens( 1 );
    Input
        >> Global::asSky;
    // anything else left in the section has no defined meaning
    skip_until( Input, "endsky" );
}

void
state_manager::deserialize_test( cParser &Input, scene::scratch_data &Scratchpad ) {

    // legacy section, no longer supported;
    skip_until( Input, "endtest" );
}

void
state_manager::deserialize_time( cParser &Input, scene::scratch_data &Scratchpad ) {

    // current scenario time
    cParser timeparser( Input.getToken<std::string>() );
    timeparser.getTokens( 2, false, ":" );
    auto &time = simulation::Time.data();
    timeparser
        >> time.wHour
        >> time.wMinute;

    // remaining sunrise and sunset parameters are no longer used, as they're now calculated dynamically
    // anything else left in the section has no defined meaning
    skip_until( Input, "endtime" );
}

void
state_manager::deserialize_trainset( cParser &Input, scene::scratch_data &Scratchpad ) {

    // TODO: implement
    skip_until( Input, "endtrainset" );
}

void
state_manager::deserialize_endtrainset( cParser &Input, scene::scratch_data &Scratchpad ) {

    // TODO: implement
}

// creates path and its wrapper, restoring class data from provided stream
TTrack *
state_manager::deserialize_path( cParser &Input, scene::scratch_data &Scratchpad, scene::node_data const &Nodedata ) {

    // TODO: refactor track and wrapper classes and their de/serialization. do offset and rotation after deserialization is done
    auto *track = new TTrack( Nodedata );
    Math3D::vector3 offset = (
        Scratchpad.location_offset.empty() ?
        Math3D::vector3() :
        Math3D::vector3(
            Scratchpad.location_offset.top().x,
            Scratchpad.location_offset.top().y,
            Scratchpad.location_offset.top().z ) );
    track->Load( &Input, offset );

    return track;
}

TTraction *
state_manager::deserialize_traction( cParser &Input, scene::scratch_data &Scratchpad, scene::node_data const &Nodedata ) {

    // TODO: refactor track and wrapper classes and their de/serialization. do offset and rotation after deserialization is done
    auto *traction = new TTraction( Nodedata );
    auto offset = (
        Scratchpad.location_offset.empty() ?
            glm::dvec3() :
            Scratchpad.location_offset.top() );
    traction->Load( &Input, offset );

    return traction;
}

TMemCell *
state_manager::deserialize_memorycell( cParser &Input, scene::scratch_data &Scratchpad, scene::node_data const &Nodedata ) {

    auto *memorycell = new TMemCell( Nodedata );
    memorycell->Load( &Input );
    // adjust location
    memorycell->location( transform( memorycell->location(), Scratchpad ) );

    return memorycell;
}

TAnimModel *
state_manager::deserialize_model( cParser &Input, scene::scratch_data &Scratchpad, scene::node_data const &Nodedata ) {

    glm::dvec3 location;
    glm::vec3 rotation;
    Input.getTokens( 4 );
    Input
        >> location.x
        >> location.y
        >> location.z
        >> rotation.y;
    // adjust location

    auto *instance = new TAnimModel( Nodedata );
    instance->RaAnglesSet( Scratchpad.location_rotation + rotation ); // dostosowanie do pochylania linii

    if( false == instance->Load( &Input, false ) ) {
        // model nie wczytał się - ignorowanie node
        SafeDelete( instance );
    }
    instance->location( transform( location, Scratchpad ) );

    return instance;
}

// skips content of stream until specified token
void
state_manager::skip_until( cParser &Input, std::string const &Token ) {

    std::string token { Input.getToken<std::string>() };
    while( ( false == token.empty() )
        && ( token != Token ) ) {

        token = Input.getToken<std::string>();
    }
}

// transforms provided location by specifed rotation and offset
glm::dvec3
state_manager::transform( glm::dvec3 Location, scene::scratch_data const &Scratchpad ) {

    if( Scratchpad.location_rotation != glm::vec3( 0, 0, 0 ) ) {
        auto const rotation = glm::radians( Scratchpad.location_rotation );
        Location = glm::rotateY<double>( Location, rotation.y ); // Ra 2014-11: uwzględnienie rotacji
    }
    if( false == Scratchpad.location_offset.empty() ) {
        Location += Scratchpad.location_offset.top();
    }
    return Location;
}

} // simulation

//---------------------------------------------------------------------------
