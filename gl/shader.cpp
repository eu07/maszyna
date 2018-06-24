#include "stdafx.h"

#include <fstream>
#include <sstream>
#include "shader.h"

inline bool strcend(std::string const &value, std::string const &ending)
{
    if (ending.size() > value.size())
        return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

gl::shader::shader(const std::string &filename)
{
    std::stringstream stream;
    std::ifstream f;
    f.exceptions(std::ifstream::badbit);

    f.open(filename);
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
    glUniform1i(glGetUniformLocation(*this, "tex"), 3);
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

void gl::program::bind()
{
    glUseProgram(*this);
}
