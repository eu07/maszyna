#include "Logs.h"
#include "nvrenderer/nvrenderer.h"

namespace {
std::unique_ptr<gfx_renderer> create_nvrenderer_for_d3d12() {
  return std::make_unique<NvRenderer>(NvRenderer::Api::D3D12);
}
std::unique_ptr<gfx_renderer> create_nvrenderer_for_vulkan() {
  return std::make_unique<NvRenderer>(NvRenderer::Api::Vulkan);
}
std::unique_ptr<gfx_renderer> create_nvrenderer_default() {
#if LIBMANUL_WITH_D3D12
  return std::make_unique<NvRenderer>(NvRenderer::Api::D3D12);
#elif LIBMANUL_WITH_VULKAN
  return std::make_unique<NvRenderer>(NvRenderer::Api::Vulkan);
#endif
  ErrorLog("Failed to initialize any NVRHI Renderer");
  return nullptr;
}
}  // namespace

bool register_manul_d3d12 =
    LIBMANUL_WITH_D3D12 &&
    gfx_renderer_factory::get_instance()->register_backend(
        "experimental_d3d12", &create_nvrenderer_for_d3d12);
bool register_manul_vulkan =
    LIBMANUL_WITH_VULKAN &&
    gfx_renderer_factory::get_instance()->register_backend(
        "experimental_vk", &create_nvrenderer_for_vulkan);
bool register_manul = gfx_renderer_factory::get_instance()->register_backend(
    "experimental", &create_nvrenderer_default);