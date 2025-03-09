#pragma once

#include <nvrhi/nvrhi.h>
#include <glm/glm.hpp>

struct MaContactShadows {
  MaContactShadows(class NvRenderer* renderer, class NvGbuffer* gbuffer);
  void Init();
  void Render(nvrhi::ICommandList* command_list);
  void UpdateConstants(nvrhi::ICommandList* command_list,
                       const glm::dmat4& projection, const glm::dmat4& view,
                       const glm::dvec3& light, uint64_t frame_index);

  struct ComputeConstants {
    glm::mat4 g_Projection;
    glm::mat4 g_InverseProjection;
    glm::vec3 g_LightDirView;
    float g_NumSamples;
    float g_SampleRange;
    float g_Thickness;
    uint32_t g_FrameIndex;
  };

  nvrhi::BindingLayoutHandle m_binding_layout;
  nvrhi::BindingSetHandle m_binding_set;
  nvrhi::ComputePipelineHandle m_pipeline;
  nvrhi::ShaderHandle m_compute_shader;
  nvrhi::BufferHandle m_constant_buffer;

  nvrhi::TextureHandle m_output_texture;

  class NvRendererBackend* m_backend;
  class NvGbuffer* m_gbuffer;
  int m_width;
  int m_height;
};