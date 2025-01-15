#include <fbjni/fbjni.h>
#include "AudioAPIInstaller.h"

using namespace audioapi;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *) {
  return facebook::jni::initialize(
      vm, [] { AudioAPIInstaller::registerNatives(); });
}
