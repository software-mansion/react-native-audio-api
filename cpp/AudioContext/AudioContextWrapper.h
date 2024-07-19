#pragma once

#include <jsi/jsi.h>
#include <memory>

#ifdef ANDROID
#include "AudioContext.h"
#else
#include "OscillatorNodeHostObject.h"
#endif

namespace audiocontext {
    using namespace facebook;

#ifdef ANDROID
    class AudioContext;
#endif

    class AudioContextWrapper {
#ifdef ANDROID
    private:
        std::shared_ptr<AudioContext> audiocontext_;
#endif

    public:
#ifdef ANDROID
        explicit AudioContextWrapper(
                std::shared_ptr<AudioContext> audiocontext) : audiocontext_(audiocontext) {}
#else
        explicit AudioContextWrapper() {}
#endif

        jsi::Value createOscillator(jsi::Runtime& runtime, const jsi::PropNameID& propNameId);
        jsi::Value getDestination(jsi::Runtime& runtime, const jsi::PropNameID& propNameId);
    };
} // namespace audiocontext
