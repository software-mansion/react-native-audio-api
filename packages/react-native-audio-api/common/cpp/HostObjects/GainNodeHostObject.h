#pragma once

#include <memory>
#include <vector>

#include "AudioNodeHostObject.h"
#include "AudioParamHostObject.h"
#include "GainNode.h"

namespace audioapi {
using namespace facebook;

class GainNodeHostObject : public AudioNodeHostObject {
 protected:
  std::shared_ptr<AudioParamHostObject> gainParam_;

 public:
  explicit GainNodeHostObject(const std::shared_ptr<GainNode> &node);

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;
  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;
  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;
};
} // namespace audioapi
