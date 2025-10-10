#pragma once

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

  auto promise = promiseVendor_->createPromise(
      [data, size, sampleRate](std::shared_ptr<Promise> promise) {
        std::thread([data, size, sampleRate, promise = std::move(promise)]() {
          auto result =
              AudioDecoder::decodeWithMemoryBlock(data, size, sampleRate);

          if (!result) {
            promise->reject("Failed to decode audio data.");
            return;
          }

          auto audioBufferHostObject =
              std::make_shared<AudioBufferHostObject>(result);

          promise->resolve([audioBufferHostObject = std::move(
                                audioBufferHostObject)](jsi::Runtime &runtime) {
            auto jsiObject = jsi::Object::createFromHostObject(
                runtime, audioBufferHostObject);
            jsiObject.setExternalMemoryPressure(
                runtime, audioBufferHostObject->getSizeInBytes());
            return jsiObject;
          });
        }).detach();
      });
  return promise;
}

JSI_HOST_FUNCTION_IMPL(AudioDecoderHostObject, decodeWithFilePath) {
  auto sourcePath = args[0].getString(runtime).utf8(runtime);
  auto sampleRate = args[1].getNumber();

  auto promise = promiseVendor_->createPromise(
      [sourcePath, sampleRate](std::shared_ptr<Promise> promise) {
        std::thread([sourcePath, sampleRate, promise = std::move(promise)]() {
          auto result =
              AudioDecoder::decodeWithFilePath(sourcePath, sampleRate);

          if (!result) {
            promise->reject("Failed to decode audio data source.");
            return;
          }

          auto audioBufferHostObject =
              std::make_shared<AudioBufferHostObject>(result);

          promise->resolve([audioBufferHostObject = std::move(
                                audioBufferHostObject)](jsi::Runtime &runtime) {
            auto jsiObject = jsi::Object::createFromHostObject(
                runtime, audioBufferHostObject);
            jsiObject.setExternalMemoryPressure(
                runtime, audioBufferHostObject->getSizeInBytes());
            return jsiObject;
          });
        }).detach();
      });

  return promise;
}

JSI_HOST_FUNCTION_IMPL(AudioDecoderHostObject, decodeWithPCMInBase64) {
  auto b64 = args[0].getString(runtime).utf8(runtime);
  auto inputSampleRate = args[1].getNumber();
  auto inputChannelCount = args[2].getNumber();
  auto interleaved = args[3].getBool();

  auto promise = promiseVendor_->createPromise(
      [b64, inputSampleRate, inputChannelCount, interleaved](
          std::shared_ptr<Promise> promise) {
        std::thread([b64,
                     inputSampleRate,
                     inputChannelCount,
                     interleaved,
                     promise = std::move(promise)]() {
          auto result = AudioDecoder::decodeWithPCMInBase64(
              b64, inputSampleRate, inputChannelCount, interleaved);

          if (!result) {
            promise->reject("Failed to decode audio data source.");
            return;
          }

          auto audioBufferHostObject =
              std::make_shared<AudioBufferHostObject>(result);

          promise->resolve([audioBufferHostObject = std::move(
                                audioBufferHostObject)](jsi::Runtime &runtime) {
            auto jsiObject = jsi::Object::createFromHostObject(
                runtime, audioBufferHostObject);
            jsiObject.setExternalMemoryPressure(
                runtime, audioBufferHostObject->getSizeInBytes());
            return jsiObject;
          });
        }).detach();
      });

  return promise;
}

} // namespace audioapi
