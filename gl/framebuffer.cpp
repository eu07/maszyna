#include "stdafx.h"
#include "framebuffer.h"

#include "Logs.h"
#include "utilities.h"

gl::framebuffer::framebuffer()
{
    glGenFramebuffers(1, *this);
}

gl::framebuffer::~framebuffer()
{
    glDeleteFramebuffers(1, *this);
}

void gl::framebuffer::bind(GLuint id)
{
    glBindFramebuffer(GL_FRAMEBUFFER, id);
}

void gl::framebuffer::attach(const opengl_texture &tex, GLenum location)
{
    bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, location, tex.target, tex.id, 0);
}

void gl::framebuffer::attach(const opengl_texture &tex, GLenum location, GLint layer)
{
    bind();
    glFramebufferTextureLayer(GL_FRAMEBUFFER, location, tex.id, 0, layer);
}

void gl::framebuffer::attach(const cubemap &tex, int face, GLenum location)
{
    bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, location, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, *tex, 0);
}

void gl::framebuffer::attach(const renderbuffer &rb, GLenum location)
{
    bind();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, location, GL_RENDERBUFFER, *rb);
}

void gl::framebuffer::detach(GLenum location)
{
    bind();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, location, GL_RENDERBUFFER, 0);
}

bool gl::framebuffer::is_complete()
{
    bind();
    GLenum const status { glCheckFramebufferStatus( GL_FRAMEBUFFER ) };
    auto const iscomplete { status == GL_FRAMEBUFFER_COMPLETE };

    if( false == iscomplete ) {
        ErrorLog( "framebuffer status: error " + to_hex_str( status ) );
    }

    return iscomplete;
}

void gl::framebuffer::clear(GLbitfield mask)
{
    bind();
    if (mask & GL_DEPTH_BUFFER_BIT)
        glDepthMask(GL_TRUE);
    glClear(mask);
}

void gl::framebuffer::blit_to(framebuffer *other, int w, int h, GLbitfield mask, GLenum attachment)
{
    blit(this, other, 0, 0, w, h, mask, attachment);
}

void gl::framebuffer::blit_from(framebuffer *other, int w, int h, GLbitfield mask, GLenum attachment)
{
    blit(other, this, 0, 0, w, h, mask, attachment);
}

void gl::framebuffer::blit(framebuffer *src, framebuffer *dst, int sx, int sy, int w, int h, GLbitfield mask, GLenum attachment)
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, src ? *src : 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst ? *dst : 0);

    if (mask & GL_COLOR_BUFFER_BIT)
    {
        int attachment_n = attachment - GL_COLOR_ATTACHMENT0;

        {
            GLenum outputs[8] = { GL_NONE };
            outputs[attachment_n] = src != 0 ? attachment : GL_BACK_LEFT;

            glReadBuffer(attachment);
        }

        {
            GLenum outputs[8] = { GL_NONE };
            outputs[attachment_n] = dst != 0 ? attachment : GL_BACK_LEFT;

            glDrawBuffers(attachment_n + 1, outputs);
        }
    }

    glBlitFramebuffer(sx, sy, sx + w, sy + h, 0, 0, w, h, mask, GL_NEAREST);
    unbind();
}

void gl::framebuffer::setup_drawing(int attachments)
{
    bind();
    GLenum a[8] =
    {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3,
        GL_COLOR_ATTACHMENT4,
        GL_COLOR_ATTACHMENT5,
        GL_COLOR_ATTACHMENT6,
        GL_COLOR_ATTACHMENT7
    };
    glDrawBuffers(std::min(attachments, 8), &a[0]);
}
