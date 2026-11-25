#include <audioapi/HostObjects/AudioConsumerHostObject.h>
#include <audioapi/HostObjects/inputs/AudioRecorderHostObject.h>

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/core/AudioConsumer.h>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/core/inputs/IRecorderInput.h>
#include <audioapi/core/inputs/RecordingSessionManager.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/jsi/JsiPromise.h>
#include <audioapi/jsi/JsiUtils.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/AudioRecorderOptions.h>
#include <audioapi/utils/Result.hpp>
#include <atomic>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace audioapi {

AudioRecorderHostObject::AudioRecorderHostObject(
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker,
    AudioRecorderOptions options) {
  audioRecorder_ = std::make_shared<AudioRecorder>(
      audioEventHandlerRegistry,
      createPlatformRecorderInput(options),
      RecordingSessionManager::global());

  lifecycleLane_ = std::make_shared<SerialTaskExecutor>();
  promiseVendor_ = std::make_shared<PromiseVendor>(runtime, callInvoker, lifecycleLane_);

  addFunctions(
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, start),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, stop),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, isRecording),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, isPaused),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, enableFileOutput),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, disableFileOutput),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, pause),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, resume),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, addConsumer),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, removeConsumer),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, setOnError),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, clearOnError));

  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(AudioRecorderHostObject, inputLatency),
      JSI_EXPORT_PROPERTY_GETTER(AudioRecorderHostObject, clock));
}

AudioRecorderHostObject::~AudioRecorderHostObject() {
  // Runs on the JS thread while the runtime is still alive. A native caller (a notification
  // action, iOS session deactivation) may hold the recorder past the runtime, and its last
  // release must find no jsi::Function left to destroy.
  audioRecorder_->clearOnErrorCallback();
  // Queued rather than called, so a start() still waiting on the lane cannot outlive this stop.
  lifecycleLane_->schedule([audioRecorder = audioRecorder_] { audioRecorder->stop(); });
}

namespace {

/// The recorder's duration for any runtime: a UI worklet can capture it and read
/// `currentDuration` on the UI thread. It holds only the duration, never the recorder or a JS
/// value, so reading it and destroying it are safe on any thread.
class RecordingClock final : public HostObject {
 public:
  explicit RecordingClock(std::shared_ptr<const std::atomic<double>> recordedSeconds)
      : recordedSeconds_(std::move(recordedSeconds)) {
    addGetters(JSI_EXPORT_PROPERTY_GETTER(RecordingClock, currentDuration));
  }

  JSI_PROPERTY_GETTER_DECL(currentDuration) {
    return {recordedSeconds_->load(std::memory_order_relaxed)};
  }

 private:
  std::shared_ptr<const std::atomic<double>> recordedSeconds_;
};

PromiseResolver resolveWithUndefined() {
  return [](jsi::Runtime &) -> std::variant<jsi::Value, std::string> {
    return jsi::Value::undefined();
  };
}

PromiseResolver rejectWith(std::string message) {
  return [message = std::move(message)](jsi::Runtime &) -> std::variant<jsi::Value, std::string> {
    return message;
  };
}

std::shared_ptr<AudioConsumer> audioConsumerFrom(jsi::Runtime &runtime, const jsi::Value &value) {
  auto hostObject = value.isObject()
      ? value.getObject(runtime).getHostObject<AudioConsumerHostObject>(runtime)
      : nullptr;
  if (hostObject == nullptr) {
    throw jsi::JSError(runtime, "Expected an AudioConsumer");
  }
  return hostObject->consumer();
}

} // namespace

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, start) {
  auto audioRecorder = audioRecorder_;

  return promiseVendor_->createAsyncPromise([audioRecorder]() -> PromiseResolver {
    auto result = audioRecorder->start();

    if (result.is_err()) {
      return rejectWith(result.unwrap_err());
    }

    return resolveWithUndefined();
  });
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, stop) {
  auto audioRecorder = audioRecorder_;

  return promiseVendor_->createAsyncPromise([audioRecorder]() -> PromiseResolver {
    auto result = audioRecorder->stop();

    if (result.is_err()) {
      return rejectWith(result.unwrap_err());
    }

    return [info =
                result.unwrap()](jsi::Runtime &runtime) -> std::variant<jsi::Value, std::string> {
      auto pathsArray = jsi::Array(runtime, info.paths.size());
      for (size_t i = 0; i < info.paths.size(); ++i) {
        pathsArray.setValueAtIndex(runtime, i, jsi::String::createFromUtf8(runtime, info.paths[i]));
      }

      auto fileInfo = jsi::Object(runtime);
      fileInfo.setProperty(runtime, "paths", pathsArray);
      fileInfo.setProperty(runtime, "size", info.size);
      fileInfo.setProperty(runtime, "duration", info.duration);
      return jsi::Value(std::move(fileInfo));
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

  return jsiutils::toJsResult(runtime, audioRecorder_->enableFileOutput(fileProperties));
}

/// Closing a file mid-recording joins the writer thread, so it runs on the lane like stop().
JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, disableFileOutput) {
  auto audioRecorder = audioRecorder_;

  return promiseVendor_->createAsyncPromise([audioRecorder]() -> PromiseResolver {
    audioRecorder->disableFileOutput();
    return resolveWithUndefined();
  });
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, pause) {
  auto audioRecorder = audioRecorder_;

  return promiseVendor_->createAsyncPromise([audioRecorder]() -> PromiseResolver {
    audioRecorder->pause();
    return resolveWithUndefined();
  });
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, resume) {
  auto audioRecorder = audioRecorder_;

  return promiseVendor_->createAsyncPromise([audioRecorder]() -> PromiseResolver {
    auto result = audioRecorder->resume();

    if (result.is_err()) {
      return rejectWith(result.unwrap_err());
    }

    return resolveWithUndefined();
  });
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, addConsumer) {
  return jsiutils::toJsResult(
      runtime, audioRecorder_->addConsumer(audioConsumerFrom(runtime, args[0])));
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, removeConsumer) {
  audioRecorder_->removeConsumer(audioConsumerFrom(runtime, args[0]));
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

JSI_PROPERTY_GETTER_IMPL(AudioRecorderHostObject, inputLatency) {
  return {audioRecorder_->getInputLatency()};
}

JSI_PROPERTY_GETTER_IMPL(AudioRecorderHostObject, clock) {
  return jsi::Object::createFromHostObject(
      runtime, std::make_shared<RecordingClock>(audioRecorder_->getDurationClock()));
}

} // namespace audioapi
