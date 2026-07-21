#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/PannerNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>

#include <cfloat>
#include <memory>

namespace audioapi {

PannerNode::PannerNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const PannerOptions &options)
    : AudioNode(context, options),
      positionXParam_(std::make_shared<AudioParam>(options.positionX, -FLT_MAX, FLT_MAX, context)),
      positionYParam_(std::make_shared<AudioParam>(options.positionY, -FLT_MAX, FLT_MAX, context)),
      positionZParam_(std::make_shared<AudioParam>(options.positionZ, -FLT_MAX, FLT_MAX, context)),
      orientationXParam_(
          std::make_shared<AudioParam>(options.orientationX, -FLT_MAX, FLT_MAX, context)),
      orientationYParam_(
          std::make_shared<AudioParam>(options.orientationY, -FLT_MAX, FLT_MAX, context)),
      orientationZParam_(
          std::make_shared<AudioParam>(options.orientationZ, -FLT_MAX, FLT_MAX, context)),
      panningModel_(options.panningModel),
      distanceModel_(options.distanceModel),
      refDistance_(options.refDistance),
      maxDistance_(options.maxDistance),
      rolloffFactor_(options.rolloffFactor),
      coneInnerAngle_(options.coneInnerAngle),
      coneOuterAngle_(options.coneOuterAngle),
      coneOuterGain_(options.coneOuterGain),
      outputBuffer_(
          std::make_shared<DSPAudioBuffer>(
              RENDER_QUANTUM_SIZE,
              channelCount_,
              context->getSampleRate())) {}

std::shared_ptr<AudioParam> PannerNode::getPositionXParam() const {
  return positionXParam_;
}

std::shared_ptr<AudioParam> PannerNode::getPositionYParam() const {
  return positionYParam_;
}

std::shared_ptr<AudioParam> PannerNode::getPositionZParam() const {
  return positionZParam_;
}

std::shared_ptr<AudioParam> PannerNode::getOrientationXParam() const {
  return orientationXParam_;
}

std::shared_ptr<AudioParam> PannerNode::getOrientationYParam() const {
  return orientationYParam_;
}

std::shared_ptr<AudioParam> PannerNode::getOrientationZParam() const {
  return orientationZParam_;
}

void PannerNode::setPanningModel(PanningModelType model) {
  panningModel_ = model;
}

PanningModelType PannerNode::getPanningModel() const {
  return panningModel_;
}

void PannerNode::setDistanceModel(DistanceModelType model) {
  distanceModel_ = model;
}

DistanceModelType PannerNode::getDistanceModel() const {
  return distanceModel_;
}

void PannerNode::setRefDistance(double distance) {
  refDistance_ = distance;
}

double PannerNode::getRefDistance() const {
  return refDistance_;
}

void PannerNode::setMaxDistance(double distance) {
  maxDistance_ = distance;
}

double PannerNode::getMaxDistance() const {
  return maxDistance_;
}

void PannerNode::setRolloffFactor(double factor) {
  rolloffFactor_ = factor;
}

double PannerNode::getRolloffFactor() const {
  return rolloffFactor_;
}

void PannerNode::setConeInnerAngle(double angle) {
  coneInnerAngle_ = angle;
}

double PannerNode::getConeInnerAngle() const {
  return coneInnerAngle_;
}

void PannerNode::setConeOuterAngle(double angle) {
  coneOuterAngle_ = angle;
}

double PannerNode::getConeOuterAngle() const {
  return coneOuterAngle_;
}

void PannerNode::setConeOuterGain(double gain) {
  coneOuterGain_ = gain;
}

double PannerNode::getConeOuterGain() const {
  return coneOuterGain_;
}

std::shared_ptr<DSPAudioBuffer> PannerNode::getOutputBuffer() const {
  return outputBuffer_;
}

std::shared_ptr<DSPAudioBuffer> PannerNode::getNegotiatedBuffer() const {
  return getInputBuffer();
}

void PannerNode::setNegotiatedBuffer(const std::shared_ptr<DSPAudioBuffer> &buffer) {
  audioBuffer_ = buffer;
}

size_t PannerNode::getUpstreamChannelCount(size_t /*negotiatedChannelCount*/) const {
  return outputBuffer_->getNumberOfChannels();
}

void PannerNode::processNode(int framesToProcess) {
  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr || audioBuffer_ == nullptr) {
    return;
  }

  // TODO (Krok 3): Tutaj zaimplementujemy matematykę 3D, tłumienie dystansu (Inverse)
  // oraz podział na kanały algorytmem EqualPower. Na czas testu kompilacji
  // zostawiamy pusty przebieg — węzeł wyprodukuje bezpieczną ciszę w outputBuffer_.
}

} // namespace audioapi