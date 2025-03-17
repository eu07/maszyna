#include "nvrenderer/nvrenderer.h"

#include <AnimModel.h>
#include <Logs.h>
#include <Model3d.h>
#include <Timer.h>
#include <application.h>
#include <nvrhi/utils.h>
#include <simulation.h>

#include <future>
#include <glm/gtx/transform.hpp>

#include "auto_exposure.h"
#include "bloom.h"
#include "config.h"
#include "contactshadows.h"
#include "csm.h"
#include "environment.h"
#include "fsr.h"
#include "gbuffer.h"
#include "gbufferblitpass.h"
#include "gbufferlighting.h"
#include "instancecache.h"
#include "materialparser.h"
#include "motioncache.h"
#include "nvrenderer_imgui.h"
#include "nvrendererbackend.h"
#include "nvtexture.h"
#include "sky.h"
#include "ssao.h"
#include "tonemap.h"

#define TINYEXR_USE_MINIZ 0
#define TINYEXR_USE_STB_ZLIB 1
#define TINYEXR_IMPLEMENTATION
#include <fmt/chrono.h>
#include <fmt/format.h>

#include "tinyexr.h"

bool NvRenderer::Init(GLFWwindow *Window) {
  m_message_callback = std::make_shared<NvRendererMessageCallback>();
  m_config = std::make_shared<MaConfig>();
  m_config->Init("config.manul");

  switch (m_api) {
    case Api::D3D12:
#if LIBMANUL_WITH_D3D12
      if (!InitForD3D12(Window)) {
        return false;
      }
#else
      ErrorLog("D3D12 rendering backend not available in current Eu07 build!");
      return false;
#endif
      break;
    case Api::Vulkan:
#if LIBMANUL_WITH_VULKAN
      if (!InitForVulkan(Window)) {
        return false;
      }
#else
      ErrorLog("Vulkan rendering backend not available in current Eu07 build!");
      return false;
#endif
      break;
  }

  m_backend->Init();
  InitResourceRegistry();

  RegisterTexture("noise_2d_ldr",
                  GetTextureManager()->FetchTexture(
                      "manul/textures/noise/LDR_RGB1_0", GL_RGBA, 0, false));

  m_imgui_renderer = std::make_shared<NvImguiRenderer>(this);
  m_gbuffer = std::make_shared<NvGbuffer>(this);
  m_gbuffer_cube = std::make_shared<NvGbuffer>(this);
  m_gbuffer_shadow = std::make_shared<NvGbuffer>(this);
  m_contact_shadows = std::make_shared<MaContactShadows>(this, m_gbuffer.get());
  m_shadow_map = std::make_shared<MaShadowMap>(this);
  m_sky = std::make_shared<Sky>(this);
  m_environment = std::make_shared<MaEnvironment>(this);
  m_ssao = std::make_shared<NvSsao>(this);
  m_gbuffer_lighting = std::make_shared<GbufferLighting>(this);
  m_gbuffer_blit = std::make_shared<GbufferBlitPass>(
      this, m_gbuffer.get(), m_gbuffer_shadow.get(), m_ssao.get(),
      m_environment.get(), m_shadow_map.get(), m_contact_shadows.get(),
      m_sky.get());
  m_auto_exposure = std::make_shared<MaAutoExposure>(this);
  m_fsr = std::make_shared<NvFSR>(this);
  m_bloom = std::make_shared<Bloom>(GetBackend());

  m_gbuffer->Init(Global.gfx_framebuffer_width, Global.gfx_framebuffer_height,
                  false, false, false);
  m_contact_shadows->Init();
  m_shadow_map->Init();
  m_gbuffer_cube->Init(512, 512, true, false, false);
  const auto &shadow_config = GetConfig()->m_shadow;
  m_gbuffer_shadow->Init(shadow_config.m_resolution, shadow_config.m_resolution,
                         false, true, m_shadow_map->m_cascade_matrices.size());
  m_ssao->Init();
  m_sky->Init();
  m_environment->Init("farm_field_puresky_8k.hdr", -1.f);
  m_gbuffer_lighting->Init();
  m_gbuffer_blit->Init();
  m_auto_exposure->Init();
  m_fsr->Init();
  //   m_taa->Init();
  //    m_tonemap = std::make_shared<TonemapPass>(m_backend.get(),
  //    m_fsr->m_output);
  //  m_bloom->Init(m_fsr->m_output);
  m_bloom->Init(m_fsr->m_output);
  m_tonemap = std::make_shared<TonemapPass>(this, m_bloom->m_working_texture);
  m_tonemap->Init();

  m_framebuffer_forward = GetBackend()->GetDevice()->createFramebuffer(
      nvrhi::FramebufferDesc()
          .addColorAttachment(m_gbuffer_blit->m_output)
          .setDepthAttachment(m_gbuffer->m_gbuffer_depth));

  m_motion_cache = std::make_shared<MotionCache>();
  m_instance_cache = std::make_shared<InstanceBank>();

  m_command_list = m_backend->GetDevice()->createCommandList();

  RegisterResource(true, "shadow_depths", m_gbuffer_shadow->m_gbuffer_depth,
                   nvrhi::ResourceType::Texture_SRV);

  // RegisterResource(true, "gbuffer_depth", m_gbuffer->m_gbuffer_depth,
  //                  nvrhi::ResourceType::Texture_SRV);

  if (!InitMaterials()) return false;
  return true;
}

bool NvRenderer::AddViewport(
    const global_settings::extraviewport_config &conf) {
  return false;
}

void NvRenderer::Shutdown() {}

