#ifndef ANDROID
#include "OscillatorNodeWrapper.h"

namespace audiocontext {
    jsi::Value OscillatorNodeWrapper::getFrequency(jsi::Runtime &runtime, const jsi::PropNameID &propNameId) {
        return jsi::Value(oscillator_->getFrequency());
    }

    jsi::Value OscillatorNodeWrapper::getDetune(jsi::Runtime &runtime, const jsi::PropNameID &propNameId) {
        return jsi::Value(oscillator_->getDetune());
    }

    jsi::Value OscillatorNodeWrapper::getType(jsi::Runtime &runtime, const jsi::PropNameID &propNameId) {
        return jsi::String::createFromUtf8(runtime, oscillator_->getType());
    }

    jsi::Value OscillatorNodeWrapper::start(jsi::Runtime &runtime, const jsi::PropNameID &propNameId) {
        return jsi::Function::createFromHostFunction(runtime, propNameId, 0, [this](jsi::Runtime &rt, const jsi::Value &thisValue, const jsi::Value *args, size_t count) -> jsi::Value
        {
            oscillator_->start();
            return jsi::Value::undefined();
        });
    }

    jsi::Value OscillatorNodeWrapper::stop(jsi::Runtime &runtime, const jsi::PropNameID &propNameId) {
        return jsi::Function::createFromHostFunction(runtime, propNameId, 0, [this](jsi::Runtime &rt, const jsi::Value &thisValue, const jsi::Value *args, size_t count) -> jsi::Value
        {
            oscillator_->stop();
            return jsi::Value::undefined();
        });
    }

    jsi::Value OscillatorNodeWrapper::connect(jsi::Runtime &runtime, const jsi::PropNameID &propNameId) {
        return jsi::Function::createFromHostFunction(runtime, propNameId, 1, [this](jsi::Runtime &rt, const jsi::Value &thisValue, const jsi::Value *args, size_t count) -> jsi::Value
        {
            throw std::runtime_error("[OscillatorNodeHostObject::connect] Not yet implemented!");
        });
    }

    void OscillatorNodeWrapper::setFrequency(jsi::Runtime &runtime, const jsi::PropNameID &propNameId, const jsi::Value &value) {
        double frequency = value.getNumber();
        oscillator_->changeFrequency(frequency);
    }

    void OscillatorNodeWrapper::setDetune(jsi::Runtime &runtime, const jsi::PropNameID &propNameId, const jsi::Value &value) {
        double detune = value.getNumber();
        oscillator_->changeDetune(detune);
    }

    void OscillatorNodeWrapper::setType(jsi::Runtime &runtime, const jsi::PropNameID &propNameId, const jsi::Value &value) {
        std::string waveType = value.getString(runtime).utf8(runtime);
        oscillator_->setType(waveType);
    }
}
#endif
