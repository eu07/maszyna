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
    glFramebufferTexture2D(GL_FRAMEBUFFER, location, GL_TEXTURE_2D, tex.id, 0);
}

void gl::framebuffer::attach(const renderbuffer &rb, GLenum location)
{
    bind();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, location, GL_RENDERBUFFER, *rb);
}

bool gl::framebuffer::is_complete()
{
    bind();
    return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

void gl::framebuffer::clear()
{
    bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
