#include "stdafx.h"
#include "vao.h"

bool gl::vao::use_vao = true;

gl::vao::vao()
{
	if (!use_vao)
		return;

	glGenVertexArrays(1, *this);
}

gl::vao::~vao()
{
	if (!use_vao)
		return;

	glDeleteVertexArrays(1, *this);
}

void gl::vao::setup_attrib(gl::buffer &buffer, int attrib, int size, int type, int stride, int offset)
{
	if (use_vao) {
		bind();
		buffer.bind(buffer::ARRAY_BUFFER);
		glVertexAttribPointer(attrib, size, type, GL_FALSE, stride, reinterpret_cast<void*>(offset));
		glEnableVertexAttribArray(attrib);
	}
	else {
		params.push_back({buffer, attrib, size, type, stride, offset});
		active = nullptr;
	}
}

void gl::vao::setup_ebo(buffer &e)
{
	if (use_vao) {
		bind();
		e.bind(buffer::ELEMENT_ARRAY_BUFFER);
	}
	else {
		ebo = &e;
		active = nullptr;
	}
}

void gl::vao::bind()
{
	if (active == this)
		return;
	active = this;

	if (use_vao) {
		glBindVertexArray(*this);
	}
	else {
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
}

void gl::vao::unbind()
{
	active = nullptr;
}
