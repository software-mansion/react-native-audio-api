package com.audiocontext.nodes.audionode

enum class Channelnterpretation {
  SPEAKERS,
  DISCRETE;

  companion object {
    fun fromString(interpretation: String): Channelnterpretation {
      return when (interpretation.lowercase()) {
        "speakers" -> SPEAKERS
        "discrete" -> DISCRETE
        else -> throw IllegalArgumentException("Invalid channel interpretation")
      }
    }

    fun toString(interpretation: Channelnterpretation): String {
      return when (interpretation) {
        SPEAKERS -> "speakers"
        DISCRETE -> "discrete"
      }
    }
  }
}
