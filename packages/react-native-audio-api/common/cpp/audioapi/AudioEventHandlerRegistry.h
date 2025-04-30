#pragma once

#include <jsi/jsi.h>
#include <ReactCommon/CallInvoker.h>
#include <memory>

namespace audioapi {
using namespace facebook;

class AudioEventHandlerRegistry {
 public:
  explicit AudioEventHandlerRegistry(
      const std::shared_ptr<react::CallInvoker> &callInvoker);

 private:
  std::shared_ptr<react::CallInvoker> callInvoker_;
};

} // namespace audioapi
