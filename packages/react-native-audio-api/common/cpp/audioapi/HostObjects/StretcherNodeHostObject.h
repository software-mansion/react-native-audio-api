#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/core/effects/StretcherNode.h>

#include <memory>
#include <vector>

namespace audioapi {

using namespace facebook;

class StretcherNodeHostObject : public AudioNodeHostObject {
 public:
  explicit StretcherNodeHostObject(const std::shared_ptr<StretcherNode> &node): AudioNodeHostObject(node) {
        addGetters(JSI_EXPORT_PROPERTY_GETTER(StretcherNodeHostObject, rate),
               JSI_EXPORT_PROPERTY_GETTER(StretcherNodeHostObject, semitones));
    }

  JSI_PROPERTY_GETTER(rate) {
    auto stretcherNode = std::static_pointer_cast<StretcherNode>(node_);
    auto rateParam =
            std::make_shared<AudioParamHostObject>(stretcherNode->getRateParam());
    return jsi::Object::createFromHostObject(runtime, rateParam);
  }

  JSI_PROPERTY_GETTER(semitones) {
    auto stretcherNode = std::static_pointer_cast<StretcherNode>(node_);
    auto semitonesParam =
            std::make_shared<AudioParamHostObject>(stretcherNode->getSemitonesParam());
    return jsi::Object::createFromHostObject(runtime, semitonesParam);
  }
};
} // namespace audioapi
