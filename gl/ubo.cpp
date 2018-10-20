#include "stdafx.h"
#include "ubo.h"

gl::ubo::ubo(int size, int idx, GLenum hint)
{
    allocate(buffer::UNIFORM_BUFFER, size, hint);
    index = idx;
    bind_uniform();
}

void gl::ubo::bind_uniform()
{
    bind_base(buffer::UNIFORM_BUFFER, index);
}

void gl::ubo::update(const uint8_t *data, int offset, int size)
{
    upload(buffer::UNIFORM_BUFFER, data, offset, size);
}
