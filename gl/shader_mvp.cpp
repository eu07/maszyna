#include "stdafx.h"
#include "shader_mvp.h"

void gl::program_mvp::init()
{
    glUniformBlockBinding(*this, 0, glGetUniformBlockIndex(*this, "scene_ubo"));
    glUniformBlockBinding(*this, 1, glGetUniformBlockIndex(*this, "model_ubo"));
}
