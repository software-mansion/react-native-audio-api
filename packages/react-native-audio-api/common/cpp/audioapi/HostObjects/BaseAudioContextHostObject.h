#pragma once

#include <audioapi/jsi/JsiHostObject.h>
#include <audioapi/jsi/JsiPromise.h>

#include <jsi/jsi.h>
#include <memory>
#include <utility>
#include <vector>
#include <cstddef>

namespace audioapi {
using namespace facebook;

class BaseAudioContext;

class BaseAudioContextHostObject : public JsiHostObject {
 public:
  explicit BaseAudioContextHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker);

  JSI_PROPERTY_GETTER_DECL(destination);
  JSI_PROPERTY_GETTER_DECL(state);
  JSI_PROPERTY_GETTER_DECL(sampleRate);
  JSI_PROPERTY_GETTER_DECL(currentTime);

  JSI_HOST_FUNCTION_DECL(createWorkletSourceNode);
  JSI_HOST_FUNCTION_DECL(createWorkletNode);
  JSI_HOST_FUNCTION_DECL(createWorkletProcessingNode);
  JSI_HOST_FUNCTION_DECL(createRecorderAdapter);
  JSI_HOST_FUNCTION_DECL(createOscillator);
  JSI_HOST_FUNCTION_DECL(createStreamer);
  JSI_HOST_FUNCTION_DECL(createConstantSource);
  JSI_HOST_FUNCTION_DECL(createGain);
  JSI_HOST_FUNCTION_DECL(createStereoPanner);
  JSI_HOST_FUNCTION_DECL(createBiquadFilter);
  JSI_HOST_FUNCTION_DECL(createBufferSource);
  JSI_HOST_FUNCTION_DECL(createBufferQueueSource);
  JSI_HOST_FUNCTION_DECL(createBuffer);
  JSI_HOST_FUNCTION_DECL(createPeriodicWave);
  JSI_HOST_FUNCTION_DECL(createAnalyser);
  JSI_HOST_FUNCTION_DECL(createConvolver);

  std::shared_ptr<BaseAudioContext> context_;

 protected:
  std::shared_ptr<PromiseVendor> promiseVendor_;
  std::shared_ptr<react::CallInvoker> callInvoker_;
};
} // namespace audioapi
