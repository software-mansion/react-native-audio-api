package com.swmansion.audioapi.utils

import android.util.Log
import com.facebook.jni.HybridData

class AudioBuffer(
  numberOfChannels: Int,
  length: Int,
  sampleRate: Int,
) {
  val sampleRate: Int = sampleRate
    get() = field
  val length: Int = length
    get() = field
  private val duration: Double = length.toDouble() / sampleRate
    get() = field
  val numberOfChannels: Int = numberOfChannels
    get() = field
  private val channels: Array<FloatArray> = Array(numberOfChannels) { FloatArray(length) }

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

  fun getChannelData(channel: Int): FloatArray {
    if (channel < 0 || channel >= numberOfChannels) {
      throw IllegalArgumentException("Channel index out of bounds")
    }

    return channels[channel]
  }

  private fun setChannelData(
    channel: Int,
    data: FloatArray,
  ) {
    if (channel < 0 || channel >= numberOfChannels) {
      throw IllegalArgumentException("Channel index out of bounds")
    }

    if (data.size != length) {
      throw IllegalArgumentException("Data length does not match buffer length")
    }

    channels[channel] = data
  }

  fun copy(): AudioBuffer {
    val outputBuffer = AudioBuffer(numberOfChannels, length, sampleRate)

    for (i in 0 until numberOfChannels) {
      outputBuffer.setChannelData(i, channels[i].copyOf())
    }

    return outputBuffer
  }

  fun mix(outputNumberOfChannels: Int): AudioBuffer {
    if (this.numberOfChannels == outputNumberOfChannels) {
      return this
    }

    Log.d("AudioBuffer", "Mixing buffer with $numberOfChannels channels to $outputNumberOfChannels channels")

    when (this.numberOfChannels) {
      1 -> {
        if (outputNumberOfChannels == 2) {
          val outputBuffer = AudioBuffer(2, length, sampleRate)
          outputBuffer.setChannelData(0, channels[0])
          outputBuffer.setChannelData(1, channels[0])

          return outputBuffer
        }
      }
      2 -> {
        if (outputNumberOfChannels == 1) {
          val outputBuffer = AudioBuffer(1, length, sampleRate)
          val outputData = FloatArray(length)

          for (i in 0 until length) {
            outputData[i] = (channels[0][i] + channels[1][i]) / 2
          }

          return outputBuffer
        }
      }
    }

    throw IllegalArgumentException("Unsupported number of channels")
  }
}
