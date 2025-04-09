package com.swmansion.audioapi

import android.content.Context
import android.media.AudioManager
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactContextBaseJavaModule
import com.facebook.react.bridge.ReactMethod
import com.facebook.react.bridge.ReadableArray
import com.facebook.react.bridge.ReadableMap
import com.swmansion.audioapi.system.LockScreenManager
import com.swmansion.audioapi.system.MediaSessionManager

class AudioManagerModule(
  reactContext: ReactApplicationContext,
) : ReactContextBaseJavaModule(reactContext) {
  companion object {
    const val NAME = "AudioManagerModule"
  }

  private val audioManager: AudioManager = reactContext.getSystemService(Context.AUDIO_SERVICE) as AudioManager
  private val mediaSessionManager: MediaSessionManager = MediaSessionManager(reactContext)

  init {
    try {
      System.loadLibrary("react-native-audio-api")
    } catch (exception: UnsatisfiedLinkError) {
      throw RuntimeException("Could not load native module AudioAPIModule", exception)
    }
  }

  @ReactMethod
  fun setLockScreenInfo(info: ReadableMap?) {
    mediaSessionManager.setLockScreenInfo(info)
  }

  @ReactMethod
  fun resetLockScreenInfo() {
    mediaSessionManager.resetLockScreenInfo()
  }

  @ReactMethod
  fun enableRemoteCommand(
    name: String,
    enabled: Boolean,
  ) {
    mediaSessionManager.enableRemoteCommand(name, enabled)
  }

  @ReactMethod
  fun setAudioSessionOptions(
    category: String?,
    mode: String?,
    options: ReadableArray?,
    active: Boolean,
  ) {
  }

  @ReactMethod(isBlockingSynchronousMethod = true)
  fun getDevicePreferredSampleRate(): Double {
    val sampleRate = this.audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE)
    return sampleRate.toDouble()
  }

  override fun getName(): String = NAME
}
