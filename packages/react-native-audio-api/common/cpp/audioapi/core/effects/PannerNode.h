#pragma once

#include <memory>

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioBuffer.hpp>

namespace audioapi {

class AudioListener;

class PannerNode : public AudioNode {
 public:
  PannerNode(
      const std::shared_ptr<BaseAudioContext> &context,
      AudioListener *listener,
      const PannerOptions &options);

  ~PannerNode() override = default;

  [[nodiscard]] std::shared_ptr<AudioParam> getPositionXParam() const {
    return positionXParam_;
  }
  [[nodiscard]] std::shared_ptr<AudioParam> getPositionYParam() const {
    return positionYParam_;
  }
  [[nodiscard]] std::shared_ptr<AudioParam> getPositionZParam() const {
    return positionZParam_;
  }
  [[nodiscard]] std::shared_ptr<AudioParam> getOrientationXParam() const {
    return orientationXParam_;
  }
  [[nodiscard]] std::shared_ptr<AudioParam> getOrientationYParam() const {
    return orientationYParam_;
  }
  [[nodiscard]] std::shared_ptr<AudioParam> getOrientationZParam() const {
    return orientationZParam_;
  }

  void setPanningModel(PanningModelType model) {
    panningModel_ = model;
  }
  [[nodiscard]] PanningModelType getPanningModel() const {
    return panningModel_;
  }

  void setDistanceModel(DistanceModelType model) {
    distanceModel_ = model;
  }
  [[nodiscard]] DistanceModelType getDistanceModel() const {
    return distanceModel_;
  }

  void setRefDistance(double distance) {
    refDistance_ = distance;
  }
  [[nodiscard]] double getRefDistance() const {
    return refDistance_;
  }

  void setMaxDistance(double distance) {
    maxDistance_ = distance;
  }
  [[nodiscard]] double getMaxDistance() const {
    return maxDistance_;
  }

  void setRolloffFactor(double factor) {
    rolloffFactor_ = factor;
  }
  [[nodiscard]] double getRolloffFactor() const {
    return rolloffFactor_;
  }

  void setConeInnerAngle(double angle) {
    coneInnerAngle_ = angle;
  }
  [[nodiscard]] double getConeInnerAngle() const {
    return coneInnerAngle_;
  }

  void setConeOuterAngle(double angle) {
    coneOuterAngle_ = angle;
  }
  [[nodiscard]] double getConeOuterAngle() const {
    return coneOuterAngle_;
  }

  void setConeOuterGain(double gain) {
    coneOuterGain_ = gain;
  }
  [[nodiscard]] double getConeOuterGain() const {
    return coneOuterGain_;
  }

  [[nodiscard]] std::shared_ptr<DSPAudioBuffer> getOutputBuffer() const override {
    return outputBuffer_;
  }
  [[nodiscard]] std::shared_ptr<DSPAudioBuffer> getNegotiatedBuffer() const override {
    return getInputBuffer();
  }
  void setNegotiatedBuffer(const std::shared_ptr<DSPAudioBuffer> &buffer) override {
    audioBuffer_ = buffer;
  }
  [[nodiscard]] size_t getUpstreamChannelCount(size_t /*negotiatedChannelCount*/) const override {
    return outputBuffer_->getNumberOfChannels();
  }

 protected:
  void processNode(int framesToProcess) override;
  [[nodiscard]] const DSPAudioBuffer *getOutput() const override {
    return outputBuffer_.get();
  }

 private:
  AudioListener *listener_ = nullptr;

  const std::shared_ptr<AudioParam> positionXParam_;
  const std::shared_ptr<AudioParam> positionYParam_;
  const std::shared_ptr<AudioParam> positionZParam_;
  const std::shared_ptr<AudioParam> orientationXParam_;
  const std::shared_ptr<AudioParam> orientationYParam_;
  const std::shared_ptr<AudioParam> orientationZParam_;

  PanningModelType panningModel_;
  DistanceModelType distanceModel_;
  double refDistance_;
  double maxDistance_;
  double rolloffFactor_;
  double coneInnerAngle_;
  double coneOuterAngle_;
  double coneOuterGain_;

  const std::shared_ptr<DSPAudioBuffer> outputBuffer_;
};

} // namespace audioapi
