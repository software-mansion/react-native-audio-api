#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <GainNode.h>
#else // when compiled as C++
typedef struct objc_object GainNode;
#endif // __OBJC__

#include "IOSAudioNode.h"
#include "IOSAudioParam.h"

namespace audioapi {
class IOSGainNode : public IOSAudioNode {
 protected:
  GainNode *gainNode_;

 public:
  explicit IOSGainNode(GainNode *gainNode);
  ~IOSGainNode();
  std::shared_ptr<IOSAudioParam> getGainParam();
};
} // namespace audioapi
