#pragma once

#include <fbjni/fbjni.h>
#include <jsi/jsi.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <memory>
#include <string>
#include <utility>

#include "AudioBuffer.h"
#include "AudioBufferSourceNode.h"
#include "AudioContextHostObject.h"
#include "AudioContextWrapper.h"
#include "AudioDestinationNode.h"
#include "BiquadFilterNode.h"
#include "GainNode.h"
#include "OscillatorNode.h"
#include "StereoPannerNode.h"

namespace audioapi {

using namespace facebook;
using namespace facebook::jni;

class AudioContext : public jni::HybridClass<AudioContext> {
 public:
  static auto constexpr kJavaDescriptor =
      "Lcom/swmansion/audioapi/context/AudioContext;";

  static jni::local_ref<AudioContext::jhybriddata> initHybrid(
      jni::alias_ref<jhybridobject> jThis) {
    return makeCxxInstance(jThis);
  }

  static void registerNatives() {
    registerHybrid({
        makeNativeMethod("initHybrid", AudioContext::initHybrid),
    });
  }

  std::shared_ptr<AudioDestinationNode> getDestination();
  std::shared_ptr<OscillatorNode> createOscillator();
  std::shared_ptr<GainNode> createGain();
  std::shared_ptr<StereoPannerNode> createStereoPanner();
  std::shared_ptr<BiquadFilterNode> createBiquadFilter();
  std::shared_ptr<AudioBufferSourceNode> createBufferSource();
  AudioBuffer *createBuffer(int numberOfChannels, int length, int sampleRate);
  std::string getState();
  int getSampleRate();
  double getCurrentTime();
  void close();

  void install(jlong jsContext);

 private:
  friend HybridBase;

  global_ref<AudioContext::javaobject> javaPart_;

  explicit AudioContext(jni::alias_ref<AudioContext::jhybridobject> &jThis);
};

} // namespace audioapi
