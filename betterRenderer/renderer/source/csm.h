#pragma once

#include <nvrhi/nvrhi.h>

#include "nvrenderer/resource_registry.h"

struct MaShadowMap : public MaResourceRegistry {
  MaShadowMap(class NvRenderer* renderer);
  void Init();
  std::vector<double> m_shadow_planes;
  std::vector<glm::dmat4> m_cascade_matrices;
  std::vector<glm::dmat4> m_cascade_matrices_clamped;
  double m_far_plane_first;
  double m_far_plane_last;
  double m_shadow_far_plane;
  nvrhi::BufferHandle m_projection_buffer;
  nvrhi::BufferHandle m_inverse_projection_buffer;
  class NvRendererBackend* m_backend;
  void CalcCascades(const glm::dmat4& projection, const glm::dmat4& view,
                    const glm::dvec3& light);
  void UpdateConstants(nvrhi::ICommandList* command_list);
  static void GetViewPlaneCornersWorldSpace(
      const glm::dmat4& projection, const glm::dmat4& inverse_projection,
      const glm::dmat4& inverse_view, const double& d, bool normalize,
      glm::dvec3& ll, glm::dvec3& lr, glm::dvec3& ul, glm::dvec3& ur);
  struct ShadowProjectionConstants {
    std::array<glm::mat4, 8> m_shadow_projection;
    std::array<float, 8> m_shadow_planes;
  };
};