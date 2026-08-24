#pragma once

#include <audioapi/core/utils/Disposer.hpp>
#include <audioapi/core/utils/graph/AudioGraph.h>
#include <audioapi/core/utils/graph/HostGraph.h>
#include <audioapi/core/utils/graph/InputPool.h>

#include <audioapi/utils/Result.hpp>
#include <audioapi/utils/SpscChannel.hpp>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <utility>

namespace audioapi::utils::graph {

/// @brief Thread-safe graph coordinator that bridges HostGraph (main thread)
/// and AudioGraph (audio thread) via two SPSC event channels.
///
/// Two producer threads are supported:
///   - The JS thread produces structural mutations (`addNode`, `addEdge`,
///     `removeEdge`, `removeAllEdges`) and pre-grow events on **Channel A**.
///   - The JS runtime's finalizer / GC thread produces `removeNode`
///     (orphan) events on **Channel B**. This is the path taken when a
///     HostObject's destructor runs on the Hermes GC finalizer thread.
///
/// The audio thread drains Channel A fully before Channel B in every
/// `processEvents()` call. That preserves the invariant
/// `addNode(X) happens-before orphan(X)` on the audio side even though
/// the two channels carry no cross-ordering by themselves.
///
/// Memory pre-growth: the main thread tracks edge and node counts. When
/// growth is needed it sends an inline grow AGEvent immediately followed
/// by the graph-mutation AGEvent through Channel A, guaranteeing FIFO
/// ordering: the audio thread always applies growth before the operation
/// that needs it.
///
/// ## Audio-thread call order
/// ```
/// graph.processEvents();       // drain Channel A, then Channel B (FIFO within each)
/// graph.process();             // toposort + compaction + settle processable state
/// for (auto&& [node, inputs] : graph.iter()) { ... }
/// ```
class Graph {
  using AGEvent = HostGraph::AGEvent;

  /// Channel B payload: an orphan AGEvent plus a barrier — the value of
  /// Channel A's `sendCursor()` snapshotted *after* the host-side ghost
  /// flip but *before* the orphan is enqueued. The audio thread refuses to
  /// apply the orphan until Channel A's `rcvCursor()` has reached the
  /// barrier, guaranteeing every A event that existed at orphan-enqueue
  /// time is applied first.
  struct OrphanEnvelope {
    std::uint64_t barrier{0};
    AGEvent action;
  };

  constexpr static audioapi::channels::spsc::WaitStrategy EVENT_WAIT_STRATEGY =
      audioapi::channels::spsc::WaitStrategy::ATOMIC_WAIT;
  constexpr static audioapi::channels::spsc::OverflowStrategy EVENT_OVERFLOW_STRATEGY =
      audioapi::channels::spsc::OverflowStrategy::WAIT_ON_FULL;

  // ── Event channel (main → audio): grow + graph mutations ───────────────

  using EventReceiver =
      audioapi::channels::spsc::Receiver<AGEvent, EVENT_OVERFLOW_STRATEGY, EVENT_WAIT_STRATEGY>;
  using EventSender =
      audioapi::channels::spsc::Sender<AGEvent, EVENT_OVERFLOW_STRATEGY, EVENT_WAIT_STRATEGY>;

  using GcEventReceiver = audioapi::channels::spsc::
      Receiver<OrphanEnvelope, EVENT_OVERFLOW_STRATEGY, EVENT_WAIT_STRATEGY>;
  using GcEventSender = audioapi::channels::spsc::
      Sender<OrphanEnvelope, EVENT_OVERFLOW_STRATEGY, EVENT_WAIT_STRATEGY>;

  using HNode = HostGraph::Node;

 public:
  using ResultError = HostGraph::ResultError;
  using Res = Result<NoneType, ResultError>;

  Graph(
      size_t eventQueueCapacity,
      Disposer<audioapi::DISPOSER_PAYLOAD_SIZE> *disposer,
      std::uint32_t initialNodeCapacity = AUDIO_GRAPH_INITIAL_CAPACITY,
      std::uint32_t initialEdgeCapacity = AUDIO_GRAPH_INITIAL_CAPACITY);

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

  /// @brief Runs toposort + compaction on the audio graph, then settles every
  /// node's processable state for the coming quantum (reverse-topo pull).
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
  HNode *addNode(std::unique_ptr<GraphObject> audioNode);

  template <std::derived_from<GraphObject> TObject>
  HNode *addNode(std::unique_ptr<TObject> audioNode) {
    return addNode(std::unique_ptr<GraphObject>(std::move(audioNode)));
  }

  /// @brief Removes a node (marks as ghost). Pointer remains valid until
  /// the ghost is collected after AudioGraph releases its shared_ptr.
  Res removeNode(HNode *node);

  /// @brief Adds a directed edge from → to. Rejects cycles and duplicates.
  Res addEdge(HNode *from, HNode *to);

