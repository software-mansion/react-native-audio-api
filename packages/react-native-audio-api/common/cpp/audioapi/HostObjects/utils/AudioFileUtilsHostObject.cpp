#include <audioapi/HostObjects/utils/AudioFileUtilsHostObject.h>
#include <audioapi/HostObjects/utils/NodeOptionsParser.h>
#include <audioapi/core/utils/AudioDecoding.h>
#include <audioapi/core/utils/AudioFileConcatenator.h>
#include <audioapi/jsi/JsiPromise.h>

#include <jsi/jsi.h>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace audioapi {

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
  if (count < 1) {
    throw jsi::JSError(runtime, "probeDuration expects an ArrayBuffer or URL/path string.");
  }

  const auto sampleRate =
      count > 1 && args[1].isNumber() ? static_cast<int>(args[1].getNumber()) : 0;

  if (args[0].isObject() && args[0].asObject(runtime).isArrayBuffer(runtime)) {
    auto arrayBuffer = args[0].asObject(runtime).getArrayBuffer(runtime);
    const auto *data = arrayBuffer.data(runtime);
    const auto size = arrayBuffer.size(runtime);

    auto promise = promiseVendor_->createAsyncPromise(
        [bytes = std::vector<uint8_t>(data, data + size), sampleRate]() -> PromiseResolver {
          auto result =
              audiodecoding::probeDurationWithMemory(bytes.data(), bytes.size(), sampleRate);
          if (result.is_err()) {
            return [](jsi::Runtime &runtime) -> std::variant<jsi::Value, std::string> {
              return jsi::Value::null();
            };
          }

          return [duration = static_cast<double>(result.unwrap())](
                     jsi::Runtime &runtime) -> std::variant<jsi::Value, std::string> {
            return jsi::Value(duration);
          };
        });

    return promise;
  }

  if (args[0].isString()) {
    const auto pathOrUrl = args[0].asString(runtime).utf8(runtime);

    std::map<std::string, std::string> headers;
    if (count > 2 && args[2].isObject()) {
      headers = audioapi::option_parser::parseHttpHeaders(runtime, args[2].asObject(runtime));
    }

    auto promise = promiseVendor_->createAsyncPromise(
        [pathOrUrl, sampleRate, headers = std::move(headers)]() -> PromiseResolver {
          auto result = audiodecoding::isHttpUrl(pathOrUrl)
              ? audiodecoding::probeDurationWithUrl(pathOrUrl, sampleRate, headers)
              : audiodecoding::probeDurationWithFilePath(pathOrUrl);

          if (result.is_err()) {
            return [](jsi::Runtime &runtime) -> std::variant<jsi::Value, std::string> {
              return jsi::Value::null();
            };
          }

          return [duration = static_cast<double>(result.unwrap())](
                     jsi::Runtime &runtime) -> std::variant<jsi::Value, std::string> {
            return jsi::Value(duration);
          };
        });

    return promise;
  }

  throw jsi::JSError(runtime, "probeDuration expects an ArrayBuffer or URL/path string.");
}

} // namespace audioapi
