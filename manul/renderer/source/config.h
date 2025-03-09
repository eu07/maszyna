#pragma once

#include <yaml-cpp/yaml.h>

#include <filesystem>

struct MaConfig {
  enum MaTonemapType { Default_Srgb = 0, Linear_Rec2020 };
  void Init(const std::filesystem::path& config_file);
  void Init(const YAML::Node& node);
  struct MaConfigDisplay {
    void Init(const YAML::Node& node);
    struct MaConfigHDR {
      void Init(const YAML::Node& node);
      MaTonemapType m_tonemap_function;
      float m_ui_nits;
      float m_scene_nits;
      float m_scene_exposure;
      float m_scene_gamma;
      float GetMaxOutputNits() const {
        return std::max(m_ui_nits, m_scene_nits);
      }
      float GetMinOutputNits() const { return 1.e-3f; }
    };
    std::array<MaConfigHDR, 2> m_hdr_configs;
    int m_adapter_index;
    int m_output_index;
    bool m_disable_hdr = false;
    MaConfigHDR GetHDRConfig(bool is_hdr) const { return m_hdr_configs[is_hdr]; }
    MaConfigHDR GetHDRConfig() const;
  } m_display;
  struct MaConfigLighting{
    void Init(const YAML::Node& node);
    int m_max_lights_per_scene;
    float m_selfillum_multiplier;
    float m_freespot_multiplier;
    float m_luminance_threshold;
    float m_min_luminance_ev;
    float m_max_luminance_ev;
  } m_lighting;
  float m_weight_lines;
  float m_weight_tractions;
  struct MaConfigShadow {
    void Init(const YAML::Node& node);
    int m_resolution;
    std::vector<double> m_cascade_distances;
  } m_shadow;
};