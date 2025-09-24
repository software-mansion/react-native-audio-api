#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>

#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

class AudioScheduledSourceNode;

class AudioScheduledSourceNodeHostObject : public AudioNodeHostObject {
 public:
  explicit AudioScheduledSourceNodeHostObject(
      const std::shared_ptr<AudioScheduledSourceNode> &node);

  ~AudioScheduledSourceNodeHostObject() override;

  JSI_PROPERTY_SETTER_DECL(onEnded);

  JSI_HOST_FUNCTION_DECL(start);
  JSI_HOST_FUNCTION_DECL(stop);
};
} // namespace audioapi
