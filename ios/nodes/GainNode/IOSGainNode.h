#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <GainNode.h>
#else // when compiled as C++
typedef struct objc_object GainNode;
#endif // __OBJC__

#include "IOSAudioContext.h"
#include "IOSAudioNode.h"
#include "IOSAudioParam.h"

namespace audiocontext {
class IOSGainNode : public IOSAudioNode {
 public:
  explicit IOSGainNode(std::shared_ptr<IOSAudioContext> context);
  ~IOSGainNode();
  std::shared_ptr<IOSAudioParam> getGainParam();

 protected:
  GainNode *gainNode_;
};
} // namespace audiocontext
