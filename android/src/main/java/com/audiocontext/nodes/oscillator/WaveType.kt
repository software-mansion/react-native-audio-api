package com.audiocontext.nodes.oscillator

import kotlin.math.abs
import kotlin.math.floor
import kotlin.math.sin

enum class WaveType {
  SINE,
  SQUARE,
  SAWTOOTH,
  TRIANGLE;

  private fun sineWave(wavePhase: Double): Short {
    return (sin(wavePhase) * Short.MAX_VALUE).toInt().toShort()
  }

  private fun squareWave(wavePhase: Double): Short {
    return ((if (sin(wavePhase) >= 0) 1 else -1) * Short.MAX_VALUE).toShort()
  }

  private fun sawtoothWave(wavePhase: Double): Short {
    return ((2 * (wavePhase / (2 * Math.PI) - floor(wavePhase / (2 * Math.PI) + 0.5))) * Short.MAX_VALUE).toInt().toShort()
  }

  private fun triangleWave(wavePhase: Double): Short {
    return ((2 * abs(2 * (wavePhase / (2 * Math.PI) - floor(wavePhase / (2 * Math.PI) + 0.5))) - 1) * Short.MAX_VALUE).toInt().toShort()
  }

  private fun getWaveValue(wavePhase: Double): Short {
    return when(this) {
      SINE -> sineWave(wavePhase)
      SQUARE -> squareWave(wavePhase)
      SAWTOOTH -> sawtoothWave(wavePhase)
      TRIANGLE -> triangleWave(wavePhase)
    }
  }

  companion object {
    fun fromString(type: String): WaveType {
      return when (type.lowercase()) {
        "sine" -> SINE
        "square" -> SQUARE
        "sawtooth" -> SAWTOOTH
        "triangle" -> TRIANGLE
        else -> throw IllegalArgumentException("Unknown wave type: $type")
      }
    }

    fun toString(type: WaveType): String {
      return when (type) {
        SINE -> "sine"
        SQUARE -> "square"
        SAWTOOTH -> "sawtooth"
        TRIANGLE -> "triangle"
      }
    }

    fun getWaveBufferElement(wavePhase: Double, waveType: WaveType): Short {
      return waveType.getWaveValue(wavePhase)
    }
  }
}
