#include "OscillatorNodeHostObject.h"

namespace audiocontext
{
  using namespace facebook;

    OscillatorNodeHostObject::OscillatorNodeHostObject(const std::shared_ptr<OscillatorNodeWrapper> &wrapper) : AudioNodeHostObject(wrapper), wrapper_(wrapper) {
        auto frequencyParam = wrapper->getFrequencyParam();
        frequencyParam_ = AudioParamHostObject::createFromWrapper(frequencyParam);
        auto detuneParam = wrapper->getDetuneParam();
        detuneParam_ = AudioParamHostObject::createFromWrapper(detuneParam);
    }

  std::vector<jsi::PropNameID> OscillatorNodeHostObject::getPropertyNames(jsi::Runtime &runtime)
  {
    std::vector<jsi::PropNameID> propertyNames = AudioNodeHostObject::getPropertyNames(runtime);
    propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "start"));
    propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "stop"));
    propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "frequency"));
    propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "detune"));
    propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "type"));
    return propertyNames;
  }

  jsi::Value OscillatorNodeHostObject::get(jsi::Runtime &runtime, const jsi::PropNameID &propNameId)
  {
    auto propName = propNameId.utf8(runtime);

    if (propName == "start")
    {
        return jsi::Function::createFromHostFunction(runtime, propNameId, 1, [this](jsi::Runtime &rt, const jsi::Value &thisValue, const jsi::Value *args, size_t count) -> jsi::Value
        {
            auto time = args[0].getNumber();
            wrapper_->start(time);
            return jsi::Value::undefined();
        });
    }

    if (propName == "stop")
    {
        return jsi::Function::createFromHostFunction(runtime, propNameId, 1, [this](jsi::Runtime &rt, const jsi::Value &thisValue, const jsi::Value *args, size_t count) -> jsi::Value
        {
            auto time = args[0].getNumber();
            wrapper_->stop(time);
            return jsi::Value::undefined();
        });
    }

    if (propName == "frequency")
    {
        return jsi::Object::createFromHostObject(runtime, frequencyParam_);
    }

    if (propName == "detune")
    {
        return jsi::Object::createFromHostObject(runtime, detuneParam_);
    }

    if (propName == "type")
    {
        auto waveType = wrapper_->getType();
        return jsi::String::createFromUtf8(runtime, waveType);
    }

    return AudioNodeHostObject::get(runtime, propNameId);
  }

  void OscillatorNodeHostObject::set(jsi::Runtime &runtime, const jsi::PropNameID &propNameId, const jsi::Value &value)
  {
    auto propName = propNameId.utf8(runtime);

    if (propName == "type")
    {
        std::string waveType = value.getString(runtime).utf8(runtime);
        wrapper_->setType(waveType);
        return;
    }

    throw std::runtime_error("Not yet implemented!");
  }
}