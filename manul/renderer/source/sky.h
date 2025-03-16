#pragma once

#include "fullscreenpass.h"
#include "nvrenderer/resource_registry.h"

struct Sky : public MaResourceRegistry {
  Sky(class NvRenderer* renderer);

  void Init();
  void Render(nvrhi::ICommandList* command_list, const glm::dmat4& projection,
              const glm::dmat4& view);
  struct SkyConstants {
    glm::vec4 g_AerosolAbsorptionCrossSection;
    glm::vec4 g_AerosolScatteringCrossSection;
    glm::vec4 g_GroundAlbedo;
    float g_AerosolHeightScale;
    float g_AerosolTurbidity;
    float g_AerosolBaseDensity;
    float g_AerosolBackgroundDividedByBaseDensity;
    float g_OzoneMean;
    float g_FogDensity;
    float g_FogHeightOffset;
    float g_FogHeightScale;
  };
  nvrhi::BufferHandle m_sky_constants;
  enum class AerosolPreset {
    Rural,
    Urban,
    Num
  } m_aerosol_preset = AerosolPreset::Rural;
  float m_visibility = 1e4f;
  float m_fog_height_offset = 0.f;
  float m_fog_height_scale = .1f;
  float m_aerosol_turbidity = 1.f;
  float m_ground_albedo = .3f;
  bool m_gui_active = false;
  void UpdateConstants(nvrhi::ICommandList* command_list);
  void OnGui();
  void ShowGui();
  static const char* GetAerosolTypeDesc(AerosolPreset preset);
  class NvRendererBackend* m_backend;
  std::shared_ptr<struct SkyTransmittancePass> m_transmittance_pass;
  std::shared_ptr<struct SkyAerialLut> m_aerial_lut;
  float GetOzoneMean() const;
  glm::vec4 GetAerosolAbsorptionCrossSection() const;
  glm::vec4 GetAerosolScatteringCrossSection() const;
  float GetAerosolBaseDensity() const;
  float GetAerosolBackgroundDividedByBaseDensity() const;
  float GetAerosolHeightScale() const;
  float GetFogDensity() const;
  glm::vec4 GetMolecularScatteringCoefficient(float h) const;
  glm::vec4 GetMolecularAbsorptionCoefficient(float h) const;
  glm::vec4 GetFogScatteringCoefficient(float h) const;
  glm::vec3 CalcSunColor() const;
  glm::vec3 CalcMoonColor() const;
  void CalcLighting(glm::vec3& direction, glm::vec3& color) const;
  static glm::vec3 LinearSrgbFromSpectralSamples(glm::vec4 L);
  static float RaySphereIntersection(glm::vec3 ro, glm::vec3 rd, float radius);
  float GetAerosolDensity(float h) const;
  void GetAtmosphereCollisionCoefficients(
      float h, glm::vec4& aerosol_absorption, glm::vec4& aerosol_scattering,
      glm::vec4& molecular_absorption, glm::vec4& molecular_scattering,
      glm::vec4& fog_scattering, glm::vec4& extinction) const;
  glm::vec4 ComputeTransmittance(const glm::vec3& dir, int steps) const;
};

struct SkyTransmittancePass : public FullScreenPass {
  SkyTransmittancePass(Sky* sky) : FullScreenPass(sky->m_backend), m_sky(sky) {}

  virtual void Init() override;

  virtual void CreatePipelineDesc(
      nvrhi::GraphicsPipelineDesc& pipeline_desc) override;

  virtual void Render(nvrhi::ICommandList* command_list) override;

  virtual nvrhi::IFramebuffer* GetFramebuffer() override;

  nvrhi::BindingLayoutHandle m_binding_layout;
  nvrhi::BindingSetHandle m_binding_set;
  nvrhi::ShaderHandle m_pixel_shader;
  nvrhi::FramebufferHandle m_framebuffer;

  nvrhi::ITexture* m_source;
  nvrhi::Format m_output_format;

  nvrhi::TextureHandle m_output;

  Sky* m_sky;
};

struct SkyAerialLut {
  struct DispatchConstants {
    glm::mat4 g_InverseView;  // DO NOT TRANSPOSE
    glm::mat4 g_InverseProjection;
    glm::vec3 g_SunDir;
    float g_Altitude;
    glm::vec3 g_MoonDir;
    float g_MaxDepth;
  };
  SkyAerialLut(Sky* sky) : m_sky(sky) {}
  void Init();
  void Render(nvrhi::ICommandList* command_list, const glm::dmat4& projection,
              const glm::dmat4& view);
  nvrhi::TextureHandle m_lut;
  nvrhi::TextureHandle m_sky_texture;
  nvrhi::BufferHandle m_constant_buffer;
  nvrhi::ComputePipelineHandle m_pso_lut;
  nvrhi::ComputePipelineHandle m_pso_sky;
  int m_sky_width;
  int m_sky_height;
  int m_lut_width;
  int m_lut_height;
  int m_lut_slices;
  nvrhi::BindingSetHandle m_bindings_lut;
  nvrhi::BindingSetHandle m_bindings_sky;
  Sky* m_sky;
};
