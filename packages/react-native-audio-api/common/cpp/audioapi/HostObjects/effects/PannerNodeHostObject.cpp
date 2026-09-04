#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/TypedAudioNodePtr.hpp>
#include <audioapi/HostObjects/effects/PannerNodeHostObject.h>
#include <audioapi/HostObjects/utils/JsEnumParser.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/PannerNode.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>
#include <utility>

namespace audioapi {

PannerNodeHostObject::PannerNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    AudioListener *listener,
    const PannerOptions &options)
    : AudioNodeHostObject(
          context->getGraph(),
          std::make_unique<PannerNode>(context, listener, options),
          options),
      pannerNode_(typedAudioNode<PannerNode>(node_)),
      panningModel_(options.panningModel),
      distanceModel_(options.distanceModel),
      refDistance_(options.refDistance),
      maxDistance_(options.maxDistance),
      rolloffFactor_(options.rolloffFactor),
      coneInnerAngle_(options.coneInnerAngle),
      coneOuterAngle_(options.coneOuterAngle),
      coneOuterGain_(options.coneOuterGain) {
  positionXParam_ =
      std::make_shared<AudioParamHostObject>(graph_, node_, pannerNode_->getPositionXParam());
  positionYParam_ =
      std::make_shared<AudioParamHostObject>(graph_, node_, pannerNode_->getPositionYParam());
  positionZParam_ =
      std::make_shared<AudioParamHostObject>(graph_, node_, pannerNode_->getPositionZParam());
  orientationXParam_ =
      std::make_shared<AudioParamHostObject>(graph_, node_, pannerNode_->getOrientationXParam());
  orientationYParam_ =
      std::make_shared<AudioParamHostObject>(graph_, node_, pannerNode_->getOrientationYParam());
  orientationZParam_ =
      std::make_shared<AudioParamHostObject>(graph_, node_, pannerNode_->getOrientationZParam());

  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(PannerNodeHostObject, positionX),
      JSI_EXPORT_PROPERTY_GETTER(PannerNodeHostObject, positionY),
      JSI_EXPORT_PROPERTY_GETTER(PannerNodeHostObject, positionZ),
      JSI_EXPORT_PROPERTY_GETTER(PannerNodeHostObject, orientationX),
      JSI_EXPORT_PROPERTY_GETTER(PannerNodeHostObject, orientationY),
      JSI_EXPORT_PROPERTY_GETTER(PannerNodeHostObject, orientationZ),
      JSI_EXPORT_PROPERTY_GETTER(PannerNodeHostObject, panningModel),
      JSI_EXPORT_PROPERTY_GETTER(PannerNodeHostObject, distanceModel),
      JSI_EXPORT_PROPERTY_GETTER(PannerNodeHostObject, refDistance),
      JSI_EXPORT_PROPERTY_GETTER(PannerNodeHostObject, maxDistance),
      JSI_EXPORT_PROPERTY_GETTER(PannerNodeHostObject, rolloffFactor),
      JSI_EXPORT_PROPERTY_GETTER(PannerNodeHostObject, coneInnerAngle),
      JSI_EXPORT_PROPERTY_GETTER(PannerNodeHostObject, coneOuterAngle),
      JSI_EXPORT_PROPERTY_GETTER(PannerNodeHostObject, coneOuterGain));

  addSetters(
      JSI_EXPORT_PROPERTY_SETTER(PannerNodeHostObject, panningModel),
      JSI_EXPORT_PROPERTY_SETTER(PannerNodeHostObject, distanceModel),
      JSI_EXPORT_PROPERTY_SETTER(PannerNodeHostObject, refDistance),
      JSI_EXPORT_PROPERTY_SETTER(PannerNodeHostObject, maxDistance),
      JSI_EXPORT_PROPERTY_SETTER(PannerNodeHostObject, rolloffFactor),
      JSI_EXPORT_PROPERTY_SETTER(PannerNodeHostObject, coneInnerAngle),
      JSI_EXPORT_PROPERTY_SETTER(PannerNodeHostObject, coneOuterAngle),
      JSI_EXPORT_PROPERTY_SETTER(PannerNodeHostObject, coneOuterGain));
}

JSI_PROPERTY_GETTER_IMPL(PannerNodeHostObject, positionX) {
  return jsi::Object::createFromHostObject(runtime, positionXParam_);
}

JSI_PROPERTY_GETTER_IMPL(PannerNodeHostObject, positionY) {
  return jsi::Object::createFromHostObject(runtime, positionYParam_);
}

JSI_PROPERTY_GETTER_IMPL(PannerNodeHostObject, positionZ) {
  return jsi::Object::createFromHostObject(runtime, positionZParam_);
}

JSI_PROPERTY_GETTER_IMPL(PannerNodeHostObject, orientationX) {
  return jsi::Object::createFromHostObject(runtime, orientationXParam_);
}

JSI_PROPERTY_GETTER_IMPL(PannerNodeHostObject, orientationY) {
  return jsi::Object::createFromHostObject(runtime, orientationYParam_);
}

JSI_PROPERTY_GETTER_IMPL(PannerNodeHostObject, orientationZ) {
  return jsi::Object::createFromHostObject(runtime, orientationZParam_);
}

JSI_PROPERTY_GETTER_IMPL(PannerNodeHostObject, panningModel) {
  return jsi::String::createFromUtf8(runtime, js_enum_parser::panningModelToString(panningModel_));
}

