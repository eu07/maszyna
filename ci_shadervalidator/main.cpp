#include "stdafx.h"
#include "../gl/shader.h"
#include "../gl/glsl_common.h"
#include "Globals.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <filesystem>

_global Global;

bool check_shader(std::string filename)
{
	gl::shader shader_processor;

	gl::glsl_common_setup();
	std::pair<GLuint, std::string> source = shader_processor.process_source(filename, "../shaders/");

	std::string stage;
	if (source.first == GL_VERTEX_SHADER)
		stage = "vert";
	else if (source.first == GL_FRAGMENT_SHADER)
		stage = "frag";
	else if (source.first == GL_GEOMETRY_SHADER)
		stage = "geom";
	else
		return false;

	std::ofstream file("/tmp/shader");
	file << source.second;
	file.close();

	int ret = system(("glslangValidator -S " + stage + " /tmp/shader").c_str());

	return ret == 0;
}

bool check_shader_allopts(std::string filename)
{
	bool suc = true;

	std::cout << "features on..." << std::flush;
	Global.gfx_shadowmap_enabled = true;	
	Global.gfx_envmap_enabled = true;
	Global.gfx_postfx_motionblur_enabled = true;
	Global.gfx_skippipeline = false;
	Global.gfx_extraeffects = true;
	Global.gfx_shadergamma = true;
	if (!check_shader(filename))
		suc = false;

	std::cout << "done, features off..." << std::flush;
	Global.gfx_shadowmap_enabled = false;
	Global.gfx_envmap_enabled = false;
	Global.gfx_postfx_motionblur_enabled = false;
	Global.gfx_skippipeline = true;
	Global.gfx_extraeffects = false;
	Global.gfx_shadergamma = false;
	if (!check_shader(filename))
		suc = false;

	std::cout << "done" << std::flush;

	return suc;
}


bool check_shader_confs(std::string filename)
{
	bool suc = true;

	std::cout << "desktop GL: " << std::flush;
	Global.gfx_usegles = false;
	if (!check_shader_allopts(filename))
		suc = false;

	std::cout << "; GLES: " << std::flush;
	GLAD_GL_ES_VERSION_3_1 = 1;
	Global.gfx_usegles = true;
	if (!check_shader_allopts(filename))
		suc = false;

	std::cout << std::endl;

	return suc;
}

int main()
{
	bool suc = true;
	for (auto &f : std::filesystem::recursive_directory_iterator("../shaders/")) {
		if (!f.is_regular_file())
			continue;

		std::string relative = std::filesystem::relative(f.path(), "../shaders/").string();
		if (f.path().extension() != ".frag" && f.path().extension() != ".geom" && f.path().extension() != ".vert")
			continue;

		std::cout << "== validating shader " << relative << ": " << std::flush;
		if (!check_shader_confs(relative))
			suc = false;
	}

	if (suc)
		std::cout << "==== all checks ok" << std::endl;
	else {
		std::cout << "==== checks failed" << std::endl;
		return 1;
	}

	return 0;
}
