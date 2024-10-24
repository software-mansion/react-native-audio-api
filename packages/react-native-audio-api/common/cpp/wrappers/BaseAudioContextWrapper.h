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
#include "StereoPannerNodeWrapper.h"

namespace audioapi {

class BaseAudioContextWrapper {
 public:
  explicit BaseAudioContextWrapper(
      const std::shared_ptr<BaseAudioContext> &context);

  std::shared_ptr<AudioDestinationNodeWrapper> getDestination() const;
  std::shared_ptr<OscillatorNodeWrapper> createOscillator() const;
  std::shared_ptr<GainNodeWrapper> createGain() const;
  std::shared_ptr<StereoPannerNodeWrapper> createStereoPanner() const;
  std::shared_ptr<BiquadFilterNodeWrapper> createBiquadFilter() const;
  std::shared_ptr<AudioBufferSourceNodeWrapper> createBufferSource() const;
  std::shared_ptr<AudioBufferWrapper>
  createBuffer(int numberOfChannels, int length, int sampleRate) const;
  std::string getState() const;
  int getSampleRate() const;
  double getCurrentTime() const;

 protected:
  std::shared_ptr<AudioDestinationNodeWrapper> destination_;
  std::shared_ptr<BaseAudioContext> context_;
};
} // namespace audioapi
