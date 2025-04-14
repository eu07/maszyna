#include "materialparser.h"

#include "nvrenderer/nvrenderer.h"

#include <Globals.h>
#include <Logs.h>
#include <fmt/format.h>
#include <material.h>
#include <parser.h>
#include <utilities.h>
#include <yaml-cpp/yaml.h>
#include <string>

std::string_view MaterialAdapter::GetTexturePathForEntry(
    std::string_view key) const {
  if (m_material_internal) {
    return m_material_internal->GetTexturePathForEntry(key);
  }
  return std::string_view();
}

int MaterialAdapter::GetTextureSizeBiasForEntry(std::string_view key) const {
  if (m_material_internal) {
    return m_material_internal->GetTextureSizeBiasForEntry(key);
  }
  return 0;
}

std::string_view MaterialAdapter::GetShader() const {
  if (m_material_internal) {
    return m_material_internal->GetShader();
  }
  return std::string_view();
}

int MaterialAdapter::GetShadowRank() const {
  if (m_material_internal) {
    return m_material_internal->GetShadowRank();
  }
  return 0;
}

glm::dvec2 MaterialAdapter::GetSize() const {
  if (m_material_internal) {
    return m_material_internal->GetSize();
  }
  return {-1., -1.};
}

std::optional<float> MaterialAdapter::GetOpacity() const {
  if (m_material_internal) {
    return m_material_internal->GetOpacity();
  }
  return std::nullopt;
}

bool MaterialAdapter::Parse(std::string_view filepath) {
  if (m_material_internal = MaterialAdapterManul::Parse(filepath);
      m_material_internal) {
    return true;
  }
  if (m_material_internal = MaterialAdapterLegacyMatFile::Parse(filepath);
      m_material_internal) {
    return true;
  }
  if (m_material_internal = MaterialAdapterSingleTexture::Parse(filepath);
      m_material_internal) {
    return true;
  }
  return false;
}

std::optional<bool> MaterialAdapter::GetMaskedShadowOverride() const {
  if (m_material_internal) {
    return m_material_internal->GetMaskedShadowOverride();
  }
  return std::nullopt;
}

std::shared_ptr<MaterialAdapterInternal> MaterialAdapterLegacyMatFile::Parse(
    std::string_view path) {
  auto const locator{material_manager::find_on_disk(std::string(path))};

  if (locator.first.size()) {
    // try to parse located file resource
    cParser materialparser(locator.first + locator.second,
                           cParser::buffer_FILE);
    auto material = std::make_shared<MaterialAdapterLegacyMatFile>();
    while (material->ParseLevel(materialparser, 0)) {
    }
    return material;
  } else {
    return nullptr;
  }
}

std::string_view MaterialAdapterLegacyMatFile::GetTexturePathForEntry(
    std::string_view key) const {
  if (key == "diffuse") {
    if (auto it = m_texture_mapping.find("_diffuse:");
        it != m_texture_mapping.end()) {
      return it->second.m_path;
    }
    if (auto it = m_texture_mapping.find("1:"); it != m_texture_mapping.end()) {
      return it->second.m_path;
    }
  } else if (key == "normal") {
    if (auto it = m_texture_mapping.find("_normals:");
        it != m_texture_mapping.end()) {
      return it->second.m_path;
        }
    if (auto it = m_texture_mapping.find("_normalmap:");
        it != m_texture_mapping.end()) {
      return it->second.m_path;
        }
    if (auto it = m_texture_mapping.find("_reflmap:");
        it != m_texture_mapping.end()) {
      return it->second.m_path;
        }
    if (auto it = m_texture_mapping.find("2:"); it != m_texture_mapping.end()) {
      return it->second.m_path;
    }
  } else {
    if (auto it = m_texture_mapping.find(fmt::format("_{}:", key));
        it != m_texture_mapping.end()) {
      return it->second.m_path;
    }
  }
  return std::string_view();
}

