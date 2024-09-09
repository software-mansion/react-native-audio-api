#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <AudioBufferSourceNode.h>
#else // when compiled as C++
typedef struct objc_object AudioBufferSourceNode;
#endif // __OBJC__

#include <string>
#include "IOSAudioNode.h"
#include "IOSAudioBuffer.h"

namespace audioapi {
class IOSAudioBufferSourceNode : public IOSAudioNode {
 public:
  explicit IOSAudioBufferSourceNode(AudioContext *context);
  ~IOSAudioBufferSourceNode();
  void start(double time) const;
  void stop(double time) const;
  void setLoop(bool loop) const;
    bool getLoop();
    IOSAudioBuffer *getBuffer() const;
    void setBuffer(IOSAudioBuffer *buffer);
    

 protected:
  AudioBufferSourceNode *bufferSource_;
};
} // namespace audioapi

