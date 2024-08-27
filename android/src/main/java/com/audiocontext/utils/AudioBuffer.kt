package com.audiocontext.utils
import com.facebook.jni.HybridData


class AudioBuffer(sampleRate: Int, length: Int, numberOfChannels: Int) {
  val sampleRate = sampleRate
    get() = field
  val length = length
    get() = field
  private val duration = length / sampleRate
 val numberOfChannels = numberOfChannels
    get() = field
  private val channels = Array(numberOfChannels) { ShortArray(length) }

  private val mHybridData: HybridData?;

  companion object {
    init {
      System.loadLibrary("react-native-audio-context")
    }
  }

  init {
    mHybridData = initHybrid()
  }

  fun getChannelData(channel: Int): ShortArray {
    if (channel < 0 || channel >= numberOfChannels) {
      throw IllegalArgumentException("Channel index out of bounds")
    }

    return channels[channel]
  }

  external fun initHybrid(): HybridData?
}

