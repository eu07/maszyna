#include "stdafx.h"
#include "shader_mvp.h"

void gl::program_mvp::init()
{
    mv_uniform = glGetUniformLocation(*this, "modelview");
    mvn_uniform = glGetUniformLocation(*this, "modelviewnormal");
    p_uniform = glGetUniformLocation(*this, "projection");
}

void gl::program_mvp::set_mv(const glm::mat4 &m)
{
    if (last_mv == m)
        return;
    last_mv = m;

    if (mvn_uniform != -1)
    {
        glm::mat3 mvn = glm::mat3(glm::transpose(glm::inverse(m)));
        glUniformMatrix3fv(mvn_uniform, 1, GL_FALSE, glm::value_ptr(mvn));
    }
    glUniformMatrix4fv(mv_uniform, 1, GL_FALSE, glm::value_ptr(m));
}

void gl::program_mvp::set_p(const glm::mat4 &m)
{
    if (last_p == m)
        return;
    last_p = m;

    glUniformMatrix4fv(p_uniform, 1, GL_FALSE, glm::value_ptr(m));
}

void gl::program_mvp::copy_gl_mvp()
{
    set_mv(OpenGLMatrices.data(GL_MODELVIEW));
    set_p(OpenGLMatrices.data(GL_PROJECTION));
}
