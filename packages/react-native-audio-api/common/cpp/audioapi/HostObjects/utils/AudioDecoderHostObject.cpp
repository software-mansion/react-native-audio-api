#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/HostObjects/utils/AudioDecodeStreamHostObject.h>
#include <audioapi/HostObjects/utils/AudioDecoderHostObject.h>
#include <audioapi/decoding/AudioDecodeStream.h>
#include <audioapi/decoding/AudioDecoding.h>
#include <audioapi/jsi/JsiPromise.h>

#include <jsi/jsi.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace audioapi {
AudioDecoderHostObject::AudioDecoderHostObject(
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker)
    : callInvoker_(callInvoker) {
  promiseVendor_ = std::make_shared<PromiseVendor>(runtime, callInvoker);
  addFunctions(
      JSI_EXPORT_FUNCTION(AudioDecoderHostObject, decodeWithPCMInBase64),
      JSI_EXPORT_FUNCTION(AudioDecoderHostObject, decodeWithFilePath),
      JSI_EXPORT_FUNCTION(AudioDecoderHostObject, decodeWithMemoryBlock),
      JSI_EXPORT_FUNCTION(AudioDecoderHostObject, openStreamWithFilePath),
      JSI_EXPORT_FUNCTION(AudioDecoderHostObject, openStreamWithMemoryBlock));
}

namespace {

PromiseResolver rejectWithMessage(std::string message) {
  return [message = std::move(message)](jsi::Runtime &) {
    return message;
  };
}

PromiseResolver resolveWithAudioBuffer(std::shared_ptr<AudioBuffer> buffer) {
  return [buffer = std::move(buffer)](jsi::Runtime &runtime) {
    return AudioBufferHostObject::createJsObject(runtime, buffer);
  };
}

// The host object is built on the JS thread, where the resolver runs: its lane must be fed by
// that thread only.
PromiseResolver resolveWithDecodeStream(
    audiodecoding::AudioDecodeStream::OpenResult &&openResult,
    const std::shared_ptr<react::CallInvoker> &callInvoker) {
  if (openResult.is_err()) {
    return rejectWithMessage(openResult.unwrap_err());
  }

  std::shared_ptr<audiodecoding::AudioDecodeStream> stream = std::move(openResult).unwrap();
  return [stream = std::move(stream), callInvoker](jsi::Runtime &runtime) {
    return AudioDecodeStreamHostObject::createJsObject(
        runtime, std::make_shared<AudioDecodeStreamHostObject>(&runtime, callInvoker, stream));
  };
}

} // namespace

JSI_HOST_FUNCTION_IMPL(AudioDecoderHostObject, decodeWithMemoryBlock) {
  auto arrayBuffer =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  const auto *data = arrayBuffer.data(runtime);
  const auto size = arrayBuffer.size(runtime);

  auto sampleRate = static_cast<float>(args[1].getNumber());

  auto promise = promiseVendor_->createAsyncPromise(
      [bytes = std::vector<uint8_t>(data, data + size), sampleRate]() -> PromiseResolver {
        auto result = audiodecoding::decodeWithMemoryBlock(bytes.data(), bytes.size(), sampleRate);

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

JSI_HOST_FUNCTION_IMPL(AudioDecoderHostObject, openStreamWithMemoryBlock) {
  auto arrayBuffer =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  const auto *data = arrayBuffer.data(runtime);
  const auto size = arrayBuffer.size(runtime);
  auto sampleRate = static_cast<float>(args[1].getNumber());

  return promiseVendor_->createAsyncPromise(
      [bytes = std::vector<uint8_t>(data, data + size),
       sampleRate,
       callInvoker = callInvoker_]() -> PromiseResolver {
        return resolveWithDecodeStream(
            audiodecoding::AudioDecodeStream::openWithMemoryBlock(
                bytes.data(), bytes.size(), sampleRate),
            callInvoker);
      });
}

JSI_HOST_FUNCTION_IMPL(AudioDecoderHostObject, openStreamWithFilePath) {
  auto sourcePath = args[0].getString(runtime).utf8(runtime);
  auto sampleRate = static_cast<float>(args[1].getNumber());

  return promiseVendor_->createAsyncPromise(
      [sourcePath, sampleRate, callInvoker = callInvoker_]() -> PromiseResolver {
        return resolveWithDecodeStream(
            audiodecoding::AudioDecodeStream::openWithFilePath(sourcePath, sampleRate),
            callInvoker);
      });
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
