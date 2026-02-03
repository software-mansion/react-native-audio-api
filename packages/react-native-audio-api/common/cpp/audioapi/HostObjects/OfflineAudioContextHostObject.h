#pragma once

#include <audioapi/HostObjects/BaseAudioContextHostObject.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>

#include <jsi/jsi.h>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi {
using namespace facebook;

class OfflineAudioContext;

class OfflineAudioContextHostObject : public BaseAudioContextHostObject {
 public:
  explicit OfflineAudioContextHostObject(
      int numberOfChannels,
      size_t length,
      float sampleRate,
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const RuntimeRegistry &runtimeRegistry,
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker);

  JSI_HOST_FUNCTION_DECL(resume);
  JSI_HOST_FUNCTION_DECL(suspend);
  JSI_HOST_FUNCTION_DECL(startRendering);
};
} // namespace audioapi
