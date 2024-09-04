#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <BiquadFilterNode.h>
#else // when compiled as C++
typedef struct objc_object BiquadFilterNode;
#endif // __OBJC__

#include <string>
#include "IOSAudioContext.h"
#include "IOSAudioNode.h"
#include "IOSAudioParam.h"

namespace audiocontext {
class IOSBiquadFilterNode : public IOSAudioNode {
 public:
  explicit IOSBiquadFilterNode(std::shared_ptr<IOSAudioContext> context);
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
} // namespace audiocontext
