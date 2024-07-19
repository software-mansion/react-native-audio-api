#pragma once

#include <jsi/jsi.h>
#include "AudioDestinationNodeWrapper.h"
#include "OscillatorNodeWrapper.h"

namespace audiocontext {
    using namespace facebook;

    class AudioDestinationNodeWrapper;
    class OscillatorNodeWrapper;

    class AudioDestinationNodeHostObject : public jsi::HostObject {

    private:
        friend class OscillatorNodeWrapper;

        std::shared_ptr<AudioDestinationNodeWrapper> wrapper_;

    public:
        explicit AudioDestinationNodeHostObject(std::shared_ptr<AudioDestinationNodeWrapper> wrapper): wrapper_(wrapper) {}

        jsi::Value get(jsi::Runtime& runtime, const jsi::PropNameID& name) override;
        void set(jsi::Runtime& runtime, const jsi::PropNameID& name, const jsi::Value& value) override;
        std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime& rt) override;
    };
} // namespace audiocontext
