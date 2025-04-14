#include "gbufferlighting.h"

#include <nvrhi/utils.h>

#include <glm/gtc/packing.hpp>

#include "gbuffer.h"
#include "nvrendererbackend.h"
#include "ssao.h"
#include "config.h"

void GbufferLighting::Init() {
  auto fb_info = m_renderer->m_gbuffer->m_framebuffer->getFramebufferInfo();
  m_width = fb_info.width;
  m_height = fb_info.height;

  m_tiles_x = (m_width + 7) / 8;
  m_tiles_y = (m_height + 7) / 8;

  m_frustum_buffer = m_renderer->GetBackend()->GetDevice()->createBuffer(
      nvrhi::BufferDesc()
          .setDebugName("Forward Plus Frustum Buffer")
          .setStructStride(4 * sizeof(glm::vec4) + 10 * sizeof(float))
          .setByteSize(m_tiles_x * m_tiles_y * (4 * sizeof(glm::vec4) + 10 * sizeof(float)))
          .setCanHaveUAVs(true)
          .setInitialState(nvrhi::ResourceStates::UnorderedAccess)
          .setKeepInitialState(true));

  m_view_params_buffer = m_renderer->GetBackend()->GetDevice()->createBuffer(
      nvrhi::utils::CreateVolatileConstantBufferDesc(
          sizeof(ViewParams), "Forward Plus View Parameters", 16)
          .setInitialState(nvrhi::ResourceStates::ConstantBuffer)
          .setKeepInitialState(true));

  m_dispatch_params_buffer =
      m_renderer->GetBackend()->GetDevice()->createBuffer(
          nvrhi::utils::CreateVolatileConstantBufferDesc(
              sizeof(ViewParams), "Forward Plus Dispatch Parameters", 16)
              .setInitialState(nvrhi::ResourceStates::ConstantBuffer)
              .setKeepInitialState(true));

  {
    auto atomic_counter_desc =
        nvrhi::BufferDesc()
            .setInitialState(nvrhi::ResourceStates::UnorderedAccess)
            .setStructStride(sizeof(uint32_t))
            .setKeepInitialState(true)
            .setByteSize(sizeof(uint32_t))
            .setCanHaveUAVs(true);

    m_atomic_counter_lights_opaque =
        m_renderer->GetBackend()->GetDevice()->createBuffer(
            nvrhi::BufferDesc(atomic_counter_desc)
                .setDebugName("Forward Plus Atomic Counter Lights Opaque"));

    m_atomic_counter_lights_transparent =
        m_renderer->GetBackend()->GetDevice()->createBuffer(
            nvrhi::BufferDesc(atomic_counter_desc)
                .setDebugName(
                    "Forward Plus Atomic Counter Lights Transparent"));
  }

  {
    auto index_buffer_desc =
        nvrhi::BufferDesc()
            .setInitialState(nvrhi::ResourceStates::UnorderedAccess)
            .setStructStride(sizeof(uint32_t))
            .setKeepInitialState(true)
            .setByteSize(sizeof(uint32_t) * m_tiles_x * m_tiles_y * 64)
            .setCanHaveUAVs(true);

    m_index_buffer_lights_opaque =
        m_renderer->GetBackend()->GetDevice()->createBuffer(
            nvrhi::BufferDesc(index_buffer_desc)
                .setDebugName("Forward Plus Index Buffer Lights Opaque"));

    m_index_buffer_lights_transparent =
        m_renderer->GetBackend()->GetDevice()->createBuffer(
            nvrhi::BufferDesc(index_buffer_desc)
                .setDebugName("Forward Plus Index Buffer Lights Transparent"));
  }

  {
    auto index_grid_desc =
        nvrhi::TextureDesc()
            .setInitialState(nvrhi::ResourceStates::UnorderedAccess)
            .setFormat(nvrhi::Format::RG32_UINT)
            .setKeepInitialState(true)
            .setWidth(m_tiles_x)
            .setHeight(m_tiles_y)
            .setIsUAV(true);

    m_index_grid_lights_opaque =
        m_renderer->GetBackend()->GetDevice()->createTexture(
            nvrhi::TextureDesc(index_grid_desc)
                .setDebugName("Forward Plus Index Grid Lights Opaque"));

    m_index_grid_lights_transparent =
        m_renderer->GetBackend()->GetDevice()->createTexture(
            nvrhi::TextureDesc(index_grid_desc)
                .setDebugName("Forward Plus Index Grid Lights Transparent"));
  }

  {
    m_lighting_buffer_capacity =
        NvRenderer::Config()->m_lighting.m_max_lights_per_scene;
    m_light_buffer = m_renderer->GetBackend()->GetDevice()->createBuffer(
        nvrhi::BufferDesc()
            .setInitialState(nvrhi::ResourceStates::ShaderResource)
            .setKeepInitialState(true)
            .setByteSize(m_lighting_buffer_capacity *
                         sizeof(m_packed_buffer[0]))
            .setStructStride(sizeof(m_packed_buffer[0]))
            .setDebugName("Light Buffer"));
  }

  RegisterResource(true, "forwardplus_light_buffer", m_light_buffer,
                   nvrhi::ResourceType::StructuredBuffer_SRV);
  RegisterResource(true, "forwardplus_index_grid_opaque",
                   m_index_grid_lights_opaque,
                   nvrhi::ResourceType::Texture_SRV);
  RegisterResource(true, "forwardplus_index_grid_transparent",
                   m_index_grid_lights_transparent,
                   nvrhi::ResourceType::Texture_SRV);
  RegisterResource(true, "forwardplus_index_buffer_opaque",
                   m_index_buffer_lights_opaque,
                   nvrhi::ResourceType::StructuredBuffer_SRV);
  RegisterResource(true, "forwardplus_index_buffer_transparent",
                   m_index_buffer_lights_transparent,
                   nvrhi::ResourceType::StructuredBuffer_SRV);

  {
    nvrhi::BindingLayoutHandle binding_layout_comp_frustums;
    nvrhi::utils::CreateBindingSetAndLayout(
        m_renderer->GetBackend()->GetDevice(), nvrhi::ShaderType::Compute, 0,
        nvrhi::BindingSetDesc()
            .addItem(
                nvrhi::BindingSetItem::ConstantBuffer(3, m_view_params_buffer))
            .addItem(nvrhi::BindingSetItem::ConstantBuffer(
                4, m_dispatch_params_buffer))
            .addItem(nvrhi::BindingSetItem::StructuredBuffer_UAV(
                0, m_frustum_buffer)),
        binding_layout_comp_frustums, m_bindings_comp_frustums);

    auto cs_comp_frustums = m_renderer->m_backend->CreateShader(
        "forwardplus_compute_frustums", nvrhi::ShaderType::Compute);

    m_pso_comp_frustums =
        m_renderer->GetBackend()->GetDevice()->createComputePipeline(
            nvrhi::ComputePipelineDesc()
                .addBindingLayout(binding_layout_comp_frustums)
                .setComputeShader(cs_comp_frustums));
  }

  {
    nvrhi::BindingLayoutHandle binding_layout_cull_lights;
    nvrhi::utils::CreateBindingSetAndLayout(
        m_renderer->GetBackend()->GetDevice(), nvrhi::ShaderType::Compute, 0,
        nvrhi::BindingSetDesc()
            .addItem(
                nvrhi::BindingSetItem::ConstantBuffer(3, m_view_params_buffer))
            .addItem(nvrhi::BindingSetItem::ConstantBuffer(
                4, m_dispatch_params_buffer))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(
                3, m_renderer->m_gbuffer->m_gbuffer_depth))
            .addItem(
                nvrhi::BindingSetItem::StructuredBuffer_SRV(8, m_light_buffer))
            .addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(
                9, m_frustum_buffer))
            .addItem(nvrhi::BindingSetItem::StructuredBuffer_UAV(
                1, m_atomic_counter_lights_opaque))
            .addItem(nvrhi::BindingSetItem::StructuredBuffer_UAV(
                2, m_atomic_counter_lights_transparent))
            .addItem(nvrhi::BindingSetItem::StructuredBuffer_UAV(
                3, m_index_buffer_lights_opaque))
            .addItem(nvrhi::BindingSetItem::StructuredBuffer_UAV(
                4, m_index_buffer_lights_transparent))
            .addItem(nvrhi::BindingSetItem::Texture_UAV(
                5, m_index_grid_lights_opaque))
            .addItem(nvrhi::BindingSetItem::Texture_UAV(
                6, m_index_grid_lights_transparent)),
        binding_layout_cull_lights, m_bindings_cull_lights);

    auto cs_cull_lights = m_renderer->m_backend->CreateShader(
        "forwardplus_cull_lights", nvrhi::ShaderType::Compute);

    m_pso_cull_lights =
        m_renderer->GetBackend()->GetDevice()->createComputePipeline(
            nvrhi::ComputePipelineDesc()
                .addBindingLayout(binding_layout_cull_lights)
                .setComputeShader(cs_cull_lights));
  }

