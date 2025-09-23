#pragma once

#include <audioapi/core/core/AudioContext.h>
#include <audioapi/HostObjects/core/BaseAudioContextHostObject.h>

#include <jsi/jsi.h>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi {
using namespace facebook;

class AudioContextHostObject : public BaseAudioContextHostObject {
 public:
  explicit AudioContextHostObject(
      const std::shared_ptr<AudioContext> &audioContext,
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker)
      : BaseAudioContextHostObject(audioContext, runtime, callInvoker) {
    addFunctions(
      JSI_EXPORT_FUNCTION(AudioContextHostObject, close),
      JSI_EXPORT_FUNCTION(AudioContextHostObject, resume),
      JSI_EXPORT_FUNCTION(AudioContextHostObject, suspend));
  }

  JSI_HOST_FUNCTION_DECL(close);
  JSI_HOST_FUNCTION_DECL(resume);
  JSI_HOST_FUNCTION_DECL(suspend);
};
} // namespace audioapi
