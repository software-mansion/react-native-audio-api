#pragma once

#include <memory>
#include <string>
#include <vector>

#include "AudioNodeHostObject.h"
#include "AudioParamHostObject.h"
#include "StereoPannerNode.h"

namespace audioapi {
using namespace facebook;

class StereoPannerNodeHostObject : public AudioNodeHostObject {
 public:
  explicit StereoPannerNodeHostObject(
      const std::shared_ptr<StereoPannerNode> &node);

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;

  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;

  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

 private:
  std::shared_ptr<AudioParamHostObject> panParam_;
};
} // namespace audioapi