bool NvRenderer::Render() {
  std::scoped_lock lock(m_mtx_context_lock);
  Timer::subsystem.gfx_total
      .start();  // note: gfx_total is actually frame total, clean this up
  m_backend->UpdateWindowSize();
  m_backend->BeginFrame();
  nvrhi::CommandListHandle command_list_clear =
      m_backend->GetDevice()->createCommandList(/*
          nvrhi::CommandListParameters().setEnableImmediateExecution(false)*/);
  nvrhi::FramebufferHandle framebuffer = m_backend->GetCurrentFramebuffer();
  command_list_clear->open();

  // nvrhi::utils::ClearColorAttachment(
  //     command_list_clear, framebuffer, 0,
  //     nvrhi::Color(51.0f / 255.0f, 102.0f / 255.0f, 85.0f / 255.0f, 1.0f));
  command_list_clear->close();
  m_backend->GetDevice()->executeCommandList(command_list_clear);

  struct Frustum {
    std::array<glm::dvec4, 6> m_planes;
    Frustum(const glm::dmat4 &view_projection) {
      glm::mat4 m = glm::transpose(view_projection);
      m_planes[0] = {m[3] + m[0]};
      m_planes[1] = {m[3] - m[0]};
      m_planes[2] = {m[3] + m[1]};
      m_planes[3] = {m[3] - m[1]};
      m_planes[4] = {m[2]};
      m_planes[5] = {m[3] - m[2]};
      for (auto &plane : m_planes) {
        plane /= glm::length(static_cast<glm::dvec3>(plane.xyz));
      }
    }
    bool SphereInside(const glm::dvec3 &origin, double radius) const {
      for (const auto &plane : m_planes) {
        if (glm::dot(glm::dvec4(origin, 1.), plane) < -radius) {
          return false;
        }
      }
      return true;
    }
    bool BoxInside(const glm::dvec3 &origin, const glm::dvec3 &extent) const {
      glm::dvec4 p[8];
      for (int i = 0; i < 8; ++i) {
        p[i] =
            glm::dvec4{(i & 1) ? origin.x + extent.x : origin.x - extent.x,
                       (i & 2) ? origin.y + extent.y : origin.y - extent.y,
                       (i & 4) ? origin.z + extent.z : origin.z - extent.z, 1.};
      }
      for (const auto &plane : m_planes) {
        bool inside = false;

        for (int j = 0; j < 8; ++j) {
          if (glm::dot(p[j], plane) > 0.) {
            inside = true;
            break;
          }
        }

        if (!inside) return false;
      }

      return true;
    }
    bool BoxInside(const glm::dmat4x3 &transform, const glm::dvec3 &origin,
                   const glm::dvec3 &extent) const {
      glm::dvec4 p[8];
      for (int i = 0; i < 8; ++i) {
        p[i] =
            glm::dvec4{(i & 1) ? origin.x + extent.x : origin.x - extent.x,
                       (i & 2) ? origin.y + extent.y : origin.y - extent.y,
                       (i & 4) ? origin.z + extent.z : origin.z - extent.z, 1.};
        p[i] = glm::dvec4(transform * p[i], 1.);
      }
      for (const auto &plane : m_planes) {
        bool inside = false;

        for (int j = 0; j < 8; ++j) {
          if (glm::dot(p[j], plane) > 0.) {
            inside = true;
            break;
          }
        }

        if (!inside) return false;
      }

      return true;
    }
  };

  struct SingleFrustumTester : public IFrustumTester {
    Frustum m_frustum;
    SingleFrustumTester(const glm::dmat4 &projection, const glm::dmat4 &view)
        : m_frustum(projection * view) {}
    virtual bool IntersectSphere(const glm::dvec3 &origin,
                                 double radius) const override {
      return m_frustum.SphereInside(origin, radius);
    }
    virtual bool IntersectBox(const glm::dvec3 &origin,
                              const glm::dvec3 &extent) const override {
      return m_frustum.BoxInside(origin, extent);
    }
    virtual bool IntersectBox(const glm::dmat4x3 &transform,
                              const glm::dvec3 &origin,
                              const glm::dvec3 &extent) const override {
      return m_frustum.BoxInside(transform, origin, extent);
    }
  };
  struct CubeFrustumTester : public IFrustumTester {
    Frustum m_frustum;
    CubeFrustumTester()
        : m_frustum(glm::orthoRH_ZO(-Global.reflectiontune.range_instances,
                                    Global.reflectiontune.range_instances,
                                    -Global.reflectiontune.range_instances,
                                    Global.reflectiontune.range_instances,
                                    -Global.reflectiontune.range_instances,
                                    Global.reflectiontune.range_instances)) {}
    virtual bool IntersectSphere(const glm::dvec3 &origin,
                                 double radius) const override {
      return m_frustum.SphereInside(origin, radius);
    }
    virtual bool IntersectBox(const glm::dvec3 &origin,
                              const glm::dvec3 &extent) const override {
      return m_frustum.BoxInside(origin, extent);
    }
    virtual bool IntersectBox(const glm::dmat4x3 &transform,
                              const glm::dvec3 &origin,
                              const glm::dvec3 &extent) const override {
      return m_frustum.BoxInside(transform, origin, extent);
    }
  };
  if (simulation::is_ready && !Global.gfx_skiprendering) {
    if (m_scene_initialized) {
      m_fsr->BeginFrame();

      glm::dmat4 transform{1.};

      RenderPass pass{};

      pass.m_draw_shapes = m_draw_shapes;
      pass.m_draw_lines = m_draw_lines;
      pass.m_draw_tanimobj = m_draw_tanimobj;
      pass.m_draw_dynamic = m_draw_dynamic;
      pass.m_draw_track = m_draw_track;
      pass.m_draw_instances = m_draw_instances;
      pass.m_sort_batches = m_sort_batches;
      pass.m_sort_transparents = m_sort_transparents;

      // modelview
      // if (!DebugCameraFlag) {
      pass.m_origin = Global.pCamera.Pos;
      Global.pCamera.SetMatrix(transform);

      pass.m_draw_range = Global.BaseDrawRange * Global.fDistanceFactor;

      // if (!Global.headtrack_conf.magic_window || false) {
      //   pass.m_camera.position() +=
      //       Global.viewport_move * glm::dmat3(transform);
      //   transform =
      //       glm::dmat4(glm::inverse(Global.viewport_rotate)) * transform;
      // }
      pass.m_frame_index = GetCurrentFrame();

      transform = transform *
                  glm::translate(static_cast<glm::dvec3>(Global.pCamera.Pos));
      transform = static_cast<glm::dmat4>(static_cast<glm::dmat3>(transform));
      //} else {
      //  pass.m_camera.position() = Global.pDebugCamera.Pos;
      //  Global.pDebugCamera.SetMatrix(transform);
      //}

      // glm::dvec3 view_pos = static_cast<glm::dvec3>(Global.pCamera.Pos);
      // glm::dvec3 view_center =
      // static_cast<glm::dvec3>(Global.pCamera.LookAt); glm::dvec3 view_up =
      // static_cast<glm::dvec3>(Global.pCamera.vUp);
      //
      // glm::dvec3 view_dir = glm::rotateY(
      //     glm::rotateX(glm::rotateZ(-view_center, Global.pCamera.Angle.z),
      //                  Global.pCamera.Angle.x),
      //     Global.pCamera.Angle.y);
      //
      // transform = glm::lookAtRH(view_pos, view_pos + view_dir, view_up);

      m_drawcall_counter = nullptr;
      m_drawcalls_dynamic.Reset();
      m_drawcalls_shape.Reset();
      m_drawcalls_line.Reset();
      m_drawcalls_tanimobj.Reset();
      m_drawcalls_track.Reset();
      m_drawcalls_instances.Reset();

      pass.m_type = RenderPassType::Deferred;
      pass.m_framebuffer = m_gbuffer->m_framebuffer;

      auto info = pass.m_framebuffer->getFramebufferInfo();
      pass.m_viewport_state =
          nvrhi::ViewportState().addViewportAndScissorRect(info.getViewport());

      double fov = glm::radians(static_cast<double>(Global.FieldOfView) /
                                Global.ZoomFactor);
      glm::dmat4 projection = glm::perspectiveFovRH_ZO(
          fov, static_cast<double>(Global.window_size.x),
          static_cast<double>(Global.window_size.y), pass.m_draw_range, .1);

      SingleFrustumTester frustum_tester{projection, transform};
      DrawConstants draw_constants{};

      glm::dmat4 jitter = m_fsr->GetJitterMatrix();

      draw_constants.m_jittered_projection =
          static_cast<glm::mat4>(jitter * projection);
      draw_constants.m_projection = static_cast<glm::mat4>(projection);
      draw_constants.m_projection_history =
          std::exchange(m_previous_projection, draw_constants.m_projection);

      uint64_t sky_instance_id;
      {
        nvrhi::CommandListHandle command_list_sky =
            m_backend->GetDevice()->createCommandList();
        command_list_sky->open();
        command_list_sky->beginMarker("Sky LUT render");
        m_sky->Render(command_list_sky, projection, transform);
        command_list_sky->endMarker();
        command_list_sky->close();
        sky_instance_id =
            m_backend->GetDevice()->executeCommandList(command_list_sky);
      }

      if (!m_pause_animations) {
        Animate(pass.m_origin, pass.m_draw_range, pass.m_frame_index);
      }

      nvrhi::CommandListHandle command_list =
          m_backend->GetDevice()->createCommandList();
      command_list->open();
      command_list->beginMarker("Application render");
      command_list->beginMarker("Render scene");

      pass.m_projection = projection;
      pass.m_command_list_preparation = command_list;
      pass.m_command_list_draw = command_list;
      m_gbuffer->Clear(pass.m_command_list_draw);

      pass.m_command_list_preparation->writeBuffer(
          m_drawconstant_buffer, &draw_constants, sizeof(draw_constants));
      pass.m_frustum_tester = &frustum_tester;
      pass.m_transform = transform;
      pass.m_history_transform =
          std::exchange(m_previous_view, pass.m_transform);

      Timer::subsystem.gfx_color.start();
      RenderKabina(pass);
      RenderShapes(pass);
      RenderBatches(pass);
      RenderTracks(pass);
      RenderAnimateds(pass);
      RenderLines(pass);

      GatherSpotLights(pass);

      Timer::subsystem.gfx_color.stop();

      glm::dmat4 view_proj = projection * transform;
      glm::dmat4 previous_view_proj =
          std::exchange(m_previous_view_proj, view_proj);

      glm::dmat4 reproject = glm::inverse(view_proj) * previous_view_proj;

      command_list->beginMarker("Render skybox");
      m_environment->Render(command_list, jitter, projection, transform,
                            previous_view_proj);
      command_list->endMarker();

      command_list->endMarker();

      {  // Shadow render pass
        Timer::subsystem.gfx_shadows.start();
        command_list->beginMarker("Shadow render");

        glm::vec3 light_color, light_direction;
        m_sky->CalcLighting(light_direction, light_color);
        m_shadow_map->CalcCascades(
            glm::perspectiveFovRH_ZO(fov,
                                     static_cast<double>(Global.window_size.x),
                                     static_cast<double>(Global.window_size.y),
                                     .1, pass.m_draw_range),
            transform, -light_direction);
        m_shadow_map->UpdateConstants(command_list);
        m_gbuffer_shadow->Clear(command_list);

        if (dot(light_color, light_color) > 0.) {
          for (int cascade = 0;
               cascade < m_shadow_map->m_cascade_matrices.size(); ++cascade) {
            RenderPass pass{};

            pass.m_draw_shapes =
                m_draw_shapes;  // && Global.shadowtune.fidelity >= 0;
            pass.m_draw_lines =
                m_draw_lines;  // && Global.shadowtune.fidelity >= 0;
            pass.m_draw_tanimobj =
                m_draw_tanimobj;  // && Global.reflectiontune.fidelity >= 1;
            pass.m_draw_dynamic =
                m_draw_dynamic;  // && Global.reflectiontune.fidelity >= 2;
            pass.m_draw_track =
                m_draw_track;  // && Global.reflectiontune.fidelity >= 0;
            pass.m_draw_instances = m_draw_instances;
            pass.m_sort_batches = m_sort_batches;
            pass.m_sort_transparents = m_sort_transparents;

            SingleFrustumTester frustum_tester{
                m_shadow_map->m_cascade_matrices_clamped[cascade],
                glm::dmat4{1.}};

            pass.m_origin = Global.pCamera.Pos;
            pass.m_transform = glm::dmat3{1.};
            // pass.m_camera.position() =
            // simulation::Train->Dynamic()->vPosition;
            pass.m_draw_range = 150.;
            pass.m_type = RenderPassType::ShadowMap;
            pass.m_framebuffer =
                m_gbuffer_shadow->m_slice_framebuffers[cascade];
            pass.m_command_list_draw = command_list;
            pass.m_command_list_preparation = command_list;
            pass.m_frustum_tester = &frustum_tester;
            pass.m_projection = m_shadow_map->m_cascade_matrices[cascade];

            auto info = pass.m_framebuffer->getFramebufferInfo();
            pass.m_viewport_state =
                nvrhi::ViewportState().addViewportAndScissorRect(
                    info.getViewport());

            // CullBatches(pass);
            //  for (int i = 0.; i < 6.; ++i) {
            //    pass.m_viewport_state.addViewportAndScissorRect(
            //        nvrhi::Viewport(i * 2048, (i + 1) * 2048, 0, 2048, 0, 1));
            //  }

            RenderKabina(pass);
            RenderShapes(pass);
            RenderBatches(pass);
            RenderTracks(pass);
            RenderAnimateds(pass);
          }
        }

        Timer::subsystem.gfx_shadows.stop();
        command_list->endMarker();
      }

      if (m_environment->IsReady()) {  // CubeMap render pass
        command_list->beginMarker("Envmap render");

        Timer::subsystem.gfx_reflections.start();
        // CubeDrawConstants draw_constants{};
        // for (int i = 0; i < std::size(draw_constants.m_face_projection); ++i)
        // {
        //   draw_constants.m_face_projection[i] =
        //       glm::perspectiveFovRH_ZO(
        //           glm::radians(90.f), 1.f, 1.f, .1f,
        //           static_cast<float>(Global.BaseDrawRange *
        //                              Global.fDistanceFactor)) *
        //       glm::lookAtRH(glm::vec3{0.f, 0.f, 0.f}, targets[i], ups[i]);
        // }
        //
        // command_list->writeBuffer(m_cubedrawconstant_buffer, &draw_constants,
        //                           sizeof(draw_constants));

        static glm::dvec3 targets[]{{-1., 0., 0.}, {1., 0., 0.}, {0., 1., 0.},
                                    {0., -1., 0.}, {0., 0., 1.}, {0., 0., -1.}};
        static glm::dvec3 ups[]{{0., 1., 0.}, {0., 1., 0.}, {0., 0., -1.},
                                {0., 0., 1.}, {0., 1., 0.}, {0., 1., 0.}};

        glm::dvec3 current_camera_pos = Global.pCamera.Pos;
        uint64_t current_frame = GetCurrentFrame();
        uint64_t previous_frame =
            std::exchange(m_previous_env_frame, current_frame);

        m_gbuffer_cube->Clear(command_list);
        for (int face = 0; face < 6; ++face) {
          m_instance_cache->Clear();
          glm::dmat4 transform =
              glm::lookAtRH(glm::dvec3{0., 0., 0.}, targets[face], ups[face]);

          glm::dmat4 projection = glm::perspectiveFovRH_ZO(
              M_PI_2, 1., 1.,
              static_cast<double>(Global.reflectiontune.range_instances), .1);

          SingleFrustumTester frustum_tester{projection, transform};
          RenderPass pass{};

          pass.m_transform = transform;
          pass.m_draw_shapes =
              m_draw_shapes && Global.reflectiontune.fidelity >= 0;
          pass.m_draw_lines =
              m_draw_lines && Global.reflectiontune.fidelity >= 2;
          pass.m_draw_tanimobj =
              m_draw_tanimobj && Global.reflectiontune.fidelity >= 1;
          pass.m_draw_dynamic =
              m_draw_dynamic && Global.reflectiontune.fidelity >= 2;
          pass.m_draw_track =
              m_draw_track && Global.reflectiontune.fidelity >= 0;
          pass.m_draw_instances = m_draw_instances;
          pass.m_sort_batches = m_sort_batches;
          pass.m_sort_transparents = m_sort_transparents;

          pass.m_origin = current_camera_pos;
          // pass.m_camera.position() = simulation::Train->Dynamic()->vPosition;
          pass.m_draw_range = Global.BaseDrawRange * Global.fDistanceFactor;
          pass.m_type = RenderPassType::CubeMap;
          pass.m_framebuffer = m_gbuffer_cube->m_slice_framebuffers[face];
          pass.m_command_list_draw = command_list;
          pass.m_command_list_preparation = command_list;
          pass.m_frustum_tester = &frustum_tester;
          pass.m_projection = projection;

          auto info = pass.m_framebuffer->getFramebufferInfo();
          pass.m_viewport_state =
              nvrhi::ViewportState().addViewportAndScissorRect(
                  info.getViewport());

          // for (int i = 0.; i < 6.; ++i) {
          //   pass.m_viewport_state.addViewportAndScissorRect(
          //       nvrhi::Viewport(i * 2048, (i + 1) * 2048, 0, 2048, 0, 1));
          // }

          // CullBatches(pass);
          RenderShapes(pass);
          RenderBatches(pass);
          RenderTracks(pass);
          RenderAnimateds(pass);
        }

        Timer::subsystem.gfx_reflections.stop();
        command_list->endMarker();
        m_envir_can_filter = true;
      }

      m_contact_shadows->UpdateConstants(command_list, projection, transform,
                                         Global.DayLight.direction,
                                         pass.m_frame_index);
      m_contact_shadows->Render(command_list);
      m_ssao->Render(command_list, draw_constants.m_projection,
                     pass.m_frame_index);
      //  m_sky_transmittance->Render(command_list);
      Timer::subsystem.gfx_swap.start();

      command_list->beginMarker("Deferred lights");
      m_gbuffer_lighting->Render(pass);
      command_list->endMarker();

      command_list->beginMarker("GBuffer blit");
      m_gbuffer_blit->Render(command_list, transform, jitter * projection);
      command_list->endMarker();

      if (true) {
        command_list->beginMarker("Forward pass");
        pass.m_framebuffer = m_framebuffer_forward;
        pass.m_type = RenderPassType::Forward;
        RenderShapes(pass);
        RenderAnimateds(pass);
        RenderKabina(pass);
        command_list->endMarker();
      }

      m_auto_exposure->Render(command_list);
      m_fsr->Render(command_list, pass.m_draw_range, .1, fov);
      m_bloom->Render(command_list);
      m_tonemap->Render(command_list);

      for (auto &bank : m_geometry_banks) {
        for (auto &chunk : bank.m_chunks) {
          if (glm::distance2(chunk.m_last_position_requested, pass.m_origin) >
              Global.BaseDrawRange * Global.BaseDrawRange) {
            chunk.m_is_uptodate = false;
            chunk.m_index_buffer = nullptr;
            chunk.m_vertex_buffer = nullptr;
          }
        }
      }

      for (auto &material : m_material_cache) {
        if (glm::distance2(material.m_last_position_requested, pass.m_origin) >
            10 * 10) {
          material.m_binding_set = nullptr;
          material.m_binding_set_cubemap = nullptr;
          material.m_binding_set_shadow = nullptr;
        }
      }

      command_list->endMarker();
      command_list->close();
      m_backend->GetDevice()->executeCommandList(command_list);

      GetTextureManager()->Cleanup(pass.m_origin);
      for (auto &material : m_material_cache) {
        if (GetCurrentFrame() - material.m_last_frame_requested > 100) {
          material.m_binding_sets.fill(nullptr);
          material.m_last_texture_updates.fill(0);
        }
      }

      Timer::subsystem.gfx_swap.stop();

      if (m_envir_can_filter)
        m_environment->Filter(sky_instance_id, m_gbuffer_cube.get());
    } else {
      // Prepare all section geometries to be uploaded to gpu as soon as we need
      // them
      for (auto section : simulation::Region->m_sections) {
        if (!section) continue;
        section->create_geometry();
      }
      GatherModelsForBatching();
      GatherTracksForBatching();
      GatherShapesForBatching();
      GatherAnimatedsForBatching();
      GatherCellsForAnimation();
      GatherDynamics();
      // RenderPass pass{};
      // pass.m_command_list_preparation = command_list;
      // for (uint32_t bank = 0; bank < m_geometry_banks.size(); ++bank) {
      //   for (uint32_t chunk = 0; chunk <
      //   m_geometry_banks[bank].m_chunks.size();
      //        ++chunk) {
      //     m_geometry_banks[bank].m_chunks[chunk].m_last_frame_requested =
      //         GetCurrentFrame();
      //     UpdateGeometry({bank + 1, chunk + 1}, pass);
      //   }
      // }
      m_scene_initialized = true;
    }
  }
  m_previous_env_position = Global.pCamera.Pos;

  // Timer::subsystem.gfx_reflections.start();
  // Timer::subsystem.gfx_reflections.stop();
  Timer::subsystem.gfx_gui.start();
  DebugUi();
  m_sky->OnGui();
  m_bloom->OnGui();
  Application.render_ui();
  Timer::subsystem.gfx_gui.stop();
  Timer::subsystem.gfx_total.stop();
  return true;
}

