/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

/*
    MaSzyna EU07 locomotive simulator
    Copyright (C) 2001-2004  Marcin Wozniak and others

*/

#include "stdafx.h"
#include "Texture.h"

#include "GL/glew.h"

#include "utilities.h"
#include "Globals.h"
#include "Logs.h"
#include "sn_utils.h"
#include "utilities.h"
#include "flip-s3tc.h"
#include <png.h>

#define EU07_DEFERRED_TEXTURE_UPLOAD

texture_manager::texture_manager() {

    // since index 0 is used to indicate no texture, we put a blank entry in the first texture slot
    m_textures.emplace_back( new opengl_texture(), std::chrono::steady_clock::time_point() );
}

// loads texture data from specified file
// TODO: wrap it in a workitem class, for the job system deferred loading
void
opengl_texture::load() {

    if( name.size() < 3 ) { goto fail; }

    WriteLog( "Loading texture data from \"" + name + "\"", logtype::texture );

    data_state = resource_state::loading;
    {
        std::string const extension = name.substr( name.size() - 3, 3 );

             if( extension == "dds" ) { load_DDS(); }
        else if( extension == "tga" ) { load_TGA(); }
        else if( extension == "png" ) { load_PNG(); }
        else if( extension == "bmp" ) { load_BMP(); }
        else if( extension == "tex" ) { load_TEX(); }
        else { goto fail; }
    }

    // data state will be set by called loader, so we're all done here
    if( data_state == resource_state::good ) {

        has_alpha = (
            data_components == GL_RGBA ?
                true :
                false );

        size = data.size() / 1024;

        return;
    }

fail:
    data_state = resource_state::failed;
    ErrorLog( "Bad texture: failed to load texture \"" + name + "\"" );
    // NOTE: temporary workaround for texture assignment errors
    id = 0;
    return;
}

void opengl_texture::load_PNG()
{
	png_image png;
	memset(&png, 0, sizeof(png_image));
	png.version = PNG_IMAGE_VERSION;

	png_image_begin_read_from_file(&png, name.c_str());
	if (png.warning_or_error)
	{
		data_state = resource_state::failed;
		ErrorLog(name + " error: " + std::string(png.message));
		return;
	}

	if (png.format & PNG_FORMAT_FLAG_ALPHA)
	{
		data_format = GL_RGBA;
		data_components = GL_RGBA;
		png.format = PNG_FORMAT_RGBA;
	}
	else
	{
		data_format = GL_RGB;
		data_components = GL_RGB;
		png.format = PNG_FORMAT_RGB;
	}
	data_width = png.width;
	data_height = png.height;

	data.resize(PNG_IMAGE_SIZE(png));

    png_image_finish_read(&png, nullptr,
        (void*)&data[0], -data_width * PNG_IMAGE_PIXEL_SIZE(png.format), nullptr);
	// we're storing texture data internally with bottom-left origin
	// so use negative stride

    if (png.warning_or_error)
    {
        data_state = resource_state::failed;
        ErrorLog(name + " error: " + std::string(png.message));
        return;
    }

    data_mapcount = 1;
    data_state = resource_state::good;
}

void
opengl_texture::load_BMP() {

    std::ifstream file( name, std::ios::binary ); file.unsetf( std::ios::skipws );

    BITMAPFILEHEADER header;

    file.read( (char *)&header, sizeof( BITMAPFILEHEADER ) );
    if( file.eof() ) {

        data_state = resource_state::failed;
        return;
    }

    // Read in bitmap information structure
    BITMAPINFO info;
    unsigned int infosize = header.bfOffBits - sizeof( BITMAPFILEHEADER );
    if( infosize > sizeof( info ) ) {
        WriteLog( "Warning - BMP header is larger than expected, possible format difference.", logtype::texture );
    }
    file.read( (char *)&info, std::min( (size_t)infosize, sizeof( info ) ) );

    data_width = info.bmiHeader.biWidth;
    data_height = info.bmiHeader.biHeight;

    if( info.bmiHeader.biCompression != BI_RGB ) {

        ErrorLog( "Bad texture: compressed BMP textures aren't supported.", logtype::texture );
        data_state = resource_state::failed;
        return;
    }

    unsigned long datasize = info.bmiHeader.biSizeImage;
    if( 0 == datasize ) {
        // calculate missing info
        datasize = ( data_width * info.bmiHeader.biBitCount + 7 ) / 8 * data_height;
    }

    data.resize( datasize );
    file.read( &data[0], datasize );
	// we're storing texture data internally with bottom-left origin
	// so BMP origin matches, no flipping needed

    // fill remaining data info
    if( info.bmiHeader.biBitCount == 32 ) {

        data_format = GL_BGRA;
        data_components = GL_RGBA;
    }
    else {

        data_format = GL_BGR;
        data_components = GL_RGB;
    }
    data_mapcount = 1;
    data_state = resource_state::good;

    return;
}

