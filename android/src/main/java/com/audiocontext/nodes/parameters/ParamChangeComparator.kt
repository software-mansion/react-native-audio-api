package com.audiocontext.nodes.parameters

class ParamChangeComparator {
  companion object: Comparator<ParamChange> {
    override fun compare(o1: ParamChange, o2: ParamChange): Int {
      return when {
        o1.startTime > o2.startTime -> 1
        o1.startTime < o2.startTime -> -1
        else ->
          when {
            o1.endTime > o2.endTime -> 1
            o1.endTime < o2.endTime -> -1
            else -> 0
          }
      }
    }
  }
}
