#pragma once

#include "nvrenderer/resource_registry.h"

#include <nvrhi/nvrhi.h>

class NvRenderer;
class NvRendererBackend;

struct MaAutoExposure : public MaResourceRegistry {
  MaAutoExposure(NvRenderer* renderer);

  void Init();
  void Render(nvrhi::ICommandList* command_list);

  uint32_t m_width;
  uint32_t m_height;
  uint32_t m_num_mips;

  struct PushConstantsAutoExposure {
    uint32_t m_num_mips;
    uint32_t m_num_workgroups;
    float m_delta_time;
    float m_min_luminance_ev;
    float m_max_luminance_ev;
  };

  NvRendererBackend* m_backend;

  nvrhi::TextureHandle m_output_exposure_fsr2;
  nvrhi::TextureHandle m_6th_mip_img;
  nvrhi::BufferHandle m_atomic_counter;

  nvrhi::BindingSetHandle m_bindings_auto_exposure;
  nvrhi::ComputePipelineHandle m_pso_auto_exposure;

  nvrhi::BindingSetHandle m_bindings_auto_exposure_apply;
  nvrhi::ComputePipelineHandle m_pso_auto_exposure_apply;
};