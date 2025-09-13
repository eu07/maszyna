#include "motioncache.h"

MotionCacheScope::MotionCacheScope(void const* instance, MotionCache* cache)
    : m_instance(instance), m_cache(cache) {
  m_cache->SetActiveScope(this);
}

MotionCacheScope::~MotionCacheScope() {
  m_cache->UnsetActiveScope(this);
}
