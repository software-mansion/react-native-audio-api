#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/utils/graph/HostGraph.h>

namespace audioapi {

/// @brief Downcast a graph node's audio payload to its concrete AudioNode type.
/// Valid for the HostObject lifetime — the concrete type is fixed at construction.
template <typename NodeT>
  requires std::derived_from<NodeT, AudioNode>
[[nodiscard]] NodeT *typedAudioNode(utils::graph::HostGraph::Node *graphNode) noexcept {
  return static_cast<NodeT *>(graphNode->handle->audioNode->asAudioNode());
}

} // namespace audioapi
