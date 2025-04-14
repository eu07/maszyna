#include "nvrenderer/quadtree.h"

void QuadTreeBuilder::Node::Insert(uint32_t index, glm::dvec3 min_,
                                   glm::dvec3 max_, int level) {
  if (level > 0) {
    int child = GetChildIndex(.5 * (min_ + max_));
    if (!m_children[child]) {
      m_children[child] = std::make_shared<Node>(GetChildBounds(child));
    }
    m_children[child]->Insert(index, min_, max_, level - 1);
  } else {
    m_leaves.emplace_back(.5 * (max_ + min_), .5 * (max_ - min_), index);
  }

#ifndef NDEBUG
  if (compMin(m_actual_min) < -1.e10) {
    __debugbreak();
  }
  if (compMax(m_actual_max) > 1.e10) {
    __debugbreak();
  }
#endif

  m_actual_min = min(min_, m_actual_min);
  m_actual_max = max(max_, m_actual_max);

#ifndef NDEBUG
  if (compMax(glm::abs(m_actual_min)) > 1.e10) {
    __debugbreak();
  }
  if (compMax(glm::abs(m_actual_max)) > 1.e10) {
    __debugbreak();
  }
#endif
}

QuadTreeBuilder::QuadTreeBuilder(double min_x, double min_z, double max_x,
                                 double max_z)
    : m_root(std::make_shared<Node>(min_x, min_z, max_x, max_z)),
      m_min(min_x, -std::numeric_limits<double>::max(), min_z),
      m_max(max_x, std::numeric_limits<double>::max(), max_z),
      m_max_levels(32) {
  // Ensure outer bounds are square
  glm::dvec2 origin = .5 * (m_max + m_min).xz;
  glm::dvec2 extent = .5 * (m_max - m_min).xz;
  extent = glm::dvec2(glm::compMax(extent));
  m_min.xz = origin - extent;
  m_max.xz = origin + extent;
  // glm::dvec3 scale_v = (m_max - m_min) / 50.;
  // double scale = glm::min(scale_v.z, scale_v.x);
  // m_max_levels = glm::max(1, (int)glm::ceil(glm::log2(scale)));
}

void QuadTreeBuilder::Insert(uint32_t index, glm::dvec3 min,
                             glm::dvec3 max) const {
  glm::dvec3 scale_v = (m_max - m_min) / (max - min);
  double scale = glm::min(scale_v.z, scale_v.x);
  int level =
      glm::clamp((int)glm::floor(glm::log2(scale)), 0, m_max_levels - 1);
  m_root->Insert(index, min, max, level);
}

void QuadTree::Build(const QuadTreeBuilder& builder) {
  Add(m_nodes.emplace_back(), *builder.m_root);
}

void QuadTree::Add(Node& node, const QuadTreeBuilder::Node& item) {
  if (item.m_leaves.empty() && item.NumChildren() == 1) {
    return Add(node, *item.FirstChild());
  }
  node.m_origin = .5 * (item.m_actual_max + item.m_actual_min);
  node.m_extent = .5 * (item.m_actual_max - item.m_actual_min);
  node.m_first_leave = m_leaves.size();
  node.m_leave_count = item.m_leaves.size();
  m_leaves.insert(m_leaves.end(), item.m_leaves.begin(), item.m_leaves.end());
  uint32_t first_child = m_nodes.size();
  uint32_t child_count = 0;
  for (const auto& child : item.m_children) {
    if (child) ++child_count;
  }
  node.m_first_child = first_child;
  node.m_child_count = child_count;
  // node reference invalidated from here
  for (int i = 0; i < child_count; ++i) {
    m_nodes.emplace_back();
  }
  {
    int child_index = first_child;
    for (const auto& child : item.m_children) {
      if (!child) continue;
      Add(m_nodes[child_index++], *child);
    }
  }
}
