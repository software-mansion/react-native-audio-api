#include <audioapi/HostObjects/inputs/AudioRecorderHostObject.h>

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/HostObjects/sources/RecorderAdapterNodeHostObject.h>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#ifdef ANDROID
#include <audioapi/android/core/AndroidAudioRecorder.h>
#else
#include <audioapi/ios/core/IOSAudioRecorder.h>
#endif

namespace audioapi {

AudioRecorderHostObject::AudioRecorderHostObject(
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
    float sampleRate,
    int bufferLength) {
#ifdef ANDROID
  audioRecorder_ = std::make_shared<AndroidAudioRecorder>(
      sampleRate, bufferLength, audioEventHandlerRegistry);
#else
  audioRecorder_ = std::make_shared<IOSAudioRecorder>(
      sampleRate, bufferLength, audioEventHandlerRegistry);
#endif

  addSetters(JSI_EXPORT_PROPERTY_SETTER(AudioRecorderHostObject, onAudioReady));

  addFunctions(
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, start),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, stop),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, connect),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, disconnect));
}

JSI_PROPERTY_SETTER_IMPL(AudioRecorderHostObject, onAudioReady) {
  audioRecorder_->setOnAudioReadyCallbackId(
      std::stoull(value.getString(runtime).utf8(runtime)));
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, connect) {
  auto adapterNodeHostObject =
      args[0].getObject(runtime).getHostObject<RecorderAdapterNodeHostObject>(
          runtime);
  audioRecorder_->connect(
      std::static_pointer_cast<RecorderAdapterNode>(
          adapterNodeHostObject->node_));
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