DDCOLORKEY opengl_texture::deserialize_ddck(std::istream &s)
{
	DDCOLORKEY ddck;

	ddck.dwColorSpaceLowValue = sn_utils::ld_uint32(s);
	ddck.dwColorSpaceHighValue = sn_utils::ld_uint32(s);

	return ddck;
}

DDPIXELFORMAT opengl_texture::deserialize_ddpf(std::istream &s)
{
	DDPIXELFORMAT ddpf;

	ddpf.dwSize = sn_utils::ld_uint32(s);
	ddpf.dwFlags = sn_utils::ld_uint32(s);
	ddpf.dwFourCC = sn_utils::ld_uint32(s);
	ddpf.dwRGBBitCount = sn_utils::ld_uint32(s);
	ddpf.dwRBitMask = sn_utils::ld_uint32(s);
	ddpf.dwGBitMask = sn_utils::ld_uint32(s);
	ddpf.dwBBitMask = sn_utils::ld_uint32(s);
	ddpf.dwRGBAlphaBitMask = sn_utils::ld_uint32(s);

	return ddpf;
}

DDSCAPS2 opengl_texture::deserialize_ddscaps(std::istream &s)
{
	DDSCAPS2 ddsc;

	ddsc.dwCaps = sn_utils::ld_uint32(s);
	ddsc.dwCaps2 = sn_utils::ld_uint32(s);
	ddsc.dwCaps3 = sn_utils::ld_uint32(s);
	ddsc.dwCaps4 = sn_utils::ld_uint32(s);

	return ddsc;
}

DDSURFACEDESC2 opengl_texture::deserialize_ddsd(std::istream &s)
{
	DDSURFACEDESC2 ddsd;

	ddsd.dwSize = sn_utils::ld_uint32(s);
	ddsd.dwFlags = sn_utils::ld_uint32(s);
	ddsd.dwHeight = sn_utils::ld_uint32(s);
	ddsd.dwWidth = sn_utils::ld_uint32(s);
	ddsd.lPitch = sn_utils::ld_uint32(s);
	ddsd.dwBackBufferCount = sn_utils::ld_uint32(s);
	ddsd.dwMipMapCount = sn_utils::ld_uint32(s);
	ddsd.dwAlphaBitDepth = sn_utils::ld_uint32(s);
	ddsd.dwReserved = sn_utils::ld_uint32(s);
	sn_utils::ld_uint32(s);
	ddsd.lpSurface = nullptr;
	ddsd.ddckCKDestOverlay = deserialize_ddck(s);
	ddsd.ddckCKDestBlt = deserialize_ddck(s);
	ddsd.ddckCKSrcOverlay = deserialize_ddck(s);
	ddsd.ddckCKSrcBlt = deserialize_ddck(s);
	ddsd.ddpfPixelFormat = deserialize_ddpf(s);
	ddsd.ddsCaps = deserialize_ddscaps(s);
	ddsd.dwTextureStage = sn_utils::ld_uint32(s);

	return ddsd;
}

