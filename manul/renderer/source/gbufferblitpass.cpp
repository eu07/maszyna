#include "gbufferblitpass.h"

#include <chrono>
#include <Timer.h>
#include <nvrhi/utils.h>

#include "contactshadows.h"
#include "csm.h"
#include "environment.h"
#include "gbuffer.h"
#include "nvrendererbackend.h"
#include "sky.h"
#include "ssao.h"

GbufferBlitPass::GbufferBlitPass(NvRenderer* renderer, NvGbuffer* gbuffer,
                                 NvGbuffer* gbuffer_shadow, NvSsao* ssao,
                                 MaEnvironment* environment,
                                 MaShadowMap* shadow_map,
                                 MaContactShadows* contact_shadows, Sky* sky)
    : FullScreenPass(renderer->GetBackend()),
      MaResourceRegistry(renderer),
      m_gbuffer(gbuffer),
      m_gbuffer_shadow(gbuffer_shadow),
      m_ssao(ssao),
      m_environment(environment),
      m_shadow_map(shadow_map),
      m_contact_shadows(contact_shadows),
      m_sky(sky) {}

void GbufferBlitPass::Init() {
  InitResourceRegistry();
  m_draw_constants = m_backend->GetDevice()->createBuffer(
      nvrhi::utils::CreateVolatileConstantBufferDesc(
          sizeof(DrawConstants), "GBuffer Lighting Constants", 16)
          .setInitialState(nvrhi::ResourceStates::ConstantBuffer)
          .setKeepInitialState(true));
  RegisterResource(true, "gbuffer_lighting_constants", m_draw_constants,
                   nvrhi::ResourceType::VolatileConstantBuffer);
  const nvrhi::SamplerHandle sampler_shadow_comp =
      m_backend->GetDevice()->createSampler(
          nvrhi::SamplerDesc()
              .setReductionType(nvrhi::SamplerReductionType::Comparison)
              .setComparisonFunc(nvrhi::ComparisonFunc::Greater)
              .setAllAddressModes(nvrhi::SamplerAddressMode::ClampToEdge)
              .setAllFilters(true));
  const nvrhi::SamplerHandle sampler_linear =
      m_backend->GetDevice()->createSampler(
          nvrhi::SamplerDesc().setAllFilters(true));
  const nvrhi::SamplerHandle sampler_linear_clamp =
      m_backend->GetDevice()->createSampler(
          nvrhi::SamplerDesc()
              .setAllAddressModes(nvrhi::SamplerAddressMode::ClampToEdge)
              .setAllFilters(true));
  RegisterResource(true, "shadow_sampler_comp", sampler_shadow_comp,
                   nvrhi::ResourceType::Sampler);
  RegisterResource(true, "sampler_linear_wrap", sampler_linear,
                   nvrhi::ResourceType::Sampler);
  RegisterResource(true, "sampler_linear_clamp", sampler_linear_clamp,
                   nvrhi::ResourceType::Sampler);
  m_scene_depth = m_backend->GetDevice()->createTexture(
      nvrhi::TextureDesc(m_gbuffer->m_gbuffer_depth->getDesc())
          .setFormat(nvrhi::Format::R32_FLOAT)
          .setDebugName("Scene Depth")
          .setInitialState(nvrhi::ResourceStates::ShaderResource)
          .setUseClearValue(false)
          .setIsRenderTarget(false));
  RegisterResource(true, "gbuffer_depth", m_scene_depth,
                   nvrhi::ResourceType::Texture_SRV);
  m_output = m_backend->GetDevice()->createTexture(
      nvrhi::TextureDesc()
          .setWidth(m_gbuffer->m_framebuffer->getFramebufferInfo().width)
          .setHeight(m_gbuffer->m_framebuffer->getFramebufferInfo().height)
          .setFormat(nvrhi::Format::RGBA16_FLOAT)
          .setIsUAV(true)
          .setIsRenderTarget(true)
          .setInitialState(nvrhi::ResourceStates::ShaderResource)
          .setKeepInitialState(true));
  RegisterResource(true, "scene_lit_texture", m_output,
                   nvrhi::ResourceType::Texture_SRV);
  m_binding_layout = m_backend->GetDevice()->createBindingLayout(
      nvrhi::BindingLayoutDesc()
          .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(2))
          .addItem(nvrhi::BindingLayoutItem::ConstantBuffer(11))
          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(0))
          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(1))
          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(2))
          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(3))
          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(4))
          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(5))
          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(8))
          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(9))
          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(10))
          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(11))
          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(12))
          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(14))
          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(16))
          .addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(17))
          .addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(18))
          .addItem(nvrhi::BindingLayoutItem::Sampler(8))
          .addItem(nvrhi::BindingLayoutItem::Sampler(11))
          .addItem(nvrhi::BindingLayoutItem::Sampler(13))
          .addItem(nvrhi::BindingLayoutItem::Texture_UAV(0))
          .setVisibility(nvrhi::ShaderType::Compute));
  for (int i = 0; i < std::size(m_binding_set); ++i) {
    m_binding_set[i] = m_backend->GetDevice()->createBindingSet(
        nvrhi::BindingSetDesc()
            .addItem(nvrhi::BindingSetItem::ConstantBuffer(2, m_draw_constants))
            .addItem(nvrhi::BindingSetItem::ConstantBuffer(
                11, m_shadow_map->m_projection_buffer))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(
                0, m_gbuffer->m_gbuffer_diffuse))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(
                1, m_gbuffer->m_gbuffer_emission))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(
                2, m_gbuffer->m_gbuffer_params))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(
                3, m_gbuffer->m_gbuffer_normal))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(
                4, m_gbuffer->m_gbuffer_depth))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(5, m_ssao->m_outputAO))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(
                8, m_environment->m_dynamic_envmap_diffuse[0]))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(
                9, m_environment->m_dynamic_envmap_specular[0]))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(
                10, m_environment->m_brdf_lut))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(
                11, m_gbuffer_shadow->m_gbuffer_depth))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(
                12, m_contact_shadows->m_output_texture))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(
                14, m_sky->m_aerial_lut->m_lut))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(
                16, static_cast<nvrhi::ITexture*>(
                       GetResource("forwardplus_index_grid_opaque", nvrhi::ResourceType::Texture_SRV)
                           .m_resource)))
            .addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(
                17, static_cast<nvrhi::IBuffer*>(
                       GetResource("forwardplus_index_buffer_opaque", nvrhi::ResourceType::StructuredBuffer_SRV)
                           .m_resource)))
            .addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(
                18, static_cast<nvrhi::IBuffer*>(
                       GetResource("forwardplus_light_buffer", nvrhi::ResourceType::StructuredBuffer_SRV)
                           .m_resource)))
            .addItem(nvrhi::BindingSetItem::Sampler(8, sampler_linear))
            .addItem(nvrhi::BindingSetItem::Sampler(11, sampler_shadow_comp))
            .addItem(nvrhi::BindingSetItem::Sampler(13, sampler_linear_clamp))
            .addItem(nvrhi::BindingSetItem::Texture_UAV(0, m_output)),
        m_binding_layout);
  }
  m_pixel_shader =
      m_backend->CreateShader("gbuffer_lighting", nvrhi::ShaderType::Compute);
  m_pso = m_backend->GetDevice()->createComputePipeline(
      nvrhi::ComputePipelineDesc()
          .setComputeShader(m_pixel_shader)
          .addBindingLayout(m_binding_layout));
  // m_framebuffer = m_backend->GetDevice()->createFramebuffer(
  //     nvrhi::FramebufferDesc().addColorAttachment(m_output));
  // FullScreenPass::Init();
}

