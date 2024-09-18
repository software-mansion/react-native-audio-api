#include <fbjni/fbjni.h>
#include "AudioBuffer.h"
#include "AudioContext.h"
#include "AudioNode.h"
#include "AudioParam.h"
#include "AudioAPI.h"

using namespace audioapi;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
  return facebook::jni::initialize(vm, [] {
      AudioAPI::registerNatives();
    AudioContext::registerNatives();
    AudioNode::registerNatives();
    AudioParam::registerNatives();
    AudioBuffer::registerNatives();
  });
}
