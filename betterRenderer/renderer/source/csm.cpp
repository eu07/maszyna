#include "csm.h"

#include <nvrhi/utils.h>

#include "config.h"
#include "nvrenderer/nvrenderer.h"
#include "nvrendererbackend.h"

MaShadowMap::MaShadowMap(NvRenderer* renderer)
    : MaResourceRegistry(renderer), m_backend(renderer->m_backend.get()) {}

void MaShadowMap::Init() {
  InitResourceRegistry();
  const auto& shadow_config = NvRenderer::Config()->m_shadow;
  int num_cascades = shadow_config.m_cascade_distances.size();
  m_cascade_matrices.resize(num_cascades);
  m_cascade_matrices_clamped.resize(num_cascades);
  m_shadow_planes.resize(num_cascades + 1);
  m_far_plane_first = shadow_config.m_cascade_distances.front();
  m_far_plane_last = shadow_config.m_cascade_distances.back();
  m_projection_buffer = m_backend->GetDevice()->createBuffer(
      nvrhi::utils::CreateStaticConstantBufferDesc(
          sizeof(ShadowProjectionConstants), "Shadow projections")
          .setInitialState(nvrhi::ResourceStates::ConstantBuffer)
          .setKeepInitialState(true));
  m_inverse_projection_buffer = m_backend->GetDevice()->createBuffer(
      nvrhi::utils::CreateStaticConstantBufferDesc(
          sizeof(ShadowProjectionConstants), "Inverse shadow projections")
          .setInitialState(nvrhi::ResourceStates::ConstantBuffer)
          .setKeepInitialState(true));
  RegisterResource(true, "shadow_projection_buffer", m_projection_buffer,
                   nvrhi::ResourceType::ConstantBuffer);
}

