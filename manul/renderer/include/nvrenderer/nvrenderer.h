#pragma once

//STL includes required by EU07 includes
#include <deque>
#include <stdexcept>
#include <memory>
#include <variant>
#include <math.h>

// OpenGL includes required by EU07 includes
#include <glad/glad.h> // for GLuint, GLint

// Div. includes required by EU07 includes
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <nvrhi/nvrhi.h>
#include <yaml-cpp/yaml.h>

#include <entt/container/dense_map.hpp>
#include <future>
#include <mutex>

// EU07 includes
#include <Classes.h>
#include <scene.h>

#include "quadtree.h"
#include "renderer.h"
#include "nvrenderer_enums.h"
#include "resource_registry.h"
#include "sky.h"

template <typename Renderer>
struct MaRendererConstants {
  static constexpr size_t NumMaterialPasses() noexcept {
    return static_cast<size_t>(Renderer::RenderPassType::Num);
  }
  static constexpr size_t NumDrawTypes() noexcept {
    return static_cast<size_t>(Renderer::DrawType::Num);
  }
  static constexpr size_t NumShadowPipelines() noexcept {
    return NumDrawTypes() << 1;
  }
  static constexpr size_t NumMaterialPipelines() noexcept {
    return NumDrawTypes() * NumMaterialPasses();
  }

  static constexpr size_t GetPipelineIndex(
      typename Renderer::RenderPassType pass_type,
      typename Renderer::DrawType draw_type) {
    return static_cast<size_t>(draw_type) * NumMaterialPasses() +
           static_cast<size_t>(pass_type);
  }

  static constexpr size_t GetDefaultShadowPipelineIndex(
      typename Renderer::DrawType type, const bool masked) {
    return static_cast<size_t>(type) << 1 | !!masked & 1;
  }
};

class NvRenderer : public gfx_renderer, public MaResourceRegistry {
 public:
  enum class Api { D3D11, D3D12, Vulkan };

  using RenderPassType = RendererEnums::RenderPassType;
  using DrawType = RendererEnums::DrawType;

 public:
  using Constants = MaRendererConstants<NvRenderer>;
  // SZ renderer interface implementation
  virtual ~NvRenderer() override {}
  virtual bool Init(GLFWwindow *Window) override;
  virtual bool AddViewport(
      const global_settings::extraviewport_config &conf) override;
  virtual void Shutdown() override;
  virtual bool Render() override;
  virtual void SwapBuffers() override;
  virtual float Framerate() override;
  virtual gfx::geometrybank_handle Create_Bank() override;
  virtual gfx::geometry_handle Insert(gfx::index_array &Indices,
                                      gfx::vertex_array &Vertices,
                                      gfx::geometrybank_handle const &Geometry,
                                      int const Type) override;
  virtual gfx::geometry_handle Insert(gfx::vertex_array &Vertices,
                                      gfx::geometrybank_handle const &Geometry,
                                      int const Type) override;
  virtual bool Replace(gfx::vertex_array &Vertices,
                       gfx::geometry_handle const &Geometry, int const Type,
                       std::size_t const Offset = 0) override;
  virtual bool Append(gfx::vertex_array &Vertices,
                      gfx::geometry_handle const &Geometry,
                      int const Type) override;
  virtual gfx::index_array const &Indices(
      gfx::geometry_handle const &Geometry) const override;
  virtual gfx::vertex_array const &Vertices(
      gfx::geometry_handle const &Geometry) const override;
  virtual material_handle Fetch_Material(std::string const &Filename,
                                         bool const Loadnow = true) override;
  virtual void Bind_Material(material_handle const Material,
                             TSubModel const *sm = nullptr,
                             lighting_data const *lighting = nullptr) override;
  virtual IMaterial const *Material(
      material_handle const Material) const override;
  virtual std::shared_ptr<gl::program> Fetch_Shader(
      std::string const &name) override;
  virtual texture_handle Fetch_Texture(
      std::string const &Filename, bool const Loadnow = true,
      GLint format_hint = GL_SRGB_ALPHA) override;
  virtual void Bind_Texture(texture_handle const Texture) override;
  virtual void Bind_Texture(std::size_t const Unit,
                            texture_handle const Texture) override;
  virtual ITexture &Texture(texture_handle const Texture) override;
  virtual ITexture const &Texture(texture_handle const Texture) const override;
  virtual void Pick_Control_Callback(
      std::function<void(TSubModel const *, const glm::vec2)> Callback)
      override;
  virtual void Pick_Node_Callback(
      std::function<void(scene::basic_node *)> Callback) override;
  virtual TSubModel const *Pick_Control() const override;
  virtual scene::basic_node const *Pick_Node() const override;
  virtual glm::dvec3 Mouse_Position() const override;
  virtual void Update(double const Deltatime) override;
  virtual void Update_Pick_Control() override;
  virtual void Update_Pick_Node() override;
  virtual glm::dvec3 Update_Mouse_Position() override;
  virtual bool Debug_Ui_State(std::optional<bool>) override;
  virtual std::string const &info_times() const override;
  virtual std::string const &info_stats() const override;
  virtual imgui_renderer *GetImguiRenderer() override;
  virtual void MakeScreenshot() override;
  NvRenderer(Api api) : m_api(api), MaResourceRegistry(nullptr) {}

