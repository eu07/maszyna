#pragma once

#include <nvrhi/nvrhi.h>

#include "nvrenderer/nvrenderer.h"

#include "fullscreenpass.h"

struct TonemapPass : public FullScreenPass {
  TonemapPass(class NvRenderer* renderer, nvrhi::ITexture* source,
           nvrhi::Format output_format = nvrhi::Format::UNKNOWN)
      : FullScreenPass(renderer->GetBackend()),
        m_renderer(renderer),
        m_source(source),
        m_output_format(output_format == nvrhi::Format::UNKNOWN
                            ? source->getDesc().format
                            : output_format) {}

  virtual void Init() override;

  virtual void CreatePipelineDesc(
      nvrhi::GraphicsPipelineDesc& pipeline_desc) override;

  virtual void Render(nvrhi::ICommandList* command_list) override;

  virtual nvrhi::IFramebuffer* GetFramebuffer() override;

  nvrhi::BindingLayoutHandle m_binding_layout;
  nvrhi::BindingSetHandle m_binding_set;
  nvrhi::ShaderHandle m_pixel_shader;
  nvrhi::FramebufferHandle m_framebuffer;

  nvrhi::ITexture* m_source;
  nvrhi::Format m_output_format;

  nvrhi::TextureHandle m_output;

  class NvRenderer* m_renderer;
};