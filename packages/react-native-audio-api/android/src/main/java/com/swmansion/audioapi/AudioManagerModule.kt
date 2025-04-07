package com.swmansion.audioapi

import android.content.Context
import android.media.AudioManager
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactContextBaseJavaModule
import com.facebook.react.bridge.ReactMethod
import com.facebook.react.bridge.ReadableArray
import com.facebook.react.bridge.ReadableMap
import com.swmansion.audioapi.system.MusicControlModule

class AudioManagerModule(
  reactContext: ReactApplicationContext,
) : ReactContextBaseJavaModule(reactContext) {
  companion object {
    const val NAME = "AudioManagerModule"
  }

  private val audioManager: AudioManager = reactContext.getSystemService(Context.AUDIO_SERVICE) as AudioManager
  private val musicControl = MusicControlModule(reactContext)

  init {
    try {
      System.loadLibrary("react-native-audio-api")
    } catch (exception: UnsatisfiedLinkError) {
      throw RuntimeException("Could not load native module AudioAPIModule", exception)
    }
  }

  @ReactMethod
  fun setLockScreenInfo(info: ReadableMap?) {
    musicControl.setNowPlaying(info)
  }

  @ReactMethod
  fun resetLockScreenInfo() {
    musicControl.resetNowPlaying()
  }

  @ReactMethod
  fun enableRemoteCommand(
    name: String?,
    enabled: Boolean,
  ) {
  }

  @ReactMethod
  fun setAudioSessionOptions(
    category: String?,
    mode: String?,
    options: ReadableArray?,
    active: Boolean,
  ) {
  }

  @ReactMethod
  fun getDevicePreferredSampleRate(): Double {
    return this.audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE).toDouble()
  }

  override fun getName(): String = NAME
}
