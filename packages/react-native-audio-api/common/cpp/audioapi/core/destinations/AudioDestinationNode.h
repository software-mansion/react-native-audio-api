#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <cstddef>
#include <memory>

namespace audioapi {

class BaseAudioContext;

class AudioDestinationNode : public AudioNode {
 public:
  explicit AudioDestinationNode(const std::shared_ptr<BaseAudioContext> &context)
      : AudioNode(context, AudioDestinationOptions()) {
    processableState_ = GraphObject::PROCESSABLE_STATE::ALWAYS_PROCESSABLE;
  }

 protected:
  void processNode(int) final {
    audioBuffer_->normalize();
  };
};

} // namespace audioapi
