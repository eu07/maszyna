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
#include "utilities.h"
#include "Logs.h"
#include "ResourceManager.h"

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"

namespace audio {

openal_buffer::openal_buffer( std::string const &Filename ) :
    name( Filename ) {

    WriteLog( "Loading sound data from \"" + Filename + "\"", logtype::sound );

    ::alGenBuffers( 1, &id );
    // fetch audio data
    if( Filename.substr( Filename.rfind( '.' ) ) == ".wav" ) {
        // .wav audio data file
        auto *file { drwav_open_file( Filename.c_str() ) };
        if( file != nullptr ) {
            rate = file->sampleRate;
            auto const samplecount{ static_cast<std::size_t>( file->totalSampleCount ) };
            data.resize( samplecount );
            drwav_read_s16(
                file,
                samplecount,
                &data[ 0 ] );
            if( file->channels > 1 ) {
                narrow_to_mono( file->channels );
                data.resize( samplecount / file->channels );
            }
        }
        else {
            ErrorLog( "Bad file: failed to load audio file \"" + Filename + "\"", logtype::file );
        }
        // we're done with the disk data
        drwav_close( file );
    }
    else if( Filename.substr( Filename.rfind( '.' ) ) == ".flac" ) {
        // .flac audio data file
        auto *file { drflac_open_file( Filename.c_str() ) };
        if( file != nullptr ) {
            rate = file->sampleRate;
            auto const samplecount{ static_cast<std::size_t>( file->totalSampleCount ) };
            data.resize( samplecount );
            drflac_read_s16(
                file,
                samplecount,
                &data[ 0 ] );
            if( file->channels > 1 ) {
                narrow_to_mono( file->channels );
                data.resize( samplecount / file->channels );
            }
        }
        else {
            ErrorLog( "Bad file: failed to load audio file \"" + Filename + "\"", logtype::file );
        }
        // we're done with the disk data
        drflac_close( file );
    }
    else if( Filename.substr( Filename.rfind( '.' ) ) == ".ogg" ) {
        // vorbis .ogg audio data file
        // TBD, TODO: customized vorbis_decode to avoid unnecessary shuffling around of the decoded data
        int channels, samplerate;
        std::int16_t *filedata { nullptr };
        auto const samplecount { stb_vorbis_decode_filename( Filename.c_str(), &channels, &samplerate, &filedata ) };
        if( samplecount > 0 ) {
            rate = samplerate;
            data.resize( samplecount * channels );
            std::copy( filedata, filedata + data.size(), std::begin( data ) );
            free( filedata );
            if( channels > 1 ) {
                narrow_to_mono( channels );
                data.resize( samplecount );
            }
        }
        else {
            ErrorLog( "Bad file: failed to load audio file \"" + Filename + "\"", logtype::file );
        }
    }

    if( false == data.empty() ) {
        // send the data to openal side
        ::alBufferData( id, AL_FORMAT_MONO16, data.data(), data.size() * sizeof( std::int16_t ), rate );
        // and get rid of the source, we shouldn't need it anymore
        // TBD, TODO: delay data fetching and transfers until the buffer is actually used?
        std::vector<std::int16_t>().swap( data );
    }
    fetch_caption();
}

// mix specified number of interleaved multi-channel data, down to mono
void
openal_buffer::narrow_to_mono( std::uint16_t const Channelcount ) {
 
    std::size_t monodataindex { 0 };
    std::int64_t accumulator { 0 };
    auto channelcount { Channelcount };

    for( auto const channeldata : data ) {

        accumulator += channeldata;
        if( --channelcount == 0 ) {

            data[ monodataindex++ ] = static_cast<std::int16_t>( accumulator / Channelcount );
            accumulator = 0;
            channelcount = Channelcount;
        }
    }
}

// retrieves sound caption in currently set language
void
openal_buffer::fetch_caption() {

    std::string captionfilename { name };
    captionfilename.erase( captionfilename.rfind( '.' ) ); // obcięcie rozszerzenia
    captionfilename += "-" + Global.asLang + ".txt"; // już może być w różnych językach
    if( true == FileExists( captionfilename ) ) {
        // wczytanie
        std::ifstream inputfile( captionfilename );
        caption.assign( std::istreambuf_iterator<char>( inputfile ), std::istreambuf_iterator<char>() );
    }
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

    erase_extension( filename );
    replace_slashes( filename );

    audio::buffer_handle lookup { null_handle };
    std::string filelookup;
    if( false == Global.asCurrentDynamicPath.empty() ) {
        // try dynamic-specific sounds first
        lookup = find_buffer( Global.asCurrentDynamicPath + filename );
        if( lookup != null_handle ) {
            return lookup;
        }
        filelookup = find_file( Global.asCurrentDynamicPath + filename );
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
    ErrorLog( "Bad file: failed to locate audio file \"" + Filename + "\"", logtype::file );
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
    erase_extension( Filename );
    m_buffermappings.emplace(
        Filename,
        handle );

    return handle;
}

audio::buffer_handle
buffer_manager::find_buffer( std::string const &Buffername ) const {

    auto const lookup = m_buffermappings.find( Buffername );
    if( lookup != std::end( m_buffermappings ) )
        return lookup->second;
    else
        return null_handle;
}

std::string
buffer_manager::find_file( std::string const &Filename ) const {

    auto const lookup {
        FileExists(
            { Filename },
            { ".ogg", ".flac", ".wav" } ) };

    return lookup.first + lookup.second;
}

} // audio

//---------------------------------------------------------------------------
