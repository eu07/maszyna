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
            Input.getTokens( 1, true, "\n\r\t ,;" );
            break;
        }
        case sound_type::multipart: {
            // three samples: start, middle, stop
            Input.getTokens( 3, true, "\n\r\t ,;" );
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
sound_source::play() {

}

bool
sound_source::empty() const {

    return true;
}

//---------------------------------------------------------------------------
