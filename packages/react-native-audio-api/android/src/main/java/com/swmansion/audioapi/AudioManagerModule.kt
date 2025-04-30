package com.swmansion.audioapi

import com.facebook.react.bridge.Promise
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactContextBaseJavaModule
import com.facebook.react.bridge.ReactMethod
import com.facebook.react.bridge.ReadableArray
import com.facebook.react.bridge.ReadableMap
import com.swmansion.audioapi.system.MediaSessionManager

class AudioManagerModule(
  reactContext: ReactApplicationContext,
) : ReactContextBaseJavaModule(reactContext) {
  companion object {
    const val NAME = "AudioManagerModule"
  }

  init {
    try {
      System.loadLibrary("react-native-audio-api")
    } catch (exception: UnsatisfiedLinkError) {
      throw RuntimeException("Could not load native module AudioAPIModule", exception)
    }

    MediaSessionManager.initialize(reactContext)
  }

  @ReactMethod(isBlockingSynchronousMethod = true)
  fun setLockScreenInfo(info: ReadableMap?) {
    MediaSessionManager.setLockScreenInfo(info)
  }

  @ReactMethod(isBlockingSynchronousMethod = true)
  fun resetLockScreenInfo() {
    MediaSessionManager.resetLockScreenInfo()
  }

  @ReactMethod(isBlockingSynchronousMethod = true)
  fun enableRemoteCommand(
    name: String,
    enabled: Boolean,
  ) {
    MediaSessionManager.enableRemoteCommand(name, enabled)
  }

  @ReactMethod(isBlockingSynchronousMethod = true)
  fun setAudioSessionOptions(
    category: String?,
    mode: String?,
    options: ReadableArray?,
  ) {
    // Nothing to do here
  }

  @ReactMethod(isBlockingSynchronousMethod = true)
  fun getDevicePreferredSampleRate(): Double = MediaSessionManager.getDevicePreferredSampleRate()

  @ReactMethod(isBlockingSynchronousMethod = true)
  fun observeAudioInterruptions(enable: Boolean) = MediaSessionManager.observeAudioInterruptions(enable)

  @ReactMethod(isBlockingSynchronousMethod = true)
  fun observeVolumeChanges(enable: Boolean) = MediaSessionManager.observeVolumeChanges(enable)

  @ReactMethod
  fun requestRecordingPermissions(promise: Promise) {
    val res = MediaSessionManager.requestRecordingPermissions(currentActivity)
    promise.resolve(res)
  }

  @ReactMethod
  fun checkRecordingPermissions(promise: Promise) {
    val res = MediaSessionManager.checkRecordingPermissions()
    promise.resolve(res)
  }

  override fun getName(): String = NAME
}
