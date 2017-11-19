/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "audio.h"
#include "resourcemanager.h"

class sound_source;

namespace audio {

// implementation part of the sound emitter
// TODO: generic interface base, for implementations other than openAL
struct openal_source {

// types
    using buffer_sequence = std::vector<audio::buffer_handle>;

// members
    ALuint id { audio::null_resource }; // associated AL resource
    sound_source *controller { nullptr }; // source controller 
    buffer_sequence buffers; // sequence of samples the source will emit
    int buffer_index; // currently queued sample from the buffer sequence
    bool is_playing { false };

// methods
    template <class Iterator_>
    openal_source &
        bind( sound_source *Controller, Iterator_ First, Iterator_ Last ) {
            controller = Controller;
            buffers.insert( std::end( buffers ), First, Last );
            if( id == audio::null_resource ) {
                ::alGenSources( 1, &id ); }
            // look up and queue assigned buffers
            std::vector<ALuint> bufferids;
            for( auto const buffer : buffers ) {
                bufferids.emplace_back( audio::renderer.buffer( buffer ).id ); }
            ::alSourceQueueBuffers( id, bufferids.size(), bufferids.data() );
            return *this; }
    // starts playback of queued buffers
    void
        play();
    // stops the playback
    void
        stop();
    // updates state of the source
    void
        update( int const Deltatime );
    // toggles looping of the sound emitted by the source
   void
        loop( bool const State );
    // releases bound buffers and resets state of the class variables
    // NOTE: doesn't release allocated implementation-side source
    void
        clear();
};



class openal_renderer {

public:
// destructor
    ~openal_renderer();
// methods
    // buffer methods
    // returns handle to a buffer containing audio data from specified file
    audio::buffer_handle
        fetch_buffer( std::string const &Filename );
    // provides direct access to a specified buffer
    audio::openal_buffer const &
        buffer( audio::buffer_handle const Buffer ) const;
    // core methods
    // initializes the service
    bool
        init();
    // schedules playback of provided range of samples, under control of the specified sound emitter
    template <class Iterator_>
    void
        insert( sound_source *Controller, Iterator_ First, Iterator_ Last ) {
            m_sources.emplace_back( fetch_source().bind( Controller, First, Last ) ); }
    // schedules playback of specified sample, under control of the specified sound emitter
    void
        insert( sound_source *Controller, audio::buffer_handle const Sound );
    // updates state of all active emitters
    void
        update( int const Deltatime );

private:
// types
    using source_list = std::list<audio::openal_source>;
    using source_sequence = std::stack<audio::openal_source>;
// methods
    bool
        init_caps();
    // returns an instance of implementation-side part of the sound emitter
    audio::openal_source
        fetch_source();
// members
    ALCdevice * m_device { nullptr };
    ALCcontext * m_context { nullptr };
    bool m_ready { false }; // renderer is initialized and functional

    buffer_manager m_buffers;
    // TBD: list of sources as vector, sorted by distance, for openal implementations with limited number of active sources?
    source_list m_sources;
    source_sequence m_sourcespares; // already created and currently unused sound sources
};

extern openal_renderer renderer;

} // audio

//---------------------------------------------------------------------------
