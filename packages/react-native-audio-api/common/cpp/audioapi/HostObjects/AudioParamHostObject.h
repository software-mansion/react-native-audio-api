#pragma once

#include <audioapi/jsi/JsiHostObject.h>

#include <jsi/jsi.h>
#include <utility>
#include <memory>
#include <vector>
#include <cstddef>

namespace audioapi {
using namespace facebook;

class AudioParam;

class AudioParamHostObject : public JsiHostObject {
 public:
  explicit AudioParamHostObject(const std::shared_ptr<AudioParam> &param);

  JSI_PROPERTY_GETTER_DECL(value);
  JSI_PROPERTY_GETTER_DECL(defaultValue);
  JSI_PROPERTY_GETTER_DECL(minValue);
  JSI_PROPERTY_GETTER_DECL(maxValue);

  JSI_PROPERTY_SETTER_DECL(value);

  JSI_HOST_FUNCTION_DECL(setValueAtTime);
  JSI_HOST_FUNCTION_DECL(linearRampToValueAtTime);
  JSI_HOST_FUNCTION_DECL(exponentialRampToValueAtTime);
  JSI_HOST_FUNCTION_DECL(setTargetAtTime);
  JSI_HOST_FUNCTION_DECL(setValueCurveAtTime);
  JSI_HOST_FUNCTION_DECL(cancelScheduledValues);
  JSI_HOST_FUNCTION_DECL(cancelAndHoldAtTime);

 private:
  friend class AudioNodeHostObject;

  std::shared_ptr<AudioParam> param_;
};
} // namespace audioapi
