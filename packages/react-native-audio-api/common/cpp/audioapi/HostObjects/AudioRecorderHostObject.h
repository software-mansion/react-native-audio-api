#pragma once

#include <jsi/jsi.h>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi {
using namespace facebook;

class AudioRecorderHostObject : public JsiHostObject {
 public:
  explicit AudioRecorderHostObject(
      const std::shared_ptr<AudioRecorder> &audioRecorder,
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker)
      : audioRecorder_(audioRecorder), callInvoker_(callInvoker) {
    promiseVendor_ = std::make_shared<PromiseVendor>(runtime, callInvoker);

    addFunctions(
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, start),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, stop));
  }

  JSI_HOST_FUNCTION(start) {
    audioRecorder_->start();
    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(stop) {
    audioRecorder_->stop();
    return jsi::Value::undefined();
  }

 protected:
  std::shared_ptr<AudioRecorder> audioRecorder_;
  std::shared_ptr<PromiseVendor> promiseVendor_;
  std::shared_ptr<react::CallInvoker> callInvoker_;
};

} // namespace audioapi
