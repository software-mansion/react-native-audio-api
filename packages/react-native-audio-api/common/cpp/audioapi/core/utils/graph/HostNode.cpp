#include <audioapi/core/utils/graph/HostNode.h>

#include <memory>
#include <utility>

namespace audioapi::utils::graph {

HostNode::HostNode(std::shared_ptr<Graph> graph, std::unique_ptr<GraphObject> graphObject)
    : graph_(std::move(graph)), node_(graph_->addNode(std::move(graphObject))) {}

HostNode::~HostNode() {
  if (graph_ && (node_ != nullptr)) {
    graph_->collectDisposedNodes();
    (void)graph_->removeNode(node_);
    node_ = nullptr;
  }
}

HostNode::HostNode(HostNode &&other) noexcept
    : graph_(std::move(other.graph_)), node_(other.node_) {
  other.node_ = nullptr;
}

HostNode &HostNode::operator=(HostNode &&other) noexcept {
  if (this != &other) {
    if (graph_ && (node_ != nullptr)) {
      (void)graph_->removeNode(node_);
    }
    graph_ = std::move(other.graph_);
    node_ = other.node_;
    other.node_ = nullptr;
  }
  return *this;
}

auto HostNode::connect(HostNode &other) -> Result<NoneType, GraphError> {
  return graph_->addEdge(node_, other.node_);
}

auto HostNode::disconnect(HostNode &other) -> Result<NoneType, GraphError> {
  return graph_->removeEdge(node_, other.node_);
}

auto HostNode::disconnect() -> Result<NoneType, GraphError> {
  return graph_->removeAllEdges(node_);
}

auto HostNode::renegotiate() -> Result<NoneType, GraphError> {
  return graph_->renegotiateNodeChannels(node_);
}

HostNode::HostVertex *HostNode::rawNode() const {
  return node_;
}

const std::shared_ptr<Graph> &HostNode::graph() const {
  return graph_;
}

} // namespace audioapi::utils::graph
