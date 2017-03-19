/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <istream>
#include <ddraw.h>
#include <string>
#include "GL/glew.h"

enum class resource_state {
    none,
    loading,
    good,
    failed
};

struct opengl_texture {
	static DDSURFACEDESC2 deserialize_ddsd(std::istream&);
	static DDCOLORKEY deserialize_ddck(std::istream&);
	static DDPIXELFORMAT deserialize_ddpf(std::istream&);
	static DDSCAPS2 deserialize_ddscaps(std::istream&);

    // methods
    void load();
    resource_state bind();
    resource_state create();
    // members
    GLuint id{ (GLuint)-1 }; // associated GL resource
    bool has_alpha{ false }; // indicates the texture has alpha channel
    bool is_ready{ false }; // indicates the texture was processed and is ready for use
    std::string traits; // requested texture attributes: wrapping modes etc
    std::string name; // name of the texture source file
    std::size_t size{ 0 }; // size of the texture data, in kb

private:
    // methods
    void load_BMP();
    void load_DDS();
    void load_TEX();
    void load_TGA();
    void set_filtering();
    void downsize( GLuint const Format );

    // members
    std::vector<char> data; // texture data
    resource_state data_state{ resource_state::none }; // current state of texture data
    int data_width{ 0 },
        data_height{ 0 },
        data_mapcount{ 0 };
    GLuint data_format{ 0 },
        data_components{ 0 };
/*
    std::atomic<bool> is_loaded{ false }; // indicates the texture data was loaded and can be processed
    std::atomic<bool> is_good{ false }; // indicates the texture data was retrieved without errors
*/
};

class texture_manager {

private:
    typedef std::vector<opengl_texture> opengltexture_array;

public:
//    typedef opengltexture_array::size_type size_type;
    typedef int size_type;

    texture_manager();
    ~texture_manager() { Free(); }

    size_type
        GetTextureId( std::string Filename, std::string const &Dir, int const Filter = -1, bool const Loadnow = true );
    void
        Bind( size_type const Id );
    opengl_texture &
        Texture( size_type const Id ) { return m_textures[ Id ]; }
    void
        Init();
    void
        Free();
    // debug performance string
    std::string
        Info() const;

private:
    typedef std::unordered_map<std::string, size_type> index_map;
/*
    opengltexture_array::size_type LoadFromFile(std::string name, int filter = -1);
*/
/*
    bool LoadBMP( std::string const &fileName);
    bool LoadTEX( std::string fileName );
    bool LoadTGA( std::string fileName, int filter = -1 );
    bool LoadDDS( std::string fileName, int filter = -1 );
*/
    // checks whether specified texture is in the texture bank. returns texture id, or npos.
    size_type find_in_databank( std::string const &Texturename );
    // checks whether specified file exists. returns name of the located file, or empty string.
    std::string find_on_disk( std::string const &Texturename );
/*
    void SetFiltering(int filter);
    void SetFiltering(bool alpha, bool hash);
    GLuint CreateTexture(GLubyte *buff, GLint bpp, int width, int height, bool bHasAlpha,
                         bool bHash, bool bDollar = true, int filter = -1);
*/
    static const size_type npos{ 0 }; // should be -1, but the rest of the code uses -1 for something else
    opengltexture_array m_textures;
    index_map m_texturemappings;
    size_type m_activetexture{ 0 }; // last i.e. currently bound texture
};

// reduces provided data image to half of original size, using basic 2x2 average
template <typename _Colortype>
void
downsample( std::size_t const Width, std::size_t const Height, char *Imagedata ) {

    _Colortype *destination = reinterpret_cast<_Colortype*>( Imagedata );
    _Colortype *sampler = reinterpret_cast<_Colortype*>( Imagedata );

    _Colortype accumulator, color;
/*
    _Colortype color;
    float component;
*/
    for( size_t row = 0; row < Height; row += 2, sampler += Width ) { // column movement advances us down another row
        for( size_t column = 0; column < Width; column += 2, sampler += 2 ) {
/*
            // straightforward, but won't work with byte data
            auto color = (
                *sampler
                + *( sampler + 1 )
                + *( sampler + Width )
                + *( sampler + Width + 1 ) );
            color /= 4;
*/
            // manual version of the above, but drops colour resolution to 6 bits
            accumulator = *sampler;
            accumulator /= 4;
            color = accumulator;
            accumulator = *(sampler + 1);
            accumulator /= 4;
            color += accumulator;
            accumulator = *(sampler + Width);
            accumulator /= 4;
            color += accumulator;
            accumulator = *(sampler + Width + 1);
            accumulator /= 4;
            color += accumulator;

            *destination++ = color;
/*
            // "full" 8bit resolution
            color = _Colortype(); component = 0;
            for( int idx = 0; idx < sizeof( _Colortype ); ++idx ) {

                component = (
                    (*sampler)[idx]
                    + ( *( sampler + 1 ) )[idx]
                    + ( *( sampler + Width ) )[idx]
                    + (*( sampler + Width + 1 ))[idx] );
                color[ idx ] = component /= 4;
            }
            *destination++ = color;
*/
        }
    }
}
