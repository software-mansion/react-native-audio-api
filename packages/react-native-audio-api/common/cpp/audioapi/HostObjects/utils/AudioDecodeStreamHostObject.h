#pragma once

#include <audioapi/decoding/AudioDecodeStream.h>
#include <audioapi/jsi/HostObject.h>
#include <audioapi/jsi/JsiPromise.h>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/SerialTaskExecutor.hpp>

#include <jsi/jsi.h>
#include <cstddef>
#include <memory>

namespace audioapi {
using namespace facebook;

/// @brief JS handle on an open `AudioDecodeStream`.
/// @note `read`, `seek` and `close` run on a lane owned by this object, in the order JS called
/// them: the stream is never touched by two threads at once, and a `close` waits for the reads
/// requested before it.
class AudioDecodeStreamHostObject : public HostObject {
 public:
  explicit AudioDecodeStreamHostObject(
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker,
      std::shared_ptr<audiodecoding::AudioDecodeStream> stream);
  /// @note Runs inside the garbage collector, so it waits for nothing: the lane and the
  /// decoder are released on threads of their own.
  ~AudioDecodeStreamHostObject() override;
  DELETE_COPY_AND_MOVE(AudioDecodeStreamHostObject);

  /// The JS object for @p hostObject, reporting what the open stream holds as external memory
  /// pressure, so a stream left open weighs on the collector.
  static jsi::Object createJsObject(
      jsi::Runtime &runtime,
      const std::shared_ptr<AudioDecodeStreamHostObject> &hostObject);

  JSI_PROPERTY_GETTER_DECL(sampleRate);
  JSI_PROPERTY_GETTER_DECL(numberOfChannels);
  JSI_PROPERTY_GETTER_DECL(duration);

  JSI_HOST_FUNCTION_DECL(read);
  JSI_HOST_FUNCTION_DECL(seek);
  JSI_HOST_FUNCTION_DECL(close);

 private:
  /// An open decoder holds a codec, a file descriptor and a thread, which no byte count
  /// reflects: a nominal amount stands for them.
  static constexpr size_t OPEN_DECODER_MEMORY_PRESSURE = 1024 * 1024;
  /// Frame counts above this are requests for "everything left". Clamping them keeps the
  /// conversion from a JS number defined on every ABI.
  static constexpr double MAX_FRAME_COUNT = 2147483647.0;

  std::shared_ptr<audiodecoding::AudioDecodeStream> stream_;
  std::shared_ptr<SerialTaskExecutor> lane_;
  std::shared_ptr<PromiseVendor> promiseVendor_;
};
} // namespace audioapi
