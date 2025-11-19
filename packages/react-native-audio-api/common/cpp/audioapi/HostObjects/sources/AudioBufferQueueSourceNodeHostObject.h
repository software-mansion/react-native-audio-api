#pragma once

#include <audioapi/HostObjects/sources/AudioBufferBaseSourceNodeHostObject.h>

#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

class AudioBufferQueueSourceNode;

class AudioBufferQueueSourceNodeHostObject : public AudioBufferBaseSourceNodeHostObject {
 public:
  explicit AudioBufferQueueSourceNodeHostObject(
      const std::shared_ptr<AudioBufferQueueSourceNode> &node);

  JSI_HOST_FUNCTION_DECL(start);
  JSI_HOST_FUNCTION_DECL(pause);
  JSI_HOST_FUNCTION_DECL(enqueueBuffer);
  JSI_HOST_FUNCTION_DECL(dequeueBuffer);
  JSI_HOST_FUNCTION_DECL(clearBuffers);
};

} // namespace audioapi
