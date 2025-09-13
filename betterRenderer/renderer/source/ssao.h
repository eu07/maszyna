#pragma once

#include <memory>

#include <glm/glm.hpp>

#include <nvrhi/nvrhi.h>

struct NvSsao {
  NvSsao(class NvRenderer* renderer);
  void Init();

  class NvRendererBackend* m_backend;
  struct NvGbuffer* m_gbuffer;

  nvrhi::ShaderHandle m_CSPrefilterDepths16x16;
  nvrhi::ComputePipelineHandle m_PSOPrefilterDepths;
  nvrhi::BindingSetHandle m_BSPrefilterDepths;

  nvrhi::SamplerHandle m_SamplerPoint;

  nvrhi::ShaderHandle m_CSGTAOLow;
  nvrhi::ShaderHandle m_CSGTAOMedium;
  nvrhi::ShaderHandle m_CSGTAOHigh;
  nvrhi::ShaderHandle m_CSGTAOUltra;
  nvrhi::ComputePipelineHandle m_PSOGTAO;
  nvrhi::BindingSetHandle m_BSGTAO;

  nvrhi::ShaderHandle m_CSDenoisePass;
  nvrhi::ShaderHandle m_CSDenoiseLastPass;
  nvrhi::ComputePipelineHandle m_PSODenoise;
  nvrhi::ComputePipelineHandle m_PSODenoiseLastPass;
  nvrhi::BindingLayoutHandle m_BLDenoise;
  nvrhi::BindingSetHandle m_BSDenoise;

  nvrhi::ShaderHandle m_CSGenerateNormals;
  nvrhi::ComputePipelineHandle m_PSOGenerateNormals;
  nvrhi::BindingSetHandle m_BSGenerateNormals;

  nvrhi::TextureHandle m_workingDepths;
  nvrhi::TextureHandle m_workingEdges;
  nvrhi::TextureHandle m_workingAOTerm;
  nvrhi::TextureHandle m_workingAOTermPong;
  nvrhi::TextureHandle m_workingNormals;
  nvrhi::TextureHandle m_debugImage;
  nvrhi::TextureHandle m_outputAO;
  nvrhi::BufferHandle m_constantBuffer;


  int m_width;
  int m_height;

  void Render(nvrhi::ICommandList* command_list, const glm::mat4& projection,
              size_t frame_index);
};