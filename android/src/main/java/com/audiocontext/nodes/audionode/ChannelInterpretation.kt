package com.audiocontext.nodes.audionode

enum class ChannelInterpretation {
  SPEAKERS,
  DISCRETE,
  ;

  companion object {
    fun fromString(interpretation: String): ChannelInterpretation =
      when (interpretation.lowercase()) {
        "speakers" -> SPEAKERS
        "discrete" -> DISCRETE
        else -> throw IllegalArgumentException("Invalid channel interpretation")
      }

    fun toString(interpretation: ChannelInterpretation): String =
      when (interpretation) {
        SPEAKERS -> "speakers"
        DISCRETE -> "discrete"
      }
  }
}
