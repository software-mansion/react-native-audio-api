#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/core/types/OverSampleType.h>

#include <memory>
#include <string>
#include <vector>

namespace audioapi {
using namespace facebook;

struct WaveShaperOptions;
class BaseAudioContext;

class WaveShaperNodeHostObject : public AudioNodeHostObject {
 public:
  explicit WaveShaperNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const WaveShaperOptions &options);

  JSI_PROPERTY_GETTER_DECL(oversample);
  JSI_PROPERTY_GETTER_DECL(curve);

  JSI_PROPERTY_SETTER_DECL(oversample);
  JSI_HOST_FUNCTION_DECL(setCurve);
};
} // namespace audioapi
