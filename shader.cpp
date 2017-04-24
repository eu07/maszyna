/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx.h"
#include "shader.h"
#include "Float3d.h"
#include "Logs.h"

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

gl_program::operator GLuint()
{
	return id;
}

gl_program_mvp::gl_program_mvp(std::vector<gl_shader> v) : gl_program(v)
{
	mv_uniform = glGetUniformLocation(id, "modelview");
	p_uniform = glGetUniformLocation(id, "projection");
}

void gl_program_mvp::copy_gl_mvp()
{
	float4x4 mv, p;
	glGetFloatv(GL_MODELVIEW_MATRIX, mv.e);
	glGetFloatv(GL_PROJECTION_MATRIX, p.e);

	glUniformMatrix4fv(mv_uniform, 1, GL_FALSE, mv.e);
	glUniformMatrix4fv(p_uniform, 1, GL_FALSE, p.e);
}

void gl_program_mvp::set_mv(const glm::mat4 &m)
{
	glUniformMatrix4fv(mv_uniform, 1, GL_FALSE, glm::value_ptr(m));
}

void gl_program_mvp::set_p(const glm::mat4 &m)
{
	glUniformMatrix4fv(p_uniform, 1, GL_FALSE, glm::value_ptr(m));
}

gl_program_light::gl_program_light(std::vector<gl_shader> v) : gl_program_mvp(v)
{
	ambient_uniform = glGetUniformLocation(id, "ambient");
	lcount_uniform = glGetUniformLocation(id, "lights_count");

	for (size_t i = 0; i < MAX_LIGHTS; i++)
	{
		lights_uniform[i].type = glGetUniformLocation(id, std::string("lights[" + std::to_string(i) + "].type").c_str());
		lights_uniform[i].pos = glGetUniformLocation(id, std::string("lights[" + std::to_string(i) + "].pos").c_str());
		lights_uniform[i].dir = glGetUniformLocation(id, std::string("lights[" + std::to_string(i) + "].dir").c_str());
		lights_uniform[i].in_cutoff = glGetUniformLocation(id, std::string("lights[" + std::to_string(i) + "].in_cutoff").c_str());
		lights_uniform[i].out_cutoff = glGetUniformLocation(id, std::string("lights[" + std::to_string(i) + "].out_cutoff").c_str());
		lights_uniform[i].color = glGetUniformLocation(id, std::string("lights[" + std::to_string(i) + "].color").c_str());
		lights_uniform[i].linear = glGetUniformLocation(id, std::string("lights[" + std::to_string(i) + "].linear").c_str());
		lights_uniform[i].quadratic = glGetUniformLocation(id, std::string("lights[" + std::to_string(i) + "].quadratic").c_str());
	}

	glUseProgram(id);
	glUniform3f(ambient_uniform, 1.0f, 1.0f, 1.0f);
	glUniform1i(lcount_uniform, 0);
}

void gl_program_light::set_ambient(glm::vec3 &ambient)
{
	glUseProgram(id);
	glUniform3fv(ambient_uniform, 1, glm::value_ptr(ambient));
}

void gl_program_light::set_light_count(GLuint count)
{
	glUseProgram(id);
	glUniform1ui(lcount_uniform, count);
}

void gl_program_light::set_light(GLuint i, type t, glm::vec3 &pos, glm::vec3 &dir,
	float in_cutoff, float out_cutoff,
	glm::vec3 &color, float linear, float quadratic)
{
	glm::mat4 mv = OpenGLMatrices.data(GL_MODELVIEW);

	glm::vec3 trans_pos = mv * glm::vec4(pos.x, pos.y, pos.z, 1.0f);
	glm::vec3 trans_dir = mv * glm::vec4(dir.x, dir.y, dir.z, 0.0f);

	glUseProgram(id);
	glUniform1ui(lights_uniform[i].type, (GLuint)t);
	glUniform3fv(lights_uniform[i].pos, 1, glm::value_ptr(trans_pos));
	glUniform3fv(lights_uniform[i].dir, 1, glm::value_ptr(trans_dir));
	glUniform1f(lights_uniform[i].in_cutoff, in_cutoff);
	glUniform1f(lights_uniform[i].out_cutoff, out_cutoff);
	glUniform3fv(lights_uniform[i].color, 1, glm::value_ptr(color));
	glUniform1f(lights_uniform[i].linear, linear);
	glUniform1f(lights_uniform[i].quadratic, quadratic);
}