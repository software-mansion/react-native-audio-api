#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <AudioNode.h>
#else // when compiled as C++
typedef struct objc_object AudioNode;
#endif // __OBJC__

#import <memory>
#include "AudioContext.h"

namespace audioapi {
class IOSAudioNode {
 public:
  ~IOSAudioNode();
  AudioNode *audioNode_;
  virtual void connect(std::shared_ptr<IOSAudioNode> node);
  virtual void disconnect(std::shared_ptr<IOSAudioNode> node);
  int getNumberOfInputs();
  int getNumberOfOutputs();
};
} // namespace audioapi
