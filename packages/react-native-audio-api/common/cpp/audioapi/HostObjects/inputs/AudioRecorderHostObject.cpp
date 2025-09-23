#include <audioapi/HostObjects/inputs/AudioRecorderHostObject.h>

namespace audioapi {

JSI_PROPERTY_SETTER_IMPL(AudioRecorderHostObject, onAudioReady) {
    audioRecorder_->setOnAudioReadyCallbackId(std::stoull(value.getString(runtime).utf8(runtime)));
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, connect) {
  auto adapterNodeHostObject = args[0].getObject(runtime).getHostObject<RecorderAdapterNodeHostObject>(runtime);
  audioRecorder_->connect(
      std::static_pointer_cast<RecorderAdapterNode>(adapterNodeHostObject->node_));
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, disconnect) {
  audioRecorder_->disconnect();
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, start) {
  audioRecorder_->start();

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, stop) {
  audioRecorder_->stop();

  return jsi::Value::undefined();
}

} // namespace audioapi
