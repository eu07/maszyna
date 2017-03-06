/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx.h"
#include "shader.h"

inline bool strcend(std::string const &value, std::string const &ending)
{
	if (ending.size() > value.size())
		return false;
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

gl_shader::gl_shader(std::string filename)
{
	std::stringstream stream;
	std::ifstream f;
	f.exceptions(std::ifstream::badbit);

	f.open("shaders/" + filename);
	stream << f.rdbuf();
	f.close();

	std::string str = stream.str();
	const GLchar *cstr = str.c_str();

	if (!cstr[0])
		throw std::runtime_error("cannot read shader " + filename);

	GLuint type;
	if (strcend(filename, ".vert"))
		type = GL_VERTEX_SHADER;
	else if (strcend(filename, ".frag"))
		type = GL_FRAGMENT_SHADER;
	else
		throw std::runtime_error("unknown shader " + filename);

	id = glCreateShader(type);
	glShaderSource(id, 1, &cstr, 0);
	glCompileShader(id);

	GLint status;
	glGetShaderiv(id, GL_COMPILE_STATUS, &status);
	if (!status)
	{
		GLchar info[512];
		glGetShaderInfoLog(id, 512, 0, info);
		throw std::runtime_error("failed to compile " + filename + ": " + std::string(info));
	}
}

gl_program::gl_program(std::vector<gl_shader> shaders)
{
	id = glCreateProgram();
	for (auto s : shaders)
		glAttachShader(id, s.id);
	glLinkProgram(id);

	GLint status;
	glGetProgramiv(id, GL_LINK_STATUS, &status);
	if (!status)
	{
		GLchar info[512];
		glGetProgramInfoLog(id, 512, 0, info);
		throw std::runtime_error("failed to link program: " + std::string(info));
	}
}

void gl_program::use()
{
	glUseProgram(id);
}