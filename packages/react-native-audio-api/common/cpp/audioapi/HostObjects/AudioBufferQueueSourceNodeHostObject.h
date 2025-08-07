#pragma once

#include <audioapi/HostObjects/AudioBufferHostObject.h>
#include <audioapi/core/sources/AudioBufferQueueSourceNode.h>
#include <audioapi/HostObjects/AudioBufferBaseSourceNodeHostObject.h>

#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

class AudioBufferQueueSourceNodeHostObject
            : public AudioBufferBaseSourceNodeHostObject {
 public:
    explicit AudioBufferQueueSourceNodeHostObject(
            const std::shared_ptr<AudioBufferQueueSourceNode> &node)
            : AudioBufferBaseSourceNodeHostObject(node) {
        addFunctions(
                JSI_EXPORT_FUNCTION(AudioBufferQueueSourceNodeHostObject, enqueueBuffer),
                JSI_EXPORT_FUNCTION(AudioBufferQueueSourceNodeHostObject, pause));
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

        auto bufferId = audioBufferQueueSourceNode->enqueueBuffer(audioBufferHostObject->audioBuffer_);

        return jsi::String::createFromUtf8(runtime, bufferId);
    }

    JSI_HOST_FUNCTION(dequeueBuffer) {
        auto audioBufferQueueSourceNode =
                std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

        auto bufferId =
                args[0].getNumber();

        audioBufferQueueSourceNode->dequeueBuffer(bufferId);

        return jsi::Value::undefined();
    }

    JSI_HOST_FUNCTION(clearBuffers) {
        auto audioBufferQueueSourceNode =
                std::static_pointer_cast<AudioBufferQueueSourceNode>(node_);

        audioBufferQueueSourceNode->clearBuffers();

        return jsi::Value::undefined();
    }
};

} // namespace audioapi
