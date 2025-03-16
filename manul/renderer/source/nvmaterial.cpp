#include "nvmaterial.h"
#include "nvrenderer/nvrenderer.h"

#include <fmt/format.h>
#include <nvrhi/utils.h>

#include "Logs.h"
#include "csm.h"
#include "environment.h"
#include "gbuffer.h"
#include "gbufferblitpass.h"
#include "nvrendererbackend.h"
#include "nvtexture.h"
#include "sky.h"

bool NvRenderer::MaterialTemplate::CreateBindingSet(size_t pipeline_index,
                                                    MaterialCache &cache) {
  // for (size_t i = 0; i < cache.m_pipelines.size(); ++i) {
  auto pipeline = cache.m_pipelines[pipeline_index];
  if (!pipeline) return false;

  nvrhi::BindingSetDesc binding_set_desc{};

  auto binding_layout = pipeline->getDesc().bindingLayouts[0];

  if (!m_resource_mappings[pipeline_index].ToBindingSet(
          binding_set_desc, &cache, binding_layout->getDesc()))
    return false;

  cache.m_binding_sets[pipeline_index] =
      m_renderer->GetBackend()->GetDevice()->createBindingSet(binding_set_desc,
                                                              binding_layout);
  //}
  // nvrhi::BindingSetDesc binding_set_desc = nvrhi::BindingSetDesc();
  // nvrhi::BindingSetDesc binding_set_desc_cube = nvrhi::BindingSetDesc();
  // nvrhi::BindingSetDesc binding_set_desc_shadow = nvrhi::BindingSetDesc();
  // nvrhi::static_vector<nvrhi::SamplerHandle, 8 + 1> samplers{};
  // for (int i = 0; i < m_texture_bindings.size(); ++i) {
  //   const auto &binding = m_texture_bindings[i];
  //   if (binding.m_name.empty()) continue;
  //   auto texture = m_renderer->m_texture_manager->GetRhiTexture(
  //       cache.m_textures[i], pass.m_command_list_preparation);
  //   if (!texture) {
  //     texture = m_renderer->m_texture_manager->GetRhiTexture(
  //         1, pass.m_command_list_preparation);
  //   }
  //   auto &sampler = samplers.emplace_back();
  //   auto &sampler_cubemap = samplers.emplace_back();
  //   sampler = m_renderer->m_texture_manager->GetSamplerForTraits(
  //       m_renderer->m_texture_manager->GetTraits(cache.m_textures[i]),
  //       RenderPassType::Deferred);
  //   sampler_cubemap = m_renderer->m_texture_manager->GetSamplerForTraits(
  //       m_renderer->m_texture_manager->GetTraits(cache.m_textures[i]),
  //       RenderPassType::CubeMap);
  //   binding_set_desc.addItem(nvrhi::BindingSetItem::Texture_SRV(i, texture));
  //   binding_set_desc.addItem(nvrhi::BindingSetItem::Sampler(i, sampler));
  //   binding_set_desc_cube.addItem(
  //       nvrhi::BindingSetItem::Texture_SRV(i, texture));
  //   binding_set_desc_cube.addItem(
  //       nvrhi::BindingSetItem::Sampler(i, sampler_cubemap));
  //   if (cache.m_masked_shadow && i == m_masked_shadow_texture) {
  //     auto &shadow_sampler = samplers.emplace_back();
  //     shadow_sampler = m_renderer->m_texture_manager->GetSamplerForTraits(
  //         m_renderer->m_texture_manager->GetTraits(cache.m_textures[i]),
  //         RenderPassType::ShadowMap);
  //     binding_set_desc_shadow.addItem(
  //         nvrhi::BindingSetItem::Texture_SRV(0, texture));
  //     binding_set_desc_shadow.addItem(
  //         nvrhi::BindingSetItem::Sampler(0, shadow_sampler));
  //   }
  // }
  // cache.m_binding_set =
  // m_renderer->GetBackend()->GetDevice()->createBindingSet(
  //     binding_set_desc, m_binding_layout);
  // cache.m_binding_set_cubemap =
  //     m_renderer->GetBackend()->GetDevice()->createBindingSet(
  //         binding_set_desc_cube, m_binding_layout);
  // if (cache.m_masked_shadow) {
  //   cache.m_binding_set_shadow =
  //       m_renderer->GetBackend()->GetDevice()->createBindingSet(
  //           binding_set_desc_shadow,
  //           m_renderer->m_binding_layout_shadow_masked);
  // }
  cache.m_last_texture_updates[pipeline_index] = NvTexture::s_change_counter;
  return true;
}

