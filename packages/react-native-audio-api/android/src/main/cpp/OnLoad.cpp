#include <fbjni/fbjni.h>
#include "AudioAPIModule.h"

using namespace audioapi;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *) {
  return facebook::jni::initialize(
      vm, [] { AudioAPIModule::registerNatives(); });
}
