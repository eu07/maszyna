#pragma once

#include <ffx-fsr2-api/ffx_fsr2.h>
#include <glm/glm.hpp>

#include "nvrendererbackend.h"

struct NvFSR {
  NvFSR(class NvRenderer* renderer);
  void Init();
  void Render(nvrhi::ICommandList* command_list, double far_plane,
              double near_plane, double fov);

  FfxFsr2Interface m_fsr2_interface;
  FfxFsr2Context m_fsr2_context;
  std::string m_scratch_buffer;

  nvrhi::ITexture* m_color;
  nvrhi::ITexture* m_auto_exposure_buffer;
  nvrhi::ITexture* m_depth;
  nvrhi::ITexture* m_motion;
  nvrhi::TextureHandle m_output;

  FfxResource GetFfxResource(
      nvrhi::ITexture* texture, wchar_t const* name = nullptr,
      FfxResourceStates state = FFX_RESOURCE_STATE_COMPUTE_READ);

  int m_input_width;
  int m_input_height;

  int m_width;
  int m_height;
  glm::vec2 m_current_jitter;

  class NvRendererBackend* m_backend;
  struct NvGbuffer* m_gbuffer;
  struct GbufferBlitPass* m_inputpass;
  struct MaAutoExposure* m_auto_exposure;

  size_t m_current_frame = 0;

  void BeginFrame();
  glm::dmat4 GetJitterMatrix() const;
  void ComputeJitter();
};