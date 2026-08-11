#pragma once

#include <audioapi/HostObjects/BaseAudioContextHostObject.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>

#include <jsi/jsi.h>
#include <memory>

namespace audioworklets {
using namespace facebook;

class WorkletAudioContextHostObject : public audioapi::BaseAudioContextHostObject {
 public:
  explicit WorkletAudioContextHostObject(
      float sampleRate,
      const std::shared_ptr<audioapi::IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker);

  JSI_HOST_FUNCTION_DECL(close);
  JSI_HOST_FUNCTION_DECL(resume);
  JSI_HOST_FUNCTION_DECL(suspend);
};

} // namespace audioworklets
