#include "config.h"

#include "nvrenderer/nvrenderer.h"
#include "nvrendererbackend.h"

void MaConfig::Init(const std::filesystem::path& config_file) {
  Init(YAML::LoadFile(config_file.generic_string()));
}

void MaConfig::Init(const YAML::Node& node) {
  m_display.Init(node["display"]);
  m_shadow.Init(node["shadows"]);
  m_lighting.Init(node["lighting"]);
  m_weight_lines = node["wire_diameter_lines"].as<float>(.01f);
  m_weight_tractions = node["wire_diameter_tractions"].as<float>(.005f);
}

void MaConfig::MaConfigDisplay::Init(const YAML::Node& node) {
  m_hdr_configs[0].Init(node["hdr_config"]["sdr"]);
  m_hdr_configs[0].m_tonemap_function = MaTonemapType::Default_Srgb;
  m_hdr_configs[1].Init(node["hdr_config"]["hdr"]);
  m_hdr_configs[1].m_tonemap_function = MaTonemapType::Linear_Rec2020;
  m_adapter_index = node["adapter"].as<int>();
  m_output_index = node["output"].as<int>();
  m_disable_hdr = node["disable_hdr"].as<bool>(false);
}

MaConfig::MaConfigDisplay::MaConfigHDR MaConfig::MaConfigDisplay::GetHDRConfig()
    const {
  auto renderer = dynamic_cast<NvRenderer*>(GfxRenderer.get());
  return GetHDRConfig(renderer ? renderer->GetBackend()->IsHDRDisplay()
                               : false);
}

void MaConfig::MaConfigDisplay::MaConfigHDR::Init(const YAML::Node& node) {
  m_scene_exposure = node["pre_tonemap_exposure"].as<float>();
  m_ui_nits = node["ui_nits"].as<float>();
  m_scene_nits = node["scene_nits"].as<float>();
  m_scene_gamma = node["scene_gamma"].as<float>();
}

void MaConfig::MaConfigShadow::Init(const YAML::Node& node) {
  m_resolution = node["resolution"].as<int>();
  m_cascade_distances = node["cascades"].as<std::vector<double>>();
}

void MaConfig::MaConfigLighting::Init(const YAML::Node& node) {
  m_max_lights_per_scene = node["max_lights_per_scene"].as<int>(2048);
  m_selfillum_multiplier = node["selfillum_factor"].as<float>(1.f);
  m_freespot_multiplier = node["freespot_factor"].as<float>(10.f);
  m_luminance_threshold = node["luminance_threshold"].as<float>(.01f);
  m_min_luminance_ev = node["min_luminance_ev"].as<float>(-3.f);
  m_max_luminance_ev = node["max_luminance_ev"].as<float>(3.f);
}
