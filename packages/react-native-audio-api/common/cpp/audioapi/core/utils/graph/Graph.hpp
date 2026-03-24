#pragma once

#include <audioapi/core/utils/Disposer.hpp>
#include <audioapi/core/utils/graph/AudioGraph.hpp>
#include <audioapi/core/utils/graph/BridgeNode.hpp>
#include <audioapi/core/utils/graph/HostGraph.hpp>
#include <audioapi/core/utils/graph/InputPool.hpp>

#include <audioapi/utils/FatFunction.hpp>
#include <audioapi/utils/SpscChannel.hpp>

#include <audioapi/utils/Result.hpp>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>

namespace audioapi {
class AudioParam;
}

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
  static constexpr size_t kDisposerPayloadSize = HostGraph::kDisposerPayloadSize;
  using ResultError = HostGraph::ResultError;
  using Res = Result<NoneType, ResultError>;

  Graph(size_t eventQueueCapacity, Disposer<kDisposerPayloadSize> *disposer) : disposer_(disposer) {
    using namespace audioapi::channels::spsc;

    auto [es, er] = channel<AGEvent, OverflowStrategy::WAIT_ON_FULL, WaitStrategy::BUSY_LOOP>(
        eventQueueCapacity);
    eventSender_ = std::move(es);
    eventReceiver_ = std::move(er);
  }

  Graph(
      size_t eventQueueCapacity,
      Disposer<kDisposerPayloadSize> *disposer,
      std::uint32_t initialNodeCapacity,
      std::uint32_t initialEdgeCapacity)
      : Graph(eventQueueCapacity, disposer) {
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
        event(audioGraph, *disposer_);
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
  HNode *addNode(std::unique_ptr<GraphObject> audioNode = nullptr) {
    hostGraph.collectDisposedNodes();

    auto handle = std::make_shared<NodeHandle>(0, std::move(audioNode));
    auto [hostNode, event] = hostGraph.addNode(handle);

    sendNodeGrowIfNeeded();

    eventSender_.send(std::move(event));
    return hostNode;
  }

  template <std::derived_from<GraphObject> TObject>
  HNode *addNode(std::unique_ptr<TObject> audioNode) {
    return addNode(std::unique_ptr<GraphObject>(std::move(audioNode)));
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

  // ── Param bridge API ───────────────────────────────────────────────────

  /// @brief Creates a bridge node representing: source → bridge → owner.
  ///
  /// The bridge encodes a param connection in the graph for cycle detection
  /// and topological ordering. The bridge itself is not processable.
  ///
  /// @param source the node whose output feeds the param
  /// @param owner the node that owns the param
  /// @param param raw pointer to the AudioParam (lifetime guaranteed by owner)
  /// @return Ok on success, Err on cycle/duplicate/not-found
  Res connectParam(HNode *source, HNode *owner, AudioParam *param) {
    hostGraph.collectDisposedNodes();

    BridgeKey key{source, param};
    if (bridgeMap_.count(key)) {
      return Res::Err(ResultError::EDGE_ALREADY_EXISTS);
    }

    // Create bridge node
    auto bridgeObj = std::make_unique<BridgeNode>(param);
    auto bridgeHandle = std::make_shared<NodeHandle>(0, std::move(bridgeObj));
    auto [bridgeHostNode, addEvent] = hostGraph.addNode(bridgeHandle);

    // source → bridge
    auto edgeRes1 = hostGraph.addEdge(source, bridgeHostNode);
    if (edgeRes1.is_err()) {
      // Rollback: remove bridge node
      (void)hostGraph.removeNode(bridgeHostNode);
      return Res::Err(edgeRes1.unwrap_err());
    }

    // bridge → owner
    auto edgeRes2 = hostGraph.addEdge(bridgeHostNode, owner);
    if (edgeRes2.is_err()) {
      // Rollback: remove source→bridge edge and bridge node
      (void)hostGraph.removeEdge(source, bridgeHostNode);
      (void)hostGraph.removeNode(bridgeHostNode);
      return Res::Err(edgeRes2.unwrap_err());
    }

    // All succeeded — send events through SPSC
    sendNodeGrowIfNeeded();
    eventSender_.send(std::move(addEvent));

    sendPoolGrowIfNeeded();
    eventSender_.send(std::move(edgeRes1).unwrap());

    sendPoolGrowIfNeeded();
    eventSender_.send(std::move(edgeRes2).unwrap());

    // Track bridge
    bridgeMap_[key] = bridgeHostNode;
    bridgeOwners_[bridgeHostNode] = owner;

    return Res::Ok(NoneType{});
  }

  /// @brief Removes a bridge node for the given (source, param) pair.
  Res disconnectParam(HNode *source, HNode * /*owner*/, AudioParam *param) {
    hostGraph.collectDisposedNodes();

    BridgeKey key{source, param};
    auto it = bridgeMap_.find(key);
    if (it == bridgeMap_.end()) {
      return Res::Err(ResultError::EDGE_NOT_FOUND);
    }

    HNode *bridge = it->second;
    removeBridge(source, bridge);
    bridgeMap_.erase(it);

    return Res::Ok(NoneType{});
  }

  /// @brief Removes a node and cascade-removes any bridges where this node
  /// is the source or owner.
  Res removeNodeWithBridges(HNode *node) {
    hostGraph.collectDisposedNodes();

    // Cascade: remove bridges where this node is source
    for (auto it = bridgeMap_.begin(); it != bridgeMap_.end();) {
      if (it->first.source == node) {
        HNode *bridge = it->second;
        removeBridge(node, bridge);
        bridgeOwners_.erase(bridge);
        it = bridgeMap_.erase(it);
      } else {
        ++it;
      }
    }

    // Cascade: remove bridges where this node is owner
    for (auto it = bridgeMap_.begin(); it != bridgeMap_.end();) {
      auto ownerIt = bridgeOwners_.find(it->second);
      if (ownerIt != bridgeOwners_.end() && ownerIt->second == node) {
        HNode *bridge = it->second;
        HNode *source = it->first.source;
        removeBridge(source, bridge);
        bridgeOwners_.erase(ownerIt);
        it = bridgeMap_.erase(it);
      } else {
        ++it;
      }
    }

    return removeNode(node);
  }

 private:
  using OwnedSlotBuffer = std::unique_ptr<InputPool::Slot[]>;

  // Aligning to cache line size to prevent false sharing between audio and main thread
  alignas(hardware_destructive_interference_size) AudioGraph audioGraph;
  alignas(hardware_destructive_interference_size) HostGraph hostGraph;

  // ── Channel (immutable after construction — no false sharing) ───────────

  EventSender eventSender_;
  EventReceiver eventReceiver_;

  // ── Disposer — destroys old pool buffers off the audio thread ───────────

  Disposer<kDisposerPayloadSize> *disposer_;

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
      eventSender_.send([buf = std::move(buf), newCap](
                            AudioGraph &graph, Disposer<kDisposerPayloadSize> &disposer) mutable {
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
      eventSender_.send([newCap](AudioGraph &graph, auto &) { graph.reserveNodes(newCap); });
      nodeCapacity_ = newCap;
    }
  }

  // ── Bridge tracking (main thread only) ──────────────────────────────────

  struct BridgeKey {
    HNode *source;
    AudioParam *param;

    bool operator==(const BridgeKey &other) const {
      return source == other.source && param == other.param;
    }
  };

  struct BridgeKeyHash {
    size_t operator()(const BridgeKey &k) const {
      auto h1 = std::hash<HNode *>{}(k.source);
      auto h2 = std::hash<AudioParam *>{}(k.param);
      return h1 ^ (h2 << 1);
    }
  };

  /// Maps (source, param) → bridge host node
  std::unordered_map<BridgeKey, HNode *, BridgeKeyHash> bridgeMap_;

  /// Maps bridge host node → owner host node (for cascade removal)
  std::unordered_map<HNode *, HNode *> bridgeOwners_;

  /// @brief Removes a bridge node: tears down edges and marks for removal.
  void removeBridge(HNode *source, HNode *bridge) {
    // Find the owner from bridgeOwners_
    auto ownerIt = bridgeOwners_.find(bridge);
    HNode *owner = (ownerIt != bridgeOwners_.end()) ? ownerIt->second : nullptr;

    // Remove edges: source→bridge, bridge→owner
    auto res1 = hostGraph.removeEdge(source, bridge);
    if (res1.is_ok()) {
      eventSender_.send(std::move(res1).unwrap());
    }

    if (owner) {
      auto res2 = hostGraph.removeEdge(bridge, owner);
      if (res2.is_ok()) {
        eventSender_.send(std::move(res2).unwrap());
      }
    }

    // Remove bridge node
    auto res3 = hostGraph.removeNode(bridge);
    if (res3.is_ok()) {
      eventSender_.send(std::move(res3).unwrap());
    }
  }

  friend class GraphTest;
};

} // namespace audioapi::utils::graph