  static bool d3d11_renderer_register;
  static bool d3d12_renderer_register;
  static bool vk_renderer_register;
  static bool renderer_register;

  struct IFrustumTester {
    virtual bool IntersectSphere(const glm::dvec3 &origin,
                                 double radius) const = 0;
    virtual bool IntersectBox(const glm::dvec3 &origin,
                              const glm::dvec3 &extent) const = 0;
    virtual bool IntersectBox(const glm::dmat4x3 &transform,
                              const glm::dvec3 &origin,
                              const glm::dvec3 &extent) const = 0;
  };

  class NvRendererBackend *GetBackend() const { return m_backend.get(); }

  // private:
  friend class NvRendererBackend;
  friend class NvRendererMessageCallback;
  friend class NvImguiRenderer;
  friend class NvTextureManager;
  friend struct NvGbuffer;
  friend struct NvSsao;
  friend struct FullScreenPass;
  friend struct NvTAA;
  friend struct NvFSR;
  friend struct MaEnvironment;
  friend struct MaShadowMap;
  Api m_api;
  std::shared_ptr<class NvRendererBackend> m_backend;
  std::shared_ptr<class NvRendererMessageCallback> m_message_callback;
  std::shared_ptr<class NvImguiRenderer> m_imgui_renderer;
  std::shared_ptr<struct NvGbuffer> m_gbuffer;
  std::shared_ptr<struct NvGbuffer> m_gbuffer_cube;
  std::shared_ptr<struct NvGbuffer> m_gbuffer_shadow;
  std::shared_ptr<struct GbufferLighting> m_gbuffer_lighting;
  std::shared_ptr<struct GbufferBlitPass> m_gbuffer_blit;
  std::shared_ptr<struct NvSsao> m_ssao;
  std::shared_ptr<struct MotionCache> m_motion_cache;
  std::shared_ptr<struct InstanceBank> m_instance_cache;
  std::shared_ptr<struct NvTAA> m_taa;
  std::shared_ptr<struct TonemapPass> m_tonemap;
  std::shared_ptr<struct NvFSR> m_fsr;
  std::shared_ptr<struct MaEnvironment> m_environment;
  std::shared_ptr<struct MaShadowMap> m_shadow_map;
  std::shared_ptr<struct MaContactShadows> m_contact_shadows;
  std::shared_ptr<struct Bloom> m_bloom;
  std::shared_ptr<struct Sky> m_sky;
  std::shared_ptr<struct MaAutoExposure> m_auto_exposure;

  std::shared_ptr<struct MaConfig> m_config;
  struct MaConfig *GetConfig() const { return m_config.get(); }
  static struct MaConfig *Config();

  struct GeometryBounds {
    glm::dvec3 m_origin;
    glm::dvec3 m_extent;
  };
  GeometryBounds GetGeometryBounds(gfx::geometry_handle const &handle) const;

  size_t GetCurrentFrame() const;
  void DebugUi();

  glm::dmat4 m_previous_view_proj;
  glm::dmat4 m_previous_view;

  bool m_debug_ui_active;

  nvrhi::CommandListHandle m_command_list;
  nvrhi::EventQueryHandle m_upload_event_query;

