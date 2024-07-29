#pragma once

#include <jsi/jsi.h>
#include "AudioDestinationNodeWrapper.h"
#include "AudioNodeHostObject.h"

namespace audiocontext {
    using namespace facebook;

    class AudioDestinationNodeWrapper;

    class AudioDestinationNodeHostObject : public AudioNodeHostObject {

    protected:
        std::shared_ptr<AudioDestinationNodeWrapper> wrapper_;

    public:
        explicit AudioDestinationNodeHostObject(const std::shared_ptr<AudioDestinationNodeWrapper> &wrapper): AudioNodeHostObject(wrapper), wrapper_(wrapper) {}

        jsi::Value get(jsi::Runtime& runtime, const jsi::PropNameID& name) override;
        void set(jsi::Runtime& runtime, const jsi::PropNameID& name, const jsi::Value& value) override;
        std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime& rt) override;

        static std::shared_ptr<AudioDestinationNodeHostObject> createFromWrapper(const std::shared_ptr<AudioDestinationNodeWrapper> &wrapper) {
            return std::make_shared<AudioDestinationNodeHostObject>(wrapper);
        }
    };
} // namespace audiocontext
