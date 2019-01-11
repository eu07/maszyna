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
#include "Logs.h"
#include "sn_utils.h"

opengl_material::opengl_material()
{
    for (size_t i = 0; i < params.size(); i++)
        params[i] = glm::vec4(std::numeric_limits<float>::quiet_NaN());
}

bool
opengl_material::deserialize( cParser &Input, bool const Loadnow ) {
    parse_info = std::make_unique<parse_info_s>();

    bool result { false };
    while( true == deserialize_mapping( Input, 0, Loadnow ) ) {
        result = true; // once would suffice but, eh
    }

    return result;
}

void opengl_material::log_error(const std::string &str)
{
    ErrorLog("bad material: " + name + ": " + str, logtype::material);
}

void opengl_material::finalize(bool Loadnow)
{
    if (parse_info)
    {
        for (auto it : parse_info->tex_mapping)
        {
            std::string key = it.first;
            std::string value = it.second.name;

            if (key.size() > 0 && key[0] != '_')
            {
                size_t num = std::stoi(key) - 1;
                if (num < gl::MAX_TEXTURES)
                    textures[num] = GfxRenderer.Fetch_Texture(value, Loadnow);
                else
                    log_error("invalid texture binding: " + std::to_string(num));
            }
            else if (key.size() > 2)
            {
                key.erase(0, 1);
                key.pop_back();
                if (shader && shader->texture_conf.find(key) != shader->texture_conf.end())
                    textures[shader->texture_conf[key].id] = GfxRenderer.Fetch_Texture(value, Loadnow);
                else
                    log_error("unknown texture binding: " + key);
            }
            else
                log_error("unrecognized texture binding: " + key);
        }

        if (!shader)
        {
            if (textures[0] == null_handle)
            {
                log_error("shader not specified, assuming \"default_0\"");
                shader = GfxRenderer.Fetch_Shader("default_0");
            }
            else if (textures[1] == null_handle)
            {
                log_error("shader not specified, assuming \"default_1\"");
                shader = GfxRenderer.Fetch_Shader("default_1");
            }
            else if (textures[2] == null_handle)
            {
                log_error("shader not specified, assuming \"default_2\"");
                shader = GfxRenderer.Fetch_Shader("default_2");
            }
        }

        if (!shader)
            return;

        for (auto it : parse_info->param_mapping)
        {
            std::string key = it.first;
            glm::vec4 value = it.second.data;

            if (key.size() > 1 && key[0] != '_')
            {
                size_t num = std::stoi(key) - 1;
                if (num < gl::MAX_PARAMS)
                    params[num] = value;
                else
                    log_error("invalid param binding: " + std::to_string(num));
            }
            else if (key.size() > 2)
            {
                key.erase(0, 1);
                key.pop_back();
                if (shader->param_conf.find(key) != shader->param_conf.end())
                {
                    gl::shader::param_entry entry = shader->param_conf[key];
                    for (size_t i = 0; i < entry.size; i++)
                        params[entry.location][entry.offset + i] = value[i];
                }
                else
                    log_error("unknown param binding: " + key);
            }
            else
                log_error("unrecognized param binding: " + key);
        }

        parse_info.reset();
    }

    for (auto it : shader->param_conf)
    {
        gl::shader::param_entry entry = it.second;
        if (std::isnan(params[entry.location][entry.offset]))
        {
            float value = std::numeric_limits<float>::quiet_NaN();
            if (entry.defaultparam == gl::shader::defaultparam_e::one)
                value = 1.0f;
            else if (entry.defaultparam == gl::shader::defaultparam_e::zero)
                value = 0.0f;
            else if (entry.defaultparam == gl::shader::defaultparam_e::required)
                log_error("unspecified required param: " + it.first);
            else if (entry.defaultparam != gl::shader::defaultparam_e::nan)
            {
                params_state.push_back(entry);
                continue;
            }

            for (size_t i = 0; i < entry.size; i++)
                params[entry.location][entry.offset + i] = value;
        }
    }

    for (auto it : shader->texture_conf)
    {
        gl::shader::texture_entry &entry = it.second;
        texture_handle handle = textures[entry.id];
        if (handle)
            GfxRenderer.Texture(handle).set_components_hint((GLint)entry.components);
        else
            log_error("missing texture: " + it.first);
    }
}