  std::mutex m_mtx_context_lock;
  glm::dvec3 m_previous_env_position;
  uint64_t m_previous_env_frame;

  using section_sequence = std::vector<scene::basic_section *>;
  using distancecell_pair = std::pair<double, scene::basic_cell *>;
  using cell_sequence = std::vector<distancecell_pair>;
  struct RenderPass {
    RenderPassType m_type;
    double m_draw_range;
    uint64_t m_frame_index;
    bool m_draw_shapes = true;
    bool m_draw_lines = true;
    bool m_draw_tanimobj = true;
    bool m_draw_dynamic = true;
    bool m_draw_track = true;
    bool m_draw_instances = true;
    bool m_sort_batches = true;
    bool m_sort_transparents = true;
    nvrhi::CommandListHandle m_command_list_draw;
    nvrhi::CommandListHandle m_command_list_preparation;
    nvrhi::FramebufferHandle m_framebuffer;
    nvrhi::ViewportState m_viewport_state;
    glm::dmat4 m_projection;
    glm::dvec3 m_origin;
    glm::dmat3 m_transform;
    glm::dmat3 m_history_transform;
    double ScreenRadius(const glm::dvec3 &origin, double radius) const {
      glm::dvec4 ndc =
          m_projection *
          glm::dvec4(0., radius, -glm::length(origin - m_origin), 1.);
      return glm::abs(ndc.y / ndc.w);
    }
    double ScreenRadius(double sqr_distance, double radius) const {
      glm::dvec4 ndc =
          m_projection * glm::dvec4(0., radius, -glm::sqrt(sqr_distance), 1.);
      return glm::abs(ndc.y / ndc.w);
    }
    bool NeedsHistory() const { return m_type == RenderPassType::Deferred; }
    bool SphereVisible(const glm::dvec3 &origin, double radius) const {
      return !m_frustum_tester ||
             m_frustum_tester->IntersectSphere(origin - m_origin, radius);
    }
    bool BoxVisible(const glm::dvec3 &origin, const glm::dvec3 &extent) const {
      return !m_frustum_tester ||
             m_frustum_tester->IntersectBox(origin - m_origin, extent);
    }
    bool BoxVisible(glm::dmat4x3 transform, const glm::dvec3 &origin,
                    const glm::dvec3 &extent) const {
      transform[3] -= m_origin;
      return !m_frustum_tester ||
             m_frustum_tester->IntersectBox(transform, origin, extent);
    }
    bool Visible(scene::bounding_area const &area) const {
      return SphereVisible(area.center, area.radius);
    }
    bool Visible(TDynamicObject *dynamic) const {
      return SphereVisible(dynamic->GetPosition(), dynamic->radius() * 1.25);
    }
    const IFrustumTester *m_frustum_tester;
  };

  struct GeometryChunk {
    std::vector<gfx::basic_vertex> m_vertices;
    std::vector<gfx::basic_index> m_indices;
    size_t m_vertex_offset;
    size_t m_index_offset;
    size_t m_vertex_count;
    size_t m_index_count;
    bool m_indexed;
    bool m_is_uptodate = false;
    uint64_t m_last_frame_requested = static_cast<uint64_t>(-1);
    uint64_t m_last_frame_updated = static_cast<uint64_t>(-1);
    uint64_t m_last_frame_uploaded = static_cast<uint64_t>(-1);
    nvrhi::BufferHandle m_index_buffer;
    nvrhi::BufferHandle m_vertex_buffer;
    nvrhi::EventQueryHandle m_chunk_ready_handle;
    std::future<void> m_chunk_upload;
    glm::vec3 m_origin;
    glm::vec3 m_extent;
    glm::dvec3 m_last_position_requested;
    std::function<bool(gfx::vertex_array &, int)> m_replace_impl;
    void UpdateBounds();
  };

  struct GeometryBank {
    nvrhi::BufferHandle m_index_buffer;
    nvrhi::BufferHandle m_vertex_buffer;
    size_t m_vertex_count;
    size_t m_index_count;
    std::vector<GeometryChunk> m_chunks;
    bool m_is_uptodate = false;
    nvrhi::EventQueryHandle m_uploaded_query;
    uint64_t m_last_frame_requested = static_cast<uint64_t>(-1);
  };
  std::vector<GeometryBank> m_geometry_banks;

