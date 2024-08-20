package com.audiocontext.nodes.filter

import android.os.Build
import androidx.annotation.RequiresApi
import com.audiocontext.context.BaseAudioContext
import com.audiocontext.nodes.AudioNode
import com.audiocontext.parameters.AudioParam
import com.audiocontext.parameters.PlaybackParameters
import com.audiocontext.utils.Constants
import kotlin.math.*

//https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html
//https://github.com/LabSound/LabSound/blob/main/src/internal/src/Biquad.cpp

class BiquadFilterNode(context: BaseAudioContext): AudioNode(context) {
  override val numberOfInputs: Int = 1
  override val numberOfOutputs: Int = 1
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
  private var a1 = 0.0
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

  private fun reset() {
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

  private fun setLowpassCoefficients(cutOffFrequency: Double, Q: Double) {
    val cutOff = max(0.0, min(cutOffFrequency, Constants.NYQUIST_FREQUENCY))

    if (cutOff == 1.0) {
      setNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
    } else if (cutOff > 0.0) {
      val resonance = max(0.0, Q)
      val g = 10.0.pow(resonance / 20.0)
      val d = sqrt((4.0 - sqrt(16.0 - 16.0 / (g * g))) / 2.0)

      val theta = 2.0 * PI * cutOff
      val sn = 0.5 * d * sin(theta)
      val beta = 0.5 * (1.0 - sn) / (1.0 + sn)
      val gamma = (0.5 + beta) * cos(theta)
      val alpha = 0.25 * (0.5 + beta - gamma)

      val b0 = 2.0 * alpha
      val b1 = 4.0 * alpha
      val b2 = 2.0 * alpha
      val a1 = -2.0 * gamma
      val a2 = 2.0 * beta

      setNormalizedCoefficients(1.0, a1, a2, b0, b1, b2)
    } else {
      setNormalizedCoefficients(1.0, 0.0, 0.0, 0.0, 0.0, 0.0)
    }
  }

  private fun setHighpassCoefficients(frequency: Double, Q: Double) {
    val cutOff = max(0.0, min(frequency, 1.0))

    if (cutOff == 1.0) {
      setNormalizedCoefficients(1.0, 0.0, 0.0, 0.0, 0.0, 0.0)
    } else if (cutOff > 0.0) {
      val resonance = max(0.0, Q)
      val g = 10.0.pow(resonance / 20.0)
      val d = sqrt((4.0 - sqrt(16.0 - 16.0 / (g * g))) / 2.0)

      val theta = 2.0 * PI * cutOff
      val sn = 0.5 * d * sin(theta)
      val beta = 0.5 * (1.0 - sn) / (1.0 + sn)
      val gamma = (0.5 + beta) * cos(theta)
      val alpha = 0.25 * (0.5 + beta - gamma)

      val b0 = 2.0 * alpha
      val b1 = -4.0 * alpha
      val b2 = 2.0 * alpha
      val a1 = -2.0 * gamma
      val a2 = 2.0 * beta

      setNormalizedCoefficients(1.0, a1, a2, b0, b1, b2)
    } else {
      setNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
    }
  }

  private fun setBandpassCoefficients(frequency: Double, Q: Double) {
    val cutOff = max(0.0, frequency)
    val resonance = max(0.0, Q)

    if (cutOff > 0 && cutOff < 1) {
      if (resonance > 0) {
        val w0 = PI * cutOff
        val alpha = sin(w0) / (2.0 * resonance)

        val b0 = alpha
        val b1 = 0.0
        val b2 = -alpha
        val a0 = 1.0 + alpha
        val a1 = -2.0 * cos(w0)
        val a2 = 1.0 - alpha

        setNormalizedCoefficients(a0, a1, a2, b0, b1, b2)
      } else {
        setNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
      }
    } else {
      setNormalizedCoefficients(1.0, 0.0, 0.0, 0.0, 0.0, 0.0)
    }
  }

  private fun setLowshelfCoefficients(frequency: Double, gain: Double) {
    val cutOff = max(0.0, min(frequency, 1.0))
    val A = 10.0.pow(gain / 40.0)

    if (cutOff == 1.0) {
      setNormalizedCoefficients(1.0, 0.0, 0.0, A * A, 0.0, 0.0)
    } else if (cutOff > 0.0) {
      val w0 = PI * cutOff
      val S = 1.0
      val alpha = sin(w0) / 2.0 * sqrt((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0)

      val b0 = A * ((A + 1.0) - (A - 1.0) * cos(w0) + 2.0 * sqrt(A) * alpha)
      val b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cos(w0))
      val b2 = A * ((A + 1.0) - (A - 1.0) * cos(w0) - 2.0 * sqrt(A) * alpha)
      val a0 = (A + 1.0) + (A - 1.0) * cos(w0) + 2.0 * sqrt(A) * alpha
      val a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cos(w0))
      val a2 = (A + 1.0) + (A - 1.0) * cos(w0) - 2.0 * sqrt(A) * alpha

      setNormalizedCoefficients(a0, a1, a2, b0, b1, b2)
    } else {
      setNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
    }
  }

