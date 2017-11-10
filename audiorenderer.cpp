/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"

#include "audiorenderer.h"
#include "logs.h"

namespace audio {

openal_renderer renderer;

openal_renderer::~openal_renderer() {

    ::alcMakeContextCurrent( nullptr );

    if( m_context != nullptr ) { ::alcDestroyContext( m_context ); }
    if( m_device != nullptr )  { ::alcCloseDevice( m_device ); }
}

buffer_handle
openal_renderer::fetch_buffer( std::string const &Filename ) {

    return m_buffers.create( Filename );
}

// initializes the service
bool
openal_renderer::init() {

    if( true == m_ready ) {
        // already initialized and enabled
        return true;
    }
    if( false == init_caps() ) {
        // basic initialization failed
        return false;
    }

    m_ready = true;
    return true;
}

bool
openal_renderer::init_caps() {

    // select the "preferred device"
    // TODO: support for opening config-specified device
    m_device = ::alcOpenDevice( nullptr );
    if( m_device == nullptr ) {
        ErrorLog( "Failed to obtain audio device" );
        return false;
    }

    ALCint versionmajor, versionminor;
    ::alcGetIntegerv( m_device, ALC_MAJOR_VERSION, 1, &versionmajor );
    ::alcGetIntegerv( m_device, ALC_MINOR_VERSION, 1, &versionminor );
    auto const oalversion { std::to_string( versionmajor ) + "." + std::to_string( versionminor ) };

    WriteLog(
        "Audio Renderer: " + std::string { (char *)::alcGetString( m_device, ALC_DEVICE_SPECIFIER ) }
        + " OpenAL Version: " + oalversion );

    WriteLog( "Supported extensions: " + std::string{ (char *)::alcGetString( m_device, ALC_EXTENSIONS ) } );

    m_context = ::alcCreateContext( m_device, nullptr );
    if( m_context == nullptr ) {
        ErrorLog( "Failed to create audio context" );
        return false;
    }

    return ( ::alcMakeContextCurrent( m_context ) == AL_TRUE );
}

} // audio

//---------------------------------------------------------------------------
