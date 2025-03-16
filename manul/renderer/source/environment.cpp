#include "environment.h"

#include <fmt/format.h>
#include <nvrhi/utils.h>

#include "gbuffer.h"
#include "nvrenderer/nvrenderer.h"
#include "nvrendererbackend.h"
#include "simulationenvironment.h"
#include "sky.h"

int MaEnvironment::GetCurrentSetIndex() {
  if (m_locked_set_index >= 0 && IsReady()) {
    m_current_set_index = std::exchange(m_locked_set_index, -1);
  }
  return m_current_set_index;
}

nvrhi::BindingSetHandle MaEnvironment::GetBindingSet(int pass, int mip, int set,
                                                     NvGbuffer* gbuffer_cube) {
  using namespace nvrhi;
  static std::map<int, BindingSetHandle> cache{};
  auto& handle =
      cache[((set & 0xf) << 28) | ((pass & 0xfff) << 16) | (mip & 0xffff)];
  if (!handle) {
    switch (pass) {
      case 1:
        if (!mip) {
          int previous_set = set - 1;
          while (previous_set < 0)
            previous_set += std::size(m_dynamic_envmap_diffuse);
          handle = m_backend->GetDevice()->createBindingSet(
              BindingSetDesc()
                  .addItem(BindingSetItem::Texture_SRV(0, m_skybox))
                  .addItem(BindingSetItem::Texture_SRV(
                      1, gbuffer_cube->m_gbuffer_diffuse, Format::UNKNOWN,
                      AllSubresources))
                  .addItem(BindingSetItem::Texture_SRV(
                      2, gbuffer_cube->m_gbuffer_normal, Format::UNKNOWN,
                      AllSubresources))
                  .addItem(BindingSetItem::Texture_SRV(
                      3, gbuffer_cube->m_gbuffer_params, Format::UNKNOWN,
                      AllSubresources))
                  .addItem(BindingSetItem::Texture_SRV(
                      4, gbuffer_cube->m_gbuffer_depth, Format::UNKNOWN,
                      AllSubresources))
                  .addItem(BindingSetItem::Texture_SRV(
                      5, m_dynamic_envmap_diffuse[previous_set],
                      Format::UNKNOWN, AllSubresources))
                  .addItem(BindingSetItem::Texture_SRV(13, m_sky_texture))
                  .addItem(BindingSetItem::Texture_SRV(14, m_aerial_lut))
                  .addItem(BindingSetItem::Texture_UAV(
                      0, m_dynamic_skybox[set], Format::UNKNOWN,
                      TextureSubresourceSet(
                          0, 1, 0, TextureSubresourceSet::AllArraySlices),
                      TextureDimension::Texture2DArray))
                  .addItem(BindingSetItem::Sampler(0, m_sampler_linear_clamp))
                  .addItem(BindingSetItem::Sampler(1, m_sampler_point_clamp))
                  .addItem(BindingSetItem::Sampler(13, m_sampler_linear_clamp))
                  .addItem(BindingSetItem::ConstantBuffer(
                      13, m_sky->m_sky_constants))
                  .addItem(BindingSetItem::ConstantBuffer(
                      1, m_face_inverse_projection_buffer))
                  .addItem(BindingSetItem::PushConstants(
                      0, sizeof(CubeLightingConstants))),
              m_bl_gbuffer_lighting);
        } else {
          handle = m_backend->GetDevice()->createBindingSet(
              BindingSetDesc()
                  .addItem(BindingSetItem::Texture_SRV(
                      1, m_dynamic_skybox[set], Format::UNKNOWN,
                      TextureSubresourceSet(
                          mip - 1, 1, 0,
                          TextureSubresourceSet::AllArraySlices)))
                  .addItem(BindingSetItem::Texture_UAV(
                      0, m_dynamic_skybox[set], Format::UNKNOWN,
                      TextureSubresourceSet(
                          mip, 1, 0, TextureSubresourceSet::AllArraySlices),
                      TextureDimension::Texture2DArray))
                  .addItem(BindingSetItem::Sampler(1, m_sampler_linear_clamp))
                  .addItem(BindingSetItem::PushConstants(
                      0, sizeof(EnvironmentFilterConstants))),
              m_bl_diffuse_ibl);
        }
        break;
      case 2:
        handle = m_backend->GetDevice()->createBindingSet(
            BindingSetDesc()
                .addItem(BindingSetItem::Texture_SRV(
                    1, m_dynamic_skybox[set], Format::UNKNOWN,
                    TextureSubresourceSet(
                        0, TextureSubresourceSet::AllMipLevels, 0,
                        TextureSubresourceSet::AllArraySlices)))
                .addItem(BindingSetItem::Texture_UAV(
                    0, m_dynamic_envmap_diffuse[set], Format::UNKNOWN,
                    AllSubresources, TextureDimension::Texture2DArray))
                .addItem(BindingSetItem::Sampler(1, m_sampler_linear_clamp))
                .addItem(BindingSetItem::PushConstants(
                    0, sizeof(EnvironmentFilterConstants))),
            m_bl_diffuse_ibl);
        break;
      case 3:
        handle = m_backend->GetDevice()->createBindingSet(
            BindingSetDesc()
                .addItem(BindingSetItem::Texture_SRV(
                    1, m_dynamic_skybox[set], Format::UNKNOWN,
                    TextureSubresourceSet(
                        0, TextureSubresourceSet::AllMipLevels, 0,
                        TextureSubresourceSet::AllArraySlices)))
                .addItem(BindingSetItem::Texture_UAV(
                    0, m_dynamic_envmap_specular[set], Format::UNKNOWN,
                    TextureSubresourceSet(
                        mip, 1, 0, TextureSubresourceSet::AllArraySlices),
                    TextureDimension::Texture2DArray))
                .addItem(BindingSetItem::Sampler(1, m_sampler_linear_clamp))
                .addItem(BindingSetItem::PushConstants(
                    0, sizeof(EnvironmentFilterConstants))),
            m_bl_diffuse_ibl);
        break;
    }
  }
  return handle;
}

