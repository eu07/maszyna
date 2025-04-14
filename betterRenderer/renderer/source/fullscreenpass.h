#pragma once

#include <nvrhi/nvrhi.h>

struct FullScreenPass {

  FullScreenPass(class NvRendererBackend* backend);
  virtual ~FullScreenPass() = default;
  nvrhi::IShader* GetVertexShader();
  nvrhi::IInputLayout* GetInputLayout();
  nvrhi::IBuffer* GetVertexBuffer();
  virtual void Init();
  void CreatePipeline();
  virtual void CreatePipelineDesc(nvrhi::GraphicsPipelineDesc& pipeline_desc);
  nvrhi::GraphicsPipelineHandle m_pipeline;
  class NvRendererBackend* m_backend;
  virtual nvrhi::IFramebuffer* GetFramebuffer();
  void InitState(nvrhi::GraphicsState& render_state);
  void Draw(nvrhi::ICommandList* command_list);
  virtual void Render(nvrhi::ICommandList* command_list) = 0;
};