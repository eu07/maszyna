#include "pbo.h"

void gl::pbo::request_read(int x, int y, int lx, int ly)
{
    int s = lx * ly * 4;
    if (s != size)
        allocate(PIXEL_PACK_BUFFER, s, GL_STREAM_DRAW);
    size = s;

    data_ready = false;
    sync.reset();

    bind(PIXEL_PACK_BUFFER);
    glReadPixels(x, y, lx, ly, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    unbind(PIXEL_PACK_BUFFER);

    sync.emplace();
}

bool gl::pbo::read_data(int lx, int ly, uint8_t *data)
{
    is_busy();

    if (!data_ready)
        return false;

    int s = lx * ly * 4;
    if (s != size)
        return false;

    download(PIXEL_PACK_BUFFER, data, 0, s);
    unbind(PIXEL_PACK_BUFFER);
    data_ready = false;

    return true;
}

bool gl::pbo::is_busy()
{
    if (!sync)
        return false;

    if (sync->is_signalled())
    {
        data_ready = true;
        sync.reset();
        return false;
    }

    return true;
}
