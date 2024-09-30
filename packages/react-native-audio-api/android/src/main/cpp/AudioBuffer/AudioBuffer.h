#pragma once

#include <fbjni/fbjni.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <memory>
#include <string>
#include <vector>

namespace audioapi {

using namespace facebook;
using namespace facebook::jni;

class AudioBuffer : public jni::HybridClass<AudioBuffer> {
 public:
  static auto constexpr kJavaDescriptor =
      "Lcom/swmansion/audioapi/utils/AudioBuffer;";

  static jni::local_ref<AudioBuffer::jhybriddata> initHybrid(
      jni::alias_ref<jhybridobject> jThis) {
    return makeCxxInstance(jThis);
  }

  static void registerNatives() {
    registerHybrid({
        makeNativeMethod("initHybrid", AudioBuffer::initHybrid),
    });
  }

  int getSampleRate() const;
  int getLength() const;
  double getDuration() const;
  int getNumberOfChannels() const;
  float *getChannelData(int channel) const;
  void setChannelData(int channel, const float *data) const;

 public:
  friend HybridBase;

  global_ref<AudioBuffer::javaobject> javaPart_;

  explicit AudioBuffer(jni::alias_ref<AudioBuffer::jhybridobject> &jThis);
};

} // namespace audioapi
