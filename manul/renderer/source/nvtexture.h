#pragma once

#include <bitset>
#include <memory>
#include <unordered_map>
#include <nvrhi/nvrhi.h>

#include <glm/glm.hpp>
#include "interfaces/ITexture.h"
#include "nvrenderer/nvrenderer_enums.h"

enum MaTextureTraits {
  MaTextureTraits_Sharpen,
  MaTextureTraits_ClampS,
  MaTextureTraits_ClampT,

  MaTextureTraits_NoMipBias,
  MaTextureTraits_NoAnisotropy,

  MaTextureTraits_Num
};

using TextureTraitFlags = std::bitset<MaTextureTraits_Num>;

struct opengl_texture;

struct NvTexture : public ITexture {
  virtual bool create(bool Static = false) override {
    return CreateRhiTexture();
  }
  virtual int get_width() const override { return m_width; }
  virtual int get_height() const override { return m_height; }
  virtual size_t get_id() const override {
    return reinterpret_cast<size_t>(m_rhi_texture.Get());
  }
  virtual void release() override {}
  virtual void make_stub() override {}
  virtual std::string_view get_traits() const override { return ""; }
  virtual std::string_view get_name() const override {
    return m_sz_texture->get_name();
  }
  virtual std::string_view get_type() const override {
    return m_sz_texture->get_type();
  }
  virtual bool is_stub() const override { return false; }
  virtual bool get_has_alpha() const override { return m_has_alpha; }
  virtual bool get_is_ready() const override { return m_rhi_texture; }
  virtual void set_components_hint(int format_hint) override;
  virtual void make_from_memory(size_t width, size_t height,
                                const uint8_t* data) override;
  virtual void update_from_memory(size_t width, size_t height,
                                  const uint8_t* data) override;

  void Load(int size_bias);
  bool LoadDDS(std::string& data, int size_bias);
  bool IsLoaded() const;
  std::shared_ptr<ITexture> m_sz_texture;
  nvrhi::TextureHandle m_rhi_texture;
  std::vector<std::pair<int, std::string>> m_data;
  TextureTraitFlags GetTraits() const { return 0; }
  int m_width;
  int m_height;
  int m_mipcount;
  int m_slicecount;
  bool m_srgb;
  bool m_bc;
  bool m_has_alpha;
  nvrhi::Format m_format;
  size_t m_gc_slot;
  bool CreateRhiTexture();
  uint64_t m_last_change;
  static uint64_t s_change_counter;
  uint64_t m_own_handle;
};

class NvTextureManager {
 public:
  struct TextureGCSlot {
    size_t m_index;
    glm::dvec3 m_last_position_requested;
  };
  NvTextureManager(class NvRenderer* renderer);
  size_t FetchTexture(std::string path, int format_hint, int size_bias,
                      bool unload_on_location);
  void UpdateLastUse(size_t handle, const glm::dvec3& location);
  bool IsValidHandle(size_t handle);
  NvTexture* GetTexture(size_t handle);
  nvrhi::ITexture* GetRhiTexture(size_t handle,
                                 nvrhi::ICommandList* command_list = nullptr);
  TextureTraitFlags GetTraits(size_t handle);
  nvrhi::SamplerHandle GetSamplerForTraits(TextureTraitFlags traits,
                                           RendererEnums::RenderPassType pass);
  std::vector<TextureGCSlot> m_unloadable_textures;

  void Cleanup(const glm::dvec3& location);

 private:
  std::unordered_map<TextureTraitFlags, nvrhi::SamplerHandle> m_samplers;
  class NvRendererBackend* m_backend;
  std::unordered_map<std::string, size_t> m_texture_map;
  std::vector<std::shared_ptr<NvTexture>> m_texture_cache;
};
