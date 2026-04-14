#pragma once

#include <audioapi/core/utils/graph/AudioGraph.hpp>
#include <audioapi/core/utils/graph/Disposer.hpp>
#include <audioapi/core/utils/graph/HostGraph.hpp>
#include <audioapi/core/utils/graph/InputPool.hpp>

#include <audioapi/utils/FatFunction.hpp>
#include <audioapi/utils/SpscChannel.hpp>

#include <audioapi/utils/Result.hpp>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>

namespace audioapi::utils::graph {

/// @brief Thread-safe graph coordinator that bridges HostGraph (main thread)
/// and AudioGraph (audio thread) via a single SPSC event channel.
///
/// Memory pre-growth: the main thread tracks edge and node counts. When
/// growth is needed it sends an inline grow AGEvent immediately followed
/// by the graph-mutation AGEvent through the **same** channel, guaranteeing
/// FIFO ordering: the audio thread always applies growth before the
/// operation that needs it.
///
/// ## Audio-thread call order
/// ```
/// graph.processEvents();       // apply pending graph mutations (if any) — in FIFO order
/// graph.process();             // toposort + compaction
/// for (auto&& [node, inputs] : graph.iter()) { ... }
/// ```
template <AudioGraphNode NodeType>
class Graph {
  using AGEvent = HostGraph<NodeType>::AGEvent;

  // ── Event channel (main → audio): grow + graph mutations ───────────────

  using EventReceiver = audioapi::channels::spsc::Receiver<
      AGEvent,
      audioapi::channels::spsc::OverflowStrategy::WAIT_ON_FULL,
      audioapi::channels::spsc::WaitStrategy::BUSY_LOOP>;
  using EventSender = audioapi::channels::spsc::Sender<
      AGEvent,
      audioapi::channels::spsc::OverflowStrategy::WAIT_ON_FULL,
      audioapi::channels::spsc::WaitStrategy::BUSY_LOOP>;

  using HNode = HostGraph<NodeType>::Node;

 public:
  using ResultError = HostGraph<NodeType>::ResultError;
  using Res = Result<NoneType, ResultError>;

  explicit Graph(size_t eventQueueCapacity) {
    using namespace audioapi::channels::spsc;

    auto [es, er] = channel<AGEvent, OverflowStrategy::WAIT_ON_FULL, WaitStrategy::BUSY_LOOP>(
        eventQueueCapacity);
    eventSender_ = std::move(es);
    eventReceiver_ = std::move(er);
  }

  Graph(
      size_t eventQueueCapacity,
      std::uint32_t initialNodeCapacity,
      std::uint32_t initialEdgeCapacity)
      : Graph(eventQueueCapacity) {
    if (initialNodeCapacity > 0) {
      audioGraph.reserveNodes(initialNodeCapacity);
      nodeCapacity_ = initialNodeCapacity;
    }
    if (initialEdgeCapacity > 0) {
      audioGraph.pool().grow(initialEdgeCapacity);
      poolCapacity_ = initialEdgeCapacity;
    }
  }

  // ── Audio-thread API ────────────────────────────────────────────────────

  /// @brief Processes all scheduled events (grow + graph-mutation).
  ///
  /// Grow events (pool buffer adoption, node vector reserve) may allocate,
  /// so call this **before** entering the allocation-free zone.
  /// Graph-mutation events (addNode, orphan, push, remove, markDirty) are
  /// allocation-free because their capacity was ensured by a preceding
  /// grow event in the same FIFO.
  ///
  /// @note Should be called only from the audio thread.
  void processEvents() {
    AGEvent event;
    while (eventReceiver_.try_receive(event) == audioapi::channels::spsc::ResponseStatus::SUCCESS) {
      if (event) {
        event(audioGraph, disposer_);
      }
    }
  }

  /// @brief Runs toposort + compaction on the audio graph.
  /// Allocation-free.
  /// @note Should be called only from the audio thread.
  void process() {
    audioGraph.process();
  }

  /// @brief Returns an iterable view of nodes in topological order.
  ///
  /// Each entry contains a reference to the NodeType and an immutable view
  /// of its inputs (as references to NodeType).
  /// Allocation-free.
  ///
  /// @note Should be called only from the audio thread, after process().
  [[nodiscard]] auto iter() {
    return audioGraph.iter();
  }

  // ── Main-thread API ────────────────────────────────────────────────────

