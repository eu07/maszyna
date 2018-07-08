/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"

#include "material.h"
#include "renderer.h"
#include "Globals.h"
#include "utilities.h"

bool
opengl_material::deserialize( cParser &Input, bool const Loadnow ) {

    bool result { false };
    while( true == deserialize_mapping( Input, 0, Loadnow ) ) {
        result = true; // once would suffice but, eh
    }

    return result;
}

// imports member data pair from the config file
bool
opengl_material::deserialize_mapping( cParser &Input, int const Priority, bool const Loadnow ) {
    // token can be a key or block end
    std::string key = Input.getToken<std::string>( true, "\n\r\t  ,;[]" );

    if( ( true == key.empty() ) || ( key == "}" ) ) { return false; }

    auto value { Input.getToken<std::string>( true, "\n\r\t ,;" ) };

    if( Priority != -1 ) {
        // regular attribute processing mode
        if( key == Global.Weather ) {
            // weather textures override generic (pri 0) and seasonal (pri 1) textures
            // seasonal weather textures (pri 1+2=3) override generic weather (pri 2) textures
            while( true == deserialize_mapping( Input, Priority + 2, Loadnow ) ) {
                ; // all work is done in the header
            }
        }
        else if( key == Global.Season ) {
            // seasonal textures override generic textures
            while( true == deserialize_mapping( Input, 1, Loadnow ) ) {
                ; // all work is done in the header
            }
        }
        else if (key.compare(0, 7, "texture") == 0) {
            key.erase(0, 7);
            size_t num = std::stoi(key) - 1;
            if (num < textures.size() &&
                    (textures[num] == null_handle || Priority > m_priority[num]))
            {
				std::replace(value.begin(), value.end(), '\\', '/');
                textures[num] = GfxRenderer.Fetch_Texture( value, Loadnow );
                m_priority[num] = Priority;
            }
        }
        else if (key == "shader:" &&
                (!shader || Priority > m_priority[max_textures]))
        {
            shader = GfxRenderer.Fetch_Shader(value);
            m_priority[max_textures] = Priority;
        }
        else if (key == "opacity:" &&
                Priority > m_priority[max_textures + 1])
        {
            opacity = std::stoi(value); //m7t: handle exception
            m_priority[max_textures + 1] = Priority;
        }
        else if( value == "{" ) {
            // unrecognized or ignored token, but comes with attribute block and potential further nesting
            // go through it and discard the content
            while( true == deserialize_mapping( Input, -1, Loadnow ) ) {
                ; // all work is done in the header
            }
        }
    }
    else {
        // discard mode; ignores all retrieved tokens
        if( value == "{" ) {
            // ignored tokens can come with their own blocks, ignore these recursively
            // go through it and discard the content
            while( true == deserialize_mapping( Input, -1, Loadnow ) ) {
                ; // all work is done in the header
            }
        }
    }

    return true; // return value marks a key: value pair was extracted, nothing about whether it's recognized
}


// create material object from data stored in specified file.
// NOTE: the deferred load parameter is passed to textures defined by material, the material itself is always loaded immediately
material_handle
material_manager::create( std::string const &Filename, bool const Loadnow ) {

    auto filename { Filename };

    if( filename.find( '|' ) != std::string::npos )
        filename.erase( filename.find( '|' ) ); // po | może być nazwa kolejnej tekstury

    erase_extension( filename );

    if( filename[ 0 ] == '/' ) {
        // filename can potentially begin with a slash, and we don't need it
        filename.erase( 0, 1 );
    }

    // try to locate requested material in the databank
    auto const databanklookup { find_in_databank( filename ) };
    if( databanklookup != null_handle ) {
        return databanklookup;
    }
    // if this fails, try to look for it on disk
    opengl_material material;
    auto const disklookup { find_on_disk( filename ) };
    if( false == disklookup.first.empty() ) {
        cParser materialparser( disklookup.first + disklookup.second, cParser::buffer_FILE );
        if( false == material.deserialize( materialparser, Loadnow ) ) {
            // deserialization failed but the .mat file does exist, so we give up at this point
            return null_handle;
        }
        material.name = disklookup.first;
    }
    else {
        // if there's no .mat file, this could be legacy method of referring just to diffuse texture directly, make a material out of it in such case
        material.textures[0] = GfxRenderer.Fetch_Texture( Filename, Loadnow );
        if( material.textures[0] == null_handle ) {
            // if there's also no texture, give up
            return null_handle;
        }
        // use texture path and name to tell the newly created materials apart
        filename = GfxRenderer.Texture( material.textures[0] ).name;
        erase_extension( filename );
        material.name = filename;
    }

    if (!material.shader)
        material.shader = GfxRenderer.Fetch_Shader("default");

    if (std::isnan(material.opacity))
    {
        if (material.textures[0] != null_handle)
            material.opacity = GfxRenderer.Texture( material.textures[0] ).has_alpha ? 0.0f : 1.0f;
        else
            material.opacity = 1.0f;
    }

    material_handle handle = m_materials.size();
    m_materials.emplace_back( material );
    m_materialmappings.emplace( material.name, handle );
    return handle;
};

// checks whether specified material is in the material bank. returns handle to the material, or a null handle
material_handle
material_manager::find_in_databank( std::string const &Materialname ) const {

    std::vector<std::string> const filenames {
        Global.asCurrentTexturePath + Materialname,
        Materialname,
        szTexturePath + Materialname };

    for( auto const &filename : filenames ) {
        auto const lookup { m_materialmappings.find( filename ) };
        if( lookup != m_materialmappings.end() ) {
            return lookup->second;
        }
    }
    // all lookups failed
    return null_handle;
}

// checks whether specified file exists.
// NOTE: technically could be static, but we might want to switch from global texture path to instance-specific at some point
std::pair<std::string, std::string>
material_manager::find_on_disk( std::string const &Materialname ) const {

    return (
        FileExists(
            { Global.asCurrentTexturePath + Materialname, Materialname, szTexturePath + Materialname },
            { ".mat" } ) );
}

//---------------------------------------------------------------------------