//  auto texture_desc =
//      nvrhi::TextureDesc()
//          .setWidth(m_width)
//          .setHeight(m_height)
//          .setFormat(nvrhi::Format::R11G11B10_FLOAT)
//          .setIsUAV(true)
//          .setInitialState(nvrhi::ResourceStates::ShaderResource)
//          .setKeepInitialState(true);
//texture_desc
//  texture_desc.setDebugName("Deferred Diffuse Buffer");
//  m_diffuse_texture =
//      m_renderer->GetBackend()->GetDevice()->createTexture(texture_desc);
//  RegisterResource(true, "deferred_lights", m_diffuse_texture,
//                   nvrhi::ResourceType::Texture_SRV);
//
//  m_view_constants = m_renderer->GetBackend()->GetDevice()->createBuffer(
//      nvrhi::utils::CreateVolatileConstantBufferDesc(
//          sizeof(ViewConstants), "Deferred Lighting Constants", 16)
//          .setInitialState(nvrhi::ResourceStates::ConstantBuffer)
//          .setKeepInitialState(true));
//
//  nvrhi::BindingLayoutHandle binding_layout_spot;
//  nvrhi::BindingLayoutHandle binding_layout_clear;
//
//  nvrhi::utils::CreateBindingSetAndLayout(
//      m_renderer->GetBackend()->GetDevice(), nvrhi::ShaderType::Compute, 0,
//      nvrhi::BindingSetDesc().addItem(
//          nvrhi::BindingSetItem::Texture_UAV(0, m_diffuse_texture)),
//      binding_layout_clear, m_binding_set_clear);
//
//  nvrhi::utils::CreateBindingSetAndLayout(
//      m_renderer->GetBackend()->GetDevice(), nvrhi::ShaderType::Compute, 0,
//      nvrhi::BindingSetDesc()
//          .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, m_view_constants))
//          .addItem(nvrhi::BindingSetItem::PushConstants(
//              1, sizeof(PushConstantsSpotLight)))
//          .addItem(nvrhi::BindingSetItem::Texture_UAV(0, m_diffuse_texture))
//          .addItem(nvrhi::BindingSetItem::Texture_SRV(
//              0, m_renderer->m_gbuffer->m_gbuffer_depth))
//          .addItem(nvrhi::BindingSetItem::Texture_SRV(
//              1, m_renderer->m_gbuffer->m_gbuffer_normal))
//          .addItem(nvrhi::BindingSetItem::Texture_SRV(
//              2, m_renderer->m_gbuffer->m_gbuffer_params))
//          .addItem(nvrhi::BindingSetItem::Texture_SRV(
//              3, m_renderer->m_gbuffer->m_gbuffer_diffuse))
//          .addItem(nvrhi::BindingSetItem::Texture_SRV(
//              5, m_renderer->m_ssao->m_outputAO)),
//      binding_layout_spot, m_binding_set_spotlight);
//
//  auto clear_shader = m_renderer->m_backend->CreateShader(
//      "gbuffer_light_clear", nvrhi::ShaderType::Compute);
//  auto spotlight_shader = m_renderer->m_backend->CreateShader(
//      "gbuffer_light_spot", nvrhi::ShaderType::Compute);
//
//  m_pso_clear = m_renderer->m_backend->GetDevice()->createComputePipeline(
//      nvrhi::ComputePipelineDesc()
//          .setComputeShader(clear_shader)
//          .addBindingLayout(binding_layout_clear));
//
//  m_pso_spotlight = m_renderer->m_backend->GetDevice()->createComputePipeline(
//      nvrhi::ComputePipelineDesc()
//          .setComputeShader(spotlight_shader)
//          .addBindingLayout(binding_layout_spot));
}

