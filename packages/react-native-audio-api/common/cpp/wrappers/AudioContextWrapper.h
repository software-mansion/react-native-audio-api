#pragma once

#include <memory>
#include <string>
#include <utility>

#include "AudioBufferSourceNodeWrapper.h"
#include "AudioBufferWrapper.h"
#include "AudioDestinationNodeWrapper.h"
#include "BiquadFilterNodeWrapper.h"
#include "GainNodeWrapper.h"
#include "OscillatorNodeWrapper.h"
#include "StereoPannerNodeWrapper.h"

#ifdef ANDROID
#include "AudioContext.h"
#else
#include "IOSAudioContext.h"
#endif

namespace audioapi {

class AudioContextWrapper {
#ifdef ANDROID

 public:
  explicit AudioContextWrapper(
      const std::shared_ptr<AudioContext> &audiocontext);

 private:
  std::shared_ptr<AudioContext> audioContext_;
#else

 public:
  AudioContextWrapper();

 private:
  std::shared_ptr<IOSAudioContext> audioContext_;
#endif

 public:
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
  void close() const;

 private:
  std::shared_ptr<AudioDestinationNodeWrapper> destination_;
};
} // namespace audioapi
