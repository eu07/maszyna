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
#include "parser.h"
#include "names.h"

enum sound_type {
    single,
    multipart
};

enum sound_parameters {
    none,
    range = 0x1,
    amplitude = 0x2,
    frequency = 0x4
};

// mini controller and audio dispatcher; issues play commands for the audio renderer,
// updates parameters of assigned audio sources for the playback duration
// TODO: move to simulation namespace after clean up of owner classes
class sound_source {

public:
// methods
    // restores state of the class from provided data stream
    sound_source &
        deserialize( cParser &Input, sound_type const Legacytype, int const Legacyparameters = sound_parameters::none );
    sound_source &
        deserialize( std::string const &Input, sound_type const Legacytype, int const Legacyparameters = sound_parameters::none );
    // issues contextual play commands for the audio renderer
    void
        play();
    // sets volume of audio streams associated with this source
    sound_source &
        volume( float const Volume );
    // sound source name setter/getter
    void
        name( std::string Name );
    std::string const &
        name() const;
    // returns true if there isn't any sound buffer associated with the object, false otherwise
    bool
        empty() const;

private:
// members
    std::string m_name;
};

// sound source name setter/getter
inline void sound_source::name( std::string Name ) { m_name = Name; }
inline std::string const & sound_source::name() const { return m_name; }

// collection of generators for power grid present in the scene
class sound_table : public basic_table<sound_source> {

};

//---------------------------------------------------------------------------
