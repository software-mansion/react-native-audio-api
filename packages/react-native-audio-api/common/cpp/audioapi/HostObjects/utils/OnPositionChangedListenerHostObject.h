#pragma once

#include <audioapi/core/sources/OnPositionChangedNode.h>
#include <audioapi/utils/Macros.h>

#include <jsi/jsi.h>
#include <cstdint>

namespace audioapi {
using namespace facebook;

class OnPositionChangedListenerHostObject {
 public:
  explicit OnPositionChangedListenerHostObject(OnPositionChangedNode &host);
  ~OnPositionChangedListenerHostObject();
  DELETE_COPY_AND_MOVE(OnPositionChangedListenerHostObject);

  void setCallbackIdFromJsi(jsi::Runtime &runtime, const jsi::Value &value);

 private:
  OnPositionChangedNode *host_;
  uint64_t callbackId_ = 0;
};

} // namespace audioapi
