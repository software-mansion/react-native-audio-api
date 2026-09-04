#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>

namespace audioapi {
using namespace facebook;

class AudioListener;
struct PannerOptions;
class BaseAudioContext;
class PannerNode;

class PannerNodeHostObject : public AudioNodeHostObject {
 public:
  explicit PannerNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      AudioListener *listener,
      const PannerOptions &options);

  JSI_PROPERTY_GETTER_DECL(positionX);
  JSI_PROPERTY_GETTER_DECL(positionY);
  JSI_PROPERTY_GETTER_DECL(positionZ);
  JSI_PROPERTY_GETTER_DECL(orientationX);
  JSI_PROPERTY_GETTER_DECL(orientationY);
  JSI_PROPERTY_GETTER_DECL(orientationZ);
  JSI_PROPERTY_GETTER_DECL(panningModel);
  JSI_PROPERTY_SETTER_DECL(panningModel);
  JSI_PROPERTY_GETTER_DECL(distanceModel);
  JSI_PROPERTY_SETTER_DECL(distanceModel);
  JSI_PROPERTY_GETTER_DECL(refDistance);
  JSI_PROPERTY_SETTER_DECL(refDistance);
  JSI_PROPERTY_GETTER_DECL(maxDistance);
  JSI_PROPERTY_SETTER_DECL(maxDistance);
  JSI_PROPERTY_GETTER_DECL(rolloffFactor);
  JSI_PROPERTY_SETTER_DECL(rolloffFactor);
  JSI_PROPERTY_GETTER_DECL(coneInnerAngle);
  JSI_PROPERTY_SETTER_DECL(coneInnerAngle);
  JSI_PROPERTY_GETTER_DECL(coneOuterAngle);
  JSI_PROPERTY_SETTER_DECL(coneOuterAngle);
  JSI_PROPERTY_GETTER_DECL(coneOuterGain);
  JSI_PROPERTY_SETTER_DECL(coneOuterGain);

  [[nodiscard]] size_t getMemoryPressure() const override {
    return AudioNodeHostObject::getMemoryPressure() + 6 * kAudioParamBytes;
  }

 private:
  PannerNode *pannerNode_ = nullptr;

  std::shared_ptr<AudioParamHostObject> positionXParam_;
  std::shared_ptr<AudioParamHostObject> positionYParam_;
  std::shared_ptr<AudioParamHostObject> positionZParam_;
  std::shared_ptr<AudioParamHostObject> orientationXParam_;
  std::shared_ptr<AudioParamHostObject> orientationYParam_;
  std::shared_ptr<AudioParamHostObject> orientationZParam_;

  PanningModelType panningModel_;
  DistanceModelType distanceModel_;
  double refDistance_;
  double maxDistance_;
  double rolloffFactor_;
  double coneInnerAngle_;
  double coneOuterAngle_;
  double coneOuterGain_;
};

} // namespace audioapi
