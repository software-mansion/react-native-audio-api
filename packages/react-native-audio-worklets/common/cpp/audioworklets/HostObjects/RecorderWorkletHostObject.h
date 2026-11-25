#pragma once

#include <audioapi/compatibility/StableAPI.h>
#include <audioworklets/core/RecorderWorklet.h>

#include <memory>

namespace audioworklets {

using namespace facebook;

class RecorderWorkletHostObject : public audioapi::AudioConsumerHostObject {
 public:
  explicit RecorderWorkletHostObject(const std::shared_ptr<RecorderWorklet> &worklet);

  JSI_PROPERTY_GETTER_DECL(bufferLength);
  JSI_PROPERTY_GETTER_DECL(smoothingTimeConstant);

  JSI_PROPERTY_SETTER_DECL(smoothingTimeConstant);

 private:
  RecorderWorklet *worklet_;
};

} // namespace audioworklets
