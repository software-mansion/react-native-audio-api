#pragma once

#include <audioapi/core/AudioParam.h>
#include <audioapi/core/effects/PeriodicWave.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/core/types/OscillatorType.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

namespace audioapi {

class AudioBus;
class OscillatorOptions;

class OscillatorNode : public AudioScheduledSourceNode {
 public:
  explicit OscillatorNode(
      std::shared_ptr<BaseAudioContext> context,
      const OscillatorOptions &options);

  [[nodiscard]] std::shared_ptr<AudioParam> getFrequencyParam() const;
  [[nodiscard]] std::shared_ptr<AudioParam> getDetuneParam() const;
  [[nodiscard]] OscillatorType getType();
  void setType(OscillatorType);
  void setPeriodicWave(const std::shared_ptr<PeriodicWave> &periodicWave);

 protected:
  std::shared_ptr<AudioBus> processNode(
      const std::shared_ptr<AudioBus> &processingBus,
      int framesToProcess) override;

 private:
  std::shared_ptr<AudioParam> frequencyParam_;
  std::shared_ptr<AudioParam> detuneParam_;
  OscillatorType type_;
  float phase_ = 0.0;
  std::shared_ptr<PeriodicWave> periodicWave_;
};
} // namespace audioapi
