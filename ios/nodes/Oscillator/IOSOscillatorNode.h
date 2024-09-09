#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <OscillatorNode.h>
#else // when compiled as C++
typedef struct objc_object OscillatorNode;
#endif // __OBJC__

#include "IOSAudioContext.h"
#include "IOSAudioNode.h"
#include "IOSAudioParam.h"

namespace audioapi {
class IOSOscillatorNode : public IOSAudioNode {
 public:
  explicit IOSOscillatorNode(std::shared_ptr<IOSAudioContext> context);
  ~IOSOscillatorNode();
  void start(double time) const;
  void stop(double time) const;
  void setType(const std::string &type) const;
  std::string getType() const;
  std::shared_ptr<IOSAudioParam> getFrequencyParam();
  std::shared_ptr<IOSAudioParam> getDetuneParam();

 protected:
  OscillatorNode *oscillatorNode_;
};
} // namespace audioapi
