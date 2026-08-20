#pragma once

#include <audioapi/jsi/ContextPromiseResolver.hpp>
#include <audioapi/jsi/HostObject.h>
#include <audioapi/jsi/JsiPromise.h>

#include <jsi/jsi.h>
#include <cstddef>
#include <memory>

namespace audioapi {
using namespace facebook;

class BaseAudioContext;
class AudioDestinationNodeHostObject;
class AudioListenerHostObject;

class BaseAudioContextHostObject : public HostObject {
 public:
  explicit BaseAudioContextHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker,
      int destinationChannelCount = 2);

  ~BaseAudioContextHostObject() override;

  JSI_PROPERTY_GETTER_DECL(destination);
  JSI_PROPERTY_GETTER_DECL(listener);
  JSI_PROPERTY_GETTER_DECL(sampleRate);
  JSI_PROPERTY_GETTER_DECL(currentTime);

  JSI_HOST_FUNCTION_DECL(createRecorderAdapter);
  JSI_HOST_FUNCTION_DECL(createOscillator);
  JSI_HOST_FUNCTION_DECL(createConstantSource);
  JSI_HOST_FUNCTION_DECL(createGain);
  JSI_HOST_FUNCTION_DECL(createStereoPanner);
  JSI_HOST_FUNCTION_DECL(createBiquadFilter);
  JSI_HOST_FUNCTION_DECL(createIIRFilter);
  JSI_HOST_FUNCTION_DECL(createBufferSource);
  JSI_HOST_FUNCTION_DECL(createFileSource);
  JSI_HOST_FUNCTION_DECL(createBufferQueueSource);
  JSI_HOST_FUNCTION_DECL(createPeriodicWave);
  JSI_HOST_FUNCTION_DECL(createAnalyser);
  JSI_HOST_FUNCTION_DECL(createConvolver);
  JSI_HOST_FUNCTION_DECL(createWaveShaper);
  JSI_HOST_FUNCTION_DECL(createDelay);
  JSI_HOST_FUNCTION_DECL(createChannelMerger);
  JSI_HOST_FUNCTION_DECL(createChannelSplitter);

  /// @brief Access the underlying C++ audio context.
  /// @return The underlying C++ audio context.
  [[nodiscard]] const std::shared_ptr<BaseAudioContext> &getContext() const {
    return context_;
  }

 protected:
  std::shared_ptr<BaseAudioContext> context_;
  std::shared_ptr<PromiseVendor> promiseVendor_;
  std::shared_ptr<react::CallInvoker> callInvoker_;

  std::shared_ptr<AudioDestinationNodeHostObject> destination_;
  std::shared_ptr<AudioListenerHostObject> listener_;
};
} // namespace audioapi
