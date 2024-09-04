#pragma once

#include <memory>
#include <vector>
#include "AudioNodeHostObject.h"
#include "AudioParamHostObject.h"
#include "GainNodeWrapper.h"

namespace audiocontext {
using namespace facebook;

class GainNodeWrapper;

class GainNodeHostObject : public AudioNodeHostObject {
 protected:
  std::shared_ptr<AudioParamHostObject> gainParam_;

 public:
  explicit GainNodeHostObject(const std::shared_ptr<GainNodeWrapper> &wrapper);

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;
  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;
  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

  static std::shared_ptr<GainNodeHostObject> createFromWrapper(
      const std::shared_ptr<GainNodeWrapper> &wrapper) {
    return std::make_shared<GainNodeHostObject>(wrapper);
  }
};
} // namespace audiocontext
