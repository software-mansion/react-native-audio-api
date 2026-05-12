#pragma once

#include <audioapi/jsi/JsiHostObject.h>
#include <audioapi/jsi/JsiPromise.h>

#include <jsi/jsi.h>
#include <memory>

namespace audioapi {
using namespace facebook;

class AudioFileUtilsHostObject : public JsiHostObject {
 public:
  explicit AudioFileUtilsHostObject(
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker);

  JSI_HOST_FUNCTION_DECL(concatAudioFiles);

 private:
  std::shared_ptr<PromiseVendor> promiseVendor_;
};

} // namespace audioapi
