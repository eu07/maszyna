#include "copypass.h"

#include <nvrhi/utils.h>

#include "nvrendererbackend.h"

void CopyPass::Init() {
  nvrhi::utils::CreateBindingSetAndLayout(
      m_backend->GetDevice(), nvrhi::ShaderType::Pixel, 0,
      nvrhi::BindingSetDesc()
          .addItem(nvrhi::BindingSetItem::Texture_SRV(0, m_source))
          .addItem(nvrhi::BindingSetItem::Sampler(
              0, m_backend->GetDevice()->createSampler(nvrhi::SamplerDesc()))),
      m_binding_layout, m_binding_set);
  m_pixel_shader = m_backend->CreateShader("copy", nvrhi::ShaderType::Pixel);
  m_framebuffer = m_backend->GetDevice()->createFramebuffer(
      nvrhi::FramebufferDesc().addColorAttachment(m_output));
  FullScreenPass::Init();
}

void CopyPass::CreatePipelineDesc(nvrhi::GraphicsPipelineDesc& pipeline_desc) {
  FullScreenPass::CreatePipelineDesc(pipeline_desc);
  pipeline_desc.addBindingLayout(m_binding_layout);
  pipeline_desc.setPixelShader(m_pixel_shader);
}

void CopyPass::Render(nvrhi::ICommandList* command_list) {
  nvrhi::GraphicsState graphics_state;
  InitState(graphics_state);
  graphics_state.addBindingSet(m_binding_set);
  command_list->setGraphicsState(graphics_state);
  Draw(command_list);
}

nvrhi::IFramebuffer* CopyPass::GetFramebuffer() {
  return m_framebuffer;
}
