#include "fsr.h"

#if LIBMANUL_WITH_D3D12
#include <ffx-fsr2-api/dx12/ffx_fsr2_dx12.h>

#include "nvrenderer_d3d12.h"
#endif

#if LIBMANUL_WITH_VULKAN
#include <ffx-fsr2-api/vk/ffx_fsr2_vk.h>
#include <nvrhi/vulkan.h>

#include "nvrenderer_vulkan.h"
#endif

#include "Globals.h"
#include "gbuffer.h"
#include "gbufferblitpass.h"

NvFSR::NvFSR(NvRenderer* renderer)
    : m_backend(renderer->m_backend.get()),
      m_inputpass(renderer->m_gbuffer_blit.get()),
      m_gbuffer(renderer->m_gbuffer.get()) {}

void OnFsrMessage(FfxFsr2MsgType type, const wchar_t* msg) {
  //__debugbreak();
}

void NvFSR::Init() {
  m_width = m_backend->GetCurrentFramebuffer()->getFramebufferInfo().width;
  m_height = m_backend->GetCurrentFramebuffer()->getFramebufferInfo().height;
  m_input_width = m_gbuffer->m_framebuffer->getFramebufferInfo().width;
  m_input_height = m_gbuffer->m_framebuffer->getFramebufferInfo().height;

  m_color = m_inputpass->m_output;
  m_depth = m_gbuffer->m_gbuffer_depth;
  m_motion = m_gbuffer->m_gbuffer_motion;

  m_output = m_backend->GetDevice()->createTexture(
      nvrhi::TextureDesc()
          .setFormat(nvrhi::Format::RGBA16_FLOAT)
          .setWidth(m_width)
          .setHeight(m_height)
          .setIsUAV(true)
          .setDebugName("FSR Output")
          .setInitialState(nvrhi::ResourceStates::ShaderResource)
          .setKeepInitialState(true));
  m_auto_exposure_buffer = m_backend->GetDevice()->createTexture(
      nvrhi::TextureDesc()
          .setFormat(nvrhi::Format::RG32_FLOAT)
          .setWidth(1)
          .setHeight(1)
          .setIsUAV(true)
          .setDebugName("FSR Auto Exposure Buffer")
          .setInitialState(nvrhi::ResourceStates::UnorderedAccess)
          .setKeepInitialState(true));
  auto device = m_backend->GetDevice();
  FfxFsr2ContextDescription context_description{};
  switch (device->getGraphicsAPI()) {
#if LIBMANUL_WITH_D3D12
    case nvrhi::GraphicsAPI::D3D12:
      m_scratch_buffer.resize(ffxFsr2GetScratchMemorySizeDX12());
      ffxFsr2GetInterfaceDX12(&m_fsr2_interface,
                              m_backend->GetDevice()->getNativeObject(
                                  nvrhi::ObjectTypes::D3D12_Device),
                              m_scratch_buffer.data(), m_scratch_buffer.size());
      context_description.device = m_backend->GetDevice()->getNativeObject(
          nvrhi::ObjectTypes::D3D12_Device);
      break;
#endif
#if LIBMANUL_WITH_VULKAN
    case nvrhi::GraphicsAPI::VULKAN:
      m_scratch_buffer.resize(
          ffxFsr2GetScratchMemorySizeVK(m_backend->GetDevice()->getNativeObject(
              nvrhi::ObjectTypes::VK_PhysicalDevice)));
      ffxFsr2GetInterfaceVK(&m_fsr2_interface, m_scratch_buffer.data(),
                            m_scratch_buffer.size(),
                            m_backend->GetDevice()->getNativeObject(
                                nvrhi::ObjectTypes::VK_PhysicalDevice),
                            dynamic_cast<NvRendererBackend_Vulkan*>(m_backend)
                                ->GetDeviceProcAddr());
      context_description.device =
          ffxGetDeviceVK(m_backend->GetDevice()->getNativeObject(
              nvrhi::ObjectTypes::VK_Device));
      break;
#endif
    default:
      break;
  }
  context_description.callbacks = m_fsr2_interface;
  context_description.displaySize.width = m_width;
  context_description.displaySize.height = m_height;
  context_description.maxRenderSize.width = 3440;
  context_description.maxRenderSize.height = 1440;
  context_description.flags =  // FFX_FSR2_ENABLE_DEBUG_CHECKING |
      FFX_FSR2_ENABLE_AUTO_EXPOSURE | FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE |
      FFX_FSR2_ENABLE_DEPTH_INVERTED;
  context_description.fpMessage = &OnFsrMessage;
  ffxFsr2ContextCreate(&m_fsr2_context, &context_description);
}

