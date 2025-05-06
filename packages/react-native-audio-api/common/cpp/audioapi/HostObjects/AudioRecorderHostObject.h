#pragma once

#include <jsi/jsi.h>

#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/HostObjects/AudioBufferHostObject.h>
#include <audioapi/core/inputs/AudioRecorder.h>

#ifdef ANDROID
#include <audioapi/android/core/AndroidAudioRecorder.h>
#else
#include <audioapi/ios/core/IOSAudioRecorder.h>
#endif

#include <memory>
#include <utility>
#include <vector>
#include <cstdio>

namespace audioapi {
using namespace facebook;

class AudioRecorderHostObject : public JsiHostObject {
 public:
  explicit AudioRecorderHostObject(
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker,
      float sampleRate,
      int bufferLength)
      : callInvoker_(callInvoker) {
#ifdef ANDROID
    audioRecorder_ = std::make_shared<AndroidAudioRecorder>(
      sampleRate,
      bufferLength,
      this->getOnAudioReady()
    );
#else
  audioRecorder_ = std::make_shared<IOSAudioRecorder>(
      sampleRate,
      bufferLength,
      this->getOnAudioReady()
    );
#endif

    addFunctions(
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, start),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, stop),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, onAudioReady));
  }

  ~AudioRecorderHostObject() override {
    audioReadyCallback_ = nullptr;
  }

  JSI_HOST_FUNCTION(start) {
    audioRecorder_->start();

    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(stop) {
    audioRecorder_->stop();

    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(onAudioReady) {
    audioReadyCallback_ = std::make_unique<jsi::Function>(args[0].getObject(runtime).getFunction(runtime));

    return jsi::Value::undefined();
  }

 protected:
  std::shared_ptr<AudioRecorder> audioRecorder_;
  std::shared_ptr<react::CallInvoker> callInvoker_;

  std::unique_ptr<jsi::Function> audioReadyCallback_;

  std::function<void(std::shared_ptr<AudioBus>, int, double)> getOnAudioReady() {
    return [this](const std::shared_ptr<AudioBus> &bus, int numFrames, double when) {
      if (audioReadyCallback_ == nullptr) {
        return;
      }

      callInvoker_->invokeAsync([this, bus = bus, numFrames, when](jsi::Runtime &runtime) {
        auto buffer = std::make_shared<AudioBuffer>(bus);
        auto bufferHostObject = std::make_shared<AudioBufferHostObject>(buffer);

        audioReadyCallback_->call(
          runtime,
          jsi::Object::createFromHostObject(runtime, bufferHostObject),
          jsi::Value(numFrames),
          jsi::Value(when)
        );
      });
    };
  }
};

} // namespace audioapi