nvrhi::IGraphicsPipeline *NvRenderer::MaterialTemplate::GetPipeline(
    RenderPassType pass_type, DrawType draw_type, bool alpha_masked) const {
  if (auto pipeline =
          m_pipelines[Constants::GetPipelineIndex(pass_type, draw_type)])
    return pipeline;
  if (pass_type == RenderPassType::ShadowMap) {
    return m_renderer->m_pso_shadow[Constants::GetDefaultShadowPipelineIndex(
        draw_type, alpha_masked)];
  }
  return nullptr;
}

void NvRenderer::MaterialTemplate::Init(const YAML::Node &conf) {
  InitResourceRegistry();
  std::vector<MaResourceMapping> texture_mappings{};
  std::vector<MaResourceMapping> sampler_mappings{};
  MaResourceMapping texture_mapping_shadow{};
  MaResourceMapping sampler_mapping_shadow{};
  m_pipelines.fill(nullptr);
  m_masked_shadow_texture = conf["masked_shadow_texture"].as<std::string>();
  for (const auto it : conf["textures"]) {
    auto index = it.second["binding"].as<int>();
    m_texture_bindings.resize(
        glm::max(m_texture_bindings.size(), static_cast<size_t>(index) + 1));
    auto &[name, sampler_name, hint, default_texture] =
        m_texture_bindings[index];

    name = it.first.as<std::string>();
    sampler_name = fmt::format("{:s}_sampler", name.c_str());

    texture_mappings.emplace_back(
        MaResourceMapping::Texture_SRV(index, name.c_str()));
    sampler_mappings.emplace_back(
        MaResourceMapping::Sampler(index, sampler_name.c_str()));

    RegisterTexture(name.c_str(), 1);
    RegisterResource(
        false, "masked_shadow_sampler",
        GetTextureManager()->GetSamplerForTraits(0, RenderPassType::ShadowMap),
        nvrhi::ResourceType::Sampler);

    const static std::unordered_map<std::string_view, int> hints{
        {"color", GL_SRGB_ALPHA}, {"linear", GL_RGBA}, {"normalmap", GL_RG}};
    hint = hints.at(it.second["hint"].as<std::string_view>());

    if (it.first.Scalar() == conf["masked_shadow_texture"].Scalar()) {
      texture_mapping_shadow = MaResourceMapping::Texture_SRV(0, name.c_str());
      sampler_mapping_shadow =
          MaResourceMapping::Sampler(0, "masked_shadow_sampler");
    }
  }
  std::array<nvrhi::ShaderHandle, static_cast<size_t>(RenderPassType::Num)>
      pixel_shaders;
  for (int i = 0; i < pixel_shaders.size(); ++i) {
    pixel_shaders[i] = m_renderer->GetBackend()->CreateMaterialShader(
        m_name, static_cast<RenderPassType>(i));
  }

  {
    nvrhi::BindingLayoutDesc binding_layout_desc;
    binding_layout_desc.setVisibility(nvrhi::ShaderType::Pixel);
    for (int i = 0; i < m_texture_bindings.size(); ++i) {
      if (m_texture_bindings[i].m_name.empty()) continue;
      binding_layout_desc.addItem(nvrhi::BindingLayoutItem::Texture_SRV(i))
          .addItem(nvrhi::BindingLayoutItem::Sampler(i));
    }
    m_binding_layout =
        m_renderer->GetBackend()->GetDevice()->createBindingLayout(
            binding_layout_desc);
  }
  for (size_t i = 0; i < Constants::NumMaterialPasses(); ++i) {
    auto pass = static_cast<RenderPassType>(i);
    for (size_t j = 0; j < Constants::NumDrawTypes(); ++j) {
      auto draw = static_cast<DrawType>(j);
      auto &mappings =
          m_resource_mappings[Constants::GetPipelineIndex(pass, draw)];
      nvrhi::GraphicsPipelineDesc pipeline_desc{};
      auto pixel_shader = pixel_shaders[static_cast<size_t>(pass)];
      if (!pixel_shader) {
        if (pass == RenderPassType::ShadowMap) {
          mappings.Add(MaResourceMapping::PushConstants<PushConstantsShadow>(1))
              .Add(texture_mapping_shadow)
              .Add(sampler_mapping_shadow);
        }
        continue;
      }
      pipeline_desc.setPixelShader(pixel_shader)
          .setPrimType(nvrhi::PrimitiveType::TriangleList);

      for (const auto &mapping : texture_mappings) mappings.Add(mapping);
      for (const auto &mapping : sampler_mappings) mappings.Add(mapping);

      nvrhi::IFramebuffer *framebuffer;
      pipeline_desc.setInputLayout(
          m_renderer->m_input_layout[static_cast<size_t>(draw)]);
      switch (pass) {
        case RenderPassType::Deferred:
        case RenderPassType::Forward:
          pipeline_desc.setVertexShader(
              m_renderer->m_vertex_shader[static_cast<size_t>(draw)]);
          break;
        case RenderPassType::CubeMap:
          pipeline_desc.setVertexShader(
              m_renderer->m_vertex_shader_cubemap[static_cast<size_t>(draw)]);
          break;
        default:;
      }
      switch (pass) {
        case RenderPassType::Deferred:
        case RenderPassType::Forward:
          mappings.Add(MaResourceMapping::PushConstants<PushConstantsDraw>(1))
              .Add(MaResourceMapping::ConstantBuffer(
                  0, "cb_draw_constants_static"));
          break;
        case RenderPassType::CubeMap:
          mappings
              .Add(MaResourceMapping::PushConstants<PushConstantsCubemap>(1))
              .Add(MaResourceMapping::ConstantBuffer(
                  0, "cb_draw_constants_static_cubemap"));
          break;
        default:;
      }
      switch (pass) {
        case RenderPassType::Forward:
          mappings
              .Add(MaResourceMapping::VolatileConstantBuffer(
                  2, "gbuffer_lighting_constants"))
              .Add(MaResourceMapping::ConstantBuffer(
                  11, "shadow_projection_buffer"))
              .Add(MaResourceMapping::Texture_SRV(8, "env_dynamic_diffuse"))
              .Add(MaResourceMapping::Texture_SRV(9, "env_dynamic_specular"))
              .Add(MaResourceMapping::Texture_SRV(10, "env_brdf_lut"))
              .Add(MaResourceMapping::Texture_SRV(11, "shadow_depths"))
              .Add(MaResourceMapping::Texture_SRV(12, "gbuffer_depth"))
              .Add(MaResourceMapping::Texture_SRV(14, "sky_aerial_lut"))
              .Add(MaResourceMapping::Texture_SRV(16, "forwardplus_index_grid_transparent"))
              .Add(MaResourceMapping::StructuredBuffer_SRV(17, "forwardplus_index_buffer_transparent"))
              .Add(MaResourceMapping::StructuredBuffer_SRV(18, "forwardplus_light_buffer"))
              .Add(MaResourceMapping::Sampler(8, "sampler_linear_wrap"))
              .Add(MaResourceMapping::Sampler(11, "shadow_sampler_comp"))
              .Add(MaResourceMapping::Sampler(13, "sampler_linear_clamp_v_repeat_h"));
          break;
        default:;
      }
      switch (pass) {
        case RenderPassType::Deferred:
          framebuffer = m_renderer->m_gbuffer->m_framebuffer;
          break;
        case RenderPassType::Forward:
          framebuffer = m_renderer->m_framebuffer_forward;
          break;
        case RenderPassType::CubeMap:
          framebuffer = m_renderer->m_gbuffer_cube->m_framebuffer;
          break;
        default:;
      }
      switch (pass) {
        case RenderPassType::Deferred:
          case RenderPassType::CubeMap:
          pipeline_desc.setRenderState(
              nvrhi::RenderState()
                  .setDepthStencilState(
                      nvrhi::DepthStencilState()
                          .enableDepthTest()
                          .enableDepthWrite()
                          .disableStencil()
                          .setDepthFunc(nvrhi::ComparisonFunc::Greater))
                  .setRasterState(nvrhi::RasterState()
                                      .setFillSolid()
                                      .enableDepthClip()
                                      .disableScissor()
                                      .setCullFront())
                  .setBlendState(nvrhi::BlendState().setRenderTarget(
                      0, nvrhi::BlendState::RenderTarget().disableBlend())));
          break;
        case RenderPassType::Forward:
          pipeline_desc.setRenderState(
              nvrhi::RenderState()
                  .setDepthStencilState(
                      nvrhi::DepthStencilState()
                          .enableDepthTest()
                          .disableDepthWrite()
                          .disableStencil()
                          .setDepthFunc(nvrhi::ComparisonFunc::Greater))
                  .setRasterState(nvrhi::RasterState()
                                      .setFillSolid()
                                      .enableDepthClip()
                                      .disableScissor()
                                      .setCullFront())
                  .setBlendState(
                      nvrhi::BlendState()
                          .setRenderTarget(
                              0, nvrhi::BlendState::RenderTarget()
                                     .enableBlend()
                                     .setBlendOp(nvrhi::BlendOp::Add)
                                     .setSrcBlend(nvrhi::BlendFactor::One)
                                     .setDestBlend(
                                         nvrhi::BlendFactor::OneMinusSrcAlpha))
                          .setRenderTarget(1, nvrhi::BlendState::RenderTarget()
                                                  .disableBlend())));
          break;
        default:;
      }

      nvrhi::BindingLayoutDesc bl_desc{};
      bl_desc.visibility = nvrhi::ShaderType::All;
      // for (const auto &bl : pipeline_desc.bindingLayouts) {
      //   bl_desc.visibility = bl_desc.visibility | bl->getDesc()->visibility;
      //   for (const auto &element : bl->getDesc()->bindings) {
      //     bl_desc.addItem(element);
      //   }
      // }

      mappings.ToBindingLayout(bl_desc);

      pipeline_desc.bindingLayouts.resize(0);
      pipeline_desc.addBindingLayout(
          m_renderer->GetBackend()->GetDevice()->createBindingLayout(bl_desc));

      m_pipelines[Constants::GetPipelineIndex(pass, draw)] =
          m_renderer->GetBackend()->GetDevice()->createGraphicsPipeline(
              pipeline_desc, framebuffer);
    }
  }
}