void NvRenderer::SwapBuffers() {
  m_backend->Present();

  // if (!m_upload_event_query ||
  //     m_backend->GetDevice()->pollEventQuery(m_upload_event_query)) {
  //   {
  //     m_upload_event_query = m_backend->GetDevice()->createEventQuery();
  //     size_t frame_index = GetCurrentFrame();
  //
  //     for (auto &bank : m_geometry_banks) {
  //       if (bank.m_last_frame_requested == frame_index &&
  //       !bank.m_is_uptodate) {
  //         bank.m_uploaded_query = m_upload_event_query;
  //       }
  //     }
  //
  //     RenderPass pass{};
  //     pass.m_type = RenderPassType::Deferred;
  //     pass.m_frame_index = frame_index;
  //     pass.m_command_list_preparation =
  //         m_backend->GetDevice()->createCommandList();
  //     pass.m_command_list_preparation->open();
  //     for (gfx::geometry_handle handle{1, 0};
  //          handle.bank <= m_geometry_banks.size(); ++handle.bank) {
  //       auto &bank = m_geometry_banks[handle.bank - 1];
  //       if (bank.m_is_uptodate) continue;
  //       UpdateGeometryBank(handle, pass, true);
  //     }
  //     pass.m_command_list_preparation->close();
  //     m_backend->GetDevice()->executeCommandList(pass.m_command_list_preparation);
  //     m_backend->GetDevice()->setEventQuery(m_upload_event_query,
  //                                        nvrhi::CommandQueue::Graphics);
  //   }
  // }
  m_backend->GetDevice()->runGarbageCollection();
}

float NvRenderer::Framerate() { return 0.0f; }

gfx::geometrybank_handle NvRenderer::Create_Bank() {
  gfx::geometrybank_handle handle{
      static_cast<uint32_t>(m_geometry_banks.size() + 1), 0};
  auto &bank = m_geometry_banks.emplace_back();
  return handle;
}

gfx::geometry_handle NvRenderer::Insert(
    gfx::index_array &Indices, gfx::vertex_array &Vertices,
    gfx::geometrybank_handle const &Geometry, int const Type) {
  if (!Type || Type >= TP_ROTATOR) return {};

  auto &bank = m_geometry_banks[Geometry.bank - 1];
  gfx::geometrybank_handle handle{
      Geometry.bank, static_cast<uint32_t>(bank.m_chunks.size() + 1)};

  auto &chunk = bank.m_chunks.emplace_back();
  chunk.m_indices = std::move(Indices);
  chunk.m_vertices = std::move(Vertices);

  switch (Type) {
    case GL_TRIANGLES:
      break;
    case GL_LINES:
    case GL_LINE_LOOP:
    case GL_LINE_STRIP:
      break;
    default:
      break;
  }

  chunk.UpdateBounds();
  chunk.m_is_uptodate = false;
  bank.m_is_uptodate = false;
  return handle;
}

