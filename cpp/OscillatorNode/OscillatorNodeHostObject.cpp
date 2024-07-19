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
      return wrapper_->start(runtime, propNameId);
    }

    if (propName == "stop")
    {
        return wrapper_->stop(runtime, propNameId);
    }

    if (propName == "frequency")
    {
        return wrapper_->getFrequency(runtime, propNameId);
    }

    if (propName == "detune")
    {
        return wrapper_->getDetune(runtime, propNameId);
    }

    if (propName == "type")
    {
        return wrapper_->getType(runtime, propNameId);
    }

    if (propName == "connect")
    {
        return wrapper_->connect(runtime, propNameId);
    }

    throw std::runtime_error("Prop not yet implemented!");
  }

  void OscillatorNodeHostObject::set(jsi::Runtime &runtime, const jsi::PropNameID &propNameId, const jsi::Value &value)
  {
    auto propName = propNameId.utf8(runtime);

    if (propName == "frequency")
    {
      wrapper_->setFrequency(runtime, propNameId, value);
      return;
    }

    if (propName == "detune")
    {
        wrapper_->setDetune(runtime, propNameId, value);
      return;
    }

    if (propName == "type")
    {
        std::string waveType = value.getString(runtime).utf8(runtime);
        if (waveType == "sine" || waveType == "triangle" || waveType == "sawtooth" || waveType == "square") {
          wrapper_->setType(runtime, propNameId, value);
          return;
        }

        throw std::runtime_error("You have to pick correct wave type: ['sine', 'square', 'sawtooth', 'trinagle']");
    }

    throw std::runtime_error("Not yet implemented!");
  }
}
