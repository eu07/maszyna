#include "stdafx.h"
#include "framebuffer.h"

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
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    return status == GL_FRAMEBUFFER_COMPLETE;
}

void gl::framebuffer::clear(GLbitfield mask)
{
    bind();
    glClear(mask);
}

void gl::framebuffer::blit_to(framebuffer &other, int w, int h, GLbitfield mask)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, *this);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, other);
	glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, mask, GL_NEAREST);
	unbind();
}

void gl::framebuffer::blit_from(framebuffer &other, int w, int h, GLbitfield mask)
{
	other.blit_to(*this, w, h, mask);
}