gfx::geometry_handle NvRenderer::Insert(
    gfx::vertex_array &Vertices, gfx::geometrybank_handle const &Geometry,
    int const Type) {
  if (!Type || Type >= TP_ROTATOR) return {};

  auto &bank = m_geometry_banks[Geometry.bank - 1];
  gfx::geometrybank_handle handle{
      Geometry.bank, static_cast<uint32_t>(bank.m_chunks.size() + 1)};

  auto &chunk = bank.m_chunks.emplace_back();
  chunk.m_indices = {};

  gfx::calculate_tangents(Vertices, {}, Type);

  switch (Type) {
    case GL_TRIANGLES:
      chunk.m_vertices = std::move(Vertices);
      break;
    case GL_TRIANGLE_STRIP:
      chunk.m_vertices = std::move(Vertices);
      for (int i = 2; i < std::size(chunk.m_vertices); ++i) {
        if (i % 2) {
          chunk.m_indices.emplace_back(i);
          chunk.m_indices.emplace_back(i - 1);
          chunk.m_indices.emplace_back(i - 2);
        } else {
          chunk.m_indices.emplace_back(i - 2);
          chunk.m_indices.emplace_back(i - 1);
          chunk.m_indices.emplace_back(i);
        }
      }
      break;
    case GL_TRIANGLE_FAN:
      chunk.m_vertices = std::move(Vertices);
      for (int i = 2; i < std::size(chunk.m_vertices); ++i) {
        chunk.m_indices.emplace_back(0);
        chunk.m_indices.emplace_back(i - 1);
        chunk.m_indices.emplace_back(i);
      }
      break;
    case GL_LINES:
      chunk.m_vertices = std::move(Vertices);
      break;
    case GL_LINE_LOOP:
      chunk.m_vertices = std::move(Vertices);
      chunk.m_indices.reserve(chunk.m_vertices.size() * 2);
      for (int i = 1; i <= chunk.m_vertices.size(); ++i) {
        chunk.m_indices.emplace_back(i - 1);
        chunk.m_indices.emplace_back(i % chunk.m_vertices.size());
      }
      break;
    case GL_LINE_STRIP:
      chunk.m_vertices = std::move(Vertices);
      chunk.m_indices.reserve((chunk.m_vertices.size() - 1) * 2);
      for (int i = 1; i < chunk.m_vertices.size(); ++i) {
        chunk.m_indices.emplace_back(i - 1);
        chunk.m_indices.emplace_back(i);
      }
      break;
    default:;
      break;
  }

  chunk.UpdateBounds();
  chunk.m_is_uptodate = false;
  bank.m_is_uptodate = false;
  return handle;
}

bool NvRenderer::Replace(gfx::vertex_array &Vertices,
                         gfx::geometry_handle const &Geometry, int const Type,
                         std::size_t const Offset) {
  if (!Type || Type >= TP_ROTATOR) return false;

  auto &bank = m_geometry_banks[Geometry.bank - 1];
  auto &chunk = bank.m_chunks[Geometry.chunk - 1];

  gfx::calculate_tangents(Vertices, {}, Type);
  if (chunk.m_replace_impl) return chunk.m_replace_impl(Vertices, Type);

  chunk.m_indices = {};

  switch (Type) {
    case GL_TRIANGLES:
      chunk.m_vertices = std::move(Vertices);
      break;
    case GL_TRIANGLE_STRIP:
      chunk.m_vertices = std::move(Vertices);
      for (int i = 2; i < std::size(chunk.m_vertices); ++i) {
        if (i % 2) {
          chunk.m_indices.emplace_back(i);
          chunk.m_indices.emplace_back(i - 1);
          chunk.m_indices.emplace_back(i - 2);
        } else {
          chunk.m_indices.emplace_back(i - 2);
          chunk.m_indices.emplace_back(i - 1);
          chunk.m_indices.emplace_back(i);
        }
      }
      break;
    case GL_LINES:
    case GL_LINE_LOOP:
    case GL_LINE_STRIP:
      break;
    default:
      break;
  }

  chunk.UpdateBounds();
  chunk.m_is_uptodate = false;
  bank.m_is_uptodate = false;
  return true;
}

bool NvRenderer::Append(gfx::vertex_array &Vertices,
                        gfx::geometry_handle const &Geometry, int const Type) {
  return false;
}

gfx::index_array const &NvRenderer::Indices(
    gfx::geometry_handle const &handle) const {
  // if (!handle.bank || !handle.chunk || handle.bank > m_geometry_banks.size()
  // ||
  //     handle.chunk > m_geometry_banks[handle.bank - 1].m_chunks.size())
  //   return {};
  return m_geometry_banks[handle.bank - 1].m_chunks[handle.chunk - 1].m_indices;
}

gfx::vertex_array const &NvRenderer::Vertices(
    gfx::geometry_handle const &handle) const {
  // if (!handle.bank || !handle.chunk || handle.bank > m_geometry_banks.size()
  // ||
  //     handle.chunk > m_geometry_banks[handle.bank - 1].m_chunks.size())
  //   return {};
  return m_geometry_banks[handle.bank - 1]
      .m_chunks[handle.chunk - 1]
      .m_vertices;
}

material_handle NvRenderer::Fetch_Material(std::string const &Filename,
                                           bool const Loadnow) {
  auto filename{Filename};

  if (contains(filename, '|')) {
    filename.erase(
        filename.find('|'));  // po | może być nazwa kolejnej tekstury
  }

  // discern references to textures generated by a script
  // TBD: support file: for file resources?

  // process supplied resource name
  if (auto const isgenerated{filename.find("make:") == 0 ||
                             filename.find("internal_src:") == 0}) {
    // generated resource
    // scheme:(user@)path?query

    // remove scheme indicator
    // filename.erase(0, filename.find(':') + 1);
    //// TBD, TODO: allow shader specification as part of the query?
    // erase_leading_slashes(filename);
  } else {
    // regular file resource
    // (filepath/)filename.extension

    erase_extension(filename);
    replace_slashes(filename);
    erase_leading_slashes(filename);
  }

  const auto &[it, is_new] = m_material_map.try_emplace(filename, -1);
  if (is_new) {
    MaterialAdapter adapter{};
    adapter.Parse(filename);
    auto shader = adapter.GetShader();
    MaterialTemplate *material_template = nullptr;
    if (const auto template_found =
            m_material_templates.find(adapter.GetShader());
        template_found != m_material_templates.end()) {
      material_template = template_found->second.get();
    }

    auto &cache = m_material_cache.emplace_back(material_template);
    cache.Init();
    it->second = static_cast<int>(m_material_cache.size());
    cache.m_name = filename;
    cache.m_opacity = adapter.GetOpacity();
    cache.m_shadow_rank = adapter.GetShadowRank();
    cache.m_size = adapter.GetSize();
    cache.m_masked_shadow = false;

    auto texture_manager = GetTextureManager();

    for (int i = 0; i < material_template->m_texture_bindings.size(); ++i) {
      const auto &[key, sampler_key, hint, m_default_texture] =
          material_template->m_texture_bindings[i];
      auto handle = texture_manager->FetchTexture(
          static_cast<std::string>(adapter.GetTexturePathForEntry(key)), hint,
          adapter.GetTextureSizeBiasForEntry(key), true);
      cache.m_texture_handles[i] = handle;
      cache.RegisterTexture(key.c_str(), handle);
      cache.RegisterResource(false, sampler_key.c_str(),
                             texture_manager->GetSamplerForTraits(
                                 texture_manager->GetTraits(handle),
                                 NvRenderer::RenderPassType::Deferred),
                             nvrhi::ResourceType::Sampler);
      if (key == cache.m_template->m_masked_shadow_texture &&
          texture_manager->IsValidHandle(handle)) {
        cache.m_masked_shadow =
            texture_manager->GetTexture(handle)->m_has_alpha;
      }
    }

    // cache.m_textures.resize(cache.m_template->m_texture_bindings.size());
    // for (int i = 0; i < cache.m_textures.size(); ++i) {
    //   const auto &binding = cache.m_template->m_texture_bindings[i];
    //   cache.m_textures[i] = ;
    //   if (i == cache.m_template->m_masked_shadow_texture &&
    //       m_texture_manager->IsValidHandle(cache.m_textures[i])) {
    //     cache.m_masked_shadow =
    //         m_texture_manager->GetTexture(cache.m_textures[i])->m_has_alpha;
    //   }
    // }
    // if (auto masked_shadow = adapter.GetMaskedShadowOverride())
    //   cache.m_masked_shadow = *masked_shadow;
    cache.m_pipelines.fill(nullptr);
    for (int j = 0; j < static_cast<int>(DrawType::Num); ++j) {
      for (int i = 0; i < static_cast<int>(RenderPassType::Num); ++i) {
        const auto pass = static_cast<RenderPassType>(i);
        const auto draw = static_cast<DrawType>(j);
        auto pipeline =
            cache.m_template->GetPipeline(pass, draw, cache.m_masked_shadow);
        cache.m_pipelines[Constants::GetPipelineIndex(pass, draw)] = pipeline;
      }
    }
  }
  return it->second;
}

void NvRenderer::Bind_Material(material_handle const Material,
                               TSubModel const *sm,
                               lighting_data const *lighting) {}

IMaterial const *NvRenderer::Material(material_handle const handle) const {
  if (!handle || handle > m_material_cache.size()) {
    return &m_material_cache[0];
  }
  return &m_material_cache[handle - 1];
}

std::shared_ptr<gl::program> NvRenderer::Fetch_Shader(std::string const &name) {
  return std::shared_ptr<gl::program>();
}

texture_handle NvRenderer::Fetch_Texture(std::string const &Filename,
                                         bool const Loadnow,
                                         GLint format_hint) {
  return GetTextureManager()->FetchTexture(Filename, format_hint, 0, false);
}

void NvRenderer::Bind_Texture(texture_handle const Texture) {}

void NvRenderer::Bind_Texture(std::size_t const Unit,
                              texture_handle const Texture) {}

ITexture &NvRenderer::Texture(texture_handle const Texture) {
  if (auto texture_manager = GetTextureManager();
      texture_manager->IsValidHandle(Texture))
    return *texture_manager->GetTexture(Texture);
  return *ITexture::null_texture();
}

ITexture const &NvRenderer::Texture(texture_handle const Texture) const {
  if (auto texture_manager = GetTextureManager();
      texture_manager->IsValidHandle(Texture))
    return *texture_manager->GetTexture(Texture);
  return *ITexture::null_texture();
}

