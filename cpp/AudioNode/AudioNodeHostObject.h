#pragma once

#include <jsi/jsi.h>
#include "AudioNodeWrapper.h"

namespace audiocontext {
    using namespace facebook;

    class AudioNodeWrapper;

    class AudioNodeHostObject : public jsi::HostObject {

    public:
        std::shared_ptr<AudioNodeWrapper> wrapper_;

    public:
        explicit AudioNodeHostObject(std::shared_ptr<AudioNodeWrapper> wrapper) : wrapper_(wrapper) {}
    };
} // namespace audiocontext
