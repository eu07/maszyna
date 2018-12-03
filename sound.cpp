/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"

#include "sound.h"
#include "parser.h"
#include "Globals.h"
#include "Camera.h"
#include "Train.h"
#include "DynObj.h"
#include "simulation.h"

// constructors
sound_source::sound_source( sound_placement const Placement, float const Range ) :
    m_placement( Placement ),
    m_range( Range )
{}

// destructor
sound_source::~sound_source() {

    audio::renderer.erase( this );
}

// restores state of the class from provided data stream
sound_source &
sound_source::deserialize( std::string const &Input, sound_type const Legacytype, int const Legacyparameters ) {
	auto cp = cParser{ Input };
    return deserialize( cp, Legacytype, Legacyparameters );
}

sound_source &
sound_source::deserialize( cParser &Input, sound_type const Legacytype, int const Legacyparameters, int const Chunkrange ) {

    // cache parser config, as it may change during deserialization
    auto const inputautoclear { Input.autoclear() };

    Input.getTokens( 1, true, "\n\r\t ,;" );
    if( Input.peek() == "{" ) {
        // block type config
        while( true == deserialize_mapping( Input ) ) {
            ; // all work done by while()
        }

        if( false == m_soundchunks.empty() ) {
            // arrange loaded sound chunks in requested order
            std::sort(
                std::begin( m_soundchunks ), std::end( m_soundchunks ),
                []( soundchunk_pair const &Left, soundchunk_pair const &Right ) {
                    return ( Left.second.threshold < Right.second.threshold ); } );
            // calculate and cache full range points for each chunk, including crossfade sections:
            // on the far end the crossfade section extends to the threshold point of the next chunk...
            for( std::size_t idx = 0; idx < m_soundchunks.size() - 1; ++idx ) {
                m_soundchunks[ idx ].second.fadeout = m_soundchunks[ idx + 1 ].second.threshold;
/*
                m_soundchunks[ idx ].second.fadeout =
                    interpolate<float>(
                        m_soundchunks[ idx ].second.threshold,
                        m_soundchunks[ idx + 1 ].second.threshold,
                        m_crossfaderange * 0.01f );
*/
            }
            //  ...and on the other end from the threshold point back into the range of previous chunk
            m_soundchunks.front().second.fadein = std::max( 0, m_soundchunks.front().second.threshold );
//            m_soundchunks.front().second.fadein = m_soundchunks.front().second.threshold;
            for( std::size_t idx = 1; idx < m_soundchunks.size(); ++idx ) {
                auto const previouschunkwidth { m_soundchunks[ idx ].second.threshold - m_soundchunks[ idx - 1 ].second.threshold };
                m_soundchunks[ idx ].second.fadein = m_soundchunks[ idx ].second.threshold - 0.01f * m_crossfaderange * previouschunkwidth;
/*
                m_soundchunks[ idx ].second.fadein =
                    interpolate<float>(
                        m_soundchunks[ idx ].second.threshold,
                        m_soundchunks[ idx - 1 ].second.threshold,
                        m_crossfaderange * 0.01f );
*/
            }
            m_soundchunks.back().second.fadeout = std::max( Chunkrange, m_soundchunks.back().second.threshold );
//            m_soundchunks.back().second.fadeout = m_soundchunks.back().second.threshold;
            // test if the chunk table contains any actual samples while at it
            for( auto &soundchunk : m_soundchunks ) {
                if( soundchunk.first.buffer != null_handle ) {
                    m_soundchunksempty = false;
                    break;
                }
            }
        }
    }
    else {
        // legacy type config

        // set the parser to preserve retrieved tokens, so we don't need to mess with separately passing the initial read
        Input.autoclear( false );

        switch( Legacytype ) {
            case sound_type::single: {
                // single sample only
                m_sounds[ main ].buffer = audio::renderer.fetch_buffer( deserialize_random_set( Input, "\n\r\t ,;" ) );
                break;
            }
            case sound_type::multipart: {
                // three samples: start, middle, stop
                for( auto &sound : m_sounds ) {
                    sound.buffer = audio::renderer.fetch_buffer( deserialize_random_set( Input, "\n\r\t ,;" ) );
                }
                break;
            }
            default: {
                break;
            }
        }

        if( Legacyparameters & sound_parameters::range ) {
            Input.getTokens( 1, false );
            Input >> m_range;
        }
        if( Legacyparameters & sound_parameters::amplitude ) {
            Input.getTokens( 2, false );
            Input
                >> m_amplitudefactor
                >> m_amplitudeoffset;
        }
        if( Legacyparameters & sound_parameters::frequency ) {
            Input.getTokens( 2, false );
            Input
                >> m_frequencyfactor
                >> m_frequencyoffset;
        }
    }
    // restore parser behaviour
    Input.autoclear( inputautoclear );

    // catch and correct oddball cases with the same sample assigned as all parts of multipart sound
    if( m_sounds[ begin ].buffer == m_sounds[ main ].buffer ) {
        m_sounds[ begin ].buffer = null_handle;
    }
    if( m_sounds[ end ].buffer == m_sounds[ main ].buffer ) {
        m_sounds[ end ].buffer = null_handle;
    }

    return *this;
}

