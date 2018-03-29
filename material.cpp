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
#include "utilities.h"
#include "globals.h"

// helper, returns potential path part from provided file name
std::string path( std::string const &Filename ) {
    return (
        Filename.rfind( '/' ) != std::string::npos ?
            Filename.substr( 0, Filename.rfind( '/' ) + 1 ) :
            "" );
}

bool
opengl_material::deserialize( cParser &Input, bool const Loadnow ) {

    bool result { false };
    while( true == deserialize_mapping( Input, 0, Loadnow ) ) {
        result = true; // once would suffice but, eh
    }

    has_alpha = (
        texture1 != null_handle ?
            GfxRenderer.Texture( texture1 ).has_alpha :
            false );

    return result;
}

// imports member data pair from the config file
bool
opengl_material::deserialize_mapping( cParser &Input, int const Priority, bool const Loadnow ) {
    // token can be a key or block end
    std::string const key { Input.getToken<std::string>( true, "\n\r\t  ,;[]" ) };

    if( ( true == key.empty() ) || ( key == "}" ) ) { return false; }

    auto const value { Input.getToken<std::string>( true, "\n\r\t ,;" ) };

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
        else if( key == "texture1:" ) {
            if( ( texture1 == null_handle )
             || ( Priority > priority1 ) ) {
                texture1 = GfxRenderer.Fetch_Texture( value, Loadnow );
                priority1 = Priority;
            }
        }
        else if( key == "texture2:" ) {
            if( ( texture2 == null_handle )
             || ( Priority > priority2 ) ) {
                texture2 = GfxRenderer.Fetch_Texture( value, Loadnow );
                priority2 = Priority;
            }
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
    replace_slashes( filename );

    filename += ".mat";

    // try to locate requested material in the databank
    auto const databanklookup = find_in_databank( filename );
    if( databanklookup != null_handle ) {
        return databanklookup;
    }
    // if this fails, try to look for it on disk
    opengl_material material;
    material.name = filename;
    auto const disklookup = find_on_disk( filename );
    if( disklookup != "" ) {
        cParser materialparser( disklookup, cParser::buffer_FILE );
        if( false == material.deserialize( materialparser, Loadnow ) ) {
            // deserialization failed but the .mat file does exist, so we give up at this point
            return null_handle;
        }
    }
    else {
        // if there's no .mat file, this could be legacy method of referring just to diffuse texture directly, make a material out of it in such case
        material.texture1 = GfxRenderer.Fetch_Texture( Filename, Loadnow );
        if( material.texture1 == null_handle ) {
            // if there's also no texture, give up
            return null_handle;
        }
        material.has_alpha = GfxRenderer.Texture( material.texture1 ).has_alpha;
    }

    material_handle handle = m_materials.size();
    m_materials.emplace_back( material );
    m_materialmappings.emplace( material.name, handle );
    return handle;
};

// checks whether specified texture is in the texture bank. returns texture id, or npos.
material_handle
material_manager::find_in_databank( std::string const &Materialname ) const {

    auto lookup = m_materialmappings.find( Materialname );
    if( lookup != m_materialmappings.end() ) {
        return lookup->second;
    }
    // jeszcze próba z dodatkową ścieżką
    lookup = m_materialmappings.find( szTexturePath + Materialname );

    return (
        lookup != m_materialmappings.end() ?
            lookup->second :
            null_handle );
}

// checks whether specified file exists.
// NOTE: this is direct copy of the method used by texture manager. TBD, TODO: refactor into common routine?
std::string
material_manager::find_on_disk( std::string const &Materialname ) const {

    return(
        FileExists( Materialname ) ? Materialname :
        FileExists( szTexturePath + Materialname ) ? szTexturePath + Materialname :
        "" );
}

//---------------------------------------------------------------------------