  /// @brief Links two nodes so that `to` follows the processable state of
  /// `from` (one-way), for nodes that share processing semantics but not an
  /// audio edge (e.g. DelayReader → DelayWriter, which communicate through a
  /// ring buffer). Records the link on both the host graph (for cleanup) and
  /// the audio graph (a `link_head` entry consumed by
  /// AudioGraph::settleProcessableState()). Idempotent.
  void linkNodes(HNode *from, HNode *to);

  /// @brief Removes a directed edge from → to.
  Res removeEdge(HNode *from, HNode *to);

  /// @brief Removes all outgoing edges from `from`.
  Res removeAllEdges(HNode *from);

  /// @brief Recomputes channel-count negotiation for `node` (cascading
  /// downstream) after its `channelCount` / `channelCountMode` changed. Sends
  /// the resulting buffer-swap event through Channel A.
  Res renegotiateNodeChannels(HNode *node);

  void collectDisposedNodes();

  /// @brief Enters self-drain mode: producing (JS / GC finalizer) threads
  /// drain both event channels themselves right after enqueuing, then flushes
  /// any backlog already in the channels.
  ///
  /// The event channels are bounded SPSC queues with `WAIT_ON_FULL` +
  /// `ATOMIC_WAIT` senders: once full, `send()` blocks until a consumer
  /// advances the receive cursor.
  ///
  /// Enable self-drain only while there is **no** audio/render consumer:
  ///   - `OfflineAudioContext`: before `startRendering()`, and again after a
  ///     scheduled suspend until `resume()` restarts the render thread.
  ///   - Realtime `AudioContext`: while SUSPENDED / stopped (construction,
  ///     after `suspend()` / `close()` quiescence). With no callback draining
  ///     the channels, a large graph would otherwise fill them and block.
  ///
  /// While enabled, drains from the JS thread and the GC finalizer thread are
  /// serialized by `selfDrainMutex_`, so exactly one thread consumes at a time.
  void enableProducerSelfDrain() {
    std::scoped_lock lock(selfDrainMutex_);
    producerSelfDrain_.store(true, std::memory_order_release);
    processEvents();
  }

  /// @brief Leaves self-drain mode so the audio/render thread can become the
  /// single consumer. Flushes both channels first (while still serialized with
  /// any in-flight self-drain), so the consumer starts on empty channels.
  ///
  /// Call *before* starting the audio/render thread; once it runs, producers
  /// must not touch the receivers. If start/resume fails, re-enable.
  void disableProducerSelfDrain() {
    std::scoped_lock lock(selfDrainMutex_);
    processEvents();
    producerSelfDrain_.store(false, std::memory_order_release);
  }

 private:
  using OwnedSlotBuffer = std::unique_ptr<InputPool::Slot[]>;
  using OwnedNodeBuffer = AudioGraph::NodeBuffer;

  // Aligning to cache line size to prevent false sharing between audio and main thread
  alignas(hardware_destructive_interference_size) AudioGraph audioGraph;
  alignas(hardware_destructive_interference_size) HostGraph hostGraph;

  // ── Channels (immutable after construction — no false sharing) ─────────
  //
  // Channel A: JS thread producer — addNode / addEdge / removeEdge /
  //            removeAllEdges / grow events.
  // Channel B: finalizer (GC) thread producer — removeNode (orphan) events.
  //
  // Each channel has a single producer, so SPSC invariants hold.

  EventSender eventSender_;
  EventReceiver eventReceiver_;

  GcEventSender gcEventSender_;
  GcEventReceiver gcEventReceiver_;

  // ── Disposer — destroys old pool buffers off the audio thread ───────────

  Disposer<audioapi::DISPOSER_PAYLOAD_SIZE> *disposer_;

  // ── Main-thread tracking for pre-growth ─────────────────────────────────

  std::uint32_t poolCapacity_; ///< Pool capacity we have ensured
  std::uint32_t nodeCapacity_; ///< Node vector capacity we have ensured

  /// @brief When set, producer threads drain the channels right after each
  /// enqueue (see enableProducerSelfDrain). Default off — realtime contexts
  /// rely on the audio thread as the consumer.
  std::atomic<bool> producerSelfDrain_{false};

  /// @brief Serializes self-drain consumption between producer threads (JS
  /// thread mutations vs GC-finalizer `removeNode`) and the mode toggles.
  std::mutex selfDrainMutex_;

  /// @brief Drains any pending produced events on the calling (producer)
  /// thread when self-drain is enabled. No-op otherwise.
  ///
  /// The flag is re-checked under `selfDrainMutex_`: a concurrent
  /// `disableProducerSelfDrain()` may have handed consumption to the
  /// audio/render thread between the fast-path check and the lock, and
  /// consuming past that point would race the new consumer.
  void drainProducedEventsIfSelfDraining() {
    if (!producerSelfDrain_.load(std::memory_order_acquire)) {
      return;
    }
    std::scoped_lock lock(selfDrainMutex_);
    if (!producerSelfDrain_.load(std::memory_order_acquire)) {
      return;
    }
    processEvents();
  }

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
