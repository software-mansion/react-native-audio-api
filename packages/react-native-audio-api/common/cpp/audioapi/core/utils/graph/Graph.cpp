// Graph coordinator implementation (formerly inline in Graph.hpp).

#include <audioapi/core/utils/graph/Graph.h>
#include <audioapi/core/utils/graph/NodeHandle.h>

#include <algorithm>
#include <memory>
#include <utility>

namespace audioapi::utils::graph {

Graph::Graph(size_t eventQueueCapacity, Disposer<audioapi::DISPOSER_PAYLOAD_SIZE> *disposer)
    : disposer_(disposer) {
  using namespace audioapi::channels::spsc;

  auto [es, er] =
      channel<AGEvent, OverflowStrategy::WAIT_ON_FULL, WaitStrategy::BUSY_LOOP>(eventQueueCapacity);
  eventSender_ = std::move(es);
  eventReceiver_ = std::move(er);

  auto [gs, gr] =
      channel<AGEvent, OverflowStrategy::WAIT_ON_FULL, WaitStrategy::BUSY_LOOP>(eventQueueCapacity);
  gcEventSender_ = std::move(gs);
  gcEventReceiver_ = std::move(gr);
}

Graph::Graph(
    size_t eventQueueCapacity,
    Disposer<audioapi::DISPOSER_PAYLOAD_SIZE> *disposer,
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

void Graph::processEvents() {
  AGEvent event;
  // Drain Channel A (JS thread producer: addNode / addEdge / grow / …)
  // fully first — this guarantees that any `addNode(X)` pending here is
  // applied to AudioGraph before we process a matching `orphan(X)` that
  // may already be sitting in Channel B.
  while (eventReceiver_.try_receive(event) == audioapi::channels::spsc::ResponseStatus::SUCCESS) {
    if (event) {
      event(audioGraph, *disposer_);
    }
  }
  // Drain Channel B (finalizer / GC thread producer: removeNode orphan
  // events). These are idempotent w.r.t. each other and never require
  // capacity growth (they only flip a boolean on an existing node).
  while (gcEventReceiver_.try_receive(event) == audioapi::channels::spsc::ResponseStatus::SUCCESS) {
    if (event) {
      event(audioGraph, *disposer_);
    }
  }
}

void Graph::process() {
  audioGraph.process();
}

Graph::HNode *Graph::addNode(std::unique_ptr<GraphObject> audioNode) {
  // collectDisposedNodes();

  auto handle = std::make_shared<NodeHandle>(0, std::move(audioNode));
  auto [hostNode, event] = hostGraph.addNode(handle);

  sendNodeGrowIfNeeded();

  eventSender_.send(std::move(event));
  return hostNode;
}

Graph::Res Graph::removeNode(HNode *node) {
  // collectDisposedNodes();
  // Routed through Channel B: HostNode destructors (and therefore this
  // call) may fire on the JS runtime's finalizer thread (e.g. Hermes GC).
  // Sending through the dedicated SPSC channel keeps the single-producer
  // invariant for both channels.
  return hostGraph.removeNode(node).map([&](AGEvent event) {
    gcEventSender_.send(std::move(event));
    return NoneType{};
  });
}

Graph::Res Graph::addEdge(HNode *from, HNode *to) {
  // collectDisposedNodes();
  return hostGraph.addEdge(from, to).map([&](AGEvent event) {
    sendPoolGrowIfNeeded();
    eventSender_.send(std::move(event));
    return NoneType{};
  });
}

void Graph::linkNodes(HNode *from, HNode *to) {
  HostGraph::linkNodes(from, to);
}

Graph::Res Graph::removeEdge(HNode *from, HNode *to) {
  // collectDisposedNodes();
  return hostGraph.removeEdge(from, to).map([&](AGEvent event) {
    eventSender_.send(std::move(event));
    return NoneType{};
  });
}

Graph::Res Graph::removeAllEdges(HNode *from) {
  // collectDisposedNodes();
  return hostGraph.removeAllEdges(from).map([&](AGEvent event) {
    eventSender_.send(std::move(event));
    return NoneType{};
  });
}

void Graph::collectDisposedNodes() {
  hostGraph.collectDisposedNodes();
}

void Graph::sendPoolGrowIfNeeded() {
  auto edges = static_cast<std::uint32_t>(hostGraph.edgeCount());
  // edges > poolCapacity_ / 2 || (poolCapacity_ == 0 && edges > 0) left for clarity
  if (edges > poolCapacity_ / 2) {
    std::uint32_t newCap = std::max(static_cast<std::uint32_t>(edges * 2), std::uint32_t{64});
    auto buf = std::make_unique<InputPool::Slot[]>(newCap);
    eventSender_.send(
        [buf = std::move(buf), newCap](
            AudioGraph &graph, Disposer<audioapi::DISPOSER_PAYLOAD_SIZE> &disposer) mutable {
          auto *old = graph.pool().adoptBuffer(buf.release(), newCap);
          if (old) {
            disposer.dispose(OwnedSlotBuffer(old));
          }
        });
    poolCapacity_ = newCap;
  }
}

void Graph::sendNodeGrowIfNeeded() {
  auto nodes = static_cast<std::uint32_t>(hostGraph.nodeCount());
  if (nodes > nodeCapacity_) {
    std::uint32_t newCap = std::max(static_cast<std::uint32_t>(nodes * 2), std::uint32_t{64});
    auto buf = AudioGraph::makeNodeBuffer(newCap);
    eventSender_.send(
        [buf = std::move(buf)](
            AudioGraph &graph, Disposer<audioapi::DISPOSER_PAYLOAD_SIZE> &disposer) mutable {
          auto old = graph.adoptNodeBuffer(std::move(buf));
          disposer.dispose(std::move(old));
        });
    nodeCapacity_ = newCap;
  }
}

} // namespace audioapi::utils::graph
