#include <audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h>

namespace audioapi {

JSI_PROPERTY_SETTER_IMPL(AudioScheduledSourceNodeHostObject, onEnded) {
  auto audioScheduleSourceNode =
          std::static_pointer_cast<AudioScheduledSourceNode>(node_);

  audioScheduleSourceNode->setOnEndedCallbackId(std::stoull(value.getString(runtime).utf8(runtime)));
}

JSI_HOST_FUNCTION_IMPL(AudioScheduledSourceNodeHostObject, start) {
  auto when = args[0].getNumber();
  auto audioScheduleSourceNode =
      std::static_pointer_cast<AudioScheduledSourceNode>(node_);
  audioScheduleSourceNode->start(when);
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioScheduledSourceNodeHostObject, stop) {
  auto time = args[0].getNumber();
  auto audioScheduleSourceNode =
      std::static_pointer_cast<AudioScheduledSourceNode>(node_);
  audioScheduleSourceNode->stop(time);
  return jsi::Value::undefined();
}

} // namespace audioapi