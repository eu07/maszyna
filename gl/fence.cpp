#include "fence.h"

gl::fence::fence()
{
    sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

gl::fence::~fence()
{
    glDeleteSync(sync);
}

bool gl::fence::is_signalled()
{
    GLint val;
    glGetSynciv(sync, GL_SYNC_STATUS, 1, &val);
    return val == GL_SIGNALED;
}
