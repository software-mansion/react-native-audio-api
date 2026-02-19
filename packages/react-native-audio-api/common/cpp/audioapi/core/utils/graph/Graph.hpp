#pragma once

#include <audioapi/core/utils/graph/AudioGraph.hpp>
#include <audioapi/core/utils/graph/HostGraph.h>

#include <audioapi/utils/FatFunction.hpp>
#include <audioapi/utils/SpscChannel.hpp>

#include <audioapi/utils/Result.hpp>

#include <memory>
#include <utility>

namespace audioapi::utils::graph {

class Graph {
  using Receiver = audioapi::channels::spsc::Receiver<
      HostGraph::AGEvent,
      audioapi::channels::spsc::OverflowStrategy::WAIT_ON_FULL,
      audioapi::channels::spsc::WaitStrategy::BUSY_LOOP>;
  using Sender = audioapi::channels::spsc::Sender<
      HostGraph::AGEvent,
      audioapi::channels::spsc::OverflowStrategy::WAIT_ON_FULL,
      audioapi::channels::spsc::WaitStrategy::BUSY_LOOP>;

  using HNode = HostGraph::Node;

 public:
  using ResultError = HostGraph::ResultError;
  using Res = Result<NoneType, ResultError>;

  explicit Graph(size_t eventQueueCapacity) {
    auto [sender, receiver] = audioapi::channels::spsc::channel<
        HostGraph::AGEvent,
        audioapi::channels::spsc::OverflowStrategy::WAIT_ON_FULL,
        audioapi::channels::spsc::WaitStrategy::BUSY_LOOP>(eventQueueCapacity);
    eventSender = std::move(sender);
    eventReceiver = std::move(receiver);
  }
  ~Graph() = default;

  /// @brief Processes all scheduled events
  /// @note IMPORTANT: Should be called only from the audio thread
  void processEvents() {
    HostGraph::AGEvent event;
    while (eventReceiver.try_receive(event) == audioapi::channels::spsc::ResponseStatus::SUCCESS) {
      if (event) {
        event(audioGraph);
      }
    }
  }

  /// @brief Adds a new node to the graph and returns a pointer to it.
  /// @param audioNode the audio processing node to add (ownership transferred)
  /// @return pointer to the newly added HostGraph::Node
  HNode *addNode(std::unique_ptr<AudioNode> audioNode = nullptr) {
    auto handle = std::make_shared<NodeHandle>(0, std::move(audioNode));
    auto [hostNode, event] = hostGraph.addNode(handle);
    eventSender.send(std::move(event));
    return hostNode;
  }

  /// @brief Removes a node and all its edges from the graph. Does nothing if the node does not exist.
  /// @param node pointer to the node to be removed
  /// @return Result indicating success or failure (e.g., if node was not found)
  /// @note This will also destroy this HostGraph::Node and dealocate its memory, so the pointer will become invalid after this call. Be careful with dangling pointers if you keep references to nodes outside of HostGraph.
  Res removeNode(HNode *node) {
    return hostGraph.removeNode(node).map([&](HostGraph::AGEvent event) {
      eventSender.send(std::move(event));
      return NoneType{};
    });
  }

  /// @brief Adds an edge from `from` to `to` if it does not create a cycle.
  /// @param from
  /// @param to
  /// @return Result indicating success or failure (e.g., if edge would create a cycle)
  Res addEdge(HNode *from, HNode *to) {
    return hostGraph.addEdge(from, to).map([&](HostGraph::AGEvent event) {
      eventSender.send(std::move(event));
      return NoneType{};
    });
  }

  /// @brief Removes an edge from `from` to `to`. Does nothing if the edge does not exist.
  /// @param from
  /// @param to
  /// @return Result indicating success or failure (e.g., if edge was not found)
  Res removeEdge(HNode *from, HNode *to) {
    return hostGraph.removeEdge(from, to).map([&](HostGraph::AGEvent event) {
      eventSender.send(std::move(event));
      return NoneType{};
    });
  }

 private:
  // Aligning to cache line size to prevent false sharing between audio and main thread
  alignas(64) AudioGraph audioGraph;
  alignas(64) HostGraph hostGraph;

  // These are const and their memory won't be modified after initialization, so no false sharing here
  Sender eventSender;
  Receiver eventReceiver;

  friend class GraphTest;
};

} // namespace audioapi::utils::graph