// imports member data pair from the config file
bool
sound_source::deserialize_mapping( cParser &Input ) {
    // token can be a key or block end
    std::string const key { Input.getToken<std::string>( true, "\n\r\t  ,;[]" ) };

    if( ( true == key.empty() ) || ( key == "}" ) ) { return false; }

    // if not block end then the key is followed by assigned value or sub-block
    if( key == "soundmain:" ) {
        sound( sound_id::main ).buffer = audio::renderer.fetch_buffer( deserialize_random_set( Input, "\n\r\t ,;" ) );
    }
    else if( key == "soundset:" ) {
        deserialize_soundset( Input );
    }
    else if( key == "soundbegin:" ) {
        sound( sound_id::begin ).buffer = audio::renderer.fetch_buffer( deserialize_random_set( Input, "\n\r\t ,;" ) );
    }
    else if( key == "soundend:" ) {
        sound( sound_id::end ).buffer = audio::renderer.fetch_buffer( deserialize_random_set( Input, "\n\r\t ,;" ) );
    }
    else if( key.compare( 0, std::min<std::size_t>( key.size(), 5 ), "sound" ) == 0 ) {
        // sound chunks, defined with key soundX where X = activation threshold
        auto const indexstart { key.find_first_of( "1234567890" ) };
        auto const indexend { key.find_first_not_of( "1234567890", indexstart ) };
        if( indexstart != std::string::npos ) {
            // NOTE: we'll sort the chunks at the end of deserialization
            m_soundchunks.emplace_back(
                soundchunk_pair {
                    // sound data
                    { audio::renderer.fetch_buffer( deserialize_random_set( Input, "\n\r\t ,;" ) ), 0 },
                    // chunk data
                    { std::stoi( key.substr( indexstart, indexend - indexstart ) ), 0, 0, 1.f } } );
        }
    }
    else if( key.compare( 0, std::min<std::size_t>( key.size(), 5 ), "pitch" ) == 0 ) {
        // sound chunk pitch, defined with key pitchX where X = activation threshold
        auto const indexstart { key.find_first_of( "1234567890" ) };
        auto const indexend { key.find_first_not_of( "1234567890", indexstart ) };
        if( indexstart != std::string::npos ) {
            auto const index { std::stoi( key.substr( indexstart, indexend - indexstart ) ) };
            auto const pitch { Input.getToken<float>( false, "\n\r\t ,;" ) };
            for( auto &chunk : m_soundchunks ) {
                if( chunk.second.threshold == index ) {
                    chunk.second.pitch = ( pitch > 0.f ? pitch : 1.f );
                    break;
                }
            }
        }
    }
    else if( key == "crossfade:" ) {
        // for combined sounds, percentage of assigned range allocated to crossfade sections
        Input.getTokens( 1, "\n\r\t ,;" );
        Input >> m_crossfaderange;
        m_crossfaderange = clamp( m_crossfaderange, 0, 100 );
    }
    else if( key == "placement:" ) {
        auto const value { Input.getToken<std::string>( true, "\n\r\t ,;" ) };
        std::map<std::string, sound_placement> const placements {
            { "internal", sound_placement::internal },
            { "engine", sound_placement::engine },
            { "external", sound_placement::external },
            { "general", sound_placement::general } };
        auto lookup{ placements.find( value ) };
        if( lookup != placements.end() ) {
            m_placement = lookup->second;
        }
    }
    else if( key == "offset:" ) {
        // point in 3d space, in format [ x, y, z ]
        Input.getTokens( 3, false, "\n\r\t ,;[]" );
        Input
            >> m_offset.x
            >> m_offset.y
            >> m_offset.z;
    }
    else {
        // floating point properties
        std::map<std::string, float &> const properties {
            { "frequencyfactor:", m_frequencyfactor },
            { "frequencyoffset:", m_frequencyoffset },
            { "amplitudefactor:", m_amplitudefactor },
            { "amplitudeoffset:", m_amplitudeoffset },
            { "range:", m_range } };

        auto lookup { properties.find( key ) };
        if( lookup != properties.end() ) {
            Input.getTokens( 1, false, "\n\r\t ,;" );
            Input >> lookup->second;
        }
    }

    return true; // return value marks a [ key: value ] pair was extracted, nothing about whether it's recognized
}

