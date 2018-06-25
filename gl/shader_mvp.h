#pragma once

#include "shader.h"

namespace gl
{
    class program_mvp : public program
    {
    public:
        using program::program;

        virtual void init() override;
    };
}
