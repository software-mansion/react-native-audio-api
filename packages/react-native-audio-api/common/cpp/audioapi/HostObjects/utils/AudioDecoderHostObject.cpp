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
    const std::shared_ptr<react::CallInvoker> &callInvoker,
    float sampleRate) {
  promiseVendor_ = std::make_shared<PromiseVendor>(runtime, callInvoker);
  decoder_ = std::make_shared<AudioDecoder>(sampleRate);
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

  auto promise = promiseVendor_->createPromise(
      [this, data, size](std::shared_ptr<Promise> promise) {
        std::thread([this, data, size, promise = std::move(promise)]() {
          auto result = decoder_->decodeWithMemoryBlock(data, size);

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

  auto promise = promiseVendor_->createPromise(
      [this, sourcePath](std::shared_ptr<Promise> promise) {
        std::thread([this, sourcePath, promise = std::move(promise)]() {
          auto result = decoder_->decodeWithFilePath(sourcePath);

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

  auto promise = promiseVendor_->createPromise(
      [this, b64](std::shared_ptr<Promise> promise) {
        std::thread([this, b64, promise = std::move(promise)]() {
          auto result = decoder_->decodeWithPCMInBase64(b64);

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
