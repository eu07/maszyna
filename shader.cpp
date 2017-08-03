/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx.h"
#include "shader.h"
#include "Float3d.h"
#include "Logs.h"
#include "openglmatrixstack.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

inline bool strcend(std::string const &value, std::string const &ending)
{
	if (ending.size() > value.size())
		return false;
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

gl_shader::gl_shader(std::string filename)
{
	WriteLog("loading shader " + filename + " ..", false);

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

	WriteLog("done.");
}

gl_shader::operator GLuint()
{
	return id;
}

gl_shader::~gl_shader()
{
	//glDeleteShader(*this); //m7todo: something is broken
}

gl_program::gl_program(std::vector<gl_shader> shaders)
{
	id = glCreateProgram();
	for (auto s : shaders)
		glAttachShader(id, s);
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

gl_program* gl_program::current_program = nullptr;
gl_program* gl_program::last_program = nullptr;

void gl_program::bind()
{
	if (current_program == this)
		return;
	last_program = current_program;
	current_program = this;
	glUseProgram(*current_program);
}

void gl_program::bind_last()
{
	current_program = last_program;
	if (current_program != nullptr)
		glUseProgram(*current_program);
	else
		glUseProgram(0);
}

void gl_program::unbind()
{
	last_program = current_program;
	current_program = nullptr;
	glUseProgram(0);
}

gl_program::operator GLuint()
{
	return id;
}

gl_program::~gl_program()
{
	//glDeleteProgram(*this); //m7todo: something is broken
}

gl_program_mvp::gl_program_mvp(std::vector<gl_shader> v) : gl_program(v)
{
	mv_uniform = glGetUniformLocation(id, "modelview");
	mvn_uniform = glGetUniformLocation(id, "modelviewnormal");
	p_uniform = glGetUniformLocation(id, "projection");
}

void gl_program_mvp::copy_gl_mvp()
{
	set_mv(OpenGLMatrices.data(GL_MODELVIEW));
	set_p(OpenGLMatrices.data(GL_PROJECTION));
}

void gl_program_mvp::set_mv(const glm::mat4 &m)
{
	if (m != mv)
	{
		mv = m;
		mv_dirty = true;
	}
}

void gl_program_mvp::set_p(const glm::mat4 &m)
{
	if (m != p)
	{
		p = m;
		p_dirty = true;
	}
}

void gl_program_mvp::update()
{
	if (current_program != this)
		return;

	if (mv_dirty)
	{
		if (mvn_uniform != -1)
		{
			glm::mat3 mvn = glm::mat3(glm::transpose(glm::inverse(mv)));
			glUniformMatrix3fv(mvn_uniform, 1, GL_FALSE, glm::value_ptr(mvn));
		}
		glUniformMatrix4fv(mv_uniform, 1, GL_FALSE, glm::value_ptr(mv));
		mv_dirty = false;
	}

	if (p_dirty)
	{
		glUniformMatrix4fv(p_uniform, 1, GL_FALSE, glm::value_ptr(p));
		mv_dirty = false;
	}
}

gl_program_light::gl_program_light(std::vector<gl_shader> v) : gl_program_mvp(v)
{
	bind();

	glUniform1i(glGetUniformLocation(id, "tex"), 0);
	glUniform1i(glGetUniformLocation(id, "shadowmap"), 1);
	lightview_uniform = glGetUniformLocation(id, "lightview");
	emission_uniform = glGetUniformLocation(id, "emission");
	specular_uniform = glGetUniformLocation(id, "specular");
	color_uniform = glGetUniformLocation(id, "color");
}

void gl_program_light::bind_ubodata(GLuint point)
{
	GLuint index = glGetUniformBlockIndex(*this, "ubodata");
	glUniformBlockBinding(*this, index, point);
}

void gl_program_light::set_lightview(const glm::mat4 &lightview)
{
	if (current_program != this)
		return;

	glUniformMatrix4fv(lightview_uniform, 1, GL_FALSE, glm::value_ptr(lightview));
}

void gl_program_light::set_material(const material_s &mat)
{
	if (current_program != this)
		return;

	if (std::memcmp(&material, &mat, sizeof(material_s)))
	{
		material_dirty = true;
		material = mat;
	}
}

void gl_program_light::update()
{
	if (current_program != this)
		return;

	gl_program_mvp::update();

	if (material_dirty)
	{
		glUniform1f(specular_uniform, material.specular);
		glUniform3fv(emission_uniform, 1, glm::value_ptr(material.emission));
		if (color_uniform != -1)
			glUniform4fv(color_uniform, 1, glm::value_ptr(material.color));

		material_dirty = false;
	}
}

template<typename T>
GLuint gl_ubo<T>::binding_point_cnt = 0;

template<typename T>
void gl_ubo<T>::init()
{
	glGenBuffers(1, &buffer_id);
	glBindBuffer(GL_UNIFORM_BUFFER, buffer_id);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(T), nullptr, GL_DYNAMIC_DRAW);
	binding_point = binding_point_cnt++;
	glBindBufferBase(GL_UNIFORM_BUFFER, binding_point, buffer_id);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

template<typename T>
void gl_ubo<T>::update()
{
	glBindBuffer(GL_UNIFORM_BUFFER, buffer_id);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(T), &data);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

template<typename T>
gl_ubo<T>::~gl_ubo()
{
	if (buffer_id)
		glDeleteBuffers(1, &buffer_id);
}

template class gl_ubo<gl_ubodata_light_params>;