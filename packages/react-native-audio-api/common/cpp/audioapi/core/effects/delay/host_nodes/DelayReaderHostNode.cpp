#include <audioapi/core/effects/delay/DelayReader.h>
#include <audioapi/core/effects/delay/host_nodes/DelayReaderHostNode.h>
#include <audioapi/core/utils/graph/Graph.h>
#include <audioapi/core/utils/graph/HostNode.h>

#include <memory>
#include <utility>

namespace audioapi {

class DelayReader;

DelayReaderHostNode::DelayReaderHostNode(
    const std::shared_ptr<utils::graph::Graph> &graph,
    std::unique_ptr<DelayReader> delayReader)
    : utils::graph::HostNode(graph, std::move(delayReader)) {}
} // namespace audioapi
