#include "stdafx.h"

#include <fstream>
#include <sstream>
#include "shader.h"
#include "glsl_common.h"

inline bool strcend(std::string const &value, std::string const &ending)
{
    if (ending.size() > value.size())
        return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

std::string gl::shader::read_file(const std::string &filename)
{
    std::stringstream stream;
    std::ifstream f;
    f.exceptions(std::ifstream::badbit);

    f.open("shaders/" + filename);
    stream << f.rdbuf();
    f.close();

    std::string str = stream.str();

    return str;
}

void gl::shader::expand_includes(std::string &str)
{
    size_t start_pos = 0;

    std::string magic = "#include";
    while ((start_pos = str.find(magic, start_pos)) != str.npos)
    {
        size_t fp = str.find('<', start_pos);
        size_t fe = str.find('>', start_pos);
        if (fp == str.npos || fe == str.npos)
            return;

        std::string filename = str.substr(fp + 1, fe - fp - 1);
        std::string content;
        if (filename != "common")
            content = read_file(filename);
        else
            content = glsl_common;

        str.replace(start_pos, fe - start_pos + 1, content);
    }
}

gl::shader::shader(const std::string &filename)
{
    std::string str = read_file(filename);
    expand_includes(str);

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

    **this = glCreateShader(type);
    glShaderSource(*this, 1, &cstr, 0);
    glCompileShader(*this);

    GLint status;
    glGetShaderiv(*this, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        GLchar info[512];
        glGetShaderInfoLog(*this, 512, 0, info);
        throw std::runtime_error("failed to compile " + filename + ": " + std::string(info));
    }

}

gl::shader::~shader()
{
    glDeleteShader(*this);
}

void gl::program::init()
{
    bind();

    int i = 0;
    GLuint loc;
    while (true)
    {
        std::string name = "tex" + std::to_string(i + 1);
        loc = glGetUniformLocation(*this, name.c_str());
        if (loc != -1)
            glUniform1i(loc, i);
        else
            break;
        i++;
    }

    //tbd: do something better

    glUniform1i(glGetUniformLocation(*this, "shadowmap"), MAX_TEXTURES + 0);
    glUniform1i(glGetUniformLocation(*this, "envmap"), MAX_TEXTURES + 1);

	GLuint index;

	if ((index = glGetUniformBlockIndex(*this, "scene_ubo")) != GL_INVALID_INDEX)
		glUniformBlockBinding(*this, 0, index);

	if ((index = glGetUniformBlockIndex(*this, "model_ubo")) != GL_INVALID_INDEX)
		glUniformBlockBinding(*this, 1, index);

	if ((index = glGetUniformBlockIndex(*this, "light_ubo")) != GL_INVALID_INDEX)
		glUniformBlockBinding(*this, 2, index);
}

gl::program::program()
{
    **this = glCreateProgram();
}

gl::program::program(std::vector<std::reference_wrapper<const gl::shader>> shaders) : program()
{
    for (const gl::shader &s : shaders)
        attach(s);
    link();
}

void gl::program::attach(const gl::shader &s)
{
    glAttachShader(*this, *s);
}

void gl::program::link()
{
    glLinkProgram(*this);

    GLint status;
    glGetProgramiv(*this, GL_LINK_STATUS, &status);
    if (!status)
    {
        GLchar info[512];
        glGetProgramInfoLog(*this, 512, 0, info);
        throw std::runtime_error("failed to link program: " + std::string(info));
    }

    init();
}

gl::program::~program()
{
    glDeleteProgram(*this);
}

void gl::program::bind(GLuint i)
{
    glUseProgram(i);
}
