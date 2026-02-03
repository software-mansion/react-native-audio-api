#pragma once

#include <audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h>

#include <memory>
#include <string>
#include <vector>

namespace audioapi {
using namespace facebook;

struct ConstantSourceOptions;
class BaseAudioContext;

class ConstantSourceNodeHostObject : public AudioScheduledSourceNodeHostObject {
 public:
  explicit ConstantSourceNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const ConstantSourceOptions &options);

  JSI_PROPERTY_GETTER_DECL(offset);
};
} // namespace audioapi
