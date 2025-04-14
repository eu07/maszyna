#include "auto_exposure.h"

#include <nvrhi/utils.h>

#include "config.h"
#include "nvrenderer/nvrenderer.h"
#include "nvrendererbackend.h"

MaAutoExposure::MaAutoExposure(NvRenderer *renderer)
    : MaResourceRegistry(renderer), m_backend(renderer->GetBackend()) {}

void MaAutoExposure::Render(nvrhi::ICommandList *command_list) {
  PushConstantsAutoExposure push_constants{};
  push_constants.m_delta_time = .01f;
  push_constants.m_num_mips = m_num_mips;
  push_constants.m_num_workgroups =
      ((m_width + 63) / 64) * ((m_height + 64) / 64);
  push_constants.m_min_luminance_ev =
      NvRenderer::Config()->m_lighting.m_min_luminance_ev;
  push_constants.m_max_luminance_ev =
      NvRenderer::Config()->m_lighting.m_max_luminance_ev;
  command_list->setComputeState(nvrhi::ComputeState()
                                    .addBindingSet(m_bindings_auto_exposure)
                                    .setPipeline(m_pso_auto_exposure));
  command_list->setPushConstants(&push_constants, sizeof(push_constants));
  command_list->dispatch((m_width + 63) / 64, (m_height + 64) / 64, 1);

  command_list->setComputeState(
      nvrhi::ComputeState()
          .addBindingSet(m_bindings_auto_exposure_apply)
          .setPipeline(m_pso_auto_exposure_apply));
  command_list->dispatch((m_width + 7) / 8, (m_height + 7) / 8, 1);
}

void MaAutoExposure::Init() {
  nvrhi::TextureHandle input_texture = dynamic_cast<nvrhi::ITexture *>(
      GetResource("scene_lit_texture", nvrhi::ResourceType::Texture_SRV)
          .m_resource);

  auto desc = input_texture->getDesc();
  m_width = desc.width;
  m_height = desc.height;
  m_num_mips =
      glm::floor(glm::log2(static_cast<float>(glm::max(m_width, m_height))));

  m_6th_mip_img = m_backend->GetDevice()->createTexture(
      nvrhi::TextureDesc()
          .setFormat(nvrhi::Format::R32_FLOAT)
          .setWidth(m_width >> 6)
          .setHeight(m_height >> 6)
          .setIsUAV(true)
          .setInitialState(nvrhi::ResourceStates::UnorderedAccess)
          .setKeepInitialState(true)
          .setDebugName("Auto Exposure 6th Mip"));

  m_output_exposure_fsr2 = m_backend->GetDevice()->createTexture(
      nvrhi::TextureDesc()
          .setFormat(nvrhi::Format::RG16_FLOAT)
          .setWidth(1)
          .setHeight(1)
          .setIsUAV(true)
          .setInitialState(nvrhi::ResourceStates::ShaderResource)
          .setKeepInitialState(true)
          .setDebugName("Auto Exposure Output (FSR2)"));

  m_atomic_counter = m_backend->GetDevice()->createBuffer(
      nvrhi::BufferDesc()
          .setByteSize(sizeof(uint32_t))
          .setStructStride(sizeof(uint32_t))
          .setCanHaveUAVs(true)
          .setInitialState(nvrhi::ResourceStates::UnorderedAccess)
          .setKeepInitialState(true)
          .setDebugName("Auto Exposure Atomic Counter"));

  auto cs_auto_exposure = m_backend->CreateShader(
      "autoexp_compute_avg_luminance", nvrhi::ShaderType::Compute);

  nvrhi::BindingLayoutHandle binding_layout;
  nvrhi::utils::CreateBindingSetAndLayout(
      m_backend->GetDevice(), nvrhi::ShaderType::Compute, 0,
      nvrhi::BindingSetDesc()
          .addItem(nvrhi::BindingSetItem::PushConstants(
              1, sizeof(PushConstantsAutoExposure)))
          .addItem(nvrhi::BindingSetItem::Texture_SRV(0, input_texture))
          .addItem(
              nvrhi::BindingSetItem::Texture_UAV(0, m_output_exposure_fsr2))
          .addItem(
              nvrhi::BindingSetItem::StructuredBuffer_UAV(2, m_atomic_counter))
          .addItem(nvrhi::BindingSetItem::Texture_UAV(3, m_6th_mip_img)),
      binding_layout, m_bindings_auto_exposure);

  m_pso_auto_exposure = m_backend->GetDevice()->createComputePipeline(
      nvrhi::ComputePipelineDesc()
          .addBindingLayout(binding_layout)
          .setComputeShader(cs_auto_exposure));

  auto cs_auto_exposure_apply =
      m_backend->CreateShader("autoexp_apply", nvrhi::ShaderType::Compute);

  nvrhi::BindingLayoutHandle binding_layout_apply;
  nvrhi::utils::CreateBindingSetAndLayout(
      m_backend->GetDevice(), nvrhi::ShaderType::Compute, 0,
      nvrhi::BindingSetDesc()
          .addItem(
              nvrhi::BindingSetItem::Texture_SRV(0, m_output_exposure_fsr2))
          .addItem(nvrhi::BindingSetItem::Texture_UAV(0, input_texture)),
      binding_layout_apply, m_bindings_auto_exposure_apply);

  m_pso_auto_exposure_apply = m_backend->GetDevice()->createComputePipeline(
      nvrhi::ComputePipelineDesc()
          .addBindingLayout(binding_layout_apply)
          .setComputeShader(cs_auto_exposure_apply));
}
