#include <audioapi/HostObjects/inputs/AudioRecorderHostObject.h>

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/HostObjects/sources/RecorderAdapterNodeHostObject.h>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/jsi/JsiUtils.h>
#ifdef ANDROID
#include <audioapi/android/core/AndroidAudioRecorder.h>
#else
#include <audioapi/ios/core/IOSAudioRecorder.h>
#endif
#include <memory>

namespace audioapi {

AudioRecorderHostObject::AudioRecorderHostObject(
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry) {
#ifdef ANDROID
  audioRecorder_ = std::make_shared<AndroidAudioRecorder>(audioEventHandlerRegistry);
#else
  audioRecorder_ = std::make_shared<IOSAudioRecorder>(audioEventHandlerRegistry);
#endif

  addFunctions(
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, start),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, stop),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, isRecording),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, enableFileOutput),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, disableFileOutput),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, pause),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, resume),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, connect),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, disconnect),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, setOnAudioReady),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, clearOnAudioReady),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, setOnError),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, clearOnError),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, getCurrentDuration));
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, start) {
  auto fileNameOverride = jsiutils::argToString(runtime, args, count, 0, "");
  auto result = audioRecorder_->start(fileNameOverride);
  auto jsResult = jsi::Object(runtime);

  jsResult.setProperty(
      runtime,
      "status",
      jsi::String::createFromUtf8(runtime, result.is_ok() ? "success" : "error"));

  if (result.is_ok()) {
    jsResult.setProperty(runtime, "path", jsi::String::createFromUtf8(runtime, result.unwrap()));
  } else {
    jsResult.setProperty(
        runtime, "message", jsi::String::createFromUtf8(runtime, result.unwrap_err()));
  }

  return jsResult;
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, stop) {
  auto result = audioRecorder_->stop();
  auto jsResult = jsi::Object(runtime);

  jsResult.setProperty(
      runtime,
      "status",
      jsi::String::createFromUtf8(runtime, result.is_ok() ? "success" : "error"));

  if (result.is_ok()) {
    auto info = result.unwrap();

    jsResult.setProperty(runtime, "path", jsi::String::createFromUtf8(runtime, std::get<0>(info)));
    jsResult.setProperty(runtime, "size", std::get<1>(info));
    jsResult.setProperty(runtime, "duration", std::get<2>(info));
  } else {
    jsResult.setProperty(
        runtime, "message", jsi::String::createFromUtf8(runtime, result.unwrap_err()));
  }

  return jsResult;
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, isRecording) {
  return jsi::Value(audioRecorder_->isRecording());
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, isPaused) {
  return jsi::Value(audioRecorder_->isPaused());
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, enableFileOutput) {
  auto fileProperties = AudioFileProperties::CreateFromJSIValue(runtime, args[0]);

  auto result = audioRecorder_->enableFileOutput(fileProperties);
  auto jsResult = jsi::Object(runtime);

  jsResult.setProperty(
      runtime,
      "status",
      jsi::String::createFromUtf8(runtime, result.is_ok() ? "success" : "error"));

  if (result.is_ok()) {
    jsResult.setProperty(runtime, "path", jsi::String::createFromUtf8(runtime, result.unwrap()));
  } else {
    jsResult.setProperty(
        runtime, "message", jsi::String::createFromUtf8(runtime, result.unwrap_err()));
  }

  return jsResult;
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, disableFileOutput) {
  audioRecorder_->disableFileOutput();
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, pause) {
  audioRecorder_->pause();

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, resume) {
  audioRecorder_->resume();
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, connect) {
  auto adapterNodeHostObject =
      args[0].getObject(runtime).getHostObject<RecorderAdapterNodeHostObject>(runtime);

  audioRecorder_->connect(
      std::static_pointer_cast<RecorderAdapterNode>(adapterNodeHostObject->node_));
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, disconnect) {
  audioRecorder_->disconnect();

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, setOnAudioReady) {
  auto options = args[0].getObject(runtime);

  auto sampleRate = static_cast<float>(options.getProperty(runtime, "sampleRate").getNumber());
  auto bufferLength = static_cast<size_t>(options.getProperty(runtime, "bufferLength").getNumber());
  auto channelCount = static_cast<int>(options.getProperty(runtime, "channelCount").getNumber());
  uint64_t callbackId =
      std::stoull(options.getProperty(runtime, "callbackId").getString(runtime).utf8(runtime));

  auto result =
      audioRecorder_->setOnAudioReadyCallback(sampleRate, bufferLength, channelCount, callbackId);
  auto jsResult = jsi::Object(runtime);

  jsResult.setProperty(
      runtime,
      "status",
      jsi::String::createFromUtf8(runtime, result.is_ok() ? "success" : "error"));

  if (result.is_err()) {
    jsResult.setProperty(
        runtime, "message", jsi::String::createFromUtf8(runtime, result.unwrap_err()));
  }

  return jsResult;
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, clearOnAudioReady) {
  audioRecorder_->clearOnAudioReadyCallback();
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, setOnError) {
  auto options = args[0].getObject(runtime);

  uint64_t callbackId =
      std::stoull(options.getProperty(runtime, "callbackId").getString(runtime).utf8(runtime));

  audioRecorder_->setOnErrorCallback(callbackId);
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, clearOnError) {
  audioRecorder_->clearOnErrorCallback();
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, getCurrentDuration) {
  double duration = audioRecorder_->getCurrentDuration();
  return jsi::Value(duration);
}

} // namespace audioapi