void NvFSR::Render(nvrhi::ICommandList* command_list, double far_plane,
                   double near_plane, double fov) {
  FfxFsr2DispatchDescription dispatch_description{};
  switch (m_backend->GetDevice()->getGraphicsAPI()) {
#if LIBMANUL_WITH_D3D12
    case nvrhi::GraphicsAPI::D3D12:
      dispatch_description.commandList =
          ffxGetCommandListDX12(command_list->getNativeObject(
              nvrhi::ObjectTypes::D3D12_GraphicsCommandList));
      break;
#endif
#if LIBMANUL_WITH_VULKAN
    case nvrhi::GraphicsAPI::VULKAN:
      dispatch_description.commandList = ffxGetCommandListVK(
          command_list->getNativeObject(nvrhi::ObjectTypes::VK_CommandBuffer));
      break;
#endif
    default:
      break;
  }
  dispatch_description.color = GetFfxResource(m_color, L"FSR2 Input Color");
  dispatch_description.depth = GetFfxResource(m_depth, L"FSR2 Input Depth");
  dispatch_description.motionVectors =
      GetFfxResource(m_motion, L"FSR2 Input Motion");
  dispatch_description.exposure =
      GetFfxResource(nullptr, L"FSR2 Empty Auto Exposure");
  dispatch_description.reactive =
      GetFfxResource(nullptr, L"FSR2 Empty Input Reactive Map");
  dispatch_description.transparencyAndComposition =
      GetFfxResource(nullptr, L"FSR2 Empty Transparency And Composition Map");
  dispatch_description.output = GetFfxResource(
      m_output, L"FSR2 Output", FFX_RESOURCE_STATE_UNORDERED_ACCESS);

  dispatch_description.jitterOffset.x = m_current_jitter.x;
  dispatch_description.jitterOffset.y = m_current_jitter.y;
  dispatch_description.motionVectorScale.x = static_cast<float>(m_input_width);
  dispatch_description.motionVectorScale.y = static_cast<float>(m_input_height);
  dispatch_description.reset = false;
  dispatch_description.enableSharpening = true;
  dispatch_description.sharpness = .4;
  dispatch_description.frameTimeDelta = 10.f;
  dispatch_description.preExposure = 1.0f;
  dispatch_description.renderSize.width = m_input_width;
  dispatch_description.renderSize.height = m_input_height;
  dispatch_description.cameraFar = far_plane;
  dispatch_description.cameraNear = near_plane;
  dispatch_description.cameraFovAngleVertical = fov;

  ffxFsr2ContextDispatch(&m_fsr2_context, &dispatch_description);
  command_list->clearState();
}

FfxResource NvFSR::GetFfxResource(nvrhi::ITexture* texture, wchar_t const* name,
                                  FfxResourceStates state) {
  switch (m_backend->GetDevice()->getGraphicsAPI()) {
#if LIBMANUL_WITH_D3D12
    case nvrhi::GraphicsAPI::D3D12:
      if (!texture) {
        return ffxGetResourceDX12(&m_fsr2_context, nullptr, name, state);
      }
      return ffxGetResourceDX12(
          &m_fsr2_context,
          texture->getNativeObject(nvrhi::ObjectTypes::D3D12_Resource), name,
          state);
#endif
#if LIBMANUL_WITH_VULKAN
    case nvrhi::GraphicsAPI::VULKAN: {
      if (!texture) {
        return ffxGetTextureResourceVK(&m_fsr2_context, nullptr, nullptr, 1, 1,
                                       VK_FORMAT_UNDEFINED, name, state);
      }
      const auto& desc = texture->getDesc();
      return ffxGetTextureResourceVK(
          &m_fsr2_context,
          texture->getNativeObject(nvrhi::ObjectTypes::VK_Image),
          texture->getNativeView(nvrhi::ObjectTypes::VK_ImageView), desc.width,
          desc.height, nvrhi::vulkan::convertFormat(desc.format), name, state);
    }
#endif
    default:
      return {};
  }
}

void NvFSR::BeginFrame() {
  ++m_current_frame;
  ComputeJitter();
}

glm::dmat4 NvFSR::GetJitterMatrix() const {
  return glm::translate(glm::dvec3(2. * m_current_jitter.x / m_input_width,
                                   -2. * m_current_jitter.y / m_input_height,
                                   0.));
}

void NvFSR::ComputeJitter() {
  int phase_count = ffxFsr2GetJitterPhaseCount(m_input_width, m_width);
  ffxFsr2GetJitterOffset(&m_current_jitter.x, &m_current_jitter.y,
                         m_current_frame % phase_count, phase_count);
}