void
opengl_texture::load_DDS() {

    std::ifstream file( name, std::ios::binary | std::ios::ate ); file.unsetf( std::ios::skipws );
    std::size_t filesize = static_cast<size_t>(file.tellg());   // ios::ate already positioned us at the end of the file
    file.seekg( 0, std::ios::beg ); // rewind the caret afterwards

    char filecode[5];
    file.read(filecode, 4);
    filesize -= 4;
    filecode[4] = 0;

    if( filecode != std::string( "DDS " ) )
    {
        data_state = resource_state::failed;
        return;
    }

	DDSURFACEDESC2 ddsd = deserialize_ddsd(file);
	filesize -= 124;

    //
    // This .dds loader supports the loading of compressed formats DXT1, DXT3
    // and DXT5.
    //

    switch (ddsd.ddpfPixelFormat.dwFourCC)
    {
    case FOURCC_DXT1:
        // DXT1's compression ratio is 8:1
        data_format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        break;

    case FOURCC_DXT3:
        // DXT3's compression ratio is 4:1
        data_format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        break;

    case FOURCC_DXT5:
        // DXT5's compression ratio is 4:1
        data_format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        break;

    default:
        data_state = resource_state::failed;
        return;
    }

    data_width = ddsd.dwWidth;
    data_height = ddsd.dwHeight;
    data_mapcount = ddsd.dwMipMapCount;

    int blockSize = ( data_format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? 8 : 16 );
    int offset = 0;

    while( ( data_width > Global.iMaxTextureSize ) || ( data_height > Global.iMaxTextureSize ) ) {
        // pomijanie zbyt dużych mipmap, jeśli wymagane jest ograniczenie rozmiaru
        offset += ( ( data_width + 3 ) / 4 ) * ( ( data_height + 3 ) / 4 ) * blockSize;
        data_width /= 2;
        data_height /= 2;
        --data_mapcount;
        WriteLog( "Texture size exceeds specified limits, skipping mipmap level" );
    };

    if( data_mapcount <= 0 ) {
        // there's a chance we've discarded the provided mipmap(s) as too large
        WriteLog( "Texture \"" + name + "\" has no mipmaps which can fit currently set texture size limits." );
        data_state = resource_state::failed;
        return;
    }

    size_t datasize = filesize - offset;

    if( datasize == 0 ) {
        // catch malformed .dds files
        WriteLog( "Bad texture: file \"" + name + "\" is malformed and holds no texture data.", logtype::texture );
        data_state = resource_state::failed;
        return;
    }
    // reserve space and load texture data
    data.resize( datasize );

    if( offset != 0 ) {
        // skip data for mipmaps we don't need
        file.seekg( offset, std::ios_base::cur );
        filesize -= offset;
    }
    file.read((char *)&data[0], datasize);
    filesize -= datasize;

	// we're storing texture data internally with bottom-left origin,
	// while DDS stores it with top-left origin. we need to flip it.
	if (Global.dds_upper_origin)
	{
        char *mipmap = (char*)&data[0];
        int mapcount = data_mapcount,
            width = data_width,
            height = data_height;
        while (mapcount)
		{
			if (ddsd.ddpfPixelFormat.dwFourCC == FOURCC_DXT1)
				flip_s3tc::flip_dxt1_image(mipmap, width, height);
			else if (ddsd.ddpfPixelFormat.dwFourCC == FOURCC_DXT3)
				flip_s3tc::flip_dxt23_image(mipmap, width, height);
			else if (ddsd.ddpfPixelFormat.dwFourCC == FOURCC_DXT5)
				flip_s3tc::flip_dxt45_image(mipmap, width, height);

            mipmap += ( ( width + 3 ) / 4 ) * ( ( height + 3 ) / 4 ) * blockSize;
            width = std::max( width / 2, 4 );
            height = std::max( height / 2, 4 );
            --mapcount;
        }
	}

    data_components =
        ( ddsd.ddpfPixelFormat.dwFourCC == FOURCC_DXT1 ?
            GL_RGB :
            GL_RGBA );

    data_state = resource_state::good;

    return;
}

void
opengl_texture::load_TEX() {

    std::ifstream file( name, std::ios::binary ); file.unsetf( std::ios::skipws );

    char head[ 5 ];
    file.read( head, 4 );
    head[ 4 ] = 0;

    bool hasalpha;
    if( std::string( "RGB " ) == head ) {
        hasalpha = false;
    }
    else if( std::string( "RGBA" ) == head ) {
        hasalpha = true;
    }
    else {
        ErrorLog( "Bad texture: unrecognized TEX texture sub-format: " + std::string(head), logtype::texture );
        data_state = resource_state::failed;
        return;
    };

    file.read( (char *)&data_width, sizeof( int ) );
    file.read( (char *)&data_height, sizeof( int ) );

    std::size_t datasize = data_width * data_height * ( hasalpha ? 4 : 3 );

    data.resize( datasize );
    file.read( reinterpret_cast<char *>( &data[0] ), datasize );

    // fill remaining data info
    if( true == hasalpha ) {

        data_format = GL_RGBA;
        data_components = GL_RGBA;
    }
    else {

        data_format = GL_RGB;
        data_components = GL_RGB;
    }
    data_mapcount = 1;
    data_state = resource_state::good;

    return;
}