// imports values for initial, main and ending sounds from provided data stream
void
sound_source::deserialize_soundset( cParser &Input ) {

    auto const soundset { deserialize_random_set( Input, "\n\r\t ,;" ) };
    // split retrieved set
    cParser setparser( soundset );
    sound( sound_id::begin ).buffer = audio::renderer.fetch_buffer( setparser.getToken<std::string>( true, "|" ) );
    sound( sound_id::main ).buffer  = audio::renderer.fetch_buffer( setparser.getToken<std::string>( true, "|" ) );
    sound( sound_id::end ).buffer   = audio::renderer.fetch_buffer( setparser.getToken<std::string>( true, "|" ) );
}

// sends content of the class in legacy (text) format to provided stream
// NOTE: currently exports only the main sound
void
sound_source::export_as_text( std::ostream &Output ) const {

    if( sound( sound_id::main ).buffer == null_handle ) { return;  }

    // generic node header
    Output
        << "node "
        // visibility
        << m_range << ' '
        << 0 << ' '
        // name
        << m_name << ' ';
    // sound node header
    Output
        << "sound ";
    // location
    Output
        << m_offset.x << ' '
        << m_offset.y << ' '
        << m_offset.z << ' ';
    // sound data
    auto soundfile { audio::renderer.buffer( sound( sound_id::main ).buffer ).name };
    if( soundfile.find( szSoundPath ) == 0 ) {
        // don't include 'sounds/' in the path
        soundfile.erase( 0, std::string{ szSoundPath }.size() );
    }
    Output
        << soundfile << ' ';
    // footer
    Output
        << "endsound"
        << "\n";
}

// copies list of sounds from provided source
sound_source &
sound_source::copy_sounds( sound_source const &Source ) {

    m_sounds = Source.m_sounds;
    m_soundchunks = Source.m_soundchunks;
    m_soundchunksempty = Source.m_soundchunksempty;
    // reset source's playback counters
    for( auto &sound : m_sounds ) {
        sound.playing = 0;
    }
    for( auto &sound : m_soundchunks ) {
        sound.first.playing = 0;
    }
    return *this;
}

// issues contextual play commands for the audio renderer
void
sound_source::play( int const Flags ) {

    if( ( false == Global.bSoundEnabled )
     || ( true == empty() ) ) {
        // if the sound is disabled altogether or nothing can be emitted from this source, no point wasting time
        return;
    }

    // NOTE: we cache the flags early, even if the sound is out of range, to mark activated event sounds 
    m_flags = Flags;

    if( m_range > 0 ) {
        auto const cutoffrange { m_range * 5 };
        if( glm::length2( location() - glm::dvec3 { Global.pCamera.Pos } ) > std::min( 2750.f * 2750.f, cutoffrange * cutoffrange ) ) {
            // while we drop sounds from beyond sensible and/or audible range
            // we act as if it was activated normally, meaning no need to include the opening bookend in subsequent calls
            m_playbeginning = false;
            return;
        }
    }

    // initialize emitter-specific pitch variation if it wasn't yet set
    if( m_pitchvariation == 0.f ) {
        m_pitchvariation = 0.01f * static_cast<float>( Random( 97.5, 102.5 ) );
    }
/*
    if( ( ( m_flags & sound_flags::exclusive ) != 0 )
     && ( sound( sound_id::end ).playing > 0 ) ) {
        // request termination of the optional ending bookend for single instance sounds
        m_stopend = true;
    }
*/
    if( sound( sound_id::main ).buffer != null_handle ) {
        // basic variant: single main sound, with optional bookends
        play_basic();
    }
    else {
        // combined variant, main sound consists of multiple chunks, with optional bookends
        play_combined();
    }
}

