#pragma once

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/jsi/JsiPromise.h>

#include <jsi/jsi.h>
#include <memory>

namespace audioapi {
using namespace facebook;

class AudioDecoderHostObject : public HostObject {
 public:
  explicit AudioDecoderHostObject(
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker);
  JSI_HOST_FUNCTION_DECL(decodeWithMemoryBlock);
  JSI_HOST_FUNCTION_DECL(decodeWithFilePath);
  JSI_HOST_FUNCTION_DECL(decodeWithPCMInBase64);
  JSI_HOST_FUNCTION_DECL(openStreamWithMemoryBlock);
  JSI_HOST_FUNCTION_DECL(openStreamWithFilePath);

 private:
  std::shared_ptr<react::CallInvoker> callInvoker_;
  std::shared_ptr<PromiseVendor> promiseVendor_;
};
} // namespace audioapi
