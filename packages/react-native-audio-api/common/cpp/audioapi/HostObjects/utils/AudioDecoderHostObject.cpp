#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/HostObjects/utils/AudioDecoderHostObject.h>
#include <audioapi/decoding/AudioDecoding.h>
#include <audioapi/jsi/JsiPromise.h>

#include <jsi/jsi.h>
#include <memory>
#include <string>
#include <utility>

namespace audioapi {
AudioDecoderHostObject::AudioDecoderHostObject(
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker) {
  promiseVendor_ = std::make_shared<PromiseVendor>(runtime, callInvoker);
  addFunctions(
      JSI_EXPORT_FUNCTION(AudioDecoderHostObject, decodeWithPCMInBase64),
      JSI_EXPORT_FUNCTION(AudioDecoderHostObject, decodeWithFilePath),
      JSI_EXPORT_FUNCTION(AudioDecoderHostObject, decodeWithMemoryBlock));
}

namespace {

PromiseResolver rejectWithMessage(std::string message) {
  return [message = std::move(message)](jsi::Runtime &) {
    return message;
  };
}

/// Wraps @p buffer in an `AudioBuffer` JS object, its sample memory reported to the JS garbage
/// collector.
PromiseResolver resolveWithAudioBuffer(std::shared_ptr<AudioBuffer> buffer) {
  return [buffer = std::move(buffer)](jsi::Runtime &runtime) {
    auto hostObject = std::make_shared<AudioBufferHostObject>(buffer);
    auto object = jsi::Object::createFromHostObject(runtime, hostObject);
    object.setExternalMemoryPressure(runtime, hostObject->getSizeInBytes());
    return object;
  };
}

} // namespace

JSI_HOST_FUNCTION_IMPL(AudioDecoderHostObject, decodeWithMemoryBlock) {
  auto arrayBuffer =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto *data = arrayBuffer.data(runtime);
  auto size = static_cast<int>(arrayBuffer.size(runtime));

  auto sampleRate = static_cast<float>(args[1].getNumber());

  auto promise = promiseVendor_->createAsyncPromise([data, size, sampleRate]() -> PromiseResolver {
    auto result = audiodecoding::decodeWithMemoryBlock(data, size, sampleRate);

    if (result.is_err()) {
      return rejectWithMessage(result.unwrap_err());
    }

    return resolveWithAudioBuffer(result.unwrap());
  });
  return promise;
}

JSI_HOST_FUNCTION_IMPL(AudioDecoderHostObject, decodeWithFilePath) {
  auto sourcePath = args[0].getString(runtime).utf8(runtime);
  auto sampleRate = static_cast<float>(args[1].getNumber());

  auto promise = promiseVendor_->createAsyncPromise([sourcePath, sampleRate]() -> PromiseResolver {
    auto result = audiodecoding::decodeWithFilePath(sourcePath, sampleRate);

    if (result.is_err()) {
      return rejectWithMessage(result.unwrap_err());
    }

    return resolveWithAudioBuffer(result.unwrap());
  });

  return promise;
}

JSI_HOST_FUNCTION_IMPL(AudioDecoderHostObject, decodeWithPCMInBase64) {
  auto b64 = args[0].getString(runtime).utf8(runtime);
  auto inputSampleRate = static_cast<float>(args[1].getNumber());
  auto inputChannelCount = static_cast<int>(args[2].getNumber());
  auto interleaved = args[3].getBool();

  auto promise = promiseVendor_->createAsyncPromise(
      [b64, inputSampleRate, inputChannelCount, interleaved]() -> PromiseResolver {
        auto result = audiodecoding::decodeWithPCMInBase64(
            b64, inputSampleRate, inputChannelCount, interleaved);

        if (result.is_err()) {
          return rejectWithMessage(result.unwrap_err());
        }

        return resolveWithAudioBuffer(result.unwrap());
      });

  return promise;
}

} // namespace audioapi
