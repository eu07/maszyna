#pragma once

#include <nvrenderer/nvrenderer.h>
#include <nvrhi/nvrhi.h>

#include <glm/glm.hpp>

struct GbufferLighting : public MaResourceRegistry {
  GbufferLighting(NvRenderer* renderer)
      : MaResourceRegistry(renderer), m_renderer(renderer) {}

  struct ViewConstants {
    glm::mat4 m_inverse_model_view;
    glm::mat4 m_inverse_projection;
  };

  struct PushConstantsSpotLight {
    glm::vec3 m_origin;
    float m_radius_sqr;
    glm::vec3 m_direction;
    float m_cos_inner_cone;
    glm::vec3 m_color;
    float m_cos_outer_cone;
  };

  struct PackedSpotLight {
    explicit PackedSpotLight(NvRenderer::Renderable::SpotLight const& spotlight,
                             glm::dvec3 const& origin);
    glm::vec3 m_origin;
    float m_radius;
    uint64_t m_direction_cos_inner;
    uint64_t m_color_cos_outer;
  };

  struct DispatchParams {
    alignas(sizeof(glm::vec4)) glm::uvec3 m_num_thread_groups;
    alignas(sizeof(glm::vec4)) glm::uvec3 m_num_threads;
  };

  struct ViewParams {
    glm::mat4 m_inverse_projection;
    glm::mat4 m_view;
    glm::vec2 m_screen_dimensions;
    uint32_t m_num_lights;
  };

  std::vector<PackedSpotLight> m_packed_buffer;
  size_t m_lighting_buffer_capacity;

  nvrhi::BindingSetHandle m_bindings_cull_lights;
  nvrhi::ComputePipelineHandle m_pso_cull_lights;

  nvrhi::BindingSetHandle m_bindings_comp_frustums;
  nvrhi::ComputePipelineHandle m_pso_comp_frustums;

  nvrhi::BufferHandle m_atomic_counter_lights_opaque;
  nvrhi::BufferHandle m_atomic_counter_lights_transparent;
  nvrhi::BufferHandle m_index_buffer_lights_opaque;
  nvrhi::BufferHandle m_index_buffer_lights_transparent;
  nvrhi::TextureHandle m_index_grid_lights_opaque;
  nvrhi::TextureHandle m_index_grid_lights_transparent;

  nvrhi::TextureHandle m_diffuse_texture;
  nvrhi::TextureHandle m_specular_texture;
  nvrhi::BufferHandle m_light_buffer;
  nvrhi::BufferHandle m_view_constants;
  nvrhi::BufferHandle m_frustum_buffer;
  nvrhi::BufferHandle m_dispatch_params_buffer;
  nvrhi::BufferHandle m_view_params_buffer;
  nvrhi::BindingSetHandle m_binding_set_spotlight;
  nvrhi::ComputePipelineHandle m_pso_spotlight;
  nvrhi::BindingSetHandle m_binding_set_clear;
  nvrhi::ComputePipelineHandle m_pso_clear;

  int m_width;
  int m_height;
  int m_tiles_x;
  int m_tiles_y;

  void Init();
  void Render(const NvRenderer::RenderPass& pass);

  NvRenderer* m_renderer;
};