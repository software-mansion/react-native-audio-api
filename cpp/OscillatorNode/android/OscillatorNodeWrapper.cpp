#ifdef ANDROID
#include "OscillatorNodeWrapper.h"

namespace audiocontext {
    jsi::Value OscillatorNodeWrapper::getFrequency(jsi::Runtime &runtime, const jsi::PropNameID &propNameId) {
        return jsi::Function::createFromHostFunction(runtime, propNameId, 0, [this](jsi::Runtime &rt, const jsi::Value &thisValue, const jsi::Value *args, size_t count) -> jsi::Value
        {
            auto frequency = oscillator_->getFrequency();
            return jsi::Value(frequency);
        });
    }

    jsi::Value OscillatorNodeWrapper::getDetune(jsi::Runtime &runtime, const jsi::PropNameID &propNameId) {
        return jsi::Function::createFromHostFunction(runtime, propNameId, 0, [this](jsi::Runtime &rt, const jsi::Value &thisValue, const jsi::Value *args, size_t count) -> jsi::Value
        {
            auto detune = oscillator_->getDetune();
            return jsi::Value(detune);
        });
    }

    jsi::Value OscillatorNodeWrapper::getType(jsi::Runtime &runtime, const jsi::PropNameID &propNameId) {
        return jsi::Function::createFromHostFunction(runtime, propNameId, 0, [this](jsi::Runtime &rt, const jsi::Value &thisValue, const jsi::Value *args, size_t count) -> jsi::Value
        {
            auto waveType = oscillator_->getWaveType();
            return jsi::String::createFromUtf8(rt, waveType);
        });
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
            auto destination = args[0].getObject(rt).getHostObject<AudioDestinationNodeHostObject>(rt);
            oscillator_->connect(*destination->wrapper_->destination_);
            return jsi::Value::undefined();
        });
    }

    void OscillatorNodeWrapper::setFrequency(jsi::Runtime &runtime, const jsi::PropNameID &propNameId, const jsi::Value &value) {
        double frequency = value.getNumber();
        oscillator_->setFrequency(frequency);
    }

    void OscillatorNodeWrapper::setDetune(jsi::Runtime &runtime, const jsi::PropNameID &propNameId, const jsi::Value &value) {
        double detune = value.getNumber();
        oscillator_->setDetune(detune);
    }

    void OscillatorNodeWrapper::setType(jsi::Runtime &runtime, const jsi::PropNameID &propNameId, const jsi::Value &value) {
        std::string waveType = value.getString(runtime).utf8(runtime);
        oscillator_->setWaveType(waveType);
    }
}
#endif