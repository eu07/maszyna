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
#include "usefull.h"
#include "Globals.h"

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

    if( false == Input.getTokens( 2, true, "\n\r\t;, " ) ) {
        return false;
    }

    std::string path;
    if( name.rfind( '/' ) != std::string::npos ) {
        path = name.substr( 0, name.rfind( '/' ) + 1 );
    }

    std::string key, value;
    Input
        >> key
        >> value;

    if( value == "{" ) {
        // detect and optionally process config blocks
        cParser blockparser( Input.getToken<std::string>( false, "}" ) );
        if( key == Global::Season ) {
            // seasonal textures override generic textures
            while( true == deserialize_mapping( blockparser, 1, Loadnow ) ) {
                ; // all work is done in the header
            }
        }
    }
    else if( key == "texture1:" ) {
        // TODO: full-fledged priority system
        if( ( texture1 == null_handle )
         || ( Priority > 0 ) ) {
            texture1 = GfxRenderer.Fetch_Texture( path + value, Loadnow );
        }
    }
    else if( key == "texture2:" ) {
        // TODO: full-fledged priority system
        if( ( texture2 == null_handle )
         || ( Priority > 0 ) ) {
            texture2 = GfxRenderer.Fetch_Texture( path + value, Loadnow );
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

    if( ( filename.rfind( '.' ) != std::string::npos )
     && ( filename.rfind( '.' ) != filename.rfind( ".." ) + 1 ) ) {
        // we can get extension for .mat or, in legacy files, some image format. just trim it and set it to material file extension
        filename.erase( filename.rfind( '.' ) );
    }
    filename += ".mat";

	std::replace(filename.begin(), filename.end(), '\\', '/'); // fix slashes

    if( filename.find( '/' ) == std::string::npos ) {
        // jeśli bieżaca ścieżka do tekstur nie została dodana to dodajemy domyślną
        filename = global_texture_path + filename;
    }

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
    lookup = m_materialmappings.find( global_texture_path + Materialname );

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
        FileExists( global_texture_path + Materialname ) ? global_texture_path + Materialname :
        "" );
}

//---------------------------------------------------------------------------