void
sound_source::play_basic() {

    if( false == is_playing() ) {
        // dispatch appropriate sound
        if( ( true == m_playbeginning )
         && ( sound( sound_id::begin ).buffer != null_handle ) ) {
            std::vector<sound_id> sounds { sound_id::begin, sound_id::main };
            insert( std::begin( sounds ), std::end( sounds ) );
            m_playbeginning = false;
        }
        else {
            insert( sound_id::main );
        }
    }
    else {
        // for single part non-looping samples we allow spawning multiple instances, if not prevented by set flags
        if( ( ( m_flags & ( sound_flags::exclusive | sound_flags::looping ) ) == 0 )
         && ( sound( sound_id::begin ).buffer == null_handle ) ) {
            insert( sound_id::main );
        }
    }
}

void
sound_source::play_combined() {
    // combined sound consists of table od samples, each sample associated with certain range of values of controlling variable
    // current value of the controlling variable is passed to the source with pitch() call
    auto const soundpoint { compute_combined_point() };
    for( std::uint32_t idx = 0; idx < m_soundchunks.size(); ++idx ) {

        auto const &soundchunk { m_soundchunks[ idx ] };
        // a chunk covers range from fade in point, where it starts rising in volume over crossfade distance,
        // lasts until fadeout - crossfade distance point, past which it grows quiet until fade out point where it ends
        if( soundpoint < soundchunk.second.fadein )  { break; }
        if( soundpoint >= soundchunk.second.fadeout ) { continue; }
        
        if( ( soundchunk.first.buffer == null_handle )
         || ( ( ( m_flags & ( sound_flags::exclusive | sound_flags::looping ) ) != 0 )
           && ( soundchunk.first.playing > 0 ) ) ) {
            // combined sounds only play looped, single copy of each activated chunk
            continue;
        }

        if( idx > 0 ) {
            insert( sound_id::chunk | idx );
        }
        else {
            // initial chunk requires some safety checks if the optional bookend is present,
            // so we don't queue another instance while the bookend is still playing
            if( sound( sound_id::begin ).buffer == null_handle ) {
                // no bookend, safe to play the chunk
                insert( sound_id::chunk | idx );
            }
            else {
                // branches:
                // beginning requested, not playing; queue beginning and chunk
                // beginning not requested, not playing; queue chunk
                // otherwise skip, one instance is already in the audio queue
                if( sound( sound_id::begin ).playing == 0 ) {
                    if( true == m_playbeginning ) {
                        std::vector<sound_handle> sounds{ sound_id::begin, sound_id::chunk | idx };
                        insert( std::begin( sounds ), std::end( sounds ) );
                        m_playbeginning = false;
                    }
                    else {
                        insert( sound_id::chunk | idx );
                    }
                }
            }
        }
    }
}

// calculates requested sound point, used to select specific sample from the sample table
float
sound_source::compute_combined_point() const {

    return (
        m_properties.pitch <= 1.f ?
            // most sounds use 0-1 value range, we clamp these to 0-99 to allow more intuitive sound definition in .mmd files
            clamp( m_properties.pitch, 0.f, 0.99f ) :
            std::max( 0.f, m_properties.pitch )
        ) * 100.f;
}

// maintains playback of sounds started by event
void
sound_source::play_event() {

    if( true == TestFlag( m_flags, ( sound_flags::event | sound_flags::looping ) ) ) {
        // events can potentially start scenery sounds out of the sound's audible range
        // such sounds are stopped on renderer side, but unless stopped by the simulation keep their activation flags
        // we use this to discern event-started sounds which should be re-activated if the listener gets close enough
        play( m_flags );
    }
}

// stops currently active play commands controlled by this emitter
void
sound_source::stop( bool const Skipend ) {

    // if the source was stopped on simulation side, we should play the opening bookend next time it's activated
    m_playbeginning = true;
    // clear the event flags to discern between manual stop and out-of-range/sound-end stop
    m_flags = 0;

    if( false == is_playing() ) { return; }

    m_stop = true;

    if( ( false == Skipend )
     && ( sound( sound_id::end ).buffer != null_handle )
/*     && ( sound( sound_id::end ).buffer != sound( sound_id::main ).buffer ) */ // end == main can happen in malformed legacy cases
     && ( sound( sound_id::end ).playing < 2 ) ) { // allows potential single extra instance to account for longer overlapping sounds
        // spawn potentially defined sound end sample, if the emitter is currently active
        insert( sound_id::end );
    }
}

