package com.swmansion.audioapi.system

/**
 * Lifecycle state of the native audio recorder.
 *
 * Mirrors C++ `audioapi::RecorderState`
 */
enum class RecorderState {
  IDLE,
  RECORDING,
  PAUSED,
  ;

  companion object {
    fun fromOrdinal(ordinal: Int): RecorderState = entries.getOrElse(ordinal) { IDLE }
  }
}
