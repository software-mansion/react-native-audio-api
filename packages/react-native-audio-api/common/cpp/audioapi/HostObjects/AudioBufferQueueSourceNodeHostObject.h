#pragma once

#include <audioapi/HostObjects/AudioBufferHostObject.h>
#include <audioapi/core/sources/AudioBufferQueueSourceNode.h>
#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/AudioScheduledSourceNodeHostObject.h>

#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

class AudioBufferQueueSourceNodeHostObject
            : public AudioScheduledSourceNodeHostObject {
 public:
    explicit AudioBufferQueueSourceNodeHostObject(
            const std::shared_ptr<AudioBufferQueueSourceNode> &node)
            : AudioScheduledSourceNodeHostObject(node) {
        addGetters(
                JSI_EXPORT_PROPERTY_GETTER(AudioBufferQueueSourceNodeHostObject, detune),
                JSI_EXPORT_PROPERTY_GETTER(AudioBufferQueueSourceNodeHostObject, playbackRate));

        addSetters(
                JSI_EXPORT_PROPERTY_SETTER(AudioBufferQueueSourceNodeHostObject, onPositionChanged),
                JSI_EXPORT_PROPERTY_SETTER(AudioBufferQueueSourceNodeHostObject, onPositionChangedInterval));

        // start method is overridden in this class
        functions_->erase("start");

        addFunctions(
                JSI_EXPORT_FUNCTION(AudioBufferQueueSourceNodeHostObject, start),
                JSI_EXPORT_FUNCTION(AudioBufferQueueSourceNodeHostObject, enqueueBuffer),
                JSI_EXPORT_FUNCTION(AudioBufferQueueSourceNodeHostObject, pause));
    }

    JSI_PROPERTY_GETTER(detune) {
        auto audioBufferSourceNode =
                std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);
        auto detune = audioBufferSourceNode->getDetuneParam();
        auto detuneHostObject = std::make_shared<AudioParamHostObject>(detune);
        return jsi::Object::createFromHostObject(runtime, detuneHostObject);
    }

    JSI_PROPERTY_GETTER(playbackRate) {
        auto audioBufferSourceNode =
                std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);
        auto playbackRate = audioBufferSourceNode->getPlaybackRateParam();
        auto playbackRateHostObject =
                std::make_shared<AudioParamHostObject>(playbackRate);
        return jsi::Object::createFromHostObject(runtime, playbackRateHostObject);
    }

    JSI_PROPERTY_SETTER(onPositionChanged) {
        auto audioBufferQueueSourceNode =
                std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

        audioBufferQueueSourceNode->setOnPositionChangedCallbackId(std::stoull(value.getString(runtime).utf8(runtime)));
    }

    JSI_PROPERTY_SETTER(onPositionChangedInterval) {
        auto audioBufferQueueSourceNode =
                std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

        audioBufferQueueSourceNode->setOnPositionChangedInterval(value.getNumber());
    }

    JSI_HOST_FUNCTION(start) {
        auto when = args[0].getNumber();

        auto audioBufferQueueSourceNode =
                std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

        double offset = -1.0;

        if (args[1].isNumber()) {
            offset = args[1].asNumber();
        }

        audioBufferQueueSourceNode->start(when, offset);

        return jsi::Value::undefined();
    }

    JSI_HOST_FUNCTION(pause) {
        auto audioBufferQueueSourceNode =
                std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

        audioBufferQueueSourceNode->pause();

        return jsi::Value::undefined();
    }

    JSI_HOST_FUNCTION(enqueueBuffer) {
        auto audioBufferQueueSourceNode =
                std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

        auto audioBufferHostObject =
                args[0].getObject(runtime).asHostObject<AudioBufferHostObject>(runtime);
        int bufferId = args[1].asNumber();
        auto isLastBuffer = args[2].asBool();

        audioBufferQueueSourceNode->enqueueBuffer(audioBufferHostObject->audioBuffer_, bufferId, isLastBuffer);

        return jsi::Value::undefined();
    }
};

} // namespace audioapi
