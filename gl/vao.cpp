#include "stdafx.h"
#include "vao.h"

gl::vao::vao()
{
    glGenVertexArrays(1, *this);
}

gl::vao::~vao()
{
    glDeleteVertexArrays(1, *this);
}

void gl::vao::setup_attrib(int attrib, int size, int type, int stride, int offset)
{
    bind();
    glVertexAttribPointer(attrib, size, type, GL_FALSE, stride, reinterpret_cast<void*>(offset));
    glEnableVertexAttribArray(attrib);
}

void gl::vao::bind(GLuint i)
{
    glBindVertexArray(i);
}