MaEnvironment::MaEnvironment(NvRenderer* renderer)
    : MaResourceRegistry(renderer),
      m_backend(renderer->m_backend.get()),
      m_gbuffer(renderer->m_gbuffer.get()),
      m_sky(renderer->m_sky.get()) {}

void MaEnvironment::Init(const std::string& texture, float pre_exposure) {
  InitResourceRegistry();
  using namespace nvrhi;
  m_cs_sample_equirectangular = m_backend->CreateShader(
      "envmap_CSSampleEquirectangular", ShaderType::Compute);
  m_cs_diffuse_ibl =
      m_backend->CreateShader("envmap_CSDiffuseIBL", ShaderType::Compute);
  m_cs_specular_ibl =
      m_backend->CreateShader("envmap_CSSpecularIBL", ShaderType::Compute);
  m_cs_generate_mip =
      m_backend->CreateShader("envmap_CSGenerateCubeMip", ShaderType::Compute);
  m_cs_integrate_brdf =
      m_backend->CreateShader("envmap_CSIntegrateBRDF", ShaderType::Compute);
  m_cs_gbuffer_lighting =
      m_backend->CreateShader("gbuffer_cube_lighting", ShaderType::Compute);

  m_face_inverse_projection_buffer = m_backend->GetDevice()->createBuffer(
      nvrhi::utils::CreateStaticConstantBufferDesc(
          sizeof(glm::mat4), "Inverse Face Projection Buffer")
          .setInitialState(nvrhi::ResourceStates::ConstantBuffer)
          .setKeepInitialState(true));

  {
    auto command_list = m_backend->GetDevice()->createCommandList();
    glm::mat4 inverse_projection = glm::inverse(
        glm::perspectiveFovRH_ZO(glm::radians(90.f), 1.f, 1.f, .1f,
                                 Global.reflectiontune.range_instances));
    command_list->open();
    command_list->writeBuffer(m_face_inverse_projection_buffer,
                              &inverse_projection, sizeof(inverse_projection));
    command_list->close();
    m_backend->GetDevice()->executeCommandList(command_list);
  }

  m_bl_gbuffer_lighting = m_backend->GetDevice()->createBindingLayout(
      BindingLayoutDesc()
          .setVisibility(ShaderType::Compute)
          .addItem(BindingLayoutItem::Texture_SRV(0))
          .addItem(BindingLayoutItem::Texture_SRV(1))
          .addItem(BindingLayoutItem::Texture_SRV(2))
          .addItem(BindingLayoutItem::Texture_SRV(3))
          .addItem(BindingLayoutItem::Texture_SRV(4))
          .addItem(BindingLayoutItem::Texture_SRV(5))
          .addItem(BindingLayoutItem::Texture_SRV(13))
          .addItem(BindingLayoutItem::Texture_SRV(14))
          .addItem(BindingLayoutItem::Texture_UAV(0))
          .addItem(BindingLayoutItem::Sampler(0))
          .addItem(BindingLayoutItem::Sampler(1))
          .addItem(BindingLayoutItem::Sampler(13))
          .addItem(BindingLayoutItem::VolatileConstantBuffer(13))
          .addItem(BindingLayoutItem::ConstantBuffer(1))
          .addItem(BindingLayoutItem::PushConstants(
              0, sizeof(CubeLightingConstants))));
  m_pso_gbuffer_lighting = m_backend->GetDevice()->createComputePipeline(
      ComputePipelineDesc()
          .addBindingLayout(m_bl_gbuffer_lighting)
          .setComputeShader(m_cs_gbuffer_lighting));

  m_bl_sample_equirectangular = m_backend->GetDevice()->createBindingLayout(
      BindingLayoutDesc()
          .setVisibility(ShaderType::Compute)
          .addItem(BindingLayoutItem::Texture_SRV(0))
          .addItem(BindingLayoutItem::Texture_UAV(0))
          .addItem(BindingLayoutItem::Sampler(0))
          .addItem(BindingLayoutItem::PushConstants(
              0, sizeof(EnvironmentFilterConstants))));
  m_pso_sample_equirectangular = m_backend->GetDevice()->createComputePipeline(
      ComputePipelineDesc()
          .addBindingLayout(m_bl_sample_equirectangular)
          .setComputeShader(m_cs_sample_equirectangular));

  m_bl_diffuse_ibl = m_backend->GetDevice()->createBindingLayout(
      BindingLayoutDesc()
          .setVisibility(ShaderType::Compute)
          .addItem(BindingLayoutItem::Texture_SRV(1))
          .addItem(BindingLayoutItem::Texture_UAV(0))
          .addItem(BindingLayoutItem::Sampler(1))
          .addItem(BindingLayoutItem::PushConstants(
              0, sizeof(EnvironmentFilterConstants))));
  m_pso_diffuse_ibl = m_backend->GetDevice()->createComputePipeline(
      ComputePipelineDesc()
          .addBindingLayout(m_bl_diffuse_ibl)
          .setComputeShader(m_cs_diffuse_ibl));

  m_pso_generate_mip = m_backend->GetDevice()->createComputePipeline(
      ComputePipelineDesc()
          .addBindingLayout(m_bl_diffuse_ibl)
          .setComputeShader(m_cs_generate_mip));

  m_pso_specular_ibl = m_backend->GetDevice()->createComputePipeline(
      ComputePipelineDesc()
          .addBindingLayout(m_bl_diffuse_ibl)
          .setComputeShader(m_cs_specular_ibl));

  m_bl_integrate_brdf = m_backend->GetDevice()->createBindingLayout(
      BindingLayoutDesc()
          .setVisibility(ShaderType::Compute)
          .addItem(BindingLayoutItem::Texture_UAV(1))
          .addItem(BindingLayoutItem::PushConstants(
              0, sizeof(EnvironmentFilterConstants))));

  m_pso_integrate_brdf = m_backend->GetDevice()->createComputePipeline(
      ComputePipelineDesc()
          .addBindingLayout(m_bl_integrate_brdf)
          .setComputeShader(m_cs_integrate_brdf));

  int skybox_width = 2048;
  int dynamic_skybox_width = 512;
  int skybox_mips = static_cast<int>(floor(log2(skybox_width))) + 1;
  int dynamic_skybox_mips =
      static_cast<int>(floor(log2(dynamic_skybox_width))) + 1;
  int diffuse_width = 64;
  int specular_width = 1024;
  int dynamic_specular_width = 256;
  int specular_mips = 6;

  m_skybox = m_backend->GetDevice()->createTexture(
      TextureDesc()
          .setDimension(TextureDimension::TextureCube)
          .setFormat(Format::R11G11B10_FLOAT)
          .setArraySize(6)
          .setWidth(skybox_width)
          .setHeight(skybox_width)
          .setMipLevels(skybox_mips)
          .setIsUAV(true)
          .setInitialState(ResourceStates::ShaderResource)
          .setKeepInitialState(true)
          .setDebugName("Skybox"));

  m_dynamic_skybox[0] = m_backend->GetDevice()->createTexture(
      TextureDesc()
          .setDimension(TextureDimension::TextureCube)
          .setFormat(Format::R11G11B10_FLOAT)
          .setArraySize(6)
          .setWidth(dynamic_skybox_width)
          .setHeight(dynamic_skybox_width)
          .setMipLevels(dynamic_skybox_mips)
          .setIsUAV(true)
          .setInitialState(ResourceStates::UnorderedAccess)
          .setKeepInitialState(true)
          .setDebugName("Dynamic Skybox"));

  m_dynamic_skybox[1] = m_backend->GetDevice()->createTexture(
      TextureDesc()
          .setDimension(TextureDimension::TextureCube)
          .setFormat(Format::R11G11B10_FLOAT)
          .setArraySize(6)
          .setWidth(dynamic_skybox_width)
          .setHeight(dynamic_skybox_width)
          .setMipLevels(dynamic_skybox_mips)
          .setIsUAV(true)
          .setInitialState(ResourceStates::UnorderedAccess)
          .setKeepInitialState(true)
          .setDebugName("Dynamic Skybox"));

  m_sampler_linear_clamp_v_repeat_h = m_backend->GetDevice()->createSampler(
      SamplerDesc()
          .setAllAddressModes(SamplerAddressMode::Clamp)
          .setAddressU(SamplerAddressMode::Repeat)
          .setAllFilters(true));

  m_sampler_linear_clamp = m_backend->GetDevice()->createSampler(
      SamplerDesc()
          .setAllAddressModes(SamplerAddressMode::Clamp)
          .setAllFilters(true));

  m_sampler_point_clamp = m_backend->GetDevice()->createSampler(
      SamplerDesc()
          .setAllAddressModes(SamplerAddressMode::Clamp)
          .setAllFilters(false));
  CommandListHandle command_list = m_backend->GetDevice()->createCommandList();
  command_list->open();

  RegisterResource(true, "sampler_linear_clamp_v_repeat_h",
                   m_sampler_linear_clamp_v_repeat_h, ResourceType::Sampler);

  {
    m_brdf_lut = m_backend->GetDevice()->createTexture(
        TextureDesc()
            .setFormat(Format::RG16_FLOAT)
            .setDimension(TextureDimension::Texture2D)
            .setWidth(512)
            .setHeight(512)
            .setMipLevels(1)
            .setIsUAV(true)
            .setInitialState(ResourceStates::ShaderResource)
            .setKeepInitialState(true)
            .setDebugName("BRDF LUT"));

    RegisterResource(true, "env_brdf_lut", m_brdf_lut,
                     ResourceType::Texture_SRV);

    BindingSetHandle binding_set = m_backend->GetDevice()->createBindingSet(
        BindingSetDesc()
            .addItem(BindingSetItem::Texture_UAV(1, m_brdf_lut))
            .addItem(BindingSetItem::PushConstants(
                0, sizeof(EnvironmentFilterConstants))),
        m_bl_integrate_brdf);

    ComputeState compute_state = ComputeState()
                                     .setPipeline(m_pso_integrate_brdf)
                                     .addBindingSet(binding_set);

    int target_width = m_brdf_lut->getDesc().width;
    int target_height = m_brdf_lut->getDesc().height;
    command_list->setComputeState(compute_state);

    EnvironmentFilterConstants filter_constants{};
    filter_constants.m_sample_count = 4096;
    command_list->setPushConstants(&filter_constants, sizeof(filter_constants));

    command_list->dispatch((target_width + 31) / 32, (target_height + 31) / 32,
                           1);
  }

  {
    m_envmap_diffuse = m_backend->GetDevice()->createTexture(
        TextureDesc()
            .setArraySize(6)
            .setFormat(Format::R11G11B10_FLOAT)
            .setDimension(TextureDimension::TextureCube)
            .setWidth(diffuse_width)
            .setHeight(diffuse_width)
            .setMipLevels(1)
            .setIsUAV(true)
            .setInitialState(ResourceStates::ShaderResource)
            .setKeepInitialState(true)
            .setDebugName("Diffuse IBL"));
    for (int i = 0; i < std::size(m_dynamic_envmap_diffuse); ++i) {
      auto debug_name = fmt::format("Dynamic Diffuse IBL[{}]", i);
      auto resource_name = fmt::format("env_dynamic_diffuse_{}", i);
      m_dynamic_envmap_diffuse[i] = m_backend->GetDevice()->createTexture(
          TextureDesc()
              .setArraySize(6)
              .setFormat(Format::R11G11B10_FLOAT)
              .setDimension(TextureDimension::TextureCube)
              .setWidth(diffuse_width)
              .setHeight(diffuse_width)
              .setMipLevels(1)
              .setIsUAV(true)
              .setInitialState(i ? ResourceStates::UnorderedAccess
                                 : ResourceStates::ShaderResource)
              .setKeepInitialState(true)
              .setDebugName(debug_name.c_str()));
    }
    RegisterResource(true, "env_dynamic_diffuse", m_dynamic_envmap_diffuse[0],
                     ResourceType::Texture_SRV);
  }

  {
    m_envmap_specular = m_backend->GetDevice()->createTexture(
        TextureDesc()
            .setArraySize(6)
            .setFormat(Format::R11G11B10_FLOAT)
            .setDimension(TextureDimension::TextureCube)
            .setWidth(specular_width)
            .setHeight(specular_width)
            .setMipLevels(specular_mips)
            .setIsUAV(true)
            .setInitialState(ResourceStates::ShaderResource)
            .setKeepInitialState(true)
            .setDebugName("Specular IBL"));
    for (int i = 0; i < std::size(m_dynamic_envmap_specular); ++i) {
      auto debug_name = fmt::format("Dynamic Specular IBL[{}]", i);
      auto resource_name = fmt::format("env_dynamic_specular_{}", i);
      m_dynamic_envmap_specular[i] = m_backend->GetDevice()->createTexture(
          TextureDesc()
              .setArraySize(6)
              .setFormat(Format::R11G11B10_FLOAT)
              .setDimension(TextureDimension::TextureCube)
              .setWidth(dynamic_specular_width)
              .setHeight(dynamic_specular_width)
              .setMipLevels(specular_mips)
              .setIsUAV(true)
              .setInitialState(i ? ResourceStates::UnorderedAccess
                                 : ResourceStates::ShaderResource)
              .setKeepInitialState(true)
              .setDebugName(debug_name.c_str()));
    }
    RegisterResource(true, "env_dynamic_specular", m_dynamic_envmap_specular[0],
                     ResourceType::Texture_SRV);
  }

  m_sky_texture = m_backend->GetDevice()->createTexture(
      TextureDesc(m_sky->m_aerial_lut->m_sky_texture->getDesc())
          .setDebugName("Envmap Sky Texture"));

  m_aerial_lut = m_backend->GetDevice()->createTexture(
      TextureDesc(m_sky->m_aerial_lut->m_lut->getDesc())
          .setDebugName("Envmap Aerial LUT"));

  command_list->close();
  m_backend->GetDevice()->executeCommandList(command_list);

  m_environment_pass = std::make_shared<EnvironmentRenderPass>(this);
  m_environment_pass->Init();
}

