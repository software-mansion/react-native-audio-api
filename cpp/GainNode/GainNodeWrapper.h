#pragma once

#include "AudioNodeWrapper.h"
#include "AudioParamWrapper.h"
#include <memory>

#ifdef ANDROID
#include "GainNode.h"
#else
#include "IOSAudioContext.h"
#include "IOSGainNode.h"
#endif

namespace audiocontext {

#ifdef ANDROID
class GainNode;
#endif

class GainNodeWrapper : public AudioNodeWrapper {
#ifdef ANDROID
 public:
  explicit GainNodeWrapper(GainNode *gainNode);
#else
 private:
  std::shared_ptr<IOSGainNode> getGainNodeFromAudioNode();

 public:
  explicit GainNodeWrapper(std::shared_ptr<IOSAudioContext> context);
#endif
 private:
  std::shared_ptr<AudioParamWrapper> gainParam_;

 public:
  std::shared_ptr<AudioParamWrapper> getGainParam() const;
};
} // namespace audiocontext
