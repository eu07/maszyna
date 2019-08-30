#include "stdafx.h"
#include "cubemap.h"

gl::cubemap::cubemap()
{
    glGenTextures(1, *this);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

gl::cubemap::~cubemap()
{
    glDeleteTextures(1, *this);
}

void gl::cubemap::alloc(GLint format, int width, int height, GLenum components, GLenum type)
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, *this);
    for (GLuint tgt = GL_TEXTURE_CUBE_MAP_POSITIVE_X; tgt <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z; tgt++)
        glTexImage2D(tgt, 0, format, width, height, 0, components, type, nullptr);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void gl::cubemap::bind(int unit)
{
    glActiveTexture(unit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, *this);
}

void gl::cubemap::generate_mipmaps()
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, *this);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}
