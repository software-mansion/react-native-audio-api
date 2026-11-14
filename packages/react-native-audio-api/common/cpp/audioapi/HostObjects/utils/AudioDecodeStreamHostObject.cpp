#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/HostObjects/utils/AudioDecodeStreamHostObject.h>
#include <audioapi/jsi/JsiPromise.h>

#include <jsi/jsi.h>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>

namespace audioapi {

AudioDecodeStreamHostObject::AudioDecodeStreamHostObject(
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker,
    std::shared_ptr<audiodecoding::AudioDecodeStream> stream)
    : stream_(std::move(stream)),
      lane_(std::make_shared<SerialTaskExecutor>()),
      promiseVendor_(std::make_shared<PromiseVendor>(runtime, callInvoker, lane_)) {
  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(AudioDecodeStreamHostObject, sampleRate),
      JSI_EXPORT_PROPERTY_GETTER(AudioDecodeStreamHostObject, numberOfChannels),
      JSI_EXPORT_PROPERTY_GETTER(AudioDecodeStreamHostObject, duration));
  addFunctions(
      JSI_EXPORT_FUNCTION(AudioDecodeStreamHostObject, read),
      JSI_EXPORT_FUNCTION(AudioDecodeStreamHostObject, seek),
      JSI_EXPORT_FUNCTION(AudioDecodeStreamHostObject, close));
}

jsi::Object AudioDecodeStreamHostObject::createJsObject(
    jsi::Runtime &runtime,
    const std::shared_ptr<AudioDecodeStreamHostObject> &hostObject) {
  auto object = jsi::Object::createFromHostObject(runtime, hostObject);
  object.setExternalMemoryPressure(runtime, hostObject->stream_->encodedSizeInBytes());
  return object;
}

namespace {

PromiseResolver rejectWithMessage(std::string message) {
  return [message = std::move(message)](jsi::Runtime &) {
    return message;
  };
}

} // namespace

JSI_PROPERTY_GETTER_IMPL(AudioDecodeStreamHostObject, sampleRate) {
  return {stream_->sampleRate()};
}

JSI_PROPERTY_GETTER_IMPL(AudioDecodeStreamHostObject, numberOfChannels) {
  return {stream_->numberOfChannels()};
}

JSI_PROPERTY_GETTER_IMPL(AudioDecodeStreamHostObject, duration) {
  return {stream_->duration()};
}

JSI_HOST_FUNCTION_IMPL(AudioDecodeStreamHostObject, read) {
  const auto frameCount = static_cast<size_t>(args[0].getNumber());

  return promiseVendor_->createAsyncPromise([stream = stream_, frameCount]() -> PromiseResolver {
    auto result = stream->read(frameCount);
    if (result.is_err()) {
      return rejectWithMessage(result.unwrap_err());
    }

    return [buffer = result.unwrap()](jsi::Runtime &runtime) -> jsi::Value {
      if (buffer == nullptr) {
        return jsi::Value::null();
      }
      return AudioBufferHostObject::createJsObject(runtime, buffer);
    };
  });
}

JSI_HOST_FUNCTION_IMPL(AudioDecodeStreamHostObject, seek) {
  const auto seconds = args[0].getNumber();

  return promiseVendor_->createAsyncPromise([stream = stream_, seconds]() -> PromiseResolver {
    auto result = stream->seek(seconds);
    if (result.is_err()) {
      return rejectWithMessage(result.unwrap_err());
    }

    return [](jsi::Runtime &) {
      return jsi::Value::undefined();
    };
  });
}

JSI_HOST_FUNCTION_IMPL(AudioDecodeStreamHostObject, close) {
  lane_->schedule([stream = stream_]() { stream->close(); });
  return jsi::Value::undefined();
}

} // namespace audioapi
