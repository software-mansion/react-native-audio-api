package com.swmansion.audioapi.utils

import kotlin.math.log10
import kotlin.math.log2

// https://webaudio.github.io/web-audio-api/

object Constants {
  const val SAMPLE_RATE: Int = 44100
  const val BUFFER_SIZE = 100

  private const val MOST_POSITIVE_SINGLE_FLOAT: Double = (Float.MAX_VALUE).toDouble()
  const val NYQUIST_FREQUENCY: Double = SAMPLE_RATE / 2.0
  val MAX_DETUNE: Double = 1200 * log2(MOST_POSITIVE_SINGLE_FLOAT)
  const val MAX_GAIN: Double = MOST_POSITIVE_SINGLE_FLOAT
  const val MAX_PAN: Double = 1.0
  const val MAX_FILTER_Q: Double = MOST_POSITIVE_SINGLE_FLOAT
  const val MAX_FILTER_FREQUENCY: Double = NYQUIST_FREQUENCY
  const val MIN_FILTER_FREQUENCY: Double = 0.0
  val MAX_FILTER_GAIN = 40 * log10(MOST_POSITIVE_SINGLE_FLOAT)
  const val MIN_FILTER_GAIN = -MAX_GAIN
}
