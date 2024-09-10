#include "AudioDestinationNodeHostObject.h"

namespace audioapi {
using namespace facebook;

std::vector<jsi::PropNameID> AudioDestinationNodeHostObject::getPropertyNames(
    jsi::Runtime &runtime) {
  std::vector<jsi::PropNameID> propertyNames;
  return propertyNames;
}

jsi::Value AudioDestinationNodeHostObject::get(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId) {
  auto propName = propNameId.utf8(runtime);

  return AudioNodeHostObject::get(runtime,propNameId);
}

void AudioDestinationNodeHostObject::set(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId,
    const jsi::Value &value) {
  auto propName = propNameId.utf8(runtime);

  return AudioNodeHostObject::set(runtime, propNameId, value);
}

} // namespace audioapi