  nvrhi::BufferHandle m_culling_view_data;
  struct GeometryBatch {
    struct CullingViewData {
      glm::vec4 m_frustum_planes[6];
      glm::mat4 m_projection;
    };
    struct CullingPushConstants {
      glm::vec3 m_origin;
      float m_instance_radius;
      float m_min_radius;
      float m_max_radius;
      uint32_t m_num_instances;
    };
    struct CullingCommandBuffer {
      uint32_t m_index_count_per_instance;
      uint32_t m_instance_count;
      uint32_t m_start_index_location;
      int32_t m_base_vertex_location;
      uint32_t m_start_instance_location;
    };
    gfx::geometry_handle m_geometry;
    material_handle m_material;
    std::vector<glm::dmat4> m_transforms;
    glm::vec3 m_diffuse;
    glm::dvec3 m_origin;
    glm::dvec3 m_extent;
    glm::dvec3 m_instance_origin;
    nvrhi::BufferHandle m_static_instance_buffer;
    nvrhi::BufferHandle m_instance_buffer;
    nvrhi::BufferHandle m_static_command_buffer;
    nvrhi::BufferHandle m_command_buffer;
    nvrhi::BindingSetHandle m_culling_binding_set;
    double m_sqr_distance_max;
    double m_sqr_distance_min;
    double m_instance_radius;
    std::string m_name;
  };
  std::vector<GeometryBatch> m_geometry_batches;
  QuadTree m_batch_quadtree;

  struct TrackBatch {
    std::string m_name;
    glm::dvec3 m_origin;
    glm::dvec3 m_extent;
    glm::dvec3 m_geometry_origin;
    glm::vec3 m_diffuse;
    std::vector<nvrhi::DrawArguments> m_draw_commands;
    gfx::geometry_handle m_geometry;
    material_handle m_material;
    nvrhi::BufferHandle m_command_buffer;
    struct BufferRegion {
      size_t m_start_vertex;
      size_t m_vertex_count;
      size_t m_start_index;
      size_t m_index_count;
    };
    std::vector<BufferRegion> m_regions;
    std::vector<bool> m_regions_need_update;
  };

  bool TrackBatchPartialUpdate(size_t batch_index, size_t track_index,
                               gfx::vertex_array &vertices);

  std::vector<TrackBatch> m_track_batches;
  QuadTree m_track_quadtree;

  struct Shape {
    std::string m_name;
    glm::dvec3 m_origin;
    glm::dvec3 m_extent;
    glm::dvec3 m_geometry_origin;
    gfx::geometry_handle m_geometry;
    bool m_draw_in_opaque;
    bool m_draw_in_transparent;
    material_handle m_material;
    glm::vec3 m_diffuse;
  };

  std::vector<Shape> m_shapes;
  QuadTree m_shape_quadtree;

  struct Line {
    std::string m_name;
    glm::dvec3 m_origin;
    glm::dvec3 m_extent;
    glm::dvec3 m_geometry_origin;
    gfx::geometry_handle m_geometry;
    float m_line_width;
    glm::vec3 m_color;
    float m_metalness;
    float m_roughness;
  };

  std::vector<Line> m_lines;
  QuadTree m_line_quadtree;

  struct Renderable {
    struct RenderableItem {
      bool m_render_in_forward = false;
      glm::dmat4x3 m_transform;
      glm::dmat4x3 m_history_transform;
      gfx::geometry_handle m_geometry;
      material_handle m_material;
      double m_sqr_distance_max;
      double m_sqr_distance_min;
      float m_opacity;
      float m_selfillum;
      glm::vec3 m_diffuse;
    };
    struct SpotLight {
      glm::vec3 m_color;
      glm::dvec3 m_origin;
      glm::vec3 m_direction;
      float m_radius;
      float m_cos_inner_cone;
      float m_cos_outer_cone;
    };
    std::vector<RenderableItem> m_items;
    std::vector<SpotLight> m_spotlights;
    bool m_render_in_deferred = false;
    bool m_render_in_forward = false;
    void Reset() {
      m_items.clear();
      m_spotlights.clear();
      m_render_in_deferred = false;
      m_render_in_forward = false;
    }
  };

