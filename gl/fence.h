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
    };
}
