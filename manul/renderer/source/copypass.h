#pragma once

#include <nvrhi/nvrhi.h>

#include "fullscreenpass.h"

struct CopyPass : public FullScreenPass {
  CopyPass(class NvRendererBackend* backend, nvrhi::ITexture* source, nvrhi::ITexture* output)
      : FullScreenPass(backend),
        m_source(source), m_output(output) {}

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
  nvrhi::ITexture* m_output;
};