#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>

#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

class AudioScheduledSourceNodeHostObject : public AudioNodeHostObject {
 public:
  explicit AudioScheduledSourceNodeHostObject(
      const std::shared_ptr<AudioScheduledSourceNode> &node)
      : AudioNodeHostObject(node) {
    addSetters(
      JSI_EXPORT_PROPERTY_SETTER(AudioScheduledSourceNodeHostObject, onended));
    addFunctions(
        JSI_EXPORT_FUNCTION(AudioScheduledSourceNodeHostObject, start),
        JSI_EXPORT_FUNCTION(AudioScheduledSourceNodeHostObject, stop));
  }

  JSI_PROPERTY_SETTER(onended) {
    auto audioScheduleSourceNode =
            std::static_pointer_cast<AudioScheduledSourceNode>(node_);

    audioScheduleSourceNode->setOnEndedCallbackId(std::stoull(value.getString(runtime).utf8(runtime)));
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
