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

#ifdef ANDROID
using namespace facebook;
class AudioContext;
#endif

class AudioContextWrapper {
#ifdef ANDROID

 private:
  AudioContext *audiocontext_;

 public:
  explicit AudioContextWrapper(AudioContext *audiocontext);
#else

 private:
  std::shared_ptr<IOSAudioContext> audiocontext_;

 public:
  AudioContextWrapper();
#endif

 private:
  std::shared_ptr<AudioDestinationNodeWrapper> destinationNode_;
  int sampleRate_;

 public:
  std::shared_ptr<OscillatorNodeWrapper> createOscillator();
  std::shared_ptr<AudioDestinationNodeWrapper> getDestination() const;
  std::shared_ptr<GainNodeWrapper> createGain();
  std::shared_ptr<StereoPannerNodeWrapper> createStereoPanner();
  std::shared_ptr<BiquadFilterNodeWrapper> createBiquadFilter();
  std::shared_ptr<AudioBufferSourceNodeWrapper> createBufferSource();
  std::shared_ptr<AudioBufferWrapper>
  createBuffer(int numberOfChannels, int length, int sampleRate);
  std::string getState();
  int getSampleRate() const;
  double getCurrentTime();
  void close();
};
} // namespace audioapi