// adjusts parameters of provided implementation-side sound source
void
sound_source::update( audio::openal_source &Source ) {

    if( ( m_owner != nullptr )
     && ( false == m_owner->bEnabled ) ) {
        // terminate the sound if the owner is gone
        // TBD, TODO: replace with a listener pattern to receive vehicle removal and cab change events and such?
        m_stop = true;
    }

    if( sound( sound_id::main ).buffer != null_handle ) {
        // basic variant: single main sound, with optional bookends
        update_basic( Source );
        return;
    }
    if( false == m_soundchunksempty ) {
        // combined variant, main sound consists of multiple chunks, with optional bookends
        update_combined( Source );
        return;
    }
}

void
sound_source::update_basic( audio::openal_source &Source ) {

    if( true == Source.is_playing ) {

        auto const soundhandle { Source.sounds[ Source.sound_index ] };

        if( sound( sound_id::begin ).buffer != null_handle ) {
            // potentially a multipart sound
            // detect the moment when the sound moves from startup sample to the main
            if( true == Source.sound_change ) {
                // when it happens update active sample counters, and potentially activate the looping
                update_counter( sound_id::begin, -1 );
                update_counter( soundhandle, 1 );
                Source.loop( TestFlag( m_flags, sound_flags::looping ) );
            }
        }

        if( ( true == m_stop )
         && ( soundhandle != sound_id::end ) ) {
            // kill the sound if stop was requested, unless it's sound bookend sample
            update_counter( soundhandle, -1 );
            Source.stop();
            m_stop = is_playing(); // end the stop mode when all active sounds are dead
            return;
        }
/*
        if( ( true == m_stopend )
         && ( soundhandle == sound_id::end ) ) {
            // kill the sound if it's the bookend sample and stopping it was requested
            Source.stop();
            update_counter( sound_id::end, -1 );
            if( sound( sound_id::end ).playing == 0 ) {
                m_stopend = false;
            }
            return;
        }
*/
        // check and update if needed current sound properties
        update_location();
        update_soundproofing();
        Source.sync_with( m_properties );
        if( Source.sync != sync_state::good ) {
            // if the sync went wrong we let the renderer kill its part of the emitter, and update our playcounter(s) to match
            update_counter( soundhandle, -1 );
        }

    }
    else {
        // if the emitter isn't playing it's either done or wasn't yet started
        // we can determine this from number of processed buffers
        if( Source.sound_index != Source.sounds.size() ) {
            // the emitter wasn't yet started
            auto const soundhandle { Source.sounds[ Source.sound_index ] };
            // emitter initialization
            if( soundhandle == sound_id::main ) {
                // main sample can be optionally set to loop
                Source.loop( TestFlag( m_flags, sound_flags::looping ) );
            }
            Source.range( m_range );
            Source.pitch( m_pitchvariation );
            update_location();
            update_soundproofing();
            Source.sync_with( m_properties );
            if( Source.sync == sync_state::good ) {
                // all set, start playback
                Source.play();
                if( false == Source.is_playing ) {
                    // if the playback didn't start update the state counter
                    update_counter( soundhandle, -1 );
                }
            }
            else {
                // if the initial sync went wrong we skip the activation so the renderer can clean the emitter on its end
                update_counter( soundhandle, -1 );
            }
        }
        else {
            // the emitter is either all done or was terminated early
            update_counter( Source.sounds[ Source.sound_index - 1 ], -1 );
        }
    }
}

