#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <AudioNode.h>
#else // when compiled as C++
typedef struct objc_object AudioNode;
#endif // __OBJC__

#import <memory>
#include <string>

namespace audioapi {
class IOSAudioNode {
 protected:
  AudioNode *audioNode_;

 public:
  ~IOSAudioNode();
  virtual void connect(std::shared_ptr<IOSAudioNode> node);
  virtual void disconnect(std::shared_ptr<IOSAudioNode> node);
  int getNumberOfInputs();
  int getNumberOfOutputs();
  int getChannelCount() const;
  std::string getChannelCountMode() const;
  std::string getChannelInterpretation() const;
};
} // namespace audioapi