// imports member data pair from the config file
bool
opengl_material::deserialize_mapping( cParser &Input, int const Priority, bool const Loadnow ) {

    // NOTE: comma can be part of legacy file names, so we don't treat it as a separator here
    std::string key { Input.getToken<std::string>( true, "\n\r\t  ;[]" ) };
    // key can be an actual key or block end
    if( ( true == key.empty() ) || ( key == "}" ) ) { return false; }

    if( Priority != -1 ) {
        // regular attribute processing mode
        if( key == Global.Weather ) {
            // weather textures override generic (pri 0) and seasonal (pri 1) textures
            // seasonal weather textures (pri 1+2=3) override generic weather (pri 2) textures
            // skip the opening bracket
            auto const value { Input.getToken<std::string>( true, "\n\r\t ;" ) };
            while( true == deserialize_mapping( Input, Priority + 2, Loadnow ) ) {
                ; // all work is done in the header
            }
        }
        else if( key == Global.Season ) {
            // seasonal textures override generic textures
            // skip the opening bracket
            auto const value { Input.getToken<std::string>( true, "\n\r\t ;" ) };
            while( true == deserialize_mapping( Input, Priority + 1, Loadnow ) ) {
                ; // all work is done in the header
            }
        }

        else if (key.compare(0, 7, "texture") == 0) {
            key.erase(0, 7);

            std::string value = deserialize_random_set( Input );
            std::replace(value.begin(), value.end(), '\\', '/');
            auto it = parse_info->tex_mapping.find(key);
            if (it == parse_info->tex_mapping.end())
                parse_info->tex_mapping.emplace(std::make_pair(key, parse_info_s::tex_def({ value, Priority })));
            else if (Priority > it->second.priority)
            {
                parse_info->tex_mapping.erase(it);
                parse_info->tex_mapping.emplace(std::make_pair(key, parse_info_s::tex_def({ value, Priority })));
            }
        }
        else if (key.compare(0, 5, "param") == 0) {
            key.erase(0, 5);

            std::string value = Input.getToken<std::string>( true, "\n\r\t;" );
            std::istringstream stream(value);
            glm::vec4 data;
            stream >> data.r;
            stream >> data.g;
            stream >> data.b;
            stream >> data.a;

            auto it = parse_info->param_mapping.find(key);
            if (it == parse_info->param_mapping.end())
                parse_info->param_mapping.emplace(std::make_pair(key, parse_info_s::param_def({ data, Priority })));
            else if (Priority > it->second.priority)
            {
                parse_info->param_mapping.erase(it);
                parse_info->param_mapping.emplace(std::make_pair(key, parse_info_s::param_def({ data, Priority })));
            }
        }
        else if (key == "shader:" &&
                (!shader || Priority > m_shader_priority))
        {
            try
            {
                std::string value = deserialize_random_set( Input );
                shader = GfxRenderer.Fetch_Shader(value);
                m_shader_priority = Priority;
            }
            catch (gl::shader_exception const &e)
            {
                log_error("invalid shader: " + std::string(e.what()));
            }
        }
        else if (key == "opacity:" &&
                Priority > m_opacity_priority)
        {
            std::string value = deserialize_random_set( Input );
            opacity = std::stof(value); //m7t: handle exception
            m_opacity_priority = Priority;
        }
        else if (key == "selfillum:" &&
                Priority > m_selfillum_priority)
        {
            std::string value = deserialize_random_set( Input );
            selfillum = std::stof(value); //m7t: handle exception
            m_selfillum_priority = Priority;
        }
        else if( key == "size:" ) {
            Input.getTokens( 2 );
            Input
                >> size.x
                >> size.y;
        }
        else {
            auto const value = Input.getToken<std::string>( true, "\n\r\t ;" );
            if( value == "{" ) {
                // unrecognized or ignored token, but comes with attribute block and potential further nesting
                // go through it and discard the content
                while( true == deserialize_mapping( Input, -1, Loadnow ) ) {
                    ; // all work is done in the header
                }
            }
        }
    }
    else {
        // discard mode; ignores all retrieved tokens
        auto const value { Input.getToken<std::string>( true, "\n\r\t ;" ) };
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

float opengl_material::get_or_guess_opacity()
{
    if (!std::isnan(opacity))
        return opacity;

    if (textures[0] != null_handle)
    {
        opengl_texture &tex = GfxRenderer.Texture(textures[0]);
        if (tex.has_alpha)
            return 0.0f;
        else
            return 0.5f;
    }

    return 0.0f;
}

// create material object from data stored in specified file.
// NOTE: the deferred load parameter is passed to textures defined by material, the material itself is always loaded immediately
material_handle
material_manager::create( std::string const &Filename, bool const Loadnow ) {

    auto filename { Filename };

    if( filename.find( '|' ) != std::string::npos )
        filename.erase( filename.find( '|' ) ); // po | może być nazwa kolejnej tekstury

    // discern references to textures generated by a script
    // TBD: support file: for file resources?
    auto const isgenerated { filename.find( "make:" ) == 0 };

    // process supplied resource name
    if( isgenerated ) {
        // generated resource
        // scheme:(user@)path?query

        // remove scheme indicator
        filename.erase( 0, filename.find(':') + 1 );
        // TBD, TODO: allow shader specification as part of the query?
        erase_leading_slashes( filename );
    }
    else {
        // regular file resource
        // (filepath/)filename.extension

        erase_extension( filename );
        replace_slashes( filename );
        erase_leading_slashes( filename );
    }

    auto const databanklookup { find_in_databank( ToLower( filename ) ) };
    if( databanklookup != null_handle ) {
        return databanklookup;
    }

    opengl_material material;
    material_handle materialhandle { null_handle };

    auto const locator {
        isgenerated ?
            std::make_pair( filename, "make:" ) :
            find_on_disk( filename ) };

    if( ( false == isgenerated )
     && ( false == locator.first.empty() ) ) {
        // try to parse located file resource
        cParser materialparser( locator.first + locator.second, cParser::buffer_FILE );
        if( true == material.deserialize( materialparser, Loadnow ) ) {
            material.name = locator.first;
        }
    }
    else {
        // if there's no .mat file, this can be either autogenerated texture,
        // or legacy method of referring just to diffuse texture directly.
        // wrap basic material around it in either case
        material.textures[0] = GfxRenderer.Fetch_Texture( Filename, Loadnow );
		if( material.textures[0] != null_handle )
		{
			// use texture path and name to tell the newly created materials apart
			material.name = GfxRenderer.Texture( material.textures[0] ).name;

            // material would attach default shader anyway, but it would spit to error log
	        try
	        {
	            material.shader = GfxRenderer.Fetch_Shader("default_1");
	        }
	        catch (gl::shader_exception const &e)
	        {
	            ErrorLog("invalid shader: " + std::string(e.what()));
	        }
        }
    }

	if( false == material.name.empty() ) {
		// if we have material name and shader it means resource was processed succesfully
		material.finalize(Loadnow);
        materialhandle = m_materials.size();
        m_materialmappings.emplace( material.name, materialhandle );
		m_materials.emplace_back( std::move(material) );
    }
    else {
        // otherwise record our failure to process the resource, to speed up subsequent attempts
        m_materialmappings.emplace( filename, materialhandle );
    }

    return materialhandle;
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

    auto const materialname { ToLower( Materialname ) };
    return (
        FileExists(
            { Global.asCurrentTexturePath + materialname, materialname, szTexturePath + materialname },
            { ".mat" } ) );
}

//---------------------------------------------------------------------------
