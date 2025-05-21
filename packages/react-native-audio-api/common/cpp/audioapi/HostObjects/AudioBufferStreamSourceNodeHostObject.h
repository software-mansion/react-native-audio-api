#pragma once

#include <audioapi/HostObjects/AudioBufferHostObject.h>
#include <audioapi/core/sources/AudioBufferStreamSourceNode.h>
#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/AudioScheduledSourceNodeHostObject.h>

#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

class AudioBufferStreamSourceNodeHostObject
            : public AudioScheduledSourceNodeHostObject {
 public:
    explicit AudioBufferStreamSourceNodeHostObject(
            const std::shared_ptr<AudioBufferStreamSourceNode> &node)
            : AudioScheduledSourceNodeHostObject(node) {
        addGetters(
                JSI_EXPORT_PROPERTY_GETTER(AudioBufferStreamSourceNodeHostObject, detune),
                JSI_EXPORT_PROPERTY_GETTER(AudioBufferStreamSourceNodeHostObject, playbackRate));

        // start method is overridden in this class
        functions_->erase("start");

        addFunctions(
                JSI_EXPORT_FUNCTION(AudioBufferStreamSourceNodeHostObject, start),
                JSI_EXPORT_FUNCTION(AudioBufferStreamSourceNodeHostObject, enqueueAudioBuffer));
    }

    JSI_PROPERTY_GETTER(detune) {
        auto audioBufferSourceNode =
                std::static_pointer_cast<AudioBufferStreamSourceNode>(node_);
        auto detune = audioBufferSourceNode->getDetuneParam();
        auto detuneHostObject = std::make_shared<AudioParamHostObject>(detune);
        return jsi::Object::createFromHostObject(runtime, detuneHostObject);
    }

    JSI_PROPERTY_GETTER(playbackRate) {
        auto audioBufferSourceNode =
                std::static_pointer_cast<AudioBufferStreamSourceNode>(node_);
        auto playbackRate = audioBufferSourceNode->getPlaybackRateParam();
        auto playbackRateHostObject =
                std::make_shared<AudioParamHostObject>(playbackRate);
        return jsi::Object::createFromHostObject(runtime, playbackRateHostObject);
    }

    JSI_HOST_FUNCTION(start) {
        auto when = args[0].getNumber();
        auto offset = args[1].getNumber();

        auto audioBufferStreamSourceNode =
                std::static_pointer_cast<AudioBufferStreamSourceNode>(node_);

        audioBufferStreamSourceNode->start(when, offset);

        return jsi::Value::undefined();
    }

    JSI_HOST_FUNCTION(enqueueAudioBuffer) {
        auto audioBufferStreamSourceNode =
                std::static_pointer_cast<AudioBufferStreamSourceNode>(node_);

        auto audioBufferHostObject =
                args[0].getObject(runtime).asHostObject<AudioBufferHostObject>(runtime);

        audioBufferStreamSourceNode->enqueueAudioBuffer(audioBufferHostObject->audioBuffer_);

        return jsi::Value::undefined();
    }
};

} // namespace audioapi
