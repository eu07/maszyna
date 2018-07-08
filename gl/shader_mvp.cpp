#include "stdafx.h"
#include "shader_mvp.h"

void gl::program_mvp::init()
{
    GLuint index;

    if ((index = glGetUniformBlockIndex(*this, "scene_ubo")) != GL_INVALID_INDEX)
        glUniformBlockBinding(*this, 0, index);

    if ((index = glGetUniformBlockIndex(*this, "model_ubo")) != GL_INVALID_INDEX)
        glUniformBlockBinding(*this, 1, index);

    if ((index = glGetUniformBlockIndex(*this, "light_ubo")) != GL_INVALID_INDEX)
        glUniformBlockBinding(*this, 2, index);
}
