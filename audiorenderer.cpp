/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"

#include "audiorenderer.h"
#include "sound.h"
#include "logs.h"

namespace audio {

openal_renderer renderer;

// starts playback of queued buffers
void
openal_source::play() {

    ::alSourcePlay( id );
    is_playing = true;
}

// stops the playback
void
openal_source::stop() {

    ::alSourcei( id, AL_LOOPING, AL_FALSE );
    ::alSourceStop( id );
    // NOTE: we don't update the is_playing flag
    // this way the state will change only on next update loop,
    // giving the controller a chance to properly change state from cease to none
    // even with multiple active sounds under control
}

// updates state of the source
void
openal_source::update( int const Deltatime ) {

    // TODO: test whether the emitter was within range during the last tick, potentially update the counter and flag it for timeout
    ::alGetSourcei( id, AL_BUFFERS_PROCESSED, &buffer_index );
    // for multipart sounds trim away processed sources until only one remains, the last one may be set to looping by the controller
    ALuint bufferid;
    while( ( buffer_index > 0 )
        && ( buffers.size() > 1 ) ) {
        ::alSourceUnqueueBuffers( id, 1, &bufferid );
        buffers.erase( std::begin( buffers ) );
        --buffer_index;
    }
    int state;
    ::alGetSourcei( id, AL_SOURCE_STATE, &state );
    is_playing = ( state == AL_PLAYING );
    // request instructions from the controller
    controller->update( *this );
}

// toggles looping of the sound emitted by the source
void
openal_source::loop( bool const State ) {

    ::alSourcei(
        id,
        AL_LOOPING,
        ( State ?
            AL_TRUE :
            AL_FALSE ) );
}

// releases bound buffers and resets state of the class variables
// NOTE: doesn't release allocated implementation-side source
void
openal_source::clear() {

    controller = nullptr;
    // unqueue bound buffers:
    // ensure no buffer is in use...
    ::alSourcei( id, AL_LOOPING, AL_FALSE );
    ::alSourceStop( id );
    is_playing = false;
    // ...prepare space for returned ids of unqueued buffers (not that we need that info)...
    std::vector<ALuint> bufferids;
    bufferids.resize( buffers.size() );
    // ...release the buffers and update source data to match
    ::alSourceUnqueueBuffers( id, bufferids.size(), bufferids.data() );
    buffers.clear();
    buffer_index = 0;
}



openal_renderer::~openal_renderer() {

    ::alcMakeContextCurrent( nullptr );

    if( m_context != nullptr ) { ::alcDestroyContext( m_context ); }
    if( m_device != nullptr )  { ::alcCloseDevice( m_device ); }
}

audio::buffer_handle
openal_renderer::fetch_buffer( std::string const &Filename ) {

    return m_buffers.create( Filename );
}

// provides direct access to a specified buffer
audio::openal_buffer const &
openal_renderer::buffer( audio::buffer_handle const Buffer ) const {

    return m_buffers.buffer( Buffer );
}

// initializes the service
bool
openal_renderer::init() {

    if( true == m_ready ) {
        // already initialized and enabled
        return true;
    }
    if( false == init_caps() ) {
        // basic initialization failed
        return false;
    }
    // all done
    m_ready = true;
    return true;
}

// schedules playback of specified sample, under control of the specified emitter
void
openal_renderer::insert( sound_source *Controller, audio::buffer_handle const Sound ) {

    audio::openal_source::buffer_sequence buffers { Sound };
    return
        insert(
            Controller,
            std::begin( buffers ), std::end( buffers ) );
}

// updates state of all active emitters
void
openal_renderer::update( int const Deltatime ) {

    auto source { std::begin( m_sources ) };
    while( source != std::end( m_sources ) ) {
        // update each source
        source->update( Deltatime );
        // if after the update the source isn't playing, put it away on the spare stack, it's done
        if( false == source->is_playing ) {
            source->clear();
            m_sourcespares.push( *source );
            source = m_sources.erase( source );
        }
        else {
            // otherwise proceed through the list normally
            ++source;
        }
    }
}

// returns an instance of implementation-side part of the sound emitter
audio::openal_source
openal_renderer::fetch_source() {

    audio::openal_source soundsource;
    if( false == m_sourcespares.empty() ) {
        // reuse (a copy of) already allocated source
        soundsource = m_sourcespares.top();
        m_sourcespares.pop();
    }
    return soundsource;
}

bool
openal_renderer::init_caps() {

    // select the "preferred device"
    // TODO: support for opening config-specified device
    m_device = ::alcOpenDevice( nullptr );
    if( m_device == nullptr ) {
        ErrorLog( "Failed to obtain audio device" );
        return false;
    }

    ALCint versionmajor, versionminor;
    ::alcGetIntegerv( m_device, ALC_MAJOR_VERSION, 1, &versionmajor );
    ::alcGetIntegerv( m_device, ALC_MINOR_VERSION, 1, &versionminor );
    auto const oalversion { std::to_string( versionmajor ) + "." + std::to_string( versionminor ) };

    WriteLog(
        "Audio Renderer: " + std::string { (char *)::alcGetString( m_device, ALC_DEVICE_SPECIFIER ) }
        + " OpenAL Version: " + oalversion );

    WriteLog( "Supported extensions: " + std::string{ (char *)::alcGetString( m_device, ALC_EXTENSIONS ) } );

    m_context = ::alcCreateContext( m_device, nullptr );
    if( m_context == nullptr ) {
        ErrorLog( "Failed to create audio context" );
        return false;
    }

    return ( ::alcMakeContextCurrent( m_context ) == AL_TRUE );
}

} // audio

//---------------------------------------------------------------------------
