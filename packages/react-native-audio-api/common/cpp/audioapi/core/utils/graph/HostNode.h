#pragma once

#include <audioapi/core/utils/graph/Graph.h>
#include <audioapi/core/utils/graph/GraphObject.h>
#include <audioapi/utils/Result.hpp>

#include <concepts>
#include <memory>
#include <utility>

namespace audioapi::utils::graph {

/// @brief RAII base class for host-side nodes.
///
/// Holds a `shared_ptr<Graph>` to keep the graph alive and owns a
/// `HostGraph::Node*` managed by that graph. On construction the node is
/// registered in the graph (and an event is sent to AudioGraph); on
/// destruction the node is removed (scheduling orphan-marking on AudioGraph).
///
/// Host objects that represent audio processing nodes should publicly inherit
/// from HostNode and pass their payload (GraphObject-derived object) to the
/// constructor. `connect` / `disconnect` provide edge management.
///
/// @note HostNode intentionally does NOT prevent cycles — callers must handle
/// the error returned by `connect()`.
///
/// ## Example usage:
/// ```cpp
/// class MyGainNode : public HostNode {
///  public:
///   MyGainNode(std::shared_ptr<Graph> g,
///              std::unique_ptr<AudioNode> impl)
///       : HostNode(std::move(g), std::move(impl)) {}
/// };
///
/// auto gain = std::make_unique<MyGainNode>(graph, std::move(gainImpl));
/// gain->connect(*destination);
/// gain.reset(); // destructor removes the node from the graph
/// ```
class HostNode {
 public:
  using GraphType = Graph;
  using HNode = HostGraph::Node;
  using ResultError = HostGraph::ResultError;
  using Res = Result<NoneType, ResultError>;

  /// @brief Constructs a HostNode, adding it to the graph.
  /// @param graph shared ownership of the Graph — prevents the graph from
  ///              being destroyed while any HostNode still references it
  /// @param graphObject the payload (ownership transferred through to
  ///                    AudioGraph via NodeHandle)
  explicit HostNode(
      std::shared_ptr<GraphType> graph,
      std::unique_ptr<GraphObject> graphObject = nullptr);

  template <typename TObject>
    requires std::derived_from<TObject, GraphObject>
  explicit HostNode(std::shared_ptr<GraphType> graph, std::unique_ptr<TObject> graphObject)
      : HostNode(std::move(graph), std::unique_ptr<GraphObject>(std::move(graphObject))) {}

  /// @brief Destructor removes the node from the graph.
  /// This marks the node as a ghost in HostGraph, and schedules an event
  /// that sets `orphaned = true` on the AudioGraph side.
  virtual ~HostNode();

  // Non-copyable (unique graph node identity)
  HostNode(const HostNode &) = delete;
  HostNode &operator=(const HostNode &) = delete;

  // Movable
  HostNode(HostNode &&other) noexcept;
  HostNode &operator=(HostNode &&other) noexcept;

  /// @brief Connects this node's output to another node's input (this → other).
  /// @return Ok on success, Err on cycle / duplicate / not-found
  Res connect(HostNode &other);

  /// @brief Disconnects this node's output from another node's input.
  /// @return Ok on success, Err on not-found
  Res disconnect(HostNode &other);

  /// @brief Disconnects all this node's outputs.
  /// @return Ok on success, Err on not-found
  Res disconnect();

  /// @brief Returns the raw HostGraph::Node pointer (for advanced usage / testing).
  [[nodiscard]] HNode *rawNode() const;

  /// @brief Returns the Graph this node belongs to.
  [[nodiscard]] const std::shared_ptr<GraphType> &graph() const;

 protected:
  std::shared_ptr<GraphType> graph_;
  HNode *node_ = nullptr;
};

} // namespace audioapi::utils::graph
