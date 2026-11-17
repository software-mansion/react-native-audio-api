#pragma once

#include <audioapi/core/utils/graph/GraphObject.h>

#include <cstdint>
#include <memory>

namespace audioapi::utils::graph {

/// @brief Shared handle bridging HostGraph and AudioGraph.
///
/// A single heap allocation stores both the payload node and the mutable
/// index into AudioGraph's flat vector. AudioGraph assigns the index in
/// addNode() and updates it during compaction and topological sort.
///
/// Ownership model (shared_ptr):
/// - Created on the main thread via std::make_shared.
/// - A shared_ptr is stored in both HostGraph::HostVertex and AudioGraph::Vertex.
/// - When a host node is removed, HostGraph marks it as a ghost and keeps
///   its shared_ptr. The AudioGraph event sets orphaned = true.
/// - When AudioGraph compacts out an orphaned node it releases its shared_ptr
///   (refcount 2 → 1). HostGraph detects use_count() == 1 and destroys the
///   ghost + payload on the main thread.
///
/// The position is private to AudioGraph: only the audio thread may read it,
/// through `AudioGraph::operator[](const NodeHandle &)` or `AudioGraph::indexOf`,
/// because every toposort and compaction rewrites it.
struct NodeHandle {
  std::unique_ptr<GraphObject> audioNode; // payload graph object (may be null in tests)

  explicit NodeHandle(std::unique_ptr<GraphObject> audioNode);

 private:
  friend class AudioGraph;
  std::uint32_t index_ = 0; // current position in AudioGraph::nodes
};

} // namespace audioapi::utils::graph
