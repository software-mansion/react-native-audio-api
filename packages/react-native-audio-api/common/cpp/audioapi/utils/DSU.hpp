#pragma once

#include <vector>

namespace audioapi::utils {

/// @brief Disjoint Set Union (DSU) or Union-Find data structure
/// @details this structure provides efficient find and union operations for managing disjoint sets
class DSU {
 public:
  explicit DSU(size_t n) : parent(n) {
    for (size_t i = 0; i < n; ++i) {
      parent[i] = i;
    }
  }

  size_t find(size_t a) {
    if (parent[a] != a) {
      parent[a] = find(parent[a]);
    }
    return parent[a];
  }

  void unite(size_t a, size_t b) {
    size_t rootA = find(a);
    size_t rootB = find(b);
    if (rootA != rootB) {
      parent[rootB] = rootA;
    }
  }

  void reset(size_t n) {
    parent.resize(n);
    for (size_t i = 0; i < n; ++i) {
      parent[i] = i;
    }
  }

  void add() {
    parent.push_back(parent.size());
  }

 private:
  std::vector<size_t> parent;
};

} // namespace audioapi::utils
