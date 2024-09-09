#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import "StereoPannerNode.h"
#else // when compiled as C++
typedef struct objc_object StereoPannerNode;
#endif // __OBJC__

#include "IOSAudioNode.h"
#include "IOSAudioParam.h"

namespace audioapi {
class IOSStereoPannerNode : public IOSAudioNode {
 public:
  explicit IOSStereoPannerNode(StereoPannerNode *stereoPanner);
  ~IOSStereoPannerNode();
  std::shared_ptr<IOSAudioParam> getPanParam();

 protected:
  StereoPannerNode *panner_;
};
} // namespace audioapi
