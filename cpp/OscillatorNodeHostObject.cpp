#include "OscillatorNodeHostObject.h"
#include <jsi/jsi.h>

namespace audiocontext {
  using namespace facebook;

  std::vector<jsi::PropNameID> OscillatorNodeHostObject::getPropertyNames(jsi::Runtime &runtime)
  {
    std::vector<jsi::PropNameID> propertyNames;
    propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "start"));
    propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "stop"));
    return propertyNames;
  }

  jsi::Value OscillatorNodeHostObject::get(jsi::Runtime &runtime, const jsi::PropNameID &propNameId) {
    auto propName = propNameId.utf8(runtime);

    if (propName == "start") {
      return jsi::Function::createFromHostFunction(runtime, propNameId, 0,
        [this](jsi::Runtime &rt, const jsi::Value &, const jsi::Value *args, size_t count) {
          if (count != 0) {
            throw std::invalid_argument("start expects exactly zero arguments");
          }
          IOSOscillator_.start();
          return jsi::Value::undefined();
        });
    }

    if (propName == "stop") {
      return jsi::Function::createFromHostFunction(runtime, propNameId, 0,
        [this](jsi::Runtime &rt, const jsi::Value &, const jsi::Value *args, size_t count) {
          if (count != 0) {
            throw std::invalid_argument("stop expects exactly zero arguments");
          }
          IOSOscillator_.stop();
          return jsi::Value::undefined();
        });
    }

    if (propName == "frequency") {
      return jsi::Value(frequency_);
    }

    throw std::runtime_error("Not yet implemented!");
  }

  void OscillatorNodeHostObject::set(jsi::Runtime &runtime, const jsi::PropNameID &propNameId, const jsi::Value &value)
  {
    auto propName = propNameId.utf8(runtime);

    if (propName == "frequency") {
      float frequency = static_cast<float>(value.asNumber());
      frequency_ = frequency;
      return IOSOscillator_.changeFrequency(frequency);
    }

    throw std::runtime_error("Not yet implemented!");
  }

} // namespace audiocontext
