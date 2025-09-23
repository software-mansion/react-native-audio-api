#pragma once

#include <audioapi/core/sources/AudioBufferBaseSourceNode.h>
#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h>

#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

class AudioBufferBaseSourceNodeHostObject
        : public AudioScheduledSourceNodeHostObject {
 public:
    explicit AudioBufferBaseSourceNodeHostObject(
            const std::shared_ptr<AudioBufferBaseSourceNode> &node)
            : AudioScheduledSourceNodeHostObject(node) {
        addGetters(
                JSI_EXPORT_PROPERTY_GETTER(AudioBufferBaseSourceNodeHostObject, detune),
                JSI_EXPORT_PROPERTY_GETTER(AudioBufferBaseSourceNodeHostObject, playbackRate),
                JSI_EXPORT_PROPERTY_GETTER(AudioBufferBaseSourceNodeHostObject, onPositionChangedInterval));

        addSetters(
                JSI_EXPORT_PROPERTY_SETTER(AudioBufferBaseSourceNodeHostObject, onPositionChanged),
                JSI_EXPORT_PROPERTY_SETTER(AudioBufferBaseSourceNodeHostObject, onPositionChangedInterval));
    }

    ~AudioBufferBaseSourceNodeHostObject() {
        auto sourceNode =
                std::static_pointer_cast<AudioBufferBaseSourceNode>(node_);

      // When JSI object is garbage collected (together with the eventual callback),
      // underlying source node might still be active and try to call the non-existing callback.
      sourceNode->clearOnPositionChangedCallback();
    }

    JSI_PROPERTY_GETTER_DECL(detune);
    JSI_PROPERTY_GETTER_DECL(playbackRate);
    JSI_PROPERTY_GETTER_DECL(onPositionChangedInterval);

    JSI_PROPERTY_SETTER_DECL(onPositionChanged);
    JSI_PROPERTY_SETTER_DECL(onPositionChangedInterval);
};

} // namespace audioapi
