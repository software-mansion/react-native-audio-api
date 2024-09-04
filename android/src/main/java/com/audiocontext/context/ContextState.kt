package com.audiocontext.context

enum class ContextState {
  RUNNING,
  CLOSED,
  ;

  companion object {
    fun fromString(state: String): ContextState =
      when (state.lowercase()) {
        "running" -> RUNNING
        "closed" -> CLOSED
        else -> throw IllegalArgumentException("Unknown context state: $state")
      }

    fun toString(state: ContextState): String =
      when (state) {
        RUNNING -> "running"
        CLOSED -> "closed"
      }
  }
}