JSI_PROPERTY_SETTER_IMPL(PannerNodeHostObject, panningModel) {
  const auto modelStr = value.toString(runtime).utf8(runtime);
  if (modelStr == "HRTF") {
    throw jsi::JSError(
        runtime, "NotSupportedError: panningModel 'HRTF' is not supported yet; use 'equalpower'");
  }

  PanningModelType parsedModel;
  try {
    parsedModel = js_enum_parser::panningModelFromString(modelStr);
  } catch (const std::invalid_argument &) {
    return;
  }
  panningModel_ = parsedModel;
  auto event = [node = pannerNode_, parsedModel](BaseAudioContext &) {
    node->setPanningModel(parsedModel);
  };
  pannerNode_->scheduleAudioEvent(std::move(event));
}

JSI_PROPERTY_GETTER_IMPL(PannerNodeHostObject, distanceModel) {
  return jsi::String::createFromUtf8(
      runtime, js_enum_parser::distanceModelToString(distanceModel_));
}

JSI_PROPERTY_SETTER_IMPL(PannerNodeHostObject, distanceModel) {
  DistanceModelType parsedModel;
  try {
    parsedModel = js_enum_parser::distanceModelFromString(value.asString(runtime).utf8(runtime));
  } catch (const std::invalid_argument &) {
    return;
  }
  distanceModel_ = parsedModel;
  auto event = [node = pannerNode_, parsedModel](BaseAudioContext &) {
    node->setDistanceModel(parsedModel);
  };
  pannerNode_->scheduleAudioEvent(std::move(event));
}

JSI_PROPERTY_GETTER_IMPL(PannerNodeHostObject, refDistance) {
  return refDistance_;
}

JSI_PROPERTY_SETTER_IMPL(PannerNodeHostObject, refDistance) {
  if (!value.isNumber()) {
    return;
  }
  const double distance = value.getNumber();
  if (distance < 0.0) {
    throw jsi::JSError(runtime, "refDistance cannot be set to a negative value");
  }
  refDistance_ = distance;
  auto event = [node = pannerNode_, distance](BaseAudioContext &) {
    node->setRefDistance(distance);
  };
  pannerNode_->scheduleAudioEvent(std::move(event));
}

JSI_PROPERTY_GETTER_IMPL(PannerNodeHostObject, maxDistance) {
  return maxDistance_;
}

JSI_PROPERTY_SETTER_IMPL(PannerNodeHostObject, maxDistance) {
  if (!value.isNumber()) {
    return;
  }
  const double distance = value.getNumber();
  if (distance <= 0.0) {
    throw jsi::JSError(runtime, "maxDistance cannot be set to a non-positive value");
  }
  maxDistance_ = distance;
  auto event = [node = pannerNode_, distance](BaseAudioContext &) {
    node->setMaxDistance(distance);
  };
  pannerNode_->scheduleAudioEvent(std::move(event));
}

JSI_PROPERTY_GETTER_IMPL(PannerNodeHostObject, rolloffFactor) {
  return rolloffFactor_;
}

JSI_PROPERTY_SETTER_IMPL(PannerNodeHostObject, rolloffFactor) {
  if (!value.isNumber()) {
    return;
  }
  const double factor = value.getNumber();
  if (factor < 0.0) {
    throw jsi::JSError(runtime, "rolloffFactor cannot be set to a negative value");
  }
  rolloffFactor_ = factor;
  auto event = [node = pannerNode_, factor](BaseAudioContext &) {
    node->setRolloffFactor(factor);
  };
  pannerNode_->scheduleAudioEvent(std::move(event));
}

JSI_PROPERTY_GETTER_IMPL(PannerNodeHostObject, coneInnerAngle) {
  return coneInnerAngle_;
}

JSI_PROPERTY_SETTER_IMPL(PannerNodeHostObject, coneInnerAngle) {
  if (!value.isNumber()) {
    return;
  }
  const double angle = value.getNumber();
  coneInnerAngle_ = angle;
  auto event = [node = pannerNode_, angle](BaseAudioContext &) {
    node->setConeInnerAngle(angle);
  };
  pannerNode_->scheduleAudioEvent(std::move(event));
}

JSI_PROPERTY_GETTER_IMPL(PannerNodeHostObject, coneOuterAngle) {
  return coneOuterAngle_;
}

JSI_PROPERTY_SETTER_IMPL(PannerNodeHostObject, coneOuterAngle) {
  if (!value.isNumber()) {
    return;
  }
  const double angle = value.getNumber();
  coneOuterAngle_ = angle;
  auto event = [node = pannerNode_, angle](BaseAudioContext &) {
    node->setConeOuterAngle(angle);
  };
  pannerNode_->scheduleAudioEvent(std::move(event));
}

JSI_PROPERTY_GETTER_IMPL(PannerNodeHostObject, coneOuterGain) {
  return coneOuterGain_;
}

JSI_PROPERTY_SETTER_IMPL(PannerNodeHostObject, coneOuterGain) {
  if (!value.isNumber()) {
    return;
  }
  const double gain = value.getNumber();
  if (gain < 0.0 || gain > 1.0) {
    throw jsi::JSError(runtime, "coneOuterGain must be between 0 and 1");
  }
  coneOuterGain_ = gain;
  auto event = [node = pannerNode_, gain](BaseAudioContext &) {
    node->setConeOuterGain(gain);
  };
  pannerNode_->scheduleAudioEvent(std::move(event));
}

} // namespace audioapi
