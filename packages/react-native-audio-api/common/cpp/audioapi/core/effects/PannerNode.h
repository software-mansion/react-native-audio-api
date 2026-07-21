#pragma once

#include <memory>

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioBuffer.hpp>

namespace audioapi {

class PannerNode : public AudioNode {
 public:
  PannerNode(const std::shared_ptr<BaseAudioContext> &context, const PannerOptions &options);

  ~PannerNode() override = default;

  [[nodiscard]] std::shared_ptr<AudioParam> getPositionXParam() const;
  [[nodiscard]] std::shared_ptr<AudioParam> getPositionYParam() const;
  [[nodiscard]] std::shared_ptr<AudioParam> getPositionZParam() const;

  [[nodiscard]] std::shared_ptr<AudioParam> getOrientationXParam() const;
  [[nodiscard]] std::shared_ptr<AudioParam> getOrientationYParam() const;
  [[nodiscard]] std::shared_ptr<AudioParam> getOrientationZParam() const;

  void setPanningModel(PanningModelType model);
  [[nodiscard]] PanningModelType getPanningModel() const;

  void setDistanceModel(DistanceModelType model);
  [[nodiscard]] DistanceModelType getDistanceModel() const;

  void setRefDistance(double distance);
  [[nodiscard]] double getRefDistance() const;

  void setMaxDistance(double distance);
  [[nodiscard]] double getMaxDistance() const;

  void setRolloffFactor(double factor);
  [[nodiscard]] double getRolloffFactor() const;

  void setConeInnerAngle(double angle);
  [[nodiscard]] double getConeInnerAngle() const;

  void setConeOuterAngle(double angle);
  [[nodiscard]] double getConeOuterAngle() const;

  void setConeOuterGain(double gain);
  [[nodiscard]] double getConeOuterGain() const;

  [[nodiscard]] std::shared_ptr<DSPAudioBuffer> getOutputBuffer() const override;
  [[nodiscard]] std::shared_ptr<DSPAudioBuffer> getNegotiatedBuffer() const override;
  void setNegotiatedBuffer(const std::shared_ptr<DSPAudioBuffer> &buffer) override;
  [[nodiscard]] size_t getUpstreamChannelCount(size_t negotiatedChannelCount) const override;

 protected:
  void processNode(int framesToProcess) override;
  [[nodiscard]] const DSPAudioBuffer *getOutput() const override {
    return outputBuffer_.get();
  }

 private:
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