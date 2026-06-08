#include <audioapi/HostObjects/utils/AudioFileUtilsHostObject.h>
#include <audioapi/core/utils/AudioFileConcatenator.h>
#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#endif // RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/core/utils/AudioDecoding.hpp>
#include <audioapi/jsi/JsiPromise.h>
#include <audioapi/libs/miniaudio/MiniAudioDecoding.h>

#include <jsi/jsi.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace audioapi {

namespace {

std::optional<double> probeDurationWithDecoder(const uint8_t *data, size_t size, int sampleRate) {
  auto duration =
      audiodecoding::probeDuration<miniaudio_decoder::MiniAudioDecoder>(data, size, sampleRate);
  if (duration.has_value()) {
    return duration;
  }
#if !RN_AUDIO_API_FFMPEG_DISABLED
  duration = audiodecoding::probeDuration<ffmpeg_decoder::FFmpegDecoder>(data, size, sampleRate);
  return duration;
#else
  return std::nullopt;
#endif // RN_AUDIO_API_FFMPEG_DISABLED
}

} // namespace

AudioFileUtilsHostObject::AudioFileUtilsHostObject(
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker) {
  promiseVendor_ = std::make_shared<PromiseVendor>(runtime, callInvoker);
  addFunctions(
      JSI_EXPORT_FUNCTION(AudioFileUtilsHostObject, concatAudioFiles),
      JSI_EXPORT_FUNCTION(AudioFileUtilsHostObject, probeDuration));
}

JSI_HOST_FUNCTION_IMPL(AudioFileUtilsHostObject, concatAudioFiles) {
  if (count < 2 || !args[0].isObject() || !args[1].isString()) {
    throw jsi::JSError(runtime, "concatAudioFiles expects input paths and an output path.");
  }

  auto inputPathArray = args[0].asObject(runtime).asArray(runtime);
  const auto inputPathCount = inputPathArray.size(runtime);
  std::vector<std::string> inputPaths;
  inputPaths.reserve(inputPathCount);

  for (size_t i = 0; i < inputPathCount; ++i) {
    auto value = inputPathArray.getValueAtIndex(runtime, i);
    if (!value.isString()) {
      throw jsi::JSError(runtime, "concatAudioFiles input paths must be strings.");
    }
    inputPaths.push_back(value.asString(runtime).utf8(runtime));
  }

  auto outputPath = args[1].asString(runtime).utf8(runtime);

  auto promise = promiseVendor_->createAsyncPromise(
      [inputPaths = std::move(inputPaths), outputPath]() -> PromiseResolver {
        auto result = audioapi::concatAudioFiles(inputPaths, outputPath);

        if (result.is_err()) {
          return [result = std::move(result)](
                     jsi::Runtime &runtime) -> std::variant<jsi::Value, std::string> {
            return result.unwrap_err();
          };
        }

        return [outputPath = std::move(outputPath)](
                   jsi::Runtime &runtime) -> std::variant<jsi::Value, std::string> {
          return jsi::String::createFromUtf8(runtime, outputPath);
        };
      });

  return promise;
}

JSI_HOST_FUNCTION_IMPL(AudioFileUtilsHostObject, probeDuration) {
  if (count < 1 || !args[0].isObject()) {
    throw jsi::JSError(runtime, "probeDuration expects an ArrayBuffer.");
  }

  auto arrayBufferObject = args[0].asObject(runtime);
  if (!arrayBufferObject.isArrayBuffer(runtime)) {
    throw jsi::JSError(runtime, "probeDuration expects an ArrayBuffer.");
  }

  auto arrayBuffer = arrayBufferObject.getArrayBuffer(runtime);
  const auto *data = arrayBuffer.data(runtime);
  const auto size = arrayBuffer.size(runtime);

  const auto sampleRate =
      count > 1 && args[1].isNumber() ? static_cast<int>(args[1].getNumber()) : 0;

  auto promise = promiseVendor_->createAsyncPromise(
      [bytes = std::vector<uint8_t>(data, data + size), sampleRate]() -> PromiseResolver {
        auto duration = probeDurationWithDecoder(bytes.data(), bytes.size(), sampleRate);
        if (!duration.has_value()) {
          return [](jsi::Runtime &runtime) -> std::variant<jsi::Value, std::string> {
            return jsi::Value::null();
          };
        }

        return [duration = duration.value()](
                   jsi::Runtime &runtime) -> std::variant<jsi::Value, std::string> {
          return jsi::Value(duration);
        };
      });

  return promise;
}

} // namespace audioapi
