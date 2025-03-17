#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <optional>
#include <unordered_map>

#include <glm/glm.hpp>
#include <string>

struct MaterialAdapterInternal {
  virtual ~MaterialAdapterInternal() = default;
  virtual std::string_view GetTexturePathForEntry(
      std::string_view key) const = 0;
  virtual int GetTextureSizeBiasForEntry(
      std::string_view key) const = 0;
  virtual std::string_view GetShader() const = 0;
  virtual int GetShadowRank() const { return 0; }
  virtual glm::dvec2 GetSize() const { return {-1., -1.}; }
  virtual std::optional<float> GetOpacity() const = 0;
  virtual std::optional<bool> GetMaskedShadowOverride() const;
};

struct MaterialAdapter {
  std::string_view GetTexturePathForEntry(std::string_view key) const;
  int GetTextureSizeBiasForEntry(std::string_view key) const;
  std::string_view GetShader() const;
  int GetShadowRank() const;
  glm::dvec2 GetSize() const;
  std::optional<float> GetOpacity() const;
  bool Parse(std::string_view filepath);
  std::shared_ptr<MaterialAdapterInternal> m_material_internal;
  std::optional<bool> GetMaskedShadowOverride() const;
};

struct MaterialAdapterManul : public MaterialAdapterInternal {
  static std::shared_ptr<MaterialAdapterInternal> Parse(std::string_view path);
  virtual std::string_view GetTexturePathForEntry(
      std::string_view key) const override;
  virtual int GetTextureSizeBiasForEntry(std::string_view key) const override;
  virtual std::string_view GetShader() const override;
  virtual std::optional<float> GetOpacity() const override;
  std::string m_shader;
  std::unordered_map<std::string, std::string> m_textures;
  int m_size_bias;
  std::optional<float> m_alpha_threshold = std::nullopt;
  glm::dvec2 m_size{-1., -1.};
  virtual glm::dvec2 GetSize() const override { return m_size; }
};

struct MaterialAdapterLegacyMatFile : public MaterialAdapterInternal {
  static std::shared_ptr<MaterialAdapterInternal> Parse(std::string_view path);
  virtual std::string_view GetTexturePathForEntry(
      std::string_view key) const override;
  virtual int GetTextureSizeBiasForEntry(std::string_view key) const override {
    return 0;
  }
  virtual std::string_view GetShader() const override;
  bool ParseLevel(class cParser& parser, int priority);
  std::optional<float> m_opacity = std::nullopt;
  int m_shadow_rank;
  float m_selfillum;
  std::string m_tex_diffuse;
  std::string m_tex_normal;
  std::string m_shader;
  int m_shader_priority = -1;
  struct TextureEntry {
    int m_priority = -1;
    std::string m_path = "";
  };
  glm::dvec2 m_size{-1., -1.};
  virtual glm::dvec2 GetSize() const override { return m_size; }
  bool HasNormalMap() const;
  bool HasSpecGlossMap() const;
  bool IsSpecGlossShader() const;
  bool IsNormalMapShader() const;
  bool IsReflectionShader() const;
  std::unordered_map<std::string, TextureEntry> m_texture_mapping;
  virtual int GetShadowRank() const override { return m_shadow_rank; }
  virtual std::optional<float> GetOpacity() const override;
  int m_opacity_priority = -1;
  int m_selfillum_priority = -1;
  int m_diffuse_priority = -1;
  int m_normal_priority = -1;
};

struct MaterialAdapterSingleTexture : public MaterialAdapterInternal {
  static std::shared_ptr<MaterialAdapterInternal> Parse(std::string_view path);
  MaterialAdapterSingleTexture(std::string_view path) : m_tex_diffuse(path){};
  virtual std::string_view GetTexturePathForEntry(
      std::string_view key) const override;
  virtual int GetTextureSizeBiasForEntry(std::string_view key) const override {
    return 0;
  }
  virtual std::string_view GetShader() const override;
  virtual std::optional<float> GetOpacity() const override;
  std::string m_tex_diffuse;
};