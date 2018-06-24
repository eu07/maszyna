#pragma once

#include "object.h"
#include "bindable.h"

namespace gl
{
    class vao : public object, public bindable<vao>
    {
    public:
        vao();
        ~vao();

        void setup_attrib(int attrib, int size, int type, int stride, int offset);

        using bindable::bind;
        static void bind(GLuint i);
    };
}
