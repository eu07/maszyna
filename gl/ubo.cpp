#include "stdafx.h"
#include "ubo.h"

gl::ubo::ubo(int size, int index)
{
    glGenBuffers(1, *this);
    bind();
    glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_STREAM_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, index, *this);
}

gl::ubo::~ubo()
{
    glDeleteBuffers(1, *this);
}

void gl::ubo::bind(GLuint i)
{
    glBindBuffer(GL_UNIFORM_BUFFER, i);
}

void gl::ubo::update(const uint8_t *data, int offset, int size)
{
    bind();
    glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
}