void
opengl_texture::load_TGA() {

    std::ifstream file( name, std::ios::binary ); file.unsetf( std::ios::skipws );

    // Read the header of the TGA, compare it with the known headers for compressed and uncompressed TGAs
    unsigned char tgaheader[ 18 ];
    file.read( (char *)tgaheader, sizeof( unsigned char ) * 18 );

    while( tgaheader[ 0 ] > 0 ) {
        --tgaheader[ 0 ];

        unsigned char temp;
        file.read( (char *)&temp, sizeof( unsigned char ) );
    }

    data_width = tgaheader[ 13 ] * 256 + tgaheader[ 12 ];
    data_height = tgaheader[ 15 ] * 256 + tgaheader[ 14 ];
    int const bytesperpixel = tgaheader[ 16 ] / 8;

    // check whether width, height an BitsPerPixel are valid
    if( ( data_width <= 0 )
     || ( data_height <= 0 )
     || ( ( bytesperpixel != 1 ) && ( bytesperpixel != 3 ) && ( bytesperpixel != 4 ) ) ) {

        data_state = resource_state::failed;
        return;
    }

    // allocate the data buffer
    int const datasize = data_width * data_height * 4;
	data.resize(datasize);

    // call the appropriate loader-routine
    if( tgaheader[ 2 ] == 2 ) {
        // uncompressed TGA
        if( bytesperpixel == 4 ) {
            // read the data directly
            file.read( reinterpret_cast<char*>( &data[0] ), datasize );
        }
        else {
            // rgb or greyscale image, expand to bgra
            unsigned char buffer[ 4 ] = { 255, 255, 255, 255 }; // alpha channel will be white

            unsigned int *datapointer = (unsigned int*)&data[0];
            unsigned int *bufferpointer = (unsigned int*)&buffer[ 0 ];

            int const pixelcount = data_width * data_height;

            for( int i = 0; i < pixelcount; ++i ) {

                file.read( (char *)&buffer[ 0 ], sizeof( unsigned char ) * bytesperpixel );
                if( bytesperpixel == 1 ) {
                    // expand greyscale data
                    buffer[ 1 ] = buffer[ 0 ];
                    buffer[ 2 ] = buffer[ 0 ];
                }
                // copy all four values in one operation
                ( *datapointer ) = ( *bufferpointer );
                ++datapointer;
            }
        }
    }
    else if( tgaheader[ 2 ] == 10 ) {
        // compressed TGA
        int currentpixel = 0;

        unsigned char buffer[ 4 ] = { 255, 255, 255, 255 };
        const int pixelcount = data_width * data_height;

        unsigned int *datapointer = (unsigned int *)&data[0];
        unsigned int *bufferpointer = (unsigned int *)&buffer[ 0 ];

        do {
            unsigned char chunkheader = 0;

            file.read( (char *)&chunkheader, sizeof( unsigned char ) );

            if( (chunkheader & 0x80 ) == 0 ) {
                // if the high bit is not set, it means it is the number of RAW color packets, plus 1
                for( int i = 0; i <= chunkheader; ++i ) {

                    file.read( (char *)&buffer[ 0 ], bytesperpixel );

                    if( bytesperpixel == 1 ) {
                        // expand greyscale data
                        buffer[ 1 ] = buffer[ 0 ];
                        buffer[ 2 ] = buffer[ 0 ];
                    }
                    // copy all four values in one operation
                    ( *datapointer ) = ( *bufferpointer );

                    ++datapointer;
                    ++currentpixel;
                }
            }
            else {
                // rle chunk, the color supplied afterwards is reapeated header + 1 times (not including the highest bit)
                chunkheader &= ~0x80;
                // read the current color
                file.read( (char *)&buffer[ 0 ], bytesperpixel );

                if( bytesperpixel == 1 ) {
                    // expand greyscale data
                    buffer[ 1 ] = buffer[ 0 ];
                    buffer[ 2 ] = buffer[ 0 ];
                }
                // copy the color into the image data as many times as dictated
                for( int i = 0; i <= chunkheader; ++i ) {

                    ( *datapointer ) = ( *bufferpointer );
                    ++datapointer;
                    ++currentpixel;
                }
            }

        } while( currentpixel < pixelcount );
    }
    else {
        // unrecognized TGA sub-type
        data_state = resource_state::failed;
        return;
    }

    if( ( tgaheader[ 17 ] & 0x20 ) != 0 ) {
        // normally origin is bottom-left
        // if byte 17 bit 5 is set, it is top-left and needs flip
        flip_vertical();
    }

    downsize( GL_BGRA );
    if( ( data_width > Global.iMaxTextureSize ) || ( data_height > Global.iMaxTextureSize ) ) {
        // for non-square textures there's currently possibility the scaling routine will have to abort
        // before it gets all work done
        data_state = resource_state::failed;
        return;
    }

    // TODO: add horizontal/vertical data flip, based on the descriptor (18th) header byte

    // fill remaining data info
    data_mapcount = 1;
    data_format = GL_BGRA;
    data_components =
        ( bytesperpixel == 4 ?
            GL_RGBA :
            GL_RGB );
    data_state = resource_state::good;

    return;
}

