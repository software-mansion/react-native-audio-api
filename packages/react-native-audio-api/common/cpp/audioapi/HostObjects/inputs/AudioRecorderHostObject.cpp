#include <audioapi/HostObjects/inputs/AudioRecorderHostObject.h>

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/HostObjects/sources/RecorderAdapterNodeHostObject.h>
#include <audioapi/core/inputs/ActiveRecorderHandle.h>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/jsi/JsiPromise.h>
#include <audioapi/jsi/JsiUtils.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/AudioRecorderOptions.h>
#include <audioapi/utils/Result.hpp>
#include <audioapi/utils/SerialTaskExecutor.hpp>
#include <memory>
#include <string>
#include <utility>

namespace audioapi {

AudioRecorderHostObject::AudioRecorderHostObject(
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker,
    AudioRecorderOptions options) {
  audioRecorder_ = createPlatformAudioRecorder(audioEventHandlerRegistry, options);

  // Serial executor, so start/stop/pause/resume/disableFileOutput run in the order JS called
  // them even when the caller does not await. The remaining methods run on the JS thread
  // and are excluded against lane work by the recorder's per-output mutexes.
  promiseVendor_ =
      std::make_shared<PromiseVendor>(runtime, callInvoker, std::make_shared<SerialTaskExecutor>());

  addFunctions(
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, start),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, stop),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, isRecording),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, isPaused),
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

  addGetters(JSI_EXPORT_PROPERTY_GETTER(AudioRecorderHostObject, inputLatency));
}

AudioRecorderHostObject::~AudioRecorderHostObject() {
  // Queued rather than called, so a start() still waiting on the lane cannot outlive this
  // stop. promiseVendor_ is destroyed right after this body and drains the queue before
  // joining its worker, which keeps the blocking-stop contract of the previous direct call.
  promiseVendor_->scheduleDetached([audioRecorder = audioRecorder_]() {
    ActiveRecorderHandle::global().stopAndReturnInfo(audioRecorder);
  });
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, start) {
  auto audioRecorder = audioRecorder_;

  return promiseVendor_->createAsyncPromise([audioRecorder]() -> PromiseResolver {
    auto result = ActiveRecorderHandle::global().tryStart(audioRecorder);

    return [result =
                std::move(result)](jsi::Runtime &runtime) -> std::variant<jsi::Value, std::string> {
      auto jsResult = jsi::Object(runtime);

      jsResult.setProperty(
          runtime,
          "status",
          jsi::String::createFromUtf8(runtime, result.is_ok() ? "success" : "error"));

      if (!result.is_ok()) {
        jsResult.setProperty(
            runtime, "message", jsi::String::createFromUtf8(runtime, result.unwrap_err()));
      }

      return jsi::Value(std::move(jsResult));
    };
  });
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, stop) {
  auto audioRecorder = audioRecorder_;

  return promiseVendor_->createAsyncPromise([audioRecorder]() -> PromiseResolver {
    auto result = ActiveRecorderHandle::global().stopAndReturnInfo(audioRecorder);

    using returnValue = std::variant<jsi::Value, std::string>;

    return [result = std::move(result)](jsi::Runtime &runtime) -> returnValue {
      auto jsResult = jsi::Object(runtime);

      jsResult.setProperty(
          runtime,
          "status",
          jsi::String::createFromUtf8(runtime, result.is_ok() ? "success" : "error"));

      if (result.is_ok()) {
        const auto &info = result.unwrap();
        auto pathsArray = jsi::Array(runtime, info.paths.size());
        for (size_t i = 0; i < info.paths.size(); ++i) {
          pathsArray.setValueAtIndex(
              runtime, i, jsi::String::createFromUtf8(runtime, info.paths[i]));
        }
        jsResult.setProperty(runtime, "paths", pathsArray);
        jsResult.setProperty(runtime, "size", info.size);
        jsResult.setProperty(runtime, "duration", info.duration);
      } else {
        jsResult.setProperty(
            runtime, "message", jsi::String::createFromUtf8(runtime, result.unwrap_err()));
      }

      return jsi::Value(std::move(jsResult));
    };
  });
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, isRecording) {
  return {audioRecorder_->isRecording()};
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, isPaused) {
  return {audioRecorder_->isPaused()};
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, enableFileOutput) {
  auto fileProperties = AudioFileProperties::CreateFromJSIValue(runtime, args[0]);

  auto result = audioRecorder_->enableFileOutput(fileProperties);
  auto jsResult = jsi::Object(runtime);

  jsResult.setProperty(
      runtime,
      "status",
      jsi::String::createFromUtf8(runtime, result.is_ok() ? "success" : "error"));

  if (!result.is_ok()) {
    jsResult.setProperty(
        runtime, "message", jsi::String::createFromUtf8(runtime, result.unwrap_err()));
  }

  return jsResult;
}

namespace {

PromiseResolver resolveWithUndefined() {
  return [](jsi::Runtime &) -> std::variant<jsi::Value, std::string> {
    return jsi::Value::undefined();
  };
}

} // namespace

/// Closing a file mid-recording joins the writer thread, so it runs on the lane like stop().
JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, disableFileOutput) {
  auto audioRecorder = audioRecorder_;

  return promiseVendor_->createAsyncPromise([audioRecorder]() -> PromiseResolver {
    audioRecorder->disableFileOutput();
    return resolveWithUndefined();
  });
}

/// Through the handle rather than the recorder, so it is excluded against the notification
/// and audio-session paths that also drive the occupant.
JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, pause) {
  auto audioRecorder = audioRecorder_;

  return promiseVendor_->createAsyncPromise([audioRecorder]() -> PromiseResolver {
    ActiveRecorderHandle::global().pause(audioRecorder);
    return resolveWithUndefined();
  });
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, resume) {
  auto audioRecorder = audioRecorder_;

  return promiseVendor_->createAsyncPromise([audioRecorder]() -> PromiseResolver {
    ActiveRecorderHandle::global().resume(audioRecorder);
    return resolveWithUndefined();
  });
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, connect) {
  auto adapterNodeHostObject =
      args[0].getObject(runtime).getHostObject<RecorderAdapterNodeHostObject>(runtime);

  audioRecorder_->connect(adapterNodeHostObject->node_->handle);
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

JSI_PROPERTY_GETTER_IMPL(AudioRecorderHostObject, inputLatency) {
  return jsi::Value(audioRecorder_->getInputLatency());
}

} // namespace audioapi
