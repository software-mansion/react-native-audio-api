#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <AudioBufferSourceNode.h>
#else // when compiled as C++
typedef struct objc_object AudioBufferSourceNode;
#endif // __OBJC__

#include "IOSAudioNode.h"

namespace audioapi {
class IOSAudioBufferSourceNode : public IOSAudioNode {
 public:
  explicit IOSAudioBufferSourceNode(AudioBufferSourceNode *bufferSource);
  ~IOSAudioBufferSourceNode();
  void setLoop(bool loop) const;
  bool getLoop();

 protected:
  AudioBufferSourceNode *bufferSource_;
};
} // namespace audioapi
