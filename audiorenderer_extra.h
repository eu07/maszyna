// separate file because of include dependecies mess

namespace audio {
template <class Iterator_>
openal_source &
openal_source::bind( sound_source *Controller, uint32_sequence Sounds, Iterator_ First, Iterator_ Last ) {

    controller = Controller;
    sounds = Sounds;
    // look up and queue assigned buffers
    std::vector<ALuint> buffers;
    std::for_each(
        First, Last,
        [&]( audio::buffer_handle const &bufferhandle ) {
            auto const &buffer { audio::renderer.buffer( bufferhandle ) };
			if (buffer.id != null_resource) buffers.emplace_back( buffer.id ); } );

    is_multipart = ( buffers.size() > 1 );

    if( id != audio::null_resource && !buffers.empty()) {
        ::alSourceQueueBuffers( id, static_cast<ALsizei>( buffers.size() ), buffers.data() );
        ::alSourceRewind( id );
        // sound controller can potentially request playback to start from certain buffer point
        // for multipart sounds the offset is applied only to last piece during playback
        // for single sound we also make sure not to apply the offset to optional bookends
        if( controller->start() == 0.f || is_multipart || controller->is_bookend( buffers.front() ) ) {
            // regular case with no offset, reset bound source just in case
            ::alSourcei( id, AL_SAMPLE_OFFSET, 0 );
        }
        else {
            // move playback start to specified point in 0-1 range
            ALint buffersize;
            ::alGetBufferi( buffers.front(), AL_SIZE, &buffersize );
            ::alSourcei(
                id,
                AL_SAMPLE_OFFSET,
                static_cast<ALint>( controller->start() * ( buffersize / sizeof( std::int16_t ) ) ) );
        }
    }

    return *this;
}

inline
float
amplitude_to_db( float const Amplitude ) {

    return 20.f * std::log10( Amplitude );
}

inline
float
db_to_amplitude( float const Decibels ) {

    return std::pow( 10.f, Decibels / 20.f );
}
}
