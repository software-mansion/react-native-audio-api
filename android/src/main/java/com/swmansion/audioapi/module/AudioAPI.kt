package com.swmansion.audioapi.module

import com.facebook.jni.HybridData
import com.swmansion.audioapi.context.AudioContext

class AudioAPI {
  private val mHybridData: HybridData?

  companion object {
    init {
      System.loadLibrary("react-native-audio-api")
    }
  }

  init {
    mHybridData = initHybrid()
  }

  external fun initHybrid(): HybridData?

  external fun install(jsContext: Long)

  fun createAudioContext(): AudioContext = AudioContext()
}