void MaEnvironment::Render(nvrhi::ICommandList* command_list,
                           glm::dmat4 const& jitter,
                           glm::dmat4 const& projection, glm::dmat4 const& view,
                           glm::dmat4 const& previous_view_projection) {
  m_environment_pass->UpdateConstants(command_list, jitter, projection, view,
                                      previous_view_projection);
  m_environment_pass->Render(command_list);
}

void MaEnvironment::Filter(uint64_t sky_instance_id, NvGbuffer* gbuffer_cube) {
  using namespace nvrhi;
  int pixels_per_pass_sqr = m_pixels_per_pass * m_pixels_per_pass;
  int target_set_index = 1;
  int source_set_index = 0;

  static uint64_t prev_instance = -1;
  if (filter_step == 0) {
    if (!IsReady()) {
      return;
    }
    // m_locked_set_index = (GetCurrentSetIndex() + 1) % 2;
    m_ready_handle = m_backend->GetDevice()->createEventQuery();

    // CommandListHandle transition_command_list =
    //     m_backend->GetDevice()->createCommandList();
    // transition_command_list->open();
    //
    // transition_command_list->setTextureState(m_sky->m_aerial_lut->m_sky_texture,
    //                                         nvrhi::AllSubresources,
    //                                         nvrhi::ResourceStates::Common);
    // transition_command_list->setTextureState(m_sky->m_aerial_lut->m_lut,
    //                                         nvrhi::AllSubresources,
    //                                         nvrhi::ResourceStates::Common);
    //
    // transition_command_list->close();
    // auto transition_id =
    //    m_backend->GetDevice()->executeCommandList(transition_command_list);
    //
    // m_backend->GetDevice()->queueWaitForCommandList(
    //    nvrhi::CommandQueue::Copy, nvrhi::CommandQueue::Graphics,
    //    transition_id);

    CommandListHandle command_list = m_backend->GetDevice()->createCommandList(
        CommandListParameters()
            .setQueueType(CommandQueue::Graphics)
            .setEnableImmediateExecution(false));
    command_list->open();

    {
      auto diff_desc = m_sky_texture->getDesc();
      for (int i = 0; i < diff_desc.arraySize; ++i) {
        for (int j = 0; j < diff_desc.mipLevels; ++j) {
          auto slice = nvrhi::TextureSlice()
                           .setArraySlice(i)
                           .setMipLevel(j)
                           .setWidth(-1)
                           .setHeight(-1)
                           .resolve(diff_desc);
          command_list->copyTexture(m_sky_texture, slice,
                                    m_sky->m_aerial_lut->m_sky_texture, slice);
        }
      }
    }

    {
      auto diff_desc = m_aerial_lut->getDesc();
      for (int i = 0; i < diff_desc.arraySize; ++i) {
        for (int j = 0; j < diff_desc.mipLevels; ++j) {
          auto slice = nvrhi::TextureSlice()
                           .setArraySlice(i)
                           .setMipLevel(j)
                           .setWidth(-1)
                           .setHeight(-1)
                           .resolve(diff_desc);
          command_list->copyTexture(m_aerial_lut, slice,
                                    m_sky->m_aerial_lut->m_lut, slice);
        }
      }
    }

    // auto diff_desc = m_dynamic_envmap_diffuse[0]->getDesc();
    // for (int i = 0; i < 6; ++i) {
    //   for (int j = 0; j < diff_desc.mipLevels; ++j) {
    //     auto slice = nvrhi::TextureSlice()
    //                      .setArraySlice(i)
    //                      .setMipLevel(j)
    //                      .setWidth(-1)
    //                      .setHeight(-1)
    //                      .resolve(diff_desc);
    //     command_list->copyTexture(m_dynamic_envmap_diffuse[0], slice,
    //                               m_dynamic_envmap_diffuse[1], slice);
    //   }
    // }
    //
    // auto spec_desc = m_dynamic_envmap_specular[0]->getDesc();
    // for (int i = 0; i < 6; ++i) {
    //  for (int j = 0; j < spec_desc.mipLevels; ++j) {
    //    auto slice = nvrhi::TextureSlice()
    //                     .setArraySlice(i)
    //                     .setMipLevel(j)
    //                     .setWidth(-1)
    //                     .setHeight(-1)
    //                     .resolve(spec_desc);
    //    command_list->copyTexture(m_dynamic_envmap_specular[0], slice,
    //                              m_dynamic_envmap_specular[1], slice);
    //  }
    //}

    // command_list->setTextureState(m_sky_texture, nvrhi::AllSubresources,
    //                               nvrhi::ResourceStates::Common);
    // command_list->setTextureState(m_aerial_lut, nvrhi::AllSubresources,
    //                               nvrhi::ResourceStates::Common);
    command_list->close();
    prev_instance = m_backend->GetDevice()->executeCommandList(
        command_list, nvrhi::CommandQueue::Graphics);
    // CommandListHandle transition_command_list =
    //     m_backend->GetDevice()->createCommandList();
    // transition_command_list->open();
    //
    // transition_command_list->setTextureState(m_skybox, AllSubresources,
    //                                         ResourceStates::UnorderedAccess);
    // transition_command_list->setTextureState(m_dynamic_skybox[target_set_index],
    //                                         AllSubresources,
    //                                         ResourceStates::UnorderedAccess);
    // transition_command_list->setTextureState(
    //    m_dynamic_envmap_diffuse[target_set_index], AllSubresources,
    //    ResourceStates::UnorderedAccess);
    // transition_command_list->setTextureState(
    //    m_dynamic_envmap_specular[target_set_index], AllSubresources,
    //    ResourceStates::UnorderedAccess);
    // transition_command_list->setTextureState(
    //    m_dynamic_envmap_diffuse[source_set_index], AllSubresources,
    //    ResourceStates::ShaderResource);
    // transition_command_list->setTextureState(
    //    m_dynamic_envmap_specular[source_set_index], AllSubresources,
    //    ResourceStates::ShaderResource);
    // transition_command_list->setTextureState(gbuffer_cube->m_gbuffer_diffuse,
    //                                         AllSubresources,
    //                                         ResourceStates::ShaderResource);
    // transition_command_list->setTextureState(gbuffer_cube->m_gbuffer_normal,
    //                                         AllSubresources,
    //                                         ResourceStates::ShaderResource);
    // transition_command_list->setTextureState(gbuffer_cube->m_gbuffer_params,
    //                                         AllSubresources,
    //                                         ResourceStates::ShaderResource);
    // transition_command_list->setTextureState(gbuffer_cube->m_gbuffer_depth,
    //                                         AllSubresources,
    //                                         ResourceStates::ShaderResource);
    // transition_command_list->commitBarriers();
    //
    // transition_command_list->close();
    // auto instance =
    //    m_backend->GetDevice()->executeCommandList(transition_command_list);
    // m_backend->GetDevice()->queueWaitForCommandList(
    //    nvrhi::CommandQueue::Compute, nvrhi::CommandQueue::Graphics,
    //    instance);
    ++filter_step;
    mip = 0;
    return;
  }

  if (filter_step == 1) {
    // CommandListHandle command_list_transition =
    //     m_backend->GetDevice()->createCommandList();
    // command_list_transition->open();
    //
    // command_list_transition->setTextureState(
    //    m_sky->m_aerial_lut->m_sky_texture, nvrhi::AllSubresources,
    //    nvrhi::ResourceStates::ShaderResource);
    //
    // command_list_transition->close();
    // auto instance =
    //    m_backend->GetDevice()->executeCommandList(command_list_transition);

    if (prev_instance != -1) {
      // m_backend->GetDevice()->queueWaitForCommandList(
      //     nvrhi::CommandQueue::Compute, nvrhi::CommandQueue::Graphics,
      //     prev_instance);
      // prev_instance = -1;
    }
    CommandListHandle command_list = m_backend->GetDevice()->createCommandList(
        CommandListParameters()
            .setQueueType(CommandQueue::Graphics)
            .setEnableImmediateExecution(false));
    // m_backend->GetDevice()->queueWaitForCommandList(
    //     nvrhi::CommandQueue::Compute, nvrhi::CommandQueue::Graphics,
    //     instance);
    command_list->open();

    while (true) {
      int target_width = std::max(
          m_dynamic_skybox[target_set_index]->getDesc().width >> mip, 1u);
      int target_height = std::max(
          m_dynamic_skybox[target_set_index]->getDesc().height >> mip, 1u);

      int pixels_x = std::min(m_pixels_per_pass, target_width - x);
      int pixels_y = std::min(m_pixels_per_pass, target_height - y);

      if (pixels_x * pixels_y > pixels_per_pass_sqr) {
        break;
      }

      pixels_per_pass_sqr -= pixels_x * pixels_y;

      if (mip >= m_dynamic_skybox[target_set_index]->getDesc().mipLevels) {
        break;
      }
      if (!mip) m_sky->UpdateConstants(command_list);
      ComputeState compute_state;
      compute_state
          .setPipeline(mip ? m_pso_generate_mip : m_pso_gbuffer_lighting)
          .addBindingSet(
              GetBindingSet(filter_step, mip, target_set_index, gbuffer_cube));
      command_list->setComputeState(compute_state);

      if (!mip) {
        CubeLightingConstants filter_constants{};
        const auto& daylight = Global.DayLight;
        filter_constants.m_offset_x = x;
        filter_constants.m_offset_y = y;
        filter_constants.m_offset_z = 0;
        filter_constants.m_light_direction = -daylight.direction;
        filter_constants.m_height = Global.pCamera.Pos.y;

        m_sky->CalcLighting(filter_constants.m_light_direction,
                            filter_constants.m_light_color);

        filter_constants.m_light_color = glm::vec4(m_sky->CalcSunColor(), 1.f);
        command_list->setPushConstants(&filter_constants,
                                       sizeof(filter_constants));
      } else {
        EnvironmentFilterConstants filter_constants{};
        filter_constants.m_offset_x = x;
        filter_constants.m_offset_y = y;
        filter_constants.m_offset_z = 0;
        command_list->setPushConstants(&filter_constants,
                                       sizeof(filter_constants));
      }

      command_list->dispatch((pixels_x + 31) / 32, (pixels_y + 31) / 32, 6);
      x += m_pixels_per_pass;
      if (x >= target_width) {
        x = 0;
        y += m_pixels_per_pass;
      }
      if (y >= target_height) {
        y = 0;
        ++mip;
      }
    }

    if (mip >= m_dynamic_skybox[target_set_index]->getDesc().mipLevels) {
      mip = 0;
      ++filter_step;
    }

    command_list->close();
    m_backend->GetDevice()->executeCommandList(command_list,
                                               nvrhi::CommandQueue::Graphics);
  }

  if (filter_step == 2) {
    CommandListHandle command_list = m_backend->GetDevice()->createCommandList(
        CommandListParameters()
            .setQueueType(CommandQueue::Graphics)
            .setEnableImmediateExecution(false));
    command_list->open();

    ComputeState compute_state =
        ComputeState()
            .setPipeline(m_pso_diffuse_ibl)
            .addBindingSet(
                GetBindingSet(filter_step, 0, target_set_index, gbuffer_cube));

    int target_width =
        m_dynamic_envmap_diffuse[target_set_index]->getDesc().width;
    int target_height =
        m_dynamic_envmap_diffuse[target_set_index]->getDesc().height;
    command_list->setComputeState(compute_state);

    EnvironmentFilterConstants filter_constants{};
    filter_constants.m_sample_count = 32;
    filter_constants.m_source_width =
        m_dynamic_skybox[target_set_index]->getDesc().width;
    filter_constants.m_offset_x = 0;
    filter_constants.m_offset_y = 0;
    filter_constants.m_offset_z = 0;
    filter_constants.m_mip_bias = 1.;
    command_list->setPushConstants(&filter_constants, sizeof(filter_constants));

    command_list->dispatch((target_width + 31) / 32, (target_height + 31) / 32,
                           6);
    command_list->close();
    m_backend->GetDevice()->executeCommandList(command_list,
                                               nvrhi::CommandQueue::Graphics);
    ++filter_step;
  }

  if (filter_step == 3) {
    CommandListHandle command_list = m_backend->GetDevice()->createCommandList(
        CommandListParameters()
            .setQueueType(CommandQueue::Graphics)
            .setEnableImmediateExecution(false));
    command_list->open();

    while (true) {
      int target_width = std::max(
          1u,
          m_dynamic_envmap_specular[target_set_index]->getDesc().width >> mip);
      int target_height = std::max(
          1u,
          m_dynamic_envmap_specular[target_set_index]->getDesc().height >> mip);

      int pixels_x = std::min(m_pixels_per_pass, target_width - x);
      int pixels_y = std::min(m_pixels_per_pass, target_height - y);

      if (pixels_x * pixels_y > pixels_per_pass_sqr) {
        break;
      }

      if (mip >=
          m_dynamic_envmap_specular[target_set_index]->getDesc().mipLevels) {
        break;
      }

      pixels_per_pass_sqr -= pixels_x * pixels_y;

      ComputeState compute_state =
          ComputeState()
              .setPipeline(m_pso_specular_ibl)
              .addBindingSet(GetBindingSet(filter_step, mip, target_set_index,
                                           gbuffer_cube));

      EnvironmentFilterConstants filter_constants{};
      filter_constants.m_sample_count = 32;
      filter_constants.m_roughness =
          glm::pow(static_cast<double>(mip) /
                       (m_dynamic_envmap_specular[target_set_index]
                            ->getDesc()
                            .mipLevels -
                        1.),
                   2.);
      filter_constants.m_source_width =
          m_dynamic_skybox[target_set_index]->getDesc().width;
      filter_constants.m_mip_bias =
          mip ? 1.
              : log2(target_width /
                     static_cast<double>(filter_constants.m_source_width));
      filter_constants.m_offset_x = x;
      filter_constants.m_offset_y = y;
      filter_constants.m_offset_z = 0;
      command_list->setComputeState(compute_state);
      command_list->setPushConstants(&filter_constants,
                                     sizeof(filter_constants));

      command_list->dispatch((pixels_x + 31) / 32, (pixels_y + 31) / 32, 6);

      x += m_pixels_per_pass;
      if (x >= target_width) {
        x = 0;
        y += m_pixels_per_pass;
      }
      if (y >= target_height) {
        y = 0;
        ++mip;
      }
    }
    command_list->close();
    m_backend->GetDevice()->executeCommandList(command_list,
                                               nvrhi::CommandQueue::Graphics);
    if (mip >=
        m_dynamic_envmap_specular[target_set_index]->getDesc().mipLevels) {
      mip = 0;
      filter_step = 4;
    }
  }

  if (filter_step == 4) {
    CommandListHandle command_list = m_backend->GetDevice()->createCommandList(
        CommandListParameters()
            .setQueueType(CommandQueue::Graphics)
            .setEnableImmediateExecution(false));
    command_list->open();

    auto diff_desc = m_dynamic_envmap_diffuse[0]->getDesc();
    for (int i = 0; i < 6; ++i) {
      for (int j = 0; j < diff_desc.mipLevels; ++j) {
        auto slice = nvrhi::TextureSlice()
                         .setArraySlice(i)
                         .setMipLevel(j)
                         .setWidth(-1)
                         .setHeight(-1)
                         .resolve(diff_desc);
        command_list->copyTexture(m_dynamic_envmap_diffuse[0], slice,
                                  m_dynamic_envmap_diffuse[1], slice);
      }
    }

    auto spec_desc = m_dynamic_envmap_specular[0]->getDesc();
    for (int i = 0; i < 6; ++i) {
      for (int j = 0; j < spec_desc.mipLevels; ++j) {
        auto slice = nvrhi::TextureSlice()
                         .setArraySlice(i)
                         .setMipLevel(j)
                         .setWidth(-1)
                         .setHeight(-1)
                         .resolve(spec_desc);
        command_list->copyTexture(m_dynamic_envmap_specular[0], slice,
                                  m_dynamic_envmap_specular[1], slice);
      }
    }

    command_list->close();
    m_backend->GetDevice()->executeCommandList(command_list,
                                               nvrhi::CommandQueue::Graphics);
    filter_step = 0;
    m_backend->GetDevice()->setEventQuery(m_ready_handle,
                                          nvrhi::CommandQueue::Graphics);
  }
}

