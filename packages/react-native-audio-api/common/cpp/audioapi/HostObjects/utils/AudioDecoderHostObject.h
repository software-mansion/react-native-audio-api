#pragma once

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/core/utils/AudioDecoder.h>
#include <audioapi/jsi/JsiPromise.h>

#include <jsi/jsi.h>
#include <memory>
#include <string>
#include <thread>
#include <utility>

namespace audioapi {
using namespace facebook;

class AudioDecoderHostObject : public JsiHostObject {
 public:
  explicit AudioDecoderHostObject(
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker);
  JSI_HOST_FUNCTION_DECL(decodeWithMemoryBlock);
  JSI_HOST_FUNCTION_DECL(decodeWithFilePath);
  JSI_HOST_FUNCTION_DECL(decodeWithPCMInBase64);

 private:
  std::shared_ptr<PromiseVendor> promiseVendor_;
};
} // namespace audioapi
