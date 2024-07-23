#include "OscillatorNodeHostObject.h"

namespace audiocontext
{
  using namespace facebook;

  std::vector<jsi::PropNameID> OscillatorNodeHostObject::getPropertyNames(jsi::Runtime &runtime)
  {
    std::vector<jsi::PropNameID> propertyNames;
    propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "start"));
    propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "stop"));
    propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "frequency"));
    propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "detune"));
    propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "type"));
    propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "connect"));
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
        auto frequency = wrapper_->getFrequency();
        return jsi::Value(frequency);
    }

    if (propName == "detune")
    {
        auto detune = wrapper_->getDetune();
        return jsi::Value(detune);
    }

    if (propName == "type")
    {
        auto waveType = wrapper_->getType();
        return jsi::String::createFromUtf8(runtime, waveType);
    }

    if (propName == "connect")
    {
        return jsi::Function::createFromHostFunction(runtime, propNameId, 1, [this](jsi::Runtime &rt, const jsi::Value &thisValue, const jsi::Value *args, size_t count) -> jsi::Value
        {
            auto node = args[0].getObject(rt).getHostObject<AudioNodeHostObject>(rt);
            wrapper_->connect(std::shared_ptr<AudioNodeHostObject>(node)->wrapper_);
            return jsi::Value::undefined();
        });
    }

    throw std::runtime_error("Prop not yet implemented!");
  }

  void OscillatorNodeHostObject::set(jsi::Runtime &runtime, const jsi::PropNameID &propNameId, const jsi::Value &value)
  {
    auto propName = propNameId.utf8(runtime);

    if (propName == "frequency")
    {
        double frequency = value.getNumber();
        wrapper_->setFrequency(frequency);
        return;
    }

    if (propName == "detune")
    {

        double detune = value.getNumber();
        wrapper_->setDetune(detune);
        return;
    }

    if (propName == "type")
    {
        std::string waveType = value.getString(runtime).utf8(runtime);
        wrapper_->setType(waveType);
        return;
    }

    throw std::runtime_error("Not yet implemented!");
  }
}
