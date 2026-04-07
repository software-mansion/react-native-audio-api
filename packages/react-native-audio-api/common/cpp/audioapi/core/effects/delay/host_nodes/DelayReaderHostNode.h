#pragma once

#include <audioapi/core/utils/graph/Graph.h>
#include <audioapi/core/utils/graph/HostNode.h>
#include <memory>

namespace audioapi {

class DelayReader;

class DelayReaderHostNode : public utils::graph::HostNode {
 public:
  explicit DelayReaderHostNode(
      const std::shared_ptr<utils::graph::Graph> &graph,
      std::unique_ptr<DelayReader> delayReader);
};
} // namespace audioapi
