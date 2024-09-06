#pragma once

#include <memory>
#include "AudioNodeWrapper.h"

#ifdef ANDROID
#include "AudioDestinationNode.h"
#include "AudioNodeWrapper.h"
#else
#include "IOSAudioDestinationNode.h"
#endif

namespace audioapi {

#ifdef ANDROID
class AudioDestinationNode;
#endif

class AudioDestinationNodeWrapper : public AudioNodeWrapper {
#ifdef ANDROID
 public:
  explicit AudioDestinationNodeWrapper(AudioDestinationNode *destinationNode)
      : AudioNodeWrapper(destinationNode) {}
#else
 public:
  explicit AudioDestinationNodeWrapper(std::shared_ptr<IOSAudioDestinationNode> destination)
    : AudioNodeWrapper(destination) {}
#endif
};
} // namespace audioapi
