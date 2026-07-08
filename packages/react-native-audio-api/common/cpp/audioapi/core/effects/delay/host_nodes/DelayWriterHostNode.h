#pragma once

#include <audioapi/core/effects/delay/DelayWriter.h>
#include <audioapi/core/utils/graph/Graph.h>
#include <audioapi/core/utils/graph/HostNode.h>
#include <memory>
#include <utility>

namespace audioapi {

class DelayWriter;

class DelayWriterHostNode : public utils::graph::HostNode {
 public:
  explicit DelayWriterHostNode(
      const std::shared_ptr<utils::graph::Graph> &graph,
      std::unique_ptr<DelayWriter> delayWriter)
      : utils::graph::HostNode(graph, std::move(delayWriter)) {}
};
} // namespace audioapi
