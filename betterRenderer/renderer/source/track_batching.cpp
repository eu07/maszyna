#include "config.h"
#include "motioncache.h"
#include "nvrenderer/nvrenderer.h"
#include "nvrendererbackend.h"

#include <AnimModel.h>
#include <simulation.h>

namespace BatchingUtils {

struct RadiusTester {
  glm::dvec3 m_origin;
  double m_radius_sqr;
  bool BoxVisible(const glm::dvec3& origin, const glm::dvec3& extent) const {
    glm::dvec3 closest = glm::clamp(m_origin, origin - extent, origin + extent);
    return glm::distance2(closest, m_origin) < m_radius_sqr;
  }
  RadiusTester(const glm::dvec3& origin, double radius)
      : m_origin(origin), m_radius_sqr(radius * radius) {}
};

}  // namespace BatchingUtils

void NvRenderer::GatherModelsForBatching() {
  struct BatchedId {
    gfx::geometry_handle m_geometry;
    material_handle m_material;
    int m_tile_x;
    int m_tile_y;
  };
  struct BatchedIdHash {
    constexpr static size_t HashCombine(std::size_t seed, std::size_t hash) {
      return hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    size_t operator()(const BatchedId& rhs) const {
      std::hash<uint64_t> hasher;
      return HashCombine(HashCombine(HashCombine(hasher(rhs.m_geometry),
                                                 hasher(rhs.m_material)),
                                     hasher(rhs.m_tile_x)),
                         hasher(rhs.m_tile_y));
    }
  };
  struct BatchedIdEq {
    bool operator()(const BatchedId& lhs, const BatchedId& rhs) const {
      return lhs.m_geometry == rhs.m_geometry &&
             lhs.m_material == rhs.m_material && lhs.m_tile_x == rhs.m_tile_x &&
             lhs.m_tile_y == rhs.m_tile_y;
    }
  };
  std::unordered_map<BatchedId, uint32_t, BatchedIdHash, BatchedIdEq> batches{};
  double tile_size_std = 500.;
  double tile_size_loddable = 250.;
  auto get_batch_for_geometry =
      [&batches, this, tile_size_std, tile_size_loddable](
          gfx::geometry_handle geometry, material_handle material,
          const glm::dvec3& origin, bool likely_loddable, bool& is_new,
          const glm::vec3& diffuse) -> GeometryBatch* {
    // is_new = true;
    // return &m_geometry_batches.emplace_back();
    double tile_size = likely_loddable ? tile_size_loddable : tile_size_std;
    BatchedId id{};
    id.m_geometry = geometry;
    id.m_material = material;
    id.m_tile_x = glm::floor(origin.x / tile_size);
    id.m_tile_y = glm::floor(origin.z / tile_size);
    auto [it, is_actually_new] = batches.emplace(id, 0);
    is_new = is_actually_new;
    if (is_actually_new) {
      it->second = m_geometry_batches.size();
      auto& batch = m_geometry_batches.emplace_back();
      batch.m_name =
          std::to_string(id.m_tile_x) + "_" + std::to_string(id.m_tile_y);
      batch.m_diffuse = diffuse;
      return &batch;
    }
    return &m_geometry_batches[it->second];
  };
  auto sqr_dist_to_screensize =
      [this](double sqr_dist, gfx::geometry_handle geometry) -> double {
    const auto [origin, extent] = GetGeometryBounds(geometry);
    double radius = glm::length(extent);
    return SqrDistToScreenSize(sqr_dist, radius);
  };
  for (auto section : simulation::Region->m_sections) {
    if (!section) continue;
    for (const auto& cell : section->m_cells) {
      for (auto instance : cell.m_instancesopaque) {
        static bool (*is_allowed_animation)(TAnimType) =
            [](TAnimType anim) -> bool {
          switch (anim) {
            case TAnimType::at_None:
            case TAnimType::at_Wind:
              return true;
            default:
              return false;
          }
        };
        static void (*for_each_submodel)(
            TSubModel const* sm, std::function<bool(TSubModel const*)>) =
            [](TSubModel const* sm,
               std::function<bool(TSubModel const*)> callback) {
              for (; sm; sm = sm->Next) {
                if (callback(sm)) for_each_submodel(sm->Child, callback);
              }
            };
        auto root = instance->Model()->GetSMRoot();
        auto alpha = instance->Material() ? instance->Material()->textures_alpha
                                          : 0x20200020;
        bool likely_loddable = false;
        for_each_submodel(root, [&](TSubModel const* sm) -> bool {
          if (sm->fSquareMinDist >= 0.) {
            likely_loddable = true;
            return false;
          }
          return true;
        });
        bool fully_batchable = true;
        root->ReplacableSet(instance->Material()->replacable_skins, 0);
        glm::dvec3 angle = glm::radians(instance->Angles());
        glm::dmat4 transform =
            glm::translate(static_cast<glm::dvec3>(instance->location())) *
            glm::rotate(static_cast<double>(angle.y), glm::dvec3{0., 1., 0.}) *
            glm::rotate(static_cast<double>(angle.x), glm::dvec3{1., 0., 0.}) *
            glm::rotate(static_cast<double>(angle.z), glm::dvec3{0., 0., 1.});
        for_each_submodel(root, [&](TSubModel const* sm) -> bool {
          // Model światła wyłączonego -> zmienny stan widoczności
          if (std::find(instance->LightsOff.begin(), instance->LightsOff.end(),
                        sm) != instance->LightsOff.end()) {
            fully_batchable = false;
            return false;
          }
          // Model światła włączonego -> zmienny stan widoczności
          if (std::find(instance->LightsOn.begin(), instance->LightsOn.end(),
                        sm) != instance->LightsOn.end()) {
            fully_batchable = false;
            return false;
          }
          // Model animowany przez event
          if (std::find_if(instance->m_animlist.begin(),
                           instance->m_animlist.end(),
                           [sm](std::shared_ptr<TAnimContainer> ac) -> bool {
                             return ac->pSubModel == sm;
                           }) != instance->m_animlist.end()) {
            fully_batchable = false;
            return false;
          }
          // Model animowany
          if ((sm->Flags() & 0x4000) && !is_allowed_animation(sm->b_Anim)) {
            fully_batchable = false;
            return false;
          }
          // Model przezroczysty -> kontynować sprawdzanie w hierarchii
          if (sm->Flags() & 0x0000002F & alpha) {
            fully_batchable = false;
            return true;
          }
          if (sm->eType >= TP_ROTATOR) return true;
          m_batched_instances.emplace(CombinePointers(instance, sm));
          float4x4 matrix;
          sm->ParentMatrix(&matrix);
          if (!sm->m_geometry.handle) return true;
          auto material = sm->m_material < 0
                              ? sm->ReplacableSkinId[-sm->m_material]
                              : sm->m_material;
          if (!material) return true;
          auto sm_transform =
              transform *
              static_cast<glm::dmat4>(glm::make_mat4(matrix.readArray()));
          auto [origin, extent] = GetGeometryBounds(sm->m_geometry.handle);
          bool new_batch;
          auto batch = get_batch_for_geometry(sm->m_geometry.handle, material,
                                              sm_transform[3], likely_loddable,
                                              new_batch, sm->f4Diffuse);
          if (new_batch) {
            batch->m_geometry = sm->m_geometry.handle;
            batch->m_instance_origin = origin;
            batch->m_material = material;
            batch->m_name +=
                "_" + instance->Model()->NameGet() + "_" + sm->pName;
            batch->m_sqr_distance_max = sm->fSquareMaxDist;
            batch->m_sqr_distance_min = sm->fSquareMinDist;
          }
          batch->m_transforms.emplace_back(sm_transform);
          return true;
        });
        // Model w całości statyczny? -> można pomijać ewaluację TAnimObj
        if (fully_batchable) {
          m_batched_instances.emplace(CombinePointers(instance, nullptr));
        }
      }
    }
  }
  glm::dvec3 world_min{std::numeric_limits<double>::max()};
  glm::dvec3 world_max{-std::numeric_limits<double>::max()};
  for (auto& batch : m_geometry_batches) {
    auto [origin, extent] = GetGeometryBounds(batch.m_geometry);
    batch.m_instance_radius = glm::length(extent);
    glm::dvec3 batch_min{std::numeric_limits<double>::max()};
    glm::dvec3 batch_max{-std::numeric_limits<double>::max()};
    for (const auto& transform : batch.m_transforms) {
      for (int i = 0; i < 8; ++i) {
        glm::dvec3 corner =
            transform *
            glm::dvec4{i & 1 ? origin.x + extent.x : origin.x - extent.x,
                       i & 2 ? origin.y + extent.y : origin.y - extent.y,
                       i & 4 ? origin.z + extent.z : origin.z - extent.z, 1.};
        batch_min = glm::min(batch_min, corner);
        batch_max = glm::max(batch_max, corner);
      }
    }
    world_min = glm::min(batch_min, world_min);
    world_max = glm::max(batch_max, world_max);
    batch.m_origin = .5 * (batch_max + batch_min);
    batch.m_extent = .5 * (batch_max - batch_min);
  }
  QuadTreeBuilder builder{world_min.x, world_min.z, world_max.x, world_max.z};
  for (int i = 0; i < m_geometry_batches.size(); ++i) {
    const auto& batch = m_geometry_batches[i];
    builder.Insert(i, batch.m_origin - batch.m_extent,
                   batch.m_origin + batch.m_extent);
  }
  m_batch_quadtree.Build(builder);
}

void NvRenderer::GatherTracksForBatching() {
  struct TrackBatchId {
    glm::dvec3 m_geometry_origin;
    material_handle m_material;
  };
  struct TrackBatchIdHash {
    constexpr static size_t HashCombine(std::size_t seed, std::size_t hash) {
      return hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    size_t operator()(const TrackBatchId& rhs) const {
      std::hash<uint64_t> hasher;
      std::hash<double> hasher_d;
      return HashCombine(
          HashCombine(HashCombine(hasher(rhs.m_material),
                                  hasher_d(rhs.m_geometry_origin.x)),
                      hasher_d(rhs.m_geometry_origin.y)),
          hasher_d(rhs.m_geometry_origin.z));
    }
  };
  struct TrackBatchIdEq {
    bool operator()(const TrackBatchId& lhs, const TrackBatchId& rhs) const {
      return lhs.m_material == rhs.m_material &&
             lhs.m_geometry_origin == rhs.m_geometry_origin;
    }
  };
  struct TrackEntry {
    glm::dvec3 m_origin;
    glm::dvec3 m_extent;
    gfx::geometry_handle* m_original_geometry;
  };
  struct TrackBatch {
    glm::dvec3 m_geometry_origin{0.};
    material_handle m_material = 0;
    glm::dvec3 m_world_min = glm::dvec3{std::numeric_limits<double>::max()};
    glm::dvec3 m_world_max = glm::dvec3{-std::numeric_limits<double>::max()};
    std::vector<TrackEntry> m_tracks{};
  };

  std::vector<TrackBatch> batches{};
  std::unordered_map<TrackBatchId, uint32_t, TrackBatchIdHash, TrackBatchIdEq>
      batch_mapping{};
  glm::dvec3 world_min{std::numeric_limits<double>::max()};
  glm::dvec3 world_max{-std::numeric_limits<double>::max()};
  auto add_geometry = [&](TTrack* track, material_handle material,
                          gfx::geometry_handle* geometry) {
    auto [origin, extent] = GetGeometryBounds(*geometry);
    origin += track->m_origin;
    world_min = glm::min(world_min, origin - extent);
    world_max = glm::max(world_max, origin + extent);
    TrackBatchId id{};
    id.m_geometry_origin = track->m_origin;
    id.m_material = material;
    auto [it, is_actually_new] = batch_mapping.emplace(id, 0);
    if (is_actually_new) {
      it->second = batches.size();
      auto& batch = batches.emplace_back();
      batch.m_material = material;
      batch.m_geometry_origin = track->m_origin;
    }
    auto& batch = batches[it->second];
    batch.m_world_min = glm::min(batch.m_world_min, origin - extent);
    batch.m_world_max = glm::max(batch.m_world_max, origin + extent);
    auto& entry = batch.m_tracks.emplace_back();
    entry.m_extent = extent;
    entry.m_origin = origin;
    entry.m_original_geometry = geometry;
  };
  for (auto section : simulation::Region->m_sections) {
    if (!section) continue;
    for (const auto& cell : section->m_cells) {
      for (const auto track : cell.m_paths) {
        if (!track) continue;
        for (auto& geometry : track->Geometry1)
          add_geometry(track, track->m_material1, &geometry);
        for (auto& geometry : track->Geometry2)
          add_geometry(track, track->m_material2, &geometry);
        if (auto extension = track->SwitchExtension.get())
          add_geometry(track, extension->m_material3, &extension->Geometry3);
      }
    }
  }
  QuadTreeBuilder builder{world_min.x, world_min.z, world_max.x, world_max.z};
  m_track_batches.resize(batches.size());
  for (int i = 0; i < batches.size(); ++i) {
    const auto& batch = batches[i];
    builder.Insert(i, batch.m_world_min, batch.m_world_max);
    auto& track_batch = m_track_batches[i];
    track_batch.m_diffuse = {1.f, 1.f, 1.f};
    track_batch.m_name = Material(batch.m_material)->GetName();
    track_batch.m_origin = .5 * (batch.m_world_max + batch.m_world_min);
    track_batch.m_extent = .5 * (batch.m_world_max - batch.m_world_min);
    track_batch.m_material = batch.m_material;
    track_batch.m_geometry_origin = batch.m_geometry_origin;
    std::vector<gfx::basic_vertex> vertices{};
    std::vector<gfx::vertex_userdata> userdatas{};
    std::vector<gfx::basic_index> indices{};
    for (int j = 0; j < batch.m_tracks.size(); ++j) {
      const auto& track = batch.m_tracks[j];
      if (!(track.m_original_geometry->bank &&
            track.m_original_geometry->chunk))
        continue;
      auto& chunk = m_geometry_banks[track.m_original_geometry->bank - 1]
                        .m_chunks[track.m_original_geometry->chunk - 1];
      // assert(chunk.m_indices.size());
      track_batch.m_draw_commands.emplace_back()
          .setStartVertexLocation(vertices.size())
          .setStartIndexLocation(indices.size())
          .setVertexCount(chunk.m_indices.size());
      auto& region = track_batch.m_regions.emplace_back();
      region.m_start_vertex = vertices.size();
      region.m_vertex_count = chunk.m_vertices.size();
      region.m_start_index = indices.size();
      region.m_index_count = chunk.m_indices.size();
      vertices.insert(vertices.end(), chunk.m_vertices.begin(),
                      chunk.m_vertices.end());
      indices.insert(indices.end(), chunk.m_indices.begin(),
                     chunk.m_indices.end());
      chunk.m_replace_impl = std::bind(&NvRenderer::TrackBatchPartialUpdate,
                                       this, i, j, std::placeholders::_1);
    }
    track_batch.m_regions_need_update.resize(
        track_batch.m_draw_commands.size());
    auto bank = Create_Bank();
    track_batch.m_geometry =
        Insert(indices, vertices, userdatas, bank, GL_TRIANGLES);
  }
  m_track_quadtree.Build(builder);
}

void NvRenderer::GatherShapesForBatching() {
  float mult_lines = Config()->m_weight_lines;
  float mult_traction = Config()->m_weight_tractions;
  glm::dvec3 world_min{std::numeric_limits<double>::max()};
  glm::dvec3 world_max{-std::numeric_limits<double>::max()};
  auto add_shape = [&](const scene::shape_node::shapenode_data& data) {
    if (!data.material || !data.geometry) return;
    auto [origin, extent] = GetGeometryBounds(data.geometry);
    origin += data.origin;
    auto& shape = m_shapes.emplace_back();
    shape.m_extent = extent;
    shape.m_origin = origin;
    shape.m_geometry = data.geometry;
    shape.m_material = data.material;
    shape.m_draw_in_opaque = !data.translucent;
    shape.m_draw_in_transparent = data.translucent;
    shape.m_geometry_origin = data.origin;
    shape.m_diffuse = data.lighting.diffuse;
    world_min = glm::min(world_min, origin - extent);
    world_max = glm::max(world_max, origin + extent);
  };
  auto add_line = [&](const scene::lines_node::linesnode_data& data) {
    auto [origin, extent] = GetGeometryBounds(data.geometry);
    origin += data.origin;
    auto& line = m_lines.emplace_back();
    line.m_extent = extent;
    line.m_origin = origin;
    line.m_geometry = data.geometry;
    line.m_geometry_origin = data.origin;
    line.m_line_width = data.line_width * mult_lines;
    line.m_color = data.lighting.diffuse;
    line.m_metalness = 0.f;
    line.m_roughness = 1.f;
    world_min = glm::min(world_min, origin - extent);
    world_max = glm::max(world_max, origin + extent);
  };
  auto add_traction = [&](const TTraction* traction) {
    auto [origin, extent] = GetGeometryBounds(traction->m_geometry);
    origin += traction->m_origin;
    auto& line = m_lines.emplace_back();
    line.m_extent = extent;
    line.m_origin = origin;
    line.m_geometry = traction->m_geometry;
    line.m_geometry_origin = traction->m_origin;
    line.m_line_width = traction->WireThickness * mult_traction;
    line.m_metalness = 1.f;
    switch (traction->Material) {
      case 1: {
        if (TestFlag(traction->DamageFlag, 1)) {
          line.m_color = {0.00000f, 0.32549f, 0.2882353f};  // zielona miedź
          line.m_roughness = .8f;
        } else {
          line.m_color = {0.35098f, 0.22549f, 0.1f};  // czerwona miedź
          line.m_roughness = .1f;
        }
        break;
      }
      case 2: {
        if (TestFlag(traction->DamageFlag, 1)) {
          line.m_color = {0.10f, 0.10f, 0.10f};  // czarne Al
          line.m_roughness = .8f;
        } else {
          line.m_color = {0.25f, 0.25f, 0.25f};  // srebrne Al
          line.m_roughness = .1f;
        }
        break;
      }
      default: {
        break;
      }
    }
    world_min = glm::min(world_min, origin - extent);
    world_max = glm::max(world_max, origin + extent);
  };
  for (auto section : simulation::Region->m_sections) {
    if (!section) continue;
    for (const auto& shape : section->m_shapes) add_shape(shape.data());
    for (const auto& cell : section->m_cells) {
      for (const auto& shape : cell.m_shapesopaque) add_shape(shape.data());
      for (const auto& shape : cell.m_shapestranslucent)
        add_shape(shape.data());
      for (const auto& line : cell.m_lines) add_line(line.data());
      for (const auto& traction : cell.m_traction) add_traction(traction);
    }
  }

  QuadTreeBuilder shape_builder{world_min.x, world_min.z, world_max.x,
                                world_max.z};
  for (int i = 0; i < m_shapes.size(); ++i) {
    const auto& shape = m_shapes[i];
    shape_builder.Insert(i, shape.m_origin - shape.m_extent,
                         shape.m_origin + shape.m_extent);
  }
  m_shape_quadtree.Build(shape_builder);

  QuadTreeBuilder line_builder{world_min.x, world_min.z, world_max.x,
                               world_max.z};
  for (int i = 0; i < m_lines.size(); ++i) {
    const auto& line = m_lines[i];
    line_builder.Insert(i, line.m_origin - line.m_extent,
                        line.m_origin + line.m_extent);
  }
  m_line_quadtree.Build(line_builder);
}

void NvRenderer::GatherAnimatedsForBatching() {
  glm::dvec3 world_min{std::numeric_limits<double>::max()};
  glm::dvec3 world_max{-std::numeric_limits<double>::max()};
  auto add_instance = [&](TAnimModel* model) {
    if (m_batched_instances.find(CombinePointers(model, nullptr)) !=
        m_batched_instances.end())
      return;
    m_batched_instances.emplace(CombinePointers(model, nullptr));
    auto const flags = model->Flags();
    auto alpha =
        (model->Material() != nullptr ? model->Material()->textures_alpha
                                      : 0x30300030);

    glm::dvec3 origin = model->location();
    glm::dvec3 extent = glm::dvec3(model->radius());
    auto& animated = m_animateds.emplace_back();
    animated.m_extent = extent;
    animated.m_origin = origin;
    animated.m_name = model->name();
    animated.m_model = model;
    animated.m_animatable = model->iNumLights || model->m_animlist.size();
    animated.m_render_in_forward = alpha & flags & 0x2F2F002F;
    animated.m_render_in_deferred = (alpha ^ 0x0F0F000F) & flags & 0x1F1F001F;
    animated.m_sqr_distance_max = model->m_rangesquaredmax;
    animated.m_sqr_distance_min = model->m_rangesquaredmin;
    world_min = glm::min(world_min, origin - extent);
    world_max = glm::max(world_max, origin + extent);
  };
  for (auto section : simulation::Region->m_sections) {
    if (!section) continue;
    for (const auto& cell : section->m_cells) {
      for (const auto& instance : cell.m_instancesopaque)
        add_instance(instance);
      for (const auto& instance : cell.m_instancetranslucent)
        add_instance(instance);
    }
  }
  QuadTreeBuilder builder{world_min.x, world_min.z, world_max.x, world_max.z};
  for (int i = 0; i < m_animateds.size(); ++i) {
    const auto& instance = m_animateds[i];
    builder.Insert(i, instance.m_origin - instance.m_extent,
                   instance.m_origin + instance.m_extent);
  }
  m_animated_quadtree.Build(builder);
}

void NvRenderer::GatherCellsForAnimation() {
  glm::dvec3 world_min{std::numeric_limits<double>::max()};
  glm::dvec3 world_max{-std::numeric_limits<double>::max()};
  auto add_cell = [&](scene::basic_cell* sz_cell) {
    auto& cell = m_cells.emplace_back();
    cell.m_cell = sz_cell;
    cell.m_origin = sz_cell->m_area.center;
    cell.m_extent = glm::dvec3(sz_cell->m_area.radius);
    world_min = glm::min(world_min, cell.m_origin - cell.m_extent);
    world_max = glm::max(world_max, cell.m_origin + cell.m_extent);
  };
  for (auto section : simulation::Region->m_sections) {
    if (!section) continue;
    for (auto& cell : section->m_cells) {
      bool has_switch = false;
      for (const auto& track : cell.m_paths) {
        if (track->SwitchExtension) {
          has_switch = true;
          break;
        }
      }
      if (!has_switch) continue;
      add_cell(&cell);
    }
  }
  QuadTreeBuilder builder{world_min.x, world_min.z, world_max.x, world_max.z};
  for (int i = 0; i < m_cells.size(); ++i) {
    const auto& cell = m_cells[i];
    builder.Insert(i, cell.m_origin - cell.m_extent,
                   cell.m_origin + cell.m_extent);
  }
  m_cell_quadtree.Build(builder);
}

void NvRenderer::GatherDynamics() {
  auto add_dynamic = [&](TDynamicObject* obj) {
    if (!obj->mdModel) return;
    auto& dynamic = m_dynamics.emplace_back();
    dynamic.m_dynamic = obj;
    dynamic.m_radius = obj->radius();
  };
  for (const auto dynamic : simulation::Vehicles.sequence()) {
    add_dynamic(dynamic);
  }
}

bool NvRenderer::TrackBatchPartialUpdate(size_t batch_index, size_t track_index,
                                         gfx::vertex_array& vertices) {
  auto& batch = m_track_batches[batch_index];
  auto& chunk = m_geometry_banks[batch.m_geometry.bank - 1]
                    .m_chunks[batch.m_geometry.chunk - 1];
  const auto& region = batch.m_regions[track_index];
  if (vertices.size() > region.m_vertex_count) return false;
  std::copy(vertices.begin(), vertices.end(),
            chunk.m_vertices.begin() + region.m_start_vertex);
  if (chunk.m_vertex_buffer) batch.m_regions_need_update[track_index] = true;
  return true;
}

void NvRenderer::RenderBatches(const RenderPass& pass) {
  static std::vector<std::pair<size_t, size_t>> batches_to_render;
  batches_to_render.clear();

  if (!pass.m_draw_instances) return;
  m_drawcall_counter = &m_drawcalls_instances;

  auto motion_scope = m_motion_cache->SetInstance(nullptr);
  const auto& motion_cache = m_motion_cache->Get(nullptr);
  glm::dmat4 batch_transform =
      pass.m_transform * motion_cache.m_cached_transform;
  glm::dmat4 history_transform =
      pass.m_history_transform * motion_cache.m_history_transform;

  m_batch_quadtree.ForEach(pass, [&](uint32_t batch_index) {
    GeometryBatch& batch = m_geometry_batches[batch_index];
    glm::dvec3 closest_point =
        glm::clamp(pass.m_origin, batch.m_origin - batch.m_extent,
                   batch.m_origin + batch.m_extent) -
        pass.m_origin;
    if (glm::dot(closest_point, closest_point) >= batch.m_sqr_distance_max)
      return;
    if (batch.m_transforms.size() > 0) {
      double min_dist = std::numeric_limits<double>::max();
      double max_dist = -std::numeric_limits<double>::max();
      for (const auto& instance_transform : batch.m_transforms) {
        if (!pass.SphereVisible(
                instance_transform * glm::dvec4(batch.m_instance_origin, 1.),
                batch.m_instance_radius))
          continue;
        glm::dvec3 offset = instance_transform[3].xyz - pass.m_origin;
        double dist = glm::dot(offset, offset);
        min_dist = glm::min(min_dist, dist);
        max_dist = glm::max(max_dist, dist);
      }
      if (min_dist > max_dist) return;
      if (min_dist >= batch.m_sqr_distance_max ||
          min_dist < batch.m_sqr_distance_min)
        return;

      if (!batch.m_static_instance_buffer) {
        pass.m_command_list_preparation->beginMarker(batch.m_name.c_str());
        std::vector<glm::mat3x4> instances;
        instances.reserve(batch.m_transforms.size());
        glm::dmat4 pre_transform = glm::translate(-batch.m_origin);
        for (const auto& instance_transform : batch.m_transforms) {
          instances.emplace_back(static_cast<glm::mat3x4>(
              glm::transpose(pre_transform * instance_transform)));
        }
        batch.m_static_instance_buffer =
            GetBackend()->GetDevice()->createBuffer(
                nvrhi::BufferDesc()
                    .setByteSize(instances.size() * sizeof(instances[0]))
                    .setIsVertexBuffer(true)
                    .setInitialState(nvrhi::ResourceStates::VertexBuffer)
                    .setKeepInitialState(true));
        pass.m_command_list_preparation->writeBuffer(
            batch.m_static_instance_buffer, instances.data(),
            instances.size() * sizeof(instances[0]));
        pass.m_command_list_preparation->setPermanentBufferState(
            batch.m_static_instance_buffer,
            nvrhi::ResourceStates::VertexBuffer);
        pass.m_command_list_preparation->endMarker();
      }

      const auto material = m_material_cache[batch.m_material - 1];
      if (!material.ShouldRenderInPass(pass)) return;

      union {
        uint64_t m_command_index;
        struct {
          uint32_t m_priority;
          uint32_t m_material_index;
        };
      } command_index;

      command_index.m_priority = 0;
      if (material.m_masked_shadow) command_index.m_priority |= 1;

      command_index.m_material_index = batch.m_material;

      batches_to_render.emplace_back(command_index.m_command_index,
                                     batch_index);

      // if (indexed)
      //   pass.m_command_list_draw->drawIndexedIndirect(0);
      // else
      //   pass.m_command_list_draw->drawIndirect(0);
      //  m_drawcall_counter->Draw(draw_arguments);
    }
  });
  if (pass.m_sort_batches) {
    std::sort(batches_to_render.begin(), batches_to_render.end(),
              [](const auto& lhs, const auto& rhs) -> bool {
                return std::get<0>(lhs) < std::get<0>(rhs);
              });
  }
  for (const auto& [material_handle, batch_index] : batches_to_render) {
    const GeometryBatch& batch = m_geometry_batches[batch_index];
    nvrhi::GraphicsState gfx_state;
    nvrhi::DrawArguments draw_arguments{};
    bool indexed;
    float alpha_threshold;
    if (!BindGeometry(batch.m_geometry, pass, gfx_state, draw_arguments,
                      indexed))
      return;

    if (!BindMaterial(batch.m_material, DrawType::InstancedModel, pass,
                      gfx_state, alpha_threshold))
      return;

    BindConstants(pass, gfx_state);

    gfx_state.addVertexBuffer(nvrhi::VertexBufferBinding().setSlot(1).setBuffer(
        batch.m_static_instance_buffer));

    pass.m_command_list_draw->beginMarker(batch.m_name.c_str());

    pass.m_command_list_draw->setGraphicsState(gfx_state);

    UpdateDrawData(pass, batch_transform * glm::translate(batch.m_origin),
                   history_transform * glm::translate(batch.m_origin),
                   alpha_threshold, 1.f, 0.f, batch.m_diffuse);

    draw_arguments.setInstanceCount(batch.m_transforms.size());
    if (indexed)
      pass.m_command_list_draw->drawIndexed(draw_arguments);
    else
      pass.m_command_list_draw->draw(draw_arguments);

    pass.m_command_list_draw->endMarker();
    m_drawcall_counter->Draw(draw_arguments);
  }
  m_drawcall_counter = nullptr;
}

void NvRenderer::RenderTracks(const RenderPass& pass) {
  if (!pass.m_draw_track) return;
  m_drawcall_counter = &m_drawcalls_track;

  auto motion_scope = m_motion_cache->SetInstance(nullptr);
  const auto& motion_cache = m_motion_cache->Get(nullptr);
  glm::dmat4 batch_transform =
      pass.m_transform * motion_cache.m_cached_transform;
  glm::dmat4 history_transform =
      pass.m_history_transform * motion_cache.m_history_transform;

  m_track_quadtree.ForEach(pass, [&](size_t batch_index) {
    TrackBatch& batch = m_track_batches[batch_index];
    nvrhi::GraphicsState gfx_state;
    nvrhi::DrawArguments draw_arguments{};
    bool indexed;
    float alpha_threshold;
    if (!BindGeometry(batch.m_geometry, pass, gfx_state, draw_arguments,
                      indexed))
      return;
    if (!BindMaterial(batch.m_material, DrawType::Model, pass, gfx_state,
                      alpha_threshold))
      return;

    if (!batch.m_command_buffer) {
      batch.m_command_buffer = GetBackend()->GetDevice()->createBuffer(
          nvrhi::BufferDesc()
              .setByteSize(batch.m_draw_commands.size() *
                           sizeof(nvrhi::DrawArguments))
              .setStructStride(sizeof(nvrhi::DrawArguments))
              .setIsDrawIndirectArgs(true)
              .setInitialState(nvrhi::ResourceStates::IndirectArgument)
              .setKeepInitialState(true));
      pass.m_command_list_preparation->writeBuffer(
          batch.m_command_buffer, batch.m_draw_commands.data(),
          batch.m_draw_commands.size() * sizeof(nvrhi::DrawArguments));
    }

    {  // Partial buffer update (needed immediately for legacy animations)
      if (const auto& chunk = m_geometry_banks[batch.m_geometry.bank - 1]
                                  .m_chunks[batch.m_geometry.chunk - 1];
          chunk.m_vertex_buffer) {
        for (int i = 0; i < batch.m_regions.size(); ++i) {
          if (!batch.m_regions_need_update[i]) continue;
          batch.m_regions_need_update[i] = false;
          const auto& region = batch.m_regions[i];
          pass.m_command_list_preparation->writeBuffer(
              chunk.m_vertex_buffer, &chunk.m_vertices[region.m_start_vertex],
              sizeof(gfx::basic_vertex) * region.m_vertex_count,
              sizeof(gfx::basic_vertex) * region.m_start_vertex);
        }
      }
    }

    gfx_state.setIndirectParams(batch.m_command_buffer);

    BindConstants(pass, gfx_state);

    glm::dmat4 instance_transform = glm::translate(batch.m_geometry_origin);

    pass.m_command_list_draw->beginMarker(batch.m_name.c_str());

    pass.m_command_list_draw->setGraphicsState(gfx_state);

    UpdateDrawData(pass, batch_transform * instance_transform,
                   history_transform * instance_transform, alpha_threshold, 1.f,
                   0.f, batch.m_diffuse);

    pass.m_command_list_draw->drawIndexedIndirect(0,
                                                  batch.m_draw_commands.size());

    m_drawcall_counter->Draw(draw_arguments);

    pass.m_command_list_draw->endMarker();
  });
  m_drawcall_counter = nullptr;
}

void NvRenderer::RenderShapes(const RenderPass& pass) {
  if (!pass.m_draw_shapes) return;
  m_drawcall_counter = &m_drawcalls_shape;

  auto motion_scope = m_motion_cache->SetInstance(nullptr);
  const auto& motion_cache = m_motion_cache->Get(nullptr);
  glm::dmat4 batch_transform =
      pass.m_transform * motion_cache.m_cached_transform;
  glm::dmat4 history_transform =
      pass.m_history_transform * motion_cache.m_history_transform;

  m_shape_quadtree.ForEach(pass, [&](uint32_t shape_index) {
    const auto& shape = m_shapes[shape_index];
    switch (pass.m_type) {
      case RenderPassType::Forward:
        if (!shape.m_draw_in_transparent) return;
        break;
      default:
        if (!shape.m_draw_in_opaque) return;
    }
    nvrhi::GraphicsState gfx_state;
    nvrhi::DrawArguments draw_arguments{};
    bool indexed;
    float alpha_threshold;
    if (!BindGeometry(shape.m_geometry, pass, gfx_state, draw_arguments,
                      indexed))
      return;
    if (!BindMaterial(shape.m_material, DrawType::Model, pass, gfx_state,
                      alpha_threshold))
      return;

    BindConstants(pass, gfx_state);

    glm::dmat4 instance_transform = glm::translate(shape.m_geometry_origin);

    // pass.m_command_list_draw->beginMarker(batch.m_name.c_str());

    pass.m_command_list_draw->setGraphicsState(gfx_state);

    UpdateDrawData(pass, batch_transform * instance_transform,
                   history_transform * instance_transform, alpha_threshold, 1.f,
                   0.f, shape.m_diffuse);

    if (indexed)
      pass.m_command_list_draw->drawIndexed(draw_arguments);
    else
      pass.m_command_list_draw->draw(draw_arguments);
    m_drawcall_counter->Draw(draw_arguments);
  });
}

void NvRenderer::RenderLines(const NvRenderer::RenderPass& pass) {
  if (!pass.m_draw_lines) return;
  m_drawcall_counter = &m_drawcalls_line;

  auto motion_scope = m_motion_cache->SetInstance(nullptr);
  const auto& motion_cache = m_motion_cache->Get(nullptr);
  glm::dmat4 batch_transform =
      pass.m_transform * motion_cache.m_cached_transform;
  glm::dmat4 history_transform =
      pass.m_history_transform * motion_cache.m_history_transform;

  pass.m_command_list_draw->beginMarker("Lines");

  m_line_quadtree.ForEach(pass, [&](uint32_t shape_index) {
    const auto& line = m_lines[shape_index];

    nvrhi::GraphicsState gfx_state;
    nvrhi::DrawArguments draw_arguments{};
    bool indexed;
    if (!BindGeometry(line.m_geometry, pass, gfx_state, draw_arguments,
                      indexed))
      return;
    if (!BindLineMaterial(DrawType::Model, pass, gfx_state)) return;

    BindConstants(pass, gfx_state);

    glm::dmat4 instance_transform = glm::translate(line.m_geometry_origin);

    // pass.m_command_list_draw->beginMarker(batch.m_name.c_str());

    pass.m_command_list_draw->setGraphicsState(gfx_state);

    UpdateDrawDataLine(pass, batch_transform * instance_transform,
                       history_transform * instance_transform, line);

    if (indexed)
      pass.m_command_list_draw->drawIndexed(draw_arguments);
    else
      pass.m_command_list_draw->draw(draw_arguments);
    m_drawcall_counter->Draw(draw_arguments);
  });
  pass.m_command_list_draw->endMarker();
}

void NvRenderer::RenderAnimateds(const RenderPass& pass) {
  auto motion_scope = m_motion_cache->SetInstance(nullptr);
  const auto& motion_cache = m_motion_cache->Get(nullptr);
  glm::dvec3 history_origin = motion_cache.m_history_transform[3];

  struct RenderCommand {
    enum ObjectType { ObjectType_Animated, ObjectType_Dynamic };
    uint32_t m_render_distance;
    ObjectType m_type;
    uint32_t m_index;
    void SetDistance(float distance) {
      union {
        uint32_t asuint;
        float asfloat;
      } val;
      val.asfloat = distance;
      constexpr static uint32_t sign_mask = 1 << 31;
      m_render_distance =
          val.asuint & sign_mask ? val.asuint : sign_mask - val.asuint;
    }
  };

  struct RenderCommandComparer {
    bool operator()(const RenderCommand& lhs, const RenderCommand& rhs) const {
      return lhs.m_render_distance < rhs.m_render_distance;
    }
  };

  static std::vector<RenderCommand> render_commands{};
  render_commands.clear();

  if (pass.m_draw_tanimobj) {
    m_animated_quadtree.ForEach(pass, [&](uint32_t animated_index) {
      const auto& instance = m_animateds[animated_index];

      switch (pass.m_type) {
        case RenderPassType::Forward:
          if (!instance.m_renderable.m_render_in_forward) return;
          break;
        default:
          if (!instance.m_renderable.m_render_in_deferred) return;
      }

      double distance = glm::distance2(instance.m_origin, pass.m_origin);
      if (distance >= instance.m_sqr_distance_max ||
          distance < instance.m_sqr_distance_min)
        return;

      RenderCommand render_command{};
      render_command.SetDistance(distance);
      render_command.m_type = RenderCommand::ObjectType_Animated;
      render_command.m_index = animated_index;

      render_commands.emplace_back(render_command);
    });
  }

  if (pass.m_draw_dynamic) {
    for (uint32_t dynamic_index : m_renderable_dynamics) {
      const auto& dynamic = m_dynamics[dynamic_index];

      switch (pass.m_type) {
        case RenderPassType::Forward:
          if (!dynamic.m_renderable.m_render_in_forward) continue;
          break;
        default:
          if (!dynamic.m_renderable.m_render_in_deferred) continue;
      }

      if (!pass.SphereVisible(dynamic.m_location, dynamic.m_radius)) continue;

      RenderCommand render_command{};
      render_command.SetDistance(dynamic.m_distance);
      render_command.m_type = RenderCommand::ObjectType_Dynamic;
      render_command.m_index = dynamic_index;

      render_commands.emplace_back(render_command);
    }
  }

  if (pass.m_type == RenderPassType::Forward && pass.m_sort_transparents) {
    std::sort(render_commands.begin(), render_commands.end(),
              RenderCommandComparer{});
  }

  for (const auto& command : render_commands) {
    switch (command.m_type) {
      case RenderCommand::ObjectType_Animated:
        m_drawcall_counter = &m_drawcalls_tanimobj;
        Render(m_animateds[command.m_index].m_renderable, pass, history_origin,
               m_animateds[command.m_index].m_distance);
        break;
      case RenderCommand::ObjectType_Dynamic:
        m_drawcall_counter = &m_drawcalls_dynamic;
        Render(m_dynamics[command.m_index].m_renderable, pass, history_origin,
               m_dynamics[command.m_index].m_distance);
        break;
    }
  }
}

void NvRenderer::RenderKabina(const RenderPass& pass) {
  if (!pass.m_draw_dynamic) return;
  if (m_dynamic_with_kabina >= m_dynamics.size()) return;

  m_drawcall_counter = &m_drawcalls_dynamic;

  auto motion_scope = m_motion_cache->SetInstance(nullptr);
  const auto& motion_cache = m_motion_cache->Get(nullptr);
  glm::dvec3 history_origin = motion_cache.m_history_transform[3];

  const auto& dynamic = m_dynamics[m_dynamic_with_kabina];
  switch (pass.m_type) {
    case RenderPassType::Forward:
      if (!dynamic.m_renderable_kabina.m_render_in_forward) return;
      break;
    default:
      if (!dynamic.m_renderable_kabina.m_render_in_deferred) return;
  }
  Render(dynamic.m_renderable_kabina, pass, history_origin, dynamic.m_distance);
}

void NvRenderer::Render(const Renderable& renderable, const RenderPass& pass,
                        const glm::dvec3& history_origin, double distance) {
  for (const auto& item : renderable.m_items) {
    if (item.m_render_in_forward != (pass.m_type == RenderPassType::Forward))
      continue;
    if (distance < item.m_sqr_distance_min ||
        distance >= item.m_sqr_distance_max)
      continue;
    const auto& [origin, extent] = GetGeometryBounds(item.m_geometry);
    if (!pass.BoxVisible(item.m_transform, origin, extent)) continue;

    nvrhi::GraphicsState gfx_state;
    nvrhi::DrawArguments draw_arguments{};
    bool indexed;
    float alpha_threshold;
    if (!BindGeometry(item.m_geometry, pass, gfx_state, draw_arguments,
                      indexed))
      return;
    if (!BindMaterial(item.m_material, DrawType::Model, pass, gfx_state,
                      alpha_threshold))
      return;

    BindConstants(pass, gfx_state);

    pass.m_command_list_draw->setGraphicsState(gfx_state);

    auto transform = item.m_transform;
    transform[3] -= pass.m_origin;

    auto history_transform = item.m_history_transform;
    history_transform[3] += history_origin;

    UpdateDrawData(pass, pass.m_transform * transform,
                   pass.m_history_transform * history_transform,
                   alpha_threshold, item.m_opacity, item.m_selfillum,
                   item.m_diffuse);

    if (indexed)
      pass.m_command_list_draw->drawIndexed(draw_arguments);
    else
      pass.m_command_list_draw->draw(draw_arguments);
    m_drawcall_counter->Draw(draw_arguments);
  }
}

void NvRenderer::Animate(const glm::dvec3& origin, double radius,
                         uint64_t frame) {
  {
    auto motion_scope = m_motion_cache->SetInstance(nullptr);
    auto& motion_cache = m_motion_cache->Get(nullptr);
    motion_cache.m_history_transform =
        std::exchange(motion_cache.m_cached_transform, glm::translate(-origin));
  }

  BatchingUtils::RadiusTester tester{origin, radius};

  m_renderable_dynamics.clear();
  m_dynamic_with_kabina = static_cast<uint32_t>(-1);

  for (uint32_t i = 0; i < m_dynamics.size(); ++i) {
    auto& dynamic = m_dynamics[i];
    dynamic.m_location = dynamic.m_dynamic->vPosition;
    dynamic.m_distance = glm::distance2(dynamic.m_location, origin);

    if (Global.pCamera.m_owner == dynamic.m_dynamic) m_dynamic_with_kabina = i;

    if (m_dynamic_with_kabina == i ||
        dynamic.m_radius * dynamic.m_radius / dynamic.m_distance >=
            .005 * .005) {
      m_renderable_dynamics.emplace_back(i);
      Animate(dynamic, origin, frame);
    }
  }

  m_cell_quadtree.ForEach(tester, [&](uint32_t cell_index) {
    const auto& cell = m_cells[cell_index];
    cell.m_cell->RaAnimate(frame);
  });

  m_animated_quadtree.ForEach(tester, [&](uint32_t animated_index) {
    auto& instance = m_animateds[animated_index];
    instance.m_distance = glm::distance2(instance.m_origin, origin);
    if (!instance.m_animatable && instance.m_was_once_animated) return;
    Animate(instance, origin, frame);
  });
}

void NvRenderer::GatherSpotLights(const RenderPass& pass) {
  m_renderable_spotlights.clear();
  auto traverse_spotlights = [&](const Renderable& renderable) {
    for (const auto& spotlight : renderable.m_spotlights) {
      if (pass.SphereVisible(spotlight.m_origin, spotlight.m_radius))
        m_renderable_spotlights.emplace_back(spotlight);
    }
  };
  for (const auto& dyn_id : m_renderable_dynamics) {
    const auto& dynamic = m_dynamics[dyn_id];
    traverse_spotlights(dynamic.m_renderable);
    if (dyn_id == m_dynamic_with_kabina)
      traverse_spotlights(dynamic.m_renderable_kabina);
  }
  m_animated_quadtree.ForEach(
      BatchingUtils::RadiusTester{pass.m_origin, pass.m_draw_range},
      [&](uint32_t animated_index) {
        auto& instance = m_animateds[animated_index];
        if (instance.m_renderable.m_spotlights.empty()) return;
        traverse_spotlights(instance.m_renderable);
      });
}
