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
            m_soundmain = audio::renderer.fetch_buffer( Input.getToken<std::string>( true, "\n\r\t ,;" ) );
            break;
        }
        case sound_type::multipart: {
            // three samples: start, middle, stop
            m_soundbegin = audio::renderer.fetch_buffer( Input.getToken<std::string>( true, "\n\r\t ,;" ) );
            m_soundmain = audio::renderer.fetch_buffer( Input.getToken<std::string>( true, "\n\r\t ,;" ) );
            m_soundend = audio::renderer.fetch_buffer( Input.getToken<std::string>( true, "\n\r\t ,;" ) );
            break;
        }
        default: {
            break;
        }
    }

    if( Legacyparameters & sound_parameters::range ) {
        Input.getTokens( 1, false );
    }
    if( Legacyparameters & sound_parameters::amplitude ) {
        Input.getTokens( 2, false );
    }
    if( Legacyparameters & sound_parameters::frequency ) {
        Input.getTokens( 2, false );
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

    m_flags = Flags;

    switch( m_stage ) {
        case stage::none:
        case stage::restart: {
            if( m_soundbegin != null_handle ) {
                std::vector<audio::buffer_handle> bufferlist{ m_soundbegin, m_soundmain };
                audio::renderer.insert( this, std::begin( bufferlist ), std::end( bufferlist ) );
            }
            else {
                audio::renderer.insert( this, m_soundmain );
            }
            break;
        }
        case stage::main: {
            // TODO: schedule another main sample playback, or a suitable sample from the table for combined sources
            break;
        }
        case stage::end: {
            // schedule stop of current sequence end sample...
            m_stage = stage::restart;
            // ... and queue startup or main sound again
            return play( Flags );
            break;
        }
        default: {
            break;
        }
    }
}

// stops currently active play commands controlled by this emitter
void
sound_source::stop() {

    if( ( m_stage == stage::none )
     || ( m_stage == stage::end ) ) {
        return;
    }

    switch( m_stage ) {
        case stage::begin:
        case stage::main: {
            // set the source to kill any currently active sounds
            m_stage = stage::cease;
            if( m_soundend != null_handle ) {
                // and if there's defined sample for the sound end, play it instead
                audio::renderer.insert( this, m_soundend );
            }
            break;
        }
        case stage::end: {
            // set the source to kill any currently active sounds
            m_stage = stage::cease;
            break;
        }
        default: {
            break;
        }
    }
}

// adjusts parameters of provided implementation-side sound source
void
sound_source::update( audio::openal_source &Source ) {

    if( true == Source.is_playing ) {

        // kill the sound if requested
        if( m_stage == stage::cease ) {
            Source.stop();
            return;
        }
        // TODO: positional update
        if( m_soundbegin != null_handle ) {
            // multipart sound
            // detect the moment when the sound moves from startup sample to the main
            if( ( m_stage == stage::begin )
             && ( Source.buffers[ Source.buffer_index ] == m_soundmain ) ) {
                // when it happens update active sample flags, and activate the looping
                Source.loop( true );
                m_stage = stage::main;
            }
        }
    }
    else {
        // if the emitter isn't playing it's either done or wasn't yet started
        // we can determine this from number of processed buffers
        if( Source.buffer_index != Source.buffers.size() ) {
            auto const buffer { Source.buffers[ Source.buffer_index ] };
                 if( buffer == m_soundbegin ) { m_stage = stage::begin; }
            // TODO: take ito accound sample table for combined sounds
            else if( buffer == m_soundmain )  { m_stage = stage::main; }
            else if( buffer == m_soundend )   { m_stage = stage::end; }
            // TODO: emitter initialization
            if( ( buffer == m_soundmain )
             && ( true == TestFlag( m_flags, sound_flags::looping ) ) ) {
                // main sample can be optionally set to loop
                Source.loop( true );
            }
            // all set, start playback
            Source.play();
        }
        else {
            //
            auto const buffer { Source.buffers[ Source.buffer_index - 1 ] };
            if( ( buffer == m_soundend )
             || ( ( m_soundend == null_handle )
               && ( buffer == m_soundmain ) ) ) {
                m_stage = stage::none;
            }
        }
    }
}

bool
sound_source::empty() const {

    // NOTE: we test only the main sound, won't bother playing potential bookends if this is missing
    // TODO: take into account presence of sample table, for combined sounds
    return ( m_soundmain == null_handle );
}

// returns true if the source is emitting any sound
bool
sound_source::is_playing() const {

    return ( m_stage != stage::none );
}

//---------------------------------------------------------------------------
