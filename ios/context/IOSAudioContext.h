#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <AudioContext.h>
#else // when compiled as C++
typedef struct objc_object AudioContext;
#endif // __OBJC__

#include <memory>
#include <string>
#include "IOSAudioBufferSourceNode.h"
#include "IOSAudioDestinationNode.h"
#include "IOSBiquadFilterNode.h"
#include "IOSGainNode.h"
#include "IOSOscillatorNode.h"
#include "IOSStereoPannerNode.h"

namespace audioapi {
class IOSAudioContext {
 public:
  AudioContext *audioContext_;

  explicit IOSAudioContext();
  ~IOSAudioContext();

  std::shared_ptr<IOSAudioDestinationNode> getDestination();
  std::shared_ptr<IOSOscillatorNode> createOscillator();
  std::shared_ptr<IOSGainNode> createGain();
  std::shared_ptr<IOSStereoPannerNode> createStereoPanner();
  std::shared_ptr<IOSBiquadFilterNode> createBiquadFilter();
  std::shared_ptr<IOSAudioBufferSourceNode> createBufferSource();
  //  std::shared_ptr<IOSAudioBufferSourceNode> createBufferSource();
  //  std::shared_ptr<IOSAudioBuffer>
  // createBuffer(int sampleRate, int length, int numberOfChannels);
  void close();
  double getCurrentTime();
  std::string getState();
  double getSampleRate();
};
} // namespace audioapi
