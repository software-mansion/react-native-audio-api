#pragma once

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/core/utils/AudioStretcher.h>
#include <audioapi/jsi/JsiPromise.h>

#include <jsi/jsi.h>
#include <memory>
#include <string>
#include <thread>
#include <utility>

namespace audioapi {
using namespace facebook;

class AudioStretcherHostObject : public JsiHostObject {
 public:
  explicit AudioStretcherHostObject(
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker);
  JSI_HOST_FUNCTION_DECL(changePlaybackSpeed);

 private:
  std::shared_ptr<PromiseVendor> promiseVendor_;
};
} // namespace audioapi
