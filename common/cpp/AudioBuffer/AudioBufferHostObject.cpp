#include "AudioBufferHostObject.h"
#include <android/log.h>

namespace audiocontext {
using namespace facebook;

std::vector<jsi::PropNameID> AudioBufferHostObject::getPropertyNames(
    jsi::Runtime &runtime) {
  std::vector<jsi::PropNameID> propertyNames;
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "buffer"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "sampleRate"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "length"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "duration"));
  propertyNames.push_back(
      jsi::PropNameID::forAscii(runtime, "numberOfChannels"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "getChannelData"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "setChannelData"));
  return propertyNames;
}

jsi::Value AudioBufferHostObject::get(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId) {
  auto propName = propNameId.utf8(runtime);

  if (propName == "sampleRate") {
    return jsi::Value(wrapper_->getSampleRate());
  }

  if (propName == "length") {
    return jsi::Value(wrapper_->getLength());
  }

  if (propName == "duration") {
    return jsi::Value(wrapper_->getDuration());
  }

  if (propName == "numberOfChannels") {
    return jsi::Value(wrapper_->getNumberOfChannels());
  }

  if (propName == "getChannelData") {
    return jsi::Function::createFromHostFunction(
        runtime,
        propNameId,
        1,
        [this](
            jsi::Runtime &rt,
            const jsi::Value &thisVal,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          int channel = args[0].getNumber();
          int16_t **channelData = wrapper_->getChannelData(channel);

          auto array = jsi::Array(rt, wrapper_->getLength());
          for (int i = 0; i < wrapper_->getLength(); i++) {
            array.setValueAtIndex(rt, i, jsi::Value(*channelData[i]));
          }

          return array;
        });
  }

  if (propName == "setChannelData") {
    return jsi::Function::createFromHostFunction(
        runtime,
        propNameId,
        2,
        [this](
            jsi::Runtime &rt,
            const jsi::Value &thisVal,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          int channel = args[0].getNumber();
          auto array = args[1].getObject(rt).asArray(rt);
          auto **channelData = new int16_t *[wrapper_->getLength()];

          for (int i = 0; i < wrapper_->getLength(); i++) {
            channelData[i] =
                new int16_t(array.getValueAtIndex(rt, i).getNumber() * 32767);
          }

          wrapper_->setChannelData(channel, channelData);

          return jsi::Value::undefined();
        });
  }

  throw std::runtime_error("Not yet implemented!");
}

void AudioBufferHostObject::set(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId,
    const jsi::Value &value) {
  auto propName = propNameId.utf8(runtime);

  throw std::runtime_error("Not yet implemented!");
}

} // namespace audiocontext
