#include <audioapi/HostObjects/inputs/AudioRecorderHostObject.h>

#include <audioapi/HostObjects/TypedAudioNodePtr.hpp>
#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/HostObjects/sources/RecorderAdapterNodeHostObject.h>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/core/inputs/IRecorderInput.h>
#include <audioapi/core/inputs/RecorderOutput.h>
#include <audioapi/core/inputs/RecordingSessionManager.h>
#include <audioapi/core/utils/AudioRecorderCallback.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/jsi/JsiPromise.h>
#include <audioapi/jsi/JsiUtils.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/AudioRecorderOptions.h>
#include <audioapi/utils/Result.hpp>
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>

namespace audioapi {

AudioRecorderHostObject::AudioRecorderHostObject(
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker,
    AudioRecorderOptions options)
    : audioEventHandlerRegistry_(audioEventHandlerRegistry) {
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
  // Runs on the JS thread while the runtime is still alive. A native caller (a notification
  // action, iOS session deactivation) may hold the recorder past the runtime, and its last
  // release must find no jsi::Function left to destroy.
  audioRecorder_->clearOnErrorCallback();
  for (const auto &output : {audioReadyOutput_, adapterOutput_}) {
    if (output != nullptr) {
      output->invalidate();
    }
  }
  // Queued rather than called, so a start() still waiting on the lane cannot outlive this stop.
  lifecycleLane_->schedule([audioRecorder = audioRecorder_] { audioRecorder->stop(); });
}

namespace {

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

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, connect) {
  auto adapterNodeHostObject =
      args[0].getObject(runtime).getHostObject<RecorderAdapterNodeHostObject>(runtime);

  auto *graphNode = adapterNodeHostObject->node_;
  auto *adapterNode = typedAudioNode<RecorderAdapterNode>(graphNode);
  // Owns the handle, not the node: the graph destroys a removed node only once nothing else
  // holds its handle, so the node stays alive while the recorder lists it.
  auto adapterOutput = std::shared_ptr<RecorderOutput>(graphNode->handle, adapterNode);

  // Only fails mid-session, once the node's context is gone: the JS API has no result to report
  // it in.
  replaceOutput(adapterOutput_, std::move(adapterOutput));
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, disconnect) {
  replaceOutput(adapterOutput_, nullptr);

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioRecorderHostObject, setOnAudioReady) {
  auto options = args[0].getObject(runtime);

  auto sampleRate = static_cast<float>(options.getProperty(runtime, "sampleRate").getNumber());
  auto bufferLength = static_cast<size_t>(options.getProperty(runtime, "bufferLength").getNumber());
  auto channelCount = static_cast<int>(options.getProperty(runtime, "channelCount").getNumber());
  uint64_t callbackId =
      std::stoull(options.getProperty(runtime, "callbackId").getString(runtime).utf8(runtime));

  auto result = replaceOutput(
      audioReadyOutput_,
      std::make_shared<AudioRecorderCallback>(
          audioEventHandlerRegistry_, sampleRate, bufferLength, channelCount, callbackId));
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
  replaceOutput(audioReadyOutput_, nullptr);
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
  return {duration};
}

Result<NoneType, std::string> AudioRecorderHostObject::replaceOutput(
    std::shared_ptr<RecorderOutput> &current,
    std::shared_ptr<RecorderOutput> replacement) {
  if (current != nullptr) {
    audioRecorder_->removeOutput(current);
  }
  current = nullptr;

  if (replacement == nullptr) {
    return Ok(None);
  }

  auto addResult = audioRecorder_->addOutput(replacement);
  if (addResult.is_ok()) {
    current = std::move(replacement);
  }
  return addResult;
}

JSI_PROPERTY_GETTER_IMPL(AudioRecorderHostObject, inputLatency) {
  return {audioRecorder_->getInputLatency()};
}

} // namespace audioapi
