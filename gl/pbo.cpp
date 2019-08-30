#include "stdafx.h"
#include "pbo.h"

void gl::pbo::request_read(int x, int y, int lx, int ly, int pixsize, GLenum format, GLenum type)
{
    int s = lx * ly * pixsize;
    if (s != size)
        allocate(PIXEL_PACK_BUFFER, s, GL_STREAM_DRAW);
    size = s;

    data_ready = false;
    sync.reset();

    bind(PIXEL_PACK_BUFFER);
    glReadPixels(x, y, lx, ly, format, type, 0);
    unbind(PIXEL_PACK_BUFFER);

    sync.emplace();
}

bool gl::pbo::read_data(int lx, int ly, void *data, int pixsize)
{
    is_busy();

    if (!data_ready)
        return false;

    int s = lx * ly * pixsize;
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

void* gl::pbo::map(GLuint mode, targets target)
{
	bind(target);
	return glMapBuffer(buffer::glenum_target(target), mode);
}

void gl::pbo::unmap(targets target)
{
	bind(target);
	glUnmapBuffer(buffer::glenum_target(target));
	sync.emplace();
}
