#include <fbjni/fbjni.h>
#include "AudioAPIInstaller.h"
#include "AudioBuffer.h"
#include "AudioContext.h"

using namespace audioapi;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
  return facebook::jni::initialize(vm, [] {
    AudioAPIInstaller::registerNatives();
    AudioContext::registerNatives();
    AudioBuffer::registerNatives();
  });
}
