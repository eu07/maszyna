#pragma once

#include <string>
#include <vector>
#include <functional>

#include "object.h"
#include "bindable.h"

namespace gl
{
    class shader : public object
    {
    public:
        shader(const std::string &filename);
        ~shader();

        enum class components_e
        {
            R,
            RG,
            RGB,
            RGBA,
            sRGB,
            sRGB_A
        };

        struct texture_entry
        {
            std::string name;
            size_t id;
            components_e components;
        };

        std::vector<texture_entry> texture_conf;

    private:
        void expand_includes(std::string &str);
        void parse_config(std::string &str);
        std::string read_file(const std::string &filename);

        static std::unordered_map<std::string, components_e> components_mapping;
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

        std::vector<shader::texture_entry> texture_conf;

	private:
        void init();
	};
}