bool NvRenderer::InitMaterials() {
  m_drawconstant_buffer = m_backend->GetDevice()->createBuffer(
      nvrhi::utils::CreateStaticConstantBufferDesc(sizeof(DrawConstants),
                                                   "Draw Constants")
          .setInitialState(nvrhi::ResourceStates::ConstantBuffer)
          .setKeepInitialState(true));
  m_cubedrawconstant_buffer = m_backend->GetDevice()->createBuffer(
      nvrhi::utils::CreateStaticConstantBufferDesc(sizeof(CubeDrawConstants),
                                                   "Draw Data")
          .setInitialState(nvrhi::ResourceStates::ConstantBuffer)
          .setKeepInitialState(true));

  RegisterResource(true, "cb_draw_constants_static", m_drawconstant_buffer,
                   nvrhi::ResourceType::ConstantBuffer);
  RegisterResource(true, "cb_draw_constants_static_cubemap",
                   m_cubedrawconstant_buffer,
                   nvrhi::ResourceType::ConstantBuffer);

  {
    nvrhi::CommandListHandle command_list =
        GetBackend()->GetDevice()->createCommandList();
    command_list->open();
    CubeDrawConstants data;
    data.m_face_projection = glm::perspectiveFovRH_ZO(
        M_PI_2, 1., 1., .1,
        static_cast<double>(Global.reflectiontune.range_instances));
    command_list->writeBuffer(m_cubedrawconstant_buffer, &data, sizeof(data));
    command_list->close();
    GetBackend()->GetDevice()->executeCommandList(command_list);
  }

  nvrhi::utils::CreateBindingSetAndLayout(
      m_backend->GetDevice(), nvrhi::ShaderType::AllGraphics, 0,
      nvrhi::BindingSetDesc()
          .addItem(
              nvrhi::BindingSetItem::ConstantBuffer(0, m_drawconstant_buffer))
          .addItem(nvrhi::BindingSetItem::PushConstants(
              1, sizeof(PushConstantsDraw))),
      m_binding_layout_drawconstants, m_binding_set_drawconstants);
  nvrhi::utils::CreateBindingSetAndLayout(
      m_backend->GetDevice(), nvrhi::ShaderType::AllGraphics, 0,
      nvrhi::BindingSetDesc()
          .addItem(nvrhi::BindingSetItem::ConstantBuffer(
              0, m_cubedrawconstant_buffer))
          .addItem(nvrhi::BindingSetItem::PushConstants(
              1, sizeof(PushConstantsCubemap))),
      m_binding_layout_cubedrawconstants, m_binding_set_cubedrawconstants);
  nvrhi::utils::CreateBindingSetAndLayout(
      m_backend->GetDevice(), nvrhi::ShaderType::AllGraphics, 0,
      nvrhi::BindingSetDesc().addItem(
          nvrhi::BindingSetItem::PushConstants(1, sizeof(PushConstantsShadow))),
      m_binding_layout_shadowdrawconstants, m_binding_set_shadowdrawconstants);
  m_binding_layout_forward = GetBackend()->GetDevice()->createBindingLayout(
      nvrhi::BindingLayoutDesc()
          .setVisibility(nvrhi::ShaderType::Pixel)
          .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(2))
          .addItem(nvrhi::BindingLayoutItem::ConstantBuffer(11))
          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(8))
          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(9))
          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(10))
          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(11))
          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(12))
          .addItem(nvrhi::BindingLayoutItem::Texture_SRV(14))
          .addItem(nvrhi::BindingLayoutItem::Sampler(8))
          .addItem(nvrhi::BindingLayoutItem::Sampler(11))
          .addItem(nvrhi::BindingLayoutItem::Sampler(13)));
  for (int i = 0; i < m_binding_set_forward.size(); ++i) {
    m_binding_set_forward[i] = m_backend->GetDevice()->createBindingSet(
        nvrhi::BindingSetDesc()
            .addItem(nvrhi::BindingSetItem::ConstantBuffer(
                2, m_gbuffer_blit->m_draw_constants))
            .addItem(nvrhi::BindingSetItem::ConstantBuffer(
                11, m_shadow_map->m_projection_buffer))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(
                8, m_environment->m_dynamic_envmap_diffuse[0]))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(
                9, m_environment->m_dynamic_envmap_specular[0]))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(
                10, m_environment->m_brdf_lut))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(
                11, m_gbuffer_shadow->m_gbuffer_depth))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(
                12, m_gbuffer->m_gbuffer_depth))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(
                14, m_sky->m_aerial_lut->m_lut))
            .addItem(nvrhi::BindingSetItem::Sampler(
                8, m_backend->GetDevice()->createSampler(
                       nvrhi::SamplerDesc().setAllFilters(true))))
            .addItem(nvrhi::BindingSetItem::Sampler(
                11, m_backend->GetDevice()->createSampler(
                        nvrhi::SamplerDesc()
                            .setReductionType(
                                nvrhi::SamplerReductionType::Comparison)
                            .setComparisonFunc(nvrhi::ComparisonFunc::Greater)
                            .setAllAddressModes(
                                nvrhi::SamplerAddressMode::ClampToEdge)
                            .setAllFilters(true))))
            .addItem(nvrhi::BindingSetItem::Sampler(
                13, m_backend->GetDevice()->createSampler(
                        nvrhi::SamplerDesc()
                            .setAllAddressModes(
                                nvrhi::SamplerAddressMode::ClampToEdge)
                            .setAllFilters(true)))),
        m_binding_layout_forward);
  }

  m_vertex_shader[static_cast<size_t>(DrawType::Model)] =
      m_backend->CreateShader("default_vertex", nvrhi::ShaderType::Vertex);
  m_vertex_shader[static_cast<size_t>(DrawType::InstancedModel)] =
      m_backend->CreateShader("instanced_vertex", nvrhi::ShaderType::Vertex);
  m_vertex_shader_shadow[static_cast<size_t>(DrawType::Model)] =
      m_backend->CreateShader("shadow_vertex", nvrhi::ShaderType::Vertex);
  m_vertex_shader_shadow[static_cast<size_t>(DrawType::InstancedModel)] =
      m_backend->CreateShader("instanced_shadow_vertex",
                              nvrhi::ShaderType::Vertex);
  m_vertex_shader_cubemap[static_cast<size_t>(DrawType::Model)] =
      m_backend->CreateShader("cubemap_vertex", nvrhi::ShaderType::Vertex);
  m_vertex_shader_cubemap[static_cast<size_t>(DrawType::InstancedModel)] =
      m_backend->CreateShader("instanced_cubemap_vertex",
                              nvrhi::ShaderType::Vertex);
  m_pixel_shader_shadow_masked =
      m_backend->CreateShader("shadow_masked", nvrhi::ShaderType::Pixel);

  nvrhi::VertexAttributeDesc desc[]{
      nvrhi::VertexAttributeDesc()
          .setBufferIndex(0)
          .setElementStride(sizeof(gfx::basic_vertex))
          .setFormat(nvrhi::Format::RGB32_FLOAT)
          .setName("Position")
          .setIsInstanced(false)
          .setOffset(offsetof(gfx::basic_vertex, position)),
      nvrhi::VertexAttributeDesc()
          .setBufferIndex(0)
          .setElementStride(sizeof(gfx::basic_vertex))
          .setFormat(nvrhi::Format::RGB32_FLOAT)
          .setName("Normal")
          .setIsInstanced(false)
          .setOffset(offsetof(gfx::basic_vertex, normal)),
      nvrhi::VertexAttributeDesc()
          .setBufferIndex(0)
          .setElementStride(sizeof(gfx::basic_vertex))
          .setFormat(nvrhi::Format::RG32_FLOAT)
          .setName("TexCoord")
          .setIsInstanced(false)
          .setOffset(offsetof(gfx::basic_vertex, texture)),
      nvrhi::VertexAttributeDesc()
          .setBufferIndex(0)
          .setElementStride(sizeof(gfx::basic_vertex))
          .setFormat(nvrhi::Format::RGBA32_FLOAT)
          .setName("Tangent")
          .setIsInstanced(false)
          .setOffset(offsetof(gfx::basic_vertex, tangent)),
      nvrhi::VertexAttributeDesc()
          .setBufferIndex(1)
          .setElementStride(sizeof(glm::mat3x4))
          .setFormat(nvrhi::Format::RGBA32_FLOAT)
          .setName("InstanceTransform")
          .setIsInstanced(true)
          .setArraySize(3)
          .setOffset(0)};
  m_input_layout[static_cast<size_t>(DrawType::Model)] =
      m_backend->GetDevice()->createInputLayout(
          desc, std::size(desc) - 1,
          m_vertex_shader[static_cast<size_t>(DrawType::Model)]);
  m_input_layout[static_cast<size_t>(DrawType::InstancedModel)] =
      m_backend->GetDevice()->createInputLayout(
          desc, std::size(desc),
          m_vertex_shader[static_cast<size_t>(DrawType::InstancedModel)]);

  {
    m_binding_layout_shadow_masked =
        m_backend->GetDevice()->createBindingLayout(
            nvrhi::BindingLayoutDesc()
                .setVisibility(nvrhi::ShaderType::Pixel)
                .addItem(nvrhi::BindingLayoutItem::Texture_SRV(0))
                .addItem(nvrhi::BindingLayoutItem::Sampler(0)));
    for (int i = 0; i < Constants::NumDrawTypes(); ++i) {
      for (int j = 0; j < 2; ++j) {
        DrawType draw = static_cast<DrawType>(i);
        bool masked = j;
        nvrhi::BindingLayoutDesc desc =
            *m_binding_layout_shadowdrawconstants->getDesc();
        desc.visibility = nvrhi::ShaderType::All;
        if (masked)
          desc.addItem(nvrhi::BindingLayoutItem::Texture_SRV(0))
              .addItem(nvrhi::BindingLayoutItem::Sampler(0));
        auto binding_layout =
            GetBackend()->GetDevice()->createBindingLayout(desc);
        auto pipeline_desc =
            nvrhi::GraphicsPipelineDesc()
                .setRenderState(
                    nvrhi::RenderState()
                        .setDepthStencilState(
                            nvrhi::DepthStencilState()
                                .enableDepthTest()
                                .enableDepthWrite()
                                .disableStencil()
                                .setDepthFunc(nvrhi::ComparisonFunc::Greater))
                        .setRasterState(nvrhi::RasterState()
                                            .setFillSolid()
                                            .disableDepthClip()
                                            .disableScissor()
                                            .setCullBack()
                                            .setDepthBias(1)
                                            .setSlopeScaleDepthBias(-4.f))
                        .setBlendState(nvrhi::BlendState().setRenderTarget(
                            0,
                            nvrhi::BlendState::RenderTarget().disableBlend())))
                .setVertexShader(
                    m_vertex_shader_shadow[static_cast<size_t>(draw)])
                .setInputLayout(m_input_layout[static_cast<size_t>(draw)]);
        if (masked) {
          pipeline_desc.setPixelShader(m_pixel_shader_shadow_masked);
        }
        pipeline_desc.addBindingLayout(binding_layout);

        m_pso_shadow[Constants::GetDefaultShadowPipelineIndex(draw, masked)] =
            GetBackend()->GetDevice()->createGraphicsPipeline(
                pipeline_desc, m_gbuffer_shadow->m_framebuffer);
      }
    }
  }

  {
    auto vs_line =
        m_backend->CreateShader("vtx_line", nvrhi::ShaderType::Vertex);
    auto gs_line =
        m_backend->CreateShader("geo_line", nvrhi::ShaderType::Geometry);
    auto ps_line =
        m_backend->CreateShader("pix_line", nvrhi::ShaderType::Pixel);
    nvrhi::BindingLayoutHandle binding_layout_line;
    nvrhi::utils::CreateBindingSetAndLayout(
        GetBackend()->GetDevice(), nvrhi::ShaderType::AllGraphics, 0,
        nvrhi::BindingSetDesc()
            .addItem(
                nvrhi::BindingSetItem::ConstantBuffer(0, m_drawconstant_buffer))
            .addItem(nvrhi::BindingSetItem::PushConstants(
                1, sizeof(PushConstantsLine))),
        binding_layout_line, m_binding_set_line);

    m_pso_line = GetBackend()->GetDevice()->createGraphicsPipeline(
        nvrhi::GraphicsPipelineDesc()
            .setRenderState(
                nvrhi::RenderState()
                    .setDepthStencilState(
                        nvrhi::DepthStencilState()
                            .enableDepthTest()
                            .enableDepthWrite()
                            .disableStencil()
                            .setDepthFunc(nvrhi::ComparisonFunc::Greater))
                    .setRasterState(nvrhi::RasterState()
                                        .setFillSolid()
                                        .enableDepthClip()
                                        .disableScissor()
                                        .setCullFront())
                    .setBlendState(nvrhi::BlendState().setRenderTarget(
                        0, nvrhi::BlendState::RenderTarget().disableBlend())))
            .setPrimType(nvrhi::PrimitiveType::LineList)
            .setVertexShader(vs_line)
            .setGeometryShader(gs_line)
            .setPixelShader(ps_line)
            .setInputLayout(
                m_input_layout[static_cast<size_t>(DrawType::Model)])
            .addBindingLayout(binding_layout_line),
        m_gbuffer->m_framebuffer);
  }

  auto config = YAML::LoadFile("shaders/project.manul");
  for (const auto item : config["shaders"]["materials"]) {
    auto material =
        std::make_shared<MaterialTemplate>(item.first.as<std::string>(), this);
    m_material_templates[material->m_name] = material;
    material->Init(item.second);
  }

  return true;
}
