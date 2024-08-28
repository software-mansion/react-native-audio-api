package com.audiocontext.nodes.audionode

enum class ChannelCountMode {
  MAX,
  CLAMPED_MAX,
  EXPLICIT;

  companion object {
    fun fromString(mode: String): ChannelCountMode {
      return when (mode.lowercase()) {
        "max" -> MAX
        "clamped-max" -> CLAMPED_MAX
        "explicit" -> EXPLICIT
        else -> throw IllegalArgumentException("Invalid channel count mode")
      }
    }

    fun toString(mode: ChannelCountMode): String {
      return when (mode) {
        MAX -> "max"
        CLAMPED_MAX -> "clamped-max"
        EXPLICIT -> "explicit"
      }
    }
  }
}