  struct AnimObj {
    std::string m_name;
    glm::dvec3 m_origin;
    glm::dvec3 m_extent;
    bool m_render_in_deferred;
    bool m_render_in_forward;
    bool m_animatable;
    bool m_was_once_animated = false;
    double m_sqr_distance_max;
    double m_sqr_distance_min;
    double m_distance;
    TAnimModel *m_model;
    Renderable m_renderable;
  };

  std::vector<AnimObj> m_animateds;
  QuadTree m_animated_quadtree;

  struct DynObj {
    TDynamicObject *m_dynamic;
    glm::dvec3 m_location;
    double m_radius;
    double m_distance;
    Renderable m_renderable;
    Renderable m_renderable_kabina;
  };
  uint32_t m_dynamic_with_kabina;
  std::vector<DynObj> m_dynamics;
  std::vector<uint32_t> m_renderable_dynamics;
  std::vector<Renderable::SpotLight> m_renderable_spotlights;

  struct Cell {
    scene::basic_cell *m_cell;
    glm::dvec3 m_origin;
    glm::dvec3 m_extent;
  };

  std::vector<Cell> m_cells;
  QuadTree m_cell_quadtree;

  void GatherSpotLights(const RenderPass &pass);

  void UpdateGeometry(gfx::geometry_handle handle, const RenderPass &pass,
                      bool *p_is_uploading = nullptr);
  section_sequence m_sectionqueue;
  cell_sequence m_cellqueue;
  struct MaterialCache;
  struct MaterialTemplate : public MaResourceRegistry {
    struct TextureBinding {
      std::string m_name;
      std::string m_sampler_name;
      int m_hint;
      texture_handle m_default_texture;
    };
    nvrhi::static_vector<TextureBinding, 8> m_texture_bindings;
    std::array<nvrhi::GraphicsPipelineHandle, Constants::NumMaterialPipelines()>
        m_pipelines;
    std::array<MaResourceMappingSet, Constants::NumMaterialPipelines()>
        m_resource_mappings;
    std::string m_name;
    std::string m_masked_shadow_texture;
    nvrhi::BindingLayoutHandle m_binding_layout;
    virtual bool CreateBindingSet(size_t pipeline_index, MaterialCache &cache);
    [[nodiscard]] nvrhi::IGraphicsPipeline *GetPipeline(
        RenderPassType pass_type, DrawType draw_type, bool alpha_masked) const;
    virtual void Init(const YAML::Node &conf);
    MaterialTemplate(const std::string &name, NvRenderer *renderer)
        : m_name(name), MaResourceRegistry(renderer), m_renderer(renderer) {}
    NvRenderer *m_renderer;
  };
  struct MaterialCache : public IMaterial, public MaResourceRegistry {
    explicit MaterialCache(MaterialTemplate *my_template)
        : MaResourceRegistry(my_template), m_template(my_template) {}
    void Init() { InitResourceRegistry(); }
    std::array<uint64_t, Constants::NumMaterialPipelines()>
        m_last_texture_updates{};
    std::string m_name;
    std::array<nvrhi::IGraphicsPipeline *, Constants::NumMaterialPipelines()>
        m_pipelines;
    std::array<nvrhi::BindingSetHandle, Constants::NumMaterialPipelines()>
        m_binding_sets;
    std::array<texture_handle, 8> m_texture_handles;
    nvrhi::BindingSetHandle m_binding_set;
    nvrhi::BindingSetHandle m_binding_set_cubemap;
    nvrhi::BindingSetHandle m_binding_set_shadow;
    glm::dvec3 m_last_position_requested;
    MaterialTemplate *m_template;
    bool m_masked_shadow;
    int m_shadow_rank;
    uint64_t m_last_frame_requested = 0;
    std::optional<float> m_opacity;
    glm::dvec2 m_size;
    [[nodiscard]] bool ShouldRenderInPass(const RenderPass &pass) const;
    void finalize(bool Loadnow) override {}
    bool update() override { return false; }
    [[nodiscard]] float get_or_guess_opacity() const override {
      if (m_opacity.has_value()) return *m_opacity;
      if (m_masked_shadow)
        return 0.0f;
      else
        return 0.5f;
    }
    bool is_translucent() const override { return m_masked_shadow; }
    glm::vec2 GetSize() const override { return m_size; }
    std::string GetName() const override { return m_name; }
    std::optional<float> GetSelfillum() const override { return std::nullopt; }
    texture_handle GetTexture(int slot) const override { return m_texture_handles[slot]; };
    int GetShadowRank() const override { return m_shadow_rank; }
  };

