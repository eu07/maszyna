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

enum sound_type {
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

// mini controller and audio dispatcher; issues play commands for the audio renderer,
// updates parameters of created audio emitters for the playback duration
// TODO: move to simulation namespace after clean up of owner classes
class sound_source {

public:
// methods
    // restores state of the class from provided data stream
    sound_source &
        deserialize( cParser &Input, sound_type const Legacytype, int const Legacyparameters = NULL );
    sound_source &
        deserialize( std::string const &Input, sound_type const Legacytype, int const Legacyparameters = NULL );
    // issues contextual play commands for the audio renderer
    void
        play( int const Flags = NULL );
    // stops currently active play commands controlled by this emitter
    void
        stop();
    // adjusts parameters of provided implementation-side sound source
    void
        update( audio::openal_source &Source );
    // sound source name setter/getter
    void
        name( std::string Name );
    std::string const &
        name() const;
    // returns true if there isn't any sound buffer associated with the object, false otherwise
    bool
        empty() const;
    // returns true if the source is emitting any sound
    bool
        is_playing() const;

private:
// types
    enum stage {
        none,
        begin,
        main,
        end,
        cease,
        restart
    };

// members
    TDynamicObject * m_owner { nullptr }; // optional, the vehicle carrying this sound source
    glm::vec3 m_offset; // relative position of the source, either from the owner or the region centre
    std::string m_name;
    stage m_stage{ stage::none };
    int m_flags{ NULL };
    audio::buffer_handle m_soundmain { null_handle }; // main sound emitted by the source
    audio::buffer_handle m_soundbegin { null_handle }; // optional, sound emitted before the main sound
    audio::buffer_handle m_soundend { null_handle }; // optional, sound emitted after the main sound
    // TODO: table of samples with associated values, activated when controlling variable matches the value
};

// sound source name setter/getter
inline void sound_source::name( std::string Name ) { m_name = Name; }
inline std::string const & sound_source::name() const { return m_name; }

// collection of generators for power grid present in the scene
class sound_table : public basic_table<sound_source> {

};

//---------------------------------------------------------------------------
