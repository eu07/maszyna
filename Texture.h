/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include <string>
#include "opengl/glew.h"

enum resource_state {
    none,
    loading,
    good,
    failed
};

struct opengl_texture {

    GLuint id{ -1 }; // associated GL resource
    bool has_alpha{ false }; // indicates the texture has alpha channel
    bool is_ready{ false }; // indicates the texture was processed and is ready for use
    std::string attributes; // requested texture attributes: wrapping modes etc
    std::string name; // name of the texture source file
    std::vector<char> data; // texture data
    resource_state data_state{ none }; // current state of texture data
    int data_width{ 0 },
        data_height{ 0 },
        data_mapcount{ 0 };
    GLuint data_format{ 0 },
        data_components{ 0 };
/*
    std::atomic<bool> is_loaded{ false }; // indicates the texture data was loaded and can be processed
    std::atomic<bool> is_good{ false }; // indicates the texture data was retrieved without errors
*/
    void load();
    void create();

private:
//    void load_BMP();
    void load_DDS();
//    void load_TEX();
//    void load_TGA();
    void set_filtering();
};

class texture_manager {

private:
    typedef std::vector<opengl_texture> opengltexture_array;

public:
//    typedef opengltexture_array::size_type size_type;
    typedef int size_type;

    texture_manager();
    ~texture_manager() { Free(); }

    size_type GetTextureId( std::string Filename, std::string const &Dir, int const Filter = -1, bool const Loadnow = true );
    void Bind( size_type const Id );
    opengl_texture &Texture( size_type const Id ) { return m_textures.at( Id ); }
    void Init();
    void Free();

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
};

extern texture_manager TextureManager;