void
sound_source::update_combined( audio::openal_source &Source ) {

    if( true == Source.is_playing ) {

        auto const soundhandle { Source.sounds[ Source.sound_index ] };

        if( sound( sound_id::begin ).buffer != null_handle ) {
            // potentially a multipart sound
            // detect the moment when the sound moves from startup sample to the main
            if( true == Source.sound_change ) {
                // when it happens update active sample counters, and activate the looping
                update_counter( sound_id::begin, -1 );
                update_counter( soundhandle, 1 );
                Source.loop( true );
            }
        }

        if( ( true == m_stop )
         && ( soundhandle != sound_id::end ) ) {
            // kill the sound if stop was requested, unless it's sound bookend sample
            Source.stop();
            update_counter( soundhandle, -1 );
            if( false == is_playing() ) {
                m_stop = false;
            }
            return;
        }
/*
        if( ( true == m_stopend )
         && ( soundhandle == sound_id::end ) ) {
            // kill the sound if it's the bookend sample and stopping it was requested
            Source.stop();
            update_counter( sound_id::end, -1 );
            if( sound( sound_id::end ).playing == 0 ) {
                m_stopend = false;
            }
            return;
        }
*/
        if( ( soundhandle & sound_id::chunk ) != 0 ) {
            // for sound chunks, test whether the chunk should still be active given current value of the controlling variable
            if( ( m_flags & ( sound_flags::exclusive | sound_flags::looping ) ) != 0 ) {
                auto const soundpoint { compute_combined_point() };
                auto const &soundchunk { m_soundchunks[ soundhandle ^ sound_id::chunk ] };
                if( ( soundpoint < soundchunk.second.fadein )
                 || ( soundpoint >= soundchunk.second.fadeout ) ) {
                    Source.stop();
                    update_counter( soundhandle, -1 );
                    return;
                }
            }
        }

        // check and update if needed current sound properties
        update_location();
        update_soundproofing();
        // pitch and volume are adjusted on per-chunk basis
        // since they're relative to base values, backup these...
        auto const baseproperties = m_properties;
        // ...adjust per-chunk parameters...
        update_crossfade( soundhandle );
        // ... pass the parameters to the audio renderer...
        Source.sync_with( m_properties );
        if( Source.sync != sync_state::good ) {
            // if the sync went wrong we let the renderer kill its part of the emitter, and update our playcounter(s) to match
            update_counter( soundhandle, -1 );
        }
        // ...and restore base properties
        m_properties = baseproperties;
    }
    else {
        // if the emitter isn't playing it's either done or wasn't yet started
        // we can determine this from number of processed buffers
        if( Source.sound_index != Source.sounds.size() ) {
            // the emitter wasn't yet started
            auto const soundhandle { Source.sounds[ Source.sound_index ] };
            // emitter initialization
            if( ( soundhandle != sound_id::begin )
             && ( soundhandle != sound_id::end )
             && ( true == TestFlag( m_flags, sound_flags::looping ) ) ) {
                // main sample can be optionally set to loop
                Source.loop( true );
            }
            Source.range( m_range );
            Source.pitch( m_pitchvariation );
            update_location();
            update_soundproofing();
            // pitch and volume are adjusted on per-chunk basis
            auto const baseproperties = m_properties;
            update_crossfade( soundhandle );
            Source.sync_with( m_properties );
            if( Source.sync == sync_state::good ) {
                // all set, start playback
                Source.play();
                if( false == Source.is_playing ) {
                    // if the playback didn't start update the state counter
                    update_counter( soundhandle, -1 );
                }
            }
            else {
                // if the initial sync went wrong we skip the activation so the renderer can clean the emitter on its end
                update_counter( soundhandle, -1 );
            }
            m_properties = baseproperties;
        }
        else {
            // the emitter is either all done or was terminated early
            update_counter( Source.sounds[ Source.sound_index - 1 ], -1 );
        }
    }
}

