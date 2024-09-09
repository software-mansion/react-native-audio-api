#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <AudioBufferSourceNode.h>
#else // when compiled as C++
typedef struct objc_object AudioBufferSourceNode;
#endif // __OBJC__

#include "IOSAudioNode.h"

namespace audioapi {
class IOSAudioBufferSourceNode : public IOSAudioNode {
    
protected:
 AudioBufferSourceNode *bufferSource_;
    
 public:
  explicit IOSAudioBufferSourceNode(AudioBufferSourceNode *bufferSource);
  ~IOSAudioBufferSourceNode();
  void setLoop(bool loop) const;
  bool getLoop();
};
} // namespace audioapi