void NvRenderer::Pick_Control_Callback(
    std::function<void(TSubModel const *, const glm::vec2)> Callback) {}

void NvRenderer::Pick_Node_Callback(
    std::function<void(scene::basic_node *)> Callback) {}

TSubModel const *NvRenderer::Pick_Control() const { return nullptr; }

scene::basic_node const *NvRenderer::Pick_Node() const { return nullptr; }

glm::dvec3 NvRenderer::Mouse_Position() const { return glm::dvec3(); }

void NvRenderer::Update(double const Deltatime) {}

void NvRenderer::Update_Pick_Control() {}

void NvRenderer::Update_Pick_Node() {}

glm::dvec3 NvRenderer::Update_Mouse_Position() { return glm::dvec3(); }

bool NvRenderer::Debug_Ui_State(std::optional<bool> param) {
  if (param) {
    m_debug_ui_active = *param;
  }
  return m_debug_ui_active;
}

std::string const &NvRenderer::info_times() const {
  // TODO: insert return statement here
  static std::string placeholder = "";
  return placeholder;
}

std::string const &NvRenderer::info_stats() const {
  // TODO: insert return statement here
  static std::string placeholder = "";
  return placeholder;
}

imgui_renderer *NvRenderer::GetImguiRenderer() {
  return m_imgui_renderer.get();
}

void NvRenderer::MakeScreenshot() {
  nvrhi::CommandListHandle command_list =
      GetBackend()->GetDevice()->createCommandList();
  command_list->open();

  nvrhi::ITexture *source = m_tonemap->m_source;

  nvrhi::TextureDesc staging_desc = source->getDesc();
  nvrhi::StagingTextureHandle staging_texture =
      GetBackend()->GetDevice()->createStagingTexture(
          staging_desc, nvrhi::CpuAccessMode::Read);

  command_list->copyTexture(staging_texture, nvrhi::TextureSlice(), source,
                            nvrhi::TextureSlice());

  command_list->close();
  GetBackend()->GetDevice()->executeCommandList(command_list);

  size_t row_pitch;
  auto map = GetBackend()->GetDevice()->mapStagingTexture(
      staging_texture, nvrhi::TextureSlice().resolve(staging_desc),
      nvrhi::CpuAccessMode::Read, &row_pitch);

  std::string output{};
  output.resize(row_pitch * staging_desc.height);
  memcpy(output.data(), map, output.size());

  GetBackend()->GetDevice()->unmapStagingTexture(staging_texture);

  {
    EXRHeader header;
    InitEXRHeader(&header);

    EXRImage image;
    InitEXRImage(&image);

    image.num_channels = 3;

    std::vector<uint16_t> images[3];
    images[0].resize(staging_desc.width * staging_desc.height);
    images[1].resize(staging_desc.width * staging_desc.height);
    images[2].resize(staging_desc.width * staging_desc.height);

    // Split RGBRGBRGB... into R, G and B layer
    for (int y = 0; y < staging_desc.height; ++y) {
      const uint16_t *src =
          reinterpret_cast<uint16_t *>(output.data() + row_pitch * y);
      size_t row_start = y * staging_desc.width;
      for (int x = 0; x < staging_desc.width; ++x) {
        images[0][row_start + x] = src[4 * x + 0];
        images[1][row_start + x] = src[4 * x + 1];
        images[2][row_start + x] = src[4 * x + 2];
      }
    }

    uint16_t *image_ptr[3];
    image_ptr[0] = images[2].data();  // B
    image_ptr[1] = images[1].data();  // G
    image_ptr[2] = images[0].data();  // R

    image.images = reinterpret_cast<unsigned char **>(image_ptr);
    image.width = static_cast<int>(staging_desc.width);
    image.height = static_cast<int>(staging_desc.height);

    header.num_channels = 3;
    header.channels = static_cast<EXRChannelInfo *>(
        malloc(sizeof(EXRChannelInfo) * header.num_channels));
    // Must be (A)BGR order, since most of EXR viewers expect this channel
    // order.
    strncpy(header.channels[0].name, "B", 255);
    header.channels[0].name[strlen("B")] = '\0';
    strncpy(header.channels[1].name, "G", 255);
    header.channels[1].name[strlen("G")] = '\0';
    strncpy(header.channels[2].name, "R", 255);
    header.channels[2].name[strlen("R")] = '\0';

    header.channels[0].p_linear = 1;
    header.channels[1].p_linear = 1;
    header.channels[2].p_linear = 1;

    header.pixel_types =
        static_cast<int *>(malloc(sizeof(int) * header.num_channels));
    header.requested_pixel_types =
        static_cast<int *>(malloc(sizeof(int) * header.num_channels));
    for (int i = 0; i < header.num_channels; i++) {
      header.pixel_types[i] =
          TINYEXR_PIXELTYPE_HALF;  // pixel type of input image
      header.requested_pixel_types[i] =
          TINYEXR_PIXELTYPE_HALF;  // pixel type of output image to be stored in
                                   // .EXR
    }

    const char *err = nullptr;  // or nullptr in C++11 or later.
    if (int ret = SaveEXRImageToFile(
            &image, &header,
            fmt::format("{}@{}_{:%Y%m%d_%H%M%S}.exr",
                        simulation::Train != nullptr
                            ? simulation::Train->Occupied()->Name
                            : "",
                        Global.SceneryFile, fmt::localtime(std::time(nullptr)))
                .c_str(),
            &err);
        ret != TINYEXR_SUCCESS) {
      fprintf(stderr, "Save EXR err: %s\n", err);
      FreeEXRErrorMessage(err);  // free's buffer for an error message
      //__debugbreak();
    }
    // printf("Saved exr file. [ %s ] \n", outfilename);
    free(header.channels);
    free(header.pixel_types);
    free(header.requested_pixel_types);
  }
}

MaConfig *NvRenderer::Config() {
  NvRenderer *me = dynamic_cast<NvRenderer *>(GfxRenderer.get());
  return me ? me->GetConfig() : nullptr;
}

NvRenderer::GeometryBounds NvRenderer::GetGeometryBounds(
    gfx::geometry_handle const &handle) const {
  if (!handle.bank || handle.bank > m_geometry_banks.size()) return {};
  auto &bank = m_geometry_banks[handle.bank - 1];
  if (!handle.chunk || handle.chunk > bank.m_chunks.size()) return {};
  auto &chunk = bank.m_chunks[handle.chunk - 1];
  return {chunk.m_origin, chunk.m_extent};
}

size_t NvRenderer::GetCurrentFrame() const { return m_fsr->m_current_frame; }