void
sound_source::update_crossfade( sound_handle const Chunk ) {

    if( ( Chunk & sound_id::chunk ) == 0 ) {
        // bookend sounds are played at their base pitch
        m_properties.pitch = 1.f;
        return;
    }

    auto const soundpoint { compute_combined_point() };

    // NOTE: direct access to implementation details ahead, kinda fugly
    auto const chunkindex { Chunk ^ sound_id::chunk };
    auto const &chunkdata { m_soundchunks[ chunkindex ].second };

    // relative pitch adjustment
    // pitch of each chunk is modified based on ratio of the chunk's pitch to that of its neighbour
    if( soundpoint < chunkdata.threshold ) {

        if( chunkindex > 0 ) {
            // interpolate between the pitch of previous chunk and this chunk's base pitch,
            // based on how far the current soundpoint is in the range of previous chunk
            auto const &previouschunkdata{ m_soundchunks[ chunkindex - 1 ].second };
            m_properties.pitch =
                interpolate(
                    previouschunkdata.pitch / chunkdata.pitch,
                    1.f,
                    clamp(
                        ( soundpoint - previouschunkdata.threshold ) / ( chunkdata.threshold - previouschunkdata.threshold ),
                        0.f, 1.f ) );
        }
    }
    else {

        if( chunkindex < ( m_soundchunks.size() - 1 ) ) {
            // interpolate between this chunk's base pitch and the pitch of next chunk
            // based on how far the current soundpoint is in the range of this chunk
            auto const &nextchunkdata { m_soundchunks[ chunkindex + 1 ].second };
            m_properties.pitch =
                interpolate(
                    1.f,
                    nextchunkdata.pitch / chunkdata.pitch,
                    clamp(
                        ( soundpoint - chunkdata.threshold ) / ( nextchunkdata.threshold - chunkdata.threshold ),
                        0.f, 1.f ) );
        }
        else {
            // pitch of the last (or the only) chunk remains fixed throughout
            m_properties.pitch = 1.f;
        }
    }
    // if there's no crossfade sections, our work is done
    if( m_crossfaderange == 0 ) { return; }

    if( chunkindex > 0 ) {
        // chunks other than the first can have fadein
        auto const fadeinwidth { chunkdata.threshold - chunkdata.fadein };
        if( soundpoint < chunkdata.threshold ) {
            m_properties.gain *=
                interpolate(
                    0.f, 1.f,
                    clamp(
                        ( soundpoint - chunkdata.fadein ) / fadeinwidth,
                        0.f, 1.f ) );
            return;
        }
    }
    if( chunkindex < ( m_soundchunks.size() - 1 ) ) {
        // chunks other than the last can have fadeout
        // TODO: cache widths in the chunk data struct?
        // fadeout point of this chunk and activation threshold of the next are the same
        // fadein range of the next chunk and the fadeout of the processed one are the same
        auto const fadeoutwidth { chunkdata.fadeout - m_soundchunks[ chunkindex + 1 ].second.fadein };
        auto const fadeoutstart { chunkdata.fadeout - fadeoutwidth };
        if( soundpoint > fadeoutstart ) {
            m_properties.gain *=
                interpolate(
                    1.f, 0.f,
                    clamp(
                        ( soundpoint - fadeoutstart ) / fadeoutwidth,
                        0.f, 1.f ) );
            return;
        }
    }
}

// sets base volume of the emiter to specified value
sound_source &
sound_source::gain( float const Gain ) {

    m_properties.gain = clamp( Gain, 0.f, 2.f );
    return *this;
}

// returns current base volume of the emitter
float
sound_source::gain() const {

    return m_properties.gain;
}

// sets base pitch of the emitter to specified value
sound_source &
sound_source::pitch( float const Pitch ) {

    m_properties.pitch = Pitch;
    return *this;
}

bool
sound_source::empty() const {

    // NOTE: we test only the main sound, won't bother playing potential bookends if this is missing
    return ( ( sound( sound_id::main ).buffer == null_handle ) && ( m_soundchunksempty ) );
}

// returns true if the source is emitting any sound
bool
sound_source::is_playing( bool const Includesoundends ) const {

    auto isplaying { ( sound( sound_id::begin ).playing > 0 ) || ( sound( sound_id::main ).playing > 0 ) };
    if( ( false == isplaying )
     && ( false == m_soundchunks.empty() ) ) {
        // for emitters with sample tables check also if any of the chunks is active
        for( auto const &soundchunk : m_soundchunks ) {
            if( soundchunk.first.playing > 0 ) {
                isplaying = true;
                break; // one will do
            }
        }
    }
    return isplaying;
}

// returns true if the source uses sample table
bool
sound_source::is_combined() const {

    return ( ( !m_soundchunks.empty() ) && ( sound( sound_id::main ).buffer == null_handle ) );
}

// returns location of the sound source in simulation region space
glm::dvec3 const
sound_source::location() const {

    if( m_owner == nullptr ) {
        // if emitter isn't attached to any vehicle the offset variable defines location in region space
        return { m_offset };
    }
    // otherwise combine offset with the location of the carrier
    return {
        m_owner->GetPosition()
        + m_owner->VectorLeft() * m_offset.x
        + m_owner->VectorUp() * m_offset.y
        + m_owner->VectorFront() * m_offset.z };
}

