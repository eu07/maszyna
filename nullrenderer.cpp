#include "stdafx.h"
#include "nullrenderer.h"

std::unique_ptr<gfx_renderer> null_renderer::create_func()
{
    return std::unique_ptr<null_renderer>(new null_renderer());
}

bool null_renderer::renderer_register = gfx_renderer_factory::get_instance()->register_backend("null", null_renderer::create_func);
