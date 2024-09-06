#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <BiquadFilterNode.h>
#else // when compiled as C++
typedef struct objc_object BiquadFilterNode;
#endif // __OBJC__

#include <string>
#include "IOSAudioNode.h"
#include "IOSAudioParam.h"

namespace audioapi {
class IOSBiquadFilterNode : public IOSAudioNode {
 public:
  explicit IOSBiquadFilterNode(AudioContext *context);
  ~IOSBiquadFilterNode();
  void setType(const std::string &type) const;
  std::string getType() const;
  std::shared_ptr<IOSAudioParam> getFrequencyParam();
  std::shared_ptr<IOSAudioParam> getDetuneParam();
  std::shared_ptr<IOSAudioParam> getQParam();
  std::shared_ptr<IOSAudioParam> getGainParam();

 protected:
  BiquadFilterNode *biquadFilterNode_;
};
} // namespace audioapi
