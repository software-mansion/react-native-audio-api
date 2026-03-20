#pragma once

#include <audioapi/core/utils/graph/GraphObject.hpp>

namespace audioapi {
class AudioParam;
}

namespace audioapi::utils::graph {

/// @brief Lightweight graph-only node that represents an AudioParam connection.
///
/// A BridgeNode sits between a source AudioNode and the owner AudioNode of a
/// param, forming the path: source → bridge → owner. This lets the graph
/// system detect cycles and compute correct topological ordering for param
/// connections without creating real ownership dependencies.
///
/// BridgeNodes are:
///   - **Not processable** — skipped by `AudioGraph::iter()`.
///   - **Always destructible** — removed by compaction when orphaned with no inputs.
///   - **Non-owning** — stores a raw `AudioParam*` whose lifetime is guaranteed
///     by the owner node.
class BridgeNode final : public GraphObject {
 public:
  explicit BridgeNode(AudioParam *param) : param_(param) {}

  [[nodiscard]] bool isProcessable() const override {
    return false;
  }

  [[nodiscard]] bool canBeDestructed() const override {
    return true;
  }

  /// @brief Returns the param this bridge represents a connection to.
  [[nodiscard]] AudioParam *param() const {
    return param_;
  }

 private:
  AudioParam *param_; // non-owning — lifetime guaranteed by owner node
};

} // namespace audioapi::utils::graph
