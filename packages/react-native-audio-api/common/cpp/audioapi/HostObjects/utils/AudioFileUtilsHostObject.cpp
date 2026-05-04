#include <audioapi/HostObjects/utils/AudioFileUtilsHostObject.h>
#include <audioapi/core/utils/AudioFileConcatenator.h>
#include <audioapi/jsi/JsiPromise.h>

#include <jsi/jsi.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace audioapi {

AudioFileUtilsHostObject::AudioFileUtilsHostObject(
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker) {
  promiseVendor_ = std::make_shared<PromiseVendor>(runtime, callInvoker);
  addFunctions(JSI_EXPORT_FUNCTION(AudioFileUtilsHostObject, concatAudioFiles));
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

} // namespace audioapi