bool
opengl_texture::bind(size_t unit) {

    if( ( false == is_ready )
     && ( false == create() ) ) {
        return false;
    }

    if (units[unit] == id)
        return true;

    if (GLEW_ARB_direct_state_access)
    {
        glBindTextureUnit(unit, id);
    }
    else
    {
        if (unit != m_activeunit)
        {
            glActiveTexture(GL_TEXTURE0 + unit);
            m_activeunit = unit;
        }
        glBindTexture(target, id);
    }

    units[unit] = id;

    return true;
}

void opengl_texture::unbind(size_t unit)
{
    if (GLEW_ARB_direct_state_access)
    {
        glBindTextureUnit(unit, 0);
    }
    else
    {
        if (unit != m_activeunit)
        {
            glActiveTexture(GL_TEXTURE0 + unit);
            m_activeunit = unit;
        }
        //todo: for other targets
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void opengl_texture::reset_unit_cache()
{
    for( auto &unit : units ) {
        unit = 0;
    }
    m_activeunit = -1;
}

std::array<GLuint, gl::MAX_TEXTURES + 2> opengl_texture::units = { 0 };
GLint opengl_texture::m_activeunit = -1;

std::unordered_map<GLint, int> opengl_texture::precompressed_formats =
{
    { GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, 8 },
    { GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, 16 },
    { GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, 16 },
    { GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, 8 },
    { GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 16 },
    { GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, 16 },
};

std::unordered_map<GLint, GLint> opengl_texture::drivercompressed_formats =
{
    { GL_SRGB_ALPHA, GL_COMPRESSED_SRGB_ALPHA },
    { GL_SRGB, GL_COMPRESSED_SRGB },
    { GL_RGBA, GL_COMPRESSED_RGBA },
    { GL_RGB, GL_COMPRESSED_RGB },
    { GL_RG, GL_COMPRESSED_RG },
    { GL_RED, GL_COMPRESSED_RED },
};

std::unordered_map<GLint, std::unordered_map<GLint, GLint>> opengl_texture::mapping =
{
    // image have,                         material wants, gl internalformat
    { GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, { { GL_SRGB_ALPHA, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT },
                                          { GL_SRGB,       GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT },
                                          { GL_RGBA,       GL_COMPRESSED_RGBA_S3TC_DXT1_EXT },
                                          { GL_RGB,        GL_COMPRESSED_RGBA_S3TC_DXT1_EXT },
                                          { GL_RG,         GL_COMPRESSED_RGBA_S3TC_DXT1_EXT },
                                          { GL_RED,        GL_COMPRESSED_RGBA_S3TC_DXT1_EXT } } },
    { GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, { { GL_SRGB_ALPHA, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT },
                                          { GL_SRGB,       GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT },
                                          { GL_RGBA,       GL_COMPRESSED_RGBA_S3TC_DXT3_EXT },
                                          { GL_RGB,        GL_COMPRESSED_RGBA_S3TC_DXT3_EXT },
                                          { GL_RG,         GL_COMPRESSED_RGBA_S3TC_DXT3_EXT },
                                          { GL_RED,        GL_COMPRESSED_RGBA_S3TC_DXT3_EXT } } },
    { GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, { { GL_SRGB_ALPHA, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT },
                                          { GL_SRGB,       GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT },
                                          { GL_RGBA,       GL_COMPRESSED_RGBA_S3TC_DXT5_EXT },
                                          { GL_RGB,        GL_COMPRESSED_RGBA_S3TC_DXT5_EXT },
                                          { GL_RG,         GL_COMPRESSED_RGBA_S3TC_DXT5_EXT },
                                          { GL_RED,        GL_COMPRESSED_RGBA_S3TC_DXT5_EXT } } },
    { GL_RGBA,                          { { GL_SRGB_ALPHA, GL_SRGB_ALPHA },
                                          { GL_SRGB,       GL_SRGB },
                                          { GL_RGBA,       GL_RGBA },
                                          { GL_RGB,        GL_RGB },
                                          { GL_RG,         GL_RG },
                                          { GL_RED,        GL_RED } } },
    { GL_RGB,                           { { GL_SRGB_ALPHA, GL_SRGB }, // bad
                                          { GL_SRGB,       GL_SRGB },
                                          { GL_RGBA,       GL_RGB }, // bad
                                          { GL_RGB,        GL_RGB },
                                          { GL_RG,         GL_RG },
                                          { GL_RED,        GL_RED } } },
    { GL_RG,                            { { GL_SRGB_ALPHA, GL_SRGB }, // bad
                                          { GL_SRGB,       GL_SRGB }, // bad
                                          { GL_RGBA,       GL_RG }, // bad
                                          { GL_RGB,        GL_RG }, // bad
                                          { GL_RG,         GL_RG },
                                          { GL_RED,        GL_RED } } },
    { GL_RED,                           { { GL_SRGB_ALPHA, GL_SRGB }, // bad
                                          { GL_SRGB,       GL_SRGB }, // bad
                                          { GL_RGBA,       GL_RED }, // bad
                                          { GL_RGB,        GL_RED },  // bad
                                          { GL_RG,         GL_RED }, // bad
                                          { GL_RED,        GL_RED } } },
};

void opengl_texture::set_components_hint(GLint hint)
{
    components_hint = hint;
}

bool
opengl_texture::create() {

    if( data_state != resource_state::good && !is_rendertarget ) {
        // don't bother until we have useful texture data
        // and it isn't rendertarget texture without loaded data
        return false;
    }

    // TODO: consider creating and storing low-res version of the texture if it's ever unloaded from the gfx card,
    // as a placeholder until it can be loaded again
    if( id == -1 ) {

        ::glGenTextures( 1, &id );
        ::glBindTexture( target, id );

        // analyze specified texture traits
        bool wraps{ true };
        bool wrapt{ true };
        for( auto const &trait : traits ) {

            switch( trait ) {

                case 's': { wraps = false; break; }
                case 't': { wrapt = false; break; }
            }
        }

        // upload texture data
        int dataoffset = 0,
            datasize = 0,
            datawidth = data_width,
            dataheight = data_height;
        if (is_rendertarget)
        {
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			if (data_components == GL_DEPTH_COMPONENT)
            {
				glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
                float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
				glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, borderColor);
			}
			if (target == GL_TEXTURE_2D)
				glTexImage2D(target, 0, data_format, data_width, data_height, 0, data_components, data_type, nullptr);
			else if (target == GL_TEXTURE_2D_MULTISAMPLE)
				glTexImage2DMultisample(target, samples, data_format, data_width, data_height, GL_FALSE);
        }
        else
        {
			::glTexParameteri(target, GL_TEXTURE_WRAP_S, (wraps == true ? GL_REPEAT : GL_CLAMP_TO_EDGE));
			::glTexParameteri(target, GL_TEXTURE_WRAP_T, (wrapt == true ? GL_REPEAT : GL_CLAMP_TO_EDGE));
            set_filtering();

            // data_format and data_type specifies how image is laid out in memory
            // data_components specifies what useful channels image contains
            // components_hint specifies what format we want to load

            // now map that mess into opengl internal format

            GLint components = data_components;
            auto f_it = precompressed_formats.find(data_format);
            if (f_it != precompressed_formats.end())
                components = data_format;

            GLint internal_format = mapping[components][components_hint];

            auto blocksize_it = precompressed_formats.find(internal_format);

            for( int maplevel = 0; maplevel < data_mapcount; ++maplevel ) {

                if (blocksize_it != precompressed_formats.end())
                {
                    // compressed dds formats
                    const int datablocksize = blocksize_it->second;

                    datasize = ( ( std::max( datawidth, 4 ) + 3 ) / 4 ) * ( ( std::max( dataheight, 4 ) + 3 ) / 4 ) * datablocksize;

                    ::glCompressedTexImage2D(
                        target, maplevel, internal_format,
                        datawidth, dataheight, 0,
                        datasize, (GLubyte *)&data[ dataoffset ] );

                    dataoffset += datasize;
                    datawidth = std::max( datawidth / 2, 1 );
                    dataheight = std::max( dataheight / 2, 1 );
                }
                else {
                    GLint compressed_format = drivercompressed_formats[internal_format];

                    // uncompressed texture data. have the gfx card do the compression as it sees fit
                    ::glTexImage2D(
                        target, 0,
                        Global.compress_tex ? compressed_format : internal_format,
                        data_width, data_height, 0,
                        data_format, data_type, (GLubyte *)&data[ 0 ] );
                }
            }

            if( data_mapcount == 1 ) {
                // fill missing mipmaps if needed
                glGenerateMipmap(target);
            }

            if( ( true == Global.ResourceMove )
             || ( false == Global.ResourceSweep ) ) {
                // if garbage collection is disabled we don't expect having to upload the texture more than once
                data = std::vector<char>();
                data_state = resource_state::none;
            }
        }
        is_ready = true;
    }

    return true;
}

// releases resources allocated on the opengl end, storing local copy if requested
void
opengl_texture::release() {

    if( id == -1 ) { return; }

    if( true == Global.ResourceMove && !is_rendertarget ) {
        // if resource move is enabled we don't keep a cpu side copy after upload
        // so need to re-acquire the data before release
        // TBD, TODO: instead of vram-ram transfer fetch the data 'normally' from the disk using worker thread
        ::glBindTexture(target, id );
        GLint datasize {};
        GLint iscompressed {};
        ::glGetTexLevelParameteriv(target, 0, GL_TEXTURE_COMPRESSED, &iscompressed );
        if( iscompressed == GL_TRUE ) {
            // texture is compressed on the gpu side
            // query texture details needed to perform the backup...
            ::glGetTexLevelParameteriv(target, 0, GL_TEXTURE_INTERNAL_FORMAT, &data_format );
            ::glGetTexLevelParameteriv(target, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &datasize );
            data.resize( datasize );
            // ...fetch the data...
            ::glGetCompressedTexImage(target, 0, &data[ 0 ] );
        }
        else {
            // for whatever reason texture didn't get compressed during upload
            // fallback on plain rgba storage...
            data_format = GL_RGBA;
            data_type = GL_UNSIGNED_BYTE;
            data.resize( data_width * data_height * 4 );
            // ...fetch the data...
            ::glGetTexImage(target, 0, data_format, GL_UNSIGNED_BYTE, &data[ 0 ] );
        }
        // ...and update texture object state
        data_mapcount = 1; // we keep copy of only top mipmap level
        data_state = resource_state::good;
    }
    // release opengl resources
    ::glDeleteTextures( 1, &id );
    id = -1;
    is_ready = false;

    return;
}

void opengl_texture::alloc_rendertarget(GLint format, GLint components, GLint type, int width, int height, int s)
{
    data_width = width;
    data_height = height;
    data_format = format;
    data_components = components;
    data_type = type;
    data_mapcount = 1;
    is_rendertarget = true;
	samples = s;
	if (samples > 1)
		target = GL_TEXTURE_2D_MULTISAMPLE;
    create();
}

void
opengl_texture::set_filtering() const
{
    // default texture mode
    ::glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    ::glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    ::glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, Global.AnisotropicFiltering );

    bool sharpen{ false };
    for( auto const &trait : traits ) {

        switch( trait ) {

            case '#': { sharpen = true; break; }
            default:  {                 break; }
        }
    }
}

