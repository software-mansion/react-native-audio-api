#pragma once

#include <audioapi/core/AudioNode.h>

#include <cstdint>
#include <memory>
#include <utility>

namespace audioapi::utils::graph {

/// @brief Shared handle bridging HostGraph and AudioGraph.
///
/// A single heap allocation stores both the AudioNode and the mutable index
/// into AudioGraph's flat vector. AudioGraph updates the index during
/// compaction and topological sort.
///
/// Ownership model (shared_ptr):
/// - Created on the main thread via std::make_shared.
/// - A shared_ptr is stored in both HostGraph::Node and AudioGraph::Node.
/// - When a host node is removed, its shared_ptr is moved into an event
///   lambda which marks the AudioGraph node as orphaned; the HostGraph
///   side no longer references it.
/// - When AudioGraph compacts out an orphaned node it disposes its
///   shared_ptr through the Disposer, so the actual destruction of the
///   AudioNode happens on the disposal thread (not the audio thread).
template <typename T>
struct NodeHandle {
  std::uint32_t index;          // current position in AudioGraph::nodes, updated during compaction
  std::unique_ptr<T> audioNode; // the actual audio processing node (may be null in tests)

  NodeHandle(std::uint32_t index, std::unique_ptr<T> audioNode)
      : index(index), audioNode(std::move(audioNode)) {}
};

} // namespace audioapi::utils::graph
