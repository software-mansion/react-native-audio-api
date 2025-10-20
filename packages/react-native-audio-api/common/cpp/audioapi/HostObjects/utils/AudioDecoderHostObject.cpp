#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/HostObjects/utils/AudioDecoderHostObject.h>
#include <audioapi/core/utils/AudioDecoder.h>
#include <audioapi/jsi/JsiPromise.h>

#include <jsi/jsi.h>
#include <memory>
#include <string>
#include <thread>
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

JSI_HOST_FUNCTION_IMPL(AudioDecoderHostObject, decodeWithMemoryBlock) {
  auto arrayBuffer = args[0]
                         .getObject(runtime)
                         .getPropertyAsObject(runtime, "buffer")
                         .getArrayBuffer(runtime);
  auto data = arrayBuffer.data(runtime);
  auto size = static_cast<int>(arrayBuffer.size(runtime));

  auto sampleRate = args[1].getNumber();

  auto promise = promiseVendor_->createAsyncPromise(
      [data, size, sampleRate]() -> PromiseResolver {
        auto result =
            AudioDecoder::decodeWithMemoryBlock(data, size, sampleRate);

        if (!result) {
          return [](jsi::Runtime &runtime)
                     -> std::variant<jsi::Value, std::string> {
            return std::string("Failed to decode audio data.");
          };
        }

        auto audioBufferHostObject =
            std::make_shared<AudioBufferHostObject>(result);

        return [audioBufferHostObject =
                    std::move(audioBufferHostObject)](jsi::Runtime &runtime)
                   -> std::variant<jsi::Value, std::string> {
          auto jsiObject =
              jsi::Object::createFromHostObject(runtime, audioBufferHostObject);
          jsiObject.setExternalMemoryPressure(
              runtime, audioBufferHostObject->getSizeInBytes());
          return jsiObject;
        };
      });
  return promise;
}

JSI_HOST_FUNCTION_IMPL(AudioDecoderHostObject, decodeWithFilePath) {
  auto sourcePath = args[0].getString(runtime).utf8(runtime);
  auto sampleRate = args[1].getNumber();

  auto promise = promiseVendor_->createAsyncPromise(
      [sourcePath, sampleRate]() -> PromiseResolver {
        auto result = AudioDecoder::decodeWithFilePath(sourcePath, sampleRate);

        if (!result) {
          return [](jsi::Runtime &runtime)
                     -> std::variant<jsi::Value, std::string> {
            return std::string("Failed to decode audio data source.");
          };
        }

        auto audioBufferHostObject =
            std::make_shared<AudioBufferHostObject>(result);

        return [audioBufferHostObject =
                    std::move(audioBufferHostObject)](jsi::Runtime &runtime)
                   -> std::variant<jsi::Value, std::string> {
          auto jsiObject =
              jsi::Object::createFromHostObject(runtime, audioBufferHostObject);
          jsiObject.setExternalMemoryPressure(
              runtime, audioBufferHostObject->getSizeInBytes());
          return jsiObject;
        };
      });

  return promise;
}

JSI_HOST_FUNCTION_IMPL(AudioDecoderHostObject, decodeWithPCMInBase64) {
  auto b64 = args[0].getString(runtime).utf8(runtime);
  auto inputSampleRate = args[1].getNumber();
  auto inputChannelCount = args[2].getNumber();
  auto interleaved = args[3].getBool();

  auto promise = promiseVendor_->createAsyncPromise(
      [b64, inputSampleRate, inputChannelCount, interleaved]()
          -> PromiseResolver {
        auto result = AudioDecoder::decodeWithPCMInBase64(
            b64, inputSampleRate, inputChannelCount, interleaved);

        if (!result) {
          return [](jsi::Runtime &runtime)
                     -> std::variant<jsi::Value, std::string> {
            return std::string("Failed to decode audio data source.");
          };
        }

        auto audioBufferHostObject =
            std::make_shared<AudioBufferHostObject>(result);

        return [audioBufferHostObject =
                    std::move(audioBufferHostObject)](jsi::Runtime &runtime)
                   -> std::variant<jsi::Value, std::string> {
          auto jsiObject =
              jsi::Object::createFromHostObject(runtime, audioBufferHostObject);
          jsiObject.setExternalMemoryPressure(
              runtime, audioBufferHostObject->getSizeInBytes());
          return jsiObject;
        };
      });

  return promise;
}

} // namespace audioapi
