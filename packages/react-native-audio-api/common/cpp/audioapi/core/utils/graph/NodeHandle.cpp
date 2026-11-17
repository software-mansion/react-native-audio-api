#include <audioapi/core/utils/graph/NodeHandle.h>
#include <memory>
#include <utility>

namespace audioapi::utils::graph {

NodeHandle::NodeHandle(std::unique_ptr<GraphObject> audioNode) : audioNode(std::move(audioNode)) {}

} // namespace audioapi::utils::graph
