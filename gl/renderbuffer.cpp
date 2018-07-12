#include "stdafx.h"

#include "renderbuffer.h"

gl::renderbuffer::renderbuffer()
{
    glGenRenderbuffers(1, *this);
}

gl::renderbuffer::~renderbuffer()
{
    glDeleteRenderbuffers(1, *this);
}

void gl::renderbuffer::bind(GLuint id)
{
    glBindRenderbuffer(GL_RENDERBUFFER, id);
}

void gl::renderbuffer::alloc(GLuint format, int width, int height, int samples)
{
    bind();
	if (samples == 1)
		glRenderbufferStorage(GL_RENDERBUFFER, format, width, height);
	else
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, format, width, height);
}
