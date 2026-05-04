#include <audioapi/core/utils/graph/Graph.h>
#include <audioapi/core/utils/graph/NodeHandle.h>

#include <memory>
#include <utility>

namespace audioapi::utils::graph {

Graph::Graph(
    size_t eventQueueCapacity,
    Disposer<audioapi::DISPOSER_PAYLOAD_SIZE> *disposer,
    std::uint32_t initialNodeCapacity,
    std::uint32_t initialEdgeCapacity)
    : disposer_(disposer), poolCapacity_(initialEdgeCapacity), nodeCapacity_(initialNodeCapacity) {
  using namespace audioapi::channels::spsc;

  auto [es, er] =
      channel<AGEvent, EVENT_OVERFLOW_STRATEGY, EVENT_WAIT_STRATEGY>(eventQueueCapacity);
  eventSender_ = std::move(es);
  eventReceiver_ = std::move(er);

  auto [gs, gr] =
      channel<OrphanEnvelope, EVENT_OVERFLOW_STRATEGY, EVENT_WAIT_STRATEGY>(eventQueueCapacity);
  gcEventSender_ = std::move(gs);
  gcEventReceiver_ = std::move(gr);
  if (nodeCapacity_ > 0) {
    audioGraph.reserveNodes(nodeCapacity_);
  }
  if (poolCapacity_ > 0) {
    audioGraph.pool().grow(poolCapacity_);
  }
}

void Graph::processEvents() {
  std::atomic_thread_fence(std::memory_order_acquire);
  using audioapi::channels::spsc::ResponseStatus;

  auto drainA = [&] {
    AGEvent ev;
    while (eventReceiver_.try_receive(ev) == ResponseStatus::SUCCESS) {
      if (ev) {
        ev(audioGraph, *disposer_);
      }
    }
  };

  // Steady-state: drain any A events queued since the last cycle.
  drainA();

  // For every pending orphan on Channel B: ensure Channel A's receive
  // cursor has reached the barrier the GC thread snapshotted at orphan
  // enqueue time. Until that barrier is met, every A event that the
  // orphan happens-after must still be in flight; we drain A again to
  // catch up. Only then do we consume and apply the orphan.
  while (const OrphanEnvelope *front = gcEventReceiver_.try_peek()) {
    if (eventReceiver_.rcvCursor() < front->barrier) {
      drainA();
      if (eventReceiver_.rcvCursor() < front->barrier) {
        // Producer is mid-send on Channel A; the event will arrive
        // imminently. Defer this orphan to the next processEvents()
        // tick rather than burning audio-thread cycles spinning.
        break;
      }
    }
    OrphanEnvelope consumed;
    gcEventReceiver_.try_receive(consumed);
    if (consumed.action) {
      consumed.action(audioGraph, *disposer_);
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
    // Snapshot Channel A's send cursor *after* the host-side ghost flip
    // performed inside `hostGraph.removeNode`. Any subsequent JS-thread
    // attempt to enqueue an A event referencing this node is guaranteed
    // to see ghost=true (the JS thread re-acquires nodesMutex_ in
    // addEdge/removeEdge and bails out). Therefore every A event that
    // could possibly reference this node lives at index < barrier.
    auto barrier = eventSender_.sendCursor();
    gcEventSender_.send(OrphanEnvelope{.barrier = barrier, .action = std::move(event)});
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
    std::uint32_t newCap = edges * 2;
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
    std::uint32_t newCap = nodes * 2;
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
