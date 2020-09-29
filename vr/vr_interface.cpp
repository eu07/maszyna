#include "stdafx.h"
#include "vr_interface.h"
#include "Logs.h"

vr_interface::~vr_interface() {}

bool vr_interface_factory::register_backend(const std::string &backend, vr_interface_factory::create_method func)
{
    backends[backend] = func;
    return true;
}

std::unique_ptr<vr_interface> vr_interface_factory::create(const std::string &backend)
{
    auto it = backends.find(backend);
    if (it != backends.end())
        return it->second();

    ErrorLog("vr backend \"" + backend + "\" not found!");
    return nullptr;
}

vr_interface_factory *vr_interface_factory::get_instance()
{
    if (!instance)
        instance = new vr_interface_factory();

    return instance;
}

vr_interface_factory *vr_interface_factory::instance;
