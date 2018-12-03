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
#include "Globals.h"
#include "Camera.h"
#include "Logs.h"
#include "utilities.h"

namespace audio {

openal_renderer renderer;

float const EU07_SOUND_CUTOFFRANGE { 3000.f }; // 2750 m = max expected emitter spawn range, plus safety margin
float const EU07_SOUND_VELOCITYLIMIT { 250 / 3.6f }; // 343 m/sec ~= speed of sound; arbitrary limit of 250 km/h

// potentially clamps length of provided vector to 343 meters
// TBD: make a generic method for utilities out of this
glm::vec3
limit_velocity( glm::vec3 const &Velocity ) {

    auto const ratio { glm::length( Velocity ) / EU07_SOUND_VELOCITYLIMIT };

    return (
        ratio > 1.f ?
            Velocity / ratio :
            Velocity );
}

// starts playback of queued buffers
void
openal_source::play() {

    if( id == audio::null_resource ) { return; } // no implementation-side source to match, no point

    ::alSourcePlay( id );

    ALint state;
    ::alGetSourcei( id, AL_SOURCE_STATE, &state );
    is_playing = ( state == AL_PLAYING );
}

// stops the playback
void
openal_source::stop() {

    if( id == audio::null_resource ) { return; } // no implementation-side source to match, no point

    loop( false );
    // NOTE: workaround for potential edge cases where ::alSourceStop() doesn't set source which wasn't yet started to AL_STOPPED
    int state;
    ::alGetSourcei( id, AL_SOURCE_STATE, &state );
    if( state == AL_INITIAL ) {
        play();
    }
    ::alSourceStop( id );
    is_playing = false;
}

// updates state of the source
void
openal_source::update( double const Deltatime, glm::vec3 const &Listenervelocity ) {

    update_deltatime = Deltatime; // cached for time-based processing of data from the controller
    if( sound_range < 0.0 ) {
        sound_velocity = Listenervelocity; // cached for doppler shift calculation
    }
/*
    // HACK: if the application gets stuck for long time loading assets the audio can gone awry.
    // terminate all sources when it happens to stay on the safe side
    if( Deltatime > 1.0 ) {
        stop();
    }
*/
    if( id != audio::null_resource ) {

        sound_change = false;
        ::alGetSourcei( id, AL_BUFFERS_PROCESSED, &sound_index );
        // for multipart sounds trim away processed sources until only one remains, the last one may be set to looping by the controller
        // TBD, TODO: instead of change flag move processed buffer ids to separate queue, for accurate tracking of longer buffer sequences
        ALuint bufferid;
        while( ( sound_index > 0 )
            && ( sounds.size() > 1 ) ) {
            ::alSourceUnqueueBuffers( id, 1, &bufferid );
            sounds.erase( std::begin( sounds ) );
            --sound_index;
            sound_change = true;
        }

        int state;
        ::alGetSourcei( id, AL_SOURCE_STATE, &state );
        is_playing = ( state == AL_PLAYING );
    }

    // request instructions from the controller
    controller->update( *this );
}

// configures state of the source to match the provided set of properties
void
openal_source::sync_with( sound_properties const &State ) {

    if( id == audio::null_resource ) {
        // no implementation-side source to match, return sync error so the controller can clean up on its end
        sync = sync_state::bad_resource;
        return;
    }
    // velocity
    if( ( update_deltatime > 0.0 )
     && ( sound_range >= 0 )
     && ( properties.location != glm::dvec3() ) ) {
        // after sound position was initialized we can start velocity calculations
        sound_velocity = limit_velocity( ( State.location - properties.location ) / update_deltatime );
    }
    // NOTE: velocity at this point can be either listener velocity for global sounds, actual sound velocity, or 0 if sound position is yet unknown
    ::alSourcefv( id, AL_VELOCITY, glm::value_ptr( sound_velocity ) );
    // location
    properties.location = State.location;
    sound_distance = properties.location - glm::dvec3 { Global.pCamera.Pos };
    if( sound_range > 0 ) {
        // range cutoff check
        auto const cutoffrange = (
            is_multipart ?
                EU07_SOUND_CUTOFFRANGE : // we keep multi-part sounds around longer, to minimize restarts as the sounds get out and back in range
                sound_range * 7.5f );
        if( glm::length2( sound_distance ) > std::min( ( cutoffrange * cutoffrange ), ( EU07_SOUND_CUTOFFRANGE * EU07_SOUND_CUTOFFRANGE ) ) ) {
            stop();
            sync = sync_state::bad_distance; // flag sync failure for the controller
            return;
        }
    }
    if( sound_range >= 0 ) {
        ::alSourcefv( id, AL_POSITION, glm::value_ptr( sound_distance ) );
    }
    else {
        // sounds with 'unlimited' range are positioned on top of the listener
        ::alSourcefv( id, AL_POSITION, glm::value_ptr( glm::vec3() ) );
    }
    // gain
    if( ( State.soundproofing_stamp != properties.soundproofing_stamp )
     || ( State.gain != properties.gain ) ) {
        // gain value has changed
        properties.gain = State.gain;
        properties.soundproofing = State.soundproofing;
        properties.soundproofing_stamp = State.soundproofing_stamp;

        ::alSourcef( id, AL_GAIN, properties.gain * properties.soundproofing );
    }
    if( sound_range > 0 ) {
        auto const rangesquared { sound_range * sound_range };
        auto const distancesquared { glm::length2( sound_distance ) };
        if( ( distancesquared > rangesquared )
         || ( false == is_in_range ) ) {
            // if the emitter is outside of its nominal hearing range or was outside of it during last check
            // adjust the volume to a suitable fraction of nominal value
            auto const fadedistance { sound_range * 0.75 };
            auto const rangefactor {
                interpolate(
                    1.f, 0.f,
                    clamp<float>(
                        ( distancesquared - rangesquared ) / ( fadedistance * fadedistance ),
                        0.f, 1.f ) ) };
            ::alSourcef( id, AL_GAIN, properties.gain * properties.soundproofing * rangefactor );
        }
        is_in_range = ( distancesquared <= rangesquared );
    }
    // pitch
    if( State.pitch != properties.pitch ) {
        // pitch value has changed
        properties.pitch = State.pitch;

        ::alSourcef( id, AL_PITCH, clamp( properties.pitch * pitch_variation, 0.1f, 10.f ) );
    }
    sync = sync_state::good;
}

// sets max audible distance for sounds emitted by the source
void
openal_source::range( float const Range ) {

    // NOTE: we cache actual specified range, as we'll be giving 'unlimited' range special treatment
    sound_range = Range;

    if( id == audio::null_resource ) { return; } // no implementation-side source to match, no point

    auto const range { (
        Range >= 0 ?
            Range :
            5 ) }; // range of -1 means sound of unlimited range, positioned at the listener
    ::alSourcef( id, AL_REFERENCE_DISTANCE, range * ( 1.f / 16.f ) );
    ::alSourcef( id, AL_ROLLOFF_FACTOR, 1.75f );
}

// sets modifier applied to the pitch of sounds emitted by the source
void
openal_source::pitch( float const Pitch ) {

    pitch_variation = Pitch;
    // invalidate current pitch value to enforce change of next syns
    properties.pitch = -1.f;
}

// toggles looping of the sound emitted by the source
void
openal_source::loop( bool const State ) {

    if( id == audio::null_resource ) { return; } // no implementation-side source to match, no point
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

    if( id != audio::null_resource ) {
        // unqueue bound buffers:
        // ensure no buffer is in use...
        stop();
        // ...prepare space for returned ids of unqueued buffers (not that we need that info)...
        std::vector<ALuint> bufferids;
        bufferids.resize( sounds.size() );
        // ...release the buffers...
        ::alSourceUnqueueBuffers( id, bufferids.size(), bufferids.data() );
    }
    // ...and reset reset the properties, except for the id of the allocated source
    // NOTE: not strictly necessary since except for the id the source data typically get discarded in next step
    auto const sourceid { id };
    *this = openal_source();
    id = sourceid;
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
    ::alDistanceModel( AL_INVERSE_DISTANCE_CLAMPED );
    // all done
    m_ready = true;
    return true;
}

// removes from the queue all sounds controlled by the specified sound emitter
void
openal_renderer::erase( sound_source const *Controller ) {

    auto source { std::begin( m_sources ) };
    while( source != std::end( m_sources ) ) {
        if( source->controller == Controller ) {
            // if the controller is the one specified, kill it
            source->clear();
            if( source->id != audio::null_resource ) {
                // keep around functional sources, but no point in doing it with the above-the-limit ones
                m_sourcespares.push( source->id );
            }
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

	ALenum err = alGetError();
	if (err != AL_NO_ERROR)
	{
		std::string errname;
		if (err == AL_INVALID_NAME)
			errname = "AL_INVALID_NAME";
		else if (err == AL_INVALID_ENUM)
			errname = "AL_INVALID_ENUM";
		else if (err == AL_INVALID_VALUE)
			errname = "AL_INVALID_VALUE";
		else if (err == AL_INVALID_OPERATION)
			errname = "AL_INVALID_OPERATION";
		else if (err == AL_OUT_OF_MEMORY)
			errname = "AL_OUT_OF_MEMORY";
		else
			errname = "unknown";
		
		ErrorLog("sound: al error: " + errname);
	}

	if (Deltatime == 0.0)
	{
		if (alcDevicePauseSOFT)
			alcDevicePauseSOFT(m_device);
		return;
	}

	if (alcDeviceResumeSOFT)
		alcDeviceResumeSOFT(m_device);

    // update listener
    // gain
    ::alListenerf( AL_GAIN, clamp( Global.AudioVolume, 0.f, 2.f ) * ( Global.iPause == 0 ? 1.f : 0.15f ) );
    // orientation
    glm::dmat4 cameramatrix;
    Global.pCamera.SetMatrix( cameramatrix );
    auto rotationmatrix { glm::mat3{ cameramatrix } };
    glm::vec3 const orientation[] = {
        glm::vec3{ 0, 0,-1 } * rotationmatrix ,
        glm::vec3{ 0, 1, 0 } * rotationmatrix };
    ::alListenerfv( AL_ORIENTATION, reinterpret_cast<ALfloat const *>( orientation ) );
    // velocity
    if( Deltatime > 0 ) {
        glm::dvec3 const listenerposition { Global.pCamera.Pos };
        glm::dvec3 const listenermovement { listenerposition - m_listenerposition };
        m_listenerposition = listenerposition;
        m_listenervelocity = (
            glm::length( listenermovement ) < 1000.0 ? // large jumps are typically camera changes
                limit_velocity( listenermovement / Deltatime ) :
                glm::vec3() );
        ::alListenerfv( AL_VELOCITY, reinterpret_cast<ALfloat const *>( glm::value_ptr( m_listenervelocity ) ) );
    }

    // update active emitters
    auto source { std::begin( m_sources ) };
    while( source != std::end( m_sources ) ) {
        // update each source
        source->update( Deltatime, m_listenervelocity );
        // if after the update the source isn't playing, put it away on the spare stack, it's done
        if( false == source->is_playing ) {
            source->clear();
            if( source->id != audio::null_resource ) {
                // keep around functional sources, but no point in doing it with the above-the-limit ones
                m_sourcespares.push( source->id );
            }
            source = m_sources.erase( source );
        }
        else {
            // otherwise proceed through the list normally
            ++source;
        }
    }

	if (alProcessUpdatesSOFT)
	{
		alProcessUpdatesSOFT();
		alDeferUpdatesSOFT();
	}
}

// returns an instance of implementation-side part of the sound emitter
audio::openal_source
openal_renderer::fetch_source() {

    audio::openal_source newsource;
    if( false == m_sourcespares.empty() ) {
        // reuse (a copy of) already allocated source
        newsource.id = m_sourcespares.top();
        m_sourcespares.pop();
    }
    if( newsource.id == audio::null_resource ) {
        // if there's no source to reuse, try to generate a new one
        ::alGenSources( 1, &( newsource.id ) );
    }
    if( newsource.id == audio::null_resource ) {
		alGetError();
        // if we still don't have a working source, see if we can sacrifice an already active one
        // under presumption it's more important to play new sounds than keep the old ones going
        // TBD, TODO: for better results we could use range and/or position for the new sound
        // to better weight whether the new sound is really more important
        auto leastimportantsource { std::end( m_sources ) };
        auto leastimportantweight { std::numeric_limits<float>::max() };

        for( auto source { std::begin( m_sources ) }; source != std::cend( m_sources ); ++source ) {

            if( ( source->id == audio::null_resource )
             || ( true == source->is_multipart )
             || ( false == source->is_playing ) ) {

                continue;
            }
            auto const sourceweight { (
                source->sound_range > 0 ?
                    ( source->sound_range * source->sound_range ) / ( glm::length2( source->sound_distance ) + 1 ) :
                    std::numeric_limits<float>::max() ) };
            if( sourceweight < leastimportantweight ) {
                leastimportantsource = source;
                leastimportantweight = sourceweight;
            }
        }
        if( ( leastimportantsource != std::end( m_sources ) )
         && ( leastimportantweight < 1.f ) ) {
            // only accept the candidate if it's outside of its nominal hearing range
            leastimportantsource->stop();
            // HACK: dt of 0 is a roundabout way to notify the controller its emitter has stopped
            leastimportantsource->update( 0, m_listenervelocity );
            leastimportantsource->clear();
            // we should be now free to grab the id and get rid of the remains
            newsource.id = leastimportantsource->id;
            m_sources.erase( leastimportantsource );
        }
    }

    if( newsource.id == audio::null_resource ) {
        // for sources with functional emitter reset emitter parameters from potential last use
        ::alSourcef( newsource.id, AL_PITCH, 1.f );
        ::alSourcef( newsource.id, AL_GAIN, 1.f );
        ::alSourcefv( newsource.id, AL_POSITION, glm::value_ptr( glm::vec3{ 0.f } ) );
        ::alSourcefv( newsource.id, AL_VELOCITY, glm::value_ptr( glm::vec3{ 0.f } ) );
    }

    return newsource;
}

bool
openal_renderer::init_caps() {

    // NOTE: default value of audio renderer variable is empty string, meaning argument of NULL i.e. 'preferred' device
    m_device = ::alcOpenDevice( Global.AudioRenderer.c_str() );
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

	ALCint attr[3] = { ALC_MONO_SOURCES, Global.audio_max_sources, 0 }; // request more sounds

    m_context = ::alcCreateContext( m_device, attr );
    if( m_context == nullptr ) {
        ErrorLog( "Failed to create audio context" );
        return false;
    }

	if (!alcMakeContextCurrent(m_context))
	{
		ErrorLog("sound: cannot select context");
		return false;
	}

	if (alIsExtensionPresent("AL_SOFT_deferred_updates"))
	{
		alDeferUpdatesSOFT = (void(*)())alGetProcAddress("alDeferUpdatesSOFT");
		alProcessUpdatesSOFT = (void(*)())alGetProcAddress("alProcessUpdatesSOFT");
	}
	if (!alDeferUpdatesSOFT || !alProcessUpdatesSOFT)
		WriteLog("sound: warning: extension AL_SOFT_deferred_updates not found");

	if (alcIsExtensionPresent(m_device, "ALC_SOFT_pause_device"))
	{
		alcDevicePauseSOFT = (void(*)(ALCdevice*))alcGetProcAddress(m_device, "alcDevicePauseSOFT");
		alcDeviceResumeSOFT = (void(*)(ALCdevice*))alcGetProcAddress(m_device, "alcDeviceResumeSOFT");
	}
	if (!alcDevicePauseSOFT || !alcDeviceResumeSOFT)
		WriteLog("sound: warning: extension ALC_SOFT_pause_device not found");

    return true;
}

} // audio
