/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once
#include "Float3d.h"

class gl_shader
{
	GLuint id;
public:
	gl_shader();
	gl_shader(std::string);

	operator GLuint();
};

class gl_program
{
protected:
	GLuint id;
public:
	gl_program() = default;
	gl_program(std::vector<gl_shader>);

	operator GLuint();
};

class gl_program_mvp : public gl_program
{
	GLuint mv_uniform;
	GLuint p_uniform;

public:
	gl_program_mvp() = default;
	gl_program_mvp(std::vector<gl_shader>);

	void copy_gl_mvp();
};

class gl_program_light : public gl_program_mvp
{
public:
	static const size_t MAX_LIGHTS = 8;

	enum type
	{
		SPOT = 0,
		POINT,
		DIR
	};

	gl_program_light() = default;
	gl_program_light(std::vector<gl_shader>);

	void set_ambient(float3 &ambient);
	void set_light_count(GLuint count);
	void set_light(GLuint id, type t, float3 &pos, float3 &dir, float in_cutoff, float out_cutoff,
	               float3 &color, float linear, float quadratic);

private:
	GLuint lcount_uniform;
	GLuint ambient_uniform;
	struct light_s
	{
		GLuint type;
		GLuint pos;
		GLuint dir;
		GLuint in_cutoff;
		GLuint out_cutoff;
		GLuint color;
		GLuint linear;
		GLuint quadratic;
	} lights_uniform[MAX_LIGHTS];
};