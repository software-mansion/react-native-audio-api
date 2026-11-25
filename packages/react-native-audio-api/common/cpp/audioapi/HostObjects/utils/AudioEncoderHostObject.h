#pragma once

#include <audioapi/jsi/HostObject.h>
#include <audioapi/jsi/JsiPromise.h>

#include <jsi/jsi.h>
#include <memory>

namespace audioapi {
using namespace facebook;

class AudioEncoderHostObject : public HostObject {
 public:
  explicit AudioEncoderHostObject(
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker);
  JSI_HOST_FUNCTION_DECL(encode);

 private:
  std::shared_ptr<PromiseVendor> promiseVendor_;
};
} // namespace audioapi
