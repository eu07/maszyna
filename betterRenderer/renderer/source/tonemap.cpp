#include "tonemap.h"

#include <nvrhi/utils.h>

#include "config.h"
#include "nvrendererbackend.h"

void TonemapPass::Init() {
  nvrhi::utils::CreateBindingSetAndLayout(
      m_backend->GetDevice(), nvrhi::ShaderType::Pixel, 0,
      nvrhi::BindingSetDesc()
          .addItem(nvrhi::BindingSetItem::Texture_SRV(0, m_source))
          .addItem(nvrhi::BindingSetItem::Texture_SRV(
              1, static_cast<nvrhi::ITexture*>(
                     m_renderer
                         ->GetResource("noise_2d_ldr",
                                       nvrhi::ResourceType::Texture_SRV)
                         .m_resource)))
          .addItem(nvrhi::BindingSetItem::Sampler(
              0, m_backend->GetDevice()->createSampler(nvrhi::SamplerDesc())))
          .addItem(nvrhi::BindingSetItem::PushConstants(0, sizeof(glm::vec4))),
      m_binding_layout, m_binding_set);
  m_pixel_shader = m_backend->CreateShader("tonemap", nvrhi::ShaderType::Pixel);
  m_output = m_backend->GetDevice()->createTexture(
      nvrhi::TextureDesc()
          .setFormat(m_output_format)
          .setWidth(m_source->getDesc().width)
          .setHeight(m_source->getDesc().height)
          .setIsRenderTarget(true)
          .setInitialState(nvrhi::ResourceStates::ShaderResource)
          .setKeepInitialState(true));
  m_framebuffer = m_backend->GetDevice()->createFramebuffer(
      nvrhi::FramebufferDesc().addColorAttachment(m_output));
  FullScreenPass::Init();
}

void TonemapPass::CreatePipelineDesc(
    nvrhi::GraphicsPipelineDesc& pipeline_desc) {
  FullScreenPass::CreatePipelineDesc(pipeline_desc);
  pipeline_desc.addBindingLayout(m_binding_layout);
  pipeline_desc.setPixelShader(m_pixel_shader);
}

void TonemapPass::Render(nvrhi::ICommandList* command_list) {
  nvrhi::GraphicsState graphics_state;
  InitState(graphics_state);
  graphics_state.addBindingSet(m_binding_set);
  command_list->setGraphicsState(graphics_state);

  auto hdr_config = NvRenderer::Config()->m_display.GetHDRConfig();

  struct TonemapConstants {
    MaConfig::MaTonemapType m_tonemap_function;
    float m_scene_exposure;
    float m_scene_nits;
    float m_scene_gamma;
  } tonemap_constants{hdr_config.m_tonemap_function,
                      glm::pow(2.f, hdr_config.m_scene_exposure), hdr_config.m_scene_nits,
                      hdr_config.m_scene_gamma};

  command_list->setPushConstants(&tonemap_constants, sizeof(tonemap_constants));

  Draw(command_list);
}

nvrhi::IFramebuffer* TonemapPass::GetFramebuffer() {
  return FullScreenPass::GetFramebuffer();  // m_framebuffer;
}
