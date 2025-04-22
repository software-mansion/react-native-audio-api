#pragma once

#include <jsi/jsi.h>

#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/HostObjects/AudioBufferHostObject.h>

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
      int numberOfChannels,
      int bufferLength,
      bool enableVoiceProcessing)
      : callInvoker_(callInvoker) {
    promiseVendor_ = std::make_shared<PromiseVendor>(runtime, callInvoker);

    audioRecorder_ = std::make_shared<AudioRecorder>(
      sampleRate,
      numberOfChannels,
      bufferLength,
      enableVoiceProcessing,
      this->getOnError(),
      this->getOnStatusChange(),
      this->getOnAudioReady()
    );

    addFunctions(
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, start),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, stop),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, onAudioReady),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, onError),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, onStatusChange));
  }

  ~AudioRecorderHostObject() override {
    errorCallback_ = nullptr;
    audioReadyCallback_ = nullptr;
    statusChangeCallback_ = nullptr;
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

  JSI_HOST_FUNCTION(onError) {
    errorCallback_ = std::make_unique<jsi::Function>(args[0].getObject(runtime).getFunction(runtime));

    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(onStatusChange) {
    statusChangeCallback_ = std::make_unique<jsi::Function>(args[0].getObject(runtime).getFunction(runtime));

    return jsi::Value::undefined();
  }

 protected:
  std::shared_ptr<AudioRecorder> audioRecorder_;
  std::shared_ptr<PromiseVendor> promiseVendor_;
  std::shared_ptr<react::CallInvoker> callInvoker_;

  // TODO: can we really assume those are unique ptrs? 🤔
  // JS also has a ref for them
  std::unique_ptr<jsi::Function> errorCallback_;
  std::unique_ptr<jsi::Function> audioReadyCallback_;
  std::unique_ptr<jsi::Function> statusChangeCallback_;

  // this is ugly as duck
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

  std::function<void(void)> getOnError() {
    return [this]() {
      if (errorCallback_ == nullptr) {
        return;
      }

      callInvoker_->invokeAsync([this](jsi::Runtime &runtime) {
        errorCallback_->call(runtime);
      });
    };
  }

  std::function<void(void)> getOnStatusChange() {
    return [this]() {
      if (statusChangeCallback_ == nullptr) {
        return;
      }

      callInvoker_->invokeAsync([this](jsi::Runtime &runtime) {
        statusChangeCallback_->call(runtime);
      });
    };
  }
};

} // namespace audioapi
