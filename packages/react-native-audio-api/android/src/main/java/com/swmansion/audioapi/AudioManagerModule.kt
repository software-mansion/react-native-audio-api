package com.swmansion.audioapi

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

  private val mediaSessionManager: MediaSessionManager = MediaSessionManager(reactContext)

  init {
    try {
      System.loadLibrary("react-native-audio-api")
    } catch (exception: UnsatisfiedLinkError) {
      throw RuntimeException("Could not load native module AudioAPIModule", exception)
    }
  }

  @ReactMethod(isBlockingSynchronousMethod = true)
  fun setLockScreenInfo(info: ReadableMap?) {
    mediaSessionManager.setLockScreenInfo(info)
  }

  @ReactMethod(isBlockingSynchronousMethod = true)
  fun resetLockScreenInfo() {
    mediaSessionManager.resetLockScreenInfo()
  }

  @ReactMethod(isBlockingSynchronousMethod = true)
  fun enableRemoteCommand(
    name: String,
    enabled: Boolean,
  ) {
    mediaSessionManager.enableRemoteCommand(name, enabled)
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
  fun getDevicePreferredSampleRate(): Double = mediaSessionManager.getDevicePreferredSampleRate()

  @ReactMethod(isBlockingSynchronousMethod = true)
  fun observeAudioInterruptions(enable: Boolean) = mediaSessionManager.observeAudioInterruptions(enable)

  override fun getName(): String = NAME
}
