#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <AudioDestinationNode.h>
#else // when compiled as C++
typedef struct objc_object AudioDestinationNode;
#endif // __OBJC__

#include "IOSAudioNode.h"

#import <memory>

namespace audioapi {
class IOSAudioDestinationNode : public IOSAudioNode {
 public:
  explicit IOSAudioDestinationNode(AudioDestinationNode *destination);
  ~IOSAudioDestinationNode();

 protected:
  AudioDestinationNode *destination_;
};
} // namespace audioapi
