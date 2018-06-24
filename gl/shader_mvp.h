#pragma once

#include "shader.h"

namespace gl
{
    class program_mvp : public program
    {
        GLuint mv_uniform;
        GLint mvn_uniform;
        GLuint p_uniform;

        glm::mat4 last_mv;
        glm::mat4 last_p;

    public:
        using program::program;

        void set_mv(const glm::mat4 &);
        void set_p(const glm::mat4 &);
        void copy_gl_mvp();

        virtual void init() override;
    };
}
