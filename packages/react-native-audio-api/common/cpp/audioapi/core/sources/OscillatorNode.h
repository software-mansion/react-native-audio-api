#pragma once

#include <audioapi/core/AudioParam.h>
#include <audioapi/core/effects/PeriodicWave.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/core/types/OscillatorType.h>

#include <memory>

namespace audioapi {

struct OscillatorOptions;

class OscillatorNode : public AudioScheduledSourceNode {
 public:
  explicit OscillatorNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const OscillatorOptions &options);

  [[nodiscard]] std::shared_ptr<AudioParam> getFrequencyParam() const;
  [[nodiscard]] std::shared_ptr<AudioParam> getDetuneParam() const;

  /// @note Audio Thread only
  void setType(OscillatorType);

  /// @note Audio Thread only
  void setPeriodicWave(const std::shared_ptr<PeriodicWave> &periodicWave);

 protected:
  void processNode(int framesToProcess) override;

 private:
  std::shared_ptr<AudioParam> frequencyParam_;
  std::shared_ptr<AudioParam> detuneParam_;
  OscillatorType type_;
  float phase_ = 0.0;
  std::shared_ptr<PeriodicWave> periodicWave_;
};
} // namespace audioapi
