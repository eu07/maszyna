#pragma once

#include "fullscreenpass.h"
#include "nvrenderer/resource_registry.h"

struct GbufferBlitPass : public FullScreenPass, public MaResourceRegistry {
  GbufferBlitPass(class NvRenderer* renderer, struct NvGbuffer* gbuffer,
                  struct NvGbuffer* gbuffer_shadow, struct NvSsao* ssao,
                  struct MaEnvironment* environment,
                  struct MaShadowMap* shadow_map,
                  struct MaContactShadows* contact_shadows, struct Sky* sky);

  virtual void Init() override;

  virtual void CreatePipelineDesc(
      nvrhi::GraphicsPipelineDesc& pipeline_desc) override;

  void UpdateConstants(nvrhi::ICommandList* command_list, glm::dmat4& view,
                       const glm::dmat4& projection);

  void Render(nvrhi::ICommandList* command_list, glm::dmat4& view,
              const glm::dmat4& projection);
  virtual void Render(nvrhi::ICommandList* command_list) override;

  struct DrawConstants {
    glm::mat4 m_inverse_model_view;
    glm::mat4 m_inverse_projection;
    glm::vec3 m_light_dir;
    float m_altitude;
    glm::vec3 m_light_color;
    float m_time;
  };

  nvrhi::BindingLayoutHandle m_binding_layout;
  nvrhi::BindingSetHandle m_binding_set[2];
  nvrhi::ShaderHandle m_pixel_shader;
  nvrhi::FramebufferHandle m_framebuffer;
  nvrhi::BufferHandle m_draw_constants;
  nvrhi::TextureHandle m_scene_depth;

  NvGbuffer* m_gbuffer;
  NvGbuffer* m_gbuffer_shadow;
  NvSsao* m_ssao;
  MaEnvironment* m_environment;
  MaShadowMap* m_shadow_map;
  MaContactShadows* m_contact_shadows;
  Sky* m_sky;

  nvrhi::ComputePipelineHandle m_pso;

  nvrhi::TextureHandle m_output;
  virtual nvrhi::IFramebuffer* GetFramebuffer() override;
};