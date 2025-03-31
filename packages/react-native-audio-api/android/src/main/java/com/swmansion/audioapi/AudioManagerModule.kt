package com.swmansion.audioapi

import android.content.Context
import android.media.AudioManager
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReadableArray
import com.facebook.react.bridge.ReadableMap
import com.facebook.react.common.annotations.FrameworkAPI
import com.facebook.react.module.annotations.ReactModule

@ReactModule(name = AudioManagerModule.NAME)
class AudioManagerModule(
  reactContext: ReactApplicationContext,
) : NativeAudioManagerModuleSpec(reactContext) {
  companion object {
    const val NAME = NativeAudioManagerModuleSpec.NAME
  }

  init {
    try {
      System.loadLibrary("react-native-audio-api")
    } catch (exception: UnsatisfiedLinkError) {
      throw RuntimeException("Could not load native module AudioAPIModule", exception)
    }
  }

  override fun setLockScreenInfo(info: ReadableMap?) {
  }

  override fun resetLockScreenInfo() {
  }

  override fun enableRemoteCommand(name: String?, enabled: Boolean) {
  }

  override fun setAudioSessionOptions(
    category: String?,
    mode: String?,
    options: ReadableArray?,
    active: Boolean
  ) {
  }

  override fun getDevicePreferredSampleRate(): Double {
    val audioManager = reactApplicationContext.getSystemService(Context.AUDIO_SERVICE) as AudioManager
    return audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE).toDouble()
  }
}
