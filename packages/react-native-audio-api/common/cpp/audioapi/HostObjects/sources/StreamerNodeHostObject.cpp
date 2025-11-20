#include <audioapi/HostObjects/sources/StreamerNodeHostObject.h>

#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/effects/PeriodicWaveHostObject.h>
#include <audioapi/core/sources/StreamerNode.h>
#include <memory>

namespace audioapi {

StreamerNodeHostObject::StreamerNodeHostObject(const std::shared_ptr<StreamerNode> &node)
    : AudioScheduledSourceNodeHostObject(node) {
  addFunctions(JSI_EXPORT_FUNCTION(StreamerNodeHostObject, initialize));
}

JSI_HOST_FUNCTION_IMPL(StreamerNodeHostObject, initialize) {
#if !RN_AUDIO_API_FFMPEG_DISABLED
  auto streamerNode = std::static_pointer_cast<StreamerNode>(node_);
  auto path = args[0].getString(runtime).utf8(runtime);
  auto result = streamerNode->initialize(path);
  return {result};
#else
  return false;
#endif
}

} // namespace audioapi
