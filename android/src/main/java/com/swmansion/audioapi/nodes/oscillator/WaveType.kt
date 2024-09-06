package com.swmansion.audioapi.nodes.oscillator

import kotlin.math.abs
import kotlin.math.floor
import kotlin.math.sin

enum class WaveType {
  SINE,
  SQUARE,
  SAWTOOTH,
  TRIANGLE,
  ;

  private fun sineWave(wavePhase: Double): Float = sin(wavePhase).toFloat()

  private fun squareWave(wavePhase: Double): Float = (if (sin(wavePhase) >= 0) 1.0 else -1.0).toFloat()

  private fun sawtoothWave(wavePhase: Double): Float =
    (2 * (wavePhase / (2 * Math.PI) - floor(wavePhase / (2 * Math.PI) + 0.5))).toFloat()

  private fun triangleWave(wavePhase: Double): Float =
    (2 * abs(2 * (wavePhase / (2 * Math.PI) - floor(wavePhase / (2 * Math.PI) + 0.5))) - 1).toFloat()

  private fun getWaveValue(wavePhase: Double): Float =
    when (this) {
      SINE -> sineWave(wavePhase)
      SQUARE -> squareWave(wavePhase)
      SAWTOOTH -> sawtoothWave(wavePhase)
      TRIANGLE -> triangleWave(wavePhase)
    }

  companion object {
    fun fromString(type: String): WaveType =
      when (type.lowercase()) {
        "sine" -> SINE
        "square" -> SQUARE
        "sawtooth" -> SAWTOOTH
        "triangle" -> TRIANGLE
        else -> throw IllegalArgumentException("Unknown wave type: $type")
      }

    fun toString(type: WaveType): String =
      when (type) {
        SINE -> "sine"
        SQUARE -> "square"
        SAWTOOTH -> "sawtooth"
        TRIANGLE -> "triangle"
      }

    fun getWaveBufferElement(
      wavePhase: Double,
      waveType: WaveType,
    ): Float = waveType.getWaveValue(wavePhase)
  }
}
