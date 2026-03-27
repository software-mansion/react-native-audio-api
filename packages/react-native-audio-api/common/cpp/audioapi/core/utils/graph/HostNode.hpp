#pragma once

#include <audioapi/core/utils/graph/Graph.hpp>

#include <memory>
#include <utility>

namespace audioapi::utils::graph {

/// @brief RAII base class for host-side nodes.
///
/// Holds a `shared_ptr<Graph<NodeType>>` to keep the graph alive and owns a
/// `HostGraph::Node*` managed by that graph. On construction the node is
/// registered in the graph (and an event is sent to AudioGraph); on
/// destruction the node is removed (scheduling orphan-marking on AudioGraph).
///
/// Host objects that represent audio processing nodes should publicly inherit
/// from HostNode and pass their payload (the AudioNode-like object) to the
/// constructor. `connect` / `disconnect` provide edge management.
///
/// @note HostNode intentionally does NOT prevent cycles — callers must handle
/// the error returned by `connect()`.
///
/// ## Example usage:
/// ```cpp
/// class MyGainNode : public HostNode<AudioNode> {
///  public:
///   MyGainNode(std::shared_ptr<Graph<AudioNode>> g,
///              std::unique_ptr<AudioNode> impl)
///       : HostNode(std::move(g), std::move(impl)) {}
/// };
///
/// auto gain = std::make_unique<MyGainNode>(graph, std::move(gainImpl));
/// gain->connect(*destination);
/// gain.reset(); // destructor removes the node from the graph
/// ```
template <AudioGraphNode NodeType>
class HostNode {
 public:
  using GraphType = Graph<NodeType>;
  using HNode = HostGraph<NodeType>::Node;
  using ResultError = HostGraph<NodeType>::ResultError;
  using Res = Result<NoneType, ResultError>;

  /// @brief Constructs a HostNode, adding it to the graph.
  /// @param graph shared ownership of the Graph — prevents the graph from
  ///              being destroyed while any HostNode still references it
  /// @param audioNode the audio processing payload (ownership transferred
  ///                  through to AudioGraph via NodeHandle)
  explicit HostNode(std::shared_ptr<GraphType> graph, std::unique_ptr<NodeType> audioNode = nullptr)
      : graph_(std::move(graph)), node_(graph_->addNode(std::move(audioNode))) {}

  /// @brief Destructor removes the node from the graph.
  /// This marks the node as a ghost in HostGraph, and schedules an event
  /// that sets `orphaned = true` on the AudioGraph side.
  virtual ~HostNode() {
    if (graph_ && node_) {
      // Ignore the result — the node should always be found unless the
      // graph was already torn down.
      (void)graph_->removeNode(node_);
      node_ = nullptr;
    }
  }

  // Non-copyable (unique graph node identity)
  HostNode(const HostNode &) = delete;
  HostNode &operator=(const HostNode &) = delete;

  // Movable
  HostNode(HostNode &&other) noexcept : graph_(std::move(other.graph_)), node_(other.node_) {
    other.node_ = nullptr;
  }

  HostNode &operator=(HostNode &&other) noexcept {
    if (this != &other) {
      // Remove current node first
      if (graph_ && node_) {
        (void)graph_->removeNode(node_);
      }
      graph_ = std::move(other.graph_);
      node_ = other.node_;
      other.node_ = nullptr;
    }
    return *this;
  }

  /// @brief Connects this node's output to another node's input (this → other).
  /// @return Ok on success, Err on cycle / duplicate / not-found
  Res connect(HostNode &other) {
    return graph_->addEdge(node_, other.node_);
  }

  /// @brief Disconnects this node's output from another node's input.
  /// @return Ok on success, Err on not-found
  Res disconnect(HostNode &other) {
    return graph_->removeEdge(node_, other.node_);
  }

  /// @brief Returns the raw HostGraph::Node pointer (for advanced usage / testing).
  [[nodiscard]] HNode *rawNode() const {
    return node_;
  }

  /// @brief Returns the Graph this node belongs to.
  [[nodiscard]] const std::shared_ptr<GraphType> &graph() const {
    return graph_;
  }

 protected:
  std::shared_ptr<GraphType> graph_;
  HNode *node_ = nullptr;
};

} // namespace audioapi::utils::graph