bool MaEnvironment::IsReady() const {
  return !m_ready_handle ||
         m_backend->GetDevice()->pollEventQuery(m_ready_handle);
}

EnvironmentRenderPass::EnvironmentRenderPass(MaEnvironment* environment)
    : FullScreenPass(environment->m_backend),
      m_environment(environment),
      m_gbuffer(environment->m_gbuffer),
      m_skybox_texture(environment->m_skybox) {}

void EnvironmentRenderPass::Init() {
  m_environment_constants = m_backend->GetDevice()->createBuffer(
      nvrhi::utils::CreateVolatileConstantBufferDesc(
          sizeof(EnvironmentConstants), "Skybox Draw Constants", 16)
          .setInitialState(nvrhi::ResourceStates::ConstantBuffer)
          .setKeepInitialState(true));
  m_binding_layout = m_backend->GetDevice()->createBindingLayout(
      nvrhi::BindingLayoutDesc()
          .setVisibility(nvrhi::ShaderType::Pixel)
          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(0))
          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(13))
          .addItem(nvrhi::BindingLayoutItem::Sampler(0))
          .addItem(nvrhi::BindingLayoutItem::Sampler(13))
          .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0)));
  auto sampler = m_backend->GetDevice()->createSampler(
      nvrhi::SamplerDesc()
          .setAllAddressModes(nvrhi::SamplerAddressMode::Clamp)
          .setAllFilters(true));
  for (int i = 0; i < 2; ++i) {
    m_binding_set[i] = m_backend->GetDevice()->createBindingSet(
        nvrhi::BindingSetDesc()
            .addItem(
                nvrhi::BindingSetItem::Texture_SRV(0, m_environment->m_skybox))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(
                13, m_environment->m_sky->m_aerial_lut->m_sky_texture))
            .addItem(nvrhi::BindingSetItem::Sampler(
                0, m_environment->m_sampler_linear_clamp))
            .addItem(nvrhi::BindingSetItem::Sampler(
                13, m_environment->m_sampler_linear_clamp_v_repeat_h))
            .addItem(nvrhi::BindingSetItem::ConstantBuffer(
                0, m_environment_constants)),
        m_binding_layout);
  }

  m_pixel_shader = m_backend->CreateShader("skybox", nvrhi::ShaderType::Pixel);
  FullScreenPass::Init();
}

