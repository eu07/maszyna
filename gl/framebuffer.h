#pragma once

#include "object.h"
#include "bindable.h"
#include "renderbuffer.h"
#include "Texture.h"
#include "cubemap.h"

namespace gl
{
    class framebuffer : public object, public bindable<framebuffer>
    {
    public:
        framebuffer();
        ~framebuffer();

        void attach(const opengl_texture &tex, GLenum location);
        void attach( const opengl_texture &tex, GLenum location, GLint layer );
        void attach(const cubemap &tex, int face, GLenum location);
        void attach(const renderbuffer &rb, GLenum location);
        void setup_drawing(int attachments);
        void detach(GLenum location);
        void clear(GLbitfield mask);

        bool is_complete();
        void blit_to(framebuffer *other, int w, int h, GLbitfield mask, GLenum attachment);
        void blit_from(framebuffer *other, int w, int h, GLbitfield mask, GLenum attachment);

        static void blit(framebuffer *src, framebuffer *dst, int sx, int sy, int w, int h, GLbitfield mask, GLenum attachment);

        using bindable::bind;
        static void bind(GLuint id);
    };
}
