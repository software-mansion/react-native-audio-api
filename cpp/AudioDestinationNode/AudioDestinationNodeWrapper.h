#pragma once

#include "AudioNodeWrapper.h"
#include <memory>

#ifdef ANDROID
#include "AudioDestinationNode.h"
#include "AudioNodeWrapper.h"
#else
#include "IOSAudioContext.h"
#include "IOSAudioDestinationNode.h"
#endif

namespace audiocontext {

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
  explicit AudioDestinationNodeWrapper(std::shared_ptr<IOSAudioContext> context)
      : AudioNodeWrapper() {
    node_ = std::make_shared<IOSAudioDestinationNode>(context);
  }
#endif
};
} // namespace audiocontext