void EnvironmentRenderPass::CreatePipelineDesc(
    nvrhi::GraphicsPipelineDesc& pipeline_desc) {
  FullScreenPass::CreatePipelineDesc(pipeline_desc);
  pipeline_desc.addBindingLayout(m_binding_layout);
  pipeline_desc.setPixelShader(m_pixel_shader);
  pipeline_desc.renderState.setDepthStencilState(
      nvrhi::DepthStencilState()
          .enableDepthTest()
          .disableDepthWrite()
          .setDepthFunc(nvrhi::ComparisonFunc::GreaterOrEqual)
          .disableStencil());
}

void EnvironmentRenderPass::UpdateConstants(
    nvrhi::ICommandList* command_list, glm::dmat4 const& jitter,
    glm::dmat4 const& projection, glm::dmat4 const& view,
    glm::dmat4 const& previous_view_projection) {
  EnvironmentConstants constants{};

  constants.m_reproject_matrix =
      glm::inverse(projection * view) * previous_view_projection;
  constants.m_inverse_view_projection =
      glm::inverse(jitter * projection * glm::dmat4(glm::dmat3(view)));
  constants.m_sun_direction = simulation::Environment.sun().getDirection();
  constants.m_moon_direction = simulation::Environment.moon().getDirection();
  constants.m_height = Global.pCamera.Pos.y;

  command_list->writeBuffer(m_environment_constants, &constants,
                            sizeof(constants));
}

void EnvironmentRenderPass::Render(nvrhi::ICommandList* command_list) {
  nvrhi::GraphicsState graphics_state;
  InitState(graphics_state);
  graphics_state.addBindingSet(
      m_binding_set[m_environment->GetCurrentSetIndex()]);
  command_list->setGraphicsState(graphics_state);
  Draw(command_list);
}

nvrhi::IFramebuffer* EnvironmentRenderPass::GetFramebuffer() {
  return m_gbuffer->m_framebuffer;
}
