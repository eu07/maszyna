/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "al.h"
#include "alc.h"

namespace audio {

ALuint const null_resource{ ~( ALuint { 0 } ) };

// wrapper for audio sample
struct openal_buffer {
// members
    ALuint id { null_resource }; // associated AL resource
    unsigned int rate { 0 }; // sample rate of the data
    std::string name;
// constructors
    openal_buffer() = default;
    explicit openal_buffer( std::string const &Filename );

private:
// methods
    // mix specified number of interleaved multi-channel data, down to mono
    void
        narrow_to_mono( std::uint16_t const Channelcount );
// members
    std::vector<std::int16_t> data; // audio data
};

using buffer_handle = std::size_t;


// 
class buffer_manager {

public:
// constructors
    buffer_manager() { m_buffers.emplace_back( openal_buffer() ); } // empty bindings for null buffer
// destructor
    ~buffer_manager();
// methods
    // creates buffer object out of data stored in specified file. returns: handle to the buffer or null_handle if creation failed
    buffer_handle
        create( std::string const &Filename );
    // provides direct access to a specified buffer
    audio::openal_buffer const &
        buffer( audio::buffer_handle const Buffer ) const;

private:
// types
    using buffer_sequence = std::vector<openal_buffer>;
    using index_map = std::unordered_map<std::string, std::size_t>;
// methods
    // places in the bank a buffer containing data stored in specified file. returns: handle to the buffer
    buffer_handle
        emplace( std::string Filename );
    // checks whether specified buffer is in the buffer bank. returns: buffer handle, or null_handle.
    buffer_handle
        find_buffer( std::string const &Buffername ) const;
    // checks whether specified file exists. returns: name of the located file, or empty string.
    std::string
        find_file( std::string const &Filename ) const;
// members
    buffer_sequence m_buffers;
    index_map m_buffermappings;
};

} // audio

//---------------------------------------------------------------------------