  /// @brief Adds a new node to the graph and returns a pointer to it.
  /// @param audioNode the audio processing node to add (ownership transferred)
  /// @return pointer to the newly added HostGraph::Node
  HNode *addNode(std::unique_ptr<NodeType> audioNode = nullptr) {
    hostGraph.collectDisposedNodes();

    auto handle = std::make_shared<NodeHandle<NodeType>>(0, std::move(audioNode));
    auto [hostNode, event] = hostGraph.addNode(handle);

    sendNodeGrowIfNeeded();

    eventSender_.send(std::move(event));
    return hostNode;
  }

  /// @brief Removes a node (marks as ghost). Pointer remains valid until
  /// the ghost is collected after AudioGraph releases its shared_ptr.
  Res removeNode(HNode *node) {
    hostGraph.collectDisposedNodes();
    return hostGraph.removeNode(node).map([&](AGEvent event) {
      eventSender_.send(std::move(event));
      return NoneType{};
    });
  }

  /// @brief Adds a directed edge from → to. Rejects cycles and duplicates.
  Res addEdge(HNode *from, HNode *to) {
    hostGraph.collectDisposedNodes();
    return hostGraph.addEdge(from, to).map([&](AGEvent event) {
      sendPoolGrowIfNeeded();
      eventSender_.send(std::move(event));
      return NoneType{};
    });
  }

  /// @brief Removes a directed edge from → to.
  Res removeEdge(HNode *from, HNode *to) {
    hostGraph.collectDisposedNodes();
    return hostGraph.removeEdge(from, to).map([&](AGEvent event) {
      eventSender_.send(std::move(event));
      return NoneType{};
    });
  }

 private:
  static constexpr size_t kDisposerPayloadSize = HostGraph<NodeType>::kDisposerPayloadSize;

  using OwnedSlotBuffer = std::unique_ptr<InputPool::Slot[]>;

  // Aligning to cache line size to prevent false sharing between audio and main thread
  alignas(hardware_destructive_interference_size) AudioGraph<NodeType> audioGraph;
  alignas(hardware_destructive_interference_size) HostGraph<NodeType> hostGraph;

  // ── Channel (immutable after construction — no false sharing) ───────────

  EventSender eventSender_;
  EventReceiver eventReceiver_;

  // ── Disposer — destroys old pool buffers off the audio thread ───────────

  DisposerImpl<kDisposerPayloadSize> disposer_{64};

  // ── Main-thread tracking for pre-growth ─────────────────────────────────

  std::uint32_t poolCapacity_ = 0; ///< Pool capacity we have ensured
  std::uint32_t nodeCapacity_ = 0; ///< Node vector capacity we have ensured

  /// @brief Pre-grows the InputPool when the edge count approaches capacity.
  ///
  /// Queries HostGraph::edgeCount() for the current truth. Allocates a new
  /// slot buffer on the main thread and sends it as an AGEvent through the
  /// event channel. The old buffer is sent to the Disposer for deallocation
  /// on a separate thread — never on the audio thread.
  void sendPoolGrowIfNeeded() {
    auto edges = static_cast<std::uint32_t>(hostGraph.edgeCount());
    // edges > poolCapacity_ / 2 || (poolCapacity_ == 0 && edges > 0) left for clarity
    if (edges > poolCapacity_ / 2) {
      std::uint32_t newCap = std::max(static_cast<std::uint32_t>(edges * 2), std::uint32_t{64});
      auto buf = std::make_unique<InputPool::Slot[]>(newCap);
      eventSender_.send(
          [buf = std::move(buf), newCap](
              AudioGraph<NodeType> &graph, Disposer<kDisposerPayloadSize> &disposer) mutable {
            auto *old = graph.pool().adoptBuffer(buf.release(), newCap);
            if (old) {
              disposer.dispose(OwnedSlotBuffer(old));
            }
          });
      poolCapacity_ = newCap;
    }
  }

  /// @brief Pre-reserves the AudioGraph node vector when node count exceeds
  /// the last ensured capacity. Queries HostGraph::nodeCount() for the
  /// current truth. Sends a grow event through the event channel.
  void sendNodeGrowIfNeeded() {
    auto nodes = static_cast<std::uint32_t>(hostGraph.nodeCount());
    if (nodes > nodeCapacity_) {
      std::uint32_t newCap = std::max(static_cast<std::uint32_t>(nodes * 2), std::uint32_t{64});
      eventSender_.send(
          [newCap](AudioGraph<NodeType> &graph, auto &) { graph.reserveNodes(newCap); });
      nodeCapacity_ = newCap;
    }
  }

  friend class GraphTest;
};

} // namespace audioapi::utils::graph
