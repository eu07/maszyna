#pragma once

#include <array>
#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <memory>
#include <vector>

struct QuadTreeLeave {
  QuadTreeLeave(const glm::dvec3& origin, const glm::dvec3& extent,
                uint32_t index)
      : m_origin(origin), m_extent(extent), m_index(index) {}
  glm::dvec3 m_origin;
  glm::dvec3 m_extent;
  uint32_t m_index;
};

struct QuadTreeBuilder {
  struct Node {
    std::array<std::shared_ptr<Node>, 4> m_children;
    std::vector<QuadTreeLeave> m_leaves;
    glm::dvec2 m_min;
    glm::dvec2 m_max;
    glm::dvec3 m_actual_min;
    glm::dvec3 m_actual_max;
    [[nodiscard]] int GetChildIndex(glm::dvec3 origin) const {
      glm::dvec2 mid = .5 * (m_min + m_max);
      return (origin.x < mid.x ? 0 : 1) | (origin.z < mid.y ? 0 : 2);
    }
    [[nodiscard]] glm::dvec4 GetChildBounds(int index) const {
      glm::dvec2 mid = .5 * (m_min + m_max);
      return glm::dvec4{
          index & 1 ? mid.x : m_min.x, index & 2 ? mid.y : m_min.y,
          index & 1 ? m_max.x : mid.x, index & 2 ? m_max.y : mid.y};
    }
    Node(const double min_x, const double min_z, const double max_x,
         const double max_z)
        : m_min(min_x, min_z),
          m_max(max_x, max_z),
          m_actual_min(std::numeric_limits<double>::max()),
          m_actual_max(-std::numeric_limits<double>::max()) {}
    explicit Node(const glm::dvec4& minmax_xz)
        : m_min(minmax_xz.x, minmax_xz.y),
          m_max(minmax_xz.z, minmax_xz.w),
          m_actual_min(std::numeric_limits<double>::max()),
          m_actual_max(-std::numeric_limits<double>::max()) {}
    [[nodiscard]] int NumChildren() const {
      int result = 0;
      for (const auto& child : m_children) {
        if (child) ++result;
      }
      return result;
    }
    [[nodiscard]] const Node* FirstChild() const {
      for (const auto& child : m_children) {
        if (child) return child.get();
      }
      return nullptr;
    }
    void Insert(uint32_t index, glm::dvec3 min_, glm::dvec3 max_, int level);
  };
  std::shared_ptr<Node> m_root;
  glm::dvec3 m_min;
  glm::dvec3 m_max;
  int m_max_levels;
  QuadTreeBuilder(double min_x, double min_z, double max_x, double max_z);
  void Insert(uint32_t index, glm::dvec3 min, glm::dvec3 max) const;
};

struct QuadTree {
  struct Node {
    glm::dvec3 m_origin;
    glm::dvec3 m_extent;
    uint32_t m_first_child;
    uint32_t m_child_count;
    uint32_t m_first_leave;
    uint32_t m_leave_count;
  };
  std::vector<Node> m_nodes;
  std::vector<QuadTreeLeave> m_leaves;
  void Build(const QuadTreeBuilder& builder);
  void Add(Node& node, const QuadTreeBuilder::Node& item);
  template <typename Tester, typename CallbackFn>
  void ForEach(const Tester& tester, CallbackFn fn, size_t subtree_index = 0) {
    const auto& subtree = m_nodes[subtree_index];
    if (!tester.BoxVisible(subtree.m_origin, subtree.m_extent)) return;
    for (int i = 0; i < subtree.m_leave_count; ++i) {
      const auto& leave = m_leaves[subtree.m_first_leave + i];
      if (!tester.BoxVisible(leave.m_origin, leave.m_extent)) continue;
      fn(leave.m_index);
    }
    for (int i = 0; i < subtree.m_child_count; ++i) {
      ForEach(tester, fn, subtree.m_first_child + i);
    }
  }
};