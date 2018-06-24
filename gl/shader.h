#pragma once

#include <string>
#include <vector>
#include <functional>

#include "object.h"

namespace gl
{
    class shader : public object
    {
    public:
        shader(const std::string &filename);
        ~shader();
    };

    class program : public object
    {
    public:
        program();
        program(std::vector<std::reference_wrapper<const gl::shader>>);
        ~program();

        void bind();

        void attach(const shader &);
        void link();

        virtual void init();
	};
}
