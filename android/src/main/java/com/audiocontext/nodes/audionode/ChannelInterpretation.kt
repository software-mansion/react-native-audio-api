package com.audiocontext.nodes.audionode

enum class ChannelInterpretation {
  SPEAKERS,
  DISCRETE;

  companion object {
    fun fromString(interpretation: String): ChannelInterpretation {
      return when (interpretation.lowercase()) {
        "speakers" -> SPEAKERS
        "discrete" -> DISCRETE
        else -> throw IllegalArgumentException("Invalid channel interpretation")
      }
    }

    fun toString(interpretation: ChannelInterpretation): String {
      return when (interpretation) {
        SPEAKERS -> "speakers"
        DISCRETE -> "discrete"
      }
    }
  }
}
