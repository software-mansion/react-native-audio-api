#pragma once

#include <memory>
#include <vector>

#include "AudioNodeHostObject.h"
#include "AudioScheduledSourceNode.h"

namespace audioapi {
using namespace facebook;

class AudioScheduledSourceNodeHostObject : public AudioNodeHostObject {
 public:
  explicit AudioScheduledSourceNodeHostObject(
      const std::shared_ptr<AudioScheduledSourceNode> &node)
      : AudioNodeHostObject(node) {
    addFunctions(
        JSI_EXPORT_FUNCTION(AudioScheduledSourceNodeHostObject, start),
        JSI_EXPORT_FUNCTION(AudioScheduledSourceNodeHostObject, stop));
  }

  JSI_HOST_FUNCTION(start) {
    auto when = args[0].getNumber();
    auto audioScheduleSourceNode =
        std::static_pointer_cast<AudioScheduledSourceNode>(node_);
    audioScheduleSourceNode->start(when);
    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(stop) {
    auto time = args[0].getNumber();
    auto audioScheduleSourceNode =
        std::static_pointer_cast<AudioScheduledSourceNode>(node_);
    audioScheduleSourceNode->stop(time);
    return jsi::Value::undefined();
  }
};
} // namespace audioapi
