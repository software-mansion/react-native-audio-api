package com.audiocontext.nodes.filter

import android.os.Build
import androidx.annotation.RequiresApi
import com.audiocontext.context.BaseAudioContext
import com.audiocontext.nodes.audionode.AudioNode
import com.audiocontext.parameters.AudioParam
import com.audiocontext.parameters.PlaybackParameters
import com.audiocontext.utils.Constants
import kotlin.math.*

// https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html - math formulas for filters
// https://github.com/LabSound/LabSound/blob/main/src/internal/src/Biquad.cpp - implementation of filters on which I based mine

class BiquadFilterNode(context: BaseAudioContext): AudioNode(context) {
  override val numberOfInputs: Int = 1
  override val numberOfOutputs: Int = 1
  override val channelCount: Int = 2

  private val frequency: AudioParam = AudioParam(context, 350.0, Constants.MAX_FILTER_FREQUENCY, Constants.MIN_FILTER_FREQUENCY)
    get() = field
  private val detune: AudioParam = AudioParam(context, 0.0, Constants.MAX_DETUNE, -Constants.MAX_DETUNE)
    get() = field
  private val Q: AudioParam = AudioParam(context, 1.0, Constants.MAX_FILTER_Q, -Constants.MAX_FILTER_Q)
    get() = field
  private val gain: AudioParam = AudioParam(context, 0.0, Constants.MAX_FILTER_GAIN, Constants.MIN_FILTER_GAIN)
    get() = field
  private var filterType: FilterType = FilterType.LOWPASS

  // delayed samples
  private var y1 = 0.0
  private var y2 = 0.0
  private var x1 = 0.0
  private var x2 = 0.0
  // coefficients
  private var a1 = 1.0
  private var a2 = 0.0
  private var b0 = 1.0
  private var b1 = 0.0
  private var b2 = 0.0

  fun getFilterType(): String {
    return FilterType.toString(filterType)
  }

  fun setFilterType(type: String) {
    filterType = FilterType.fromString(type)
  }

  private fun resetCoefficients() {
    y1 = 0.0
    y2 = 0.0
    x1 = 0.0
    x2 = 0.0
  }

  private fun setNormalizedCoefficients(a0: Double, a1: Double, a2: Double, b0: Double, b1: Double, b2: Double) {
    this.a1 = a1 / a0
    this.a2 = a2 / a0
    this.b0 = b0 / a0
    this.b1 = b1 / a0
    this.b2 = b2 / a0
  }

  private fun setLowpassCoefficients(normalizedFrequency: Double, resonance: Double) {
    val frequency = max(0.0, min(normalizedFrequency, 1.0))
    val Q = max(0.0, resonance)

    if (frequency == 1.0) {
      setNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
      return
    }

    if (frequency <= 0.0) {
      setNormalizedCoefficients(1.0, 0.0, 0.0, 0.0, 0.0, 0.0)
      return
    }

    val w0 = 2.0 * PI * frequency
    val alpha = sin(w0) / (2.0 * Q)

    val b0 = (1.0 - cos(w0)) / 2.0
    val b1 = 1.0 - cos(w0)
    val b2 = (1.0 - cos(w0)) / 2.0
    val a0 = 1.0 + alpha
    val a1 = -2.0 * cos(w0)
    val a2 = 1.0 - alpha

    setNormalizedCoefficients(a0, a1, a2, b0, b1, b2)
  }

  private fun setHighpassCoefficients(normalizedFrequency: Double, resonance: Double) {
    val frequency = max(0.0, min(normalizedFrequency, 1.0))
    val Q = max(0.0, resonance)

    if (frequency == 1.0) {
      setNormalizedCoefficients(1.0, 0.0, 0.0, 0.0, 0.0, 0.0)
      return
    }

    if (frequency <= 0.0) {
      setNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
      return
    }

    val w0 = 2.0 * PI * frequency
    val alpha = sin(w0) / (2.0 * Q)

    val b0 = (1.0 + cos(w0)) / 2.0
    val b1 = -(1.0 + cos(w0))
    val b2 = (1.0 + cos(w0)) / 2.0
    val a0 = 1.0 + alpha
    val a1 = -2.0 * cos(w0)
    val a2 = 1.0 - alpha

    setNormalizedCoefficients(a0, a1, a2, b0, b1, b2)
  }

