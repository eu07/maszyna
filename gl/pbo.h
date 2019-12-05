#pragma once

#include "buffer.h"
#include "fence.h"
#include <optional>

namespace gl {
    class pbo : public buffer
    {
        std::optional<fence> sync;
        int size = 0;
        bool data_ready;

    public:
        void request_read(int x, int y, int lx, int ly, int pixsize = 4, GLenum format = GL_RGBA, GLenum type = GL_UNSIGNED_BYTE);
        bool read_data(int lx, int ly, void *data, int pixsize = 4);
        bool is_busy();

		void* map(GLuint mode, targets target = PIXEL_UNPACK_BUFFER);
		void unmap(targets target = PIXEL_UNPACK_BUFFER);
    };
}
