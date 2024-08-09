package com.audiocontext.nodes.filter

enum class FilterType {
  LOWPASS,
  HIGHPASS,
  BANDPASS,
  LOWSHELF,
  HIGHSHELF,
  PEAKING,
  NOTCH,
  ALLPASS;

  companion object {
    fun fromString(type: String): FilterType {
      return when (type.lowercase()) {
        "lowpass" -> LOWPASS
        "highpass" -> HIGHPASS
        "bandpass" -> BANDPASS
        "lowshelf" -> LOWSHELF
        "highshelf" -> HIGHSHELF
        "peaking" -> PEAKING
        "notch" -> NOTCH
        "allpass" -> ALLPASS
        else -> throw IllegalArgumentException("Invalid filter type")
      }
    }

    fun toString(type: FilterType): String {
      return when (type) {
        LOWPASS -> "lowpass"
        HIGHPASS -> "highpass"
        BANDPASS -> "bandpass"
        LOWSHELF -> "lowshelf"
        HIGHSHELF -> "highshelf"
        PEAKING -> "peaking"
        NOTCH -> "notch"
        ALLPASS -> "allpass"
      }
    }
  }
}
