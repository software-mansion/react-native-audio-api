#include "OscillatorHostObject.h"
#include <jsi/jsi.h>

namespace audiocontext {
  using namespace facebook;

  std::vector<jsi::PropNameID> OscillatorHostObject::getPropertyNames(jsi::Runtime &runtime)
  {
    std::vector<jsi::PropNameID> propertyNames;
    propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "start"));
    propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "stop"));
    return propertyNames;
  }

  jsi::Value OscillatorHostObject::get(jsi::Runtime &runtime, const jsi::PropNameID &propNameId) {
    auto propName = propNameId.utf8(runtime);

    if (propName == "start") {
      return jsi::Function::createFromHostFunction(runtime, propNameId, 0,
        [this](jsi::Runtime &rt, const jsi::Value &, const jsi::Value *args, size_t count) {
          if (count != 0) {
            throw std::invalid_argument("start expects exactly zero arguments");
          }
          platformOscillator_.start();
          return jsi::Value::undefined();
        });
    }

    if (propName == "stop") {
      return jsi::Function::createFromHostFunction(runtime, propNameId, 0,
        [this](jsi::Runtime &rt, const jsi::Value &, const jsi::Value *args, size_t count) {
          if (count != 0) {
            throw std::invalid_argument("stop expects exactly zero arguments");
          }
          platformOscillator_.stop();
          return jsi::Value::undefined();
        });
    }

    throw std::runtime_error("Not yet implemented!");
  }

  void OscillatorHostObject::set(jsi::Runtime &runtime, const jsi::PropNameID &propNameId, const jsi::Value &value)
  {
    auto propName = propNameId.utf8(runtime);
    throw std::runtime_error("Not yet implemented!");
  }

} // namespace audiocontext
