#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

#include "object.h"
#include "bindable.h"

namespace gl
{
    class shader_exception : public std::runtime_error
    {
        using runtime_error::runtime_error;
    };

    class shader : public object
    {
    public:
#ifdef SHADERVALIDATOR_STANDALONE
        shader() = default;
#endif
        shader(const std::string &filename);
        ~shader();

        enum class components_e
        {
            R = GL_RED,
            RG = GL_RG,
            RGB = GL_RGB,
            RGBA = GL_RGBA,
            sRGB = GL_SRGB,
            sRGB_A = GL_SRGB_ALPHA
        };

        struct texture_entry
        {
            size_t id;
            components_e components;
        };

        enum class defaultparam_e
        {
            required,
            nan,
            zero,
            one,
            ambient,
            diffuse,
            specular,
            glossiness
        };

        struct param_entry
        {
            size_t location;
            size_t offset;
            size_t size;
            defaultparam_e defaultparam;
        };

        std::unordered_map<std::string, texture_entry> texture_conf;
        std::unordered_map<std::string, param_entry> param_conf;
        std::string name;

#ifndef SHADERVALIDATOR_STANDALONE
    private:
#endif
        std::pair<GLuint, std::string> process_source(const std::string &filename, const std::string &basedir);

        void expand_includes(std::string &str, const std::string &basedir);
        void parse_texture_entries(std::string &str);
        void parse_param_entries(std::string &str);

        std::string read_file(const std::string &filename);

        static std::unordered_map<std::string, components_e> components_mapping;
        static std::unordered_map<std::string, defaultparam_e> defaultparams_mapping;

        void log_error(const std::string &str);
    };

    class program : public object, public bindable<program>
    {
    public:
        program();
        program(std::vector<std::reference_wrapper<const gl::shader>>);
        ~program();

        using bindable::bind;
        static void bind(GLuint i);

        void attach(const shader &);
        void link();

        std::unordered_map<std::string, shader::texture_entry> texture_conf;
        std::unordered_map<std::string, shader::param_entry> param_conf;

	private:
        void init();
	};
}
