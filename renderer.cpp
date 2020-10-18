/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "renderer.h"
#include "Logs.h"

std::unique_ptr<gfx_renderer> GfxRenderer;

bool gfx_renderer_factory::register_backend(const std::string &backend, gfx_renderer_factory::create_method func)
{
    backends[backend] = func;
    return true;
}

std::unique_ptr<gfx_renderer> gfx_renderer_factory::create(const std::string &backend)
{
    auto it = backends.find(backend);
    if (it != backends.end())
        return it->second();

    ErrorLog("renderer \"" + backend + "\" not found!");
    return nullptr;
}

gfx_renderer_factory *gfx_renderer_factory::get_instance()
{
    if (!instance)
        instance = new gfx_renderer_factory();

    return instance;
}

gfx_renderer_factory *gfx_renderer_factory::instance;


//---------------------------------------------------------------------------
