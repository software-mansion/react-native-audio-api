#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>

#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

class AudioScheduledSourceNodeHostObject : public AudioNodeHostObject {
 public:
  explicit AudioScheduledSourceNodeHostObject(
      const std::shared_ptr<AudioScheduledSourceNode> &node,
      const std::shared_ptr<react::CallInvoker> &callInvoker)
      : AudioNodeHostObject(node), callInvoker_(callInvoker) {
    addSetters(
      JSI_EXPORT_PROPERTY_SETTER(AudioScheduledSourceNodeHostObject, onended));
    addFunctions(
        JSI_EXPORT_FUNCTION(AudioScheduledSourceNodeHostObject, start),
        JSI_EXPORT_FUNCTION(AudioScheduledSourceNodeHostObject, stop));
  }

  ~AudioScheduledSourceNodeHostObject() override {
    // https://github.com/software-mansion/react-native-reanimated/blob/2b669b069bd70ba876d97a9e19daa509808a1b3a/packages/react-native-worklets/Common/cpp/worklets/SharedItems/Shareables.h#L39-L63
    auto audioScheduleSourceNode =
            std::static_pointer_cast<AudioScheduledSourceNode>(node_);
    audioScheduleSourceNode->setOnendedCallback(nullptr);

    onendedCallback_ = nullptr;
  }

  JSI_PROPERTY_SETTER(onended) {
      onendedCallback_ = std::make_unique<jsi::Function>(value.getObject(runtime).getFunction(runtime));
    auto audioScheduleSourceNode =
            std::static_pointer_cast<AudioScheduledSourceNode>(node_);

    auto lambda = [this, &runtime](double stopTime) {
        callInvoker_->invokeAsync([this, &runtime, stopTime] () {
            onendedCallback_->call(runtime, stopTime);
        });
    };

    audioScheduleSourceNode->setOnendedCallback(lambda);
  }

  JSI_HOST_FUNCTION(start) {
    auto when = args[0].getNumber();
    auto audioScheduleSourceNode =
        std::static_pointer_cast<AudioScheduledSourceNode>(node_);
    audioScheduleSourceNode->start(when);
    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(stop) {
    auto time = args[0].getNumber();
    auto audioScheduleSourceNode =
        std::static_pointer_cast<AudioScheduledSourceNode>(node_);
    audioScheduleSourceNode->stop(time);
    return jsi::Value::undefined();
  }

 private:
    std::shared_ptr<react::CallInvoker> callInvoker_;
    std::unique_ptr<jsi::Function> onendedCallback_;
};
} // namespace audioapi
