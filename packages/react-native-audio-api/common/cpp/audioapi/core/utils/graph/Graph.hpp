#pragma once

#include <audioapi/core/utils/graph/AudioGraph.hpp>
#include <audioapi/core/utils/graph/HostGraph.hpp>

#include <audioapi/utils/FatFunction.hpp>
#include <audioapi/utils/SpscChannel.hpp>

#include <audioapi/utils/Result.hpp>

#include <memory>
#include <utility>

namespace audioapi::utils::graph {

template <AudioGraphNode NodeType>
class Graph {
  using AGEvent = typename HostGraph<NodeType>::AGEvent;

  using Receiver = audioapi::channels::spsc::Receiver<
      AGEvent,
      audioapi::channels::spsc::OverflowStrategy::WAIT_ON_FULL,
      audioapi::channels::spsc::WaitStrategy::BUSY_LOOP>;
  using Sender = audioapi::channels::spsc::Sender<
      AGEvent,
      audioapi::channels::spsc::OverflowStrategy::WAIT_ON_FULL,
      audioapi::channels::spsc::WaitStrategy::BUSY_LOOP>;

  using HNode = typename HostGraph<NodeType>::Node;

 public:
  using ResultError = typename HostGraph<NodeType>::ResultError;
  using Res = Result<NoneType, ResultError>;

  explicit Graph(size_t eventQueueCapacity) {
    auto [sender, receiver] = audioapi::channels::spsc::channel<
        AGEvent,
        audioapi::channels::spsc::OverflowStrategy::WAIT_ON_FULL,
        audioapi::channels::spsc::WaitStrategy::BUSY_LOOP>(eventQueueCapacity);
    eventSender = std::move(sender);
    eventReceiver = std::move(receiver);
  }
  ~Graph() = default;

  /// @brief Processes all scheduled events
  /// @note IMPORTANT: Should be called only from the audio thread
  void processEvents() {
    AGEvent event;
    while (eventReceiver.try_receive(event) == audioapi::channels::spsc::ResponseStatus::SUCCESS) {
      if (event) {
        event(audioGraph);
      }
    }
  }

  /// @brief Runs toposort + compaction on the audio graph.
  /// @note IMPORTANT: Should be called only from the audio thread.
  void process() {
    audioGraph.process();
  }

  /// @brief Returns an iterable view of nodes in topological order.
  ///
  /// Each entry contains a reference to the NodeType and an immutable view
  /// of its inputs (as references to NodeType).
  ///
  /// @note IMPORTANT: Should be called only from the audio thread, after process().
  [[nodiscard]] auto iter() {
    return audioGraph.iter();
  }

  /// @brief Adds a new node to the graph and returns a pointer to it.
  /// @param audioNode the audio processing node to add (ownership transferred)
  /// @return pointer to the newly added HostGraph::Node
  HNode *addNode(std::unique_ptr<NodeType> audioNode = nullptr) {
    auto handle = std::make_shared<NodeHandle<NodeType>>(0, std::move(audioNode));
    auto [hostNode, event] = hostGraph.addNode(handle);
    eventSender.send(std::move(event));
    return hostNode;
  }

  /// @brief Removes a node and all its edges from the graph. Does nothing if the node does not exist.
  /// @param node pointer to the node to be removed
  /// @return Result indicating success or failure (e.g., if node was not found)
  /// @note This marks the node as a ghost. The pointer remains valid until the
  /// ghost is collected (after AudioGraph releases its shared_ptr).
  Res removeNode(HNode *node) {
    return hostGraph.removeNode(node).map([&](AGEvent event) {
      eventSender.send(std::move(event));
      return NoneType{};
    });
  }

  /// @brief Adds an edge from `from` to `to` if it does not create a cycle.
  Res addEdge(HNode *from, HNode *to) {
    return hostGraph.addEdge(from, to).map([&](AGEvent event) {
      eventSender.send(std::move(event));
      return NoneType{};
    });
  }

  /// @brief Removes an edge from `from` to `to`. Does nothing if the edge does not exist.
  Res removeEdge(HNode *from, HNode *to) {
    return hostGraph.removeEdge(from, to).map([&](AGEvent event) {
      eventSender.send(std::move(event));
      return NoneType{};
    });
  }

 private:
  // Aligning to cache line size to prevent false sharing between audio and main thread
  alignas(64) AudioGraph<NodeType> audioGraph;
  alignas(64) HostGraph<NodeType> hostGraph;

  // These are const and their memory won't be modified after initialization, so no false sharing here
  Sender eventSender;
  Receiver eventReceiver;

  friend class GraphTest;
};

} // namespace audioapi::utils::graph