  private fun setBandpassCoefficients(normalizedFrequency: Double, resonance: Double) {
    val frequency = max(0.0, normalizedFrequency)
    val Q = max(0.0, resonance)

    if (frequency <= 0.0 || frequency >= 1.0) {
      setNormalizedCoefficients(1.0, 0.0, 0.0, 0.0, 0.0, 0.0)
      return
    }

    if (resonance <= 0.0) {
      setNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
      return
    }

    val w0 = 2.0 * PI * frequency
    val alpha = sin(w0) / (2.0 * Q)

    val b0 = alpha * Q
    val b1 = 0.0
    val b2 = -alpha * Q
    val a0 = 1.0 + alpha
    val a1 = -2.0 * cos(w0)
    val a2 = 1.0 - alpha

    setNormalizedCoefficients(a0, a1, a2, b0, b1, b2)
  }

  private fun setLowshelfCoefficients(normalizedFrequency: Double, dbGain: Double) {
    val frequency = max(0.0, min(1.0, normalizedFrequency))
    val A = 10.0.pow(dbGain / 40.0)

    if (frequency == 1.0) {
      setNormalizedCoefficients(1.0, 0.0, 0.0, A * A, 0.0, 0.0)
      return
    }

    if (frequency <= 0.0) {
      setNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
      return
    }

    val w0 = 2.0 * PI * frequency
    val S = 1.0
    val alpha = sin(w0) / 2.0 * sqrt((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0)

    val b0 = A * ((A + 1.0) - (A - 1.0) * cos(w0) + 2.0 * sqrt(A) * alpha)
    val b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cos(w0))
    val b2 = A * ((A + 1.0) - (A - 1.0) * cos(w0) - 2.0 * sqrt(A) * alpha)
    val a0 = (A + 1.0) + (A - 1.0) * cos(w0) + 2.0 * sqrt(A) * alpha
    val a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cos(w0))
    val a2 = (A + 1.0) + (A - 1.0) * cos(w0) - 2.0 * sqrt(A) * alpha