void
opengl_texture::downsize( GLuint const Format ) {

    while( ( data_width > Global.iMaxTextureSize ) || ( data_height > Global.iMaxTextureSize ) ) {
        // scale down the base texture, if it's larger than allowed maximum
        // NOTE: scaling is uniform along both axes, meaning non-square textures can drop below the maximum
        // TODO: replace with proper scaling function once we have image middleware in place
        if( ( data_width < 2 ) || ( data_height < 2 ) ) {
            // can't go any smaller
            break;
        }

        WriteLog( "Texture size exceeds specified limits, downsampling data" );
        switch( Format ) {

            case GL_RGB:  { downsample< glm::tvec3<std::uint8_t> >( data_width, data_height, data.data() ); break; }
            case GL_BGRA:
            case GL_RGBA: { downsample< glm::tvec4<std::uint8_t> >( data_width, data_height, data.data() ); break; }
            default:      { break; }
        }
        data_width /= 2;
        data_height /= 2;
        data.resize( data.size() / 4 ); // not strictly needed, but, eh
    };
}

void
opengl_texture::flip_vertical() {

    auto const swapsize { data_width * 4 };
    auto destination { data.begin() + ( data_height - 1 ) * swapsize };
    auto sampler { data.begin() };

    for( auto row = 0; row < data_height / 2; ++row ) {

        std::swap_ranges( sampler, sampler + swapsize, destination );
        sampler += swapsize;
        destination -= swapsize;
    }
}

