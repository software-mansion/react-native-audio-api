#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>

#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

struct DelayOptions;
class BaseAudioContext;

class DelayNodeHostObject : public AudioNodeHostObject {
 public:
  explicit DelayNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const DelayOptions &options);

  [[nodiscard]] size_t getSizeInBytes() const;

  JSI_PROPERTY_GETTER_DECL(delayTime);
};
} // namespace audioapi
