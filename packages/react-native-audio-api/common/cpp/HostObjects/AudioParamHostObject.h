#pragma once

#include <jsi/jsi.h>
#include <memory>
#include <vector>

#include "AudioParam.h"

namespace audioapi {
using namespace facebook;

class AudioParamHostObject : public jsi::HostObject {
 public:
  explicit AudioParamHostObject(
      const std::shared_ptr<AudioParam> &wrapparamper);

  jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;

  void set(
      jsi::Runtime &runtime,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;

  std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;

 private:
  std::shared_ptr<AudioParam> param_;
};
} // namespace audioapi