std::string_view MaterialAdapterLegacyMatFile::GetShader() const {
  if (m_shader == "water") {
    return "legacy_water";
  }
  if (IsSpecGlossShader() && HasSpecGlossMap()) {
    if (IsNormalMapShader() && HasNormalMap()) {
      return "legacy_normalmap_specgloss";
    }
    return "legacy_specgloss";
  }
  if (IsNormalMapShader() && HasNormalMap()) {
    return "legacy_normalmap";
  }
  if (HasNormalMap()) {
    return "legacy_reflection";
  }
  return "legacy";
}

bool MaterialAdapterLegacyMatFile::ParseLevel(cParser& parser, int priority) {
  // NOTE: comma can be part of legacy file names, so we don't treat it as a
  // separator here
  auto key{parser.getToken<std::string>(true, "\n\r\t  ;[]")};
  // key can be an actual key or block end
  if ((true == key.empty()) || (key == "}")) {
    return false;
  }

  if (priority != -1) {
    // regular attribute processing mode

    // mark potential material change
    // update_on_weather_change |= is_weather(key);
    // update_on_season_change |= is_season(key);

    if (key == Global.Weather) {
      // weather textures override generic (pri 0) and seasonal (pri 1) textures
      // seasonal weather textures (pri 1+2=3) override generic weather (pri 2)
      // textures skip the opening bracket
      auto const value{parser.getToken<std::string>(true, "\n\r\t ;")};
      while (ParseLevel(parser, priority + 2)) {
        ;  // all work is done in the header
      }
    } else if (key == Global.Season) {
      // seasonal textures override generic textures
      // skip the opening bracket
      auto const value{parser.getToken<std::string>(true, "\n\r\t ;")};
      while (ParseLevel(parser, priority + 1)) {
        ;  // all work is done in the header
      }
    }

    else if (key.compare(0, 7, "texture") == 0) {
      key.erase(0, 7);

      auto value{deserialize_random_set(parser)};
      replace_slashes(value);
      auto& [tex_priority, texture] = m_texture_mapping[key];
      if (tex_priority < priority) {
        texture = value;
        tex_priority = priority;
      }
    } else if (key.compare(0, 5, "param") == 0) {
      // key.erase(0, 5);
      //
      parser.getToken<std::string>(true, "\n\r\t;");
      // std::istringstream stream(value);
      // glm::vec4 data;
      // stream >> data.r;
      // stream >> data.g;
      // stream >> data.b;
      // stream >> data.a;
      //
      // auto it = parse_info->param_mapping.find(key);
      // if (it == parse_info->param_mapping.end())
      //   parse_info->param_mapping.emplace(
      //       std::make_pair(key, parse_info_s::param_def({data, Priority})));
      // else if (Priority > it->second.priority) {
      //   parse_info->param_mapping.erase(it);
      //   parse_info->param_mapping.emplace(
      //       std::make_pair(key, parse_info_s::param_def({data, Priority})));
      // }
    } else if (key == "shader:" &&
               (m_shader.empty() || priority > m_shader_priority)) {
      m_shader = deserialize_random_set(parser);
      m_shader_priority = priority;
    } else if (key == "opacity:" && priority > m_opacity_priority) {
      std::string value = deserialize_random_set(parser);
      m_opacity = std::stof(value);  // m7t: handle exception
      m_opacity_priority = priority;
    } else if (key == "selfillum:" && priority > m_selfillum_priority) {
      std::string value = deserialize_random_set(parser);
      m_selfillum = std::stof(value);  // m7t: handle exception
      m_selfillum_priority = priority;
    } else if (key == "glossiness:" /* && Priority > m_glossiness_priority*/) {
      deserialize_random_set(parser);
      // glossiness = std::stof(value);  // m7t: handle exception
      // m_glossiness_priority = Priority;
    } else if (key == "shadow_rank:") {
      std::string value = deserialize_random_set(parser);
      m_shadow_rank = std::stoi(value);  // m7t: handle exception
    } else if (key == "size:") {
      parser.getTokens(2);
      parser >> m_size.x >> m_size.y;
    } else {
      auto const value = parser.getToken<std::string>(true, "\n\r\t ;");
      if (value == "{") {
        // unrecognized or ignored token, but comes with attribute block and
        // potential further nesting go through it and discard the content
        while (ParseLevel(parser, -1)) {
          ;  // all work is done in the header
        }
      }
    }
  } else {
    // discard mode; ignores all retrieved tokens
    auto const value{parser.getToken<std::string>(true, "\n\r\t ;")};
    if (value == "{") {
      // ignored tokens can come with their own blocks, ignore these recursively
      // go through it and discard the content
      while (true == ParseLevel(parser, -1)) {
        ;  // all work is done in the header
      }
    }
  }

  return true;  // return value marks a key: value pair was extracted, nothing
  // about whether it's recognized
}

