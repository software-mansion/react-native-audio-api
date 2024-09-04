#include <fbjni/fbjni.h>
#include "AudioBuffer.h"
#include "AudioContext.h"
#include "AudioNode.h"
#include "AudioParam.h"
#include "AudioBuffer.h"

using namespace audiocontext;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
  return facebook::jni::initialize(vm, [] {
    AudioContext::registerNatives();
    AudioNode::registerNatives();
    AudioParam::registerNatives();
    AudioBuffer::registerNatives();
  });
}