// ustalenie numeru tekstury, wczytanie jeśli jeszcze takiej nie było
texture_handle
texture_manager::create(std::string Filename, bool const Loadnow , GLint fh) {

    if( Filename.find( '|' ) != std::string::npos )
        Filename.erase( Filename.find( '|' ) ); // po | może być nazwa kolejnej tekstury

    std::string traits;
    auto const traitpos = Filename.find( ':' );
    if( traitpos != std::string::npos ) {
        // po dwukropku mogą być podane dodatkowe informacje niebędące nazwą tekstury
        if( Filename.size() > traitpos + 1 )
            traits = Filename.substr( traitpos + 1 );
        Filename.erase( traitpos );
    }

    erase_extension( Filename );

    if( Filename[ 0 ] == '/' ) {
        // filename can potentially begin with a slash, and we don't need it
        Filename.erase( 0, 1 );
    }

    // try to locate requested texture in the databank
    auto lookup { find_in_databank( Filename ) };
    if( lookup != npos ) {
        return lookup;
    }
    // if we don't have the texture in the databank, check if it's on disk
    auto const disklookup { find_on_disk( Filename ) };

    if( true == disklookup.first.empty() ) {
        // there's nothing matching in the databank nor on the disk, report failure
        ErrorLog( "Bad file: failed do locate texture file \"" + Filename + "\"", logtype::file );
        return npos;
    }

    auto texture = new opengl_texture();
    texture->name = disklookup.first + disklookup.second;
    if( Filename.find('#') != std::string::npos ) {
        // temporary code for legacy assets -- textures with names beginning with # are to be sharpened
        traits += '#';
    }
    texture->traits = traits;
    texture->components_hint = fh;
    auto const textureindex = (texture_handle)m_textures.size();
    m_textures.emplace_back( texture, std::chrono::steady_clock::time_point() );
    m_texturemappings.emplace( disklookup.first, textureindex );

    WriteLog( "Created texture object for \"" + disklookup.first + disklookup.second + "\"", logtype::texture );

    if( true == Loadnow ) {

        texture_manager::texture( textureindex ).load();
#ifndef EU07_DEFERRED_TEXTURE_UPLOAD
        texture_manager::texture( textureindex ).create();
        // texture creation binds a different texture, force a re-bind on next use
        m_activetexture = -1;
#endif
    }

    return textureindex;
};