// returns defined range of the sound
float const
sound_source::range() const {

    return m_range;
}

void
sound_source::update_counter( sound_handle const Sound, int const Value ) {

//    sound( Sound ).playing = std::max( 0, sound( Sound ).playing + Value );
    sound( Sound ).playing += Value;
    assert( sound( Sound ).playing >= 0 );
}

void
sound_source::update_location() {

    m_properties.location = location();
}

float const EU07_SOUNDPROOFING_STRONG { 0.25f };
float const EU07_SOUNDPROOFING_SOME { 0.65f };
float const EU07_SOUNDPROOFING_NONE { 1.f };

bool
sound_source::update_soundproofing() {
    // NOTE, HACK: current cab id can vary from -1 to +1, and we use another higher priority value for open cab window
    // we use this as modifier to force re-calculations when moving between compartments or changing window state
    int const activecab = (
        Global.CabWindowOpen ? 2 :
        FreeFlyModeFlag ? 0 :
        ( simulation::Train ?
            simulation::Train->Occupied()->ActiveCab :
            0 ) );
    // location-based gain factor:
    std::uintptr_t soundproofingstamp = reinterpret_cast<std::uintptr_t>( (
        FreeFlyModeFlag ?
            nullptr :
            ( simulation::Train ?
                simulation::Train->Dynamic() :
                nullptr ) ) )
        + activecab;

    if( soundproofingstamp == m_properties.soundproofing_stamp ) { return false; }

    // listener location has changed, calculate new location-based gain factor
    switch( m_placement ) {
        case sound_placement::general: {
            m_properties.soundproofing = EU07_SOUNDPROOFING_NONE;
            break;
        }
        case sound_placement::external: {
            m_properties.soundproofing = (
                ( ( soundproofingstamp == 0 ) || ( true == Global.CabWindowOpen ) ) ?
                    EU07_SOUNDPROOFING_NONE : // listener outside or has a window open
                    EU07_SOUNDPROOFING_STRONG ); // listener in a vehicle with windows shut
            break;
        }
        case sound_placement::internal: {
            m_properties.soundproofing = (
                soundproofingstamp == 0 ?
                    EU07_SOUNDPROOFING_STRONG : // listener outside HACK: won't be true if active vehicle has open window
                    ( simulation::Train->Dynamic() != m_owner ?
                        EU07_SOUNDPROOFING_STRONG : // in another vehicle
                        ( activecab == 0 ?
                            EU07_SOUNDPROOFING_STRONG : // listener in the engine compartment
                            EU07_SOUNDPROOFING_NONE ) ) ); // listener in the cab of the same vehicle
            break;
        }
        case sound_placement::engine: {
            m_properties.soundproofing = (
                ( ( soundproofingstamp == 0 ) || ( true == Global.CabWindowOpen ) ) ?
                    EU07_SOUNDPROOFING_SOME : // listener outside or has a window open
                    ( simulation::Train->Dynamic() != m_owner ?
                        EU07_SOUNDPROOFING_STRONG : // in another vehicle
                        ( activecab == 0 ?
                            EU07_SOUNDPROOFING_NONE : // listener in the engine compartment
                            EU07_SOUNDPROOFING_STRONG ) ) ); // listener in another compartment of the same vehicle
            break;
        }
        default: {
            // shouldn't ever land here, but, eh
            m_properties.soundproofing = EU07_SOUNDPROOFING_NONE;
            break;
        }
    }

    m_properties.soundproofing_stamp = soundproofingstamp;
    return true;
}

void
sound_source::insert( sound_handle const Sound ) {

    std::vector<sound_handle> sounds { Sound };
    return insert( std::begin( sounds ), std::end( sounds ) );
}

sound_source::sound_data &
sound_source::sound( sound_handle const Sound ) {

    return (
        ( Sound & sound_id::chunk ) == sound_id::chunk ?
            m_soundchunks[ Sound ^ sound_id::chunk ].first :
            m_sounds[ Sound ] );
}

sound_source::sound_data const &
sound_source::sound( sound_handle const Sound ) const {

    return (
        ( Sound & sound_id::chunk ) == sound_id::chunk ?
            m_soundchunks[ Sound ^ sound_id::chunk ].first :
            m_sounds[ Sound ] );
}