  private fun setHighshelfCoefficients(frequency: Double, gain: Double) {
    val cutOff = max(0.0, min(frequency, 1.0))
    val A = 10.0.pow(gain / 40.0)

    if (cutOff == 1.0) {
      setNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
    } else if (cutOff > 0.0) {
      val w0 = PI * cutOff
      val S = 1.0
      val alpha = sin(w0) / 2.0 * sqrt((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0)

      val b0 = A * ((A + 1.0) - (A - 1.0) * cos(w0) + 2.0 * sqrt(A) * alpha)
      val b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cos(w0))
      val b2 = A * ((A + 1.0) + (A - 1.0) * cos(w0) - 2.0 * sqrt(A) * alpha)
      val a0 = (A + 1.0) - (A - 1.0) * cos(w0) + 2.0 * sqrt(A) * alpha
      val a1 = 2.0 * ((A - 1.0) + (A + 1.0) * cos(w0))
      val a2 = (A + 1.0) - (A - 1.0) * cos(w0) - 2.0 * sqrt(A) * alpha

      setNormalizedCoefficients(a0, a1, a2, b0, b1, b2)
    } else {
      setNormalizedCoefficients(1.0, 0.0, 0.0, A*A, 0.0, 0.0)
    }
  }

  private fun setPeakingCoefficients(frequency: Double, Q: Double, gain: Double) {
    val cutOff = max(0.0, min(frequency, 1.0))
    val resonance = max(0.0, Q)
    val A = 10.0.pow(gain / 40.0)

    if (cutOff > 0 && cutOff < 1) {
      if (resonance > 0) {
        val w0 = PI * cutOff
        val alpha = sin(w0) / (2.0 * resonance)

        val b0 = 1.0 + alpha * A
        val b1 = -2.0 * cos(w0)
        val b2 = 1.0 - alpha * A
        val a0 = 1.0 + alpha / A
        val a1 = -2.0 * cos(w0)
        val a2 = 1.0 - alpha / A

        setNormalizedCoefficients(a0, a1, a2, b0, b1, b2)
      } else {
        setNormalizedCoefficients(1.0, 0.0, 0.0, A*A, 0.0, 0.0)
      }
    } else {
      setNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
    }
  }

  private fun setNotchCoefficients(frequency: Double, Q: Double) {
    val cutOff = max(0.0, min(frequency, 1.0))
    val resonance = max(0.0, Q)

    if (cutOff > 0 && cutOff < 1) {
      if (resonance > 0) {
        val w0 = PI * cutOff
        val alpha = sin(w0) / (2.0 * resonance)

        val b0 = 1.0
        val b1 = -2.0 * cos(w0)
        val b2 = 1.0
        val a0 = 1.0 + alpha
        val a1 = -2.0 * cos(w0)
        val a2 = 1.0 - alpha

        setNormalizedCoefficients(a0, a1, a2, b0, b1, b2)
      } else {
        setNormalizedCoefficients(1.0, 0.0, 0.0, 0.0, 0.0, 0.0)
      }
    } else {
      setNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
    }
  }

  private fun setAllpassCoefficients(frequency: Double, Q: Double) {
    val cutOff = max(0.0, min(frequency, 1.0))
    val resonance = max(0.0, Q)

    if (cutOff > 0 && cutOff < 1) {
      if (resonance > 0) {
        val w0 = PI * cutOff
        val alpha = sin(w0) / (2.0 * resonance)

        val b0 = 1.0 - alpha
        val b1 = -2.0 * cos(w0)
        val b2 = 1.0 + alpha
        val a0 = 1.0 + alpha
        val a1 = -2.0 * cos(w0)
        val a2 = 1.0 - alpha

        setNormalizedCoefficients(a0, a1, a2, b0, b1, b2)
      } else {
        setNormalizedCoefficients(1.0, 0.0, 0.0, -1.0, 0.0, 0.0)
      }
    } else {
      setNormalizedCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
    }
  }

  @RequiresApi(Build.VERSION_CODES.N)
  private fun applyFilter() {
    var normalizedFrequency = frequency.getValueAtTime(context.getCurrentTime()) / Constants.NYQUIST_FREQUENCY

    if (detune.getValueAtTime(context.getCurrentTime()) != 0.0) {
      normalizedFrequency *= 2.0.pow(detune.getValueAtTime(context.getCurrentTime()) / 1200.0)
    }

    when (filterType) {
      FilterType.LOWPASS -> {
        setLowpassCoefficients(normalizedFrequency, Q.getValueAtTime(context.getCurrentTime()))
      }
      FilterType.HIGHPASS -> {
        setHighpassCoefficients(normalizedFrequency, Q.getValueAtTime(context.getCurrentTime()))
      }
      FilterType.BANDPASS -> {
        setBandpassCoefficients(normalizedFrequency, Q.getValueAtTime(context.getCurrentTime()))
      }
      FilterType.LOWSHELF -> {
        setLowshelfCoefficients(normalizedFrequency, gain.getValueAtTime(context.getCurrentTime()))
      }
      FilterType.HIGHSHELF -> {
        setHighshelfCoefficients(normalizedFrequency, gain.getValueAtTime(context.getCurrentTime()))
      }
      FilterType.PEAKING -> {
        setPeakingCoefficients(normalizedFrequency, Q.getValueAtTime(context.getCurrentTime()), gain.getValueAtTime(context.getCurrentTime()))
      }
      FilterType.NOTCH -> {
        setNotchCoefficients(normalizedFrequency, Q.getValueAtTime(context.getCurrentTime()))
      }
      FilterType.ALLPASS -> {
        setAllpassCoefficients(normalizedFrequency, Q.getValueAtTime(context.getCurrentTime()))
      }
    }
  }

  @RequiresApi(Build.VERSION_CODES.N)
  override fun process(playbackParameters: PlaybackParameters) {
    reset()
    applyFilter()
    for (i in playbackParameters.buffer.indices) {
      val input = playbackParameters.buffer[i]
      val output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2

      x2 = x1
      x1 = input.toDouble()
      y2 = y1
      y1 = output

      playbackParameters.buffer[i] = output.toInt().toShort()
    }

    super.process(playbackParameters)
  }
}
