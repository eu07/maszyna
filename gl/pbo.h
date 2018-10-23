#include "buffer.h"
#include "fence.h"
#include <optional>

namespace gl {
    class pbo : private buffer
    {
        std::optional<fence> sync;
        int size = 0;
        bool data_ready;

    public:
        void request_read(int x, int y, int lx, int ly);
        bool read_data(int lx, int ly, uint8_t *data);
        bool is_busy();
    };
}
