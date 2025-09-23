#pragma once

#include <audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h>
#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/effects/PeriodicWaveHostObject.h>
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

  JSI_HOST_FUNCTION_DECL(initialize);
};
} // namespace audioapi
