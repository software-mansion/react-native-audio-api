#include <fbjni/fbjni.h>
#include "OscillatorNode.h"
#include "AudioContext.h"
#include "AudioDestinationNode.h"

using namespace audiocontext;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
  return facebook::jni::initialize(vm, [] {
        OscillatorNode::registerNatives();
        AudioContext::registerNatives();
        AudioDestinationNode::registerNatives();
  });
}
