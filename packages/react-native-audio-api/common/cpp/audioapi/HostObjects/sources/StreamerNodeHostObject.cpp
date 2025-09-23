#include <audioapi/HostObjects/sources/StreamerNodeHostObject.h>

namespace audioapi {

JSI_HOST_FUNCTION_IMPL(StreamerNodeHostObject, initialize) {
  auto streamerNode = std::static_pointer_cast<StreamerNode>(node_);
  auto path = args[0].getString(runtime).utf8(runtime);
  auto result = streamerNode->initialize(path);
  return jsi::Value(result);
}

} // namespace audioapi