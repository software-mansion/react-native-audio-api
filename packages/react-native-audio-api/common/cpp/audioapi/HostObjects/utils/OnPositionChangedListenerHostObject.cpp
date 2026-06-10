#include <audioapi/HostObjects/utils/OnPositionChangedListenerHostObject.h>

#include <string>

namespace audioapi {

OnPositionChangedListenerHostObject::OnPositionChangedListenerHostObject(
    OnPositionChangedNode &host)
    : host_(&host) {}

OnPositionChangedListenerHostObject::~OnPositionChangedListenerHostObject() {
  host_->unregisterOnPositionChangedCallback(host_->getOnPositionChangedCallbackId());
  host_->assignOnPositionChangedCallbackId(0);
}

void OnPositionChangedListenerHostObject::setCallbackIdFromJsi(
    jsi::Runtime &runtime,
    const jsi::Value &value) {
  callbackId_ = std::stoull(value.getString(runtime).utf8(runtime));
  host_->assignOnPositionChangedCallbackId(callbackId_);
}

} // namespace audioapi
