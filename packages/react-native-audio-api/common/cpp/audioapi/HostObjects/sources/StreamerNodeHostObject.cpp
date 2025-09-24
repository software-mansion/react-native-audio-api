#include <audioapi/HostObjects/sources/StreamerNodeHostObject.h>

#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/effects/PeriodicWaveHostObject.h>
#include <audioapi/core/sources/StreamerNode.h>

namespace audioapi {

StreamerNodeHostObject::StreamerNodeHostObject(
    const std::shared_ptr<StreamerNode> &node)
    : AudioScheduledSourceNodeHostObject(node) {
  addFunctions(JSI_EXPORT_FUNCTION(StreamerNodeHostObject, initialize));
}

JSI_HOST_FUNCTION_IMPL(StreamerNodeHostObject, initialize) {
  auto streamerNode = std::static_pointer_cast<StreamerNode>(node_);
  auto path = args[0].getString(runtime).utf8(runtime);
  auto result = streamerNode->initialize(path);
  return {result};
}

} // namespace audioapi
