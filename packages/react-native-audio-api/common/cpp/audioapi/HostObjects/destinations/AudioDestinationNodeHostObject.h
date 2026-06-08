#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/HostObjects/TypedAudioNodePtr.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>

namespace audioapi {
using namespace facebook;

class AudioDestinationNodeHostObject : public AudioNodeHostObject {
 public:
  explicit AudioDestinationNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioDestinationOptions &options = AudioDestinationOptions())
      : AudioNodeHostObject(
            context->getGraph(),
            std::make_unique<AudioDestinationNode>(context),
            options),
        destinationNode_(typedAudioNode<AudioDestinationNode>(node_)) {
    context->initialize(destinationNode_);
  }

 private:
  AudioDestinationNode *destinationNode_ = nullptr;
};

} // namespace audioapi
