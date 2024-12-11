#include "PeriodicWaveHostObject.h"

namespace audioapi {
using namespace facebook;

PeriodicWaveHostObject::PeriodicWaveHostObject(
    const std::shared_ptr<PeriodicWave> &periodicWave)
    : periodicWave_(periodicWave) {}

std::vector<jsi::PropNameID> PeriodicWaveHostObject::getPropertyNames(
    jsi::Runtime &runtime) {
  std::vector<jsi::PropNameID> propertyNames;
  return propertyNames;
}

jsi::Value PeriodicWaveHostObject::get(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId) {
  auto propName = propNameId.utf8(runtime);

  throw std::runtime_error("Not yet implemented!");
}

void PeriodicWaveHostObject::set(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId,
    const jsi::Value &value) {
  auto propName = propNameId.utf8(runtime);

  throw std::runtime_error("Not yet implemented!");
}

} // namespace audioapi
