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

namespace audio {

class openal_renderer {

public:
// destructor
    ~openal_renderer();
// methods
    // buffer methods
    // returns handle to a buffer containing audio data from specified file
    buffer_handle
        fetch_buffer( std::string const &Filename );
    // core methods
    // initializes the service
    bool
        init();

private:
// methods
    bool
        init_caps();
// members
    ALCdevice * m_device { nullptr };
    ALCcontext * m_context { nullptr };
    bool m_ready { false }; // renderer is initialized and functional

    buffer_manager m_buffers;
};

extern openal_renderer renderer;

} // audio

//---------------------------------------------------------------------------
