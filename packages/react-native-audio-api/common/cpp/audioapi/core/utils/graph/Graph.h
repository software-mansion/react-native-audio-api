#pragma once

#include <audioapi/core/utils/Disposer.hpp>
#include <audioapi/core/utils/graph/AudioGraph.h>
#include <audioapi/core/utils/graph/HostGraph.h>
#include <audioapi/core/utils/graph/InputPool.h>

#include <audioapi/utils/FatFunction.hpp>
#include <audioapi/utils/Result.hpp>
#include <audioapi/utils/SpscChannel.hpp>

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
class Graph {
  using AGEvent = HostGraph::AGEvent;

  // ── Event channel (main → audio): grow + graph mutations ───────────────

  using EventReceiver = audioapi::channels::spsc::Receiver<
      AGEvent,
      audioapi::channels::spsc::OverflowStrategy::WAIT_ON_FULL,
      audioapi::channels::spsc::WaitStrategy::BUSY_LOOP>;
  using EventSender = audioapi::channels::spsc::Sender<
      AGEvent,
      audioapi::channels::spsc::OverflowStrategy::WAIT_ON_FULL,
      audioapi::channels::spsc::WaitStrategy::BUSY_LOOP>;

  using HNode = HostGraph::Node;

 public:
  using ResultError = HostGraph::ResultError;
  using Res = Result<NoneType, ResultError>;

  Graph(size_t eventQueueCapacity, Disposer<audioapi::DISPOSER_PAYLOAD_SIZE> *disposer);

  Graph(
      size_t eventQueueCapacity,
      Disposer<audioapi::DISPOSER_PAYLOAD_SIZE> *disposer,
      std::uint32_t initialNodeCapacity,
      std::uint32_t initialEdgeCapacity);

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
  void processEvents();

  /// @brief Runs toposort + compaction on the audio graph.
  /// Allocation-free.
  /// @note Should be called only from the audio thread.
  void process();

  /// @brief Returns an iterable view of nodes in topological order.
  ///
  /// Each entry contains a reference to GraphObject and an immutable view
  /// of its inputs (as references to GraphObject).
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
  HNode *addNode(std::unique_ptr<GraphObject> audioNode = nullptr);

  template <std::derived_from<GraphObject> TObject>
  HNode *addNode(std::unique_ptr<TObject> audioNode) {
    return addNode(std::unique_ptr<GraphObject>(std::move(audioNode)));
  }

  /// @brief Removes a node (marks as ghost). Pointer remains valid until
  /// the ghost is collected after AudioGraph releases its shared_ptr.
  Res removeNode(HNode *node);

  /// @brief Adds a directed edge from → to. Rejects cycles and duplicates.
  Res addEdge(HNode *from, HNode *to);

  /// @brief Removes a directed edge from → to.
  Res removeEdge(HNode *from, HNode *to);

  /// @brief Removes all outgoing edges from `from`.
  Res removeAllEdges(HNode *from);

  void collectDisposedNodes();

 private:
  using OwnedSlotBuffer = std::unique_ptr<InputPool::Slot[]>;
  using OwnedNodeBuffer = AudioGraph::NodeBuffer;

  // Aligning to cache line size to prevent false sharing between audio and main thread
  alignas(hardware_destructive_interference_size) AudioGraph audioGraph;
  alignas(hardware_destructive_interference_size) HostGraph hostGraph;

  // ── Channel (immutable after construction — no false sharing) ───────────

  EventSender eventSender_;
  EventReceiver eventReceiver_;

  // ── Disposer — destroys old pool buffers off the audio thread ───────────

  Disposer<audioapi::DISPOSER_PAYLOAD_SIZE> *disposer_;

  // ── Main-thread tracking for pre-growth ─────────────────────────────────

  std::uint32_t poolCapacity_ = 0; ///< Pool capacity we have ensured
  std::uint32_t nodeCapacity_ = 0; ///< Node vector capacity we have ensured

  /// @brief Pre-grows the InputPool when the edge count approaches capacity.
  ///
  /// Queries HostGraph::edgeCount() for the current truth. Allocates a new
  /// slot buffer on the main thread and sends it as an AGEvent through the
  /// event channel. The old buffer is sent to the Disposer for deallocation
  /// on a separate thread — never on the audio thread.
  void sendPoolGrowIfNeeded();

  /// @brief Pre-reserves the AudioGraph node vector when node count exceeds
  /// the last ensured capacity. Allocates a new node buffer on the main
  /// thread and sends it as an AGEvent through the event channel. The old
  /// buffer is sent to the Disposer for deallocation on a separate thread —
  /// never on the audio thread.
  void sendNodeGrowIfNeeded();

  // ── Bridge tracking (main thread only) ──────────────────────────────────

  friend class GraphTest;
};

} // namespace audioapi::utils::graph
