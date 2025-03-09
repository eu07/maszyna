#pragma once

#include <nvrhi/nvrhi.h>

struct NvGbuffer {
  NvGbuffer(class NvRenderer* renderer);
  void Init(int width, int height, bool is_cube, bool depth_only = false, int slices = 1);
  void Clear(nvrhi::ICommandList* command_list);

  nvrhi::TextureHandle m_gbuffer_diffuse;
  nvrhi::TextureHandle m_gbuffer_emission;
  nvrhi::TextureHandle m_gbuffer_normal;
  nvrhi::TextureHandle m_gbuffer_params;
  nvrhi::TextureHandle m_gbuffer_motion;
  nvrhi::TextureHandle m_gbuffer_depth;

  nvrhi::FramebufferHandle m_framebuffer;
  std::vector<nvrhi::FramebufferHandle> m_slice_framebuffers;

  class NvRendererBackend* m_backend;

  bool m_depth_only;

  static void ClearDepthStencilAttachment(nvrhi::ICommandList* commandList,
                                          nvrhi::IFramebuffer* framebuffer,
                                          float depth, uint32_t stencil);
};
