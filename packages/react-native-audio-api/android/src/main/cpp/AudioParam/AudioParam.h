#pragma once

#include <fbjni/fbjni.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <memory>

namespace audioapi {

using namespace facebook;
using namespace facebook::jni;

class AudioParam : public jni::HybridClass<AudioParam> {
 public:
  static auto constexpr kJavaDescriptor =
      "Lcom/swmansion/audioapi/parameters/AudioParam;";

  static jni::local_ref<AudioParam::jhybriddata> initHybrid(
      jni::alias_ref<jhybridobject> jThis) {
    return makeCxxInstance(jThis);
  }

  static void registerNatives() {
    registerHybrid({
        makeNativeMethod("initHybrid", AudioParam::initHybrid),
    });
  }

  double getValue();
  void setValue(double value);
  double getDefaultValue();
  double getMinValue();
  double getMaxValue();
  void setValueAtTime(double value, double startTime);
  void linearRampToValueAtTime(double value, double endTime);
  void exponentialRampToValueAtTime(double value, double endTime);
  void resetJavaPart();

 protected:
  friend HybridBase;

  global_ref<AudioParam::javaobject> javaPart_;

  explicit AudioParam(jni::alias_ref<AudioParam::jhybridobject> &jThis);
};

} // namespace audioapi