void GbufferBlitPass::CreatePipelineDesc(
    nvrhi::GraphicsPipelineDesc& pipeline_desc) {
  FullScreenPass::CreatePipelineDesc(pipeline_desc);
  pipeline_desc.addBindingLayout(m_binding_layout);
  pipeline_desc.setPixelShader(m_pixel_shader);
}

void GbufferBlitPass::UpdateConstants(nvrhi::ICommandList* command_list,
                                      glm::dmat4& view,
                                      const glm::dmat4& projection) {
  DrawConstants constants{};
  constants.m_inverse_model_view = glm::inverse(glm::dmat4(glm::dmat3(view)));
  constants.m_inverse_projection = glm::inverse(projection);

  const auto& daylight = Global.DayLight;
  constants.m_light_dir = glm::vec4(-daylight.direction, 0.f);
  constants.m_light_color = glm::vec4(m_sky->CalcSunColor(), 1.);
  constants.m_altitude = Global.pCamera.Pos.y;
  constants.m_time = Timer::GetTime();

  command_list->writeBuffer(m_draw_constants, &constants, sizeof(constants));
}

void GbufferBlitPass::Render(nvrhi::ICommandList* command_list,
                             glm::dmat4& view, const glm::dmat4& projection) {
  UpdateConstants(command_list, view, projection);
  Render(command_list);
  command_list->copyTexture(
      m_scene_depth, nvrhi::TextureSlice().resolve(m_scene_depth->getDesc()),
      m_gbuffer->m_gbuffer_depth,
      nvrhi::TextureSlice().resolve(m_scene_depth->getDesc()));
}

void GbufferBlitPass::Render(nvrhi::ICommandList* command_list) {
  nvrhi::ComputeState graphics_state;
  auto desc = m_output->getDesc();
  // InitState(graphics_state);
  graphics_state.setPipeline(m_pso);
  graphics_state.addBindingSet(
      m_binding_set[m_environment->GetCurrentSetIndex()]);
  command_list->setComputeState(graphics_state);
#define DISPATCH_SIZE(size, groupsize) ((size + groupsize - 1) / groupsize)
  command_list->dispatch(DISPATCH_SIZE(desc.width, 8),
                         DISPATCH_SIZE(desc.height, 8), 1);
  // Draw(command_list);
}

nvrhi::IFramebuffer* GbufferBlitPass::GetFramebuffer() { return m_framebuffer; }
