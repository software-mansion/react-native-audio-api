package com.audiocontext.utils

import com.facebook.jni.HybridData


class AudioBuffer(sampleRate: Int, length: Int, numberOfChannels: Int) {
  val sampleRate: Int = sampleRate
    get() = field
  val length: Int = length
    get() = field
  private val duration: Double = length.toDouble() / sampleRate
    get() = field
  val numberOfChannels: Int = numberOfChannels
    get() = field
  private val channels: Array<ShortArray> = Array(numberOfChannels) { ShortArray(length) }

  private val mHybridData: HybridData?

  companion object {
    init {
      System.loadLibrary("react-native-audio-context")
    }
  }

  init {
    mHybridData = initHybrid()
  }

  external fun initHybrid(): HybridData?

  fun getChannelData(channel: Int): ShortArray {
    if (channel < 0 || channel >= numberOfChannels) {
      throw IllegalArgumentException("Channel index out of bounds")
    }

    return channels[channel]
  }

  private fun setChannelData(channel: Int, data: ShortArray) {
    if (channel < 0 || channel >= numberOfChannels) {
      throw IllegalArgumentException("Channel index out of bounds")
    }

    if (data.size != length) {
      throw IllegalArgumentException("Data length does not match buffer length")
    }

    channels[channel] = data
  }

  fun copy(): AudioBuffer {
    val outputBuffer = AudioBuffer(sampleRate, length, numberOfChannels)

    for (i in 0 until numberOfChannels) {
      outputBuffer.setChannelData(i, channels[i].copyOf())
    }

    return outputBuffer
  }

  fun mix(outputNumberOfChannels: Int): AudioBuffer {
    if (this.numberOfChannels == outputNumberOfChannels) {
      return this
    }

    when(this.numberOfChannels) {
      1 -> {
        if (outputNumberOfChannels == 2) {
          val outputBuffer = AudioBuffer(sampleRate, length, 2)
          outputBuffer.setChannelData(0, channels[0])
          outputBuffer.setChannelData(1, channels[0])

          return outputBuffer
        }
      }
      2 -> {
        if (outputNumberOfChannels == 1) {
          val outputBuffer = AudioBuffer(sampleRate, length, 1)
          val outputData = ShortArray(length)

          for (i in 0 until length) {
            outputData[i] = ((channels[0][i] + channels[1][i]) / 2).toShort()
          }

          return outputBuffer
        }
      }
    }

    throw IllegalArgumentException("Unsupported number of channels")
  }
}

