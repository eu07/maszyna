#pragma once

#include "nvrenderer/nvrenderer.h"

struct InstanceCacheId {
  gfx::geometry_handle m_geometry;
  material_handle m_material;
  InstanceCacheId(gfx::geometry_handle geometry, material_handle material)
      : m_geometry(geometry), m_material(material) {}
};

template <>
struct std::hash<InstanceCacheId> {
  std::size_t operator()(const InstanceCacheId& op) const {
    size_t hash_geometry = std::hash<uint64_t>()(op.m_geometry);
    size_t hash_material = std::hash<uint32_t>()(op.m_material);
    return hash_geometry ^ hash_material + 0x9e3779b9 + (hash_geometry << 6) +
                               (hash_geometry >> 2);
  }
};

template <>
struct std::equal_to<InstanceCacheId> {
  bool operator()(const InstanceCacheId& lhs,
                  const InstanceCacheId& rhs) const {
    return lhs.m_geometry == rhs.m_geometry && lhs.m_material == rhs.m_material;
  }
};

struct InstanceCache {
  struct Instance {
    glm::mat3x4 m_transform;
    glm::mat3x4 m_transform_history;
    Instance(const glm::dmat4& transform, const glm::dmat4& transform_history)
        : m_transform(static_cast<glm::mat3x4>(glm::transpose(transform))),
          m_transform_history(
              static_cast<glm::mat3x4>(glm::transpose(transform_history))) {}
  };
  std::vector<Instance> m_instances;
};

struct InstanceBank {
  std::unordered_map<InstanceCacheId, InstanceCache> m_cache;
  void Clear() {
    for (auto& [id, cache] : m_cache) {
      cache.m_instances.clear();
    }
  }
  void AddInstance(gfx::geometry_handle geometry, material_handle material,
                   const glm::dmat4& transform, const glm::dmat4& history) {
    auto& cache = m_cache[{geometry, material}];
    cache.m_instances.emplace_back(transform, history);
  }
};