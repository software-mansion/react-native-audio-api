#pragma once

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/core/sources/AudioBufferQueueSourceNode.h>
#include <audioapi/HostObjects/sources/AudioBufferBaseSourceNodeHostObject.h>

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
                JSI_EXPORT_FUNCTION(AudioBufferQueueSourceNodeHostObject, dequeueBuffer),
                JSI_EXPORT_FUNCTION(AudioBufferQueueSourceNodeHostObject, clearBuffers),
                JSI_EXPORT_FUNCTION(AudioBufferQueueSourceNodeHostObject, pause));
    }

    JSI_HOST_FUNCTION_DECL(pause);
    JSI_HOST_FUNCTION_DECL(enqueueBuffer);
    JSI_HOST_FUNCTION_DECL(dequeueBuffer);
    JSI_HOST_FUNCTION_DECL(clearBuffers);
};

} // namespace audioapi