bool MaterialAdapterLegacyMatFile::HasNormalMap() const {
  return m_texture_mapping.find("_normals:") != m_texture_mapping.end() ||
         m_texture_mapping.find("_normalmap:") != m_texture_mapping.end() ||
         m_texture_mapping.find("_reflmap:") != m_texture_mapping.end() ||
         m_texture_mapping.find("2:") != m_texture_mapping.end();
}

bool MaterialAdapterLegacyMatFile::HasSpecGlossMap() const {
  return m_texture_mapping.find("_specgloss:") != m_texture_mapping.end();
}

bool MaterialAdapterLegacyMatFile::IsNormalMapShader() const {
  return m_shader.find("normalmap") != std::string::npos ||
         m_shader.find("parallax") != std::string::npos;
}

bool MaterialAdapterLegacyMatFile::IsReflectionShader() const {
  return m_shader.find("reflmap") != std::string::npos;
}

bool MaterialAdapterLegacyMatFile::IsSpecGlossShader() const {
  return m_shader.find("specgloss") != std::string::npos;
}

std::optional<float> MaterialAdapterLegacyMatFile::GetOpacity() const {
  return m_opacity;
}

std::shared_ptr<MaterialAdapterInternal> MaterialAdapterSingleTexture::Parse(
    std::string_view path) {
  return std::make_shared<MaterialAdapterSingleTexture>(path);
}

std::string_view MaterialAdapterSingleTexture::GetTexturePathForEntry(
    std::string_view key) const {
  if (key == "diffuse") {
    return m_tex_diffuse;
  }
  return std::string_view();
}

std::string_view MaterialAdapterSingleTexture::GetShader() const {
  return "legacy";
}

std::optional<float> MaterialAdapterSingleTexture::GetOpacity() const {
  return std::nullopt;
}

std::shared_ptr<MaterialAdapterInternal> MaterialAdapterManul::Parse(
    std::string_view path) {
  std::string entries[]{
      Global.asCurrentTexturePath + std::string(path) + ".manul",
      std::string(path) + ".manul"};
  for (auto it = std::begin(entries); it != std::end(entries); ++it) {
    if (!std::filesystem::exists(*it)) {
      continue;
    }
    auto root = YAML::LoadFile(*it);
    auto material = std::make_shared<MaterialAdapterManul>();
    material->m_shader = root["shader"].as<std::string>();
    for (const auto it : root["textures"]) {
      material->m_textures[it.first.as<std::string>()] =
          it.second.as<std::string>();
    }
    if (auto alpha_threshold = root["alpha_threshold"];
        alpha_threshold.IsScalar()) {
      material->m_alpha_threshold = alpha_threshold.as<float>();
    }
    if (auto size_bias = root["max_resolution_bias"]; size_bias.IsScalar())
      material->m_size_bias = size_bias.as<int>();
    return material;
  }
  return nullptr;
}

std::string_view MaterialAdapterManul::GetTexturePathForEntry(
    std::string_view key) const {
  if (auto it = m_textures.find(std::string(key)); it != m_textures.end()) {
    return it->second;
  }
  return std::string_view();
}

int MaterialAdapterManul::GetTextureSizeBiasForEntry(
    std::string_view key) const {
  return m_size_bias;
}

std::string_view MaterialAdapterManul::GetShader() const { return m_shader; }

std::optional<float> MaterialAdapterManul::GetOpacity() const {
  return m_alpha_threshold;
}

std::optional<bool> MaterialAdapterInternal::GetMaskedShadowOverride() const {
  auto threshold = GetOpacity();
  if (threshold.has_value()) {
    return !!*threshold;
  }
  return std::nullopt;
}
