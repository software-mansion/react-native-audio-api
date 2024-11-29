#pragma once

#include <memory>
#include <string>
#include <utility>

#include "AudioBufferSourceNodeWrapper.h"
#include "AudioBufferWrapper.h"
#include "AudioDestinationNodeWrapper.h"
#include "BaseAudioContext.h"
#include "BiquadFilterNodeWrapper.h"
#include "GainNodeWrapper.h"
#include "OscillatorNodeWrapper.h"
#include "PeriodicWaveWrapper.h"
#include "StereoPannerNodeWrapper.h"

namespace audioapi {

class BaseAudioContextWrapper {
 public:
  explicit BaseAudioContextWrapper(
      const std::shared_ptr<BaseAudioContext> &context);

  [[nodiscard]] std::shared_ptr<AudioDestinationNodeWrapper> getDestination()
      const;
  [[nodiscard]] std::string getState() const;
  [[nodiscard]] int getSampleRate() const;
  [[nodiscard]] double getCurrentTime() const;
  [[nodiscard]] std::shared_ptr<OscillatorNodeWrapper> createOscillator() const;
  [[nodiscard]] std::shared_ptr<GainNodeWrapper> createGain() const;
  [[nodiscard]] std::shared_ptr<StereoPannerNodeWrapper> createStereoPanner()
      const;
  [[nodiscard]] std::shared_ptr<BiquadFilterNodeWrapper> createBiquadFilter()
      const;
  [[nodiscard]] std::shared_ptr<AudioBufferSourceNodeWrapper>
  createBufferSource() const;
  [[nodiscard]] std::shared_ptr<AudioBufferWrapper>
  createBuffer(int numberOfChannels, int length, int sampleRate) const;
  [[nodiscard]] std::shared_ptr<PeriodicWaveWrapper> createPeriodicWave(
      float *real,
      float *imag,
      bool disableNormalization,
      int length);
  std::shared_ptr<AudioBufferWrapper> decodeAudioDataSource(std::string source);

 protected:
  std::shared_ptr<AudioDestinationNodeWrapper> destination_;
  std::shared_ptr<BaseAudioContext> context_;
};
} // namespace audioapi
