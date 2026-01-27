#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>

#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

struct GainOptions;
class BaseAudioContext;

class GainNodeHostObject : public AudioNodeHostObject {
 public:
  explicit GainNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const GainOptions &options);

  JSI_PROPERTY_GETTER_DECL(gain);
};
} // namespace audioapi
