#include "stdafx.h"
#include "buffer.h"

GLenum gl::buffer::glenum_target(gl::buffer::targets target)
{
	static GLenum mapping[13] =
    {
        GL_ARRAY_BUFFER,
        GL_ATOMIC_COUNTER_BUFFER,
        GL_COPY_READ_BUFFER,
        GL_COPY_WRITE_BUFFER,
        GL_DISPATCH_INDIRECT_BUFFER,
        GL_DRAW_INDIRECT_BUFFER,
        GL_ELEMENT_ARRAY_BUFFER,
        GL_PIXEL_PACK_BUFFER,
        GL_PIXEL_UNPACK_BUFFER,
        GL_SHADER_STORAGE_BUFFER,
        GL_TEXTURE_BUFFER,
        GL_TRANSFORM_FEEDBACK_BUFFER,
        GL_UNIFORM_BUFFER
    };
    return mapping[target];
}

void gl::buffer::bind(targets target)
{
    if (binding_points[target] == *this)
        return;

    glBindBuffer(glenum_target(target), *this);
    binding_points[target] = *this;
}

void gl::buffer::bind_base(targets target, GLuint index)
{
    glBindBufferBase(glenum_target(target), index, *this);
    binding_points[target] = *this;
}

void gl::buffer::unbind(targets target)
{
    if( binding_points[ target ] == 0 ) { return; }

    glBindBuffer(glenum_target(target), 0);
    binding_points[target] = 0;
}

void gl::buffer::unbind()
{
	for (size_t i = 0; i < sizeof(binding_points) / sizeof(GLuint); i++)
		unbind((targets)i);
}

gl::buffer::buffer()
{
    glGenBuffers(1, *this);
}

gl::buffer::~buffer()
{
    glDeleteBuffers(1, *this);
}

void gl::buffer::allocate(targets target, int size, GLenum hint)
{
    bind(target);
    glBufferData(glenum_target(target), size, nullptr, hint);
}

void gl::buffer::upload(targets target, const void *data, int offset, int size)
{
    bind(target);
    glBufferSubData(glenum_target(target), offset, size, data);
}

void gl::buffer::download(targets target, void *data, int offset, int size)
{
    bind(target);
    if (GLAD_GL_VERSION_3_3)
    {
        glGetBufferSubData(glenum_target(target), offset, size, data);
    }
    else
    {
        void *glbuf = glMapBufferRange(glenum_target(target), offset, size, GL_MAP_READ_BIT);
        memcpy(data, glbuf, size);
        glUnmapBuffer(glenum_target(target));
    }
}

thread_local GLuint gl::buffer::binding_points[13];
