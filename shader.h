/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once
#include "Float3d.h"

class gl_shader
{
	GLuint id = 0;
public:
	gl_shader();
	gl_shader(std::string);
	~gl_shader();

	operator GLuint();
};

class gl_program
{
protected:
	GLuint id = 0;

public:
	gl_program() = default;
	gl_program(std::vector<gl_shader>);
	~gl_program();

	static gl_program* current_program;
	static gl_program* last_program;
	void bind();
	static void bind_last();
	static void unbind();

	operator GLuint();
};

class gl_program_mvp : public gl_program
{
	GLuint mv_uniform;
	GLint mvn_uniform;
	GLuint p_uniform;

public:
	gl_program_mvp() = default;
	gl_program_mvp(std::vector<gl_shader>);

	void set_mv(const glm::mat4 &);
	void set_p(const glm::mat4 &);
	void copy_gl_mvp();
};

// layout std140
// structs must match with GLSL
// weird order to minimize padding

#define PAD(x) uint64_t : x * 4; uint64_t : x * 4;
#pragma pack(push, 1)
struct gl_ubodata_light
{
	enum type_e
	{
		SPOT = 0,
		POINT,
		DIR
	};

	glm::vec3 pos;
	type_e type;

	glm::vec3 dir;
	float in_cutoff;

	glm::vec3 color;
	float out_cutoff;

	float linear;
	float quadratic;

	PAD(8);
};

static_assert(sizeof(gl_ubodata_light) == 64, "bad size of ubo structs");

struct gl_ubodata_light_params
{
	static const size_t MAX_LIGHTS = 8;

	glm::vec3 ambient;
	float fog_density;

	glm::vec3 fog_color;
	GLuint lights_count;

	gl_ubodata_light lights[MAX_LIGHTS];
};

static_assert(sizeof(gl_ubodata_light_params) == 544, "bad size of ubo structs");

#pragma pack(pop)

template<typename T>
class gl_ubo
{
	GLuint buffer_id = 0;
	GLuint binding_point;
	static GLuint binding_point_cnt;

public:
	T data;
	void init();
	~gl_ubo();
	void update();
	GLuint get_binding_point()
	{
		return binding_point;
	};
};

class gl_program_light : public gl_program_mvp
{
public:
	gl_program_light() = default;
	gl_program_light(std::vector<gl_shader>);

	void set_lightview(const glm::mat4 &lightview);
	void set_material(float specular, const glm::vec3 &emission);
	void bind_ubodata(GLuint point);

private:
	GLuint lightview_uniform;
	GLuint specular_uniform;
	GLuint emission_uniform;
};
