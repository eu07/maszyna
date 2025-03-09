#pragma once

#include <nvrhi/nvrhi.h>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

class NvTextureManager;
class MaResourceRegistry;

struct MaResourceMapping {
  int m_slot;
  int m_size;
  entt::hashed_string m_key;
  nvrhi::ResourceType m_type;

#define MA_RESOURCE_MAPPING_INITIALIZER(type)                   \
  template <typename KeyType>                                   \
  static MaResourceMapping type(int slot, const KeyType& key) { \
    MaResourceMapping mapping{};                                \
    mapping.m_slot = slot;                                      \
    mapping.m_key = static_cast<entt::hashed_string>(key);      \
    mapping.m_type = nvrhi::ResourceType::type;                 \
    return mapping;                                             \
  }

  template <typename KeyType>
  static MaResourceMapping Texture_SRV(int slot, const KeyType& key) {
    MaResourceMapping mapping{};
    mapping.m_slot = slot;
    mapping.m_key = static_cast<entt::hashed_string>(key);
    mapping.m_type = nvrhi::ResourceType::Texture_SRV;
    return mapping;
  }
  MA_RESOURCE_MAPPING_INITIALIZER(Texture_UAV)
  MA_RESOURCE_MAPPING_INITIALIZER(TypedBuffer_SRV)
  MA_RESOURCE_MAPPING_INITIALIZER(TypedBuffer_UAV)
  MA_RESOURCE_MAPPING_INITIALIZER(StructuredBuffer_SRV)
  MA_RESOURCE_MAPPING_INITIALIZER(StructuredBuffer_UAV)
  MA_RESOURCE_MAPPING_INITIALIZER(RawBuffer_SRV)
  MA_RESOURCE_MAPPING_INITIALIZER(RawBuffer_UAV)
  MA_RESOURCE_MAPPING_INITIALIZER(ConstantBuffer)
  MA_RESOURCE_MAPPING_INITIALIZER(VolatileConstantBuffer)
  MA_RESOURCE_MAPPING_INITIALIZER(Sampler)
  MA_RESOURCE_MAPPING_INITIALIZER(RayTracingAccelStruct)

#undef MA_RESOURCE_MAPPING_INITIALIZER

  static MaResourceMapping PushConstants(const int slot, const int size) {
    MaResourceMapping mapping{};
    mapping.m_slot = slot;
    mapping.m_type = nvrhi::ResourceType::PushConstants;
    mapping.m_size = size;
    return mapping;
  }

  template <typename PushType>
  static MaResourceMapping PushConstants(const int slot) {
    MaResourceMapping mapping{};
    mapping.m_slot = slot;
    mapping.m_type = nvrhi::ResourceType::PushConstants;
    mapping.m_size = sizeof(PushType);
    return mapping;
  }
};

struct MaResourceMappingSet {
  nvrhi::static_vector<MaResourceMapping, 128> m_mappings;
  MaResourceMappingSet& Add(const MaResourceMapping& mapping);
  void ToBindingLayout(nvrhi::BindingLayoutDesc& desc) const;
  bool ToBindingSet(nvrhi::BindingSetDesc& desc,
                    const MaResourceRegistry* registry,
      const nvrhi::BindingLayoutDesc* reference_layout = nullptr) const;
};

struct MaResourceView {
  nvrhi::IResource* m_resource = nullptr;
  nvrhi::ResourceType m_type = nvrhi::ResourceType::None;
  [[nodiscard]] bool IsValid() const {
    return m_resource && m_type != nvrhi::ResourceType::None;
  }
};

class MaResourceRegistry {
 public:
  [[nodiscard]] MaResourceView GetResource(
      const entt::hashed_string& key, nvrhi::ResourceType requested_type) const;
  [[nodiscard]] NvTextureManager* GetTextureManager() const;
  [[nodiscard]] uint64_t GetLastStreamingTextureUpdate() const;
  void UpdateLastStreamingTextureUse(const glm::dvec3& location) const;
  void RegisterResource(bool global, const entt::hashed_string& key,
                        nvrhi::IResource* resource, nvrhi::ResourceType type);
  void RegisterTexture(const entt::hashed_string& key, size_t handle);
  void RegisterResource(bool global, const char* key,
                        nvrhi::IResource* resource, nvrhi::ResourceType type);
  void RegisterTexture(const char* key, size_t handle);
  virtual ~MaResourceRegistry() = default;

 protected:
  explicit MaResourceRegistry(MaResourceRegistry* parent);
  void InitResourceRegistry();

 private:
  [[nodiscard]] MaResourceView GetResourceLocal(
      const entt::hashed_string& key, nvrhi::ResourceType requested_type) const;
  MaResourceRegistry* m_parent = nullptr;
  std::shared_ptr<NvTextureManager> m_texture_manager = nullptr;
  entt::dense_map<entt::id_type, MaResourceView> m_resource_views{};
  entt::dense_map<entt::id_type, size_t> m_streaming_textures{};
};