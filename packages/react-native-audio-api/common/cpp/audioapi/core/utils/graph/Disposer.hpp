#pragma once

#include <audioapi/core/utils/graph/AudioGraph.h>

namespace audioapi::utils::graph {

class Disposer {
 public:
  virtual ~Disposer() = default;

  /// @brief Disposes the given audio node.
  /// @param node Pointer to the AudioGraph::Node to be disposed.
  virtual void dispose(AudioGraph::Node *node) = 0;
};

} // namespace audioapi::utils::graph