void GbufferLighting::Render(const NvRenderer::RenderPass& pass) {
//  ViewConstants view_constants{};
//  view_constants.m_inverse_model_view =
//      static_cast<glm::mat4>(glm::inverse(pass.m_transform));
//  view_constants.m_inverse_projection =
//      static_cast<glm::mat4>(glm::inverse(pass.m_projection));
//  pass.m_command_list_draw->writeBuffer(m_view_constants, &view_constants,
//                                        sizeof(view_constants));
  m_packed_buffer.clear();
  for (int i = 0; i < m_renderer->m_renderable_spotlights.size(); ++i) {
    if (i >= m_lighting_buffer_capacity) break;
    auto& light = m_packed_buffer.emplace_back(
        m_renderer->m_renderable_spotlights[i], pass.m_origin);
  }

  pass.m_command_list_draw->writeBuffer(
      m_light_buffer, m_packed_buffer.data(),
      m_packed_buffer.size() * sizeof(m_packed_buffer[0]));

  ViewParams view_params{};
  view_params.m_inverse_projection =
      static_cast<glm::mat4>(glm::inverse(pass.m_projection));
  view_params.m_view = static_cast<glm::mat4>(pass.m_transform);
  view_params.m_screen_dimensions = {m_width, m_height};
  view_params.m_num_lights = m_packed_buffer.size();
  pass.m_command_list_draw->writeBuffer(m_view_params_buffer, &view_params,
                                        sizeof(view_params));

  {
    DispatchParams dispatch_params{};
    dispatch_params.m_num_threads = {m_tiles_x, m_tiles_y, 1u};
    dispatch_params.m_num_thread_groups =
        (dispatch_params.m_num_threads + 7u) / 8u;
    pass.m_command_list_draw->writeBuffer(
        m_dispatch_params_buffer, &dispatch_params, sizeof(dispatch_params));

    pass.m_command_list_draw->setComputeState(
        nvrhi::ComputeState()
            .setPipeline(m_pso_comp_frustums)
            .addBindingSet(m_bindings_comp_frustums));
    pass.m_command_list_draw->dispatch(dispatch_params.m_num_thread_groups.x,
                                       dispatch_params.m_num_thread_groups.y,
                                       dispatch_params.m_num_thread_groups.z);
  }

  {
    DispatchParams dispatch_params{};
    dispatch_params.m_num_thread_groups = {m_tiles_x, m_tiles_y, 1u};
    dispatch_params.m_num_threads = dispatch_params.m_num_thread_groups * 8u;
    pass.m_command_list_draw->writeBuffer(
        m_dispatch_params_buffer, &dispatch_params, sizeof(dispatch_params));

    uint32_t zero = 0;
    pass.m_command_list_draw->writeBuffer(m_atomic_counter_lights_opaque, &zero,
                                          sizeof(zero));
    pass.m_command_list_draw->writeBuffer(m_atomic_counter_lights_transparent,
                                          &zero, sizeof(zero));

    pass.m_command_list_draw->setComputeState(
        nvrhi::ComputeState()
            .setPipeline(m_pso_cull_lights)
            .addBindingSet(m_bindings_cull_lights));
    pass.m_command_list_draw->dispatch(dispatch_params.m_num_thread_groups.x,
                                       dispatch_params.m_num_thread_groups.y,
                                       dispatch_params.m_num_thread_groups.z);
  }

//  {
//    pass.m_command_list_draw->setComputeState(
//        nvrhi::ComputeState()
//            .setPipeline(m_pso_clear)
//            .addBindingSet(m_binding_set_clear));
//    pass.m_command_list_draw->dispatch((m_width + 7) / 8, (m_height + 7) / 8);
//  }
//
//  for (const auto& spotlight : m_renderer->m_renderable_spotlights) {
//    PushConstantsSpotLight push_constants{};
//    push_constants.m_origin = spotlight.m_origin - pass.m_origin;
//    push_constants.m_radius_sqr = spotlight.m_radius * spotlight.m_radius;
//    push_constants.m_direction = spotlight.m_direction;
//    push_constants.m_cos_inner_cone = spotlight.m_cos_inner_cone;
//    push_constants.m_cos_outer_cone = spotlight.m_cos_outer_cone;
//    push_constants.m_color = spotlight.m_color;
//
//    pass.m_command_list_draw->setComputeState(
//        nvrhi::ComputeState()
//            .setPipeline(m_pso_spotlight)
//            .addBindingSet(m_binding_set_spotlight));
//    pass.m_command_list_draw->setPushConstants(&push_constants,
//                                               sizeof(push_constants));
//    pass.m_command_list_draw->dispatch((m_width + 7) / 8, (m_height + 7) / 8);
//  }
}

GbufferLighting::PackedSpotLight::PackedSpotLight(
    const NvRenderer::Renderable::SpotLight& spotlight,
    glm::dvec3 const& origin) {
  m_origin = spotlight.m_origin - origin;
  m_radius = spotlight.m_radius;
  m_direction_cos_inner =
      glm::packHalf4x16({spotlight.m_direction, spotlight.m_cos_inner_cone});
  glm::vec3 color =
      spotlight.m_color.r * glm::vec3(0.4123908f, 0.21263901f, 0.01933082f) +
      spotlight.m_color.g * glm::vec3(0.35758434f, 0.71516868f, 0.11919478f) +
      spotlight.m_color.b * glm::vec3(0.18048079f, 0.07219232f, 0.95053215f);
  m_color_cos_outer = glm::packHalf4x16({color, spotlight.m_cos_outer_cone});
}
