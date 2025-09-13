#include "bloom.h"

#include <nvrhi/utils.h>

#include "nvrendererbackend.h"

void Bloom::UpdateConstants(nvrhi::ICommandList* command_list) {
  BloomConstants constants{};
  constants.m_prefilter_vector =
      glm::vec4{m_threshold, m_threshold - m_knee, m_knee * 2.f, .25f / m_knee};
  constants.m_mix_factor = m_mix_factor;
  command_list->writeBuffer(m_constant_buffer, &constants, sizeof(constants));
}

void Bloom::OnGui() {
  if (!m_gui_active) return;
  if (ImGui::Begin("Bloom settings", &m_gui_active,
                   ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::SliderFloat("Threshold", &m_threshold, 0.f, 100.f);
    ImGui::SliderFloat("Knee", &m_knee, 0.f, 100.f);
    ImGui::SliderFloat("Mix Factor", &m_mix_factor, 0.f, 100.f);
    ImGui::End();
  }
}

void Bloom::ShowGui() { m_gui_active = true; }

void Bloom::Init(nvrhi::ITexture* texture) {
  auto source_desc = texture->getDesc();

  int mip_count = 7;
  m_working_texture = m_backend->GetDevice()->createTexture(
      nvrhi::TextureDesc()
          .setWidth(source_desc.width)
          .setHeight(source_desc.height)
          .setFormat(nvrhi::Format::RGBA16_FLOAT)
          .setMipLevels(mip_count)
          .setIsUAV(true)
          .setInitialState(nvrhi::ResourceStates::UnorderedAccess)
          .setKeepInitialState(true));

  m_constant_buffer = m_backend->GetDevice()->createBuffer(
      nvrhi::utils::CreateVolatileConstantBufferDesc(sizeof(BloomConstants),
                                                     "Bloom Constants", 16)
          .setInitialState(nvrhi::ResourceStates::ConstantBuffer)
          .setKeepInitialState(true));

  auto shader_prefilter =
      m_backend->CreateShader("bloom_prefilter", nvrhi::ShaderType::Compute);
  auto shader_downsample =
      m_backend->CreateShader("bloom_downsample", nvrhi::ShaderType::Compute);
  auto shader_upsample =
      m_backend->CreateShader("bloom_upsample", nvrhi::ShaderType::Compute);
  auto shader_final =
      m_backend->CreateShader("bloom_apply", nvrhi::ShaderType::Compute);
  auto binding_layout = m_backend->GetDevice()->createBindingLayout(
      nvrhi::BindingLayoutDesc()
          .setVisibility(nvrhi::ShaderType::Compute)
          .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0))
          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(0))
          .addItem(nvrhi::BindingLayoutItem::Texture_UAV(0))
          .addItem(nvrhi::BindingLayoutItem::Sampler(0)));
  auto binding_layout_final = m_backend->GetDevice()->createBindingLayout(
      nvrhi::BindingLayoutDesc()
          .setVisibility(nvrhi::ShaderType::Compute)
          .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0))
          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(0))
          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(1))
          .addItem(nvrhi::BindingLayoutItem::Texture_UAV(0))
          .addItem(nvrhi::BindingLayoutItem::Sampler(0)));
  auto pso_prefilter = m_backend->GetDevice()->createComputePipeline(
      nvrhi::ComputePipelineDesc()
          .setComputeShader(shader_prefilter)
          .addBindingLayout(binding_layout));
  auto pso_downsample = m_backend->GetDevice()->createComputePipeline(
      nvrhi::ComputePipelineDesc()
          .setComputeShader(shader_downsample)
          .addBindingLayout(binding_layout));
  auto pso_upsample = m_backend->GetDevice()->createComputePipeline(
      nvrhi::ComputePipelineDesc()
          .setComputeShader(shader_upsample)
          .addBindingLayout(binding_layout));
  auto pso_final = m_backend->GetDevice()->createComputePipeline(
      nvrhi::ComputePipelineDesc()
          .setComputeShader(shader_final)
          .addBindingLayout(binding_layout_final));
  auto sampler_linear_clamp = m_backend->GetDevice()->createSampler(
      nvrhi::SamplerDesc().setAllFilters(true).setAllAddressModes(
          nvrhi::SamplerAddressMode::ClampToEdge));
  {
    auto& stage = m_stages.emplace_back();
    stage.m_width = source_desc.width;
    stage.m_height = source_desc.height;
    stage.m_pso = pso_prefilter;
    stage.m_binding_set = m_backend->GetDevice()->createBindingSet(
        nvrhi::BindingSetDesc()
            .addItem(
                nvrhi::BindingSetItem::ConstantBuffer(0, m_constant_buffer))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(
                0, texture, nvrhi::Format::UNKNOWN,
                nvrhi::TextureSubresourceSet(0, 1, 0, 1)))
            .addItem(nvrhi::BindingSetItem::Texture_UAV(
                0, m_working_texture, nvrhi::Format::UNKNOWN,
                nvrhi::TextureSubresourceSet(0, 1, 0, 1)))
            .addItem(nvrhi::BindingSetItem::Sampler(0, sampler_linear_clamp)),
        binding_layout);
  }
  for (int i = 1; i < mip_count; ++i) {
    auto& stage = m_stages.emplace_back();
    stage.m_width = glm::max(1u, source_desc.width >> i);
    stage.m_height = glm::max(1u, source_desc.height >> i);
    stage.m_pso = pso_downsample;
    stage.m_binding_set = m_backend->GetDevice()->createBindingSet(
        nvrhi::BindingSetDesc()
            .addItem(
                nvrhi::BindingSetItem::ConstantBuffer(0, m_constant_buffer))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(
                0, m_working_texture, nvrhi::Format::UNKNOWN,
                nvrhi::TextureSubresourceSet(glm::max(0, i - 1), 1, 0, 1)))
            .addItem(nvrhi::BindingSetItem::Texture_UAV(
                0, m_working_texture, nvrhi::Format::UNKNOWN,
                nvrhi::TextureSubresourceSet(i, 1, 0, 1)))
            .addItem(nvrhi::BindingSetItem::Sampler(0, sampler_linear_clamp)),
        binding_layout);
  }
  for (int i = mip_count - 2; i >= 1; --i) {
    auto& stage = m_stages.emplace_back();
    stage.m_width = glm::max(1u, source_desc.width >> i);
    stage.m_height = glm::max(1u, source_desc.height >> i);
    stage.m_pso = pso_upsample;
    stage.m_binding_set = m_backend->GetDevice()->createBindingSet(
        nvrhi::BindingSetDesc()
            .addItem(
                nvrhi::BindingSetItem::ConstantBuffer(0, m_constant_buffer))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(
                0, m_working_texture, nvrhi::Format::UNKNOWN,
                nvrhi::TextureSubresourceSet(i + 1, 1, 0, 1)))
            .addItem(nvrhi::BindingSetItem::Texture_UAV(
                0, m_working_texture, nvrhi::Format::UNKNOWN,
                nvrhi::TextureSubresourceSet(i, 1, 0, 1)))
            .addItem(nvrhi::BindingSetItem::Sampler(0, sampler_linear_clamp)),
        binding_layout);
  }
  {
    auto& stage = m_stages.emplace_back();
    stage.m_width = source_desc.width;
    stage.m_height = source_desc.height;
    stage.m_pso = pso_final;
    stage.m_binding_set = m_backend->GetDevice()->createBindingSet(
        nvrhi::BindingSetDesc()
            .addItem(
                nvrhi::BindingSetItem::ConstantBuffer(0, m_constant_buffer))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(
                0, m_working_texture, nvrhi::Format::UNKNOWN,
                nvrhi::TextureSubresourceSet(1, 1, 0, 1)))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(
                1, texture, nvrhi::Format::UNKNOWN,
                nvrhi::TextureSubresourceSet(0, 1, 0, 1)))
            .addItem(nvrhi::BindingSetItem::Texture_UAV(
                0, m_working_texture, nvrhi::Format::UNKNOWN,
                nvrhi::TextureSubresourceSet(0, 1, 0, 1)))
            .addItem(nvrhi::BindingSetItem::Sampler(0, sampler_linear_clamp)),
        binding_layout_final);
  }
}

void Bloom::Render(nvrhi::ICommandList* command_list) {
  command_list->beginMarker("Bloom");
  UpdateConstants(command_list);
  for (const auto& stage : m_stages) {
    command_list->setComputeState(nvrhi::ComputeState()
                                      .setPipeline(stage.m_pso)
                                      .addBindingSet(stage.m_binding_set));
    command_list->dispatch((stage.m_width + 7) / 8,
                           (stage.m_height + 7) / 8, 1);
  }
  command_list->endMarker();
}
