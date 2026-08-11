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
  explicit AudioDestinationNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioDestinationOptions &options = AudioDestinationOptions())
      : AudioNode(context, options) {
    processableState_ = GraphObject::PROCESSABLE_STATE::ALWAYS_PROCESSABLE;
  }

 protected:
  // Output normalization (peak limiting) is applied by the real-time audio
  // players (iOS / Android) just before the buffer is handed to the hardware,
  // NOT here. Doing it in the destination node would also rescale offline
  // renders (OfflineAudioContext), breaking Web Audio API spec conformance.
  void processNode(int /*framesToProcess*/) final {}
};

} // namespace audioapi
