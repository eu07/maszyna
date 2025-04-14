#include "nvrenderer_imgui.h"

#include "nvrendererbackend.h"

bool NvImguiRenderer::Init() { return m_rhi.init(m_backend.get()); }

void NvImguiRenderer::Shutdown() {}

void NvImguiRenderer::BeginFrame() { m_rhi.beginFrame(); }

void NvImguiRenderer::Render() {
  m_rhi.render(m_backend->GetCurrentFramebuffer());
}

NvImguiRenderer::NvImguiRenderer(NvRenderer *renderer)
    : m_backend(renderer->m_backend) {
  m_rhi.renderer = m_backend->GetDevice();
  m_rhi.textureManager = renderer->GetTextureManager();
}
