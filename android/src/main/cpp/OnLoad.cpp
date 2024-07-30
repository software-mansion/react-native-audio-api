#include <fbjni/fbjni.h>
#include "AudioContext.h"
#include "AudioNode.h"
#include "AudioParam.h"

using namespace audiocontext;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
  return facebook::jni::initialize(vm, [] {
        AudioContext::registerNatives();
        AudioNode::registerNatives();
        AudioParam::registerNatives();
  });
}
