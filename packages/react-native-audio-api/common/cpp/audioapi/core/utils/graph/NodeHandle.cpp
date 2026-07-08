#include <audioapi/core/utils/graph/NodeHandle.h>
#include <memory>
#include <utility>

namespace audioapi::utils::graph {

NodeHandle::NodeHandle(std::uint32_t index, std::unique_ptr<GraphObject> audioNode)
    : audioNode(std::move(audioNode)), index(index) {}

} // namespace audioapi::utils::graph
