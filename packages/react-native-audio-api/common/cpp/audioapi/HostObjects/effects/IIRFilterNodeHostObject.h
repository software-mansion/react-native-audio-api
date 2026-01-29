#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>

#include <memory>
#include <string>
#include <vector>

namespace audioapi {
using namespace facebook;

struct IIRFilterOptions;
class BaseAudioContext;

class IIRFilterNodeHostObject : public AudioNodeHostObject {
 public:
  explicit IIRFilterNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const IIRFilterOptions &options);

  JSI_HOST_FUNCTION_DECL(getFrequencyResponse);
};
} // namespace audioapi
