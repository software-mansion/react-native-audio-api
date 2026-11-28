#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/HostObjects/utils/AudioDecodeStreamHostObject.h>
#include <audioapi/jsi/JsiPromise.h>

#include <jsi/jsi.h>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <thread>
#include <utility>

namespace audioapi {

namespace {

/// @brief A lane that is destroyed on a thread of its own. Destroying a `SerialTaskExecutor`
/// waits for the tasks still queued, and its last owner can be the garbage collector: the
/// host object, or a promise executor function JS has not collected yet.
std::shared_ptr<SerialTaskExecutor> makeLane() {
  return {new SerialTaskExecutor(), [](SerialTaskExecutor *lane) {
            std::thread([lane]() { delete lane; }).detach();
          }};
}

} // namespace

AudioDecodeStreamHostObject::AudioDecodeStreamHostObject(
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker,
    std::shared_ptr<audiodecoding::AudioDecodeStream> stream)
    : stream_(std::move(stream)),
      lane_(makeLane()),
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

AudioDecodeStreamHostObject::~AudioDecodeStreamHostObject() {
  // Dropping the last reference to the stream closes its decoder, which can take a codec
  // teardown: not inside the garbage collector. The lane drops it, after the reads still
  // queued, which hold references of their own.
  if (lane_ == nullptr) {
    // Closed: `close` already released the decoder on the lane.
    return;
  }
  lane_->schedule([stream = std::move(stream_)]() mutable { stream.reset(); });
}

jsi::Object AudioDecodeStreamHostObject::createJsObject(
    jsi::Runtime &runtime,
    const std::shared_ptr<AudioDecodeStreamHostObject> &hostObject) {
  auto object = jsi::Object::createFromHostObject(runtime, hostObject);
  object.setExternalMemoryPressure(
      runtime, OPEN_DECODER_MEMORY_PRESSURE + hostObject->stream_->encodedSizeInBytes());
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

// Scheduling on the lane waits when its queue is full, and these run on the JS thread. The
// TypeScript `AudioDecodeStream` forwards one operation at a time, so the queue never holds
// more than that operation and a `close`.
JSI_HOST_FUNCTION_IMPL(AudioDecodeStreamHostObject, read) {
  if (promiseVendor_ == nullptr) {
    throw jsi::JSError(runtime, "Cannot read from a closed decode stream");
  }

  // Zero, which the stream rejects, for anything that is not a positive frame count.
  const double requested = args[0].getNumber();
  const auto frameCount =
      requested >= 1.0 ? static_cast<size_t>(std::min(requested, MAX_FRAME_COUNT)) : size_t{0};

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
  if (promiseVendor_ == nullptr) {
    throw jsi::JSError(runtime, "Cannot seek in a closed decode stream");
  }

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
  if (lane_ == nullptr) {
    return jsi::Value::undefined();
  }

  lane_->schedule([stream = stream_]() { stream->close(); });
  // Nothing runs on the lane after a close, so it is let go now rather than when the
  // collector reaches this object: a closed stream does not keep a worker thread. Its deleter
  // lets the close run, then joins the worker on a thread of its own.
  promiseVendor_.reset();
  lane_.reset();
  return jsi::Value::undefined();
}

} // namespace audioapi
