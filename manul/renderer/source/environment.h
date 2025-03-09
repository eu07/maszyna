#pragma once

#include "fullscreenpass.h"
#include "nvrenderer/resource_registry.h"
#include "nvrhi/nvrhi.h"

struct MaEnvironment : public MaResourceRegistry {
  nvrhi::TextureHandle m_skybox;
  nvrhi::TextureHandle m_dynamic_skybox[2];
  nvrhi::TextureHandle m_envmap_specular;
  nvrhi::TextureHandle m_dynamic_envmap_specular[2];
  nvrhi::TextureHandle m_envmap_diffuse;
  nvrhi::TextureHandle m_dynamic_envmap_diffuse[2];
  nvrhi::TextureHandle m_hdri;
  nvrhi::TextureHandle m_brdf_lut;
  nvrhi::TextureHandle m_sky_texture;
  nvrhi::TextureHandle m_aerial_lut;

  nvrhi::SamplerHandle m_sampler_linear_clamp_v_repeat_h;
  nvrhi::SamplerHandle m_sampler_linear_clamp;
  nvrhi::SamplerHandle m_sampler_point_clamp;

  nvrhi::ShaderHandle m_cs_sample_equirectangular;
  nvrhi::ComputePipelineHandle m_pso_sample_equirectangular;
  nvrhi::BindingLayoutHandle m_bl_sample_equirectangular;

  nvrhi::ShaderHandle m_cs_gbuffer_lighting;
  nvrhi::ComputePipelineHandle m_pso_gbuffer_lighting;
  nvrhi::BindingLayoutHandle m_bl_gbuffer_lighting;

  nvrhi::ShaderHandle m_cs_diffuse_ibl;
  nvrhi::ComputePipelineHandle m_pso_diffuse_ibl;
  nvrhi::BindingLayoutHandle m_bl_diffuse_ibl;

  nvrhi::ShaderHandle m_cs_specular_ibl;
  nvrhi::ComputePipelineHandle m_pso_specular_ibl;

  nvrhi::ShaderHandle m_cs_generate_mip;
  nvrhi::ComputePipelineHandle m_pso_generate_mip;

  nvrhi::ShaderHandle m_cs_integrate_brdf;
  nvrhi::ComputePipelineHandle m_pso_integrate_brdf;
  nvrhi::BindingLayoutHandle m_bl_integrate_brdf;
  nvrhi::EventQueryHandle m_ready_handle;

  nvrhi::BufferHandle m_face_inverse_projection_buffer;

  class NvRendererBackend* m_backend;
  struct NvGbuffer* m_gbuffer;
  struct Sky* m_sky;
  int filter_step = 0;
  int mip = 0;
  int x = 0;
  int y = 0;

  int m_current_set_index = 0;
  int m_locked_set_index = 0;
  int GetCurrentSetIndex();

  std::shared_ptr<struct EnvironmentRenderPass> m_environment_pass;

  const int m_pixels_per_pass = 256;

  nvrhi::BindingSetHandle GetBindingSet(int pass, int mip, int set,
                                        NvGbuffer* gbuffer_cube);

  struct EnvironmentFilterConstants {
    EnvironmentFilterConstants() {
      m_pre_exposure_mul = 1.f;
      m_roughness = 0.f;
      m_sample_count = 32.f;
      m_source_width = 2048.f;
      m_mip_bias = 0.f;
      m_offset_x = 0;
      m_offset_y = 0;
      m_offset_z = 0;
    }
    float m_pre_exposure_mul;
    float m_roughness;
    float m_sample_count;
    float m_source_width;
    float m_mip_bias;
    uint32_t m_offset_x;
    uint32_t m_offset_y;
    uint32_t m_offset_z;
  };

  struct CubeLightingConstants {
    uint32_t m_offset_x;
    uint32_t m_offset_y;
    uint32_t m_offset_z;
    uint32_t m_unused;
    glm::vec3 m_light_direction;
    float m_height;
    glm::vec4 m_light_color;
  };

  MaEnvironment(class NvRenderer* renderer);
  void Init(const std::string& texture, float pre_exposure);
  void Render(nvrhi::ICommandList* command_list, glm::dmat4 const& jitter,
              glm::dmat4 const& projection, glm::dmat4 const& view,
              glm::dmat4 const& previous_view_projection);
  void Filter(uint64_t sky_instance_id, NvGbuffer* gbuffer_cube);
  bool IsReady() const;
};

struct EnvironmentRenderPass : public FullScreenPass {
  EnvironmentRenderPass(MaEnvironment* environment);

  struct EnvironmentConstants {
    glm::mat4 m_inverse_view_projection;
    glm::mat4 m_reproject_matrix;
    glm::vec3 m_sun_direction;
    float m_height;
  };

  virtual void Init() override;

  virtual void CreatePipelineDesc(
      nvrhi::GraphicsPipelineDesc& pipeline_desc) override;

  void UpdateConstants(nvrhi::ICommandList* command_list,
                       glm::dmat4 const& jitter, glm::dmat4 const& projection,
                       glm::dmat4 const& view,
                       glm::dmat4 const& previous_view_projection);

  virtual void Render(nvrhi::ICommandList* command_list) override;

  virtual nvrhi::IFramebuffer* GetFramebuffer() override;

  struct NvGbuffer* m_gbuffer;
  MaEnvironment* m_environment;
  nvrhi::BufferHandle m_environment_constants;
  nvrhi::TextureHandle m_skybox_texture;
  nvrhi::BindingLayoutHandle m_binding_layout;
  nvrhi::BindingSetHandle m_binding_set[2];
  nvrhi::ShaderHandle m_pixel_shader;
  nvrhi::FramebufferHandle m_framebuffer;
};