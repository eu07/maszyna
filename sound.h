/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "audiorenderer.h"
#include "classes.h"
#include "names.h"

float const EU07_SOUND_CABCONTROLSCUTOFFRANGE { 7.5f };
float const EU07_SOUND_BRAKINGCUTOFFRANGE { 100.f };
float const EU07_SOUND_RUNNINGNOISECUTOFFRANGE { 200.f };

enum class sound_type {
    single,
    multipart
};

enum sound_parameters {
    range     = 0x1,
    amplitude = 0x2,
    frequency = 0x4
};

enum sound_flags {
    looping = 0x1, // the main sample will be looping; implied for multi-sounds
    exclusive = 0x2 // the source won't dispatch more than one active instance of the sound; implied for multi-sounds
};

enum class sound_placement {
    general, // source is equally audible in potential carrier and outside of it
    internal, // source is located inside of the carrier, and less audible when the listener is outside
    engine, // source is located in the engine compartment, less audible when the listener is outside and even less in the cabs
    external // source is located on the outside of the carrier, and less audible when the listener is inside
};

// mini controller and audio dispatcher; issues play commands for the audio renderer,
// updates parameters of created audio emitters for the playback duration
// TODO: move to simulation namespace after clean up of owner classes
class sound_source {

public:
// constructors
    sound_source( sound_placement const Placement, float const Range = 50.f );

// destructor
    ~sound_source();

// methods
    // restores state of the class from provided data stream
    sound_source &
        deserialize( cParser &Input, sound_type const Legacytype, int const Legacyparameters = 0 );
    sound_source &
        deserialize( std::string const &Input, sound_type const Legacytype, int const Legacyparameters = 0 );
    // issues contextual play commands for the audio renderer
    void
        play( int const Flags = 0 );
    // stops currently active play commands controlled by this emitter
    void
        stop();
    // adjusts parameters of provided implementation-side sound source
    void
        update( audio::openal_source &Source );
    // sets base volume of the emiter to specified value
    sound_source &
        gain( float const Gain );
    // returns current base volume of the emitter
    float
        gain() const;
    // sets base pitch of the emitter to specified value
    sound_source &
        pitch( float const Pitch );
    // owner setter/getter
    void
        owner( TDynamicObject const *Owner );
    TDynamicObject const *
        owner() const;
    // sound source offset setter/getter
    void
        offset( glm::vec3 const Offset );
    glm::vec3 const &
        offset() const;
    // sound source name setter/getter
    void
        name( std::string Name );
    std::string const &
        name() const;
    // returns true if there isn't any sound buffer associated with the object, false otherwise
    bool
        empty() const;
    // returns true if the source is emitting any sound; by default doesn't take into account optional ending soudnds
    bool
        is_playing( bool const Includesoundends = false ) const;
    // returns location of the sound source in simulation region space
    glm::dvec3 const
        location() const;

// members
    float m_amplitudefactor { 1.f }; // helper, value potentially used by gain calculation
    float m_amplitudeoffset { 0.f }; // helper, value potentially used by gain calculation
    float m_frequencyfactor { 1.f }; // helper, value potentially used by pitch calculation
    float m_frequencyoffset { 0.f }; // helper, value potentially used by pitch calculation

private:
// types
    struct sound_data {
        audio::buffer_handle buffer { null_handle };
        int playing { 0 }; // number of currently active sample instances
    };

// methods
    // extracts name of the sound file from provided data stream
    std::string
        deserialize_filename( cParser &Input );
    void
        update_counter( audio::buffer_handle const Buffer, int const Value );
    void
        update_location();
    // potentially updates area-based gain factor of the source. returns: true if location has changed
    bool
        update_soundproofing();
    void
        insert( audio::buffer_handle Buffer );
    template <class Iterator_>
    void
        insert( Iterator_ First, Iterator_ Last ) {

        audio::renderer.insert( this, First, Last );
        update_counter( *First, 1 );
    }

// members
    TDynamicObject const * m_owner { nullptr }; // optional, the vehicle carrying this sound source
    glm::vec3 m_offset; // relative position of the source, either from the owner or the region centre
    sound_placement m_placement;
    float m_range { 50.f }; // audible range of the emitted sounds
    std::string m_name;
    int m_flags { 0 }; // requested playback parameters
    sound_properties m_properties; // current properties of the emitted sounds
    float m_pitchvariation { 0.f }; // emitter-specific shift in base pitch
    bool m_stop { false }; // indicates active sample instances should be terminated
    sound_data m_soundmain; // main sound emitted by the source
    sound_data m_soundbegin; // optional, sound emitted before the main sound
    sound_data m_soundend; // optional, sound emitted after the main sound
    // TODO: table of samples with associated values, activated when controlling variable matches the value
};

// owner setter/getter
inline void sound_source::owner( TDynamicObject const *Owner ) { m_owner = Owner; }
inline TDynamicObject const * sound_source::owner() const { return m_owner; }
// sound source offset setter/getter
inline void sound_source::offset( glm::vec3 const Offset ) { m_offset = Offset; }
inline glm::vec3 const & sound_source::offset() const { return m_offset; }
// sound source name setter/getter
inline void sound_source::name( std::string Name ) { m_name = Name; }
inline std::string const & sound_source::name() const { return m_name; }

// collection of sound sources present in the scene
class sound_table : public basic_table<sound_source> {

};

//---------------------------------------------------------------------------
