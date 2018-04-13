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

#include "Globals.h"
#include "Logs.h"
#include "uilayer.h"
#include "renderer.h"
#include "World.h"

namespace simulation {

state_manager State;
event_manager Events;
memory_table Memory;
path_table Paths;
traction_table Traction;
powergridsource_table Powergrid;
instance_table Instances;
vehicle_table Vehicles;
light_array Lights;
sound_table Sounds;
lua Lua;

scene::basic_region *Region { nullptr };

bool
state_manager::deserialize( std::string const &Scenariofile ) {

    // TODO: move initialization to separate routine so we can reuse it
    SafeDelete( Region );
    Region = new scene::basic_region();

    // TODO: check first for presence of serialized binary files
    // if this fails, fall back on the legacy text format
    scene::scratch_data importscratchpad;
    if( Scenariofile != "$.scn" ) {
        // compilation to binary file isn't supported for rainsted-created overrides
        importscratchpad.binary.terrain = Region->deserialize( Scenariofile );
    }
    // NOTE: for the time being import from text format is a given, since we don't have full binary serialization
    cParser scenarioparser( Scenariofile, cParser::buffer_FILE, Global.asCurrentSceneryPath, Global.bLoadTraction );

    if( false == scenarioparser.ok() ) { return false; }

    deserialize( scenarioparser, importscratchpad );
    if( ( false == importscratchpad.binary.terrain )
     && ( Scenariofile != "$.scn" ) ) {
        // if we didn't find usable binary version of the scenario files, create them now for future use
        // as long as the scenario file wasn't rainsted-created base file override
        Region->serialize( Scenariofile );
    }

    Global.iPause &= ~0x10; // koniec pauzy wczytywania
    return true;
}

// legacy method, calculates changes in simulation state over specified time
void
state_manager::update( double const Deltatime, int Iterationcount ) {
    // aktualizacja animacji krokiem FPS: dt=krok czasu [s], dt*iter=czas od ostatnich przeliczeń
    if (Deltatime == 0.0) {
        // jeśli załączona jest pauza, to tylko obsłużyć ruch w kabinie trzeba
        return;
    }

    auto const totaltime { Deltatime * Iterationcount };
    // NOTE: we perform animations first, as they can determine factors like contact with powergrid
    TAnimModel::AnimUpdate( totaltime ); // wykonanie zakolejkowanych animacji

    simulation::Powergrid.update( totaltime );
    simulation::Vehicles.update( Deltatime, Iterationcount );
}

// restores class data from provided stream
void
state_manager::deserialize( cParser &Input, scene::scratch_data &Scratchpad ) {

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
                { "lua",         &state_manager::deserialize_lua },
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
        functionmap.emplace( function.first, std::bind( function.second, this, std::ref( Input ), std::ref( Scratchpad ) ) );
    }

    // deserialize content from the provided input
    auto
        timelast { std::chrono::steady_clock::now() },
        timenow { timelast };
    std::string token { Input.getToken<std::string>() };
    while( false == token.empty() ) {

        auto lookup = functionmap.find( token );
        if( lookup != functionmap.end() ) {
            lookup->second();
        }
        else {
            ErrorLog( "Bad scenario: unexpected token \"" + token + "\" encountered in file \"" + Input.Name() + "\" (line " + std::to_string( Input.Line() - 1 ) + ")" );
        }

        timenow = std::chrono::steady_clock::now();
        if( std::chrono::duration_cast<std::chrono::milliseconds>( timenow - timelast ).count() >= 200 ) {
            timelast = timenow;
            glfwPollEvents();
            UILayer.set_progress( Input.getProgress(), Input.getFullProgress() );
            GfxRenderer.Render();
        }

        token = Input.getToken<std::string>();
    }

