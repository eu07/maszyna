/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"

#include "audio.h"
#include "Globals.h"
#include "McZapkie/mctools.h"
#include "Logs.h"
#include <sndfile.h>

namespace audio {

openal_buffer::openal_buffer( std::string const &Filename ) :
    name( Filename ) {

	SF_INFO si;
	si.format = 0;

	std::string file = Filename;
	std::replace(file.begin(), file.end(), '\\', '/');

	SNDFILE *sf = sf_open(file.c_str(), SFM_READ, &si);
	if (sf == nullptr)
		throw std::runtime_error("sound: sf_open failed");

	int16_t *fbuf = new int16_t[si.frames * si.channels];
	if (sf_readf_short(sf, fbuf, si.frames) != si.frames)
		throw std::runtime_error("sound: incomplete file");

	sf_close(sf);

	rate = si.samplerate;

	int16_t *buf = nullptr;
	if (si.channels == 1)
		buf = fbuf;
	else
	{
		WriteLog("sound: warning: mixing multichannel file to mono");
		buf = new int16_t[si.frames];
		for (size_t i = 0; i < si.frames; i++)
		{
			int32_t accum = 0;
			for (size_t j = 0; j < si.channels; j++)
				accum += fbuf[i * si.channels + j];
			buf[i] = accum / si.channels;
		}
	}

	id = 0;
	alGenBuffers(1, &id);
	if (!id)
		throw std::runtime_error("sound: cannot generate buffer");

	alBufferData(id, AL_FORMAT_MONO16, buf, si.frames * 2, rate);

	if (si.channels != 1)
		delete[] buf;
	delete[] fbuf;
}

buffer_manager::~buffer_manager() {

    for( auto &buffer : m_buffers ) {
        if( buffer.id != null_resource ) {
            ::alDeleteBuffers( 1, &( buffer.id ) );
        }
    }
}

// creates buffer object out of data stored in specified file. returns: handle to the buffer or null_handle if creation failed
audio::buffer_handle
buffer_manager::create( std::string const &Filename ) {

    auto filename { ToLower( Filename ) };

    auto const dotpos { filename.rfind( '.' ) };
    if( ( dotpos != std::string::npos )
     && ( dotpos != filename.rfind( ".." ) + 1 ) ) {
        // trim extension if there's one, but don't mistake folder traverse for extension
        filename.erase( dotpos );
    }
    // convert slashes
    std::replace(
        std::begin( filename ), std::end( filename ),
        '\\', '/' );

    audio::buffer_handle lookup { null_handle };
    std::string filelookup;
    if( false == Global::asCurrentDynamicPath.empty() ) {
        // try dynamic-specific sounds first
        lookup = find_buffer( Global::asCurrentDynamicPath + filename );
        if( lookup != null_handle ) {
            return lookup;
        }
        filelookup = find_file( Global::asCurrentDynamicPath + filename );
        if( false == filelookup.empty() ) {
            return emplace( filelookup );
        }
    }
    if( filename.find( '/' ) != std::string::npos ) {
        // if the filename includes path, try to use it directly
        lookup = find_buffer( filename );
        if( lookup != null_handle ) {
            return lookup;
        }
        filelookup = find_file( filename );
        if( false == filelookup.empty() ) {
            return emplace( filelookup );
        }
    }
    // if dynamic-specific and/or direct lookups find nothing, try the default sound folder
    lookup = find_buffer( szSoundPath + filename );
    if( lookup != null_handle ) {
        return lookup;
    }
    filelookup = find_file( szSoundPath + filename );
    if( false == filelookup.empty() ) {
        return emplace( filelookup );
    }
    // if we still didn't find anything, give up
    ErrorLog( "Bad file: failed do locate audio file \"" + Filename + "\"" );
    return null_handle;
}

// provides direct access to a specified buffer
audio::openal_buffer const &
buffer_manager::buffer( audio::buffer_handle const Buffer ) const {

    return m_buffers[ Buffer ];
}

// places in the bank a buffer containing data stored in specified file. returns: handle to the buffer
audio::buffer_handle
buffer_manager::emplace( std::string Filename ) {

    buffer_handle const handle { m_buffers.size() };
    m_buffers.emplace_back( Filename );

    // NOTE: we store mapping without file type extension, to simplify lookups
    m_buffermappings.emplace(
        Filename.erase( Filename.rfind( '.' ) ),
        handle );

    return handle;
}

audio::buffer_handle
buffer_manager::find_buffer( std::string const &Buffername ) const {

    auto lookup = m_buffermappings.find( Buffername );
    if( lookup != m_buffermappings.end() )
        return lookup->second;
    else
        return null_handle;
}


std::string
buffer_manager::find_file( std::string const &Filename ) const {

    std::vector<std::string> const extensions { ".wav", ".WAV", ".flac", ".ogg" };

    for( auto const &extension : extensions ) {
        if( FileExists( Filename + extension ) ) {
            // valid name on success
            return Filename + extension;
        }
    }
    return {}; // empty string on failure
}

} // audio

//---------------------------------------------------------------------------
