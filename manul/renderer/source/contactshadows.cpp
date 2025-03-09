#include "contactshadows.h"

#include <nvrhi/utils.h>

#include "gbuffer.h"
#include "nvrenderer/nvrenderer.h"
#include "nvrendererbackend.h"

MaContactShadows::MaContactShadows(NvRenderer* renderer, NvGbuffer* gbuffer)
    : m_backend(renderer->GetBackend()),
      m_gbuffer(gbuffer),
      m_width(0),
      m_height(0) {}

void MaContactShadows::Init() {
  m_width = m_gbuffer->m_gbuffer_depth->getDesc().width;
  m_height = m_gbuffer->m_gbuffer_depth->getDesc().height;
  m_compute_shader = m_backend->CreateShader("contact_shadows",
                                             nvrhi::ShaderType::Compute);
  m_constant_buffer = m_backend->GetDevice()->createBuffer(
      nvrhi::utils::CreateStaticConstantBufferDesc(sizeof(ComputeConstants),
                                                   "Contact shadow constants")
          .setInitialState(nvrhi::ResourceStates::ConstantBuffer)
          .setKeepInitialState(true));
  m_output_texture = m_backend->GetDevice()->createTexture(
      nvrhi::TextureDesc()
          .setFormat(nvrhi::Format::R8_UNORM)
          .setWidth(m_width)
          .setHeight(m_height)
          .setIsUAV(true)
          .setInitialState(nvrhi::ResourceStates::ShaderResource)
          .setKeepInitialState(true));
  nvrhi::utils::CreateBindingSetAndLayout(
      m_backend->GetDevice(), nvrhi::ShaderType::Compute, 0,
      nvrhi::BindingSetDesc()
          .addItem(
              nvrhi::BindingSetItem::Texture_SRV(0, m_gbuffer->m_gbuffer_depth))
          .addItem(nvrhi::BindingSetItem::Texture_UAV(0, m_output_texture))
          .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, m_constant_buffer))
          .addItem(nvrhi::BindingSetItem::Sampler(
              0, m_backend->GetDevice()->createSampler(
                     nvrhi::SamplerDesc()
                         .setAllFilters(true)
                         .setAllAddressModes(
                             nvrhi::SamplerAddressMode::ClampToEdge)
                         .setReductionType(
                             nvrhi::SamplerReductionType::Maximum)))),
      m_binding_layout, m_binding_set);
  m_pipeline = m_backend->GetDevice()->createComputePipeline(
      nvrhi::ComputePipelineDesc()
          .setComputeShader(m_compute_shader)
          .addBindingLayout(m_binding_layout));
}

void MaContactShadows::Render(nvrhi::ICommandList* command_list) {
  command_list->setComputeState(nvrhi::ComputeState()
                                    .setPipeline(m_pipeline)
                                    .addBindingSet(m_binding_set));
  command_list->dispatch((m_width + 7) / 8, (m_height + 7) / 8, 1);
}

void MaContactShadows::UpdateConstants(nvrhi::ICommandList* command_list,
                                       const glm::dmat4& projection,
                                       const glm::dmat4& view,
                                       const glm::dvec3& light,
                                       uint64_t frame_index) {
  ComputeConstants constants{};
  constants.g_InverseProjection = glm::inverse(projection);
  constants.g_Projection = projection;
  constants.g_LightDirView = view * glm::dvec4(-light, 0.);
  constants.g_NumSamples = 16;
  constants.g_SampleRange = .1;
  constants.g_Thickness = .01;
  constants.g_FrameIndex = frame_index % 64;
  command_list->writeBuffer(m_constant_buffer, &constants, sizeof(constants));
}
