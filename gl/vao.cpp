#include "stdafx.h"
#include "vao.h"

/*
gl::vao::vao()
{
	glGenVertexArrays(1, *this);
}

gl::vao::~vao()
{
	glDeleteVertexArrays(1, *this);
}

void gl::vao::setup_attrib(gl::buffer &buffer, int attrib, int size, int type, int stride, int offset)
{
	bind();
	buffer.bind(gl::buffer::ARRAY_BUFFER);
	glVertexAttribPointer(attrib, size, type, GL_FALSE, stride, reinterpret_cast<void*>(offset));
	glEnableVertexAttribArray(attrib);

}

void gl::vao::setup_ebo(buffer &ebo)
{
	bind();
	ebo.bind(gl::buffer::ELEMENT_ARRAY_BUFFER);
}

void gl::vao::bind(GLuint i)
{
	glBindVertexArray(i);
}
*/

void gl::vao::setup_attrib(gl::buffer &buffer, int attrib, int size, int type, int stride, int offset)
{
	params.push_back({buffer, attrib, size, type, stride, offset});
	active = nullptr;
}

void gl::vao::setup_ebo(buffer &e)
{
	ebo = &e;
	active = nullptr;
}

void gl::vao::bind()
{
	if (active == this)
		return;
	active = this;

	for (attrib_params &param : params) {
		param.buffer.bind(gl::buffer::ARRAY_BUFFER);
		glVertexAttribPointer(param.attrib, param.size, param.type, GL_FALSE, param.stride, reinterpret_cast<void*>(param.offset));
		glEnableVertexAttribArray(param.attrib);
	}

	for (size_t i = params.size(); i < 4; i++)
		glDisableVertexAttribArray(i);

	if (ebo)
		ebo->bind(gl::buffer::ELEMENT_ARRAY_BUFFER);
	else
		gl::buffer::unbind(gl::buffer::ELEMENT_ARRAY_BUFFER);
}

void gl::vao::unbind()
{
	active = nullptr;
}