void NvRenderer::DebugUi() {
  if (!m_debug_ui_active) return;

  if (ImGui::Begin("Renderer Debug", &m_debug_ui_active,
                   ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::LabelText("Draws Dynamic", "%d (%d triangles)",
                     m_drawcalls_dynamic.m_drawcalls,
                     m_drawcalls_dynamic.m_triangles);
    ImGui::LabelText("Draws TAnimObj", "%d (%d triangles)",
                     m_drawcalls_tanimobj.m_drawcalls,
                     m_drawcalls_tanimobj.m_triangles);
    ImGui::LabelText("Draws Shape", "%d (%d triangles)",
                     m_drawcalls_shape.m_drawcalls,
                     m_drawcalls_shape.m_triangles);
    ImGui::LabelText("Draws Line", "%d (%d triangles)",
                     m_drawcalls_line.m_drawcalls,
                     m_drawcalls_line.m_triangles);
    ImGui::LabelText("Draws Track", "%d (%d triangles)",
                     m_drawcalls_track.m_drawcalls,
                     m_drawcalls_track.m_triangles);
    ImGui::LabelText("Draws Instances", "%d (%d triangles)",
                     m_drawcalls_instances.m_drawcalls,
                     m_drawcalls_instances.m_triangles);
    ImGui::Checkbox("Draw Dynamics", &m_draw_dynamic);
    ImGui::Checkbox("Draw TAnimObjs", &m_draw_tanimobj);
    ImGui::Checkbox("Draw Shapes", &m_draw_shapes);
    ImGui::Checkbox("Draw Lines", &m_draw_lines);
    ImGui::Checkbox("Draw Tracks", &m_draw_track);
    ImGui::Checkbox("Draw Instances", &m_draw_instances);
    ImGui::Checkbox("Sort Batches", &m_sort_batches);
    ImGui::Checkbox("Sort Transparents", &m_sort_transparents);
    ImGui::Checkbox("Pause Animation updates", &m_pause_animations);
    if (ImGui::Button("Sky Config")) m_sky->ShowGui();
    ImGui::SameLine();
    if (ImGui::Button("Bloom Config")) m_bloom->ShowGui();
    ImGui::SameLine();
    if (ImGui::Button("Take Screenshot")) MakeScreenshot();
  }
  ImGui::End();
}

void NvRenderer::UpdateGeometry(gfx::geometry_handle handle,
                                const RenderPass &pass, bool *p_is_uploading) {
  if (!handle.bank || handle.bank > m_geometry_banks.size()) return;
  auto &bank = m_geometry_banks[handle.bank - 1];
  if (!handle.chunk || handle.chunk > bank.m_chunks.size()) return;
  auto &chunk = bank.m_chunks[handle.chunk - 1];

  bool is_uploading =
      chunk.m_chunk_ready_handle &&
      !m_backend->GetDevice()->pollEventQuery(chunk.m_chunk_ready_handle);

  if (p_is_uploading) *p_is_uploading = is_uploading;

  if (is_uploading) return;

  chunk.m_chunk_ready_handle = nullptr;
  if (chunk.m_is_uptodate) return;  // Nothing to do
  if (chunk.m_last_frame_requested != pass.m_frame_index) {
    return;
  }

  // chunk.m_index_offset = index_count;
  // chunk.m_vertex_offset = vertex_count;
  chunk.m_indexed = !!chunk.m_indices.size();
  // vertex_count += chunk.m_vertices.size();
  // index_count += chunk.m_indices.size();

  if (chunk.m_vertices.size() &&
      (!chunk.m_vertex_buffer ||
       chunk.m_vertices.size() > chunk.m_vertex_count)) {
    chunk.m_vertex_buffer = m_backend->GetDevice()->createBuffer(
        nvrhi::BufferDesc()
            .setIsVertexBuffer(true)
            .setByteSize(chunk.m_vertices.size() * sizeof(gfx::basic_vertex))
            .setInitialState(nvrhi::ResourceStates::VertexBuffer)
            .setKeepInitialState(true));
  }

  if (chunk.m_indices.size() &&
      (!chunk.m_index_buffer || chunk.m_indices.size() > chunk.m_index_count)) {
    chunk.m_index_buffer = m_backend->GetDevice()->createBuffer(
        nvrhi::BufferDesc()
            .setIsIndexBuffer(true)
            .setByteSize(chunk.m_indices.size() * sizeof(gfx::basic_index))
            .setInitialState(nvrhi::ResourceStates::IndexBuffer)
            .setKeepInitialState(true));
  }

  chunk.m_vertex_count = chunk.m_vertices.size();
  chunk.m_index_count = chunk.m_indices.size();

  if (chunk.m_vertex_buffer)
    pass.m_command_list_preparation->writeBuffer(
        chunk.m_vertex_buffer, chunk.m_vertices.data(),
        chunk.m_vertices.size() * sizeof(gfx::basic_vertex));

  if (chunk.m_index_buffer)
    pass.m_command_list_preparation->writeBuffer(
        chunk.m_index_buffer, chunk.m_indices.data(),
        chunk.m_indices.size() * sizeof(gfx::basic_index));

  // nvrhi::CommandListHandle command_list = pass.m_command_list_preparation;
  //
  // if (big_and_chunky) {
  //   command_list = m_backend->GetDevice()->createCommandList();
  //   command_list->open();
  //   chunk.m_chunk_ready_handle = m_backend->GetDevice()->createEventQuery();
  // }
  //
  // if (chunk.m_vertex_buffer)
  //   command_list->writeBuffer(
  //       chunk.m_vertex_buffer, chunk.m_vertices.data(),
  //       chunk.m_vertices.size() * sizeof(gfx::basic_vertex));
  //
  // if (chunk.m_index_buffer)
  //   command_list->writeBuffer(
  //       chunk.m_index_buffer, chunk.m_indices.data(),
  //       chunk.m_indices.size() * sizeof(gfx::basic_index));
  //
  // if (big_and_chunky) {
  //   command_list->close();
  //   m_backend->GetDevice()->executeCommandList(command_list);
  //   m_backend->GetDevice()->setEventQuery(chunk.m_chunk_ready_handle,
  //                                      nvrhi::CommandQueue::Graphics);
  // }

  chunk.m_is_uptodate = true;

  // bool needs_initialize_all = false;
  //
  // if (!bank.m_vertex_buffer || vertex_count > bank.m_vertex_count ||
  //     index_count > bank.m_vertex_count) {
  //   // Need to create new set of buffers (either not enough space to upload,
  //   or
  //   // not initialized yet)
  //   if (!allow_full_rebuild) {
  //     return;
  //   }
  //   bank.m_vertex_count = vertex_count;
  //   bank.m_index_count = index_count;
  //   if (bank.m_vertex_count)
  //     bank.m_vertex_buffer = m_backend->GetDevice()->createBuffer(
  //         nvrhi::BufferDesc()
  //             .setIsVertexBuffer(true)
  //             .setByteSize(vertex_count * sizeof(gfx::basic_vertex))
  //             .setInitialState(nvrhi::ResourceStates::VertexBuffer)
  //             .setCpuAccess(nvrhi::CpuAccessMode::Write)
  //             .setKeepInitialState(true));
  //   if (bank.m_index_count)
  //     bank.m_index_buffer = m_backend->GetDevice()->createBuffer(
  //         nvrhi::BufferDesc()
  //             .setIsIndexBuffer(true)
  //             .setByteSize(index_count * sizeof(gfx::basic_index))
  //             .setInitialState(nvrhi::ResourceStates::IndexBuffer)
  //             .setCpuAccess(nvrhi::CpuAccessMode::Write)
  //             .setKeepInitialState(true));
  //   else
  //     bank.m_index_buffer = nullptr;
  //   needs_initialize_all = true;
  // }
  //
  //// Upload vertex chunks
  // if (bank.m_vertex_buffer) {
  //   for (auto &chunk : bank.m_chunks) {
  //     if (!needs_initialize_all && chunk.m_is_uptodate) {
  //       continue;
  //     }
  //     pass.m_command_list_preparation->writeBuffer(
  //         bank.m_vertex_buffer, chunk.m_vertices.data(),
  //         chunk.m_vertex_count * sizeof(gfx::basic_vertex),
  //         chunk.m_vertex_offset * sizeof(gfx::basic_vertex));
  //   }
  // }
  //
  //// Upload index chunks
  // if (bank.m_index_buffer) {
  //   for (auto &chunk : bank.m_chunks) {
  //     if (!needs_initialize_all && chunk.m_is_uptodate) {
  //       continue;
  //     }
  //     chunk.m_is_uptodate = true;
  //     if (chunk.m_indexed)
  //       pass.m_command_list_preparation->writeBuffer(
  //           bank.m_index_buffer, chunk.m_indices.data(),
  //           chunk.m_index_count * sizeof(gfx::basic_index),
  //           chunk.m_index_offset * sizeof(gfx::basic_index));
  //   }
  // }

  // Set uptodate flag
  // if (!needs_further_updates) bank.m_is_uptodate = true;
}

nvrhi::InputLayoutHandle NvRenderer::GetInputLayout(DrawType draw_type) {
  // auto &input_layout = m_input_layouts[static_cast<size_t>(draw_type)];
  // if (!input_layout) {
  //   switch (draw_type) {
  //     case DrawType::Model: {
  //       break;
  //     }
  //   }
  // }
  // return input_layout;
  return nullptr;
}

void NvRenderer::UpdateDrawData(const RenderPass &pass,
                                const glm::dmat4 &transform,
                                const glm::dmat4 &history_transform,
                                float opacity_threshold, float opacity_mult,
                                float selfillum, const glm::vec3 &diffuse) {
  switch (pass.m_type) {
    case RenderPassType::RendererWarmUp:
      return;
    case RenderPassType::ShadowMap: {
      PushConstantsShadow data{};
      data.m_modelviewprojection = pass.m_projection * transform;
      data.m_alpha_threshold = opacity_threshold;
      pass.m_command_list_draw->setPushConstants(&data, sizeof(data));
      break;
    }
    case RenderPassType::CubeMap: {
      PushConstantsCubemap data{};
      data.m_modelview = static_cast<glm::mat3x4>(glm::transpose(transform));
      data.m_alpha_mult = opacity_mult;
      data.m_alpha_threshold = opacity_threshold;
      data.m_selfillum =
          selfillum * Config()->m_lighting.m_selfillum_multiplier;
      data.m_diffuse = diffuse;
      pass.m_command_list_draw->setPushConstants(&data, sizeof(data));
      break;
    }
    case RenderPassType::Deferred:
    case RenderPassType::Forward: {
      PushConstantsDraw data{};
      data.m_modelview = static_cast<glm::mat3x4>(glm::transpose(transform));
      data.m_modelview_history =
          static_cast<glm::mat3x4>(glm::transpose(history_transform));
      data.m_alpha_mult = opacity_mult;
      data.m_alpha_threshold = opacity_threshold;
      data.m_selfillum =
          selfillum * Config()->m_lighting.m_selfillum_multiplier;
      data.m_diffuse = diffuse;
      pass.m_command_list_draw->setPushConstants(&data, sizeof(data));
      break;
    }
  }
}

void NvRenderer::UpdateDrawDataLine(const RenderPass &pass,
                                    const glm::dmat4 &transform,
                                    const glm::dmat4 &history_transform,
                                    const Line &line) {
  switch (pass.m_type) {
    case RenderPassType::Deferred:
    case RenderPassType::Forward: {
      PushConstantsLine data{};
      data.m_modelview = static_cast<glm::mat3x4>(glm::transpose(transform));
      data.m_modelview_history =
          static_cast<glm::mat3x4>(glm::transpose(history_transform));
      data.m_color = line.m_color;
      data.m_line_weight = line.m_line_width;
      data.m_metalness = line.m_metalness;
      data.m_roughness = line.m_roughness;
      pass.m_command_list_draw->setPushConstants(&data, sizeof(data));
      break;
    }
    default:
      return;
  }
}

void NvRenderer::BindConstants(const RenderPass &pass,
                               nvrhi::GraphicsState &gfx_state) {
  gfx_state.setFramebuffer(pass.m_framebuffer);
  gfx_state.setViewport(pass.m_viewport_state);
  // switch (pass.m_type) {
  //   case RenderPassType::CubeMap:
  //     gfx_state.addBindingSet(m_binding_set_cubedrawconstants);
  //     break;
  //   case RenderPassType::Forward:
  //     gfx_state.addBindingSet(m_binding_set_drawconstants);
  //     gfx_state.addBindingSet(
  //         m_binding_set_forward[m_environment->GetCurrentSetIndex()]);
  //     break;
  //   case RenderPassType::Deferred:
  //     gfx_state.addBindingSet(m_binding_set_drawconstants);
  //     break;
  //   case RenderPassType::ShadowMap:
  //     gfx_state.addBindingSet(m_binding_set_shadowdrawconstants);
  //     break;
  // }
}

bool NvRenderer::BindMaterial(material_handle handle, DrawType draw_type,
                              const RenderPass &pass,
                              nvrhi::GraphicsState &gfx_state,
                              float &alpha_threshold) {
  if (pass.m_type == RenderPassType::RendererWarmUp) return true;
  if (!handle || handle > m_material_cache.size()) {
    return false;
  }
  MaterialCache &cache = m_material_cache[handle - 1];

  if (!cache.ShouldRenderInPass(pass)) return false;

  auto pipeline_index = Constants::GetPipelineIndex(pass.m_type, draw_type);

  if (!cache.m_pipelines[pipeline_index]) {
    return false;
  }

  cache.m_last_position_requested = pass.m_origin;

  if (cache.m_last_texture_updates[pipeline_index] <
      cache.GetLastStreamingTextureUpdate()) {
    cache.m_template->CreateBindingSet(pipeline_index, cache);
  }

  if (!cache.m_binding_sets[pipeline_index]) return false;

  cache.m_last_frame_requested = GetCurrentFrame();
  cache.UpdateLastStreamingTextureUse(pass.m_origin);

  gfx_state.setPipeline(cache.m_pipelines[pipeline_index]);
  gfx_state.addBindingSet(cache.m_binding_sets[pipeline_index]);

  if (cache.m_opacity) {
    alpha_threshold =
        (pass.m_type == RenderPassType::Forward ? -cache.m_opacity.value()
                                                : cache.m_opacity.value());
  } else {
    alpha_threshold = (pass.m_type == RenderPassType::Forward ? 0.f : .5f);
  }

  return true;
}

bool NvRenderer::BindLineMaterial(NvRenderer::DrawType draw_type,
                                  const NvRenderer::RenderPass &pass,
                                  nvrhi::GraphicsState &gfx_state) {
  if (draw_type != DrawType::Model || pass.m_type != RenderPassType::Deferred)
    return false;

  gfx_state.setPipeline(m_pso_line);
  gfx_state.addBindingSet(m_binding_set_line);

  return true;
}

bool NvRenderer::BindGeometry(gfx::geometry_handle handle,
                              const RenderPass &pass,
                              nvrhi::GraphicsState &gfx_state,
                              nvrhi::DrawArguments &args, bool &indexed) {
  if (!handle.bank || !handle.chunk || handle.bank > m_geometry_banks.size())
    return false;
  auto &bank = m_geometry_banks[handle.bank - 1];
  if (handle.chunk > bank.m_chunks.size()) return false;
  auto &chunk = bank.m_chunks[handle.chunk - 1];
  bank.m_last_frame_requested = pass.m_frame_index;
  chunk.m_last_frame_requested = pass.m_frame_index;
  chunk.m_last_position_requested = pass.m_origin;
  bool is_uploading;
  UpdateGeometry(handle, pass, &is_uploading);
  if (is_uploading) return false;
  // if (bank.m_uploaded_query &&
  //     !m_backend->GetDevice()->pollEventQuery(bank.m_uploaded_query))
  //   return false;
  //  gfx_state.addVertexBuffer(
  //      nvrhi::VertexBufferBinding()
  //          .setBuffer(bank.m_vertex_buffer)
  //          .setSlot(0)
  //          .setOffset(chunk.m_vertex_offset * sizeof(gfx::basic_vertex))
  //          .setSize(chunk.m_vertex_count * sizeof(gfx::basic_vertex)));
  //  gfx_state.setIndexBuffer(
  //      nvrhi::IndexBufferBinding()
  //          .setBuffer(bank.m_index_buffer)
  //          .setFormat(sizeof(gfx::basic_index) == 2 ? nvrhi::Format::R16_UINT
  //                                                   :
  //                                                   nvrhi::Format::R32_UINT)
  //          .setOffset(chunk.m_index_offset * sizeof(gfx::basic_index))
  //          .setSize(chunk.m_index_count * sizeof(gfx::basic_index)));
  gfx_state.addVertexBuffer(
      nvrhi::VertexBufferBinding().setBuffer(chunk.m_vertex_buffer).setSlot(0));
  gfx_state.setIndexBuffer(nvrhi::IndexBufferBinding()
                               .setBuffer(chunk.m_index_buffer)
                               .setFormat(sizeof(gfx::basic_index) == 2
                                              ? nvrhi::Format::R16_UINT
                                              : nvrhi::Format::R32_UINT));
  args.setStartVertexLocation(0).setStartIndexLocation(0).setVertexCount(
      chunk.m_indexed ? chunk.m_index_count : chunk.m_vertex_count);
  indexed = chunk.m_indexed;
  return chunk.m_vertex_buffer && (!indexed || chunk.m_index_buffer);
}

void NvRenderer::Render(TAnimModel *Instance, const RenderPass &pass) {
  if (!pass.m_draw_tanimobj) return;
  m_drawcall_counter = &m_drawcalls_tanimobj;
  if (!Instance->m_visible) {
    return;
  }

  double distancesquared = glm::distance2(Instance->location(), pass.m_origin);

  pass.m_command_list_draw->beginMarker(Instance->m_name.c_str());
  TSubModel::iInstance = reinterpret_cast<size_t>(Instance);
  auto instance_scope = m_motion_cache->SetInstance(Instance);
  if (Instance->pModel) {
    // renderowanie rekurencyjne submodeli
    Render(Instance->pModel, Instance->Material(), distancesquared, pass);
  }
  pass.m_command_list_draw->endMarker();
}

bool NvRenderer::Render(TDynamicObject *Dynamic, const RenderPass &pass) {
  if (!pass.m_draw_dynamic) return false;
  m_drawcall_counter = &m_drawcalls_dynamic;

  // lod visibility ranges are defined for base (x 1.0) viewing distance. for
  // render we adjust them for actual range multiplier and zoom
  double squaredistance;
  glm::dvec3 const originoffset = Dynamic->vPosition - pass.m_origin;
  squaredistance =
      glm::length2(originoffset / static_cast<double>(Global.ZoomFactor));

  pass.m_command_list_draw->beginMarker(Dynamic->name().c_str());
  TSubModel::iInstance = reinterpret_cast<size_t>(Dynamic);
  auto instance_scope = m_motion_cache->SetInstance(Dynamic);
  // render
  if (Dynamic->mdLowPolyInt) {
    // Render_lowpoly(Dynamic, squaredistance, false);
    Render(Dynamic->mdLowPolyInt, Dynamic->Material(), squaredistance, pass);
  }
  if (Dynamic->mdLoad) {
    // renderowanie nieprzezroczystego ładunku
    Render(Dynamic->mdLoad, Dynamic->Material(), squaredistance, pass);
  }
  if (Dynamic->mdModel) {
    // main model
    Render(Dynamic->mdModel, Dynamic->Material(), squaredistance, pass);
  }
  // optional attached models
  for (auto *attachment : Dynamic->mdAttachments) {
    Render(attachment, Dynamic->Material(), squaredistance, pass);
  }
  // optional coupling adapters
  // Render_coupler_adapter(Dynamic, squaredistance, end::front);
  // Render_coupler_adapter(Dynamic, squaredistance, end::rear);

  // TODO: check if this reset is needed. In theory each object should render
  // all parts based on its own instance data anyway?
  // if (Dynamic->btnOn)
  //  Dynamic->TurnOff();  // przywrócenie domyślnych pozycji submodeli

  pass.m_command_list_draw->endMarker();
  return true;
}

bool NvRenderer::Render(TModel3d *Model, material_data const *material,
                        float const Squaredistance, const RenderPass &pass) {
  // int alpha = material ? material->textures_alpha : 0x30300030;
  // if (pass.m_type == RenderPassType::Forward) {
  //   if (!(alpha & Model->iFlags & 0x2F2F002F)) {
  //     // nothing to render
  //     return false;
  //   }
  // } else {
  //   alpha ^=
  //       0x0F0F000F;  // odwrócenie flag tekstur, aby wyłapać nieprzezroczyste
  //   if (!(alpha & Model->iFlags & 0x1F1F001F)) {
  //     // czy w ogóle jest co robić w tym cyklu?
  //     return false;
  //   }
  // }

  // pass.m_command_list_draw->beginMarker(Model->NameGet().c_str());
  // Model->Root->fSquareDist = Squaredistance;  // zmienna globalna!
  //
  //// setup
  // Model->Root->ReplacableSet((material ? material->replacable_skins :
  // nullptr),
  //                            alpha);
  //
  //// render
  // Render(Model->Root, pass);
  //
  // pass.m_command_list_draw->endMarker();
  // post-render cleanup

  return true;
}

void NvRenderer::Render(TSubModel *Submodel, const RenderPass &pass) {
  // for (; Submodel; Submodel = Submodel->Next) {
  //   if (!Submodel->iVisible) continue;
  //
  //  if (Submodel->fSquareDist >= Submodel->fSquareMaxDist ||
  //      Submodel->fSquareDist < Submodel->fSquareMinDist)
  //    continue;
  //
  //  if (m_batched_instances.find(CombinePointers(
  //          reinterpret_cast<const void *>(TSubModel::iInstance), Submodel))
  //          == m_batched_instances.end() &&
  //      (Submodel->iAlpha & Submodel->iFlags &
  //       ((pass.m_type == RenderPassType::Forward) ? 0x2F : 0x1F)) &&
  //      Submodel->eType < TP_ROTATOR) {
  //    nvrhi::GraphicsState gfx_state{};
  //
  //    nvrhi::DrawArguments draw_arguments{};
  //    bool indexed;
  //    float alpha_threshold;
  //
  //    if (!BindGeometry(Submodel->m_geometry.handle, pass, gfx_state,
  //                      draw_arguments, indexed))
  //      return;
  //    // textures...
  //    if (Submodel->m_material < 0) {  // zmienialne skóry
  //      if (!BindMaterial(Submodel->ReplacableSkinId[-Submodel->m_material],
  //                        DrawType::Model, pass, gfx_state, alpha_threshold))
  //        return;
  //    } else {
  //      // również 0
  //      if (!BindMaterial(Submodel->m_material, DrawType::Model, pass,
  //                        gfx_state, alpha_threshold))
  //        return;
  //    }
  //    pass.m_command_list_draw->beginMarker(Submodel->pName.c_str());
  //
  //    float emission =
  //        Submodel->fLight > Global.fLuminance ? Submodel->f4Emision.a : 0.f;
  //
  //    BindConstants(pass, gfx_state);
  //
  //    pass.m_command_list_draw->setGraphicsState(gfx_state);
  //
  //    auto &motion_cache = m_motion_cache->Get(Submodel);
  //    UpdateDrawData(
  //        pass, pass.m_transform * motion_cache.m_cached_transform,
  //        pass.m_history_transform * motion_cache.m_history_transform,
  //        alpha_threshold, 1.f, emission);
  //
  //    if (indexed)
  //      pass.m_command_list_draw->drawIndexed(draw_arguments);
  //    else
  //      pass.m_command_list_draw->draw(draw_arguments);
  //    m_drawcall_counter->Draw(draw_arguments);
  //    pass.m_command_list_draw->endMarker();
  //  }
  //  if (Submodel->Child != nullptr) {
  //    if (Submodel->iAlpha & Submodel->iFlags &
  //        ((pass.m_type == RenderPassType::Forward) ? 0x002F0000
  //                                                  : 0x001F0000)) {
  //      Render(Submodel->Child, pass);
  //    }
  //  }
  //}
}

bool NvRenderer::Render_cab(TDynamicObject const *Dynamic,
                            const RenderPass &pass) {
  // if (!pass.m_draw_dynamic) return false;
  // m_drawcall_counter = &m_drawcalls_dynamic;
  // if (FreeFlyModeFlag || !Dynamic->bDisplayCab ||
  //     (Dynamic->mdKabina == Dynamic->mdModel)) {
  //   return false;
  // }
  // TSubModel::iInstance = reinterpret_cast<size_t>(Dynamic);
  // auto instance_scope = m_motion_cache->SetInstance(Dynamic);
  // if (Dynamic->mdKabina)
  //   return Render(Dynamic->mdKabina, Dynamic->Material(), 0.0, pass);
  // else
  //   return false;
  return true;
}

void NvRenderer::Animate(AnimObj &instance, const glm::dvec3 &origin,
                         uint64_t frame) {
  instance.m_was_once_animated = true;
  instance.m_model->RaAnimate(frame);
  instance.m_model->RaPrepare();
  auto instance_scope = m_motion_cache->SetInstance(instance.m_model);
  TSubModel::iInstance = reinterpret_cast<size_t>(instance.m_model);
  instance.m_renderable.Reset();
  double distance = glm::distance2(instance.m_origin, origin);
  if (!instance.m_animatable) distance = -1.;
  Animate(instance.m_renderable, instance.m_model->Model(),
          instance.m_model->Material(), distance, instance.m_origin,
          glm::radians(instance.m_model->vAngle), frame);
}

void NvRenderer::Animate(DynObj &instance, const glm::dvec3 &origin,
                         uint64_t frame) {
  // setup
  TSubModel::iInstance = reinterpret_cast<size_t>(
      instance.m_dynamic);  // żeby nie robić cudzych animacji
  instance.m_dynamic
      ->ABuLittleUpdate(  // ustawianie zmiennych submodeli dla wspólnego modelu
          0);

  glm::dmat4 dynamic_transform =
      glm::translate(instance.m_location) *
      static_cast<glm::dmat4>(instance.m_dynamic->mMatrix);

  const material_data *material = instance.m_dynamic->Material();

  bool render_kabina = Global.pCamera.m_owner == instance.m_dynamic &&
                       instance.m_dynamic->mdKabina;

  auto instance_scope = m_motion_cache->SetInstance(instance.m_dynamic);
  instance.m_renderable.Reset();
  instance.m_renderable_kabina.Reset();
  // render
  if (instance.m_dynamic->mdLowPolyInt) {
    // Render_lowpoly(Dynamic, squaredistance, false);
    Animate(instance.m_renderable, instance.m_dynamic->mdLowPolyInt, material,
            instance.m_distance, dynamic_transform, frame);
  }
  if (instance.m_dynamic->mdLoad) {
    // renderowanie nieprzezroczystego ładunku
    Animate(instance.m_renderable, instance.m_dynamic->mdLoad, material,
            instance.m_distance,
            dynamic_transform * glm::translate(static_cast<glm::dvec3>(
                                    instance.m_dynamic->LoadOffset)),
            frame);
  }
  if (instance.m_dynamic->mdModel) {
    // main model
    Animate(instance.m_renderable, instance.m_dynamic->mdModel, material,
            instance.m_distance, dynamic_transform, frame);
  }
  // optional attached models
  for (auto *attachment : instance.m_dynamic->mdAttachments) {
    Animate(instance.m_renderable, attachment, material, instance.m_distance,
            dynamic_transform, frame);
  }

  if (render_kabina) {
    Animate(instance.m_renderable_kabina, instance.m_dynamic->mdKabina,
            material, instance.m_distance, dynamic_transform, frame);
  }

  if (instance.m_dynamic->btnOn) instance.m_dynamic->TurnOff();
}

void NvRenderer::Animate(Renderable &renderable, TModel3d *Model,
                         const material_data *material, double distance,
                         const glm::dvec3 &position, const glm::vec3 &angle,
                         uint64_t frame) {
  return Animate(
      renderable, Model, material, distance,
      glm::translate(static_cast<glm::dvec3>(position)) *
          glm::rotate(static_cast<double>(angle.y), glm::dvec3{0., 1., 0.}) *
          glm::rotate(static_cast<double>(angle.x), glm::dvec3{1., 0., 0.}) *
          glm::rotate(static_cast<double>(angle.z), glm::dvec3{0., 0., 1.}),
      frame);
}

void NvRenderer::Animate(Renderable &renderable, TModel3d *Model,
                         const material_data *material, double distance,
                         const glm::dmat4 &transform, uint64_t frame) {
  Model->Root->pRoot = Model;
  // render
  Animate(renderable, Model->Root, material, distance, transform, frame);
}

void NvRenderer::Animate(Renderable &renderable, TSubModel *Submodel,
                         const material_data *material, double distance,
                         const glm::dmat4 &transform, uint64_t frame) {
  static material_handle null_material[5]{0};
  const material_handle *replacable =
      material ? material->replacable_skins : null_material;
  const int alpha = material ? material->textures_alpha : 0x30300030;

  for (; Submodel; Submodel = Submodel->Next) {
    if (!Submodel->iVisible) continue;

    if (distance >= 0. && (distance >= Submodel->fSquareMaxDist ||
                           distance < Submodel->fSquareMinDist))
      continue;

    glm::dmat4 model_transform = transform;

    if (Submodel->iFlags & 0xC000) {
      glm::mat4 anim_transform{1.f};
      if (Submodel->fMatrix) {
        anim_transform = glm::make_mat4(Submodel->fMatrix->readArray());
      }
      if (Submodel->b_aAnim != TAnimType::at_None) {
        Submodel->RaAnimation(anim_transform, Submodel->b_aAnim);
      }
      model_transform =
          model_transform * static_cast<glm::dmat4>(anim_transform);
    }

    if (Submodel->eType < TP_ROTATOR &&
        m_batched_instances.find(CombinePointers(
            reinterpret_cast<const void *>(TSubModel::iInstance), Submodel)) ==
            m_batched_instances.end()) {
      auto &item = renderable.m_items.emplace_back();

      auto &motion_cache = m_motion_cache->Get(Submodel);

      motion_cache.m_history_transform =
          std::exchange(motion_cache.m_cached_transform, model_transform);

      item.m_transform = motion_cache.m_cached_transform;
      item.m_history_transform = motion_cache.m_history_transform;

      item.m_material = Submodel->m_material < 0
                            ? replacable[-Submodel->m_material]
                            : Submodel->m_material;
      item.m_geometry = Submodel->m_geometry.handle;
      item.m_render_in_forward = alpha & Submodel->Flags() & 0x0000002F;
      item.m_opacity = Submodel->Opacity;

      if ((Submodel->f4Emision.a > 0.f) &&
          (Global.fLuminance < Submodel->fLight))
        item.m_selfillum = Submodel->f4Emision.a;
      else
        item.m_selfillum = 0.f;

      if (distance < 0.) {
        item.m_sqr_distance_max = Submodel->fSquareMaxDist;
        item.m_sqr_distance_min = Submodel->fSquareMinDist;
        item.m_history_transform = item.m_transform;
      } else {
        item.m_sqr_distance_max = std::numeric_limits<double>::max();
        item.m_sqr_distance_min = -std::numeric_limits<double>::max();
      }

      item.m_diffuse = Submodel->f4Diffuse;
      renderable.m_render_in_forward |= item.m_render_in_forward;
      renderable.m_render_in_deferred |= !item.m_render_in_forward;
    }

    else if (Submodel->eType == TP_FREESPOTLIGHT) {
      if ((Submodel->f4Emision.a <= 0.f) ||
          (Global.fLuminance >= Submodel->fLight))
        continue;
      Renderable::SpotLight &spotlight = renderable.m_spotlights.emplace_back();
      spotlight.m_origin = model_transform[3];
      spotlight.m_direction = glm::normalize(-model_transform[2]);
      spotlight.m_color =
          Submodel->f4Diffuse * Config()->m_lighting.m_freespot_multiplier;
      spotlight.m_cos_outer_cone = Submodel->fCosFalloffAngle;
      spotlight.m_cos_inner_cone = Submodel->fCosHotspotAngle;
      spotlight.m_radius = glm::sqrt(
          1.f / Config()->m_lighting.m_luminance_threshold *
          glm::dot(spotlight.m_color, glm::vec3(.2126f, .7152f, .0722f)));
    }

    // auto last_frame =
    //     std::exchange(motion_cache.m_cached_frame, m_frame_index);
    // assert(last_frame != pass.m_frame_index);

    Animate(renderable, Submodel->Child, material, distance, model_transform,
            frame);
  }
}

void NvRenderer::GeometryChunk::UpdateBounds() {
  glm::vec3 min{std::numeric_limits<float>::max()};
  glm::vec3 max{-std::numeric_limits<float>::max()};
  for (const auto &vertex : m_vertices) {
    min = glm::min(min, vertex.position);
    max = glm::max(max, vertex.position);
  }
  m_origin = .5f * (max + min);
  m_extent = .5f * (max - min);
}

bool NvRenderer::MaterialCache::ShouldRenderInPass(
    const RenderPass &pass) const {
  if ((pass.m_type == RenderPassType::ShadowMap ||
       pass.m_type == RenderPassType::CubeMap) &&
      m_shadow_rank > Global.gfx_shadow_rank_cutoff) {
    return false;
  }
  return true;
}
