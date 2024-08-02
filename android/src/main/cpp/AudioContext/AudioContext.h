#pragma once

#include <fbjni/fbjni.h>
#include <jsi/jsi.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <memory>
#include <utility>

#include "AudioContextHostObject.h"
#include "AudioContextWrapper.h"
#include "OscillatorNode.h"
#include "AudioDestinationNode.h"
#include "GainNode.h"
#include "StereoPannerNode.h"

namespace audiocontext
{

  using namespace facebook;
  using namespace facebook::jni;

  class AudioContext : public jni::HybridClass<AudioContext>
  {
  public:
    static auto constexpr kJavaDescriptor = "Lcom/audiocontext/context/AudioContext;";

    static jni::local_ref<AudioContext::jhybriddata> initHybrid(jni::alias_ref<jhybridobject> jThis)
    {
      return makeCxxInstance(jThis);
    }

    static void registerNatives()
    {
      registerHybrid({
          makeNativeMethod("initHybrid", AudioContext::initHybrid),
      });

      javaClassLocal()->registerNatives({
          makeNativeMethod("install", AudioContext::install),
      });
    }

    std::shared_ptr<AudioDestinationNode> getDestination();
    std::shared_ptr<OscillatorNode> createOscillator();
    std::shared_ptr<GainNode> createGain();
    std::shared_ptr<StereoPannerNode> createStereoPanner();
    std::string getState();
    int getSampleRate();
    double getCurrentTime();
    void close();

    void install(jlong jsContext);

  private:
    friend HybridBase;

    global_ref<AudioContext::javaobject> javaObject_;
    std::shared_ptr<AudioDestinationNode> destinationNode_;

    explicit AudioContext(jni::alias_ref<AudioContext::jhybridobject> &jThis);
  };

} // namespace audiocontext
