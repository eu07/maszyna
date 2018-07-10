#pragma once

#include "object.h"
#include "bindable.h"
#include "renderbuffer.h"
#include "Texture.h"

namespace gl
{
    class framebuffer : public object, public bindable<framebuffer>
    {
    public:
        framebuffer();
        ~framebuffer();

        void attach(const opengl_texture &tex, GLenum location);
        void attach(const renderbuffer &rb, GLenum location);
        void clear();

        bool is_complete();

        using bindable::bind;
        static void bind(GLuint id);
    };
}
