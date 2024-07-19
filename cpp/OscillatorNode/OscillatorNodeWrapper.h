#pragma once

#include <jsi/jsi.h>
#include <memory>

#ifdef ANDROID
#include "OscillatorNode.h"
#else
#include "IOSOscillator.h"
#endif

namespace audiocontext {
    using namespace facebook;

#ifdef ANDROID
    class OscillatorNode;
#endif

    class OscillatorNodeWrapper {
    private:
#ifdef ANDROID
        std::shared_ptr<OscillatorNode> oscillator_;
#else
        std::shared_ptr<IOSOscillator> oscillator_;
#endif

    public:
#ifdef ANDROID
        explicit OscillatorNodeWrapper(std::shared_ptr<OscillatorNode> oscillator) : oscillator_(oscillator) {}
#else
        explicit OscillatorNodeWrapper() : oscillator_(std::make_shared<IOSOscillator>()) {}
#endif

        jsi::Value getFrequency(jsi::Runtime &runtime, const jsi::PropNameID &propNameId);
        jsi::Value getDetune(jsi::Runtime &runtime, const jsi::PropNameID &propNameId);
        jsi::Value getType(jsi::Runtime &runtime, const jsi::PropNameID &propNameId);
        jsi::Value start(jsi::Runtime &runtime, const jsi::PropNameID &propNameId);
        jsi::Value stop(jsi::Runtime &runtime, const jsi::PropNameID &propNameId);
        jsi::Value connect(jsi::Runtime &runtime, const jsi::PropNameID &propNameId);

        void setFrequency(jsi::Runtime &runtime, const jsi::PropNameID &propNameId, const jsi::Value &value);
        void setDetune(jsi::Runtime &runtime, const jsi::PropNameID &propNameId, const jsi::Value &value);
        void setType(jsi::Runtime &runtime, const jsi::PropNameID &propNameId, const jsi::Value &value);
    };
} // namespace audiocontext
