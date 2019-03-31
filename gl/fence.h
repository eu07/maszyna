#pragma once

#include "object.h"

namespace gl
{
    class fence
    {
        GLsync sync;

    public:
        fence();
        ~fence();

        bool is_signalled();

        fence(const fence&) = delete;
        fence& operator=(const fence&) = delete;
    };
}