  std::unordered_map<std::string, material_handle> m_material_map;
  std::vector<MaterialCache> m_material_cache;
  bool m_scene_initialized = false;
  bool m_envir_can_filter = false;

  std::array<nvrhi::InputLayoutHandle, static_cast<size_t>(DrawType::Num)>
      m_input_layouts;
  nvrhi::InputLayoutHandle GetInputLayout(DrawType draw_type);
  struct DrawConstants {
    glm::mat4 m_jittered_projection;
    glm::mat4 m_projection;
    glm::mat4 m_projection_history;
  };
  struct CubeDrawConstants {
    glm::mat4 m_face_projection;
  };
  struct PushConstantsDraw {
    glm::mat3x4 m_modelview;
    glm::mat3x4 m_modelview_history;
    glm::vec3 m_diffuse;
    float m_alpha_threshold;
    float m_alpha_mult;
    float m_selfillum;
  };
  typedef PushConstantsDraw PushConstantsCubemap;
  struct PushConstantsLine {
    glm::mat3x4 m_modelview;
    glm::mat3x4 m_modelview_history;
    glm::vec3 m_color;
    float m_line_weight;
    float m_metalness;
    float m_roughness;
  };

  struct PushConstantsShadow {
    glm::mat4 m_modelviewprojection;
    float m_alpha_threshold;
  };
  bool InitMaterials();
  std::unordered_map<std::string_view, std::shared_ptr<MaterialTemplate>>
      m_material_templates;

  nvrhi::FramebufferHandle m_framebuffer_forward;
  nvrhi::BindingLayoutHandle m_binding_layout_drawconstants;
  nvrhi::BindingLayoutHandle m_binding_layout_cubedrawconstants;
  nvrhi::BindingLayoutHandle m_binding_layout_shadowdrawconstants;
  nvrhi::BindingLayoutHandle m_binding_layout_forward;
  nvrhi::BindingSetHandle m_binding_set_drawconstants;
  nvrhi::BindingSetHandle m_binding_set_cubedrawconstants;
  nvrhi::BindingSetHandle m_binding_set_shadowdrawconstants;
  nvrhi::BindingSetHandle m_binding_set_line;
  std::array<nvrhi::BindingSetHandle, 2> m_binding_set_forward;
  nvrhi::BufferHandle m_cubedrawconstant_buffer;
  nvrhi::BufferHandle m_drawconstant_buffer;
  std::array<nvrhi::ShaderHandle, Constants::NumDrawTypes()> m_vertex_shader;
  std::array<nvrhi::ShaderHandle, Constants::NumDrawTypes()>
      m_vertex_shader_cubemap;
  std::array<nvrhi::ShaderHandle, Constants::NumDrawTypes()>
      m_vertex_shader_shadow;
  nvrhi::BindingLayoutHandle m_binding_layout_shadow_masked;
  nvrhi::ShaderHandle m_pixel_shader_shadow_masked;
  std::array<nvrhi::InputLayoutHandle, Constants::NumDrawTypes()>
      m_input_layout;
  std::array<nvrhi::GraphicsPipelineHandle, Constants::NumShadowPipelines()>
      m_pso_shadow;
  nvrhi::GraphicsPipelineHandle m_pso_line;

  std::condition_variable m_cv_next_frame;
  glm::mat4 m_previous_projection;

  struct DrawCallStat {
    void Reset() {
      m_drawcalls = 0;
      m_triangles = 0;
    }
    int m_drawcalls = 0;
    int m_triangles = 0;
    void Draw(nvrhi::DrawArguments const &args) {
      ++m_drawcalls;
      m_triangles += args.vertexCount * args.instanceCount / 3;
    }
  };
  DrawCallStat m_drawcalls_shape{};
  DrawCallStat m_drawcalls_line{};
  DrawCallStat m_drawcalls_tanimobj{};
  DrawCallStat m_drawcalls_dynamic{};
  DrawCallStat m_drawcalls_track{};
  DrawCallStat m_drawcalls_instances{};
  DrawCallStat *m_drawcall_counter = nullptr;
  bool m_draw_shapes = true;
  bool m_draw_lines = true;
  bool m_draw_tanimobj = true;
  bool m_draw_dynamic = true;
  bool m_draw_track = true;
  bool m_draw_instances = true;
  bool m_sort_batches = true;
  bool m_sort_transparents = true;
  bool m_pause_animations = false;
  std::unordered_set<const void *> m_batched_instances;

