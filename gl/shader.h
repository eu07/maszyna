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

    private:
        void expand_includes(std::string &str);
        std::string read_file(const std::string &filename);
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

        virtual void init();
	};
}
