#include "AudioContextHostObject.h"

namespace audiocontext
{
  using namespace facebook;

  std::vector<jsi::PropNameID> AudioContextHostObject::getPropertyNames(jsi::Runtime &runtime)
  {
    std::vector<jsi::PropNameID> propertyNames;
    propertyNames.push_back(jsi::PropNameID::forUtf8(runtime, "createOscillator"));
    propertyNames.push_back(jsi::PropNameID::forUtf8(runtime, "destination"));
    propertyNames.push_back(jsi::PropNameID::forUtf8(runtime, "Symbol.toStringTag"));

    return propertyNames;
  }

  jsi::Value AudioContextHostObject::get(jsi::Runtime &runtime, const jsi::PropNameID &propNameId)
  {
    auto propName = propNameId.utf8(runtime);

    if (propName == "Symbol.toStringTag")
    {
      return toString(runtime, propNameId);
    }

    if (propName == "createOscillator")
    {
      return createOscillator(runtime, propNameId);
    }

    if (propName == "destination")
    {
      return getDestination(runtime, propNameId);
    }

    throw std::runtime_error("[AudioContextHostObject::get] Not yet implemented!");
  }

  void AudioContextHostObject::set(jsi::Runtime &runtime, const jsi::PropNameID &propNameId, const jsi::Value &value)
  {
    auto propName = propNameId.utf8(runtime);

    throw std::runtime_error("[AudioContextHostObject::set]Not yet implemented!");
  }

  jsi::Value AudioContextHostObject::createOscillator(jsi::Runtime &runtime,
                                                      const jsi::PropNameID &propNameId)
  {
    return jsi::Function::createFromHostFunction(runtime, propNameId, 0, [this](jsi::Runtime &runtime, const jsi::Value &thisValue, const jsi::Value *arguments, size_t count) -> jsi::Value
                                                 { return audiocontext_->createOscillator(); });
  }

  jsi::Value AudioContextHostObject::getDestination(jsi::Runtime &runtime, const jsi::PropNameID &propNameId)
  {
    return jsi::Function::createFromHostFunction(runtime, propNameId, 0, [this](jsi::Runtime &runtime, const jsi::Value &thisValue, const jsi::Value *arguments, size_t count) -> jsi::Value
                                                 { return audiocontext_->getDestination(); });
  }

  jsi::Value AudioContextHostObject::toString(jsi::Runtime &runtime, const jsi::PropNameID &propNameId)
  {
    return jsi::Function::createFromHostFunction(runtime, propNameId, 0, [this](jsi::Runtime &runtime, const jsi::Value &thisValue, const jsi::Value *arguments, size_t count) -> jsi::Value
                                                 { return jsi::String::createFromUtf8(runtime, "[AudioContext]"); });
  }
}