  static const void *CombinePointers(const void *a, const void *b) {
    union {
      struct {
        uint32_t a;
        uint32_t b;
      };
      const void *c;
    } val{};
    val.a = static_cast<uint32_t>(reinterpret_cast<uint64_t>(a));
    val.b = static_cast<uint32_t>(reinterpret_cast<uint64_t>(b));
    return val.c;
  }

  static double SqrDistToScreenSize(double sqr_dist, double radius) {
    double dist = glm::sqrt(sqr_dist);
    glm::dmat4 std_projection =
        glm::perspectiveRH_ZO(glm::radians(45.), 1., .1, 2e5);
    glm::dvec4 ndc = std_projection * glm::dvec4(0., radius, -dist, 1.);
    return glm::abs(ndc.y / ndc.w);
  };

  void UpdateDrawData(const RenderPass &pass, const glm::dmat4 &transform,
                      const glm::dmat4 &history_transform,
                      float opacity_threshold, float opacity_mult,
                      float selfillum, const glm::vec3& diffuse);

  void UpdateDrawDataLine(const RenderPass &pass, const glm::dmat4 &transform,
                          const glm::dmat4 &history_transform,
                          const Line &line);

  void BindConstants(const RenderPass &pass, nvrhi::GraphicsState &gfx_state);

  bool BindMaterial(material_handle handle, DrawType draw_type,
                    const RenderPass &pass, nvrhi::GraphicsState &gfx_state,
                    float &alpha_threshold);

  bool BindLineMaterial(DrawType draw_type, const RenderPass &pass,
                        nvrhi::GraphicsState &gfx_state);

  bool BindGeometry(gfx::geometry_handle handle, const RenderPass &pass,
                    nvrhi::GraphicsState &gfx_state, nvrhi::DrawArguments &args,
                    bool &indexed);

  void GatherModelsForBatching();
  void GatherTracksForBatching();
  void GatherShapesForBatching();
  void GatherAnimatedsForBatching();
  void GatherCellsForAnimation();
  void GatherDynamics();

  void RenderBatches(const RenderPass &pass);
  void RenderTracks(const RenderPass &pass);
  void RenderShapes(const RenderPass &pass);
  void RenderLines(const RenderPass &pass);
  void RenderAnimateds(const RenderPass &pass);
  void RenderKabina(const RenderPass &pass);
  void Render(const Renderable &renderable, const RenderPass &pass,
              const glm::dvec3 &history_origin, const double distance);
  void Animate(const glm::dvec3 &origin, double radius, uint64_t frame);

  void Render(TAnimModel *Instance, const RenderPass &pass);
  bool Render(TDynamicObject *Dynamic, const RenderPass &pass);
  bool Render(TModel3d *Model, material_data const *Material,
              float const Squaredistance, const RenderPass &pass);
  void Render(TSubModel *Submodel, const RenderPass &pass);
  bool Render_cab(TDynamicObject const *Dynamic, const RenderPass &pass);

  void Animate(AnimObj &instance, const glm::dvec3 &origin, uint64_t frame);
  void Animate(DynObj &instance, const glm::dvec3 &origin, uint64_t frame);
  void Animate(Renderable &renderable, TModel3d *Model,
               const material_data *material, double distance,
               const glm::dvec3 &position, const glm::vec3 &angle,
               uint64_t frame);
  void Animate(Renderable &renderable, TModel3d *Model,
               const material_data *material, double distance,
               const glm::dmat4 &transform, uint64_t frame);
  void Animate(Renderable &renderable, TSubModel *Submodel,
               const material_data *material, double distance,
               const glm::dmat4 &transform, uint64_t frame);

#if LIBMANUL_WITH_D3D12
  bool InitForD3D12(GLFWwindow *Window);
#endif

#if LIBMANUL_WITH_VULKAN
  bool InitForVulkan(GLFWwindow *Window);
#endif
};