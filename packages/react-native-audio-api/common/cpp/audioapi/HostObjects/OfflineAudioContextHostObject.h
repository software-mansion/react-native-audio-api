#pragma once

#include <audioapi/HostObjects/BaseAudioContextHostObject.h>

#include <jsi/jsi.h>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi {
using namespace facebook;

class OfflineAudioContext;

class OfflineAudioContextHostObject : public BaseAudioContextHostObject {
 public:
  explicit OfflineAudioContextHostObject(
      const std::shared_ptr<OfflineAudioContext> &offlineAudioContext,
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker);

  JSI_HOST_FUNCTION_DECL(resume);
  JSI_HOST_FUNCTION_DECL(suspend);
  JSI_HOST_FUNCTION_DECL(startRendering);
};
} // namespace audioapi
