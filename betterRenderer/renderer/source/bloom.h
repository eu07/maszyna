#pragma once

#include <glm/glm.hpp>
#include <nvrhi/nvrhi.h>

struct Bloom {
  struct BloomConstants {
    glm::vec4 m_prefilter_vector;
    float m_mix_factor;
  };
  float m_threshold = 3.f;
  float m_knee = .6f;
  float m_mix_factor = .04f;
  bool m_gui_active = false;
  void UpdateConstants(nvrhi::ICommandList* command_list);
  void OnGui();
  void ShowGui();
  Bloom(class NvRendererBackend* backend) : m_backend(backend) {}

  void Init(nvrhi::ITexture* texture);
  void Render(nvrhi::ICommandList* command_list);

  nvrhi::TextureHandle m_working_texture;
  nvrhi::BufferHandle m_constant_buffer;

  struct BloomStage {
    int m_width;
    int m_height;
    nvrhi::BindingSetHandle m_binding_set;
    nvrhi::ComputePipelineHandle m_pso;
  };

  std::vector<BloomStage> m_stages;

  class NvRendererBackend* m_backend;
};