#pragma once

#include <memory>
#include "AudioNodeWrapper.h"

#ifdef ANDROID
#include "AudioDestinationNode.h"
#else
#include "IOSAudioDestinationNode.h"
#endif

namespace audioapi {

class AudioDestinationNodeWrapper : public AudioNodeWrapper {
#ifdef ANDROID

 public:
  explicit AudioDestinationNodeWrapper(
      const std::shared_ptr<AudioDestinationNode> &destinationNode)
      : AudioNodeWrapper(destinationNode) {}
#else

 public:
  explicit AudioDestinationNodeWrapper(
      const std::shared_ptr<IOSAudioDestinationNode> &destination)
      : AudioNodeWrapper(destination) {}
#endif
};
} // namespace audioapi
