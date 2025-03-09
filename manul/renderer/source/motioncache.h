#pragma once

#include <cstdint>
#include <functional>
#include <glm/glm.hpp>

struct MotionCacheId {
  union {
    struct {
      void const* m_instance;
      void const* m_sub;
    };
    struct {
      size_t m_key[2];
    };
  };
  MotionCacheId(void const* instance, void const* sub)
      : m_instance(instance), m_sub(sub) {}
};

template <>
struct std::hash<MotionCacheId> {
  std::size_t operator()(const MotionCacheId& k) const {
    using hash = std::hash<size_t>;
    size_t hash_instance = hash()(k.m_key[0]);
    size_t hash_sub = hash()(k.m_key[1]);
    return hash_instance ^
           hash_sub + 0x9e3779b9 + (hash_instance << 6) + (hash_instance >> 2);
  }
};

template <>
struct std::equal_to<MotionCacheId> {
  bool operator()(const MotionCacheId& lhs, const MotionCacheId& rhs) const{
    return lhs.m_key[0] == rhs.m_key[0] && lhs.m_key[1] == rhs.m_key[1];
  }
};

struct MotionCacheEntry {
  glm::dmat4x3 m_cached_transform{};
  glm::dmat4x3 m_history_transform{};
  size_t m_cached_frame = static_cast<size_t>(-1);
};

struct MotionCacheScope {
  friend struct MotionCache;
  ~MotionCacheScope();

 private:
  MotionCacheScope(void const* instance, struct MotionCache* cache);
  void const* m_instance;
  MotionCache* m_cache;
};

struct MotionCache {
  friend struct MotionCacheScope;
  [[nodiscard]] MotionCacheScope SetInstance(void const* instance) {
    return {instance, this};
  }
  MotionCacheEntry& Get(void const* sub) {
    assert(m_active_scope);
    return m_cache[MotionCacheId{m_active_scope->m_instance, sub}];
  }

 private:
  std::unordered_map<MotionCacheId, MotionCacheEntry> m_cache{};
  MotionCacheScope* m_active_scope{};
  void SetActiveScope(MotionCacheScope* scope) {
    assert(!m_active_scope);
    m_active_scope = scope;
  }
  void UnsetActiveScope(const MotionCacheScope* scope) {
    assert(m_active_scope == scope);
    m_active_scope = nullptr;
  }
};