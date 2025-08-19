#pragma once

#include <audioapi/HostObjects/AudioScheduledSourceNodeHostObject.h>
#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/PeriodicWaveHostObject.h>
#include <audioapi/core/sources/StreamerNode.h>

#include <memory>
#include <string>
#include <vector>

namespace audioapi {
using namespace facebook;

class StreamerNodeHostObject : public AudioScheduledSourceNodeHostObject {
 public:
  explicit StreamerNodeHostObject(
          const std::shared_ptr<StreamerNode> &node)
      : AudioScheduledSourceNodeHostObject(node) {
    addFunctions(JSI_EXPORT_FUNCTION(StreamerNodeHostObject, initialize));
  }

  JSI_HOST_FUNCTION(initialize) {
    auto streamerNode = std::static_pointer_cast<StreamerNode>(node_);
    auto path = args[0].getString(runtime).utf8(runtime);
    auto result = streamerNode->initialize(path);
    return jsi::Value(result);
  }
};
} // namespace audioapi
