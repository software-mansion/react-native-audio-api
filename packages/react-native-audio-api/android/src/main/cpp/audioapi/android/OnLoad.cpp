#include <audioapi/android/AudioAPIModule.h>
#include <audioapi/android/system/NativeRecorderControl.hpp>

#include <fbjni/fbjni.h>

using namespace audioapi;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *) {
  return facebook::jni::initialize(vm, [] {
    AudioAPIModule::registerNatives();
    NativeRecorderControl::registerNatives();
  });
}