    if( false == Scratchpad.initialized ) {
        // manually perform scenario initialization
        deserialize_firstinit( Input, Scratchpad );
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
        >> Global.fFogStart
        >> Global.fFogEnd;

    if( Global.fFogEnd > 0.0 ) {
        // fog colour; optional legacy parameter, no longer used
        Input.getTokens( 3 );
    }

    std::string token { Input.getToken<std::string>() };
    if( token != "endatmo" ) {
        // optional overcast parameter
        Global.Overcast = clamp( std::stof( token ), 0.f, 2.f );
        // overcast drives weather so do a calculation here
        // NOTE: ugly, clean it up when we're done with world refactoring
        Global.pWorld->compute_weather();
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
        into = ++Global.iCameraLast;
    if( into < 10 ) { // przepisanie do odpowiedniego miejsca w tabelce
        Global.FreeCameraInit[ into ] = xyz;
        Global.FreeCameraInitAngle[ into ] =
            Math3D::vector3(
                glm::radians( abc.x ),
                glm::radians( abc.y ),
                glm::radians( abc.z ) );
        Global.iCameraLast = into; // numer ostatniej
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
    Global.ConfigParse( Input );
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
        Scratchpad.location.offset.empty() ?
            Math3D::vector3() :
            Math3D::vector3(
                Scratchpad.location.offset.top().x,
                Scratchpad.location.offset.top().y,
                Scratchpad.location.offset.top().z ) );
    event->Load( &Input, offset );

    if( false == simulation::Events.insert( event ) ) {
        delete event;
    }
}

void state_manager::deserialize_lua( cParser &Input, scene::scratch_data &Scratchpad )
{
	Input.getTokens(1, false);
	std::string file;
	Input >> file;
	simulation::Lua.interpret(Global.asCurrentSceneryPath + file);
}

void
state_manager::deserialize_firstinit( cParser &Input, scene::scratch_data &Scratchpad ) {

    if( true == Scratchpad.initialized ) { return; }

    simulation::Paths.InitTracks();
    simulation::Traction.InitTraction();
    simulation::Events.InitEvents();
    simulation::Events.InitLaunchers();
    simulation::Memory.InitCells();

    Scratchpad.initialized = true;
}

void
state_manager::deserialize_light( cParser &Input, scene::scratch_data &Scratchpad ) {

    // legacy section, no longer used nor supported;
    skip_until( Input, "endlight" );
}

void
state_manager::deserialize_node( cParser &Input, scene::scratch_data &Scratchpad ) {

    auto const inputline = Input.Line(); // cache in case we need to report error

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

        auto *vehicle { deserialize_dynamic( Input, Scratchpad, nodedata ) };
        // vehicle import can potentially fail
        if( vehicle == nullptr ) { return; }

        if( false == simulation::Vehicles.insert( vehicle ) ) {

            ErrorLog( "Bad scenario: vehicle with duplicate name \"" + vehicle->name() + "\" encountered in file \"" + Input.Name() + "\" (line " + std::to_string( inputline ) + ")" );
        }

        if( ( vehicle->MoverParameters->CategoryFlag == 1 ) // trains only
         && ( ( ( vehicle->LightList( side::front ) & ( light::headlight_left | light::headlight_right | light::headlight_upper ) ) != 0 )
           || ( ( vehicle->LightList( side::rear )  & ( light::headlight_left | light::headlight_right | light::headlight_upper ) ) != 0 ) ) ) {
            simulation::Lights.insert( vehicle );
        }
    }
    else if( nodedata.type == "track" ) {

        auto *path { deserialize_path( Input, Scratchpad, nodedata ) };
        // duplicates of named tracks are currently experimentally allowed
        if( false == simulation::Paths.insert( path ) ) {
            ErrorLog( "Bad scenario: track with duplicate name \"" + path->name() + "\" encountered in file \"" + Input.Name() + "\" (line " + std::to_string( inputline ) + ")" );
/*
            delete path;
            delete pathnode;
*/
        }
        simulation::Region->insert_path( path, Scratchpad );
    }
    else if( nodedata.type == "traction" ) {

        auto *traction { deserialize_traction( Input, Scratchpad, nodedata ) };
        // traction loading is optional
        if( traction == nullptr ) { return; }

        if( false == simulation::Traction.insert( traction ) ) {
            ErrorLog( "Bad scenario: traction piece with duplicate name \"" + traction->name() + "\" encountered in file \"" + Input.Name() + "\" (line " + std::to_string( inputline ) + ")" );
        }
        simulation::Region->insert_traction( traction, Scratchpad );
    }
    else if( nodedata.type == "tractionpowersource" ) {

        auto *powersource { deserialize_tractionpowersource( Input, Scratchpad, nodedata ) };
        // traction loading is optional
        if( powersource == nullptr ) { return; }

        if( false == simulation::Powergrid.insert( powersource ) ) {
            ErrorLog( "Bad scenario: power grid source with duplicate name \"" + powersource->name() + "\" encountered in file \"" + Input.Name() + "\" (line " + std::to_string( inputline ) + ")" );
        }
/*
        // TODO: implement this
        simulation::Region.insert_powersource( powersource, Scratchpad );
*/
    }
    else if( nodedata.type == "model" ) {

        if( nodedata.range_min < 0.0 ) {
            // 3d terrain
            if( false == Scratchpad.binary.terrain ) {
                // if we're loading data from text .scn file convert and import
                auto *instance { deserialize_model( Input, Scratchpad, nodedata ) };
                // model import can potentially fail
                if( instance == nullptr ) { return; }
                // go through submodels, and import them as shapes
                auto const cellcount = instance->TerrainCount() + 1; // zliczenie submodeli
                for( auto i = 1; i < cellcount; ++i ) {
                    auto *submodel = instance->TerrainSquare( i - 1 );
                    simulation::Region->insert_shape(
                        scene::shape_node().convert( submodel ),
                        Scratchpad,
                        false );
                    // if there's more than one group of triangles in the cell they're held as children of the primary submodel
                    submodel = submodel->ChildGet();
                    while( submodel != nullptr ) {
                        simulation::Region->insert_shape(
                            scene::shape_node().convert( submodel ),
                            Scratchpad,
                            false );
                        submodel = submodel->NextGet();
                    }
                }
                // with the import done we can get rid of the source model
                delete instance;
            }
            else {
                // if binary terrain file was present, we already have this data
                skip_until( Input, "endmodel" );
            }
        }
        else {
            // regular instance of 3d mesh
            auto *instance { deserialize_model( Input, Scratchpad, nodedata ) };
            // model import can potentially fail
            if( instance == nullptr ) { return; }

            if( false == simulation::Instances.insert( instance ) ) {
                ErrorLog( "Bad scenario: 3d model instance with duplicate name \"" + instance->name() + "\" encountered in file \"" + Input.Name() + "\" (line " + std::to_string( inputline ) + ")" );
            }
            simulation::Region->insert_instance( instance, Scratchpad );
        }
    }
    else if( ( nodedata.type == "triangles" )
          || ( nodedata.type == "triangle_strip" )
          || ( nodedata.type == "triangle_fan" ) ) {

        if( false == Scratchpad.binary.terrain ) {

            simulation::Region->insert_shape(
                scene::shape_node().deserialize(
                    Input, nodedata ),
                Scratchpad,
                true );
        }
        else {
            // all shapes were already loaded from the binary version of the file
            skip_until( Input, "endtri" );
        }
    }
    else if( ( nodedata.type == "lines" )
          || ( nodedata.type == "line_strip" )
          || ( nodedata.type == "line_loop" ) ) {

        if( false == Scratchpad.binary.terrain ) {

            simulation::Region->insert_lines(
                scene::lines_node().deserialize(
                    Input, nodedata ),
                Scratchpad );
        }
        else {
            // all lines were already loaded from the binary version of the file
            skip_until( Input, "endline" );
        }
    }
    else if( nodedata.type == "memcell" ) {

        auto *memorycell { deserialize_memorycell( Input, Scratchpad, nodedata ) };
        if( false == simulation::Memory.insert( memorycell ) ) {
            ErrorLog( "Bad scenario: memory cell with duplicate name \"" + memorycell->name() + "\" encountered in file \"" + Input.Name() + "\" (line " + std::to_string( inputline ) + ")" );
        }
/*
        // TODO: implement this
        simulation::Region.insert_memorycell( memorycell, Scratchpad );
*/
    }
    else if( nodedata.type == "eventlauncher" ) {

        auto *eventlauncher { deserialize_eventlauncher( Input, Scratchpad, nodedata ) };
        if( false == simulation::Events.insert( eventlauncher ) ) {
            ErrorLog( "Bad scenario: event launcher with duplicate name \"" + eventlauncher->name() + "\" encountered in file \"" + Input.Name() + "\" (line " + std::to_string( inputline ) + ")" );
        }
            // event launchers can be either global, or local with limited range of activation
            // each gets assigned different caretaker
        if( true == eventlauncher->IsGlobal() ) {
            simulation::Events.queue( eventlauncher );
        }
        else {
            simulation::Region->insert_launcher( eventlauncher, Scratchpad );
        }
    }
    else if( nodedata.type == "sound" )
	{
        auto *sound { deserialize_sound( Input, Scratchpad, nodedata ) };
        if( false == simulation::Sounds.insert( sound ) ) {
            ErrorLog( "Bad scenario: sound node with duplicate name \"" + sound->name() + "\" encountered in file \"" + Input.Name() + "\" (line " + std::to_string( inputline ) + ")" );
        }
        simulation::Region->insert_sound( sound, Scratchpad );
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
    Scratchpad.location.offset.emplace(
        offset + (
            Scratchpad.location.offset.empty() ?
                glm::dvec3() :
                Scratchpad.location.offset.top() ) );
}

void
state_manager::deserialize_endorigin( cParser &Input, scene::scratch_data &Scratchpad ) {

    if( false == Scratchpad.location.offset.empty() ) {
        Scratchpad.location.offset.pop();
    }
    else {
        ErrorLog( "Bad origin: endorigin instruction with empty origin stack in file \"" + Input.Name() + "\" (line " + std::to_string( Input.Line() - 1 ) + ")" );
    }
}

void
state_manager::deserialize_rotate( cParser &Input, scene::scratch_data &Scratchpad ) {

    Input.getTokens( 3 );
    Input
        >> Scratchpad.location.rotation.x
        >> Scratchpad.location.rotation.y
        >> Scratchpad.location.rotation.z;
}

void
state_manager::deserialize_sky( cParser &Input, scene::scratch_data &Scratchpad ) {

    // sky model
    Input.getTokens( 1 );
    Input
        >> Global.asSky;
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

    if( true == Global.ScenarioTimeCurrent ) {
        // calculate time shift required to match scenario time with local clock
        auto timenow = std::time( 0 );
        auto const *localtime = std::localtime( &timenow );
        Global.ScenarioTimeOffset = ( ( localtime->tm_hour * 60 + localtime->tm_min ) - ( time.wHour * 60 + time.wMinute ) ) / 60.f;
    }

    // remaining sunrise and sunset parameters are no longer used, as they're now calculated dynamically
    // anything else left in the section has no defined meaning
    skip_until( Input, "endtime" );
}

void
state_manager::deserialize_trainset( cParser &Input, scene::scratch_data &Scratchpad ) {

    if( true == Scratchpad.trainset.is_open ) {
        // shouldn't happen but if it does wrap up currently open trainset and report an error
        deserialize_endtrainset( Input, Scratchpad );
        ErrorLog( "Bad scenario: encountered nested trainset definitions in file \"" + Input.Name() + "\" (line " + std::to_string( Input.Line() ) + ")" );
    }

    Scratchpad.trainset = scene::scratch_data::trainset_data();
    Scratchpad.trainset.is_open = true;

    Input.getTokens( 4 );
    Input
        >> Scratchpad.trainset.name
        >> Scratchpad.trainset.track
        >> Scratchpad.trainset.offset
        >> Scratchpad.trainset.velocity;
}

void
state_manager::deserialize_endtrainset( cParser &Input, scene::scratch_data &Scratchpad ) {

    if( ( false == Scratchpad.trainset.is_open )
     || ( true == Scratchpad.trainset.vehicles.empty() ) ) {
        // not bloody likely but we better check for it just the same
        ErrorLog( "Bad trainset: empty trainset defined in file \"" + Input.Name() + "\" (line " + std::to_string( Input.Line() - 1 ) + ")" );
        Scratchpad.trainset.is_open = false;
        return;
    }

    std::size_t vehicleindex { 0 };
    for( auto *vehicle : Scratchpad.trainset.vehicles ) {
        // go through list of vehicles in the trainset, coupling them together and checking for potential driver
        if( ( vehicle->Mechanik != nullptr )
         && ( vehicle->Mechanik->Primary() ) ) {
            // primary driver will receive the timetable for this trainset
            Scratchpad.trainset.driver = vehicle;
        }
        if( vehicleindex > 0 ) {
            // from second vehicle on couple it with the previous one
            Scratchpad.trainset.vehicles[ vehicleindex - 1 ]->AttachPrev(
                vehicle,
                Scratchpad.trainset.couplings[ vehicleindex - 1 ] );
        }
        ++vehicleindex;
    }

    if( Scratchpad.trainset.driver != nullptr ) {
        // if present, send timetable to the driver
        // wysłanie komendy "Timetable" ustawia odpowiedni tryb jazdy
        auto *controller = Scratchpad.trainset.driver->Mechanik;
            controller->DirectionInitial();
            controller->PutCommand(
                "Timetable:" + Scratchpad.trainset.name,
                Scratchpad.trainset.velocity,
                0,
                nullptr );
    }
    if( Scratchpad.trainset.couplings.back() == coupling::faux ) {
        // jeśli ostatni pojazd ma sprzęg 0 to założymy mu końcówki blaszane (jak AI się odpali, to sobie poprawi)
        Scratchpad.trainset.vehicles.back()->RaLightsSet( -1, light::rearendsignals );
    }
    // all done
    Scratchpad.trainset.is_open = false;
}

// creates path and its wrapper, restoring class data from provided stream
TTrack *
state_manager::deserialize_path( cParser &Input, scene::scratch_data &Scratchpad, scene::node_data const &Nodedata ) {

    // TODO: refactor track and wrapper classes and their de/serialization. do offset and rotation after deserialization is done
    auto *track = new TTrack( Nodedata );
    Math3D::vector3 offset = (
        Scratchpad.location.offset.empty() ?
        Math3D::vector3() :
        Math3D::vector3(
            Scratchpad.location.offset.top().x,
            Scratchpad.location.offset.top().y,
            Scratchpad.location.offset.top().z ) );
    track->Load( &Input, offset );

    return track;
}

TTraction *
state_manager::deserialize_traction( cParser &Input, scene::scratch_data &Scratchpad, scene::node_data const &Nodedata ) {

    if( false == Global.bLoadTraction ) {
        skip_until( Input, "endtraction" );
        return nullptr;
    }
    // TODO: refactor track and wrapper classes and their de/serialization. do offset and rotation after deserialization is done
    auto *traction = new TTraction( Nodedata );
    auto offset = (
        Scratchpad.location.offset.empty() ?
            glm::dvec3() :
            Scratchpad.location.offset.top() );
    traction->Load( &Input, offset );

    return traction;
}

TTractionPowerSource *
state_manager::deserialize_tractionpowersource( cParser &Input, scene::scratch_data &Scratchpad, scene::node_data const &Nodedata ) {

    if( false == Global.bLoadTraction ) {
        skip_until( Input, "end" );
        return nullptr;
    }

    auto *powersource = new TTractionPowerSource( Nodedata );
    powersource->Load( &Input );
    // adjust location
    powersource->location( transform( powersource->location(), Scratchpad ) );

    return powersource;
}

TMemCell *
state_manager::deserialize_memorycell( cParser &Input, scene::scratch_data &Scratchpad, scene::node_data const &Nodedata ) {

    auto *memorycell = new TMemCell( Nodedata );
    memorycell->Load( &Input );
    // adjust location
    memorycell->location( transform( memorycell->location(), Scratchpad ) );

    return memorycell;
}

TEventLauncher *
state_manager::deserialize_eventlauncher( cParser &Input, scene::scratch_data &Scratchpad, scene::node_data const &Nodedata ) {

    glm::dvec3 location;
    Input.getTokens( 3 );
    Input
        >> location.x
        >> location.y
        >> location.z;

    auto *eventlauncher = new TEventLauncher( Nodedata );
    eventlauncher->Load( &Input );
    eventlauncher->location( transform( location, Scratchpad ) );

    return eventlauncher;
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

    auto *instance = new TAnimModel( Nodedata );
    instance->RaAnglesSet( Scratchpad.location.rotation + rotation ); // dostosowanie do pochylania linii

    if( false == instance->Load( &Input, false ) ) {
        // model nie wczytał się - ignorowanie node
        SafeDelete( instance );
    }
    instance->location( transform( location, Scratchpad ) );

    return instance;
}

TDynamicObject *
state_manager::deserialize_dynamic( cParser &Input, scene::scratch_data &Scratchpad, scene::node_data const &Nodedata ) {

    if( false == Scratchpad.trainset.is_open ) {
        // part of trainset data is used when loading standalone vehicles, so clear it just in case
        Scratchpad.trainset = scene::scratch_data::trainset_data();
    }
    auto const inputline { Input.Line() }; // cache in case of errors
    // basic attributes
    auto datafolder { Input.getToken<std::string>() };
    auto skinfile { Input.getToken<std::string>() };
    auto mmdfile { Input.getToken<std::string>() };

	std::replace(datafolder.begin(), datafolder.end(), '\\', '/');
	std::replace(skinfile.begin(), skinfile.end(), '\\', '/');
	std::replace(mmdfile.begin(), mmdfile.end(), '\\', '/');

    auto const pathname = (
        Scratchpad.trainset.is_open ?
            Scratchpad.trainset.track :
            Input.getToken<std::string>() );
    auto const offset { Input.getToken<double>( false ) };
    auto const drivertype { Input.getToken<std::string>() };
    auto const couplingdata = (
        Scratchpad.trainset.is_open ?
            Input.getToken<std::string>() :
            "3" );
    auto const velocity = (
        Scratchpad.trainset.is_open ?
            Scratchpad.trainset.velocity :
            Input.getToken<float>( false ) );
    // extract coupling type and optional parameters
    auto const couplingdatawithparams = couplingdata.find( '.' );
    auto coupling = (
        couplingdatawithparams != std::string::npos ?
            std::atoi( couplingdata.substr( 0, couplingdatawithparams ).c_str() ) :
            std::atoi( couplingdata.c_str() ) );
    if( coupling < 0 ) {
        // sprzęg zablokowany (pojazdy nierozłączalne przy manewrach)
        coupling = ( -coupling ) | coupling::permanent;
    }
    if( ( offset != -1.0 )
     && ( std::abs( offset ) > 0.5 ) ) { // maksymalna odległość między sprzęgami - do przemyślenia
        // likwidacja sprzęgu, jeśli odległość zbyt duża - to powinno być uwzględniane w fizyce sprzęgów...
        coupling = coupling::faux; 
    }
    auto const params = (
        couplingdatawithparams != std::string::npos ?
            couplingdata.substr( couplingdatawithparams + 1 ) :
            "" );
    // load amount and type
    auto loadcount { Input.getToken<int>( false ) };
    auto loadtype = (
        loadcount ?
            Input.getToken<std::string>() :
            "" );
    if( loadtype == "enddynamic" ) {
        // idiotoodporność: ładunek bez podanego typu nie liczy się jako ładunek
        loadcount = 0;
        loadtype = "";
    }

    auto *path = simulation::Paths.find( pathname );
    if( path == nullptr ) {

        ErrorLog( "Bad scenario: vehicle \"" + Nodedata.name + "\" placed on nonexistent path \"" + pathname + "\" in file \"" + Input.Name() + "\" (line " + std::to_string( inputline ) + ")" );
        skip_until( Input, "enddynamic" );
        return nullptr;
    }

    if( ( true == Scratchpad.trainset.vehicles.empty() ) // jeśli pierwszy pojazd,
     && ( false == path->asEvent0Name.empty() ) // tor ma Event0
     && ( std::abs( velocity ) <= 1.f ) // a skład stoi
     && ( Scratchpad.trainset.offset >= 0.0 ) // ale może nie sięgać na owy tor
     && ( Scratchpad.trainset.offset <  8.0 ) ) { // i raczej nie sięga
        // przesuwamy około pół EU07 dla wstecznej zgodności
        Scratchpad.trainset.offset = 8.0;
    }

    auto *vehicle = new TDynamicObject();
    
    auto const length =
        vehicle->Init(
            Nodedata.name,
            datafolder, skinfile, mmdfile,
            path,
            ( offset == -1.0 ?
                Scratchpad.trainset.offset :
                Scratchpad.trainset.offset - offset ),
            drivertype,
            velocity,
            Scratchpad.trainset.name,
            loadcount, loadtype,
            ( offset == -1.0 ),
            params );

    if( length != 0.0 ) { // zero oznacza błąd
        // przesunięcie dla kolejnego, minus bo idziemy w stronę punktu 1
        Scratchpad.trainset.offset -= length;
        // automatically establish permanent connections for couplers which specify them in their definitions
        if( ( coupling != 0 )
         && ( vehicle->MoverParameters->Couplers[ ( offset == -1.0 ? side::front : side::rear ) ].AllowedFlag & coupling::permanent ) ) {
            coupling |= coupling::permanent;
        }
        if( true == Scratchpad.trainset.is_open ) {
            Scratchpad.trainset.vehicles.emplace_back( vehicle );
            Scratchpad.trainset.couplings.emplace_back( coupling );
        }
    }
    else {
        if( vehicle->MyTrack != nullptr ) {
            // rare failure case where vehicle with length of 0 is added to the track,
            // treated as error code and consequently deleted, but still remains on the track
            vehicle->MyTrack->RemoveDynamicObject( vehicle );
        }
        delete vehicle;
        skip_until( Input, "enddynamic" );
        return nullptr;
    }

    auto const destination { Input.getToken<std::string>() };
    if( destination != "enddynamic" ) {
        // optional vehicle destination parameter
        vehicle->asDestination = Input.getToken<std::string>();
        skip_until( Input, "enddynamic" );
    }

    return vehicle;
}

sound_source *
state_manager::deserialize_sound( cParser &Input, scene::scratch_data &Scratchpad, scene::node_data const &Nodedata ) {

    glm::dvec3 location;
    Input.getTokens( 3 );
    Input
        >> location.x
        >> location.y
        >> location.z;
    // adjust location
    location = transform( location, Scratchpad );

    auto *sound = new sound_source( sound_placement::external, Nodedata.range_max );
    sound->offset( location );
    sound->name( Nodedata.name );
    sound->deserialize( Input, sound_type::single );

    skip_until( Input, "endsound" );

    return sound;
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

    if( Scratchpad.location.rotation != glm::vec3( 0, 0, 0 ) ) {
        auto const rotation = glm::radians( Scratchpad.location.rotation );
        Location = glm::rotateY<double>( Location, rotation.y ); // Ra 2014-11: uwzględnienie rotacji
    }
    if( false == Scratchpad.location.offset.empty() ) {
        Location += Scratchpad.location.offset.top();
    }
    return Location;
}

} // simulation

//---------------------------------------------------------------------------
