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
        stop( bool const Skipend = false );
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
    // returns true if the source uses sample table
    bool
        is_combined() const;
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
        audio::buffer_handle buffer;
        int playing; // number of currently active sample instances
    };

    struct chunk_data {
        int threshold; // nominal point of activation for the given chunk
        float fadein; // actual activation point for the given chunk
        float fadeout; // actual end point of activation range for the given chunk
        float pitch; // base pitch of the chunk
    };

    using soundchunk_pair = std::pair<sound_data, chunk_data>;

    using sound_handle = std::uint32_t;
    enum sound_id : std::uint32_t {
        begin,
        main,
        end,
        // 31 bits for index into relevant array, msb selects between the sample table and the basic array
        chunk = ( 1u << 31 )
    };

// methods
    // extracts name of the sound file from provided data stream
    std::string
        deserialize_filename( cParser &Input );
    // imports member data pair from the provided data stream
    bool
        deserialize_mapping( cParser &Input );
    // imports values for initial, main and ending sounds from provided data stream
    void
        deserialize_soundset( cParser &Input );
    // issues contextual play commands for the audio renderer
    void
        play_basic();
    void
        play_combined();
    // calculates requested sound point, used to select specific sample from the sample table
    float
        compute_combined_point() const;
    void
        update_basic( audio::openal_source &Source );
    void
        update_combined( audio::openal_source &Source );
    void
        update_crossfade( sound_handle const Chunk );
    void
        update_counter( sound_handle const Sound, int const Value );
    void
        update_location();
    // potentially updates area-based gain factor of the source. returns: true if location has changed
    bool
        update_soundproofing();
    void
        insert( sound_handle const Sound );
    template <class Iterator_>
    void
        insert( Iterator_ First, Iterator_ Last ) {
            uint32_sequence sounds;
            std::vector<audio::buffer_handle> buffers;
            std::for_each(
                First, Last,
                [&]( sound_handle const &soundhandle ) {
                    sounds.emplace_back( soundhandle );
                    buffers.emplace_back( sound( soundhandle ).buffer ); } );
            audio::renderer.insert( std::begin( buffers ), std::end( buffers ), this, sounds );
            update_counter( *First, 1 ); }
    sound_data &
        sound( sound_handle const Sound );
    sound_data const &
        sound( sound_handle const Sound ) const;

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
    bool m_playbeginning { true }; // indicates started sounds should be preceeded by opening bookend if there's one
    std::array<sound_data, 3> m_sounds { {} }; // basic sounds emitted by the source, main and optional bookends
    std::vector<soundchunk_pair> m_soundchunks; // table of samples activated when associated variable is within certain range
    bool m_soundchunksempty { true }; // helper, cached check whether sample table is linked with any actual samples
    int m_crossfaderange {}; // range of transition from one chunk to another 
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
