#pragma once

#include <audioapi/core/core/OfflineAudioContext.h>
#include <audioapi/HostObjects/core/BaseAudioContextHostObject.h>

#include <jsi/jsi.h>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi {
using namespace facebook;

class OfflineAudioContextHostObject : public BaseAudioContextHostObject {
 public:
  explicit OfflineAudioContextHostObject(
          const std::shared_ptr<OfflineAudioContext> &offlineAudioContext,
          jsi::Runtime *runtime,
          const std::shared_ptr<react::CallInvoker> &callInvoker)
      : BaseAudioContextHostObject(offlineAudioContext, runtime, callInvoker) {
    addFunctions(
      JSI_EXPORT_FUNCTION(OfflineAudioContextHostObject, resume),
      JSI_EXPORT_FUNCTION(OfflineAudioContextHostObject, suspend),
      JSI_EXPORT_FUNCTION(OfflineAudioContextHostObject, startRendering));
  }

  JSI_HOST_FUNCTION_DECL(resume);
  JSI_HOST_FUNCTION_DECL(suspend);
  JSI_HOST_FUNCTION_DECL(startRendering);
};
} // namespace audioapi
