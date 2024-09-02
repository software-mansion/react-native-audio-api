#include "AudioBufferSourceNodeHostObject.h"
#include <android/log.h>

namespace audiocontext
{
    using namespace facebook;

    std::shared_ptr<AudioBufferSourceNodeWrapper>
    AudioBufferSourceNodeHostObject::getAudioBufferSourceNodeWrapperFromAudioNodeWrapper() {
        return std::static_pointer_cast<AudioBufferSourceNodeWrapper>(wrapper_);
    }

    std::vector<jsi::PropNameID> AudioBufferSourceNodeHostObject::getPropertyNames(jsi::Runtime &runtime)
    {
        std::vector<jsi::PropNameID> propertyNames = AudioNodeHostObject::getPropertyNames(runtime);
        propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "start"));
        propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "stop"));
        propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "loop"));
        propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "buffer"));
        return propertyNames;
    }

    jsi::Value AudioBufferSourceNodeHostObject::get(jsi::Runtime &runtime, const jsi::PropNameID &propNameId)
    {
        auto propName = propNameId.utf8(runtime);

        if (propName == "start")
        {
            return jsi::Function::createFromHostFunction(runtime, propNameId, 1, [this](jsi::Runtime &rt, const jsi::Value &thisValue, const jsi::Value *args, size_t count) -> jsi::Value
            {
                auto time = args[0].getNumber();
                auto wrapper = getAudioBufferSourceNodeWrapperFromAudioNodeWrapper();
                wrapper->start(time);
                return jsi::Value::undefined();
            });
        }

        if (propName == "stop")
        {
            return jsi::Function::createFromHostFunction(runtime, propNameId, 1, [this](jsi::Runtime &rt, const jsi::Value &thisValue, const jsi::Value *args, size_t count) -> jsi::Value
            {
                auto time = args[0].getNumber();
                auto wrapper = getAudioBufferSourceNodeWrapperFromAudioNodeWrapper();
                wrapper->stop(time);
                return jsi::Value::undefined();
            });
        }

        if (propName == "loop")
        {
            auto wrapper = getAudioBufferSourceNodeWrapperFromAudioNodeWrapper();
            return jsi::Value(wrapper->getLoop());
        }

        if (propName == "buffer")
        {
            auto wrapper = getAudioBufferSourceNodeWrapperFromAudioNodeWrapper();
            auto buffer = wrapper->getBuffer();
            auto bufferHostObject = AudioBufferHostObject::createFromWrapper(buffer);
            return jsi::Object::createFromHostObject(runtime, bufferHostObject);
        }

        return AudioNodeHostObject::get(runtime, propNameId);
    }

    void AudioBufferSourceNodeHostObject::set(jsi::Runtime &runtime, const jsi::PropNameID &propNameId, const jsi::Value &value)
    {
        auto propName = propNameId.utf8(runtime);

        if (propName == "loop")
        {
            auto wrapper = getAudioBufferSourceNodeWrapperFromAudioNodeWrapper();
            wrapper->setLoop(value.getBool());
            return;
        }

        if (propName == "buffer")
        {
            auto bufferHostObject = value.getObject(runtime).asHostObject<AudioBufferHostObject>(runtime);
            auto wrapper = getAudioBufferSourceNodeWrapperFromAudioNodeWrapper();
            wrapper->setBuffer(bufferHostObject->wrapper_);
            return;
        }

        throw std::runtime_error("Not yet implemented!");
    }
}

