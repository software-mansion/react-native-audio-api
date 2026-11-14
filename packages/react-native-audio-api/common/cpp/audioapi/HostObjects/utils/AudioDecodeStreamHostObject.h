#pragma once

#include <audioapi/decoding/AudioDecodeStream.h>
#include <audioapi/jsi/HostObject.h>
#include <audioapi/jsi/JsiPromise.h>
#include <audioapi/utils/SerialTaskExecutor.hpp>

#include <jsi/jsi.h>
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

  /// The JS object for @p hostObject, reporting the encoded bytes the stream keeps in memory
  /// as external memory pressure.
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
  std::shared_ptr<audiodecoding::AudioDecodeStream> stream_;
  std::shared_ptr<SerialTaskExecutor> lane_;
  std::shared_ptr<PromiseVendor> promiseVendor_;
};
} // namespace audioapi
