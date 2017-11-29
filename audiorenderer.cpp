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
#include "globals.h"
#include "logs.h"

namespace audio {

openal_renderer renderer;

float const EU07_SOUND_CUTOFFRANGE { 3000.f }; // 2750 m = max expected emitter spawn range, plus safety margin

// starts playback of queued buffers
void
openal_source::play() {

    ::alSourcePlay( id );
    is_playing = true;
}

// stops the playback
void
openal_source::stop() {

    loop( false );
    ::alSourceStop( id );
    is_playing = false;
}

// updates state of the source
void
openal_source::update( double const Deltatime ) {

    update_deltatime = Deltatime; // cached for time-based processing of data from the controller

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

// configures state of the source to match the provided set of properties
void
openal_source::sync_with( sound_properties const &State ) {

/*
    // velocity
    // not used yet
    glm::vec3 const velocity { ( State.location - properties.location ) / update_deltatime };
*/
    // location
    properties.location = State.location;
    auto sourceoffset { glm::vec3 { properties.location - glm::dvec3 { Global::pCameraPosition } } };
    if( glm::length2( sourceoffset ) > std::max( ( sound_range * sound_range ), ( EU07_SOUND_CUTOFFRANGE * EU07_SOUND_CUTOFFRANGE ) ) ) {
        // range cutoff check
        stop();
        return;
    }
    if( sound_range >= 0 ) {
        ::alSourcefv( id, AL_POSITION, glm::value_ptr( sourceoffset ) );
    }
    else {
        // sounds with 'unlimited' range are positioned on top of the listener
        ::alSourcefv( id, AL_POSITION, glm::value_ptr( glm::vec3() ) );
    }
    // gain
    if( ( State.placement_stamp != properties.placement_stamp )
     || ( State.base_gain != properties.base_gain ) ) {
        // gain value has changed
        properties.base_gain = State.base_gain;
        properties.placement_gain = State.placement_gain;
        properties.placement_stamp = State.placement_stamp;

        ::alSourcef( id, AL_GAIN, properties.base_gain * properties.placement_gain * Global::AudioVolume );
    }
    // pitch
    if( State.base_pitch != properties.base_pitch ) {
        // pitch value has changed
        properties.base_pitch = State.base_pitch;

        ::alSourcef( id, AL_PITCH, properties.base_pitch * pitch_variation );
    }
}

// sets max audible distance for sounds emitted by the source
void
openal_source::range( float const Range ) {

    auto const range(
        Range >= 0 ?
            Range :
            5 ); // range of -1 means sound of unlimited range, positioned at the listener
    ::alSourcef( id, AL_REFERENCE_DISTANCE, range * ( 1.f / 16.f ) );
    ::alSourcef( id, AL_ROLLOFF_FACTOR, 1.5f );
    // NOTE: we cache actual specified range, as we'll be giving 'unlimited' range special treatment
    sound_range = Range;
}

// sets modifier applied to the pitch of sounds emitted by the source
void
openal_source::pitch( float const Pitch ) {

    pitch_variation = Pitch;
    // invalidate current pitch value to enforce change of next syns
    properties.base_pitch = -1.f;
}

// toggles looping of the sound emitted by the source
void
openal_source::loop( bool const State ) {

    if( is_looping == State ) { return; }

    is_looping = State;
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
    stop();
    // ...prepare space for returned ids of unqueued buffers (not that we need that info)...
    std::vector<ALuint> bufferids;
    bufferids.resize( buffers.size() );
    // ...release the buffers and update source data to match
    ::alSourceUnqueueBuffers( id, bufferids.size(), bufferids.data() );
    buffers.clear();
    buffer_index = 0;
    // reset properties
    properties = sound_properties();
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
    //
//    ::alDistanceModel( AL_LINEAR_DISTANCE );
    ::alDistanceModel( AL_INVERSE_DISTANCE_CLAMPED );
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

// removes from the queue all sounds controlled by the specified sound emitter
void
openal_renderer::erase( sound_source const *Controller ) {

    auto source { std::begin( m_sources ) };
    while( source != std::end( m_sources ) ) {
        if( source->controller == Controller ) {
            // if the controller is the one specified, kill it
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

// updates state of all active emitters
void
openal_renderer::update( double const Deltatime ) {

    // update listener
    glm::dmat4 cameramatrix;
    Global::pCamera->SetMatrix( cameramatrix );
    auto rotationmatrix { glm::mat3{ cameramatrix } };
    glm::vec3 const orientation[] = {
        glm::vec3{ 0, 0,-1 } * rotationmatrix ,
        glm::vec3{ 0, 1, 0 } * rotationmatrix };
    ::alListenerfv( AL_ORIENTATION, reinterpret_cast<ALfloat const *>( orientation ) );
/*
    glm::dvec3 const listenerposition { Global::pCameraPosition };
    // not used yet
    glm::vec3 const velocity { ( listenerposition - m_listenerposition ) / Deltatime };
    m_listenerposition = listenerposition;
*/

    // update active emitters
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
