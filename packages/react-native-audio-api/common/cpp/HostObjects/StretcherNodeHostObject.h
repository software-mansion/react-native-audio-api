#pragma once

#include <memory>
#include <vector>

#include "AudioNodeHostObject.h"
#include "AudioParamHostObject.h"
#include "StretcherNode.h"

namespace audioapi {
    using namespace facebook;

    class StretcherNodeHostObject : public AudioNodeHostObject {
    public:
        explicit StretcherNodeHostObject(const std::shared_ptr<StretcherNode> &node)
                : AudioNodeHostObject(node) {
            addGetters(JSI_EXPORT_PROPERTY_GETTER(StretcherNodeHostObject, rate));
        }

        JSI_PROPERTY_GETTER(rate) {
            auto stretcherNode = std::static_pointer_cast<StretcherNode>(node_);
            auto rateParam =
                    std::make_shared<AudioParamHostObject>(stretcherNode->getRateParam());
            return jsi::Object::createFromHostObject(runtime, rateParam);
        }
    };
} // namespace audioapi
