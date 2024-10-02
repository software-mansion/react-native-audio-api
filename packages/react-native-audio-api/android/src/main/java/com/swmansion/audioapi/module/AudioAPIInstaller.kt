package com.swmansion.audioapi.module

import com.facebook.jni.HybridData

class AudioAPIInstaller {
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
}
