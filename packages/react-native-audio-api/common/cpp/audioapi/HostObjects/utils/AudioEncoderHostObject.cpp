#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/HostObjects/utils/AudioEncoderHostObject.h>
#include <audioapi/encoding/AudioEncoding.h>
#include <audioapi/jsi/JsiPromise.h>
#include <audioapi/utils/AudioFileProperties.h>

#include <jsi/jsi.h>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace audioapi {
AudioEncoderHostObject::AudioEncoderHostObject(
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker) {
  promiseVendor_ = std::make_shared<PromiseVendor>(runtime, callInvoker);
  addFunctions(JSI_EXPORT_FUNCTION(AudioEncoderHostObject, encode));
}

JSI_HOST_FUNCTION_IMPL(AudioEncoderHostObject, encode) {
  if (count < 2) {
    throw jsi::JSError(runtime, "encodeAudioData expects AudioBuffers and options.");
  }

  auto bufferArray = args[0].asObject(runtime).asArray(runtime);
  const auto bufferCount = bufferArray.size(runtime);
  std::vector<std::shared_ptr<const AudioBuffer>> buffers;
  buffers.reserve(bufferCount);

  for (size_t i = 0; i < bufferCount; ++i) {
    // A snapshot: JS writes made after this call reach a copy, not the encoder.
    buffers.push_back(bufferArray.getValueAtIndex(runtime, i)
                          .asObject(runtime)
                          .asHostObject<AudioBufferHostObject>(runtime)
                          ->sharedBuffer_->acquireContent());
  }

  auto properties = AudioFileProperties::CreateFromJSIValue(runtime, args[1]);

  auto promise = promiseVendor_->createAsyncPromise(
      [buffers = std::move(buffers), properties = std::move(properties)]() -> PromiseResolver {
        auto result = audioencoding::encodeAudioData(buffers, properties);

        if (result.is_err()) {
          return rejectWith(std::move(result).unwrap_err());
        }

        return [info = std::move(result).unwrap()](
                   jsi::Runtime &runtime) -> std::variant<jsi::Value, std::string> {
          return info.toJSIValue(runtime);
        };
      });

  return promise;
}

} // namespace audioapi
