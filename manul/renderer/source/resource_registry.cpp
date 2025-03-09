#include "nvrenderer/resource_registry.h"

#include "Logs.h"
#include "fmt/compile.h"
#include "nvtexture.h"
#include "nvrenderer/nvrenderer.h"

MaResourceMappingSet& MaResourceMappingSet::Add(
    const MaResourceMapping& mapping) {
  m_mappings.push_back(mapping);
  return *this;
}

void MaResourceMappingSet::ToBindingLayout(
    nvrhi::BindingLayoutDesc& desc) const {
  for (const auto& [slot, size, key, type] : m_mappings) {
    nvrhi::BindingLayoutItem item{};
    item.type = type;
    item.slot = slot;
    item.size = size;
    desc.addItem(item);
  }
}

bool MaResourceMappingSet::ToBindingSet(
    nvrhi::BindingSetDesc& desc, const MaResourceRegistry* registry,
    const nvrhi::BindingLayoutDesc* reference_layout) const {
  bool is_valid = true;
  for (const auto& [slot, size, key, type] : m_mappings) {
    MaResourceView resource{};
    switch (type) {
      case nvrhi::ResourceType::PushConstants:
      case nvrhi::ResourceType::None:
        break;
      default:
        resource = registry->GetResource(key, type);
    }
    bool item_is_valid = resource.IsValid();
    bool item_is_present = true;
    nvrhi::BindingSetItem item{};
    item.slot = slot;
    item.type = type;
    item.resourceHandle = resource.m_resource;
    item.dimension = nvrhi::TextureDimension::Unknown;
    item.format = nvrhi::Format::UNKNOWN;
    switch (type) {
      case nvrhi::ResourceType::PushConstants:
        item.range.byteOffset = 0;
        item.range.byteSize = size;
        item_is_valid = true;
        break;
      case nvrhi::ResourceType::RawBuffer_SRV:
      case nvrhi::ResourceType::RawBuffer_UAV:
      case nvrhi::ResourceType::StructuredBuffer_SRV:
      case nvrhi::ResourceType::StructuredBuffer_UAV:
      case nvrhi::ResourceType::TypedBuffer_SRV:
      case nvrhi::ResourceType::TypedBuffer_UAV:
      case nvrhi::ResourceType::ConstantBuffer:
      case nvrhi::ResourceType::VolatileConstantBuffer:
        item.range = nvrhi::EntireBuffer;
        break;
      case nvrhi::ResourceType::Texture_SRV:
      case nvrhi::ResourceType::Texture_UAV:
        item.subresources = nvrhi::AllSubresources;
      default:;
    }
    if (reference_layout) {
      item_is_present = false;
      for (const auto& binding : reference_layout->bindings) {
        if (binding.type != item.type || binding.slot != item.slot) continue;
        item_is_present = true;
        break;
      }
    }
    if (item_is_valid && item_is_present) desc.addItem(item);
    is_valid &= item_is_valid;
  }
  return is_valid;
}

MaResourceView MaResourceRegistry::GetResource(
    const entt::hashed_string& key,
    const nvrhi::ResourceType requested_type) const {
  auto registry = this;
  while (registry) {
    if (const auto resource = registry->GetResourceLocal(key, requested_type);
        resource.IsValid())
      return resource;
    registry = registry->m_parent;
  }
  std::string what = fmt::format("Unable to locate resource {:s} [{:#x}]",
                                 key.data(), key.value());
  ErrorLog(fmt::format("Unable to locate resource {:s} [{:#x}]", key.data(),
                       key.value()));
  throw std::runtime_error(what.c_str());
  return {};
}

MaResourceRegistry::MaResourceRegistry(MaResourceRegistry* parent)
    : m_parent(parent) {}

void MaResourceRegistry::InitResourceRegistry() {
  if (m_parent) {
    m_texture_manager =
        std::shared_ptr<NvTextureManager>(m_parent->GetTextureManager());
  } else {
    m_texture_manager =
        std::make_shared<NvTextureManager>(dynamic_cast<NvRenderer*>(this));
  }
}

void MaResourceRegistry::RegisterResource(const bool global,
                                          const entt::hashed_string& key,
                                          nvrhi::IResource* resource,
                                          const nvrhi::ResourceType type) {
  auto registry = this;
  while (global && registry->m_parent) registry = registry->m_parent;
  registry->m_resource_views[key] = {resource, type};
}

void MaResourceRegistry::RegisterTexture(const entt::hashed_string& key,
                                         const size_t handle) {
  m_streaming_textures[key] = handle;
}

void MaResourceRegistry::RegisterResource(const bool global, const char* key,
                                          nvrhi::IResource* resource,
                                          const nvrhi::ResourceType type) {
  RegisterResource(global, static_cast<entt::hashed_string>(key), resource,
                   type);
}

void MaResourceRegistry::RegisterTexture(const char* key, const size_t handle) {
  RegisterTexture(static_cast<entt::hashed_string>(key), handle);
}

MaResourceView MaResourceRegistry::GetResourceLocal(
    const entt::hashed_string& key,
    const nvrhi::ResourceType requested_type) const {
  // Attempt to fetch streaming texture
  if (requested_type == nvrhi::ResourceType::Texture_SRV) {
    if (const auto it = m_streaming_textures.find(key);
        it != m_streaming_textures.end()) {
      if (const auto texture_manager = GetTextureManager();
          texture_manager && texture_manager->IsValidHandle(it->second))
        return {texture_manager->GetRhiTexture(it->second),
                nvrhi::ResourceType::Texture_SRV};
    }
  }
  // Attempt to fetch static resource
  if (const auto it = m_resource_views.find(key);
      it != m_resource_views.end() && requested_type == it->second.m_type) {
    return it->second;
  }
  return {};
}

NvTextureManager* MaResourceRegistry::GetTextureManager() const {
  auto registry = this;
  while (!registry->m_texture_manager && registry->m_parent)
    registry = registry->m_parent;
  return registry->m_texture_manager.get();
}

uint64_t MaResourceRegistry::GetLastStreamingTextureUpdate() const {
  uint64_t last_update = 1;
  const auto texture_manager = GetTextureManager();
  for (const auto& [key, texture] : m_streaming_textures) {
    if (!texture_manager->IsValidHandle(texture)) continue;
    last_update = std::max(last_update,
                           texture_manager->GetTexture(texture)->m_last_change);
  }
  return last_update;
}
void MaResourceRegistry::UpdateLastStreamingTextureUse(
    const glm::dvec3& location) const {
  const auto texture_manager = GetTextureManager();
  for (const auto& [key, texture] : m_streaming_textures) {
    texture_manager->UpdateLastUse(texture, location);
  }
}
