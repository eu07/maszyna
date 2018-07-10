#pragma once

#include "object.h"
#include "bindable.h"

namespace gl
{
    class renderbuffer : public object, public bindable<renderbuffer>
    {
    public:
        renderbuffer();
        ~renderbuffer();

        void alloc(GLuint format, int width, int height);

        static void bind(GLuint id);
        using bindable::bind;
    };
}