void MaShadowMap::CalcCascades(const glm::dmat4& projection,
                               const glm::dmat4& view,
                               const glm::dvec3& light) {
  const auto& shadow_config = NvRenderer::Config()->m_shadow;

  glm::dmat4 inverse_projection = glm::inverse(projection);
  glm::dmat4 inverse_view = glm::inverse(view);

  double projection_near, projection_far;
  {
    glm::dvec4 ndc_near{0., 0., 0., 1.};
    glm::dvec4 ndc_far{0., 0., 1., 1.};
    ndc_near = inverse_projection * ndc_near;
    ndc_far = inverse_projection * ndc_far;

    projection_near = -ndc_near.z / ndc_near.w;
    projection_far = -ndc_far.z / ndc_far.w;
  }

  // Set near and far plane offset
  m_shadow_planes.front() = projection_near;
  for (int i = 1; i < shadow_config.m_cascade_distances.size(); ++i)
    m_shadow_planes[i] = shadow_config.m_cascade_distances[i];
  // m_shadow_planes.back() = std::min(m_far_plane_last, projection_far);
  // m_shadow_planes[1] = std::min(m_far_plane_first, m_shadow_planes.back());
  //
  //// Fill intermediate plane offsets
  // double base = m_shadow_planes.back() / m_shadow_planes[1];
  // double denom = 1. / (m_shadow_planes.size() - 2.);
  // for (int i = 2; i < m_shadow_planes.size() - 1; ++i) {
  //   m_shadow_planes[i] = m_shadow_planes[1] * glm::pow(base, (i - 1) *
  //   denom);
  // }

  static std::vector<glm::dvec3> plane_corners;
  plane_corners.resize(4 * (m_shadow_planes.size()));

  // for (int i = 0; i < m_shadow_planes.size(); ++i) {
  //   glm::dvec3* plane = &plane_corners[i * 4];
  //   GetViewPlaneCornersWorldSpace(projection, inverse_projection,
  //   inverse_view,
  //                                 m_shadow_planes[i], plane[0], plane[1],
  //                                 plane[2], plane[3]);
  // }

  glm::dvec3 up = glm::abs(light.y) > .999 ? glm::dvec3{0., 0., 1.}
                                           : glm::dvec3{0., 1., 0.};

  for (int i = 0; i < m_cascade_matrices.size(); ++i) {
    glm::dvec3 plane[8];
    GetViewPlaneCornersWorldSpace(projection, inverse_projection, inverse_view,
                                  m_shadow_planes[i], true, plane[0], plane[1],
                                  plane[2], plane[3]);
    GetViewPlaneCornersWorldSpace(projection, inverse_projection, inverse_view,
                                  m_shadow_planes[i + 1], false, plane[4],
                                  plane[5], plane[6], plane[7]);

    glm::dvec3 world_min{std::numeric_limits<double>::max()};
    glm::dvec3 world_max{-std::numeric_limits<double>::max()};
    double offset = 0.;
    for (int j = 0; j < 8; ++j) {
      world_min = glm::min(world_min, plane[j]);
      world_max = glm::max(world_max, plane[j]);
    }

    double actual_size = glm::max(glm::distance(plane[0], plane[7]),
                                  glm::distance(plane[4], plane[7]));

    glm::dvec3 center = -Global.pCamera.Pos;

    glm::dmat4 shadow_view = glm::lookAtRH(center, center - light, up);

    glm::dvec3 min{std::numeric_limits<double>::max()};
    glm::dvec3 max{-std::numeric_limits<double>::max()};

    double frustumchunkdepth =
        m_shadow_planes[i + 1] -
        (m_shadow_planes[i] > 1.f ? m_shadow_planes[i] : 0.f);
    double quantizationstep{glm::min(frustumchunkdepth, 50.)};

    for (int j = 0; j < 8; ++j) {
      glm::dvec3 v = shadow_view * glm::dvec4(plane[j], 1.f);
      min = glm::min(min, v);
      max = glm::max(max, v);
    }

    glm::dvec2 size = max - min;
    glm::dvec2 diff = static_cast<glm::dvec2>(actual_size) - size;
    max.xy += glm::max(diff * .5, 0.);
    min.xy -= glm::max(diff * .5, 0.);

    glm::dvec2 pixel_size =
        static_cast<glm::dvec2>(actual_size / shadow_config.m_resolution);
    min.xy = glm::round(min.xy / pixel_size) * pixel_size;
    max.xy = glm::round(max.xy / pixel_size) * pixel_size;

    // max = quantizationstep * glm::ceil(max / quantizationstep);
    // min = quantizationstep * glm::floor(min / quantizationstep);

    glm::dmat4 shadow_proj = glm::orthoRH_ZO(min.x, max.x, min.y, max.y,
                                             -max.z - 500., -min.z + 500.);
    glm::dmat4 shadow_proj_clamped = glm::orthoRH_ZO(
        min.x, max.x, min.y, max.y, -max.z - 500., -min.z + 500.);

    m_cascade_matrices[i] = shadow_proj * shadow_view;
    m_cascade_matrices_clamped[i] = shadow_proj_clamped * shadow_view;
  }
}

void MaShadowMap::UpdateConstants(nvrhi::ICommandList* command_list) {
  {
    ShadowProjectionConstants constants{};
    for (int i = 0; i < std::min(m_cascade_matrices.size(),
                                 constants.m_shadow_projection.size());
         ++i) {
      constants.m_shadow_projection[i] = m_cascade_matrices[i];
      constants.m_shadow_planes[i] =
          m_shadow_planes[i + 1] * m_shadow_planes[i + 1];
    }
    command_list->writeBuffer(m_projection_buffer, &constants,
                              sizeof(constants));
  }
}

void MaShadowMap::GetViewPlaneCornersWorldSpace(
    const glm::dmat4& projection, const glm::dmat4& inverse_projection,
    const glm::dmat4& inverse_view, const double& d, bool normalize,
    glm::dvec3& ll, glm::dvec3& lr, glm::dvec3& ul, glm::dvec3& ur) {
  glm::dvec4 lll{-1., -1., 1., 1.};
  glm::dvec4 llr{1., -1., 1., 1.};
  glm::dvec4 lul{-1., 1., 1., 1.};
  glm::dvec4 lur{1., 1., 1., 1.};

  auto unproject = [&](glm::dvec4& v) -> void {
    v = inverse_projection * v;
    if (normalize) {
      v /= v.w;
      v *= d / glm::length(glm::dvec3(v.xyz));
    } else {
      v *= d / -v.z;
    }
    v.w = 1.;
  };

  unproject(lll);
  unproject(llr);
  unproject(lul);
  unproject(lur);

  ll = inverse_view * lll;
  lr = inverse_view * llr;
  ul = inverse_view * lul;
  ur = inverse_view * lur;
}
