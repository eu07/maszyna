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
#include "globals.h"
#include "world.h"
#include "train.h"

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

    return deserialize( cParser{ Input }, Legacytype, Legacyparameters );
}

sound_source &
sound_source::deserialize( cParser &Input, sound_type const Legacytype, int const Legacyparameters ) {

    // TODO: implement block type config parsing
    switch( Legacytype ) {
        case sound_type::single: {
            // single sample only
            m_soundmain.buffer = audio::renderer.fetch_buffer( Input.getToken<std::string>( true, "\n\r\t ,;" ) );
            break;
        }
        case sound_type::multipart: {
            // three samples: start, middle, stop
            m_soundbegin.buffer = audio::renderer.fetch_buffer( Input.getToken<std::string>( true, "\n\r\t ,;" ) );
            m_soundmain.buffer = audio::renderer.fetch_buffer( Input.getToken<std::string>( true, "\n\r\t ,;" ) );
            m_soundend.buffer = audio::renderer.fetch_buffer( Input.getToken<std::string>( true, "\n\r\t ,;" ) );
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

    return *this;
}

// issues contextual play commands for the audio renderer
void
sound_source::play( int const Flags ) {

    if( ( false == Global::bSoundEnabled )
     || ( true == empty() ) ) {
        // if the sound is disabled altogether or nothing can be emitted from this source, no point wasting time
        return;
    }
    if( m_range > 0 ) {
        auto const cutoffrange{ (
            ( m_soundbegin.buffer != null_handle ) && ( Flags & sound_flags::looping ) ?
                m_range * 10 : // larger margin to let the startup sample finish playing at safe distance
                m_range * 5 ) };
        if( glm::length2( location() - glm::dvec3{ Global::pCameraPosition } ) > std::min( 2750.f * 2750.f, cutoffrange * cutoffrange ) ) {
            // drop sounds from beyond sensible and/or audible range
            return;
        }
    }

    // initialize emitter-specific pitch variation if it wasn't yet set
    if( m_pitchvariation == 0.f ) {
        m_pitchvariation = 0.01f * static_cast<float>( Random( 95, 105 ) );
    }

    m_flags = Flags;

    if( false == is_playing() ) {
        // dispatch appropriate sound
        // TODO: support for parameter-driven sound table
        if( m_soundbegin.buffer != null_handle ) {
            std::vector<audio::buffer_handle> bufferlist { m_soundbegin.buffer, m_soundmain.buffer };
            insert( std::begin( bufferlist ), std::end( bufferlist ) );
        }
        else {
            insert( m_soundmain.buffer );
        }
    }
    else {

        if( ( m_soundbegin.buffer == null_handle )
         && ( ( m_flags & ( sound_flags::exclusive | sound_flags::looping ) ) == 0 ) ) {
            // for single part non-looping samples we allow spawning multiple instances, if not prevented by set flags
            insert( m_soundmain.buffer );
        }
    }
}

// stops currently active play commands controlled by this emitter
void
sound_source::stop() {

    if( false == is_playing() ) { return; }

    m_stop = true;

    if( ( m_soundend.buffer != null_handle )
     && ( m_soundend.buffer != m_soundmain.buffer ) ) { // end == main can happen in malformed legacy cases
        // spawn potentially defined sound end sample, if the emitter is currently active
        insert( m_soundend.buffer );
    }
}

// adjusts parameters of provided implementation-side sound source
void
sound_source::update( audio::openal_source &Source ) {

    if( true == Source.is_playing ) {

        // kill the sound if requested
        if( true == m_stop ) {
            Source.stop();
            update_counter( Source.buffers[ Source.buffer_index ], -1 );
            if( false == is_playing() ) {
                m_stop = false;
            }
            return;
        }

        if( m_soundbegin.buffer != null_handle ) {
            // potentially a multipart sound
            // detect the moment when the sound moves from startup sample to the main
            if( ( false == Source.is_looping )
             && ( Source.buffers[ Source.buffer_index ] == m_soundmain.buffer ) ) {
                // when it happens update active sample flags, and activate the looping
                Source.loop( true );
                --( m_soundbegin.playing );
                ++( m_soundmain.playing );
            }
        }

        // check and update if needed current sound properties
        update_location();
        update_placement_gain();
        Source.sync_with( m_properties );
        if( false == Source.is_synced ) {
            // if the sync went wrong we let the renderer kill its part of the emitter, and update our playcounter(s) to match
            update_counter( Source.buffers[ Source.buffer_index ], -1 );
        }

    }
    else {
        // if the emitter isn't playing it's either done or wasn't yet started
        // we can determine this from number of processed buffers
        if( Source.buffer_index != Source.buffers.size() ) {
            auto const buffer { Source.buffers[ Source.buffer_index ] };
            // emitter initialization
            if( ( buffer == m_soundmain.buffer )
             && ( true == TestFlag( m_flags, sound_flags::looping ) ) ) {
                // main sample can be optionally set to loop
                Source.loop( true );
            }
            Source.range( m_range );
            Source.pitch( m_pitchvariation );
            update_location();
            update_placement_gain();
            Source.sync_with( m_properties );
            if( true == Source.is_synced ) {
                // all set, start playback
                Source.play();
            }
            else {
                // if the initial sync went wrong we skip the activation so the renderer can clean the emitter on its end
                update_counter( buffer, -1 );
            }
        }
        else {
            auto const buffer { Source.buffers[ Source.buffer_index - 1 ] };
            update_counter( buffer, -1 );
        }
    }
}

// sets base volume of the emiter to specified value
sound_source &
sound_source::gain( float const Gain ) {

    m_properties.base_gain = clamp( Gain, 0.f, 2.f );
    return *this;
}

// returns current base volume of the emitter
float
sound_source::gain() const {

    return m_properties.base_gain;
}

// sets base pitch of the emitter to specified value
sound_source &
sound_source::pitch( float const Pitch ) {

    m_properties.base_pitch = clamp( Pitch, 0.1f, 10.f );
    return *this;
}

bool
sound_source::empty() const {

    // NOTE: we test only the main sound, won't bother playing potential bookends if this is missing
    // TODO: take into account presence of sample table, for combined sounds
    return ( m_soundmain.buffer == null_handle );
}

// returns true if the source is emitting any sound
bool
sound_source::is_playing( bool const Includesoundends ) const {

    return ( ( m_soundbegin.playing + m_soundmain.playing ) > 0 );
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

void
sound_source::update_counter( audio::buffer_handle const Buffer, int const Value ) {

         if( Buffer == m_soundbegin.buffer ) { m_soundbegin.playing += Value; }
    // TODO: take ito accound sample table for combined sounds
    else if( Buffer == m_soundmain.buffer )  { m_soundmain.playing += Value; }
    else if( Buffer == m_soundend.buffer )   { m_soundend.playing += Value; }
}

float const EU07_SOUNDGAIN_LOW { 0.2f };
float const EU07_SOUNDGAIN_MEDIUM { 0.65f };
float const EU07_SOUNDGAIN_FULL { 1.f };

void
sound_source::update_location() {

    m_properties.location = location();
}

bool
sound_source::update_placement_gain() {

    // NOTE, HACK: current cab id can vary from -1 to +1
    // we use this as modifier to force re-calculations when moving between compartments
    int const activecab = (
        FreeFlyModeFlag ?
            0 :
            ( Global::pWorld->train() ?
                Global::pWorld->train()->Dynamic()->MoverParameters->ActiveCab :
                0 ) );
    // location-based gain factor:
    std::uintptr_t placementstamp = reinterpret_cast<std::uintptr_t>( (
        FreeFlyModeFlag ?
            nullptr :
            ( Global::pWorld->train() ?
                Global::pWorld->train()->Dynamic() :
                nullptr ) ) )
        + activecab;

    if( placementstamp == m_properties.placement_stamp ) { return false; }

    // listener location has changed, calculate new location-based gain factor
    switch( m_placement ) {
        case sound_placement::general: {
            m_properties.placement_gain = EU07_SOUNDGAIN_FULL;
            break;
        }
        case sound_placement::external: {
            m_properties.placement_gain = (
                placementstamp == 0 ?
                    EU07_SOUNDGAIN_FULL : // listener outside
                    EU07_SOUNDGAIN_LOW ); // listener in a vehicle
            break;
        }
        case sound_placement::internal: {
            m_properties.placement_gain = (
                placementstamp == 0 ?
                    EU07_SOUNDGAIN_LOW : // listener outside
                    ( Global::pWorld->train()->Dynamic() != m_owner ?
                        EU07_SOUNDGAIN_LOW : // in another vehicle
                        ( activecab == 0 ?
                            EU07_SOUNDGAIN_LOW : // listener in the engine compartment
                            EU07_SOUNDGAIN_FULL ) ) ); // listener in the cab of the same vehicle
            break;
        }
        case sound_placement::engine: {
            m_properties.placement_gain = (
                placementstamp == 0 ?
                EU07_SOUNDGAIN_MEDIUM : // listener outside
                ( Global::pWorld->train()->Dynamic() != m_owner ?
                    EU07_SOUNDGAIN_LOW : // in another vehicle
                    ( activecab == 0 ?
                        EU07_SOUNDGAIN_FULL : // listener in the engine compartment
                        EU07_SOUNDGAIN_LOW ) ) ); // listener in another compartment of the same vehicle
            break;
        }
        default: {
            // shouldn't ever land here, but, eh
            m_properties.placement_gain = EU07_SOUNDGAIN_FULL;
            break;
        }
    }

    m_properties.placement_stamp = placementstamp;
    return true;
}

void
sound_source::insert( audio::buffer_handle Buffer ) {

    std::vector<audio::buffer_handle> buffers { Buffer };
    return insert( std::begin( buffers ), std::end( buffers ) );
}

//---------------------------------------------------------------------------