    setNormalizedCoefficients(a0, a1, a2, b0, b1, b2)
  }

  private fun setHighshelfCoefficients(normalizedFrequency: Double, dbGain: Double) {
    val frequency = max(0.0, min(1.0, normalizedFrequency))
    val A = 10.0.pow(dbGain / 40.0)

    if (frequency == 1.0) {
      setNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
      return
    }

    if (frequency <= 0.0) {
      setNormalizedCoefficients(1.0, 0.0, 0.0, A * A, 0.0, 0.0)
      return
    }

    val w0 = PI * frequency
    val S = 1.0
    val alpha = sin(w0) / 2.0 * sqrt((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0)

    val b0 = A * ((A + 1.0) - (A - 1.0) * cos(w0) + 2.0 * sqrt(A) * alpha)
    val b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cos(w0))
    val b2 = A * ((A + 1.0) + (A - 1.0) * cos(w0) - 2.0 * sqrt(A) * alpha)
    val a0 = (A + 1.0) - (A - 1.0) * cos(w0) + 2.0 * sqrt(A) * alpha
    val a1 = 2.0 * ((A - 1.0) + (A + 1.0) * cos(w0))
    val a2 = (A + 1.0) - (A - 1.0) * cos(w0) - 2.0 * sqrt(A) * alpha

    setNormalizedCoefficients(a0, a1, a2, b0, b1, b2)
  }

  private fun setPeakingCoefficients(normalizedFrequency: Double, resonance: Double, dbGain: Double) {
    val frequency = max(0.0, min(1.0, normalizedFrequency))
    val Q = max(0.0, resonance)
    val A = 10.0.pow(dbGain / 40.0)

    if (frequency <= 0.0 || frequency >= 1.0) {
      setNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
      return
    }

    if (Q <= 0.0) {
      setNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
      return
    }

    val w0 = 2.0 * PI * frequency
    val alpha = sin(w0) / (2.0 * Q)

    val b0 = 1.0 + alpha * A
    val b1 = -2.0 * cos(w0)
    val b2 = 1.0 - alpha * A
    val a0 = 1.0 + alpha / A
    val a1 = -2.0 * cos(w0)
    val a2 = 1.0 - alpha / A

    setNormalizedCoefficients(a0, a1, a2, b0, b1, b2)
  }

  private fun setNotchCoefficients(normalizedFrequency: Double, resonance: Double) {
    val frequency = max(0.0, min(1.0, normalizedFrequency))
    val Q = max(0.0, resonance)

    if (frequency <= 0.0 || frequency >= 1.0) {
      setNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
      return
    }

    if (Q <= 0.0) {
      setNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
      return
    }

    val w0 = 2.0 * PI * frequency
    val alpha = sin(w0) / (2.0 * Q)

    val b0 = 1.0
    val b1 = -2.0 * cos(w0)
    val b2 = 1.0
    val a0 = 1.0 + alpha
    val a1 = -2.0 * cos(w0)
    val a2 = 1.0 - alpha

    setNormalizedCoefficients(a0, a1, a2, b0, b1, b2)
  }

  private fun setAllpassCoefficients(normalizedFrequency: Double, resonance: Double) {
    val frequency = max(0.0, min(1.0, normalizedFrequency))
    val Q = max(0.0, resonance)

    if (frequency <= 0.0 || frequency >= 1.0) {
      setNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
      return
    }

    if (Q <= 0.0) {
      setNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
      return
    }

    val w0 = 2.0 * PI * frequency
    val alpha = sin(w0) / (2.0 * Q)

    val b0 = 1.0 - alpha
    val b1 = -2.0 * cos(w0)
    val b2 = 1.0 + alpha
    val a0 = 1.0 + alpha
    val a1 = -2.0 * cos(w0)
    val a2 = 1.0 - alpha

    setNormalizedCoefficients(a0, a1, a2, b0, b1, b2)
  }

  @RequiresApi(Build.VERSION_CODES.N)
  private fun applyFilter() {
    val currentTime = context.getCurrentTime()
    var normalizedFrequency = frequency.getValueAtTime(currentTime) / Constants.NYQUIST_FREQUENCY

    if (detune.getValueAtTime(currentTime) != 0.0) {
      normalizedFrequency *= 2.0.pow(detune.getValueAtTime(currentTime) / 1200.0)
    }

    when (filterType) {
      FilterType.LOWPASS -> {
        setLowpassCoefficients(normalizedFrequency, Q.getValueAtTime(currentTime))
      }
      FilterType.HIGHPASS -> {
        setHighpassCoefficients(normalizedFrequency, Q.getValueAtTime(currentTime))
      }
      FilterType.BANDPASS -> {
        setBandpassCoefficients(normalizedFrequency, Q.getValueAtTime(currentTime))
      }
      FilterType.LOWSHELF -> {
        setLowshelfCoefficients(normalizedFrequency, gain.getValueAtTime(currentTime))
      }
      FilterType.HIGHSHELF -> {
        setHighshelfCoefficients(normalizedFrequency, gain.getValueAtTime(currentTime))
      }
      FilterType.PEAKING -> {
        setPeakingCoefficients(normalizedFrequency, Q.getValueAtTime(currentTime), gain.getValueAtTime(currentTime))
      }
      FilterType.NOTCH -> {
        setNotchCoefficients(normalizedFrequency, Q.getValueAtTime(currentTime))
      }
      FilterType.ALLPASS -> {
        setAllpassCoefficients(normalizedFrequency, Q.getValueAtTime(currentTime))
      }
    }
  }

  @RequiresApi(Build.VERSION_CODES.N)
  override fun process(playbackParameters: PlaybackParameters) {
    resetCoefficients()
    applyFilter()

    var x1 = this.x1
    var x2 = this.x2
    var y1 = this.y1
    var y2 = this.y2

    val b0 = this.b0
    val b1 = this.b1
    val b2 = this.b2
    val a1 = this.a1
    val a2 = this.a2

    for (i in 0 until playbackParameters.audioBuffer.length) {
      val input = playbackParameters.audioBuffer.getChannelData(0)[i]
      val output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2

      x2 = x1
      x1 = input.toDouble()
      y2 = y1
      y1 = output

      for (j in 0 until playbackParameters.audioBuffer.numberOfChannels) {
        playbackParameters.audioBuffer.getChannelData(j)[i] = output.toInt().toShort()
      }
    }

    super.process(playbackParameters)
  }
}
