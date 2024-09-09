#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <AudioBufferSourceNode.h>
#else // when compiled as C++
typedef struct objc_object AudioBufferSourceNode;
#endif // __OBJC__

#include "IOSAudioNode.h"
#include "IOSAudioBuffer.h"

namespace audioapi {
class IOSAudioBufferSourceNode : public IOSAudioNode {
    
protected:
 AudioBufferSourceNode *bufferSource_;
    
 public:
  explicit IOSAudioBufferSourceNode(AudioBufferSourceNode *bufferSource);
  ~IOSAudioBufferSourceNode();
  void setLoop(bool loop) const;
  bool getLoop();
    std::shared_ptr<IOSAudioBuffer> getBuffer();
    void setBuffer(std::shared_ptr<IOSAudioBuffer> buffer);
    void start(double time);
    void stop(double time);
};
} // namespace audioapi
