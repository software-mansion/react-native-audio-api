#pragma once

#include <audioapi/HostObjects/core/AudioNodeHostObject.h>
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
      JSI_EXPORT_PROPERTY_SETTER(AudioScheduledSourceNodeHostObject, onEnded));

    addFunctions(
        JSI_EXPORT_FUNCTION(AudioScheduledSourceNodeHostObject, start),
        JSI_EXPORT_FUNCTION(AudioScheduledSourceNodeHostObject, stop));
  }

  ~AudioScheduledSourceNodeHostObject() {
    auto audioScheduledSourceNode =
        std::static_pointer_cast<AudioScheduledSourceNode>(node_);

    // When JSI object is garbage collected (together with the eventual callback),
    // underlying source node might still be active and try to call the non-existing callback.
    audioScheduledSourceNode->clearOnEndedCallback();
  }

  JSI_PROPERTY_SETTER_DECL(onEnded);

  JSI_HOST_FUNCTION_DECL(start);
  JSI_HOST_FUNCTION_DECL(stop);
};
} // namespace audioapi
