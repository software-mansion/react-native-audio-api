#include "AudioNodeHostObject.h"
#include "AudioContextHostObject.h"

namespace audioapi {
using namespace facebook;

std::vector<jsi::PropNameID> AudioNodeHostObject::getPropertyNames(
    jsi::Runtime &runtime) {
  std::vector<jsi::PropNameID> propertyNames;
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "connect"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "disconnect"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "numberOfInputs"));
  propertyNames.push_back(
      jsi::PropNameID::forAscii(runtime, "numberOfOutputs"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "channelCount"));
  propertyNames.push_back(
      jsi::PropNameID::forAscii(runtime, "channelCountMode"));
  propertyNames.push_back(
      jsi::PropNameID::forAscii(runtime, "channelInterpretation"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "context"));
  return propertyNames;
}

jsi::Value AudioNodeHostObject::get(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId) {
  auto propName = propNameId.utf8(runtime);

  if (propName == "connect") {
    return jsi::Function::createFromHostFunction(
        runtime,
        propNameId,
        1,
        [this](
            jsi::Runtime &rt,
            const jsi::Value &thisValue,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          auto node =
              args[0].getObject(rt).getHostObject<AudioNodeHostObject>(rt);
          wrapper_->connect(
              std::shared_ptr<AudioNodeHostObject>(node)->wrapper_);
          return jsi::Value::undefined();
        });
  }

  if (propName == "disconnect") {
    return jsi::Function::createFromHostFunction(
        runtime,
        propNameId,
        1,
        [this](
            jsi::Runtime &rt,
            const jsi::Value &thisValue,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          auto node =
              args[0].getObject(rt).getHostObject<AudioNodeHostObject>(rt);
          wrapper_->disconnect(
              std::shared_ptr<AudioNodeHostObject>(node)->wrapper_);
          return jsi::Value::undefined();
        });
  }

  if (propName == "numberOfInputs") {
    return jsi::Value(wrapper_->getNumberOfInputs());
  }

  if (propName == "numberOfOutputs") {
    return jsi::Value(wrapper_->getNumberOfOutputs());
  }

  if (propName == "channelCount") {
    return jsi::Value(wrapper_->getChannelCount());
  }

  if (propName == "channelCountMode") {
    return jsi::String::createFromUtf8(
        runtime, wrapper_->getChannelCountMode());
  }

  if (propName == "channelInterpretation") {
    return jsi::String::createFromUtf8(
        runtime, wrapper_->getChannelInterpretation());
  }

  if (propName == "context") {
    auto context =
        runtime.global().getPropertyAsObject(runtime, "__AudioContext");
    auto hostObject = context.getHostObject<AudioContextHostObject>(runtime);
    return jsi::Object::createFromHostObject(runtime, hostObject);
  }

  throw std::runtime_error("Not yet implemented!");
}

void AudioNodeHostObject::set(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId,
    const jsi::Value &value) {
  auto propName = propNameId.utf8(runtime);

  throw std::runtime_error("Not yet implemented!");
}

} // namespace audioapi