void
texture_manager::bind( std::size_t const Unit, texture_handle const Texture ) {

    mark_as_used(Texture);

    if (Texture != null_handle)
        texture(Texture).bind(Unit);
    else
        opengl_texture::unbind(Unit);
}

opengl_texture &texture_manager::mark_as_used(const texture_handle Texture)
{
    auto &pair = m_textures[ Texture ];
    pair.second = m_garbagecollector.timestamp();
    return *pair.first;
}

void
texture_manager::delete_textures() {
    for( auto const &texture : m_textures ) {
        // usunięcie wszyskich tekstur (bez usuwania struktury)
        if( ( texture.first->id > 0 )
         && ( texture.first->id != -1 ) ) {
            ::glDeleteTextures( 1, &(texture.first->id) );
        }
        delete texture.first;
    }
}

// performs a resource sweep
void
texture_manager::update() {

    if( m_garbagecollector.sweep() > 0 ) {
        opengl_texture::reset_unit_cache();
    }
}

// debug performance string
std::string
texture_manager::info() const {

    // TODO: cache this data and update only during resource sweep
    std::size_t totaltexturecount{ m_textures.size() - 1 };
    std::size_t totaltexturesize{ 0 };
#ifdef EU07_DEFERRED_TEXTURE_UPLOAD
    std::size_t readytexturecount{ 0 };
    std::size_t readytexturesize{ 0 };
#endif

    for( auto const& texture : m_textures ) {

        totaltexturesize += texture.first->size;
#ifdef EU07_DEFERRED_TEXTURE_UPLOAD

        if( texture.first->is_ready ) {

            ++readytexturecount;
            readytexturesize += texture.first->size;
        }
#endif
    }

    return
        "; textures: "
#ifdef EU07_DEFERRED_TEXTURE_UPLOAD
        + std::to_string( readytexturecount )
        + " ("
        + to_string( readytexturesize / 1024.0f, 2 ) + " mb)"
        + " in vram, "
#endif
        + std::to_string( totaltexturecount )
        + " ("
        + to_string( totaltexturesize / 1024.0f, 2 ) + " mb)"
        + " total";
}

// checks whether specified texture is in the texture bank. returns texture id, or npos.
texture_handle
texture_manager::find_in_databank( std::string const &Texturename ) const {

    std::vector<std::string> const filenames {
        Global.asCurrentTexturePath + Texturename,
        Texturename,
        szTexturePath + Texturename };

    for( auto const &filename : filenames ) {
        auto const lookup { m_texturemappings.find( filename ) };
        if( lookup != m_texturemappings.end() ) {
            return lookup->second;
        }
    }
    // all lookups failed
    return npos;
}

// checks whether specified file exists.
std::pair<std::string, std::string>
texture_manager::find_on_disk( std::string const &Texturename ) const {

    std::vector<std::string> const filenames {
        Global.asCurrentTexturePath + Texturename,
        Texturename,
        szTexturePath + Texturename };

    auto lookup =
        FileExists(
            filenames,
            { Global.szDefaultExt } );

    if( false == lookup.first.empty() ) {
        return lookup;
    }

    // if the first attempt fails, try entire extension list
    // NOTE: slightly wasteful as it means preferred extension is tested twice, but, eh
    return (
        FileExists(
            filenames,
            { ".dds", ".tga", ".png", ".bmp", ".ext" } ) );
}

//---------------------------------------------------------------------------
