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
#include "ResourceManager.h"
#include "uitranscripts.h"

#define EU07_SOUND_PROOFINGUSESRANGE

class opengl_renderer;
class sound_source;

using uint32_sequence = std::vector<std::uint32_t>;

enum class sound_category : unsigned int {
    unknown = 0, // source gain is unaltered
    vehicle, // source gain is altered by vehicle sound volume modifier
    local, // source gain is altered by positional environment sound volume modifier
    ambient, // source gain is altered by ambient environment sound volume modifier
};

// sound emitter state sync item
struct sound_properties {
    glm::dvec3 location;
    float pitch { 1.f };
    sound_category category { sound_category::unknown };
    float gain { 1.f };
    float soundproofing { 1.f };
    std::uintptr_t soundproofing_stamp { ~( std::uintptr_t{ 0 } ) };
};

enum class sync_state {
    good,
    bad_distance,
    bad_resource
};

namespace audio {

// implementation part of the sound emitter
// TODO: generic interface base, for implementations other than openAL
struct openal_source {

    friend class openal_renderer;

// types
    using buffer_sequence = std::vector<audio::buffer_handle>;

// members
    ALuint id { audio::null_resource }; // associated AL resource
    sound_source *controller { nullptr }; // source controller 
    uint32_sequence sounds; // 
//    buffer_sequence buffers; // sequence of samples the source will emit
    int sound_index { 0 }; // currently queued sample from the buffer sequence
    bool sound_change { false }; // indicates currently queued sample has changed
    bool is_playing { false };
    bool is_looping { false };
    sound_properties properties;
    sync_state sync { sync_state::good };
// constructors
    openal_source() = default;
// methods
    template <class Iterator_>
    openal_source &
        bind( sound_source *Controller, uint32_sequence Sounds, Iterator_ First, Iterator_ Last );
    // starts playback of queued buffers
    void
        play();
    // updates state of the source
    void
        update( double const Deltatime, glm::vec3 const &Listenervelocity );
    // configures state of the source to match the provided set of properties
    void
        sync_with( sound_properties const &State );
    // stops the playback
    void
        stop();
    // toggles looping of the sound emitted by the source
   void
        loop( bool const State );
    // sets max audible distance for sounds emitted by the source
   void
       range( float const Range );
   // sets modifier applied to the pitch of sounds emitted by the source
   void
       pitch( float const Pitch );
    // releases bound buffers and resets state of the class variables
    // NOTE: doesn't release allocated implementation-side source
    void
        clear();

private:
// members
    double update_deltatime { 0.0 }; // time delta of most current update
    float pitch_variation { 1.f }; // emitter-specific variation of the base pitch
    float sound_range { 50.f }; // cached audible range of the emitted samples
    glm::vec3 sound_distance { 0.f }; // cached distance between sound and the listener
    glm::vec3 sound_velocity { 0.f }; // sound movement vector
    bool is_in_range { false }; // helper, indicates the source was recently within audible range
    bool is_multipart { false }; // multi-part sounds are kept alive at longer ranges
};



class openal_renderer {

    friend opengl_renderer;

public:
// constructors
    openal_renderer() = default;
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
        insert( Iterator_ First, Iterator_ Last, sound_source *Controller, uint32_sequence Sounds ) {
            m_sources.emplace_back( fetch_source().bind( Controller, Sounds, First, Last ) ); }
    // removes from the queue all sounds controlled by the specified sound emitter
    void
        erase( sound_source const *Controller );
    // updates state of all active emitters
    void
        update( double const Deltatime );

    glm::dvec3 cached_camerapos;

private:
// types
    using source_list = std::list<audio::openal_source>;
    using source_sequence = std::stack<ALuint>;
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
/*
    glm::dvec3 m_listenerposition;
*/
    glm::vec3 m_listenervelocity;
    bool m_freeflymode{ true };
    bool m_windowopen{ true };
    int m_activecab{ 0 };

    buffer_manager m_buffers;
    // TBD: list of sources as vector, sorted by distance, for openal implementations with limited number of active sources?
    source_list m_sources;
    source_sequence m_sourcespares; // already created and currently unused sound sources

	void (*alDeferUpdatesSOFT)() = nullptr;
	void (*alProcessUpdatesSOFT)() = nullptr;
	void (*alcDevicePauseSOFT)(ALCdevice*) = nullptr;
	void (*alcDeviceResumeSOFT)(ALCdevice*) = nullptr;
};

extern openal_renderer renderer;
extern bool event_volume_change;

} // audio

//---------------------------------------------------------------------------
