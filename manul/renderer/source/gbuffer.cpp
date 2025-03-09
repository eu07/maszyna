#include "gbuffer.h"

#include <nvrhi/utils.h>

#include "nvrenderer/nvrenderer.h"
#include "nvrendererbackend.h"

NvGbuffer::NvGbuffer(NvRenderer* renderer)
    : m_backend(renderer->m_backend.get()) {}

void NvGbuffer::Init(int width, int height, bool is_cube, bool depth_only,
                     int slices) {
  m_depth_only = depth_only;
  nvrhi::TextureDesc buffer_desc =
      nvrhi::TextureDesc()
          .setWidth(width)
          .setHeight(height)
          .setInitialState(nvrhi::ResourceStates::ShaderResource)
          .setIsRenderTarget(true)
          .setKeepInitialState(true)
          .setClearValue(nvrhi::Color(0.f, 0.f, 0.f, 0.f))
          .setUseClearValue(true);
  if (is_cube) {
    buffer_desc.setDimension(nvrhi::TextureDimension::TextureCube);
    buffer_desc.setArraySize(6);
  } else if (slices > 1) {
    buffer_desc.setDimension(nvrhi::TextureDimension::Texture2DArray);
    buffer_desc.setArraySize(slices);
  }
  if (!depth_only) {
    m_gbuffer_diffuse = m_backend->GetDevice()->createTexture(
        nvrhi::TextureDesc(buffer_desc)
            .setFormat(nvrhi::Format::R10G10B10A2_UNORM)
            .setDebugName("GBuffer Diffuse"));
    m_gbuffer_emission = m_backend->GetDevice()->createTexture(
        nvrhi::TextureDesc(buffer_desc)
            .setFormat(nvrhi::Format::RGBA16_FLOAT)
            .setDebugName("GBuffer Emission"));
    m_gbuffer_params = m_backend->GetDevice()->createTexture(
        nvrhi::TextureDesc(buffer_desc)
            .setFormat(nvrhi::Format::RGBA8_UNORM)
            .setDebugName("GBuffer Params"));
    m_gbuffer_normal = m_backend->GetDevice()->createTexture(
        nvrhi::TextureDesc(buffer_desc)
            .setFormat(nvrhi::Format::R10G10B10A2_UNORM)
            .setClearValue(nvrhi::Color(.5f, .5f, 1.f, 0.f))
            .setDebugName("GBuffer Normal"));
    m_gbuffer_motion = m_backend->GetDevice()->createTexture(
        nvrhi::TextureDesc(buffer_desc)
            .setFormat(nvrhi::Format::RG16_FLOAT)
            .setClearValue(nvrhi::Color(0.f, 0.f, 0.f, 0.f))
            .setDebugName("GBuffer Motion"));
  }
  m_gbuffer_depth = m_backend->GetDevice()->createTexture(
      nvrhi::TextureDesc(buffer_desc)
          .setIsTypeless(true)
          .setInitialState(nvrhi::ResourceStates::ShaderResource)
          .setFormat(nvrhi::Format::D32)
          .setDebugName("GBuffer Deph"));
  nvrhi::TextureSubresourceSet subresource_set =
      nvrhi::TextureSubresourceSet(0, 1, 0, buffer_desc.arraySize);
  nvrhi::FramebufferDesc fb_desc = nvrhi::FramebufferDesc().setDepthAttachment(
      m_gbuffer_depth, subresource_set);
  if (!depth_only) {
    fb_desc.addColorAttachment(m_gbuffer_diffuse, subresource_set)
        .addColorAttachment(m_gbuffer_emission, subresource_set)
        .addColorAttachment(m_gbuffer_params, subresource_set)
        .addColorAttachment(m_gbuffer_normal, subresource_set)
        .addColorAttachment(m_gbuffer_motion, subresource_set);
  }
  m_framebuffer = m_backend->GetDevice()->createFramebuffer(fb_desc);
  m_slice_framebuffers.resize(buffer_desc.arraySize);
  for (int i = 0; i < buffer_desc.arraySize; ++i) {
    nvrhi::TextureSubresourceSet subresource_set =
        nvrhi::TextureSubresourceSet(0, 1, i, 1);
    nvrhi::FramebufferDesc fb_desc =
        nvrhi::FramebufferDesc().setDepthAttachment(m_gbuffer_depth,
                                                    subresource_set);
    if (!depth_only) {
      fb_desc.addColorAttachment(m_gbuffer_diffuse, subresource_set)
          .addColorAttachment(m_gbuffer_emission, subresource_set)
          .addColorAttachment(m_gbuffer_params, subresource_set)
          .addColorAttachment(m_gbuffer_normal, subresource_set)
          .addColorAttachment(m_gbuffer_motion, subresource_set);
    }
    m_slice_framebuffers[i] =
        m_backend->GetDevice()->createFramebuffer(fb_desc);
  }
}

void NvGbuffer::Clear(nvrhi::ICommandList* command_list) {
  if (!m_depth_only) {
    nvrhi::utils::ClearColorAttachment(command_list, m_framebuffer, 0,
                                       nvrhi::Color(0.f, 0.f, 0.f, 0.f));
    nvrhi::utils::ClearColorAttachment(command_list, m_framebuffer, 1,
                                       nvrhi::Color(0.f, 0.f, 0.f, 0.f));
    nvrhi::utils::ClearColorAttachment(command_list, m_framebuffer, 2,
                                       nvrhi::Color(0.f, 0.f, 0.f, 0.f));
    nvrhi::utils::ClearColorAttachment(command_list, m_framebuffer, 3,
                                       nvrhi::Color(.5f, .5f, 1.f, 0.f));
    nvrhi::utils::ClearColorAttachment(command_list, m_framebuffer, 4,
                                       nvrhi::Color(0.f, 0.f, 0.f, 0.f));
  }
  ClearDepthStencilAttachment(command_list, m_framebuffer,
                              m_depth_only ? 0.f : 0.f, 0);
}

void NvGbuffer::ClearDepthStencilAttachment(nvrhi::ICommandList* commandList,
                                            nvrhi::IFramebuffer* framebuffer,
                                            float depth, uint32_t stencil) {
  const nvrhi::FramebufferAttachment& att =
      framebuffer->getDesc().depthAttachment;
  if (att.texture) {
    commandList->clearDepthStencilTexture(att.texture, att.subresources, true,
                                          depth, false, stencil);
  }
}
