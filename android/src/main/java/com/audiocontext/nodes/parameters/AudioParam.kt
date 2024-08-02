package com.audiocontext.nodes.parameters

import com.facebook.jni.HybridData

class AudioParam(defaultValue: Double, maxValue: Double, minValue: Double) {
  private var value: Double = defaultValue
  private val defaultValue: Double = defaultValue
    get() = field
  private val maxValue: Double = maxValue
    get() = field
  private val minValue: Double = minValue
    get() = field

  private val mHybridData: HybridData?;

  companion object {
    init {
      System.loadLibrary("react-native-audio-context")
    }
  }

  init {
    mHybridData = initHybrid()
  }

  external fun initHybrid(): HybridData?

  fun getValue(): Double {
    return value
  }

  fun setValue(value: Double) {
    if(value > maxValue || value < minValue) {
      throw IllegalArgumentException("Value must be between $minValue and $maxValue")
    }
    this.value = value
  }
}
